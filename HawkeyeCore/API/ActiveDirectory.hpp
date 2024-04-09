#pragma once

#include <string>

//#include "HawkeyeError.hpp"
#include "UserLevels.hpp"

struct ActiveDirectoryConfig
{
    ActiveDirectoryConfig() {
        port = 0;
        server = NULL;
        domain = NULL;
    }

    uint16_t port;
    char* server;
    char* domain;
};

struct ActiveDirectoryGroup
{
    ActiveDirectoryGroup() {
        role = UserPermissionLevel::eNormal;
        group = NULL;
    }

    UserPermissionLevel role;
    char* group;
};
