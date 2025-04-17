#pragma once
#include<iostream>
#include<vector>
#include<functional>
#include<string>
#include<unordered_set>
#include<cstdlib>
#include<cstring>
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<strings.h>
#include<pthread.h>
class connection;
using fun_t = std::function<void(connection&)>;
const size_t default_send = 128;
const size_t default_recv = 128;
const unsigned int default_inevent = (EPOLLIN|EPOLLET);
const unsigned int default_outevent = (EPOLLOUT|EPOLLET);
class connection;
std::unordered_set<connection*> safe_code;                  //安全队列，确保执行流不会对已关闭连接进行操作
pthread_mutex_t _safe_lock = PTHREAD_MUTEX_INITIALIZER;
void get_safe_lock() {pthread_mutex_lock(&_safe_lock);}
void put_safe_lock() {pthread_mutex_unlock(&_safe_lock);}
class connection
{
public:
    fun_t _recv_cb;
    fun_t _send_cb;
    fun_t _except_cb;
    connection(int fd,int epfd,fun_t recv_cb,fun_t send_cb,fun_t except_cb):_fd(fd),__epfd(epfd),_recv_cb(recv_cb),_send_cb(send_cb),_except_cb(except_cb),_client_port(0)
    {
        pthread_mutex_init(&_lock,nullptr);
    }
    ~connection()
    {
        pthread_mutex_destroy(&_lock);
    }
    int getfd() {return _fd;}
    void set_ip(const std::string& ip) {_client_ip = ip;}
    void set_port(const uint16_t& port) {_client_port = port;}
    const std::string& get_ip() {return _client_ip;}
    const uint16_t& get_port() {return _client_port;}
    void lock() {pthread_mutex_lock(&_lock);}
    void unlock() {pthread_mutex_unlock(&_lock);}
    void _readtobuff()
    {
        char buffer[default_recv];
        bzero((void*)buffer,sizeof(buffer));
        size_t len = sizeof(buffer);
        lock();
        while(true)
        {
            ssize_t n = recv(_fd,buffer,len,0);
            // ssize_t n = read(_fd,buffer,len);
            if(n <= 0)
            {
                if(errno == EWOULDBLOCK)        //接收缓冲区为空
                {
                    errno = 0;
                    break;
                }     
                if(errno == EINTR)              //被信号打断，继续读
                {
                    errno = 0;
                    continue;
                }
                //log 未知错误&&对端关闭
                get_safe_lock();
                if(safe_code.count(this))
                    _except_cb(*this);
                put_safe_lock();
                return;
            }
            else
            {
                for(size_t i = 0;i < n;++i)
                    _recvbuffer.push_back(buffer[i]);
                bzero((void*)buffer,sizeof(buffer));
            }
        }
        unlock();
    }
    size_t iscomplete()  
    {
        std::string sep;            //确认行分隔符
        size_t i = 0;
        while(i < _recvbuffer.size())
        {
            if(_recvbuffer[i] == '\r' || _recvbuffer[i] == '\n') break;
            i++;
        }
        if(i == _recvbuffer.size()) return 0;
        if(i + 1 < _recvbuffer.size() && _recvbuffer[i] == _recvbuffer[i+1] || i + 1 < _recvbuffer.size() && isalpha(_recvbuffer[i+1])) sep += _recvbuffer[i];
        else if(i + 1 < _recvbuffer.size() && _recvbuffer[i] == '\r' && _recvbuffer[i+1] == '\n') sep = "\r\n";
        else return 0;
        std::string tmp(_recvbuffer.begin(),_recvbuffer.end());
        size_t n = tmp.find("Content-Length: ");
        if(n == std::string::npos) return 0;
        size_t m = tmp.find(sep,n);
        if(m == std::string::npos) return 0;
        n += strlen("Content-Length: ");
        int size = atoi(tmp.substr(n,m-n).c_str());
        while(n != m)
        {
            if(m + sep.size() < tmp.size())
            {
                m += sep.size();
                n = m;
                m = tmp.find(sep,n);
                if(m == std::string::npos) return 0;
            }
            else return 0;
        }
        if(m + sep.size() + size <= _recvbuffer.size()) return m + sep.size() + size;
        else return 0;
    }
    void handle()
    {
        lock();
        _sendbuffer = _recvbuffer;
        _recvbuffer.erase(_recvbuffer.begin(),_recvbuffer.end());
        unlock();
    }
    void _sendtoclient()
    {
        char buffer[default_send];
        size_t n = 0;
        size_t buffer_size = sizeof(buffer);
        lock();
        while(!_sendbuffer.empty())
        {
            bzero((void*)buffer,buffer_size);
            for(;n < _sendbuffer.size()&&n < buffer_size;++n)
            {
                buffer[n] = _sendbuffer[n];
            }
            ssize_t m = send(_fd,buffer,n,0);
            // ssize_t m = write(_fd,buffer,n);
            if(m < 0)
            {
                if(errno == EWOULDBLOCK)            //发送缓冲区满了
                {
                    errno = 0;
                    break;
                }     
                if(errno == EINTR)                  //被信号打断，继续发
                {
                    errno = 0;
                    continue;
                } 
                //log 未知错误
                get_safe_lock();
                if(safe_code.count(this))
                    _except_cb(*this);
                put_safe_lock();
                return;
            }
            else
            {
                auto it = _sendbuffer.begin();
                for(size_t i = 0;i < m;++i) ++it;
                _sendbuffer.erase(_sendbuffer.begin(),it);
                n = 0;
                if(m < n) break;    //发送缓冲区满了
            }
        }
        if(_sendbuffer.empty())
        {
            //关闭写关心
            epoll_data_t mod_data;
            mod_data.fd = _fd;
            struct epoll_event mod_event = {default_inevent,mod_data};
            epoll_ctl(__epfd,EPOLL_CTL_MOD,_fd,&mod_event);
            get_safe_lock();
            if(safe_code.count(this))
                _except_cb(*this);          //单次响应结束，关闭连接
            put_safe_lock();
        }
        else
        {
            //开启写关心
            epoll_data_t mod_data;
            mod_data.fd = _fd;
            struct epoll_event mod_event = {(default_inevent|default_outevent),mod_data};
            epoll_ctl(__epfd,EPOLL_CTL_MOD,_fd,&mod_event);
        }
        unlock();
    }
private:
    int _fd;
    int __epfd;
    std::vector<char> _sendbuffer;
    std::vector<char> _recvbuffer;
    std::string _client_ip;
    uint16_t _client_port;
    pthread_mutex_t _lock;          //保证同时只有单执行流执行该任务
};