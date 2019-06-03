#include <iostream>
#include <sstream>

#include "Defines.h"
#include "DevClient.h"
#include "Logger.h"

extern Config g_config;
extern Logger console;

using namespace std;

/**
 * DevClient
 */
DevClient::DevClient() {
    // 初始化 socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // 参数最后一个0代表自动选择协议
        ostringstream ss;
        ss << "create socket error: "<< strerror(errno) <<" (errno: " << errno << ")";
        console.log(ss.str(), DBG_ERR);
        exit(0);
    }
}

DevClient::~DevClient() {}

