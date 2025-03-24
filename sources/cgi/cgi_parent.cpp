#include "../../headers/Server.hpp"

static void	read_from_pipe(t_cgiData* cgi, std::string& cgiBody)
{
	char	buffer[CGIBUFFER];
	int		bytes_read = 0;

	if (close(cgi->ets_pipe[1]) == -1)
	{
		std::cerr << "CGI ERROR: Failed closing write-end pipe in parent" << std::endl;
		if (close(cgi->ets_pipe[0]) == -1)
			std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;
		return ;
	}
	do
	{
		bytes_read = read(cgi->ets_pipe[0], buffer, CGIBUFFER - 1);
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
	if (close(cgi->ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;
}

int	cgi_parent_process(t_cgiData* cgi, HttpRequest& request, const pid_t& pid)
{
	pid_t	wpid;
	int		status;
	int		exit_code = 0;

	read_from_pipe(cgi, request.cgiBody);
	if (errno != 0)
		return (errno);
	wpid = waitpid(pid, &status, 0);
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