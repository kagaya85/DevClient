#include <iostream>
#include <sstream>

#include "Defines.h"
#include "DevClient.h"
#include "Logger.h"

extern Config g_config;
extern Logger console;

using namespace std;

/**
 * DevClient
 */
DevClient::DevClient()
{
    // 初始化 socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { // 参数最后一个0代表自动选择协议
        ostringstream ss;
        ss << "create socket error: " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    // servaddr初始化
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // 设置ip地址
    if (inet_pton(AF_INET, g_config.serverIp.c_str(), &servaddr.sin_addr) <= 0)
    {
        ostringstream ss;
        ss << "inet_pton error for " << g_config.serverIp;
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    //设置的端口为输入参数
    servaddr.sin_port = htons(g_config.port);

    // 请求连接
    // if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    //     ostringstream ss;
    //     ss << "create socket error: "<< strerror(errno) <<" (errno: " << errno << ")";
    //     console.log(ss.str(), DBG_ERR);
    //     exit(-1);
    // }

    // 阻塞读
    Head head;
    if (read(sock, &head, sizeof(Head)))
    {
        if(head.origin != SERVER)
        {

        }

        switch(head.type) 
        {

        }
    }
}

DevClient::~DevClient()
{
    close(sock);
}

void DevClient::SendFile(const string &filename)
{
    char sendBuff[BUF_SIZE];
    int wNum = 0;

    if (write(sock, sendBuff, wNum) <= 0)
    {
        ostringstream ss;
        ss << "send message error, filename: " << filename;
        console.log(ss.str(), DBG_ERR);
        exit(0);
    }
}
