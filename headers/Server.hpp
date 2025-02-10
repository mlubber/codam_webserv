#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <arpa/inet.h>
# include <cstring>
# include <unistd.h>

# define PORT 8080
# define BUFFER_SIZE 1024

class Server
{
	public:

		Server();
		Server(const Server& other);
		~Server();

		Server&	operator=(const Server& other);

		bool	initialize();
		void	run();

	private:

		int					_server_fd;
		int					_client_fd;
		struct sockaddr_in	_address;
		socklen_t			_addr_len;
};

#endif