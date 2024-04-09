#include "stdafx.h"

#include "ImageAnalysisUtilities.hpp"
#include "SaveResults.h"

static std::map<int, std::string>map_eProcess;

namespace CP = ConfigParameters;

//*****************************************************************************
void SaveResult::Initialize()
{
	map_eProcess[0] = "eSuccess";
	map_eProcess[1] = "eInvalidInputPath";
	map_eProcess[2] ="eValueOutOfRange";
	map_eProcess[3] ="eZeroInput";
	map_eProcess[4] ="eNullCellsData";
	map_eProcess[5] ="eInvalidImage";
	map_eProcess[6] ="eResultNotAvailable";
	map_eProcess[7] ="eFileWriteError";
	map_eProcess[8] ="eZeroOutputData";
	map_eProcess[9] ="eParameterIsNegative";
	map_eProcess[10] ="eInvalidParameter";
	map_eProcess[11] ="eBubbleImage";
	map_eProcess[12] ="eFLChannelsMissing";
	map_eProcess[13] ="eInvalidCharacteristics";
	map_eProcess[14] ="eInvalidAlgorithmMode";
	map_eProcess[15] ="eMoreThanOneFLImageSupplied";
	map_eProcess[16] ="eTransformationMatrixMissing";
	map_eProcess[17] ="eFailure";
	map_eProcess[18] ="eInvalidBackgroundIntensity";
}

