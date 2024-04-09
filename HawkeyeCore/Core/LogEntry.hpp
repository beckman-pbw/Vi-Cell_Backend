#pragma once
#include <cstdint>
#include "Logger_auxiliary.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

struct LogEntry 
{
	boost::posix_time::ptime pTimeLogged;
	severity_level SeverityLevel;
	std::string ModuleName;
	std::string LogMessage;
	bool isValid;
};

struct LogIndex 
{
	boost::posix_time::ptime ptime_logged;
	std::fpos<_Mbstatet> beg_fpos;
	std::fpos<_Mbstatet> end_fpos;
	size_t length;
	std::string hash;
};
