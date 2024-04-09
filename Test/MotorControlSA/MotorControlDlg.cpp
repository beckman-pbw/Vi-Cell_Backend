
// MotorControlDlg.cpp : implementation file
//

#include "stdafx.h"

#include <sstream>

#include "Logger.hpp"
#include "MotorControl.h"
#include "MotorControlDlg.h"    // this file MUST be included prior to afxdialog.h to prevent BOOST conflicts with the use of the 'new' operator

#include <afxdialogex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static const char MODULENAME[] = "MotorControlDlgSA";



// CAboutDlg dialog used for App About

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


// CMotorControlDlg dialog



CMotorControlDlg::CMotorControlDlg( CWnd* pParent /*=NULL*/ )
    : CDialogEx( IDD_MOTORCONTROL_DIALOG, pParent )
    , motorConfigFile( "MotorControl.info" )
    , pTheta( NULL )
    , pRadius( NULL )
    , pProbe( NULL )
    , pRack1( NULL )
    , pRack2( NULL )
    , cbiPort1( CNTLR_SN_A_STR )
    , cbiPort2( CNTLR_SN_B_STR )
    , cbiInit( false )
    , infoInit( false )
    , cntlEdit( false )
    , carouselSelected( TRUE )
    , plateSelected( FALSE )
    , probePos( 0 )
    , probeAbovePos( 0 )
    , probeRaisePos( 0 )
    , carouselSetPos( 1 )
    , carouselPos( 1 )
    , carouselTube1Pos( 0 )
    , carouselRadiusPos( 0 )
    , maxCarouselThetaPos(MaxThetaPosition)
    , plateRadiusCenterPos( 0 )
    , plateThetaCalPos( 0 )
    , maxPlateThetaPos( MaxThetaPosition )
    , plateSetRow( 0 )
    , setRowStr( _T( "" ) )
    , plateRow( 0 )
    , currentRowStr( _T( "" ) )
    , plateSetCol( 1 )
    , plateCol( 1 )
    , focusPos( 0 )
    , focusSetPos( 0 )
    , focusMaxTravel( FocusMaxTravel )
    , boardStatusStr( _T( "" ) )
    , rack1Pos( LEDRackController::PathUnk )
    , rack1PathStr( _T( "" ) )
    , rack2Pos( LEDRackController::PathUnk )
    , rack2PathStr( _T( "" ) )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    pLocalIosvc_.reset( new boost::asio::io_context() );
    pLocalWork_.reset( new boost::asio::io_context::work( *pLocalIosvc_ ) );

    // Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
    auto THREAD = std::bind( static_cast <std::size_t( boost::asio::io_context::* )( void )> ( &boost::asio::io_context::run ), pLocalIosvc_.get() );
    pIoThread_.reset( new std::thread( THREAD ) );
    pIoThread_->detach();

    pSignals_.reset( new boost::asio::signal_set( *pLocalIosvc_, SIGINT, SIGTERM, SIGABRT ) );
    pCbi_.reset( new ControllerBoardInterface( pLocalIosvc_, cbiPort1, cbiPort2 ) );

    pRadiusController.reset( new RadiusController( pLocalIosvc_, pCbi_ ) );
    pThetaController.reset(new ThetaController( pLocalIosvc_, pCbi_ ) );
    pProbeController.reset( new ProbeController( pLocalIosvc_, pCbi_ ) );
    pReagentController.reset(new ReagentController( pLocalIosvc_, pCbi_ ) );
    pFocusController.reset( new FocusController( pLocalIosvc_, pCbi_ ) );
    pLedRackController.reset(new LEDRackController( pLocalIosvc_, pCbi_ ) );
    pCarouselController.reset( new CarouselController( pLocalIosvc_, pCbi_ ) );
    pPlateController.reset(new PlateController( pLocalIosvc_, pCbi_ ) );

    boost::system::error_code ec;
    Logger::L().Initialize( ec, boost::str( boost::format( "%s.info" ) % MODULENAME ) );
    Logger::L().Log ( MODULENAME, severity_level::normal, MODULENAME );
}

CMotorControlDlg::~CMotorControlDlg()
{
    Quit();
}

//*****************************************************************************
void CMotorControlDlg::signalHandler( const boost::system::error_code& ec, int signal_number )
{
    if ( ec )
    {
        Logger::L().Log ( MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"" );
    }

    Logger::L().Log ( MODULENAME, severity_level::critical, boost::str( boost::format( "Received signal no. %d" ) % signal_number ) );

    // All done listening.
    pSignals_->cancel();

    // Try to get out of here.
    pLocalIosvc_->post( std::bind( &CMotorControlDlg::Quit, this ) );
}

void CMotorControlDlg::DoDataExchange( CDataExchange* pDX )
{
    CDialogEx::DoDataExchange( pDX );
    DDX_Control( pDX, IDC_CAROUSEL_SELECT, CarouselSelect );
    DDX_Control( pDX, IDC_CAROUSEL_CNTL_GRP_LBL, CarouselGroup );
    DDX_Control( pDX, IDC_CAROUSEL_POS_LBL, CarouselCurrentPosLbl );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_DISP_LBL, CarouselTubeLbl );
    DDX_Control( pDX, IDC_CAROUSEL_NEXT_TUBE_LBL, CarouselNextTubeLbl );
    DDX_Control( pDX, IDC_CAROUSEL_NEXT_TUBE_BTN, CarouselNextTubeBtn );
    DDX_Control( pDX, IDC_CAROUSEL_FIND_TUBE_LBL, CarouselFindTubeLbl );
    DDX_Control( pDX, IDC_CAROUSEL_FIND_TUBE_BTN, CarouselFindTubeBtn );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_SET_LBL, CarouselSetTubeLbl );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_PLUS_BTN, CarouselTubePlusBtn );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_MINUS_BTN, CarouselTubeMinusBtn );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_SET_BTN, CarouselGotoBtn );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_SET_EDIT, CarouselSetTubeEditDisp );
    DDX_Text   ( pDX, IDC_CAROUSEL_TUBE_SET_EDIT, carouselSetPos );
    DDV_MinMaxInt( pDX, carouselSetPos, 1, 24 );
    DDX_Control( pDX, IDC_CAROUSEL_TUBE_DISP, CarouselCurrentPosDisp );
    DDX_Text   ( pDX, IDC_CAROUSEL_TUBE_DISP, carouselPos );
    DDV_MinMaxInt( pDX, carouselPos, 1, 24 );

    DDX_Control( pDX, IDC_PLATE_SELECT, PlateSelect );
    DDX_Control( pDX, IDC_PLATE_CNTL_GRP_LBL, PlateGroup );
    DDX_Control( pDX, IDC_PLATE_POS_LBL, PlateCurrentPosLbl );
    DDX_Control( pDX, IDC_PLATE_ROW_DISP_LBL, PlateCurrentRowLbl );
    DDX_Control( pDX, IDC_PLATE_ROW_DISP, PlateCurrentRowDisp );
    DDX_Text   ( pDX, IDC_PLATE_ROW_DISP, currentRowStr );
    DDX_Control( pDX, IDC_PLATE_COL_DISP_LBL, PlateCurrentColLbl );
    DDX_Control( pDX, IDC_PLATE_COL_DISP, PlateCurrentColDisp );
    DDX_Text   ( pDX, IDC_PLATE_COL_DISP, plateCol );
    DDV_MinMaxInt( pDX, plateCol, 1, 12 );
    DDX_Control( pDX, IDC_PLATE_ROW_SET_LBL, PlateSetRowLbl );
    DDX_Control( pDX, IDC_PLATE_ROW_SET_EDIT, PlateSetRowEditDisp );
    DDX_Text   ( pDX, IDC_PLATE_ROW_SET_EDIT, setRowStr );
    DDX_Control( pDX, IDC_PLATE_COL_SET_LBL, PlateSetColLbl );
    DDX_Control( pDX, IDC_PLATE_COL_SET_EDIT, PlateSetColEditDisp );
    DDX_Text   ( pDX, IDC_PLATE_COL_SET_EDIT, plateSetCol );
    DDV_MinMaxInt( pDX, plateSetCol, 1, 12 );
    DDX_Control( pDX, IDC_PLATE_ROW_PLUS_BTN, PlateRowPlusBtn );
    DDX_Control( pDX, IDC_PLATE_ROW_MINUS_BTN, PlateRowMinusBtn );
    DDX_Control( pDX, IDC_PLATE_COL_PLUS_BTN, PlateColPlusBtn );
    DDX_Control( pDX, IDC_PLATE_COL_MINUS_BTN, PlateColMinusBtn );
    DDX_Control( pDX, IDC_PLATE_ROW_COL_SET_BTN, PlateGotoBtn );

    DDX_Control( pDX, IDC_MOTOR_STATUS_DISP, CMotorStatusDisp );

    DDX_Control( pDX, IDC_CAROUSEL_STATUS_DISP, CarouselStatusDisp );
    DDX_Text   ( pDX, IDC_CAROUSEL_STATUS_DISP, carouselStatusStr );
    DDX_Control( pDX, IDC_PLATE_STATUS_DISP, PlateStatusDisp );
    DDX_Text   ( pDX, IDC_PLATE_STATUS_DISP, plateStatusStr );
    DDX_Control( pDX, IDC_BOARD_STATUS_DISP, BoardStatusDisp );
    DDX_Text   ( pDX, IDC_BOARD_STATUS_DISP, boardStatusStr );

    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_CONFIG_GRP, CarouselCntlrGrp );
    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_RESET_BTN, CarouselCntlrResetBtn );
    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_LOAD_HOME_BTN, CarouselCntlrLoadHomeBtn );
    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_EJECT_BTN, CarouselCntlrEjectBtn );
    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_INIT_BTN, CarouselCntlrInitBtn );
    DDX_Control( pDX, IDC_CAROUSEL_CNTLR_CAL_BTN, CarouselCntlrCalBtn );

    DDX_Control( pDX, IDC_PLATE_CNTLR_CONFIG_GRP, PlateCntlrGrp );
    DDX_Control( pDX, IDC_PLATE_CNTLR_RESET_BTN, PlateCntlrResetBtn );
    DDX_Control( pDX, IDC_PLATE_CNTLR_LOAD_HOME_BTN, PlateCntlrLoadHomeBtn );
    DDX_Control( pDX, IDC_PLATE_CNTLR_EJECT_BTN, PlateCntlrEjectBtn );
    DDX_Control( pDX, IDC_PLATE_CNTLR_INIT_BTN, PlateCntlrInitBtn );
    DDX_Control( pDX, IDC_PLATE_CNTLR_CAL_BTN, PlateCntlrCalBtn );

    DDX_Control( pDX, IDC_LED_CNTLR_CONFIG_GRP, LedCntlrGrp );
    DDX_Control( pDX, IDC_LED_CNTLR_RESET_BTN, LedCntlrResetBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_HOME_BTN, LedCntlrHomeBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_PATH_DISP, Rack1PathDisp );
    DDX_Text   ( pDX, IDC_LED_CNTLR_RACK1_PATH_DISP, rack1PathStr );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_SET_TOP_BTN, LedCntlrRack1SetTopBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_CAL_TOP_BTN, LedCntlrRack1CalTopBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_SET_CTR_BTN, LedCntlrRack1SetCtrBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_CAL_CTR_BTN, LedCntlrRack1CalCtrBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_SET_BOT_BTN, LedCntlrRack1SetBotBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_CAL_BOT_BTN, LedCntlrRack1CalBotBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK1_SET_HOME_BTN, LedCntlrRack1SetHomeBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_PATH_DISP, Rack2PathDisp );
    DDX_Text   ( pDX, IDC_LED_CNTLR_RACK2_PATH_DISP, rack2PathStr );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_SET_TOP_BTN, LedCntlrRack2SetTopBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_CAL_TOP_BTN, LedCntlrRack2CalTopBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_SET_CTR_BTN, LedCntlrRack2SetCtrBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_CAL_CTR_BTN, LedCntlrRack2CalCtrBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_SET_BOT_BTN, LedCntlrRack2SetBotBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_CAL_BOT_BTN, LedCntlrRack2CalBotBtn );
    DDX_Control( pDX, IDC_LED_CNTLR_RACK2_SET_HOME_BTN, LedCntlrRack2SetHomeBtn );

    DDX_Control( pDX, IDC_FOCUS_POS_DISP, FocusCurrentPosDisp );
    DDX_Text   ( pDX, IDC_FOCUS_POS_DISP, focusPos );
    DDX_Control( pDX, IDC_FOCUS_POS_SET_EDIT, FocusPosSetEditDisp );
    DDX_Text   ( pDX, IDC_FOCUS_POS_SET_EDIT, focusSetPos );
    DDV_MinMaxInt( pDX, focusSetPos, 0, focusMaxTravel );

    DDX_Control( pDX, IDC_THETA_MOTOR_SET_TUBE1_POS_BTN, ThetaSetTube1PosBtn );

    DDX_Control( pDX, IDC_RADIUS_MOTOR_SET_PLATE_CENTER_POS_BTN, RadiusSetCenterPosBtn );
    DDX_Control( pDX, IDC_RADIUS_MOTOR_SET_TUBE_POS_BTN, RadiusSetTubePosBtn );

    DDX_Control( pDX, IDC_LEDRACK1_MOTOR_CONFIG_GRP, LEDRack1Grp );
    DDX_Control( pDX, IDC_LEDRACK1_MOTOR_RESET_BTN, LEDRack1ResetBtn );
    DDX_Control( pDX, IDC_LEDRACK1_MOTOR_HOME_BTN, LEDRack1HomeBtn );
    DDX_Control( pDX, IDC_LEDRACK1_ADJUST_UP_BTN, LEDRack1AdjustUpBtn );
    DDX_Control( pDX, IDC_LEDRACK1_ADJUST_DN_BTN, LEDRack1AdjustDnBtn );
    DDX_Control( pDX, IDC_LEDRACK1_CLEAR_ERRORS_BTN, LEDRack1ClearErrorsBtn );
    DDX_Control( pDX, IDC_LEDRACK1_MOTOR_STATUS_BTN, LEDRack1StatusBtn );
    DDX_Control( pDX, IDC_LEDRACK1_MOTOR_STOP_BTN, LEDRack1StopBtn );

    DDX_Control( pDX, IDC_LEDRACK2_MOTOR_CONFIG_GRP, LEDRack2Grp );
    DDX_Control( pDX, IDC_LEDRACK2_MOTOR_RESET_BTN, LEDRack2ResetBtn );
    DDX_Control( pDX, IDC_LEDRACK2_MOTOR_HOME_BTN, LEDRack2HomeBtn );
    DDX_Control( pDX, IDC_LEDRACK2_ADJUST_UP_BTN, LEDRack2AdjustUpBtn );
    DDX_Control( pDX, IDC_LEDRACK2_ADJUST_DN_BTN, LEDRack2AdjustDnBtn );
    DDX_Control( pDX, IDC_LEDRACK2_CLEAR_ERRORS_BTN, LEDRack2ClearErrorsBtn );
    DDX_Control( pDX, IDC_LEDRACK2_MOTOR_STATUS_BTN, LEDRack2StatusBtn );
    DDX_Control( pDX, IDC_LEDRACK2_MOTOR_STOP_BTN, LEDRack2StopBtn );
}