//*****************************************************************************
void SaveResult::SaveAnalysisInfo(
	CellCounterResult::SResult &map_SResult,
	const string& strPathToSave,
	const string& strFileName)
{
	string strDistributionPathToSave = strPathToSave;
	std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> map_ConfigParams = map_SResult.Processing_Settings.map_InputConfigParameters;
	const CellCounterResult::SImageStats stCumulativeOutput = map_SResult.Cumulative_results;
	
	strDistributionPathToSave += "/" + strFileName + ".csv";
	ofstream file(strDistributionPathToSave);

	if (file.is_open())
	{
		file << "Results- GP " << "," << "," << "," 
			 << "Results- POI" << "," << "," << "," << "Settings" << endl;

		file << "TotalImages :" << "," << stCumulativeOutput.nTotalCumulative_Imgs << "," << "," <<"," << "," <<"," 
			 << "Algorithm Mode :" << "," << ((map_ConfigParams[CP::eAlgorithmMode] == 0) ? "eLEGACYXR" : "eBLUE") << endl;
			
		file << "TotalCells_GP :" << "," << stCumulativeOutput.nTotalCellCount_GP << "," << "," 
			 << "TotalCells_POI :" << "," << stCumulativeOutput.nTotalCellCount_POI << "," << ","
			 << "EnableBubble Mode :" << "," << ((map_ConfigParams[CP::eBubbleMode] == 0) ? " False" : " True") << endl;

		file << "PopulationPercentage :" << "," << stCumulativeOutput.dPopulationPercentage << "," << "," << "," << "," << "," 
			 << "Decluster Mode : :" << "," << ((map_ConfigParams[CP::eDeclusterMode] == 0) ? " False" : " True") << endl;

		file << "CellConcentration_GP :" << "," << stCumulativeOutput.dCellConcentration_GP<< "," << ","
			 << "CellConcentration_POI :" << "," << stCumulativeOutput.dCellConcentration_POI << "," << ","
			 << "SubPeakAnalysis Mode : :" << "," << ((map_ConfigParams[CP::eSubPeakAnalysisMode] == 0) ? " False" : " True") << endl;

		file << "AvgDiameter_GP(microns):" << "," << stCumulativeOutput.dAvgDiameter_GP << "," << ","	
			 << "AvgDiameter_POI(microns) :" << "," << stCumulativeOutput.dAvgDiameter_POI << "," << ","
			 << "ROI_Xcords :" << "," << map_ConfigParams[CP::eROIXCoOrdinate] << endl;

		file << "AvgCircularity_GP :" << "," << stCumulativeOutput.dAvgCircularity_GP << "," << ","
			 << "AvgCircularity_POI :" << "," << stCumulativeOutput.dAvgCircularity_POI << "," << ","
			 << "ROI_Ycords :" << "," << map_ConfigParams[CP::eROIYCoOrdinate] << endl;

		file << "AvgSharpness_GP :" << "," << stCumulativeOutput.dAvgSharpness_GP << "," << ","
			 << "AvgSharpness_POI :" << "," << stCumulativeOutput.dAvgSharpness_POI << "," << ","
			 << "SizingSlope :" << "," << map_ConfigParams[CP::eSizingSlope] << endl;

		file << "AvgEccentricity_GP :" << "," << (stCumulativeOutput.dAvgEccentricity_GP) << "," << ","
			 << "AvgEccentricity_POI :" << "," << (stCumulativeOutput.dAvgEccentricity_POI) << "," << ","
			 << "SizingIntercept :" << "," << map_ConfigParams[CP::eSizingIntercept] << endl;

		file << "AvgAspectRatio_GP :" << "," << (stCumulativeOutput.dAvgAspectRatio_GP) << "," << ","
			 << "AvgAspectRatio_POI :" << "," << (stCumulativeOutput.dAvgAspectRatio_POI) << "," << "," 
			 << "DilutionFactor:" << "," << map_ConfigParams[CP::eDilutionFactor] << endl;

		file << "AvgRoundness_GP :" << "," << (stCumulativeOutput.dAvgRoundness_GP) << "," << ","
			 << "AvgRoundness_POI :" << "," << (stCumulativeOutput.dAvgRoundness_POI) << "," << ","
			 << "Concentration_Slope(M):" << "," << map_ConfigParams[CP::eConcentrationSlope] << endl;
		 	
		file << "AvgRawCellSpotBrightness_GP :" << "," << (stCumulativeOutput.dAvgRawCellSpotBrightness_GP) << "," << ","
			 << "AvgRawCellSpotBrightness_POI :" << "," << (stCumulativeOutput.dAvgRawCellSpotBrightness_POI) << "," << "," 
			 << "Concentration_Intercept(B):" << "," << map_ConfigParams[CP::eConcentrationIntercept] << endl;
		
		file << "AvgCellSpotBrightness_GP :" << "," << (stCumulativeOutput.dAvgCellSpotBrightness_GP) << "," << ","
			 << "AvgCellSpotBrightness_POI :" << "," << (stCumulativeOutput.dAvgCellSpotBrightness_POI) << "," << ","
			 << "ImageControlCount:" << "," << map_ConfigParams[CP::eConcentrationImageControlCount] << endl;

		file << "Avgbackground Intensity:" << "," << stCumulativeOutput.dAvgBackgroundIntensity << "," << "," << "," << "," << ","  
			 << "BubbleMinSpotAreaPercent:" << "," << map_ConfigParams[CP::eBubbleMinSpotAreaPercent] << endl;
			
		file << "TotalBubble Count :" << "," << stCumulativeOutput.nTotalBubbleCount << "," << "," << "," << "," << "," 
			 << "BubbleMinSpotAvgBrightness:" << "," << map_ConfigParams[CP::eBubbleMinSpotAvgBrightness] << endl;
			
		file << "nLargeClusterCount :" << "," << stCumulativeOutput.nLargeClusterCount << "," << "," << "," << "," << ","
			 << "BubbleRejectionImgAreaPercent:" << "," << map_ConfigParams[CP::eBubbleRejectionImgAreaPercent] << endl;

		file << "," << "," << "," << "," << "," << ","
			 << "NominalBackgroundLevel:" << "," << map_ConfigParams[CP::eNominalBackgroundLevel] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "BackgroundIntensityTolerance:" << "," << map_ConfigParams[CP::eBackgroundIntensityTolerance] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "CenterSpotMinIntensityLimit:" << "," << map_ConfigParams[CP::eCenterSpotMinIntensityLimit] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "PeakIntensitySelectionAreaLimit:" << "," << map_ConfigParams[CP::ePeakIntensitySelectionAreaLimit] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "CellSpotBrightnessExclusionThreshold:" << "," << map_ConfigParams[CP::eCellSpotBrightnessExclusionThreshold] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "HotPixelEliminationMode:" << "," << map_ConfigParams[CP::eHotPixelEliminationMode] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "ImageBoundaryAnnotationMode:" << "," << map_ConfigParams[CP::eImgBottomAndRightBoundaryAnnotationMode] << endl;

		file << "," << "," << "," << "," << "," << "," 
			 << "SmallParticleSizingCorrection:" << "," << map_ConfigParams[CP::eSmallParticleSizingCorrection] << endl;

		file << endl;
		
		file << "Cell Identification Parameters:" <<","<<"Threshold"<<","<<"Polarity"<< endl;
		for (const auto& v : map_SResult.Processing_Settings.v_CellIdentificationParameters)
		{
			const std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(std::get<0>(v));
			file << label << "," << std::get<1>(v) << ","  << ((std::get<2>(v)) != 0 ? "eATABOVE" :"eBELOW" )<< endl;
		}
		file << endl;

		file << "POI Identification Parameters:" <<","<<"Threshold"<<","<<"Polarity"<< endl;
		for (const auto& v : map_SResult.Processing_Settings.v_POIIdentificationParameters)
		{
			const std::string label = ImageAnalysisUtilities::GetCharacteristicLabel (std::get<0>(v));
				file << label << "," << std::get<1>(v) << "," << ((std::get<2>(v)) != 0 ? "eATABOVE" : "eBELOW")<< endl;
		}
		file << endl;

		file << "Image Number" << "," 
			 << "TotalCells_GP" << "," 
			 << "TotalCells_POI" << "," 
			 << "PopulationPercentage" << ","
			 << "CellConcentration_GP-Cells/ml (x10^6)" << "," 
			 << "CellConcentration_POI-Cells/ml (x10^6)" << "," 
			 << "AvgDiameter_GP(microns)" << ","
			 << "AvgDiameter_POI(microns)" << ","
			 << "AvgCircularity_GP" << ","
			 << "AvgCircularity_POI" << ","
			 << "AvgSharpness_GP" << "," 
			 << "AvgSharpness_POI" << "," 
			 << "AvgEccentricity_GP" << "," 
			 << "AvgEccentricity_POI" << ","
			 << "AvgAspectRatio_GP" << "," 
			 << "AvgAspectRatio_POI" << "," 
			 << "AvgRoundness_GP" << ","
			 << "AvgRoundness_POI" << "," 
			 << "AvgRawCellSpotBrightness_GP" << "," 
			 << "AvgRawCellSpotBrightness_POI" << "," 
			 << "AvgCellSpotBrightness_GP" << "," 
			 << "AvgCellSpotBrightness_POI" << ","
			 << "Avgbackground Intensity" << ","
			 << "TotalBubble Count" << "," 
			 << "LargeCluster Count" << "," 
			 << "Process Status"
			 << endl;

		for (const auto& v : map_SResult.map_Image_Results)
		{
			const CellCounterResult::SImageStats stTempData = v.second;

			file << v.first << ","
				 << stTempData.nTotalCellCount_GP << ","
				 << stTempData.nTotalCellCount_POI << ","
				 << stTempData.dPopulationPercentage << ","
				 << stTempData.dCellConcentration_GP << ","
				 << stTempData.dCellConcentration_POI << ","
				 << stTempData.dAvgDiameter_GP << ","
				 << stTempData.dAvgDiameter_POI << ","
				 << stTempData.dAvgCircularity_GP << ","
				 << stTempData.dAvgCircularity_POI << ","
				 << stTempData.dAvgSharpness_GP << ","
				 << stTempData.dAvgSharpness_POI << ","
				 << stTempData.dAvgEccentricity_GP << ","
				 << stTempData.dAvgEccentricity_POI << ","
				 << stTempData.dAvgAspectRatio_GP << ","
				 << stTempData.dAvgAspectRatio_POI << ","
				 << stTempData.dAvgRoundness_GP << ","
				 << stTempData.dAvgRoundness_POI << ","
				 << stTempData.dAvgRawCellSpotBrightness_GP << ","
				 << stTempData.dAvgRawCellSpotBrightness_POI << ","
				 << stTempData.dAvgCellSpotBrightness_GP << ","
				 << stTempData.dAvgCellSpotBrightness_POI << ","
				 << stTempData.dAvgBackgroundIntensity << ","
				 << stTempData.nTotalBubbleCount << ","
				 << stTempData.nLargeClusterCount << ","
				 << map_eProcess[stTempData.eProcessStatus] 
				 << endl;
		}
		file.close();
	}
}

