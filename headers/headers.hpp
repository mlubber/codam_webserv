#pragma once

# include <iostream>
# include <arpa/inet.h>
# include <cstring>
# include <unistd.h>
# include <fcntl.h>
# include <vector>
# include <map>
# include <set>
# include <netdb.h>
# include <sstream>
# include <fstream>
# include <sys/stat.h>
# include <sys/epoll.h>
# include <dirent.h>
# include <stdlib.h>
# include <sys/wait.h>
# include <memory>
# include <algorithm>
# include <csignal>

class Client;
class Server;
struct clRequest;
struct ConfigBlock;

extern int	signal_pipe[2]; // To wake up epoll_wait for signals
extern int	got_signal;		// To check the received signal

int					initialize_signals();
int					check_if_signal();
void				handleReceivedSignal(Server& server);
void				close_signal_pipe(int message);


int					setNonBlocking(int fd);
std::string			joinPaths(const std::string &path, const std::string &file);
std::string			getExtType(const std::string& filename);
std::string 		urlDecode(const std::string& encoded);
std::vector<int>	convertToInt(std::vector<std::string> &portsStr);
int					parsingRequest(Server& server, Client& client);

void				close_webserv(Server& server);
