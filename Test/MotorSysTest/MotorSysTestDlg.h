
// MotorSysTestDlg.h : header file
//

#pragma once

#include <stdint.h>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/thread.hpp>

#include <iostream>
#include <fstream>

#include <string>

#include "CarouselController.hpp"
#include "Configuration.hpp"
#include "ControllerBoardInterface.hpp"
#include "FocusController.hpp"
#include "LEDRackController.hpp"
#include "MotorBase.hpp"
#include "PlateController.hpp"
#include "ReagentController.hpp"


#include "afxwin.h"
#include "afxcmn.h"


const int32_t SignalsInterval = 250;                    // milliseconds for background timer update operation

// MotorSysTestDlg dialog
class MotorSysTestDlg : public CDialogEx
{
    static const std::string CmdSimModeStr;             // toggle script execution simulation mode (no hardware actions)
    static const std::string CmdInitPlateStr;           // Initialize Plate: home both theta and radius plate motors
    static const std::string CmdInitCarouselStr;        // Initialize Carousel: home the carousel; may not be the tube 1 position
    static const std::string CmdSelectPlateStr;         // Select Plate: initialize and apply plate motor control parameters for all following commands
    static const std::string CmdSelectCarouselStr;      // Select Carousel: initialize and apply carousel motor control parameters for all following commands
    static const std::string CmdHomeThetaStr;           // Home the Theta motor
    static const std::string CmdHomeRadiusStr;          // Home the Radius motor
    static const std::string CmdMoveThetaStr;           // move the Theta motor to the specified absolute location
    static const std::string CmdMoveThetaRelStr;        // move the Theta motor to the specified relative location
    static const std::string CmdMoveRadiusStr;          // move the Radius motor to the specified absolute location
    static const std::string CmdMoveRadiusRelStr;       // move the Radius motor to the specified relative location
    static const std::string CmdMoveRThetaStr;          // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
    static const std::string CmdMoveRThetaRelStr;       // move the Theta and Radius motors to the relative theta (p1) / radius (p2) coordinate pair
    static const std::string CmdMoveCarouselStr;        // move carousel to tube number: '0' for next tube, 1-24 for discreet tube numbers
    static const std::string CmdMovePlateWellStr;       // move the plate to the specified row (p1) / col (p2) location
    static const std::string CmdMovePlateStr;           // Move plate to absolute radius, theta position
    static const std::string CmdMovePlateRelStr;        // Move plate to relative radius, theta position change values
    static const std::string CmdProbeHomeStr;           // home the probe
    static const std::string CmdProbeMoveStr;           // move the probe: '0' for 'Up', and '1' for 'down'
    static const std::string CmdWaitForKeyPressStr;     // Wait for a keypress.
    static const std::string CmdSleepStr;               // insert a delay into the command stream processing
    static const std::string CmdExitStr;                // Stop processing the run list and exit the program.

// Construction
public:

    enum InstrumentTypes
    {
        InstrumentTypeUnknown = 0,
        InstrumentTypeScout = 1,
        InstrumentTypeHunter = 2,
    };

    MotorSysTestDlg(CWnd* pParent = NULL);	// standard constructor
    ~MotorSysTestDlg();

    void Quit();

    void SetEditingMode( bool allow );
    void SetSimMode( bool allow );
    void SetInstrumentType( MotorSysTestDlg::InstrumentTypes type );

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MOTOR_SYS_TEST_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

private:
    const int32_t DefaultPositionTolerance = 1000;      // allow 0.1 mm of inaccuracy?

    enum MsgBoxTypes
    {
        MsgBoxTypeNone = 0,
        MsgBoxTypeThetaZero,
        MsgBoxTypeRadiusZero,
        MsgBoxTypeThetaCal,
        MsgBoxTypeRadiusCal,
        MsgBoxTypeCalibrateCarousel,
        MsgBoxTypeCalibratePlate,
        MsgBoxTypeCalibrateRack1,
        MsgBoxTypeCalibrateRack2,
    };

    void signalHandler( const boost::system::error_code& ec, int signal_number );

    std::shared_ptr<boost::asio::signal_set>        pSystemSignals_;
    std::shared_ptr<boost::asio::io_context>        pLocalIosvc_;
    std::shared_ptr<boost::asio::io_context::work>  pLocalWork_;
    std::shared_ptr<std::thread>                    pIoThread_;
    std::shared_ptr<ControllerBoardInterface>       pCbi_;
    std::shared_ptr<CarouselController>             pCarouselController;
    std::shared_ptr<FocusController>                pFocusController;
    std::shared_ptr<LEDRackController>              pLedRackController;
    std::shared_ptr<PlateController>                pPlateController;
    std::shared_ptr<ReagentController>              pReagentController;

