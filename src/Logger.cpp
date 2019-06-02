#include "Logger.h"
#include "Defines.h"
#include <iostream>
#include <sys/file.h>
#include <string.h>

extern Config g_config;

Logger::Logger()
{
    //��ʼ��
	m_pFileStream = NULL;
    char temp[1024] = {0};
    strcat(temp, m_logPath);
    strcat(temp, m_logName);
    m_pFileStream = fopen(temp, "a+");
    if (!m_pFileStream)
    {
        std::cerr << "Can't open log file." << std::endl;
        return;
    }
}

Logger::~Logger()
{
    //�ر��ļ���
	if(m_pFileStream)
		fclose(m_pFileStream);
}

//��ȡϵͳ��ǰʱ��
char *Logger::getCurrentTime()
{
    time_t curTime;
    struct tm *pTimeInfo = NULL;
    time(&curTime);
    pTimeInfo = localtime(&curTime);
    char temp[MAX_STR_LEN] = {0};
    // sprintf(temp, "%04d-%02d-%02d %02d:%02d:%02d", pTimeInfo->tm_year + 1990, pTimeInfo->tm_mon + 1, pTimeInfo->tm_mday,pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);
    sprintf(temp, "%02d:%02d:%02d", pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);
    char *pTemp = temp;
    return pTemp;
}

// ����devid
void Logger::setDevid(const char* devId)
{
    if(devId > 0)
        strcpy(this->m_devId, devId);
}

//д�ļ�����
void Logger::log(const char *strInfo)
{
    if (!strInfo)
        return;
    try
    {
        //���ļ���û�д򿪣������´�
        if (!m_pFileStream)
        {
            char temp[1024] = {0};
            strcat(temp, m_logPath);
            strcat(temp, m_logName);
            m_pFileStream = fopen(temp, "a+");
            if (!m_pFileStream)
            {
                return;
            }
        }
        int m_fd = fileno(m_pFileStream);
        //�����ٽ������ļ�����
        flock(m_fd, LOCK_EX);
        //д��־��Ϣ���ļ���
        fprintf(m_pFileStream, "%s [%s] %s\n", getCurrentTime(), m_devId, strInfo);
        fflush(m_pFileStream);
        //�뿪�ٽ���
        flock(m_fd, LOCK_UN);
    }
    //�������쳣�������뿪�ٽ�������ֹ����
    catch (...)
    {
        int m_fd = fileno(m_pFileStream);
        flock(m_fd, LOCK_UN);
    }
}