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
		// void	checkErrorPageExist(std::string defRoot, std::string overrideRoot, std::vector<std::string> errorPath, int blockIndex, bool add);
		// void	addBaseValueIfNeedit();

};


