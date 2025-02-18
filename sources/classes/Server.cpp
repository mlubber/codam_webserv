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

		// Check if the server socket is ready to accept a new client
		if (FD_ISSET(_server_fd, &tmp_fds))
		{
			int new_client = accept(_server_fd, (struct sockaddr*)&_address, &_addr_len);
			if (new_client < 0)
			{
				std::cerr << "Failed to accept connection" << std::endl;
				continue;
			}
			setNonBlocking(new_client);
			// Add new client to the set
			FD_SET(new_client, &read_fds);
			if (new_client > max_fd) 
				max_fd = new_client; // Update max fd
			std::cout << "Client connected: " << new_client << std::endl;
		}

		// Check all connected clients for incoming data
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
					std::cout << "\n" << i << " Received:" << std::endl;
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
}

bool	Server::parseRequest(const char* request, HttpRequest& httprequest)
{
	std::cout << "\n\nPARSE REQUEST:\n" << std::endl;
	std::istringstream	request_stream(request);
	std::string			line;

	// Read the first line (Request Line)
	if (!getline(request_stream, line) || line.empty())
		return (false);

	std::istringstream first_line(line);
	if (!(first_line >> httprequest.method >> httprequest.path >> httprequest.version))
		return (false); // Malformed request line

	std::cout << "Method: " << httprequest.method << std::endl;
	std::cout << "Path: " << httprequest.path << std::endl;
	std::cout << "Version: " << httprequest.version << std::endl;

	// std::cout << line << std::endl;
	while (getline(request_stream, line) && line != "\r")
	{
		// std::cout << line << std::endl;
		std::size_t	colonPos = line.find(": ");
		if (colonPos != std::string::npos)
		{
			// std::cout << colonPos << std::endl;
			std::string	key = line.substr(0, colonPos);
			std::string	value = line.substr(colonPos + 2);
			// std::cout	<< key << ": "
			// 			<< value << std::endl;
			httprequest.headers[key] = value;
		}
	}

	// Read body if Content-Length is provided
    if (httprequest.headers.find("Content-Length") != httprequest.headers.end()) 
	{
        int contentLength = std::atoi(httprequest.headers["Content-Length"].c_str());
        char bodyBuffer[contentLength + 1];
        request_stream.read(bodyBuffer, contentLength);
        bodyBuffer[contentLength] = '\0';
        httprequest.body = std::string(bodyBuffer);
		std::cout << "Body: " << httprequest.body << std::endl;
    }

	// (void)httprequest;
	return (true);
}

void	Server::sendBadRequest(int client_fd)
{
	std::cout << "Bad request!" << std::endl;
    const char *bad_response =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Bad Request";
    send(client_fd, bad_response, strlen(bad_response), 0);
}

void	Server::printRequest(HttpRequest& httprequest)
{
	std::cout << "\n\nPRINT REQUEST:\n" << std::endl;
	std::cout << "Method: " << httprequest.method << std::endl;
	std::cout << "Path: " << httprequest.path << std::endl;
	std::cout << "Version: " << httprequest.version << std::endl;
	for (std::map<std::string, std::string>::iterator it = httprequest.headers.begin(); it != httprequest.headers.end(); it++)
		std::cout << it->first << ": " << it->second << std::endl;
}

std::string	Server::getExtType(const std::string& filename)
{
	std::map<std::string, std::string> types = 
	{
		{".html", "text/html"},
		{".css", "text/css"},
		{".js", "application/javascript"},
		{".png", "image/png"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".gif", "image/gif"},
		{".txt", "text/plain"}
	};

	size_t dotPos = filename.find_last_of('.');

	if (dotPos != std::string::npos)
	{
		std::string extention = filename.substr(dotPos);
		if (types.find(extention) != types.end())
			return (types[extention]);
	}
	return ("application/octet-stream");
}

