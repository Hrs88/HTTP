#include<iostream>
#include<fstream>
#include<string>
#include<unordered_map>
#include<unistd.h>
#include<strings.h>
#include"toolkit.hpp"
static const size_t default_buff_size = 128;
static const int read_fd = 0;
int main()
{
    std::string method = getenv("METHOD");
    if(method != "POST") return 0;
    char buff[default_buff_size] = {0};
    std::string body_size = getenv("BODY_SIZE");
    std::string body;
    const size_t size = std::stoul(body_size);
    size_t total = 0;
    size_t n = 0;
    while(total < size)
    {
        n = read(read_fd,buff,sizeof(buff));
        body += buff;
        total += n;
        bzero(buff,sizeof(buff));
    }
    std::unordered_map<std::string,std::string> kv;
    deal_post(body,kv);
    std::fstream file("logs/message.txt",std::ios::app);
    file << "name: " << UrlCoder::UrlDecode(kv["name"]) << std::endl;
    file << "contact: " << UrlCoder::UrlDecode(kv["contact"]) << std::endl;
    file << "body: " << UrlCoder::UrlDecode(kv["body"]) << std::endl;
    file << "----------Divider----------" << std::endl;
    return 0;
}