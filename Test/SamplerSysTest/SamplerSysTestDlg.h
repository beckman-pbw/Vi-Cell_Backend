
// SamplerSysTestDlg.h : header file
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
#include "MotorBase.hpp"
#include "PlateController.hpp"


#include "afxwin.h"
#include "afxcmn.h"


const int32_t SignalsInterval = 250;                    // milliseconds for background timer update operation

// SamplerSysTestDlg dialog
class SamplerSysTestDlg : public CDialogEx
{
    static const std::string CmdSimModeStr;             // toggle script execution simulation mode (no hardware actions)
    static const std::string CmdInitPlateStr;           // Initialize Plate: home both theta and radius plate motors
    static const std::string CmdInitCarouselStr;        // Initialize Carousel: home the carousel; may not be the tube 1 position
    static const std::string CmdSelectPlateStr;         // Select Plate: initialize and apply plate motor control parameters for all following commands
    static const std::string CmdSelectCarouselStr;      // Select Carousel: initialize and apply carousel motor control parameters for all following commands
    static const std::string CmdHomeThetaStr;           // Home the Theta motor
    static const std::string CmdZeroThetaStr;           // Set the current position as the Theta zero point
    static const std::string CmdThetaToCalStr;          // move the Theta motor to the cal position
    static const std::string CmdHomeRadiusStr;          // Home the Radius motor
    static const std::string CmdZeroRadiusStr;          // Set the current position as the Radius zero point
    static const std::string CmdRadiusToCalStr;         // move the Radius motor to the center calibration position
    static const std::string CmdMoveThetaStr;           // move the Theta motor to the specified absolute location
    static const std::string CmdMoveThetaRelStr;        // move the Theta motor to the specified relative location
    static const std::string CmdMoveRadiusStr;          // move the Radius motor to the specified absolute location
    static const std::string CmdMoveRadiusRelStr;       // move the Radius motor to the specified relative location
    static const std::string CmdMoveRThetaStr;          // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
    static const std::string CmdMoveRThetaRelStr;       // move the Theta and Radius motors to the relative theta (p1) / radius (p2) coordinate pair
    static const std::string CmdCalPlateThetaStr;       // set the plate Theta cal1 position to the current position
    static const std::string CmdCalPlateRadiusStr;      // set the plate Radius cal1 position to the current location
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
	SamplerSysTestDlg(CWnd* pParent = NULL);	// standard constructor
    ~SamplerSysTestDlg();

    void Quit();

