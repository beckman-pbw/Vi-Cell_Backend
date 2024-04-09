#include "stdafx.h"

#include "AuditLog.hpp"
#include "DataConversion.hpp"
#include "DBif_Api.h"
#include "HawkeyeDataAccess.h"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "PtreeConversionHelper.hpp"
#include "SystemErrors.hpp"

#include "../../target/properties/HawkeyeCore/dllversion.hpp"

static const char MODULENAME[] = "InstrumentConfig";

// Default instrument database connectivity settings.
static HawkeyeConfig::DatabaseConfig_t databaseConfig = { "127.0.0.1", "5432", "ViCellDB" };

//*****************************************************************************
bool InstrumentConfig::Initialize()
{
	if (isInitialized_)
	{
		return true;
	}

	HawkeyeConfig::Instance().SetDatabaseConfig (databaseConfig);

	if (!InitializeDatabase())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Initialize: <exit, failed to connect to database>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::instrument_configuration,
			instrument_error::severity_level::warning));
		return false;
	}

	// Get the default record.
	dbInstConfig_.InstrumentSNStr = "";
	dbInstConfig_.InstrumentIdNum = 1;

	DBApi::eQueryResult dbStatus = DBApi::DbFindInstrumentConfig (dbInstConfig_);
	if (dbStatus != DBApi::eQueryResult::QueryOk) {
		Logger::L().Log(MODULENAME, severity_level::error, "Initialize: <exit, DbFindInstrumentConfig failed>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::instrument_configuration,
			instrument_error::severity_level::warning));
		return false;
	}

	// Set default level to Normal.  May be overrided by setting in InstrumentConfig.
	severity_level severityLevel = severity_level::normal;
	// "severityLevel" is not modified if the string is invalid.
	Logger::L().StringToSeverityLevel (severityLevel, dbInstConfig_.LogSensitivity);


#ifndef DATAIMPORTER
	LoggerSettings_t loggerSettings = {
		dbInstConfig_.LogName, 
		severityLevel, 
		static_cast<size_t>(dbInstConfig_.LogMaxSize),
		static_cast<size_t>(dbInstConfig_.MaxLogs),
		dbInstConfig_.AlwaysFlush, 
		false };

	boost::system::error_code ec;
	Logger::L().Initialize (ec, loggerSettings);
#endif

	if (!HawkeyeConfig::Instance().Initialize())
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "internalInitialize: <exit: configuration failed to load>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::configStatic,
			instrument_error::severity_level::error));
		AuditLogger::L().Log (generateAuditWriteData(
			"",
			audit_event_type::evt_instrumentconfignotfound,
			"Main configuration"));
		
		return false;
	}

	isInitialized_ = true;

	return true;
}

//*****************************************************************************
DBApi::DB_InstrumentConfigRecord& InstrumentConfig::Get()
{
	std::lock_guard<std::mutex> guard(instrumentConfigMutex_);
	return dbInstConfig_;
}

void InstrumentConfig::setLedType(HawkeyeConfig::BrightfieldLedType type)
{
	dbInstConfig_.BrightFieldLedType = static_cast<int16_t>(type);
	Set();
}

HawkeyeError InstrumentConfig::setOpticalHardwareConfig(OpticalHardwareConfig type)
{
	const auto beginIter = std::begin(HawkeyeConfig::OpticalHardwarePairings);
	const auto endIter = std::end(HawkeyeConfig::OpticalHardwarePairings);
	const auto config = std::find_if(beginIter, endIter,
		[=](const HawkeyeConfig::OpticalHardwareTypePairing pair)
		{
			return pair.type == type;
		});
	if (config == endIter) return HawkeyeError::eInvalidArgs;
	dbInstConfig_.BrightFieldLedType = static_cast<int16_t>(config->hardware.brightfieldLed);
	dbInstConfig_.CameraType = static_cast<int16_t>(config->hardware.camera);
	const auto illuminatorInfoIter =
		std::find_if(dbInstConfig_.IlluminatorsInfoList.begin(), dbInstConfig_.IlluminatorsInfoList.end(), [](DBApi::illuminator_info info)
			{
				return info.type == static_cast<int16_t>(HawkeyeConfig::LedType::LED_BrightField);
			});
	//Did we find the entry to update?
	if (illuminatorInfoIter != dbInstConfig_.IlluminatorsInfoList.end())
	{
		//Find the InstrumentConfig for this type
		std::vector<DBApi::DB_IlluminatorRecord> records;
		DBApi::DbGetIlluminatorsList(records, DBApi::eListFilterCriteria::IlluminatorType, "=", std::to_string(dbInstConfig_.BrightFieldLedType));
		//Did we find the illuminator record for it?
		if (!records.empty())
		{
			illuminatorInfoIter->index = records[0].IlluminatorIndex;
		};
	}
	Logger::L().Log(MODULENAME, severity_level::notification, "Changing hardware type to " +
		std::to_string(static_cast<int32_t>(type)) +
		", which is camera " +
		std::to_string(dbInstConfig_.CameraType) +
		"and brightfield led type" +
		std::to_string(dbInstConfig_.BrightFieldLedType));
	return Set() ? HawkeyeError::eSuccess : HawkeyeError::eDatabaseError;

}

