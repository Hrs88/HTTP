#pragma once
#include<cstring>
#include<cstdlib>
#include<fcntl.h>
#include<unistd.h>
bool set_nonblock(int fd)
{
    int value = fcntl(fd,F_GETFL);
    if(value < 0) return false;
    int n = fcntl(fd,F_SETFL,value|O_NONBLOCK);
    if(n < 0) return false;
    return true;
}