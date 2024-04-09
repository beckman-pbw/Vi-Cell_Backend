#include "stdafx.h"

#include "CameraSim.hpp"
#include "GlobalImageSize.hpp"
#include "HawkeyeDataAccess.h"
#include "ImageWrapperUtilities.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "CameraSim";
static const std::string external_image_path = "../ExternalImages";

//*****************************************************************************
CameraSim::CameraSim (std::shared_ptr<CBOService> pCBOService)
	: CameraBase (pCBOService)
	, imageCnt_(0)
	, MaxSimulationImageCount(8)
	, use_external_images(false)
{
	globalImageSize.width = DEFAULTIMAGEWIDTH;
	globalImageSize.height = DEFAULTIMAGEHEIGHT;

	// Observe the environment and see if we have external images to use.
	boost::filesystem::path external_image_folder(external_image_path);

	if (!boost::filesystem::exists(external_image_folder) ||
		!boost::filesystem::is_directory(external_image_folder))
	{
		// No images.
		return;
	}

	external_image_iterator = boost::filesystem::directory_iterator(external_image_folder);
	boost::filesystem::directory_iterator end_iter;

	use_external_images = false;
	std::size_t img_count = 0;
	while (external_image_iterator != end_iter)
	{
		if (boost::filesystem::is_regular_file(*external_image_iterator) &&
			(boost::filesystem::extension(*external_image_iterator) == ".png"))
		{
			use_external_images = true;

			Logger::L().Log (MODULENAME, severity_level::notification, 
				boost::str (boost::format ("[%d]: %s") % img_count % *external_image_iterator));

			++img_count;
		}
		++external_image_iterator;
	}

	external_image_iterator = boost::filesystem::directory_iterator(external_image_folder);
	
	std::string msg;
	if (use_external_images)
	{
		msg = boost::str(boost::format("Camera Simulator: using %d external images from %s") % img_count % external_image_folder.string());
	}
	else
	{
		msg = "Camera Simulator: using default simulation images";
	}

	Logger::L().Log (MODULENAME, severity_level::notification, msg);
}

//*****************************************************************************
CameraSim::~CameraSim()
{ }

bool CameraSim::isPresent()
{
	return true;
}

void CameraSim::shutdown()
{
	
}


//*****************************************************************************
bool CameraSim::setGain (double gain)
{
	return true;
}

//*****************************************************************************
bool CameraSim::setGainLimitsState(bool state)
{
	return true;
}

//*****************************************************************************
bool CameraSim::getGainRange (double& minGain, double& maxGain)
{
	minGain = 0.0;
	maxGain = 23.1;

	return true;
}

//*****************************************************************************
bool CameraSim::getImageSize(uint32_t& width, uint32_t& height)
{
	width  = globalImageSize.width;
	height = globalImageSize.height;

	return true;
}

//*****************************************************************************
bool CameraSim::getImageInfo(iCamera::ImageInfo& imageInfo)
{
	imageInfo.bitsPerChannel = 8;
	return true;
}

//*****************************************************************************
std::string CameraSim::getVersion()
{
	return "000";
}

//*****************************************************************************
void CameraSim::initializeAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	pCBOService_->enqueueInternal(callback, true);
}

//*****************************************************************************
void CameraSim::captureSingleFrame (
	uint32_t exposureTime_usec,
	HawkeyeConfig::LedType ledType, 
	std::function<void(cv::Mat)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	pCBOService_->enqueueInternal(callback, getImage());
}

