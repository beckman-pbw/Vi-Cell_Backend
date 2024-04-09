#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "CellTypesDLL.hpp"
#include "HawkeyeConfig.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "CellTypes";

static std::vector<CellTypeDLL> cellTypesDLL_ = {};
static uint32_t num_bci_ct_;
static bool isInitialized = false;

//*****************************************************************************
bool CellTypesDLL::loadCellType (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it, CellTypeDLL& ct)
{
	ct.celltype_index = assoc_it->second.get<uint32_t>("index");
	ct.label = assoc_it->second.get<std::string>("label");
	ct.max_image_count = assoc_it->second.get<uint16_t>("max_image_count");
	ct.aspiration_cycles = assoc_it->second.get<uint8_t>("aspiration_cycles", 3);
	ct.minimum_diameter_um = assoc_it->second.get<float>("minimum_diameter_um");
	ct.maximum_diameter_um = assoc_it->second.get<float>("maximum_diameter_um");
	ct.minimum_circularity = assoc_it->second.get<float>("minimum_circularity");
	ct.sharpness_limit = assoc_it->second.get<float>("sharpness_limit");
	boost::optional<float> tFloat = assoc_it->second.get_optional<float>("concentration_adjustment_factor");
	if (tFloat)
	{
		ct.concentration_adjustment_factor = tFloat.get();
	}
	boost::optional<bool> tBool = assoc_it->second.get_optional<bool>("retired");
	if (tBool)
	{
		ct.retired = tBool.get();
	}
	else
	{
		ct.retired = false;
	}

	uint16_t temp = assoc_it->second.get<uint16_t>("decluster_setting");
	ct.decluster_setting = (eCellDeclusterSetting)temp;
	ct.fl_roi_extent = assoc_it->second.get<float>("fl_roi_extent");

	auto apRange = assoc_it->second.equal_range("analysis_parameter");
	ct.cell_identification_parameters.clear();
	for (auto it = apRange.first; it != apRange.second; ++it)
	{
		AnalysisParameterDLL ap;
		AnalysisDefinitionsDLL::loadAnalysisParameter(it, ap);
		ct.cell_identification_parameters.push_back(ap);
	}

	auto adRange = assoc_it->second.equal_range("analysis_specialization");
	for (auto it = adRange.first; it != adRange.second; ++it)
	{
		AnalysisDefinitionDLL ad;
		AnalysisDefinitionsDLL::loadAnalysisDefinition (it, ad);

		// Make sure that the index for the specialization is in the analysis definition collection.
		if (!AnalysisDefinitionsDLL::isAnalysisIndexValid(ad.analysis_index)) {
			Logger::L().Log (MODULENAME, severity_level::error, "No matching Analysis Definition found for Analysis Specialization");
			return false;
		}

		ct.analysis_specializations.push_back(ad);
	}

	// Populate the ROI x,y values with the values from the CellCounting configuration.
	ct.roi_x_pixels = static_cast<uint16_t>(HawkeyeConfig::Instance().get().cellCounting.roiXCoordinate);
	ct.roi_y_pixels = static_cast<uint16_t>(HawkeyeConfig::Instance().get().cellCounting.roiYCoordinate);

	return true;
}

//*****************************************************************************
static bool writeCellTypeToConfig(CellTypeDLL& ct, boost::property_tree::ptree& pt_CellTypes) {

	boost::property_tree::ptree pt_CellType = {};

	pt_CellType.put("index", ct.celltype_index);
	pt_CellType.put("label", ct.label);
	pt_CellType.put("max_image_count", ct.max_image_count);
	pt_CellType.put("aspiration_cycles", ct.aspiration_cycles);
	pt_CellType.put("minimum_diameter_um", ct.minimum_diameter_um);
	pt_CellType.put("maximum_diameter_um", ct.maximum_diameter_um);
	pt_CellType.put("minimum_circularity", ct.minimum_circularity);
	pt_CellType.put("sharpness_limit", ct.sharpness_limit);
	pt_CellType.put("decluster_setting", ct.decluster_setting);
	pt_CellType.put("fl_roi_extent", ct.fl_roi_extent);
	pt_CellType.put("concentration_adjustment_factor", ct.concentration_adjustment_factor);
	pt_CellType.put("retired", ct.retired);

	for (auto ap : ct.cell_identification_parameters) {
		AnalysisDefinitionsDLL::writeAnalysisParameterToInfoFile(ap, pt_CellType);
	}

	for (auto ad : ct.analysis_specializations) {
		AnalysisDefinitionsDLL::writeAnalysisDefinitionToInfoFile("analysis_specialization", ad, pt_CellType);
	}

	pt_CellTypes.add_child("cell_type", pt_CellType);

	return true;
}

