// ReSharper disable CppInconsistentNaming
#include "stdafx.h"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <windows.h>
#include <processthreadsapi.h>
#include <functional>

#include <DBif_Api.h>

// This must be here in the list of include files.
#include "UserList.hpp"

#include "AuditLog.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "FileSystemUtilities.hpp"
#include "GetAsStrFunctions.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeUUID.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "PtreeConversionHelper.hpp"
#include "SystemErrors.hpp"
#include "UserLevels.hpp"

static const char MODULENAME[] = "UserList";

const std::string DEFAULT_USER = "DefaultUser";
const std::string DEFAULT_ELEVATED = "DefaultElevated";
const std::string DEFAULT_ADMIN = "DefaultAdmin";
const uint32_t MIN_PER_DAY = 24 * 60;
static std::list<ExpandedUser> userList = {};
static std::vector<UserList::BurnedUsers> burnedUsernameList = {};
static DBApi::DB_UserRoleRecord normalRole = {};
static DBApi::DB_UserRoleRecord elevatedRole = {};
static DBApi::DB_UserRoleRecord adminRole = {};
static std::vector<DBApi::DB_UserRoleRecord> dbRolesList = {};
static bool isInitialized = false;
static 	std::vector<DBApi::DB_UserRecord> s_dbUserList = {};

// Make the current username available to the error logging code (SystemErrors.cpp).  This is a terrible hack exposing
// a variable this way; yet because not all applications in the solution use all of the HawkeyeLogicImpl
// code we're forced to support both users of HawkeyeCore DLL and the standalone applications that
// use only fragments of the HawkeyeCore codebase.
// the g_ prefix helps to point out this is a global variable
std::string g_currentSysErrorUsername;

bool g_currentSystemAutomationLockstatus;

// the current set of logged-in remote users and their count of concurrent logins.
static std::map<std::string, std::size_t> m_currentRemoteUsers{};

//*****************************************************************************
static bool IsAnyRemoteUserLoggedIn()
{
	return !m_currentRemoteUsers.empty();
}

//*****************************************************************************
static int RemoteUserLoginCount(std::string username)
{
	auto remote = m_currentRemoteUsers.find(username);
	return (remote == m_currentRemoteUsers.end() ? 0 : remote->second);
}

//**************************************************************************
UserList::UserList()
{
	userList.clear();
	currentUser = userList.end();
	burnedUsernameList.clear();
}

// **************************************************************************
UserList::~UserList()
{
}

// **************************************************************************
HawkeyeError UserList::Initialize (bool isImporting)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");
	g_currentSystemAutomationLockstatus = false;
	HawkeyeError status = HawkeyeError::eSuccess;

	userList = {};
	dbRolesList = {};
	currentUser = userList.end();
	burnedUsernameList.clear();
	s_dbUserList = {};

	DBApi::eQueryResult queryResult = DBApi::DbGetRolesList (dbRolesList);
	if ((queryResult != DBApi::eQueryResult::QueryOk) || (dbRolesList.size() != 3))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Initialize: unable to get roles list from database");
		return HawkeyeError::eStorageFault;
	}

	for (auto role : dbRolesList)
	{
		if (role.RoleNameStr == DEFAULT_USER)
			normalRole = role;
		else if (role.RoleNameStr == DEFAULT_ELEVATED)
			elevatedRole = role;
		else if (role.RoleNameStr == DEFAULT_ADMIN)
			adminRole = role;
	}

	{
		// Get user list from DB
		std::vector<DBApi::eListFilterCriteria> filtertypelist;
		std::vector<std::string> filtercomparelist;
		std::vector<std::string> filtertgtlist;
		DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::NoSort;
		DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::NoSort;
		int32_t sortdir = -1;
		std::string orderstr = "";
		int32_t limitcnt = 0;
		int32_t startindex = -1;
		int64_t startidnum = -1;
		DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
		std::string filtercompare = "";
		std::string filtertgt = "";

		queryResult = DBApi::DbGetUserList (
			s_dbUserList,
			filtertype,
			filtercompare,
			filtertgt,
			limitcnt,
			primarysort,
			secondarysort,
			sortdir,
			orderstr,
			startindex,
			startidnum);
	}

	bool serviceUserInDB = false;
	bool silentAdminInDB = false;
	bool automationUserInDB = false;

	eSECURITYTYPE secType = GetSystemSecurityType();
	if (isImporting)
	{
		// Force security mode to LocalSecurity so that when the users are loaded during the v1.2 import of the UserList.einfo file
		// all of the known users will be loaded.  This is needed since in LocalSecurity mode only "ViCell and bci_service" are loaded.
		// This resulted in the failure to find the other users when the time came to import data from users other than "ViCell and bci_service".
		secType = eSECURITYTYPE::eLocalSecurity;
	}

	for (auto& dbUser : s_dbUserList)
	{
		ExpandedUser EU;

		if (!EU.userCore.FromStorageString(dbUser.AuthenticatorList[0]))
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				"Initialize: username <" + EU.userCore.UserName() + "> failed get name from auth - skipping");
			continue;
		}

		if (EU.userCore.UserName() != dbUser.UserNameStr)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				"Initialize: usernames don't match <" + EU.userCore.UserName() + "> != <" + dbUser.UserNameStr + "> skipping");
			continue;
		}

		EU.FromDbStyle (dbUser);

		if (EU.userCore.UserName() == SILENTADMIN_USER)
		{
			userList.push_back (EU);

			std::list<ExpandedUser>::iterator lastIter = userList.end();
			silentAdministrator = --lastIter;
			silentAdminInDB = true;
		}
		else if (EU.userCore.UserName() == SERVICE_USER)
		{
			// Service user password is arbitrary; it needs to be checked against a programatically generated HMAC key.
			// The password is valid for one calendar year.
			userList.push_back (EU);

			std::list<ExpandedUser>::iterator lastIter = userList.end();
			serviceUser = --lastIter;
			serviceUserInDB = true;
		}
		else if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule &&
			EU.userCore.UserName() == AUTOMATION_USER)
		{
			// Service user password is arbitrary; it needs to be checked against a programatically generated HMAC key.
			// The password is valid for one calendar year.
			userList.push_back (EU);

			std::list<ExpandedUser>::iterator lastIter = userList.end();
			automationUser = --lastIter;
			automationUserInDB = true;
		}
		else
		{
			// Make sure AD user names are all lower case
			if (EU.ADUser)
			{
				boost::algorithm::to_lower(dbUser.UserNameStr);
				EU.userCore.UsernameToLowerCase();
			}

			if (memcmp((void*)dbUser.RoleId.u, (void*)normalRole.RoleId.u, sizeof(uuid__t)) == 0)
			{
				EU.userCore.SetUserLevel(UserPermissionLevel::eNormal);
			}
			else if (memcmp((void*)dbUser.RoleId.u, (void*)elevatedRole.RoleId.u, sizeof(uuid__t)) == 0)
			{
				EU.userCore.SetUserLevel(UserPermissionLevel::eElevated);
			}
			else if (memcmp((void*)dbUser.RoleId.u, (void*)adminRole.RoleId.u, sizeof(uuid__t)) == 0)
			{
				EU.userCore.SetUserLevel(UserPermissionLevel::eAdministrator);
			}
			else
			{
				Logger::L().Log(MODULENAME, severity_level::error, "Initialize: Invalid Role for user " + EU.userCore.UserName() + " - skipping");
				continue;
			}

			if (dbUser.Retired)
			{
				// Load the burned username list.
				// Make sure the list only contains retired names from users in the current security setting.
				// To mimic v1.2, if security is off, add local + retired users to the burned list
				if (
					((secType == eSECURITYTYPE::eLocalSecurity) && !dbUser.ADUser) ||
					((secType == eSECURITYTYPE::eNoSecurity) && !dbUser.ADUser))
				{
					burnedUsernameList.push_back(BurnedUsers(dbUser.UserNameStr, dbUser.UserId));
				}
				else if ((secType == eSECURITYTYPE::eActiveDirectory) && dbUser.ADUser)
				{
					// AD Users are not allowed to be deleted so this is an error condition
					Logger::L().Log(MODULENAME, severity_level::error, "Initialize: Found retired AD user:  " + dbUser.UserNameStr);
					burnedUsernameList.push_back(BurnedUsers(dbUser.UserNameStr, dbUser.UserId));
				}
			}
			else
			{
				if (((secType == eSECURITYTYPE::eLocalSecurity) && !dbUser.ADUser) ||
					((secType == eSECURITYTYPE::eActiveDirectory) && dbUser.ADUser))
				{
					userList.push_back(EU);
				}
			}
		}

	} // End "for (auto& user : urlist)"

	if (!silentAdminInDB)
	{
		// Silent admin is not in the DB so add it
		ExpandedUser EU = create_silent_administrator();
		WriteUserToDatabase (EU);			// also adds new user to the end of the userlist

		std::list<ExpandedUser>::iterator lastIter = userList.end();
		silentAdministrator = --lastIter;
	}

	if (!serviceUserInDB)
	{
		// Service user is not in the DB so add it
		ExpandedUser EU = create_service_user();
		WriteUserToDatabase (EU);			// also adds new user to the end of the userlist

		std::list<ExpandedUser>::iterator lastIter = userList.end();
		serviceUser = --lastIter;
	}

	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		if (!automationUserInDB)
		{
			// Service user is not in the DB so add it
			ExpandedUser EU = create_automation_user();
			WriteUserToDatabase (EU);			// also adds new user to the end of the userlist

			std::list<ExpandedUser>::iterator lastIter = userList.end();
			automationUser = --lastIter;
		}

	}
	
	if (s_dbUserList.size() == 0)
	{
		AddFactoryAdminUser();				// also adds new user to the end of the userlist
	}

	currentUser = userList.end();

	assert(serviceUser->userCore.UserName() == SERVICE_USER);
	assert(silentAdministrator->userCore.UserName() == SILENTADMIN_USER);

	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		assert(automationUser->userCore.UserName() == AUTOMATION_USER);
	}

	isInitialized = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
eSECURITYTYPE UserList::GetSystemSecurityType()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "GetSystemSecurityFeaturesState: <enter/exit>");

	auto sectype = HawkeyeConfig::Instance().get().securityType.get();
	return static_cast<eSECURITYTYPE>(sectype);
}

HawkeyeError UserList::SetupExistingActiveDirectoryUser(const std::string& uName, const std::string& uPW, UserPermissionLevel adRole, std::list<ExpandedUser>::iterator user)
{
	if (!user->userCore.IsActive())
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_loginfailure, 
			"Failed login attempt for user \"" + uName + "\" account disabled"));
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	// User already exists and is an AD user.	
	user->lastLogin = ChronoUtilities::CurrentTime();
	const UserPermissionLevel myRole = user->userCore.PermisionLevel();
	if (myRole != adRole)
	{
		user->userCore.SetUserLevel(static_cast<UserPermissionLevel>(adRole));
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_userpermissionschange,
			boost::str(boost::format("AD user : Permission level change\n\tUser: \"%s\"\n\tNew value: %s\n\tPrevious value: %s")
				% uName % getPermissionLevelAsStr(static_cast<UserPermissionLevel>(adRole)) % getPermissionLevelAsStr(myRole))));
	}

	// @todo - v1.4 add email to info collected from AD server
	//if ((fndEmail.length() > 0) && (fndEmail != user.email))
	//	user.email = fndEmail;

	// We need to set the password so that Validate password works for AD users
	// Each time we login we must change the 'stored' password to match that given one
	user->userCore.SetPassword (uPW, true);
	return HawkeyeError::eSuccess;
}

