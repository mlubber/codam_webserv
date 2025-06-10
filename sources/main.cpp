#include "../headers/Server.hpp"
#include "../headers/Configuration.hpp"
#include "../headers/ValidationConfigFile.hpp"
#include "../headers/Host.hpp"
#include "../headers/headers.hpp"
#include <iterator>

std::vector<Host>	serverArray;

std::vector<Host>& makeServerArray(ConfigBlock& configData)
{
	int i = 0;
	
	for(std::pair<const std::string, ConfigBlock> &serverBlock : configData.nested)
	{ 
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
		Host& lastServer = serverArray.back();

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


int main(int argc, char **argv)
{

	std::string confFile;
	if (argc > 2) 
	{
		std::cerr << "you can only pass one argument to program." << std::endl;
		return (1);
	} 
	else if (argc < 2)
		confFile = "configFile2.conf";
	else 
		confFile = argv[1];
	std::ifstream configFile(confFile);
	if (!configFile) 
	{
		std::cerr << "Error: Could not open configuration file: " << confFile << std::endl;
		return (1);
	}

	Configuration myconfig;
	myconfig.parseConfig(confFile);
	// std::cout << "printing all server blocks begin!\n\n\n\n" << std::endl;
	// myconfig.printConfig(myconfig.getConfigData(), 0);



	ValidationConfigFile	validator(myconfig.getConfigData());
	// std::cout << "printing all server blocks after check and add all base key value!\n\n\n\n" << std::endl;
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

	std::vector<Host> serverArray = makeServerArray(myconfig.getConfigData());

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

	std::vector<std::pair<std::string, std::vector<int>>> configdata(serverArray.size());

	for (size_t i = 0; i < serverArray.size(); i++)
	{
		configdata[i].first = serverArray[i].getHost();
		configdata[i].second = convertToInt(serverArray[i].getPort());
		// std::cout << "host: " << serverArray[i].getHost() << std::endl;
		
		// for (size_t j = 0; j < serverArray[i].getPort().size(); j++)
		// {
		// 	std::cout << "ports: " << serverArray[i].getPort()[j] << std::endl;
		// }
	}

	if (pipe(signal_pipe) == -1)
		return (1);
	if (setNonBlocking(signal_pipe[0]) == -1)
	{
		close_signal_pipe(1);
		return (1);
	}
	if (initialize_signals() == -1)
	{
		close_signal_pipe(2);
		return (1);
	}

	while (1)
	{
		try
		{
			Server server(myconfig);
		
			if (!server.initialize(configdata))
			{
				close_signal_pipe(0);
				return (1);
			}
			server.run();
			if (server.getCloseServer() == true)
				break ;
		}
		catch(const std::exception& e)
		{
			std::cerr << "Server failed: " << e.what() << "\n..server restarting.." << std::endl;
			// close epoll fd
		}
		catch (...)
		{
			std::cerr << "Server failed: server restarting.." << std::endl;
			// close epoll fd
		}
	}
	
	return (0);
}