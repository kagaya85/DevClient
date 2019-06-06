#include <iostream>
#include <sstream>

#include "Defines.h"
#include "DevClient.h"
#include "Logger.h"

extern Config g_config;
extern Logger console;


/**
 * DevClient
 */
DevClient::DevClient()
{
    // ��ʼ�� socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { // �������һ��0�����Զ�ѡ��Э��
        std::ostringstream ss;
        ss << "create socket error: " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    // servaddr��ʼ��
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // ����ip��ַ
    if (inet_pton(AF_INET, g_config.serverIp.c_str(), &servaddr.sin_addr) <= 0)
    {
        std::ostringstream ss;
        ss << "inet_pton error for " << g_config.serverIp;
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    //���õĶ˿�Ϊ�������
    servaddr.sin_port = htons(g_config.port);

}

DevClient::~DevClient()
{
    close(sock);
}

int DevClient::Connect()
{
     // ��������
    if(connect(sock, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::ostringstream ss;
        ss << "create socket error: "<< strerror(errno) <<" (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }
}

int DevClient::WaitForMsg(Head &head, u_char* &databuf, int &buflen)
{
    int ret = 0;
    
    buflen = 0;
    ret = read(sock, &head, sizeof(Head));
    
    if (ret)
    {
        databuf = new(std::nothrow) u_char[head.datalen];
        if(databuf == NULL)
        {
            console.log("new databuf error", DBG_ERR);
            return -1;
        }

        // �������ݲ���
        while(true)
        {
            ret = read(sock, databuf + buflen, head.datalen);
            buflen += ret;
            if(buflen == head.datalen)
                break;
        }

    }
    else if(ret < 0)
    {   // error
        std::ostringstream ss;
        ss << "create socket error: "<< strerror(errno) <<" (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        return -1;
    }
    else
    {   // EOF
        console.log("socket close", DBG_ERR);
        return 0;
    }
    
    return -1;
}

int DevClient::MsgHandler(Head head, u_char* databuf, int buflen)
{
    if(databuf == NULL)
        return;

    // ����Դ���Ƿ����������⴦��
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

void DevClient::ReadFileToBuf(const std::string &filename, u_char* &databuf)
{
    char sendBuff[BUF_SIZE];
    int wNum = 0;

    if (write(sock, sendBuff, wNum) <= 0)
    {
        std::ostringstream ss;
        ss << "send message error, filename: " << filename;
        console.log(ss.str(), DBG_ERR);
        exit(0);
    }
}
