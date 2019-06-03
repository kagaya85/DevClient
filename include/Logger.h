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
    std::string m_devId;
public:
    Logger();
    ~Logger();
    void setDevid(const std::string & devId);
    std::string getCurrentTime();
    // ��־��¼����
    void log(const std::string & strInfo, int type = 0);
};

#endif