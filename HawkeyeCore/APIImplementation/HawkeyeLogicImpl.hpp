#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

//#define TEST_REPLACING_REAGENT_PACK

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include <DBif_Api.h>

#include "ActiveDirectory.hpp"
#include "AuditLog.hpp"
#include "AutomationConfig.hpp"
#include "Bioprocess.hpp"
#include "BrightfieldDustSubtractWorkflow.hpp"
#include "CameraAutofocusWorkflow.hpp"
#include "CalibrationHistory.hpp"
#include "CellHealthReagents.hpp"
#include "ColumnSetting.hpp"
#include "DBConfig.h"
#include "DecontaminateFlowCellWorkflow.hpp"
#include "DrainReagentPackWorkflow.hpp"
#include "ErrorLog.hpp"
#include "Fluidics.hpp"
#include "FlushFlowCellWorkflow.hpp"
#include "Hardware.hpp"
#include "HawkeyeResultsDataManager.hpp"
#include "HawkeyeServices.hpp"
#include "LiveScanningUtilities.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperDLL.hpp"
#include "InitializationState.hpp"
#include "NativeDataType.hpp"
#include "PrimeReagentLinesWorkflow.hpp"
#include "PurgeReagentLinesWorkflow.hpp"
#include "QualityControlDLL.hpp"
#include "ReagentPack.hpp"
#include "SecurityHelpers.hpp"
#include "ScheduledExportDLL.hpp"
#include "SMTPConfig.h"
#include "SResultBinStorage.hpp"
#include "TemporaryImageAnalysis.hpp"
#include "UserLevels.hpp"
#include "UserList.hpp"
#include "UiDllLayerMediator.hpp"
#include "uuid__t.hpp"
#include "UserRecord.hpp"
#include "WorkflowController.hpp"
#include "WorklistDLL.hpp"
#include "HawkeyeConfig.hpp"

typedef std::function<void (CalibrationState)> motor_calibration_state_callback_DLL;
typedef std::function<void (HawkeyeError, uuid__t)> delete_results_callback_DLL;
typedef std::function<void (HawkeyeError, uuid__t)> export_data_progress_callback_DLL;
typedef std::function<void (HawkeyeError, char*)> export_data_completion_callback_DLL;

typedef std::function<void(HawkeyeError)> HawkeyeErrorCallback;
typedef std::function<void(bool)> BooleanCallback;

struct ReanlyzeSample_DataParams
{
	HawkeyeError completion_status;
	std::string loggedInUsername;
	SampleDefinitionDLL sampleDef;
	uint32_t cellType_index;
	uint16_t analysis_index;
	CellTypeDLL celltype;
	AnalysisDefinitionDLL analysis;
	ResultRecordDLL resultRecord;
	bool from_images;
	std::shared_ptr<ImageSetCollection_t> tmp_image_collection;
	std::shared_ptr<ImageCollection_t> image_collection;
	ImageSet_t background_normalization_images;
	sample_analysis_callback_DLL cb;

	DBApi::DB_SampleRecord dbSampleRecord;
	DBApi::DB_AnalysisRecord dbAnalysisRecord;
};

class HawkeyeLogicImpl final
{
public:
	HawkeyeLogicImpl();
	~HawkeyeLogicImpl();

	enum class InitializationSequence
	{
		// Order of initialization.
		Hardware,
		UserList,
		SerialNumber,
		SystemStatus,
		Analysis,
		CalibrationHistory,
		CellType,
		QualityControl,
		ReagentPack,
		Results,
		Signatures,
		NightlyClean,
		Validation,
		Complete,
		Error,
	};

	// For primitive-type buffers
	HawkeyeError FreeTaggedBuffer(NativeDataType type, void* pointer);
	// For arrays of buffers
	HawkeyeError FreeListOfTaggedBuffers(NativeDataType type, void* pointer, uint32_t num_items);

	void         Initialize (bool withHardware = true);
	void         internalInitialize (BooleanCallback callback);
	InitializationState IsInitializationComplete();
	bool		 InitializationComplete();
	void         Shutdown();
	void         internalShutdown();
	bool         IsShutdownComplete();

	void         GetSystemStatus (SystemStatusData*& systemStatusData);
	void         FreeSystemStatus (SystemStatusData* systemStatusData);
	HawkeyeError GetSystemSerialNumber(char*& serialNumber);

	char*        SystemErrorCodeToString(uint32_t system_error_code);
	void         SystemErrorCodeToExpandedStrings(uint32_t system_error_code, 
	                 char*& severity, 
	                 char*& system, 
	                 char*& subsystem, 
	                 char*& instance, 
	                 char*& failure_mode,
	                 char*& cellHealthErrorCode);
	void         SystemErrorCodeToExpandedResourceStrings (uint32_t system_error_code,
	                 char*& severity, 
	                 char*& system,
	                 char*& subsystem, 
	                 char*& instance, 
	                 char*& failure_mode,
	                 char*& cellHealthErrorCode);

