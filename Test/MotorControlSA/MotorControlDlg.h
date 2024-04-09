
// MotorControlDlg.h : header file
//

#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <stdint.h>
#include <thread>


#include "CarouselController.hpp"
#include "Configuration.hpp"
#include "ControllerBoardInterface.hpp"
#include "FocusController.hpp"
#include "LEDRackController.hpp"
#include "MotorBase.hpp"
#include "PlateController.hpp"
#include "ProbeController.hpp"
#include "RadiusController.hpp"
#include "ReagentController.hpp"
#include "ThetaController.hpp"

//#define SIM_TEST


// CMotorControlDlg dialog
class CMotorControlDlg : public CDialogEx
{
// Construction
public:
	CMotorControlDlg(CWnd* pParent = NULL);	// standard constructor
    ~CMotorControlDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MOTORCONTROL_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


private:
    enum InitTypes
    {
        NoInit = 0,
        InitStatus = 1,
        InitTypeParamsLocal = 2,
        InitTypeParams = 3,
        InitAllParamsLocal = 4,
        InitAllParams = 5,
    };

    enum MsgBoxTypes
    {
        MsgBoxTypeNone = 0,
        MsgBoxTypeThetaZero,
        MsgBoxTypeRadiusZero,
        MsgBoxTypeThetaCal,
        MsgBoxTypeRadiusCal,
        MsgBoxTypeCalibratePlate,
        MsgBoxTypeCalibrateCarousel,
    };

    void signalHandler( const boost::system::error_code& ec, int signal_number );

    std::shared_ptr<boost::asio::signal_set>        pSignals_;
    std::shared_ptr<boost::asio::io_context>        pLocalIosvc_;
    std::shared_ptr<boost::asio::io_context::work>  pLocalWork_;
    std::shared_ptr<std::thread>                    pIoThread_;
    std::shared_ptr<ControllerBoardInterface>       pCbi_;
    std::shared_ptr<RadiusController>               pRadiusController;
    std::shared_ptr<ThetaController>                pThetaController;
    std::shared_ptr<ProbeController>                pProbeController;
    std::shared_ptr<ReagentController>              pReagentController;
    std::shared_ptr<FocusController>                pFocusController;
    std::shared_ptr<LEDRackController>              pLedRackController;
    std::shared_ptr<CarouselController>             pCarouselController;
    std::shared_ptr<PlateController>                pPlateController;

    t_pPTree            ptfilecfg;
    t_opPTree           ptconfig;
    std::string         motorConfigFile;

    MotorBase *         pPlateTheta;
    MotorBase *         pPlateRadius;
    MotorBase *         pPlateProbe;
    MotorBase *         pCarouselTheta;
    MotorBase *         pCarouselRadius;
    MotorBase *         pCarouselProbe;
    MotorBase *         pTheta;
    MotorBase *         pRadius;
    MotorBase *         pProbe;
    MotorBase *         pRack1;
    MotorBase *         pRack2;

    std::string         cbiPort1;
    std::string         cbiPort2;

    bool                cbiInit;
    bool                infoInit;
    bool                cntlEdit;
    BOOL                carouselSelected;
    BOOL                plateSelected;

    int32_t             probePos;
    int32_t             probeAbovePos;
    int32_t             probeRaisePos;
    int32_t             carouselSetPos;
    int32_t             carouselPos;
    int32_t             carouselTube1Pos;
    int32_t             carouselRadiusPos;
    int32_t             maxCarouselThetaPos;
    int32_t             plateRadiusCenterPos;
    int32_t             plateThetaCalPos;
    int32_t             maxPlateThetaPos;
    uint32_t            plateSetRow;
    CString             setRowStr;
    uint32_t            plateRow;
    CString             currentRowStr;
    uint32_t            plateSetCol;
    uint32_t            plateCol;
    int32_t             focusPos;
    int32_t             focusSetPos;
    int32_t             focusMaxTravel;
    uint32_t            carouselStatus;
    CString             carouselStatusStr;
    uint32_t            plateStatus;
    CString             plateStatusStr;
    uint32_t            boardStatus;
    CString             boardStatusStr;
    int32_t             rack1Pos;
    CString             rack1PathStr;
    int32_t             rack2Pos;
    CString             rack2PathStr;

    MotorRegisters      motorRegs;
    MotorRegisters *    pMotorRegs;


// Implementation
protected:
	HICON               m_hIcon;

