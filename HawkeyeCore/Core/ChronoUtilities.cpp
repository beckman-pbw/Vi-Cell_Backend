#include "stdafx.h"
#define __STDC_WANT_LIB_EXT1__ 1

#include "ChronoUtilities.hpp"
#include <iomanip>
#include <sstream>
#include <time.h>
#include "boost\date_time.hpp"

using namespace std;
using namespace chrono;

static const char MODULENAME[] = "ChronoUtilities";

static bool isInitialized_ = false;
static system_TP epochTime_ = {};

system_TP ChronoUtilities::ConvertToTimePoint(
	const string& calendarDateTime,
	const string calendarDateTimeFormat /*= "%b %d %Y %H:%M:%S"*/)
{
	tm tm = {};
	auto ss = stringstream(calendarDateTime);
	ss >> get_time(&tm, calendarDateTimeFormat.c_str());
	tm.tm_isdst = COMMONUTIL_ENABLE_DST;
	system_TP tp = system_clock::from_time_t(_mkgmtime(&tm));
	return tp;
}

string ChronoUtilities::ConvertToString(const system_TP& timePoint, const string calendarDateTimeFormat)
{
	time_t now_c = system_clock::to_time_t(timePoint);

	if (now_c < 0)
		now_c = 1483254000;		// If date < 1970, change to 1/1/2017 (before 1st release of Scout software)
	
	const int bufferLength = 50;
	char timeBuffer[bufferLength];
	tm tm = {};

	gmtime_s(&tm, &now_c);
	tm.tm_hour += COMMONUTIL_ENABLE_DST - tm.tm_isdst;
	strftime(timeBuffer, bufferLength, calendarDateTimeFormat.c_str(), &tm);
	return string(timeBuffer);
}

time_t ChronoUtilities::ConvertToLocalTime (system_TP timePoint)
{
	return std::chrono::system_clock::to_time_t (timePoint);
}

string ChronoUtilities::GetFutureDateAsString(const uint16_t& daysFromNow,
	const string calendarDateTimeFormat)
{
	system_TP now = system_clock::now();
	now += duration_days(daysFromNow);
	return ConvertToString(now, calendarDateTimeFormat);
}

uint64_t ChronoUtilities::GetDaysFromDate(const string& futureCalendarDate, const string calendarDateTimeFormat)
{
	auto tp = ConvertToTimePoint(futureCalendarDate, calendarDateTimeFormat);
	auto now = chrono::system_clock::now();

	tp -= duration_days(ConvertFromTimePoint<duration_days>(now));
	return ConvertFromTimePoint<duration_days>(tp);
}

system_TP ChronoUtilities::CurrentTime()
{
	return system_clock::now();
}

std::string ChronoUtilities::ConvertToDateString(const uint64_t & days_from_epoch)
{
	const auto epochDate = boost::gregorian::date{ 1970, 1, 1 }; // Set Epoch time as "1970-01-01"
	const auto daysSinceEpoch = boost::gregorian::days(static_cast<long>(days_from_epoch));

	return boost::gregorian::to_simple_string(epochDate + daysSinceEpoch);
}

system_TP ChronoUtilities::getEpochTime()
{
	if (isInitialized_)
	{
		return epochTime_;
	}
	std::tm fixedTime = {};

	// Set fixed time as 01/01/1970 00:00:00
	fixedTime.tm_year = 70;
	fixedTime.tm_mon = 0;
	fixedTime.tm_mday = 1;
	fixedTime.tm_hour = 0;
	fixedTime.tm_min = 0;
	fixedTime.tm_sec = 0;

	epochTime_ = (system_clock::from_time_t(_mkgmtime(&fixedTime)));
	isInitialized_ = true;	

	return epochTime_;
}

/**
 * \brief Convert the two incoming timestamps to postgres date string values that can be used in an SQL BETWEEN clause.
 * \param start starting time of the filter in a Unix Epoch format.
 * \param end ending time of the filter in a Unix Epoch format.
 * \param sTimeStr starting time string formatted in postgres format, ie: 2020-09-07.
 * \param eTimeStr ending time string formatted in postgres format, ie: 2020-09-13.
 */
void ChronoUtilities::getListTimesAsStr (uint64_t start, uint64_t end, std::string& sTimeStr, std::string& eTimeStr)
{
	auto stime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(start);
	// Must add a day to the end time to get records from the specified end time.
	auto etime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(end + SECONDSPERDAY);

	if (end == 0)
	{ // Use the current date/time.
		uint64_t time = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());
		// Must add a day to the end time to get records from the specified end time.
		time += SECONDSPERDAY;
		etime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time);
	}

	sTimeStr = ChronoUtilities::ConvertToString (stime, "%Y-%m-%d");
	eTimeStr = ChronoUtilities::ConvertToString (etime, "%Y-%m-%d");
}

/**
 * \brief Convert the two incoming timestamps to postgres timestamp string values that can be used in an SQL BETWEEN clause.
 * \param start starting time of the filter in a Unix Epoch format.
 * \param end ending time of the filter in a Unix Epoch format.
 * \param sTimeStr starting time string formatted in postgres format, ie: '2020-09-07 00:00:00'::timestamp
 * \param eTimeStr ending time string formatted in postgres format, ie: '2020-09-13 23:59:00'::timestamp
 */
void ChronoUtilities::getListTimestampAsStr(uint64_t start, uint64_t end, std::string& sTimeStr, std::string& eTimeStr)
{
	const auto startTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(start);
	// Must add a day to the end time to get records from the specified end time.
	auto endTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(end);

	if (end == 0)
	{ // Use the current date/time.
		uint64_t time = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());
		// Must add a day to the end time to get records from the specified end time.
		time += SECONDSPERDAY;
		endTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time);
	}

	sTimeStr = ChronoUtilities::ConvertToString(startTime, "'%F %T'::timestamp");
	eTimeStr = ChronoUtilities::ConvertToString(endTime, "'%F %T'::timestamp");
}

bool ChronoUtilities::IsZero (const system_TP& tp)
{
	system_TP zeroTP;
	return (tp == zeroTP ? true : false);
}
