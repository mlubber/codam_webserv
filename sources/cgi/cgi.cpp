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
		std::cerr << "CGI ERROR: closing ste_pipe[1] failed" << std::endl;

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
		std::exit(errno);
	}
}

static bool	init_cgi_struct(Client& client, clRequest& cl_request, const Server& server)
{
	std::unique_ptr<t_cgiData> cgi = std::make_unique<t_cgiData>();

	cgi->path = nullptr;
	cgi->exe = nullptr;
	cgi->envp = nullptr;
	cgi->ets_pipe[0] = -1;
	cgi->ets_pipe[1] = -1;
	cgi->ste_pipe[0] = -1;
	cgi->ste_pipe[1] = -1;
	cgi->child_pid = -1;
	cgi->started_reading = false;

	if (pipe(cgi->ets_pipe) == -1)
		return (1);
	setNonBlocking(cgi->ets_pipe[0]);
	struct epoll_event ets_pipe;
	ets_pipe.events = EPOLLIN;
	ets_pipe.data.fd = cgi->ets_pipe[0];
	if (epoll_ctl(server.getEpollFd(), EPOLL_CTL_ADD, cgi->ets_pipe[0], &ets_pipe) == -1)
	{
		cgi_cleanup(*cgi, false);
		return (1);
	}
	client.addFd(cgi->ets_pipe[0]);

	// Only needed to set when method is POST
	if (cl_request.method == "POST")
	{
		cgi->started_writing = false;
		cgi->dataWritten = 0;
		cgi->dataToWrite = cgi->writeData.size();
		if (pipe(cgi->ste_pipe) == -1)
		{
			if (close(cgi->ets_pipe[0]) == -1 || close(cgi->ets_pipe[1]) == -1)
				std::cerr << "CGI ERROR: Failed closing ets_pipe in parent" << std::endl;
			cgi->ets_pipe[0] = -1;
			cgi->ets_pipe[0] = -1;
			return (1);
		}
		std::cout << "STE PIPE: " << cgi->ste_pipe[0] << " & " << cgi->ste_pipe[1] << std::endl;
		setNonBlocking(cgi->ste_pipe[1]);
		struct epoll_event ste_pipe;
		ste_pipe.events = EPOLLIN;
		ste_pipe.data.fd = cgi->ste_pipe[0];
		if (epoll_ctl(server.getEpollFd(), EPOLL_CTL_ADD, cgi->ste_pipe[1], &ste_pipe) == -1)
		{
			cgi_cleanup(*cgi, false);
			return (1);
		}
		client.addFd(cgi->ste_pipe[1]);
	}

	client.setCgiStruct(std::move(cgi));
	return (0);
}

static int	create_cgi_child_process(t_cgiData& cgi, const clRequest& cl_request, const Server& server)
{
	cgi.child_pid = fork();
	if (cgi.child_pid == -1)
	{
		std::cerr << "CGI ERROR: Fork() failed creating child process" << std::endl;
		cgi_cleanup(cgi, false);
		return (2);
	}
	if (cgi.child_pid == 0)
		cgi_child_process(cgi, cl_request, server);
	return (0);
}

static int	closing_parent_fds(t_cgiData& cgi)
{
	if (close(cgi.ets_pipe[1]) == -1)
		std::cerr << "CGI ERROR: Failed closing read-end pipe in child" << std::endl;
	cgi.ets_pipe[1] = -1;
	if (cgi.ste_pipe[0] != -1 && close(cgi.ste_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing write-end pipe in child" << std::endl;
	cgi.ste_pipe[0] = -1;
	return (0);
}

int	start_cgi(clRequest& cl_request, const Server& server, Client& client)
{
	cl_request.cgi = true;
	if (init_cgi_struct(client, cl_request, server))
		return (2);
	if (create_cgi_child_process(client.getCgiStruct(), cl_request, server) != 0)
		return (2);
	if (closing_parent_fds(client.getCgiStruct()))
		return (2);
	if (cl_request.method == "POST")
		client.setClientState(cgi_write);
	else
		client.setClientState(cgi_read);
	return (0);
}

bool	cgi_check(std::string& path)
{
	if (path.compare(0, 9, "/cgi-bin/") == 0)
		return (true);
	else if (path.length() > 3)
	{
		size_t length = path.length();
		if (path.compare(length - 3, length, ".py") == 0)
			return (true);
		else if (length > 4 && path.compare(length - 3, length, ".php") == 0)
			return (true);
	}
	return (false);
}
