
// RFIDProgrammingStationDlg.cpp : implementation file
//
#include <iterator>
#include "stdafx.h"
#include "RFIDProgrammingStation.h"
#include "RFIDProgrammingStationDlg.h"
#include "afxdialogex.h"
#include "ControllerBoardInterface.hpp"
#include "ControllerBoardCommand.hpp"
#include "Logger.hpp"
#include "LoggerSignature.hpp"
#include <playsoundapi.h>
#include "FirmwareUpdateProgressDlg.h"
#include "AppConfig.hpp"
#include <boost/exception_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include "FirmwareDownload.hpp"


static const char MODULENAME[] = "RFIDProgrammingStationDlg";

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About
class HawkeyeServicesImpl : public HawkeyeServices
{
public:
	HawkeyeServicesImpl(std::shared_ptr<boost::asio::io_context> pMainIos)
		: HawkeyeServices(pMainIos)
	{
	}
};
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CRFIDProgrammingStationDlg, CDialogEx)
	//{{AFX_MSG_MAP(CRFIDProgrammingStationDlg)
	ON_MESSAGE(UWM_UPDATEMODEL_DATA, &CRFIDProgrammingStationDlg::OnModelDataUpdate)
	ON_MESSAGE(UWM_UPDATE_LABEL_DATA, &CRFIDProgrammingStationDlg::OnUpdate_Label_Data)
	ON_MESSAGE(UWM_DISPLAY_WRITEERR_DLG, &CRFIDProgrammingStationDlg::OnDisplayWriteErrDlg)
	ON_MESSAGE(UWM_DISTROY_WRITEERR_DLG, &CRFIDProgrammingStationDlg::OnStopScanning)
	ON_MESSAGE(UWM_DISPLAY_VERIFYERR_DLG, &CRFIDProgrammingStationDlg::OnDisplayVerifyErrDlg)
	ON_MESSAGE(UWM_DISTROY_VERIFYERR_DLG, &CRFIDProgrammingStationDlg::OnStopScanning)
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_COMBO_PART_DESCRIPTION, &CRFIDProgrammingStationDlg::OnCbnSelchangeComboPartDescription)
	ON_BN_CLICKED(IDC_BTN_CLEARSELECTION, &CRFIDProgrammingStationDlg::OnBnClickedBtnClearselection)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIME_EXPIRATION, &CRFIDProgrammingStationDlg::OnDtnDatetimechangeDatetimeExpiration)
	ON_EN_CHANGE(IDC_EDIT_LOTNUMBER, &CRFIDProgrammingStationDlg::OnEnChangeEditLotnumber)
	ON_EN_SETFOCUS(IDC_EDIT_LOTNUMBER, &CRFIDProgrammingStationDlg::OnEnSetfocusEditLotnumber)
	ON_BN_CLICKED(IDC_BTN_EXPORT_LOG, &CRFIDProgrammingStationDlg::OnBnClickedBtnExportLog)
	ON_BN_CLICKED(IDC_BTN_APP_MODE, &CRFIDProgrammingStationDlg::OnBnClickedBtnAppMode)
	ON_BN_CLICKED(IDC_BTN_IMPORT_PAYLOAD_FILES, &CRFIDProgrammingStationDlg::OnBnClickedBtnImportPayloadFiles)
END_MESSAGE_MAP()

// CRFIDProgrammingStationDlg dialog
CRFIDProgrammingStationDlg::CRFIDProgrammingStationDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_RFIDPROGRAMMINGSTATION_DIALOG, pParent)
	, CB_PartDescription_Value(_T(""))
	, DT_Expiration_Value(COleDateTime::GetCurrentTime())
	, EDIT_LotNumber_Value(_T(""))
	, SLBL_Hardware_Value(_T("Offline"))
	, SLBL_Conveyor_Value(_T("Offline"))
	, SLBL_RFIDReadWrite_Value(_T("Offline"))
	, SLBL_Payload_Value(_T("Invalid"))
	, SLBL_System_Value(_T("Invalid"))
	, SLBL_RFID_Overall_Value(_T("System Invalid"))
	, SLBL_RFID_Numb1_Value(_T("Offline"))
	, SLBL_RFID_Numb2_Value(_T("Offline"))
	, SLBL_RFID_Numb3_Value(_T("Offline"))
	, SLBL_RFID_Numb4_Value(_T("Offline"))
	, SLBL_RFID_Numb5_Value(_T("Offline"))
	, SLBL_RFID_Numb6_Value(_T("Offline"))
	, _button_lotnumber_clicked(false)
	, _state_status_payload(STATE_PAYLOAD_NULL)
	, _state_status_controller(STATE_CONTROLLER_OFFLINE)
	, _state_status_hw_rfid(STATE_HW_RFID_OFFLINE)
	, _state_status_system(STATE_SYSTEM_INVALID)
	, _state_status_rf_bank(STATE_RF_BANK_INVALID)
	, _application_mode(APP_MODE_PROGRAMMING)
	, _session_tags_scanned(0)
	, _session_tags_successful(0)
	, _session_tags_failed(0)
	, EBWS_ExportLogPath_Value(_T(""))
	, _clear_payload(false)
	, _update_required(false)
	, _write_error_at(0)
	, _verify_error_at(0)
	, SLBL_TotalTags_Value(_T(""))
	, EBWS_ImportPayloadPath_Value(_T(""))
	, Checkbox_PayloadReplaceOption(false)
{
	boost::system::error_code ec;
	Logger::L().Initialize(ec, "RFIDProgrammingStation.info");
	Logger::L().Log (MODULENAME, severity_level::normal, "Initializing Programming Station");
#ifdef _DEBUG
	Logger::L().SetLoggingSensitivity(severity_level::debug1);
#endif

	WorkLogger::L().Initialize(ec, "RFIDProgrammingStation.info", "signed_worklogger");
	get_config ("RFIDProgrammingStation.info", _config);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	_hw_rf_error.value = 0;

	auto payload_path = boost::filesystem::current_path();
	payload_path.append("ReagentPayloads");
	firmware_bin_entries = LoadFirmwarePayloads_f(payload_path.string());
	
    _io_service.reset( new boost::asio::io_context() );
    _plocal_work.reset(new boost::asio::io_context::work(*_io_service));
	_hs = std::make_unique<HawkeyeServicesImpl>(_io_service);

	// Ugliness necessary because std::bind does not appreciate overloaded function names
	auto F = std::bind(static_cast<std::size_t(boost::asio::io_context::*)(void)>(&boost::asio::io_context::run), _io_service.get());
	_pio_thread.reset(new std::thread(F));

	_brush_status_hardware.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_readwrite.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_payload.CreateSolidBrush(RGB_COLOR_RED);
	_brush_status_system.CreateSolidBrush(RGB_COLOR_RED);
	_brush_status_RFID_overall.CreateSolidBrush(RGB_COLOR_RED);
	_brush_status_RFID_numb1.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_numb2.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_numb3.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_numb4.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_numb5.CreateSolidBrush(RGB_COLOR_GRY);
	_brush_status_RFID_numb6.CreateSolidBrush(RGB_COLOR_GRY);

	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb1_Value, &_brush_status_RFID_numb1));
	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb2_Value, &_brush_status_RFID_numb2));
	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb3_Value, &_brush_status_RFID_numb3));
	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb4_Value, &_brush_status_RFID_numb4));
	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb5_Value, &_brush_status_RFID_numb5));
	_state_strings_color_rf_tags.push_back(std::make_pair(&SLBL_RFID_Numb6_Value, &_brush_status_RFID_numb6));

	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel1_Control, &SLBL_RFID_Numb1_Control));
	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel2_Control, &SLBL_RFID_Numb2_Control));
	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel3_Control, &SLBL_RFID_Numb3_Control));
	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel4_Control, &SLBL_RFID_Numb4_Control));
	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel5_Control, &SLBL_RFID_Numb5_Control));
	_state_static_control.push_back(std::make_pair(&SLBL_PackLabel6_Control, &SLBL_RFID_Numb6_Control));

	for( auto &tag : _state_status_rf_tags)
	{
		tag = STATE_RF_TAG_OFFLINE;
	}

	_gpio_wrapper = std::make_unique<Gpio::GpioWrapper>();
	_gpio_wrapper->Add_ConnectionStateChanged_Handler(boost::bind(&CRFIDProgrammingStationDlg::on_conveyor_connection_change, this, _1));
	_gpio_wrapper->DisableWriting();
}

CRFIDProgrammingStationDlg::~CRFIDProgrammingStationDlg()
{
	kill_system();
}

void CRFIDProgrammingStationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PART_DESCRIPTION, CB_PartDescription_Control);
	DDX_CBString(pDX, IDC_COMBO_PART_DESCRIPTION, CB_PartDescription_Value);
	DDX_Control(pDX, IDC_DATETIME_EXPIRATION, DT_Expiration_Control);
	DDX_DateTimeCtrl(pDX, IDC_DATETIME_EXPIRATION, DT_Expiration_Value);
	DDX_Control(pDX, IDC_EDIT_LOTNUMBER, EDIT_LotNumber_Control);
	DDX_Text(pDX, IDC_EDIT_LOTNUMBER, EDIT_LotNumber_Value);
	DDX_Control(pDX, IDC_BTN_CLEARSELECTION, BTN_ClearSelection_Control);
	DDX_Control(pDX, IDC_LABLE_STATUS_HARDWARE, SLBL_Hardware_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_HARDWARE, SLBL_Hardware_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_CONVEYOR, SLBL_Conveyor_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_CONVEYOR, SLBL_Conveyor_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_RWHW, SLBL_RFIDReadWrite_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_RWHW, SLBL_RFIDReadWrite_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_PAYLOAD, SLBL_Payload_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_PAYLOAD, SLBL_Payload_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_SYSTEM, SLBL_System_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_SYSTEM, SLBL_System_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_OVERALL, SLBL_RFID_Overall_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_OVERALL, SLBL_RFID_Overall_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM1, SLBL_RFID_Numb1_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM1, SLBL_RFID_Numb1_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM2, SLBL_RFID_Numb2_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM2, SLBL_RFID_Numb2_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM3, SLBL_RFID_Numb3_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM3, SLBL_RFID_Numb3_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM4, SLBL_RFID_Numb4_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM4, SLBL_RFID_Numb4_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM5, SLBL_RFID_Numb5_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM5, SLBL_RFID_Numb5_Value);
	DDX_Control(pDX, IDC_LABLE_STATUS_RFID_NUM6, SLBL_RFID_Numb6_Control);
	DDX_Text(pDX, IDC_LABLE_STATUS_RFID_NUM6, SLBL_RFID_Numb6_Value);
	DDX_Control(pDX, BROWSE_EXPORT_LOG_PATH, EBWS_ExportLogPath_Control);
	DDX_Text(pDX, BROWSE_EXPORT_LOG_PATH, EBWS_ExportLogPath_Value);
	DDX_Control(pDX, IDC_STATIC_PACK1, SLBL_PackLabel1_Control);
	DDX_Control(pDX, IDC_STATIC_PACK2, SLBL_PackLabel2_Control);
	DDX_Control(pDX, IDC_STATIC_PACK3, SLBL_PackLabel3_Control);
	DDX_Control(pDX, IDC_STATIC_PACK4, SLBL_PackLabel4_Control);
	DDX_Control(pDX, IDC_STATIC_PACK5, SLBL_PackLabel5_Control);
	DDX_Control(pDX, IDC_STATIC_PACK6, SLBL_PackLabel6_Control);
	DDX_Text(pDX, IDC_LABEL_TOTAL_TAGS, SLBL_TotalTags_Value);
	DDX_Control(pDX, IDC_BTN_APP_MODE, BTN_ApplicationMode_Control);
	DDX_Control(pDX, BROWSE_IMPORT_PAYLOAD_PATH, EBWS_ImportPayloadPath_Control);
	DDX_Text(pDX, BROWSE_IMPORT_PAYLOAD_PATH, EBWS_ImportPayloadPath_Value);
	//DDX_Check(pDX, IDC_CHECKPAYLOADREPLACE, Checkbox_PayloadReplaceOption);
	DDX_Control(pDX, IDC_CHECKPAYLOADREPLACE, CHECK_ReplacePayload);
}

BOOL CRFIDProgrammingStationDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			::PostMessage(this->m_hWnd, UWM_UPDATE_LABEL_DATA, 0, pMsg->wParam);
			return TRUE;                // Do not process further
		}
		if (pMsg->wParam >= 0x30 &&
			pMsg->wParam <= 0x39)
		{
			::PostMessage(this->m_hWnd, UWM_UPDATE_LABEL_DATA, pMsg->wParam, 0);
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CRFIDProgrammingStationDlg::set_app_config (const char* info_file, const char* field, const char* value, const char* tree, const char *parent_tree) const
{
	AppConfig appCfg;
	t_opPTree pConfig = appCfg.init(info_file);
	
	t_opPTree pTree;
	if (parent_tree != nullptr)
	{
		t_opPTree pParentTree = appCfg.findTagInSection (pConfig, parent_tree);
		pTree = appCfg.findTagInSection (pParentTree, tree);
	}
	else
		pTree = appCfg.findTagInSection (pConfig, tree);

	appCfg.setField (pTree, field, value);
	appCfg.write();
}

bool CRFIDProgrammingStationDlg::get_config (std::string info_file, StationAppConfig_t & config) const
{
	AppConfig appCfg;

	t_opPTree pRoot = appCfg.init(info_file);
	if (!pRoot) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Failed to open \"" + info_file + "\"");
		return false;
	}

	t_opPTree pPayloadLookupTable = appCfg.findSection (std::string("config.app"));
	if (!pPayloadLookupTable) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Info file section \"config.app\" not found.");
		return false;
	}


//TODO: is this still needed???
	//appCfg.findField<bool>(pStationAppTree, "user_confirmation", false, false, config.user_confirmation);

	appCfg.findField<std::string> (pPayloadLookupTable, "payload_lookuptable_file", false, "HawkeyePlatformReagentPayload.txt", config.payload_lookupfile);


	// The ability to field update of controller firmware has been disabled in initial release.

	if (!HawkeyeConfig::load ("HawkeyeDLL.info")) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Failed to open \"HawkeyeDLL.info\"");
		return false;
	}

	config.hawkeyeConfig = HawkeyeConfig::Instance().get();

	return true;
}

bool CRFIDProgrammingStationDlg::rf_reader_error()
{
	uint32_t rf_reader_error = 0;
	bool error_found=false;
	_hw_rf_error.value = 0;

	get_reagent_cmd(ReagentRFErrorCode, rf_reader_error, false);
	if ((rf_reader_error & 0xFF000) == 0x26000)//looking for RF HW errors
	{
		_hw_rf_error.value = 0x00FFF & rf_reader_error;
		error_found = true;
	}
	else if ((rf_reader_error & 0xFF000) == 0x41000 || //RF Reader, waiting for ISTAT - problems with the RF reader
			 (rf_reader_error & 0xFF000) == 0x10000)//SM_Busy - waiting to get response back from RF reader
	{
		error_found = true;
	}

	return error_found;
}

bool CRFIDProgrammingStationDlg::controller_cmd_error()
{
	return _pcontroller_board_command->readErrorStatus().isSet(ErrorStatus::Reagent);
}

UINT CRFIDProgrammingStationDlg::update_all_statemachines(LPVOID p)
{
	auto param = static_cast<THREADSTRUCT*>(p);
	auto dlg = param->dlg_instance;
	try
	{
		while (!dlg->_stop_thread)
		{
			::Sleep(WORKER_THREAD_SLEEP);
			dlg->statemachine_controller();
			dlg->statemachine_rfid_reader();
			dlg->statemachine_payload();
			dlg->statemachine_system();
			dlg->statemachine_rf_bank();

			if (dlg->_clear_payload)
			{
				auto btn = dlg->GetDlgItem(IDC_BTN_CLEARSELECTION);
				btn->GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_CLEARSELECTION, BN_CLICKED), reinterpret_cast<LPARAM>(btn->m_hWnd));
				dlg->_clear_payload = false;
			}
			else if (dlg->_update_required)
			{
				dlg->_update_required = false;
				::PostMessage(dlg->m_hWnd, UWM_UPDATEMODEL_DATA, FALSE, 0);
				::PostMessage(dlg->m_hWnd, UWM_UPDATEMODEL_DATA, TRUE, 0);
			}
		}
	}
	catch(...)
	{
		AfxMessageBox(_T("Critical Failure Occurred, Exiting"), IDOK);
		dlg->SendMessage(WM_QUIT);
	}
	dlg->_thread_stop_cond.notify_all();

	AfxEndThread(1);
	return 1;
}

void CRFIDProgrammingStationDlg::kill_system()
{
	if (_session_tags_scanned > 0)
	{
		std::time_t started_time = std::chrono::system_clock::to_time_t(_time_started_scanning);
		char s[50];
		ctime_s(s, 50, &started_time);
		std::string ctimestr(s);
		ctimestr.erase(std::remove(ctimestr.begin(), ctimestr.end(), '\n'), ctimestr.end());
		
		WorkLogger::L().Log("Programmer", severity_level::notification, boost::str(boost::format(
		            "Record of session\nSession Start: %s\nGTIN: %s\nLot: %s\nExpiration: %s\nProgrammed/Verified Tags: %d\nFailed Tags: %d") 
		             % ctimestr 
		             % _GTIN 
	                 % _LOT_number 
		             % _EXP_date 
		             % _session_tags_successful 
		             % _session_tags_failed));
		WorkLogger::L().Flush();
		WorkLogger::L().CloseLogFile();
	}
	_session_tags_scanned = _session_tags_successful = _session_tags_failed = 0;
	Logger::L().Log (MODULENAME, severity_level::normal, "System Shutting Down");
	Logger::L().Flush();

	_stop_thread = true;
	//Block until the "state machine" thread completes
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> lock(mutex);
	_thread_stop_cond.wait(lock);

	_gpio_wrapper.reset();
	_pcontroller_board_command.reset();
	_pcontroller_board.reset();
	_pfirmwaredownload.reset();
	_plocal_work.reset();
}

