#pragma once

#include <atomic>
#include <functional>
#include <iostream>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "AnalysisDefinitionDLL.hpp"
#include "CalibrationCommon.hpp"
#include "CellCountingOutputParams.h"
#include "CellTypesDLL.hpp"
#include "CompletionHandlerUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "ImageCollection.hpp"
#include "ResultDefinitionCommon.hpp"

// This is used to pass miscellaneous cell processing parameters to the APIs that invoke the CellCounting code.
// Due to the disparite sources of this data (HawkeyeConfig, CellType, ???) the fields in this structure 
// are populated at different point on the way to the CellCounting configuration being set.
typedef struct {
	calibration_type calType;
	uint32_t dilutionFactor;
	eCellDeclusterSetting declusterSetting;
	uint32_t declusterAccumulatorThreshold;
	uint32_t declusterMinimumDistanceThreshold;
	uint32_t concentrationImageControlCount;
} CellProcessingParameters_t;

typedef std::function<void(int imageNum, CellCounterResult::SResult oSCellCountingResult)> CellCountingCallback;

class ImageAnalysisUtilities
{     
public:
	static bool initialize();
	static void reset ();
	static void getDustReferenceImage();
	static bool setCallback (CellCountingCallback cb);
	static bool addImage(const ImageSet_t& imageSet);
	static bool addImages (const ImageCollection_t& imageSets);
	static bool SetCellCountingConfiguration (std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> configParams);
	static bool initalizeCellCountingConfiguration (
		const AnalysisDefinitionDLL& analysisDef,
		const CellTypeDLL& celltype,
		const CellProcessingParameters_t& cellProcessingParameters,
		v_IdentificationParams_t& populationOfInterest,
		v_IdentificationParams_t& generalPopulation,
		std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& configParams);
	static bool SetCellCountingParameters (const AnalysisDefinitionDLL& ad, const CellTypeDLL& ct, CellProcessingParameters_t& cellProcessingParameters);

	static bool clearResult();
	static void startProcess();
	static bool isProcessCompleted();
	static void stopProcess();
	static bool isBusy();
	static void getCellIdentificationParams (const CellTypeDLL& ct, v_IdentificationParams_t& generalPopulation);
	static void getPOIIdentificationParams (const AnalysisDefinitionDLL& ad, v_IdentificationParams_t& populationOfInterest);
	static void getBasicResultAnswers (const CellCounterResult::SImageStats& input, BasicResultAnswers& output);
	static bool writeFCSData (std::string strInputPath, const CellCounterResult::SResult& const_r_stCumulativeOutput);
	static void SetConfigurationParameters (const ConfigParamList_t& map_ParamsList);
	static void SetCellIdentificationParameters(
		v_IdentificationParams_t populationOfInterest,
		v_IdentificationParams_t generalPopulation);
	static void setBrightfieldDustReferenceImage (const ImageSet_t& imageSet);
	static void GetSResult (CellCounterResult::SResult& sresult);
	static void ApplyConcentrationAdjustmentFactor (const CellTypeDLL& cellType, CellCounterResult::SResult& sresult);	
	static std::string GetCharacteristicLabel (Characteristic_t key);
	static std::map<Characteristic_t, std::string> GetAllBlobCharacteristics();
	static ImageSet_t& GetDustReferenceImageSet();

private:

};
