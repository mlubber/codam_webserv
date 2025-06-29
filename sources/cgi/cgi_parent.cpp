#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/cgi.hpp"
#include "../../headers/Client.hpp"

static void wait_for_child(t_cgiData& cgi, Server& server)
{
	pid_t	wpid;
	int		status;
	int		exit_code = 0;

	wpid = waitpid(cgi.child_pid, &status, WNOHANG);
	if (wpid == 0)
	{
		server.addChildPidToMap(cgi.child_pid);
		cgi.child_pid = -1;
		return ;
	}
	if (wpid == -1)
		exit_code = errno;
	else if (WIFSIGNALED(status))
	{
		int signal_number = WTERMSIG(status);
		std::cerr << "CGI ERROR: Child terminated by signal: " << signal_number << std::endl;
		if (signal_number == SIGINT)
			exit_code = 130;
		else
			exit_code = 128 + signal_number;
	}
	else if (exit_code == 0 && WIFEXITED(status))
		exit_code = WEXITSTATUS(status);
	if (exit_code != 0)
		std::cerr << "NOTE: Child didn't end properly, code: " << exit_code << std::endl; 
}


void	createCgiResponse(Client& client, std::string& readData)
{
	std::string key = "\r\n\r\n";
	size_t pos = readData.find(key);
	if (pos == std::string::npos)
	{
		serveError(client, "500", client.getServerBlock());
		return ;
	}

	std::string actualBody = readData.substr(pos + 4);
	std::string contentLength = "Content-Length: " + std::to_string(actualBody.size()) + "\r\n";
	client.setResponseData("HTTP/1.1 200 OK\r\n" + contentLength + readData);
}


void	read_from_pipe(Client& client, t_cgiData& cgi, Server& server, std::string& readData)
{
	char	buffer[CGIBUFFER];
	int		bytes_read = 0;

	bytes_read = read(cgi.ets_pipe[0], buffer, CGIBUFFER - 1);
	if (bytes_read < 0)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ets_pipe[0], NULL);
		if (close(cgi.ets_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		cgi.ets_pipe[0] = -1;
		wait_for_child(cgi, server);
		serveError(client, "500", client.getServerBlock());
		return ;
	}
	if (bytes_read > 0)
	{
		readData.append(buffer, bytes_read);
		return ;
	}

	if (epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ets_pipe[0], NULL) == -1)
		std::cerr << "CGI ERROR: Failed deleting ets_pipe fd after reading all data from pipe" << std::endl;
	if (close(cgi.ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
	cgi.ets_pipe[0] = -1;

	createCgiResponse(client, readData);

	struct epoll_event event;
	event.events = EPOLLOUT;
	event.data.fd = client.getClientFds(0);
	epoll_ctl(server.getEpollFd(), EPOLL_CTL_MOD, client.getClientFds(0), &event);

	client.resetFds(client.getClientFds(0));
	client.setClientState(sending_response);
	wait_for_child(cgi, server);
	cgi.child_pid = -1;
}


void	write_to_pipe(Client& client, t_cgiData& cgi, const Server& server)
{
	int currentDataSize = cgi.writeData.size();
	int	written = 0;
		
	written = write(cgi.ste_pipe[1], cgi.writeData.c_str(), std::min(CGIBUFFER, currentDataSize));
	cgi.dataWritten += written;
	if (cgi.dataToWrite == cgi.dataWritten)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ste_pipe[1]) == -1)
			std::cerr << "CGI ERROR: Failed closing ste write-end pipe in parent" << std::endl;
		cgi.ste_pipe[1] = -1;
		client.setClientState(cgi_read);
	}
	else if (written > 0)
		cgi.writeData.erase(0, cgi.dataWritten);
	else
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ste_pipe[1]) == -1)
			std::cerr << "CGI ERROR: Failed closing ste write-end pipe in parent" << std::endl;
		cgi.ste_pipe[1] = -1;
		kill(cgi.child_pid, SIGTERM);
		serveError(client, "500", client.getServerBlock());
	}
}