ExpandedUser UserList::SetupNewActiveDirectoryUser(const std::string& uName, const std::string& uPW, const UserPermissionLevel adRole)
{
	auto EU = ExpandedUser(uName, uPW, uName, adRole);

	EU.allowFastMode = true;
	EU.ADUser = true;

	// AD users get access to all cell types.
	EU.cellTypeIndices.clear();
	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();
	for (auto& ct : cellTypesDLL)
	{
		EU.cellTypeIndices.push_back (ct.celltype_index);
	}

	// Currently the one and only analysis.
	EU.analysisIndices.push_back (0);
	
	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_useradd,
		"Added AD User\n\tName: \"" + uName +
			"\"\n\tDisplay name \"" + EU.displayName +
			"\"\n\tLevel: " + getPermissionLevelAsStr(EU.userCore.PermisionLevel())));

	// We need to set the password so that Validate password works for AD users
	// Each time we login we must change the 'stored' password to match that given
	EU.userCore.SetPassword (uPW, true);
	return EU;
}

/**
 ***************************************************************************
 * \brief Attempt to login with the given credentials. This sets the current user. 
 *
 * \param[in] uName - username of account to attempt a login for
 * \param[in] uPW the password to use in this login attempt
 *
 * \return eSuccess if the given account credentials were valid and the user is enabled
 * \return eEntryNotFound if the given name was not found
 */
 //*****************************************************************************
HawkeyeError UserList::LoginUser (const std::string& uName, const std::string& uPW, bool validateOnly)
{
	HawkeyeError he = HawkeyeError::eSuccess;
	std::string auditErrorStr = std::string("Failed validation attempt for user \"" + uName + "\".");

	g_currentSysErrorUsername = "";

	// When security is OFF, local users still need to be able to log in.
	// This is because even when security id OFF we still need to validate  
	// that a user is an admin user in order to turn security ON. 
	// Remote automation users ALWAYS need to login!
	if ((GetSystemSecurityType() != eSECURITYTYPE::eActiveDirectory) || (uName == SERVICE_USER) || (uName == SILENTADMIN_USER))
	{
		std::function<bool(const std::string& password)> PasswordValidator = nullptr;

		// In local security mode or no security mode
		// OR
		// The username is SERVICE_USER or SILENTADMIN_USER
		for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
		{
			if (user->userCore.UserName() != uName)
				continue;

			if (!user->userCore.IsValid())
				return HawkeyeError::eEntryInvalid;

			if (user == silentAdministrator)
			{
				PasswordValidator = [this](const std::string& password) -> bool
				{
					return ValidateHostPassword(password);
				};
			}
			else if (user == serviceUser || user == automationUser)
			{
				PasswordValidator = [this](const std::string& password) -> bool
				{
					return ValidateServicePassword(password);
				};
			}

			if (validateOnly)
			{
				bool isExpired = true;
				he = IsPasswordExpired (user->userCore.UserName(), isExpired);
				if (HawkeyeError::eSuccess != he)
				{
					Logger::L().Log(MODULENAME, severity_level::warning,
						boost::str (boost::format("LoginUser: <exit, IsPasswordExpired, %d") % (int)he));
					return he;
				}

				if (isExpired)
				{
					return HawkeyeError::eNotPermittedAtThisTime;
				}
			}

			if (HawkeyeError::eSuccess != user->AttemptLogin(uPW, true, PasswordValidator))
			{
				WriteUserToDatabase(*user);

				// The AttemptLogin() function handles all error conditions, audit logs.
				return HawkeyeError::eValidationFailed;
			}

			user->attemptCount = 0;
			
			// Add the audit log entry for the login for this user
			WriteUserToDatabase(*user); // The user info needs to be written to the DB 
			if (!validateOnly)
			{
				AuditLogger::L().Log(generateAuditWriteData(uName, audit_event_type::evt_login, std::string("Console login OK")));
				currentUser = user;
			}

			// Make the current username available to the error logging code.
			g_currentSysErrorUsername = std::string (user->userCore.UserName());

			return HawkeyeError::eSuccess;
		}

		Logger::L().Log(MODULENAME, severity_level::error, std::string("LoginUser: (local) <exit, user \"" + uName + "\" not found>"));
		auditErrorStr.append (", user not found");
		AuditLogger::L().Log(generateAuditWriteData(
			uName,
			audit_event_type::evt_loginfailure,
			auditErrorStr));
		return HawkeyeError::eEntryNotFound;
	}

	// ----------------------------------------------------------------------
	// AD security mode 
	// and
	// Not FACTORY_ADMIN Nor SERVICE_USER Nor SILENTADMIN_USER
	// ----------------------------------------------------------------------

	// Attempt to validate the given credentials using the AD server
	// If valid then we will also get a role for the user
	UserPermissionLevel adRole;
	std::string fndEmail = "";
	std::string adUsername = uName;

	// Usernames are always checked and stored as lowercase to prevent multiple
	// users with similar names.
	boost::algorithm::to_lower(adUsername);
	
	he = LoginADUser (adUsername, uPW, adRole, fndEmail);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, std::string("LoginUser: <exit, Failed to validate user \"" + adUsername + "\". User or role not found.>"));
		auditErrorStr.append(". User or role not found.");
		AuditLogger::L().Log(generateAuditWriteData(
			"",
			audit_event_type::evt_loginfailure,
			auditErrorStr));
		return he;
	}

	if (validateOnly)
	{
		// We validated the user with the AD server and the have a group to Role map
		// Now we need to check if the user has been disabled.
		// It doesn't matter if we don't have this user in our list, but,
		// if they are in the list, make sure they are enabled
		for (auto user = userList.begin(); user != userList.end(); ++user)
		{
			if (user->userCore.UserName() != adUsername)
				continue;

			if (!user->userCore.IsActive())
				return HawkeyeError::eNotPermittedAtThisTime;

			break; // We found the user and they are enabled
		}

		return HawkeyeError::eSuccess;
	}
	
	// Check if user already exists in the list / DB
	for (auto user = userList.begin(); user != userList.end(); ++user)
	{
		std::string str = user->userCore.UserName();
		boost::algorithm::to_lower(str);
		
		if (str != adUsername || !user->ADUser && user->userCore.UserName() != uName)
			continue;

		he = SetupExistingActiveDirectoryUser(adUsername, uPW, adRole, user);
		if (he != HawkeyeError::eSuccess)
		{
			// the 'SetupExistingActiveDirectoryUser' method generates an audit log entry for a failure.  No need to do it here as well.
			Logger::L().Log(MODULENAME, severity_level::error, std::string("LoginUser: <exit, Failed to validate user \"" + adUsername + "\". User setup fault.>"));
			return he;
		}

		WriteUserToDatabase(*user);
		currentUser = user;
		AuditLogger::L().Log(generateAuditWriteData(
			"",
			audit_event_type::evt_login,
			"Success: " + adUsername));

		return HawkeyeError::eSuccess;
	}

	auto EU = SetupNewActiveDirectoryUser(adUsername, uPW, adRole);

	// This adds the AD user to the userList
	he = WriteUserToDatabase(EU);
	if (he != HawkeyeError::eSuccess)
	{
		// If the AD user cannot be written to the DB, then the login fails.
		Logger::L().Log(MODULENAME, severity_level::error, std::string("LoginUser: <exit, Failed to validate user \"" + adUsername + "\". Storage fault.>"));
		auditErrorStr.append(" Storage fault.");
		AuditLogger::L().Log(generateAuditWriteData(
			"",
			audit_event_type::evt_loginfailure,
			auditErrorStr));
		return HawkeyeError::eValidationFailed;
	}

	AuditLogger::L().Log(generateAuditWriteData(
		"",
		audit_event_type::evt_login,
		"Success: " + uName));

	for (std::list<ExpandedUser>::iterator user_it = userList.begin(); user_it != userList.end(); ++user_it)
	{
		if (user_it->userCore.UserName() == EU.userCore.UserName())
		{
			currentUser = user_it;
			currentUser->lastLogin = ChronoUtilities::CurrentTime();
			WriteUserToDatabase(EU);
			return HawkeyeError::eSuccess;
		}
	}

	return HawkeyeError::eNoneFound;

}

// **************************************************************************
ExpandedUser UserList::create_service_user()
{
	// Password is arbitrary; it needs to be checked against an programatically generated HMAC key.
	// Service teams want 1 year passwords.
	ExpandedUser EU (SERVICE_USER, "bci_12345", SERVICE_USER_DN, eService);
	EU.allowFastMode = true;
	EU.userCore.SetActivation (true);
	return EU;
}

// **************************************************************************
ExpandedUser UserList::create_silent_administrator()
{
	// Password is arbitrary; it needs to be checked against a programatically generated HMAC key.
	// This one should be on SECONDS with very low clock tolerance.
	ExpandedUser EU (SILENTADMIN_USER, "bci_12345", SILENTADMIN_USER_DN, eAdministrator);
	EU.allowFastMode = true;
	EU.userCore.SetActivation (true);
	return EU;
}

// **************************************************************************
ExpandedUser UserList::create_automation_user()
{
	//if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		// Password is arbitrary; it needs to be checked against an programatically generated HMAC key.
		// Service teams want 1 year passwords.
		ExpandedUser EU (AUTOMATION_USER, "bci_12345", AUTOMATION_USER_DN, eAdministrator);
		EU.allowFastMode = true;
		EU.userCore.SetActivation (true);
		return EU;
	}
}

// **************************************************************************
HawkeyeError UserList::LogoutUser()
{
	currentUser = userList.end();
	g_currentSysErrorUsername = "";

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::LoginRemoteUser(const std::string& username, const std::string& password)
{
	// Count the number of entries this remote user has
	int count = RemoteUserLoginCount(username);

	auto ret = ValidateUserCredentials(username, password);
	if (ret != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			std::string("LoginRemoteUser: <exit, Failed to validate user \"" + username + "\">"));

		std::string auditErrorStr = std::string("Failed validation attempt for user \"" + username + "\".");
		AuditLogger::L().Log(generateAuditWriteData(username, audit_event_type::evt_loginfailure, auditErrorStr));
		return HawkeyeError::eValidationFailed;
	}

	m_currentRemoteUsers[username] = ++count;
	g_currentSysErrorUsername = username;
	
	AuditLogger::L().Log(generateAuditWriteData(
		username,
		audit_event_type::evt_login,
		std::string("Remote login - " + username + " now has - " + std::to_string(count) + " active remote login(s)")));
	return HawkeyeError::eSuccess;
}

