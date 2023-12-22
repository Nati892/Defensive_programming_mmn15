#pragma once
#include <string>
#define ME_FILE_PATH "me.info"
#define TRANSFER_FILE_PATH "transfer.info"
#define PRIV_KEY_PATH "priv.key"
#define DEFAULT_TRANSFER_INFO_DATA "127.0.0.1:1234\nMichael Jackson\nSomeFileToSend"

typedef struct TransferInfo {
	std::string ipAddress;
	int port=0;
	std::string ClientName;
	std::string filePath;
};

typedef struct MeInfo {
	std::string Name;
	std::string AsciiIdentifier;
	std::string Privkey;
};
typedef struct KeyInfo {
	std::string AESKEy;
};



/// <summary>
/// returns a Transfer file struct based on the file in file path
/// </summary>
/// <param name="filename"> path to config Transfer.info file</param>
/// <returns>TransferInfoStruct, all field are empty if no file was found or if data is bad</returns>
TransferInfo parseTransferInfoFile(const std::string& filename);

/// <summary>
/// checks if file exists bt path
/// </summary>
/// <param name="path"> path to file</param>
/// <returns> if file in path open</returns>
bool IfFileExists(const std::string& path);

/// <summary>
/// creates a file and returns it open if returnOpenFile is true
/// </summary>
/// <param name="path"> path to file</param>
/// <param name="returnOpenFile"> if to return the file open </param>
/// <returns></returns>
std::ofstream* CreateFileByPath(const std::string& path, bool returnOpenFile);

/// <summary>
/// writes string to file
/// </summary>
/// <param name="fileStream">the file</param>
/// <param name="content">the content to write to file</param>
/// <param name="shouldCloseAndDisposeFile">if the function should close the file or or not</param>
/// <returns> if success in writing</returns>
bool writeToFile(std::ofstream* fileStream, const std::string& content, bool shouldCloseAndDisposeFile);