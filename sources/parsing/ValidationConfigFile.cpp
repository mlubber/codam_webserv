#include "../headers/ValidationConfigFile.hpp"
#include <cstring>
#include <iterator>
#include <sys/stat.h>


std::map<std::string, bool>	errorPages;
std::map<std::string, std::string>	pathPages;
std::string homePath = getenv("PWD");
struct stat file_stat;


ValidationConfigFile::~ValidationConfigFile(){}

ValidationConfigFile::ValidationConfigFile(const ValidationConfigFile& other) : _configData(other._configData) {}

ValidationConfigFile& ValidationConfigFile::operator=(const ValidationConfigFile& other) {
	if (this != &other) {
		this->_configData.nested.clear();
		this->_configData.values.clear();
		this->_configData.nested = other._configData.nested;
		this->_configData.values = other._configData.values;
	}
	return (*this);
}

void	onlyPrintErrorExit(std::string key) {
	std::cerr << "Error: unknown methods or invalid! => " << key << std::endl;
	std::exit(1);
}

void	ValidationConfigFile::checkErrorPageExist(std::string defRoot, std::string overrideRoot, std::vector<std::string> errorPath, int blockIndex, bool add) 
{
	errorPages = {{"400", false} ,{"404", false}, {"413", false}, {"500", false}};
	pathPages = {{"400", "/errors/400.html"} ,{"404", "/errors/404.html"}, {"413", "/errors/413.html"}, {"500", "/errors/500.html"}};
	

	for (std::vector<std::string>::iterator it = errorPath.begin(); it != errorPath.end(); ++it) {
		std::string path;
		size_t	dotPosition;
		size_t	slashPosition;
		std::string	strErrorPath = *it;
		std::string	nameErrorFile;

		path = homePath + defRoot + *it;
		std::ifstream fileToOpen(path);

		if (!fileToOpen.is_open()) {
			path = homePath + overrideRoot + *it;
			std::ifstream fileToOpen(path);
			if (!fileToOpen.is_open()) {
				std::cerr << "Error: can't find error pages." << std::endl;
				std::exit(1);
			}
		}

		slashPosition = strErrorPath.find_last_of("/");
		if (slashPosition == std::string::npos)
			slashPosition = strErrorPath[0];
		else
			slashPosition++;
		nameErrorFile = strErrorPath.substr(slashPosition);
		dotPosition = strErrorPath.find_last_of(".");
		
		if (dotPosition != std::string::npos) {
			nameErrorFile = nameErrorFile.substr(0, dotPosition - slashPosition);
		} else {
			std::cerr << "Error: invalid file path!" << std::endl;
			std::exit(1);
		}
		if (errorPages.find(nameErrorFile) != errorPages.end()) {
			errorPages[nameErrorFile] = true;
		}
	}

	if (add) {
		std::multimap<std::string, ConfigBlock>::iterator it = _configData.nested.begin();
		std::advance(it, blockIndex);
		std::vector<std::string> &existingValues = it->second.values["error_page"];
		for(const std::pair<const std::string, bool> &page : errorPages) {
			if (page.second == false) {
				std::map<std::string, std::string>::iterator pa = pathPages.find(page.first);
				if (pa != pathPages.end()) {
					existingValues.push_back(pa->first);
					existingValues.push_back(pa->second);
				}
			}
		}
	}
}


void	checkBodySize(std::string &bodySize) {
	size_t i, sizeInByte;
	char last = bodySize.back();
	std::string	strNum;
	if (isdigit(last)) {
		strNum = bodySize;
		try
		{
			sizeInByte = std::stoul(strNum);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			std::cerr << "Error: can't convert the clint body size to long! => " << bodySize << std::endl;
			std::exit(1);
		}
	}
	else {
		strNum = bodySize.substr(0, bodySize.size() - 1);
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
	}
	if (sizeInByte > 1048576) {
		std::cerr << "Error: clint body size is too big! => " << bodySize << std::endl;
		std::exit(1);
	}
}

