#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/thread.hpp>

#include <DBif_Api.h>

#include "HawkeyeLogic.hpp"

#define CHK_CMDS	true
#define IGNORE_CMDS	false
#define HIDE_ENTRY	true
#define SHOW_ENTRY	false

class ScoutX
{
public:
	ScoutX (boost::asio::io_context& io_s);
	virtual ~ScoutX();

	bool init (bool withHardware);
	void loginUser (std::string username, std::string password);
	void displaySampleDefinition (SampleDefinition& sample);

private:
	void prompt();
	void showHelp();

	HawkeyeError doUserLogin( std::string username, std::string password );

	bool GetDbDataTypeEnumString( int datatype, std::string& typestr );
	void ShowDbFilterEnum( void );
	bool GetDbFilterEnumString( int filtertype, std::string & typestr );
	bool IsDbFilterStringValue( DBApi::eListFilterCriteria filtertype );
	void ShowDbListSortEnum( void );
	bool GetDbSortEnumString( int sorttype, std::string& typestr );
	bool IsDbSortStringValue( DBApi::eListSortCriteria sorttype );

	void TrimStr( std::string trim_chars, std::string& tgtstr );
	void TrimWhiteSpace( std::string& tgtstr );
	int32_t TokenizeStr( std::vector<std::string>& tokenlist, std::string& parsestr,
						 std::string sepstr, std::string sepchars );
	bool GetStringInput( std::string& entry_str, std::string prompt_str = "",
						 std::string length_warning = "", int min_length = 0, bool hide_entry = SHOW_ENTRY );
	bool GetYesOrNoResponse( std::string& entry_str, std::string prompt_str, bool emptyOk = true );
	bool GetTrueOrFalseResponse( std::string& entry_str, std::string prompt_str, bool emptyOk = true );

	void ShowDbFilterList( std::vector<DBApi::eListFilterCriteria>& filtertypes,
						   std::vector<std::string>& filtercompareops,
						   std::vector<std::string>& filtercomparevals,
						   system_TP lastRunDate = {} );
	bool GetDbListFilter( std::vector<DBApi::eListFilterCriteria>& filtertypes,
						  std::vector<std::string>& filtercompareops,
						  std::vector<std::string>& filtercomparevals );
	bool GetDbListSort( DBApi::eListSortCriteria& primarysort,
						DBApi::eListSortCriteria& secondarysort,
						int32_t& sortdir );
	bool GetDbListSortAndFilter( std::vector<DBApi::eListFilterCriteria>& filtertypes,
								 std::vector<std::string>& filtercompareops,
								 std::vector<std::string>& filtercomparevals,
								 DBApi::eListSortCriteria& primarysort,
								 DBApi::eListSortCriteria& secondarysort,
								 int32_t& sortdir, int32_t& limitcnt );
	bool SelectDbConfigRec( std::vector<DBApi::DB_InstrumentConfigRecord>& cfg_list,
							DBApi::DB_InstrumentConfigRecord& cfgrec,
							std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult );
	bool SelectDbListIdNum( size_t listsize, int64_t& cfgidnum );
	bool GetDbDefaultConfigRec( DBApi::DB_InstrumentConfigRecord& cfgrec,
								std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult );
	bool GetDbConfigRec( DBApi::DB_InstrumentConfigRecord& cfgrec,
						 std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult );

	void GetWeeklyIndicatorString( DBApi::DB_SchedulerConfigRecord& scfgrec, std::string& indicatorStr, uint16_t strType = 0 );
	bool EnterSchedulerConfig( DBApi::DB_SchedulerConfigRecord& scfgRec, bool new_rec = true );

	bool DoDbConnect( bool do_connect = false );

	void print_reagents_definitions(ReagentDefinition*& reagents, size_t num_reagents);
	void print_reagents_containerstates(ReagentContainerState*& reagents);

