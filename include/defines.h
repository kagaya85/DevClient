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
    std::string serverIp;    // ������IP
    int port;
    bool sucExt;       // ���̽��ܳɹ����˳� 1���˳� 0���������ʱ���ٴη���
    int minClinum;   // ��С�����ն�����
    int maxClinum;   // ��������ն�����
    int minScrnum;  // ÿ���ն���С��������
    int maxScrnum;  // ÿ���ն������������
    bool delLog;    // ɾ����־�ļ�
    unsigned char debug;    // DEBUG ����
    bool showDbg;   // DEBUG ��Ļ��ʾ
};

#endif // !DEFINES
