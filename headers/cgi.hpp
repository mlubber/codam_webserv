#pragma once

#include "headers.hpp"
#include "Server.hpp"

#define CGIBUFFER 8192

struct clRequest;

typedef struct s_cgiData
{
	char*	path;			// path to executable
	char** 	exe;			// executable double array
	char**	envp;			// Environment double array	
	int		ste_pipe[2];	// pipe for server to executable
	int		ets_pipe[2];	// pipe for executable to server
	std::string writeData;	// Data to write to the pipe
}	t_cgiData;

int		cgi_check(clRequest& request, const Server& server, Client& client);
void	cgi_child_process(t_cgiData& cgi, const clRequest& request, const Server& server);
bool	cgi_setup(t_cgiData& cgi, const clRequest& request, const Server& server);
void	cgi_cleanup(t_cgiData& cgi, bool child);
int		cgi_parent_process(t_cgiData& cgi, clRequest& request, const Server& server, const pid_t& pid);
void	write_to_pipe(t_cgiData& cgi, const Server& server, bool start);
void	read_from_pipe(t_cgiData& cgi, const Server& server, std::string& cgiBody, bool start);