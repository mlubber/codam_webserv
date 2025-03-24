#include "../../headers/Server.hpp"

void	cgi_child_process(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	// setup
	cgi_setup(cgi, request, server);

	// Check if file exists and permissions
	if (access(cgi->path, F_OK | X_OK) != 0)
	{
		std::cerr << "CGI ERROR: " << cgi->path << ": Path to script not found or invalid permissions" << std::endl;
		cgi_cleanup(cgi, true);
	}


	// Execve
	execve(cgi->path, cgi->exe, cgi->envp);



	// cleanup & exit if execve failed
	cgi_cleanup(cgi, true);
}
