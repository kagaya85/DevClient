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
    // 初始化随机数种子
    srand((unsigned)time(NULL));
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
    // 转主机序
    head.totlen = ntohs(head.totlen);
    head.datalen = ntohs(head.datalen);

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
        return -1;

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

    return 0;
}

int DevClient::ReadFileToBuf(const std::string &filename, u_char *&databuf, int &buflen)
{
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

/**
 * 获取CPU频率 MHz
 */
uint16_t DevClient::GetCpuFreq()
{
    std::ifstream fin("/proc/cpuinfo");
    int N = 100;
    char line[N];
    uint16_t res = 0;
    
    while (!fin.eof())
    {
        fin.getline(line, N);
        std::vector<std::string> strs = split(line, ":");
        if (trim(strs[0]) == "cpu MHz")
        {
            res = (uint16_t)atoi(trim(strs[1]).c_str());
            break;
        }
    }

    fin.close();
    return res;
}

/**
 * 获取RAM大小，MB
 */
uint16_t DevClient::GetRamSize()
{
    std::ifstream fin("/proc/meminfo");
    int N = 100;
    char line[N];
    uint16_t res = 0;
    
    while (!fin.eof())
    {
        fin.getline(line, N);
        std::vector<std::string> strs = split(line, ":");
        if (trim(strs[0]) == "MemTotal")
        {
            res = (uint16_t)(atoi(trim(strs[1]).c_str()) / 1024); // 转MB
            break;
        }
    }
    fin.close();
    return res;
}

u_char *DevClient::GenAuthStr(int random_num)
{
    u_int svr_time;
    int pos;

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

bool DevClient::CheckAuthStr(u_char *auth_str, u_int random_num, u_int svr_time)
{
    u_char key[32] = "yzmond:id*str&to!tongji@by#Auth";

    int pos = (random_num % 4093);

    for (int i = 0; i < 32; i++)
    {
        if (auth_str[i] != (key[i] ^ secret[pos]))
            return false;
        pos = ++pos % 4096;
    }

    return true;
}

void DevClient::SLog(int totlen, int sendlen, const char *typestr, u_char *data)
{
    std::ostringstream ss;
    ss << "发送客户端状态应答[intf=" << typestr << " len=" << totlen << "(C-0)]";
    console.log(ss.str(), DBG_SPACK);
    ss.clear();
    ss.str("");
    ss << "发送 " << sendlen << "字节";
    console.log(ss.str(), DBG_SPACK);
    ss.clear();
    ss.str("");
    ss << "(发送数据为:)" << std::endl
       << binstr(data, sendlen);
    console.log(ss.str(), DBG_SPACK);
}

void DevClient::RLog(int totlen, const char *typestr)
{
    std::ostringstream ss;
    ss << "收到客户端状态请求[intf=" << typestr << "]";
    console.log(ss.str(), DBG_SPACK);
}

int DevClient::SendVersionRequire(const char *version)
{
    Head head;
    head.origin = CLIENT;
    head.type = CLIENT_VER_REQ;
    head.totlen = htons(12);
    head.ethport = 0x0000;
    head.datalen = htons(4);

    u_char buf[12];
    memcpy(buf, &head, sizeof(head));
    // 最低版本号
    memcpy(buf + sizeof(head), version, 2);
    // 次1版本号
    memcpy(buf + sizeof(head) + 1, "", 1);
    // 次2版本号
    memcpy(buf + sizeof(head) + 2, "", 1);

    int ret = 0;
    ret = write(sock, buf, head.totlen);

    SLog(head.totlen, ret, "最低版本要求", buf);
}

int DevClient::SendAuthAndConf()
{
    int offset;
    Head head;
    head.origin = CLIENT;
    head.type = CLIENT_DEV_INFO;
    head.totlen = htons(29 * 4);
    head.ethport = 0x0000;
    head.datalen = htons(27 * 4);

    u_char buf[head.totlen];
    memcpy(buf, &head, sizeof(head));
    // CPU 主频
    uint16_t int16 = htons(GetCpuFreq());
    memcpy(buf + sizeof(head), &int16, sizeof(int16));
    // RAM 大小
    int16 = htons(GetRamSize());
    memcpy(buf + sizeof(head) + 2, &int16, sizeof(int16));
    // FLASH 大小 + 设备内部序列号
    offset = sizeof(head) + 4;
    for (int i = 0; i < 2; i++, offset += sizeof(int16))
    {
        int16 = htons((uint16_t)rand());
        memcpy(buf + offset, &int16, sizeof(int16));
    }

    // 设备组序列号 + 型号 + 软件版本号
    u_char char128[16];
    for (int i = 0; i < 3; i++, offset += sizeof(char128))
    {
        for (int j = 0; j < 15; j++)
            char128[j] = ((u_char)rand() % ('z' - '0' + 1)) + '0';

        char128[15] = 0;

        memcpy(buf + offset, &char128, sizeof(char128));
    }

    // 各个端口数量 16 bytes
    offset += 16;

    // 认证串
    int random_num = (u_int)rand();
    u_char *auth_string = GenAuthStr(random_num);
    memcpy(buf + offset, auth_string, 32);
    offset += 32;
    delete auth_string;

    memcpy(buf + offset, &random_num, sizeof(random_num));
    offset += sizeof(random_num);

    int ret = 0;

    assert(head.totlen == offset);
    ret = write(sock, buf, head.totlen);

    SLog(head.totlen, ret, "认证信息", buf);
}

int DevClient::SendSysInfo()
{
    return 0;
}

int DevClient::SendConfInfo()
{
    return 0;
}

int DevClient::SendProcInfo()
{
    return 0;
}

int DevClient::SendEthInfo()
{
    return 0;
}

int DevClient::SendUsbInfo()
{
    return 0;
}

int DevClient::SendPrtInfo()
{
    return 0;
}

int DevClient::SendTerInfo()
{
    return 0;
}

int DevClient::SendYaTerInfo()
{
    return 0;
}

int DevClient::SendIpTerInfo()
{
    return 0;
}

int DevClient::SendFileInfo()
{
    return 0;
}

int DevClient::SendQueInfo()
{
    return 0;
}

int DevClient::SendAck()
{
    return 0;
}
