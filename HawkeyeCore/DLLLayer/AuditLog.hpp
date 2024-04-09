#pragma once

#include "AuditEventType.hpp"
#include "LoggerDB.hpp"
#include "LogReaderBase.hpp"
#include "ReportsDLL.hpp"

std::string generateAuditWriteData (const std::string& username, audit_event_type eventType, std::string resource = "", std::string additionalInfo = "");
std::string encodeAuditData (audit_log_entryDLL auditLogEntry);

class AuditLog : public LoggerDB
{
public:
	AuditLog(std::shared_ptr<HawkeyeServices> pHawkeyeService);
	virtual ~AuditLog();

	
	static AuditLog& L()
	{	
		static AuditLog INSTANCE;
		return INSTANCE;
	}

	void readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<audit_log_entryDLL>)> data_cb);
	static boost::optional<audit_log_entryDLL> getDecodedData(const std::string& line, const std::string delimiter);

protected:
	AuditLog():LoggerDB(DBLogType::eLOG_AUDIT){}

	//boost::optional<uint64_t> getTimeFieldFromLine(const std::string& line);
	void readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<audit_log_entryDLL>)> data_cb); 

private:
	

};

using AuditLogger = AuditLog;

