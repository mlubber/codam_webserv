#include "../../headers/Server.hpp"

void	get_exe_path(t_cgiData* cgi, const HttpRequest& request)
{
	cgi->path = new char[request.path.size() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(cgi->path, request.path.c_str());
	std::cout << "CGI->PATH after conversion to char*: " <<cgi->path << std::endl;
}

void	get_exe(t_cgiData* cgi, const HttpRequest& request)
{
	cgi->exe = new char*[2]();
	int i = request.path.find_last_of('/');
	std::string exe = request.path.substr(i);
	std::cout << "PATHHHHHH: " << request.path << std::endl;


	cgi->exe[0] = new char[exe.length()];
	std::strcpy(cgi->exe[0], exe.c_str());
	std::cout << "Executable c_str: " << cgi->exe[0] << std::endl;
}

char* create_env_ptr(std::string key, std::string value)
{
	for (char& c : key) c = toupper(c);
	std::string newstr = key + "=" + value;
	char* str = new char[newstr.length() + 1]; // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs
	std::strcpy(str, newstr.c_str());
	return (str);
}

std::string	get_header_data(const HttpRequest& request, std::string header, int i)
{
	auto it = request.headers.find(header);
	if (it != request.headers.end())
		return (it->second);
	else if (i == 0)
		return ("0");
	else
		return ("\"\"");
}

void	setup_environment(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	cgi->envp = new char*[10](); // Need to be in try/catch block  OR new(std::nothrow_t) to check for nullptrs

	cgi->envp[0] = create_env_ptr("REQUEST_METHOD", request.method);
	cgi->envp[1] = create_env_ptr("SCRIPT_NAME", request.path);
	cgi->envp[2] = create_env_ptr("SERVER_NAME", server.getServerInfo(0));
	cgi->envp[3] = create_env_ptr("SERVER_PORT", server.getServerInfo(1));
	cgi->envp[4] = create_env_ptr("SERVER_PROTOCOL", "HTTP/1.1");
	cgi->envp[5] = create_env_ptr("GATEWAY_INTERFACE", "CGI/1.1");
	cgi->envp[6] = create_env_ptr("CONTENT_LENGTH", get_header_data(request, "Content-Length", 0));
	cgi->envp[7] = create_env_ptr("CONTENT_TYPE", get_header_data(request, "Content-Type", 1));
	cgi->envp[8] = create_env_ptr("QUERY_STRING", get_header_data(request, "Query-String", 1));



	//  Test printing environment
	for (int i = 0; i < 10; ++i) {
		if (cgi->envp[i] == nullptr)
			std::cout << "null\n";
		else
			std::cout << cgi->envp[i] << "\n";
	} std::cout << std::endl;
	//  End test printing environment
}

bool	cgi_setup(t_cgiData* cgi, const HttpRequest& request, const Server& server)
{
	setup_environment(cgi, request, server);
	get_exe_path(cgi, request);
	get_exe(cgi, request);
	return (true);
}