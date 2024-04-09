
// MotorSysTestDlg.cpp : implementation file
//

#include "stdafx.h"

#include <ctime>
#include <chrono>
#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "MotorSysTest.h"
#include "MotorSysTestDlg.h"

#include "Logger.hpp"

#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define USER_API


static const char MODULENAME[]                  = "MotorSysTestDlg";
static const char CAROUSELCONTROLLERNODENAME[]  = "carousel_controller";
static const char FOCUSCONTROLLERNODENAME[]     = "focus_controller";
static const char LEDRACKCONTROLLERNODENAME[]   = "ledrack_controller";
static const char PLATECONTROLLERNODENAME[]     = "plate_controller";
static const char REAGENTCONTROLLERNODENAME[]   = "reagent_controller";


const std::string MotorSysTestDlg::CmdSimModeStr            = "Sim";    // toggle script execution simulation mode (no hardware actions)
const std::string MotorSysTestDlg::CmdInitPlateStr          = "Ip";     // Initialize Plate: home both theta and radius plate motors
const std::string MotorSysTestDlg::CmdInitCarouselStr       = "Ic";     // Initialize Carousel: home the carousel; may not be the tube 1 position
const std::string MotorSysTestDlg::CmdSelectPlateStr        = "Sp";     // Select Plate: initialize and apply plate motor control parameters for all following commands
const std::string MotorSysTestDlg::CmdSelectCarouselStr     = "Sc";     // Select Carousel: initialize and apply carousel motor control parameters for all following commands
const std::string MotorSysTestDlg::CmdHomeThetaStr          = "Ht";     // Home the Theta motor
const std::string MotorSysTestDlg::CmdHomeRadiusStr         = "Hr";     // Home the Radius motor
const std::string MotorSysTestDlg::CmdMoveThetaStr          = "Mt";     // move the Theta motor to the specified absolute location
const std::string MotorSysTestDlg::CmdMoveThetaRelStr       = "Rt";     // move the Theta motor to the specified relative location
const std::string MotorSysTestDlg::CmdMoveRadiusStr         = "Mr";     // move the Radius motor to the specified absolute location
const std::string MotorSysTestDlg::CmdMoveRadiusRelStr      = "Rr";     // move the Radius motor to the specified relative location
const std::string MotorSysTestDlg::CmdMoveRThetaStr         = "Mx";     // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
const std::string MotorSysTestDlg::CmdMoveRThetaRelStr      = "Rx";     // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
const std::string MotorSysTestDlg::CmdMoveCarouselStr       = "Xt";     // move carousel to tube number: '0' for next tube, 1-24 for discreet tube numbers
const std::string MotorSysTestDlg::CmdMovePlateWellStr      = "Xw";     // move the plate to the specified row (p1) /col (p2) location
const std::string MotorSysTestDlg::CmdMovePlateStr          = "Xx";     // Move plate to absolute theta (p1), radius (p2) position
const std::string MotorSysTestDlg::CmdMovePlateRelStr       = "Xr";     // Move to relative theta (p1), radius (p2) position
const std::string MotorSysTestDlg::CmdProbeHomeStr          = "Ph";     // home the probe
const std::string MotorSysTestDlg::CmdProbeMoveStr          = "P";      // move the probe: '0' for 'Up', and '1' for 'down'
const std::string MotorSysTestDlg::CmdWaitForKeyPressStr    = "K";      // Wait for a keypress.
const std::string MotorSysTestDlg::CmdSleepStr              = "D";      // insert a delay into the command stream processing
const std::string MotorSysTestDlg::CmdExitStr               = "E";      // Stop processing the run list and exit the program.



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
    virtual void DoDataExchange( CDataExchange* pDX );    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx( IDD_ABOUTBOX )
{
}

void CAboutDlg::DoDataExchange( CDataExchange* pDX )
{
    CDialogEx::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CAboutDlg, CDialogEx )
END_MESSAGE_MAP()


// MotorSysTestDlg dialog



MotorSysTestDlg::MotorSysTestDlg( CWnd* pParent /*=NULL*/ )
    : CDialogEx( IDD_MOTOR_SYS_TEST_DIALOG, pParent )
    , inited_( false )
    , cbiInit( false )
    , quitting( false )
    , timerComplete( false )
    , allowEditing( false )
    , simMode( false )
    , infoInit( false )
    , calInit( false )
    , doRelativeMove( false )
    , showInitErrors( false )
    , signalUpdateReady( false )
    , signalUpdateStarting( false )
    , signalUpdateRunning( false )
    , instrumentType( MotorSysTestDlg::InstrumentTypeUnknown )
    , moduleConfigFile( "MotorSysTest.info" )
    , motorConfigFile( "MotorControl.info" )
    , calConfigFile( "ControllerCal.info" )
    , ScriptEditBuf( _T( "" ) )
    , ScriptPath( _T( "" ) )
    , cntlEdit( true )
    , doorClosed( FALSE )
    , packPresent( FALSE )
    , reagentHome( FALSE )
    , reagentLimit( FALSE )
    , thetaHome( FALSE )
    , radiusHome( FALSE )
    , probeHome( FALSE )
    , focusHome( FALSE )
    , rack1Home( FALSE )
    , rack2Home( FALSE )
    , plateFound( FALSE )
    , tubePresent( FALSE )
    , thetaEditVal( 0 )
    , thetaVal( 0 )
    , radiusEditVal( 0 )
    , radiusVal( 0 )
    , focusEditVal( 0 )
    , focusVal( 0 )
    , rack1Pos( LEDRackController::PathUnk )
    , rack1PathStr( _T( "" ) )
    , rack2Pos( LEDRackController::PathUnk )
    , rack2PathStr( _T( "" ) )
    , rowEditStr( _T( "A" ) )
    , rowEditVal( 1 )
    , rowCharStr( _T( "A" ) )
    , rowVal( 0 )
    , colEditVal( 1 )
    , colVal( 0 )
    , tubeEditVal( 1 )
    , tubeNum( 0 )
    , probeVal( 0 )
    , scrollIdx( 0 )
    , cmdStepLine( 0 )
    , scriptLines( 0 )
    , stopRun( true )
    , scriptStarting( false )
    , scriptRunning( false )
    , scriptPaused( false )
    , errorCnt( 0 )

    , plateSelected( FALSE )
    , plateControlsInited( false )
    , plateBacklashChanged( false )
    , maxPlateThetaPos( MaxThetaPosition )
    , platePositionTolerance( DefaultPositionTolerance )
    , plateThetaCalPos( (int32_t)DefaultPlateThetaCalPos )              // value for the theta calibration position in 1/10 degree user units * the gearing factor
    , plateThetaHomePosOffset( 0 )
    , plateThetaA1Pos( 0 )
    , plateThetaA1PosOffset( 0 )
    , plateRadiusCenterPos( 0 )
    , plateRadiusHomePosOffset( 0 )
    , plateRadiusBacklash( DefaultPlateRadiusBacklash )
    , plateThetaBacklash( DefaultPlateThetaBacklash )
    , plateRadiusA1Pos( 0 )
    , plateRadiusA1PosOffset( 0 )
    , plateRadiusMaxTravel( RadiusMaxTravel )
    , plateThetaStartTimeout( MotorStartTimeout )
    , plateThetaFullTimeout( ThetaFullTimeout )
    , plateRadiusStartTimeout( MotorStartTimeout )
    , plateRadiusFullTimeout( RadiusFullTimeout )
    , plateProbePositionTolerance( DefaultPositionTolerance )
    , plateProbeHomePos( 0 )
    , plateProbeHomePosOffset( 0 )
    , plateProbeStopPos( ProbeMaxTravel )
    , plateProbeMaxTravel( ProbeMaxTravel )
    , plateProbeStartTimeout( MotorStartTimeout )
    , plateProbeBusyTimeout( ProbeBusyTimeout )

    , carouselSelected( FALSE )
    , carouselControlsInited( false )
    , carouselPresent( FALSE )
    , carouselBacklashChanged( false )
    , maxCarouselThetaPos( MaxThetaPosition )
    , carouselPositionTolerance( 0 )
    , carouselThetaHomePos( 0 )
    , carouselThetaHomePosOffset( 0 )
    , carouselThetaBacklash( DefaultCarouselThetaBacklash )
    , maxCarouselTubes( MaxCarouselTubes )
    , carouselRadiusOffset( DefaultCarouselRadiusPos )
    , carouselRadiusMaxTravel( RadiusMaxTravel )
    , carouselThetaStartTimeout( MotorStartTimeout )
    , carouselThetaFullTimeout( ThetaFullTimeout )
    , carouselRadiusStartTimeout( MotorStartTimeout )
    , carouselRadiusFullTimeout( RadiusFullTimeout )
    , carouselProbePositionTolerance( DefaultPositionTolerance )
    , carouselProbeHomePos( 0 )
    , carouselProbeHomePosOffset( 0 )
    , carouselProbeStopPos( ProbeMaxTravel )
    , carouselProbeMaxTravel( ProbeMaxTravel )
    , carouselProbeStartTimeout( MotorStartTimeout )
    , carouselProbeBusyTimeout( ProbeBusyTimeout )

    , focusPositionTolerance( DefaultFocusPositionTolerance )
    , focusMaxPos( FocusMaxTravel )
    , focusStartPos( DefaultFocusStartPos )
    , focusCoarseStepValue( DefaultFocusCoarseStep )      // should be 1/10 revolution of the motor; motor spec'd at 3000 user units per rev
    , focusFineStepValue( DefaultFocusFineStep )          // 0.5% of full revolution
    , focusMaxTravel( FocusMaxTravel )
    , focusStartTimeout( MotorStartTimeout )
    , focusBusyTimeout( FocusBusyTimeout )
//    , focusDeadband( DefaultFocusDeadband )

    , rackPositionTolerance( DefaultLedRackPositionTolerance )
    , rack1HomePosOffset( 0 )
    , rack2HomePosOffset( 0 )
    , rack1BottomPos( 0 )
    , rack2BottomPos( 0 )
    , rack1MiddlePos( 0 )
    , rack2MiddlePos( 0 )
    , rack1TopPos( 0 )
    , rack2TopPos( 0 )
    , maxRackPosition( MaxRackPosition )
    , rackStartTimeout( MotorStartTimeout )
    , rackFullTimeout( LedRackBusyTimeout )

    , armPositionTolerance( DefaultReagentArmPositionTolerance )
    , armHomePos( 0 )
    , armHomeOffset( 0 )
    , armDownPos( 0 )
    , armDownOffset( 0 )
    , armMaxTravel( ReagentArmMaxTravel )
    , armPurgePosition( ReagentArmPurgePosition )
    , reagentStartTimeout( MotorStartTimeout )
    , reagentBusyTimeout( ReagentBusyTimeout )

    , msgType( MsgBoxTypeNone )
    , dlgStarting( false )
    , dlgRunning( false )
{
    m_hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );

    pLocalIosvc_.reset( new boost::asio::io_context() );
    pLocalWork_.reset( new boost::asio::io_context::work( *pLocalIosvc_ ) );

    // Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
    auto THREAD = std::bind( static_cast <std::size_t( boost::asio::io_context::* )( void )> ( &boost::asio::io_context::run ), pLocalIosvc_.get() );
    pIoThread_.reset( new std::thread( THREAD ) );
    pIoThread_->detach();

    pSystemSignals_.reset( new boost::asio::signal_set( *pLocalIosvc_, SIGINT, SIGTERM, SIGABRT ) );
    pCbi_.reset( new ControllerBoardInterface( pLocalIosvc_, CNTLR_SN_A_STR, CNTLR_SN_B_STR ) );

    pUpdTimer.reset( new boost::asio::deadline_timer( *pLocalIosvc_ ) );

    pScriptThread.reset();
    pDlgThread.reset();

    boost::system::error_code ec;
    Logger::L().Initialize( ec, moduleConfigFile );
    Logger::L().Log ( MODULENAME, severity_level::debug1, MODULENAME );
}

MotorSysTestDlg::~MotorSysTestDlg()
{
    Quit();
}

void MotorSysTestDlg::SetEditingMode( bool allow )
{
    allowEditing = allow;
}

void MotorSysTestDlg::SetSimMode( bool isSim )
{
    simMode = isSim;
}

void MotorSysTestDlg::SetInstrumentType( MotorSysTestDlg::InstrumentTypes type )
{
    instrumentType = type;
}


//*****************************************************************************
void MotorSysTestDlg::signalHandler( const boost::system::error_code& ec, int signal_number )
{
    if ( ec )
    {
        Logger::L().Log ( MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"" );
    }

    Logger::L().Log ( MODULENAME, severity_level::critical, boost::str( boost::format( "Received signal no. %d" ) % signal_number ) );

    // All done listening.
    pSystemSignals_->cancel();

    // Try to get out of here.
    pLocalIosvc_->post( std::bind( &MotorSysTestDlg::Quit, this ) );
}

//*****************************************************************************
void MotorSysTestDlg::Quit()
{
    quitting = true;

    // All done listening.
    pSystemSignals_->cancel();

    pUpdTimer->cancel( updTimerError );

    int32_t elapsed = 0;
    while ( ( !timerComplete ) && ( elapsed < 5000 ) )
    {
        ScriptSleep( 250 );         // must allow more time than the standard motor status polling interval...
        elapsed += 250;
    }

    elapsed = 0;
    while ( ( ( signalUpdateStarting ) || ( signalUpdateRunning ) ) && ( elapsed < 5000 ) )
    {
        ScriptSleep( 250 );         // allow signal update thread to terminate
        elapsed += 250;
    }

    if ( pReagentController )
    {
        pReagentController->Quit();
    }

    if ( pPlateController )
    {
        pPlateController->Quit();
    }

    if ( pLedRackController )
    {
        pLedRackController->Quit();
    }

    if ( pFocusController )
    {
        pFocusController->Quit();
    }

    if ( pCarouselController )
    {
        pCarouselController->Quit();
    }

    ScriptSleep( 500 );         // must allow more time than the standard motor status polling interval...

    if ( listFile.is_open() )
    {
        listFile.close();
    }

    if ( saveFile.is_open() )
    {
        saveFile.close();
    }

    if ( plateControllerCalNode )
    {
        plateControllerCalNode.reset();
    }

    if ( carouselControllerCalNode )
    {
        carouselControllerCalNode.reset();
    }

    if ( reagentControllerCfgNode )
    {
        reagentControllerCfgNode.reset();
    }

    if ( plateControllerCfgNode )
    {
        plateControllerCfgNode.reset();
    }

    if ( ledRackControllerCfgNode )
    {
        ledRackControllerCfgNode.reset();
    }

    if ( focusControllerCfgNode )
    {
        focusControllerCfgNode.reset();
    }

    if ( carouselControllerCfgNode )
    {
        carouselControllerCfgNode.reset();
    }

    if ( calfilecfg )
    {
        calfilecfg.reset();
    }

    if ( calcfg )
    {
        calcfg.reset();
    }

    if ( ptfilecfg )
    {
        ptfilecfg.reset();
    }

    if ( ptcfg )
    {
        ptcfg.reset();
    }

    if ( dlgCfgNode )
    {
        dlgCfgNode.reset();
    }

    if ( dlgconfig )
    {
        dlgconfig.reset();
    }

    if ( dlgfilecfg )
    {
        dlgfilecfg.reset();
    }

    if ( pReagentController )
    {
        pPlateController.reset();           // reset calls the normal destructor, which calls the controller 'Quit' method...
    }

    if ( pPlateController )
    {
        pPlateController.reset();           // reset calls the normal destructor, which calls the controller 'Quit' method...
    }

    if ( pLedRackController )
    {
        pLedRackController.reset();         // reset calls the normal destructor, which calls the controller 'Quit' method...
    }

    if ( pFocusController )
    {
        pFocusController.reset();           // reset calls the normal destructor, which calls the controller 'Quit' method...
    }

    if ( pCarouselController )
    {
        pCarouselController.reset();        // reset calls the normal destructor, which calls the controller 'Quit' method...
    }

    if ( pDlgThread )
    {
        pDlgThread.reset();
    }

    if ( pScriptThread )
    {
        pScriptThread.reset();
    }

    if ( pUpdTimer )
    {
        pUpdTimer.reset();
    }

    if ( pCbi_ )
    {
        pCbi_->CancelQueue();
        pCbi_->CloseSerial();
        pCbi_->Close();
        pCbi_.reset();
    }

    if ( pSystemSignals_ )
    {
        pSystemSignals_.reset();
    }

    if ( pIoThread_ )
    {
        pIoThread_.reset();
    }

    if ( pLocalWork_ )
    {
        pLocalWork_.reset();   // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
        pLocalIosvc_->stop();  // instruct io_service to stop processing (should exit ::run() and end thread.
        pLocalIosvc_.reset();  // Destroys the queue
    }

    Logger::L().Flush();
}

bool MotorSysTestDlg::InitDlgInfoFile( void )
{
    bool success = true;
    boost::system::error_code ec;

    if ( !dlgfilecfg )
    {
        dlgfilecfg = ConfigUtils::OpenConfigFile( moduleConfigFile, ec, true );
    }

    if ( !dlgfilecfg )
    {
        Logger::L().Log ( MODULENAME, severity_level::error, "Error opening configuration file \"" + moduleConfigFile + "\"" );
        success = false;
    }
    else
    {
        if ( !dlgconfig )
        {
            dlgconfig = dlgfilecfg->get_child_optional( "config" );
        }

        if ( dlgconfig )
        {
            dlgCfgNode = dlgconfig;
        }
        else
        {
            Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + moduleConfigFile + "\" - \"config\" section not found" );
            success = false;
        }
    }
    return success;
}

void MotorSysTestDlg::ConfigDlgVariables( void )
{
    boost::property_tree::ptree & config = dlgCfgNode.get();

    int32_t editFlag = ( allowEditing == true ) ? 1 : 0;
    editFlag = config.get<int32_t>( "EditMode", editFlag );
    allowEditing = ( editFlag != 0 ) ? true : false;
    int32_t initErrorsFlag = ( showInitErrors == true ) ? 1 : 0;
    initErrorsFlag = config.get<int32_t>( "ShowInitErrors", initErrorsFlag );
    showInitErrors = ( initErrorsFlag != 0 ) ? true : false;
}

