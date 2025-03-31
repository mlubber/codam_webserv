#include "../headers/ValidationConfigFile.hpp"
#include <iterator>
#include <algorithm>


std::vector<std::vector<std::string>> compare;

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


void	isSeversSame() {

	for (size_t i = 0; i < compare.size() ; ++i)  {
		std::vector<std::string> s1 = compare[i];
		for (size_t j = i + 1; j < compare.size() ; ++j)  {
			std::vector<std::string> s2 = compare[j];
			for (size_t isub = 0; isub < s1.size(); ++isub){
				for (size_t jsub = 0; jsub < s2.size(); ++jsub) {
					if (s1[isub] == s2[jsub]) {
						std::cerr << "Error: servers can't has same port! => " << s1[isub] << std::endl;
						std::exit(1);
					}
				}
			}
		}
	}
}


// void	isSeversSame() {

// 	for (size_t i = 0; i < compare.size() ; ++i)  {
// 		std::vector<std::string> s1 = compare[i];
// 		std::string listen1 = s1[0];
// 		std::string host1 = s1[1];
// 		std::string server_name1 = s1[2];
// 		for (size_t j = i + 1; j < compare.size() ; ++j)  {
// 			std::vector<std::string> s2 = compare[j];
// 			std::string listen2 = s2[0];
// 			std::string host2 = s2[1];
// 			std::string server_name2 = s2[2];
// 			if (listen1 == listen2 && host1 == host2 && server_name1 == server_name2) {
// 				std::cerr << "Error: servers can't has same port!" << std::endl;
// 				std::exit(1);
// 			}
// 		}
// 	}
// }

