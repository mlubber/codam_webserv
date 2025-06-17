#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <stack>
#include <set>
#include <filesystem>


enum TokenType
{
	KEYWORD,
	VALUE,
	BLOCK_START,
	BLOCK_END,
	SEMICOLON
};

struct Token
{
    TokenType type;
    std::string value;
};


struct ConfigBlock
{
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

		ConfigBlock&				getConfigData();
		ConfigBlock&				getServerBlock(const std::string& host, const std::string& port, const std::string& name);
	
		std::vector<Token>			tokenize(std::ifstream &line);
		void						parseConfig(const std::string &filename);
};
