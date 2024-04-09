#include "stdafx.h"

#include "CalibrationHistoryDLL.hpp"
#include "CellCounterFactory.h"
#include "FileSystemUtilities.hpp"
#include "GlobalImageSize.hpp"
#include "HawkeyeAssert.hpp"
#include "HawkeyeDataAccess.h"
#include "ImageAnalysisUtilities.hpp"
#include "ImageWrapperUtilities.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "ImageAnalysisUtilities";

static CellCountingCallback cellCounting_cb_ = nullptr;
static std::shared_ptr<ICellCounter> pCellCounter_;
static ImageSet_t dustReferenceImageSet_;

#ifndef DATAIMPORTER
//*****************************************************************************
static void cellCountingComplete_cb (int imageNum, CellCounterResult::SResult ccResult)
{
//NOTE: save for timing testing...
	//Logger::L ().Log (MODULENAME, severity_level::debug1, "cellCountingComplete_cb: <enter>");

	if (cellCounting_cb_ != nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::normal,
			boost::str (boost::format ("cellCountingComplete_cb: imageNum %d has %d blobs") 
				% imageNum % ccResult.map_Blobs_for_image.size()));
		cellCounting_cb_ (imageNum, ccResult);
	}

//NOTE: save for timing testing...
	//Logger::L ().Log (MODULENAME, severity_level::debug1, "cellCountingComplete_cb: <exit>");
}

//*****************************************************************************
bool ImageAnalysisUtilities::initialize()
{
	if (pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "already initialized...");
		return false;
	}

	pCellCounter_.reset (CellCounter::CellCounterFactory::CreateCellCounterInstance());

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "module is busy performing other tasks!");
		return false;
	}

	pCellCounter_->SetImageDimensions (globalImageSize.height, globalImageSize.width);

	pCellCounter_->RegisterImageResultCallback (cellCountingComplete_cb);

	if (dustReferenceImageSet_.first.data)
	{
		setBrightfieldDustReferenceImage (dustReferenceImageSet_);
	}

	cellCounting_cb_ = nullptr;

	return true;
}

//*****************************************************************************
void ImageAnalysisUtilities::reset()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "reset: <enter>");
	
	if (pCellCounter_)
	{
		pCellCounter_->StopProcess();
		pCellCounter_->ClearResult();
		pCellCounter_.reset();
	}

	initialize();

	Logger::L().Log (MODULENAME, severity_level::debug1, "reset: <exit>");
}

//*****************************************************************************
// This is called at the beginning of each Worklist.
//*****************************************************************************
void ImageAnalysisUtilities::getDustReferenceImage()
{
	if (!dustReferenceImageSet_.first.data)
	{
		if (!HDA_ReadEncryptedImageFile (HawkeyeDirectory::Instance().getDustReferenceImagePath(), dustReferenceImageSet_.first))
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "initialize: no dust image found");
//NOTE: keep for now in case the Core Team changes their minds...
			//ReportSystemError::Instance().ReportError (BuildErrorInstance(
			//	instrument_error::instrument_storage_filenotfound,
			//	instrument_error::instrument_storage_instance::dust_image,
			//	instrument_error::severity_level::warning));
		}
	}
}

//*****************************************************************************
bool ImageAnalysisUtilities::setCallback (CellCountingCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "setCallback: <enter>");

	if (!pCellCounter_)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "setCallback: <exit, module is not initialized>");
		return false;
	}

	if (isBusy ())
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "setCallback: <exit, module is busy performing other tasks>");
		return false;
	}

	cellCounting_cb_ = cb;

	Logger::L().Log (MODULENAME, severity_level::debug1, "setCallback: <exit>");

	return true;
}

