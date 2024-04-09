// UserTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <cstdio>

#include "Configuration.hpp"
#include "Logger.hpp"
#include "SecurityHelpers.hpp"
#include "UserList.hpp"
#include <thread>
#include <chrono>
static const char MODULENAME[] = "UserTest";

bool bci_service_login(UserList& ul) {

	bool success = true;
	Logger::L().Log (MODULENAME, severity_level::normal, "Generate *bci_service* account credentials and log into *bci_service* account...");
	
	std::string name;
	HawkeyeError he = ul.GetLoggedInUserName (name);

	std::string pc = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_YEARS);
	if (!SecurityHelpers::ValidateHMACPasscode(pc, SecurityHelpers::HMAC_YEARS))
		return false;

	if (ul.LoginUser(SERVICE_USER, pc) != HawkeyeError::eSuccess)
		return false;

	if ((ul.GetLoggedInUserName(name) != HawkeyeError::eSuccess) ||
		(name != SERVICE_USER))
		return false;

	return success;
}

bool silent_admin_login(UserList& ul)
{
	bool success = true;
	Logger::L().Log (MODULENAME, severity_level::normal, "Generate *silent admin* account credentials and log into *silent admin* account...");
	std::string name;
	HawkeyeError he = ul.GetLoggedInUserName(name);

	std::string pc = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_SECS);
	if (!SecurityHelpers::ValidateHMACPasscode(pc, SecurityHelpers::HMAC_SECS))
		return false;

	if (ul.LoginUser(SILENTADMIN_USER, pc) != HawkeyeError::eSuccess)
		return false;

	if ((ul.GetLoggedInUserName(name) != HawkeyeError::eSuccess) ||
		(name != SILENTADMIN_USER))
		return false;

	return success;
}


bool silent_admin_login_with_delay(UserList& ul, uint32_t ms)
{
	bool success = true;
	Logger::L().Log (MODULENAME, severity_level::normal, "Generate *silent admin* account credentials and log into *silent admin* account...");
	std::string name;
	HawkeyeError he = ul.GetLoggedInUserName(name);

	std::string pc = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_SECS);
	if (!SecurityHelpers::ValidateHMACPasscode(pc, SecurityHelpers::HMAC_SECS))
		return false;

	std::this_thread::sleep_for(std::chrono::milliseconds(ms));

	if (ul.LoginUser(SILENTADMIN_USER, pc) != HawkeyeError::eSuccess)
		return false;

	if ((ul.GetLoggedInUserName(name) != HawkeyeError::eSuccess) ||
		(name != SILENTADMIN_USER))
		return false;

	return success;
}

int main(int argc, char* argv[]) {
	
	boost::system::error_code ec;
	Logger::L().Initialize (ec, "UserTest.info");

	Logger::L().Log (MODULENAME, severity_level::normal, "Starting UserTest");

	std::vector<UserList::t_userProperty> uProperties;
	UserList::t_userProperty uProperty;

	std::string ulFilename = "userlist.info";
	UserList ul;


	if (!bci_service_login(ul))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to sign in with service account after user list creation");
	}
	ul.LogoutUser();

	if (silent_admin_login_with_delay(ul, 2000))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Silent Admin account should have failed with this delay but didn't");
	}

	if (!silent_admin_login(ul))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to sign in with silent admin account after user list creation");
	}
	// LEAVE SILENT ADMIN LOGGED IN FOR USER CREATION


	ul.AddUser ("User1", "User User1", "user1_12345", eNormal, "User1Folder", uProperties);
	ul.WriteUsersToDatabase();

	ul.LogoutUser();

	Logger::L().Log (MODULENAME, severity_level::normal, "Verify that the user account login works");

	UserList ul2;
	ul2.Initialize();

	HawkeyeError he = ul2.LoginUser ("User1", "user1_12345");
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Error logging in *User1*, user not found.");
	}
	else 
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "User1 successfully logged in.");
		ul2.LogoutUser();
	}

	{
		/*
		 * Test service password window.  We want to allow the service user to use a passcode
		 * up to 4 hours old (let them generate a passcode and then walk into a cleanroom for a few hours).
		 */

		// Current time passcode:
		std::string PC = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_YEARS);
		bool success = SecurityHelpers::ValidateHMACPasscode(PC, SecurityHelpers::HMAC_YEARS);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours.
		assert(success);

		he = ul2.LoginUser("bci_service", PC);
		assert(he == HawkeyeError::eSuccess);
		ul2.LogoutUser();
	}

	{
		/*
		 * Test burning user names.
		 *    Create a few more normal users; delete some of them.
		 *    Attempt to create a user using the name of a deleted one.
		 * Load a new user list up from the configuration.
		 *    Attempt to create a user using the name of a previously deleted one.
		 */

		if (!silent_admin_login(ul2))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to sign in with silent admin account after user list set from configuration");
		}
		// LEAVE SILENT ADMIN LOGGED IN FOR USER CREATION

		ul2.AddUser("User2", "User User2", "user2_12345", eNormal, "User2Folder", uProperties);
		ul2.AddUser("User3", "User User3", "user3_12345", eNormal, "User3Folder", uProperties);
		ul2.AddUser("User4", "User User4", "user4_12345", eNormal, "User4Folder", uProperties);
		ul2.AddUser("User5", "User User5", "user5_12345", eNormal, "User5Folder", uProperties);

		ul2.WriteUsersToDatabase();

		ul2.RemoveUser("User2");
		ul2.RemoveUser("User4");
		if (ul2.LoginUser("User2", "user2_12345") == HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Users did not delete as expected!");
		}
		ul2.WriteUsersToDatabase();

		if (ul2.AddUser("User2", "User User2", "user2_12345", eNormal, "User2Folder", uProperties) == HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "User allowed to be created with recycled name!");
		}
		else if (ul2.AddUser("User6", "User User2", "user2_12345", eNormal, "User2Folder", uProperties) == HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "User allowed to be created with recycled display name!");
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Use of recycled user/display names prevented.");
		}


		UserList ul3;
		ul3.Initialize();

		if (!silent_admin_login(ul3))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to sign in with silent admin account after user list 3 set from configuration");
		}
		if (ul2.AddUser("User2", "User User2", "user2_12345", eNormal, "User2Folder", uProperties) == HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "User allowed to be created with recycled name after re-load!");
		}
		else if (ul2.AddUser("User6", "User User2", "user2_12345", eNormal, "User2Folder", uProperties) == HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "User allowed to be created with recycled display name after re-load!");
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Use of recycled user/display names prevented  after re-load");
		}

	}


