#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <stack>
#include <set>


enum TokenType { KEYWORD, VALUE, BLOCK_START, BLOCK_END, SEMICOLON };

struct Token {
    TokenType type;
    std::string value;
};


struct ConfigBlock {
    std::map<std::string, std::vector<std::string>> values; // Change std::string to std::vector<std::string>
    std::multimap<std::string, ConfigBlock>			nested;
};


class Configuration
{
private:
	ConfigBlock _configData;
	
public:
	Configuration();
	~Configuration();
	Configuration& operator=(const Configuration& other);
	Configuration(const Configuration& other);

	std::vector<Token>			tokenize(std::ifstream &line);
	void						parseConfig(const std::string &filename);
	void						printConfig(const ConfigBlock &config, int depth);
	ConfigBlock&				getConfigData();
	std::vector<std::string>	getConfigValues(ConfigBlock& config, const std::string& key);
};
