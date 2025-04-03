#pragma once

#include "Server.hpp"

enum state {
	reading_request,	// In loop of reading the request from the socket
	parsing_request,	// Received the full request from the socket and now parsing
	start_response,		// Readying everything and start sending response
	sending_response,	// In loop of sending response to the socket
	cgi_read,			// In loop of reading data from the cgi_pipe
	cgi_write,			// In loop of writing data to the cgi_pipe
	none,				// All events handled
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
		int							_state;				// state of the client
		std::vector<int>			_fds;				// array of fds (socket en pipes)
		std::vector<HttpRequest>	_request;			// array of parsed request structs
		bool						_multi_request;		// 'true' if found that we've received multiple request from reading
		std::string					_received;			// data received of socket
		std::string					_temp_rec_buffer;	// data that was read but not part of 1st request
		std::string					_response;			// the response that is going to be sent to the client

	public:
		Client(int socket_fd);
		~Client();

		void	handleEvent(int fd);

		int 	getClientFds(int mode);
		
};
