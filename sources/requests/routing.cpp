#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"

static std::string serveStaticFile(const std::string& filePath)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file)
		return (ER404);
	
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

std::string serveError(std::string error_code, const ConfigBlock& serverBlock)
{
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();
	
	std::string path;
    for (const std::pair<const std::string, std::vector<std::string>>& value : serverBlock.values) 
    {
        if (value.first == "error_page") 
        {
            const std::vector<std::string>& errorValues = value.second;
            for (std::vector<std::string>::const_iterator it = errorValues.begin(); it != errorValues.end(); ++it) 
            {
                if (*it == error_code) 
                {
                    std::cout << error_code << " FOUND" << std::endl;
                    if (++it != errorValues.end()) // Check if the next value exists
                    {
                        path = *it; // Save the next value
                        std::cout << "Path for Error " << error_code << ": " << path << std::endl;
                    }
                    break; // Exit the loop after finding the next value
                }
            }
        }
    }

    static const std::map<std::string, std::string> defaultErrors = {
        {"400", ER400},
        {"403", ER403},
        {"404", ER404},
        {"405", ER405},
        {"413", ER413},
        {"500", ER500}
    };

	std::string filePath = root + path;
	std::cout << "filePath for error: " << filePath << std::endl;
	std::cout << "error_code: " << error_code << std::endl;

	struct stat stats;
	if (!path.empty() && stat(filePath.c_str(), &stats) == 0)
		return (serveStaticFile(filePath)); // Serve the custom error page

	// Return the default error response if no custom page is found
	auto it = defaultErrors.find(error_code);
	if (it != defaultErrors.end())
		return (it->second);

    return (ER500); // Fallback to 500 Internal Server Error
}

static void saveFile(const clRequest& cl_request, const std::string& boundary, const ConfigBlock& serverBlock)
{
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();
	
	std::cout << "save file client" << std::endl;
	size_t start = cl_request.body.find(boundary);
	if (start == std::string::npos)
	{
		std::cout << "Boundary not found!" << std::endl;
		return;
	}
	start += boundary.length();
	std::cout << "start position: " << start << std::endl;

	size_t dispositionPos = cl_request.body.find("Content-Disposition:", start);
	if (dispositionPos ==  std::string::npos)
		return;
	std::cout << "Content-disposition position: " << dispositionPos << std::endl;
	
	size_t	filenamePos = cl_request.body.find("filename=\"", dispositionPos);
	if (filenamePos == std::string::npos)
		return;
	filenamePos += 10;
	std::cout << "filename position: " << filenamePos << std::endl;
	
	size_t filenameEnd = cl_request.body.find("\"", filenamePos);
	std::string fileName = cl_request.body.substr(filenamePos, filenameEnd - filenamePos);
	std::cout << "filename: " << fileName << std::endl;

	std::cout << "client request path: " << cl_request.path << std::endl;
	std::string filePath = joinPaths(root + cl_request.path, fileName);
	std::cout << "registry path: " << filePath << std::endl;

	size_t dataStart = cl_request.body.find("\r\n\r\n", filenameEnd);
	if (dataStart == std::string::npos)
		return;
	dataStart += 4;
	std::cout << "data start position: " << dataStart << std::endl;

	size_t dataEnd = cl_request.body.find(boundary, dataStart);
	if (dataEnd == std::string::npos)
		return;
	dataEnd -= 2;
	std::cout << "data end position: " << dataEnd << std::endl;

	std::string fileData = cl_request.body.substr(dataStart, dataEnd - dataStart);
	// std::cout << "file data: \n" << fileData << std::endl;

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

static std::string	handlePostRequest(clRequest& cl_request, const ConfigBlock& serverBlock)
{
	// std::string filePath = STATIC_DIR + request.path + std::string("/index.html");
	std::ostringstream response;
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();


	size_t maxBody = MAX_BODY_SIZE;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "client_max_body_size")
			maxBody = std::stoul(value.second.front());

	std::cout << "client_max_body_size: " << maxBody << std::endl;
	std::cout << "cl_request.body.size: " << cl_request.body.size() << std::endl;
	if (cl_request.body.size() > maxBody)
	{
		std::cout << "body too big!" << std::endl;
		return (serveError("413", serverBlock));
	}

	if (cl_request.headers.find("content-type") != cl_request.headers.end())
	{
		const std::vector<std::string>& values_vector = cl_request.headers.at("content-type");

		for (size_t i = 0; i < values_vector.size(); i++)
		{
			if (values_vector[i].find("text/plain") != std::string::npos)
			{
				response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
							<< (cl_request.body.size()) << "\r\n\r\n"
							<< cl_request.body;
			}
			// else if (values_vector[i].find("application/x-www-form-urlencoded") != std::string::npos)
			// {
			// 	std::cout << "application/x-www-form-urlencoded found! " << cl_request.cgiBody << std::endl;
			// 	response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
			// 				<< (cl_request.cgiBody.size()) << "\r\n\r\n"
			// 				<< cl_request.cgiBody;
			// }
			else if (values_vector[i].find("multipart/form-data") != std::string::npos)
			{
				const std::vector<std::string>& contentType = cl_request.headers.at("content-type");
				for (size_t i = 0; i < contentType.size(); i++)
				{
					std::string boundary;
					std::string key = "boundary=";
					size_t pos = contentType[i].find(key);
					if (pos != std::string::npos)
						boundary = contentType[i].substr(pos + key.length());
					std::string::size_type car;
					while ((car = boundary.find('\r')) != std::string::npos)
						boundary.erase(car, 1);
					std::string closing_boundary = "--" + boundary + "--";

					std::cout << "BOUNDARY: " << boundary << std::endl;
					std::cout << "CLOSING:" << closing_boundary << std::endl;

					saveFile(cl_request, boundary, serverBlock);
				}

				std::string filePath = joinPaths((root + cl_request.path), "upload.html");
				std::cout << "filePath: " << filePath << std::endl;
				return (serveStaticFile(filePath));
			}
			else
				response 	<< "HTTP/1.1 400 Bad Request\r\nContent-Length: 14\r\n\r\nInvalid Format";
		}
	}
	else
		response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nNo Content-Type";
	return (response.str());

	// if (request.headers.find("Content-Type") != request.headers.end())
	// {
	// 	std::string contentType = request.headers.at("Content-Type");

	// 	if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
	// 	{
	// 		std::string filePath = joinPaths((root + request.path), "index.html");
	// 		// return (serveStaticFile(filePath));
	// 		response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
	// 					<< (request.cgiBody.size()) << "\r\n\r\n"
	// 					<< request.cgiBody;
	// 	} 
	// 	else if (contentType.find("application/json") != std::string::npos)
	// 	{
	// 		response	<< "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
	// 					<< (40 + request.body.size()) << "\r\n\r\n"
	// 					<< "{ \"message\": \"Received JSON\", \"data\": " << request.body << " }";
	// 	}
	// 	else if (contentType.find("text/plain") != std::string::npos)
	// 	{
	// 		response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
	// 					<< (request.body.size()) << "\r\n\r\n"
	// 					<< request.body;
	// 	}
	// 	else if (contentType.find("multipart/form-data") != std::string::npos)
	// 	{
	// 		std::string filePath = joinPaths((root + request.path), "upload.html");
	// 		std::cout << "filePath: " << filePath << std::endl;
	// 		return (serveStaticFile(filePath));
	// 	}
	// 	else
	// 		response 	<< "HTTP/1.1 400 Bad Request\r\nContent-Length: 14\r\n\r\nInvalid Format";
	// }
	// else 
	// 	response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nNo Content-Type";
	// (void)cl_request;
	// return (response.str());
}

