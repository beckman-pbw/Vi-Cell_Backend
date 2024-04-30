#include "stdafx.h"

#include <DBif_Api.h>

#include "ActiveDirectoryDLL.hpp"
#include "AppConfig.hpp"
#include "AuditLog.hpp"
#include "ExpandedUsers.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImportExportConfiguration.hpp"
#include "InstrumentConfig.hpp"

using namespace SecurityHelpers;

static const char MODULENAME[] = "Configuration";

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetSystemSecurityType (eSECURITYTYPE secType, const char* uname, const char* password)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "SetSystemSecurityFeaturesState: <enter>");

	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			uname,
			audit_event_type::evt_notAuthorized, 
			"SetSystemSecurityType"));
		return HawkeyeError::eNotPermittedByUser;
	}

	eSECURITYTYPE oldSecType = UserList::GetSystemSecurityType();
	int16_t expDays = HawkeyeConfig::Instance().get().passwordExpiration;
	std::string unameStr = uname;

	// changing to local security mode; check to re-enable password expiration
	if ( eSECURITYTYPE::eLocalSecurity == secType )
	{
		if ( expDays < NO_PWD_EXPIRATION )
		{
			HawkeyeConfig::Instance().get().passwordExpiration = -( expDays );		// restore the password expiration days value
		}
	}
	else if ( eSECURITYTYPE::eLocalSecurity == oldSecType )		// going to No-Security mode or AD mode; disable password expiration on local security 
	{
		if ( expDays > NO_PWD_EXPIRATION )
		{
			HawkeyeConfig::Instance().get().passwordExpiration = -( expDays );		// preserve the password expiration days value as a negative, below the level detected for NO_PWD_EXPIRATION
		}

		std::string factoryAdmin = FACTORY_ADMIN_USER;
		std::string faPwd = SILENTADMIN_USER;				// -> "Vi-CELL"
		faPwd.append( "#0" );								// -> "Vi-CELL#0"

		HawkeyeError he = UserList::Instance().ChangeUserPassword( factoryAdmin, faPwd, true );
		faPwd.clear();

		if ( he == HawkeyeError::eSuccess )
		{
			AuditLogger::L().Log( generateAuditWriteData(
				unameStr,
				audit_event_type::evt_passwordreset,
				"Password reset\n\tUser: \"" + factoryAdmin + "\"" ) );
		}
		else
		{
			Logger::L().Log( MODULENAME, severity_level::notification, boost::str(
				boost::format( "SetsecurityType : Unable to reset password for user <with name : %s> from logged in User : %s" )
				% factoryAdmin
				% unameStr ) );
		}
	}

	HawkeyeConfig::Instance().get().securityType = secType;

	audit_event_type event_type = (secType == eSECURITYTYPE::eNoSecurity) ? audit_event_type::evt_securitydisable : audit_event_type::evt_securityenable;
	std::string sec_type_str = "";
	if (secType == eSECURITYTYPE::eActiveDirectory)
		sec_type_str = "ActiveDir";
	else if (secType == eSECURITYTYPE::eLocalSecurity)
		sec_type_str = "Local";
	else
		sec_type_str = "Disabled";

	AuditLogger::L().Log (generateAuditWriteData(
		uname,
		event_type,
		"System security type set to " + sec_type_str + " by "+ std::string(uname)));

	LogoutConsoleUser();

	// We just changed security types, reload the user list for the new security type
	UserList::Instance().Initialize (false);

	Logger::L().Log (MODULENAME, severity_level::debug2, "SetSystemSecurityFeaturesState: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
eSECURITYTYPE HawkeyeLogicImpl::GetSystemSecurityType()
{	
	return UserList::GetSystemSecurityType();
}

//*****************************************************************************
// Export Analysis.info, CellType.info and UserList::Instance().info.
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ExportInstrumentConfiguration (const char* username, const char* password, const char* configfilename)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ExportInstrumentConfiguration: <enter, export filename: %s>") % std::string(configfilename)));

	/*
	Security ON: Instrument Admin only
	Security OFF: Any Logged In User
	*/
	auto permissionLevel = (HawkeyeConfig::Instance().get().securityType != eSECURITYTYPE::eNoSecurity) ? UserPermissionLevel::eNormal : UserPermissionLevel::eAdministrator;

	if (!UserList::Instance().IsUserPermissionAtLeast(username, permissionLevel))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized, 
			"Export Instrument Configuration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	std::string configInfoFilename;
	DataConversion::convertToStandardString (configInfoFilename, configfilename);

	// Validate File Path before doing anything else.
	auto configFileExtn = HawkeyeDirectory::Instance().getConfigFileExtension();
	if (!FileSystemUtilities::ValidateFileNameAndPath (configInfoFilename) ||
		!FileSystemUtilities::VerifyUserAccessiblePath (configInfoFilename) ||
		!FileSystemUtilities::CheckFileExtension (configInfoFilename, configFileExtn))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ExportInstrumentConfiguration: File path/Filename/File Extension is Invalid");
		return HawkeyeError::eInvalidArgs;
	}

	// Get the Configuration data
	boost::property_tree::ptree configurationDataTree;
	auto he = ImportExportConfiguration::getExportConfigurationData (username, configurationDataTree);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ExportInstrumentConfiguration: Failed to get the Configuration Data");
		return he;
	}

	// Change the input filename to ".info" extension so that the file is written in INFO format.
	configInfoFilename = std::regex_replace (configInfoFilename, std::regex(".cfg"), ".info");

	if (!HDA_WriteEncryptedPtreeFile (configurationDataTree, configInfoFilename))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ExportInstrumentConfiguration: Failed to encrypt export configuration tree");
		ReportSystemError::Instance().ReportUserError(BuildErrorInstance(
			instrument_error::instrument_storage_writeerror, 
			instrument_error::instrument_storage_instance::instrument_configuration, 
			instrument_error::severity_level::error), username);
		return HawkeyeError::eStorageFault;
	}

	// Since backend saves encrypted file with extension ("*.ecfg") but UI and user
	// are not aware of this.  So change the extension back to ("*.cfg").
	{
		std::string encryptedFileName;
		bool success = HDA_GetEncryptedFileName (configInfoFilename, encryptedFileName);
		success = success && FileSystemUtilities::FileExists(encryptedFileName);
		if (!success)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ExportInstrumentConfiguration: Failed to create encrypted configuration file name");
			ReportSystemError::Instance().ReportUserError(BuildErrorInstance(
				instrument_error::instrument_storage_filenotfound,
				instrument_error::instrument_storage_instance::instrument_configuration,
				instrument_error::severity_level::error), username);
			return HawkeyeError::eStorageFault;
		}

		boost::system::error_code ec;
		boost::filesystem::rename (encryptedFileName, configfilename, ec);
		if (ec)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ExportInstrumentConfiguration: Failed to rename encrypted configuration file : " + ec.message());
			return HawkeyeError::eStorageFault;
		}
	}

	AuditLogger::L().Log (generateAuditWriteData(
		username,
		audit_event_type::evt_instrumentconfigexported, 
		"Exported Instrument Configuration"));
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "ExportInstrumentConfiguration: <Exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// Import Analysis.info, CellType.info and UserList::Instance().info.
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ImportInstrumentConfiguration(const char* username, const char* password, const char* configFilename)
{ // v1.3.cfg input file...

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ImportInstrumentConfiguration: <enter, import filename: %s>") % configFilename));
	
	if (!UserList::Instance().IsUserPermissionAtLeastAndNotService(username, UserPermissionLevel::eAdministrator)) {
		AuditLogger::L().Log (generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized, 
			"Import Instrument Configuration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (std::string(configFilename).empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportInstrumentConfiguration: Null or empty configuration file name");
		return HawkeyeError::eStorageFault;
	}

	std::string configInfoFilename;
	DataConversion::convertToStandardString (configInfoFilename, configFilename);

	// Change the input filename to ".info" extension so that the file is read in Info format.
	configInfoFilename = std::regex_replace (configInfoFilename, std::regex(".cfg"), ".info");

	// Since backend saves encrypted file with extension ("*.ecfg") but UI and user
	// are not aware of this.  So change the extension to ("*.einfo") to read it properly.
	std::string encryptedFileName;
	if (!HDA_GetEncryptedFileName (configInfoFilename, encryptedFileName))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportInstrumentConfiguration: <exit, failed to get encrypted configuration filename>");
		ReportSystemError::Instance().ReportUserError(BuildErrorInstance(
			instrument_error::instrument_storage_filenotfound, 
			instrument_error::instrument_storage_instance::instrument_configuration, 
			instrument_error::severity_level::error), username);
		return HawkeyeError::eStorageFault;
	}

	// Make sure there is no old files around.
	boost::filesystem::remove (encryptedFileName);

	// Copy the original encrypted file to "encryptedFileName" (with appended *.ecfg)
	// Here we have to pass actual file name input to method instead of "inputFileName"
	// encryptedFileName, *.einfo.
	boost::filesystem::copy (configFilename, encryptedFileName);

	boost::property_tree::ptree importConfigTree;
	if (!HDA_ReadEncryptedPtreeFile (configInfoFilename, importConfigTree))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportInstrumentConfiguration: Failed to read the Configuration Data tree from configuration file : " + encryptedFileName);
		ReportSystemError::Instance().ReportUserError(BuildErrorInstance(
			instrument_error::instrument_storage_readerror, 
			instrument_error::instrument_storage_instance::instrument_configuration, 
			instrument_error::severity_level::error), username);
		// Invalid encrypted file. Now delete the copied file
		boost::filesystem::remove(encryptedFileName);
		return HawkeyeError::eStorageFault;
	}

	// Once the pTree is loaded, delete the copied file
	boost::filesystem::remove (encryptedFileName);

	auto he = ImportExportConfiguration::setImportConfigurationData (importConfigTree);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportInstrumentConfiguration: Failed to set the Configuration Data tree");
		return he;
	}

	AuditLogger::L().Log (generateAuditWriteData(
		username,
		audit_event_type::evt_instrumentconfigimported, 
		"Imported Instrument Configuration", 
		ImportExportConfiguration::GetGeneralInformation()));

	Logger::L().Log (MODULENAME, severity_level::debug1, "ImportInstrumentConfiguration: <Exit>");
	
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ValidateActiveDirConfig (ActiveDirectoryConfig cfg, const char* adminGroup, const char* uName, const char* password, bool& valid)
{
	valid = false;
	// The currently logged in user must be an administrator
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"ValidateActiveDirConfig"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (std::string(uName) == std::string(SERVICE_USER))
	{
		return HawkeyeError::eNotPermittedByUser;
	}

	// Log in to the server and if OK we get a list of strings including all the groups the user belongs to 
	std::vector<std::string> adGroups = {};
	ActiveDirectoryConfigDLL adConfig( cfg );
	UserList::Instance().LoginADUser( adGroups, uName, password, adConfig );

	for (const auto& group : adGroups)
	{
		if (group._Equal(adminGroup))
		{
			valid = true;
			return HawkeyeError::eSuccess;
		}
	}
	return HawkeyeError::eEntryInvalid;
}

