#include "stdafx.h"

#include "DBif_Api.h"

#include <boost/algorithm/string.hpp>

#include "AuditLog.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "ExpandedUser.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "Logger.hpp"
#include "UserLevels.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "ExpandedUser";

//*****************************************************************************
ExpandedUser::ExpandedUser()
{
	CreateDefaultColumns();
}

//*****************************************************************************
ExpandedUser::ExpandedUser (const std::string& username, const std::string& password, const std::string& dispName, UserPermissionLevel permissionLevel, bool isActive)
	: userCore (username, password, permissionLevel, isActive)
{
	std::string cleanName = boost::algorithm::trim_copy (username);
	std::string cleanDisplay = boost::algorithm::trim_copy (dispName);
	if (cleanDisplay.empty())
	{
		cleanDisplay = cleanName;
	}
	
	displayName = cleanDisplay;
	exportFolder = HawkeyeDirectory::Instance().getExportDir() + "\\" + cleanName;

	CreateDefaultColumns();
}

// **************************************************************************
bool ExpandedUser::IsReservedName(const std::string& uName)
{
	if ((uName == SERVICE_USER) ||
		(uName == SILENTADMIN_USER) ||
		//// leave for future reference or use
		//( uName == TEMPADMIN_USER ) ||
		(uName == AUTOMATION_USER))
		return true;
	else
		return false;
}

/**
 ***************************************************************************
 * \brief Attempt to validate the given password for this user. Handles login
 *        attempt counting and lockout / auto unlock after timeout.
 *
 * \param password - the password to validate
 * \param silentSuccess - don't add audit log on success
 *
 * \return eSuccess - password was validated and the the account is not disabled
 * \return eNotPermittedByUser - the account has been disabled by an admin
 * \return eTimedout - the account is locked out - too many login failures
					   this attempt just reset the timer for the timeout period
					   can self clear after the timeout period (currently 30 minutes)
 * \return eBusy - the account was just locked due to a login failure
 * \return eValidationFailed - the given password was invalid
 *                             this counts against the max consecutive login failures
 * \return eSoftwareFault - this user is an AD user and this function should never be called
 */
 //*****************************************************************************
HawkeyeError ExpandedUser::AttemptLogin (
	const std::string& password,
	bool silentSuccess,
	std::function<bool(const std::string& password)> PasswordValidator)
{
	if (this->ADUser) // We don't validate passwords or track login attempts for AD users
	{
		Logger::L().Log(MODULENAME, severity_level::error, std::string("ExpandedUser::AttemptLogin <exit, attempt to login an AD user>"));
		return HawkeyeError::eSoftwareFault;
	}

	// Log failure appropriately.
	auto PasswordValidationFailed = [ this, password, PasswordValidator ]( const std::string& username ) -> void
	{
		if ( !IsReservedName( username ) )		// don't allow built-in account to lockout...
		{
			attemptCount++;
		}

		if ( attemptCount >= MAX_LOGIN_ATTEMPTS )
		{
			AuditLogger::L().Log( generateAuditWriteData(
				username,
				audit_event_type::evt_accountlockout,
				"User account \"" + username + "\" lockout: too many failed login attempts." ) );

			// Cannot disable built-in accounts
			if ( !IsReservedName( username ) )
			{
				userCore.SetActivation( false );
			}

			return;
		}

		AuditLogger::L().Log( generateAuditWriteData(
			username,
			audit_event_type::evt_loginfailure,
			"Failed login attempt for user \"" + username + "\"" ) );
	};

	auto now = ChronoUtilities::CurrentTime();
	auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastLogin);
	lastLogin = ChronoUtilities::CurrentTime();
	std::string uname = userCore.UserName();
	bool isValid = false;

	if ( !userCore.IsActive() ||												// The user has been disabled, either due to failed logins or manually by an admin
		 ( IsReservedName( uname ) && attemptCount >= MAX_LOGIN_ATTEMPTS ) )	// an internal account has hit the failed login lockout count
	{
		// Note: IsActive() is used to both administratively disable an account 
		// and to temporarily lockout an account for too many attempted login failures. 
		// 
		// Only attempt to auto unlock (re-activate) a user if they have been disabled due to failed password attempts
		// If they are locked out by an admin we should not auto unlock after 30 minutes.
		// So, don't update the attempt count here. 
		if ( attemptCount < MAX_LOGIN_ATTEMPTS )
		{
			// This account has been disabled by an administrator
			return HawkeyeError::eNotPermittedByUser;
		}

		if ( elapsed.count() < LOCKOUT_TIME )
		{
			if ( IsReservedName( uname ) && attemptCount >= MAX_LOGIN_ATTEMPTS && PasswordValidator != nullptr )
			{
				isValid = PasswordValidator( password );
			}

			if ( !isValid )
			{
				isValid = ValidateResetPassword( uname, password );

				if ( isValid )
				{
					if ( !silentSuccess )
					{
						AuditLogger::L().Log( generateAuditWriteData(
							uname,
							audit_event_type::evt_login,
							"Console login override: " + uname ) );
					}

					attemptCount = 0;
					userCore.SetActivation( true );

					return HawkeyeError::eSuccess;
				}
			}

			if ( !isValid )
			{
				// This was another attempted login.
				// The caller resets the lockout timer by writing the user record to the DB with the updated lastLogin time.
				return HawkeyeError::eTimedout;
			}
		}

		if ( !isValid )
		{
			AuditLogger::L().Log( generateAuditWriteData(
				uname,
				audit_event_type::evt_userenable,
				"User " + uname + " lockout timeout expired." ) );
			userCore.SetActivation( true );
			attemptCount = 0;
		}
	}

	if ( !isValid )
	{
		if ( PasswordValidator == nullptr )
		{
			isValid = userCore.ValidatePassword( password );
			if ( isValid )
			{
				std::string faPwd = SILENTADMIN_USER;				// -> "Vi-CELL"
				faPwd.append( "#0" );								// -> "Vi-CELL#0"

				// check if this is the reuse of a 'reset' password or other otherwise invalid password other than the factory_admin default value
				if ( password.size() < MINPWDLEN && password != faPwd )
					isValid = false;
			}

			if ( !isValid )
			{
				isValid = ValidateResetPassword( uname, password );
			}
		}
		else
		{
			isValid = PasswordValidator( password );
		}

		if ( !isValid )
		{
			PasswordValidationFailed( uname );
			return HawkeyeError::eValidationFailed;
		}
	}

	if (!silentSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			uname,
			audit_event_type::evt_login,
			"Console login: " + uname));
	}

	attemptCount = 0;
	return HawkeyeError::eSuccess;
}