//*****************************************************************************
bool ImageAnalysisUtilities::SetCellCountingConfiguration (std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> configParams)
{
	if (!pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "setCellCountingConfiguration: module is not initialized...");
		return false;
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "module is busy performing other tasks!");
		return false;
	}

	if (configParams.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "invalid/empty configuration parameter!");
		return false;
	}

	std::stringstream ss;
	ss << std::endl << "\t*** CellCounting Configuration ***" << std::endl
		<< "\t                          Algorithm Mode: " << ((configParams[ConfigParameters::eAlgorithmMode] == 0) ? "LEGACYXR" : "BLUE") << std::endl
		<< "\t          Background Intensity Tolerance: " << configParams[ConfigParameters::eBackgroundIntensityTolerance] << std::endl
		<< "\t        Bubble Minimum Spot Area Percent: " << configParams[ConfigParameters::eBubbleMinSpotAreaPercent] << std::endl
		<< "\t  Bubble Minimum Spot Average Brightness: " << configParams[ConfigParameters::eBubbleMinSpotAvgBrightness] << std::endl
		<< "\t                             Bubble Mode: " << ((configParams[ConfigParameters::eBubbleMode] == 0) ? "False" : "True") << std::endl
		<< "\t     Bubble Rejection Image Area Percent: " << configParams[ConfigParameters::eBubbleRejectionImgAreaPercent] << std::endl
		<< "\tCell Spot Brightness Exclusion Threshold: " << configParams[ConfigParameters::eCellSpotBrightnessExclusionThreshold] << std::endl
		<< "\t     Center Spot Minimum Intensity Limit: " << configParams[ConfigParameters::eCenterSpotMinIntensityLimit] << std::endl
		<< "\t       Concentration Image Control Count: " << configParams[ConfigParameters::eConcentrationImageControlCount] << std::endl
		<< "\t                 Concentration Intercept: " << configParams[ConfigParameters::eConcentrationIntercept] << std::endl
		<< "\t                     Concentration Slope: " << configParams[ConfigParameters::eConcentrationSlope] << std::endl
		<< "\t         Decluster Accumulator Threshold: " << configParams[ConfigParameters::eDeclusterAccumulatorThreshold] << std::endl
		<< "\t    Decluster Minimum Distance Threshold: " << configParams[ConfigParameters::eDeclusterMinDistanceThreshold] << std::endl
		<< "\t                          Decluster Mode: " << ((configParams[ConfigParameters::eDeclusterMode] == 0) ? "False" : "True") << std::endl
		<< "\t                         Dilution Factor: " << configParams[ConfigParameters::eDilutionFactor] << std::endl
		<< "\t              Hot Pixel Elimination Mode: " << ((configParams[ConfigParameters::eHotPixelEliminationMode] == 0) ? "False" : "True") << std::endl
		<< "\t                Nominal Background Level: " << configParams[ConfigParameters::eNominalBackgroundLevel] << std::endl
		<< "\t     Peak Intensity Selection Area Limit: " << configParams[ConfigParameters::ePeakIntensitySelectionAreaLimit] << std::endl
		<< "\t                        ROI X Coordinate: " << configParams[ConfigParameters::eROIXCoOrdinate] << std::endl
		<< "\t                        ROI Y Coordinate: " << configParams[ConfigParameters::eROIYCoOrdinate] << std::endl
		<< "\t                        Sizing Intercept: " << configParams[ConfigParameters::eSizingIntercept] << std::endl
		<< "\t                            Sizing Slope: " << configParams[ConfigParameters::eSizingSlope] << std::endl
		<< "\t        Small Particle Sizing Correction: " << configParams[ConfigParameters::eSmallParticleSizingCorrection] << std::endl
		<< "\t                SubPeakAnalysisMode Mode: " << ((configParams[ConfigParameters::eSubPeakAnalysisMode] == 0) ? "False" : "True") << std::endl;
	Logger::L().Log (MODULENAME, severity_level::normal, ss.str());

	ss.str("");

	ss << std::endl << "\t*** CellCounting Configuration Errors ***" << std::endl;

	std::map<ConfigParameters::E_CONFIG_PARAMETERS, E_ERRORCODE> configErrorCodes;
	configErrorCodes = pCellCounter_->SetConfigurationParameters (configParams);
	for (auto v : configErrorCodes) {
		if (v.second != eSuccess) 
		{
			uint32_t val1 = v.first;
			uint32_t val2 = v.second;
			ss << boost::str (boost::format ("\tParameter: %d, Error: %d") % (uint32_t)v.first % (uint32_t)v.second) << std::endl;
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::sample_cellcounting_configurationinvalid, 
				instrument_error::severity_level::error));
			Logger::L().Log (MODULENAME, severity_level::error, ss.str());
			return false;
		}
	}
	Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());

	return true;
}

