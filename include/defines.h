#ifndef DEFINES
#define DEFINES

#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

// 客户端返回状态，关闭连接，定时重连，继续等待
enum Status{Close, Reconnect, Continue};

#define AUTH_STR "yzmond:id*str&to!tongji@by#Auth^"

struct Config {
    std::string serverIp;    // 服务器IP
    int port;
    bool sucExt;       // 进程接受成功后退出 1：退出 0：间隔若干时间再次发送
    int min_ttynum;   // 最小配置终端数量
    int max_ttynum;   // 最大配置终端数量
    int min_scrnum;  // 每个终端最小虚屏数量
    int max_scrnum;  // 每个终端最大虚屏数量
    bool delLog;    // 删除日志文件
    unsigned char debug;    // DEBUG 设置
    bool showDbg;   // DEBUG 屏幕显示
    // 连接后获取
    uint32_t reconn_time;   // 重连间隔
    uint32_t resend_time;   // 发送完毕后重发间隔
};

// 报文头结构体
struct Head {
    u_char origin;
    u_char type;
    uint16_t totlen;
    uint16_t ethport;
    uint16_t datalen;
};

/**
 * tools
 */
std::string binstr(const u_char *buf, const int buflen);
std::string confstr(Config &config);
std::string dbgstr(const u_char debug);
std::vector<std::string> split(std::string str, std::string pattern);
std::string &trim(std::string &str);

#endif // !DEFINES
