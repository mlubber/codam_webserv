#pragma once

#include "Server.hpp"

enum state {
	none,
	cgi_Read,
	cgi_write,
	request_sent,
	waiting_reponse
};

struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	bool								cgi;
	std::string							cgiBody;
};

class Client {
	private:
		int			_socket_fd;
		int			_state;
		HttpRequest	_request;

	public:
		
};