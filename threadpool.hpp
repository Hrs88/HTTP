#pragma once
#include<pthread.h>
#include<queue>
#include"connection.hpp"
const size_t default_pthread_num = 5;
const size_t default_cap = 10;
class threadpool final
{
public:
    void lock() {pthread_mutex_lock(&_lock);}               //上锁
    void unlock() {pthread_mutex_unlock(&_lock);}           //解锁
    bool is_full() {return _tasks.size() == _cap;}          //队列是否为满
    bool is_empty() {return _tasks.empty();}                //队列是否为空
    void p_wait() {pthread_cond_wait(&_p_cond,&_lock);}     //进入生产者队列 
    void c_wait() {pthread_cond_wait(&_c_cond,&_lock);}     //进入消费者队列
    void p_wake() {pthread_cond_signal(&_p_cond);}          //唤醒一个生产者
    void c_wake() {pthread_cond_signal(&_c_cond);}          //唤醒一个消费者
    ~threadpool() 
    {
        pthread_mutex_destroy(&_lock);
        pthread_cond_destroy(&_c_cond);
        pthread_cond_destroy(&_p_cond);
        delete _tp;
    }
    static threadpool* getinstance()
    {
        if(_tp == nullptr)
        {
            _tp = new threadpool(default_pthread_num,default_cap);
            _tp->init();
        }
        return _tp;
    }
    void init()
    {
        pthread_mutex_init(&_lock,nullptr);
        pthread_cond_init(&_c_cond,nullptr);
        pthread_cond_init(&_p_cond,nullptr);
        for(size_t i = 0;i < _thread_num;++i)               //创建多个线程
        {
            pthread_t tid;
            pthread_create(&tid,nullptr,_task,nullptr);
            _threads.push_back(tid);
            pthread_detach(tid);
        }
    }
    void push_task(connection* p_con)                       //生产者进行生产
    {
        lock();
        while(is_full()) p_wait();
        _tasks.push(p_con);
        c_wake();
        unlock();
    }
    connection& pop_task()                                  //消费者进行消费
    {
        connection* next_task = nullptr;
        lock();
        while(is_empty()) c_wait();
        next_task = _tasks.front();
        _tasks.pop();
        p_wake();
        unlock();
        return *next_task;
    }
private:
    std::vector<pthread_t> _threads;
    std::queue<connection*> _tasks; 
    pthread_mutex_t _lock;
    pthread_cond_t _c_cond;
    pthread_cond_t _p_cond;
    size_t _thread_num;
    size_t _cap;
    static threadpool* _tp;
    threadpool(size_t thread_num,size_t cap):_thread_num(thread_num),_cap(cap) {}
    threadpool(threadpool&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    static void* _task(void*) 
    {
        while(true)
        {
            connection& con = _tp->pop_task();
            con.handle();
            con._sendtoclient();
        }
    }
};
threadpool* threadpool::_tp = nullptr;