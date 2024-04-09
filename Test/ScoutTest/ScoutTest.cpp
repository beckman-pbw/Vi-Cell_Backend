#include "stdafx.h"

using namespace std;

#include <stdint.h>
#include <conio.h>
#include <functional>
#include <io.h>  
#include <ios>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "HawkeyeError.hpp"
#include "HawkeyeLogic.hpp"
#include "Logger.hpp"
#include "ScoutTest.hpp"
#include "StageDefines.hpp"
#include "SystemStatusCommon.hpp"

using namespace std;

static const char MODULENAME[] = "ScoutTest";

Scout* Scout::pScout;

static boost::asio::io_context io_svc_;
static std::shared_ptr<std::thread> pio_thread_;
static std::shared_ptr<boost::asio::io_context::strand> strand_;
static boost::function<void(bool)> cb_Handler_;

static uint32_t timeout_msecs_;
static std::shared_ptr<boost::asio::deadline_timer> initTimer_;
static std::shared_ptr<boost::asio::deadline_timer> initTimeoutTimer_;
static std::shared_ptr<boost::asio::deadline_timer> systemstatusTestTimer_;


// For command line input.
#define ESC 0x1B
#define BACKSPACE '\b'
#define CR '\r'
#define NEWLINE '\n'
static std::string smValueLine_;

typedef enum eStateMachine : uint32_t {
	smCmdEntry,
	smAdminOptions,
	smAdminAddUser,
	smAdminSetUserFolder,
	smAdminGetUserCellTypeIndices,
	smAdminGetUserAnalysisIndices,
	smAdminGetUserProperty,
	smAdminChangeUserPassword,
	smAdminGetCurrentUser,
	smAdminSetUserProperty,
	smAdminSetUserDisplayName,
	smAdminSetUserCellTypeIndices,
	smAdminImportConfiguration,
	smAdminExportConfiguration,
	smAdminExportInstrumentData,
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
	smBPOptions,
	smCellOptions,
	smQCOptions,
	smWorkQueueOptions,
	smMiscOptions,
	smMiscGetSystemStatus,
	smMiscRetrieveAuditTrailLogRange,
	smMiscTestSystemErrorCodeExpandedStrings,
} StateMachine_t;

static std::shared_ptr<boost::asio::deadline_timer> inputTimer_;
static boost::system::error_code timerError_;
static StateMachine_t smState_ = smCmdEntry;
static StateMachine_t smStateOption_;

typedef enum eCommand : char {
	cmdAnalysis = 'A',
	cmdBP = 'B',
	cmdCells = 'C',
	cmdAdmin = 'D',
	cmdMiscellaneous = 'M',
	cmdQC = 'Q',
	cmdReagentPack = 'R',
	cmdService = 'S',
	cmdResults = 'T',
	cmdUser = 'U',
	cmdWorkQueue = 'W',
	cmdQuit = 'X',
	cmdHelp = '?',
} Command_t;

static std::string menu =
std::string(1, cmdAnalysis) + " | " +
std::string(1, cmdBP) + " | " +
std::string(1, cmdCells) + " | " +
std::string(1, cmdAdmin) + " | " +
std::string(1, cmdMiscellaneous) + " | " +
std::string(1, cmdQC) + " | " +
std::string(1, cmdReagentPack) + " | " +
std::string(1, cmdService) + " | " +
std::string(1, cmdResults) + " | " +
std::string(1, cmdUser) + " | " +
std::string(1, cmdWorkQueue) + " | " +
std::string(1, cmdQuit) + " | " +
std::string(1, cmdHelp)
;

static Command_t smCmd_;
static std::vector<std::string> parameterStack_;
static std::string loggedInUserPassword_;
static WorkQueue* currentWorkQueue_;

static bool isHardwareAvailable_ = false;

static boost::optional<uuid__t> uuidForRetrieval;


//*****************************************************************************
Scout::Scout (boost::asio::io_context& io_s) {

	Scout::pScout = this;
}

//*****************************************************************************
Scout::~Scout() {

}

//*****************************************************************************
void Scout::loginUser (std::string username, std::string password) {

	parameterStack_.push_back (username);
	parameterStack_.push_back (password);

	HawkeyeError he = LoginUser (parameterStack_[0].c_str(), parameterStack_[1].c_str());
	if (he != HawkeyeError::eSuccess) {
		std::cout << "Login failed: " << GetErrorAsStr(he) << std::endl;

	}
	else {
		std::cout << "Login successful as \"" << username << "\"..." << std::endl;
		currentUser_ = parameterStack_[0];
		loggedInUserPassword_ = parameterStack_[1];

		uint32_t nCells = 0;
		NativeDataType tag;
		uint32_t* user_cell_indices;
		he = GetMyCellTypeIndices (nCells, tag, user_cell_indices);
		for (uint32_t i = 0; i < nCells; i++) {
			std::cout << "User Cell Type: " << std::to_string(user_cell_indices[i]) << std::endl;
		}

		if (he == HawkeyeError::eSuccess) {
			FreeTaggedBuffer (tag, user_cell_indices);
		}
	}
	parameterStack_.clear();
}

//*****************************************************************************
void Scout::prompt() {

	switch (smState_) {
		default:
		case smCmdEntry:
			std::cout << menu << ": ";
			parameterStack_.clear();
			break;
		case smAnalysisOptions:
			std::cout << " GetFactoryAnalysisDefinitions(1) | GetUserAnalysisDefinitions(2) | GetAllAnalysisDefinitions(3) | AddAnalysisDefinition(4) | ModifyAnalysis(5): " << std::endl;
			std::cout << " RemoveAnalysisDefinition(6) | GetSupportedAnalysisParameterNames(7) | GetSupportedAnalysisCharacteristics(8) | GetNameForCharacteristic(9): " << std::endl;
			std::cout << " ModifyBaseAnalysisDefinition(10):";
			break;
		case smBPOptions:
			std::cout << " AddBioprocess(1) | RemoveBioprocess(2) | GetBioprocessList(3) | GetActiveBioprocessList(4) | SetBioprocessActivation(5): ";
			break;
		case smCellOptions:
			std::cout << " GetFactoryCellTypes(1) | GetUserCellTypes(2) | GetAllCellTypes(3) | AddCellType(4) | ModifyCellType (5): " << std::endl;
			std::cout << " RemoveCellType(6): ";
			break;
		case smQCOptions:
			std::cout << " AddQualityControl(1) | RemoveQualityControl(2) | GetQualityControlList(3) | GetActiveQualityControlList(4): ";
			break;
		case smReagentPackOptions:
			std::cout << " Load(1) | Unload(2) | GetReagentDefinitions(3) | GetReagentContainerStatus(4) | GetReagentContainerStatusAll(5) " << std::endl;;
			std::cout << " StartFlushFlowCell(6) | CancelFlushFlowCell(7) | StartDecontaminateFlowCell(8) | CancelDecontaminateFlowCell(9) " << std::endl;;
			std::cout << " StartReagentPackDrain(10) | CancelReagentPackDrain(11): " << std::endl;;
			break;
		case smReagentPackDrain_Option:
			std::cout << "Valve Position to Drain (just one cycle). (1-5): ";
			break;
		case smReagentPackUnload_Option_0:
			std::cout << "How Many Containers to Unload (0-3): ";
			break;
		case smReagentPackUnload_Option_1:
			std::cout << "None(0) | Drain(1) | Purge Only(2): ";
			break;
		case smServiceOptions:
			std::cout << "GetCameraLampState(1) | SetCameraLampState(2) " << std::endl;
			std::cout << "MoveSampleProbeUp(3) | MoveSampleProbeDown(4) " << std::endl;
			std::cout << "MoveReagentArmUp(5) | MoveReagentArmDown(6) : " << std::endl;
			break;
		case smResultsOptions:
			std::cout << "RetrieveWorkQueues(1) | RetrieveWorkQueueList(2) | RetrieveWorkQueue(3): " << std::endl;
			std::cout << "RetrieveSampleRecords(4) | RetrieveSampleRecordList(5) | RetrieveSampleRecord(6): " << std::endl;
			std::cout << "RetrieveSampleImageSetRecords(7) | RetrieveSampleImageSetRecordList(8) | RetrieveSampleImageSetRecord(9): " << std::endl;
			std::cout << "RetrieveImageRecords(10) | RetrieveImageRecordList(11) | RetrieveImageRecord(12): " << std::endl;
			std::cout << "RetrieveResultRecords(13) | RetrieveResultRecordList(14) | RetrieveResultRecord(15): " << std::endl;
			std::cout << "RetrieveDetailedMeasurementsForResultRecord(16) | RetrieveHistogramForResultRecord(17): ";
			break;
		case smAdminOptions:
			std::cout << " AddUser(1) | SetUserFolder(2) | GetUserCellTypes(3) | GetUserAnalysisDefinitions (4) | GetUserProperty (5)" << std::endl;
			std::cout << " ChangeUserPasword(6) | GetCurrentUser(7) | Shutdown(8) | SetUserProperty(9) | SetUserDisplayName" << std::endl;
			std::cout << " SetUserCellTypeIndices(11) | ImportConfiguration(12) | ExportConfiguration(13) | ExportInstrumentData(14): ";
			break;
		case smUserOptions:
			std::cout << " Login(1)  | Logout(2) | ListAll(3)            | ChangePassword(4) | GetCellTypeIndices(5)" << std::endl;
			std::cout << " GetAnalysisIndices(6) | GetDisplayName(7)     | GetFolder(8)      | GetMyProperty(9)" << std::endl;
			std::cout << " GetMyProperties(10)   | GetUserPermissions(11)| GetSystemStatus(12) " << std::endl;;
			std::cout << " >> ";
			break;
		case smWorkQueueOptions:
			std::cout << " Set(1) | Status(2) | Start(3) | Stop(4) | Pause(5) | Resume(6) | SkipItem(7) | GetCurrentItem(8): ";
			break;
		case smMiscOptions:
			std::cout << " GetSystemStatus(1)                | RetrieveAuditTrailLogRange(2)      | ArchiveAuditTrailLog(3)" << std::endl;
			std::cout << " ReadArchivedAuditTrailLog(4)      | RetrieveInstrumentErrorLogRange(5) | ArchiveInstrumentErrorLog(6)" << std::endl;
			std::cout << " ReadArchivedInstrumentErrorLog(7) | RetrieveSampleActivityLogRange(8)  | ArchiveSampleActivityLog(9)" << std::endl;
			std::cout << " ReadArchivedSampleActivityLog(10) | SyringePumpValveTest(11)           | RetrieveCalibrationActivityLogRange(12)" << std::endl;
			std::cout << " SetConcentrationCalibration(13)   | SystemStatusTest(14)               | StopSystemStatusTest(15)" << std::endl;
			std::cout << " CreateFactoryUserListInfoFile(16) | TestSystemErrorResourceList(17)    |" << std::endl;
			std::cout << " >> ";
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
			std::cout << "Reagent pack number: ";
			break;
	}
}

