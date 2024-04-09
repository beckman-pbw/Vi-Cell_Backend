#include "stdafx.h"

#include "CellCounterFactory.h"
#include "HawkeyeDirectory.hpp"
#include "ImageProcessingUtilities.hpp"
#include "Logger.hpp"
#include "SResultData.hpp"

static const char MODULENAME[] = "ImageProcessingUtilities";

static std::shared_ptr<boost::asio::io_context> pImgProcessingIoSvc_;
static std::shared_ptr<boost::asio::deadline_timer> p_ReanalysisTimer;

//*****************************************************************************
//TODO: this needs to be called...
void ImageProcessingUtilities::initialize (std::shared_ptr<boost::asio::io_context> pImgProcessingIoSvc)
{
	pImgProcessingIoSvc_ = pImgProcessingIoSvc;
}

//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::generateDustImage (std::vector<cv::Mat> v_oMatInputImgs, cv::Mat& dustImage)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([v_oMatInputImgs, index, &dustImage]() -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateDustImage: <processing>");
		dustImage = CellCounter::CellCounterFactory::GenerateDustImage (v_oMatInputImgs);
		Logger::L().Log (MODULENAME, severity_level::debug1, "generateDustImage: <complete>");

		auto he = dustImage.empty() ? HawkeyeError::eInvalidArgs : HawkeyeError::eSuccess;
		CompletionHandler::getInstance(index).onComplete(he);
	});	

	return *handler;
}

//*****************************************************************************
// This is called when the decluster setting of the new celltype is the same as
// the decluster setting of the original celltype.
//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::CellCounterReanalysis(
	const AnalysisDefinitionDLL& analysisDefinition,
	const CellTypeDLL& cellType,
	const CellCounterResult::SResult& orgSresult,
	CellCounterResult::SResult& reanalysisOutput)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([=, &reanalysisOutput]() -> void
	{
		ImageAnalysisUtilities::reset();
			
		v_IdentificationParams_t generalPopulation = {};
		ImageAnalysisUtilities::getCellIdentificationParams(cellType, generalPopulation);

		v_IdentificationParams_t populationOfInterest = {};
		ImageAnalysisUtilities::getPOIIdentificationParams(analysisDefinition, populationOfInterest);

		v_IdentifiParamsErrorCode gpErrorCode = {};
		v_IdentifiParamsErrorCode poiErrorCode = {};

		reanalysisOutput = CellCounter::CellCounterFactory::CellCounterReAnalysis(
			orgSresult,
			generalPopulation, 
			populationOfInterest,
			gpErrorCode, 
			poiErrorCode);

		CompletionHandler::getInstance(index).onComplete(HawkeyeError::eSuccess);
	});

	return *handler;
}

