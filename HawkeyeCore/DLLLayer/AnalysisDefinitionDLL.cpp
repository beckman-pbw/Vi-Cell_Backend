#include "stdafx.h"

#include <stdint.h>

#include <boost/property_tree/ptree.hpp>

#include "AnalysisDefinitionDLL.hpp"
#include "AppConfig.hpp"
#include "SetIndentation.hpp"

static const char MODULENAME[] = "AnalysisDefinitionDLL";

//*****************************************************************************
AnalysisDefinitionDLL::AnalysisDefinitionDLL (AnalysisDefinition ad)
{
	analysis_index = ad.analysis_index;
	DataConversion::convertToStandardString(label, ad.label);
	mixing_cycles = ad.mixing_cycles;

	reagent_indices = DataConversion::create_vector_from_Clist<uint32_t> (ad.reagent_indices, ad.num_reagents);

	fl_illuminators = DataConversion::create_vector_from_Clist (ad.fl_illuminators, ad.num_fl_illuminators);

	DataConversion::convert_listOfCType_to_vecOfDllType (ad.analysis_parameters, ad.num_analysis_parameters, analysis_parameters);

	population_parameter.reset();
	if (ad.population_parameter)
	{
		AnalysisParameterDLL ap{ *(ad.population_parameter) };
		population_parameter.reset(ap);
	}
}

//*****************************************************************************
bool AnalysisDefinitionDLL::isUserDefined (uint32_t ad_index)
{
	return (ad_index & USER_ANALYSIS_TYPE_MASK) ? true : false;
}

//*****************************************************************************
AnalysisDefinition AnalysisDefinitionDLL::ToCStyle()
{
	AnalysisDefinition ad = {};

	DataConversion::convertToCharPointer(ad.label, label);

	ad.mixing_cycles = mixing_cycles;

	DataConversion::create_Clist_from_vector (reagent_indices, ad.reagent_indices, ad.num_reagents);

	DataConversion::create_Clist_from_vector (fl_illuminators, ad.fl_illuminators, ad.num_fl_illuminators);

	DataConversion::convert_vecOfDllType_to_listOfCType (analysis_parameters, ad.analysis_parameters, ad.num_analysis_parameters);

	ad.population_parameter = nullptr;
	if (population_parameter.is_initialized())
	{
		ad.population_parameter = new AnalysisParameter();
		*(ad.population_parameter) = population_parameter->ToCStyle();
	}

	return ad;
}

//*****************************************************************************
DBApi::DB_AnalysisDefinitionRecord AnalysisDefinitionDLL::ToDbStyle() const
{
	DBApi::DB_AnalysisDefinition ad = {};

	ad.AnalysisDefId = uuid;

	ad.AnalysisDefIndex = analysis_index;
	ad.AnalysisLabel = label;

	ad.NumReagents = ( int8_t ) reagent_indices.size();
	for (auto idxIter = reagent_indices.begin(); idxIter != reagent_indices.end(); ++idxIter)
	{
		ad.ReagentIndexList.push_back( *idxIter );
	}

	ad.MixingCycles = mixing_cycles;

	ad.NumFlIlluminators = ( int8_t ) fl_illuminators.size();
	ad.IlluminatorsList.clear();
	if ( ad.NumFlIlluminators > 0 )
	{
		for ( auto flIter = fl_illuminators.begin(); flIter != fl_illuminators.end(); ++flIter)
		{
			DBApi::DB_IlluminatorRecord flrec;

			if ( DBApi::DbFindIlluminatorsByWavelength( flrec, flIter->emission_wavelength_nm, flIter->illuminator_wavelength_nm ) != DBApi::eQueryResult::QueryOk )
			{
				flrec.IlluminatorIndex = -1;
				flrec.IlluminatorIdNum = 0;
				flrec.Protected = true;
				flrec.IlluminatorNameStr = "";
				flrec.IlluminatorWavelength = flIter->illuminator_wavelength_nm;
				flrec.EmissionWavelength = flIter->emission_wavelength_nm;
				flrec.ExposureTimeMs = flIter->exposure_time_ms;
			}

			ad.IlluminatorsList.push_back( flrec );
		}
	}

	ad.NumAnalysisParams = ( int8_t ) analysis_parameters.size();
	ad.AnalysisParamList.clear();

	if ( ad.NumAnalysisParams > 0 )
	{
		for ( auto apIter = analysis_parameters.begin(); apIter != analysis_parameters.end(); ++apIter)
		{
			DBApi::DB_AnalysisParamRecord aprec;
			aprec.ParamIdNum = 0;
			aprec.ParamId = {};
			aprec.Protected = false;
			aprec.ParamLabel = apIter->label;
			aprec.Characteristics.key = apIter->characteristic.key;
			aprec.Characteristics.s_key = apIter->characteristic.s_key;
			aprec.Characteristics.s_s_key = apIter->characteristic.s_s_key;
			aprec.ThresholdValue = apIter->threshold_value;
			aprec.AboveThreshold = apIter->above_threshold;

			ad.AnalysisParamList.push_back( aprec );
		}
	}

	DBApi::DB_AnalysisParamRecord pop_aprec;
	pop_aprec.ParamIdNum = 0;
	pop_aprec.ParamId = {};
	pop_aprec.Protected = true;

	pop_aprec.IsInitialized = population_parameter.is_initialized();
	if ( pop_aprec.IsInitialized )
	{
		pop_aprec.ParamLabel = population_parameter->label;
		pop_aprec.Characteristics.key = population_parameter->characteristic.key;
		pop_aprec.Characteristics.s_key = population_parameter->characteristic.s_key;
		pop_aprec.Characteristics.s_s_key = population_parameter->characteristic.s_s_key;
		pop_aprec.ThresholdValue = population_parameter->threshold_value;
		pop_aprec.AboveThreshold = population_parameter->above_threshold;
	}
	else
	{
		pop_aprec.ParamLabel = "";
		pop_aprec.Characteristics.key = 0;
		pop_aprec.Characteristics.s_key = 0;
		pop_aprec.Characteristics.s_s_key = 0;
		pop_aprec.ThresholdValue = 0.0;
		pop_aprec.AboveThreshold = false;
	}

	return ad;
}