//*****************************************************************************
bool InstrumentConfig::Set()
{
	std::lock_guard<std::mutex> guard(instrumentConfigMutex_);
	DBApi::eQueryResult dbStatus = DBApi::DbModifyInstrumentConfig (dbInstConfig_);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Update: <exit, DbModifyInstrumentConfig failed>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::instrument_configuration,
			instrument_error::severity_level::warning));
		return false;
	}

	return true;
}

//*****************************************************************************
bool InstrumentConfig::InitializeDatabase()
{
	if (DBApi::DBifInit())
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Initialized Database connection.");

		bool dbPropsOk = false;
		HawkeyeConfig::Database_t dbProps = HawkeyeConfig::Instance().get().database;
		std::string dbAddrStr = dbProps.ipAddr;
		std::string dbPortStr = dbProps.ipPort;
		std::string dbNameStr = dbProps.dbName;

		if (dbAddrStr.length() == 0)
		{ // OK to use for normal internal instrument DB connection.
			dbPropsOk = DBApi::DbSetDefaultConnection();
		}
		else
		{ // use when connection properties are specified.
			dbPropsOk = DBApi::DbSetFullConnection (dbAddrStr, dbPortStr, dbNameStr, "");
		}

		std::string dbPropsLogStr = boost::str(boost::format("\n\tDatabase IP: %s\n\tDatabase Port: %s\n\tDatabase Name: %s") 
			% dbAddrStr % dbPortStr % dbNameStr);

		if (dbPropsOk)
		{
			Logger::L().Log(MODULENAME, severity_level::normal, boost::str(boost::format("Set Database connection properties: %s") % dbPropsLogStr));
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, boost::str(boost::format("Failed to initialize database connection using: %s") % dbPropsLogStr));
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_precondition_notmet,
				instrument_error::instrument_precondition_instance::database,
				instrument_error::severity_level::error));
			return false;
		}

		if (DBApi::IsDBPresent())
		{
			Logger::L().Log(MODULENAME, severity_level::normal, "Database is available");
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "Database connection failed");
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_precondition_notmet,
				instrument_error::instrument_precondition_instance::database,
				instrument_error::severity_level::error));
			return false;
		}
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Failed to initialize database connection");
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::database,
			instrument_error::severity_level::error));
		return false;
	}

	return true;
}

//*****************************************************************************
std::pair<std::string, boost::property_tree::ptree> InstrumentConfig::Export(bool disableSecurity)
{
	boost::property_tree::ptree pt_InstConfig = {};

	DBApi::DB_InstrumentConfigRecord&  dbInstConfig = InstrumentConfig::Instance().Get();

	pt_InstConfig.put (SERIAL_NUMBER_NODE, dbInstConfig.InstrumentSNStr);
	pt_InstConfig.put (DEVICE_NAME_NODE, dbInstConfig.DeviceName);

	//LH6531-6532 - For Cell Health Module, disable security on export of the configuration.
	if (disableSecurity)
	{
		pt_InstConfig.put(SECURITY_MODE_NODE, (int16_t)eSECURITYTYPE::eNoSecurity);
	}
	else
	{
		pt_InstConfig.put(SECURITY_MODE_NODE, dbInstConfig.SecurityMode);
	}
	pt_InstConfig.put (INACTIVITY_TIMEOUT_NODE, dbInstConfig.InactivityTimeout);
	pt_InstConfig.put (PASSWORD_EXPIRATION_NODE, dbInstConfig.PasswordExpiration);

	boost::property_tree::ptree smtpConfig;
	smtpConfig.put (SERVER_ADDRESS_NODE, dbInstConfig.Email_Settings.server_addr);
	smtpConfig.put (PORT_NODE, dbInstConfig.Email_Settings.port_number);
	smtpConfig.put (USERNAME_NODE, dbInstConfig.Email_Settings.username);
	smtpConfig.put (AUTHENTICATE_NODE, dbInstConfig.Email_Settings.authenticate);
	smtpConfig.put (PW_HASH_NODE, dbInstConfig.Email_Settings.pwd_hash);
	pt_InstConfig.put_child (SMTP_CONFIG_NODE, smtpConfig);

	boost::property_tree::ptree adConfig;
	adConfig.put (BASE_DOMAIN_NODE, dbInstConfig.AD_Settings.base_dn);
	adConfig.put (SERVER_NAME_NODE, dbInstConfig.AD_Settings.servername);
	adConfig.put (SERVER_ADDRESS_NODE, dbInstConfig.AD_Settings.server_addr);
	adConfig.put (PORT_NODE, dbInstConfig.AD_Settings.port_number);
	adConfig.put (ENABLED_NODE, dbInstConfig.AD_Settings.enabled);
	pt_InstConfig.put_child (AD_CONFIG_NODE, adConfig);

	boost::property_tree::ptree automationConfig;
	automationConfig.put (INSTALLED_NODE, dbInstConfig.AutomationInstalled);
	automationConfig.put (ENABLED_NODE, dbInstConfig.AutomationEnabled);
	automationConfig.put (ACUP_ENABLED_NODE, dbInstConfig.ACupEnabled);
	automationConfig.put (PORT_NODE, dbInstConfig.AutomationPort);
	pt_InstConfig.put_child (AUTOMATION_CONFIG_NODE, automationConfig);

	return std::make_pair (INSTRUMENT_NODE, pt_InstConfig);
}

