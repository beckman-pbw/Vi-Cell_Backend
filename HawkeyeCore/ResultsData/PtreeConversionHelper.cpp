#include "stdafx.h"

#include "SampleSetDLL.hpp"

#include "HawkeyeUUID.hpp"
#include "Logger.hpp"
#include "PtreeConversionHelper.hpp"

static const std::string MODULENAME = "MetaDataHandler";

namespace PtreeConversionHelper
{
	//*****************************************************************************
	// ConvertToPtree APIs.
	//*****************************************************************************
	bool ConvertToPtree(const WorklistRecordDLL& wqr, pt::ptree& wqr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: WorklistRecordDLL: <Enter>");
		wqr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (wqr.uuid);
		wqr_node_tree.add(UUID_NODE, uuid_str);
		wqr_node_tree.add(USER_NODE, wqr.username);
		wqr_node_tree.add(LABEL_NODE, wqr.label);

		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(wqr.timestamp);
		wqr_node_tree.add(TIMESTAMP_NODE, time_in_secs);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: WorklistRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToPtree (const SampleSetDLL& sampleSet, pt::ptree& sampleSetNode)
	{
		sampleSetNode.clear();
		sampleSetNode.add (UUID_NODE, Uuid::ToStr (sampleSet.uuid));
		sampleSetNode.add (USER_NODE, sampleSet.username);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(sampleSet.timestamp);
		sampleSetNode.add (TIMESTAMP_NODE, time_in_secs);
		sampleSetNode.add (SAMPLESET_ID_NODE, sampleSet.name);
		sampleSetNode.add (TAG_NODE, sampleSet.setLabel);
		auto temp = static_cast<int>(sampleSet.status);
		sampleSetNode.add (STATUS_NODE, temp);

		return true;
	}

	//*****************************************************************************
	bool ConvertToPtree(const SampleRecordDLL& wqir, pt::ptree& wqir_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: SampleRecordDLL: <Enter>");

		wqir_node_tree.clear();
		std::string wqi_uuid_str = Uuid::ToStr (wqir.uuid);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(wqir.timestamp);

		wqir_node_tree.add(UUID_NODE, wqi_uuid_str);
		wqir_node_tree.add(USER_NODE, wqir.username);
		wqir_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		wqir_node_tree.add(SAMPLE_ID_NODE, wqir.sample_identifier);
		wqir_node_tree.add(BP_QC_IDENT_NODE, wqir.bp_qc_identifier);
		wqir_node_tree.add(DILUTION_FACTOR_NODE, wqir.dilution_factor);
		wqir_node_tree.add(WASH_NODE, wqir.wash);
		wqir_node_tree.add(COMMENT_NODE, wqir.comment);
		auto posStr = boost::str(boost::format("%s-%d") %  wqir.sample_position.getRow() % (int)wqir.sample_position.getColumn());
		wqir_node_tree.add(POSITION_NODE, posStr);

		for (const auto& rinfo : wqir.reagent_info_records)
		{
			pt::ptree  reagent_info_tree = {};
			reagent_info_tree.add(PACK_NUM_NODE, rinfo.pack_number);
			reagent_info_tree.add(LOT_NUM_NODE, rinfo.lot_number);
			reagent_info_tree.add(LABEL_NODE, rinfo.reagent_label);
			reagent_info_tree.add(EXP_DATE_NODE, rinfo.expiration_date);
			reagent_info_tree.add(IN_SERVICE_DATE_NODE, rinfo.in_service_date);
			reagent_info_tree.add(EFF_EXP_DATE_NODE, rinfo.effective_expiration_date);

			wqir_node_tree.add_child(REAGENT_INFO_NODE, reagent_info_tree);
		}
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: SampleRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool isNILImageRecord(const ImageRecordDLL& imr)
	{
		return Uuid::IsClear(imr.uuid);
	}

	//*****************************************************************************
	bool ConvertNormalizationImageDataToPtree(const SampleImageSetRecordDLL& imsr, const ImageRecordDLL& imr, const std::vector<ImageRecordDLL>& fl_imr_list, pt::ptree& imsr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertNoramlizationImageDataToPtree: SampleImageSetRecordDLL: <Enter>");

		//If Image set is null , then no point in creating metadata for this image set.
		if (Uuid::IsClear(imsr.uuid))
			return false;

		// If both  BF normalization image data and Fl normalization data is empty
		if (isNILImageRecord(imr) && fl_imr_list.empty())
			return false;

		//Either BF or FL normalization data is empty but not both, it is okay we will create the meta data 
		imsr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (imsr.uuid);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(imsr.timestamp);

		imsr_node_tree.add(UUID_NODE, uuid_str);
		imsr_node_tree.add(USER_NODE, imsr.username);
		imsr_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		imsr_node_tree.add(IMAGE_SEQ_NUM_NODE, imsr.sequence_number);

		//Add BF image, if available
		pt::ptree imr_node_tree = {};
		if (!isNILImageRecord(imr)) 
		{
			if (!ConvertToPtree(imr, imr_node_tree))
				return false;
			imsr_node_tree.add_child(BF_IMAGE_NODE, imr_node_tree);
		}

		//Add Fl images if available
		for (const auto& fl_imr : fl_imr_list)
		{
			imr_node_tree.clear();
			if (!ConvertToPtree(fl_imr, imsr, imr_node_tree))
				return false;

			imsr_node_tree.add_child(FL_IMAGE_NODE, imr_node_tree);
		}
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertNoramlizationImageDataToPtree: SampleImageSetRecordDLL: <Exit>");

		return true;
	}

	//*****************************************************************************
	bool ConvertToPtree(const SampleImageSetRecordDLL& imsr, const ImageRecordDLL& imr, const std::vector<ImageRecordDLL>& fl_imr_list, pt::ptree& imsr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: SampleImageSetRecordDLL: <Enter>");

		imsr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (imsr.uuid);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(imsr.timestamp);

		imsr_node_tree.add(UUID_NODE, uuid_str);
		imsr_node_tree.add(USER_NODE, imsr.username);
		imsr_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		imsr_node_tree.add(IMAGE_SEQ_NUM_NODE, imsr.sequence_number);

		//Add BF image
		pt::ptree imr_node_tree = {};
		if (!ConvertToPtree(imr, imr_node_tree))
			return false;
		imsr_node_tree.add_child(BF_IMAGE_NODE, imr_node_tree);

		//Add Fl images
		for (const auto& fl_imr : fl_imr_list)
		{
			imr_node_tree.clear();
			if (!ConvertToPtree(fl_imr, imsr, imr_node_tree))
				return false;
			
			imsr_node_tree.add_child(FL_IMAGE_NODE, imr_node_tree);
		}
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: SampleImageSetRecordDLL: <Exit>");

		return true;
	}

	//*****************************************************************************
	// FL images
	//*****************************************************************************
	bool ConvertToPtree(const ImageRecordDLL& imr, const SampleImageSetRecordDLL& imsr, pt::ptree& imr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ImageRecordDLL (FL image): <Enter>");

		imr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (imr.uuid);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(imr.timestamp);

		imr_node_tree.add(UUID_NODE, uuid_str);
		imr_node_tree.add(USER_NODE, imr.username);
		imr_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		imr_node_tree.add(PATH_NODE, imr.path);

		bool foundFL = false;
		for (auto FL : imsr.flImagesAndChannelNumlist)
		{
			std::string uuid_str2 = Uuid::ToStr (FL.second);
			if (uuid_str == uuid_str2)
			{
				imr_node_tree.add(FL_CHANNEL_NODE, FL.first);
				foundFL = true;
				break;
			}
		}
		if (!foundFL)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToPtree: ImageRecordDLL (FL image): <Exit Could not match up UUID to FL channel list>");
			return false;
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ImageRecordDLL (FL image): <Exit>");

		return true;
	}

	//*****************************************************************************
	bool ConvertToPtree(const ImageRecordDLL & imr, pt::ptree & imr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ImageRecordDLL: <Enter>");

		imr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (imr.uuid);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(imr.timestamp);

		imr_node_tree.add(UUID_NODE, uuid_str);
		imr_node_tree.add(USER_NODE, imr.username);
		imr_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		imr_node_tree.add(PATH_NODE, imr.path);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ImageRecordDLL: <Exit>");

		return true;
	}

	//*****************************************************************************
	bool ConvertToPtree(const ResultSummaryDLL& rs, pt::ptree& rr_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ResultRecordDLL: <Enter>");

		rr_node_tree.clear();
		std::string uuid_str = Uuid::ToStr (rs.uuid);

		rr_node_tree.add(UUID_NODE, uuid_str);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(rs.timestamp);
		rr_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		rr_node_tree.add(PATH_NODE, rs.path);

		//BasicResultAnswer
		rr_node_tree.add(EPROCESSEDSTATUS_NODE, rs.cumulative_result.eProcessedStatus);
		rr_node_tree.add(TOTALCUMULATIVE_IMGS_NODE, rs.cumulative_result.nTotalCumulative_Imgs);
		rr_node_tree.add(COUNT_POP_GENERAL_NODE, rs.cumulative_result.count_pop_general);
		rr_node_tree.add(COUNT_POP_OFINTEREST_NODE, rs.cumulative_result.count_pop_ofinterest);
		rr_node_tree.add(CONCENTRATION_GENERAL_NODE, rs.cumulative_result.concentration_general);
		rr_node_tree.add(CONCENTRATION_OFINTEREST_NODE, rs.cumulative_result.concentration_ofinterest);
		rr_node_tree.add(PERCENT_POP_OFINTEREST_NODE, rs.cumulative_result.percent_pop_ofinterest);
		rr_node_tree.add(AVG_DIAMETER_POP_NODE, rs.cumulative_result.avg_diameter_pop);
		rr_node_tree.add(AVG_DIAMETER_OFINTEREST_NODE, rs.cumulative_result.avg_diameter_ofinterest);
		rr_node_tree.add(AVG_CIRCULARITY_POP_NODE, rs.cumulative_result.avg_circularity_pop);
		rr_node_tree.add(AVG_CIRCULARITY_OFINTEREST_NODE, rs.cumulative_result.avg_circularity_ofinterest);
		rr_node_tree.add(COEFFICIENT_VARIANCE_NODE, rs.cumulative_result.coefficient_variance);
		rr_node_tree.add(AVERAGE_CELLS_PER_IMAGE_NODE, rs.cumulative_result.average_cells_per_image);
		rr_node_tree.add(AVERAGE_BRIGHTFIELD_BG_INTENSITY_NODE, rs.cumulative_result.average_brightfield_bg_intensity);
		rr_node_tree.add(BUBBLE_COUNT_NODE, rs.cumulative_result.bubble_count);
		rr_node_tree.add(LARGE_CLUSTER_COUNT_NODE, rs.cumulative_result.large_cluster_count);
		rr_node_tree.add(QCSTATUS_NODE, static_cast<uint16_t>(rs.qcStatus));

		pt::ptree ct_node_tree = {};
		ConvertToPtree (rs.cell_type_settings, ct_node_tree);
		pt::ptree ad_node_tree = {};
		ConvertToPtree (rs.analysis_settings, ad_node_tree);

		pt::ptree sigs_node_tree = {};

		for (const auto& sig : rs.signature_set)
		{
			pt::ptree sig_node_tree = {};
			ConvertToPtree (sig, sig_node_tree);
			sigs_node_tree.add_child(SIGNATURE_NODE, sig_node_tree);
		}

		rr_node_tree.add_child (SIGNATURES_NODE, sigs_node_tree);
		rr_node_tree.add_child(CELLTYPE_NODE, ct_node_tree);
		rr_node_tree.add_child(ANALYSIS_DEF_NODE, ad_node_tree);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: ResultRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	void ConvertToPtree(const CellTypeDLL& ct, pt::ptree& ct_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: CellTypeDLL: <Enter>");

		ct_node_tree.clear();
		ct_node_tree.add(INDEX_NODE, ct.celltype_index);
		ct_node_tree.add(LABEL_NODE, ct.label);
		ct_node_tree.add(MAX_IMAGE_COUNT_NODE, ct.max_image_count);
		ct_node_tree.add(ASPIRATION_CYCLES_NODE, ct.aspiration_cycles);
		ct_node_tree.add(ROI_X_PIXELS_NODE, ct.roi_x_pixels);
		ct_node_tree.add(ROI_Y_PIXELS_NODE, ct.roi_y_pixels);
		ct_node_tree.add(MIN_DIA_UM_NODE, ct.minimum_diameter_um);
		ct_node_tree.add(MAX_DIA_UM_NODE, ct.maximum_diameter_um);
		ct_node_tree.add(MIN_CIRCULARITY_NODE, ct.minimum_circularity);
		ct_node_tree.add(SHARPNESS_LIMIT_NODE, ct.sharpness_limit);
		ct_node_tree.add(DECLUSTER_SETTING_NODE, ct.decluster_setting);
		ct_node_tree.add(FL_ROI_EXTENT_NODE, ct.fl_roi_extent);
		ct_node_tree.add(CONC_ADJUSTMENT_FACTOR_NODE, ct.concentration_adjustment_factor);
		ct_node_tree.add(RETIRED_NODE, ct.retired);

		for (const auto& ap : ct.cell_identification_parameters)
		{
			pt::ptree ap_node_tree = {};
			ConvertToPtree(ap, ap_node_tree);
			ct_node_tree.add_child(ANALYSIS_PARAM_NODE, ap_node_tree);
		}
		//NOTE : Do not store analysis definition here, ad information available in the cell type consists of all possible ads for this particular celltype which is redundant.
		//and we need only one ad, which is used for sample analysis and it is saved separately

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: CellTypeDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToPtree(const AnalysisDefinitionDLL& ad, pt::ptree& ad_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: AnalysisDefinitionDLL: <Enter>");

		ad_node_tree.clear();
		ad_node_tree.add(INDEX_NODE, ad.analysis_index);
		ad_node_tree.add(LABEL_NODE, ad.label);

		for (const auto& ri : ad.reagent_indices)
		{
			pt::ptree ri_node_tree = {};
			ri_node_tree.add(INDEX_NODE, ri);

			ad_node_tree.add_child(REAGENT_INDICES_NODE, ri_node_tree);
		}
		ad_node_tree.add(MIXING_CYCLES_NODE, ad.mixing_cycles);

		//fl_illuminators
		for (const auto& fls : ad.fl_illuminators)
		{
			pt::ptree fls_node_tree = {};
			fls_node_tree.add(IILUMINATOR_WL_NODE, fls.illuminator_wavelength_nm);
			fls_node_tree.add(EMISSION_WL_NODE, fls.emission_wavelength_nm);
			fls_node_tree.add(EXPOSURE_TIME_NODE, fls.exposure_time_ms);

			ad_node_tree.add_child(FL_ILLUMINATION_SETTINGS_NODE, fls_node_tree);
		}

		//analysis_parameters
		for (const auto& ap : ad.analysis_parameters)
		{
			pt::ptree ap_node_tree = {};
			ConvertToPtree(ap, ap_node_tree);
			ad_node_tree.add_child(ANALYSIS_PARAM_NODE, ap_node_tree);
		}

		if (ad.population_parameter != boost::none)
		{
			auto ap = ad.population_parameter.get();
			pt::ptree ap_node_tree = {};
			ConvertToPtree(ap, ap_node_tree);
			ad_node_tree.add_child(POP_PARAM_NODE, ap_node_tree);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: AnalysisDefinitionDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToPtree(const AnalysisParameterDLL& ap, pt::ptree& ap_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: AnalysisParameterDLL: <Enter>");

		ap_node_tree.clear();
		pt::ptree ch_node_tree = {};
		ch_node_tree.add(KEY_NODE, ap.characteristic.key);
		ch_node_tree.add(SUB_KEY_NODE, ap.characteristic.s_key);
		ch_node_tree.add(SUB_SUB_KEY_NODE, ap.characteristic.s_s_key);

		ap_node_tree.add(LABEL_NODE, ap.label);
		ap_node_tree.add_child(CHARACTERISTIC_NODE, ch_node_tree);
		ap_node_tree.add(THRESHOLD_VALUE_NODE, ap.threshold_value);
		ap_node_tree.add(ABOVE_THRESHOLD_NODE, ap.above_threshold);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: AnalysisParameterDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToPtree(const DataSignatureInstanceDLL& sig, pt::ptree& sig_node_tree)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: DataSignatureInstanceDLL: <Enter>");

		sig_node_tree.clear();
		sig_node_tree.add(USER_NODE, sig.signing_user);
		uint64_t time_in_secs = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(sig.timestamp);
		sig_node_tree.add(TIMESTAMP_NODE, time_in_secs);
		sig_node_tree.add(SHORT_TEXT_NODE, sig.signature.short_text);
		sig_node_tree.add(SHORT_TEXT_SIGNATURE_NODE, sig.signature.short_text_signature);
		sig_node_tree.add(LONG_TEXT_NODE, sig.signature.long_text);
		sig_node_tree.add(LONG_TEXT_SIGNATURE_NODE, sig.signature.long_text_signature);
		sig_node_tree.add(SIGNATURE_HASH_NODE, sig.signature.sig_hash);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToPtree: DataSignatureInstanceDLL: <Exit>");
	}

	//*****************************************************************************
	// ConvertToStruct APIs.
	//*****************************************************************************
	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it& wqr_it, InstrumentRecordDLL& instr)
	{
		Logger::L().Log(MODULENAME, severity_level::debug3, "ConvertToStruct: InstrumentRecordDLL: <Enter>");

		instr.version = wqr_it->second.get<std::string>(VERSION_NODE);

		Logger::L().Log(MODULENAME, severity_level::debug3, "ConvertToStruct: InstrumentRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it&  wqr_it, WorklistRecordDLL& wqr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: WorklistRecordDLL: <Enter>");

		std::string uuid_str = wqr_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, wqr.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: WorklistRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}

		uint64_t time_in_secs = wqr_it->second.get<std::uint64_t>(TIMESTAMP_NODE);
		wqr.username = wqr_it->second.get<std::string>(USER_NODE);
		wqr.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: WorklistRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct (const pt_assoc_it& sampleSet_it, SampleSetDLL& sampleSet)
	{
		auto uuid_str = sampleSet_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, sampleSet.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: SampleRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}
		sampleSet.username = sampleSet_it->second.get<std::string>(USER_NODE);
		uint64_t time_in_secs = sampleSet_it->second.get<std::uint64_t>(TIMESTAMP_NODE);
		sampleSet.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);
		sampleSet.name = sampleSet_it->second.get<std::string>(SAMPLESET_ID_NODE);
		sampleSet.setLabel = sampleSet_it->second.get<std::string>(TAG_NODE);		
		auto temp = sampleSet_it->second.get<std::string>(STATUS_NODE);
		sampleSet.status = static_cast<DBApi::eSampleSetStatus>(std::atoi(temp.c_str()));
		int type = sampleSet_it->second.get<int>(CARRIER_TYPE_NODE);
		sampleSet.carrier = static_cast<eCarrierType>(type);

		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it&  wqir_it, SampleRecordDLL& wqir)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: SampleRecordDLL: <Enter>");

		std::string uuid_str = wqir_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, wqir.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: SampleRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}

		wqir.username = wqir_it->second.get<std::string>(USER_NODE);
		uint64_t time_in_secs = wqir_it->second.get<std::uint64_t>(TIMESTAMP_NODE);
		wqir.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);

		wqir.sample_identifier = wqir_it->second.get<std::string>(SAMPLE_ID_NODE);
		wqir.bp_qc_identifier = wqir_it->second.get<std::string>(BP_QC_IDENT_NODE);
		wqir.dilution_factor = wqir_it->second.get<uint16_t>(DILUTION_FACTOR_NODE);
		wqir.wash = (eSamplePostWash)wqir_it->second.get<uint16_t>(WASH_NODE);
		wqir.comment = wqir_it->second.get<std::string>(COMMENT_NODE);
		
		auto pStr = wqir_it->second.get_optional<std::string>(POSITION_NODE);
		if (pStr)
		{
			SamplePosition position;
			position.FromStr (*pStr);
			wqir.SetPosition(position.row, position.col);
		}

		auto rir_range = wqir_it->second.equal_range(REAGENT_INFO_NODE);
		for (auto rir_it = rir_range.first; rir_it != rir_range.second; rir_it++)
		{
			ReagentInfoRecordDLL rir;
			rir.pack_number = rir_it->second.get<std::string>(PACK_NUM_NODE);
			rir.lot_number = rir_it->second.get<std::uint32_t>(LOT_NUM_NODE);
			rir.reagent_label = rir_it->second.get<std::string>(LABEL_NODE);

			rir.expiration_date = rir_it->second.get<uint64_t>(EXP_DATE_NODE);
			rir.in_service_date = rir_it->second.get<uint64_t>(IN_SERVICE_DATE_NODE);
			rir.effective_expiration_date = rir_it->second.get<uint64_t>(EFF_EXP_DATE_NODE, 0); // Default to ZERO to keep old data (pre-shipment) alive
			 
			wqir.reagent_info_records.push_back(rir);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: SampleRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it&  imsr_it, SampleImageSetRecordDLL& imsr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: SampleImageSetRecordDLL: <Enter>");

		std::string uuid_str = imsr_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, imsr.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: SampleImageSetRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}
		imsr.username = imsr_it->second.get<std::string>(USER_NODE);
		uint64_t time_in_secs = imsr_it->second.get<uint64_t>(TIMESTAMP_NODE);
		imsr.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);
		imsr.sequence_number = imsr_it->second.get<uint16_t>(IMAGE_SEQ_NUM_NODE);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: SampleImageSetRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it&  imr_it, ImageRecordDLL& imr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: ImageRecordDLL: <Enter>");

