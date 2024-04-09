#include "stdafx.h"

#include "CameraAutofocus.hpp"
#include "FileSystemUtilities.hpp"
#include "Hardware.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "ImageWrapperUtilities.hpp"

static const char MODULENAME[] = "CameraAutofocus";

static const int32_t MaxAfImageRetries = 5;

double OffsetFromFlowCellDepth (double flowCellDepth);

CameraAutofocus::CameraAutofocus(CAFParams params)
	: params_(params)
{
	isBusy_ = false;
	imageCnt_ = 0;
	result_.reset();
	fgMaskMOG_ = cv::Mat();
	finalFocusedImage_ = cv::Mat();
	isCancelled_ = false;
}

CameraAutofocus::~CameraAutofocus()
{
	reset();
	pMOG_.reset();
}

//*****************************************************************************
// Returns the number of white (bin 255) pixels in the histogram.
//*****************************************************************************
bool CameraAutofocus::getHistogramWhiteCount (cv::Mat& image, int32_t* whiteCount)
{
	*whiteCount = 0;
	if ( image.rows <= 0 || image.cols <= 0 )
	{
		return false;
	}

	int histSize = 256;

	/// Set the ranges for grayscale.
	float range[] = { 0, 256 };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;
	cv::Mat histogram;

	calcHist( &image, 1, nullptr, cv::Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate );

	int32_t tmpCnt = ( int32_t ) histogram.at<float>( 255, 0 );
	int32_t imgPixels = image.rows * image.cols;

	// not possible to have more than 1/2 of all pixels as whitecount in real operations
	if ( tmpCnt >= imgPixels / 2 )
	{
		tmpCnt = -1;	// try to retake picture if possible
	}
	else if ( tmpCnt >= imgPixels / 4 )		// not a black image, but suspicious...
	{
		tmpCnt = -2;	// try to retake picture if possible
	}

	*whiteCount = tmpCnt;
	return true;
}

void CameraAutofocus::execute(std::function<void(bool)> onCompleteCallback)
{
	HAWKEYE_ASSERT (MODULENAME, onCompleteCallback);

	Logger::L().Log ( MODULENAME, severity_level::debug1, "\n\t*************************************"
	                                                     "\n\t************* autofocus *************"
	                                                     "\n\t*************************************" );

    if ( isBusy() )
    {
        Logger::L().Log( MODULENAME, severity_level::error, "Failed to execute AutoFocus : current instance is busy performing other task!" );
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pUpstreamIos->post( std::bind( onCompleteCallback, false ) );
        return;
    }

    isBusy_ = true;

	if (!FileSystemUtilities::CreateDirectories (HawkeyeDirectory::Instance().getAutoFocusImageDir(), true)) {
		Logger::L().Log (MODULENAME, severity_level::error, "Autofocus directory creation failed");
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pUpstreamIos->post(std::bind(onCompleteCallback, false));
		return;
	}

    executeInternal( ExecuteInternalSteps::eEntryPoint, [ this, onCompleteCallback ]( bool success )
    {
        result_ = std::move( generateResult( success, maxFocusData_, sharpnessData_ ) );

        isBusy_ = false;
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pUpstreamIos->post( std::bind( onCompleteCallback, success ) );
    });
}

void CameraAutofocus::cancelExecute()
{
	// Cancels the execute operation as soon as possible if(isBusy())
	// After cancellation the execute callback will be triggered with "true" status
	if (!isBusy())
	{
		return;
	}

	isCancelled_ = true;
}

