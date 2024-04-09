#pragma once

#include <string>
#include <vector>

#include "ActiveDirectory.hpp"
#include "DataConversion.hpp"

struct ActiveDirectoryConfigDLL
{
    ActiveDirectoryConfigDLL()
    {
        port = 0;
        server = "";
        domain = "";
    }

    ActiveDirectoryConfigDLL (const ActiveDirectoryConfig& adCfg) : ActiveDirectoryConfigDLL()
    {
        port = adCfg.port;
        DataConversion::convertToStandardString (server, adCfg.server);
        DataConversion::convertToStandardString (domain, adCfg.domain);
    }

    ActiveDirectoryConfig ToCStyle()
    {
        ActiveDirectoryConfig ad = {};

        ad.port = port;
        DataConversion::convertToCharPointer (ad.server, server);
        DataConversion::convertToCharPointer (ad.domain, domain);

        return ad;
    }

    uint16_t port;
    std::string server;
    std::string domain;
};

struct ActiveDirectoryGroupDLL
{
    ActiveDirectoryGroupDLL()
    {
        role = UserPermissionLevel::eNormal;
        group = "";
    }

    static std::vector<std::string> Get (
        const std::string domain,
		const std::string server,
		const std::string username,
        const std::string password,
        const bool tls,     // defaults to false.
        const bool unsafe); // defaults to false.

    ActiveDirectoryGroupDLL (const ActiveDirectoryGroup& adGroup)
    {
        DataConversion::convertToStandardString (group, adGroup.group);
        role = adGroup.role;
    }

    ActiveDirectoryGroup ToCStyle()
    {
        ActiveDirectoryGroup adg = {};

        DataConversion::convertToCharPointer (adg.group, group);
        adg.role = role;

        return adg;
    }

    UserPermissionLevel role;
    std::string group;
};
