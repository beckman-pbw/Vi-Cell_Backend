#pragma once

#ifndef SIGN_LOGGER_IMPL
#define SIGN_LOGGER_IMPL

// PLEASE NOTE: This file should NOT be included EXCEPT by `.cpp` files that
// INSTANTIATE a logger with a specified INDEX.

#include <aes.h>

#include "LoggerSignature.hpp"
#include "Logger_auxiliary.hpp"
#include "Logger_Impl.hxx"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptors.hpp>

namespace errc = boost::system::errc;
using namespace Logger_Impl;
using namespace SecurityHelpers;

#define Log_REPETITIONS 543

template <size_t INDEX>
LoggerSignature<INDEX>::LoggerSignature() :
	_salt_logentry_string1("retluoC namkceB"),
	_salt_logentry_string2("troF snilloC odaroloC"),
	_salt_logfile_string1("Eyekwah Tuocs"),
	_salt_logfile_string2("Ognarud Soirtsa"),
	_config_section_name("logger"),
	_valid_signature(false),
	_logfile_hasbeen_compromised(false),
	_auto_backup_compromised_logs(true)
{
	_salt_logentry.insert(_salt_logentry.end(), _salt_logentry_string1.begin(), _salt_logentry_string1.end());
	_salt_logentry.insert(_salt_logentry.end(), _salt_logentry_string2.begin(), _salt_logentry_string2.end());

	_salt_logfile.insert(_salt_logfile.end(), _salt_logfile_string1.begin(), _salt_logfile_string1.end());
	_salt_logfile.insert(_salt_logfile.end(), _salt_logfile_string2.begin(), _salt_logfile_string2.end());
}

/**
 * \brief – Close the signed log file. 
 */
template<size_t INDEX>
void LoggerSignature<INDEX>::CloseLogFile()
{
	store_signature();
	this->_log_OS.close();
}

/**
* \brief This will return the current state of the signature since the last file flush.
* \return bool - signature is valid or not.
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::isValidSignature() const
{
	return _valid_signature;
}

/**
* \brief This will manually confirm the signature that on the log file.
* \return bool -signature is valid (true) or not.
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::CheckSignature()
{
	std::lock_guard<std::mutex> lkg(m_logstream);
	_valid_signature = verify_stored_signature();

	if (!_valid_signature && _auto_backup_compromised_logs)
	{
		_logfile_hasbeen_compromised = true;
		backup_compromised_log();
	}
	else if(!_valid_signature)
		_logfile_hasbeen_compromised = true;

	return _valid_signature;
}

/**
* \brief Write out what's in memory into the log file. We have overridden it so we can record and check the signature.
*/
template <size_t INDEX>
void LoggerSignature<INDEX>::Flush()
{
	// Explicitly flush stdout.
	// XXX TODO
	// Add config-file option to completely suppress iostream output.
	// This should enhance performance when enabled.
	std::lock_guard<std::mutex> lock(this->m_logstream);
	std::cout.flush();
	if (!this->_log_OS.is_open())
	{
		return;
	}

	if(!this->_always_flush)
		_valid_signature = verify_stored_signature();

	store_signature();

	this->_log_OS.flush();
}

///**
// * \brief To load the configuration and start appending to the existing log file. We had to overload it so we can initialize the "Salt" vector for SHA. 
// * \param ec - store the error that has encountered
// * \param cfg - the string of the configuration file.
// * \param cfg_log_overrides - to override the configuration parameters from the file. 
// */
//template<size_t INDEX>
//void LoggerSignature<INDEX>::Initialize(boost::system::error_code & ec, const std::string & cfg, t_pPTree cfg_log_overrides)
//{
//	//_salt_logentry.insert(_salt_logentry.end(), _salt_logentry_string1.begin(), _salt_logentry_string1.end());
//	//_salt_logentry.insert(_salt_logentry.end(), _salt_logentry_string2.begin(), _salt_logentry_string2.end());
//
//	//_salt_logfile.insert(_salt_logfile.end(), _salt_logfile_string1.begin(), _salt_logfile_string1.end());
//	//_salt_logfile.insert(_salt_logfile.end(), _salt_logfile_string2.begin(), _salt_logfile_string2.end());
//	
//	IndexedLogger<INDEX, false>::Initialize(ec, cfg, cfg_log_overrides);
//	this->SetAlwaysFlush(true);
//}

