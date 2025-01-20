#include "thread_pool.h"
#include "any.h"
#include "semaphore.h"
#include <functional>
#include <thread>
#include <iostream>
#include <mutex>

const int IDLE_TIME = 60;

/******************Task******************/
void Task::exec(){
   result_ptr->set_data(run());
}
void Task::set_result_ptr(Result* result_ptr){
    this->result_ptr = result_ptr;
}

/******************Result******************/
Result::Result(std::shared_ptr<Task> task_ptr,bool is_valid){
    this->task_ptr = task_ptr;
    this->is_valid = is_valid;
    
    task_ptr->set_result_ptr(this); //把Result对象的指针给task！！！这样当task执行完后就可以把结果写入result了
}

//对于Result的移动构造，sem
Result::Result(Result&& res){
    this->task_ptr = res.task_ptr; 
    this->is_valid = res.is_valid;
    this->data = std::move(res.data); 

    this->task_ptr->set_result_ptr(this);
}

void Result::set_data(Any data){
    this->data = std::move(data); //转右值，触发移动构造
    sem.release(); 
}

Any Result::get_data(){
    if(!is_valid){
        return "";
    }
    sem.acquire();
    return std::move(data);//转右值，函数返回触发移动构造（res是Any类型的，没有拷贝构造）
}
 

/******************Thread******************/
int Thread::generate_id=0;

Thread::Thread(std::function<void(int)> func){
    this->func=func;
    this->thread_id = generate_id;
    generate_id++;
    //std::cout<<this->thread_id<<std::endl;
}

Thread::~Thread(){

}

int Thread::get_id(){
    return thread_id;
}

void Thread::start(){
    std::thread t(func,thread_id);
    t.detach();//注意这行代码！！！
   // std::cout<<"detach"<<std::endl;
}


/******************TheadPool******************/
ThreadPool::ThreadPool(int init_thread_num,int max_task_num,Mode mode,int max_thread_num){
    this->init_thread_num = init_thread_num;
    this->cur_task_num = 0;
    this->max_task_num = max_task_num;
    this->mode = mode;
    this->idle_thread_num = init_thread_num;
    this->cur_thread_num = init_thread_num;
    this->max_thread_num = max_thread_num;
    this->is_running = false; //this->is_running在start函数中置为true
}

ThreadPool::~ThreadPool(){
    is_running = false;

    std::unique_lock<std::mutex> lock(task_que_mtx);
    cv_task_que_not_empty.notify_all();//此时线程池中的线程可能由于任务队列为空而阻塞住了
    while(threads.size()>0){
        cv_exit.wait(lock);
    } 

} 

void ThreadPool::set_mode(Mode mode){
    this->mode = mode;
}

void ThreadPool::set_max_task_num(int max_task_num){
    this->max_task_num = max_task_num;
}

//用户主动调用该函数 （生产者）
Result ThreadPool::submit_task(std::shared_ptr<Task> task_ptr){ 
   
    //拿锁
    std::unique_lock<std::mutex> lock(task_que_mtx);

    //若task que满则释放锁并返回false  否则继续执行
    bool flag = cv_task_que_not_full.wait_for(lock,std::chrono::seconds(1),[&]()->bool{return task_que.size()<max_task_num;});
    if(!flag){
        std::cout<<"TaskQue is full."<<std::endl;
        return Result(task_ptr,false); 
    }
    //放任务
    task_que.push(task_ptr);
    cur_task_num++;
  
    //通知消费者
    cv_task_que_not_empty.notify_all();


    //cached模式需要动态增加线程
    if(mode == CACHED && cur_task_num>idle_thread_num && cur_thread_num<max_thread_num){
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::thread_func,this,std::placeholders::_1));
        int thread_id = ptr->get_id();
        threads.emplace(thread_id,std::move(ptr));
        threads[thread_id]->start();
 

        cur_thread_num++;
        idle_thread_num++;
    }
  
    return Result(task_ptr,true);
}

//线程池中的所有线程都会执行这个函数  （消费者）
void ThreadPool::thread_func(int thread_id){

    auto last_time = std::chrono::high_resolution_clock().now();

    while(1){
        
        //拿锁
        std::unique_lock<std::mutex> lock(task_que_mtx);
    

        //判断队列是否为空
        while (task_que.size() == 0)
        {   
            
            if(!is_running){
                std::cout<<"线程退出:"<<std::this_thread::get_id()<<std::endl;
                threads.erase(thread_id);
                cv_exit.notify_all();
                return;
            }


            if(mode == CACHED){
                auto status = cv_task_que_not_full.wait_for(lock,std::chrono::seconds(1));
                if(status == std::cv_status::timeout ){
                    auto now = std::chrono::high_resolution_clock().now();
                    auto dur = std::chrono::duration_cast<std::chrono::seconds>(now-last_time);
                    if(dur.count()>=IDLE_TIME && cur_thread_num>init_thread_num ){
                        threads.erase(thread_id);
                        cur_thread_num--;
                        idle_thread_num--;

                            
                        return;
                    }
                }

            }
            else{
                cv_task_que_not_empty.wait(lock);
            }

            
            
        }
            
 
        idle_thread_num--;
        //取一个任务
        auto task = task_que.front();
        task_que.pop();
        cur_task_num--;
        
        //通知生产者
        cv_task_que_not_full.notify_one();

        //此外，对于多消费者，需要额外通知其他消费者
        if(task_que.size()>0)
            cv_task_que_not_empty.notify_all();

        //最后释放锁执行任务
        lock.unlock();
        
        task->exec();
        idle_thread_num++;

        last_time = std::chrono::high_resolution_clock().now();
    }   


 

}

void ThreadPool::start(){
    this->is_running = true;

    for(int i=0;i<init_thread_num;i++){ 
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::thread_func,this,std::placeholders::_1));
        int thread_id = ptr->get_id();
        threads.emplace(thread_id,std::move(ptr));
    }
    for(int i=0;i<init_thread_num;i++){
        threads[i]->start();
    }
}





