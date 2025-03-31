#include "../../headers/Server.hpp"

Server::Server(std::string server_host) : _addr_len(sizeof(_address)), _name(server_host), _port("8080"), _root("/www")
{
	std::cout	<< "Default constructor"
				<< "\nserver_host: " << _name
				<< "\nAddr len: " << _addr_len
				<< std::endl;
}

Server::Server(const Server& other)
{
	std::cout << "Copy constructor" << std::endl;
	(void)other;
}

Server::~Server()
{
	std::cout	<< "Default destructor"
				<< "\nClosing server"
				<< std::endl;
}

Server&	Server::operator=(const Server& other)
{
	std::cout << "Copy assignment operator" << std::endl;
	(void)other;
	return (*this);
}

bool	Server::initialize(std::vector<int> ports)
{
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
	{
		std::cerr << "Failed to create epoll instance" << std::endl;
		return (false);
	}
	for (size_t i = 0; i < ports.size(); i++)
	{
		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1)
		{
			std::cerr << "Socket creation failed for port: " << ports[i] << std::endl;
			continue;
		}

		int opt = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Makes sure the server can bind to the same port immediately after restarting.
		
		memset(&_address, 0, sizeof(_address)); // Sets all bytes to 0 in address struct
		_address.sin_family = AF_INET; // Sets Adress Family to IPv4
		_address.sin_addr.s_addr = inet_addr(_name.c_str()); // INADDR_ANY; // Assigns the address to INADDR_ANY (which is 0.0.0.0)
		_address.sin_port = htons(ports[i]); // Converts port number from host byte order to network byte order.
		if (bind(server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
		{
			std::cerr << "Binding failed on port: " << ports[i] << std::endl;
			close(server_fd);
			continue;
		}
		if (listen(server_fd, 10) < 0)
		{
			std::cerr << "Listening failed on port: " << ports[i] << std::endl;
			close(server_fd);
			continue;
		}
		std::cout << "Listening on port " << ports[i] << std::endl;
        _server_fds.push_back(server_fd);

		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.fd = server_fd;

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
		{
			std::cerr << "Failed to add server socket to epoll" << std::endl;
			close(server_fd);
			continue;
		}
	}

	return (!_server_fds.empty());
}

void Server::run(void)
{
	struct epoll_event ready_events[MAX_EVENTS];

	while (true)
	{
		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, -1);
		if (event_count == -1)
		{
			std::cerr << "Epoll wait error" << std::endl;
			continue;
		}
		for (int i = 0; i < event_count; i++)
		{
			int fd = ready_events[i].data.fd;

			if (find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end())
                connectClient(_epoll_fd, fd);
			else
			{
				if (ready_events[i].events & EPOLLIN)
					handleRead(_epoll_fd, fd, *this);
				if (ready_events[i].events & EPOLLOUT)
					handleWrite(_epoll_fd, fd);
			}
		}
	}
	close(_epoll_fd);
}

void Server::connectClient(int _epoll_fd, int server_fd)
{
	struct sockaddr_in	client_addr;
    socklen_t			client_len = sizeof(client_addr);
	
	int new_client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
	if (new_client < 0)
	{
		std::cerr << "Failed to accept connection" << std::endl;
		return;
	}

	setNonBlocking(new_client);

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.fd = new_client;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client, &event) == -1)
	{
		std::cerr << "Failed to add client to epoll" << std::endl;
		close(new_client);
		return;
	}
	_client_buffers[new_client] = "";
	std::cout << "Client connected on socket " << server_fd << ": " << new_client << std::endl;
}

void	Server::handleRead(int _epoll_fd, int client_fd, const Server& server)
{
	std::vector<char> vbuffer(BUFFER_SIZE);
	std::string full_request;
	ssize_t bytes_received;
	do
	{
		bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
		if (bytes_received > 0)
		{
			std::cout << "bytes_received: " << bytes_received << std::endl;
			_client_buffers[client_fd].append(vbuffer.data(), bytes_received);
		}
		else if (bytes_received == 0)
		{
			std::cout << "Client disconnected: " << client_fd << std::endl;
			epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
			close(client_fd);
			_client_buffers.erase(client_fd);
			return ;
		}
		else
			break;
	} while (bytes_received > 0);
	
	std::cout << "\n\nClient [" << client_fd << "] full request: \n\n"<< _client_buffers[client_fd] << std::endl;
	std::cout << "\n\n\nend of buffer..." << std::endl;

	HttpRequest httprequest;
	if (!parseRequest(_client_buffers[client_fd], httprequest, server))
	{
		std::cout << "parse request failed" << std::endl;
		_responses[client_fd] = ER400;
	}
	else
		_responses[client_fd] = generateHttpResponse(httprequest);

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client_fd;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
	_client_buffers.erase(client_fd);
}

void	Server::handleWrite(int _epoll_fd, int client_fd)
{
	if (_responses[client_fd].empty())
		return;

	int bytes_sent = send(client_fd, _responses[client_fd].c_str(), _responses[client_fd].size(), 0);
	if (bytes_sent <= 0) 
	{
		std::cout << "Error writing to client: " << client_fd << std::endl;
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
		close(client_fd);
		_client_buffers.erase(client_fd);
		_responses.erase(client_fd);
		return;
	}

	_responses[client_fd] = _responses[client_fd].substr(bytes_sent); 

	if (_responses[client_fd].empty()) 
	{
		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.fd = client_fd;
		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
	}
}

void	Server::setNonBlocking(int socket)
{
	std::cout << "Setting non-blocking mode for socket: " << socket << std::endl;
	int flag = fcntl(socket, F_GETFL, 0);
	if (flag == -1)
	{
		std::cerr << flag << " fcntl get failed\n";
		return ;
	}
	if (fcntl(socket, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl set failed\n";
		return ;
	}
}

const std::string	Server::getServerInfo(int i) const
{
	if (i == 0)
		return (_name);
	else if (i == 1)
		return (_port);
	else
		return (_root);
}

int	Server::getEpollFd() const
{
	return (_epoll_fd);
}