bool MotorSysTestDlg::InitControllers()
{
    Logger::L().Log ( MODULENAME, severity_level::debug2, "init: <enter>" );

    if ( inited_ )
    {
        return true;
    }

    if ( !infoInit )
    {
        InitControllerCfgInfo();
    }

    if ( !calInit )
    {
        InitControllerCalInfo();
    }

    int32_t initCnt = 0;
    int32_t numInits = 5;

    if ( !cbiInit )
    {
        bool controllerInit = false;
        t_pPTree motorInfoTree;

        motorInfoTree.reset();

        carouselSelected = false;
        plateSelected = false;

        cbiInit = pCbi_->Initialize();

        pCarouselController.reset( new CarouselController( pLocalIosvc_, pCbi_ ) );
        if ( pCarouselController )
        {
            controllerInit = pCarouselController->Init( CNTLR_SN_A_STR, ptfilecfg, true, motorConfigFile );
            if ( controllerInit )
            {
                initCnt++;
                pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
                if ( pCarouselTheta )
                {
                    int32_t units = 0;
                    int32_t ratio = 0;
                    time_t t_now = 0;
                    time_t t_start = time( 0 );

                    do
                    {
                        ScriptSleep( 100 );
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
        }

        pFocusController.reset( new FocusController( pLocalIosvc_, pCbi_ ) );
        if ( pFocusController )
        {
            controllerInit = pFocusController->Init( CNTLR_SN_A_STR, ptfilecfg, true, motorConfigFile );
            if ( controllerInit )
            {
                initCnt++;
                pFocus = pFocusController->GetMotor();
                focusMaxTravel = pFocusController->GetFocusMax();
            }
        }

        pReagentController.reset( new ReagentController( pLocalIosvc_, pCbi_ ) );
        if ( pReagentController )
        {
            controllerInit = pReagentController->Init( CNTLR_SN_A_STR, ptfilecfg, true, motorConfigFile );
            if ( controllerInit )
            {
                initCnt++;
                pReagent = pReagentController->GetMotor();
            }
        }

        if ( instrumentType == MotorSysTestDlg::InstrumentTypeHunter )
        {
            if ( InitLedController() )
            {
                initCnt++;
            }
        }
        else
        {
            numInits--;
        }

        pPlateController.reset( new PlateController( pLocalIosvc_, pCbi_ ) );
        if ( pPlateController )
        {
            controllerInit = pPlateController->Init( CNTLR_SN_A_STR, ptfilecfg, true, motorConfigFile );
            if ( controllerInit )
            {
                initCnt++;
                pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
                if ( pPlateTheta )
                {
                    int32_t units = 0;
                    int32_t ratio = 0;
                    time_t t_now = 0;
                    time_t t_start = time( 0 );

                    do
                    {
                        ScriptSleep( 100 );
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
        }
    }

    Logger::L().Log ( MODULENAME, severity_level::debug2, "init: <exit>" );

    if ( ( infoInit ) && ( cbiInit ) && ( initCnt == numInits ) )
    {
        inited_ = true;
    }

    return inited_;
}

void MotorSysTestDlg::InitControllerCfgInfo( void )
{
    Logger::L().Log ( MODULENAME, severity_level::debug2, "initControllerCfg: <enter>" );

    if ( !infoInit )
    {
        if ( InitInfoFile( ptfilecfg, ptcfg ) )
        {
            t_opPTree   controllersNode;
            t_opPTree   thisNode;

            controllersNode.reset();
            thisNode.reset();

            controllersNode = ptcfg->get_child_optional( "motor_controllers" );                  // look for the controllers section for individualized parameters...
            if ( controllersNode )
            {
                carouselControllerCfgNode.reset();
                thisNode = controllersNode->get_child_optional( CAROUSELCONTROLLERNODENAME );       // look for this specifc controller
                if ( thisNode )
                {
                    carouselControllerCfgNode = thisNode->get_child_optional( "controller_config" );
                    if ( carouselControllerCfgNode )
                    {
                        ConfigCarouselVariables();
                        ConfigCarouselProbeVariables();
                    }
                    thisNode.reset();
                }

                focusControllerCfgNode.reset();
                thisNode = controllersNode->get_child_optional( FOCUSCONTROLLERNODENAME );          // look for this specifc controller
                if ( thisNode )
                {
                    focusControllerCfgNode = thisNode->get_child_optional( "controller_config" );
                    if ( focusControllerCfgNode )
                    {
                        ConfigFocusVariables();
                    }
                    thisNode.reset();
                }

                ledRackControllerCfgNode.reset();
                thisNode = controllersNode->get_child_optional( LEDRACKCONTROLLERNODENAME );        // look for this specifc controller
                if ( thisNode )
                {
                    ledRackControllerCfgNode = thisNode->get_child_optional( "controller_config" );
                    if ( ledRackControllerCfgNode )
                    {
                        ConfigLedRackVariables();
                    }
                    thisNode.reset();
                }

                plateControllerCfgNode.reset();
                thisNode = controllersNode->get_child_optional( PLATECONTROLLERNODENAME );          // look for this specifc controller
                if ( thisNode )
                {
                    plateControllerCfgNode = thisNode->get_child_optional( "controller_config" );
                    if ( plateControllerCfgNode )
                    {
                        ConfigPlateVariables();
                        ConfigPlateProbeVariables();
                    }
                    thisNode.reset();
                }

                reagentControllerCfgNode.reset();
                thisNode = controllersNode->get_child_optional( REAGENTCONTROLLERNODENAME );        // look for this specifc controller
                if ( thisNode )
                {
                    reagentControllerCfgNode = thisNode->get_child_optional( "controller_config" );
                    if ( reagentControllerCfgNode )
                    {
                        ConfigReagentVariables();
                    }
                }
            }

            if ( controllersNode )
            {
                controllersNode.reset();
            }

            if ( thisNode )
            {
                thisNode.reset();
            }

            infoInit = true;
        }
    }
}

void MotorSysTestDlg::InitControllerCalInfo()
{
    Logger::L().Log ( MODULENAME, severity_level::debug2, "initControllerCal: <enter>" );

    if ( !calInit )
    {
        if ( InitInfoFile( calfilecfg, calcfg, true ) )
        {
            t_opPTree   controllersNode;
            t_opPTree   thisNode;

            controllersNode.reset();
            thisNode.reset();

            controllersNode = calcfg->get_child_optional( "motor_controllers" );                  // look for the controllers section for individualized parameters...
            if ( controllersNode )
            {
                carouselControllerCalNode.reset();
                thisNode = controllersNode->get_child_optional( CAROUSELCONTROLLERNODENAME );       // look for this specifc controller
                if ( thisNode )
                {
                    carouselControllerCalNode = thisNode->get_child_optional( "controller_config" );
                    if ( carouselControllerCalNode )
                    {
                        ConfigCarouselCalVariables();
                    }
                    thisNode.reset();
                }

                plateControllerCalNode.reset();
                thisNode = controllersNode->get_child_optional( PLATECONTROLLERNODENAME );          // look for this specifc controller
                if ( thisNode )
                {
                    plateControllerCalNode = thisNode->get_child_optional( "controller_config" );
                    if ( plateControllerCalNode )
                    {
                        ConfigPlateCalVariables();
                    }
                    thisNode.reset();
                }
            }

            if ( controllersNode )
            {
                controllersNode.reset();
            }

            if ( thisNode )
            {
                thisNode.reset();
            }

            calInit = true;
        }
    }
}

bool MotorSysTestDlg::InitLedController( void )
{
    bool controllerInit = false;

    if ( instrumentType == MotorSysTestDlg::InstrumentTypeHunter )
    {
        pLedRackController.reset( new LEDRackController( pLocalIosvc_, pCbi_ ) );
        if ( pLedRackController )
        {
            controllerInit = pLedRackController->Init( CNTLR_SN_A_STR, ptfilecfg, true, motorConfigFile );
            if ( controllerInit )
            {
                pLedRackController->GetMotors( pRack1, pRack2 );
            }
        }
    }
    return controllerInit;
}

bool MotorSysTestDlg::InitInfoFile( t_pPTree & filecfg, t_opPTree & cfg, bool docalfile )
{
    bool success = true;
    boost::system::error_code ec;
    std::string cfgFile = "";


    if ( docalfile )
    {
        if ( calConfigFile.empty() )
        {
            calConfigFile = "ControllerCal.info";
        }
        cfgFile = calConfigFile;
    }
    else
    {
        if ( motorConfigFile.empty() )
        {
            motorConfigFile = "MotorControl.info";
        }
        cfgFile = motorConfigFile;
    }

    if ( !filecfg )
    {
        filecfg = ConfigUtils::OpenConfigFile( cfgFile, ec, true );
    }

    if ( !filecfg )
    {
        Logger::L().Log ( MODULENAME, severity_level::error, "Error opening configuration file \"" + cfgFile + "\"" );
        success = false;
    }
    else
    {
        if ( !cfg )
        {
            cfg = filecfg->get_child_optional( "config" );
        }

        if ( !cfg )
        {
            Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + cfgFile + "\" - \"config\" section not found" );
            success = false;
        }
    }

    return success;
}

void MotorSysTestDlg::ConfigCarouselVariables( void )
{
    boost::property_tree::ptree & config = carouselControllerCfgNode.get();

    carouselPositionTolerance = config.get<int32_t>( "CarouselPositionTolerance", carouselPositionTolerance );
    carouselThetaHomePos = config.get<int32_t>( "CarouselThetaHomePos", carouselThetaHomePos );
    carouselThetaHomePosOffset = config.get<int32_t>( "CarouselThetaHomePosOffset", carouselThetaHomePosOffset );
    maxCarouselTubes = config.get<int32_t>( "CarouselTubes", maxCarouselTubes );
    carouselThetaBacklash = config.get<int32_t>( "CarouselThetaBacklash", carouselThetaBacklash );
    carouselRadiusOffset = config.get<int32_t>( "CarouselRadiusOffset", carouselRadiusOffset );
    carouselRadiusMaxTravel = config.get<int32_t>( "CarouselRadiusMaxTravel", carouselRadiusMaxTravel );
    carouselThetaStartTimeout = config.get<int32_t>( "ThetaStartTimeout", carouselThetaStartTimeout );
    carouselThetaFullTimeout = config.get<int32_t>( "ThetaFullTimeout", carouselThetaFullTimeout );
    carouselRadiusStartTimeout = config.get<int32_t>( "RadiusStartTimeout", carouselRadiusStartTimeout );
    carouselRadiusFullTimeout = config.get<int32_t>( "RadiusFullTimeout", carouselRadiusFullTimeout );
}

void MotorSysTestDlg::ConfigCarouselProbeVariables( void )
{
    boost::property_tree::ptree & config = carouselControllerCfgNode.get();

    carouselProbePositionTolerance = config.get<int32_t>( "ProbePositionTolerance", carouselProbePositionTolerance );
    carouselProbeHomePos = config.get<int32_t>( "ProbeHomePos", carouselProbeHomePos );
    carouselProbeHomePosOffset = config.get<int32_t>( "ProbeHomePosOffset", carouselProbeHomePosOffset );
    carouselProbeStopPos = config.get<int32_t>( "ProbeDownPos", carouselProbeStopPos );
    carouselProbeMaxTravel = config.get<int32_t>( "ProbeMaxTravel", carouselProbeMaxTravel );
    carouselProbeStartTimeout = config.get<int32_t>( "ProbeStartTimeout", carouselProbeStartTimeout );
    carouselProbeBusyTimeout = config.get<int32_t>( "ProbeBusyTimeout", carouselProbeBusyTimeout );
}

void MotorSysTestDlg::ConfigCarouselCalVariables( void )
{
    boost::property_tree::ptree & config = carouselControllerCalNode.get();
    int32_t tempVal = INT_MAX;

    tempVal = config.get<int32_t>( "CarouselThetaHomePosOffset", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        carouselThetaHomePosOffset = tempVal;
    }

    tempVal = config.get<int32_t>( "CarouselThetaBacklash", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        carouselThetaBacklash = tempVal;
    }

    tempVal = config.get<int32_t>( "CarouselRadiusOffset", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        carouselRadiusOffset = tempVal;
    }

    tempVal = config.get<int32_t>( "ProbeHomePosOffset", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        carouselProbeHomePosOffset = tempVal;
    }
}

void MotorSysTestDlg::ConfigPlateVariables( void )
{
    boost::property_tree::ptree & config = plateControllerCfgNode.get();

    platePositionTolerance = config.get<int32_t>( "PlatePositionTolerance", platePositionTolerance );
    plateThetaCalPos = config.get<int32_t>( "PlateThetaCalPos", plateThetaCalPos );
    plateThetaHomePosOffset = config.get<int32_t>( "PlateThetaHomePosOffset", plateThetaHomePosOffset );
    plateRadiusCenterPos = config.get<int32_t>( "PlateRadiusCenterPos", plateRadiusCenterPos );
    plateRadiusHomePosOffset = config.get<int32_t>( "PlateRadiusHomePosOffset", plateRadiusHomePosOffset );
    plateRadiusBacklash = config.get<int32_t>( "PlateRadiusBacklash", plateRadiusBacklash );
    plateThetaBacklash = config.get<int32_t>( "PlateThetaBacklash", plateThetaBacklash );
    plateRadiusMaxTravel = config.get<int32_t>( "PlateRadiusMaxTravel", plateRadiusMaxTravel );
    plateThetaStartTimeout = config.get<int32_t>( "ThetaStartTimeout", plateThetaStartTimeout );
    plateThetaFullTimeout = config.get<int32_t>( "ThetaFullTimeout", plateThetaFullTimeout );
    plateRadiusStartTimeout = config.get<int32_t>( "RadiusStartTimeout", plateRadiusStartTimeout );
    plateRadiusFullTimeout = config.get<int32_t>( "RadiusFullTimeout", plateRadiusFullTimeout );
}

void MotorSysTestDlg::ConfigPlateProbeVariables( void )
{
    boost::property_tree::ptree & config = plateControllerCfgNode.get();

    plateProbePositionTolerance = config.get<int32_t>( "ProbePositionTolerance", plateProbePositionTolerance );
    plateProbeHomePos = config.get<int32_t>( "ProbeHomePos", plateProbeHomePos );
    plateProbeHomePosOffset = config.get<int32_t>( "ProbeHomePosOffset", plateProbeHomePosOffset );
    plateProbeStopPos = config.get<int32_t>( "ProbeDownPos", plateProbeStopPos );
    plateProbeMaxTravel = config.get<int32_t>( "ProbeMaxTravel", plateProbeMaxTravel );
    plateProbeStartTimeout = config.get<int32_t>( "ProbeStartTimeout", plateProbeStartTimeout );
    plateProbeBusyTimeout = config.get<int32_t>( "ProbeBusyTimeout", plateProbeBusyTimeout );
}

void MotorSysTestDlg::ConfigPlateCalVariables( void )
{
    boost::property_tree::ptree & config = plateControllerCalNode.get();
    int32_t tempVal = INT_MAX;


    tempVal = config.get<int32_t>( "PlateThetaCalPos", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateThetaCalPos = tempVal;
    }

    tempVal = config.get<int32_t>( "PlateThetaHomePosOffset", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateThetaHomePosOffset = tempVal;
    }

    tempVal = config.get<int32_t>( "PlateRadiusCenterPos", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateRadiusCenterPos = tempVal;
    }

    tempVal = config.get<int32_t>( "PlateRadiusBacklash", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateRadiusBacklash = tempVal;
    }

    tempVal = config.get<int32_t>( "PlateThetaBacklash", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateThetaBacklash = tempVal;
    }

    tempVal = config.get<int32_t>( "ProbeHomePosOffset", INT_MAX );
    if ( tempVal != INT_MAX )
    {
        plateProbeHomePosOffset = tempVal;
    }
}

void MotorSysTestDlg::ConfigFocusVariables( void )
{
    boost::property_tree::ptree & config = focusControllerCfgNode.get();

    focusPositionTolerance = config.get<int32_t>( "PositionTolerance", focusPositionTolerance );
    focusMaxPos = config.get<int32_t>( "FocusMaxPos", focusMaxPos );
    focusStartPos = config.get<int32_t>( "FocusStartPos", focusStartPos );
    focusCoarseStepValue = config.get<int32_t>( "FocusCoarseStepValue", focusCoarseStepValue );
    focusFineStepValue = config.get<int32_t>( "FocusFineStepValue", focusFineStepValue );
    focusMaxTravel = config.get<int32_t>( "FocusMaxTravel", focusMaxTravel );
    focusStartTimeout = config.get<int32_t>( "FocusStartTimeout", focusStartTimeout );
    focusBusyTimeout = config.get<int32_t>( "FocusBusyTimeout", focusBusyTimeout );
}

void MotorSysTestDlg::ConfigLedRackVariables( void )
{
    boost::property_tree::ptree & config = ledRackControllerCfgNode.get();

    rackPositionTolerance = config.get<int32_t>( "RackPositionTolerance", rackPositionTolerance );
    rack1HomePosOffset = config.get<int32_t>( "Rack1HomePosOffset", rack1HomePosOffset );
    rack2HomePosOffset = config.get<int32_t>( "Rack2HomePosOffset", rack2HomePosOffset );
    rack1BottomPos = config.get<int32_t>( "Rack1BottomPos", rack1BottomPos );
    rack2BottomPos = config.get<int32_t>( "Rack2BottomPos", rack2BottomPos );
    rack1MiddlePos = config.get<int32_t>( "Rack1MiddlePos", rack1MiddlePos );
    rack2MiddlePos = config.get<int32_t>( "Rack2MiddlePos", rack2MiddlePos );
    rack1TopPos = config.get<int32_t>( "Rack1TopPos", rack1TopPos );
    rack2TopPos = config.get<int32_t>( "Rack2TopPos", rack2TopPos );
    rackStartTimeout = config.get<int32_t>( "RackStartTimeout", rackStartTimeout );
    rackFullTimeout = config.get<int32_t>( "RackFullTimeout", rackFullTimeout );
    maxRackPosition = config.get<int32_t>( "RackMaxTravel", maxRackPosition );
}

// check a position for coincidence with a supplied target using supplied or default tolerances
void MotorSysTestDlg::ConfigReagentVariables( void )
{
    boost::property_tree::ptree & config = reagentControllerCfgNode.get();

    armPositionTolerance = config.get<int32_t>( "PositionTolerance", armPositionTolerance );
    armHomePos = config.get<int32_t>( "ReagentHomePos", armHomePos );
    armHomeOffset = config.get<int32_t>( "ReagentHomePosOffset", armHomeOffset );
    armDownPos = config.get<int32_t>( "ReagentDownPos", armDownPos );
    armDownOffset = config.get<int32_t>( "ReagentDownPosOffset", armDownOffset );
    armMaxTravel = config.get<int32_t>( "ReagentArmMaxTravel", armMaxTravel );
    armPurgePosition = config.get<int32_t>( "ReagentPurgePosition", armPurgePosition );
    reagentStartTimeout = config.get<int32_t>( "ReagentStartTimeout", reagentStartTimeout );
    reagentBusyTimeout = config.get<int32_t>( "ReagentBusyTimeout", reagentBusyTimeout );
}

bool MotorSysTestDlg::OpenListFile( std::string filename )
{
    if ( listFile.is_open() )
    {
        listFile.close();
    }

    listFile.open( filename );
    boost::filesystem::path p( filename );

    bool fileOpen = listFile.is_open();
    std::string logStr = "opening move run list input file: " + p.filename().generic_string();
    if ( !fileOpen )
    {
        logStr = "failed to open move run list input file: " + filename;
    }
    Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    return fileOpen;
}

bool MotorSysTestDlg::OpenSaveFile( std::string filename )
{
    if ( saveFile.is_open() )
    {
        saveFile.close();
    }

    std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
    saveFile.open( filename, mode );
    boost::filesystem::path p( filename );

    bool fileOpen = saveFile.is_open();
    std::string logStr = "opening move run list output file: " + p.filename().generic_string();
    if ( !fileOpen )
    {
        logStr = "failed to open move run list output file: " + filename;
    }
    Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    return fileOpen;
}

int32_t MotorSysTestDlg::ReadListFile( std::string& cmdBlock )
{
    int32_t lineCnt = 0;

    if ( listFile.is_open() )
    {
        std::string rdLine;
        std::string cmdLine;
        std::string tokenStr;
        bool fileEnd = false;
        int32_t paramIdx = 1;

        do
        {
            rdLine.clear();
            cmdLine.clear();
            if ( getline( listFile, rdLine ) )
            {
                boost::algorithm::trim( rdLine );

                // allow ALL lines to be read for display
                if ( ( rdLine.length() > 0 ) && ( rdLine[0] != '!' ) )      // only PARSE non-empty, non-comment lines...
                {
                    CommandParser cmdParser;                                // must create a new separate parser for each line...

                    // parse the line for token content
                    cmdParser.parse( ",", rdLine );

                    // at this point, the first token SHOULD always be valid, but...
                    cmdLine = cmdParser.getByIndex( 0 );
                    if ( !cmdLine.empty() )
                    {
                        // first token uses mixed-case string constants for commands
                        // ensure that AT LEAST the first character is upper case to match expected
                        std::toupper( cmdLine[0] );

                        paramIdx = 1;
                        while ( ( cmdParser.hasIndex( paramIdx ) ) &&
                                ( paramIdx < cmdParser.size() ) &&
                                ( paramIdx < 10 ) )                         // should never have more than this, but limit anyway...
                        {
                            cmdLine.append( ", " );
                            tokenStr = cmdParser.getByIndex( paramIdx );
                            boost::algorithm::trim( tokenStr );             // remove any embedded spaces from tokens...
                            cmdLine.append( tokenStr );
                            paramIdx++;
                        }
                    }
                }
                else
                {
                    cmdLine.append( rdLine );       // take the entire comment line when reading the file...
                }

                if ( lineCnt++ )                    // append the EOL chars for the edit display control, and later parsing...
                {
                    cmdBlock.append( "\r\n" );      // the edit control requires the carriage-return/line-feed combo, not just a line-feed
                }
                cmdBlock.append( cmdLine );
            }
            else
            {
                fileEnd = true;
            }
        } while ( !fileEnd );

        listFile.close();

        if ( lineCnt )
        {
            cmdBlock.append( "\r\n" );              // the edit control requires the carriage-return/line-feed combo, not just a line-feed
        }
    }
    return lineCnt;
}

int32_t MotorSysTestDlg::WriteListFile( void )
{
    int32_t lineIdx = 0;

    if ( saveFile.is_open() )
    {
        int32_t lineCnt = 0;
        int32_t lineLen = 0;
        std::string scriptLine;
        CString lineStr;

        lineCnt = ScriptEdit.GetLineCount();

        while ( lineIdx < lineCnt )
        {
            lineStr.Empty();
            lineLen = ScriptEdit.LineLength( ScriptEdit.LineIndex( lineIdx ) );
            ScriptEdit.GetLine( lineIdx, lineStr.GetBufferSetLength( lineLen ), lineLen );
            scriptLine = CT2A( lineStr.Left( lineLen ) );
            if ( ( lineIdx + 1 ) < lineCnt )
            {
                scriptLine.append( "\n" );      // don't force a carriage return on the last list; it adds an extra line to the output not present in the input
            }

            saveFile.write( scriptLine.c_str(), scriptLine.size() );
            lineIdx++;
        }
        saveFile.close();
    }
    return lineIdx;
}

void MotorSysTestDlg::StartScript( void )
{
    if ( ( !scriptStarting ) && ( !scriptRunning ) )
    {
        SetAllControlEnable( FALSE );
        PauseScriptBtn.EnableWindow( TRUE );
        StopScriptBtn.EnableWindow( TRUE );
        stopRun = false;
        scriptPaused = false;
        errorCnt = 0;
        scriptStarting = true;
        pScriptThread.reset( new std::thread( &MotorSysTestDlg::RunScript, this ) );
        pScriptThread->detach();
    }
}

void MotorSysTestDlg::RunScript( void )
{
    int32_t lineIdx = 0;

    if ( !scriptRunning )
    {
        scriptRunning = true;
        scriptStarting = false;

        Logger::L().Log ( MODULENAME, severity_level::debug1, "RunScript: Starting run." );

        ScriptEdit.Clear();
        ScriptEdit.EnableScrollBar( SB_HORZ, ESB_DISABLE_BOTH );
        ScriptEdit.EnableScrollBar( SB_VERT, ESB_DISABLE_UP );
        scriptLines = ScriptEdit.GetLineCount();

        ProcessScriptList( scriptLines );

        ScriptEdit.EnableScrollBar( SB_HORZ, ESB_ENABLE_BOTH );
        ScriptEdit.EnableScrollBar( SB_VERT, ESB_ENABLE_BOTH );

        if ( stopRun )
        {
            Logger::L().Log ( MODULENAME, severity_level::debug1, "RunScript: RunFile stopped." );
        }
        else
        {
            Logger::L().Log ( MODULENAME, severity_level::debug1, "RunScript: RunFile complete." );
        }

        if ( !simMode )
        {
            if ( carouselSelected )
            {
                pCarouselController->InitCarousel();
            }
            else if ( plateSelected )
            {
                pPlateController->InitPlate();
            }
        }
        SetAllControlEnable( TRUE );
        scriptRunning = false;
        scriptStarting = false;
        scriptPaused = false;
        stopRun = false;
        PauseScriptBtn.EnableWindow( FALSE );
        StopScriptBtn.EnableWindow( FALSE );
    }
}

void MotorSysTestDlg::ProcessScriptList( int32_t lineCnt )
{
    int32_t lineIdx = 0;

    Logger::L().Log ( MODULENAME, severity_level::debug1, "ProcessScriptList: Starting script list." );

    while ( ( lineIdx < lineCnt ) && ( !stopRun ) )
    {
        if ( !scriptPaused )
        {
            lineIdx = DoScriptStep( lineIdx, lineCnt );
            if ( lineIdx == -1 )
            {
                lineIdx = lineCnt;
            }
        }
        else
        {
            ScriptSleep( 100 );
        }
    }

    if ( stopRun )
    {
        Logger::L().Log ( MODULENAME, severity_level::debug1, "ProcessScriptList: list stopped." );
    }
    else
    {
        Logger::L().Log ( MODULENAME, severity_level::debug1, "ProcessScriptList: list complete." );
    }
}

int32_t MotorSysTestDlg::DoScriptStep( int32_t stepIdx, int32_t lineCnt )
{
    int32_t lineIdx = stepIdx;
    int32_t lineLen = 0;
    int32_t chIdx = 0;
    CString lineStr;
    std::string line;
    std::string cmd;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    bool listExit = false;

    if ( stepIdx >= 0 )
    {
        if ( ( lineCnt > 0 ) && ( lineIdx < lineCnt ) )
        {
            UpdateStep( lineIdx );
            lineStr.Empty();
            line.clear();

            ScriptEdit.Clear();
            chIdx = ScriptEdit.LineIndex( lineIdx );
            lineLen = ScriptEdit.LineLength( chIdx );
            ScriptEdit.SetFont( &scriptEditFont );
            ScriptEdit.SetSel( chIdx, ( chIdx + lineLen ) );
            ScriptEdit.LineScroll( 0, -( lineLen ) );

            ScriptEdit.GetLine( lineIdx, lineStr.GetBufferSetLength( lineLen ), lineLen );
            line = CT2A( lineStr );
            Logger::L().Log ( MODULENAME, severity_level::debug2, boost::str( boost::format( "DoScriptStep: Script line read: lineIdx: %d,  line: '%s',  lineLen: %d" ) % lineIdx % line % lineLen ) );

            cmd.clear();
            param1 = param2 = param3 = 0;
            if ( ParseScriptLine( line, cmd, param1, param2, param3 ) )
            {
                if ( cmd.length() > 0 )
                {
                    listExit = RunScriptCmd( cmd, param1, param2, param3 );
                }

                if ( !listExit )
                {
                    lineIdx++;
                }
                else
                {
                    lineIdx = -1;
                }
            }
            else
            {
                lineIdx++;      // for lines with no executable command
            }
            lineStr.ReleaseBufferSetLength( lineLen );
            if ( lineIdx < lineCnt )
            {
                if ( scrollIdx != -1 )
                {
                    ScriptEdit.LineScroll( 1 );
                    int32_t topIdx = ScriptEdit.GetFirstVisibleLine();
                    if ( topIdx != scrollIdx )
                    {
                        scrollIdx = topIdx;
                    }
                    else
                    {
                        scrollIdx = -1;     // indicate the window can't be scrolled any further
                    }
                }
            }
        }
    }
    return lineIdx;
}

bool MotorSysTestDlg::ParseScriptLine( std::string cmdLine, std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3 )
{
    bool parseOk = false;

    boost::algorithm::trim( cmdLine );

    // Skip empty lines
    // When parsing for execution, skip commented out lines.  Comment character is '!'.
    if ( ( cmdLine.length() > 0 ) && ( cmdLine[0] != '!' ) )
    {
        int32_t paramIdx = 1;
        int32_t lineLen = 0;
        uint32_t paramVal = 0;
        CommandParser cmdParser;    // must create a new separate parser for each line...

        cmdParser.parse( ",", cmdLine );

        cmd = cmdParser.getByIndex( 0 );
        std::toupper( cmd[0] );

        paramIdx = 1;
        while ( cmdParser.hasIndex( paramIdx ) )
        {
            paramVal = std::stoul( cmdParser.getByIndex( paramIdx ) );
            switch ( paramIdx )
            {
                case 1:
                    param1 = paramVal;
                    break;

                case 2:
                    param2 = paramVal;
                    break;

                case 3:
                    param3 = paramVal;
                    break;

                default:
                    break;
            }
            paramIdx++;
        }
        Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "ParseScriptLine: cmdLine: %s, cmd: %s, param1: %d, param2: %d, param2: %d" ) % cmdLine % cmd % param1 % param2 % param3 ) );
        parseOk = true;
    }
    return parseOk;
}

bool MotorSysTestDlg::RunScriptCmd( std::string cmd, uint32_t param1, uint32_t param2, uint32_t param3 )
{
    bool listExit = false;
    bool relativeMoveFlag = doRelativeMove;

    Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "RunScriptCmd: cmd: %s, param1: %d, param2: %d, param2: %d" ) % cmd % param1 % param2 % param3 ) );

    doRelativeMove = false;
    if ( cbiInit )
    {
        if ( cmd.compare( MotorSysTestDlg::CmdSimModeStr ) == 0 )
        {
            if ( param1 == 0 )
            {
                simMode = false;
            }
            else
            {
                simMode = true;
            }
        }
        // Initialize.
        else if ( cmd.compare( MotorSysTestDlg::CmdInitPlateStr ) == 0 )
        {
            DoPlateInit( false );
        }
        else if ( cmd.compare( MotorSysTestDlg::CmdInitCarouselStr ) == 0 )
        {
            DoCarouselInit( false );
        }
        else if ( cmd.compare( MotorSysTestDlg::CmdSelectPlateStr ) == 0 )
        {
            DoPlateSelect( false );
        }
        else if ( cmd.compare( MotorSysTestDlg::CmdSelectCarouselStr ) == 0 )
        {
            DoCarouselSelect( false );
        }
        // Home the Theta motor
        else if ( cmd.compare( MotorSysTestDlg::CmdHomeThetaStr ) == 0 )
        {
            OnBnClickedHomeThetaBtn();
        }
        // Home the Radius motor
        else if ( cmd.compare( MotorSysTestDlg::CmdHomeRadiusStr ) == 0 )
        {
            OnBnClickedHomeRadiusBtn();
        }
        // move the Theta motor to the specified absolute location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveThetaStr ) == 0 )
        {
            thetaEditVal = param1;
            DoGotoThetaPos();
        }
        // move the Theta motor to the specified relative location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveThetaRelStr ) == 0 )
        {
            thetaEditVal = param1;
            DoGotoThetaPos( true );
        }
        // move the Radius motor to the specified absolute location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveRadiusStr ) == 0 )
        {
            radiusEditVal = param1;
            DoGotoRadiusPos();
        }
        // move the Radius motor to the specified relative location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveRadiusRelStr ) == 0 )
        {
            radiusEditVal = param1;
            DoGotoRadiusPos( true );
        }
        // move the plate to the specified absolute theta/radius location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveRThetaStr ) == 0 )
        {
            thetaEditVal = param1;
            radiusEditVal = param2;
            DoGotoRThetaPos();
        }
        // move the plate to the specified relative theta/radius location
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveRThetaRelStr ) == 0 )
        {
            thetaEditVal = param1;
            radiusEditVal = param2;
            DoGotoRThetaPos( true );
        }
        else if ( cmd.compare( MotorSysTestDlg::CmdMoveCarouselStr ) == 0 )
        {
            if ( carouselSelected )
            {
                if ( ( param1 >= 0 ) && ( param1 <= MaxCarouselTubes ) )
                {
                    if ( param1 == 0 )
                    {
                        OnBnClickedNextTubeBtn();
                    }
                    else
                    {
                        tubeEditVal = param1;       // to pass to the method..
                        OnBnClickedGotoTubeBtn();
                    }
                }
            }
        }
        // move the plate to the specified row/col well location
        else if ( cmd.compare( MotorSysTestDlg::CmdMovePlateWellStr ) == 0 )
        {
            if ( plateSelected )
            {
                if ( ( param1 > 0 ) && ( param1 <= MaxPlateRowNum ) &&
                    ( param2 > 0 ) && ( param2 <= MaxPlateColNum ) )
                {
                    rowEditVal = param1;
                    colEditVal = param2;
                    OnBnClickedGotoRowColBtn();
                }
            }
        }
        // Move plate to absolute radius, theta position
        else if ( cmd.compare( MotorSysTestDlg::CmdMovePlateStr ) == 0 )
        {
            if ( plateSelected )
            {
                thetaEditVal = param1;
                radiusEditVal = param1;
                DoGotoRThetaPos();
            }
        }
        // Move plate to relative radius, theta position change values
        else if ( cmd.compare( MotorSysTestDlg::CmdMovePlateRelStr ) == 0 )
        {
            if ( plateSelected )
            {
                if ( !simMode )
                {
                    pPlateController->MoveTo( param1, param2 );
                }
            }
        }
        // Home the probe for the current active controller
        else if ( cmd.compare( MotorSysTestDlg::CmdProbeHomeStr ) == 0 )
        {
            OnBnClickedProbeHomeBtn();
        }
        // Move the probe for the current active controller
        else if ( cmd.compare( MotorSysTestDlg::CmdProbeMoveStr ) == 0 )
        {
            if ( param1 == 0 )
            {
                OnBnClickedProbeUpBtn();
            }
            else
            {
                OnBnClickedProbeDownBtn();
            }
        }
    }
    doRelativeMove = relativeMoveFlag;      // restore to previous condition

    // CHECK FOR NON-MOTOR-RELATED COMMANDS THAT CAN BE EXECUTED ANYTIME...
    // Wait for a keypress.
    if ( cmd.compare( MotorSysTestDlg::CmdWaitForKeyPressStr ) == 0 )
    {
        int response = MessageBox( _T( "Click the OK button to continue, or the 'Cancel' button to quit." ), _T( "Waiting for user to continue..." ), MB_OKCANCEL );
        if ( response == IDCANCEL )
        {
            listExit = true;
        }
    }
    // insert a delay into the command stream processing
    else if ( cmd.compare( MotorSysTestDlg::CmdSleepStr ) == 0 )
    {
        ScriptSleep( param1 );
    }
    // Stop processing the run list and exit the program.
    else if ( cmd.compare( MotorSysTestDlg::CmdExitStr ) == 0 )
    {
        listExit = true;
    }

    return listExit;
}