	HawkeyeError ClearSystemErrorCode(uint32_t active_error);

	void         GetVersionInformation(SystemVersion& version);
	HawkeyeError SetSystemSecurityType(eSECURITYTYPE secType, const char* uname, const char* password);
	eSECURITYTYPE GetSystemSecurityType();
	HawkeyeError SampleTubeDiscardTrayEmptied();
	HawkeyeError EjectSampleStage(const char* username, const char* password, HawkeyeErrorCallback cb,
								  int32_t angle = AutoSelectEjectAngle, bool loading = EJECT_HOLDING_CURRENT_OFF);		// allow positioning to non-standard angle, and provide ability to leave holding current enabled
	void         RotateCarousel(std::function<void(HawkeyeError, SamplePositionDLL)> callback);

	HawkeyeError GetUserList(bool only_enabled, char**& userList, uint32_t& numUsers);
	HawkeyeError GetCurrentUser(char*& username, UserPermissionLevel& permissions);
	HawkeyeError GetUserPermissionLevel(const char* username, UserPermissionLevel& permissions);

	HawkeyeError LoginConsoleUser(const char* username, const char* password);
	void         LogoutConsoleUser();

	HawkeyeError LoginRemoteUser(const char* username, const char* password);
	void         LogoutRemoteUser(const char* username);

	HawkeyeError SwapUser(const char* newusername, const char* password, SwapUserRole swapRole);

	HawkeyeError AdministrativeUserUnlock(const char* administrator_account, const char* administrator_password, const char* user_account);
	char*        GenerateHostPassword(const char* securitykey);
	void         GetUserInactivityTimeout(uint16_t& minutes);
	void         GetUserPasswordExpiration(uint16_t& days);

	HawkeyeError LogoutUser_Inactivity();
	HawkeyeError AddUser(const char* username, const char* displayname, const char* password, UserPermissionLevel permissions);
	HawkeyeError RemoveUser(const char* username);
	HawkeyeError EnableUser(const char* username, bool enabled);
	HawkeyeError ChangeUserPassword(const char* username, const char* password, bool resetPwd = false);
	HawkeyeError ChangeUserDisplayName(const char* username, const char* displayname);
	HawkeyeError ChangeUserPermissions(const char* username, UserPermissionLevel permissions);

	HawkeyeError GetSampleColumns(const char* username, ColumnSetting*& recs, uint32_t& retrieved_count);
	HawkeyeError SetSampleColumns(const char* username, ColumnSetting* recs, uint32_t count);
	void FreeSampleColumns(ColumnSetting* recs, uint32_t count);

	//
	// User record access
	//
	HawkeyeError GetUserRecord(const char* username, UserRecord*& record);
	void FreeUserRecord(UserRecord* record);
	// @todo - needs implementation - this will allow setting LANG and Run-Options / defaults
	// DLL_CLASS HawkeyeError UpdateUserRecord(const UserRecord* record);
	// @todo - remove the following functions once UpdateUserRecord is implemented
	// @deprecated APIs
	HawkeyeError SetDisplayDigits(const char* username, uint32_t digits);
	HawkeyeError SetUserFastMode(const char* username, bool enable);
	HawkeyeError SetUserEmail(const char* username, const char* email);
	HawkeyeError SetUserComment(const char* username, const char* comment);
	HawkeyeError SetUserFolder(const char* username, const char* folder);

	HawkeyeError GetAllowedCellTypes(const char* username, uint32_t& num_ct, CellType*& celltypes);
	// @todo - refator to remove 'indecies' 
	HawkeyeError SetAllowedCellTypeIndices (const char* username, uint32_t nCells, uint32_t* celltype_indices);
	HawkeyeError SetUserAnalysisIndices (const char* username, uint32_t n_ad, uint16_t* analysis_indices);
	HawkeyeError GetUserAnalysisIndices (const char* username, uint32_t& n_ad, uint16_t*& analysis_indices);


	// Instrument configuration
	HawkeyeError SetUserInactivityTimeout(uint16_t minutes);
	HawkeyeError SetUserPasswordExpiration(uint16_t days);

	HawkeyeError IsPasswordExpired(const char* username, bool& expired); // @todo - v1.4 add days till expires as another out param
	HawkeyeError ChangeMyPassword(const char* oldpassword, const char* newpassword);
	// \brief Validates that the current user's account is Enabled and that the given password is correct
	HawkeyeError ValidateMe(const char* password); 
	HawkeyeError ValidateUserCredentials(const char* username, const char* password);
	HawkeyeError ValidateLocalAdminAccount(const char* username, const char* password);

