#pragma once

#include <iostream>
#include <chrono>
#include <ratio>

//template<typename duration>
//using system_duration_TP = std::chrono::time_point<std::chrono::system_clock, duration>;

typedef std::chrono::system_clock::time_point system_TP;
typedef std::chrono::duration<uint64_t, std::ratio<86400>> duration_days;

#define SECONDSPERDAY 86400

#define COMMONUTIL_ENABLE_DST 0

#define SECONDSPERDAY 86400
#define HOURSPERDAY 24

class ChronoUtilities
{
public:
	template<typename T>
	static system_TP ConvertToTimePoint(const uint64_t& durationSinceEpoch)
	{
		auto epochTime = std::chrono::time_point_cast<T>(getEpochTime());
		epochTime += T(durationSinceEpoch);
		return epochTime;
	}

	template<typename T>
	static system_TP ConvertToLocalTimePoint(const uint64_t& durationSinceEpoch)
	{
		auto epochTime = std::chrono::system_clock::time_point();
		epochTime += T(durationSinceEpoch);
		return epochTime;
	}

	template<typename T>
	static uint64_t ConvertFromTimePoint(const system_TP& tp)
	{
		auto difference = tp - getEpochTime();
		return std::chrono::duration_cast<T>(difference).count();		
	}

	static system_TP ConvertToTimePoint(
		const std::string& calendarDateTime,
		std::string calendarDateTimeFormat = "%b %d %Y %H:%M:%S");

	static std::string ConvertToString(
		const system_TP& timePoint,
		std::string calendarDateTimeFormat = "%b %d %Y %H:%M:%S");

	static time_t ConvertToLocalTime (system_TP timePoint);

	/*
	Get a string representing a future date with given number
	of days from now.
	*/
	static std::string GetFutureDateAsString(const uint16_t& daysFromNow,
	                                         std::string calendarDateTimeFormat = "%b %d %Y");

	/*
	Get number of days remaining from now till given date in future.
	*/
	static uint64_t GetDaysFromDate(const std::string& futureCalendarDate,
	                                std::string calendarDateTimeFormat = "%b %d %Y");

	static system_TP CurrentTime();

	/*
	Takes number of days since epoch as input parameter and returns the calendar date as string in "YYYY-MMM-DD" format
	*/
	static std::string ConvertToDateString(const uint64_t& days_from_epoch);

	static void getListTimesAsStr (uint64_t start, uint64_t end, std::string& sTimeStr, std::string& eTimeStr);
	static void getListTimestampAsStr(uint64_t start, uint64_t end, std::string& sTimeStr, std::string& eTimeStr);
	static bool IsZero (const system_TP& tp);

private:
	static system_TP getEpochTime();
};
