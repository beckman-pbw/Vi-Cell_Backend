#include "stdafx.h"

#include "ImageAnalysisParameters.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "ImageAnalysisParameters";

//*****************************************************************************
bool ImageAnalysisParameters::Initialize()
{
	// Currently, there is only one image analysis record.
	std::vector<DBApi::DB_ImageAnalysisParamRecord> imageAnalysisParamList = {};

	DBApi::eQueryResult dbStatus = DBApi::DbGetImageAnalysisParameterList(
		imageAnalysisParamList,
		DBApi::eListFilterCriteria::NoFilter,		// Filter type.
		"",											// Filter compare operator: '=' '<' '>' etc.
		"",											// Filter target value for comparison: true false value etc
		-1,											// Limit count.
		DBApi::eListSortCriteria::SortNotDefined,	// primary Sort type.
		DBApi::eListSortCriteria::SortNotDefined,	// secondary Sort type.
		-1,											// Sort direction, descending.
		"",											// Order string.
		-1,											// Start index.
		-1);										// Start ID num.

	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			// Create the default CellCounting configuration.
			DBApi::DB_ImageAnalysisParamRecord imageAnalysisParams = {};
			imageAnalysisParams.AlgorithmMode = 1;
			imageAnalysisParams.BkgdIntensityTolerance = 8;
			imageAnalysisParams.BubbleMinSpotAreaPercent = 5;
			imageAnalysisParams.BubbleMinSpotAvgBrightness = 30;
			imageAnalysisParams.BubbleMode = 1;
			imageAnalysisParams.BubbleRejectImageAreaPercent = 30;
			imageAnalysisParams.CellSpotBrightnessExclusionThreshold = 1;
			imageAnalysisParams.CenterSpotMinIntensityLimit = 50;
			imageAnalysisParams.ConcImageControlCount = 100;
			imageAnalysisParams.DeclusterAccumulatorThreshLow = 20;
			imageAnalysisParams.DeclusterMinDistanceThreshLow = 22;
			imageAnalysisParams.DeclusterAccumulatorThreshMed = 16;
			imageAnalysisParams.DeclusterMinDistanceThreshMed = 18;
			imageAnalysisParams.DeclusterAccumulatorThreshHigh = 12;
			imageAnalysisParams.DeclusterMinDistanceThreshHigh = 15;
			imageAnalysisParams.FovDepthMM = 0.086;
			imageAnalysisParams.HotPixelEliminationMode = 0;
			imageAnalysisParams.NominalBkgdLevel = 53;
			imageAnalysisParams.PeakIntensitySelectionAreaLimit = 50;
			imageAnalysisParams.PixelFovMM = 0.00048;
			imageAnalysisParams.ROI_Xcoords = 60;
			imageAnalysisParams.ROI_Ycoords = 60;
			imageAnalysisParams.SmallParticleSizingCorrection = 19;
			imageAnalysisParams.SubPeakAnalysisMode = 0;

			DBApi::eQueryResult dbStatus = DBApi::DbAddImageAnalysisParameter (imageAnalysisParams);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("Initialize: <exit, DbGetImageAnalysisParameterList failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_writeerror,
					instrument_error::instrument_storage_instance::analysis,
					instrument_error::severity_level::error));
				return false;
			}

			imageAnalysisParamList.push_back (imageAnalysisParams);
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbGetImageAnalysisParameterList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::analysis,
				instrument_error::severity_level::error));
			return false;
		}
	}

	imageAnalysisParams_ = imageAnalysisParamList[0];

	return true;
}

//*****************************************************************************
DBApi::DB_ImageAnalysisParamRecord& ImageAnalysisParameters::Get()
{
	return imageAnalysisParams_;
}

//*****************************************************************************
bool ImageAnalysisParameters::Set()
{
	DBApi::eQueryResult dbStatus = DBApi::DbModifyImageAnalysisParameter (imageAnalysisParams_);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		if (dbStatus != DBApi::eQueryResult::InsertObjectExists)
		{
			Logger::L().Log(MODULENAME, severity_level::error, "Update: <exit, DbModifyImageAnalysisParameter failed>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::instrument_configuration,
				instrument_error::severity_level::warning));
			return false;
		}
		Logger::L().Log (MODULENAME, severity_level::warning, "Set: <ImageAnalysisParameter entry is already set>");
	}

	return true;
}