//*****************************************************************************
void updateFL_IlluminationSettings (std::stringstream& ss, const FL_IlluminationSettings& inputFL_IlluminationSettings, FL_IlluminationSettings& currentFL_IlluminationSettings) {

	if (inputFL_IlluminationSettings.illuminator_wavelength_nm != currentFL_IlluminationSettings.illuminator_wavelength_nm) {
		ss << boost::str (boost::format ("FL_Illuminator::IlluminatorWavelength: %d -> %d\n") % currentFL_IlluminationSettings.illuminator_wavelength_nm % inputFL_IlluminationSettings.illuminator_wavelength_nm);
	}
	if (inputFL_IlluminationSettings.emission_wavelength_nm != currentFL_IlluminationSettings.emission_wavelength_nm) {
		ss << boost::str (boost::format ("FL_Illuminator::EmissionWavelength: %d -> %d\n") % currentFL_IlluminationSettings.emission_wavelength_nm % inputFL_IlluminationSettings.emission_wavelength_nm);
	}
	if (inputFL_IlluminationSettings.exposure_time_ms != currentFL_IlluminationSettings.exposure_time_ms) {
		ss << boost::str (boost::format ("FL_Illuminator::ExposureTime: %d -> %d\n") % currentFL_IlluminationSettings.exposure_time_ms % inputFL_IlluminationSettings.exposure_time_ms);
	}
}

//*****************************************************************************
void updateAnalysisCharacteristic (std::stringstream& ss, const Hawkeye::Characteristic_t& inputCharacteristic, Hawkeye::Characteristic_t& currentCharacteristic) {

	if (inputCharacteristic.key != currentCharacteristic.key) {
		ss << boost::str (boost::format ("Characteristic::Key: %d -> %d\n") % currentCharacteristic.key % inputCharacteristic.key);
	}
	if (inputCharacteristic.s_key != currentCharacteristic.s_key) {
		ss << boost::str (boost::format ("Characteristic::S_Key: %d -> %d\n") % currentCharacteristic.s_key % inputCharacteristic.s_key);
	}
	if (inputCharacteristic.s_s_key != currentCharacteristic.s_s_key) {
		ss << boost::str (boost::format ("Characteristic::S_S_Key: %d -> %d\n") % currentCharacteristic.s_s_key % inputCharacteristic.s_s_key);
	}
}

