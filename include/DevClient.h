#ifndef DEVCLIENT
#define DEVCLIENT

#include "Defines.h"

class DevClient
{
private:
    Config config;

public:
    DevClient(Config config);
    ~DevClient();
};


#endif