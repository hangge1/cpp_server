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

        //if(!strncmp(cmdBuf, "login", 128))
        //{
        //    //发
        //    Login login;
        //    strncpy(login.userName, "zzh", 32);
        //    strncpy(login.passWord, "123456", 32);
        //    //client->SendData(&login);
        //}
        //else if(!strncmp(cmdBuf, "logout", 128))
        //{
        //    //发
        //    Logout logout;
        //    strncpy(logout.userName, "zzh", 32);
        //    //client->SendData(&logout);
        //}
        //else
        //{
        //    printf("命令不支持!\n");
        //}
    }  
}

const char* server_ip = "127.0.0.1";//"192.168.26.129";//
const unsigned short server_port = 9090;
const int clientCount = 1000;
const int threadCount = 4;
EasyTcpClient* g_clients[clientCount] {};

void SendThread(int id)
{
    int perClientCount = clientCount / threadCount;
    int begin = (id-1) * perClientCount;
    int end = id * perClientCount;

    for(int i = begin; i < end; i++)
    {
        if(!g_bRun) return;

        g_clients[i] = new EasyTcpClient();
        if(-1 == g_clients[i]->Connect(server_ip, server_port))
        {
            printf("Client Connect failed!\n");
            return;
        }
    }

    Login login;
    strncpy(login.userName, "zzh", 32);
    strncpy(login.passWord, "123456", 32);
    while(g_bRun)
    {
        for(int i = begin; i < end; i++)
        {
            g_clients[i]->SendData(&login);
            //client[i]->OnRun();
        }
    }

    for(int i = begin; i < end; i++)
    {
        g_clients[i]->Close();
        g_clients[i] = nullptr;
    }
}

int main()
{
    //类似UI线程
    std::thread cmdThread(CmdFunc);
    cmdThread.detach();

    
    std::thread workThread[threadCount];
    for(int i = 0; i < threadCount; i++)
    {
        workThread[i] = std::thread(SendThread, i + 1);
    }

    for(int i = 0; i < threadCount; i++)
    {
        workThread[i].detach();
    }

    while(g_bRun)
    {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1000);
#endif 
    }

    getchar();
    return 0;
}