void MotorSysTestDlg::ScriptSleep( int32_t sleepTime )
{
    auto interval = std::chrono::milliseconds( sleepTime );
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + interval;
    auto shortInterval = interval / 20;
    if ( shortInterval < std::chrono::milliseconds( 1 ) )
    {
        shortInterval = std::chrono::milliseconds( 1 );
    }
    auto next = start + ( shortInterval );

    do
    {
        next = std::chrono::high_resolution_clock::now() + ( shortInterval );
        while ( std::chrono::high_resolution_clock::now() < next );     // try looping in place and not yielding the thread...
        std::this_thread::yield();
    } while ( std::chrono::high_resolution_clock::now() < end );
}

void MotorSysTestDlg::DoDataExchange( CDataExchange* pDX )
{
    CDialogEx::DoDataExchange( pDX );
    DDX_Control( pDX, IDC_SCRIPT_EDIT, ScriptEdit );
    DDX_Text   ( pDX, IDC_SCRIPT_EDIT, ScriptEditBuf );

    DDX_Control( pDX, IDC_REAGENT_ARM_UP_BTN, ReagentArmUpBtn );
    DDX_Control( pDX, IDC_REAGENT_ARM_DN_BTN, ReagentArmDownBtn );
    DDX_Control( pDX, IDC_REAGENT_ARM_PURGE_BTN, ReagentArmPurgeBtn );
    DDX_Control( pDX, IDC_REAGENT_DOOR_CLOSED_CHK, DoorClosedChk );
    DDX_Control( pDX, IDC_UNLOCK_DOOR_BTN, ReagentDoorReleaseBtn );
    DDX_Control( pDX, IDC_REAGENT_PACK_DETECTED_CHK, PackDetectedChk );
    DDX_Control( pDX, IDC_REAGENT_HOME_CHK, ReagentArmHomeChk );
    DDX_Control( pDX, IDC_REAGENT_LIMIT_CHK, ReagentArmLimitChk );

    DDX_Control( pDX, IDC_THETA_POS_CURRENT_DISP, CurrentThetaDisp );
    DDX_Text   ( pDX, IDC_THETA_POS_CURRENT_DISP, thetaVal );
    DDX_Control( pDX, IDC_THETA_POS_EDIT, ThetaPosEdit );
    DDX_Text   ( pDX, IDC_THETA_POS_EDIT, thetaEditVal );
    DDX_Control( pDX, IDC_THETA_GOTO_POS_BTN, GotoThetaBtn );
    DDX_Control( pDX, IDC_THETA_HOME_CHK, ThetaHomeChk );
    DDX_Control( pDX, IDC_THETA_HOME_BTN, HomeThetaBtn );

    DDX_Control( pDX, IDC_RADIUS_POS_CURRENT_DISP, CurrentRadiusDisp );
    DDX_Text   ( pDX, IDC_RADIUS_POS_CURRENT_DISP, radiusVal );
    DDX_Control( pDX, IDC_RADIUS_POS_EDIT, RadiusPosEdit );
    DDX_Text   ( pDX, IDC_RADIUS_POS_EDIT, radiusEditVal );
    DDX_Control( pDX, IDC_RADIUS_GOTO_POS_BTN, GotoRadiusBtn );
    DDX_Control( pDX, IDC_RADIUS_HOME_CHK, RadiusHomeChk );
    DDX_Control( pDX, IDC_RADIUS_HOME_BTN, HomeRadiusBtn );
    DDX_Control( pDX, IDC_GOTO_RTHETA_POS_BTN, GotoRThetaBtn );

    DDX_Control( pDX, IDC_RELATIVE_MOVE_CHK, RelativeMoveChk );

    DDX_Control( pDX, IDC_PROBE_UP_BTN, ProbeUpBtn );
    DDX_Control( pDX, IDC_PROBE_DOWN_BTN, ProbeDownBtn );
    DDX_Control( pDX, IDC_PROBE_INIT_BTN, InitProbeBtn );
    DDX_Control( pDX, IDC_PROBE_HOME_BTN, HomeProbeBtn );
    DDX_Control( pDX, IDC_PROBE_HOME_CHK, ProbeHomeChk );
    DDX_Control( pDX, IDC_PROBE_POS_CURRENT_DISP, CurrentProbeDisp );
    DDX_Text   ( pDX, IDC_PROBE_POS_CURRENT_DISP, probeVal );

    DDX_Control( pDX, IDC_FINE_FOCUS_UP_BTN, FineFocusUpBtn );
    DDX_Control( pDX, IDC_FINE_FOCUS_DN_BTN, FineFocusDownBtn );
    DDX_Control( pDX, IDC_COARSE_FOCUS_UP_BTN, CoarseFocusUpBtn );
    DDX_Control( pDX, IDC_COARSE_FOCUS_DN_BTN, CoarseFocusDownBtn );
    DDX_Control( pDX, IDC_FOCUS_POS_CURRENT_DISP, CurrentFocusDisp );
    DDX_Text   ( pDX, IDC_FOCUS_POS_CURRENT_DISP, focusVal );
    DDX_Control( pDX, IDC_FOCUS_POS_EDIT, FocusPosEdit );
    DDX_Text   ( pDX, IDC_FOCUS_POS_EDIT, focusEditVal );
    DDX_Control( pDX, IDC_FOCUS_GOTO_POS_BTN, GotoFocusPosBtn );
    DDX_Control( pDX, IDC_FOCUS_HOME_CHK, FocusHomeChk );
    DDX_Control( pDX, IDC_FOCUS_HOME_BTN, HomeFocusBtn );
    DDX_Control( pDX, IDC_FOCUS_GOTO_CENTER_BTN, CenterFocusPosBtn );

    DDX_Control( pDX, IDC_INST_TYPE_CHK, InstrumentTypeChk );

    DDX_Control( pDX, IDC_LED_RACK1_LBL, LedRack1Lbl );
    DDX_Control( pDX, IDC_LED_RACK1_PATH_DISP, Rack1PathDisp );
    DDX_Control( pDX, IDC_LED_RACK1_GOTO_TOP_BTN, Rack1TopBtn );
    DDX_Control( pDX, IDC_LED_RACK1_CAL_TOP_BTN, Rack1TopCalBtn );
    DDX_Control( pDX, IDC_LED_RACK1_GOTO_CTR_BTN, Rack1CtrBtn );
    DDX_Control( pDX, IDC_LED_RACK1_CAL_CTR_BTN, Rack1CtrCalBtn );
    DDX_Control( pDX, IDC_LED_RACK1_GOTO_BOT_BTN, Rack1BotBtn );
    DDX_Control( pDX, IDC_LED_RACK1_CAL_BOT_BTN, Rack1BotCalBtn );
    DDX_Control( pDX, IDC_LED_RACK1_HOME_BTN, Rack1HomeBtn );
    DDX_Control( pDX, IDC_RACK1_HOME_CHK, Rack1HomeChk );
    DDX_Control( pDX, IDC_LED_RACK2_LBL, LedRack2Lbl );
    DDX_Control( pDX, IDC_LED_RACK2_PATH_DISP, Rack2PathDisp );
    DDX_Control( pDX, IDC_LED_RACK2_GOTO_TOP_BTN, Rack2TopBtn );
    DDX_Control( pDX, IDC_LED_RACK2_CAL_TOP_BTN, Rack2TopCalBtn );
    DDX_Control( pDX, IDC_LED_RACK2_GOTO_CTR_BTN, Rack2CtrBtn );
    DDX_Control( pDX, IDC_LED_RACK2_CAL_CTR_BTN, Rack2CtrCalBtn );
    DDX_Control( pDX, IDC_LED_RACK2_GOTO_BOT_BTN, Rack2BotBtn );
    DDX_Control( pDX, IDC_LED_RACK2_CAL_BOT_BTN, Rack2BotCalBtn );
    DDX_Control( pDX, IDC_LED_RACK2_HOME_BTN, Rack2HomeBtn );
    DDX_Control( pDX, IDC_RACK2_HOME_CHK, Rack2HomeChk );

    DDX_Control( pDX, IDC_ROW_CURRENT_DISP, CurrentRowDisp );
    DDX_Text   ( pDX, IDC_ROW_EDIT, rowCharStr );
    DDX_Control( pDX, IDC_ROW_EDIT, RowValEdit );
    DDX_Text   ( pDX, IDC_ROW_EDIT, rowEditStr );
    DDV_MaxChars( pDX, rowEditStr, 3 );
    DDX_Control( pDX, IDC_COL_CURRENT_DISP, CurrentColDisp );
    DDX_Control( pDX, IDC_COL_EDIT, ColValEdit );
    DDX_Text   ( pDX, IDC_COL_EDIT, colEditVal );
    DDV_MinMaxLong( pDX, colEditVal, 0, 12 );
    DDX_Control( pDX, IDC_ROW_COL_GOTO_BTN, GotoRowColBtn );
    DDX_Control( pDX, IDC_ROW_COL_NEXT_BTN, NextRowColBtn );

    DDX_Control( pDX, IDC_TUBE_NUM_CURRENT_DISP, CurrentTubeDisp );
    DDX_Text   ( pDX, IDC_TUBE_NUM_CURRENT_DISP, tubeNum );
    DDX_Control( pDX, IDC_TUBE_NUM_EDIT, TubeNumEdit );
    DDX_Text   ( pDX, IDC_TUBE_NUM_EDIT, tubeEditVal );
    DDX_Control( pDX, IDC_TUBE_GOTO_BTN, GotoTubeNumBtn );
    DDX_Control( pDX, IDC_TUBE_NEXT_BTN, NextTubeBtn );
    DDX_Control( pDX, IDC_TUBE_FIND_BTN, FindTubeBtn );
    DDX_Control( pDX, IDC_TUBE_DETECTED_CHK, TubePresentChk );

    DDX_Control( pDX, IDC_UPDATE_CURRENT_BTN, UpdateBtn );

    DDX_Control( pDX, IDC_RADIO_PLATE_SELECT, PlateSelectRadio );
    DDX_Control( pDX, IDC_PLATE_INIT_ERROR_CHK, PlateInitErrorChk );
    DDX_Control( pDX, IDC_PLATE_DETECTED_CHK, PlateDetectedChk );
    DDX_Control( pDX, IDC_PLATE_CAL_BTN, CalibratePlateBtn );
    DDX_Control( pDX, IDC_PLATE_EJECT_BTN, EjectPlateBtn );
    DDX_Control( pDX, IDC_PLATE_LOAD_BTN, LoadPlateBtn );
    DDX_Control( pDX, IDC_PLATE_INIT_BTN, InitPlateBtn );
    DDX_Control( pDX, IDC_PLATE_DETECT_BTN, DetectPlateBtn );
    DDX_Control( pDX, IDC_PLATE_THETA_ADJUST_LEFT_BTN, PlateThetaAdjustLeftBtn );
    DDX_Control( pDX, IDC_PLATE_THETA_ADJUST_RIGHT_BTN, PlateThetaAdjustRightBtn );
    DDX_Control( pDX, IDC_PLATE_RADIUS_ADJUST_IN_BTN, PlateRadiusAdjustInBtn );
    DDX_Control( pDX, IDC_PLATE_RADIUS_ADJUST_OUT_BTN, PlateRadiusAdjustOutBtn );
    DDX_Control( pDX, IDC_PLATE_RADIUS_BACKLASH_EDIT, PlateRadiusBacklashEdit );
    DDX_Control( pDX, IDC_PLATE_RADIUS_BACKLASH_SET_BTN, PlateRadiusBacklashSetBtn );
    DDX_Control( pDX, IDC_PLATE_THETA_BACKLASH_EDIT, PlateThetaBacklashEdit );
    DDX_Control( pDX, IDC_PLATE_THETA_BACKLASH_SET_BTN, PlateThetaBacklashSetBtn );

    DDX_Control( pDX, IDC_RADIO_CAROUSEL_SELECT, CarouselSelectRadio );
    DDX_Control( pDX, IDC_CAROUSEL_INIT_ERROR_CHK, CarouselInitErrorChk );
    DDX_Control( pDX, IDC_CAROUSEL_DETECTED_CHK, CarouselPresentChk );
    DDX_Control( pDX, IDC_CAROUSEL_CAL_BTN, CalibrateCarouselBtn );
    DDX_Control( pDX, IDC_CAROUSEL_EJECT_BTN, EjectCarouselBtn );
    DDX_Control( pDX, IDC_CAROUSEL_LOAD_BTN, LoadCarouselBtn );
    DDX_Control( pDX, IDC_CAROUSEL_INIT_BTN, InitCarouselBtn );
    DDX_Control( pDX, IDC_CAROUSEL_THETA_ADJUST_LEFT_BTN, CarouselThetaAdjustLeftBtn );
    DDX_Control( pDX, IDC_CAROUSEL_THETA_ADJUST_RIGHT_BTN, CarouselThetaAdjustRightBtn );
    DDX_Control( pDX, IDC_CAROUSEL_RADIUS_ADJUST_IN_BTN, CarouselRadiusAdjustInBtn );
    DDX_Control( pDX, IDC_CAROUSEL_RADIUS_ADJUST_OUT_BTN, CarouselRadiusAdjustOutBtn );
    DDX_Control( pDX, IDC_CAROUSEL_THETA_BACKLASH_EDIT, CarouselThetaBacklashEdit );
    DDX_Control( pDX, IDC_CAROUSEL_THETA_BACKLASH_SET_BTN, CarouselThetaBacklashSetBtn );

    DDX_Control( pDX, IDC_RUN_SCRIPT_BTN, RunScriptBtn );
    DDX_Control( pDX, IDC_STEP_NUM_DISP, StepNumDisp );
    DDX_Control( pDX, IDC_STEP_NUM_SPIN, StepNumSpin );
    DDX_Control( pDX, IDC_ERROR_CNT_DISP, ErrorCntDisp );
    DDX_Text   ( pDX, IDC_ERROR_CNT_DISP, errorCnt );
    DDX_Control( pDX, IDC_ERROR_CNT_LBL, ErrorCntLbl );
    DDX_Control( pDX, IDC_PAUSE_SCRIPT_BTN, PauseScriptBtn );
    DDX_Control( pDX, IDC_STOP_SCRIPT_BTN, StopScriptBtn );

    DDX_Control( pDX, IDC_SCRIPT_PATH_EDIT, ScriptPathEdit );
    DDX_Text   ( pDX, IDC_SCRIPT_PATH_EDIT, ScriptPath );
    DDX_Control( pDX, IDC_LOAD_SCRIPT_BTN, LoadScriptBtn );
    DDX_Control( pDX, IDC_CLEAR_SCRIPT_BTN, ClearScriptBtn );
    DDX_Control( pDX, IDC_SAVE_SCRIPT_BTN, SaveScriptBtn );
}