void	onlyPrintErrorExit(std::string key, int i) {
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

void	checkBodySize(std::string &bodySize) {
	size_t i, sizeInByte;
	char last = bodySize.back();
	std::string	strNum;
	if (isdigit(last)) {
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
	else {
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

void	ValidationConfigFile::duplicateKey() {
	std::vector<std::string> keyOneValue = {"host", "client_max_body_size", "root", "index",
	"autoindex", "upload_store", "cgi_pass" ,"location"};

	// also checking the value of autoindex and method  that should be only (on , off , get, post , delete) not its else.
	
	//looping all values inside server block NOT location block that should has only one value  
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
		if (server.second.values.find("autoindex") != server.second.values.end()) {
				std::string tempAutoIndex = server.second.values["autoindex"].front();
				if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
					onlyPrintErrorExit("autoindex", 1);
		}
		for (size_t i = 0; i < keyOneValue.size(); ++i) {
			if ( server.second.values.find(keyOneValue[i]) != server.second.values.end()) {
				if (server.second.values[keyOneValue[i]].size() != 1) {
				std::cerr << "Error: key duplicated or the number of values incorrect ! => " << keyOneValue[i]  << std::endl;
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
						std::cerr << "Error: key duplicated or the number of values incorrect ! => " << keyOneValue[i]  << std::endl;
						std::exit(1);
					}
				}
			}
		}
		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
			if (location.second.values.find("autoindex") != location.second.values.end()) {
				std::string tempAutoIndex = location.second.values["autoindex"].front();
				if (tempAutoIndex.compare("on") != 0 && tempAutoIndex.compare("off") != 0)
					onlyPrintErrorExit("autoindex", 1);
			}
			if (location.second.values.find("location") != location.second.values.end()) {
				std::string temp = location.second.values["location"].front();
				if (temp.compare("/upload") == 0) {
					if (location.second.values.find("methods") != location.second.values.end()) {
						for (std::vector<std::string>::iterator it = location.second.values["methods"].begin(); it != location.second.values["methods"].end(); ++it) {
							std::string itValue = *it;
							if (itValue.compare("POST") != 0 && itValue.compare("GET") != 0 && itValue.compare("DELETE") != 0) 
								onlyPrintErrorExit("methods", 1);
						}
						// if (location.second.values["methods"].size() != 1) 
						// 	onlyPrintErrorExit("methods", 1);
						
						// std::string tempMethod = location.second.values["methods"].front();
						// if (tempMethod.compare("POST") != 0)
						// 	onlyPrintErrorExit("methods", 1);
					}
				} else {
					if (location.second.values.find("methods") != location.second.values.end()) {
						if (location.second.values["methods"].size() > 3) 
							onlyPrintErrorExit("methods", 1);
						for (std::vector<std::string>::iterator it = location.second.values["methods"].begin(); it != location.second.values["methods"].end(); ++it) {
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

void	checkListen(std::vector<std::string> &listens)  {
	for (std::string listen : listens) {
		if (!std::all_of(listen.begin(), listen.end(), [] (unsigned char c) { return std::isdigit(c); })) {
			onlyPrintErrorExit("listen", 3);
		}
	}

	// for (std::string listen : listens) {
	// 	for (size_t i = 0; i < listen.size(); ++i){
	// 		if (!std::isdigit(listen[i]))
	// 			onlyPrintErrorExit("listen", 3);
	// 	}
	// }
}

void	checkHost(std::string host) {
	size_t		pos = 0;
	std::string	digitPart;
	int			convertToInt;
	int			dot = 0;
	while(1) {
		if (dot == 3) {
			if (!std::all_of(host.begin(), host.end(), [] (unsigned char c) { return std::isdigit(c); })) {
				onlyPrintErrorExit("host", 4);
			}
			try{
				convertToInt  = stoi(host);
			}catch(const std::exception& e){
				std::cerr << "Error: convert host to  int failed! " << e.what() << std::endl;
				std::exit(1);
			}
			if (convertToInt > 255 || convertToInt < 0)
				onlyPrintErrorExit("host", 4);
			return;
		} else {
			pos = host.find_first_of('.');
			if (pos == std::string::npos)
				onlyPrintErrorExit("host", 4);
			digitPart = host.substr(0,pos);
			if (pos < host.size())
				host = host.substr(pos+1);
			else
				onlyPrintErrorExit("host", 4);

			if (!std::all_of(digitPart.begin(), digitPart.end(), [] (unsigned char c) { return std::isdigit(c); })) {
				onlyPrintErrorExit("host", 4);
			}
			try{
				convertToInt  = stoi(digitPart);
			}catch(const std::exception& e){
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
	// duplicateKey();
	// int i = 0;
	// for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
	// 	compare.push_back(std::vector<std::string>());
	
	// 	if (server.second.values.find("listen") != server.second.values.end()) {
	// 		compare[i].push_back(server.second.values["listen"].front());	 
	// 	}else {
	// 		compare[i].push_back("");
	// 	}
	// 	if (server.second.values.find("host") != server.second.values.end()) {
	// 		compare[i].push_back(server.second.values["host"].front());	 
	// 	}else {
	// 		compare[i].push_back("");
	// 	}
	// 	if (server.second.values.find("server_name") != server.second.values.end()) {
	// 		compare[i].push_back(server.second.values["server_name"].front());	 
	// 	}else  {
	// 		compare[i].push_back("");
	// 	}
	// 	if (server.second.values.find("client_max_body_size") != server.second.values.end()) {
	// 		checkBodySize(server.second.values["client_max_body_size"].front());
	// 	}
	// 	++i;
	// }
	// isSeversSame();


	duplicateKey();
	int i = 0;
	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
		compare.push_back(std::vector<std::string>());
	
		if (server.second.values.find("listen") != server.second.values.end()) {
			compare[i] = server.second.values["listen"];
			checkListen(server.second.values["listen"]);
			//compare[i].push_back(server.second.values["listen"].front());	 
		}else {
			onlyPrintErrorExit("listen", 2);
		}
		if (server.second.values.find("host") != server.second.values.end()) {
			checkHost(server.second.values["host"].front());
 
		}else {
			onlyPrintErrorExit("host", 2);
		}
		// if (server.second.values.find("server_name") != server.second.values.end()) {
		// 	compare[i].push_back(server.second.values["server_name"].front());	 
		// }else  {
		// 	compare[i].push_back("");
		// }
		if (server.second.values.find("client_max_body_size") != server.second.values.end()) {
			checkBodySize(server.second.values["client_max_body_size"].front());
		}
		++i;
	}
	isSeversSame();
}





//#include <filesystem>
// std::map<std::string, bool>	errorPages;
// std::map<std::string, std::string>	pathPages;

// std::string resolvePath(const std::string& userPath) {
//     return std::filesystem::absolute(userPath).string();
// }

// void	fileOrDirectoryExists(std::string path) {
// 	std::string filepath = resolvePath(path);
// 	std::cout << "whole path is : " << filepath << std::endl;
// 	bool res = std::filesystem::exists(filepath);
// 	std::cout << "res is : " << res << std::endl;
// 	if (!res) { 
// 		std::cerr << "here Error: directory doesn't exist! =>  " << path << std::endl;
// 		std::exit(1);
// 	} else {
// 		std::cout << "succefully find the wholePath is : " << path << std::endl;
// 	}
// }


// void	ValidationConfigFile::checkErrorPageExist(std::string defRoot, std::string overrideRoot, std::vector<std::string> errorPath, int blockIndex, bool add) 
// {
// 	errorPages = {{"400", false} ,{"404", false}, {"413", false}, {"500", false}};
// 	pathPages = {{"400", "/errors/400.html"} ,{"404", "/errors/404.html"}, {"413", "/errors/413.html"}, {"500", "/errors/500.html"}};
	

// 	for (std::vector<std::string>::iterator it = errorPath.begin(); it != errorPath.end(); ++it) {
// 		std::string path;
// 		size_t	dotPosition;
// 		size_t	slashPosition;
// 		std::string	strErrorPath = *it;
// 		std::string	nameErrorFile;

// 		if (overrideRoot.empty()) 
// 			path = defRoot + *it;
// 		else
// 			path = overrideRoot + *it;
// 		fileOrDirectoryExists(path);

// 		slashPosition = strErrorPath.find_last_of("/");
// 		if (slashPosition == std::string::npos)
// 			slashPosition = strErrorPath[0];
// 		else
// 			slashPosition++;
// 		nameErrorFile = strErrorPath.substr(slashPosition);
// 		dotPosition = strErrorPath.find_last_of(".");
		
// 		if (dotPosition != std::string::npos) {
// 			nameErrorFile = nameErrorFile.substr(0, dotPosition - slashPosition);
// 		} else {
// 			std::cerr << "Error: invalid file path!" << std::endl;
// 			std::exit(1);
// 		}
// 		if (errorPages.find(nameErrorFile) != errorPages.end()) {
// 			errorPages[nameErrorFile] = true;
// 		}
// 	}

// 	if (add) {
// 		std::multimap<std::string, ConfigBlock>::iterator it = _configData.nested.begin();
// 		std::advance(it, blockIndex);
// 		std::vector<std::string> &existingValues = it->second.values["error_page"];
// 		for(const std::pair<const std::string, bool> &page : errorPages) {
// 			if (page.second == false) {
// 				std::map<std::string, std::string>::iterator pa = pathPages.find(page.first);
// 				if (pa != pathPages.end()) {
// 					existingValues.push_back(pa->first);
// 					existingValues.push_back(pa->second);
// 				}
// 			}
// 		}
// 	}
// }


// void	ValidationConfigFile::addBaseValueIfNeedit(){
// 	std::map<std::string, std::vector<std::string>> defaultConfigKeyValue = { 
// 	{"listen", {"80"}},{"client_max_body_size", {"1m"}},{"root",{"./www"}},
// 	{"server_name",{"default_server"}} ,{"host", {"127.0.0.1"}} 
// 	,{"error_page",{"400","/errors/400.html", "404", "/errors/404.html", "413", "/errors/413.html", "500", "/errors/500.html"}}
// 	};
// 	std::vector<std::string> baseValues = {"listen", "host", "client_max_body_size", "root", "error_page"};
// 	int blockIndex = 0;


// 	std::string  temp, overrideRoot;
// 	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){
// 		std::vector<std::string> errorPath;
// 		std::string defRoot = "./www";

// 		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
// 			if (location.second.values.find("location") != location.second.values.end()) {
// 				temp = location.second.values["location"].front();
// 				if (temp.compare(0, 7,"/errors") == 0) {
// 					if (location.second.values.find("root") != location.second.values.end())
// 						overrideRoot = location.second.values["root"].front();
// 				}
// 			}
// 		}

// 		for (std::vector<std::string>::iterator it = baseValues.begin(); it != baseValues.end(); ++it)
// 		{
// 			std::map<std::string, std::vector<std::string>>::iterator default_value = defaultConfigKeyValue.find(*it);
// 			if (*it != "error_page") {
// 				if (server.second.values.find(*it) == server.second.values.end())
// 				{
// 					if (default_value != defaultConfigKeyValue.end())
// 						server.second.values[*it] = default_value->second;
// 				}
// 			} else if (*it == "error_page") {
// 				std::map<std::string, std::vector<std::string>>::iterator itErrorPage = server.second.values.find(*it);
// 				if (itErrorPage != server.second.values.end()) {
// 					for (const std::string &value : itErrorPage->second) {
// 						if (value.find('.') != std::string::npos) {
// 							errorPath.push_back(value);
// 						}
// 					}
// 				}	
// 			}
// 			if (*it == "root"){
// 				std::cout << "here" << std::endl;
// 				defRoot = server.second.values[*it].front();
// 				std::cout << "defRoot" << defRoot << std::endl;
// 			}
// 		}
// 		// if the root or override root is same as our default root we can add the error pages of doesn't exist.
// 		if (overrideRoot == "./www" || defRoot == "./www")
// 			checkErrorPageExist(defRoot, overrideRoot, errorPath, blockIndex, true);
// 		else
// 			checkErrorPageExist(defRoot, overrideRoot, errorPath, blockIndex, false);
// 		++blockIndex;
// 	}
// }



// ValidationConfigFile::ValidationConfigFile(ConfigBlock &configData) : _configData(configData)
// {
// 	duplicateKey();
// 	addBaseValueIfNeedit();

// 	std::string temp, tempFileName, tempOverRoot, tempUploadPath;
// 	std::string defRoot = "./www";


// 	for (std::pair<const std::string, ConfigBlock> &server : _configData.nested){


// 		if (server.second.values.find("root") != server.second.values.end()) {
// 			defRoot = server.second.values["root"].front();
// 			fileOrDirectoryExists(defRoot);
// 		}
// 		if (server.second.values.find("client_max_body_size") != server.second.values.end()) {
// 			checkBodySize(server.second.values["client_max_body_size"].front());
// 		}
// 		if (server.second.values.find("index") != server.second.values.end()) {
// 			tempFileName = defRoot + server.second.values["index"].front();
// 			fileOrDirectoryExists(tempFileName);
// 		}
// 		if (server.second.values.find("upload_store") != server.second.values.end()) {
// 			tempUploadPath = server.second.values["upload_store"].front();
// 			fileOrDirectoryExists(tempUploadPath);
// 		}	

// 		// looping all location of one server block to CHECK IF FILE AND DIRECTORY ARE EXIST.
// 		for (std::pair<const std::string, ConfigBlock> location : server.second.nested) {
// 			if (location.second.values.find("location") != location.second.values.end()) {
// 				temp = location.second.values["location"].front();
// 				if (location.second.values.find("root") != location.second.values.end()) {
// 					tempOverRoot = location.second.values["root"].front();
// 					fileOrDirectoryExists(tempOverRoot);
// 				}
// 				if (location.second.values.find("index") != location.second.values.end()) {
// 					tempFileName =  location.second.values["index"].front();
// 					std::string tf;

// 					if (tempOverRoot.empty())
// 						tf = defRoot + temp + tempFileName;
// 					else 
// 						tf = tempOverRoot + tempFileName;
// 					fileOrDirectoryExists(tf);
// 				}
// 				if (location.second.values.find("upload_store") != location.second.values.end()) {
// 					tempUploadPath = location.second.values["upload_store"].front();
// 					fileOrDirectoryExists(tempUploadPath);
// 				}	
// 			}
// 		}
// 	}
// }
