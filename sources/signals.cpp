#include "../headers/headers.hpp"

int	signal_pipe[2]; // To wake up epoll_wait for signals
int	got_signal = 0;	// To check the received signal

void signalHandler(int sig)
{
	char signal = static_cast<char>(sig);
	std::cout << "\nWriting signal byte to signal_pipe" << std::endl;
	write(signal_pipe[1], &signal, 1);
	got_signal = sig;
}

int	check_if_signal()
{
	char	buf;
	ssize_t	bytes_read = 0;

	bytes_read = read(signal_pipe[0], &buf, 1);
	if (bytes_read > 0)
		return (static_cast<int>(buf));
	return (0);
}

int	initialize_signals()
{
	std::signal(SIGINT, signalHandler);
	std::signal(SIGPIPE, signalHandler);
	std::signal(SIGQUIT, SIG_IGN);
	if (errno > 0)
		return (-1);
	return (0);
}

void	close_signal_pipe(int message)
{
	switch (message)
	{
		case 1:
			std::cerr << "ERROR: Failed setting signal_pipe[0] to nonblocking" << std::endl;
			break ;
		case 2:
			std::cerr << "ERROR: Failed initializing signals" << std::endl;
			break ;
		case 3:
			std::cerr << "Failed to add signal_pipe[0] to epoll" << std::endl;
			break ;
		default:
			break ;
	}

	if (signal_pipe[0] != -1 && close(signal_pipe[0]) == -1)
		std::cerr << "ERROR: Failed closing signal_pipe 0" << std::endl;
	signal_pipe[0] = -1;
	if (signal_pipe[1] != -1 && close(signal_pipe[1]) == -1)
		std::cerr << "ERROR: Failed closing signal_pipe 1" << std::endl;
	signal_pipe[1] = -1;	
}
