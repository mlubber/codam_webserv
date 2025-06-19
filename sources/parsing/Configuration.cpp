#include "../../headers/Configuration.hpp"
#include <sstream>
#include <iterator>
#include <algorithm>
#include <cstring>

std::set<std::string> validKeywords =
{
	"server", "listen", "server_name", "host", "root", "index",
	"client_max_body_size", "error_page", "location", "methods", "return", "deny",
	"rewrite", "autoindex", "upload_store", "cgi_pass", "allow"
};

Configuration::Configuration() {}

Configuration::Configuration(const Configuration& other)
{
	this->_configData.nested = other._configData.nested;
	this->_configData.values = other._configData.values;
}

ConfigBlock&	Configuration::getConfigData()
{
	return (_configData);
}

ConfigBlock&	Configuration::getServerBlock(const std::string& host, const std::string& port, const std::string& name)
{
	ConfigBlock* fallBackServerBlock = nullptr;
	ConfigBlock* firstMatchServerBlock = nullptr;

	for (std::map<std::string, ConfigBlock>::iterator it = _configData.nested.begin(); it != _configData.nested.end(); ++it)
	{
		std::map<std::string, ConfigBlock>::iterator tempIT = it;
		if (it->first == "server")
		{
			ConfigBlock& serverBlock = it->second;

			if (!fallBackServerBlock)
				fallBackServerBlock = &serverBlock;

			std::string serverHost = serverBlock.values["host"].front();
			std::vector<std::string> listenPorts = serverBlock.values["listen"];
			std::string serverName = serverBlock.values["server_name"].front();

			bool hostMatch = serverHost == host;
			bool portMatch = (std::find(listenPorts.begin(), listenPorts.end(), port) != listenPorts.end());
			bool nameMatch = serverName == name;

			if (hostMatch && portMatch)
			{
				if (++tempIT != _configData.nested.end())
				{
					if (!firstMatchServerBlock)
						firstMatchServerBlock = &serverBlock;
				}
				else if (!firstMatchServerBlock)
					return (serverBlock);
			} 
			if(serverName.size() > 0 && hostMatch && portMatch)
			{
				if (hostMatch && portMatch && nameMatch)
					return (serverBlock);
			}
		}
	}
	if (firstMatchServerBlock)
		return (*firstMatchServerBlock);
	else
		return (*fallBackServerBlock);
	throw std::runtime_error("No server blocks found in the configuration");
}

Configuration& Configuration::operator=(const Configuration& other)
{
	if (this != &other)
	{
		this->_configData.nested.clear();
		this->_configData.values.clear();
		this->_configData.nested = other._configData.nested;
		this->_configData.values = other._configData.values;
	}
	return (*this);
}

Configuration::~Configuration(){}

bool	validateChar(char c)
{
	if (std::isalnum(c))
		return true;
	else if (c == '.' || c == '/' || c == '_' || c == '-' || c == ':')
		return true;
	else
		return false;
}

std::vector<Token> Configuration::tokenize(std::ifstream &file)
{
	std::string line;
	std::vector<Token> tokens;
    std::string word;
    char c;
	while (getline(file, line))
	{
		std::istringstream stream(line);
		while (stream.get(c))
		{
			if (std::isspace(c))
				continue;
			else if (c == '{') 
				tokens.push_back({BLOCK_START, "{"});
			else if (c == '}')
				tokens.push_back({BLOCK_END, "}"});
			else if (c == ';')
				tokens.push_back({SEMICOLON, ";"});
			else if (c == '#') 
				break;
			else 
			{
				word.clear();
				stream.putback(c);
				while(stream.get(c) && !std::isspace(c) && c != '{' && c != '}' && c != ';' && c != '#') 
				{
					if (validateChar(c))
						word += c;
					else 
					{
						std::cerr << "CONFIG ERROR: Invalid character inside config file: ( " << c << " ), in line: " << line << std::endl;
						std::exit(1);
					}
				} 
				if (!stream.eof()) 
					stream.putback(c);
				TokenType type = std::find(validKeywords.begin(), validKeywords.end(), word) != validKeywords.end() ? KEYWORD : VALUE;
				tokens.push_back({type, word});
			}
		}
		line.clear();
	}
	return (tokens);
}