void	ValidationConfigFile::duplicateKey() {
	std::vector<std::string> keyOneValue = {"listen", "host", "client_max_body_size", "root", "index",
	"autoindex", "upload_store", "cgi_pass" ,"location"};

	// also checking the value of autoindex and method  that should be only (on , off , get, post , delete) not its else.
	
	//looping all values inside server block NOT location block that should has only one value  
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
		if (server.second.values.find("autoindex") != server.second.values.end()) {
				std::string tempAutoIndex = server.second.values["autoindex"].front();
				if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
					onlyPrintErrorExit("autoindex");
		}
		for (size_t i = 0; i < keyOneValue.size(); ++i) {
			if ( server.second.values.find(keyOneValue[i]) != server.second.values.end()) {
				if (server.second.values[keyOneValue[i]].size() != 1) {
				std::cerr << "Error: key duplicated or given more than one value ! => " << keyOneValue[i]  << std::endl;
				std::exit(1);
				}
			}
		}
	}

	//looping all values insidelocation block that should has only one value 
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
		for (size_t i = 0; i < keyOneValue.size(); ++i) {
			for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
				if (location.second.values.find(keyOneValue[i]) != location.second.values.end()) {
					if (location.second.values[keyOneValue[i]].size() != 1) {
						std::cerr << "Error: key duplicated or given more than one value ! => " << keyOneValue[i]  << std::endl;
						std::exit(1);
					}
				}
			}
		}
		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
			if (location.second.values.find("autoindex") != location.second.values.end()) {
				std::string tempAutoIndex = location.second.values["autoindex"].front();
				if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
					onlyPrintErrorExit("autoindex");
			}
			if (location.second.values.find("location") != location.second.values.end()) {
				std::string temp = location.second.values["location"].front();
				if (temp.compare("/upload") == 0) {
					if (location.second.values.find("methods") != location.second.values.end()) {
						if (location.second.values["methods"].size() != 1) 
							onlyPrintErrorExit("methods");
						
						std::string tempMethod = location.second.values["methods"].front();
						if (tempMethod.compare("POST") != 0)
							onlyPrintErrorExit("methods");
					}
				} else {
					if (location.second.values.find("methods") != location.second.values.end()) {
						if (location.second.values["methods"].size() > 3) 
							onlyPrintErrorExit("methods");
						for (std::vector<std::string>::iterator it = location.second.values["methods"].begin(); it != location.second.values["methods"].end(); ++it) {
							std::string itValue = *it;
							if (itValue.compare("POST") != 0 && itValue.compare("GET") != 0 && itValue.compare("DELETE") != 0) 
								onlyPrintErrorExit("methods");
						}
					}
				}

			}
		}
	}
}

void	ValidationConfigFile::addBaseValueIfNeedit(){
	std::map<std::string, std::vector<std::string>> defaultConfigKeyValue = { 
	{"listen", {"80"}},{"client_max_body_size", {"1m"}},{"root",{"/www"}},
	{"server_name",{"default_server"}} ,{"host", {"127.0.0.1"}} 
	,{"error_page",{"400","/errors/400.html", "404", "/errors/404.html", "413", "/errors/413.html", "500", "/errors/500.html"}}
	};
	std::vector<std::string> baseValues = {"listen", "host", "client_max_body_size", "root", "error_page"};
	int blockIndex = 0;


	std::string  temp, overrideRoot;
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
		std::vector<std::string> errorPath;
		std::string defRoot = "/www";

		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
			if (location.second.values.find("location") != location.second.values.end()) {
				temp = location.second.values["location"].front();
				if (temp.compare(0, 7,"/errors") == 0) {
					if (location.second.values.find("root") != location.second.values.end())
						overrideRoot = location.second.values["root"].front();
				}
			}
		}


		for (std::vector<std::string>::iterator it = baseValues.begin(); it != baseValues.end(); ++it)
		{
			std::map<std::string, std::vector<std::string>>::iterator default_value = defaultConfigKeyValue.find(*it);
			if (*it != "error_page") {
				if (server.second.values.find(*it) == server.second.values.end())
				{
					if (default_value != defaultConfigKeyValue.end())
						server.second.values[*it] = default_value->second;
				}
			} else if (*it == "error_page") {
				std::map<std::string, std::vector<std::string>>::iterator itErrorPage = server.second.values.find(*it);
				if (itErrorPage != server.second.values.end()) {
					for (const std::string &value : itErrorPage->second) {
						if (value.find('.') != std::string::npos) {
							errorPath.push_back(value);
						}
					}
				}	
			}
			if (*it == "root"){
				//std::cout << "here" << std::endl;
				defRoot = server.second.values[*it].front();
				//std::cout << "defRoot" << defRoot << std::endl;
			}
		}
		// if the root or override root is same as our default root we can add the error pages of doesn't exist.
		if (overrideRoot == "/www" || defRoot == "/www")
			checkErrorPageExist(defRoot, overrideRoot, errorPath, blockIndex, true);
		else
			checkErrorPageExist(defRoot, overrideRoot, errorPath, blockIndex, false);
		++blockIndex;
	}
}