//*****************************************************************************
void AnalysisDefinitionDLL::UpdateAnalysisParameter (std::stringstream& ss, const AnalysisParameterDLL& inputAnalysisParameter, AnalysisParameterDLL& currentAnalysisParameter) {

	if (inputAnalysisParameter.label != currentAnalysisParameter.label) {
		ss << boost::str (boost::format ("AnalysisParameter::Label: %s -> %s\n") % currentAnalysisParameter.label % inputAnalysisParameter.label);
	}

	updateAnalysisCharacteristic (ss, inputAnalysisParameter.characteristic, currentAnalysisParameter.characteristic);

	if (inputAnalysisParameter.threshold_value != currentAnalysisParameter.threshold_value) {
		ss << boost::str (boost::format ("%s::ThresholdValue: %3.2f -> %3.2f\n") % currentAnalysisParameter.label % currentAnalysisParameter.threshold_value % inputAnalysisParameter.threshold_value);
	}
	if (inputAnalysisParameter.above_threshold != currentAnalysisParameter.above_threshold) {
		ss << boost::str (boost::format ("AnalysisParameter::AboveThreshold: %d -> %d\n")
			% DataConversion::boolToString(currentAnalysisParameter.above_threshold)
			% DataConversion::boolToString(inputAnalysisParameter.above_threshold));
	}
}

//*****************************************************************************
std::string AnalysisDefinitionDLL::UpdateAnalysisDefinition (const AnalysisDefinitionDLL& inputAnalysisDefinition, AnalysisDefinitionDLL& currentAnalysisDefinition) {

	std::stringstream ss;

	if (inputAnalysisDefinition.label != currentAnalysisDefinition.label) {
		ss << boost::str (boost::format ("Label: %s -> %s\n") % currentAnalysisDefinition.label % inputAnalysisDefinition.label);
	}

	if (inputAnalysisDefinition.reagent_indices.size() == currentAnalysisDefinition.reagent_indices.size()) {

		// Compare and update each entry.
		for (size_t i = 0; i < currentAnalysisDefinition.reagent_indices.size(); i++) {
			if (inputAnalysisDefinition.reagent_indices[i] != currentAnalysisDefinition.reagent_indices[i]) {
				ss << boost::str (boost::format ("ReagentIndex: %d -> %d\n") 
					% currentAnalysisDefinition.reagent_indices[i]
					% inputAnalysisDefinition.reagent_indices[i]);
			}
		}

	}
	else if (inputAnalysisDefinition.reagent_indices.size() > 0) {
		ss << boost::str(boost::format("ReagentIndices: cleared\n"));
		// Add the new entries.
		for (size_t i = 0; i < inputAnalysisDefinition.reagent_indices.size(); i++) {
			ss << boost::str(boost::format("ReagentIndex: add %d\n") % inputAnalysisDefinition.reagent_indices[i]);
		}

	}

	if (currentAnalysisDefinition.mixing_cycles != inputAnalysisDefinition.mixing_cycles) {
		ss << boost::str (boost::format ("MixingCycles: %d -> %d\n") 
			% (int)currentAnalysisDefinition.mixing_cycles 
			% (int)inputAnalysisDefinition.mixing_cycles);
	}

	if (inputAnalysisDefinition.fl_illuminators.size() == 0 && currentAnalysisDefinition.fl_illuminators.size()) {
		ss << boost::str (boost::format ("FL_Illuminators: cleared\n"));

	} else {

		if (inputAnalysisDefinition.fl_illuminators.size() == currentAnalysisDefinition.fl_illuminators.size()) {

			// Compare and update each entry.
			for (size_t i = 0; i < currentAnalysisDefinition.fl_illuminators.size(); i++) {
				updateFL_IlluminationSettings (ss, inputAnalysisDefinition.fl_illuminators[i], currentAnalysisDefinition.fl_illuminators[i]);
			}

		} else {

			// Delete the current entries.
			ss << boost::str (boost::format ("FL_Illuminators: cleared\n"));

			// Add the new entries.
			for (size_t i = 0; i < inputAnalysisDefinition.fl_illuminators.size(); i++) {
				ss << boost::str (boost::format ("FL_Illuminator: add %d\n") % inputAnalysisDefinition.fl_illuminators[i].illuminator_wavelength_nm);
			}
		}
	}

	if (inputAnalysisDefinition.analysis_parameters.size() == currentAnalysisDefinition.analysis_parameters.size()) {

		// Compare and update each entry.
		for (size_t i = 0; i < currentAnalysisDefinition.analysis_parameters.size(); i++) {
			UpdateAnalysisParameter (ss, inputAnalysisDefinition.analysis_parameters[i], currentAnalysisDefinition.analysis_parameters[i]);
		}

	} else {

		// Delete the current entries.
		ss << boost::str (boost::format ("AnalysisParmeter: cleared\n"));

		// Add the new entries.
		for (size_t i = 0; i < currentAnalysisDefinition.analysis_parameters.size(); i++) {
			ss << boost::str (boost::format ("AnalysisParmeter: add %s\n") % inputAnalysisDefinition.analysis_parameters[i].label);
		}
	}

	// Update population_parameter with inputAnalysisDefinition.
	if (currentAnalysisDefinition.population_parameter) {
		ss << boost::str (boost::format ("PopulationParameter: cleared\n"));
	}

	if (inputAnalysisDefinition.population_parameter) {
		AnalysisParameterDLL ap;
		UpdateAnalysisParameter (ss, inputAnalysisDefinition.population_parameter.get(), ap);
	}

	if (ss.str().empty())
		return "";

	auto ad_changes_str = "AnalysisDefintion name: " + currentAnalysisDefinition.label + " and index: " + std::to_string(currentAnalysisDefinition.analysis_index) + " Changes :\n" + ss.str();
	return ad_changes_str;
}