BEGIN_MESSAGE_MAP( MotorSysTestDlg, CDialogEx )
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()

    ON_BN_CLICKED( IDC_REAGENT_ARM_UP_BTN, &MotorSysTestDlg::OnBnClickedReagentArmUpBtn )
    ON_BN_CLICKED( IDC_REAGENT_ARM_DN_BTN, &MotorSysTestDlg::OnBnClickedReagentArmDnBtn )
    ON_BN_CLICKED( IDC_REAGENT_ARM_PURGE_BTN, &MotorSysTestDlg::OnBnClickedReagentArmPurgeBtn )
    ON_BN_CLICKED( IDC_UNLOCK_DOOR_BTN, &MotorSysTestDlg::OnBnClickedUnlockDoorBtn )

    ON_EN_CHANGE ( IDC_THETA_POS_EDIT, &MotorSysTestDlg::OnEnChangeThetaPosEdit )
    ON_BN_CLICKED( IDC_THETA_GOTO_POS_BTN, &MotorSysTestDlg::OnBnClickedGotoThetaPosBtn )
    ON_BN_CLICKED( IDC_THETA_HOME_BTN, &MotorSysTestDlg::OnBnClickedHomeThetaBtn )

    ON_EN_CHANGE ( IDC_RADIUS_POS_EDIT, &MotorSysTestDlg::OnEnChangeRadiusPosEdit )
    ON_BN_CLICKED( IDC_RADIUS_GOTO_POS_BTN, &MotorSysTestDlg::OnBnClickedGotoRadiusPosBtn )
    ON_BN_CLICKED( IDC_RADIUS_HOME_BTN, &MotorSysTestDlg::OnBnClickedHomeRadiusBtn )
    ON_BN_CLICKED( IDC_GOTO_RTHETA_POS_BTN, &MotorSysTestDlg::OnBnClickedGotoRThetaPosBtn )

    ON_BN_CLICKED( IDC_RELATIVE_MOVE_CHK, &MotorSysTestDlg::OnBnClickedRelativeMoveChk )

    ON_BN_CLICKED( IDC_PROBE_UP_BTN, &MotorSysTestDlg::OnBnClickedProbeUpBtn )
    ON_BN_CLICKED( IDC_PROBE_DOWN_BTN, &MotorSysTestDlg::OnBnClickedProbeDownBtn )
    ON_BN_CLICKED( IDC_PROBE_HOME_BTN, &MotorSysTestDlg::OnBnClickedProbeHomeBtn )
    ON_BN_CLICKED( IDC_PROBE_INIT_BTN, &MotorSysTestDlg::OnBnClickedProbeInitBtn )

    ON_BN_CLICKED( IDC_FINE_FOCUS_UP_BTN, &MotorSysTestDlg::OnBnClickedFineFocusUpBtn )
    ON_BN_CLICKED( IDC_FINE_FOCUS_DN_BTN, &MotorSysTestDlg::OnBnClickedFineFocusDnBtn )
    ON_BN_CLICKED( IDC_COARSE_FOCUS_UP_BTN, &MotorSysTestDlg::OnBnClickedCoarseFocusUpBtn )
    ON_BN_CLICKED( IDC_COARSE_FOCUS_DN_BTN, &MotorSysTestDlg::OnBnClickedCoarseFocusDnBtn )
    ON_EN_CHANGE ( IDC_FOCUS_POS_EDIT, &MotorSysTestDlg::OnEnChangeFocusPosEdit )
    ON_BN_CLICKED( IDC_FOCUS_GOTO_POS_BTN, &MotorSysTestDlg::OnBnClickedFocusGotoPosBtn )
    ON_BN_CLICKED( IDC_FOCUS_HOME_BTN, &MotorSysTestDlg::OnBnClickedFocusHomeBtn )
    ON_BN_CLICKED( IDC_FOCUS_GOTO_CENTER_BTN, &MotorSysTestDlg::OnBnClickedFocusGotoCenterBtn )

    ON_BN_CLICKED( IDC_INST_TYPE_CHK, &MotorSysTestDlg::OnBnClickedInstTypeChk )
    ON_BN_CLICKED( IDC_LED_RACK1_GOTO_TOP_BTN, &MotorSysTestDlg::OnBnClickedLedRack1GotoTopBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_CAL_TOP_BTN, &MotorSysTestDlg::OnBnClickedLedRack1CalTopBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_GOTO_CTR_BTN, &MotorSysTestDlg::OnBnClickedLedRack1GotoCtrBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_CAL_CTR_BTN, &MotorSysTestDlg::OnBnClickedLedRack1CalCtrBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_GOTO_BOT_BTN, &MotorSysTestDlg::OnBnClickedLedRack1GotoBotBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_CAL_BOT_BTN, &MotorSysTestDlg::OnBnClickedLedRack1CalBotBtn )
    ON_BN_CLICKED( IDC_LED_RACK1_HOME_BTN, &MotorSysTestDlg::OnBnClickedLedRack1HomeBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_GOTO_TOP_BTN, &MotorSysTestDlg::OnBnClickedLedRack2GotoTopBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_CAL_TOP_BTN, &MotorSysTestDlg::OnBnClickedLedRack2CalTopBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_GOTO_CTR_BTN, &MotorSysTestDlg::OnBnClickedLedRack2GotoCtrBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_CAL_CTR_BTN, &MotorSysTestDlg::OnBnClickedLedRack2CalCtrBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_GOTO_BOT_BTN, &MotorSysTestDlg::OnBnClickedLedRack2GotoBotBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_CAL_BOT_BTN, &MotorSysTestDlg::OnBnClickedLedRack2CalBotBtn )
    ON_BN_CLICKED( IDC_LED_RACK2_HOME_BTN, &MotorSysTestDlg::OnBnClickedLedRack2HomeBtn )

    ON_EN_CHANGE ( IDC_ROW_EDIT, &MotorSysTestDlg::OnEnChangeRowEdit )
    ON_EN_CHANGE ( IDC_COL_EDIT, &MotorSysTestDlg::OnEnChangeColEdit )
    ON_BN_CLICKED( IDC_ROW_COL_GOTO_BTN, &MotorSysTestDlg::OnBnClickedGotoRowColBtn )
    ON_BN_CLICKED( IDC_ROW_COL_NEXT_BTN, &MotorSysTestDlg::OnBnClickedNextRowColBtn )
    ON_BN_CLICKED( IDC_UPDATE_CURRENT_BTN, &MotorSysTestDlg::OnBnClickedUpdateCurrentBtn )

    ON_EN_CHANGE ( IDC_TUBE_NUM_EDIT, &MotorSysTestDlg::OnEnChangeTubeNumEdit )
    ON_BN_CLICKED( IDC_TUBE_GOTO_BTN, &MotorSysTestDlg::OnBnClickedGotoTubeBtn )
    ON_BN_CLICKED( IDC_TUBE_NEXT_BTN, &MotorSysTestDlg::OnBnClickedNextTubeBtn )
    ON_BN_CLICKED( IDC_TUBE_FIND_BTN, &MotorSysTestDlg::OnBnClickedFindTubeBtn )

    ON_BN_CLICKED( IDC_RADIO_PLATE_SELECT, &MotorSysTestDlg::OnBnClickedRadioPlateSelect )
    ON_BN_CLICKED( IDC_PLATE_CAL_BTN, &MotorSysTestDlg::OnBnClickedPlateCalBtn )
    ON_BN_CLICKED( IDC_PLATE_EJECT_BTN, &MotorSysTestDlg::OnBnClickedPlateEjectBtn )
    ON_BN_CLICKED( IDC_PLATE_LOAD_BTN, &MotorSysTestDlg::OnBnClickedPlateLoadBtn )
    ON_BN_CLICKED( IDC_PLATE_INIT_BTN, &MotorSysTestDlg::OnBnClickedPlateInitBtn )
    ON_BN_CLICKED( IDC_PLATE_DETECT_BTN, &MotorSysTestDlg::OnBnClickedPlateDetectBtn )
    ON_BN_CLICKED( IDC_PLATE_THETA_ADJUST_LEFT_BTN, &MotorSysTestDlg::OnBnClickedPlateThetaAdjustLeftBtn )
    ON_BN_CLICKED( IDC_PLATE_THETA_ADJUST_RIGHT_BTN, &MotorSysTestDlg::OnBnClickedPlateThetaAdjustRightBtn )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_ADJUST_IN_BTN, &MotorSysTestDlg::OnBnClickedPlateRadiusAdjustInBtn )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_ADJUST_OUT_BTN, &MotorSysTestDlg::OnBnClickedPlateRadiusAdjustOutBtn )
    ON_EN_CHANGE ( IDC_PLATE_RADIUS_BACKLASH_EDIT, &MotorSysTestDlg::OnEnChangePlateRadiusBacklashEdit )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_BACKLASH_SET_BTN, &MotorSysTestDlg::OnBnClickedPlateRadiusBacklashSetBtn )
    ON_EN_CHANGE ( IDC_PLATE_THETA_BACKLASH_EDIT, &MotorSysTestDlg::OnEnChangePlateThetaBacklashEdit )
    ON_BN_CLICKED( IDC_PLATE_THETA_BACKLASH_SET_BTN, &MotorSysTestDlg::OnBnClickedPlateThetaBacklashSetBtn )

    ON_BN_CLICKED( IDC_RADIO_CAROUSEL_SELECT, &MotorSysTestDlg::OnBnClickedRadioCarouselSelect )
    ON_BN_CLICKED( IDC_CAROUSEL_CAL_BTN, &MotorSysTestDlg::OnBnClickedCarouselCalBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_EJECT_BTN, &MotorSysTestDlg::OnBnClickedCarouselEjectBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_LOAD_BTN, &MotorSysTestDlg::OnBnClickedCarouselLoadBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_INIT_BTN, &MotorSysTestDlg::OnBnClickedCarouselInitBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_ADJUST_LEFT_BTN, &MotorSysTestDlg::OnBnClickedCarouselThetaAdjustLeftBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_RADIUS_ADJUST_IN_BTN, &MotorSysTestDlg::OnBnClickedCarouselRadiusAdjustInBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_ADJUST_RIGHT_BTN, &MotorSysTestDlg::OnBnClickedCarouselThetaAdjustRightBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_RADIUS_ADJUST_OUT_BTN, &MotorSysTestDlg::OnBnClickedCarouselRadiusAdjustOutBtn )
    ON_EN_CHANGE ( IDC_CAROUSEL_THETA_BACKLASH_EDIT, &MotorSysTestDlg::OnEnChangeCarouselThetaBacklashEdit )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_BACKLASH_SET_BTN, &MotorSysTestDlg::OnBnClickedCarouselThetaBacklashSetBtn )

    ON_BN_CLICKED( IDC_RUN_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedRunScriptBtn )
    ON_EN_VSCROLL( IDC_SCRIPT_EDIT, &MotorSysTestDlg::OnEnVscrollScriptEdit )
    ON_EN_CHANGE ( IDC_STEP_NUM_DISP, &MotorSysTestDlg::OnEnChangeStepNumDisp )
    ON_BN_CLICKED( IDC_PAUSE_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedPauseScriptBtn )
    ON_BN_CLICKED( IDC_STOP_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedStopScriptBtn )
    ON_EN_CHANGE ( IDC_SCRIPT_PATH_EDIT, &MotorSysTestDlg::OnEnChangeScriptPathEdit )
    ON_BN_CLICKED( IDC_LOAD_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedLoadScriptBtn )
    ON_BN_CLICKED( IDC_CLEAR_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedClearScriptBtn )
    ON_BN_CLICKED( IDC_SAVE_SCRIPT_BTN, &MotorSysTestDlg::OnBnClickedSaveScriptBtn )
    ON_BN_CLICKED( IDOK, &MotorSysTestDlg::OnBnClickedExit )
END_MESSAGE_MAP()


// MotorSysTestDlg message handlers

