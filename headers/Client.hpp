#pragma once

#include "headers.hpp"
#include "cgi.hpp"


enum state 
{
	reading_request,	// In loop of reading the request from the socket
	parsing_request,	// Received the full request from the socket and now parsing
	cgi_write,			// In loop of writing data to the cgi_pipe
	cgi_read,			// In loop of reading data from the cgi_pipe
	sending_response,	// In loop of sending response to the socket
	sending_error		// Only used when sending data to client fails and we need to remove the client
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
};


class Client 
{
	private:

		int							_state;				// state of the client
		std::vector<int>			_fds;				// array of fds (socket en pipes)
		clRequest					_request;			// array of parsed request structs
		std::unique_ptr<t_cgiData>	_cgi;				// unique ptr to cgi struct that handles data for cgi requests
		std::string					_received;			// data received of socket
		std::string					_response;			// the response that is going to be sent to the client
		size_t						_response_size;		// size of response to compare with _bytes_sent to client
		size_t						_bytes_sent;		// Amount of bytes_sent to compare with _response_size
		bool						_close_client;		// Check if we need to close connection to client after sending response
		ConfigBlock					_server_block;		// Server block the Client is connected to
		long						_last_request;		// timestamp of last request

		public:

		Client(int socket_fd, const ConfigBlock& serverBlock);
		~Client();

		void				handleEvent(Server& server);

		int 				getClientFds(int index)	const;
		int					getClientState() 		const;
		const std::string&	getClientReceived()		const;
		const std::string&	getClientResponse()		const;
		t_cgiData&			getCgiStruct()			const;
		clRequest&			getClStructRequest();
		bool				checkCgiPtr()			const;
		bool				getCloseClientState()	const;
		const ConfigBlock&	getServerBlock()		const;
		long				getLastRequest()		const;
		
		void	setReceivedData(const char* data, ssize_t bytes_received);
		void	setResponseData(std::string data);
		void	setClientState(int state);
		void	setCgiStruct(std::unique_ptr<t_cgiData> cgi);
		void	setCloseClientState(bool state);
		void	setLastRequest(long secondsSinceEpoch);

		void	addFd(int fd);
		void	resetFds(int fd);

		void	clearData(int epollFd);
		void	updateBytesSent(size_t bytes_sent);
	};
