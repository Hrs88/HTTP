#pragma once
#include<string>
#include<vector>
#include<unordered_map>
class request
{
public:
    request(std::string& sig) {}
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
};