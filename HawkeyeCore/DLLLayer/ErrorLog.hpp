#pragma once

#include "LoggerDB.hpp"
#include "ReportsDLL.hpp"

class ErrorLog : public LoggerDB
{
public:
	ErrorLog (std::shared_ptr<HawkeyeServices> pHawkeyeService);
	~ErrorLog();


	static ErrorLog& L()
	{
		static ErrorLog INSTANCE;
		return INSTANCE;
	}

	void readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<error_log_entryDLL>)> data_cb);

protected:
	ErrorLog() :LoggerDB(DBLogType::eLOG_ERROR) {}

	void readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<error_log_entryDLL>)> data_cb);

	boost::optional<uint64_t> getTimeFieldFromLine(const std::string& line);
	boost::optional<error_log_entryDLL> getDecodedData(const std::string& line);

};

using ErrorLogger = ErrorLog;
