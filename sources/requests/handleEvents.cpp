// #include "../../headers/Server.hpp"

// void	handleEvent(int fd, Client& client)
// {
// 	int state = client.getClientState();
// 	if (state == reading_request)
// 	{
// 		// go to reading from socket loop
// 	}
// 	else if (state == parsing_request)
// 	{
// 		// parse request
// 	}
// 	else if (state == start_response)
// 	{
// 		// Readying everything and start sending response
// 	}
// 	else if (state == sending_response)
// 	{
// 		// go to send loop
// 	}
// 	else if (state == cgi_read)
// 	{
// 		if (fd != client.getClientFds(1)) // Correct argument?
// 		// Go to read from cgi loop
// 	}
// 	else if (state == cgi_write)
// 	{
// 		// go to write to cgi loop
// 	}
// 	else if (state == none)
// 		return ;
// }





// // header 1
// class Server {
// 	bool	initialize();
// 	void	run();
// 	void	connectClient();
// 	void	recvFromSocket(Client& client);
// 	void	sendToSocket(Client& client);
// };

// // header 2
// class Client {
// 	void	parseRequest();
// 	void	createResponse();
// 	void	readCGI();
// 	void	writeCGI();
// };

// // header 3
// struct cgi {
// 	//things
// };

// header 3
// All needed headers and shared functions