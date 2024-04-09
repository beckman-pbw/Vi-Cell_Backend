#include "stdafx.h"

#include "AuditLog.hpp"
#include "UserList.hpp"
#include "GetAsStrFunctions.hpp"

static const char MODULENAME[] = "AuditLog";

//*****************************************************************************
AuditLog::AuditLog (std::shared_ptr<HawkeyeServices> pHawkeyeService)
	: LoggerDB(DBLogType::eLOG_AUDIT, pHawkeyeService) { 
}

//*****************************************************************************
AuditLog::~AuditLog()
{ }

//*****************************************************************************
void AuditLog::readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<audit_log_entryDLL>)> data_cb)
{
	HAWKEYE_ASSERT(MODULENAME, data_cb);

	pHawkeyeService_->enqueueInternal([=]()
		{
			readInternal(pHawkeyeService_, starttime, endtime, data_cb);
		});

}

//*****************************************************************************
void AuditLog::readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<audit_log_entryDLL>)> data_cb)
{
	std::vector<std::string> logRecs = {};
	retrieveRecords(starttime, endtime, logRecs);
	std::vector<audit_log_entryDLL> data = {};
	for (auto s : logRecs)
	{
		if (auto v = getDecodedData(s, Delimiter))
			data.push_back(*v);
	}
	pHawkeyeService->enqueueInternal(data_cb, HawkeyeError::eSuccess, data);
	return;
}

//*****************************************************************************
boost::optional<audit_log_entryDLL> AuditLog::getDecodedData(const std::string& line, const std::string delimiter)
{
	CommandParser commandParser = {};
	if (!commandParser.parse(delimiter, line))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Failed to parse line :" + line);
		return boost::none;
	}

	std::vector<std::string> fields;
	for (auto index = 0; index < commandParser.size(); index++)
	{
		fields.emplace_back(commandParser.getByIndex(index));
	}

	audit_log_entryDLL logData = {};
	if (fields.size() < 5)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Invalid entry count :" + std::to_string(fields.size()));
		return boost::none;
	}

	logData.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(std::stoull(fields[0]));
	logData.active_username = fields[1];
	logData.userPermission = fields[2];
	logData.event_type = static_cast<audit_event_type>(std::stol(fields[3]));
	logData.event_message = fields[4];
	if (fields.size() > 5)
	{
		logData.event_message += (" " + fields[5]);

		// This is needed to be able to convert the audit_log_entry back into a string in encodeAuditData.
		logData.additionalInfo = fields[5];
	}

	size_t msgSize = logData.event_message.size();

	// Remove line break from event message
	if (msgSize > 0 && logData.event_message[msgSize - 1] == '\n')
	{
		logData.event_message.pop_back();
	}

	return logData;
}

//*****************************************************************************
std::string generateAuditWriteData (const std::string& userName, audit_event_type eventType, std::string resource, std::string additionalInfo) {

	std::string userPermissionStr = "";

	// If the username is empty the user must be initializing, assume Admin user.
	UserPermissionLevel userPermission = UserPermissionLevel::eAdministrator;
	if (UserList::Instance().GetUserPermissionLevel (userName, userPermission) == HawkeyeError::eSuccess)
	{
		userPermissionStr = getPermissionLevelAsStr (userPermission);
	}

	std::string str = boost::str (boost::format ("%s|%s|%s|%d|%s|%s")
		% std::to_string (ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds> (ChronoUtilities::CurrentTime ()))
		% userName
		% userPermissionStr
		% eventType
		% resource
		% additionalInfo);

	return str;
}

//*****************************************************************************
std::string encodeAuditData (audit_log_entryDLL auditLogEntry) {

	std::string str = boost::str (boost::format ("%s|%s|%s|%d|%s|%s")
		% std::to_string (ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds> (ChronoUtilities::CurrentTime ()))
		% auditLogEntry.active_username
		% auditLogEntry.userPermission
		% auditLogEntry.event_type
		% auditLogEntry.event_message
		% auditLogEntry.additionalInfo);

	return str;
}
