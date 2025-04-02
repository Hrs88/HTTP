#pragma once
#include<utility>
#include<iostream>
#include<unordered_map>
#include<cstdlib>
#include<signal.h>
#include<unistd.h>
#include<strings.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include"comm.hpp"
#include"connection.hpp"
enum{
    SOCK_ERR = 1,
    BIND_ERR,
    LISTEN_ERR,
    EPOLL_ERR,
    LIS_NONBLOCK_ERR,
};
const uint16_t default_port = 65535;
const int default_backlog = 5;
const int default_timeout = -1;
const int occur_num = 32;
class server
{
public:
    void _init(int backlog = default_backlog)
    {
        _socket();          //创建socket
        _bind();            //绑定端口
        _listen(backlog);   //设置socket为监听状态
    }
    void _socket() 
    {
        _listen_socket = socket(AF_INET,SOCK_STREAM,0);
        if(_listen_socket < 0)
        {
            //log
            exit(SOCK_ERR);
        }
    }
    void _bind() 
    {
        struct sockaddr_in local;
        bzero((void*)&local,sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(_port);
        int n = bind(_listen_socket,(struct sockaddr*)&local,sizeof(local));
        if(n < 0)
        {
            //log
            exit(BIND_ERR);
        }
    }
    void _listen(int backlog)
    {
        int n = listen(_listen_socket,backlog);
        if(n < 0)
        {
            //log
            exit(LISTEN_ERR);
        }
    }
    ~server(){close(_listen_socket);}
protected:
    uint16_t _port;
    int _listen_socket = -1;
    server(uint16_t port):_port(port){}
    server(server& obj) = delete;
    server& operator=(const server& obj) = delete;
};
class httpsvr final : public server
{
public:
    ~httpsvr(){delete _svr;}
    static httpsvr& getinstance(uint16_t port = default_port)
    {
        if(nullptr == _svr)
        {
            _svr = new httpsvr(port);
            _svr->init();
        }
        return *_svr;
    }
    void init() 
    {
        int opt = 1;
        setsockopt(_listen_socket,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt));
        signal(SIGPIPE,SIG_IGN);
        _svr->_init();
        _epfd = epoll_create(128);
        if(_epfd < 0)
        {
            //log
            exit(EPOLL_ERR);
        }
        if(!set_nonblock(_listen_socket))
            exit(LIS_NONBLOCK_ERR);
        epoll_data_t listen_data;
        listen_data.fd = _listen_socket;
        struct epoll_event listen_event = {default_inevent,listen_data};
        epoll_ctl(_epfd,EPOLL_CTL_ADD,_listen_socket,&listen_event);    //监听套接字加入多路转接
        connection* con = new connection(_listen_socket,_epfd,std::bind(&httpsvr::accepter,this,std::placeholders::_1),nullptr,nullptr);
        _connects.insert(std::make_pair(_listen_socket,con));
        bzero((void*)_occur,sizeof(_occur));
    }
    void loop()
    {
        _run = true;
        while(_run)
        {
            // struct sockaddr_in peer;
            // socklen_t len = sizeof(peer);
            // int iofd = accept(_listen_socket,(struct sockaddr*)&peer,&len);
            // if(iofd < 0)
            // {
            //     //log
            //     continue;
            // }
            // std::cout << "get a new link!" << std::endl;
            // sleep(5);
            // close(iofd);
            int n = epoll_wait(_epfd,_occur,sizeof(_occur)/sizeof(_occur[0]),default_timeout);
            for(size_t i = 0;i < n;++i)
            {
                unsigned int events = _occur[i].events;
                int fd = _occur[i].data.fd;
                if(events&EPOLLHUP || events&EPOLLERR)
                {
                    events |= (EPOLLIN|EPOLLOUT);
                }
                if(events&EPOLLIN)
                {
                    if(_connects[fd]->_recv_cb)
                        _connects[fd]->_recv_cb(*_connects[fd]);
                }
                if(events&EPOLLOUT)
                {
                    if(_connects[fd]->_send_cb)
                        _connects[fd]->_send_cb(*_connects[fd]);
                }
            }
        }
    }
    void accepter(connection& con)
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        bzero((void*)&client,len);
        while(true)
        {
            int iofd = accept(_listen_socket,(struct sockaddr*)&client,&len);
            if(iofd < 0)
            {
                if(errno == EWOULDBLOCK)
                {
                    errno = 0;
                    break;
                }
                else if(errno == EINTR)
                {
                    errno = 0;
                    continue;
                }
                else break;
            }
            set_nonblock(iofd);
            uint16_t port = ntohs(client.sin_port);
            char ip[16];
            std::string _ip;
            const char* ret = inet_ntop(AF_INET,(const void*)&client.sin_addr.s_addr,ip,sizeof(ip));
            if(ret)
            {
                _ip += ip;
            }
            else
            {
                //log IP转换失败
            }
            connection* link = new connection(iofd,_epfd,std::bind(&httpsvr::receiver,this,std::placeholders::_1),
                                                std::bind(&httpsvr::sender,this,std::placeholders::_1),
                                                std::bind(&httpsvr::excepter,this,std::placeholders::_1));
            link->set_ip(_ip);
            link->set_port(port);
            _connects.insert(std::make_pair(iofd,link));
            epoll_data_t new_data;
            new_data.fd = iofd;
            struct epoll_event new_event = {default_inevent,new_data};
            epoll_ctl(_epfd,EPOLL_CTL_ADD,iofd,&new_event);
        }
    }
    void sender(connection& con)
    {
        con._sendtoclient();
    }
    void receiver(connection& con)
    {
        con._readtobuff();
        if(con.iscomplete()&&issafe(con.getfd()))   
        {
            con.handle();
            con._sendtoclient();
        }
    }
    void excepter(connection& con)
    {
        int fd = con.getfd();
        epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,nullptr);
        close(fd);
        delete &con;
        _connects.erase(fd);
    }
    bool issafe(int fd)
    {
        auto it = _connects.find(fd);
        if(it != _connects.end()) return true;
        return false;
    }
private:
    static httpsvr* _svr;
    bool _run = false;
    int _epfd = -1;
    std::unordered_map<int,connection*> _connects;
    struct epoll_event _occur[occur_num];
    httpsvr(uint16_t port):server(port){}
    httpsvr(httpsvr& obj) = delete;
    httpsvr& operator=(const httpsvr& obj) = delete;
};
httpsvr* httpsvr::_svr = nullptr;