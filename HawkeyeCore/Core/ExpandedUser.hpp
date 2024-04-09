#pragma once

#include "HawkeyeError.hpp"
#include "HawkeyeUser.hpp"
#include "SecurityHelpers.hpp"
#include "uuid__t.hpp"
#include "DBif_Structs.hpp"

const uint32_t MAX_LOGIN_ATTEMPTS = 5;
const uint32_t LOCKOUT_TIME = 30;
const uint32_t MIN_DISPLAY_DIGITS = 2;
const uint32_t MAX_DISPLAY_DIGITS = 4;

#define SERVICE_USER "bci_service"
#define SERVICE_USER_DN "Beckman Coulter Service"
/*#define SILENTADMIN_USER "Vi-CELL"
#define SILENTADMIN_USER_DN "Vi-CELL"
*/
#define SILENTADMIN_USER "Cydem"
#define SILENTADMIN_USER_DN "Cydem"
#define AUTOMATION_USER "automation"
#define AUTOMATION_USER_DN "Beckman Coulter Automation"

class ExpandedUser
{
public:
	ExpandedUser();
	ExpandedUser (const std::string& username, const std::string& password, const std::string& displayName, UserPermissionLevel permissionLevel, bool isActive = true);

	bool IsValid() const { return userCore.IsValid(); }// failures in loading extended information invalidates the core.
	DBApi::DB_UserRecord ToDbStyle();
	void FromDbStyle (DBApi::DB_UserRecord& dbUser);
	HawkeyeError AttemptLogin (
		const std::string& password,
		bool silentSuccess,
		std::function<bool(const std::string& password)> PasswordValidator);
	static bool IsReservedName(const std::string& uName);

	HawkeyeUser userCore = {};
	uuid__t uuid = {};
	bool isRetired = false;
	bool ADUser = false;
	std::string displayName = "";
	std::string comment = "";
	std::string email = "";
	std::string exportFolder = "";
	std::string csvFolder = "";
	std::string langCode = "";
	std::string defaultResultFileNameStr = "";
	std::string defaultSampleNameStr = "";
	int16_t	defaultImageSaveN = 1;
	int16_t defaultWashType = 0;
	int16_t defaultDilution = 1;
	uint32_t defaultCellTypeIndex = 0;
	bool exportPdfEnabled = false;
	uint32_t displayDigits = 2;
	system_TP lastLogin = {};
	uint32_t attemptCount = 0;
	bool allowFastMode = true;
	system_TP pwdChangeDate = {};
	std::list<DBApi::display_column_info_t> displayColumns = {};
	std::vector<uint32_t> cellTypeIndices = {};
	std::vector<uint32_t> analysisIndices = {};
	uuid__t roleID = {};

private:
	void CreateDefaultColumns();
	void ExpandedUser::AddAnyMissingCellTypes();
	void ExpandedUser::RemoveInvalidCellTypes();
};
