#ifndef SEMAPHORE
#define SEMAPHORE

#include <mutex>
#include <condition_variable>
#include <iostream>
class Semaphore{
public:
    Semaphore();
    ~Semaphore() = default;

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Semaphore&&) = delete;
    Semaphore& operator=(Semaphore&&) = delete;
    

    void acquire();
    void release();

private:
    int sem_num;//信号量个数
    std::mutex mtx;
    std::condition_variable cv;
};



 

#endif