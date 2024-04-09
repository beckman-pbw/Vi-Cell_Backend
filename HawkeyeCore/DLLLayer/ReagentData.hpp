#pragma once

#include <iostream>
#include <mutex>
#include <vector>

#include "HawkeyeServices.hpp"
#include "ReagentRFIDLayout.hpp"
#include "ResultDefinitionDLL.hpp"

class ReagentData
{
public:
	void Initialize();
	void Decode (const std::vector<RfidTag_t>& tags);
	void SetToInvalid();
	bool addMissingReagentDefinitions (std::map<uint16_t, ReagentDefinitionDLL>& input);
	bool isPackUsable() const;
	bool isExpired() const;
	bool isNearExpiration (bool reportError, uint32_t &daysRemaining, uint64_t timeRequired) const;	// within the last day of expiration
	bool isEmpty() const;

	std::vector<AnalysisDefinitionDLL>& GetAnalysisDefinitions();
	std::vector<ReagentCleaningInstruct_t>& GetCleaningInstructions();
	std::vector<ReagentContainerStateDLL>& GetReagentContainerStates();
	std::vector<ReagentInfoRecordDLL>& GetReagentInfoRecords();
	std::map<uint16_t, ReagentDefinitionDLL>& GetReagentDefinitions();
	std::map<size_t/*reagentContainerStatesDLL Index*/, std::vector<ReagentValveMap_t>>& GetReagentValveMaps();

	uint16_t getContainerRemainingEvents(const RfidTag_t& rfid_tag);

private:
	template<typename T>
	static void append (std::vector<T>& des, const std::vector<T>& src)
	{
		des.reserve(des.size() + src.size());
		des.insert(des.end(), src.begin(), src.end());
	}

	void clear();
	void decodeRfidTag (const RfidTag_t& rfidTag);

	ReagentContainerStateDLL readReagentContainerState (const RfidTag_t& rfidTag);
	static std::vector<ReagentDefinitionDLL> readReagentDefinitions (const RfidTag_t& rfidTag);
	static std::vector<ReagentDefinitionDLL> readCleanerDefinitions (const RfidTag_t& rfidTag);
	static std::vector<ReagentStateDLL> readReagentStates (const RfidTag_t& rfidTag);
	static std::vector<ReagentStateDLL> readCleanerStates (const RfidTag_t& rfidTag);
	static std::vector<ReagentCleaningInstruct_t> readCleaningInstructions (const RfidTag_t& rfidTag);
	static std::vector<AnalysisDefinitionDLL> readAnalysisDefinitions (const RfidTag_t& rfidTag);
	static std::vector<ReagentInfoRecordDLL> readReagentInfo (const RfidTag_t& rfidTag);
};
