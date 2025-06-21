#include<iostream>
#include<fstream>
#include<string>
#include<unordered_map>
#include<unistd.h>
#include<time.h>
#include<strings.h>
#include<mysql/mysql.h>
#include"toolkit.hpp"
//#define DEBUG
static const size_t default_buff_size = 128;
static const int read_fd = 0;
static const std::string default_file = "logs/message.txt";
namespace mysql
{
    static const std::string default_host = "localhost";
    static const std::string default_user = "holly";
    static const std::string default_password = "linktome";
    static const std::string default_database = "httpserver";
    static const unsigned int default_port = 3306;
}
std::string get_time()
{
    char buff[default_buff_size] = {0};
    time_t t = time(nullptr);
    struct tm* timeinfo = localtime(&t);
    strftime(buff,sizeof(buff),"%Y-%m-%d %H:%M:%S",timeinfo);
    return std::string(buff);
}
void to_file(std::unordered_map<std::string,std::string>& kv)
{
    std::string time = get_time();
    std::fstream file(default_file.c_str(),std::ios::app);
    file << "time: " << time << std::endl;
    file << "name: " << UrlCoder::UrlDecode(kv["name"]) << std::endl;
    file << "contact: " << UrlCoder::UrlDecode(kv["contact"]) << std::endl;
    file << "body: " << UrlCoder::UrlDecode(kv["body"]) << std::endl;
    file << "----------Divider----------" << std::endl;
}
void to_mysql(std::unordered_map<std::string,std::string>& kv)
{
    using namespace mysql;
    MYSQL* link = mysql_init(nullptr);
    mysql_real_connect(link,default_host.c_str(),default_user.c_str(),default_password.c_str(),default_database.c_str(),default_port,nullptr,0);
    mysql_set_character_set(link,"utf-8");
    std::string query = "insert message (time,name,contact,msg) values (\"" + get_time() + "\",\"" + 
    UrlCoder::UrlDecode(kv["name"]) + "\",\"" + UrlCoder::UrlDecode(kv["contact"]) + "\",\"" + UrlCoder::UrlDecode(kv["body"]) + "\");";
#ifdef DEBUG
    std::fstream file(default_file.c_str(),std::ios::app);
    file << query << std::endl;
#endif
    mysql_query(link,query.c_str());
    mysql_close(link);
}
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
    //to_file(kv);
    to_mysql(kv);
    return 0;
}