void CameraAutofocus::executeInternal(ExecuteInternalSteps currentStep, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto  executeNextStep = [this, callback](ExecuteInternalSteps nextStep) -> void
	{
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post([=]()
		{
			executeInternal(nextStep, callback);
		});
	};

	// Cancel auto focus operation
	if (isCancelled_.load())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "executeInternal : operation cancelled!");
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post(std::bind(callback, true));
		return;
	}

	switch (currentStep)
	{
		case ExecuteInternalSteps::eEntryPoint:
		{
			imageCnt_ = 0;
			sharpnessData_.clear();
			maxFocusData_ = { 0, 0, 0 };

			executeNextStep(ExecuteInternalSteps::eFocusHome);
			return;
		}

		case ExecuteInternalSteps::eFocusHome:
		{
			Hardware::Instance().getFocusController()->FocusHome(
				[this, executeNextStep](bool status)
			{
				if (!status || !Hardware::Instance().getFocusController()->IsHome())
				{
					Logger::L().Log (MODULENAME, severity_level::error, ("Failed to move focus motor to home!"));
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				executeNextStep(ExecuteInternalSteps::eFocusStartPos);
			});
			return;
		}

		case ExecuteInternalSteps::eFocusStartPos:
		{
			// Define a starting position based on the geometry of the hardware.
			const uint32_t coarseStartPosition = HawkeyeConfig::Instance().get().autoFocusCoarseStartingPosition;
			
			Hardware::Instance().getFocusController()->SetPosition(
				[this, executeNextStep, coarseStartPosition](bool status, int32_t currentFocusPos) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, ("Failed to move focus motor to position : " + std::to_string(coarseStartPosition)));
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("Moving to coarse starting focus position: tgtPos <%d>, currentPos <%d>") % coarseStartPosition % currentFocusPos));
				executeNextStep(ExecuteInternalSteps::eCoarseFocus);

			}, static_cast<int32_t>(coarseStartPosition));
			return;
		}

		case ExecuteInternalSteps::eCoarseFocus:
		{
			coarseFocus(
				CoarseFocusSteps::eEntryPoint, [=](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute AutoFocus : Coarse focus operation failed!");
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				executeNextStep(ExecuteInternalSteps::eFineFocus);
			});
			return;
		}

		case ExecuteInternalSteps::eFineFocus:
		{
			FocusRange_t focusRange = {};
			const uint32_t fineFocusRange = HawkeyeConfig::Instance().get().autoFocusFineRange;
			focusRange.start = maxFocusData_.position - fineFocusRange;
			focusRange.end = maxFocusData_.position + fineFocusRange;

			fineFocus(
				FineFocusSteps::eEntryPoint, focusRange, [=](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute AutoFocus : Fine focus operation failed!");
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				executeNextStep(ExecuteInternalSteps::eGetFocusImage);
			});
			return;
		}

		case ExecuteInternalSteps::eGetFocusImage:
		{
			Hardware::Instance().getFocusController()->SetPosition(
				[this, executeNextStep](bool status, int32_t currentFocusPos) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to capture final focus image at focus position : " + std::to_string(maxFocusData_.position));
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("Moving to optimal focus position: tgtPos <%d>, currentPos <%d>") % maxFocusData_.position % currentFocusPos));
				
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("autofocus:: capturing focused image at focus position: %d") % currentFocusPos));

				Hardware::Instance().getCamera()->takePicture(
					params_.exposureTime_usec, params_.ledType, [this, executeNextStep, currentFocusPos](cv::Mat image)
				{
					finalFocusedImage_.release();
					
					if (image.empty())
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Failed to capture final focus image at focus position : " + std::to_string(currentFocusPos));
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::imaging_camera_noimage, 
							instrument_error::severity_level::warning));
						executeNextStep(ExecuteInternalSteps::eError);
						return;
					}

					finalFocusedImage_ = image;
					
					if (boost::filesystem::exists(HawkeyeDirectory::Instance().getAutoFocusImageDir()))
					{
						saveAutofocusImage(finalFocusedImage_, HawkeyeDirectory::Instance().getBestFocusImagePath(Hardware::Instance().getFocusController()->Position()));
					}
					executeNextStep(ExecuteInternalSteps::eFocusOffset);
				});
			}, maxFocusData_.position);
			
			return;
		}

		case ExecuteInternalSteps::eFocusOffset:
		{
			int32_t offSet = static_cast<int32_t>(std::ceil(OffsetFromFlowCellDepth(HawkeyeConfig::Instance().get().flowCellDepth)));

			int32_t adjustedFocusPos = maxFocusData_.position - offSet;
			Hardware::Instance().getFocusController()->SetPosition(
				[this, executeNextStep, adjustedFocusPos](bool status, int32_t currentFocusPos) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to capture final focus image at focus position : " + std::to_string(adjustedFocusPos));
					executeNextStep(ExecuteInternalSteps::eError);
					return;
				}
				Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("Moving to adjusted optimal focus position: tgtPos <%d>, currentPos <%d>") % adjustedFocusPos % currentFocusPos));
				executeNextStep(ExecuteInternalSteps::eComplete);
			}, adjustedFocusPos);
			return;
		}

		case ExecuteInternalSteps::eComplete:
		{
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, true));
			return;
		}

		case ExecuteInternalSteps::eError:
		default:
		{
			maxFocusData_ = { 0, 0, 0 };
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, false));
			return;
		}
	}

	// Unreachable Code
	HAWKEYE_ASSERT (MODULENAME, false);
}

