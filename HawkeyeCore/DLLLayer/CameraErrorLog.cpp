#include "stdafx.h"

#include "CameraErrorLog.hpp"
#include "Logger.hpp"
#include "ChronoUtilities.hpp"

bool CameraErrorLog::firstEntry_ = false;

CameraErrorLog::CameraErrorLog()
{
}

void CameraErrorLog::logFirstEntryAfterInit()
{
	if (!firstEntry_)
	{
		Logger::L().Log("CamErrorLog", severity_level::debug1, "-");
		CameraErrorLogger::L().Log("CamErrorLog", severity_level::debug1, "\n================================================\n");
		firstEntry_ = true;
	}
}

void CameraErrorLog::Log(const std::string& logMsg)
{
	if (!firstEntry_)
	{
		logFirstEntryAfterInit();
	}

	auto currentTimeAsStr = ChronoUtilities::ConvertToString(ChronoUtilities::CurrentTime());
	currentTimeAsStr.append(" | " + logMsg + "\n");
	Logger::L().Log("CamErrorLog", severity_level::debug1, currentTimeAsStr);
	CameraErrorLogger::L().Log("CamErrorLog", severity_level::debug1, currentTimeAsStr);
}