	// Generated message map functions
    bool                Init(std::string port);
    void                Quit();
    bool                OpenCbi(std::string port);
    bool                InitInfoFile(void);
    void                DoAllControllers(bool close = false, bool stop = false);
    void                DoController( ControllerTypeId type, bool close = false, bool stop = false, bool applyInit = false );
    void                DisplayBoardStatus(ControllerTypeId type, MotorBase * pMotor, MotorBase * pMotor2 = NULL);
    void                SetGroupEnable( BOOL carouselEnable, BOOL plateEnable );
    void                ShowMotorRegister( MotorBase * pMotor );
    void                SetCarouselTube1ThetaPos( bool doTheta = false );
    void                UpdateFocusPosition( void );
    void                ClearFocusPosition( void );
    void                ShowFocusPosition( int32_t pos, bool clear = false );
    void                UpdateRackPositions( void );
    void                DoCarouselSelect( bool msgWait = true );
    void                DoPlateSelect( bool msgWait = true );
    void                DoCarouselInit( bool msgWait = true );
    void                DoPlateInit( bool msgWait = true );
    void                ShowInfoDialog( int32_t msgType );

    virtual BOOL        OnInitDialog();
    afx_msg void        OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void        OnPaint();
	afx_msg HCURSOR     OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
    afx_msg void    OnBnClickedOk();

    // high-level control actions
    afx_msg void    OnBnClickedCarouselSelect();
    afx_msg void    OnBnClickedPlateSelect();

    afx_msg void    OnBnClickedCarouselNextTubeBtn();
    afx_msg void    OnBnClickedCarouselFindTubeBtn();
    afx_msg void    OnEnChangeCarouselTubeSetEdit();
    afx_msg void    OnBnClickedCarouselTubePlusBtn();
    afx_msg void    OnBnClickedCarouselTubeMinusBtn();
    afx_msg void    OnBnClickedCarouselTubeSetBtn();

    afx_msg void    OnBnClickedPlateRowPlusBtn();
    afx_msg void    OnBnClickedPlateRowMinusBtn();
    afx_msg void    OnBnClickedPlateColPlusBtn();
    afx_msg void    OnBnClickedPlateColMinusBtn();
    afx_msg void    OnBnClickedPlateRowColSetBn();

    afx_msg void    OnBnClickedSampleProbeUp();
    afx_msg void    OnBnClickedSampleProbeDn();

    afx_msg void    OnBnClickedReagentArmUp();
    afx_msg void    OnBnClickedReagentArmDn();

    afx_msg void    OnBnClickedFineFocusCntlUpBtn();
    afx_msg void    OnBnClickedFineFocusCntlDnBtn();
    afx_msg void    OnBnClickedCoarseFocusCntlUpBtn();
    afx_msg void    OnBnClickedCoarseFocusCntlDnBtn();
    afx_msg void    OnBnClickedFastFocusCntlUpBtn();
    afx_msg void    OnBnClickedFastFocusCntlDnBtn();

    // controller configuration
    afx_msg void    OnBnClickedCarouselCntlrResetBtn();
    afx_msg void    OnBnClickedCarouselCntlrLoadHomeBtn();
    afx_msg void    OnBnClickedCarouselCntlrEjectBtn();
    afx_msg void    OnBnClickedCarouselCntlrInitBtn();
    afx_msg void    OnBnClickedCarouselCntlrCalBtn();

    afx_msg void    OnBnClickedPlateCntlrResetBtn();
    afx_msg void    OnBnClickedPlateCntlrLoadHomeBtn();
    afx_msg void    OnBnClickedPlateCntlrEjectBtn();
    afx_msg void    OnBnClickedPlateCntlrInitBtn();
    afx_msg void    OnBnClickedPlateCntlrCalBtn();

    afx_msg void    OnBnClickedLedCntlrResetBtn();
    afx_msg void    OnBnClickedLedCntlrHomeBtn();
    afx_msg void    OnBnClickedLedCntlrRack1SetTopBtn();
    afx_msg void    OnBnClickedLedCntlrRack1CalTopBtn();
    afx_msg void    OnBnClickedLedCntlrRack1SetCtrBtn();
    afx_msg void    OnBnClickedLedCntlrRack1CalCtrBtn();
    afx_msg void    OnBnClickedLedCntlrRack1SetBotBtn();
    afx_msg void    OnBnClickedLedCntlrRack1CalBotBtn();
    afx_msg void    OnBnClickedLedCntlrRack1SetHomeBtn();
    afx_msg void    OnBnClickedLedCntlrRack2SetTopBtn();
    afx_msg void    OnBnClickedLedCntlrRack2CalTopBtn();
    afx_msg void    OnBnClickedLedCntlrRack2SetCtrBtn();
    afx_msg void    OnBnClickedLedCntlrRack2CalCtrBtn();
    afx_msg void    OnBnClickedLedCntlrRack2SetBotBtn();
    afx_msg void    OnBnClickedLedCntlrRack2CalBotBtn();
    afx_msg void    OnBnClickedLedCntlrRack2SetHomeBtn();