//*****************************************************************************
void Scout::showHelp() {
	/*
	cmdAnalysis = 'A',
	cmdBP = 'B',
	cmdCells = 'C',
	cmdAdmin = 'D',
	cmdMiscellaneous = 'M',
	cmdQC = 'Q',
	cmdReagentPack = 'R',
	cmdService = 'S',
	cmdResults = 'T',
	cmdUser = 'U',
	cmdWorkQueue = 'W',
	cmdQuit = 'X',
	cmdHelp = '?',
	*/
	std::cout << std::endl << "Usage:\t" << menu << std::endl;
	std::cout << "\t" << (char)cmdAnalysis << " : Analysis commands" << std::endl;
	std::cout << "\t" << (char)cmdCells << " : CellType commands" << std::endl;
	std::cout << "\t" << (char)cmdBP << " : BioProcess commands" << std::endl;
	std::cout << "\t" << (char)cmdQC << " : QualityControl commands" << std::endl;
	std::cout << "\t" << (char)cmdAdmin << " : Admin commands" << std::endl;
	std::cout << "\t" << (char)cmdResults << " : Results commands" << std::endl;
	std::cout << "\t" << (char)cmdMiscellaneous << " : Misc commands" << std::endl;
	std::cout << "\t" << (char)cmdReagentPack << " : ReagentPack commands" << std::endl;
	std::cout << "\t" << (char)cmdService << " : Service commands" << std::endl;
	std::cout << "\t" << (char)cmdUser << " : User commands" << std::endl;
	std::cout << "\t" << (char)cmdWorkQueue << " : WorkQueue commands" << std::endl;
	std::cout << "\t" << (char)cmdQuit <<" : Exit the program" << std::endl;
	std::cout << "\t" << (char)cmdHelp << " : Display this help screen" << std::endl;
	std::cout << std::endl << std::endl;
}

void Scout::print_reagents_definitions(ReagentDefinition *& reagents, size_t num_reagents)
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

void Scout::print_reagents_containerstates(ReagentContainerState *& reagents)
{
	cout << "Reagent Pack BCI# : ";
	std::string bci(reagents->bci_part_number);
	cout << bci << std::endl;
	cout << "   Reagent Identifier       : " << reagents->identifier << endl;
	cout << "   Events Remaining         : " << reagents->events_remaining << endl;
	cout << "   Pack expiration          : " << reagents->exp_date << endl;
	cout << "   In-service date          : " << reagents->in_service_date << endl;
	cout << "   Reagent lot information  : " << std::string(reagents->lot_information) << endl;
	cout << "   Reagent Pack Position    : " << reagents->position << endl;
	cout << "   Reagent Pack health      : " << reagents->status << endl;;

	cout << "----------------------------- " << endl;
	for (auto i = 0; i<reagents->num_reagents; i++) {
		cout << "   Reagent lot information  : " << std::string(reagents->reagent_states[i].lot_information) << endl;
		cout << "   Reagent Index            : " << std::to_string(reagents->reagent_states[i].reagent_index) << endl;
		cout << "   Reagent Valve Number     : " << std::to_string(reagents->reagent_states[i].valve_location) << endl;
		cout << "   Reagent Events Remaining : " << std::to_string(reagents->reagent_states[i].events_remaining) << endl;
		cout << "   Reagent Events Possible  : " << std::to_string(reagents->reagent_states[i].events_possible) << endl;
		cout << endl;
	}
}

