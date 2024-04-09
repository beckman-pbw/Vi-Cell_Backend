#pragma once

#include "HawkeyeError.hpp"
#include "HawkeyeThread.hpp"
#include "ReagentRFIDLayout.hpp"
#include "ResultDefinitionDLL.hpp"

void reagentContainerStateFromDLL (ReagentContainerState& to, const ReagentContainerStateDLL& from);
void reagentDefinitionFromDLL (ReagentDefinition& to, const ReagentDefinitionDLL& from);
void reagentStateFromDLL (ReagentState& to, const ReagentStateDLL& from);

class ReagentPack
{
public:
	typedef std::function<void (ReagentUnloadSequence)> reagent_unload_status_callback_DLL;
	typedef std::function<void (ReagentUnloadSequence)> reagent_unload_complete_callback_DLL;
	typedef std::function<void(ReagentLoadSequence)> reagent_load_status_callback_DLL;
	typedef std::function<void(ReagentLoadSequence)> reagent_load_complete_callback_DLL;

	static ReagentPack& Instance()
	{
		static ReagentPack instance;
		return instance;
	}

	ReagentPack();
	virtual ~ReagentPack();

	void Initialize (std::shared_ptr<boost::asio::io_context> upstream, std::function<void (bool)> callback);
	void Load (reagent_load_status_callback_DLL onLoadStatusChange,
	           reagent_load_complete_callback_DLL onLoadComplete,
	           std::function<void (HawkeyeError)> callback);
	void Unload (ReagentContainerUnloadOption* UnloadActions,
	             uint16_t nContainers,
	             reagent_unload_status_callback_DLL onUnloadStatusChange,
	             reagent_unload_complete_callback_DLL onUnloadComplete,
	             std::function<void (HawkeyeError)> callback);
	void ScanAndReadRfidTags (std::function<void (bool)> callback);
	std::vector<AnalysisDefinitionDLL>& GetAnalysisDefinitions();
	std::vector<ReagentCleaningInstruct_t>& GetCleaningInstructions();
	std::vector<ReagentContainerStateDLL>& GetReagentContainerStates();
	std::vector<ReagentInfoRecordDLL>& GetReagentInfoRecords();
	std::map<uint16_t, ReagentDefinitionDLL>& GetReagentDefinitions();
	bool isEmpty ();
	bool isPackInitialized() const;
	bool isPackUsable() const;
	bool isNearExpiration (bool reportError, uint32_t& daysRemaining, uint64_t minutesRequired) const;
	void Stop();
	static void Import (boost::property_tree::ptree& ptConfig);

private:
	void initializeInternal (std::function<void (bool)> callback);
	void scanRfidTags (std::function<void (bool)> callback) const;
	void readRfidTags (std::function<void (bool)> callback);
	void continuouslyReadRfidTags (boost::system::error_code ec);
	void onLoadComplete (std::string loggedInUsername, HawkeyeError he, reagent_load_complete_callback_DLL onLoadComplete);
	void onUnloadComplete (std::string loggedInUsername, HawkeyeError he, reagent_unload_complete_callback_DLL onUnloadComplete);
	void setValveMap (std::function<void (bool)> callback) const;
	void setValveMapInternal (size_t rcsIndex, std::function<void (bool)> callback) const;
	void setValveMapForContainer (size_t rcsIndex, size_t valveMapIndex, std::function<void (bool)> callback) const;
	bool verify_current_pack();
	static void saveCurrentPack (const std::string tagSn);
};
