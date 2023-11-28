#pragma once
#include <string>

typedef struct TransferInfo {
    std::string ipAddress;
    int port;
    std::string dataString;
    std::string filePath;
};

TransferInfo parseFile(const std::string& filename);
bool IfFileExists(const std::string& name);