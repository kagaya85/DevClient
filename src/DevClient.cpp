#include <iostream>
#include <sstream>

#include "Defines.h"
#include "DevClient.h"
#include "Logger.h"

#include "encrpty.cpp"

extern Config g_config;
extern Logger console;

/**
 * DevClient
 */
DevClient::DevClient()
{
    // 初始化 socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { // 参数最后一个0代表自动选择协议
        std::ostringstream ss;
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
        std::ostringstream ss;
        ss << "inet_pton error for " << g_config.serverIp;
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    //设置的端口为输入参数
    servaddr.sin_port = htons(g_config.port);
}

DevClient::~DevClient()
{
    close(sock);
}

int DevClient::Connect()
{
    // 请求连接
    if (connect(sock, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::ostringstream ss;
        ss << "create socket error: " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }
}

int DevClient::WaitForMsg(Head &head, u_char *&databuf, int &buflen)
{
    int ret = 0;

    buflen = 0;
    ret = read(sock, &head, sizeof(Head));

    if (ret)
    {
        databuf = new (std::nothrow) u_char[head.datalen];
        if (databuf == NULL)
        {
            console.log("new databuf error", DBG_ERR);
            return -1;
        }

        // 读入数据部分
        while (true)
        {
            ret = read(sock, databuf + buflen, head.datalen);
            buflen += ret;
            if (buflen == head.datalen)
                break;
        }
    }
    else if (ret < 0)
    { // error
        std::ostringstream ss;
        ss << "create socket error: " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        return -1;
    }
    else
    { // EOF
        console.log("socket close", DBG_ERR);
        return 0;
    }

    return -1;
}

int DevClient::MsgHandler(Head head, u_char *databuf, int buflen)
{
    if (databuf == NULL)
        return;

    // 若来源不是服务器，特殊处理
    if (head.origin != SERVER)
    {
    }

    switch (head.type)
    {
    case SERVER_AUTH_REQ:
        break;
    case SYS_INFO:
        break;
    case CONF_INFO:
        break;
    case PROC_INFO:
        break;
    case ETH_INFO:
        break;
    case USB_INFO:
        break;
    case PRT_INFO:
        break;
    case TER_INFO:
        break;
    case YATER_INFO:
        break;
    case IPTER_INFO:
        break;
    case FILE_INFO:
        break;
    case QUE_INFO:
        break;
    case ACK:
        break;
    }
}

int DevClient::ReadFileToBuf(const std::string &filename, u_char *&databuf, int &buflen)
{
    char sendBuff[BUF_SIZE];

    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        std::ostringstream ss;
        ss << "open file error [filename: " << filename << " ]";
        console.log(ss.str(), DBG_ERR);
        exit(0);
    }

    // 获取文件大小
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp) + 1;

    if (databuf)
        delete databuf;

    databuf = new (std::nothrow) u_char[filesize];
    if (databuf == NULL)
    {
        console.log("new databuf error", DBG_ERR);
        return -1;
    }

    // 移动到文件头
    rewind(fp);

    int ret = fread(databuf, 1, filesize, fp);
    if (ret != filesize)
    {
        console.log("file read do not complete", DBG_ERR);
    }
}

u_char *DevClient::GenAuthStr()
{
    u_int random_num, svr_time;
    int pos;

    random_num = (u_int)rand();
    svr_time = (u_int)time(0);
    svr_time = svr_time ^ (u_int)0xFFFFFFFF;
    pos = (random_num % 4093);

    u_char key[32] = "yzmond:id*str&to!tongji@by#Auth";
    u_char *auth_str = new (std::nothrow) u_char[32];
    if (auth_str == NULL)
    {
        console.log("new auth_str error.", DBG_ERR);
        exit(-1);
    }

    for (int i = 0; i < 32; i++)
    {
        auth_str[i] = key[i] ^ secret[pos];
        pos = ++pos % 4096;
    }

    return auth_str;
}

bool DevClient::checkAuthStr(u_char *auth_str, u_int random_num, u_int svr_time)
{
    u_char key[32] = "yzmond:id*str&to!tongji@by#Auth";

    int pos = (random_num % 4093);

    for (int i = 0; i < 32; i++)
    {
        if(auth_str[i] != key[i] ^ secret[pos])
            return false;
        pos = ++pos % 4096;
    }

    return true;
}