#if 0

	/*
	 * 1: Create a set of users.
	 * 2: Cache those users to a file.
	 * 3: REstore those users from the file.
	 * 4: Validate the known passwords for those users.
	 */

	std::vector<HawkeyeUser> UserList;

	HawkeyeUser user("Dennis", "DennisPassword", eElevated);
	
	FILE* fptr;
	fopen_s(&fptr, "UserList.info", "wt");
	std::string user_output = user.ToStorageString();
	fputs(user_output.c_str(), fptr);
	fclose(fptr);

	(user.ValidatePassword("DennisPassword")) ? std::cout << "Password check 1 passed (validated)" << std::endl
	                                          : std::cout << "Password check 1 failed (did not validate)" << std::endl;

	(user.ValidatePassword("DennisPassword1")) ? std::cout << "Password check 2 failed (validated wrongly)" << std::endl
	                                           : std::cout << "Password check 2 passed (did not validate)" << std::endl;


	fopen_s(&fptr, "UserList.info", "rt");
	char user_input[1024];
	fgets(user_input, 1024, fptr);
	fclose(fptr);
	std::string user_inputs(user_input);

	HawkeyeUser user2;
	user2.FromStorageString(user_inputs);

	(user2.IsValid()) ?	std::cout << "User readback validated correctly" << std::endl 
	                  : std::cout << "User readback failed to validate" << std::endl;
	
	(user2.UserName() == "Dennis") ? std::cout << "User name readback successful" << std::endl 
	                               : std::cout << "User name readback failed" << std::endl;

	(user2.ValidatePassword("DennisPassword")) ? std::cout << "Password check 1 passed (validated)" << std::endl
	                                           : std::cout << "Password check 1 failed (did not validate)" << std::endl;

	(user2.ValidatePassword("DennisPassword1")) ? std::cout << "Password check 2 failed (validated wrongly)" << std::endl
	                                            : std::cout << "Password check 2 passed (did not validate)" << std::endl;

	(user2.PermisionLevel() == eElevated) ?	std::cout << "Permission level read back correctly" << std::endl
	                                      :	std::cout << "Permission level did NOT read back correctly" << std::endl;

	(user2.IsActive()) ? std::cout << "User active readback successful" << std::endl
	                 : std::cout << "User active readback failed" << std::endl;


	user.SetPassword("DennisNewPassword");
	user.SetActivation(false);

	(user.ValidatePassword("DennisNewPassword")) ? std::cout << "Password check 3 passed (validated)" << std::endl
	                                             : std::cout << "Password check 1 failed (did not validate)" << std::endl;

	(user.ValidatePassword("DennisPassword")) ? std::cout << "Password check 4 failed (validated wrongly)" << std::endl
	                                          : std::cout << "Password check 2 passed (did not validate)" << std::endl;

	fopen_s(&fptr, "UserList.info", "wt");
	user_output = user.ToStorageString();
	fputs(user_output.c_str(), fptr);
	fclose(fptr);

	
	 //* For testing purposes: set a breakpoint here and alter the line in UserTest2.txt.
	 //* Any alteration should result in validation failures.
	 

	fopen_s(&fptr, "UserList.info", "rt");
	fgets(user_input, 1024, fptr);
	fclose(fptr);
	user_inputs = std::string(user_input);

	HawkeyeUser user3;
	user3.FromStorageString(user_inputs);

	(user3.IsValid()) ? std::cout << "User readback validated correctly" << std::endl
		: std::cout << "User readback failed to validate" << std::endl;

	(user3.UserName() == "Dennis") ? std::cout << "User name readback successful" << std::endl
		: std::cout << "User name readback failed" << std::endl;

	(user3.ValidatePassword("DennisNewPassword")) ? std::cout << "Password check 1 passed (validated)" << std::endl
		: std::cout << "Password check 1 failed (did not validate)" << std::endl;

	(user3.ValidatePassword("DennisPassword1")) ? std::cout << "Password check 2 failed (validated wrongly)" << std::endl
		: std::cout << "Password check 2 passed (did not validate)" << std::endl;

	(user3.PermisionLevel() == eElevated) ? std::cout << "Permission level read back correctly" << std::endl
		: std::cout << "Permission level did NOT read back correctly" << std::endl;

	(!user3.IsActive()) ? std::cout << "User active readback successful" << std::endl
		: std::cout << "User active readback failed" << std::endl;

#endif

	return 0;
}

