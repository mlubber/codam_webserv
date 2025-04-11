#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"

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
					int client_amount = _clients[i]->getClientFds(-1);
					for (int o = 0; o < client_amount; ++o)
						if (_clients[i]->getClientFds(o) == fd)
							_clients[i]->handleEvent(*this);
				}
			}
		}
	}
	close(_epoll_fd);
}

void Server::connectClient(int _epoll_fd)
{
	try
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
	char		buffer[SOCKET_BUFFER];
	std::string	data;
	ssize_t		bytes_received;
	int			client_fd = client.getClientFds(0);

	do
	{
		bytes_received = recv(client_fd, buffer, SOCKET_BUFFER, 0);
		if (bytes_received <= 0)
			break ;
		std::cout << "bytes_received: " << bytes_received << std::endl;
		data += buffer;
	} while (bytes_received > 0);
	client.setReceivedData(data);


	if (bytes_received == -1)
		return (-1);
	else if (bytes_received == 0) // Not sure if correct, because we could just be at the end of what to read, without needing to close the connection to the client
	{
		removeClient(&client);
		return (0);
	}
	client.setClientState(parsing_request);
	return (1);
}

int	Server::sendToSocket(Client& client)
{
	std::string response = client.getClientResponse();
	if (response.empty())
		return;

	int	socket_fd = client.getClientFds(0);
	int bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	if (bytes_sent <= 0) 
	{
		std::cout << "Error writing to client: " << socket_fd << std::endl;
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
		close(socket_fd);
		_client_buffers.erase(socket_fd);
		_responses.erase(socket_fd);
		return;
	}

	_responses[socket_fd] = _responses[socket_fd].substr(bytes_sent); 

	if (_responses[socket_fd].empty()) 
	{
		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.fd = socket_fd;
		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket_fd, &event);
	}
}





/* GETTERS & SETTERS */

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