#ifndef DEVCLIENT
#define DEVCLIENT
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Defines.h"

#define BUF_SIZE 4096

class DevClient
{
private:
    struct sockaddr_in servaddr;
    char recvBuff[BUF_SIZE];
    char sendBuff[BUF_SIZE];
    int sock;
public:
    DevClient();
    ~DevClient();
};


#endif