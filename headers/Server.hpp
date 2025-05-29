#ifndef SERVER_HPP
# define SERVER_HPP

# include "headers.hpp"
# include "Configuration.hpp"

# define PORT 8080
# define SOCKET_BUFFER 8192 // 8kb
# define MAX_BODY_SIZE 1048567 // MAX 1MB
# define STATIC_DIR "./www"
# define MAX_EVENTS 16
# define ER100 "HTTP/1.1 100 Continue"
# define ER400 "HTTP/1.1 400 Bad Request\r\nContent-Length: 143\r\n\r\n<html><head><title>400 Bad Request</title></head><body><center><h1>400 Bad Request</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER403 "HTTP/1.1 403 Forbidden\r\nContent-Length: 139\r\n\r\n<html><head><title>403 Forbidden</title></head><body><center><h1>403 Forbidden</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER404 "HTTP/1.1 404 Not Found\r\nContent-Length: 139\r\n\r\n<html><head><title>404 Not Found</title></head><body><center><h1>404 Not Found</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER405 "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 157\r\n\r\n<html><head><title>405 Method Not Allowed</title></head><body><center><h1>405 Method Not Allowed</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER413 "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 155\r\n\r\n<html><head><title>413 Payload Too Large</title></head><body><center><h1>413 Payload Too Large</h1></center><hr><center>webserv</center></hr></body></html>"
# define ER500 "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 157\r\n\r\n<html><head><title>500 Internal Server Error</title></head><body><center><h1>500 Internal Server</h1></center><hr><center>webserv</center></hr></body></html>"

class Client;
struct s_clRequest;

class Server
{

	private:

		std::vector<int>			_server_fds;
		int							_server_fds_amount;
		int							_epoll_fd;
		struct sockaddr_in			_address;
		socklen_t					_addr_len;

		std::vector<Client*>		_clients;
		int							_client_count;

		Configuration				_config;
		const std::string			_name; // for cgi environment var server name - Now temp, but needs to come from config
		const std::string			_port; // for cgi environment var port - Now temp, but needs to come from config
		const std::string			_root; // temp till Abbas adds his config file code

	public:

		Server(const Configuration& config);
		Server(const Server& other);
		~Server();

		Server&	operator=(const Server& other);

		bool	initialize(const std::vector<std::pair<std::string, std::vector<int> > >& server_configs);
		void	run();
		
		void	connectClient(int epoll_fd, int server_fd);
		void	removeClient(Client* client, int index);	
		
		int 	recvFromSocket(Client& client);
		void	sendToSocket(Client& client);	
		
		void	handleReceivedSignal();

		void	close_webserv();


		const std::string		getServerInfo(int i) const;
		int						getEpollFd() const;
		const Configuration&	getConfig() const;

};

void	routeRequest(Client& client, const Server& server, clRequest& cl_request, const ConfigBlock& serverBlock);
void	serveError(Client& client, std::string error_code, const ConfigBlock& serverBlock);

#endif
