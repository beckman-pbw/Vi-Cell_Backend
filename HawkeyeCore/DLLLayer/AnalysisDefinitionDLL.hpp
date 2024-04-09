#pragma once

#include <cstdint>
#include <vector>

#include <boost/optional.hpp>

#include <DBif_Api.h>

#include "AnalysisDefinition.hpp"
#include "DataConversion.hpp"
#include "Logger.hpp"
#include "ReagentDLL.hpp"
#include "SetIndentation.hpp"
#include "SystemErrors.hpp"

// Cell types 0x0000 to 0x7FFF are BCI-supplied.
// Cell types 0x8000 to 0xFFFF are customer-generated.
#define USER_ANALYSIS_TYPE_MASK 0x8000

#define TEMP_ANALYSIS_INDEX USHRT_MAX

/*
* An Analysis Parameter describes a particular piece of data calculated and exposed
*  by the image analysis sub-system.  The Parameters are used to identify the population
*  of cells within an image which meet a given set of criteria.  Parameters are combined
*  with "AND" logic.
* Each parameter has:
*	label - this is a null-terminated ASCII text string used as a lookup for the value
*  threshold_value - some value of interest for the analysis in which this parameter is a part of.
*                    Range varies according to the particular parameter.
*  above_threshold - "true" if we are interested in cells with values AT/ABOVE the threshold, "false" if
*                    the interesting population is BELOW the threshold
*
* Example parameter combination for calculating the Analysis Definition "Viability, Trypan Blue" (as a percentage of population)
*   "BF_Spot_Brightness",0.65,true
*   "BF_Spot_Area_um",25.2,true
*  These two parameters will select out the cells with a central bright spot at least 65% of the full-scale value and
*   which occupies a minimum of 25.2 square microns.
*/
struct AnalysisParameterDLL
{
	AnalysisParameterDLL()
	{
		characteristic.key = 0;
		threshold_value = 0.0;
		above_threshold = false;
	};

	AnalysisParameterDLL( const AnalysisParameter& ap )
	{
		DataConversion::convertToStandardString( label, ap.label );
		characteristic = ap.characteristic;
		threshold_value = ap.threshold_value;
		above_threshold = ap.above_threshold;
	}

	bool operator== (const AnalysisParameterDLL& rhs) const
	{
		return (
			(label == rhs.label) &&
			(characteristic.key == rhs.characteristic.key) &&
			(characteristic.s_key == rhs.characteristic.s_key) &&
			(characteristic.s_s_key == rhs.characteristic.s_s_key) &&
			(threshold_value == rhs.threshold_value) &&
			(above_threshold == rhs.above_threshold)
			);
	}

	AnalysisParameter ToCStyle()
	{
		AnalysisParameter ap = {};

		DataConversion::convertToCharPointer( ap.label, label );
		ap.characteristic = characteristic;
		ap.threshold_value = threshold_value;
		ap.above_threshold = above_threshold;

		return ap;
	}

	DBApi::DB_AnalysisParamRecord ToDbStyle()
	{
		DBApi::DB_AnalysisParamRecord ap = {};

		ap.ParamLabel = label;
		ap.Characteristics.key = characteristic.key;
		ap.Characteristics.s_key = characteristic.s_key;
		ap.Characteristics.s_s_key = characteristic.s_s_key;
		ap.ThresholdValue = threshold_value;
		ap.AboveThreshold = above_threshold;

		return ap;
	}

	void FromDBStyle (DBApi::DB_AnalysisParamRecord ap)
	{
		label = ap.ParamLabel;
		characteristic = ap.Characteristics;
		threshold_value = ap.ThresholdValue;
		above_threshold = ap.AboveThreshold;
	}

	static std::string ToStr (const AnalysisParameterDLL& ap, uint8_t tabCount)
	{
		return boost::str(boost::format(
			"label: %s"
			"%scharacteristic: %d_%d_%d"
			"%sthreshold_value: %3.2f"
			"%sabove_threshold: %s")
			% ap.label
			% SetIndentation(tabCount + 1)
			% ap.characteristic.key % ap.characteristic.s_key % ap.characteristic.s_s_key
			% SetIndentation(tabCount + 1)
			% ap.threshold_value
			% SetIndentation(tabCount + 1)
			% (ap.above_threshold ? "eAbove" : "eBelow"));
	}

	std::string label;
	Hawkeye::Characteristic_t characteristic;
	float threshold_value; // 32bits on 64bit system.
	bool above_threshold;
};


/*
 * Analysis Definitions will be added as new reagent containers are 
 * introduced to the system.  Each container has information on
 * its contents, capacities and also how analyses that can be done
 * using the reagents onboard.
 */
class AnalysisDefinitionDLL
{
public:
	AnalysisDefinitionDLL()
	{
		analysis_index = USHRT_MAX;
		mixing_cycles = 0;
	}
	AnalysisDefinitionDLL (AnalysisDefinition ad);
	virtual ~AnalysisDefinitionDLL() {};

	AnalysisDefinition ToCStyle();
	DBApi::DB_AnalysisDefinitionRecord ToDbStyle() const;