// Reset password checking logic
bool ExpandedUser::ValidateResetPassword( const std::string& username, const std::string& password )
{
	bool isValid = false;
	if ( !IsReservedName( username ) )
	{
		// temporary user password is in 1 day resolution.
		std::string instSN = HawkeyeConfig::Instance().get().instrumentSerialNumber;
		std::string keystr = userCore.UserName();

		boost::to_upper( instSN );
		keystr.append( instSN );

		isValid = SecurityHelpers::ValidateResetPasscode( password, SecurityHelpers::HMAC_DAYS, keystr );
		if ( isValid )
		{
			if ( userCore.SetPassword( password, false, true ) )
			{
				// TODO: Log password chance success
				// Since this is a password reset by user himself, so set the password change date property to current time as seconds from epoch UTC
				// Create password change date property with empty string
				// Set the current time as seconds from epoch UTC as password change date property value
				pwdChangeDate = ChronoUtilities::CurrentTime();

				HawkeyeError he = UserList::Instance().WriteUserToDatabase( *this );
				if ( HawkeyeError::eSuccess != he )
				{
					isValid = false;
				}
			}
		}
	}
	return isValid;
};

//**************************************************************************
void ExpandedUser::RemoveInvalidCellTypes()
{
	std::vector<CellTypeDLL> cellTypesDLL = CellTypesDLL::Get();
	if (cellTypesDLL.size() > 0)
	{
		for (std::vector<uint32_t>::iterator cti = cellTypeIndices.begin(); cti != cellTypeIndices.end();)
		{
			uint32_t idx = *cti;
			bool valid = false;
			for (auto ct : cellTypesDLL)
			{
				if (idx == ct.celltype_index)
				{
					valid = true;
					break;
				}
			}
			if (!valid)
			{
				cti = cellTypeIndices.erase(cti);
			}
			else
			{
				++cti;
			}
		}
	}

	// remove duplicates
	sort (cellTypeIndices.begin(), cellTypeIndices.end());
	cellTypeIndices.erase (unique(cellTypeIndices.begin(), cellTypeIndices.end()), cellTypeIndices.end());
}

//**************************************************************************
void ExpandedUser::AddAnyMissingCellTypes()
{
	std::vector<CellTypeDLL> cellTypesDLL = CellTypesDLL::Get();
	if (cellTypesDLL.size() > 0)
	{
		for (auto ct : cellTypesDLL)
		{
			if (std::find(cellTypeIndices.begin(), cellTypeIndices.end(), ct.celltype_index) == cellTypeIndices.end())
			{
				if (!ct.retired)
				{
					cellTypeIndices.push_back(ct.celltype_index);
				}
			}
		}
	}

	// remove duplicates
	sort (cellTypeIndices.begin(), cellTypeIndices.end());
	cellTypeIndices.erase (unique(cellTypeIndices.begin(), cellTypeIndices.end()), cellTypeIndices.end());
}

