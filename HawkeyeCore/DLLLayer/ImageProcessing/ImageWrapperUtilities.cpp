#include "stdafx.h"

#include "CellCounterFactory.h"
#include "ImageWrapperUtilities.hpp"
#include "Logger.hpp"

using namespace std;
using namespace cv;

static const char MODULENAME[] = "ImageWrapperUtilities";

HawkeyeError ImageWrapperUtilities::ConvertFromMat (Mat& input, ImageWrapper_t& output)
{
	if (input.empty() ||
		input.rows <= 0 ||
		input.cols <= 0 ||
		input.data == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}

	Mat bgrImage = input;

	output.rows = (uint16_t)bgrImage.rows;
	output.cols = (uint16_t)bgrImage.cols;
	output.type = (uint8_t)bgrImage.type();

	output.step = (uint32_t)bgrImage.step;
	// output.step = (output.step + 3) & ~3u; // 4 bytes padding

	auto srcLength = bgrImage.rows * bgrImage.step;
	auto destLength = output.rows * output.step;

	output.data = new unsigned char[destLength];
	memcpy_s(output.data, destLength, bgrImage.data, srcLength);

	bgrImage.release();

	return HawkeyeError::eSuccess;
}

bool ImageWrapperUtilities::ConvertToBWImage (const cv::Mat& orgImage, cv::Mat& bwImage, double threshold)
{
	bwImage = cv::Mat();

	if (orgImage.empty())
	{		
		return false;
	}

	if (orgImage.channels() == 3) // Number of channel in BGR image
	{
		cv::cvtColor(orgImage, bwImage, CV_BGR2GRAY);
		bwImage = bwImage > threshold;		
	}
	else if(orgImage.channels() == 1) // Number of channel in grayScale image
	{
		bwImage = orgImage > threshold;
	}

	return bwImage.empty() ? false : true;
}

bool ImageWrapperUtilities::AnnotateImage (const cv::Mat& orgImage, cv::Mat& annotatedImage, CellCounterResult::SResult& resultData, int imageNum, uint8_t annotationType)
{
	annotatedImage = CellCounter::CellCounterFactory::GetAnnotatedImage (orgImage, imageNum, resultData, (annotationType & AnnotationType::Text), (annotationType & AnnotationType::NonCells), (annotationType & AnnotationType::JustOverlay));
	return !annotatedImage.empty();
}

bool ImageWrapperUtilities::TransformImage (eImageOutputType xform, const cv::Mat& orgImage, cv::Mat& transformImage, int bwThreshold, CellCounterResult::SResult& resultData, int imageNum)
{
	switch (xform)
	{
		case eImageOutputType::eImageRaw:
		{
			transformImage = orgImage.clone();
			break;
		}
		case eImageOutputType::eImageBW:
		{
			if (!ImageWrapperUtilities::ConvertToBWImage (orgImage, transformImage, bwThreshold))
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("Failed to get B/W image for index %d") % imageNum));
				return false;
			}
			break;
		}
		case eImageOutputType::eImageAnnotated:
		{
			if (!ImageWrapperUtilities::AnnotateImage (orgImage, transformImage, resultData, imageNum, ImageWrapperUtilities::AnnotationType::Default)) //Maybe? | ImageWrapperUtilities::AnnotationType::NonCells
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("Failed to get annotated image for index %d") % imageNum));
				return false;
			}
			break;
		}
		case eImageOutputType::eImageAnnotated_WithNumbers:
		{
			if (!ImageWrapperUtilities::AnnotateImage (orgImage, transformImage, resultData, imageNum, ImageWrapperUtilities::AnnotationType::Text)) //Maybe? | ImageWrapperUtilities::AnnotationType::NonCells
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("Failed to get annotated/text image for index %d") % imageNum));
				return false;
			}
			break;
		}
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "TransformImage: Unsupported transformation");
			return false;
		}
			
	}

	return true;
}