//*****************************************************************************
void Scout::handleInput (const boost::system::error_code& error) {
	static uint16_t num_containters;
	static std::vector<ReagentContainerUnloadOption> unloadActions;
	HawkeyeError he = HawkeyeError::eSuccess;


	if (error) {
		if (error == boost::asio::error::operation_aborted) {
			Logger::L().Log (MODULENAME, severity_level::debug2, "handleInput:: boost::asio::error::operation_aborted");
		}
		return;
	}

	if (!_kbhit()) {
		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
		inputTimer_->async_wait (std::bind(&Scout::handleInput, this, std::placeholders::_1));
		return;
	}

	int C = _getch();
	if (isprint(C)) {
		std::cout << (char)C;

		int CUPPER = toupper (C);
		if (CUPPER == cmdQuit) {
			Shutdown();

			while ( !IsShutdownComplete () )
			{
				::Sleep (200);
			}

			HWND mHwnd = NULL;
			mHwnd = GetConsoleWindow();
			std::cout << endl << "Detected quit request in input" << std::endl;
			PostMessage (mHwnd, WM_CLOSE, 0, 0);
			return;

		}
		else if (C == cmdHelp) {
			showHelp();
			smState_ = smCmdEntry;
			smValueLine_.clear();
			inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
			inputTimer_->async_wait (std::bind(&Scout::handleInput, this, std::placeholders::_1));
			return;
		}

		smValueLine_ += (char)C;
		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
		inputTimer_->async_wait (std::bind(&Scout::handleInput, this, std::placeholders::_1));
		return;

	} else {

		if ((C != ESC) && (C != CR) && (C != NEWLINE)) {
			if (C == BACKSPACE) {
				if (smValueLine_.length() > 0) {
					std::cout << BACKSPACE;
					std::cout << (char)' ';
					std::cout << BACKSPACE;
					smValueLine_.pop_back();
				}
			}

		} else {

			if (C == ESC) {
				smState_ = smCmdEntry;
				smValueLine_.clear();
				std::cout << std::endl;
				prompt();

			}
			else if (C == CR || C == NEWLINE) {
				std::cout << std::endl;
				goto PROCESS_STATE;
			}
		}

		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
		inputTimer_->async_wait (std::bind(&Scout::handleInput, this, std::placeholders::_1));
		return;
	}

	//	std::cout << std::endl << smValueLine_ << std::endl;

PROCESS_STATE:
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
				case cmdBP:
					smState_ = smBPOptions;
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
				case cmdWorkQueue:
					smState_ = smWorkQueueOptions;
					prompt();
					break;
				case cmdMiscellaneous:
					smState_ = smMiscOptions;
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
					smStateOption_ = smAdminSetUserFolder;
					smState_ = smUsernameEntry;
					break;
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
					smState_ = smAdminGetCurrentUser;
					goto PROCESS_STATE;
				case 8:
					Shutdown();
					std::cout << "Hawkeye DLL Shutdown Complete" << std::endl;
					while (!IsShutdownComplete()) {
						std::cout << "Shutting down..." << std::endl;
						Sleep (500);
					}
					break;
				case 9:
					smStateOption_ = smAdminSetUserProperty;
					smState_ = smUsernameEntry;
					break;
				case 10:
					smStateOption_ = smAdminSetUserDisplayName;
					smState_ = smUsernameEntry;
					break;
				case 11:
					// Currently, this deletes the CellType for the specified user.
					smStateOption_ = smAdminSetUserCellTypeIndices;
					smState_ = smUsernameEntry;
					break;
				case 12:
					smStateOption_ = smAdminImportConfiguration;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 13:
					smStateOption_ = smAdminExportConfiguration;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
				case 14:
					smStateOption_ = smAdminExportInstrumentData;
					smState_ = smStateOption_;
					goto PROCESS_STATE;
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

				case 2:
					smState_ = smUserLogout;
					break;

				case 3:
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

				case 4:
					smStateOption_ = smUserChangeMyPassword;
					smState_ = smPasswordEntry;
					break;

				case 5:
				{
					uint32_t nCells = 0;
					NativeDataType tag;
					uint32_t* user_cell_indices;
					he = GetMyCellTypeIndices (nCells, tag, user_cell_indices);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get user's cell types: " << GetErrorAsStr(he) << std::endl;
					} else {
						for (uint32_t i = 0; i < nCells; i++) {
							std::cout << "User Cell Type: " << std::hex << std::to_string(user_cell_indices[i]) << std::endl;
						}
						FreeTaggedBuffer(tag, &user_cell_indices);
					}
					break;
				}

				case 6:
				{
					uint32_t nAnalyses = 0;
					NativeDataType tag;
					uint16_t* user_analysis_indices;
					he = GetMyAnalysisIndices (nAnalyses, tag, user_analysis_indices);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get user's analyses: " << GetErrorAsStr(he) << std::endl;
					} else {
						for (uint32_t i = 0; i < nAnalyses; i++) {
							std::cout << "User Analysis: " << std::hex << ::to_string(user_analysis_indices[i]) << std::endl;
						}
						FreeTaggedBuffer(tag, &user_analysis_indices);
					}
					break;
				}

				case 7:
				{
					char* displayName = nullptr;

					he = GetMyDisplayName (displayName);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get user's display name: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Display name: " << displayName << std::endl;
						FreeCharBuffer (displayName);
					}
					break;
				}

				case 8:
				{
					char* folderName = nullptr;

					he = GetMyFolder (folderName);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get user's folder: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Folder: " << folderName << std::endl;
						FreeCharBuffer (folderName);
					}
					break;
				}

				case 9:
					smStateOption_ = smUserGetProperty;
					smState_ = smPropertyEntry;
					break;

				case 10:
				{
					uint32_t nProps = 0;
					UserProperty* properties;

					he = GetMyProperties (properties, nProps);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve user properties: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved user properties." << std::endl;
						for (uint32_t i = 0; i < nProps; i++) {
							std::cout << "Property[" << i << "]: " << properties->propertyname << ", nProps: " << std::to_string(properties->num_members) << std::endl;
							for (uint32_t j = 0; j < properties->num_members; j++) {
								std::cout << "\t[" << j << "]: " << properties->members[j] << std::endl;
							}
						}
						FreeUserProperties (properties, nProps);
					}
					break;
				}
				case 11:
				{
					smStateOption_ = smUserGetPermissions;
					smState_ = smUsernameEntry;
					break;
				}

				case 12:
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
			parameterStack_.push_back (smValueLine_);

			switch (smStateOption_) {
				case smAdminAddUser:
				{
					UserPermissionLevel permissions = UserPermissionLevel::eNormal;

					he = AddUser (parameterStack_[0].c_str(), parameterStack_[2].c_str(), parameterStack_[1].c_str(), permissions);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new user: " << GetErrorAsStr(he) << std::endl;
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

		case smUsernameEntry:
		{
			parameterStack_.push_back (smValueLine_);
			switch (smStateOption_) {
				case smUserLogin:
					smState_ = smPasswordEntry;
					break;
				case smAdminGetUserProperty:
					smState_ = smPropertyEntry;
					break;
				case smAdminChangeUserPassword:
					smState_ = smPasswordEntry;
					break;
				case smAdminAddUser:
					smState_ = smPasswordEntry;
					break;
				case smAdminSetUserFolder:
					smState_ = smFoldernameEntry;
					break;
				case smAdminSetUserProperty:
					smState_ = smPropertyEntry;
					break;
				case smAdminSetUserDisplayName:
					smState_ = smDisplaynameEntry;
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

			switch (smStateOption_) {
				case smUserLogin:
				{
					he = LoginUser (parameterStack_[0].c_str(), parameterStack_[1].c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Login failed: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Login successful..." << std::endl;
						currentUser_ = parameterStack_[0];
						loggedInUserPassword_ = parameterStack_[1];

						uint32_t nCells = 0;
						NativeDataType tag;
						uint32_t* user_cell_indices;
						he = GetMyCellTypeIndices (nCells, tag, user_cell_indices);
						for (uint32_t i = 0; i < nCells; i++) {
							std::cout << "User Cell Type: " << std::to_string(user_cell_indices[i]) << std::endl;
						}

						if (he == HawkeyeError::eSuccess) {
							FreeTaggedBuffer(tag, &user_cell_indices);
						}

					}
					smState_ = smCmdEntry;
					break;
				}

				case smUserChangeMyPassword:
					he = ChangeMyPassword (loggedInUserPassword_.c_str(), parameterStack_[0].c_str());
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to password: " << GetErrorAsStr(he) << std::endl;
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

			switch (smStateOption_) {
				case smAdminSetUserFolder:
					smState_ = smStateOption_;
					goto PROCESS_STATE;
			}
			break;

		case smPropertyEntry:
			parameterStack_.push_back (smValueLine_);
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

			char* propList;
			propList = new char[smValueLine_.length() + 1];
			strcpy_s (propList, smValueLine_.length() + 1, smValueLine_.c_str());
			he = SetUserProperty (parameterStack_[0].c_str(), parameterStack_[1].c_str(), (uint32_t)parameterStack_.size() - 2, &propList);
			free (propList);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to user property \"" << parameterStack_[1] << "\": " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully user property \"" << parameterStack_[1] << "\"" << std::endl;
			}

			smState_ = smCmdEntry;
			break;

		case smUserLogout:
		{
			LogoutUser();
			smState_ = smCmdEntry;
			break;
		}

		case smUserGetProperty:
		{
			uint32_t nProps = 0;
			char** properties;

			he = GetMyProperty (parameterStack_[0].c_str(), nProps, properties);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to retrieve \"" << parameterStack_[0] << "\"" << " property: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully retrieved \"" << parameterStack_[0] << "\"" << " property" << std::endl;
				std::cout << "Property: " << parameterStack_[0].c_str() << std::endl;
				for (uint32_t i = 0; i < nProps; i++) {
					std::cout << "[" << i << "]: " << properties[i] << std::endl;
				}
				FreeListOfCharBuffers (properties, nProps);
			}

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
			uint32_t nCells = 0;
			NativeDataType tag;
			uint32_t* user_cell_indices;

			he = GetUserCellTypeIndices (parameterStack_[0].c_str(), nCells, tag, user_cell_indices);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to retrieve celltype indices for \"" << parameterStack_[0] << "\": " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully retrieved celltype indices for \"" << parameterStack_[0] << "\"" << std::endl;
				FreeTaggedBuffer(tag, &user_cell_indices);
			}

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
			uint32_t nProps = 0;
			char** properties;

			he = GetUserProperty (parameterStack_[0].c_str(), parameterStack_[1].c_str(), nProps, properties);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to retrieve \"" << parameterStack_[1] << "\"" << " property for \"" << parameterStack_[0] << "\": " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully retrieved \"" << parameterStack_[1] << "\"" << " property for \"" << parameterStack_[0] << "\"" << std::endl;
				for (uint32_t i = 0; i < nProps; i++) {
					std::cout << "value[" << i << "]: " << properties[i] << std::endl;
				}
				FreeListOfCharBuffers (properties, nProps);
			}

			smState_ = smCmdEntry;
			break;
		}

		case smAdminChangeUserPassword:
			he = ChangeUserPassword (parameterStack_[0].c_str(), parameterStack_[1].c_str());
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to change password: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully changed password..." << std::endl;
			}

			smState_ = smCmdEntry;
			break;

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
			char filename[] = "c:\\Instrument\\Export\\InstrumentConfiguration.cfg";
			he = ImportInstrumentConfiguration (filename);
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
			char filename[] = "c:\\Instrument\\Export\\InstrumentConfiguration.cfg";
			he = ExportInstrumentConfiguration (filename);
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
			/*char* exportFilename;
			he = ExportInstrumentData ("C:\\Instrument\\Export", exportFilename, true);
			if (he != HawkeyeError::eSuccess) {
				std::cout << "Failed to export configuration: " << GetErrorAsStr(he) << std::endl;
			} else {
				std::cout << "Successfully exported data to " << exportFilename << std::endl;
			}

			smState_ = smCmdEntry;*/
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

				default:
					std::cout << "Invalid entry..." << std::endl;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			if (val != 10 && val != 2) smState_ = smCmdEntry; //cmd 10 & 2 option need input from the user
			break;
		}

		case smReagentPackDrain_Option:
		{
			uint8_t container = atoi(smValueLine_.c_str());
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
			num_containters = atoi(smValueLine_.c_str());
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
			LoginUser ("bci_service", "916348");

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
			}

			smState_ = smCmdEntry;
			break;
		}

		case smResultsOptions:
		{
			switch (atoi (smValueLine_.c_str())) {

				// WorkQueue Records.
				case 1:
				{
					WorkQueueRecord* reclist;
					uint32_t list_size;
					he = RetrieveWorkQueues (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve WorkQueues: " << GetErrorAsStr(he) << std::endl;
					} else {
						uuidForRetrieval = reclist[0].uuid;
						std::cout << "Successfully retrieved WorkQueues..." << std::endl;
						FreeListOfWorkQueueRecord (reclist, list_size);
					}
					break;
				}

				case 2:
				{
					if (!uuidForRetrieval.is_initialized()) {
						std::cout << "No UUID specified, call RetrieveWorkQueues first: " << GetErrorAsStr(he) << std::endl;

					} else {
						WorkQueueRecord* reclist;
						uint32_t list_size;

						he = RetrieveWorkQueueList (&uuidForRetrieval.get(), 1, reclist, list_size);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
						}
						else {
							uuidForRetrieval = reclist[0].uuid;
							std::cout << "Successfully retrieved WorkQueue..." << std::endl;
							FreeListOfWorkQueueRecord (reclist, list_size);
						}
					}
					break;
				}

				case 3:
				{
					if (!uuidForRetrieval.is_initialized()) {
						std::cout << "No UUID specified, call RetrieveWorkQueues first: " << GetErrorAsStr(he) << std::endl;

					} else {
						WorkQueueRecord* reclist;
						he = RetrieveWorkQueue (uuidForRetrieval.get(), reclist);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
						}
						else {
							std::cout << "Successfully retrieved WorkQueue..." << std::endl;
							FreeListOfWorkQueueRecord (reclist, 1);
						}
					}
					break;
				}

				// Sample Records.
				case 4:
				{
					SampleRecord* reclist;
					uint32_t list_size;
					he = RetrieveSampleRecords (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Samples: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Samples..." << std::endl;
						FreeListOfSampleRecord (reclist, list_size);
					}
					break;
				}

				case 5:
				{
					SampleRecord* reclist;
					uint32_t list_size;
					he = RetrieveSampleRecordList (&uuidForRetrieval.get(), 1, reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else {
						uuidForRetrieval = reclist[0].uuid;
						std::cout << "Successfully retrieved WorkQueue..." << std::endl;
						FreeListOfSampleRecord (reclist, list_size);
					}
					break;
				}

				case 6:
				{
					SampleRecord* reclist;
					he = RetrieveSampleRecord (uuidForRetrieval.get(), reclist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved WorkQueue..." << std::endl;
						FreeListOfSampleRecord (reclist, 1);
					}
					break;
				}

				// SampleImageSet Records.
				case 7:
				{
					SampleImageSetRecord* reclist;
					uint32_t list_size;
					he = RetrieveSampleImageSetRecords (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, list_size);
					}
					break;
				}

				case 8:
				{
					SampleImageSetRecord* reclist;
					uint32_t list_size;

					//TODO: where does this UUID come from???
					he = RetrieveSampleImageSetRecordList (&uuidForRetrieval.get(), 1, reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					} else {
						uuidForRetrieval = reclist[0].uuid;
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, list_size);
					}
					break;
				}

				case 9:
				{
					SampleImageSetRecord* reclist;
					//TODO: where does this UUID come from???
					he = RetrieveSampleImageSetRecord (uuidForRetrieval.get(), reclist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Sample Image Set Records: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Sample Image Set Records..." << std::endl;
						FreeListOfImageSetRecord (reclist, 1);
					}
					break;
				}


				// Image Records.
				case 10:
				{
					ImageRecord* reclist;
					uint32_t list_size;
					he = RetrieveImageRecords (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Image Records: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Image Records..." << std::endl;
						FreeListOfImageRecord (reclist, list_size);
					}
					break;
				}

				case 11:
				{
					ImageRecord* reclist;
					uint32_t list_size;

					//TODO: where does this UUID come from???
					he = RetrieveImageRecordList (&uuidForRetrieval.get(), 1, reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Image Record List: " << GetErrorAsStr(he) << std::endl;
					} else {
						uuidForRetrieval = reclist[0].uuid;
						std::cout << "Successfully retrieved Image Record List..." << std::endl;
						FreeListOfImageRecord (reclist, list_size);
					}
					break;
				}

				case 12:
				{
					ImageRecord* reclist;
					//todo: where does this uuid come from???
					he = RetrieveImageRecord (uuidForRetrieval.get(), reclist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "failed to retrieve image record: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "successfully retrieved image record..." << std::endl;
						FreeListOfImageRecord (reclist, 1);
					}
					break;
				}

				// Result Records.
				case 13:
				{
					ResultSummary* reclist;
					uint32_t list_size;
					he = RetrieveResultSummaries (0, 0, "bci_admin", reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Result Summaries: " << GetErrorAsStr(he) << std::endl;
					} else {
						uuidForRetrieval = reclist[0].uuid;
						std::cout << "Successfully retrieved Result Summaries..." << std::to_string(list_size) << std::endl;

						FreeListOfResultSummary (reclist, list_size);
					}
					break;
				}

				case 14:
				{
					ResultSummary* reclist;
					uint32_t list_size;
					he = RetrieveResultSummaryList (&uuidForRetrieval.get(), 1, reclist, list_size);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Result Summary List: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Result Summary List..." << std::endl;
						FreeListOfResultSummary(reclist, list_size);
					}
				}

				case 15:
				{
					ResultRecord* reclist;
					he = RetrieveResultRecord (uuidForRetrieval.get(), reclist);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Result Record: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Result Record..." << std::endl;
						FreeListOfResultRecord (reclist, 1);
					}
				}

				case 16:
				{
					if (!uuidForRetrieval.is_initialized()) {
						std::cout << "No UUID specified, call RetrieveResultRecords first: " << GetErrorAsStr(he) << std::endl;

					} else {
						DetailedResultMeasurements* detailedResultMeasurements;

						he = RetrieveDetailedMeasurementsForResultRecord (uuidForRetrieval.get(), detailedResultMeasurements);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to retrieve Detailed Measurements for Result Record: " << GetErrorAsStr(he) << std::endl;
						} else {
							std::cout << "Successfully retrieved Detailed Measurements for Result Record..." << std::endl;
							FreeDetailedResultMeasurement (detailedResultMeasurements);
						}
					}
				}

				case 17:
				{
					if (!uuidForRetrieval.is_initialized()) {
						std::cout << "No UUID specified, call RetrieveResultRecords first: " << GetErrorAsStr(he) << std::endl;

					} else {
						uint8_t bin_count = 8;
						histogrambin_t* bins;
						Hawkeye::Characteristic_t measurement = {21, 0, 0};	// Avg Spot Brightness

						he = RetrieveHistogramForResultRecord (uuidForRetrieval.get(), false, measurement, bin_count, bins);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to retrieve Histogram for Result Record: " << GetErrorAsStr(he) << std::endl;
						} else {
							std::cout << "Successfully retrieved Histogram for Result Record..." << std::endl;
							for (uint8_t i = 0; i < bin_count; i++)
							{
								std::cout << "bin nominal value " << bins[i].bin_nominal_value << std::endl << "Count " << bins[i].count << std::endl;
							}
							FreeHistogramBins (bins);
						}
					}
				}

				
				//HawkeyeError HawkeyeLogicImpl::RetrieveImage(uuid__t id, ImageWrapper_t*& img)
				//HawkeyeError HawkeyeLogicImpl::RetrieveAnnotatedImage(uuid__t result_id, uuid__t image_id, ImageWrapper_t*& img)
				//HawkeyeError HawkeyeLogicImpl::RetrieveBWImage(uuid__t image_id, ImageWrapper_t*& img)
				//HawkeyeError HawkeyeLogicImpl::RetrieveSampleRecordsForBPandQC (const char* bp_qc_name, SampleRecord*& reclist, uint32_t& list_size)
				//HawkeyeError HawkeyeLogicImpl::RetrieveSignatureDefinitions (DataSignature_t*& signatures, uint16_t& num_signatures)
				//HawkeyeError HawkeyeLogicImpl::SignResultRecord (uuid__t record_id, char* signature_short_text, uint16_t short_text_len)

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
					he = GetFactoryAnalysisDefinitions (num_ad, analysesList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve factory analyses: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved factory analyses..." << std::endl;
						FreeAnalysisDefinitions (analysesList, num_ad);
					}
					break;

				case 2:
					he = GetUserAnalysisDefinitions (num_ad, analysesList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve user analyses: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved user analyses..." << std::endl;
						FreeAnalysisDefinitions (analysesList, num_ad);
					}
					break;

				case 3:
					he = GetAllAnalysisDefinitions (num_ad, analysesList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve all analyses: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved all analyses..." << std::endl;
						FreeAnalysisDefinitions (analysesList, num_ad);
					}
					break;

				case 4:
				{
					uint16_t newAnalysisIndex = 0;
					AnalysisDefinition* analysis;
					buildAnalysisDefinition (analysis);
					he = AddAnalysisDefinition (*analysis, newAnalysisIndex);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new analysis type: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully added analysis... new index:" << std::to_string(newAnalysisIndex) << std::endl;
						FreeAnalysisDefinitions (analysesList, num_ad);
					}
					free (analysis);
					break;
				}

				case 5:
					std::cout << "Invalid entry..." << std::endl;
					he = HawkeyeError::eEntryInvalid;
					break;

				case 6:

					//TODO: should there be a *RemoveAnalysisDefinition*?
					//he = RemoveAnalysisDefinition (0x80000000);
					//if (he != HawkeyeError::eSuccess) {
					//	std::cout << "Failed to retrieve remove cell type: " << GetErrorAsStr(he) << std::endl;
					//}
					//else {
					//	std::cout << "Successfully removed cell type..." << std::endl;
					//}
					std::cout << "Invalid entry..." << std::endl;
					he = HawkeyeError::eEntryInvalid;
					break;

				case 7:
				{
					uint32_t nparameters;
					char** parameters;
					he = GetSupportedAnalysisParameterNames (nparameters, parameters);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Analysis parameter names: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Analysis parameter names..." << std::endl;
						for (uint32_t i = 0; i < nparameters; i++) {
							std::cout << "parameter[" << i << "]: " << parameters[i] << std::endl;
						}
						FreeListOfCharBuffers (parameters, nparameters);
					}
				}
				break;

				case 8:
				{
					uint32_t ncharacteristics;
					Hawkeye::Characteristic_t* characteristics;
					he = GetSupportedAnalysisCharacteristics (ncharacteristics, characteristics);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve Analysis characteristics: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved Analysis characteristics..." << std::endl;
						for (uint32_t i = 0; i < ncharacteristics; i++) {
							std::cout << "characteristic[" << i << "]: " << characteristics[i].key << ":" << characteristics[i].s_key << ":" << characteristics[i].s_s_key << std::endl;
						}
					}
				}
				break;

				case 9:
				{
					Hawkeye::Characteristic_t hc;
					hc.key = 20;
					hc.s_key = 0;
					hc.s_s_key = 0;
					char* name = GetNameForCharacteristic (hc);
					if (!name) {
						std::cout << "Failed to retrieve name for characteristic: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved name for characteristic: " << name << std::endl;
						if (name) {
							free (name);
							name = nullptr;
						}
					}

					hc.key = 42;
					hc.s_key = 0;
					hc.s_s_key = 0;
					name = GetNameForCharacteristic (hc);
					if (!name) {
						std::cout << "Failed to retrieve name for characteristic" << std::endl;
					} else {
						std::cout << "Successfully retrieved name for characteristic: " << name << std::endl;
						if (name) {
							free (name);
							name = nullptr;
						}
					}

				}
				break;

				case 10:
				{
					uint32_t num_ad = 0;
					AnalysisDefinition* analysesList = nullptr;

					he = GetFactoryAnalysisDefinitions (num_ad, analysesList);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve factory analyses: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved factory analyses..." << std::endl;
					}

					if (num_ad) {
						analysesList[0].label[0] = 'Z';
						analysesList[0].mixing_cycles = 5;

						he = ModifyBaseAnalysisDefinition (analysesList[0], false);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to modify factory analysis[0]: " << GetErrorAsStr(he) << std::endl;
						} else {
							std::cout << "Successfully modified factory analysis[0]..." << std::endl;
						}
						FreeAnalysisDefinitions (analysesList, num_ad);
					}
				}
				break;

				default:
					std::cout << "Invalid entry..." << std::endl;
					he = HawkeyeError::eEntryInvalid;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

		case smBPOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
				{
					Bioprocess_t bioProcess = {
						"BioProcess #1",
						"ReactorName",
						5,
// GMT: Thursday, June 14, 2018 7:37:53 PM
// Your time zone : Thursday, June 14, 2018 1 : 37 : 53 PM GMT - 06 : 00 DST
						1529005073 / 60,	
						true,
						"comment text"
					};

					he = AddBioprocess (bioProcess);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new BioProcess: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully added BioProcess" << std::endl;
					}
					break;
				}
				case 2:
				{
					he = RemoveBioprocess ("BioProcess #1");
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to remove BioProcess: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully removed BioProcess" << std::endl;
					}
					break;
				}
				case 3:
				{
					Bioprocess_t* bioprocesses;
					uint32_t num_bioprocesses;

					he = GetBioprocessList (bioprocesses, num_bioprocesses);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to Get BioProcess List: " << GetErrorAsStr(he) << std::endl;
					} else {
						FreeListOfBioprocess (bioprocesses, num_bioprocesses);
						std::cout << "Successfully Get BioProcess List" << std::endl;
					}
					break;
				}
				case 4:
				{
					Bioprocess_t* bioprocesses;
					uint32_t num_bioprocesses;

					he = GetActiveBioprocessList (bioprocesses, num_bioprocesses);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to Get Active BioProcess List: " << GetErrorAsStr(he) << std::endl;
					} else {
						FreeListOfBioprocess (bioprocesses, num_bioprocesses);
						std::cout << "Successfully Get Active BioProcess List" << std::endl;
					}
					break;
				}
				case 5:
				{
					he = SetBioprocessActivation ("BioProcess #1", false);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to Activate BioProcess: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully Activated the BioProcess" << std::endl;
					}
					break;
				}

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
					he = GetUserCellTypes (num_ct, cellTypeList);
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
					he = AddCellType (*cellType, newCellTypeIndex);
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
					CellType* cellType;
					buildModifiedCellType (cellType);
					he = ModifyCellType (*cellType);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add modify cell type: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully modified cell type.." << std::endl;
					}
					free (cellType);
					break;
				}
				case 6:
					// Remove the cell type added with AddCellType.
					he = RemoveCellType (0x80000000);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve remove cell type: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully removed cell type..." << std::endl;
					}
					break;
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

					he = AddQualityControl (qualityControl);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to add new QualityControl: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully added QualityControl" << std::endl;
					}
					break;
				}
				case 2:
				{
					he = RemoveQualityControl ("QC #1");
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to remove QualityControl: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully removed QualityControl" << std::endl;
					}
					break;
				}
				case 3:
				{
					QualityControl_t* qualitycontrols;
					uint32_t num_qcs;

					he = GetQualityControlList (qualitycontrols, num_qcs);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrieve QualityControl list: " << GetErrorAsStr(he) << std::endl;
					} else {
						FreeListOfQualityControl (qualitycontrols, num_qcs);
						std::cout << "Successfully retrieved QualityControl list" << std::endl;
					}
					break;
				}
				case 4:
				{
					QualityControl_t* qualitycontrols;
					uint32_t num_qcs;

					he = GetActiveQualityControlList (qualitycontrols, num_qcs);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to retrive active QualityControl list: " << GetErrorAsStr(he) << std::endl;
					} else {
						FreeListOfQualityControl (qualitycontrols, num_qcs);
						std::cout << "Successfully retrieved active QualityControl list" << std::endl;
					}
					break;
				}

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

		case smWorkQueueOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 1:
					WorkQueue* workQueue;
					if (buildCarouselWq(workQueue)) {
						he = SetWorkQueue (*workQueue);
						if (he != HawkeyeError::eSuccess) {
							std::cout << "Failed to set WorkQueue: " << GetErrorAsStr(he) << std::endl;
						} else {
							std::cout << "Successfully set WorkQueue..." << std::endl;
							free (workQueue);
						}
					} else {
						std::cout << "Failed to build WorkQueue: " << std::endl;
					}
					break;
				case 2:
					uint32_t queueLen;
					WorkQueueItem* wq;
					eWorkQueueStatus wqStatus;

					he = GetSystemStatus (queueLen, wq, wqStatus);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get WorkQueue status: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully retrieved WorkQueue status..." << std::endl;
						std::cout << "WorkQueue status: " << GetWorkQueueStatusAsStr (wqStatus);
						for (uint32_t i = 0; i < queueLen; i++) {
							displayWorkQueueItem (wq[i]);
						}
						FreeWorkQueueItem (wq, queueLen);
					}
					break;
				case 3:
					he = StartWorkQueue (cb_WorkQueueItemStatus, cb_WorkQueueItemComplete, cb_WorkQueueComplete, cb_WorkQueueItemImageProcessed);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to start WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else 
					{
						if (!getWqStatusTimer_)
						{
							getWqStatusTimer_ = std::make_shared<boost::asio::deadline_timer>(io_svc_);
							getWorkQueueStatus(boost::system::error_code());
						}
						std::cout << "Successfully started WorkQueue..." << std::endl;
					}
					break;
				case 4:
					he = StopQueue();
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to stop WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully stopped WorkQueue..." << std::endl;
					}
					break;
				case 5:
					he = PauseQueue();
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to pause WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully paused WorkQueue..." << std::endl;
					}
					break;
				case 6:
					he = ResumeQueue();
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to resume WorkQueue: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully resumed WorkQueue..." << std::endl;
					}
					break;
				case 7:
					he = SkipCurrentWorkQueueItem();
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to skip current WorkQueue item: " << GetErrorAsStr(he) << std::endl;
					} else {
						std::cout << "Successfully skipped current WorkQueue item..." << std::endl;
					}
					break;
				case 8:
					WorkQueueItem* wqi;
					he = GetCurrentWorkQueueItem (wqi);
					if (he != HawkeyeError::eSuccess) {
						std::cout << "Failed to get current WorkQueue item: " << GetErrorAsStr(he) << std::endl;
					} else {
						displayWorkQueueItem (*wqi);
						FreeWorkQueueItem (wqi, 1);
					}
					break;
				default:
					smState_ = smCmdEntry;
					std::cout << "Invalid entry..." << std::endl;

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
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

					LoginUser ("bci_service", "916348");

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
					LoginUser ("bci_service", "916348");

					double slope = 5.0;
					double intercept = 10.0;
					uint32_t cal_image_count =100;
					uuid__t queue_id = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
					uint16_t num_consumables = 1;
					calibration_consumable consumables = {"consumable #1", "12345", 20000, 22.2};

					he = SetConcentrationCalibration (slope, intercept, cal_image_count, queue_id, num_consumables, &consumables);

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
#ifdef _DEBUG
					CreateFactoryUserListInfoFile();
