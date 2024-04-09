#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "CellTypeDLL.hpp"
#include "DataConversion.hpp"
#include "HawkeyeUUID.hpp"
#include "Logger.hpp"
#include "SetIndentation.hpp"

static const char MODULENAME[] = "CellType";

//*****************************************************************************
CellTypeDLL::CellTypeDLL (CellType ct)
{
	celltype_index = ct.celltype_index;
	DataConversion::convertToStandardString (label, ct.label);
	max_image_count = ct.max_image_count;

	aspiration_cycles = ct.aspiration_cycles;
	minimum_diameter_um = ct.minimum_diameter_um;
	maximum_diameter_um = ct.maximum_diameter_um;
	minimum_circularity = ct.minimum_circularity;
	sharpness_limit = ct.sharpness_limit;
	concentration_adjustment_factor = ct.concentration_adjustment_factor;

	DataConversion::convert_listOfCType_to_vecOfDllType (ct.cell_identification_parameters, ct.num_cell_identification_parameters, cell_identification_parameters);

	decluster_setting = ct.decluster_setting;
	fl_roi_extent = ct.fl_roi_extent;

	if (ct.roi_x_pixels == 0)
	{
		roi_x_pixels = ROI_X_PIXELS;
	}
	else
	{
		roi_x_pixels = ct.roi_x_pixels;
	}

	if (ct.roi_y_pixels == 0)
	{
		roi_y_pixels = ROI_Y_PIXELS;
	}
	else
	{
		roi_y_pixels = ct.roi_y_pixels;
	}

	DataConversion::convert_listOfCType_to_vecOfDllType (ct.analysis_specializations, ct.num_analysis_specializations, analysis_specializations);
}

//*****************************************************************************
CellType CellTypeDLL::ToCStyle()
{
	CellType ct = {};

	ct.celltype_index = celltype_index;

	DataConversion::convertToCharPointer(ct.label, label);

	ct.max_image_count = max_image_count;
	ct.aspiration_cycles = aspiration_cycles;
	ct.minimum_diameter_um = minimum_diameter_um;
	ct.maximum_diameter_um = maximum_diameter_um;
	ct.minimum_circularity = minimum_circularity;
	ct.sharpness_limit = sharpness_limit;
	ct.concentration_adjustment_factor = concentration_adjustment_factor;

	DataConversion::convert_vecOfDllType_to_listOfCType(
		cell_identification_parameters, ct.cell_identification_parameters, ct.num_cell_identification_parameters);

	ct.decluster_setting = decluster_setting;
	ct.fl_roi_extent = fl_roi_extent;

	DataConversion::convert_vecOfDllType_to_listOfCType (analysis_specializations, ct.analysis_specializations, ct.num_analysis_specializations);

	return ct;
}

//*****************************************************************************
DBApi::DB_CellTypeRecord CellTypeDLL::ToDbStyle() const
{
	DBApi::DB_CellTypeRecord ctr = {};

	HawkeyeUUID tmpId = HawkeyeUUID();

	ctr.CellTypeIdNum = 0;
	tmpId.get_uuid__t(ctr.CellTypeId);
	ctr.Protected = false;
	ctr.CellTypeIndex = celltype_index;
	ctr.CellTypeNameStr = label;
	ctr.MaxImageCount = max_image_count;
	ctr.AspirationCycles = aspiration_cycles;
	ctr.MinDiam = minimum_diameter_um;
	ctr.MaxDiam = maximum_diameter_um;
	ctr.MinCircularity = minimum_circularity;
	ctr.SharpnessLimit = sharpness_limit;
	ctr.CalculationAdjustmentFactor = concentration_adjustment_factor;
	ctr.Retired = retired;
	ctr.NumCellIdentParams = (int8_t)cell_identification_parameters.size();
	ctr.RoiXPixels = roi_x_pixels;
	ctr.RoiYPixels = roi_y_pixels;

	ctr.CellIdentParamList.clear();
	if (ctr.NumCellIdentParams > 0)
	{
		AnalysisParameterDLL apdll;
		DBApi::DB_AnalysisParamRecord dbap;

		for (auto apIter = cell_identification_parameters.begin(); apIter != cell_identification_parameters.end(); ++apIter)
		{
			apdll = *apIter;
			dbap = apdll.ToDbStyle();
			ctr.CellIdentParamList.push_back(dbap);
		}
	}

	ctr.DeclusterSetting = decluster_setting;
	ctr.RoiExtent = fl_roi_extent;
	ctr.RoiXPixels = roi_x_pixels;
	ctr.RoiYPixels = roi_y_pixels;
	ctr.NumAnalysisSpecializations = (int32_t)analysis_specializations.size();

	ctr.SpecializationsDefList.clear();
	if (ctr.NumAnalysisSpecializations > 0)
	{
		AnalysisDefinitionDLL adefdll;
		DBApi::DB_AnalysisDefinitionRecord dbadef;

		for (auto adIter = analysis_specializations.begin(); adIter != analysis_specializations.end(); ++adIter)
		{
			adefdll = *adIter;
			dbadef = adefdll.ToDbStyle();
			ctr.SpecializationsDefList.push_back(dbadef);
		}
	}

	return ctr;
}

