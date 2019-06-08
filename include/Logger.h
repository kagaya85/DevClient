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
    // д��־�ļ���
    FILE * m_pFileStream;
	// ��־��·��
	const char* m_logPath = "./";
	// ��־������
	const char* m_logName = "ts.log";
    // �豸ID
    std::string m_devId;
public:
    Logger();
    ~Logger();
    void setDevid(const std::string & devId);
    std::string getCurrentTime();
    // ��־��¼����
    void log(const std::string & strInfo, int type = 0);
    void xls(uint ttynum, uint scrnum);
};

#endif