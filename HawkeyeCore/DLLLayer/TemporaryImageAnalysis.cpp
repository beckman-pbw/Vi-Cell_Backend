#include "stdafx.h"

#include "CalibrationCommon.hpp"
#include "CellCounterFactory.h"
#include "Hardware.hpp"
#include "TemporaryImageAnalysis.hpp"

static const char MODULENAME[] = "TemporaryImageAnalysis";

//*****************************************************************************
TemporaryImageAnalysis::TemporaryImageAnalysis (std::shared_ptr<boost::asio::io_context> pIoService)
	: pIoService_(pIoService)
{
	canContinueExecution_ = true;
	isBusy_ = false;

	exposureTime_usec_ = 50;
	ledType_ = HawkeyeConfig::LedType::LED_BrightField;
}

//*****************************************************************************
TemporaryImageAnalysis::~TemporaryImageAnalysis()
{
	canContinueExecution_ = false;
	isBusy_ = false;

	if (pIoService_)
	{
		pIoService_.reset();
	}	
}

//*****************************************************************************
void TemporaryImageAnalysis::generateResultsContinuous (
	eImageOutputType image_preference,
	std::shared_ptr<AnalysisDefinitionDLL> tempAnalysisDefinition,
	std::shared_ptr<CellTypeDLL> tempCellTypeDefinition,
	TemporaryImageAnalysis::temp_analysis_result_callback_DLL callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsContinuous: current instance is busy!");
		updateHost (callback, HawkeyeError::eBusy, BasicResultAnswers(), {});
		return;
	}

	if (ImageAnalysisUtilities::isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsContinuous: cell counting is currently busy!");
		updateHost (callback, HawkeyeError::eBusy, BasicResultAnswers(), cv::Mat());
		return;
	}

	if (tempAnalysisDefinition.get() == nullptr || tempCellTypeDefinition.get() == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsContinuous: Invalid/Empty temporary analysis/celltype definitions!");		
		updateHost (callback, HawkeyeError::eValidationFailed, BasicResultAnswers(), cv::Mat());
		return;
	}
	
	if (!canContinueExecution_.load())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsContinuous: Cannot continue execution after execution is stopped!");
		updateHost (callback, HawkeyeError::eNotPermittedAtThisTime, BasicResultAnswers(), cv::Mat());
		return;
	}

	CellCountingCallback localCallback = std::bind (&TemporaryImageAnalysis::onAnalysisComplete, this, image_preference, std::placeholders::_1, std::placeholders::_2, callback);

	ImageAnalysisUtilities::reset();
	ImageAnalysisUtilities::setCallback (localCallback);

	// Start setting the ancillary CellCounting configuration parameters that are
	// ultimately sent to *ImageAnalysisUtilities::SetCellCountingConfiguration*.
	CellProcessingParameters_t cellProcessingParameters = { };
	cellProcessingParameters.dilutionFactor = 1;
	cellProcessingParameters.calType = calibration_type::cal_Concentration; // Only use standard concentration slope here for the service function.

	bool success = ImageAnalysisUtilities::SetCellCountingParameters (*tempAnalysisDefinition, *tempCellTypeDefinition, cellProcessingParameters);
	if (!success)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsContinuous: unable to set cell counting paramters!");
		updateHost (callback, HawkeyeError::eBusy, BasicResultAnswers(), cv::Mat());
		return;
	}

	//NOTE: "startProcess" must be called before "addImages" as "startProcess" calls "CCellCounter::IsProcessCompleted"
	// which does a comparison between the # of images to process and the # of images processed to determine
	// whether the image processing is busy.  If the order is swapped then "startProcess" never starts the 
	// image processing.
	ImageAnalysisUtilities::startProcess();

	canContinueExecution_ = true;
	isBusy_ = true;

	generateResultsInternal();
}

//*****************************************************************************
void TemporaryImageAnalysis::generateResultsInternal()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: <Enter>");

	if (canContinueExecution_.load() == false)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: Cannot continue execution after execution is stopped!");
		return;
	}

	generateImage ([this]() -> void
	{
		if (singleImage_.size() <= 0)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: Invalid image parameters!");
			return;
		}

		if (!singleImage_.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "generateResultsInternal: Unknown error - previous data will get erased!");
		}

		if (!canContinueExecution_.load())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: generateImage : Cannot continue execution after execution is stopped!");
			return;
		}

		if (!ImageAnalysisUtilities::clearResult())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: generateImage : Unable to clear analysis results!");
		}

		ImageAnalysisUtilities::addImages (singleImage_);

		Logger::L().Log (MODULENAME, severity_level::debug1, "generateResultsInternal: <exit>");
	});
}