    std::shared_ptr<boost::asio::deadline_timer>    pUpdTimer;
    boost::system::error_code                       updTimerError;

    std::ifstream listFile;
    std::ofstream saveFile;

    bool        inited_;
    bool        cbiInit;
    bool        quitting;
    bool        timerComplete;
    bool        allowEditing;
    bool        simMode;
    bool        infoInit;
    bool        calInit;
    bool        doRelativeMove;
    bool        showInitErrors;

    bool        signalUpdateReady;
    bool        signalUpdateStarting;
    bool        signalUpdateRunning;
    std::shared_ptr<std::thread>    pSignalsThread;

    MotorSysTestDlg::InstrumentTypes    instrumentType;

    t_pPTree    dlgfilecfg;
    t_opPTree   dlgconfig;
    t_opPTree   dlgCfgNode;
    t_pPTree    ptfilecfg;
    t_opPTree   ptcfg;
    t_opPTree   carouselControllerCfgNode;
    t_opPTree   focusControllerCfgNode;
    t_opPTree   ledRackControllerCfgNode;
    t_opPTree   plateControllerCfgNode;
    t_opPTree   reagentControllerCfgNode;

    t_pPTree    calfilecfg;
    t_opPTree   calcfg;
    t_opPTree   controllerCalNode;
    t_opPTree   carouselControllerCalNode;
    t_opPTree   plateControllerCalNode;

    std::string moduleConfigFile;
    std::string motorConfigFile;
    std::string calConfigFile;

    CString     ScriptEditBuf;
    CString     ScriptPath;

    bool        cntlEdit;
    BOOL        doorClosed;
    BOOL        packPresent;
    BOOL        reagentHome;
    BOOL        reagentLimit;
    BOOL        thetaHome;
    BOOL        radiusHome;
    BOOL        probeHome;
    BOOL        focusHome;
    BOOL        rack1Home;
    BOOL        rack2Home;
    BOOL        plateFound;
    BOOL        tubePresent;

    int32_t     thetaEditVal;
    int32_t     thetaVal;
    int32_t     radiusEditVal;
    int32_t     radiusVal;
    int32_t     focusEditVal;
    int32_t     focusVal;
    int32_t     rack1Pos;
    CString     rack1PathStr;
    int32_t     rack2Pos;
    CString     rack2PathStr;
    CString     rowEditStr;
    int32_t     rowEditVal;
    CString     rowCharStr;
    int32_t     rowVal;
    int32_t     colEditVal;
    int32_t     colVal;
    int32_t     tubeEditVal;
    int32_t     tubeNum;
    int32_t     probeVal;

    int32_t     scrollIdx;
    int32_t     cmdStepLine;
    int32_t     scriptLines;
    bool        stopRun;
    bool        scriptStarting;
    bool        scriptRunning;
    bool        scriptPaused;
    long        errorCnt;

    std::shared_ptr<std::thread>    pScriptThread;

    BOOL        plateSelected;
    bool        plateControlsInited;
    bool        plateBacklashChanged;
    int32_t     maxPlateThetaPos;
    int32_t     platePositionTolerance;     // to allow external adjustment
    int32_t     plateThetaCalPos;           // the theta angle for calibration
    int32_t     plateThetaHomePosOffset;    // the position offset correction value associated with the Theta home position calibration
    int32_t     plateThetaA1Pos;            // the base position value for the Theta A-1 well position
    int32_t     plateThetaA1PosOffset;      // the position offset correction value for the Theta A-1 well position calibration
    int32_t     plateRadiusCenterPos;       // the center of the plate calibration position for conversion calculations; NOT a zero value
    int32_t     plateRadiusHomePosOffset;   // the position offset correction value associated with the Radius home position calibration
    int32_t     plateRadiusBacklash;        // the compensation for backlash when changing movement direction
    int32_t     plateThetaBacklash;         // the compensation for backlash when changing movement direction
    int32_t     plateRadiusA1Pos;           // the base position value for the Radius A-1 well position
    int32_t     plateRadiusA1PosOffset;     // the position offset correction for the Radius A-1 well position calibration
    int32_t     plateRadiusMaxTravel;       // the maximum allowable travel for the Radius motor
    int32_t     plateThetaStartTimeout;
    int32_t     plateThetaFullTimeout;
    int32_t     plateRadiusStartTimeout;
    int32_t     plateRadiusFullTimeout;

