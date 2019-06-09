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
        return -1;
    }

    ss << "Connected(" << g_config.serverIp << ':' << g_config.port << ") OK.";
    console.log(ss.str(), DBG_ENV);
    return 1;
}

Status DevClient::WaitForMsg(Head &head, u_char *&databuf, int &buflen)
{
    int ret = 0;

    buflen = 0;
    ret = read(sock, &head, sizeof(Head));
    if (ret < 0)
    {
        console.log("socket close", DBG_ENV);
        return Close;
    }

    // 转主机序
    head.totlen = ntohs(head.totlen);
    head.datalen = ntohs(head.datalen);

    if (ret)
    {
        databuf = new (std::nothrow) u_char[head.datalen];
        if (databuf == NULL)
        {
            console.log("new databuf error", DBG_ERR);
            return Close;
        }

        // 读入数据部分
        while (true)
        {
            ret = read(sock, databuf + buflen, head.datalen);
            buflen += ret;
            if (buflen == head.datalen)
                break;
        }

        switch (head.type)
        {
        case SERVER_AUTH_REQ:
        {
            RLog(head, "认证信息", databuf);
            break;
        }
        case SYS_INFO:
            RLog(head, "系统信息", databuf);
            break;
        case CONF_INFO:
            RLog(head, "配置信息", databuf);
            break;
        case PROC_INFO:
            RLog(head, "进程信息", databuf);
            break;
        case ETH_INFO:
            RLog(head, "以太口信息", databuf);
            break;
        case USB_INFO:
            RLog(head, "USB信息", databuf);
            break;
        case PRT_INFO:
            RLog(head, "打印口信息", databuf);
            break;
        case TER_INFO:
            RLog(head, "终端服务信息", databuf);
            break;
        case YATER_INFO:
            RLog(head, "哑终端信息", databuf);
            break;
        case IPTER_INFO:
            RLog(head, "IP终端信息", databuf);
            break;
        case FILE_INFO:
            RLog(head, "文件列表信息", databuf);
            break;
        case QUE_INFO:
            RLog(head, "打印队列信息", databuf);
            break;
        case ACK:
            RLog(head, "ACK", databuf);
            break;
        }
    }
    else if (ret < 0)
    { // error
        std::ostringstream ss;
        ss << "create socket error: " << strerror(errno) << " (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        return Close;
    }
    else
    { // EOF
        console.log("socket close", DBG_ENV);
        return Close;
    }

    return Continue;
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
        if (ntohs(data.main) < 2)
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
            if (CheckAuthStr(data.authstr, ntohl(data.random_num)))
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
    long filesize = ftell(fp);

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
        std::ostringstream ss;
        ss << "file read do not complete, filesize=" << filesize << ",ret=" << ret;
        console.log(ss.str(), DBG_ERR);
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

void DevClient::EncryptData(u_char *buf, int random_num, uint buflen)
{
    int pos;
    pos = (random_num % 4093);

    for (uint i = 0; i < buflen; i++)
    {
        buf[i] ^= secret[pos];
        pos++;
        pos = pos % 4093;
    }
}

bool DevClient::CheckAuthStr(u_char *auth_str, u_int random_num)
{
    u_char key[] = AUTH_STR;

    int pos = (random_num % 4093);
    for (u_int i = 0; i < 32; i++)
    {
        if (auth_str[i] != (key[i] ^ secret[pos]))
            return false;
        pos++;
        pos = pos % 4093;
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

void DevClient::RLog(Head head, const char *typestr, u_char *data)
{
    std::ostringstream ss;
    int totlen = head.totlen;
    u_char *buf = new u_char[totlen];
    memcpy(buf, &head, sizeof(Head));
    memcpy(buf + sizeof(Head), data, head.datalen);

    ss << "收到服务端状态请求[intf=" << typestr << "]";
    console.log(ss.str(), DBG_RPACK);
    ss.clear();
    ss.str("");
    ss << "读取 " << totlen << "字节";
    console.log(ss.str(), DBG_RPACK);
    ss.clear();
    ss.str("");
    ss << "(读取数据为:)" << std::endl
       << binstr(buf, totlen);
    console.log(ss.str(), DBG_RDATA);
    delete buf;
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
    uint totlen = sizeof(Head) + sizeof(DevConf);
    Head head;
    DevConf data;
    head.origin = CLIENT;
    head.type = CLIENT_DEV_INFO;
    head.totlen = htons(totlen);
    head.ethport = 0x0000;
    head.datalen = htons(sizeof(DevConf));

    // FLASH 大小 + 设备内部序列号
    data.flash_size = htons((uint16_t)rand());
    data.inner_num = htons((uint16_t)rand());

    // 设备组序列号 + 型号 + 软件版本号
    // u_char char128[16];
    // for (int i = 0; i < 3; i++, offset += sizeof(char128))
    // {
    //     for (int j = 0; j < 15; j++)
    //         char128[j] = ((u_char)rand() % ('z' - '0' + 1)) + '0';

    //     char128[15] = 0;
    // }

    // 各个端口数量 8 bytes
    data.eth_num = rand() % 3;
    data.sync_num = rand() % 3;
    data.async_num = (rand() % 3) * 8;
    async_num = data.async_num;
    data.exchange_num = (rand() % 4) * 8;
    data.usb_num = rand() % 2;
    data.printer_num = rand() % 2;
    data.devid = htonl(devid);
    data.devInner_num = 1;
    memcpy(data.auth_string, AUTH_STR, 32);
    // 认证串
    uint32_t random_num = (uint32_t)rand();
    data.random_num = htonl(random_num);
    // 加密
    EncryptData((u_char *)&data, random_num, 104);

    u_char *buf = new u_char[totlen];
    memcpy(buf, &head, sizeof(head));
    memcpy(buf + sizeof(Head), &data, sizeof(DevConf));

    int ret = 0;
    ret = write(sock, buf, totlen);
    SLog(totlen, ret, "认证信息", buf);

    delete buf;
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

    memcpy(buf + sizeof(Head), &info, sizeof(info));
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
    memcpy(buf + offset, databuf, buflen);
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
    memcpy(buf + offset, databuf, buflen);
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
    head.datalen = htons(4 + 32);

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
    int totlen = sizeof(Head) + sizeof(TermServ);
    Head head;
    TermServ data;

    head.origin = CLIENT;
    head.type = TER_INFO;
    head.totlen = htons(totlen);
    head.ethport = htons(0x0000);
    head.datalen = htons(sizeof(TermServ));

    int async_term_num = 0;
    if (async_num != 0)
    {
        async_term_num = ((uint8_t)rand()) % async_num;
        if (async_term_num > ttynum)
            ttynum = async_num;
    }
    int ipterm_num = ttynum - async_term_num;

    for (int i = 0; i < 16; i++)
    {
        if (i < async_term_num)
            data.ya_term[i] = 1;
        else
            data.ya_term[i] = 0;
    }

    for (int i = 0; i < 254; i++)
    {
        if (i < ipterm_num)
            data.ip_term[i] = 1;
        else
            data.ip_term[i] = 0;
    }
    data.term_num = (rand()%(270 - ttynum)) + ttynum;

    u_char *buf = new u_char[totlen];
    memcpy(buf, &head, sizeof(head));
    memcpy(buf + sizeof(Head), &data, sizeof(TermServ));

    int ret = write(sock, buf, totlen);

    SLog(totlen, ret, "终端服务信息", buf);
    delete buf;
    return 0;
}

int DevClient::SendYaTerInfo(uint16_t no)
{
    int datalen = sizeof(TtyInfo) + scrnum_list[no] * sizeof(ScreenInfo);
    int totlen = sizeof(Head) + datalen;

    u_char *buf = new u_char[totlen];

    Head head;
    TtyInfo tty;
    ScreenInfo scr;

    head.origin = CLIENT;
    head.type = YATER_INFO;
    head.ethport = htons(no);
    head.totlen = htons(totlen);
    head.datalen = htons(datalen);

    memcpy(buf, &head, sizeof(Head));

    tty.scrnum = scrnum_list[no];
    tty.act_screen = rand() % scrnum_list[no];
    tty.port = tty.config_port = no;
    memset(tty.ter_ip, 0, 4);
    memset(tty.ter_type, 0, 12);
    memcpy(tty.ter_type, "串口终端", 12);
    memset(tty.ter_state, 0, 8);    
    if (rand() % 2)
    {
        memcpy(tty.ter_state, "正常", strlen("正常"));
    }
    else
    {
        memcpy(tty.ter_state, "菜单", strlen("菜单"));
    }
    memcpy(buf + sizeof(Head), &tty, sizeof(TtyInfo));

    for (int i = 0; i < tty.scrnum; i++)
    {
        scr.no = i + 1;
        scr.port = htons(g_config.port);
        scr.ip_addr = htonl(inet_addr(g_config.serverIp.c_str()));
        scr.con_time = htonl((uint32_t)time(0));
        const char *state[] = {
            "开机",
            "关机",
            "已登录"};
        int index = rand() % 3;
        memcpy(scr.state, state[index], strlen(state[index]));

        memcpy(buf + sizeof(Head) + sizeof(TtyInfo) + i * sizeof(ScreenInfo), &scr, sizeof(ScreenInfo));
    }

    int ret = write(sock, buf, totlen);
    SLog(totlen, ret, "哑终端信息", buf);

    return 0;
}

int DevClient::SendIpTerInfo(uint16_t no)
{
    int datalen = sizeof(TtyInfo) + scrnum_list[no] * sizeof(ScreenInfo);
    int totlen = sizeof(Head) + datalen;

    u_char *buf = new u_char[totlen];

    Head head;
    TtyInfo tty;
    ScreenInfo scr;

    head.origin = CLIENT;
    head.type = IPTER_INFO;
    head.ethport = htons(no);
    head.totlen = htons(totlen);
    head.datalen = htons(datalen);

    memcpy(buf, &head, sizeof(Head));

    tty.scrnum = scrnum_list[no];
    tty.act_screen = rand() % scrnum_list[no];
    tty.port = tty.config_port = no;
    memset(tty.ter_ip, 10, 4);
    memset(tty.ter_type, 0, 12);
    memset(tty.ter_state, 0, 8);    
    if (rand() % 2)
    {
        memcpy(tty.ter_type, "IP终端", strlen("IP终端"));
    }
    else
    {
        memcpy(tty.ter_type, "IP代理", strlen("IP代理"));
    }

    if (rand() % 2)
    {
        memcpy(tty.ter_state, "正常", strlen("正常"));
    }
    else
    {
        memcpy(tty.ter_state, "菜单", strlen("菜单"));
    }

    for (int i = 0; i < tty.scrnum; i++)
    {
        scr.no = i + 1;
        scr.port = htons(g_config.port);
        scr.ip_addr = htonl(inet_addr(g_config.serverIp.c_str()));
        scr.con_time = htonl((uint32_t)time(0));
        const char *state[] = {
            "开机",
            "关机",
            "已登录"};
        int index = rand() % 3;
        memcpy(scr.state, state[index], strlen(state[index]));

        memcpy(buf + sizeof(Head) + sizeof(TtyInfo) + i * sizeof(ScreenInfo), &scr, sizeof(ScreenInfo));
    }

    memcpy(buf + sizeof(Head), &tty, sizeof(TtyInfo));

    int ret = write(sock, buf, sizeof(Head) + datalen);
    SLog(totlen, ret, "IP终端信息", buf);

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
    head.type = QUE_INFO;
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