		std::string uuid_str = imr_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, imr.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: ImageRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}

		uint64_t time_in_secs = imr_it->second.get<uint64_t>(TIMESTAMP_NODE);
		imr.username = imr_it->second.get<std::string>(USER_NODE);
		imr.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);

		imr.path = imr_it->second.get<std::string>(PATH_NODE);

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: ImageRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	bool ConvertToStruct(const pt_assoc_it&  rs_it, ResultSummaryDLL& rs)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: ResultRecordDLL: <Enter>");

		std::string uuid_str = rs_it->second.get<std::string>(UUID_NODE);
		if (!HawkeyeUUID::Getuuid__tFromStr(uuid_str, rs.uuid))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ConvertToStruct: ResultRecordDLL <Exit Failed to Create uuid from string>");
			return false;
		}

		uint64_t time_in_secs = rs_it->second.get<uint64_t>(TIMESTAMP_NODE);
		try {
			rs.username = rs_it->second.get<std::string>(USER_NODE);
		} catch (...) {}
		rs.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);
		rs.path = rs_it->second.get<std::string>(PATH_NODE);
		rs.cumulative_result.eProcessedStatus = (E_ERRORCODE)rs_it->second.get<uint32_t>(EPROCESSEDSTATUS_NODE);
		rs.cumulative_result.nTotalCumulative_Imgs = rs_it->second.get<uint32_t>(TOTALCUMULATIVE_IMGS_NODE);
		rs.cumulative_result.count_pop_general = rs_it->second.get<uint32_t>(COUNT_POP_GENERAL_NODE);
		rs.cumulative_result.count_pop_ofinterest = rs_it->second.get<uint32_t>(COUNT_POP_OFINTEREST_NODE);
		rs.cumulative_result.concentration_general = rs_it->second.get<float>(CONCENTRATION_GENERAL_NODE);
		rs.cumulative_result.concentration_ofinterest = rs_it->second.get<float>(CONCENTRATION_OFINTEREST_NODE);
		rs.cumulative_result.percent_pop_ofinterest = rs_it->second.get<float>(PERCENT_POP_OFINTEREST_NODE);
		rs.cumulative_result.avg_diameter_pop = rs_it->second.get<float>(AVG_DIAMETER_POP_NODE);
		rs.cumulative_result.avg_diameter_ofinterest = rs_it->second.get<float>(AVG_DIAMETER_OFINTEREST_NODE);
		rs.cumulative_result.avg_circularity_pop = rs_it->second.get<float>(AVG_CIRCULARITY_POP_NODE);
		rs.cumulative_result.avg_circularity_ofinterest = rs_it->second.get<float>(AVG_CIRCULARITY_OFINTEREST_NODE);
		rs.cumulative_result.coefficient_variance = rs_it->second.get<float>(COEFFICIENT_VARIANCE_NODE);
		rs.cumulative_result.average_cells_per_image = rs_it->second.get<uint16_t>(AVERAGE_CELLS_PER_IMAGE_NODE);
		rs.cumulative_result.average_brightfield_bg_intensity = rs_it->second.get<uint16_t>(AVERAGE_BRIGHTFIELD_BG_INTENSITY_NODE);
		rs.cumulative_result.bubble_count = rs_it->second.get<uint16_t>(BUBBLE_COUNT_NODE);
		rs.cumulative_result.large_cluster_count = rs_it->second.get<uint16_t>(LARGE_CLUSTER_COUNT_NODE);

		const boost::optional<uint16_t> t_uint = rs_it->second.get_optional<uint16_t>(QCSTATUS_NODE);
		if (t_uint)
		{
			rs.qcStatus = static_cast<QcStatus>(t_uint.get());
		}

		// If there is a drive ID, strip it off.
		if (rs.path[1] == ':') {
			rs.path = std::string (&rs.path[2]);
		}

		auto ct_it = rs_it->second.find(CELLTYPE_NODE);
		if (ct_it != rs_it->second.not_found())
		{
			ConvertToStruct(ct_it, rs.cell_type_settings);
		}

		auto ad_it = rs_it->second.find(ANALYSIS_DEF_NODE);
		if (ad_it != rs_it->second.not_found())
		{
			ConvertToStruct(ad_it, rs.analysis_settings);
		}

		rs.cell_type_settings.analysis_specializations.push_back(rs.analysis_settings);

		// v1.2
		std::pair<pt_assoc_it, pt_assoc_it> sig_range = rs_it->second.equal_range(SIGNATURE_NODE);
		for (auto sig_it = sig_range.first; sig_it != sig_range.second; sig_it++)
		{
			DataSignatureInstanceDLL tmp_sig = {};
			ConvertToStruct(sig_it, tmp_sig);
			rs.signature_set.push_back(tmp_sig);
		}

		// v1.3
		std::pair<pt_assoc_it, pt_assoc_it> sigs_range = rs_it->second.equal_range(SIGNATURES_NODE);
		for (auto sigs_it = sigs_range.first; sigs_it != sigs_range.second; sigs_it++)
		{
			std::pair<pt_assoc_it, pt_assoc_it> sig_range = sigs_it->second.equal_range(SIGNATURE_NODE);
			for (auto sig_it = sig_range.first; sig_it != sig_range.second; sig_it++)
			{
				DataSignatureInstanceDLL tmp_sig = {};
				ConvertToStruct(sig_it, tmp_sig);
				rs.signature_set.push_back(tmp_sig);
			}
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: ResultRecordDLL: <Exit>");
		return true;
	}

	//*****************************************************************************
	void ConvertToStruct(const pt_assoc_it&  ct_it, CellTypeDLL& ct)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: CellTypeDLL: <Enter>");

		ct.celltype_index = ct_it->second.get<uint32_t>(INDEX_NODE);
		ct.label = ct_it->second.get<std::string>(LABEL_NODE);
		ct.max_image_count = ct_it->second.get<uint16_t>(MAX_IMAGE_COUNT_NODE);
		ct.aspiration_cycles = ct_it->second.get<uint8_t>(ASPIRATION_CYCLES_NODE);
		ct.roi_x_pixels = ct_it->second.get<uint16_t>(ROI_X_PIXELS_NODE);
		ct.roi_y_pixels = ct_it->second.get<uint16_t>(ROI_Y_PIXELS_NODE);
		ct.minimum_diameter_um = ct_it->second.get<float>(MIN_DIA_UM_NODE);
		ct.maximum_diameter_um = ct_it->second.get<float>(MAX_DIA_UM_NODE);
		ct.minimum_circularity = ct_it->second.get<float>(MIN_CIRCULARITY_NODE);
		ct.sharpness_limit = ct_it->second.get<float>(SHARPNESS_LIMIT_NODE);
		ct.decluster_setting = (eCellDeclusterSetting)ct_it->second.get<uint16_t>(DECLUSTER_SETTING_NODE);
		ct.fl_roi_extent = ct_it->second.get<float>(FL_ROI_EXTENT_NODE);
		boost::optional<float> tFloat = ct_it->second.get_optional<float>(CONC_ADJUSTMENT_FACTOR_NODE);
		if (tFloat)
		{
			ct.concentration_adjustment_factor = tFloat.get();
		}
		boost::optional<bool> tBool = ct_it->second.get_optional<bool>(RETIRED_NODE);
		if (tBool)
		{
			ct.retired = tBool.get();
		}
		else
		{
			ct.retired = false;
		}
		
		auto ap_range = ct_it->second.equal_range(ANALYSIS_PARAM_NODE);
		auto ap_it = ap_range.first;
		for (ap_it; ap_it != ap_range.second; ap_it++)
		{
			AnalysisParameterDLL tmp_ap = {};
			ConvertToStruct(ap_it, tmp_ap);
			ct.cell_identification_parameters.push_back(tmp_ap);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: CellTypeDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToStruct(const pt_assoc_it&  ad_it, AnalysisDefinitionDLL& ad)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: AnalysisDefinitionDLL: <Enter>");

		ad.analysis_index = ad_it->second.get<uint16_t>(INDEX_NODE);
		ad.label = ad_it->second.get<std::string>(LABEL_NODE);
		ad.mixing_cycles = ad_it->second.get<uint8_t>(MIXING_CYCLES_NODE);

		auto ri_range = ad_it->second.equal_range(REAGENT_INDICES_NODE);
		auto ri_it = ri_range.first;
		for (ri_it; ri_it != ri_range.second; ri_it++)
		{
			uint32_t ri = ri_it->second.get<uint32_t>(INDEX_NODE);
			ad.reagent_indices.push_back(ri);
		}

		auto flis_range = ad_it->second.equal_range(FL_ILLUMINATION_SETTINGS_NODE);
		auto flis_it = flis_range.first;
		for (flis_it; flis_it != flis_range.second; flis_it++)
		{
			FL_IlluminationSettings tmp_flis = {};
			tmp_flis.illuminator_wavelength_nm = flis_it->second.get<uint16_t>(IILUMINATOR_WL_NODE);
			tmp_flis.emission_wavelength_nm = flis_it->second.get<uint16_t>(EMISSION_WL_NODE);
			tmp_flis.exposure_time_ms = flis_it->second.get<uint16_t>(EXPOSURE_TIME_NODE);

			ad.fl_illuminators.push_back(tmp_flis);
		}

		auto ap_range = ad_it->second.equal_range(ANALYSIS_PARAM_NODE);
		auto ap_it = ap_range.first;
		for (ap_it; ap_it != ap_range.second; ap_it++)
		{
			AnalysisParameterDLL tmp_ap = {};
			ConvertToStruct(ap_it, tmp_ap);

			ad.analysis_parameters.push_back(tmp_ap);
		}

		auto pop_param_it = ad_it->second.find(POP_PARAM_NODE);
		if (pop_param_it != ad_it->second.not_found())
		{
			AnalysisParameterDLL tmp_ap = {};
			ConvertToStruct(pop_param_it, tmp_ap);

			ad.population_parameter = tmp_ap;
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: AnalysisDefinitionDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToStruct(const pt_assoc_it&  ap_it, AnalysisParameterDLL& ap)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: AnalysisParameterDLL: <Enter>");

		ap.label = ap_it->second.get<std::string>(LABEL_NODE);
		ap.threshold_value = ap_it->second.get<float>(THRESHOLD_VALUE_NODE);
		ap.above_threshold = ap_it->second.get<bool>(ABOVE_THRESHOLD_NODE);
		auto characteristic_it = ap_it->second.find(CHARACTERISTIC_NODE);
		if (characteristic_it != ap_it->second.not_found())
		{
			ap.characteristic.key = characteristic_it->second.get<uint16_t>(KEY_NODE);
			ap.characteristic.s_key = characteristic_it->second.get<uint16_t>(SUB_KEY_NODE);
			ap.characteristic.s_s_key = characteristic_it->second.get<uint16_t>(SUB_SUB_KEY_NODE);
		}
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: AnalysisParameterDLL: <Exit>");
	}

	//*****************************************************************************
	void ConvertToStruct (const pt_assoc_it&  sig_it, DataSignatureInstanceDLL& sig)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: DataSignatureInstanceDLL: <Enter>");

		sig.signing_user = sig_it->second.get<std::string>(USER_NODE);
		uint64_t time_in_secs = sig_it->second.get<uint64_t>(TIMESTAMP_NODE);
		sig.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(time_in_secs);
		sig.signature.short_text = sig_it->second.get<std::string>(SHORT_TEXT_NODE);
		sig.signature.long_text = sig_it->second.get<std::string>(LONG_TEXT_NODE);

		auto iter = sig_it->second.find(SHORT_TEXT_SIGNATURE_NODE);
		if (iter != sig_it->second.not_found())
		{
			sig.signature.short_text_signature = sig_it->second.get<std::string>(SHORT_TEXT_SIGNATURE_NODE);
			sig.signature.long_text_signature = sig_it->second.get<std::string>(LONG_TEXT_SIGNATURE_NODE);
			sig.signature.sig_hash = sig_it->second.get<std::string>(SIGNATURE_HASH_NODE);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "ConvertToStruct: DataSignatureInstanceDLL: <Exit>");
	}
}
