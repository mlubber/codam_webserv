#include "../headers/Server.hpp"

int	main()
{
	Server server;

	if (!server.initialize())
		return (1);
	server.run();

	return (0);
}