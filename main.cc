#include"server.hpp"
#include"log.hpp"
int main(int argc,char* argv[])
{
    httpsvr& svr = httpsvr::getinstance();
    svr.loop();
    return 0;
}