#include "ServerConnection.h"
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

bool ServerInstance::StartConnection()
{
    //Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }

    // Specify the server address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->TInfo.port);  // Change this to the port your server is listening on

    // Convert IPv4 address from string to binary form
    if (inet_pton(AF_INET, this->TInfo.ipAddress.c_str(), &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Invalid address. Failed to convert the IP address." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Send a message to the server
    const char* message = "Hello, server!";
    if (send(clientSocket, message, strlen(message), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send data to the server." << std::endl;
    }
    else {
        std::cout << "Message sent to the server: " << message << std::endl;
    }

    // Clean up
    closesocket(clientSocket);
    WSACleanup();
}