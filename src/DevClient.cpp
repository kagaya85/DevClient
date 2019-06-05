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
    // ��ʼ�� socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { // �������һ��0�����Զ�ѡ��Э��
        ostringstream ss;
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
        ostringstream ss;
        ss << "inet_pton error for " << g_config.serverIp;
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    //���õĶ˿�Ϊ�������
    servaddr.sin_port = htons(g_config.port);

    // ��������
    // if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    //     ostringstream ss;
    //     ss << "create socket error: "<< strerror(errno) <<" (errno: " << errno << ")";
    //     console.log(ss.str(), DBG_ERR);
    //     exit(-1);
    // }

    // ������
    Head head;
    if (read(sock, &head, sizeof(Head)))
    {
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