#else
					std::cout << "CreateFactoryUserListInfoFile only supported in DEBUG build" << std::endl;
#endif
					break;
				}
				case 17:
				{
					uint32_t errorcode = 2198011906;
					char *sys, *subsys, *instance, *failure, *severity;

					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);

					errorcode = 2164588801;
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);

					errorcode = 3238134786;
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);

					errorcode = 0xc1031305; // Should have a number of spaces removed from the failure / instance.
					SystemErrorCodeToExpandedResourceStrings(errorcode, severity, sys, subsys, instance, failure);

					std::cout << "System error code: " << errorcode << std::endl;
					std::cout << "\tSeverity:  \"" << severity << "\"" << std::endl;
					std::cout << "\tSystem:    \"" << sys << "\"" << std::endl;
					std::cout << "\tSubsystem: \"" << subsys << "\"" << std::endl;
					std::cout << "\tInstance:  \"" << instance << "\"" << std::endl;
					std::cout << "\tFailure:   \"" << failure << "\"" << std::endl;
					std::cout << std::endl;

					FreeCharBuffer(sys);
					FreeCharBuffer(subsys);
					FreeCharBuffer(instance);
					FreeCharBuffer(failure);
					FreeCharBuffer(severity);
					break;
				}

				default:
					std::cout << "Invalid entry..." << std::endl;

			} // End "switch (atoi (smValueLine_.c_str()))"

			smState_ = smCmdEntry;
			break;
		}

	} // End "switch (smState_)"

	smValueLine_.clear();

	if (smState_ == smCmdEntry) {
		smValueLine_.clear();
		prompt();
	}

	inputTimer_->expires_from_now(boost::posix_time::milliseconds(100), timerError_);
	inputTimer_->async_wait (std::bind(&Scout::handleInput, this, std::placeholders::_1));
}

