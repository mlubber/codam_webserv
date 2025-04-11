#include "../../headers/Client.hpp"

Client::Client(int socket_fd) : _state(reading_request), _cgi(nullptr)
{
	_fds.push_back(socket_fd);
}

Client::~Client()
{

}


void	Client::handleEvent(const Server& server)
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
		write_to_pipe(this->getCgiStruct(), server, false);
	}
	else if (_state == none)
		return ;
}


/* GETTERS */
int Client::getClientFds(int index)
{
	if (index == -1)
		return (_fds.size());
	else
		return (_fds[index]);
}

int	Client::getClientState()
{
	return (_state);
}

std::string& Client::getClientResponse()
{
	return (_response);
}

t_cgiData& Client::getCgiStruct()
{
	return (*_cgi);
}

bool	Client::checkCgiPtr()
{
	if (_cgi != nullptr)
		return (true);
	return (false);
}




/* SETTERS */
void Client::setCgiStruct(std::unique_ptr<t_cgiData> cgi)
{
	_cgi = std::move(cgi);
}

void	Client::setReceivedData(std::string& data)
{
	_received += data;
}


void	Client::setClientState(int state)
{
	_state = state;
	switch (_state) // Just for testing
	{
		case 0:
			std::cout << "\nClient state set to: reading_request\n" << std::endl;
			break;
		case 1:
			std::cout << "\nClient state set to: parsing_request\n" << std::endl;
			break;
		case 2:
			std::cout << "\nClient state set to: start_response\n" << std::endl;
			break;
		case 3:
			std::cout << "\nClient state set to: sending_response\n" << std::endl;
			break;
		case 4:
			std::cout << "\nClient state set to: cgi_read\n" << std::endl;
			break;
		case 5:
			std::cout << "\nClient state set to: cgi_write\n" << std::endl;
			break;
		default:
			std::cout << "\nClient state set to: none\n" << std::endl;
			break;
	}
}
