#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"
#include "Configuration.hpp"

Server::Server(const Configuration& config) : _server_fds_amount(0), _addr_len(sizeof(_address)), _client_count(0), _close_server(false), _config(config)
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
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = signal_pipe[0];

    _epoll_fd = epoll_create(10);
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


// void	Server::run(void)
// {
// 	struct epoll_event ready_events[MAX_EVENTS];
// 	struct epoll_event current_event;

// 	while (_close_server == false)
// 	{
// 		std::cout << "\nEpoll_Wait() --------------- clients: " << _client_count << " ----------------" << std::endl;
// 		got_signal = 0;

// 		errno = 0;
// 		// std::cout << "CLIENT COUNT: " << _client_count << std::endl;
// 		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, 5000);
// 		if (event_count == 0)
// 		{
// 			checkTimedOut();
// 			continue ;
// 		}
// 		if (event_count == -1)
// 		{
// 			if (got_signal != 0)
// 			{
// 				if (check_if_signal() == SIGINT)
// 					break ;
// 			}
// 			else
// 				std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
// 			continue;
// 		}

// 		for (int i = 0; i < event_count; i++) // Going through events returned by epoll_wait()
// 		{
// 			int fd = ready_events[i].data.fd;
// 			current_event = ready_events[i];

// 			std::cout << "Current event: " << current_event.events << std::endl;

// 			std::cout << "---> FD: " << fd <<std::endl;
// 			for (int j = 0; j < _server_fds_amount; ++j)
// 			{
// 				if (fd == _server_fds[j])
// 				{
// 					connectClient(_epoll_fd, fd);
// 					break ;
// 				}
// 			}
			
// 			std::cout << ready_events[i].events << std::endl;
// 			if (ready_events[i].events & EPOLLHUP || ready_events[i].events & EPOLLRDHUP || ready_events[i].events & EPOLLERR)
// 			{
// 				std::cout << "in hup stuff " << ready_events[i].events << std::endl;
// 				removeClient(i);
// 			}
// 			for (int i = _client_count - 1; i >= 0; --i)
// 			{
// 				int client_fd_amount = _clients[i]->getClientFds(-1);
// 				for (int o = 0; o < client_fd_amount; ++o)
// 				{
// 					if (_clients[i]->getClientFds(o) == fd)
// 					{
// 						_clients[i]->handleEvent(*this);
// 						if ((_clients[i]->getCloseClientState() == true && _clients[i]->getClientState() == idle))
// 						{
// 							removeClient(i);
// 							break ;
// 						}
// 					}
// 					errno = 0;
// 				}
// 			}
// 			if (got_signal != 0)
// 				handleReceivedSignal();
// 		}
// 		if (_close_server == false)
// 			checkTimedOut();
// 	}
// 	close_webserv();
// }


int Server::findClient(int fd)
{
	bool found = false;
	int i;

	std::cout << "Find client: " << fd << std::endl;
	for (i = 0; i < this->_client_count; ++i)
	{
		int client_fd_amount = _clients[i]->getClientFds(-1);
		for (int o = 0; o < client_fd_amount; ++o)
		{
			// std::cout << "fd findClient: " << _clients[i]->getClientFds(o) << std::endl;
			if (_clients[i]->getClientFds(o) == -1)
				continue ;
			if (_clients[i]->getClientFds(o) == fd)
			{
				found = true;
				break ;
			}
		}
		if (found == true)
			break ;
	}
	if (found == false)
	{
		std::cout << "Couldn't find client!" << std::endl;
		return (-1);
	}
	return (i);
}

bool Server::checkIfNewConnections(int fd)
{
	for (int i = 0; i < _server_fds_amount; ++i)
	{
		if (fd == _server_fds[i])
		{
			connectClient(_epoll_fd, fd);
			return (true);
		}
	}
	return (false);
}





// ONLY FOR TESTING
void	Server::printClientFds(int client_index)
{
	int fd_amount = _clients[client_index]->getClientFds(-1);
	for (int i = 0; i < fd_amount; ++i)
		std::cout << "Client fd: " << _clients[client_index]->getClientFds(i) << std::endl;
}
// ONLY FOR TESTING






