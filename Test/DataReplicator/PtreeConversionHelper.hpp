#pragma once

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "ResultDefinitionDLL_scout.hpp"
#include "SignatureDLL.hpp"
//#include "Logger.hpp"

namespace pt = boost::property_tree;
typedef pt::basic_ptree<std::string, std::string>::assoc_iterator pt_assoc_it;


//Metadata XML node strings
static const std::string UUID_NODE = "UUID";
static const std::string USER_NAME_NODE = "User";
static const std::string timestamp_NODE = "TimeStamp";
static const std::string LABEL_NODE = "Label";
static const std::string INDEX_NODE = "Index";

// Wq Nodes
static std::string WORKQUEUE_NODE = "WorkQueue";
// Wqi Nodes
static const std::string WORKQUEUE_ITEM_NODE = "WorkQueueItem";
static const std::string SAMPLE_ID_NODE = "SampleId";
static const std::string BP_QC_IDENT_NODE = "BpQcIdentifier";
static const std::string DILUTION_FACTOR_NODE = "DilutionFactor";
static const std::string WASH_NODE = "Wash";
static const std::string COMMENT_NODE = "Comment";
//Cell type Nodes
static const std::string CELLTYPE_NODE = "Celltype";
static const std::string MAX_IMAGE_COUNT_NODE = "MaxImageCount";
static const std::string ASPIRATION_CYCLES_NODE = "AspirationCycles";
static const std::string ROI_X_PIXELS_NODE = "ROI_X_Pixels";
static const std::string ROI_Y_PIXELS_NODE = "ROI_Y_Pixels";
static const std::string MIN_DIA_UM_NODE = "MinDia";
static const std::string MAX_DIA_UM_NODE = "MaxDia";
static const std::string MIN_CIRCULARITY_NODE = "MinCircularity";
static const std::string SHARPNESS_LIMIT_NODE = "SharpnessLimit";
static const std::string DECLUSTER_SETTING_NODE = "DeclusterSetting";
static const std::string FL_ROI_EXTENT_NODE = "Fl_ROI_Extent";
// Analysis Def Nodes
static const std::string ANALYSIS_DEF_NODE = "AnalysisDef";
static const std::string REAGENT_INDICES_NODE = "ReagentIndices";
static const std::string MIXING_CYCLES_NODE = "MixingCycles";
static const std::string POP_PARAM_NODE = "PopulationParam";
//Analysis parameter Nodes
static const std::string ANALYSIS_PARAM_NODE = "AnalaysisParam";
static const std::string THRESHOLD_VALUE_NODE = "ThresholdValue";
static const std::string ABOVE_THRESHOLD_NODE = "AboveThreshold";
//FL_IlluminationSettings Nodes
static const std::string FL_ILLUMINATION_SETTINGS_NODE = "FlIlluminationSettings";
static const std::string IILUMINATOR_WL_NODE = "IlluminatorWavelength";
static const std::string EMISSION_WL_NODE = "EmissionWavelength";
static const std::string EXPOSURE_TIME_NODE = "ExposureTime";
//Characteristics Nodes
static const std::string CHARACTERISTIC_NODE = "Characteristic";
static const std::string KEY_NODE = "Key";
static const std::string SUB_KEY_NODE = "SubKey";
static const std::string SUB_SUB_KEY_NODE = "SubSubKey";
// Reagent info Nodes
static const std::string REAGENT_INFO_NODE = "ReagentInfo";
static const std::string PACK_NUM_NODE = "PackNumber";
static const std::string LOT_NUM_NODE = "LotNumber";
static const std::string EXP_DATE_NODE = "ExpDate";
static const std::string IN_SERVICE_DATE_NODE = "InServiceDate";
static const std::string EFF_EXP_DATE_NODE = "EffectiveExpDate";
// Result summary Nodes
static const std::string RESULT_NODE = "Result";
static const std::string PATH_NODE = "Path";
static const std::string EPROCESSEDSTATUS_NODE = "eProcessedStatus";
static const std::string TOTALCUMULATIVE_IMGS_NODE = "TotalNumImages";
static const std::string COUNT_POP_GENERAL_NODE = "POPGeneralCount";
static const std::string COUNT_POP_OFINTEREST_NODE = "POICount";
static const std::string CONCENTRATION_GENERAL_NODE = "ConcenGeneral";
static const std::string CONCENTRATION_OFINTEREST_NODE = "ConcenOfInterest";
static const std::string PERCENT_POP_OFINTEREST_NODE = "POIPercent";
static const std::string AVG_DIAMETER_POP_NODE = "POPAvgDiameter";
static const std::string AVG_DIAMETER_OFINTEREST_NODE = "AvgDiameter";
static const std::string AVG_CIRCULARITY_POP_NODE = "POPAvgCircularity";
static const std::string AVG_CIRCULARITY_OFINTEREST_NODE = "AvgCircularity";
static const std::string COEFFICIENT_VARIANCE_NODE = "CoeffiVar";
static const std::string AVERAGE_CELLS_PER_IMAGE_NODE = "AvgCellsPerImage";
static const std::string AVERAGE_BRIGHTFIELD_BG_INTENSITY_NODE = "AvgBfBgIntensity";
static const std::string BUBBLE_COUNT_NODE = "BubbleCount";
static const std::string LARGE_CLUSTER_COUNT_NODE = "LargeClusterCount";