//template<size_t INDEX>
//inline void LoggerSignature<INDEX>::Initialize(boost::system::error_code & ec, const std::string & cfg, const std::string & config_section, t_pPTree cfg_log_overrides)
//{
//	_config_section_name = config_section;
//	Initialize(ec, cfg, cfg_log_overrides);
//}

//template<size_t INDEX>
//void LoggerSignature<INDEX>::ReadConfiguration(boost::system::error_code & ec)
//{
//	ec = errc::make_error_code(::errc::success);
//
//	if (IndexedLogger<INDEX, false>::_config_file.empty())
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration requested but no file specified");
//		ec = errc::make_error_code(errc::invalid_argument);
//		return;
//	}
//
//	t_pPTree pt = ConfigUtils::OpenConfigFile(IndexedLogger<INDEX, false>::_config_file, ec);
//	if (ec == ::errc::no_such_file_or_directory)
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration file \"" + IndexedLogger<INDEX, false>::_config_file + "\" does not exist or is not a regular file.");
//		return;
//	}
//	else if (ec == ::errc::invalid_argument)
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration file \"" + IndexedLogger<INDEX, false>::_config_file + "\" type could not be determined.");
//		return;
//	}
//	else if (ec == ::errc::io_error)
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Unable to parse configuration file \"" + IndexedLogger<INDEX, false>::_config_file + "\".");
//		return;
//	}
//
//	t_opPTree config = (*pt).get_child_optional("config");
//	if (!config)
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Error parsing configuration file \"" + IndexedLogger<INDEX, false>::_config_file + "\" - \"config\" section not found");
//		ec = ::errc::make_error_code(::errc::io_error);
//		return;
//	}
//
//	t_opPTree module = config->get_child_optional(_config_section_name);
//	if (!module)
//	{
//		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Error parsing configuration file \"" + IndexedLogger<INDEX, false>::_config_file + "\" - \"config.logger\" section not found");
//		ec = ::errc::make_error_code(::errc::io_error);
//		return;
//	}
//
//	IndexedLogger<INDEX, false>::GetCfgFromPTree(module.get());
//}

/**
* \brief This will return the total log entries that were passed into the logger.
* \return size_t - total entries
*/
template<size_t INDEX>
size_t LoggerSignature<INDEX>::getTotalEntries() const
{
	return _log_entries_indexed.size();
}

/**
* \brief Get the log entry from a particular index value.
* \param index - value of the log line number to extract the log from
* \param entry - to store the log entry from the index
* \return successful to retrieve
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::getLogEntryIndex(size_t index, LogEntry& entry)
{
	if (index > _log_entries_indexed.size())
		return false;

	char cha;
	bool multi_line = false;
	std::string line;
	std::string full_log_entry;
	std::string hash;

	std::lock_guard<std::mutex> lkg(m_logstream);
	this->_log_OS.seekg(_log_entries_indexed[index].beg_fpos, std::ios::beg);

	while (!this->_log_OS.rdstate())
	{

		this->_log_OS.get(cha);
		line += cha;
		
		if (cha == '\n' && !multi_line)
		{
			if(!extract_line_hash(line, full_log_entry, hash))
			{
				log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Unable to extract log entry and hash value. \"" + line+ "\"");
			}

			line.pop_back();
			if(!extract_log_elements(line, entry))
			{
				log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Unable to extract log elements. \"" + line + "\"");
			}
			
			if (this->_log_OS.peek() == '\t')
				multi_line = true;
			else
				break;

			line.clear();
		}
		else if (cha == '\n')
		{
			full_log_entry += line;

			if (this->_log_OS.peek() != '\t')
				line.erase(0, 1);
			else
			{
				line.replace(0, 1, "\n");
				line.pop_back();
			}
			entry.LogMessage += line;
			line.clear();

			if (this->_log_OS.peek() != '\t')
				break;
		}
	}
	entry.isValid = (_log_entries_indexed[index].hash == sign_to_text(full_log_entry) &&
					 _log_entries_indexed[index].length == full_log_entry.length());

	if (!entry.isValid)
		_valid_signature = false; 

	if (this->_log_OS.rdstate())
		this->_log_OS.clear();

	this->_log_OS.seekp(_end_of_log, std::ios::beg);

	return true;
}

/**
* \brief Get a vector of log entries from a range of indexes.
* \param startindex - top index range
* \param endindex - bottom index range
* \param entries - the vector to store the entries
* \return successful to retrieve
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::getLogEntriesIndex(size_t startindex, size_t endindex, std::vector<LogEntry>& entries)
{
	if (endindex < startindex)
		return false;

	if (endindex >= _log_entries_indexed.size())
		endindex = _log_entries_indexed.size()-1;

	LogEntry tmp;
	for(auto i=startindex; i <= static_cast<size_t>(endindex); i++)
	{
		if (getLogEntryIndex(i, tmp))
			entries.push_back(tmp);
		else
			return false;
	}

	return true;
}

/**
* \brief Get the index value from a particular timestamp. The resolution of the timestamp should be within a second. 
* \param target - the timestamp to get the index value
* \return the index value
*/
template<size_t INDEX>
size_t LoggerSignature<INDEX>::getIndexFromTimestamp(boost::posix_time::ptime target)
{
	size_t index;
	get_index_fromtimestamp(target, index);
	return index;
}

