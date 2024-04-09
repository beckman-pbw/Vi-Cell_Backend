#pragma once

#include <vector>

#include "opencv2/opencv.hpp"
#include "ImageWrapper.hpp"
#include "ImageWrapperUtilities.hpp"
#include "DataConversion.hpp"

typedef struct ImageWrapperDLL
{
	ImageWrapperDLL() {}
	ImageWrapperDLL(cv::Mat im) : image(im) {}
	ImageWrapper_t ToCStyle()
	{
		ImageWrapper_t iw = {};
		ImageWrapperUtilities::ConvertFromMat(image, iw);
		return iw;
	}

	cv::Mat image;
} ImageWrapperDLL;

typedef struct FL_ImageWrapperDLL
{
	FL_ImageWrapper_t ToCStyle()
	{
		FL_ImageWrapper_t iw = {};
		iw.fl_channel = fl_channel;
		iw.fl_image = fl_image.ToCStyle();
		return iw;
	}

	uint16_t fl_channel;
	ImageWrapperDLL fl_image;

} FL_ImageWrapperDLL;

typedef struct ImageSetWrapperDLL
{
	ImageSetWrapper_t ToCStyle()
	{
		ImageSetWrapper_t iw = {};
		iw.brightfield_image = brightfield_image.ToCStyle();
		DataConversion::convert_vecOfDllType_to_listOfCType (fl_images, iw.fl_images, iw.num_fl_channels);
		return iw;
	}

	ImageWrapperDLL brightfield_image;
	std::vector<FL_ImageWrapperDLL> fl_images;
} ImageSetWrapperDLL;
