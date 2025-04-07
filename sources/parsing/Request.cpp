#include "../headers/Request.hpp"
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

Client::Client() {}

Client::~Client() {}

// std::map<int, clRequest>&	Client::getClStructRequest() {
// 	return (_clientRequests);
// }


clRequest&	Client::getClStructRequest(int fd){
	if (_clientRequests.find(fd) != _clientRequests.end()) {
		// std::cout << "client fd for request found" << std::endl;
		return(_clientRequests.find(fd)->second);
	}
	static clRequest defaultRequest;
    return defaultRequest;
}

void	Client::resetStruct(int clientFD) {
	if (_clientRequests.find(clientFD) != _clientRequests.end()) {
		auto &requestStruct = _clientRequests[clientFD];
		requestStruct.invalidRequest = false;
		requestStruct.body.clear();
		requestStruct.path.clear();
		requestStruct.method.clear();
		requestStruct.headers.clear();
		requestStruct.host.clear();
		requestStruct.port.clear();
	}
	// Reset the isHeaderAppearTwice map for the new request
	for (auto &header : isHeaderAppearTwice) {
		header.second = false;
	}
}

int	parseHeaderMultiValueComma(std::string &headerName ,std::string &line, clRequest &requestStruct) {
	if (isHeaderAppearTwice.find(headerName) != isHeaderAppearTwice.end()) {
		if (isHeaderAppearTwice[headerName]){
			// header should appears one 
			requestStruct.invalidRequest = true;
			return 1;
		}
		isHeaderAppearTwice[headerName] = true;

	}
	std::vector<std::string> temp;
	size_t pos = 0;
	std::string subValue;
	while ( (pos = line.find_first_of(',')) != std::string::npos) {
		subValue = line.substr(0, pos);
		temp.push_back(subValue);
		line = line.substr(pos + 1);
	}
	temp.push_back(line);
	requestStruct.headers[headerName] = temp;
	return 0;
}

int	parseHeaderSingleValue(std::string &headerName ,std::string &line, clRequest &requestStruct) {
	if (isHeaderAppearTwice.find(headerName) != isHeaderAppearTwice.end()) {
		if (isHeaderAppearTwice[headerName]){
			// header should appears one 

			std::cout << "header should appears one : " << headerName << std::endl;
			requestStruct.invalidRequest = true;
			return 1;
		}
		isHeaderAppearTwice[headerName] = true;

	}
	
	if (headerName == "host") {
		//std::cout << "host header : (" << headerName << ")" << std::endl;
		requestStruct.headers[headerName].push_back(line);
		// port host
		size_t pos = 0;
		pos = line.find_first_of(':');
		if (pos != std::string::npos) {
			
			requestStruct.host = line.substr(0, pos);
			//std::cout << "found : (" << line.substr(0, pos) << ")" << std::endl;
			if (pos + 1 < line.size()) {
				requestStruct.port = line.substr(pos + 1);
			//	std::cout << "port : (" << line.substr(pos + 1)  << ")" << std::endl;
			}
		}else {
		//	std::cout << "else not found  :"  <<std::endl;
			requestStruct.host = line;
			requestStruct.port = 80;
		}
		return (0);

	} 
//	std::cout << "not host header : key is : (" << headerName  << ")" << std::endl;
	requestStruct.headers[headerName].push_back(line);
	return 0;
}

void trim(std::string &str) {
    // Trim leading spaces
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // Trim trailing spaces
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}



