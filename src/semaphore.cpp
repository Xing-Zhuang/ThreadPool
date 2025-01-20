#include "semaphore.h"

Semaphore::Semaphore(){
    this->sem_num=0;
}

 
void Semaphore::acquire(){
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock,[&](){return sem_num>0;});
    sem_num--;
}
void Semaphore::release(){
    std::unique_lock<std::mutex> lock(mtx);
    sem_num++;
    cv.notify_all();
}

 