#pragma once
#include<sstream>
#include<utility>
#include<string>
#include<vector>
#include<unordered_map>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
const size_t default_rdbuff_size = 1024;
static const std::string http_sep = ": ";
static const std::string web = "./web";
static const std::string linux_sep = "\r\n";
static const std::string not_found = web + "/404.html";
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
        std::string __header;
        std::vector<char> __body;
        std::string path;
        if(_uri == "/") path = web + _uri + "index.html";
        else path = web + _uri;
        std::string rp_head_line;
        std::string rp_header = "Content-Type: text/html; charset=UTF-8" + linux_sep;
        int fd = open(path.c_str(),O_RDONLY);
        if(fd < 0)
        {
            fd = open(not_found.c_str(),O_RDONLY);
            rp_head_line = "HTTP/1.0 404 Not Found" + linux_sep;
        }
        else rp_head_line = "HTTP/1.0 200 OK" + linux_sep;
        char rdbuff[default_rdbuff_size] = {0};
        ssize_t n = read(fd,rdbuff,sizeof(rdbuff));
        while(n)
        {
            for(size_t i = 0;i < n;++i) __body.push_back(rdbuff[i]);
            n = read(fd,rdbuff,sizeof(rdbuff));
        }
        rp_header += "Content-Length: " + std::to_string(__body.size()) + linux_sep;
        __header += rp_head_line + rp_header + linux_sep;
        close(fd);
        return std::make_pair(__header,__body);
    }
private:
    //原始数据
    std::string _rq_head_line;
    std::vector<std::string> _rq_header;
    std::vector<char> _rq_body;
    //处理后的数据
    std::string _method;
    std::string _uri;
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
    }
};