// ReSharper disable CppInconsistentNaming
// ReSharper disable CppFunctionIsNotImplemented
#pragma once

#include <list>
#include <map>
#include <regex>

#include "ActiveDirectoryDLL.hpp"
#include "ColumnSetting.hpp"
#include "Configuration.hpp"
#include "DBif_Api.h"
#include "ExpandedUser.hpp"
#include "ExpandedUsers.hpp"
#include "HawkeyeUser.hpp"
#include "HawkeyeError.hpp"
#include "SecurityHelpers.hpp"
#include "UserRecord.hpp"

class UserList
{
public:	
	static UserList& Instance()
	{
		static UserList instance;
		return instance;
	}

	static const std::string UserListFile;

	UserList();
	virtual ~UserList();

	// This is the list of names which are no longer available for reuse.
	// We will prohibit the re-assignment of names from deleted users.
	class BurnedUsers
	{
	public:
		BurnedUsers() {};
		BurnedUsers(std::string t_username, uuid__t t_uuid)
		{
			username = t_username;
			uuid = t_uuid;
		}
		std::string username;
		uuid__t uuid;
	};

	static eSECURITYTYPE GetSystemSecurityType();

	/// Save/Load
	/// Load users list from database
	HawkeyeError Initialize (bool isImporting);

	/// Write a user's info from the user list to database
	std::pair<std::string, boost::property_tree::ptree> Export();
	HawkeyeError LoginUser(const std::string& uName, const std::string& uPW, bool validateOnly);
	HawkeyeError LogoutUser();
	HawkeyeError LoginRemoteUser (const std::string& username, const std::string& password);
	void         LogoutRemoteUser (const std::string& username);
	HawkeyeError ValidateUserCredentials (const std::string& username, const std::string& password);
	void         LoginADUser (std::vector<std::string>& adgrouplist, const std::string uname, const std::string upw, const ActiveDirectoryConfigDLL adcfg);
	HawkeyeError AdministrativeUnlock(const std::string& admin_uname, const std::string& admin_pwd, const std::string& uName);
	std::string  GenerateHostPassword(const std::string& keyval);

	HawkeyeError AddFactoryAdminUser();
	HawkeyeError RemoveUser(const std::string& uName);
	HawkeyeError EnableUser(const std::string& uName, bool uEnabled);
	HawkeyeError ChangeUserPassword(const std::string& uName, const std::string& password, bool resetPwd = false);
	HawkeyeError GetUserPermissionLevel(const std::string& uName, UserPermissionLevel& permissions);
	HawkeyeError ChangeUserPermissions(const std::string& uName, UserPermissionLevel permissions);
	HawkeyeError SetUserFolder(const std::string& uName, const std::string& folder);
	HawkeyeError GetUserFolder(const std::string& uName, std::string& folder);
	HawkeyeError SetUserDisplayName(const std::string& uName, const std::string& dName);
	HawkeyeError GetUserDisplayName(const std::string& uName, std::string& dName);
	HawkeyeError GetUserUUIDByName (const std::string& uName, uuid__t& uuid);
	HawkeyeError GetUserByName(const std::string& uName, ExpandedUser** user);
	HawkeyeError GetUsernameByUUID (const uuid__t& uuid, std::string& uName);

	HawkeyeError SetUserFastMode(const std::string& uName, bool fastmode);
	HawkeyeError GetUserFastMode(const std::string& uName, bool& fastmode);
	HawkeyeError SetDisplayDigits(const std::string& uName, uint32_t displaydigits);
	HawkeyeError GetDisplayDigits(const std::string& uName, uint32_t& displaydigits);
	HawkeyeError SetUserEmail(const std::string& uName, const std::string& email);
	HawkeyeError GetUserEmail(const std::string& uName, std::string& email);
	HawkeyeError SetUserComment(const std::string& uName, const std::string& comment);
	HawkeyeError GetUserComment(const std::string& uName, std::string& comment);

	HawkeyeError GetUserRecord(const std::string& uName, UserRecord*& record);

	HawkeyeError SetSampleColumns(const std::string& uName, ColumnSetting* recs, uint32_t count);
	HawkeyeError GetSampleColumns(const std::string& uName, ColumnSetting*& recs, uint32_t& retrieved_count);

	static HawkeyeError GetUserRole (const std::string& username, UserPermissionLevel& permissionLevel);
	
	HawkeyeError SetUserAnalysisIndices(const std::string& uName, std::vector<uint32_t> index_list);
	HawkeyeError GetUserAnalysisIndices(const std::string& uName, std::vector<uint32_t>& index_list);

	HawkeyeError SetAllowedCellTypeIndices(const std::string& uName, std::vector<uint32_t> index_list);
	HawkeyeError GetAllowedCellTypeIndices(const std::string& uName, std::vector<uint32_t>& index_list);

	HawkeyeError AddCellTypeIndex (uint32_t ct_index);
	HawkeyeError RemoveCellTypeIndex(uint32_t ct_index);


	std::vector<std::string> GetUserNames(bool onlyEnabled, bool onlyValid = true);
	bool        IsConsoleUserLoggedIn() const;

	std::string GetConsoleUsername();
	std::string GetLoggedInUsername();
	std::string GetRemoteUsername();
	std::string GetAttributableUserName(); // LH6531-6576 - Add a method to allow us to correctly attribute auditable tasks.

