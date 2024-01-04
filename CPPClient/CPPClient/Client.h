#pragma once
#include "ServerConnection.h"
#include "Utils.h"
#include "CryptoWrapper/RSAWrapper.h"
#include "CryptoWrapper/Base64Wrapper.h"

#define CLIENT_VERSION 3
void RunClient();
//Messages
bool RegisterClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out);
bool ReconnectClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out);

//requests
char* CreateRegisterRequest(MeInfo* MInfo, int* RetSize);
char* CreateReconnectRequest(MeInfo* MInfo, int* RetSize);
char* CreateSendPubKeyRequest(MeInfo* Meinfo, int* RetSize, std::string pubkey);
char* CreateSendFileRequest(MeInfo MInfo, TransferInfo TInfo, std::string AESKey, char* FileData, int FileSize, int* EncFileSize_out, int* RetSize);

//crc responses
char* CreateBadCRCRequestRetry(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo);
char* CreateBadCRCRequestTerminateConnection(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo);
char* CreateCorrectRCRRequestRetry(MeInfo* MInfo, int* RetSize, TransferInfo Tinfo);



