#include "Network.h"
#include <iostream>

bool PNet::Network::Initialize()
{
    WSADATA wsaData;
    int result = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

    if (result != 0)
    {
        std::cerr << "Fail to start up Winsock API" << std::endl;
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        std::cerr << "Fail to find proper versions of Winsock API DLL" << std::endl;
        WSACleanup();
        return false;
    }

    return true;
}

void PNet::Network::Shutdown()
{
    WSACleanup();
}
