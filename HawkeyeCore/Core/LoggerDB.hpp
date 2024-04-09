#pragma once

#ifndef LOGGER_DB
#define LOGGER_DB

#include "HawkeyeError.hpp"
#include "HawkeyeServices.hpp"
#include "CommandParser.hpp"
#include "SystemErrors.hpp"
#include "Logger.hpp"

class LoggerDB
{
public:

	enum DBLogType {
		eLOG_AUDIT = 0,
		eLOG_ERROR,
		eLOG_SAMPLE
	};

	LoggerDB(DBLogType logType);

	virtual ~LoggerDB()
	{
	}

	void Log(const std::string& entry);

	static HawkeyeError Import (DBLogType logType);
	static bool DecodeForMultilineEntries (std::string& message);

protected:
	LoggerDB(DBLogType logType, std::shared_ptr<HawkeyeServices> pHawkeyeService);
	DBLogType logType_;
	void retrieveRecords(uint64_t starttime, uint64_t endtime, std::vector<std::string>& logRecs);

	std::shared_ptr<HawkeyeServices> pHawkeyeService_;
	CommandParser commandParser_;
	const std::string Delimiter;

};


#endif