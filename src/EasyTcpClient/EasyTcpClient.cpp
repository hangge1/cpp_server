// EasyTcpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>

#include "EasyTcpClient.hpp"


bool g_bRun = true;
void CmdFunc()
{
    //if(!client) return;

    char cmdBuf[128] {0};
    while(true)
    {
        scanf("%s", cmdBuf);

        if(!strncmp(cmdBuf, "exit", 128))
        {
            printf("退出cmd线程!\n");
            g_bRun = false;
            break;
        }

        if(!strncmp(cmdBuf, "login", 128))
        {
            //发
            Login login;
            strncpy(login.userName, "zzh", 32);
            strncpy(login.passWord, "123456", 32);
            //client->SendData(&login);
        }
        else if(!strncmp(cmdBuf, "logout", 128))
        {
            //发
            Logout logout;
            strncpy(logout.userName, "zzh", 32);
            //client->SendData(&logout);
        }
        else
        {
            printf("命令不支持!\n");
        }
    }  
}

int main()
{
    //类似UI线程
    std::thread cmdThread(CmdFunc);
    cmdThread.detach();

    const int clientCount = 500;
    EasyTcpClient* client[clientCount] {};
    for(int i = 0; i < clientCount; i++)
    {
        if(!g_bRun)return -1;

        client[i] = new EasyTcpClient();
        if(-1 == client[i]->Connect("127.0.0.1", 9090))
        {
            printf("Client Connect failed!\n");
            return -1;
        }
    }

    Login login;
    strncpy(login.userName, "zzh", 32);
    strncpy(login.passWord, "123456", 32);
    while(g_bRun)
    {
        for(int i = 0; i < clientCount; i++)
        {
            client[i]->SendData(&login);
            //client[i]->OnRun();
        }
    }

    for(int i = 0; i < clientCount; i++)
    {
        client[i]->Close();
        client[i] = nullptr;
    }

    getchar();
    return 0;
}
