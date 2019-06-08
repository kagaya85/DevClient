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
#include <time.h>

#include "Defines.h"

#define BUF_SIZE 4096
#define SERVER 0x11
#define CLIENT 0x91

// 服务器事件
#define SERVER_AUTH_REQ 0x01

// 客户端事件
#define CLIENT_VER_REQ 0x00
#define CLIENT_DEV_INFO 0x01

// 请求信息事件
#define SYS_INFO 0x02
#define CONF_INFO 0x03
#define PROC_INFO 0x04
#define ETH_INFO 0x05
#define USB_INFO 0x07
#define PRT_INFO 0x08
#define TER_INFO 0x09
#define YATER_INFO 0x0a
#define IPTER_INFO 0x0b
#define FILE_INFO 0x0c
#define QUE_INFO 0x0d

#define ACK 0xff


class DevClient
{
private:
    struct sockaddr_in servaddr;
    int sock;
public:
    DevClient();
    ~DevClient();
    int Connect();
    int WaitForMsg(Head &head, u_char* &databuf, int &buflen);
    int MsgHandler(Head head, u_char* databuf, int buflen);
    int ReadFileToBuf(const std::string &, u_char* &databuf, int &buflen);
    u_char* GenAuthStr();
    bool checkAuthStr(u_char* auth_str, u_int random_num, u_int svr_time);
};


#endif