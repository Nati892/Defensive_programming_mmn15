#include "Client.h"
#include "stdio.h"
#include "FileUtils.h"
#include "ServerConnection.h"

#define ME_FILE_PATH "me.info"
#define TRANSFER_FILE_PATH "transfer.info"
void ReadTransferInfo();
FILE* ReadMeData();


void RunClient()
{
	TransferInfo tInfo;
	if (!IfFileExists(TRANSFER_FILE_PATH))
	{
		//error no transfer file. // Log and craete sample file and instructionns in log
		return;//TODO handle no transfer file or inccorect transfer file format
	}
	else
	{
		 tInfo = parseFile(TRANSFER_FILE_PATH);
	}

	ServerInstance* instance = new ServerInstance(tInfo);
	if (!instance->StartConnection())
	{
		//log error in connection
		return;
	}


	FILE* pMeFile;
	if (!IfFileExists("me.info"))
	{//register
		

	}
	else
	{//attempt re-register


	}
}


void ReadTransferInfo()
{




}