BEGIN_MESSAGE_MAP(CMotorControlDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

    ON_BN_CLICKED(IDC_CAROUSEL_SELECT, &CMotorControlDlg::OnBnClickedCarouselSelect)
    ON_BN_CLICKED(IDC_CAROUSEL_NEXT_TUBE_BTN, &CMotorControlDlg::OnBnClickedCarouselNextTubeBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_FIND_TUBE_BTN, &CMotorControlDlg::OnBnClickedCarouselFindTubeBtn)
    ON_EN_CHANGE (IDC_CAROUSEL_TUBE_SET_EDIT, &CMotorControlDlg::OnEnChangeCarouselTubeSetEdit)
    ON_BN_CLICKED(IDC_CAROUSEL_TUBE_PLUS_BTN, &CMotorControlDlg::OnBnClickedCarouselTubePlusBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_TUBE_MINUS_BTN, &CMotorControlDlg::OnBnClickedCarouselTubeMinusBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_TUBE_SET_BTN, &CMotorControlDlg::OnBnClickedCarouselTubeSetBtn)

    ON_BN_CLICKED( IDC_PLATE_SELECT, &CMotorControlDlg::OnBnClickedPlateSelect )
    ON_BN_CLICKED(IDC_PLATE_ROW_PLUS_BTN, &CMotorControlDlg::OnBnClickedPlateRowPlusBtn)
    ON_BN_CLICKED(IDC_PLATE_ROW_MINUS_BTN, &CMotorControlDlg::OnBnClickedPlateRowMinusBtn)
    ON_BN_CLICKED(IDC_PLATE_COL_PLUS_BTN, &CMotorControlDlg::OnBnClickedPlateColPlusBtn)
    ON_BN_CLICKED(IDC_PLATE_COL_MINUS_BTN, &CMotorControlDlg::OnBnClickedPlateColMinusBtn)
    ON_BN_CLICKED(IDC_PLATE_ROW_COL_SET_BTN, &CMotorControlDlg::OnBnClickedPlateRowColSetBn)

    ON_BN_CLICKED( IDC_SAMPLE_PROBE_UP_BTN, &CMotorControlDlg::OnBnClickedSampleProbeUp )
    ON_BN_CLICKED( IDC_SAMPLE_PROBE_DN_BTN, &CMotorControlDlg::OnBnClickedSampleProbeDn )

    ON_BN_CLICKED(IDC_REAGENT_ARM_UP_BTN, &CMotorControlDlg::OnBnClickedReagentArmUp)
    ON_BN_CLICKED(IDC_REAGENT_ARM_DN_BTN, &CMotorControlDlg::OnBnClickedReagentArmDn)

    ON_BN_CLICKED(IDC_FINE_FOCUS_CNTL_UP_BTN, &CMotorControlDlg::OnBnClickedFineFocusCntlUpBtn)
    ON_BN_CLICKED(IDC_FINE_FOCUS_CNTL_DN_BTN, &CMotorControlDlg::OnBnClickedFineFocusCntlDnBtn)
    ON_BN_CLICKED(IDC_COARSE_FOCUS_CNTL_UP_BTN, &CMotorControlDlg::OnBnClickedCoarseFocusCntlUpBtn)
    ON_BN_CLICKED(IDC_COARSE_FOCUS_CNTL_DN_BTN, &CMotorControlDlg::OnBnClickedCoarseFocusCntlDnBtn)
    ON_BN_CLICKED(IDC_FAST_FOCUS_CNTL_UP_BTN, &CMotorControlDlg::OnBnClickedFastFocusCntlUpBtn)
    ON_BN_CLICKED(IDC_FAST_FOCUS_CNTL_DN_BTN, &CMotorControlDlg::OnBnClickedFastFocusCntlDnBtn)

    ON_BN_CLICKED(IDC_CAROUSEL_CNTLR_RESET_BTN, &CMotorControlDlg::OnBnClickedCarouselCntlrResetBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_CNTLR_LOAD_HOME_BTN, &CMotorControlDlg::OnBnClickedCarouselCntlrLoadHomeBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_CNTLR_EJECT_BTN, &CMotorControlDlg::OnBnClickedCarouselCntlrEjectBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_CNTLR_INIT_BTN, &CMotorControlDlg::OnBnClickedCarouselCntlrInitBtn)
    ON_BN_CLICKED(IDC_CAROUSEL_CNTLR_CAL_BTN, &CMotorControlDlg::OnBnClickedCarouselCntlrCalBtn)

    ON_BN_CLICKED(IDC_PLATE_CNTLR_RESET_BTN, &CMotorControlDlg::OnBnClickedPlateCntlrResetBtn)
    ON_BN_CLICKED(IDC_PLATE_CNTLR_LOAD_HOME_BTN, &CMotorControlDlg::OnBnClickedPlateCntlrLoadHomeBtn)
    ON_BN_CLICKED(IDC_PLATE_CNTLR_EJECT_BTN, &CMotorControlDlg::OnBnClickedPlateCntlrEjectBtn)
    ON_BN_CLICKED( IDC_PLATE_CNTLR_INIT_BTN, &CMotorControlDlg::OnBnClickedPlateCntlrInitBtn )
    ON_BN_CLICKED( IDC_PLATE_CNTLR_CAL_BTN, &CMotorControlDlg::OnBnClickedPlateCntlrCalBtn )

    ON_BN_CLICKED(IDC_LED_CNTLR_RESET_BTN, &CMotorControlDlg::OnBnClickedLedCntlrResetBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_HOME_BTN, &CMotorControlDlg::OnBnClickedLedCntlrHomeBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_SET_TOP_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1SetTopBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_CAL_TOP_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1CalTopBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_SET_CTR_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1SetCtrBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_CAL_CTR_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1CalCtrBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_SET_BOT_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1SetBotBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_CAL_BOT_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1CalBotBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK1_SET_HOME_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack1SetHomeBtn )
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_SET_TOP_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2SetTopBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_CAL_TOP_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2CalTopBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_SET_CTR_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2SetCtrBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_CAL_CTR_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2CalCtrBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_SET_BOT_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2SetBotBtn)
    ON_BN_CLICKED(IDC_LED_CNTLR_RACK2_CAL_BOT_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2CalBotBtn)
    ON_BN_CLICKED( IDC_LED_CNTLR_RACK2_SET_HOME_BTN, &CMotorControlDlg::OnBnClickedLedCntlrRack2SetHomeBtn )

    ON_BN_CLICKED( IDC_FOCUS_MOTOR_RESET_BTN, &CMotorControlDlg::OnBnClickedFocusMotorResetBtn )
    ON_BN_CLICKED( IDC_FOCUS_MOTOR_HOME_BTN, &CMotorControlDlg::OnBnClickedFocusMotorHomeBtn )
    ON_BN_CLICKED( IDC_FOCUS_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedFocusClearErrorsBtn )
    ON_BN_CLICKED( IDC_FOCUS_CNTLR_STATUS_BTN, &CMotorControlDlg::OnBnClickedFocusCntlrStatusBtn )
    ON_BN_CLICKED( IDC_FOCUS_CNTLR_STOP_BTN, &CMotorControlDlg::OnBnClickedFocusCntlrStopBtn )
    ON_EN_CHANGE ( IDC_FOCUS_POS_SET_EDIT, &CMotorControlDlg::OnEnChangeFocusPosSetEdit )
    ON_BN_CLICKED( IDC_FOCUS_POS_SET_BTN, &CMotorControlDlg::OnBnClickedFocusPosSetBtn )
    ON_BN_CLICKED( IDC_FOCUS_POS_SET_MID_BTN, &CMotorControlDlg::OnBnClickedFocusPosSetCenterBtn )

    ON_BN_CLICKED(IDC_SAMPLE_PROBE_RESET_BTN, &CMotorControlDlg::OnBnClickedSampleProbeResetBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_HOME_BTN, &CMotorControlDlg::OnBnClickedSampleProbeHomeBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_SET_ABOVE_BTN, &CMotorControlDlg::OnBnClickedSampleProbeSetAboveBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_SET_RAISE_BTN, &CMotorControlDlg::OnBnClickedSampleProbeSetRaiseBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_ADJUST_UP_BTN, &CMotorControlDlg::OnBnClickedSampleProbeAdjustUpBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_ADJUST_DN_BTN, &CMotorControlDlg::OnBnClickedSampleProbeAdjustDnBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedSampleProbeClearErrorsBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_CNTLR_STATUS_BTN, &CMotorControlDlg::OnBnClickedSampleProbeCntlrStatusBtn)
    ON_BN_CLICKED(IDC_SAMPLE_PROBE_CNTLR_STOP_BTN, &CMotorControlDlg::OnBnClickedSampleProbeCntlrStopBtn)

    ON_BN_CLICKED(IDC_THETA_MOTOR_RESET_BTN, &CMotorControlDlg::OnBnClickedThetaMotorResetBtn)
    ON_BN_CLICKED(IDC_THETA_MOTOR_HOME_BTN, &CMotorControlDlg::OnBnClickedThetaMotorHomeBtn)
    ON_BN_CLICKED(IDC_THETA_MOTOR_SET_TUBE1_POS_BTN, &CMotorControlDlg::OnBnClickedThetaMotorSetTube1PosBtn)
    ON_BN_CLICKED(IDC_THETA_ADJUST_LEFT_BTN, &CMotorControlDlg::OnBnClickedThetaAdjustLeftBtn)
    ON_BN_CLICKED(IDC_THETA_ADJUST_RIGHT_BTN, &CMotorControlDlg::OnBnClickedThetaAdjustRightBtn)
    ON_BN_CLICKED(IDC_THETA_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedThetaClearErrorsBtn)
    ON_BN_CLICKED(IDC_THETA_CNTLR_STATUS_BTN, &CMotorControlDlg::OnBnClickedThetaCntlrStatusBtn)
    ON_BN_CLICKED(IDC_THETA_CNTLR_STOP_BTN, &CMotorControlDlg::OnBnClickedThetaCntlrStopBtn)

    ON_BN_CLICKED(IDC_RADIUS_MOTOR_RESET_BTN, &CMotorControlDlg::OnBnClickedRadiusMotorResetBtn)
    ON_BN_CLICKED(IDC_RADIUS_MOTOR_HOME_BTN, &CMotorControlDlg::OnBnClickedRadiusMotorHomeBtn)
    ON_BN_CLICKED(IDC_RADIUS_MOTOR_SET_TUBE_POS_BTN, &CMotorControlDlg::OnBnClickedRadiusMotorSetTubePosBtn)
    ON_BN_CLICKED(IDC_RADIUS_MOTOR_SET_PLATE_CENTER_POS_BTN, &CMotorControlDlg::OnBnClickedRadiusMotorSetPlateCenterPosBtn)
    ON_BN_CLICKED(IDC_RADIUS_ADJUST_IN_BTN, &CMotorControlDlg::OnBnClickedRadiusAdjustInBtn)
    ON_BN_CLICKED(IDC_RADIUS_ADJUST_OUT_BTN, &CMotorControlDlg::OnBnClickedRadiusAdjustOutBtn)
    ON_BN_CLICKED(IDC_RADIUS_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedRadiusClearErrorsBtn)
    ON_BN_CLICKED(IDC_RADIUS_CNTLR_STATUS_BTN, &CMotorControlDlg::OnBnClickedRadiusCntlrStatusBtn)
    ON_BN_CLICKED(IDC_RADIUS_CNTLR_STOP_BTN, &CMotorControlDlg::OnBnClickedRadiusCntlrStopBtn)

    ON_BN_CLICKED(IDC_REAGENT_ARM_RESET_BTN, &CMotorControlDlg::OnBnClickedReagentArmResetBtn)
    ON_BN_CLICKED(IDC_REAGENT_ARM_HOME_BTN, &CMotorControlDlg::OnBnClickedReagentArmHomeBtn)
    ON_BN_CLICKED(IDC_REAGENT_ADJUST_UP_BTN, &CMotorControlDlg::OnBnClickedReagentAdjustUpBtn)
    ON_BN_CLICKED(IDC_REAGENT_ADJUST_DN_BTN, &CMotorControlDlg::OnBnClickedReagentAdjustDnBtn)
    ON_BN_CLICKED(IDC_REAGENT_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedReagentClearErrorsBtn)
    ON_BN_CLICKED(IDC_REAGENT_CNTLR_STATUS_BTN, &CMotorControlDlg::OnBnClickedReagentCntlrStatusBtn)
    ON_BN_CLICKED(IDC_REAGENT_CNTLR_STOP_BTN, &CMotorControlDlg::OnBnClickedReagentCntlrStopBtn)

    ON_BN_CLICKED(IDC_LEDRACK1_MOTOR_RESET_BTN, &CMotorControlDlg::OnBnClickedLedrack1MotorResetBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_MOTOR_HOME_BTN, &CMotorControlDlg::OnBnClickedLedrack1MotorHomeBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_ADJUST_UP_BTN, &CMotorControlDlg::OnBnClickedLedrack1AdjustUpBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_ADJUST_DN_BTN, &CMotorControlDlg::OnBnClickedLedrack1AdjustDnBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedLedrack1ClearErrorsBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_MOTOR_STATUS_BTN, &CMotorControlDlg::OnBnClickedLedrack1MotorStatusBtn)
    ON_BN_CLICKED(IDC_LEDRACK1_MOTOR_STOP_BTN, &CMotorControlDlg::OnBnClickedLedrack1MotorStopBtn)

    ON_BN_CLICKED(IDC_LEDRACK2_MOTOR_RESET_BTN, &CMotorControlDlg::OnBnClickedLedrack2MotorResetBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_MOTOR_HOME_BTN, &CMotorControlDlg::OnBnClickedLedrack2MotorHomeBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_ADJUST_UP_BTN, &CMotorControlDlg::OnBnClickedLedrack2AdjustUpBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_ADJUST_DN_BTN, &CMotorControlDlg::OnBnClickedLedrack2AdjustDnBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_CLEAR_ERRORS_BTN, &CMotorControlDlg::OnBnClickedLedrack2ClearErrorsBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_MOTOR_STATUS_BTN, &CMotorControlDlg::OnBnClickedLedrack2MotorStatusBtn)
    ON_BN_CLICKED(IDC_LEDRACK2_MOTOR_STOP_BTN, &CMotorControlDlg::OnBnClickedLedrack2MotorStopBtn)

    ON_BN_CLICKED( IDOK, &CMotorControlDlg::OnBnClickedOk )

