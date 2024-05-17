#include "stdafx.h"

#include <chrono>
#include <string>

#include "AutomationUser.hpp"
#include "ChronoUtilities.hpp"
#include "ExpandedUsers.hpp"
#include "SecurityHelpers.hpp"


// Automation User Credentials dll;
//
// Used to provide the username and password of the internal local ViCell automation user
// for tightly coupled automation clients (those running in the ViCell instrument),
// removing the need for those clients to know the credentials used (e.g. username), and
// thus allowing the ViCell to control the automation userneame and the password generation
// algorithm used for that user.
//
// NOT intended for use by external remote automation clients to avoid multiple logins
// using the same user ID by multiple remote hosts that would defeat the P&S requirement
// for discrete user logins.

static std::string password;
static std::string username = AUTOMATION_USER;

DLL_CLASS const char* AutomationPassword()
{
	TIME_ZONE_INFORMATION TimeZoneInfo;
	GetTimeZoneInformation(&TimeZoneInfo);
	static int tzOffset = -( static_cast<int>( TimeZoneInfo.Bias ) );

	std::chrono::system_clock::time_point now(std::chrono::system_clock::now());
	now -= std::chrono::minutes(tzOffset);

	password = SecurityHelpers::GenerateHMACPasscode(now, SecurityHelpers::HMAC_HOURS);
	return password.c_str();
}

DLL_CLASS const char* AutomationUsername()
{
	return username.c_str();
}
