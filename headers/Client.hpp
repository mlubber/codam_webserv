#pragma once

#include "headers.hpp"
#include "cgi.hpp"

# define READSOCKET		0
# define READCGI		1
# define WRITESOCKET	2
# define WRITECGI		3

enum state 
{
	reading_request,	// In loop of reading the request from the socket
	parsing_request,	// Received the full request from the socket and now parsing
	start_response,		// Readying everything and start sending response
	sending_response,	// In loop of sending response to the socket
	cgi_read,			// In loop of reading data from the cgi_pipe
	cgi_write,			// In loop of writing data to the cgi_pipe
};

struct clRequest 
{
	bool		invalidRequest = false;
	bool		methodNotAllowd = false;
	bool		hundredContinue = false;
    std::string method;
    std::string path;
	std::string	queryStr;
    std::map<std::string, std::vector<std::string>> headers;
    std::string body;
	std::string port;
	std::string host;
	bool		cgi;
	std::string	cgiBody;
};


class Client 
{
	private:

		int							_state;				// state of the client
		std::vector<int>			_fds;				// array of fds (socket en pipes)
		clRequest					_request;			// array of parsed request structs
		std::string					_received;			// data received of socket
		std::string					_response;			// the response that is going to be sent to the client
		size_t						_response_size;		// size of response to compare with _bytes_sent to client
		size_t						_bytes_sent;		// Amount of bytes_sent to compare with _response_size
		std::unique_ptr<t_cgiData>	_cgi;				// unique ptr to cgi struct that handles data for cgi requests

	public:

		Client(int socket_fd);
		~Client();

		int		handleEvent(Server& server);

		void	readCGI();			// read from cgi	- uses read
		void	writeCGI();			// write to CGI		- uses write

		int 			getClientFds(int mode);
		int				getClientState();
		std::string&	getClientReceived();
		std::string&	getClientResponse();
		t_cgiData&		getCgiStruct();
		clRequest&		getClStructRequest();
		bool			checkCgiPtr();
		
		void	setReceivedData(std::string& data);
		void	setResponseData(std::string data);
		void	setClientState(int state);
		void	setCgiStruct(std::unique_ptr<t_cgiData> cgi);

		void	clearData(int i);
		void	updateBytesSent(size_t bytes_sent);
	};