//*****************************************************************************
// Set common CellCounting parameters from HawkeyeConfig.:
// *** MAKE SURE THAT ANY CELLCOUNTING PARAMETER ADDITIONS ARE ALSO ADDED TO      ***
// *** *ImageAnalysisUtilities::setCellCountingConfiguration* FOR LOGGING ***
//*****************************************************************************

//*****************************************************************************
///NB: Call initializeCellCountingParameters prior to calling this so that the decluster information gets set correctly
bool ImageAnalysisUtilities::initalizeCellCountingConfiguration(
	const AnalysisDefinitionDLL& analysisDef,
	const CellTypeDLL& celltype,
	const CellProcessingParameters_t& cellProcessingParameters,
	v_IdentificationParams_t& populationOfInterest,
	v_IdentificationParams_t& generalPopulation,
	std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& configParams)
{
	getCellIdentificationParams (celltype, generalPopulation);
	getPOIIdentificationParams (analysisDef, populationOfInterest);

	const HawkeyeConfig::HawkeyeConfig_t& hawkeyeConfig = HawkeyeConfig::Instance().get();
	configParams.clear();

	configParams[ConfigParameters::eAlgorithmMode] = hawkeyeConfig.cellCounting.algorithmMode;
	configParams[ConfigParameters::eBackgroundIntensityTolerance] = hawkeyeConfig.cellCounting.backgroundIntensityTolerance;
	configParams[ConfigParameters::eBubbleMinSpotAreaPercent] = hawkeyeConfig.cellCounting.bubbleMinimumSpotAreaPercentage;
	configParams[ConfigParameters::eBubbleMinSpotAvgBrightness] = hawkeyeConfig.cellCounting.bubbleMinimumSpotAverageBrightness;
	configParams[ConfigParameters::eBubbleMode] = hawkeyeConfig.cellCounting.bubbleMode;
	configParams[ConfigParameters::eBubbleRejectionImgAreaPercent] = hawkeyeConfig.cellCounting.bubbleRejectionImgAreaPercentage;
	configParams[ConfigParameters::eCellSpotBrightnessExclusionThreshold] = hawkeyeConfig.cellCounting.cellSpotBrightnessExclusionThreshold;
	configParams[ConfigParameters::eCenterSpotMinIntensityLimit] = hawkeyeConfig.cellCounting.centerSpotMinimumIntensityLimit;

	double concentrationIntercept;
	double concentrationSlope;
	double sizeIntercept;
	double sizeSlope;
	uint32_t imageControlCount;
	CalibrationHistoryDLL::Instance().GetCurrentConcentrationAndSizeCalibration (
		cellProcessingParameters.calType,
		concentrationIntercept, 
		concentrationSlope, 
		imageControlCount,
		sizeIntercept, 
		sizeSlope);

	configParams[ConfigParameters::eConcentrationIntercept] = concentrationIntercept;
	configParams[ConfigParameters::eConcentrationSlope] = concentrationSlope;
	configParams[ConfigParameters::eConcentrationImageControlCount] = imageControlCount;

	// Set the rest of the ancillary CellCounting configuration parameters that are
	// ulltimately sent to *ImageAnalysisUtilities::SetCellCountingConfiguration*.
	if (celltype.decluster_setting == eDCNone)
	{
		// Turn decluster OFF.
		configParams[ConfigParameters::eDeclusterMode] = 0;
		configParams[ConfigParameters::eDeclusterAccumulatorThreshold] = 20;
		configParams[ConfigParameters::eDeclusterMinDistanceThreshold] = 20;
	}
	else
	{
		// Turn decluster ON.
		configParams[ConfigParameters::eDeclusterMode] = 1;
		configParams[ConfigParameters::eDeclusterAccumulatorThreshold] = hawkeyeConfig.cellCounting.declusterSettings[(uint32_t)celltype.decluster_setting].accumulatorThreshold;
		configParams[ConfigParameters::eDeclusterMinDistanceThreshold] = hawkeyeConfig.cellCounting.declusterSettings[(uint32_t)celltype.decluster_setting].minimumDistanceThreshold;
	}

	configParams[ConfigParameters::eDilutionFactor] = cellProcessingParameters.dilutionFactor;
	configParams[ConfigParameters::eHotPixelEliminationMode] = hawkeyeConfig.cellCounting.hotPixelEliminationMode;
	configParams[ConfigParameters::eNominalBackgroundLevel] = hawkeyeConfig.cellCounting.nominalBackgroundLevel;
	configParams[ConfigParameters::ePeakIntensitySelectionAreaLimit] = hawkeyeConfig.cellCounting.peakIntensitySelectionAreaLimit;
	configParams[ConfigParameters::eROIXCoOrdinate] = hawkeyeConfig.cellCounting.roiXCoordinate;
	configParams[ConfigParameters::eROIYCoOrdinate] = hawkeyeConfig.cellCounting.roiYCoordinate;
	configParams[ConfigParameters::eSizingIntercept] = sizeIntercept;
	configParams[ConfigParameters::eSizingSlope] = sizeSlope;
	configParams[ConfigParameters::eSmallParticleSizingCorrection] = hawkeyeConfig.cellCounting.smallParticleSizingCorrection;  // Default value from Julie Lutti 2018/07/02 for 4.2.16 algorithm code.
	configParams[ConfigParameters::eSubPeakAnalysisMode] = hawkeyeConfig.cellCounting.subpeakAnalysisMode;
	
	return true;
}

