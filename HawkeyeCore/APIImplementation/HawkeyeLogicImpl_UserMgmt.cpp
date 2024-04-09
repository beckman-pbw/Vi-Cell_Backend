// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppInconsistentNaming
#include "stdafx.h"

#include "AuditLog.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "GetAsStrFunctions.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "Logger.hpp"
#include "QualityControlsDLL.hpp"

#define EXPIRY_DATE_FORMAT "%b %d %Y"
#define EXPIRY_DATE_PROPERTY_NAME "PasswordExpiry"

static const char MODULENAME[] = "UserMgt";

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::GetUserList (bool only_enabled, char**& userList, uint32_t& numUsers)
{
	std::vector<std::string> un = UserList::Instance().GetUserNames(only_enabled, true);

	if (un.size() > 0)
	{
		userList = new char*[un.size()];
		for (std::size_t i = 0; i < un.size(); i++)
		{
			DataConversion::convertToCharPointer(userList[i], un[i]);
		}
	}
	else
	{ 
		userList = nullptr; 
	}

	numUsers = (uint32_t)un.size();

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::GetCurrentUser(char*& name, UserPermissionLevel& permissions)
{
	name = nullptr;
	permissions = {};

	std::string username = UserList::Instance().GetConsoleUsername();
	if (username.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetCurrentUser: <exit, not allowed>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	DataConversion::convertToCharPointer(name, username);

	return UserList::Instance().GetLoggedInUserPermissions(permissions);
}

// **************************************************************************
// this retrieves the permission level for the supplied user login name; it does not check against the current effective user
HawkeyeError HawkeyeLogicImpl::GetUserPermissionLevel(const char* name, UserPermissionLevel& permissions)
{
	HawkeyeError he = UserList::Instance().GetUserPermissionLevel(name, permissions);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "GetUserPermissionLevel: <exit, cannot get user permissions>");
		return he;
	}
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::LoginConsoleUser (const char* name, const char* password)
{
	if (UserList::Instance().IsConsoleUserLoggedIn())
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "LoginConsoleUser: <exit, user already logged in>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	HawkeyeError he = UserList::Instance().LoginUser(name, password, false);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
void HawkeyeLogicImpl::LogoutConsoleUser()
{
	// limited to local logins only
	if (UserList::Instance().IsConsoleUserLoggedIn())
	{
		auto uname = UserList::Instance().GetConsoleUsername();
		UserList::Instance().LogoutUser();

		// Only add to the audit log if there is a user currently logged in
		AuditLogger::L().Log (generateAuditWriteData(
			uname,
			audit_event_type::evt_logout, 
			"Console logout"));
	}
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::LoginRemoteUser(const char* username, const char* password)
{
	auto he = UserList::Instance().LoginRemoteUser(username, password);
	if (he != HawkeyeError::eSuccess)
	{
		return HawkeyeError::eValidationFailed;
	}

	return HawkeyeError::eSuccess;	
}

// **************************************************************************
void HawkeyeLogicImpl::LogoutRemoteUser(const char* username)
{
	UserList::Instance().LogoutRemoteUser(username);
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SwapUser(const char* newusername, const char* password, SwapUserRole swapRole)
{
	HawkeyeError he = UserList::Instance().LoginUser (newusername, password, true);
	if (he != HawkeyeError::eSuccess)
	{
		// No logging required here...  lower level code does plenty of logging.
		return he;
	}

	UserPermissionLevel permissionLevel;

	// Only admin or service user allowed to perform swap
	he = UserList::GetUserRole (newusername, permissionLevel);
	if ((UserPermissionLevel::eAdministrator != permissionLevel) && (UserPermissionLevel::eService != permissionLevel))
	{
		// Not an admin nor service user
		return HawkeyeError::eNotPermittedByUser;
	}
	
	if ((swapRole == SwapUserRole::eAdminOnly) && (UserPermissionLevel::eAdministrator != permissionLevel))
	{
		return HawkeyeError::eNotPermittedByUser;
	}
	
	if ((swapRole == SwapUserRole::eServiceOnly) && (UserPermissionLevel::eService != permissionLevel))
	{
		return HawkeyeError::eNotPermittedByUser;
	}
	
	// The user we are going to swap to meets the required role
	// Log out the current user
	LogoutConsoleUser();

	// Log in new user
	return LoginConsoleUser(newusername, password);
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::AdministrativeUserEnable(const char* administrator_account, const char* administrator_password, const char* user_account)
{
	if ( nullptr == administrator_account || nullptr == administrator_password || nullptr == user_account )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "AdministrativeUserEnable : Unable to enable user with missing components: administrator_account : '%s', user_account : '%s'" ) % administrator_account % user_account );
		if ( nullptr == administrator_password )
		{
			logStr.append( ", empty administrator_password" );
		}

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	auto he = UserList::Instance().AdministrativeEnable( administrator_account, administrator_password, user_account );
	if ( he != HawkeyeError::eSuccess )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "AdministrativeUserEnable: Failed to administrative enable user \"" + std::string( user_account ) + "\"" );
	}
	return he;
}

// **************************************************************************
char* HawkeyeLogicImpl::GenerateHostPassword(const char* securitykey)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "GenerateHostPassword: <enter>");

	if ( nullptr == securitykey )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "GenerateHostPassword: missing key" );
	}

	std::string hpw = UserList::Instance().GenerateHostPassword(securitykey);

	char* op = nullptr;
	DataConversion::convertToCharPointer (op, hpw);

	return op;
}