// **************************************************************************
void UserList::LogoutRemoteUser(const std::string& username)
{
	// Count the number of entries this remote user has
	int count = RemoteUserLoginCount(username);
	auto remote = m_currentRemoteUsers.find(username);

	if (remote == m_currentRemoteUsers.end())
	{
		AuditLogger::L().Log(generateAuditWriteData(username, audit_event_type::evt_logout,
		                     std::string("Remote logout - " + username + " not found in list")));
		return;
	}

	remote->second--;

	if (remote->second == 0)
	{
		m_currentRemoteUsers.erase(remote);
		AuditLogger::L().Log(generateAuditWriteData(username, audit_event_type::evt_logout,
		                     std::string("Remote logout - " + username + " removed from list")));
	}
	else
	{
		AuditLogger::L().Log(generateAuditWriteData(username, audit_event_type::evt_logout,
		                     std::string("Remote logout - " + username + " still has - " + std::to_string(remote->second) + " active remote login(s)")));
	}


	//TODO: replace this with Attribuable users
	// I'm not sure what the precedence should be or the exact use cases 
	// But if we have any logged in user, use that name. 
	if (UserList::Instance().IsConsoleUserLoggedIn())
	{
		g_currentSysErrorUsername = UserList::Instance().GetConsoleUsername();
	}
	else
	{
		if (!m_currentRemoteUsers.empty())
			g_currentSysErrorUsername = m_currentRemoteUsers.begin()->first;
		else
			g_currentSysErrorUsername = "";
	}
	return;
}

/**
 * **************************************************************************
 * \brief Attempt to validate the given user credentials.
 * \param[in] username credential name to validate
 * \param[in] password credential password to validate
 * \return eSuccess if the given credentials are valid
 * \return eNotPermittedAtThisTime if the given user is found and validated, but the user is currently disabled
 * \return eValidationFailed if the given credentials were not valid
 * \return eNoneFound the given user wasn't found - local security mode only
 * \return eEntryInvalid & eSoftwareFault for other errors
*/
HawkeyeError UserList::ValidateUserCredentials(const std::string& username, const std::string& password)
{
	if (username.empty() || password.empty())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ValidateUserCredentials : <exit, invalid args>");
		return HawkeyeError::eInvalidArgs;
	}

	HawkeyeError he = UserList::Instance().LoginUser(username, password, true);
	// No logging required here...  lower level code does plenty of logging.

	return he;
}

/**
 ***************************************************************************
 * \brief Using the given admin credentials, enable a user. 
 *        The admin might already be in the list of users OR we might need to
 *        validate the given admin using the current AD server / configuration.  
 *        The user we are unlocking has already logged in before being locked out 
 *        so they are already a known user. 
 *
 * \param admin_uname the username of an admin in the current security mode
 * \param admin_pwd the password for the given admin user
 * \param uName the username of the account to unlock
 *
 * \return eSuccess if the given user was unlocked and the user record updated in persistent storage (DB)
 * \return eEntryNotFound if the given user to unlock was not found
 * \return eNotSupported if the given user is the service user (can't enable the service user)
 * \return eNotPermittedByUser if the given admin credentials are valid but the user is not an admin/service user
 * \return eNotPermittedAtThisTime if the given admin credentials are valid but they are locked themselves
 * \return eSoftwareFault if the write of the enabled user account fails
 */
HawkeyeError UserList::AdministrativeEnable (const std::string& admin_uname, const std::string& admin_pwd, const std::string& uName)
{
	// Cannot enable nor disable the service user
	if (uName == SERVICE_USER)
		return HawkeyeError::eNotSupported;

	// Find target user, they must be in the current list.
	std::list<ExpandedUser>::iterator user = userList.end();
	for (user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() == uName)
			break;	
	}

	// User name not found?
	if (user == userList.end())
		return HawkeyeError::eEntryNotFound;

	UserPermissionLevel permissionLevel;
	UserPermissionLevel expectedRole = UserPermissionLevel::eAdministrator;
	if (admin_uname == SERVICE_USER)
		expectedRole = UserPermissionLevel::eService;

	HawkeyeError he = LoginUser(admin_uname, admin_pwd, true);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}
	
	he = GetUserRole (uName, permissionLevel);	
	if ((he != HawkeyeError::eSuccess) || (expectedRole != permissionLevel))
	{
		if ((he == HawkeyeError::eNotPermittedByUser) || (he == HawkeyeError::eNotPermittedAtThisTime))
			return he;
		return HawkeyeError::eValidationFailed;
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
std::string UserList::GenerateHostPassword(const std::string& keyval)
{
	return SecurityHelpers::GenerateHMACPasscode(ChronoUtilities::CurrentTime(), 0, SecurityHelpers::HMAC_SECS, keyval);
}

// **************************************************************************
HawkeyeError UserList::AddUser (ExpandedUser& EU, bool isImported)
{
	if (!isImported)
	{
		// Check for duplicate user in existing user list.
		for (auto user : userList)
		{
			std::string strA = user.userCore.UserName();
			boost::algorithm::to_lower(strA);
			std::string strB = EU.userCore.UserName();
			boost::algorithm::to_lower(strB);
			if (strA == strB || user.displayName == EU.displayName)
				return HawkeyeError::eAlreadyExists;
		}
		EU.allowFastMode = true;
	}

	// Local Security mode usernames are stored as input in the database.
	// Force A/D usernames to lowercase for storing in the database.
	if (EU.ADUser)
	{
		EU.userCore.UsernameToLowerCase();
	}
	
//NOTE: Check Username / PW against TBD criteria - this is done in the UI now.

	HawkeyeError he = WriteUserToDatabase (EU, isImported);
	if (he == HawkeyeError::eSuccess)
	{
		if (EU.ADUser)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetConsoleUsername(),
				audit_event_type::evt_useradd,
				"Added AD User\n\tName: \"" + std::string(EU.userCore.UserName()) +
					"\"\n\tDisplay name \"" + std::string(EU.displayName) +
					"\"\n\tLevel: " + getPermissionLevelAsStr(EU.userCore.PermisionLevel())));
		}
		else
		{
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetConsoleUsername(),
				audit_event_type::evt_useradd,
				"Added User\n\tName: \"" + std::string(EU.userCore.UserName()) +
					"\"\n\tDisplay name \"" + std::string(EU.displayName) +
					"\"\n\tLevel: " + getPermissionLevelAsStr(EU.userCore.PermisionLevel())));
		}

		return he;
	}

	return HawkeyeError::eStorageFault;
}