	void         GetReagentContainerStatus (HawkeyeErrorCallback callback, uint16_t reagent_num, ReagentContainerState*& status);
	void         GetReagentContainerStatusAll (HawkeyeErrorCallback callback, uint16_t& num_reagents, ReagentContainerState*& status);
	void         FreeListOfReagentContainerState(ReagentContainerState* list, uint32_t num_reagents);
	void         GetReagentDefinitions (HawkeyeErrorCallback callback, uint32_t& num_reagents, ReagentDefinition*& reagents);
	void         FreeReagentDefinitions (ReagentDefinition* list, uint32_t num_reagents);
	void         UnloadReagentPack (ReagentContainerUnloadOption* UnloadActions, 
	                                uint16_t nContainers, 
	                                ReagentPack::reagent_unload_status_callback_DLL onUnloadStatusChange, 
	                                ReagentPack::reagent_unload_complete_callback_DLL onUnloadComplete, 
	                                HawkeyeErrorCallback callback);
	void         LoadReagentPack (ReagentPack::reagent_load_status_callback_DLL onLoadStatusChange, ReagentPack::reagent_load_complete_callback_DLL onLoadComplete, std::function<void(HawkeyeError)> callback);
	HawkeyeError SetReagentContainerLocation (ReagentContainerLocation& location);

	void         StartFlushFlowCell (FlushFlowCellWorkflow::flowcell_flush_status_callback_DLL on_status_change, HawkeyeErrorCallback callback);
	HawkeyeError GetFlushFlowCellState (eFlushFlowCellState& state);
	HawkeyeError CancelFlushFlowCell();
	void         StartDecontaminateFlowCell (DecontaminateFlowCellWorkflow::flowcell_decontaminate_status_callback_DLL on_status_change, HawkeyeErrorCallback callback);
	HawkeyeError GetDecontaminateFlowCellState (eDecontaminateFlowCellState& state);
	HawkeyeError CancelDecontaminateFlowCell();
	void         StartPrimeReagentLines (PrimeReagentLinesWorkflow::prime_reagentlines_callback_DLL on_status_change, HawkeyeErrorCallback callback);
	HawkeyeError GetPrimeReagentLinesState (ePrimeReagentLinesState& state);
	HawkeyeError CancelPrimeReagentLines();
	void         StartDrainReagentPack (DrainReagentPackWorkflow::drain_reagentpack_callback_DLL on_status_change, uint8_t valve_position, uint32_t repeatCount, HawkeyeErrorCallback callback);
	HawkeyeError GetDrainReagentPackState (eDrainReagentPackState& state);
	HawkeyeError CancelDrainReagentPack();
	void         StartCleanFluidics (FlushFlowCellWorkflow::flowcell_flush_status_callback_DLL on_status_change, HawkeyeErrorCallback callback);
	HawkeyeError GetCleanFluidicsState(eFlushFlowCellState& state);
	HawkeyeError CancelCleanFluidics();
	static HawkeyeError GetReagentVolume (CellHealthReagents::FluidType type, int32_t& volume_ul);
	static HawkeyeError SetReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul);
	static HawkeyeError AddReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul);
	void         StartPurgeReagentLines (PurgeReagentLinesWorkflow::purge_reagentlines_callback_DLL on_status_change, HawkeyeErrorCallback callback);
	HawkeyeError CancelPurgeReagentLines();

	void         SaveSampleSetTemplate (const SampleSet& ss, HawkeyeErrorCallback onComplete);	
	void         GetSampleSetTemplateList (uint32_t skip,
	                                       uint32_t take,
	                                       SampleSet*& ss,
	                                       uint32_t& numSampleSets,
	                                       uint32_t& totalSampleSetsAvailable,
	                                       HawkeyeErrorCallback onComplete);
	void         GetSampleSetTemplateAndSampleList (const uuid__t uuid, SampleSet*& sampleSet, HawkeyeErrorCallback onComplete);
	void         GetSampleSetAndSampleList (const uuid__t uuid, SampleSet*& sampleSet, std::function<void(HawkeyeError)> onComplete);
	void         DeleteSampleSetTemplate (const uuid__t uuid, HawkeyeErrorCallback onComplete);

	void         GetSampleSetList(
	                 eFilterItem filterItem,
	                 uint64_t fromDate,
	                 uint64_t toDate,
	                 std::string username,
	                 std::string nameSearchString,
	                 std::string tagSearchString,
	                 std::string cellTypeOrQualityControlName,
	                 uint32_t skip, uint32_t take,
	                 uint32_t& totalSampleSetsAvailable,
	                 SampleSet*& sampleSets,
	                 uint32_t& numSampleSets,
	                 std::function<void(HawkeyeError)> onComplete);

	void         SetWorklist (const Worklist& wl, HawkeyeErrorCallback callback);
	void         SetImageOutputTypePreference(eImageOutputType image_type, HawkeyeErrorCallback callback);
	void         StartProcessing (const char* username, const char* password, 
		                          WorklistDLL::sample_status_callback_DLL onSampleStatus,
	                              WorklistDLL::sample_image_result_callback_DLL onSampleImageProcessed,
	                              WorklistDLL::sample_status_callback_DLL onSampleComplete,
                                  WorklistDLL::worklist_completion_callback_DLL onWorklistComplete, 
                                  HawkeyeErrorCallback onCompleteCallback);
	void         AddSampleSet (const SampleSet& ss, std::function<void (HawkeyeError)>onComplete);
	void         CancelSampleSet (uint16_t sampleSetIndex, HawkeyeErrorCallback onComplete);
	HawkeyeError PauseProcessing(const char* username, const char* password);
	void         ResumeProcessing (const char* username, const char* password, HawkeyeErrorCallback callback);
	HawkeyeError StopProcessing(const char* username, const char* password);
	HawkeyeError GetCurrentSample (SampleDefinition*& sd);
	HawkeyeError GetSampleDefinitionBySampleId (uuid__t sampleDataUuid, SampleDefinition*& sampleDef);
	void         FreeSampleDefinition (SampleDefinition* sd, uint32_t numSamples);
	void         FreeSampleSet (SampleSet* list, uint32_t num_samplesets);

