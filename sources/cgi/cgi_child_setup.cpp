#include "../../headers/headers.hpp"
#include "../../headers/Client.hpp"
#include "../../headers/Server.hpp"

void	get_exe_path(t_cgiData& cgi, const clRequest& request, const Server& server)
{
	std::string path;
	const std::string& root = server.getServerInfo(2);
	if (root[0] != '.')
		path = "." + root + request.path;
	else
		path = root + request.path;
	cgi.path = new char[path.size() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(cgi.path, path.c_str());
}

void	get_exe(t_cgiData& cgi, const clRequest& request)
{
	cgi.exe = new char*[2](); // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	int i = request.path.find_last_of('/'); 
	std::string exe = request.path.substr(i + 1);
	cgi.exe[0] = new char[exe.length() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(cgi.exe[0], exe.c_str());
}

char* create_env_ptr(std::string key, std::string value)
{
	for (char& c : key) c = toupper(c);
	std::string newstr = key + "=" + value;
	char* str = new char[newstr.length() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(str, newstr.c_str());
	return (str);
}

// std::string	get_header_data(const clRequest& request, std::string header, int i)
// {
// 	// auto it = request.headers.find(header);
// 	// if (it != request.headers.end()) 		// Go through map in clRequest struct
// 	// 	return (it->second);
// 	// else if (i == 0)
// 	// 	return ("0");
// 	// else
// 		return ("\"\"");
// }

std::string get_path_info(const clRequest& request, const Server& server)
{
	std::string root = server.getServerInfo(2);
	if (root[0] != '/')
		root = "/" + root;
	std::string path_info = getenv("PWD") + root + request.path;


	// testing
	char* str = new char[path_info.length() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(str, path_info.c_str());
	if (access(str, F_OK | X_OK) == 0)
		std::cout << "get_path_info file was found and is executable" << std::endl;
	else
	{
		std::cout << "Checking errno in child for permission, this is errno: " << errno << std::endl;
		std::cout << "get_path_info file was not found!!!!!!!!!!!1" << std::endl;
	}
	// end of testing


	return (path_info);
}

void	setup_environment(t_cgiData& cgi, const clRequest& request, const Server& server)
{
	cgi.envp = new char*[11](); // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs

	cgi.envp[0] = create_env_ptr("REQUEST_METHOD", request.method);
	cgi.envp[1] = create_env_ptr("SCRIPT_NAME", request.path);
	cgi.envp[2] = create_env_ptr("SERVER_NAME", server.getServerInfo(0));
	cgi.envp[3] = create_env_ptr("SERVER_PORT", server.getServerInfo(1));
	cgi.envp[4] = create_env_ptr("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.envp[5] = create_env_ptr("GATEWAY_INTERFACE", "CGI/1.1");
	// cgi.envp[6] = create_env_ptr("CONTENT_LENGTH", get_header_data(request, "Content-Length", 0));
	// cgi.envp[7] = create_env_ptr("CONTENT_TYPE", get_header_data(request, "Content-Type", 1));
	// cgi.envp[8] = create_env_ptr("QUERY_STRING", get_header_data(request, "Query-String", 1));
	cgi.envp[9] = create_env_ptr("PATH_INFO", get_path_info(request, server));



	//  Test printing environment
	std::cout << "\n--- CGI Environment ---" << std::endl;
	for (int i = 0; i < 10; ++i) {
		if (cgi.envp[i] == nullptr)
			std::cout << "null\n";
		else
			std::cout << cgi.envp[i] << "\n";
	} std::cout << std::endl;
	//  End test printing environment
}

bool	cgi_setup(t_cgiData& cgi, const clRequest& request, const Server& server)
{
	setup_environment(cgi, request, server);
	get_exe_path(cgi, request, server);
	get_exe(cgi, request);

	dup2(cgi.ets_pipe[1], STDOUT_FILENO); // add error handling
	close(cgi.ets_pipe[0]); // add error handling

	return (true);
}