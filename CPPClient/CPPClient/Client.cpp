#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"



void ReadTransferInfo();
FILE* ReadMeData();


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
		PrintPChar(buff,BuffSize);
		
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
