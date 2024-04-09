#pragma once

#include "afxwin.h"
#include "afxdtctl.h"
#include "ATLComTime.h"

#include <memory>
#include <thread>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/thread/condition_variable.hpp>

#include "BoardStatus.hpp"
#include "Utilities.hpp"
#include "ControllerBoardCommand.hpp"
#include "FirmwareUpdateProgressDlg.h"
#include "HawkeyeConfig.hpp"
#include "afxeditbrowsectrl.h"
#include "TagVerifyFailureDlg.h"
#include "TagWriteFailureDlg.h"
#include "GpioWrapper.h"

class ControllerBoardInterface;
#define TOTAL_SUPPORTED_RF_TAGS 6
#define WORKER_THREAD_SLEEP 50 //milliseconds
#define SECONDS_TO_TIMEOUT_COUNT(sec) (sec*1000)/WORKER_THREAD_SLEEP
#define MILLISECONDS_TO_TIMEOUT_COUNT(msec) (msec>WORKER_THREAD_SLEEP)?msec/WORKER_THREAD_SLEEP:1
#define TOTAL_TAGS_NOTFOUND_LIMIT 500
#define TOTAL_RETRY_SCANDS_AFTER_WRITE 5
#define TOTAL_RETRY_WRITE 3
#define CONTROLLER_BOARD_SEND_CMD_TIMEOUT 1263
#define RFID_PARAMETER_MAP_SIZE 252
#define RFID_STATUS_SIZE 8

#define UWM_UPDATEMODEL_DATA (WM_USER + 1)
#define UWM_UPDATE_LABEL_DATA (WM_USER + 2)
#define UWM_DISPLAY_WRITEERR_DLG (WM_USER + 3)
#define UWM_DISTROY_WRITEERR_DLG (WM_USER + 5)
#define UWM_DISPLAY_VERIFYERR_DLG (WM_USER + 8)
#define UWM_DISTROY_VERIFYERR_DLG (WM_USER + 9)

#define RGB_COLOR_GRY RGB(180,180,180)
#define RGB_COLOR_YELW RGB(255,255,0)
#define RGB_COLOR_BLK RGB(10,10,0)
#define RGB_COLOR_RED RGB(255,127,127)
#define RGB_COLOR_GRN RGB(50,255,50)
#define RGB_COLOR_BLU RGB(66,134,255)

union hw_rf_error
{
	uint16_t value;
	struct 
	{
		unsigned code : 8;
		unsigned type: 4;
		unsigned : 4;
	}error;
};

union _rftag_appdata_header
{
	std::array <uint32_t, 2> data32;
	struct
	{
		unsigned char un1 : 8;
		unsigned char un2 : 8;
		unsigned char un3 : 8;
		unsigned char un4 : 8;
		unsigned char un5 : 8;
		unsigned char authentication_status : 8;
		unsigned char validation_status:8;
		unsigned char program_status :8;
	}rf_tag;
};

typedef struct {
	std::string payload_lookupfile;
	HawkeyeConfig::HawkeyeConfig_t* hawkeyeConfig;
} StationAppConfig_t;

// CRFIDProgrammingStationDlg dialog
class CRFIDProgrammingStationDlg : public CDialogEx
{
private:
	//Enumerations
	enum current_statemachines
	{
		SM_CONTROLLER,
		SM_RF_READER,
		SM_PAYLOAD,
		SM_SYSTEM,
		SM_RF_BANK,
		SM_RF_TAG,
		TOTAL_SM
	};
	enum app_mode
	{
		APP_MODE_PROGRAMMING=0,
		APP_MODE_VERIFYING,
		APP_MODE_MAX
	};
	const std::vector<CString> enum_app_mode_string{
		_T("Programming"),
		_T("Verifying")
	};

