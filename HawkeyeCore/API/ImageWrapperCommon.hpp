#pragma once

#include <stdint.h>

/*
Provide a structure to wrap information required to pass
an image from OpenCV through the DLL interface layer.

The Host should use this information to construct an OpenCV Mat using the constructor

cv::Mat::Mat(int     rows,
int     cols,
int     type,
void *  data,
size_t  step = AUTO_STEP) ;

If the Host receives this structure within a callback function and wishes to maintain access
to the image after the callback returns, then the Host MUST clone the image into its own memory
space - the DLL WILL deallocate the memory as soon as the callback function returns.
*/

typedef struct ImageWrapper_t
{
	uint16_t rows;
	uint16_t cols;
	uint8_t type; // OpenCV type ex: CV_8UC1 == 0 (see "types_c.h" from OpenCV
	uint32_t step; // bytes per row, including padding
	unsigned char* data; // Data buffer.  For multi-channel, will be in BGR order
} ImageWrapper_t;

typedef struct FL_ImageWrapper_t
{
	uint16_t fl_channel;
	ImageWrapper_t fl_image;

} FL_ImageWrapper_t;
