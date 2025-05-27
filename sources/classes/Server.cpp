#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"
#include "Configuration.hpp"

Server::Server(const Configuration& config) : _server_fds_amount(0), _addr_len(sizeof(_address)), _client_count(0), _config(config), _name("localhost"), _port("8080"), _root("/www")
{
	std::cout	<< "Default constructor"
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
	if (pipe(signal_pipe) == -1)
		return (false);
	if (setNonBlocking(signal_pipe[0]) == -1)
	{
		close_signal_pipe(1);
		return (false);
	}
	if (initialize_signals() == -1)
	{
		close_signal_pipe(2);
		return (false);
	}

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = signal_pipe[0];

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll instance" << std::endl;
        return (false);
    }
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, signal_pipe[0], &event) == -1)
	{
		close_signal_pipe(3);
		return (false);
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
                continue ;
            }
            bound_servers.insert(host_port_pair);

            int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1)
            {
                std::cerr << "Socket creation failed for " << host << ":" << ports[j] << std::endl;
                continue ;
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
                    continue ;
                }
                struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
                address.sin_addr = addr->sin_addr;
                freeaddrinfo(res);
            }

            if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            {
                std::cerr << "Binding failed on " << host << ":" << ports[j] << std::endl;
                close(socket_fd);
                continue ;
            }

            if (listen(socket_fd, 100) < 0)
            {
                std::cerr << "Listening failed on " << host << ":" << ports[j] << std::endl;
                close(socket_fd);
                continue ;
            }

            // std::cout << "Listening on " << host << ":" << ports[j] << std::endl;

            event.data.fd = socket_fd;
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1)
            {
                std::cerr << "Failed to add server socket to epoll" << std::endl;
                close(socket_fd);
                continue ;
            }
			_server_fds.push_back(socket_fd);
			_server_fds_amount++;
        }
    }

	// std::cout << "END SERVER INITIALIZATION ---------------------" << std::endl;
    return (!_server_fds.empty());
}


void Server::run(void)
{
	struct epoll_event ready_events[MAX_EVENTS];

	while (true)
	{
		std::cout << "\nEpoll_Wait() ----------------------------------------" << std::endl;
		got_signal = 0;
		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, -1);
		if (event_count == -1)
		{
			std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
			continue;
		}
		for (int i = 0; i < event_count; i++) // Checking if signal interrupted
		{
			int fd = ready_events[i].data.fd;
			if (fd == signal_pipe[0])
			{
				if (check_if_signal() == SIGINT)
				{
					std::cout << "RECEIVED CTRL + C" << std::endl;
					close_webserv();
				}
			}
		}

		for (int i = 0; i < event_count; i++) // Going through events returned by epoll_wait()
		{
			int fd = ready_events[i].data.fd;
			for (int i = 0; i < _server_fds_amount; ++i)
				if (fd == _server_fds[i])
					connectClient(_epoll_fd, fd);
			for (int i = 0; i < _client_count; ++i)
			{
				int client_fd_amount = _clients[i]->getClientFds(-1);
				for (int o = 0; o < client_fd_amount; ++o)
				{
					if (_clients[i]->getClientFds(o) == fd)
						_clients[i]->handleEvent(*this);
						if (_clients[i]->getCloseClientState() == true && _clients[i]->getClientState() != sending_response)
							removeClient(_clients[i], i);
					errno = 0;
				}
			}
			if (got_signal != 0)
				handleReceivedSignal();
			setCurrentClient(-1);
		}
	}
	close(_epoll_fd);
}

void	Server::handleReceivedSignal()
{
	if (got_signal == SIGINT)
	{
		std::cout << "SIGNAL: Received SIGINT, closing webserv.." << std::endl;
		close_webserv();
	}
	if (got_signal == SIGPIPE)
	{
		std::cout << "SIGNAL: Received SIGPIPE, disconnecting client now.." << std::endl;
		for (int i = 0; i < _client_count; ++i)
			if (_server_fds[i] == _current_client)
				removeClient(_clients[i], i);
	}
}


