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
	size_t bodysize = httprequest.body.size();
	httprequest.headers.insert({"Content-Length", std::to_string(bodysize)});
}

static void saveUploadedFile(const HttpRequest& httprequest, const std::string& boundary)
{
	std::cout << "save file here" << std::endl;
	size_t start = httprequest.body.find(boundary);
	if (start == std::string::npos)
	{
		std::cout << "Boundary not found!" << std::endl;
		return;
	}
	start += boundary.length();
	std::cout << "start position: " << start << std::endl;

	size_t dispositionPos = httprequest.body.find("Content-Disposition:", start);
	if (dispositionPos ==  std::string::npos)
		return;
	std::cout << "Content-disposition position: " << dispositionPos << std::endl;
	
	size_t	filenamePos = httprequest.body.find("filename=\"", dispositionPos);
	if (filenamePos == std::string::npos)
		return;
	filenamePos += 10;
	std::cout << "filename position: " << filenamePos << std::endl;
	
	size_t filenameEnd = httprequest.body.find("\"", filenamePos);
	std::string fileName = httprequest.body.substr(filenamePos, filenameEnd - filenamePos);
	std::cout << "filename: " << fileName << std::endl;

	std::string filePath = joinPaths(STATIC_DIR + httprequest.path, fileName);
	std::cout << "registry path: " << filePath << std::endl;

	size_t dataStart = httprequest.body.find("\r\n\r\n", filenameEnd);
	if (dataStart == std::string::npos)
		return;
	dataStart += 4;
	std::cout << "data start position: " << dataStart << std::endl;

	size_t dataEnd = httprequest.body.find(boundary, dataStart);
	if (dataEnd == std::string::npos)
		return;
	dataEnd -= 2;
	std::cout << "data end position: " << dataEnd << std::endl;

	std::string fileData = httprequest.body.substr(dataStart, dataEnd - dataStart);
	std::cout << "file data: \n" << fileData << std::endl;

	std::ofstream outFile(filePath.c_str(), std::ios::binary);
	if (!outFile)
	{
		std::cout << "Error opening file for writing: " << fileName << std::endl;
		return;
	}

	outFile.write(fileData.c_str(), fileData.size());
	outFile.close();

	std::cout << "File saved: " << fileName << std::endl;
}

bool	parseRequest(const std::string request, HttpRequest& httprequest, const Server& server)
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
	httprequest.cgi = false;

	std::cout << "Method: " << httprequest.method << std::endl;
	std::cout << "Path: " << httprequest.path << std::endl;
	std::cout << "Version: " << httprequest.version << std::endl;

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
			std::cout	<< key << ": "
						<< value << std::endl;
			httprequest.headers[key] = value;
		}
	}




	int cgi_status;

	cgi_status = cgi_check(httprequest, server);
	if (cgi_status == 1)
	{
		std::cout << "YEP CGI STUFF FOUND AND WORKED CORRECTLY" << std::endl;
		std::cout << "RETURN FROM CGI SCRIPT (cgiBody):\n\n" << httprequest.cgiBody << std::endl;
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





	// Process body
	if (httprequest.headers.find("Content-Type") != httprequest.headers.end() &&
		httprequest.headers.at("Content-Type").find("multipart/form-data") != std::string::npos)
	{
		size_t bod = request.find("\r\n\r\n");
		httprequest.body = request.substr(bod + 4);
		std::cout << "upload body: \n" << httprequest.body << std::endl;

		std::string contentType = httprequest.headers.at("Content-Type");
		std::string boundary;
		std::string key = "boundary=";
		size_t pos = contentType.find(key);
		
		if (pos != std::string::npos)
			boundary = contentType.substr(pos + key.length());
		std::string::size_type car;
		while ((car = boundary.find('\r')) != std::string::npos)
			boundary.erase(car, 1);
		std::string closing_boundary = "--" + boundary + "--";

		std::cout << boundary << std::endl;
		std::cout << closing_boundary << std::endl;

		saveUploadedFile(httprequest, boundary);
	}
	else if (httprequest.headers.find("Content-Length") != httprequest.headers.end()) // Read body if Content-Length is provided
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
			bodytrail(request_stream, httprequest);
	}
	else
		bodytrail(request_stream, httprequest);
	return (true);
}

std::string	generateHttpResponse(HttpRequest& parsedRequest)
{
	std::string response;

	if (parsedRequest.path == "/favicon.ico")
		parsedRequest.path = "/";
	if (!parsedRequest.path.empty() && *parsedRequest.path.rbegin() != '/')
		parsedRequest.path.append("/");
	response = routeRequest(parsedRequest);
	return (response);
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
	// std::string filePath = STATIC_DIR + request.path + std::string("/index.html");
	std::ostringstream response;


	if (request.headers.find("Content-Type") != request.headers.end())
	{
		std::string contentType = request.headers.at("Content-Type");

		if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
		{
			std::string filePath = joinPaths((STATIC_DIR + request.path), "index.html");
			// return (serveStaticFile(filePath));
			response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
						<< (request.cgiBody.size()) << "\r\n\r\n"
						<< request.cgiBody;
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
		else if (contentType.find("multipart/form-data") != std::string::npos)
		{
			std::string filePath = joinPaths((STATIC_DIR + request.path), "upload.html");
			std::cout << "filePath: " << filePath << std::endl;
			return (serveStaticFile(filePath));
		}
		else
			response 	<< "HTTP/1.1 400 Bad Request\r\nContent-Length: 14\r\n\r\nInvalid Format";
	}
	else 
		response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nNo Content-Type";

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
	closedir(dir);
	return (response.str());
}

std::string	deleteFile(const HttpRequest &request)
{
	std::ostringstream response;
	std::cout << "delete file here" << std::endl;
	std::cout << request.path << std::endl;

	size_t	filenamePos = request.path.find("filename=");
	if (filenamePos == std::string::npos)
		return ("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nBad Query");
	filenamePos += 9;
	
	size_t filenameEnd = request.path.find("/", filenamePos);
	std::string fileName = "/uploads/" + request.path.substr(filenamePos, filenameEnd - filenamePos);
	std::cout << "filename: " << fileName << std::endl;
	std::string filePath = STATIC_DIR + fileName;
	std::cout << "filepath: " << filePath << std::endl;

	if (remove(filePath.c_str()) == 0)
		response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nFile Deleted";
	else
		response	<< "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nFile Not Found";
	return (response.str());
}

std::string routeRequest(const HttpRequest &request)
{
	std::string filePath = STATIC_DIR + request.path;
	if (request.method == "GET")
	{
		// if (request.cgi == true)
		// {
		// 	// set return body not to static file but cgi
		// }
		struct stat stats;
		std::cout << "after GET filling stats" << std::endl;
		std::cout << "this is the filepath: " << filePath << std::endl;
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
		else
		{
			if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
				filePath.erase(filePath.size() - 1);
			if (stat(filePath.c_str(), &stats) == 0)
			{
				std::cout << "Size: " << stats.st_size << " bytes" << std::endl;
				if (S_ISREG(stats.st_mode)) // checks if filePath is an existing file (registry)
				{
					std::cout << filePath << " is a registry!" << std::endl;
					return (serveStaticFile(filePath));
				}
			}
			else
				return (ER404);
		}
	}
	else if (request.method == "POST")
		return (handlePostRequest(request));
	else if (request.method == "DELETE")
		return (deleteFile(request));
	return (ER400);
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

