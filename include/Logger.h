#ifndef LOGGER
#define LOGGER
#include <stdio.h>
#include <string>
static const int MAX_STR_LEN = 1024;

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
    char m_devId[30];
public:
    Logger();
    ~Logger();
    void setDevid(const char* devId);
    std::string getCurrentTime();
    // ��־��¼����
    void log(const char * strInfo);
};

#endif