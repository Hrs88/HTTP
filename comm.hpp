#pragma once
#include<cstring>
#include<string>
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
std::unordered_map<int,std::string> response_code = {
    {100,"Continue"},
    {101,"Switching Protocols"},
    {102,"Processing"},
    {103,"Early Hints"},
    {200,"OK"},
    {201,"Created"},
    {202,"Accepted"},
    {203,"Non-Authoritative Information"},
    {204,"No Content"},{205,"Reset Content"},
    {206,"Partial Content"},
    {207,"Multi-Status"},
    {208,"Already Reported"},
    {226,"IM Used"},
    {300,"Multiple Choices"},
    {301,"Moved Permanently"},
    {302,"Found"},
    {303,"See Other"},
    {304,"Not Modified"},
    {305,"Use Proxy"},
    {306,"Unused"},
    {307,"Temporary Redirect"},
    {308,"Permanent Redirect"},
    {400,"Bad Request"},
    {401,"Unauthorized"},
    {402,"Payment Required"},
    {403,"Forbidden"},
    {404,"Not Found"},
    {405,"Method Not Allowed"},
    {406,"Not Acceptable"},
    {407,"Proxy Authentication Required"},
    {408,"Request Timeout"},
    {409,"Conflict"},
    {410,"Gone"},
    {411,"Length Required"},
    {412,"Precondition Failed"},
    {413,"Payload Too Large"},
    {414,"URI Too Long"},
    {415,"Unsupported Media Type"},
    {416,"Range Not Satisfiable"},
    {417,"Expectation Failed"},
    {418,"I'm a teapot"},
    {421,"Misdirected Request"},
    {422,"Unprocessable Entity"},
    {423,"Locked"},
    {424,"Failed Dependency"},
    {425,"Too Early"},
    {426,"Upgrade Required"},
    {428,"Precondition Required"},
    {429,"Too Many Requests"},
    {431,"Request Header Fields Too Large"},
    {451,"Unavailable For Legal Reasons"},
    {500,"Internal Server Error"},
    {501,"Not Implemented"},
    {502,"Bad Gateway"},
    {503,"Service Unavailable"},
    {504,"Gateway Timeout"},
    {505,"HTTP Version Not Supported"},
    {506,"Variant Also Negotiates"},
    {507,"Insufficient Storage"},
    {508,"Loop Detected"},
    {510,"Not Extended"},
    {511,"Network Authentication Required"}
};