BOOL MotorSysTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT( ( IDM_ABOUTBOX & 0xFFF0 ) == IDM_ABOUTBOX );
    ASSERT( IDM_ABOUTBOX < 0xF000 );

    CMenu* pSysMenu = GetSystemMenu( FALSE );
    if ( pSysMenu != NULL )
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString( IDS_ABOUTBOX );
        ASSERT( bNameValid );
        if ( !strAboutMenu.IsEmpty() )
        {
            pSysMenu->AppendMenu( MF_SEPARATOR );
            pSysMenu->AppendMenu( MF_STRING, IDM_ABOUTBOX, strAboutMenu );
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon( m_hIcon, TRUE );           // Set big icon
    SetIcon( m_hIcon, FALSE );          // Set small icon

    CString valStr;

    InitDlgInfoFile();
    ConfigDlgVariables();

    scriptEditFont.CreateFont(
                               16,                          // nHeight; 16 pixels; 0 = don't care
                               0,                           // nWidth; don't care
                               0,                           // nEscapement; don't care
                               0,                           // nOrientation; don't care
//                               FW_NORMAL,                   // nWeight
                               FW_THIN,                     // nWeight thinnest possible
                               FALSE,                       // bItalic
                               FALSE,                       // bUnderline
                               0,                           // cStrikeOut
                               ANSI_CHARSET,                // nCharSet
                               OUT_DEFAULT_PRECIS,          // nOutPrecision
                               CLIP_DEFAULT_PRECIS,         // nClipPrecision
                               DEFAULT_QUALITY,             // nQuality
                               FIXED_PITCH | FF_MODERN,     // nPitchAndFamily
                               _T( "Courier New" ) );       // lpszFacename;  Courier New produces a font that scales to the requested 16 pixel height

    cntlEdit = true;

    ScriptEdit.Clear();
    BOOL rdOnly = ( allowEditing ) ? FALSE : TRUE;
    ScriptEdit.SetReadOnly( rdOnly );
    ScriptEdit.SetFont( &scriptEditFont );

    StepNumSpin.SetRange32( 0, 100000 );    // ensure an upper limit larger than any practical script size...
    StepNumSpin.SetPos32( 0 );

    InitControllers();

    ShowRThetaPositions( thetaVal, radiusVal );
    ShowRowColPositions( rowVal, colVal );
    ShowTubeNum( tubeNum );

    int32_t chkState = BST_UNCHECKED;

    RelativeMoveChk.EnableWindow( TRUE );
    if ( doRelativeMove )
    {
        chkState = BST_CHECKED;
    }
    RelativeMoveChk.SetCheck( chkState );

    InitProbeBtn.EnableWindow( TRUE );

    if ( carouselSelected )
    {
        CarouselSelectRadio.SetCheck( BST_CHECKED );
        PlateSelectRadio.SetCheck( BST_UNCHECKED );
    }
    else if ( plateSelected )
    {
        CarouselSelectRadio.SetCheck( BST_UNCHECKED );
        PlateSelectRadio.SetCheck( BST_CHECKED );
    }
    else
    {
        CarouselSelectRadio.SetCheck( BST_UNCHECKED );
        PlateSelectRadio.SetCheck( BST_UNCHECKED );
    }

    PlateInitErrorChk.SetCheck( BST_UNCHECKED );
    CarouselInitErrorChk.SetCheck( BST_UNCHECKED );
    if ( !showInitErrors )
    {
        PlateInitErrorChk.ShowWindow( SW_HIDE );
        CarouselInitErrorChk.ShowWindow( SW_HIDE );
    }

    InstrumentTypeChk.EnableWindow( TRUE );

    SetSamplerControlEnable( TRUE );

    BOOL enable = FALSE;

    if ( InitLedController() )
    {
        instrumentType = MotorSysTestDlg::InstrumentTypeHunter;
    }
    ShowInstrumentType( instrumentType );

    if ( instrumentType == MotorSysTestDlg::InstrumentTypeHunter )
    {
        enable = TRUE;
    }
    SetLedControlEnable( enable );

    SetCarouselControlEnable( carouselSelected );
    SetPlateControlEnable( plateSelected );

    signalUpdateReady = true;
    StartSignalsUpdateThread();

    SaveScriptBtn.EnableWindow( FALSE );        // if shown, it will not be active until an edit is done...
    if ( allowEditing )
    {
        SaveScriptBtn.ShowWindow( SW_SHOW );
    }
    else
    {
        SaveScriptBtn.ShowWindow( SW_HIDE );
    }

    if ( simMode )
    {
        // allow the button to show active if in sim mode
        ShowDoorState( TRUE, TRUE );
    }
    else
    {
        ShowDoorState( doorClosed, reagentHome );
    }

    // set timer to do automatic periodic status updates...
    pUpdTimer->expires_from_now( boost::posix_time::milliseconds( SignalsInterval ), updTimerError );           // should cancel any pending async operations
    pUpdTimer->async_wait( std::bind( &MotorSysTestDlg::UpdateSignalStatus, this, std::placeholders::_1 ) );    // restart the periodic update timer as time from now...

    valStr.Empty();
    valStr.Format( _T( "%d" ), plateRadiusBacklash );
    PlateRadiusBacklashEdit.SetWindowTextW( valStr );
    valStr.Empty();
    valStr.Format( _T( "%d" ), plateThetaBacklash );
    PlateThetaBacklashEdit.SetWindowTextW( valStr );
    valStr.Empty();
    valStr.Format( _T( "%d" ), carouselThetaBacklash );
    CarouselThetaBacklashEdit.SetWindowTextW( valStr );

    cntlEdit = false;

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void MotorSysTestDlg::OnSysCommand( UINT nID, LPARAM lParam )
{
    if ( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand( nID, lParam );
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void MotorSysTestDlg::OnPaint()
{
    if ( IsIconic() )
    {
        CPaintDC dc( this ); // device context for painting

        SendMessage( WM_ICONERASEBKGND, reinterpret_cast<WPARAM>( dc.GetSafeHdc() ), 0 );

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics( SM_CXICON );
        int cyIcon = GetSystemMetrics( SM_CYICON );
        CRect rect;
        GetClientRect( &rect );
        int x = ( rect.Width() - cxIcon + 1 ) / 2;
        int y = ( rect.Height() - cyIcon + 1 ) / 2;

        // Draw the icon
        dc.DrawIcon( x, y, m_hIcon );
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
// the minimized window.
HCURSOR MotorSysTestDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>( m_hIcon );
}

void MotorSysTestDlg::SetAllControlEnable( BOOL action )
{
    ReagentArmUpBtn.EnableWindow( action );
    ReagentArmDownBtn.EnableWindow( action );
    ReagentArmPurgeBtn.EnableWindow( action );

    BOOL enable = action;
    if ( simMode )
    {
        enable = TRUE;          // allow the button to show active if in sim mode
    }
    else
    {
        // check is 'action' is valid for the current condition
        if ( ( doorClosed == FALSE ) || ( reagentHome == FALSE ) )
        {
            enable = FALSE;     // only allow the button to show active if the door is closed and the arm is up
        }
    }
    ReagentDoorReleaseBtn.EnableWindow( enable );

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    FineFocusUpBtn.EnableWindow( action );
    FineFocusDownBtn.EnableWindow( action );
    CoarseFocusUpBtn.EnableWindow( action );
    CoarseFocusDownBtn.EnableWindow( action );
    FocusPosEdit.SetReadOnly( rdOnly );
    GotoFocusPosBtn.EnableWindow( action );
    HomeFocusBtn.EnableWindow( action );
    CenterFocusPosBtn.EnableWindow( action );

    InstrumentTypeChk.EnableWindow( action );

    UpdateBtn.EnableWindow( action );

    SetSamplerControlEnable( action );
    SetLedControlEnable( action );
    SetPlateControlEnable( action );
    SetCarouselControlEnable( action );

    RunScriptBtn.EnableWindow( action );
    StepNumSpin.EnableWindow( action );
    PauseScriptBtn.EnableWindow( action );
    StopScriptBtn.EnableWindow( action );

    LoadScriptBtn.EnableWindow( action );
    if ( allowEditing )
    {
        ScriptEdit.SetReadOnly( rdOnly );
        SaveScriptBtn.EnableWindow( action );
    }
}

void MotorSysTestDlg::SetSamplerControlEnable( BOOL action )
{
    BOOL setAction = FALSE;

    if ( ( plateSelected ) || ( carouselSelected ) )
    {
        setAction = action;
    }

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    ThetaPosEdit.SetReadOnly( rdOnly );
    GotoThetaBtn.EnableWindow( setAction );

    RadiusPosEdit.SetReadOnly( rdOnly );
    GotoRadiusBtn.EnableWindow( setAction );
    GotoRThetaBtn.EnableWindow( setAction );

    RelativeMoveChk.EnableWindow( setAction );

    ProbeUpBtn.EnableWindow( setAction );
    ProbeDownBtn.EnableWindow( setAction );

    if ( ( plateSelected ) || ( carouselSelected ) )
    {
        setAction = TRUE;
    }
    else
    {
        setAction = false;
    }

    HomeThetaBtn.EnableWindow( setAction );
    HomeRadiusBtn.EnableWindow( setAction );
    HomeProbeBtn.EnableWindow( setAction );
//    InitProbeBtn.EnableWindow( setAction );
}

void MotorSysTestDlg::SetLedControlEnable( BOOL action )
{
    BOOL setAction = action;

    if ( instrumentType != MotorSysTestDlg::InstrumentTypeHunter )
    {
        setAction = FALSE;      // do not allow 'enable' of controls unles on a hunter instrument...
    }

    LedRack1Lbl.EnableWindow( setAction );
    Rack1PathDisp.EnableWindow( setAction );
    Rack1TopBtn.EnableWindow( setAction );
    Rack1TopCalBtn.EnableWindow( setAction );
    Rack1CtrBtn.EnableWindow( setAction );
    Rack1CtrCalBtn.EnableWindow( setAction );
    Rack1BotBtn.EnableWindow( setAction );
    Rack1BotCalBtn.EnableWindow( setAction );
    Rack1HomeBtn.EnableWindow( setAction );
    LedRack2Lbl.EnableWindow( setAction );
    Rack2PathDisp.EnableWindow( setAction );
    Rack2TopBtn.EnableWindow( setAction );
    Rack2TopCalBtn.EnableWindow( setAction );
    Rack2CtrBtn.EnableWindow( setAction );
    Rack2CtrCalBtn.EnableWindow( setAction );
    Rack2BotBtn.EnableWindow( setAction );
    Rack2BotCalBtn.EnableWindow( setAction );
    Rack2HomeBtn.EnableWindow( setAction );
}

void MotorSysTestDlg::SetPlateControlEnable( BOOL action )
{
    InitPlateBtn.EnableWindow( action );

    if ( plateControlsInited == false )         // haven't performed a hardware init yet...
    {
        action = FALSE;                         // don't enable the control buttons until the hardware has been initialized
    }

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    RowValEdit.SetReadOnly( rdOnly );
    GotoRowColBtn.EnableWindow( action );
    ColValEdit.SetReadOnly( rdOnly );
    NextRowColBtn.EnableWindow( action );

    CalibratePlateBtn.EnableWindow( action );
    EjectPlateBtn.EnableWindow( action );
    LoadPlateBtn.EnableWindow( action );
    DetectPlateBtn.EnableWindow( action );

    PlateThetaAdjustLeftBtn.EnableWindow( action );
    PlateThetaAdjustRightBtn.EnableWindow( action );
    PlateRadiusAdjustInBtn.EnableWindow( action );
    PlateRadiusAdjustOutBtn.EnableWindow( action );

    PlateRadiusBacklashEdit.SetReadOnly( rdOnly );
    PlateRadiusBacklashSetBtn.EnableWindow( action );
    PlateThetaBacklashEdit.SetReadOnly( rdOnly );
    PlateThetaBacklashSetBtn.EnableWindow( action );
}

void MotorSysTestDlg::SetCarouselControlEnable( BOOL action )
{
    InitCarouselBtn.EnableWindow( action );

    if ( carouselControlsInited == false )      // haven't performed a hardware init yet...
    {
        action = FALSE;                         // don't enable the control buttons until the hardware has been initialized
    }

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    TubeNumEdit.SetReadOnly( rdOnly );
    GotoTubeNumBtn.EnableWindow( action );
    NextTubeBtn.EnableWindow( action );
    FindTubeBtn.EnableWindow( action );
    TubePresentChk.EnableWindow( action );

    CalibrateCarouselBtn.EnableWindow( action );
    EjectCarouselBtn.EnableWindow( action );
    LoadCarouselBtn.EnableWindow( action );

    CarouselThetaAdjustLeftBtn.EnableWindow( action );
    CarouselThetaAdjustRightBtn.EnableWindow( action );
    CarouselRadiusAdjustInBtn.EnableWindow( action );
    CarouselRadiusAdjustOutBtn.EnableWindow( action );

    CarouselThetaBacklashEdit.SetReadOnly( rdOnly );
    CarouselThetaBacklashSetBtn.EnableWindow( action );
}

void MotorSysTestDlg::SetMsgBoxControlEnable( bool enable, MsgBoxTypes msg_type )
{
    BOOL action = ( enable == true ) ? 1 : 0;

    switch ( msgType )
    {
        case MsgBoxTypeThetaZero:
            if ( plateSelected )
            {
                PlateThetaAdjustLeftBtn.EnableWindow( action );
                PlateThetaAdjustRightBtn.EnableWindow( action );
            }
            else
            {
                CarouselThetaAdjustLeftBtn.EnableWindow( action );
                CarouselThetaAdjustRightBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeRadiusZero:
            if ( plateSelected )
            {
                PlateRadiusAdjustInBtn.EnableWindow( action );
                PlateRadiusAdjustOutBtn.EnableWindow( action );
            }
            else
            {
                CarouselRadiusAdjustInBtn.EnableWindow( action );
                CarouselRadiusAdjustOutBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeThetaCal:
            if ( plateSelected )
            {
                PlateThetaAdjustLeftBtn.EnableWindow( action );
                PlateThetaAdjustRightBtn.EnableWindow( action );
            }
            else
            {
                CarouselThetaAdjustLeftBtn.EnableWindow( action );
                CarouselThetaAdjustRightBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeRadiusCal:
            if ( plateSelected )
            {
                PlateRadiusAdjustInBtn.EnableWindow( action );
                PlateRadiusAdjustOutBtn.EnableWindow( action );
            }
            else
            {
                CarouselRadiusAdjustInBtn.EnableWindow( action );
                CarouselRadiusAdjustOutBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeCalibratePlate:
            if ( plateSelected )
            {
                PlateThetaAdjustLeftBtn.EnableWindow( action );
                PlateThetaAdjustRightBtn.EnableWindow( action );
                PlateRadiusAdjustInBtn.EnableWindow( action );
                PlateRadiusAdjustOutBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeCalibrateCarousel:
            if ( carouselSelected )
            {
                CarouselThetaAdjustLeftBtn.EnableWindow( action );
                CarouselThetaAdjustRightBtn.EnableWindow( action );
                CarouselRadiusAdjustInBtn.EnableWindow( action );
                CarouselRadiusAdjustOutBtn.EnableWindow( action );
            }
            break;

        case MsgBoxTypeNone:
        default:
            break;
    }
}

void MotorSysTestDlg::UpdateSignalStatus( const boost::system::error_code error )
{
    if ( error == boost::asio::error::operation_aborted )
    {
        Logger::L().Log ( MODULENAME, severity_level::debug2, "canceling Signal Update timer" );
        updTimerError.clear();
        if ( quitting )
        {
            timerComplete = true;
        }
        return;
    }

    if ( !quitting )
    {
        signalUpdateReady = true;

        pUpdTimer->expires_from_now( boost::posix_time::milliseconds( SignalsInterval ), updTimerError );               // should cancel any pending async operations
        pUpdTimer->async_wait( std::bind( &MotorSysTestDlg::UpdateSignalStatus, this, std::placeholders::_1 ) );        // restart the periodic update timer as time from now...
    }
    else
    {
        timerComplete = true;
    }
}

void MotorSysTestDlg::UpdateSignals( void )
{
    if ( !quitting )
    {
        if ( cbiInit )
        {
            SignalStatus instrumentSignals = pCbi_->GetSignalStatus();

            carouselPresent = ( instrumentSignals.isSet( SignalStatus::CarouselPresent ) ) ? TRUE : FALSE;
            if ( carouselPresent )
            {
                tubePresent = ( instrumentSignals.isSet( SignalStatus::CarouselTube ) ) ? TRUE : FALSE;
            }
            else
            {
                tubePresent = FALSE;
            }
            thetaHome = ( instrumentSignals.isSet( SignalStatus::ThetaMotorHome ) ) ? TRUE : FALSE;
            radiusHome = ( instrumentSignals.isSet( SignalStatus::RadiusMotorHome ) ) ? TRUE : FALSE;
            probeHome = ( instrumentSignals.isSet( SignalStatus::ProbeMotorHome ) ) ? TRUE : FALSE;

            doorClosed = ( instrumentSignals.isSet( SignalStatus::ReagentDoorClosed ) ) ? TRUE : FALSE;
            packPresent = ( instrumentSignals.isSet( SignalStatus::ReagentPackInstalled ) ) ? TRUE : FALSE;
            reagentHome = ( instrumentSignals.isSet( SignalStatus::ReagentMotorHome ) ) ? TRUE : FALSE;
            reagentLimit = ( instrumentSignals.isSet( SignalStatus::ReagentMotorLimit ) ) ? TRUE : FALSE;

            focusHome = ( instrumentSignals.isSet( SignalStatus::FocusMotorHome ) ) ? TRUE : FALSE;

            if ( instrumentType == MotorSysTestDlg::InstrumentTypeHunter )
            {
                rack1Home = ( instrumentSignals.isSet( SignalStatus::Rack1MotorHome ) ) ? TRUE : FALSE;
                rack2Home = ( instrumentSignals.isSet( SignalStatus::Rack2MotorHome ) ) ? TRUE : FALSE;
            }
            else
            {
                rack1Home = FALSE;
                rack2Home = FALSE;
            }

            int32_t tVal = thetaVal;
            int32_t rVal = radiusVal;
            int32_t pVal = probeVal;
            int32_t tNum = tubeNum;
            int32_t rNum = rowVal;
            int32_t cNum = colVal;

            if ( !quitting )
            {
                GetRThetaPositions( tVal, rVal );
                GetProbePosition( pVal );
                GetTubeNum( tNum );
                GetRowColPositions( rNum, cNum );
            }
        }
        else
        {
            carouselPresent = FALSE;
            tubePresent = FALSE;
            thetaHome = FALSE;
            radiusHome = FALSE;
            probeHome = FALSE;
            doorClosed = FALSE;
            packPresent = FALSE;
            reagentHome = FALSE;
            reagentLimit = FALSE;
            focusHome = FALSE;
            rack1Home = FALSE;
            rack2Home = FALSE;
        }
    }
}

// start a persistent thread that loops displaying the positing and sensor info...
void MotorSysTestDlg::StartSignalsUpdateThread( void )
{
    if ( !quitting )
    {
        if ( ( !signalUpdateStarting ) && ( !signalUpdateRunning ) )
        {
            signalUpdateStarting = true;
            pSignalsThread.reset( new std::thread( &MotorSysTestDlg::ShowSignals, this ) );
            pSignalsThread->detach();
        }
    }
}

void MotorSysTestDlg::ShowSignals( void )
{
    if ( !signalUpdateRunning )
    {
        int showIdx = 0;
        bool doShow = false;

        signalUpdateRunning = true;
        signalUpdateStarting = false;

        while ( !quitting )
        {
            if ( signalUpdateReady )
            {
                signalUpdateReady = false;

                UpdateSignals();

                showIdx = 0;
                doShow = true;

                while ( ( !quitting ) && ( doShow ) && ( showIdx >= 0 ) )
                {
                    switch ( showIdx )
                    {
                        case 0:
                            // update the sensor states
                            ShowCarouselPresent( carouselPresent );
                            break;

                        case 1:
                            ShowTubePresent( tubePresent );
                            break;

                        case 2:
                            ShowHomeStates();
                            break;

                        case 3:
                            ShowDoorState( doorClosed, reagentHome );
                            break;

                        case 4:
                            ShowReagentPresent( packPresent );
                            break;

                        case 5:
                            // update the positional info, also
                            ShowRThetaPositions( thetaVal, radiusVal );
                            break;

                        case 6:
                            ShowProbePosition( probeVal );
                            break;

                        case 7:
                            ShowTubeNum( tubeNum );
                            break;

                        case 8:
                            ShowRowColPositions( rowVal, colVal );
                            break;

                        default:
                            doShow = false;
                            showIdx = -1;
                            break;
                    }

                    if ( ( !quitting ) && ( doShow ) && ( showIdx >= 0 ) )
                    {
                        showIdx++;
                    }
                    else
                    {
                        doShow = false;
                        showIdx = -1;
                    }
                }
            }

            if ( !quitting )
            {
                ScriptSleep( SignalsInterval / 5 ); // check for updte at 5x the possible update refresh rate, but yield the thread to others while 'sleeping'
            }
        }

        signalUpdateRunning = false;
        signalUpdateStarting = false;
    }
}

void MotorSysTestDlg::ShowReagentPresent( BOOL present )
{
    int32_t state = BST_UNCHECKED;
    if ( present == TRUE )
    {
        state = BST_CHECKED;
    }

    if ( !quitting )
    {
        PackDetectedChk.SetCheck( state );
    }
}

void MotorSysTestDlg::ShowDoorState( BOOL door_closed, BOOL arm_home )
{
    int32_t state = BST_UNCHECKED;
    BOOL showDoorBtn = FALSE;

    if ( door_closed == TRUE )
    {
        state = BST_CHECKED;
    }

    if ( simMode )
    {
        showDoorBtn = TRUE;
    }
    else
    {
        if ( ( door_closed == TRUE ) && ( arm_home == TRUE ) )
        {
            showDoorBtn = TRUE;
        }
    }

    if ( !quitting )
    {
        DoorClosedChk.SetCheck( state );
        ReagentDoorReleaseBtn.EnableWindow( showDoorBtn );
    }
}

void MotorSysTestDlg::ShowHomeStates( void )
{
    int32_t reagentHomeState    = BST_UNCHECKED;
    int32_t reagentLimitState   = BST_UNCHECKED;
    int32_t thetaState          = BST_UNCHECKED;
    int32_t radiusState         = BST_UNCHECKED;
    int32_t probeState          = BST_UNCHECKED;
    int32_t focusState          = BST_UNCHECKED;
    int32_t rack1State          = BST_UNCHECKED;
    int32_t rack2State          = BST_UNCHECKED;

    if ( reagentHome == TRUE )
    {
        reagentHomeState = BST_CHECKED;
    }

    if ( reagentLimit == TRUE )
    {
        reagentLimitState = BST_CHECKED;
    }

    if ( thetaHome == TRUE )
    {
        thetaState = BST_CHECKED;
    }

    if ( radiusHome == TRUE )
    {
        radiusState = BST_CHECKED;
    }

    if ( probeHome == TRUE )
    {
        probeState = BST_CHECKED;
    }

    if ( focusHome == TRUE )
    {
        focusState = BST_CHECKED;
    }

    if ( rack1Home == TRUE )
    {
        rack1State = BST_CHECKED;
    }

    if ( rack2Home == TRUE )
    {
        rack2State = BST_CHECKED;
    }

    if ( !quitting )
    {
        ReagentArmHomeChk.SetCheck( reagentHomeState );
        ReagentArmLimitChk.SetCheck( reagentLimitState );
        ThetaHomeChk.SetCheck( thetaState );
        RadiusHomeChk.SetCheck( radiusState );
        ProbeHomeChk.SetCheck( probeState );
        FocusHomeChk.SetCheck( focusState );
        Rack1HomeChk.SetCheck( rack1State );
        Rack2HomeChk.SetCheck( rack2State );
    }
}

void MotorSysTestDlg::UpdateProbePosition( void )
{
    int32_t probePos = probeVal;

    GetProbePosition( probePos );
    ShowProbePosition( probePos );
}

void MotorSysTestDlg::GetProbePosition( int32_t probe_pos )
{
    if ( !simMode )
    {
        if ( carouselSelected )
        {
            probeVal = pCarouselController->GetProbePosition();     // get the position
        }
        else if ( plateSelected )
        {
            probeVal = pPlateController->GetProbePosition();        // get the position
        }
        else
        {
            pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
            probeVal = pPlateProbe->GetPosition();                  // get the position
        }
    }
    probe_pos = probeVal;
}

void MotorSysTestDlg::ShowProbePosition( int32_t probe_pos )
{
    CString valStr;

    valStr.Format( _T( "%d" ), probe_pos );
    cntlEdit = true;
    CurrentProbeDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void MotorSysTestDlg::UpdateFocusPosition( void )
{
    focusVal = pFocusController->Position();

    ShowFocusPosition( focusVal );
}

void MotorSysTestDlg::ClearFocusPosition( void )
{
    cntlEdit = true;

    CurrentFocusDisp.Clear();

    cntlEdit = false;
}

void MotorSysTestDlg::ShowFocusPosition( int32_t pos )
{
    cntlEdit = true;

    CString valStr;
    valStr.Empty();
    valStr.Format( _T( "%d" ), pos );
    CurrentFocusDisp.SetWindowText( valStr );

    cntlEdit = false;
}

void MotorSysTestDlg::ShowInstrumentType( MotorSysTestDlg::InstrumentTypes type )
{
    bool showCntls = FALSE;

    if ( instrumentType == MotorSysTestDlg::InstrumentTypeHunter )
    {
        InstrumentTypeChk.SetCheck( BST_CHECKED );
        showCntls = TRUE;
    }
    else if ( instrumentType == MotorSysTestDlg::InstrumentTypeScout )
    {
        InstrumentTypeChk.SetCheck( BST_UNCHECKED );
    }
    else
    {
        InstrumentTypeChk.SetCheck( BST_INDETERMINATE );
    }

    SetLedControlEnable( showCntls );
}

void MotorSysTestDlg::UpdateRackPositions( void )
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

void MotorSysTestDlg::UpdateStep( int stepIdx )
{
    cntlEdit = true;

    ShowStepNum( stepIdx + 1 );

    cntlEdit = false;
}

void MotorSysTestDlg::ShowStepNum( int lineNum )
{
    CString valStr;

    StepNumSpin.SetPos32( lineNum );
    valStr.Empty();
    if ( lineNum >= 0 )
    {
        valStr.Format( _T( "%d" ), lineNum );
    }
    StepNumDisp.SetWindowText( valStr );

    cntlEdit = false;
}

void MotorSysTestDlg::UpdateRThetaPositions( void )
{
    int32_t tVal = thetaVal;
    int32_t rVal = radiusVal;

    GetRThetaPositions( tVal, rVal );
    ShowRThetaPositions( tVal, rVal );
}

void MotorSysTestDlg::GetRThetaPositions( int32_t & tVal, int32_t & rVal )
{
    if ( !simMode )
    {
        if ( plateSelected )
        {
            pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
            pPlateController->PlatePosition( thetaVal, radiusVal );
        }
        else
        {
            pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
            thetaVal = pCarouselTheta->GetPosition();
            radiusVal = pCarouselRadius->GetPosition();
        }

    }
    tVal = thetaVal;
    rVal = radiusVal;
}

void MotorSysTestDlg::ShowRThetaPositions( int32_t tPos, int32_t rPos )
{
    CString valStr;

    cntlEdit = true;

    valStr.Format( _T( "%d" ), tPos );
    CurrentThetaDisp.SetWindowText( valStr );
    valStr.Format( _T( "%d" ), rPos );
    CurrentRadiusDisp.SetWindowText( valStr );

    cntlEdit = false;
}

void MotorSysTestDlg::UpdateRowColPositions( void )
{
    int32_t row = rowVal;
    int32_t col = colVal;

    GetRowColPositions( row, col );
    ShowRowColPositions( row, col );
}

void MotorSysTestDlg::GetRowColPositions( int32_t & row, int32_t & col )
{
    if ( plateSelected )
    {
        uint32_t row_ = rowVal;
        uint32_t col_ = colVal;

        if ( !simMode )
        {
            pPlateController->GetCurrentRowCol( row_, col_ );
            rowVal = row_;
            colVal = col_;
        }
        row = rowVal;
        col = colVal;
    }
}

void MotorSysTestDlg::ShowRowColPositions( int32_t row, int32_t col )
{
    CString rowValStr;
    CString colValStr;
    int32_t rowIdx = 0;

    rowValStr.Empty();
    colValStr.Empty();

    if ( plateSelected )
    {
        rowIdx = row - 1;

        if ( ( rowIdx >= 0 ) && ( rowIdx < MaxPlateRowNum ) )
        {
            _TCHAR baseChar = 'A';
            _TCHAR rowChar = 'A';

            rowChar = baseChar + rowIdx;
            rowValStr.Format( _T( "%c" ), rowChar );
        }

        if ( col > 0 )
        {
            colValStr.Format( _T( "%d" ), col );
        }
    }

    cntlEdit = true;

    CurrentRowDisp.SetWindowText( rowValStr );
    CurrentColDisp.SetWindowText( colValStr );

    cntlEdit = false;
}

void MotorSysTestDlg::UpdateTubeNum( void )
{
    int32_t tube = tubeNum;

    GetTubeNum( tube );
    ShowTubeNum( tube );
}

void MotorSysTestDlg::GetTubeNum( int32_t & tube )
{
    if ( carouselSelected )
    {
        tubeNum = pCarouselController->GetCurrentTubeNum();
    }
    else
    {
        tubeNum = 0;
    }
    tube = tubeNum;
}

void MotorSysTestDlg::ShowTubeNum( int32_t tube_num )
{
    CString valStr;

    valStr.Format( _T( "%d" ), tube_num );

    cntlEdit = true;
    CurrentTubeDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void MotorSysTestDlg::ShowPlateDetected( BOOL found )
{
    int32_t state = BST_UNCHECKED;

    if ( found == TRUE )
    {
        state = BST_CHECKED;
    }

    if ( !quitting )
    {
        PlateDetectedChk.SetCheck( state );
    }
}

void MotorSysTestDlg::ShowCarouselPresent( BOOL present )
{
    int32_t state = BST_UNCHECKED;

    if ( present == TRUE )
    {
        state = BST_CHECKED;
    }

    if ( !quitting )
    {
        CarouselPresentChk.SetCheck( state );
    }
}

void MotorSysTestDlg::ShowTubePresent( BOOL present )
{
    int32_t state = BST_UNCHECKED;

    if ( present == TRUE )
    {
        state = BST_CHECKED;
    }

    if ( !quitting )
    {
        TubePresentChk.SetCheck( state );
    }
}

void MotorSysTestDlg::DoPlateSelect( bool msgWait )
{
    if ( !cntlEdit )
    {
        cntlEdit = true;    // prevent re-entry caused by control manipulation

        if ( carouselSelected )
        {
            if ( CarouselSelectRadio.GetCheck() == BST_CHECKED )
            {
                CarouselSelectRadio.SetCheck( BST_UNCHECKED );
            }
            carouselSelected = FALSE;
            carouselControlsInited = false;
            CarouselInitErrorChk.SetCheck( BST_UNCHECKED );
            SetCarouselControlEnable( carouselSelected );
            UpdateTubeNum();
        }

        if ( !plateSelected )
        {
            if ( pPlateController->ControllerOk() )
            {
                if ( PlateSelectRadio.GetCheck() == BST_UNCHECKED )
                {
                    PlateSelectRadio.SetCheck( BST_CHECKED );
                }
                plateSelected = TRUE;
                plateControlsInited = false;
                plateFound = false;
                PlateInitErrorChk.SetCheck( BST_UNCHECKED );
                SetSamplerControlEnable( FALSE );
                ShowPlateDetected( plateFound );

                if ( !simMode )
                {
                    pPlateController->Reinit();             // initialize the motor parameters
                }

                pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
                if ( pPlateTheta )
                {
                    int32_t units = 0;
                    int32_t ratio = 0;
                    time_t t_now = 0;
                    time_t t_start = time( 0 );

                    if ( !simMode )
                    {
                        do
                        {
                            ScriptSleep( 100 );
                            units = pPlateTheta->GetUnitsPerRev();
                            ratio = pPlateTheta->GetGearRatio();
                            maxPlateThetaPos = ( units * ratio );
                            t_now = time( 0 );
                        } while ( ( ( units == 0 ) || ( ratio == 0 ) ) && ( difftime( t_now, t_start ) < 5 ) );
                    }

                    if ( maxPlateThetaPos == 0 )
                    {
                        maxPlateThetaPos = MaxThetaPosition;
                    }
                }
                SetPlateControlEnable( plateSelected );

                CString valStr;

                tubeNum = 0;
                rowVal = 0;
                colVal = 0;
                thetaVal = 0;
                radiusVal = 0;

                TubePresentChk.SetCheck( BST_UNCHECKED );

                valStr.Empty();
                CurrentTubeDisp.SetWindowText( valStr );
                CurrentRowDisp.SetWindowText( valStr );
                CurrentColDisp.SetWindowText( valStr );
                CurrentThetaDisp.SetWindowText( valStr );
                CurrentRadiusDisp.SetWindowText( valStr );
            }
            else
            {
                if ( PlateSelectRadio.GetCheck() == BST_CHECKED )
                {
                    PlateSelectRadio.SetCheck( BST_UNCHECKED );
                }
                PlateInitErrorChk.SetCheck( BST_UNCHECKED );
                plateSelected = FALSE;
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::DoCarouselSelect( bool msgWait )
{
    if ( !cntlEdit )
    {
        cntlEdit = true;    // prevent re-entry caused by control manipulation

        if ( plateSelected )
        {
            if ( PlateSelectRadio.GetCheck() == BST_CHECKED )
            {
                PlateSelectRadio.SetCheck( BST_UNCHECKED );
            }
            plateSelected = FALSE;
            plateControlsInited = false;
            plateFound = false;
            SetPlateControlEnable( plateSelected );
            UpdateRowColPositions();
            ShowPlateDetected( plateFound );
        }

        if ( !carouselSelected )
        {
            if ( pCarouselController->ControllerOk() )
            {
                if ( CarouselSelectRadio.GetCheck() == BST_UNCHECKED )
                {
                    CarouselSelectRadio.SetCheck( BST_CHECKED );
                }
                carouselSelected = TRUE;
                carouselControlsInited = false;
                CarouselInitErrorChk.SetCheck( BST_UNCHECKED );
                SetSamplerControlEnable( FALSE );

                if ( !simMode )
                {
                    pCarouselController->Reinit();              // initialize the motor parameters
                }

                pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
                if ( pCarouselTheta )
                {
                    int32_t units = 0;
                    int32_t ratio = 0;
                    time_t t_now = 0;
                    time_t t_start = time( 0 );

                    if ( !simMode )
                    {
                        do
                        {
                            ScriptSleep( 100 );
                            units = pCarouselTheta->GetUnitsPerRev();
                            ratio = pCarouselTheta->GetGearRatio();
                            maxCarouselThetaPos = ( units * ratio );
                            t_now = time( 0 );
                        } while ( ( ( units == 0 ) || ( ratio == 0 ) ) && ( difftime( t_now, t_start ) < 5 ) );
                    }

                    if ( maxCarouselThetaPos == 0 )
                    {
                        maxCarouselThetaPos = MaxThetaPosition;
                    }
                }
                SetCarouselControlEnable( carouselSelected );

                CString valStr;

                tubeNum = 0;
                thetaVal = 0;
                radiusVal = 0;

                TubePresentChk.SetCheck( BST_UNCHECKED );

                valStr.Empty();
                CurrentTubeDisp.SetWindowText( valStr );
                CurrentRowDisp.SetWindowText( valStr );
                CurrentColDisp.SetWindowText( valStr );
                CurrentThetaDisp.SetWindowText( valStr );
                CurrentRadiusDisp.SetWindowText( valStr );
            }
            else
            {
                if ( CarouselSelectRadio.GetCheck() == BST_CHECKED )
                {
                    CarouselSelectRadio.SetCheck( BST_UNCHECKED );
                }
                CarouselInitErrorChk.SetCheck( BST_UNCHECKED );
                carouselSelected = FALSE;
            }
        }

        cntlEdit = false;
    }
}

bool MotorSysTestDlg::DoGotoThetaPos( bool relativeMove )
{
    std::string controllerName;
    std::string methodName;
    bool moveOk = true;
    int32_t tPos = 0;
    int32_t rPos = 0;

    if ( plateSelected )
    {
        controllerName = "PlateController";
        methodName = "GoTo";

        if ( !simMode )
        {
            pPlateController->PlatePosition( tPos, rPos );      // get the current position value to leave the radius at current location
            tPos = thetaEditVal;
            if ( doRelativeMove )
            {
                moveOk = pPlateController->MoveTo( tPos, rPos );
            }
            else
            {
                moveOk = pPlateController->GoTo( tPos, rPos );
            }
        }
    }
    else
    {
        bool isHome = false;

        if ( !simMode )
        {
            isHome = true;
        }

        if ( doRelativeMove )
        {
            methodName = "Motor->MovePosRelative";
        }
        else
        {
            methodName = "Motor->SetPosition";
        }

        pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );

        if ( carouselSelected )
        {
            controllerName = "CarouselController";
            if ( !simMode )
            {
                isHome = pCarouselController->IsProbeUp();
            }
        }
        else
        {
            controllerName = "non-initialized";
            if ( ( !simMode ) && ( pCarouselProbe ) )
            {
                isHome = pCarouselProbe->IsHome();
            }
        }

        if ( !isHome )
        {
            OnBnClickedProbeUpBtn();
        }

        if ( !simMode )
        {
            if ( pCarouselTheta )
            {
                int32_t tgtPos = pCarouselTheta->GetPosition();

                pCarouselTheta->ClearErrors();
                if ( doRelativeMove )
                {
                    pCarouselTheta->MovePosRelative( thetaEditVal );
                    tgtPos += thetaEditVal;
                }
                else
                {
                    pCarouselTheta->SetPosition( thetaEditVal );
                    tgtPos = thetaEditVal;
                }
                pCarouselTheta->DoMotorWait();
                ScriptSleep( DefaultPosUpdateStdInterval );
                tPos = pCarouselTheta->GetPosition();
                moveOk = pCarouselTheta->PosAtTgt( tPos, tgtPos );
            }
        }
    }

    if ( !moveOk )
    {
        CString valStr;
        std::string logStr = boost::str( boost::format( "DoGotoThetaPos: %s returned an error from '%s'." ) % controllerName % methodName );

        errorCnt++;
        valStr.Format( _T( "%d" ), errorCnt );
        ErrorCntDisp.SetWindowText( valStr );
        Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    }

    if ( carouselSelected )
    {
        UpdateTubeNum();
    }

    UpdateRThetaPositions();

    return moveOk;
}

bool MotorSysTestDlg::DoGotoRadiusPos( bool relativeMove )
{
    std::string controllerName;
    std::string methodName;
    bool moveOk = true;
    int32_t tPos = 0;
    int32_t rPos = 0;

    if ( plateSelected )
    {
        controllerName = "PlateController";
        methodName = "GoTo";

        if ( !simMode )
        {
            pPlateController->PlatePosition( tPos, rPos );      // get the current position value to leave the theta at current location
            rPos = radiusEditVal;
            moveOk = pPlateController->GoTo( tPos, rPos );
        }
    }
    else
    {
        bool isHome = false;

        if ( simMode )
        {
            isHome = true;
        }

        if ( doRelativeMove )
        {
            methodName = "Motor->MovePosRelative";
        }
        else
        {
            methodName = "Motor->SetPosition";
        }

        pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );

        if ( carouselSelected )
        {
            controllerName = "CarouselController";
            if ( !simMode )
            {
                isHome = pCarouselController->IsProbeUp();
            }
        }
        else
        {
            controllerName = "non-initialized";
            if ( ( !simMode ) && ( pCarouselProbe ) )
            {
                isHome = pCarouselProbe->IsHome();
            }
        }

        if ( !isHome )
        {
            OnBnClickedProbeUpBtn();
        }

        if ( !simMode )
        {
            if ( pCarouselRadius )
            {
                int32_t tgtPos = pCarouselRadius->GetPosition();

                pCarouselRadius->ClearErrors();
                if ( doRelativeMove )
                {
                    pCarouselRadius->MovePosRelative( radiusEditVal );
                    tgtPos += radiusEditVal;
                }
                else
                {
                    pCarouselRadius->SetPosition( radiusEditVal );
                    tgtPos = radiusEditVal;
                }
                pCarouselRadius->DoMotorWait();
                ScriptSleep( DefaultPosUpdateStdInterval );
                rPos = pCarouselRadius->GetPosition();
                moveOk = pCarouselRadius->PosAtTgt( rPos, tgtPos );
            }
        }
    }

    if ( !moveOk )
    {
        CString valStr;
        std::string logStr = boost::str( boost::format( "DoGotoRadiusPos: %s returned an error from '%s'." ) % controllerName % methodName );

        errorCnt++;
        valStr.Format( _T( "%d" ), errorCnt );
        ErrorCntDisp.SetWindowText( valStr );
        Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    }

    if ( carouselSelected )
    {
        UpdateTubeNum();
    }
    UpdateRThetaPositions();

    return moveOk;
}

bool MotorSysTestDlg::DoGotoRThetaPos( bool relativeMove )
{
    std::string controllerName;
    std::string methodName;
    bool moveOk = false;
    int32_t tPos = 0;
    int32_t rPos = 0;
    int32_t maxThetaPos = MaxThetaPosition;

    if ( carouselSelected )
    {
        controllerName = "CarouselController";
        maxThetaPos = maxCarouselThetaPos;
    }
    else if ( plateSelected )
    {
        controllerName = "PlateController";
        maxThetaPos = maxPlateThetaPos;
    }
    else
    {
        controllerName = "non-initialized";
        maxThetaPos = maxCarouselThetaPos;
    }

    if ( ( thetaEditVal <= ( 2 * maxThetaPos ) ) && ( thetaEditVal > -( maxThetaPos ) ) &&
        ( radiusEditVal <= plateRadiusMaxTravel ) && ( radiusEditVal >= -( plateRadiusMaxTravel / 4 ) ) )      // limit negative value inputs...
    {
        methodName = "GotoThetaPos/GotoRadiusPos";

        moveOk = DoGotoThetaPos( relativeMove );
        if ( moveOk )
        {
            moveOk = DoGotoRadiusPos( relativeMove );
        }

        if ( plateSelected )
        {
            UpdateRowColPositions();
        }

        if ( !moveOk )
        {
            CString valStr;
            std::string logStr = boost::str( boost::format( "DoGotoRThetaPos: %s returned an error from '%s'." ) % controllerName % methodName );

            errorCnt++;
            valStr.Format( _T( "%d" ), errorCnt );
            ErrorCntDisp.SetWindowText( valStr );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
        }

        UpdateRThetaPositions();
    }

    return moveOk;
}

void MotorSysTestDlg::DoGotoTubeBtn( bool msgWait )
{
    if ( carouselSelected )
    {
        CString valStr;

        if ( ( tubeEditVal > 0 ) && ( tubeEditVal <= MaxCarouselTubes ) )
        {
            if ( !simMode )
            {
                if ( !pCarouselController->GotoTubeNum( tubeEditVal ) )
                {
                    std::string logStr = "DoGotoTubeBtn: CarouselController returned an error from 'GotoTubeNum'.";

                    errorCnt++;
                    valStr.Format( _T( "%d" ), errorCnt );
                    ErrorCntDisp.SetWindowText( valStr );
                    Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
                }
            }
            UpdateTubeNum();
        }
        else
        {
            if ( msgWait )
            {
                valStr.Format( _T( "Illegal tube number, or no tube number entered: %d" ), tubeEditVal );
                MessageBox( valStr, _T( "Illegal Tube Number" ) );
            }
        }
    }
}

void MotorSysTestDlg::DoPlateInit( bool msgWait )
{
    if ( plateSelected )
    {
        if ( msgWait )
        {
            MessageBox( _T( "SYSTEM WILL INITIALIZE PLATE POSITIONING REFERENCE INFORMATION...\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Initialize Plate positioning" ) );
        }

        if ( !simMode )
        {
            PlateInitErrorChk.SetCheck( BST_UNCHECKED );
            plateControlsInited = false;

            bool initOk = pPlateController->InitPlate();              // initialize the motor parameters and initialize the controller's positioning references

            std::string logStr = boost::str( boost::format( "DoPlateInit:: initialization %s " ) % ( ( initOk == true ) ? "succeeded" : " failed" ) );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

            if ( ( !initOk ) && ( showInitErrors ) )
            {
                PlateInitErrorChk.SetCheck( BST_CHECKED );
            }
            else
            {
                initOk = true;
            }

            if ( ( initOk ) && ( !plateControlsInited ) )
            {
                plateControlsInited = true;
            }
            SetPlateControlEnable( plateSelected );
            SetSamplerControlEnable( initOk );

            UpdateRowColPositions();
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::DoPlateDetect( bool msgWait )
{
    if ( plateSelected )
    {
        if ( msgWait )
        {
            MessageBox( _T( "SYSTEM WILL ATTEMPTY TO DETECT A PLATE ON THE CARRIER...\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Initialize Plate positioning" ) );
        }

        if ( !simMode )
        {
            PlateDetectedChk.SetCheck( BST_UNCHECKED );

            plateFound = pPlateController->IsPlatePresent();                // do a positional touch to detect a plate

        }
        ShowPlateDetected( plateFound );
    }
}

void MotorSysTestDlg::DoCarouselInit( bool msgWait )
{
    if ( carouselSelected )
    {
        if ( msgWait )
        {
            MessageBox( _T( "SYSTEM WILL INITIALIZE CAROUSEL POSITIONING REFERENCE INFORMATION...\r\n\r\nENSURE CAROUSEL IS REMOVED TO PREVENT TUBE LOSS\r\n\r\nPress 'Enter' or click/press the 'OK' button when done." ),
                        _T( "Initialize Carousel positioning" ) );
        }

        if ( !simMode )
        {
            CarouselInitErrorChk.SetCheck( BST_UNCHECKED );
            carouselControlsInited = false;

            bool initOk = pCarouselController->InitCarousel();        // initialize the motor parameters and initialize the controller's positioning references

            std::string logStr = boost::str( boost::format( "DoCarouselInit:: initialization %s " ) % ( ( initOk == true ) ? "succeeded" : " failed" ) );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

            if ( ( !initOk ) && ( showInitErrors ) )
            {
                CarouselInitErrorChk.SetCheck( BST_CHECKED );
            }
            else
            {
                initOk = true;
            }

            if ( ( initOk ) && ( !carouselControlsInited ) )
            {
                carouselControlsInited = true;
            }
            SetCarouselControlEnable( carouselSelected );
            SetSamplerControlEnable( initOk );
        }
        UpdateTubeNum();
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::StartInfoDialog( void )
{
    if ( ( !dlgStarting ) && ( !dlgRunning ) && ( !scriptStarting ) && ( !scriptRunning ) )
    {
        SetAllControlEnable( FALSE );                       // disable ALL controls first
        PauseScriptBtn.EnableWindow( FALSE );               // disable the ones not handled by the generic control enable handler
        StopScriptBtn.EnableWindow( FALSE );

        dlgStarting = true;
        dlgRunning = false;
        pDlgThread.reset( new std::thread( &MotorSysTestDlg::ShowInfoDialog, this ) );
        pDlgThread->detach();
    }
}

void MotorSysTestDlg::ShowInfoDialog( void )
{
    if ( !dlgRunning )
    {
        dlgRunning = true;
        dlgStarting = false;

        int dlgResponse = IDCANCEL;
        CString msgStr;

        if ( !simMode )
        {
            bool probeUp = false;

            if ( plateSelected )
            {
                probeUp = pPlateController->IsProbeUp();
            }
            else if ( carouselSelected )
            {
                probeUp = pCarouselController->IsProbeUp();
            }

            if ( !probeUp )
            {
                OnBnClickedProbeUpBtn();
            }
        }

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
                        if ( !simMode )
                        {
                            pPlateController->MarkThetaPosAsZero();
                        }
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
                        if ( !simMode )
                        {
                            pCarouselController->MarkCarouselPosAsZero();
                        }
                    }
                    UpdateTubeNum();
                }
                UpdateRThetaPositions();
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
                        if ( !simMode )
                        {
                            pPlateController->MarkRadiusPosAsZero();
                        }
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
                        if ( !simMode )
                        {
                            pCarouselController->MarkCarouselRadiusPos();
                        }
                    }
                    UpdateTubeNum();
                }
                UpdateRThetaPositions();
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

                        if ( !simMode )
                        {
                            pPlateController->PlatePosition( thetaPos, radiusPos );
                            plateThetaCalPos = thetaPos;
                            pPlateController->CalibrateThetaPosition();
                        }
                    }
                    UpdateRThetaPositions();
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

                        if ( !simMode )
                        {
                            pPlateController->CalibratePlateCenter();
                            pPlateController->PlatePosition( thetaPos, radiusPos );
                            plateRadiusCenterPos = radiusPos;
                        }
                    }
                    UpdateRThetaPositions();
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
                                   _T( "necessary to assist or confirm the positioning.\r\n\r\n" ),
                                   _T( "When each step is complete, press 'Enter' or click/press the 'OK' button\r\n" ),
                                   _T( "to accept the actions and move to the next step,\r\n" ),
                                   _T( "or 'Cancel' to abort the calibration." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Set the Plate center position: Start" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pPlateController->HomeRadius();
                        }

                        msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "First, manually adjust the radius mechanism to position the probe\r\n" ),
                                       _T( "at the center calibration point of the plate calibration template.\r\n\r\n" ),
                                       _T( "Theta adjustments may be made as necessary, but Theta calibration\r\n" ),
                                       _T( "will occur later in the calibration process.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done and ready to\r\n" ),
                                       _T( "continue to the next step, or 'Cancel' to abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate center position: Step 1" ), MB_OKCANCEL );
                    }

#if(0)
                    if ( dlgResponse == IDOK )
                    {
                        msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "After the initial manual positioning, use the 'Radius Adjust' buttons\r\n" ),
                                       _T( "to precisely position the probe at the plate center calibration location\r\n" ),
                                       _T( "on the calibration fixture.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done to accept and\r\n" ),
                                       _T( "record the plate center location and continue on to the next step,\r\n" ),
                                       _T( "or 'Cancel' to abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate center position: Step 2" ), MB_OKCANCEL );
                    }
#endif

                    int32_t thetaPos = 0;
                    int32_t radiusPos = 0;

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pPlateController->PlatePosition( thetaPos, radiusPos );
                            plateRadiusCenterPos = radiusPos;
                            pPlateController->CalibratePlateCenter();       // capture and mark the current position as the plate center zero point
                            pPlateController->MarkRadiusPosAsZero();        // now set the center to zero
                            pPlateController->HomeTheta();
                        }

                        msgStr.Format( _T( "%s%s%s%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "Next, manually adjust the radius and theta mechanisms to position the probe\r\n" ),
                                       _T( "at the row alignment calibration point of the plate calibration template.\r\n\r\n" ),
                                       _T( "The radius may be adjusted to position the probe at or beyond the end of the\r\n" ),
                                       _T( "calibration template.  Adjusting the radius to place the probe slightly beyond\r\n" ),
                                       _T( "the end of the template can help improve the calibration angular accuracy.\r\n\r\n" ),
//                                       _T( "Press 'Enter' or click/press the 'OK' button to continue to the next step,\r\n" ),
//                                       _T( "or 'Cancel' to abort the theta calibration." ) );
                                       _T( "Press 'Enter' or click/press the 'OK' button when done to accept and\r\n" ),
                                       _T( "record the plate row alignment value and complete the calibration,\r\n" ),
                                       _T( "or 'Cancel' to abort the calibration." ) );

//                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate row alignment: Step 3" ), MB_OKCANCEL );
                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate row alignment: Step 2" ), MB_OKCANCEL );
                    }

#if(0)
                    if ( dlgResponse == IDOK )
                    {
                        msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "After the initial manual positioning, use the 'Theta Adjust' buttons\r\n" ),
                                       _T( "to precisely align the probe with the row alignment calibration point\r\n" ),
                                       _T( "of the calibration fixture.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done to accept and\r\n" ),
                                       _T( "record the plate row alignment value and complete the calibration,\r\n" ),
                                       _T( "or 'Cancel' to abort the theta calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Set the Plate row alignment: Step 4" ), MB_OKCANCEL );
                    }
#endif

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pPlateController->PlatePosition( thetaPos, radiusPos );
                            plateThetaCalPos = thetaPos;
                            pPlateController->CalibrateThetaPosition();
                            pPlateController->MarkThetaPosAsZero();     // capture and mark the current position as the plate center zero point
                            pPlateController->InitPlate();
                        }
                    }
                    UpdateRThetaPositions();
                }
                break;

            case MsgBoxTypeCalibrateCarousel:
                if ( carouselSelected )
                {
                    msgStr.Format( _T( "%s%s%s%s%s%s" ),
                                   //   1        10        20        30        40        50        60        70        80
                                   //   |        |         |         |         |         |         |         |         |
//                                    _T( "Calibration will be performed using multiple steps!\r\n\r\n" ),
                                   _T( "Calibration will be performed in the following step.\r\n\r\n" ),
//                                    _T( "For all steps, the probe may be manually adjusted up or down as\r\n" ),
                                   _T( "During calibration, the probe may be manually adjusted up or down as\r\n" ),
                                   _T( "necessary to assist or confirm the positioning.\r\n\r\n" ),
//                                    _T( "When each step is complete, press 'Enter' or click/press the 'OK' button\r\n" ),
//                                    _T( "to accept the actions and move to the next step,\r\n" ),
                                   _T( "When calibration adjustments are complete, press 'Enter' or click/press\r\n" ),
                                   _T( "the 'OK' button to accept and record the calibration values,\r\n" ),
                                   _T( "or 'Cancel' to abort the calibration." ) );

                    dlgResponse = MessageBox( msgStr, _T( "Calibrate the Carousel radius and theta positions: Start" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pCarouselController->HomeTheta();
                            pCarouselController->HomeRadius();
                        }

                        msgStr.Format( _T( "%s%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "Manually adjust the radius and theta mechanisms\r\n" ),
//                                        _T( "First, manually adjust the radius and theta mechanisms\r\n" ),
                                       _T( "to position the center of the tube one position under the probe.\r\n\r\n" ),
//                                        _T( "Press 'Enter' or click/press the 'OK' button to continue to the next step,\r\n" ),
//                                        _T( "or 'Cancel' to abort the calibration." ) );
                                       _T( "When calibration adjustments are complete, press 'Enter' or click/press\r\n" ),
                                       _T( "the 'OK' button to accept and record the calibration values,\r\n" ),
                                       _T( "or 'Cancel' to abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Calibrate the Carousel radius and theta positions: Step 1" ), MB_OKCANCEL );
                    }

#if(0)
                    if ( dlgResponse == IDOK )
                    {
                        msgStr.Format( _T( "%s%s%s%s" ),
                                       //   1        10        20        30        40        50        60        70        80
                                       //   |        |         |         |         |         |         |         |         |
                                       _T( "After the initial manual positioning, use the 'Theta Adjust' and\r\n" ),
                                       _T( "'Radius Adjust' buttons to precisely position the carousel.\r\n\r\n" ),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done to record\r\n" ),
                                       _T( "the calibration values, or 'Cancel' to abort the calibration." ) );

                        dlgResponse = MessageBox( msgStr, _T( "Calibrate the Carousel radius and theta positions: Step 2" ), MB_OKCANCEL );
                    }
#endif

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pCarouselController->CalibrateCarouselHome();
                            pCarouselController->InitCarousel();
                        }
                    }
                    UpdateTubeNum();
                    UpdateRThetaPositions();
                }
                break;

            case MsgBoxTypeNone:
            default:
                break;
        }

        dlgRunning = false;
        dlgStarting = false;
    }
    SetAllControlEnable( TRUE );               // re-enable ALL controls
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MotorSysTestDlg::OnBnClickedReagentArmUpBtn()
{
    if ( ( !simMode ) && ( !pReagentController->IsHome() ) )
    {
        bool moveOk = pReagentController->ArmUp();
    }
}


