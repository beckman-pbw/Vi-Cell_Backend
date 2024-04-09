#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "AuditLog.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "AnalysisDefinitionsDLL";

static std::vector<AnalysisDefinitionDLL> analysesDLL_;
static std::shared_ptr<AnalysisDefinitionDLL> pTempAnalysisDefinition_;
static uint32_t num_bci_ad_;
static uint32_t num_user_ad_;
static bool isInitialized = false;

//*****************************************************************************
static bool writeAPToInfoFile (const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType,
	const std::string& baseTag)
{
	boost::property_tree::ptree pt_AnalysisParameter;
	pt_AnalysisParameter.put("label", ap.label);
	pt_AnalysisParameter.put("threshold_value", ap.threshold_value);
	pt_AnalysisParameter.put("above_threshold", ap.above_threshold);

	boost::property_tree::ptree pt_Characteristic;
	pt_Characteristic.put("key", ap.characteristic.key);
	pt_Characteristic.put("s_key", ap.characteristic.s_key);
	pt_Characteristic.put("s_s_key", ap.characteristic.s_s_key);
	pt_AnalysisParameter.add_child("characteristic", pt_Characteristic);

	pt_CellType.add_child(baseTag, pt_AnalysisParameter);

	return true;
}

//*****************************************************************************
// This is called from HawkeyeLogicImpl_CellType::writeCellTypeToInfoFile.
//*****************************************************************************
bool AnalysisDefinitionsDLL::writeAnalysisParameterToInfoFile (const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType)
{
	return writeAPToInfoFile(ap, pt_CellType, "analysis_parameter");
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::writeAnalysisDefinitionToInfoFile (std::string baseTag, AnalysisDefinitionDLL& ad, boost::property_tree::ptree& pt_Analyses) {

	boost::property_tree::ptree pt_AnalysisDefinition;

	pt_AnalysisDefinition.put ("index", ad.analysis_index);
	pt_AnalysisDefinition.put ("label", ad.label);

	for (auto rg : ad.reagent_indices) {
		boost::property_tree::ptree pt_Reagent;
		pt_Reagent.put ("value", rg);
		pt_AnalysisDefinition.add_child ("reagent", pt_Reagent);
	}

	pt_AnalysisDefinition.put("mixing_cycles", ad.mixing_cycles);

	for (auto fl : ad.fl_illuminators) {
		boost::property_tree::ptree pt_FLIlluminator;
		pt_FLIlluminator.put ("illuminator_wavelength_nm", fl.illuminator_wavelength_nm);
		pt_FLIlluminator.put("emission_wavelength_nm", fl.emission_wavelength_nm);
		pt_FLIlluminator.put ("exposure_time_ms", fl.exposure_time_ms);
		pt_AnalysisDefinition.add_child ("fl_illuminator", pt_FLIlluminator);
	}

	for (auto ap : ad.analysis_parameters) {
		writeAnalysisParameterToInfoFile (ap, pt_AnalysisDefinition);
	}

	if (ad.population_parameter)
	{
		writeAPToInfoFile (ad.population_parameter.get(), pt_AnalysisDefinition, "population_parameter");
	}

	pt_Analyses.add_child (baseTag, pt_AnalysisDefinition);

	return true;
}

//*****************************************************************************
static bool writeAPToConfig(const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType,
	const std::string& baseTag)
{
	boost::property_tree::ptree pt_AnalysisParameter;
	pt_AnalysisParameter.put("label", ap.label);
	pt_AnalysisParameter.put("threshold_value", ap.threshold_value);
	pt_AnalysisParameter.put("above_threshold", ap.above_threshold);

	boost::property_tree::ptree pt_Characteristic;
	pt_Characteristic.put("key", ap.characteristic.key);
	pt_Characteristic.put("s_key", ap.characteristic.s_key);
	pt_Characteristic.put("s_s_key", ap.characteristic.s_s_key);
	pt_AnalysisParameter.add_child("characteristic", pt_Characteristic);

	pt_CellType.add_child(baseTag, pt_AnalysisParameter);

	return true;
}

//*****************************************************************************
// This is called from 
//*****************************************************************************
bool AnalysisDefinitionsDLL::writeAnalysisParameterToConfig(const AnalysisParameterDLL& ap, boost::property_tree::ptree& pt_CellType)
{
	return writeAPToInfoFile(ap, pt_CellType, "analysis_parameter");
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::writeAnalysisDefinitionToConfig(std::string baseTag, AnalysisDefinitionDLL& ad, boost::property_tree::ptree& pt_Analyses) {

	boost::property_tree::ptree pt_AnalysisDefinition;

	pt_AnalysisDefinition.put("index", ad.analysis_index);
	pt_AnalysisDefinition.put("label", ad.label);

	for (auto rg : ad.reagent_indices) {
		boost::property_tree::ptree pt_Reagent;
		pt_Reagent.put("value", rg);
		pt_AnalysisDefinition.add_child("reagent", pt_Reagent);
	}

	pt_AnalysisDefinition.put("mixing_cycles", ad.mixing_cycles);

	for (auto fl : ad.fl_illuminators) {
		boost::property_tree::ptree pt_FLIlluminator;
		pt_FLIlluminator.put("illuminator_wavelength_nm", fl.illuminator_wavelength_nm);
		pt_FLIlluminator.put("emission_wavelength_nm", fl.emission_wavelength_nm);
		pt_FLIlluminator.put("exposure_time_ms", fl.exposure_time_ms);
		pt_AnalysisDefinition.add_child("fl_illuminator", pt_FLIlluminator);
	}

	for (auto ap : ad.analysis_parameters) {
		writeAnalysisParameterToConfig(ap, pt_AnalysisDefinition);
	}

	if (ad.population_parameter)
	{
		writeAPToConfig(ad.population_parameter.get(), pt_AnalysisDefinition, "population_parameter");
	}

	pt_Analyses.add_child(baseTag, pt_AnalysisDefinition);

	return true;
}

//*****************************************************************************
std::vector<AnalysisDefinitionDLL>& AnalysisDefinitionsDLL::Get() {
	return analysesDLL_;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::loadCharacteristic (
	boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, Hawkeye::Characteristic_t& c) {

	auto chRange = it->second.equal_range("characteristic");
	if (chRange.first == chRange.second) {
		return false;
	}

	auto it2 = chRange.first;
	c.key = it2->second.get<uint16_t>("key");
	c.s_key = it2->second.get<uint16_t>("s_key");
	c.s_s_key = it2->second.get<uint16_t>("s_s_key");

	return true;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::loadAnalysisParameter (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, AnalysisParameterDLL& ap) {

	ap.label = it->second.get<std::string>("label");
	ap.threshold_value = it->second.get<float>("threshold_value");
	ap.above_threshold = it->second.get<bool>("above_threshold");

	if (!loadCharacteristic (it, ap.characteristic)) {
		return false;
	}

	return true;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::loadReagentIndex (AppConfig& appCfg, uint8_t entryNum, t_opPTree pTree, uint32_t& reagentIndex) {

	std::string str = boost::str (boost::format("reagent_%d") % (int)entryNum);
	if (!appCfg.findField<uint16_t> (pTree, str, true, 0, (uint16_t&)reagentIndex)) {
		return false;
	}

	return true;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::loadFLIlluminator (AppConfig& appCfg, uint8_t entryNum, t_opPTree pTree, FL_IlluminationSettings& fli) {

	std::string str = boost::str (boost::format("fl_illuminator_%d") % (int)entryNum);
	t_opPTree pFLIlluminatorTree = appCfg.findTagInSection (pTree, str);
	if (!pFLIlluminatorTree) {
		return false;
	}

	appCfg.findField<uint16_t> (pFLIlluminatorTree, "illuminator_wavelength_nm", false, 0, (uint16_t&)fli.illuminator_wavelength_nm);
	appCfg.findField<uint16_t> (pFLIlluminatorTree, "emission_wavelength_nm", false, 0, (uint16_t&)fli.emission_wavelength_nm);
	appCfg.findField<uint16_t> (pFLIlluminatorTree, "exposure_time_ms", false, 0, (uint16_t&)fli.exposure_time_ms);

	return true;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::isAnalysisIndexValid (uint16_t index)
{
	if (index == TEMP_ANALYSIS_INDEX)
		return true;

	for (size_t i = 0; i < analysesDLL_.size(); i++) {
		if (index == analysesDLL_[i].analysis_index) {
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::loadAnalysisDefinition (
	boost::property_tree::basic_ptree<std::string, 
	std::string>::assoc_iterator& it, 
	AnalysisDefinitionDLL& ad) {

	ad.analysis_index = it->second.get<uint16_t>("index");
	ad.label = it->second.get<std::string>("label");
	ad.reagent_indices.clear();
	const auto rRange = it->second.equal_range("reagent");
	for (auto i = rRange.first; i != rRange.second; ++i) {
		ad.reagent_indices.push_back ((i->second.get<uint16_t>("value")));
	}

	ad.mixing_cycles = it->second.get<uint8_t>("mixing_cycles", 3);

	ad.fl_illuminators.clear();
	const auto fliRange = it->second.equal_range("fl_illuminator");
	for (auto i = fliRange.first; i != fliRange.second; ++i) {
		FL_IlluminationSettings fli;
		fli.illuminator_wavelength_nm = i->second.get<uint16_t>("illuminator_wavelength_nm");
		fli.emission_wavelength_nm = i->second.get<uint16_t>("emission_wavelength_nm");
		fli.exposure_time_ms = i->second.get<uint16_t>("exposure_time_ms");
		ad.fl_illuminators.push_back (fli);
	}

	ad.analysis_parameters.clear();
	const auto adRange = it->second.equal_range("analysis_parameter");
	for (auto i = adRange.first; i != adRange.second; ++i) {
		AnalysisParameterDLL ap;
		if (!loadAnalysisParameter (i, ap)) {
			return false;
		}
		ad.analysis_parameters.push_back (ap);
	}

	auto ppIt = it->second.find("population_parameter");
	if (ppIt != it->second.not_found())
	{
		AnalysisParameterDLL ap;
		if (!loadAnalysisParameter (ppIt, ap)) {
			return false;
		}
		ad.population_parameter.reset(ap);
	} else {
		ad.population_parameter.reset();
	}

	return true;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::Initialize()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

	if (isInitialized)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Initialize: <exit, already initialized>");
		return true;
	}

	std::vector<DBApi::DB_AnalysisDefinition> dbAnalysesDLL;

	// Check if the Analysis.einfo contents are in the DB.
	DBApi::eQueryResult dbStatus = DBApi::DbGetAnalysisDefinitionsList(
		dbAnalysesDLL,
		DBApi::eListFilterCriteria::NoFilter,
		"",
		"",
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0, 
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbGetAnalysisDefinitionsList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::analysis_definition,
				instrument_error::severity_level::error));
			return false;
		}
	
		analysesDLL_.clear();

		return true;
	}

	auto nextBciAnalysisIndex = HawkeyeConfig::Instance().get().nextAnalysisDefIndex;
//TODO: is there such a thing as a "user" Analysis???
// "user" analyses are not currently supported...
	//nextUserAnalysisIndex_ = USER_ANALYSIS_TYPE_MASK /*dbInstConfig.NextUserAnalysisIndex*/;

	// Analyses were found in DB, convert to AnalysisDefinitionDLL.
	for (auto& v : dbAnalysesDLL)
	{
		AnalysisDefinitionDLL ad;
		ad.FromDbStyle (v);
		analysesDLL_.push_back (ad);

//TODO: save for if/when there is more than one Analysis (user and/or BCI).
		//if (AnalysisDefinitionDLL::isUserDefined(v.AnalysisDefIndex))
		//{
		//	num_user_ad_++;
		//}
		//else
		//{
		//	num_bci_ad_++;
		//}
	}

	num_bci_ad_ = 1;

	isInitialized = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return true;
}

//*****************************************************************************
void AnalysisDefinitionsDLL::Import (boost::property_tree::ptree& ptParent)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	Initialize();

	// DO NOT IMPORT ANYTHING...  CURRENTLY, THERE IS ONLY "ONE" ANALYSIS DEFINTION
	// AND IT IS DEFINED IN THE DATABASE TEMPLATE.
	return;

	// Importing configuration is only allowed on a system that contains no sample data.

	// If the "analyses" tag is found then this data is coming from 
	// the v1.2 Analysis.info file whose parent is the "config" tag.
	auto pAnalyses = ptParent.get_child_optional ("analyses");
	if (pAnalyses) {
		//TODO: There is no next user defined analysis index in InstrumentConfig.
		HawkeyeConfig::Instance().get().nextAnalysisDefIndex = ptParent.get<int16_t>("next_bci_analysis_index");
		//TODO: HawkeyeConfig::Instance().get().nextAnalysisDefIndex = pAnalyses->get<uint16_t>("next_bci_analysis_index");
		ptParent = pAnalyses.get();
	}
	else
	{ // Otherwise, the parent is the "analyses" tag used in v1.3 configuration
	  // export and data export for offline analysis.

		//TODO: There is no next user defined analysis index in InstrumentConfig.
		HawkeyeConfig::Instance().get().nextAnalysisDefIndex = ptParent.get<int16_t>("next_bci_analysis_index");
		//TODO: HawkeyeConfig::Instance().get().nextAnalysisDefIndex = ptParent.get<uint16_t>("next_bci_analysis_index");
	}

	const auto adRange = ptParent.equal_range("analysis");

	for (auto it = adRange.first; it != adRange.second; ++it)
	{
		AnalysisDefinitionDLL adDLL;
		if (!loadAnalysisDefinition(it, adDLL)) {
			Logger::L().Log(MODULENAME, severity_level::notification, "Import: <exit, <analysis> tag not found>");
			return;
		}

		// Only import user defined analyses.  Currently, there are none.
		if (AnalysisDefinitionDLL::isUserDefined(adDLL.analysis_index))
		{
//TODO: this code is incorrect...  should probably use analysis index.
// Since there is no support for user defined analyses...  this is put off until actually needed...
			auto item = std::find_if(analysesDLL_.begin(), analysesDLL_.end(),
				[adDLL](const auto& item) { return item.label == adDLL.label; });

			// Only add analysis if it doesn't already exist.
			if (item == analysesDLL_.end())
			{
				uint16_t newIndex;
				bool isUserAnalysis = true;
				Add (adDLL, newIndex, isUserAnalysis);
			}
			else
			{
				Logger::L().Log(MODULENAME, severity_level::normal,
					boost::str(boost::format("ImportConfig: Duplicate analysis index found %ld>") % (int32_t)adDLL.analysis_index));
			}
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}

//*****************************************************************************
std::pair<std::string, boost::property_tree::ptree> AnalysisDefinitionsDLL::Export()
{
	boost::property_tree::ptree pt_Analyses = {};

	pt_Analyses.put ("next_bci_analysis_index", HawkeyeConfig::Instance().get().nextAnalysisDefIndex);
	// Using the default value since there is no field for user analyses.
	pt_Analyses.put ("next_user_analysis_index", USER_ANALYSIS_TYPE_MASK);

	for (auto a : analysesDLL_) {
		writeAnalysisDefinitionToConfig("analysis", a, pt_Analyses);
	}

	return std::make_pair ("analyses", pt_Analyses);
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::GetAnalysisByIndex (uint16_t index, AnalysisDefinitionDLL& analysis)
{
	analysis = {};

	if ((index == TEMP_ANALYSIS_INDEX) && pTempAnalysisDefinition_)
	{
		analysis = *pTempAnalysisDefinition_;
		return true;
	}

	for (size_t i = 0; i < analysesDLL_.size(); i++) {
		if (index == analysesDLL_[i].analysis_index) {
			analysis = analysesDLL_[i];
			return true;
		}
	}

	analysis.analysis_index = USHRT_MAX;
	return false;
}

//*****************************************************************************
bool AnalysisDefinitionsDLL::GetAnalysisByIndex (uint16_t index, const CellTypeDLL& cellType, AnalysisDefinitionDLL& analysis)
{
	analysis = {};

	if ((index == TEMP_ANALYSIS_INDEX) && pTempAnalysisDefinition_)
	{
		analysis = *pTempAnalysisDefinition_;
		return true;
	}

	for (size_t i = 0; i < analysesDLL_.size(); i++)
	{
		if (index == analysesDLL_[i].analysis_index)
		{
			analysis = analysesDLL_[i];

			// Check through the cell type for an overriding specialization
			for (auto a : cellType.analysis_specializations)
			{
				if (a.analysis_index == index)
				{
					analysis = a;
					break;
				}
			}

			return true;
		}
	}

	return false;
}

//*****************************************************************************
//TODO: this code has not been tested since currently there is ONLY one AnalysisDefintion.
//*****************************************************************************
//TODO: why is "isUserAnalysis" pass in here... CellTypesDLL:Add does pass it in...
bool AnalysisDefinitionsDLL::Add (AnalysisDefinitionDLL& adDLL, uint16_t& newIndex, bool isUserAnalysis)
{
	//NOTE: since there is currently only ONE analysis and there is no way to add another (user or BCI),
	// this code is not complete and has not been successfully tested.
	// The DB InstrumentConfig record does not have a field for a user defined analysis index.
	if (isUserAnalysis) {
		adDLL.analysis_index = HawkeyeConfig::Instance().get().nextAnalysisDefIndex;
	} else {
		adDLL.analysis_index = HawkeyeConfig::Instance().get().nextAnalysisDefIndex;
	}

	DBApi::DB_AnalysisDefinitionRecord dbAnalysisDefinitionRecord = adDLL.ToDbStyle();

	DBApi::eQueryResult dbStatus = DBApi::DbAddAnalysisDefinition(dbAnalysisDefinitionRecord);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::InsertObjectExists != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("Import: <exit, DbAddAnalysisDefinition failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::analysis_definition,
				instrument_error::severity_level::error));
			return false;
		}
	}

	analysesDLL_.push_back (adDLL);
	
	newIndex = adDLL.analysis_index;

	HawkeyeConfig::Instance().get().nextAnalysisDefIndex = newIndex + 1;

//TODO: if the "Add" method is ever implemented in HawkeyeLogicImpl move this to the caller of "nalysisDefinitionsDLL::Add".
	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetLoggedInUsername(),
		audit_event_type::evt_analysiscreate, 
		AnalysisDefinitionDLL::ToStr(adDLL)));

	return true;
}

//*****************************************************************************
// Setting *force* to true allows deleting a BCI analysis.
//TODO: this code has not been tested since currently there is ONLY one AnalysisDefintion.
//*****************************************************************************
HawkeyeError AnalysisDefinitionsDLL::Delete (uint16_t index) {

	// Delete the object from the analysis collection.
	auto ad_it = std::find_if (analysesDLL_.begin(), analysesDLL_.end(), [index](const AnalysisDefinitionDLL& ad)->bool {
		return ad.analysis_index == index;
	});

	if (ad_it == analysesDLL_.end())
	{
		return HawkeyeError::eEntryNotFound;
	}

//TODO: if the "Delete" method is ever implemented in HawkeyeLogicImpl move this to the caller of "nalysisDefinitionsDLL::Add".
	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetLoggedInUsername(),
		audit_event_type::evt_analysisdelete, 
		AnalysisDefinitionDLL::ToStr(*ad_it)));

	DBApi::eQueryResult dbStatus = DBApi::DbRemoveAnalysisDefinitionByUuid (ad_it->uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("Initialize: <exit, DbRemoveAnalysisDefinitionByUuid failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_deleteerror,
			instrument_error::instrument_storage_instance::analysis_definition,
			instrument_error::severity_level::error));
		return HawkeyeError::eStorageFault;
	}

	analysesDLL_.erase (ad_it);

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void AnalysisDefinitionsDLL::SetTemporaryAnalysis (AnalysisDefinition* tempAnalysis) {

	 HAWKEYE_ASSERT (MODULENAME, tempAnalysis);

	if (tempAnalysis) {
		pTempAnalysisDefinition_ = std::make_shared<AnalysisDefinitionDLL>(*tempAnalysis);

		// Assign Max value for Any Temp analysis Index.
		pTempAnalysisDefinition_->analysis_index = TEMP_ANALYSIS_INDEX;
	}

}

//*****************************************************************************
bool AnalysisDefinitionsDLL::SetTemporaryAnalysis (uint16_t analysis_index) {

	pTempAnalysisDefinition_ = std::make_shared<AnalysisDefinitionDLL>();

	AnalysisDefinitionDLL temporaryAnalysis;
	if (!AnalysisDefinitionsDLL::GetAnalysisByIndex (analysis_index, temporaryAnalysis))
	{
		pTempAnalysisDefinition_.reset();
		return false;
	}

	// Assign Max value for Any Temp analysis Index.
	pTempAnalysisDefinition_->analysis_index = TEMP_ANALYSIS_INDEX;

	return true;
}

//*****************************************************************************
std::shared_ptr<AnalysisDefinitionDLL> AnalysisDefinitionsDLL::GetTemporaryAnalysis() {

	return pTempAnalysisDefinition_;
}

//*****************************************************************************
void AnalysisDefinitionsDLL::freeAnalysisDefinitionsInternal (AnalysisDefinition* list, uint32_t num_analyses)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeAnalysisDefinitions: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < num_analyses; i++)
	{
		AnalysisDefinitionDLL::freeAnalysisDefinitionInternal(list[i]);
	}

	delete[] list;

	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeAnalysisDefinitions: <exit>");
}

//*****************************************************************************
AnalysisDefinitionDLL AnalysisDefinitionsDLL::getAnalysisDefinitionByUUID (uuid__t uuid)
{
	for (auto ad : analysesDLL_)
	{
		if (Uuid(uuid) == Uuid(ad.uuid))
		{
			return ad;
		}
	}

	return {};
}
