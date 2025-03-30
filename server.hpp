#include<iostream>
#include<cstdlib>
#include<unistd.h>
#include<strings.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/socket.h>
enum{
    SOCK_ERR = 1,
    BIND_ERR,
    LISTEN_ERR,
};
const uint16_t default_port = 65535;
const int default_backlog = 5;
class server
{
public:
    static server& getinstance()
    {
        if(nullptr == svr)
        {  
            svr = new server();
            svr->init();
        }
        return *svr;
    }
    void init(int backlog = default_backlog)
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
private:
    uint16_t _port;
    int _listen_socket;
    static server* svr;
    server(uint16_t port = default_port):_port(port),_listen_socket(-1){}
    server(server& obj) = delete;
    server& operator=(const server& obj) = delete;
};
server* server::svr = nullptr;