//*****************************************************************************
void HawkeyeLogicImpl::SetActiveDirConfig (ActiveDirectoryConfig adConfig)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "SetActiveDirConfig: <enter>");

	ActiveDirectoryConfigDLL adConfigDLL = {};
	adConfigDLL = adConfig;

	HawkeyeError he = UserList::Instance().SetActiveDirectoryConfig (adConfigDLL);
	
	Logger::L().Log(MODULENAME, severity_level::debug2, "SetActiveDirConfig: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::GetActiveDirConfig (ActiveDirectoryConfig*& adConfig)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "GetActiveDirConfig: <enter>");

	adConfig = nullptr;

	ActiveDirectoryConfigDLL adConfigDLL = {};
	HawkeyeError he = UserList::Instance().GetActiveDirectoryConfig (adConfigDLL);
	if (HawkeyeError::eSuccess != he)
	{
		return;
	}

	adConfig = new ActiveDirectoryConfig();
	*adConfig = adConfigDLL.ToCStyle();

	Logger::L().Log( MODULENAME, severity_level::debug2, "GetActiveDirConfig: <exit>" );
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeActiveDirConfig (ActiveDirectoryConfig* cfg)
{
	//Logger::L().Log(MODULENAME, severity_level::debug1, "FreeActiveDirConfig: <enter>");

	if (cfg == NULL)
		return;

	if (cfg->domain != NULL) 
	{
		delete[] cfg->domain;
		cfg->domain = NULL;
	}

	if (cfg->server != NULL)
	{
		delete[] cfg->server;
		cfg->server = NULL;
	}

	delete cfg;
	cfg = NULL;
}

//*****************************************************************************
void HawkeyeLogicImpl::GetActiveDirectoryGroupMaps (ActiveDirectoryGroup*& adGroups, uint32_t& retrieved_count)
{
	Logger::L().Log( MODULENAME, severity_level::debug2, "GetActiveDirectoryGroupMaps: <enter>" );

	adGroups = nullptr;
	retrieved_count = 0;

	std::vector<ActiveDirectoryGroupDLL> adGroupsDLL = {};
	HawkeyeError he = UserList::Instance().GetActiveDirectoryGroupMaps (adGroupsDLL);
	if (HawkeyeError::eSuccess != he)
	{
		return;
	}

	adGroups = new ActiveDirectoryGroup[adGroupsDLL.size()];
	DataConversion::convert_vecOfDllType_to_listOfCType (adGroupsDLL, adGroups, retrieved_count);

	Logger::L().Log(MODULENAME, severity_level::debug2, "GetActiveDirectoryGroupMaps: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::SetActiveDirectoryGroupMaps (ActiveDirectoryGroup* adGroups, uint32_t count)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "SetActiveDirectoryGroupMaps: <enter>");
	
	if (adGroups == NULL)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "SetActiveDirectoryGroupMaps: <exit, no groups specified>");
		return;
	}

	std::vector<ActiveDirectoryGroupDLL> adGroupsDLL = {};
	DataConversion::convert_listOfCType_to_vecOfDllType (adGroups, count, adGroupsDLL);

	HawkeyeError he = UserList::Instance().SetActiveDirectoryGroupMaps (adGroupsDLL);

	Logger::L().Log(MODULENAME, severity_level::debug2, "SetActiveDirectoryGroupMaps: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeActiveDirGroupMaps (ActiveDirectoryGroup* recs, uint32_t count)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "FreeActiveDirGroupMaps: <enter>");

	if (recs == NULL)
		return;

	for (uint32_t j = 0; j < count; j++)
	{
		delete[] recs[j].group;
		recs[j].group = NULL;
	}

	delete[] recs;
	recs = NULL;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetDBConfig (DBConfig cfg)
{
	//Logger::L().Log(MODULENAME, severity_level::debug1, "SetDBConfig: <enter>");

	HawkeyeConfig::Database_t dbProps = HawkeyeConfig::Instance().get().database;
	dbProps.dbName = std::string(cfg.name);
	dbProps.ipAddr = std::string(cfg.ipaddr);
	dbProps.ipPort = std::to_string(cfg.port);

	//Logger::L().Log(MODULENAME, severity_level::debug1, "SetDBConfig: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetDBConfig (DBConfig*& cfg)
{
	//Logger::L().Log(MODULENAME, severity_level::debug1, "GetDBConfig: <enter>");

	cfg = new DBConfig();
	HawkeyeConfig::Database_t dbProps = HawkeyeConfig::Instance().get().database;

	DataConversion::convertToCharPointer (cfg->name, dbProps.dbName);
	DataConversion::convertToCharPointer (cfg->ipaddr, dbProps.ipAddr);
	cfg->port = atoi (dbProps.ipPort.get().c_str());

	//Logger::L().Log(MODULENAME, severity_level::debug1, "GetDBConfig: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeDBConfig(DBConfig* cfg)
{
//	Logger::L().Log(MODULENAME, severity_level::debug1, "FreeDBConfig: <enter>");

	if (cfg == NULL) return;

	if (cfg->ipaddr != NULL)
	{
		delete[] cfg->ipaddr;
		cfg->ipaddr = NULL;
	}

	if (cfg->name != NULL)
	{
		delete[] cfg->name;
		cfg->name = NULL;
	}

	delete cfg;
	cfg = NULL;

	//	Logger::L().Log(MODULENAME, severity_level::debug1, "FreeDBConfig: <exit>");
}

// **************************************************************************
HawkeyeError HawkeyeLogicImpl::SetDbBackupUserPassword( const char* password )
{
	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	if ( nullptr == password || std::string( password ).empty() )
	{
		Logger::L().Log( MODULENAME, severity_level::error, "SetDbBackupUserPassword : Empty password" );
		return HawkeyeError::eInvalidArgs;
	}

	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eAdministrator );
	if ( status != HawkeyeError::eSuccess )
	{
		// If no user is not logged in 
		if ( status == HawkeyeError::eNotPermittedAtThisTime )
			return status;

		AuditLogger::L().Log( generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized,
			"DB Backup User Password set" ) );
		Logger::L().Log( MODULENAME, severity_level::warning, boost::str(
			boost::format( "Restricted Access : Invalid attempt to change password for DB Backup user from logged in User : %s" )
			% loggedInUsername ) );

		return status;
	}

	HawkeyeError he = HawkeyeError::eSuccess;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;

	std::string pwdStr( password );

	if ( DBApi::DbSetBackupUserPwd( queryResult, pwdStr ) && queryResult == DBApi::eQueryResult::QueryOk )
	{
		AuditLogger::L().Log( generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_passwordchange,
			"DB Backup User Password set" ) );
	}
	else
	{
		Logger::L().Log( MODULENAME, severity_level::notification, boost::str(
			boost::format( "SetDbBackupUserPassword : Unable to change password for DB Backup user from logged in User : %s" )
			% loggedInUsername ) );
	}

	switch ( queryResult )
	{
		case DBApi::eQueryResult::QueryOk:
			he = HawkeyeError::eSuccess;
			break;

		case DBApi::eQueryResult::MissingQueryKey:
			he = HawkeyeError::eInvalidArgs;
			break;

		case DBApi::eQueryResult::NotConnected:
			he = HawkeyeError::eSoftwareFault;
			break;

		case DBApi::eQueryResult::QueryFailed:
		default:
			he = HawkeyeError::eDatabaseError;
			break;
	}

	return he;
}

// *****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetSMTPConfig (SMTPConfig cfg)
{
//	Logger::L().Log(MODULENAME, severity_level::debug1, "SetSMTPConfig: <enter>");

	DBApi::DB_InstrumentConfigRecord& dbCfg = InstrumentConfig::Instance().Get();

	dbCfg.Email_Settings.server_addr = cfg.server;
	dbCfg.Email_Settings.port_number = cfg.port;
	dbCfg.Email_Settings.username = cfg.username;
	dbCfg.Email_Settings.authenticate = cfg.auth_enabled;
	dbCfg.Email_Settings.pwd_hash = "";

	if (cfg.password && (strlen(cfg.password) > 0))
	{
		char* phash = nullptr;
		if (HDA_EncryptString (cfg.password, phash))
		{
			dbCfg.Email_Settings.pwd_hash = std::string(phash);
			HDA_FreeCharBufferData (phash);
		}
		else
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_integrity_softwarefault,
				instrument_error::instrument_storage_instance::instrument_configuration,
				instrument_error::severity_level::warning));
			Logger::L().Log(MODULENAME, severity_level::error, "SetSMTPConfig: <exit, failed to encrypt password>");
			return HawkeyeError::eStorageFault;
		}

	}

	InstrumentConfig::Instance().Set();

