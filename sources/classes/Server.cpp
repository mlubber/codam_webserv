#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"
#include "Configuration.hpp"

Server::Server(const Configuration& config) : _server_fds_amount(0), _addr_len(sizeof(_address)), _client_count(0), _close_server(false), _config(config)
{

}

Server::~Server()
{
	for (int i = 0; i < _server_fds_amount; ++i)
		if (close(_server_fds[i]) == -1)
			std::cerr << "CLOSING ERROR: Failed closing server_fd: " << _server_fds[i] << std::endl;
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

    return (!_server_fds.empty());
}

int Server::findClient(int fd)
{
	bool found = false;
	int i;

	for (i = 0; i < this->_client_count; ++i)
	{
		int client_fd_amount = _clients[i]->getClientFds(-1);
		for (int o = 0; o < client_fd_amount; ++o)
		{
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
		return (-1);
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




void	Server::run(void)
{
	struct epoll_event ready_events[MAX_EVENTS];

	while (_close_server == false)
	{
		got_signal = 0;
		errno = 0;
		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, 5000);
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

		checkTimedOut();
		cleanUpChildPids();	
	}
	close_webserv();
}


void	Server::handleReceivedSignal()
{
	if (got_signal == SIGINT)
	{
		_close_server = true;
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

		std::unique_ptr<Client> new_client = std::make_unique<Client>(new_client_fd);

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

	return ;
}


void Server::close_webserv()
{
	while (_client_count > 0)
		removeClient(_client_count - 1);

	close_signal_pipe(0);

	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _epoll_fd, NULL);
	if (close(_epoll_fd) == -1)
		std::cerr << "Failed closing epoll fd on closing webserv" << std::endl;
	cleanUpChildPids();	
	
	_close_server = true;
	std::cout << "\nClosing server" << std::endl;
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

	bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
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
	
	size_t headerEnd = tempReceivedData.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
	{
		size_t contentLength = 0;
		size_t contentLengthPos = tempReceivedData.find("Content-Length:");
		if (contentLengthPos != std::string::npos)
		{
			size_t start = contentLengthPos + 15;
			size_t end = tempReceivedData.find("\r\n", start);
			contentLength = std::stoul(tempReceivedData.substr(start, end - start));
		}

		size_t totalLength = headerEnd + 4 + contentLength;
		if (tempReceivedData.size() >= totalLength)
		{
			client.setClientState(parsing_request);
			return (0);
		}
		struct epoll_event event;
		event.events = EPOLLOUT;
		event.data.fd = client.getClientFds(0);
		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getClientFds(0), &event);
	}
	return (1);
}

void	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();

	int	socket_fd = client.getClientFds(0);
	ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	if (bytes_sent <= 0)
	{
		std::cerr << "Error writing to client: " << socket_fd << std::endl;
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
	int time_since_request;
	int state;
	int client_fd;
	long timestamp = std::time(nullptr);
	for (int i = _client_count - 1; i >= 0; --i)
	{
		time_since_request = timestamp - _clients[i]->getLastRequest();
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
			{
				int	child_pid = _clients[i]->getCgiStruct().child_pid;
				if (_clients[i]->getCgiStruct().child_pid != -1)
				{
					pid_t	wpid;
					int		status;

					kill(_clients[i]->getCgiStruct().child_pid, SIGTERM);
					wpid = waitpid(_clients[i]->getCgiStruct().child_pid, &status, 0);
					std::cerr << "Child process hanging, killing child process (" << child_pid << ")\n" << std::endl;
				}
			}
			serveError(*_clients[i], "500", _clients[i]->getServerBlock());
		}
		else if (state == idle && time_since_request >= KEEPALIVETIME)
			removeClient(i);
	}
}

void	Server::addChildPidToMap(int child_pid)
{
	_child_pids.insert({child_pid, std::time(nullptr)});
}

void	Server::cleanUpChildPids()
{
	pid_t	wpid;
	long	timestamp = std::time(nullptr);
	int		status;
	int		exit_code;

	for (auto it = _child_pids.begin(); it != _child_pids.end();)
	{
		try
		{
			exit_code = 0;
			wpid = waitpid(it->first, &status, WNOHANG);
			if (wpid == 0)
			{
				if (timestamp - it->second > TIMEOUT)
				{
					kill(it->first, SIGTERM);
					wpid = waitpid(it->first, &status, 0);
					it = _child_pids.erase(it);
				}
				else
					++it;
			}
			else if (wpid == -1)
				exit_code = errno;
			else
			{
				if (WIFSIGNALED(status))
				{
					int signal_number = WTERMSIG(status);
					std::cerr << "CGI ERROR: Child terminated by signal: " << signal_number << std::endl;
					if (signal_number == SIGINT)
						exit_code = 130;
					else
						exit_code = 128 + signal_number;
				}
				else if (exit_code == 0 && WIFEXITED(status))
					exit_code = WEXITSTATUS(status);
				if (exit_code != 0)
					std::cerr << "NOTE: Child didn't end properly, code: " << exit_code << std::endl;
				it = _child_pids.erase(it);
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << "CGI ERROR: Couldn't find child process to handle termination" << std::endl;
			++it;
		}
	}
}
