#include "../../headers/Server.hpp"

static bool	init_cgi_struct(t_cgiData* cgi)
{
	cgi->path = nullptr;
	cgi->exe = nullptr;
	cgi->envp = nullptr;
	cgi->stdin_backup = dup(STDIN_FILENO);
	cgi->stdout_backup = dup(STDOUT_FILENO);
	if (cgi->stdin_backup == -1 || cgi->stdout_backup == -1)
		return (1);
	if (pipe(cgi->ste_pipe) == -1)
	{
		std::cout << "CGI ERROR: creating pipe failed" << std::endl;
		return (1);
	}
	else if (pipe(cgi->ets_pipe) == -1)
	{
		std::cout << "CGI ERROR: creating pipe failed" << std::endl;
		closing_fds(cgi, true);
		return (1);
	}
	return (0);
}

static bool	create_cgi_process(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	pid_t		pid;

	pid = fork();
	if (pid == -1)
	{
		std::cout << "CGI ERROR: creating child process failed" << std::endl;
		return (-1);
	}
	else if (pid == 0)
		cgi_child_process(cgi, request, server);
	else
		std::cout << "Doing parent process stuff!" << std::endl;
	return (1);
}

int	cgi_check(const HttpRequest& request, const Server& server)
{
	t_cgiData	cgi;
	int			status = 0;

	std::cout << "PATH from request: " << request.path << std::endl;
	if (request.path.compare(0, 9, "/cgi-bin/") == 0)
	{
		std::cout << "CGI FOUND !!!" << std::endl;
		if (init_cgi_struct(&cgi))
			return (-1);
		status = create_cgi_process(&cgi, request, server);
		if (status == -1)
			std::cout << "Something with this status" << std::endl; // Something here when something goes wrong in the CGI functions
		return (status);
	}
	return (status);
}


/* STEPS */

// 1. check if CGI needed	✓

// 2. create 2 pipes -> 1 from server to script, 1 from script to server	✓

// 3. fork process	✓

//		***** Child process *****			|	***** parent process *****

// 4.	create environment array  ✓			|	close write-end / dup2 read-end

// 5. 	close read-end / dup2 write-end		|	Wait for data

// 6.	run executable with execve			|	Store data in std::string(?) and return

// 7.										|	close fds and set STDIN and STDOUT back

// 8.										|	Create response header and send to client


// Errors

//		In case of errors, close fds and	| 	In case of errors, close fds
//		free malloc'ed data -> exit(1)?