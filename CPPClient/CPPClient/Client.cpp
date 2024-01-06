#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"
#include "CRCChecksum.h"
#include "CryptoWrapper/AESWrapper.h"


void RunClient()
{
	TransferInfo tInfo;
	MeInfo MInfo;
	ServerInstance* instance;
	ServerResponseMessage* rec_msg;
	std::string* AESKey;
	RSAPrivateWrapper* rsapriv;
	int BuffSize = 0;
	char* buff = nullptr;
	bool SendSuccess = false;
	bool success_reg;
	if (!IfFileExists(TRANSFER_FILE_PATH))
	{
		//error no transfer file. // Log and create sample file
		std::cerr << "warning: transfer file doesnt exist, creating example file" << std::endl;
		auto file = CreateFileByPath(TRANSFER_FILE_PATH, true);
		if (file == nullptr)
		{
			std::cerr << "Error: cant create transfer file" << std::endl;
			return;
		}
		if (!writeToFile(file, DEFAULT_TRANSFER_INFO_DATA, true))
		{
			std::cerr << "Error: writing to transfer file" << std::endl;
			return;
		}
	}

	tInfo = parseTransferInfoFile(TRANSFER_FILE_PATH);
	std::cerr << "info: Read trasfer file" << std::endl;
	std::cerr << "info: client: " << tInfo.ClientName << std::endl;
	std::cerr << "info: file name " << tInfo.filePath << std::endl;
	
	//create server isntance based on files
	instance = new ServerInstance(tInfo);
	if (!instance->StartConnection())
	{
		std::cerr << "error: Starting connecion with remote server" << std::endl;
		delete instance;
		return;
	}
	std::cerr << "info: server connection started" << std::endl;

	if (!IfFileExists(ME_FILE_PATH))
	{//register
		std::cerr << "info: couldnt find Me.info file, trying to register" << std::endl;
		success_reg = RegisterClient(tInfo, instance, &MInfo, &AESKey, &rsapriv);
	}
	else
	{//attempt re-register

		std::cerr << "info: attempting reconnect with server" << std::endl;
		success_reg = ReconnectClient(tInfo, instance, &MInfo, &AESKey, &rsapriv);
	}

	if (!success_reg)
	{
		std::cout << "error: failed to register or connect to server" << std::endl;
		return;
	}
	std::cerr << "info: register or reconnect successful" << std::endl;
	std::string tmp(*AESKey);

	if (!IfFileExists(tInfo.filePath))
	{
		std::cerr << "error: bad transfer info file path" << std::endl;
		return;
	}

	int FileSize = 0;
	char* FileBuff;
	bool ReadSuccess = readfile(tInfo.filePath, &FileBuff, &FileSize);
	if (!ReadSuccess)
	{
		std::cerr << "error: reading file data" << std::endl;
		return;
	}
	unsigned long CRCCode = memcrc(FileBuff, FileSize);

	//create the request once because its cpu expensive and keep in case of retries
	int FileBuffSize = 0;
	int EncFileSize = 0;
	char* FileBuffRequest = CreateSendFileRequest(MInfo, tInfo, tmp, FileBuff, FileSize, &EncFileSize, &FileBuffSize);
	delete FileBuff;
	if (buff != nullptr && BuffSize > 0)
	{
		std::cerr << "error: creating send file request" << std::endl;
		delete[] FileBuffRequest;
		return;
	}

	bool CorrectCRCReceived = false;
	for (int i = 0; i < 4 && !CorrectCRCReceived; i++)
	{
		std::cerr << "info: file send attempt - " << i+1 << std::endl;
		instance->SendBufferToServer(FileBuffRequest, FileBuffSize);//send file message

		rec_msg = instance->RecieveMessageFromServer();//recieve response
		if (rec_msg == nullptr)
		{
			std::cerr << "info: Recieving crc msg from server" << std::endl;
			return;
		}

		if (rec_msg->Code == file_received_sending_crc_checksum && rec_msg->PayloadSize >= SendCRCChecksumReqBuffSize)
		{
			int offset = 0;
			char rec_CleintId[16];
			char rec_FileName[255];

			std::memcpy(rec_CleintId, rec_msg->payload, CLIENT_ID_LENGTH);
			offset += CLIENT_ID_LENGTH;

			int rec_file_Size = ConvertLittleEndianToInt32(rec_msg->payload + offset);
			offset += sizeof(int);

			std::memcpy(rec_FileName, rec_msg->payload + offset, FILE_NAME_SIZE);
			offset += FILE_NAME_SIZE;
			std::string rec_FileNameStr = bufferToString(rec_FileName, FILE_NAME_SIZE);

			unsigned long rec_crc = (unsigned long)ConvertLittleEndianToInt32(rec_msg->payload + offset);
			offset += sizeof(int);
			bool CID_check = std::memcmp(rec_CleintId, AsciiIdentifier(MInfo).c_str(), CLIENT_ID_LENGTH) == 0;
			bool crcCodeCheck = rec_crc == CRCCode;
			bool file_size_check = rec_file_Size == EncFileSize;
			bool file_path_check = (rec_FileNameStr.size() > 0 && rec_FileNameStr == tInfo.filePath);
			if (CID_check && crcCodeCheck && file_size_check && file_path_check)
			{
				CorrectCRCReceived = true;
				std::cerr << "info: file send success!" << std::endl;
				std::cerr << "info: crc checksum success!" << std::endl;
			}
		}
		else
		{//send bad crc request
			if (i == 3)//in case third time of resending
				break;
			int send_buff_size = 0;
			char* sendCRCBuff = nullptr;
			sendCRCBuff = CreateBadCRCRequestRetry(&MInfo, &send_buff_size, tInfo);
			if (sendCRCBuff == nullptr || send_buff_size <= 0)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				if (sendCRCBuff != nullptr)
					delete[] sendCRCBuff;
				delete FileBuffRequest;
				return;
			}
			SendSuccess = instance->SendBufferToServer(sendCRCBuff, send_buff_size);
			delete[] sendCRCBuff;

			if (!SendSuccess)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				delete[] FileBuffRequest;
				return;
			}


		}
		//cleanup old stuff
		delete[] FileBuffRequest;
		delete rec_msg;

		//sned final result for crc checksum
		int send_buff_size = 0;
		char* sendCRCBuff = nullptr;
		//finished send file loop
		if (!CorrectCRCReceived)//if retried and nothing then send abort message
		{
			sendCRCBuff = CreateBadCRCRequestTerminateConnection(&MInfo, &send_buff_size, tInfo);
			if (sendCRCBuff == nullptr || send_buff_size <= 0)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				if (sendCRCBuff != nullptr)
					delete[] sendCRCBuff;
				return;
			}
			SendSuccess = instance->SendBufferToServer(sendCRCBuff, send_buff_size);
			delete[] sendCRCBuff;

			if (!SendSuccess)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				return;
			}
		}
		else
		{
			sendCRCBuff = CreateCorrectRCRRequestRetry(&MInfo, &send_buff_size, tInfo);
			if (sendCRCBuff == nullptr || send_buff_size <= 0)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				if (sendCRCBuff != nullptr)
					delete[] sendCRCBuff;
				return;
			}
			SendSuccess = instance->SendBufferToServer(sendCRCBuff, send_buff_size);
			delete[] sendCRCBuff;

			if (!SendSuccess)
			{
				std::cerr << "Error: creating bad crc gotten from server response to server" << std::endl;
				return;
			}
		}

		rec_msg = instance->RecieveMessageFromServer();
		if (rec_msg == nullptr || rec_msg->Code != message_received)
		{
			std::cerr << "Warning: missing message received from server" << std::endl;
		}
		std::cerr << "info: client exiting" << std::endl;
	}

}

