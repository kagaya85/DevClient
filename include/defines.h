#ifndef DEFINES
#define DEFINES

#include <string>

#define CONFIG_FILENAME "ts.conf"
#define DBG_ENV 0x80
#define DBG_ERR 0x40
#define DBG_SPACK 0x20
#define DBG_RPACK 0x10
#define DBG_SDATA 0x8
#define DBG_RDATA 0x4

struct Config {
    std::string serverIp;    // 服务器IP
    int port;
    bool sucExt;       // 进程接受成功后退出 1：退出 0：间隔若干时间再次发送
    int minClinum;   // 最小配置终端数量
    int maxClinum;   // 最大配置终端数量
    int minScrnum;  // 每个终端最小虚屏数量
    int maxScrnum;  // 每个终端最大虚屏数量
    bool delLog;    // 删除日志文件
    unsigned char debug;    // DEBUG 设置
    bool showDbg;   // DEBUG 屏幕显示
};

// 报文投结构体
struct Head {
    char origin;
    char type;
    short totlen;
    short ethport;
    short datalen;
};

/**
 * tools
 */
std::string binstr(const char *buf, const int buflen);
std::string confstr(Config &config);
std::string dbgstr(const unsigned char debug);

#endif // !DEFINES
