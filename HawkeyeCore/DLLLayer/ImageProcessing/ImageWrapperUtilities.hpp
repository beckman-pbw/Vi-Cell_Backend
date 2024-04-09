#pragma once

#include <iostream>

#include "opencv2/opencv.hpp"

#include "HawkeyeError.hpp"
#include "ImageWrapper.hpp"

#include "CellCountingOutputParams.h"

enum class eImageOutputType : uint16_t
{
	eImageRaw = 0,
	eImageBW,
	eImageAnnotated,
	eImageAnnotated_WithNumbers,
	//	eImageOverlay,  // FL-only
};

class ImageWrapperUtilities
{
public:	
	enum AnnotationType // Used as a bitmask for the annotation
	{
		Default = 0x00,
		Text = 0x01,
		NonCells = 0x02,
		JustOverlay = 0x04,
	};

	static HawkeyeError ConvertFromMat(cv::Mat& input, ImageWrapper_t& output);
	static bool ConvertToBWImage(const cv::Mat& orgImage, cv::Mat& bwImage, double threshold);
	static bool AnnotateImage(const cv::Mat& orgImage, cv::Mat& annotatedImage, CellCounterResult::SResult& resultData, int imageNum, uint8_t annotationType);
	static bool TransformImage(eImageOutputType xform, const cv::Mat& orgImage, cv::Mat& transformImage, int bwThreshold, CellCounterResult::SResult& resultData, int imageNum);
};
