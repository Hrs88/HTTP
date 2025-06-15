#include<iostream>
#include<cstdlib>
#include<fstream>
#include<unistd.h>
const size_t buff_size = 256;
int main()
{
    std::string method = getenv("METHOD");
    std::fstream file("logs/info.txt",std::ios::out);
    if(method == "GET")
    {
        std::string arguments = getenv("ARGUMENTS");
        file << "方法:GET,参数为:" << arguments << std::endl;
    }
    else if(method == "POST")
    {
        std::string body_size = getenv("BODY_SIZE");
        std::string body;
        size_t size = std::stoul(body_size);
        char buff[buff_size] = {0};
        size_t n = 1;
        size_t total = 0;
        while(total < size && n > 0)
        {
            n = read(0,buff,sizeof(buff));
            total += n;
            body += buff;
        }
        file << "方法:POST,内容大小:" << body_size << ",内容为:" << body << std::endl;
    }
    else file << "非法方法" << std::endl;
    file.close();
    return 0;
}
