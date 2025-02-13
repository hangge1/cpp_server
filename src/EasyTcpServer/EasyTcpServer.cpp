// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
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

bool g_bRun = true;
void CmdFunc(EasyTcpServer* server)
{
    if(!server) return;

    char cmdBuf[128] {0};
    while(server->isRun())
    {
        scanf("%s", cmdBuf);

        if(!strncmp(cmdBuf, "exit", 128))
        {
            printf("退出cmd线程!\n");
            server->Close();
            break;
        }
        else
        {
            printf("命令不支持!\n");
        }
    }  
    
    
}

int main()
{
    EasyTcpServer server;
    server.Bind(nullptr, 9090);
    server.Listen(5);
    
    std::thread cmdThread(CmdFunc, &server);
    cmdThread.detach();

    while(server.isRun())
    {
        server.OnRun();
    }
    
    std::cout << "Server Exit\n";

    server.Close();
    getchar();

    return 0;
}
