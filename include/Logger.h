#ifndef LOGGER
#define LOGGER
#include <stdio.h>
static const int MAX_STR_LEN = 1024;

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
    char m_devId[30];
public:
    Logger();
    ~Logger();
    void setDevid(const char* devId);
    char* getCurrentTime();
    // 日志记录函数
    void log(const char * strInfo);
};

#endif