#pragma once
#include<sstream>
#include<utility>
#include<string>
#include<vector>
#include<algorithm>
#include<unordered_map>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<unistd.h>
#include"log.hpp"
#include"comm.hpp"
const size_t default_rdbuff_size = 1024;
static const std::string http_sep = ": ";
static const std::string web = "./web";
static const std::string linux_sep = "\r\n";
static const std::string not_found = web + "/errors/404.html";
class request
{
public:
    request(const std::string& sig,const std::string sep) 
    {
        get(sig,sep);
        parse();
    }
    std::pair<std::string,std::vector<char>> response()
    {
        std::string page_path = web + _uri;
        if(_method == "GET")
        {
            if(page_path[page_path.size()-1] == '/') page_path += "index.html";
            return page_get(page_path);
        }
        else if(_method == "POST")
        {
            return CGI_solve(page_path);
        }
        else return page404();      //不处理
    }
private:
    //原始数据
    std::string _rq_head_line;
    std::vector<std::string> _rq_header;
    std::vector<char> _rq_body;
    //处理后的数据
    std::string _method;
    std::string _uri;
    std::string _arguments;
    std::string _version;
    std::unordered_map<std::string,std::string> _header;
    void get(const std::string& sig,const std::string sep)
    {
        std::string tmp;
        size_t n = 0;
        size_t m = sig.find(sep);
        _rq_head_line = sig.substr(n,m-n);
        while(tmp.size())
        {
            m += sep.size();
            n = m;
            m = sig.find(sep,n);
            tmp = sig.substr(n,m-n);
            if(tmp.size()) _rq_header.push_back(tmp);
        }
        m += sep.size();
        while(m < sig.size()) _rq_body.push_back(sig[m++]);
    }
    void parse()
    {
        std::stringstream ss(_rq_head_line);
        ss >> _method >> _uri >> _version;
        size_t pos = 0;
        for(const auto& e : _rq_header)
        {
            pos = e.find(http_sep);
            _header[e.substr(0,pos)] = e.substr(pos+http_sep.size());
        }
        std::transform(_method.begin(),_method.end(),_method.begin(),toupper);          //防止类似"Get"/"Post"的不规范报头出现
        pos = _uri.find("?");                                                           //判断是否带参，若带参则分离，利好CGI
        if(pos != std::string::npos)
        {
            _arguments = _uri.substr(pos+1);
            _uri = _uri.substr(0,pos);
        }
    }
    std::pair<std::string,std::vector<char>> page404()
    {
        std::string rp_head_line = "HTTP/1.0 404 Not Found" + linux_sep;
        std::string rp_header = "Content-Type: text/html; charset=UTF-8" + linux_sep;
        std::vector<char> rp_body;
        int fd = open(not_found.c_str(),O_RDONLY);
        get_rp_body(fd,rp_body);
        close(fd);
        rp_header += "Content-Length: " + std::to_string(rp_body.size()) + linux_sep; 
        return std::make_pair(rp_head_line + rp_header + linux_sep,rp_body);
    }
    std::pair<std::string,std::vector<char>> page_get(std::string& page_path)
    {
        int fd = open(page_path.c_str(),O_RDONLY);
        if(fd < 0) return page404();                //页面不存在
        struct stat file_attribute;
        int n = fstat(fd,&file_attribute);
        if(n == 0) _log(INFO,__FILE__,__LINE__,"scan the file success.");       //获取文件属性成功
        else 
        {
            _log(ERROR,__FILE__,__LINE__,"scan the file fail.");                //获取文件属性失败
            return page404();
        }
        if(S_ISREG(file_attribute.st_mode))                                                     //文件是普通文件
        {
            if(file_attribute.st_mode & S_IXUSR || file_attribute.st_mode & S_IXGRP || file_attribute.st_mode & S_IXOTH)        //文件为可执行文件需要特殊处理
            {
                _log(INFO,__FILE__,__LINE__,"the file is executable.");
                close(fd);
                return CGI_solve(page_path);                                                    //CGI处理数据  结果数据 + 报头构建
            }
            else
            {
                _log(INFO,__FILE__,__LINE__,"the file is a normal file or web page.");
                size_t suffix_pos = page_path.rfind(".");
                if(suffix_pos == std::string::npos)
                {
                    _log(WARNING,__FILE__,__LINE__,"the file has no suffix!");
                    close(fd);
                    return page404();
                }
                else
                {
                    std::string suffix = page_path.substr(suffix_pos);
                    std::string rp_head_line = "HTTP/1.0 200 OK" + linux_sep;
                    std::string rp_header = "Content-Type: " + content_type[suffix] + linux_sep;
                    std::vector<char> rp_body;
                    get_rp_body(fd,rp_body);
                    close(fd);
                    rp_header += "Content-Length: " + std::to_string(rp_body.size()) + linux_sep;
                    return std::make_pair(rp_head_line + rp_header + linux_sep,rp_body);
                }
            }
        }
        else if(S_ISDIR(file_attribute.st_mode))                                                //文件是目录文件
        {
            close(fd);
            _log(WARNING,__FILE__,__LINE__,"the file is a directory.");
            std::string real_path = page_path + "/index.html";
            return page_get(real_path);
        }
        else                                                                                    //文件不是普通文件，也不是目录文件
        {
            _log(WARNING,__FILE__,__LINE__,"the file is not a normal file.");
            close(fd);
            return page404();
        }
    }
    void get_rp_body(int fd,std::vector<char>& rp_body)
    {
        char rdbuff[default_rdbuff_size] = {0};
        int n = read(fd,rdbuff,sizeof(rdbuff));
        while(n)
        {
            for(size_t i = 0;i < n;++i) rp_body.push_back(rdbuff[i]);
            n = read(fd,rdbuff,sizeof(rdbuff));
        }
    }
    std::pair<std::string,std::vector<char>> CGI_solve(std::string& exe_path)
    {
        int ptoc[2]; //父写子读
        int ctop[2]; //父读子写
        if(pipe(ptoc) < 0)
        {
            _log(ERROR,__FILE__,__LINE__,"pipe ptoc create error!");
            return page404();
        }
        if(pipe(ctop) < 0)       
        {
            _log(ERROR,__FILE__,__LINE__,"pipe ctop create error!");
            for(auto e : ptoc) close(e);
            return page404();
        }
        pid_t id = fork();
        if(id < 0) 
        {
            _log(ERROR,__FILE__,__LINE__,"fork error!");
            for(auto e : ptoc) close(e);
            for(auto e : ctop) close(e);
            return page404();
        }
        else if(id == 0) //子进程
        {
            close(ptoc[1]);
            close(ctop[0]);
            int recvfd = ptoc[0];
            int sendfd = ctop[1];
            dup2(recvfd,0);
            dup2(sendfd,1);
            close(recvfd);
            close(sendfd);
            std::string method_env = "METHOD=" + _method;
            putenv(const_cast<char*>(method_env.c_str()));
            if(_method == "GET")
            {
                std::string arguments_env = "ARGUMENTS=" + _arguments;
                putenv(const_cast<char*>(arguments_env.c_str()));
            }
            else
            {
                std::string body_size_env = "BODY_SIZE=" + std::to_string(_rq_body.size());
                putenv(const_cast<char*>(body_size_env.c_str()));
            }
            execl(exe_path.c_str(),exe_path.c_str(),nullptr);
        }
        //父进程
        close(ptoc[0]);
        close(ctop[1]);
        int recvfd = ctop[0];
        int sendfd = ptoc[1];
        if(_method == "POST")
        {
            size_t total = 0;
            size_t size = 0;
            std::string body(_rq_body.begin(),_rq_body.end());
            while(total < body.size())
            {
                size = write(sendfd,body.c_str()+total,body.size()-total);
                total += size;
            }
        }
        waitpid(id,nullptr,0);
        close(recvfd);
        close(sendfd);
        return page404();
    }
};