END_MESSAGE_MAP()


bool CMotorControlDlg::Init(std::string port)
{
    Logger::L().Log ( MODULENAME, severity_level::debug1, "MotorControltester::init: <enter>" );

    DWORD num_devices;
    FT_STATUS fts = FT_CreateDeviceInfoList( &num_devices );
    if ( num_devices > 0 )
    {
        Logger::L().Log ( MODULENAME, severity_level::normal, std::to_string( num_devices ) + " FTDI devices attached" );

        for ( std::size_t i = 0; i < num_devices; i++ )
        {
            DWORD flags;
            DWORD id;
            DWORD type;
            DWORD locId;
            char serialNumber[16];
            char description[64];
            FT_HANDLE ftHandleTemp;

            // Get information for device.
            fts = FT_GetDeviceInfoDetail( (DWORD)i, &flags, &type, &id, &locId, serialNumber, description, &ftHandleTemp );

            Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str(
                boost::format( "dev #: %d, flags: 0x%04X, type: 0x%04X, locId: 0x%04X, sn: %s, desc: %s" ) % i % flags % type % locId % serialNumber % description ) );
        }

        boost::system::error_code ec = pCbi_->OpenSerial( port );
        if ( ec )
        {
            Logger::L().Log ( MODULENAME, severity_level::critical, "Unable to open controller board " + port + ": " + ec.message() );
            return false;
        }
    }
    else
    {
        Logger::L().Log ( MODULENAME, severity_level::critical, "No devices found! " );
        return false;
    }

    return ( num_devices > 0 );
}

bool CMotorControlDlg::OpenCbi(std::string port)
{
    bool isOpen = false;

    isOpen = pCbi_->IsSerialOpen();

    if ( !isOpen )
    {
        boost::system::error_code ec = pCbi_->OpenSerial( port );
        if ( ec )
        {
            Logger::L().Log ( MODULENAME, severity_level::normal, "Unable to open controller board " + port + ": " + ec.message() );
        }
        else
        {
            isOpen = true;
        }
    }
    return isOpen;
}

bool CMotorControlDlg::InitInfoFile(void)
{
    bool success = true;
    boost::system::error_code ec;

    if ( !infoInit )
    {
        if ( !ptfilecfg )
        {
            ptfilecfg = ConfigUtils::OpenConfigFile( motorConfigFile, ec, true );
        }

        if ( !ptfilecfg )
        {
            Logger::L().Log ( MODULENAME, severity_level::error, "Error opening configuration file \"" + motorConfigFile + "\"" );
            success = false;
        }
        else
        {
            if ( !ptconfig )
            {
                ptconfig = ptfilecfg->get_child_optional( "config" );
            }

            if ( !ptconfig )
            {
                Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + motorConfigFile + "\" - \"config\" section not found" );
                success = false;
            }
        }
        infoInit = success;
    }
    return success;
}

void CMotorControlDlg::DoAllControllers( bool close, bool stop )
{
    ControllerTypeId idNext = ControllerTypeUnknown;
    ControllerTypeId idThis = ControllerTypeUnknown;
    bool apply = true;

    while ( idNext != ControllerTypeIllegal )
    {
        apply = true;
        switch ( idNext )
        {
            case ControllerTypeId::ControllerTypeUnknown:
            {
                idThis = idNext;
                idNext = ControllerTypeRadius;
                apply = false;
                break;
            }

            case ControllerTypeId::ControllerTypeRadius:
            {
                idThis = idNext;
                idNext = ControllerTypeTheta;
                apply = false;
                break;
            }

            case ControllerTypeId::ControllerTypeTheta:
            {
                idThis = idNext;
                idNext = ControllerTypeProbe;
                apply = false;
                break;
            }

            case ControllerTypeId::ControllerTypeProbe:
            {
                idThis = idNext;
                idNext = ControllerTypeReagent;
                break;
            }

            case ControllerTypeId::ControllerTypeReagent:
            {
                idThis = idNext;
                idNext = ControllerTypeFocus;
                break;
            }

            case ControllerTypeId::ControllerTypeFocus:
            {
                idThis = idNext;
                idNext = ControllerTypeLEDRack;
                break;
            }

            case ControllerTypeId::ControllerTypeLEDRack:
            {
                idThis = idNext;
                idNext = ControllerTypeCarousel;
                break;
            }

            case ControllerTypeId::ControllerTypeCarousel:
            {
                idThis = idNext;
                idNext = ControllerTypePlate;
                break;
            }

            case ControllerTypeId::ControllerTypePlate:
            {
                idThis = idNext;
                idNext = ControllerTypeIllegal;
                break;
            }

            default:
            {
                idThis = ControllerTypeUnknown;
                idNext = ControllerTypeIllegal;
                break;
            }
        }

        if ( ( idThis != ControllerTypeId::ControllerTypeUnknown ) && ( idThis != ControllerTypeId::ControllerTypeIllegal ) )
        {
            DoController( idThis, close, stop, apply );
        }
    }
}