    int32_t     plateProbePositionTolerance;
    int32_t     plateProbeHomePos;
    int32_t     plateProbeHomePosOffset;
    int32_t     plateProbeStopPos;
    int32_t     plateProbeMaxTravel;
    int32_t     plateProbeStartTimeout;
    int32_t     plateProbeBusyTimeout;

    BOOL        carouselSelected;
    bool        carouselControlsInited;
    BOOL        carouselPresent;
    bool        carouselBacklashChanged;
    int32_t     maxCarouselThetaPos;
    int32_t     carouselPositionTolerance;  // to allow external adjustment
    int32_t     carouselThetaHomePos;       // the theta angle for calibration
    int32_t     carouselThetaHomePosOffset; // the position offset correction value associated with the Theta home position calibration
    int32_t     carouselThetaBacklash;      // the compensation for backlash when changing rotation direction
    int32_t     maxCarouselTubes;           // maximum number of tubes the carousel can hold
    int32_t     carouselRadiusOffset;       // the position of the radius at the tube center
    int32_t     carouselRadiusMaxTravel;    // the maximum allowable travel for the Radius motor
    int32_t     carouselThetaStartTimeout;
    int32_t     carouselThetaFullTimeout;
    int32_t     carouselRadiusStartTimeout;
    int32_t     carouselRadiusFullTimeout;

    int32_t     carouselProbePositionTolerance;
    int32_t     carouselProbeHomePos;
    int32_t     carouselProbeHomePosOffset;
    int32_t     carouselProbeStopPos;
    int32_t     carouselProbeMaxTravel;
    int32_t     carouselProbeStartTimeout;
    int32_t     carouselProbeBusyTimeout;

    int32_t     focusPositionTolerance;
    int32_t     focusMaxPos;
    int32_t     focusStartPos;
    int32_t     focusCoarseStepValue;
    int32_t     focusFineStepValue;
    int32_t     focusMaxTravel;
    int32_t     focusStartTimeout;
    int32_t     focusBusyTimeout;

    int32_t     rackPositionTolerance;
    int32_t     rack1HomePosOffset;
    int32_t     rack2HomePosOffset;
    int32_t     rack1BottomPos;
    int32_t     rack2BottomPos;
    int32_t     rack1MiddlePos;
    int32_t     rack2MiddlePos;
    int32_t     rack1TopPos;
    int32_t     rack2TopPos;
    int32_t     rackStartTimeout;
    int32_t     rackFullTimeout;
    int32_t     maxRackPosition;

    int32_t     armPositionTolerance;
    int32_t     armHomePos;
    int32_t     armHomeOffset;
    int32_t     armDownPos;
    int32_t     armDownOffset;
    int32_t     armMaxTravel;
    int32_t     armPurgePosition;
    int32_t     reagentStartTimeout;
    int32_t     reagentBusyTimeout;

    MotorBase * pCarouselTheta;
    MotorBase * pCarouselRadius;
    MotorBase * pCarouselProbe;
    MotorBase * pPlateTheta;
    MotorBase * pPlateRadius;
    MotorBase * pPlateProbe;
    MotorBase * pFocus;
    MotorBase * pRack1;
    MotorBase * pRack2;
    MotorBase * pReagent;

    MsgBoxTypes msgType;
    bool        dlgStarting;
    bool        dlgRunning;

    CFont       scriptEditFont;

    std::shared_ptr<std::thread>    pDlgThread;