//*****************************************************************************
std::string AnalysisDefinitionDLL::ToStr (const AnalysisDefinitionDLL& ad, uint8_t tabCount)
{
	std::string reagent_indices_str = {};
	for (auto& item : ad.reagent_indices)
	{
		reagent_indices_str += std::to_string(item);
	}

	std::string analysis_parameters_str = {};
	for (auto& item : ad.analysis_parameters)
	{
		analysis_parameters_str += SetIndentation(tabCount + 2);
		analysis_parameters_str += AnalysisParameterDLL::ToStr (item, tabCount + 2);
	}

	std::string population_parameter_str = {};
	if (ad.population_parameter)
	{
		population_parameter_str += SetIndentation(tabCount + 2);
		population_parameter_str += AnalysisParameterDLL::ToStr (*ad.population_parameter, tabCount + 2);
	}

	std::string analysis_str = boost::str(boost::format(
		"%sanalysis_index: %d"
		"%slabel: %s"
		"%sreagent_indices: %s"
		"%smixing_cycles: %d"
		"%sanalysis_parameters: %s")
		% SetIndentation(tabCount, false)
		% ad.analysis_index
		% SetIndentation(tabCount + 1)
		% ad.label
		% SetIndentation(tabCount + 1)
		% reagent_indices_str
		% SetIndentation(tabCount + 1)
		% (int)ad.mixing_cycles
		% SetIndentation(tabCount + 1)
		% analysis_parameters_str);
	if (!population_parameter_str.empty())
		analysis_str += boost::str(boost::format(
			"%spopulation_parameter: %s")
			% SetIndentation(tabCount + 1)
			% population_parameter_str);

	return analysis_str;
}

//*****************************************************************************
void AnalysisDefinitionDLL::freeAnalysisDefinitionInternal(AnalysisDefinition& ad)
{
	if (ad.label)
	{
		delete[] ad.label;
		ad.label = nullptr;
	}

	if (ad.num_reagents > 0)
	{
		delete[] ad.reagent_indices;
		ad.reagent_indices = nullptr;
		ad.num_reagents = 0;
	}

	if (ad.num_fl_illuminators > 0)
	{
		delete[] ad.fl_illuminators;
		ad.fl_illuminators = nullptr;
		ad.num_fl_illuminators = 0;
	}

	if (ad.num_analysis_parameters > 0)
	{
		for (uint8_t index = 0; index <ad.num_analysis_parameters; index++)
		{
			freeAnalysisParametersInternal(ad.analysis_parameters[index]);
		}
		delete[] ad.analysis_parameters;
		ad.analysis_parameters = nullptr;
		ad.num_analysis_parameters = 0;
	}

	if (ad.population_parameter)
	{
		delete ad.population_parameter;
		ad.population_parameter = nullptr;
	}
}

//*****************************************************************************
void AnalysisDefinitionDLL::freeAnalysisParametersInternal(AnalysisParameter& ap)
{
	if (ap.label)
	{
		delete[] ap.label;
		ap.label = nullptr;
	}
}