// **************************************************************************
HawkeyeError UserList::RemoveUser (const std::string& uName)
{
	// Administrator only.  Service shouldn't be altering customer user lists.
	// limited to local logins only
	if (!IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		return HawkeyeError::eNotPermittedByUser;
	}

	// Cannot remove built-in accounts
	if ( BuiltInAccount( uName ) )
	{
		return HawkeyeError::eInvalidArgs;
	}

	// Cannot remove self
	if (uName == UserList::Instance().GetConsoleUsername())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		// If service user or silent admin, find user record amongst all users in database.
		DBApi::eUserType userType = {};
		if (ExpandedUser::IsReservedName(uName))
			userType = DBApi::eUserType::AllUsers;
		else
			userType = (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory) ? DBApi::eUserType::AdUsers : DBApi::eUserType::LocalUsers;

		DBApi::eQueryResult qResult = DBApi::eQueryResult::BadQuery;
		DBApi::DB_UserRecord dbUser = {};
		qResult = DBApi::DbFindUserByName(dbUser, user->userCore.UserName(), userType);
		if (qResult == DBApi::eQueryResult::QueryOk)
		{
			dbUser.Retired = true;
			qResult = DBApi::DbModifyUser(dbUser);
			if (qResult == DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log(MODULENAME, severity_level::normal, "RemoveUser: modified user in datbase");

				// Burn this user's names.
				burnedUsernameList.push_back (BurnedUsers(user->userCore.UserName(), dbUser.UserId));

				// write user to DB before removing it from user list or else the write won't work. 
				WriteUserToDatabase (*user);
				userList.erase (user);
				return HawkeyeError::eSuccess;
			}
		}

		Logger::L().Log(MODULENAME, severity_level::error, "RemoveUser: Unable to find user in datbase");
		return HawkeyeError::eDatabaseError;
	}
	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::EnableUser(const std::string& uName, bool uEnabled)
{
	// Administrator OR Service should be able to enable a user on request. 
	// limited to local logins only
	auto status = CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
		return status;
	
	if ((uEnabled == false) && (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory))
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "EnableUser (disable) <exit failed - AD security mode - not allowed");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (BuiltInAccount(uName) && !uEnabled) // cannot disable the built in accounts
		return HawkeyeError::eInvalidArgs;

	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "EnableUser: Invalid user");
			return HawkeyeError::eValidationFailed;
		}

		// Prevent a user from disabling themselves
		if ((uName == loggedInUsername) && !uEnabled)
			return HawkeyeError::eNotPermittedAtThisTime;

		bool enChanged = user->userCore.IsActive() != uEnabled;
		if (!user->userCore.SetActivation(uEnabled))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "EnableUser: SetActivation failed");
			return HawkeyeError::eValidationFailed;
		}

		// If we are changing anything, then add an audit log entry
		bool changed = (enChanged || (user->attemptCount > 0));

		// Reset the attempted login count - regardles of enable / disable
		// When enabling a user their attempt count should start at 0
		// If we are disabling a user we don't want the lockout timer to re-enable them if they try to login		
		user->attemptCount = 0;

		if (changed)
		{
			if (WriteUserToDatabase(*user) != HawkeyeError::eSuccess)
			{
				Logger::L().Log(MODULENAME, severity_level::error, "EnableUser: WriteUserToDatabase failed");
				return HawkeyeError::eStorageFault;
			}

			audit_event_type eventType = uEnabled ? audit_event_type::evt_userenable : audit_event_type::evt_userdisable;
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				eventType, 
				boost::str(boost::format("%s user \"%s\"") % (uEnabled ? "Enabled " : "Disabled ") % uName)));
		}
		return HawkeyeError::eSuccess;
	}
	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::ChangeUserPassword(const std::string& uName, const std::string& password)
{	
	if (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "ChangeUserPassword <exit failed - AD security mode - not allowed");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	// Administrator / Service only (service should be able to reset a password upon request)
	// limited to local logins only
	auto status = CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
	{
		return status;
	}

	if (BuiltInAccount(uName))
		return HawkeyeError::eInvalidArgs;
		
	std::string loggedInUsername = GetConsoleUsername();

	// If you want to change the PW of the current user, use ChangeCurrentUserPassword(...)
	if ( uName == loggedInUsername)
		return HawkeyeError::eNotPermittedAtThisTime;

	// New password cannot be empty
	if (password.empty())
		return HawkeyeError::eInvalidArgs;

	// Since this is a password reset by  another admin/service account, so set the password change date property to empty string
	// This will force user to change the password on login

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid() || !user->userCore.SetPassword(password))
		{
			// TODO: Log user password reset by admin failure
			return HawkeyeError::eValidationFailed;
		}

		// Reset to change date to the begining of time so that they must change pwd on next login
		user->pwdChangeDate = {};

		// Add a second to the epoch to get the DB to store the change. 
		user->pwdChangeDate += static_cast<std::chrono::seconds>(1);
		user->attemptCount = 0;

		if (HawkeyeError::eSuccess == WriteUserToDatabase (*user))
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_passwordchange,
				boost::str (boost::format ("Changed user \"%s\" password") % uName)));
			return HawkeyeError::eSuccess;
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ChangeUserPassword: failed to save password to DB");
			return HawkeyeError::eValidationFailed;
		}
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::ChangeUserPermissions(const std::string& uName, UserPermissionLevel permissions)
{
	// Administrator only
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		return HawkeyeError::eNotPermittedByUser;
	}

	if (BuiltInAccount(uName))
		return HawkeyeError::eInvalidArgs;

	std::string loggedInUsername = GetConsoleUsername();

	// Cannot change the permissions of the current user
	if ( uName == loggedInUsername)
		return HawkeyeError::eNotPermittedAtThisTime;

	// Cannot promote a user to "service"
	if (permissions == UserPermissionLevel::eService) {
		return HawkeyeError::eValidationFailed;
	}

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;
		if (!user->userCore.IsValid() || !user->userCore.SetUserLevel(permissions))
		{
			// TODO: Log user permission update failure
			return HawkeyeError::eValidationFailed;
		}

		if (HawkeyeError::eSuccess == WriteUserToDatabase (*user))
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_passwordchange,
				boost::str (boost::format ("Changed user \"%s\" permissions") % uName)));
			return HawkeyeError::eSuccess;
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ChangeUserPermissions: failed to save user permissions to DB");
			return HawkeyeError::eValidationFailed;
		}

		return WriteUserToDatabase (*user);
	}
	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserFolder(const std::string& uName, std::string& folder)
{
	// Administrator
	auto status = CheckPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
		return status;
		//TODO: Log invalid user folder name access

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "GetUserFolder: failed to retrieve folder name for invalid account: " + uName);
			return HawkeyeError::eValidationFailed;
		}

		folder = user->exportFolder;

		return HawkeyeError::eSuccess;
	}
	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetUserFolder(const std::string& uName, const std::string& folder)
{
	// Administrator
	//TODO: Log invalid user folder change request (wrong permissions)
	// limited to local logins only
	auto status = CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
		return status;

	if (uName == SERVICE_USER)
		return HawkeyeError::eInvalidArgs;

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "GetUserFolder: failed to set folder name for invalid account: " + uName);
			return HawkeyeError::eValidationFailed;
		}

		//TODO: Validate folder path existence (or non-existence - don't want duplicate paths(?)
		//TODO: Validate folder path acceptability (probably want only relative paths from a common DATA direcotory
		//      and make sure there's no sneaky "../../../" stuff going on!
		//TODO: Make sure no other user is using this folder.
		//TODO: Validate permissions on folder - application must be able to read/write to the folder
		//

		user->exportFolder = folder;

		return WriteUserToDatabase (*user);
	}
	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetUserDisplayName (const std::string& uName, const std::string& dName)
{
	// Administrator only
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		return HawkeyeError::eNotPermittedByUser;
	}

	if (BuiltInAccount(uName))
		return HawkeyeError::eInvalidArgs;

	std::string sanitizedName = boost::algorithm::trim_copy(dName);
	if (sanitizedName.empty())
		return HawkeyeError::eInvalidArgs;

	// Check for duplicates
	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->displayName == sanitizedName)
			return HawkeyeError::eAlreadyExists;
	}

	for (auto burnt : burnedUsernameList)
	{
		if (burnt.username == sanitizedName)
			return HawkeyeError::eAlreadyExists;
	}

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "SetUserDisplayName: failed to set display name for invalid account: " + uName);
			return HawkeyeError::eValidationFailed;
		}

		user->displayName = sanitizedName;
		
		//TODO: Log user display name update
		return WriteUserToDatabase (*user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserDisplayName(const std::string& uName, std::string& dName)
{
	// Non-privileged information ?

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "GetUserDisplayName: failed to get display name for invalid account: " + uName);
			return HawkeyeError::eValidationFailed;
		}

		dName = user->displayName;

		return HawkeyeError::eSuccess;
	}
	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserPermissionLevel(const std::string& uName, UserPermissionLevel& permissions)
{
	// Non-privileged information ?

	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (user->userCore.UserName() != uName)
			continue;

		if (!user->userCore.IsValid())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "GetUserPermissionLevel: failed to get permission level for invalid account: " + uName);
			return HawkeyeError::eValidationFailed;
		}

		permissions = user->userCore.PermisionLevel();

		return HawkeyeError::eSuccess;
	}
	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
std::vector<std::string> UserList::GetUserNames(bool onlyEnabled, bool onlyValid)
{
	std::vector<std::string> userNames;
	for (std::list<ExpandedUser>::iterator user = userList.begin(); user != userList.end(); ++user)
	{
		if (onlyValid && !user->userCore.IsValid())
			continue;

		if (onlyEnabled && !user->userCore.IsActive())
			continue;

		if (user != serviceUser &&
			user != silentAdministrator)
			userNames.push_back(user->userCore.UserName());
	}
		
	return userNames;
}

// **************************************************************************
bool UserList::IsConsoleUserLoggedIn() const
{
	return userList.end() != currentUser;
}

// **************************************************************************
std::string UserList::GetLoggedInUsername()
{
	return GetConsoleUsername();
}

// **************************************************************************
std::string UserList::GetConsoleUsername()
{
	if (IsConsoleUserLoggedIn())
		return currentUser->userCore.UserName();
	return "";
}

// **************************************************************************
std::string UserList::GetRemoteUsername()
{
	// ISSUE: it is possible to have multiple remote users connected.  
	//        For CHM this is /unlikely/, but there's no easy way to tell WHICH
	//        remote user is trying to do something that we're interessted in 
	//        knowing about.
	
	return (m_currentRemoteUsers.empty() ? "" : m_currentRemoteUsers.begin()->first);
}

// **************************************************************************
std::string UserList::GetAttributableUserName()
{
	// This function returns the most likely user to blame/attribute to an audit event or error.
	// If we are automation locked, then the remote user (problem: can have multiple!) should be attributable
	// If we are unlocked, then the local user should be attributable.
	
	if (g_currentSystemAutomationLockstatus)
	{
		return GetRemoteUsername();
	}
	else
	{
		return GetConsoleUsername();
	}


}