std::string Server::serveStaticFile(const std::string& filePath)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file)
		return ("\r\n\r\nHTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found");
	
	// Get file content
	std::ostringstream fileStream;
	fileStream << file.rdbuf();
	std::string fileContent = fileStream.str();
	// std::cout << fileContent << std::endl;

	// Get content type
	std::string	contentType = getExtType(filePath);
	// std::cout << contentType << std::endl;

	// Create HTTP response
	std::ostringstream response;
	response << "HTTP/1.1 200 OK\r\n";
	response << "Content-Type: " << contentType << "\r\n";
	response << "Content-Length: " << fileContent.size() << "\r\n";
	response << "\r\n";
	response << fileContent;

	return (response.str());
}

std::string	Server::handlePostRequest(const HttpRequest &request)
{
	std::string filePath = STATIC_DIR + request.path + std::string("/index.html");
	std::ostringstream response;


	if (request.headers.find("Content-Type") != request.headers.end())
	{
		std::string contentType = request.headers.at("Content-Type");

		if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
		{
			return (serveStaticFile(filePath));
			// response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
			// 			<< (20 + request.body.size()) << "\r\n\r\n"
			// 			<< "Received Form Data: " << request.body;
		} 
		else if (contentType.find("application/json") != std::string::npos)
		{
			response	<< "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
						<< request.body.size() << "\r\n\r\n"
						<< "{ \"message\": \"Received JSON\", \"data\": " << request.body << " }";
		} 
		else
			response 	<< "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nInvalid Format";
	} 
	else 
		response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nNo Content-Type";

	return (response.str());
}

std::string Server::routeRequest(const HttpRequest &request)
{
	std::string filePath = STATIC_DIR + request.path + std::string("/index.html");

	if (request.method == "GET")
	{
		if (request.path == "/" || request.path == "/favicon.ico")
			filePath = STATIC_DIR + std::string("/index.html");
		return (serveStaticFile(filePath));
	}
	else if (request.method == "POST")
		return (handlePostRequest(request));
	return ("HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 18\r\n\r\nMethod Not Allowed");
}

void	Server::sendHtmlResponse(int client_fd, char* buffer, HttpRequest& parsedRequest)
{
	std::string response;

	response = routeRequest(parsedRequest);
	// if (strncmp(buffer, "GET", 3) == 0)
	// {
	// 	response = routeRequest(parsedRequest);
	// 	// http_response =
	// 	// 	"HTTP/1.1 200 OK\r\n"
	// 	// 	"Content-Type: text/html\r\n"
	// 	// 	"Content-Length: 410\r\n"
	// 	// 	"\r\n"
	// 	// 	"<html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>webserv</title><style>body{font-family:Arial,sans-serif;text-align:center;margin:50px;}h1{color:#333;}p{color:#666}</style></head><body><h1>Welcome to this website</h1><p>This is a basic webpage created with HTML and CSS</p><button onclick=\"alert('Hello!')\">Click Me</button></body></html>\r\n";
	// }
	// else if (strncmp(buffer, "\r\n", 2) == 0)
	// 	response = "GET / HTTP/1.1\r\nHost: localhost\r\n";
	// else
	// 	response = "\r";
	send(client_fd, response.c_str(), response.length(), 0);
	// std::cout << "html response send" << std::endl;
	// std::string response = routeRequest(parsedRequest);
	// std::cout << response << std::endl;
	(void)buffer;
}


void setNonBlocking(int socket)
{
	std::cout << "Setting non-blocking mode for socket: " << socket << std::endl;
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1)
	{
		std::cerr << flags << " fcntl get failed\n";
		return ;
	}
	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl set failed\n";
		return ;
	}
}

// void	Server::run(void)
// {
// 	char	buffer[BUFFER_SIZE];
// 	fd_set	read_fds;

// 	FD_ZERO(&read_fds); // Initializes the read_fds set to be empty.
// 	FD_SET(_server_fd, &read_fds); // Set server's fd to the fd_set, monitoring the server socket for incoming client connections.
// 	int max_fd = _server_fd;

// 	while (true)
// 	{
// 		fd_set tmp_fds = read_fds;