//*****************************************************************************
std::pair<std::string, boost::property_tree::ptree> CellTypesDLL::Export()
{
	boost::property_tree::ptree pt_CellTypes = {};

	for (auto ct : cellTypesDLL_) {
		writeCellTypeToConfig(ct, pt_CellTypes);
	}

	return std::make_pair ("cell_types", pt_CellTypes);
}

//*****************************************************************************
uint32_t CellTypesDLL::GetNextUserCellTypeIndex()
{
	uint32_t nextUserCellTypeIndex = USER_CELLTYPE_MASK - 1;
	
	for (auto& v : cellTypesDLL_)
	{
		if (v.celltype_index > nextUserCellTypeIndex)
		{
			nextUserCellTypeIndex = v.celltype_index;
		}
	}

	return nextUserCellTypeIndex + 1;
}

//*****************************************************************************
bool CellTypesDLL::Initialize() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

	if (isInitialized)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Initialize: <exit, already initialized>");
		return true;
	}

	std::vector<DBApi::DB_CellTypeRecord> dbCellTypesDLL;

	DBApi::eQueryResult dbStatus = DBApi::DbGetCellTypeList(
		dbCellTypesDLL,
		DBApi::eListFilterCriteria::NoFilter,
		"",
		"",
		-1,
		DBApi::eListSortCriteria::IdNumSort,
		DBApi::eListSortCriteria::SortNotDefined,
		1,	// Sort ascending.
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format ("Initialize: <exit, DDbGetCellTypeList failed; query status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::celltype_definition,
				instrument_error::severity_level::error));
			return false;
		}

		cellTypesDLL_.clear();
		
		return true;
	}

	// CellTypes were found in DB, convert to CellTypeDLL.
	for (auto& v : dbCellTypesDLL)
	{		
		CellTypeDLL ct = {};
		ct.FromDbStyle (v);

		cellTypesDLL_.push_back (ct);
	}

	isInitialized = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return true;
}

