// Copyright(C)2017 by L&T Technology Services Inc. All rights reserved.

// This software contains proprietary and confidential information of 
// L&T Technology Services Inc., and its suppliers. Except as may be set forth 
// in the license agreement under which this software is supplied, use, 
// disclosure, or reproduction is prohibited without the prior express 
// written consent of L&T Technology Services, Inc.

#ifndef _IMAGETYPEDEF_H_
#define _IMAGETYPEDEF_H_


#include <map>
#include <utility>
#include <vector>

//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>



/*! \var typedef std::pair<cv::Mat, std::map<int, cv::Mat>> pair_InputImage_t;
	\brief A type definition for input image 
*/
typedef std::pair<cv::Mat, std::map<int, cv::Mat>> pair_InputImage_t;

/*! \var typedef std::vector<pair_InputImage_t> v_pair_InputImage_t; 
	\brief A type definition for vector of input images
*/
using v_pair_InputImage_t = std::vector<pair_InputImage_t>; 

#endif   /*_IMAGETYPEDEF_H_*/
