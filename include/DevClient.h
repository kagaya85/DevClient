#ifndef DEVCLIENT
#define DEVCLIENT

#include "defines.h"

class DevClient
{
private:
    Config config;

public:
    DevClient(Config config);
    ~DevClient();
};


#endif