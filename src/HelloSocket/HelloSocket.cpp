// HelloSocket.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <WinSock2.h>

int main()
{
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

    /*
    
    */

    WSACleanup();
}