	void print_db_worklist_list( std::vector<DBApi::DB_WorklistRecord>& wl_list );
	void print_db_worklist_obj( DBApi::DB_WorklistRecord worklist );
	void print_db_sampleset_list( std::vector<DBApi::DB_SampleSetRecord>& ss_list );
	void print_db_sampleitem_list( std::vector<DBApi::DB_SampleItemRecord>& si_list );
	void print_db_samples_list( std::vector<DBApi::DB_SampleRecord>& s_list );
	void print_db_imageset_list( std::vector<DBApi::DB_ImageSetRecord>& is_list );
	void print_db_imageseq_list( std::vector<DBApi::DB_ImageSeqRecord>& is_list );
	void print_db_image_list( std::vector<DBApi::DB_ImageRecord>& img_list );
	void print_db_celltypes_list( std::vector<DBApi::DB_CellTypeRecord>& ct_list );
	void print_db_analysis_def_list( std::vector<DBApi::DB_AnalysisDefinitionRecord>& def_list );
	void print_db_analysis_param_list( std::vector<DBApi::DB_AnalysisParamRecord>& param_list );
	void print_db_user_list( std::vector<DBApi::DB_UserRecord>& user_list );
	void print_db_user_record( DBApi::DB_UserRecord& user_rec );
	void print_db_signature_list( std::vector<DBApi::DB_SignatureRecord>& sig_list );
	void print_db_roles_list( std::vector<DBApi::DB_UserRoleRecord>& roles_list );
	void print_db_calibrations_list( std::vector<DBApi::DB_CalibrationRecord>& cal_list );
	void print_db_qc_process_list( std::vector<DBApi::DB_QcProcessRecord>& qc_list );
	void print_db_analyses_list( std::vector<DBApi::DB_AnalysisRecord>& an_list );
	void print_db_summary_results_list( std::vector<DBApi::DB_SummaryResultRecord>& summary_list );
	void print_db_detailed_results_list( std::vector<DBApi::DB_DetailedResultRecord>& detailed_list );
	void print_db_detailed_result( DBApi::DB_DetailedResultRecord& detailed_result );
	void print_db_image_results_list( std::vector<DBApi::DB_ImageResultRecord>& ir_list, int rec_format = -1 );
	void print_db_image_result( DBApi::DB_ImageResultRecord& ir, std::string& line_str );
	void print_db_reagent_info_list( std::vector<DBApi::DB_ReagentTypeRecord>& rx_list );
	void print_db_instrument_config_list( std::vector<DBApi::DB_InstrumentConfigRecord>& cfg_list );
	void print_db_instrument_config( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_illuminators_list( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_rfid_settings( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_af_settings( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_email_settings( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_ad_settings( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_lang_list( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_instrument_config_run_options( DBApi::DB_InstrumentConfigRecord cfgRec );
	void print_db_log_lists( std::vector<DBApi::DB_LogEntryRecord>& log_list, DBApi::eLogEntryType log_type );
	void print_db_scheduler_config_list( std::vector<DBApi::DB_SchedulerConfigRecord>& cfg_list );
	void print_db_scheduler_config( DBApi::DB_SchedulerConfigRecord cfgRec );


	bool readLine( bool& done_quit, bool chk_cmds = CHK_CMDS, bool hide_entry = SHOW_ENTRY );
	void handleInput (const boost::system::error_code& error);

	void runAsync_ (boost::function<void(bool)> cb_Handler);
	void doWork_ (boost::function<void(bool)> cb_Handler);

	void waitForInit (const boost::system::error_code& ec);
	void waitForInitTimeout (const boost::system::error_code& ec);

	void SystemStatusTest (boost::system::error_code ec);
	void StopSystemStatusTest();
	void createSampleParameters (SampleParameters& sp,
		char* label,
		char* tag,
		char* bp_qc_name,
		uint16_t analysisIndex,
		uint32_t cellTypeIndex,
		uint32_t dilutionFactor,
		eSamplePostWash postWash,
		uint32_t saveEveryNthImage);
	void createSampleDefinition (
		uint16_t sampleSetIndex,
		uint16_t index,
		SampleDefinition& sd,
		SampleParameters& sampleParameters,
		SamplePosition& samplePosition);
	void createSampleSet (uint16_t index, SampleSet& ss, char* name, eCarrierType carrier, SampleDefinition* sampleDefinitions, uint32_t numSampleDefinitions);
	void createEmptyCarouselWorklist (Worklist& wl);
	void createCarouselSampleSet2 (SampleSet& ss);
	void createCarouselSampleSet5 (SampleSet& ss);
	void createCarouselSampleSet10 (SampleSet& ss);
	void createCarouselWorklist (Worklist& wl);

	void createEmptyPlateWorklist (Worklist& wl);
	void createPlateSampleSet_3_6 (SampleSet& ss);
	void createPlateWorklist_A1_A12_H1_H12 (Worklist& wl);

	void setEmptySampleCupWorklist (Worklist& wl);

	bool buildNewCellType (CellType*& ct);
	bool buildModifiedCellType (CellType*& ct);
	bool buildAnalysisDefinition (AnalysisDefinition*& ad);

	uint32_t timeout_msecs_;
	std::shared_ptr<boost::asio::deadline_timer> initTimer_;
	std::shared_ptr<boost::asio::deadline_timer> initTimeoutTimer_;
	std::shared_ptr<boost::asio::deadline_timer> getWqStatusTimer_;
	std::string currentUser_;
	std::string currentPassword_;
	uuid__t addedSampleSetUUID;

	static ScoutX* pScoutX;

	static void cb_ReagentDrainStatus(eDrainReagentPackState rls);
	static void cb_ReagentPackLoadStatus (ReagentLoadSequence rls);
	static void cb_ReagentPackLoadComplete (ReagentLoadSequence rls);
	static void cb_ReagentPackUnloadStatus (ReagentUnloadSequence rls);
	static void cb_ReagentPackUnloadComplete (ReagentUnloadSequence rls);
	static void cb_ReagentFlowCellFlush(eFlushFlowCellState rls);
	static void cb_CleanFluidics(eFlushFlowCellState rls);
	static void cb_ReagentDecontaminateFlowCell(eDecontaminateFlowCellState rls);

	static void onSampleStatus (SampleDefinition*);
	static void onSampleImageProcessed (
		SampleDefinition* sd, 
		uint16_t imageSeqNum,					/* image sequence number */
		ImageSetWrapper_t* image,				/* image */
		BasicResultAnswers cumulativeResults,	/* cumulative */
		BasicResultAnswers imageResults);		/* this_image */
	static void onSampleComplete (SampleDefinition*);
	static void onWorklistComplete (uuid__t sampleId /* sample record indentifier*/);
};