/*
//*****************************************************************************
//NOTE: not currently used.
//*****************************************************************************
Mat SaveResult::AnnotateImage(const cv::Mat &oMatInputImage,
	const CellCounterResult::SResult &oCellCountingOutputParam, bool bIsTextWrite,
	bool bAnnotateBlobs, bool bOverlayOnly,
	cv::Scalar POIColorSetting,
	cv::Scalar CellColorSetting,
	cv::Scalar BlobColorSetting) const
{	
	cv::Mat oMatThreeChannelImage;
	if (!oCellCountingOutputParam.map_Blobs_for_image.empty())
	{
		const auto v_BlobDataItr = *(oCellCountingOutputParam.map_Blobs_for_image.begin());
		std::shared_ptr<std::vector<Blob::SBlobData>> v_stBlobData = v_BlobDataItr.second;

		auto map_ConfigParams = oCellCountingOutputParam.Processing_Settings.map_InputConfigParameters;
		if (bOverlayOnly)
		{
			oMatThreeChannelImage = cv::Mat(oMatInputImage.rows, oMatInputImage.cols, CV_8UC3, cv::Scalar(255, 255, 255));
		}
		else
		{
			if (oMatInputImage.channels() == 1) {
				ConvertImageToThreeChannels(oMatInputImage, oMatThreeChannelImage);
			}
			else if (oMatInputImage.channels() == 3) {
				oMatInputImage.copyTo(oMatThreeChannelImage);
			}
		}
		const cv::Point optTopLeftPoint(static_cast<int>(map_ConfigParams[CP::eROIXCoOrdinate]), (int)map_ConfigParams[CP::eROIYCoOrdinate]);
		const cv::Point optBottomLeftPoint(static_cast<int>(map_ConfigParams[CP::eROIXCoOrdinate]), oMatInputImage.rows);

		const cv::Point optTopRightPoint(oMatInputImage.cols, static_cast<int>(map_ConfigParams[CP::eROIYCoOrdinate]));

		cv::line(oMatThreeChannelImage, optTopLeftPoint, optBottomLeftPoint, cv::Scalar(255, 0, 0), 2, 8);
		cv::line(oMatThreeChannelImage, optTopLeftPoint, optTopRightPoint, cv::Scalar(255, 0, 0), 2, 8);

		const cv::Point oPoint1BottomImageBoundry((oMatInputImage.rows - 5), static_cast<int>(map_ConfigParams[CP::eROIYCoOrdinate]));
		const cv::Point oPoint2BottomImageBoundry((oMatInputImage.rows - 5), oMatInputImage.cols - 5);

		const cv::Point oPoint1RightImageBoundry(static_cast<int>(map_ConfigParams[CP::eROIXCoOrdinate]), (oMatInputImage.rows - 5));
		const cv::Point oPoint2RightImageBoundry((oMatInputImage.rows - 5), (oMatInputImage.cols - 5));

		cv::line(oMatThreeChannelImage, oPoint1BottomImageBoundry, oPoint2BottomImageBoundry, cv::Scalar(255, 0, 0), 2, 8);
		cv::line(oMatThreeChannelImage, oPoint1RightImageBoundry, oPoint2RightImageBoundry, cv::Scalar(255, 0, 0), 2, 8);


		if (nullptr != v_stBlobData) {
			int nBlobItr = 0;
			for (auto ostTempData = v_stBlobData->begin(); ostTempData != v_stBlobData->end(); ++ostTempData, ++nBlobItr) {
				if (bIsTextWrite) {
					if (0 == ostTempData->map_Characteristics[Characteristic(IsDeclustered)]) {
						putText(oMatThreeChannelImage, std::to_string(nBlobItr),
							ostTempData->v_oCellBoundaryPoints[0], cv::FONT_HERSHEY_SIMPLEX,
							0.8, cv::Scalar(0, 0, 0), 2);
					}
					else {
						putText(oMatThreeChannelImage, std::to_string(nBlobItr),
							ostTempData->oPointCenter, cv::FONT_HERSHEY_SIMPLEX,
							0.8, cv::Scalar(0, 0, 0), 2);
					}
				}

				if (1 == ostTempData->map_Characteristics[Characteristic(IsCell)]) {
					if (1 == ostTempData->map_Characteristics[Characteristic(IsPOI)]) {
						cv::circle(oMatThreeChannelImage, ostTempData->oPointCenter,
							static_cast<int>(ostTempData->map_Characteristics[Characteristic(DiameterInPixels)]) / 2,
							POIColorSetting, 2, 3, 0);
					}
					else if (0 == ostTempData->map_Characteristics[Characteristic(IsPOI)]) {
						cv::circle(oMatThreeChannelImage, ostTempData->oPointCenter,
							static_cast<int>(ostTempData->map_Characteristics[Characteristic(DiameterInPixels)]) / 2,
							CellColorSetting, 2, 3, 0);
					}
				}
				else if (1 == ostTempData->map_Characteristics[Characteristic(IsBubble)]) {
					
					const cv::Scalar oScalarColorValue(0, 255, 255);
					std::vector<std::vector<cv::Point>> v_BubbleContours;
					v_BubbleContours.push_back(ostTempData->v_oCellBoundaryPoints);
					cv::drawContours(oMatThreeChannelImage, v_BubbleContours, -1,
						oScalarColorValue, 5, 8);
//					rectangle(oMatThreeChannelImage, cv::Point(r.x, r.y), cv::Point(r.x + r.width, r.y + r.height),
//						oScalarColorValue, 2, 3, 0);
				}
				else {
					if (bAnnotateBlobs) {
						cv::circle(oMatThreeChannelImage, ostTempData->oPointCenter,
							static_cast<int>(ostTempData->map_Characteristics[Characteristic(DiameterInPixels)]) / 2,
							BlobColorSetting, 2, 3, 0);
					}
				}
			}

			const auto v_stLargeClusterItr = *(oCellCountingOutputParam.map_v_stLargeClusterData.begin());
			std::vector<CellCounterResult::SLargeClusterData> v_LargeCluster = v_stLargeClusterItr.second;
			for (auto& v_ClusterItr : v_LargeCluster)
			{
				const cv::Rect r = v_ClusterItr.BoundingBox;
				const cv::Scalar oScalarColorValue(0, 0, 255);
				rectangle(oMatThreeChannelImage, cv::Point(r.x, r.y), cv::Point(r.x + r.width, r.y + r.height), 
					oScalarColorValue, 5, 8);
			}
		}
	}
	return oMatThreeChannelImage;
}

void SaveResult::ConvertImageToThreeChannels(const Mat& const_r_oMatINPUT_IMAGE, Mat& r_oMatOutputImage) const
{
	r_oMatOutputImage = Mat(const_r_oMatINPUT_IMAGE.rows, const_r_oMatINPUT_IMAGE.cols, CV_8UC3);

	vector<Mat> o_vMatTemp;
	o_vMatTemp.reserve(3);
	for (int nCount = 0; nCount < 3; nCount++) {
		o_vMatTemp.push_back(const_r_oMatINPUT_IMAGE);
	}
	cv::merge(o_vMatTemp, r_oMatOutputImage);

	for (int nCount = 0; nCount < 3; nCount++) {
		o_vMatTemp[nCount].release();
	}
	o_vMatTemp.clear();
}

*/