    bool        InitDlgInfoFile( void );
    void        ConfigDlgVariables( void );
    bool        InitControllers();
    void        InitControllerCfgInfo();
    void        InitControllerCalInfo();
    bool        InitLedController( void );
    bool        InitInfoFile( t_pPTree & tfilecfg, t_opPTree & cfg, bool docalfile = false );
    void        ConfigCarouselVariables( void );
    void        ConfigCarouselProbeVariables( void );
    void        ConfigCarouselCalVariables( void );
    void        ConfigFocusVariables( void );
    void        ConfigLedRackVariables( void );
    void        ConfigPlateVariables( void );
    void        ConfigPlateProbeVariables( void );
    void        ConfigPlateCalVariables( void );
    void        ConfigReagentVariables( void );
    void        SetAllControlEnable( BOOL enable );
    void        SetSamplerControlEnable( BOOL action );
    void        SetLedControlEnable( BOOL action );
    void        SetPlateControlEnable( BOOL enable );
    void        SetCarouselControlEnable( BOOL enable );
    void        SetMsgBoxControlEnable( bool enable, MsgBoxTypes msg_type );
    void        UpdateSignalStatus( const boost::system::error_code error );
    void        UpdateSignals( void );
    void        StartSignalsUpdateThread( void );
    void        ShowSignals( void );
    void        ShowReagentPresent( BOOL installed );
    void        ShowDoorState( BOOL door_closed, BOOL arm_home );
    void        ShowHomeStates( void );
    void        UpdateProbePosition( void );
    void        GetProbePosition( int32_t probe_pos );
    void        ShowProbePosition( int32_t probe_pos );
    void        UpdateFocusPosition( void );
    void        ClearFocusPosition( void );
    void        ShowFocusPosition( int32_t pos );
    void        ShowInstrumentType( MotorSysTestDlg::InstrumentTypes type );
    void        UpdateRackPositions( void );
    void        UpdateStep( int stepIdx );
    void        ShowStepNum( int lineNum );
    void        UpdateRThetaPositions( void );
    void        GetRThetaPositions( int32_t & tPos, int32_t & rPos );
    void        ShowRThetaPositions( int32_t tPos, int32_t rPos );
    void        UpdateRowColPositions( void );
    void        GetRowColPositions( int32_t & row, int32_t & col );
    void        ShowRowColPositions( int32_t row, int32_t col );
    void        UpdateTubeNum( void );
    void        GetTubeNum( int32_t & tube );
    void        ShowTubeNum( int32_t tube_num );
	void        ShowPlateDetected( BOOL found );
    void        ShowCarouselPresent( BOOL present );
    void        ShowTubePresent( BOOL present );
    bool        OpenListFile( std::string filename );
    bool        OpenSaveFile( std::string filename );
    int32_t     ReadListFile( std::string& cmdBlock );
    int32_t     WriteListFile( void );
    void        StartScript( void );
    void        RunScript( void );
    void        ProcessScriptList( int32_t lineCnt );
    int32_t     DoScriptStep( int32_t stepIdx, int32_t lineCnt );
    bool        ParseScriptLine( std::string cmdLine, std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3 );
    bool        RunScriptCmd( std::string cmd, uint32_t param1, uint32_t param2, uint32_t param3 );
    void        ScriptSleep( int32_t sleepTime );
    void        DoPlateSelect( bool msgWait );
    void        DoCarouselSelect( bool msgWait );
    bool        DoGotoThetaPos( bool relativeMove = false );
    bool        DoGotoRadiusPos( bool relativeMove = false );
    bool        DoGotoRThetaPos( bool relativeMove = false );
    void        DoGotoTubeBtn( bool msgWait );
    void        DoPlateInit( bool msgWait );
	void        DoPlateDetect( bool msgWait );
    void        DoCarouselInit( bool msgWait );
    void        StartInfoDialog( void );
    void        ShowInfoDialog( void );


    CEdit           ScriptEdit;

    CButton         ReagentArmUpBtn;
    CButton         ReagentArmDownBtn;
    CButton         ReagentArmPurgeBtn;
    CButton         DoorClosedChk;
    CButton         PackDetectedChk;
    CButton         ReagentDoorReleaseBtn;
    CButton         ReagentArmHomeChk;
    CButton         ReagentArmLimitChk;

    CEdit           CurrentThetaDisp;
    CEdit           ThetaPosEdit;
    CButton         GotoThetaBtn;
    CButton         ThetaHomeChk;
    CButton         HomeThetaBtn;

    CEdit           CurrentRadiusDisp;
    CEdit           RadiusPosEdit;
    CButton         GotoRadiusBtn;
    CButton         RadiusHomeChk;
    CButton         HomeRadiusBtn;
    CButton         GotoRThetaBtn;

    CButton         RelativeMoveChk;

    CButton         ProbeUpBtn;
    CButton         ProbeDownBtn;
    CEdit           CurrentProbeDisp;
    CButton         InitProbeBtn;
    CButton         HomeProbeBtn;
    CButton         ProbeHomeChk;

    CButton         FineFocusUpBtn;
    CButton         CoarseFocusUpBtn;
    CEdit           CurrentFocusDisp;
    CEdit           FocusPosEdit;
    CButton         GotoFocusPosBtn;
    CButton         FineFocusDownBtn;
    CButton         CoarseFocusDownBtn;
    CButton         FocusHomeChk;
    CButton         HomeFocusBtn;
    CButton         CenterFocusPosBtn;