int	parseRequestHeaders(std::string &line, clRequest& requestStruct) {

	size_t pos = 0;
	pos = line.find_first_of(':');
	if (pos == std::string::npos) {
		std::cout << "headers here 1" << std::endl;
		requestStruct.invalidRequest = true;
		return 1;
	}
	if (isspace(line[pos - 1])) {
		std::cout << "headers here 2" << std::endl;
		requestStruct.invalidRequest = true;
		return 1;
	}
	std::string headerName = line.substr(0, pos);
	std::string headerValue = line.substr(pos + 1);
	trim(headerValue);
	if (headerValue.empty()) {
		requestStruct.invalidRequest = true;
		return 1;
	}
	
	
	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
	// std::cout << "headerName is : (" << headerName << ")" << std::endl;
	// std::cout << "headerValue is : (" << headerValue << ")" << std::endl;

	if (headerName ==  "expect" && headerValue == "100-continue")
		requestStruct.hundredContinue = true;
	if (std::any_of(headerName.begin(), headerName.end(), [](unsigned char c) {return std::isspace(c);})) {
		std::cerr << "header name (key) can't have any space! => " << headerName << std::endl;
		requestStruct.invalidRequest = true;
		return 1;
	}

	if (std::find(MultieValueByComma.begin(), MultieValueByComma.end(), headerName) != MultieValueByComma.end() ) {
		// multie value 
		if (parseHeaderMultiValueComma(headerName, headerValue, requestStruct)) {
			std::cout << "here 3" << std::endl;
			return 1;
		}
	}
	else if (headerName.compare("cookie") == 0) {
		// multie value cookie
	} else {
		// single value
		if (parseHeaderSingleValue(headerName, headerValue, requestStruct) ) {
			std::cout << "here 4" << std::endl;
			return 1;
		}
	}



	return 0;
}



int	parseRequestLine(std::string &line, clRequest &requestStruct) {
	//std::cout << "parse line (" << line << ")" << std::endl;

	//size_t i = 0;
	int wordCounter = 0;
	std::string word;
	size_t pos = 0;

	// max size request line 8 k.
	if (line.size() > 8192 ) {
		requestStruct.invalidRequest = true;
		//std::cout << "return" << std::endl;
		return 1;
	}
	while(wordCounter < 3){
		if (wordCounter < 2) {
			pos = line.find_first_of(' ');
			if (pos == std::string::npos){
				requestStruct.invalidRequest = true;
				return 1;
			}
			word = line.substr(0, pos);
			line = line.substr(pos + 1);
		} else if (wordCounter == 2) {
			word = line;
		}
		++wordCounter;
		
		std::string query;
		switch (wordCounter)
		{
		case 1:
			requestStruct.method = word;
			if (word != "POST" && word != "GET" && word != "DELETE") {
				std::cout << "here method " << std::endl;
				requestStruct.methodNotAllowd = true;
				return 1;
			}
			break;
		case 2:
			pos = word.find('?');
			if (pos == std::string::npos)
				requestStruct.path = word;
			else {
				query = word.substr(pos, word.size() - pos);
				word = word.substr(0, pos);
				requestStruct.path = word;
				requestStruct.queryStr = query;
			}
			break;
		case 3:
			if (word.compare("HTTP/1.1") != 0) {
				requestStruct.invalidRequest = true;
				//std::cout << "return" << std::endl;
				return 1;
			}
			break;
		default:
			requestStruct.invalidRequest = true;
			//std::cout << "return" << std::endl;
			return 1;
			break;
		}
	}
	return 0;
}














// int	parseRequestLine(std::string &line, clRequest &requestStruct) {
// 	//std::cout << "parse line (" << line << ")" << std::endl;

// 	size_t i = 0;
// 	size_t j = 0;
// 	std::string word;

// 	// max size request line 8 k.
// 	if (line.size() > 8192 ) {
// 		requestStruct.invalidRequest = true;
// 		//std::cout << "return" << std::endl;
// 		return 1;
// 	}
// 	int wordCounter = 0;
// 	while (i < line.size()) {
// 		j = i;
// 		word.clear();
// 		while (j < line.size() && !isspace(line[j]))
// 		{
// 			word += line[j];
// 				++j;
// 		}
// 		++wordCounter;
// 		size_t pos = 0;
// 		std::string query;
// 		switch (wordCounter)
// 		{
// 		case 1:
// 			requestStruct.method = word;
// 			if (word != "POST" && word != "GET" && word != "DELETE") {
// 				requestStruct.invalidRequest = true;
// 				return 1;
// 			}
// 			break;
// 		case 2:
// 			pos = word.find('?');
// 			if (pos == std::string::npos)
// 				requestStruct.path = word;
// 			else {
// 				query = word.substr(pos, word.size() - pos);
// 				word = word.substr(0, pos);
// 				requestStruct.path = word;
// 				requestStruct.queryStr = query;
// 			}
// 			break;
// 		case 3:
// 			if (word.compare("HTTP/1.1") != 0) {
// 				requestStruct.invalidRequest = true;
// 				//std::cout << "return" << std::endl;
// 				return 1;
// 			}
// 			break;
// 		default:
// 			requestStruct.invalidRequest = true;
// 			//std::cout << "return" << std::endl;
// 			return 1;
// 			break;
// 		}