void CRFIDProgrammingStationDlg::statemachine_controller()
{
	static int32_t timeout_expired=0;

	auto prev_state = _state_status_controller;
	switch (_state_status_controller)
	{
		case STATE_CONTROLLER_NULL: 
			break;
		case STATE_CONTROLLER_OFFLINE:
		{
			if (InitializeController())
			{
				//The ability to field update of controller firmware has been disabled in initial release.
				//if (_config.enableK70Update == true &&
				//	_config.new_fw_version != "unknown" &&  
				//	_config.new_fw_version != "" && 
				//	_pcontroller_board->getVersion() != _config.new_fw_version)
				//{
				//	_pcontroller_board_command.reset();//I do not want any polling to happen
				//	_state_status_controller = STATE_CONTROLLER_FW_INIT;
				//}
				//else
				_state_status_controller = STATE_CONTROLLER_VALID;
				//_pcontroller_board->StopLocalIOService();
			}
			else
			{
				_state_status_controller = STATE_CONTROLLER_INIT_FAILED;
				timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(500);
			}
			break;
		}
		case STATE_CONTROLLER_INIT_FAILED:
		{
			if (--timeout_expired <= 0)
			{
				_state_status_controller = STATE_CONTROLLER_OFFLINE;
			}
			break;
		}
		case STATE_CONTROLLER_ERROR:
			break;
		case STATE_CONTROLLER_FW_INIT:
		{
//NOTE: updating firmware is no longer supported.
			//boost::system::error_code ec;
			//_pfwupdate_progress_dialog.reset (new FirmwareUpdateProgressDlg(AfxGetMainWnd()));
			//_pfirmwaredownload.reset (new FirmwareDownload(_io_service, _pcontroller_board));
			//_pfirmwaredownload->setFirmwareProgressCallback (&firmware_update_progress);
			//_pfirmwaredownload->setHashKey (_config.hawkeyeConfig->controllerBoard.firmware.hashkey);
			//
			//// This API is non-blocking.
			//// The *FIRMDOWN_END* state in FirmwareDownload.cpp updates 
			//_pfirmwaredownload->StartK70FirmwareUpdate (_config.hawkeyeConfig->controllerBoard.firmware.path, ec);
			//if (ec == boost::system::errc::success) {
			//	_state_status_controller = STATE_CONTROLLER_FW_UPDATE;
			//} else {
			//	_state_status_controller = STATE_CONTROLLER_FW_UPDATE_FAILED;
			//}
			break;
		}
		case STATE_CONTROLLER_FW_UPDATE:
		{
			if (_fw_update_finished)
			{
				_state_status_controller = STATE_CONTROLLER_FW_UPDATE_SUCCESS;
				_pfwupdate_progress_dialog->SendMessage(WM_CLOSE);
				_pfwupdate_progress_dialog.reset();
			}
			else
			{
				_pfwupdate_progress_dialog->SendMessage(UWM_UPDATE_FW_PROGRESS, _fw_update_progress, 0);
			}
			break;
		}
		case STATE_CONTROLLER_FW_UPDATE_FAILED:
//This was a really terrible way to do the reset in "STATE_CONTROLLER_FW_UPDATE_SUCCESS without a state change to "update success".
// We'll reorder the state machine so we can use the fallthrough path instead
//			goto reset_control;
		/*  INTENTIONAL FALLTHROUGH TO _UPDATE_SUCCESS */
		case STATE_CONTROLLER_FW_UPDATE_SUCCESS:
		{
//reset_control:
			_pfirmwaredownload.reset();
			_pcontroller_board.reset();
			if (InitializeController())
				_state_status_controller = STATE_CONTROLLER_VALID;
			else
			{
				timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(500);
				_state_status_controller = STATE_CONTROLLER_INIT_FAILED;
			}
			break;
		}
		
		case STATE_CONTROLLER_VALID:
		{
			//Check the state of the commutation to the controller when the system is not actively
			//scanning for tags. This will reduce the load to the controller when writing to the RF tag. 
			if(_state_status_system == STATE_SYSTEM_INVALID)
			{
				if (_pcontroller_board->GetBoardStatus().isSet(BoardStatus::FwUpdateBusy))//board status is 0xFFFF when connection to the controller drops
				{
					_state_status_controller = STATE_CONTROLLER_CHANGED_INVALID;
				}
			}
			break;
		}
		
		
		case STATE_CONTROLLER_CHANGED_INVALID:
		{
			if (--timeout_expired < 0)
			{
				timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(1000);
			}
			else if(timeout_expired == 0)
			{
				_state_status_controller = STATE_CONTROLLER_OFFLINE;
			}
			break;
		}
		default:
			break;
	}

	change_status_label<state_controller_status>(_state_status_controller, prev_state,
                                                 SLBL_Hardware_Value, _brush_status_hardware, 
                                                 enum_map_status_contorller, _update_required);
}

void CRFIDProgrammingStationDlg::statemachine_rfid_reader()
{
	static int32_t timeout_expired = 0;
	auto prev_state = _state_status_hw_rfid;
	switch (_state_status_hw_rfid)
	{
		case STATE_HW_RFID_NULL: 
			break;
		case STATE_HW_RFID_OFFLINE:
		{
			if (--timeout_expired <= 0 && _state_status_controller == STATE_CONTROLLER_VALID)
			{
				send_reagent_cmd(ReagentCommandRFScanTagMode, 0);//0-Signal Scan
				if (rf_reader_error() && _hw_rf_error.value == 0)//if there is a RF error, the reader did respond)
				{
					_state_status_hw_rfid = STATE_HW_RFID_ERROR;
					timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(1);
				}
				else
				{
					_state_status_hw_rfid = STATE_HW_RFID_CONNECTED;

					//to keep the controller sm active in checking for tags
					//this way we can keep communication open to the RF Reader
					send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false); //1-AutoScan
				}
				_pcontroller_board_command->clearErrorCode(ReagentErrorCode);
			}
			break;
		}
		case STATE_HW_RFID_ERROR:
		{
			static uint32_t istat_count = 0;
			if (--timeout_expired <= 0)
			{	
				send_reagent_cmd(ReagentCommandRFScanTagMode, 0);//0-Signal Scan
				if (rf_reader_error() && _hw_rf_error.value == 0)//if there is a RF error, the reader did respond
				{
					_state_status_hw_rfid = STATE_HW_RFID_ERROR;
					if (!controller_cmd_error()) send_reagent_cmd(ReagentCommandRFReaderReset, 0);
					set_reagent_cmd(ReagentRFErrorCode, 0);
					timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(100);//keep the displaying RFID error
				}
				else
				{
					_state_status_hw_rfid = STATE_HW_RFID_OFFLINE;
					timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(100);
				}
			}
			break;
		}
		case STATE_HW_RFID_CONNECTED:
		{
			if (_state_status_controller != STATE_CONTROLLER_VALID)
			{
				_state_status_hw_rfid = STATE_HW_RFID_OFFLINE;
				break;
			}

			//Check the state of the commutation to the controller when the system is not actively
			//scanning for tags. This will reduce the load to the controller when writing to the RF tag. 
			if (_state_status_system == STATE_SYSTEM_INVALID)
			{
				send_reagent_cmd(ReagentCommandRFScanTagMode, 0);//0-Signal Scan
				if (rf_reader_error() && _hw_rf_error.value == 0)//if there is a RF error, the reader did respond
				{
					_state_status_hw_rfid = STATE_HW_RFID_ERROR;
					timeout_expired = 0;
				}
			}
			break;
		}
		default: 
			break;
	}
	change_status_label<state_hw_rfid_status>(_state_status_hw_rfid, prev_state,
                                                 SLBL_RFIDReadWrite_Value, _brush_status_RFID_readwrite,
                                                 enum_map_status_hw_rfid, _update_required);
}

void CRFIDProgrammingStationDlg::statemachine_payload()
{
	bool entries_valid = true;
	static int32_t timeout_expired = 0;

	auto prev_state = _state_status_payload;
	switch (_state_status_payload)
	{
		case STATE_PAYLOAD_NULL:
		{
			if (firmware_bin_entries.size() != 0)
			{
				for (auto entry : firmware_bin_entries)
				{
					if (entries_valid)//we need to capture at least one entry is false
						entries_valid = entry.isLoaded();
				}
				_state_status_payload = STATE_PAYLOAD_INVALID;
			}
			else
			{
				_state_status_payload = STATE_PAYLOAD_FILES_INVALID;
			}
			break;
		}
			
		case STATE_PAYLOAD_INVALID:
		{
			if (entries_valid)
			{
				if (!CB_PartDescription_Value.IsEmpty())
				{
					_state_status_payload = STATE_PAYLOAD_VALID;
				}
				else
				{
					_state_status_payload = STATE_PAYLOAD_DESCRIPT_NOT_SELECTED;
				}
			}
			break;
		}
		case STATE_PAYLOAD_DESCRIPT_NOT_SELECTED:
		{
			if (!CB_PartDescription_Value.IsEmpty())
			{
				_state_status_payload = STATE_PAYLOAD_LOTNUM_NOT_ENTERED;
				_button_lotnumber_clicked = false;
			}
			break;
		}
		case STATE_PAYLOAD_LOTNUM_NOT_ENTERED:
		{
			if (!EDIT_LotNumber_Value.IsEmpty())
			{
				_state_status_payload = STATE_PAYLOAD_VALID;
			}
			break;
		}
		case STATE_PAYLOAD_VALID:
		{
			if (!entries_valid ||
				_state_status_system == STATE_SYSTEM_INVALID ||
				CB_PartDescription_Value.IsEmpty())
			{
				_state_status_payload = STATE_PAYLOAD_INVALID;
			}
			break;
		}
		case STATE_PAYLOAD_BARCODE_INVALID:
		case STATE_PAYLOAD_BARCODE_INVALID_DATE:
		{
			if (--timeout_expired == 0)
			{
				_state_status_payload = STATE_PAYLOAD_DESCRIPT_NOT_SELECTED;
			}
			else if (timeout_expired < 0)
			{
				timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(100);
				PlaySound(_T("sounds\\Computer Error.wav"), NULL, SND_APPLICATION | SND_ASYNC);

				//the state changed outside from the state machine, 
				//the text and brush would be updated in when changing previous state with invalid state.
				prev_state = STATE_PAYLOAD_INVALID;
			}
			break;
		}
		case STATE_PAYLOAD_BARCODE_GTIN_NOTFOUND:
		{
			if (--timeout_expired == 0)
			{
				_state_status_payload = STATE_PAYLOAD_BARCODE_INVALID;
			}
			else if (timeout_expired < 0)
			{
				timeout_expired = MILLISECONDS_TO_TIMEOUT_COUNT(100);
				PlaySound(_T("sounds\\Computer Error.wav"), NULL, SND_APPLICATION | SND_ASYNC);

				//the state changed outside from the state machine, 
				//the text and brush would be updated in when changing previous state with invalid state.
				prev_state = STATE_PAYLOAD_INVALID;
			}

			break;
		}
		case STATE_PAYLOAD_BARCODE_VALID:
		{
			_state_status_payload = STATE_PAYLOAD_VALID;
			prev_state = STATE_PAYLOAD_BARCODE_INVALID;

			PlaySound(_T("sounds\\Front Desk Bell.wav"), NULL, SND_APPLICATION | SND_ASYNC);
			EDIT_LotNumber_Value = StdStrToCStr(_LOT_number);
			
			EDIT_LotNumber_Control.ShowWindow(TRUE);
			break;
		}
		case STATE_PAYLOAD_FILES_INVALID: 
			break;//we cannot do anything when this happened.  The user must restart the application. 
		default:
			break;;
	}

	change_status_label<state_payload_status>(_state_status_payload, prev_state,
                                              SLBL_Payload_Value, _brush_status_payload,
                                              enum_map_status_payload, _update_required);
}