void MotorSysTestDlg::OnBnClickedReagentArmDnBtn()
{
    if ( ( !simMode ) && ( !pReagentController->IsDown() ) )
    {
        bool moveOk = pReagentController->ArmDown();
    }
}


void MotorSysTestDlg::OnBnClickedReagentArmPurgeBtn()
{
    if ( !simMode )
    {
        bool moveOk = pReagentController->ArmPurge();
    }
}

void MotorSysTestDlg::OnBnClickedUnlockDoorBtn()
{
    if ( !simMode )
    {
        pReagentController->ReleaseDoor();
    }
}

void MotorSysTestDlg::OnEnChangeThetaPosEdit()
{
    CString valStr;

    if ( !cntlEdit )
    {
        cntlEdit = true;

        ThetaPosEdit.GetWindowText( valStr );
        if ( valStr.GetLength() > 0 )
        {
            std::string posStr = CT2A( valStr.GetString() );
            bool showText = false;

            //  ignore initial leading minus entry for negative values...
            if ( ( posStr.length() > 1 ) || ( ( posStr.length() == 1 ) && ( posStr.at( 0 ) != '-' ) ) )
            {
                int32_t posVal = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &posVal );
                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), posVal );
                    MessageBox( valStr, _T( "Illegal Theta Position" ) );
                    showText = true;
                }
                else
                {
                    int32_t maxThetaPos = MaxThetaPosition;

                    if ( carouselSelected )
                    {
                        maxThetaPos = maxCarouselThetaPos;
                    }
                    else if ( plateSelected )
                    {
                        maxThetaPos = maxPlateThetaPos;
                    }

                    if ( ( posVal <= ( 2 * maxThetaPos ) ) && ( posVal > -( maxThetaPos ) ) )
                    {
                        thetaEditVal = posVal;
                    }
                    else
                    {
                        valStr.Format( _T( "Value out of range: %d\r\n(%d - %d)" ), posVal, -( maxThetaPos - 1 ), ( 2 * maxThetaPos ) );
                        MessageBox( valStr, _T( "Illegal Theta Position" ) );
                        showText = true;
                    }
                }

                if ( showText )
                {
                    if ( ThetaPosEdit.CanUndo() )
                    {
                        ThetaPosEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), thetaEditVal );
                        ThetaPosEdit.SetWindowText( valStr );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedGotoThetaPosBtn()
{
    DoGotoThetaPos( doRelativeMove );
}

