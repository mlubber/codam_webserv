#include "../headers/Host.hpp"
#include "../headers/Client.hpp"
#include <sstream>
#include <exception>
#include <algorithm>

std::vector<std::string> headerSingleValue = {"authorization","content-type","content-length",	
	"date","host","location","server","set-cookie","user-agent","x-requested-with","cache-control",
	"if-modified-since","etag","referer","upgrade-insecure-requests","x-frame-options","expect",
	"x-content-type-options","content-disposition","strict-ransport-security","content-encoding"};

std::vector<std::string> MultieValueByComma = {"accept", "accept-encoding",
	"accept-language", "cache-control", "if-none-match", "via", "warning", "x-content-type-options",
	"ink", "content-encoding", "accept-charset", "accept-features", "x-frame-options"};

std::map<std::string, bool> isHeaderAppearTwice = {{"authorization" , false}, {"content-type", false},
{"content-length", false}, {"host", false}, {"User-Agent", false}, {"upgrade-insecure-requests", false},
{"x-frame-options", false}, {"x-content-type-options", false}, {"strict-transport-security", false}, {"content-disposition", false}};


std::string headerMultieValueSeparatedWithSemicolon = "cookie";

void	resetStruct(clRequest& clRequest)
{
	clRequest.invalidRequest = false;
	clRequest.method.clear();
	clRequest.path.clear();
	if (!clRequest.queryStr.empty())
		clRequest.queryStr.clear();
	clRequest.headers.clear();
	clRequest.body.clear();
	clRequest.port.clear();
	clRequest.host.clear();
	for (auto &header : isHeaderAppearTwice)
	{
		header.second = false;
	}
	clRequest.cgi = false;
}

int	parseHeaderMultiValueComma(std::string &headerName ,std::string &line, clRequest &clRequest)
{
	if (isHeaderAppearTwice.find(headerName) != isHeaderAppearTwice.end())
	{
		if (isHeaderAppearTwice[headerName])
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		isHeaderAppearTwice[headerName] = true;

	}
	std::vector<std::string> temp;
	size_t pos = 0;
	std::string subValue;
	while ( (pos = line.find_first_of(',')) != std::string::npos)
	{
		subValue = line.substr(0, pos);
		temp.push_back(subValue);
		line = line.substr(pos + 1);
	}
	temp.push_back(line);
	clRequest.headers[headerName] = temp;
	return (0);
}

int	parseHeaderSingleValue(std::string &headerName ,std::string &line, clRequest &clRequest, Client& client)
{
	if (isHeaderAppearTwice.find(headerName) != isHeaderAppearTwice.end())
	{
		if (isHeaderAppearTwice[headerName])
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		isHeaderAppearTwice[headerName] = true;

	}
	
	if (headerName == "host")
	{
		clRequest.headers[headerName].push_back(line);
		size_t pos = 0;
		pos = line.find_first_of(':');
		if (pos != std::string::npos)
		{
			clRequest.host = line.substr(0, pos);
			if (pos + 1 < line.size()) {
				clRequest.port = line.substr(pos + 1);
			}
		}
		else
		{
			clRequest.host = line;
			clRequest.port = 80;
		}
		return (0);
	}
	clRequest.headers[headerName].push_back(line);
	if (clRequest.headers.find("connection") != clRequest.headers.end())
	{
		if (clRequest.headers["connection"].front() == "close")
		{
			std::cout << "SETTING CLIENT CLOSE STATE TO TRUE" << std::endl;
			client.setCloseClientState(true);
		}
	}
	return (0);
}