//	Logger::L().Log(MODULENAME, severity_level::debug1, "SetSMTPConfig: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetSMTPConfig (SMTPConfig*& cfg)
{
//	Logger::L().Log(MODULENAME, severity_level::debug1, "GetSMTPConfig: <enter>");

	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();

	cfg = new SMTPConfig();
	cfg->port = static_cast<uint32_t>(instConfig.Email_Settings.port_number);
	DataConversion::convertToCharPointer (cfg->server, instConfig.Email_Settings.server_addr );
	DataConversion::convertToCharPointer (cfg->username, instConfig.Email_Settings.username );
	cfg->auth_enabled = instConfig.Email_Settings.authenticate;

	if (instConfig.Email_Settings.pwd_hash.length() > 0)
	{
		char* plaintext = nullptr;
		if (HDA_DecryptString(instConfig.Email_Settings.pwd_hash.c_str(), plaintext))
		{
			DataConversion::convertToCharPointer (cfg->password, std::string(plaintext));
			HDA_FreeCharBufferData (plaintext);
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "GetSMTPConfig: <failed to decrypt password>");
			return HawkeyeError::eStorageFault;
		}
	}

//	Logger::L().Log( MODULENAME, severity_level::debug1, "GetSMTPConfig: <exit>" );

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSMTPConfig(SMTPConfig* cfg)
{
//	Logger::L().Log(MODULENAME, severity_level::debug1, "FreeSMTPConfig: <enter>");

	if (cfg == NULL) return;
	if (cfg->username != NULL)
	{
		delete[] cfg->username;
		cfg->username = NULL;
	}

	if (cfg->password != NULL)
	{
		delete[] cfg->password;
		cfg->password = NULL;
	}

	if (cfg->server != NULL)
	{
		delete[] cfg->server;
		cfg->server = NULL;
	}

	delete cfg;
	cfg = NULL;

//	Logger::L().Log(MODULENAME, severity_level::debug1, "FreeSMTPConfig: <exit>");
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetAutomationSettings (AutomationConfig cfg)
{
//	Logger::L().Log(MODULENAME, severity_level::debug1, "SetAutomationSettings: <enter>");

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	UserPermissionLevel userPermission;
	UserList::Instance().GetUserPermissionLevel (loggedInUsername, userPermission);

	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();

	// Service can turn automation mode off or on.
	if (userPermission == UserPermissionLevel::eService)
	{
		if ( !instConfig.AutomationInstalled && cfg.automationIsEnabled )
		{ // if enabling, set the installed flag to know that it has been turned on at least once.
			instConfig.AutomationInstalled = true;
		}

		if (!cfg.automationIsEnabled)
		{ // if not enabled, don't allow Acup to be enabled
			cfg.acupIsEnabled = false;
		}
		instConfig.AutomationEnabled = cfg.automationIsEnabled;
		instConfig.ACupEnabled = cfg.acupIsEnabled;
	}

	// Admin can only turn off automation mode...  if it was ever turned on.
	else if (userPermission == UserPermissionLevel::eAdministrator)
	{
		if (instConfig.AutomationInstalled && !cfg.automationIsEnabled)
		{
			instConfig.AutomationEnabled = cfg.automationIsEnabled;
		}
		if (instConfig.AutomationInstalled && !cfg.acupIsEnabled)
		{
			instConfig.ACupEnabled = cfg.acupIsEnabled;
		}
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1, 
			"SetAutomationSettings: <exit, <not permitted, only admin and service may modify automation settings>");
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			"Modify Automation Settings"));
		return HawkeyeError::eNotPermittedByUser;
	}

	instConfig.AutomationPort = cfg.port;

	InstrumentConfig::Instance().Set();

	{
		auto str = std::string (cfg.automationIsEnabled ? "Enabled, port: " + std::to_string(cfg.port) : "Disabled");
		Logger::L ().Log (MODULENAME, severity_level::debug1, "SetAutomationSettings: <automation " + str + ">");
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_automation,
			str));
	}
	{
		auto str = std::string (cfg.acupIsEnabled ? "Enabled" : "Disabled");
		Logger::L ().Log (MODULENAME, severity_level::debug1, "SetAutomationSettings: <automation " + str + ">");
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_acup,
			str));
	}


