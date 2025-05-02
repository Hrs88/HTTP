#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<ctime>
#include<cstdarg>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pthread.h>
const int file_right = 0666;
enum{
    INFO,
    WARNING,
    ERROR,
    FATAL
};
enum mode
{
    STD,
    SIG_FILE,
    MULTI_FILE
};
class __log
{
public:
    __log():_file_fd(-1),_std_fd(1),_mode(STD) {pthread_mutex_init(&_file_lock,nullptr);}
    ~__log() 
    {
        if(_file_fd > 0) close(_file_fd);
        if(_files_fd.size()) for(const auto& e : _files_fd) close(e);
        pthread_mutex_destroy(&_file_lock);
    }
    void lock() {pthread_mutex_lock(&_file_lock);}
    void unlock() {pthread_mutex_unlock(&_file_lock);}
    void change_mode(enum mode c_mode)
    {
        if(c_mode == SIG_FILE)
        {
            if(_file_fd < 0) _file_fd = open("logs/log.txt",O_WRONLY|O_TRUNC|O_CREAT,file_right);
            _mode = c_mode;
        }
        else if(c_mode == MULTI_FILE)
        {
            if(_files_fd.empty())
            {
                _files_fd.push_back(open("logs/info_log.txt",O_WRONLY|O_TRUNC|O_CREAT,file_right));
                _files_fd.push_back(open("logs/warning_log.txt",O_WRONLY|O_TRUNC|O_CREAT,file_right));
                _files_fd.push_back(open("logs/error_log.txt",O_WRONLY|O_TRUNC|O_CREAT,file_right));
                _files_fd.push_back(open("logs/fatal_log.txt",O_WRONLY|O_TRUNC|O_CREAT,file_right));
            }
            _mode = c_mode;
        }
        else if(c_mode == STD) _mode = c_mode;
        else return;
    }
    void operator()(int level,std::string file,int line,const char* format...) 
    {
        int fd = 0;
        if(_mode == STD) fd = _std_fd;
        if(_mode == SIG_FILE) fd = _file_fd;
        if(_mode == MULTI_FILE)
        {
            if(level == INFO) fd = _files_fd[INFO];
            else if(level == WARNING) fd = _files_fd[WARNING];
            else if(level == ERROR) fd = _files_fd[ERROR];
            else if(level == FATAL) fd = _files_fd[FATAL];
        }                                                                            //确认目标文件
        std::string _level;
        if(level == INFO) _level = std::string("[INFO]");
        else if(level == WARNING) _level = std::string("[WARNING]");
        else if(level == ERROR) _level = std::string("[ERROR]");
        else if(level == FATAL) _level = std::string("[FATAL]");
        else return;                                                                 //等级
        time_t now = time(nullptr);
        struct tm _lctime;
        localtime_r(&now,&_lctime);
        char time_str[30];
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",&_lctime);
        std::string _now = "[" + std::string(time_str) + "]";                        //时间
        std::string _file_info = "[" + file + ": " + std::to_string(line) + "] ";    //文件+行数
        va_list s;
        va_start(s,format);
        lock();                                                                      //开始打印日志
        write(fd,_level.c_str(),_level.size());
        write(fd,_now.c_str(),_now.size());
        write(fd,_file_info.c_str(),_file_info.size());
        vdprintf(fd,format,s);
        dprintf(fd,"\n");
        unlock();
        va_end(s);
    }
private:
    int _file_fd;                   //log文件
    int _std_fd;                    //标准输出
    std::vector<int> _files_fd;     //多文件log
    enum mode _mode;                //输出模式
    pthread_mutex_t _file_lock;      //文件也是共享资源
};
__log _log;