	HawkeyeError GetLoggedInUserDisplayName(std::string& uDName);
	HawkeyeError GetLoggedInUserPermissions(UserPermissionLevel& permissions);
	HawkeyeError GetLoggedInUserFolder(std::string& folder);

	HawkeyeError ValidateConsoleUser(const std::string& uPW);
	HawkeyeError ValidateLocalAdminAccount(const std::string& uname, const std::string& password);

	HawkeyeError ChangeCurrentUserPassword(const std::string& currentPW, const std::string& newPW);

	HawkeyeError GetUserPermissions (const std::string& uName, UserPermissionLevel& permissions);
	
	// Return codes: eSuccess, eNoneFound (no user logged in), eNotPermittedByUser
	HawkeyeError CheckPermissionAtLeast(const UserPermissionLevel& permission); // @deprecated - to be removed

	HawkeyeError CheckConsoleUserPermissionAtLeast( const UserPermissionLevel& permission );

	bool IsConsoleUserPermissionAtLeast(const UserPermissionLevel& permission);
	bool IsUserPermissionAtLeast(const std::string& uName, const UserPermissionLevel& permission);

	bool IsConsoleUserPermissionAtLeastAndNotService(const UserPermissionLevel& permission);
	bool IsUserPermissionAtLeastAndNotService(const std::string& uName, const UserPermissionLevel& permission);

	HawkeyeError IsPasswordExpired(const std::string& uName, bool& expired);

	HawkeyeError GetActiveDirectoryGroupMaps (std::vector<ActiveDirectoryGroupDLL>& adGroups);
	HawkeyeError SetActiveDirectoryGroupMaps (const std::vector<ActiveDirectoryGroupDLL>& adGroups);
	HawkeyeError GetActiveDirectoryConfig (ActiveDirectoryConfigDLL& cfg);
	HawkeyeError SetActiveDirectoryConfig (const ActiveDirectoryConfigDLL& adConfig);

	HawkeyeError AddUser (ExpandedUser& EU, bool isImported = false);
	static HawkeyeError WriteUserToDatabase (ExpandedUser& user, bool isImported = false);

	static void Import (boost::property_tree::ptree& ptConfig);
	static void ImportRoles (const boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it);

	// @todo - v1.4 add email address validation
	//static bool IsEmailValid_LatinLang(const std::string& email)
	//{		
	//	const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
	//	auto ret = !(std::regex_match(email, pattern));
	//	return ret;
	//}

protected:
	std::list<ExpandedUser>::iterator currentUser;

	// TODOO: The configuration as delivered to the customer should provide a default Administrative account.
	t_opPTree config_tree_root;
	std::string config_file_cache_name;

	// A service user is baked into the list with a dynamic password and CANNOT be modified or removed.
	std::list<ExpandedUser>::iterator serviceUser;
	ExpandedUser create_service_user();

	// An Ambr automation connector client is baked into the list with a dynamic password and CANNOT be modified or removed.
	std::list<ExpandedUser>::iterator automationUser;
	ExpandedUser create_automation_user();

	// The Silent Administrator is the account to be used when the instrument is in a "security disabled"
	//  state.  This user is under the control of the host UI and uses a dynamic password for access.
	//  Unlike the service user, changes to the Silent Administrator must track to the long-term storage.
	std::list<ExpandedUser>::iterator silentAdministrator;
	ExpandedUser create_silent_administrator();

	//// Leave for future reference or use
	//// The temporary Administrator is the account intended to be used by customer service when the customer has
	//// forgotten his password.  It uses an automatically generated password with a 1-day expiration.
	////  Like the service user, changes to the temporary Administrator do not track to the long-term storage.
	//std::list<ExpandedUser>::iterator tempAdministrator;
	//ExpandedUser create_temp_administrator();

	// Host password uses the HMAC protocol with a +/- 2 second window.
	bool ValidateHostPassword(const std::string& pw);
	// Service password uses HMAC protocol in years with zero window.
	bool ValidateServicePassword(const std::string& pw);
	
	// Ambr automatiion connector client password uses HMAC protocol in hours with +/- 1 hour window.
	// NOTE that this is not a standard automation user using a local or AD login.
	bool ValidateAutomationPassword( const std::string& pw );
	
	//// Leave for future reference or use
	//// Temporary admin password for service desk use; password uses HMAC protocol in days with 1 day window.
	//// This is an emergency admin user using a local or AD login.
	////bool ValidateTempPassword( const std::string& pw );

	static bool BuiltInAccount(const std::string& uName)
	{
		// Leave for future reference or use
//		return ( uName == SERVICE_USER ) || ( uName == SILENTADMIN_USER ) || ( uName == AUTOMATION_USER ) || ( uName == TEMPADMIN_USER );
		return ( uName == SERVICE_USER ) || ( uName == SILENTADMIN_USER ) || ( uName == AUTOMATION_USER );
	}

private:
	HawkeyeError	LoginADUser(const std::string& uName, const std::string& uPW, UserPermissionLevel& fndRole, std::string& fndEmail);
	bool			ImportUser (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it);
	HawkeyeError	CheckUserPermissionAtLeast( ExpandedUser* user, const UserPermissionLevel& permission );

	static HawkeyeError SetupExistingActiveDirectoryUser( const std::string& uName, const std::string& uPW, UserPermissionLevel adRole, std::list<ExpandedUser>::iterator user );
	static ExpandedUser SetupNewActiveDirectoryUser( const std::string& uName, const std::string& uPW, UserPermissionLevel adRole );
};