void MotorSysTestDlg::OnBnClickedHomeThetaBtn()
{
    std::string controllerName;
    std::string methodName;
    bool homeOk = true;

    methodName = "HomeTheta";
    if ( carouselSelected )
    {
        controllerName = "CarouselController";

        if ( !simMode )
        {
            homeOk = pCarouselController->HomeTheta();
        }
    }
    else if ( plateSelected )
    {
        controllerName = "PlateController";

        if ( !simMode )
        {
            homeOk = pPlateController->HomeTheta();
        }
    }
    else
    {
        pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
        controllerName = "non-initialized";

        if ( !simMode )
        {
            if ( pCarouselTheta )
            {
                pCarouselTheta->ClearErrors();
                pCarouselTheta->Home();
                pCarouselTheta->DoMotorWait();
                ScriptSleep( DefaultPosUpdateStdInterval );
                homeOk = pCarouselTheta->IsHome();
            }
        }
    }

    if ( !homeOk )
    {
        CString valStr;
        std::string logStr = boost::str( boost::format( "OnBnHomeThetaBtn: %s returned an error from '%s'." ) % controllerName % methodName );

        errorCnt++;
        valStr.Format( _T( "%d" ), errorCnt );
        ErrorCntDisp.SetWindowText( valStr );
        Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    }

    if ( carouselSelected )
    {
        UpdateTubeNum();
    }

    UpdateRThetaPositions();
}

void MotorSysTestDlg::OnEnChangeRadiusPosEdit()
{
    CString valStr;

    if ( !cntlEdit )
    {
        cntlEdit = true;

        RadiusPosEdit.GetWindowText( valStr );
        if ( valStr.GetLength() > 0 )
        {
            std::string posStr = CT2A( valStr.GetString() );
            bool showText = false;

            //  ignore initial leading minus entry for negative values...
            if ( ( posStr.length() > 1 ) || ( ( posStr.length() == 1 ) && ( posStr.at( 0 ) != '-' ) ) )
            {
                int32_t posVal = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &posVal );
                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), posVal );
                    MessageBox( valStr, _T( "Illegal Radius Position" ) );
                    showText = true;
                }
                else
                {
                    int32_t minVal = -( RadiusMaxTravel );
                    int32_t maxVal = RadiusMaxTravel;

                    if ( carouselSelected )
                    {
                        maxVal = carouselRadiusMaxTravel;
                    }
                    else if ( plateSelected )
                    {
                        minVal = -( plateRadiusMaxTravel / 4 );
                        maxVal = plateRadiusMaxTravel;
                    }

                    if ( ( posVal < minVal ) || ( posVal > maxVal ) )
                    {
                        valStr.Format( _T( "Value out of range: %d\r\n(%d - %d)" ), posVal, minVal, maxVal );
                        MessageBox( valStr, _T( "Illegal Radius Position" ) );
                        showText = true;
                    }
                    else
                    {
                        radiusEditVal = posVal;
                    }
                }

                if ( showText )
                {
                    if ( RadiusPosEdit.CanUndo() )
                    {
                        RadiusPosEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), radiusEditVal );
                        RadiusPosEdit.SetWindowText( valStr );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedGotoRadiusPosBtn()
{
    DoGotoRadiusPos( doRelativeMove );
}

void MotorSysTestDlg::OnBnClickedHomeRadiusBtn()
{
    std::string controllerName;
    std::string methodName;
    bool homeOk = true;

    methodName = "HomeRadius";
    if ( carouselSelected )
    {
        controllerName = "CarouselController";

        if ( !simMode )
        {
            homeOk = pCarouselController->HomeRadius();
        }
    }
    else if ( plateSelected )
    {
        controllerName = "PlateController";

        if ( !simMode )
        {
            homeOk = pPlateController->HomeRadius();
        }
    }
    else
    {
        pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
        controllerName = "non-initialized";

        if ( !simMode )
        {
            if ( pCarouselRadius )
            {
                pCarouselRadius->ClearErrors();
                pCarouselRadius->Home();
                pCarouselRadius->DoMotorWait();
                ScriptSleep( DefaultPosUpdateStdInterval );
                homeOk = pCarouselRadius->IsHome();
            }
        }
    }

    if ( !homeOk )
    {
        CString valStr;
        std::string logStr = boost::str( boost::format( "OnBnClickedHomeRadiusBtn: %s returned an error from '%s'." ) % controllerName % methodName );

        errorCnt++;
        valStr.Format( _T( "%d" ), errorCnt );
        ErrorCntDisp.SetWindowText( valStr );
        Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    }

    if ( carouselSelected )
    {
        UpdateTubeNum();
    }

    UpdateRThetaPositions();
}

void MotorSysTestDlg::OnBnClickedGotoRThetaPosBtn()
{
    DoGotoRThetaPos( doRelativeMove );
}

void MotorSysTestDlg::OnBnClickedRelativeMoveChk()
{
    int32_t chkState = RelativeMoveChk.GetCheck();      // checkbox state is updated prior to getting here...

    if ( chkState == BST_UNCHECKED )
    {
        doRelativeMove = false;
    }
    else if ( chkState == BST_CHECKED )
    {
        doRelativeMove = true;
    }
}

void MotorSysTestDlg::OnBnClickedProbeUpBtn()
{
    if ( !simMode )
    {
        std::string controllerName;
        std::string methodName = "ProbeUp";
        bool moveOk = true;

        if ( carouselSelected )
        {
            controllerName = "CarouselController";
            moveOk = pCarouselController->ProbeUp();
        }
        else if ( plateSelected )
        {
            controllerName = "PlateController";
            moveOk = pPlateController->ProbeUp();
        }
        else
        {
            pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
            controllerName = "non-initialized";

            if ( !simMode )
            {
                if ( pCarouselProbe )
                {
                    pCarouselProbe->ClearErrors();
                    pCarouselProbe->ProbeUp();
                    pCarouselProbe->DoMotorWait();
                    ScriptSleep( DefaultPosUpdateStdInterval );
                    moveOk = pCarouselProbe->IsHome();
                }
            }
        }

        if ( !moveOk )
        {
            CString valStr;
            std::string logStr = boost::str( boost::format( "OnBnClickedProbeUpBtn: %s returned an error from '%s'." ) % controllerName % methodName );

            errorCnt++;
            valStr.Format( _T( "%d" ), errorCnt );
            ErrorCntDisp.SetWindowText( valStr );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
        }
        UpdateProbePosition();
    }
}

void MotorSysTestDlg::OnBnClickedProbeDownBtn()
{
    if ( !simMode )
    {
        std::string controllerName;
        std::string methodName = "ProbeDown";
        bool moveOk = true;

        if ( carouselSelected )
        {
            controllerName = "CarouselController";
            moveOk = pCarouselController->ProbeDown();
        }
        else if ( plateSelected )
        {
            controllerName = "PlateController";
            moveOk = pPlateController->ProbeDown();
        }
        else
        {
            pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
            controllerName = "non-initialized";

            if ( !simMode )
            {
                if ( pCarouselProbe )
                {
                    int32_t currentPos = 0;
                    int32_t stopPos = 0;

                    pCarouselProbe->ClearErrors();
                    pCarouselProbe->ProbeDown();
                    pCarouselProbe->DoMotorWait();
                    ScriptSleep( DefaultPosUpdateStdInterval );
                    pCarouselProbe->ClearErrors();
                    currentPos = pCarouselProbe->GetPosition();
                    stopPos = pCarouselProbe->GetProbeStop();
                    moveOk = pCarouselProbe->PosAtTgt( currentPos, ( stopPos - pCarouselProbe->GetProbeRaise() ) );
                }
            }
        }

        if ( !moveOk )
        {
            CString valStr;
            std::string logStr = boost::str( boost::format( "OnBnClickedProbeDownBtn: %s returned an error from '%s'." ) % controllerName % methodName );

            errorCnt++;
            valStr.Format( _T( "%d" ), errorCnt );
            ErrorCntDisp.SetWindowText( valStr );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
        }
        UpdateProbePosition();
    }
}

void MotorSysTestDlg::OnBnClickedProbeHomeBtn()
{
    if ( !simMode )
    {
        std::string controllerName;
        std::string methodName = "ProbeHome";
        bool moveOk = true;

        if ( carouselSelected )
        {
            controllerName = "CarouselController";
            moveOk = pCarouselController->ProbeHome();
        }
        else if ( plateSelected )
        {
            controllerName = "PlateController";
            moveOk = pPlateController->ProbeHome();
        }
        else
        {
            pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
            controllerName = "non-initialized";

            if ( !pCarouselProbe->IsHome() )
            {
                if ( !simMode )
                {
                    if ( pCarouselProbe )
                    {
                        pCarouselProbe->ClearErrors();
                        pCarouselProbe->Home();
                        pCarouselProbe->DoMotorWait();
                        ScriptSleep( DefaultPosUpdateStdInterval );
                        moveOk = pCarouselProbe->IsHome();
                    }
                }
            }
        }

        if ( !moveOk )
        {
            CString valStr;
            std::string logStr = boost::str( boost::format( "OnBnClickedProbeHomeBtn: %s returned an error from '%s'." ) % controllerName % methodName );

            errorCnt++;
            valStr.Format( _T( "%d" ), errorCnt );
            ErrorCntDisp.SetWindowText( valStr );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
        }
        UpdateProbePosition();
    }
}

void MotorSysTestDlg::OnBnClickedProbeInitBtn()
{
    if ( !simMode )
    {
        if ( carouselSelected )
        {
            pCarouselController->InitProbe();
        }
        else if ( plateSelected )
        {
            pPlateController->InitProbe();
        }
        UpdateProbePosition();
    }
}

void MotorSysTestDlg::OnBnClickedFineFocusUpBtn()
{
    ClearFocusPosition();
    pFocusController->FocusStepUpFine();
    UpdateFocusPosition();
}

void MotorSysTestDlg::OnBnClickedFineFocusDnBtn()
{
    ClearFocusPosition();
    pFocusController->FocusStepDnFine();
    UpdateFocusPosition();
}

void MotorSysTestDlg::OnBnClickedCoarseFocusUpBtn()
{
    ClearFocusPosition();
    pFocusController->FocusStepUpCoarse();
    UpdateFocusPosition();
}

void MotorSysTestDlg::OnBnClickedCoarseFocusDnBtn()
{
    ClearFocusPosition();
    pFocusController->FocusStepDnCoarse();
    UpdateFocusPosition();
}

void MotorSysTestDlg::OnEnChangeFocusPosEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;

        FocusPosEdit.GetWindowText( valStr );
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
                        focusEditVal = posVal;
                    }
                }

                if ( showText )
                {
                    cntlEdit = true;        // prevent re-entry caused by value update from other controls

                    if ( FocusPosEdit.CanUndo() )
                    {
                        FocusPosEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), focusEditVal );
                        FocusPosEdit.SetWindowText( valStr );
                    }

                    cntlEdit = false;
                }
            }
        }
    }
}

void MotorSysTestDlg::OnBnClickedFocusGotoPosBtn()
{
    int newPos = focusEditVal;

    if ( newPos > focusMaxTravel )
    {
        newPos = focusMaxTravel;                // limit position entries to the expected allowable range
    }
    else if ( newPos < 0 )
    {
        newPos = 0;
    }

    int32_t focusPos = pFocusController->Position();
    // set focus position...
    if ( newPos != focusPos )
    {
        ClearFocusPosition();
        pFocusController->SetPosition( newPos );
        UpdateFocusPosition();
    }
}

void MotorSysTestDlg::OnBnClickedFocusHomeBtn()
{
    ClearFocusPosition();
    pFocusController->FocusHome();
    UpdateFocusPosition();
}

void MotorSysTestDlg::OnBnClickedFocusGotoCenterBtn()
{
    int newPos = focusMaxTravel / 2;

    int32_t focusPos = pFocusController->Position();
    // set focus position...
    if ( newPos != focusPos )
    {
        ClearFocusPosition();
        pFocusController->SetPosition( newPos );
        UpdateFocusPosition();
    }
}

void MotorSysTestDlg::OnBnClickedInstTypeChk()
{
    int32_t chkState = InstrumentTypeChk.GetCheck();
    MotorSysTestDlg::InstrumentTypes oldType = instrumentType;

    if ( chkState == BST_INDETERMINATE )
    {
        instrumentType = MotorSysTestDlg::InstrumentTypeScout;
    }
    else if ( chkState == BST_UNCHECKED )
    {
        MotorSysTestDlg::InstrumentTypes newType = MotorSysTestDlg::InstrumentTypeHunter;
        if ( oldType != newType )
        {
            instrumentType = newType;
            if ( !InitLedController() )
            {
                cntlEdit = true;
                instrumentType = oldType;
                InstrumentTypeChk.SetCheck( BST_INDETERMINATE );
                cntlEdit = false;
            }
        }
    }
    else if ( chkState == BST_CHECKED )
    {
        instrumentType = MotorSysTestDlg::InstrumentTypeUnknown;
    }

    if ( ( oldType == MotorSysTestDlg::InstrumentTypeHunter ) && ( oldType != instrumentType ) )
    {
        pLedRackController->Stop();
        pLedRackController->Quit();
        pLedRackController.reset();
    }

    ShowInstrumentType( instrumentType );
}

void MotorSysTestDlg::OnBnClickedLedRack1GotoTopBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathTop );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack1CalTopBtn()
{
    pLedRackController->CalibrateRack1TopPos();
}

void MotorSysTestDlg::OnBnClickedLedRack1GotoCtrBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathMiddle );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack1CalCtrBtn()
{
    pLedRackController->CalibrateRack1MiddlePos();
}

void MotorSysTestDlg::OnBnClickedLedRack1GotoBotBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack1, LEDRackController::PathBottom );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack1CalBotBtn()
{
    pLedRackController->CalibrateRack1BottomPos();
}

void MotorSysTestDlg::OnBnClickedLedRack1HomeBtn()
{
    pLedRackController->HomeRack( LEDRackController::Rack1 );
}

void MotorSysTestDlg::OnBnClickedLedRack2GotoTopBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathTop );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack2CalTopBtn()
{
    pLedRackController->CalibrateRack2TopPos();
}

void MotorSysTestDlg::OnBnClickedLedRack2GotoCtrBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathMiddle );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack2CalCtrBtn()
{
    pLedRackController->CalibrateRack2MiddlePos();
}

void MotorSysTestDlg::OnBnClickedLedRack2GotoBotBtn()
{
    pLedRackController->SetPath( LEDRackController::Rack2, LEDRackController::PathBottom );
    cntlEdit = true;
    UpdateRackPositions();
    cntlEdit = false;
    pLedRackController->GetMotors( pRack1, pRack2 );
}

void MotorSysTestDlg::OnBnClickedLedRack2CalBotBtn()
{
    pLedRackController->CalibrateRack2BottomPos();
}

void MotorSysTestDlg::OnBnClickedLedRack2HomeBtn()
{
    pLedRackController->HomeRack( LEDRackController::Rack2 );
}