// **************************************************************************
HawkeyeError UserList::GetLoggedInUserDisplayName(std::string& uDName)
{
	if (!IsConsoleUserLoggedIn())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	uDName = currentUser->displayName;
	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::GetLoggedInUserPermissions(UserPermissionLevel& permissions)
{
	if (!IsConsoleUserLoggedIn())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	permissions = currentUser->userCore.PermisionLevel();
	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::GetLoggedInUserFolder(std::string& folder)
{
	if (!IsConsoleUserLoggedIn())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	folder = currentUser->exportFolder;
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// This API is not exposed through HawkeyeLogic.
//*****************************************************************************
HawkeyeError UserList::GetUserUUIDByName(const std::string& uName, uuid__t& uuid)
{
	std::string tstName = uName;
	if (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory)
		boost::algorithm::to_lower(tstName);

	for (const auto& v : userList)
	{
		if (v.userCore.UserName() == tstName) {
			if (!v.userCore.IsValid()) {
				return HawkeyeError::eEntryInvalid;
			}
			uuid = v.uuid;
			return HawkeyeError::eSuccess;
		}
	}

	// Check the retired users.
	for (const auto& v : burnedUsernameList)
	{
		if (v.username == tstName)
		{
			uuid = v.uuid;
			return HawkeyeError::eSuccess;
		}
	}
	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
// This API is not exposed through HawkeyeLogic.
//*****************************************************************************
HawkeyeError UserList::GetUserByName(const std::string& uName, ExpandedUser **user)
{
	for (auto& v : userList)
	{
		if (v.userCore.UserName() == uName) {
			if (!v.userCore.IsValid()) {
				return HawkeyeError::eEntryInvalid;
			}
			*user = &v;
			return HawkeyeError::eSuccess;
		}
	}

	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
// This API is not exposed through HawkeyeLogic.
//*****************************************************************************
HawkeyeError UserList::GetUsernameByUUID (const uuid__t& uuid, std::string& uName)
{
	for (const auto& v : s_dbUserList)
	{
		if (Uuid(v.UserId) == Uuid(uuid)) {
			uName = v.UserNameStr;
			return HawkeyeError::eSuccess;
		}
	}
	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::GetUserPermissions (const std::string& uName, UserPermissionLevel& permissions)
{
	for (auto user : userList) {
		if (user.userCore.UserName() == uName) {
			if (!user.userCore.IsValid()) {
				return HawkeyeError::eEntryInvalid;
			}
			permissions = user.userCore.PermisionLevel();

			return HawkeyeError::eSuccess;
		}
	}

	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::ValidateConsoleUser(const std::string& uPW)
{
	if (!IsConsoleUserLoggedIn())
		return HawkeyeError::eNotPermittedAtThisTime;

	// A bad or disabled user cannot be validated
	if (!currentUser->IsValid() || !currentUser->userCore.IsActive())
		return HawkeyeError::eValidationFailed;

	// Check if current user is service user, then validate password 
	// using "ValidateHMACPasscode()" method
	if (currentUser == serviceUser)
	{
		if (ValidateServicePassword(uPW))
			return HawkeyeError::eSuccess;
	}
	else if (currentUser == silentAdministrator)
	{
		if (ValidateHostPassword(uPW))
			return HawkeyeError::eSuccess;
	}
	else if (currentUser == automationUser)
	{ // Automation user uses the same password authentication mechanism as BCI service user.
		if (ValidateServicePassword(uPW))
			return HawkeyeError::eSuccess;
	}
	else
	{
		if (currentUser->userCore.ValidatePassword(uPW))
			return HawkeyeError::eSuccess;
	}
	
	return HawkeyeError::eValidationFailed;
}

// ********************************************************************
/**
 * \brief Get the role for a given AD user. The given name & password are used to
 *        log into the configured AD server and get a list of groups the
 *        user belongs to. We then compare these to the Role/Group maps.
 *
 * \param[in] uName is the uName of the user to validate to the server
 * \param[in] uPW the password to validate against the server
 * \param[out] fndRole - if eSeccess is returned this value contains the role for the given user
 * \param[out] fndEmail - if eSeccess is returned this value MIGHT contain an email address for the given user or it may be empty
 *
 * \return eSuccess if the given user credentials were valid and a role was found for this user
 * \return eSoftwareFault if we fail to get the AD configurations
 * \return eValidationFailed if the given credentials were NOT valid OR the user does not belong to a group in the group to role mapping
 */
HawkeyeError UserList::LoginADUser (const std::string& uName, const std::string& uPW, UserPermissionLevel& fndRole, std::string& fndEmail)
{
	Logger::L().Log( MODULENAME, severity_level::debug1, "LoginADUser: <enter>" );

	std::vector<ActiveDirectoryGroupDLL> adRoleMaps = {};
	auto status = GetActiveDirectoryGroupMaps(adRoleMaps); // This function call allocates memory - be sure to delete the object
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("LoginADUser: <exit, GetActiveDirectoryGroupMaps, status: %ld>") % (int32_t)status));
		return HawkeyeError::eSoftwareFault;
	}

	ActiveDirectoryConfigDLL adConfig = {};
	status = GetActiveDirectoryConfig (adConfig); // This function call allocates memory - be sure to delete the object
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("LoginADUser: <exit, GetActiveDirConfig, status: %ld>") % (int32_t)status));
		return status;
	}

	// This is where the attempt is made to verify the user credentials with the AD server
	// The returned array of strings contains, among other things, the groups the user belongs to.
	// We use this to determine the user's Role (permissions) 

	std::vector<std::string> adGroups = {};
	ValidateADUser ( adGroups, uName, uPW, adConfig );

	// The method below works because of the ordering of the UserPermissionLevel enum
	// This is the current ordering in terms of integer values: User < Elevated < Admin 
	// If that order changes this code will need to be modified.
	//
	// We start with an invalid value less than the lowest valid value (User = 0).
	int myRole = -1;
	for (auto& roleMap : adRoleMaps)
	{
		if (myRole >= static_cast<int>(roleMap.role))
			continue; // We already have at least this role

		std::string currGrp = roleMap.group;
		for (const auto& group : adGroups)
		{
			// @todo - v1.4 - Add returning of the email address
			//if (IsEmailValid(group))
			//{
			//	fndEmail = group;
			//}
			if (group._Equal(currGrp))
			{
				// We found a match between the group maps and one of the user's groups
				// Store the role for this map
				myRole = static_cast<int>(roleMap.role);
				break;
			}
		}
		
		// Stop once we've got the highest role allowed
		if (myRole >= UserPermissionLevel::eAdministrator)
			break;
	}

	if ((myRole < UserPermissionLevel::eNormal) || (myRole > UserPermissionLevel::eAdministrator))
	{
		// Login failed or we could not match the role
		return HawkeyeError::eValidationFailed;
	}

	fndRole = static_cast<UserPermissionLevel>(myRole);
	
	Logger::L().Log( MODULENAME, severity_level::debug1, "LoginADUser: <exit>" );

	return HawkeyeError::eSuccess;
}

// ********************************************************************
void UserList::ValidateADUser (std::vector<std::string>& adgrouplist, const std::string uname, const std::string upw, const ActiveDirectoryConfigDLL adcfg)
{
	// This is where the attempt is made to verify the user credentials with the AD server
	// The returned array of strings contains, among other things, the groups the user belongs to.
	// We use this to determine the user's Role (permissions) 

	Logger::L().Log( MODULENAME, severity_level::debug1, "LoginAdUser: <enter>" );

	bool tlsFlag = false;
	bool unsafeFlag = true;

	switch ( adcfg.port )
	{
		case 389:		// 389 is treated as 'unsafe' with no TLS
		case 3268:		// 3268 is treated as 'unsafe' with no TLS
			tlsFlag = false;		// NOTE that setting 'tls' to true will force port to 389!
			unsafeFlag = true;
			break;

		case 636:		// port 636 is treated as 'safe' without TLS
		case 3269:		// port 3269 is treated as 'safe' without TLS
			tlsFlag = false;		// NOTE that setting 'tls' to true will force port to 389!
			unsafeFlag = false;
			break;
	}

	std::string logStr = "LoginAdUser:\r\n\tdomain: ";
	if ( adcfg.domain.length() > 0 )
	{
		logStr.append( adcfg.domain );
	}
	else
	{
		logStr.append( "empty" );
	}

	logStr.append( "\n\tserver: " );
	if ( adcfg.server.length() > 0 )
	{
		logStr.append( adcfg.server );
	}
	else
	{
		logStr.append( "empty" );
	}
	logStr.append( "\n\tusername: " );
	if ( uname.length() > 0 )
	{
		logStr.append( uname );
	}
	else
	{
		logStr.append( "empty" );
	}

	logStr.append( boost::str( boost::format( "\r\n\tTLS flag: %s\r\n\tunsafe flag: %s" ) % ( ( tlsFlag ) ? "true" : "false" ) % ( ( unsafeFlag ) ? "true" : "false" ) ) );
	Logger::L().Log( MODULENAME, severity_level::debug1, logStr );

	std::vector<std::string> adGroups = ActiveDirectoryGroupDLL::Get( adcfg.domain, adcfg.server, uname, upw, tlsFlag, unsafeFlag );

#ifdef _DEBUG
	std::string configStr = "LoginAdUser: ActiveDirectoryGroupDLL::Get returned:\r\n\t";
#endif
	std::string grpStr = "";
	size_t vecSize = adGroups.size();
	for ( size_t x = 0; x < vecSize; x++ )
	{
		grpStr = adGroups[x];
		adgrouplist.push_back( grpStr );
#ifdef _DEBUG
		configStr.append( grpStr );
		if ( ( x + 1 ) < vecSize )
		{
			configStr.append( "\r\n\t" );
		}
#endif
	}
#ifdef _DEBUG
	Logger::L().Log( MODULENAME, severity_level::debug1, configStr );
#endif

	Logger::L().Log( MODULENAME, severity_level::debug1, "LoginAdUser: <exit>" );
}

/**
 ****************************************************************************
 * \brief Validate that the given credentials are valid for a local admin.
 *        Regardless of the current security mode setting, validate the 
 *        given credentials against the 'local' users. 
 *
 * \param uname is the username of the local admin
 * \param password is the password for the local admin
 *
 * \return eSuccess if the given credentials are valid for a local admin and that account is currently enabled
 * \return eStorageFault if the user is not found, or eValidationFailed if the user is invalid.
 */
HawkeyeError UserList::ValidateLocalAdminAccount (const std::string& uname, const std::string& password)
{
	DBApi::eQueryResult qResult = DBApi::eQueryResult::BadQuery;
	DBApi::DB_UserRecord dbUser = {};
	
	qResult = DBApi::DbFindUserByName (dbUser, uname, DBApi::LocalUsers);
	if (qResult == DBApi::eQueryResult::QueryOk)
	{
		ExpandedUser EU;
		EU.FromDbStyle (dbUser);

		if (!EU.userCore.FromStorageString(dbUser.AuthenticatorList[0]))
			return HawkeyeError::eSoftwareFault;

		if (!EU.userCore.ValidatePassword(password))
		{
			return HawkeyeError::eValidationFailed;
		}

		// We have a valid username and password
		if (dbUser.Retired)
			return HawkeyeError::eDeprecated;

		if (memcmp((void*)dbUser.RoleId.u, (void*)adminRole.RoleId.u, sizeof(uuid__t)) != 0)
			return HawkeyeError::eNotPermittedByUser; // Not an admin

		if (!EU.userCore.IsActive())
			return HawkeyeError::eNotPermittedAtThisTime;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eNoneFound;
}

// **************************************************************************
bool UserList::ValidateHostPassword(const std::string& pw)
{
	// Minimal grace on this one.  We will give ONE WHOLE SECOND just in case 
	// the system clock rolled over between the calls.
	return SecurityHelpers::ValidateHMACPasscode(pw, SecurityHelpers::HMAC_SECS, 2, 1);
}

// **************************************************************************
bool UserList::ValidateServicePassword(const std::string& pw)
{
	// Service password is in yearly chunks.
	return SecurityHelpers::ValidateHMACPasscode(pw, SecurityHelpers::HMAC_YEARS);
}

// **************************************************************************
HawkeyeError UserList::ChangeCurrentUserPassword (const std::string& currentPW, const std::string& newPW)
{
	auto status = ValidateConsoleUser(currentPW);
	if (status != HawkeyeError::eSuccess)
	{
		//TODO: Log password change validation failure
		return status;
	}

	if ( !IsConsoleUserLoggedIn() )
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	// Not for built-in accounts.
	if ( BuiltInAccount( currentUser->userCore.UserName() ) )
		return HawkeyeError::eInvalidArgs;

	// Don't allow AD user to change password
	if (currentUser->ADUser )
		return HawkeyeError::eNotPermittedByUser;

	if (currentUser->userCore.SetPassword(newPW))
	{
		// TODO: Log password chance success
		// Since this is a password reset by user himself, so set the password change date property to current time as seconds from epoch UTC
		// Create password change date property with empty string
		// Set the current time as seconds from epoch UTC as password change date property value
		currentUser->pwdChangeDate = ChronoUtilities::CurrentTime();
		currentUser->attemptCount = 0;

		return WriteUserToDatabase (*currentUser);
	}
	
	// Log password change failure
	return HawkeyeError::eValidationFailed;
}

// **************************************************************************
HawkeyeError UserList::WriteUserToDatabase (ExpandedUser& user, bool isImported)
{
	HawkeyeError status = HawkeyeError::eSuccess;

	DBApi::eUserType userType;
	if (ExpandedUser::IsReservedName (user.userCore.UserName()))
		userType = DBApi::eUserType::AllUsers;
	else
		userType = (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory) ? DBApi::eUserType::AdUsers : DBApi::eUserType::LocalUsers;

	bool newDbUser = true;

	DBApi::DB_UserRecord dbUser = {};
	DBApi::eQueryResult qResult = DBApi::eQueryResult::BadQuery;
	qResult = DBApi::DbFindUserByName (dbUser, user.userCore.UserName(), userType);
	if (qResult == DBApi::eQueryResult::QueryOk)
	{
		newDbUser = false;
		user.uuid = dbUser.UserId;
	}

	dbUser = user.ToDbStyle();

	UserPermissionLevel permission = user.userCore.PermisionLevel();
	switch (permission)
	{
		case UserPermissionLevel::eNormal:
			dbUser.RoleId = normalRole.RoleId;
			break;
		case UserPermissionLevel::eElevated:
			dbUser.RoleId = elevatedRole.RoleId;
			break;
		case UserPermissionLevel::eAdministrator:
		case UserPermissionLevel::eService:
			dbUser.RoleId = adminRole.RoleId;
			permission = UserPermissionLevel::eAdministrator;
			break;
		default:
			return HawkeyeError::eInvalidArgs;
	}

	// hash values should be calculated using the standard hash technique and stored as strings; here I'm just storing the permissions values as un-hashed strings
	// @TODO AppPermissions aren't currently used, we need to figure out how to do this - change this code
	// User records have both the permission bit - field and the permission hash field while the role record has only 
	// the default permission bits.Don't assign the permission bits to the hash field... assign like-to-like fields; 
	// permission bits to permission bits and generate the has values from the stored permission bits, i.e.
	// user.ApplicationPermissions = role.ApplicationPermissions;
	// usr.ApplicationPermisisionHash = hashValue(user.ApplicationPermissions);
	//User.AppPermissionsHash = boost::str(boost::format("%lu") % (rolesList.at(permissions).ApplicationPermissions));
	//User.InstPermissionsHash = boost::str(boost::format("%lu") % (rolesList.at(permissions).InstrumentPermissions));	
	dbUser.AppPermissions = dbRolesList.at(permission).ApplicationPermissions;

	dbUser.AuthenticatorList.clear();
	dbUser.AuthenticatorList.push_back (user.userCore.ToStorageString());
		
	// A number of the following fields need to be supplied and are not, so a default is written.
	if (newDbUser)
	{
		if (!isImported)
		{
			dbUser.DefaultSampleNameStr = "Sample";
			dbUser.UserImageSaveN = 1;
			dbUser.DefaultResultFileNameStr = "Summary";
			dbUser.CSVFolderStr = HawkeyeDirectory::Instance().getExportDir() + "\\CSV\\" + dbUser.UserNameStr;
			dbUser.PdfExport = true;
			dbUser.WashType = 0;
			dbUser.Dilution = 1;
		}

		auto found_it = std::find_if(burnedUsernameList.begin(), burnedUsernameList.end(),
			[dbUser](const BurnedUsers& item)->bool
			{
				return dbUser.UserNameStr == item.username;
			});

		if (found_it != burnedUsernameList.end())
			dbUser.Retired = true;
		else
			dbUser.Retired = false;

		qResult = DbAddUser (dbUser);
		if (qResult == DBApi::eQueryResult::QueryOk)
		{
			// The DB sets the UUID so, after adding the user 
			// we MUST update the UUID in the UserList object it is used to find the user record again 		
			user.uuid = dbUser.UserId;
			userList.push_back(user);
			s_dbUserList.push_back(dbUser);
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "WriteUserToDatabase: DbAddUser failed");
		}
	}
	else
	{
		qResult = DbModifyUser (dbUser);
		if (qResult == DBApi::eQueryResult::QueryOk)
		{
			for (int j = 0; j < s_dbUserList.size(); j++)
			{
				if (Uuid(s_dbUserList[j].UserId) == Uuid(dbUser.UserId))
				{
					s_dbUserList[j] = dbUser;
					break;
				}
			}
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "WriteUserToDatabase: DbModifyUser failed");
		}
	}

	if (qResult != DBApi::eQueryResult::QueryOk)
		status = HawkeyeError::eStorageFault;

	return status;
}

