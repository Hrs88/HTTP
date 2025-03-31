#pragma once
#include<iostream>
#include<vector>
#include<functional>
#include<string>
class connection;
using fun_t = std::function<void(connection&)>;
class connection
{
public:
    fun_t _recv_cb;
    fun_t _send_cb;
    fun_t _except_cb;
    connection(int fd,fun_t recv_cb,fun_t send_cb,fun_t except_cb):_fd(fd),_recv_cb(recv_cb),_send_cb(send_cb),_except_cb(except_cb),_client_port(0){}
    ~connection(){}
    int getfd() {return _fd;}
    void set_ip(const std::string& ip) {_client_ip = ip;}
    void set_port(const uint16_t& port) {_client_port = port;}
    const std::string& get_ip() {return _client_ip;}
    const uint16_t& get_port() {return _client_port;}
private:
    int _fd;
    std::vector<char> _sendbuffer;
    std::vector<char> _recvbuffer;
    std::string _client_ip;
    uint16_t _client_port;
};