//*****************************************************************************
std::string CellTypeDLL::CellTypeAsString (const CellTypeDLL& ct, uint8_t tabCount)
{
	std::string ct_str_1 = boost::str(boost::format(
		"celltype_index: %d"
		"%slabel: %s"
		"%smax_image_count: %d"
		"%saspiration_cycles: %d"
		// "%sroi_x_pixels: %d"			// Keep for possible later inclusion
		// "%sroi_y_pixels: %d"			// Keep for possible later inclusion
		"%sminimum_diameter_um: %.2f"
		"%smaximum_diameter_um: %.2f"
		"%sminimum_circularity: %.2f"
		"%ssharpness_limit: %.1f"
		"%sconcentration_adjustment_factor: %.1f"
		"%sretired: %s")
		% ct.celltype_index
		% SetIndentation(tabCount + 1)
		% ct.label
		% SetIndentation(tabCount + 1)
		% ct.max_image_count
		% SetIndentation(tabCount + 1)
		% (int)ct.aspiration_cycles
		% SetIndentation(tabCount + 1)
		// Save for possible later inclusion
		//% ct.roi_x_pixels
		//% SetIndentation(tabCount + 1)
		//% ct.roi_y_pixels
		//% SetIndentation(tabCount + 1)
		% ct.minimum_diameter_um
		% SetIndentation(tabCount + 1)
		% ct.maximum_diameter_um
		% SetIndentation(tabCount + 1)
		% ct.minimum_circularity
		% SetIndentation(tabCount + 1)
		% ct.sharpness_limit
		% SetIndentation(tabCount + 1)
		% ct.concentration_adjustment_factor
		% SetIndentation(tabCount + 1)
		% (ct.retired ? "true" : "false"));

	std::string cell_identification_parameters_str = {};
	for (auto& item : ct.cell_identification_parameters)
	{
		cell_identification_parameters_str += SetIndentation(tabCount + 2);
		cell_identification_parameters_str += AnalysisParameterDLL::ToStr (item, tabCount + 2);
	}

	std::string analysis_specializations_str = {};
	for (auto& item : ct.analysis_specializations)
	{
		analysis_specializations_str += SetIndentation(tabCount + 1);
		analysis_specializations_str += AnalysisDefinitionDLL::ToStr (item, tabCount + 2);
	}

	tabCount++;

	std::string ct_str_2 = boost::str (boost::format(
		"cell_identification_parameters: %s"
		"%sdecluster_setting: %d"
		"%sanalysis_specializations: %s")
		% cell_identification_parameters_str
		% SetIndentation(tabCount + 1)
		% ct.decluster_setting
		% SetIndentation(tabCount + 1)
		% analysis_specializations_str);

	return ct_str_1 + "\n" + SetIndentation(tabCount, false) + ct_str_2;
}

//*****************************************************************************
bool CellTypeDLL::isUserDefined (uint32_t ct_index)
{
	return (ct_index & USER_CELLTYPE_MASK) ? true : false;
}

