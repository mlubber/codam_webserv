#pragma once

#include "Server.hpp"
#define CGIBUFFER 1024

typedef struct s_cgiData
{
	char*	path;			// path to executable
	char** 	exe;			// executable double array
	char**	envp;			// Environment double array	
	int		ets_pipe[2];	// pipe for executable to server
	std::string	data;		// Data read from pipe and saved into string
}	t_cgiData;

int		cgi_check(HttpRequest& request, const Server& server);
void	cgi_child_process(t_cgiData* cgi, const HttpRequest& request, const Server& server);
int		cgi_parent_process(t_cgiData* cgi, HttpRequest& request, const pid_t& pid);
bool	cgi_setup(t_cgiData* cgi, const HttpRequest& request, const Server& server);
void	cgi_cleanup(t_cgiData* cgi, bool child);
