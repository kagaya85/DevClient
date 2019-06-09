#ifndef DEFINES
#define DEFINES

#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

// �ͻ��˷���״̬���ر����ӣ���ʱ�����������ȴ�
enum Status{Close, Reconnect, Continue};

#define AUTH_STR "yzmond:id*str&to!tongji@by#Auth^"

struct Config {
    std::string serverIp;    // ������IP
    int port;
    bool sucExt;       // ���̽��ܳɹ����˳� 1���˳� 0���������ʱ���ٴη���
    int min_ttynum;   // ��С�����ն�����
    int max_ttynum;   // ��������ն�����
    int min_scrnum;  // ÿ���ն���С��������
    int max_scrnum;  // ÿ���ն������������
    bool delLog;    // ɾ����־�ļ�
    unsigned char debug;    // DEBUG ����
    bool showDbg;   // DEBUG ��Ļ��ʾ
    // ���Ӻ��ȡ
    uint32_t reconn_time;   // �������
    uint32_t resend_time;   // ������Ϻ��ط����
};

// ����ͷ�ṹ��
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
