#include "../headers/Server.hpp"

int	main(int argc, char** argv, char** envp)
{
	Server server(envp);

	/* temp unused variable error suppression */
	if (argc != 1)
		exit(1);
	(void)argv;
	/////////////////

	if (!server.initialize())
		return (1);
	server.run();

	return (0);
}