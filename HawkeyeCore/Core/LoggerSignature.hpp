#pragma once
#ifndef SIGN_LOGGER
#define SIGN_LOGGER

#include <crc.h>

#include "LogEntry.hpp"
#include "Logger.hpp"
#include "SecurityHelpers.hpp"

using namespace CryptoPP;
using namespace SecurityHelpers;

static constexpr size_t LAST_ENTRY_INDEX = -1;
#define COMPROMISED_FILE_POSTFIX "_COMPROMISED"

template <size_t INDEX>
class LoggerSignature : public IndexedLogger<INDEX, false>  //'false' will make it append to the existing log file.
{
public:
	
	static LoggerSignature& L()
	{
		static LoggerSignature INSTANCE;
		return INSTANCE;
	}

	virtual ~LoggerSignature()
	{
		LoggerSignature<INDEX>::L().store_signature();
	}

	void CloseLogFile();
	bool isValidSignature() const;
	bool CheckSignature();
	virtual void Flush() override;

	size_t getTotalEntries() const;
	bool getLogEntryIndex(size_t index, LogEntry& entry);
	bool getLogEntriesIndex(size_t startindex, size_t endindex, std::vector<LogEntry>& entries);

	boost::posix_time::ptime getTimestampFromIndex(size_t index);
	size_t getIndexFromTimestamp(boost::posix_time::ptime target);
	bool getLogEntryTimestamp(const boost::posix_time::ptime& pt, LogEntry& log_entry);
	bool getLogEntriesTimestamp(const boost::posix_time::ptime& pt_start, const boost::posix_time::ptime& pt_end, std::vector<LogEntry>& entries);
	bool getLogEntriesAfterTimestamp(const boost::posix_time::ptime& pt_start, std::vector<LogEntry>& entries);

	bool deleteLogEntryIndex(size_t index);
	bool deleteLogEntriesIndex(size_t start_index, size_t end_index);
	bool deleteLogEntriesThroughIndex(size_t last_index_to_delete);
	bool deleteLogEntryTimestamp(const boost::posix_time::ptime& ptime);
	bool deleteLogEntriesTimestamp(const boost::posix_time::ptime& ptime_beg, boost::posix_time::ptime ptime_end);
	bool deleteLogEntriesPriorToTimestamp(const boost::posix_time::ptime& ptime);
	
	void setAutoCompromisedBackup(bool automatic);
	bool hasLogsBeenCompromised();
	bool hasLogsBeenCompromised(std::vector<std::string> & files);
	bool deleteCompromisedLogs(size_t& count);
	bool BackupCompromisedLog();

protected:
	LoggerSignature();

private:

	

	//Overrides for base class
	virtual void log__INTERNAL(boost::posix_time::ptime timestamp, const std::string& component, severity_level svl, const std::string& message) override;
	virtual std::string CreateLogEntryString(boost::posix_time::ptime timestamp, const std::string& component, severity_level svl, const std::string& message) override;
	virtual bool rolllogs__INTERNAL(bool initial = false) override;

	std::string sign_to_text(const std::string& freeText) const;
	void reset_entries_indexing();

	void store_signature();
	bool verify_stored_signature();

	bool extract_log_elements(std::string log_entry, LogEntry& entry) const;
	bool extract_log_elements(std::string log_entry, LogEntry& entry, std::string& hash) const;
	bool extract_line_hash(std::string full_log_line, std::string& line, std::string& original_hash) const;
	bool extract_hash(const std::string& cs, std::string& hash) const;
	
	bool remove_data_block(std::fpos<_Mbstatet> start_block, std::fpos<_Mbstatet> end_block);
	bool get_index_fromtimestamp(boost::posix_time::ptime stamp, size_t& index);
	bool get_timestamp_fromindex(size_t index, boost::posix_time::ptime& stamp);

	bool backup_compromised_log();
	bool get_compromised_logs(std::vector<std::string>& files);
	bool do_remove_file(std::vector<std::string>::value_type file) const;

	const std::string _salt_logentry_string1;
	const std::string _salt_logentry_string2;
	const std::string _salt_logfile_string1;
	const std::string _salt_logfile_string2;
	std::string _config_section_name;
	std::vector<unsigned char> _salt_logentry;
	std::vector<unsigned char> _salt_logfile;

	std::vector<LogIndex> _log_entries_indexed;
	std::fpos<_Mbstatet> _end_of_log;
	
	bool _valid_signature;
	bool _logfile_hasbeen_compromised;
	bool _auto_backup_compromised_logs;

};

// Define the standard singletons signature logger type
using UserLogger = LoggerSignature<10>;
using WorkLogger = LoggerSignature<11>;
using SignedErrorLogger = LoggerSignature<12>;

#endif
