#include "../../headers/Server.hpp"

Server::Server() : _addr_len(sizeof(_address)), _name("localhost"), _port("8080"), _root("/www")
{
	std::cout	<< "Default constructor"
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

bool Server::initialize(const std::vector<std::pair<std::string, std::vector<int> > >& server_configs)
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll instance" << std::endl;
        return false;
    }

    std::set<std::pair<std::string, int> > bound_servers;

    for (size_t i = 0; i < server_configs.size(); i++)
    {
        const std::string& host = server_configs[i].first;
        const std::vector<int>& ports = server_configs[i].second;

        for (size_t j = 0; j < ports.size(); j++)
        {
            std::pair<std::string, int> host_port_pair = std::make_pair(host, ports[j]);
            if (bound_servers.find(host_port_pair) != bound_servers.end())
            {
                std::cerr << "Skipping duplicate binding: " << host << ":" << ports[j] << std::endl;
                continue;
            }
            bound_servers.insert(host_port_pair);

            int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1)
            {
                std::cerr << "Socket creation failed for " << host << ":" << ports[j] << std::endl;
                continue;
            }

            int opt = 1;
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_port = htons(ports[j]);

            if (host == "*" || host == "0.0.0.0")
            {
                address.sin_addr.s_addr = INADDR_ANY;
            }
            else if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1)
            {
                struct addrinfo hints, *res;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_flags = AI_PASSIVE;

                if (getaddrinfo(host.c_str(), NULL, &hints, &res) != 0)
                {
                    std::cerr << "Failed to resolve hostname: " << host << std::endl;
                    close(socket_fd);
                    continue;
                }
                struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
                address.sin_addr = addr->sin_addr;
                freeaddrinfo(res);
            }

            if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            {
                std::cerr << "Binding failed on " << host << ":" << ports[j] << std::endl;
                close(socket_fd);
                continue;
            }

            if (listen(socket_fd, 10) < 0)
            {
                std::cerr << "Listening failed on " << host << ":" << ports[j] << std::endl;
                close(socket_fd);
                continue;
            }

            std::cout << "Listening on " << host << ":" << ports[j] << std::endl;
            _server_fds.push_back(socket_fd);

            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = socket_fd;

            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1)
            {
                std::cerr << "Failed to add server socket to epoll" << std::endl;
                close(socket_fd);
                continue;
            }
        }
    }

    return !_server_fds.empty();
}

// bool	Server::initialize(std::vector<int> ports)
// {
// 	_epoll_fd = epoll_create1(0);
// 	if (_epoll_fd == -1)
// 	{
// 		std::cerr << "Failed to create epoll instance" << std::endl;
// 		return (false);
// 	}


// 	for (size_t i = 0; i < ports.size(); i++)
// 	{
// 		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
// 		if (server_fd == -1)
// 		{
// 			std::cerr << "Socket creation failed for port: " << ports[i] << std::endl;
// 			continue;
// 		}

// 		int opt = 1;
// 		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Makes sure the server can bind to the same port immediately after restarting.
		
// 		memset(&_address, 0, sizeof(_address)); // Sets all bytes to 0 in address struct
// 		_address.sin_family = AF_INET; // Sets Adress Family to IPv4
// 		_address.sin_addr.s_addr = INADDR_ANY; // Assigns the address to INADDR_ANY (which is 0.0.0.0)
// 		_address.sin_port = htons(ports[i]); // Converts port number from host byte order to network byte order.
// 		if (bind(server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
// 		{
// 			std::cerr << "Binding failed on port: " << ports[i] << std::endl;
// 			close(server_fd);
// 			continue;
// 		}
// 		if (listen(server_fd, 10) < 0)
// 		{
// 			std::cerr << "Listening failed on port: " << ports[i] << std::endl;
// 			close(server_fd);
// 			continue;
// 		}
// 		std::cout << "Listening on port " << ports[i] << std::endl;
//         _server_fds.push_back(server_fd);

// 		struct epoll_event event;
// 		event.events = EPOLLIN;
// 		event.data.fd = server_fd;

// 		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
// 		{
// 			std::cerr << "Failed to add server socket to epoll" << std::endl;
// 			close(server_fd);
// 			continue;
// 		}
// 	}

// 	return (!_server_fds.empty());
// }

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
				// for (client : clients)
				// {
				// 	for (int i = 0; i < client._fds.size(); ++i)
				// 		if (fd == client._fds[i])
				// 			break;
				// }
				// if (client._state == waiting_for_read)
				// 	readthis();
				// else if (client._state == request_sent)
				// 	parserequest();
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
	std::cout << "Client connected on socket: " << new_client << std::endl;
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
	Client client;
	client.readRequest(_client_buffers[client_fd], client_fd);
	clRequest cl_request = client.getClStructRequest(client_fd);
	if (!parseRequest(_client_buffers[client_fd], httprequest, server, cl_request))
	{
		std::cout << "parse request failed" << std::endl;
		_responses[client_fd] = ER400;
	}
	else
		_responses[client_fd] = generateHttpResponse(httprequest, cl_request);

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