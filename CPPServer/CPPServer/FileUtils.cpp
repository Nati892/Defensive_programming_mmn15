#include "FileUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>


std::vector<std::string> split(const std::string& s, char delimiter);

TransferInfo parseFile(const std::string& filename) {
	TransferInfo transferInfo;

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return transferInfo;
	}

	// Parse the first line with string splitting
	std::string firstLine;
	if (std::getline(file, firstLine)) {
		std::vector<std::string> tokens = split(firstLine, ':');

		if (tokens.size() == 2) {
			transferInfo.ipAddress = tokens[0];
			try {
				transferInfo.port = std::stoi(tokens[1]);
			}
			catch (const std::exception& e) {
				std::cerr << "Error parsing port. Line: " << firstLine << std::endl;
				file.close();
				return transferInfo;
			}
		}
		else {
			std::cerr << "Error parsing IP address and port. Line: " << firstLine << std::endl;
			file.close();
			return transferInfo;
		}
	}
	else {
		std::cerr << "Error reading the first line." << std::endl;
		file.close();
		return transferInfo;
	}

	std::getline(file, transferInfo.dataString);
	transferInfo.dataString = transferInfo.dataString.substr(0, 100);  // Limit to 100 characters

	std::getline(file, transferInfo.filePath);

	file.close();
	return transferInfo;
}


std::vector<std::string> split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream tokenStream(s);
	std::string token;
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}


FILE* ReadMeData()
{
	FILE* pMeFile = nullptr;
	//std::fopen
	return pMeFile;
}


bool IfFileExists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}