void trim(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

int	parseRequestHeaders(std::string &line, clRequest& clRequest, Client& client) 
{

	size_t pos = 0;
	pos = line.find_first_of(':');
	if (pos == std::string::npos)
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	if (isspace(line[pos - 1]))
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	std::string headerName = line.substr(0, pos);
	std::string headerValue = line.substr(pos + 1);
	trim(headerValue);
	if (headerValue.empty())
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	
	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

	if (headerName ==  "expect" && headerValue == "100-continue")
		clRequest.hundredContinue = true;
	if (std::any_of(headerName.begin(), headerName.end(), [](unsigned char c) {return std::isspace(c);}))
	{
		std::cerr << "header name (key) can't have any space! => " << headerName << std::endl;
		clRequest.invalidRequest = true;
		return (1);
	}

	if (std::find(MultieValueByComma.begin(), MultieValueByComma.end(), headerName) != MultieValueByComma.end())
	{
		if (parseHeaderMultiValueComma(headerName, headerValue, clRequest))
			return (1);
	}
	else if (headerName.compare("cookie") == 0)
	{
		// multie value cookie
	}
	else
	{
		if (parseHeaderSingleValue(headerName, headerValue, clRequest, client))
			return (1);
	}
	return (0);
}



int	parseRequestLine(std::string &line, clRequest &clRequest)
{
	int wordCounter = 0;
	std::string word;
	size_t pos = 0;

	if (line.size() > 8192 )
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	while(wordCounter < 3)
	{
		if (wordCounter < 2)
		{
			pos = line.find_first_of(' ');
			if (pos == std::string::npos)
			{
				clRequest.invalidRequest = true;
				return (1);
			}
			word = line.substr(0, pos);
			line = line.substr(pos + 1);
		} 
		else if (wordCounter == 2)
			word = line;
		++wordCounter;
		
		std::string query;
		switch (wordCounter)
		{
		case 1:
			clRequest.method = word;
			if (word != "POST" && word != "GET" && word != "DELETE")
			{
				clRequest.methodNotAllowd = true;
				return (1);
			}
			break;
		case 2:
			pos = word.find('?');
			if (pos == std::string::npos)
				clRequest.path = word;
			else
			{
				query = word.substr(pos + 1, word.size() - pos);
				word = word.substr(0, pos);
				clRequest.path = word;
				clRequest.queryStr = query;
			}
			break;
		case 3:
			if (word.compare("HTTP/1.1") != 0)
			{
				clRequest.invalidRequest = true;
				return (1);
			}
			break;
		default:
			clRequest.invalidRequest = true;
			return (1);
			break;
		}
	}
	return (0);
}

int	parseChunkedBody(std::string &body, clRequest &clRequest)
{
	std::cout << "---chunked body---" << std::endl;
	if (clRequest.headers.find("transfer-encoding") == clRequest.headers.end())
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	bool	isChunked = false;
	for(size_t i = 0; i < clRequest.headers["transfer-encoding"].size(); ++i)
	{
		if (clRequest.headers["transfer-encoding"][i] == "chunked") 
			isChunked = true;	
	}
	if (!isChunked)
	{
		clRequest.invalidRequest = true;
		return (1);
	}
	std::string line;
	std::string	hexSize;
	size_t		pos = 0;
	int			intSize = 0;
	std::istringstream	request_body(body);
	std::string	chunkText;
	while (getline(request_body, line))
	{
		pos = line.find('\r');
		if (pos == std::string::npos)
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		hexSize = line.substr(0, pos);
		try
		{
			intSize = std::stoi(hexSize,0,16);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error:" << e.what() << std::endl;
			clRequest.invalidRequest = true;
			return (1);
		}
		if (intSize == 0 && line.size() >= 1 && line[1] == '\r')
		{
			line.clear();
			getline(request_body, line);
			if (line[0] == '\r')
				break;
		}
		line.clear();
		getline(request_body, line);
		pos = line.find('\r');
		if (pos == std::string::npos)
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		line = line.substr(0, pos);
		if (line.size() < static_cast<size_t>(intSize))
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		line = line.substr(0, intSize);
		chunkText = chunkText + line;
	}
	clRequest.body = chunkText;
	return (0);
}

int	parseBody(std::string &body, clRequest &clRequest)
{
	if (clRequest.method != "POST") 
		return (0);
	std::string strBodyLength, bodyHasToRead;
	size_t	bodyLength = 0;
	if (clRequest.headers.find("content-length") != clRequest.headers.end())
	{
		strBodyLength = clRequest.headers["content-length"].front();
		try
		{
			bodyLength = std::stoul(strBodyLength);
		}
		catch (const std::exception &e)
		{
			std::cerr << "can't convert content length" << e.what() << std::endl;
			clRequest.invalidRequest = true;
			return (1);
		}
		if (body.size() != bodyLength)
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		bodyHasToRead = body.substr(0, bodyLength);
		if (clRequest.hundredContinue)
			clRequest.hundredContinue = false;
		clRequest.body = bodyHasToRead;
	}
	else
	{
		if (parseChunkedBody(body, clRequest))
			return (1);	
	}
	return (0);
}


void readRequest(Client& client)
{
	std::string strClRequest = client.getClientReceived();
	clRequest&	clRequest = client.getClStructRequest();

	bool	is100Continue = false;
	bool	foundEndOfHeaders  = false;

	if (clRequest.hundredContinue)
		is100Continue = true;
	else
		resetStruct(clRequest);

    size_t headerEndPos = strClRequest.find("\r\n\r\n");
    if (headerEndPos == std::string::npos) 
	{
        std::cerr << "Headers not complete" << std::endl;
        clRequest.invalidRequest = true;
        return;
    }
	std::string headers = strClRequest.substr(0, headerEndPos + 4);
    std::string body = strClRequest.substr(headerEndPos + 4);

	size_t	pos = 0;
	size_t	i = 0;
	size_t	headersSize = 0;
	if (is100Continue)
	{
		parseBody(strClRequest, clRequest);
		return;

	}
	while ((pos = headers.find_first_of('\n')) != std::string::npos)
	{
		if (pos == 0 || (pos > 0 && headers[pos - 1] != '\r'))
		{
			clRequest.invalidRequest = true;
 			return;
		}
		else if (pos == 1)
		{
			foundEndOfHeaders = true;
			if (clRequest.hundredContinue)
			{
				if (body.size() > 2)
				{
					clRequest.invalidRequest = true;
					return;
				}
			}
			if (body.size() > 2)
			{
				parseBody(body, clRequest);
				return ;

			}
		}
		else
		{
			std::string line = (pos > 1) ? headers.substr(0, pos - 1) : "";
			if (i == 0)
			{
				if (parseRequestLine(line, clRequest) != 0 )
				{
					return;
				}
			}
			else
			{
				headersSize += line.size();
				if (headersSize > 8192)
				{
					clRequest.invalidRequest = true;
					return;
				}
				if (parseRequestHeaders(line, clRequest, client) != 0 )
					return;
			}
		}
		headers = headers.substr(pos + 1);
		++i;
	}
	if (!foundEndOfHeaders)
		clRequest.invalidRequest = true;
}


void	generateHttpResponse(Client& client, const Server& server, clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::string response;

	if (cl_request.path == "/favicon.ico")
		cl_request.path = "/";
	routeRequest(client, server, cl_request, serverBlock);
}

void	parsingRequest(Server& server, Client& client)
{
	readRequest(client);
	clRequest		cl_request = client.getClStructRequest();
	Configuration	config = server.getConfig();
	int				client_fd = client.getClientFds(0);

	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (getsockname(client_fd, (sockaddr*)&addr, &len) == -1)
		std::cerr << "Error: couldn't find sock name" << std::endl;

	ConfigBlock serverBlock = config.getServerBlock(ip_to_string(addr.sin_addr), std::to_string(ntohs(addr.sin_port)), cl_request.host);
	client.setServerBlock(serverBlock);
	
	if (cl_request.invalidRequest == true)
	{
		serveError(client, "400", serverBlock);
		return ;
	}

	generateHttpResponse(client, server, cl_request, serverBlock);
	if (client.checkCgiPtr() == true)
		return ;

	struct epoll_event event;
	event.events = EPOLLOUT;
	event.data.fd = client_fd;
	epoll_ctl(server.getEpollFd(), EPOLL_CTL_MOD, client_fd, &event);
	client.setClientState(sending_response);
	return ;
}
