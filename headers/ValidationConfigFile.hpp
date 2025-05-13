#pragma once
#include "Configuration.hpp"


class ValidationConfigFile
{
	private:

		ConfigBlock		&_configData;

	public:

		ValidationConfigFile(ConfigBlock &configData);
		ValidationConfigFile(const ValidationConfigFile& other);
		ValidationConfigFile& operator=(const ValidationConfigFile& other);
		~ValidationConfigFile();

		void	duplicateKey();

};