// 		//std::cout << "word is : (" << word << ")" << std::endl;
// 		if (j < line.size()) {
// 			if ((line[j] != ' ') || (isspace(line[j + 1]))) {
// 				requestStruct.invalidRequest = true;
// 				//std::cout << "return" << std::endl;
// 				return 1;
// 			}
// 		}
// 		if (wordCounter == 3 && j != line.size()) {
// 			requestStruct.invalidRequest = true;
// 			//std::cout << "return" << std::endl;
// 			return 1;
// 		}
// 		i = j;
// 		if (i + 1 <= line.size())
// 			++i;
// 	}
// 	return 0;
// }

void	printRequestStruct(clRequest &requestStruct) {
	std::cout << "is valid : (" << requestStruct.invalidRequest << ")"<< std::endl;
	std::cout << "method is : (" << requestStruct.method << ")"<< std::endl;
	std::cout << "path is : (" << requestStruct.path << ")"<< std::endl;

	for (std::pair<const std::string, std::vector<std::string>> &it : requestStruct.headers) {
		std::cout << "key is : (" << it.first << ")\nvalue is : (" << it.second.front() << ")" << std::endl;
	}

}

// fix issue for colon for each key of headers and fix it for to works find for map 

int	parseChunkedBody(std::string &body, clRequest &requestStruct) {
	std::cout << "---chunked body---" << std::endl;
	//chunked
	// for (std::pair<std::string, std::vector<std::string>> myPair : requestStruct.headers) {
	// 	std::cout << "key: (" << myPair.first << "), value: " << myPair.second.front() << std::endl; 
	// }
	if (requestStruct.headers.find("transfer-encoding") == requestStruct.headers.end()) {
		std::cout << "transfer-encoding  not found" << std::endl;
		requestStruct.invalidRequest = true;
		return 1;
	}
	bool	isChunked = false;
	for(size_t i = 0; i < requestStruct.headers["transfer-encoding"].size(); ++i) {
		if (requestStruct.headers["transfer-encoding"][i] == "chunked") 
			isChunked = true;	
	}
	if (!isChunked) {
		std::cout << "transfer-encoding: hasn't value chunked!" << std::endl;
		requestStruct.invalidRequest = true;
		return 1;
	}
	std::string line;
	std::string	hexSize;
	size_t		pos = 0;
	int			intSize = 0;
	std::istringstream	request_body(body);
	std::string	chunkText;
	while (getline(request_body, line)) {
		// first line size
		pos = line.find('\r');
		if (pos == std::string::npos) {
			//std::cerr << "Error: here 1" << std::endl;
			requestStruct.invalidRequest = true;
			return 1;
		}
		hexSize = line.substr(0, pos);
		try{
			intSize = std::stoi(hexSize,0,16);
		}catch (const std::exception& e) {
			std::cerr << "Error:" << e.what() << std::endl;
			requestStruct.invalidRequest = true;
			return (1);
		}
		if (intSize == 0 && line.size() >= 1 && line[1] == '\r')  {
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
		if (pos == std::string::npos) {
			//std::cerr << "Error: here 2" << std::endl;
			requestStruct.invalidRequest = true;
			return 1;
		}
		line = line.substr(0, pos);
		if (line.size() < static_cast<size_t>(intSize)) {
			//std::cerr << "Error: here 3 line.size() is : " << line.size() << "intSize is : " << intSize << std::endl;
			requestStruct.invalidRequest = true;
			return (1);
		}
		line = line.substr(0, intSize);
		chunkText = chunkText + line;
	}
	std::cout << "chunked body : " << chunkText << std::endl;
	return 0;
}

int	parseBody(std::string &body, clRequest &requestStruct) {
	std::cout << "---parse body---" << std::endl;
	if (requestStruct.method != "POST") {
		std::cout << "---method not post--- nginx ignore the body" << std::endl;
		return 0;
	}
	std::cout << "parse body" << std::endl;
	std::string strBodyLength, bodyHasToRead;
	size_t	bodyLength = 0;
	// for (std::pair<std::string, std::vector<std::string>> myPair : requestStruct.headers) {
	// 	std::cout << "key: (" << myPair.first << "), value: " << myPair.second.front() << std::endl; 
	// }
	if (requestStruct.headers.find("content-length") != requestStruct.headers.end()) {
		std::cout << "---content-length---" << std::endl;
		// if (requestStruct.headers.find("transfer-encoding") != requestStruct.headers.end()) {
		// 	std::cout << "---transfer-encoding---" << std::endl;
		// 	requestStruct.invalidRequest = true;
		// 	return 1;
		// }

		std::cout << "inside " << std::endl;
		strBodyLength = requestStruct.headers["content-length"].front();
		try {
			bodyLength = std::stoul(strBodyLength);
		} catch (const std::exception &e) {
			std::cerr << "can't convert content length" << e.what() << std::endl;
			requestStruct.invalidRequest = true;
			return 1;
		}
		std::cout << "actual body size:  (" << body.size() << ", content length is: (" << bodyLength << ")." << std::endl;
		if (body.size() != bodyLength) {
			requestStruct.invalidRequest = true;
			return 1;
		}
		bodyHasToRead = body.substr(0, bodyLength);
		if (requestStruct.hundredContinue)
		requestStruct.hundredContinue = false;
		//std::cout << "body : " << bodyHasToRead << std::endl;
	} // chunk or not found content length and chunk
	else {
		if (parseChunkedBody(body, requestStruct))
			return 1;	
	}
	return 0;
}





void Client::readRequest(std::string strClRequest, int clientFD) {
	
	std::cout << "read  request" << std::endl;
	bool	is100Continue = false;
	bool	foundEndOfHeaders  = false;
	clRequest &requestStruct = _clientRequests[clientFD];

	if (_clientRequests.find(clientFD) != _clientRequests.end()) {
		if (_clientRequests[clientFD].hundredContinue) {
			is100Continue = true;
			//std::cout << "inside if" << std::endl;
		} else {
			resetStruct(clientFD);
			//std::cout << "inside else" << std::endl;
		}
    }

	size_t pos = 0;
	size_t	i = 0;
	size_t	headersSize = 0;
	if (is100Continue)  {
		//std::cout << "just parse body" << std::endl;
		parseBody(strClRequest, requestStruct);
		return;

	}
	while ((pos = strClRequest.find_first_of('\n')) != std::string::npos)
	{

		if (pos == 0 || (pos > 0 && strClRequest[pos - 1] != '\r')) {
			requestStruct.invalidRequest = true;
			std::cout << "return 1" << std::endl;
 			return;
		}
		else if (pos == 1) {
			// end of headers
			foundEndOfHeaders = true;
			std::cout << "end of headers" << std::endl;
			if (requestStruct.hundredContinue) {
				//std::cout << "strClRequest.size() here is : " << strClRequest.size() << std::endl;
				if (strClRequest.size() > 2) {
					// it shouldn't has any 
					requestStruct.invalidRequest = true;
					std::cout << "return 6" << std::endl;
					return;
				}
			}
			if (strClRequest.size() > 2) {
				std::string body = strClRequest.substr(pos + 1);
				//std::cout << "read request printing body: (" << body << ")" << std::endl;
				parseBody(body, requestStruct);
					//std::cout << "return 2" << std::endl;
				return ;

			}
		} else {
			std::string line = (pos > 1) ? strClRequest.substr(0, pos - 1) : "";
			if (i == 0) {
				if (parseRequestLine(line, requestStruct) != 0 ) {
					std::cout << "return 3" << std::endl;
					return;
				}
			} else {
				headersSize += line.size();
				//std::cout << "headersSize is : " << headersSize << std::endl;
				// total size for all headers can't be more than 8 kilobytes
				if (headersSize > 8192) {
					requestStruct.invalidRequest = true;
					std::cout << "return 4" << std::endl;
					return;
				}
				if (parseRequestHeaders(line, requestStruct) != 0 ) {
					std::cout << "return 5" << std::endl;
					return;
				}
			}
		}
		strClRequest = strClRequest.substr(pos + 1);
		++i;
	}
	// std::cout << "printing  request\n\n\n" << std::endl;
	// printRequestStruct(requestStruct);
	
	std::cout << "end read  request" << std::endl;
	if (!foundEndOfHeaders)
		requestStruct.invalidRequest = true;
	

}