// **************************************************************************
std::string getSignatureHash (std::string sigData)
{
	std::vector<unsigned char> message(sigData.begin(), sigData.end());
	std::string salt = "This Is The Smokey Salt";
	std::vector<unsigned char> saltB(salt.begin(), salt.end());
	return SecurityHelpers::CalculateHash(message, saltB, 5);
}

// **************************************************************************
std::pair<std::string, boost::property_tree::ptree> UserList::Export()
{
	boost::property_tree::ptree pt_UserList = {};
	boost::property_tree::ptree pt_BurnedUsers = {};

	std::string sigData = "";
	for (auto b : burnedUsernameList)
	{
		sigData += b.username;
		pt_BurnedUsers.add("Burnt", b.username); 
		pt_BurnedUsers.add("BurntUUID", Uuid::ToStr(b.uuid));
	}

	std::string hash = getSignatureHash (sigData);
	pt_BurnedUsers.put("BurntSignature", hash);

	pt_UserList.add_child("BurnedNames", pt_BurnedUsers);

	for (auto user : userList)
	{
		// We will ONLY write the valid users back to disk.
		// We will NEVER write the service user back to disk.
		if (!user.IsValid() || user.userCore.PermisionLevel() == eService)
			continue;

		boost::property_tree::ptree pt_User = {};
		pt_User.put("Core", user.userCore.ToStorageString());
		pt_User.put("EUFolder", user.exportFolder);
		pt_User.put("EUDisplayName", user.displayName);
		pt_User.put("ADUser", user.ADUser);
		pt_User.put("Email", user.email);
		pt_User.put("DisplayDigits", user.displayDigits);
		pt_User.put("Comment", user.comment);
		pt_User.put("AllowFastMode", user.allowFastMode);
		pt_User.put("PasswordChangeDate", ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(user.pwdChangeDate));
		pt_User.put("LastLoginDate", ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(user.lastLogin));
		for (auto dc : user.displayColumns) {
			boost::property_tree::ptree pt_DisplayColumns;
			pt_DisplayColumns.put("ColumnType", dc.ColumnType);
			pt_DisplayColumns.put("OrderIndex", dc.OrderIndex);
			pt_DisplayColumns.put("Width", dc.Width);
			pt_DisplayColumns.put("Visible", dc.Visible);
			pt_User.add_child("DisplayColumns", pt_DisplayColumns);
		}
		boost::property_tree::ptree pt_CellTypeIndices;
		for (auto cti : user.cellTypeIndices) {
			pt_CellTypeIndices.put("Index", cti);
			pt_User.add_child("CellTypeIndices", pt_CellTypeIndices);
		}

		boost::property_tree::ptree pt_AnalysisIndices;
		for (auto ai : user.analysisIndices) {
			pt_AnalysisIndices.put("Index", ai);
			pt_User.add_child("AnalysisIndices", pt_AnalysisIndices);
		}

		pt_UserList.add_child("User", pt_User);
	}

	boost::property_tree::ptree pt_Roles;

	for (const auto& role : dbRolesList)
	{
		boost::property_tree::ptree pt_Role;
		pt_Role.put ("UUID", Uuid::ToStr(role.RoleId));
		pt_Role.put ("Name", role.RoleNameStr);
		pt_Role.put ("Type", role.RoleType);

		boost::property_tree::ptree pt_GroupMapList;
		for (const auto& item : role.GroupMapList)
		{
			boost::property_tree::ptree pt_GroupMap;
			pt_GroupMap.put ("Name", item);
			pt_Role.add_child ("ADGroup", pt_GroupMap);
		}

		boost::property_tree::ptree pt_CellTypeIndexList;
		for (const auto& item : role.CellTypeIndexList)
		{
			boost::property_tree::ptree pt_CellTypeIndex;
			pt_CellTypeIndex.put ("Value", item);
			pt_Role.add_child ("CellTypeIndex", pt_CellTypeIndex);
		}

		pt_Role.put ("InstPermissions", role.InstrumentPermissions);
		pt_Role.put ("AppPermissions", role.ApplicationPermissions);
		pt_Roles.add_child("Role", pt_Role);
	}

	pt_UserList.add_child ("Roles", pt_Roles);

	return std::make_pair (USERLIST_NODE, pt_UserList);
}

//*****************************************************************************
std::vector<std::string> ExtractSetUserPropertyFromStorageString (const std::string& storedProperty)
{
	// Remember that the string will end on a delimiter.
	std::string delimiter = "^?%%??^";

	std::vector<std::string> propValues;

	std::size_t start_pos = 0;
	std::size_t pos = storedProperty.find (delimiter, start_pos);
	while (pos != std::string::npos)
	{
		std::string lab = storedProperty.substr (start_pos, (pos - start_pos));
		propValues.push_back(lab);

		start_pos = pos + delimiter.length();
		pos = storedProperty.find (delimiter, start_pos);
	}

	return propValues;
}

// **************************************************************************
bool UserList::ImportUser (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it)
{
	ExpandedUser EU = {};

	std::string authenticatorList = assoc_it->second.get<std::string>("Core");
	if (!EU.userCore.FromStorageString(authenticatorList))
	{
		// Validation failed on core user data.
		//TODO: log validation failure on this user.
		return true; // Success in that we read it - still invalid.
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "ImportUser: " + EU.userCore.UserName());
	
	// Common tags.
	EU.exportFolder = assoc_it->second.get<std::string>("EUFolder");
	EU.displayName = assoc_it->second.get<std::string>("EUDisplayName");

	auto ppIt = assoc_it->second.find("EUProperty");
	if (ppIt != assoc_it->second.not_found())
	{ // v1.2 tags.
		auto rng = assoc_it->second.equal_range("EUProperty");
		for (auto it = rng.first; it != rng.second; ++it)
		{
			auto propName = it->second.get<std::string>("PropName");

			if (propName == "Analyses")
			{
				auto values = ExtractSetUserPropertyFromStorageString (it->second.get<std::string>("PropValue"));
				for (const auto v : values)
				{
					EU.analysisIndices.push_back(std::strtoul(v.c_str(), nullptr, 0));
				}
			}
			else if (propName == "CellTypes")
			{
				auto values = ExtractSetUserPropertyFromStorageString (it->second.get<std::string>("PropValue"));
				for (const auto v : values)
				{
					EU.cellTypeIndices.push_back(std::strtoul(v.c_str(), nullptr, 0));
				}
			}
			else if (propName == "PasswordChangeDate")
			{
				auto values = ExtractSetUserPropertyFromStorageString (it->second.get<std::string>("PropValue"));
				uint64_t pwChgDate = std::strtoll (values[0].c_str(), nullptr, 0);
				EU.pwdChangeDate = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(pwChgDate);
			}
			else if (propName == "UserDetails")
			{
				auto values = ExtractSetUserPropertyFromStorageString(it->second.get<std::string>("PropValue"));
				EU.comment = values[0].c_str();
			}
		}
		
		EU.allowFastMode = true;
	}
	else
	{ // v1.3 tags.
		EU.ADUser = assoc_it->second.get<bool>("ADUser", false);
		EU.email = assoc_it->second.get<std::string>("Email", "");
		EU.displayDigits = assoc_it->second.get<uint32_t>("DisplayDigits", 2);
		EU.comment = assoc_it->second.get<std::string>("Comment", "");
		EU.allowFastMode = assoc_it->second.get<bool>("AllowFastMode", false);
		auto pwChgDate = assoc_it->second.get<uint64_t>("PasswordChangeDate", 0);
		EU.pwdChangeDate = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(pwChgDate);
		auto lastLoginDate = assoc_it->second.get<uint64_t>("LastLoginDate", 0);
		EU.lastLogin = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(lastLoginDate);

		// Clear the existing settings to avoid duplicates.
		auto colRange = assoc_it->second.equal_range("DisplayColumns");
		for (auto it = colRange.first; it != colRange.second; ++it)
		{
			EU.displayColumns.clear();
			break;
		}

		for (auto it = colRange.first; it != colRange.second; ++it)
		{
			if (it->second.size())
			{
				DBApi::display_column_info_t displayCol;
				displayCol.ColumnType = it->second.get<int32_t>("ColumnType");
				displayCol.OrderIndex = it->second.get<uint16_t>("OrderIndex");
				displayCol.Visible = it->second.get<bool>("Visible");
				displayCol.Width = it->second.get<uint16_t>("Width");
				EU.displayColumns.push_back(displayCol);
			}
		}

		uint32_t analysisIndex;
		auto anr = assoc_it->second.equal_range("AnalysisIndices");
		for (auto it = anr.first; it != anr.second; ++it)
		{
			if (it->second.size())
			{
				analysisIndex = it->second.get<uint32_t>("Index");
				EU.analysisIndices.push_back(analysisIndex);
			}
		}

		uint32_t cellTypeIndex;
		auto ctr = assoc_it->second.equal_range("CellTypeIndices");
		for (auto it = ctr.first; it != ctr.second; ++it)
		{
			if (it->second.size())
			{
				cellTypeIndex = it->second.get<uint32_t>("Index"); 
				EU.cellTypeIndices.push_back(cellTypeIndex);
			}
		}
	}

	// Add any burned names in imported userlist that aren't already there
	for (auto bn : burnedUsernameList)
	{
		// look for burned name in burnedUserNameList
		auto item = std::find_if(burnedUsernameList.begin(), burnedUsernameList.end(),
			[bn](const auto& burnt) {
				return (bn.username == burnt.username);
			});

		if (item != burnedUsernameList.end())
		{
				continue;
		}
		else
		{
			burnedUsernameList.push_back (bn);
		}
	}

	HawkeyeError he = UserList::Instance().AddUser (EU, true);
	if (he == HawkeyeError::eAlreadyExists)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("ImportUsers: duplicate user found %s") % EU.userCore.UserName()));
		return false;
	}

	return true;
}