bool CameraAutofocus::isBusy() const
{
	return isBusy_.load();
}

std::shared_ptr<AutofocusResultsDLL> CameraAutofocus::getResult() const
{
	return result_;
}

void CameraAutofocus::reset()
{
	isBusy_ = false;

	fgMaskMOG_.release();
	result_.reset();
	sharpnessData_.clear();
	finalFocusedImage_.release();
}

void CameraAutofocus::coarseFocus(CoarseFocusSteps currentStep, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto  executeNextStep = [this, callback](CoarseFocusSteps nextStep) -> void
	{
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post([=]()
		{
			coarseFocus(nextStep, callback);
		});
	};

	// Check if operation has been cancelled
	if (isCancelled_.load())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "coarseFocus : operation cancelled!");
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post(std::bind(callback, true));
		return;
	}

	static int coarseImgRetries = 0;
	static uint32_t coarseEndPosition;
	switch (currentStep)
	{
		case CoarseFocusSteps::eEntryPoint:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Coarse Focus ##########");

			coarseImgRetries = 0;			// ensure the image retry count is reset at the start of each focus sweep
			maxFocusData_ = { 0, 0, 0 };
			coarseEndPosition = HawkeyeConfig::Instance().get().autoFocusCoarseEndingPosition;

			const uint32_t coarseStep = HawkeyeConfig::Instance().get().autoFocusCoarseStep;
			Hardware::Instance().getFocusController()->AdjustCoarseStep(coarseStep);

			executeNextStep(CoarseFocusSteps::eFocusStep);
			return;
		}

		case CoarseFocusSteps::eFocusStep:
		{
			Hardware::Instance().getFocusController()->FocusStepUpCoarse(
				[this, executeNextStep](bool status, int32_t endPos)
			{
				executeNextStep(status ? CoarseFocusSteps::eTakePicture : CoarseFocusSteps::eError);
			});
			coarseImgRetries = 0;			// reseet the image retry count for each focus step
			return;
		}

		case CoarseFocusSteps::eTakePicture:
		{
			takePictureAndUpdateFgMask(true, [=](bool status)
			{
				if (!status)
				{
					if ( ++coarseImgRetries >= MaxAfImageRetries )
					{
						Logger::L().Log ( MODULENAME, severity_level::error, "Failed to update foreground mask image!" );
						executeNextStep( CoarseFocusSteps::eError );
					}
					else
					{
						Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str( boost::format( "Failure updating foreground mask image! try: %d" ) % coarseImgRetries ) );
						executeNextStep( CoarseFocusSteps::eTakePicture );
					}
					return;
				}
				executeNextStep(CoarseFocusSteps::eCheckComplete);
			});
			return;
		}

		case CoarseFocusSteps::eCheckComplete:
		{
			int32_t sharpness = 0;
			if (!getHistogramWhiteCount(fgMaskMOG_, &sharpness))
			{
				if ( ++coarseImgRetries >= MaxAfImageRetries )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "Failed to get histogram white count of current image!" );
					executeNextStep( CoarseFocusSteps::eError );
				}
				else
				{
					Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str( boost::format( "Failure getting histogram white count of current image! try: %d" ) % coarseImgRetries ) );
					executeNextStep( CoarseFocusSteps::eTakePicture );
				}
				return;
			}

			if ( sharpness < 0 )
			{
				if ( ++coarseImgRetries >= MaxAfImageRetries )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "Failed to get histogram white count of current image!" );
					executeNextStep( CoarseFocusSteps::eError );
				}
				else
				{
					std::string logStr = "Current image invalid : ";
					if ( sharpness < -1 )	// unknown invalid image content
					{
						logStr.append( "unknown image error." );
					}
					else	// whitecount too high; appears to be black image
					{
						logStr.append( "black image." );
					}
					logStr.append( boost::str( boost::format( "  try: %d" ) % coarseImgRetries ) );
					Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
					executeNextStep( CoarseFocusSteps::eTakePicture );
				}
				return;
			}

			Logger::L().Log (
				MODULENAME, severity_level::debug1,
				boost::str(boost::format("autofocus:: sharpness: %f, position: %d, filenum: %d")
						   % sharpness
						   % Hardware::Instance().getFocusController()->Position()
						   % imageCnt_));

			if (sharpness > maxFocusData_.sharpness)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "*** sharpness > max sharpness ***");

				maxFocusData_.sharpness = sharpness;
				maxFocusData_.position = Hardware::Instance().getFocusController()->Position();
				maxFocusData_.filenum = imageCnt_;

				Logger::L().Log (
					MODULENAME, severity_level::debug1,
					boost::str(boost::format("autofocus:: MAX sharpness: %f, position: %d, FileNum: %d")
							   % maxFocusData_.sharpness
							   % maxFocusData_.position
							   % maxFocusData_.filenum));
			}

			if (Hardware::Instance().getFocusController()->Position() >= static_cast<int32_t>(coarseEndPosition))
			{
				const uint32_t sharpnessLowThreshold = HawkeyeConfig::Instance().get().autoFocusSharpnessLowThreshold;

				if (maxFocusData_.sharpness > sharpnessLowThreshold)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "######### Coarse Focus Complete ##########");
					Logger::L().Log (MODULENAME, severity_level::debug1,
									boost::str(boost::format("focus:: fileNum: %d, sharpness: %f, position: %d")
											   % maxFocusData_.filenum
											   % maxFocusData_.sharpness
											   % maxFocusData_.position));
					executeNextStep(CoarseFocusSteps::eComplete);
					return;
				}
				Logger::L().Log (MODULENAME, severity_level::notification, "Failed to find coarse focus position.");
				executeNextStep(CoarseFocusSteps::eError);
				return;
			}
			executeNextStep(CoarseFocusSteps::eFocusStep);
			return;
		}

		case CoarseFocusSteps::eComplete:
		{
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, true));
			return;
		}

		case CoarseFocusSteps::eError:
		default:
		{
			maxFocusData_ = { 0, 0, 0 };
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, false));
			return;
		}
	}

	// Unreachable Code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void CameraAutofocus::fineFocus(FineFocusSteps currentStep, FocusRange_t focusRange,
	std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto  executeNextStep = [this, callback, focusRange](FineFocusSteps nextStep) -> void
	{
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post([=]()
		{
			fineFocus(nextStep, focusRange, callback);
		});
	};

	// Check if operation has been cancelled
	if (isCancelled_.load())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "fineFocus : operation cancelled!");
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post(std::bind(callback, true));
		return;
	}

	static int fineImgRetries = 0;
	switch (currentStep)
	{
		case FineFocusSteps::eEntryPoint:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Fine Focus ##########");
			sharpnessData_.clear();
			fineImgRetries = 0;			// ensure the image retry count is reset at the start of each focus sweep

			Hardware::Instance().getFocusController()->SetPosition(
				[this, executeNextStep](bool status, int32_t endPos)
			{
				const uint32_t fineFocusRange = HawkeyeConfig::Instance().get().autoFocusFineRange;
				const uint32_t fineFocusStepSize = HawkeyeConfig::Instance().get().autoFocusFineStep;

				Hardware::Instance().getFocusController()->AdjustFineStep(fineFocusStepSize);

				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("using +/- %d counts and step size of %d") % fineFocusRange % fineFocusStepSize));
				executeNextStep(status ? FineFocusSteps::eFocusStep : FineFocusSteps::eError);

			}, focusRange.start);
			return;
		}

		case FineFocusSteps::eFocusStep:
		{
			Hardware::Instance().getFocusController()->FocusStepUpFine(
				[this, executeNextStep](bool status, int32_t endPos)
			{
				executeNextStep(status ? FineFocusSteps::eTakePicture : FineFocusSteps::eError);
			});
			fineImgRetries = 0;			// reseet the image retry count for each focus step
			return;
		}

		case FineFocusSteps::eTakePicture:
		{
			takePictureAndUpdateFgMask(false, [=](bool status)
			{
				if (!status)
				{
					if ( ++fineImgRetries >= MaxAfImageRetries )
					{
						Logger::L().Log ( MODULENAME, severity_level::error, "Failed to update foreground mask image!" );
						executeNextStep( FineFocusSteps::eError );
					}
					else
					{
						Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str(boost::format("Failure updating foreground mask image! try: %d") % fineImgRetries ));
						executeNextStep( FineFocusSteps::eTakePicture );
					}
					return;
				}
				executeNextStep(FineFocusSteps::eCheckComplete);
			});
			return;
		}

		case FineFocusSteps::eCheckComplete:
		{
			int32_t sharpness = 0;
			if (!getHistogramWhiteCount(fgMaskMOG_, &sharpness))
			{
				if ( ++fineImgRetries >= MaxAfImageRetries )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "Failed to get histogram white count of current image!" );
					executeNextStep( FineFocusSteps::eError );
				}
				else
				{
					Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str( boost::format( "Failure getting histogram white count of current image! try: %d" ) % fineImgRetries ) );
					executeNextStep( FineFocusSteps::eTakePicture );
				}
				return;
			}

			if (sharpness < 0)
			{
				if (++fineImgRetries >= MaxAfImageRetries)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to get histogram white count of current image!");
					executeNextStep(FineFocusSteps::eError);
				}
				else
				{
					std::string logStr = "Current image invalid : ";
					if (sharpness < -1)	// unknown invalid image content
					{
						logStr.append("unknown image error.");
					}
					else	// whitecount too high; appears to be black image
					{
						logStr.append("black image.");
					}
					logStr.append(boost::str(boost::format("  try: %d") % fineImgRetries));
					Logger::L().Log (MODULENAME, severity_level::debug1, logStr);
					executeNextStep(FineFocusSteps::eTakePicture);
				}
				return;
			}

			SharpnessData_t sd;
			sd.filenum = imageCnt_;
			sd.sharpness = sharpness;
			sd.position = Hardware::Instance().getFocusController()->Position();
			
			sharpnessData_.push_back(sd);

			Logger::L().Log (MODULENAME, severity_level::debug1,
							boost::str(boost::format("fineFocus:: filenum: %d, sharpness: %d, position: %d")
									   % sd.filenum
									   % sd.sharpness
									   % sd.position));

			auto curPos = (uint32_t)Hardware::Instance().getFocusController()->Position();
			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("focus::fine_focus1 curPos: %d") % curPos));

			if (curPos <= static_cast<uint32_t>(focusRange.end))
			{
				executeNextStep(FineFocusSteps::eFocusStep);
			}
			else
			{
				executeNextStep(FineFocusSteps::eComplete);
			}
			return;
		}

		case FineFocusSteps::eComplete:
		{
			auto tempMaxSharpness = findMaxSharpness(sharpnessData_);
			if (!tempMaxSharpness)
			{
				executeNextStep(FineFocusSteps::eError);
				return;
			}
			maxFocusData_ = tempMaxSharpness.get();
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, true));
			return;
		}

		case FineFocusSteps::eError:
		default:
		{
			maxFocusData_ = {0, 0, 0};
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, false));
			return;
		}
	}

	// Unreachable Code
	HAWKEYE_ASSERT (MODULENAME, false);
}