// Signature Nodes
static const std::string SIGNATURE_NODE = "Signature";
static const std::string LONG_TEXT_NODE = "LongText";
static const std::string SHORT_TEXT_NODE = "ShortText";

//Image set nodes
static const std::string IMAGE_SET_NODE = "ImageSet";
static const std::string IMAGE_SEQ_NUM_NODE = "ImageSeqNum";
static const std::string BF_IMAGE_NODE = "BFImage";
static const std::string FL_IMAGE_NODE = "FLImage";
static const std::string FL_CHANNEL_NODE = "FLChannel";

// Normalization Image Set node
static const std::string NORMALIZATION_IMAGE_SET_NODE = "NormalizationImageSet";



namespace PtreeConversionHelper
{
	/**************************************************************************************************/
	bool ConvertToPtree(const WorkQueueRecordDLL& wqr, pt::ptree& wqr_node_tree);
	bool ConvertToPtree(const SampleRecordDLL& wqir, pt::ptree& wqir_node_tree);
	bool ConvertToPtree(const SampleImageSetRecordDLL& imsr, const ImageRecordDLL& imr, const std::vector<ImageRecordDLL>& fl_imr_list, pt::ptree& imsr_node_tree);
	bool ConvertToPtree(const ImageRecordDLL & imr, pt::ptree & imr_node_tree);
	bool ConvertToPtree(const ImageRecordDLL& imr, const SampleImageSetRecordDLL& imsr, pt::ptree& imr_node_tree); // FL image
	bool ConvertToPtree(const ResultSummaryDLL& rr, pt::ptree& rr_node_tree);
	/*---------------------------------------------------------------*/
	void ConvertToPtree(const CellTypeDLL& ct, pt::ptree& ct_node_tree);
	void ConvertToPtree(const AnalysisDefinitionDLL& ad, pt::ptree& ad_node_tree);
	void ConvertToPtree(const AnalysisParameterDLL& ap, pt::ptree& ap_node_tree);
	void ConvertToPtree(const DataSignatureInstanceDLL& sig, pt::ptree& sig_node_tree);
	/**************************************************************************************************/
	bool ConvertToStruct(const pt_assoc_it&  wqr_it, WorkQueueRecordDLL& wqr);
	bool ConvertToStruct(const pt_assoc_it&  wqir_it, SampleRecordDLL& wqir);
	bool ConvertToStruct(const pt_assoc_it&  imsr_it, SampleImageSetRecordDLL& imsr);
	bool ConvertToStruct(const pt_assoc_it&  imr_it, ImageRecordDLL& imr);
	bool ConvertToStruct(const pt_assoc_it&  rr_it, ResultSummaryDLL& rr);
	/*---------------------------------------------------------------*/
	void ConvertToStruct(const pt_assoc_it&  ct_it, CellTypeDLL& ct);
	void ConvertToStruct(const pt_assoc_it&  ad_it, AnalysisDefinitionDLL& ad);
	void ConvertToStruct(const pt_assoc_it&  ap_it, AnalysisParameterDLL& ap);
	void ConvertToStruct(const pt_assoc_it&  sig_it, DataSignatureInstanceDLL& sig);
	/**************************************************************************************************/
}