//*****************************************************************************
void Scout::SystemStatusTest (boost::system::error_code ec) {
	static uint32_t testCount = 0;


	SystemStatusData* systemStatus;
	GetSystemStatus (systemStatus);
	std::string str = boost::str (boost::format ("SystemStatus::health: %d [%d]") % std::to_string(systemStatus->health) % ++testCount);
	std::cout << str << std::endl;
	FreeSystemStatus (systemStatus);

	systemstatusTestTimer_->expires_from_now (boost::posix_time::seconds(1));
	systemstatusTestTimer_->async_wait ([this](boost::system::error_code ec)->void {
		SystemStatusTest (ec);
	});
}

//*****************************************************************************
void Scout::StopSystemStatusTest() {

	systemstatusTestTimer_->cancel();
}

//*****************************************************************************
void Scout::waitForInit (const boost::system::error_code& ec) {

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
			initTimer_->async_wait (std::bind (&Scout::waitForInit, this, std::placeholders::_1));
			return;

		case eInitializationFailed:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Failed...");
			std::cout << "Initialization Failed..." << std::endl;
			break;

		case eFirmwareUpdateInProgress:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Firmware Update In Progress...");
			std::cout << "Firmware Update In Progress..." << std::endl;
			initTimer_->expires_from_now (boost::posix_time::milliseconds (250));
			initTimer_->async_wait (std::bind (&Scout::waitForInit, this, std::placeholders::_1));
			return;

		case eInitializationComplete:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Complete...");
			std::cout << "Initialization Complete..." << std::endl;
			break;

		default:
			Logger::L().Log (MODULENAME, severity_level::debug1, "Initialization Failed, Unknown Initialization Status...");
			std::cout << "Initialization Failed, Unknown Initialization Status..." << std::endl;
	}

	initTimeoutTimer_->cancel();
	cb_Handler_ (true);
}

