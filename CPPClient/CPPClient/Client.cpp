#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"


void RunClient()
{
	TransferInfo tInfo;
	MeInfo MInfo;
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

	std::ofstream* pMeFile;
	if (!IfFileExists("me.info"))
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
		if (rec_msg == nullptr || rec_msg->Code != register_success_response )
		{
			//todo failed to register bye bye
		std::cerr << ("debug: register failed!") << std::endl;;
		}
		std::cerr << ("debug: register success!") << std::endl;;
		
		//PrintPChar(buff,BuffSize);//todo delete 
		//char*
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
