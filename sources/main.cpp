#include "../headers/Server.hpp"
#include "../headers/Configuration.hpp"
#include "../headers/ValidationConfigFile.hpp"
#include "../headers/Serve.hpp"
#include "../headers/Request.hpp"
#include <iterator>



// #include <iostream>
// #include <filesystem>

// std::string resolvePath(const std::string& userPath) {
//     return std::filesystem::absolute(userPath).string();
// }

// int main() {
//     std::string userPath = "./www";
//     std::cout << "Resolved Path: " << resolvePath(userPath) << std::endl;
// }

std::vector<Serve>	serverArray;

std::vector<Serve>& makeServerArray(ConfigBlock& configData) {
	int i = 0;
	
	for(std::pair<const std::string, ConfigBlock> &serverBlock : configData.nested) { 
	std::string  tHost, tRoot, tServerName;
	std::vector<std::string> tPort;
	
	//std::cout << "i is : " << i << std::endl;
	std::string n = serverBlock.first;
	if (serverBlock.second.values.find("root") !=  serverBlock.second.values.end())
		tRoot = serverBlock.second.values["root"].front();
	if (serverBlock.second.values.find("listen") !=  serverBlock.second.values.end())
		tPort = serverBlock.second.values["listen"];
	if (serverBlock.second.values.find("host") !=  serverBlock.second.values.end())
		tHost = serverBlock.second.values["host"].front();
	if (serverBlock.second.values.find("server_name") !=  serverBlock.second.values.end())
		tServerName = serverBlock.second.values["server_name"].front();

	serverArray.emplace_back(tHost, tPort, tRoot, tServerName);
	Serve& lastServer = serverArray.back();

	lastServer.getServerBlock().nested = serverBlock.second.nested;
	lastServer.getServerBlock().values = serverBlock.second.values;
	++i;

	}
	// i = 0;
	// for (Server &data : serverArray) {
	// 	std::cout << "server number : " << i << std::endl;
	// 	ConfigBlock map = data.getServerBlock();
	// 	for (const std::pair<const std::string, std::vector<std::string>> &value : map.values) {
	// 		std::cout << value.first <<"  :  ";
	// 		for (const std::string& str : value.second)
    // 			std::cout << str << ",  ";

	// 		// for (std::vector<std::string>::const_iterator it = value.second.begin(); it != value.second.end(); ++it)
	// 		// 	std::cout << *it <<",  ";
	// 		std::cout << std::endl;
	// 	}
	// 	++i;
	// 	std::cout << std::endl;
	// }
	return (serverArray);
}

int	findResponsibleServer(std::vector<Serve>& serverArray, clRequest clientStruct){
	if (clientStruct.methodNotAllowd) {
		std::cout << "we have to respond with method not allowed!" << std::endl;
		return -1;
	}
	if (!clientStruct.invalidRequest) {
		int i = 0;
		for (Serve &data : serverArray) {
			std::vector<std::string> ports = data.getPort();
			std::string host = data.getHost();
			std::string virtualHost = data.getServer_name();
			// std::cout << "server virtualHost is : (" << virtualHost << ") , and client host is : (" << clientStruct.host << ")." << std::endl;
			//std::cout << "server host is : (" << host << ") , and client host is : (" << clientStruct.host << ")." << std::endl;
			// std::cout << "server port is : (" << ports.front() << ") , and client port is : (" << clientStruct.port << ")." << std::endl;
			for (std::string &port : ports) {
				if (port == clientStruct.port) {
					if (host == clientStruct.host) {
						return i;
					}else if (virtualHost == clientStruct.host) {
						return i;
					}
				}
			}
			++i;
		}
	}
	// response bad request
	std::cout << "we have to response with bad request!" << std::endl;
	return -1;

}



// void mainLoop()
// {
// 	_epoll_fd = epoll_create1(0);
// 	while (true)
// 	{
// 		epoll_wait()
// 		handle_event();
// 	}
// }
std::vector<int> convertTo_int(std::vector<std::string> &portsStr) {

	std::vector<int> portsInt;
	for (size_t i = 0; i <portsStr.size(); ++i){
		try{
			portsInt.push_back(std::stoi(portsStr[i]));

		}catch(const std::exception &e) {
			std::cerr << "Error : Conversion failed!" << e.what() << std::endl;
		}
	}
	return (portsInt);
}