boost::optional<CameraAutofocus::SharpnessData_t> CameraAutofocus::findMaxSharpness(const std::vector<SharpnessData_t>& vSharpnessData)
{
	// Find the max sharpness value for the fine focus.
	SharpnessData_t maxFocusData = {0, 0, 0};

	if (vSharpnessData.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "findMaxSharpness : Empty list of sharpness data");
		return boost::none;
	}

	uint32_t maxIdx = 0;
	uint32_t idx = 0;

	std::string output;
	for (auto it : vSharpnessData)
	{
		if (Logger::L().IsOfInterest(severity_level::debug1))
		{
			output += boost::str(boost::format("\n\tfilenum: %d, sharpness: %d, position: %d")
								 % it.filenum
								 % it.sharpness
								 % it.position);
		}

		if (it.sharpness > maxFocusData.sharpness)
		{
			maxFocusData = it;
			maxIdx = idx;
		}
		idx++;
	}

	if (Logger::L().IsOfInterest(severity_level::debug1))
	{

		Logger::L().Log (MODULENAME,
		                severity_level::debug1,
		                boost::str(boost::format("######### Fine Focus Complete ##########\nSharpness Data:%s\nMax Focus at: filenum: %d, sharpness: %d, position: %d")
		                                         % output
								                 % maxFocusData.filenum
								                 % maxFocusData.sharpness
								                 % maxFocusData.position));
		Logger::L().Log (MODULENAME, severity_level::debug1, "######### Calculating Optimal Focus Position ##########");
	}

	// create initial expected threshold floor value
	double W0_half = maxFocusData.sharpness / 2.0;
	std::vector<SharpnessData_t> M;
	M.reserve(vSharpnessData.size());
	output.clear();
	for (size_t index = 0; index < vSharpnessData.size(); index++)
	{
		SharpnessData_t tempSharpnessData = {};
		tempSharpnessData.sharpness = vSharpnessData[index].sharpness - W0_half;
		tempSharpnessData.position = vSharpnessData[index].position;
		tempSharpnessData.filenum = vSharpnessData[index].filenum;
		M.emplace_back(tempSharpnessData);

		if (Logger::L().IsOfInterest(severity_level::debug1))
			output += boost::str(boost::format("\nM[%d]: %d, %d") % index % M[index].sharpness % M[index].position);
	}
	Logger::L().Log (MODULENAME, severity_level::debug1, "Focus/Sharpness Measurements:" + output);

	// sanity check for index
	uint32_t m_size = static_cast<uint32_t>(M.size());
	if (maxIdx >= m_size-1)
	{
		maxIdx = m_size-1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// The adjusted focus detection algorithm attemps to handle less than ideal conditions in the
	// optics and fluidics systems.  Complete failure of the detection algorithm currently does NOT
	// provide the operator with any feedback regarding the reason for the failure.  The adjusted
	// detection algorithm attempts to detect the focus peak under conditions that may indicate the
	// instrument is in need of servicing (e.g. flow cell decontamination or cleaning; optics dust
	// accumulation), and display the results to make an operator aware of the instrument performance
	// degradation so that appriopriate action may be taken.  While the detection algorithm presents
	// data which may result from a system which needs service, the decision to put the instrument into
	// service based on the use of (potentially) degraded data is still controlled by the operator.
	////////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// search for the last value less than zero after the initial floor value is subtracted form all
	// values in the search array. The last negative value (if it exists) will be an indicator of the
	// transition slope to the max peak.  NOTE however that dirty/noisy systems or low bead counts may
	// produce floor values resulting in residuals which do not fall below 0.  Searching for the LAST
	// negative value prevents premature exit and index failure; if the first value in the array is
	// greater than 0, the index will be 0, and will fail the index sanity check.  Also, prematurely
	// detected transition points will produce erroneous interpolated transition slopes and may yield
	// incorrect peak width calculations.  Finally, the search must proceed to the detected MAX index,
	// not just half the array, to handle peaks skewed to the far end of the count search range.
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// last value < 0 in the section of M prior to the max; some conditions may produce NO values <= 0!
	uint32_t idx_zero = 0;
	for (idx = 0; idx < maxIdx; idx++)
	{
		if (M[idx].sharpness < 0.0)
		{
			idx_zero = idx + 1;	// point at the next position as the first value >= 0
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// calculate a residual value threshold based on the found peak count.  This value is used to
	// detect the point at which the peak curve should be starting a transition to the noise floor
	// state.  Search backward from the max point to ensure proper proximity to the peak.  NOTE that
	// for noisy/dirty systems, or for low bead counts, the floor value calculation may result in
	// floor values much greater than 0, so this check allow detection of a peak inflection transition
	// under much less than ideal conditions.
	////////////////////////////////////////////////////////////////////////////////////////////////////

	double W0_3_prcnt = maxFocusData.sharpness * 0.03;
	if (W0_3_prcnt < 1.0)	// ensure a non-zero comparison value
	{
		W0_3_prcnt = 1.0;
	}

	// start from max position and search backwards to find first residual value (after floor subtraction)
	// which is < 3% of max.  Don't let the search index go to 0, as 0 is an illegal index condition,
	// If no values found, the focus is not acceptable.
	uint32_t idx_thresh = 0;
	for (idx = maxIdx; idx > 0; idx--)
	{
		if (M[idx].sharpness < W0_3_prcnt)
		{
			idx_thresh = idx;
			if (idx_thresh < maxIdx)
			{
				idx_thresh++;		// point at the next position as the first value >= 3% residual
			}
			break;
		}
	}

	uint32_t idx1_pos = 0;

	// if no values found with residual < 3% of max, focus is not valid
	if (idx_thresh > 0)
	{
		// use the position closest to the max
		idx1_pos = max(idx_zero, idx_thresh);
	}

	// the index position can't be 0, >= ((M.size() - 1) || >= (vSharpnessData.size() - 1)
	if (idx1_pos < 1 || idx1_pos >= (m_size-1) || idx1_pos >= (vSharpnessData.size()-1))
	{
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("findMaxSharpness : Incorrect CMin position found!\nidx1: %d, Min: %d, Max(M): %d, Max(vSD): %d") % idx1_pos % 1 % (m_size - 2) % (vSharpnessData.size() - 2)));
		return boost::none;
	}

	// Location of first negative point is just before the first positive one:
	uint32_t idx1_neg = idx1_pos - 1;
	
	if (Logger::L().IsOfInterest(severity_level::debug1))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("CMin Sharpness values:\nM[%d]:: p: %d, s: %d\nM[%d]:: p: %d, s: %d") % (uint32_t)(idx1_neg) % M[idx1_neg].position % M[idx1_neg].sharpness % idx1_pos % M[idx1_pos].position % M[idx1_pos].sharpness));
	}

	// Interpolate to find Cmin.  Use the points on either side of the zero crossing.
	auto Cmin = (vSharpnessData[idx1_neg].position * M[idx1_pos].sharpness - vSharpnessData[idx1_pos].position * M[idx1_neg].sharpness)
		/ (M[idx1_pos].sharpness - M[idx1_neg].sharpness);

	idx_zero = 0;
	for (idx = maxIdx; idx < m_size; idx++)
	{
		if (M[idx].sharpness < 0.0)
		{
			idx_zero = idx;
			break;
		}
	}

	idx_thresh = 0;
	for (idx = maxIdx; idx < m_size; idx++)
	{
		if (M[idx].sharpness < W0_3_prcnt)
		{
			idx_thresh = idx;
			break;
		}
	}

	uint32_t idx2_neg = 0;

	// if no values found with residual < 3% of max, focus is not valid
	if (idx_thresh > 0)
	{
		// use the position closest to the max
		idx2_neg = std::min (idx_zero, idx_thresh);
	}

	if (idx2_neg < 1 || idx2_neg >= (m_size-1) || idx2_neg >= (vSharpnessData.size()-1))
	{
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("findMaxSharpness : Incorrect CMax position found!\nidx2: %d, Min: %d, Max(M): %d, Max(vSD): %d") % idx2_neg % 1 % (M.size() - 2) % (vSharpnessData.size() - 2)));
		return boost::none;
	}

	// Location of last positive point (just before the first negative one)
	uint32_t idx2_pos = idx2_neg - 1;
	
	if (Logger::L().IsOfInterest(severity_level::debug1))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("CMax Sharpness values:\nM[%d]:: p: %d, s: %d\nM[%d]:: p: %d, s: %d") % (uint32_t)(idx2_pos) % M[idx2_pos].position % M[idx2_pos].sharpness % idx2_neg % M[idx2_neg].position % M[idx2_neg].sharpness));
	}

	// Interpolate to find Cmax.
	auto Cmax = (vSharpnessData[idx2_neg].position * M[idx2_pos].sharpness - vSharpnessData[idx2_pos].position * M[idx2_neg].sharpness)
		/ (M[idx2_pos].sharpness - M[idx2_neg].sharpness);

	uint32_t Cref = (uint32_t)round((Cmin + Cmax) / 2);
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("Cref: %d") % Cref));

	maxFocusData.position = Cref;
	return maxFocusData;
}

