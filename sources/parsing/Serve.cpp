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

std::string resolvePath(const std::string& userPath) {
    return std::filesystem::absolute(userPath).string();
}

bool	fileOrDirectoryExists(std::string path) {
	std::string filepath = resolvePath(path);
	bool res = std::filesystem::exists(filepath);
	return res;

}

void Serve::answerRequest(clRequest& clientRequest) {
	if (clientRequest.invalidRequest){
		// return bad request
	}else {
		if (clientRequest.hundredContinue){
			// check if content lenght is less than max body size return 100 continue else return 413 payload
		}else{
			if (clientRequest.method == "GET" ){
				if (fileOrDirectoryExists(clientRequest.path)){
					// path exist
				}else {
					// 404 
				}
			}
			if (clientRequest.method == "POST" ){
				
			}
			if (clientRequest.method == "DELETE" ){
				
			}
		}
	}
}

