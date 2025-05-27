#include "../../headers/Client.hpp"

Client::Client(int socket_fd, const ConfigBlock& serverBlock) : _state(reading_request), _cgi(nullptr), _close_client(false), _server_block(serverBlock)
{
	_fds.push_back(socket_fd);
}

Client::~Client()
{

}

// int	Client::handleEvent(Server& server)
// {
// 	int status;

// 	if (_state == reading_request)
// 	{
// 		std::cout << "\nCurrent client state: reading_request" << std::endl;
// 		std::cout << "\nErrno before recvFromSocket: " << errno << ", str: " << strerror(errno) << std::endl;
// 		// status = server.recvFromSocket(*this, this->_request.receivedData);
// 		status = server.recvFromSocket(*this);
// 		std::cout << "status is now: " << status << std::endl;
// 		errno = 0;
// 		if (status == 2)
// 		{
// 			std::cout << "Lost connection or error occurred" << std::endl;
// 			return (2);
// 		}
// 		if (status == 1)
// 		{
// 			std::cout << "Partial request received" << std::endl;
// 			return (1);
// 		}

// 		// if (status > 0)
// 		// {
// 		// 	return (status);
// 		// }
// 	}
// 	// std::cout << "\nErrno before parsing: " << errno << ", str: " << strerror(errno) << std::endl;

// 	if (_state == parsing_request)
// 	{
// 		std::cout << "\nCurrent client state: parsing_request" << std::endl;
// 		status = parsingRequest(server, *this);
// 		if (status == 2)
// 			return (status);
// 		else if (_state != cgi_write)
// 			return (status);
// 	}
// 	if (_state == cgi_write)
// 	{
// 		std::cout << "\nCurrent client state: cgi_write" << std::endl;
// 		status = write_to_pipe(*this, this->getCgiStruct(), server);
// 		if (status == 2)
// 		{
// 			// response is internal server error -> need to send that and then return to main loop and remove connection
// 			return (2);
// 		}
// 		return (status);
// 	}
// 	if (_state == cgi_read)
// 	{
// 		std::cout << "\nCurrent client state: cgi_read" << std::endl;
// 		status = read_from_pipe(*this, *this->_cgi, server, _cgi->readData);
// 		std::cout << "\nStatus after read_from_pipe : " << status << std::endl;
// 		if (status == 2)
// 		{
// 			// response is internal server error -> need to send that and then return to main loop and remove connection
// 			std::cerr << "ABOUT TO CLOSE CLIENT" << std::endl;
// 			this->setCloseClientState(true);
// 			this->setResponseData(ER500);
// 			this->setClientState(sending_response);
// 			return (2);
// 		}
// 		if (status == 1)
// 		{
// 			return (1);
// 		}
// 	}
// 	if (_state == sending_response)
// 	{
// 		std::cout << "\nCurrent client state: sending_response" << std::endl;
// 		return (server.sendToSocket(*this));
// 	}
// 	return (0);
// }




int	Client::handleEvent(Server& server)
{
	int status;

	if (_state == reading_request)
	{
		// status = server.recvFromSocket(*this, this->_request.receivedData);
		status = server.recvFromSocket(*this);
		errno = 0;
		if (status == 2)
			return (2);
		if (status == 1)
			return (1);
	}

	if (_state == parsing_request)
	{
		parsingRequest(server, *this);
	}
	if (_state == cgi_write)
	{
		write_to_pipe(*this, this->getCgiStruct(), server); return ;
	}
	if (_state == cgi_read)
	{
		std::cout << "\nCurrent client state: cgi_read" << std::endl;
		status = read_from_pipe(*this, *this->_cgi, server, _cgi->readData);
		std::cout << "\nStatus after read_from_pipe : " << status << std::endl;
		if (status == 2)
		{
			this->setCloseClientState(true);
			this->setResponseData(ER500);
			this->setClientState(sending_response);
			return (2);
		}
		if (status == 1)
		{
			return (1);
		}
	}
	if (_state == sending_response)
	{
		std::cout << "\nCurrent client state: sending_response" << std::endl;
		return (server.sendToSocket(*this));
	}
	return (0);
}










/* GETTERS */
int Client::getClientFds(int index) const
{
	if (index == -1)
		return (_fds.size());
	else
		return (_fds[index]);
}

int	Client::getClientState() const
{
	return (_state);
}

const std::string&	Client::getClientReceived() const
{
	return (_received);
}

const std::string& Client::getClientResponse() const
{
	return (_response);
}

t_cgiData& Client::getCgiStruct() const
{
	return (*_cgi);
}

bool	Client::checkCgiPtr() const
{
	if (_cgi != nullptr)
		return (true);
	return (false);
}

clRequest&	Client::getClStructRequest()
{
    return (_request);
}

bool	Client::getCloseClientState() const
{
	return (_close_client);
}

const ConfigBlock&	Client::getServerBlock() const
{
	return (_server_block);
}



/* SETTERS */
void Client::setCgiStruct(std::unique_ptr<t_cgiData> cgi)
{
	_cgi = std::move(cgi);
}

void	Client::setReceivedData(std::string data)
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
			std::cout << "\nClient state set to: cgi_write\n" << std::endl;
			break;
		case 3:
			std::cout << "\nClient state set to: cgi_read\n" << std::endl;
			break;
		case 4:
			std::cout << "\nClient state set to: sending_response\n" << std::endl;
			break;

	}
}

void	Client::setCloseClientState(bool state)
{
	_close_client = state;
}



void Client::addFd(int fd)
{
	_fds.push_back(fd);
}

void Client::resetFds(int fd)
{
	_fds = { fd };
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
