#pragma once

// PLEASE NOTE: This file should NOT be included EXCEPT by `.cpp` files that
// INSTANTIATE a logger with a specified INDEX.

#define BOOST_LOG_DYN_LINK 1

#include "Logger.hpp"
#include "Logger_auxiliary.hpp"

#include <iostream>
#include <string>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>

#include "../../target/properties/HawkeyeCore/dllversion.hpp"


namespace errc = boost::system::errc;
using namespace Logger_Impl;


static const auto STR_COMPONENT_LOG_INTERNAL = "Logger";


template <size_t INDEX, bool ROLL_INITIAL>
	IndexedLogger<INDEX, ROLL_INITIAL>::IndexedLogger()
	: _log_path("\\Instrument\\Logs")
	, _log_file("")
	, _always_flush(false)
	, _do_console_output(false)
	, _config_file("")
	, _max_size_bytes(5*1024*1024)
	, _max_roll_files(25)
	, log_level(severity_level::normal)
	, _enabled(true)
{
	// Logger initially uses cout so that we have something workable immediately.
	// ReadFromConfig may prompt us to do a log file as well, but that's for later on.
	boost::system::error_code ec;
	Init(ec);
	// No error can occur, because we are not initializing a log file, so ignore ec.
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Init(boost::system::error_code& ec)
{
	std::lock_guard<std::mutex> lkg(m_logstream);
	if (_log_OS.is_open())
	{
		_log_OS.close();
	}

	// If attempting to create a log file:
	if (!_log_file.empty())
	{
        if ( _log_path.empty() )
        {
            _log_path = "\\Instrument\\Logs";
        }

        boost::filesystem::path p = LogFile();
        if (boost::filesystem::exists(p) && !boost::filesystem::is_regular_file(p))
		{
			log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Log file \"" + _log_file + "\" is not a regular file; will log to console ONLY.");
			ec = ::errc::make_error_code(::errc::bad_file_descriptor);
			_log_file = "";
		}
		else
		{
			std::string failure_msg;
			if (!CreatePathIfNotExist(_log_path, failure_msg))
			{
				std::string msg = boost::str(boost::format("Unable to create logging directory \"%s\".  Continuing with CWD.") % _log_path);
				log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, msg);

				_log_path = ".\\";
			}

			if (!rolllogs__INTERNAL(true))
			{
				// Failure mode has basically already been addressed.
				// Worst case: we're logging console-only.
				_do_console_output = true;
			}
		}

	}
	// We have either opened the log file after rolling the old ones OR we are operating on console alone.
	// Either way, that's the best we can do.

	_flush_time = boost::posix_time::second_clock::local_time() + boost::posix_time::seconds(60);
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::CreatePathIfNotExist(std::string pth, std::string& failure_msg)
{
	/*
	 * Attempt to create the directory path "pth" (and all components thereof).
	 * We will log to the console and also pass the message up to our caller so that
	 * they can try to write it to a file after whatever remediation steps are performed.
	 *
	 */

	namespace fs = boost::filesystem;
	namespace errc = boost::system::errc;

	fs::path p(pth);

	boost::system::error_code ec;

	bool exists = fs::exists(p, ec);
	if (ec && ec != boost::system::errc::no_such_file_or_directory)
	{
		failure_msg = boost::str(boost::format("While creating log path \"%s\", failure \"%s\" while checking if path exists.") % pth % ec.message());
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, failure_msg);
		return false;
	}

	if (exists)
	{
		bool is_dir = fs::is_directory(p, ec);
		if (ec)
		{
			failure_msg = boost::str(boost::format("While creating log path \"%s\", failure \"%s\" while checking if path is a directory") % pth % ec.message());
			log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, failure_msg);
			return false;
		}

		if (!is_dir)
		{
			failure_msg = boost::str(boost::format("While creating log path \"%s\", the path already exists but is NOT a directory.") % pth);
			log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, failure_msg);
			return false;
		}

	}

	// If the directory path already exists, we are completed.
	if (exists)
		return true;

	bool created = fs::create_directories(p, ec); // c_d's returns false if no directory is created, but that's not necessarily an error!

	if (!created && ec)
	{
		failure_msg = boost::str(boost::format("While creating log path \"%s\", failure \"%s\" while creating the directory.") % pth % ec.message());
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, failure_msg);
		return false;
	}

	return true;
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::rolllogs__INTERNAL(bool initial)
{
	/*
	 * Moves all existing log file backups out of the way and then
	 * closes the existing log, backs it up to .1 and opens a new log file.
	 *
	 * On error, the existing log file will remain active.
	 *
	 * This is an INTERNAL function, so mutex has already been acquired.
	 */
	
	// If no log file is configured, then this is not a useful task.
	if (_log_file.empty())
		return true;

	// A different way of indicating that no log is configured...
	if (!_log_OS.is_open() && !initial)
		return true;
	
	if (!ROLL_INITIAL)
	{
		if (initial)
		{
			assert(!_log_OS.is_open());

			// Open the OLD log, if it exists; otherwise start a new one.
			_log_OS.open(LogFile(), std::fstream::in | std::fstream::out | std::fstream::app);
			if ( !_log_OS.good() )
			{
				_log_OS.close();
				std::string msg = boost::str(boost::format("Unable to continue existing log file %s") % LogFile() );
				log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
				// XXX TODO INTEGRATION - ERROR-PATHS
				// Should we simply roll the logs as usual if this happens?
				return false;
			}
			return true;
		}
	}

	std::string failure_msg;
	// Move most recent log out of the way.
	if ( !rolllog__INTERNAL(1, failure_msg) )
	{
		// failure message has already been recorded
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Continuing with existing log file.");
		return false;
	}

	// Move the current log to log.1
	{
		namespace fs = boost::filesystem;
		fs::path p(LogFile());
		fs::path p1(LogFile() + std::string(".1"));
		boost::system::error_code ec;

		bool exists = fs::exists(p, ec);
		if (ec && ec != boost::system::errc::no_such_file_or_directory)
		{
			std::string msg = boost::str(boost::format("While rolling logs, failure \"%s\" while checking existence of log file \"%s\". Continuing with existing log file.") % ec.message() % LogFile() );
			log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, msg);
			return false;
		}

		if (exists)
		{
			// Close the current log
			if (_log_OS.is_open())
				_log_OS.close();

			fs::rename(p, p1, ec);
			if(ec)
			{
				// Failure.  Attempt to re-open previous log.
				_log_OS.open(LogFile(), std::fstream::in | std::fstream::out | std::fstream::app);
				if ( !_log_OS.good() )
				{
					_log_OS.close();
					std::string msg = boost::str(boost::format("While rolling logs, renaming existing log file \"%s\" failed (\"%s\"); unable to reopen after failure.") % LogFile() % ec.message() );
					log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
					return false;
				}

				std::string msg = boost::str(boost::format("While rolling logs, renaming existing log file \"%s\" failed (\"%s\"); Continuing with current log.") % LogFile() % ec.message() );
				log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::error, msg);
				return false;
			}
		}

	}

	// Open a new log.
	_log_OS.open(LogFile(), std::fstream::in | std::fstream::out | std::fstream::app);
	if ( !_log_OS.good() )
	{
		_log_OS.close();
		std::string msg = boost::str(boost::format("While rolling logs, unable to open new log %s") % LogFile() );
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
		return false;
	}

	return true;
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::rolllog__INTERNAL(unsigned int log_n, std::string& failure_msg)
{
	/*
	 * Recursively roll log file "n" to log file "n+1"
	 *
	 * Naming convention for rolled logs:
	 * 		_log_path/_log_file.n
	 * 		The lowest number 'n' will be the most recent log.
	 * 		When 'n' exceeds _max_roll_files, that file will be erased.
	 *
	 * Return shortcuts:
	 * 		If log_n does not exist: no need to roll it.
	 * 		If log_n+1 exceeds our retention spec: delete log_n.
	 */
	namespace fs = boost::filesystem;

	unsigned int next_log = log_n + 1;
	std::string log_name_n = boost::str(boost::format("%s.%d") % LogFile() % log_n );
	std::string log_name_next = boost::str(boost::format("%s.%d") % LogFile() % (log_n + 1) );


	fs::path p(log_name_n);
	fs::path p1(log_name_next);
	boost::system::error_code ec;


	// Exit shortcut: log_n does not exist
	bool exists = fs::exists(p, ec);
	if (ec && ec != boost::system::errc::no_such_file_or_directory)
	{
		std::string msg = boost::str(boost::format("While rolling log \"%s\", failure \"%s\" while checking if file exists") % log_name_n % ec.message() );
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
		return false;
	}
	if (!exists)
		return true;


	// Exit shortcut: log_n+1 would exceed our retention spec, so it can just be deleted.
	if (next_log > _max_roll_files)
	{
		// Delete log_n
		fs::remove(p, ec);
		if (ec)
		{
			std::string msg = boost::str(boost::format("While rolling log \"%s\", failure \"%s\" while removing file") % log_name_n % ec.message() );
			log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
			return false;
		}

		return true;
	}


	// Recurse to make room for rolling this log.
	if (!rolllog__INTERNAL((log_n + 1), failure_msg) )
	{
		return false;
	}

	// Rename

	fs::rename(p, p1, ec);
	if (ec)
	{
		std::string msg = boost::str(boost::format("While rolling log \"%s\", failure \"%s\" while renaming log file.") % log_name_n % ec.message() );
		log__INTERNAL(boost::posix_time::microsec_clock::local_time(), STR_COMPONENT_LOG_INTERNAL, severity_level::critical, msg);
		return false;
	}


	return true;
}

