#include "stdafx.h"

#include "DBif_Api.h"
#include "HawkeyeDirectory.hpp"
#include "LoggerDB.hpp"
#include "LogReaderBase.hpp"

static const char MODULENAME[] = "LoggerDB";

LoggerDB::LoggerDB(DBLogType logType, std::shared_ptr<HawkeyeServices> pHawkeyeService)
	: logType_(logType), pHawkeyeService_(pHawkeyeService), Delimiter("|")
{
}

LoggerDB::LoggerDB(DBLogType logType)
	: logType_(logType), Delimiter("|")
{
}

// **************************************************************************
/**
 * \brief Add an entry to the log.
 * \param[in] entry the string to store in the log
 */
void LoggerDB::Log(const std::string& entry)
{
	DBApi::DB_LogEntryRecord log_entry = {};
	DBApi::eQueryResult qr = DBApi::eQueryResult::BadQuery;
	log_entry.EntryStr = entry;
	if (logType_ == DBLogType::eLOG_AUDIT)
	{
		qr = DBApi::DbAddAuditLogEntry(log_entry);
	}
	else if (logType_ == DBLogType::eLOG_ERROR)
	{
		qr = DBApi::DbAddErrorLogEntry(log_entry);
	}
	else if (logType_ == DBLogType::eLOG_SAMPLE)
	{
		qr = DBApi::DbAddSampleLogEntry(log_entry);
	}
	if (qr != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "LoggerDB::Log() - failed to write to DB: " + entry);
	}

}

// **************************************************************************
/**
 * \brief Get a list of records as strings. The filter by date does not work today. 
 *
 * \param[in] starttime the starting date to search for (0: no start date)
 * \param[in] endtime the end date of the search  (0: no end date)
 * \param[out] logRecs - a list of strings from the log 
 *
 */
void LoggerDB::retrieveRecords(uint64_t starttime, uint64_t endtime, std::vector<std::string>& logRecs)
{
	std::vector<DBApi::DB_LogEntryRecord> log_list;
	std::vector<DBApi::eListFilterCriteria> filtertypelist = {};
	std::vector<std::string> filtercomparelist = {};
	std::vector<std::string> filtertgtlist = {};
	DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::CreationDateSort;
	DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::NoSort;
	int32_t sortdir = -1;

	DBApi::eQueryResult qr;
	std::string logTypeStr;

	switch (logType_)
	{
		case DBLogType::eLOG_AUDIT:
			qr = DBApi::DbGetAuditLogList (log_list, filtertypelist, filtercomparelist, filtertgtlist, -2, primarysort, secondarysort, sortdir);
			logTypeStr = "Audit";
			break;

		case DBLogType::eLOG_ERROR:
			qr = DBApi::DbGetErrorLogList(log_list, filtertypelist, filtercomparelist, filtertgtlist, -2, primarysort, secondarysort, sortdir);
			logTypeStr = "Error";
			break;

		case DBLogType::eLOG_SAMPLE:
			qr = DBApi::DbGetSampleLogList(log_list, filtertypelist, filtercomparelist, filtertgtlist, -2, primarysort, secondarysort, sortdir);
			logTypeStr = "Sample";
			break;
	}

	if (qr == DBApi::eQueryResult::QueryOk)
	{
		for (auto entry : log_list)
		{
			if (((starttime == 0) || (ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(entry.EntryDate) >= starttime)) &&
				((endtime == 0) || (ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(entry.EntryDate) <= endtime)))
			{
				logRecs.push_back(entry.EntryStr);
			}
		}
	}
	else 
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveRecords: <failed to read " + logTypeStr + " log>");
	}
}

//*****************************************************************************
static void readLegacyFile (LoggerDB::DBLogType logType, std::vector<std::string>& data)
{
	std::string filePath;

	switch (logType)
	{
		case LoggerDB::DBLogType::eLOG_AUDIT:
			filePath = HawkeyeDirectory::Instance().getLogsDir() + "\\Audit.Log";
			break;

		case LoggerDB::DBLogType::eLOG_ERROR:
			filePath = HawkeyeDirectory::Instance().getLogsDir() + "\\Error.Log";
			break;

		case LoggerDB::DBLogType::eLOG_SAMPLE:
			filePath = HawkeyeDirectory::Instance().getLogsDir() + "\\Sample.Log";
			break;
	}

	std::ifstream file (filePath);

	DBApi::DB_LogEntryRecord log_entry = {};
	DBApi::eQueryResult qr = DBApi::eQueryResult::BadQuery;

	std::string line;

	std::getline (file, line);	// Skip the "EncryptionKeyHeader".

	bool foundRecordToSkip = false;

	size_t lineCount = 0;

	std::string multiLine;
	while (std::getline (file, line))
	{
		if (!(lineCount % 100))
		{
			std::cout << "\rLines read: " + std::to_string (lineCount);
		}

		lineCount++;

		// Find the entry that starts the data to skip.
		std::size_t found = line.find ("Missing result data file");
		if (found != std::string::npos)
		{
			foundRecordToSkip = true;
			continue;
		}

		// Skip the rest of the entry.
		if (foundRecordToSkip && line[0] == '\t')
		{
			continue;
		}

		// No more data to skip.
		foundRecordToSkip = false;

		// std::getline removes line-end when reading string, put it back.
		line += "\n";

		if (LoggerDB::DecodeForMultilineEntries(line))
		{
			multiLine += line;
			data.pop_back(); // remove last entry, a new multiline entry will be added
		}
		else
		{
			multiLine = line;
		}
		data.emplace_back(multiLine);
	}

	std::cout << "\rLines read: " + std::to_string (lineCount) << std::endl;
}

//*****************************************************************************
HawkeyeError LoggerDB::Import (DBLogType logType)
{
	std::vector<std::string> data;

	DBApi::DB_LogEntryRecord log_entry = {};
	DBApi::eQueryResult qr = DBApi::eQueryResult::BadQuery;

	readLegacyFile (logType, data);

	size_t lineCount = 0;

	for (auto& rec : data)
	{
		if (!(lineCount % 100))
		{
			std::cout << "\rLines stored: " + std::to_string (lineCount);
		}

		lineCount++;

		std::string timeStr = rec.substr (0, rec.find("|"));
		log_entry.EntryDate = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(std::stoull(timeStr.c_str()));
		log_entry.EntryStr = rec;

		switch (logType)
		{
			case DBLogType::eLOG_AUDIT:
				qr = DBApi::DbAddAuditLogEntry (log_entry);
				break;

			case DBLogType::eLOG_ERROR:
				qr = DBApi::DbAddErrorLogEntry (log_entry);
				break;

			case DBLogType::eLOG_SAMPLE:
				qr = DBApi::DbAddSampleLogEntry (log_entry);
				break;

			default:
				Logger::L().Log(MODULENAME, severity_level::error, "Import: failed to import: " + rec);
				break;
		}
	}

	std::cout << "\rLines stored: " + std::to_string (lineCount) << std::endl;

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool LoggerDB::DecodeForMultilineEntries (std::string& message)
{
	// Check if first char is "\t", that means its continuation from previous line
	if (message[0] == '\t')
	{
		// Remove the '\t' when reading back
		message.erase(0, 1);
		return true;
	}
	return false;
}