//*****************************************************************************
void CellTypesDLL::Import (boost::property_tree::ptree& ptParent) {

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	// If the "cell_types" tag is found then this data is coming from 
	// the v1.2 CellType.info file whose parent is the "config" tag.
	auto pCellTypes = ptParent.get_child_optional ("cell_types");
	if (pCellTypes) {
		ptParent = pCellTypes.get();
	}

	Initialize();

	const auto ctRange = ptParent.equal_range("cell_type");
	for (auto it = ctRange.first; it != ctRange.second; ++it) {
		CellTypeDLL ctDLL;
		if (!loadCellType(it, ctDLL)) {
			Logger::L().Log(MODULENAME, severity_level::notification, "Import: <exit, <celltype> tag not found>");
			return;
		}

		// Only import user defined analyses.  Currently, there are none.
		if (CellTypeDLL::isUserDefined(ctDLL.celltype_index))
		{
			auto item = std::find_if(cellTypesDLL_.begin(), cellTypesDLL_.end(),
				[ctDLL](const auto& item) {
					return item.celltype_index == ctDLL.celltype_index;
				});

			// Only add cell type if it doesn't already exist.
			if (item == cellTypesDLL_.end())
				Add (ctDLL, true);
			else
			{
				Logger::L().Log(MODULENAME, severity_level::normal,
					boost::str(boost::format("Import: Duplicate cell type index found %ld>") % (int32_t)ctDLL.celltype_index));
			}
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}

//*****************************************************************************
std::vector<CellTypeDLL>& CellTypesDLL::Get()
{
	return cellTypesDLL_;
}

//*****************************************************************************
CellTypeDLL& CellTypesDLL::Add (CellTypeDLL& ctDLL, bool isImported)
{
	if (!isImported)
	{
		ctDLL.celltype_index = GetNextUserCellTypeIndex();
	}
	
	// Look for the cell type index in the returned list.
	auto item = std::find_if (cellTypesDLL_.begin(), cellTypesDLL_.end(),
		[ctDLL](const auto& item) { return item.celltype_index == ctDLL.celltype_index; });

	//NOTE: user CellTypes in v1.2 did not add the <regent> tag to the CellType definition in the "info" file.
	// Populate the "reagent" field in the "analysis_specialization" now.
	if (ctDLL.analysis_specializations[0].reagent_indices.size() == 0)
	{
		//NOTE: This only works since currently there is only one AnalysisDefinition
		// and the code to add more is not called from anywhere.
		const std::vector<AnalysisDefinitionDLL>& adsDLL = AnalysisDefinitionsDLL::Get();
		uint32_t reagentIndex = adsDLL[0].reagent_indices[0];
		ctDLL.analysis_specializations[0].reagent_indices.push_back (reagentIndex);
	}

	// Skip this entry If the cell type already exists in the DLL.
	if (item == cellTypesDLL_.end())
	{
		DBApi::DB_CellTypeRecord dbCellTypeRecord = {};
		dbCellTypeRecord = ctDLL.ToDbStyle();

		DBApi::eQueryResult dbStatus = DBApi::DbAddCellType(dbCellTypeRecord);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("Add: <exit, DbAddCellType failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::error));
		}
		else
		{
			ctDLL.FromDbStyle (dbCellTypeRecord);
			cellTypesDLL_.push_back (ctDLL);

			Logger::L().Log(MODULENAME, severity_level::normal, "Add: successfully added cell type to database");
		}
	}

	return cellTypesDLL_[cellTypesDLL_.size()-1];
}

//*****************************************************************************
uint32_t CellTypesDLL::NumBCICellTypes() {
	return num_bci_ct_;
}

//*****************************************************************************
bool CellTypesDLL::isCellTypeIndexValid (uint32_t index)
{
	if (index == TEMP_CELLTYPE_INDEX) {
		return true;
	}

	for (auto& ct : cellTypesDLL_) {
		if (index == ct.celltype_index) {
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool CellTypesDLL::getCellTypeByIndex (uint32_t index, CellTypeDLL& cellType)
{
	cellType = {};

	for (auto& ct : cellTypesDLL_) {
		if (index == ct.celltype_index) {
			cellType = ct;
			return true;
		}
	}

	return false;
}

//*****************************************************************************
std::string CellTypesDLL::getCellTypeName (uint32_t index)
{
	for (auto ct : cellTypesDLL_)
	{
		if (index == ct.celltype_index)
		{
			return ct.label;
		}
	}

	return "";
}

//*****************************************************************************
CellTypeDLL CellTypesDLL::getCellTypeByIndex (uint32_t index)
{
	for (auto ct : cellTypesDLL_)
	{
		if (index == ct.celltype_index)
		{
			return ct;
		}
	}

	return {};
}

//*****************************************************************************
CellTypeDLL CellTypesDLL::getCellTypeByUUID (const uuid__t& uuid)
{
	for (auto ct : cellTypesDLL_)
	{
		if (Uuid(uuid) == Uuid(ct.uuid))
		{
			return ct;
		}
	}

	return {};
}

//*****************************************************************************
CellTypeDLL CellTypesDLL::getCellTypeByName (std::string name)
{
	for (auto ct : cellTypesDLL_)
	{
		if (name == ct.label)
		{
			return ct;
		}
	}

	return {};
}
