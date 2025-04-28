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
	std::cout << "START SERVER INITIALIZATION ---------------------" << std::endl;
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
	event.events = EPOLLIN;

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

            if (listen(socket_fd, 10) < 0)
            {
                std::cerr << "Listening failed on " << host << ":" << ports[j] << std::endl;
                close(socket_fd);
                continue ;
            }

            std::cout << "Listening on " << host << ":" << ports[j] << std::endl;

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
	std::cout << "END SERVER INITIALIZATION ---------------------" << std::endl;
}


void Server::run(void)
{
	struct epoll_event ready_events[MAX_EVENTS];

	while (true)
	{
		std::cout << "Yep ----------------------------------------" << std::endl;
		got_signal = 0;
		int event_count = epoll_wait(_epoll_fd, ready_events, MAX_EVENTS, -1);
		if (event_count == -1)
		{
			std::cerr << "Epoll wait error" << std::endl;
			continue;
		}
		else if (check_if_signal() == SIGINT)
			close_webserv();

		for (int i = 0; i < event_count; i++)
		{
			int fd = ready_events[i].data.fd;

			std::cout << "Event count: " << event_count << " , current event fd: " << fd << std::endl;
			for (int i = 0; i < _server_fds_amount; ++i)
				if (fd == _server_fds[i])
					connectClient(_epoll_fd, fd);
			for (int i = 0; i < _client_count; ++i)
			{
				int client_amount = _clients[i]->getClientFds(-1);
				for (int o = 0; o < client_amount; ++o)
				{
					std::cout << "current event fd: " << fd << " , client fd: " << _clients[i]->getClientFds(o) << std::endl;
					if (_clients[i]->getClientFds(o) == fd)
						if (_clients[i]->handleEvent(*this) == 2)
							removeClient(_clients[i], i);
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



	exit(0);
	exit(errno); // IF we want to quit and return errno set to last error
}

int	Server::recvFromSocket(Client& client)
{
	char		buffer[SOCKET_BUFFER];
	std::string	data;
	size_t		bytes_received;
	int			client_fd = client.getClientFds(0);

	do
	{
		bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
		if (bytes_received <= 0)
			break ;
		std::cout << "bytes_received: " << bytes_received << std::endl;
		data += buffer;
		if (bytes_received == 0) // Not sure if correct, because we could just be at the end of what to read, without needing to close the connection to the client
		{
			return (2);
		}
	} while (bytes_received > 0);
	client.setReceivedData(data);
	client.setClientState(parsing_request);
	return (0);
}

int	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();

	int	socket_fd = client.getClientFds(0);
	size_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	if (bytes_sent <= 0) 
	{
		std::cout << "Error writing to client: " << socket_fd << std::endl;
		return (2);
	}
	if (bytes_sent != response.size())
	{
		client.setResponseData(response.substr(bytes_sent));
		client.updateBytesSent(bytes_sent);
		return (1);
	}
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = socket_fd;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket_fd, &event);
	client.setClientState(reading_request);

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
