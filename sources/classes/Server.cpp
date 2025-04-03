#include "../../headers/Server.hpp"

Server::Server() : _server_fd(-1), _addr_len(sizeof(_address)), _client_count(0), _name("localhost"), _port("8080"), _root("/www")
{
	std::cout	<< "Default constructor"
				<< "\nServer fd: " << _server_fd
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
	if (_server_fd != -1)
	{
		close(_server_fd);
		_server_fd = -1;
	}

}

Server&	Server::operator=(const Server& other)
{
	std::cout << "Copy assignment operator" << std::endl;
	(void)other;
	return (*this);
}

bool	Server::initialize(void)
{
	int opt = 1;
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Makes sure the server can bind to the same port immediately after restarting.
	if (_server_fd == -1)
	{
		std::cerr << "Socket creation failed" << std::endl;
		return (false);
	}
	memset(&_address, 0, sizeof(_address));	// Sets all bytes to 0 in address struct
	_address.sin_family = AF_INET; 			// Sets Adress Family to IPv4
	_address.sin_addr.s_addr = INADDR_ANY;	// Assigns the address to INADDR_ANY (which is 0.0.0.0)
	_address.sin_port = htons(PORT);		// Converts port number from host byte order to network byte order.
	if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
	{
		std::cerr << "Binding failed" << std::endl;
		return (false);
	}
	if (listen(_server_fd, 10) < 0)
	{
		std::cerr << "Listening failed" << std::endl;
		return (false);
	}
	std::cout	<< "Server initialized"
				<< "\nServer fd: " << _server_fd
				<< "\nListening on port: " << PORT
				<< std::endl;
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
	{
		std::cerr << "Failed to create epoll instance" << std::endl;
		return (false);
	}
	return (true);
}

// void Server::run(void)
// {
// 	struct epoll_event event;
// 	event.events = EPOLLIN;
// 	event.data.fd = _server_fd;

// 	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &event) == -1)
// 	{
// 		std::cerr << "Failed to add server socket to epoll" << std::endl;
// 		close(_epoll_fd);
// 		return;
// 	}

// 	struct epoll_event ready_events[MAX_EVENTS];
// 	while (true)
// 	{
// 		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, -1);
// 		if (event_count == -1)
// 		{
// 			std::cerr << "Epoll wait error" << std::endl;
// 			continue;
// 		}

// 		for (int i = 0; i < event_count; i++)
// 		{
// 			int fd = ready_events[i].data.fd;

// 			if (fd == _server_fd)
// 				connectClient(_epoll_fd);
// 			else
// 			{
// 				for (int i = 0; i < _client_count; ++i)
// 				{
// 					for (int o = 0; o < _clients[i].getClientFds(-1); ++o)
// 						if (_clients[i].getClientFds(o) == fd)
// 							_clients[i].handleEvent(fd);
// 				}
// 				// if (ready_events[i].events & EPOLLIN)
// 				// 	handleRead(_epoll_fd, fd, *this);
// 				// if (ready_events[i].events & EPOLLOUT)
// 				// 	handleWrite(_epoll_fd, fd);
// 			}
// 		}
// 	}
// 	close(_epoll_fd);
// }



void Server::run(void)
{
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = _server_fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &event) == -1)
	{
		std::cerr << "Failed to add server socket to epoll" << std::endl;
		close(_epoll_fd);
		return;
	}

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

			if (fd == _server_fd)
				connectClient(_epoll_fd);
			else
			{
				for (int i = 0; i < _client_count; ++i)
				{
					for (int o = 0; o < _clients[i].getClientFds(-1); ++o)
						if (_clients[i].getClientFds(o) == fd)
							_clients[i].handleEvent(fd);
				}
			}
		}
	}
	close(_epoll_fd);
}

void Server::connectClient(int _epoll_fd)
{
	int new_client_fd = accept(_server_fd, (struct sockaddr *)&_address, &_addr_len);
	if (new_client_fd < 0)
	{
		std::cerr << "Failed to accept connection" << std::endl;
		return;
	}

	setNonBlocking(new_client_fd);

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.fd = new_client_fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event) == -1)
	{
		std::cerr << "Failed to add client to epoll" << std::endl;
		close(new_client_fd);
		return;
	}
	std::cout << "Client connected: " << new_client_fd << std::endl;
	Client new_client(new_client_fd);

	this->_clients.push_back(new_client); 
	this->_client_count++;
}


