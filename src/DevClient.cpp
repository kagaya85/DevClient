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

/**
 * global
 */
Config g_config;

/**
 * DevClient
 */
DevClient::DevClient(Config config) {}

DevClient::~DevClient() {}

/**
 * Tool Function
 */
string binstr(const char *buf, const int buflen)
{
    string charPart;
    ostringstream sout;
    int i;
    
    for (i = 0; i < buflen; i++)
    {
        if (i % 16 == 0) // 行号
            sout << "  "  << setw(4) << setfill('0') << right << hex << i << ": ";
        else if (i % 16 != 0 && i % 8 == 0) // 分隔符
            sout << " -";

        unsigned int c = (unsigned int)buf[i];

        // 输出二进制
        sout << ' ' << setw(2) << setfill('0') << hex << c;

        // 转换为可视字符
        if (c >= 32 && c <= 126)
            charPart.push_back(char(c));
        else
            charPart.push_back('.');

        // 判断是否到行尾
        if ((i + 1) % 16 == 0)
        {
            sout << "  " << charPart << endl; // 换行
            charPart.clear();  // 清空
        }
    }

    // 如果最后一行不完整，补齐输出
    if ((i + 1) % 16 != 0)
    {
        for(;i % 16 != 0; i++) {
            sout << "   ";
            if(i % 8 == 0)  // 补齐分隔符的空格
                sout << "  ";
        }
        
        sout << "  " << charPart << endl; // 换行
    }
    
    return sout.str();
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

// 将字符串转二进制数
inline unsigned char str2bin(const char *data)
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
    config->sucExt = 1;     // 进程接受成功后退出 1：退出 0：间隔若干时间再次发送
    config->minClinum = 5;  // 最小配置终端数量
    config->maxClinum = 28; // 最大配置终端数量
    config->minScrnum = 3;  // 每个终端最小虚屏数量
    config->maxScrnum = 10; // 每个终端最大虚屏数量
    config->delLog = 0;     // 删除日志文件
    config->debug = 0;      // DEBUG 设置
    config->showDbg = 0;    // DEBUG 屏幕显示

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
            if (item == "服务器IP地址")
            {
                if (only_once[0])
                    continue;
                config->serverIp = value;
                only_once[0] = 1;
            }
            else if (item == "端口号")
            {
                if (only_once[1])
                    continue;
                config->port = stoi(value);
                only_once[1] = 1;
            }
            else if (item == "进程接收成功后退出")
            {
                if (only_once[2])
                    continue;
                config->sucExt = value == "1" ? true : false;
                only_once[2] = 1;
            }
            else if (item == "最小配置终端数量")
            {
                if (only_once[3])
                    continue;
                int v = stoi(value);
                if (v < 3 || v > 10)
                    v = 5;
                config->minClinum = v;
                only_once[3] = 1;
            }
            else if (item == "最大配置终端数量")
            {
                if (only_once[4])
                    continue;
                int v = stoi(value);
                if (v < 10 || v > 50)
                    v = 5;
                config->maxClinum = v;
                only_once[4] = 1;
            }
            else if (item == "每个终端最小虚屏数量")
            {
                if (only_once[5])
                    continue;
                int v = stoi(value);
                if (v < 1 || v > 3)
                    v = 3;
                config->minScrnum = v;
                only_once[5] = 1;
            }
            else if (item == "每个终端最大虚屏数量")
            {
                if (only_once[6])
                    continue;
                int v = stoi(value);
                if (v < 4 || v >= 16)
                    v = 10;
                config->maxScrnum = v;
                only_once[6] = 1;
            }
            else if (item == "删除日志文件")
            {
                if (only_once[7])
                    continue;
                config->delLog = value == "1" ? true : false;
                only_once[7] = 1;
            }
            else if (item == "DEBUG设置")
            {
                if (only_once[8])
                    continue;
                config->debug = str2bin(value.c_str());
                only_once[8] = 1;
            }
            else if (item == "DEBUG屏幕显示")
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

string confstr(Config &config)
{
    int width = 25;
    ostringstream ss;
    ss << left << "当前配置如下:" << endl;
    ss << left << setw(width) << "\t服务器IP地址"
        << ": " << config.serverIp << endl;
    ss << left << setw(width) << "\t端口号"
        << ": " << config.port << endl;
    ss << left << setw(width) << "\t进程接受成功后退出"
        << ": " << config.sucExt << endl;
    ss << left << setw(width) << "\t最小配置终端数量"
        << ": " << config.minClinum << endl;
    ss << left << setw(width) << "\t最大配置终端数量"
        << ": " << config.maxClinum << endl;
    ss << left << setw(width) << "\t每个终端最小虚屏数量"
        << ": " << config.minScrnum << endl;
    ss << left << setw(width) << "\t每个终端最大虚屏数量"
        << ": " << config.maxScrnum << endl;
    ss << left << setw(width) << "\t删除日志文件"
        << ": " << config.delLog << endl;
    ss << left << setw(width) << "\tDEBUG屏幕显示"
        << ": " << config.showDbg << endl;
    ss << left << setw(width) << "\tDEBUG设置"
        << ": " << dbgString(config.debug) << endl;
    
    return ss.str();
}

/**
 * Ko ↗ Ko → Da ↘ Yo ↗
 */
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
    // 开启日志记录
    Logger console;
    console.setDevid(argv[1]);
    console.log("Hello world");

    // 读取配置文件
    readConfig(&g_config);
    cout << confstr(g_config);

    cout << binstr("12345678901234567890", 20);

    // 开启子进程
    int status;
    pid_t pid;

    return 0;
}