public:
	HawkeyeError GetAllCellTypes (uint32_t& num_ct, CellType*& celltypes);
	HawkeyeError GetFactoryCellTypes (uint32_t& num_ct, CellType*& celltypes);

	HawkeyeError AddCellType (const char* username, const char* password, CellType& celltype, uint32_t& ct_index, const char* retiredName);

	// Deprecated
	HawkeyeError ModifyCellType (const char* username, const char* password, CellType& celltype);

	HawkeyeError RemoveCellType (const char* username, const char* password, uint32_t ct_index);

	void         FreeListOfCellType(CellType* list, uint32_t num_ct);
	HawkeyeError GetTemporaryCellType(CellType*& temp_cell);
	HawkeyeError SetTemporaryCellType(CellType* temp_cell);
	HawkeyeError SetTemporaryCellTypeFromExisting(uint32_t ct_index);

	HawkeyeError GetAllAnalysisDefinitions(uint32_t& num_ad, AnalysisDefinition*& analyses);
	HawkeyeError GetFactoryAnalysisDefinitions(uint32_t& num_ad, AnalysisDefinition*& analyses);
	HawkeyeError GetUserAnalysisDefinitions(uint32_t& num_ad, AnalysisDefinition*& analyses);

	void         FreeAnalysisDefinitions(AnalysisDefinition* list, uint32_t num_ad);
	
	//Deprecated
	HawkeyeError IsAnalysisSpecializedForCellType (uint16_t ad_index, uint32_t ct_index, bool& is_specialized);
	
	HawkeyeError GetAnalysisForCellType(uint16_t ad_index, uint32_t ct_index, AnalysisDefinition*& ad);
	HawkeyeError SpecializeAnalysisForCellType (AnalysisDefinition& ad, uint32_t ct_index);

	//Deprecated
	HawkeyeError RemoveAnalysisSpecializationForCellType(uint16_t ad_index, uint32_t ct_index);

	HawkeyeError GetTemporaryAnalysisDefinition(AnalysisDefinition*& temp_definition);
	HawkeyeError SetTemporaryAnalysisDefinition(AnalysisDefinition* temp_definition);

	//Deprecated
	HawkeyeError SetTemporaryAnalysisDefinitionFromExisting(uint16_t analysis_index);

	HawkeyeError ExportInstrumentConfiguration(const char* username, const char* password, const char * configfilename);
	HawkeyeError ImportInstrumentConfiguration(const char* username, const char* password, const char* configfilename);

	HawkeyeError ExportInstrumentData(const char* username, const char* password, 
		                              uuid__t * rs_uuid_list,
                                      uint32_t num_uuid,
                                      const  char* export_location,
                                      eExportImages exportImages,
                                      uint16_t export_nth_image,
                                      export_data_completion_callback_DLL onExportCompletionCb,
                                      export_data_progress_callback_DLL exportdataProgressCb = nullptr);

	HawkeyeError CancelExportData();


	HawkeyeError Export_Start(
		const char* username, const char* password,
		uuid__t* rs_uuid_list,
		uint32_t num_uuid,
		const  char* outPath,
		eExportImages exportImages,
		uint16_t export_nth_image);

	HawkeyeError  Export_NextMetaData(uint32_t index, uint32_t delayms);

	HawkeyeError  Export_IsStorageAvailable();

	HawkeyeError  Export_ArchiveData(const char* filename, char*& outname);

	HawkeyeError  Export_Cleanup(bool removeFile);




	HawkeyeError GetQualityControlList(const char* username, const char* password, bool allFlag, QualityControl_t*& qualitycontrols, uint32_t& num_qcs);
	HawkeyeError AddQualityControl(const char* username, const char* password, QualityControl_t qualitycontrol, const char* retiredQcName);
	void         FreeListOfQualityControl(QualityControl_t* list, uint32_t n_items);
	
	HawkeyeError SetConcentrationCalibration (calibration_type calType, double slope, double intercept, uint32_t cal_image_count, uuid__t queue_id, uint16_t num_consumables, calibration_consumable* consumables);

	void         StartBrightfieldDustSubtract(BrightfieldDustSubtractWorkflow::brightfield_dustsubtraction_callback_DLL on_status_change, HawkeyeErrorCallback callback);

	void         AcceptDustReference(bool accepted, HawkeyeErrorCallback callback);
	HawkeyeError CancelBrightfieldDustSubtract();

	void		RetrieveAuditTrailLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, audit_log_entry*& log_entries, HawkeyeErrorCallback callback);
	void		WriteToAuditLog (const char* username, audit_event_type type, const char* resource);

	void		RetrieveInstrumentErrorLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, error_log_entry*& log_entries, HawkeyeErrorCallback callback);

	void		RetrieveSampleActivityLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, sample_activity_entry*& log_entries, HawkeyeErrorCallback callback);

	HawkeyeError RetrieveCalibrationActivityLogRange(calibration_type cal_type, uint64_t starttime, uint64_t endtime, uint32_t& num_entries, calibration_history_entry*& log_entries);
	HawkeyeError ClearCalibrationActivityLog(calibration_type cal_type, uint64_t clear_prior_to_time, char* verification_password, bool clearAllACupData=false);

	HawkeyeError AddSignatureDefinition(DataSignature_t* signature);
	HawkeyeError RemoveSignatureDefinition(char* signature_short_text, uint16_t short_text_len);

	void         svc_SetSystemSerialNumber(char* serial, char* service_password, HawkeyeErrorCallback onComplete);

	void         svc_CameraFocusAdjust(HawkeyeErrorCallback callback, bool direction_up, bool adjustment_fine);
	void         svc_CameraFocusRestoreToSavedLocation(HawkeyeErrorCallback callback);

	HawkeyeError svc_PerformPlateCalibration (motor_calibration_state_callback_DLL);
	HawkeyeError svc_PerformCarouselCalibration (motor_calibration_state_callback_DLL);
	HawkeyeError svc_CancelCalibration (motor_calibration_state_callback_DLL);

	HawkeyeError svc_GetValvePort (char & port);
	void         svc_SetValvePort (HawkeyeErrorCallback callback, char port, SyringePumpDirection direction);

	void         svc_AspirateSample (HawkeyeErrorCallback callback, uint32_t volume);
	void         svc_DispenseSample (HawkeyeErrorCallback callback, uint32_t volume);
	HawkeyeError svc_GetProbePostion(int32_t &pos);
	void         svc_MoveProbe (bool upDown, HawkeyeErrorCallback cb);
	void         svc_SetProbePostion(bool upDown, uint32_t stepsToMove, HawkeyeErrorCallback callback);
	HawkeyeError svc_GetSampleWellPosition(SamplePosition& pos);
	void         svc_SetSampleWellPosition(SamplePosition pos, HawkeyeErrorCallback callback);

	void         svc_MoveReagentArm(bool up, HawkeyeErrorCallback cb);
	void         InitializeCarrier(HawkeyeErrorCallback cb);

	void         svc_CameraAutoFocus(
		              SamplePosition focusbead_location, 
		              CameraAutofocusWorkflow::autofocus_state_callback_t_DLL on_status_change, 
		              CameraAutofocusWorkflow::countdown_timer_callback_t_DLL on_timer_tick, 
		              HawkeyeErrorCallback callback);
	HawkeyeError svc_CameraAutoFocus_ServiceSkipDelay();
	void         svc_CameraAutoFocus_FocusAcceptance(eAutofocusCompletion decision, HawkeyeErrorCallback callback);
	HawkeyeError svc_CameraAutoFocus_Cancel();

	HawkeyeError svc_GetFlowCellDepthSetting(uint16_t& flow_cell_depth);
	HawkeyeError svc_SetFlowCellDepthSetting(uint16_t flow_cell_depth);

	void         svc_GetCameraLampState (bool& lamp_on, float& intensity_0_to_100, HawkeyeErrorCallback callback);
	void         svc_SetCameraLampState (bool lamp_on, float intensity_0_to_100, HawkeyeErrorCallback callback);
	void         svc_SetCameraLampIntensity (float intensity_0_to_100, HawkeyeErrorCallback callback);

	HawkeyeError svc_GenerateSingleShotAnalysis(TemporaryImageAnalysis::temp_analysis_result_callback_DLL callback);
	HawkeyeError svc_GenerateContinuousAnalysis(TemporaryImageAnalysis::temp_analysis_result_callback_DLL callback);
	HawkeyeError svc_StopContinuousAnalysis();
	void         svc_StartLiveImageFeed (LiveScanningUtilities::service_live_image_callback_DLL liveImageCallback, HawkeyeErrorCallback functionCallback);
	HawkeyeError svc_StopLiveImageFeed();

	HawkeyeError svc_ManualSample_Load();
	HawkeyeError svc_ManualSample_Nudge();
	HawkeyeError svc_ManualSample_Expel();

	void FreeListOfWorklistRecord (WorklistRecord* list, uint32_t n_items);
	
	void RetrieveSampleRecord(uuid__t id, SampleRecord*& rec, HawkeyeErrorCallback cb);
	void RetrieveSampleRecords(uint64_t start, uint64_t end, char* username, SampleRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb);
	void RetrieveSampleRecordList(uuid__t* ids, uint32_t list_size, SampleRecord*& recs, uint32_t& retrieved_size, HawkeyeErrorCallback cb);
	void FreeListOfSampleRecord(SampleRecord* list, uint32_t n_items);
	void freeListOfReagentInfoRecord(ReagentInfoRecord* list, uint32_t n_items);
	
	void RetrieveSampleImageSetRecord (uuid__t id, SampleImageSetRecord*& rec, HawkeyeErrorCallback cb);
	void RetrieveSampleImageSetRecords (uint64_t start, uint64_t end, char* username, SampleImageSetRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb);
	void RetrieveSampleImageSetRecordList (uuid__t* ids, uint32_t list_size, SampleImageSetRecord*& recs, uint32_t& retrieved_size, HawkeyeErrorCallback cb);
	void FreeListOfImageSetRecord (SampleImageSetRecord* list, uint32_t n_items);

	void RetrieveImage (uuid__t id, ImageWrapper_t*& img, HawkeyeErrorCallback cb);
	void RetrieveAnnotatedImage(uuid__t result_id, uuid__t image_id, ImageWrapper_t*& img, HawkeyeErrorCallback cb);
	void RetrieveBWImage(uuid__t image_id, ImageWrapper_t*& img, HawkeyeErrorCallback cb);
	void FreeListOfImageRecord(ImageRecord* list, uint32_t n_items);

	void RetrieveResultRecord (uuid__t id, ResultRecord*& rec, HawkeyeErrorCallback cb);
	void RetrieveResultRecords(uint64_t start, uint64_t end, char* username, ResultRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb);
	void RetrieveResultRecordList(uuid__t* ids, uint32_t list_size, ResultRecord*& recs, uint32_t& retrieved_size, HawkeyeErrorCallback cb);
	void FreeListOfResultRecord (ResultRecord* list, uint32_t n_items);

	void RetrieveResultSummaryRecord (uuid__t id, ResultSummary*& rec, HawkeyeErrorCallback cb);
	
	void RetrieveResultSummaryRecordList (uuid__t * ids, uint32_t list_size, ResultSummary *& recs, uint32_t & retrieved_size, HawkeyeErrorCallback cb);
	void FreeListOfResultSummary (ResultSummary* list, uint32_t n_items);

	void RetrieveDetailedMeasurementsForResultRecord(uuid__t id, DetailedResultMeasurements*& measurements, HawkeyeErrorCallback cb);
	void FreeDetailedResultMeasurement(DetailedResultMeasurements* meas);

	HawkeyeError ReanalyzeSample(uuid__t sample_id, uint32_t celltype_index, uint16_t analysis_index, bool from_images, sample_analysis_callback_DLL onSampleComplete);

	void RetrieveHistogramForResultRecord (uuid__t id, bool POI, Hawkeye::Characteristic_t measurement, uint8_t& bin_count, histogrambin_t*& bins, HawkeyeErrorCallback cb);
	HawkeyeError FreeHistogramBins (histogrambin_t* list);

	void RetrieveSampleRecordsForBPandQC (const char* bp_qc_name, SampleRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb);
	
	HawkeyeError RetrieveSignatureDefinitions(DataSignature_t*& signatures, uint16_t& num_signatures);
	void SignResultRecord(uuid__t record_id, char* signature_short_text, uint16_t short_text_len, HawkeyeErrorCallback cb);

	void FreeImageSetWrapper(ImageSetWrapper_t*& image_set_wrapper, uint16_t num_image_set_wrapper);
	void FreeImageWrapper(ImageWrapper_t*& image_wrapper, uint16_t num_image_wrapper);
	void FreeAutofocusResults(AutofocusResults*& results, uint8_t num_result);

	void FreeAuditLogEntry(audit_log_entry* entries, uint32_t num_entries);

	void freeSampleWorklistEntry (worklist_sample_entry* entries, uint32_t num_entries);

	void FreeSampleActivityEntry(sample_activity_entry* entries, uint32_t num_entries);
	void FreeCalibrationHistoryEntry(calibration_history_entry* entries, uint32_t num_entries);
	void FreeErrorLogEntry(error_log_entry* entries, uint32_t num_entries);

	void FreeDataSignature(DataSignature_t* signatures, uint16_t num_signatures);
	void FreeDataSignatureInstance (DataSignatureInstance_t* signatures, uint16_t num_signatures);

	void DeleteSampleRecord (
		const char* username,
		const char* password,
		uuid__t *sample_uuidlist, 
		uint32_t num_uuid, 
		bool retain_results_and_first_image, 
		delete_results_callback_DLL onDeleteCompletion, std::function<void(HawkeyeError)> onComplete);

	HawkeyeError ValidateActiveDirConfig(ActiveDirectoryConfig cfg, const char* adminGroup, const char* uName, const char* password, bool& valid);

	void GetActiveDirConfig (ActiveDirectoryConfig*& myCfg);
	void SetActiveDirConfig (ActiveDirectoryConfig myCfg);
	void FreeActiveDirConfig (ActiveDirectoryConfig* myCfg);

	void GetActiveDirectoryGroupMaps (ActiveDirectoryGroup*& recs, uint32_t& retrieved_count);
	void SetActiveDirectoryGroupMaps (ActiveDirectoryGroup* recs, uint32_t count);
	void FreeActiveDirGroupMaps (ActiveDirectoryGroup* recs, uint32_t count);

	HawkeyeError GetDBConfig (DBConfig*& cfg);
	HawkeyeError SetDBConfig (DBConfig cfg);
	void FreeDBConfig (DBConfig* cfg);
	HawkeyeError SetDbBackupUserPassword( const char* password );

	HawkeyeError GetSMTPConfig (SMTPConfig*& cfg);
	HawkeyeError SetSMTPConfig (SMTPConfig cfg);
	void FreeSMTPConfig (SMTPConfig* cfg);

	HawkeyeError GetAutomationSettings (AutomationConfig*& cfg);
	HawkeyeError SetAutomationSettings (AutomationConfig cfg);
	void FreeAutomationSettings (AutomationConfig* cfg);

	static bool GetCellTypeByIndex (uint32_t index, CellTypeDLL& cellType);

	UiDllLayerMediator& getUiDllInstance();

	bool SystemHasData();

	/// Scheduled Export Functions
	HawkeyeError AddScheduledExport(ScheduledExport scheduled_export, uuid__t *uuid);
	HawkeyeError EditScheduledExport(ScheduledExport scheduled_export);
	HawkeyeError DeleteScheduledExport(uuid__t uuid);
	HawkeyeError GetScheduledExports(eScheduledExportType type, ScheduledExport*& scheduled_exports, uint32_t& count);
	HawkeyeError FreeListOfScheduledExports(ScheduledExport* scheduled_exports, uint32_t count);
	static void ShutdownOrReboot (ShutdownOrRebootEnum operation);
	static HawkeyeError DeleteCampaignData();

	/// Device configuration options
	HawkeyeError SetOpticalHardwareConfig(OpticalHardwareConfig type);
	OpticalHardwareConfig GetOpticalHardwareConfig() const;