// void Server::connectClient(int _epoll_fd)
// {
// 	int new_client = accept(_server_fd, (struct sockaddr *)&_address, &_addr_len);
// 	if (new_client < 0)
// 	{
// 		std::cerr << "Failed to accept connection" << std::endl;
// 		return;
// 	}

// 	setNonBlocking(new_client);

// 	struct epoll_event event;
// 	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
// 	event.data.fd = new_client;

// 	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client, &event) == -1)
// 	{
// 		std::cerr << "Failed to add client to epoll" << std::endl;
// 		close(new_client);
// 		return;
// 	}
// 	_client_buffers[new_client] = "";
// 	std::cout << "Client connected: " << new_client << std::endl;
// }

// void	Server::handleRead(int _epoll_fd, int client_fd, const Server& server)
// {
// 	std::vector<char> vbuffer(BUFFER_SIZE);
// 	std::string full_request;

// 	ssize_t	bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
// 	std::cout << "bytes read: " << bytes_received << std::endl;
// 	if (bytes_received <= 0)
// 	{
// 		std::cout << "Client disconnected: " << client_fd << std::endl;
// 		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
// 		close(client_fd);
// 		_client_buffers.erase(client_fd);
// 		_client_count--;
// 		return;
// 	}
// 	else
// 		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);

// 	while ((bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0)) > 0)
// 	{
// 		std::cout << "bytes_received: " << bytes_received << std::endl;
// 		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);
// 	}

// 	// ssize_t	bytes_received = 0;
// 	// do
// 	// {
// 	// 	bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
// 	// 	std::cout << "bytes_received: " << bytes_received << std::endl;
// 	// 	if (bytes_received > 0)
// 	// 	{
// 	// 		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
// 	// 		close(client_fd);
// 	// 		std::cout << "Client disconnected: " << client_fd << std::endl;
// 	// 		_client_buffers.erase(client_fd);
// 	// 		return ;
// 	// 	}
// 	// } while (bytes_received == BUFFER_SIZE);

// 	std::cout << "\n\nClient [" << client_fd << "] full request: \n\n"<< _client_buffers[client_fd] << std::endl;
// 	std::cout << "\n\n\nend of buffer..." << std::endl;

// 	HttpRequest httprequest;
// 	if (!parseRequest(_client_buffers[client_fd], httprequest, server))
// 	{
// 		std::cout << "parse request failed" << std::endl;
// 		_responses[client_fd] = ER400;
// 	}
// 	else
// 		_responses[client_fd] = generateHttpResponse(httprequest);

// 	struct epoll_event event;
// 	event.events = EPOLLIN | EPOLLOUT;
// 	event.data.fd = client_fd;
// 	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
// 	_client_buffers.erase(client_fd);
// }

void	Server::handleRead(int _epoll_fd, int client_fd, const Server& server)
{
	std::vector<char> vbuffer(BUFFER_SIZE);
	std::string full_request;

	ssize_t	bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
	std::cout << "bytes read: " << bytes_received << std::endl;
	if (bytes_received <= 0)
	{
		std::cout << "Client disconnected: " << client_fd << std::endl;
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
		close(client_fd);
		_client_buffers.erase(client_fd);
		_client_count--;
		return;
	}
	else
		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);

	while ((bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0)) > 0)
	{
		std::cout << "bytes_received: " << bytes_received << std::endl;
		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);
	}

	// ssize_t	bytes_received = 0;
	// do
	// {
	// 	bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
	// 	std::cout << "bytes_received: " << bytes_received << std::endl;
	// 	if (bytes_received > 0)
	// 	{
	// 		_client_buffers[client_fd].append(vbuffer.data(), bytes_received);
	// 	}
	// 	else
	// 	{
	// 		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
	// 		close(client_fd);
	// 		std::cout << "Client disconnected: " << client_fd << std::endl;
	// 		_client_buffers.erase(client_fd);
	// 		return ;
	// 	}
	// } while (bytes_received == BUFFER_SIZE);

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

void	Server::setNonBlocking(int fd)
{
	std::cout << "Setting non-blocking mode for fd: " << fd << std::endl;
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
	{
		std::cerr << flag << " fcntl get failed\n";
		return ;
	}
	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
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