void Server::connectClient(int _epoll_fd, int server_fd)
{
	try
	{
		int new_client_fd = accept(server_fd, (struct sockaddr *)&_address, &_addr_len);

		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		getsockname(server_fd, (sockaddr*)&addr, &len);
		// std::cout << "Server listening on port: " << ntohs(addr.sin_port) << std::endl;
		// std::cout << "Server IP: " << inet_ntoa(addr.sin_addr) << std::endl;
		// std::cout << "iptostring: " << ip_to_string(addr.sin_addr) << std::endl;

		ConfigBlock serverBlock = _config.getServerBlock(ip_to_string(addr.sin_addr), std::to_string(ntohs(addr.sin_port)));
		
		if (new_client_fd < 0)
		{
			std::cerr << "ERROR: Failed to accept connection" << std::endl;
			return ;
		}
		if (setNonBlocking(new_client_fd) == -1)
		{
			if (close(new_client_fd) == -1)
				std::cerr << "ERROR: Closing new_client_fd failed" << std::endl;
			return ;
		}

		struct epoll_event event;
		event.events = EPOLLIN | EPOLLOUT | EPOLLET;
		event.data.fd = new_client_fd;

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event) == -1)
		{
			std::cerr << "CLIENT ERROR: Failed to add client to epoll" << std::endl;
			close(new_client_fd);
			return;
		}
		std::cout << "Client connected: " << new_client_fd << std::endl;

		Client* new_client = new Client(new_client_fd, serverBlock);

		this->_clients.push_back(new_client);
		this->_client_count++;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n' << "CLIENT ERROR: failed connecting new client" << std::endl;
	}

}

void Server::removeClient(Client* client, int index)
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

	_clients.erase(_clients.begin() + index);
	delete client;
	_client_count--;

	std::cout << "Client disconnected: " << client_fd << std::endl;

	return ;
}

void Server::close_webserv()
{
	// remove clients
	while (_client_count > 0)
		removeClient(_clients[_client_count - 1], _client_count - 1);


	// close signal pipe
	close_signal_pipe(0);

	// close epoll fd
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _epoll_fd, NULL);

	// remove configuration stuff ??



	std::exit(0);
	std::exit(errno); // IF we want to quit and return errno set to last error
}


// int Server::recvFromSocket(Client& client, std::string& receivedData)
// {
// 	char			buffer[SOCKET_BUFFER];
// 	// std::string&	receivedData = client.getClientReceived();
// 	long			bytes_received;
// 	int				client_fd = client.getClientFds(0);


// 	std::cout << "\nErrno before receiving: " << errno << ", str: " << strerror(errno) << std::endl;


// 	bytes_received = recv(client_fd, buffer, SOCKET_BUFFER - 1, 0);
// 	buffer[bytes_received - 1] = '\0';
// 	std::cout << "\nbytes_received: " << bytes_received << std::endl;

// 	if (bytes_received == -1)
// 	{
// 		client.setCloseClientState(true);
// 		client.setResponseData(ER500);
// 		client.setClientState(sending_response);
// 		return (1);
// 	}
// 	else if (bytes_received == 0)
// 		return (2);
// 	else if (bytes_received == SOCKET_BUFFER - 1)
// 	{
// 		receivedData += buffer;
// 		return (1);
// 	}

// 	std::cout << "\nErrno after receiving: " << errno << ", str: " << strerror(errno) << std::endl;
	
// 	size_t headerEnd = receivedData.find("\r\n\r\n");
// 	if (headerEnd != std::string::npos)
// 	{
// 		size_t contentLength = 0;
// 		size_t contentLengthPos = receivedData.find("Content-Length:");
// 		if (contentLengthPos != std::string::npos)
// 		{
// 			size_t start = contentLengthPos + 15;
// 			size_t end = receivedData.find("\r\n", start);
// 			contentLength = std::stoul(receivedData.substr(start, end - start));
// 		}
// 		size_t totalLength = headerEnd + 4 + contentLength;
// 		if (receivedData.size() >= totalLength)
// 		{
// 			client.setClientState(parsing_request);
// 			return (0);
// 		}
// 	}
// 	return (1);
// }



