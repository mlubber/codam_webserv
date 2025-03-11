#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <arpa/inet.h>
# include <cstring>
# include <unistd.h>
# include <fcntl.h>
# include <map>
# include <sstream>
# include <fstream>
# include <sys/stat.h>
# include <sys/epoll.h>
# include <dirent.h>

# define PORT 8080
# define BUFFER_SIZE 1024
# define STATIC_DIR "./www"
# define MAX_EVENTS 16
# define ER400 "HTTP/1.1 400 Bad Request\r\nContent-Length: 143\r\n\r\n<html><head><title>400 Bad Request</title></head><body><center><h1>400 Bad Request</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER404 "HTTP/1.1 404 Not Found\r\nContent-Length: 139\r\n\r\n<html><head><title>404 Not Found</title></head><body><center><h1>404 Not Found</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER405 "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 157\r\n\r\n<html><head><title>405 Method Not Allowed</title></head><body><center><h1>405 Method Not Allowed</h1></center><hr><center>webserv</center></hr></body></html>"

struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
};

class Server
{
	public:

		Server();
		Server(const Server& other);
		~Server();

		Server&	operator=(const Server& other);

		bool			initialize();
		void			run();
		void			connectClient(int epoll_fd);
		void			handleRead(int epoll_fd, int client_fd);
		void			handleWrite(int epoll_fd, int client_fd);
		void			setNonBlocking(int socket);

	private:

		int							_server_fd;
		struct sockaddr_in			_address;
		socklen_t					_addr_len;
		std::map<int, std::string>	_client_buffers;
		std::map<int, std::string>	_responses;

};

bool		parseRequest(const char* request, HttpRequest& httprequest);
std::string	generateHttpResponse(HttpRequest& parsedRequest);
std::string	getExtType(const std::string& filename);
std::string	serveStaticFile(const std::string& filePath);
std::string	handlePostRequest(const HttpRequest &request);
std::string	routeRequest(const HttpRequest &request);
void		printRequest(HttpRequest& httprequest);
std::string dechunk(std::istream& stream, const std::string& input);


#endif