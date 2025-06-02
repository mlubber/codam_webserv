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
// GET /index.html HTTP/1.1
// Host: 127.0.0.1
// User-Agent: curl/7.68.0
// Accept: text/html
// Connection: keep-alive



// POST /upload HTTP/1.1
// Host: 127.0.0.1
// Content-Type: multipart/form-data; boundary=------WebKitFormBoundary
// Content-Length: 134

// ------WebKitFormBoundary
// Content-Disposition: form-data; name="file"; filename="image.png"
// Content-Type: image/png

// (binary image data here)
// ------WebKitFormBoundary--

// std::map<int, clRequest>&	getClStructRequest() {
// 	return (_clientRequests);
// }


// clRequest&	getClStructRequest(int fd){
// 	if (_clientRequests.find(fd) != _clientRequests.end()) {
// 		// std::cout << "client fd for request found" << std::endl;
// 		return(_clientRequests.find(fd)->second);
// 	}
// 	static clRequest defaultRequest;
//     return defaultRequest;
// }

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

int	parseHeaderSingleValue(std::string &headerName ,std::string &line, clRequest &clRequest)
{
	if (isHeaderAppearTwice.find(headerName) != isHeaderAppearTwice.end())
	{
		if (isHeaderAppearTwice[headerName])
		{
			// std::cout << "header should appear once: " << headerName << std::endl;
			clRequest.invalidRequest = true;
			return (1);
		}
		isHeaderAppearTwice[headerName] = true;

	}
	
	if (headerName == "host")
	{
		//std::cout << "host header : (" << headerName << ")" << std::endl;
		clRequest.headers[headerName].push_back(line);
		// port host
		size_t pos = 0;
		pos = line.find_first_of(':');
		if (pos != std::string::npos)
		{
			clRequest.host = line.substr(0, pos);
			//std::cout << "found : (" << line.substr(0, pos) << ")" << std::endl;
			if (pos + 1 < line.size()) {
				clRequest.port = line.substr(pos + 1);
			//	std::cout << "port : (" << line.substr(pos + 1)  << ")" << std::endl;
			}
		}
		else
		{
		//	std::cout << "else not found  :"  <<std::endl;
			clRequest.host = line;
			clRequest.port = 80;
		}
		return (0);
	}
//	std::cout << "not host header : key is : (" << headerName  << ")" << std::endl;
	clRequest.headers[headerName].push_back(line);
	return (0);
}

void trim(std::string &str)
{
    // Trim leading spaces
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));

    // Trim trailing spaces
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

int	parseRequestHeaders(std::string &line, clRequest& clRequest) 
{

	size_t pos = 0;
	pos = line.find_first_of(':');
	if (pos == std::string::npos)
	{
		// std::cout << "headers here 1" << std::endl;
		clRequest.invalidRequest = true;
		return (1);
	}
	if (isspace(line[pos - 1]))
	{
		// std::cout << "headers here 2" << std::endl;
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
	// std::cout << "headerName is : (" << headerName << ")" << std::endl;
	// std::cout << "headerValue is : (" << headerValue << ")" << std::endl;

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
		// multie value 
		if (parseHeaderMultiValueComma(headerName, headerValue, clRequest))
		{
			// std::cout << "here 3" << std::endl;
			return (1);
		}
	}
	else if (headerName.compare("cookie") == 0)
	{
		// multie value cookie
	}
	else
	{
		// single value
		if (parseHeaderSingleValue(headerName, headerValue, clRequest))
		{
			// std::cout << "here 4" << std::endl;
			return (1);
		}
	}
	return (0);
}



int	parseRequestLine(std::string &line, clRequest &clRequest)
{
	//std::cout << "parse line (" << line << ")" << std::endl;

	//size_t i = 0;
	int wordCounter = 0;
	std::string word;
	size_t pos = 0;

	// max size request line 8 k.
	if (line.size() > 8192 )
	{
		clRequest.invalidRequest = true;
		//std::cout << "return" << std::endl;
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
				// std::cout << "here method " << std::endl;
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
				//std::cout << "return" << std::endl;
				return (1);
			}
			break;
		default:
			clRequest.invalidRequest = true;
			//std::cout << "return" << std::endl;
			return (1);
			break;
		}
	}
	return (0);
}

