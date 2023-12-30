#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"
#include "CryptoWrapper/RSAWrapper.h"
#include "CryptoWrapper/Base64Wrapper.h"


void RunClient()
{
	TransferInfo tInfo;
	MeInfo MInfo;
	KeyInfo KInfo;
	ServerInstance* instance;
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
		MInfo.Name = tInfo.ClientName;
		int BuffSize = 0;
		auto buff = CreateRegisterRequest(&MInfo, &BuffSize);
		if (BuffSize == 0 || buff == nullptr)
		{
			//TODO bad register request
			return;
		}
		bool SendSuccess = instance->SendBufferToServer(buff, BuffSize);
		delete buff;
		ServerResponseMessage* rec_msg = instance->RecieveMessageFromServer();
		if (rec_msg == nullptr || rec_msg->Code != register_success_response || rec_msg->PayloadSize != CLIENT_ID_LENGTH)
		{
			//todo failed to register bye bye
			std::cerr << ("debug: register failed!") << std::endl;
			return;
		}
		std::cerr << ("debug: register success!") << std::endl;
		MInfo.HexStrIdentifier = AsciiToHexStr(rec_msg->payload);//save public identifier
		delete rec_msg;
		//start rs and send rsa pub key
		//generate rsa key-pair 
		RSAPrivateWrapper rsapriv;

		std::string pubkey = rsapriv.getPublicKey();
		std::string privKey = rsapriv.getPrivateKey();
		std::string privKeyb64 = Base64Wrapper::encode(privKey);
		/*MInfo.Privkey = privKeyb64;
		std::string uncodedtst = Base64Wrapper::decode(privKeyb64);*/
		MInfo.Privkey = privKeyb64;
		KInfo.PrivKey = privKeyb64;
		bool SuccessSaved = MInfo.SaveFile();
		SuccessSaved = KInfo.SaveFile();

		if (!SuccessSaved)
		{
			std::cerr << "Error saving My info file" << std::endl;
		}

		buff = CreateSendPubKeyRequest(&MInfo, &BuffSize, pubkey);

		SendSuccess = instance->SendBufferToServer(buff, BuffSize);//send public key to server
		delete buff;
		//receive aes key from server
		rec_msg = instance->RecieveMessageFromServer();

		if (rec_msg == nullptr || rec_msg->Code != pub_rsa_received_sending_encrypted_aes || rec_msg->PayloadSize < MIN_2102_PAYLOAD_SIZE)
		{

			//todo check client id 
			//todo failed to register bye bye
			std::cerr << ("debug: register failed!") << std::endl;
			return;
		}
		if (!compareClientId(MInfo, rec_msg->payload))
		{
			std::cerr << ("debug: register failed! wrong client id") << std::endl;
			return;
		}
		//check client id to be good

		int keylen = rec_msg->PayloadSize - CLIENT_ID_LENGTH;
		char* Enc_AES_Key = new char(keylen);
		std::memcpy(Enc_AES_Key,rec_msg->payload+CLIENT_ID_LENGTH, keylen);
		std::string AESKey = rsapriv.decrypt(Enc_AES_Key,keylen);
		bool DebugPoint = 0;
	}
	else
	{//attempt re-register
	 //auto MeInfoData =parse




	}



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

#define SendPubKeyRequestPayloadSize 415
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