//*****************************************************************************
void CameraSim::captureMultipleFrame(
	uint32_t exTime, HawkeyeConfig::LedType ledType, uint32_t numFrames,
	uint8_t fps, std::function<void(cv::Mat)> perImageCallack,
	std::function<void(bool)> completionCallback)
{
	HAWKEYE_ASSERT (MODULENAME, perImageCallack);
	HAWKEYE_ASSERT (MODULENAME, completionCallback);

	if (numFrames == 0)
	{
		pCBOService_->enqueueInternal(completionCallback, true);
		return;
	}

	auto image = getImage();
	if (image.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "captureMultipleFrame : Failed to load image");
		pCBOService_->enqueueInternal(completionCallback, false);
		return;
	}
	pCBOService_->enqueueInternal(perImageCallack, image);

	uint32_t newFrameCount = numFrames - 1;
	pCBOService_->enqueueInternal([=]()
	{
		captureMultipleFrame(exTime, ledType, newFrameCount, fps, perImageCallack, completionCallback);
	});
}

//*****************************************************************************
cv::Mat CameraSim::getImage()
{
	/*
		Image sources: 
			1: If we find image files (raw PNG) within the folder "ExternalImages" then we will read those.
			2: If we find NO images in 1, then we'll work on CamBF_XXX.epng from the start
	*/

	cv::Mat image = {};

	if (use_external_images)
	{
		auto find_next_image = [](boost::filesystem::directory_iterator& iter, boost::filesystem::path& folder, cv::Mat& img)->bool
		{
			/*
			Return the next instance of a PNG file. Allow a reset and full wrap-around.
			Increment iterator before returning
			
				Starting with current iterator, look for the next instance of .png file type

				If we find one before we reach the end of the iteration, return that one.

				Reset iterator to beginning of folder

				Starting with current iterator, look for the next instance of .png file type

				If we find one before we reach the end of the iteration, return that one.

				Return a failure (no image found)
			*/
						
			boost::filesystem::directory_iterator end_iter;

			// Initial pass from current iterator position through end of folder
			while (iter != end_iter)
			{
				if (boost::filesystem::is_regular_file(*iter) &&
					(boost::filesystem::extension(*iter) == ".png"))
				{
					Logger::L().Log (MODULENAME, severity_level::notification,
						boost::str (boost::format ("1: %s") % (*iter).path().string()));

					if (!ReadGrayScaleImageFromFile((*iter).path().string(), img))
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Failed to load image from path : " + (*iter).path().string());
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::imaging_camera_noimage, 
							instrument_error::severity_level::error));

						img.release();
						return false;
					}

					++iter;
					return true;
				}

				// Not this file.  Go to next.
				++iter;
			}


			// Not yet found. Reset to beginning of folder and retry.
			iter = boost::filesystem::directory_iterator(folder);
			while (iter != end_iter)
			{
				if (boost::filesystem::is_regular_file(*iter) &&
					(boost::filesystem::extension(*iter) == ".png"))
				{
					Logger::L().Log (MODULENAME, severity_level::notification,
						boost::str (boost::format ("2: %s") % (*iter).path().string()));

					if (!ReadGrayScaleImageFromFile((*iter).path().string(), img))
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Failed to load image from path : " + (*iter).path().string());
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::imaging_camera_noimage,
							instrument_error::severity_level::error));

						img.release();
						return false;
					}

					++iter;
					return true;
				}

				// Not this file.  Go to next.
				++iter;
			}
			
			// Not yet found after complete cycle through code.
			return false;
		};

		find_next_image(external_image_iterator, boost::filesystem::path(external_image_path), image);
	}
	else
	{
		std::string imagePath = boost::str(boost::format("CamBF_%d.png") % imageCnt_);
		Logger::L().Log (MODULENAME, severity_level::debug1, "Loading " + imagePath);

		
		if (!HDA_ReadEncryptedImageFile(imagePath, image))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to load image from path : " + imagePath);
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_camera_noimage, 
				instrument_error::severity_level::error));
			return image;
		}

		imageCnt_++;

		if (imageCnt_ == MaxSimulationImageCount)
		{
			imageCnt_ = 0;
		}
	}

	//std::string imagePath = boost::str(boost::format("CamBF_%d.png") % imageCnt_);

	//cvShowImage (imagePath.c_str(), &IplImage(image));
	//cv::waitKey (2000);

	return image;
}
