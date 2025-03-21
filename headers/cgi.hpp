#pragma once

#include "Server.hpp"

typedef struct s_cgiData
{
	char*	path;		// path to executable
	char** 	exe;		// ?
	char**	envp;		// Environment double array
	int		ste_pipe[2];	// pipe for server to executable	
	int		ets_pipe[2];	// pipe for executable to server
	int		stdin_backup;	// backup fd for STDIN
	int		stdout_backup;	// backup fd for STDOUT
}	t_cgiData;

int		cgi_check(const HttpRequest& request, const Server& server);
void	cgi_child_process(t_cgiData* cgi, const HttpRequest& request, const Server& server);
bool	cgi_setup(t_cgiData* cgi, const HttpRequest& request, const Server& server);

void	cgi_cleanup(t_cgiData* cgi);
void	closing_fds(t_cgiData* cgi, bool parent);