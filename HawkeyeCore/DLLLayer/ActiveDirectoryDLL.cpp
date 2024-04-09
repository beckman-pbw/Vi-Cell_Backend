#include "stdafx.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ActiveDirectoryDLL.hpp"
#include "ActiveDirectoryGroups.h"

//*****************************************************************************
std::vector<std::string> ActiveDirectoryGroupDLL::Get(
    const std::string domain, 
	const std::string server,
	const std::string username,
    const std::string password,
    const bool tls,     // defaults to false.
    const bool unsafe   // defaults to false.
    )
{
    GoString server_gs = { server.c_str(), static_cast<int>(server.length()) };
    GoString domain_gs = { domain.c_str(), static_cast<int>(domain.length()) };
    GoString username_gs = { username.c_str(), static_cast<int>(username.length()) };
    GoString password_gs = { password.c_str(), static_cast<int>(password.length()) };
    GoUint8  tls_gi = static_cast<bool>(tls);
    GoUint8  unsafe_gi = static_cast<bool>(unsafe);

    //TODO: status???  if "outputString" is empty then the login failed???
    // On error I got:
    //"ERROR->LDAP Result Code 1 \"Operations Error\": 000004DC: LdapErr: DSID-0C0907E1, comment: In order to perform this operation a successful bind must be completed on the connection., data 0, v2580"
    //On success I get:
    //One of the substrings will contain the username 
    //E.g. "SVCSoftwareTest1,,nFront-ServiceAccounts,HadoopUsers"
    //E.g. "Pursel, James,JPURSEL@beckman.com,BEC AutoDL – All Mgr – LS (w/out China),BEC AutoDL – All Assoc – LS (w/out China),SCCM_MS_Visio_Users,BEC AutoDL - ALL Assoc – US (w/Contingent Workers),VIP...

    std::string outputString = getAdGroups_extern(server_gs, domain_gs, username_gs, tls_gi, unsafe_gi, password_gs);

    std::stringstream ss(outputString);

    std::vector<std::string> result;
    while (ss.good())
    {
        std::string substr;
        getline (ss, substr, ',');
        result.push_back (substr);

    }

    return result;
}

