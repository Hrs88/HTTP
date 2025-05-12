#include"server.hpp"
#include"log.hpp"
void* start_loop(void* svr_pointer)
{
    httpsvr& svr = *((httpsvr*)svr_pointer);
    svr.loop();
}
int main(int argc,char* argv[])
{
    daemon(1,0);                    //守护进程化
    _log.change_mode(SIG_FILE);     //单文件日志
    _log(INFO,__FILE__,__LINE__,"pid:%d",getpid());
    httpsvr& svr = httpsvr::getinstance();
    pthread_t tid;
    pthread_create(&tid,nullptr,start_loop,&svr);
    pthread_detach(tid);
    for(;;);
    return 0;
}