#pragma once
#include <cstdint>


struct SMTPConfig
{
    SMTPConfig() {
        port = 0;
        server = NULL;
        username = NULL;
        password = NULL;
        auth_enabled = true;
    }

    uint32_t port;
    char* server;
    char* username;
    char* password;
    bool auth_enabled;
};