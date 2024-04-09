#pragma once

#include <opencv2/opencv.hpp>

#include <map>
#include <vector>

typedef std::map<int, cv::Mat> FLImages_t;
typedef std::pair<cv::Mat, FLImages_t> ImageSet_t;
typedef std::vector<ImageSet_t> ImageCollection_t;

// Used to hold black images to fill-in for Reanalysis.
typedef std::map<uint16_t, ImageSet_t> ImageSetCollection_t;
