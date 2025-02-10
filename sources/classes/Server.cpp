#include "../../headers/Server.hpp"

Server::Server() : _server_fd(-1), _client_fd(-1), _addr_len(sizeof(_address))
{
	std::cout	<< "Default constructor"
				<< "\nServer fd: " << _server_fd
				<< "\nClient fd: " << _client_fd
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
		// _server_fd = -1;
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
	setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

	if (listen(_server_fd, 3) < 0) 
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

		FD_ZERO(&read_fds); // Initializes the read_fds set to be empty.
		FD_SET(_server_fd, &read_fds); // Set server's fd to the fd_set, monitoring the server socket for incoming client connections.

		while (true)
		{
			fd_set tmp_fds = read_fds;

			// select() monitors the file descriptors in tmp_fds for activity.
			if (select(_server_fd + 1, &tmp_fds, NULL, NULL, NULL) < 0)
			{
				std::cerr << "Select error" << std::endl;
				continue;
			}
			// FD_ISSET checks if the server socket is ready to accept a new client connection.
			if (FD_ISSET(_server_fd, &tmp_fds))
			{
				// accept() is called to accept the incoming connection. 
				// It returns a new socket file descriptor (_client_fd) for communication with the client.
				_client_fd = accept(_server_fd, (struct sockaddr*)&_address, &_addr_len);
				// std::cout << "client fd: " << _client_fd << std::endl;
				if (_client_fd < 0)
				{
					std::cerr << "Failed to accept connection" << std::endl;
					continue;
				}
				std::cout << "Client connected" << std::endl;

				// recv() is used to read data from the client.
				// It returns the number of bytes actually read.
				int bytes_read = recv(_client_fd, buffer, BUFFER_SIZE - 1, 0);
				if (bytes_read > 0)
				{
					buffer[bytes_read] = '\0'; // creates proper null-terminated string;
					std::cout << _client_fd << " Received: " << buffer << std::endl;

					if (strncmp(buffer, "GET", 3) == 0)
					{
						// Define an HTTP response with a basic HTML page
						const char *http_response =
							"HTTP/1.1 200 OK\r\n"
							"Content-Type: text/html\r\n"
							"Content-Length: 76\r\n"
							"\r\n"
							"<html><body><h1>WEBSERV!</h1><p>Wouter heeft een pet af</p></body></html>";

						// Send response
						send(_client_fd, http_response, strlen(http_response), 0);
					}
				close(_client_fd);
			}
		}
	}
}