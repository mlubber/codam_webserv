#include "../../headers/Configuration.hpp"
#include <sstream>
#include <iterator>
#include <algorithm>
#include <cstring>

std::set<std::string> validKeywords = {
	"server", "listen", "server_name", "host", "root", "index",
	"client_max_body_size", "error_page", "location", "methods", "return", "deny",
	"rewrite", "autoindex", "upload_store", "cgi_pass", "allow"
};

Configuration::Configuration() {}

Configuration::Configuration(const Configuration& other) {
	this->_configData.nested = other._configData.nested;
	this->_configData.values = other._configData.values;
}

ConfigBlock&	Configuration::getConfigData(){
	return (_configData);
}

ConfigBlock&	Configuration::getServerBlock(const std::string& host, const std::string& port) {
	for (std::map<std::string, ConfigBlock>::iterator it = _configData.nested.begin(); it != _configData.nested.end(); ++it) {
		if (it->first == "server") {
			ConfigBlock& serverBlock = it->second;
			std::vector<std::string> serverHosts = serverBlock.values["host"];
			std::vector<std::string> listenPorts = serverBlock.values["listen"];

			if ((std::find(serverHosts.begin(), serverHosts.end(), host) != serverHosts.end() || host == "localhost") &&
				std::find(listenPorts.begin(), listenPorts.end(), port) != listenPorts.end()) {
				return (serverBlock);
			}
		}
	}
	throw std::runtime_error("No matching server block found for host: " + host + " and port: " + port);
}

Configuration& Configuration::operator=(const Configuration& other) {
	if (this != &other) {
		this->_configData.nested.clear();
		this->_configData.values.clear();
		this->_configData.nested = other._configData.nested;
		this->_configData.values = other._configData.values;
	}
	return (*this);
}


Configuration::~Configuration(){}

bool	validateChar(char c){
	if (std::isalnum(c))
		return true;
	else if (c == '.' || c == '/' || c == '_' || c == '-' || c == ':')
		return true;
	else
		return false;
}


std::vector<Token> Configuration::tokenize(std::ifstream &file) {
	std::string line;
	std::vector<Token> tokens;
    std::string word;
    char c;
	while (getline(file, line)){
		std::istringstream stream(line);
		while (stream.get(c)) {
			if (std::isspace(c)) { continue;}
			else if (c == '{') { tokens.push_back({BLOCK_START, "{"}); }
			else if (c == '}') { tokens.push_back({BLOCK_END, "}"}); }
			else if (c == ';') { tokens.push_back({SEMICOLON, ";"}); }
			else if (c == '#') {break;}
			else {
				word.clear();
				stream.putback(c);
				while(stream.get(c) && !std::isspace(c) && c != '{' && c != '}' && c != ';' && c != '#') {
					if (validateChar(c))
						word += c;
					else {
						std::cerr << "Error: invalid character inside config file: ( " << c << " ), in line: " << line << std::endl;
						exit(1);
					}
				} if (!stream.eof()) 
					stream.putback(c);
				TokenType type = std::find(validKeywords.begin(), validKeywords.end(), word) != validKeywords.end() ? KEYWORD : VALUE;
				tokens.push_back({type, word});
			}
		}
		line.clear();
	}
	return (tokens);
}

void	checkErrorPage(std::vector<std::string> values) {
	if (values.size() != 2){
		std::cerr << "Error: Invalid syntax in config file.(error_page has to have page number and path)!" << std::endl;
		std::exit(1);
	} else {
		if (values[0].size() != 3 || !isdigit(values[0][0]) || !isdigit(values[0][1]) || !isdigit(values[0][2])) {
			std::cerr << "Error: Invalid syntax, (page number should be 3 digits)!" << std::endl;
			std::exit(1);
		}
	}
}


void	callectAllValuesForAKeyPushToVector(std::vector<Token> &chunkTokens, std::stack<std::pair<std::string, ConfigBlock>> &blockStack) {
	std::string key = chunkTokens[0].value;

	if ( std::find(validKeywords.begin(), validKeywords.end(), key) == validKeywords.end() ) {
		std::cerr << "Error: Using invalid keyword in configuration file: " << key << std::endl;
		std::exit(1);
	}

	std::vector<std::string> values;
	
	for (size_t i = 1; i < chunkTokens.size() - 1; ++i) { // Collect all values before ;
		if (chunkTokens[i].type == VALUE)
			values.push_back(chunkTokens[i].value);
		else {
			std::cerr << "Error: Invalid syntax near '" << chunkTokens[i].value << "' in config file." << std::endl;
			std::exit(1);
		}
	}
	if (key == "error_page")
		checkErrorPage(values);
	std::vector<std::string> &existingValues = blockStack.top().second.values[key];
	existingValues.insert(existingValues.end(), values.begin(), values.end());
}


void Configuration::parseConfig(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filename << std::endl;
		if (errno) {
            std::cerr << "System error: " << std::strerror(errno) << " (errno " << errno << ")" << std::endl;
        }
        std::exit(1);
    }

    std::stack<std::pair<std::string, ConfigBlock>> blockStack;
    blockStack.push({"root", ConfigBlock()});
	std::vector<Token> tokens = tokenize(file);
	file.close();

	std::vector<Token>::iterator itb = tokens.begin();
	std::vector<Token>::iterator itFinder = tokens.begin();
	std::vector<Token>::iterator ite = tokens.end();
	while(itb != ite) {
		while (itFinder != ite) {
			if (itFinder->type == BLOCK_START) {
				++itFinder;
				std::vector<Token> chunkTokens(itb, itFinder);
				if (!chunkTokens.empty()) {
					blockStack.push({chunkTokens[0].value, ConfigBlock()});
					if (chunkTokens.size() > 2)
						callectAllValuesForAKeyPushToVector(chunkTokens, blockStack);
				} else {
					std::cerr << "Error: Unexpected BLOCK_START without a key." << std::endl;
					std::exit(1);
				}
				
				break;}
			else if (itFinder->type == SEMICOLON) {
				++itFinder;
				std::vector<Token> chunkTokens(itb, itFinder);
				callectAllValuesForAKeyPushToVector(chunkTokens, blockStack);
				
				break;}	
			else if (itFinder->type == BLOCK_END) {

				if (blockStack.size() > 1) {
					std::pair<std::string, ConfigBlock> nestedBlock = blockStack.top();
					blockStack.pop();
					blockStack.top().second.nested.insert({nestedBlock.first, std::move(nestedBlock.second)});
				} else {
					std::cerr << "Error: Mismatched closing '}' in config file." << std::endl;
					std::exit(1);
				}
				++itFinder;
				break;}
			else {++itFinder;}
		}
		itb = itFinder;
	}
	if (blockStack.size() > 1) {
        std::cerr << "Error: Unclosed block in config file." << std::endl;
        std::exit(1);
    }

	_configData = std::move(blockStack.top().second); 
}

// only print to see values and keys doesn't need it later 
void Configuration::printConfig(const ConfigBlock &config, int depth) {
    std::string indent(depth * 2, ' ');

    for (const std::pair<const std::string, std::vector<std::string>> &pair : config.values) {
        std::cout << indent << pair.first << " = ";
		std::copy(pair.second.begin(), pair.second.end(), std::ostream_iterator<std::string>(std::cout, ", "));
		std::cout << std::endl;
    }
    for (const std::pair<const std::string, ConfigBlock> &block : config.nested) {
        std::cout << indent << "Block: " << block.first << " {" << std::endl;
        printConfig(block.second, depth + 1);
        std::cout << indent << "}" << std::endl;
    }
}

