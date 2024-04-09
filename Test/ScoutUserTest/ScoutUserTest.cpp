//*****************************************************************************
// This test app is used to set up for testing with ScoutTest.
// It is derived from *UserTest" application.
//*****************************************************************************

#include "stdafx.h"
#include <iostream>
#include <cstdio>
#include <string>

#include "Configuration.hpp"
#include "HawkeyeError.hpp"
#include "Logger.hpp"
#include "SecurityHelpers.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "ScoutUserTest";

static UserList ul_;


//*****************************************************************************
HawkeyeError SetUserProperty (const char* uname, const char* propertyname, uint32_t nmembers, char** propertymembers) {
	std::vector<std::string> vs_members;

	for (std::size_t i = 0; i < nmembers; i++) {
		vs_members.push_back(propertymembers[i]);
	}

	return ul_.SetUserProperty(uname, propertyname, vs_members);
}

//*****************************************************************************
HawkeyeError SetUserCellTypes (const char* name, uint32_t nCells, uint32_t* celltype_indices) {
	
	char** celltypes = new char*[nCells];
	
	for (uint32_t i = 0; i < nCells; i++) {
		celltypes[i] = new char[15];
		_itoa_s(celltype_indices[i], celltypes[i], 15, 10);
	}

	HawkeyeError he = SetUserProperty (name, "CellTypes", nCells, celltypes);

	for (uint32_t i = 0; i < nCells; i++) {
		delete(celltypes[i]);
	}
	delete[] celltypes;

	return he;
}

//*****************************************************************************
void bci_service_login() {

	Logger::L().Log (MODULENAME, severity_level::normal, "Generate *bci_service* account credentials and log into *bci_service* account...");
	std::string name;
	HawkeyeError he = ul_.GetLoggedInUserName (name);
	std::string pc = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_YEARS);
	bool success = SecurityHelpers::ValidateHMACPasscode (pc, SecurityHelpers::HMAC_YEARS);

	ul_.LoginUser("bci_service", pc);
	he = ul_.GetLoggedInUserName (name);
}

//*****************************************************************************
void bci_admin_login() {

	Logger::L().Log (MODULENAME, severity_level::normal, "Generate *bci_admin* account credentials and log into *bci_admin* account...");
	std::string name;
	HawkeyeError he = ul_.GetLoggedInUserName (name);
	std::string pc = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_SECS);
	bool success = SecurityHelpers::ValidateHMACPasscode (pc, SecurityHelpers::HMAC_SECS, 1, 0);

	ul_.LoginUser ("bci_admin", pc);
	he = ul_.GetLoggedInUserName (name);
}