//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::CellCounterReanalysis(
	std::shared_ptr<ImageCollection_t> image_collection,
	uint16_t dilution_factor,
	const ImageSet_t& background_normalization_image,
	const AnalysisDefinitionDLL& analysisDef,
	const CellTypeDLL& cellType,
	const CellCounterResult::SResult& orgSresult,
	CellCounterResult::SResult& reanalysisSResult)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([=, &reanalysisSResult]() -> void
	{
		ImageAnalysisUtilities::reset();

		// Set the ancillary CellCounting configuration parameters from previous sample settings.
		CellProcessingParameters_t cellProcessingParameters = {};
		v_IdentificationParams_t populationOfInterest = {};
		v_IdentificationParams_t generalPopulation = {};

		// Pick up new dilution factor.
		cellProcessingParameters.dilutionFactor = static_cast<uint16_t>(dilution_factor);

		std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> configParams;
		ImageAnalysisUtilities::initalizeCellCountingConfiguration (analysisDef, cellType, cellProcessingParameters, populationOfInterest, generalPopulation, configParams);

		// Set concentration parameters from original sample result record.
		// This overrides the values set in ImageAnalysisUtilities::initalizeCellCountingConfiguration.
		configParams[ConfigParameters::eConcentrationImageControlCount] = orgSresult.Processing_Settings.map_InputConfigParameters.at(ConfigParameters::eConcentrationImageControlCount);
		configParams[ConfigParameters::eConcentrationIntercept] = orgSresult.Processing_Settings.map_InputConfigParameters.at(ConfigParameters::eConcentrationIntercept);
		configParams[ConfigParameters::eConcentrationSlope] = orgSresult.Processing_Settings.map_InputConfigParameters.at(ConfigParameters::eConcentrationSlope);

		ImageAnalysisUtilities::SetCellCountingConfiguration (configParams);
		ImageAnalysisUtilities::SetCellIdentificationParameters (populationOfInterest, generalPopulation);

		if (!background_normalization_image.first.empty())
			ImageAnalysisUtilities::setBrightfieldDustReferenceImage (background_normalization_image);

		//NOTE: "startProcess" must be called before "addImages" as "startProcess" calls "CCellCounter::IsProcessCompleted"
		// which does a comparison between the # of images to process and the # of images processed to determine
		// whether the image processing is busy.  If the order is swapped then "startProcess" never starts the 
		// image processing.
		ImageAnalysisUtilities::startProcess();
		ImageAnalysisUtilities::addImages (*image_collection);

		if (!p_ReanalysisTimer)
		{
			p_ReanalysisTimer = std::make_shared<boost::asio::deadline_timer>(*pImgProcessingIoSvc_);
		}

		p_ReanalysisTimer->expires_from_now(boost::posix_time::milliseconds(250));
		p_ReanalysisTimer->async_wait([cellType, index, &reanalysisSResult](const boost::system::error_code& ec)->void
			{
				internal_cellCounterReanalysis (ec, cellType, reanalysisState::rs_Wait, index, reanalysisSResult);
			});
	});

	return *handler;
}

//*****************************************************************************
void ImageProcessingUtilities::internal_cellCounterReanalysis(
	const boost::system::error_code& ec, 
	const CellTypeDLL& cellType,
	reanalysisState state,
	uint64_t ch_Index, 	
	CellCounterResult::SResult& reanalysisOutput)
{
	if (ec && ec != boost::asio::error::operation_aborted)
	{
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("Internal error on cellCounterReanalysis: %s") % ec.message()));
		state = reanalysisState::rs_Error;
	}
	else if (ec && ec == boost::asio::error::operation_aborted)
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "cellCounterReanalysis cancelled");
		state = reanalysisState::rs_Cancelled;
	}

	if (!pImgProcessingIoSvc_)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "cellCounterReanalysis - no asynchronous processor instance!");
		state = reanalysisState::rs_Error;
	}

	switch (state)
	{
		case reanalysisState::rs_Wait:
		{
			if (ImageAnalysisUtilities::isProcessCompleted())
			{
				// this workflow does not require the user id, so do not use the transient user technique
				pImgProcessingIoSvc_->post ([cellType, ch_Index, &reanalysisOutput]()->void
					{
						ImageAnalysisUtilities::GetSResult (reanalysisOutput);
						internal_cellCounterReanalysis (boost::system::errc::make_error_code(boost::system::errc::success), cellType, reanalysisState::rs_Complete, ch_Index, reanalysisOutput);
					});
				break;
			}
				
			if (!p_ReanalysisTimer) {
				p_ReanalysisTimer = std::make_shared<boost::asio::deadline_timer>(*pImgProcessingIoSvc_);
			}

			p_ReanalysisTimer->expires_from_now (boost::posix_time::milliseconds(250));
			p_ReanalysisTimer->async_wait ([cellType, ch_Index, &reanalysisOutput](const boost::system::error_code& ec)->void
			{
				internal_cellCounterReanalysis (ec, cellType, reanalysisState::rs_Wait, ch_Index, reanalysisOutput);
			});
				
			break;
		}
		
		case reanalysisState::rs_Complete:
		{
			ImageAnalysisUtilities::stopProcess();
			CompletionHandler::getInstance(ch_Index).onComplete(HawkeyeError::eSuccess);
			break;
		}

		case reanalysisState::rs_Error:
		{
			ImageAnalysisUtilities::stopProcess();
			CompletionHandler::getInstance(ch_Index).onComplete(HawkeyeError::eSoftwareFault);
			break;
		}

		case reanalysisState::rs_Cancelled:
		{
			ImageAnalysisUtilities::stopProcess();
			CompletionHandler::getInstance(ch_Index).onComplete(HawkeyeError::eIdle);
			break;
		}
	}

}

