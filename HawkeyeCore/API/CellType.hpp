#pragma once

#include <cstdint>

#include "AnalysisDefinition.hpp"
#include "AnalysisDefinitionCommon.hpp"

enum eCellDeclusterSetting : uint16_t
{
	eDCNone = 0,// Declustering disabled.
	eDCLow,	    // Wider spacing between potential cell centers
				// higher edge threshold
				// higher accumulator (less aggressive, more missed cells)

	eDCMedium,  // Middle of the road

	eDCHigh,    // Narrower spacing between potential cell centers
				// lower edge threshold
				// lower accumulator (more aggressive, more false positives)
};

/*
 * CellType describes the characteristics used to identify and select
 * a cell within the brightfield image.
 */
struct CellType
{
	/// Cell types 0x00000000 to 0x7FFFFFFF are BCI-supplied.
	/// Cell types 0x80000000 to 0xFFFFFFFF are customer-generated.
	uint32_t celltype_index;
	char* label;

	// Number of images to acquire/analyze for a sample using this cell type
	uint16_t max_image_count;

	// Number of aspiration cycles used to re-suspend particles in the sample
	uint8_t aspiration_cycles;

	// Exclusion zone on top/left of image to balance out partial cells on image borders.
	// These fields are no longer part of the actual CellType definition.
	// They are only here as a mechanism to pass them from the CellCouning configuration to the UI.
	uint16_t roi_x_pixels;
	uint16_t roi_y_pixels;

	/*NB: We would be wise to redefine these in terms of characteristic_t values
	      so that we can have whatever set of things we want to 
     */

	// Cell morphology parameters:
	float minimum_diameter_um;
	float maximum_diameter_um;
	float minimum_circularity; // 0.0-1.0 : 1.0 is perfectly circular
	float sharpness_limit; // 0.0-1.0 : 1.0 is perfectly sharp

	/*
	 * The collection of parameters (characteristic, threshold, polarity) that are used
	 *  to identify this cell type within the collection of identified/measured blobs.
	 * These REPLACE the traditional hard-coded set of ViCell cell morphology parameters
	 *  and allow for flexibility and future expansion.
	 */
	uint8_t num_cell_identification_parameters;
	AnalysisParameter* cell_identification_parameters;
	
	// Declustering
	eCellDeclusterSetting decluster_setting; // Effort given to identify individual cells within a cluster
	
	// For Fluorescent analysis:
	// Region of interest within the FL images in relation to the 
	// cell's boundaries defined in the brightfield image.
	// Multiplier: 1.0 = exact ROI as defined by bright field; 0.5 = half of BF ROI
	float fl_roi_extent; 
	
	// Specializations of factory/user analysis parameters
	// For each analysis type known to the instrument, the user may specialize it 
	//  to the cell type under test.
	// If an analysis is requested for a sample, that sample's cell-type instance is first checked for
	//  a specialization for that analysis.  If one exists, that parameter set will be used.  If no 
	//  specialization is found, then the default parameters from the factory list will be used.
	//
	// Note: When showing the list of analyses for a sample, go ONLY with the master list.  Do NOT rely
	// on this list as the source.

	uint32_t num_analysis_specializations;
	AnalysisDefinition* analysis_specializations;

	// Adjustment factor used with automation to ensure values are what is expected when calculations are performed
	float concentration_adjustment_factor;
};
