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

std::string Serve::answerRequest(clRequest& clientRequest) {
	if (clientRequest.invalidRequest){
		// return bad request
	}
	if (clientRequest.methodNotAllowd){
		// return	method not allowed
	}else {
		if (clientRequest.hundredContinue){
			// check if content lenght is less than max body size return 100 continue else return 413 payload
			std::string clContentLengthStr;
			std::string serverMaxBodySizeStr = "1048576";
			int	clContentLengthInt;
			int	serverMaxBodySizeInt;
			if (clientRequest.headers.find("content-length") != clientRequest.headers.end()) {
				clContentLengthStr = clientRequest.headers["content-length"].front();
				if (_serverBlock.values.find("client_max_body_size") != _serverBlock.values.end()) {
					serverMaxBodySizeStr = _serverBlock.values["client_max_body_size"].front();
				}
				try{
					serverMaxBodySizeInt = std::stoi(serverMaxBodySizeStr);
					clContentLengthInt  = std::stoi(clContentLengthStr);
				}catch(const std::exception &e) {
					// internal error 500
					std::cerr << "Error : converstion failed! " << e.what() << std::endl;
				}
				if (clContentLengthInt > serverMaxBodySizeInt) {
					// return 413 payload
				} else {
					// return 100 continue
				}

			}else{
				// return bad request no content length exist
			}
		}else{
			if (clientRequest.method == "GET" ){
				if (fileOrDirectoryExists(_root + clientRequest.path)){
					// path exist
					std::cout << "file found " << std::endl;
				}else {
					std::cout << "file NOT found " << std::endl;
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