//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::sharpness (const cv::Mat& inputImage, double& resultVariance)
{	
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([inputImage, index, &resultVariance]() -> void
	{
		// Any primitive type from the list can be defined by an identifier in the form CV_<bit-depth>{U|S|F}C(<number_of_channels>).
		cv::Mat imageL(inputImage.rows, inputImage.cols, CV_8U);

		// void Laplacian (InputArray src, OutputArray dst, int ddepth, int ksize = 1, double scale = 1, double delta = 0, int borderType = BORDER_DEFAULT);
		Laplacian(inputImage, imageL, CV_8U, 3, 1, 0, cv::BORDER_DEFAULT);

		cv::Scalar meanL;
		cv::Scalar stddevL;

		meanStdDev(imageL, meanL, stddevL);

		resultVariance = stddevL[0] * stddevL[0];

		//Logger::L().Log ( MODULENAME, severity_level::debug1,
		//                 boost::str( boost::format ("sharpness: %f") % resultVariance));

		CompletionHandler::getInstance(index).onComplete(HawkeyeError::eSuccess);		
	});
	
	return *handler;
}

#if 0 // Deprecated...  this is not currently used...  save for future use...
//*****************************************************************************
void ImageProcessingUtilities::showImage (std::string title, const CvArr * image)
{
	cvShowImage (title.c_str(), image);
	cv::waitKey (25); // Wait 25ms.
}
#endif

//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::generateHistogram (const cv::Mat& image, std::string histogramPath, bool& success)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([image, histogramPath, index, &success]() -> void
	{
		int histSize = 256;

		/// Set the ranges for grayscale.
		float range[] = { 0, 256 };
		const float* histRange = { range };
		bool uniform = true;
		bool accumulate = false;
		cv::Mat histogram;

		calcHist(&image, 1, nullptr, cv::Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate);

		std::ofstream histFile;
		histFile.open(histogramPath);
		if (!histFile.is_open())
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Failed to create histogram file: " + histogramPath);
			success = false;
			CompletionHandler::getInstance(index).onComplete(HawkeyeError::eStorageFault);
			return;
		}

		histFile << "Bin, Count" << std::endl;

		for (int i = 0; i < histSize; i++)
		{
			histFile << i << ", " << histogram.at<float>(i, 0) << std::endl;
		}

		histFile.close();

		success = true;
		CompletionHandler::getInstance(index).onComplete(HawkeyeError::eSuccess);
	});

	return *handler;	
}

//*****************************************************************************
// Returns the number of white (bin 255) pixels in the histogram.
//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::getHistogramWhiteCount (const cv::Mat& image, int32_t* whiteCount, bool& success)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([image, index, &whiteCount, &success]() -> void
	{
		*whiteCount = 0;
		if ( image.rows <= 0 || image.cols <= 0 )
		{
			success = false;
			return;
		}

		int histSize = 256;

		/// Set the ranges for grayscale.
		float range[] = { 0, 256 };
		const float* histRange = { range };
		bool uniform = true;
		bool accumulate = false;
		cv::Mat histogram;

		calcHist(&image, 1, nullptr, cv::Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate);

		int32_t tmpCnt = ( int32_t ) histogram.at<float>( 255, 0 );
		int32_t imgPixels = image.rows * image.cols;

		if ( tmpCnt >= imgPixels / 4 )	// a suspicious image
		{
			// not possible to have more than 1/2 of all pixels as whitecount in real operations
			if ( tmpCnt >= imgPixels / 2 )
			{
				tmpCnt = -1;			// try to retake picture if possible
			}
			else						// not a black image, but suspicious...
			{
				tmpCnt = -2;			// try to retake picture if possible
			}
			*whiteCount = tmpCnt;

			// NOTE: no calls to this implementation instance of 'getHistogramWhiteCount' were found.
			//		HOWEVER, I'm leaving the return through this execution path as 'false' (the failure indication) with the
			//		early return since I can't absolutely guarantee the handling of the whiteCount problem indicator values.
			success = false;
			return;
		}

		*whiteCount = tmpCnt;

		success = true;
		CompletionHandler::getInstance(index).onComplete(HawkeyeError::eSuccess);
	});

	return *handler;	
}

