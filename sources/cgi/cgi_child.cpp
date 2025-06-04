#include "../../headers/headers.hpp"
#include "../../headers/Client.hpp"
#include "../../headers/Server.hpp"

void	get_exe_path(t_cgiData& cgi, const clRequest& cl_request, const Server& server, Client& client)
{
	std::string path;
	const std::string& root = server.getServerInfo(2);
	if (root[0] != '.')
		path = "." + root + cl_request.path;
	else
		path = root + cl_request.path;
	cgi.path = new char[path.size() + 1];
	std::strcpy(cgi.path, path.c_str());
	(void)client;
}

void	get_exe(t_cgiData& cgi, const clRequest& cl_request)
{
	cgi.exe = new char*[2]();
	int i = cl_request.path.find_last_of('/'); 
	std::string exe = cl_request.path.substr(i + 1);
	cgi.exe[0] = new char[exe.length() + 1];
	std::strcpy(cgi.exe[0], exe.c_str());
}

char* create_env_ptr(std::string key, std::string value)
{
	for (char& c : key) c = toupper(c);
	std::string newstr = key + "=" + value;
	char* str = new char[newstr.length() + 1];
	std::strcpy(str, newstr.c_str());
	return (str);
}

std::string	get_header_data(const clRequest& cl_request, std::string header, int i)
{
	for (const auto& pair : cl_request.headers)
	{
		// std::cout << "Header data: " << pair.first << std::endl;
		if (pair.first == header)
		{
			for (const auto& value : pair.second)
				return (value);
			break ;
		}
	}
	if (i == 0)
		return ("0");
	else
		return ("\"\"");
}

std::string get_path_info(const clRequest& cl_request, const Server& server, Client& client)
{
	std::string root = client.getServerBlockInfo("root");
	if (root[0] != '/')
		root = "/" + root;
	std::string path_info = getenv("PWD") + root + cl_request.path;
	(void)server;
	return (path_info);
}

void	setup_environment(t_cgiData& cgi, const clRequest& cl_request, const Server& server, Client& client)
{
	cgi.envp = new char*[11]();

	cgi.envp[0] = create_env_ptr("REQUEST_METHOD", cl_request.method);
	cgi.envp[1] = create_env_ptr("SCRIPT_NAME", cl_request.path);
	cgi.envp[2] = create_env_ptr("SERVER_NAME", client.getServerBlockInfo("host"));
	cgi.envp[3] = create_env_ptr("SERVER_PORT", client.getServerBlockInfo("listen"));
	cgi.envp[4] = create_env_ptr("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.envp[5] = create_env_ptr("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.envp[6] = create_env_ptr("CONTENT_LENGTH", get_header_data(cl_request, "content-length", 0));
	cgi.envp[7] = create_env_ptr("CONTENT_TYPE", get_header_data(cl_request, "content-type", 1));
	if (!cl_request.queryStr.empty())
		cgi.envp[8] = create_env_ptr("QUERY_STRING", cl_request.queryStr);
	else
		cgi.envp[8] = create_env_ptr("QUERY_STRING", "\"\"");
	cgi.envp[9] = create_env_ptr("PATH_INFO", get_path_info(cl_request, server, client));



	//  Test printing environment
	std::cout << "\n--- CGI Environment ---" << std::endl;
	for (int i = 0; i < 10; ++i) {
		if (cgi.envp[i] == nullptr)
			std::cout << "null\n";
		else
			std::cout << cgi.envp[i] << "\n";
	} std::cout << std::endl;
	//  End test printing environment
	std::cout << "ACTUAL SERVER_NAME=" << client.getServerBlockInfo("host") << std::endl;
}

static int	setting_fds(t_cgiData& cgi)
{
	if (dup2(cgi.ets_pipe[1], STDOUT_FILENO) == -1)
	{
		std::cerr << "CGI ERROR: Failed dup2 in child process" << std::endl;
		return (1);
	}
	if (close(cgi.ets_pipe[0]) == -1)
		std::cerr << "CGI ERROR: Failed closing read-end pipe in child" << std::endl;
	cgi.ets_pipe[0] = -1;
	if (cgi.ste_pipe[0] != -1)
	{
		if (dup2(cgi.ste_pipe[0], STDIN_FILENO) == -1)
		{
			std::cerr << "CGI ERROR: Failed dup2 in child process" << std::endl;
			return (1);
		}
	}
	if (cgi.ste_pipe[1] != -1 && close(cgi.ste_pipe[1]) == -1)
		std::cerr << "CGI ERROR: Failed closing write-end pipe in child" << std::endl;
	cgi.ste_pipe[1] = -1;
	return (0);
}

void	cgi_child_process(t_cgiData& cgi, const clRequest& cl_request, const Server& server, Client& client)
{
	try
	{
		setup_environment(cgi, cl_request, server, client);
		get_exe_path(cgi, cl_request, server, client);
		get_exe(cgi, cl_request);
	}
	catch(const std::exception& e)
	{
		std::cerr << "CGI ERROR: Failed setting up child process" << std::endl;
		cgi_cleanup(cgi, true);
	}

	if (setting_fds(cgi))
		cgi_cleanup(cgi, true);
	if (access(cgi.path, F_OK | X_OK) != 0)
	{
		std::cerr << "CGI ERROR: " << cgi.path << ": Path to script not found or invalid permissions" << std::endl;
		cgi_cleanup(cgi, true);
	}
	execve(cgi.path, cgi.exe, cgi.envp);
	cgi_cleanup(cgi, true);
}
