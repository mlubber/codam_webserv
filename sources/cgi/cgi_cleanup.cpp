#include "../../headers/Server.hpp"

void	closing_fds(t_cgiData* cgi, bool parent)
{
	if (cgi->ste_pipe[0] == -1 && close(cgi->ste_pipe[0]))
		std::cout << "CGI ERROR: closing ste_pipe[0] failed" << std::endl;
	if (cgi->ste_pipe[1] == -1 && close(cgi->ste_pipe[1]))
		std::cout << "CGI ERROR: closing ste_pipe[1] failed" << std::endl;
	if (cgi->ets_pipe[0] == -1 && close(cgi->ets_pipe[0]))
		std::cout << "CGI ERROR: closing ets_pipe[0] failed" << std::endl;
	if (cgi->ets_pipe[1] == -1 && close(cgi->ets_pipe[1]))
		std::cout << "CGI ERROR: closing ets_pipe[1] failed" << std::endl;
	cgi->ste_pipe[0] = -1;
	cgi->ste_pipe[1] = -1;
	cgi->ets_pipe[0] = -1;
	cgi->ets_pipe[1] = -1;
	if (parent == true)
	{
		if (dup2(STDIN_FILENO, cgi->stdin_backup) == -1)
			std::cout << "CGI ERROR: dup2 stdin_backup to STDIN_FILENO failed" << std::endl;
		if (dup2(STDOUT_FILENO, cgi->stdout_backup) == -1)
			std::cout << "CGI ERROR: dup2 stdout_backup to STDOUT_FILENO failed" << std::endl;
	}
}

void	cgi_cleanup(t_cgiData* cgi)
{
	if (cgi->path != nullptr)
		delete[] cgi->path;
	for (int i = 0; i < 10; i++)
	{
		if (cgi->envp[i] != nullptr)
			delete[] cgi->envp[i];
	}
	if (cgi->envp != nullptr)
		delete[] cgi->envp;
	closing_fds(cgi, false);
	exit(1);
}