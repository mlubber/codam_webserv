#pragma once

#include "Server.hpp"

typedef struct s_cgiData
{
	char*	path;
	char** 	script;
	char**	envp;
	int		ste_pipe[2];
	int		ets_pipe[2];
	int		stdin_backup;
	int		stdout_backup;
}	t_cgiData;

int		check_cgi(const HttpRequest& request);

/* Utils */
void	init_cgi_struct(t_cgiData* cgi);
void	closing_fds(t_cgiData* cgi, bool parent);