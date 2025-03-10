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
# include <dirent.h>
# include <stdlib.h>

// # include "cgi.hpp"

# define PORT 8080
# define BUFFER_SIZE 1024
# define STATIC_DIR "./www"

struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
};

#include "cgi.hpp"

class Server
{
	public:

		Server(char** envp);
		Server(const Server& other);
		~Server();

		Server&	operator=(const Server& other);

		bool			initialize();
		void			run();
		int				connectClients(fd_set &read_fds, fd_set &tmp_fds, int max_fd);
		void			handleData(fd_set &read_fds, fd_set &tmp_fds, int max_fd, char *buffer);
		void			setNonBlocking(int socket);
		void			sendHtmlResponse(int client_fd, char* buffer, HttpRequest& parsedRequest);

	private:

		int					_server_fd;
		struct sockaddr_in	_address;
		socklen_t			_addr_len;
		char**				_envp;

		const std::string	_root; // temp till Abbas adds his config file code
};

bool		parseRequest(const char* request, HttpRequest& httprequest);
void		sendBadRequest(int client_fd);
std::string	getExtType(const std::string& filename);
std::string	serveStaticFile(const std::string& filePath);
std::string	handlePostRequest(const HttpRequest &request);
std::string	routeRequest(const HttpRequest &request);
void		printRequest(HttpRequest& httprequest);
std::string dechunk(std::istream& stream, const std::string& input);

#endif