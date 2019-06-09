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
DevClient::DevClient(uint32_t id)
{
    devid = id;
    ttynum = (rand() % (g_config.max_ttynum - g_config.min_ttynum)) + g_config.min_ttynum;
    // 初始化每个tty终端数量
    for (int i = 0; i < ttynum; i++)
    {
        uint8_t screen_num = (rand() % (g_config.max_scrnum - g_config.min_scrnum)) + g_config.min_scrnum;
        scrnum_list.push_back(screen_num);
    }

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
    std::ostringstream ss;
    if (connect(sock, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        ss << "Connect " << g_config.serverIp << ':' << g_config.port << " error, " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(-1);
    }

    ss << "Connected(" << g_config.serverIp << ':' << g_config.port << ") OK.";
    console.log(ss.str(), DBG_ENV);
    return 1;
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

Status DevClient::MsgHandler(Head head, u_char *databuf, int buflen)
{
    if (databuf == NULL)
        return Close;

    // 若来源不是服务器，特殊处理
    if (head.origin != SERVER)
    {
        console.log("package origin error", DBG_ERR);
        return Close;
    }

    switch (head.type)
    {
    case SERVER_AUTH_REQ:
    {
        AuthReq data;
        memcpy(&data, databuf, buflen);
        if(ntohs(data.main) < 2)
        {
            console.log("服务器版本过低", DBG_RPACK);
            SendVersionRequire();
        }
        else
        {
            g_config.reconn_time = ntohs(data.reconn_time);
            g_config.resend_time = ntohs(data.resend_time);
            
            // 验证服务器时间
            if (ntohl(data.svr_time) < 1483200000)
            {
                console.log("数字证书过期", DBG_RPACK);
                return Close;
            }
            // 验证加密串
            if(CheckAuthStr(data.authstr, ntohl(data.random_num)))
            {
                console.log("认证非法", DBG_RPACK);
                return Close;
            }

            SendAuthAndConf();
        }
        break;
    }
    case SYS_INFO:
        SendSysInfo();
        break;
    case CONF_INFO:
        SendConfInfo();
        break;
    case PROC_INFO:
        SendProcInfo();
        break;
    case ETH_INFO:
        SendEthInfo(ntohs(head.ethport));
        break;
    case USB_INFO:
        SendUsbInfo();
        break;
    case PRT_INFO:
        SendPrtInfo();
        break;
    case TER_INFO:
        SendTerInfo();
        break;
    case YATER_INFO:
        SendYaTerInfo(ntohs(head.ethport));
        break;
    case IPTER_INFO:
        SendIpTerInfo(ntohs(head.ethport));
        break;
    case FILE_INFO:
        SendFileInfo();
        break;
    case QUE_INFO:
        SendQueInfo();
        break;
    case ACK:
        SendAck();
        break;
    }

    return Continue;
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

    if (filesize > 8191)
        filesize = 8191;

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

    return 0;
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

bool DevClient::CheckAuthStr(u_char *auth_str, u_int random_num)
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
    console.log(ss.str(), DBG_SDATA);
}

void DevClient::RLog(int totlen, const char *typestr)
{
    std::ostringstream ss;
    ss << "收到客户端状态请求[intf=" << typestr << "]";
    console.log(ss.str(), DBG_SPACK);
}

int DevClient::SendVersionRequire()
{
    Head head;
    head.origin = CLIENT;
    head.type = CLIENT_VER_REQ;
    head.totlen = htons(12);
    head.ethport = 0x0000;
    head.datalen = htons(4);

    uint16_t main = htons(2);
    uint8_t sub1 = htons(0);
    uint8_t sub2 = htons(0);

    u_char buf[12];
    memcpy(buf, &head, sizeof(head));
    // 最低版本号
    memcpy(buf + sizeof(head), &main, 2);
    // 次1版本号
    memcpy(buf + sizeof(head) + 1, &sub1, 1);
    // 次2版本号
    memcpy(buf + sizeof(head) + 2, &sub2, 1);

    int ret = 0;
    ret = write(sock, buf, ntohs(head.totlen));

    SLog(ntohs(head.totlen), ret, "最低版本要求", buf);
    return 0;
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

    // 各个端口数量 8 bytes
    offset += 8;

    uint32_t did = htonl(devid);
    memcpy(buf + offset, &did, sizeof(did));

    offset += 8;

    // 认证串
    int random_num = (u_int)rand();
    u_char *auth_string = GenAuthStr(random_num);
    memcpy(buf + offset, auth_string, 32);
    offset += 32;
    delete auth_string;

    memcpy(buf + offset, &random_num, sizeof(random_num));
    offset += sizeof(random_num);

    int ret = 0;

    ret = write(sock, buf, offset);

    SLog(offset, ret, "认证信息", buf);
    return 0;
}

int DevClient::SendSysInfo()
{
    int offset = 0;
    Head head;
    head.origin = CLIENT;
    head.type = SYS_INFO;
    head.totlen = htons(7 * 4);
    head.ethport = 0x0000;
    head.datalen = htons(5 * 4);

    u_char buf[head.totlen];
    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);

    SysInfo info;

    // CPU
    std::ifstream fin("/proc/stat");
    int N = 100;
    char line[N];

    while (!fin.eof())
    {
        fin.getline(line, N);
        std::vector<std::string> strs = split(line, " ");
        if (trim(strs[0]) == "cpu")
        {
            info.user_time = htonl(atoi(strs[1].c_str()));
            info.nice_time = htonl(atoi(strs[2].c_str()));
            info.sys_time = htonl(atoi(strs[3].c_str()));
            info.idle_time = htonl(atoi(strs[4].c_str()));
            break;
        }
    }
    fin.clear();
    fin.close();

    // MEM
    fin.open("/proc/meminfo");
    while (!fin.eof())
    {
        fin.getline(line, N);
        std::vector<std::string> strs = split(line, ":");
        std::string item = trim(strs[0]);
        if (item == "MemFree" || item == "Buffers" || item == "Cached")
        {
            info.free_mem += atoi(trim(strs[1]).c_str());
            if (item == "Cached")
            {
                info.free_mem = htonl(info.free_mem);
                break;
            }
        }
    }
    fin.close();

    memcpy(buf, &info, sizeof(info));
    offset += sizeof(info);

    int ret = 0;
    ret = write(sock, buf, offset);

    SLog(offset, ret, "系统信息", buf);
    return 0;
}

int DevClient::SendConfInfo()
{
    int offset = 0;
    Head head;
    head.origin = CLIENT;
    head.type = CONF_INFO;
    head.ethport = 0x0000;

    u_char *databuf = NULL;
    u_char *buf = NULL;
    int buflen;

    ReadFileToBuf("data/config.dat", databuf, buflen);
    buf = new u_char[buflen + sizeof(head) + 1];
    head.totlen = htons(buflen + sizeof(head) + 1);
    head.datalen = htons(buflen + 1);

    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);
    memcpy(buf + offset, buf, buflen);
    offset += buflen;
    buf[offset] = '\0';
    offset += 1;
    delete databuf;

    int ret = 0;
    ret = write(sock, buf, offset);

    SLog(offset, ret, "配置信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendProcInfo()
{
    int offset = 0;
    Head head;
    head.origin = CLIENT;
    head.type = PROC_INFO;
    head.ethport = 0x0000;

    u_char *databuf = NULL;
    u_char *buf = NULL;
    int buflen;

    ReadFileToBuf("data/process.dat", databuf, buflen);
    buf = new u_char[buflen + sizeof(head) + 1];
    head.totlen = htons(buflen + sizeof(head) + 1);
    head.datalen = htons(buflen + 1);

    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);
    memcpy(buf + offset, buf, buflen);
    offset += buflen;
    buf[offset] = '\0';
    offset++;

    delete databuf;

    int ret = 0;
    ret = write(sock, buf, offset);

    SLog(offset, ret, "进程信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendEthInfo(uint16_t eth_port)
{
    int offset = 0;
    Head head;
    EthInfo info;

    head.origin = CLIENT;
    head.type = ETH_INFO;
    head.totlen = sizeof(head) + sizeof(info);
    head.ethport = htons(eth_port);
    head.datalen = sizeof(info);

    std::ifstream fin("/proc/net/dev");
    std::string eth_name;
    int N = 1024;
    char line[N];

    // 第一行
    fin.getline(line, N);

    // 1端口需再读入一行
    if (eth_port == 1)
        fin.getline(line, N);

    fin.getline(line, N);
    std::istringstream ss;
    ss.str(line);

    ss >> eth_name >> info.rbytes >> info.rpackets >> info.rerrs >> info.rdrop >> info.rfifo >> info.rframe >> info.rcompressed >> info.rmulticast;
    ss >> info.tbytes >> info.tpackets >> info.terrs >> info.tdrop >> info.tfifo >> info.tframe >> info.tcompressed >> info.tmulticast;

    fin.close();

    info.rbytes = htonl(info.rbytes);
    info.rpackets = htonl(info.rpackets);
    info.rerrs = htonl(info.rerrs);
    info.rdrop = htonl(info.rdrop);
    info.rfifo = htonl(info.rfifo);
    info.rframe = htonl(info.rframe);
    info.rcompressed = htonl(info.rcompressed);
    info.rmulticast = htonl(info.rmulticast);
    info.tbytes = htonl(info.tbytes);
    info.tpackets = htonl(info.tpackets);
    info.terrs = htonl(info.terrs);
    info.tdrop = htonl(info.tdrop);
    info.tfifo = htonl(info.tfifo);
    info.tframe = htonl(info.tframe);
    info.tcompressed = htonl(info.tcompressed);
    info.tmulticast = htonl(info.tmulticast);

    u_char *buf = new u_char[head.totlen];
    // 转网络序
    head.totlen = htons(head.totlen);
    head.datalen = htons(head.datalen);
    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);
    memcpy(buf + offset, &info, sizeof(info));
    offset += sizeof(info);

    int ret = write(sock, buf, offset);

    SLog(offset, ret, "以太口信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendUsbInfo()
{
    Head head;

    head.origin = CLIENT;
    head.type = USB_INFO;
    head.totlen = htons(sizeof(head) + 4);
    head.ethport = htons(0x0000);
    head.datalen = htons(sizeof(4));

    u_char *buf = new u_char[head.totlen];
    memcpy(buf, &head, sizeof(head));
    buf[sizeof(head)] = rand() % 2;

    int ret = write(sock, buf, ntohs(head.totlen));

    SLog(ntohs(head.totlen), ret, "U盘信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendPrtInfo()
{
    int offset = 0;
    Head head;

    head.origin = CLIENT;
    head.type = PRT_INFO;
    head.totlen = htons(sizeof(head) + 4 + 32);
    head.ethport = htons(0x0000);
    head.datalen = htons(sizeof(4));

    u_char *buf = new u_char[head.totlen];
    memcpy(buf, &head, sizeof(head));
    buf[sizeof(head)] = rand() % 2;
    buf[sizeof(head) + 2] = rand() % 10;
    offset = sizeof(head) + 4;

    for (int j = 0; j < 32; j++)
        buf[offset + j] = ((u_char)rand() % ('z' - '0' + 1)) + '0';
    offset += 32;

    int ret = write(sock, buf, offset);

    SLog(offset, ret, "打印口信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendTerInfo()
{
    int offset = 0;
    Head head;

    head.origin = CLIENT;
    head.type = TER_INFO;
    head.totlen = htons(sizeof(head) + 16 + 254 + 2);
    head.ethport = htons(0x0000);
    head.datalen = htons(16 + 254 + 2);

    u_char *buf = new u_char[head.totlen];

    memcpy(buf, &head, sizeof(head));
    offset = sizeof(head);

    uint16_t num = htons(ttynum);
    memcpy(buf + 16 + 254, &num, sizeof(num));
    offset = offset + 16 + 254 + 2;

    int ret = write(sock, buf, offset);

    SLog(offset, ret, "终端服务信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendYaTerInfo(uint16_t no)
{

    int datalen = sizeof(Head) + (7 * 4) + scrnum_list[no] * sizeof(ScreenInfo);

    u_char *buf = new u_char[sizeof(Head) + datalen];

    int offset;
    Head head;
    head.origin = CLIENT;
    head.type = YATER_INFO;
    head.ethport = htons(no);
    head.totlen = htons(datalen + sizeof(Head));
    head.datalen = htons(datalen);

    memcpy(buf, &head, sizeof(Head));
    offset = sizeof(Head);
    buf[offset + 3] = (u_char)scrnum_list[no];

    write(sock, buf, sizeof(Head) + datalen);

    return 0;
}

int DevClient::SendIpTerInfo(uint16_t no)
{
    int datalen = sizeof(Head) + (7 * 4) + scrnum_list[no] * sizeof(ScreenInfo);

    u_char *buf = new u_char[sizeof(Head) + datalen];

    int offset;
    Head head;
    head.origin = CLIENT;
    head.type = IPTER_INFO;
    head.ethport = htons(no);
    head.totlen = htons(datalen + sizeof(Head));
    head.datalen = htons(datalen);

    memcpy(buf, &head, sizeof(Head));
    offset = sizeof(Head);
    buf[offset + 3] = (u_char)scrnum_list[no];

    write(sock, buf, sizeof(Head) + datalen);

    return 0;
}

int DevClient::SendFileInfo()
{
    int offset = 0;
    Head head;
    head.origin = CLIENT;
    head.type = FILE_INFO;
    head.ethport = 0x0000;

    u_char *databuf = NULL;
    u_char *buf = NULL;
    int buflen;

    ReadFileToBuf("data/usbfiles.dat", databuf, buflen);
    buf = new u_char[buflen + sizeof(head) + 1];
    head.totlen = htons(buflen + sizeof(head) + 1);
    head.datalen = htons(buflen + 1);

    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);
    memcpy(buf + offset, buf, buflen);
    offset += buflen;
    buf[offset] = '\0';
    offset++;

    delete databuf;

    int ret = 0;
    ret = write(sock, buf, offset);

    SLog(offset, ret, "USB文件列表信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendQueInfo()
{
    int offset = 0;
    Head head;
    head.origin = CLIENT;
    head.type = FILE_INFO;
    head.ethport = 0x0000;

    u_char buf[9];

    head.totlen = htons(sizeof(head) + 1);
    head.datalen = htons(1);

    memcpy(buf, &head, sizeof(head));
    offset += sizeof(head);
    buf[offset] = '\0';
    offset++;

    int ret = 0;
    ret = write(sock, buf, offset);

    SLog(offset, ret, "打印队列信息", buf);
    return 0;
}

int DevClient::SendAck()
{
    Head head;
    head.origin = CLIENT;
    head.type = ACK;
    head.totlen = 8;
    head.ethport = 0x0000;
    head.datalen = 0;
    // 转网络序
    head.totlen = htons(head.totlen);
    head.datalen = htons(head.datalen);
    int ret = write(sock, &head, sizeof(head));

    SLog(sizeof(head), ret, "ACK", (u_char *)&head);
    return 0;
}
