#pragma once

#include <cstdint>
#include <string>
#include <vector>

#pragma warning(push, 0)
#include <boost/property_tree/ptree.hpp>
#pragma warning(pop)

#include "AnalysisDefinitionDLL.hpp"
#include "AppConfig.hpp"
#include "CellTypeDLL.hpp"
#include "HawkeyeError.hpp"

class AnalysisDefinitionsDLL
{
public:
	static bool Initialize();
	static void Import (boost::property_tree::ptree& ptConfig);
	static std::pair<std::string, boost::property_tree::ptree> Export();

	static bool GetAnalysisByIndex (uint16_t index, AnalysisDefinitionDLL& analysis);
	static bool GetAnalysisByIndex (uint16_t index, const CellTypeDLL& cellType, AnalysisDefinitionDLL& analysis);

	static bool Add (AnalysisDefinitionDLL& ad, uint16_t& newIndex, bool isUserAnalysis = true);
	static HawkeyeError Delete (uint16_t index);

	static std::vector<AnalysisDefinitionDLL>& Get();
	static void SetTemporaryAnalysis (AnalysisDefinition* tempAnalysis);
	static bool SetTemporaryAnalysis (uint16_t analysis_index);
	static std::shared_ptr<AnalysisDefinitionDLL> GetTemporaryAnalysis();
	static bool isAnalysisIndexValid (uint16_t index);
	static bool loadAnalysisDefinition (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, AnalysisDefinitionDLL& ap);
	static bool loadAnalysisParameter (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, AnalysisParameterDLL& ap);
	static bool loadReagentIndex (AppConfig& appCfg, uint8_t entryNum, t_opPTree pTree, uint32_t& reagentIndex);
	static bool loadFLIlluminator (AppConfig& appCfg, uint8_t entryNum, t_opPTree pTree, FL_IlluminationSettings& fli);
	static bool loadCharacteristic (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, Hawkeye::Characteristic_t& c);
	static bool writeAnalysisParameterToInfoFile (const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType);
	static bool writeAnalysisDefinitionToInfoFile (std::string baseTag, AnalysisDefinitionDLL& ad, boost::property_tree::ptree& pt_Analyses);
	static bool writeAnalysisParameterToConfig(const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType);
	static bool writeAnalysisDefinitionToConfig(std::string baseTag, AnalysisDefinitionDLL& ad, boost::property_tree::ptree& pt_Analyses);
	static void freeAnalysisDefinitionsInternal (AnalysisDefinition* list, uint32_t num_analyses);
	static AnalysisDefinitionDLL getAnalysisDefinitionByUUID (uuid__t uuid);
};
