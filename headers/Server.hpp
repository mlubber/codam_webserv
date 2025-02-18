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

class Server
{
	public:

		Server();
		Server(const Server& other);
		~Server();

		Server&	operator=(const Server& other);

		bool			initialize();
		void			run();
		bool			parseRequest(const char* request, HttpRequest& httprequest);
		void			sendBadRequest(int client_fd);
		std::string		getExtType(const std::string& filename);
		std::string		serveStaticFile(const std::string& filePath);
		std::string		handlePostRequest(const HttpRequest &request);
		std::string		routeRequest(const HttpRequest &request);
		void			sendHtmlResponse(int client_fd, char* buffer, HttpRequest& parsedRequest);
		void			printRequest(HttpRequest& httprequest);

	private:

		int					_server_fd;
		struct sockaddr_in	_address;
		socklen_t			_addr_len;
};

void	setNonBlocking(int socket);

#endif