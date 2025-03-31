#include "../../headers/Serve.hpp"

Serve::Serve(std::string host, std::vector<std::string> port, std::string root, std::string server_name) 
	: _host(host), _port(port), _root(root), _server_name(server_name) {}

Serve::~Serve()
{
}

ConfigBlock& Serve::getServerBlock() {
	return _serverBlock;
}

std::string&	Serve::getHost(){
	return _host;
}
std::vector<std::string>&	Serve::getPort(){
	return _port;
}
std::string&	Serve::getRoot(){
	return _root;
}
std::string& 	Serve::getServer_name(){
	return _server_name;
}
