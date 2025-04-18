#pragma once
#include<sstream>
#include<string>
#include<vector>
#include<unordered_map>
static const std::string http_sep = ": ";
class request
{
public:
    request(const std::string& sig,const std::string sep) 
    {
        get(sig,sep);
        parse();
    }
    std::string response()
    {
        return "HTTP/1.0 200 OK\r\n\r\n";
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