#include "stdafx.h"

#include "ActiveDirectoryGroups.h"
#include "SystemErrors.hpp"
#include "UserLevels.hpp"

static std::string MODULENAME = "HawkeyeResultsDataManager";

//*****************************************************************************
const char* getPermissionLevelAsStr (UserPermissionLevel permission)
{
	return "Unknown";
}

//*****************************************************************************
extern char* getAdGroups_extern (GoString p0, GoString p1, GoString p2, GoUint8 p3, GoUint8 p4, GoString p5)
{
	return "Unknown";
}

static SystemStatus systemStatusInit = {};
//*****************************************************************************
ReportSystemError::ReportSystemError() :
	_systemStatus(systemStatusInit)
{
}

//*****************************************************************************
ReportSystemError::~ReportSystemError()
{}

//*****************************************************************************
void ReportSystemError::ReportError(uint32_t errorCode, bool logDescription)
{
}