void CRFIDProgrammingStationDlg::statemachine_system()
{
	auto prev_state = _state_status_system;
	switch (_state_status_system)
	{
		case STATE_SYSTEM_NULL: 
			break;
		case STATE_SYSTEM_INVALID:
		{
			if (_state_status_controller == STATE_CONTROLLER_VALID &&
				_state_status_hw_rfid == STATE_HW_RFID_CONNECTED &&
				_state_status_payload == STATE_PAYLOAD_VALID)
			{
				_state_status_system = STATE_SYSTEM_READY;
			}

			break;
		}
		case STATE_SYSTEM_READY:
		{
			if (_state_status_controller != STATE_CONTROLLER_VALID ||
				_state_status_hw_rfid != STATE_HW_RFID_CONNECTED ||
				_state_status_payload != STATE_PAYLOAD_VALID)
			{
				_state_status_system = STATE_SYSTEM_INVALID;
				change_statemachine_state(SM_RF_BANK, STATE_RF_BANK_CFGRESET_TIMEOUT);
				WorkLogger::L().Log("Programmer", severity_level::notification, "End Session");
			}
			break;
		}
		default:
			break;;
	}

	change_status_label<state_system_status>(_state_status_system, prev_state,
                                              SLBL_System_Value, _brush_status_system,
                                              enum_map_status_system, _update_required);
}

void CRFIDProgrammingStationDlg::on_conveyor_connection_change(bool isConnected)
{
	state_hw_rfid_status prevStatus = isConnected ? STATE_HW_RFID_OFFLINE : STATE_HW_RFID_CONNECTED;
	state_hw_rfid_status status = isConnected ? STATE_HW_RFID_CONNECTED : STATE_HW_RFID_OFFLINE;
	
	change_status_label<state_hw_rfid_status>(status, prevStatus,
											  SLBL_Conveyor_Value, _brush_status_conveyor,
											  enum_map_status_hw_rfid, _update_required);
}

void CRFIDProgrammingStationDlg::show_pack_label(uint32_t& current_tag, BOOL mode)
{
	_state_static_control[current_tag].first->ShowWindow(mode);
	_state_static_control[current_tag].second->ShowWindow(mode);
}

void CRFIDProgrammingStationDlg::get_payload_data(std::vector<uint32_t>& data)
{
	if (CB_PartDescription_Control.GetCurSel() < 0)
		return;

	// Strip alpha characters, assume DECIMAL input stream - TODO: Change if vendor switches to non-numeric lot information.
	auto lot_info = LotNumberToUINT32(_LOT_number);

	auto rfid_data = firmware_bin_entries[CB_PartDescription_Control.GetCurSel()].GetRFIDContents(OleDateTimeToEpochRef(DT_Expiration_Value) ,lot_info);
	std::vector<uint8_t> data8(rfid_data.begin(), rfid_data.end());
	auto pdata32 = reinterpret_cast<const uint32_t*>(data8.data());
	data.insert(data.begin(), pdata32, pdata32 + data8.size() / sizeof(uint32_t));
}

std::string tagSNToHexString(std::vector<uint32_t> sn)
{
	// Take first 5 bytes and hexify them.
	std::string temp = "";

	if (sn.size() < 2)
		return temp;

	uint8_t* b = (uint8_t*)sn.data();
	for (int i = 0; i < 5; i++)
	{
		unsigned char nibble1 = *b >> 4;
		unsigned char nibble2 = *b & 0x0f;

		if (i != 0)
			temp += ' ';

		if (nibble1 < 10)
			temp += (char)('0' + nibble1);
		else
		{
			nibble1 -= 10;
			temp += (char)('A' + nibble1);
		}

		if (nibble2 < 10)
			temp += (char)('0' + nibble2);
		else
		{
			nibble2 -= 10;
			temp += (char)('A' + nibble2);
		}

		b++;
	}

	return temp;
}

