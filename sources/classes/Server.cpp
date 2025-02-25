#include "../../headers/Server.hpp"

Server::Server() : _server_fd(-1), _addr_len(sizeof(_address))
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
	memset(&_address, 0, sizeof(_address)); // Sets all bytes to 0 in address struct
	_address.sin_family = AF_INET; // Sets Adress Family to IPv4
	_address.sin_addr.s_addr = INADDR_ANY; // Assigns the address to INADDR_ANY (which is 0.0.0.0)
	_address.sin_port = htons(PORT); // Converts port number from host byte order to network byte order.
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
	return (true);
}

void	Server::run(void)
{
	char	buffer[BUFFER_SIZE];
	fd_set	read_fds;
	int		max_fd;

	FD_ZERO(&read_fds);
	FD_SET(_server_fd, &read_fds);
	max_fd = _server_fd; // Keep track of the highest fd number

	while (true)
	{
		fd_set tmp_fds = read_fds; // Copy the fd_set to avoid overwriting

		if (select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) < 0)
		{
			std::cerr << "Select error" << std::endl;
			continue;
		}
		max_fd = connectClients(read_fds, tmp_fds, max_fd);
		handleData(read_fds, tmp_fds, max_fd, buffer);
	}
}

int		Server::connectClients(fd_set &read_fds, fd_set &tmp_fds, int max_fd)
{
	// Check if the server socket is ready to accept a new client
	if (FD_ISSET(_server_fd, &tmp_fds))
	{
		int new_client = accept(_server_fd, (struct sockaddr*)&_address, &_addr_len);
		if (new_client < 0)
		{
			std::cerr << "Failed to accept connection" << std::endl;
			return (max_fd);
		}
		setNonBlocking(new_client);
		// Add new client to the set
		FD_SET(new_client, &read_fds);
		if (new_client > max_fd) 
			max_fd = new_client; // Update max fd
		std::cout << "Client connected: " << new_client << std::endl;
	}
	return (max_fd);
}

void	Server::handleData(fd_set &read_fds, fd_set &tmp_fds, int max_fd, char *buffer)
{
	for (int i = 0; i <= max_fd; i++)
	{
		if (FD_ISSET(i, &tmp_fds) && i != _server_fd) // Ignore server socket
		{
			int bytes_read = recv(i, buffer, BUFFER_SIZE - 1, 0);
			if (bytes_read <= 0)
			{
				std::cout << "Client disconnected: " << i << std::endl;
				close(i);
				FD_CLR(i, &read_fds);
			}
			else
			{
				buffer[bytes_read] = '\0';
				std::cout << "\n\n\n\nReceived in buffer:\n\n" << buffer;
				std::cout << "\n" << "Received from: [" << i << "] in buffer end." << std::endl;
				HttpRequest httprequest;
				if (!parseRequest(buffer, httprequest))
					sendBadRequest(i);
				else
				{
					printRequest(httprequest);
					sendHtmlResponse(i, buffer, httprequest);
				}
			}
		}
	}
}

void	Server::sendHtmlResponse(int client_fd, char* buffer, HttpRequest& parsedRequest)
{
	std::string response;

	response = routeRequest(parsedRequest);
	send(client_fd, response.c_str(), response.length(), 0);
	(void)buffer;
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
