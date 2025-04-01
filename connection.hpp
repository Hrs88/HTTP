#pragma once
#include<iostream>
#include<vector>
#include<functional>
#include<string>
#include<sys/epoll.h>
class connection;
using fun_t = std::function<void(connection&)>;
const size_t default_send = 128;
const size_t default_recv = 128;
const unsigned int default_inevent = (EPOLLIN|EPOLLET);
const unsigned int default_outevent = (EPOLLOUT|EPOLLET);
class connection
{
public:
    fun_t _recv_cb;
    fun_t _send_cb;
    fun_t _except_cb;
    connection(int fd,int epfd,fun_t recv_cb,fun_t send_cb,fun_t except_cb):_fd(fd),__epfd(epfd),_recv_cb(recv_cb),_send_cb(send_cb),_except_cb(except_cb),_client_port(0){}
    ~connection(){}
    int getfd() {return _fd;}
    void set_ip(const std::string& ip) {_client_ip = ip;}
    void set_port(const uint16_t& port) {_client_port = port;}
    const std::string& get_ip() {return _client_ip;}
    const uint16_t& get_port() {return _client_port;}
    void _readtobuff()
    {
        char buffer[default_recv];
        bzero((void*)buffer,sizeof(buffer));
        size_t len = sizeof(buffer);
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
                _except_cb(*this);
                return;
            }
            else
            {
                for(size_t i = 0;i < n;++i)
                    _recvbuffer.push_back(buffer[i]);
                bzero((void*)buffer,sizeof(buffer));
            }
        }
    }
    bool iscomplete()
    {
        return true;
    }
    void handle()
    {
        _sendbuffer = _recvbuffer;
        _recvbuffer.erase(_recvbuffer.begin(),_recvbuffer.end());
    }
    void _sendtoclient()
    {
        char buffer[default_send];
        size_t n = 0;
        size_t buffer_size = sizeof(buffer);
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
                _except_cb(*this);
                return;
            }
            else
            {
                auto it = _sendbuffer.begin();
                for(size_t i = 0;i < m;++i) ++it;
                _sendbuffer.erase(_sendbuffer.begin(),it);
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
        }
        else
        {
            //开启写关心
            epoll_data_t mod_data;
            mod_data.fd = _fd;
            struct epoll_event mod_event = {(default_inevent|default_outevent),mod_data};
            epoll_ctl(__epfd,EPOLL_CTL_MOD,_fd,&mod_event);
        }
    }
private:
    int _fd;
    int __epfd;
    std::vector<char> _sendbuffer;
    std::vector<char> _recvbuffer;
    std::string _client_ip;
    uint16_t _client_port;
};