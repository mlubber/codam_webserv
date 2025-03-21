#include "../headers/Serve.hpp"



Serve::~Serve(){}

Serve::Serve(ConfigBlock &config) : _configData(config) {}

Serve::Serve(const Serve& other) : _configData(other._configData) {}

Serve& Serve::operator=(const Serve& other){
	if (this != &other) {
		this->_configData.nested.clear();
		this->_configData.values.clear();
		this->_configData.nested = other._configData.nested;
		this->_configData.values = other._configData.values;
	}
	return (*this);
}


void	Serve::answerRequest(std::string host, std::string port, std::string error_page, std::string index) {
	error_page = "";
	index = "";
	
	// which server block
	std::string configHost;
	std::string configPort;
	int i = 1;
	int serverNumber = 0;
	for (const std::pair<const std::string, ConfigBlock> &server : _configData.nested) {
		bool	isHost = false;
		bool	isPort = false;
		for (const std::pair<const std::string, std::vector<std::string>> &value : server.second.values) {
			if (value.first == "host") {
				configHost = value.second.front();
				if (configHost.compare(host) == 0) { isHost = true; }
			}
			if (value.first == "listen") {
				configPort = value.second.front();
				if (configPort.compare(port) == 0) { isPort = true; }
			}
		}
		if (isHost && isPort) { 
			std::cout << "server block: " << i << " has to respone to the request." << std::endl;
			serverNumber = i;
		}
		++i;

	}
}
