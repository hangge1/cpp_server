#ifndef _TimeStamp_H_
#define _TimeStamp_H_

#include <chrono>

class TimeStamp
{
public:
    TimeStamp()
    {
        Update();
    }

    ~TimeStamp()
    {
        
    }

    void Update()
    {
        _begin = std::chrono::high_resolution_clock::now();
    }

    double getElapsedSecond()
    {
        return getElapsedMicroSecond() * 0.000001;
    }

    double getElapsedMiliSecond()
    {
        return getElapsedMicroSecond() * 0.001;
    }

    double getElapsedMicroSecond()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - _begin);
        return (double)duration.count();
    }
protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> _begin;
};

#endif