// **************************************************************************
void UserList::ImportRoles (const boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it)
{
	const auto roleRange = assoc_it->second.equal_range ("Role");

	for (auto it = roleRange.first; it != roleRange.second; ++it)
	{
		DBApi::DB_UserRoleRecord dbRole = {};
		DBApi::eQueryResult dbStatus = DBApi::DbFindUserRoleByName (dbRole, it->second.get<std::string>("Name"));
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("ImportRoles: <exit, DbFindUserRoleByName failed, status: %ld>") % (int32_t)dbStatus));
			return;
		}

		HawkeyeUUID uuid (it->second.get<std::string>("UUID").c_str());
		uuid.get_uuid__t (dbRole.RoleId);
		dbRole.RoleNameStr = it->second.get<std::string>("Name");
		dbRole.RoleType = it->second.get<uint16_t>("Type");

		dbRole.GroupMapList.clear();
		auto adGroupRange = it->second.equal_range ("ADGroup");
		for (auto group_it = adGroupRange.first; group_it != adGroupRange.second; group_it++)
		{
			dbRole.GroupMapList.push_back (group_it->second.get<std::string>("Name"));
		}

		dbRole.CellTypeIndexList.clear();
		auto celltypeRange = it->second.equal_range ("CellTypeIndex");
		for (auto ct_it = celltypeRange.first; ct_it != celltypeRange.second; ct_it++)
		{
			dbRole.CellTypeIndexList.push_back (ct_it->second.get<uint32_t>("Value"));
		}

		dbRole.InstrumentPermissions = it->second.get<uint64_t>("InstPermissions");
		dbRole.ApplicationPermissions = it->second.get<uint64_t>("AppPermissions");

		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			if (DBApi::eQueryResult::NoResults == dbStatus)
			{
				dbStatus = DBApi::DbAddUserRole (dbRole);
				if (DBApi::eQueryResult::QueryOk != dbStatus)
				{
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("Initialize: <exit, DbAddUserRole failed, status: %ld>") % (int32_t)dbStatus));
				}
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("Initialize: <exit, DbFindUserRoleByName failed, status: %ld>") % (int32_t)dbStatus));
			}
			return;
		}

		dbStatus = DBApi::DbModifyUserRole (dbRole);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbModifyUserRole failed, status: %ld>") % (int32_t)dbStatus));
		}
	}
}

// **************************************************************************
void UserList::Import (boost::property_tree::ptree& ptParent)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	UserList::Instance().Initialize (true);

	t_opPTree BN = ptParent.get_child_optional("BurnedNames");
	if (BN)
	{
		// Validate the entries in the list of "burned" users.
		BurnedUsers burnt = {};
		auto brn = (*BN).equal_range("Burnt");
		for (auto it = brn.first; it != brn.second; ++it)
		{
			burnt.username = it->second.data();
			boost::optional<std::string> temp = (*BN).get_optional<std::string>("BurntUUID");
			if (temp)
			{
				burnt.uuid = Uuid::FromStr(temp.get());
			}
			else
			{
				Uuid::Clear (burnt.uuid);
			}

			burnedUsernameList.push_back (burnt);			
		}

		if ((*BN).find("BurntSignature") == (*BN).not_found())
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_configuration_failedvalidation, 
				instrument_error::instrument_configuration_instance::userlist, 
				instrument_error::severity_level::error));
			return;
		}
		else
		{
			std::string signature = (*BN).get<std::string>("BurntSignature");
			std::string sigData = "";
			for (auto b : burnedUsernameList)
			{
				sigData += b.username;
			}

			std::string hash = getSignatureHash (sigData);
			if (hash != signature)
			{
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_configuration_failedvalidation, 
					instrument_error::instrument_configuration_instance::userlist, 
					instrument_error::severity_level::error));
				return;
			}

			for (auto& v : burnedUsernameList)
			{
				ExpandedUser EU (v.username, "", "", UserPermissionLevel::eNormal);
				EU.uuid = v.uuid;
				EU.isRetired = true;
				WriteUserToDatabase (EU, true);
			}
		}
	}

	auto rolesRange = ptParent.equal_range("Roles");
	for (auto it = rolesRange.first; it != rolesRange.second; ++it)
	{
		ImportRoles (it);
	}

	auto usersRange = ptParent.equal_range("User");
	for (auto it = usersRange.first; it != usersRange.second; ++it)
	{
		UserList::Instance().ImportUser (it);
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}

// **************************************************************************
HawkeyeError UserList::CheckUserPermissionAtLeast( ExpandedUser * user, const UserPermissionLevel& required_permission )
{
	//Return codes : eSuccess, eNoneFound(no user logged in), eNotPermittedByUser, eNotPermittedAtThisTime
	if ( !user )
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if ( !user->userCore.IsActive() )
	{
		// When user lock the application (without logging out) and enter wrong password many times
		// User will be disabled but still logged in, so check for disabled user as well

		// User should not be disabled either.
		return HawkeyeError::eNotPermittedByUser;
	}

	UserPermissionLevel userPermission = user->userCore.PermisionLevel();

	// User should have either requested permission or higher.
	// Validation based on enum hierarchy.
	if ( (int) userPermission < (int) required_permission )
	{
		return HawkeyeError::eNotPermittedByUser;
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::CheckPermissionAtLeast(const UserPermissionLevel& required_permission)
{
	//Return codes : eSuccess, eNoneFound(no user logged in), eNotPermittedByUser, eNotPermittedAtThisTime
	if (!IsConsoleUserLoggedIn() && !IsAnyRemoteUserLoggedIn())
	{		
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (IsAnyRemoteUserLoggedIn())
	{
		return HawkeyeError::eSuccess;
	}
	
	if (currentUser != userList.end())
	{
		if (IsUserPermissionAtLeast(currentUser->userCore.UserName(), required_permission))
			return HawkeyeError::eSuccess;
	}
	
	return HawkeyeError::eNotPermittedByUser;
}

// **************************************************************************
bool UserList::IsConsoleUserPermissionAtLeast( const UserPermissionLevel& required_permission )
{
	if (!IsConsoleUserLoggedIn())
		return false;
	if (IsUserPermissionAtLeast(currentUser->userCore.UserName(), required_permission))
		return true;
	return false;
}

// **************************************************************************
bool UserList::IsUserPermissionAtLeast(const std::string& uName, const UserPermissionLevel& required_permission)
{
	UserPermissionLevel myLevel;
	if (GetUserPermissions(uName, myLevel) == HawkeyeError::eSuccess)
	{
		return (myLevel >= required_permission);
	}
	return false;
}

// **************************************************************************
bool UserList::IsConsoleUserPermissionAtLeastAndNotService(const UserPermissionLevel& required_permission)
{
	if (!IsConsoleUserLoggedIn())
		return false;
	if (IsUserPermissionAtLeastAndNotService(currentUser->userCore.UserName(), required_permission))
		return true;
	return false;
}


// **************************************************************************
bool UserList::IsUserPermissionAtLeastAndNotService(const std::string& uName, const UserPermissionLevel& required_permission)
{
	UserPermissionLevel myLevel;
	if (GetUserPermissions(uName, myLevel) == HawkeyeError::eSuccess)
	{
		return ((myLevel >= required_permission) && (myLevel != UserPermissionLevel::eService));
	}
	return false;
}


// **************************************************************************
HawkeyeError UserList::CheckConsoleUserPermissionAtLeast(const UserPermissionLevel& required_permission)
{
	if (!IsConsoleUserLoggedIn())
		return HawkeyeError::eNotPermittedAtThisTime;
	if (IsUserPermissionAtLeast(currentUser->userCore.UserName(), required_permission))
		return HawkeyeError::eSuccess;
	return HawkeyeError::eNotPermittedByUser;
}

// **************************************************************************
HawkeyeError UserList::IsPasswordExpired(const std::string& uName, bool& expired)
{
	if (BuiltInAccount(uName))
	{
		// None of these accounts ever have their passwords expire
		expired = false;
		return HawkeyeError::eSuccess;
	}

	if (GetSystemSecurityType() == eSECURITYTYPE::eActiveDirectory)
	{
		// AD Users passwords are not managed by this system 
		// These passwords should never be reported as expired.
		expired = false;
		return HawkeyeError::eSuccess;
	}

	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;
		if (!user.userCore.IsValid())
		{
			expired = true;
			return HawkeyeError::eNotPermittedAtThisTime;
		}

		auto now = ChronoUtilities::CurrentTime();
		auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - user.pwdChangeDate);
		if (static_cast<uint32_t>(elapsed.count()) < (HawkeyeConfig::Instance().get().passwordExpiration * (MIN_PER_DAY))) // comparing minutes 
		{
			expired = false;
		}
		else
		{
			expired = true;
		}

		return HawkeyeError::eSuccess;
	}

	expired = true;

	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::AddFactoryAdminUser()
{
	// The factory admin account is just an "initial" admin account
	// 
	// Only create local constants to discourage use elsewhere
	std::string cleanName = "factory_admin";
	std::string cleanDisplay = "Factory Admin";

	// Check for duplication in existing user list.
	for (auto& user : userList)
	{
		if (user.userCore.UserName() == cleanName)
			return HawkeyeError::eAlreadyExists;
	}

	// Check for duplication in burned name list.
	for (auto burnt : burnedUsernameList)
	{
		if (burnt.username == cleanName ||
			burnt.username == cleanDisplay)
		{
			return HawkeyeError::eAlreadyExists;
		}
	}

	ExpandedUser newUser (cleanName, "Vi-CELL#0", cleanDisplay, UserPermissionLevel::eAdministrator);

	return WriteUserToDatabase (newUser);
}

// **************************************************************************
HawkeyeError UserList::SetUserFastMode(const std::string& uName, bool fastmode)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		user.allowFastMode = fastmode;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserFastMode(const std::string& uName, bool& fastmode)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		fastmode = user.allowFastMode;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetDisplayDigits(const std::string& uName, uint32_t displaydigits)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		user.displayDigits = displaydigits;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetDisplayDigits(const std::string& uName, uint32_t& displaydigits)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		displaydigits = user.displayDigits;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

/**
 ****************************************************************************
 * \brief Set a user's email.
 * \param uName is the username of the user to change the email.
 * \param email is the new email.
 * \return eSuccess if the email was successfully updated, eInvalidArgs if the user is not found, or eValidationFailed if the user is invalid.
 */
HawkeyeError UserList::SetUserEmail(const std::string& uName, const std::string& email)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		user.email = email;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserEmail(const std::string& uName, std::string& email)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		email = user.email;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetUserComment(const std::string& uName, const std::string& comment)
{

	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		user.comment = comment;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserComment(const std::string& uName, std::string& comment)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		comment = user.comment;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetUserRecord(const std::string& uName, UserRecord*& record)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;
		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		record = new UserRecord(); // somebody better delete this record
		record->displayDigits = user.displayDigits;
		record->allowFastMode = user.allowFastMode;
		record->defaultCellTypeIndex = user.defaultCellTypeIndex;
		record->defaultDilution = user.defaultDilution;
		record->defaultImageSaveN = user.defaultImageSaveN;
		record->defaultWashType = user.defaultWashType;
		record->exportPdfEnabled = user.exportPdfEnabled;
		record->permissionLevel = user.userCore.PermisionLevel();

		DataConversion::convertToCharPointer(record->username, uName);
		DataConversion::convertToCharPointer(record->displayName, user.displayName);
		DataConversion::convertToCharPointer(record->email, user.email);
		DataConversion::convertToCharPointer(record->comments, user.comment);
		DataConversion::convertToCharPointer(record->exportFolder, user.exportFolder);
		DataConversion::convertToCharPointer(record->csvFolder, user.csvFolder);
		DataConversion::convertToCharPointer(record->langCode, user.langCode);
		DataConversion::convertToCharPointer(record->defaultResultFileNameStr, user.defaultResultFileNameStr);
		DataConversion::convertToCharPointer(record->defaultSampleNameStr, user.defaultSampleNameStr);

		return HawkeyeError::eSuccess;
	}
	return HawkeyeError::eEntryNotFound;
}

// **************************************************************************
HawkeyeError UserList::SetUserAnalysisIndices(const std::string& uName, std::vector<uint32_t> index_list)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}
		
		user.analysisIndices = index_list;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;

}

