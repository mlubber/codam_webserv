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
	wpid = waitpid(cgi.child_pid, &status, 0);
	if (wpid == -1)
		exit_code = errno;
	else if (exit_code == 0 && WIFSIGNALED(status))
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
	return (exit_code);
}


int	read_from_pipe(t_cgiData& cgi, const Server& server, std::string& cgiBody)
{
	char	buffer[CGIBUFFER];
	int		bytes_read = 0;

	if (cgi.started_reading == false)
	{
		if (close(cgi.ets_pipe[1]) == -1)
		{
			std::cerr << "CGI ERROR: Failed closing write-end pipe in parent" << std::endl;
			if (close(cgi.ets_pipe[0]) == -1)
				std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;
			return (1);
		}
		cgi.started_reading = true;
	}
	do
	{
		bytes_read = read(cgi.ets_pipe[0], buffer, CGIBUFFER - 1);
		if (bytes_read == -1)
		{
			std::cerr << "CGI ERROR: Failed reading from pipe with read()" << std::endl;
			break ;
		}
		if (bytes_read > 0)
		{
			buffer[bytes_read] = '\0';
			cgiBody += buffer;
		}
	} while (bytes_read > 0);

	if (bytes_read == -1)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		close(cgi.ste_pipe[1]);
		// client._state = cgi_read; // Needs to set state if not finished reading
	}
	if (cgi.ets_pipe[0] != -1 && close(cgi.ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;

	return (wait_for_child(cgi));
	
	// This is missing some checks to see if reading was completed 
	// and if we already closed the ets_pipe[0] at the end
}


void	write_to_pipe(t_cgiData& cgi, const Server& server, bool start)
{
	int dataSize = cgi.writeData.size();
	int bytesToWrite = 0;
	int	writtenBytes = 0;
	int	written = 0;

	if (start == true && close(cgi.ste_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing write-end pipe in parent" << std::endl;
	while (writtenBytes < dataSize)
	{
		bytesToWrite = std::min(CGIBUFFER, dataSize - writtenBytes);
		written = write(cgi.ste_pipe[1], cgi.writeData.c_str() + writtenBytes, bytesToWrite);
		if (written == -1)
		{
			std::cerr << "CGI ERROR: Failed writing to pipe with write()" << std::endl;
			break ;
		}
		writtenBytes += written;
	}
	if (written != -1)
	{
		epoll_ctl(server.getEpollFd(), EPOLL_CTL_DEL, cgi.ste_pipe[1], NULL);
		if (close(cgi.ste_pipe[1]) == -1)
			std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;
	}
	errno = 0;
}

int	cgi_parent_process(t_cgiData& cgi, clRequest& request, const Server& server)
{
	if (request.method == "POST")
		write_to_pipe(cgi, server, true);
	return (read_from_pipe(cgi, server, request.cgiBody));
}
