#include "ServerConnection.h"
#include "Utils.h"
#include <string>
#include <WS2tcpip.h>
#include <iostream>
#include <algorithm>
#ifndef  min
#define min std::min
#endif //  min


bool IsSocketOpen(SOCKET socket);
bool ServerInstance::StartConnection()
{
	bool ConnectSuccess = false;
	//Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize Winsock." << std::endl;
		return ConnectSuccess;
	}

	// Create a socket
	clientSocket = new SOCKET(socket(AF_INET, SOCK_STREAM, 0));
	if (*clientSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket." << std::endl;
		clientSocket = nullptr;
		WSACleanup();
		return ConnectSuccess;
	}

	// Specify the server address and port
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(this->TInfo.port);  // Change this to the port your server is listening on

	// Convert IPv4 address from string to binary form
	if (inet_pton(AF_INET, this->TInfo.ipAddress.c_str(), &(serverAddr.sin_addr)) <= 0) {
		std::cerr << "Invalid address. Failed to convert the IP address." << std::endl;
		closesocket(*clientSocket);
		WSACleanup();
		return ConnectSuccess;
	}

	// Connect to the server
	if (connect(*clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Failed to connect to the server." << std::endl;
		closesocket(*clientSocket);
		WSACleanup();
		return ConnectSuccess;
	}
	ConnectSuccess = true;
	return ConnectSuccess;
}

void ServerInstance::CloseConnection() {
	if (clientSocket != nullptr && *clientSocket != INVALID_SOCKET) {
		closesocket(*clientSocket);
		delete clientSocket;
		clientSocket = nullptr;
	}
}

bool ServerInstance::SendMessageToServer(ClientRequestMessageHeader header, char* payload)
{
	bool SendSuccess = false;
	int Buffsize = sizeof(header) + header.PayloadSize;
	char* BuffSend = new char[Buffsize];

	if (SendSuccess = IsSocketOpen(*(this->clientSocket)))
	{
		SendSuccess = SendBufferToServer(BuffSend, Buffsize);
	}
	free(BuffSend);
	return SendSuccess;
}

bool ServerInstance::SendBufferToServer(const char* BuffToSend, int size)
{
	bool success = false;

	try {
		if (success = send(*clientSocket, BuffToSend, size, 0) == SOCKET_ERROR) {
			std::cerr << "Failed to send data to the server." << std::endl;
		}
		else {
			std::cerr << "Message sent to the server" << std::endl;
			success = true;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error sending message" << std::endl;
		success = false;
	}

	return success;
}

bool IsSocketOpen(SOCKET socket) {
	int error = 0;
	socklen_t len = sizeof(error);

	int result = getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);

	if (result != 0) {
		return false;
	}

	return error == 0;
}

char* ClientRequestMessageHeader::SerializeToBuffer(const char* Payload, int buffsize, int* RetSize)
{
	int NewBuffSize = sizeof(this->ClientID)+sizeof(this->Code)+sizeof(this->version)+sizeof(this->PayloadSize) + PayloadSize;
	char* buffer = new char[NewBuffSize];
	int i = 0;
	char* TmpBuff;

	//serialize and memcpy the ClientId field
	std::memcpy(buffer, ClientID, sizeof(ClientID));
	i += sizeof(ClientID);

	//no need to serizlize, memcpy the version field
	std::memcpy(buffer + i, &version, sizeof(version));
	i += sizeof(version);

	//serialize and memcpy the Code field
	TmpBuff = ConvertInt16ToEndian(Code);
	std::memcpy(buffer + i, TmpBuff, sizeof(Code));
	delete TmpBuff;
	i += sizeof(Code);

	//serialize and memcpy the PayloadSize field
	TmpBuff = ConvertInt32ToEndian(PayloadSize);
	std::memcpy(buffer + i, TmpBuff, sizeof(PayloadSize));
	delete TmpBuff;
	i += sizeof(PayloadSize);


	*RetSize = i+PayloadSize;//return the size of buffer
	int CopySize = min(PayloadSize, buffsize);
	std::memcpy(buffer + i, Payload, CopySize);
	i += CopySize;
	return buffer;
}

