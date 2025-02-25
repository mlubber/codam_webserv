#include "../../headers/Server.hpp"

bool	parseRequest(const char* request, HttpRequest& httprequest)
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

	// std::cout << "Method: " << httprequest.method << std::endl;
	// std::cout << "Path: " << httprequest.path << std::endl;
	// std::cout << "Version: " << httprequest.version << std::endl;

	// Extract headers
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
	// Unchunk body
	if (httprequest.headers.find("Content-Length") != httprequest.headers.end()) // Else read body if Content-Length is provided
	{
		std::cout << "Content-Length found" << std::endl;
		int contentLength = std::atoi(httprequest.headers["Content-Length"].c_str());
		char bodyBuffer[contentLength + 1];
		request_stream.read(bodyBuffer, contentLength);
		bodyBuffer[contentLength] = '\0';
		httprequest.body = std::string(bodyBuffer);
		std::cout << "\nRegular Body: \n" << httprequest.body << std::endl;
	}
	else
	{
		std::string body;
		char		ch;
		while (request_stream.get(ch))
			body = body + ch;
		std::string	trail = (body.size() >= 5) ? body.substr(body.size() - 5, 5) : body; 
		if (trail == "0\r\n\r\n")
		{
			httprequest.body = dechunk(body);
			std::cout << "\nDechunked Body: \n" << httprequest.body << std::endl;
		}
		else
		{
			httprequest.body = body;
			std::cout << "\nNo-Length Body: \n" << httprequest.body << std::endl;
		}
	}
	return (true);
}

void	sendBadRequest(int client_fd)
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

std::string	getExtType(const std::string& filename)
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
		std::string extension = filename.substr(dotPos);
		if (types.find(extension) != types.end())
			return (types[extension]);
	}
	return ("application/octet-stream"); //
}

std::string serveStaticFile(const std::string& filePath)
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

std::string	handlePostRequest(const HttpRequest &request)
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
		else if (contentType.find("text/plain") != std::string::npos)
		{
			response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
						<< (request.body.size()) << "\r\n\r\n"
						<< request.body;
		} 
		else
			response 	<< "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nInvalid Format";
	} 
	else 
		response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nNo Content-Type";

	return (response.str());
}

std::string routeRequest(const HttpRequest &request)
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

void	printRequest(HttpRequest& httprequest)
{
	std::cout << "\n\nPRINT REQUEST:\n" << std::endl;
	std::cout << "Method: " << httprequest.method << std::endl;
	std::cout << "Path: " << httprequest.path << std::endl;
	std::cout << "Version: " << httprequest.version << std::endl;
	for (std::map<std::string, std::string>::iterator it = httprequest.headers.begin(); it != httprequest.headers.end(); it++)
		std::cout << it->first << ": " << it->second << std::endl;
	std::cout << "Body: " << httprequest.body << std::endl;
}

std::string dechunk(const std::string& input)
{
	std::istringstream	stream(input);
	std::string			decoded_body;
	std::string			line;

	int chunk_size = 0;
	getline(stream, line);
	do {
		std::istringstream hex_size_stream(line);
		hex_size_stream >> std::hex >> chunk_size;
		getline(stream, line);
		decoded_body.append(line.c_str(), chunk_size);
	} while (getline(stream, line));


	return (decoded_body);
}
