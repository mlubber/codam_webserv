#include "../headers/ValidationConfigFile.hpp"
#include <iterator>
#include <algorithm>

std::vector<std::map<std::string, std::vector<std::string>>> compareArray;

ValidationConfigFile::~ValidationConfigFile(){}

ValidationConfigFile::ValidationConfigFile(const ValidationConfigFile& other) : _configData(other._configData) {}

ValidationConfigFile& ValidationConfigFile::operator=(const ValidationConfigFile& other) 
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

void isSeversSame()
{
    for (size_t i = 0; i < compareArray.size(); ++i)
	{
        std::map<std::string, std::vector<std::string>>& s1 = compareArray[i];
        for (const auto& entry1 : s1)
		{
            const std::string& host1 = entry1.first;
            const std::vector<std::string>& listen1 = entry1.second;
			const std::string& serverName1 = *(entry1.second.end() - 1);

            for (size_t j = i + 1; j < compareArray.size(); ++j)
			{
                std::map<std::string, std::vector<std::string>>& s2 = compareArray[j];
                for (const auto& entry2 : s2)
				{
                    const std::string& host2 = entry2.first;
                    const std::vector<std::string>& listen2 = entry2.second;
					const std::string& serverName2 = *(entry2.second.end() - 1);

                    if (host1 == host2)
					{
                        for (size_t k = 0; k < listen1.size() - 1; ++k)
						{
                            std::string lis1 = listen1[k];
                            for (size_t l = 0; l < listen2.size() - 1; ++l)
							{
                                std::string lis2 = listen2[l];
                                if (lis1 == lis2)
								{
									if (serverName1 == serverName2) 
									{
										std::cerr << "Error: servers can't have the same host!" << std::endl;
                                    	std::exit(1);
									}

                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void	onlyPrintErrorExit(std::string key, int i) 
{
	if (i == 1)
		std::cerr << "Error: unknown methods or invalid! => " << key << std::endl;
	if (i == 2)
		std::cerr << "Error: ( " << key << " ) should exist and have a value!" << std::endl;
	if (i == 3)
		std::cerr << "Error: ( " << key << " ) should contain only didgit !" << std::endl;
	if (i == 4)
		std::cerr << "Error: ( " << key << " ) has to have correct format => (0 - 255 and 3 dot !) 127.0.0.0" << std::endl;
	if (i == 5)
		std::cerr << "Error: ( " << key << " ) has incorrect format !" << std::endl;
	std::exit(1);
}

void	checkBodySize(std::string &bodySize) 
{
	size_t i, sizeInByte;
	char last = bodySize.back();
	std::string	strNum;
	if (isdigit(last))
	{
		strNum = bodySize;
		if (!std::all_of(strNum.begin(), strNum.end(), [](unsigned char c) {return std::isdigit(c);})) 
			onlyPrintErrorExit("client_max_body_size", 5);
		try
		{
			i = std::stoul(strNum);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			std::cerr << "Error: can't convert the clint body size to long! => " << bodySize << std::endl;
			std::exit(1);
		}
	}
	else 
	{
		strNum = bodySize.substr(0, bodySize.size() - 1);
		if (!std::all_of(strNum.begin(), strNum.end(), [](unsigned char c) {return std::isdigit(c);})) 
			onlyPrintErrorExit("client_max_body_size", 5);
		try
		{
			i = std::stoul(strNum);
		}
		catch(const std::exception& e)
		{
			std::cerr << "Error:" << e.what() << std::endl;
			std::cerr << "Error: can't convert the clint body size to long! => " << bodySize << std::endl;
			std::exit(1);
		}
		switch (std::tolower(last))
		{
		case 'k':
			sizeInByte = i * 1024;
			break;
		case 'm':
			sizeInByte = i * 1024 * 1024;
			break;
		case 'g':
			sizeInByte = i * 1024 * 1024 * 1024;
			break;	
		default:
			std::cerr << "Error: unknown unit! => " << bodySize << std::endl;
			std::exit(1);
			break;
		}
		if (sizeInByte > 1048576) {
			std::cerr << "Error: clint body size is too big! => " << bodySize << std::endl;
			std::exit(1);
		}
	}
}

void	ValidationConfigFile::duplicateKey() 
{
	std::vector<std::string> keyOneValue = {"host", "client_max_body_size", "root", "index",
	"autoindex", "upload_store", "cgi_pass" ,"location", "server_name"};

	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested)
	{
		if (server.second.values.find("server_name") == server.second.values.end())
		{
			server.second.values["server_name"].push_back("");
		}
		if (server.second.values.find("autoindex") != server.second.values.end())
		{
			std::string tempAutoIndex = server.second.values["autoindex"].front();
			if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
				onlyPrintErrorExit("autoindex", 1);
		}
		for (size_t i = 0; i < keyOneValue.size(); ++i)
		{
			if ( server.second.values.find(keyOneValue[i]) != server.second.values.end())
			{
				if (server.second.values[keyOneValue[i]].size() != 1)
				{
					std::cerr << "Error: key duplicated or the number of values incorrect ! => " << keyOneValue[i]  << std::endl;
					std::exit(1);
				}
			}
		}
	}

	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested)
	{
		for (size_t i = 0; i < keyOneValue.size(); ++i)
		{
			for (std::pair<const std::string, ConfigBlock> location : server.second.nested)
			{
				if (location.second.values.find(keyOneValue[i]) != location.second.values.end())
				{
					if (location.second.values[keyOneValue[i]].size() != 1)
					{
						std::cerr << "Error: key duplicated or the number of values incorrect ! => " << keyOneValue[i]  << std::endl;
						std::exit(1);
					}
				}
			}
		}
		for (std::pair<const std::string, ConfigBlock> location : server.second.nested)
		{
			if (location.second.values.find("autoindex") != location.second.values.end())
			{
				std::string tempAutoIndex = location.second.values["autoindex"].front();
				if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
					onlyPrintErrorExit("autoindex", 1);
			}
			if (location.second.values.find("location") != location.second.values.end())
			{
				std::string temp = location.second.values["location"].front();
				if (temp.compare("/upload") == 0) {
					if (location.second.values.find("methods") != location.second.values.end())
					{
						for (std::vector<std::string>::iterator it = location.second.values["methods"].begin(); it != location.second.values["methods"].end(); ++it)
						{
							std::string itValue = *it;
							if (itValue.compare("POST") != 0 && itValue.compare("GET") != 0 && itValue.compare("DELETE") != 0) 
								onlyPrintErrorExit("methods", 1);
						}
					}
				}
				else 
				{
					if (location.second.values.find("methods") != location.second.values.end()) 
					{
						if (location.second.values["methods"].size() > 3) 
							onlyPrintErrorExit("methods", 1);
						for (std::vector<std::string>::iterator it = location.second.values["methods"].begin(); it != location.second.values["methods"].end(); ++it) 
						{
							std::string itValue = *it;
							if (itValue.compare("POST") != 0 && itValue.compare("GET") != 0 && itValue.compare("DELETE") != 0) 
								onlyPrintErrorExit("methods", 1);
						}
					}
				}

			}
		}
	}
}

void	checkListen(std::vector<std::string> &listens)
{
	for (std::string listen : listens)
	{
		if (!std::all_of(listen.begin(), listen.end(), [] (unsigned char c) { return std::isdigit(c); })) {
			onlyPrintErrorExit("listen", 3);
		}
	}
}

void	checkHost(std::string host)
{
	size_t		pos = 0;
	std::string	digitPart;
	int			convertToInt;
	int			dot = 0;
	while(1) 
	{
		if (dot == 3)
		{
			if (!std::all_of(host.begin(), host.end(), [] (unsigned char c) { return std::isdigit(c); }))
			{
				onlyPrintErrorExit("host", 4);
			}
			try
			{
				convertToInt  = stoi(host);
			}
			catch(const std::exception& e)
			{
				std::cerr << "Error: convert host to  int failed! " << e.what() << std::endl;
				std::exit(1);
			}
			if (convertToInt > 255 || convertToInt < 0)
				onlyPrintErrorExit("host", 4);
			return;
		}
		else
		{
			pos = host.find_first_of('.');
			if (pos == std::string::npos)
				onlyPrintErrorExit("host", 4);
			digitPart = host.substr(0,pos);
			if (pos < host.size())
				host = host.substr(pos+1);
			else
				onlyPrintErrorExit("host", 4);
			if (!std::all_of(digitPart.begin(), digitPart.end(), [] (unsigned char c) { return std::isdigit(c); })) 
				onlyPrintErrorExit("host", 4);
			try
			{
				convertToInt  = stoi(digitPart);
			}
			catch(const std::exception& e)
			{
				std::cerr << "Error: convert host to  int failed! " << e.what() << std::endl;
				std::exit(1);
			}
			if (convertToInt > 255 || convertToInt < 0)
				onlyPrintErrorExit("host", 4);
		}
		++dot;
	}
}


ValidationConfigFile::ValidationConfigFile(ConfigBlock &configData) : _configData(configData)
{
	duplicateKey();
	int i = 0;
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested)
	{
		std::string key;
		std::vector<std::string> valueForKey;
		std::map<std::string, std::vector<std::string>> myMap;
	
		if (server.second.values.find("host") != server.second.values.end())
		{
			checkHost(server.second.values["host"].front());
			key = server.second.values["host"].front();
		}
		else
			onlyPrintErrorExit("host", 2);
		if (server.second.values.find("listen") != server.second.values.end())
		{
			checkListen(server.second.values["listen"]);
			valueForKey = server.second.values["listen"];
		}
		else
			onlyPrintErrorExit("listen", 2);

		if (server.second.values.find("server_name") != server.second.values.end())
		{
			valueForKey.push_back(server.second.values["server_name"].front());	 
		}
		else
			valueForKey.push_back("");

		if (server.second.values.find("client_max_body_size") != server.second.values.end())
			checkBodySize(server.second.values["client_max_body_size"].front());
		myMap[key] = valueForKey;
		compareArray.push_back(myMap);
		++i;
	}
	isSeversSame();
}