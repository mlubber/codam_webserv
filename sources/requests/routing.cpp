#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/Client.hpp"

static void serveStaticFile(Client& client, const std::string& filePath, std::string status_code)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file)
	{
		serveError(client, "404", client.getServerBlock());
		return ;
	}
	
	std::ostringstream fileStream;
	fileStream << file.rdbuf();
	std::string fileContent = fileStream.str();

	std::string	contentType = getExtType(filePath);

	std::ostringstream response;

	if (status_code == "200")
		response << "HTTP/1.1 200 OK\r\n";
		
	else if (status_code == "400")
		response << "HTTP/1.1 400 Bad Request\r\n";
	else if (status_code == "403")
		response << "HTTP/1.1 403 Forbidden\r\n";
	else if (status_code == "404")
		response << "HTTP/1.1 404 Not Found\r\n";
	else if (status_code == "405")
		response << "HTTP/1.1 405 Method Not Allowed\r\n";
	else if (status_code == "408")
		response << "HTTP/1.1 408 Request Timeout\r\n";
	else if (status_code == "413")
		response << "HTTP/1.1 413 Payload Too Large\r\n";
	else if (status_code == "500")
		response << "HTTP/1.1 500 Internal Server Error\r\n";

	response << "Content-Type: " << contentType << "\r\n";
	response << "Content-Length: " << fileContent.size() << "\r\n";
	response << "\r\n";
	response << fileContent;

	client.setResponseData(response.str());
	client.setClientState(sending_response);
}

void serveError(Client& client, std::string error_code, const ConfigBlock& serverBlock)
{
	std::cerr << "Serving error: " << error_code << std::endl;
	client.setCloseClientState(true);
	std::string root = client.getServerBlockInfo("root");
	
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
                    if (++it != errorValues.end())
                        path = *it;
                    break;
                }
            }
        }
    }

    static const std::map<std::string, std::string> defaultErrors = {
        {"400", ER400},
        {"403", ER403},
        {"404", ER404},
        {"405", ER405},
		{"408", ER408},
        {"413", ER413},
        {"500", ER500}
    };

	std::string filePath = root + path;

	struct stat stats;
	if (!path.empty() && stat(filePath.c_str(), &stats) == 0)
	{
		serveStaticFile(client, filePath, error_code);
		return ;
	}

	client.setClientState(sending_response);
	auto it = defaultErrors.find(error_code);
	if (it != defaultErrors.end())
	{
		client.setResponseData(it->second);
		return ;
	}
	client.setResponseData(ER500);
}

static void saveFile(const clRequest& cl_request, const std::string& boundary, const ConfigBlock& serverBlock)
{
	std::string root;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "root")
			root = value.second.front();
	if (root[0] != '.')
		root = "." + root;
	
	size_t start = cl_request.body.find(boundary);
	if (start == std::string::npos)
		return;
	start += boundary.length();

	size_t dispositionPos = cl_request.body.find("Content-Disposition:", start);
	if (dispositionPos ==  std::string::npos)
		return;
	
	size_t	filenamePos = cl_request.body.find("filename=\"", dispositionPos);
	if (filenamePos == std::string::npos)
		return;
	filenamePos += 10;
	
	size_t filenameEnd = cl_request.body.find("\"", filenamePos);
	std::string fileName = cl_request.body.substr(filenamePos, filenameEnd - filenamePos);

	std::string filePath = joinPaths(root + cl_request.path, fileName);

	size_t dataStart = cl_request.body.find("\r\n\r\n", filenameEnd);
	if (dataStart == std::string::npos)
		return;
	dataStart += 4;

	size_t dataEnd = cl_request.body.find(boundary, dataStart);
	if (dataEnd == std::string::npos)
		return;
	dataEnd -= 2;

	std::string fileData = cl_request.body.substr(dataStart, dataEnd - dataStart);

	std::ofstream outFile(filePath.c_str(), std::ios::binary);
	if (!outFile)
		return;

	outFile.write(fileData.c_str(), fileData.size());
	outFile.close();
}

