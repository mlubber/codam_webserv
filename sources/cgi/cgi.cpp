#include "../../headers/Server.hpp"

void	prepare_cgi_data(t_cgiData* cgi, const HttpRequest& request)
{
	// const char* path = request.path.c_str();
	cgi->path = new char[request.path.size() + 1];
	std::strcpy(cgi->path, request.path.c_str());
	std::cout << "CGI->PATH after conversion to char*: " <<cgi->path << std::endl;
}

static bool	create_cgi_process(t_cgiData* cgi, const HttpRequest& request)
{
	pid_t		pid;

	if (pipe(cgi->ste_pipe) == -1)
	{
		std::cout << "CGI ERROR: creating pipe failed" << std::endl;
		return (-1);
	}
	else if (pipe(cgi->ets_pipe) == -1)
	{
		std::cout << "CGI ERROR: creating pipe failed" << std::endl;
		closing_fds(cgi, true);
		return (-1);
	}

	pid = fork();
	if (pid == -1)
	{
		std::cout << "CGI ERROR: creating child process failed" << std::endl;
		return (-1);
	}
	else if (pid == 0)
	{
		std::cout << "We're in the child process!" << std::endl;

		prepare_cgi_data(cgi, request);
		closing_fds(cgi, false);
		exit(0);
	}
	else
	{
		std::cout << "Doing parent process stuff!" << std::endl;
	}
	return (1);
}

int	check_cgi(const HttpRequest& request)
{
	t_cgiData	cgi;
	int			status = 0;

	std::cout << "PATH from request: " << request.path << std::endl;
	if (request.path.compare(0, 10, "/cgi-bin/") == 0)
	{
		init_cgi_struct(&cgi);
		status = create_cgi_process(&cgi, request);
		if (status == -1)
			std::cout << "Something with this status" << std::endl; // Something here when something goes wrong in the CGI functions
		return (status);
	}
	std::cout << "NO CGI FOUND !!!!!!!" << std::endl;
	return (status);
}


/* STEPS */

// 1. check if CGI needed

// 2. create 2 pipes -> 1 from server to script, 1 from script to server

// 3. fork process

//		***** Child process *****			|	***** parent process *****

// 4.	prepare execve data					|	close write-end / dup2 read-end

// 5. 	close read-end / dup2 write-end		|	Wait for data

// 6.	run executable with execve			|	Store data in std::string(?) and return

// 7.	In case of errors, close fds and 	|	close fds and set STDIN and STDOUT back
//		free malloc'ed data -> exit(1)?		|

// 8.										|	Create response header and send to client

// 9 .										|	In case of errors, close fds