// **************************************************************************
void ExpandedUser::FromDbStyle (DBApi::DB_UserRecord& dbUser)
{
	uuid = dbUser.UserId;
	displayName = dbUser.DisplayNameStr;
	displayDigits = dbUser.DecimalPrecision;
	email = dbUser.UserEmailStr;
	ADUser = dbUser.ADUser;
	allowFastMode = dbUser.AllowFastMode;
	exportFolder = dbUser.UserNameStr;
	lastLogin = dbUser.LastLogin;
	attemptCount = dbUser.AttemptCount;
	pwdChangeDate = dbUser.AuthenticatorDateTP;
	comment = dbUser.Comment;
	cellTypeIndices = dbUser.UserCellTypeIndexList;
	roleID = dbUser.RoleId;

	csvFolder = dbUser.CSVFolderStr;
	langCode = dbUser.LanguageCode;
	defaultResultFileNameStr = dbUser.DefaultResultFileNameStr;
	defaultSampleNameStr = dbUser.DefaultSampleNameStr;
	defaultImageSaveN = dbUser.UserImageSaveN;
	defaultWashType = dbUser.WashType;
	defaultDilution = dbUser.Dilution;
	defaultCellTypeIndex = dbUser.DefaultCellType;
	exportPdfEnabled = dbUser.PdfExport;

	// Ensure that ALL except Normal users have all the celltypes.
	RemoveInvalidCellTypes();
	if (userCore.PermisionLevel() != UserPermissionLevel::eNormal)
	{
		AddAnyMissingCellTypes();
		allowFastMode = true;
	}
	if (ADUser) 
	{
		if (!userCore.IsActive())
			userCore.SetActivation(true);
	}

	// Must have a display name, causes problems for the UI if not
	if (displayName.length() == 0)
	{
		displayName = dbUser.UserNameStr;
	}

	// Display digits uses a dropdown list and therefore must be one of those values
	if ((displayDigits < MIN_DISPLAY_DIGITS) || (displayDigits > MAX_DISPLAY_DIGITS))
	{
		displayDigits = MIN_DISPLAY_DIGITS;
	}

	// Not allowed to have an empty list of cell types
	if (cellTypeIndices.size() == 0)
	{
		cellTypeIndices.push_back(0);
	}

	// @todo - v1.4 add email address validation
	//if ((email.length() > 0) && !UserList::IsEmailValid_LatinLang(email))
	//	email = "";

	if (dbUser.UserAnalysisIndexList.size() == 0)
	{
		analysisIndices.push_back(0);
	}
	else
	{
		analysisIndices.clear();
		for (auto v : dbUser.UserAnalysisIndexList)
			analysisIndices.push_back(v);
	}

	if (dbUser.ColumnDisplayList.size() > 0)
	{
		uint16_t displayIdx = 0;
		displayColumns.clear();
		for (auto dc : dbUser.ColumnDisplayList)
		{
			DBApi::display_column_info_t dbCols = {};
			dbCols.ColumnType = static_cast<DBApi::eDisplayColumns>(dc.ColumnType);
			dbCols.OrderIndex = displayIdx++;
			dbCols.Visible = dc.Visible;
			dbCols.Width = dc.Width;
			displayColumns.push_back(dbCols);
		}
	}
	else
	{
		CreateDefaultColumns();
	}

};

