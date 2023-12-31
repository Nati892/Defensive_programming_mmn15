#pragma once
#include "ServerConnection.h"
#include "Utils.h"
#include "CryptoWrapper/RSAWrapper.h"
#include "CryptoWrapper/Base64Wrapper.h"

#define CLIENT_VERSION 3
void RunClient();
char* CreateRegisterRequest(MeInfo* MInfo, int* RetSize);
char* CreateSendPubKeyRequest(MeInfo* Meinfo, int* RetSize, std::string pubkey);
char* CreateReconnectRequest(MeInfo* MInfo, int* RetSize);
bool RegisterClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out);
bool ReconnectClient(TransferInfo TInfo, ServerInstance* server, MeInfo* MInfo_out, std::string** AESKey_out, RSAPrivateWrapper** Rsa_Encryptor_out);