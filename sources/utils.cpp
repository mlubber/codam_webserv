#include "../headers/headers.hpp"
#include "../headers/Server.hpp"
#include "../headers/Configuration.hpp"
#include "../headers/ValidationConfigFile.hpp"
#include "../headers/Serve.hpp"

int	setNonBlocking(int fd)
{
	std::cout << "Setting non-blocking mode for fd: " << fd << std::endl;
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
	{
		std::cerr << flag << " fcntl get failed\n";
		return (-1);
	}
	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl set failed\n";
		return (-1);
	}
	return (0);
}