static void	handlePostRequest(Client& client, clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::ostringstream response;
	std::string root = client.getServerBlockInfo("root");
	if (root[0] != '.')
		root = "." + root;

	size_t maxBody = MAX_BODY_SIZE;
	for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values)
		if (value.first == "client_max_body_size")
			maxBody = std::stoul(value.second.front());

	if (cl_request.body.size() > maxBody)
	{
		serveError(client, "413", serverBlock);
		return ;
	}

	if (cl_request.headers.find("content-type") != cl_request.headers.end())
	{
		const std::vector<std::string>& values_vector = cl_request.headers.at("content-type");

		for (size_t i = 0; i < values_vector.size(); i++)
		{
			if (values_vector[i].find("text/plain") != std::string::npos || values_vector[i].find("plain/text") != std::string::npos)
			{
				response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
							<< (cl_request.body.size()) << "\r\n\r\n"
							<< cl_request.body;
				client.setResponseData(response.str());
				return ;
			}
			else if (values_vector[i].find("application/x-www-form-urlencoded") != std::string::npos)
			{
				response	<< "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
							<< (cl_request.body.size()) << "\r\n\r\n"
							<< cl_request.body;
				client.setResponseData(response.str());
				return ;
				}
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
					saveFile(cl_request, boundary, serverBlock);
				}

				std::string filePath = joinPaths((root + cl_request.path), "upload.html");
				serveStaticFile(client, filePath, "200");
				return ;
			}
			else
				client.setResponseData("HTTP/1.1 400 Bad Request\r\nContent-Length: 14\r\n\r\nInvalid Format");
		}
	}
	else
		client.setResponseData("HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nNo Content-Type");

}




void	showDirList(Client& client, clRequest& cl_request, const std::string& filePath, const ConfigBlock& serverBlock)
{
	std::string root = client.getServerBlockInfo("root");
	if (root[0] != '.')
		root = "." + root;
	
	std::ostringstream response;
	std::ostringstream list;

	DIR *dir = opendir(filePath.c_str());
	if (!dir)
	{
		serveError(client, "403", serverBlock);
		return ;
	}
	std::string home = root;
	if (!home.empty() && *home.rbegin() != '/')
		home.append("/");

	if (!cl_request.path.empty() && *cl_request.path.rbegin() != '/')
		cl_request.path.append("/");
	
	std::string cutPath;
	if (!cl_request.path.empty() && cl_request.path[cl_request.path.size() - 1] == '/')
		cutPath = cl_request.path.substr(0, cl_request.path.size() - 1);
	cutPath = cutPath.substr(0, cutPath.find_last_of('/'));
	if (cutPath.empty())
		cutPath = "../";

	if (filePath != home)
		list << "<li><a href=\"" << cutPath << "\">../</a></li>\n";

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


	response	<< "HTTP/1.1 200 OK\r\n"
				<< "Content-Length: " << (113 + cl_request.path.size() + list.str().size()) << "\r\n"
				<< "\r\n"
				<< "<!DOCTYPE html><html><head><title>Directory Listing</title></head>"
				<< "<body><h1>Index of " << cl_request.path << "</h1><ul>"
				<< list.str();
	response	<< "</ul></body></html>";
	closedir(dir);
	client.setResponseData(response.str());
	return ;
}

static void	deleteFile(Client& client, clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::ostringstream response;
	
	std::string root = client.getServerBlockInfo("root");
	if (root[0] != '.')
		root = "." + root;
	
	size_t	filenamePos = cl_request.queryStr.find("filename=");
	if (filenamePos == std::string::npos)
	{
		client.setResponseData("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nBad Query");
		return ;
	}
	filenamePos += 9;

	std::string uploads = "/uploads/";
	for (const std::pair<const std::string, ConfigBlock>& nested : serverBlock.nested)
	{
		for (const std::pair<const std::string, std::vector<std::string>>& value : nested.second.values)
		{
			if (value.first == "upload_store")
			{
				uploads = value.second.front();
				if (!uploads.empty() && *uploads.rbegin() != '/')
					uploads.append("/");
			}
		}
	}
	std::string fileName = uploads + cl_request.queryStr.substr(filenamePos);
	std::string filePath = root + fileName;

	if (remove(filePath.c_str()) == 0)
		client.setResponseData("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nFile Deleted");
	else
		client.setResponseData("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nFile Not Found");
}

