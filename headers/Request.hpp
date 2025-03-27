#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>

class Serve;



struct clRequest {
	bool	invalidRequest = false;
	bool	hundredContinue = false;
    std::string method;
    std::string path;
	std::string	queryStr;
    std::map<std::string, std::vector<std::string>> headers;
    std::string body;
	std::string port;
	std::string host;
};


class Request
{
private:
	std::map<int, clRequest> _clientRequests;
public:
	Request();
	void readRequest(std::string strClRequest, int clientFD);
	void	resetStruct(int clientFD);
	clRequest&	getClStructRequest(int fd);
	~Request();
};