void	Server::run(void)
{
	struct epoll_event ready_events[MAX_EVENTS];

	while (_close_server == false)
	{
		std::cout << "\nEpoll_Wait() --------------- clients: " << _client_count << " ----------------" << std::endl;
		got_signal = 0;
		errno = 0;
		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, 5000);
		if (event_count == 0)
		{
			checkTimedOut();
			continue ;
		}
		if (event_count == -1)
		{
			if (got_signal != 0)
			{
				if (check_if_signal() == SIGINT)
					break ;
			}
			else
				std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
			continue;
		}

		for (int i = 0; i < event_count; ++i)
		{
			int fd = ready_events[i].data.fd;
			if (checkIfNewConnections(fd))
				continue ;
			int event_flags = ready_events[i].events;
			int client_index = findClient(fd);
			if (client_index == -1)
				continue ;
			if (fd == _clients[client_index]->getClientFds(0) && (event_flags & EPOLLHUP || event_flags & EPOLLRDHUP || event_flags & EPOLLERR))
			{
				removeClient(client_index);
				continue ;
			}
			std::cout << "\n\nFD: " << fd << " / event: " << event_flags << std::endl;
			printClientFds(client_index); // <-- ONLY FOR TESTING
			_clients[client_index]->handleEvent(*this);
			if ((_clients[client_index]->getCloseClientState() == true && _clients[client_index]->getClientState() == idle))
			{
				removeClient(client_index);
				break ;
			}
			errno = 0;
			if (got_signal != 0)
				handleReceivedSignal();
		}
		if (_close_server == false)
			checkTimedOut();
	}
	close_webserv();
}


void	Server::handleReceivedSignal()
{
	if (got_signal == SIGINT)
	{
		std::cout << "SIGNAL: Received SIGINT, closing webserv.." << std::endl;
		_close_server = true;
	}
	if (got_signal == SIGPIPE)
	{
		std::cout << "SIGNAL: Received SIGPIPE, disconnecting client now.." << std::endl;
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
		event.events = EPOLLIN;
		event.data.fd = new_client_fd;

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event) == -1)
		{
			std::cerr << "CLIENT ERROR: Failed to add client to epoll" << std::endl;
			close(new_client_fd);
			return;
		}
		std::cout << "Client connected: " << new_client_fd << std::endl;

		std::unique_ptr<Client> new_client = std::make_unique<Client>(new_client_fd, serverBlock);

		this->_clients.push_back(std::move(new_client));
		this->_client_count++;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n' << "CLIENT ERROR: failed connecting new client" << std::endl;
	}
}









void Server::removeClient(int index)
{
	int client_fd = _clients[index]->getClientFds(0);
	std::cout << "REMOVE: CLIENT FD IN REMOVE CLIENT: " << client_fd << std::endl;

	if (_clients[index]->checkCgiPtr())
	{
		if (_clients[index]->getCgiStruct().ets_pipe[0] != -1)
		{
			if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _clients[index]->getCgiStruct().ets_pipe[0], NULL) == -1)
				std::cerr << "Failed deleting " << _clients[index]->getCgiStruct().ets_pipe[0] << " from EPOLL" << std::endl;
			if (close(_clients[index]->getCgiStruct().ets_pipe[0]) == -1)
				std::cerr << "SERVER ERROR: Failed closing cgi_pipe write_end " << std::endl;
			_clients[index]->getCgiStruct().ets_pipe[0] = -1;
		}
		if (_clients[index]->getCgiStruct().ste_pipe[1] != -1)
		{
			if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _clients[index]->getCgiStruct().ste_pipe[1], NULL) == -1)
				std::cerr << "Failed deleting " << _clients[index]->getCgiStruct().ste_pipe[1] << " from EPOLL" << std::endl;
			if (close(_clients[index]->getCgiStruct().ste_pipe[1]) == -1)
				std::cerr << "SERVER ERROR: Failed closing cgi_pipe read_end " << std::endl;
			_clients[index]->getCgiStruct().ste_pipe[1] = -1;
		}
	}

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1)
		std::cerr << "SERVER ERROR: Failed deleting client fd " << client_fd << " from epoll!" << std::endl;
	if (close(client_fd) != 0)
		std::cerr << "SERVER ERROR: Failed closing client_fd " << client_fd << std::endl;

	_clients[index] = nullptr;
	_clients.erase(_clients.begin() + index);
	_client_count--;

	std::cout << "Client disconnected: " << client_fd << std::endl;

	return ;
}