ValidationConfigFile::ValidationConfigFile(ConfigBlock &configData) : _configData(configData)
{
	duplicateKey();
	addBaseValueIfNeedit();

	std::vector<std::string>	directoryOrFile = {"index", "upload_store", "root"};
	std::string temp, tempFileName, wholePath, tempOverRoot, tempUploadPath;
	std::string defRoot = "/www";
	const char *str = "";


	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){

		if (server.second.values.find("root") != server.second.values.end()) {
			defRoot = server.second.values["root"].front();
		}
		if (server.second.values.find("client_max_body_size") != server.second.values.end()) {
			checkBodySize(server.second.values["client_max_body_size"].front());
		}
		// looping all file and directory NOT INSIDE LOCATION of one server block to CHECK IF FILE AND DIRECTORY ARE EXIST.
		for (size_t i = 0; i < directoryOrFile.size(); ++i) {
			if (server.second.values.find(directoryOrFile[i]) != server.second.values.end()) {
				wholePath = homePath;
				tempFileName.clear();
				tempUploadPath.clear();
				if (directoryOrFile[i].compare("index") == 0) 	
					tempFileName = "/" + server.second.values["index"].front();
				if (directoryOrFile[i].compare("upload_store") == 0) 	
					tempUploadPath = server.second.values["upload_store"].front();	
				if (tempFileName.empty() && tempUploadPath.empty())
					wholePath = homePath + defRoot;
				else if (!tempFileName.empty())
					wholePath = homePath + defRoot + tempFileName;
				else if (!tempUploadPath.empty())
					wholePath = homePath + tempUploadPath;
				str = wholePath.c_str();
				if (stat(str, &file_stat) != 0) {
					if (tempFileName.empty())
						std::cerr << "error directory doesn't exist! =>" << defRoot << std::endl;
					else
						std::cerr << "error directory doesn't exist! =>" << tempFileName << std::endl;
					std::exit(1);
				}
			}
		}	

		// looping all location of one server block to CHECK IF FILE AND DIRECTORY ARE EXIST.
		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
			if (location.second.values.find("location") != location.second.values.end()) {
				temp = location.second.values["location"].front();
				tempOverRoot.clear();
				if (location.second.values.find("root") != location.second.values.end()) {
					tempOverRoot = location.second.values["root"].front();
				}
				tempFileName.clear();
				if (location.second.values.find("index") != location.second.values.end()) {
					tempFileName = location.second.values["index"].front();
				}
				wholePath.clear();
				if (!tempOverRoot.empty()) {
					wholePath = homePath + tempOverRoot;
					str = wholePath.c_str();
					if (stat(str, &file_stat) != 0) {
						std::cerr << "error directory doesn't exist! =>" << tempOverRoot << std::endl;
						std::exit(1);
					}
					if (!tempFileName.empty()) {
						wholePath = homePath + tempOverRoot + tempFileName;
						str = wholePath.c_str();
						if (stat(str, &file_stat) != 0) {
							std::cerr << "error directory doesn't exist! =>" << tempFileName << std::endl;
							std::exit(1);
						}
					}
				}
				if (tempOverRoot.empty() && !tempFileName.empty()) {
					wholePath = homePath + defRoot + temp + tempFileName;
					str = wholePath.c_str();
					if (stat(str, &file_stat) != 0) {
						std::cerr << "error directory doesn't exist! =>" << tempFileName << std::endl;
						std::exit(1);
					}
				}	
			}
		}
	}
}