// 		// select() monitors the file descriptors in tmp_fds for activity.
// 		if (select(_server_fd + 1, &tmp_fds, NULL, NULL, NULL) < 0)
// 		{
// 			std::cerr << "Select error" << std::endl;
// 			continue;
// 		}
// 		// FD_ISSET checks if the server socket is ready to accept a new client connection.
// 		if (FD_ISSET(_server_fd, &tmp_fds))
// 		{
// 			// accept() is called to accept the incoming connection. 
// 			// It returns a new socket file descriptor (_client_fd) for communication with the client.
// 			int new_client_fd = accept(_server_fd, (struct sockaddr*)&_address, &_addr_len);
// 			// std::cout << "client fd: " << _client_fd << std::endl;
// 			if (new_client_fd < 0)
// 			{
// 				std::cerr << "Failed to accept connection" << std::endl;
// 				continue;
// 			}
// 			std::cout << "Client connected: " << new_client_fd << std::endl;

// 			// Add new client to the set
// 			FD_SET(new_client_fd, &read_fds);
// 			if (new_client_fd > max_fd) 
// 				max_fd = new_client_fd; // Update max fd


// 			for (int i = 0; i <= max_fd; i++)
// 			{
// 				if (FD_ISSET(i, &tmp_fds) && i != _server_fd) // Ignore server socket
// 				{
// 					int bytes_read = recv(i, buffer, BUFFER_SIZE - 1, 0);
// 					if (bytes_read <= 0)
// 					{
// 						std::cout << "Client disconnected: " << i << std::endl;
// 						close(i);
// 						FD_CLR(i, &read_fds);
// 					}
// 					else
// 					{
// 						const char *http_response;
// 						buffer[bytes_read] = '\0';
// 						std::cout << i << " Received: " << buffer << std::endl;
// 						if (strncmp(buffer, "GET", 3) == 0)
// 						{
// 							http_response =
// 								"HTTP/1.1 200 OK\r\n"
// 								"Content-Type: text/html\r\n"
// 								"Content-Length: 410\r\n"
// 								"\r\n"
// 								"<html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>webserv</title><style>body{font-family:Arial,sans-serif;text-align:center;margin:50px;}h1{color:#333;}p{color:#666}</style></head><body><h1>Welcome to this website</h1><p>This is a basic webpage created with HTML and CSS</p><button onclick=\"alert('Hello!')\">Click Me</button></body></html>";
// 						}
// 						else
// 							http_response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
// 						// Send an immediate response to the client
// 						send(i, http_response, strlen(http_response), 0);
// 					}
// 				}
// 			}
// 			// // recv() is used to read data from the client.
// 			// // It returns the number of bytes actually read.
// 			// int bytes_read = recv(_client_fd, buffer, BUFFER_SIZE - 1, 0);
// 			// if (bytes_read > 0)
// 			// {
// 			// 	buffer[bytes_read] = '\0'; // creates proper null-terminated string;
// 			// 	std::cout << _client_fd << " Received: " << buffer << std::endl;

// 			// 	if (strncmp(buffer, "GET", 3) == 0)
// 			// 	{
// 			// 		// Define an HTTP response with a basic HTML page
// 			// 		const char *http_response =
// 			// 			"HTTP/1.1 200 OK\r\n"
// 			// 			"Content-Type: text/html\r\n"
// 			// 			"Content-Length: 410\r\n"
// 			// 			"\r\n"
// 			// 			"<html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>webserv</title><style>body{font-family:Arial,sans-serif;text-align:center;margin:50px;}h1{color:#333;}p{color:#666}</style></head><body><h1>Welcome to this website</h1><p>This is a basic webpage created with HTML and CSS</p><button onclick=\"alert('Hello!')\">Click Me</button></body></html>";

// 			// 		// Send response
// 			// 		send(_client_fd, http_response, strlen(http_response), 0);
// 			// 	}
// 			// close(_client_fd);
// 		}
// 	}
// }

