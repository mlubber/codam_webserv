#include "../headers/headers.hpp"
#include "../headers/Server.hpp"
#include "../headers/Configuration.hpp"
#include "../headers/ValidationConfigFile.hpp"
#include "../headers/Host.hpp"

int	setNonBlocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
	{
		std::cerr << "SERVER ERROR: fcntl F_GETFL flag failed\n";
		return (-1);
	}
	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		std::cerr << "SERVER ERROR: fcntl F_SETFL flag failed\n";
		return (-1);
	}
	return (0);
}

std::string joinPaths(const std::string &path, const std::string &file)
{
	if (!path.empty() && *path.rbegin() == '/')
		return (path + file);
	return (path + "/" + file);
}

std::string	getExtType(const std::string& filename)
{
	std::map<std::string, std::string> types = 
	{
		{".html", "text/html"},
		{".css", "text/css"},
		{".js", "application/javascript"},
		{".png", "image/png"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".gif", "image/gif"},
		{".txt", "text/plain"}
	};

	size_t dotPos = filename.find_last_of('.');

	if (dotPos != std::string::npos)
	{
		std::string extension = filename.substr(dotPos);
		if (types.find(extension) != types.end())
			return (types[extension]);
	}
	return ("application/octet-stream"); //
}

std::string urlDecode(const std::string& encoded)
{
	std::ostringstream decoded;
	for (size_t i = 0; i < encoded.length(); ++i)
	{
		if (encoded[i] == '%' && i + 2 < encoded.length())
		{
			std::istringstream hexStream(encoded.substr(i + 1, 2));
            int hexValue;
            hexStream >> std::hex >> hexValue;
            decoded << static_cast<char>(hexValue);
            i += 2;
		}
		else if (encoded[i] == '+')
			decoded << ' ';
		else
			decoded << encoded[i];
	}
	return(decoded.str());
}

std::vector<int> convertToInt(std::vector<std::string> &portsStr)
{

	std::vector<int> portsInt;
	for (size_t i = 0; i <portsStr.size(); ++i)
	{
		try
		{
			portsInt.push_back(std::stoi(portsStr[i]));
		}
		catch(const std::exception &e) 
		{
			std::cerr << "SERVER ERROR: Port to int conversion failed: " << e.what() << std::endl;
		}
	}
	return (portsInt);
}

std::string ip_to_string(struct in_addr addr) 
{
	const unsigned char *bytes = reinterpret_cast<const unsigned char *>(&addr.s_addr);
    std::stringstream ss;
    ss << (int)bytes[0] << "."
       << (int)bytes[1] << "."
       << (int)bytes[2] << "."
       << (int)bytes[3];
    return ss.str();
}

std::string	extract_first_word(std::string path)
{
	std::string word;

	if (path.size() > 1)
	{
		std::string::size_type first = path.find('/');
		if (first != std::string::npos)
		{
			std::string::size_type second = path.find('/', first + 1);
			if (second != std::string::npos)
				word = path.substr(first, second + 1 - first);
			else
			{
				word = path.substr(first);
				std::string::size_type dot = word.find('.');
				if (dot != std::string::npos)
					word = path.substr(0, 1);
				if (!word.empty() && *word.rbegin() != '/')
					word.append("/");
			}
		}
	}
	else
		return (path);
	return (word);
}

int	check_path(std::string filePath, std::string locPath)
{
	struct stat stats;
	if (stat(filePath.c_str(), &stats) == 0)
	{
		if (S_ISDIR(stats.st_mode))
		{
			if (locPath.empty())
				return (1);
			return (0);
		}
		else if (S_ISREG(stats.st_mode))
		{
			if (locPath.empty())
				return (1);
			return (0);
		}
	}
	return (1);
}