    CButton         InstrumentTypeChk;

    CStatic         LedRack1Lbl;
    CEdit           Rack1PathDisp;
    CButton         Rack1TopBtn;
    CButton         Rack1TopCalBtn;
    CButton         Rack1CtrBtn;
    CButton         Rack1CtrCalBtn;
    CButton         Rack1BotBtn;
    CButton         Rack1BotCalBtn;
    CButton         Rack1HomeBtn;
    CButton         Rack1HomeChk;
    CStatic         LedRack2Lbl;
    CEdit           Rack2PathDisp;
    CButton         Rack2TopBtn;
    CButton         Rack2TopCalBtn;
    CButton         Rack2CtrBtn;
    CButton         Rack2CtrCalBtn;
    CButton         Rack2BotBtn;
    CButton         Rack2BotCalBtn;
    CButton         Rack2HomeBtn;
    CButton         Rack2HomeChk;

    CEdit           CurrentRowDisp;
    CEdit           RowValEdit;
    CButton         GotoRowColBtn;
    CEdit           CurrentColDisp;
    CEdit           ColValEdit;
    CButton         NextRowColBtn;

    CEdit           CurrentTubeDisp;
    CEdit           TubeNumEdit;
    CButton         GotoTubeNumBtn;
    CButton         TubePresentChk;
    CButton         NextTubeBtn;
    CButton         FindTubeBtn;

    CButton         UpdateBtn;

    CButton         PlateSelectRadio;
    CButton         PlateInitErrorChk;
	CButton         PlateDetectedChk;
    CButton         CalibratePlateBtn;
    CButton         EjectPlateBtn;
    CButton         LoadPlateBtn;
    CButton         InitPlateBtn;
	CButton         DetectPlateBtn;
    CButton         PlateThetaAdjustLeftBtn;
    CButton         PlateThetaAdjustRightBtn;
    CButton         PlateRadiusAdjustInBtn;
    CButton         PlateRadiusAdjustOutBtn;
    CEdit           PlateRadiusBacklashEdit;
    CButton         PlateRadiusBacklashSetBtn;
    CEdit           PlateThetaBacklashEdit;
    CButton         PlateThetaBacklashSetBtn;

    CButton         CarouselSelectRadio;
    CButton         CarouselInitErrorChk;
    CButton         CarouselPresentChk;
    CButton         CalibrateCarouselBtn;
    CButton         EjectCarouselBtn;
    CButton         LoadCarouselBtn;
    CButton         InitCarouselBtn;
    CButton         CarouselThetaAdjustLeftBtn;
    CButton         CarouselThetaAdjustRightBtn;
    CButton         CarouselRadiusAdjustInBtn;
    CButton         CarouselRadiusAdjustOutBtn;
    CEdit           CarouselThetaBacklashEdit;
    CButton         CarouselThetaBacklashSetBtn;

    CButton         RunScriptBtn;
    CEdit           StepNumDisp;
    CSpinButtonCtrl StepNumSpin;
    CStatic         ErrorCntLbl;
    CEdit           ErrorCntDisp;
    CButton         PauseScriptBtn;
    CButton         StopScriptBtn;

    CEdit           ScriptPathEdit;
    CButton         LoadScriptBtn;
    CButton         ClearScriptBtn;
    CButton         SaveScriptBtn;


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:

    afx_msg void OnBnClickedReagentArmUpBtn();
    afx_msg void OnBnClickedReagentArmDnBtn();
    afx_msg void OnBnClickedReagentArmPurgeBtn();
    afx_msg void OnBnClickedUnlockDoorBtn();

    afx_msg void OnEnChangeThetaPosEdit();
    afx_msg void OnBnClickedGotoThetaPosBtn();
    afx_msg void OnBnClickedHomeThetaBtn();

    afx_msg void OnEnChangeRadiusPosEdit();
    afx_msg void OnBnClickedGotoRadiusPosBtn();
    afx_msg void OnBnClickedHomeRadiusBtn();
    afx_msg void OnBnClickedGotoRThetaPosBtn();

    afx_msg void OnBnClickedRelativeMoveChk();

    afx_msg void OnBnClickedProbeUpBtn();
    afx_msg void OnBnClickedProbeDownBtn();
    afx_msg void OnBnClickedProbeInitBtn();
    afx_msg void OnBnClickedProbeHomeBtn();