//*****************************************************************************
void InstrumentConfig::Import (boost::property_tree::ptree& ptConfig)
{
	DBApi::DB_InstrumentConfigRecord& importInstConfig = InstrumentConfig::Instance().Get();

	// DO NOT modify the serial number on import
	// In the future, we may need to apply the serial number for import	
	// for v1.3 we are not attrempting to future proof

	importInstConfig.DeviceName = ptConfig.get<std::string>(DEVICE_NAME_NODE);
	importInstConfig.SecurityMode = ptConfig.get<int16_t>(SECURITY_MODE_NODE);
	importInstConfig.InactivityTimeout = ptConfig.get<int16_t>(INACTIVITY_TIMEOUT_NODE);
	importInstConfig.PasswordExpiration = ptConfig.get<int16_t>(PASSWORD_EXPIRATION_NODE);

	auto ptSmtp = ptConfig.find (SMTP_CONFIG_NODE);
	if (!ptSmtp->second.empty())
	{
		importInstConfig.Email_Settings.server_addr = ptSmtp->second.get<std::string>(SERVER_ADDRESS_NODE);
		importInstConfig.Email_Settings.port_number = ptSmtp->second.get<int32_t>(PORT_NODE);
		importInstConfig.Email_Settings.username = ptSmtp->second.get<std::string>(USERNAME_NODE);
		importInstConfig.Email_Settings.authenticate = ptSmtp->second.get<bool>(AUTHENTICATE_NODE);
		importInstConfig.Email_Settings.pwd_hash = ptSmtp->second.get<std::string>(PW_HASH_NODE);
	}

	auto ptAD = ptConfig.find (AD_CONFIG_NODE);
	if (!ptAD->second.empty())
	{
		importInstConfig.AD_Settings.base_dn = ptAD->second.get<std::string>(BASE_DOMAIN_NODE);
		importInstConfig.AD_Settings.servername = ptAD->second.get<std::string>(SERVER_NAME_NODE);
		importInstConfig.AD_Settings.server_addr = ptAD->second.get<std::string>(SERVER_ADDRESS_NODE);
		importInstConfig.AD_Settings.port_number  = ptAD->second.get<int32_t>(PORT_NODE);
		importInstConfig.AD_Settings.enabled = ptAD->second.get<bool>(ENABLED_NODE);
	}

	auto ptAutomation = ptConfig.find (AUTOMATION_CONFIG_NODE);
	if (!ptAutomation->second.empty())
	{
		importInstConfig.AutomationInstalled = ptAutomation->second.get<bool>(INSTALLED_NODE);
		importInstConfig.AutomationEnabled = ptAutomation->second.get<bool>(ENABLED_NODE);
		importInstConfig.ACupEnabled = false;
		boost::optional<bool> tBool = ptAutomation->second.get_optional<bool>(ACUP_ENABLED_NODE);
		if (tBool)
		{ // v1.3 config file did not contain this field.
			importInstConfig.ACupEnabled = ptAutomation->second.get<bool>(ACUP_ENABLED_NODE);
		}
		importInstConfig.AutomationPort = ptAutomation->second.get<int32_t>(PORT_NODE);
	}

	if (!InstrumentConfig::Instance().Set())
	{
		Logger::L().Log("ImportInstrumentConfig", severity_level::warning, "Set Instrument Config failed");
	}

	return;
}

//*****************************************************************************
std::string InstrumentConfig::GetDLLVersion()
{
	const std::string dllVersion = DLL_Version;

	// Find the second decimal point and extract everything before the second decimal point.
	size_t pos = dllVersion.find ('.');
	if (pos == std::string::npos)
	{
		HAWKEYE_ASSERT (MODULENAME, true);
	}
	pos = dllVersion.find ('.', pos + 1);
	if (pos == std::string::npos)
	{
		HAWKEYE_ASSERT (MODULENAME, true);
	}

	return dllVersion.substr(0, pos);
}
