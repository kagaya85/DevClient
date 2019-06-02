#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "DevClient.h"
#include "Defines.h"
#include "Logger.h"

using namespace std;

// ȫ�����ñ���
Config g_config;

DevClient::DevClient(Config config) {}

DevClient::~DevClient() {}

string &trim(string &str)
{
    if (str.empty())
    {
        return str;
    }
    string blanks("\f\v\r\t\n ");
    str.erase(0, str.find_first_not_of(blanks));
    str.erase(str.find_last_not_of(blanks) + 1);
    return str;
}

inline unsigned char data2bin(const char *data)
{
    unsigned char c = '\0';
    for (int i = 0; i < 8; i++)
    {
        if (data[i] == '1')
            c = (c << 1) | 1;
        else
            c = c << 1;
    }
    return c;
}

void readConfig(Config *config)
{
    ifstream fin(CONFIG_FILENAME);
    string buf;

    config->serverIp = "192.168.1.246";
    config->port = 40275;
    config->sucExt = 1;     // ���̽��ܳɹ����˳� 1���˳� 0���������ʱ���ٴη���
    config->minClinum = 5;  // ��С�����ն�����
    config->maxClinum = 28; // ��������ն�����
    config->minScrnum = 3;  // ÿ���ն���С��������
    config->maxScrnum = 10; // ÿ���ն������������
    config->delLog = 0;     // ɾ����־�ļ�
    config->debug = 0;      // DEBUG ����
    config->showDbg = 0;    // DEBUG ��Ļ��ʾ

    while (!fin.eof())
    {
        char tmp;
        string item, value;
        getline(fin, buf);
        istringstream sin(trim(buf));
        tmp = sin.peek();

        int only_once[10] = {0};
        if (tmp - '#')
        {
            sin >> item >> value;
            if (item == "������IP��ַ")
            {
                if (only_once[0])
                    continue;
                config->serverIp = value;
                only_once[0] = 1;
            }
            else if (item == "�˿ں�")
            {
                if (only_once[1])
                    continue;
                config->port = stoi(value);
                only_once[1] = 1;
            }
            else if (item == "���̽��ճɹ����˳�")
            {
                if (only_once[2])
                    continue;
                config->sucExt = value == "1" ? true : false;
                only_once[2] = 1;
            }
            else if (item == "��С�����ն�����")
            {
                if (only_once[3])
                    continue;
                int v = stoi(value);
                if (v < 3 || v > 10)
                    v = 5;
                config->minClinum = v;
                only_once[3] = 1;
            }
            else if (item == "��������ն�����")
            {
                if (only_once[4])
                    continue;
                int v = stoi(value);
                if (v < 10 || v > 50)
                    v = 5;
                config->maxClinum = v;
                only_once[4] = 1;
            }
            else if (item == "ÿ���ն���С��������")
            {
                if (only_once[5])
                    continue;
                int v = stoi(value);
                if (v < 1 || v > 3)
                    v = 3;
                config->minScrnum = v;
                only_once[5] = 1;
            }
            else if (item == "ÿ���ն������������")
            {
                if (only_once[6])
                    continue;
                int v = stoi(value);
                if (v < 4 || v >= 16)
                    v = 10;
                config->maxScrnum = v;
                only_once[6] = 1;
            }
            else if (item == "ɾ����־�ļ�")
            {
                if (only_once[7])
                    continue;
                config->delLog = value == "1" ? true : false;
                only_once[7] = 1;
            }
            else if (item == "DEBUG����")
            {
                if (only_once[8])
                    continue;
                config->debug = data2bin(value.c_str());
                only_once[8] = 1;
            }
            else if (item == "DEBUG��Ļ��ʾ")
            {
                if (only_once[9])
                    continue;
                config->showDbg = value == "1" ? true : false;
                only_once[9] = 1;
            }
        }
    }

    return;
}

string dbgString(const unsigned char debug)
{
    string result;

    if (debug | DBG_ENV)
    {
        result += "ENV";
    }

    if (debug | DBG_ERR)
    {
        result += " | ";
        result += "ERR";
    }

    if (debug | DBG_SPACK)
    {
        result += " | ";
        result += "SPACK";
    }

    if (debug | DBG_RPACK)
    {
        result += " | ";
        result += "RPACK";
    }

    if (debug | DBG_SDATA)
    {
        result += " | ";
        result += "SDATA";
    }

    if (debug | DBG_RDATA)
    {
        result += " | ";
        result += "RDATA";
    }

    return result;
}

void showConfig(ostream &out, Config &config)
{
    out << left << "��ǰ��������:" << endl;
    out << left << setw(30) << "\t������IP��ַ"
        << ": " << config.serverIp << endl;
    out << left << setw(30) << "\t�˿ں�"
        << ": " << config.port << endl;
    out << left << setw(30) << "\t���̽��ܳɹ����˳�"
        << ": " << config.sucExt << endl;
    out << left << setw(30) << "\t��С�����ն�����"
        << ": " << config.minClinum << endl;
    out << left << setw(30) << "\t��������ն�����"
        << ": " << config.maxClinum << endl;
    out << left << setw(30) << "\tÿ���ն���С��������"
        << ": " << config.minScrnum << endl;
    out << left << setw(30) << "\tÿ���ն������������"
        << ": " << config.maxScrnum << endl;
    out << left << setw(30) << "\tɾ����־�ļ�"
        << ": " << config.delLog << endl;
    out << left << setw(30) << "\tDEBUG��Ļ��ʾ"
        << ": " << config.showDbg << endl;
    out << left << setw(30) << "\tDEBUG����"
        << ": " << dbgString(config.debug) << endl;
}

int main(int argc, char **argv)
{
    // ��������
    if (argc != 3)
    {
        cerr << "Need at least 2 args: [device id] [client number]" << endl;
        exit(1);
    }

    int devid = atoi(argv[1]);
    int clinum = atoi(argv[2]);

    if (!devid || !clinum)
    {
        cerr << "Parameters illegal." << endl;
        exit(1);
    }
    // ������־��¼
    Logger console;
    console.setDevid(argv[1]);
    console.log("Hello world");

    // ��ȡ�����ļ�
    readConfig(&g_config);
    showConfig(cout, g_config);


    // �����ӽ���
    int status;
    pid_t pid;

    return 0;
}