#pragma once
#include <cstdint>


struct DBConfig
{
    DBConfig() {
        port = 0;
        ipaddr = NULL;
        name = NULL;
    }
    uint32_t port;
    char* ipaddr;
    char* name;
};