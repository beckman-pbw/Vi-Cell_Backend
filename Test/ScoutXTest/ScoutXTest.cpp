#include "stdafx.h"

using namespace std;

#include <stdint.h>
#include <conio.h>
#include <functional>
#include <ios>
#include <cstdlib>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "DataConversion.hpp"
#include "HawkeyeAssert.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeError.hpp"
#include "HawkeyeLogic.hpp"
#include "HawkeyeUUID.hpp"
#include "Logger.hpp"
#include "ReportsDLL.hpp"
#include "ScoutXTest.hpp"
#include "SetString.hpp"
#include "StageDefines.hpp"
#include "SystemStatusCommon.hpp"

using namespace std;

static std::string MODULENAME = "ScoutXTest";

const std::string ServiceUser = "bci_service";
const std::string ServicePwd = "020177";

ScoutX* ScoutX::pScoutX;

static boost::asio::io_context io_svc;
static std::shared_ptr<std::thread> pio_thread_;
static std::shared_ptr<boost::asio::io_context::strand> strand_;
static boost::function<void(bool)> cb_Handler_;

static uint32_t timeout_msecs_;
static std::shared_ptr<boost::asio::deadline_timer> initTimer_;
static std::shared_ptr<boost::asio::deadline_timer> initTimeoutTimer_;
static std::shared_ptr<boost::asio::deadline_timer> systemstatusTestTimer_;
static uint16_t curSampleSetIndex = 0;
static uint16_t curSampleIndex = 0;

// For command line input.
#define ESC 0x1B
#define BACKSPACE '\b'
#define CR '\r'
#define NEWLINE '\n'
#define	CTRL_C 0x03
#define CTRL_Q 0x11
#define CTRL_X 0x18

static std::string smValueLine_;

typedef enum eStateMachine : uint32_t {
	smCmdEntry,
	smAdminOptions,
	smAdminAddUser,
	smAdminGetCurrentUser,
	smAdminGetUserCellTypeIndices,
	smAdminGetUserAnalysisIndices,
	smAdminGetUserProperty,
	smAdminChangeUserPassword,
	smAdminSetUserDisplayName,
	smAdminSetUserFolder,
	smAdminSetUserProperty,
	smAdminSetUserComment,
	smAdminSetUserEmail,
	smAdminSetUserCellTypeIndices,
	smAdminImportConfiguration,
	smAdminExportConfiguration,
	smAdminExportInstrumentData,
	smAdminDeleteSampleRecord,
	smPropertyEntry,
	smPropertyValueEntry,
	smFoldernameEntry,
	smUserOptions,
	smDisplaynameEntry,
	smUserLogin,
	smUserLogout,
	smUserChangeMyPassword,
	smUsernameEntry,
	smPasswordEntry,
	smUserListAll,
	smUserGetPermissions,
	smUserGetProperty,
	smSetDbBackupUserPwd,
	smReagentPackOptions,
	smReagentPackLoad,
	smReagentPackUnload,
	smReagentPackState,
	smReagentPackStateAll,
	smReagentPackNumberEntry,
	smReagentPackDrain_Option,
	smReagentPackUnload_Option_0,
	smReagentPackUnload_Option_1,
	smReagentPackDrainState,
	smReagentPackFlushFlowCell,
	smReagentPackFlushFlowCellState,
	smReagentPackDecontaminateFlowCell,
	smReagentPackDecontaminateFlowCellState,
	smServiceOptions,
	smResultsOptions,
	smAnalysisOptions,
	smCellOptions,
	smQCOptions,
	smWorklistOptions,
	smSampleSetToCancelEntry,
	smCancelSampleSet,
	smMiscOptions,
	smMiscGetSystemStatus,
	smMiscRetrieveAuditTrailLogRange,
	smMiscTestSystemErrorCodeExpandedStrings,
	smDatabaseOptions,
	smDbConnectionOptions,
	smDbCellTypeOptions,
	smDbAnalysisDefinitionOptions,
	smDbAnalysisParameterOptions,
	smDbUserOptions,
	smDbSetBackupUserPwd,
	smDbRoleOptions,
	smDbInstrumentConfigOptions,
	smDbLogOptions,
	smDbObjAdditionOptions,
	smDbObjUpdateOptions,
	smDbObjRemovalOptions,
	smDbRecordListOptions,
} StateMachine_t;

static std::shared_ptr<boost::asio::deadline_timer> inputTimer_;
static boost::system::error_code timerError_;
static StateMachine_t smState_ = smCmdEntry;
static StateMachine_t smStateOption_;

typedef enum eCommand : char {
	cmdAnalysis = 'A',
	cmdCells = 'C',
	cmdAdmin = 'D',

	cmdDatabase = 'E',

	cmdMiscellaneous = 'M',
	cmdQC = 'Q',
	cmdReagentPack = 'R',
	cmdService = 'S',
	cmdResults = 'T',
	cmdUser = 'U',
	cmdWorklist = 'W',
	cmdQuit = 'X',
	cmdHelp = '?',
} Command_t;

static std::string menu =
std::string(1, cmdAnalysis) + " | " +
std::string(1, cmdCells) + " | " +
std::string(1, cmdAdmin) + " | " +
std::string( 1, cmdDatabase ) + " | " +
std::string(1, cmdMiscellaneous) + " | " +
std::string(1, cmdQC) + " | " +
std::string(1, cmdReagentPack) + " | " +
std::string(1, cmdService) + " | " +
std::string(1, cmdResults) + " | " +
std::string(1, cmdUser) + " | " +
std::string(1, cmdWorklist) + " | " +
std::string(1, cmdQuit) + " | " +
std::string(1, cmdHelp)
;

static Command_t smCmd_;
static std::vector<std::string> parameterStack_;
static Worklist* currentWorklist_;

static bool isHardwareAvailable_ = false;

static boost::optional<uuid__t> uuidForRetrieval;

static std::string currentInstSN = "";
static int64_t currentInstCfgIdNum = -1;
static DBApi::DB_InstrumentConfigRecord currentCfgRec = {};
static std::string dfltInstSN = "";
static int64_t dfltInstCfgIdNum = -1;
static DBApi::DB_InstrumentConfigRecord defaultCfgRec = {};

//*****************************************************************************
static std::string carrierTypeToString( eCarrierType st )
{
	std::string typeStr = "";

	switch ( st )
	{
		case eCarrierType::eCarousel:
		{
			typeStr = "Carousel";
			break;
		}

		case eCarrierType::ePlate_96:
		{
			typeStr = "Plate_96";
			break;
		}

		case eCarrierType::eACup:
		{
			typeStr = "ACup";
			break;
		}

		case eCarrierType::eUnknown:
		{
			typeStr = "Unknown";
			break;
		}

		default:
		{
			typeStr = "Unrecognized";
			break;
		}
	}

	return typeStr;
}

//*****************************************************************************
static std::string precessionToString( ePrecession pr )
{
	return ( pr == eRowMajor ? "RowMajor" : "ColumnMajor" );
}

//*****************************************************************************
static std::string postWashToString( eSamplePostWash pw )
{
	return ( pw == eNormalWash ? "Normal" : "Fast" );
}

//*****************************************************************************
static std::string boolToString( bool val )
{
	return ( val ? "True" : "False" );
}

//*****************************************************************************
static std::string analysisParameterToString( const AnalysisParameter& ap )
{

	std::cout << "         Label: " << ap.label << std::endl;
	std::cout << " hresholdValue: " << ap.threshold_value << std::endl;
	std::cout << "AboveThreshold: " << boolToString( ap.above_threshold ) << std::endl;
}

//*****************************************************************************
static std::string flIlluminatorToString( const FL_IlluminationSettings& fli )
{

	std::cout << "IlluminatorWavelength: " << fli.illuminator_wavelength_nm << " nm" << std::endl;
	std::cout << " ExposureTime: " << fli.exposure_time_ms << " ms" << std::endl;
}

//*****************************************************************************
static std::string analysisDefinitionToString( const AnalysisDefinition& ad )
{

	std::cout << "  AnalysisIndex: " << ad.analysis_index << std::endl;
	std::cout << "          Label: " << ad.label << std::endl;

	std::cout << "     # Reagents: " << ad.num_reagents << std::endl;
	for ( uint8_t i = 0; i < ad.num_reagents; i++ )
	{
		std::cout << "   [" << i << "]: " << ad.reagent_indices[i] << std::endl;
	}
	std::cout << "   MixingCycles: " << ad.mixing_cycles << std::endl;
	std::cout << "# FL Parameters: " << ad.num_fl_illuminators << std::endl;
	for ( uint8_t i = 0; i < ad.num_reagents; i++ )
	{
		std::cout << "   [" << i << "]: " << flIlluminatorToString( ad.fl_illuminators[i] ) << std::endl;
	}
}


//*****************************************************************************
ScoutX::ScoutX (boost::asio::io_context& io_s) {

	ScoutX::pScoutX = this;
}

//*****************************************************************************
ScoutX::~ScoutX() {

}

//*****************************************************************************
void ScoutX::loginUser (std::string username, std::string password)
{

	doUserLogin( username, password );
	parameterStack_.clear();

#if 0	// For testing new API for Maggie...
	uuid__t sampleDataUuid = Uuid::FromStr ("37c00247-3624-41fe-bd6f-4b71ed6270c9");
	SampleDefinition *sampleDefinition;
	HawkeyeError he = GetSampleDefinitionBySampleId(sampleDataUuid, sampleDefinition);
	FreeSampleDefinition (sampleDefinition, 1);
#endif
}

//*****************************************************************************
HawkeyeError ScoutX::doUserLogin( std::string username, std::string password )
{
	HawkeyeError he = LoginUser( username.c_str(), password.c_str() );
	if ( he != HawkeyeError::eSuccess )
	{
		std::cout << "Login failed: " << GetErrorAsStr( he ) << std::endl;
	}
	else
	{
		std::cout << "Login successful as \"" << username << "\"..." << std::endl;
		currentUser_ = username;
		currentPassword_ = password;
	}
	return he;
}

//*****************************************************************************
void ScoutX::prompt() {

	switch (smState_) {
		default:
		case smCmdEntry:
			std::cout << menu << ": ";
			parameterStack_.clear();
			break;
		case smAnalysisOptions:
			std::cout << std::endl;
			std::cout << " GetFactoryAnalysisDefinitions(1)  | GetUserAnalysisDefinitions(2) | GetAllAnalysisDefinitions(3)" << std::endl;
			std::cout << " AddAnalysisDefinition(4)          | ModifyAnalysis(5)             | RemoveAnalysisDefinition(6)" << std::endl;
			std::cout << " GetSupportedAnalysisParamNames(7) | GetSupportedAnalysisCharacteristics(8)" << std::endl;
			std::cout << " ModifyBaseAnalysisDefinition(10)  | GetNameForCharacteristic(9): ";
			break;
		case smCellOptions:
			std::cout << std::endl;
			std::cout << " GetFactoryCellTypes(1) | GetUserCellTypes(2) | GetAllCellTypes(3) | AddCellType(4) | ModifyCellType(5)" << std::endl;
			std::cout << " RemoveCellType(6): ";
			break;
		case smQCOptions:
			std::cout << std::endl;
			std::cout << " AddQualityControl(1) | GetQualityControlList(2) | GetActiveQualityControlList(3): ";
			break;
		case smReagentPackOptions:
			std::cout << std::endl;
			std::cout << " Load(1)                         | Unload(2)                 | GetReagentDefinitions(3) | GetReagentContainerStatus(4)" << std::endl;
			std::cout << " GetReagentContainerStatusAll(5) | StartFlushFlowCell(6)     | CancelFlushFlowCell(7)   | StartDecontaminateFlowCell(8)" << std::endl;
			std::cout << " CancelDecontaminateFlowCell(9)  | StartReagentPackDrain(10) | CancelReagentPackDrain(11) | StartCleanFluidics(12): ";
			break;
		case smReagentPackDrain_Option:
			std::cout << std::endl;
			std::cout << " Valve Position to Drain (just one cycle). (1-5): ";
			break;
		case smReagentPackUnload_Option_0:
			std::cout << std::endl;
			std::cout << " How Many Containers to Unload (0-3): ";
			break;
		case smReagentPackUnload_Option_1:
			std::cout << std::endl;
			std::cout << " None(0) | Drain(1) | Purge Only(2): ";
			break;
		case smServiceOptions:
			std::cout << std::endl;
			std::cout << "GetCameraLampState(1) | SetCameraLampState(2)    | MoveSampleProbeUp(3) | MoveSampleProbeDown(4)" << std::endl;
			std::cout << "MoveReagentArmUp(5)   | MoveReagentArmDown(6)    | SetSerialNumber (7)  | SetSampleWellPosition(8)" << std::endl;
			std::cout << "EjectStage(9)         | StageLoadUnload(10): ";
			break;
		case smResultsOptions:
			std::cout << std::endl;
			//std::cout << " RetrieveWorklists(1)             | RetrieveWorklistList(2)             | RetrieveWorklist(3)" << std::endl;
			std::cout << " Not Supported(1)                 | Not Supported(2)                    | Not Supported(3)" << std::endl;
			std::cout << " RetrieveSampleRecord(4)          | RetrieveSampleRecords(5)            | RetrieveSampleRecordList(6)" << std::endl;
			std::cout << " RetrieveSampleImageSetRecord(7)  | RetrieveSampleImageSetRecords(8)    | RetrieveSampleImageSetRecordList(9)" << std::endl;
			std::cout << " RetrieveImage(10)                | RetrieveBWImage(11)                 | RetrieveAnnotatedImage(12)" << std::endl;
			std::cout << " RetrieveResultRecord(13)         | RetrieveResultRecords(14)           | RetrieveResultRecordList(15)" << std::endl;
			std::cout << " RetrieveResultSummary(16)        | RetrieveResultSummaryRecordList(17)" << std::endl;
			std::cout << " SignResultRecord(18)             | RetrieveSignatureDefinitions(19)" << std::endl;
			std::cout << " RemoveSignatureDefinition(20): ";
			//std::cout << " RetrieveDetailedMeasurementsForResultRecord(16) | RetrieveHistogramForResultRecord(17): ";
			break;
		case smAdminOptions:
			std::cout << std::endl;
			std::cout << " AddUser(1)                  | GetCurrentUser(2)       | GetUserCellTypes(3)      | GetUserAnalysisDefinitions (4)" << std::endl;
			std::cout << " GetUserProperty (5)         | ChangeUserPasword(6)    | SetUserDisplayName(7)    | SetUserFolder(8)" << std::endl;
			std::cout << " SetUserProperty(9)          | SetUserComment(10)      | SetUserEmail(11)         | SetUserCellTypeIndices(12)" << std::endl;
			std::cout << " ImportConfiguration(13)     | ExportConfiguration(14) | ExportInstrumentData(15) | DeleteSampleRecord(16)" << std::endl;
			std::cout << " SetDbBackupUserPassword(17) | Shutdown(18): ";
			break;
		case smUserOptions:
			std::cout << std::endl;
			std::cout << " UserLogin(1)        | --(2)                 | UserLogout(3)          | -- (4)             | ListAll(5)" << std::endl;
			std::cout << " ChangePassword(6)   | GetCellTypeIndices(7) | GetAnalysisIndices(8)  | GetDisplayName(9)  | GetFolder(10)" << std::endl;
			std::cout << " GetMyProperty(11)   | GetMyProperties(12)   | GetUserPermissions(13) | GetSystemStatus(14): ";
			break;
		case smWorklistOptions:
			std::cout << std::endl;
			std::cout << " SetEmptyCarouselWorklist(1)       | Clear curSampleSetIndex(2)  | GetCurrentSample(3)          | SetWorklistWithCarouselSampleSets(4)" << std::endl;
			std::cout << " Start(5)                          | Stop(6)                     | Pause(7)                     | Resume(8)" << std::endl;
			std::cout << " Status(9)                         | SkipItem(10)                | AddCarouselSampleSet2(11)    | AddCarouselSampleSet5(12)" << std::endl;
			std::cout << " AddCarouselSampleSet10(13)        | CancelSampleSet(14)         | SetEmptyPlateWorklist(15)    | AddPlateSampleSet2(16)" << std::endl;
			std::cout << " SetWorklistWithPlateSampleSet(17) | DeleteSampleSetTemplate(18) | GetSampleSetTemplateAndSampleList(19)" << std::endl;
			std::cout << " SaveSampleSetTemplate(20)         | GetSampleSetTemplateList(21): ";
			break;
		case smMiscOptions:
			std::cout << std::endl;
			std::cout << " GetSystemStatus(1)                | RetrieveAuditTrailLogRange(2)      | ArchiveAuditTrailLog(3)" << std::endl;
			std::cout << " ReadArchivedAuditTrailLog(4)      | RetrieveInstrumentErrorLogRange(5) | ArchiveInstrumentErrorLog(6)" << std::endl;
			std::cout << " ReadArchivedInstrumentErrorLog(7) | RetrieveSampleActivityLogRange(8)  | ArchiveSampleActivityLog(9)" << std::endl;
			std::cout << " ReadArchivedSampleActivityLog(10) | SyringePumpValveTest(11)           | RetrieveCalibrationActivityLogRange(12)" << std::endl;
			std::cout << " SetConcentrationCalibration(13)   | SystemStatusTest(14)               | StopSystemStatusTest(15)" << std::endl;
			std::cout << " TestSystemErrorResourceList(16)   | EmptyDisposalTray(17)              | SetOpticalHardwareConfig(18)" << std::endl;
			std::cout << " GetOpticalHardwareConfig(19)" << std::endl;
			break;
		case smDisplaynameEntry:
			std::cout << " Display name: ";
			break;
		case smUsernameEntry:
			std::cout << " Username: ";
			break;
		case smPasswordEntry:
			std::cout << " Password: ";
			break;
		case smAdminSetUserComment:
			std::cout << " Comment: ";
			break;
		case smAdminSetUserEmail:
			std::cout << " Email: ";
			break;
		case smPropertyEntry:
			std::cout << " Property: ";
			break;
		case smPropertyValueEntry:
			std::cout << " Property Value: ";
			break;
		case smFoldernameEntry:
			std::cout << " Folder name: ";
			break;
		case smReagentPackNumberEntry:
			std::cout << " Reagent pack number: ";
			break;
		case smSampleSetToCancelEntry:
			std::cout << " SampleSet to cancel: ";
			break;
		case smDatabaseOptions:
			std::cout << std::endl;
			std::cout << " DbConnectionOptions(1)         | DbCellTypeOptions(2)  | DbAnalysisDefinitionOptions(3)" << std::endl;
			std::cout << " DbAnalysisParameterOptions(4)  | DbUserOptions(5)      | DbRoleOptions(6)" << std::endl;
			std::cout << " DbInstrumentConfigOptions(7)   | DbLogOptions(8)       | DbObjAddition(9)" << std::endl;
			std::cout << " DbObjectUpdate(10)             | DbObjectRemoval(11)   | DbRecordList(12): ";
			break;
		case smDbConnectionOptions:
			std::cout << std::endl;
			std::cout << " SetDbIpAddress(1)       | SetDbListeningPort(2)      | SetDbName(3)" << std::endl;
			std::cout << " ShowDbConfig(4)         | InitializeDbConnection(5)" << std::endl;
			std::cout << " ConnectToDatabase(6)    | DisconnectDatabase(7)      | RecreateDatabase(8)" << std::endl;
			std::cout << " DbLoginAsInstrument(9)  | DbLoginAsAdmin(10)         | DbLoginAsUser(11)" << std::endl;
			std::cout << " DbInstrumentLogout(12)  | DbAdminLogout(13)          | DbUserLogout(14)   | LogoutAll(15): ";
			break;
		case smDbCellTypeOptions:
			std::cout << std::endl;
			std::cout << " ListFileCellTypes(1)  | ExportFileCellTypeToDB(2)  | CreateFileCellType(3)" << std::endl;
			std::cout << " ListDBCellType(4)     | CreateDBCellType(5): ";
			break;
		case smDbAnalysisDefinitionOptions:
			std::cout << std::endl;
			std::cout << " ListFileAnalysisDefinitions(1)  | ExportFileAnalysisParamToDB(2)  | CreateFileAnalysisParameter(3)" << std::endl;
			std::cout << " ConvertFileAnalysisDefToDb(4)   | EditDBAnalysisDefinitions(5)    | CreateDBAnalysisDefinition(6): ";
			break;
		case smDbAnalysisParameterOptions:
			std::cout << std::endl;
			std::cout << " ListFileAnalysisParameters(1)  | ExportFileAnalysisParamToDB(2)  | CreateFileAnalysisParameter(3)" << std::endl;
			std::cout << " ConvertFileParamToDb(4)        | ListDBAnalysisParameters(5)     | CreateDBAnalysisParameter(6): ";
			break;
		case smDbUserOptions:
			std::cout << std::endl;
			std::cout << " ListDBUsers(1)  | CreateDBUser(2)  | FindUserByName(3)  | UpdateUser(4)  | SetBackupUserPassword(5): ";
			break;
		case smDbRoleOptions:
			std::cout << std::endl;
			std::cout << " ListRoles(1)  | CreateRole(2): ";
			break;
		case smDbInstrumentConfigOptions:
			std::cout << std::endl;
			std::cout << " ListInstrumentConfigurations(1)     | ShowInstrumentConfiguration(2)" << std::endl;
			std::cout << " CreateDefaultInstrumentConfig(3)    | GetDefaultInstrumentConfig(4)    | UpdateInstrumentConfig(5)" << std::endl;
			std::cout << " ListInstrumentAutoFocusSettigns(6)  | ListInstrumentRfidSimSettings(7)" << std::endl;
			std::cout << " ListInstrumentEmailSettings(8)      | ListInstrumentActiveDirectorySettings(9)" << std::endl;
			std::cout << " ListInstrumentLanguages(10)         | ListInstrumentRunOptions(11)" << std::endl;
			std::cout << " SetAutomationStates(12)             | SetAutomationPort(13): ";
			break;
		case smDbLogOptions:
			std::cout << std::endl;
			std::cout << " ListAuditLog(1)   | AddAuditLogEntry(2)    | ModifyAuditLogEntry(3)    | ClearAuditLog(4)" << std::endl;
			std::cout << " ListErrorLog(5)   | AddErrorLogEntry(6)    | ModifyErrorLogEntry(7)    | ClearErrorLog(8)" << std::endl;
			std::cout << " ListSampleLog(9)  | AddSampleLogEntry(10)  | ModifySampleLogEntry(11)  | ClearSampleLog(12): ";
			break;
		case smDbObjAdditionOptions:
			std::cout << std::endl;
			std::cout << " AddWorklistRecord(1)        | AddSampleSetRecord(2)      | AddSampleItemRecord(3)" << std::endl;
			std::cout << " AddSampleRecord(4)          | AddAnalysisRecord(5)       | AddSummaryResultRecord(6)" << std::endl;
			std::cout << " AddDetailedResultRecord(7)  | AddCellTypeRecord(8)       | AddSignatureDefRecord(9)" << std::endl;
			std::cout << " AddQcProcessRecord(10)      | AddReagentTypeRecord(11)   | AddSchedulerRecord(12): ";
			break;
		case smDbObjUpdateOptions:
			std::cout << std::endl;
			std::cout << " UpdateWorklistRecord(1)        | UpdateSampleSetRecord(2)      | UpdateSampleItemRecord(3)" << std::endl;
			std::cout << " UpdateSampleRecord(4)          | UpdateAnalysisRecord(5)       | UpdateSummaryResultRecord(6)" << std::endl;
			std::cout << " UpdateDetailedResultRecord(7)  | UpdateCellTypeRecord(8)       | UpdateSignatureDefRecord(9)" << std::endl;
			std::cout << " UpdateQcProcessRecord(10)      | UpdateReagentTypeRecord(11)   | UpdateSchedulerRecord(12): ";
			break;
		case smDbObjRemovalOptions:
			std::cout << std::endl;
 			std::cout << " RemoveWorklistRecord(1)         | RemoveWorklistRecordList(2)" << std::endl;
			std::cout << " RemoveSampleSetRecord(3)        | RemoveSampleSetRecordList(4)" << std::endl;
			std::cout << " RemoveSampleItemRecord(5)       | RemoveSampleItemRecordList(6)" << std::endl;
			std::cout << " RemoveSampleRecord(7)           | RemoveSampleRecordList(8)" << std::endl;
			std::cout << " RemoveImageSetRecord(9)         | RemoveImageSetRecordList(10)" << std::endl;
			std::cout << " RemoveImageSeqRecord(11)        | RemoveImageSeqRecordList(12)" << std::endl;
			std::cout << " RemoveImageRecord(13)           | RemoveImageRecordList(14)" << std::endl;
			std::cout << " RemoveAnalysisRecord(15)        | RemoveAnalysisRecordList(16)" << std::endl;
			std::cout << " RemoveSummaryResultRecord(17)   | RemoveSummaryResultRecordList(18)" << std::endl;
			std::cout << " RemoveDetailedResultRecord(19)  | RemoveDetailedResultRecordList(20)" << std::endl;
			std::cout << " RemoveCellType(21)              | RemoveAnalysisDefinitions(22)" << std::endl;
			std::cout << " RemoveAnalysisParameters(23)    | RemoveSignatureDefRecord(24)" << std::endl;
			std::cout << " RemoveReagentInfoRecord(25)     | RemoveSchedulerRecord(26):  ";
			break;
		case smDbRecordListOptions:
			std::cout << std::endl;
			std::cout << " ListWorklists(1)      | ListWorklistTemplates(2)          | ListSampleSets(3)             | ListSampleSetTemplates(4)" << std::endl;
			std::cout << " ListSampleItems(5)    | ListSampleItemTemplates(6)" << std::endl;
			std::cout << " ListSamples(11)       | ListImageSets(12)                 | ListImageSequences(13)        | ListImages(14)" << std::endl;
			std::cout << " ListAnalyses(15)      | ListSummaryResults(16)            | ListDetailedResults(17)" << std::endl;
			std::cout << " ListCellType(21)      | ListAnalysisDefinitions(22)       | ListAnalysisParameters(23)" << std::endl;
			std::cout << " ListUsers(24)         | ListRoles(25)                     | ListSignatureDefinitions(26)" << std::endl;
			std::cout << " ListCalibrations(27)  | ListReagents(28)                  | ListInstrumentConfigurations(29)" << std::endl;
			std::cout << " ListSchedulerRecords(30): ";
			break;
	}
}

//*****************************************************************************
void ScoutX::showHelp() {
	/*
	cmdAnalysis = 'A',
	cmdBP = 'B',
	cmdCells = 'C',
	cmdAdmin = 'D',

	cmdDatabase = 'E',

	cmdMiscellaneous = 'M',
	cmdQC = 'Q',
	cmdReagentPack = 'R',
	cmdService = 'S',
	cmdResults = 'T',
	cmdUser = 'U',
	cmdWorklist = 'W',
	cmdQuit = 'X',
	cmdHelp = '?',
	*/
	std::cout << std::endl << "Usage:\t" << menu << std::endl;
	std::cout << "\t" << (char)cmdAnalysis << " : Analysis commands" << std::endl;
	std::cout << "\t" << (char)cmdCells << " : CellType commands" << std::endl;
	std::cout << "\t" << (char)cmdAdmin << " : Admin commands" << std::endl;

	std::cout << "\t" << (char)cmdDatabase << " : Database command section options" << std::endl;
	std::cout << "\t  : Database commands access the database interface DLL directly, not through HawkeyeCore" << std::endl;
	std::cout << "\t  : Database sub-menu commands: " << std::endl;
	std::cout << "\t      : Database connection commands;" << std::endl;
	std::cout << "\t      : Database Cell Type commands;" << std::endl;
	std::cout << "\t      : Database Analysis Definition commands;" << std::endl;
	std::cout << "\t      : Database Analysis Parameter commands;" << std::endl;
	std::cout << "\t      : Database User commands;" << std::endl;
	std::cout << "\t      : Database Role commands;" << std::endl;
	std::cout << "\t      : Database record commands;" << std::endl;

	std::cout << "\t" << (char) cmdMiscellaneous << " : Misc commands" << std::endl;
	std::cout << "\t" << (char) cmdQC << " : QualityControl commands" << std::endl;
	std::cout << "\t" << (char) cmdReagentPack << " : ReagentPack commands" << std::endl;
	std::cout << "\t" << (char) cmdService << " : Service commands" << std::endl;
	std::cout << "\t" << (char) cmdResults << " : Results commands" << std::endl;
	std::cout << "\t" << (char) cmdUser << " : User commands" << std::endl;
	std::cout << "\t" << (char) cmdWorklist << " : Worklist commands" << std::endl;

	std::cout << "\t" << (char)cmdQuit <<" : Exit the program" << std::endl;
	std::cout << "\t" << (char)cmdHelp << " : Display this help screen" << std::endl;
	std::cout << std::endl << std::endl;
}

bool ScoutX::GetDbDataTypeEnumString( int datatype, std::string& typestr )
{
	DBApi::eListType eDataType = static_cast<DBApi::eListType> ( datatype );
	bool typeValid = true;
	std::string workStr = "";

	switch ( eDataType )
	{
		case DBApi::eListType::NoListType:
		{
			workStr = "No data record type";
			break;
		}

		case DBApi::eListType::WorklistList:
		{
			workStr = "Worklist record type";
			break;
		}

		case DBApi::eListType::SampleSetList:
		{
			workStr = "SampleSet record type";
			break;
		}

		case DBApi::eListType::SampleItemList:
		{
			workStr = "SampleItem record type";
			break;
		}

		case DBApi::eListType::SampleList:
		{
			workStr = "Sample record type";
			break;
		}

		case DBApi::eListType::AnalysisList:
		{
			workStr = "Analysis record type";
			break;
		}

		case DBApi::eListType::SummaryResultList:
		{
			workStr = "SumaryResult record type";
			break;
		}

		case DBApi::eListType::DetailedResultList:
		{
			workStr = "DetailedResult record type";
			break;
		}

		case DBApi::eListType::ImageResultList:
		{
			workStr = "ImageResult record type";
			break;
		}

		case DBApi::eListType::SResultList:
		{
			workStr = "SResult record type";
			break;
		}

		case DBApi::eListType::ClusterDataList:
		{
			workStr = "ClusterData record type";
			break;
		}

		case DBApi::eListType::BlobDataList:
		{
			workStr = "BlobData record type";
			break;
		}

		case DBApi::eListType::ImageSetList:
		{
			workStr = "ImageSet record type";
			break;
		}

		case DBApi::eListType::ImageSequenceList:
		{
			workStr = "ImageSequence record type";
			break;
		}

		case DBApi::eListType::ImageList:
		{
			workStr = "Image record type";
			break;
		}

		case DBApi::eListType::CellTypeList:
		{
			workStr = "CellType record type";
			break;
		}

		case DBApi::eListType::ImageAnalysisParamList:
		{
			workStr = "ImageAnalysisParam record type";
			break;
		}

		case DBApi::eListType::AnalysisInputSettingsList:
		{
			workStr = "AnalysisInputSettings record type";
			break;
		}

		case DBApi::eListType::ImageAnalysisCellIdentParamList:
		{
			workStr = "ImageAnalysisCellIdentParam record type";
			break;
		}

		case DBApi::eListType::AnalysisDefinitionList:
		{
			workStr = "AnalysisDefinition record type";
			break;
		}

		case DBApi::eListType::AnalysisParamList:
		{
			workStr = "AnalysisParam record type";
			break;
		}

		case DBApi::eListType::IlluminatorList:
		{
			workStr = "Illuminator record type";
			break;
		}

		case DBApi::eListType::RolesList:
		{
			workStr = "Roles record type";
			break;
		}

		case DBApi::eListType::UserList:
		{
			workStr = "User record type";
			break;
		}

		case DBApi::eListType::UserPropertiesList:
		{
			workStr = "UserProperties record type";
			break;
		}

		case DBApi::eListType::SignatureDefList:
		{
			workStr = "SignatureDefinition record type";
			break;
		}

		case DBApi::eListType::ReagentInfoList:
		{
			workStr = "ReagentInfo record type";
			break;
		}

		case DBApi::eListType::WorkflowList:
		{
			workStr = "Workflow record type";
			break;
		}

		case DBApi::eListType::BioProcessList:
		{
			workStr = "BioProcess record type";
			break;
		}

		case DBApi::eListType::QcProcessList:
		{
			workStr = "QcProcess record type";
			break;
		}

		case DBApi::eListType::CalibrationsList:
		{
			workStr = "Calibration record type";
			break;
		}

		case DBApi::eListType::InstrumentConfigList:
		{
			workStr = "InstrumentConfig record type";
			break;
		}

		case DBApi::eListType::LogEntryList:
		{
			workStr = "LogEntry record type";
			break;
		}

		case DBApi::eListType::SchedulerConfigList:
		{
			workStr = "SchedulerConfig record type";
			break;
		}

		default:
		{
			workStr = "Unknown Data Type";
			typeValid = false;
			break;
		}
	}

	typestr = boost::str( boost::format( "%s  (%ld)" ) % workStr % datatype);
	return typeValid;
}

void ScoutX::ShowDbFilterEnum( void )
{
	int colCnt = 0;
	std::string lineStr = "";
	std::string typeStr = "";

	for ( int i = static_cast<int>(DBApi::eListFilterCriteria::FilterNotDefined); i < static_cast<int>(DBApi::eListFilterCriteria::MaxFilterTypes); i++ )
	{
		typeStr.clear();

		GetDbFilterEnumString( i, typeStr );

		lineStr.append( boost::str( boost::format( "%26s : %2d    " ) % typeStr % i ) );

		if ( ++colCnt >= 3 )
		{
			std::cout << "\t" << lineStr << std::endl;
			lineStr.clear();
			colCnt = 0;
		}
	}

	if ( colCnt > 0 )
	{
		std::cout << "\t" << lineStr << std::endl;
	}
}

bool ScoutX::GetDbFilterEnumString( int filtertype, std::string& typestr )
{
	DBApi::eListFilterCriteria eType = static_cast<DBApi::eListFilterCriteria> ( filtertype );
	bool typeValid = true;

	switch ( eType )
	{
		case DBApi::eListFilterCriteria::FilterNotDefined:
		{
			typestr = "FilterNotDefined";
			break;
		}

		case DBApi::eListFilterCriteria::NoFilter:
		{
			typestr = "NoFilter";
			break;
		}

		case DBApi::eListFilterCriteria::IdFilter:
		{
			typestr = "IdFilter";
			break;
		}

		case DBApi::eListFilterCriteria::IdNumFilter:
		{
			typestr = "IdNumFilter";
			break;
		}

		case DBApi::eListFilterCriteria::IndexFilter:
		{
			typestr = "IndexFilter";
			break;
		}

		case DBApi::eListFilterCriteria::ItemNameFilter:
		{
			typestr = "ItemNameFilter";
			break;
		}

		case DBApi::eListFilterCriteria::LabelFilter:
		{
			typestr = "LabelFilter";
			break;
		}

		case DBApi::eListFilterCriteria::StatusFilter:
		{
			typestr = "StatusFilter";
			break;
		}

		case DBApi::eListFilterCriteria::OwnerFilter:
		{
			typestr = "OwnerFilter";
			break;
		}

		case DBApi::eListFilterCriteria::ParentFilter:
		{
			typestr = "ParentFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CarrierFilter:
		{
			typestr = "CarrierFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CreationDateFilter:
		{
			typestr = "CreationDateFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CreationDateRangeFilter:
		{
			typestr = "CreationDateRangeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CreationUserIdFilter:
		{
			typestr = "CreationUserIdFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CreationUserNameFilter:
		{
			typestr = "CreationUserNameFilter";
			break;
		}

		case DBApi::eListFilterCriteria::ModifyDateFilter:
		{
			typestr = "ModifyDateFilter";
			break;
		}

		case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
		{
			typestr = "ModifyDateRangeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::RunDateFilter:
		{
			typestr = "RunDateFilter";
			break;
		}

		case DBApi::eListFilterCriteria::RunDateRangeFilter:
		{
			typestr = "RunDateRangeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::RunUserIdFilter:
		{
			typestr = "RunUserIdFilter";
			break;
		}

		case DBApi::eListFilterCriteria::RunUserNameFilter:
		{
			typestr = "RunUserNameFilter";
			break;
		}

		case DBApi::eListFilterCriteria::SampleAcquiredFilter:
		{
			typestr = "SampleAcquiredFilter";
			break;
		}

		case DBApi::eListFilterCriteria::AcquisitionDateFilter:
		{
			typestr = "AcquisitionDateFilter";
			break;
		}

		case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
		{
			typestr = "AcquisitionDateRangeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::SampleIdFilter:
		{
			typestr = "SampleIdFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CellTypeFilter:
		{
			typestr = "CellTypeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::LotFilter:
		{
			typestr = "LotFilter";
			break;
		}

		case DBApi::eListFilterCriteria::QcFilter:
		{
			typestr = "QcFilter";
			break;
		}

		case DBApi::eListFilterCriteria::InstrumentFilter:
		{
			typestr = "InstrumentFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CommentsFilter:
		{
			typestr = "CommentsFilter";
			break;
		}

		case DBApi::eListFilterCriteria::RoleFilter:
		{
			typestr = "RoleFilter";
			break;
		}

		case DBApi::eListFilterCriteria::UserTypeFilter:
		{
			typestr = "UserTypeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::CalTypeFilter:
		{
			typestr = "CalTypeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::LogTypeFilter:
		{
			typestr = "LogTypeFilter";
			break;
		}

		case DBApi::eListFilterCriteria::SinceLastDateFilter:
		{
			typestr = "SinceLastDateFilter";
			break;
		}

		default:
		{
			typestr = "UnknownFilter";
			typeValid = false;
			break;
		}
	}

	return typeValid;
}

bool ScoutX::IsDbFilterStringValue( DBApi::eListFilterCriteria filtertype )
{
	bool strType = false;

	switch ( filtertype )
	{
		case DBApi::eListFilterCriteria::IdFilter:
		case DBApi::eListFilterCriteria::ItemNameFilter:
		case DBApi::eListFilterCriteria::LabelFilter:
		case DBApi::eListFilterCriteria::OwnerFilter:
		case DBApi::eListFilterCriteria::ParentFilter:
		case DBApi::eListFilterCriteria::CreationUserNameFilter:
		case DBApi::eListFilterCriteria::RunUserIdFilter:
		case DBApi::eListFilterCriteria::RunUserNameFilter:
		case DBApi::eListFilterCriteria::SampleIdFilter:
		case DBApi::eListFilterCriteria::LotFilter:
		case DBApi::eListFilterCriteria::InstrumentFilter:
		case DBApi::eListFilterCriteria::CommentsFilter:
		{
			strType = true;
			break;
		}
	}

	return strType;
}

void ScoutX::ShowDbListSortEnum( void )
{
	int colCnt = 0;
	std::string lineStr = "";
	std::string typeStr = "";

	for ( int i = static_cast<int>( DBApi::eListSortCriteria::SortNotDefined ); i < static_cast<int>( DBApi::eListSortCriteria::MaxSortTypes ); i++ )
	{
		typeStr.clear();

		GetDbSortEnumString( i, typeStr );

		lineStr.append( boost::str( boost::format( "%16s : %2d    " ) % typeStr % i ) );

		if ( ++colCnt >= 3 )
		{
			std::cout << "\t" << lineStr << std::endl;
			lineStr.clear();
			colCnt = 0;
		}
	}

	if ( colCnt > 0 )
	{
		std::cout << "\t" << lineStr << std::endl;
	}

}

bool ScoutX::GetDbSortEnumString( int sorttype, std::string& typestr )
{
	DBApi::eListSortCriteria eType = static_cast<DBApi::eListSortCriteria> ( sorttype );
	bool typeValid = true;

	switch ( eType )
	{
		case DBApi::eListSortCriteria::SortNotDefined:
		{
			typestr = "SortNotDefined";
			break;
		}

		case DBApi::eListSortCriteria::NoSort:
		{
			typestr = "NoSort";
			break;
		}

		case DBApi::eListSortCriteria::GuidSort:
		{
			typestr = "GuidSort";
			break;
		}

		case DBApi::eListSortCriteria::IdNumSort:
		{
			typestr = "IdNumSort";
			break;
		}

		case DBApi::eListSortCriteria::IndexSort:
		{
			typestr = "IndexSort";
			break;
		}

		case DBApi::eListSortCriteria::OwnerSort:
		{
			typestr = "OwnerSort";
			break;
		}

		case DBApi::eListSortCriteria::ParentSort:
		{
			typestr = "ParentSort";
			break;
		}

		case DBApi::eListSortCriteria::CarrierSort:
		{
			typestr = "CarrierSort";
			break;
		}

		case DBApi::eListSortCriteria::CreationDateSort:
		{
			typestr = "CreationDateSort";
			break;
		}

		case DBApi::eListSortCriteria::CreationUserSort:
		{
			typestr = "CreationUserSort";
			break;
		}

		case DBApi::eListSortCriteria::RunDateSort:
		{
			typestr = "RunDateSort";
			break;
		}

		case DBApi::eListSortCriteria::UserIdSort:
		{
			typestr = "UserIdSort";
			break;
		}

		case DBApi::eListSortCriteria::UserIdNumSort:
		{
			typestr = "UserIdNumSort";
			break;
		}

		case DBApi::eListSortCriteria::UserNameSort:
		{
			typestr = "UserNameSort";
			break;
		}

		case DBApi::eListSortCriteria::SampleIdSort:
		{
			typestr = "SampleIdSort";
			break;
		}

		case DBApi::eListSortCriteria::SampleIdNumSort:
		{
			typestr = "SampleIdNumSort";
			break;
		}

		case DBApi::eListSortCriteria::LabelSort:
		{
			typestr = "LabelSort";
			break;
		}

		case DBApi::eListSortCriteria::RxTypeNumSort:
		{
			typestr = "RxTypeNumSort";
			break;
		}

		case DBApi::eListSortCriteria::RxLotNumSort:
		{
			typestr = "RxLotNumSort";
			break;
		}

		case DBApi::eListSortCriteria::RxLotExpSort:
		{
			typestr = "RxLotExpSort";
			break;
		}

		case DBApi::eListSortCriteria::ItemNameSort:
		{
			typestr = "ItemNameSort";
			break;
		}

		case DBApi::eListSortCriteria::CellTypeSort:
		{
			typestr = "CellTypeSort";
			break;
		}

		case DBApi::eListSortCriteria::InstrumentSort:
		{
			typestr = "InstrumentSort";
			break;
		}

		case DBApi::eListSortCriteria::QcSort:
		{
			typestr = "QcSort";
			break;
		}

		case DBApi::eListSortCriteria::CalTypeSort:
		{
			typestr = "CalTypeSort";
			break;
		}

		case DBApi::eListSortCriteria::LogTypeSort:
		{
			typestr = "LogTypeSort";
			break;
		}

		default:
		{
			typestr = "UnknownSort";
			typeValid = false;
			break;
		}
	}

	return typeValid;
}

bool ScoutX::IsDbSortStringValue( DBApi::eListSortCriteria filtertype )
{
	bool strType = false;

	switch ( filtertype )
	{
		case DBApi::eListSortCriteria::UserNameSort:
		case DBApi::eListSortCriteria::LabelSort:
		case DBApi::eListSortCriteria::RxLotNumSort:
		case DBApi::eListSortCriteria::ItemNameSort:
		case DBApi::eListSortCriteria::InstrumentSort:
		{
			strType = true;
			break;
		}
	}

	return strType;
}

// remove leading and trailing target characters from std::strings
void ScoutX::TrimStr( std::string trim_chars, std::string& tgtstr )
{
	if ( trim_chars.length() == 0 || tgtstr.length() <= 0 )
	{
		return;
	}

	char tgtChar;
	int trimCharIdx = 0;
	size_t trimCharSize = trim_chars.length();
	bool done = false;

	// first trim the trailing characters of the string if necessary;
	size_t tgtIdx = 0;
	do
	{
		tgtIdx = tgtstr.length() - 1;
		tgtChar = tgtstr.at( tgtIdx );

		trimCharIdx = 0;
		do
		{
			if ( tgtChar == trim_chars[trimCharIdx] )
			{
				tgtstr.erase( tgtIdx, 1 );
				break;
			}

			if ( ++trimCharIdx >= trimCharSize )
			{
				done = true;
			}
		} while ( !done );
	} while ( tgtstr.length() > 0 && !done );

	// now remove any target leading characters if any remaining string...
	if ( tgtstr.length() > 0 )
	{
		done = false;
		do
		{
			tgtChar = tgtstr.at( 0 );

			trimCharIdx = 0;
			do
			{
				if ( ( tgtChar == trim_chars[trimCharIdx] ) )
				{
					tgtstr.erase( 0, 1 );
					break;
				}
				if ( ++trimCharIdx >= trimCharSize )
				{
					done = true;
				}
			} while ( !done );
		} while ( tgtstr.length() > 0 && !done );
	}
}

// remove leading and trailing spaces from std::strings
void ScoutX::TrimWhiteSpace( std::string& tgtstr )
{
	std::string trimChars = " \t";

	TrimStr( trimChars, tgtstr );
}

int32_t ScoutX::TokenizeStr( std::vector<std::string>& tokenlist, std::string& parsestr,
							 std::string sepstr, std::string sepchars )
{
	size_t tokenCnt = 0;
	size_t chCnt = 0;
	size_t sepLen = sepstr.length();

	if ( sepchars.length() > 0 )
	{
		chCnt = sepchars.length();	    // check count of standard characcter array length
	}

	// can't have both as non-zero length, and can't have both as zero length
	if ( ( ( sepLen > 0 ) && ( chCnt == 0 ) ) || ( ( sepLen == 0 ) && ( chCnt > 0 ) ) )
	{
		std::string::size_type tokenLen = 0;
		std::string::size_type startPos = 0;
		std::string::size_type endPos;
		std::string::size_type parseLen = parsestr.length();
		uint32_t chIdx = 0;

		tokenlist.clear();

		do
		{
			if ( chCnt > 0 )
			{
				chIdx = 0;
				endPos = std::string::npos;
				// look for ANY of the supplied token seperator characters...
				while ( ( sepchars[chIdx] != 0 ) && ( chIdx < chCnt ) && ( endPos == std::string::npos ) )
				{
					endPos = parsestr.find( sepchars[chIdx], startPos );
					if ( endPos == startPos )		// may happen if two identical separator characters in a row (typically spaces)
					{
						startPos++;
						endPos = std::string::npos;
					}
					else
					{
						chIdx++;
					}
				}
			}
			else
			{
				if ( sepstr.length() > 1 )
				{
					endPos = parsestr.find( sepstr, startPos );
				}
				else
				{
					endPos = parsestr.find( sepstr.at( 0 ), startPos );
				}
			}

			if ( endPos == std::string::npos && startPos < parseLen )	// handle search on final string segment...
			{
				endPos = parseLen;
			}

			if ( endPos != std::string::npos )
			{
				if ( endPos > startPos )
				{
					tokenLen = endPos - startPos;
					if ( tokenLen == 1 )
					{	// ensure this is not a token containing only the seperator character
						for ( int chkIdx = 0; chkIdx < chCnt && tokenLen > 0; chkIdx++ )
						{
							if ( parsestr.at( startPos ) == sepchars[chkIdx] )
							{
								tokenLen--;
							}
						}
					}

					if ( tokenLen >= 1 )
					{
						tokenlist.push_back( parsestr.substr( startPos, tokenLen ) );
						TrimWhiteSpace( tokenlist.back() );
					}

					if ( ( chCnt > 0 ) || ( sepstr.length() == 1 ) )
					{
						endPos++;   // account for the found token seperator character in the string being parsed
					}
					else
					{
						endPos += sepstr.length();
					}
				}
				else if ( endPos <= startPos || endPos == parseLen || startPos == parseLen )
				{
					endPos = std::string::npos;
				}

				startPos = endPos;
			}
		} while ( endPos != std::string::npos );
	}
	tokenCnt = tokenlist.size();

	return (int32_t) tokenCnt;
}

// IGNORES special command characters for 'quit' and 'help'!
bool ScoutX::GetStringInput( std::string& entry_str, std::string prompt_str, std::string length_warning, int min_length, bool hide_entry )
{
	bool input_quit = false;
	bool process_line = false;
	bool emptyOk = length_warning.empty();
	bool entryDone = false;

	smValueLine_ = entry_str;
	do
	{
		process_line = false;
		std::cout << prompt_str;
		std::cout << smValueLine_;

		do
		{
			process_line = readLine( input_quit, IGNORE_CMDS, hide_entry );
		} while ( !input_quit && !process_line );

		if ( input_quit )
		{
			return input_quit;
		}

		if ( !emptyOk && smValueLine_.length() <= min_length )
		{
			if ( min_length == 0 )
			{
				std::cout << "WARNING: Cannot be blank!" << std::endl;
			}
			else
			{
				std::cout << length_warning << std::endl;
			}
		}
		else
		{
			entry_str = smValueLine_;
			entryDone = true;
		}
		std::cout << std::endl;
	} while ( !entryDone );
	std::cout << std::endl;

	return false;
}

// input for specific responses
bool ScoutX::GetYesOrNoResponse( std::string& entry_str, std::string prompt_str, bool emptyOk )
{
	bool input_quit = false;
	bool entryDone = false;

	std::string entryStr = entry_str;
	std::string lengthWarning = "";

	do
	{
		entryDone = false;
		input_quit = GetStringInput( entryStr, prompt_str, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 )
		{
			boost::to_upper( entryStr );
			if ( ( strnicmp( entryStr.c_str(), "Y", 2 ) == 0 ) ||
				 ( strnicmp( entryStr.c_str(), "YES", 4 ) == 0 ) ||
				 ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 ) ||
				 ( strnicmp( entryStr.c_str(), "NO", 3 ) == 0 ) )
			{
				entryDone = true;
				if ( strnicmp( entryStr.c_str(), "YES", 4 ) == 0 )
				{
					entryStr = "Y";
				}
				else if ( strnicmp( entryStr.c_str(), "NO", 3 ) == 0 )
				{
					entryStr = "N";
				}
			}
			else
			{
				std::cout << "WARNING: Illegal response!" << std::endl;
				entryStr.clear();
			}
		}
		else if ( emptyOk )
		{
			entryDone = true;
		}

	} while ( !entryDone );

	entry_str = entryStr;

	return false;
}

// input for specific responses
bool ScoutX::GetTrueOrFalseResponse( std::string& entry_str, std::string prompt_str, bool emptyOk )
{
	bool input_quit = false;
	bool entryDone = false;

	std::string entryStr = entry_str;
	std::string lengthWarning = "";

	do
	{
		entryDone = false;
		input_quit = GetStringInput( entryStr, prompt_str, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 )
		{
			boost::to_upper( entryStr );
			if ( ( strnicmp( entryStr.c_str(), "T", 2 ) == 0 ) ||
				( strnicmp( entryStr.c_str(), "TRUE", 5 ) == 0 ) ||
				 ( strnicmp( entryStr.c_str(), "F", 2 ) == 0 ) ||
				 ( strnicmp( entryStr.c_str(), "FALSE", 6 ) == 0 ) )
			{
				entryDone = true;
				if ( strnicmp( entryStr.c_str(), "TRUE", 5 ) == 0 )
				{
					entryStr = "T";
				}
				else if ( strnicmp( entryStr.c_str(), "FALSE", 6 ) == 0 )
				{
					entryStr = "F";
				}
			}
			else
			{
				std::cout << "WARNING: Illegal response!" << std::endl;
				entryStr.clear();
			}
		}
		else if ( emptyOk )
		{
			entryDone = true;
		}
	} while ( !entryDone );

	entry_str = entryStr;

	return false;
}

void ScoutX::ShowDbFilterList( std::vector<DBApi::eListFilterCriteria>& filtertypes,
							   std::vector<std::string>& filtercompareops,
							   std::vector<std::string>& filtercomparevals,
							   system_TP lastrundate )
{
	size_t listSize = filtertypes.size();
	int filterType = 0;
	std::string workStr = "";
	std::string filterTypeStr = "";
	std::string compareOpStr = "";
	std::string compareValStr = "";
	std::string fmtStr = "";
	std::string tgtStr = "";
	system_TP zeroTP = {};

	std::cout << "\tFilters:" << std::endl;
	workStr.clear();
	for ( size_t listIdx = 0; listIdx < listSize; listIdx++ )
	{
		filterTypeStr.clear();
		compareOpStr.clear();
		compareValStr.clear();

		filterType = static_cast<int>( filtertypes.at( listIdx ) );
		if ( GetDbFilterEnumString( filterType, filterTypeStr ) )
		{
			fmtStr = boost::str( boost::format( "( %s )" ) % filterTypeStr );
			workStr = boost::str( boost::format( "\t  %2d) %2d %-30s    " ) % (listIdx+1)  % filterType % fmtStr );
			if ( filtertypes.at( listIdx ) == DBApi::eListFilterCriteria::SinceLastDateFilter )
			{
				if ( lastrundate == zeroTP )
				{
					compareOpStr = " ";
					fmtStr = "(never run)";
					compareValStr = fmtStr;
				}
				else
				{
					compareOpStr = "'>'";
					fmtStr = ChronoUtilities::ConvertToString( lastrundate, "%Y-%m-%d %H:%M:%S" );
					compareValStr = boost::str( boost::format( "'%s'" ) % fmtStr );
				}
			}
			else
			{
				compareOpStr = boost::str( boost::format( "'%s'" ) % filtercompareops.at( listIdx ) );
				compareValStr = boost::str( boost::format( "'%s'" ) % filtercomparevals.at( listIdx ) );
			}
			workStr.append( boost::str( boost::format( "%-6s    " ) % compareOpStr ) );
			workStr.append( boost::str( boost::format( "%-6s    " ) % compareValStr ) );
		}
		std::cout << workStr << std::endl;
	}
	std::cout << std::endl;
}

bool ScoutX::GetDbListFilter( std::vector<DBApi::eListFilterCriteria>& filtertypes,
							  std::vector<std::string>& filtercompareops,
							  std::vector<std::string>& filtercomparevals )
{
	bool input_quit = false;
	bool process_line = false;
	size_t filterListSize = filtertypes.size();

	std::string entrystr = "";
	std::string promptStr = "";

	if ( filterListSize > 0 )
	{
		promptStr = "Add additional Filters? (y/N):  ";
	}
	else
	{
		promptStr = "Select Filters? (y/N):  ";
	}

	entrystr.clear();
	input_quit = GetYesOrNoResponse( entrystr, promptStr );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entrystr.length() == 0 )
	{
		entrystr = "N";
	}

	bool modify_filters = false;

	if ( strnicmp( entrystr.c_str(), "N", 2 ) == 0 )
	{
		if ( filterListSize == 0 )
		{
			return input_quit;
		}

		promptStr = "Clear Filters? (y/N):  ";

		entrystr.clear();
		input_quit = GetYesOrNoResponse( entrystr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entrystr.length() == 0 )
		{
			entrystr = "N";
		}

		if ( strnicmp( entrystr.c_str(), "Y", 2 ) == 0 )
		{
			filtertypes.clear();
			filtercompareops.clear();
			filtercomparevals.clear();
			return input_quit;
		}

		promptStr = "Modify Filters? (y/N):  ";

		entrystr.clear();
		input_quit = GetYesOrNoResponse( entrystr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entrystr.length() == 0 )
		{
			entrystr = "N";
		}

		if ( strnicmp( entrystr.c_str(), "N", 2 ) == 0 )
		{
			std::cout << std::endl;
			return input_quit;
		}

		modify_filters = true;
	}

	int64_t listIdxNum = -1;
	bool done = false;

	do
	{
		ShowDbFilterEnum();
		std::cout << std::endl;

		if ( filtertypes.size() > 0 )
		{
			ShowDbFilterList( filtertypes, filtercompareops, filtercomparevals );

			if ( modify_filters )
			{
				do
				{
					input_quit = SelectDbListIdNum( filterListSize, listIdxNum );
					if ( input_quit )
					{
						std::cout << std::endl;
						return input_quit;
					}

					listIdxNum--;	// assume the number is entered as an line id not an index...
					if ( listIdxNum > filterListSize || listIdxNum < 0)
					{
						std::cout << "Illegal filter index entered" << std::endl;
						listIdxNum = -1;
					}
				} while ( listIdxNum < 0 );
			}
		}

		promptStr = "Delete Filter entry? (y/N):  ";

		entrystr.clear();
		input_quit = GetYesOrNoResponse( entrystr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entrystr.length() == 0 )
		{
			entrystr = "N";
		}

		DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
		std::string filtertypestr = "";
		std::string compareopstr = "";
		std::string tgtvaluestr = "";

		if ( strnicmp( entrystr.c_str(), "N", 2 ) == 0 )
		{
			std::string date1str = "";
			std::string date2str = "";
			bool filterTypeOk = true;

			do
			{
				filtertype = DBApi::eListFilterCriteria::NoFilter;

				compareopstr.clear();
				tgtvaluestr.clear();
				date1str.clear();
				date2str.clear();
				entrystr.clear();

				do
				{
					filterTypeOk = true;
					filtertypestr.clear();
					std::cout << "Filter Type (or RETURN for done): ";
					smValueLine_.clear();
					if ( modify_filters && listIdxNum >= 0 )
					{
						smValueLine_ = boost::str( boost::format( "%d" ) % static_cast<int32_t>( filtertypes.at( listIdxNum ) ) );
						std::cout << smValueLine_;
					}
					process_line = false;
					do
					{
						process_line = readLine( input_quit );
					} while ( !input_quit && !process_line );

					if ( input_quit )
					{
						std::cout << std::endl;
						return input_quit;
					}

					if ( smValueLine_.length() > 0 )
					{
						int type = stol( smValueLine_ );
						if ( GetDbFilterEnumString( type, filtertypestr ) )
						{
							filtertype = static_cast<DBApi::eListFilterCriteria>( type );
							std::cout << " ( " << filtertypestr << " )" << std::endl;
						}
						else
						{
							std::cout << "Invalid Entry!" << std::endl;
							filterTypeOk = false;
						}
					}
				} while ( !filterTypeOk );

				if ( filtertypestr.length() > 0 )
				{
					if ( filtertype == DBApi::eListFilterCriteria::CreationDateRangeFilter ||
						 filtertype == DBApi::eListFilterCriteria::ModifyDateRangeFilter ||
						 filtertype == DBApi::eListFilterCriteria::RunDateRangeFilter )
					{
						entrystr = "=";		// not used, but required for to keep the the vector counts identical
					}
					else if ( filtertype == DBApi::eListFilterCriteria::SinceLastDateFilter )
					{
						entrystr = ">";		// not used, but auto-populate to keep vectors equally sized
					}
					else
					{
						std::cout << "Compare op: ";
						smValueLine_.clear();
						if ( modify_filters && listIdxNum >= 0 )
						{
							smValueLine_ = filtercompareops.at( listIdxNum );
							std::cout << smValueLine_;
						}

						process_line = false;
						do
						{
							process_line = readLine( input_quit );
						} while ( !input_quit && !process_line );

						if ( input_quit )
						{
							std::cout << std::endl;
							return input_quit;
						}

						if ( strnicmp( smValueLine_.c_str(), "contains", 8 ) == 0 )		// shourtcut for case-insensitive search compare opertor; NOT part of the DB interface!
						{
							entrystr = "~*";
						}
						else
						{
							entrystr = smValueLine_.c_str();
						}
					}
				}

				if ( entrystr.length() > 0 )
				{
					compareopstr = entrystr;

					entrystr.clear();

					if ( filtertype == DBApi::eListFilterCriteria::CreationDateFilter ||
						 filtertype == DBApi::eListFilterCriteria::CreationDateRangeFilter ||
						 filtertype == DBApi::eListFilterCriteria::ModifyDateFilter ||
						 filtertype == DBApi::eListFilterCriteria::ModifyDateRangeFilter ||
						 filtertype == DBApi::eListFilterCriteria::RunDateFilter ||
						 filtertype == DBApi::eListFilterCriteria::RunDateRangeFilter )
					{
						std::cout << "Enter Dates as 2020-08-31 with optional 12:00:00 time component." << std::endl;
						std::cout << "Start Date: ";
						smValueLine_.clear();
						if ( modify_filters && listIdxNum >= 0 )
						{
							smValueLine_ = filtercomparevals.at( listIdxNum );
							std::cout << smValueLine_;
						}

						process_line = false;
						do
						{
							process_line = readLine( input_quit );
						} while ( !input_quit && !process_line );

						if ( input_quit )
						{
							std::cout << std::endl;
							return input_quit;
						}

						entrystr = smValueLine_;

						if ( entrystr.length() > 0 )
						{
							if ( entrystr.length() > 10 )
							{
								date1str = boost::str( boost::format( "'%s'::timestamp" ) % entrystr );
							}
							else
							{
								date1str = entrystr;
							}

							entrystr.clear();

							if ( date1str.length() > 0 &&
								( filtertype == DBApi::eListFilterCriteria::CreationDateRangeFilter ||
								  filtertype == DBApi::eListFilterCriteria::ModifyDateRangeFilter ||
								  filtertype == DBApi::eListFilterCriteria::RunDateRangeFilter ) )
							{
								std::cout << "End Date: ";
								smValueLine_.clear();
								process_line = false;
								do
								{
									process_line = readLine( input_quit );
								} while ( !input_quit && !process_line );

								if ( input_quit )
								{
									std::cout << std::endl;
									return input_quit;
								}

								entrystr = smValueLine_.c_str();

								if ( entrystr.length() > 0 )
								{
									if ( entrystr.length() > 10 )
									{
										date2str = boost::str( boost::format( "'%s'::timestamp" ) % entrystr );
									}
									else
									{
										date2str = entrystr;
									}
									tgtvaluestr = boost::str( boost::format( "%s;%s" ) % date1str % date2str );
								}
							}
							else
							{
								tgtvaluestr = date1str;
								entrystr = tgtvaluestr;
							}
						}
					}
					else if ( filtertype == DBApi::eListFilterCriteria::SinceLastDateFilter )
					{
						entrystr = ">";		// not used, but auto-populate to keep vectors equally sized
					}
					else
					{
						std::cout << "Compare/match value: ";
						smValueLine_.clear();
						process_line = false;
						do
						{
							process_line = readLine( input_quit );
						} while ( !input_quit && !process_line );

						if ( input_quit )
						{
							std::cout << std::endl;
							return input_quit;
						}

						entrystr = smValueLine_;

						if ( entrystr.length() > 0 )
						{
							if ( IsDbFilterStringValue( filtertype ) )
							{
								tgtvaluestr = boost::str( boost::format( "'%s'" ) % entrystr );
							}
							else
							{
								tgtvaluestr = entrystr;
							}
						}
					}
				}

				if ( filtertypestr.length() > 0 &&
					 compareopstr.length() > 0 &&
					 tgtvaluestr.length() > 0 )
				{
					if ( modify_filters && listIdxNum >= 0 )
					{
						filtertypes.at( listIdxNum ) = filtertype;
						filtercompareops.at( listIdxNum ) = compareopstr;
						filtercomparevals.at( listIdxNum ) = tgtvaluestr;
					}
					else
					{
						// only add complete matching sets of filter criteria, comparison, and target values
						filtertypes.push_back( filtertype );
						filtercompareops.push_back( compareopstr );
						filtercomparevals.push_back( tgtvaluestr );
					}
					entrystr = tgtvaluestr;
				}
				else
				{
					entrystr.clear();
				}
			} while ( entrystr.length() > 0 );
		}
		else
		{
			filtertypes.at( listIdxNum ) = DBApi::eListFilterCriteria::NoFilter;
			filtercompareops.at( listIdxNum ) = "";
			filtercomparevals.at( listIdxNum ) = "";

			for ( auto typeIter = filtertypes.begin(); typeIter != filtertypes.end(); ++typeIter )
			{
				if ( *typeIter == DBApi::eListFilterCriteria::NoFilter )
				{
					filtertypes.erase( typeIter );
					break;
				}
			}

			for ( auto opIter = filtercompareops.begin(); opIter != filtercompareops.end(); ++opIter )
			{
				if ( opIter->empty() )
				{
					filtercompareops.erase( opIter );
					break;
				}
			}

			for ( auto valIter = filtercomparevals.begin(); valIter != filtercomparevals.end(); ++valIter )
			{
				if ( valIter->empty() )
				{
					filtercomparevals.erase( valIter );
					break;
				}
			}
		}
		std::cout << std::endl;
		filterListSize = filtertypes.size();

		if ( modify_filters )
		{
			promptStr = "Modify more filters? (y/N):  ";

			entrystr.clear();
			input_quit = GetYesOrNoResponse( entrystr, promptStr );
			if ( input_quit )
			{
				return input_quit;
			}

			if ( entrystr.length() == 0 )
			{
				entrystr = "N";
			}

			if ( strnicmp( entrystr.c_str(), "N", 2 ) == 0 )
			{
				done = true;
				std::cout << std::endl;
			}
		}
		else
		{
			done = true;
			std::cout << std::endl;
		}
	} while ( !done );

	return input_quit;
}

bool ScoutX::GetDbListSort( DBApi::eListSortCriteria& primarysort,
							DBApi::eListSortCriteria& secondarysort,
							int32_t& sortdir )
{
	bool input_quit = false;
	bool process_line = false;
	std::string entryStr = "";
	std::string promptStr = "Set Sort? (y/N):  ";

	entryStr.clear();
	input_quit = GetYesOrNoResponse( entryStr, promptStr );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
	{
		ShowDbListSortEnum();
		std::cout << std::endl;

		DBApi::eListSortCriteria sorttype = DBApi::eListSortCriteria::SortNotDefined;
		bool sortTypeOk = true;
		std::string sortTypeStr = "";

		do
		{
			sorttype = DBApi::eListSortCriteria::SortNotDefined;
			sortTypeOk = true;
			sortTypeStr.clear();
			entryStr.clear();
			promptStr = "Primary Sort (or RETURN for done): ";

			GetStringInput( entryStr, promptStr );
			if ( input_quit )
			{
				std::cout << std::endl;
				return input_quit;
			}

			if ( entryStr.length() > 0 )
			{
				int type = stol( entryStr );
				if ( GetDbSortEnumString( type, sortTypeStr ) )
				{
					sorttype = static_cast<DBApi::eListSortCriteria>( type );
					std::cout << " ( " << sortTypeStr << " )" << std::endl;
				}
				else
				{
					std::cout << "Invalid Entry!" << std::endl;
					sortTypeOk = false;
				}
			}
		} while ( !sortTypeOk );

		if ( entryStr.length() > 0 )
		{
			primarysort = static_cast<DBApi::eListSortCriteria>( std::stol( entryStr ) );

			do
			{
				sorttype = DBApi::eListSortCriteria::SortNotDefined;
				sortTypeOk = true;
				sortTypeStr.clear();
				entryStr.clear();
				promptStr = "Secondary Sort (or RETURN for done): ";

				GetStringInput( entryStr, promptStr );
				if ( input_quit )
				{
					std::cout << std::endl;
					return input_quit;
				}

				if ( entryStr.length() > 0 )
				{
					int type = stol( entryStr );
					if ( GetDbSortEnumString( type, sortTypeStr ) )
					{
						sorttype = static_cast<DBApi::eListSortCriteria>( type );
						std::cout << " ( " << sortTypeStr << " )" << std::endl;
					}
					else
					{
						std::cout << "Invalid Entry!" << std::endl;
						sortTypeOk = false;
					}
				}
			} while ( !sortTypeOk );

			if ( entryStr.length() > 0 )
			{
				secondarysort = static_cast<DBApi::eListSortCriteria>( std::stol( entryStr ) );
			}
		}
		std::cout << std::endl;
	}

	promptStr = "Select Sort Order? (y/N):  ";

	entryStr.clear();
	input_quit = GetYesOrNoResponse( entryStr, promptStr );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
	{
		entryStr.clear();
		promptStr = "Sort Order (ASC/DESC/NONE): ";

		GetStringInput( entryStr, promptStr );
		if ( input_quit )
		{
			std::cout << std::endl;
			return input_quit;
		}

		sortdir = 0;		// set default to the 'don't care' condition
		if ( entryStr.length() > 0 )
		{
			if ( entryStr.length() == 1 )
			{
				if ( strnicmp( entryStr.c_str(), "A", 1 ) == 0 )
				{
					sortdir = 1;
				}
				else if ( strnicmp( entryStr.c_str(), "D", 1 ) == 0 )
				{
					sortdir = -1;
				}
				// to explicitly allow entry of a don't care option
				else if ( strnicmp( entryStr.c_str(), "N", 1 ) == 0 )
				{
					sortdir = 0;
				}
				else
				{
					std::cout << "Unrecognized entry! using default value 'None'";
				}
			}
			else
			{
				if ( strnicmp( entryStr.c_str(), "ASC", 3 ) == 0 )
				{
					sortdir = 1;
				}
				else if ( strnicmp( entryStr.c_str(), "DESC", 4 ) == 0 )
				{
					sortdir = -1;
				}
				// to explicitly allow entry of a don't care option
				else if ( strnicmp( entryStr.c_str(), "NONE", 4 ) == 0 )
				{
					sortdir = 0;
				}
				else
				{
					std::cout << "Unrecognized entry! using default value 'None'";
				}
			}
		}
		std::cout << std::endl;
	}

	return input_quit;
}

bool ScoutX::GetDbListSortAndFilter( std::vector<DBApi::eListFilterCriteria>& filtertypes,
									 std::vector<std::string>& filtercompareops,
									 std::vector<std::string>& filtercomparevals,
									 DBApi::eListSortCriteria& primarysort,
									 DBApi::eListSortCriteria& secondarysort,
									 int32_t& sortdir, int32_t& limitcnt )
{
	bool input_quit = false;
	bool process_line = false;
	std::string entryStr = "";

	std::cout << std::endl;
	input_quit = GetDbListFilter( filtertypes, filtercompareops, filtercomparevals );

	if ( input_quit )
	{
		return input_quit;
	}

	input_quit = GetDbListSort( primarysort, secondarysort, sortdir );

	if ( input_quit )
	{
		return input_quit;
	}

	std::string promptStr = "Select record limit? (y/N):  ";

	entryStr.clear();
	input_quit = GetYesOrNoResponse( entryStr, promptStr );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
	{
		promptStr = "Record Limit(0-n): ";
		entryStr.clear();
		input_quit = GetStringInput( entryStr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 )
		{
			limitcnt = std::stol( entryStr );
		}
	}

	return input_quit;
}

bool ScoutX::DoDbConnect( bool do_connect )
{

	if ( !do_connect )
	{
		bool input_quit = false;
		bool process_line = false;

		std::string promptStr = "Connect using current configuration? (y/n):  ";
		std::string entryStr = "";
		input_quit = GetStringInput( entryStr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
		{
			do_connect = true;
		}
	}

	if (do_connect)
	{
		if ( DBApi::IsDBPresent() )
		{
			std::cout << "Successfully connected to the DB" << std::endl;
		}
		else
		{
			std::cout << "Failed to connect to the DB..." << std::endl;
		}
	}
	return false;
}

bool ScoutX::SelectDbConfigRec( std::vector<DBApi::DB_InstrumentConfigRecord>& cfg_list,
								DBApi::DB_InstrumentConfigRecord& cfgrec,
								std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult )
{
	bool input_quit = false;
	bool process_line = false;
	std::string cfgSN = "";
	int64_t cfgIdNum = -1;
	bool cfgOk = false;
	std::string entryStr = "";
	std::string promptStr = "";
	std::string lengthWarning = "";

	if ( cfgrec.InstrumentIdNum > 0 || cfgrec.InstrumentSNStr.length() > 0 || currentInstCfgIdNum >= 0)
	{
		entryStr.clear();
		promptStr = "Use current Instrument configuration record? (y/n):  ";

		input_quit = GetYesOrNoResponse( entryStr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
		{
			if ( cfgrec.InstrumentIdNum > 0 || cfgrec.InstrumentSNStr.length() > 0 )
			{
				cfgSN = cfgrec.InstrumentSNStr;
				cfgIdNum = cfgrec.InstrumentIdNum;
			}
			else
			{
				cfgSN = currentCfgRec.InstrumentSNStr;
				cfgIdNum = currentCfgRec.InstrumentIdNum;
				if ( cfgrec.InstrumentSNStr != currentCfgRec.InstrumentSNStr &&
					 cfgrec.InstrumentIdNum != currentCfgRec.InstrumentIdNum )
				{
					cfgrec = currentCfgRec;
				}
			}
			qresult = DBApi::eQueryResult::QueryOk;
			cfgOk = true;
		}
		else
		{
			cfgrec = {};
			cfgsn.clear();
			cfgidnum = 0;
		}
	}

	if ( !cfgOk )
	{
		entryStr.clear();
		promptStr = "Use Default Instrument configuration? (y/n):  ";

		input_quit = GetYesOrNoResponse( entryStr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
		{
			cfg_list.clear();

			if ( dfltInstCfgIdNum < 0 )
			{
				std::string instSN = "";
				int64_t instCfgIdNum = 0;

				GetDbDefaultConfigRec( cfgrec, instSN, instCfgIdNum, qresult );
				cfgsn = instSN;
				cfgidnum = instCfgIdNum;
			}
			else
			{
				cfgrec = defaultCfgRec;
				cfgsn = dfltInstSN;
				cfgidnum = dfltInstCfgIdNum;
			}
		}
		else
		{
			cfg_list.clear();

			qresult = DBApi::DbGetInstrumentConfigList( cfg_list );
			if ( qresult == DBApi::eQueryResult::QueryOk || qresult == DBApi::eQueryResult::NoResults )
			{
				std::cout << "Successfully retrieved instrument configuration List..." << std::endl;

				print_db_instrument_config_list( cfg_list );
			}
			else
			{
				std::cout << "Failed to retrieve instrument config list: " << (int32_t) qresult << std::endl;
			}

			if ( cfg_list.size() > 0 )
			{
				size_t listSize = cfg_list.size();
				int selectMode = -1;
				bool selectValid = false;
				bool recFound = false;

				promptStr = "Select identifier: 0 = ID number; 1 = Serial Number/name:  ";
				do
				{
					do
					{
						entryStr.clear();

						GetStringInput( entryStr, promptStr );
						if ( input_quit )
						{
							return input_quit;
						}

						if ( entryStr.length() == 0 )
						{
							std::cout << "WARNING: identifier select type is blank!  Using the first config in the list!" << std::endl;
							entryStr = "-1";
						}
						std::cout << std::endl;
					} while ( entryStr.length() == 0 );

					selectMode = std::stoi( entryStr );
					if ( selectMode >= -1 && selectMode <= 1 )
					{
						selectValid = true;
					}
					else
					{
						std::cout << "WARNING: Invalid identifier type entered!" << std::endl;
					}
				} while ( !selectValid );

				switch ( selectMode )
				{
					case -1:		// select first on list; different from delect default record...
					{
						cfgrec = cfg_list.at( 0 );
						recFound = true;
						break;
					}

					case 0:		// select by ID number
					{
						promptStr = "Enter record ID number:  ";
						lengthWarning = "WARNING: ID number cannot be blank!";

						do
						{
							entryStr.clear();

							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return input_quit;
							}

							cfgIdNum = std::stoll( entryStr );

							if ( cfgIdNum > 0 )
							{
								for ( auto cIter = cfg_list.begin(); cIter != cfg_list.end(); ++cIter )
								{
									DBApi::DB_InstrumentConfigRecord cfg = *cIter;

									if ( cfg.InstrumentIdNum == cfgIdNum )
									{
										cfgrec = cfg;
										recFound = true;
										break;
									}
								}

								if ( !recFound )
								{
									std::cout << "No matching ID number found in the list! Ensure entry is correct.";
								}
							}
							else
							{
								std::cout << "WARNING: Illegal ID number! Must be greater than '0'" << std::endl;
							}
						} while ( !recFound );
						break;
					}

					case 1:		// select by serial number / name
					{
						promptStr = "Enter record serial number / name:  ";
						lengthWarning = "WARNING: Serial number / name cannot be blank!";

						do
						{
							entryStr.clear();

							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return input_quit;
							}

							cfgSN = entryStr;

							for ( auto cIter = cfg_list.begin(); cIter != cfg_list.end(); ++cIter )
							{
								DBApi::DB_InstrumentConfigRecord cfg = *cIter;

								// this will be a case-sensitive comparison...
								if ( cfg.InstrumentSNStr == cfgSN )
								{
									cfgrec = cfg;
									recFound = true;
									break;
								}
							}

							if ( !recFound )
							{
								std::cout << "No matching serial number / name  found in the list! Ensure entry is correct and case-sensitve.";
							}

							recFound = false;		// reset for the retrieve operation

							qresult = DBApi::DbFindInstrumentConfigBySN( cfgrec, cfgSN );
							if ( qresult == DBApi::eQueryResult::QueryOk )
							{
								recFound = true;
							}

						} while ( !recFound );
						break;
					}
				}

				if ( recFound )
				{
					cfgSN = cfgrec.InstrumentSNStr;
					cfgIdNum = cfgrec.InstrumentIdNum;
				}
			}
			else
			{
				cfgSN.clear();
				cfgIdNum = -1;
				cfgrec = {};
			}
		}
	}

	cfgsn = cfgSN;
	cfgidnum = cfgIdNum;

	return false;
}

bool ScoutX::GetDbDefaultConfigRec( DBApi::DB_InstrumentConfigRecord& cfgrec,
									std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult )
{
	std::string instSN = "";
	int64_t instCfgIdNum = 0;

	bool status = GetDbConfigRec( cfgrec, instSN, instCfgIdNum, qresult );

	cfgsn = instSN;
	cfgidnum = instCfgIdNum;

	return status;
}

bool ScoutX::GetDbConfigRec( DBApi::DB_InstrumentConfigRecord& cfgrec,
							 std::string& cfgsn, int64_t& cfgidnum, DBApi::eQueryResult& qresult )
{
	DBApi::DB_InstrumentConfigRecord newCfgRec = {};
	bool status = false;

	qresult = DBApi::DbFindInstrumentConfig( newCfgRec, cfgidnum, cfgsn );
	if ( qresult == DBApi::eQueryResult::QueryOk )
	{
		if ( cfgidnum == 0 )
		{
			std::cout << "Successfully retrieved default instrument configuration..." << std::endl;

			defaultCfgRec = newCfgRec;
			dfltInstSN = newCfgRec.InstrumentSNStr;
			dfltInstCfgIdNum = newCfgRec.InstrumentIdNum;
		}
		else
		{
			std::cout << "Successfully retrieved instrument configuration..." << std::endl;

			currentCfgRec = newCfgRec;
			currentInstSN = newCfgRec.InstrumentSNStr;
			currentInstCfgIdNum = newCfgRec.InstrumentIdNum;
		}

		cfgsn = newCfgRec.InstrumentSNStr;
		cfgidnum = newCfgRec.InstrumentIdNum;
		status = true;

		print_db_instrument_config( newCfgRec );
	}
	else
	{
		std::string queryResultStr = "";

		DBApi::GetQueryResultString( qresult, queryResultStr );
		if ( cfgidnum == 0 )
		{
			std::cout << "Failed to retrieve default instrument configuration: " << queryResultStr << " (" << (int32_t) qresult << ") " << std::endl;

			defaultCfgRec = {};
			dfltInstSN.clear();
			dfltInstCfgIdNum = -1;
		}
		else
		{
			std::cout << "Failed to retrieve instrument configuration: " << queryResultStr << " (" << (int32_t) qresult << ") " << std::endl;

			currentCfgRec = {};
			currentInstSN.clear();
			currentInstCfgIdNum = -1;
		}

		cfgsn.clear();
		cfgidnum = -1;
	}
	return status;
}

bool ScoutX::SelectDbListIdNum( size_t listsize, int64_t& cfgidnum )
{
	bool input_quit = false;
	bool process_line = false;
	int64_t cfgIdNum = 0;
	bool cfgOk = false;
	std::string entryStr = "";

	if ( listsize > 0 )
	{
		int selectMode = -1;
		bool selectValid = false;
		bool recFound = false;

		do
		{
			entryStr.clear();
			std::string promptStr = "Enter list ID number:  ";
			std::string lengthWarning = "WARNING: ID number cannot be blank!";

			input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
			if ( input_quit )
			{
				return input_quit;
			}

			cfgIdNum = std::stoll( entryStr );
		} while ( cfgIdNum <= 0 );
	}
	else
	{
		cfgIdNum = 0;
	}

	cfgidnum = cfgIdNum;

	return false;
}

void ScoutX::GetWeeklyIndicatorString( DBApi::DB_SchedulerConfigRecord& scfgrec, std::string& indicatorStr, uint16_t subtype )
{
	std::string workStr = "";

	if ( subtype == 0 || ( subtype & DBApi::eDayWeekIndicatorBits::DayIndicatorMask ) )
	{
		std::string currentDays = "";
		int16_t daysIndicator = ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::DayIndicatorMask );

		daysIndicator &= DBApi::eDayWeekIndicatorBits::IndicatorErrorClearMask;		// clear error bit
		for ( int i = 0; i < 7; i++ )
		{
			if ( daysIndicator & ( DBApi::eDayWeekIndicatorBits::DayBit << i ) )
			{
				if ( currentDays.length() > 0 )
				{
					currentDays.append( "," );
				}

				switch ( i )
				{
					case 0:
						currentDays.append( "Su" );
						break;

					case 1:
						currentDays.append( "Mo" );
						break;

					case 2:
						currentDays.append( "Tu" );
						break;

					case 3:
						currentDays.append( "We" );
						break;

					case 4:
						currentDays.append( "Th" );
						break;

					case 5:
						currentDays.append( "Fr" );
						break;

					case 6:
						currentDays.append( "Sa" );
						break;
				}
			}
		}
		workStr = currentDays;
	}

	if ( subtype == 0 || (subtype & DBApi::eDayWeekIndicatorBits::WeekIndicatorMask ))
	{
		std::string currentWeeks = "";
		std::string weeksLabel = "";
		int16_t weekIndicator = ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::WeekIndicatorMask );

		if ( ( weekIndicator & DBApi::eDayWeekIndicatorBits::WeekIndicatorMask ) != 0 )
		{
			bool weekSelected = false;
			bool multiWeekSelected = false;
			int weekIdx = -1;

			for ( int i = 0; i < 9; i++ )	// 5 selector choices + 3 leftover bits + error bit
			{
				if ( weekIndicator & ( DBApi::eDayWeekIndicatorBits::WeekBit << i ) )
				{
					switch ( i )
					{
						case 0:
							if ( !weekSelected )
							{
								weekSelected = true;
								weeksLabel = "Weekly";
								weekIdx = i;
							}
							else
							{
								multiWeekSelected = true;
							}
							break;

						case 1:
							if ( !weekSelected )
							{
								weekSelected = true;
								weeksLabel = "Every two weeks";
								weekIdx = i;
							}
							else
							{
								multiWeekSelected = true;
							}
							break;

						case 2:
							if ( !weekSelected )
							{
								weekSelected = true;
								weeksLabel = "Every three weeks";
								weekIdx = i;
							}
							else
							{
								multiWeekSelected = true;
							}
							break;

						case 3:
							if ( !weekSelected )
							{
								weekSelected = true;
								weeksLabel = "Every four weeks";
								weekIdx = i;
							}
							else
							{
								multiWeekSelected = true;
							}
							break;

						case 4:
							if ( !weekSelected )
							{
								weekSelected = true;
								weeksLabel = "Monthly";
								workStr.clear();
								scfgrec.DayWeekIndicator &= ~( DBApi::eDayWeekIndicatorBits::DayIndicatorMask);
								weekIdx = i;
							}
							else
							{
								multiWeekSelected = true;
							}
							break;

						case 8:		// indicate error bit was found set...
							currentWeeks.append( " - Error bit set" );
							break;

						default:
							currentWeeks = " - Unrecognized weekly interval";
							break;
					}
				}
			}

			if ( multiWeekSelected )
			{
				currentWeeks = " - Multiple weekly intervals selected";
				multiWeekSelected = false;
			}
			else if ( weekSelected && weekIdx >= 0)
			{
				weekIdx++;
				if ( subtype == 0 || subtype == DBApi::eDayWeekIndicatorBits::FullIndicatorMask )
				{
					currentWeeks = weeksLabel;
				}
				else
				{
					currentWeeks = boost::str( boost::format( "%ld" ) % weekIdx );
				}
			}
		}
		else
		{
			if ( subtype == 0 || subtype == DBApi::eDayWeekIndicatorBits::FullIndicatorMask )
			{
				currentWeeks = "Once";
			}
			else
			{
				currentWeeks = "0";
			}
		}

		if ( workStr.length() > 0  && currentWeeks.length() > 0)
		{
			workStr.append( ", " );
		}
		workStr.append( currentWeeks );
	}

	indicatorStr = workStr;
}

bool ScoutX::EnterSchedulerConfig( DBApi::DB_SchedulerConfigRecord& scfgrec, bool make_new_rec )
{
	bool input_quit = false;
	bool process_line = false;
	int64_t cfgIdNum = 0;
	uuid__t blankId = {};
	system_TP zeroTP = {};

//	const char		DbTimeFmtStr[] = "'%Y-%m-%d %H:%M:%S'";		// formatting string used for generation of time strings used in database insertion statements

	if ( make_new_rec )
	{
		scfgrec.ConfigIdNum = 0;
		scfgrec.ConfigId = blankId;
		scfgrec.Name.clear();
		scfgrec.Comments.clear();
		scfgrec.FilenameTemplate.clear();
		scfgrec.OwnerUserId = blankId;
		scfgrec.CreationDate = zeroTP;
		scfgrec.OutputType = (int16_t) DBApi::eExportTypes::NoExportType;
		scfgrec.StartDate = zeroTP;
		scfgrec.StartOffset = 0;
		scfgrec.MultiRepeatInterval = 0;
		scfgrec.DayWeekIndicator = 0;
		scfgrec.MonthlyRunDay = 0;
		scfgrec.DestinationFolder.clear();
		scfgrec.DataType = static_cast<int32_t>(DBApi::eListType::SummaryResultList);		// this appears to be the starting point for the data retrieval sequence
//		scfgrec.DataType = static_cast<int32_t>(DBApi::eListType::AnalysisList);
//		scfgrec.DataType = static_cast<int32_t>(DBApi::eListType::SampleList);
		scfgrec.FilterTypesList.clear();
		scfgrec.CompareOpsList.clear();
		scfgrec.CompareValsList.clear();
		scfgrec.Enabled = false;
		scfgrec.LastRunTime = zeroTP;
		scfgrec.LastSuccessRunTime = zeroTP;
		scfgrec.LastRunStatus = 0;
		scfgrec.NotificationEmail.clear();
		scfgrec.EmailServerAddr.clear();
		scfgrec.EmailServerPort = 0;
		scfgrec.AuthenticateEmail = false;
		scfgrec.AccountUsername.clear();
		scfgrec.AccountPwdHash.clear();
	}

	std::string entryStr = "";
	std::string promptStr = "";
	std::string lengthWarning = "";
	std::string currentValStr = "";

	if ( scfgrec.Name.length() > 0 )
	{
		entryStr = scfgrec.Name;
	}
	promptStr = "Enter Scheduler task name:  ";
	lengthWarning = "WARNING: Task name should not be blank!";

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.Name = entryStr;

	entryStr.clear();
	if ( scfgrec.Comments.length() > 0 )
	{
		entryStr = scfgrec.Comments;
	}
	promptStr = "Enter Scheduler task comments:  ";
	lengthWarning.clear();

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.Comments = entryStr;

	entryStr.clear();
	if ( scfgrec.FilenameTemplate.length() > 0 )
	{
		entryStr = scfgrec.FilenameTemplate;
	}
	promptStr = "Enter Output filename template:  ";
	lengthWarning = "WARNING: Filename template should not be blank!";

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.FilenameTemplate = entryStr;

	bool userFound = false;
	bool typeOk = true;
	std::string userNameStr = "";
	std::string userTypeStr = "all";
	DBApi::eUserType userType = DBApi::eUserType::AllUsers;
	DBApi::DB_UserRecord userRec = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NoResults;

	queryResult = DbFindUserByUuid( userRec, scfgrec.OwnerUserId );
	if ( DBApi::eQueryResult::QueryOk == queryResult )
	{
		userNameStr = userRec.UserNameStr;
		if ( userRec.ADUser )
		{
			userType = DBApi::eUserType::AdUsers;
			userTypeStr = "AD";
		}
		else
		{
			userType = DBApi::eUserType::LocalUsers;
			userTypeStr = "local";
		}
	}

	do
	{
		promptStr = "Enter Owner-user Name:  ";
		lengthWarning = "WARNING: Username must not be blank!";

		input_quit = GetStringInput( userNameStr, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		promptStr = "Enter User type (local, all, AD):  ";
		lengthWarning.clear();
		do
		{
			typeOk = true;

			input_quit = GetStringInput( userTypeStr, promptStr, lengthWarning );
			if ( input_quit )
			{
				return input_quit;
			}

			if ( userTypeStr.length() > 0 )
			{
				if ( 0 == stricmp( userTypeStr.c_str(), "local" ) )
				{
					userType = DBApi::eUserType::LocalUsers;
				}
				else if ( 0 == stricmp( userTypeStr.c_str(), "all" ) )
				{
					userType = DBApi::eUserType::AllUsers;
				}
				else if ( 0 == stricmp( userTypeStr.c_str(), "ad" ) )
				{
					userType = DBApi::eUserType::AdUsers;
				}
				else
				{
					std::cout << "Ilegal/unrecognized type entered!" << std::endl;
					typeOk = false;
				}
			}
		} while ( !typeOk );

		DBApi::DB_UserRecord userRec = {};
		DBApi::eQueryResult queryResult = DbFindUserByName( userRec, userNameStr, userType );
		if ( DBApi::eQueryResult::QueryOk == queryResult )
		{
			scfgrec.OwnerUserId = userRec.UserId;
			userFound = true;
		}
	} while ( !userFound );

	// DO NOT enter a creation date! it is filled-in automatically!

	entryStr.clear();
	promptStr = "Enter Export output type (1=encrypted, 2=non-encrypted):  ";
	int outputType = 0;
	if ( scfgrec.OutputType > 0 )
	{
		entryStr = boost::str(boost::format("%d") % scfgrec.OutputType );
	}
	lengthWarning = "WARNING: Output type must not be blank!";

	do
	{
		input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		outputType = std::stoi( entryStr );

		switch ( outputType )
		{
			case 1:
			case 2:
				scfgrec.OutputType = (int16_t) outputType;
				break;

			default:
				std::cout << "Ilegal output type entered!" << std::endl;
				if ( scfgrec.OutputType > 0 )
				{
					entryStr = boost::str( boost::format( "%d" ) % scfgrec.OutputType );
				}
				outputType = 0;
				break;
		}
	} while ( outputType == 0 );

	// Get the desired start date using only the year-month-day combination; automatically add the time of 00:00:00
	entryStr.clear();
	bool boolVal = false;
	system_TP startDate = {};

	if ( scfgrec.StartDate != startDate )	// not an empty time point
	{
		DBApi::GetDbTimeString( scfgrec.StartDate, entryStr );

		// DB time strings are surrounded by single quotes
		if ( entryStr.at( 0 ) == '\'' )
		{
			entryStr.erase( 0, 1 );
		}

		if ( entryStr.at( entryStr.size() - 1 ) == '\'' )
		{
			entryStr.erase( entryStr.size() - 1, 1 );
		}

		if ( entryStr.length() > 11 )
		{
			entryStr.erase( 10, entryStr.size() - 10 );
		}
	}

	std::string fmtStr = "";

	do
	{
		promptStr = "Enter desired start date as YYYY-MM-DD, or blank for today:  ";
		lengthWarning.clear();

		input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() == 0 )
		{
			startDate = ChronoUtilities::CurrentTime();
			boolVal = true;
		}
		else if ( entryStr.length() >= 10 )
		{
			fmtStr.clear();
			if ( entryStr.length() >= 10 )
			{
				entryStr.erase( 10, entryStr.size() - 10 );
			}
			entryStr.append( " 00:00:00" );		// start date starts at at midnight

			fmtStr = boost::str( boost::format( "'%s'" ) % entryStr );
			startDate = {};
			boolVal = DBApi::GetTimePtFromDbTimeString( startDate, fmtStr );
		}
		else			// doesn't appear to be properly entered
		{
			std::cout << "Ilegal date entry! Must be blank or properly formatted!" << std::endl;
			entryStr.clear();
			boolVal = false;
		}
	} while ( !boolVal );

	scfgrec.StartDate = startDate;

	int minutes = -1;
	entryStr.clear();

	do
	{
		entryStr = boost::str( boost::format( "%d" ) % scfgrec.StartOffset );
		promptStr = "Enter task start offset from midnight in minutes:  ";
		lengthWarning.clear();

		input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 )
		{
			minutes = std::stoi( entryStr );
			if ( minutes < 0 || minutes > 1439 )
			{
				std::cout << "Ilegal offset entered!" << std::endl;
				minutes = -1;
			}
		}
		else
		{
			minutes = 0;
		}
	} while ( minutes < 0 );

	scfgrec.StartOffset = (int16_t) minutes;

	minutes = -1;
	entryStr.clear();

	do
	{
		entryStr = boost::str( boost::format( "%d" ) % scfgrec.MultiRepeatInterval );
		promptStr = "Enter same-day repetition inteval in minutes (0=no repetitions, min: 60, max: 360):  ";
		lengthWarning.clear();

		input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() > 0 )
		{
			minutes = std::stoi( entryStr );
			if ( minutes < 0 || minutes > 360 || (minutes > 0 && minutes < 60) )
			{
				std::cout << "Ilegal repetition interval entered!" << std::endl;
				minutes = -1;
			}
		}
		else
		{
			minutes = 0;
		}
	} while ( minutes < 0 );

	scfgrec.MultiRepeatInterval = (int16_t) minutes;

	int16_t daysIndicator = (scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::DayIndicatorMask);
	std::string currentDays = "";
	std::vector<std::string> tokenList = {};
	std::string tokenStr = "";
	do
	{
		currentDays.clear();
		GetWeeklyIndicatorString( scfgrec, currentDays, DBApi::eDayWeekIndicatorBits::DayIndicatorMask );

		promptStr = "Enter days of week to run, as shown in help (none for one-time or monthly; Su,Mo,Tu,We,Th,Fr,Sa):  ";
		lengthWarning.clear();

		input_quit = GetStringInput( currentDays, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		daysIndicator = 0;

		if ( currentDays.length() > 0 )
		{
			TokenizeStr( tokenList, currentDays, "", ",+" );

			for ( int tokenIdx = 0; tokenIdx < tokenList.size(); tokenIdx++ )
			{
				tokenStr = tokenList.at( tokenIdx );

				if ( strnicmp( tokenStr.c_str(), "Su", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::SundayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "Mo", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::MondayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "Tu", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::TuesdayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "We", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::WednesdayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "Th", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::ThursdayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "Fr", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::FridayShift );
				}
				else if ( strnicmp( tokenStr.c_str(), "Sa", 2 ) == 0 )
				{
					daysIndicator |= ( DBApi::eDayWeekIndicatorBits::DayBit << DBApi::eDayBitShift::SaturdayShift );
				}
				else
				{
					daysIndicator |= DBApi::eDayWeekIndicatorBits::IndicatorErrorBit;		// set the high bit to make it a negative flag value...
					std::cout << "Ilegal day indicator entered!" << std::endl;
				}
			}
		}
	} while ( daysIndicator & DBApi::eDayWeekIndicatorBits::IndicatorErrorBit );

	if ( ( daysIndicator & DBApi::eDayWeekIndicatorBits::DayIndicatorMask )  &&
		 ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::MonthBit ) )
	{
		std::cout << std::endl << "Setting days of the week will reset monthly repetitions!" << std::endl << std::endl;
		daysIndicator &= ~( DBApi::eDayWeekIndicatorBits::MonthBit );
		scfgrec.MonthlyRunDay = 0;
	}

	std::string currentWeeks = "";
	int weekCnt = 0;
	int16_t weekIndicator = ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::WeekMonthIndicatorMask );

	tokenList.clear();
	tokenStr.clear();
	do
	{
		currentWeeks.clear();
		GetWeeklyIndicatorString( scfgrec, currentWeeks, DBApi::eDayWeekIndicatorBits::WeekMonthIndicatorMask );

		promptStr = "Enter the weekly repetitions (0=once, 1=weekly, 2=every 2 week, 3=every 3 weeks, 4=every 4 weeks, 5=monthly) ";
		if ( currentWeeks.length() > 1 )
		{
			promptStr.append( currentWeeks );
			currentWeeks.clear();
		}
		promptStr.append(":  ");
		lengthWarning.clear();

		input_quit = GetStringInput( currentWeeks, promptStr, lengthWarning );
		if ( input_quit )
		{
			return input_quit;
		}

		weekIndicator = 0;

		if ( currentWeeks.length() > 0 )
		{
			weekCnt = std::stoi( currentWeeks );

			switch ( weekCnt )
			{
				case 0:
					currentWeeks = "Once";
					weekIndicator = 0;			// clear so that there are no week or month bits selected
					break;

				case 1:
				case 2:
				case 3:
				case 4:
					weekIndicator = ( DBApi::eDayWeekIndicatorBits::WeekBit << ( weekCnt - 1 ) );
					break;

				case 5:
					weekIndicator = DBApi::eDayWeekIndicatorBits::MonthBit;
					if ( ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::DayIndicatorMask ) ||
						 ( daysIndicator & DBApi::eDayWeekIndicatorBits::DayIndicatorMask ) )
					{
						std::cout << std::endl << "This will clear any entered days of the week!" << std::endl << std::endl;
					}
					daysIndicator = 0;
					break;

				default:
					weekIndicator |= DBApi::eDayWeekIndicatorBits::IndicatorErrorBit;		// set the high bit to make it a negative flag value...
					std::cout << "Ilegal weekly repetition entered!" << std::endl;
					break;
			}
		}
		else
		{
			weekIndicator &= ~( DBApi::eDayWeekIndicatorBits::WeekMonthIndicatorMask);
		}
	} while ( weekIndicator & DBApi::eDayWeekIndicatorBits::IndicatorErrorBit );
	scfgrec.DayWeekIndicator = ( weekIndicator | daysIndicator );

	if ( scfgrec.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::MonthBit )
	{
		int runDay = scfgrec.MonthlyRunDay;

		do
		{
			entryStr.clear();
			if ( scfgrec.MonthlyRunDay > 0 )
			{
				entryStr = boost::str( boost::format( "%d" ) % scfgrec.MonthlyRunDay );
			}
			promptStr = "Enter day of month to run:  ";
			lengthWarning = "Day cannot be blank or '0' when monthly repetition is selected!";

			input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
			if ( input_quit )
			{
				return input_quit;
			}

			if ( entryStr.length() == 0 )
			{
				std::cout << "Cannot be blank or '0'!" << std::endl;
				runDay = 0;
			}
			else
			{
				runDay = std::stoi( entryStr );
				if ( runDay <= 0 || runDay > 31 )
				{
					std::cout << "Illegal value entered!" << std::endl;
					runDay = 0;
				}
			}
		} while ( runDay == 0 );

		scfgrec.MonthlyRunDay = (int16_t)runDay;
	}
	else
	{
		scfgrec.MonthlyRunDay = 0;
	}

	entryStr.clear();
	if ( scfgrec.DestinationFolder.length() > 0 )
	{
		entryStr = scfgrec.DestinationFolder;
	}

	promptStr = "Enter output destination folder:  ";
	lengthWarning = "WARNING: Output destination folder should not be blank!";

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.DestinationFolder = entryStr;

//	ShowDbListDataTypeEnum();
//	GetDbListDataType( scfgrec.DataType );

	input_quit = GetDbListFilter( scfgrec.FilterTypesList, scfgrec.CompareOpsList, scfgrec.CompareValsList );

	if ( input_quit )
	{
		return input_quit;
	}

	boolVal = scfgrec.Enabled;
	if ( scfgrec.Enabled == true )
	{
		entryStr = "Y";
	}
	else
	{
		entryStr = "N";
	}

	promptStr = "Scheduler task enabled (y/N): ";

	input_quit = GetYesOrNoResponse( entryStr, promptStr );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entryStr.length() == 0 )
	{
		entryStr = "N";
	}

	if ( strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
	{
		boolVal = true;
	}
	else if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
	{
		boolVal = false;
	}

	scfgrec.Enabled = boolVal;

	entryStr.clear();
	entryStr = scfgrec.NotificationEmail;
	promptStr = "Enter notification email address: ";
	lengthWarning.clear();

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.NotificationEmail = entryStr;

	entryStr.clear();
	entryStr = scfgrec.EmailServerAddr;
	promptStr = "Enter email server name: ";
	lengthWarning.clear();

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.EmailServerAddr = entryStr;

	entryStr.clear();
	if ( scfgrec.EmailServerPort > 0 )
	{
		entryStr = boost::str( boost::format( "%ld" ) % scfgrec.EmailServerPort );
	}
	promptStr = "Enter Email server port number:  ";
	lengthWarning.clear();

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	if ( entryStr.length() == 0 )
	{
		entryStr = "0";
	}
	scfgrec.EmailServerPort = std::stol( entryStr );

	boolVal = scfgrec.AuthenticateEmail;
	if ( scfgrec.AuthenticateEmail == true )
	{
		entryStr = "Y";
	}
	else
	{
		entryStr = "N";
	}

	promptStr = "Do email server authentication? (y/N): ";
	lengthWarning.clear();
	do
	{
		input_quit = GetYesOrNoResponse( entryStr, promptStr );
		if ( input_quit )
		{
			return input_quit;
		}

		if ( entryStr.length() == 0 )
		{
			entryStr = "N";
		}

		if ( strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
		{
			boolVal = true;
		}
		else if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
		{
			boolVal = false;
		}
	} while ( entryStr.length() == 0 );
	scfgrec.AuthenticateEmail = boolVal;

	entryStr.clear();
	entryStr = scfgrec.AccountUsername;
	promptStr = "Enter email account user name: ";
	lengthWarning.clear();

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.AccountUsername = entryStr;

	entryStr.clear();
	entryStr = scfgrec.AccountPwdHash;
	promptStr = "Enter email account password: ";

	int minLength = 0;

//	minLength = 8;
//	lengthWarning.clear();
//	lengthWarning = boost::str( boost::format( "WARNING: Password should be %d or more characters for this test!" ) % minLength );
	lengthWarning = "WARNING: Password should be not be blank!";

	input_quit = GetStringInput( entryStr, promptStr, lengthWarning, minLength, HIDE_ENTRY );
	if ( input_quit )
	{
		return input_quit;
	}

	scfgrec.AccountPwdHash = entryStr;

	return false;
}

void ScoutX::print_reagents_definitions(ReagentDefinition *& reagents, size_t num_reagents)
{
	for (size_t i = 0; i<num_reagents; i++)
	{
		auto reagent = reagents[i];
		reagent.reagent_index;
		cout << "Reagent Index : " << std::to_string(reagent.reagent_index) << endl;

		std::string label_str(reagent.label);
		cout << "Reagent Label : " << label_str << endl;
		cout << "Reagent cycles: " << std::to_string(reagent.mixing_cycles) << endl;
		cout << endl;
	}
}

void ScoutX::print_reagents_containerstates( ReagentContainerState*& reagents )
{
	cout << "Reagent Pack BCI# : ";
	std::string bci( reagents->bci_part_number );
	cout << bci << std::endl;
	cout << "   Reagent Identifier       : " << reagents->identifier << endl;
	cout << "   Events Remaining         : " << reagents->events_remaining << endl;
	cout << "   Pack expiration          : " << reagents->exp_date << endl;
	cout << "   In-service date          : " << reagents->in_service_date << endl;
	cout << "   Reagent lot information  : " << std::string( reagents->lot_information ) << endl;
	cout << "   Reagent Pack Position    : " << reagents->position << endl;
	cout << "   Reagent Pack health      : " << reagents->status << endl;

	cout << "----------------------------- " << endl;
	for ( auto i = 0; i < reagents->num_reagents; i++ )
	{
		cout << "   Reagent lot information  : " << std::string( reagents->reagent_states[i].lot_information ) << endl;
		cout << "   Reagent Index            : " << std::to_string( reagents->reagent_states[i].reagent_index ) << endl;
		cout << "   Reagent Valve Number     : " << std::to_string( reagents->reagent_states[i].valve_location ) << endl;
		cout << "   Reagent Events Remaining : " << std::to_string( reagents->reagent_states[i].events_remaining ) << endl;
		cout << "   Reagent Events Possible  : " << std::to_string( reagents->reagent_states[i].events_possible ) << endl;
		cout << endl;
	}
}

void ScoutX::print_db_worklist_list( std::vector<DBApi::DB_WorklistRecord>& wl_list )
{
	std::cout << std::endl;

	if ( wl_list.size() == 0 )
	{
		std::cout << "No Worklist records found." << std::endl;
	}
	else
	{
		std::string idStr;
		std::string runDateStr;
		HawkeyeUUID wlid = {};

		std::cout << "Worklist records found: " << wl_list.size() << std::endl << std::endl;
		for ( auto wlIter = wl_list.begin(); wlIter != wl_list.end(); ++wlIter )
		{
			idStr.clear();
			runDateStr.clear();

			wlid = wlIter->WorklistId;
			idStr = wlid.to_string();
			runDateStr = ChronoUtilities::ConvertToString( wlIter->RunDateTP, "%Y-%m-%d %H:%M:%S" );

			std::cout << "IdNum: " << wlIter->WorklistIdNum << "\tWorklist UUID: " << idStr << "\tRun Date: " << runDateStr << "\tWorklist Name: " << wlIter->WorklistNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_worklist_obj( DBApi::DB_WorklistRecord worklist )
{
	std::string dateStr;

	std::cout << std::endl;
	std::cout << "\tIdNum: " << (uint64_t)worklist.WorklistIdNum << std::endl;
	std::cout << "\tUUID: " << HawkeyeUUID::GetStrFromuuid__t(worklist.WorklistId) << std::endl;
	std::cout << "\tStatus: " << (int32_t)worklist.WorklistStatus << std::endl;
	std::cout << "\tWorklist Name: " << worklist.WorklistNameStr << std::endl;
	std::cout << "\tComments: " << worklist.ListComments << std::endl;
	std::cout << "\tInstrument S/N: " << worklist.InstrumentSNStr << std::endl;
	std::cout << "\tCreate User ID: " << HawkeyeUUID::GetStrFromuuid__t( worklist.CreationUserId ) << std::endl;
	std::cout << "\tCreate User Name: " << worklist.CreationUserNameStr << std::endl;
	std::cout << "\tRun User ID: " << HawkeyeUUID::GetStrFromuuid__t( worklist.RunUserId ) << std::endl;
	std::cout << "\tRun User Name: " << worklist.RunUserNameStr << std::endl;
	dateStr = ChronoUtilities::ConvertToString( worklist.RunDateTP, "%b %d %Y %H:%M:%S" );
	std::cout << "\tRun Date: " << dateStr << std::endl;
	std::cout << "\tAcquire Sample: " << ((worklist.AcquireSample) ? "true" : "false") << std::endl;
	std::cout << "\tCarrier: " << (int32_t) worklist.CarrierType << " ( " << ( carrierTypeToString( static_cast<eCarrierType>(worklist.CarrierType ) ) ) << " )" << std::endl;
	if ( ( worklist.CarrierType == (int32_t) eCarrierType::eCarousel ) )
	{
		std::cout << "\tBy Column: " << ( ( worklist.ByColumn ) ? "true" : "false" ) << std::endl;
	}
	else
	{
		std::cout << "\tBy Column: N/A" << std::endl;
	}
	std::cout << "\tSave Every Nth Image: " << (int32_t) worklist.SaveNthImage << std::endl;
	std::cout << "\tWash type: " << (int32_t) worklist.WashTypeIndex << std::endl;
	std::cout << "\tDilution: " << (int32_t) worklist.Dilution << std::endl;
	std::cout << "\tDefault SampleSet Name: " << worklist.SampleSetNameStr << std::endl;
	std::cout << "\tDefault Sample Item Name: " << worklist.SampleItemNameStr << std::endl;

	std::cout << std::endl;
}

void ScoutX::print_db_sampleset_list( std::vector<DBApi::DB_SampleSetRecord>& ss_list )
{
	std::cout << std::endl;

	if ( ss_list.size() == 0 )
	{
		std::cout << "No SampleSet records found." << std::endl;
	}
	else
	{
		std::string idStr;
		std::string createDateStr;
		std::string modifyDateStr;

		std::cout << "SampleSet records found: " << ss_list.size() << std::endl << std::endl;
		for ( auto ssIter = ss_list.begin(); ssIter != ss_list.end(); ++ssIter )
		{
			HawkeyeUUID objid = ssIter->SampleSetId;

			idStr.clear();
			createDateStr.clear();
			modifyDateStr.clear();

			idStr = objid.to_string();
			createDateStr = ChronoUtilities::ConvertToString( ssIter->CreateDateTP, "%Y-%m-%d %H:%M:%S" );
			modifyDateStr = ChronoUtilities::ConvertToString( ssIter->ModifyDateTP, "%Y-%m-%d %H:%M:%S" );

			std::cout << "IdNum: " << ssIter->SampleSetIdNum << "\tSampleSet UUID: " << idStr << "\tCreate Date: " << createDateStr << "\tModify Date: " << modifyDateStr << "\tSampleSet Name: " << ssIter->SampleSetNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_sampleitem_list( std::vector<DBApi::DB_SampleItemRecord>& si_list )
{
	std::cout << std::endl;

	if ( si_list.size() == 0 )
	{
		std::cout << "No SampleItem records found." << std::endl;
	}
	else
	{
		std::string idStr;
		std::string runDateStr;

		std::cout << "SampleItem records found: " << si_list.size() << std::endl << std::endl;
		for ( auto siIter = si_list.begin(); siIter != si_list.end(); ++siIter )
		{
			HawkeyeUUID objid = siIter->SampleItemId;

			idStr.clear();
			runDateStr.clear();

			idStr = objid.to_string();
			runDateStr = ChronoUtilities::ConvertToString( siIter->RunDateTP, "%Y-%m-%d %H:%M:%S" );

			std::cout << "IdNum: " << siIter->SampleItemIdNum << "\tSampleItem UUID: " << idStr << "\tRun Date: " << runDateStr << "\tSampleItem Name: " << siIter->SampleItemNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_samples_list( std::vector<DBApi::DB_SampleRecord>& s_list )
{
	std::cout << std::endl;

	if ( s_list.size() == 0 )
	{
		std::cout << "No Sample records found." << std::endl;
	}
	else
	{
		std::string idStr;
		std::string acqDateStr;

		std::cout << "Sample records found: " << s_list.size() << std::endl << std::endl;
		for ( auto sIter = s_list.begin(); sIter != s_list.end(); ++sIter )
		{
			HawkeyeUUID objid = sIter->SampleId;

			idStr.clear();
			acqDateStr.clear();

			idStr = objid.to_string();
			acqDateStr = ChronoUtilities::ConvertToString( sIter->AcquisitionDateTP, "%Y-%m-%d %H:%M:%S" );

			std::cout << "IdNum: " << sIter->SampleIdNum << "\tSample UUID: " << idStr << "\tAcquisition Date: " << acqDateStr << "\tSample Name: " << sIter->SampleNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_imageset_list( std::vector<DBApi::DB_ImageSetRecord>& is_list )
{
	std::cout << std::endl;

	if ( is_list.size() == 0 )
	{
		std::cout << "No ImageSet records found." << std::endl;
	}
	else
	{
		std::cout << "ImageSet records found: " << is_list.size() << std::endl << std::endl;
		for ( auto isIter = is_list.begin(); isIter != is_list.end(); ++isIter )
		{
			std::string objIdStr;
			std::string sampleIdStr;
			std::string dateStr;
			HawkeyeUUID objId = isIter->ImageSetId;
			HawkeyeUUID sampleId = isIter->SampleId;

			objIdStr = objId.to_string();
			sampleIdStr = sampleId.to_string();
			dateStr = ChronoUtilities::ConvertToString( isIter->CreationDateTP, "%Y-%m-%d %H:%M:%S" );

			std::cout << "IdNum: " << isIter->ImageSetIdNum << "\tImageSet UUID: " << objIdStr << "\tImageSeq Cnt: " << (int32_t)(isIter->ImageSequenceCount) << "\tSample UUID: " << sampleIdStr << "\tCreation Date: " << dateStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_imageseq_list( std::vector<DBApi::DB_ImageSeqRecord>& is_list )
{
	std::cout << std::endl;

	if ( is_list.size() == 0 )
	{
		std::cout << "No ImageSequence records found." << std::endl;
	}
	else
	{
		std::cout << "ImageSequence records found: " << is_list.size() << std::endl << std::endl;
		for ( auto isIter = is_list.begin(); isIter != is_list.end(); ++isIter )
		{
			std::string objIdStr;
			std::string setIdStr;
			HawkeyeUUID objId = isIter->ImageSequenceId;
			HawkeyeUUID setId = isIter->ImageSetId;

			objIdStr = objId.to_string();
			setIdStr = setId.to_string();

			std::cout << "IdNum: " << isIter->ImageSequenceIdNum << "\tImageSeq UUID: " << objIdStr << "\tImageSet UUID: " << setIdStr << "\tImage Cnt: " << (int32_t)(isIter->ImageCount) << "\tSequence Num: " << isIter->SequenceNum << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_image_list( std::vector<DBApi::DB_ImageRecord>& img_list )
{
	std::cout << std::endl;

	if ( img_list.size() == 0 )
	{
		std::cout << "No Image records found." << std::endl;
	}
	else
	{
		std::cout << "Image records found: " << img_list.size() << std::endl << std::endl;
		for ( auto iIter = img_list.begin(); iIter != img_list.end(); ++iIter )
		{
			std::string objIdStr;
			std::string seqIdStr;
			HawkeyeUUID objId = iIter->ImageId;
			HawkeyeUUID seqId = iIter->ImageSequenceId;

			objIdStr = objId.to_string();
			seqIdStr = seqId.to_string();

			std::cout << "IdNum: " << iIter->ImageIdNum << "\tImage UUID: " << objIdStr << "\tImageSeq UUID: " << seqIdStr << "\tImage Channel: " << (int32_t)( iIter->ImageChannel) << "\tFile Name: " << iIter->ImageFileNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_celltypes_list( std::vector<DBApi::DB_CellTypeRecord>& ct_list )
{
	std::cout << std::endl;

	if ( ct_list.size() == 0 )
	{
		std::cout << "No CellType records found." << std::endl;
	}
	else
	{
		std::cout << "CellType records found: " << ct_list.size() << std::endl << std::endl;
		for ( auto ctIter = ct_list.begin(); ctIter != ct_list.end(); ++ctIter )
		{
			std::string idStr;
			HawkeyeUUID objid = ctIter->CellTypeId;

			idStr = objid.to_string();
			std::cout << "IdNum: " << ctIter->CellTypeIdNum << "\tCellType UUID: " << idStr << "\tCellType Index: " << ctIter->CellTypeIndex << "\tCellType Name: " << ctIter->CellTypeNameStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_analysis_def_list( std::vector<DBApi::DB_AnalysisDefinitionRecord>& def_list )
{
	std::cout << std::endl;

	if ( def_list.size() == 0 )
	{
		std::cout << "No Analysis Definition records found." << std::endl;
	}
	else
	{
		std::cout << "Analysis Definition records found: " << def_list.size() << std::endl << std::endl;
		for ( auto defIter = def_list.begin(); defIter != def_list.end(); ++defIter )
		{
			std::string idStr;
			HawkeyeUUID objid = defIter->AnalysisDefId;

			idStr = objid.to_string();
			std::cout << "IdNum: " << defIter->AnalysisDefIdNum << "\tAnalysis Definition UUID: " << idStr << "\tAnalysis Definition Label: " << defIter->AnalysisLabel << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_analysis_param_list( std::vector<DBApi::DB_AnalysisParamRecord>& param_list )
{
	std::cout << std::endl;

	if ( param_list.size() == 0 )
	{
		std::cout << "No Analysis Parameter records found." << std::endl;
	}
	else
	{
		std::cout << "Analysis Parameter records found: " << param_list.size() << std::endl << std::endl;
		for ( auto paramIter = param_list.begin(); paramIter != param_list.end(); ++paramIter )
		{
			std::string idStr;
			HawkeyeUUID objid = paramIter->ParamId;

			idStr = objid.to_string();
			std::cout << "IdNum: " << paramIter->ParamIdNum << "\tAnalysis Parameter UUID: " << idStr << "\tAnalysis Parameter Label: " << paramIter->ParamLabel << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_user_list( std::vector<DBApi::DB_UserRecord>& user_list )
{
	std::cout << std::endl;

	if ( user_list.size() == 0 )
	{
		std::cout << "No User records found." << std::endl;
	}
	else
	{
		std::cout << "User records found: " << user_list.size() << std::endl << std::endl;
		for ( auto uIter = user_list.begin(); uIter != user_list.end(); ++uIter )
		{
			DBApi::DB_UserRecord userRec = *uIter;

			print_db_user_record( userRec );
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_user_record( DBApi::DB_UserRecord& user_rec )
{
	std::string userIdStr;
	HawkeyeUUID objid = user_rec.UserId;
	std::string roleIdStr;
	HawkeyeUUID roleid = user_rec.RoleId;
	std::string userNameStr = "";
	std::string displayNameStr = "";
	std::string colStr = "";

	userNameStr = boost::str( boost::format( "%-24s" ) % user_rec.UserNameStr );
	displayNameStr = boost::str( boost::format( "%-24s" ) % user_rec.DisplayNameStr );
	userIdStr = objid.to_string();
	roleIdStr = objid.to_string();

	for ( int i = 0; i < user_rec.ColumnDisplayList.size(); i++ )
	{
		DBApi::display_column_info_t colInfo = user_rec.ColumnDisplayList.at( i );
		colStr.append( boost::str( boost::format( "\r\n\tt: %2d p: %2d w: %3d v: %s" ) % colInfo.ColumnType % colInfo.OrderIndex % colInfo.Width % ( ( colInfo.Visible == true ) ? "t" : "f" ) ) );
	}

	std::cout << "IdNum: " << user_rec.UserIdNum << "\tUser UUID: " << userIdStr << "\t    Active/Retired: " << ( ( user_rec.Retired ) ? "retired" : "active" ) << "\t    AD user: " << ( ( user_rec.ADUser ) ? "true" : "false" ) << "\t    Role UUID: " << roleIdStr << "\t    User Name: " << userNameStr << "\tDisplay Name: " << displayNameStr << "Column Info: " << colStr << std::endl;
}

void ScoutX::print_db_signature_list( std::vector<DBApi::DB_SignatureRecord>& sig_list )
{
	std::cout << std::endl;

	if ( sig_list.size() == 0 )
	{
		std::cout << "No Signature Definition records found." << std::endl;
	}
	else
	{
		std::cout << "Signature Definition records found: " << sig_list.size() << std::endl << std::endl;

		for ( auto sigIter = sig_list.begin(); sigIter != sig_list.end(); ++sigIter )
		{
			std::string idStr;
			HawkeyeUUID objid = sigIter->SignatureDefId;

			idStr = objid.to_string();
			std::cout << "IdNum: " << sigIter->SignatureDefIdNum << "\tSignature UUID: " << idStr << "\t\tLong Signature: " << sigIter->LongSignatureStr << "\tShort Signature: " << sigIter->ShortSignatureStr  << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_roles_list( std::vector<DBApi::DB_UserRoleRecord>& roles_list )
{
	std::cout << std::endl;

	if ( roles_list.size() == 0 )
	{
		std::cout << "No Role records found." << std::endl;
	}
	else
	{
		std::cout << "Role records found: " << roles_list.size() << std::endl << std::endl;

		std::string name_str = "";

		for ( auto roleIter = roles_list.begin(); roleIter != roles_list.end(); ++roleIter )
		{
			std::string idStr;
			HawkeyeUUID objid = roleIter->RoleId;
			std::string instrument_permissions_str = "0x00000000";
			std::string app_permissions_str = "0x00000000";

			name_str = boost::str( boost::format( "%-24s" ) % roleIter->RoleNameStr );
			instrument_permissions_str = boost::str( boost::format( "%X" ) % roleIter->InstrumentPermissions );
			app_permissions_str = boost::str( boost::format( "%X" ) % roleIter->ApplicationPermissions );

			idStr = objid.to_string();
			std::cout << "IdNum: " << roleIter->RoleIdNum << "\tRole UUID: " << idStr << "\t\tRole Name: " << name_str << "\tInstrument Permissions: " << instrument_permissions_str << "\t\tApp Permissions: " << app_permissions_str << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_calibrations_list( std::vector<DBApi::DB_CalibrationRecord>& cal_list )
{
	std::cout << std::endl;

	if ( cal_list.size() == 0 )
	{
		std::cout << "No Calibration records found." << std::endl;
	}
	else
	{
		std::cout << "Calibration records found: " << cal_list.size() << std::endl << std::endl;

		std::string name_str = "";

		for ( auto cIter = cal_list.begin(); cIter != cal_list.end(); ++cIter )
		{
			std::string idStr;
			HawkeyeUUID objid = cIter->CalId;
			std::string instrument_sn_str = "SN ";
			std::string cal_date_str = "0x00000000";

			instrument_sn_str = boost::str( boost::format( "SN %-32s" ) % cIter->InstrumentSNStr );
			cal_date_str = ChronoUtilities::ConvertToString( cIter->CalDate, "%Y-%m-%d %H:%M:%S" );

			idStr = objid.to_string();
			std::cout << "IdNum: " << cIter->CalIdNum << "\tCal UUID: " << idStr << "\t\tCal Instrument SN: " << instrument_sn_str << "\tCal Date: " << cal_date_str << "\tCal type: " << cIter->CalType << "\tSlope: " << cIter->Slope << "\tIntercept: " << cIter->Intercept << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_qc_process_list( std::vector<DBApi::DB_QcProcessRecord>& qc_list )
{
	std::cout << std::endl;

	if ( qc_list.size() == 0 )
	{
		std::cout << "No QC process records found." << std::endl;
	}
	else
	{
		std::cout << "QC process records found: " << qc_list.size() << std::endl << std::endl;

		std::string name_str = "";

		for ( auto qcIter = qc_list.begin(); qcIter != qc_list.end(); ++qcIter )
		{
			std::string idStr;
			HawkeyeUUID objid = qcIter->QcId;

			idStr = objid.to_string();
			std::cout << "IdNum: " << qcIter->QcIdNum << "\tQC UUID: " << idStr << "\t\tQC name: " << qcIter->QcName << "\tQC Lot Number: " << qcIter->LotInfo << "\tQC Assay value: " << qcIter->AssayValue << "\tQC comments: " << qcIter->Comments << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_analyses_list( std::vector<DBApi::DB_AnalysisRecord>& an_list )
{
	std::cout << std::endl;

	if ( an_list.size() == 0 )
	{
		std::cout << "No analysis records found." << std::endl;
	}
	else
	{
		std::cout << "Analysis records found: " << an_list.size() << std::endl << std::endl;
		for ( auto anIter = an_list.begin(); anIter != an_list.end(); ++anIter )
		{
			HawkeyeUUID objid = anIter->AnalysisId;
			std::string idStr = objid.to_string();

			objid = anIter->SampleId;
			std::string sampleIdStr = objid.to_string();

			objid = anIter->AnalysisUserId;
			std::string userIdStr = objid.to_string();

			objid = anIter->ImageSetId;
			std::string imageSetIdStr = objid.to_string();

			std::cout << "IdNum: " << anIter->AnalysisIdNum << "\tAnalysis UUID: " << idStr << "\tSample UUID: " << sampleIdStr << "\tUser UUID: " << userIdStr << "\tImageSet UUID: " << imageSetIdStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_summary_results_list( std::vector<DBApi::DB_SummaryResultRecord>& summary_list )
{
	std::cout << std::endl;

	if ( summary_list.size() == 0 )
	{
		std::cout << "No Summary result records found." << std::endl;
	}
	else
	{
		std::cout << "SummaryResult records found: " << summary_list.size() << std::endl << std::endl;
		for ( auto resultIter = summary_list.begin(); resultIter != summary_list.end(); ++resultIter )
		{
			HawkeyeUUID objid = resultIter->SummaryResultId;
			std::string idStr = objid.to_string();

			objid = resultIter->SampleId;
			std::string sampleIdStr = objid.to_string();

			objid = resultIter->AnalysisId;
			std::string analysisIdStr = objid.to_string();

//			objid = resultIter->OwnerUserId;
//			std::string ownerIdStr = objid.to_string();

			objid = resultIter->ImageSetId;
			std::string imageSetIdStr = objid.to_string();

			std::cout << "IdNum: " << resultIter->SummaryResultIdNum << "\tResult UUID: " << idStr << "\tSample UUID: " << sampleIdStr << "\tAnalysis UUID: " << analysisIdStr /*<< "\tOwner UUID: " << ownerIdStr */ << "\tImageSet UUID: " << imageSetIdStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_detailed_results_list( std::vector<DBApi::DB_DetailedResultRecord>& detailed_list )
{
	std::cout << std::endl;

	if ( detailed_list.size() == 0 )
	{
		std::cout << "No DetailedResults records found." << std::endl;
	}
	else
	{
		std::cout << "DetailedResults records found: " << detailed_list.size() << std::endl << std::endl;
		for ( auto resultIter = detailed_list.begin(); resultIter != detailed_list.end(); ++resultIter )
		{
			print_db_detailed_result( *resultIter );
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_detailed_result( DBApi::DB_DetailedResultRecord& detailed_result )
{
	std::cout << std::endl;

	HawkeyeUUID objid = detailed_result.DetailedResultId;
	std::string idStr = objid.to_string();

	objid = detailed_result.SampleId;
	std::string sampleIdStr = objid.to_string();

	objid = detailed_result.AnalysisId;
	std::string analysisIdStr = objid.to_string();

	objid = detailed_result.OwnerUserId;
	std::string ownerIdStr = objid.to_string();

	objid = detailed_result.ImageId;
	std::string imageIdStr = objid.to_string();

	std::cout << "IdNum: " << detailed_result.DetailedResultIdNum << "\tResult UUID: " << idStr << "\tSample UUID: " << sampleIdStr << "\tAnalysis UUID: " << analysisIdStr << "\tOwner UUID: " << ownerIdStr << "\tImage UUID: " << imageIdStr << std::endl;
}

void ScoutX::print_db_image_results_list( std::vector<DBApi::DB_ImageResultRecord>& ir_list, int rec_format )
{
	std::cout << std::endl;

	if ( ir_list.size() == 0 )
	{
		std::cout << "No Image Result records found." << std::endl;
	}
	else
	{
		std::string lineStr = "";
		std::ofstream resultfile;


		lineStr = boost::str( boost::format( "\\Instrument\\Logs\\ImageResults.%d.txt" ) % rec_format );
		resultfile.open( lineStr, std::ios_base::app );
		lineStr = boost::str( boost::format( "Image Result records found : %d" ) % ir_list.size() );
		if ( resultfile.is_open() )
		{
			resultfile << "________________________________________________________________________________" << std::endl << std::endl;
			if ( rec_format < 0 )
			{
				resultfile << "Image Results List using default format." << std::endl << std::endl;
			}
			else
			{
				resultfile << "Image Results List using format: " << rec_format << std::endl << std::endl;
			}
			resultfile << lineStr << std::endl << std::endl;
		}

		std::cout << lineStr << std::endl << std::endl;

		for ( auto irIter = ir_list.begin(); irIter != ir_list.end(); ++irIter )
		{
			print_db_image_result( *irIter, lineStr );

			if ( resultfile.is_open() && lineStr.length() > 0 )
			{
				resultfile << lineStr;
			}
		}

		if ( resultfile.is_open() )
		{
			resultfile.close();
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_image_result( DBApi::DB_ImageResultRecord& ir, std::string & line_str )
{
	std::string lineStr = "";
	HawkeyeUUID objid = ir.ResultId;
	std::string idStr = objid.to_string();

	line_str.clear();

	objid = ir.SampleId;
	std::string sampleIdStr = objid.to_string();

	objid = ir.ImageId;
	std::string imageIdStr = objid.to_string();

	objid = ir.AnalysisId;
	std::string analysisIdStr = objid.to_string();

	lineStr.clear();

	lineStr = boost::str( boost::format( "IdNum: %05lld    ResultId: %s    SampleID: %s    ImageID: %s    AnalysisID: %s    ImageSeq: %3d    NumBlobs: %5d(%5d)    NumClusters: %5d(%5d)") % ir.ResultIdNum % idStr % sampleIdStr % imageIdStr % analysisIdStr % ir.ImageSeqNum % ir.NumBlobs % ir.BlobDataList.size() % ir.NumClusters % ir.ClusterDataList.size() );
	int listIdx = 0;
	for ( listIdx = 0; listIdx < ir.BlobDataList.size(); listIdx++ )
	{
		lineStr.append( boost::str( boost::format( "\n\t\tBlob Index: %5d    BlobInfoCnt: %3d    BlobCenterX: %4d    BlobCenterY: %4d    BlobOutlineCnt: %3d" ) % listIdx % ir.BlobDataList.at(listIdx).blob_info.size() % ir.BlobDataList.at( listIdx ).blob_center.startx % ir.BlobDataList.at( listIdx ).blob_center.starty % ir.BlobDataList.at( listIdx ).blob_outline.size() ) );
	}
	lineStr.append( "\n" );
	for ( listIdx = 0; listIdx < ir.ClusterDataList.size(); listIdx++ )
	{
		lineStr.append( boost::str( boost::format( "\n\t\tCluster Index: %5d    Cluster CellCnt: %3d    ClusterPolygonVertexCnt: %3d    ClusterBoxX: %4d    ClusterBoxY: %4d    ClusterBoxWidth: %4d    ClusterBoxHeight: %4d" ) % listIdx % ir.ClusterDataList.at( listIdx ).cell_count % ir.ClusterDataList.at( listIdx ).cluster_polygon.size() % ir.ClusterDataList.at( listIdx ).cluster_box.start.startx % ir.ClusterDataList.at( listIdx ).cluster_box.start.starty % ir.ClusterDataList.at( listIdx ).cluster_box.width % ir.ClusterDataList.at( listIdx ).cluster_box.height ) );
	}
	std::cout << lineStr << std::endl;

	line_str = lineStr;
}

void ScoutX::print_db_reagent_info_list( std::vector<DBApi::DB_ReagentTypeRecord>& rx_list )
{
	std::cout << std::endl;

	if ( rx_list.size() == 0 )
	{
		std::cout << "No Reagent info records found." << std::endl;
	}
	else
	{
		std::string lineStr;
		std::string expDateStr;
		std::string serviceDateStr;

		std::cout << "Reagent info records found: " << rx_list.size() << std::endl << std::endl;
		for ( auto rxIter = rx_list.begin(); rxIter != rx_list.end(); ++rxIter )
		{
			expDateStr.clear();
			serviceDateStr.clear();
			lineStr.clear();

			expDateStr = ChronoUtilities::ConvertToDateString( rxIter->LotExpirationDate );
			serviceDateStr = ChronoUtilities::ConvertToDateString( rxIter->InServiceDate );

			lineStr = boost::str( boost::format( "%s IdNum: %05lld    Tag Serial #: %20s    P/N: %10s    Lot #: %10s    Exp Date: %10s    In-service: %10s    Service Life: %d" ) % ( rxIter->Current == true ? "*" : " " ) % rxIter->ReagentIdNum % rxIter->ContainerTagSn % rxIter->PackPartNumStr % rxIter->LotNumStr % expDateStr %serviceDateStr % rxIter->InServiceExpirationLength );
			for ( int i = 0; i < rxIter->ReagentIndexList.size(); i++ )
			{
				lineStr.append( boost::str( boost::format( "\n\t\t\t[ Index: %02d  Rx Name: %20s  MixingCycles: %d ]" ) % rxIter->ReagentIndexList.at( i ) % rxIter->ReagentNamesList.at( 1 ) % rxIter->MixingCyclesList.at( i ) ) );
			}
			std::cout << lineStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_instrument_config_list( std::vector<DBApi::DB_InstrumentConfigRecord>& cfg_list )
{
	std::cout << std::endl;

	if ( cfg_list.size() == 0 )
	{
		std::cout << "No instrument configuration records found." << std::endl;
	}
	else
	{
		std::cout << "instrument configuration records found: " << cfg_list.size() << std::endl << std::endl;

		std::string name_str = "";

		for ( auto cIter = cfg_list.begin(); cIter != cfg_list.end(); ++cIter )
		{
			std::string instrument_sn_str = "";

			instrument_sn_str = boost::str( boost::format( "SN %-32s" ) % cIter->InstrumentSNStr );

			std::cout << "IdNum: " << cIter->InstrumentIdNum << "\t\tInstrument SN: " << instrument_sn_str << "\tInst type: " << cIter->InstrumentType << "\tUI ver: " << cIter->UIVersion << "\tSW ver: " << cIter->SoftwareVersion << "\tFW ver: " << cIter->FirmwareVersion << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_instrument_config( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;

	std::cout << "instrument configuration settings: " << std::endl << std::endl;

	std::cout << "\tIdNum:\t\t\t\t" << cfgRec.InstrumentIdNum << std::endl;
	std::cout << "\tInst SN:\t\t\t" << cfgRec.InstrumentSNStr << std::endl;
	std::cout << "\tInst Type:\t\t\t" << cfgRec.InstrumentType << std::endl;
	std::cout << "\tDeviceName:\t\t\t" << cfgRec.DeviceName << std::endl;
	std::cout << "\tUIVersion:\t\t\t" << cfgRec.UIVersion << std::endl;
	std::cout << "\tSoftwareVersion:\t\t" << cfgRec.SoftwareVersion << std::endl;
	std::cout << "\tAnalysisVersion:\t\t" << cfgRec.AnalysisVersion << std::endl;
	std::cout << "\tFirmwareVersion:\t\t" << cfgRec.FirmwareVersion << std::endl;
	std::cout << "\tCameraType:\t\t\t" << cfgRec.CameraType << std::endl;
	std::cout << "\tCameraFWVersion:\t\t" << cfgRec.CameraFWVersion << std::endl;
	if ( cfgRec.CameraConfig.length() > 0 )
	{
		std::cout << "\tCameraConfig:" << std::endl << cfgRec.CameraConfig << std::endl << std::endl;
	}
	else
	{
		std::cout << "\tCameraConfig:\t\t\t' '"<< std::endl;
	}
	std::cout << "\tPumpType:\t\t\t" << cfgRec.PumpType << std::endl;
	std::cout << "\tPumpFWVersion:\t\t\t" << cfgRec.PumpFWVersion << std::endl;
	if ( cfgRec.CameraConfig.length() > 0 )
	{
		std::cout << "\tPumpConfig:" << std::endl << cfgRec.CameraConfig << std::endl << std::endl;
	}
	else
	{
		std::cout << "\tPumpConfig:\t\t\t' '" << std::endl;
	}

	print_db_instrument_config_illuminators_list( cfgRec );

	if ( cfgRec.CameraConfig.length() > 0 )
	{
		std::cout << "\tIlluminatorConfig:" << std::endl << cfgRec.CameraConfig << std::endl << std::endl;
	}
	else
	{
		std::cout << "\tIlluminatorConfig:\t\t' '" << std::endl;
	}
	std::cout << "\tConfigType:\t\t\t" << cfgRec.ConfigType << std::endl;
	std::cout << "\tLogName:\t\t\t" << cfgRec.LogName << std::endl;
	std::cout << "\tLogMaxSize:\t\t\t" << cfgRec.LogMaxSize << std::endl;
	std::cout << "\tLogSensitivity:\t\t\t" << cfgRec.LogSensitivity << std::endl;
	std::cout << "\tMaxLogs:\t\t\t" << cfgRec.MaxLogs << std::endl;
	std::cout << "\tAlwaysFlush:\t\t\t" << cfgRec.AlwaysFlush << std::endl;
	std::cout << "\tCameraErrorLogName:\t\t" << cfgRec.CameraErrorLogName << std::endl;
	std::cout << "\tCameraErrorLogMaxSize:\t\t" << cfgRec.CameraErrorLogMaxSize << std::endl;
	std::cout << "\tStorageErrorLogName:\t\t" << cfgRec.StorageErrorLogName << std::endl;
	std::cout << "\tStorageErrorLogMaxSize:\t\t" << cfgRec.StorageErrorLogMaxSize << std::endl;
	std::cout << "\tCarouselThetaHome:\t\t" << cfgRec.CarouselThetaHomeOffset << std::endl;
	std::cout << "\tCarouselRadiusOffset:\t\t" << cfgRec.CarouselRadiusOffset << std::endl;
	std::cout << "\tPlateThetaHome:\t\t\t" << cfgRec.PlateThetaHomeOffset << std::endl;
	std::cout << "\tPlateThetaCal:\t\t\t" << cfgRec.PlateThetaCalPos << std::endl;
	std::cout << "\tPlateRadiusCenter:\t\t" << cfgRec.PlateRadiusCenterPos << std::endl;
	std::cout << "\tSaveImage:\t\t\t" << cfgRec.SaveImage << std::endl;
	std::cout << "\tFocusPosition:\t\t\t" << cfgRec.FocusPosition << std::endl;

	print_db_instrument_config_af_settings( cfgRec );

	std::cout << "\tAbiMaxImageCount:\t\t" << cfgRec.AbiMaxImageCount << std::endl;
	std::cout << "\tSampleNudgeVolume:\t\t" << cfgRec.SampleNudgeVolume << std::endl;
	std::cout << "\tSampleNudgeSpeed:\t\t" << cfgRec.SampleNudgeSpeed << std::endl;
	std::cout << "\tFlowCellDepth:\t\t\t" << cfgRec.FlowCellDepth << std::endl;
	std::cout << "\tFlowCellDepthConstant:\t\t" << cfgRec.FlowCellDepthConstant << std::endl;

	print_db_instrument_config_rfid_settings( cfgRec );

	std::cout << "\tLegacyData:\t\t\t" << ( ( cfgRec.LegacyData == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tCarouselSim:\t\t\t" << ( ( cfgRec.CarouselSimulator == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tNightlyCleanOffset:\t\t" << cfgRec.NightlyCleanOffset << std::endl;		// offset from midnight in minutes
	std::cout << "\tLastNightlyClean:\t\t" << ( ChronoUtilities::ConvertToString( cfgRec.LastNightlyClean, "'%Y-%m-%d %H:%M:%S'" ) ) << std::endl;
	std::cout << "\tSecurityMode:\t\t\t" << cfgRec.SecurityMode << std::endl;
	std::cout << "\tInactivityTimeout:\t\t" << cfgRec.InactivityTimeout << std::endl;		// inactivity timeout in seconds?
	std::cout << "\tPwdExpiration:\t\t\t" << cfgRec.PasswordExpiration << std::endl;		// password expiration in days
	std::cout << "\tNormalShutdown:\t\t\t" << ( ( cfgRec.NormalShutdown == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tTotalSamples:\t\t\t" << cfgRec.TotalSamplesProcessed << std::endl;
	std::cout << "\tDiscardCapacity:\t\t" << cfgRec.DiscardTrayCapacity << std::endl;

	std::cout << "\tAutomationInstalled:\t\t" << ( ( cfgRec.AutomationInstalled == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tAutomationEnabled:\t\t" << ( ( cfgRec.AutomationEnabled == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tACupEnabled:\t\t\t" << ((cfgRec.ACupEnabled == true) ? "true" : "false") << std::endl;
	std::cout << "\tAutomationPort:\t\t\t" << cfgRec.AutomationPort << std::endl;

	print_db_instrument_config_email_settings( cfgRec );

	print_db_instrument_config_ad_settings( cfgRec );

	print_db_instrument_config_lang_list( cfgRec );

	print_db_instrument_config_run_options( cfgRec );

	std::cout << std::endl;
}

void ScoutX::print_db_instrument_config_illuminators_list( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration illuminator list: " << std::endl;

	size_t listSize = cfgRec.IlluminatorsInfoList.size();

	if ( listSize == 0 )
	{
		std::cout << "No illuminators in the list: (" << listSize << ")" << std::endl;
	}
	else
	{
		for ( int i = 0; i < listSize; i++ )
		{
			DBApi::illuminator_info_t illum = cfgRec.IlluminatorsInfoList.at( i );

			std::cout << std::endl;
			std::cout << "\tIlliuminator " << i << ":" << std::endl;
			std::cout << "\t\tIlluminatorType:\t" << illum.type << std::endl;
			std::cout << "\t\tIlluminatorIndex:\t" << illum.index << std::endl;
		}
	}

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_af_settings( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration auto-focus settings: " << std::endl << std::endl;

	std::cout << "\tsave_image:\t\t\t" << ( ( cfgRec.AF_Settings.save_image == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tcoarse_start:\t\t\t" << cfgRec.AF_Settings.coarse_start << std::endl;
	std::cout << "\tcoarse_end:\t\t\t" << cfgRec.AF_Settings.coarse_end << std::endl;
	std::cout << "\tcoarse_step:\t\t\t" << cfgRec.AF_Settings.coarse_step << std::endl;
	std::cout << "\tfine_range:\t\t\t" << cfgRec.AF_Settings.fine_range << std::endl;
	std::cout << "\tfine_step:\t\t\t" << cfgRec.AF_Settings.fine_step << std::endl;
	std::cout << "\tsharpness_low_threshold:\t" << cfgRec.AF_Settings.sharpness_low_threshold << std::endl;

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_rfid_settings( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration rfid settings: " << std::endl << std::endl;

	std::cout << "\tset_valid_tag_data:\t\t" << ( ( cfgRec.RfidSim.set_valid_tag_data == true ) ? "true" : "false") << std::endl;
	std::cout << "\ttotal_tags:\t\t\t" << cfgRec.RfidSim.total_tags << std::endl;
	std::cout << "\tmain_bay_file:\t\t\t" << cfgRec.RfidSim.main_bay_file << std::endl;
	std::cout << "\tdoor_left_file:\t\t\t" << cfgRec.RfidSim.door_left_file << std::endl;
	std::cout << "\tdoor_right_file:\t\t" << cfgRec.RfidSim.door_right_file << std::endl;

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_email_settings( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration email settings: " << std::endl << std::endl;

	std::cout << "\tserver_addr:\t\t\t" << cfgRec.Email_Settings.server_addr << std::endl;
	std::cout << "\tport_number:\t\t\t" << cfgRec.Email_Settings.port_number << std::endl;
	std::cout << "\tauthenticate:\t\t\t" << ( ( cfgRec.Email_Settings.authenticate == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tusername:\t\t\t" << cfgRec.Email_Settings.username << std::endl;
	std::cout << "\tpwd_hash:\t\t\t" << cfgRec.Email_Settings.pwd_hash << std::endl;

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_ad_settings( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration Active Directory settings: " << std::endl << std::endl;

	std::cout << "\tservername:\t\t\t" << cfgRec.AD_Settings.servername << std::endl;
	std::cout << "\tserver_addr:\t\t\t" << cfgRec.AD_Settings.server_addr << std::endl;
	std::cout << "\tport_number:\t\t\t" << cfgRec.AD_Settings.port_number << std::endl;
	std::cout << "\tbase_dn:\t\t\t" << cfgRec.AD_Settings.base_dn << std::endl;
	std::cout << "\tenabled:\t\t\t" << ( ( cfgRec.AD_Settings.enabled == true ) ? "true" : "false" ) << std::endl;

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_lang_list( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration Language list: " << std::endl;

	for ( auto cIter = cfgRec.LanguageList.begin(); cIter != cfgRec.LanguageList.end(); ++cIter )
	{
		DBApi::language_info_t lang = *cIter;

		std::cout << std::endl;
		std::cout << "\tlanguage_id:\t\t\t" << lang.language_id << std::endl;
		std::cout << "\tlanguage_name:\t\t\t" << lang.language_name << std::endl;
		std::cout << "\tlocale_tag:\t\t\t" << lang.locale_tag << std::endl;
		std::cout << "\tactive:\t\t\t\t" << ( ( lang.active == true ) ? "true" : "false" ) << std::endl;
	}

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_instrument_config_run_options( DBApi::DB_InstrumentConfigRecord cfgRec )
{
	std::cout << std::endl;
	std::cout << "===========================================================" << std::endl;

	std::cout << "instrument configuration run-options settings: " << std::endl << std::endl;

	std::cout << "\tsample_set_name:\t\t" << cfgRec.RunOptions.sample_set_name << std::endl;
	std::cout << "\tsample_name:\t\t\t" << cfgRec.RunOptions.sample_name << std::endl;
	std::cout << "\tsave_image_count:\t\t" << cfgRec.RunOptions.save_image_count << std::endl;
	std::cout << "\tsave_nth_image:\t\t\t" << cfgRec.RunOptions.save_nth_image << std::endl;
	std::cout << "\tresults_export:\t\t\t" << ( ( cfgRec.RunOptions.results_export == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tresults_export_folder:\t\t" << cfgRec.RunOptions.results_export_folder << std::endl;
	std::cout << "\tappend_results_export:\t\t" << ( ( cfgRec.RunOptions.append_results_export == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tappend_results_export_folder:\t" << cfgRec.RunOptions.append_results_export_folder << std::endl;
	std::cout << "\tresult_filename:\t\t" << cfgRec.RunOptions.result_filename << std::endl;
	std::cout << "\tresults_folder:\t\t\t" << cfgRec.RunOptions.results_folder << std::endl;
	std::cout << "\tauto_export_pdf:\t\t" << ( ( cfgRec.RunOptions.auto_export_pdf == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tcsv_folder:\t\t\t" << cfgRec.RunOptions.csv_folder << std::endl;
	std::cout << "\twash_type:\t\t\t" << cfgRec.RunOptions.wash_type << std::endl;
	std::cout << "\tdilution:\t\t\t" << cfgRec.RunOptions.dilution << std::endl;
	std::cout << "\tbpqc_cell_type_index:\t\t" << cfgRec.RunOptions.bpqc_cell_type_index << std::endl;

	std::cout << "===========================================================" << std::endl << std::endl;
}

void ScoutX::print_db_log_lists( std::vector<DBApi::DB_LogEntryRecord>& log_list, DBApi::eLogEntryType log_type )
{
	std::cout << std::endl;

	std::string typeStr = "";
	std::string logTypeStr = "";
	int16_t logType = static_cast<int16_t>( log_type );

	switch ( log_type )
	{
		case DBApi::eLogEntryType::NoLogType:
		{
			typeStr = "No";
			break;
		}

		case DBApi::eLogEntryType::AllLogTypes:
		{
			typeStr = "All";
			break;
		}

		case DBApi::eLogEntryType::AuditLogType:
		{
			typeStr = "Audit";
			break;
		}

		case DBApi::eLogEntryType::ErrorLogType:
		{
			typeStr = "Error";
			break;
		}

		case DBApi::eLogEntryType::SampleLogType:
		{
			typeStr = "Sample";
			break;
		}

		default:
			typeStr = "Unrecognized";
			logTypeStr = boost::str( boost::format( "Unrecognized or undefined log type (%d)" ) % logType );
			break;
	}

	if ( logTypeStr.length() <= 0 )
	{
		logTypeStr = boost::str( boost::format( "%s log type (%d)" ) % typeStr % logType );
	}

	std::string msgStr = "";
	if ( log_list.size() == 0 )
	{
		msgStr = boost::str( boost::format( "No log entry records for %s found." ) % logTypeStr );
		std::cout << msgStr << std::endl;
	}
	else
	{
		msgStr = boost::str( boost::format( "Log entry records for %s found: %d." ) % logTypeStr % log_list.size() );
		std::cout << msgStr << std::endl << std::endl;

		std::string logDateStr = "";
		std::string logLineStr = "";

		for ( auto logIter = log_list.begin(); logIter != log_list.end(); ++logIter )
		{
			logDateStr.clear();
			logLineStr.clear();

			logDateStr = ChronoUtilities::ConvertToString( logIter->EntryDate, "%Y-%m-%d %H:%M:%S" );

			logLineStr = boost::str( boost::format( "IdNum: %-10s  \tEntry Date: %-20s  \t" ) % logIter->IdNum % logDateStr );
			if ( logType <= static_cast<int16_t>(DBApi::eLogEntryType::AllLogTypes) )
			{
				logLineStr.append( boost::str( boost::format( "Entry Type: %-2d  \t" ) % logIter->EntryType ) );
			}
			logLineStr.append( boost::str( boost::format( "Entry Text: %s" ) % logIter->EntryStr ) );

			std::cout << logLineStr << std::endl;
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_scheduler_config_list( std::vector<DBApi::DB_SchedulerConfigRecord>& cfg_list )
{
	std::cout << std::endl;

	if ( cfg_list.size() == 0 )
	{
		std::cout << "No scheduler configuration records found." << std::endl;
	}
	else
	{
		std::cout << "Scheduler configuration records found: " << cfg_list.size() << std::endl << std::endl;

		for ( size_t cfgIdx = 0; cfgIdx != cfg_list.size(); ++cfgIdx )
		{
			print_db_scheduler_config( cfg_list.at( cfgIdx ) );
		}
	}
	std::cout << std::endl;
}

void ScoutX::print_db_scheduler_config( DBApi::DB_SchedulerConfigRecord cfgRec )
{
	std::cout << "Scheduler configuration settings: " << std::endl << std::endl;

	std::string idStr = "";
	std::string dateStr = "";
	std::string workStr = "";
	HawkeyeUUID cfgId = cfgRec.ConfigId;
	HawkeyeUUID ownerId = cfgRec.OwnerUserId;
	system_TP zeroTP = {};

	std::cout << "\tConfigIdNum:\t\t\t" << cfgRec.ConfigIdNum << std::endl;
	idStr.clear();
	idStr = cfgId.to_string();
	std::cout << "\tConfigID:\t\t\t" << idStr << std::endl;
	std::cout << "\tName:\t\t\t\t" << cfgRec.Name << std::endl;
	std::cout << "\tComments:\t\t\t" << cfgRec.Comments << std::endl;
	std::cout << "\tFilenameTemplate:\t\t" << cfgRec.FilenameTemplate << std::endl;
	idStr.clear();
	idStr = ownerId.to_string();
	std::cout << "\tOwnerUserID:\t\t\t" << idStr << std::endl;
	dateStr.clear();
	dateStr = ChronoUtilities::ConvertToString( cfgRec.CreationDate, "%Y-%m-%d %H:%M:%S" );
	std::cout << "\tCreationDate:\t\t\t" << dateStr << std::endl;

	workStr.clear();
	switch ( cfgRec.OutputType )
	{
		case 0:
			workStr = "No Output type";
			break;

		case 1:
			workStr = "Encrypted Output";
			break;

		case 2:
			workStr = "Non-Encrypted Output";
			break;

		default:
			workStr = "Unknown Output type";
			break;
	}
	cout << "\tOutputType:\t\t\t" << workStr << std::endl;

	dateStr.clear();
	dateStr = ChronoUtilities::ConvertToString( cfgRec.StartDate, "%Y-%m-%d" );
	std::cout << "\tStartDate:\t\t\t" << dateStr << std::endl;
	std::cout << "\tStartOffset:\t\t\t" << cfgRec.StartOffset << std::endl;
	std::cout << "\tRepeatInterval:\t\t\t" << cfgRec.MultiRepeatInterval << std::endl;

	workStr.clear();
	GetWeeklyIndicatorString( cfgRec, workStr, DBApi::eDayWeekIndicatorBits::FullIndicatorMask );

	std::cout << "\tWeeklyIndicator:\t\t" << workStr << std::endl;
	std::cout << "\tMonthlyRunDay:\t\t\t" << cfgRec.MonthlyRunDay << std::endl;
	std::cout << "\tDestinationFolder:\t\t" << cfgRec.DestinationFolder << std::endl;
	workStr.clear();
	GetDbDataTypeEnumString( cfgRec.DataType, workStr );
	std::cout << "\tDataType:\t\t\t" << workStr << std::endl;

	ShowDbFilterList( cfgRec.FilterTypesList, cfgRec.CompareOpsList, cfgRec.CompareValsList, cfgRec.LastSuccessRunTime );

	std::cout << "\tEnabled:\t\t\t" << ( ( cfgRec.Enabled == true ) ? "true" : "false" ) << std::endl;
	dateStr.clear();
	if ( cfgRec.LastRunTime == zeroTP )
	{
		dateStr = "(never run)";
	}
	else
	{
		dateStr = ChronoUtilities::ConvertToString( cfgRec.LastRunTime, "%Y-%m-%d %H:%M:%S" );
	}
	std::cout << "\tLastRunTime:\t\t\t" << dateStr << std::endl;
	dateStr.clear();
	if ( cfgRec.LastSuccessRunTime == zeroTP )
	{
		if ( cfgRec.LastRunTime == zeroTP )
		{
			dateStr = "(never run)";
		}
		else
		{
			dateStr = "(no successful runs)";
		}
	}
	else
	{
		dateStr = ChronoUtilities::ConvertToString( cfgRec.LastSuccessRunTime, "%Y-%m-%d %H:%M:%S" );
	}
	std::cout << "\tLastSuccessRunTime:\t\t" << dateStr << std::endl;
	std::cout << "\tNotificationEmail:\t\t" << cfgRec.NotificationEmail << std::endl;
	std::cout << "\tEmailServerAddr:\t\t" << cfgRec.EmailServerAddr << std::endl;
	std::cout << "\tEmailServerPort:\t\t" << cfgRec.EmailServerPort << std::endl;
	std::cout << "\tAuthenticateEmail:\t\t" << ( ( cfgRec.Enabled == true ) ? "true" : "false" ) << std::endl;
	std::cout << "\tAccountUserName:\t\t" << cfgRec.AccountUsername << std::endl;

	std::cout << std::endl;
}

bool ScoutX::readLine(bool& input_quit, bool chk_cmds, bool hide_entry)
{
	if ( !_kbhit() )
	{
		inputTimer_->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError_ );
		inputTimer_->async_wait( std::bind( &ScoutX::handleInput, this, std::placeholders::_1 ) );
		return false;
	}

	int C = _getch();
	if ( isprint( C ) )
	{
		if ( hide_entry )		// doing string entry that may contain some of the special characters; disable handling command characters as special characters.
		{
			chk_cmds = false;
			std::cout << (char) '*';
		}
		else
		{
			std::cout << (char) C;
		}

		if ( chk_cmds )
		{
			int CUPPER = toupper( C );

			if ( CUPPER == cmdQuit )
			{
				Shutdown();

				while ( !IsShutdownComplete() )
				{
					::Sleep( 200 );
				}

				HWND mHwnd = NULL;
				mHwnd = GetConsoleWindow();
				std::cout << endl << "Detected quit request in input" << std::endl;
				PostMessage( mHwnd, WM_CLOSE, 0, 0 );
				input_quit = true;
				return false;

			}
			else if ( C == cmdHelp )
			{
				showHelp();
				smState_ = smCmdEntry;
				smValueLine_.clear();
				input_quit = true;
			}
		}

		if ( !hide_entry && C != cmdHelp )		// NOT doing special string entry AND the entered character is not the question mark character; OK to accept and echo
		{
			smValueLine_ += (char) C;
		}
	}
	else
	{
		if ( C == ESC || C == CTRL_Q )
		{
			smState_ = smCmdEntry;
			smValueLine_.clear();
			std::cout << std::endl;
			input_quit = true;
			prompt();
		}
		else if ( C == CTRL_X )
		{
			Shutdown();

			while ( !IsShutdownComplete() )
			{
				::Sleep( 200 );
			}

			HWND mHwnd = NULL;
			mHwnd = GetConsoleWindow();
			std::cout << endl << "Detected quit request in input" << std::endl;
			PostMessage( mHwnd, WM_CLOSE, 0, 0 );
			input_quit = true;
			return false;
		}
		else if ( C == BACKSPACE )
		{
			if ( smValueLine_.length() > 0 )
			{
				std::cout << BACKSPACE;
				std::cout << (char) ' ';
				std::cout << BACKSPACE;
				smValueLine_.pop_back();
			}
		}
		else if ( C == CR || C == NEWLINE )
		{
			std::cout << std::endl;
			return true;
		}
	}

	inputTimer_->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError_ );
	inputTimer_->async_wait( std::bind( &ScoutX::handleInput, this, std::placeholders::_1 ) );
	return false;
}

//*****************************************************************************
void ScoutX::handleInput (const boost::system::error_code& error) {
	static uint16_t num_containters;
	static std::vector<ReagentContainerUnloadOption> unloadActions;
	HawkeyeError he = HawkeyeError::eSuccess;

	std::string entryStr = "";
	std::string queryResultStr = "";
	std::string promptStr = "";
	std::string lengthWarning = "";

	if (error) {
		if (error == boost::asio::error::operation_aborted) {
			Logger::L().Log (MODULENAME, severity_level::debug2, "handleInput:: boost::asio::error::operation_aborted");
		}
		return;
	}

	if (!_kbhit()) {
		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
		inputTimer_->async_wait (std::bind(&ScoutX::handleInput, this, std::placeholders::_1));
		return;
	}

	bool input_quit = false;
	bool process_cmd = readLine( input_quit );
	if ( !process_cmd )
	{
		return;
	}

PROCESS_STATE:

	entryStr.clear();
	queryResultStr.clear();
	promptStr.clear();
	lengthWarning.clear();

	switch (smState_) {

		//***********************************************************************
		// These are the high level classes of commands.
		//***********************************************************************
		case smCmdEntry:
		{
			boost::to_upper (smValueLine_);
			smCmd_ = (Command_t)smValueLine_.c_str()[0];
			switch (smCmd_) {
				case cmdAdmin:
					smState_ = smAdminOptions;
					prompt();
					break;
				case cmdUser:
					smState_ = smUserOptions;
					prompt();
					break;
				case cmdReagentPack:
					smState_ = smReagentPackOptions;
					prompt();
					break;
				case cmdService:
					smState_ = smServiceOptions;
					prompt();
					break;
				case cmdResults:
					smState_ = smResultsOptions;
					prompt();
					break;
				case cmdAnalysis:
					smState_ = smAnalysisOptions;
					prompt();
					break;
				case cmdQC:
					smState_ = smQCOptions;
					prompt();
					break;
				case cmdCells:
					smState_ = smCellOptions;
					prompt();
					break;
				case cmdWorklist:
					smState_ = smWorklistOptions;
					prompt();
					break;
				case cmdMiscellaneous:
					smState_ = smMiscOptions;
					prompt();
					break;
				case cmdDatabase:
					smState_ = smDatabaseOptions;
					prompt();
					break;

				default:
					std::cout << "Invalid command..." << std::endl;
					break;
			} // End "switch (smCmd_)"

			break;
		}

		//***********************************************************************
		// These are groupings of specific commands for a class of commands.
		//***********************************************************************
		case smAdminOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
					smStateOption_ = smAdminAddUser;
					smState_ = smUsernameEntry;
					break;
				case 2:
					smState_ = smAdminGetCurrentUser;
					goto PROCESS_STATE;
				case 3:
					smStateOption_ = smAdminGetUserCellTypeIndices;
					smState_ = smUsernameEntry;
					break;
				case 4:
					smStateOption_ = smAdminGetUserAnalysisIndices;
					smState_ = smUsernameEntry;
					break;
				case 5:
					smStateOption_ = smAdminGetUserProperty;
					smState_ = smUsernameEntry;
					break;
				case 6:
					smStateOption_ = smAdminChangeUserPassword;
					smState_ = smUsernameEntry;
					break;
				case 7:
					smStateOption_ = smAdminSetUserDisplayName;
					smState_ = smUsernameEntry;
					break;
				case 8:
					smStateOption_ = smAdminSetUserFolder;
					smState_ = smUsernameEntry;
					break;
				case 9:
					smStateOption_ = smAdminSetUserProperty;
					smState_ = smUsernameEntry;
					break;
				case 10:
					smStateOption_ = smAdminSetUserComment;
					smState_ = smUsernameEntry;
					break;
				case 11:
					smStateOption_ = smAdminSetUserEmail;
					smState_ = smUsernameEntry;
					break;
				case 12:
					// Currently, this deletes the CellType for the specified user.
					smStateOption_ = smAdminSetUserCellTypeIndices;
					smState_ = smUsernameEntry;
					break;
				case 13:
					smStateOption_ = smAdminImportConfiguration;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 14:
					smStateOption_ = smAdminExportConfiguration;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 15:
					smStateOption_ = smAdminExportInstrumentData;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 16:
					smStateOption_ = smAdminDeleteSampleRecord;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 17:
					// Update Backup User password; modify the password for the PostgreSQL global DbBackupUser role, not a Vi-CELL user; does not modify anything in the Vi-CELL database
					// This method entry state will test the HawkeyeCore DLL API exposed to the UI
					smStateOption_ = smSetDbBackupUserPwd;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 18:
					Shutdown();
					std::cout << "Hawkeye DLL Shutdown Complete" << std::endl;
					while ( !IsShutdownComplete() )
					{
						std::cout << "Shutting down..." << std::endl;
						Sleep( 500 );
					}
					break;
				default:
					smState_ = smCmdEntry;
					std::cout << "Invalid entry..." << std::endl;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			prompt();
			break;
		}

		case smUserOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
					smStateOption_ = smUserLogin;
					smState_ = smUsernameEntry;
					prompt();
					break;

				//case 2:
				//	break;

				case 3:
					smState_ = smUserLogout;
					break;

				//case 4:
				//	break;

				case 5:
				{
					uint32_t nUsers = 0;
					char** userList;
					he = GetUserList (false, userList, nUsers);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve list of all users: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved list of all users" << std::endl;
						for (uint32_t i = 0; i < nUsers; i++) {
							std::cout << "user[" << i << "]: " << userList[i] << std::endl;
						}
						FreeListOfCharBuffers (userList, nUsers);
					}
					break;
				}

				case 6:
					smStateOption_ = smUserChangeMyPassword;
					parameterStack_.clear();
					smValueLine_.clear();
					smState_ = smPasswordEntry;
					break;

				case 7:
				{
					break;
				}

				case 8:
				{
					break;
				}

				case 9:
				{
					break;
				}

				case 10:
				{
					break;
				}

				case 11:
					smStateOption_ = smUserGetProperty;
					smState_ = smPropertyEntry;
					smValueLine_.clear();
					break;

				case 12:
				{
					uint32_t nProps = 0;
					break;
				}
				case 13:
				{
					smStateOption_ = smUserGetPermissions;
					smState_ = smUsernameEntry;
					smValueLine_.clear();
					break;
				}

				case 14:
				{
					SystemStatusData* systemStatus;
					GetSystemStatus (systemStatus);
					FreeSystemStatus (systemStatus);
					break;
				}

				default:
					smState_ = smCmdEntry;
					std::cout << "Invalid entry..." << std::endl;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			break;
		}

		case smDisplaynameEntry:
		{
			parameterStack_.push_back (smValueLine_);	// get the display name
			smValueLine_.clear();

			switch (smStateOption_) {
				case smAdminAddUser:
				{
					UserPermissionLevel permissions = UserPermissionLevel::eNormal;
					std::string permissionsStr = "NORMAL";
					bool process_line = false;

					promptStr = "Set user permissions? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						do
						{
							entryStr.clear();
							permissionsStr.clear();
							promptStr = "Enter User Permissions as 'Normal', 'Advanced', 'Admin', or 'Service' (blank for default 'Normal'): ";

							GetStringInput( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() == 0 )
							{
								entryStr = "Normal";
							}

							boost::to_upper( entryStr );

							if ( entryStr == "NORMAL" || entryStr == "ADVANCED" || entryStr == "ADMIN" || entryStr == "SERVICE" )
							{
								permissionsStr = entryStr;
								if ( permissionsStr == "NORMAL" )
								{
									permissions = UserPermissionLevel::eNormal;
								}
								else if ( permissionsStr == "ADVANCED" )
								{
									permissions = UserPermissionLevel::eElevated;
								}
								else if ( permissionsStr == "ADMIN" )
								{
									permissions = UserPermissionLevel::eAdministrator;
								}
								else if ( permissionsStr == "SERVICE" )
								{
									permissions = UserPermissionLevel::eService;
								}
							}
							else
							{
								std::cout << "Response not recognized!" << std::endl;
							}
							std::cout << std::endl;
						} while ( permissionsStr.length() == 0 );
					}

					he = AddUser (parameterStack_[0].c_str(), parameterStack_[2].c_str(), parameterStack_[1].c_str(), permissions);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new user: " << GetErrorAsStr(he) << "  using permissions: " << permissionsStr << std::endl;
					} else {
						std::cout << "Successfully added \"" << parameterStack_[0].c_str() << "\"..." << std::endl;
					}
					smState_ = smCmdEntry;
					break;
				}

				case smAdminSetUserDisplayName:
				{
					he = ChangeUserDisplayName(parameterStack_[0].c_str(), parameterStack_[1].c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set the user's display name: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Current user: " << parameterStack_[0] << std::endl;
						std::cout << "Display name: " << parameterStack_[1] << std::endl;
					}
					smState_ = smCmdEntry;
					break;
				}
			}

			prompt();
			break;
		}

		case smAdminSetUserComment:
		{
			parameterStack_.push_back( smValueLine_ );	// get the user comment

			he = SetUserComment( parameterStack_[0].c_str(), (parameterStack_[1].length() == 0) ? nullptr : parameterStack_[1].c_str() );
			if ( he != HawkeyeError::eSuccess )
			{
				std::cout << "Failed to set the user's comment: " << GetErrorAsStr( he ) << std::endl;
			}
			else
			{
				std::cout << "Current user: " << parameterStack_[0] << std::endl;
				std::cout << "Comment: " << parameterStack_[1] << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smAdminSetUserEmail:
		{
			parameterStack_.push_back( smValueLine_ );	// get the user email

			he = SetUserComment( parameterStack_[0].c_str(), ( parameterStack_[1].length() == 0 ) ? nullptr : parameterStack_[1].c_str() );
			if ( he != HawkeyeError::eSuccess )
			{
				std::cout << "Failed to set the user's email: " << GetErrorAsStr( he ) << std::endl;
			}
			else
			{
				std::cout << "Current user: " << parameterStack_[0] << std::endl;
				std::cout << "Email: " << parameterStack_[1] << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smUsernameEntry:
		{
			parameterStack_.push_back (smValueLine_);
			smValueLine_.clear();

			switch (smStateOption_) {
				case smUserLogin:
					smState_ = smPasswordEntry;
					break;
				case smAdminAddUser:
					smState_ = smPasswordEntry;
					break;
				case smAdminGetUserProperty:
					smState_ = smPropertyEntry;
					break;
				case smAdminChangeUserPassword:
					smState_ = smPasswordEntry;
					break;
				case smAdminSetUserDisplayName:
					smState_ = smDisplaynameEntry;
					break;
				case smAdminSetUserFolder:
					smState_ = smFoldernameEntry;
					break;
				case smAdminSetUserProperty:
					smState_ = smPropertyEntry;
					break;
				case smAdminSetUserComment:
					smState_ = smStateOption_;
					break;
				case smAdminSetUserEmail:
					smState_ = smStateOption_;
					break;
				default:
					smState_ = smStateOption_;
					goto PROCESS_STATE;
			}
			prompt();
			break;
		}

		case smPasswordEntry:
		{
			parameterStack_.push_back (smValueLine_);
			smValueLine_.clear();

			switch (smStateOption_) {
				case smUserLogin:
				{
					he = doUserLogin(parameterStack_[0].c_str(), parameterStack_[1].c_str());
					smState_ = smCmdEntry;
					break;
				}

				case smUserChangeMyPassword:
					he = ChangeMyPassword (currentPassword_.c_str(), parameterStack_[0].c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to change password: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully changed password..." << std::endl;
					}
					smState_ = smCmdEntry;
					break;

				case smAdminChangeUserPassword:
					smState_ = smStateOption_;
					goto PROCESS_STATE;

				case smAdminAddUser:
					smState_ = smDisplaynameEntry;
					prompt();
					break;

			} // End "switch (smUserOption_)"

			break;
		}

		case smFoldernameEntry:
			parameterStack_.push_back (smValueLine_);
			smValueLine_.clear();

			switch (smStateOption_) {
				case smAdminSetUserFolder:
					smState_ = smStateOption_;
					goto PROCESS_STATE;
			}
			break;

		case smPropertyEntry:
			parameterStack_.push_back (smValueLine_);
			smValueLine_.clear();

			switch (smStateOption_) {
				case smAdminSetUserProperty:
					smState_ = smPropertyValueEntry;
					prompt();
					break;
				case smUserGetProperty:
				case smAdminGetUserProperty:
					smState_ = smStateOption_;
					goto PROCESS_STATE;
			}
			break;

		case smPropertyValueEntry:
			parameterStack_.push_back (smValueLine_);
			smValueLine_.clear();

			smState_ = smCmdEntry;
			break;

		case smSampleSetToCancelEntry:
		{
			switch (smStateOption_)
			{
				case smCancelSampleSet:
				{
					uint16_t sampleSetIndexToCancel = static_cast<uint16_t>(atoi (smValueLine_.c_str()));
					he = CancelSampleSet (sampleSetIndexToCancel);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set cancel SampleSet: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully cancelled SampleSet..." << std::endl;
					}
					break;
				}
			}

			smState_ = smCmdEntry;
			break;
		}

		case smUserLogout:
		{
			LogoutUser();
			smState_ = smCmdEntry;
			break;
		}

		case smUserGetProperty:
		{
			smState_ = smCmdEntry;
			break;
		}
		case smUserGetPermissions:
		{
			UserPermissionLevel permission;
			he = GetUserPermissionLevel(parameterStack_[0].c_str(), permission);
			if (he != HawkeyeError::eSuccess)
			{
				std::cout << "Failed to retrieve \"" << parameterStack_[0] << "\"" << " permission level: " << GetErrorAsStr(he) << std::endl;
			}
			else
			{
				std::cout << "Successfully retrieved \"" << parameterStack_[0] << "\"" << " permission level" << std::endl;
				std::cout << "Permissions: " << GetPermissionLevelAsStr(permission) << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smAdminGetUserCellTypeIndices:
		{
			smState_ = smCmdEntry;
			break;
		}

		case smAdminSetUserCellTypeIndices:
		{
			uint32_t nCells = 1;
			uint32_t user_cell_indices[1];
			user_cell_indices[0] = 0x00000000;

			he = SetUserCellTypeIndices (parameterStack_[0].c_str(), nCells, user_cell_indices);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to set celltype indices for \"" << parameterStack_[0] << "\": " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully set celltype indices for \"" << parameterStack_[0] << "\"" << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smAdminGetUserAnalysisIndices:
		{
			uint32_t nAnalyses = 0;
			NativeDataType tag;
			uint16_t* analysis_indices;

			he = GetUserAnalysisIndices (parameterStack_[0].c_str(), nAnalyses, tag, analysis_indices);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to retrieve analyses for \"" << parameterStack_[0] << "\": " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully retrieved analyses for \"" << parameterStack_[0] << "\"" << std::endl;
				FreeTaggedBuffer(tag, &analysis_indices);
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminGetUserProperty:
		{
			smState_ = smCmdEntry;
			break;
		}

		case smAdminGetCurrentUser:
		{
			char* userName;
			UserPermissionLevel permissions;

			he = GetCurrentUser (userName, permissions);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to get the current user: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Current user: " << userName << std::endl;
				std::cout << "Permissions: " << GetPermissionLevelAsStr(permissions) << std::endl;
				FreeCharBuffer (userName);
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminChangeUserPassword:
			he = ChangeUserPassword( parameterStack_[0].c_str(), parameterStack_[1].c_str() );
			if ( he != HawkeyeError::eSuccess )
			{
				std::cout << "Failed to change password: " << GetErrorAsStr( he ) << std::endl;
			}
			else
			{
				std::cout << "Successfully changed password..." << std::endl;
			}
			smState_ = smCmdEntry;
			break;

		case smAdminSetUserFolder:
		{
			he = SetUserFolder (parameterStack_[0].c_str(), parameterStack_[1].c_str());
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to set the user's folder: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Current user: " << parameterStack_[0] << std::endl;
				std::cout << "Folder: " << parameterStack_[1] << std::endl;
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminImportConfiguration:
		{
			char filename[] = "\\Instrument\\Export\\InstrumentConfiguration.cfg";
			he = ImportInstrumentConfiguration (currentUser_.c_str(), currentPassword_.c_str(), filename);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to import configuration: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully imported configuration" << std::endl;
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminExportConfiguration:
		{
			char filename[] = "\\Instrument\\Export\\InstrumentConfiguration.cfg";
			he = ExportInstrumentConfiguration (currentUser_.c_str(), currentPassword_.c_str(), filename);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to export configuration: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully exported configuration" << std::endl;
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminExportInstrumentData:
		{
			//char* exportFilename;
			//he = ExportInstrumentData ("\\Instrument\\Export", exportFilename, true);
			//if (he != HawkeyeError::eSuccess) {
			//	std::cout << "Failed to export configuration: " << GetErrorAsStr(he) << std::endl;
			//} else {
			//	std::cout << "Successfully exported data to " << exportFilename << std::endl;
			//}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminDeleteSampleRecord:
		{
			//uuid__t wqi_uuidlist;	//TODO: put a specific UUID here...
			//uint32_t num_uuid = 1;
			//bool retain_results_and_first_image = false;

			//he = DeleteSampleRecord (&wqi_uuidlist, num_uuid, retain_results_and_first_image);
			//if (he != HawkeyeError::eSuccess) {
			//	std::cout << "Failed to delete SampleRecord: " << GetErrorAsStr(he) << std::endl;
			//} else {
			//	std::cout << "Successfully deleted SampleRecord" << std::endl;
			//}

			smState_ = smCmdEntry;
			break;
		}		

		case smReagentPackOptions:
		{
			int val = atoi (smValueLine_.c_str());
			switch (val) {
				case 1:
					he = LoadReagentPack (cb_ReagentPackLoadStatus, cb_ReagentPackLoadComplete);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Reagent pack load command failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Reagent pack load in progress..." << std::endl;
					}
					break;

				case 2:
				{
					smState_ = smReagentPackUnload_Option_0;
					prompt();
					break;
				}

				case 3:
				{
					uint32_t num_reagents;
					ReagentDefinition* reagents;

					he = GetReagentDefinitions (num_reagents, reagents);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Getting reagent definitions failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << boost::str (boost::format ("Successfully retrieved %d reagent definitions...") % num_reagents) << std::endl;
						print_reagents_definitions(reagents, size_t(num_reagents));
						FreeReagentDefinitions (reagents, num_reagents);
					}
					break;
				}

				case 4:
				{
					uint16_t num_reagents = 1;
					ReagentContainerState* reagentContainerState;
					he = GetReagentContainerStatus(num_reagents, reagentContainerState);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Getting reagent definitions failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully retrieved the reagent definition..." << std::endl;
						print_reagents_containerstates(reagentContainerState);
						FreeListOfReagentContainerState(reagentContainerState, num_reagents);
					}
					break;
				}

				case 5:
				{
					uint16_t num_reagents;
					ReagentContainerState* reagentContainerState;

					he = GetReagentContainerStatusAll (num_reagents, reagentContainerState);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Getting reagent definitions failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved the reagent definitions..." << std::endl;
						print_reagents_containerstates(reagentContainerState);
						FreeListOfReagentContainerState (reagentContainerState, num_reagents);
					}
					break;
				}

				case 6:
				{
					he = StartFlushFlowCell(cb_ReagentFlowCellFlush);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Reagent start flow cell flush command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Reagent flow cell flush progress..." << std::endl;
					}
					break;
				}

				case 7:
				{
					he = CancelFlushFlowCell();
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Reagent canceling of flow cell flush command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Reagent canceling flow cell flush in progress..." << std::endl;
					}
					break;
				}


				case 8:
				{
					he = StartDecontaminateFlowCell(cb_ReagentDecontaminateFlowCell);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Reagent start flow cell decontaminate command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Reagent flow cell decontaminate progress..." << std::endl;
					}
					break;
				}

				case 9:
				{
					he = CancelDecontaminateFlowCell();
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Reagent canceling of flow cell decontaminate command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Reagent canceling flow cell decontaminate in progress..." << std::endl;
					}
					break;
				}

				case 10:
				{
					smState_ = smReagentPackDrain_Option;
					prompt();
					break;
				}

				case 11:
				{
					he = CancelDrainReagentPack();
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Reagent Cancel reagent pack drain command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Reagent Canceling of draining of reagent pack in progress..." << std::endl;
					}
					break;
				}

				case 12:
				{
					he = StartCleanFluidics(cb_CleanFluidics);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "StartCleanFluidics command failed: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "StartCleanFluidics in started..." << std::endl;
					}
					break;
				}
				
				default:
					std::cout << "Invalid entry..." << std::endl;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			if (val != 10 && val != 2) smState_ = smCmdEntry; //cmd 10 & 2 option need input from the user
			break;
		}

		case smReagentPackDrain_Option:
		{
			uint8_t container = static_cast<uint8_t>(atoi(smValueLine_.c_str()));
			if (container < 1 || (5 < container))
			{
				std::cout << "Invalid entry..." << std::endl;
				smState_ = smCmdEntry;
				break;
			}
			he = StartDrainReagentPack(cb_ReagentDrainStatus, container);
			if (he != HawkeyeError::eSuccess)
			{
				std::cout << "Reagent start of draining of reagent pack command failed: " << GetErrorAsStr(he) << std::endl;
			}
			else
			{
				std::cout << "Reagent draining of pack in progress..." << std::endl;
			}
			break;
		}

		case smReagentPackUnload_Option_0:
		{
			num_containters = static_cast<uint16_t>(atoi(smValueLine_.c_str()));
			unloadActions.resize(num_containters);
			smState_ = smReagentPackUnload_Option_1;

			if (num_containters == 0) goto unload_opt1;
			std::cout << "Action for Container #1" << endl;
			smState_ = smReagentPackUnload_Option_1;
			prompt();

			break;
		}

		case smReagentPackUnload_Option_1:
		{
		unload_opt1:
			static uint16_t curr_containter = 0;

			if (curr_containter < num_containters)
			{
				unloadActions[curr_containter].container_action = ReagentUnloadOption(atoi(smValueLine_.c_str()));
				if (unloadActions[curr_containter].container_action < 0 || (2 < unloadActions[curr_containter].container_action))
				{
					std::cout << "Invalid entry..." << std::endl;
					smState_ = smCmdEntry;
					break;
				}
				curr_containter++;
			}
			if (curr_containter < num_containters)
			{
				std::cout << "Action for Container #" << curr_containter + 1 << endl;
				smState_ = smReagentPackUnload_Option_1;
				prompt();
			}
			else
			{
				curr_containter = 0;
				for (auto action : unloadActions)
				{
					std::fill(&action.container_id[0], &action.container_id[8], 0);
				}
				he = UnloadReagentPack(unloadActions.data(), num_containters, cb_ReagentPackUnloadStatus, cb_ReagentPackUnloadComplete);
				if (he != HawkeyeError::eSuccess)
				{
					std::cout << "Reagent pack unload command failed: " << GetErrorAsStr(he) << std::endl;
				}
				else
				{
					std::cout << "Reagent pack unload in progress..." << std::endl;
				}
				smState_ = smCmdEntry;
			}
			break;
		}

		case smServiceOptions:
		{
			LogoutUser();
			LoginUser ( ServiceUser.c_str(), ServicePwd.c_str() );

			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					bool lampState;
					float intensity_0_to_100;
					he = svc_GetCameraLampState (lampState, intensity_0_to_100);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get Camera Lamp State" << std::endl;
					} else {
						std::cout << boost::str (boost::format ("Lamp state: %s, power: %f") % (lampState ? "true" : "false") % intensity_0_to_100) << std::endl;
					}
					break;
				}
				case 2:
				{
					bool lampState = false;
					float intensity_0_to_25 = 25;
					he = svc_SetCameraLampState (lampState, intensity_0_to_25);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set Camera Lamp State" << std::endl;
					} else {
						std::cout << boost::str (boost::format ("Lamp state: %s, power: %f") % (lampState ? "true" : "false") % intensity_0_to_25) << std::endl;
					}
					break;
				}
				case 3:
				{
					he = svc_MoveProbe (true);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Failed to move sample probe" << std::endl;
					}
					else
					{
						std::cout << "Sample Probe UP" << std::endl;
					}
					break;
				}
				case 4:
				{
					he = svc_MoveProbe (false);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Failed to move sample probe" << std::endl;
					}
					else
					{
						std::cout << "Sample Probe DOWN" << std::endl;
					}
					break;
				}
				case 5:
				{
					he = svc_MoveReagentArm (true);
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Failed to move reagent arm" << std::endl;
					}
					else
					{
						std::cout << "Reagent Arm UP" << std::endl;
					}
					break;
				}
				case 6:
				{
					he = svc_MoveReagentArm (false);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to move reagent arm" << std::endl;
					} else {
						std::cout << "Reagent Arm DOWN" << std::endl;
					}
					break;
				}
				case 7: 
				{
					he = svc_SetSystemSerialNumber ("VAL-1", "916348");
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set serial number" << std::endl;
					}
					else {
						std::cout << "Successfully set serial number" << std::endl;
					}
					break;
				}
				case 8:
				{
					SamplePosition pos = {'Z', 5};
					he = svc_SetSampleWellPosition (pos);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set sample position" << std::endl;
					}
					else {
						std::cout << "Successfully set sample position" << std::endl;
					}
					break;
				}
				case 9:		// standard 'ejcct' operation (local instrument)
				{
					he = EjectSampleStage( currentUser_.c_str(), currentPassword_.c_str() );
					if ( he != HawkeyeError::eSuccess )
					{
						std::cout << "Failed to eject sample stage" << std::endl;
					}
					else
					{
						std::cout << "Successfully ejected sample stage" << std::endl;
					}
					break;
				}
				case 10:		// automation 'eject' operation with position designation; requires manual removal of the carousel
				{
					int32_t angle = 0;

					entryStr.clear();
					promptStr = boost::str( boost::format( "Enter eject rotation angle: %d\b" ) % angle );

					GetStringInput( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 )
					{
						angle = stoi(entryStr);
					}

					he = SampleStageLoadUnload( currentUser_.c_str(), currentPassword_.c_str(), angle );
					if ( he != HawkeyeError::eSuccess )
					{
						std::cout << "Failed to eject sample stage" << std::endl;
					}
					else
					{
						std::cout << "Successfully ejected sample stage" << std::endl;
					}
					break;
				}
			}

			smState_ = smCmdEntry;
			break;
		}

		case smResultsOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					std::cout << "This option is not supported..." << std::endl;
					break;
				}

				case 2:
				{
					std::cout << "This option is not supported..." << std::endl;
					break;
				}

				case 3:
				{
					std::cout << "This option is not supported..." << std::endl;
					break;
				}

				// Sample Records.
				case 4:
				{
					//SampleRecord* reclist;
					//he = RetrieveSampleRecord (uuidForRetrieval.get(), reclist);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
					//}
					//else {
					//	std::cout << "Successfully retrieved WorkQueue..." << std::endl;
					//	FreeListOfSampleRecord (reclist, 1);
					//}
					std::cout << "RetrieveSampleRecord is not supported..." << std::endl;
					break;
				}

				case 5:
				{
					//SampleRecord* reclist;
					//uint32_t list_size;
					//he = RetrieveSampleRecords (0, 0, "bci_admin", reclist, list_size);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve Samples: " << GetErrorAsStr(he) << std::endl;
					//} else {
					//	std::cout << "Successfully retrieved Samples..." << std::endl;
					//	FreeListOfSampleRecord (reclist, list_size);
					//}
					std::cout << "RetrieveSampleRecords is not supported..." << std::endl;
					break;
				}

				case 6:
				{
					//SampleRecord* reclist;
					//uint32_t list_size;
					//he = RetrieveSampleRecordList (&uuidForRetrieval.get(), 1, reclist, list_size);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
					//} else {
					//	uuidForRetrieval = reclist[0].uuid;
					//	std::cout << "Successfully retrieved WorkQueue..." << std::endl;
					//	FreeListOfSampleRecord (reclist, list_size);
					//}
					std::cout << "RetrieveSampleRecordList is not supported..." << std::endl;
					break;
				}

				// Image Set Records.
				case 7:
				{
					uuid__t uuid;	// Pick UUID from the ViCellData.ImageSequences DB table.
					HawkeyeUUID::Getuuid__tFromStr ("ff18ab40414149ac92f2b5a94658a6ff", uuid);
					SampleImageSetRecord* reclist;

					he = RetrieveSampleImageSetRecord (uuid, reclist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, 1);
					}
					break;
				}

				case 8:
				{
					SampleImageSetRecord* reclist;
					uint32_t list_size;
					he = RetrieveSampleImageSetRecords (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, list_size);
					}
					break;
				}

				case 9:
				{
					SampleImageSetRecord* reclist;
					uint32_t list_size;
					uuid__t uuids[3];
					uint32_t uuidListSize = 3;
					
					// Pick several UUIDs from the ViCellData.ImageSequences DB table.
					HawkeyeUUID::Getuuid__tFromStr ("fcfbe07d71084a1db0579743f4987119", uuids[0]);
					HawkeyeUUID::Getuuid__tFromStr ("f346a5022f354ecd8d1b833ae6a567bd", uuids[1]);
					HawkeyeUUID::Getuuid__tFromStr ("ee5ac49637df4b0183c5b10e512be4a7", uuids[2]);

					he = RetrieveSampleImageSetRecordList (uuids, uuidListSize, reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, list_size);
					}
					break;
				}

				// Image Records.
				case 10:
				{
					uuid__t uuid;	// Pick one from DB...
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", uuid);
					ImageWrapper_t* imageWrapper;

					he = RetrieveImage (uuid, imageWrapper);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve image: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved image..." << std::endl;
						//TODO: list out image properties...  write image to unencrypted file.
						FreeImageWrapper (imageWrapper, 1);
					}
					break;
				}

				case 11:
				{
					uuid__t uuid;	// Pick one from DB...
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", uuid);
					ImageWrapper_t* imageWrapper;

					he = RetrieveBWImage (uuid, imageWrapper);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve BW image: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved BW image..." << std::endl;
						FreeImageWrapper (imageWrapper, 1);
					}
					break;
				}

				case 12:
				{
					uuid__t result_uuid;
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", result_uuid);
					uuid__t image_uuid;	// Pick one from DB that goes with the result UUID...
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", image_uuid);
					ImageWrapper_t* imageWrapper;

					//he = RetrieveAnnotatedImage (result_uuid, image_uuid, imageWrapper);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve retrieved annotated image: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved annotated image..." << std::endl;
						FreeImageWrapper (imageWrapper, 1);
					}
					break;
				}

				case 13:
				{
					uuid__t uuid;
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", uuid);
					ResultRecord* rec;

		//			//todo: where does this uuid come from???
					he = RetrieveResultRecord (uuid, rec);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "failed to retrieve result record: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "successfully retrieved result record..." << std::endl;
						FreeListOfResultRecord (rec, 1);
					}
					break;
				}

				case 14:
				{
					uuid__t uuid;
//TODO: this UUID is not correct...
					HawkeyeUUID::Getuuid__tFromStr ("fa02393f0c9f4e98923d79d12ba858d1", uuid);
					ResultRecord* rec_list;
					uint32_t list_size;

					he = RetrieveResultRecords (0, 0, "bci_admin", rec_list, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "failed to retrieve result record: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "successfully retrieved result record..." << std::endl;
						FreeListOfResultRecord (rec_list, 1);
					}
					break;
				}

				case 15:
				{
					ResultRecord* recs;
					uint32_t retrieved_size;
					uuid__t uuids[3];
					uint32_t uuidListSize = 3;

////TODO: these UUIDs are not correct...
					// Pick several UUIDs from the ViCellData.ImageSequences DB table.
					HawkeyeUUID::Getuuid__tFromStr ("fcfbe07d71084a1db0579743f4987119", uuids[0]);
					HawkeyeUUID::Getuuid__tFromStr ("f346a5022f354ecd8d1b833ae6a567bd", uuids[1]);
					HawkeyeUUID::Getuuid__tFromStr ("ee5ac49637df4b0183c5b10e512be4a7", uuids[2]);

					he = RetrieveResultRecordList (uuids, uuidListSize, recs, retrieved_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Image Record List: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Image Record List..." << std::endl;
						FreeListOfResultRecord (recs, retrieved_size);
					}
					break;
				}

				case 16:
				{
					uuid__t uuid;
					ResultSummary* rec;

					// Pick a UUID from the ViCellData.SummaryResults DB table.
					HawkeyeUUID::Getuuid__tFromStr ("fb3902d6739246feac2fad8de2b560cc", uuid);

					he = RetrieveResultSummaryRecord (uuid, rec);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Result Summary record: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Result Summary record..." << std::endl;
						FreeListOfResultSummary (rec, 1);
					}
					break;
				}

				case 17:
				{
					ResultSummary* recs;
					uint32_t retrieved_size;
					uuid__t uuids[3];
					uint32_t uuidListSize = 3;

					// Pick several UUIDs from the ViCellData.ImageSequences DB table.
					HawkeyeUUID::Getuuid__tFromStr ("f7e89d9a1d294b61a257d6feb6334a80", uuids[0]);
					HawkeyeUUID::Getuuid__tFromStr ("f62ee047d70b499aa5dd8f46bb0a16a2", uuids[1]);
					HawkeyeUUID::Getuuid__tFromStr ("fb3902d6739246feac2fad8de2b560cc", uuids[2]);

					he = RetrieveResultSummaryRecordList (uuids, uuidListSize, recs, retrieved_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Result Summary List: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved Result Summary List..." << std::endl;
						FreeListOfResultSummary (recs, retrieved_size);
					}
					break;
				}


				case 18:
				{
					//		//HawkeyeError HawkeyeLogicImpl::SignResultRecord (DataSignature_t*& signatures, uint16_t& num_signatures)
					std::cout << "SignResultRecord not currently supported..." << std::endl;
					break;
				}

				// RetrieveSignatureDefinitions

				case 19:
				{
					uint16_t num_signatures;
					DataSignature_t* signatures;
					he = RetrieveSignatureDefinitions (signatures, num_signatures);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve signature defintions: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved signature defintions..." << std::endl;

						for (uint16_t i = 0; i < num_signatures; i++)
						{
							std::cout << signatures[i].short_text << " : " << signatures[i].long_text << std::endl;
						}

						FreeDataSignature (signatures, num_signatures);
					}
					break;
				}

				case 20:
				{
					char* signatureToRemove = "REV";
					he = RemoveSignatureDefinition (signatureToRemove, static_cast<uint16_t>(strlen(signatureToRemove)));
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to remove signature defintion: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully removed signature defintion..." << std::endl;
					}
					break;
				}

		//		case 19:
		//		{
		//			if (!uuidForRetrieval.is_initialized()) {
		//				std::cout << "No UUID specified, call RetrieveResultRecords first: " << GetErrorAsStr(he) << std::endl;

		//			} else {
		//				DetailedResultMeasurements* detailedResultMeasurements;

		//				he = RetrieveDetailedMeasurementsForResultRecord (uuidForRetrieval.get(), detailedResultMeasurements);
		//				if (he != HawkeyeError::eSuccess) {
		//					std::cout << "Failed to retrieve Detailed Measurements for Result Record: " << GetErrorAsStr(he) << std::endl;
		//				} else {
		//					std::cout << "Successfully retrieved Detailed Measurements for Result Record..." << std::endl;
		//					FreeDetailedResultMeasurement (detailedResultMeasurements);
		//				}
		//			}
		//		}

		//		case 20:
		//		{
		//			if (!uuidForRetrieval.is_initialized()) {
		//				std::cout << "No UUID specified, call RetrieveResultRecords first: " << GetErrorAsStr(he) << std::endl;

		//			} else {
		//				uint8_t bin_count = 8;
		//				histogrambin_t* bins;
		//				Hawkeye::Characteristic_t measurement = {21, 0, 0};	// Avg Spot Brightness

		//				he = RetrieveHistogramForResultRecord (uuidForRetrieval.get(), false, measurement, bin_count, bins);
		//				if (he != HawkeyeError::eSuccess) {
		//					std::cout << "Failed to retrieve Histogram for Result Record: " << GetErrorAsStr(he) << std::endl;
		//				} else {
		//					std::cout << "Successfully retrieved Histogram for Result Record..." << std::endl;
		//					for (uint8_t i = 0; i < bin_count; i++)
		//					{
		//						std::cout << "bin nominal value " << bins[i].bin_nominal_value << std::endl << "Count " << bins[i].count << std::endl;
		//					}
		//					FreeHistogramBins (bins);
		//				}
		//			}
				//}

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

		case smAnalysisOptions:
		{
			uint32_t num_ad = 0;
			AnalysisDefinition* analysesList = nullptr;

			switch (atoi (smValueLine_.c_str())) {
				case 1:
		//		{
		//			he = GetFactoryAnalysisDefinitions (num_ad, analysesList);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve factory analyses: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved factory analyses..." << std::endl;
		//				FreeAnalysisDefinitions (analysesList, num_ad);
		//			}
		//			break;
		//		}

				case 2:
		//		{
		//			he = GetUserAnalysisDefinitions (num_ad, analysesList);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve user analyses: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved user analyses..." << std::endl;
		//				FreeAnalysisDefinitions (analysesList, num_ad);
		//			}
					break;
		//		}

				case 3:
		//		{
		//			he = GetAllAnalysisDefinitions (num_ad, analysesList);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve all analyses: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved all analyses..." << std::endl;
		//				FreeAnalysisDefinitions (analysesList, num_ad);
		//			}
					break;
		//		}

				case 4:
		//		{
		//			uint16_t newAnalysisIndex = 0;
		//			AnalysisDefinition* analysis;
		//			buildAnalysisDefinition (analysis);
		//			he = AddAnalysisDefinition (*analysis, newAnalysisIndex);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to add new analysis type: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully added analysis... new index:" << std::to_string(newAnalysisIndex) << std::endl;
		//				FreeAnalysisDefinitions (analysesList, num_ad);
		//			}
		//			free (analysis);
		//			break;
		//		}

				case 5:
		//		{
		//			std::cout << "Invalid entry..." << std::endl;
		//			he = HawkeyeError::eEntryInvalid;
		//			break;
		//		}

				case 6:
		//		{
		//			//TODO: should there be a *RemoveAnalysisDefinition*?
		//			//he = RemoveAnalysisDefinition (0x80000000);
		//			//if (he != HawkeyeError::eSuccess) {
		//			//	std::cout << "Failed to retrieve remove cell type: " << GetErrorAsStr(he) << std::endl;
		//			//}
		//			//else {
		//			//	std::cout << "Successfully removed cell type..." << std::endl;
		//			//}
		//			std::cout << "Invalid entry..." << std::endl;
		//			he = HawkeyeError::eEntryInvalid;
		//			break;
		//		}

				case 7:
		//		{
		//			uint32_t nparameters;
		//			char** parameters;
		//			he = GetSupportedAnalysisParameterNames (nparameters, parameters);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve Analysis parameter names: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved Analysis parameter names..." << std::endl;
		//				for (uint32_t i = 0; i < nparameters; i++) {
		//					std::cout << "parameter[" << i << "]: " << parameters[i] << std::endl;
		//				}
		//				FreeListOfCharBuffers (parameters, nparameters);
		//			}
		//			break;
		//		}

				case 8:
		//		{
		//			uint32_t ncharacteristics;
		//			Hawkeye::Characteristic_t* characteristics;
		//			he = GetSupportedAnalysisCharacteristics (ncharacteristics, characteristics);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve Analysis characteristics: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved Analysis characteristics..." << std::endl;
		//				for (uint32_t i = 0; i < ncharacteristics; i++) {
		//					std::cout << "characteristic[" << i << "]: " << characteristics[i].key << ":" << characteristics[i].s_key << ":" << characteristics[i].s_s_key << std::endl;
		//				}
		//			}
		//			break;
		//		}

				case 9:
		//		{
		//			Hawkeye::Characteristic_t hc;
		//			hc.key = 20;
		//			hc.s_key = 0;
		//			hc.s_s_key = 0;
		//			char* name = GetNameForCharacteristic (hc);
		//			if (!name) {
		//				std::cout << "Failed to retrieve name for characteristic: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved name for characteristic: " << name << std::endl;
		//				if (name) {
		//					free (name);
		//					name = nullptr;
		//				}
		//			}

		//			hc.key = 42;
		//			hc.s_key = 0;
		//			hc.s_s_key = 0;
		//			name = GetNameForCharacteristic (hc);
		//			if (!name) {
		//				std::cout << "Failed to retrieve name for characteristic" << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved name for characteristic: " << name << std::endl;
		//				if (name) {
		//					free (name);
		//					name = nullptr;
		//				}
		//			}
		//			break;
		//		}

				case 10:
		//		{
		//			uint32_t num_ad = 0;
		//			AnalysisDefinition* analysesList = nullptr;

		//			he = GetFactoryAnalysisDefinitions (num_ad, analysesList);
		//			if (he != HawkeyeError::eSuccess) {
		//				std::cout << "Failed to retrieve factory analyses: " << GetErrorAsStr(he) << std::endl;
		//			} else {
		//				std::cout << "Successfully retrieved factory analyses..." << std::endl;
		//			}

		//			if (num_ad) {
		//				analysesList[0].label[0] = 'Z';
		//				analysesList[0].mixing_cycles = 5;

		//				he = ModifyBaseAnalysisDefinition (analysesList[0], false);
		//				if (he != HawkeyeError::eSuccess) {
		//					std::cout << "Failed to modify factory analysis[0]: " << GetErrorAsStr(he) << std::endl;
		//				} else {
		//					std::cout << "Successfully modified factory analysis[0]..." << std::endl;
		//				}
		//				FreeAnalysisDefinitions (analysesList, num_ad);
		//			}
		//			break;
		//		}
					smState_ = smCmdEntry;
					std::cout << std::endl;
					std::cout << "Not active." << std::endl;
					std::cout << std::endl;
					break;

				default:
					std::cout << "Invalid entry..." << std::endl;
					he = HawkeyeError::eEntryInvalid;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

		case smCellOptions:
		{
			uint32_t num_ct = 0;
			CellType* cellTypeList = nullptr;

			switch (atoi (smValueLine_.c_str())) {
				case 1:
					he = GetFactoryCellTypes (num_ct, cellTypeList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve factory cell types: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved factory cell types..." << std::endl;
					}
					break;
				case 2:
					he = GetAllowedCellTypes (SILENTADMIN_USER, num_ct, cellTypeList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve user defined cell types: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved user defined cell types..." << std::endl;
					}
					break;
				case 3:
					he = GetAllCellTypes (num_ct, cellTypeList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve all cell types: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved all cell types..." << std::endl;
					}
					break;
				case 4:
				{
					uint32_t newCellTypeIndex = 0;
					CellType* cellType;
					buildNewCellType (cellType);
					he = AddCellType (currentUser_.c_str(), currentPassword_.c_str(), *cellType, newCellTypeIndex, "");
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new cell type: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully added cell type... new index:" << std::to_string(newCellTypeIndex) << std::endl;
					}
					free (cellType);
					break;
				}
				case 5:
				{
					// Save for possible future use...
					//CellType* cellType;
					//buildModifiedCellType (cellType);
					//he = ModifyCellType (currentUser_.c_str(), currentPassword_.c_str(), *cellType);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to add modify cell type: " << GetErrorAsStr(he) << std::endl;
					//} else {
					//	std::cout << "Successfully modified cell type.." << std::endl;
					//}
					//free (cellType);

					std::cout << "ModifyCellType is deprecated and should not be used." << std::endl;
					break;
				}
				case 6:
				{
					// Remove the cell type added with AddCellType.
					he = RemoveCellType (currentUser_.c_str(), currentPassword_.c_str(), 0x80000000);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve remove cell type: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully removed cell type..." << std::endl;
					}
					break;
				}
				default:
					std::cout << "Invalid entry..." << std::endl;
					he = HawkeyeError::eEntryInvalid;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			if (he == HawkeyeError::eSuccess && cellTypeList) {
				FreeListOfCellType (cellTypeList, num_ct);
			}

			smState_ = smCmdEntry;
			break;
		}

		case smQCOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					QualityControl_t qualityControl = {
						"QC #1",
						5,
						ap_PopulationPercentage,
						"Lot #42",
						2.0,
						10.0,
						// GMT: Thursday, June 14, 2018 7:37:53 PM
						// Your time zone : Thursday, June 14, 2018 1 : 37 : 53 PM GMT - 06 : 00 DST
						1529005073 / (60 * 60 * 24),
						"comment text"
					};

					he = AddQualityControl (currentUser_.c_str(), currentPassword_.c_str(), qualityControl, "");
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new QualityControl: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully added QualityControl" << std::endl;
					}
					break;
				}
				case 2:
				{
					QualityControl_t* qualitycontrols;
					uint32_t num_qcs;

					he = GetQualityControlList (currentUser_.c_str(), currentPassword_.c_str(), false, qualitycontrols, num_qcs);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve QualityControl list: " << GetErrorAsStr(he) << std::endl;
					} else {
						FreeListOfQualityControl (qualitycontrols, num_qcs);
						std::cout << "Successfully retrieved QualityControl list" << std::endl;
					}
					break;
				}

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

		case smWorklistOptions:
		{
			smState_ = smCmdEntry;

			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					Worklist worklist;

					createEmptyCarouselWorklist (worklist);
					he = SetWorklist (worklist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set empty carousel Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully set empty carousel Worklist..." << std::endl;
					}
					break;
				}

				case 2:
					std::cout << "curSampleSetIndex set to zero..." << std::endl;
					curSampleSetIndex = 0;
					break;

				case 3:
					SampleDefinition* sample;
					he = GetCurrentSample (sample);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get current sample: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						displaySampleDefinition (*sample);
						FreeSampleDefinition (sample, 1);
					}
					break;

				case 4:
				{
					Worklist worklist;
					createCarouselWorklist (worklist);
					he = SetWorklist (worklist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully set Worklist..." << std::endl;
					}
					break;
				}

				case 5:
					he = StartProcessing (currentUser_.c_str(), currentPassword_.c_str(), onSampleStatus, onSampleImageProcessed, onSampleComplete, onWorklistComplete);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to start Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						//if (!getWqStatusTimer_)
						//{
						//	getWqStatusTimer_ = std::make_shared<boost::asio::deadline_timer>(io_svc);
						//	getWorklistStatus(boost::system::error_code());
						//}
						std::cout << "Successfully started Worklist..." << std::endl;
					}
					break;

				case 6:
					he = StopProcessing(currentUser_.c_str(), currentPassword_.c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to stop Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully stopped Worklist..." << std::endl;
					}
					break;

				case 7:
					he = PauseProcessing(currentUser_.c_str(), currentPassword_.c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to pause Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully paused Worklist..." << std::endl;
					}
					break;

				case 8:
					he = ResumeProcessing(currentUser_.c_str(), currentPassword_.c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to resume Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully resumed Worklist..." << std::endl;
					}
					break;

				case 9:
					//uint32_t queueLen;
					//Worklist* wl;
					//eSystemStatus wqStatus;

					//he = GetWorklistStatus (queueLen, wq, wqStatus);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to get Worklist status: " << GetErrorAsStr(he) << std::endl;
					//} else {
					//	std::cout << "Successfully retrieved Worklist status..." << std::endl;
					//	std::cout << "Worklist status: " << GetWorklistStatusAsStr (wqStatus);
					//	for (uint32_t i = 0; i < queueLen; i++) {
					//		displayWorklistItem (wq[i]);
					//	}
					//	FreeWorklistItem (wq, queueLen);
					//}
					break;

				case 10:
					//he = SkipCurrentWorklistItem();
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to skip current Worklist item: " << GetErrorAsStr(he) << std::endl;
					//} else {
					//	std::cout << "Successfully skipped current Worklist item..." << std::endl;
					//}
					break;

				case 11:
				{
					SampleSet sampleSet;
					createCarouselSampleSet2 (sampleSet);
					he = AddSampleSet (sampleSet);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set additional SampleSet: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully added SampleSet to Worklist..." << std::endl;
					}
					break;
				}

				case 12:
				{
					SampleSet sampleSet;
					createCarouselSampleSet5 (sampleSet);
					he = AddSampleSet (sampleSet);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set additional SampleSet: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully added SampleSet to Worklist..." << std::endl;
					}
					break;
				}

				case 13:
				{
					SampleSet sampleSet;
					createCarouselSampleSet10 (sampleSet);
					he = AddSampleSet (sampleSet);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set additional SampleSet: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully added SampleSet to Worklist..." << std::endl;
					}
					break;
				}

				case 14:
				{
					smStateOption_ = smCancelSampleSet;
					smState_ = smSampleSetToCancelEntry;
					prompt();
					break;
				}

				case 15:
				{
					Worklist worklist;
					createEmptyPlateWorklist (worklist);
					he = SetWorklist (worklist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to create empty plate Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully create empty plate Worklist..." << std::endl;
					}
					break;
				}

				case 16:
				{
					SampleSet sampleSet;
					createPlateSampleSet_3_6 (sampleSet);
					he = AddSampleSet (sampleSet);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set plate SampleSet to Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else
					{
						std::cout << "Successfully added plate SampleSet to Worklist..." << std::endl;
					}
					break;
				}

				case 17:
				{
					Worklist worklist;
					createPlateWorklist_A1_A12_H1_H12 (worklist);
					he = SetWorklist (worklist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to set Worklist: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully set Worklist..." << std::endl;
					}
					break;
				}

				case 18:
				{
//TODO: create a Worklist with a SampleSet (other than orphan)...
//TODO: to be completed...
					//uuid__t uuid;

					//he = DeleteSampleSetTemplate (uuid);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve list of SampleSets: " << GetErrorAsStr(he) << std::endl;
					//}
					//else {
					//	std::cout << "Successfully retrieved list of SampleSets..." << std::endl;
					//}
					//break;
					std::cout << "DeleteSampleSetTemplate not supported..." << std::endl;
					break;
				}

				case 19:
				{
//TODO: create a Worklist with a SampleSet (other than orphan)...
//TODO: to be completed...
					//uuid__t uuid;

					//he = DeleteSampleSetTemplate (uuid);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve list of SampleSets: " << GetErrorAsStr(he) << std::endl;
					//}
					//else {
					//	std::cout << "Successfully retrieved list of SampleSets..." << std::endl;
					//}
					//break;
					std::cout << "GetSampleSetTemplateAndSampleList not supported..." << std::endl;
					break;
				}

				case 20:
				{
					SampleSet sampleSet;
					createCarouselSampleSet2 (sampleSet);
					he = SaveSampleSetTemplate (sampleSet);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to save SampleSetTemplate: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully saved SampleSetTemplate..." << std::endl;
					}
					break;
				}

				case 21:
				{
					uint32_t skip = 0;
					uint32_t take = 10;
					SampleSet* sampleSets;
					uint32_t numSampleSets;
					uint32_t totalSampleSetsAvailable;

					he = GetSampleSetTemplateList (skip, take, sampleSets, numSampleSets, totalSampleSetsAvailable);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve list of SampleSets: " << GetErrorAsStr(he) << std::endl;
					}
					else {
						std::cout << "Successfully retrieved list of SampleSets..." << std::endl;
						FreeSampleSet (sampleSets, numSampleSets);
					}
					break;
				}

				default:
					smState_ = smCmdEntry;
					std::cout << "Invalid entry..." << std::endl;

			} // End "switch (atoi (smValueLine_.c_str()))"

			break;
		}

		case smMiscOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					SystemStatusData* systemStatus;
					GetSystemStatus (systemStatus);
					FreeSystemStatus (systemStatus);
					break;
				}
				case 2:
				{
					uint64_t starttime = 0;
					uint64_t endtime = 0;
					uint32_t num_entries = 0;
					audit_log_entry* log_entries;

					he = RetrieveAuditTrailLogRange (starttime, endtime, num_entries, log_entries);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "RetrieveAuditTrailLogRange failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "RetrieveAuditTrailLogRange Successful..." << std::endl;
						FreeAuditLogEntry (log_entries, num_entries);
					}
					break;
				}

				case 3:
				{
					//virtual HawkeyeError ArchiveAuditTrailLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 4:
				{
					//virtual HawkeyeError ReadArchivedAuditTrailLog(char* archive_location, uint32_t& num_entries, audit_log_entry*& log_entries);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 5:
				{
					uint64_t starttime = 0;
					uint64_t endtime = 0;
					uint32_t num_entries = 0;
					error_log_entry* log_entries;

					he = RetrieveInstrumentErrorLogRange (starttime, endtime, num_entries, log_entries);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "RetrieveInstrumentErrorLogRange failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "RetrieveInstrumentErrorLogRange Successful..." << std::endl;
						FreeErrorLogEntry (log_entries, num_entries);
					}
					break;
				}
				case 6:
				{
					//virtual HawkeyeError ArchiveInstrumentErrorLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 7:
				{
					//virtual HawkeyeError ReadArchivedInstrumentErrorLog(char* archive_location, uint32_t& num_entries, error_log_entry*& log_entries);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 8:
				{
					uint64_t starttime = 0;
					uint64_t endtime = 0;
					uint32_t num_entries = 0;
					sample_activity_entry* log_entries;

					he = RetrieveSampleActivityLogRange (starttime, endtime, num_entries, log_entries);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "RetrieveSampleActivityLogRange failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "RetrieveSampleActivityLogRange Successful..." << std::endl;
						FreeSampleActivityEntry (log_entries, num_entries);
					}

					break;
				}
				case 9:
				{
					//virtual HawkeyeError ArchiveSampleActivityLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 10:
				{
					//virtual HawkeyeError ReadArchivedSampleActivityLog(char* archive_location, uint32_t& num_entries, error_log_entry*& log_entries);
					std::cout << "Not currently supported..." << std::endl;
					break;
				}
				case 11:	// SyringePumpValveTest
				{
					LogoutUser();

					LoginUser ( ServiceUser.c_str(), ServicePwd.c_str() );

					while (true) {
						int count = 10;
						char physicalPort = 'A';

						while (count--) {
							he = svc_SetValvePort (physicalPort);
							if (he != HawkeyeError::eSuccess) {
								std::cout << boost::str (boost::format ("SyringePumpValveTest: moving valve to Physical port %c failed: %s") % physicalPort % GetErrorAsStr(he)) << std::endl;
							}
							else {
								std::cout << boost::str (boost::format ("SyringePumpValveTest: moving valve to Physical port %c successful") % physicalPort) << std::endl;
							}

							physicalPort += 1;
							if (physicalPort > 'H') {
								physicalPort = 'A';
							}
						}

						count = 10;
						while (count--) {
							he = svc_SetValvePort (physicalPort);
							if (he != HawkeyeError::eSuccess) {
								std::cout << boost::str (boost::format ("SyringePumpValveTest: moving valve to Physical port %c failed: %s") % physicalPort % GetErrorAsStr(he)) << std::endl;
							}
							else {
								std::cout << boost::str (boost::format ("SyringePumpValveTest: moving valve to Physical port %c successful") % physicalPort) << std::endl;
							}

							physicalPort -= 1;
							if (physicalPort < 'A') {
								physicalPort = 'H';
							}
						}

					}
					break;
				}
				case 12:
				{
					uint32_t num_entries;
					calibration_history_entry* log_entries;
					he = RetrieveCalibrationActivityLogRange (cal_Concentration, 0, 0, num_entries, log_entries);
					FreeCalibrationHistoryEntry (log_entries, num_entries);
					break;
				}
				case 13:
				{
					LogoutUser();
					LoginUser ( ServiceUser.c_str(), ServicePwd.c_str() );

					double slope = 5.0;
					double intercept = 10.0;
					uint32_t cal_image_count = 100;
					uuid__t queue_id = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
					uint16_t num_consumables = 3;
					calibration_consumable consumables[3] = { {"consumable #1", "11111", 17000, 11.1}, {"consumable #2", "22222", 17000, 22.2}, {"consumable #3", "33333", 17000, 33.3} };

					he = SetConcentrationCalibration (calibration_type::cal_Concentration, slope, intercept, cal_image_count, queue_id, num_consumables, consumables);

					break;
				}

				case 14:
				{
					boost::system::error_code ec;
					SystemStatusTest (ec);
					break;
				}

				case 15:
				{
					StopSystemStatusTest();
					break;
				}

				case 16:
				{
					uint32_t errorcode = 2198011906;
					char *sys, *subsys, *instance, *failure, *severity, *cellHealthErrorCode;

					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure, cellHealthErrorCode);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << "\tCellHealthErrorCode:   \"" << cellHealthErrorCode << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);
					FreeCharBuffer(cellHealthErrorCode);

					errorcode = 2164588801;
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure, cellHealthErrorCode);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << "\tCellHealthErrorCode:   \"" << cellHealthErrorCode << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);
					FreeCharBuffer(cellHealthErrorCode);

					errorcode = 3238134786;
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure, cellHealthErrorCode);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << "\tCellHealthErrorCode:   \"" << cellHealthErrorCode << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);
					FreeCharBuffer(cellHealthErrorCode);

					errorcode = 0xc1031305; // Should have a number of spaces removed from the failure / instance.
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure, cellHealthErrorCode);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << "\tCellHealthErrorCode:   \"" << cellHealthErrorCode << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);
					FreeCharBuffer(cellHealthErrorCode);

					break;
				}

				case 17:
				{
					SampleTubeDiscardTrayEmptied();
					break;
				}
				case 18:
					{
					auto quit = GetStringInput(entryStr, "Enter hardware type:");
						if(!quit)
						{
							try
							{
								auto type = (OpticalHardwareConfig)std::stoi(entryStr);
								auto result = SetOpticalHardwareConfig(type);
								std::cout << "Result of setting hardware type: " << (int32_t)result << std::endl;

							}
							catch(...)
							{
								cout << "Error setting hardware type" << endl;
							}

						}
						
					}
					break;
				case 19:
					{
						auto type = GetOpticalHardwareConfig();
						std::cout << "Hardware type = " << (int32_t)type << std::endl;
						
					}
				break;
				default:
					std::cout << "Invalid entry..." << std::endl;

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		
		} // End "case smMiscOptions"

		case smDatabaseOptions:
		{
			std::cout << std::endl;

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:
				{
					smState_ = smDbConnectionOptions;
					prompt();
					break;
				}

				case 2:
				{
					smState_ = smDbCellTypeOptions;
					prompt();
					break;
				}

				case 3:
				{
					smState_ = smDbAnalysisDefinitionOptions;
					prompt();
					break;
				}

				case 4:
				{
					smState_ = smDbAnalysisParameterOptions;
					prompt();
					break;
				}

				case 5:
				{
					smState_ = smDbUserOptions;
					prompt();
					break;
				}

				case 6:
				{
					smState_ = smDbRoleOptions;
					prompt();
					break;
				}

				case 7:
				{
					smState_ = smDbInstrumentConfigOptions;
					prompt();
					break;
				}

				case 8:
				{
					smState_ = smDbLogOptions;
					prompt();
					break;
				}

				case 9:
				{
					smState_ = smDbObjAdditionOptions;
					prompt();
					break;
				}

				case 10:
				{
					smState_ = smDbObjUpdateOptions;
					prompt();
					break;
				}

				case 11:
				{
					smState_ = smDbObjRemovalOptions;
					prompt();
					break;
				}

				case 12:
				{
					smState_ = smDbRecordListOptions;
					prompt();
					break;
				}

				default:
				{
					smState_ = smCmdEntry;
					std::cout << "Invalid entry..." << std::endl;
					break;
				}
			}
			break;
		}

		case smDbConnectionOptions:
		{
			static std::string dbAddrStr= "127.0.0.1";
			static std::string dbNameStr = "ViCellDB";
			static std::string dbPortStr = "5432";
			static int32_t dbport = 5432;
			static std::string dbDriverStr = "{PostgreSQL ODBC Driver(UNICODE)}";

			DBApi::DbGetConnectionProperties( dbAddrStr, dbPortStr, dbNameStr, dbDriverStr );
			dbport = stoi( dbPortStr );

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:
				{
					entryStr.clear();
					promptStr = boost::str( boost::format( "Current addess: %s\r\nEnter address: " ) % dbAddrStr );

					GetStringInput( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() == 0 )
					{
						std::cout << "No address entered!" << std::endl;
						break;
					}
					dbAddrStr = entryStr;

					if ( DBApi::DbSetConnectionAddr( dbAddrStr, dbport ) )
					{
						std::cout << "Successfully set DB IP address to " << dbAddrStr << std::endl;
						input_quit = DoDbConnect();
						if ( input_quit )
						{
							return;
						}
					}
					else
					{
						std::cout << "Failed to set DB IP address..." << std::endl;
					}

					if ( input_quit )
					{
						return;
					}
					break;
				}

				case 2:
				{
					entryStr.clear();
					promptStr = boost::str( boost::format( "Current port #: %s\r\nEnter Port #: " ) % dbPortStr );

					GetStringInput( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() == 0 )
					{
						std::cout << "No port number entered!" << std::endl;
						break;
					}
					dbPortStr = entryStr;
					dbport = stoi( dbPortStr );

					if ( DBApi::DbSetConnectionAddr( dbAddrStr, dbport ) )
					{
						std::cout << "Successfully set DB listening port to " << dbport << std::endl;
						input_quit = DoDbConnect();
						if ( input_quit )
						{
							return;
						}
					}
					else
					{
						std::cout << "Failed to set DB listening port..." << std::endl;
					}
					break;
				}

				case 3:
				{
					entryStr.clear();
					promptStr = boost::str( boost::format( "Current Database: %s\r\nEnter DB Name: " ) % dbNameStr );

					GetStringInput( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() == 0 )
					{
						std::cout << "No DB name entered!" << std::endl;
						break;
					}
					dbNameStr = entryStr;

					if ( DBApi::DbSetDbName( dbNameStr ) )
					{
						std::cout << "Successfully set DB name to " << dbNameStr << std::endl;
						input_quit = DoDbConnect();
						if ( input_quit )
						{
							return;
						}
					}
					else
					{
						std::cout << "Failed to set DB name..." << std::endl;
					}
					break;
				}

				case 4:
				{
					std::string currentDbAddrStr = "";
					std::string currentDbNameStr = "";
					std::string currentDbPortStr = "";
					std::string currentDbDriverStr = "";

					DBApi::DbGetConnectionProperties( currentDbAddrStr, currentDbPortStr, currentDbNameStr, currentDbDriverStr );
					std::cout << std::endl;
					std::cout << "Current server interface configuration: " << std::endl;
					std::cout << "\tServer addess:\t\t" << currentDbAddrStr << std::endl;
					std::cout << "\tServer port #:\t\t" << currentDbPortStr << std::endl;
					std::cout << "\tServer Database:\t" << currentDbNameStr << std::endl;
					std::cout << "\tServer Driver:\t\t" << currentDbDriverStr << std::endl;
					std::cout << std::endl;

					break;
				}

				case 5:
				{
					DBApi::DBifInit();
					break;
				}

				case 6:
				{
					std::cout << "Current addess: " << dbAddrStr << std::endl;
					std::cout << "Current port #: " << dbPortStr << std::endl;
					std::cout << "Current Database: " << dbNameStr << std::endl;
					std::cout << std::endl;

					input_quit = DoDbConnect( true );
					if ( input_quit )
					{
						return;
					}
					break;
				}

				case 7:
				{
					DBApi::DbDisconnect();
					break;
				}

				case 8:
				{
					bool clearOk = DBApi::DbClearDb();
					if ( !clearOk )
					{
						std::cout << std::endl << "Error clearing database! DB may not be valid!" << std::endl << std::endl;
					};
					break;
				}

				case 9:
				{
					DBApi::DbInstrumentLogin();
					break;
				}

				case 10:
				{
					DBApi::DbAdminLogin();
					break;
				}

				case 11:
				{
					DBApi::DbUserLogin();
					break;
				}

				case 12:
				{
					DBApi::DbLogoutUserType( DBApi::eLoginType::InstrumentLoginType );
					break;
				}

				case 13:
				{
					DBApi::DbLogoutUserType( DBApi::eLoginType::AdminLoginType );
					break;
				}

				case 14:
				{
					DBApi::DbLogoutUserType(DBApi::eLoginType::UserLoginType);
					break;
				}

				case 15:
				{
					DBApi::DbDisconnect();
					break;
				}

				default:
				{
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
				}
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbCellTypeOptions:
		{
			smState_ = smCmdEntry;
			std::cout << std::endl;
			std::cout << "Not implemented.." << std::endl;
			std::cout << std::endl;
			break;
		}

		case smDbAnalysisDefinitionOptions:
		{
			smState_ = smCmdEntry;
			std::cout << std::endl;
			std::cout << "Not implemented.." << std::endl;
			std::cout << std::endl;
			break;
		}

		case smDbAnalysisParameterOptions:
		{
			smState_ = smCmdEntry;
			std::cout << std::endl;
			std::cout << "Not implemented.." << std::endl;
			std::cout << std::endl;
			break;
		}

		case smDbUserOptions:
		{
#define	MAX_DISPLAY_COLUMNS 12

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:		// Retrieve list of Users
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_UserRecord> user_list;
					DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;

					qResult = DBApi::DbGetUserList(user_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved User List..." << std::endl;

						print_db_user_list( user_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve User list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:		// Create User
				{	// this will bypass the HawkeyeCore DLL
					DBApi::DB_UserRecord new_user = {};
					bool process_line = false;
					DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;
					std::string nameStr = "";
					int minLength = 0;

					entryStr.clear();
					promptStr = "Enter User Name: ";
					lengthWarning = "ERROR: User name cannot be blank!";

					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					nameStr = entryStr;

					std::vector<DBApi::DB_UserRoleRecord> roles_list;

					qResult = DBApi::DbGetRolesList( roles_list );
					if ( qResult != DBApi::eQueryResult::QueryOk )
					{
						if ( qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "No roles available..." << std::endl;
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Failed to retrieve Role list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						}
						break;
					}

					print_db_roles_list( roles_list );

					int32_t listIdx = 0;
					size_t listSize = 0;
					int64_t roleIdNum = 0;
					bool idNumFound = false;
					DBApi::DB_UserRoleRecord userRole = {};

					listSize = roles_list.size();

					if ( listSize > 0 )
					{
						do
						{
							input_quit = SelectDbListIdNum( listSize, roleIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( roleIdNum >= 0 )
							{
								do
								{
									userRole = roles_list.at( listIdx );
									if ( userRole.RoleIdNum == roleIdNum )
									{
										idNumFound = true;
									}
								} while ( idNumFound == false && ++listIdx < listSize );
							}
							else
							{
								break;
							}
						} while ( idNumFound == false );

						entryStr.clear();
						promptStr = "AD user type? (y/n):  ";

						input_quit = GetYesOrNoResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
						{
							new_user.ADUser = true;			// new user may be either local or AD user
						}

						// initialize to one hour beyond the start of the epoch to avoid an empty system_TP record.
						system_TP zeroTP = ChronoUtilities::ConvertToTimePoint<std::chrono::hours>( 1 );

						new_user.UserIdNum = 0;				// should be cleared to 0; value is filled-in by DB interface
						new_user.UserId = {};				// jshould be cleared; value is filled-in by the Db interface
						new_user.Protected = false;
						new_user.Retired = false;			// new user is never retired
						new_user.RoleId = userRole.RoleId;	// initialize with the uuid value of the associated role;
						new_user.UserNameStr = nameStr;		// REQUIRED: will be checked for duplicates by the DB! Check for duplicate name SHOULD happen prior to submitting a new user record!
						new_user.DisplayNameStr = nameStr;	// this is not required and not maintained as unique.  it is used only for display
						new_user.Comment = "";
						new_user.UserEmailStr = new_user.UserNameStr;	// insert entered email address string; here I'm constructing a bogus address
						new_user.UserEmailStr.append("@InvalidEmail.com");
						new_user.AuthenticatorList.push_back( "authenticator1-1234567890abcdef" );	// authenticators are hashed and stored as a list of strings; here I'm just storing the unhashed authenticator strings
						new_user.AuthenticatorList.push_back( "authenticator2-1234567890abcdef" );
						new_user.AuthenticatorDateTP = zeroTP;		// may be omitted if initialized correctly to an empty content and will be filled-ion by db interface;
						new_user.LastLogin = zeroTP;				// may be omitted if initialized correctly to an empty content and will be filled-ion by db interface;
						new_user.AttemptCount = 0;					// new user; initialize to 0
						new_user.DefaultSampleNameStr = "Sample";	// not required, but should be initialized to the default if provided
						new_user.UserImageSaveN = 1;				// should be initialized to 1 if no default is provided
						new_user.DecimalPrecision = 5;				// should be initialized to the sytem default precision (???)
						new_user.ExportFolderStr = "\\Instrument\\Export\\";	// initialize to a value if provided
						new_user.ExportFolderStr.append(new_user.UserNameStr);	// this is using the name and appending to the default export folder (not required)
						new_user.DefaultResultFileNameStr = "Summary";			// initialize to a value if provided
						new_user.CSVFolderStr = "\\Instrument\\Export\\CSV\\";	// initialize to a value if provided, or the system default CSV folder
						new_user.CSVFolderStr.append( new_user.UserNameStr );	// this is constructing the foldername from the username (not required)
						new_user.PdfExport = true;								// initialize to the default or the value provided
						new_user.AllowFastMode = false;							// initialize to the default or the value provided
						new_user.WashType = 0;									// initialize to the default or the value provided
						new_user.Dilution = 1;									// initialize to the default or the value provided
						new_user.NumUserCellTypes = 1;							// initialize to the number of entries in the cell-type list
						new_user.UserCellTypeIndexList.push_back( 0 );			// initialize to the default or the value(s) provided
						new_user.NumUserProperties = 0;							// initialize to the default or the value(s) provided
						// hash values should be calculated using the standard has technique and stored as strings; here I'm just storing the permissions values as un-hashed strings
						new_user.AppPermissions = userRole.ApplicationPermissions;
						new_user.InstPermissions = userRole.InstrumentPermissions;
						new_user.AppPermissionsHash = boost::str( boost::format( "%lu" ) % ( userRole.ApplicationPermissions ) );
						new_user.InstPermissionsHash = boost::str( boost::format( "%lu" ) % ( userRole.InstrumentPermissions ) );
						new_user.LanguageCode = "en-US";

						if ( userRole.RoleNameStr == "DefaultAdmin" || userRole.RoleIdNum == 1 )
						{
							roleIdNum = 1;
						}
	
						for ( uint16_t i = 0; i < MAX_DISPLAY_COLUMNS; i++ )
						{
							DBApi::display_column_info_t columnInfo = {};

							columnInfo.ColumnType = i;

							if ( roleIdNum == 1 )
							{
								columnInfo.OrderIndex = ( i + 2 );
							}
							else
							{
								columnInfo.OrderIndex = i;
							}

							switch ( i )
							{
								case 0:
									if ( roleIdNum == 1 )
									{
										columnInfo.Width = 205;
									}
									else
									{
										columnInfo.Width = 80;
									}
									break;

								case 1:
									columnInfo.Width = 50;
									break;

								case 2:
									columnInfo.Width = 125;
									break;

								case 6:
									if ( roleIdNum == 1 )
									{
										columnInfo.Width = 125;
									}
									else
									{
										columnInfo.Width = 70;
									}
									break;

								case 7:
									if ( roleIdNum == 1 )
									{
										columnInfo.Width = 70;
									}
									else
									{
										columnInfo.Width = 125;
									}
									break;

								case 3:
								case 4:
								case 5:
								case 8:
								case 9:
									columnInfo.Width = 70;
									break;

								case 10:
									if ( roleIdNum == 1 )
									{
										columnInfo.Width = 145;
									}
									else
									{
										columnInfo.Width = 100;
									}
									break;

								case 11:
									columnInfo.Width = 100;
									break;

								default:
									break;
							}
							columnInfo.Visible = true;

							new_user.ColumnDisplayList.push_back( columnInfo );
						}

						size_t ctListSize = new_user.UserCellTypeIndexList.size();
						if (new_user.UserAnalysisIndexList.size() > 0 )
						{
							new_user.UserAnalysisIndexList.clear();
						}

						for ( int i = 0; i < ctListSize; i++ )
						{
							new_user.UserAnalysisIndexList.push_back( 0 );		// currently, all analysis def indices are '0'...
						}

						qResult = DBApi::DbAddUser( new_user );
						if ( qResult == DBApi::eQueryResult::QueryOk )
						{
							std::cout << "Successfully Added User..." << std::endl;
							print_db_user_record( new_user );
							std::cout << std::endl;
						}
						else if ( qResult == DBApi::eQueryResult::InsertObjectExists )
						{
							std::cout << "Failed to add User: " << nameStr << " of type: " << ((new_user.ADUser) ? "ActiveDirectory" : "local") << "  : User exists for the name and user-type" << std::endl;
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Query failed adding User: " << nameStr << " of type: " << ( ( new_user.ADUser ) ? "ActiveDirectory" : "local" ) << ")  returned: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						}
					}
					else
					{
						std::cout << "User not added: no roles to assign" << std::endl;
					}
					break;
				}

				case 3:		// Find user by name
				{
					bool process_line = false;
					DBApi::eUserType userType = DBApi::eUserType::AllUsers;
					DBApi::DB_UserRecord userRec = {};
					DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;
					std::string nameStr = "";

					entryStr.clear();
					promptStr = "Enter User Name: ";
					lengthWarning = "ERROR: User name cannot be blank!";

					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					nameStr = entryStr;

					entryStr.clear();
					promptStr = "Select user type? (y/n):  ";

					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						bool typeOk = true;
						promptStr = "User Type (-1: local  0: all  1: AD): ";
						lengthWarning = "ERROR: User type cannot be blank!";

						do
						{
							entryStr.clear();
							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							int typeNum = std::stol( entryStr );

							switch ( typeNum )
							{
								case -1:
								{
									userType = DBApi::eUserType::LocalUsers;
									break;
								}
								case 0:
								{
									userType = DBApi::eUserType::AllUsers;
									break;
								}

								case 1:
								{
									userType = DBApi::eUserType::AdUsers;
									break;
								}

								default:
								{
									std::cout << "Illegal user type entered. values must be -1, 0, or 1" << std::endl;
									typeOk = false;
									break;
								}
							}

							std::cout << std::endl;
						} while ( !typeOk );
					}

					qResult = DBApi::DbFindUserByName( userRec, nameStr, userType );
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully Found User for Name: " << nameStr << "..." << std::endl;
						print_db_user_record( userRec );
						std::cout << std::endl;
					}
					else if ( qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "No uses found for Name: " << nameStr << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Query failed finding user for Name: " << nameStr << "returned: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 4:		// Update User
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_UserRecord> user_list;
					DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;

					qResult = DBApi::DbGetUserList( user_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved User List..." << std::endl;

						print_db_user_list( user_list );

						int32_t listIdx = 0;
						size_t listSize = 0;
						int64_t listIdNum = 0;
						bool idNumFound = false;
						bool process_line = false;
						DBApi::DB_UserRecord userRec = {};

						listSize = user_list.size();
						if ( listSize > 0 )
						{
							do
							{
								input_quit = SelectDbListIdNum( listSize, listIdNum );
								if ( input_quit )
								{
									return;
								}

								if ( listIdNum >= 0 )
								{
									do
									{
										userRec = user_list.at( listIdx );
										if ( userRec.UserIdNum == listIdNum )
										{
											idNumFound = true;
										}
									} while ( idNumFound == false && ++listIdx < listSize );
								}
								else
								{
									break;
								}
							} while ( idNumFound == false );

							std::string nameStr = "";

							nameStr = userRec.DisplayNameStr;

							entryStr.clear();
							promptStr = "Update display name? (y/n):  ";

							input_quit = GetYesOrNoResponse( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
							{
								entryStr.clear();
								promptStr = "Enter display name: ";

								GetStringInput( entryStr, promptStr );
								if ( input_quit )
								{
									return;
								}

								if ( entryStr.length() == 0 )
								{
									std::cout << "WARNING: Display name is blank!" << std::endl;
								}
								std::cout << std::endl;
								nameStr = entryStr;
							}
							userRec.DisplayNameStr = nameStr;	// this is not maintained as unique; it is just the displayed name...

							promptStr = "Retire user? (y/n):  ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
							{
								userRec.Retired = true;
							}

							userRec.Comment = "Adding a comment";
							userRec.UserEmailStr = userRec.UserNameStr;	// insert entered email address string; here I'm constructing a bogus address
							userRec.UserEmailStr.append( "@AnotherInvalidEmail.com" );
							userRec.WashType = 0;									// initialize to the default or the value provided
							userRec.Dilution = 1;									// initialize to the default or the value provided

							std::vector<DBApi::display_column_info_t> infoList = userRec.ColumnDisplayList;

							userRec.ColumnDisplayList.clear();
							size_t infoListSize = infoList.size();

							for ( uint16_t i = 0; i < MAX_DISPLAY_COLUMNS; i++ )
							{
								DBApi::display_column_info_t columnInfo = infoList.at( i );

								if ( infoListSize <= 0 )
								{
									/*
									factory_admin: {"(0,2,207,f)","(1,3,50,f)","(2,4,125,f)","(3,5,70,f)","(4,6,70,f)","(5,7,70,f)","(6,8,125,f)","(7,9, 70,f)","(8,10,70,f)","(9,11,70,f)","(10,12,147,f)","(11,13,100,f)"}
									other users:   {"(0,0, 80,f)","(1,1,50,f)","(2,2,125,f)","(3,3,70,f)","(4,4,70,f)","(5,5,70,f)","(6,6, 70,f)","(7,7,125,f)","(8, 8,70,f)","(9, 9,70,f)","(10,10,100,f)","(11,11,100,f)"}
									*/
									columnInfo.ColumnType = i;

									if ( userRec.UserIdNum == 1 )
									{
										columnInfo.OrderIndex = ( i + 2 );
									}
									else
									{
										columnInfo.OrderIndex = i;
									}

									switch ( i )
									{
										case 0:
											if ( userRec.UserIdNum == 1 )
											{
												columnInfo.Width = 207;
											}
											else
											{
												columnInfo.Width = 80;
											}
											break;

										case 1:
											columnInfo.Width = 50;
											break;

										case 2:
											columnInfo.Width = 125;
											break;

										case 6:
											if ( userRec.UserIdNum == 1 )
											{
												columnInfo.Width = 125;
											}
											else
											{
												columnInfo.Width = 70;
											}
											break;

										case 7:
											if ( userRec.UserIdNum == 1 )
											{
												columnInfo.Width = 70;
											}
											else
											{
												columnInfo.Width = 125;
											}
											break;

										case 3:
										case 4:
										case 5:
										case 8:
										case 9:
											columnInfo.Width = 70;
											break;

										case 10:
											if ( userRec.UserIdNum == 1 )
											{
												columnInfo.Width = 147;
											}
											else
											{
												columnInfo.Width = 100;
											}
											break;

										case 11:
											columnInfo.Width = 100;
											break;

										default:
											break;
									}
								}
								columnInfo.Visible = true;

								userRec.ColumnDisplayList.push_back( columnInfo );
							}

							size_t ctListSize = userRec.UserCellTypeIndexList.size();
							if ( userRec.UserAnalysisIndexList.size() > 0 )
							{
								userRec.UserAnalysisIndexList.clear();
							}

							for ( int i = 0; i < ctListSize; i++ )
							{
								userRec.UserAnalysisIndexList.push_back( 0 );		// currently, all analysis def indices are '0'...
							}

							qResult = DBApi::DbModifyUser( userRec );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								DBApi::DB_UserRecord tmpUserRec = {};

								std::cout << "Successfully modified User..." << std::endl;

								qResult = DBApi::DbFindUserByUuid( tmpUserRec, userRec.UserId );
								if ( qResult == DBApi::eQueryResult::QueryOk )
								{
									std::cout << "Successfully retrieved modified user record..." << std::endl;

									print_db_user_record( tmpUserRec );
								}
								else
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to retrieve modified User record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}

								std::cout << std::endl;
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to modify User: returned: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve User list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 5:
				{
					// Update Backup User password; modify the password for the PostgreSQL global DbBackupUser role, not a Vi-CELL user; does not modify anything in the Vi-CELL database
					// This method entry state will directly test the DBif interface API and will bypass the HawkeyeCore DLL; another test method will need to test the HawkeyeCore DLL API exposed to the UI

					smStateOption_ = smDbSetBackupUserPwd;
					smState_ = smStateOption_;
					goto PROCESS_STATE;

					break;
				}

				default:
				{
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
				}
			}

			smState_ = smCmdEntry;
			break;
		}

		case smSetDbBackupUserPwd:
		case smDbSetBackupUserPwd:
		{
			// Update Backup User password; modify the password for the PostgreSQL global DbBackupUser role, not a Vi-CELL user; does not modify anything in the Vi-CELL database
			DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;
			std::string pwdStr = "";
			int minPwdLength = 8;

			entryStr.clear();
			promptStr = "Enter the new paswsword, or 'x' or esc to quit:  ";
			lengthWarning = boost::str( boost::format( "WARNING: Password should be %d or more characters for this test!" ) % minPwdLength );

			input_quit = GetStringInput( entryStr, promptStr, lengthWarning, minPwdLength, HIDE_ENTRY );
			if ( input_quit )
			{
				return;
			}

			pwdStr = entryStr;

			bool pwdOk = false;
			std::string errStr = "Failed to set backup user password";

			if ( smState_ == smSetDbBackupUserPwd )
			{
				// This method will test the HawkeyeCore DLL API exposed to the UI
				he = SetDbBackupUserPassword( pwdStr.c_str() );
				if ( he == HawkeyeError::eSuccess )
				{
					pwdOk = true;
				}
				else
				{
					errStr.append( "." );
				}
			}
			else
			{
				// This method entry will directly test the DBif interface API and will bypass the HawkeyeCore DLL; another test method will need to test the HawkeyeCore DLL API exposed to the UI
				pwdOk = DBApi::DbSetBackupUserPwd( qResult, pwdStr );
				if ( !pwdOk || qResult != DBApi::eQueryResult::QueryOk )
				{
					DBApi::GetQueryResultString( qResult, queryResultStr );
					errStr.append( boost::str(boost::format( ": returned: %s (%d)") % queryResultStr % (int32_t)qResult ) );
				}
			}

			if ( pwdOk )
			{
				std::cout << "Successfully set backup user password..." << std::endl;
			}
			else
			{
				std::cout << errStr << std::endl;
			}
			std::cout << std::endl;

			smState_ = smCmdEntry;
			break;
		}

		case smDbRoleOptions:
		{
			promptStr = "";
			lengthWarning = "";

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:		// Retrieve list of Roles
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_UserRoleRecord> roles_list;
					DBApi::eQueryResult qResult;

					qResult = DBApi::DbGetRolesList( roles_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Role List..." << std::endl;

						print_db_roles_list( roles_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Role List: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:		// Create Role
				{
					DBApi::DB_UserRoleRecord new_role = {};
					uint32_t roleClass = static_cast<uint32_t>( DBApi::eRoleClass::RoleClassUndefined );
					bool process_line = false;
					DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryOk;

					entryStr.clear();
					promptStr = "Enter Role Name: ";
					lengthWarning = "ERROR: Role name cannot be blank!";

					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					new_role.RoleNameStr = entryStr;

					promptStr = "Enter Role Class Number: ";
					lengthWarning = "ERROR: Role class number cannot be blank!";

					do
					{
						entryStr.clear();

						input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
						if ( input_quit )
						{
							return;
						}

						roleClass = stol( entryStr );
						switch ( roleClass )
						{
							case ( (uint32_t) DBApi::eRoleClass::UserClass1 ):
							case ( (uint32_t) DBApi::eRoleClass::UserClass2 ):
							case ( (uint32_t) DBApi::eRoleClass::UserClass3 ):
							case ( (uint32_t) DBApi::eRoleClass::UserClass4 ):
							case ( (uint32_t) DBApi::eRoleClass::ElevatedClass1 ):
							case ( (uint32_t) DBApi::eRoleClass::ElevatedClass2 ):
							case ( (uint32_t) DBApi::eRoleClass::ElevatedClass3 ):
							case ( (uint32_t) DBApi::eRoleClass::ElevatedClass4 ):
							case ( (uint32_t) DBApi::eRoleClass::AdminClass1 ):
							case ( (uint32_t) DBApi::eRoleClass::AdminClass2 ):
							case ( (uint32_t) DBApi::eRoleClass::AdminClass3 ):
							case ( (uint32_t) DBApi::eRoleClass::AdminClass4 ):
							{
								new_role.ApplicationPermissions = 0xFFFFFFFF;
								new_role.InstrumentPermissions = 0xFFFFFFFF;
								break;
							}

							case ( (uint32_t) DBApi::eRoleClass::NoRoleClass ):
							{
								std::cout << "Role class cannot be specified as 'NoRoleClass'" << std::endl;
								roleClass = 0;
								break;
							}

							case ( (uint32_t) DBApi::eRoleClass::RoleClassUndefined ):
							{
								std::cout << "Role class cannot be specified as 'Undefined'." << std::endl;
								roleClass = 0;
								break;
							}

							case ( (uint32_t) DBApi::eRoleClass::AllUserClasses ):
							case ( (uint32_t) DBApi::eRoleClass::AllElevatedClasses ):
							case ( (uint32_t) DBApi::eRoleClass::AllAdminClasses ):
							{
								std::cout << "Illegal multi-class class-type. Must be a single class-type(single identifier bit)." << std::endl;
								roleClass = 0;
								break;
							}

							default:
							{
								std::cout << "Illegal out-of-range or undefined role class: must be >= 1 and <= " << ( (int) DBApi::eRoleClass::AllRoleClasses + 1) << std::endl;
								roleClass = 0;
								break;
							}
						}
						std::cout << std::endl;
					} while ( roleClass == 0 );

					new_role.RoleType = (uint16_t) roleClass ;

					promptStr = "Set Instrument permissions? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						uint32_t instPermissions = 0;

						promptStr = "Enter Instrument Permissions bit-field (default = 0xFFFFFFFF): ";
						lengthWarning = "ERROR: permissions field entry cannot be blank!";

						do
						{
							entryStr.clear();

							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							instPermissions = stoul( entryStr );
							if ( instPermissions == 0 )
							{
								std::cout << "ERROR: permissions field entry cannot be 0!" << std::endl;
							}

							std::cout << std::endl;
						} while ( instPermissions == 0 );

						new_role.InstrumentPermissions = instPermissions;
					}

					promptStr = "Set App permissions? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						uint32_t appPermissions = 0;

						promptStr = "Enter Application Permissions bit-field (default = 0xFFFFFFFF): ";
						lengthWarning = "ERROR: permissions field entry cannot be blank!";

						do
						{
							entryStr.clear();

							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							appPermissions = stoul( entryStr );
							if ( appPermissions == 0 )
							{
								std::cout << "ERROR: permissions field entry cannot be 0!" << std::endl;
							}
							std::cout << std::endl;
						} while ( appPermissions == 0 );

						new_role.ApplicationPermissions = appPermissions;
					}

					qResult = DBApi::DbAddUserRole( new_role );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully Added User Role..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add User Role: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				default:
				{
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
				}
			}

			smState_ = smCmdEntry;
			break;
		}

		case smDbInstrumentConfigOptions:
		{
			std::string instSN = "";
			int64_t instCfgIdNum = -1;

			std::vector<DBApi::DB_InstrumentConfigRecord> cfg_list = {};
			DBApi::eQueryResult qResult = DBApi::eQueryResult::NoResults;
			bool process_line = false;

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:
				{
					qResult = DBApi::DbGetInstrumentConfigList( cfg_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved instrument configuration List..." << std::endl;

						print_db_instrument_config_list( cfg_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve instrument config list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						currentInstCfgIdNum = currentCfgRec.InstrumentIdNum;
						currentInstSN = currentCfgRec.InstrumentSNStr;
					}
					else
					{
						currentInstCfgIdNum = -1;
						currentInstSN.clear();
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config( currentCfgRec );
					}
					break;
				}

				case 3:		// create default instrument config record
				{
					DBApi::DB_InstrumentConfigRecord newCfgRec = {};
					DBApi::eQueryResult qResult;
					system_TP zeroTP = {};

					newCfgRec.InstrumentIdNum = 0;
					newCfgRec.InstrumentSNStr = DbDefaultConfigSN;
					newCfgRec.InstrumentType = 1;
					newCfgRec.DeviceName = "Vi-CELL BLU";
					newCfgRec.UIVersion = "";
					newCfgRec.SoftwareVersion = "";
					newCfgRec.AnalysisVersion = "";
					newCfgRec.FirmwareVersion = "";
					newCfgRec.CameraType = 1;
					newCfgRec.CameraFWVersion = "";
					newCfgRec.CameraConfig = "";
					newCfgRec.PumpType = 1;
					newCfgRec.PumpFWVersion = "";
					newCfgRec.PumpConfig = "";

#define MAX_ILLUMINATORS	5
					// all locale tags will use the xx-XX format and the associated LCID
					for ( int i = 0; i < MAX_ILLUMINATORS; i++ )
					{
						DBApi::illuminator_info_t info = {};

						info.index = (int16_t) i;
						switch ( i )
						{
							case 0:		// enumeration for brightfield
							{
								info.type = (int16_t) HawkeyeConfig::LedType::LED_BrightField;
								break;
							}

							case 1:		// enumeration for UV / position TOP_1
							{
								info.type = (int16_t) HawkeyeConfig::LedType::LED_TOP1;
								break;
							}

							case 2:		// enumeration for BLUE / position TOP_1
							{
								info.type = (int16_t) HawkeyeConfig::LedType::LED_BOTTOM1;
								break;
							}

							case 3:		// enumeration for GREEN / position TOP_2
							{
								info.type = (int16_t) HawkeyeConfig::LedType::LED_TOP2;
								break;
							}

							case 4:		// enumeration for RED / position BOTTOM_2
							{
								info.type = (int16_t) HawkeyeConfig::LedType::LED_BOTTOM2;
								break;
							}
						}
						newCfgRec.IlluminatorsInfoList.push_back( info );
					}

					newCfgRec.IlluminatorConfig = "";
					newCfgRec.ConfigType = 0;
					newCfgRec.LogName = "HawkeyeDLL.log";
					newCfgRec.LogMaxSize = 25000000;
					newCfgRec.LogSensitivity = "DBG1";							// limit 16 characters
					newCfgRec.MaxLogs = 100;
					newCfgRec.AlwaysFlush = true;
					newCfgRec.CameraErrorLogName = "CameraErrorLogger.log";	// max 32 characters
					newCfgRec.CameraErrorLogMaxSize = 1000000;
					newCfgRec.StorageErrorLogName = "StorageErrorLogger.log";	// max 32 characters
					newCfgRec.StorageErrorLogMaxSize = 1000000;

					newCfgRec.CarouselThetaHomeOffset = 0;
					newCfgRec.CarouselRadiusOffset = 0;
					newCfgRec.PlateThetaHomeOffset = 0;
					newCfgRec.PlateThetaCalPos = 0;
					newCfgRec.PlateRadiusCenterPos = 0;

					newCfgRec.SaveImage = true;
					newCfgRec.FocusPosition = 0;

					newCfgRec.AF_Settings.save_image = true;
					newCfgRec.AF_Settings.coarse_start = 45000;
					newCfgRec.AF_Settings.coarse_end = 75000;
					newCfgRec.AF_Settings.coarse_step = 300;
					newCfgRec.AF_Settings.fine_range = 2000;
					newCfgRec.AF_Settings.fine_step = 20;
					newCfgRec.AF_Settings.sharpness_low_threshold = 0;

					newCfgRec.AbiMaxImageCount = 10;
					newCfgRec.SampleNudgeVolume = 5;
					newCfgRec.SampleNudgeSpeed = 3;
					newCfgRec.FlowCellDepth = 63.0;
					newCfgRec.FlowCellDepthConstant = 83.0;

					newCfgRec.RfidSim.set_valid_tag_data = true;
					newCfgRec.RfidSim.total_tags = 1;
					newCfgRec.RfidSim.main_bay_file = "C06019_ViCell_BLU_Reagent_Pak.bin";			// limit 64 characters
					newCfgRec.RfidSim.door_left_file = "C06002_ViCell_FLR_Reagent_Pak.bin";		// limit 64 characters
					newCfgRec.RfidSim.door_right_file = "C06001_ViCell_FLR_Reagent_Pak.bin";		// limit 64 characters

					newCfgRec.LegacyData = false;
					newCfgRec.CarouselSimulator = true;
					newCfgRec.NightlyCleanOffset = 120;						// offset from midnight in minutes
					newCfgRec.LastNightlyClean = zeroTP;
					newCfgRec.SecurityMode = 1;								// now an enumeration for none, local, and Ad mode...
					newCfgRec.InactivityTimeout = 60;							// inactivity timeout in minutes
					newCfgRec.PasswordExpiration = 30;							// password expiration in days
					newCfgRec.NormalShutdown = true;
					newCfgRec.TotalSamplesProcessed = 0;
					newCfgRec.DiscardTrayCapacity = 120;

					newCfgRec.Email_Settings.server_addr = "";
					newCfgRec.Email_Settings.port_number = 0;
					newCfgRec.Email_Settings.authenticate = true;
					newCfgRec.Email_Settings.username = "";
					newCfgRec.Email_Settings.pwd_hash = "";

					newCfgRec.AD_Settings.servername = "";
					newCfgRec.AD_Settings.server_addr = "";
					newCfgRec.AD_Settings.port_number = 0;
					newCfgRec.AD_Settings.base_dn = "";
					newCfgRec.AD_Settings.enabled = false;

#define MAX_LANGUAGES 6

					/*
						Language ID as per MSLCID values : currently supported :
						LCID( s )																	Language name					Language tag
						0x00000009( en ), 0x00000409( en-US )										English( United States )		en, en-US
						0x00000007( de ), 0x00000407( de-DE )										German							de, de-DE
						0x0000000A( es ), 0x00000C0A( es-ES ), 0x0000040A( es-ES_tradnl )			Spanish							es, es-ES, es-ES_tradnl
						0x0000000C( fr ), 0x0000040C( fr-FR )										French							fr, fr-FR
						ox00000011( ja ), 0x00000411( ja_JP )										Japanese						ja, ja-JP
						0x00000004( zh-Hans ), 0x00007804( zh )										Chinese( Simplified )			zh-Hans, zh
					*/

					// all locale tags will use the xx-XX format and the associated LCID
					for ( int i = 0; i < MAX_LANGUAGES; i++ )
					{
						DBApi::language_info_t lang = {};

						switch ( i )
						{
							case 0:		// enumeration for English
							{
								lang.active = true;
								lang.language_id = 0x00000409;
								lang.language_name = "English (United States)";
								lang.locale_tag = "en-US";
								break;
							}

							case 1:		// enumeration for German
							{
								lang.active = false;
								lang.language_id = 0x00000407;
								lang.language_name = "German";
								lang.locale_tag = "de-DE";
								break;
							}

							case 2:		// enumeration for Spanish
							{
								lang.active = false;
								lang.language_id = 0x00000C0A;
//								lang.language_id = 0x0000040A;
								lang.language_name = "Spanish";
								lang.locale_tag = "es-ES";
//								lang.locale_tag = "es-ES_tradnl";
								break;
							}

							case 3:		// enumeration for French
							{
								lang.active = false;
								lang.language_id = 0x0000040C;
								lang.language_name = "French";
								lang.locale_tag = "fr-FR";
								break;
							}

							case 4:		// enumeration for Japanese
							{
								lang.active = false;
								lang.language_id = 0x00000411;
								lang.language_name = "Japanese";
								lang.locale_tag = "ja-JP";
								break;
							}

							case 5:		// enumeration for Chinese (simplified)
							{
								lang.active = false;
								lang.language_id = 0x00000004;
								lang.language_name = "Chinese( Simplified )";
								lang.locale_tag = "zh-Hans";
								break;
							}


						}
						newCfgRec.LanguageList.push_back( lang );
					}

					newCfgRec.RunOptions.sample_set_name = "SampleSet";
					newCfgRec.RunOptions.sample_name = "Sample";
					newCfgRec.RunOptions.save_image_count = 100;
					newCfgRec.RunOptions.save_nth_image = 1;
					newCfgRec.RunOptions.results_export = true;
					newCfgRec.RunOptions.results_export_folder = "\\Instrument\\Export";
					newCfgRec.RunOptions.append_results_export = true;
					newCfgRec.RunOptions.append_results_export_folder = "\\Instrument\\Export";
					newCfgRec.RunOptions.result_filename = "Summary";
					newCfgRec.RunOptions.results_folder = "\\Instrument\\Results";
					newCfgRec.RunOptions.auto_export_pdf = false;
					newCfgRec.RunOptions.csv_folder = "\\Instrument\\CSV";
					newCfgRec.RunOptions.wash_type = 0;
					newCfgRec.RunOptions.dilution = 1;
					newCfgRec.RunOptions.bpqc_cell_type_index = 5;

					newCfgRec.AutomationInstalled = false;
					newCfgRec.AutomationEnabled = false;
					newCfgRec.ACupEnabled = false;
					newCfgRec.AutomationPort = 0;

					qResult = DBApi::DbAddInstrumentConfig( newCfgRec );
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully added default instrument configuration..." << std::endl;

						newCfgRec = {};
						GetDbDefaultConfigRec( newCfgRec, instSN, instCfgIdNum, qResult );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add default local instrument configuration: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 4:		// retrieve the default instrument config record
				{
					DBApi::DB_InstrumentConfigRecord cfgRec = {};

					instCfgIdNum = 0;
					instSN.clear();

					GetDbDefaultConfigRec( cfgRec, instSN, instCfgIdNum, qResult );
					break;
				}

				case 5:		// update instrument config record
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						if ( instCfgIdNum == 0 )
						{
							std::cout << "Successfully retrieved default instrument configuration..." << std::endl;
						}
						else
						{
							std::cout << "Successfully retrieved instrument configuration..." << std::endl;
						}

						print_db_instrument_config( currentCfgRec );

						currentInstSN = currentCfgRec.InstrumentSNStr;
						currentInstCfgIdNum = currentCfgRec.InstrumentIdNum;

						currentCfgRec.CarouselThetaHomeOffset = 1850;
						currentCfgRec.CarouselRadiusOffset = 836400;
						currentCfgRec.PlateThetaHomeOffset = 14;
						currentCfgRec.PlateThetaCalPos = 914;
						currentCfgRec.PlateRadiusCenterPos = 18000;

						currentCfgRec.Email_Settings.server_addr = "Test-Email-Server";
						currentCfgRec.Email_Settings.port_number = 12345;
						currentCfgRec.Email_Settings.authenticate = true;
						currentCfgRec.Email_Settings.username = "TestUser";
						currentCfgRec.Email_Settings.pwd_hash = "";

						currentCfgRec.AD_Settings.servername = "Test-AD-Server";
						currentCfgRec.AD_Settings.server_addr = "192.168.0.250";
						currentCfgRec.AD_Settings.port_number = 54321;
						currentCfgRec.AD_Settings.base_dn = "test-domain";
						currentCfgRec.AD_Settings.enabled = false;

						qResult = DBApi::DbModifyInstrumentConfig( currentCfgRec );
						if ( qResult == DBApi::eQueryResult::QueryOk )
						{
							std::cout << "Successfully modified instrument configuration..." << std::endl;

							instSN = currentCfgRec.InstrumentSNStr;
							instCfgIdNum = currentCfgRec.InstrumentIdNum;
							currentCfgRec = {};
							qResult = DBApi::DbFindInstrumentConfig( currentCfgRec, instCfgIdNum, instSN );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully retrieved updated instrument configuration..." << std::endl;
								if ( instCfgIdNum == 0 )
								{
									dfltInstSN = currentCfgRec.InstrumentSNStr;
									dfltInstCfgIdNum = currentCfgRec.InstrumentIdNum;
									defaultCfgRec = currentCfgRec;
								}

								print_db_instrument_config( currentCfgRec );
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to retrieve local instrument configuration: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								currentInstSN.clear();
								currentInstCfgIdNum = 0;
							}
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Error updating instrument config: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve local instrument configuration: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						currentInstSN.clear();
						currentInstCfgIdNum = 0;
						currentCfgRec = {};
					}
					break;
				}

				case 6:		// show Auto-focus settings
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_af_settings( currentCfgRec );
					}
					break;
				}


				case 7:		// show RFID simulation settings
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_rfid_settings( currentCfgRec );
					}
					break;
				}

				case 8:		// show email server ettings
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_email_settings( currentCfgRec );
					}
					break;
				}

				case 9:		// show Active Directory settings
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_ad_settings( currentCfgRec );
					}
					break;
				}

				case 10:		// show Language List with active indicator)
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_lang_list( currentCfgRec );
					}
					break;
				}

				case 11:		// show run options
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						print_db_instrument_config_run_options( currentCfgRec );
					}
					break;
				}

				case 12:		// set automation installed, enabled, and acup states
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						bool automationInstalled = currentCfgRec.AutomationInstalled;
						bool automationEnabled = currentCfgRec.AutomationEnabled;
						bool aCupEnabled = currentCfgRec.ACupEnabled;

						promptStr = "Set install state (t/f):  ";

						entryStr.clear();
						input_quit = GetTrueOrFalseResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 )
						{
							if ( strnicmp( entryStr.c_str(), "T", 1 ) == 0 )
							{
								automationInstalled = true;
							}
							else if	( strnicmp( entryStr.c_str(), "F", 1 ) == 0 )
							{
								automationInstalled = false;
							}
						}

						promptStr = "Set enable state (t/f):  ";

						entryStr.clear();
						input_quit = GetTrueOrFalseResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 )
						{
							if ( strnicmp( entryStr.c_str(), "T", 1 ) == 0 )
							{
								automationEnabled = true;
							}
							else if ( strnicmp( entryStr.c_str(), "F", 1 ) == 0 )
							{
								automationEnabled = false;
							}
						}

						promptStr = "Set ACup enable state (t/f):  ";

						entryStr.clear();
						input_quit = GetTrueOrFalseResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 )
						{
							if ( strnicmp( entryStr.c_str(), "T", 1 ) == 0 )
							{
								aCupEnabled = true;
							}
							else if ( strnicmp( entryStr.c_str(), "F", 1 ) == 0 )
							{
								aCupEnabled = false;
							}
						}

						if ( currentCfgRec.AutomationInstalled != automationInstalled ||
							 currentCfgRec.AutomationEnabled != automationEnabled ||
							 currentCfgRec.ACupEnabled != aCupEnabled )
						{
							currentCfgRec.AutomationInstalled = automationInstalled;
							currentCfgRec.AutomationEnabled = automationEnabled;
							currentCfgRec.ACupEnabled = aCupEnabled;

							qResult = DBApi::DbModifyInstrumentConfig( currentCfgRec );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully modified instrument configuration..." << std::endl;

								instCfgIdNum = currentCfgRec.InstrumentIdNum;
								instSN = currentCfgRec.InstrumentSNStr;
								currentCfgRec = {};

								qResult = DBApi::DbFindInstrumentConfig( currentCfgRec, instCfgIdNum, instSN );
								if ( qResult == DBApi::eQueryResult::QueryOk )
								{
									std::cout << "Successfully retrieved updated instrument configuration..." << std::endl;
									currentInstCfgIdNum = currentCfgRec.InstrumentIdNum;
									currentInstSN = currentCfgRec.InstrumentSNStr;
									if ( currentInstCfgIdNum == 0 )
									{
										dfltInstSN = currentCfgRec.InstrumentSNStr;
										dfltInstCfgIdNum = currentCfgRec.InstrumentIdNum;
										defaultCfgRec = currentCfgRec;
									}
								}
								else
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to retrieve local instrument configuration: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									currentInstSN.clear();
									currentInstCfgIdNum = -1;
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error updating instrument config: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
						print_db_instrument_config( currentCfgRec );
					}
					break;
				}

				case 13:		// set automation port
				{
					if ( currentInstCfgIdNum < 0 )
					{
						instCfgIdNum = 0;
						instSN.clear();
					}
					else
					{
						instCfgIdNum = currentInstCfgIdNum;
						instSN = currentInstSN;
					}

					input_quit = SelectDbConfigRec( cfg_list, currentCfgRec, instSN, instCfgIdNum, qResult );
					if ( input_quit )
					{
						return;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						entryStr.clear();
						promptStr = "Enter port number: ";

						GetStringInput( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 )
						{
							currentCfgRec.AutomationPort = stol( entryStr );

							qResult = DBApi::DbModifyInstrumentConfig( currentCfgRec );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully modified instrument configuration..." << std::endl;

								instCfgIdNum = currentCfgRec.InstrumentIdNum;
								instSN = currentCfgRec.InstrumentSNStr;
								currentCfgRec = {};

								qResult = DBApi::DbFindInstrumentConfig( currentCfgRec, instCfgIdNum, instSN );
								if ( qResult == DBApi::eQueryResult::QueryOk )
								{
									std::cout << "Successfully retrieved updated instrument configuration..." << std::endl;

									currentInstCfgIdNum = currentCfgRec.InstrumentIdNum;
									currentInstSN = currentCfgRec.InstrumentSNStr;
									if ( currentInstCfgIdNum == 0 )
									{
										dfltInstSN = currentCfgRec.InstrumentSNStr;
										dfltInstCfgIdNum = currentCfgRec.InstrumentIdNum;
										defaultCfgRec = currentCfgRec;
									}

									print_db_instrument_config( currentCfgRec );
								}
								else
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to retrieve local instrument configuration: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									currentInstSN.clear();
									currentInstCfgIdNum = 0;
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error updating instrument config: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbLogOptions:
		{
			DBApi::eQueryResult qResult = DBApi::eQueryResult::NoResults;
			std::vector<DBApi::eListFilterCriteria> filtertypelist = {};
			std::vector<std::string> filtercomparelist = {};
			std::vector<std::string> filtertgtlist = {};
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
			int32_t sortdir = -1;
			std::string orderstr = "";
			int32_t limitcnt = 0;
			int32_t startindex = -1;
			int64_t startidnum = -1;
			size_t listSize = 0;
			int32_t listIdx = 0;
			int64_t listIdNum = 0;
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
			std::string filtercompare = "=";
			std::string filtertgt = "";
			DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
			std::vector<DBApi::DB_LogEntryRecord> log_list = {};
			DBApi::DB_LogEntryRecord logRec = {};
			DBApi::eLogEntryType log_type = DBApi::eLogEntryType::NoLogType;
			CommandParser commandParser_;
			bool recFound = false;

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:			// ListAuditLog 
				{
					log_type = DBApi::eLogEntryType::AuditLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbGetAuditLogList( log_list, filtertypelist, filtercomparelist, filtertgtlist,
														limitcnt, primarysort, secondarysort, sortdir );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Audit Log List..." << std::endl;

						print_db_log_lists( log_list, log_type );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Audit log list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:			// AddAuditLogEntry
				{
					system_TP zeroTP = {};
					bool process_line = false;

					logRec = {};
					promptStr = "Enter Audit Log Entry Text: ";
					lengthWarning = "ERROR: Log Entry cannot be blank!";

					logRec.EntryType = static_cast<int16_t>(DBApi::eLogEntryType::AuditLogType);
					logRec.EntryDate = zeroTP;
					logRec.EntryStr = "Audit Log Entry: ";

					entryStr.clear();
					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					logRec.EntryStr.append( entryStr );

					qResult = DBApi::DbAddAuditLogEntry( logRec );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully added Audit Log entry..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add Audit log entry: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 3:			// Update audit log entry
				{
					log_type = DBApi::eLogEntryType::AuditLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbGetAuditLogList( log_list, filtertypelist, filtercomparelist, filtertgtlist,
														limitcnt, primarysort, secondarysort, sortdir );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Audit Log List..." << std::endl;

						print_db_log_lists( log_list, log_type );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Audit log list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						break;
					}

					listSize = log_list.size();
					if ( listSize <= 0 )
					{
						std::cout << "No audit log entries!" << std::endl;
						break;
					}

					listIdNum = -1;
					input_quit = SelectDbListIdNum( listSize, listIdNum );
					if ( input_quit )
					{
						return;
					}

					if ( listIdNum < 0 )
					{
						std::cout << "No audit log entry idnumn selected!" << std::endl;
						break;
					}

					recFound = false;
					listIdx = 0;
					do
					{
						logRec = log_list.at( listIdx );
						if ( logRec.IdNum == listIdNum )
						{
							recFound = true;
						}
					} while ( recFound == false && ++listIdx < listSize );

					if ( !recFound )
					{
						std::cout << "Selected entry idnum not found in list!" << std::endl;
						break;
					}

					if ( !commandParser_.parse( "|", logRec.EntryStr ) )
					{
						std::cout << "Failed to parse log entry :" << logRec.EntryStr << std::endl;
						break;
					}

					if ( commandParser_.size() < 5 )
					{
						std::cout << "Invalid log entry field count:" << std::to_string( commandParser_.size() ) << std::endl;
						break;
					}

					int32_t event_type = std::stol( commandParser_.getByIndex( 3 ) );
					int32_t alteredEvent_type = event_type + 2;

					std::string event_message = commandParser_.getByIndex( 4 );
					size_t msgSize = event_message.size();
					// Remove line break from event message
					if ( msgSize > 0 && event_message[msgSize - 1] == '\n' )
					{
						event_message.pop_back();
					}
					std::string alteredMsg = "Altered event description: ";
					alteredMsg.append( event_message );

					std::string additionalInfo = {};
					if ( commandParser_.size() > 5 )
					{
						additionalInfo = commandParser_.getByIndex( 5 );
					}

					std::string alteredEntry = boost::str( boost::format( "%s|%s|%s|%ld|%s|%s" ) % commandParser_.getByIndex( 0 ) % commandParser_.getByIndex( 1 ) % commandParser_.getByIndex( 2 ) % alteredEvent_type % alteredMsg % additionalInfo);
					logRec.EntryStr = alteredEntry;
					log_list.at( listIdx ) = logRec;

					qResult = DBApi::DbModifyAuditLogEntry(logRec);
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully updated Audit Log entry..." << std::endl;

						qResult = DBApi::DbGetAuditLogList( log_list, filtertypelist, filtercomparelist, filtertgtlist,
															limitcnt, primarysort, secondarysort, sortdir );

						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved Audit Log List..." << std::endl;

							print_db_log_lists( log_list, log_type );
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Failed to retrieve Audit log list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							break;
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to modify Audit log entry: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 4:			// ClearAuditLog
				{
					log_type = DBApi::eLogEntryType::AuditLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbClearAuditLog( filtertypelist, filtercomparelist, filtertgtlist );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully cleared Audit Log..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to clear Audit log: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					break;
				}

				case 5:			// ListErrorLog
				{
					log_type = DBApi::eLogEntryType::ErrorLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbGetErrorLogList( log_list, filtertypelist, filtercomparelist, filtertgtlist,
														limitcnt, primarysort, secondarysort, sortdir );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Error Log List..." << std::endl;

						print_db_log_lists( log_list, log_type );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Error log list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 6:			// AddErrorLogEntry
				{
					system_TP zeroTP = {};
					bool process_line = false;

					logRec = {};
					promptStr = "Enter Error Log Entry Text: ";
					lengthWarning = "ERROR: Log Entry cannot be blank!";

					logRec.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType );
					logRec.EntryDate = zeroTP;
					logRec.EntryStr = "Error Log Entry: ";

					entryStr.clear();
					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					logRec.EntryStr.append( entryStr );

					qResult = DBApi::DbAddErrorLogEntry( logRec );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully added Error Log entry..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add Error log entry: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 7:			// Update error log entry
				{
					std::cout << "Not implemented" << std::endl;
					break;
				}

				case 8:			// ClearErrorLog
				{
					log_type = DBApi::eLogEntryType::ErrorLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbClearErrorLog( filtertypelist, filtercomparelist, filtertgtlist );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully cleared Error Log..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to clear Error log: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					break;
				}

				case 9:			// ListSampleLog
				{
					log_type = DBApi::eLogEntryType::SampleLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbGetSampleLogList( log_list, filtertypelist, filtercomparelist, filtertgtlist,
														limitcnt, primarysort, secondarysort, sortdir );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Sample Log List..." << std::endl;

						print_db_log_lists( log_list, log_type );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Sample log list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 10:			//AddSampleLogEntry(6)
				{
					system_TP zeroTP = {};
					bool process_line = false;

					logRec = {};
					promptStr = "Enter Sample Log Entry Text: ";
					lengthWarning = "ERROR: Log Entry cannot be blank!";

					logRec.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType );
					logRec.EntryDate = zeroTP;
					logRec.EntryStr = "Sample Log Entry: ";

					entryStr.clear();
					input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
					if ( input_quit )
					{
						return;
					}

					logRec.EntryStr.append( entryStr );

					qResult = DBApi::DbAddSampleLogEntry( logRec );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully added Sample Log entry..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add Sample log entry: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 11:			// Update sample log entry
				{
					std::cout << "Not implemented" << std::endl;
					break;
				}

				case 12:			// ClearSampleLog
				{
					log_type = DBApi::eLogEntryType::SampleLogType;
					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					qResult = DBApi::DbClearSampleLog( filtertypelist, filtercomparelist, filtertgtlist );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully cleared Sample Log..." << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to clear Sample log: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbObjAdditionOptions:
		{
			DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
			std::vector<DBApi::eListFilterCriteria> filtertypelist;
			std::vector<std::string> filtercomparelist;
			std::vector<std::string> filtertgtlist;
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
			int32_t sortdir = -1;
			std::string orderstr = "";
			int32_t listIdx = 0;
			size_t listSize = 0;
			int32_t limitcnt = 0;
			int32_t startindex = -1;
			int64_t startidnum = -1;
			int64_t listIdNum = 0;
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
			bool recFound = false;
			bool process_line = false;

			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:
				{
					DBApi::DB_WorklistRecord wlRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 2:
				{
					DBApi::DB_SampleSetRecord ssRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 3:
				{
					DBApi::DB_SampleItemRecord siRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 4:
				{
					DBApi::DB_SampleRecord smRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 5:
				{
					DBApi::DB_AnalysisRecord anRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 6:
				{
					DBApi::DB_SummaryResultRecord srRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 7:
				{
					DBApi::DB_DetailedResultRecord drRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 8:
				{
					DBApi::DB_CellTypeRecord ctRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 9:
				{
					DBApi::DB_SignatureRecord sigRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 10:
				{
					DBApi::DB_QcProcessRecord qcRec = {};
					HawkeyeUUID tgtId = HawkeyeUUID( "f207e97c851a42e18c2938721cb15d91" );

					qcRec.QcIdNum = 0;
					qcRec.QcId = {};
					qcRec.Protected = false;
					qcRec.QcName = "TestQcName";
					qcRec.QcType = 1;
					tgtId.get_uuid__t(qcRec.CellTypeId);
					qcRec.CellTypeIndex = 8;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
					qcRec.LotInfo = "TestQcLotNumber";
					qcRec.LotExpiration = ChronoUtilities::ConvertToString(ChronoUtilities::CurrentTime());
					qcRec.AssayValue = 10.0000;
					qcRec.AllowablePercentage = 1.00;
					qcRec.QcSequence = "";
					qcRec.Comments = "Testing Qc Insertion";

					qResult = DBApi::DbAddQcProcess( qcRec );
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully added QcProcess record" << std::endl;
					}
					else if ( qResult == DBApi::eQueryResult::InsertObjectExists )
					{
						std::cout << "Existing object found with matching UUID" << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add Qc process record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 11:
				{
					std::vector<DBApi::DB_ReagentTypeRecord> rec_list;
					DBApi::DB_ReagentTypeRecord rxRec = {};

					qResult = DBApi::DbGetReagentInfoList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Reagent info List..." << std::endl;
						print_db_reagent_info_list( rec_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Reagent info list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					size_t listSize = rec_list.size();

					rxRec.ReagentIdNum = 0;
					rxRec.ReagentTypeNum = 0;

					rxRec.Current = false;
					rxRec.PackPartNumStr = "BEC-P/N-12345667890";
					rxRec.LotNumStr = "TEST-10-13-2020";
					system_TP curTpt = ChronoUtilities::CurrentTime();
					int64_t curDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>( curTpt ) / SECONDSPERDAY;
					std::string curDateStr = ChronoUtilities::ConvertToDateString( curDate );
					boost::to_upper( curDateStr );
					system_TP expTpt = curTpt + duration_days( 180 );
					rxRec.LotExpirationDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>( expTpt ) / SECONDSPERDAY;
					system_TP serviceTpt = curTpt + duration_days( 90 );
					rxRec.InServiceDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>( serviceTpt ) / SECONDSPERDAY;
					rxRec.InServiceExpirationLength = 90;
					rxRec.Protected = false;

					rxRec.ContainerTagSn = boost::str( boost::format( "RXTEST-%s-0102030405-%d" ) % curDateStr % ( listSize + 1 ) );

					rxRec.ReagentIndexList.clear();
					rxRec.ReagentNamesList.clear();
					rxRec.MixingCyclesList.clear();

					for ( int i = 0; i < 4; i++ )
					{
						switch ( i )
						{
							rxRec.ReagentIndexList.push_back( i + 1 );
							case 0:
							{
								rxRec.ReagentNamesList.push_back( "Cleaning Agent" );
								rxRec.MixingCyclesList.push_back( 1 );
								break;
							}

							case 1:
							{
								rxRec.ReagentNamesList.push_back( "Conditioning Solution" );
								rxRec.MixingCyclesList.push_back( 1 );
								break;
							}

							case 2:
							{
								rxRec.ReagentNamesList.push_back( "Buffer Solution" );
								rxRec.MixingCyclesList.push_back( 1 );
								break;
							}

							case 3:
							{
								rxRec.ReagentNamesList.push_back( "Trypan Blue" );
								rxRec.MixingCyclesList.push_back( 3 );
								break;
							}
						}
					}

					qResult = DBApi::DbAddReagentInfo( rxRec );
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully added Reagent info record" << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add Reagent info record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						rec_list.clear();
						qResult = DBApi::DbGetReagentInfoList( rec_list );
						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved Reagent info List..." << std::endl;

							print_db_reagent_info_list( rec_list );
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Failed to retrieve Reagent info list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						}
					}
					break;
				}

				case 12:
				{
					std::vector<DBApi::DB_SchedulerConfigRecord> rec_list;
					DBApi::DB_SchedulerConfigRecord scfgRec = {};

					size_t listSize = rec_list.size();

					input_quit = EnterSchedulerConfig( scfgRec, true );
					if ( input_quit )
					{
						return;
					}

					qResult = DBApi::DbAddSchedulerConfig( scfgRec );
					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::cout << "Successfully added SchedulerConfig record" << std::endl;
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to add SchedulerConfig record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						rec_list.clear();
						qResult = DBApi::DbGetSchedulerConfigList( rec_list );
						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved SchedulerConfig List..." << std::endl;
							print_db_scheduler_config_list( rec_list );
						}
						else
						{
							DBApi::GetQueryResultString( qResult, queryResultStr );
							std::cout << "Failed to retrieve SchedulerConfig list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
						}
					}

					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbObjUpdateOptions:
		{
			DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
			std::vector<DBApi::eListFilterCriteria> filtertypelist = {};
			std::vector<std::string> filtercomparelist = {};
			std::vector<std::string> filtertgtlist = {};
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
			int32_t sortdir = 1;
			std::string orderstr = "";
			int32_t listIdx = 0;
			size_t listSize = 0;
			int32_t limitcnt = -2;
			int32_t startindex = -1;
			int64_t startidnum = -1;
			int64_t listIdNum = 0;
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
			std::string compareop = "";
			std::string compareval = "";
			bool recFound = false;
			bool process_line = false;

			switch ( atoi( smValueLine_.c_str() ) )
			{
				listIdx = 0;
				listSize = 0;
				recFound = false;

				case 1:
				{
					std::vector<DBApi::DB_WorklistRecord> rec_list;
					DBApi::DB_WorklistRecord wlRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 2:
				{
					std::vector<DBApi::DB_SampleSetRecord> rec_list;
					DBApi::DB_SampleSetRecord ssRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 3:
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;
					DBApi::DB_SampleItemRecord siRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 4:
				{
					std::vector<DBApi::DB_SampleRecord> rec_list;
					DBApi::DB_SampleRecord smRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 5:
				{
					std::vector<DBApi::DB_AnalysisRecord> rec_list;
					DBApi::DB_AnalysisRecord anRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 6:
				{
					std::vector<DBApi::DB_SummaryResultRecord> rec_list;
					DBApi::DB_SummaryResultRecord srRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 7:
				{
					std::vector<DBApi::DB_DetailedResultRecord> rec_list;
					DBApi::DB_DetailedResultRecord drRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 8:
				{
					std::vector<DBApi::DB_CellTypeRecord> rec_list;
					DBApi::DB_CellTypeRecord ctRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 9:
				{
					std::vector<DBApi::DB_SignatureRecord> rec_list;
					DBApi::DB_SignatureRecord sigRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 10:
				{
					std::vector<DBApi::DB_QcProcessRecord> rec_list;
					DBApi::DB_QcProcessRecord qcRec = {};
					HawkeyeUUID tgtId = HawkeyeUUID( "f207e97c851a42e18c2938721cb15d91" );

					qResult = DBApi::DbGetQcProcessList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved QC Process List..." << std::endl;

						print_db_qc_process_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									qcRec = rec_list.at( listIdx );
									if ( qcRec.QcIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								if ( recFound )
								{
									/*
									// apply any updates/changes now...
									qcRec.Protected;
									qcRec.QcName;
									qcRec.QcType;
									tgtId.get_uuid__t( qcRec.CellTypeId );
									qcRec.CellTypeIndex;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
									qcRec.LotInfo = "TestQcLotNumber";
									qcRec.LotExpiration = ChronoUtilities::ConvertToString( ChronoUtilities::CurrentTime() );
									qcRec.AssayValue = 10.0000;
									qcRec.AllowablePercentage = 1.00;
									qcRec.QcSequence = "";
									qcRec.Comments = "Testing Qc Insertion";
									*/
									qcRec.LotInfo = "TestQcLotNumber2";
									qcRec.Comments = "Now Testing Qc Update";

									qResult = DBApi::DbModifyQcProcess( qcRec );
									if ( qResult == DBApi::eQueryResult::QueryOk )
									{
										std::cout << "Successfully updated Qc process record" << std::endl;

										qResult = DBApi::DbGetQcProcessList( rec_list );
										if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
										{
											std::cout << "Successfully retrieved QC Process List..." << std::endl;

											print_db_qc_process_list( rec_list );
										}
									}
									else
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Failed to update Qc process record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									}
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve QC process list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 11:
				{
					std::vector<DBApi::DB_ReagentTypeRecord> rec_list;
					DBApi::DB_ReagentTypeRecord rxRec = {};
					DBApi::DB_ReagentTypeRecord rxCurrent = {};

					qResult = DBApi::DbGetReagentInfoList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Reagent info List..." << std::endl;

						print_db_reagent_info_list( rec_list );

						bool currentFound = false;
						int currentCnt = 0;

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									rxRec = rec_list.at( listIdx );
									if ( rxRec.ReagentIdNum == listIdNum )
									{
										recFound = true;
									}

									if ( rxRec.Current == true )
									{
										currentFound = true;
										currentCnt++;
										rxCurrent = rxRec;
									}
								} while ( recFound == false && ++listIdx < listSize );

								if ( recFound )
								{
									if ( currentFound )
									{
										if ( currentCnt > 1 )
										{
											int32_t currentIdx = 0;
											do
											{
												rxRec = rec_list.at( currentIdx );

												if ( rxRec.Current == true )
												{
													rxRec.Current = false;
													qResult = DBApi::DbModifyReagentInfo( rxRec );
												}
											} while ( ++currentIdx < listSize );
										}
										rxCurrent.Current = false;

										qResult = DBApi::DbModifyReagentInfo( rxCurrent );
										if ( qResult == DBApi::eQueryResult::QueryOk )
										{
											std::cout << "Successfully updated previous current Reagent info record" << std::endl;
										}
										else
										{
											DBApi::GetQueryResultString( qResult, queryResultStr );
											std::cout << "Failed to update previous current Reagent info record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										}
									}

									rxRec.Current = true;

									qResult = DBApi::DbModifyReagentInfo( rxRec );
									if ( qResult == DBApi::eQueryResult::QueryOk )
									{
										std::cout << "Successfully updated Reagent info record" << std::endl;
									}
									else
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Failed to update Reagent info record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									}

									if ( qResult == DBApi::eQueryResult::QueryOk )
									{
										rec_list.clear();
										qResult = DBApi::DbGetReagentInfoList( rec_list );
										if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
										{
											std::cout << "Successfully retrieved Reagent info List..." << std::endl;

											print_db_reagent_info_list( rec_list );
										}
										else
										{
											DBApi::GetQueryResultString( qResult, queryResultStr );
											std::cout << "Failed to retrieve Reagent info list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										}
									}
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Reagent info list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 12:
				{
					std::vector<DBApi::DB_SchedulerConfigRecord> rec_list;
					DBApi::DB_SchedulerConfigRecord scfgRec = {};

					sortdir = 1;
					orderstr.clear();
					qResult = DBApi::DbGetSchedulerConfigList( rec_list,
															   filtertype, compareop, compareval,
															   limitcnt, primarysort, secondarysort,
															   sortdir, orderstr, -1, -1 );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SchedulerConfig List..." << std::endl;
						print_db_scheduler_config_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									scfgRec = rec_list.at( listIdx );
									if ( scfgRec.ConfigIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								if ( recFound )
								{
									input_quit = EnterSchedulerConfig( scfgRec, false );
									if ( input_quit )
									{
										return;
									}

									qResult = DBApi::DbModifySchedulerConfig( scfgRec );
									if ( qResult == DBApi::eQueryResult::QueryOk )
									{
										std::cout << "Successfully updated SchedulerConfig record" << std::endl;

										rec_list.clear();
										qResult = DBApi::DbGetSchedulerConfigList( rec_list );
										if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
										{
											std::cout << "Successfully retrieved SchedulerConfig List..." << std::endl;
											print_db_scheduler_config_list( rec_list );
										}
										else
										{
											DBApi::GetQueryResultString( qResult, queryResultStr );
											std::cout << "Failed to retrieve SchedulerConfig list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										}
									}
									else
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Failed to update SchedulerConfig record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									}
								}
								else
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to find selected SchedulerConfig record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SchedulerConfig list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					size_t listSize = rec_list.size();

					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
					break;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbObjRemovalOptions:
		{
			DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
			std::vector<DBApi::eListFilterCriteria> filtertypelist;
			std::vector<std::string> filtercomparelist;
			std::vector<std::string> filtertgtlist;
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
			int32_t sortdir = -1;
			std::string orderstr = "";
			bool process_line = false;
			int32_t listIdx = 0;
			size_t listSize = 0;
			int32_t limitcnt = 0;
			int32_t startindex = -1;
			int64_t startidnum = -1;
			int64_t listIdNum = 0;
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
			bool recFound = false;

#define MAX_WL_GEN	5
#define MAX_SS_GEN	5
#define MAX_SI_GEN	5
#define MAX_SEQ_GEN	5

			switch ( atoi( smValueLine_.c_str() ) )
			{
				listIdx = 0;
				listSize = 0;
				recFound = false;

				case 1:			// remove Worklist record
				{
					std::vector<DBApi::DB_WorklistRecord> rec_list;
					DBApi::DB_WorklistRecord wlRec = {};

					qResult = DBApi::DbGetWorklistList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved worklist List..." << std::endl;

						print_db_worklist_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									wlRec = rec_list.at( listIdx );
									if ( wlRec.WorklistIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveWorklist( wlRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove worklist record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve worklist list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:			// remove Worklist record list
				{
					std::vector<DBApi::DB_WorklistRecord> rec_list;
					size_t wlListSize = 0;

					DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
					std::string filtercompare = "=";
					std::string filtertgt = "0";
					DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::AllSubObjs;
					int32_t limitcnt = 1000;

					qResult = DBApi::DbGetWorklistList( rec_list,filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						// no records found; generate a list to delete...
						int wlIdx = 0;

						for ( wlIdx = 0; wlIdx < MAX_WL_GEN; wlIdx++ )
						{
							DBApi::DB_WorklistRecord insertRec = {};
							int well = 1;
							int ssIdx = 0;

							insertRec.WorklistIdNum = 0;
							insertRec.WorklistId = {};
							insertRec.WorklistStatus = 0;
							insertRec.WorklistNameStr = boost::str( boost::format( "Worklist Deletion Test Worklist %d" ) % ( wlIdx + 1 ) );
							insertRec.ListComments = "Testing Worklist deletion";
							insertRec.InstrumentSNStr = "WLTestInstrument";
							insertRec.CreationUserId = {};
							insertRec.CreationUserNameStr = "WLTestUser";
							insertRec.RunUserId = {};
							insertRec.RunUserNameStr = "WLTestRunUser";
							insertRec.RunDateTP = {};
							insertRec.AcquireSample = true;
							insertRec.CarrierType = 0;
							insertRec.ByColumn = false;
							insertRec.SaveNthImage = 1;
							insertRec.WashTypeIndex = 0;
							insertRec.Dilution = 1;
							insertRec.SampleSetNameStr = "Default SampleSet name";
							insertRec.SampleItemNameStr = "Default SampleItem name";
							insertRec.ImageAnalysisParamId = {};
							insertRec.AnalysisDefId = {};
							insertRec.AnalysisDefIndex = 0;
							insertRec.AnalysisParamId = {};
							insertRec.CellTypeId = {};
							insertRec.CellTypeIndex = 0;
							insertRec.BioProcessId = {};
							insertRec.BioProcessNameStr = "Default BioProcess name";
							insertRec.QcProcessId = {};
							insertRec.QcProcessNameStr = "Default QcProcess name";
							insertRec.WorkflowId = {};
							insertRec.SampleSetCount = 0;
							insertRec.ProcessedSetCount = 0;

							for ( ssIdx = 0; ssIdx < MAX_SS_GEN; ssIdx++ )
							{
								DBApi::DB_SampleSetRecord ssRec = {};
								int siIdx = 0;

								ssRec.SampleSetIdNum = 0;
								ssRec.SampleSetId = {};
								ssRec.SampleSetStatus = 0;
								ssRec.SampleSetNameStr = boost::str( boost::format( "WL - Test SampleSet%d" ) % ( ssIdx + 1 ) );
								ssRec.SampleSetLabel = boost::str( boost::format( "WL - SampleSet%d Label" ) % ( ssIdx + 1 ) );
								ssRec.Comments = boost::str( boost::format( "WL - SampleSet%d comment" ) % ( ssIdx + 1 ) );
								ssRec.CarrierType = 0;
								ssRec.OwnerId = {};
								ssRec.OwnerNameStr = boost::str( boost::format( "WL - SSTestUser%d" ) % ( ssIdx + 1 ) );
								ssRec.CreateDateTP = {};
								ssRec.ModifyDateTP = {};
								ssRec.RunDateTP = {};
								ssRec.WorklistId = {};
								ssRec.ProcessedItemCount = 0;
								ssRec.SSItemsList.clear();

								for ( siIdx = 0; siIdx < MAX_SI_GEN; siIdx++ )
								{
									DBApi::DB_SampleItemRecord siRec = {};

									siRec.SampleItemIdNum = 0;
									siRec.SampleItemId = {};
									siRec.SampleItemStatus = 0;
									siRec.SampleItemNameStr = "Worklist Removal SampleItem name";
									siRec.SampleItemNameStr = boost::str( boost::format( "Deletion Test SampleItem %d - SampleSet %d - Worklist &d" ) % ( siIdx + 1 ) % ( ssIdx + 1 ) % ( wlIdx + 1 ) );
									siRec.Comments = "Worklist - SampleItem removal test";
									siRec.RunDateTP = {};
									siRec.SampleSetId = {};
									siRec.SampleId = {};
									siRec.OwnerId = {};
									siRec.OwnerNameStr = boost::str( boost::format( "WL - SITestUser%d" ) % ( ssIdx + 1 ) );
									siRec.SaveNthImage = insertRec.SaveNthImage;
									siRec.WashTypeIndex = insertRec.WashTypeIndex;
									siRec.Dilution = insertRec.Dilution;
									siRec.ItemLabel = "Worklist SampleItem removal test";
									siRec.ImageAnalysisParamId = {};
									siRec.AnalysisDefId = {};
									siRec.AnalysisDefIndex = insertRec.AnalysisDefIndex;
									siRec.AnalysisDef = {};
									siRec.AnalysisParamId = {};
									siRec.AnalysisParam = {};
									siRec.CellTypeId = {};
									siRec.CellTypeIndex = insertRec.CellTypeIndex;
									siRec.CellType = {};
									siRec.BioProcessId = {};
									siRec.BioProcessNameStr = "Worklist SampleItem Removal Test BioProcess name";
									siRec.QcProcessId = {};
									siRec.QcProcessNameStr = "Worklist SampleItem Removal Test QcProcess name";
									siRec.WorkflowId = {};
									siRec.SampleRow = 'Z';
									siRec.SampleCol = well++;
									siRec.RotationCount = 0;

									ssRec.SSItemsList.push_back( siRec );
								}
								ssRec.SampleItemCount = (uint16_t) siIdx;

								insertRec.SSList.push_back( ssRec );
							}
							insertRec.SampleSetCount = (int16_t) ssIdx;

							qResult = DBApi::DbAddWorklist( insertRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding test image records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								wlIdx = 0;
								break;
							}
						}

						if ( wlIdx > 0 )
						{
							qResult = DBApi::DbGetWorklistList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved worklist List..." << std::endl;

						print_db_worklist_list( rec_list );

						std::vector<DBApi::DB_SampleSetRecord> ssrec_list;

						qResult = DBApi::DbGetSampleSetList( ssrec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
						size_t ssCnt = ssrec_list.size();
						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved sample-set List..." << std::endl;

							print_db_sampleset_list( ssrec_list );
						}

						std::vector<DBApi::DB_SampleItemRecord> sirec_list;

						qResult = DBApi::DbGetSampleItemList( sirec_list, filtertype, filtercompare, filtertgt, limitcnt );
						size_t siCnt = sirec_list.size();
						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved sample-set List..." << std::endl;

							print_db_sampleitem_list( sirec_list );
						}

						std::vector<uuid__t> id_list;

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all worklists? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->WorklistId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveWorklists( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed Worklists using uuid list" << std::endl;

								rec_list.clear();
								qResult = DBApi::DbGetWorklistList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
								if ( qResult == DBApi::eQueryResult::QueryOk )
								{
									print_db_worklist_list( rec_list );
								}

								ssrec_list.clear();
								qResult = DBApi::DbGetSampleSetList( ssrec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved sample-set List...";

									print_db_sampleset_list( ssrec_list );
									if ( ssCnt != ssrec_list.size() )
									{
										std::cout << "... but some sample set records were deleted...";
									}
									std::cout << std::endl;
								}

								sirec_list.clear();
								qResult = DBApi::DbGetSampleItemList( sirec_list, filtertype, filtercompare, filtertgt, limitcnt );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved sample-item List...";

									print_db_sampleitem_list( sirec_list );

									if ( siCnt != sirec_list.size() )
									{
										std::cout << "... but some sample item records were deleted...";
									}
									std::cout << std::endl;
								}

							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove Worklists using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve worklist list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 3:			// remove SampleSet record
				{
					std::vector<DBApi::DB_SampleSetRecord> rec_list;
					DBApi::DB_SampleSetRecord ssRec = {};

					qResult = DBApi::DbGetSampleSetList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample-set List..." << std::endl;

						print_db_sampleset_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									ssRec = rec_list.at( listIdx );
									if ( ssRec.SampleSetIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveSampleSet( ssRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove sample-set record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve sample-set list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 4:			// remove SampleSet Record List
				{
					std::vector<DBApi::DB_SampleSetRecord> rec_list;

					DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
					std::string filtercompare = "=";
					std::string filtertgt = "0";
					DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::AllSubObjs;
					int32_t limitcnt = 1000;

					qResult = DBApi::DbGetSampleSetList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						// no records found; generate a list to delete...
						DBApi::DB_WorklistRecord insertRec = {};
						int wlIdx = 0;

						for ( wlIdx = 0; wlIdx < MAX_WL_GEN; wlIdx++ )
						{
							int well = 1;
							int ssIdx = 0;

							insertRec.WorklistIdNum = 0;
							insertRec.WorklistId = {};
							insertRec.WorklistStatus = 0;
							insertRec.WorklistNameStr = boost::str( boost::format( "SampleSet Deletion Test Worklist %d" ) % ( wlIdx + 1 ) );
							insertRec.ListComments = "Testing SampleSet deletion";
							insertRec.InstrumentSNStr = "SSTestInstrument";
							insertRec.CreationUserId = {};
							insertRec.CreationUserNameStr = "SSTestUser";
							insertRec.RunUserId = {};
							insertRec.RunUserNameStr = "SSTestRunUser";
							insertRec.RunDateTP = {};
							insertRec.AcquireSample = true;
							insertRec.CarrierType = 0;
							insertRec.ByColumn = false;
							insertRec.SaveNthImage = 1;
							insertRec.WashTypeIndex = 0;
							insertRec.Dilution = 1;
							insertRec.SampleSetNameStr = "Default SampleSet name";
							insertRec.SampleItemNameStr = "Default SampleItem name";
							insertRec.ImageAnalysisParamId = {};
							insertRec.AnalysisDefId = {};
							insertRec.AnalysisDefIndex = 0;
							insertRec.AnalysisParamId = {};
							insertRec.CellTypeId = {};
							insertRec.CellTypeIndex = 0;
							insertRec.BioProcessId = {};
							insertRec.BioProcessNameStr = "Default BioProcess name";
							insertRec.QcProcessId = {};
							insertRec.QcProcessNameStr = "Default QcProcess name";
							insertRec.WorkflowId = {};
							insertRec.SampleSetCount = 0;
							insertRec.ProcessedSetCount = 0;

							for ( ssIdx = 0; ssIdx < MAX_SS_GEN; ssIdx++ )
							{
								DBApi::DB_SampleSetRecord ssRec = {};
								int siIdx = 0;

								ssRec.SampleSetIdNum = 0;
								ssRec.SampleSetId = {};
								ssRec.SampleSetStatus = 0;
								ssRec.SampleSetNameStr = boost::str( boost::format( "SS - Test SampleSet%d" ) % ( ssIdx + 1 ) );
								ssRec.SampleSetLabel = boost::str( boost::format( "SS - SampleSet%d Label" ) % ( ssIdx + 1 ) );
								ssRec.Comments = boost::str( boost::format( "DD - SampleSet%d comment" ) % ( ssIdx + 1 ) );
								ssRec.CarrierType = 0;
								ssRec.OwnerId = {};
								ssRec.OwnerNameStr = boost::str( boost::format( "SS - SSTestUser%d" ) % ( ssIdx + 1 ) );
								ssRec.CreateDateTP = {};
								ssRec.ModifyDateTP = {};
								ssRec.RunDateTP = {};
								ssRec.WorklistId = {};
								ssRec.ProcessedItemCount = 0;
								ssRec.SSItemsList.clear();

								for ( siIdx = 0; siIdx < MAX_SI_GEN; siIdx++ )
								{
									DBApi::DB_SampleItemRecord siRec = {};

									siRec.SampleItemIdNum = 0;
									siRec.SampleItemId = {};
									siRec.SampleItemStatus = 0;
									siRec.SampleItemNameStr = boost::str( boost::format( "SampleSet Removal Test SampleItem %d - SampleSet %d - Worklist %d" ) % ( siIdx + 1 ) % ( ssIdx + 1 ) % ( wlIdx + 1 ) );
									siRec.Comments = "SampleSet - SampleItem removal test";
									siRec.RunDateTP = {};
									siRec.SampleSetId = {};
									siRec.SampleId = {};
									siRec.OwnerId = {};
									siRec.OwnerNameStr = boost::str( boost::format( "SS - SITestUser%d" ) % ( ssIdx + 1 ) );
									siRec.SaveNthImage = insertRec.SaveNthImage;
									siRec.WashTypeIndex = insertRec.WashTypeIndex;
									siRec.Dilution = insertRec.Dilution;
									siRec.ItemLabel = "SampleSet SampleItem removal test";
									siRec.ImageAnalysisParamId = {};
									siRec.AnalysisDefId = {};
									siRec.AnalysisDefIndex = insertRec.AnalysisDefIndex;
									siRec.AnalysisDef = {};
									siRec.AnalysisParamId = {};
									siRec.AnalysisParam = {};
									siRec.CellTypeId = {};
									siRec.CellTypeIndex = insertRec.CellTypeIndex;
									siRec.CellType = {};
									siRec.BioProcessId = {};
									siRec.BioProcessNameStr = "SampleSet SampleItem Removal Test BioProcess name";
									siRec.QcProcessId = {};
									siRec.QcProcessNameStr = "SampleSet SampleItem Removal Test QcProcess name";
									siRec.WorkflowId = {};
									siRec.SampleRow = 'Z';
									siRec.SampleCol = well++;
									siRec.RotationCount = 0;

									ssRec.SSItemsList.push_back( siRec );
								}
								ssRec.SampleItemCount = (uint16_t) siIdx;

								insertRec.SSList.push_back( ssRec );
							}
							insertRec.SampleSetCount = (int16_t) ssIdx;

							qResult = DBApi::DbAddWorklist( insertRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding test image records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								wlIdx = 0;
								break;
							}
						}

						for ( auto ssIter = insertRec.SSList.begin(); ssIter != insertRec.SSList.end(); ++ssIter )
						{
							DBApi::DB_SampleSetRecord ssRec = *ssIter;

							for ( auto siIter = ssRec.SSItemsList.begin(); siIter != ssRec.SSItemsList.end(); ++siIter )
							{
								DBApi::DB_SampleItemRecord& siRec = *siIter;
								DBApi::DB_SampleRecord sampleRec = {};
								DBApi::DB_ImageSetRecord isRec = {};
								int seqIdx = 0;

								sampleRec.SampleIdNum = 0;
								sampleRec.SampleId = {};
								sampleRec.SampleNameStr = siRec.SampleItemNameStr;
								sampleRec.SampleStatus = 0;
								sampleRec.CellTypeId = {};
								sampleRec.CellTypeIndex = siRec.CellTypeIndex;
								sampleRec.AnalysisDefId = {};
								sampleRec.AnalysisDefIndex = siRec.AnalysisDefIndex;
								sampleRec.Label = siRec.ItemLabel;
								sampleRec.BioProcessId = {};
								sampleRec.QcId = {};
								sampleRec.WorkflowId = {};
								sampleRec.CommentsStr = siRec.Comments;
								sampleRec.WashTypeIndex = siRec.WashTypeIndex;
								sampleRec.Dilution = siRec.Dilution;
								sampleRec.OwnerUserId = {};
								sampleRec.OwnerUserNameStr = siRec.OwnerNameStr;
								sampleRec.RunUserId = {};
								sampleRec.RunUserNameStr = insertRec.RunUserNameStr;
								sampleRec.AcquisitionDateTP = {};
								sampleRec.ImageSetId = isRec.ImageSetId;
								sampleRec.DustRefImageSetId = {};
								sampleRec.AcquisitionInstrumentSNStr = insertRec.InstrumentSNStr;
								sampleRec.ImageAnalysisParamId = {};
								sampleRec.NumReagents = 0;
								sampleRec.ReagentTypeNameList = {};
								sampleRec.ReagentPackNumList = {};
								sampleRec.PackLotNumList = {};
								sampleRec.PackLotExpirationList = {};
								sampleRec.PackInServiceList = {};
								sampleRec.PackServiceExpirationList = {};

								qResult = DBApi::DbAddSample( sampleRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Error adding sample record worklist: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									wlIdx = 0;
									break;
								}
								siRec.SampleId = sampleRec.SampleId;

								qResult = DBApi::DbModifySampleItem( siRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Error updating sample-item record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									wlIdx = 0;
									break;
								}

								isRec.ImageSetIdNum;
								isRec.ImageSetId;
								isRec.SampleId;
								isRec.CreationDateTP = ChronoUtilities::CurrentTime();
								isRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
								isRec.ImageSequenceList.clear();

								HawkeyeUUID tgtId = sampleRec.SampleId;
								std::string idStr = tgtId.to_string();

								for ( seqIdx = 0; seqIdx < MAX_SEQ_GEN; seqIdx++ )
								{
									DBApi::DB_ImageSeqRecord seqRec = {};

									seqRec.FlChannels = 0;
									seqRec.ImageCount = 1;
									seqRec.ImageSequenceFolderStr = idStr;
									seqRec.SequenceNum = seqIdx + 1;
									seqRec.ImageList.clear();

									for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
									{
										DBApi::DB_ImageRecord imgRec = {};

										imgRec.ImageId = {};
										imgRec.ImageIdNum = 0;
										imgRec.ImageChannel = imgIdx + 1;
										imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

										seqRec.ImageList.push_back( imgRec );
									}
									isRec.ImageSequenceList.push_back( seqRec );
								}
								isRec.ImageSequenceCount = (int16_t) seqIdx;

								if ( seqIdx > 0 )
								{
									qResult = DBApi::DbAddImageSet( isRec );
									if ( qResult != DBApi::eQueryResult::QueryOk )
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										wlIdx = 0;
										break;
									}
								}
								sampleRec.ImageSetId = isRec.ImageSetId;

								qResult = DBApi::DbModifySample( sampleRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Error adding sample record worklist: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									wlIdx = 0;
									break;
								}
							}
						}

						if ( wlIdx > 0 )
						{
							qResult = DBApi::DbGetSampleSetList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample-set List..." << std::endl;

						print_db_sampleset_list( rec_list );

						std::vector<DBApi::DB_SampleItemRecord> sirec_list;

						qResult = DBApi::DbGetSampleItemList( sirec_list, filtertype, filtercompare, filtertgt, limitcnt );
						size_t siCnt = sirec_list.size();
						if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
						{
							std::cout << "Successfully retrieved sample-set List..." << std::endl;

							print_db_sampleitem_list( sirec_list );
						}

						std::vector<uuid__t> id_list;

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all sample-sets? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->SampleSetId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveSampleSets( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed sample-sets using uuid list" << std::endl;

								rec_list.clear();
								qResult = DBApi::DbGetSampleSetList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved sample-set List..." << std::endl;

									print_db_sampleset_list( rec_list );
								}

								sirec_list.clear();
								qResult = DBApi::DbGetSampleItemList( sirec_list, filtertype, filtercompare, filtertgt, limitcnt );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved sample-item List...";

									print_db_sampleitem_list( sirec_list );

									if ( sirec_list.size() != 0 )
									{
										std::cout << "... but some sample item records were NOT deleted...";
									}
									std::cout << std::endl;
								}

							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove sample-sets using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
					DBApi::GetQueryResultString( qResult, queryResultStr );
					std::cout << "Failed to retrieve sample-set list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
				}

				case 5:			// remove SampleItem record
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;
					DBApi::DB_SampleItemRecord siRec = {};

					qResult = DBApi::DbGetSampleItemList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample-set List..." << std::endl;

						print_db_sampleitem_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									siRec = rec_list.at( listIdx );
									if ( siRec.SampleItemIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveSampleItem( siRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove sample-set record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve sample-set list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 6:			// remove SampleItem Record List
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;

					DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
					std::string filtercompare = "=";
					std::string filtertgt = "0";
					DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::AllSubObjs;
					int32_t limitcnt = 1000;

					qResult = DBApi::DbGetSampleItemList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample-item List..." << std::endl;

						print_db_sampleitem_list( rec_list );

						std::vector<uuid__t> id_list;

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all sample-items? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->SampleItemId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveSampleItems( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed SampleItems using uuid list" << std::endl;

								rec_list.clear();
								qResult = DBApi::DbGetSampleItemList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
								if ( qResult == DBApi::eQueryResult::QueryOk )
								{
									print_db_sampleitem_list( rec_list );
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove SampleItems using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleItems list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 7:			// remove Sample Record
				{
					std::vector<DBApi::DB_SampleRecord> rec_list;
					DBApi::DB_SampleRecord smRec = {};

					qResult = DBApi::DbGetSampleList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample List..." << std::endl;

						print_db_samples_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									smRec = rec_list.at( listIdx );
									if ( smRec.SampleIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveSample( smRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove sample record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve sample list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					break;
				}

				case 8:			// remove Sample Record list
				{
					std::vector<DBApi::DB_SampleRecord> rec_list;

					DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
					std::string filtercompare = "=";
					std::string filtertgt = "0";
					DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::AllSubObjs;
					int32_t limitcnt = 1000;

					qResult = DBApi::DbGetSampleList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						// no records found; generate a list to delete...
						int wlIdx = 0;

						for ( wlIdx = 0; wlIdx < MAX_WL_GEN; wlIdx++ )
						{
							DBApi::DB_WorklistRecord insertRec = {};
							int well = 1;
							int ssIdx = 0;

							insertRec.WorklistIdNum = 0;
							insertRec.WorklistId = {};
							insertRec.WorklistStatus = 0;
							insertRec.WorklistNameStr = boost::str( boost::format( "Sample Deletion Test Worklist %d" ) % ( wlIdx + 1 ) );
							insertRec.ListComments = "Testing Sample deletion";
							insertRec.InstrumentSNStr = "SampleTestInstrument";
							insertRec.CreationUserId = {};
							insertRec.CreationUserNameStr = "SampleTestUser";
							insertRec.RunUserId = {};
							insertRec.RunUserNameStr = "SampleTestRunUser";
							insertRec.RunDateTP = {};
							insertRec.AcquireSample = true;
							insertRec.CarrierType = 0;
							insertRec.ByColumn = false;
							insertRec.SaveNthImage = 1;
							insertRec.WashTypeIndex = 0;
							insertRec.Dilution = 1;
							insertRec.SampleSetNameStr = "Default SampleSet name";
							insertRec.SampleItemNameStr = "Default SampleItem name";
							insertRec.ImageAnalysisParamId = {};
							insertRec.AnalysisDefId = {};
							insertRec.AnalysisDefIndex = 0;
							insertRec.AnalysisParamId = {};
							insertRec.CellTypeId = {};
							insertRec.CellTypeIndex = 0;
							insertRec.BioProcessId = {};
							insertRec.BioProcessNameStr = "Default BioProcess name";
							insertRec.QcProcessId = {};
							insertRec.QcProcessNameStr = "Default QcProcess name";
							insertRec.WorkflowId = {};
							insertRec.SampleSetCount = 0;
							insertRec.ProcessedSetCount = 0;

							for ( ssIdx = 0; ssIdx < MAX_SS_GEN; ssIdx++ )
							{
								DBApi::DB_SampleSetRecord ssRec = {};
								int siIdx = 0;

								ssRec.SampleSetIdNum = 0;
								ssRec.SampleSetId = {};
								ssRec.SampleSetStatus = 0;
								ssRec.SampleSetNameStr = boost::str( boost::format( "SA - Test SampleSet%d" ) % ( ssIdx + 1 ) );
								ssRec.SampleSetLabel = boost::str( boost::format( "SA - SampleSet%d Label" ) % ( ssIdx + 1 ) );
								ssRec.Comments = boost::str( boost::format( "SA - SampleSet%d comment" ) % ( ssIdx + 1 ) );
								ssRec.CarrierType = 0;
								ssRec.OwnerId = {};
								ssRec.OwnerNameStr = boost::str( boost::format( "SA - SampleTestUser%d" ) % ( ssIdx + 1 ) );
								ssRec.CreateDateTP = {};
								ssRec.ModifyDateTP = {};
								ssRec.RunDateTP = {};
								ssRec.WorklistId = {};
								ssRec.ProcessedItemCount = 0;
								ssRec.SSItemsList.clear();

								for ( siIdx = 0; siIdx < MAX_SI_GEN; siIdx++ )
								{
									DBApi::DB_SampleItemRecord siRec = {};

									siRec.SampleItemIdNum = 0;
									siRec.SampleItemId = {};
									siRec.SampleItemStatus = 0;
									siRec.SampleItemNameStr = boost::str( boost::format( "Sample Removal Test SampleItem %d - SampleSet %d - Worklist %d" ) % ( siIdx + 1 ) % ( ssIdx + 1 ) % ( wlIdx + 1 ) );
									siRec.Comments = "Sample - SampleItem removal test";
									siRec.RunDateTP = {};
									siRec.SampleSetId = {};
									siRec.SampleId = {};
									siRec.OwnerId = {};
									siRec.OwnerNameStr = boost::str( boost::format( "SA - SITestUser%d" ) % ( ssIdx + 1 ) );
									siRec.SaveNthImage = insertRec.SaveNthImage;
									siRec.WashTypeIndex = insertRec.WashTypeIndex;
									siRec.Dilution = insertRec.Dilution;
									siRec.ItemLabel = "Sample SampleItem removal test";
									siRec.ImageAnalysisParamId = {};
									siRec.AnalysisDefId = {};
									siRec.AnalysisDefIndex = insertRec.AnalysisDefIndex;
									siRec.AnalysisDef = {};
									siRec.AnalysisParamId = {};
									siRec.AnalysisParam = {};
									siRec.CellTypeId = {};
									siRec.CellTypeIndex = insertRec.CellTypeIndex;
									siRec.CellType = {};
									siRec.BioProcessId = {};
									siRec.BioProcessNameStr = "Sample SampleItem Removal Test BioProcess name";
									siRec.QcProcessId = {};
									siRec.QcProcessNameStr = "Sample SampleItem Removal Test QcProcess name";
									siRec.WorkflowId = {};
									siRec.SampleRow = 'Z';
									siRec.SampleCol = well++;
									siRec.RotationCount = 0;

									ssRec.SSItemsList.push_back( siRec );
								}
								ssRec.SampleItemCount = (uint16_t) siIdx;

								insertRec.SSList.push_back( ssRec );
							}
							insertRec.SampleSetCount = (int16_t) ssIdx;

							qResult = DBApi::DbAddWorklist( insertRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding sample record worklist: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								wlIdx = 0;
								break;
							}

							for ( auto ssIter = insertRec.SSList.begin(); ssIter != insertRec.SSList.end(); ++ssIter )
							{
								DBApi::DB_SampleSetRecord ssRec = *ssIter;

								for ( auto siIter = ssRec.SSItemsList.begin(); siIter != ssRec.SSItemsList.end(); ++siIter )
								{
									DBApi::DB_SampleItemRecord& siRec = *siIter;
									DBApi::DB_SampleRecord sampleRec = {};
									DBApi::DB_ImageSetRecord isRec = {};
									int seqIdx = 0;

									sampleRec.SampleIdNum = 0;
									sampleRec.SampleId = {};
									sampleRec.SampleNameStr = siRec.SampleItemNameStr;
									sampleRec.SampleStatus = 0;
									sampleRec.CellTypeId = {};
									sampleRec.CellTypeIndex = siRec.CellTypeIndex;
									sampleRec.AnalysisDefId = {};
									sampleRec.AnalysisDefIndex = siRec.AnalysisDefIndex;
									sampleRec.Label = siRec.ItemLabel;
									sampleRec.BioProcessId = {};
									sampleRec.QcId = {};
									sampleRec.WorkflowId = {};
									sampleRec.CommentsStr = siRec.Comments;
									sampleRec.WashTypeIndex = siRec.WashTypeIndex;
									sampleRec.Dilution = siRec.Dilution;
									sampleRec.OwnerUserId = {};
									sampleRec.OwnerUserNameStr = siRec.OwnerNameStr;
									sampleRec.RunUserId = {};
									sampleRec.RunUserNameStr = insertRec.RunUserNameStr;
									sampleRec.AcquisitionDateTP = {};
									sampleRec.ImageSetId = isRec.ImageSetId;
									sampleRec.DustRefImageSetId = {};
									sampleRec.AcquisitionInstrumentSNStr = insertRec.InstrumentSNStr;
									sampleRec.ImageAnalysisParamId = {};
									sampleRec.NumReagents = 0;
									sampleRec.ReagentTypeNameList = {};
									sampleRec.ReagentPackNumList = {};
									sampleRec.PackLotNumList = {};
									sampleRec.PackLotExpirationList = {};
									sampleRec.PackInServiceList = {};
									sampleRec.PackServiceExpirationList = {};

									qResult = DBApi::DbAddSample( sampleRec );
									if ( qResult != DBApi::eQueryResult::QueryOk )
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Error adding sample record worklist: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										wlIdx = 0;
										break;
									}
									siRec.SampleId = sampleRec.SampleId;

									qResult = DBApi::DbModifySampleItem( siRec );
									if ( qResult != DBApi::eQueryResult::QueryOk )
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Error updating sample-item record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										wlIdx = 0;
										break;
									}

									isRec.ImageSetIdNum;
									isRec.ImageSetId;
									isRec.SampleId;
									isRec.CreationDateTP = ChronoUtilities::CurrentTime();
									isRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
									isRec.ImageSequenceList.clear();

									HawkeyeUUID tgtId = sampleRec.SampleId;
									std::string idStr = tgtId.to_string();

									for ( seqIdx = 0; seqIdx < MAX_SEQ_GEN; seqIdx++ )
									{
										DBApi::DB_ImageSeqRecord seqRec = {};

										seqRec.FlChannels = 0;
										seqRec.ImageCount = 1;
										seqRec.ImageSequenceFolderStr = idStr;
										seqRec.SequenceNum = seqIdx + 1;
										seqRec.ImageList.clear();

										for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
										{
											DBApi::DB_ImageRecord imgRec = {};

											imgRec.ImageId = {};
											imgRec.ImageIdNum = 0;
											imgRec.ImageChannel = imgIdx + 1;
											imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

											seqRec.ImageList.push_back( imgRec );
										}
										isRec.ImageSequenceList.push_back( seqRec );
									}
									isRec.ImageSequenceCount = (int16_t) seqIdx;

									if ( seqIdx > 0 )
									{
										qResult = DBApi::DbAddImageSet( isRec );
										if ( qResult != DBApi::eQueryResult::QueryOk )
										{
											DBApi::GetQueryResultString( qResult, queryResultStr );
											std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
											wlIdx = 0;
											break;
										}
									}
									sampleRec.ImageSetId = isRec.ImageSetId;

									qResult = DBApi::DbModifySample( sampleRec );
									if ( qResult != DBApi::eQueryResult::QueryOk )
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Error adding sample record worklist: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
										wlIdx = 0;
										break;
									}
								}
							}
						}

						if ( wlIdx > 0 )
						{
							rec_list.clear();
							qResult = DBApi::DbGetSampleList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved sample List..." << std::endl;

						print_db_samples_list( rec_list );

						std::vector<uuid__t> id_list;

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all samples? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->SampleId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveSamples( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed samples using uuid list" << std::endl;

								rec_list.clear();
								qResult = DBApi::DbGetSampleList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved sample List..." << std::endl;

									print_db_samples_list( rec_list );
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove samples using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve samples list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 9:			// remove ImageSet Record
				{
					std::vector<DBApi::DB_ImageSetRecord> rec_list;

					rec_list.clear();
					qResult = DBApi::DbGetImageSetsList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						int seqIdx = 0;
						// no records found; generate a list to select deletion record...
						DBApi::DB_ImageSetRecord setRec = {};
						HawkeyeUUID tgtId = HawkeyeUUID::Generate();

						setRec.CreationDateTP = ChronoUtilities::CurrentTime();
						setRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
						tgtId.get_uuid__t( setRec.SampleId );

						for ( seqIdx = 0; seqIdx < MAX_SEQ_GEN; seqIdx++ )
						{
							DBApi::DB_ImageSeqRecord seqRec = {};

							seqRec.FlChannels = 4;
							seqRec.ImageCount = 5;
							seqRec.ImageSequenceFolderStr = tgtId.to_string();
							seqRec.SequenceNum = seqIdx + 1;
							seqRec.ImageList.clear();

							for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
							{
								DBApi::DB_ImageRecord imgRec = {};

								imgRec.ImageId = {};
								imgRec.ImageIdNum = 0;
								imgRec.ImageChannel = imgIdx + 1;
								imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

								seqRec.ImageList.push_back( imgRec );
							}
							setRec.ImageSequenceList.push_back( seqRec );
						}
						setRec.ImageSequenceCount = (int16_t)seqIdx;

						if ( seqIdx > 0 )
						{
							qResult = DBApi::DbAddImageSet( setRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
							else
							{
								qResult = DBApi::DbGetImageSetsList( rec_list );
							}
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image set List..." << std::endl;

						print_db_imageset_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								DBApi::DB_ImageSetRecord setRec = {};

								do
								{
									setRec = rec_list.at( listIdx );
									if ( setRec.ImageSetIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveImageSet( setRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove image set record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Image set list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 10:			// remove ImageSet Record list
				{
					std::vector<DBApi::DB_ImageSetRecord> rec_list;

					rec_list.clear();
					qResult = DBApi::DbGetImageSetsList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						uint16_t seqIdx = 0;
						// no records found; generate a list to select deletion record...
						DBApi::DB_ImageSetRecord setRec = {};
						HawkeyeUUID tgtId = HawkeyeUUID::Generate();

						setRec.CreationDateTP = ChronoUtilities::CurrentTime();
						setRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
						tgtId.get_uuid__t( setRec.SampleId );

						for ( seqIdx = 0; seqIdx < 25; seqIdx++ )
						{
							DBApi::DB_ImageSeqRecord seqRec = {};

							seqRec.FlChannels = 4;
							seqRec.ImageCount = 5;
							seqRec.ImageSequenceFolderStr = tgtId.to_string();
							seqRec.SequenceNum = seqIdx + 1;
							seqRec.ImageList.clear();

							for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
							{
								DBApi::DB_ImageRecord imgRec = {};

								imgRec.ImageId = {};
								imgRec.ImageIdNum = 0;
								imgRec.ImageChannel = imgIdx + 1;
								imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

								seqRec.ImageList.push_back( imgRec );
							}
							setRec.ImageSequenceList.push_back( seqRec );
						}
						setRec.ImageSequenceCount = seqIdx;

						if ( seqIdx > 0 )
						{
							qResult = DBApi::DbAddImageSet( setRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
							else
							{
								qResult = DBApi::DbGetImageSetsList( rec_list );
							}
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image set List..." << std::endl;

						print_db_imageset_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								DBApi::DB_ImageSetRecord setRec = {};

								do
								{
									setRec = rec_list.at( listIdx );
									if ( setRec.ImageSetIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveImageSet( setRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove image set record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Image sequence list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 11:			// remove ImageSequence Record
				{
					std::vector<DBApi::DB_ImageSeqRecord> rec_list;

					rec_list.clear();
					qResult = DBApi::DbGetImageSequenceList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						uint16_t seqIdx = 0;
						// no records found; generate a list to select deletion record...
						DBApi::DB_ImageSetRecord setRec = {};
						HawkeyeUUID tgtId = HawkeyeUUID::Generate();

						setRec.CreationDateTP = ChronoUtilities::CurrentTime();
						setRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
						tgtId.get_uuid__t( setRec.SampleId );

						for ( seqIdx = 0; seqIdx < 25; seqIdx++ )
						{
							DBApi::DB_ImageSeqRecord seqRec = {};

							seqRec.FlChannels = 4;
							seqRec.ImageCount = 5;
							seqRec.ImageSequenceFolderStr = tgtId.to_string();
							seqRec.SequenceNum = seqIdx + 1;
							seqRec.ImageList.clear();

							for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
							{
								DBApi::DB_ImageRecord imgRec = {};

								imgRec.ImageId = {};
								imgRec.ImageIdNum = 0;
								imgRec.ImageChannel = imgIdx + 1;
								imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

								seqRec.ImageList.push_back( imgRec );
							}
						}
						setRec.ImageSequenceCount = seqIdx;

						if ( seqIdx > 0 )
						{
							// adding the set adds all contained objects...
							qResult = DBApi::DbAddImageSet( setRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
							else
							{
								qResult = DBApi::DbGetImageSequenceList( rec_list );
							}
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image sequence record list..." << std::endl;

						print_db_imageseq_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								DBApi::DB_ImageSeqRecord seqRec = {};

								do
								{
									seqRec = rec_list.at( listIdx );
									if ( seqRec.ImageSequenceIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveImageSequence( seqRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove image sequence record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Image sequence record list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 12:			// remove ImageSequence Record list
				{
					std::vector<DBApi::DB_ImageSeqRecord> rec_list;
					std::vector<uuid__t> id_list;
					DBApi::DB_ImageSeqRecord isRec = {};

					rec_list.clear();
					qResult = DBApi::DbGetImageSequenceList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
						{
							// no records found; generate a list to select deletion record...
							uint16_t seqIdx = 0;
							DBApi::DB_ImageSetRecord setRec = {};
							HawkeyeUUID tgtId = HawkeyeUUID::Generate();

							setRec.CreationDateTP = ChronoUtilities::CurrentTime();
							setRec.ImageSetPathStr = "\\Instrument\\ResultData\\Images";
							tgtId.get_uuid__t( setRec.SampleId );

							for ( seqIdx = 0; seqIdx < 25; seqIdx++ )
							{
								DBApi::DB_ImageSeqRecord seqRec = {};

								seqRec.FlChannels = 4;
								seqRec.ImageCount = 5;
								seqRec.ImageSequenceFolderStr = tgtId.to_string();
								seqRec.SequenceNum = seqIdx + 1;
								seqRec.ImageList.clear();

								for ( int imgIdx = 0; imgIdx < seqRec.ImageCount; imgIdx++ )		// simulate multiple image channels... 1 brightfield and 4 fluorescence
								{
									DBApi::DB_ImageRecord imgRec = {};

									imgRec.ImageId = {};
									imgRec.ImageIdNum = 0;
									imgRec.ImageChannel = imgIdx + 1;
									imgRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d-%d" ) % ( seqIdx + 1 ) % ( imgIdx + 1 ) );

									seqRec.ImageList.push_back( imgRec );
								}
							}
							setRec.ImageSequenceCount = seqIdx;

							if ( seqIdx > 0 )
							{
								// adding the set adds all contained objects...
								qResult = DBApi::DbAddImageSet( setRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Error adding Image tree objects: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
								else
								{
									qResult = DBApi::DbGetImageSequenceList( rec_list );
								}
							}
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image sequence records list..." << std::endl;

						print_db_imageseq_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all image sequence records? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->ImageSetId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveImageSequencesByUuidList( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed image sequence records using uuid list" << std::endl;
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove image sequence records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve image sequence records list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 13:			// remove Image record
				{
					std::vector<DBApi::DB_ImageRecord> rec_list;

					rec_list.clear();
					qResult = DBApi::DbGetImagesList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						// no records found; generate a list to select deletion record...
						int imgIdx = 0;

						for ( imgIdx = 0; imgIdx < 20; imgIdx++ )
						{
							DBApi::DB_ImageRecord insertRec = {};
							HawkeyeUUID tgtId = HawkeyeUUID::Generate();

							insertRec.ImageId = {};
							insertRec.ImageIdNum = 0;
							insertRec.ImageChannel = 1;
							insertRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d" ) % ( imgIdx + 1 ) );
							tgtId.get_uuid__t( insertRec.ImageSequenceId );

							qResult = DBApi::DbAddImage( insertRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding image records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								break;
							}
						}

						if ( imgIdx > 0 )
						{
							qResult = DBApi::DbGetImagesList( rec_list );
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image List..." << std::endl;

						print_db_image_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								DBApi::DB_ImageRecord imgRec = {};

								do
								{
									imgRec = rec_list.at( listIdx );
									if ( imgRec.ImageIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveImage( imgRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove image record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Image record list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 14:			// remove Image record List
				{
					std::vector<DBApi::DB_ImageRecord> rec_list;
					std::vector<uuid__t> id_list;

					rec_list.clear();
					qResult = DBApi::DbGetImagesList( rec_list );
					if ( ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults ) && rec_list.size() == 0 )
					{
						// no records found; generate a list to delete...
						int imgIdx = 0;

						for ( imgIdx = 0; imgIdx < 20; imgIdx++ )
						{
							DBApi::DB_ImageRecord insertRec = {};
							HawkeyeUUID tgtId = HawkeyeUUID::Generate();

							insertRec.ImageId = {};
							insertRec.ImageIdNum = 0;
							insertRec.ImageChannel = 1;
							insertRec.ImageFileNameStr = boost::str( boost::format( "TestImage%d" ) % ( imgIdx + 1 ) );
							tgtId.get_uuid__t( insertRec.ImageSequenceId );

							qResult = DBApi::DbAddImage( insertRec );
							if ( qResult != DBApi::eQueryResult::QueryOk )
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Error adding test image records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								break;
							}
						}

						if ( imgIdx > 0 )
						{
							qResult = DBApi::DbGetImagesList( rec_list );
						}
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved image List..." << std::endl;

						print_db_image_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							promptStr = "Delete all images? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
							if ( input_quit )
							{
								return;
							}

							if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
							{
								break;
							}

							for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
							{
								uuid__t id = recIter->ImageId;
								id_list.push_back( id );
							}

							qResult = DBApi::DbRemoveImagesByUuidList( id_list );
							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								std::cout << "Successfully removed images using uuid list" << std::endl;
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove images using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve image record list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 15:			// remove Analysis record
				{
					std::vector<DBApi::DB_AnalysisRecord> rec_list;
					DBApi::DB_AnalysisRecord anRec = {};

					qResult = DBApi::DbGetAnalysesList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved analysis List..." << std::endl;

						print_db_analyses_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									anRec = rec_list.at( listIdx );
									if ( anRec.AnalysisIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveAnalysis( anRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove analysis record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve analysis list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 16:			// remove Analysis Record list
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;
					std::vector<uuid__t> id_list;
					DBApi::DB_SampleItemRecord isRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;

					//std::vector<DBApi::DB_AnalysisRecord> rec_list;

					//DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
					//std::string filtercompare = "=";
					//std::string filtertgt = "0";
					//DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
					//DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					//DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::AllSubObjs;
					//int32_t limitcnt = 1000;

					//qResult = DBApi::DbGetAnalysesList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
					//if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					//{
					//	std::cout << "Successfully retrieved analysis List..." << std::endl;

					//	print_db_analyses_list( rec_list );

					//	std::vector<uuid__t> id_list;

					//	listSize = rec_list.size();
					//	if ( listSize > 0 )
					//	{
					//		promptStr = "Delete all analyses? (y/n): ";

					//		entryStr.clear();
					//		input_quit = GetYesOrNoResponse( entryStr, promptStr, false );
					//		if ( input_quit )
					//		{
					//			return;
					//		}

					//		if ( strnicmp( entryStr.c_str(), "N", 2 ) == 0 )
					//		{
					//			break;
					//		}

					//		for ( auto recIter = rec_list.begin(); recIter != rec_list.end(); ++recIter )
					//		{
					//			uuid__t id = recIter->AnalysisId;
					//			id_list.push_back( id );
					//		}

					//		qResult = DBApi::DbRemoveAnalyses( id_list );
					//		if ( qResult == DBApi::eQueryResult::QueryOk )
					//		{
					//			std::cout << "Successfully removed SampleItems using uuid list" << std::endl;

					//			rec_list.clear();
					//			qResult = DBApi::DbGetSampleItemList( rec_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs );
					//			if ( qResult == DBApi::eQueryResult::QueryOk )
					//			{
					//				print_db_sampleitem_list( rec_list );
					//			}
					//		}
					//		else
					//		{
					//			DBApi::GetQueryResultString( qResult, resultStr );
					//			std::cout << "Failed to remove SampleItems using uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					//		}
					//	}
					//}
					//else
					//{
					//	DBApi::GetQueryResultString( qResult, resultStr );
					//	std::cout << "Failed to retrieve SampleItems list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					//}
					//break;
				}

				case 17:			// remove SummaryResult record
				{
					std::vector<DBApi::DB_SummaryResultRecord> rec_list;
					DBApi::DB_SummaryResultRecord srRec = {};

					qResult = DBApi::DbGetSummaryResultsList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved summary results List..." << std::endl;

						print_db_summary_results_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									srRec = rec_list.at( listIdx );
									if ( srRec.SummaryResultIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveSummaryResult( srRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove summary results record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve summary results list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 18:			// remove SummaryResult Record list
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;
					std::vector<uuid__t> id_list;
					DBApi::DB_SampleItemRecord isRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 19:			// remove DetailedResult record
				{
					std::vector<DBApi::DB_DetailedResultRecord> rec_list;
					DBApi::DB_DetailedResultRecord drRec = {};

					qResult = DBApi::DbGetDetailedResultsList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved detailed results List..." << std::endl;

						print_db_detailed_results_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									drRec = rec_list.at( listIdx );
									if ( drRec.DetailedResultIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveDetailedResult( drRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove detailed result record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve detailed results list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 20:			// remove DetailedResult Record list
				{
					std::vector<DBApi::DB_SampleItemRecord> rec_list;
					std::vector<uuid__t> id_list;
					DBApi::DB_SampleItemRecord isRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
				}

				case 21:			// remove CellType record
				{
					std::vector<DBApi::DB_CellTypeRecord> rec_list;
					DBApi::DB_CellTypeRecord ctRec = {};

					qResult = DBApi::DbGetCellTypeList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved cell type List..." << std::endl;

						print_db_celltypes_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									ctRec = rec_list.at( listIdx );
									if ( ctRec.CellTypeIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveCellType( ctRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove cell type record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve cell type list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 22:			// remove AnalysisDefinition Record
				{
					std::vector<DBApi::DB_AnalysisDefinitionRecord> rec_list;
					DBApi::DB_AnalysisDefinitionRecord adRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
					break;
				}

				case 23:			// remove AnalysisDefinition Record
				{
					std::vector<DBApi::DB_AnalysisParamRecord> rec_list;
					DBApi::DB_AnalysisParamRecord apRec = {};

					std::cout << std::endl;
					std::cout << "Not Implemented..." << std::endl;
					std::cout << std::endl;
					break;
					break;
				}

				case 24:			// remove SignatureDefinition record
				{
					std::vector<DBApi::DB_SignatureRecord> rec_list;
					// 1c1fc498 - 2556 - 4d82 - ad2e - f17fec8738f2
					DBApi::DB_SignatureRecord sigRec = {};

					qResult = DBApi::DbGetSignatureList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved signature List..." << std::endl;

						print_db_signature_list(rec_list);

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									sigRec = rec_list.at( listIdx );
									if ( sigRec.SignatureDefIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								qResult = DBApi::DbRemoveSignature( sigRec );
								if ( qResult != DBApi::eQueryResult::QueryOk )
								{
									DBApi::GetQueryResultString( qResult, queryResultStr );
									std::cout << "Failed to remove signature definition record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve signature definition list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 25:			// remove Reagent info record
				{
					std::vector<DBApi::DB_ReagentTypeRecord> rec_list;
					DBApi::DB_ReagentTypeRecord rxRec = {};
					promptStr = "";

					qResult = DBApi::DbGetReagentInfoList( rec_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Reagent info List..." << std::endl;

						print_db_reagent_info_list( rec_list );

						bool deleteList = false;

						recFound = false;

						listSize = rec_list.size();
						if ( listSize > 1 )
						{
							promptStr = "Delete list of records? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
							{
								deleteList = true;
							}
						}

						if ( deleteList )
						{
							bool useLot = false;

							promptStr = "Delete list of records by lot number? (y/n): ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
							{
								useLot = true;
								std::string lotNumStr = "";

								promptStr = "Select Lot number from a list record? (y/n):  ";

								entryStr.clear();
								input_quit = GetYesOrNoResponse( entryStr, promptStr );
								if ( input_quit )
								{
									return;
								}

								if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
								{
									do
									{
										input_quit = SelectDbListIdNum( listSize, listIdNum );
										if ( input_quit )
										{
											return;
										}

										if ( listIdNum >= 0 )
										{
											do
											{
												rxRec = rec_list.at( listIdx );
												if ( rxRec.ReagentIdNum == listIdNum )
												{
													recFound = true;
												}
											} while ( recFound == false && ++listIdx < listSize );

											if ( recFound )
											{
												lotNumStr = rxRec.LotNumStr;
											}
											else
											{
												std::cout << "Selected IdNum not found in record list: " << (int32_t) listIdNum << std::endl;
											}
										}
										else
										{
											std::cout << "Bad IdNum: " << (int32_t) listIdNum << std::endl;
										}
									} while ( !recFound );
								}
								else
								{
									promptStr = "Enter the desired Lot number: ";
									lengthWarning = "";
//									lengthWarning = "WARNING: Lot number should not be blank!";

									entryStr.clear();
									input_quit = GetStringInput( entryStr, promptStr, lengthWarning);
									if ( input_quit )
									{
										return;
									}
									lotNumStr = entryStr;
								}

								if ( lotNumStr.length() > 0 )
								{
									std::string paramStr = boost::str( boost::format( "'%s'" ) % lotNumStr );
									qResult = DBApi::DbRemoveReagentInfoByLotNum( paramStr );
								}
								else
								{
									std::cout << "No lot number entered. Skipping delete." << std::endl;
								}
							}
							else
							{
								std::vector<int64_t> idnumList = {};

								for ( int i = 0; i < listSize; i++ )
								{
									if ( rec_list.at( i ).ReagentIdNum > 0 )
									{
										idnumList.push_back( rec_list.at( i ).ReagentIdNum );
									}
								}

								if ( idnumList.size() > 0 )
								{
									qResult = DBApi::DbRemoveReagentInfoByIdNumList( idnumList );
								}
							}

							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								rec_list.clear();
								qResult = DBApi::DbGetReagentInfoList( rec_list );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved Reagent info List..." << std::endl;

									print_db_reagent_info_list( rec_list );
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove Reagent info records: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
						else if ( listSize > 0 )
						{
							bool useRecord = false;
							bool useTag = false;

							promptStr = "Delete record by Rfid tag serial number? (y/n):  ";

							entryStr.clear();
							input_quit = GetYesOrNoResponse( entryStr, promptStr );
							if ( input_quit )
							{
								return;
							}

							if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
							{
								useTag = true;
							}

							if ( !useTag )
							{
								promptStr = "Delete record using complete record? (y/n):  ";

								entryStr.clear();
								input_quit = GetYesOrNoResponse( entryStr, promptStr );
								if ( input_quit )
								{
									return;
								}

								if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
								{
									useRecord = true;
								}
							}

							if ( useTag )
							{
								std::string tagStr = "";

								promptStr = "Select tag from a list record? (y/n):  ";

								entryStr.clear();
								input_quit = GetYesOrNoResponse( entryStr, promptStr );
								if ( input_quit )
								{
									return;
								}

								if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 1 ) == 0 )
								{
									do
									{
										input_quit = SelectDbListIdNum( listSize, listIdNum );
										if ( input_quit )
										{
											return;
										}

										if ( listIdNum >= 0 )
										{
											listIdx = 0;
											do
											{
												rxRec = rec_list.at( listIdx );
												if ( rxRec.ReagentIdNum == listIdNum )
												{
													recFound = true;
												}
											} while ( recFound == false && ++listIdx < listSize );

											if ( recFound )
											{
												tagStr = rxRec.ContainerTagSn;
											}
											else
											{
												std::cout << "Selected IdNum not found in record list: " << (int32_t) listIdNum << std::endl;
											}
										}
										else
										{
											std::cout << "Bad IdNum: " << (int32_t) listIdNum << std::endl;
										}
									} while ( !recFound );
								}
								else
								{
									promptStr = "Enter the desired tag serial number: ";
									lengthWarning = "";
//									lengthWarning = "WARNING: Tag serial number should not be blank!";

									entryStr.clear();
									input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
									if ( input_quit )
									{
										return;
									}
									tagStr = entryStr;
								}

								if ( tagStr.length() > 0 )
								{
									std::string paramStr = boost::str( boost::format( "'%s'" ) % tagStr );
									qResult = DBApi::DbRemoveReagentInfoBySN( paramStr );
								}
							}
							else if ( useRecord )
							{
								do
								{
									std::cout << "Select the desired record id number." << std::endl;
									input_quit = SelectDbListIdNum( listSize, listIdNum );
									if ( input_quit )
									{
										return;
									}

									if ( listIdNum >= 0 )
									{
										listIdx = 0;
										do
										{
											rxRec = rec_list.at( listIdx );
											if ( rxRec.ReagentIdNum == listIdNum )
											{
												recFound = true;
											}
										} while ( recFound == false && ++listIdx < listSize );

										if ( !recFound )
										{
											std::cout << "Selected IdNum not found in record list: " << (int32_t) listIdNum << std::endl;
										}
									}
									else
									{
										std::cout << "Bad IdNum: " << (int32_t) listIdNum << std::endl;
									}
								} while ( !recFound );

								if ( recFound )
								{
									qResult = DBApi::DbRemoveReagentInfo( rxRec );
								}
							}
							else
							{
								do
								{
									std::cout << "Select the desired record id number." << std::endl;
									input_quit = SelectDbListIdNum( listSize, listIdNum );
									if ( input_quit )
									{
										return;
									}

									if ( listIdNum >= 0 )
									{
										listIdx = 0;
										do
										{
											rxRec = rec_list.at( listIdx );
											if ( rxRec.ReagentIdNum == listIdNum )
											{
												recFound = true;
											}
										} while ( recFound == false && ++listIdx < listSize );

										if ( !recFound )
										{
											std::cout << "Selected IdNum not found in record list: " << (int32_t) listIdNum << std::endl;
										}
									}
									else
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Bad IdNum" << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									}
								} while ( !recFound );

								if ( recFound )
								{
									qResult = DBApi::DbRemoveReagentInfoByIdNum( rxRec.ReagentIdNum );
								}
							}

							if ( qResult == DBApi::eQueryResult::QueryOk )
							{
								rec_list.clear();
								qResult = DBApi::DbGetReagentInfoList( rec_list );
								if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
								{
									std::cout << "Successfully retrieved Reagent info List..." << std::endl;

									print_db_reagent_info_list( rec_list );
								}
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to remove reagent info record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
						else
						{
							std::cout << "No reagent info records to delete..." << std::endl;
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve reagent info record list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 26:
				{
					std::vector<DBApi::DB_SchedulerConfigRecord> rec_list;
					DBApi::DB_SchedulerConfigRecord scfgRec = {};

					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					primarysort = DBApi::eListSortCriteria::SortNotDefined;
					secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
					sortdir = -1;
					limitcnt = 1000;
					orderstr.clear();

					GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist,
											primarysort, secondarysort, sortdir, limitcnt );

					qResult = DBApi::DbGetSchedulerConfigListEnhanced( rec_list,
																	   filtertypelist, filtercomparelist, filtertgtlist,
																	   limitcnt, primarysort, secondarysort,
																	   sortdir, orderstr, -1, -1 );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SchedulerConfig List..." << std::endl;
						print_db_scheduler_config_list( rec_list );

						listSize = rec_list.size();
						if ( listSize > 0 )
						{
							input_quit = SelectDbListIdNum( listSize, listIdNum );
							if ( input_quit )
							{
								return;
							}

							if ( listIdNum >= 0 )
							{
								do
								{
									scfgRec = rec_list.at( listIdx );
									if ( scfgRec.ConfigIdNum == listIdNum )
									{
										recFound = true;
									}
								} while ( recFound == false && ++listIdx < listSize );

								if ( recFound )
								{
									qResult = DBApi::DbRemoveSchedulerConfig( scfgRec );
									if ( qResult != DBApi::eQueryResult::QueryOk )
									{
										DBApi::GetQueryResultString( qResult, queryResultStr );
										std::cout << "Failed to remove SchedulerConfig record: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
									}
								}
								else
								{
									std::cout << "Selected IdNum not found in record list: " << (int32_t) listIdNum << std::endl;
								}
							}
						}
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SchedulerConfig list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		case smDbRecordListOptions:
		{
			DBApi::eQueryResult qResult = DBApi::eQueryResult::NoResults;
			std::vector<DBApi::eListFilterCriteria> filtertypelist = {};
			std::vector<std::string> filtercomparelist = {};
			std::vector<std::string> filtertgtlist = {};
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined;
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined;
			int32_t sortdir = -1;
			std::string orderstr = "";
			int32_t limitcnt = 0;
			int32_t startindex = -1;
			int64_t startidnum = -1;
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter;
			std::string filtercompare = "=";
			std::string filtertgt = "";
			DBApi::eContainedObjectRetrieval sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
			promptStr = "";

			/*
			enum class eListSortCriteria : int32_t
			{
				SortNotDefined = -1,
				NoSort = 0,
				GuidSort,           // may not be practical for UUIDs
				IdNumSort,
				IndexSort,
				OwnerSort,
				CarrierSort,
				CreationDateSort,
				CreationUserSort,
				RunDateSort,
				UserIdSort,         // may not be practical for UUIDs
				UserIdNumSort,
				UserNameSort,
				SampleIdSort,       // may not be practical for UUIDs
				SampleIdNumSort,
				LabelSort,
				RxTypeNumSort,
				RxLotNumSort,
				ItemNameSort,
				CellTypeSort,
				InstrumentSort,
				QcSort,
				CalTypeSort
			};

			enum class eListFilterCriteria : int32_t
			{
				FilterNotDefined = -1,
				NoFilter = 0,
				IdFilter,
				IdNumFilter,
				IndexFilter,
				ItemNameFilter,
				LabelFilter,
				StatusFilter,
				OwnerFilter,
				ParentFilter,
				CarrierFilter,
				CreationDateFilter,
				CreationDateRangeFilter,
				CreationUserIdFilter,
				CreationUserNameFilter,
				ModifyDateFilter,
				ModifyDateRangeFilter,
				RunDateFilter,
				RunDateRangeFilter,
				RunUserIdFilter,
				RunUserNameFilter,
				SampleAcquiredFilter,
				AcquisitionDateFilter,
				AcquisitionDateRangeFilter,
				SampleIdFilter,
				CellTypeFilter,
				LotFilter,
				QcFilter,
				InstrumentFilter,
				CommentsFilter,
				RoleFilter,
				UserTypeFilter,
				CalTypeFilter
			};
			*/
			switch ( atoi( smValueLine_.c_str() ) )
			{
				case 1:		// Retrieve list of DB worklists; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_WorklistRecord> wl_list = {};

					sub_objs = DBApi::eContainedObjectRetrieval::FirstLevelObjs;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					promptStr = "Select sub-object retrieve mode? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						int mode = 0;
						bool modeValid = false;

						promptStr = "Retrieve mode (0:None, 1: All, -1:FirstLevel): ";
						lengthWarning = "ERROR: mode entry cannot be blank!";
						do
						{
							entryStr.clear();
							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							mode = std::stoi( entryStr );
							if ( mode == 0 || mode == 1 || mode == -1 )
							{
								modeValid = true;
							}
							else
							{
								std::cout << "ERROR: illegal mode value entered!" << std::endl;
								modeValid = false;
							}
							std::cout << std::endl;
						} while ( !modeValid );

						sub_objs = static_cast<DBApi::eContainedObjectRetrieval>( mode );
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetWorklistList( wl_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetWorklistListEnhanced( wl_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Worklist List..." << std::endl;

						print_db_worklist_list( wl_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Worklist list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 2:		// Retrieve list of DB worklist templates; allow definition of sort, filter, limit, and order
				{
					std::vector<DBApi::DB_WorklistRecord> wl_list = {};

					filtertype = DBApi::eListFilterCriteria::StatusFilter;
					filtercompare = "=";
					filtertgt = boost::str( boost::format( "%d" ) % ( static_cast<int32_t>( DBApi::eWorklistStatus::WorklistTemplate ) ) );

					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();

					std::cout << std::endl;
					std::cout << boost::str( boost::format( "Retrieving Worklist list:  filter criteris: %d   primary sort criteria: %d   secondary sort criteria: %d   sort direction: %s   limitcnt: %d" ) % static_cast<int32_t>( filtertype ) % static_cast<int32_t>( primarysort ) % static_cast<int32_t>( secondarysort ) % ( ( sortdir > 0 ) ? "ASC" : "DESC" ) % limitcnt ) << std::endl;
					std::cout << std::endl;

					promptStr = "Change / Select Additional filters or sort criteria? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						filtertypelist.push_back( filtertype );
						filtercomparelist.push_back( filtercompare );
						filtertgtlist.push_back( filtertgt );

						input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
						if ( input_quit )
						{
							return;
						}

						promptStr = "Select sub-object retrieve mode? (y/n):  ";

						entryStr.clear();
						input_quit = GetYesOrNoResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
						{
							int mode = 0;
							bool modeValid = false;

							promptStr = "Retrieve mode (0:None, 1: All, -1:FirstLevel): ";
							lengthWarning = "ERROR: mode entry cannot be blank!";
							do
							{
								entryStr.clear();
								input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
								if ( input_quit )
								{
									return;
								}

								mode = std::stoi( entryStr );
								if ( mode == 0 || mode == 1 || mode == -1 )
								{
									modeValid = true;
								}
								else
								{
									std::cout << "ERROR: illegal mode value entered!" << std::endl;
									modeValid = false;
								}
								std::cout << std::endl;
							} while ( !modeValid );

							sub_objs = static_cast<DBApi::eContainedObjectRetrieval>( mode );
						}
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetWorklistList( wl_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetWorklistListEnhanced( wl_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Worklist template List..." << std::endl;

						print_db_worklist_list( wl_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Worklist template list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 3:		// Retrieve list of sample-sets; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_SampleSetRecord> ss_list = {};

					sub_objs = DBApi::eContainedObjectRetrieval::FirstLevelObjs;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					promptStr = "Select sub-object retrieve mode? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						int mode = 0;
						bool modeValid = false;

						promptStr = "Retrieve mode (0:None, 1: All, -1:FirstLevel): ";
						lengthWarning = "ERROR: mode entry cannot be blank!";
						do
						{
							entryStr.clear();
							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							mode = std::stoi( entryStr );
							if ( mode == 0 || mode == 1 || mode == -1 )
							{
								modeValid = true;
							}
							else
							{
								std::cout << "ERROR: illegal mode value entered!" << std::endl;
								modeValid = false;
							}
							std::cout << std::endl;
						} while ( !modeValid );

						sub_objs = static_cast<DBApi::eContainedObjectRetrieval>( mode );
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleSetList( ss_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleSetListEnhanced( ss_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SampleSet List..." << std::endl;

						print_db_sampleset_list( ss_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleSet list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 4:		// Retrieve list of sample-set templates; allow definition of sort, filter, limit, and order
				{	// this will attempt to bypass the HawkeyeCore DLL for now...
					std::vector<DBApi::DB_SampleSetRecord> ss_list = {};

					filtertype = DBApi::eListFilterCriteria::StatusFilter;
					filtercompare = "=";
					filtertgt = boost::str( boost::format( "%d" ) % ( static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate ) ) );

					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();

					std::cout << std::endl;
					std::cout << boost::str( boost::format( "Retrieving SampleSet list:  filter criteris: %d   primary sort criteria: %d   secondary sort criteria: %d   sort direction: %s   limitcnt: %d" ) % static_cast<int32_t>( filtertype ) % static_cast<int32_t>( primarysort ) % static_cast<int32_t>( secondarysort ) % ( ( sortdir > 0 ) ? "ASC" : "DESC" ) % limitcnt ) << std::endl;
					std::cout << std::endl;

					promptStr = "Select Additional filters or sort criteria? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						filtertypelist.push_back( filtertype );
						filtercomparelist.push_back( filtercompare );
						filtertgtlist.push_back( filtertgt );

						input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
						if ( input_quit )
						{
							return;
						}

						promptStr = "Select sub-object retrieve mode? (y/n):  ";

						entryStr.clear();
						input_quit = GetYesOrNoResponse( entryStr, promptStr );
						if ( input_quit )
						{
							return;
						}

						if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
						{
							int mode = 0;
							bool modeValid = false;

							promptStr = "Retrieve mode (0:None, 1: All, -1:FirstLevel): ";
							lengthWarning = "ERROR: mode entry cannot be blank!";
							do
							{
								entryStr.clear();
								input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
								if ( input_quit )
								{
									return;
								}

								mode = std::stoi( entryStr );
								if ( mode == 0 || mode == 1 || mode == -1 )
								{
									modeValid = true;
								}
								else
								{
									std::cout << "ERROR: illegal mode value entered!" << std::endl;
									modeValid = false;
								}
								std::cout << std::endl;
							} while ( !modeValid );

							sub_objs = static_cast<DBApi::eContainedObjectRetrieval>( mode );
						}
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleSetList( ss_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleSetListEnhanced( ss_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SampleSet template List..." << std::endl;

						print_db_sampleset_list( ss_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleSet template list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 5:		// Retrieve list of sample-items; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_SampleItemRecord> si_list = {};

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleItemList( si_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleItemListEnhanced( si_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SampleItem List..." << std::endl;

						print_db_sampleitem_list( si_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleItem list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 6:		// Retrieve list of sample-item templates; allow definition of sort, filter, limit, and order
				{	// this will attempt to bypass the HawkeyeCore DLL for now...
					std::vector<DBApi::DB_SampleItemRecord> si_list = {};

					filtertype = DBApi::eListFilterCriteria::StatusFilter;
					filtercompare = "=";
					filtertgt = boost::str( boost::format( "%d" ) % ( static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate ) ) );

					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();

					std::cout << std::endl;
					std::cout << boost::str( boost::format( "Retrieving SampleItem list:  filter criteris: %d   primary sort criteria: %d   secondary sort criteria: %d   sort direction: %s   limitcnt: %d" ) % static_cast<int32_t>( filtertype ) % static_cast<int32_t>( primarysort ) % static_cast<int32_t>( secondarysort ) % ( ( sortdir > 0 ) ? "ASC" : "DESC" ) % limitcnt ) << std::endl;
					std::cout << std::endl;

					promptStr = "Select Additional filters or sort criteria? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						filtertypelist.push_back( filtertype );
						filtercomparelist.push_back( filtercompare );
						filtertgtlist.push_back( filtertgt );

						input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
						if ( input_quit )
						{
							return;
						}
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleItemList( si_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleItemListEnhanced( si_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SampleItem template List..." << std::endl;

						print_db_sampleitem_list( si_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleItem template list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 11:		// Retrieve list of samples; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_SampleRecord> s_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleList( s_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleListEnhanced( s_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Samples List..." << std::endl;

						print_db_samples_list( s_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Samples list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 12:		// Retrieve list of image-set records; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_ImageSetRecord> is_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetImageSetsList( is_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetImageSetsListEnhanced( is_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Imageset List..." << std::endl;

						print_db_imageset_list( is_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve ImageSet list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 13:		// Retrieve list of image-sequence records; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_ImageSeqRecord> is_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetImageSequenceList( is_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetImageSequenceListEnhanced( is_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved ImageSequence List..." << std::endl;

						print_db_imageseq_list( is_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve ImageSeuence list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 14:		// Retrieve list of image records; allow definition of sort, filter, limit, and order
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_ImageRecord> img_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetImagesList( img_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetImagesListEnhanced( img_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Image List..." << std::endl;

						print_db_image_list( img_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Image list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 15:		// Retrieve list of Analyses
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_AnalysisRecord> analysis_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetAnalysesList( analysis_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetAnalysesListEnhanced( analysis_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Analyses List..." << std::endl;

						print_db_analyses_list( analysis_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Analyses list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 16:		// Retrieve list of SummaryResults
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_SummaryResultRecord> summary_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSummaryResultsList( summary_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSummaryResultsListEnhanced( summary_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SummaryResults List..." << std::endl;

						print_db_summary_results_list( summary_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SummaryResults list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 17:		// Retrieve list of DetailedResult
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_DetailedResultRecord> detailed_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetDetailedResultsList( detailed_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetDetailedResultsListEnhanced( detailed_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					qResult = DBApi::DbGetDetailedResultsList( detailed_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved DetailedResult List..." << std::endl;

						print_db_detailed_results_list( detailed_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve DetailedResult list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 21:		// Retrieve list of Cell Types
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_CellTypeRecord> ct_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetCellTypeList( ct_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetCellTypeListEnhanced( ct_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved CellType List..." << std::endl;

						print_db_celltypes_list( ct_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve CellType list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 22:		// Retrieve list of Analysis Definitions
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_AnalysisDefinitionRecord> def_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetAnalysisDefinitionsList( def_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetAnalysisDefinitionsListEnhanced( def_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Analysis Definition List..." << std::endl;

						print_db_analysis_def_list( def_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Analysis Definition list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 23:		// Retrieve list of Analysis Parameter
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_AnalysisParamRecord> param_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetAnalysisParameterList( param_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetAnalysisParameterListEnhanced( param_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Analysis Parameters List..." << std::endl;

						print_db_analysis_param_list( param_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Analysis Parameters list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 24:		// Retrieve list of Users
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_UserRecord> user_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetUserList( user_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetUserListEnhanced( user_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved User List..." << std::endl;

						print_db_user_list( user_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Role list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 25:		// Retrieve list of Roles
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_UserRoleRecord> roles_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetRolesList( roles_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetRolesListEnhanced( roles_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Role List..." << std::endl;

						print_db_roles_list( roles_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Role list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 26:		// Retrieve list of Signature definitions
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_SignatureRecord> sig_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSignatureList( sig_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSignatureListEnhanced( sig_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Signatures List..." << std::endl;

						print_db_signature_list( sig_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Signatures list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 27:		// Retrieve list of Calibrations
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_CalibrationRecord> cal_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetCalibrationList( cal_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetCalibrationListEnhanced( cal_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Calibrations List..." << std::endl;

						print_db_calibrations_list( cal_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Calibrations list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 28:		// retrieve  list of reagents
				{
					std::vector<DBApi::DB_ReagentTypeRecord> rx_list;
					DBApi::DB_ReagentTypeRecord rxRec = {};

					qResult = DBApi::DbGetReagentInfoList( rx_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved Reagent info List..." << std::endl;
						print_db_reagent_info_list( rx_list);
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve Reagent info list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 29:		// Retrieve list of Instrument configurations
				{	// this will bypass the HawkeyeCore DLL
					std::vector<DBApi::DB_InstrumentConfigRecord> cfg_list;

					qResult = DBApi::DbGetInstrumentConfigList( cfg_list );
					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved instrument configuration List..." << std::endl;

						print_db_instrument_config_list( cfg_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve instrument config list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}

				case 30:
				{
					std::vector<DBApi::DB_SchedulerConfigRecord> rec_list;
//					DBApi::DB_SchedulerConfigRecord scfgRec = {};

					filtertypelist.clear();
					filtercomparelist.clear();
					filtertgtlist.clear();
					primarysort = DBApi::eListSortCriteria::SortNotDefined;
					secondarysort = DBApi::eListSortCriteria::SortNotDefined;
					sub_objs = DBApi::eContainedObjectRetrieval::NoSubObjs;
					sortdir = 1;
					limitcnt = 1000;
					orderstr.clear();

					GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist,
											primarysort, secondarysort, sortdir, limitcnt );

					qResult = DBApi::DbGetSchedulerConfigListEnhanced( rec_list,
																	   filtertypelist, filtercomparelist, filtertgtlist,
																	   limitcnt, primarysort, secondarysort,
																	   sortdir, orderstr, -1, -1 );

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SchedulerConfig List..." << std::endl;
						print_db_scheduler_config_list( rec_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SchedulerConfig list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}
					break;
				}


				case 100:
				{
					std::vector<DBApi::DB_SampleSetRecord> ss_list;

					input_quit = GetDbListSortAndFilter( filtertypelist, filtercomparelist, filtertgtlist, primarysort, secondarysort, sortdir, limitcnt );
					if ( input_quit )
					{
						return;
					}

					promptStr = "Select sub-object retrieve mode? (y/n):  ";

					entryStr.clear();
					input_quit = GetYesOrNoResponse( entryStr, promptStr );
					if ( input_quit )
					{
						return;
					}

					if ( entryStr.length() > 0 && strnicmp( entryStr.c_str(), "Y", 2 ) == 0 )
					{
						int mode = 0;
						bool modeValid = false;

						promptStr = "Retrieve mode (0:None, 1: All, -1:FirstLevel): ";
						lengthWarning = "ERROR: mode entry cannot be blank!";
						do
						{
							entryStr.clear();
							input_quit = GetStringInput( entryStr, promptStr, lengthWarning );
							if ( input_quit )
							{
								return;
							}

							mode = std::stoi( entryStr );
							if ( mode == 0 || mode == 1 || mode == -1 )
							{
								modeValid = true;
							}
							else
							{
								std::cout << "ERROR: illegal mode value entered!" << std::endl;
								modeValid = false;
							}
							std::cout << std::endl;
						} while ( !modeValid );

						sub_objs = static_cast<DBApi::eContainedObjectRetrieval>( mode );
					}

					if ( filtertypelist.size() != filtercomparelist.size() ||
						 filtertypelist.size() != filtertgtlist.size() ||
						 filtercomparelist.size() != filtertgtlist.size() )		// all vectors must contain the same number of elements
					{
						std::cout << "Invalid filter conditions! Parameters lists are not equal!" << std::endl;
						break;
					}

					if ( filtertypelist.size() <= 1 )	// always pass in all criteria that may have been set in GetDbListSortAndFilter
					{
						if ( filtertypelist.size() > 0 )
						{
							filtertype = filtertypelist.at( 0 );
							filtercompare = filtercomparelist.at( 0 );
							filtertgt = filtertgtlist.at( 0 );
						}
						qResult = DBApi::DbGetSampleSetList( ss_list, filtertype, filtercompare, filtertgt, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}
					else
					{
						qResult = DBApi::DbGetSampleSetListEnhanced( ss_list, filtertypelist, filtercomparelist, filtertgtlist, limitcnt, primarysort, secondarysort, sub_objs, sortdir, orderstr, startindex, startidnum );
					}

					if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
					{
						std::cout << "Successfully retrieved SampleSet List..." << std::endl;

						print_db_sampleset_list( ss_list );
					}
					else
					{
						DBApi::GetQueryResultString( qResult, queryResultStr );
						std::cout << "Failed to retrieve SampleSet list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
					}

					if ( qResult == DBApi::eQueryResult::QueryOk )
					{
						std::vector<uuid__t> ss_uuids;
						int32_t itemCnt = 0;

						for ( auto ssIter = ss_list.begin(); ssIter != ss_list.end() && itemCnt < 10; ++ssIter )
						{
							DBApi::DB_SampleSetRecord ss = *ssIter;
							uuid__t ssid = ss.SampleSetId;
							ss_uuids.push_back( ssid );
							itemCnt++;
						}

						if ( ss_uuids.size() > 1 )
						{
							qResult = DBApi::DbGetSampleSetListByUuidList( ss_list, ss_uuids, 10, DBApi::eListSortCriteria::IdNumSort, DBApi::eListSortCriteria::SortNotDefined, sub_objs, sortdir, orderstr, startindex, startidnum );
							if ( qResult == DBApi::eQueryResult::QueryOk || qResult == DBApi::eQueryResult::NoResults )
							{
								std::cout << "Successfully retrieved SampleSet List by uuid list..." << std::endl;

								print_db_sampleset_list( ss_list );
							}
							else
							{
								DBApi::GetQueryResultString( qResult, queryResultStr );
								std::cout << "Failed to retrieve SampleSet list by uuid list: " << queryResultStr << " (" << (int32_t) qResult << ") " << std::endl;
							}
						}
					}

					break;
				}

				default:
					std::cout << std::endl;
					std::cout << "Invalid entry..." << std::endl;
					std::cout << std::endl;
			}
			smState_ = smCmdEntry;
			break;
		}

		default:
			smState_ = smCmdEntry;
			std::cout << "Invalid entry..." << std::endl;

	} // End "switch (smState_)"

	smValueLine_.clear();

	if (smState_ == smCmdEntry) {
		prompt();
	}

	inputTimer_->expires_from_now(boost::posix_time::milliseconds(100), timerError_);
	inputTimer_->async_wait (std::bind(&ScoutX::handleInput, this, std::placeholders::_1));
}

//*****************************************************************************
void ScoutX::SystemStatusTest (boost::system::error_code ec) {
	static uint32_t testCount = 0;


	SystemStatusData* systemStatus;
	GetSystemStatus (systemStatus);

	std::string str = boost::str (boost::format ("SystemStatus::status: %d [%d]") 
		% std::string(GetSystemStatusAsStr(systemStatus->status)) % ++testCount);
	std::cout << str << std::endl;

	FreeSystemStatus (systemStatus);

	systemstatusTestTimer_->expires_from_now (boost::posix_time::seconds(1));
	systemstatusTestTimer_->async_wait ([this](boost::system::error_code ec)->void {
		SystemStatusTest (ec);
	});
}

//*****************************************************************************
void ScoutX::StopSystemStatusTest() {

	systemstatusTestTimer_->cancel();
}

//*****************************************************************************
void ScoutX::waitForInit (const boost::system::error_code& ec) {

	// Did completion time out?
	if (ec == boost::asio::error::operation_aborted) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "waitForInit: canceled: " + ec.message());
		return;
	}

	InitializationState initializationState = IsInitializationComplete();
	switch (initializationState) {
		case eInitializationInProgress:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization In Progress...");
			std::cout << "Initialization In Progress..." << std::endl;
			initTimer_->expires_from_now (boost::posix_time::milliseconds (250));
			initTimer_->async_wait (std::bind (&ScoutX::waitForInit, this, std::placeholders::_1));
			return;

		case eInitializationFailed:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Failed...");
			std::cout << "Initialization Failed..." << std::endl;
			break;

		case eFirmwareUpdateInProgress:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Firmware Update In Progress...");
			std::cout << "Firmware Update In Progress..." << std::endl;
			initTimer_->expires_from_now (boost::posix_time::milliseconds (250));
			initTimer_->async_wait (std::bind (&ScoutX::waitForInit, this, std::placeholders::_1));
			return;

		case eInitializationComplete:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Complete...");
			std::cout << "Initialization Complete..." << std::endl;
			prompt();
			break;

		default:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Failed, Unknown Initialization Status...");
			std::cout << "Initialization Failed, Unknown Initialization Status..." << std::endl;
	}

	initTimeoutTimer_->cancel();
	cb_Handler_ (true);
}

//*****************************************************************************
void ScoutX::waitForInitTimeout (const boost::system::error_code& ec) {

	// Did completion timer get cancelled?
	if (ec == boost::asio::error::operation_aborted) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "waitForInitTimeout: cancelled");
		return;
	}

	initTimer_->cancel();

	Logger::L().Log (MODULENAME, severity_level::error, "waitForInitTimeout: init timed out");
	std::cout << "waitForInitTimeout: init timed out" << std::endl;

	cb_Handler_ (false);
}

//void ScoutX::getWorklistStatus (const boost::system::error_code& ec)
//{
//	uint32_t wqLength = 0;
//	WorklistItem* wqis = nullptr;
//	eSystemStatus status;
//	auto he = GetWorklistStatus(wqLength, wqis, status);
//	
//	if (he != HawkeyeError::eSuccess)
//	{
//		std::cout << "Failed to get the Worklist status" << std::endl;
//	}
//	else
//	{
//		std::cout << "WORK LIST STATUS " << std::string(GetWorklistStatusAsStr(status)) << std::endl;
//	}
//
//	getWqStatusTimer_->expires_from_now(boost::posix_time::seconds(2));
//	getWqStatusTimer_->async_wait(std::bind(&ScoutX::getWorklistStatus, this, std::placeholders::_1));
//}

//*****************************************************************************
void ScoutX::doWork_ (boost::function<void(bool)> cb_Handler) {

	cb_Handler_ = cb_Handler;

	Initialize (isHardwareAvailable_);

	initTimer_->expires_from_now (boost::posix_time::milliseconds (1));
	initTimer_->async_wait (std::bind (&ScoutX::waitForInit, this, std::placeholders::_1));
}

//*****************************************************************************
void ScoutX::runAsync_ (boost::function<void(bool)> cb_Handler) {

	strand_->post (std::bind (&ScoutX::doWork_, this, cb_Handler));
}

#define INIT_TIMEOUT 400000  // msecs
//*****************************************************************************
bool ScoutX::init (bool withHardware) {

	isHardwareAvailable_ = withHardware;

	strand_ = std::make_shared<boost::asio::io_context::strand>(io_svc);
	pio_thread_ = std::make_shared<std::thread>();

	// Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
	auto THREAD = std::bind (static_cast <std::size_t (boost::asio::io_context::*)(void)> (&boost::asio::io_context::run), &io_svc);
	pio_thread_.reset (new std::thread (THREAD));
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ScoutX::init:: thread_id: %08X") % pio_thread_.get()->get_id()));

	initTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc);
	initTimeoutTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc);
	systemstatusTestTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc);

	initTimeoutTimer_->expires_from_now (boost::posix_time::milliseconds (INIT_TIMEOUT));
	initTimeoutTimer_->async_wait (std::bind (&ScoutX::waitForInitTimeout, this, std::placeholders::_1));

	typedef boost::promise<bool> Promise_t;
	Promise_t promise;

	// Pass the handler to async operation that will set the promise.
	// *const uint32_t* is the return type of the promise.
	void (Promise_t::*setter)(const bool&) = &Promise_t::set_value;

	runAsync_ (boost::bind (setter, &promise, _1));

	// Synchronously wait for the future to finish.
	bool retStatus = promise.get_future().get();

	inputTimer_.reset (new boost::asio::deadline_timer(io_svc));
	inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
	inputTimer_->async_wait (std::bind (&ScoutX::handleInput, this, std::placeholders::_1));
	prompt();

	return retStatus;
}

void ScoutX::cb_ReagentDrainStatus (eDrainReagentPackState rls)
{
	std::cout << "Reagent pack unload status: " << GetReagentDrainStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void ScoutX::cb_ReagentPackLoadStatus (ReagentLoadSequence rls) {

	//if (he != eSuccess) {
	//	std::cout << "Reagent pack failed to load: " << GetErrorAsStr(he) << std::endl;
	//	return;
	//}

	std::cout << "Reagent pack load status: " << GetReagentPackLoadStatusAsStr(rls) << std::endl;

	//	std::cout << "Reagent pack successfully loaded..." << std::endl;

	uint32_t num_reagents;
	ReagentDefinition* reagents;

	GetReagentDefinitions (num_reagents, reagents);

	//TODO: print out the reagent definitions.

	FreeReagentDefinitions (reagents, num_reagents);
}

//*****************************************************************************
void ScoutX::cb_ReagentPackLoadComplete (ReagentLoadSequence rls) {

	std::cout << "Reagent pack load complete: " << GetReagentPackLoadStatusAsStr(rls) << std::endl;

	//uint32_t num_reagents;
	//ReagentDefinition* reagents;

	//GetReagentDefinitions (num_reagents, reagents);

	////TODO: print out the reagent definitions.

	//FreeReagentDefinitions (reagents, num_reagents);
}

//*****************************************************************************
void ScoutX::cb_ReagentPackUnloadStatus (ReagentUnloadSequence rls) {

	//if (he != eSuccess) {
	//	std::cout << "Reagent pack failed to unload: " << GetErrorAsStr(he) << std::endl;
	//	return;
	//}

	std::cout << "Reagent pack unload status: " << GetReagentPackUnloadStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
//*****************************************************************************
void ScoutX::cb_ReagentPackUnloadComplete (ReagentUnloadSequence rls) {

	//if (he != eSuccess) {
	//	std::cout << "Reagent pack failed to load: " << GetErrorAsStr(he) << std::endl;
	//	return;
	//}

	std::cout << "Reagent pack unload complete: " << GetReagentPackUnloadStatusAsStr(rls) << std::endl;

	//std::cout << "Reagent pack successfully unloaded..." << std::endl;

	//uint32_t num_reagents;
	//ReagentDefinition* reagents;

	//GetReagentDefinitions (num_reagents, reagents);

	////TODO: print out the reagent definitions.

	//FreeReagentDefinitions (reagents, num_reagents);
}

//*****************************************************************************
void ScoutX::cb_ReagentFlowCellFlush(eFlushFlowCellState rls)
{
	//std::cout << "Reagent pack unload status: " << GetReagentFlushFlowCellStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void ScoutX::cb_CleanFluidics(eFlushFlowCellState rls)
{
	std::cout << "Clean Fluidics status: " << GetReagentFlushFlowCellStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void ScoutX::cb_ReagentDecontaminateFlowCell(eDecontaminateFlowCellState rls)
{
	std::cout << "Reagent pack Decontaminate Flow Cell status: " << GetReagentDecontaminateFlowCellStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void ScoutX::onSampleStatus (SampleDefinition* sample) {

	std::cout << "*** Sample Status ***" << std::endl;

	pScoutX->displaySampleDefinition (*sample);
}

//*****************************************************************************
void ScoutX::onSampleComplete (SampleDefinition* sample) {

	std::cout << "*** Sample Completed ***" << std::endl;

	pScoutX->displaySampleDefinition (*sample);
}

//*****************************************************************************
void ScoutX::onWorklistComplete (uuid__t worklistId) {

	std::cout << "*** Worklist Completed ***" << std::endl;
}

//*****************************************************************************
void ScoutX::onSampleImageProcessed (SampleDefinition* sd,
	uint16_t imageSeqNum,					/* image sequence number */
	ImageSetWrapper_t* image,				/* image */
	BasicResultAnswers cumulativeResults,	/* cumulative */
	BasicResultAnswers imageResults) {		/* this_image */

	std::cout << "Image processing completed for image: " << imageSeqNum << std::endl;

}

//*****************************************************************************
void ScoutX::displaySampleDefinition (SampleDefinition& sample) {

	//std::cout << "         Label: " << wqi.label << std::endl;
	std::cout << "SampleSet UUID: " << Uuid::ToStr(sample.sampleSetUuid) << std::endl;
	std::cout << "   Sample UUID: " << Uuid::ToStr(sample.sampleDefUuid) << std::endl;
	std::cout << "     Data UUID: " << Uuid::ToStr(sample.sampleDataUuid) << std::endl;
	std::cout << "     TimeStamp: " << sample.timestamp << std::endl;
	std::cout << "         Label: " << (sample.parameters.label ? sample.parameters.label : "empty") << std::endl;
	std::cout << "           Tag: " << (sample.parameters.tag ? sample.parameters.tag : "empty") << std::endl;
	std::cout << "      Location: " << sample.position.row << "-" << (int)sample.position.col << std::endl;
	std::cout << "      BpQcName: " << (sample.parameters.bp_qc_name ? sample.parameters.bp_qc_name : "empty") << std::endl;
	std::cout << "     CellIndex: " << sample.parameters.celltypeIndex << std::endl;
	std::cout << "DilutionFactor: " << sample.parameters.dilutionFactor << std::endl;
	std::cout << "      PostWash: " << postWashToString(sample.parameters.postWash) << std::endl;
	std::cout << "Analysis Index: " << sample.parameters.analysisIndex << std::endl;
	std::cout << "        Status: " << GetSampleStatusAsStr(sample.status) << std::endl << std::endl;
}

//*****************************************************************************
void ScoutX::createSampleParameters (SampleParameters& sp,
	char* label,
	char* tag,
	char* bp_qc_name,
	uint16_t analysisIndex,
	uint32_t cellTypeIndex,
	uint32_t dilutionFactor,
	eSamplePostWash postWash,
	uint32_t saveEveryNthImage) {

	SetString (&sp.label, label);
	SetString (&sp.tag, tag);
	SetString (&sp.bp_qc_name, bp_qc_name);

	sp.analysisIndex = analysisIndex;    // Only one analysis, always 0x0000.
	sp.celltypeIndex = cellTypeIndex;
	sp.dilutionFactor = dilutionFactor;
	sp.postWash = postWash;
	sp.saveEveryNthImage = saveEveryNthImage;
}

//*****************************************************************************
void ScoutX::createSampleDefinition (
	uint16_t sampleSetIndex,
	uint16_t index,
	SampleDefinition& sd,
	SampleParameters& parameters,
	SamplePosition& position) {

	sd.sampleSetIndex = sampleSetIndex;
	sd.index = index;
	sd.parameters = parameters;
	sd.timestamp = 0;
	sd.sampleSetUuid = {};
	sd.sampleDefUuid = {};
	sd.sampleDataUuid = {};
	sd.position = position;
	SetString (&sd.username, (char*)currentUser_.c_str());
	HAWKEYE_ASSERT ("createSampleDefinition", sd.position.isValid());

	sd.status = eNotProcessed;    // Always for a new WQ item.
}

//*****************************************************************************
void ScoutX::createSampleSet (uint16_t index, SampleSet& ss, char* name, eCarrierType carrier, SampleDefinition* sampleDefinitions, uint32_t numSampleDefinitions) {

	ss.uuid = {};
	ss.index = index;
	ss.carrier = carrier;
	ss.numSamples = static_cast<uint16_t>(numSampleDefinitions);
	SetString (&ss.name, name);
	SetString (&ss.username, (char*)currentUser_.c_str());

	if (ss.numSamples > 0)
	{
		ss.samples = new SampleDefinition[ss.numSamples];

		for (uint32_t i = 0; i < numSampleDefinitions; i++)
		{
			ss.samples[i] = sampleDefinitions[i];
		}
	}
	else
	{
		ss.samples = nullptr;
	}
}

//*****************************************************************************
void ScoutX::createCarouselWorklist (Worklist& wl) {

	SampleSet orphanSampleSet;
	createSampleSet (curSampleSetIndex, orphanSampleSet, "Orphan SampleSet", eCarrierType::eCarousel, NULL, 0);
	curSampleSetIndex++;


	// SampleSet 1.
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample 1", "Group A", nullptr, 0x0000, 1, 1, eNormalWash, 15);
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample 2", "Group A", nullptr, 0x0000, 2, 1, eFastWash, 5);
	SampleParameters sp3;
	createSampleParameters (sp3, "Sample 3", "Group A", nullptr, 0x0000, 3, 1, eNormalWash, 10);

	uint32_t numSampleDefinitions = 1;
	SampleDefinition* sds1 = new SampleDefinition[numSampleDefinitions];
	//createSampleDefinition (1, 1, sds1[1], sp1, SamplePosition ('Z', 4));
	createSampleDefinition (1, 0, sds1[0], sp3, SamplePosition ('Z', 2));
	//createSampleDefinition (1, 2, sds1[2], sp2, SamplePosition ('Z', 1));

	SampleSet ss1;
	createSampleSet (curSampleSetIndex, ss1, "SampleSet 1", eCarrierType::eCarousel, sds1, numSampleDefinitions);
	curSampleSetIndex++;


	// SampleSet 3.
	SampleParameters sp6;
	createSampleParameters (sp6, "Sample 10", "Group B", nullptr, 0x0000, 3, 1, eNormalWash, 5);
	SampleParameters sp7;
	createSampleParameters (sp7, "Sample 12", "Group B", nullptr, 0x0000, 4, 1, eFastWash, 20);

	numSampleDefinitions = 2;
	SampleDefinition* sds3 = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (3, 0, sds3[0], sp6, SamplePosition ('Z', 12));
	createSampleDefinition (3, 1, sds3[1], sp7, SamplePosition ('Z', 10));

	SampleSet ss3;
	createSampleSet (3, ss3, "SampleSet 2", eCarrierType::eCarousel, sds3, numSampleDefinitions);
	curSampleSetIndex++;


	// SampleSet 2.
	SampleParameters sp4;
	createSampleParameters (sp4, "Sample 7", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	//createSampleParameters (sp4, "Sample 7", "Group B", 0x1111, 1, 1, eNormalWash, 5);		// This one has an invalid Analysis.
	SampleParameters sp5;
	createSampleParameters (sp5, "Sample 6", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	//createSampleParameters (sp5, "Sample 6", "Group B", 0x0000, 42, 1, eFastWash, 20);		// This one has an invalid CellType.

	numSampleDefinitions = 2;
	SampleDefinition* sds2 = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (2, 0, sds2[0], sp4, SamplePosition ('Z', 7));
	createSampleDefinition (2, 1, sds2[1], sp5, SamplePosition ('Z', 6));

	SampleSet ss2;
	createSampleSet (2, ss2, "SampleSet 3", eCarrierType::eCarousel, sds2, numSampleDefinitions);
	curSampleSetIndex++;


	SetString (&wl.username, "factory_admin");
	SetString (&wl.runUsername, "factory_admin");
	SetString (&wl.label,  "My Worklist");
	wl.timestamp = 0;
	wl.uuid = {};
	wl.carrier = eCarrierType::eCarousel;
	wl.precession = eColumnMajor;
	createSampleParameters (wl.defaultParameterSettings, "Default Sample", "N/A", nullptr, 0x0000, 1, 4, eFastWash, 1);

	wl.numSampleSets = 2; /* curSampleSetIndex;*/
	wl.sampleSets = new SampleSet[wl.numSampleSets];

	wl.sampleSets[0] = orphanSampleSet;
	wl.sampleSets[1] = ss1;
	//wl.sampleSets[2] = ss2;
	//wl.sampleSets[3] = ss3;

	wl.useSequencing = true;
	wl.sequencingTextFirst = true;
	SetString (&wl.sequencingBaseLabel, "Orphan");
	wl.sequencingStartingDigit = 42;
	wl.sequencingNumberOfDigits = 4;
}

//*****************************************************************************
void ScoutX::createCarouselSampleSet2 (SampleSet& ss)
{
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample 2", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	//createSampleParameters (sp1, "Sample 7", "Group B", 0x1111, 1, 1, eNormalWash, 5);		// This one has an invalid Analysis.
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample 1", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	//createSampleParameters (sp2, "Sample 6", "Group B", 0x0000, 42, 1, eFastWash, 20);		// This one has an invalid CellType.

	uint32_t numSampleDefinitions = 2;
	SampleDefinition* sds = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (curSampleSetIndex, 0, sds[0], sp1, SamplePosition ('Z', 1));
	createSampleDefinition (curSampleSetIndex, 1, sds[1], sp2, SamplePosition ('Z', 2));

	createSampleSet (curSampleSetIndex, ss, "createCarouselSampleSet2", eCarrierType::eCarousel, sds, numSampleDefinitions);
	curSampleSetIndex++;
}

//*****************************************************************************
void ScoutX::createCarouselSampleSet5 (SampleSet& ss)
{
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample 4", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample 5", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	SampleParameters sp3;
	createSampleParameters (sp3, "Sample 6", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp4;
	createSampleParameters (sp4, "Sample 7", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	SampleParameters sp5;
	createSampleParameters (sp5, "Sample 8", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);

	uint32_t numSampleDefinitions = 5;
	SampleDefinition* sds = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (curSampleSetIndex, 0, sds[0], sp1, SamplePosition ('Z', 4));
	createSampleDefinition (curSampleSetIndex, 1, sds[1], sp2, SamplePosition ('Z', 5));
	createSampleDefinition (curSampleSetIndex, 2, sds[2], sp3, SamplePosition ('Z', 6));
	createSampleDefinition (curSampleSetIndex, 3, sds[3], sp4, SamplePosition ('Z', 7));
	createSampleDefinition (curSampleSetIndex, 4, sds[4], sp5, SamplePosition ('Z', 8));

	createSampleSet (curSampleSetIndex, ss, "createCarouselSampleSet5", eCarrierType::eCarousel, sds, numSampleDefinitions);
	curSampleSetIndex++;
}

//*****************************************************************************
void ScoutX::createCarouselSampleSet10 (SampleSet& ss)
{
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample 10", "Group C", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample 11", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	SampleParameters sp3;
	createSampleParameters (sp3, "Sample 12", "Group A", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp4;
	createSampleParameters (sp4, "Sample 13", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp5;
	createSampleParameters (sp5, "Sample 14", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp6;
	createSampleParameters (sp6, "Sample 15", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp7;
	createSampleParameters (sp7, "Sample 16", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp8;
	createSampleParameters (sp8, "Sample 17", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp9;
	createSampleParameters (sp9, "Sample 18", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	SampleParameters sp10;
	createSampleParameters (sp10, "Sample 19", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);

	uint32_t numSampleDefinitions = 10;
	SampleDefinition* sds = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (curSampleSetIndex, 0, sds[0], sp1, SamplePosition ('Z', 10));
	createSampleDefinition (curSampleSetIndex, 1, sds[1], sp2, SamplePosition ('Z', 11));
	createSampleDefinition (curSampleSetIndex, 2, sds[2], sp3, SamplePosition ('Z', 12));
	createSampleDefinition (curSampleSetIndex, 3, sds[3], sp4, SamplePosition ('Z', 13));
	createSampleDefinition (curSampleSetIndex, 4, sds[4], sp5, SamplePosition ('Z', 14));
	createSampleDefinition (curSampleSetIndex, 5, sds[5], sp6, SamplePosition ('Z', 15));
	createSampleDefinition (curSampleSetIndex, 6, sds[6], sp7, SamplePosition ('Z', 16));
	createSampleDefinition (curSampleSetIndex, 7, sds[7], sp8, SamplePosition ('Z', 17));
	createSampleDefinition (curSampleSetIndex, 8, sds[8], sp9, SamplePosition ('Z', 18));
	createSampleDefinition (curSampleSetIndex, 9, sds[9], sp10, SamplePosition ('Z', 19));

	createSampleSet (curSampleSetIndex, ss, "createCarouselSampleSet10", eCarrierType::eCarousel, sds, numSampleDefinitions);
	curSampleSetIndex++;
}

//*****************************************************************************
void ScoutX::createEmptyCarouselWorklist (Worklist& wl)
{
	createSampleParameters (wl.defaultParameterSettings, "Default Sample", "", nullptr, 0x0000, 1, 4, eNormalWash, 1);

	SampleSet orphanSampleSet;
	createSampleSet (curSampleSetIndex, orphanSampleSet, "Orphan Carousel SampleSet", eCarrierType::eCarousel, NULL, 0);
	curSampleSetIndex++;

	wl.uuid = {};

	SetString (&wl.username, "factory_admin");
	SetString (&wl.runUsername, "factory_admin");
	//For testing: SetString (&wl.label, "Empty Carousel Worklist");
	SetString (&wl.label, "");
	wl.timestamp = 0;
	wl.carrier = eCarrierType::eCarousel;
	wl.precession = eColumnMajor;

	wl.numSampleSets = curSampleSetIndex;
	wl.sampleSets = new SampleSet[wl.numSampleSets];
	wl.sampleSets[0] = orphanSampleSet;

	wl.useSequencing = true;
	wl.sequencingTextFirst = true;
	SetString (&wl.sequencingBaseLabel, "Orphan");
	wl.sequencingStartingDigit = 42;
	wl.sequencingNumberOfDigits = 4;
}

//*****************************************************************************
void ScoutX::createEmptyPlateWorklist (Worklist& wl)
{
	SampleSet orphanSampleSet;
	createSampleSet (curSampleSetIndex, orphanSampleSet, "Orphan SampleSet", eCarrierType::ePlate_96, NULL, 0);
	curSampleSetIndex++;

	wl.uuid = {};

	SetString (&wl.username, "factory_admin");
	SetString (&wl.runUsername, "factory_admin");
	SetString (&wl.label, "Empty Plate Worklist");
	//SetString (&wl.label, "");
	wl.timestamp = 0;
	wl.carrier = eCarrierType::ePlate_96;
	wl.precession = eColumnMajor;
	
	wl.numSampleSets = curSampleSetIndex;
	wl.sampleSets = new SampleSet[wl.numSampleSets];
	wl.sampleSets[0] = orphanSampleSet;
	wl.sampleSets[0].index = 0;

	wl.useSequencing = true;
	wl.sequencingTextFirst = true;
	SetString (&wl.sequencingBaseLabel, "Orphan");
	wl.sequencingStartingDigit = 42;
	wl.sequencingNumberOfDigits = 4;
}

//*****************************************************************************
void ScoutX::createPlateSampleSet_3_6 (SampleSet& ss)
{
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample 6", "Group B", nullptr, 0x0000, 1, 1, eNormalWash, 5);
	//createSampleParameters (sp1, "Sample 7", "Group B", 0x1111, 1, 1, eNormalWash, 5);		// This one has an invalid Analysis.
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample 3", "Group B", nullptr, 0x0000, 2, 1, eFastWash, 20);
	//createSampleParameters (sp2, "Sample 6", "Group B", 0x0000, 42, 1, eFastWash, 20);		// This one has an invalid CellType.

	uint32_t numSampleDefinitions = 2;
	SampleDefinition* sds = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (curSampleSetIndex, 0, sds[0], sp1, SamplePosition ('A', 6));
	createSampleDefinition (curSampleSetIndex, 1, sds[1], sp2, SamplePosition ('A', 3));

	createSampleSet (curSampleSetIndex, ss, "createPlateSampleSet_3_6", eCarrierType::ePlate_96, sds, numSampleDefinitions);
	curSampleSetIndex++;
}

//*****************************************************************************
void ScoutX::createPlateWorklist_A1_A12_H1_H12 (Worklist& wl) {

	SampleSet orphanSampleSet;
	createSampleSet (curSampleSetIndex, orphanSampleSet, "Orphan Plate SampleSet", eCarrierType::ePlate_96, NULL, 0);
	curSampleSetIndex++;

	// SampleSet.
	SampleParameters sp1;
	createSampleParameters (sp1, "Sample A-1",  "Group A", nullptr, 0x0000, 1, 1, eNormalWash, 1);
	SampleParameters sp2;
	createSampleParameters (sp2, "Sample A-12", "Group A", nullptr, 0x0000, 2, 1, eFastWash, 15);
	SampleParameters sp3;
	createSampleParameters (sp3, "Sample H-1",  "Group A", nullptr, 0x0000, 3, 1, eNormalWash, 10);
	SampleParameters sp4;
	createSampleParameters (sp4, "Sample H-12", "Group A", nullptr, 0x0000, 3, 1, eNormalWash, 10);

	uint32_t numSampleDefinitions = 1;
	SampleDefinition* sds = new SampleDefinition[numSampleDefinitions];
	createSampleDefinition (curSampleSetIndex, 0, sds[0], sp1, SamplePosition ('A',  1));
	//createSampleDefinition (curSampleSetIndex, 3, sds[3], sp2, SamplePosition ('A', 12));
	//createSampleDefinition (curSampleSetIndex, 2, sds[2], sp3, SamplePosition ('H',  1));
	//createSampleDefinition (curSampleSetIndex, 1, sds[1], sp4, SamplePosition ('H', 12));

	SampleSet ss;
	createSampleSet (curSampleSetIndex, ss, "createPlateWorklist_A1_A12_H1_H12", eCarrierType::ePlate_96, sds, numSampleDefinitions);
	curSampleSetIndex++;

	SetString (&wl.username, "factory_admin");
	SetString (&wl.runUsername, "factory_admin");
	SetString (&wl.label, "Plate Worklist");
	wl.timestamp = 0;
	wl.uuid = {};
	wl.carrier = eCarrierType::ePlate_96;
	//wl.precession = eColumnMajor;
	wl.precession = eRowMajor;

	// This is here for consistency, it is not used for a plate.
	createSampleParameters (wl.defaultParameterSettings, "Default Sample", "N/A", nullptr, 0x0000, 1, 4, eFastWash, 1);

	// Plate worklist does not contain an orphan sample set.

	wl.numSampleSets = curSampleSetIndex;
	wl.sampleSets = new SampleSet[wl.numSampleSets];
	wl.sampleSets[0] = orphanSampleSet;
	wl.sampleSets[1] = ss;

	// Always set this information even though there is no orphan SampleSet.
	wl.useSequencing = true;
	wl.sequencingTextFirst = true;
	SetString (&wl.sequencingBaseLabel, "Orphan");
	wl.sequencingStartingDigit = 42;
	wl.sequencingNumberOfDigits = 4;
}

//*****************************************************************************
void ScoutX::setEmptySampleCupWorklist (Worklist& wl)
{
	SetString (&wl.username, "factory_admin");
	SetString (&wl.runUsername, "factory_admin");
	SetString (&wl.label, "Empty SampleCup Worklist");
	wl.timestamp = 0;
	wl.carrier = eCarrierType::eACup;
	wl.precession = eColumnMajor;
	createSampleParameters (wl.defaultParameterSettings, "Default Sample", "SampleCup", nullptr, 0x0000, 1, 4, eNormalWash, 1);

	// SampleCup worklist does not contain an orphan sample set.
}

//*****************************************************************************
static void deepCopyAnalysisDefinition (AnalysisDefinition& to, const AnalysisDefinition& from) {

	to.analysis_index = from.analysis_index;

	size_t len = strlen (from.label) + 1;
	to.label = new char[len];
	strncpy_s (to.label, len, from.label, strlen (from.label));


	to.num_reagents = from.num_reagents;
	if (to.num_reagents > 0) {
		to.reagent_indices = new uint32_t[to.num_reagents];
		//	memcpy (to.reagent_indices, sizeof(uint32_t)*to.num_reagents, (size_t)from.reagent_indices, sizeof(uint32_t)*to.num_reagents);
		for (uint8_t i = 0; i < to.num_reagents; i++) {
			to.reagent_indices[i] = from.reagent_indices[i];
		}
	}
	else {
		to.reagent_indices = nullptr;
	}
	to.mixing_cycles = from.mixing_cycles;

	to.num_fl_illuminators = from.num_fl_illuminators;
	if (to.num_fl_illuminators > 0) {
		to.fl_illuminators = new FL_IlluminationSettings[to.num_fl_illuminators];
		for (uint8_t i = 0; i < to.num_fl_illuminators; i++) {
			to.fl_illuminators[i].illuminator_wavelength_nm = from.fl_illuminators[i].illuminator_wavelength_nm;
			to.fl_illuminators[i].exposure_time_ms = from.fl_illuminators[i].exposure_time_ms;
		}
	}
	else {
		to.fl_illuminators = nullptr;
	}

	to.num_analysis_parameters = from.num_analysis_parameters;
	if (to.num_analysis_parameters > 0) {
		to.analysis_parameters = new AnalysisParameter[to.num_analysis_parameters];
		for (uint8_t i = 0; i < to.num_analysis_parameters; i++) {
			size_t len = strlen (from.analysis_parameters[i].label) + 1;
			to.analysis_parameters[i].label = new char[len];
			strncpy_s (to.analysis_parameters[i].label, len, from.analysis_parameters[i].label, strlen (from.analysis_parameters[i].label));
			to.analysis_parameters[i].characteristic.key = from.analysis_parameters[i].characteristic.key;
			to.analysis_parameters[i].characteristic.s_key = from.analysis_parameters[i].characteristic.s_key;
			to.analysis_parameters[i].characteristic.s_s_key = from.analysis_parameters[i].characteristic.s_s_key;
			to.analysis_parameters[i].threshold_value = from.analysis_parameters[i].threshold_value;
			to.analysis_parameters[i].above_threshold = from.analysis_parameters[i].above_threshold;
		}
	}
	else {
		to.analysis_parameters = nullptr;
	}

	if (from.population_parameter) {
		to.population_parameter = new AnalysisParameter;
		size_t len = strlen (from.population_parameter->label) + 1;
		to.population_parameter->label = new char[len];
		strncpy_s (to.population_parameter->label, len, from.population_parameter->label, len - 1);
		to.population_parameter->characteristic = from.population_parameter->characteristic;
		to.population_parameter->threshold_value = from.population_parameter->threshold_value;
		to.population_parameter->above_threshold = from.population_parameter->above_threshold;
	}
	else {
		to.population_parameter = nullptr;
	}
}

//*****************************************************************************
bool ScoutX::buildNewCellType (CellType*& ct) {

	ct = new CellType;

	ct->celltype_index = 0;
	std::string str = "New CellType";
	ct->label = new char[str.length () + 1];
	strcpy_s (ct->label, (str.length () + 1), str.c_str ());
	ct->max_image_count = 50;
	ct->minimum_diameter_um = 3;
	ct->maximum_diameter_um = 5;
	ct->minimum_circularity = 4;
	ct->sharpness_limit = 8;
	ct->num_cell_identification_parameters = 1;

	ct->cell_identification_parameters = new AnalysisParameter[ct->num_cell_identification_parameters];
	AnalysisParameter* p_ap = ct->cell_identification_parameters;
	std::string temp = "Cell ID Param #1";
	p_ap->label = new char[temp.length () + 1];
	strcpy_s (p_ap->label, temp.length () + 1, "Cell ID Param #1");
	p_ap->characteristic.key = 8;
	p_ap->characteristic.s_key = 9;
	p_ap->characteristic.s_s_key = 10;
	p_ap->threshold_value = (float)3.33;
	p_ap->above_threshold = false;

	ct->decluster_setting = eDCHigh;
	ct->fl_roi_extent = 0.5;

	ct->num_analysis_specializations = 1;
	ct->analysis_specializations = new AnalysisDefinition[ct->num_analysis_specializations];

	// Use one of the existing analyses.
	uint32_t num_ad = 0;
	AnalysisDefinition* analysesList = nullptr;
	HawkeyeError he = GetAllAnalysisDefinitions (num_ad, analysesList);
	if (he != HawkeyeError::eSuccess) {
		return false;
	}

	// Use the first one in the returned list.
	deepCopyAnalysisDefinition (*ct->analysis_specializations, analysesList[0]);

	FreeAnalysisDefinitions (analysesList, num_ad);

	return true;
}

//*****************************************************************************
bool ScoutX::buildModifiedCellType (CellType*& ct) {

	ct = new CellType;

	ct->celltype_index = 0x80000001;
	std::string str = "Modified CellType";
	ct->label = new char[str.length () + 1];
	strcpy_s (ct->label, (str.length () + 1), str.c_str ());
	ct->max_image_count = 50;
	ct->minimum_diameter_um = 3;
	ct->maximum_diameter_um = 5;
	ct->minimum_circularity = 4;
	ct->sharpness_limit = 8;
	ct->num_cell_identification_parameters = 1;

	ct->cell_identification_parameters = new AnalysisParameter[ct->num_cell_identification_parameters];
	AnalysisParameter* p_ap = ct->cell_identification_parameters;
	strcpy_s (p_ap->label, sizeof (p_ap->label) + 1, "Cell ID Param #1");
	p_ap->characteristic.key = 8;
	p_ap->characteristic.s_key = 9;
	p_ap->characteristic.s_s_key = 10;
	p_ap->threshold_value = (float)3.33;
	p_ap->above_threshold = false;

	ct->decluster_setting = eDCHigh;
	ct->fl_roi_extent = 0.5;

	ct->num_analysis_specializations = 1;
	ct->analysis_specializations = new AnalysisDefinition[ct->num_analysis_specializations];

	// Use one of the existing analyses.
	uint32_t num_ad = 0;
	AnalysisDefinition* analysesList = nullptr;
	HawkeyeError he = GetAllAnalysisDefinitions (num_ad, analysesList);
	if (he != HawkeyeError::eSuccess) {
		return false;
	}

	// Use the first one in the returned list.
	deepCopyAnalysisDefinition (*ct->analysis_specializations, analysesList[0]);

	FreeAnalysisDefinitions (analysesList, num_ad);

	return true;
}

//*****************************************************************************
bool ScoutX::buildAnalysisDefinition (AnalysisDefinition*& ad) {

	// Use one of the existing analyses.
	uint32_t num_ad = 0;
	AnalysisDefinition* analysesList = nullptr;
	HawkeyeError he = GetAllAnalysisDefinitions (num_ad, analysesList);
	if (he != HawkeyeError::eSuccess) {
		return false;
	}

	ad = new AnalysisDefinition;

	// Use the first one in the returned list.
	deepCopyAnalysisDefinition (*ad, analysesList[0]);

	FreeAnalysisDefinitions (analysesList, num_ad);

	// Make some changes.
	ad->analysis_index = 0;
	std::string str = "New Analysis Definition";
	strcpy_s (ad->label, (str.length () + 1), str.c_str ());
	str = "New Result Unit";
	strcpy_s (ad->label, (str.length () + 1), str.c_str ());

	return true;
}

//*****************************************************************************
int main (int argc, char* argv[]) {

	std::shared_ptr<boost::asio::io_context::work> io_svcwork;
	io_svcwork.reset (new boost::asio::io_context::work(io_svc));

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "ScoutXTest.info", "logger");

	ScoutX scoutX(io_svc);

	bool withHardware;
	if (argc > 1) {
		withHardware = true;
	} else {
		withHardware = false;
	}

	if (scoutX.init (withHardware)) {
		Logger::L().Log (MODULENAME, severity_level::normal, "ScoutX system initialized...");
	} else {
		Logger::L().Log (MODULENAME, severity_level::error, "ScoutX system failed to initialize...");
	}

	scoutX.loginUser ("factory_admin", "Vi-CELL#01");
	//scoutX.loginUser( ServiceUser, ServicePwd );

	//boost::optional<uint8_t> fred;
	//fred = boost::none;

	Logger::L().Flush();

	io_svc.run();


	return 0;
}