	enum state_controller_status : uint32_t
	{
		STATE_CONTROLLER_NULL = 0,
		STATE_CONTROLLER_OFFLINE,
		STATE_CONTROLLER_INIT_FAILED,
		STATE_CONTROLLER_ERROR,
		STATE_CONTROLLER_FW_INIT,
		STATE_CONTROLLER_FW_UPDATE,
		STATE_CONTROLLER_FW_UPDATE_SUCCESS,
		STATE_CONTROLLER_FW_UPDATE_FAILED,
		STATE_CONTROLLER_VALID,
		STATE_CONTROLLER_CHANGED_INVALID
	};
	const std::map<state_controller_status, std::pair< CString, COLORREF>> enum_map_status_contorller{
			{ STATE_CONTROLLER_NULL, {_T("NULL!"),  RGB_COLOR_RED}},
			{ STATE_CONTROLLER_OFFLINE,{ _T("Offline"), RGB_COLOR_GRY }},
			{ STATE_CONTROLLER_INIT_FAILED,{ _T("Initialize Failed!"),RGB_COLOR_RED }},
			{ STATE_CONTROLLER_ERROR, {_T("Error! :"), RGB_COLOR_RED}},
			{ STATE_CONTROLLER_FW_INIT, {_T("Updating FW"), RGB_COLOR_YELW}},
			{ STATE_CONTROLLER_FW_UPDATE,{ _T("Updating FW"), RGB_COLOR_YELW } },
			{ STATE_CONTROLLER_FW_UPDATE_SUCCESS,{ _T("Firmware Update Completed"), RGB_COLOR_BLU } },
			{ STATE_CONTROLLER_FW_UPDATE_FAILED,{ _T("Firmware Update Failed"), RGB_COLOR_RED } },
			{ STATE_CONTROLLER_VALID, {_T("Connected"), RGB_COLOR_GRN }},
			{ STATE_CONTROLLER_CHANGED_INVALID,{ _T("Lost Connection"), RGB_COLOR_RED } }
	};
	

	enum state_hw_rfid_status
	{
		STATE_HW_RFID_NULL = 0,
		STATE_HW_RFID_OFFLINE,
		STATE_HW_RFID_ERROR,
		STATE_HW_RFID_CONNECTED
	};
	const std::map<state_hw_rfid_status, std::pair< CString, COLORREF>> enum_map_status_hw_rfid{
			{ STATE_HW_RFID_NULL, {_T("NULL!"),RGB_COLOR_RED}},
			{ STATE_HW_RFID_OFFLINE,{ _T("Offline"),RGB_COLOR_GRY }},
			{ STATE_HW_RFID_ERROR,{ _T("Error Occurred"),RGB_COLOR_RED}},
			{ STATE_HW_RFID_CONNECTED,{ _T("Connected"),RGB_COLOR_GRN}}
	};

	enum state_payload_status
	{
		STATE_PAYLOAD_NULL = 0,
		STATE_PAYLOAD_INVALID,
		STATE_PAYLOAD_DESCRIPT_NOT_SELECTED,
		STATE_PAYLOAD_LOTNUM_NOT_ENTERED,
		STATE_PAYLOAD_VALID,
		STATE_PAYLOAD_FILES_INVALID,
		STATE_PAYLOAD_BARCODE_INVALID,
		STATE_PAYLOAD_BARCODE_INVALID_DATE,
		STATE_PAYLOAD_BARCODE_VALID,
		STATE_PAYLOAD_BARCODE_GTIN_NOTFOUND
	};
	const std::map<state_payload_status, std::pair< CString, COLORREF>> enum_map_status_payload{
		{ STATE_PAYLOAD_NULL,{ _T("NULL!"),RGB_COLOR_RED }},
		{ STATE_PAYLOAD_INVALID,{ _T("Checking Configuration"),RGB_COLOR_RED }},
		{ STATE_PAYLOAD_DESCRIPT_NOT_SELECTED,{ _T("Part Description Required"),RGB_COLOR_YELW } },
		{ STATE_PAYLOAD_LOTNUM_NOT_ENTERED,{ _T("Lot Number Required"),RGB_COLOR_YELW } },
		{ STATE_PAYLOAD_VALID,{ _T("Payload Loaded"),RGB_COLOR_GRN }},
		{ STATE_PAYLOAD_FILES_INVALID,{ _T("Payload File Corrupted!"),RGB_COLOR_RED } },
		{ STATE_PAYLOAD_BARCODE_INVALID,{ _T("Barcode Scan Invalid"),RGB_COLOR_RED } },
		{ STATE_PAYLOAD_BARCODE_INVALID_DATE,{ _T("Barcode Dates Invalid"),RGB_COLOR_RED } },
		{ STATE_PAYLOAD_BARCODE_VALID,{ _T("Barcode Scan Valid"),RGB_COLOR_BLU } },
		{ STATE_PAYLOAD_BARCODE_GTIN_NOTFOUND,{ _T("GTIN Not Found in Known Entries"),RGB_COLOR_RED } }
	};