//*****************************************************************************
void Scout::waitForInitTimeout (const boost::system::error_code& ec) {

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

void Scout::getWorkQueueStatus(const boost::system::error_code& ec)
{
	uint32_t wqLength = 0;
	WorkQueueItem* wqis = nullptr;
	eWorkQueueStatus status;
	auto he = GetSystemStatus(wqLength, wqis, status);
	
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "Failed to get the WQ status" << std::endl;
	}
	else
	{
		std::cout << "WORK QUEUE STATUS " << std::string(GetWorkQueueStatusAsStr(status)) << std::endl;
	}

	getWqStatusTimer_->expires_from_now(boost::posix_time::seconds(2));
	getWqStatusTimer_->async_wait(std::bind(&Scout::getWorkQueueStatus, this, std::placeholders::_1));
}

//*****************************************************************************
void Scout::doWork_ (boost::function<void(bool)> cb_Handler) {

	cb_Handler_ = cb_Handler;

	Initialize (isHardwareAvailable_);

	initTimer_->expires_from_now (boost::posix_time::milliseconds (1));
	initTimer_->async_wait (std::bind (&Scout::waitForInit, this, std::placeholders::_1));
}

//*****************************************************************************
void Scout::runAsync_ (boost::function<void(bool)> cb_Handler) {

	strand_->post (std::bind (&Scout::doWork_, this, cb_Handler));
}

#define INIT_TIMEOUT 400000  // msecs
//*****************************************************************************
bool Scout::init (bool withHardware) {

	isHardwareAvailable_ = withHardware;

	strand_ = std::make_shared<boost::asio::io_context::strand>(io_svc_);
	pio_thread_ = std::make_shared<std::thread>();

	// Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
	auto THREAD = std::bind (static_cast <std::size_t (boost::asio::io_context::*)(void)> (&boost::asio::io_context::run), &io_svc_);
	pio_thread_.reset (new std::thread (THREAD));
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Scout::init:: thread_id: %08X") % pio_thread_.get()->get_id()));

	initTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc_);
	initTimeoutTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc_);
	systemstatusTestTimer_ = std::make_shared <boost::asio::deadline_timer> (io_svc_);

	initTimeoutTimer_->expires_from_now (boost::posix_time::milliseconds (INIT_TIMEOUT));
	initTimeoutTimer_->async_wait (std::bind (&Scout::waitForInitTimeout, this, std::placeholders::_1));

	typedef boost::promise<bool> Promise_t;
	Promise_t promise;

	// Pass the handler to async operation that will set the promise.
	// *const uint32_t* is the return type of the promise.
	void (Promise_t::*setter)(const bool&) = &Promise_t::set_value;

	runAsync_ (boost::bind (setter, &promise, _1));

	// Synchronously wait for the future to finish.
	bool retStatus = promise.get_future().get();

	inputTimer_.reset (new boost::asio::deadline_timer(io_svc_));
	inputTimer_->expires_from_now (boost::posix_time::milliseconds(100), timerError_);
	inputTimer_->async_wait (std::bind (&Scout::handleInput, this, std::placeholders::_1));
	prompt();

	return retStatus;
}