void	printclRequest(clRequest &clRequest)
{
	std::cout << "is valid : (" << clRequest.invalidRequest << ")"<< std::endl;
	std::cout << "method is : (" << clRequest.method << ")"<< std::endl;
	std::cout << "path is : (" << clRequest.path << ")"<< std::endl;

	for (std::pair<const std::string, std::vector<std::string>> &it : clRequest.headers)
		std::cout << "key is : (" << it.first << ")\nvalue is : (" << it.second.front() << ")" << std::endl;

}

// fix issue for colon for each key of headers and fix it for to works find for map 

int	parseChunkedBody(std::string &body, clRequest &clRequest)
{
	std::cout << "---chunked body---" << std::endl;
	//chunked
	// for (std::pair<std::string, std::vector<std::string>> myPair : clRequest.headers) {
	// 	std::cout << "key: (" << myPair.first << "), value: " << myPair.second.front() << std::endl; 
	// }
	if (clRequest.headers.find("transfer-encoding") == clRequest.headers.end())
	{
		// std::cout << "transfer-encoding  not found" << std::endl;
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
		// std::cout << "transfer-encoding: hasn't value chunked!" << std::endl;
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
		// first line size
		pos = line.find('\r');
		if (pos == std::string::npos)
		{
			//std::cerr << "Error: here 1" << std::endl;
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
			// end of chunked body
			//std::cout << "end of chunked body" << std::endl;
			line.clear();
			getline(request_body, line);
			if (line[0] == '\r')
				break;
		}
		// second line word
		line.clear();
		getline(request_body, line);
		pos = line.find('\r');
		if (pos == std::string::npos)
		{
			//std::cerr << "Error: here 2" << std::endl;
			clRequest.invalidRequest = true;
			return (1);
		}
		line = line.substr(0, pos);
		if (line.size() < static_cast<size_t>(intSize))
		{
			//std::cerr << "Error: here 3 line.size() is : " << line.size() << "intSize is : " << intSize << std::endl;
			clRequest.invalidRequest = true;
			return (1);
		}
		line = line.substr(0, intSize);
		chunkText = chunkText + line;
	}
	std::cout << "chunked body : " << chunkText << std::endl;
	clRequest.body = chunkText;
	return (0);
}

