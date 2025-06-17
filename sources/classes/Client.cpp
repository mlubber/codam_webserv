#include "../../headers/Client.hpp"

Client::Client(int socket_fd) : _state(reading_request), _cgi(nullptr), _close_client(false)
{
	_fds.push_back(socket_fd);
	_last_request = std::time(nullptr);
}

Client::~Client()
{

}

void	Client::handleEvent(Server& server)
{
	int status;

	if (_state == reading_request || _state == idle)
	{
		status = server.recvFromSocket(*this);
		if (status == 1)
			return ;
	}
	if (_state == parsing_request)
	{
		parsingRequest(server, *this);
		return ;
	}
	if (_state == cgi_write)
	{
		write_to_pipe(*this, this->getCgiStruct(), server);
		return ;
	}
	if (_state == cgi_read)
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

std::string&	Client::getClientReceived()
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

long	Client::getLastRequest() const
{
	return (_last_request);
}

std::string		Client::getServerBlockInfo(std::string search) const
{
	std::string found;
	for (const std::pair<const std::string, std::vector<std::string>> &value : _server_block.values)
	{
		if (value.first == search)
		{
			found = value.second.front();
			return (found);
		}
	}
	return ("\"\"");
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

void	Client::setServerBlock(ConfigBlock serverBlock)
{
	_server_block = serverBlock;
}


void	Client::setClientState(int state)
{
	_state = state;
}

void	Client::setCloseClientState(bool state)
{
	_close_client = state;
}

void	Client::setLastRequest()
{
	_last_request = std::time(nullptr);
}


void Client::addFd(int fd)
{
	_fds.push_back(fd);
}

void Client::resetFds(int fd)
{
	_fds = { fd };
}

void	Client::resetClient(int epoll_fd)
{
	_received.clear();
	_response.clear();
	_response_size = 0;
	_bytes_sent = 0;

	if (_cgi != nullptr)
	{
		if (_cgi->ets_pipe[0] != -1)
		{
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, _cgi->ets_pipe[0], NULL);
			if (close(_cgi->ets_pipe[0]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
			if (_fds.size() > 1)
				_fds.erase(_fds.begin() + 1);
		}
		if (_cgi->ste_pipe[1] != -1)
		{
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, _cgi->ste_pipe[1], NULL);
			if (close(_cgi->ste_pipe[1]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
			if (_fds.size() > 1)
				_fds.erase(_fds.begin() + 1);
		}
		_cgi = nullptr;
	}
	_state = idle;
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = _fds[0];
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _fds[0], &event);
}




void	Client::updateBytesSent(size_t bytes_sent)
{
	this->_bytes_sent  += bytes_sent;
}