bool RegisterClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out)
{
	TransferInfo tInfo = TInfo;
	MeInfo* MInfo = new MeInfo;
	KeyInfo KInfo;
	ServerInstance* instance = server;
	ServerResponseMessage* rec_msg;
	int BuffSize = 0;
	char* buff;
	bool SendSuccess = false;

	MInfo->Name = tInfo.ClientName;
	buff = CreateRegisterRequest(MInfo, &BuffSize);
	if (BuffSize == 0 || buff == nullptr)
	{
		std::cerr << "Error: bad register request created" << std::endl;
		return false;
	}
	SendSuccess = instance->SendBufferToServer(buff, BuffSize);
	delete buff;
	rec_msg = instance->RecieveMessageFromServer();
	if (rec_msg == nullptr || rec_msg->Code != register_success_response || rec_msg->PayloadSize != CLIENT_ID_LENGTH)
	{
		std::cerr << "Warning: server rejected registe request" << std::endl;
		std::cerr << ("debug: register failed!") << std::endl;
		return false;
	}
	std::cerr << ("debug: register success!") << std::endl;
	MInfo->HexStrIdentifier = AsciiToHexStr(rec_msg->payload);//save public identifier
	delete rec_msg;
	//start rs and send rsa pub key
	//generate rsa key-pair 
	RSAPrivateWrapper* rsapriv = new RSAPrivateWrapper;

	std::string pubkey = rsapriv->getPublicKey();
	std::string privKey = rsapriv->getPrivateKey();
	std::string privKeyb64 = Base64Wrapper::encode(privKey);
	/*MInfo.Privkey = privKeyb64;
	std::string uncodedtst = Base64Wrapper::decode(privKeyb64);*/
	MInfo->Privkey = privKeyb64;
	bool SuccessSaved = MInfo->SaveFile();
	KInfo.PrivKey = privKey;
	SuccessSaved = KInfo.SaveFile();

	if (!SuccessSaved)
	{
		std::cerr << "Error saving My info file" << std::endl;
	}
	//send public key to server
	buff = CreateSendPubKeyRequest(MInfo, &BuffSize, pubkey);

	SendSuccess = instance->SendBufferToServer(buff, BuffSize);
	delete buff;
	//receive aes key from server
	rec_msg = instance->RecieveMessageFromServer();

	if (rec_msg == nullptr || rec_msg->Code != pub_rsa_received_sending_encrypted_aes || rec_msg->PayloadSize < MIN_2102_PAYLOAD_SIZE)
	{
		std::cerr << ("warning : register failed!") << std::endl;
		return false;
	}
	if (!compareClientId(*MInfo, rec_msg->payload))
	{
		std::cerr << ("debug: register failed! wrong client id received from server") << std::endl;
		return false;
	}
	//check client id to be good
	std::cerr << "info: register success!, received aes key" << std::endl;
	int keylen = rec_msg->PayloadSize - CLIENT_ID_LENGTH;
	char* Enc_AES_Key = new char[keylen];
	std::memcpy(Enc_AES_Key, rec_msg->payload + CLIENT_ID_LENGTH, keylen);
	std::string AESKey = rsapriv->decrypt(Enc_AES_Key, keylen);

	*MInfo_out = *MInfo;
	*AESKey_out = new std::string(AESKey);
	*Rsa_Encryptor_out = rsapriv;
}

