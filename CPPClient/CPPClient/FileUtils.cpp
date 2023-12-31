#include "FileUtils.h"

std::string AsciiIdentifier(MeInfo MInfo) {
	auto str = hexStringToAscii(MInfo.HexStrIdentifier.data(), MInfo.HexStrIdentifier.size());
	return str;
}

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
		Name = trim(MeLines[0]);
		Uid = trim(MeLines[1]);
		for (int i = 2; i < MeLines.size(); i++)
			PrivKey = PrivKey + MeLines[i];
	}
	if (Name.length() < 1 || Uid.length() != 32)//If faulty data return empty struct
	{
		PrivKey = "";
		Uid = "";
		Name = "";
	}
	meInfo.Name = Name;
	meInfo.HexStrIdentifier = Uid;
	meInfo.Privkey = PrivKey;
	return meInfo;
}

KeyInfo parseKeyInfoFile(const std::string& filename) {
	KeyInfo keyInfo;

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return keyInfo;
	}

	size_t BuffSize;
	auto data = GetFileData(filename, &BuffSize);
	// Parse the first line with string splitting
	std::string PrivKey = readStringFromFile(filename);
	if (PrivKey.size() < 1)
	{
		PrivKey = "";
	}
	keyInfo.PrivKey = PrivKey;
	return keyInfo;
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

std::vector<std::string> splitFirstTokens(const std::string& s, char delimiter, int amount) {
	std::vector<std::string> tokens;
	std::string token;
	int i = 0;

	// Use getline for the first part
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter) && i < amount) {
		tokens.push_back(token);
		i++;
	}

	// Use getc for the remaining part
	int c;
	int len = 0;
	while ((c = tokenStream.get()) != EOF) {
		if (c == delimiter) {
			break; // Stop at the first delimiter after reaching the specified amount
		}
		token += static_cast<char>(c);
		len++;
	}

	// Add the last token
	tokens.push_back(token);

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
		ReturnedBuffer = std::string(FileData);
		delete FileData;
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




//writes me info struct to file
bool MeInfo::SaveFile()
{
	bool write_success = false;
	std::ofstream file(ME_FILE_PATH);
	if (file.is_open()) { // Checking if the file is open
		file << Name << std::endl << HexStrIdentifier << std::endl << Privkey; // Writing content to the file
		file.close(); // Closing the file stream
		std::cerr << "Content written to " << ME_FILE_PATH << " successfully." << std::endl;
		write_success = true;
	}
	else {
		std::cerr << "Unable to create or open the file." << std::endl;
	}
	return write_success;
}

//writes key info struct to file
bool KeyInfo::SaveFile()
{
	writeStringToFile(this->PrivKey, PRIV_KEY_PATH);
	return true;
}



std::string readEntireFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);

	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return "";
	}

	// Read the entire content into a string
	std::ostringstream contentStream;
	contentStream << file.rdbuf();
	return contentStream.str();
}


bool readfile(std::string fname, char** buffer_out, int* buff_size_out) {
	if (std::filesystem::exists(fname)) {
		std::filesystem::path fpath = fname;
		std::ifstream f1(fname.c_str(), std::ios::binary);

		size_t size = std::filesystem::file_size(fpath);
		char* b = new char[size];
		f1.seekg(0, std::ios::beg);
		f1.read(b, size);
		*buffer_out = b;
		*buff_size_out = size;
		return true;
	}
	else {
		std::cerr << "Cannot open input file " << fname << std::endl;
		return false;
	}
}


std::string readStringFromFile(const std::string& filename) {
	// Open the file in binary mode
	std::ifstream inputFile(filename, std::ios::binary);

	if (!inputFile.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return "";
	}

	// Determine the size of the file
	inputFile.seekg(0, std::ios::end);
	size_t fileSize = static_cast<size_t>(inputFile.tellg());
	inputFile.seekg(0, std::ios::beg);

	// Read the contents of the file into a string
	std::string fileContents(fileSize, '\0');
	inputFile.read(&fileContents[0], fileSize);

	// Close the file
	inputFile.close();

	return fileContents;
}

void writeStringToFile(const std::string& myString, const std::string& filename) {
	// Open the file in binary mode
	std::ofstream outputFile(filename, std::ios::binary);

	if (!outputFile.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}

	// Write the string to the file
	outputFile.write(myString.c_str(), myString.size());

	// Close the file
	outputFile.close();
}

bool deleteFile(const char* filename) {
	if (std::remove(filename) == 0) {
		return true;
	}
	else {
		std::cerr << "Error deleting file" << std::endl;
		return false;
	}
}

bool compareClientId(MeInfo MInfo, char* buff)
{
	bool ret = false;
	auto toCmp = hexStringToAscii(MInfo.HexStrIdentifier.data(), MInfo.HexStrIdentifier.size());
	ret = std::memcmp(toCmp.c_str(), buff, CLIENT_ID_LENGTH) == 0;
	return ret;
}