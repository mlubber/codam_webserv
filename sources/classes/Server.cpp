#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"
#include "Configuration.hpp"

Server::Server(const Configuration& config) : _server_fds_amount(0), _addr_len(sizeof(_address)), _client_count(0), _config(config), _name("localhost"), _port("8080"), _root("/www")
{
	std::cout	<< "Default constructor"
				<< std::endl;
}

/* COPY CONSTRUCTOR */
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
	for (int i = 0; i < _server_fds_amount; ++i)
		if (close(_server_fds[i]) == -1)
			std::cerr << "CLOSING ERROR: Failed closing server_fd: " << _server_fds[i] << std::endl;
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

            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = socket_fd;

            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1)
            {
                std::cerr << "Failed to add server socket to epoll" << std::endl;
                close(socket_fd);
                continue;
            }
			_server_fds.push_back(socket_fd);
			_server_fds_amount++;
        }
    }

    return !_server_fds.empty();
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

			for (int i = 0; i < _server_fds_amount; ++i)
				if (fd == _server_fds[i])
					connectClient(_epoll_fd, fd);
			for (int i = 0; i < _client_count; ++i)
			{
				int client_amount = _clients[i]->getClientFds(-1);
				for (int o = 0; o < client_amount; ++o)
					if (_clients[i]->getClientFds(o) == fd)
						_clients[i]->handleEvent(*this);
			}
		}
	}
	close(_epoll_fd);
}

void Server::connectClient(int _epoll_fd, int server_fd)
{
	try
	{
		int new_client_fd = accept(server_fd, (struct sockaddr *)&_address, &_addr_len);
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
	
		Client* new_client = new Client(new_client_fd);
		this->_clients.push_back(new_client);
		this->_client_count++;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n' << "failed connecting new client" << std::endl;
	}

}

void Server::removeClient(Client* client)
{
	int client_fd = client->getClientFds(0);

	if (client->checkCgiPtr())
	{
		if (client->getCgiStruct().ets_pipe[1] != -1)
			if (close(client->getCgiStruct().ets_pipe[1]) == -1)
				std::cout << "SERVER ERROR: Failed closing cgi_pipe write_end " << std::endl;
		if (client->getCgiStruct().ste_pipe[0] != -1)
			if (close(client->getCgiStruct().ste_pipe[0]) == -1)
				std::cout << "SERVER ERROR: Failed closing cgi_pipe read_end " << std::endl;
	}

	int status = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
	if (status == -1)
		std::cout << "SERVER ERROR: Failed deleting client fd " << client_fd << " from epoll!" << std::endl;
	if (close(client_fd) == -1)
		std::cout << "SERVER ERROR: Failed closing client_fd " << client_fd << std::endl;

	delete client;
	std::cout << "Client disconnected: " << client_fd << std::endl;
	return ;
}

int	Server::recvFromSocket(Client& client)
{
	char 			buffer[SOCKET_BUFFER];
    ssize_t 		bytes_received;
    std::string& 	receivedData = client.getClientReceived();

	do
	{
		bytes_received = recv(client.getClientFds(0), buffer, SOCKET_BUFFER, 0);
		if (bytes_received > 0)
		{
			std::cout << "bytes received: " << bytes_received << std::endl;
			receivedData.append(buffer, bytes_received);
		}
		else if (bytes_received == 0)
		{
			// Client disconnected
			removeClient(&client);
			return 0;
		}
	} while (bytes_received > 0);

	std::cout <<  "\nDATA: \n" << receivedData << std::endl;

	std::cout <<  "\nDATA IN CLIENT: \n" << client.getClientReceived() << std::endl;
	// char		buffer[SOCKET_BUFFER];
	// std::string	data;
	// ssize_t		bytes_received;
	// int			client_fd = client.getClientFds(0);

	// do
	// {
	// 	bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
	// 	if (bytes_received <= 0)
	// 		break ;
	// 	std::cout << "bytes_received: " << bytes_received << std::endl;
	// 	data += buffer;
	// 	if (bytes_received == 0) // Not sure if correct, because we could just be at the end of what to read, without needing to close the connection to the client
	// 	{
	// 		removeClient(&client);
	// 		return (0);
	// 	}
	// } while (bytes_received > 0);
	// client.setReceivedData(data);
	// std::cout << data << std::endl;

	// // if (bytes_received == -1)
	// // 	return (-1);
	client.setClientState(parsing_request);
	parsingRequest(*this, client);
	return (1);
}