	enum state_system_status
	{
		STATE_SYSTEM_NULL = 0,
		STATE_SYSTEM_INVALID,
		STATE_SYSTEM_READY
	};
	const std::map<state_system_status, std::pair< CString, COLORREF>> enum_map_status_system{
		{ STATE_SYSTEM_NULL,{ _T("NULL!"),RGB_COLOR_RED } },
		{ STATE_SYSTEM_INVALID,{ _T("Checking System"),RGB_COLOR_RED } },
		{ STATE_SYSTEM_READY,{ _T("Ready"),RGB_COLOR_GRN } }
	};

	enum state_rf_bank_status
	{
		STATE_RF_BANK_NULL = 0,
		STATE_RF_BANK_INVALID,
		STATE_RF_BANK_HW_ERROR,
		STATE_RF_BANK_UPDATE_TAGS,
		STATE_RF_BANK_ENUMERATE_TAGS,
		STATE_RF_BANK_WRITE_SENDDATA,
		STATE_RF_BANK_WRITE_SENDDATA_WAIT,
		STATE_RF_BANK_WRITE_APPDATA_OK,
		STATE_RF_BANK_WRITE_APPDATA_OK_WAIT,
		STATE_RF_BANK_WRITE_APPDATA_FAILED,
		STATE_RF_BANK_VERIFY_PAYLOAD_SCAN,
		STATE_RF_BANK_VERIFY_PAYLOAD_READ,
		STATE_RF_BANK_VERIFY_PAYLOAD_PASSED,
		STATE_RF_BANK_VERIFY_PAYLOAD_COMPLETE,
		STATE_RF_BANK_CFGRESET_TIMEOUT
	};
	const std::map<state_rf_bank_status, std::pair< CString, COLORREF>> enum_map_status_rfbank{
		{ STATE_RF_BANK_NULL,{ _T("NULL!"),RGB_COLOR_RED } },
		{ STATE_RF_BANK_INVALID,{ _T("Waiting for Payload"),RGB_COLOR_RED } },
		{ STATE_RF_BANK_HW_ERROR,{ _T("Retrying Command"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_UPDATE_TAGS,{ _T("Scanning"),RGB_COLOR_GRN } },
		{ STATE_RF_BANK_WRITE_SENDDATA,{ _T("Programming"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_WRITE_SENDDATA_WAIT,{ _T("Programming"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_WRITE_APPDATA_OK,{ _T("Programed Success\n remove reagent pack(s)"),RGB_COLOR_BLU } },
		{ STATE_RF_BANK_WRITE_APPDATA_OK_WAIT,{ _T("Programed Success\n remove reagent pack(s)"),RGB_COLOR_BLU } },
		{ STATE_RF_BANK_WRITE_APPDATA_FAILED,{ _T("Programed Failed\n remove pack, try again"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_VERIFY_PAYLOAD_SCAN,{ _T("Verifying"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_VERIFY_PAYLOAD_READ,{ _T("Verifying"),RGB_COLOR_YELW } },
		{ STATE_RF_BANK_VERIFY_PAYLOAD_PASSED,{ _T("Verifying\n PASS"),RGB_COLOR_BLU } },
		{ STATE_RF_BANK_VERIFY_PAYLOAD_COMPLETE,{ _T("Verifying\n COMPLETE"),RGB_COLOR_BLU } },
		{ STATE_RF_BANK_CFGRESET_TIMEOUT,{ _T("Clearing Payload Information"),RGB_COLOR_YELW } }
	};

	enum state_rf_tags_status
	{
		STATE_RF_TAG_NULL = 0,
		STATE_RF_TAG_OFFLINE,
		STATE_RF_TAG_INVALID,
		STATE_RF_TAG_BEING_PROGRAMED,
		STATE_RF_TAG_PROGRAMED_OK,
		STATE_RF_TAG_PROGRAMED_FAIL,
		STATE_RF_TAG_PROGRAMED_FAIL_LOCKEDTAG,
		STATE_RF_TAG_VERIFYING,
		STATE_RF_TAG_VERIFY_OK,
		STATE_RF_TAG_VERIFY_FAIL
	};
	const std::map<state_rf_tags_status, std::pair< CString, COLORREF>> enum_map_status_rftag{
		{ STATE_RF_TAG_NULL,{ _T("NULL!"),RGB_COLOR_RED } },
		{ STATE_RF_TAG_OFFLINE,{ _T("Offline"),RGB_COLOR_GRY } },
		{ STATE_RF_TAG_INVALID,{ _T("Command to Tag Failed"),RGB_COLOR_RED } },
		{ STATE_RF_TAG_BEING_PROGRAMED,{ _T("Being Programmed"),RGB_COLOR_YELW } },
		{ STATE_RF_TAG_PROGRAMED_OK,{ _T("New Data"),RGB_COLOR_BLU } },
		{ STATE_RF_TAG_PROGRAMED_FAIL,{ _T("Writing to Tag Failed"),RGB_COLOR_RED } },
		{ STATE_RF_TAG_PROGRAMED_FAIL_LOCKEDTAG,{ _T("Invalid Tag"),RGB_COLOR_RED } },
		{ STATE_RF_TAG_VERIFYING,{ _T("Verifying"),RGB_COLOR_YELW } },
		{ STATE_RF_TAG_VERIFY_OK,{ _T("Verified"),RGB_COLOR_BLU } },
		{ STATE_RF_TAG_VERIFY_FAIL,{ _T("Corrupted Data"),RGB_COLOR_RED } }
	};


public:
	// Construction
	CRFIDProgrammingStationDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CRFIDProgrammingStationDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RFIDPROGRAMMINGSTATION_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

	// Implementation
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	void set_app_config(const char* info_file, const char* field, const char* value, const char* tree = "app", const char *op_p_parent_tree = nullptr) const;
	bool get_config(std::string info_file, StationAppConfig_t& config) const;

	// Initialization Sequences for Hardware and Configuration
	bool InitializeController();

public:
	CComboBox CB_PartDescription_Control;
	CString CB_PartDescription_Value;
	CDateTimeCtrl DT_Expiration_Control;
	COleDateTime DT_Expiration_Value;
	CEdit EDIT_LotNumber_Control;
	CString EDIT_LotNumber_Value;
	CButton BTN_ClearSelection_Control;
	CButton BTN_ApplicationMode_Control;
	CStatic SLBL_Hardware_Control;
	CString SLBL_Hardware_Value;
	CStatic SLBL_Conveyor_Control;
	CString SLBL_Conveyor_Value;
	CStatic SLBL_RFIDReadWrite_Control;
	CString SLBL_RFIDReadWrite_Value;
	CStatic SLBL_Payload_Control;
	CString SLBL_Payload_Value;
	CStatic SLBL_System_Control;
	CString SLBL_System_Value;
	CStatic SLBL_RFID_Overall_Control;
	CString SLBL_RFID_Overall_Value;
	CStatic SLBL_RFID_Numb1_Control;
	CString SLBL_RFID_Numb1_Value;
	CStatic SLBL_RFID_Numb2_Control;
	CString SLBL_RFID_Numb2_Value;
	CStatic SLBL_RFID_Numb3_Control;
	CString SLBL_RFID_Numb3_Value;
	CStatic SLBL_RFID_Numb4_Control;
	CString SLBL_RFID_Numb4_Value;
	CStatic SLBL_RFID_Numb5_Control;
	CString SLBL_RFID_Numb5_Value;
	CStatic SLBL_RFID_Numb6_Control;
	CString SLBL_RFID_Numb6_Value;

	CBrush _brush_status_hardware;
	CBrush _brush_status_conveyor;
	CBrush _brush_status_RFID_readwrite;
	CBrush _brush_status_payload;
	CBrush _brush_status_system;
	CBrush _brush_status_RFID_overall;
	CBrush _brush_status_RFID_numb1;
	CBrush _brush_status_RFID_numb2;
	CBrush _brush_status_RFID_numb3;
	CBrush _brush_status_RFID_numb4;
	CBrush _brush_status_RFID_numb5;
	CBrush _brush_status_RFID_numb6;

	static bool _fw_update_finished;
	static uint32_t _fw_update_progress;
	state_controller_status _state_status_controller;
	state_hw_rfid_status _state_status_hw_rfid;
	state_payload_status _state_status_payload;
	state_system_status _state_status_system;
	state_rf_bank_status _state_status_rf_bank;
	app_mode _application_mode;
	std::array<state_rf_tags_status, TOTAL_SUPPORTED_RF_TAGS> _state_status_rf_tags;
	std::vector<std::pair<CString*, CBrush*>> _state_strings_color_rf_tags;
	std::vector<std::pair<CStatic*, CStatic*>> _state_static_control;

	volatile hw_rf_error _hw_rf_error;
	CString _label_bar_code;
	//bool _rfid_reader_busy;
	bool _button_lotnumber_clicked;
	bool _clear_payload;
	bool _update_required; 
	uint32_t _session_tags_scanned;
	uint32_t _session_tags_successful;
	uint32_t _session_tags_failed;
	uint32_t _write_error_at;
	uint32_t _verify_error_at;
	std::chrono::time_point<std::chrono::system_clock> _time_started_scanning;

	std::map < std::string, std::vector< std::pair<bool, uint16_t> > > session_tag_history; // tags that we've encountered in the current SESSION
	//         TAG SN       History of             P/F   ERRC


	std::string _GTIN;
	std::string _MFG_date;
	std::string _EXP_date;
	std::string _LOT_number;

	//State Machines and support functions
	static UINT update_all_statemachines(LPVOID param);
	void kill_system();
	void statemachine_controller();
	void statemachine_rfid_reader();
	void statemachine_payload();
	void statemachine_system();
	void statemachine_rf_bank();
	
private:
	std::vector<FirmwareBinaryEntry> firmware_bin_entries;
	std::shared_ptr<FirmwareDownload> _pfirmwaredownload;
	std::shared_ptr<FirmwareUpdateProgressDlg> _pfwupdate_progress_dialog;
	std::shared_ptr<TagWriteFailureDlg> _ptag_write_failure;
	std::shared_ptr<TagVerifyFailureDlg> _ptag_verify_failure;
	StationAppConfig_t _config;

	std::unique_ptr<Gpio::GpioWrapper> _gpio_wrapper;

	bool rf_reader_error();
	bool controller_cmd_error();
	static void firmware_update_progress(bool finish, int16_t total);
	bool tag_missing();
	bool is_headerstatus_valid(TagStatus tag_status);

	void on_conveyor_connection_change(bool isConnected);

	BoardStatus get_conntroller_cmd(RegisterIds cmd, uint32_t& data);
	void get_reagent_cmd(ReagentRegisterOffsets cmd, uint32_t& payload, bool block = true);
	void get_reagent_data(ReagentRegisterOffsets cmd, std::vector<uint32_t>& payload, uint32_t offset = 0);
	void set_reagent_cmd(ReagentRegisterOffsets cmd, uint32_t payload);
	void send_reagent_data(ReagentRegisterOffsets cmd, std::vector<uint32_t>& payload, uint32_t offset = 0);
	void send_reagent_cmd(ReagentCommandRegisterCodes subcmd, uint32_t payload, bool block = true);
	void read_reagent_cmd(ReagentCommandRegisterCodes subcmd, uint32_t& payload, size_t sizepayload);

	static void change_color(CBrush& brush, COLORREF color);
	void change_tag_state(CRFIDProgrammingStationDlg::state_rf_tags_status to, int i);
	void change_tags_state_all(state_rf_tags_status to, uint32_t total);
	void change_statemachine_state(current_statemachines sm, uint32_t state);
	void show_pack_label(uint32_t& current_tag, BOOL mode);
	void get_payload_data(std::vector<uint32_t>& data);

	template <typename T>
    static void change_status_label(T &current_state, T &prev_state, CString &label, CBrush &bush, 
                                    const std::map<T, std::pair< CString, COLORREF>> &map, 
                                    bool &update_required);

	// Infrastructure for ASIO
	std::shared_ptr<boost::asio::io_context> _io_service; 
	std::shared_ptr<HawkeyeServices> _hs;
    std::shared_ptr<boost::asio::io_context::work> _plocal_work;
	std::shared_ptr<std::thread> _pio_thread;

	std::shared_ptr<ControllerBoardOperation> _pcontroller_board;
	std::shared_ptr<ControllerBoardCommand> _pcontroller_board_command;

	//Worker Thread
	typedef struct 
	{
		CRFIDProgrammingStationDlg*    dlg_instance;
	} THREADSTRUCT;
	THREADSTRUCT _thread_param;
	bool _stop_thread;
	CCriticalSection _critsection_dataupdate;
	boost::condition_variable _thread_stop_cond;
	
public:
	//{{AFX_MSG(CRFIDProgrammingStationDlg)
	afx_msg LRESULT OnModelDataUpdate(WPARAM wparam, LPARAM);
	afx_msg LRESULT OnUpdate_Label_Data(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnDisplayWriteErrDlg(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnDisplayVerifyErrDlg(WPARAM, LPARAM);
	afx_msg LRESULT OnStopScanning(WPARAM, LPARAM);
	//}}AFX_MSG
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	void clear_payload_information();
	void update_data_safe(BOOL mode);


	afx_msg void OnCbnSelchangeComboPartDescription();
	afx_msg void OnBnClickedBtnClearselection();
	afx_msg void OnDtnDatetimechangeDatetimeExpiration(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEditLotnumber();
	afx_msg void OnEnSetfocusEditLotnumber();
	afx_msg void OnBnClickedBtnAppMode();
	afx_msg void OnBnClickedBtnImportPayloadFiles();
	
	void export_log_files(const std::string& cs);
	bool backup_payload_files(boost::filesystem::path& homepath, boost::system::error_code& ec, bool remove_after_backup = true);
	bool is_result_error(boost::system::error_code ec) const;
	void import_payload_files(const std::string& cs, bool replace_payload);
	void OnBnClickedBtnExportLog();
	
	CMFCEditBrowseCtrl EBWS_ExportLogPath_Control;
	CMFCEditBrowseCtrl EBWS_ImportPayloadPath_Control;
	CString EBWS_ExportLogPath_Value;
	CString EBWS_ImportPayloadPath_Value;
	CStatic SLBL_PackLabel1_Control;
	CStatic SLBL_PackLabel2_Control;
	CStatic SLBL_PackLabel3_Control;
	CStatic SLBL_PackLabel4_Control;
	CStatic SLBL_PackLabel5_Control;
	CStatic SLBL_PackLabel6_Control;
	CString SLBL_TotalTags_Value;	
	BOOL Checkbox_PayloadReplaceOption;
	CButton CHECK_ReplacePayload;
};

template <typename T>
void CRFIDProgrammingStationDlg::change_status_label(T &current_state, T &prev_state, CString &label, CBrush &bush, const std::map<T, std::pair< CString, COLORREF>> &map, bool &update_required)
{
	if(map.find(current_state) == map.end())
	{
		label = _T("State Not Defined");
		change_color(bush, RGB_COLOR_RED);
		return;
	}
	
	if (current_state != prev_state)
	{
		update_required = true;
		label = map.find(current_state)->second.first;
		change_color(bush, map.find(current_state)->second.second);
	}
}
