#include "../../headers/Server.hpp"

void	cgi_child_process(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	// setup
	cgi_setup(cgi, request, server);

	// execve
	execve(cgi->path, cgi->exe, cgi->envp);

	std::cout << "execve failed" << std::endl;

	// cleanup / exit
	cgi_cleanup(cgi);
}