// void	Server::handleRead(int _epoll_fd, int client_fd, const Server& server)
// {
	// std::vector<char> vbuffer(BUFFER_SIZE);
	// std::string full_request;

	// ssize_t	bytes_received = recv(client_fd, vbuffer.data(), BUFFER_SIZE, 0);
	// std::cout << "bytes read: " << bytes_received << std::endl;
	// if (bytes_received <= 0)
	// {
	// 	std::cout << "Client disconnected: " << client_fd << std::endl;
	// 	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
	// 	close(client_fd);
	// 	_client_buffers.erase(client_fd);
	// 	_client_count--;
	// 	return;
	// }
	// else
	// 	_client_buffers[client_fd].append(vbuffer.data(), bytes_received);

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



// int	Server::recvFromSocket(Client& client)
// {
// 	// std::vector<char>	vbuffer(BUFFER_SIZE);
// 	char		buffer[SOCKET_BUFFER];
// 	std::string	data;
// 	ssize_t		bytes_received;
// 	int			client_fd = client.getClientFds(0);

// 	do
// 	{
// 		bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
// 		if (bytes_received > 0)
// 		{
// 			std::cout << "bytes_received: " << bytes_received << std::endl;
// 			data += buffer;
// 		}
// 		else if (bytes_received == 0)
// 		{
// 			std::cout << "Client disconnected: " << client_fd << std::endl;
// 			epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
// 			close(client_fd);
// 			_client_buffers.erase(client_fd);
// 			return ;
// 		}
// 		else
// 			break;
// 	} while (bytes_received > 0);
// 	client.setReceivedData(data);


// 	if (bytes_received == -1)
// 		return (-1);
// 	else if (bytes_received == 0)
// 	{
// 		// tell server to remove client
// 		return (0);
// 	}
// 	client.setClientState(parsing_request);
// 	return (1);
// }



// void	Server::parsingRequest(int _epoll_fd, int client_fd)
// {
// 	HttpRequest httprequest;
// 	client.readRequest(_client_buffers[client_fd], client_fd);
// 	clRequest cl_request = client.getClStructRequest(client_fd);

// 	std::cout << "client request host: \n" << cl_request.host << std::endl;
// 	std::cout << "client request port: \n" << cl_request.port << std::endl;

// 	ConfigBlock serverBlock = _config.getServerBlock(cl_request.host, cl_request.port);

// 	std::cout << "server block that's responsible for this request: \n" << std::endl;
// 	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values) 
// 	{
// 		std::cout << value.first <<": ";
// 		for (const std::string& str : value.second)
// 			std::cout << str << ", ";
// 		std::cout << std::endl;
// 	}
// 	for (const std::pair<const std::string, ConfigBlock> &nested : serverBlock.nested) {
// 		std::cout << nested.first << " {" << std::endl;
// 		for (const std::pair<const std::string, std::vector<std::string>> &value : nested.second.values) {
// 			std::cout << value.first <<": ";
// 			for (const std::string& str : value.second)
// 				std::cout << str << ", ";
// 			std::cout << std::endl;
// 		}
// 		std::cout << "}" << std::endl;
// 	}
	
// 	if (cl_request.invalidRequest == true)
// 	{
// 		std::cout << "parse request failed" << std::endl;
// 		_responses[client_fd] = serveError("400", serverBlock);
// 	}
// 	else
// 		_responses[client_fd] = generateHttpResponse(cl_request, serverBlock);


// 	struct epoll_event event;
// 	event.events = EPOLLIN | EPOLLOUT;
// 	event.data.fd = client_fd;
// 	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
// 	_client_buffers.erase(client_fd);
// }

void	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();
	if (response.empty())
		return;

	int	socket_fd = client.getClientFds(0);
	unsigned long bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	if (bytes_sent <= 0) 
	{
		std::cout << "Error writing to client: " << socket_fd << std::endl;
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
		close(socket_fd);
		return;
	}
	if (bytes_sent != response.size())
		client.setResponseData(response.substr(bytes_sent));

	if (bytes_sent == response.size()) 
	{
		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.fd = socket_fd;
		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket_fd, &event);
		client.setClientState(reading_request);
	}
}

// void	Server::handleWrite(int _epoll_fd, int client_fd)
// {
// 	if (_responses[client_fd].empty())
// 		return;

// 	int bytes_sent = send(client_fd, _responses[client_fd].c_str(), _responses[client_fd].size(), 0);
// 	if (bytes_sent <= 0) 
// 	{
// 		std::cout << "Error writing to client: " << client_fd << std::endl;
// 		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
// 		close(client_fd);
// 		_client_buffers.erase(client_fd);
// 		_responses.erase(client_fd);
// 		return;
// 	}

// 	_responses[client_fd] = _responses[client_fd].substr(bytes_sent);

// 	if (_responses[client_fd].empty())
// 	{
// 		struct epoll_event event;
// 		event.events = EPOLLIN;
// 		event.data.fd = client_fd;
// 		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
// 	}
// }

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

const Configuration&	Server::getConfig() const
{
	return (_config);
}