void CRFIDProgrammingStationDlg::statemachine_rf_bank()
{
	static boost::posix_time::ptime start_time = {};
	static boost::posix_time::ptime stop_time = {};

	static int32_t timeout_expired = 0;
	static uint32_t total_tags_reported = 0;	
	static uint32_t current_tag = 0;
	CString append_string("");
	uint32_t reported = 0;

	static std::vector<std::string> visible_tag_serialnumbers = {}; // Serial numbers visible in the current scan

	auto prev_state = _state_status_rf_bank;
	switch (_state_status_rf_bank)
	{
		case STATE_RF_BANK_NULL: 
			break;
		case STATE_RF_BANK_INVALID:
		{
			if (_state_status_system == STATE_SYSTEM_READY)
			{
				_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;
				_time_started_scanning = std::chrono::system_clock::now();
				if (_application_mode == APP_MODE_PROGRAMMING)
					WorkLogger::L().Log("Programmer", severity_level::notification, "Start Programming Session");
				else
					WorkLogger::L().Log("Programmer", severity_level::notification, "Start Verifying Session");

				session_tag_history.clear();
			}
			else 
			{
				_gpio_wrapper->DisableWriting();
			}
			break;
		}
		case STATE_RF_BANK_HW_ERROR:
		{
			static bool retry = false;
			if (retry)
			{
				get_reagent_cmd(ReagentRFTagsTotalNum, reported);
				if (controller_cmd_error())
				{
					_state_status_rf_bank = STATE_RF_BANK_INVALID;
					change_statemachine_state(SM_CONTROLLER, STATE_CONTROLLER_CHANGED_INVALID);
					break;
				}
				else if (rf_reader_error())
				{
					_state_status_rf_bank = STATE_RF_BANK_INVALID;
					change_statemachine_state(SM_RF_READER, STATE_HW_RFID_ERROR);
					break;
				}
				else
				{
					retry = false;
					_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;
				}
			}
			else
			{
				retry = true;
				total_tags_reported = 0;
				PlaySound(_T("sounds\\Computer Error.wav"), NULL, SND_APPLICATION | SND_ASYNC);
				std::stringstream stream;
				stream << std::hex << _hw_rf_error.error.code;
				Logger::L().Log (MODULENAME, severity_level::error, "RF error: ErrorType[" + std::to_string(_hw_rf_error.error.type) +
                                "] ErrorCode[0x" + stream.str() + "]");
				if (!controller_cmd_error()) send_reagent_cmd(ReagentCommandRFReaderReset, 0);
			}
			break;
		}
		case STATE_RF_BANK_UPDATE_TAGS:
		{
			if (_state_status_system == STATE_SYSTEM_INVALID)
			{
				_state_status_rf_bank = STATE_RF_BANK_INVALID; 
				break;
			}

			bool busy = _pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy);
			rf_reader_error();
			if (busy == true && tag_missing())
			{
				_gpio_wrapper->EnableWriting();
				break;
			}

			static uint32_t no_tag_found_count = 0;
			get_reagent_cmd(ReagentRFTagsTotalNum, reported);
			if (controller_cmd_error()) 
			{
				_state_status_rf_bank = STATE_RF_BANK_HW_ERROR;
				break;
			}
			
			if (reported)
			{
				if (reported <= TOTAL_SUPPORTED_RF_TAGS)
				{
					total_tags_reported = reported;
					_session_tags_scanned += reported;

					visible_tag_serialnumbers.clear();
					current_tag = 0;
					_state_status_rf_bank = STATE_RF_BANK_ENUMERATE_TAGS;

					no_tag_found_count = 0;	
				}
				else
				{
					AfxMessageBox(_T("Too Many Tags In Range\n RFID system reports too many tags within read range.\n Remove Tags and retry."), MB_OK | MB_ICONEXCLAMATION);
					// Reset back into the current state to re-scan. 
					}
				}
			else if (no_tag_found_count++ > TOTAL_TAGS_NOTFOUND_LIMIT)
			{
				_state_status_rf_bank = STATE_RF_BANK_CFGRESET_TIMEOUT;
				no_tag_found_count = 0;
			}
			else
				send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.. do not block

			break;
		}
		case STATE_RF_BANK_ENUMERATE_TAGS:
		{
			if (current_tag < total_tags_reported)
			{
				std::vector<uint32_t> tag_data(2); // Need to read in 5 bytes total, so (2x4 > 5);
				std::fill(tag_data.begin(), tag_data.end(), 0);

				// Offset into the tag data appropriately.
				get_reagent_data(ReagentRFTagsAppData, tag_data, (current_tag * (sizeof(RfAppdataRegisters) / sizeof(uint32_t))) );

				visible_tag_serialnumbers.push_back(tagSNToHexString(tag_data));

				// record label
				show_pack_label(current_tag, TRUE);

				// Repeat for the next tag in view
				current_tag++;
			}
			else
			{
				// Enumeration complete; start again at first tag.
				current_tag = 0;

				if (_application_mode == APP_MODE_PROGRAMMING)
				{
					_state_status_rf_bank = STATE_RF_BANK_WRITE_SENDDATA;
					change_tags_state_all(STATE_RF_TAG_BEING_PROGRAMED, total_tags_reported);
				}
				else
				{
					_state_status_rf_bank = STATE_RF_BANK_VERIFY_PAYLOAD_SCAN;
					change_tags_state_all(STATE_RF_TAG_VERIFYING, total_tags_reported);
					timeout_expired = 0;
				}
			}
			break;
		}
		case STATE_RF_BANK_WRITE_SENDDATA:
		{
			if (total_tags_reported <= 0)
			{
				_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;
				break;
			}
			std::vector<uint32_t> data;
			get_payload_data(data);


			/*
			 NB: THE SYSTEM DOES NOT PRESENTLY HANDLE MULTIPLE TAGS CORRECTLY.
			     (That is: "at all".)
				 THIS CODE WILL NOT BEHAVE FOR >1 TAG.
				 THE FIX WILL INVOLVE RECYCLING THE STATE MACHINE N TIMES THROUGH THE 
				 WRITE SEQUENCE.
			 */
			std::vector<uint32_t> data_to_send;
			for (uint32_t i = 0; i < total_tags_reported; i++)
				data_to_send.insert(data_to_send.end(), data.begin(), data.end());

			stop_time = boost::posix_time::not_a_date_time;
			start_time = boost::posix_time::microsec_clock::local_time();
			send_reagent_data(ReagentRFTagsAppData, data_to_send, 8);
			send_reagent_cmd(ReagentCommandProgTag, current_tag, false);
			_state_status_rf_bank = STATE_RF_BANK_WRITE_SENDDATA_WAIT;

			break;
		}
		case STATE_RF_BANK_WRITE_SENDDATA_WAIT:
		{
			if (_pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy))
				break;

			stop_time = boost::posix_time::microsec_clock::local_time();
			auto diff = stop_time - start_time;
			Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("tag #%d - Write Time: %d msec") % total_tags_reported % diff.total_milliseconds()));
			Logger::L().Flush();

			uint32_t error_code=0;
			get_reagent_cmd(ReagentRFErrorCode, error_code);
			if ((error_code & 0xFF000) == 0x10000)//SM_Busy - waiting to get response back from RF reader
				break;

			if (!rf_reader_error())
			{
				total_tags_reported++;
				++_session_tags_successful;

				// only want to signal write success once, so this is the place to do it.
				_gpio_wrapper->SignalWriteSuccess();
				PlaySound(_T("sounds\\Checkout Scanner Beep.wav"), NULL, SND_APPLICATION | SND_ASYNC);
				
				session_tag_history[visible_tag_serialnumbers[current_tag]].push_back(std::make_pair<bool, uint32_t>(true, 0));
				

				if (_write_error_at > 0)
					if(_ptag_write_failure != nullptr)
						_ptag_write_failure->SendMessage(UWM_WRITEERR_ERROR_SINCE, _write_error_at, 0);

				change_tag_state(STATE_RF_TAG_PROGRAMED_OK, current_tag);
				_state_status_rf_bank = STATE_RF_BANK_WRITE_APPDATA_OK;
				send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.. do not block
			}
			else 
			{
				++_session_tags_failed;
				_gpio_wrapper->DisableWriting();

				_write_error_at = _session_tags_scanned;
				if(_ptag_write_failure == nullptr)
					::PostMessage(AfxGetMainWnd()->m_hWnd, UWM_DISPLAY_WRITEERR_DLG, 0, 0);
				else
				{
					_ptag_write_failure->SendMessage(UWM_WRITEERR_ERROR_AT, _write_error_at, 0);
					_ptag_write_failure->SendMessage(UWM_WRITEERR_ERROR_SINCE, 0, 0);
				}


				PlaySound(_T("sounds\\Computer Error.wav"), NULL, SND_APPLICATION | SND_ASYNC);

				if (_hw_rf_error.value == 0x0112)//occurred on a tag with secured data
				{
					change_tag_state(STATE_RF_TAG_PROGRAMED_FAIL_LOCKEDTAG, current_tag);
					_state_status_rf_bank = STATE_RF_BANK_WRITE_APPDATA_FAILED;
					Logger::L().Log (MODULENAME, severity_level::error, "Invalid Tag occurred at tag count: " + std::to_string(_session_tags_scanned) + "");
				}
				else if (tag_missing())//can not write if there is no tag.
				{
					change_tag_state(STATE_RF_TAG_PROGRAMED_FAIL, current_tag);
					_state_status_rf_bank = STATE_RF_BANK_WRITE_APPDATA_FAILED;
					Logger::L().Log (MODULENAME, severity_level::error, "Write error occurred at tag count: " + std::to_string(_session_tags_scanned) + "");
				}
				else if (_hw_rf_error.error.type == 0 && _hw_rf_error.error.code == 1)
				{
					//tag is not in field anymore.
					change_tag_state(STATE_RF_TAG_OFFLINE, current_tag);
					_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;
					send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan..
					Logger::L().Log (MODULENAME, severity_level::error, "Write error, tag not in range: " + std::to_string(_session_tags_scanned) + "");
				}
				else
				{
					change_tag_state(STATE_RF_TAG_PROGRAMED_FAIL, current_tag);
					_state_status_rf_bank = STATE_RF_BANK_WRITE_APPDATA_FAILED;
					Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("Write error, tag #%u had an error: 0x%08X") %  _session_tags_scanned % uint16_t(_hw_rf_error.value)));
				}

				uint16_t hwv = _hw_rf_error.value;
				session_tag_history[visible_tag_serialnumbers[current_tag]].push_back(std::make_pair(false, hwv));
			}
			SLBL_TotalTags_Value.Format(_T("Total tags: %d  Programmed: %d  Failed: %d"), _session_tags_scanned, _session_tags_successful, _session_tags_failed);
			_hw_rf_error.value = 0;

			break;
		}
		case STATE_RF_BANK_WRITE_APPDATA_OK:
		{
			bool busy = _pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy);
			rf_reader_error();
			if (busy == true && !tag_missing()) break;

			// Stall here until we see the tag removed from the reader.
			if (tag_missing())
			{
				//we expect errors when the RFID tag is removed from the reader
				//do not need to check for them
				_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;

				change_tags_state_all(STATE_RF_TAG_OFFLINE, TOTAL_SUPPORTED_RF_TAGS);
				show_pack_label(current_tag, FALSE);
				total_tags_reported = 0;
				current_tag = 0;
			}
			else
			{
				send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.. do not block
			}

			break;
		}
		case STATE_RF_BANK_WRITE_APPDATA_FAILED:
		{
			bool busy = _pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy);
			rf_reader_error();
			if (busy == true && !tag_missing()) break;
			
			if (tag_missing() && _hw_rf_error.value != 0x0112)
			{
				// This state change causes a bug that should be in Trac. 
				// Once a failed write occurs, if you remove the tag before dismissing the failure dialog then you come
				// here and the system eventually thinks everything is fine which means a conveyor belt will be moving product
				// until you finally dismiss the dialog.
				// If, however, you dismiss the dialog before removing the failed tag then the payload is cleared and a conveyor will not move.
				//TODO: to fix bug, remove the next line that changes state
//				_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;

				change_tags_state_all(STATE_RF_TAG_OFFLINE, TOTAL_SUPPORTED_RF_TAGS);
				show_pack_label(current_tag, FALSE);
				total_tags_reported = 0;
				current_tag = 0;
			}
			else
			{
				send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.. do not block
			}

			break;
		}
		
		case STATE_RF_BANK_CFGRESET_TIMEOUT:
		{
			if (--timeout_expired <= 0)
			{
				_clear_payload = true;
				_write_error_at = 0;
				show_pack_label(current_tag, FALSE);
				total_tags_reported = 0;
				_state_status_rf_bank = STATE_RF_BANK_INVALID;
			}
			break;
		}
		case STATE_RF_BANK_VERIFY_PAYLOAD_SCAN:
		{
			/**
			 * Get time duration of a tag scan and obtainning its data. 
			 */
			if (--timeout_expired < 0)
			{
				start_time = boost::posix_time::microsec_clock::local_time();
				send_reagent_cmd(ReagentCommandRFScanTagMode, 0);//0-Signal Scan..
				timeout_expired = 10;
			}
			else if(timeout_expired ==  0 || !_pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy))
			{
				_state_status_rf_bank = STATE_RF_BANK_VERIFY_PAYLOAD_READ;
			}
		}
		break;
		case STATE_RF_BANK_VERIFY_PAYLOAD_READ:
		{
			std::vector<uint32_t> tag_data(sizeof(RfAppdataRegisters) / sizeof(uint32_t));
			stop_time = boost::posix_time::not_a_date_time;			
			std::fill(tag_data.begin(), tag_data.end(), 0);
			get_reagent_data(ReagentRFTagsAppData, tag_data);
			
			stop_time = boost::posix_time::microsec_clock::local_time();
			auto diff = stop_time - start_time;
			Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("tag #%d - Read Time: %d msec") % total_tags_reported % diff.total_milliseconds()));
			Logger::L().Flush();
				

			/**
			 *Confirm the communication to controller board was successful. Then,
			 *Confirm the tag header has valid states and then
			 *compare it to requested payload  
			 */
			RfAppdataRegisters rf_appdata_registers;
			std::copy(tag_data.begin(), tag_data.end(), stdext::checked_array_iterator<uint32_t*>(reinterpret_cast<uint32_t*>(&rf_appdata_registers),
                                                                                                  sizeof(RfAppdataRegisters) / sizeof(uint32_t)));
			//Copy rfid application parameters map data into a vector, so it can be easily compared to a vector of the selected payload data.. 
			std::vector<uint32_t> app_data(reinterpret_cast<uint32_t*>(rf_appdata_registers.ParamMap),
										   reinterpret_cast<uint32_t*>(rf_appdata_registers.ParamMap + sizeof(rf_appdata_registers.ParamMap)));
			std::vector<uint32_t> payload_data;
			get_payload_data(payload_data);
			auto is_eq = std::equal(payload_data.begin(), payload_data.end(), app_data.begin());

			if(rf_reader_error() || 
			   !is_headerstatus_valid(rf_appdata_registers.Status) || 
			   !is_eq)
			{
				_verify_error_at = _session_tags_scanned; 			
				if (_ptag_verify_failure == nullptr)
				{
					::PostMessage(AfxGetMainWnd()->m_hWnd, UWM_DISPLAY_VERIFYERR_DLG, 0, 0);
				}
				else
				{
					_ptag_verify_failure->SendMessage(UWM_VERIFYERR_ERROR_AT, _verify_error_at, 0);
				}

				PlaySound(_T("sounds\\Computer Error.wav"), NULL, SND_APPLICATION | SND_ASYNC);
				change_tag_state(STATE_RF_TAG_VERIFY_FAIL, current_tag);
				++_session_tags_failed;
				_gpio_wrapper->DisableWriting();

                                                                              % is_headerstatus_valid(rf_appdata_registers.Status) 
			{
				PlaySound(_T("sounds\\Checkout Scanner Beep.wav"), NULL, SND_APPLICATION | SND_ASYNC);
				change_tag_state(STATE_RF_TAG_VERIFY_OK, current_tag);
				++_session_tags_successful;

				// only want to signal success once, so this is the place to do it.
				_gpio_wrapper->SignalWriteSuccess();

				session_tag_history[visible_tag_serialnumbers[current_tag]].push_back(std::make_pair(true, 0));
			}

				// only want to signal success once, so this is the place to do it.
				_gpio_wrapper->SignalWriteSuccess();

				session_tag_history[visible_tag_serialnumbers[current_tag]].push_back(std::make_pair(true, 0));
			}

			if (_ptag_verify_failure != nullptr)
			{
				_ptag_verify_failure->SendMessage(UWM_VERIFYERR_ERROR_SINCE, _session_tags_scanned - _verify_error_at, 0);
			}
			SLBL_TotalTags_Value.Format(_T("Total tags: %d  Verified: %d  Failed: %d"), _session_tags_scanned, _session_tags_successful, _session_tags_failed);

			_state_status_rf_bank = STATE_RF_BANK_VERIFY_PAYLOAD_COMPLETE;
			send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.	
			}
		break;
		case STATE_RF_BANK_VERIFY_PAYLOAD_COMPLETE:
		{
				bool busy = _pcontroller_board->GetBoardStatus().isSet(BoardStatus::ReagentBusy);
			rf_reader_error();
			if (busy == true && !tag_missing()) break;

			if (tag_missing())
			{
				_state_status_rf_bank = STATE_RF_BANK_UPDATE_TAGS;

				change_tags_state_all(STATE_RF_TAG_OFFLINE, TOTAL_SUPPORTED_RF_TAGS);
				show_pack_label(current_tag, FALSE);
				total_tags_reported = 0;
				current_tag = 0;
			}
			else
				send_reagent_cmd(ReagentCommandRFScanTagMode, 1, false);//1-AUtoScan.. do not block

			break;
				}
		default:
			break;
	}

	change_status_label<state_rf_bank_status>(_state_status_rf_bank, prev_state,
                                              SLBL_RFID_Overall_Value, _brush_status_RFID_overall,
                                              enum_map_status_rfbank, _update_required);
}