#if 0 // Deprecated...  this is not currently used...
//*****************************************************************************
struct ImageVarianceData
{
	double  lowerPercent;
	double  upperPercent;
	uint8_t blue;
	uint8_t green;
	uint8_t red;
};

#define MAX_VARIANCE_RANGES 7
static ImageVarianceData imageVariance[MAX_VARIANCE_RANGES] =
{
	{ 12.5,   100.0, 255,  15,   0 },
	{ 7.5,     12.5, 255, 241,   0 },
	{ 2.5,      7.5, 245, 245,   0 },
	{ -2.5,     2.5,   1, 254,   1 },
	{ -7.5,    -2.5,   1, 224, 254 },
	{ -12.5,   -7.5,   0, 110, 255 },
	{ -100.0, -12.5,   1,   1, 171 }
};

#define PERCENT_VARIANCE_STEP 0.01
//*****************************************************************************
ICompletionHandler& ImageProcessingUtilities::colorizeImage (const cv::Mat & grayImage, cv::Mat& colorizedImage, bool& success)
{
	auto handler = &CompletionHandler::createInstance();
	auto index = handler->getIndex();

	// this workflow does not require the user id, so do not use the transient user technique
	pImgProcessingIoSvc_->post([grayImage, index, &colorizedImage, &success]() -> void
	{
		// Compute the mean pixel value for all of the pixels.
		uint32_t totalPixelValue = 0;
		uint32_t totalNumPixels = (uint32_t)(grayImage.rows * grayImage.cols);
		for (int row = 0; row < grayImage.rows; row++)
		{
			for (int col = 0; col < grayImage.cols; col++)
			{
				UINT8 value = grayImage.at<uint8_t>(row, col);
				totalPixelValue += value;
			}
		}

		uint32_t mean = totalPixelValue / totalNumPixels;
		Logger::L().Log (MODULENAME, severity_level::debug1, "colorizeImage:: Grayscale Mean: " + std::to_string(mean));

		//Mat colorImage;
		cv::cvtColor(grayImage, colorizedImage, cv::COLOR_GRAY2BGR);

		for (int row = 0; row < grayImage.rows; row++)
		{
			for (int col = 0; col < grayImage.cols; col++)
			{

				uint8_t grayPixel = grayImage.at<uint8_t>(row, col);
				cv::Vec3b color = colorizedImage.at<cv::Vec3b>(row, col);

				for (int i = 0; i < 7; i++)
				{
					int16_t lowerVarianceValue = (int16_t)round((double)mean * (imageVariance[i].lowerPercent / 100.0));
					int16_t upperVarianceValue = (int16_t)round((double)mean * (imageVariance[i].upperPercent / 100.0));

#if 0
					Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("graypixel: %d") % (int)grayPixel));
					Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("mean - %f%% of mean: %d") % imageVariance[i].lowerPercent % lowerVarianceValue));
					Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("mean + %f%% of mean: %d") % imageVariance[i].upperPercent % upperVarianceValue));
#endif

					if (grayPixel > (mean + lowerVarianceValue) && grayPixel <= (mean + upperVarianceValue))
					{
						color.val[0] = imageVariance[i].blue;
						color.val[1] = imageVariance[i].green;
						color.val[2] = imageVariance[i].red;
					}
				}

				colorizedImage.at<cv::Vec3b>(row, col) = color;

			} // End "for (int col = 0; col < grayImage.cols; col++)"

		} // End "for (int row = 0; row < grayImage.rows; row++)"

		success = true;
		CompletionHandler::getInstance(index).onComplete(HawkeyeError::eSuccess);
	});

	return *handler;
}
#endif
