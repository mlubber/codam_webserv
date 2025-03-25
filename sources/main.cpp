#include "../headers/Server.hpp"


// int	main()
// {
	// Server server;
// int	main(void)
// {
// 	Server server;

// 	if (!server.initialize())
// 		return (1);
// 	server.run();

// 	return (0);
// }



#include "../headers/Configuration.hpp"
#include "../headers/ValidationConfigFile.hpp"
#include "../headers/Serve.hpp"

// void mainLoop()
// {
// 	_epoll_fd = epoll_create1(0);
// 	while (true)
// 	{
// 		epoll_wait()
// 		handle_event();
// 	}
// }

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
	myconfig.printConfig(myconfig.getConfigData(), 0);




	ValidationConfigFile	validator(myconfig.getConfigData());
	std::cout << "printing all server blocks after check and add all base key value!\n\n\n\n" << std::endl;
	// validator.printConfig(validator.getConfig(), 0);
	myconfig.printConfig(myconfig.getConfigData(), 0);

	Serve	serveRequest(myconfig.getConfigData());
	serveRequest.answerRequest("127.0.0.1", "80", "", "");
	
	std::vector<std::string> configports;
	configports = myconfig.getConfigValues(myconfig.getConfigData(), "listen");

	Server server;
	
	std::vector<int> ports;

	for (const std::string& str : configports)
		ports.push_back(std::stoi(str));

	if (!server.initialize(ports))
		return (1);
	server.run();

	return (0);
}