bool CRFIDProgrammingStationDlg::_fw_update_finished = false;
uint32_t CRFIDProgrammingStationDlg::_fw_update_progress = 0;
void CRFIDProgrammingStationDlg::firmware_update_progress(bool finish, int16_t total)
{
	_fw_update_finished = finish;
	_fw_update_progress = total;
}

bool CRFIDProgrammingStationDlg::tag_missing()
{
    return (_hw_rf_error.error.type == 0 &&  _hw_rf_error.error.code == 1 || // 0x001: No Tag Found
            _hw_rf_error.error.type == 1 && (_hw_rf_error.error.code == 2 || // 0x102: No tag in field
                                             _hw_rf_error.error.code == 7 ||
                                             _hw_rf_error.error.code == 18));//0x0112 - unknown error - this occurs when the tag is removed 
}

bool CRFIDProgrammingStationDlg::is_headerstatus_valid(TagStatus tag_status)
{
	//std::string tag_sn_str = boost::str(boost::format("%X%X%X%X%X")
 //                                       % int(tag_status.TagSN[0])
 //                                       % int(tag_status.TagSN[1])
 //                                       % int(tag_status.TagSN[2])
 //                                       % int(tag_status.TagSN[3])
 //                                       % int(tag_status.TagSN[4]));
	//Getting serial number disabled since invalid status tag will use previous good tag. 

	if (tag_status.ProgramStatus != 1 ||
		tag_status.ValidationStatus != 1 ||
		tag_status.AuthStatus != 1)
	{
		if (tag_status.ProgramStatus == 0) Logger::L().Log (MODULENAME, severity_level::error,
                                                                         boost::str(boost::format("tag was not programmed.") ));
		if (tag_status.ValidationStatus == 0) Logger::L().Log (MODULENAME, severity_level::error,
                                                                            boost::str(boost::format("Unable to validate tag.") ));
		if (tag_status.ValidationStatus == 2) Logger::L().Log (MODULENAME, severity_level::error,
                                                                            boost::str(boost::format("Invalid reagent data.") ));
		if (tag_status.AuthStatus == 0) Logger::L().Log (MODULENAME, severity_level::error,
                                                                      boost::str(boost::format("Unable to authorize reagent tag.") ));
		if (tag_status.AuthStatus == 2) Logger::L().Log (MODULENAME, severity_level::error,
                                                                      boost::str(boost::format("Authorize reagent tag Failed.") ));
		return false;
	}
	return true;
}

BoardStatus CRFIDProgrammingStationDlg::get_conntroller_cmd(RegisterIds cmd, uint32_t& data)
{
	BoardStatus boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Read,
	                                                           RegisterIds(cmd),
	                                                           &data, sizeof(uint32_t), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
	return boardStatus;
}

void CRFIDProgrammingStationDlg::get_reagent_cmd(ReagentRegisterOffsets cmd, uint32_t &payload, bool block)
{
	BoardStatus boardStatus;

	BoardStatus::StatusBit bit_to_check;
	if (block == false)
		bit_to_check = BoardStatus::DoNotCheckAnyBit;
	else
		bit_to_check = BoardStatus::ReagentBusy;
	
	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Read,
                                                        RegisterIds(RegisterIds::ReagentRegs + cmd),
                                                        &payload, sizeof(uint32_t), bit_to_check, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
}

void CRFIDProgrammingStationDlg::get_reagent_data(ReagentRegisterOffsets cmd, std::vector<uint32_t> &payload, uint32_t offset)
{
	BoardStatus boardStatus;

	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Read,
                                                    RegisterIds(RegisterIds::ReagentRegs + cmd + offset),
                                                    payload.data(), static_cast<uint32_t>(payload.size() * sizeof(uint32_t)), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);


}

void CRFIDProgrammingStationDlg::set_reagent_cmd(ReagentRegisterOffsets cmd, uint32_t payload)
{
	BoardStatus boardStatus;
	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Write,
                                                            RegisterIds(RegisterIds::ReagentRegs + cmd),
                                                            &payload, sizeof(uint32_t), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);

}

void CRFIDProgrammingStationDlg::send_reagent_data(ReagentRegisterOffsets cmd, std::vector<uint32_t> & payload, uint32_t offset)
{
	if(payload.size() <= 0)
		return;

	BoardStatus boardStatus;
	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Write,
                                                   RegisterIds(RegisterIds::ReagentRegs + cmd + offset),
                                                        payload.data(), uint32_t(payload.size()* sizeof(uint32_t)), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
}

void CRFIDProgrammingStationDlg::send_reagent_cmd(ReagentCommandRegisterCodes subcmd, uint32_t payload, bool block)
{
	BoardStatus boardStatus;
	uint32_t subcmd_pyload = subcmd;

	BoardStatus::StatusBit bit_to_check;
	if (block == false)
		bit_to_check =  BoardStatus::DoNotCheckAnyBit;
	else
		bit_to_check = BoardStatus::ReagentBusy;

	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Write,
                                                   RegisterIds(RegisterIds::ReagentRegs + ReagentCommandParam),
                                                        &payload, sizeof(uint32_t), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
	
	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Write,
                                                       RegisterIds(RegisterIds::ReagentRegs + ReagentCommand),
                                                       &subcmd_pyload, sizeof(uint32_t), bit_to_check, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
}

void CRFIDProgrammingStationDlg::read_reagent_cmd(ReagentCommandRegisterCodes subcmd, uint32_t &payload, size_t sizepayload)
{
	BoardStatus boardStatus;
	uint32_t subcmd_pyload = subcmd;

	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Write,
												   RegisterIds(RegisterIds::ReagentRegs + ReagentCommand),
												   &subcmd_pyload, sizeof(uint32_t), BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);

	boardStatus = _pcontroller_board_command->send(ControllerBoardMessage::Read,
												   RegisterIds(RegisterIds::ReagentRegs + ReagentCommandParam),
												   &payload, (uint32_t)sizepayload, BoardStatus::ReagentBusy, CONTROLLER_BOARD_SEND_CMD_TIMEOUT);
}

void CRFIDProgrammingStationDlg::change_tags_state_all(state_rf_tags_status to, uint32_t total)
{
	for (uint32_t i = 0; i < total; i++)
	{
		change_tag_state(to, i);
	}
}

void CRFIDProgrammingStationDlg::change_statemachine_state(current_statemachines sm, uint32_t state)
{
	switch (sm)
	{
		case SM_CONTROLLER:
		{
			auto n = state_controller_status(0);
			auto s = state_controller_status(state);
			_state_status_controller = s;
			change_status_label<state_controller_status>(s, n,
                                                         SLBL_Hardware_Value, _brush_status_hardware,
                                                         enum_map_status_contorller, _update_required);
			break;
		}
		case SM_RF_READER:
		{
			auto n = state_hw_rfid_status(0);
			auto s = state_hw_rfid_status(state);
			_state_status_hw_rfid = s;
			change_status_label<state_hw_rfid_status>(s, n,
                                                      SLBL_RFIDReadWrite_Value, _brush_status_RFID_readwrite,
                                                      enum_map_status_hw_rfid, _update_required);
			break;
		}
		case SM_PAYLOAD:
		{
			auto n = state_payload_status(0);
			auto s = state_payload_status(state);
			_state_status_payload = s;
			change_status_label<state_payload_status>(s, n,
                                                      SLBL_Payload_Value, _brush_status_payload,
                                                      enum_map_status_payload, _update_required);
			break;
		}
		case SM_SYSTEM:
		{
			auto n = state_system_status(0);
			auto s = state_system_status(state);
			_state_status_system = s;
			change_status_label<state_system_status>(s, n,
                                                     SLBL_System_Value, _brush_status_system,
                                                     enum_map_status_system, _update_required);
			break;
		}
		case SM_RF_BANK:
		{
			auto n = state_rf_bank_status(0);
			auto s = state_rf_bank_status(state);
			_state_status_rf_bank = s;
			change_status_label<state_rf_bank_status>(s, n,
                                                      SLBL_RFID_Overall_Value, _brush_status_RFID_overall,
                                                      enum_map_status_rfbank, _update_required);
			break;
		}
		case SM_RF_TAG:
		{
			auto n = state_rf_tags_status(0);
			auto s = state_rf_tags_status(state);
			change_status_label<state_rf_tags_status>(s, n,
                                                      SLBL_RFID_Overall_Value, _brush_status_RFID_overall,
                                                      enum_map_status_rftag, _update_required);
			break;
		}
		case TOTAL_SM: 
			break;
		default:
			break;
	}
}

