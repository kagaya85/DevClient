#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Defines.h"
#include "DevClient.h"
#include "Logger.h"

using namespace std;

/**
 * global
 */
// 全局配置结构体
Config g_config;
// 成功结束计数
int g_sucNum = 0;
// 正在运行计数
int g_runNum = 0;
// 建立正在运行进程的进程号与devid的映射关系
map<int, uint32_t> g_devidMap;
// 待传送设备号队列
queue<uint32_t> g_devidQueue;
// 日志记录器
Logger console;

/**
 * child 信号处理函数
 */
static void child(int signo)
{
    pid_t pid;
    int status;
    // ostringstream ss;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status)) // exit结束
        {

            // ss << "child " << pid << " exited normal exit status=" << WEXITSTATUS(status);
            // console.log(ss.str(), DBG_ENV);

            if (WEXITSTATUS(status) == 0)
                g_sucNum++;
            else
            {
                // 非正常结束，将pid对应的devid加入设备号队列
                g_devidQueue.push(g_devidMap[pid]);
            }
            g_runNum--;
        }
        else if (WIFSIGNALED(status)) // 信号结束
        {
            // ss << "child " << pid << " exited abnormal signal number=" << WTERMSIG(status);
            // console.log(ss.str(), DBG_ENV);

            // 非正常结束，将pid对应的devid加入设备号队列
            g_devidQueue.push(g_devidMap[pid]);
            g_runNum--;
        }
        else if (WIFSTOPPED(status)) // 暂停
        {
            // ss << "child " << pid << " stoped signal number=%d\n" << WSTOPSIG(status);
            // console.log(ss.str(), DBG_ENV);
        }
        // ss.str("");
    }
}

/**
 * Tool Function
 */