int main(int argc, char **argv) {

	std::string confFile;
	if (argc > 2) {
		std::cerr << "you can only pass one argument to program." << std::endl;
		return 1;
	} else if (argc < 2) {
		confFile = "configFile2.conf";
	} else {
		confFile = argv[1];
	}
	std::ifstream configFile(confFile);
	if (!configFile) {
		std::cerr << "Error: Could not open configuration file: " << confFile << std::endl;
		return 1;
	}

	Configuration myconfig;
	myconfig.parseConfig(confFile);
	std::cout << "printing all server blocks begin!\n\n\n\n" << std::endl;
	// myconfig.printConfig(myconfig.getConfigData(), 0);



	ValidationConfigFile	validator(myconfig.getConfigData());
	std::cout << "printing all server blocks after check and add all base key value!\n\n\n\n" << std::endl;
	//validator.printConfig(validator.getConfig(), 0);
	// myconfig.printConfig(myconfig.getConfigData(), 0);

	// ConfigBlock serverBlock = myconfig.getServerBlock("127.0.0.2", "8082");

	// for (const std::pair<const std::string, std::vector<std::string>> &value : serverBlock.values) 
	// {
	// 	std::cout << value.first <<": ";
	// 	for (const std::string& str : value.second)
	// 		std::cout << str << ", ";
	// 	std::cout << std::endl;
	// }
	// for (const std::pair<const std::string, ConfigBlock> &nested : serverBlock.nested) {
	// 	std::cout << nested.first << " {" << std::endl;
	// 	for (const std::pair<const std::string, std::vector<std::string>> &value : nested.second.values) {
	// 		std::cout << value.first <<": ";
	// 		for (const std::string& str : value.second)
	// 			std::cout << str << ", ";
	// 		std::cout << std::endl;
	// 	}
	// 	std::cout << "}" << std::endl;
	// }

	std::vector<Serve> serverArray = makeServerArray(myconfig.getConfigData());

	// std::cout << "print server array: \n\n\n" << std::endl;
	// for (size_t i = 0; i < serverArray.size(); i++)
	// {
	// 	std::cout << "server index : " << i << std::endl;
	// 	ConfigBlock map = serverArray[i].getServerBlock();
	// 	for (const std::pair<const std::string, std::vector<std::string>> &value : map.values) {
	// 		std::cout << value.first <<"  :  ";
	// 		for (const std::string& str : value.second)
	// 			std::cout << str << ",  ";
	// 		std::cout << std::endl;
	// 	}
	// 	std::cout << std::endl;
	// }

	// std::string str = 
    // "GET /index.html?now=hello HTTP/1.1\r\n"
    // "Host: 127.0.0.2:8081\r\n"
    // "User-Agent: curl/7.68.0\r\n"
    // "Accept: text/html\r\n"
    // "Connection: keep-alive\r\n";

	// std::string str = 
    // "GET /index.html HTTP/1.1\r\n"
    // "Host: 127.0.0.1:8080\r\n"
    // "User-Agent: curl/7.68.0\r\n"
    // "Accept: text/html\r\n"
	// "content-type: text/html\r\n"
    // "Connection: keep-alive\r\n"
	// "Transfer-Encoding:chunked\r\n"
	// "\r\n"
	// "a\r\n"
	// "key: value\r\n"
	// "e\r\n"
	// "blabla: blabla\r\n"
	// "0\r\n"
	// "\r\n";

	// std::string str =
	// "DELETE /upload HTTP/1.1\r\n"
	// "Host: 127.0.0.1:8080\r\n"
	// "Content-Type: multipart/form-data\r\n"
	// "Content-Length: 174\r\n"
	// "\r\n"
	// "------WebKitFormBoundary\r\n"
	// "Content-Disposition: form-data; name=\"file\"; filename=\"image.png\"\r\n"
	// "Content-Type: image/png\r\n"
	// "\r\n"
	// "(binary image data here)\r\n"
	// "------WebKitFormBoundary--\r\n";

	// std::string str =
	// "POST /upload HTTP/1.1\r\n"
	// "Host: 127.0.0.1:8080\r\n"
	// "Transfer-Encoding: chunked\r\n"
	// "Content-Type: text/plain\r\n"
	// "\r\n"
	// "5\r\n"
	// "Hello\r\n"
	// "8\r\n"
	// " Chunked\r\n"
	// "4\r\n"
	// "Body\r\n"
	// "0\r\n"
	// "\r\n";


	// std::string str1 =
	// "POST /upload HTTP/1.1\r\n"
	// "Host: 127.0.0.1:8080\r\n"
	// "Content-Type: multipart/form-data\r\n"
	// "Expect: 100-continue\r\n"
	// "Content-Length: 174\r\n"
	// "\r\n";
	// std::string str2 =
	// "------WebKitFormBoundary\r\n"
	// "Content-Disposition: form-data; name=\"file\"; filename=\"image.png\"\r\n"
	// "Content-Type: image/png\r\n"
	// "\r\n"
	// "(binary image data here)\r\n"
	// "------WebKitFormBoundary--\r\n";



	// Client		client;


	// for (int i = 0 ; i < 2; ++i) {
	// 	if (i == 0)
	// 		client.readRequest(str1, 5);
	// 	if (i == 1)
	// 		client.readRequest(str2, 5);
	// 	std::cout << "to check if server found\n" << std::endl;
	// 	int result = findResponsibleServer(serverArray, client.getClStructRequest(5));
	// 	if (result != -1) {
	// 		std::cout << "server number : (" << result << ") should answer!" << std::endl; 
	// 	}
	// }

	// client.readRequest(str, 5);
	// std::cout << "to check if server found\n" << std::endl;
	// int result = findResponsibleServer(serverArray, client.getClStructRequest(5));
	// if (result != -1) {
	// 	std::cout << "server number : (" << result << ") should answer!" << std::endl; 
	// 	serverArray[result].answerRequest(client.getClStructRequest(5));
	// } else {
	// 	serverArray[0].answerRequest(client.getClStructRequest(5));
	// }


	std::vector<std::pair<std::string, std::vector<int>>> configdata(serverArray.size());

	for (size_t i = 0; i < serverArray.size(); i++)
	{
		configdata[i].first = serverArray[i].getHost();
		configdata[i].second = convertTo_int(serverArray[i].getPort());
		// std::cout << "host: " << serverArray[i].getHost() << std::endl;
		
		for (size_t j = 0; j < serverArray[i].getPort().size(); j++)
		{
			// std::cout << "ports: " << serverArray[i].getPort()[j] << std::endl;
		}
	}

	Server server(myconfig);

	if (!server.initialize(configdata))
		return (1);
	server.run();

	
	return 0;

}