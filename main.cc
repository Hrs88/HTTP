#include "server.hpp"
int main(int argc,char* argv[])
{
    server& svr = server::getinstance();  //获取单例，确保后续多线程可并发获取
    return 0;
}