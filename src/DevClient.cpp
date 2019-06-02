#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "defines.h"
#include "DevClient.h"

using namespace std;

DevClient::DevClient(Config config)
{
}

DevClient::~DevClient()
{
}

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
    config->port = 0;
    config->sucExt = 1;       // 进程接受成功后退出 1：退出 0：间隔若干时间再次发送
    config->minClinum = 0;    // 最小配置终端数量
    config->maxClinum = 0;    // 最大配置终端数量
    config->minScrnum = 0;    // 每个终端最小虚屏数量
    config->maxScrnum = 0;    // 每个终端最大虚屏数量
    config->delLog = 0;       // 删除日志文件
    config->debug = 0; // DEBUG 设置
    config->showDbg = 0;      // DEBUG 屏幕显示

    while (!fin.eof())
    {
        char tmp;
        string item, value;
        getline(fin, buf);
        istringstream sin(trim(buf));
        tmp = sin.peek();
        if (tmp - '#')
        {
            sin >> item >> value;
            if (item == "服务器IP地址")
            {
                config->serverIp = value;
            }
            else if (item == "端口号")
            {
                config->port = stoi(value);
            }
            else if (item == "进程接收成功后退出")
            {
                config->sucExt = value == "1" ? true : false;
            }
            else if (item == "最小配置终端数量")
            {
                config->minClinum = stoi(value);
            }
            else if (item == "最大配置终端数量")
            {
                config->maxClinum = stoi(value);
            }
            else if (item == "每个终端最小虚屏数量")
            {
                config->minScrnum = stoi(value);
            }
            else if (item == "每个终端最大虚屏数量")
            {
                config->maxScrnum = stoi(value);
            }
            else if (item == "删除日志文件")
            {
                config->delLog = value == "1" ? true : false;
            }
            else if (item == "DEBUG设置")
            {
                config->debug = data2bin(value.c_str());
            }
            else if (item == "DEBUG屏幕显示")
            {
                config->showDbg = value == "1" ? true : false;
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
    out << left << "当前配置如下:" << endl;
    out << left << setw(30) << "\t服务器IP地址"
        << ": " << config.serverIp << endl;
    out << left << setw(30) << "\t端口号"
        << ": " << config.port << endl;
    out << left << setw(30) << "\t进程接受成功后退出"
        << ": " << config.sucExt << endl;
    out << left << setw(30) << "\t最小配置终端数量"
        << ": " << config.minClinum << endl;
    out << left << setw(30) << "\t最大配置终端数量"
        << ": " << config.maxClinum << endl;
    out << left << setw(30) << "\t每个终端最小虚屏数量"
        << ": " << config.minScrnum << endl;
    out << left << setw(30) << "\t每个终端最大虚屏数量"
        << ": " << config.maxScrnum << endl;
    out << left << setw(30) << "\t删除日志文件"
        << ": " << config.delLog << endl;
    out << left << setw(30) << "\tDEBUG屏幕显示"
        << ": " << config.showDbg << endl;
    out << left << setw(30) << "\tDEBUG设置"
        << ": " << dbgString(config.debug) << endl;
}

int main(int argc, char **argv)
{
    // 参数处理
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

    // 读取配置文件
    Config config;
    readConfig(&config);
    showConfig(cout, config);
    // 赋值全局变量

    // 开启子进程
    int status;
    pid_t pid;

    return 0;
}