    void SetEditingMode( bool allow );
    void SetSimMode( bool allow );

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SAMPLER_SYS_TEST_DIALOG };
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
    };

    void signalHandler( const boost::system::error_code& ec, int signal_number );

    std::shared_ptr<boost::asio::signal_set>        pSystemSignals_;
    std::shared_ptr<boost::asio::io_context>        pLocalIosvc_;
    std::shared_ptr<boost::asio::io_context::work>  pLocalWork_;
    std::shared_ptr<std::thread>                    pIoThread_;
    std::shared_ptr<ControllerBoardInterface>       pCbi_;
    std::shared_ptr<CarouselController>             pCarouselController;
    std::shared_ptr<PlateController>                pPlateController;

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
    bool        doRelativeMove;
    bool        showInitErrors;

    bool        signalUpdateReady;
    bool        signalUpdateStarting;
    bool        signalUpdateRunning;
    std::shared_ptr<std::thread>    pSignalsThread;

    t_pPTree    dlgfilecfg;
    t_opPTree   dlgconfig;
    t_opPTree   dlgCfgNode;
    t_pPTree    ptfilecfg;
    t_opPTree   ptconfig;
    t_opPTree   carouselControllerCfgNode;
    t_opPTree   plateControllerCfgNode;

    std::string moduleConfigFile;
    std::string motorConfigFile;

    CString     ScriptEditBuf;
    CString     ScriptPath;

    bool        cntlEdit;
    BOOL        thetaHome;
    BOOL        radiusHome;
    BOOL        probeHome;
    BOOL        tubePresent;
    CString     rowEditStr;
    int32_t     rowEditVal;
    CString     rowCharStr;
    int32_t     rowVal;
    int32_t     colEditVal;
    int32_t     colVal;
    int32_t     thetaEditVal;
    int32_t     thetaVal;
    int32_t     thetaRawVal;
    int32_t     radiusEditVal;
    int32_t     radiusVal;
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


    MotorBase * pCarouselTheta;
    MotorBase * pCarouselRadius;
    MotorBase * pCarouselProbe;
    MotorBase * pPlateTheta;
    MotorBase * pPlateRadius;
    MotorBase * pPlateProbe;

    MsgBoxTypes msgType;
    bool        dlgStarting;
    bool        dlgRunning;

    CFont       scriptEditFont;

    std::shared_ptr<std::thread>    pDlgThread;

    bool        InitDlgInfoFile( void );
    void        ConfigDlgVariables( void );
    bool        InitControllers();
    bool        InitMotorInfoFile( void );
    void        ConfigCarouselVariables( void );
    void        ConfigCarouselProbeVariables( void );
    void        ConfigPlateVariables( void );
    void        ConfigPlateProbeVariables( void );
    void        SetAllControlEnable( BOOL enable );
    void        SetSamplerControlEnable( BOOL action );
    void        SetPlateControlEnable( BOOL enable );
    void        SetCarouselControlEnable( BOOL enable );
    void        SetMsgBoxControlEnable( bool enable, MsgBoxTypes msg_type );
    void        UpdateSignalStatus( const boost::system::error_code error );
    void        UpdateSignals( void );
    void        StartSignalsUpdateThread( void );
    void        ShowSignals( void );
    void        ShowHomeStates( void );
    void        UpdateProbePosition( void );
    void        GetProbePosition( int32_t probe_pos );
    void        ShowProbePosition( int32_t probe_pos );
    void        UpdateStep( int stepIdx );
    void        ShowStepNum( int lineNum );
    void        UpdateRThetaPositions( void );
    void        GetRThetaPositions( int32_t & tPos, int32_t & raw_tPos, int32_t & rPos );
    void        ShowRThetaPositions( int32_t tPos, int32_t raw_tPos, int32_t rPos );
    void        UpdateRowColPositions( void );
    void        GetRowColPositions( int32_t & row, int32_t & col );
    void        ShowRowColPositions( int32_t row, int32_t col );
    void        UpdateTubeNum( void );
    void        GetTubeNum( int32_t & tube );
    void        ShowTubeNum( int32_t tube_num );
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
    void        DoCarouselInit( bool msgWait );
    void        StartInfoDialog( void );
    void        ShowInfoDialog( void );


    CEdit           ScriptEdit;

    CButton         ThetaHomeChk;
    CButton         HomeThetaBtn;
    CButton         SetThetaZeroBtn;
    CButton         SetThetaCalBtn;
    CButton         RadiusHomeChk;
    CButton         HomeRadiusBtn;
    CButton         SetRadiusZeroBtn;
    CButton         SetRadiusCalBtn;

    CEdit           RowValEdit;
    CEdit           CurrentRowDisp;
    CButton         GotoRowColBtn;
    CEdit           ColValEdit;
    CEdit           CurrentColDisp;
    CButton         NextRowColBtn;

    CEdit           ThetaPosEdit;
    CEdit           CurrentThetaDisp;
    CButton         GotoThetaBtn;
    CEdit           RawThetaDisp;
    CEdit           RadiusPosEdit;
    CEdit           CurrentRadiusDisp;
    CButton         GotoRadiusBtn;
    CButton         GotoRThetaBtn;

    CButton         RelativeMoveChk;

    CEdit           TubeNumEdit;
    CEdit           CurrentTubeDisp;
    CButton         GotoTubeNumBtn;
    CButton         TubePresentChk;
    CButton         FindTubeBtn;
    CButton         NextTubeBtn;

    CEdit           CurrentProbeDisp;
    CButton         InitProbeBtn;
    CButton         ProbeUpBtn;
    CButton         ProbeHomeChk;
    CButton         HomeProbeBtn;
    CButton         ProbeDownBtn;

    CButton         UpdateBtn;

    CButton         PlateSelectRadio;
    CButton         PlateInitErrorChk;
    CButton         CalibratePlateBtn;
    CButton         EjectPlateBtn;
    CButton         LoadPlateBtn;
    CButton         InitPlateBtn;
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
    CButton         StepScriptBtn;
    CEdit           CmdStepEdit;
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

    afx_msg void OnBnClickedHomeThetaBtn();
    afx_msg void OnBnClickedHomeRadiusBtn();
    afx_msg void OnBnClickedZeroThetaBtn();
    afx_msg void OnBnClickedZeroRadiusBtn();
    afx_msg void OnBnClickedSetThetaCalBtn();
    afx_msg void OnBnClickedSetRadiusCalBtn();

    afx_msg void OnEnChangeRowEdit();
    afx_msg void OnEnChangeColEdit();

    afx_msg void OnEnChangeThetaPosEdit();
    afx_msg void OnBnClickedGotoThetaPosBtn();
    afx_msg void OnBnClickedPosUpdateBtn();
    afx_msg void OnEnChangeRadiusPosEdit();
    afx_msg void OnBnClickedGotoRadiusPosBtn();
    afx_msg void OnBnClickedGotoRThetaPosBtn();
    afx_msg void OnBnClickedGotoRowColBtn();
    afx_msg void OnBnClickedNextRowColBtn();

    afx_msg void OnBnClickedRelativeMoveChk();

    afx_msg void OnEnChangeTubeNumEdit();
    afx_msg void OnBnClickedFindTubeBtn();
    afx_msg void OnBnClickedGotoTubeBtn();
    afx_msg void OnBnClickedNextTubeBtn();

    afx_msg void OnBnClickedProbeHomeBtn();
    afx_msg void OnBnClickedProbeInitBtn();
    afx_msg void OnBnClickedProbeUpBtn();
    afx_msg void OnBnClickedProbeDownBtn();

    afx_msg void OnBnClickedRadioPlateSelect();
    afx_msg void OnBnClickedPlateCalBtn();
    afx_msg void OnBnClickedPlateEjectBtn();
    afx_msg void OnBnClickedPlateLoadBtn();
    afx_msg void OnBnClickedPlateInitBtn();
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
    afx_msg void OnBnClickedStepScriptBtn();
    afx_msg void OnEnVscrollScriptEdit();
    afx_msg void OnEnChangeCmdStepEdit();
    afx_msg void OnBnClickedPauseScriptBtn();
    afx_msg void OnBnClickedStopScriptBtn();
    afx_msg void OnEnChangeScriptPathEdit();
    afx_msg void OnBnClickedLoadScriptBtn();
    afx_msg void OnBnClickedSaveScriptBtn();
    afx_msg void OnBnClickedClearScriptBtn();
    afx_msg void OnBnClickedExit();
};
