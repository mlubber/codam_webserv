#include "../../headers/Server.hpp"

void	handleEvent(int fd, Client& client)
{
	int state = client.getClientState();
	if (state == reading_request)
	{
		// go to reading from socket loop
	}
	else if (state == parsing_request)
	{
		// parse request
	}
	else if (state == start_response)
	{
		// Readying everything and start sending response
	}
	else if (state == sending_response)
	{
		// go to send loop
	}
	else if (state == cgi_read)
	{
		if (fd != client.getClientFds(1)) // Correct argument?
		// Go to read from cgi loop
	}
	else if (state == cgi_write)
	{
		// go to write to cgi loop
	}
	else if (state == none)
		return ;
}





// int	readEvent(Client& client, int mode, int fd)
// {
// 	char		buffer[CGIBUFFER];
// 	int			bytes_read = 0;
// 	std::string	data;

// 	do
// 	{
// 		if (mode == READSOCKET)
// 			bytes_read = recv(fd, buffer, BUFFER_SIZE - 1, 0);
// 		else if (mode == READCGI)
// 			bytes_read = read(fd, buffer, CGIBUFFER - 1);
// 		if (bytes_read == -1)
// 		{
// 			std::cerr << "CGI ERROR: Failed reading from pipe with read()" << std::endl;
// 			break ;
// 		}
// 		if (bytes_read > 0)
// 		{
// 			buffer[bytes_read] = '\0';
// 			data += buffer;
// 			if (mode == READSOCKET)
// 				client._
// 			else if (mode == READCGI)
// 				client._response += buffer;
// 		}
// 	} while (bytes_read > 0);
// 	client.setReadData(data, mode);
// 	if (bytes_read  == -1)
// 		return (-1)
// }










class Server {
	bool	initialize();
	void	run();
	void	connectClient();
	void	recvFromSocket(Client& client);
	void	sendToSocket(Client& client);
};

class Client {
	void	parseRequest();
	void	createResponse();
	void	readCGI();
	void	writeCGI();
};