void CRFIDProgrammingStationDlg::change_tag_state(CRFIDProgrammingStationDlg::state_rf_tags_status to, int i)
{
	_state_status_rf_tags[i] = to;
	
	if (enum_map_status_rftag.find(to) == enum_map_status_rftag.end())
	{
		*(_state_strings_color_rf_tags[i]).first = "Unknown State";
		change_color(*(_state_strings_color_rf_tags[i]).second,RGB_COLOR_RED);
	}
	else
	{
		*(_state_strings_color_rf_tags[i]).first = enum_map_status_rftag.find(to)->second.first;
		change_color(*(_state_strings_color_rf_tags[i]).second, enum_map_status_rftag.find(to)->second.second);
	}

	
}

void CRFIDProgrammingStationDlg::change_color(CBrush &brush, COLORREF color)
{
	brush.DeleteObject();
	brush.CreateSolidBrush(color);
}

bool CRFIDProgrammingStationDlg::InitializeController()
{
	_pcontroller_board.reset(new ControllerBoardOperation(_hs->getInternalIos(), CNTLR_SN_A_STR, CNTLR_SN_B_STR));
	if(!_pcontroller_board->Initialize()) return false;

	if (_pcontroller_board_command == nullptr)
	{
		_pcontroller_board_command.reset(new ControllerBoardCommand(_pcontroller_board));
		_pcontroller_board_command->clearAllErrorCodes();
	}
	return true;
}

// CRFIDProgrammingStationDlg message handlers
BOOL CRFIDProgrammingStationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	ShowWindow(SW_MAXIMIZE);

	// TODO: Add extra initialization here
	auto index = 0;
	for (auto entry : firmware_bin_entries)
	{
		CString description(entry.GetDescription().c_str(), int(entry.GetDescription().length()));
		CB_PartDescription_Control.InsertString(index++, description);
	}

	CFont m_font;
	m_font.CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
	GetDlgItem(IDC_LABLE_STATUS_HARDWARE)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_CONVEYOR)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_RWHW)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_PAYLOAD)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_SYSTEM)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_OVERALL)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM1)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM2)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM3)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM4)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM5)->SetFont(&m_font);
	GetDlgItem(IDC_LABLE_STATUS_RFID_NUM6)->SetFont(&m_font);

	_thread_param.dlg_instance = this;
	AfxBeginThread(update_all_statemachines, &_thread_param);
	return TRUE;
}

void CRFIDProgrammingStationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CRFIDProgrammingStationDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRFIDProgrammingStationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CRFIDProgrammingStationDlg::OnModelDataUpdate(WPARAM wparam, LPARAM)
{
	BOOL mode = static_cast<BOOL>(wparam);
	CSingleLock singleLock(&_critsection_dataupdate, TRUE);

	//TRUE- Transfer data from controls to variables
	//FALSE- Transfer data from variables to controls
	UpdateData(mode);
	singleLock.Unlock();
	return LRESULT(0);
}

LRESULT CRFIDProgrammingStationDlg::OnUpdate_Label_Data(WPARAM wparam, LPARAM lparam)
{
	auto ch_input = static_cast<char>(wparam);
	auto control_input = static_cast<char>(lparam);
	
	if (control_input == VK_RETURN || control_input == VK_ESCAPE)
	{
		if (EDIT_LotNumber_Control.LineLength() <= 0)
		{
			std::string failure;
			// Convert a TCHAR string to a LPCSTR
			CT2CA pszConvertedAnsiString(_label_bar_code);
			// construct a std::string using the LPCSTR input
			std::string barcode_string(pszConvertedAnsiString);

			Logger::L().Log (MODULENAME, severity_level::debug1, "Barcode: " + barcode_string);
			if (!ParseBarcodeString(barcode_string, _GTIN, _MFG_date, _EXP_date, _LOT_number, failure))
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "Barcode Failure:" + failure);
				change_statemachine_state(SM_PAYLOAD, STATE_PAYLOAD_BARCODE_INVALID);
			}
			else
			{
				std::stringstream sslog;
				sslog << "Barcode data: GTIN[" + _GTIN + "] MFG_DATE[" + _MFG_date + "] EXP_DATE[" + _EXP_date + "] LOT#[" + _LOT_number + "]" << std::endl;

				size_t index = 0;
				change_statemachine_state(SM_PAYLOAD, STATE_PAYLOAD_BARCODE_GTIN_NOTFOUND);
				sslog << "Searching for GTIN: " + _GTIN << std::endl;
				for (auto entry : firmware_bin_entries)
				{
					sslog << "\t entry #" + std::to_string(index) + " :" + entry.GetGTIN() << std::endl;
					if (entry.GetGTIN() == _GTIN)
					{
						sslog << "\t found entry at #" + std::to_string(index) << std::endl;
						change_statemachine_state(SM_PAYLOAD, STATE_PAYLOAD_BARCODE_VALID);
						CB_PartDescription_Control.SetCurSel(int(index));
						update_data_safe(TRUE);// Transfer data from controls to variables
						BTN_ApplicationMode_Control.EnableWindow(FALSE);
						break;
					}
					index++;
				}
				Logger::L().Log (MODULENAME, severity_level::debug1, sslog.str());
				if (_state_status_payload == STATE_PAYLOAD_BARCODE_GTIN_NOTFOUND)
					Logger::L().Log (MODULENAME, severity_level::warning, "Unable to locate GTIN: " + _GTIN + " : from known platform products");

				DT_Expiration_Value = StringToOleDateTime(_EXP_date);
				if (DT_Expiration_Value.GetStatus() != ATL::COleDateTime::DateTimeStatus::valid)
				{
					change_statemachine_state(SM_PAYLOAD,STATE_PAYLOAD_BARCODE_INVALID_DATE);
					Logger::L().Log (MODULENAME, severity_level::error, "Invalid expiration date \"" + _EXP_date + "\" read from barcode");
				}

				auto DT_Mfg_Value = StringToOleDateTime(_MFG_date);
				if (DT_Mfg_Value.GetStatus() != ATL::COleDateTime::DateTimeStatus::valid)
				{
					change_statemachine_state(SM_PAYLOAD, STATE_PAYLOAD_BARCODE_INVALID_DATE);
					Logger::L().Log (MODULENAME, severity_level::error, "Invalid manufacturing date \"" + _MFG_date + "\" read from barcode");
				}

			}
			_label_bar_code.Delete(0, _label_bar_code.GetLength());
		}
	}
	else
	{
		_label_bar_code.AppendChar(ch_input);
	}
	return true;
}

LRESULT CRFIDProgrammingStationDlg::OnDisplayWriteErrDlg(WPARAM, LPARAM)
{
	if (_ptag_write_failure == nullptr)
		_ptag_write_failure.reset(new TagWriteFailureDlg(AfxGetMainWnd()));
	if (_ptag_write_failure != nullptr)
	{
		_ptag_write_failure->SendMessage(UWM_WRITEERR_ERROR_AT, _write_error_at, 0);
		_ptag_write_failure->SendMessage(UWM_WRITEERR_ERROR_SINCE, 0, 0);
	}
	return LRESULT(0);
}

LRESULT CRFIDProgrammingStationDlg::OnDisplayVerifyErrDlg(WPARAM, LPARAM)
{
	if (_ptag_verify_failure == nullptr)
		_ptag_verify_failure.reset(new TagVerifyFailureDlg(AfxGetMainWnd()));
	if (_ptag_verify_failure != nullptr)
	{
		_ptag_verify_failure->SendMessage(UWM_VERIFYERR_ERROR_AT, _verify_error_at, 0);
		_ptag_verify_failure->SendMessage(UWM_VERIFYERR_ERROR_SINCE, 0, 0);
	}
	return LRESULT(0);
}

LRESULT CRFIDProgrammingStationDlg::OnStopScanning(WPARAM wparam, LPARAM lparam)
{
	int stop_scanning = static_cast<int>(wparam);

	if (_ptag_write_failure != nullptr)
	{
		_ptag_write_failure->SendMessage(WM_CLOSE);
		_ptag_write_failure.reset();
	}

	if(_ptag_verify_failure != nullptr)
	{
		_ptag_verify_failure->SendMessage(WM_CLOSE);
		_ptag_verify_failure.reset();
	}

	if (stop_scanning)
		_clear_payload = true;

	return LRESULT(0);
}

void CRFIDProgrammingStationDlg::OnCbnSelchangeComboPartDescription()
{
	update_data_safe(TRUE);// Transfer data from controls to variables
	_GTIN = firmware_bin_entries[CB_PartDescription_Control.GetCurSel()].GetGTIN();
	EDIT_LotNumber_Value.Delete(0, EDIT_LotNumber_Value.GetLength());
	update_data_safe(FALSE);// Transfer data from variables to controls
}

HBRUSH CRFIDProgrammingStationDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here	
	switch (pWnd->GetDlgCtrlID())
	{
		case IDC_LABLE_STATUS_HARDWARE:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_hardware.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_CONVEYOR:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_conveyor.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_SYSTEM:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_system.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_RWHW:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_readwrite.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_PAYLOAD:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_payload.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_OVERALL:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_overall.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM1:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb1.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM2:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb2.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM3:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb3.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM4:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb4.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM5:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb5.GetSafeHandle());
			break;
		}
		case IDC_LABLE_STATUS_RFID_NUM6:
		{
			pDC->SetTextColor(RGB_COLOR_BLK);
			pDC->SetBkMode(TRANSPARENT);
			hbr = HBRUSH(_brush_status_RFID_numb6.GetSafeHandle());
			break;
		}
		default: 
			break;
	}

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}

void CRFIDProgrammingStationDlg::clear_payload_information()
{
	CB_PartDescription_Control.SetCurSel(-1);
	DT_Expiration_Control.SetTime(COleDateTime::GetCurrentTime().GetTickCount());
	EDIT_LotNumber_Control.SetWindowTextW(_T("")); 

	BTN_ApplicationMode_Control.EnableWindow();
}

void CRFIDProgrammingStationDlg::update_data_safe(BOOL mode)
{
	//TRUE- Transfer data from controls to variables
	//FALSE- Transfer data from variables to controls
	CSingleLock singleLock(&_critsection_dataupdate, TRUE);
	UpdateData(mode);
	singleLock.Unlock();
}

void CRFIDProgrammingStationDlg::OnBnClickedBtnClearselection()
{
	std::stringstream sslog;
	if (_session_tags_scanned > 0)
	{
		std::time_t started_time = std::chrono::system_clock::to_time_t(_time_started_scanning);
		char s[50];
		ctime_s(s, 50, &started_time);
		std::string ctimestr(s);
		ctimestr.erase(std::remove(ctimestr.begin(), ctimestr.end(), '\n'), ctimestr.end());
		
		uint32_t pass = 0;
		uint32_t fail = 0;

		std::string tag_history = "Tag History from session:";
		for (auto iter = session_tag_history.begin(); iter != session_tag_history.end(); iter++)
		{
			std::string this_tag_history = "(";
			for (auto evt = iter->second.begin(); evt != iter->second.end(); evt++)
			{
				if (evt != iter->second.begin())
					this_tag_history += ",";

				if (evt->first)
					this_tag_history += "Pass";
				else
					this_tag_history += boost::str(boost::format("Fail<%d>") % evt->second);

				


			}
			this_tag_history += ")";

			auto last = iter->second.rbegin();
			std::string last_action;
			if (last == iter->second.rend())
			{
				last_action = "NONE";
			}
			else
			{
				if (last->first)
				{
					last_action = "Pass";
					pass++;
				}
				else
				{
					last_action = "Fail";
					fail++;
				};
			}

			tag_history += (boost::str(boost::format("\n\tSN: %s : %s : %s") % iter->first % last_action % this_tag_history));
		}

		CString exp_date_cstr = StringToOleDateTime(_EXP_date).Format(_T("%B %d, %Y"));

		WorkLogger::L().Log("Programmer", severity_level::notification, boost::str(boost::format(
            "Record of session\nSession Start: %s\nGTIN: %s\nLot: %s\nExpiration: %s\nProgrammed/Verified Tags: %d\nFailed Tags: %d\n%s")
            % ctimestr
            % _GTIN
            % _LOT_number
            % CStrToStdStr(exp_date_cstr)
            % pass
            % fail
		    % tag_history));

		
	}
	_session_tags_scanned = _session_tags_successful = _session_tags_failed = 0;
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "Reagent information was cleared.");

	update_data_safe(FALSE);// Transfer data from variables to controls... we have to do this first. 
	clear_payload_information();
	update_data_safe(TRUE);// Transfer data from controls to variables
}

void CRFIDProgrammingStationDlg::OnDtnDatetimechangeDatetimeExpiration(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
	update_data_safe(TRUE);// Transfer data from controls to variables
	*pResult = 0;
}

void CRFIDProgrammingStationDlg::OnEnChangeEditLotnumber()
{
	// all numeric input will be stored in bar code label member.
	//we can clear barcode data since we know that current input was meant for lot number.
	_label_bar_code.Delete(0, _label_bar_code.GetLength());
	update_data_safe(TRUE);// Transfer data from controls to variables
}

void CRFIDProgrammingStationDlg::OnEnSetfocusEditLotnumber()
{
	EDIT_LotNumber_Value.Delete(0, EDIT_LotNumber_Value.GetLength());
	update_data_safe(FALSE);// Transfer data from variables to controls
}

void CRFIDProgrammingStationDlg::export_log_files(const std::string & cs)
{
	boost::system::error_code ec;

	boost::filesystem::path to_file(cs + "logs");
	boost::filesystem::path from_file("\\Instrument\\Logs");
	boost::filesystem::copy_directory(from_file, to_file, ec);
	
	for (boost::filesystem::directory_iterator file(from_file);
         file != boost::filesystem::directory_iterator();
         ++file)
	{
		boost::filesystem::path current(file->path());		

		boost::filesystem::copy_file(current, to_file / current.filename(), boost::filesystem::copy_option::fail_if_exists, ec);
		if (ec.value())
		{
			AfxMessageBox(_T("Error occurred while copying files:\n") + StdStrToCStr(ec.message()), MB_OK | MB_ICONERROR);
			break;
		}
	}
}

bool CRFIDProgrammingStationDlg::backup_payload_files(boost::filesystem::path &homepath, boost::system::error_code& ec, bool remove_after_backup)
{
	if (!boost::filesystem::is_directory(homepath, ec))
		return false;

	const auto backup_dirname = std::string("backup_" + boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()));
	if(!boost::filesystem::create_directory(homepath / backup_dirname, ec))return false;
	for (auto& payloadfile : boost::make_iterator_range(boost::filesystem::directory_iterator(homepath), {}))
	{
		if (boost::filesystem::is_regular_file(payloadfile.status()))
		{
			boost::filesystem::copy_file(payloadfile,
											homepath / (backup_dirname +"\\"+ payloadfile.path().filename().string()), ec);

			if (remove_after_backup)
				boost::filesystem::remove(payloadfile);
		}
	}
	return true;
}

bool CRFIDProgrammingStationDlg::is_result_error(boost::system::error_code ec) const
{
	if (!ec)
		return false;

	AfxMessageBox(_T("Error occurred while copying files:\n") + StdStrToCStr(ec.message()), MB_OK | MB_ICONERROR);
	return true;
}

void CRFIDProgrammingStationDlg::import_payload_files(const std::string & cs, bool replace_payload)
{
	/*

	boost::system::error_code ec;
	boost::filesystem::path to_path(boost::filesystem::current_path().string() + "\\ReagentPayloads\\");
	boost::filesystem::path target_path(cs);
	
	// Backup previous contents
	backup_payload_files(to_path, ec, replace_payload);
	if (is_result_error(ec))
		return;

	// Backup previous contents
	backup_payload_files(to_path, ec, replace_payload);
	if (is_result_error(ec))
		return;

	// Copy new files over
	boost::filesystem::directory_iterator d_iter(target_path);
	for (/**/; d_iter != boost::filesystem::directory_iterator{}; d_iter++)
	{
		boost::filesystem::path p{ *d_iter };
		if (!boost::filesystem::is_regular_file(p) ||
			!p.has_extension() ||
			(p.extension() != ".bin" && p.extension() != ".payload"))
			continue;

		boost::filesystem::path to_file = to_path / p.filename();
		boost::filesystem::copy_file(p, to_file, boost::filesystem::copy_option::overwrite_if_exists, ec);
		if (ec)
		{
			AfxMessageBox(_T("Unable to copy file!"));
			continue;
		}
	}

	// Load payload entries
	auto bin_entries = LoadFirmwarePayloads_f(to_path.string());

	if (bin_entries.size() == 0)
	{
		AfxMessageBox(_T("Error occurred!\n \
                         Unable to locate or verify binary files.\n \
                         The file that was selected might not be a reagent lookup table."), MB_OK | MB_ICONERROR);
		return;
	}
	// Set available payload entries table from new list.

	firmware_bin_entries = bin_entries;
	CB_PartDescription_Control.Clear();
	auto index = 0;

	std::string new_log_entries = "New Payload Lookup Table and Binaries Copied Over:\n";

	for (auto entry : firmware_bin_entries)
	{
		CString description(entry.GetDescription().c_str(), int(entry.GetDescription().length()));
		CB_PartDescription_Control.InsertString(index++, description);

		new_log_entries += "\t" + entry.GetDescription() + " : " + entry.GetGTIN() + "\n";
	}

	new_log_entries += "From: " + target_path.string();
	WorkLogger::L().Log("Programmer", severity_level::normal, new_log_entries);
}

void CRFIDProgrammingStationDlg::OnBnClickedBtnExportLog()
{
	EBWS_ExportLogPath_Control.OnBrowse();
	update_data_safe(TRUE);// Transfer data from controls to variables
	if (EBWS_ExportLogPath_Value.GetLength() > 0)
		export_log_files(CStrToStdStr(EBWS_ExportLogPath_Value));
}

void CRFIDProgrammingStationDlg::OnBnClickedBtnImportPayloadFiles()
{
	EBWS_ImportPayloadPath_Control.OnBrowse();
	update_data_safe(TRUE);// Transfer data from controls to variables
	if (EBWS_ImportPayloadPath_Value.GetLength() > 0)
		import_payload_files(CStrToStdStr(EBWS_ImportPayloadPath_Value), CHECK_ReplacePayload.GetCheck());
}

void CRFIDProgrammingStationDlg::OnBnClickedBtnAppMode()
{
	_application_mode = static_cast<app_mode>((_application_mode + 1) % APP_MODE_MAX);//cycle to the next mode
	BTN_ApplicationMode_Control.SetWindowText(_T("Application &Mode: ") + enum_app_mode_string[_application_mode]);
}