// **************************************************************************
HawkeyeError UserList::GetUserAnalysisIndices(const std::string& uName, std::vector<uint32_t>& index_list)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		index_list = user.analysisIndices;

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetAllowedCellTypeIndices(const std::string& uName, std::vector<uint32_t> index_list)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		user.cellTypeIndices = index_list;

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;

}

// **************************************************************************
HawkeyeError UserList::AddCellTypeIndex (uint32_t ct_index)
{

	// Add cell type index to all users with advanced and admin permissions.
	for (auto& user : userList)
	{
		if (!user.userCore.IsValid())
			continue;

		// All AD users get new cell types
		// Local security + Normal users do NOT get new CTs
		if ((!user.ADUser) && (user.userCore.PermisionLevel() == UserPermissionLevel::eNormal))
			continue;

		user.cellTypeIndices.push_back (ct_index);		
		WriteUserToDatabase (user);
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::RemoveCellTypeIndex(uint32_t ct_index)
{
	// Remove the cell type index from all users that have it
	for (auto& user : userList)
	{
		if (!user.userCore.IsValid())
			continue;

		auto it = std::find(user.cellTypeIndices.begin(), user.cellTypeIndices.end(), ct_index);
		if (it != user.cellTypeIndices.end())
		{
			user.cellTypeIndices.erase(it);
			WriteUserToDatabase(user);
		}
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError UserList::GetAllowedCellTypeIndices (const std::string& uName, std::vector<uint32_t>& index_list)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		index_list = user.cellTypeIndices;		

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::SetSampleColumns (const std::string& uName, ColumnSetting* recs, uint32_t count)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;
		
		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}
		
		if (count > 0)
		{
			for (uint32_t j = 0; j < count; j++)
			{
				ColumnSetting newCS = recs[j];
				for (std::list<DBApi::display_column_info>::iterator currRec = user.displayColumns.begin(); currRec != user.displayColumns.end(); ++currRec)
				{
					if (newCS.Column == currRec->ColumnType)
					{
						// Found - update this record
						currRec->Visible = newCS.IsVisible;
						currRec->Width = static_cast<uint16_t>(newCS.Width);
						break;
					}
				}
			}
		}

		return WriteUserToDatabase (user);
	}

	return HawkeyeError::eInvalidArgs;
}

// **************************************************************************
HawkeyeError UserList::GetSampleColumns (const std::string& uName, ColumnSetting*& recs, uint32_t& retrieved_count)
{
	for (auto& user : userList)
	{
		if (user.userCore.UserName() != uName)
			continue;

		if (!user.userCore.IsValid())
		{
			return HawkeyeError::eValidationFailed;
		}

		const int MAX_COUNT = 15;
		retrieved_count = static_cast<uint32_t>(user.displayColumns.size());
		recs = new ColumnSetting[retrieved_count];

		if (retrieved_count > 0)
		{
			int colIdx = 0;
			for (auto colIter = user.displayColumns.begin(); colIter != user.displayColumns.end() && colIdx < MAX_COUNT; ++colIter, colIdx++)
			{
				DBApi::display_column_info_t dbCol = *colIter;

				recs[colIdx].Column = static_cast<ColumnOption>(dbCol.ColumnType);
				// ??? = dbCol.OrderIndex;		order not user??
				recs[colIdx].IsVisible = dbCol.Visible;
				recs[colIdx].Width = dbCol.Width;
			}
		}

		return HawkeyeError::eSuccess;
	}

	return HawkeyeError::eInvalidArgs;
}

//*****************************************************************************
HawkeyeError UserList::GetUserRole (const std::string& username, UserPermissionLevel& permissionLevel)
{
	for (auto user_it = userList.begin(); user_it != userList.end(); ++user_it)
	{
		if (user_it->userCore.UserName() == username)
		{
			permissionLevel = user_it->userCore.PermisionLevel();
			return HawkeyeError::eSuccess;
		}
	}

	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
HawkeyeError UserList::GetActiveDirectoryGroupMaps (std::vector<ActiveDirectoryGroupDLL>& adGroups)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "GetActiveDirectoryGroupMaps: <enter>");

	for (auto v : dbRolesList)
	{
		ActiveDirectoryGroupDLL adGroup = {};

		if (v.GroupMapList.size())
		{
			//NOTE: As of ScoutX v1.3 there can only be ONE Group per role.
			adGroup.group = v.GroupMapList[0];
		}
		else
		{
			adGroup.group = "";
		}

		switch (static_cast<DBApi::eRoleClass>(v.RoleType))
		{
			case DBApi::eRoleClass::UserClass1:
				adGroup.role = UserPermissionLevel::eNormal;
				break;

			case DBApi::eRoleClass::ElevatedClass1:
				adGroup.role = UserPermissionLevel::eElevated;
				break;

			case DBApi::eRoleClass::AdminClass1:
				adGroup.role = UserPermissionLevel::eAdministrator;
				break;

			default:
				//TODO: throw warning that an unknown role was found.
				continue;
			}

		adGroups.push_back(adGroup);
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "GetActiveDirectoryGroupMaps: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError UserList::SetActiveDirectoryGroupMaps (const std::vector<ActiveDirectoryGroupDLL>& adGroups)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "SetActiveDirectoryGroupMaps: <enter>");

	std::string logstr = "Security on - Set Group to Role maps \n";

	// Match the ActiveDirGroups to the DB roles.
	// Only modify the Group name.
	for (auto& v : dbRolesList)
	{
		for (auto& vv : adGroups)
		{
			if (v.RoleType == (1 << (vv.role * 4)))
			{
				v.GroupMapList.clear();
				v.GroupMapList.push_back (vv.group);

				logstr += "\tGroup: " + std::string(vv.group);
				if (vv.role == UserPermissionLevel::eNormal)
					logstr += " maps to Normal\n";
				else if (vv.role == UserPermissionLevel::eElevated)
					logstr += " maps to Advanced\n";
				else if (vv.role == UserPermissionLevel::eAdministrator)
					logstr += " maps to Admin\n";

				DBApi::eQueryResult dbStatus = DbModifyUserRole (v);
				if (DBApi::eQueryResult::QueryOk != dbStatus)
				{
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("SetActiveDirectoryGroupMaps: <exit, DbModifyUserRole failed, status: %ld>") % (int32_t)dbStatus));
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::instrument_storage_writeerror,
						instrument_error::instrument_storage_instance::instrument_configuration,
						instrument_error::severity_level::error));
					return HawkeyeError::eStorageFault;
				}
			}
		}
	}

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_securityenable,
		logstr));

	Logger::L().Log(MODULENAME, severity_level::debug1, "SetActiveDirectoryGroupMaps: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError UserList::SetActiveDirectoryConfig (const ActiveDirectoryConfigDLL& adConfig)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "SetActiveDirectoryConfig: <enter>");

	DBApi::DB_InstrumentConfigRecord& initConfig = InstrumentConfig::Instance().Get();
	initConfig.AD_Settings.servername = adConfig.server;
	initConfig.AD_Settings.port_number = adConfig.port;
	initConfig.AD_Settings.base_dn = adConfig.domain;

	if (InstrumentConfig::Instance().Set())
	{
		std::string logstr = "Security on - SetActiveDirectoryConfig\n\tServer: " + initConfig.AD_Settings.servername;
		logstr += "\n\tDomain: " + initConfig.AD_Settings.base_dn;
		logstr += "\n\tPort  : " + std::to_string(initConfig.AD_Settings.port_number);
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_securityenable,
			logstr));
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetActiveDirectoryConfig: <exit - failed to modify cfg record>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::instrument_configuration,
			instrument_error::severity_level::warning));
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "SetActiveDirectoryConfig: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError UserList::GetActiveDirectoryConfig (ActiveDirectoryConfigDLL& adConfig)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "GetActiveDirConfig: <enter>");

	DBApi::DB_InstrumentConfigRecord& initConfig = InstrumentConfig::Instance().Get();

	adConfig.port = static_cast<uint16_t>(initConfig.AD_Settings.port_number);
	adConfig.server = initConfig.AD_Settings.servername;
	adConfig.domain = initConfig.AD_Settings.base_dn;

	Logger::L().Log(MODULENAME, severity_level::debug2, "GetActiveDirConfig: <exit>");

	return HawkeyeError::eSuccess;
}
