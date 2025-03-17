#include "../../headers/Server.hpp"

static std::string joinPaths(const std::string &path, const std::string &file) 
{
	if (!path.empty() && *path.rbegin() == '/')
		return (path + file);
	return (path + "/" + file);
}

static void bodytrail(std::istringstream& request_stream, HttpRequest& httprequest)
{
	std::string body;
	char		ch;
	while (request_stream.get(ch))
		body = body + ch;
	std::string	trail = (body.size() >= 5) ? body.substr(body.size() - 5, 5) : body; 
	if (trail == "0\r\n\r\n")
	{
		httprequest.body = dechunk(request_stream, body);
		std::cout << "\nDechunked Body: \n" << httprequest.body << std::endl;
	}
	else
	{
		httprequest.body = body;
		std::cout << "\nNo-Length Body: \n" << httprequest.body << std::endl;
	}
}

bool	parseRequest(const char* request, HttpRequest& httprequest, const Server& server)
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






	int cgi_status;

	cgi_status = cgi_check(httprequest, server);
	if (cgi_status == 1)
	{
		std::cout << "YEP CGI STUFF FOUND AND WORKED CORRECTLY" << std::endl;
		return (true);
	}
	else if (cgi_status == -1)
	{
		std::cout << "CGI FOUND BUT FAILED" << std::endl;
		return (false);
	}
	else if (cgi_status == 0)
	{
		std::cout << "NO CGI FOUND !!!!!!!" << std::endl;
	}
















	// Unchunk body
	if (httprequest.headers.find("Content-Length") != httprequest.headers.end()) // Read body if Content-Length is provided
	{
		std::cout << "Content-Length found" << std::endl;
		int contentLength = std::atoi(httprequest.headers["Content-Length"].c_str());
		char bodyBuffer[contentLength + 1];
		request_stream.read(bodyBuffer, contentLength);
		bodyBuffer[contentLength] = '\0';
		httprequest.body = std::string(bodyBuffer);
		std::cout << "\nRegular Body: \n" << httprequest.body << std::endl;
	}
	else if (httprequest.headers.find("Transfer-Encoding") != httprequest.headers.end())
	{
		std::string contentType = httprequest.headers.at("Transfer-Encoding");

		if (contentType.find("chunked") != std::string::npos)
		{
			std::cout << "chunked encoding found" << std::endl;
			httprequest.body = dechunk(request_stream, "0\r\n\r\n");
			std::cout << "\nDecoded body: \n" << httprequest.body << std::endl;
		}
		else
		{
			bodytrail(request_stream, httprequest);
			// std::string body;
			// char		ch;
			// while (request_stream.get(ch))
			// 	body = body + ch;
			// std::string	trail = (body.size() >= 5) ? body.substr(body.size() - 5, 5) : body; 
			// if (trail == "0\r\n\r\n")
			// {
			// 	httprequest.body = dechunk(request_stream, body);
			// 	std::cout << "\nNo chunked encoding still chunked Body: \n" << httprequest.body << std::endl;
			// }
			// else
			// {
			// 	httprequest.body = body;
			// 	std::cout << "\nNo chunked encoding not chunked Body: \n" << httprequest.body << std::endl;
			// }
		}
	}
	else
	{
		bodytrail(request_stream, httprequest);
		// std::string body;
		// char		ch;
		// while (request_stream.get(ch))
		// 	body = body + ch;
		// std::string	trail = (body.size() >= 5) ? body.substr(body.size() - 5, 5) : body; 
		// if (trail == "0\r\n\r\n")
		// {
		// 	httprequest.body = dechunk(request_stream, body);
		// 	std::cout << "\nDechunked Body: \n" << httprequest.body << std::endl;
		// }
		// else
		// {
		// 	httprequest.body = body;
		// 	std::cout << "\nNo-Length Body: \n" << httprequest.body << std::endl;
		// }
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
						<< (40 + request.body.size()) << "\r\n\r\n"
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

std::string	showDirList(const HttpRequest &request, const std::string& filePath)
{
	std::cout << "show directory listing" << std::endl;
	std::ostringstream response;
	std::ostringstream list;

	DIR *dir = opendir(filePath.c_str());
	if (!dir)
		return ("HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden");
	std::string home = STATIC_DIR;
	home.append("/");
	if (filePath != home)
		list << "<li><a href=\"../\">../</a></li>\n";

	struct dirent *entry;
	while ((entry = readdir(dir)))
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..") 
			continue;

		std::string href = request.path + name;
		if (entry->d_type == DT_DIR)
		{
			href = href + "/";
			list << "<li><a href=\"" << href << "\"><b>" << name << "/</b></a></li>\n";
		}
		else
			list << "<li><a href=\"" << href << "\">" << name << "</a></li>\n";
	}

	std::cout << list.str() << std::endl;
	std::cout << list.str().size() << std::endl;

	response	<< "HTTP/1.1 200 OK\r\n"
				<< "Content-Length: " << (113 + request.path.size() + list.str().size()) << "\r\n"
				<< "\r\n"
				<< "<!DOCTYPE html><html><head><title>Directory Listing</title></head>"
				<< "<body><h1>Index of " << request.path << "</h1><ul>"
				<< list.str();
	response	<< "</ul></body></html>";
	return (response.str());
}

std::string routeRequest(const HttpRequest &request)
{
	std::string filePath = STATIC_DIR + request.path;

	if (request.method == "GET")
	{
		struct stat stats;
		if (stat(filePath.c_str(), &stats) == 0) //fills stats with metadata from filePath if filePath exists
		{
			std::cout << "Size: " << stats.st_size << " bytes" << std::endl;
			if (S_ISDIR(stats.st_mode)) // checks if filePath is a working directory
			{
				std::cout << filePath << " is a directory!" << std::endl;
				std::string index = joinPaths(filePath, "index.html");
				std::cout << "looking for index: " << index << std::endl;
				if (stat(index.c_str(), &stats) == 0) // checks if the given index is present in the directory
				{
					std::cout << "index.html found!" << std::endl;
					std::cout << "filePath: " << index << std::endl;
					return (serveStaticFile(index));
				}
				else
					std::cout << "no index found" << std::endl;
				return (showDirList(request, filePath));
			}
		}
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

std::string dechunk(std::istream& stream, const std::string& input)
{
	std::istringstream	charstream(input);
	std::string			decoded_body;
	std::string			line;

	if (input != "0\r\n\r\n")
	{
		stream.rdbuf(charstream.rdbuf());
		std::cout << "no encoding found" << std::endl;
	}
	int chunk_size = 0;
	getline(stream, line);
	do {
		std::istringstream hex_size_stream(line);
		hex_size_stream >> std::hex >> chunk_size;
		if (chunk_size == 0)
            break;
		getline(stream, line);
		decoded_body.append(line.c_str(), chunk_size);
	} while (getline(stream, line));


	return (decoded_body);
}

