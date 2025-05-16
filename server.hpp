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
#include"log.hpp"
#include"comm.hpp"
#include"connection.hpp"
#include"threadpool.hpp"
enum{
    SOCK_ERR = 1,
    BIND_ERR,
    LISTEN_ERR,
    EPOLL_ERR,
    LIS_NONBLOCK_ERR,
};
const uint16_t default_port = 80;
const int default_backlog = 5;
const int default_timeout = -1;
const int occur_num = 32;
const int default_fixed_time = 3600;
void* timed_disconnection(void*);
class server
{
public:
    void _init(int backlog = default_backlog)
    {
        _socket();          //创建socket
        _reuse_port();      //设置端口复用
        _bind();            //绑定端口
        _listen(backlog);   //设置socket为监听状态
    }
    void _reuse_port()
    {
        int opt = 1;
        int ret = setsockopt(_listen_socket,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt));
        if(ret == 0) _log(INFO,__FILE__,__LINE__,"set port reuse successfully.");
        else _log(WARNING,__FILE__,__LINE__,"set port reuse error.");
    }
    void _socket() 
    {
        _listen_socket = socket(AF_INET,SOCK_STREAM,0);
        if(_listen_socket < 0)
        {
            _log(FATAL,__FILE__,__LINE__,"socket error.");
            exit(SOCK_ERR);
        }
        _log(INFO,__FILE__,__LINE__,"socket success.");
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
            _log(FATAL,__FILE__,__LINE__,"bind error.");
            exit(BIND_ERR);
        }
        _log(INFO,__FILE__,__LINE__,"bind success.");
    }
    void _listen(int backlog)
    {
        int n = listen(_listen_socket,backlog);
        if(n < 0)
        {
            _log(FATAL,__FILE__,__LINE__,"listen error.");
            exit(LISTEN_ERR);
        }
        _log(INFO,__FILE__,__LINE__,"listen success.");
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
    ~httpsvr(){}
    int get_epoll_fd() {return _epfd;}
    static httpsvr& getinstance(uint16_t port = default_port)
    {
        if(nullptr == _svr)
        {
            _svr = new httpsvr(port);
            _svr->init();
            _log(INFO,__FILE__,__LINE__,"httpsvr is created.");
        }
        return *_svr;
    }
    void init() 
    {
        signal(SIGPIPE,SIG_IGN);
        _svr->_init();
        _epfd = epoll_create(128);
        if(_epfd < 0)
        {
            _log(FATAL,__FILE__,__LINE__,"epoll create error.");
            exit(EPOLL_ERR);
        }
        _log(INFO,__FILE__,__LINE__,"epoll create success.");
        if(!set_nonblock(_listen_socket))
        {
            _log(ERROR,__FILE__,__LINE__,"set listen_socket nonblock error.");
            exit(LIS_NONBLOCK_ERR);
        }
        _log(INFO,__FILE__,__LINE__,"set listen_socket:%d nonblock success.",_listen_socket);
        epoll_data_t listen_data;
        listen_data.fd = _listen_socket;
        struct epoll_event listen_event = {default_inevent,listen_data};
        epoll_ctl(_epfd,EPOLL_CTL_ADD,_listen_socket,&listen_event);    //监听套接字加入多路转接
        connection* con = new connection(_listen_socket,_epfd,std::bind(&httpsvr::accepter,this,std::placeholders::_1),nullptr,nullptr);
        _connects.insert(std::make_pair(_listen_socket,con));
        bzero((void*)_occur,sizeof(_occur));
        _ptp = threadpool::getinstance();
        bool ret = create_timed_disconnection();
        if(ret) _log(INFO,__FILE__,__LINE__,"timed disconnection thread is created.");
        else _log(ERROR,__FILE__,__LINE__,"timed disconnection has a error.");
    }
    void loop()
    {
        _run = true;
        _log(INFO,__FILE__,__LINE__,"server start loop.");
        while(_run)
        {
            size_t pre = 0;
            size_t cur = 0;
            int n = epoll_wait(_epfd,_occur,sizeof(_occur)/sizeof(_occur[0]),default_timeout);
            get_safe_lock();
            for(cur = 0;cur < n;++cur)
            {
                int fd = _occur[cur].data.fd;
                if(safe_code.count(_connects[fd]) || fd == _listen_socket)
                {
                    _connects[fd]->update_last_hit(time(nullptr));
                    _occur[pre++] = _occur[cur];
                }
            }
            put_safe_lock();
            for(size_t i = 0;i < pre;++i)
            {
                unsigned int events = _occur[i].events;
                int fd = _occur[i].data.fd;
                if(fd != _listen_socket) epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,nullptr);
                if(events&EPOLLHUP || events&EPOLLERR)
                {
                    _log(INFO,__FILE__,__LINE__,"%d fd has a error event.",fd);
                    get_safe_lock();
                    if(safe_code.count(_connects[fd]))
                        _connects[fd]->_except_cb(*_connects[fd]);
                    put_safe_lock();
                    continue;
                }
                if(events&EPOLLIN)
                {
                    _log(INFO,__FILE__,__LINE__,"%d fd has a read event.",fd);
                    if(_connects[fd]->_recv_cb)
                        _connects[fd]->_recv_cb(*_connects[fd]);
                }
                else if(events&EPOLLOUT)
                {
                    _log(INFO,__FILE__,__LINE__,"%d fd has a write event.",fd);
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
                _log(INFO,__FILE__,__LINE__,"IP transform success.");
            }
            else
            {
                _log(WARNING,__FILE__,__LINE__,"IP transform error.");
            }
            connection* link = new connection(iofd,_epfd,std::bind(&httpsvr::receiver,this,std::placeholders::_1),
                                                std::bind(&httpsvr::sender,this,std::placeholders::_1),
                                                std::bind(&httpsvr::excepter,this,std::placeholders::_1));
            link->set_ip(_ip);
            link->set_port(port);
            _connects.insert(std::make_pair(iofd,link));
            get_safe_lock();
            safe_code.insert(link);
            put_safe_lock();
            epoll_data_t new_data;
            new_data.fd = iofd;
            struct epoll_event new_event = {default_inevent,new_data};
            epoll_ctl(_epfd,EPOLL_CTL_ADD,iofd,&new_event);
            _log(INFO,__FILE__,__LINE__,"get a new link: %d",iofd);
        }
    }
    void sender(connection& con)
    {
        _ptp->push_task(&con);
    }
    void receiver(connection& con)
    {
        con._readtobuff();
        if(issafe(con.getfd()))   
        {
            _ptp->push_task(&con);
        }
    }
    void excepter(connection& con)          //进入该函数前就已得到安全锁，无需再加锁
    {
        safe_code.erase(&con);
        int fd = con.getfd();
        _connects.erase(fd);
        close(fd);
        delete &con;
        _log(INFO,__FILE__,__LINE__,"delete a link: %d",fd);
    }
    bool issafe(int fd)
    {
        auto it = _connects.find(fd);
        if(it != _connects.end()) return true;
        return false;
    }
    bool create_timed_disconnection()
    {
        pthread_t tid;
        int ret = pthread_create(&tid,nullptr,timed_disconnection,nullptr);
        if(ret == 0)
        {
            pthread_detach(tid);
            return true;
        }
        else return false;
    }
private:
    static httpsvr* _svr;
    threadpool* _ptp;
    bool _run = false;
    int _epfd = -1;
    std::unordered_map<int,connection*> _connects;
    struct epoll_event _occur[occur_num];
    httpsvr(uint16_t port):server(port),_ptp(nullptr){}
    httpsvr(httpsvr& obj) = delete;
    httpsvr& operator=(const httpsvr& obj) = delete;
};
httpsvr* httpsvr::_svr = nullptr;
void* timed_disconnection(void*)
{
    httpsvr& hsvr = httpsvr::getinstance();
    while(true)
    {
        sleep(default_fixed_time);
        _log(INFO,__FILE__,__LINE__,"%d s timed disconnection.",default_fixed_time);
        get_safe_lock();
        std::vector<connection*> _safe_code(safe_code.begin(),safe_code.end());
        for(auto e : _safe_code)
        {
            if(time(nullptr) - e->get_last_hit() > default_fixed_time)
            {
                int fd = e->getfd();
                epoll_ctl(hsvr.get_epoll_fd(),EPOLL_CTL_DEL,fd,nullptr);
                hsvr.excepter(*e);
                _log(INFO,__FILE__,__LINE__,"%d fd timeout.",fd);
            }
        }
        put_safe_lock();
    }
    return nullptr;
}