#pragma once
#include "FileUtils.h"
#include <WinSock2.h>   


enum ServerMessageType :__int16
{
	register_success_response = 2100,
	register_failuer_response = 2101,
	pub_rsa_received_sending_encrypted_aes = 2102,
	file_received_sending_crc_checksum = 2103,
	message_received = 2104,
	reconnect_allowed_sending_aes_key = 2105,
	reconnect_denied = 2106,
	general_error = 2107
};

enum ClientMessageType :__int16
{
	register_request = 1025,
	send_public_key_request = 1026,
	reconnect_request = 1027,
	send_file_request = 1028,
	correct_crc_code_response = 1029,
	invalid_crc_code_response = 1030,
	terminate_connect_invalid_crc_code_response = 1031,
};

struct ClientRequestMessageHeader {
	unsigned char ClientID[16];
	unsigned char version;
	unsigned short Code;
	unsigned int PayloadSize;
	char* SerializeToBuffer(const char* Payload,int PayloadSize, int* RetSize);
};

struct ServerResponseMessageHeader
{
	unsigned char Version;
	unsigned short Code;
	unsigned int PayloadSize;
	ServerResponseMessageHeader() {}
};

class ServerInstance {
private:
	TransferInfo TInfo;
	bool _SocketIsLive = false;
	SOCKET* clientSocket = nullptr;
public:

	ServerInstance(TransferInfo TInfo)
	{
		this->TInfo = TInfo;
	}
	~ServerInstance()
	{
		// Clean up
		closesocket(*clientSocket);
		WSACleanup();
		delete clientSocket;
	}
	bool StartConnection();
	void CloseConnection();
	bool SendMessageToServer(ClientRequestMessageHeader header, char* payload);
	bool SendBufferToServer(const char* payload, int size);
	bool HandleRecievedMessage();
};