//*****************************************************************************
bool ImageAnalysisUtilities::SetCellCountingParameters(
	const AnalysisDefinitionDLL& analysisDef,
	const CellTypeDLL& cellType,
	CellProcessingParameters_t& cellProcessingParameters)
{
	if (!pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "setCellCountingParameters: module is not initialized...");
		return false;
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "module is busy performing other tasks!");
		return false;
	}

	// Set more of the ancillary CellCounting configuration parameters.
	v_IdentificationParams_t populationOfInterest;
	v_IdentificationParams_t generalPopulation;

	std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> configParams;
	initalizeCellCountingConfiguration (
		analysisDef, 
		cellType,
		cellProcessingParameters,
		populationOfInterest,
		generalPopulation,
		configParams);
	SetCellCountingConfiguration (configParams);
	SetCellIdentificationParameters (populationOfInterest, generalPopulation);

	return true;
}

//*****************************************************************************
void ImageAnalysisUtilities::SetCellIdentificationParameters(
	v_IdentificationParams_t populationOfInterest,
	v_IdentificationParams_t generalPopulation)
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);

	v_IdentifiParamsErrorCode paramErrorCodes = pCellCounter_->SetCellIdentificationParams (generalPopulation, true);
	if (!paramErrorCodes.empty())
	{
		std::string ss = "*** General Population Parameter Errors ***";
		for (auto v : paramErrorCodes)
		{
			if (v.second != eSuccess)
			{
				auto ch = std::get<0>(v.first);
				ss += boost::str(boost::format("\nParameter: <%d, %d, %d>, Error: %d")
					% std::get<0>(v.first)
					% std::get<1>(v.first)
					% std::get<2>(v.first)
					% (uint32_t)v.second);
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::sample_cellcounting_gpinvalid,
					instrument_error::severity_level::error));
			}
		}
		Logger::L().Log (MODULENAME, severity_level::debug1, ss);
	}

	paramErrorCodes = pCellCounter_->SetPOIIdentificationParams (populationOfInterest, true);
	if (!paramErrorCodes.empty())
	{
		std::string ss = "*** Population Of Interest Parameter Errors ***";
		for (auto v : paramErrorCodes)
		{
			if (v.second != eSuccess)
			{
				auto ch = std::get<0>(v.first);
				ss += boost::str(boost::format("\nParameter: <%d, %d, %d>, Error: %d")
					% std::get<0>(v.first)
					% std::get<1>(v.first)
					% std::get<2>(v.first)
					% static_cast<uint32_t>(v.second));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::sample_cellcounting_poiinvalid,
					instrument_error::severity_level::error));
			}
		}
		Logger::L().Log (MODULENAME, severity_level::debug1, ss);
	}
}

