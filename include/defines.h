#ifndef DEFINES
#define DEFINES

#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

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

// ����Ͷ�ṹ��
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
std::string binstr(const u_char *buf, const int buflen);
std::string confstr(Config &config);
std::string dbgstr(const u_char debug);
std::vector<std::string> split(std::string str, std::string pattern);
std::string &trim(std::string &str);

#endif // !DEFINES