	void FromDbStyle (const DBApi::DB_AnalysisDefinitionRecord& dbAnalysisDef)
	{
		uuid = dbAnalysisDef.AnalysisDefId;

		analysis_index = dbAnalysisDef.AnalysisDefIndex;
		label = dbAnalysisDef.AnalysisLabel;

		//dbAnalysisDef.NumReagents = (int8_t)reagent_indices.size();
		reagent_indices = {};
		for (auto& v : dbAnalysisDef.ReagentIndexList)
		{
			reagent_indices.push_back (v);
		}

		mixing_cycles = static_cast<uint8_t>(dbAnalysisDef.MixingCycles);

		fl_illuminators = {};
		for (auto& v : dbAnalysisDef.IlluminatorsList)
		{
			FL_IlluminationSettings fl;

			DBApi::DB_IlluminatorRecord flrec;

			DBApi::eQueryResult dbStatus = DBApi::DbFindIlluminatorsByWavelength (flrec, v.EmissionWavelength, v.IlluminatorWavelength);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				//NOTE: cannot log 
				Logger::L().Log ("AnalysisParameterDLL", severity_level::error,
					boost::str (boost::format ("AnalysisDefinitionDLL:::FromDbStyle <DbFindIlluminatorsByWavelength failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_readerror,
					instrument_error::instrument_storage_instance::illuminator,
					instrument_error::severity_level::error));
			}

			fl.emission_wavelength_nm = flrec.EmissionWavelength;
			fl.exposure_time_ms = flrec.ExposureTimeMs;
			fl.illuminator_wavelength_nm = flrec.IlluminatorWavelength;

			fl_illuminators.push_back (fl);
		}

		analysis_parameters = {};
		for (auto& v : dbAnalysisDef.AnalysisParamList)
		{
			AnalysisParameterDLL ap;
			ap.label = v.ParamLabel;
			ap.characteristic = v.Characteristics;
			ap.threshold_value = v.ThresholdValue;
			ap.above_threshold = v.AboveThreshold;
			analysis_parameters.push_back (ap);
		}

//TODO: needs DB update to allow this to work...
		//DBApi::DB_AnalysisParamsRecord pop_aprec;
		//if (dbAnalysisDef.PopulationParams.IsInitialized)
		//{
		//	population_parameter->label = dbAnalysisDef.PopulationParams.ParamLabel;
		//	population_parameter->characteristic = dbAnalysisDef.PopulationParams.Characteristics;
		//	population_parameter->threshold_value = dbAnalysisDef.PopulationParams.ThresholdValue;
		//	population_parameter->above_threshold = dbAnalysisDef.PopulationParams.AboveThreshold;
		//}
	}

	static bool isUserDefined (uint32_t ad_index);
	static std::string UpdateAnalysisDefinition (const AnalysisDefinitionDLL& inputAnalysisDefinition, AnalysisDefinitionDLL& currentAnalysisDefinition);
	static void UpdateAnalysisParameter (std::stringstream& ss, const AnalysisParameterDLL& inputAnalysisParameter, AnalysisParameterDLL& currentAnalysisParameter);
	static std::string ToStr (const AnalysisDefinitionDLL& ad, uint8_t tabCount = 0);
	static void freeAnalysisDefinitionInternal (AnalysisDefinition& ad);
	static void freeAnalysisParametersInternal (AnalysisParameter& ap);

	uuid__t uuid;

	/// Unique-to-system index value.
	/// Values 0x0000 to 0x7FFF are reserved for 
	///   Beckman-Coulter-defined analyses.
	/// Values 0x8000 through 0xFFFF are reserved for
	///   customer-defined analyses.
	uint16_t analysis_index;

	/// Label must be unique-to-system.
	std::string label;

	/// Identifier for the required reagent type(s).
	/// An analysis may not require ANY reagents (ex: GFP-positive cells; these cells naturally 
	///  express a fluorescent protein without the aid of an external reagent)
	std::vector<uint32_t> reagent_indices; // Ref: ReagentDefinition::reagent_index

	/// Number of times to mix the reagent(s) with the sample
	/// CAN BE ALTERED BY A SPECIALIZATION WITHIN A CELL TYPE
	uint8_t mixing_cycles;

	/*
	 * Fluorescent analysis parameters - these are interpreted as "AND"
	 * conditions (that is: the interesting population are the set of cells
	 * that meet analysisparameter[0], AND meet analysisparameter[1], AND....
	 * It is assumed that there is always a *bright field* illumination.
	 * For Scout, *num_fl_illuminators* is zero.
	 */
	std::vector<FL_IlluminationSettings> fl_illuminators;

	std::vector<AnalysisParameterDLL> analysis_parameters;

	boost::optional<AnalysisParameterDLL> population_parameter;

	bool ContainsFluorescence() const { return !fl_illuminators.empty(); }
	bool IsBCIAnalysis() const { return analysis_index < USER_ANALYSIS_TYPE_MASK; }
	bool IsCustomerAnalysis() { return !IsBCIAnalysis(); }
};