    afx_msg void OnBnClickedFineFocusUpBtn();
    afx_msg void OnBnClickedFineFocusDnBtn();
    afx_msg void OnBnClickedCoarseFocusUpBtn();
    afx_msg void OnBnClickedCoarseFocusDnBtn();
    afx_msg void OnEnChangeFocusPosEdit();
    afx_msg void OnBnClickedFocusGotoPosBtn();
    afx_msg void OnBnClickedFocusHomeBtn();
    afx_msg void OnBnClickedFocusGotoCenterBtn();

    afx_msg void OnBnClickedInstTypeChk();
    afx_msg void OnBnClickedLedRack1GotoTopBtn();
    afx_msg void OnBnClickedLedRack1CalTopBtn();
    afx_msg void OnBnClickedLedRack1GotoCtrBtn();
    afx_msg void OnBnClickedLedRack1CalCtrBtn();
    afx_msg void OnBnClickedLedRack1GotoBotBtn();
    afx_msg void OnBnClickedLedRack1CalBotBtn();
    afx_msg void OnBnClickedLedRack1HomeBtn();
    afx_msg void OnBnClickedLedRack2GotoTopBtn();
    afx_msg void OnBnClickedLedRack2CalTopBtn();
    afx_msg void OnBnClickedLedRack2GotoCtrBtn();
    afx_msg void OnBnClickedLedRack2CalCtrBtn();
    afx_msg void OnBnClickedLedRack2GotoBotBtn();
    afx_msg void OnBnClickedLedRack2CalBotBtn();
    afx_msg void OnBnClickedLedRack2HomeBtn();

    afx_msg void OnEnChangeRowEdit();
    afx_msg void OnEnChangeColEdit();
    afx_msg void OnBnClickedGotoRowColBtn();
    afx_msg void OnBnClickedNextRowColBtn();
    afx_msg void OnEnChangeTubeNumEdit();
    afx_msg void OnBnClickedFindTubeBtn();
    afx_msg void OnBnClickedGotoTubeBtn();
    afx_msg void OnBnClickedNextTubeBtn();

    afx_msg void OnBnClickedUpdateCurrentBtn();

    afx_msg void OnBnClickedRadioPlateSelect();
    afx_msg void OnBnClickedPlateCalBtn();
    afx_msg void OnBnClickedPlateEjectBtn();
    afx_msg void OnBnClickedPlateLoadBtn();
    afx_msg void OnBnClickedPlateInitBtn();
	afx_msg void OnBnClickedPlateDetectBtn();
    afx_msg void OnBnClickedPlateThetaAdjustLeftBtn();
    afx_msg void OnBnClickedPlateThetaAdjustRightBtn();
    afx_msg void OnBnClickedPlateRadiusAdjustInBtn();
    afx_msg void OnBnClickedPlateRadiusAdjustOutBtn();
    afx_msg void OnEnChangePlateRadiusBacklashEdit();
    afx_msg void OnBnClickedPlateRadiusBacklashSetBtn();
    afx_msg void OnEnChangePlateThetaBacklashEdit();
    afx_msg void OnBnClickedPlateThetaBacklashSetBtn();

    afx_msg void OnBnClickedRadioCarouselSelect();
    afx_msg void OnBnClickedCarouselCalBtn();
    afx_msg void OnBnClickedCarouselEjectBtn();
    afx_msg void OnBnClickedCarouselLoadBtn();
    afx_msg void OnBnClickedCarouselInitBtn();
    afx_msg void OnBnClickedCarouselThetaAdjustLeftBtn();
    afx_msg void OnBnClickedCarouselRadiusAdjustInBtn();
    afx_msg void OnBnClickedCarouselThetaAdjustRightBtn();
    afx_msg void OnBnClickedCarouselRadiusAdjustOutBtn();
    afx_msg void OnEnChangeCarouselThetaBacklashEdit();
    afx_msg void OnBnClickedCarouselThetaBacklashSetBtn();

    afx_msg void OnBnClickedRunScriptBtn();
    afx_msg void OnEnVscrollScriptEdit();
    afx_msg void OnEnChangeStepNumDisp();
    afx_msg void OnBnClickedPauseScriptBtn();
    afx_msg void OnBnClickedStopScriptBtn();
    afx_msg void OnEnChangeScriptPathEdit();
    afx_msg void OnBnClickedLoadScriptBtn();
    afx_msg void OnBnClickedSaveScriptBtn();
    afx_msg void OnBnClickedClearScriptBtn();
    afx_msg void OnBnClickedExit();
};