bool ReconnectClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out)
{
	bool success = false;
	MeInfo* MInfo = new MeInfo;
	*MInfo = parseMeInfoFile(ME_FILE_PATH);
	KeyInfo* KInfo = new KeyInfo;
	*KInfo = parseKeyInfoFile(PRIV_KEY_PATH);
	ServerInstance* instance = server;
	ServerResponseMessage* rec_msg;
	bool SendSuccess;
	int BuffSize = 0;
	char* buff;
	RSAPrivateWrapper* rsapriv = nullptr;
	if (KInfo->PrivKey.size() <= 0)
	{
		//handle error here
	}
	try {
		rsapriv = new RSAPrivateWrapper(KInfo->PrivKey);
	}
	catch (std::exception& e)
	{
		std::cerr << "warning: failed to create rsa codec from key in private.key file: " << e.what() << std::endl;
		success = false;
		return success;
	}
	//send reconnect request
	buff = CreateReconnectRequest(MInfo, &BuffSize);
	if (BuffSize == 0 || buff == nullptr)
	{
		std::cerr << "Error: bad register request" << std::endl;
		return false;
	}
	SendSuccess = instance->SendBufferToServer(buff, BuffSize);
	delete buff;
	rec_msg = instance->RecieveMessageFromServer();
	if (rec_msg == nullptr || rec_msg->Code != reconnect_allowed_sending_aes_key || rec_msg->PayloadSize < CLIENT_ID_LENGTH)
	{

		std::cerr << ("debug: reconnect failed! deleting Me.info and private key data to do a register on the next run") << std::endl;
		deleteFile(ME_FILE_PATH);
		deleteFile(PRIV_KEY_PATH);
		return false;
	}
	if (!compareClientId(*MInfo, rec_msg->payload))
	{
		std::cerr << ("debug: reconnect failed! wrong client id") << std::endl;
		return false;
	}
	std::cerr << "info: Reconnect success received AES Key" << std::endl;
	int keylen = rec_msg->PayloadSize - CLIENT_ID_LENGTH;
	char* Enc_AES_Key = new char[keylen];
	std::memcpy(Enc_AES_Key, rec_msg->payload + CLIENT_ID_LENGTH, keylen);
	std::string AESKey = rsapriv->decrypt(Enc_AES_Key, keylen);
	delete rec_msg;

	*MInfo_out = *MInfo;
	*AESKey_out = new std::string(AESKey);
	*Rsa_Encryptor_out = rsapriv;

	success = true;
	return success;
}

char* CreateReconnectRequest(MeInfo* MInfo, int* RetSize)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(*MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = reconnect_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = 255;
	ReturnedBuff = h.SerializeToBuffer(MInfo->Name.data(), MInfo->Name.length() + 1, RetSize);
	return ReturnedBuff;
}

