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
	return (serverArray);
}


int main(int argc, char **argv)
{

	std::string confFile;
	if (argc > 2) 
	{
		std::cerr << "SERVER ERROR: You can only pass one argument to the webserver" << std::endl;
		return (1);
	} 
	else if (argc < 2)
		confFile = "defaultConfigFile.conf";
	else 
		confFile = argv[1];
	std::ifstream configFile(confFile);
	if (!configFile) 
	{
		std::cerr << "SERVER ERROR: Could not open configuration file: " << confFile << std::endl;
		return (1);
	}

	Configuration myconfig;
	myconfig.parseConfig(confFile);
	ValidationConfigFile validator(myconfig.getConfigData());
	std::vector<Host> serverArray = makeServerArray(myconfig.getConfigData());
	std::vector<std::pair<std::string, std::vector<int>>> configdata(serverArray.size());

	for (size_t i = 0; i < serverArray.size(); i++)
	{
		configdata[i].first = serverArray[i].getHost();
		configdata[i].second = convertToInt(serverArray[i].getPort());
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
		Server server(myconfig);

		try
		{
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
			std::cerr << "SERVER ERROR: Server failed: " << e.what() << "\n..server restarting.." << std::endl;
			if (close(server.getEpollFd()) == -1)
				std::cerr << "SERVER ERROR: Failed closing epoll fd" << std::endl;
		}
		catch (...)
		{
			std::cerr << "SERVER ERROR: Server failed\n..server restarting.." << std::endl;
			if (close(server.getEpollFd()) == -1)
				std::cerr << "SERVER ERROR: Failed closing epoll fd" << std::endl;
		}
	}
	
	return (0);
}