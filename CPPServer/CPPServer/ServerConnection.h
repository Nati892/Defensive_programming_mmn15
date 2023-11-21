#pragma once
#include "FileUtils.h"


enum ServerMessageType 
{
	register_success_response=2100,
	register_failuer_response=2101,
	pub_rsa_received_sending_encrypted_aes=2102,
	file_received_sending_crc_checksum=2103,
	message_received=2104,
	reconnect_allowed_sending_aes_key=2105,
	reconnect_denied=2106,
	general_error=2107
};

enum ClientMessageType
{
	register_to_server_request = 1025,
	send_public_key_request=1026,
	reconnect_request=1027,
	send_file_request=1028,
	correct_crc_code_response=1029,
	invalid_crc_code_response=1030,
	terminate_connect_invalid_crc_code_response=1031,
};

struct ClientRequestMessageHeader {
	char ClientID[16];
	char version;
	__int16 Code;
	__int32 PayloadSize;
};

struct ServerResponseMessageHeader 
{
	char Version;
	__int16 Code;
	__int32 PayloadSize;
};


class ServerInstance {
private:
    TransferInfo TInfo;
    bool _SocketIsLive = false;

public:

    ServerInstance(TransferInfo TInfo)
    {
        this->TInfo = TInfo;
    }
	bool StartConnection();
};
