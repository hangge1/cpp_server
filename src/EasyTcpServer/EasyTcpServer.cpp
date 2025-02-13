// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>
#include "EasyTcpServer.hpp"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <cstring>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif


int main()
{
    EasyTcpServer server;
    server.Bind(nullptr, 9090);
    server.Listen(5);
    
    while(server.isRun())
    {
        server.OnRun();
    }
    
    std::cout << "Server Exit\n";

    server.Close();
    getchar();

    return 0;
}
