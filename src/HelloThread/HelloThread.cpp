// HelloThread.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

std::mutex g_mtx;
int g_count = 0;
std::atomic_int g_count2 = 0;

void workFunc(int i)
{
    g_count2++;

    std::lock_guard<std::mutex> lock(g_mtx);
    std::cout << "Hello, WorkThread " << i << std::endl;
    g_count++;   
}

int main()
{
    std::thread t[4];
    for(int i = 0; i < 4; i++)
    {
        t[i] = std::thread(workFunc, i);
    }

    for(int i = 0; i < 4; i++)
    {
        t[i].join();
    }

    std::cout << "count = " << g_count << std::endl;
    std::cout << "count2 = " << g_count2 << std::endl;
    std::cout << "Hello, MainThread\n";

    getchar();
    return 0;
}