//*****************************************************************************
void TemporaryImageAnalysis::generateImage (std::function<void()> image_cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "generateImage: <enter>");

	if (image_cb == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateImage: Invalid input parameters!");
		return;
	}

	// this workflow does not require the user id, so do not use the transient user technique
	pIoService_->post ([this, image_cb]() -> void
	{
		if (!canContinueExecution_.load())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "generateImage: Cannot continue execution after execution is stopped!");
			return;
		}

		auto onTakePicture = [this, image_cb](cv::Mat image) -> void
		{
			// this workflow does not require the user id, so do not use the transient user technique
			pIoService_->post (std::bind(&TemporaryImageAnalysis::onCameraTrigger, this, image, image_cb));
			Logger::L ().Log (MODULENAME, severity_level::debug1, "generateImage : <Exit>");
		};
		
		Hardware::Instance().getCamera()->takePicture (exposureTime_usec_, ledType_, onTakePicture);
	});
}

//*****************************************************************************
void TemporaryImageAnalysis::onCameraTrigger (cv::Mat capturedImage, std::function<void()> trigger_cb)
{
	if (capturedImage.empty()) {
		Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraTrigger: <exit, error getting image");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_noimage, 
			instrument_error::severity_level::warning));
		return;
	}

	cv::Mat localImage = capturedImage;
	if (!localImage.empty())
	{
		ImageSet_t imageSet = std::make_pair (localImage, FLImages_t());
		singleImage_.clear ();
		singleImage_.push_back (imageSet);
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "onCameraTrigger: invalid/empty image captured!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_noimage, 
			instrument_error::severity_level::warning));
		return;
	}

	// this workflow does not require the user id, so do not use the transient user technique
	pIoService_->post ([trigger_cb]() -> void
	{
		if (trigger_cb != nullptr)
		{
			trigger_cb();
		}
	});
}

//*****************************************************************************
void TemporaryImageAnalysis::onAnalysisComplete (
	eImageOutputType image_preference,
	int imageNum, 
	CellCounterResult::SResult oSCellCountingResult,
	temp_analysis_result_callback_DLL callback)
{	
	Logger::L().Log (MODULENAME, severity_level::debug1, "onAnalysisComplete: <enter> with image number : " + std::to_string(imageNum));
	BasicResultAnswers result = { };
	ImageAnalysisUtilities::getBasicResultAnswers (oSCellCountingResult.map_Image_Results[imageNum], result);
	
	cv::Mat image = { };
	if (!singleImage_.empty())
	{
		image = singleImage_.front().first;
		singleImage_.clear();
	}

	// Transform image according to preferences.
	cv::Mat transformImage;

	if (!ImageWrapperUtilities::TransformImage (image_preference, image, transformImage, 127, oSCellCountingResult, imageNum))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onAnalysisComplete: image transformation failed!");
		image.copyTo (transformImage);
	}
	
	updateHost (callback, HawkeyeError::eSuccess, result, transformImage);
	
	// this workflow does not require the user id, so do not use the transient user technique
	pIoService_->post([this]() -> void
	{
		generateResultsInternal();
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "onAnalysisComplete: <exit>!");
}

//*****************************************************************************
void TemporaryImageAnalysis::stopContinuousExecution ()
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "stopContinuousExecution: Stopping temporary analysis!");

	canContinueExecution_ = false;
	isBusy_ = false;

	// Explicitly stop the CellCounting process to ensure that it is really stopped.
	// "ImageAnalysisUtilities::reset" stops the CellCounting process as a side-effect.
	// Being explicit is clearer.
	ImageAnalysisUtilities::stopProcess();
}

//*****************************************************************************
bool TemporaryImageAnalysis::isBusy () const
{
	return isBusy_.load ();
}

//*****************************************************************************
void TemporaryImageAnalysis::setCameraParams (uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType)
{
	exposureTime_usec_ = exposureTime_usec;
	ledType_ = ledType;
}

//*****************************************************************************
void TemporaryImageAnalysis::updateHost (temp_analysis_result_callback_DLL cb, HawkeyeError he, BasicResultAnswers result, cv::Mat image)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "updateHost: <enter>!");	

	if (he == HawkeyeError::eSuccess && image.empty())
	{
		he = HawkeyeError::eInvalidArgs;
	}

	// this workflow does not require the user id, so do not use the transient user technique
	pIoService_->post ([he, result, image, cb]() -> void
	{
		if (cb)
		{
			cb(he, result, ImageWrapperDLL{ image });
		}
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "updateHost: <exit>!");
}