std::string	showDirList(clRequest& cl_request, const std::string& filePath, const ConfigBlock& serverBlock)
{
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();
	
	std::cout << "show directory listing" << std::endl;
	std::ostringstream response;
	std::ostringstream list;

	DIR *dir = opendir(filePath.c_str());
	if (!dir)
		return (serveError("403", serverBlock));
	std::string home = root;
	if (!home.empty() && *home.rbegin() != '/')
		home.append("/");
	if (filePath != home)
		list << "<li><a href=\"../\">../</a></li>\n";

	struct dirent *entry;
	while ((entry = readdir(dir)))
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..") 
			continue;

		std::string href = cl_request.path + name;
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
				<< "Content-Length: " << (113 + cl_request.path.size() + list.str().size()) << "\r\n"
				<< "\r\n"
				<< "<!DOCTYPE html><html><head><title>Directory Listing</title></head>"
				<< "<body><h1>Index of " << cl_request.path << "</h1><ul>"
				<< list.str();
	response	<< "</ul></body></html>";
	closedir(dir);
	return (response.str());
}

static std::string	deleteFile(clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::ostringstream response;
	std::cout << "delete file here" << std::endl;
	std::cout << cl_request.path << std::endl;
	std::cout << cl_request.queryStr << std::endl;

	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();
	
	size_t	filenamePos = cl_request.queryStr.find("filename=");
	std::cout << "filenamepos: " << filenamePos << std::endl;
	if (filenamePos == std::string::npos)
		return ("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nBad Query");
	filenamePos += 9;

	std::string uploads = "/uploads/";
	for (const std::pair<const std::string, ConfigBlock>& nested : serverBlock.nested)
	{
		for (const std::pair<const std::string, std::vector<std::string>>& value : nested.second.values)
		{
			if (value.first == "upload_store")
			{
				std::cout << "Found upload_store in location: " << nested.first << std::endl;
				std::cout << "Upload store path: " << value.second.front() << std::endl;
				uploads = value.second.front();
				if (!uploads.empty() && *uploads.rbegin() != '/')
				uploads.append("/");
			}
		}
	}
	std::cout << "uploads path: " << uploads << std::endl;

	std::string fileName = uploads + cl_request.queryStr.substr(filenamePos);

	std::cout << "filename: " << fileName << std::endl;
	std::string filePath = root + fileName;
	std::cout << "filepath: " << filePath << std::endl;

	if (remove(filePath.c_str()) == 0)
		response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nFile Deleted";
	else
		response	<< "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nFile Not Found";
	return (response.str());
}

