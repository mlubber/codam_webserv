#include "../../headers/Client.hpp"

Client::Client(int socket_fd, const ConfigBlock& serverBlock) : _state(reading_request), _cgi(nullptr), _close_client(false), _server_block(serverBlock)
{
	_fds.push_back(socket_fd);
}

Client::~Client()
{

}

void	Client::handleEvent(Server& server)
{
	int status;

	if (_state == reading_request)
	{
		// status = server.recvFromSocket(*this, this->_request.receivedData);
		status = server.recvFromSocket(*this);
		errno = 0;
		// if (status == 2)
		// 	return (2);
		// if (status == 1)
		// 	return (1);
		// return ;
	}

	if (_state == parsing_request)
	{
		parsingRequest(server, *this);
		return ;
	}


	if (_state == cgi_write) // POST method
	{
		write_to_pipe(*this, this->getCgiStruct(), server);
		return ;
	}


	if (_state == cgi_read) // GET method or next step after POST
	{
		read_from_pipe(*this, *this->_cgi, server, _cgi->readData);
		return ;
	}



	if (_state == sending_response)
	{
		server.sendToSocket(*this);
	}
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

void	Client::setReceivedData(const char* data, ssize_t bytes_received)
{
	_received.append(data, bytes_received);
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
void	Client::clearData(int epollFd)
{
	std::cout << "\n\n---- REQUEST RECEIVED IN CLEAR DATA -----\n" << _received << "\n---- END OF RECEIVED IN CLEAR DATA -----\n\n" << std::endl;
	_received.clear();
	std::cout << "---- REQUEST RECEIVED IN CLEAR DATA -----\n" << _received << "\n---- END OF RECEIVED IN CLEAR DATA -----\n" << std::endl;

	_response.clear();
	_response_size = 0;
	_bytes_sent = 0;

	if (_cgi != nullptr)
	{
		if (_cgi->ets_pipe[0] != -1)
		{
			std::cout << "Deleting and closing read-end pipe " << _cgi->ets_pipe[0] << std::endl;
			epoll_ctl(epollFd, EPOLL_CTL_DEL, _cgi->ets_pipe[0], NULL);
			if (close(_cgi->ets_pipe[0]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		}
		if (_cgi->ste_pipe[1] != -1)
		{
			std::cout << "Deleting and closing read-end pipe " << _cgi->ste_pipe[1] << std::endl;
			epoll_ctl(epollFd, EPOLL_CTL_DEL, _cgi->ste_pipe[1], NULL);
			if (close(_cgi->ste_pipe[1]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		}
		_cgi = nullptr;
	}

	_state = reading_request;
}

void	Client::updateBytesSent(size_t bytes_sent)
{
	this->_bytes_sent  += bytes_sent;
}
