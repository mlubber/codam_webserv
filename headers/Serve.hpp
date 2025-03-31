#pragma once
#include "Configuration.hpp"

#include "Request.hpp"

// struct ConfigBlock {
//     std::map<std::string, std::vector<std::string>> values;
//     std::multimap<std::string, ConfigBlock>		nested;
// };

class Serve
{
private:
	ConfigBlock	_serverBlock;
	std::string	_host;
	std::vector<std::string>	_port;
	std::string	_root;
	std::string _server_name;
public:
	Serve(std::string host, std::vector<std::string> port, std::string root, std::string server_name);
	~Serve();
	ConfigBlock& 				getServerBlock();
	std::string&				getHost();
	std::string&				getRoot();
	std::string& 				getServer_name();
	std::vector<std::string>&	getPort();
	void	answerRequest(clRequest& clientRequest);

};


