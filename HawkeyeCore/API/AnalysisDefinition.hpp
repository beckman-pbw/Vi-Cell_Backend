#pragma once

#include <cstdint>

#include "AnalysisDefinitionCommon.hpp"
#include "Reagent.hpp"


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
struct AnalysisParameter
{	
	char* label;
	Hawkeye::Characteristic_t characteristic;
	float threshold_value; // 32bits on 64bit system.
	bool above_threshold;
};


/*
 * Analysis Definitions will be added as new reagent bottles are 
 * introduced to the system.  Each bottle contains information on
 * its contents, capacities and also how analyses that can be done
 * using the reagents onboard.
 */
struct AnalysisDefinition
{
	/// Unique-to-system index value.
	/// Values 0x0000 to 0x7FFF are reserved for 
	///   Beckman-Coulter-defined analyses.
	/// Values 0x8000 through 0xFFFF are reserved for
	///   customer-defined analyses.
	uint16_t analysis_index;

	/// Label must be unique-to-system.	
	char* label;

	/// Identifier for the required reagent type(s).
	/// An analysis may not require ANY reagents (ex: GFP-positive cells; these cells naturally 
	///  express a fluorescent protein without the aid of an external reagent)
	uint8_t num_reagents;
	uint32_t* reagent_indices;		// Ref: ReagentDefinition::reagent_index

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
	uint8_t num_fl_illuminators;
	FL_IlluminationSettings* fl_illuminators;

	uint8_t num_analysis_parameters;
	AnalysisParameter* analysis_parameters;
  
	/*
	Population-parameter - It is an optional analysis parameter.
	NULL - if no population_paramter is defined.
	*/
	AnalysisParameter* population_parameter;

	bool ContainsFluorescence() const { return num_fl_illuminators > 0; }
	bool IsBCIAnalysis() const { return analysis_index < 0x8000; }
	bool IsCustomerAnalysis() { return !IsBCIAnalysis(); }
};
