#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "AuditLog.hpp"
#include "CellCounterFactory.h"
#include "FileSystemUtilities.hpp"
#include "HawkeyeLogicImpl.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_Analysis";

//*****************************************************************************
bool HawkeyeLogicImpl::initializeAnalysis ()

{
	Logger::L ().Log (MODULENAME, severity_level::debug2, "initializeAnalysis: <enter>");

	bool retStatus = AnalysisDefinitionsDLL::Initialize ();

	Logger::L ().Log (MODULENAME, severity_level::debug2, "initializeAnalysis: <exit>");

	return retStatus;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeAnalysisDefinitions (AnalysisDefinition* list, uint32_t num_analyses)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "FreeAnalysisDefinitions: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < num_analyses; i++)
	{
		AnalysisDefinitionDLL::freeAnalysisDefinitionInternal (list[i]);
	}

	delete[] list;

	Logger::L ().Log (MODULENAME, severity_level::debug3, "FreeAnalysisDefinitions: <exit>");
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetAllAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetAllAnalysisDefinitions: <enter>");

	num_ad = 0;
	analyses = nullptr;

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	DataConversion::convert_vecOfDllType_to_listOfCType (AnalysisDefinitionsDLL::Get (), analyses, num_ad);
	if (num_ad == 0)
	{
		return HawkeyeError::eNoneFound;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetAllAnalyses: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetFactoryAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetFactoryAnalysisDefinitions: <enter>");

	num_ad = 0;
	analyses = nullptr;

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}


	std::vector<AnalysisDefinitionDLL>& analysesDLL = AnalysisDefinitionsDLL::Get ();
	if (!analysesDLL.size ())
	{
		return HawkeyeError::eNoneFound;
	}

	analyses = new AnalysisDefinition[analysesDLL.size ()];
	num_ad = 0;

	for (auto& adDLL : AnalysisDefinitionsDLL::Get ()) {
		if (!AnalysisDefinitionDLL::isUserDefined (adDLL.analysis_index)) {
			analyses[num_ad++] = adDLL.ToCStyle ();
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetFactoryAnalysisDefinitions: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetUserAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetUserAnalysisDefinitions: <enter>");

	num_ad = 0;
	analyses = nullptr;

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	std::vector<AnalysisDefinitionDLL>& analysesDLL = AnalysisDefinitionsDLL::Get ();
	if (!analysesDLL.size ())
	{
		return HawkeyeError::eNoneFound;
	}

	analyses = new AnalysisDefinition[analysesDLL.size ()];
	num_ad = 0;

	for (auto& adDLL : analysesDLL) {
		if (!AnalysisDefinitionDLL::isUserDefined (adDLL.analysis_index)) {
			analyses[num_ad++] = adDLL.ToCStyle ();
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetUserAnalysisDefinitions: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// These APIs are only used internally to HawkeyeLogicImpl.
// They are not available in HawkeyeLogic.
//*****************************************************************************

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetTemporaryAnalysisDefinition (AnalysisDefinition*& temp_definition)
{
	temp_definition = nullptr;

	// This must be done by elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eElevated );
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) {
			return status;
		}
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Get Temporary Analysis Definition"));
		return HawkeyeError::eNotPermittedByUser;
	}

	std::shared_ptr<AnalysisDefinitionDLL> pTempAnalysisDefinition = AnalysisDefinitionsDLL::GetTemporaryAnalysis ();
	if (pTempAnalysisDefinition.get () == nullptr)
	{
		temp_definition = nullptr;
		return HawkeyeError::eNoneFound;
	}

	DataConversion::convert_dllType_to_cType (*pTempAnalysisDefinition, temp_definition);

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetTemporaryAnalysisDefinition (AnalysisDefinition* temp_definition)
{
	// This must be done by elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eElevated );
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Set Temporary Analysis Definition"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// ToDo - validate the incoming "temp_definition"
	if (temp_definition == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}

	AnalysisDefinitionsDLL::SetTemporaryAnalysis (temp_definition);

	return HawkeyeError::eSuccess;
}
