#ifndef DEVCLIENT
#define DEVCLIENT
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fstream>

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

struct AuthReq
{
    uint16_t main;
    uint8_t sub1;
    uint8_t sub2;
    uint16_t reconn_time;
    uint16_t resend_time;
    bool allow_void;
    char pad[3];
    u_char authstr[32];
    uint32_t random_num;
    uint32_t svr_time;
};

struct SysInfo
{
    int user_time;
    int nice_time;
    int sys_time;
    int idle_time;
    int free_mem;
};

struct EthInfo
{
    bool isExist;
    bool isConfig;
    bool state;
    char pad;
    u_char mac[6];
    unsigned short options;
    u_char ip_addr[4];
    u_char mask[4];
    u_char ip_addr_p1[4];
    u_char mask_p1[4];
    u_char ip_addr_p2[4];
    u_char mask_p2[4];
    u_char ip_addr_p3[4];
    u_char mask_p3[4];
    u_char ip_addr_p4[4];
    u_char mask_p4[4];
    u_char ip_addr_p5[4];
    u_char mask_p5[4];
    // 数据统计
    uint32_t rbytes;
    uint32_t rpackets;
    uint32_t rerrs;
    uint32_t rdrop;
    uint32_t rfifo;
    uint32_t rframe;
    uint32_t rcompressed;
    uint32_t rmulticast;
    uint32_t tbytes;
    uint32_t tpackets;
    uint32_t terrs;
    uint32_t tdrop;
    uint32_t tfifo;
    uint32_t tframe;
    uint32_t tcompressed;
    uint32_t tmulticast;

};

struct ScreenInfo
{
    uint8_t no;
    char pad;
    uint16_t port;
    u_char ip_addr[4];
    u_char proto[12];
    u_char state[8];
    u_char info[24];
    u_char type[12];
    uint32_t con_time;
    uint32_t stbytes;
    uint32_t rtbytes;
    uint32_t ssbytes;
    uint32_t rsbytes;
    uint32_t ping_min;
    uint32_t ping_avg;
    uint32_t ping_max;
};

class DevClient
{
private:
    struct sockaddr_in servaddr;
    int sock;
    uint32_t devid;
    uint16_t ttynum;
    std::vector<uint8_t> scrnum_list;
    uint32_t reconn_time;
    uint32_t resend_time;
private:
    int ReadFileToBuf(const std::string &, u_char* &databuf, int &buflen);
    u_char* GenAuthStr(int random_num);
    bool CheckAuthStr(u_char* auth_str, u_int random_num);
    void SLog(int totlen, int sendlen, const char* typestr, u_char* data);
    void RLog(int totlen, const char* typestr);
    uint16_t GetCpuFreq();
    uint16_t GetRamSize();

    // Data send function
    int SendVersionRequire();
    int SendAuthAndConf();
    int SendSysInfo();
    int SendConfInfo();
    int SendProcInfo();
    int SendEthInfo(uint16_t eth_port);
    int SendUsbInfo();
    int SendPrtInfo();
    int SendTerInfo();
    int SendYaTerInfo(uint16_t no);
    int SendIpTerInfo(uint16_t no);
    int SendFileInfo();
    int SendQueInfo();
    int SendAck();
public:
    DevClient(uint32_t id);
    ~DevClient();
    int Connect();
    int WaitForMsg(Head &head, u_char* &databuf, int &buflen);
    Status MsgHandler(Head head, u_char* databuf, int buflen);
};


#endif