//*****************************************************************************
std::string CellTypeDLL::UpdateCellType (const CellTypeDLL& inputCellType, CellTypeDLL& currentCellType) {

	std::stringstream ss;

	Logger::L().Log (MODULENAME, severity_level::debug3, "Updating CellType...");

	if (inputCellType.label != currentCellType.label) {
		ss << boost::str (boost::format ("Label: %s -> %s\n") % currentCellType.label % inputCellType.label);
	}
	if (inputCellType.max_image_count != currentCellType.max_image_count) {
		ss << boost::str (boost::format ("MaxImageCount: %d -> %d\n") % currentCellType.max_image_count % inputCellType.max_image_count);
	}
	if (inputCellType.aspiration_cycles != currentCellType.aspiration_cycles) {
		ss << boost::str (boost::format ("AspirationCycles: %s -> %s\n") % std::to_string(currentCellType.aspiration_cycles) % std::to_string(inputCellType.aspiration_cycles));
	}
	if (inputCellType.minimum_diameter_um != currentCellType.minimum_diameter_um) {
		ss << boost::str (boost::format ("MinimumDiameter: %3.2f -> %3.2f\n") % currentCellType.minimum_diameter_um % inputCellType.minimum_diameter_um);
	}
	if (inputCellType.maximum_diameter_um != currentCellType.maximum_diameter_um) {
		ss << boost::str (boost::format ("MaximumDiameter: %3.2f -> %3.2f\n") % currentCellType.maximum_diameter_um % inputCellType.maximum_diameter_um);
	}
	if (inputCellType.minimum_circularity != currentCellType.minimum_circularity) {
		ss << boost::str (boost::format ("MinimumCircularity: %3.2f -> %3.2f\n") % currentCellType.minimum_circularity % inputCellType.minimum_circularity);
	}
	if (inputCellType.sharpness_limit != currentCellType.sharpness_limit) {
		ss << boost::str (boost::format ("SharpnessLimit: %3.2f -> %3.2f\n") % currentCellType.sharpness_limit % inputCellType.sharpness_limit);
	}
	if (inputCellType.cell_identification_parameters.size() == currentCellType.cell_identification_parameters.size()) {

		// Compare and update each entry.
		for (size_t i = 0; i < currentCellType.cell_identification_parameters.size(); i++) {
			AnalysisDefinitionDLL::UpdateAnalysisParameter (ss, inputCellType.cell_identification_parameters[i], currentCellType.cell_identification_parameters[i]);
		}
	}
	else
	{
		// Delete the current entries.
		ss << boost::str (boost::format ("CellIdentifcationParameters: cleared\n"));

		// Add the new entries.
		for (size_t i = 0; i < inputCellType.cell_identification_parameters.size(); i++) {
			ss << boost::str (boost::format ("CellIdentifcationParameters: add %s\n") % inputCellType.cell_identification_parameters[i].label);
		}
	}
	if (inputCellType.decluster_setting != currentCellType.decluster_setting) {
		ss << boost::str (boost::format ("DeclusterSetting: %3.2f -> %3.2f\n") % currentCellType.decluster_setting % inputCellType.decluster_setting);
	}
	if (inputCellType.fl_roi_extent != currentCellType.fl_roi_extent) {
		ss << boost::str (boost::format ("FL_RoiExtent: %3.2f -> %3.2f\n") % currentCellType.fl_roi_extent % inputCellType.fl_roi_extent);
	}
	if (inputCellType.concentration_adjustment_factor != currentCellType.concentration_adjustment_factor) {
		ss << boost::str (boost::format ("ConcAdjustmentFactor: %4.2f -> %4.2f\n") % currentCellType.concentration_adjustment_factor % inputCellType.concentration_adjustment_factor);
	}

	if (inputCellType.analysis_specializations.size() == 0)
	{
		ss << "AnalysisSpecializations: cleared\n";
	}
	else
	{
		std::vector<AnalysisDefinitionDLL> updated_analysis_special = {};
		// Compare and update each entry.
		for (const auto& input_ct_ad : inputCellType.analysis_specializations)
		{
			auto found_it = std::find_if(currentCellType.analysis_specializations.begin(),
				currentCellType.analysis_specializations.end(),
				[input_ct_ad](const AnalysisDefinitionDLL& ad)->bool
			{
				return input_ct_ad.analysis_index == ad.analysis_index;
			});

			if (found_it != currentCellType.analysis_specializations.end())
			{
				AnalysisDefinitionDLL ad = *found_it;
				ss << AnalysisDefinitionDLL::UpdateAnalysisDefinition (input_ct_ad, ad);
				updated_analysis_special.push_back(ad);
			} else
			{
				updated_analysis_special.push_back(input_ct_ad);
				ss << boost::str(boost::format("AnalysisSpecializations: add %s\n") 
					% AnalysisDefinitionDLL::ToStr (input_ct_ad));
			}
		}
	}

	if (inputCellType.concentration_adjustment_factor != currentCellType.concentration_adjustment_factor) {
		ss << boost::str (boost::format ("ConcentrationAdjustmentFactor: %3.2f -> %3.2f\n") % currentCellType.concentration_adjustment_factor % inputCellType.concentration_adjustment_factor);
	}

	if (ss.str().empty())
		return "";

	// Archive the OLD data starts here:
	DBApi::DB_CellTypeRecord dbCellTypeRecord = currentCellType.ToDbStyle();
	dbCellTypeRecord.CellTypeIndex = currentCellType.celltype_index;
	DBApi::DB_CellTypeRecord ctr = {};
	DBApi::eQueryResult dbStatus = DBApi::DbFindCellTypeByIndex(ctr, dbCellTypeRecord.CellTypeIndex);
	if (dbStatus == DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "UpdateCellType: successfuly found cell type by index in the database");
		dbCellTypeRecord.CellTypeId = ctr.CellTypeId;
		dbCellTypeRecord.CellTypeIdNum = ctr.CellTypeIdNum;
		for(int j = 0; j < ctr.SpecializationsDefList.size(); j++)
		{
			dbCellTypeRecord.SpecializationsDefList[j].AnalysisDefId = ctr.SpecializationsDefList[j].AnalysisDefId;
			dbCellTypeRecord.SpecializationsDefList[j].AnalysisDefIdNum = ctr.SpecializationsDefList[j].AnalysisDefIdNum;
			for (int k = 0; k < dbCellTypeRecord.SpecializationsDefList[j].AnalysisParamList.size(); k++)
			{
				dbCellTypeRecord.SpecializationsDefList[j].AnalysisParamList[k].ParamId = ctr.SpecializationsDefList[j].AnalysisParamList[k].ParamId;
				dbCellTypeRecord.SpecializationsDefList[j].AnalysisParamList[k].ParamIdNum = ctr.SpecializationsDefList[j].AnalysisParamList[k].ParamIdNum;
			}
		}

		// Updating a CellType now forces the existing CellType to be "retired" and a new user-defined CellType created.		
		dbCellTypeRecord.Retired = true;

		dbStatus = DBApi::DbModifyCellType(dbCellTypeRecord);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::celltype,
				instrument_error::severity_level::warning));

			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("UpdateCellType: <exit, unable to modify database cell type, status: %ld>") % (int32_t)dbStatus));
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::normal, "UpdateCellType: successfuly modified cell type in the database");
		}
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("UpdateCellType: <exit, unable to find database cell type by index, status: %ld>") % (int32_t)dbStatus));
	}

	auto celltype_changes_str = "name: " + currentCellType.label + " and index: " + std::to_string(currentCellType.celltype_index) + ":\n" + ss.str();
	return celltype_changes_str;

}

//*****************************************************************************
void CellTypeDLL::freeCelltypeInternal (CellType& ct)
{
	if (ct.label)
	{
		delete[] ct.label;
		ct.label = nullptr;
	}

	if (ct.num_cell_identification_parameters > 0)
	{
		for (uint8_t index = 0; index < ct.num_cell_identification_parameters; index++)
		{
			AnalysisDefinitionDLL::freeAnalysisParametersInternal (ct.cell_identification_parameters[index]);
		}
		delete[] ct.cell_identification_parameters;
		ct.cell_identification_parameters = nullptr;
		ct.num_cell_identification_parameters = 0;
	}

	AnalysisDefinitionsDLL::freeAnalysisDefinitionsInternal (ct.analysis_specializations, ct.num_analysis_specializations);
}