void routeRequest(Client& client, const Server& server, clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::string root = client.getServerBlockInfo("root");
	if (root[0] != '.')
		root = "." + root;

	cl_request.queryStr = urlDecode(cl_request.queryStr);

	ConfigBlock	locBlock;
	std::string	locPath;
	std::string locConf = extract_first_word(cl_request.path);
	for (const std::pair<const std::string, ConfigBlock> &nested : serverBlock.nested)
	{
		for (const std::pair<const std::string, std::vector<std::string>> &value : nested.second.values)
		{
			if (value.first == "location")
			{
				std::string tempLoc = value.second.front();
				if (!tempLoc.empty() && *tempLoc.rbegin() != '/')
					tempLoc.append("/");
				std::string tempPath = cl_request.path;
				if (!tempPath.empty() && *tempPath.rbegin() != '/')
					tempPath.append("/");
				if (tempPath.find(tempLoc) == 0)
				{
					locPath = tempLoc;
					if (locPath != locConf)
						locPath.erase();
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
			client.setResponseData(response.str());
            return ;
        }
    }

	std::string index;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
		if (value.first == "index")
			index = value.second.front();
	if (index.empty())
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
			cl_request.path = upload_store;
		}
	}
	std::string filePath;

	std::string rootOverride;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
		if (value.first == "root")
			rootOverride = value.second.front();
	if (!rootOverride.empty() && rootOverride[0] != '.')
		rootOverride = "." + rootOverride;
	if (!rootOverride.empty())
	{
		filePath = rootOverride + cl_request.path.substr(locPath.size() - 1);
	}
	else
		filePath = root + cl_request.path;

	std::vector<std::string> methods;
	for (const std::pair<const std::string, std::vector<std::string>> &value : locBlock.values)
	{
		if (value.first == "methods")
			methods = value.second;
	}
	if (std::find(methods.begin(), methods.end(), cl_request.method) == methods.end())
	{
		serveError(client, "405", serverBlock);
		return ;
	}

	if (cl_request.method == "GET")
	{
		struct stat stats;
		if (stat(filePath.c_str(), &stats) == 0)
		{
			if (S_ISDIR(stats.st_mode) && !locPath.empty())
			{
				std::string fullPath = joinPaths(filePath, index);
				if (stat(fullPath.c_str(), &stats) == 0)
				{
					serveStaticFile(client, fullPath, "200");
					return ;
				}
				else
				{
					if (autoindex == "on")
					{
						showDirList(client, cl_request, filePath, serverBlock);
						return ;
					}
					else
					{
						serveError(client, "404", serverBlock);
						return ;
					}
				}
			}
			else if (S_ISREG(stats.st_mode))
			{
				if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
					filePath.erase(filePath.size() - 1);
				if (check_path(filePath, locPath) == 1)
				{
					serveError(client, "404", serverBlock);
					return ;
				}
				if (cgi_check(cl_request.path))
				{
					int status = start_cgi(cl_request, server, client);
					if (status != 0)
					{
						serveError(client, "500", serverBlock);
						if (client.checkCgiPtr() && client.getCgiStruct().child_pid != -1)
							kill(client.getCgiStruct().child_pid, SIGTERM);
						return ;
					}
					return ;
				}
				else
					serveStaticFile(client, filePath, "200");
				return ;
			}
			else
			{
				std::cout << "not a directory or registry. filepath not found" << std::endl;
				serveError(client, "404", serverBlock);
				return ;
			}
		}
		else
		{
			serveError(client, "404", serverBlock);
			return ;
		}
	}
	else if (cl_request.method == "POST")
	{
		if (check_path(filePath, locPath) == 1)
		{
			serveError(client, "404", serverBlock);
			return ;
		}
		if (cgi_check(cl_request.path))
		{
			int status = start_cgi(cl_request, server, client);
			if (status != 0)
			{
				serveError(client, "500", serverBlock);
				if (client.checkCgiPtr() && client.getCgiStruct().child_pid != -1)
				{
					kill(client.getCgiStruct().child_pid, SIGTERM);
					waitpid(client.getCgiStruct().child_pid, &status, 0);
				}
				return ;
			}
			return ;
		}
		else
			handlePostRequest(client, cl_request, serverBlock);
		return ;
	}
	else if (cl_request.method == "DELETE")
	{
		deleteFile(client, cl_request, serverBlock);
		return ;
	}
	serveError(client, "400", client.getServerBlock());
}

