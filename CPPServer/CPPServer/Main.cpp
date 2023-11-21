#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h> // Include this header for inet_pton
#include "Client.h"
#pragma comment(lib, "ws2_32.lib")

int main() {
    RunClient();
    return 0;
}