//*****************************************************************************
void SaveResult::SaveSingleImageInfo(
	CellCounterResult::SResult &oCellCountingOutputParam,
	const string& strPathToSave, 
	int nImageNumber)
{
	std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> &map_ConfigParams = oCellCountingOutputParam.Processing_Settings.map_InputConfigParameters;
	const CellCounterResult::SImageStats &stImageStats = oCellCountingOutputParam.map_Image_Results[nImageNumber];

	if (stImageStats.eProcessStatus == eSuccess)
	{
		ofstream file(strPathToSave);
		if (file.is_open())
		{
			file << "Results- GP " << "," << "," << "," 
				 << "Results- POI" << "," << "," << "," 
				 << "Settings" << endl;

			file << "ImageNo :" << "," << nImageNumber << "," << "," << "," << "," << ","
				 << "Algorithm Mode :" << "," << ((map_ConfigParams[CP::eAlgorithmMode] == 0) ? "eLEGACYXR" : "eBLUE") << endl;

			file << "TotalCells_GP :" << "," << stImageStats.nTotalCellCount_GP << "," << ","
				 << "TotalCells_POI :" << "," << stImageStats.nTotalCellCount_POI << "," << ","
				 << "EnableBubble Mode :" << "," << ((map_ConfigParams[CP::eBubbleMode] == 0) ? " False" : " True") << endl;

			file << "PopulationPercentage :" << "," << stImageStats.dPopulationPercentage << "," << ","
				 << "," << "," << ","
				 << "Decluster Mode :" << "," << ((map_ConfigParams[CP::eDeclusterMode] == 0) ? " False" : " True") << endl;

			file << "CellConcentration_GP :" << "," << stImageStats.dCellConcentration_GP << "," << ","
				 << "CellConcentration_POI :" << "," << stImageStats.dCellConcentration_POI << "," << ","
				 << "SubPeakAnalysis Mode :" << "," << ((map_ConfigParams[CP::eSubPeakAnalysisMode] == 0) ? " False" : " True") << endl;

			file << "AvgDiameter_GP(microns):" << "," << stImageStats.dAvgDiameter_GP << "," << ","
				 << "AvgDiameter_POI(microns) :" << "," << stImageStats.dAvgDiameter_POI << "," << ","
				 << "ROI_Xcords :" << "," << map_ConfigParams[CP::eROIXCoOrdinate] << endl;

			file << "AvgCircularity_GP :" << "," << stImageStats.dAvgCircularity_GP << "," << ","
				 << "AvgCircularity_POI :" << "," << stImageStats.dAvgCircularity_POI << "," << ","
				 << "ROI_Ycords :" << "," << map_ConfigParams[CP::eROIYCoOrdinate] << endl;

			file << "AvgSharpness_GP :" << "," << stImageStats.dAvgSharpness_GP << "," << ","
				 << "AvgSharpness_POI :" << "," << stImageStats.dAvgSharpness_POI << "," << ","
				 << "SizingSlope :" << "," << map_ConfigParams[CP::eSizingSlope] << endl;

			file << "AvgEccentricity_GP :" << "," << (stImageStats.dAvgEccentricity_GP) << "," << ","
				 << "AvgEccentricity_POI :" << "," << (stImageStats.dAvgEccentricity_POI) << "," << ","
				 << "SizingIntercept :" << "," << map_ConfigParams[CP::eSizingIntercept] << endl;

			file << "AvgAspectRatio_GP :" << "," << (stImageStats.dAvgAspectRatio_GP) << "," << ","
				 << "AvgAspectRatio_POI :" << "," << (stImageStats.dAvgAspectRatio_POI) << "," << ","
				 << "DilutionFactor:" << "," << map_ConfigParams[CP::eDilutionFactor] << endl;
				
			file << "AvgRoundness_GP :" << "," << (stImageStats.dAvgRoundness_GP) << "," << ","
				 << "AvgRoundness_POI :" << "," << (stImageStats.dAvgRoundness_POI) << ","<<","
				 << "Concentration_Slope(M):" << "," << map_ConfigParams[CP::eConcentrationSlope] << endl;
			
			file << "AvgRawCellSpotBrightness_GP :" << "," << (stImageStats.dAvgRawCellSpotBrightness_GP) << "," << ","
				 << "AvgRawCellSpotBrightness_POI :" << "," << (stImageStats.dAvgRawCellSpotBrightness_POI) << "," << ","
				 << "Concentration_Intercept(B):" << "," << map_ConfigParams[CP::eConcentrationIntercept] << endl;

			file << "AvgCellSpotBrightness_GP :" << "," << (stImageStats.dAvgCellSpotBrightness_GP) << "," << ","
				 << "AvgCellSpotBrightness_POI :" << "," << (stImageStats.dAvgCellSpotBrightness_POI) << "," << ","
				 << "ImageControlCount:" << "," << map_ConfigParams[CP::eConcentrationImageControlCount] << endl;

			file << "Avgbackground Intensity:" << "," << stImageStats.dAvgBackgroundIntensity << "," << "," << "," << "," << ","
				 << "BubbleMinSpotAreaPercent:" << "," << map_ConfigParams[CP::eBubbleMinSpotAreaPercent] << endl;

			file << "TotalBubble Count :" << "," << stImageStats.nTotalBubbleCount << "," << "," << "," << "," << ","
				 << "BubbleMinSpotAvgBrightness:" << "," << map_ConfigParams[CP::eBubbleMinSpotAvgBrightness] << endl;

			file << "nLargeClusterCount :" << "," << stImageStats.nLargeClusterCount << "," << "," << "," << "," << ","
				 << "BubbleRejectionImgAreaPercent:" << "," << map_ConfigParams[CP::eBubbleRejectionImgAreaPercent] << endl;
		   
			file << "," << "," << "," << "," << "," << "," 
				 << "NominalBackgroundLevel:" << "," << map_ConfigParams[CP::eNominalBackgroundLevel] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "BackgroundIntensityTolerance:" << "," << map_ConfigParams[CP::eBackgroundIntensityTolerance] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "CenterSpotMinIntensityLimit:" << "," << map_ConfigParams[CP::eCenterSpotMinIntensityLimit] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "PeakIntensitySelectionAreaLimit:" << "," << map_ConfigParams[CP::ePeakIntensitySelectionAreaLimit] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "CellSpotBrightnessExclusionThreshold:" << "," << map_ConfigParams[CP::eCellSpotBrightnessExclusionThreshold] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "HotPixelEliminationMode:" << "," << map_ConfigParams[CP::eHotPixelEliminationMode] << endl;

			file << "," << "," << "," << "," << "," << "," 
				 << "ImageBoundaryAnnotation:" << "," << map_ConfigParams[CP::eImgBottomAndRightBoundaryAnnotationMode] << endl;
	  
			file << "," << "," << "," << "," << "," << "," 
				 << "SmallParticleSizingCorrection:" << "," << map_ConfigParams[CP::eSmallParticleSizingCorrection] << endl;

			file << endl << endl;

			file << "Cell Identification Parameters:" << "," << "Threshold" << "," << "Polarity" << endl;
			for (const auto& v : oCellCountingOutputParam.Processing_Settings.v_CellIdentificationParameters)
			{
				const std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(std::get<0>(v));
				file << label << "," << std::get<1>(v) << ","  << ((std::get<2>(v)) != 0 ? "eATABOVE" : "eBELOW") << endl;
			}
			file << endl;

			file << "POI Identification Parameters:" << "," << "Threshold" << "," << "Polarity" << endl;
			const v_IdentificationParams_t& POIIdentificationParams = oCellCountingOutputParam.Processing_Settings.v_POIIdentificationParameters;

			for (const auto& v : oCellCountingOutputParam.Processing_Settings.v_POIIdentificationParameters)
			{
				const std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(std::get<0>(v));
				file << label << "," << std::get<1>(v) << "," << ((std::get<2>(v)) != 0 ? "eATABOVE" : "eBELOW") << endl;
			}
			file << endl << endl;

			file << "Blob Number" << ",";
			auto v_pBlobData = oCellCountingOutputParam.map_Blobs_for_image[nImageNumber];
			if (!v_pBlobData->empty())
			{
				for (auto& v : v_pBlobData->at(0).map_Characteristics)
				{
					std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(v.first);
					if (label.find("FL") == std::string::npos) {
						file << label << ",";
					}
				}
			}

			if (!oCellCountingOutputParam.map_MaxNoOfPeaksFLChannel[nImageNumber].empty())
			{
				int nMaxSubPeaks(0);
				for (auto& v : oCellCountingOutputParam.map_MaxNoOfPeaksFLChannel[nImageNumber])
				{
					auto nFLChannel = static_cast<uint16_t>(v.first);
					std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLAvgIntensity, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLCellArea, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakCount, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLPeakAvgIntensity, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLPeakArea, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLPeakLoc_X, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLPeakLoc_Y, nFLChannel, 0));
					file << label << ",";
					label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLPeakDistance, nFLChannel, 0));
					file << label << ",";
					
					nMaxSubPeaks = v.second;
					for (int nSubPeaks = 1; nSubPeaks < (nMaxSubPeaks + 1); nSubPeaks++)
					{
						label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakPixelCount, nFLChannel, nSubPeaks));
						file << label << ",";
						label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakAvgIntensity, nFLChannel, nSubPeaks));
						file << label << ",";
						label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakArea, nFLChannel, nSubPeaks));
						file << label << ",";
						label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakLoc_X, nFLChannel, nSubPeaks));
						file << label << ",";
						label = ImageAnalysisUtilities::GetCharacteristicLabel(FLCharacteristic(FLSubPeakLoc_Y, nFLChannel, nSubPeaks));
						file << label << ",";
					}
				}
			}
			file << endl;

			if (!v_pBlobData->empty())
			{
				int nItr = 0;
				for (auto v_BlobItr = v_pBlobData->begin(); v_BlobItr != v_pBlobData->end(); ++v_BlobItr, nItr++)
				{
					file << nItr << ","
						 << v_BlobItr->map_Characteristics[Characteristic(IsCell)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(IsPOI)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(IsBubble)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(IsDeclustered)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(IsIrregular)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Area)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(DiameterInPixels)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(DiameterInMicrons)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Circularity)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Sharpness)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Perimeter)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Eccentricity)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(AspectRatio)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(MinorAxis)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(MajorAxis)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Volume)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Roundness)] << ","
						 << v_BlobItr->map_Characteristics[Characteristic(Intrl_MinEnclosedArea_InPixels)] << ",";

					if (0 == v_BlobItr->map_Characteristics[Characteristic(Elimination)]) 
					{
						file << "" << ",";
					}
					else
					{
						const uint16_t value = static_cast<uint16_t>(v_BlobItr->map_Characteristics[Characteristic(Elimination)]);
						const std::string label = ImageAnalysisUtilities::GetCharacteristicLabel(Characteristic(value));
						file << label << ",";
					}

					file << v_BlobItr->map_Characteristics[Characteristic(CellSpotArea)] << ","
						<< v_BlobItr->map_Characteristics[Characteristic(AvgSpotBrightness)] << ","
						<< v_BlobItr->map_Characteristics[Characteristic(RawCellSpotBrightness)] << ",";

					if (!oCellCountingOutputParam.map_MaxNoOfPeaksFLChannel[nImageNumber].empty())
					{
						int nMaxSubPeaks(0);
						auto FLChanneltr = oCellCountingOutputParam.map_MaxNoOfPeaksFLChannel[nImageNumber].begin();
						for (; FLChanneltr != oCellCountingOutputParam.map_MaxNoOfPeaksFLChannel[nImageNumber].end(); ++FLChanneltr)
						{
							auto nFLChannel = static_cast<uint16_t>(FLChanneltr->first);
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLAvgIntensity, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLCellArea, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakCount, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLPeakAvgIntensity, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLPeakArea, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLPeakLoc_X, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLPeakLoc_Y, nFLChannel, 0)] << ",";
							file << v_BlobItr->map_Characteristics[FLCharacteristic(FLPeakDistance, nFLChannel, 0)] << ",";
							
							nMaxSubPeaks = FLChanneltr->second;
							for (int nSubPeaks = 1; nSubPeaks < (nMaxSubPeaks + 1); nSubPeaks++)
							{
								file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakPixelCount, nFLChannel, nSubPeaks)] << ",";
								file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakAvgIntensity, nFLChannel, nSubPeaks)] << ",";
								file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakArea, nFLChannel, nSubPeaks)] << ",";
								file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakLoc_X, nFLChannel, nSubPeaks)] << ",";
								file << v_BlobItr->map_Characteristics[FLCharacteristic(FLSubPeakLoc_Y, nFLChannel, nSubPeaks)] << ",";
							}
						}
					}
					file << endl;
				}
			}
		}
		file.close();
	}	
}