/**
* \brief Get the logged timestamp of a particular index value
* \param index to get the timestamp from. 
* \return the the timestamp 
*/
template<size_t INDEX>
boost::posix_time::ptime LoggerSignature<INDEX>::getTimestampFromIndex(size_t index)
{
	boost::posix_time::ptime p_time;
	get_timestamp_fromindex(index, p_time);
	return p_time;
}

/**
* \brief Get the log entry from a particular timestamp.
* \param pt the timestamp for the log entry
* \param log_entry to store the data into
* \return successful to retrieve
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::getLogEntryTimestamp(const boost::posix_time::ptime& pt, LogEntry& log_entry)
{
	size_t index;

	if(get_index_fromtimestamp(pt, index))
		return getLogEntryIndex(index, log_entry);

	return false;
}

/**
* \brief Get a vector of log entries from a range of timestamps.
* \param pt_start the top or the range 
* \param pt_end the bottom of the range
* \param entries the vector to store the entries
* \return successful to get log entries
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::getLogEntriesTimestamp(const boost::posix_time::ptime& pt_start, const boost::posix_time::ptime& pt_end, std::vector<LogEntry>& entries)
{
	size_t start_index;
	size_t end_index;

	if (!get_index_fromtimestamp(pt_start, start_index))
		return false;
	if (!get_index_fromtimestamp(pt_end, end_index))
		return false;
	if(!getLogEntriesIndex(start_index, static_cast<int64_t>(end_index), entries))
		return false;

	return true;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::getLogEntriesAfterTimestamp(const boost::posix_time::ptime& pt_start, std::vector<LogEntry>& entries)
{
	return getLogEntriesTimestamp(pt_start, getTimestampFromIndex(_log_entries_indexed.size()-1), entries);
}


template<size_t INDEX>
bool LoggerSignature<INDEX>::get_index_fromtimestamp(const boost::posix_time::ptime stamp, size_t& index)
{
	//LogEntry entry;
	boost::posix_time::time_duration  diff;

	//need to check the range.
	diff = stamp - _log_entries_indexed.front().ptime_logged;//entry.pTimeLogged;
	if (diff.total_milliseconds() < 0)//the first entry is bigger than the stamp
	{
		//the first entry is less than the stamp.
		// no need to search for the index, give them the first index
		index = 0;
		return true;
	}

	auto ptime_back = _log_entries_indexed.back().ptime_logged;
	diff = ptime_back - stamp;
	if (diff.total_milliseconds() <= 0 && //the stamp is bigger than the last entry
		ptime_back != stamp) {
		return false;
	}
		
	if (ptime_back == stamp)
	{
		//why search the file if requested timestamp is the last entry.
		index = _log_entries_indexed.size() - 1;
		return true;
	}

	//std::cout << "stamp: " + boost::posix_time::to_simple_string(stamp)+"\n"; //some useful debug strings
	index = 0;
	size_t loopindex=0;
	for(auto const & element : _log_entries_indexed)
	{
		diff = stamp - element.ptime_logged;
		//std::cout << "\tLogged: " + boost::posix_time::to_simple_string(element.ptime_logged) +
		//	" \tdiff: " + std::to_string(diff.total_seconds()) + ":" + std::to_string(diff.total_milliseconds()) + "\n";
		if (diff.total_seconds() == 0 && diff.total_milliseconds() <= 0)
		{
			index = loopindex;
			break;
		}
		loopindex++;
	}
	
	if (index == 0) {
		return false; //index is 0 if unable to find index.
	}
		
	return true;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::get_timestamp_fromindex(const size_t index, boost::posix_time::ptime& stamp)
{
	// INTERNAL - SHOULD NOT NEED TO LOCK m_logstream
	stamp = boost::posix_time::not_a_date_time;

	if (index > _log_entries_indexed.size())
		return false;
	

	bool result = true;
	LogEntry entry;
	std::string line;

	this->_log_OS.seekg(_log_entries_indexed[index].beg_fpos, std::ios::beg);
	std::getline(this->_log_OS, line);

	if (extract_log_elements(line, entry))
		stamp = entry.pTimeLogged;
	else
		result = false;

	return result;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::backup_compromised_log()
{
	// INTERNAL - SHOULD NOT NEED TO LOCK m_logstream
	if (!_logfile_hasbeen_compromised) return false;// the log file was not checked or the signature is valid.

	boost::system::error_code ec;
	/*We will be using rolllogs__INTERNAL() to backup the compromised log.
	  We just change the name of the log file, then roll the log.*/
	
	//save the real log name
	auto real_log = this->_log_file;
	this->_log_OS.close();
	boost::filesystem::path orignal_file_path(LogFile());

	//append a label to the current log file
	this->_log_file = this->_log_file + COMPROMISED_FILE_POSTFIX;
	boost::filesystem::path compromised_file_path(LogFile());

	//rename the current log file with the new compromised log name
	boost::filesystem::rename(orignal_file_path, compromised_file_path, ec);
	this->_log_OS.open(this->LogFile(), std::fstream::in | std::fstream::out);//need to reopen the log file for rolllogs__INTERNAL() to work

	//roll the compromised log file.
	std::string fail_messag;
	if (ec != boost::system::errc::success)
	{
		this->_log_file = real_log;
		return false;
	}
	rolllogs__INTERNAL(false);//do not check for the return value. we do not need to.
	
	//close the compromised file, rename back the log file, then reopen it. 
	this->_log_OS.close();
	this->_log_file = real_log;
	this->_log_OS.open(this->LogFile(), std::fstream::in | std::fstream::out | std::fstream::app);
	reset_entries_indexing();

	return true;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::get_compromised_logs(std::vector<std::string> & files)
{ 
	const boost::regex compromised_filter(this->_log_file+ COMPROMISED_FILE_POSTFIX +".*");
	boost::filesystem::directory_iterator end_itr; 
	
	for (boost::filesystem::directory_iterator i(this->_log_path); i != end_itr; ++i)
	{
		// Skip if not a file
		if (!boost::filesystem::is_regular_file(i->status())) continue;

		boost::smatch what;
		if( !boost::regex_match( i->path().filename().string(), what, compromised_filter) ) continue;

		// File matches, store it
		files.push_back(i->path().filename().string());
	}

	if (files.size() <= 0)
		return false;
	return true;
}

