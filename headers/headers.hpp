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

class Client;
class Server;
struct clRequest;
struct ConfigBlock;

void		setNonBlocking(int fd);
void		parsingRequest(Server& server, Client& client);
std::string	generateHttpResponse(clRequest& cl_request, const ConfigBlock& serverBlock);
