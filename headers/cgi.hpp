#pragma once

#include "headers.hpp"
#include "Server.hpp"

#define CGIBUFFER 32768

struct clRequest;

enum script_type {
	python,		// Python script
	php,		// PHP script
};

typedef struct s_cgiData
{
	char*		path;				// path to executable
	char** 		exe;				// executable double array
	char**		envp;				// Environment double array	
	int			ste_pipe[2];		// pipe for server to executable
	int			ets_pipe[2];		// pipe for executable to server
	bool		started_reading;	// Check if client object already started reading or has yet to start
	bool		started_writing;	// Check if client object already started writing or has yet to start
	pid_t		child_pid;			// Child process ID
	std::string writeData;			// Data to write to the pipe
	int			dataToWrite;		// Bytes needed to be written to
	int			dataWritten;		// bytes written to pipe
	std::string	readData;			// bytes read from pipe
	int			script_type;		// Python or PHP
}	t_cgiData;

bool	cgi_check(std::string& path);
int		start_cgi(clRequest& request, const Server& server, Client& client);
void	cgi_child_process(t_cgiData& cgi, const clRequest& request, Client& client);
void	cgi_cleanup(t_cgiData& cgi, bool child);
void	write_to_pipe(Client& client, t_cgiData& cgi, const Server& server);
void	read_from_pipe(Client& client, t_cgiData& cgi, Server& server, std::string& cgiBody);
