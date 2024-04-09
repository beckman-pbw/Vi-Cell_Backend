#pragma once

#include <iostream>

#include <CellCountingOutputParams.h>
#include <ICellCounter.h>

using namespace cv;
using namespace std;

class SaveResult
{
public:
	static void Initialize();

	static void SaveAnalysisInfo (CellCounterResult::SResult &map_oCellCountingOutputParamsBuffer, const string& strPathToSave, const string& strFileName);
/*
	Mat AnnotateImage(const cv::Mat &oMatInputImage,
		const CellCounterResult::SResult &oCellCountingOutputParam, bool bIsTextWrite = true,
		bool bAnnotateBlobs = false, bool bOverlayOnly = false, cv::Scalar POIColorSetting = cv::Scalar(0, 255, 0),
		cv::Scalar CellColorSetting = cv::Scalar(0, 0, 255),
		cv::Scalar BlobColorSetting = cv::Scalar(255, 0, 0)) const;

	void ConvertImageToThreeChannels(const Mat& const_r_oMatINPUT_IMAGE, Mat& r_oMatOutputImage) const;
*/
	static void SaveSingleImageInfo (CellCounterResult::SResult &oCellCountingOutputParam, const string& strPathToSave, int nImageNumber);
};
