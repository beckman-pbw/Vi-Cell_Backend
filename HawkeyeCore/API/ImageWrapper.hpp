#pragma once

#include "ImageWrapperCommon.hpp"

typedef struct ImageSetWrapper_t
{
	ImageWrapper_t brightfield_image;

	uint8_t num_fl_channels;
	FL_ImageWrapper_t* fl_images;
} ImageSetWrapper_t;
