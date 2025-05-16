#include "../../headers/Client.hpp"

Client::Client(int socket_fd) : _state(reading_request), _cgi(nullptr)
{
	_fds.push_back(socket_fd);
}

Client::~Client()
{

}

int	Client::handleEvent(Server& server)
{
	server.setCurrentClient(_fds[0]);
	int status;

	if (_state == reading_request)
	{
		status = server.recvFromSocket(*this);
		if (status > 0)
			return (status);
	}
	if (_state == parsing_request)
	{
		status = parsingRequest(server, *this);
		if (status > 0)
			return (status);
	}
	if (_state == cgi_write)
	{
		status = write_to_pipe(*this, this->getCgiStruct(), server);
		if (status == 2)
		{
			// response is internal server error -> need to send that and then return to main loop and remove connection
			return (2);
		}
		if (status == 1)
			return (1);
	}
	if (_state == cgi_read)
	{
		status = read_from_pipe(*this, *this->_cgi, server, _request.cgiBody);
		if (status == 2)
		{
			// response is internal server error -> need to send that and then return to main loop and remove connection
			std::cerr << "ABOUT TO CLOSE CLIENT" << std::endl;
			return (2);
		}
		if (status == 1)
			return (1);
	}
	if (_state == sending_response)
	{
		status = server.sendToSocket(*this);
		if (status > 0)
			return (status);
	}
	return (0);
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

std::string&	Client::getClientReceived()
{
	return (_received);
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

clRequest&	Client::getClStructRequest()
{
    return (_request);
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


void	Client::setResponseData(std::string data)
{
	_response = data;
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
	}
}

/* Update client object data */
// Pass 0 to clear _received data, and pass 1 or bigger to clear _reponse
void	Client::clearData(int i)
{
	if (i == 0)
		_received.clear();
	else
	{
		_response.clear();
		_response_size = 0;
		_bytes_sent = 0;
	}
}

void	Client::updateBytesSent(size_t bytes_sent)
{
	this->_bytes_sent  += bytes_sent;
}
