#include "../../headers/headers.hpp"
#include "../../headers/Server.hpp"
#include "../../headers/cgi.hpp"
#include "../../headers/Client.hpp"

int	wait_for_child(t_cgiData& cgi)
{
	pid_t	wpid;
	int		status;
	int		exit_code = 0;

	if (errno != 0)
		return (errno);
	std::cerr << "CHECKING WPID" << std::endl;
	wpid = waitpid(cgi.child_pid, &status, WNOHANG);
	if (wpid == 0)
	{
		std::cerr << "Killing child process\n" << std::endl;
		kill(cgi.child_pid, SIGTERM);
		wpid = waitpid(cgi.child_pid, &status, 0);
		return (0);
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
	if (exit_code == 0)
		return (0);
	else
		return(2);
}


int	read_from_pipe(Client& client, t_cgiData& cgi, const Server& server, std::string& cgiBody)
{
	char	buffer[CGIBUFFER];
	int		bytes_read = 0;

	std::cerr << "\nREADING FROM PIPE" << std::endl;

	std::cerr << "read-end num: " << cgi.ets_pipe[0] << std::endl;
	bytes_read = read(cgi.ets_pipe[0], buffer, CGIBUFFER - 1);
	std::cerr << "Bytes read from cgi pipe: " << bytes_read << std::endl;
	if (bytes_read == -1)
	{
		std::cerr << "BYTES_READ -1!" << std::endl;
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ets_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		cgi.ets_pipe[0] = -1;
		return (2);
	}
	else if (bytes_read == 0)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ets_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing ets read-end pipe in parent" << std::endl;
		cgi.ets_pipe[0] = -1;		
		client.setClientState(cgi_read);
		return (wait_for_child(cgi));
	}
	buffer[bytes_read] = '\0';
	cgiBody += buffer;
	std::cerr << "CGI BODY:\n\n" << cgiBody << std::endl;
	return (1);
}

int	write_to_pipe(Client& client, t_cgiData& cgi, const Server& server)
{
	int currentDataSize = cgi.writeData.size();
	int	written = 0;

	if (cgi.started_writing == false) 
	{
		if (close(cgi.ste_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing ste read-end pipe in parent" << std::endl;
		cgi.ste_pipe[0] = -1;
	}
		
	written += write(cgi.ste_pipe[1], cgi.writeData.c_str(), std::min(CGIBUFFER, currentDataSize));
	cgi.dataWritten += written;
	if (cgi.dataToWrite == cgi.dataWritten)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ste_pipe[1]) == -1)
			std::cerr << "CGI ERROR: Failed closing ste write-end pipe in parent" << std::endl;
		cgi.ste_pipe[1] = -1;
		client.setClientState(cgi_read);
		return (0);
	}
	else if (written > 0)
	{
		cgi.writeData.erase(0, cgi.dataWritten);
		return (1);
	}
	epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
	if (close(cgi.ste_pipe[1]) == -1)
		std::cerr << "CGI ERROR: Failed closing ste write-end pipe in parent" << std::endl;
	cgi.ste_pipe[1] = -1;
	kill(cgi.child_pid, SIGTERM);
	return (2);
}
