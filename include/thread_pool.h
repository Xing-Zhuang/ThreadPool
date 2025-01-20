#ifndef THREAD_POOL
#define THREAD_POOL
 
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include "any.h"
#include "semaphore.h"
#include <thread>

 
class Thread{
    public:
        Thread(std::function<void(int)> func);  
        ~Thread();
        void start();

        Thread(const Thread&) = delete;
        Thread(const Thread&&) = delete;
        Thread& operator=(Thread&) = delete;
        Thread& operator=(Thread&&) = delete;

        int get_id();
    private:
        std::function<void(int)> func; //void：返回类型是void  （）：不接受任何参数
        static int generate_id;
        int thread_id;
};


//线程池的submit_task函数返回Result类型 
class Task;
class Result{ 
    public:
        Result(std::shared_ptr<Task> task_ptr,bool is_valid);
        ~Result() = default;

        Result(const Result&) = delete;
        Result& operator=(const Result&) = delete; 
        Result(Result&& res); //Result的移动构造得自己实现  
        Result& operator=(Result&&) = delete; 

        void set_data(Any res);
        Any get_data();
    private:
        Any data;
        bool is_valid;
        std::shared_ptr<Task> task_ptr;
        Semaphore sem;
        const int x=10;
};



class Task{
public:
    Task() = default;
    ~Task() = default;
    
    Task(const Task&) = default;
    Task& operator=(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;

    virtual Any run() = 0;
    void exec();
    void set_result_ptr(Result* result);

    
private:
    Result* result_ptr;
    
};


//线程池模式
enum Mode
{
    FIXED,
    CACHED,
};

class ThreadPool{
public:
    
    ThreadPool(int init_thread_num=std::thread::hardware_concurrency(),int max_task_num=1024,Mode mode=Mode::FIXED,int max_thread_num=1024);
    ~ThreadPool();

    void start();
    void set_mode(Mode mode);
    void set_max_task_num(int max_task_num);
    Result submit_task(std::shared_ptr<Task> task_ptr);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    std::atomic_int init_thread_num; //初始线程的数量
    std::atomic_int cur_thread_num;//当前线程数量
    std::atomic_int idle_thread_num; //空闲线程数量
    std::atomic_int max_thread_num;//CACHED模式下线程数量上限
    std::unordered_map<int,std::unique_ptr<Thread>> threads;//线程队列
    
   
    int max_task_num;//任务数上限
    std::atomic_int cur_task_num; //当前任务队列中的任务数量
    std::queue<std::shared_ptr<Task>> task_que; //任务队列
    
    bool is_running;
    Mode mode; 
    std::mutex task_que_mtx;
    std::condition_variable cv_task_que_not_full;  
    std::condition_variable cv_task_que_not_empty; 
    std::condition_variable cv_exit; 

    void thread_func(int thread_id);//线程池中每个线程都会执行thread_func函数

};




#endif