//*****************************************************************************
bool ImageAnalysisUtilities::clearResult()
{
	if (!pCellCounter_)
	{
		Logger::L ().Log (MODULENAME, severity_level::normal, "clearResult: module is not initialized");
		return false;
	}

	pCellCounter_->ClearResult();
	return true;
}

//*****************************************************************************
// "startProcess" must be called before "addImages" as "startProcess" calls "CCellCounter::IsProcessCompleted"
// which does a comparison between the # of images to process and the # of images processed to determine
// whether the image processing is busy.  If the order is swapped then "startProcess" never starts the 
// image processing.
//*****************************************************************************
void ImageAnalysisUtilities::startProcess()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "startProcess: <enter>");

	if (!pCellCounter_)
	{
		Logger::L ().Log (MODULENAME, severity_level::normal, "startProcess: <exit, module is not initialized>");
		return;
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "startProcess: <exit, module is busy performing other tasks>");
		return;
	}

	pCellCounter_->StartProcess();

	Logger::L().Log (MODULENAME, severity_level::debug1, "startProcess: <exit>");
}

//*****************************************************************************
void ImageAnalysisUtilities::stopProcess()
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "stopProcess: <enter>");

	if (!pCellCounter_)
	{
		Logger::L ().Log (MODULENAME, severity_level::normal, "stopProcess: module is not initialized...");
		return;
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "stopProcess - aborting existing work");
	}

	pCellCounter_->StopProcess();

	Logger::L ().Log (MODULENAME, severity_level::debug1, "stopProcess: <exit>");
}

//*****************************************************************************
bool ImageAnalysisUtilities::addImage(const ImageSet_t& imageSet)
{
	if (!pCellCounter_)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "addImage: module is not initialized...");
		return false;
	}

	const E_ERRORCODE errorCode = pCellCounter_->AddImage(imageSet);
	if (errorCode == E_ERRORCODE::eSuccess)
	{
		return true;
	}
	if (errorCode == E_ERRORCODE::eInvalidImage)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "addImage: invalid images");
	}
	else if (errorCode == E_ERRORCODE::eZeroInput)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "addImage: no images found");
	}

	return false;
}

//*****************************************************************************
bool ImageAnalysisUtilities::addImages (const ImageCollection_t& imageSets)
{
	if (!pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "addImages: module is not initialized...");
		return false;
	}

	const E_ERRORCODE errorCode = pCellCounter_->AddImages (imageSets);
	if (errorCode == E_ERRORCODE::eSuccess)
	{
		return true;
	}
	if (errorCode == E_ERRORCODE::eInvalidImage)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "addImages: invalid images");
	}
	else if (errorCode == E_ERRORCODE::eZeroInput)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "addImages: no images found");
	}

	return false;
}

//*****************************************************************************
bool ImageAnalysisUtilities::isProcessCompleted()
{
	if (!pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "isProcessCompleted: module is not initialized...");
		return true;
	}

	return pCellCounter_->IsProcessCompleted();
}

//*****************************************************************************
// Currently, this is only called from "TemporaryImageAnalysis::generateResultsContinuous.
//*****************************************************************************
bool ImageAnalysisUtilities::isBusy()
{
	return !isProcessCompleted();
}