void CameraAutofocus::takePictureAndUpdateFgMask(bool coarse_focus, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto generateFgMask = [this, callback, coarse_focus](cv::Mat image) -> void
	{
		if (image.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "generateFgMask: exit, empty image");
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pInternalIos->post(std::bind(callback, false));
			return;
		}

		std::string FOC = coarse_focus ? "COARSE" : "FINE";

		imageCnt_++;
		auto POS = Hardware::Instance().getFocusController()->Position();

		// Save grayscale focus image.
		std::string imagePath = boost::str (boost::format("%s\\AF_%s_%03d_%06d.png")
			% HawkeyeDirectory::Instance().getAutoFocusImageDir()
			% FOC
			% imageCnt_
			% POS);
		saveAutofocusImage (image, imagePath);

		if (!pMOG_)
		{
			pMOG_ = std::make_unique<BackgroundSubtractorMOG>();
			pMOG_->initialize(image.size(), image.type());
		}

		pMOG_->operator() (image, fgMaskMOG_);

		// Save b/w focus image.
		imagePath = boost::str (boost::format("%s\\AF_%s_%03d_%06d_bw.png")
			% HawkeyeDirectory::Instance().getAutoFocusImageDir()
			% FOC
			% imageCnt_
			% POS);
		saveAutofocusImage (fgMaskMOG_, imagePath);

		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post(std::bind(callback, true));
	};

	auto cameraCaptureCallback = [this, generateFgMask](cv::Mat image)
	{
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pInternalIos->post(std::bind(generateFgMask, image));
	};

	Hardware::Instance().getCamera()->takePicture (params_.exposureTime_usec, params_.ledType, cameraCaptureCallback);
}