template <size_t INDEX, bool ROLL_INITIAL>
std::string IndexedLogger<INDEX, ROLL_INITIAL>::LogFile()
{
	// Ensure the path-separator is correctly specified
	boost::filesystem::path p = _log_path;
	return (p / _log_file).string();
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Log(const std::string& component, severity_level svl, const std::string& message)
{
	if (!_enabled)
		return;

	// Discard if below our current level of interest
	if (IsBelowLevelOfInterest (svl))
		return;

	Log_knownTimestamp(boost::posix_time::microsec_clock::local_time(), component, svl, message);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Log_knownTimestamp(
		boost::posix_time::ptime timestamp,
		const std::string& component, severity_level svl, const std::string& message)
{
	std::lock_guard<std::mutex> lkg(m_logstream);
	if (timestamp >= _flush_time)
	{
		/*
		* Update the flush time BEFORE calling Flush to avoid a potential
		* Flushing loop if we have to log a failure during the flush.
		*/
		_flush_time = timestamp + boost::posix_time::seconds(60);
		flush__INTERNAL();
	}

	log__INTERNAL(timestamp, component, svl, message);

	if (_always_flush) {
		flush__INTERNAL();
	}
}

template <size_t INDEX, bool ROLL_INITIAL>
std::string IndexedLogger<INDEX, ROLL_INITIAL>::CreateLogEntryString(
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
		+ STR_THREAD_START + boost::str (boost::format ("%08X") % boost::lexical_cast<std::string>(boost::this_thread::get_id())) + STR_THREAD_END
		+ STR_SEVERITY_START + severity_string(svl) + STR_SEVERITY_END
		+ STR_COMPONENT_START + component + STR_COMPONENT_END
		+ outputmessage
		+ "\n";
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::log__INTERNAL(
		boost::posix_time::ptime timestamp,
		const std::string& component, severity_level svl,
		const std::string& message)
{
	/*
	 * This is private and can only be called internally.  Based on that, we can assume that the mutex is SAFE.
	 */

	std::string output = CreateLogEntryString (timestamp, component, svl, message);
	
	// XXX TODO
	// Add config-file option to completely suppress iostream output.
	// This should enhance performance when enabled.
	bool do_output = _do_console_output;

	if (_log_OS.is_open())
	{
		_log_OS << output;

		if (_max_size_bytes > 0)
		{
			if ((std::size_t)_log_OS.tellp() >= _max_size_bytes)
			{
				// the timestamps here match that of the previous log entry, as
				// well as each other. The fact that they match each other should
				// actually be beneficial, since that will provide verification
				// that sequential logfiles were indeed generated sequentially.
				_log_OS << CreateLogEntryString(
						timestamp, STR_COMPONENT_LOG_INTERNAL, severity_level::notification,
						"Log exceeded maximum size; about to roll logs.\nEND OF LOGFILE");

				rolllogs__INTERNAL();

				_log_OS << CreateLogEntryString(
						timestamp, STR_COMPONENT_LOG_INTERNAL, severity_level::notification,
						"Previous log exceeded maximum size; logs rolled.");

				_log_OS << CreateLogEntryString(
						timestamp, STR_COMPONENT_LOG_INTERNAL, severity_level::notification,
						boost::str( boost::format( "DLL software version : %s\n" ) % DLL_Version ) );
			}
		}
	}
	else
	{
		do_output = true;
	}

	if ( do_output )
	{
		std::cout << output << std::endl;
	}
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Flush()
{
	std::lock_guard<std::mutex> lkg(m_logstream);
	flush__INTERNAL();
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::flush__INTERNAL()
{
	// Explicitly flush stdout.
	// XXX TODO
	// Add config-file option to completely suppress iostream output.
	// This should enhance performance when enabled.
	bool do_output = _do_console_output;

	if ( _log_OS.is_open() )
	{
		_log_OS.flush();
	}
	else
	{
		do_output = true;
	}

	if ( do_output )
	{
		std::cout.flush();
	}
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Out(const std::string& component, const std::string& message)
{
	Log(component, severity_level::normal, message);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Warn(const std::string& component, const std::string& message)
{
	Log(component, severity_level::warning, message);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Err(const std::string& component, const std::string& message)
{
	Log(component, severity_level::error, message);
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::IsBelowLevelOfInterest(severity_level svl)
{
	return !IsOfInterest(svl);
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::IsOfInterest(severity_level svl)
{
	std::lock_guard<std::mutex> lkg(m_sens);
	return svl >= log_level;
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::IncreaseLoggingSensitivity()
{
	std::lock_guard<std::mutex> lkg(m_sens);
	switch (log_level)
	{
		case severity_level::notification:
			log_level = severity_level::normal;
			break;

		case severity_level::warning:
			log_level = severity_level::notification;
			break;

		case severity_level::error:
			log_level = severity_level::warning;
			break;

		case severity_level::critical:
			log_level = severity_level::error;
			break;

		case severity_level::console:
			log_level = severity_level::critical;
			break;

		case severity_level::normal:
			log_level = severity_level::debug1;
			break;

		case severity_level::debug1:
			log_level = severity_level::debug2;
			break;

		case severity_level::debug2:
			log_level = severity_level::debug3;
			break;

		case severity_level::debug3:
		default:
			break;
	}
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::DecreaseLoggingSensitivity()
{
	std::lock_guard<std::mutex> lock(m_sens);
	switch (log_level)
	{
		case severity_level::debug3:
			log_level = severity_level::debug2;
			break;
		case severity_level::debug2:
			log_level = severity_level::debug1;
			break;
		case severity_level::debug1:
			log_level = severity_level::normal;
			break;
		case severity_level::normal:
			log_level = severity_level::notification;
			break;
		case severity_level::notification:
			log_level = severity_level::warning;
			break;
		case severity_level::warning:
			log_level = severity_level::error;
			break;
		case severity_level::error:
			log_level = severity_level::critical;
			break;

		case severity_level::critical:
			log_level = severity_level::console;
			break;

		case severity_level::console:
		default:
			break;
	}
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::SetLoggingSensitivity(severity_level svl)
{
	std::lock_guard<std::mutex> lock(m_sens);
	log_level = svl;
}

template <size_t INDEX, bool ROLL_INITIAL>
severity_level IndexedLogger<INDEX, ROLL_INITIAL>::GetLoggingSensitivity()
{
	std::lock_guard<std::mutex> lock(m_sens);
	return log_level;
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::InitializeOutputFile(boost::system::error_code& ec, const std::string& lFile, const std::string& lPath)
{
	if (!lFile.empty()) _log_file = lFile;
	if (!lPath.empty()) _log_path = lPath;

	_always_flush = true;

	Init(ec);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Initialize(boost::system::error_code& ec, const std::string& cfg, const std::string& config_section, t_pPTree cfg_log_overrides)
{
	// Get config info from file
	_config_file = cfg;
	_config_section = config_section;
	ReadConfiguration(ec);

	if (ec)
		return;

	// Apply overrides
	if (cfg_log_overrides) GetCfgFromPTree(*cfg_log_overrides, ec);

	if (!ec)
		Init(ec);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::Initialize (boost::system::error_code& ec, const LoggerSettings_t& settings)
{
	if (ec)
		return;

	if (!ec)
	{
		_log_file = settings.logFile;
		log_level = settings.logLevel;
		_max_size_bytes = settings.maxSizeBytes;
		_max_roll_files = settings.maxLogRolls;
		_always_flush = settings.alwaysFlush;
		_do_console_output = settings.doConsoleOutput;
		Init(ec);
	}
}

/*
 * config
 * {
 * 		logger
 * 		{
 * 			logfile [file]
 * 			sensitivity [NORM|NTFY|ERRR|CRIT]
 * 			max_size_bytes [bytes]
 * 			TESTING__always_flush [bool]
			TESTING__do_console_output [bool]
 * 		}
 * }
 */
template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::ReadConfiguration (boost::system::error_code& ec)
{
	ec = errc::make_error_code(::errc::success);

	if (_config_file.empty())
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration requested but no file specified");
		ec = errc::make_error_code(errc::invalid_argument);
		return;
	}

	t_pPTree pt = ConfigUtils::OpenConfigFile(_config_file, ec);
	if (ec == ::errc::no_such_file_or_directory)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration file \"" + _config_file + "\" does not exist or is not a regular file.");
		return;
	}
	
	if (ec == ::errc::invalid_argument)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Configuration file \"" + _config_file + "\" type could not be determined.");
		return;
	}
	
	if (ec == ::errc::io_error)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Unable to parse configuration file \"" + _config_file + "\".");
		return;
	}

	t_opPTree config = (*pt).get_child_optional("config");
	if (!config)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Error parsing configuration file \"" + _config_file + "\" - \"config\" section not found");
		ec = ::errc::make_error_code(::errc::io_error );
		return;
	}


	t_opPTree module = config->get_child_optional(_config_section);
	if (!module)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Error parsing configuration file \"" + _config_file + "\" - \"config." + _config_section + "\" section not found");
		ec = ::errc::make_error_code(::errc::io_error);
		return;
	}

	GetCfgFromPTree(module.get(),ec);
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::GetCfgFromPTree(boost::property_tree::ptree& module, boost::system::error_code& ec)
{
	// Throughout, if a value is missing, preserve the old value, except for the log file as the there is no default log file name.
	ec = errc::make_error_code(::errc::success);

	auto file_op = module.get_optional<std::string>("logfile");
	if (!file_op || file_op.get().empty())
	{
		ec = ::errc::make_error_code(::errc::io_error);
		return;
	}
	_log_file = file_op.get();
	_log_path = module.get<std::string>("logpath", "\\Instrument\\Logs");

	std::string svLvl = module.get<std::string>("sensitivity", severity_string(log_level));
	auto read_lvl_success = severity_level_from_str(log_level, svLvl);
	if (!read_lvl_success)
	{
		Log(STR_COMPONENT_LOG_INTERNAL, severity_level::error, "Invalid sensitivity setting in \"" + _config_file + "\"; ignoring and using previous value of " + severity_string(log_level));
	}

	_max_size_bytes = module.get<std::size_t>("max_size_bytes", _max_size_bytes);
	_max_roll_files = module.get<std::size_t>("max_log_rolls", _max_roll_files);
	_always_flush = module.get<bool>("TESTING__always_flush", _always_flush);
	_do_console_output = module.get<bool>( "TESTING__do_console_output", _do_console_output );
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::SetAlwaysFlush(bool state)
{
	_always_flush = state;
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::SetMaxSizeBytes(uint32_t max)
{
	_max_size_bytes = max;
}

template <size_t INDEX, bool ROLL_INITIAL>
void IndexedLogger<INDEX, ROLL_INITIAL>::SetConsoleOutput( bool state )
{
	_do_console_output = state;
}

template <size_t INDEX, bool ROLL_INITIAL>
bool IndexedLogger<INDEX, ROLL_INITIAL>::StringToSeverityLevel (severity_level& severityLevel, const std::string& str)
{
	return severity_level_from_str (severityLevel, str);
}

