#include "ServerConnection.h"
#include "Utils.h"
#include <string>
#include <WS2tcpip.h>
#include <iostream>
#include <algorithm>

#ifndef  min
#define min std::min
#endif //  min


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
	if (inet_pton(AF_INET, this->TInfo.ipAddress.data(), &(serverAddr.sin_addr)) <= 0) {
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
	delete[] BuffSend;
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

bool ServerInstance::HandleRecievedMessage()
{

	return false;
}

ServerResponseMessage* ServerInstance::RecieveMessageFromServer()
{
	// Example of receiving data
	size_t BytesToRead = 0;
	int rec_buffer_left_size = BUFFER_SIZE;
	int rec_buffer_left_index = 0;
	char receivedBuffer[BUFFER_SIZE];  // Assuming a buffer of size BUFFER_SIZE

	if (this->SavedDataBufferSize > 0)//if there is stored data from last message
	{
		std::memcpy(receivedBuffer, this->SavedDataBuffer, SavedDataBufferSize);
		BytesToRead += SavedDataBufferSize;
		rec_buffer_left_size -= this->SavedDataBufferSize;
	}
	while (BytesToRead < SERVER_MESSAGE_HEADER_SIZE && rec_buffer_left_size>0)//get header size at least, this loop fills up buff until it reaches at least the header size
	{
		BytesToRead += recv(*(this->clientSocket), receivedBuffer + (BUFFER_SIZE - rec_buffer_left_size), rec_buffer_left_size, 0);
		if (BytesToRead < 0)
		{
			return nullptr;
		}
		rec_buffer_left_size -= BytesToRead;
	}
	ServerResponseMessage* header = new ServerResponseMessage;

	if (IsLittleEndian())
	{
		header->Version = (char)receivedBuffer[0];
		std::memcpy(&header->Code, receivedBuffer + 1, sizeof(unsigned short));
		std::memcpy(&header->PayloadSize, receivedBuffer + 3, sizeof(int));
	}
	else//if local machine is big endian
	{
		header->Version = receivedBuffer[0];
		header->Code = (receivedBuffer[2] << 8) | receivedBuffer[1];
		header->PayloadSize =
			(unsigned int)
			((receivedBuffer[6] << 24) |
				(receivedBuffer[5] << 16) |
				(receivedBuffer[4] << 8) |
				receivedBuffer[3]);
	}

	if (header->PayloadSize <= 0)//if no payload return
	{
		header->payload = nullptr;
		return header;
	}

	BytesToRead -= SERVER_MESSAGE_HEADER_SIZE;//went 7 steps right in rec buffer
	rec_buffer_left_index += SERVER_MESSAGE_HEADER_SIZE;
	int PayloadCopyIndex = 0;
	char* Payload = nullptr;
	if (header->PayloadSize > 0)
	{
		Payload = new char[header->PayloadSize];
		header->payload = Payload;
	}
	while (PayloadCopyIndex < header->PayloadSize)
	{
		int BytesToCopy = min(BytesToRead, header->PayloadSize - PayloadCopyIndex);
		if (BytesToRead > 0)//if there is something in buffer, read it.
		{
			std::memcpy(Payload + PayloadCopyIndex, receivedBuffer + rec_buffer_left_index, BytesToCopy);
			rec_buffer_left_index += BytesToCopy;
			BytesToRead -= BytesToCopy;
			PayloadCopyIndex += BytesToCopy;
		}
		else//if buffer is empty then recv and re-loop
		{
			BytesToRead = 0;
			BytesToRead += recv(*(this->clientSocket), receivedBuffer, BUFFER_SIZE, 0);
			if (BytesToRead < 0)
			{
				delete[] Payload;
				delete header;
				return nullptr;
			}
			rec_buffer_left_index = 0;
		}
	}
	if (BytesToRead > 0)//if there is still data from the next message, store it and use it next time
	{
		std::memcpy(this->SavedDataBuffer, receivedBuffer + rec_buffer_left_index, BytesToRead);
		this->SavedDataBufferSize = BytesToRead;
	}
	return header;
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

char* ClientRequestMessageHeader::SerializeToBuffer(char* Payload, int buffsize, int* RetSize)
{
	int NewBuffSize = sizeof(this->ClientID) + sizeof(this->Code) + sizeof(this->version) + sizeof(this->PayloadSize) + PayloadSize;
	char* buffer = new char[NewBuffSize];
	int i = 0;
	char* TmpBuff;

	//serialize and memcpy the ClientId field
	std::memcpy(buffer, ClientID, CLIENT_ID_LENGTH);
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


	*RetSize = i + PayloadSize;//return the size of buffer
	int CopySize = min(PayloadSize, buffsize);
	std::memcpy(buffer + i, Payload, CopySize);
	i += CopySize;
	return buffer;
}

