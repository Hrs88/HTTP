#pragma once
#include<cstring>
#include<cstdlib>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<unordered_map>
bool set_nonblock(int fd)
{
    int value = fcntl(fd,F_GETFL);
    if(value < 0) return false;
    int n = fcntl(fd,F_SETFL,value|O_NONBLOCK);
    if(n < 0) return false;
    return true;
}
std::unordered_map<std::string,std::string> content_type = {
    {".html","text/html; charset=UTF-8"},
    {".htm","text/html; charset=UTF-8"},
    {".css","text/css; charset=UTF-8"},
    {".js","text/javascript; charset=UTF-8"},
    {".json","application/json; charset=UTF-8"},
    {".xml","application/xml; charset=UTF-8"},
    {".txt","text/plain; charset=UTF-8"},
    {".csv","text/csv; charset=UTF-8"},
    {".svg","image/svg+xml; charset=UTF-8"},
    {".md","text/markdown; charset=UTF-8"},
    {".jpg","image/jpeg;"},
    {".png","image/png;"},
    {".gif","image/gif;"},
    {".mp3","audio/mpeg;"},
    {".mp4","video/mp4;"},
    {".pdf","application/pdf;"},
    {".zip","application/zip;"}
};