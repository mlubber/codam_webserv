#pragma once
#include "Configuration.hpp"
#include "Server.hpp"
#include "Client.hpp"

struct clRequest;

class Host
{
	
	public:
	
		Host(std::string host, std::vector<std::string> port, std::string root, std::string server_name);
		~Host();
	
		ConfigBlock& 				getServerBlock();
		std::string&				getHost();
		std::string&				getRoot();
		std::string& 				getServer_name();
		std::vector<std::string>&	getPort();
	
	private:

		ConfigBlock					_serverBlock;
		std::string					_host;
		std::vector<std::string>	_port;
		std::string					_root;
		std::string					_server_name;
};