void CMotorControlDlg::DoController( ControllerTypeId type, bool close, bool stop, bool applyInit )
{
    bool controllerOk = false;

    switch ( type )
    {
        case ControllerTypeId::ControllerTypeRadius:
        {
            controllerOk = ( pRadiusController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeTheta:
        {
            controllerOk = ( pThetaController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeProbe:
        {
            controllerOk = ( pProbeController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeReagent:
        {
            controllerOk = ( pReagentController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeFocus:
        {
            controllerOk = ( pFocusController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeLEDRack:
        {
            controllerOk = ( pLedRackController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypeCarousel:
        {
            controllerOk = ( pCarouselController != NULL );
            break;
        }

        case ControllerTypeId::ControllerTypePlate:
        {
            controllerOk = ( pPlateController != NULL );
            break;
        }

        default:
        {
            break;
        }
    }

    if ( ( !close ) && ( !stop ) && ( !infoInit ) )
    {
        InitInfoFile();
    }

    if ( controllerOk )
    {

        switch ( type )
        {
            case ControllerTypeId::ControllerTypeRadius:
            {
                if ( close )
                {
                    pRadiusController->Quit();
                }
                else if ( stop )
                {
                    pRadiusController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pRadiusController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypeTheta:
            {
                if ( close )
                {
                    pThetaController->Quit();
                }
                else if ( stop )
                {
                    pThetaController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pThetaController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypeProbe:
            {
                if ( close )
                {
                    pProbeController->Quit();
                }
                else if ( stop )
                {
                    pProbeController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pProbeController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypeReagent:
            {
                if ( close )
                {
                    pReagentController->Quit();
                }
                else if ( stop )
                {
                    pReagentController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pReagentController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypeFocus:
            {
                if ( close )
                {
                    pFocusController->Quit();
                }
                else if ( stop )
                {
                    pFocusController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pFocusController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                    focusMaxTravel = pFocusController->GetFocusMax();
                }
                break;
            }

            case ControllerTypeId::ControllerTypeLEDRack:
            {
                if ( close )
                {
                    pLedRackController->Quit();
                }
                else if ( stop )
                {
                    pLedRackController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pLedRackController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypeCarousel:
            {
                if ( close )
                {
                    pCarouselController->Quit();
                }
                else if ( stop )
                {
                    pCarouselController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pCarouselController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            case ControllerTypeId::ControllerTypePlate:
            {
                if ( close )
                {
                    pPlateController->Quit();
                }
                else if ( stop )
                {
                    pPlateController->Stop();
                }
                else
                {
                    // allow each motor controller to initialize its parameters from the external info file
                    pPlateController->Init( cbiPort1, ptfilecfg, applyInit, motorConfigFile );
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void CMotorControlDlg::DisplayBoardStatus(ControllerTypeId type, MotorBase * pMotor, MotorBase * pMotor2)
{
    CString statusStr;
    uint32_t status = 0;
    uint32_t status2 = 0;

    // TODO: if required or desired, use a switch statement to allow display of individual controller status
    if ( type == ControllerTypeId::ControllerTypeCarousel )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        if ( pTheta )
        {
            status = pTheta->GetBoardStatus();
        }
        statusStr.Format( _T( "0x%04X" ), status );
        carouselStatusStr = statusStr;
        CarouselStatusDisp.SetWindowText( statusStr );
    }
    else if ( type == ControllerTypeId::ControllerTypePlate )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        if ( pTheta )
        {
            status = pTheta->GetBoardStatus();
        }
        if ( pRadius )
        {
            status2 = pRadius->GetBoardStatus();
        }
        statusStr.Format( _T( "0x%04X / 0x%04X" ), status, status2 );
        plateStatusStr = statusStr;
        PlateStatusDisp.SetWindowText( statusStr );
    }
    else
    {
        if ( pMotor )
        {
            status = pMotor->GetBoardStatus();
            if ( pMotor2 )
            {
                status2 = pMotor2->GetBoardStatus();
                statusStr.Format(_T("0x%04X / 0x%04X"), status, status2);
            }
            else
            {
                statusStr.Format(_T("0x%04X"), status);
            }

            boardStatusStr = statusStr;
            BoardStatusDisp.SetWindowText( statusStr );
        }
    }
}

void CMotorControlDlg::SetGroupEnable( BOOL carouselEnable, BOOL plateEnable )
{
    CarouselGroup.EnableWindow( carouselEnable );
    CarouselCurrentPosLbl.EnableWindow( carouselEnable );
    CarouselTubeLbl.EnableWindow( carouselEnable );
    CarouselCurrentPosDisp.EnableWindow( carouselEnable );
    CarouselSetTubeEditDisp.EnableWindow( carouselEnable );
    CarouselNextTubeLbl.EnableWindow( carouselEnable );
    CarouselNextTubeBtn.EnableWindow( carouselEnable );
    CarouselFindTubeLbl.EnableWindow( carouselEnable );
    CarouselFindTubeBtn.EnableWindow( carouselEnable );
    CarouselSetTubeLbl.EnableWindow( carouselEnable );
    CarouselTubePlusBtn.EnableWindow( carouselEnable );
    CarouselTubeMinusBtn.EnableWindow( carouselEnable );
    CarouselGotoBtn.EnableWindow( carouselEnable );

    CarouselCntlrGrp.EnableWindow( carouselEnable );
    CarouselCntlrResetBtn.EnableWindow( carouselEnable );
    CarouselCntlrLoadHomeBtn.EnableWindow( carouselEnable );
    CarouselCntlrEjectBtn.EnableWindow( carouselEnable );
    CarouselCntlrInitBtn.EnableWindow( carouselEnable );
    CarouselCntlrCalBtn.EnableWindow( carouselEnable );

    ThetaSetTube1PosBtn.EnableWindow( carouselEnable );

    PlateGroup.EnableWindow( plateEnable );
    PlateCurrentPosLbl.EnableWindow( plateEnable );
    PlateCurrentRowLbl.EnableWindow( plateEnable );
    PlateCurrentRowDisp.EnableWindow( plateEnable );
    PlateCurrentColLbl.EnableWindow( plateEnable );
    PlateCurrentColDisp.EnableWindow( plateEnable );
    PlateSetRowLbl.EnableWindow( plateEnable );
    PlateSetRowEditDisp.EnableWindow( plateEnable );
    PlateRowPlusBtn.EnableWindow( plateEnable );
    PlateRowMinusBtn.EnableWindow( plateEnable );
    PlateSetColLbl.EnableWindow( plateEnable );
    PlateSetColEditDisp.EnableWindow( plateEnable );
    PlateColPlusBtn.EnableWindow( plateEnable );
    PlateColMinusBtn.EnableWindow( plateEnable );
    PlateGotoBtn.EnableWindow( plateEnable );

    PlateCntlrGrp.EnableWindow( plateEnable );
    PlateCntlrResetBtn.EnableWindow( plateEnable );
    PlateCntlrLoadHomeBtn.EnableWindow( plateEnable );
    PlateCntlrEjectBtn.EnableWindow( plateEnable );
    PlateCntlrInitBtn.EnableWindow( plateEnable );
    PlateCntlrCalBtn.EnableWindow( plateEnable );

    RadiusSetCenterPosBtn.EnableWindow( carouselEnable );
    RadiusSetTubePosBtn.EnableWindow( plateEnable );
}

void CMotorControlDlg::Quit()
{
#ifndef SIM_TEST
    // All done listening.
    pSignals_->cancel();

    DoAllControllers( false, true );

    DoAllControllers( true );

    Sleep( 100 );

    if ( infoInit )
    {
        if ( ptfilecfg )
        {
            ptfilecfg.reset();
        }

        if ( ptconfig )
        {
            ptconfig.reset();
        }
    }

    if ( pRadiusController )
    {
        pRadiusController.reset();
    }

    if ( pThetaController )
    {
        pThetaController.reset();
    }

    if ( pProbeController )
    {
        pProbeController.reset();
    }

    if ( pReagentController )
    {
        pReagentController.reset();
    }

    if ( pFocusController )
    {
        pFocusController.reset();
    }

    if ( pLedRackController )
    {
        pLedRackController.reset();
    }

    if ( pCarouselController )
    {
        pCarouselController.reset();
    }

    if ( pPlateController )
    {
        pPlateController.reset();
    }

    if ( pSignals_ )
    {
        pSignals_.reset();
    }

    if ( pCbi_ )
    {
        pCbi_->CancelQueue();
        pCbi_->CloseSerial();
        pCbi_->Close();
        pCbi_.reset();
    }
#endif // !SIM_TEST

    if ( pLocalWork_ )
    {
        pLocalWork_.reset();    // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
        pLocalIosvc_->stop();   // instruct io_service to stop processing (should exit ::run() and end thread.
        pLocalIosvc_.reset();   // Destroys the queue
    }

    if ( pIoThread_ )
    {
        pIoThread_.reset();
    }

    Logger::L().Flush();
}

void CMotorControlDlg::ShowMotorRegister( MotorBase * pMotor )
{
    std::string motorStr = "";
    uint32_t motorType = 0;
    std::stringstream ss;
    std::string statusStr;
    CString dispStr;
    MotorRegisters regs;

    if ( pMotor )
    {
        motorType = pMotor->GetMotorType();
        motorStr = pMotor->GetMotorTypeAsString( (MotorTypeId)motorType );
        pMotor->GetMotorRegs( regs );

        statusStr = "Motor Register Content:  ";
        statusStr.append( boost::str( boost::format( " Id: %u - %s\r\n" ) % motorType % motorStr ) );
        statusStr.append( boost::str( boost::format( "    Command: %-2u\t\t\tParam: %-8u\r\n" ) % regs.Command % regs.CommandParam ) );
        statusStr.append( boost::str( boost::format( "    ErrorCode: 0x%08X\r\n" ) % regs.ErrorCode ) );
        statusStr.append( boost::str( boost::format( "    Position: %-8d\t\t\tHomeDirection: %u\r\n" ) % regs.Position % regs.HomeDirection ) );
        statusStr.append( boost::str( boost::format( "    MotorFullStepsPerRev: %-4u\tUnitsPerRev: %-6u\r\n" ) % regs.MotorFullStepsPerRev % regs.UnitsPerRev ) );
        statusStr.append( boost::str( boost::format( "    GearheadRatio: %u\t\tEncoderTicksPerRev: %d\r\n" ) % regs.GearheadRatio % regs.EncoderTicksPerRev ) );
        statusStr.append( boost::str( boost::format( "    Deadband: %u\r\n" ) % regs.Deadband ) );
        statusStr.append( boost::str( boost::format( "    InvertedDirection: %u\t\tStepSize: %u\r\n" ) % regs.InvertedDirection % regs.StepSize ) );
        statusStr.append( boost::str( boost::format( "    Acceleration: %-10u\t\tDeAcceleration: %-10u\r\n" ) % regs.commonRegs.Acceleration % regs.commonRegs.Deceleration ) );
        statusStr.append( boost::str( boost::format( "    MinSpeed: %-8u\t\t\tMaxSpeed: %8u\r\n" ) % regs.MinSpeed % regs.commonRegs.MaxSpeed ) );
        statusStr.append( boost::str( boost::format( "    OverCurrent: %-8u\t\tStallCurrent: %-8u\r\n" ) % regs.OverCurrent % regs.StallCurrent ) );
        statusStr.append( boost::str( boost::format( "    HoldVoltageDivide: %-8u\tRunVoltageDivide: %-8u\r\n" ) % regs.HoldVoltageDivide % regs.commonRegs.RunVoltageDivide ) );
        statusStr.append( boost::str( boost::format( "    AccVoltageDivide: %-8u\t\tDecVoltageDivide: %-8u\r\n" ) % regs.commonRegs.AccVoltageDivide % regs.commonRegs.DecVoltageDivide ) );
        statusStr.append( boost::str( boost::format( "    CONFIG: %u\r\n" ) % regs.CONFIG ) );
        statusStr.append( boost::str( boost::format( "    INT_SPEED: %u\t\t\tST_SLP: %u\r\n" ) % regs.INT_SPEED % regs.ST_SLP ) );
        statusStr.append( boost::str( boost::format( "    FN_SLP_ACC: %u\t\t\tFN_SLP_DEC: %u\r\n" ) % regs.FN_SLP_ACC % regs.FN_SLP_DEC ) );
        statusStr.append( boost::str( boost::format( "    DelayAfterMove: %u\r\n" ) % regs.DelayAfterMove ) );
        statusStr.append( boost::str( boost::format( "    ProbeSpeed1: %-8u\t\tProbeCurrent1: %-8u\r\n" ) % regs.ProbeRegs.ProbeSpeed1 % regs.ProbeRegs.ProbeCurrent1 ) );
        statusStr.append( boost::str( boost::format( "    ProbeSpeed2: %-8u\t\tProbeCurrent2: %-8u\r\n" ) % regs.ProbeRegs.ProbeSpeed2 % regs.ProbeRegs.ProbeCurrent2 ) );
        statusStr.append( boost::str( boost::format( "    ProbeAbovePosition: %-8u\t\tProbeRaise: %-8u\r\n" ) % regs.ProbeRegs.ProbeAbovePosition % regs.ProbeRegs.ProbeRaise ) );
        statusStr.append( boost::str( boost::format( "    ProbeStopPosition: %-8u\r\n" ) % regs.ProbeStopPosition ) );
        statusStr.append( boost::str( boost::format( "    MaxTravelPosition: %-8u\r\n" ) % regs.MaxTravelPosition ) );

        dispStr = statusStr.c_str();
        CMotorStatusDisp.SetWindowText( dispStr );
    }
}

void CMotorControlDlg::SetCarouselTube1ThetaPos(bool doTheta)
{
    // disable holding currents
    if ( carouselSelected )
    {
#ifndef SIM_TEST
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        if ( ( pTheta ) && ( pProbe ) )
        {
            pProbe->Enable( false );        // disable the probe holding current to allow it to be raised if necessary
            pTheta->Enable( false );

            MessageBox( _T( "If required, EMPTY THE CAROUSEL AND RAISE THE PROBE, then rotate\r\nthe carousel left or right to place the center of tube #1 under the probe.\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Set the Carousel tube 1 position" ) );

            // re-enable holding currents
            pTheta->Enable( true );
            pProbe->Enable( true );

            if ( doTheta )
            {
                pThetaController->MarkPosAsZero();
            }
            else
            {
                pCarouselController->MarkCarouselPosAsZero();
            }
            DisplayBoardStatus( ControllerTypeId::ControllerTypeCarousel, pTheta, pProbe );
        }
#endif
        carouselPos = 1;
        carouselSetPos = 1;

        cntlEdit = true;

        CString valStr;
        valStr.Format( _T( "%d" ), carouselPos );
        CarouselCurrentPosDisp.SetWindowText( valStr );
        CarouselSetTubeEditDisp.SetWindowText( valStr );

        cntlEdit = false;
    }
}

void CMotorControlDlg::UpdateFocusPosition( void )
{
    pFocusController->ClearErrors();
    focusPos = pFocusController->Position();

    ShowFocusPosition( focusPos );
}

void CMotorControlDlg::ClearFocusPosition( void )
{
    cntlEdit = true;

    FocusCurrentPosDisp.Clear();
//    ShowFocusPosition( 0, true );

    cntlEdit = false;
}

void CMotorControlDlg::ShowFocusPosition( int32_t pos, bool clear )
{
    cntlEdit = true;

    CString valStr;
    valStr.Empty();
    if ( clear )
    {
        FocusCurrentPosDisp.Clear();
    }
    else
    {
        valStr.Format( _T( "%d" ), pos );
        FocusCurrentPosDisp.SetWindowText( valStr );
    }

    cntlEdit = false;
}

void CMotorControlDlg::UpdateRackPositions( void )
{
    if ( ( pLedRackController ) && ( pLedRackController->ControllerOk() ) )
    {
        pLedRackController->GetPositions( rack1Pos, rack2Pos );

        std::string pathStr = "";
        pLedRackController->GetPathStr( LEDRackController::Rack1, pathStr );
        rack1PathStr = pathStr.c_str();
        Rack1PathDisp.SetWindowText( rack1PathStr );
        pathStr.clear();
        pLedRackController->GetPathStr( LEDRackController::Rack2, pathStr );
        rack2PathStr = pathStr.c_str();
        Rack2PathDisp.SetWindowText( rack2PathStr );
    }
}

void CMotorControlDlg::DoCarouselSelect( bool msgWait )
{
    if ( msgWait )
    {
        MessageBox( _T( "SYSTEM WILL INITIALIZE CAROUSEL POSITIONING REFERENCE INFORMATION...\r\n\r\nENSURE CAROUSEL IS REMOVED TO PREVENT TUBE LOSS\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                    _T( "Initialize Carousel positioning" ) );
    }

    pCarouselController->SelectCarousel();      // initialize the motor parameters and initialize the controller's positioning references

    pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
    if ( pCarouselTheta )
    {
        int32_t units = 0;
        int32_t ratio = 0;
        time_t t_now = 0;
        time_t t_start = time( 0 );

        do
        {
            Sleep( 100 );
            units = pCarouselTheta->GetUnitsPerRev();
            ratio = pCarouselTheta->GetGearRatio();
            maxCarouselThetaPos = ( units * ratio );
            t_now = time( 0 );
        } while ( ( ( units == 0 ) || ( ratio == 0 ) ) && ( difftime( t_now, t_start ) < 5 ) );

        if ( maxCarouselThetaPos == 0 )
        {
            maxCarouselThetaPos = MaxThetaPosition;
        }
    }
}

void CMotorControlDlg::DoPlateSelect( bool msgWait )
{
    if ( msgWait )
    {
        MessageBox( _T( "SYSTEM WILL INITIALIZE PLATE POSITIONING REFERENCE INFORMATION...\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                    _T( "Initialize Plate positioning" ) );
    }

    pPlateController->SelectPlate();

    pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
    if ( pPlateTheta )
    {
        int32_t units = 0;
        int32_t ratio = 0;
        time_t t_now = 0;
        time_t t_start = time( 0 );

        do
        {
            Sleep( 100 );
            units = pPlateTheta->GetUnitsPerRev();
            ratio = pPlateTheta->GetGearRatio();
            maxPlateThetaPos = ( units * ratio );
            t_now = time( 0 );
        } while ( ( ( units == 0 ) || ( ratio == 0 ) ) && ( difftime( t_now, t_start ) < 5 ) );

        if ( maxPlateThetaPos == 0 )
        {
            maxPlateThetaPos = MaxThetaPosition;
        }
    }
}

void CMotorControlDlg::DoCarouselInit( bool msgWait )
{
    if ( carouselSelected )
    {
        if ( msgWait )
        {
            MessageBox( _T( "SYSTEM WILL INITIALIZE CAROUSEL POSITIONING REFERENCE INFORMATION...\r\n\r\nENSURE CAROUSEL IS REMOVED TO PREVENT TUBE LOSS\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Initialize Carousel positioning" ) );
        }

        pCarouselController->InitCarousel();        // initialize the motor parameters and initialize the controller's positioning references
    }
}

void CMotorControlDlg::DoPlateInit( bool msgWait )
{
    if ( plateSelected )
    {
        if ( msgWait )
        {
            MessageBox( _T( "SYSTEM WILL INITIALIZE PLATE POSITIONING REFERENCE INFORMATION...\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Initialize Plate positioning" ) );
        }

        pPlateController->InitPlate();              // initialize the motor parameters and initialize the controller's positioning references
    }
}

void CMotorControlDlg::ShowInfoDialog( int32_t msgType )
{
    int dlgResponse = IDCANCEL;
    CString msgStr;
    bool probeUp = false;

    if ( plateSelected )
    {
        probeUp = pPlateController->IsProbeUp();
        if ( !probeUp )
        {
            probeUp = pPlateController->ProbeUp();
        }
    }
    else if ( carouselSelected )
    {
        probeUp = pCarouselController->IsProbeUp();
        if ( !probeUp )
        {
            probeUp = pCarouselController->ProbeUp();
        }
    }

    if ( probeUp )
    {

        switch ( msgType )
        {
            case MsgBoxTypeThetaZero:
                if ( plateSelected )
                {
                    msgStr.Format( _T( "%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the theta mechanism to position the 'zero' position under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Theta Zero position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pPlateController->MarkThetaPosAsZero();
                    }
                }
                else
                {
                    msgStr.Format( _T( "%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the theta mechanism to position the tube one position under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Tube one position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pCarouselController->MarkCarouselPosAsZero();
                    }
                }
                break;

            case MsgBoxTypeRadiusZero:
                if ( plateSelected )
                {
                    msgStr.Format( _T( "%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the radius plate to position the 'zero' position under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Radius Zero position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pPlateController->MarkRadiusPosAsZero();
                    }
                }
                else
                {
                    msgStr.Format( _T( "%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the radius plate mechanism to place the center of a tube\r\n" ),
                                   _T( "position under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Carousel radius position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pCarouselController->MarkCarouselRadiusPos();
                    }
                }
                break;

            case MsgBoxTypeThetaCal:
                if ( plateSelected )
                {
                    msgStr.Format( _T( "%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the theta mechanism to position the calibration position\r\n" ),
                                   _T( "under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Theta Cal position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        int32_t thetaPos = 0;
                        int32_t radiusPos = 0;

                        pPlateController->PlatePosition( thetaPos, radiusPos );
                        plateThetaCalPos = thetaPos;
                        pPlateController->CalibrateThetaPosition();
                    }
                }
                break;

            case MsgBoxTypeRadiusCal:
                if ( plateSelected )
                {
                    msgStr.Format( _T( "%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Adjust the radius plate to position the center calibration position\r\n" ),
                                   _T( "under the probe.\r\n\r\n" ),
                                   _T( "Press 'Enter' or click/press the 'OK' button when done." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Radius Cal position" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        int32_t thetaPos = 0;
                        int32_t radiusPos = 0;

                        pPlateController->CalibratePlateCenter();
                        pPlateController->PlatePosition( thetaPos, radiusPos );
                        plateRadiusCenterPos = radiusPos;
                    }
                }
                break;

            case MsgBoxTypeCalibratePlate:
                if ( plateSelected )
                {
                    // this is purely for ease of readability and line length estimation...
                    // ensure the number of string parameters in the format string matches the actual number or instruction lines...
                    // use the following character guide to limit strings to 80 characters or less.  More characters cause wrap-around on the message box...
                    msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Calibration will be performed using multiple steps!\r\n\r\n" ),
                                   _T( "For all steps, the probe may be manually adjusted up or down as\r\n" ),
                                   _T( "necessary to confirm the positioning.\r\n\r\n" ),
                                   _T( "When each step is complete, press 'Enter' or click/press the 'OK' button\r\n" ),
                                   _T( "to accept the actions and move to the next step,\r\n" ),
                                   _T( "or 'Cancel' to abort the calibration." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Plate center position: Start" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pPlateController->HomeRadius();

                        msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "First, manually adjust the radius mechanism to position the probe\r\n" ),
                                       _T( "at the center calibration point of the plate calibration fixture.\r\n\r\n" ),
                                       _T( "Theta adjust may be made as necessary, but Theta calibration will occur\r\n" ),
                                       _T( "later in the calibration process.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done and ready to continue\r\n" ),
                                       _T( "to the next step, or 'Cancel' to abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate center position: Step 1" ), MB_OKCANCEL );
                    }

                    int32_t thetaPos = 0;
                    int32_t radiusPos = 0;

                    if ( dlgResponse == IDOK )
                    {
                        pPlateController->PlatePosition( thetaPos, radiusPos );
                        plateRadiusCenterPos = radiusPos;
                        pPlateController->CalibratePlateCenter();       // capture and mark the current position as the plate center zero point

                        pPlateController->MarkRadiusPosAsZero();        // now set the center to zero
                        Sleep( PositionUpdateDelay );

                        pPlateController->HomeTheta();

                        msgStr.Format( _T( "%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "Next, manually adjust the theta mechanism to position the probe\r\n" ),
                                       _T( "at the row alignment calibration point of the plate calibration fixture.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button to continue to the next step,\r\n" ),
                                       _T( "or 'Cancel' to abort the theta calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate row alignment: Step 2" ), MB_OKCANCEL );
                    }

                    if ( dlgResponse == IDOK )
                    {
                        pPlateController->PlatePosition( thetaPos, radiusPos );
                        plateThetaCalPos = thetaPos;
                        pPlateController->CalibrateThetaPosition();

                        pPlateController->MarkThetaPosAsZero();     // capture and mark the current position as the plate center zero point
                        Sleep( PositionUpdateDelay );

                        pPlateController->InitPlate();
                    }
                }
                break;

            case MsgBoxTypeCalibrateCarousel:
                if ( carouselSelected )
                {
                    msgStr.Format( _T( "%s%s%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
                                   _T( "Calibration will be performed in a single step!\r\n\r\n" ),
                                   _T( "During the calibration, the probe may be manually adjusted up or down\r\n" ),
                                   _T( "as necessary to confirm the positioning.\r\n\r\n" ),
                                   _T( "When calibration is complete, press 'Enter' or click/press the 'OK' button\r\n" ),
                                   _T( "to accept the calibration values, or 'Cancel' to abort the calibration." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Calibrate the Carousel radius and theta positions: Start" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        pCarouselController->HomeTheta();
                        pCarouselController->HomeRadius();

                        msgStr.Format( _T( "%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "Manually adjust the radius and theta mechanisms to position\r\n" ),
                                       _T( "the center of the tube one position under the probe.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button to accept the values\r\n" ),
                                       _T( "and complete the calilbration, or 'Cancel' to discard the values\r\n"),
                                       _T( "and abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Calibrate the Carousel radius and theta positions" ), MB_OKCANCEL );
                    }

                    if ( dlgResponse == IDOK )
                    {
                        Sleep( PositionUpdateDelay );
                        pCarouselController->CalibrateCarouselHome();

                        Sleep( PositionUpdateDelay );
                        pCarouselController->InitCarousel();
                    }
                }
                break;

            case MsgBoxTypeNone:
            default:
                break;
        }
    }
}


// CMotorControlDlg message handlers

BOOL CMotorControlDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT(( IDM_ABOUTBOX & 0xFFF0 ) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if ( pSysMenu != NULL )
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if ( !strAboutMenu.IsEmpty() )
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    boost::system::error_code ec;

    Logger::L().Log ( MODULENAME, severity_level::normal, "InitDialog: < enter >" );

    int idResponse = IDOK;
    if ( !cbiInit )
    {
        do
        {
            if ( !pCbi_->Initialize() )
            {
                Logger::L().Log ( MODULENAME, severity_level::normal, "No devices found." );
                idResponse = MessageBox( _T( "No devices found. Ensure all USB connections to the controller board are present,\r\nthen quit and restart the program, or press 'OK' to display the dialog for edit functions." ),
                                         _T( "No devices" ), MB_CANCELTRYCONTINUE | MB_SETFOREGROUND | MB_ICONEXCLAMATION );

                if ( idResponse == IDCANCEL )
                {
                    PostQuitMessage( -1 );
                    return TRUE;
                }
            }
            else
            {
                idResponse = IDOK;
            }
        } while ( ( idResponse != IDCONTINUE ) && ( idResponse != IDOK ) );
    }

    DoAllControllers();

    bool samplerOk = false;
    if ( ( pCarouselController ) && ( pCarouselController->ControllerOk() ) )
//        if ( ( pCarouselController ) && ( pCarouselController->ControllerOk() ) && ( pCarouselController->IsCarouselPresent() ) )
    {
        samplerOk = pCarouselController->IsCarouselPresent();
        if ( samplerOk )
        {
            carouselSelected = TRUE;
        }
        else
        {
            carouselSelected = FALSE;
        }
    }

    if ( ( !samplerOk ) && ( pPlateController ) && ( pPlateController->ControllerOk() ) )
//        if ( ( pPlateController ) && ( pPlateController->ControllerOk() ) && ( pPlateController->IsPlateActive() ) )
    {
        samplerOk = pPlateController->IsPlateActive();
        if ( samplerOk )
        {
            plateSelected = TRUE;
        }
        else
        {
            plateSelected = FALSE;
        }
    }

    if ( !samplerOk )
    {
        carouselSelected = TRUE;
    }

    CString valStr;

    cntlEdit = true;    // prevent re-entry caused by value updates

    valStr = "1";
    carouselSetPos = 1;
    carouselPos = 1;
    CarouselCurrentPosDisp.SetWindowText( valStr );
    CarouselSetTubeEditDisp.SetWindowText( valStr );

    plateSetCol = 1;
    plateCol = 1;
    PlateCurrentColDisp.SetWindowText( valStr );
    PlateSetColEditDisp.SetWindowText( valStr );

    valStr = "A";
    setRowStr = valStr;
    currentRowStr = valStr;
    PlateCurrentRowDisp.SetWindowText( valStr );
    PlateSetRowEditDisp.SetWindowText( valStr );
    plateRow = 0;
    plateSetRow = 0;

    valStr.Format( _T( "0x%04X" ), 0 );
    carouselStatusStr = valStr;
    CarouselStatusDisp.SetWindowText( valStr );
    boardStatusStr = valStr;
    BoardStatusDisp.SetWindowText( valStr );

    valStr.Format( _T( "0x%04X / 0x%04X" ), 0, 0 );
    plateStatusStr = valStr;
    PlateStatusDisp.SetWindowText( valStr );

    CarouselSelect.SetCheck( BST_UNCHECKED );
    carouselSelected = FALSE;
    PlateSelect.SetCheck( BST_UNCHECKED );
    plateSelected = FALSE;

    SetGroupEnable( carouselSelected, plateSelected );

    UpdateRackPositions();

    if ( ( carouselSelected ) && ( pCarouselController ) && ( pCarouselController->ControllerOk() ) )
    {
        CarouselSelect.SetCheck( BST_CHECKED );
        carouselSelected = TRUE;
        PlateSelect.SetCheck( BST_UNCHECKED );
        plateSelected = FALSE;
        DoCarouselSelect( false );      // ensure carousel is fully selected and initialized as the default sampler
    }
    else if ( ( plateSelected ) && ( pPlateController ) && ( pPlateController->ControllerOk() ) )
    {
        CarouselSelect.SetCheck( BST_UNCHECKED );
        carouselSelected = FALSE;
        PlateSelect.SetCheck( BST_CHECKED );
        plateSelected = TRUE;
        DoPlateSelect( false );
    }
    else
    {
        carouselSelected = FALSE;
        plateSelected = FALSE;
    }

    cntlEdit = false;

    Logger::L().Log ( MODULENAME, severity_level::normal, "InitDialog: < exit >" );
	
    return TRUE;  // return TRUE unless you set the focus to a control
}

void CMotorControlDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMotorControlDlg::OnPaint()
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
HCURSOR CMotorControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMotorControlDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnOK();
}



////////////////////////////////////////////////////////////////////////////////
// High-level actions
////////////////////////////////////////////////////////////////////////////////

void CMotorControlDlg::OnBnClickedCarouselSelect()
{
    if ( !cntlEdit )
    {
        BOOL prevSelect = carouselSelected;

        cntlEdit = true;    // prevent re-entry caused by control manipulation

        if ( plateSelected )
        {
            plateSelected = FALSE;
            PlateSelect.SetCheck( BST_UNCHECKED );
        }
        carouselSelected = TRUE;
        CarouselSelect.SetCheck( BST_CHECKED );
        SetGroupEnable( carouselSelected, plateSelected );

        if ( prevSelect != carouselSelected )
        {
            DoCarouselSelect( true );
        }

        cntlEdit = false;
    }
}

void CMotorControlDlg::OnBnClickedPlateSelect()
{
    if ( !cntlEdit )
    {
        BOOL prevSelect = plateSelected;

        cntlEdit = true;    // prevent re-entry caused by control manipulation

        if ( carouselSelected )
        {
            carouselSelected = FALSE;
            CarouselSelect.SetCheck( BST_UNCHECKED );
        }
        plateSelected = TRUE;
        PlateSelect.SetCheck( BST_CHECKED );
        SetGroupEnable( carouselSelected, plateSelected );

        if (prevSelect != plateSelected)
        {
            DoPlateSelect( true );
        }

        cntlEdit = false;
    }
}

void CMotorControlDlg::OnBnClickedCarouselNextTubeBtn()
{
    cntlEdit = true;    // prevent re-entry caused by spin position reset

#ifndef SIM_TEST
    pCarouselController->ClearErrors();
    if ( pCarouselController->GotoNextTube() > 0 )
#endif // !SIM_TEST
    {
        CString valStr;

#ifdef SIM_TEST
        carouselPos++;
        if ( carouselPos > MaxCarouselTubes )
        {
            carouselPos -= MaxCarouselTubes;     // to ensure the carousel always moves in the right direction (clockwise...)
        }
#else
        pCarouselController->ClearErrors();
        carouselPos = pCarouselController->GetCurrentTubeNum();
#endif // !SIM_TEST
        carouselSetPos = carouselPos;

        cntlEdit = true;
        valStr.Format(_T("%d"), carouselPos);
        CarouselCurrentPosDisp.SetWindowText( valStr );
        CarouselSetTubeEditDisp.SetWindowText( valStr );
        cntlEdit = false;
#ifndef SIM_TEST
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        DisplayBoardStatus(ControllerTypeId::ControllerTypeCarousel, pTheta);
#endif
    }

    cntlEdit = false;
}

void CMotorControlDlg::OnBnClickedCarouselFindTubeBtn()
{
    if ( carouselSelected )
    {
        pCarouselController->FindNextTube();
    }
}

void CMotorControlDlg::OnEnChangeCarouselTubeSetEdit()
{
    if ( carouselSelected )
    {
        if ( !cntlEdit )
        {
            cntlEdit = true;        // prevent re-entry caused by value update from other controls

            UpdateData( TRUE );     // update from the local variables

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedCarouselTubePlusBtn()
{
    if ( carouselSelected )
    {
        if ( !cntlEdit )
        {
            carouselSetPos++;
            if ( carouselSetPos > MaxCarouselTubes )
            {
                carouselSetPos -= MaxCarouselTubes;     // to ensure the carousel always moves in the right direction (clockwise...)
            }

            cntlEdit = true;        // prevent re-entry caused by spin position reset

            CString valStr;
            valStr.Format( _T( "%d" ), carouselSetPos );
            CarouselSetTubeEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}


void CMotorControlDlg::OnBnClickedCarouselTubeMinusBtn()
{
    if ( carouselSelected )
    {
        if ( !cntlEdit )
        {
            carouselSetPos--;
            if ( carouselSetPos <= 0 )
            {
                carouselSetPos += MaxCarouselTubes;     // to ensure the carousel always moves in the right direction (clockwise...)
            }

            cntlEdit = true;        // prevent re-entry caused by spin position reset

            CString valStr;
            valStr.Format( _T( "%d" ), carouselSetPos );
            CarouselSetTubeEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedCarouselTubeSetBtn()
{
    if ( carouselSelected )
    {
        int newPos = carouselSetPos;

        // set new carousel position...
        // allow the requested tube number to be the same as current to do a complete rotation...
        if ( carouselSetPos != carouselPos )
        {
            if ( newPos < carouselPos )
            {
                newPos += MaxCarouselTubes;     // to ensure the carousel always moves in the right direction (clockwise...)
            }
        }
        else
        {
            newPos += MaxCarouselTubes;         // to ensure the carousel always moves in the right direction (clockwise...)
        }

#ifndef SIM_TEST
        if ( pCarouselController->GotoTubeNum(newPos) > 0 )
#endif
        {
            CString valStr;

#ifdef SIM_TEST
            if ( newPos > MaxCarouselTubes )
            {
                newPos -= MaxCarouselTubes;
            }
            carouselPos = newPos;
#else
            carouselPos = pCarouselController->GetCurrentTubeNum();
#endif // SIM_TEST
            carouselSetPos = carouselPos;

            cntlEdit = true;
            valStr.Format(_T("%d"), carouselSetPos);
            CarouselCurrentPosDisp.SetWindowText( valStr );
            CarouselSetTubeEditDisp.SetWindowText( valStr );
            cntlEdit = false;
#ifndef SIM_TEST
            pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
            DisplayBoardStatus(ControllerTypeId::ControllerTypeCarousel, pTheta);
#endif
        }
    }
}

void CMotorControlDlg::OnBnClickedPlateRowPlusBtn()
{
    if ( plateSelected )
    {
        if ( !cntlEdit )
        {
            CString valStr = ( LPCWSTR ) "A";
            int setRow = plateSetRow;
            _TCHAR spinChar = 'A';

            plateSetRow = ( ++setRow % MaxPlateRowNum );
            spinChar += plateSetRow;
            valStr.Format( _T( "%c" ), spinChar );
            setRowStr = valStr;

            cntlEdit = true;    // prevent re-entry caused by spin position reset

            PlateSetRowEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedPlateRowMinusBtn()
{
    if ( plateSelected )
    {
        if ( !cntlEdit )
        {
            CString valStr = ( LPCWSTR ) "A";
            uint32_t setRow = plateSetRow + ( MaxPlateRowNum - 1 );
            _TCHAR spinChar = 'A';

            plateSetRow = ( setRow % MaxPlateRowNum );
            spinChar += plateSetRow;
            valStr.Format( _T( "%c" ), spinChar );
            setRowStr = valStr;

            cntlEdit = true;    // prevent re-entry caused by spin position reset

            PlateSetRowEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedPlateColPlusBtn()
{
    if ( plateSelected )
    {
        if ( !cntlEdit )
        {
            CString valStr;
            int setCol = plateSetCol + 1;

            if ( setCol > MaxPlateColNum )
            {
                setCol -= MaxPlateColNum;
            }
            plateSetCol = setCol;
            valStr.Format( _T( "%d" ), setCol );

            cntlEdit = true;    // prevent re-entry caused by spin position reset

            PlateSetColEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedPlateColMinusBtn()
{
    if ( plateSelected )
    {
        if ( !cntlEdit )
        {
            CString valStr;
            int setCol = plateSetCol - 1;

            if ( setCol <= 0 )
            {
                setCol += MaxPlateColNum;
            }
            plateSetCol = setCol;
            valStr.Format( _T( "%d" ), setCol );

            cntlEdit = true;    // prevent re-entry caused by spin position reset

            PlateSetColEditDisp.SetWindowText( valStr );

            cntlEdit = false;
        }
    }
}

void CMotorControlDlg::OnBnClickedPlateRowColSetBn()
{
    if ( plateSelected )
    {
        // set new carousel position....
        if ( ( plateSetRow != plateRow ) || ( plateSetCol != plateCol ) )
        {
#ifndef SIM_TEST
            if ( pPlateController->SetRowCol( plateSetRow, plateSetCol ) )
#endif // !SIM_TEST
            {
                CString valStr;
                _TCHAR rowChar = 'A';

                plateRow = plateSetRow;
                rowChar += plateSetRow;
                valStr.Format( _T( "%c" ), rowChar );
                currentRowStr = valStr;
                setRowStr = valStr;
                plateCol = plateSetCol;

                cntlEdit = true;

                PlateCurrentRowDisp.SetWindowText( valStr );
                PlateSetRowEditDisp.SetWindowText( valStr );

                valStr.Format( _T( "%d" ), plateSetCol );
                PlateCurrentColDisp.SetWindowText( valStr );
                PlateSetColEditDisp.SetWindowText( valStr );
#ifndef SIM_TEST
                pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
                DisplayBoardStatus( ControllerTypeId::ControllerTypePlate, pTheta, pRadius );
#endif
                cntlEdit = false;
            }
        }
    }
}

void CMotorControlDlg::OnBnClickedSampleProbeUp()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->ProbeUp();
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->ProbeUp();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeDn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->ClearErrors();
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->ProbeDown();
    }
    else if ( plateSelected )
    {
        pPlateController->ClearErrors();
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->ProbeDown();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentArmUp()
{
#ifndef SIM_TEST
//    if ( !pReagentController->IsUp() )
    {
        pReagentController->ClearErrors();
        pReagentController->ArmUp();
        DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
    }
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentArmDn()
{
#ifndef SIM_TEST
    if ( !pReagentController->IsDown() )
    {
        pReagentController->ArmDown();
        DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
    }
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFineFocusCntlUpBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepUpFine();
    UpdateFocusPosition();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFineFocusCntlDnBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepDnFine();
    UpdateFocusPosition();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedCoarseFocusCntlUpBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepUpCoarse();
    UpdateFocusPosition();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedCoarseFocusCntlDnBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepDnCoarse();
    UpdateFocusPosition();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFastFocusCntlUpBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepUpFast();
    UpdateFocusPosition();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFastFocusCntlDnBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusStepDnFast();
    UpdateFocusPosition();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFocusMotorResetBtn()
{
#ifndef SIM_TEST
    pFocusController->Reset();
    UpdateFocusPosition();
    MotorBase * pMotor = pFocusController->GetMotor();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pMotor );
    ShowMotorRegister( pMotor );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFocusMotorHomeBtn()
{
#ifndef SIM_TEST
    ClearFocusPosition();
    pFocusController->FocusHome();
    UpdateFocusPosition();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFocusClearErrorsBtn()
{
#ifndef SIM_TEST
    pFocusController->ClearErrors();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFocusCntlrStatusBtn()
{
#ifndef SIM_TEST
    ShowMotorRegister( pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedFocusCntlrStopBtn()
{
#ifndef SIM_TEST
    pFocusController->Stop();
    UpdateFocusPosition();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnEnChangeFocusPosSetEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;

        cntlEdit = true;        // prevent re-entry caused by value update from other controls

        FocusPosSetEditDisp.GetWindowText( valStr );
        if ( valStr.GetLength() > 0 )
        {
            std::string posStr = CT2A( valStr.GetString() );
            bool showText = false;

            if ( posStr.length() > 0 )
            {
                int32_t posVal = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &posVal );
                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), posVal );
                    MessageBox( valStr, _T( "Illegal Focus Position" ) );
                    showText = true;
                }
                else
                {
                    if ( ( posVal > focusMaxTravel ) || ( posVal < 0 ) )
                    {
                        valStr.Format( _T( "Value out of range: %d\r\n(0 - %d)" ), posVal, focusMaxTravel );
                        MessageBox( valStr, _T( "Illegal Focus Position" ) );
                        showText = true;
                    }
                    else
                    {
                        focusSetPos = posVal;
                    }
                }

                if ( showText )
                {
                    if ( FocusPosSetEditDisp.CanUndo() )
                    {
                        FocusPosSetEditDisp.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), focusSetPos );
                        FocusPosSetEditDisp.SetWindowText( valStr );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void CMotorControlDlg::OnBnClickedFocusPosSetBtn()
{
    int newPos = focusSetPos;

    if ( newPos > focusMaxTravel )
    {
        newPos = focusMaxTravel;                // limit position entries to the expected allowable range
    }
    else if ( newPos < 0 )
    {
        newPos = 0;
    }

    pFocusController->ClearErrors();
    int32_t focusPos = pFocusController->Position();
    // set focus position...
    if ( newPos != focusPos )
    {
#ifndef SIM_TEST
        ClearFocusPosition();
        pFocusController->ClearErrors();
        pFocusController->SetPosition( newPos );
#endif
        UpdateFocusPosition();

#ifndef SIM_TEST
        DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif
        cntlEdit = false;
    }
}

void CMotorControlDlg::OnBnClickedFocusPosSetCenterBtn()
{
    int newPos = focusMaxTravel/2;

    pFocusController->ClearErrors();
    int32_t focusPos = pFocusController->Position();
    // set focus position...
    if ( newPos != focusPos )
    {
#ifndef SIM_TEST
        ClearFocusPosition();
        pFocusController->ClearErrors();
        pFocusController->SetPosition( newPos );
#endif
        UpdateFocusPosition();
#ifndef SIM_TEST
        DisplayBoardStatus( ControllerTypeId::ControllerTypeFocus, pFocusController->GetMotor() );
#endif
    }
}



////////////////////////////////////////////////////////////////////////////////
// Controller configuration
////////////////////////////////////////////////////////////////////////////////

void CMotorControlDlg::OnBnClickedCarouselCntlrResetBtn()
{
#ifndef SIM_TEST
    pCarouselController->Reset();
    pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    DisplayBoardStatus(ControllerTypeId::ControllerTypeCarousel, pTheta, pProbe);
    ShowMotorRegister( pTheta );
//    ShowMotorRegister( pProbe );
#endif
    carouselPos = 1;
    carouselSetPos = 1;

    cntlEdit = true;

    CString valStr;
    valStr.Format( _T( "%d" ), carouselPos );
    CarouselCurrentPosDisp.SetWindowText( valStr );
    CarouselSetTubeEditDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void CMotorControlDlg::OnBnClickedCarouselCntlrLoadHomeBtn()
{
#ifndef SIM_TEST
    pCarouselController->LoadCarousel();
    pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeCarousel, pTheta, pProbe );
#endif
    carouselPos = 1;
    carouselSetPos = 1;

    CString valStr;
    valStr.Format( _T( "%d" ), carouselPos );
    CarouselCurrentPosDisp.SetWindowText( valStr );
    CarouselSetTubeEditDisp.SetWindowText( valStr );
}

void CMotorControlDlg::OnBnClickedCarouselCntlrEjectBtn()
{
#ifndef SIM_TEST
    pCarouselController->EjectCarousel();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedCarouselCntlrInitBtn()
{
    DoCarouselInit();
}

void CMotorControlDlg::OnBnClickedCarouselCntlrCalBtn()
{
#ifndef SIM_TEST
    ShowInfoDialog( MsgBoxTypeCalibrateCarousel );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedPlateCntlrResetBtn()
{
#ifndef SIM_TEST
    pPlateController->Reset();
    pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    DisplayBoardStatus( ControllerTypeId::ControllerTypePlate, pTheta, pRadius );
    ShowMotorRegister( pTheta );
//    ShowMotorRegister( pRadius );
//    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedPlateCntlrLoadHomeBtn()
{
#ifndef SIM_TEST
    pPlateController->HomePlate();
    pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    DisplayBoardStatus(ControllerTypeId::ControllerTypePlate, pTheta, pRadius);
#endif // !SIM_TEST
    CString valStr;
    _TCHAR rowChar = 'A';

    plateRow = 0;
    plateSetRow = 0;
    plateCol = 1;
    plateSetCol = 1;

    rowChar += plateSetRow;
    valStr.Format( _T( "%c" ), rowChar );
    currentRowStr = valStr;
    setRowStr = valStr;

    cntlEdit = true;
    PlateCurrentRowDisp.SetWindowText( valStr );
    PlateSetRowEditDisp.SetWindowText( valStr );

    valStr.Format( _T( "%d" ), plateSetCol );
    PlateCurrentColDisp.SetWindowText( valStr );
    PlateSetColEditDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void CMotorControlDlg::OnBnClickedPlateCntlrEjectBtn()
{
#ifndef SIM_TEST
    pPlateController->EjectPlate();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedPlateCntlrInitBtn()
{
    DoPlateInit();
}

void CMotorControlDlg::OnBnClickedPlateCntlrCalBtn()
{
#ifndef SIM_TEST
    ShowInfoDialog( MsgBoxTypeCalibratePlate );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrResetBtn()
{
#ifndef SIM_TEST
    pLedRackController->Reset();
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
    ShowMotorRegister( pRack1 );
//    ShowMotorRegister( pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrHomeBtn()
{
#ifndef SIM_TEST
    pLedRackController->HomeRacks();
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1SetTopBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathTop );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1CalTopBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack1TopPos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1SetCtrBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathMiddle );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1CalCtrBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack1MiddlePos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1SetBotBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathBottom );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1CalBotBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack1BottomPos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack1SetHomeBtn()
{
#ifndef SIM_TEST
    pLedRackController->HomeRack( LEDRackController::Rack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2SetTopBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathTop );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2CalTopBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack2TopPos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2SetCtrBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathMiddle );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2CalCtrBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack2MiddlePos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2SetBotBtn()
{
#ifndef SIM_TEST
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathBottom );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2CalBotBtn()
{
#ifndef SIM_TEST
    pLedRackController->CalibrateRack2BottomPos();
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedCntlrRack2SetHomeBtn()
{
#ifndef SIM_TEST
    pLedRackController->HomeRack( LEDRackController::Rack2 );
#endif // !SIM_TEST
}



////////////////////////////////////////////////////////////////////////////////
// low-level motor configuration
////////////////////////////////////////////////////////////////////////////////

void CMotorControlDlg::OnBnClickedSampleProbeResetBtn()
{
#ifndef SIM_TEST
    pProbeController->Reset();
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if (plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeHomeBtn()
{
#ifndef SIM_TEST
    pProbeController->CalibrateHome();
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

// use the current probe position as the 'above' position for probe movement speed transitioning
// preserves and uses the current 'raise' value...
void CMotorControlDlg::OnBnClickedSampleProbeSetAboveBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    // disable the holding current
    pProbe->Enable(false);

    MessageBox( _T( "If the probe is not already in the correct position, press 'ESC' and\r\nuse the probe Up and Down buttons to position the probe at the point\r\nwhere the tube bottom search should slow, then press the 'OK' button.\r\n" ),
                _T( "Set the Probe slow point" ) );

    // re-enable the holding current
    pProbe->Enable(true);

    if ( carouselSelected )
    {
        pCarouselController->ProbePosition( probePos );
        pCarouselController->GetProbeStopPositions( probeAbovePos, probeRaisePos );
        pCarouselController->AdjustProbeStop( probePos, probeRaisePos );     // note use of current value of probeRaisePos
    }
    else if ( plateSelected )
    {
        pPlateController->ProbePosition( probePos );
        pPlateController->GetProbeStopPositions( probeAbovePos, probeRaisePos );
        pPlateController->AdjustProbeStop( probePos, probeRaisePos );     // note use of current value of probeRaisePos
    }
    DisplayBoardStatus(ControllerTypeId::ControllerTypeProbe, pProbe);
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

// use the current probe position as the 'raise' position for probe position above the tube bottom
// preserves and uses the current 'above' value...
void CMotorControlDlg::OnBnClickedSampleProbeSetRaiseBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    // disable the holding current
    pProbe->Enable( false );

    MessageBox( _T( "If the probe is not already in the correct position, press 'ESC' and\r\nuse the probe Up and Down buttons to position the probe at the point\r\nwhere the probe will aspirate, then press the 'OK' button.\r\n" ),
                _T( "Set the Probe aspiration point" ) );

    // re-enable the holding current
    pProbe->Enable( true );

    if ( carouselSelected )
    {
        pCarouselController->ProbePosition( probePos );
        pCarouselController->GetProbeStopPositions( probeAbovePos, probeRaisePos );
        pCarouselController->AdjustProbeStop( probeAbovePos, probePos );     // note use of current value of probeAbovePos
    }
    else if ( plateSelected )
    {
        pPlateController->ProbePosition( probePos );
        pPlateController->GetProbeStopPositions( probeAbovePos, probeRaisePos );
        pPlateController->AdjustProbeStop( probeAbovePos, probePos );     // note use of current value of probeAbovePos
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeAdjustUpBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->ProbeStepUp( 1000 );       // adjust position by 0.1 mm
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->ProbeStepUp( 1000 );          // adjust position by 0.1 mm
    }
    else
    {
        return;
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeAdjustDnBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->ProbeStepDn( 1000 );       // adjust position by 0.1 mm
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->ProbeStepDn( 1000 );          // adjust position by 0.1 mm
    }
    else
    {
        return;
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeClearErrorsBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    pProbe->ClearErrors();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeCntlrStatusBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedSampleProbeCntlrStopBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    pProbe->Stop(0);    // always do a 'soft' stop...
    DisplayBoardStatus( ControllerTypeId::ControllerTypeProbe, pProbe );
    ShowMotorRegister( pProbe );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedThetaMotorResetBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    pTheta->Reset();    // discrete motor stop
    DisplayBoardStatus(ControllerTypeId::ControllerTypeTheta, pTheta);
    ShowMotorRegister( pTheta );
#endif
    carouselPos = 1;
    carouselSetPos = 1;

    cntlEdit = true;

    CString valStr;
    valStr.Format( _T( "%d" ), carouselPos );
    CarouselCurrentPosDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void CMotorControlDlg::OnBnClickedThetaMotorHomeBtn()
{
    CString valStr;

    cntlEdit = true;

    if ( carouselSelected )
    {
#ifndef SIM_TEST
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->HomeTheta();
        DisplayBoardStatus( ControllerTypeId::ControllerTypeTheta, pTheta );
#endif
        carouselPos = 1;
        carouselSetPos = 1;

        valStr.Format( _T( "%d" ), carouselPos );
        CarouselCurrentPosDisp.SetWindowText( valStr );
        CarouselSetTubeEditDisp.SetWindowText( valStr );
    }
    else if ( plateSelected )
    {
#ifndef SIM_TEST
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->HomeTheta();
        DisplayBoardStatus( ControllerTypeId::ControllerTypeTheta, pTheta );
#endif
        plateRow = 0;
        plateSetRow = 0;
        plateCol = 0;
        plateSetCol = 0;

        valStr.Empty();
        currentRowStr = valStr;
        setRowStr = valStr;

        PlateCurrentRowDisp.SetWindowText( valStr );
        PlateSetRowEditDisp.SetWindowText( valStr );
        PlateCurrentColDisp.SetWindowText( valStr );
        PlateSetColEditDisp.SetWindowText( valStr );
    }
    ShowMotorRegister( pTheta );

    cntlEdit = false;
}

void CMotorControlDlg::OnBnClickedThetaMotorSetTube1PosBtn()
{
    SetCarouselTube1ThetaPos( true );
}

// use the theta motor in the carousel controller to adjust position by 1/2 degree...
// will have the same effect on either plate or carousel
void CMotorControlDlg::OnBnClickedThetaAdjustLeftBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->StepThetaLeft();        // to adjust by 1 degree...
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->Stop();
        pPlateController->StepThetaLeft();           // to adjust by 1 degree...
    }
    else
    {
        return;
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeTheta, pTheta );
    ShowMotorRegister( pTheta );
#endif
}

// use the theta motor in the carousel controller to adjust position by 1/2 degree...
// will have the same effect on either plate or carousel
void CMotorControlDlg::OnBnClickedThetaAdjustRightBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pCarouselController->StepThetaRight();       // to adjust by 1 degree...
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pPlateController->StepThetaRight();          // to adjust by 1 degree...
    }
    else
    {
        return;
    }
    DisplayBoardStatus(ControllerTypeId::ControllerTypeTheta, pTheta);
#endif
}

void CMotorControlDlg::OnBnClickedThetaClearErrorsBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    pTheta->ClearErrors();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeTheta, pTheta );
    ShowMotorRegister( pTheta );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedThetaCntlrStatusBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    ShowMotorRegister( pTheta );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedThetaCntlrStopBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        return;
    }
    pTheta->Stop(0);
    DisplayBoardStatus(ControllerTypeId::ControllerTypeTheta, pTheta);
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusMotorResetBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
    }
    pRadius->Reset();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeRadius, pRadius );
    ShowMotorRegister( pRadius );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusMotorHomeBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
    }
    pRadius->Home();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeRadius, pRadius);
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusMotorSetTubePosBtn()
{
#ifndef SIM_TEST
    if ( carouselSelected )
    {
        pCarouselController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
        pRadius = pRadiusController->GetMotor();
        if ( ( pTheta ) && ( pProbe ) && ( pRadius ) )
        {
            // disable holding currents
            pProbe->Enable( false );
//            pTheta->Enable( false );
            pRadius->Enable( false );

            MessageBox( _T( "If required, RAISE THE PROBE then slide the carousel in or out to position\r\nand center a tube under the probe, then press 'Enter' or click/press the 'OK' button." ),
                        _T( "Set the Carousel radius tube position" ) );

            // re-enable holding currents
//            pProbe->Enable( true );
//            pTheta->Enable( true );
            pRadius->Enable( true );
            
            pRadiusController->SetTubePosAsCurrent();
            DisplayBoardStatus( ControllerTypeId::ControllerTypeRadius, pRadius );
        }
    }
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusMotorSetPlateCenterPosBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->MarkPlateCenter();
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
        pRadiusController->SetPlateCenterPosAsCurrent();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeRadius, pRadius );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusAdjustInBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->StepRadiusIn();
        DisplayBoardStatus( ControllerTypeId::ControllerTypeRadius, pRadiusController->GetMotor() );
    }
    else
    {
        pRadiusController->StepRadiusIn();
    }
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusAdjustOutBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->StepRadiusOut();
        DisplayBoardStatus( ControllerTypeId::ControllerTypeRadius, pRadiusController->GetMotor() );
    }
    else
    {
        pRadiusController->StepRadiusOut();
    }
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusClearErrorsBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
    }
    pRadius->ClearErrors();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeRadius, pRadius);
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusCntlrStatusBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
    }
    ShowMotorRegister( pRadius );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedRadiusCntlrStopBtn()
{
#ifndef SIM_TEST
    if ( plateSelected )
    {
        pPlateController->GetMotors( true, pTheta, true, pRadius, true, pProbe );
    }
    else
    {
        pRadius = pRadiusController->GetMotor();
    }
    pRadius->Stop(0);
    DisplayBoardStatus(ControllerTypeId::ControllerTypeRadius, pRadius);
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentArmResetBtn()
{
#ifndef SIM_TEST
    pReagentController->Reset();
    MotorBase * pMotor = pReagentController->GetMotor();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeReagent, pMotor );
    ShowMotorRegister( pMotor );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentArmHomeBtn()
{
#ifndef SIM_TEST
    pReagentController->ArmHome();
    MotorBase * pMotor = pReagentController->GetMotor();
    DisplayBoardStatus( ControllerTypeId::ControllerTypeReagent, pMotor );
    ShowMotorRegister( pMotor );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentAdjustUpBtn()
{
#ifndef SIM_TEST
    pReagentController->ArmStepUp();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentAdjustDnBtn()
{
#ifndef SIM_TEST
    pReagentController->ArmStepDn();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentClearErrorsBtn()
{
#ifndef SIM_TEST
    pReagentController->ClearErrors();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentCntlrStatusBtn()
{
#ifndef SIM_TEST
    ShowMotorRegister( pReagentController->GetMotor() );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedReagentCntlrStopBtn()
{
#ifndef SIM_TEST
    pReagentController->Stop();
    DisplayBoardStatus(ControllerTypeId::ControllerTypeReagent, pReagentController->GetMotor());
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1MotorResetBtn()
{
#ifndef SIM_TEST
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        pRack1->Reset();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
    ShowMotorRegister( pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1MotorHomeBtn()
{
#ifndef SIM_TEST
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        pRack1->Home();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1AdjustUpBtn()
{
#ifndef SIM_TEST
    int32_t rackpos = 0;
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        rackpos = pRack1->GetPosition();
        rackpos += (pRack1->GetDeadband() * 2);
        if ( rackpos > pRack1->GetMaxTravel() )
        {
            rackpos = pRack1->GetMaxTravel();
        }
        pRack1->SetPosition( rackpos );
//        pRack1Controller->AdjustRackUp();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1AdjustDnBtn()
{
#ifndef SIM_TEST
    int32_t rackpos = 0;
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        rackpos = pRack1->GetPosition();
        rackpos -= ( pRack1->GetDeadband() * 2 );
        if ( rackpos < 0 )
        {
            rackpos = 0;
        }
        pRack1->SetPosition( rackpos );
//        pRack1Controller->AdjustRackDn();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1ClearErrorsBtn()
{
#ifndef SIM_TEST
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        pRack1->ClearErrors();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1MotorStatusBtn()
{
#ifndef SIM_TEST
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    ShowMotorRegister( pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack1MotorStopBtn()
{
#ifndef SIM_TEST
    if ( pRack1 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack1 )
    {
        pRack1->Stop(HardStop);
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack1 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2MotorResetBtn()
{
#ifndef SIM_TEST
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        pRack2->Reset();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
    ShowMotorRegister( pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2MotorHomeBtn()
{
#ifndef SIM_TEST
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        pRack2->Home();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
    ShowMotorRegister( pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2AdjustUpBtn()
{
#ifndef SIM_TEST
    int32_t rackpos = 0;
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        rackpos = pRack2->GetPosition();
        rackpos += ( pRack2->GetDeadband() * 2 );
        if ( rackpos > pRack2->GetMaxTravel() )
        {
            rackpos = pRack2->GetMaxTravel();
        }
        pRack2->SetPosition( rackpos );
//        pRack2Controller->AdjustRackUp();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2AdjustDnBtn()
{
#ifndef SIM_TEST
    int32_t rackpos = 0;
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        rackpos = pRack2->GetPosition();
        rackpos -= ( pRack2->GetDeadband() * 2 );
        if ( rackpos < 0 )
        {
            rackpos = 0;
        }
        pRack2->SetPosition( rackpos );
//        pRack2Controller->AdjustRackDn();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2ClearErrorsBtn()
{
#ifndef SIM_TEST
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        pRack2->ClearErrors();
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2MotorStatusBtn()
{
#ifndef SIM_TEST
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    ShowMotorRegister( pRack2 );
#endif // !SIM_TEST
}

void CMotorControlDlg::OnBnClickedLedrack2MotorStopBtn()
{
#ifndef SIM_TEST
    if ( pRack2 == NULL )
    {
        pLedRackController->GetMotors( pRack1, pRack2 );
    }
    if ( pRack2 )
    {
        pRack2->Stop( HardStop );
    }
    DisplayBoardStatus( ControllerTypeId::ControllerTypeLEDRack, pRack2 );
#endif // !SIM_TEST
}