// **************************************************************************
void HawkeyeLogicImpl::GetUserInactivityTimeout(uint16_t& minutes)
{
	minutes = HawkeyeConfig::Instance().get().inactivityTimeout_mins;
}

// **************************************************************************
void HawkeyeLogicImpl::GetUserPasswordExpiration(uint16_t& days)
{
	days = HawkeyeConfig::Instance().get().passwordExpiration;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::LogoutUser_Inactivity()
{

	if (UserList::Instance().IsConsoleUserLoggedIn()) 
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (UserList::Instance().IsConsoleUserPermissionAtLeast(UserPermissionLevel::eNormal)) 
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "LogoutUser_Inactivity: <exit, not allowed>");
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Logout User, Inactivity"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (eSECURITYTYPE::eNoSecurity == GetSystemSecurityType()) 
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "LogoutUser_Inactivity: <exit, invalid args>");
		return HawkeyeError::eInvalidArgs;
	}

	UserList::Instance().LogoutUser();

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_logoutforced, 
		"Logout User, Inactivity"));
	
	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetUserInactivityTimeout(uint16_t minutes)
{
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Set User Inactivity Timeout"));

		return HawkeyeError::eNotPermittedByUser;
	}

	// Set only if there is change.
	if (HawkeyeConfig::Instance().get().inactivityTimeout_mins != minutes)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_setuserinactivitytimeout,
		    boost::str(boost::format("Inactivity timeout changed\n\tNew value: %d minutes\n\tPrevious value: %d minutes") 
				% minutes % (HawkeyeConfig::Instance().get().inactivityTimeout_mins))));

		HawkeyeConfig::Instance().get().inactivityTimeout_mins = minutes;
	}

	return HawkeyeError::eSuccess;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetUserPasswordExpiration (uint16_t days)
{
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Set User Password Expiration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// Set only if there is a change.
	if (HawkeyeConfig::Instance().get().passwordExpiration != days)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_setuserpasswordexpiration,
			boost::str(boost::format("Password expiration changed\n\tNew value: %d days\n\tPrevious value: %d days")
				% days % (HawkeyeConfig::Instance().get().passwordExpiration))));

		HawkeyeConfig::Instance().get().passwordExpiration = days;
	}

	return HawkeyeError::eSuccess;
}