bool CameraAutofocus::saveAutofocusImage (cv::Mat& image, const std::string& imagePath)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "saveAutofocusImage: writing " + imagePath);

	if (!HDA_WriteImageToFile(imagePath, image))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "saveAutofocusImage: Failed to save image to path : " + imagePath);
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::published_errors::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::debug_image,
			instrument_error::severity_level::notification));
		return false;
	}

	return true;
}

std::unique_ptr<AutofocusResultsDLL> CameraAutofocus::generateResult(
	bool success, const SharpnessData_t& maxFocusData_, const std::vector<SharpnessData_t>& vSharpnessData)
{
	auto result = std::make_unique<AutofocusResultsDLL>();
	result->dataset.clear();
	result->focus_successful = success;
	if (!success)
	{
		return result;
	}

	for (auto item : vSharpnessData)
	{
		AutofocusDatapoint dp;
		dp.focalvalue = (uint32_t)item.sharpness;
		dp.position = item.position;
		result->dataset.push_back(dp);
	}

	result->bestfocus_af_position = maxFocusData_.position;
	result->offset_from_bestfocus_um = static_cast<int32_t>(std::ceil(OffsetFromFlowCellDepth(HawkeyeConfig::Instance().get().flowCellDepth)));
	result->final_af_position = Hardware::Instance().getFocusController()->Position();

	result->bestfocus_af_image = {};
	result->bestfocus_af_image.image = cv::Mat(finalFocusedImage_.size(), finalFocusedImage_.type());
	finalFocusedImage_.copyTo(result->bestfocus_af_image.image);

	return result;
}
