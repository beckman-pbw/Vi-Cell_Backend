#pragma once

#include <atomic>
#include <mutex>

#include <boost/optional.hpp>

#include "AnalysisDefinitionDLL.hpp"
#include "HawkeyeServices.hpp"
#include "ReagentRFIDLayout.hpp"
#include "ResultDefinitionDLL.hpp"

class ReagentDataPrivate
{
public:
	ReagentDataPrivate()
	{
		clear();
	}

	void clear()
	{
		analysisDefinitions.clear();
		cleaningInstructions.clear();
		reagentContainerStates.clear();
		reagentInfoRecords.clear();
		reagentDefinitions.clear();
		reagentValveMaps.clear();
		isExpired = true;
		isEmpty = true;
	}

	std::vector<AnalysisDefinitionDLL> analysisDefinitions;
	std::vector<ReagentCleaningInstruct_t> cleaningInstructions;
	std::vector<ReagentContainerStateDLL> reagentContainerStates;

	std::vector<ReagentInfoRecordDLL> reagentInfoRecords;
	std::map<uint16_t, ReagentDefinitionDLL> reagentDefinitions;
	std::map<size_t, std::vector<ReagentValveMap_t>> reagentValveMaps;
	bool isExpired;
	bool isEmpty;
};
