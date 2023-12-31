#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"



void RunClient()
{
	TransferInfo tInfo;
	MeInfo MInfo;
	ServerInstance* instance;
	ServerResponseMessage* rec_msg;
	std::string* AESKey;
	RSAPrivateWrapper* rsapriv;
	int BuffSize = 0;
	char* buff;
	bool SendSuccess = false;
	bool success_reg;
	if (!IfFileExists(TRANSFER_FILE_PATH))
	{
		//error no transfer file. // Log and create sample file
		//TODO log error:: transfer file doesnt exist, creating it
		auto file = CreateFileByPath(TRANSFER_FILE_PATH, true);
		if (file == nullptr)
		{
			//TODO log error couldnt create file
			return;
		}
		if (!writeToFile(file, DEFAULT_TRANSFER_INFO_DATA, true))
		{
			return;
			//TODO log error:: couldnt write to transfer file
		}
	}
	tInfo = parseTransferInfoFile(TRANSFER_FILE_PATH);
	//create server isntance based on files
	instance = new ServerInstance(tInfo);
	if (!instance->StartConnection())
	{
		//log error in connection
		delete instance;
		return;
	}

	if (!IfFileExists(ME_FILE_PATH))
	{//register
		//log couldnt find the me.info file , registering
		success_reg = RegisterClient(tInfo, instance, &MInfo, &AESKey, &rsapriv);
	}
	else
	{//attempt re-register
	 //auto MeInfoData =parse
		success_reg=ReconnectClient(tInfo, instance, &MInfo, &AESKey, &rsapriv);
	}
	
	if (!success_reg)
	{
		std::cout << "failed to register or connect to server" << std::endl;
		return;
	}

	//add file send, crc and loop for crc. and go through todos




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
		//TODO bad register request
		return false;
	}
	SendSuccess = instance->SendBufferToServer(buff, BuffSize);
	delete buff;
	rec_msg = instance->RecieveMessageFromServer();
	if (rec_msg == nullptr || rec_msg->Code != register_success_response || rec_msg->PayloadSize != CLIENT_ID_LENGTH)
	{
		//todo failed to register bye bye
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

		//todo check client id 
		//todo failed to register bye bye
		std::cerr << ("debug: register failed!") << std::endl;
		return false;
	}
	if (!compareClientId(*MInfo, rec_msg->payload))
	{
		std::cerr << ("debug: register failed! wrong client id") << std::endl;
		return false;
	}
	//check client id to be good

	int keylen = rec_msg->PayloadSize - CLIENT_ID_LENGTH;
	char* Enc_AES_Key = new char(keylen);
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
	if (KInfo->PrivKey.size() <= 0)
	{
		//handle error here
	}
	RSAPrivateWrapper* rsapriv = new RSAPrivateWrapper(KInfo->PrivKey);
	//todo handle case file data is not good
	//send reconnect request
	buff = CreateReconnectRequest(MInfo, &BuffSize);
	if (BuffSize == 0 || buff == nullptr)
	{
		//TODO bad register request
		return false;
	}
	SendSuccess = instance->SendBufferToServer(buff, BuffSize);
	delete buff;
	rec_msg = instance->RecieveMessageFromServer();
	if (rec_msg == nullptr || rec_msg->Code != reconnect_allowed_sending_aes_key || rec_msg->PayloadSize < CLIENT_ID_LENGTH)
	{

		//todo check client id 
		//todo failed to register bye bye
		std::cerr << ("debug: reconnect failed! deleting Me.info and private key data to do a register on the next run") << std::endl;
		deleteFile(ME_FILE_PATH);
		deleteFile(PRIV_KEY_PATH);
		return false;//todo re - register
	}
	if (!compareClientId(*MInfo, rec_msg->payload))
	{
		std::cerr << ("debug: reconnect failed! wrong client id") << std::endl;
		return false;//todo re - register
	}

	int keylen = rec_msg->PayloadSize - CLIENT_ID_LENGTH;
	char* Enc_AES_Key = new char(keylen);
	std::memcpy(Enc_AES_Key, rec_msg->payload + CLIENT_ID_LENGTH, keylen);
	std::string AESKey = rsapriv->decrypt(Enc_AES_Key, keylen);
	//delete Enc_AES_Key; //todo check why crash
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
	auto tmp = MInfo->AsciiIdentifier();
	tmp.c_str();
	std::memcpy(h.ClientID, tmp.c_str(), CLIENT_ID_LENGTH);
	h.Code = reconnect_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = 255;
	ReturnedBuff = h.SerializeToBuffer(MInfo->Name.c_str(), MInfo->Name.length() + 1, RetSize);
	return ReturnedBuff;
}

char* CreateRegisterRequest(MeInfo* MInfo, int* RetSize)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	h.Code = register_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = 255;
	ReturnedBuff = h.SerializeToBuffer(MInfo->Name.c_str(), MInfo->Name.length() + 1, RetSize);
	return ReturnedBuff;
}


char* CreateSendPubKeyRequest(MeInfo* MInfo, int* RetSize, std::string pubkey)
{
	char* ReturnedBuff = nullptr;
	ClientRequestMessageHeader h;
	auto tmp = MInfo->AsciiIdentifier();
	std::memcpy(h.ClientID, tmp.c_str(), CLIENT_ID_LENGTH);
	h.Code = send_public_key_request;
	h.version = CLIENT_VERSION;
	h.PayloadSize = SendPubKeyRequestPayloadSize;
	char payload[SendPubKeyRequestPayloadSize];//create payload buffer
	std::memcpy(payload, MInfo->Name.c_str(), MInfo->Name.length() + 1);//copy name
	std::memcpy(payload + 255, pubkey.c_str(), RSAPublicWrapper::KEYSIZE);//copy rsapubkey

	ReturnedBuff = h.SerializeToBuffer(payload, SendPubKeyRequestPayloadSize, RetSize);
	return ReturnedBuff;
}

bool compareClientId(MeInfo MInfo, char* buff)
{
	bool ret = false;
	auto toCmp = hexStringToAscii(MInfo.HexStrIdentifier.c_str());
	ret = std::memcmp(toCmp.c_str(), buff, CLIENT_ID_LENGTH) == 0;
	return ret;
}