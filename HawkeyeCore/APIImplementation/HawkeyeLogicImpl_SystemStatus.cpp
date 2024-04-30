#include "stdafx.h"

#include "AuditLog.hpp"
#include "EEPROMDataSignatures.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "Registers.hpp"
#include "SystemStatusCommon.hpp"
#include "SystemErrors.hpp"
#include "MotorStatusDLL.hpp"

static const char MODULENAME[] = "SystemStatus";

//*****************************************************************************
void HawkeyeLogicImpl::GetSystemStatus (SystemStatusData*& systemStatusData) {

	// Only for simulation.
	if (!HawkeyeConfig::Instance().get().hardwareConfig.stageController)
	{
		//Set Carousel if carousel is true in Hardware config
		//Set Plate if carousel is false in Hardware config
		if (HawkeyeConfig::Instance().get().isSimulatorCarousel) {
			SystemStatus::Instance().getData().sensor_carousel_detect = eSensorStatus::ssStateActive;
		} else {
			SystemStatus::Instance().getData().sensor_carousel_detect = eSensorStatus::ssStateInactive;
		}
	}
	SystemStatus::Instance().ToCStyle (systemStatusData);
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSystemStatus (SystemStatusData* systemStatusData)
{
	SystemStatus::Instance().free (systemStatusData);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ClearSystemErrorCode (uint32_t active_error)
{
	// This must be done by any logged in  User.
	auto status = UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
//TODO: 			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Clear System Error Code"));
		return HawkeyeError::eNotPermittedByUser;
	}

	ReportSystemError::Instance().ClearErrorCode (active_error);

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool HawkeyeLogicImpl::initializeSerialNumber()
{
	bool status = true;

	if (!HawkeyeConfig::Instance().get().withHardware)
	{
		serialNumber_ConfigCopy = HawkeyeConfig::Instance().get().instrumentSerialNumber;
		serialNumber_FirmwareCacheCopy = HawkeyeConfig::Instance().get().instrumentSerialNumber;
		serialNumber_Canonical = HawkeyeConfig::Instance().get().instrumentSerialNumber;
		return status;
	}

	serialNumber_FirmwareCacheCopy.clear();
	serialNumber_ConfigCopy.clear();
	serialNumber_Canonical.clear();
	serialNumber_ConfigCopy = HawkeyeConfig::Instance().get().instrumentSerialNumber;

	std::vector<unsigned char> data;

	if (!Hardware::Instance().getEEPROMController()->GetData(SERIAL_NUMBER_SIGNATURE, data) || data.empty())
	{
		// Unable to retrieve signature.
		this->serialNumber_FirmwareCacheCopy.clear();
	}
	else
	{
		// Copy the cache into a string.
		char* ptr = (char*)data.data();
		serialNumber_FirmwareCacheCopy = ptr;
	}

	// If config serial is empty and cached is not: 
	if (!serialNumber_FirmwareCacheCopy.empty() && serialNumber_ConfigCopy.empty())
	{
		//    Update config; record audit entry
		HawkeyeConfig::Instance().get().instrumentSerialNumber = serialNumber_FirmwareCacheCopy;
		serialNumber_ConfigCopy = serialNumber_FirmwareCacheCopy;
		serialNumber_Canonical = serialNumber_ConfigCopy;
		AuditLogger::L().Log (generateAuditWriteData(
			"",
			audit_event_type::evt_serialnumber,
			" Instrument serial number recovered from hardware cache \"" + serialNumber_FirmwareCacheCopy + "\""));
		Logger::L().Log(MODULENAME, severity_level::debug1, "initializeSerialNumber <Exit - Instrument serial number recovered from hardware cache, sn \"" + serialNumber_FirmwareCacheCopy + "\">");
	}
	else if (serialNumber_FirmwareCacheCopy.empty() && !serialNumber_ConfigCopy.empty())
	{
		serialNumber_Canonical = serialNumber_ConfigCopy;

		AuditLogger::L().Log(generateAuditWriteData(
			"",
			audit_event_type::evt_serialnumber,
			" Instrument serial number recovered from configuration cache \"" + serialNumber_ConfigCopy + "\""));
		Logger::L().Log(MODULENAME, severity_level::normal, "initializeSerialNumber <exit, instrument serial number recovered from configuration cache, sn \"" + serialNumber_ConfigCopy + "\">");
	}
	else
	{
		// If neither is empty:
		if (serialNumber_ConfigCopy == serialNumber_FirmwareCacheCopy)
		{ 
			/* OK - no action*/ 
			serialNumber_Canonical = serialNumber_ConfigCopy;
		}
		else
		{
			/* NOT OK - Flag an error condition*/
			serialNumber_Canonical.clear();

			auto audit_str = boost::str(boost::format(" Conflict : Configuration reports \"%s\" and Hardware reports \"%s\"") % serialNumber_ConfigCopy % serialNumber_FirmwareCacheCopy);
			AuditLogger::L().Log(generateAuditWriteData(
				"",
				audit_event_type::evt_serialnumber,
				audit_str));
			Logger::L().Log(MODULENAME, severity_level::error, "initializeSerialNumber: <Exit -" + audit_str + ">");

			return false;
		}
	}

	return true;
}

HawkeyeError HawkeyeLogicImpl::GetSystemSerialNumber (char*& serialNumber)
{
	if (serialNumber_Canonical.empty())
	{
		if (serialNumber_ConfigCopy != serialNumber_FirmwareCacheCopy)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "System serial number state is inconsistent between configuration and cached copy");
			DataConversion::convertToCharPointer(serialNumber, (serialNumber_ConfigCopy.empty() ? serialNumber_FirmwareCacheCopy : serialNumber_ConfigCopy));
			return HawkeyeError::eEntryInvalid;
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "System serial number is not configured");
			return HawkeyeError::eNoneFound;
		}			
	}
	else
	{
		DataConversion::convertToCharPointer(serialNumber, serialNumber_Canonical);
	}

	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::svc_SetSystemSerialNumber (char* serial, char* service_password, std::function<void(HawkeyeError)>onComplete)
{
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(), 
			audit_event_type::evt_notAuthorized, 
			"Service: Set Serial Number"));
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eNotPermittedByUser);
		return;
	}

	if (UserList::Instance().ValidateConsoleUser(service_password) != HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Serial Number"));
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eValidationFailed);
		return;
	}

	std::string sn;
	DataConversion::convertToStandardString (sn, serial);

	if (sn.empty())
	{
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
		return;
	}

	serialNumber_Canonical = sn;
	serialNumber_ConfigCopy = sn;
	serialNumber_FirmwareCacheCopy = sn;

	HawkeyeConfig::Instance().get().instrumentSerialNumber = serialNumber_ConfigCopy;

	if (!HawkeyeConfig::Instance().get().withHardware)
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "System serial number updated to " + sn);
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eSuccess);
	}
	else
	{
		std::vector<unsigned char> d;
		d.assign(sn.begin(), sn.end());

		auto writedata_cb = [this, sn, onComplete](HawkeyeError he)
		{
			if (he == HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::notification, "System serial number updated to " + sn);
			}
			pHawkeyeServices_->enqueueExternal (onComplete, he);
		};

		Hardware::Instance().getEEPROMController()->WriteData (SERIAL_NUMBER_SIGNATURE, d, writedata_cb);
	}
}