int Server::recvFromSocket(Client& client)
{
	char			buffer[SOCKET_BUFFER];
	// std::string&	receivedData = client.getClientReceived();
	long			bytes_received;
	int				client_fd = client.getClientFds(0);

	std::cout << "\nErrno before receiving: " << errno << ", str: " << strerror(errno) << std::endl;


	// Read data from the socket level-triggerd without EPOLLET:
	// bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
	// std::cout << "bytes_received: " << bytes_received << std::endl;

	// if (bytes_received < 0)
	// {
	// 	std::cout << "No more data to read, waiting for the next EPOLLIN event" << std::endl;
	// 	return (1); // Wait for the next EPOLLIN event
	// }
	// else if (bytes_received == 0)
	// {
	// 	std::cout << "Client disconnected: " << client_fd << std::endl;
	// 	return (2); // Indicates the client has disconnected
	// }
	// receivedData.append(buffer, bytes_received);


	// Read data from the socket edge-triggered with EPOLLET:
	do
	{
		bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
		std::cout << "bytes_received: " << bytes_received << std::endl;

		if (bytes_received < 0)
			break ;
		else if (bytes_received == 0)
		{
			// std::cout << "Client disconnected: " << client_fd << std::endl;
			return (2);
		}
		client.setReceivedData(buffer);
		// receivedData.append(buffer, bytes_received);
	} while (bytes_received > 0);

	// std::cout << "receivedData: \n" << receivedData << std::endl;

	std::string tempReceivedData = client.getClientReceived();

	std::cout << "\nErrno after receiving: " << errno << ", str: " << strerror(errno) << std::endl;
	
	// Check if the headers are fully received
	size_t headerEnd = tempReceivedData.find("\r\n\r\n");
	// size_t headerEnd = receivedData.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
	{
		// Headers are complete, check if the body is also complete
		size_t contentLength = 0;
		size_t contentLengthPos = tempReceivedData.find("Content-Length:");
		if (contentLengthPos != std::string::npos)
		{
			size_t start = contentLengthPos + 15; // Skip "Content-Length: "
			size_t end = tempReceivedData.find("\r\n", start);
			contentLength = std::stoul(tempReceivedData.substr(start, end - start));
		}

		// Check if the full request (headers + body) has been received
		size_t totalLength = headerEnd + 4 + contentLength; // Headers + "\r\n\r\n" + Body
		if (tempReceivedData.size() >= totalLength)
		{
			std::cout << "Full request received (" << tempReceivedData.size() << " bytes)" << std::endl;
			client.setClientState(parsing_request);
			return (0); // Full request received
		}
	}
	return (1);
}

int	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();

	// std::cout << "\n--- RESPONSE ---\n" << response << "\n--- END OF RESPONSE --- " << std::endl;
	
	int	socket_fd = client.getClientFds(0);
	ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	std::cout << "Response size / bytes sent: " << response.size() << " / " << bytes_sent << std::endl;
	if (bytes_sent <= 0)
	{
		std::cout << "Error writing to client: " << socket_fd << std::endl;
		return (2);
	}
	if (bytes_sent != static_cast<ssize_t>(response.size()))
	{
		client.setResponseData(response.substr(bytes_sent));
		client.updateBytesSent(bytes_sent);
		return (1);
	}
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = socket_fd;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket_fd, &event);
	if (client.getCloseClientState() == true)
		return (2);
	client.setClientState(reading_request);
	client.clearData(1);

	return (0);
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


void Server::setCurrentClient(int client_fd)
{
	_current_client = client_fd;
}