    // low-level motor configuration
    afx_msg void    OnBnClickedFocusMotorResetBtn();
    afx_msg void    OnBnClickedFocusMotorHomeBtn();
    afx_msg void    OnBnClickedFocusClearErrorsBtn();
    afx_msg void    OnBnClickedFocusCntlrStatusBtn();
    afx_msg void    OnBnClickedFocusCntlrStopBtn();
    afx_msg void    OnEnChangeFocusPosSetEdit();
    afx_msg void    OnBnClickedFocusPosSetBtn();
    afx_msg void    OnBnClickedFocusPosSetCenterBtn();

    afx_msg void    OnBnClickedSampleProbeResetBtn();
    afx_msg void    OnBnClickedSampleProbeHomeBtn();
    afx_msg void    OnBnClickedSampleProbeSetAboveBtn();
    afx_msg void    OnBnClickedSampleProbeSetRaiseBtn();
    afx_msg void    OnBnClickedSampleProbeAdjustUpBtn();
    afx_msg void    OnBnClickedSampleProbeAdjustDnBtn();
    afx_msg void    OnBnClickedSampleProbeClearErrorsBtn();
    afx_msg void    OnBnClickedSampleProbeCntlrStatusBtn();
    afx_msg void    OnBnClickedSampleProbeCntlrStopBtn();

    afx_msg void    OnBnClickedThetaMotorResetBtn();
    afx_msg void    OnBnClickedThetaMotorHomeBtn();
    afx_msg void    OnBnClickedThetaMotorSetTube1PosBtn();
    afx_msg void    OnBnClickedThetaAdjustLeftBtn();
    afx_msg void    OnBnClickedThetaAdjustRightBtn();
    afx_msg void    OnBnClickedThetaClearErrorsBtn();
    afx_msg void    OnBnClickedThetaCntlrStatusBtn();
    afx_msg void    OnBnClickedThetaCntlrStopBtn();

    afx_msg void    OnBnClickedRadiusMotorResetBtn();
    afx_msg void    OnBnClickedRadiusMotorHomeBtn();
    afx_msg void    OnBnClickedRadiusMotorSetTubePosBtn();
    afx_msg void    OnBnClickedRadiusMotorSetPlateCenterPosBtn();
    afx_msg void    OnBnClickedRadiusAdjustInBtn();
    afx_msg void    OnBnClickedRadiusAdjustOutBtn();
    afx_msg void    OnBnClickedRadiusClearErrorsBtn();
    afx_msg void    OnBnClickedRadiusCntlrStatusBtn();
    afx_msg void    OnBnClickedRadiusCntlrStopBtn();

    afx_msg void    OnBnClickedReagentArmResetBtn();
    afx_msg void    OnBnClickedReagentArmHomeBtn();
    afx_msg void    OnBnClickedReagentAdjustUpBtn();
    afx_msg void    OnBnClickedReagentAdjustDnBtn();
    afx_msg void    OnBnClickedReagentClearErrorsBtn();
    afx_msg void    OnBnClickedReagentCntlrStatusBtn();
    afx_msg void    OnBnClickedReagentCntlrStopBtn();

    afx_msg void    OnBnClickedLedrack1MotorResetBtn();
    afx_msg void    OnBnClickedLedrack1MotorHomeBtn();
    afx_msg void    OnBnClickedLedrack1AdjustUpBtn();
    afx_msg void    OnBnClickedLedrack1AdjustDnBtn();
    afx_msg void    OnBnClickedLedrack1ClearErrorsBtn();
    afx_msg void    OnBnClickedLedrack1MotorStatusBtn();
    afx_msg void    OnBnClickedLedrack1MotorStopBtn();

    afx_msg void    OnBnClickedLedrack2MotorResetBtn();
    afx_msg void    OnBnClickedLedrack2MotorHomeBtn();
    afx_msg void    OnBnClickedLedrack2AdjustUpBtn();
    afx_msg void    OnBnClickedLedrack2AdjustDnBtn();
    afx_msg void    OnBnClickedLedrack2ClearErrorsBtn();
    afx_msg void    OnBnClickedLedrack2MotorStatusBtn();
    afx_msg void    OnBnClickedLedrack2MotorStopBtn();