void Server::close_webserv()
{
	// remove clients
	std::cerr << "Quiting webserv" << std::endl;
	while (_client_count > 0)
	{
		std::cout << "END OF WEBSERV - REMOVING " << _client_count << std::endl;
		removeClient(_client_count - 1);
	}


	// close signal pipe
	close_signal_pipe(0);

	// close epoll fd
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _epoll_fd, NULL);
	if (close(_epoll_fd) == -1)
		std::cerr << "Failed closing epoll fd on closing webserv" << std::endl;
	
	_close_server = true;
}











int Server::recvFromSocket(Client& client)
{
	char			buffer[SOCKET_BUFFER] = {0};
	long			bytes_received;
	int				client_fd = client.getClientFds(0);

	if (client.getClientState() == idle)
		client.setClientState(reading_request);
	if (client.getClientReceived().empty())
		client.setLastRequest();


	std::cout << "\nErrno before receiving: " << errno << ", str: " << strerror(errno) << std::endl;

	bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
	std::cout << "bytes_received: " << bytes_received << std::endl;

	if (bytes_received == -1)
	{
		serveError(client, "500", client.getServerBlock());
		return (1);
	}
	else if (bytes_received == 0 && client.getClientReceived().empty())
	{
		client.setClientState(idle);
		client.setCloseClientState(true);
		return (2);
	}
	client.setReceivedData(buffer, bytes_received);

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
			// std::cout << "Full request received (" << tempReceivedData.size() << " bytes)" << std::endl;
			client.setClientState(parsing_request);
			return (0); // Full request received
		}
		struct epoll_event event;
		event.events = EPOLLOUT;
		event.data.fd = client.getClientFds(0);
		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getClientFds(0), &event);
	}
	else
		std::cout << "Client: " << client_fd << ": Didn't find the received end thing \\r\\n\\r\\n" << std::endl;
	return (1);
}

void	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();

	// std::cout << "\n\n\n-----------FULL RESPONSE BEFORE SENDING-------------\n\n\n" << response << "\n\n\n-------------- END OF RESPONSE BEFORE SENDING--------------\n\n\n" << std::endl;


	int	socket_fd = client.getClientFds(0);
	ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	std::cout << "Response size / bytes sent: " << response.size() << " / " << bytes_sent << std::endl;
	if (bytes_sent < 0)
	{
		std::cout << "Error writing to client: " << socket_fd << std::endl;
		client.setCloseClientState(true);
		client.setClientState(idle);
		return ;
	}
	if (bytes_sent == 0)
	{
		client.setCloseClientState(true);
		client.setClientState(idle);
		return ;
	}
	if (bytes_sent != static_cast<ssize_t>(response.size()))
	{
		client.setResponseData(response.substr(bytes_sent));
		client.updateBytesSent(bytes_sent);
		return ;
	}
	if (client.getCloseClientState() == false)
		client.resetClient(_epoll_fd);
	client.setClientState(idle);
}



int	Server::getEpollFd() const
{
	return (_epoll_fd);
}

const Configuration&	Server::getConfig() const
{
	return (_config);
}

bool	Server::getCloseServer() const
{
	return (_close_server);
}


void Server::checkTimedOut()
{
	std::cout << "\nChecking time outs\n" << std::endl;
	int time_since_request;
	int state;
	int client_fd;
	for (int i = _client_count - 1; i >= 0; --i)
	{
		time_since_request = std::time(nullptr) - _clients[i]->getLastRequest();
		client_fd = _clients[i]->getClientFds(0);
		state = _clients[i]->getClientState();
		if (state == reading_request && time_since_request >= TIMEOUT)
		{
			struct epoll_event event;
			event.events = EPOLLOUT;
			event.data.fd = client_fd;
			epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
			serveError(*_clients[i], "408", _clients[i]->getServerBlock());
		}
		else if ((state == cgi_write || state == cgi_read) && time_since_request >= TIMEOUT)
		{
			if (_clients[i]->checkCgiPtr() && _clients[i]->getCgiStruct().child_pid != -1)
				kill(_clients[i]->getCgiStruct().child_pid, SIGTERM);
			serveError(*_clients[i], "500", _clients[i]->getServerBlock());
		}
		else if (state == idle && time_since_request >= KEEPALIVETIME)
			removeClient(i);
	}
}
