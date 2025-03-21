#include "../../headers/Server.hpp"

void	cgi_child_process(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	// setup
	cgi_setup(cgi, request, server);



	// execve
	// std::cout << "execve Path: " << cgi->path << std::endl;
	// std::cout << "execve exe[0]: " << cgi->exe[0] << std::endl;
	// if (cgi->exe[1] == nullptr)
	// 	std::cout << "execve exe[1]: nullptr" << std::endl;
	if (access(cgi->path, F_OK | X_OK) != 0)
	{
		std::cout << "Path to python script was not found and executable" << std::endl;
		// return that script or whatever was not found and handle error
	}
	// std::cout << "Path to python script is found and executable" << std::endl;
	std::cout << errno << std::endl;

	execve(cgi->path, cgi->exe, cgi->envp);


	std::cout << "execve failed" << std::endl;



	// cleanup / exit if execve failed
	cgi_cleanup(cgi);
}