std::string routeRequest(clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();

	cl_request.queryStr = urlDecode(cl_request.queryStr);

	ConfigBlock	locBlock;
	std::string	locPath;
	// std::cout << "cl_request path: " << cl_request.path << std::endl;
	for (const std::pair<const std::string, ConfigBlock> &nested : serverBlock.nested)
	{
		for (const std::pair<const std::string, std::vector<std::string>> &value : nested.second.values)
		{
			if (value.first == "location")
			{
				std::string temp = value.second.front();
				if (!temp.empty() && *temp.rbegin() != '/')
					temp.append("/");
				if (cl_request.path.find(temp) == 0)
				{
					locPath = value.second.front();
					// std::cout << "locationPath: " << locPath << std::endl;
					locBlock = nested.second;
				}
				break;
			}
		}
	}
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
    {
        if (value.first == "return" && value.second.size() >= 2)
        {
			std::ostringstream response;
			response << "HTTP/1.1 " << value.second[0] << " Moved Permanently\r\n"
					<< "Location: " << value.second[1] << "\r\n"
					<< "Content-Length: 0\r\n"
					<< "Connection: close\r\n"
					<< "\r\n";
            return (response.str());
        }
    }

	std::string index;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
		if (value.first == "index")
			index = value.second.front();
	if (!index.empty())
		std::cout << "index found in nested block: " << index << std::endl;
	else
		index = "index.html";

	std::string autoindex;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
		if (value.first == "autoindex")
			autoindex = value.second.front();

	std::string upload_store;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
	{
		if (value.first == "upload_store")
		{
			upload_store = value.second.front();
			if (!upload_store.empty() && *upload_store.rbegin() != '/')
				upload_store.append("/");
			// std::cout << "alt upload location: " << upload_store << std::endl;
			// std::cout << "client request path: " << cl_request.path << std::endl;
			cl_request.path = upload_store;
		}
	}
	
	std::string filePath;

	std::string rootOverride;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
		if (value.first == "root")
			rootOverride = value.second.front();
	if (!rootOverride.empty())
	{
		// std::cout << "root found in nested block: " << rootOverride << std::endl;
		filePath = rootOverride + cl_request.path.substr(locPath.size());
	}
	else
		filePath = root + cl_request.path;

	std::vector<std::string> methods;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
	{
		if (value.first == "methods")
		{
			methods = value.second;
			// for (const std::string& method : methods)
			// 	std::cout << method << " ";
			// std::cout << std::endl;
		}
	}
	if (std::find(methods.begin(), methods.end(), cl_request.method) == methods.end())
	{
		// std::cout << "Method not allowed: " << cl_request.method << std::endl;
		return (serveError("405", serverBlock));
	}
	
	if (cl_request.method == "GET")
	{
		struct stat stats;
		// std::cout << "after GET filling stats" << std::endl;
		// std::cout << "this is the filepath: " << filePath << std::endl;
		if (stat(filePath.c_str(), &stats) == 0) //fills stats with metadata from filePath if filePath exists
		{
			// std::cout << "Size: " << stats.st_size << " bytes" << std::endl;
			if (S_ISDIR(stats.st_mode) && !locPath.empty()) // checks if filePath is a working directory
			{
				// std::cout << filePath << " is a directory!" << std::endl;

				std::string fullPath = joinPaths(filePath, index);

				// std::cout << "looking for index: " << fullPath << std::endl;
				if (stat(fullPath.c_str(), &stats) == 0) // checks if the given index is present in the directory
				{
					// std::cout << "index found!" << std::endl;
					// std::cout << "filePath: " << fullPath << std::endl;
					return (serveStaticFile(fullPath));
				}
				else
				{
					// std::cout << "no index found" << std::endl;
					if (autoindex == "on")
						return (showDirList(cl_request, filePath, serverBlock));
					else
						return (serveError("404", serverBlock));
				}
			}
			else
				return (serveError("404", serverBlock));
		}
		else
		{
			if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
				filePath.erase(filePath.size() - 1);
			if (stat(filePath.c_str(), &stats) == 0)
			{
				// std::cout << "Size: " << stats.st_size << " bytes" << std::endl;
				if (S_ISREG(stats.st_mode)) // checks if filePath is an existing file (registry)
				{
					// std::cout << filePath << " is a registry!" << std::endl;
					return (serveStaticFile(filePath));
				}
			}
			else
				return (serveError("404", serverBlock));
		}
	}
	else if (cl_request.method == "POST")
		return (handlePostRequest(cl_request, serverBlock));
	else if (cl_request.method == "DELETE")
		return (deleteFile(cl_request, serverBlock));
	return (ER400);
}

