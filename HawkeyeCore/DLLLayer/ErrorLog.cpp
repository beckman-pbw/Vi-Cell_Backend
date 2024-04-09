#include "stdafx.h"

#include "ErrorLog.hpp"

static const char MODULENAME[] = "ErrorLog";

std::string GetSystemErrorCodeAsStr(uint32_t system_error_code);

//*****************************************************************************
ErrorLog::ErrorLog (std::shared_ptr<HawkeyeServices> pHawkeyeService)
	: LoggerDB(DBLogType::eLOG_ERROR, pHawkeyeService) 
{	
}

//*****************************************************************************
ErrorLog::~ErrorLog()
{ }

void ErrorLog::readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<error_log_entryDLL>)> data_cb)
{
	HAWKEYE_ASSERT(MODULENAME, data_cb);

	pHawkeyeService_->enqueueInternal([=]()
		{
			readInternal(pHawkeyeService_, starttime, endtime, data_cb);
		});

}

void ErrorLog::readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<error_log_entryDLL>)> data_cb)
{
	std::vector<std::string> logRecs = {};
	retrieveRecords(starttime, endtime, logRecs);
	std::vector<error_log_entryDLL> data = {};
	for (auto s : logRecs)
	{
		if (auto v = getDecodedData(s))
			data.push_back(*v);
	}
	pHawkeyeService->enqueueInternal(data_cb, HawkeyeError::eSuccess, data);
	return;
}
//*****************************************************************************
boost::optional<uint64_t> ErrorLog::getTimeFieldFromLine(const std::string& line)
{
	std::string timeField;
	if (!commandParser_.parse(Delimiter, line)
		|| !commandParser_.getByIndex(0, timeField))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getTimeFieldFromLine : Failed to parse time field in line :" + line);
		return boost::none;
	}
	return std::stoull(timeField);
}

//*****************************************************************************
boost::optional<error_log_entryDLL> ErrorLog::getDecodedData(const std::string& line)
{
	if (!commandParser_.parse(Delimiter, line))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Failed to parse line :" + line);
		return boost::none;
	}

	std::vector<std::string> fields;
	for (auto index = 0; index < commandParser_.size(); index++)
	{
		fields.emplace_back(commandParser_.getByIndex(index));
	}

	error_log_entryDLL logData = {};
	if (fields.size() < 3)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Invalid entry count :" + std::to_string(fields.size()));
		return boost::none;
	}

	logData.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(std::stoull(fields[0]));
	logData.error_code = std::stoul(fields[1]);
	logData.username = fields[2];
	logData.message = GetSystemErrorCodeAsStr(logData.error_code);

	size_t msgSize = logData.message.size();
	// Remove line break from message
	if (msgSize > 0 && logData.message[msgSize - 1] == '\n')
	{
		logData.message.pop_back();
	}

	return logData;
}