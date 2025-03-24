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
	if (pipe(cgi->ets_pipe) == -1)
	{
		std::cerr << "CGI ERROR: creating pipe failed" << std::endl;
		return (1);
	}
	return (0);
}

void	read_from_pipe(t_cgiData* cgi, std::string& cgiBody)
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
	std::cerr << "WE'RE IN CGI READ_FROM_PIPE" << std::endl;

	dup2(cgi->ets_pipe[0], STDIN_FILENO);
	do
	{
		bytes_read = read(cgi->ets_pipe[0], buffer, CGIBUFFER - 1);
		if (bytes_read > 0)
		{
			buffer[bytes_read] = '\0';
			cgiBody += buffer;
		}
	} while (bytes_read > 0);
	if (close(cgi->ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing read-end pipe in parent" << std::endl;
	dup2(cgi->stdin_backup, STDIN_FILENO);



}


static bool	create_cgi_process(t_cgiData* cgi, HttpRequest& request, const Server& server)
{
	pid_t	pid;
	pid_t	wpid;
	int		status;

	pid = fork();
	if (pid == -1)
	{
		std::cerr << "CGI ERROR: creating child process failed" << std::endl;
		return (-1);
	}
	else if (pid == 0)
		cgi_child_process(cgi, request, server);
	else
	{
		std::cerr << "Doing parent process stuff!" << std::endl;
		read_from_pipe(cgi, request.cgiBody);
		wpid = waitpid(pid, &status, 0);
		while (wait(NULL) != -1)
			continue ;
		if (wpid == -1)
			std::cout << "Something wrong with child process after waitpid" << std::endl;
		std::cout << "WPID and Status: " << wpid << " " << status << std::endl;
	}
	return (1);
}

int	cgi_check(HttpRequest& request, const Server& server)
{
	t_cgiData	cgi;
	int			status = 0;

	if (request.path.compare(0, 9, "/cgi-bin/") == 0)
	{
		// std::cout << "CGI FOUND !!!" << std::endl;
		request.cgi = true;
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