/**
* \brief Delete one log entry. 
* \param index - the index to delete
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntryIndex(size_t index)
{
	if (index >= _log_entries_indexed.size())
		return false;

	LogEntry entry;
	getLogEntryIndex(index, entry);

	std::lock_guard<std::mutex> lkg(m_logstream);
	if(remove_data_block(_log_entries_indexed[index].beg_fpos, _log_entries_indexed[index].end_fpos))
	{
		this->log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, "Removed 1 log entry: {" + to_simple_string(entry.pTimeLogged) + "}");
		return true;
	}
	
	this->log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, "Unable to Removed log entry: " + std::to_string(index));

	return false;
}

/**
* \brief Delete log entries with a range of indexes. 
* \param start_index
* \param end_index
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntriesIndex(size_t start_index, size_t end_index)
{
	if (_log_entries_indexed.size() <= 0)
		return false;
	if (end_index < start_index)
		return false;

	if (end_index >= _log_entries_indexed.size())
		end_index = _log_entries_indexed.size() - 1;

	auto start_time = _log_entries_indexed[start_index].ptime_logged;
	auto end_time = _log_entries_indexed[end_index].ptime_logged;

	std::lock_guard<std::mutex> lkg(m_logstream);
	if (remove_data_block(_log_entries_indexed[start_index].beg_fpos, _log_entries_indexed[end_index].end_fpos))
	{
		this->log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL,
				  severity_level::critical,
				  "Removed log entries. From: {" + to_simple_string(start_time) + "} To: {" + to_simple_string(end_time) + "}");
		return true;
	}
	
	this->log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL,
	                    severity_level::critical,"Unable to Remove Entries from:"+std::to_string(start_index)+" to:"+std::to_string(end_index));
	return false;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntriesThroughIndex(size_t last_index_to_delete)
{
	return deleteLogEntriesIndex(0, last_index_to_delete);
}

/**
* \brief Delete one log entry by timestamp.
* \param ptime
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntryTimestamp(const boost::posix_time::ptime& ptime)
{
	if (ptime.is_not_a_date_time())
		return false;

	size_t index;
	if (!get_index_fromtimestamp(ptime, index))
		return false;
	return deleteLogEntryIndex(index);
}

/**
* \brief Delete log entries with a range of timestamps. 
* \param ptime_beg
* \param ptime_end
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntriesTimestamp(const boost::posix_time::ptime& ptime_beg, boost::posix_time::ptime ptime_end)
{
	if (ptime_beg.is_not_a_date_time() || ptime_end.is_not_a_date_time() ||
		ptime_end < ptime_beg) 
		return false;

	size_t index_beg;
	size_t index_end;

	LogEntry end_entry;
	LogEntry top_entry;
	getLogEntryIndex(_log_entries_indexed.size()-1, end_entry);
	getLogEntryIndex(0, top_entry);

	auto diff =  end_entry.pTimeLogged - ptime_end;
	if (diff.total_seconds() < 0 || diff.total_milliseconds() < 0)
		ptime_end = end_entry.pTimeLogged;
	
	get_index_fromtimestamp(ptime_beg, index_beg);
	get_index_fromtimestamp(ptime_end, index_end);

	return deleteLogEntriesIndex(index_beg, index_end);
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteLogEntriesPriorToTimestamp(const boost::posix_time::ptime& timestamp)
{
	if (timestamp.is_not_a_date_time())
		return false;

	size_t end_index;
	if (!get_index_fromtimestamp(timestamp, end_index))
		return true;

	//Remove entries prior to timestamp,
	//does not include the entry pointed by the timestamp. 
	if (end_index - 1 <= _log_entries_indexed.size())
		end_index -= 1; //just go back one index to keep the timestamp entry. 

	return deleteLogEntriesIndex(0,end_index);
}

/**
 * \brief – Set the state of automatically backup  the log file its has been compromised.
 * \param – automatic  - Pass the new state for the flag to indicate to backup the compromised the file. 
 */