char* CreateRegisterRequest(MeInfo* MInfo, int* RetSize)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	h.Code = register_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = 255;
	ReturnedBuff = h.SerializeToBuffer(MInfo->Name.data(), MInfo->Name.length() + 1, RetSize);
	return ReturnedBuff;
}

char* CreateSendPubKeyRequest(MeInfo* MInfo, int* RetSize, std::string pubkey)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(*MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = send_public_key_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = SendPubKeyRequestPayloadSize;
	char payload[SendPubKeyRequestPayloadSize];//create payload buffer
	std::memcpy(payload, MInfo->Name.data(), MInfo->Name.length() + 1);//copy name
	std::memcpy(payload + 255, pubkey.data(), RSAPublicWrapper::KEYSIZE);//copy rsapubkey

	ReturnedBuff = h.SerializeToBuffer(payload, SendPubKeyRequestPayloadSize, RetSize);
	return ReturnedBuff;
}

char* CreateSendFileRequest(MeInfo MInfo, TransferInfo TInfo, std::string AESKey, char* FileData, int FileSize, int* EncFileSize_out, int* RetSize_out)
{
	char* ReturnedBuff;
	char* buff = nullptr;
	*RetSize_out = 0;

	//encrypt file
	unsigned char aeskey[AESWrapper::DEFAULT_KEYLENGTH];
	std::memcpy(aeskey, AESKey.data(), AESWrapper::DEFAULT_KEYLENGTH);
	AESWrapper aesEncryptor(aeskey, AESWrapper::DEFAULT_KEYLENGTH);
	std::string EncryptedFile = aesEncryptor.encrypt(FileData, FileSize);

	int EncFileSize = EncryptedFile.size();
	*EncFileSize_out = EncFileSize;
	//calc payload size
	auto PayloadSize = sizeof(int) + FILE_NAME_SIZE + EncFileSize;
	buff = new char[PayloadSize];
	std::memset(buff, 0, PayloadSize);

	char* FileSizeLength = ConvertInt32ToEndian(EncFileSize);//4 bytes of file size length
	char file_name[FILE_NAME_SIZE];//file size

	//copy size
	int msgBuffLocation = 0;
	std::memcpy(buff, FileSizeLength, sizeof(int));
	msgBuffLocation += sizeof(int);

	//copy file name
	std::memcpy(buff + msgBuffLocation, TInfo.filePath.data(), min(TInfo.filePath.size(), FILE_NAME_SIZE));
	msgBuffLocation += FILE_NAME_SIZE;

	//copy encrypted file
	std::memcpy(buff + msgBuffLocation, EncryptedFile.data(), EncryptedFile.size());

	//createMessage
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = send_file_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = PayloadSize;
	ReturnedBuff = h.SerializeToBuffer(buff, h.PayloadSize, RetSize_out);
	//	delete FileSizeLength;//crashing for some reason

	return ReturnedBuff;
}

char* CreateBadCRCRequestRetry(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(*MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = invalid_crc_code_response;
	h.version = CLIENT_VERSION;
	h.PayloadSize = FILE_NAME_SIZE;
	char payload[FILE_NAME_SIZE];//create payload buffer
	std::memset(payload, 0, FILE_NAME_SIZE);
	std::memcpy(payload, Tinfo.filePath.data(), Tinfo.filePath.size());//copy name

	ReturnedBuff = h.SerializeToBuffer(payload, SendPubKeyRequestPayloadSize, RetSize);
	return ReturnedBuff;
}

char* CreateCorrectRCRRequestRetry(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(*MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = correct_crc_code_response;
	h.version = CLIENT_VERSION;
	h.PayloadSize = FILE_NAME_SIZE;
	char payload[FILE_NAME_SIZE];//create payload buffer
	std::memset(payload, 0, FILE_NAME_SIZE);
	std::memcpy(payload, Tinfo.filePath.data(), Tinfo.filePath.size());//copy name

	ReturnedBuff = h.SerializeToBuffer(payload, SendPubKeyRequestPayloadSize, RetSize);
	return ReturnedBuff;
}

char* CreateBadCRCRequestTerminateConnection(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = AsciiIdentifier(*MInfo);
	std::memcpy(h.ClientID, tmp.data(), CLIENT_ID_LENGTH);
	h.Code = invalid_crc_code_response_terminate_connection;
	h.version = CLIENT_VERSION;
	h.PayloadSize = FILE_NAME_SIZE;
	char payload[FILE_NAME_SIZE];//create payload buffer
	std::memset(payload, 0, FILE_NAME_SIZE);
	std::memcpy(payload, Tinfo.filePath.data(), Tinfo.filePath.size());//copy name

	ReturnedBuff = h.SerializeToBuffer(payload, SendPubKeyRequestPayloadSize, RetSize);
	return ReturnedBuff;
}