void MotorSysTestDlg::OnEnChangeRowEdit()
{
    if ( !cntlEdit )
    {
        cntlEdit = true;

        CString valStr;
        valStr = "A";

        RowValEdit.GetWindowText( valStr );
        valStr.Trim();
        if ( valStr.GetLength() > 0 )
        {
            _TCHAR baseChar = 'A';
            int32_t idx = 0;

            valStr.MakeUpper();
            _TCHAR rowChar = valStr.GetAt( 0 );

            idx = rowChar - baseChar;
            if ( ( idx >= 0 ) && ( idx < MaxPlateRowNum ) )
            {
                rowEditVal = idx + 1;
                rowEditStr = valStr;
            }
            else
            {
                valStr.Format( _T( "Not a legal value: %c" ), rowChar );
                MessageBox( valStr, _T( "Illegal Row Value" ) );
                if ( RowValEdit.CanUndo() )
                {
                    RowValEdit.Undo();
                }
                else
                {
                    RowValEdit.SetWindowText( rowEditStr );
                }
            }
        }
        else
        {
            rowEditVal = 0;
            rowEditStr.Empty();
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnEnChangeColEdit()
{
    if ( !cntlEdit )
    {
        cntlEdit = true;

        CString valStr;
        int32_t col = colEditVal;
        int32_t idx = 0;

        ColValEdit.GetWindowText( valStr );
        if ( valStr.GetLength() > 0 )
        {
            std::string colStr = CT2A( valStr.GetString() );
            int status = sscanf_s( colStr.c_str(), "%d", &col );

            if ( status == 1 )
            {
                if ( ( col < 1 ) || ( col > MaxPlateColNum ) )
                {
                    status = -1;
                }
            }

            if ( status != 1 )
            {
                valStr.Format( _T( "Not a legal value: %d" ), col );
                MessageBox( valStr, _T( "Illegal Columm Value" ) );
                if ( ColValEdit.CanUndo() )
                {
                    ColValEdit.Undo();
                }
                else
                {
                    valStr.Format( _T( "%d" ), colEditVal );
                    ColValEdit.SetWindowText( valStr );
                }
            }
            else
            {
                colEditVal = col;
            }
        }
        else
        {
            colEditVal = 0;
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedGotoRowColBtn()
{
    if ( plateSelected )
    {
        std::string logStr = boost::str( boost::format( "OnBnClickedGotoRowColBtn: rowEditVal: %d  colEditVal: %d    rowVal(old): %d  colVal(old): %d" ) % rowEditVal % colEditVal % rowVal % colVal );

        // set plate position by row / column values
        if ( ( rowEditVal != rowVal ) || ( colEditVal != colVal ) )
        {
            if ( ( rowEditVal > 0 ) && ( rowEditVal <= MaxPlateRowNum ) &&
                 ( colEditVal > 0 ) && ( colEditVal <= MaxPlateColNum ) )
            {
                CString valStr;
                bool positionOk = true;
                std::string methodStr;

#ifdef USER_API
                methodStr = "SetRowCol";
                if ( !simMode )
                {
                    positionOk = pPlateController->SetRowCol( rowEditVal, colEditVal );
                }
#else
                int32_t rTgtPos = 0;
                int32_t tTgtPos = 0;
                bool calcOk = false;

                calcOk = pPlateController->CalculateRowColPos( rowEditVal - 1, colEditVal - 1, tTgtPos, rTgtPos );
                if ( calcOk )
                {
                    logStr.append( boost::str( boost::format( "  tTgtPos: %d  rTgtPos: %d" ) % tTgtPos % rTgtPos ) );
                    if ( !simMode )
                    {
                        methodStr = "GoTo";
                        positionOk = pPlateController->GoTo( tTgtPos, rTgtPos );
                        if ( !positionOk )
                        {
                            logStr.append( " PlateController returned an error from 'GoTo'." );
                        }
                    }
                }
                else
                {
                    positionOk = false;
                    methodStr = "CalculateRowColPos";
                }
#endif // USER_API
                if ( !positionOk )
                {
                    if ( !positionOk )
                    {
                        logStr.append( boost::str( boost::format( " PlateController returned an error from '%s'." ) % methodStr ) );
                    }

                    errorCnt++;
                    valStr.Format( _T( "%d" ), errorCnt );
                    ErrorCntDisp.SetWindowText( valStr );
                }

                UpdateRowColPositions();
                UpdateRThetaPositions();
            }
        }
        Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
    }
}

void MotorSysTestDlg::OnBnClickedNextRowColBtn()
{
    if ( plateSelected )
    {
        uint32_t tgtRow = rowVal;
        uint32_t tgtCol = colVal;

        if ( tgtCol < MaxPlateColNum )
        {
            tgtCol++;
        }
        else
        {
            if ( tgtRow < MaxPlateRowNum )
            {
                tgtRow++;
                tgtCol = 1;
            }
            else
            {
                tgtRow = MaxPlateRowNum + 1;
                tgtCol = MaxPlateColNum + 1;
            }
        }

        if ( ( tgtRow > 0 ) && ( tgtRow <= MaxPlateRowNum ) &&
             ( tgtCol > 0 ) && ( tgtCol <= MaxPlateColNum ) )
        {
            rowEditVal = tgtRow;
            colEditVal = tgtCol;
            OnBnClickedGotoRowColBtn();
        }
    }
}

void MotorSysTestDlg::OnEnChangeTubeNumEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;
        cntlEdit = true;

        TubeNumEdit.GetWindowText( valStr );
        if ( !valStr.IsEmpty() )
        {
            std::string posStr = CT2A( valStr.GetString() );
            int32_t tube = 0;
            int status = sscanf_s( posStr.c_str(), "%d", &tube );

            if ( status == 1 )
            {
                if ( ( tube < 0 ) || ( tube > maxCarouselTubes ) )
                {
                    status = -1;
                }
                else
                {
                    tubeEditVal = tube;
                }
            }

            if ( status != 1 )
            {
                valStr.Format( _T( "Not a legal value: %d\rMust be between 1 and 24." ), tube );
                MessageBox( valStr, _T( "Illegal Tube Number" ) );
                if ( TubeNumEdit.CanUndo() )
                {
                    TubeNumEdit.Undo();
                }
                else
                {
                    valStr.Format( _T( "%d" ), tubeEditVal );
                    TubeNumEdit.SetWindowText( valStr );
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedGotoTubeBtn()
{
    DoGotoTubeBtn( true );
}

void MotorSysTestDlg::OnBnClickedNextTubeBtn()
{
    if ( carouselSelected )
    {
        CString valStr;

        if ( !simMode )
        {
            pCarouselController->GotoNextTube();
        }
        UpdateTubeNum();
    }
}

void MotorSysTestDlg::OnBnClickedFindTubeBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            bool found = pCarouselController->FindNextTube();

            if ( !found )
            {
                CString valStr = _T( "No tubes found!" );
                MessageBox( valStr, _T( "Tube search" ) );
            }
        }
        UpdateTubeNum();
    }
}

void MotorSysTestDlg::OnBnClickedUpdateCurrentBtn()
{
    UpdateRThetaPositions();
    UpdateProbePosition();
    if ( carouselSelected )
    {
        UpdateTubeNum();
    }
    else if ( plateSelected )
    {
        UpdateRowColPositions();
    }
}

void MotorSysTestDlg::OnBnClickedRadioPlateSelect()
{
    DoPlateSelect( true );
}

void MotorSysTestDlg::OnBnClickedPlateCalBtn()
{
    if ( plateSelected )
    {
        msgType = MsgBoxTypeCalibratePlate;
        StartInfoDialog();
    }
}

void MotorSysTestDlg::OnBnClickedPlateEjectBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->EjectPlate();
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedPlateLoadBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->LoadPlate();
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedPlateInitBtn()
{
    if ( plateSelected )
    {
        DoPlateInit( true );
    }
}

void MotorSysTestDlg::OnBnClickedPlateDetectBtn()
{
    if ( plateSelected )
    {
        DoPlateDetect( true );
        ShowPlateDetected( plateFound );
    }
}

void MotorSysTestDlg::OnBnClickedPlateThetaAdjustLeftBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->StepThetaLeft();          // these return the 'new' position, so it's not obvious if there was an error
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedPlateThetaAdjustRightBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->StepThetaRight();         // these return the 'new' position, so it's not obvious if there was an error
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedPlateRadiusAdjustInBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->StepRadiusIn();           // these return the 'new' position, so it's not obvious if there was an error
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedPlateRadiusAdjustOutBtn()
{
    if ( plateSelected )
    {
        if ( !simMode )
        {
            pPlateController->StepRadiusOut();          // these return the 'new' position, so it's not obvious if there was an error
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnEnChangePlateRadiusBacklashEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;
        cntlEdit = true;

        PlateRadiusBacklashEdit.GetWindowText( valStr );
        if ( !valStr.IsEmpty() )
        {
            std::string posStr = CT2A( valStr.GetString() );

            //  ignore initial leading minus entry for negative values...
            if ( ( posStr.length() > 1 ) || ( ( posStr.length() == 1 ) && ( posStr.at( 0 ) != '-' ) ) )
            {
                int32_t backlash = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &backlash );

// allow negative values...
//                if ( status == 1 )
//                {
//                    if ( backlash < 0 )
//                    {
//                        status = -1;
//                    }
//                }

                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), backlash );
                    MessageBox( valStr, _T( "Illegal Backlash Value" ) );
                    if ( PlateRadiusBacklashEdit.CanUndo() )
                    {
                        PlateRadiusBacklashEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), plateRadiusBacklash );
                        PlateRadiusBacklashEdit.SetWindowText( valStr );
                    }
                }
                else
                {
                    if ( backlash != plateRadiusBacklash )
                    {
                        plateRadiusBacklash = backlash;
                        pPlateController->UpdateRadiusBacklash( plateRadiusBacklash );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedPlateRadiusBacklashSetBtn()
{
    pPlateController->SetRadiusBacklash( plateRadiusBacklash );
}

void MotorSysTestDlg::OnEnChangePlateThetaBacklashEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;
        cntlEdit = true;

        PlateThetaBacklashEdit.GetWindowText( valStr );
        if ( !valStr.IsEmpty() )
        {
            std::string posStr = CT2A( valStr.GetString() );

            //  ignore initial leading minus entry for negative values...
            if ( ( posStr.length() > 1 ) || ( ( posStr.length() == 1 ) && ( posStr.at( 0 ) != '-' ) ) )
            {
                int32_t backlash = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &backlash );
// allow negative values...
//                if ( status == 1 )
//                {
//                    if ( backlash < 0 )
//                    {
//                        status = -1;
//                    }
//                }

                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), backlash );
                    MessageBox( valStr, _T( "Illegal Backlash Value" ) );
                    if ( PlateThetaBacklashEdit.CanUndo() )
                    {
                        PlateThetaBacklashEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), plateThetaBacklash );
                        PlateThetaBacklashEdit.SetWindowText( valStr );
                    }
                }
                else
                {
                    if ( backlash != plateThetaBacklash )
                    {
                        plateThetaBacklash = backlash;
                        pPlateController->UpdateThetaBacklash( plateThetaBacklash );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedPlateThetaBacklashSetBtn()
{
    if ( plateSelected )
    {
        pPlateController->SetThetaBacklash( plateThetaBacklash );
    }
}

void MotorSysTestDlg::OnBnClickedRadioCarouselSelect()
{
    DoCarouselSelect( true );
}

void MotorSysTestDlg::OnBnClickedCarouselCalBtn()
{
    if ( carouselSelected )
    {
        msgType = MsgBoxTypeCalibrateCarousel;
        StartInfoDialog();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselEjectBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->EjectCarousel();
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselLoadBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->LoadCarousel();
        }
        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselInitBtn()
{
    if ( carouselSelected )
    {
        DoCarouselInit( true );
    }
}

void MotorSysTestDlg::OnBnClickedCarouselThetaAdjustLeftBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->StepThetaLeft();       // these return the 'new' position, so it's not obvious if there was an error
        }

        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselThetaAdjustRightBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->StepThetaRight();      // these return the 'new' position, so it's not obvious if there was an error
        }

        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselRadiusAdjustInBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->StepRadiusIn();        // these return the 'new' position, so it's not obvious if there was an error
        }

        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnBnClickedCarouselRadiusAdjustOutBtn()
{
    if ( carouselSelected )
    {
        if ( !simMode )
        {
            pCarouselController->StepRadiusOut();       // these return the 'new' position, so it's not obvious if there was an error
        }

        UpdateRThetaPositions();
    }
}

void MotorSysTestDlg::OnEnChangeCarouselThetaBacklashEdit()
{
    if ( !cntlEdit )
    {
        CString valStr;
        cntlEdit = true;

        CarouselThetaBacklashEdit.GetWindowText( valStr );
        if ( !valStr.IsEmpty() )
        {
            std::string posStr = CT2A( valStr.GetString() );

            //  ignore initial leading minus entry for negative values...
            if ( ( posStr.length() > 1 ) || ( ( posStr.length() == 1 ) && ( posStr.at( 0 ) != '-' ) ) )
            {
                int32_t backlash = 0;
                int status = sscanf_s( posStr.c_str(), "%d", &backlash );
// allow negative values
//                if ( status == 1 )
//                {
//                    if ( backlash < 0 )
//                    {
//                        status = -1;
//                    }
//                }

                if ( status != 1 )
                {
                    valStr.Format( _T( "Not a legal value: %d" ), backlash );
                    MessageBox( valStr, _T( "Illegal Backlash Value" ) );
                    if ( CarouselThetaBacklashEdit.CanUndo() )
                    {
                        CarouselThetaBacklashEdit.Undo();
                    }
                    else
                    {
                        valStr.Format( _T( "%d" ), carouselThetaBacklash );
                        CarouselThetaBacklashEdit.SetWindowText( valStr );
                    }
                }
                else
                {
                    if ( backlash != carouselThetaBacklash )
                    {
                        carouselThetaBacklash = backlash;
                        pCarouselController->UpdateThetaBacklash( carouselThetaBacklash );
                    }
                }
            }
        }

        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedCarouselThetaBacklashSetBtn()
{
    if ( carouselSelected )
    {
        pCarouselController->SetThetaBacklash( carouselThetaBacklash );
    }
}

void MotorSysTestDlg::OnBnClickedRunScriptBtn()
{
    if ( ( !scriptStarting ) && ( !scriptRunning ) )
    {
        StartScript();
    }
}

void MotorSysTestDlg::OnEnVscrollScriptEdit()
{
    int32_t topIdx = ScriptEdit.GetFirstVisibleLine();

    if ( topIdx != scrollIdx )
    {
        scrollIdx = topIdx;
    }
    else
    {
        scrollIdx = -1;     // indicate the window can't be scrolled any further
    }
}

void MotorSysTestDlg::OnEnChangeStepNumDisp()
{
    CString valStr;

    if ( !cntlEdit )
    {
        cntlEdit = true;

        valStr.Empty();
        StepNumDisp.GetWindowText( valStr );

        if ( !valStr.IsEmpty() )
        {
            std::string idxStr = CT2A( valStr.GetString() );
            int32_t lineVal = -1;

            int status = sscanf_s( idxStr.c_str(), "%d", &lineVal );
            if ( status == 1 )
            {
                scriptLines = ScriptEdit.GetLineCount();

                if ( ( lineVal < 0 ) || ( lineVal > scriptLines ) )
                {
                    status = -1;
                }
                else
                {
                    int32_t lineLen = 0;
                    int32_t chIdx = 0;
                    int32_t lineIdx = lineVal - 1;

                    if ( ( scriptLines == 1 ) || ( lineIdx < 0 ) )
                    {
                        lineIdx = 0;
                    }

                    chIdx = ScriptEdit.LineIndex( lineIdx );
                    lineLen = ScriptEdit.LineLength( chIdx );
                    if ( ( lineLen == 0 ) && ( lineVal == 0 ) )
                    {
                        scrollIdx = 0;
                    }
                    else
                    {
                        ScriptEdit.Clear();
                        ScriptEdit.SetFont( &scriptEditFont );
                        ScriptEdit.SetSel( chIdx, ( chIdx + lineLen ) );
                        ScriptEdit.LineScroll( 0, -( lineLen ) );
                        scrollIdx = ScriptEdit.GetFirstVisibleLine();
                    }

                    cmdStepLine = lineVal;
                }
            }

            if ( status != 1 )
            {
                valStr.Format( _T( "Not a legal value: %d\r\n\r\nLine number must be less than or equal to script lines; (0 = none)." ), lineVal );
                MessageBox( valStr, _T( "Illegal Line number" ) );
            }

            ShowStepNum( cmdStepLine );
        }
        else
        {
            cmdStepLine = 0;
        }
        cntlEdit = false;
    }
}

void MotorSysTestDlg::OnBnClickedPauseScriptBtn()
{
    CString labelStr;
    // notify the script runner to pause or continue
    if ( !scriptPaused )
    {
        scriptPaused = true;
        labelStr = _T( "Continue" );
    }
    else
    {
        scriptPaused = false;
        labelStr = _T( "Pause" );
    }
    PauseScriptBtn.SetWindowText( labelStr );
}

void MotorSysTestDlg::OnBnClickedStopScriptBtn()
{
    stopRun = true;     // notify the script runner theread of the request to stop
}

void MotorSysTestDlg::OnEnChangeScriptPathEdit()
{
    ScriptPathEdit.GetWindowText( ScriptPath );    // initialize with the contents of the path edit control...
}

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// szFilters is a text string that includes three file name filters:
// "*.txt" for "Text Files",  "*.scr" for "Script Files", and "*.*' for "All Files."
static TCHAR BASED_CODE szFilter[] = _T( "Text Files (*.txt)|*.txt|" )
_T( "Script Files (*.scr)|*.scr|" )
_T( "All Files (*.*)|*.*||" );

void MotorSysTestDlg::OnBnClickedLoadScriptBtn()
{
//  CFileDialog cconstruction parameters 
//
//  For File Open, specify true for the bOpenFileDialog parameter...
//
//    explicit CFileDialog( BOOL bOpenFileDialog,
//                          LPCTSTR lpszDefExt = NULL,
//                          LPCTSTR lpszFileName = NULL,
//                          DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//                          LPCTSTR lpszFilter = NULL,
//                          CWnd* pParentWnd = NULL,
//                          DWORD dwSize = 0,
//                          BOOL bVistaStyle = TRUE );

    // Create an Open dialog; the default file name extension is "txt" and the filter for that extension is "*.txt"
    CFileDialog fileOpenDlg( TRUE, _T( "txt" ), _T( "*.txt" ),
                             OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
                             szFilter );

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if ( fileOpenDlg.DoModal() == IDOK )
    {
        ScriptPath = fileOpenDlg.GetPathName();
        ScriptPathEdit.SetWindowText( ScriptPath.GetString() );

        if ( !ScriptPath.IsEmpty() )
        {
            CString valStr;

            ScriptEdit.Clear();
            valStr.Empty();
            ScriptEdit.SetWindowText( valStr );
            ShowStepNum( cmdStepLine );

            std::string fileStr = CT2A( ScriptPath.GetString() );
            if ( !OpenListFile( fileStr ) )
            {
                Logger::L().Log ( MODULENAME, severity_level::error, "Unable to open Plate Move RunFile: " + fileStr );
                return;
            }

            std::string cmdLine;
            std::string cmdBlock;

            int32_t lineCnt = ReadListFile( cmdBlock );

            if ( lineCnt )
            {
                ScriptEdit.SetFont( &scriptEditFont );
                ScriptEditBuf = CString( cmdBlock.c_str() );
                ScriptEdit.SetWindowText( ScriptEditBuf );

                // after loading, highlight the first line
                int32_t lineLen = 0;
                int32_t chIdx = 0;

                cntlEdit = true;
                scriptLines = ScriptEdit.GetLineCount();
                chIdx = ScriptEdit.LineIndex( 0 );
                lineLen = ScriptEdit.LineLength( chIdx );
                if ( ( lineLen == 0 ) && ( scriptLines <= 1 ) )
                {
                    cmdStepLine = 0;
                    scriptLines = 0;
                }
                else
                {
                    ScriptEdit.SetSel( chIdx, ( chIdx + lineLen ) );
                    ScriptEdit.LineScroll( 0, -( lineLen ) );
                    cmdStepLine = 1;
                }
                scrollIdx = 0;

                ShowStepNum( cmdStepLine );
                cntlEdit = false;
            }
        }
    }
}

void MotorSysTestDlg::OnBnClickedSaveScriptBtn()
{
//  CFileDialog cconstruction parameters
//
//  For File Save, specify false for the bOpenFileDialog parameter...
//
//    explicit CFileDialog( BOOL bOpenFileDialog,
//                          LPCTSTR lpszDefExt = NULL,
//                          LPCTSTR lpszFileName = NULL,
//                          DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//                          LPCTSTR lpszFilter = NULL,
//                          CWnd* pParentWnd = NULL,
//                          DWORD dwSize = 0,
//                          BOOL bVistaStyle = TRUE );


    // Create a Save dialog displaying all files in the target folder
    CFileDialog fileSaveDlg( FALSE, _T( "*" ), ScriptPath.GetString(),
                             OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter );

    // Display the file dialog. When user clicks OK, fileDlg.DoModal() 
    // returns IDOK.
    if ( fileSaveDlg.DoModal() == IDOK )
    {
        CString SaveFile = fileSaveDlg.GetPathName();

        if ( !SaveFile.IsEmpty() )
        {
            ScriptPath = SaveFile;
            ScriptPathEdit.SetWindowText( ScriptPath.GetString() );

            std::string fileStr = CT2A( ScriptPath.GetString() );
            if ( !OpenSaveFile( fileStr ) )
            {
                Logger::L().Log ( MODULENAME, severity_level::error, "Unable to save Plate Move RunFile: " + fileStr );
                return;
            }

            WriteListFile();
        }
    }
}

void MotorSysTestDlg::OnBnClickedClearScriptBtn()
{
    CString valStr;

    cmdStepLine = 0;
    scrollIdx = 0;
    scriptLines = 0;

    cntlEdit = true;
    ScriptEdit.Clear();
    valStr.Empty();
    ScriptEdit.SetWindowText( valStr );
    ShowStepNum( cmdStepLine );
    cntlEdit = false;
}

void MotorSysTestDlg::OnBnClickedExit()
{
    quitting = true;

    CDialogEx::OnOK();
}

