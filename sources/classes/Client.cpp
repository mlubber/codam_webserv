#include "../../headers/Client.hpp"

Client::Client(int socket_fd) : _state(reading_request)
{
	_fds.push_back(socket_fd);
}

Client::~Client()
{

}


void	Client::handleEvent(int fd)
{
	if (_state == reading_request)
	{
		// go to reading from socket loop
	}
	else if (_state == parsing_request)
	{
		// parse request
	}
	else if (_state == start_response)
	{
		// Readying everything and start sending response
	}
	else if (_state == sending_response)
	{
		// go to send loop
	}
	else if (_state == cgi_read)
	{
		// Go to read from cgi loop
	}
	else if (_state == cgi_write)
	{
		// go to write to cgi loop
	}
	else if (_state == none)
		return ;
}



int Client::getClientFds(int index)
{
	if (index == -1)
		return (_fds.size());
	else
		return (_fds[index]);
}