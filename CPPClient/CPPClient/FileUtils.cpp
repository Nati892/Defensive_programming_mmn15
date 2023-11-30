#include "FileUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <filesystem>


std::vector<std::string> split(const std::string& s, char delimiter);
std::string GetFileData(std::string fname, size_t* buffer_size);
TransferInfo parseTransferInfoFile(const std::string& filename) {
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

	std::getline(file, transferInfo.ClientName);
	transferInfo.ClientName = transferInfo.ClientName.substr(0, 100);  // Limit to 100 characters

	std::getline(file, transferInfo.filePath);

	file.close();
	return transferInfo;
}
std::ofstream* CreateFile(const std::string& path, bool returnOpenFile);
bool writeToFile(std::ofstream* fileStream, const std::string& content, bool shouldCloseFile);

MeInfo parseMeInfoFile(const std::string& filename) {
	MeInfo meInfo;

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return meInfo;
	}

	size_t BuffSize;
	auto data = GetFileData(filename, &BuffSize);
	// Parse the first line with string splitting
	std::string Name = "";
	std::string Uid = "";
	std::string PrivKey = "";
	std::vector<std::string> MeLines = split(data, '\n');
	if (MeLines.size() >= 3)
	{
		Name = MeLines[0];
		Uid = MeLines[1];
		PrivKey = MeLines[2];
	}
	if (Name.length() < 1 || Uid.length() != 16 || PrivKey.length() != 16)//If faulty data return empty struct
	{
		PrivKey = "";
		Uid = "";
		Name = "";
	}
	meInfo.Name = Name;
	meInfo.AsciiIdentifier = Uid;
	meInfo.Privkey = PrivKey;
	return meInfo;
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

bool IfFileExists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

std::string GetFileData(std::string fname, size_t* buffer_size) {
	size_t Size = 0;
	std::string ReturnedBuffer = "";
	if (std::filesystem::exists(fname)) {
		std::filesystem::path fpath = fname;
		std::ifstream f1(fname.c_str(), std::ios::binary);

		Size = std::filesystem::file_size(fpath);
		char* FileData = new char[Size];
		f1.seekg(0, std::ios::beg);
		f1.read(FileData, Size);
	}
	else {
		std::cerr << "GetFileData: Cannot open input file " << fname << std::endl;
	}

	*buffer_size = Size;
	return ReturnedBuffer;
}

std::ofstream* CreateFileByPath(const std::string& path, bool returnOpenFile)
{
	try
	{
		auto file = new std::ofstream();

		file->open(path, std::ios::app); // Open the file for writing, create if it doesn't exist
		if (!file->is_open())
		{
			// Failed to open the file, try creating it without opening
			file->open(path);
			if (!file->is_open())
			{
				delete file; // Clean up the allocated memory
				return nullptr; // Return nullptr if failed to open or create the file
			}
		}

		if (!returnOpenFile)
		{
			file->close(); // Close the file if not requested to return it open
		}

		return file;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return nullptr; // Return nullptr in case of an exception
	}
}


bool writeToFile(std::ofstream* fileStream, const std::string& content, bool shouldCloseAndDisposeFile)
{
	bool WriteSuccess = false;
	if (fileStream != nullptr && fileStream->is_open())
	{
		(*fileStream) << content;
		WriteSuccess = true;
		if (shouldCloseAndDisposeFile)
		{
			fileStream->close();
			delete fileStream; // Clean up the allocated memory
		}
	}
	else
	{
		std::cerr << "Invalid file stream or file not open." << std::endl;//log
	}
	return WriteSuccess;
}