template<size_t INDEX>
void LoggerSignature<INDEX>::setAutoCompromisedBackup(bool automatic)
{
	_auto_backup_compromised_logs = automatic;
}


/**
* \brief – Get the current state of any compromised log files has occurred. 
* \return – Return true where there has been a log file that been compromised.
*/
template<size_t INDEX>
bool LoggerSignature<INDEX>::hasLogsBeenCompromised()
{
	if (!_logfile_hasbeen_compromised)
		CheckSignature(); //make sure that the log has not been compromised

	return _logfile_hasbeen_compromised;
}

/**
 * \brief –
 * \param files - Pass the compromised log files that have occurred. 
 * \return 
 */
template<size_t INDEX>
bool LoggerSignature<INDEX>::hasLogsBeenCompromised(std::vector<std::string>& files)
{
	if (!_logfile_hasbeen_compromised)
		CheckSignature();

	if (_logfile_hasbeen_compromised)
		get_compromised_logs(files);

	return _logfile_hasbeen_compromised;
}


template <size_t INDEX>
bool LoggerSignature<INDEX>::do_remove_file(std::vector<std::string>::value_type file) const
{
	boost::system::error_code ec;
	auto file_path = boost::filesystem::path(file);
	boost::filesystem::remove(file_path, ec);
	if (ec) return false;
	return true;
}

