#include "thread_pool.h"
#include "any.h"
#include "semaphore.h"
#include <functional>
#include <thread>
#include <iostream>
#include <mutex>


 
class MyTask:public Task{

public:
    Any run(){
        std::cout<<"run:"<<std::this_thread::get_id()<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::string str="TEST";
        return str;
    }
   
};
int main(){
    
    {
 
        ThreadPool pool(5);
        pool.start(); 
        
        auto task_ptr = std::make_shared<MyTask>(); 
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);
        pool.submit_task(task_ptr);

        
        Result res = pool.submit_task(task_ptr);
        
        std::string data = res.get_data().cast<std::string>();  
        std::cout<<data;
    }
    
    
    return 0;
} 

