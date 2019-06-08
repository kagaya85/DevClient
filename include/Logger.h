#ifndef LOGGER
#define LOGGER
#include <Defines.h>
#include <stdio.h>
#include <string>

static const int MAX_STR_LEN = 1024;

#define XLS_FILE "ts_count.xls"
#define CONFIG_FILENAME "ts.conf"
#define DBG_ENV 0x80
#define DBG_ERR 0x40
#define DBG_SPACK 0x20
#define DBG_RPACK 0x10
#define DBG_SDATA 0x8
#define DBG_RDATA 0x4

class Logger
{
private:
    // 写日志文件流
    FILE * m_pFileStream;
	// 日志的路径
	const char* m_logPath = "./";
	// 日志的名称
	const char* m_logName = "ts.log";
    // 设备ID
    std::string m_devId;
public:
    Logger();
    ~Logger();
    void setDevid(const std::string & devId);
    std::string getCurrentTime();
    // 日志记录函数
    void log(const std::string & strInfo, int type = 0);
    void xls(uint ttynum, uint scrnum);
};

#endif