//**************************************************************************
// This is only used in Local Security mode.
// A/D users are added to the database when logging in (see UserList::LoginUser).
//**************************************************************************
HawkeyeError HawkeyeLogicImpl::AddUser(const char* name, const char* displayname, const char* password, UserPermissionLevel permission)
{
	std::string consoleUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == name || nullptr == displayname || nullptr == password )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "AddUser : Unable to add user with missing components: name : '%s', display_name : '%s'" ) % name % displayname );
		if ( nullptr == password )
		{
			logStr.append( ", empty password" );
		}

		Logger::L().Log( MODULENAME, severity_level::error, logStr);
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );
	std::string pwdStr( password );
	std::string displayNameStr(displayname);

	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		auto description_str = boost::str(boost::format("<with name : %s, display_name : %s> from logged in User : %s")
			% nameStr
			% displayNameStr
			% consoleUsername);
		Logger::L().Log (MODULENAME, severity_level::warning, "Restricted Access : Invalid attempt to add user" + description_str);

		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_notAuthorized, 
			"Add User (not authorized)\n\tName: \"" + nameStr +
				"\"\n\tDisplay name \"" + displayNameStr +
				"\"\n\tLevel: " + getPermissionLevelAsStr(permission)));

		return HawkeyeError::eNotPermittedByUser;
	}

	ExpandedUser EU (nameStr, pwdStr, displayNameStr, permission);
	pwdStr.clear();
	HawkeyeError he = UserList::Instance().AddUser (EU);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_useradd,
			"Added User\n\tName: \"" + nameStr +
				"\"\n\tDisplay name \"" + displayNameStr +
					"\"\n\tLevel: " + getPermissionLevelAsStr(permission)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str(
			boost::format("AddUser : Unable to add user <with name : %s, display_name : %s> from logged in User : %s")
			% nameStr
			% displayNameStr
			% consoleUsername));
	}

	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::RemoveUser (const char* name)
{
	if ( nullptr == name )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "RemoveUser : invalid name : '%s'" ) % name );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );
	std::string consoleUsername = UserList::Instance().GetConsoleUsername();

	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_notAuthorized, 
			"Remove user \"" + nameStr + "\""));

		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to remove user <with name : %s> from logged in User : %s")
			% nameStr			
			% consoleUsername));

		return HawkeyeError::eNotPermittedByUser;
	}

	HawkeyeError he = UserList::Instance().RemoveUser(nameStr);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_userremove, 
			"Removed User\n\tName: \"" + nameStr + "\""));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str(
			boost::format("RemoveUser :Unable to remove user <with name : %s> from logged in User : %s")
			% nameStr	
			% consoleUsername));
	}
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::EnableUser(const char* name, bool enabled)
{
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == name )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "EnableUser : invalid name : '%s'" ) % name );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );

	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
	{
		// If no logged in user.
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			return status;
		}

		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to enable user <with name : %s, set_enabled : %s> from logged in User : %s")
			% nameStr
			% (enabled ? "Enabled" : "Disabled")
			% loggedInUsername));
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			boost::str(boost::format("%s user \"%s\"") %(enabled ? "Enable " : "Disable ") % nameStr)));

		return status;
	}

	HawkeyeError he = UserList::Instance().EnableUser(nameStr, enabled);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_userenable,
			"Enabled User\n\tName: \"" + nameStr + "\""
			+ "\n\tEnabled: " + (enabled ? "true" : "false")));
	}

	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ChangeUserPassword (const char* name, const char* password)
{
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == name )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "ChangeUserPassword : Unable to add user with missing components: name : '%s'" ) );
		if ( nullptr == password )
		{
			logStr.append( ", empty password" );
		}

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );
	std::string pwdStr( password );

	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (status != HawkeyeError::eSuccess)
	{
		// If no user is not logged in 
		if (status == HawkeyeError::eNotPermittedAtThisTime)
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized,
			"Password reset\n\tUser: \"" + nameStr +"\""));
		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to change password for user <with name : %s> from logged in User : %s")
			% nameStr	
			% loggedInUsername));

		return status;
	}

	HawkeyeError he = UserList::Instance().ChangeUserPassword(nameStr, pwdStr);
	pwdStr.clear();
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_passwordreset, 
			"Password reset\n\tUser: \"" + nameStr + "\""));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str(
			boost::format("ChangeUSerPassword : Unable to change password for user <with name : %s> from logged in User : %s")
			% nameStr
			% loggedInUsername));
	}
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ChangeUserDisplayName(const char* username, const char* displayname)
{
	std::string consoleUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == username || nullptr == displayname )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "ChangeUserDisplayName : Unable to change user displaynamewith missing components: username : '%s', display_name : '%s'" ) % username % displayname );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( username );
	std::string displayNameStr( displayname );

	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_notAuthorized,
			"Change User Display Name\n\tUser: \"" + nameStr + "\""));

		return HawkeyeError::eNotPermittedByUser;
	}

	HawkeyeError he = UserList::Instance().SetUserDisplayName(nameStr, displayNameStr);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_setusercfg, "Set User Display Name\n\tName: \"" + nameStr + "\""
			+ "\n\tDisplay Name: \"" + displayNameStr + "\""));
	}

	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ChangeUserPermissions(const char* name, UserPermissionLevel permissions)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "ChangeUserPermissions: <enter>");

	if ( nullptr == name )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "ChangeUserPermissions : Unable to change user permissions with missing components: name : '%s'" ) % name );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );

	if (eSECURITYTYPE::eActiveDirectory == GetSystemSecurityType())
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "ChangeUserPermissions: <exit - not allowed in AD security mode>");
		return HawkeyeError::eSoftwareFault;
	}

	std::string consoleInUsername = UserList::Instance().GetConsoleUsername();

	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleInUsername,
			audit_event_type::evt_notAuthorized,
			"Permission level change\n\tUser: \"" + nameStr + "\""));

		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to change permission for user <with name : %s, permission : %s> from logged in User : %s")
			% nameStr
			% getPermissionLevelAsStr(permissions)
			% consoleInUsername));

		return HawkeyeError::eNotPermittedByUser;
	}

	// Store the old permission to save as audit log
	UserPermissionLevel oldPermission;
	UserList::Instance().GetUserPermissions(nameStr, oldPermission);

	if (oldPermission == permissions)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "ChangeUserPermissions: no change <exit>");
		return HawkeyeError::eSuccess;
	}

	HawkeyeError he = UserList::Instance().ChangeUserPermissions(nameStr, permissions);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleInUsername,
			audit_event_type::evt_userpermissionschange,
			boost::str(boost::format("Permission level change\n\tUser: \"%s\"\n\tNew value: %s\n\tPrevious value: %s")
				% nameStr % getPermissionLevelAsStr(permissions) % getPermissionLevelAsStr(oldPermission))));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str(
			boost::format("ChangeUserPermission: Unable to change permission for user <name %s, permission : %s> logged in user %s")
			% nameStr
			% getPermissionLevelAsStr(permissions)
			% consoleInUsername));
	}
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetAllowedCellTypeIndices (const char* username, uint32_t nCells, uint32_t* celltype_indices)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SetUserCellTypeIndices: <enter>");

	if ( nullptr == username )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "SetUserCellTypeIndices : invalid name : '%s'" ) % username );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( username );
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if (loggedInUsername != nameStr)
	{
		// If the logged in user is requesting data for a different user 
		// then they must have at least elevated permission 
		auto status = UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
		if (status != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
				boost::format("Restricted Access : Invalid attempt to set user cell type indices for user <with name : %s> from logged in User : %s")
				% nameStr
				% loggedInUsername));
			return status;
		}
	}

	std::string temp;
	std::vector<uint32_t> indexList = {};
	for (uint32_t i = 0; i < nCells; i++)
	{
		indexList.push_back(celltype_indices[i]);
		temp += std::to_string (indexList.back()) + ", ";
	}
	
	auto he = UserList::Instance().SetAllowedCellTypeIndices(nameStr, indexList);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_setusercfg,
			"Set User CellTypes\n\tName: \"" + nameStr + "\"\n\tCellTypes: \"" + temp + "\""));
	}

	return he;
}


// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetUserAnalysisIndices (const char* username, uint32_t n_ad, uint16_t* analysis_indices)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SetUserAnalysisIndices: <enter>");

	if ( nullptr == username )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "SetUserAnalysisIndices : invalid name : '%s'" ) % username );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( username );
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	// limited to local logins only
	auto he = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to set user analysis indices for user <with name : %s> from logged in User : %s")
			% nameStr
			% loggedInUsername));
		return he;
	}

	std::vector<uint32_t> indexList = {};
	std::string temp;
	for (uint32_t i = 0; i < n_ad; i++)
	{
		indexList.push_back(analysis_indices[i]);
		temp += std::to_string (indexList.back()) + ", ";		
	}

	he = UserList::Instance().SetUserAnalysisIndices(nameStr, indexList);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_setusercfg,
			"Set User Analyses\n\tName: \"" + nameStr + "\"\n\tAnalyses: \"" + temp + "\""));
	}

	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::GetUserAnalysisIndices (const char* name, uint32_t& n_ad, uint16_t*& analysis_indices)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetUserAnalysisIndices: <enter>");

	if ( nullptr == name )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "GetUserAnalysisIndices : invalid name : '%s'" ) % name );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( name );

	n_ad = 0;
	analysis_indices = nullptr;
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if (loggedInUsername != nameStr)
	{
		// If the logged in user is requesting data for a deifferent user 
		// then they must have at least elevated permission 
		auto status = UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
		if (status != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
				boost::format("Restricted Access : Invalid attempt to get user analysis indices for user <with name : %s> from logged in User : %s")
				% nameStr
				% loggedInUsername));
			return status;
		}
	}

	std::vector<uint32_t> vanalyses;
	HawkeyeError he = UserList::Instance().GetUserAnalysisIndices(nameStr, vanalyses);
	if (he != HawkeyeError::eSuccess) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetUserAnalysisIndices <exit, not found>");
		return he;
	}

	n_ad = (uint32_t)vanalyses.size();
	if (n_ad > 0)
	{
		analysis_indices = new uint16_t[n_ad];
		for (uint32_t i = 0; i < n_ad; i++)
		{
			analysis_indices[i] = static_cast<uint16_t>(vanalyses[i]);
		}
	}

	return he;
}


// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetUserFolder(const char* username, const char* folder)
{
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == username || nullptr == folder )
	{
		std::string logStr = "";

		logStr = boost::str( boost::format( "SetUserFolder : <exit, invalid args : name : '%s', folder : '%s'" ) % username % folder );

		Logger::L().Log( MODULENAME, severity_level::error, logStr );
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( username );
	std::string folderStr( folder );

	// limited to local logins only
	auto he = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eAdministrator);
	if (he != HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized,
			"Set User Folder"));
		return he;
	}

	he = UserList::Instance().SetUserFolder(nameStr, folderStr);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_setusercfg,
			"Set User Folder\n\tName: \"" + nameStr + "\"\n\tFolder: \"" + folderStr + "\""));
	}

	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ChangeMyPassword(const char* oldpassword, const char* newpassword)
{
	if ( nullptr == oldpassword || nullptr == newpassword )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "ChangeMyPassword : <exit, invalid args>" );
		return HawkeyeError::eInvalidArgs;
	}

	if (eSECURITYTYPE::eActiveDirectory == GetSystemSecurityType())
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "ChangeMyPassword: <exit - not allowed in AD security mode>");
		return HawkeyeError::eSoftwareFault;
	}

	std::string consoleUsername = UserList::Instance().GetConsoleUsername();

	// This must be done by any  Logged in user and restricted to Service Engineer
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserLoggedIn())
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_notAuthorized,
			"Change My password"));

		Logger::L().Log (MODULENAME, severity_level::warning, boost::str(
			boost::format("Restricted Access : Invalid attempt to change logged-in user password from logged in User : %s")
				% consoleUsername));
		return HawkeyeError::eNotPermittedByUser;
	}

	HawkeyeError he = UserList::Instance().ChangeCurrentUserPassword(oldpassword, newpassword);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			consoleUsername,
			audit_event_type::evt_passwordchange, 
			"User-initiated password change"));
	}
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::IsPasswordExpired(const char* uname, bool& expired)
{
	return UserList::Instance().IsPasswordExpired(uname, expired);
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ValidateMe(const char* password)
{
	if ( nullptr == password )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "ValidateMe : <exit, invalid args>" );
		return HawkeyeError::eInvalidArgs;
	}

	auto status = UserList::Instance().ValidateConsoleUser(password);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No user logged in
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetConsoleUsername(),
				audit_event_type::evt_notAuthorized,
				"Validate Credentials"));
		else // Failed validation.
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetConsoleUsername(),
				audit_event_type::evt_loginfailure, 
				"Validate Credentials"));
	}
		
	return status;
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
HawkeyeError HawkeyeLogicImpl::ValidateUserCredentials(const char* username, const char* password)
{
	if ( nullptr == username || nullptr == password )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "ValidateUserCredentials : <exit, invalid args>" );
		return HawkeyeError::eInvalidArgs;
	}

	HawkeyeError he = UserList::Instance().LoginUser(username, password, true);
	// No logging required here...  lower level code does plenty of logging.
	
	return he;
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::ValidateLocalAdminAccount(const char* username, const char* password)
{
	if ( nullptr == username || nullptr == password )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "ValidateLocalAdminAccount : <exit, invalid args>" );
		return HawkeyeError::eInvalidArgs;
	}

	auto result = UserList::Instance().ValidateLocalAdminAccount(username, password);
	return result;
}


//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetDisplayDigits(const char* username, uint32_t digits)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetDisplayDigits: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}
	
	auto result = UserList::Instance().SetDisplayDigits(username, digits);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetDisplayDigits: <exit, UserList...SetDisplayDigits failed>");
		return HawkeyeError::eStorageFault;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetUserFastMode(const char* username, bool enable)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserFastMode: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	auto result = UserList::Instance().SetUserFastMode(username, enable);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserFastMode: <exit, UserList...SetUserFastMode failed>");
		return HawkeyeError::eStorageFault;
	}

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_setusercfg,
		"Set User Fast Mode\n\tName: \"" + std::string(username) + "\"\n\tFastMode: " + (enable ? "true" : "false")));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