//字符串分割函数
std::vector<std::string> split(std::string str, std::string pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern; //扩展字符串以方便操作
    auto size = str.size();

    for (size_t i = 0; i < size; i++)
    {
        pos = str.find(pattern, i);
        if (pos < size)
        {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}

// 去掉前后空格
std::string &trim(std::string &str)
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
inline u_char str2bin(const char *data)
{
    u_char c = '\0';
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
    config->sucExt = 1;      // 进程接受成功后退出 1：退出 0：间隔若干时间再次发送
    config->min_ttynum = 5;  // 最小配置终端数量
    config->max_ttynum = 28; // 最大配置终端数量
    config->min_scrnum = 3;  // 每个终端最小虚屏数量
    config->max_scrnum = 10; // 每个终端最大虚屏数量
    config->delLog = 0;      // 删除日志文件
    config->debug = 0;       // DEBUG 设置
    config->showDbg = 0;     // DEBUG 屏幕显示

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
                config->min_ttynum = v;
                only_once[3] = 1;
            }
            else if (item == "最大配置终端数量")
            {
                if (only_once[4])
                    continue;
                int v = stoi(value);
                if (v < 10 || v > 50)
                    v = 5;
                config->max_ttynum = v;
                only_once[4] = 1;
            }
            else if (item == "每个终端最小虚屏数量")
            {
                if (only_once[5])
                    continue;
                int v = stoi(value);
                if (v < 1 || v > 3)
                    v = 3;
                config->min_scrnum = v;
                only_once[5] = 1;
            }
            else if (item == "每个终端最大虚屏数量")
            {
                if (only_once[6])
                    continue;
                int v = stoi(value);
                if (v < 4 || v >= 16)
                    v = 10;
                config->max_scrnum = v;
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

string dbgstr(const u_char debug)
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
       << ": " << config.min_ttynum << endl;
    ss << left << setw(width) << "\t最大配置终端数量"
       << ": " << config.max_ttynum << endl;
    ss << left << setw(width) << "\t每个终端最小虚屏数量"
       << ": " << config.min_scrnum << endl;
    ss << left << setw(width) << "\t每个终端最大虚屏数量"
       << ": " << config.max_scrnum << endl;
    ss << left << setw(width) << "\t删除日志文件"
       << ": " << config.delLog << endl;
    ss << left << setw(width) << "\tDEBUG屏幕显示"
       << ": " << config.showDbg << endl;
    ss << left << setw(width) << "\tDEBUG设置"
       << ": " << dbgstr(config.debug) << endl;

    return ss.str();
}

//将buf内的数据格式化后返回string
string binstr(const u_char *buf, const int buflen)
{
    string charPart;
    ostringstream sout;
    int i;

    for (i = 0; i < buflen; i++)
    {
        if (i % 16 == 0) // 行号
            sout << "  " << setw(4) << setfill('0') << right << hex << i << ": ";
        else if (i % 16 != 0 && i % 8 == 0) // 分隔符
            sout << " -";

        u_int c = (u_int)buf[i];

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
            charPart.clear();                 // 清空
        }
    }

    // 如果最后一行不完整，补齐输出
    if ((i + 1) % 16 != 0)
    {
        for (; i % 16 != 0; i++)
        {
            sout << "   ";
            if (i % 8 == 0) // 补齐分隔符的空格
                sout << "  ";
        }

        sout << "  " << charPart << endl; // 换行
    }

    return sout.str();
}

// 生成连续的devid，放入队列中
void createDevid(uint32_t startId, int clinum)
{
    for (int i = 0; i < clinum; i++)
    {
        g_devidQueue.push(startId);
        startId++;
    }
}

/**
 * Ko ↗ Ko → Da ↘ Yo ↗
 */
int main(int argc, char **argv)
{
    // 参数处理
    if (argc != 3)
    {
        cerr << "Need 2 parameters at least : [device id] [client number]" << endl;
        exit(1);
    }
    // 删除xls
    remove("ts_count.xls");
    int devid = atoi(argv[1]);
    int clinum = atoi(argv[2]);

    if (!devid || !clinum)
    {
        cerr << "Parameters illegal." << endl;
        exit(1);
    }

    // 读取配置文件
    readConfig(&g_config);

    // 打开log文件
    console.openFile();

    // 生成devid
    createDevid(devid, clinum);

    // 转变为守护进程
    // daemon(0, g_config.showDbg); // 工作在当前目录，由showDbg决定是否输出到屏幕

    // 设置信号处理函数
    signal(SIGCHLD, child);

    console.setDevid(to_string(getpid()));
    console.log("主进程启动成功", DBG_ENV);
    console.log(confstr(g_config), DBG_ENV);

    // 开启子进程
    pid_t cpid;
    console.log("子进程fork开始", DBG_ENV);
    time_t start = time(0);
    while (true)
    {
        if (g_sucNum + g_runNum < clinum && g_runNum < 100)
        {
            cpid = fork();
            if (cpid < 0)
            {
                // 分裂失败
                if (sleep(5) == 5) // 重试
                    child(SIGCHLD);
                continue;
            }
            // child process
            else if (cpid == 0)
            {
                Head head;
                u_char *databuf = NULL;
                int buflen = 0;
                uint32_t devid = g_devidQueue.front();
                // 设置父进程结束后子进程结束
                prctl(PR_SET_PDEATHSIG, SIGKILL);
                // DevClient
                DevClient client(devid);

                // 开启日志记录
                console.setDevid(to_string(devid));

                Status sta;
                while (true)
                {
                    // 与服务器通信
                    client.Connect();

                    while (true)
                    {
                        // 阻塞等待消息
                        sta = client.WaitForMsg(head, databuf, buflen);
                        if (sta == Close)
                            break;
                        // 消息处理
                        sta = client.MsgHandler(head, databuf, buflen);
                        if (sta == Close)
                            break;
                    }

                    if (databuf)
                    {
                        delete databuf;
                        databuf = NULL;
                        buflen = 0;
                    }

                    if (g_config.sucExt) // 接受成功后退出
                    {
                        console.log("client exit.");
                        break;
                    }
                    else // 间隔一段时间后重复发送
                    {
                        uint sleeptime = g_config.resend_time;
                        while (sleeptime)
                            sleeptime -= sleep(sleeptime);
                    }
                }

                console.xls(client.ttynum, client.scrnum);
                return 0;
            }
            else
            {
                // parent process
                uint32_t devid = g_devidQueue.front();
                g_devidQueue.pop();       // 弹出
                g_devidMap[cpid] = devid; // 这里插入，相同cpid会覆盖
                g_runNum++;
            }
        }
        else
        {
            // 入睡等待子进程结束
            uint sleeptime = 10;
            if (g_sucNum < clinum)
            {
                if (sleep(sleeptime) == sleeptime) // 睡够事件后主动回收子进程
                    child(SIGCHLD);
            }
            else // 全部发送成功，父进程退出
                break;
        }
    }

    time_t end = time(0);
    ostringstream ss;
    ss << "子进程fork结束, 总数=" << g_sucNum << ", 耗时=" << end - start << "秒";
    console.log(ss.str(), DBG_ENV);

    return 0;
}
