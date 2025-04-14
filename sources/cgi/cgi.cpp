#include "../../headers/headers.hpp"
#include "../../headers/Client.hpp"
#include "../../headers/Server.hpp"

void	cgi_cleanup(t_cgiData& cgi, bool child)
{
	if (cgi.ets_pipe[0] != -1 && close(cgi.ets_pipe[0]))
		std::cerr << "CGI ERROR: closing ets_pipe[0] failed" << std::endl;
	if (cgi.ets_pipe[1] != -1 && close(cgi.ets_pipe[1]))
		std::cerr << "CGI ERROR: closing ets_pipe[1] failed" << std::endl;
	if (cgi.ste_pipe[0] != -1 && close(cgi.ste_pipe[0]))
		std::cerr << "CGI ERROR: closing ste_pipe[0] failed" << std::endl;
	if (cgi.ste_pipe[1] != -1 && close(cgi.ste_pipe[1]))
		std::cerr << "CGI ERROR: closing ets_pipe[1] failed" << std::endl;

	if (child == true)
	{
		if (cgi.path != nullptr)
			delete[] cgi.path;
		if (cgi.exe != nullptr && cgi.exe[0] != nullptr)
			delete[] cgi.exe[0];
		if (cgi.exe != nullptr)
			delete[] cgi.exe;
		for (int i = 0; i < 10; i++)
		{
			if (cgi.envp[i] != nullptr)
				delete[] cgi.envp[i];
		}
		if (cgi.envp != nullptr)
			delete[] cgi.envp;
		exit(errno); // Exit actually not allowed
	}
}

static bool	init_cgi_struct(Client& client, clRequest& request, Server& server)
{
	std::unique_ptr<t_cgiData> cgi = std::make_unique<t_cgiData>();

	cgi->path = nullptr;
	cgi->exe = nullptr;
	cgi->envp = nullptr;
	cgi->ets_pipe[0] = -1;
	cgi->ets_pipe[1] = -1;
	cgi->ste_pipe[0] = -1;
	cgi->ste_pipe[1] = -1;

	if (pipe(cgi->ets_pipe) == -1)
		return (1);
	server.setNonBlocking(cgi->ets_pipe[0]);
	struct epoll_event ets_pipe;
	ets_pipe.events = EPOLLIN | EPOLLET;
	ets_pipe.data.fd = cgi->ets_pipe[0];
	if (epoll_ctl(server.getEpollFd(), EPOLL_CTL_ADD, cgi->ets_pipe[0], &ets_pipe) == -1)
	{
		cgi_cleanup(*cgi, false);
		return (1);
	}

	if (request.method == "POST")
	{
		if (pipe(cgi->ste_pipe) == -1)
		{
			if (close(cgi->ets_pipe[0]) == -1 || close(cgi->ets_pipe[0]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets_pipe in parent" << std::endl;
			return (1);
		}
		server.setNonBlocking(cgi->ste_pipe[1]);
		struct epoll_event ste_pipe;
		ste_pipe.events = EPOLLIN | EPOLLET;
		ste_pipe.data.fd = cgi->ste_pipe[0];
		if (epoll_ctl(server.getEpollFd(), EPOLL_CTL_ADD, cgi->ste_pipe[1], &ste_pipe) == -1)
		{
			cgi_cleanup(*cgi, false);
			return (1);
		}
	}
	client.setCgiStruct(std::move(cgi));
	return (0);
}

static int	create_cgi_process(t_cgiData& cgi, clRequest& request, const Server& server)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{
		cgi_cleanup(cgi, false);
		return (-1);
	}
	if (pid == 0)
		cgi_child_process(cgi, request, server);
	return (cgi_parent_process(cgi, request, server, pid));
}

int	cgi_check(clRequest& request, Server& server, Client& client)
{
	int	status;

	if (request.path.compare(0, 9, "/cgi-bin/") == 0)
	{
		errno = 0; // temporary, till bug somewhere else is found
		request.cgi = true;
		if (init_cgi_struct(client, request, server))
			return (errno);
		status = create_cgi_process(client.getCgiStruct(), request, server);
		std::cout << "CGI STATUS: " << status << std::endl;
		errno = 0;
		return (status);
	}
	return (-1);
}


/* STEPS */

// 1. check if CGI needed	✓

// 2. create 2 pipes -> 1 from server to script, 1 from script to server	✓

// 3. fork process	✓

//		***** Child process *****			|	***** parent process *****

// 4.	create environment array  ✓			|	close write-end

// 5. 	close read-end / dup2 write-end		|	Wait for data and read from pipe

// 6.	run executable with execve			|	Store data in std::string(?) and return

// 7.										|	Create response header and send to client


// Errors

//		In case of errors, close fds and	| 	In case of errors, close fds
//		free malloc'ed data -> exit(1)?