//	Logger::L().Log(MODULENAME, severity_level::debug1, "SetAutomationSettings: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetAutomationSettings (AutomationConfig*& cfg)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "GetAutomationSettings: <enter>");
	
	cfg = new AutomationConfig();
	cfg->port = HawkeyeConfig::Instance().get().automationPort;
	cfg->automationIsEnabled = HawkeyeConfig::Instance().get().automationEnabled;
	cfg->acupIsEnabled = HawkeyeConfig::Instance().get().acupEnabled;

	Logger::L().Log(MODULENAME, severity_level::debug2, "GetAutomationSettings: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeAutomationSettings (AutomationConfig* cfg)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "FreeAutomationSettings: <enter>");

	if (cfg == NULL) return;
	delete cfg;
	cfg = NULL;
	
	Logger::L().Log(MODULENAME, severity_level::debug2, "FreeAutomationSettings: <exit>");
}

HawkeyeError HawkeyeLogicImpl::SetOpticalHardwareConfig(OpticalHardwareConfig type)
{
	return InstrumentConfig::Instance().setOpticalHardwareConfig(type);
}

OpticalHardwareConfig HawkeyeLogicImpl::GetOpticalHardwareConfig() const
{
	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();
	const auto beginIter = std::begin(HawkeyeConfig::OpticalHardwarePairings);
	const auto endIter = std::end(HawkeyeConfig::OpticalHardwarePairings);
	auto config = std::find_if(beginIter, endIter,
		[=](const HawkeyeConfig::OpticalHardwareTypePairing pair)
		{
			return (pair.hardware.camera == instConfig.CameraType && pair.hardware.brightfieldLed == instConfig.BrightFieldLedType);
		});
	if (config == endIter) return OpticalHardwareConfig::UNKNOWN;
	return config->type;
}