//*****************************************************************************
void ImageAnalysisUtilities::getCellIdentificationParams (const CellTypeDLL& cellType, v_IdentificationParams_t& generalPopulation) {

	generalPopulation.clear();

	// Create tuples for *minimum_diameter_um, maximum_diameter_um, mimimum_circularity, sharpness_limit" and
	// add these to the general population settings.
	Characteristic_t ct = std::make_tuple (8, 0, 0);	// minimum_diameter and maximum_diameter.
	generalPopulation.push_back (std::make_tuple (ct, cellType.minimum_diameter_um, E_POLARITY::eATABOVE));
	generalPopulation.push_back (std::make_tuple (ct, cellType.maximum_diameter_um, E_POLARITY::eBELOW));
	ct = std::make_tuple (9, 0, 0);						// mimimum_circularity.
	generalPopulation.push_back (std::make_tuple (ct, cellType.minimum_circularity, E_POLARITY::eATABOVE));
	ct = std::make_tuple (10, 0, 0);					// sharpness_limit.
	generalPopulation.push_back (std::make_tuple (ct, cellType.sharpness_limit, E_POLARITY::eATABOVE));

	// Now, do any additional analysis parameters.
	for (auto cip : cellType.cell_identification_parameters)
	{
		ct = std::make_tuple(
			cip.characteristic.key,
			cip.characteristic.s_key,
			cip.characteristic.s_s_key);

		generalPopulation.push_back(std::make_tuple(
			ct, cip.threshold_value,
			(cip.above_threshold == 0 ? E_POLARITY::eBELOW : E_POLARITY::eATABOVE)));
	}

	std::stringstream ss;
	ss << std::endl << "\t*** Cell Identification Parameters ***" << std::endl;

	for (auto gp : generalPopulation) {
		auto ch = std::get<0>(gp);
		ss << boost::str(boost::format("\t<%d, %d, %d> threshold: %d, %s")
						 % std::get<0>(ch)
						 % std::get<1>(ch)
						 % std::get<2>(ch)
						 % std::get<1>(gp)
						 % ( std::get<2>(gp) == 0 ? "below" : "above" )) << std::endl;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());
}

//*****************************************************************************
void ImageAnalysisUtilities::getPOIIdentificationParams (const AnalysisDefinitionDLL& ad, v_IdentificationParams_t& populationOfInterest)
{
	populationOfInterest.clear();

	for (auto ap : ad.analysis_parameters)
	{
		Characteristic_t ct = std::make_tuple(
			ap.characteristic.key,
			ap.characteristic.s_key,
			ap.characteristic.s_s_key);

		//Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("<%d, %d, %d> threshold: %d, %s")
		//	% ap.characteristic.key
		//	% ap.characteristic.s_key
		//	% ap.characteristic.s_s_key
		//	% ap.above_threshold
		//	% (ap.above_threshold == 0 ? "below" : "above")
		//));

		populationOfInterest.push_back(std::make_tuple(
			ct, ap.threshold_value,
			(ap.above_threshold == 0 ? E_POLARITY::eBELOW : E_POLARITY::eATABOVE)));
	}

	std::stringstream ss;
	ss << std::endl << "\t*** POI Identification Parameters ***" << std::endl;

	for (auto gp : populationOfInterest) {
		auto ch = std::get<0>(gp);
		ss << boost::str(boost::format("\t<%d, %d, %d> threshold: %d, %s")
						 % std::get<0>(ch)
						 % std::get<1>(ch)
						 % std::get<2>(ch)
						 % std::get<1>(gp)
						 % ( std::get<2>(gp) == 0 ? "below" : "above" )) << std::endl;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());
}

//*****************************************************************************
void ImageAnalysisUtilities::setBrightfieldDustReferenceImage (const ImageSet_t& imageSet)
{
	if (imageSet.first.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "setBrightfieldDustReferenceImage: <exit, no dust subtract image to set>");
		return;
	}
	
	if (!pCellCounter_)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setBrightfieldDustReferenceImage: <exit, invalid cellcounter instance>");
		return;
	}

	const auto errorCode = pCellCounter_->SetDustImage (imageSet.first);
	if (errorCode != E_ERRORCODE::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, 
			boost::str (boost::format ("setBrightfieldDustReferenceImage: <exit, failed to set dust image, error code: %d") % errorCode));
	}

	dustReferenceImageSet_ = imageSet;
}

//*****************************************************************************
bool ImageAnalysisUtilities::writeFCSData (std::string outputBaseDir, const CellCounterResult::SResult& const_r_stCumulativeOutput)
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);
	//API function to create & write the FCS version 3.0 data to specified path from the user

	E_ERRORCODE errorCode = pCellCounter_->WriteFCSData ("ScoutFCSFile", outputBaseDir, const_r_stCumulativeOutput);
	if (errorCode == E_ERRORCODE::eSuccess)
		return true;
	else
		return false;
}