/**
 * \brief Set a user's email.
 * \param uName is the username of the user to change the email.
 * \param email is the new email. If email is null, an empty string is used to clear the value.
 * \param cb - the callback, cb, is invoked if any error occurs providing an eStorageFault.
 */
HawkeyeError HawkeyeLogicImpl::SetUserEmail(const char* username, const char* email)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserEmail: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	const std::string tempEmail(nullptr != email ? email : "");
	auto result = UserList::Instance().SetUserEmail(username, tempEmail);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserEmail: <exit, UserList...SetUserEmail failed>");
		return HawkeyeError::eStorageFault;
	}

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_setusercfg,
		"Set User Email\n\tName: \"" + std::string(username) + "\"\n\tEmail: \"" + tempEmail + "\""));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
/**
 * \brief Set a user's comment.
 * \param uName is the username of the user to change the comment.
 * \param comment is the new comment. If comment is null, an empty string is used to clear the value.
 * \param cb - the callback, cb, is invoked if any error occurs providing an eDatabaseError.
 */
HawkeyeError HawkeyeLogicImpl::SetUserComment(const char* username, const char* comment)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserComment: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr( username );
	const std::string tempComment(nullptr != comment ? comment : "");
	auto result = UserList::Instance().SetUserComment(nameStr, tempComment);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetUserComment: <exit, UserList...SetUserComment failed>");
		return HawkeyeError::eStorageFault;
	}

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_setusercfg,
		"Set User Comment\n\tName: \"" + nameStr + "\"\n\tComment: \"" + tempComment + "\""));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetSampleColumns(const char* username, ColumnSetting*& recs, uint32_t& retrieved_count)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetSampleColumns: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	auto result = UserList::Instance().GetSampleColumns(username, recs, retrieved_count);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetSampleColumns: <exit, UserList...SetSampleColumns failed>");
		return HawkeyeError::eStorageFault;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetSampleColumns(const char* username, ColumnSetting* recs, uint32_t count)
{
	if (recs == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}
	
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetSampleColumns: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	auto result = UserList::Instance().SetSampleColumns(username, recs, count);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetSampleColumns: <exit, UserList...SetSampleColumns failed>");
		return HawkeyeError::eStorageFault;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSampleColumns(ColumnSetting* recs, uint32_t count)
{
	if (recs == NULL)
		return;

	delete[] recs;
	recs = NULL;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetUserRecord(const char* username, UserRecord*& record)
{
	if (username == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetUserRecord: <exit, no username supplied>");
		return HawkeyeError::eInvalidArgs;
	}

	std::string nameStr(username);
	//Keep for debugging: Logger::L().Log(MODULENAME, severity_level::debug1, "GetUserRecord: <enter user:> " + nameStr);

	if (record != nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "GetUserRecord: possible memory leak; pointer not null");
	}

	auto result = UserList::Instance().GetUserRecord(nameStr, record);
	if (result != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetUserRecord: <exit, UserList...GetUserComment failed>");
		return HawkeyeError::eStorageFault;
	}
	
	// keep for debug Logger::L().Log(MODULENAME, severity_level::error, "GetUserRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeUserRecord(UserRecord* record)
{
	if (record == nullptr) return;

	if (record->displayName != nullptr)
	{
		delete[] record->displayName;
		record->displayName = nullptr;
	}	
	if (record->email != nullptr)
	{
		delete[] record->email;
		record->email = nullptr;
	}
	if (record->comments != nullptr)
	{
		delete[] record->comments;
		record->comments = nullptr;
	}
	if (record->exportFolder != nullptr)
	{
		delete[] record->exportFolder;
		record->exportFolder = nullptr;
	}
	if (record->csvFolder != nullptr)
	{
		delete[] record->csvFolder;
		record->csvFolder = nullptr;
	}
	if (record->langCode != nullptr)
	{
		delete[] record->langCode;
		record->langCode = nullptr;
	}
	if (record->defaultResultFileNameStr != nullptr)
	{
		delete[] record->defaultResultFileNameStr;
		record->defaultResultFileNameStr = nullptr;
	}
	if (record->defaultSampleNameStr != nullptr)
	{
		delete[] record->defaultSampleNameStr;
		record->defaultSampleNameStr = nullptr;
	}
	delete record;
	record = nullptr;
}
