#pragma once
#include "ServerConnection.h"
#include "Utils.h"

#define CLIENT_VERSION 3
void RunClient();
char* CreateRegisterRequest(MeInfo* MInfo, int* RetSize);