//*****************************************************************************
void ImageAnalysisUtilities::SetConfigurationParameters (const ConfigParamList_t& paramList)
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);
	pCellCounter_->SetConfigurationParameters (paramList);
}

//*****************************************************************************
void ImageAnalysisUtilities::GetSResult (CellCounterResult::SResult& sresult)
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);
	pCellCounter_->GetCumulativeResult (sresult);
}

//*****************************************************************************
void ImageAnalysisUtilities::ApplyConcentrationAdjustmentFactor (const CellTypeDLL& cellType, CellCounterResult::SResult& sresult)
{
	const double concentration_adjustment_factor = static_cast<double>(cellType.concentration_adjustment_factor / 100.0);

	sresult.Cumulative_results.dCellConcentration_GP  += (sresult.Cumulative_results.dCellConcentration_GP * concentration_adjustment_factor);
	sresult.Cumulative_results.dCellConcentration_POI += sresult.Cumulative_results.dCellConcentration_POI * concentration_adjustment_factor;

	for (auto& v : sresult.map_Image_Results)
	{
		v.second.dCellConcentration_GP  += v.second.dCellConcentration_GP  * concentration_adjustment_factor;
		v.second.dCellConcentration_POI += v.second.dCellConcentration_POI * concentration_adjustment_factor;
	}
}

//*****************************************************************************
std::string ImageAnalysisUtilities::GetCharacteristicLabel (Characteristic_t key)
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);
	return pCellCounter_->GetCharacteristicLabel (key);
}

//*****************************************************************************
std::map<Characteristic_t, std::string> ImageAnalysisUtilities::GetAllBlobCharacteristics()
{
	HAWKEYE_ASSERT (MODULENAME, pCellCounter_);
	return pCellCounter_->GetAllBlobCharacteristics();
}

//*****************************************************************************
ImageSet_t& ImageAnalysisUtilities::GetDustReferenceImageSet()
{
	return dustReferenceImageSet_;
}

#endif

//*****************************************************************************
void ImageAnalysisUtilities::getBasicResultAnswers (const CellCounterResult::SImageStats& input, BasicResultAnswers& output)
{
	output = {};

	output.eProcessedStatus = input.eProcessStatus;
	output.nTotalCumulative_Imgs = input.nTotalCumulative_Imgs;

	output.count_pop_general = input.nTotalCellCount_GP;
	output.count_pop_ofinterest = input.nTotalCellCount_POI;
	output.concentration_general = static_cast<float>(input.dCellConcentration_GP);
	output.concentration_ofinterest = static_cast<float>(input.dCellConcentration_POI);
	output.percent_pop_ofinterest = static_cast<float>(input.dPopulationPercentage);
	output.avg_diameter_pop = static_cast<float>(input.dAvgDiameter_GP);
	output.avg_diameter_ofinterest = static_cast<float>(input.dAvgDiameter_POI);
	output.avg_circularity_pop = static_cast<float>(input.dAvgCircularity_GP);
	output.avg_circularity_ofinterest = static_cast<float>(input.dAvgCircularity_POI);

//TODO: calculate CV...
output.coefficient_variance = 0;
//TODO: it has been decided that the UI will not display CVs.

	// Prevent divide by zero error.
	if (input.nTotalCumulative_Imgs > 0) {
		output.average_cells_per_image = input.nTotalCellCount_GP / input.nTotalCumulative_Imgs;
	}
	else
	{
		output.average_cells_per_image = 0;
	}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "input.nTotalCumulative_Imgs: " + std::to_string(input.nTotalCumulative_Imgs));

	output.average_brightfield_bg_intensity = static_cast<uint16_t>(input.dAvgBackgroundIntensity);
	output.bubble_count = static_cast<uint16_t>(input.nTotalBubbleCount);
	output.large_cluster_count = static_cast<uint16_t>(input.nLargeClusterCount);
}