void Scout::cb_ReagentDrainStatus(eDrainReagentPackState rls)
{
	std::cout << "Reagent pack unload status: " << GetReagentDrainStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void Scout::cb_ReagentPackLoadStatus (ReagentLoadSequence rls) {

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
void Scout::cb_ReagentPackLoadComplete (ReagentLoadSequence rls) {

	std::cout << "Reagent pack load complete: " << GetReagentPackLoadStatusAsStr(rls) << std::endl;

	//uint32_t num_reagents;
	//ReagentDefinition* reagents;

	//GetReagentDefinitions (num_reagents, reagents);

	////TODO: print out the reagent definitions.

	//FreeReagentDefinitions (reagents, num_reagents);
}

//*****************************************************************************
void Scout::cb_ReagentPackUnloadStatus (ReagentUnloadSequence rls) {

	//if (he != eSuccess) {
	//	std::cout << "Reagent pack failed to unload: " << GetErrorAsStr(he) << std::endl;
	//	return;
	//}

	std::cout << "Reagent pack unload status: " << GetReagentPackUnloadStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void Scout::cb_ReagentPackUnloadComplete (ReagentUnloadSequence rls) {

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

void Scout::cb_ReagentFlowCellFlush(eFlushFlowCellState rls)
{
	//std::cout << "Reagent pack unload status: " << GetReagentFlushFlowCellStatusAsStr(rls) << std::endl;
}

void Scout::cb_ReagentDecontaminateFlowCell(eDecontaminateFlowCellState rls)
{
	std::cout << "Reagent pack Decontaminate Flow Cell status: " << GetReagentDecontaminateFlowCellStatusAsStr(rls) << std::endl;
}

//*****************************************************************************
void Scout::cb_WorkQueueItemStatus (WorkQueueItem* wqi, uuid__t uuid) {

	//TODO: return code???

	std::cout << "*** WorkQueue Item Status ***" << std::endl;

	pScout->displayWorkQueueItem (*wqi);
}

//*****************************************************************************
void Scout::cb_WorkQueueItemComplete (WorkQueueItem* wqi, uuid__t uuid) {

	std::cout << "*** WorkQueue Item completed ***" << std::endl;

	pScout->displayWorkQueueItem (*wqi);
}

//*****************************************************************************
void Scout::cb_WorkQueueComplete (uuid__t workQueueId) {

	std::cout << "*** WorkQueue Completed ***" << std::endl;
}

//*****************************************************************************
void Scout::cb_WorkQueueItemImageProcessed (WorkQueueItem* wqi,
	uint16_t imageSeqNum,					/* image sequence number */
	ImageSetWrapper_t* image,				/* image */
	BasicResultAnswers cumulativeResults,	/* cumulative */
	BasicResultAnswers imageResults) {		/* this_image */

//	std::cout << "WorkQueue Item image processing completed..." << std::endl;

}

//*****************************************************************************
static std::string carrierTypeToString (eCarrierType st) {
	return (st == eCarousel ? "Carousel" : "Plate_96");
}

//*****************************************************************************
static std::string precessionToString (ePrecession pr) {
	return (pr == eRowMajor ? "RowMajor" : "ColumnMajor");
}

//*****************************************************************************
static std::string sampleStatusToString (eSampleStatus st) {
	switch (st) {
		case eNotProcessed:
			return "NotProcessed";
		case eInProcess_Aspirating:
			return "eInProcess_Aspirating";
		case eInProcess_Mixing:
			return "eInProcess_Mixing";
		case eInProcess_ImageAcquisition:
			return "eInProcess_ImageAcquisition";
		case eInProcess_Cleaning:
			return "eInProcess_Cleaning";
		case eCompleted:
			return "Completed";
		case eSkip_Error:
			return "SkipError";
		default:
			return "UknownSampleStatus";
	}
}

//*****************************************************************************
static std::string postWashToString (eSamplePostWash pw) {
	return (pw == eNormalWash ? "Normal" : "Fast");
}

//*****************************************************************************
static std::string boolToString (bool val) {
	return (val ? "True" : "False");
}

//*****************************************************************************
static std::string analysisParameterToString (const AnalysisParameter& ap) {

	std::cout << "         Label: " << ap.label << std::endl;
	std::cout << " hresholdValue: " << ap.threshold_value << std::endl;
	std::cout << "AboveThreshold: " << boolToString(ap.above_threshold) << std::endl;
}

//*****************************************************************************
static std::string flIlluminatorToString (const FL_IlluminationSettings& fli) {

	std::cout << "IlluminatorWavelength: " << fli.illuminator_wavelength_nm << " nm" << std::endl;
	std::cout << " ExposureTime: " << fli.exposure_time_ms << " ms" << std::endl;
}

//*****************************************************************************
static std::string analysisDefinitionToString (const AnalysisDefinition& ad) {

	std::cout << "  AnalysisIndex: " << ad.analysis_index << std::endl;
	std::cout << "          Label: " << ad.label << std::endl;

	std::cout << "     # Reagents: " << ad.num_reagents << std::endl;
	for (uint8_t i = 0; i < ad.num_reagents; i++) {
		std::cout << "   [" << i << "]: " << ad.reagent_indices[i] << std::endl;
	}
	std::cout << "   MixingCycles: " << ad.mixing_cycles << std::endl;
	std::cout << "# FL Parameters: " << ad.num_fl_illuminators << std::endl;
	for (uint8_t i = 0; i < ad.num_reagents; i++) {
		std::cout << "   [" << i << "]: " << flIlluminatorToString(ad.fl_illuminators[i]) << std::endl;
	}
}

//*****************************************************************************
void Scout::displayWorkQueueItem (WorkQueueItem& wqi) {

	//std::cout << "         Label: " << wqi.label << std::endl;
	std::cout << "       Comment: " << wqi.comment << std::endl;
	std::cout << "      Location: " << wqi.location.row << "-" << (int)wqi.location.col << std::endl;
	std::cout << "     CellIndex: " << wqi.celltypeIndex << std::endl;
	std::cout << "DilutionFactor: " << wqi.dilutionFactor << std::endl;
	std::cout << "      PostWash: " << postWashToString(wqi.postWash) << std::endl;
	std::cout << "Analysis Index: " << wqi.analysisIndex << std::endl;
	std::cout << "  SampleStatus: " << sampleStatusToString(wqi.status) << std::endl << std::endl;
}

//*****************************************************************************
int main (int argc, char* argv[]) {

	std::shared_ptr<boost::asio::io_context::work> io_svc_work;
	io_svc_work.reset (new boost::asio::io_context::work(io_svc_));

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "ScoutTest.info", "logger");

	Scout scout(io_svc_);

	bool withHardware;
	if (argc > 1) {
		withHardware = true;
	} else {
		withHardware = false;
	}

	if (scout.init (withHardware)) {
		Logger::L().Log (MODULENAME, severity_level::normal, "Scout system initialized...");
	} else {
		Logger::L().Log (MODULENAME, severity_level::error, "Scout system failed to initialize...");
	}

	//scout.loginUser ("factory_admin", "Vi-CELL#007");
	scout.loginUser("bci_service", "916348");

	Logger::L().Flush();

	io_svc_.run();


	return 0;
}

//*****************************************************************************
bool Scout::buildCarouselWq (WorkQueue*& wq) 
{
	// The cell types for this user...
	uint32_t* user_celltype_indices;
	NativeDataType tag;
	uint32_t nCells = 0;
	HawkeyeError he = GetMyCellTypeIndices (nCells, tag, user_celltype_indices);
	if (he != HawkeyeError::eSuccess) {
		cout << "Failed to get user's cell types: " << GetErrorAsStr(he) << endl;
		return false;
	}

	cout << "User \"" << currentUser_ << "\" has " << nCells << " cell types." << endl;
	for (uint32_t i = 0; i < nCells; i++) {
		cout << "[" << i << "]: " << user_celltype_indices[i] << endl;
	}

	FreeTaggedBuffer(tag, user_celltype_indices);

	size_t len;
	std::string str;
	wq = new WorkQueue;
	wq->numWQI = 3;
	str = "ScoutTest_WQ";
	len = str.size() + 1;
	wq->label = new char[len];
	strcpy_s (wq->label, len, str.c_str());
	wq->workQueueItems = new WorkQueueItem[wq->numWQI];
	wq->carrier = eCarousel;
	wq->precession = eColumnMajor;
	memset (&wq->additionalWorkSettings, 0, sizeof(WorkQueueItem));


	//Additional Settings
	str = "AdditionalSample";
	len = str.size() + 1;
	wq->additionalWorkSettings.comment = new char[len];
	strcpy_s (wq->additionalWorkSettings.comment, len, str.c_str());
	
	wq->additionalWorkSettings.celltypeIndex = 0x00000001;
	wq->additionalWorkSettings.dilutionFactor = 1;
	wq->additionalWorkSettings.postWash = eNormalWash;
	wq->additionalWorkSettings.bp_qc_name = nullptr;
	wq->additionalWorkSettings.numAnalyses = 1;
	wq->additionalWorkSettings.analysisIndex = 0x0000;
	wq->additionalWorkSettings.status = eNotProcessed;
	wq->additionalWorkSettings.saveEveryNthImage = 5;

	//************************************************
	// Item #1
	//************************************************
	WorkQueueItem* wqi = &wq->workQueueItems[0];
	str = "Hole Z-1";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 1";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 1);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000003;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 99;

	//************************************************
	// Item #2
	//************************************************
	wqi = &wq->workQueueItems[1];
	str = "Hole Z-2";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 2";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 2);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000002;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 99;

	//************************************************
	// Item #3
	//************************************************
	wqi = &wq->workQueueItems[2];
	str = "Hole Z-3";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 3";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 3);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000001;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 99;
	
	return true;

	//************************************************
	// Item #4
	//************************************************
	wqi = &wq->workQueueItems[3];
	str = "Hole 4";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 4";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 4);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 10;

	//************************************************
	// Item #5
	//************************************************
	wqi = &wq->workQueueItems[4];
	str = "Hole 5";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 5";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 5);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 4;

	//************************************************
	// Item #6
	//************************************************
	wqi = &wq->workQueueItems[5];
	str = "Hole 6";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 6";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 6);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 20;

	//************************************************
	// Item #7
	//************************************************
	wqi = &wq->workQueueItems[6];
	str = "Hole 7";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 7";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 7);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 15;

	//************************************************
	// Item #8
	//************************************************
	wqi = &wq->workQueueItems[7];
	str = "Hole 8";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 8";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 8);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 8;

	//************************************************
	// Item #9
	//************************************************
	wqi = &wq->workQueueItems[8];
	str = "Hole 9";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 9";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 9);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 7;

	//************************************************
	// Item #10
	//************************************************
	wqi = &wq->workQueueItems[9];
	str = "Hole 10";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s (wqi->label, len, str.c_str());

	str = "Comment 10";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s (wqi->comment, len, str.c_str());

	wqi->location.setRowColumn ('Z', 10);
	if (!wqi->location.isValid()) {
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 0;

	return true;
}

//*****************************************************************************
bool Scout::buildPlateWq(WorkQueue*& wq)
{
	// The cell types for this user...
	uint32_t* user_celltype_indices;
	NativeDataType tag;
	uint32_t nCells = 0;
	HawkeyeError he = GetMyCellTypeIndices(nCells, tag, user_celltype_indices);
	if (he != HawkeyeError::eSuccess)
	{
		cout << "Failed to get user's cell types: " << GetErrorAsStr(he) << endl;
		return false;
	}

	cout << "User \"" << currentUser_ << "\" has " << nCells << " cell types." << endl;
	for (uint32_t i = 0; i < nCells; i++)
	{
		cout << "[" << i << "]: " << user_celltype_indices[i] << endl;
	}

	FreeTaggedBuffer(tag, user_celltype_indices);

	size_t len;
	std::string str;
	wq = new WorkQueue;
	wq->numWQI = 1;
	str = "ScoutTest_WQ";
	len = str.size() + 1;
	wq->label = new char[len];
	strcpy_s(wq->label, len, str.c_str());
	wq->workQueueItems = new WorkQueueItem[wq->numWQI];
	wq->carrier = ePlate_96;
	wq->precession = eRowMajor;
	memset(&wq->additionalWorkSettings, 0, sizeof(WorkQueueItem));


	//************************************************
	// Item #1
	//************************************************
	WorkQueueItem* wqi = &wq->workQueueItems[0];
	str = "Sample C-1";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 1";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('C', 1);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000003;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 10;

	return true;


	//************************************************
	// Item #2
	//************************************************
	wqi = &wq->workQueueItems[1];
	str = "Hole Z-3";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 3";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	//wqi->location.setRowColumn ('Z', 3);
	wqi->location.setRowColumn('B', 3);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000001;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 2;


	//************************************************
	// Item #3
	//************************************************
	wqi = &wq->workQueueItems[2];
	str = "Hole Z-2";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 2";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	//wqi->location.setRowColumn ('Z', 2);
	wqi->location.setRowColumn('B', 2);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000002;
	wqi->dilutionFactor = 1;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 3;

	return true;

	//************************************************
	// Item #4
	//************************************************
	wqi = &wq->workQueueItems[3];
	str = "Hole 4";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 4";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 4);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #5
	//************************************************
	wqi = &wq->workQueueItems[4];
	str = "Hole 5";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 5";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 5);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #6
	//************************************************
	wqi = &wq->workQueueItems[5];
	str = "Hole 6";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 6";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 6);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #7
	//************************************************
	wqi = &wq->workQueueItems[6];
	str = "Hole 7";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 7";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 7);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #8
	//************************************************
	wqi = &wq->workQueueItems[7];
	str = "Hole 8";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 8";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 8);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #9
	//************************************************
	wqi = &wq->workQueueItems[8];
	str = "Hole 9";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 9";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 9);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	//************************************************
	// Item #10
	//************************************************
	wqi = &wq->workQueueItems[9];
	str = "Hole 10";
	len = str.size() + 1;
	wqi->label = new char[len];
	strcpy_s(wqi->label, len, str.c_str());

	str = "Comment 10";
	len = str.size() + 1;
	wqi->comment = new char[len];
	strcpy_s(wqi->comment, len, str.c_str());

	wqi->location.setRowColumn('Z', 10);
	if (!wqi->location.isValid())
	{
		cout << "*** Error: cell location is not valid ***" << endl;
		return false;
	}

	wqi->celltypeIndex = 0x00000000;
	wqi->dilutionFactor = 5;
	wqi->postWash = eNormalWash;
	wqi->bp_qc_name = nullptr;
	wqi->numAnalyses = 1;
	wqi->analysisIndex = 0x0000;
	wqi->status = eNotProcessed;
	wqi->saveEveryNthImage = 5;

	return true;
}

//*****************************************************************************
static void deepCopyAnalysisDefinition (AnalysisDefinition& to, const AnalysisDefinition& from) {

	to.analysis_index = from.analysis_index;

	size_t len = strlen (from.label) + 1;
	to.label = new char[len];
	strncpy_s (to.label, len, from.label, strlen(from.label));


	to.num_reagents = from.num_reagents;
	if (to.num_reagents > 0) {
		to.reagent_indices = new uint32_t[to.num_reagents];
		//	memcpy (to.reagent_indices, sizeof(uint32_t)*to.num_reagents, (size_t)from.reagent_indices, sizeof(uint32_t)*to.num_reagents);
		for (uint8_t i = 0; i < to.num_reagents; i++) {
			to.reagent_indices[i] = from.reagent_indices[i];
		}
	} else {
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
	} else {
		to.fl_illuminators = nullptr;
	}

	to.num_analysis_parameters = from.num_analysis_parameters;
	if (to.num_analysis_parameters > 0) {
		to.analysis_parameters = new AnalysisParameter[to.num_analysis_parameters];
		for (uint8_t i = 0; i < to.num_analysis_parameters; i++) {
			size_t len = strlen (from.analysis_parameters[i].label) + 1;
			to.analysis_parameters[i].label = new char[len];
			strncpy_s (to.analysis_parameters[i].label, len, from.analysis_parameters[i].label, strlen(from.analysis_parameters[i].label));
			to.analysis_parameters[i].characteristic.key = from.analysis_parameters[i].characteristic.key;
			to.analysis_parameters[i].characteristic.s_key = from.analysis_parameters[i].characteristic.s_key;
			to.analysis_parameters[i].characteristic.s_s_key = from.analysis_parameters[i].characteristic.s_s_key;
			to.analysis_parameters[i].threshold_value = from.analysis_parameters[i].threshold_value;
			to.analysis_parameters[i].above_threshold = from.analysis_parameters[i].above_threshold;
		}
	} else {
		to.analysis_parameters = nullptr;
	}

	if (from.population_parameter) {
		to.population_parameter = new AnalysisParameter;
		size_t len = strlen(from.population_parameter->label) + 1;
		to.population_parameter->label = new char[len];
		strncpy_s (to.population_parameter->label, len, from.population_parameter->label, len - 1);
		to.population_parameter->characteristic = from.population_parameter->characteristic;
		to.population_parameter->threshold_value = from.population_parameter->threshold_value;
		to.population_parameter->above_threshold = from.population_parameter->above_threshold;
	} else {
		to.population_parameter = nullptr;
	}
}

//*****************************************************************************
bool Scout::buildNewCellType (CellType*& ct) {

	ct = new CellType;

	ct->celltype_index = 0;
	std::string str = "New CellType";
	ct->label = new char[str.length() + 1];
	strcpy_s (ct->label, (str.length() + 1), str.c_str());
	ct->max_image_count = 50;
	ct->minimum_diameter_um = 3;
	ct->maximum_diameter_um = 5;
	ct->minimum_circularity = 4;
	ct->sharpness_limit = 8;
	ct->num_cell_identification_parameters = 1;

	ct->cell_identification_parameters = new AnalysisParameter[ct->num_cell_identification_parameters];
	AnalysisParameter* p_ap = ct->cell_identification_parameters;
	std::string temp = "Cell ID Param #1";
	p_ap->label = new char[temp.length() + 1];
	strcpy_s (p_ap->label, temp.length() + 1, "Cell ID Param #1");
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
bool Scout::buildModifiedCellType (CellType*& ct) {

	ct = new CellType;

	ct->celltype_index = 0x80000001;
	std::string str = "Modified CellType";
	ct->label = new char[str.length() + 1];
	strcpy_s (ct->label, (str.length() + 1), str.c_str());
	ct->max_image_count = 50;
	ct->minimum_diameter_um = 3;
	ct->maximum_diameter_um = 5;
	ct->minimum_circularity = 4;
	ct->sharpness_limit = 8;
	ct->num_cell_identification_parameters = 1;

	ct->cell_identification_parameters = new AnalysisParameter[ct->num_cell_identification_parameters];
	AnalysisParameter* p_ap = ct->cell_identification_parameters;
	strcpy_s (p_ap->label, sizeof(p_ap->label) + 1, "Cell ID Param #1");
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
bool Scout::buildAnalysisDefinition (AnalysisDefinition*& ad) {

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
	strcpy_s (ad->label, (str.length() + 1), str.c_str());
	str = "New Result Unit";
	strcpy_s (ad->label, (str.length() + 1), str.c_str());

	return true;
}