void	checkErrorPage(std::vector<std::string> values)
{
	if (values.size() != 2)
	{
		std::cerr << "CONFIG ERROR: Invalid syntax in config file.(error_page has to have page number and path)!" << std::endl;
		std::exit(1);
	} 
	else 
	{
		if (values[0].size() != 3 || !isdigit(values[0][0]) || !isdigit(values[0][1]) || !isdigit(values[0][2]))
		{
			std::cerr << "CONFIG ERROR: Invalid syntax, (page number should be 3 digits)!" << std::endl;
			std::exit(1);
		}
	}
}


void	callectAllValuesForAKeyPushToVector(std::vector<Token> &chunkTokens, std::stack<std::pair<std::string, ConfigBlock>> &blockStack)
{
	std::string key = chunkTokens[0].value;

	if (std::find(validKeywords.begin(), validKeywords.end(), key) == validKeywords.end()) 
	{
		std::cerr << "CONFIG ERROR: Using invalid keyword in configuration file: " << key << std::endl;
		std::exit(1);
	}

	std::vector<std::string> values;
	
	for (size_t i = 1; i < chunkTokens.size() - 1; ++i)
	{
		if (chunkTokens[i].type == VALUE)
			values.push_back(chunkTokens[i].value);
		else 
		{
			std::cerr << "CONFIG ERROR: Invalid syntax near '" << chunkTokens[i].value << "' in config file." << std::endl;
			std::exit(1);
		}
	}
	if (key == "error_page")
		checkErrorPage(values);
	std::vector<std::string> &existingValues = blockStack.top().second.values[key];
	existingValues.insert(existingValues.end(), values.begin(), values.end());
}


void Configuration::parseConfig(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
	{
        std::cerr << "CONFIG ERROR: Could not open config file: " << filename << std::endl;
		if (errno)
		{
            std::cerr << "CONFIG ERROR: " << std::strerror(errno) << " (errno: " << errno << ")" << std::endl;
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
	while(itb != ite)
	{
		while (itFinder != ite)
		{
			if (itFinder->type == BLOCK_START)
			{
				++itFinder;
				std::vector<Token> chunkTokens(itb, itFinder);
				if (!chunkTokens.empty())
				{
					blockStack.push({chunkTokens[0].value, ConfigBlock()});
					if (chunkTokens.size() > 2)
						callectAllValuesForAKeyPushToVector(chunkTokens, blockStack);
				} 
				else 
				{
					std::cerr << "CONFIG ERROR: Unexpected BLOCK_START without a key." << std::endl;
					std::exit(1);
				}
				break;
			}
			else if (itFinder->type == SEMICOLON) 
			{
				++itFinder;
				std::vector<Token> chunkTokens(itb, itFinder);
				callectAllValuesForAKeyPushToVector(chunkTokens, blockStack);
				break;
			}	
			else if (itFinder->type == BLOCK_END)
			{
				if (blockStack.size() > 1) 
				{
					std::pair<std::string, ConfigBlock> nestedBlock = blockStack.top();
					blockStack.pop();
					blockStack.top().second.nested.insert({nestedBlock.first, std::move(nestedBlock.second)});
				} 
				else
				{
					std::cerr << "CONFIG ERROR: Mismatched closing '}' in config file." << std::endl;
					std::exit(1);
				}
				++itFinder;
				break;
			}
			else 
				++itFinder;
		}
		itb = itFinder;
	}
	if (blockStack.size() > 1)
	{
        std::cerr << "CONFIG ERROR: Unclosed block in config file." << std::endl;
        std::exit(1);
    }

	_configData = std::move(blockStack.top().second); 
}