private:
	std::shared_ptr<boost::asio::io_context> pLocalIosvc_;
	std::unique_ptr<HawkeyeThreadPool> pLocalIosThreadPool_;

	// HawkeyeServices
	std::shared_ptr<HawkeyeServices> pHawkeyeServices_;
	
	// Creates Public and Private key
	std::shared_ptr<SecurityHelpers::RSADigitalSignature> pDigitalSignature_;
	
	bool initializationRun;
	bool loggersInitialized;
	bool asioInitialized;
	bool isShutdownComplete_;
	bool isShutdownInProgress_;

	std::unique_ptr<UiDllLayerMediator> pUiDllLayerMediator_;

	bool intializeLoggers();
	bool intializeAsio();
	void internalInitializationProcess (BooleanCallback callback, InitializationSequence sequence);
	void setNextInitializationSequence (BooleanCallback callback, InitializationSequence nextSequence);
	void validateInstrumentStatusOnLogin();
	void ejectSampleStageInternal( int32_t angle, bool loading, HawkeyeErrorCallback cb );

	// Analysis
	bool initializeAnalysis();

	/// Calibration
	bool initializeCalibrationHistory();
	CalibrationState calibrationState_;

	/// CellTypes
	bool initializeCellType();
	std::shared_ptr<CellTypeDLL> GetTemporaryCellType();
	void SetTemporaryCellType (std::shared_ptr<CellTypeDLL> cellType);

	/// Nightly Clean
	bool initializeNightlyClean();
	static void destroyNightlyClean();
	void nightCleanCycleRoutine(const boost::system::error_code& ec);
	void executeNightCleanCycle();
	void ExecuteNightlyClean (uint8_t workflowSubType, HawkeyeErrorCallback callback);
	void updateNightlyCleanStatus(eNightlyCleanStatus ncs, HawkeyeError he);

	/// Quality Control
	bool initializeQualityControl() const;

	/// ReagentPack
	void initializeReagentPack (BooleanCallback callback);
	static std::map<uint16_t, ReagentDefinitionDLL> cachedReagentDefinitions_;

	/// Results
	void initializeResults();
	void freeResultSummaryInternal(ResultSummary& rs);
	void freeImageWrapperInternal(ImageWrapper_t& iw);

	/// Reanalysis
	std::shared_ptr<ImageProcessingUtilities> pReanalysisImgProcUtilities_;
	enum class ReanalyzeSampleStates : uint8_t
	{
		rs_GetExistingRecords = 0,
		rs_GetCellTypeAndAnalysis,
		rs_LoadImages,
		rs_LoadNormalizationImages,
		rs_DoReanalysis,
		rs_GenerateResultRecord,
		rs_Complete,
		rs_Error,
	};

	void reanlyzeSampleInternal(ReanlyzeSample_DataParams data_params, ReanalyzeSampleStates state);
	void reanlyzeSample_GetRecord(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);
	void reanlyzeSample_GetCellTypeAnalysis(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);
	void reanlyzeSample_LoadImages(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);
	void reanlyzeSample_LoadNormalizationImages(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);
	void reanalyzeSample_DoReanalysis(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);
	void reanalyzeSample_GenerateResultRecord(ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback);

	/// Serial number
	bool initializeSerialNumber();
	std::string serialNumber_FirmwareCacheCopy;
	std::string serialNumber_ConfigCopy;
	std::string serialNumber_Canonical;

	/// SignatureDefinitions
	bool initializeSignatures();

	/// Service
	enum class RepetitionState : uint8_t
	{
		ArmHome = 0,
		ArmDown,
		ArmUp,
		Complete,
		Error,
	};

	enum class SyringeRepetitionState : uint8_t
	{
		Empty = 0,
		Aspirate,
		Dispense,
		Complete,
		Error
	};
	void SyringeRepetition (HawkeyeErrorCallback callback, SyringeRepetitionState state);

	// service support
	enum class sm_CameraFocusAdjust : uint8_t
	{
		cfa_InitializeFocusController = 0,
		cfa_CheckMotorHome,
		cfa_MoveToPosition,
		cfa_DecideMotorMovement,
		cfa_StepMotorCoarse,
		cfa_StepMotorFine,
		cfa_Complete,
		cfa_Error,
	};

	void internal_svc_CameraFocusAdjust(
		std::string loggedInUsername,
		sm_CameraFocusAdjust state, 
		bool direction_up, 
		bool adjustment_fine, 
		int32_t startFocusPos, 
		HawkeyeErrorCallback callback);

	enum class sm_StoreCurrentFocusPosition : uint8_t
	{
		scfp_EntryPoint = 0,
		scfp_PreFlight,
		scfp_HomeMotor,
		scfp_PositionMotor,
		scfp_SaveLocation,
		scfp_Complete,
		scfp_Error,
	};

	void internal_svc_CameraFocusStoreCurrentPosition(
		std::string loggedInUsername,
		sm_StoreCurrentFocusPosition state, 
		int32_t location, 
		HawkeyeErrorCallback callback);

	enum class sm_PerformFocusRepetition : uint8_t
	{
		pfr_EntryPoint = 0,
		pfr_Home,
		pfr_SetCenter,
		pfr_Complete,
		pfr_Error,
	};	

	/// Private "free" methods
	void FreeListOfReagentState(ReagentState* list, uint32_t num_reagents);
	void FreeListOfBlobMeasurement(blob_measurements_t* list, uint32_t n_items);

	std::shared_ptr<boost::asio::deadline_timer> diskUsageLoggerTimer_;
	void LogMemoryAndDiskUsage(boost::system::error_code ec);
	std::shared_ptr<boost::asio::deadline_timer> cpuUsageTimer_;
	void LogCPUUsage(boost::system::error_code ec);

	//LH6531-5892 - A faulted firmware needs to be detected and the HW interaction needs to be shut down ASAP
	std::shared_ptr<boost::asio::deadline_timer> commsHealthTimer_;
	void CheckCommsStatusAndShutdown(boost::system::error_code ec);
};
