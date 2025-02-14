// EasyTcpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include "EasyTcpServer.hpp"

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
    if(server.Bind(nullptr, 9090) < 0)
    {
        printf("Bind Error!\n");
        return -1;
    }

    if(server.Listen(5) < 0)
    {
        printf("Listen Error!\n");
        return -2;
    }
    
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
