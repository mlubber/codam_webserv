#pragma once

#include "headers.hpp"
#include "Server.hpp"

#define CGIBUFFER 8192

struct clRequest;

typedef struct s_cgiData
{
	char*		path;				// path to executable
	char** 		exe;				// executable double array
	char**		envp;				// Environment double array	
	int			ste_pipe[2];		// pipe for server to executable
	int			ets_pipe[2];		// pipe for executable to server
	bool		started_reading;	// Check if client object already started reading or has yet to start
	bool		started_writing;	// Check if client object already started writing or has yet to start
	int			child_pid;			// Child process ID
	std::string writeData;			// Data to write to the pipe
	int			dataToWrite;		// Bytes needed to be written to
	int			dataWritten;		// bytes written to pipe
	std::string	readData;			// bytes read from pipe
}	t_cgiData;

bool	cgi_check(std::string& path);
int		start_cgi(clRequest& request, Server& server, Client& client);
void	cgi_child_process(t_cgiData& cgi, const clRequest& request, const Server& server);
void	cgi_cleanup(t_cgiData& cgi, bool child);
int		write_to_pipe(Client& client, t_cgiData& cgi, const Server& server);
int		read_from_pipe(Client& client, t_cgiData& cgi, const Server& server, std::string& cgiBody);