// **************************************************************************
DBApi::DB_UserRecord ExpandedUser::ToDbStyle()
{
	DBApi::DB_UserRecord dbUser = {};

	dbUser.UserNameStr = userCore.UserName();
	dbUser.DisplayNameStr = displayName;
	dbUser.ADUser = ADUser;
	dbUser.UserEmailStr = email;
	dbUser.AllowFastMode = allowFastMode;
	dbUser.DecimalPrecision = static_cast<uint16_t>(displayDigits);
	dbUser.UserId = uuid;
	dbUser.LastLogin = lastLogin;
	dbUser.AttemptCount = static_cast<uint16_t>(attemptCount);
	dbUser.AuthenticatorDateTP = pwdChangeDate;
	dbUser.Comment = comment;
	dbUser.RoleId = roleID;
	dbUser.ExportFolderStr = exportFolder;

	dbUser.CSVFolderStr = csvFolder;
	dbUser.LanguageCode = langCode;
	dbUser.DefaultResultFileNameStr = defaultResultFileNameStr;
	dbUser.DefaultSampleNameStr = defaultSampleNameStr;
	dbUser.UserImageSaveN = defaultImageSaveN;
	dbUser.WashType = defaultWashType;
	dbUser.Dilution = defaultDilution;
	dbUser.DefaultCellType = defaultCellTypeIndex;
	dbUser.PdfExport = exportPdfEnabled;

	// Ensure we have a list of valid indices 
	RemoveInvalidCellTypes();
	if (userCore.PermisionLevel() != UserPermissionLevel::eNormal)
	{
		// Ensure all but normal users have all cell types
		AddAnyMissingCellTypes();
	}
	
	dbUser.UserCellTypeIndexList = cellTypeIndices;

	// Must have a display name, causes problems for the UI if not
	if (dbUser.DisplayNameStr.length() == 0)
	{
		dbUser.DisplayNameStr = userCore.UserName();
	}

	// Display digits uses a dropdown list and therefore must be one of those values
	if ((dbUser.DecimalPrecision < MIN_DISPLAY_DIGITS) || (dbUser.DecimalPrecision > MAX_DISPLAY_DIGITS))
	{
		dbUser.DecimalPrecision = MIN_DISPLAY_DIGITS;
	}

	// Not allowed to have an empty list of cell types
	if (dbUser.UserCellTypeIndexList.size() == 0)
	{
		dbUser.UserCellTypeIndexList.push_back(0);
		dbUser.NumUserCellTypes = 1;
	}

	// @todo - v1.4 add email address validation
	//if ((dbUser.UserEmailStr.length() > 0) && !UserList::IsEmailValid_LatinLang(dbUser.UserEmailStr))
	//	dbUser.UserEmailStr = "";

	if (analysisIndices.size() == 0)
	{
		dbUser.UserAnalysisIndexList.push_back(0);
	}
	else
	{
		dbUser.UserAnalysisIndexList.clear();
		for (auto v : analysisIndices)
			dbUser.UserAnalysisIndexList.push_back(v);
	}

	if (dbUser.ColumnDisplayList.size() == 0)
	{
		for (auto dc : displayColumns)
		{
			DBApi::display_column_info_t dbCols = {};
			dbCols.ColumnType = dc.ColumnType;
			dbCols.OrderIndex = dc.OrderIndex;
			dbCols.Visible = dc.Visible;
			dbCols.Width = dc.Width;
			dbUser.ColumnDisplayList.push_back(dbCols);
		}
	}
	else {
		for (auto newCS : displayColumns)
		{
			for (auto& currRec : dbUser.ColumnDisplayList)
			{
				if (newCS.ColumnType == currRec.ColumnType)
				{
					currRec.Visible = newCS.Visible;
					currRec.Width = newCS.Width;
					break;
				}
			}
		}
	}

	return dbUser;
}

// **************************************************************************
void ExpandedUser::CreateDefaultColumns()
{
	displayColumns.clear();
	DBApi::display_column_info_t dbCols = {};
	int orderidx = 0; // This is not currently used
	dbCols.ColumnType = DBApi::eDisplayColumns::SampleStatus; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 105;
	displayColumns.push_back(dbCols);
	dbCols.ColumnType = DBApi::eDisplayColumns::SamplePosition; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 70;
	displayColumns.push_back(dbCols);

	// This is displayed as Sample ID in the UI
	dbCols.ColumnType = DBApi::eDisplayColumns::SampleName; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 125;
	displayColumns.push_back(dbCols);

	// now ColumnOption::eTotalConcentration @todo 1.4 change DB names - this is just an order change
	dbCols.ColumnType = DBApi::eDisplayColumns::TotalCells; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 115;
	displayColumns.push_back(dbCols);

	// Now ColumnOption::eViableConcentration @todo 1.4 change DB names to match
	dbCols.ColumnType = DBApi::eDisplayColumns::TotalViableCells; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 115;
	displayColumns.push_back(dbCols);

	dbCols.ColumnType = DBApi::eDisplayColumns::TotalViability; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 70;
	displayColumns.push_back(dbCols);

	// Now eTotalCells @todo 1.4 change DB order to match
	dbCols.ColumnType = DBApi::eDisplayColumns::CellTypeQcName; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 65;
	displayColumns.push_back(dbCols);

	// Now	eAverageDiameter  @todo 1.4 change DB order to match
	dbCols.ColumnType = DBApi::eDisplayColumns::Dilution; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 85;
	displayColumns.push_back(dbCols);

	// Now eCellTypeQcName, @todo 1.4 change DB order to match
	dbCols.ColumnType = DBApi::eDisplayColumns::AverageDiameter; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 125;
	displayColumns.push_back(dbCols);

	// Now eDilution  @todo 1.4 change DB order to match
	dbCols.ColumnType = DBApi::eDisplayColumns::AverageViableDiameter; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 85;
	displayColumns.push_back(dbCols);

	dbCols.ColumnType = DBApi::eDisplayColumns::WashType; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 130;
	displayColumns.push_back(dbCols);
	dbCols.ColumnType = DBApi::eDisplayColumns::SampleTag; dbCols.OrderIndex = orderidx++; dbCols.Visible = true; dbCols.Width = 120;
	displayColumns.push_back(dbCols);
}
