#include"server.hpp"
#include"log.hpp"
int main(int argc,char* argv[])
{
    daemon(1,0);                    //守护进程化
    _log.change_mode(SIG_FILE);     //单文件日志
    _log(INFO,__FILE__,__LINE__,"pid:%d",getpid());
    httpsvr& svr = httpsvr::getinstance();
    svr.loop();
    return 0;
}