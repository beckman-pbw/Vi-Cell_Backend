#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "CompletionHandlerUtilities.hpp"
#include "ImageAnalysisUtilities.hpp"

class ImageProcessingUtilities
{
public:
	static void initialize (std::shared_ptr<boost::asio::io_context> pImgProcessingIoSvc);
	
	static ICompletionHandler& generateDustImage (std::vector<cv::Mat> v_oMatInputImgs, cv::Mat & dustImage);
	static ICompletionHandler& CellCounterReanalysis(
		const AnalysisDefinitionDLL& analysisDefinition,
		const CellTypeDLL& cellType,
		const CellCounterResult::SResult& orgSresult,
		CellCounterResult::SResult& reanalysisSResult);
	static ICompletionHandler& CellCounterReanalysis(
		std::shared_ptr<ImageCollection_t> image_collection,
		uint16_t dilution_factor,
		const ImageSet_t& background_normalization_images,
		const AnalysisDefinitionDLL& analysis_definition, 
		const CellTypeDLL& cellType,
		const CellCounterResult::SResult& orgSresult,
		CellCounterResult::SResult& reanalysisSResult);
	static ICompletionHandler& sharpness (const cv::Mat& image, double& variance);
	static ICompletionHandler& generateHistogram (const cv::Mat& image, std::string histogramPath, bool & success);
	static ICompletionHandler& getHistogramWhiteCount (const cv::Mat & image, int32_t * whiteCount, bool & success);
	static ICompletionHandler& colorizeImage (const cv::Mat & grayImage, cv::Mat & colorizedImage, bool & success);
	static void showImage (std::string title, const CvArr* image);

private:
	//std::shared_ptr<boost::asio::io_context> pImgProcessingIoSvc_;
	//std::shared_ptr<boost::asio::deadline_timer> p_ReanalysisTimer;
	enum class reanalysisState
	{
		rs_Wait,
		rs_Complete,
		rs_Error,
		rs_Cancelled,
	};

	static void internal_cellCounterReanalysis(
		const boost::system::error_code& ec,
		const CellTypeDLL& cellType,
		reanalysisState state,
		uint64_t ch_Index, 
		CellCounterResult::SResult& reanalysisOutput);
};
