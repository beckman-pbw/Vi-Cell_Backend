#pragma once

#include <cassert>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <vector>
#include <string>

#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Configuration.hpp"

enum class severity_level
{
	debug3,         // Lowest severity
	debug2,
	debug1,
	normal,         // Typical sensitivity level
	notification,
	warning,
	error,
	critical,
	console         // Dedicated super-high-priority level for things that MUST hit the console.
};

typedef struct {
	std::string logFile;
	severity_level logLevel;
	size_t maxSizeBytes;
	size_t maxLogRolls;
	bool alwaysFlush;
	bool doConsoleOutput;
} LoggerSettings_t;

template <size_t INDEX, bool ROLL_INITIAL>
class IndexedLogger
{
public:

	static IndexedLogger& L()
	{
		static IndexedLogger INSTANCE;
		return INSTANCE;
	}

	virtual ~IndexedLogger()
	{
		if (_log_OS.is_open()) _log_OS.close();
	}

	// If separate IndexedLogger instances exist in one process, each IndexedLogger instance
	// MUST be initialized with separate log files.
	virtual void Initialize (boost::system::error_code& ec, const std::string& cfg, const std::string& config_section="logger", t_pPTree cfg_log_overrides = t_pPTree{nullptr});
	virtual void Initialize (boost::system::error_code& ec, const LoggerSettings_t& settings);
	virtual void InitializeOutputFile (boost::system::error_code& ec, const std::string& file = "", const std::string& path = "");

	/*
	 * Write a message to the log.
	 * If "message" is a multi-line string (split on line feeds), then all lines past the first will be indented
	 * with an extra TAB character in the output.
	 */
	void Log(const std::string& component, severity_level svl, const std::string& message);
	// NOTE: this call *DOES NOT* respect the `Enable()`/`Disable()` flag.
	void Log_knownTimestamp(
			boost::posix_time::ptime timestamp,
			const std::string& component, severity_level svl, const std::string& message);
	virtual void Flush();

	void Out(const std::string& component, const std::string& message);
	void Warn(const std::string& component, const std::string& message);
	void Err(const std::string& component, const std::string& message);

	/*
	 * Sensitivity
	 * 		Determines the minimum severity_level that will pass through to the logs.
	 * 		Default is "warning"
	 *
	 * 		Increasing the sensitivity will allow lower severity levels to pass through.
	 */
	bool IsBelowLevelOfInterest(severity_level svl);
	bool IsOfInterest(severity_level svl);
	void IncreaseLoggingSensitivity();
	void DecreaseLoggingSensitivity();
	void SetLoggingSensitivity(severity_level svl);
	severity_level GetLoggingSensitivity();
	void SetAlwaysFlush(bool state);
	void SetMaxSizeBytes(uint32_t max);
	void SetConsoleOutput( bool state );
	bool StringToSeverityLevel (severity_level& severityLevel, const std::string& str);

	void Enable() { _enabled = true; }
	void Disable() { _enabled = false; }
	
	std::string LogFile();

protected:
	IndexedLogger();

	virtual void log__INTERNAL(
		boost::posix_time::ptime timestamp,
		const std::string& component, severity_level svl,
		const std::string& message);
	virtual void flush__INTERNAL();

	/*
	* The externally-visible "Log" function will engage the mutex, but it
	* may need to record log messages during the logging process - turns out that
	* calling "Log" from "Log" deadlocks the mutex.
	* External users will call "Log" which will lock the mutex.  Operations DRIVEN from
	* "Log" will now call a private call that assumes the mutex is already claimed
	* somewhere up the call chain.
	*/
	virtual std::string CreateLogEntryString(
		boost::posix_time::ptime timestamp,
		const std::string& component, severity_level svl,
		const std::string& message);

	virtual bool rolllogs__INTERNAL(bool initial = false);
	virtual bool rolllog__INTERNAL(unsigned int log_n, std::string& failure_msg);
	virtual void ReadConfiguration(boost::system::error_code& ec);
	
	void GetCfgFromPTree(boost::property_tree::ptree&, boost::system::error_code& ec);

	std::fstream _log_OS;
	std::string _log_path;
	std::string _log_file;
	bool _always_flush;
	bool _do_console_output;
	std::string _config_file;
	std::string _config_section;

	std::size_t _max_size_bytes;
	std::size_t _max_roll_files;

	/*
	* Synchronization mutexes.
	* m_logstream is acquired as a RULE by the INNERMOST PUBLICLY ACCESSIBLE FUNCTION that
	* the user calls.
	* Internal functions may be calling other accessors for the resource (ex: log calling flush / roll) 
	 *and that's a sure path to deadlock.
	*/
	std::mutex m_logstream; // Protects _log_OS
	std::mutex m_sens; // Protects log_level

private:
	
	void Init(boost::system::error_code& ec);
	bool CreatePathIfNotExist(std::string pth, std::string& failure_msg);

	severity_level log_level;

	boost::posix_time::ptime _flush_time;

	bool _enabled;
};


// Define the standard singleton logger type
using Logger = IndexedLogger<0, true>;
using CameraErrorLogger = IndexedLogger<1, false>;


// NO: This is *only* included when *specializing* the IndexedLogger template,
// i.e. when defining a logger for a particular index.
// #include "Logger_Impl.hxx"
