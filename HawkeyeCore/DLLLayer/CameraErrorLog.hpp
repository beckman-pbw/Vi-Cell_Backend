#pragma once

#include <string>

class CameraErrorLog
{
public:

	static void Log(const std::string& logMsg);

private:
	CameraErrorLog();
	static void logFirstEntryAfterInit();
	static bool firstEntry_;
};