//*****************************************************************************
int main(int argc, char* argv[]) {
	HawkeyeError he;
	boost::system::error_code ec;
	Logger::L().Initialize (ec, "ScoutUserTest.info");

	Logger::L().Log (MODULENAME, severity_level::normal, "Starting ScoutUserTest");

	std::vector<UserList::t_userProperty> uProperties;
	UserList::t_userProperty uProperty;

	std::string ulFilename = "userlist.info";

	bci_service_login();

	{
		{
			// Create an empty userlist.info file so that when OpenConfigFile is called
			// it will return a valid pTree object pointer.
			FILE* fptr;
			fopen_s (&fptr, "userlist.info", "wt");
			fputs ("\r\n", fptr);
			fclose (fptr);

			Logger::L().Log (MODULENAME, severity_level::normal, "Generate *bci_admin* account and save to *" + ulFilename + "*");

			ul_.AddUser ("bci_admin", "System Admin", "bci_admin", eAdministrator, "AdminFolder", uProperties);

			t_pPTree cfg = ConfigUtils::OpenConfigFile ("userlist.info", ec, true);
			cfg->add_child ("UsersGoHere", boost::property_tree::ptree{});

			t_opPTree ucfg = cfg->get_child_optional("UsersGoHere");

			ul_.WriteConfiguration (ucfg);

			ConfigUtils::WriteCachedConfigFile ("userlist.info", ec);

			Logger::L().Log (MODULENAME, severity_level::normal, "*bci_admin* account added to *" + ulFilename + "*");
		}

		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Load *" + ulFilename + "* and add user account");

			HawkeyeError he = ul_.LoadFromConfiguration (ulFilename);

			he = ul_.LoginUser ("bci_admin", "bci_admin");
			if (he != HawkeyeError::eSuccess) {
				Logger::L().Log (MODULENAME, severity_level::normal, "Error logging in *User1*, user not found.");
			} else {
				Logger::L().Log (MODULENAME, severity_level::normal, "bci_admin successfully logged in.");
			}

			std::string username = "user1";
			std::string password = "12345";
			ul_.AddUser (username.c_str(), "User User1", password.c_str(), eNormal, "User1Folder", uProperties);
			ul_.WriteConfiguration();

			// Set the cell types for this user.
			uint32_t celltype_indices_to_set[] = { 100, 200, 300 };
			uint32_t nCells = 3;
			he = SetUserCellTypes (username.c_str(), nCells, celltype_indices_to_set);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to set user's cell types: " << HawkeyeErrorAsString(he) << std::endl;
			}
			ul_.WriteConfiguration();

			ul_.LogoutUser();
		}
	}


	Logger::L().Log (MODULENAME, severity_level::normal, "Verify that the user account login works using new instance of UserList...");

	UserList ul;
	ul_.LoadFromConfiguration ("userlist.info");

	he = ul_.LoginUser ("user1", "12345");
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Error logging in *User1*, user not found.");
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "User1 successfully logged in.");
		ul_.LogoutUser();
	}

	{
		/*
		 * Test service password window.  We want to allow the service user to use a passcode
		 * up to 4 hours old (let them generate a passcode and then walk into a cleanroom for a few hours).
		 */

		 // Current time passcode:
		std::string PC = SecurityHelpers::GenerateHMACPasscode(SecurityHelpers::HMAC_HOURS);
		bool success = SecurityHelpers::ValidateHMACPasscode(PC, SecurityHelpers::HMAC_HOURS);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours.
		assert(success);
		
		he = ul_.LoginUser("bci_service", PC);
		assert(he == HawkeyeError::eSuccess);
		ul_.LogoutUser();
		/*
		// 3 hours old
		PC = SecurityHelpers::GenerateHMACPasscode(-3);
		success = SecurityHelpers::ValidateHMACPasscode(PC);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours.
		assert(success);

		he = ul_.LoginUser("bci_service", PC);
		assert(he == HawkeyeError::eSuccess);
		ul_.LogoutUser();

		// 4 hours old
		PC = SecurityHelpers::GenerateHMACPasscode(-4);
		success = SecurityHelpers::ValidateHMACPasscode(PC);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours., 4, 1);
		assert(success);

		he = ul_.LoginUser("bci_service", PC);
		assert(he == HawkeyeError::eSuccess);
		ul_.LogoutUser();

		// 5 hours old (should fail)
		PC = SecurityHelpers::GenerateHMACPasscode(-5);
		success = SecurityHelpers::ValidateHMACPasscode(PC);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours., 4, 1);
		assert(!success);

		he = ul_.LoginUser("bci_service", PC);
		assert(he != HawkeyeError::eSuccess);
		ul_.LogoutUser();

		// Ahead 1 hour (clock oopsie)
		PC = SecurityHelpers::GenerateHMACPasscode(1);
		success = SecurityHelpers::ValidateHMACPasscode(PC);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours., 4, 1);
		assert(success);

		he = ul_.LoginUser("bci_service", PC);
		assert(he == HawkeyeError::eSuccess);
		ul_.LogoutUser();

		// ahead 2 hours (suspicuous, isn't it?)
		PC = SecurityHelpers::GenerateHMACPasscode(2);
		success = SecurityHelpers::ValidateHMACPasscode(PC);// , 4, 1);// Eliminated because Service would rather do a yearly passowrd than one measured in hours., 4, 1);
		assert(!success);

		he = ul_.LoginUser("bci_service", PC);
		assert(he != HawkeyeError::eSuccess);
		ul_.LogoutUser();
		*/
	}
}
