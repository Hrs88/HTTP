#include "server.hpp"
int main(int argc,char* argv[])
{
    httpsvr& svr = httpsvr::getinstance();
    svr.loop();
    return 0;
}