    CButton         CarouselSelect;
    CStatic         CarouselGroup;
    CStatic         CarouselCurrentPosLbl;
    CStatic         CarouselTubeLbl;
    CEdit           CarouselCurrentPosDisp;
    CEdit           CarouselSetTubeEditDisp;
    CStatic         CarouselNextTubeLbl;
    CButton         CarouselNextTubeBtn;
    CStatic         CarouselFindTubeLbl;
    CButton         CarouselFindTubeBtn;
    CStatic         CarouselSetTubeLbl;
    CButton         CarouselTubePlusBtn;
    CButton         CarouselTubeMinusBtn;
    CButton         CarouselGotoBtn;

    CButton         PlateSelect;
    CStatic         PlateGroup;
    CStatic         PlateCurrentPosLbl;
    CStatic         PlateCurrentRowLbl;
    CEdit           PlateCurrentRowDisp;
    CStatic         PlateCurrentColLbl;
    CEdit           PlateCurrentColDisp;
    CStatic         PlateSetRowLbl;
    CEdit           PlateSetRowEditDisp;
    CButton         PlateRowPlusBtn;
    CButton         PlateRowMinusBtn;
    CStatic         PlateSetColLbl;
    CEdit           PlateSetColEditDisp;
    CButton         PlateColPlusBtn;
    CButton         PlateColMinusBtn;
    CButton         PlateGotoBtn;

    CEdit           CMotorStatusDisp;

    CEdit           CarouselStatusDisp;
    CEdit           PlateStatusDisp;
    CEdit           BoardStatusDisp;

    CStatic         CarouselCntlrGrp;
    CButton         CarouselCntlrResetBtn;
    CButton         CarouselCntlrLoadHomeBtn;
    CButton         CarouselCntlrEjectBtn;
    CButton         CarouselCntlrInitBtn;
    CButton         CarouselCntlrCalBtn;

    CStatic         PlateCntlrGrp;
    CButton         PlateCntlrResetBtn;
    CButton         PlateCntlrLoadHomeBtn;
    CButton         PlateCntlrEjectBtn;
    CButton         PlateCntlrInitBtn;
    CButton         PlateCntlrCalBtn;

    CStatic         LedCntlrGrp;
    CButton         LedCntlrResetBtn;
    CButton         LedCntlrHomeBtn;
    CEdit           Rack1PathDisp;
    CButton         LedCntlrRack1SetTopBtn;
    CButton         LedCntlrRack1CalTopBtn;
    CButton         LedCntlrRack1SetCtrBtn;
    CButton         LedCntlrRack1CalCtrBtn;
    CButton         LedCntlrRack1SetBotBtn;
    CButton         LedCntlrRack1CalBotBtn;
    CButton         LedCntlrRack1SetHomeBtn;
    CEdit           Rack2PathDisp;
    CButton         LedCntlrRack2SetTopBtn;
    CButton         LedCntlrRack2CalTopBtn;
    CButton         LedCntlrRack2SetCtrBtn;
    CButton         LedCntlrRack2CalCtrBtn;
    CButton         LedCntlrRack2SetBotBtn;
    CButton         LedCntlrRack2CalBotBtn;
    CButton         LedCntlrRack2SetHomeBtn;

    CEdit           FocusCurrentPosDisp;
    CEdit           FocusPosSetEditDisp;

    CButton         ThetaSetTube1PosBtn;
    CButton         RadiusSetCenterPosBtn;
    CButton         RadiusSetTubePosBtn;

    CStatic         LEDRack1Grp;
    CButton         LEDRack1ResetBtn;
    CButton         LEDRack1HomeBtn;
    CButton         LEDRack1AdjustUpBtn;
    CButton         LEDRack1AdjustDnBtn;
    CButton         LEDRack1ClearErrorsBtn;
    CButton         LEDRack1StatusBtn;
    CButton         LEDRack1StopBtn;

    CStatic         LEDRack2Grp;
    CButton         LEDRack2ResetBtn;
    CButton         LEDRack2HomeBtn;
    CButton         LEDRack2AdjustUpBtn;
    CButton         LEDRack2AdjustDnBtn;
    CButton         LEDRack2ClearErrorsBtn;
    CButton         LEDRack2StatusBtn;
    CButton         LEDRack2StopBtn;
};