int	parseBody(std::string &body, clRequest &clRequest)
{
	std::cout << "---parse body---" << std::endl;
	if (clRequest.method != "POST") 
	{
		std::cout << "---method not post--- nginx ignore the body" << std::endl;
		return (0);
	}
	std::cout << "parse body" << std::endl;
	std::string strBodyLength, bodyHasToRead;
	size_t	bodyLength = 0;
	// for (std::pair<std::string, std::vector<std::string>> myPair : clRequest.headers) {
	// 	std::cout << "key: (" << myPair.first << "), value: " << myPair.second.front() << std::endl; 
	// }
	if (clRequest.headers.find("content-length") != clRequest.headers.end())
	{
		// std::cout << "---content-length---" << std::endl;
		// if (clRequest.headers.find("transfer-encoding") != clRequest.headers.end()) {
		// 	std::cout << "---transfer-encoding---" << std::endl;
		// 	clRequest.invalidRequest = true;
		// 	return 1;
		// }

		// std::cout << "inside " << std::endl;
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
		std::cout << "actual body size:  (" << body.size() << ", content length is: (" << bodyLength << ")." << std::endl;
		if (body.size() != bodyLength)
		{
			clRequest.invalidRequest = true;
			return (1);
		}
		bodyHasToRead = body.substr(0, bodyLength);
		if (clRequest.hundredContinue)
			clRequest.hundredContinue = false;
		// std::cout << "body : " << bodyHasToRead << std::endl;
		clRequest.body = bodyHasToRead;
	} // chunk or not found content length and chunk
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
	// int			clientFD = client.getClientFds(0);
	clRequest&	clRequest = client.getClStructRequest();

	// std::cout << "read  request" << std::endl;
	bool	is100Continue = false;
	bool	foundEndOfHeaders  = false;

	if (clRequest.hundredContinue)
	{
		is100Continue = true;
		//std::cout << "inside if" << std::endl;
	}
	else
	{
		resetStruct(clRequest);
		//std::cout << "inside else" << std::endl;
	}

    size_t headerEndPos = strClRequest.find("\r\n\r\n");
    if (headerEndPos == std::string::npos) 
	{
        std::cerr << "Headers not complete" << std::endl;
        clRequest.invalidRequest = true;
        return;
    }
	std::string headers = strClRequest.substr(0, headerEndPos + 4);
    std::string body = strClRequest.substr(headerEndPos + 4);

	// std::cout << "headers extracted: \n" << headers << std::endl;

	size_t	pos = 0;
	size_t	i = 0;
	size_t	headersSize = 0;
	if (is100Continue)
	{
		std::cout << "just parse body" << std::endl;
		parseBody(strClRequest, clRequest);
		return;

	}
	while ((pos = headers.find_first_of('\n')) != std::string::npos)
	{
		// std::cout << "reading headers request..." << std::endl;
		if (pos == 0 || (pos > 0 && headers[pos - 1] != '\r'))
		{
			clRequest.invalidRequest = true;
			std::cout << "return 1" << std::endl;
 			return;
		}
		else if (pos == 1)
		{
			// end of headers
			foundEndOfHeaders = true;
			// std::cout << "end of headers" << std::endl;
			if (clRequest.hundredContinue)
			{
				//std::cout << "strClRequest.size() here is : " << strClRequest.size() << std::endl;
				if (body.size() > 2)
				{
					// it shouldn't has any 
					clRequest.invalidRequest = true;
					// std::cout << "return 6" << std::endl;
					return;
				}
			}
			if (body.size() > 2)
			{
				// std::string body = strClRequest.substr(pos + 1);
				// std::cout << "read request printing body: (" << body << ")" << std::endl;
				parseBody(body, clRequest);
					//std::cout << "return 2" << std::endl;
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
					// std::cout << "return 3" << std::endl;
					return;
				}
			}
			else
			{
				headersSize += line.size();
				//std::cout << "headersSize is : " << headersSize << std::endl;
				// total size for all headers can't be more than 8 kilobytes
				if (headersSize > 8192)
				{
					clRequest.invalidRequest = true;
					// std::cout << "return 4" << std::endl;
					return;
				}
				if (parseRequestHeaders(line, clRequest) != 0 )
				{
					// std::cout << "return 5" << std::endl;
					return;
				}
			}
		}
		headers = headers.substr(pos + 1);
		++i;
	}
	// std::cout << "printing  request\n\n\n" << std::endl;
	// printclRequest(clRequest);
	
	// std::cout << "end read  request" << std::endl;
	if (!foundEndOfHeaders)
		clRequest.invalidRequest = true;
	
}


void	generateHttpResponse(Client& client, const Server& server, clRequest& cl_request, const ConfigBlock& serverBlock)
{
	std::string response;

	// if (parsedRequest.path == "/favicon.ico")
	// 	parsedRequest.path = "/";
	// if (!parsedRequest.path.empty() && *parsedRequest.path.rbegin() != '/')
	// 	parsedRequest.path.append("/");
	if (cl_request.path == "/favicon.ico")
		cl_request.path = "/";
	// if (!cl_request.path.empty() && *cl_request.path.rbegin() != '/')
	// 	cl_request.path.append("/");

	// std::cout << "generate client request path: " << cl_request.path << std::endl;

	routeRequest(client, server, cl_request, serverBlock);
}

void	parsingRequest(Server& server, Client& client)
{
	// std::cout << "Errno at start of parsingRequest: " << errno << ", str: " << strerror(errno) << std::endl;

	std::cout << "\n\n---Client received: ---\n" << client.getClientReceived() << "\n--- END OF RECEIVED ---" << std::endl;

	readRequest(client);
	clRequest		cl_request = client.getClStructRequest();
	Configuration	config = server.getConfig();
	int				client_fd = client.getClientFds(0);

	ConfigBlock serverBlock = config.getServerBlock(cl_request.host, cl_request.port);


	if (cl_request.invalidRequest == true)
	{
		serveError(client, "400", serverBlock);
		return ;
	}

	generateHttpResponse(client, server, cl_request, serverBlock);
	if (client.checkCgiPtr() == true)
	{
		std::cout << "cgi ptr not nullptr" << std::endl;
		return ;
	}

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client_fd;
	epoll_ctl(server.getEpollFd(), EPOLL_CTL_MOD, client_fd, &event);
	client.setClientState(sending_response);
	std::cout << "Finished parsing and set client state to sending_response" << std::endl;
	return ;
}
