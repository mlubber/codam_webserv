#pragma once

# include <iostream>
# include <arpa/inet.h>
# include <cstring>
# include <unistd.h>
# include <fcntl.h>
# include <vector>
# include <map>
# include <sstream>
# include <fstream>
# include <sys/stat.h>
# include <sys/epoll.h>
# include <dirent.h>
# include <stdlib.h>
# include <sys/wait.h>
# include <memory>

void	setNonBlocking(int fd);