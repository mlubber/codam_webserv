#pragma once
#include "Configuration.hpp"



class Serve
{
private:
	ConfigBlock &_configData;
public:
	Serve(ConfigBlock &config);
	
	void answerRequest(std::string host, std::string port, std::string error_page, std::string index);
	~Serve();
	Serve(const Serve& other);
	Serve& operator=(const Serve& other);
};