/**
 * \brief – Delete all of backed up compromised log files. 
 * \param count - Pass the number of files that have been deleted. 
 * \return - Return true when it was able delete all the files. 
 */
template<size_t INDEX>
bool LoggerSignature<INDEX>::deleteCompromisedLogs(size_t & count)
{
	count = 0;
	std::vector<std::string> files;
	if(!get_compromised_logs(files)) return false;
	for(auto file : files)
	{
		if (do_remove_file(file)) count++;
	}

	if (files.size() != count)
		return false;//we were unable to delete all the files that were found.
	_logfile_hasbeen_compromised = false; //clear flag since it was able to delete all the log files. 
	return true;
}

/**
 * \brief – Call this when automatic compromised log file has not been set. 
 * \return - True when it was able to backup the current log file. 
 */
template<size_t INDEX>
 bool LoggerSignature<INDEX>::BackupCompromisedLog()
{
	std::lock_guard<std::mutex> lkg(m_logstream);
	return backup_compromised_log();
}

template <size_t INDEX>
std::string LoggerSignature<INDEX>::CreateLogEntryString(
	boost::posix_time::ptime timestamp,
	const std::string& component, severity_level svl,
	const std::string& message)
{
	std::string outputmessage = message;
	/*
	* Manipulate the input string to add an extra TAB character after each newline
	* This gives a level of indents to the output to set them apart in the log file.
	*/
	{
		boost::algorithm::replace_all(outputmessage, "\n", "\n\t");
	}

	return
		STR_TIMESTAMP_START + boost::posix_time::to_simple_string(timestamp) + STR_TIMESTAMP_END
		+ STR_SEVERITY_START + severity_string(svl) + STR_SEVERITY_END
		+ STR_COMPONENT_START + component + STR_COMPONENT_END
		+ outputmessage
		+ "\n";
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::rolllogs__INTERNAL(bool initial)
{
	// INTERNAL - should not need to lock the mutex

	if (IndexedLogger<INDEX, false>::rolllogs__INTERNAL(initial))
	{
		//i need to change the file mode to in/out (not append) when is present.
		this->_log_OS.close();
		if (boost::filesystem::exists(this->LogFile()))
			this->_log_OS.open(this->LogFile(), std::fstream::in | std::fstream::out);
		else
			this->_log_OS.open(this->LogFile(), std::fstream::in | std::fstream::out | std::fstream::app);

		_valid_signature = verify_stored_signature();
		reset_entries_indexing();
		this->_log_OS.seekp(_end_of_log, std::ios::beg);

		return true;
	}

	return false;
}

template <size_t INDEX>
void LoggerSignature<INDEX>::log__INTERNAL(
	boost::posix_time::ptime timestamp,
	const std::string& component, severity_level svl,
	const std::string& message)
{
	/*
	* This is private and can only be called internally.  Based on that, we can assume that the mutex is SAFE.
	*/
	
	_log_entries_indexed.push_back(LogIndex());
	std::stringstream string_stream_message(message);
	string_stream_message.seekg(0, std::ios::beg);

	std::string output = CreateLogEntryString(
		timestamp, component, svl, message);
	

	auto hash = sign_to_text(output);

	// XXX TODO
	// Add config-file option to completely suppress iostream output.
	// This should enhance performance when enabled.
	std::cout << output;

	if (this->_log_OS.is_open())
	{
		_log_entries_indexed.back().beg_fpos = this->_log_OS.tellp();
		_log_entries_indexed.back().ptime_logged = timestamp;
		_log_entries_indexed.back().length = output.length();
		_log_entries_indexed.back().hash = hash;
		output = "#" + hash + "#" + output;
		this->_log_OS << output;

		_end_of_log = this->_log_OS.tellp();
		_log_entries_indexed.back().end_fpos = _end_of_log;
		if (this->_max_size_bytes > 0)
		{
			if (static_cast<std::size_t>(this->_log_OS.tellp()) >= this->_max_size_bytes)
			{
				// TODO does it matter that the timestamps here match that of the
				// previous log entry, as well as each other? The fact that they match
				// each other may actually be beneficial, since that will provide
				// verification that sequential log files were indeed generated
				// sequentially.
				this->_log_OS << CreateLogEntryString(
					timestamp, STR_COMPONENT_LOG_INTERNAL, severity_level::notification,
					"Log exceeded maximum size; about to roll logs.\nEND OF LOGFILE");
				this->rolllogs__INTERNAL();
				this->_log_OS << CreateLogEntryString(
					timestamp, STR_COMPONENT_LOG_INTERNAL, severity_level::notification,
					"Previous log exceeded maximum size; logs rolled.");
			}
		}
	}
}

template<size_t INDEX>
void LoggerSignature<INDEX>::reset_entries_indexing()
{
	char cha;
	bool multi_line = false;
	bool sign_located = true;
	std::string line;
	// INTERNAL - should not need to lock the mutex

	_log_entries_indexed.clear();

	this->_log_OS.seekg(0, std::ios::beg);
	_end_of_log = this->_log_OS.tellg();

	while (!this->_log_OS.eof())
	{
		this->_log_OS.get(cha);
		line += cha;
		if (cha == '\n')
		{
			if (!multi_line)
			{
				_log_entries_indexed.push_back(LogIndex());
				_log_entries_indexed.back().beg_fpos = _end_of_log;
			}

			if (this->_log_OS.peek() == '\t')
				multi_line = true;
			else
			{
				std::string log;
				multi_line = false;
				_log_entries_indexed.back().end_fpos = this->_log_OS.tellg();

				if (extract_line_hash(line, log, _log_entries_indexed.back().hash))
				{
					LogEntry entry;
					extract_log_elements(line, entry);
					_log_entries_indexed.back().ptime_logged = entry.pTimeLogged;
					_log_entries_indexed.back().length = log.length();
				}
				line.clear();
			}

			if (this->_log_OS.rdstate())
			{
				//newline and eof, then there was no signature at the end of the file
				//do not clear the flags so it would fall through the loop
				sign_located = false;
			}
			
			_end_of_log = this->_log_OS.tellg();
		}
	}

	this->_log_OS.clear();//the eof will always be set at end of the loop
	if(!sign_located)
	{
		this->_log_OS.seekg(0, std::ios::end);
		_end_of_log = this->_log_OS.tellg();
		_log_entries_indexed.back().end_fpos = _end_of_log;
	}
}

template <size_t INDEX>
std::string LoggerSignature<INDEX>::sign_to_text(const std::string& sign) const
{
	std::vector<unsigned char> message(sign.begin(), sign.end());

	auto sstrout = SecurityHelpers::CalculateHash(message, _salt_logentry, Log_REPETITIONS);
	return sstrout;
}

template <size_t INDEX>
void LoggerSignature<INDEX>::store_signature()
{
	RunningSHADigest running_sha_digest(_salt_logfile , Log_REPETITIONS);
	for (auto & element : _log_entries_indexed)
	{
		running_sha_digest.UpdateDigest(element.hash);
	}
	this->_log_OS << running_sha_digest.FinalizeDigestToHash();
	this->_log_OS.seekp(_end_of_log, std::ios::beg); //move the file pointer back to the end of the log
}

template <size_t INDEX>
bool LoggerSignature<INDEX>::verify_stored_signature()
{
	std::ifstream ilogfile(this->LogFile(), std::ifstream::in); //reopen the log file so we can catch any
									   //external modifications.
	if(!ilogfile.is_open())
		return false;

	ilogfile.seekg(0, ilogfile.end);
	if (ilogfile.tellg() <= 0)
	{
		ilogfile.close();
		return true; //is a good log since there is no signature.
	}
	ilogfile.seekg(0, ilogfile.beg);

	char cha;
	std::string line;
	std::string log_line;
	std::string hash;
	RunningSHADigest file_running_sha_digest(_salt_logfile, Log_REPETITIONS);
	
	while (!ilogfile.eof())
	{
		ilogfile.get(cha);
		line += cha;
		if(cha == '\n' && line.at(0)!='\t')
		{
			if (extract_line_hash(line, log_line, hash))
			{
				if (sign_to_text(log_line) != hash)
					break; //if one line has wrong hash, the whole file is invalid.
				file_running_sha_digest.UpdateDigest(hash);
			}
			line.clear();
		}
		else if(cha == '\n')
			line.clear();
	}
	if(line.size() > 0)
		line.pop_back();//is adding an extra character before eof flag sets

	std::string calc_sign = file_running_sha_digest.FinalizeDigestToHash();
	ilogfile.close();

	return (calc_sign == line);
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::remove_data_block(std::fpos<_Mbstatet> start_block, std::fpos<_Mbstatet> end_block)
{
	char ch;
	boost::system::error_code ec;

	//i have been finding this files laying about during testing
	//is a good thing if they are around to remove them.
	if (boost::filesystem::exists(this->_log_path + "log.tmp"))
		boost::filesystem::remove(this->_log_path + "log.tmp");
	if (boost::filesystem::exists(this->_log_path + "log.bak"))
		boost::filesystem::remove(this->_log_path + "log.bak");

	std::ofstream temp_file(this->_log_path + "log.tmp", std::ios::out | std::ios::app);

	//copy data before the block of data to be removed
	this->_log_OS.clear();
	this->_log_OS.seekg(0, std::ios::beg);
	while (this->_log_OS.tellg() <= start_block.seekpos() - 1)
	{
		this->_log_OS.get(ch);
		temp_file.put(ch);
	}

	//copy data after the block of data to be removed
	this->_log_OS.seekg(end_block, std::ios::beg);
	while (this->_log_OS.tellg().seekpos() < _end_of_log)//we want to skip the signature
	{
		this->_log_OS.get(ch);
		temp_file.put(ch);
	}

	this->_log_OS.close();
	temp_file.close();

	//rename the files and removed the old files
	boost::filesystem::rename(this->LogFile(), this->_log_path + "log.bak", ec);
	boost::filesystem::remove(this->LogFile(), ec);
	boost::filesystem::rename(this->_log_path + "log.tmp", this->LogFile(), ec);
	if (ec != boost::system::errc::success)
	{
		//something happened while renaming the log files
		//need to put every thing back
		boost::filesystem::rename(this->_log_path + "log.bak", this->LogFile());
		boost::filesystem::remove(this->_log_path + "log.tmp");
		boost::filesystem::remove(this->_log_path + "log.bak");
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, "Unable to remove data from log");
		return false;
	}
	this->_log_OS.open(this->LogFile(), std::fstream::in | std::fstream::out);

	//reset and save the new signature
	reset_entries_indexing();
	store_signature();
	return true;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::extract_log_elements(std::string log_entry, LogEntry& entry) const
{
	std::string through_away;
	return extract_log_elements(log_entry, entry, through_away);
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::extract_log_elements(std::string log_entry, LogEntry& entry, std::string & hash) const
{
	if (log_entry.empty())
		return false;

	std::string line_entry;
	if (!extract_line_hash(log_entry, line_entry, hash))
		return false;

	std::vector<std::string> elements;
	boost::char_separator<char> sep("[]<>");
	boost::tokenizer<boost::char_separator<char>> tokens(line_entry, sep);
	BOOST_FOREACH(std::string t, tokens)
	{
		elements.push_back(t);
	}
	if (elements.size() == 4)
	{
		boost::posix_time::ptime tmp_ptime(boost::posix_time::time_from_string(elements[0]));
		entry.pTimeLogged = tmp_ptime;
		Logger_Impl::severity_level_from_str(entry.SeverityLevel, elements[1]);

		entry.ModuleName = elements[2];
		elements[3].erase(0, 1);//the split of the log message, left whitespace in front of it
		entry.LogMessage = elements[3];
		entry.isValid = true;
	}
	else
	{
		entry.LogMessage = "LOGGER: unable to extract log entry";
		entry.isValid = false;
	}
	return true;
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::extract_hash(const std::string & cs, std::string & hash) const
{
	std::string through_away;
	return extract_line_hash(cs, through_away, hash);
}

template<size_t INDEX>
bool LoggerSignature<INDEX>::extract_line_hash(std::string full_log_line, std::string & line, std::string & original_hash) const
{
	if (full_log_line.empty())
		return false;

	std::vector<std::string> elements;
	boost::char_separator<char> sep("#");
	boost::tokenizer<boost::char_separator<char>> tokens(full_log_line, sep);
	BOOST_FOREACH(std::string t, tokens)
	{
		elements.push_back(t);
	}

	if (elements.size() == 2)
	{
		original_hash = elements[0];
		line = elements[1];
	}

	return true;
}


#endif
