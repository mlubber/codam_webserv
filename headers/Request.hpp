#pragma once
#include <iostream>
#include <map>
#include <vector>


// class Serve;

enum state {
	none,
	cgi_Read,
	cgi_write,
	request_sent,
	waiting_reponse
};

struct clRequest {
	bool	invalidRequest = false;
	bool	methodNotAllowd = false;
	bool	hundredContinue = false;
    std::string method;
    std::string path;
	std::string	queryStr;
    std::map<std::string, std::vector<std::string>> headers;
    std::string body;
	std::string port;
	std::string host;

};


class Client
{
	private:
		std::map<int, clRequest>	_clientRequests;
		// int							_state;
	public:
		Client();
		void 		readRequest(std::string strclRequest, int clientFD);
		void		resetStruct(int clientFD);
		clRequest&	getClStructRequest(int fd);
		~Client();
};






// class Clients {
// 	private:
// 		int					_state; -> waiting_for_read
// 		clRequest*			_request;
// 		std::vector<int>	_fds; { 4, 5, 8 }
// 		std::string			_inc_buffer;
// 		std::string			_out_buffer;

// }; 


// EPOLL ready -> 5;
// std::string << parsing << input << input << input  << waiting               << input << eof

// class Server {
// 	private:
// 		std::vector<Client> Clients;
// };