#include "../../headers/Host.hpp"

Host::Host(std::string host, std::vector<std::string> port, std::string root, std::string server_name) 
	: _host(host), _port(port), _root(root), _server_name(server_name) 
{

}

Host::~Host()
{
	
}

ConfigBlock& Host::getServerBlock()
{
	return (_serverBlock);
}

std::string&	Host::getHost()
{
	return (_host);
}
std::vector<std::string>&	Host::getPort()
{
	return (_port);
}
std::string&	Host::getRoot()
{
	return (_root);
}
std::string& 	Host::getServer_name()
{
	return (_server_name);
}


