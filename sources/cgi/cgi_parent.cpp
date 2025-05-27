#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/cgi.hpp"
#include "../../headers/Client.hpp"

int	wait_for_child(t_cgiData& cgi)
{
	pid_t	wpid;
	int		status;
	int		exit_code = 0;

	std::cout << "errno before wait_for_child: " << errno << ", str: " << strerror(errno) << std::endl;

	if (errno != 0)
		return (errno);
	std::cout << "CHECKING WPID" << std::endl;
	wpid = waitpid(cgi.child_pid, &status, WNOHANG);
	if (wpid == 0)
	{
		std::cout << "Killing child process\n" << std::endl;
		kill(cgi.child_pid, SIGTERM);
		wpid = waitpid(cgi.child_pid, &status, 0);
		return (0);
	}
	if (wpid == -1)
		exit_code = errno;
	else if (WIFSIGNALED(status))
	{
		int signal_number = WTERMSIG(status);
		std::cout << "CGI ERROR: Child terminated by signal: " << signal_number << std::endl;
		if (signal_number == SIGINT)
			exit_code = 130;
		else
			exit_code = 128 + signal_number;
	}
	else if (exit_code == 0 && WIFEXITED(status))
		exit_code = WEXITSTATUS(status);
	if (exit_code == 0)
		return (0);
	else
		return(2);
}




void	createCgiResponse(Client& client, std::string& readData)
{
	std::string key = "\r\n\r\n";
	size_t pos = readData.find(key);
	if (pos == std::string::npos)
	{
		std::cout << "NPOS FOUND? INTERNAL SERVER ERROR" << std::endl;
		serveError(client, "500", client.getServerBlock());
		return ;
	}

	std::string actualBody = readData.substr(pos + 4);
	std::cout << "ACTUAL BODY:" << actualBody << std::endl;

	std::string contentLength = "Content-Length: " + std::to_string(actualBody.size()) + "\r\n";

	client.setResponseData("HTTP/1.1 200 OK\r\n" + contentLength + readData);
}




















void	read_from_pipe(Client& client, t_cgiData& cgi, const Server& server, std::string& readData)
{
	std::cout << "\nREAD FROM PIPE" << std::endl;

	char	buffer[CGIBUFFER];
	int		bytes_read = 0;

	bytes_read = read(cgi.ets_pipe[0], buffer, CGIBUFFER - 1);
	buffer[bytes_read] = '\0';
	std::cout << "\nBytes read from cgi pipe: " << bytes_read << " / " << CGIBUFFER - 1 << std::endl;
	if (bytes_read == -1)
	{
		std::cout << "\nBYTES_READ -1!" << std::endl;
		serveError(client, "500", client.getServerBlock());
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ets_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		cgi.ets_pipe[0] = -1;
		return ;
	}
	readData += buffer;
	if (bytes_read > 0)
		return ;

	epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ets_pipe[0], NULL);
	if (close(cgi.ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
	cgi.ets_pipe[0] = -1;

	createCgiResponse(client, readData);

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client.getClientFds(0);
	epoll_ctl(server.getEpollFd(), EPOLL_CTL_MOD, client.getClientFds(0), &event);

	client.resetFds(client.getClientFds(0));
	client.setClientState(sending_response);
	wait_for_child(cgi);
}


















void	write_to_pipe(Client& client, t_cgiData& cgi, const Server& server)
{
	int currentDataSize = cgi.writeData.size();
	int	written = 0;
		
	written += write(cgi.ste_pipe[1], cgi.writeData.c_str(), std::min(CGIBUFFER, currentDataSize));
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
