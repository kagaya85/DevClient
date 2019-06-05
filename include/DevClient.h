#ifndef DEVCLIENT
#define DEVCLIENT
    
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Defines.h"

#define BUF_SIZE 4096
#define SERVER 0x11
#define CLIENT 0x91

class DevClient
{
private:
    struct sockaddr_in servaddr;
    int sock;
public:
    DevClient();
    ~DevClient();
    void SendFile(const string &);
};


#endif