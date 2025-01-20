#ifndef ANY
#define ANY

#include <iostream>

class Any{
public:
    Any(){};
    ~Any(){};
    template<typename T>
    Any(T data){
        this->base_ptr = std::make_unique<Derive<T>>(data);
    }
 
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    Any(Any&& any){
        this->base_ptr = std::move(any.base_ptr);
    };
    Any& operator=(Any&& any){
        this->base_ptr = std::move(any.base_ptr);
        return *this;
    };

public:
    template<typename T>
    T cast(){
        //把基类指针转派生类指针去拿data数据
        Derive<T>* ptr =  dynamic_cast<Derive<T>*>(base_ptr.get());
        if(ptr==nullptr){
            throw "date type dismatch.";
        }
        return ptr->data;
    }

private:
    class Base{
    public:
        Base(){};
        virtual ~Base(){};
    };

    template<typename T> 
    class Derive:public Base{
    public:
        Derive(T data){
            this->data=data;
        }
        T data;
    };

private:
    std::unique_ptr<Base> base_ptr;//基类指针
     
};
#endif
