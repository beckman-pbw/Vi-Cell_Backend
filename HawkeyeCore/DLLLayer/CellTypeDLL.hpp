#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <DBif_Api.h>

#include "AnalysisDefinitionDLL.hpp"
#include "CellType.hpp"
#include "DataConversion.hpp"

#define TEMP_CELLTYPE_INDEX USHRT_MAX
static constexpr uint16_t ROI_X_PIXELS = 60;
static constexpr uint16_t ROI_Y_PIXELS = 60;

// Cell types 0x00000000 to 0x7FFFFFFF are BCI-supplied.
// Cell types 0x80000000 to 0xFFFFFFFF are customer-generated.
#define USER_CELLTYPE_MASK 0x80000000

/*
 * CellType describes the characteristics used to identify and select
 * a cell within the brightfield image.
 */
class CellTypeDLL
{
public:
	CellTypeDLL()
	{
		celltype_index = ULONG_MAX;
		max_image_count = 0;
		aspiration_cycles = 0;
		roi_x_pixels = ROI_X_PIXELS;
		roi_y_pixels = ROI_Y_PIXELS;
		minimum_diameter_um = 0;
		maximum_diameter_um = 0;
		minimum_circularity = 0;
		sharpness_limit = 0;
		decluster_setting = eDCNone;
		fl_roi_extent = 0;
		concentration_adjustment_factor = 0;
		retired = false;
	};
	CellTypeDLL (CellType ct);
	virtual ~CellTypeDLL() {};

	CellType ToCStyle();
	DBApi::DB_CellTypeRecord ToDbStyle() const;

	void CellTypeDLL::FromDbStyle (DBApi::DB_CellTypeRecord& dbCellType)
	{
		uuid = dbCellType.CellTypeId;

		celltype_index = static_cast<uint32_t>(dbCellType.CellTypeIndex);
		label = dbCellType.CellTypeNameStr;
		max_image_count = dbCellType.MaxImageCount;
		aspiration_cycles = static_cast<uint8_t>(dbCellType.AspirationCycles);
		minimum_diameter_um = dbCellType.MinDiam;
		maximum_diameter_um = dbCellType.MaxDiam;
		minimum_circularity = dbCellType.MinCircularity;
		sharpness_limit = dbCellType.SharpnessLimit;
		
		cell_identification_parameters = {};
		for (auto& v : dbCellType.CellIdentParamList)
		{
			AnalysisParameterDLL ap;
			ap.FromDBStyle (v);
			cell_identification_parameters.push_back (ap);
		}

		decluster_setting = static_cast<eCellDeclusterSetting>(dbCellType.DeclusterSetting);
		fl_roi_extent = dbCellType.RoiExtent;
		roi_x_pixels = dbCellType.RoiXPixels;
		roi_y_pixels = dbCellType.RoiYPixels;

		analysis_specializations = {};

		for (auto& v : dbCellType.SpecializationsDefList)
		{
			AnalysisDefinitionDLL ad;
			ad.FromDbStyle (v);
			analysis_specializations.push_back (ad);
		}

		concentration_adjustment_factor = dbCellType.CalculationAdjustmentFactor;

		retired = dbCellType.Retired;
	}

	static bool isUserDefined (uint32_t ct_index);
	static std::string UpdateCellType (const CellTypeDLL& inputCellType, CellTypeDLL& currentCellType);
	static std::string CellTypeAsString (const CellTypeDLL& ct, uint8_t tabCount = 0);
	static void freeCelltypeInternal (CellType& ct);

	uuid__t uuid;

	/// Cell types 0x00000000 to 0x7FFFFFFF are BCI-supplied.
	/// Cell types 0x80000000 to 0xFFFFFFFF are customer-generated.
	uint32_t celltype_index;
	std::string label;

	// Number of images to acquire/analyze for a sample using this cell type
	uint16_t max_image_count;

	// Number of aspiration cycles used to re-suspend particles in the sample
	uint8_t aspiration_cycles;

	// Exclusion zone on top/left of image to balance out partial cellS on image borders.  
	// These fields are no longer part of the actual CellType definition.
	// They are only here as a mechanism to pass them from the CellCouning configuration to the UI.
	uint16_t roi_x_pixels;
	uint16_t roi_y_pixels;

	// Cell morphology parameters:
	float minimum_diameter_um;
	float maximum_diameter_um;
	float minimum_circularity; // 0.0-1.0 : 1.0 is perfectly circular
	float sharpness_limit;     // 0.0-1.0 : 1.0 is perfectly sharp

	/*
	* The collection of parameters (characteristic, threshold, polarity) that are used
	*  to identify this cell type within the collection of identified/measured blobs.
	* These REPLACE the traditional hard-coded set of ViCell cell morphology parameters
	*  and allow for flexibility and future expansion.
	*/
	std::vector<AnalysisParameterDLL> cell_identification_parameters;

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

	std::vector<AnalysisDefinitionDLL> analysis_specializations;

	// Adjustment factor used with automation to ensure values are what is expected when calculations are performed
	float concentration_adjustment_factor;

	bool retired;
};
