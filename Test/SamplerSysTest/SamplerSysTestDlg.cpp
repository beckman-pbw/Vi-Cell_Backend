
// SamplerSysTestDlg.cpp : implementation file
//

#include "stdafx.h"

#include <ctime>
#include <chrono>
#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "SamplerSysTest.h"
#include "SamplerSysTestDlg.h"

#include "Logger.hpp"

#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define USER_API


static const char MODULENAME[] = "SamplerSysTestDlg";
static const char CAROUSELCONTROLLERNODENAME[] = "carousel_controller";
static const char PLATECONTROLLERNODENAME[] = "plate_controller";


const std::string SamplerSysTestDlg::CmdSimModeStr          = "Sim";    // toggle script execution simulation mode (no hardware actions)
const std::string SamplerSysTestDlg::CmdInitPlateStr        = "Ip";     // Initialize Plate: home both theta and radius plate motors
const std::string SamplerSysTestDlg::CmdInitCarouselStr     = "Ic";     // Initialize Carousel: home the carousel; may not be the tube 1 position
const std::string SamplerSysTestDlg::CmdSelectPlateStr      = "Sp";     // Select Plate: initialize and apply plate motor control parameters for all following commands
const std::string SamplerSysTestDlg::CmdSelectCarouselStr   = "Sc";     // Select Carousel: initialize and apply carousel motor control parameters for all following commands
const std::string SamplerSysTestDlg::CmdHomeThetaStr        = "Ht";     // Home the Theta motor
const std::string SamplerSysTestDlg::CmdZeroThetaStr        = "Zt";     // Set the current position as the Theta zero point
const std::string SamplerSysTestDlg::CmdHomeRadiusStr       = "Hr";     // Home the Radius motor
const std::string SamplerSysTestDlg::CmdZeroRadiusStr       = "Zr";     // Set the current position as the Radius zero point
const std::string SamplerSysTestDlg::CmdThetaToCalStr       = "Tc";     // move the Theta motor to the cal position
const std::string SamplerSysTestDlg::CmdRadiusToCalStr      = "Rc";     // move the Radius motor to the center calibration position
const std::string SamplerSysTestDlg::CmdMoveThetaStr        = "Mt";     // move the Theta motor to the specified absolute location
const std::string SamplerSysTestDlg::CmdMoveThetaRelStr     = "Rt";     // move the Theta motor to the specified relative location
const std::string SamplerSysTestDlg::CmdMoveRadiusStr       = "Mr";     // move the Radius motor to the specified absolute location
const std::string SamplerSysTestDlg::CmdMoveRadiusRelStr    = "Rr";     // move the Radius motor to the specified relative location
const std::string SamplerSysTestDlg::CmdMoveRThetaStr       = "Mx";     // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
const std::string SamplerSysTestDlg::CmdMoveRThetaRelStr    = "Rx";     // move the Theta and Radius motors to the specified theta (p1) / radius (p2) coordinate pair
const std::string SamplerSysTestDlg::CmdCalPlateThetaStr    = "Ct";     // set the plate Theta motor cal position
const std::string SamplerSysTestDlg::CmdCalPlateRadiusStr   = "Cr";     // set the plate Radius motor center calibration position
const std::string SamplerSysTestDlg::CmdMoveCarouselStr     = "Xt";     // move carousel to tube number: '0' for next tube, 1-24 for discreet tube numbers
const std::string SamplerSysTestDlg::CmdMovePlateWellStr    = "Xw";     // move the plate to the specified row (p1) /col (p2) location
const std::string SamplerSysTestDlg::CmdMovePlateStr        = "Xx";     // Move plate to absolute theta (p1), radius (p2) position
const std::string SamplerSysTestDlg::CmdMovePlateRelStr     = "Xr";     // Move to relative theta (p1), radius (p2) position
const std::string SamplerSysTestDlg::CmdProbeHomeStr        = "Ph";     // home the probe
const std::string SamplerSysTestDlg::CmdProbeMoveStr        = "P";      // move the probe: '0' for 'Up', and '1' for 'down'
const std::string SamplerSysTestDlg::CmdWaitForKeyPressStr  = "K";      // Wait for a keypress.
const std::string SamplerSysTestDlg::CmdSleepStr            = "D";      // insert a delay into the command stream processing
const std::string SamplerSysTestDlg::CmdExitStr             = "E";      // Stop processing the run list and exit the program.



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


// SamplerSysTestDlg dialog



SamplerSysTestDlg::SamplerSysTestDlg( CWnd* pParent /*=NULL*/ )
    : CDialogEx( IDD_SAMPLER_SYS_TEST_DIALOG, pParent )
    , inited_( false )
    , cbiInit( false )
    , quitting( false )
    , timerComplete (false )
    , allowEditing( false )
    , simMode( false )
    , infoInit( false )
    , doRelativeMove( false )
    , showInitErrors( false )
    , signalUpdateReady( false )
    , signalUpdateStarting( false )
    , signalUpdateRunning( false )
    , moduleConfigFile( "SamplerSysTest.info" )
    , motorConfigFile( "MotorControl.info" )
    , ScriptEditBuf( _T( "" ) )
    , ScriptPath( _T( "" ) )
    , cntlEdit( true )
    , thetaHome( FALSE )
    , radiusHome( FALSE )
    , probeHome( FALSE )
    , tubePresent( FALSE )
    , thetaEditVal( 0 )
    , thetaVal( 0 )
    , thetaRawVal( 0 )
    , radiusEditVal( 0 )
    , radiusVal( 0 )
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
    , plateControlsInited(false)
    , plateBacklashChanged( false )
    , maxPlateThetaPos( MaxThetaPosition )
    , platePositionTolerance( DefaultPositionTolerance )
    , plateThetaCalPos( (int32_t) DefaultPlateThetaCalPos )             // value for the theta calibration position in 1/10 degree user units * the gearing factor
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

    , msgType(MsgBoxTypeNone)
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
    Logger::L().Log ( MODULENAME, severity_level::normal, MODULENAME );
}

SamplerSysTestDlg::~SamplerSysTestDlg()
{
    Quit();
}

void SamplerSysTestDlg::SetEditingMode( bool allow )
{
    allowEditing = allow;
}

void SamplerSysTestDlg::SetSimMode( bool isSim )
{
    simMode = isSim;
}

//*****************************************************************************
void SamplerSysTestDlg::signalHandler( const boost::system::error_code& ec, int signal_number )
{
    if ( ec )
    {
        Logger::L().Log ( MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"" );
    }

    Logger::L().Log ( MODULENAME, severity_level::critical, boost::str( boost::format( "Received signal no. %d" ) % signal_number ) );

    // All done listening.
    pSystemSignals_->cancel();

    // Try to get out of here.
    pLocalIosvc_->post( std::bind( &SamplerSysTestDlg::Quit, this ) );
}

//*****************************************************************************
void SamplerSysTestDlg::Quit()
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

    if ( pPlateController )
    {
        pPlateController->Quit();
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

    if ( plateControllerCfgNode )
    {
        plateControllerCfgNode.reset();
    }

    if ( carouselControllerCfgNode )
    {
        carouselControllerCfgNode.reset();
    }

    if ( ptfilecfg )
    {
        ptfilecfg.reset();
    }

    if ( ptconfig )
    {
        ptconfig.reset();
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

    if ( pPlateController )
    {
        pPlateController.reset();           // reset calls the normal destructor, which calls the controller 'Quit' method...
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

bool SamplerSysTestDlg::InitDlgInfoFile( void )
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

void SamplerSysTestDlg::ConfigDlgVariables( void )
{
    boost::property_tree::ptree & config = dlgCfgNode.get();

    int32_t editFlag = ( allowEditing == true ) ? 1 : 0;
    editFlag = config.get<int32_t>( "EditMode", editFlag );
    allowEditing = ( editFlag != 0 ) ? true : false;
    int32_t initErrorsFlag = ( showInitErrors == true ) ? 1 : 0;
    initErrorsFlag = config.get<int32_t>( "ShowInitErrors", initErrorsFlag );
    showInitErrors = ( initErrorsFlag != 0 ) ? true : false;
}

bool SamplerSysTestDlg::InitControllers()
{
    Logger::L().Log ( MODULENAME, severity_level::debug2, "init: <enter>" );

    if ( inited_ )
    {
        return true;
    }

    if ( !infoInit )
    {
        InitMotorInfoFile();

        t_opPTree   controllersNode;
        t_opPTree   thisNode;

        plateControllerCfgNode.reset();
        controllersNode.reset();
        thisNode.reset();

        controllersNode = ptconfig->get_child_optional( "motor_controllers" );                  // look for the controllers section for individualized parameters...
        if ( controllersNode )
        {
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

    int32_t initCnt = 0;
    int32_t numInits = 2;

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

bool SamplerSysTestDlg::InitMotorInfoFile( void )
{
    bool success = true;
    boost::system::error_code ec;

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
    return success;
}

void SamplerSysTestDlg::ConfigCarouselVariables( void )
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

void SamplerSysTestDlg::ConfigCarouselProbeVariables( void )
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

void SamplerSysTestDlg::ConfigPlateVariables( void )
{
    boost::property_tree::ptree & config = plateControllerCfgNode.get();

    platePositionTolerance = config.get<int32_t>( "PlatePositionTolerance", platePositionTolerance );
    plateThetaCalPos = config.get<int32_t>( "PlateThetaCalPos", plateThetaCalPos );
    plateThetaHomePosOffset = config.get<int32_t>( "PlateThetaHomePosOffset", plateThetaHomePosOffset );
// these do not need to be read from the configuration file...
//    plateThetaA1Pos = config.get<int32_t>( "PlateThetaA1Pos", plateThetaA1Pos );
//    plateThetaA1PosOffset = config.get<int32_t>( "PlateThetaA1PosOffset", plateThetaA1PosOffset );
    plateRadiusCenterPos = config.get<int32_t>( "PlateRadiusCenterPos", plateRadiusCenterPos );
// these do not need to be read from the configuration file...
//    plateRadiusHomePosOffset = config.get<int32_t>( "PlateRadiusHomePosOffset", plateRadiusHomePosOffset );
    plateRadiusBacklash = config.get<int32_t>( "PlateRadiusBacklash", plateRadiusBacklash );
    plateThetaBacklash = config.get<int32_t>( "PlateThetaBacklash", plateThetaBacklash );
//    plateRadiusA1Pos = config.get<int32_t>( "PlateRadiusA1Pos", plateRadiusA1Pos );
//    plateRadiusA1PosOffset = config.get<int32_t>( "PlateRadiusA1PosOffset", plateRadiusA1PosOffset );
    plateRadiusMaxTravel = config.get<int32_t>( "PlateRadiusMaxTravel", plateRadiusMaxTravel );
    plateThetaStartTimeout = config.get<int32_t>( "ThetaStartTimeout", plateThetaStartTimeout );
    plateThetaFullTimeout = config.get<int32_t>( "ThetaFullTimeout", plateThetaFullTimeout );
    plateRadiusStartTimeout = config.get<int32_t>( "RadiusStartTimeout", plateRadiusStartTimeout );
    plateRadiusFullTimeout = config.get<int32_t>( "RadiusFullTimeout", plateRadiusFullTimeout );
}

void SamplerSysTestDlg::ConfigPlateProbeVariables( void )
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

bool SamplerSysTestDlg::OpenListFile( std::string filename )
{
    if ( listFile.is_open() )
    {
        listFile.close();
    }

    listFile.open( filename );
    boost::filesystem::path p( filename );

    Logger::L().Log ( MODULENAME, severity_level::normal, "opening move run list input file: " + p.filename().generic_string() );
    if ( listFile.is_open() )
    {
        return true;
    }
    else
    {
        Logger::L().Log ( MODULENAME, severity_level::normal, "failed to open move run list input file: " + filename );
        return false;
    }
}

bool SamplerSysTestDlg::OpenSaveFile( std::string filename )
{
    if ( saveFile.is_open() )
    {
        saveFile.close();
    }

    std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
    saveFile.open( filename, mode );
    boost::filesystem::path p( filename );

    Logger::L().Log ( MODULENAME, severity_level::normal, "opening move run list output file: " + p.filename().generic_string() );
    if ( saveFile.is_open() )
    {
        return true;
    }
    else
    {
        Logger::L().Log ( MODULENAME, severity_level::normal, "failed to open move run list output file: " + filename );
        return false;
    }
}

int32_t SamplerSysTestDlg::ReadListFile( std::string& cmdBlock )
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

int32_t SamplerSysTestDlg::WriteListFile( void )
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
            ScriptEdit.GetLine( lineIdx, lineStr.GetBufferSetLength(lineLen), lineLen);
            scriptLine = CT2A( lineStr.Left(lineLen) );
            if ( ( lineIdx  + 1 ) < lineCnt )
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

void SamplerSysTestDlg::StartScript( void )
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
        pScriptThread.reset( new std::thread( &SamplerSysTestDlg::RunScript, this ) );
        pScriptThread->detach();
    }
}

void SamplerSysTestDlg::RunScript( void )
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

        if (!simMode)
        {
            if (carouselSelected)
            {
                pCarouselController->InitCarousel();
            }
            else if (plateSelected)
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

void SamplerSysTestDlg::ProcessScriptList( int32_t lineCnt )
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

int32_t SamplerSysTestDlg::DoScriptStep( int32_t stepIdx, int32_t lineCnt )
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
            Logger::L().Log ( MODULENAME, severity_level::debug2, boost::str( boost::format( "DoScriptStep: Script line read: lineIdx: %d,  line: '%s',  lineLen: %d" ) % lineIdx % line % lineLen) );

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

bool SamplerSysTestDlg::ParseScriptLine( std::string cmdLine, std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3 )
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
        Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "ParseScriptLine: cmdLine: %s, cmd: %s, param1: %d, param2: %d, param2: %d" ) %cmdLine % cmd % param1 % param2 % param3 ) );
        parseOk = true;
    }
    return parseOk;
}

bool SamplerSysTestDlg::RunScriptCmd( std::string cmd, uint32_t param1, uint32_t param2, uint32_t param3 )
{
    bool listExit = false;
    bool relativeMoveFlag = doRelativeMove;

    Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "RunScriptCmd: cmd: %s, param1: %d, param2: %d, param2: %d" ) % cmd % param1 % param2 % param3 ) );

    doRelativeMove = false;
    if ( cbiInit )
    {
        if ( cmd.compare( SamplerSysTestDlg::CmdSimModeStr ) == 0 )
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
        else if ( cmd.compare( SamplerSysTestDlg::CmdInitPlateStr ) == 0 )
        {
            DoPlateInit( false );
        }
        else if ( cmd.compare( SamplerSysTestDlg::CmdInitCarouselStr ) == 0 )
        {
            DoCarouselInit( false );
        }
        else if ( cmd.compare( SamplerSysTestDlg::CmdSelectPlateStr ) == 0 )
        {
            DoPlateSelect( false );
        }
        else if ( cmd.compare( SamplerSysTestDlg::CmdSelectCarouselStr ) == 0 )
        {
            DoCarouselSelect( false );
        }
        // Home the Theta motor
        else if ( cmd.compare( SamplerSysTestDlg::CmdHomeThetaStr ) == 0 )
        {
            OnBnClickedHomeThetaBtn();
        }
        // Set the current position as the Theta zero point
        else if ( cmd.compare( SamplerSysTestDlg::CmdZeroThetaStr ) == 0 )
        {
            OnBnClickedZeroThetaBtn();
        }
        // Home the Radius motor
        else if ( cmd.compare( SamplerSysTestDlg::CmdHomeRadiusStr ) == 0 )
        {
            OnBnClickedHomeRadiusBtn();
        }
        // Set the current position as the Radius zero point
        else if ( cmd.compare( SamplerSysTestDlg::CmdZeroRadiusStr ) == 0 )
        {
            OnBnClickedZeroRadiusBtn();
        }
        // move the Theta motor to the cal position
        else if ( cmd.compare( SamplerSysTestDlg::CmdThetaToCalStr ) == 0 )
        {
            if ( plateSelected )
            {
                if ( plateThetaCalPos != 0 )
                {
                    thetaEditVal = plateThetaCalPos;
                    DoGotoThetaPos();
                }
            }
        }
        // move the Radius motor to the center cal position
        else if ( cmd.compare( SamplerSysTestDlg::CmdRadiusToCalStr ) == 0 )
        {
            if ( plateSelected )
            {
                if ( plateRadiusCenterPos != 0 )
                {
                    radiusEditVal = plateRadiusCenterPos;
                    DoGotoRadiusPos();
                }
            }
        }
        // move the Theta motor to the specified absolute location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveThetaStr ) == 0 )
        {
            thetaEditVal = param1;
            DoGotoThetaPos();
        }
        // move the Theta motor to the specified relative location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveThetaRelStr ) == 0 )
        {
            thetaEditVal = param1;
            DoGotoThetaPos( true );
        }
        // move the Radius motor to the specified absolute location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveRadiusStr ) == 0 )
        {
            radiusEditVal = param1;
            DoGotoRadiusPos();
        }
        // move the Radius motor to the specified relative location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveRadiusRelStr ) == 0 )
        {
            radiusEditVal = param1;
            DoGotoRadiusPos( true );
        }
        // move the plate to the specified absolute theta/radius location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveRThetaStr ) == 0 )
        {
            thetaEditVal = param1;
            radiusEditVal = param2;
            DoGotoRThetaPos();
        }
        // move the plate to the specified relative theta/radius location
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveRThetaRelStr ) == 0 )
        {
            thetaEditVal = param1;
            radiusEditVal = param2;
            DoGotoRThetaPos( true );
        }
        // set the Theta motor cal position
        else if ( cmd.compare( SamplerSysTestDlg::CmdCalPlateThetaStr ) == 0 )
        {
            OnBnClickedSetThetaCalBtn();
        }
        // set the Radius motor center cal position
        else if ( cmd.compare( SamplerSysTestDlg::CmdCalPlateRadiusStr ) == 0 )
        {
            OnBnClickedSetRadiusCalBtn();
        }
        // Move plate to absolute radius, theta position
        else if ( cmd.compare( SamplerSysTestDlg::CmdMoveCarouselStr ) == 0 )
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
        else if ( cmd.compare( SamplerSysTestDlg::CmdMovePlateWellStr ) == 0 )
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
        else if ( cmd.compare( SamplerSysTestDlg::CmdMovePlateStr ) == 0 )
        {
            if ( plateSelected )
            {
                thetaEditVal = param1;
                radiusEditVal = param1;
                DoGotoRThetaPos();
            }
        }
        // Move plate to relative radius, theta position change values
        else if ( cmd.compare( SamplerSysTestDlg::CmdMovePlateRelStr ) == 0 )
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
        else if ( cmd.compare( SamplerSysTestDlg::CmdProbeHomeStr ) == 0 )
        {
            OnBnClickedProbeHomeBtn();
        }
        // Move the probe for the current active controller
        else if ( cmd.compare( SamplerSysTestDlg::CmdProbeMoveStr ) == 0 )
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
    if ( cmd.compare( SamplerSysTestDlg::CmdWaitForKeyPressStr ) == 0 )
    {
        int response = MessageBox( _T( "Click the OK button to continue, or the 'Cancel' button to quit." ), _T( "Waiting for user to continue..." ), MB_OKCANCEL );
        if ( response == IDCANCEL )
        {
            listExit = true;
        }
    }
    // insert a delay into the command stream processing
    else if ( cmd.compare( SamplerSysTestDlg::CmdSleepStr ) == 0 )
    {
        ScriptSleep( param1 );
    }
    // Stop processing the run list and exit the program.
    else if ( cmd.compare( SamplerSysTestDlg::CmdExitStr ) == 0 )
    {
        listExit = true;
    }

    return listExit;
}

void SamplerSysTestDlg::ScriptSleep( int32_t sleepTime )
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

void SamplerSysTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange( pDX );
    DDX_Control( pDX, IDC_SCRIPT_EDIT, ScriptEdit );
    DDX_Text   ( pDX, IDC_SCRIPT_EDIT, ScriptEditBuf );

    DDX_Control( pDX, IDC_THETA_HOME_CHK, ThetaHomeChk );
    DDX_Control( pDX, IDC_THETA_HOME_BTN, HomeThetaBtn );
    DDX_Control( pDX, IDC_ZERO_THETA_BTN, SetThetaZeroBtn );
    DDX_Control( pDX, IDC_SET_THETA_CAL_BTN, SetThetaCalBtn );
    DDX_Control( pDX, IDC_RADIUS_HOME_CHK, RadiusHomeChk );
    DDX_Control( pDX, IDC_RADIUS_HOME_BTN, HomeRadiusBtn );
    DDX_Control( pDX, IDC_ZERO_RADIUS_BTN, SetRadiusZeroBtn );
    DDX_Control( pDX, IDC_SET_RADIUS_CAL_BTN, SetRadiusCalBtn );

    DDX_Control( pDX, IDC_ROW_EDIT, RowValEdit );
    DDX_Text   ( pDX, IDC_ROW_EDIT, rowEditStr );
    DDV_MaxChars( pDX, rowEditStr, 3 );
    DDX_Control( pDX, IDC_CURRENT_ROW_DISP, CurrentRowDisp );
    DDX_Text   ( pDX, IDC_ROW_EDIT, rowCharStr );
    DDX_Control( pDX, IDC_COL_EDIT, ColValEdit );
    DDX_Text   ( pDX, IDC_COL_EDIT, colEditVal );
    DDV_MinMaxLong( pDX, colEditVal, 0, 12 );
    DDX_Control( pDX, IDC_CURRENT_COL_DISP, CurrentColDisp );
    DDX_Control( pDX, IDC_GOTO_ROW_COL_BTN, GotoRowColBtn );
    DDX_Control( pDX, IDC_NEXT_ROW_COL_BTN, NextRowColBtn );

    DDX_Control( pDX, IDC_THETA_POS_EDIT, ThetaPosEdit );
    DDX_Text   ( pDX, IDC_THETA_POS_EDIT, thetaEditVal );
    DDX_Control( pDX, IDC_CURRENT_THETA_DISP, CurrentThetaDisp );
    DDX_Text   ( pDX, IDC_CURRENT_THETA_DISP, thetaVal );
    DDX_Control( pDX, IDC_THETA_RAW_POS_DISP, RawThetaDisp );
    DDX_Text   ( pDX, IDC_THETA_RAW_POS_DISP, thetaRawVal );
    DDX_Control( pDX, IDC_POS_UPDATE_BTN, UpdateBtn );
    DDX_Control( pDX, IDC_GOTO_THETA_POS_BTN, GotoThetaBtn );
    DDX_Control( pDX, IDC_RADIUS_POS_EDIT, RadiusPosEdit );
    DDX_Text   ( pDX, IDC_RADIUS_POS_EDIT, radiusEditVal );
    DDX_Control( pDX, IDC_CURRENT_RADIUS_DISP, CurrentRadiusDisp );
    DDX_Text   ( pDX, IDC_CURRENT_RADIUS_DISP, radiusVal );
    DDX_Control( pDX, IDC_GOTO_RADIUS_POS_BTN, GotoRadiusBtn );
    DDX_Control( pDX, IDC_GOTO_RTHETA_POS_BTN, GotoRThetaBtn );

    DDX_Control( pDX, IDC_RELATIVE_MOVE_CHK, RelativeMoveChk );

    DDX_Control( pDX, IDC_TUBE_NUM_EDIT, TubeNumEdit );
    DDX_Text   ( pDX, IDC_TUBE_NUM_EDIT, tubeEditVal );
    DDX_Control( pDX, IDC_CURRENT_TUBE_NUM_DISP, CurrentTubeDisp );
    DDX_Text   ( pDX, IDC_CURRENT_TUBE_NUM_DISP, tubeNum );
    DDX_Control( pDX, IDC_GOTO_TUBE_BTN, GotoTubeNumBtn );
    DDX_Control( pDX, IDC_TUBE_DETECTED_CHK, TubePresentChk );
    DDX_Control( pDX, IDC_FIND_TUBE_BTN, FindTubeBtn );
    DDX_Control( pDX, IDC_NEXT_TUBE_BTN, NextTubeBtn );

    DDX_Control( pDX, IDC_CURRENT_PROBE_DISP, CurrentProbeDisp );
    DDX_Text   ( pDX, IDC_CURRENT_PROBE_DISP, probeVal );
    DDX_Control( pDX, IDC_PROBE_HOME_CHK, ProbeHomeChk );
    DDX_Control( pDX, IDC_PROBE_HOME_BTN, HomeProbeBtn );
    DDX_Control( pDX, IDC_PROBE_INIT_BTN, InitProbeBtn );
    DDX_Control( pDX, IDC_PROBE_UP_BTN, ProbeUpBtn );
    DDX_Control( pDX, IDC_PROBE_DOWN_BTN, ProbeDownBtn );

    DDX_Control( pDX, IDC_RADIO_PLATE_SELECT, PlateSelectRadio );
    DDX_Control( pDX, IDC_PLATE_INIT_ERROR_CHK, PlateInitErrorChk );
    DDX_Control( pDX, IDC_PLATE_CAL_BTN, CalibratePlateBtn );
    DDX_Control( pDX, IDC_PLATE_EJECT_BTN, EjectPlateBtn );
    DDX_Control( pDX, IDC_PLATE_LOAD_BTN, LoadPlateBtn );
    DDX_Control( pDX, IDC_PLATE_INIT_BTN, InitPlateBtn );
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
    DDX_Check  ( pDX, IDC_CAROUSEL_DETECTED_CHK, carouselPresent );
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
    DDX_Control( pDX, IDC_STEP_SCRIPT_BTN, StepScriptBtn );
    DDX_Control( pDX, IDC_CMD_STEP_EDIT, CmdStepEdit );
    DDX_Control( pDX, IDC_CMD_STEP_EDIT_SPIN, StepNumSpin );
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

BEGIN_MESSAGE_MAP(SamplerSysTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

    ON_BN_CLICKED( IDC_THETA_HOME_BTN, &SamplerSysTestDlg::OnBnClickedHomeThetaBtn )
    ON_BN_CLICKED( IDC_RADIUS_HOME_BTN, &SamplerSysTestDlg::OnBnClickedHomeRadiusBtn )
    ON_BN_CLICKED( IDC_ZERO_THETA_BTN, &SamplerSysTestDlg::OnBnClickedZeroThetaBtn )
    ON_BN_CLICKED( IDC_ZERO_RADIUS_BTN, &SamplerSysTestDlg::OnBnClickedZeroRadiusBtn )
    ON_BN_CLICKED( IDC_SET_THETA_CAL_BTN, &SamplerSysTestDlg::OnBnClickedSetThetaCalBtn )
    ON_BN_CLICKED( IDC_SET_RADIUS_CAL_BTN, &SamplerSysTestDlg::OnBnClickedSetRadiusCalBtn )

    ON_EN_CHANGE ( IDC_ROW_EDIT, &SamplerSysTestDlg::OnEnChangeRowEdit )
    ON_EN_CHANGE ( IDC_COL_EDIT, &SamplerSysTestDlg::OnEnChangeColEdit )
    ON_BN_CLICKED( IDC_GOTO_ROW_COL_BTN, &SamplerSysTestDlg::OnBnClickedGotoRowColBtn )
    ON_BN_CLICKED( IDC_NEXT_ROW_COL_BTN, &SamplerSysTestDlg::OnBnClickedNextRowColBtn )

    ON_EN_CHANGE ( IDC_THETA_POS_EDIT, &SamplerSysTestDlg::OnEnChangeThetaPosEdit )
    ON_BN_CLICKED( IDC_GOTO_THETA_POS_BTN, &SamplerSysTestDlg::OnBnClickedGotoThetaPosBtn )
    ON_BN_CLICKED( IDC_POS_UPDATE_BTN, &SamplerSysTestDlg::OnBnClickedPosUpdateBtn )

    ON_EN_CHANGE ( IDC_RADIUS_POS_EDIT, &SamplerSysTestDlg::OnEnChangeRadiusPosEdit )
    ON_BN_CLICKED( IDC_GOTO_RADIUS_POS_BTN, &SamplerSysTestDlg::OnBnClickedGotoRadiusPosBtn )
    ON_BN_CLICKED( IDC_GOTO_RTHETA_POS_BTN, &SamplerSysTestDlg::OnBnClickedGotoRThetaPosBtn )

    ON_BN_CLICKED( IDC_RELATIVE_MOVE_CHK, &SamplerSysTestDlg::OnBnClickedRelativeMoveChk )

    ON_EN_CHANGE ( IDC_TUBE_NUM_EDIT, &SamplerSysTestDlg::OnEnChangeTubeNumEdit )
    ON_BN_CLICKED( IDC_GOTO_TUBE_BTN, &SamplerSysTestDlg::OnBnClickedGotoTubeBtn )
    ON_BN_CLICKED( IDC_NEXT_TUBE_BTN, &SamplerSysTestDlg::OnBnClickedNextTubeBtn )
    ON_BN_CLICKED( IDC_FIND_TUBE_BTN, &SamplerSysTestDlg::OnBnClickedFindTubeBtn )

    ON_BN_CLICKED( IDC_PROBE_HOME_BTN, &SamplerSysTestDlg::OnBnClickedProbeHomeBtn )
    ON_BN_CLICKED( IDC_PROBE_INIT_BTN, &SamplerSysTestDlg::OnBnClickedProbeInitBtn )
    ON_BN_CLICKED( IDC_PROBE_UP_BTN, &SamplerSysTestDlg::OnBnClickedProbeUpBtn )
    ON_BN_CLICKED( IDC_PROBE_DOWN_BTN, &SamplerSysTestDlg::OnBnClickedProbeDownBtn )

    ON_BN_CLICKED( IDC_RADIO_PLATE_SELECT, &SamplerSysTestDlg::OnBnClickedRadioPlateSelect )
    ON_BN_CLICKED( IDC_PLATE_CAL_BTN, &SamplerSysTestDlg::OnBnClickedPlateCalBtn )
    ON_BN_CLICKED( IDC_PLATE_EJECT_BTN, &SamplerSysTestDlg::OnBnClickedPlateEjectBtn )
    ON_BN_CLICKED( IDC_PLATE_LOAD_BTN, &SamplerSysTestDlg::OnBnClickedPlateLoadBtn )
    ON_BN_CLICKED( IDC_PLATE_INIT_BTN, &SamplerSysTestDlg::OnBnClickedPlateInitBtn )
    ON_BN_CLICKED( IDC_PLATE_THETA_ADJUST_LEFT_BTN, &SamplerSysTestDlg::OnBnClickedPlateThetaAdjustLeftBtn )
    ON_BN_CLICKED( IDC_PLATE_THETA_ADJUST_RIGHT_BTN, &SamplerSysTestDlg::OnBnClickedPlateThetaAdjustRightBtn )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_ADJUST_IN_BTN, &SamplerSysTestDlg::OnBnClickedPlateRadiusAdjustInBtn )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_ADJUST_OUT_BTN, &SamplerSysTestDlg::OnBnClickedPlateRadiusAdjustOutBtn )
    ON_EN_CHANGE ( IDC_PLATE_RADIUS_BACKLASH_EDIT, &SamplerSysTestDlg::OnEnChangePlateRadiusBacklashEdit )
    ON_BN_CLICKED( IDC_PLATE_RADIUS_BACKLASH_SET_BTN, &SamplerSysTestDlg::OnBnClickedPlateRadiusBacklashSetBtn )
    ON_EN_CHANGE ( IDC_PLATE_THETA_BACKLASH_EDIT, &SamplerSysTestDlg::OnEnChangePlateThetaBacklashEdit )
    ON_BN_CLICKED( IDC_PLATE_THETA_BACKLASH_SET_BTN, &SamplerSysTestDlg::OnBnClickedPlateThetaBacklashSetBtn )

    ON_BN_CLICKED( IDC_RADIO_CAROUSEL_SELECT, &SamplerSysTestDlg::OnBnClickedRadioCarouselSelect )
    ON_BN_CLICKED( IDC_CAROUSEL_CAL_BTN, &SamplerSysTestDlg::OnBnClickedCarouselCalBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_EJECT_BTN, &SamplerSysTestDlg::OnBnClickedCarouselEjectBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_LOAD_BTN, &SamplerSysTestDlg::OnBnClickedCarouselLoadBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_INIT_BTN, &SamplerSysTestDlg::OnBnClickedCarouselInitBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_ADJUST_LEFT_BTN, &SamplerSysTestDlg::OnBnClickedCarouselThetaAdjustLeftBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_RADIUS_ADJUST_IN_BTN, &SamplerSysTestDlg::OnBnClickedCarouselRadiusAdjustInBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_ADJUST_RIGHT_BTN, &SamplerSysTestDlg::OnBnClickedCarouselThetaAdjustRightBtn )
    ON_BN_CLICKED( IDC_CAROUSEL_RADIUS_ADJUST_OUT_BTN, &SamplerSysTestDlg::OnBnClickedCarouselRadiusAdjustOutBtn )
    ON_EN_CHANGE ( IDC_CAROUSEL_THETA_BACKLASH_EDIT, &SamplerSysTestDlg::OnEnChangeCarouselThetaBacklashEdit )
    ON_BN_CLICKED( IDC_CAROUSEL_THETA_BACKLASH_SET_BTN, &SamplerSysTestDlg::OnBnClickedCarouselThetaBacklashSetBtn )

    ON_BN_CLICKED( IDC_RUN_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedRunScriptBtn )
    ON_BN_CLICKED( IDC_STEP_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedStepScriptBtn )
    ON_EN_VSCROLL( IDC_SCRIPT_EDIT, &SamplerSysTestDlg::OnEnVscrollScriptEdit )
    ON_EN_CHANGE ( IDC_CMD_STEP_EDIT, &SamplerSysTestDlg::OnEnChangeCmdStepEdit )
    ON_BN_CLICKED( IDC_PAUSE_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedPauseScriptBtn )
    ON_BN_CLICKED( IDC_STOP_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedStopScriptBtn )
    ON_EN_CHANGE ( IDC_SCRIPT_PATH_EDIT, &SamplerSysTestDlg::OnEnChangeScriptPathEdit )
    ON_BN_CLICKED( IDC_LOAD_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedLoadScriptBtn )
    ON_BN_CLICKED( IDC_CLEAR_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedClearScriptBtn )
    ON_BN_CLICKED( IDC_SAVE_SCRIPT_BTN, &SamplerSysTestDlg::OnBnClickedSaveScriptBtn )
    ON_BN_CLICKED( IDOK, &SamplerSysTestDlg::OnBnClickedExit )
END_MESSAGE_MAP()


// SamplerSysTestDlg message handlers

BOOL SamplerSysTestDlg::OnInitDialog()
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

    ShowRThetaPositions( thetaVal, thetaRawVal, radiusVal );
    ShowRowColPositions( rowVal, colVal );
    ShowTubeNum( tubeNum );

    int32_t chkState = BST_UNCHECKED;

    RelativeMoveChk.EnableWindow( TRUE );
    if ( doRelativeMove )
    {
        chkState = BST_CHECKED;
    }
    RelativeMoveChk.SetCheck( chkState );

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

    SetSamplerControlEnable( TRUE );
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

    // set timer to do automatic periodic status updates...
    pUpdTimer->expires_from_now( boost::posix_time::milliseconds( SignalsInterval ), updTimerError );               // should cancel any pending async operations
    pUpdTimer->async_wait( std::bind( &SamplerSysTestDlg::UpdateSignalStatus, this, std::placeholders::_1 ) );      // restart the periodic update timer as time from now...

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

void SamplerSysTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void SamplerSysTestDlg::OnPaint()
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
// the minimized window.
HCURSOR SamplerSysTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void SamplerSysTestDlg::SetAllControlEnable( BOOL action )
{
    UpdateBtn.EnableWindow( action );

    SetSamplerControlEnable( action );
    SetPlateControlEnable( action );
    SetCarouselControlEnable( action );

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    RunScriptBtn.EnableWindow( action );
    StepScriptBtn.EnableWindow( action );
    CmdStepEdit.SetReadOnly( rdOnly );
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

void SamplerSysTestDlg::SetSamplerControlEnable( BOOL action )
{
    BOOL setAction = FALSE;

    if ( ( plateSelected ) || ( carouselSelected ) )
    {
        setAction = action;
    }

    BOOL rdOnly = ( action == FALSE ) ? TRUE : FALSE;
    ThetaPosEdit.SetReadOnly( rdOnly );
    SetThetaZeroBtn.EnableWindow( action );
    SetThetaCalBtn.EnableWindow( action );
    GotoThetaBtn.EnableWindow( setAction );

    RadiusPosEdit.SetReadOnly( rdOnly );
    SetRadiusZeroBtn.EnableWindow( action );
    SetRadiusCalBtn.EnableWindow( action );
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
    InitProbeBtn.EnableWindow( setAction );
}

void SamplerSysTestDlg::SetPlateControlEnable( BOOL action )
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

    PlateThetaAdjustLeftBtn.EnableWindow( action );
    PlateThetaAdjustRightBtn.EnableWindow( action );
    PlateRadiusAdjustInBtn.EnableWindow( action );
    PlateRadiusAdjustOutBtn.EnableWindow( action );

    PlateRadiusBacklashEdit.SetReadOnly( rdOnly );
    PlateRadiusBacklashSetBtn.EnableWindow( action );
    PlateThetaBacklashEdit.SetReadOnly( rdOnly );
    PlateThetaBacklashSetBtn.EnableWindow( action );
}

void SamplerSysTestDlg::SetCarouselControlEnable( BOOL action )
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

void SamplerSysTestDlg::SetMsgBoxControlEnable( bool enable, MsgBoxTypes msg_type )
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

void SamplerSysTestDlg::UpdateSignalStatus( const boost::system::error_code error )
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
        pUpdTimer->async_wait( std::bind( &SamplerSysTestDlg::UpdateSignalStatus, this, std::placeholders::_1 ) );      // restart the periodic update timer as time from now...
    }
    else
    {
        timerComplete = true;
    }
}

void SamplerSysTestDlg::UpdateSignals( void )
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

            int32_t tVal = thetaVal;
            int32_t raw_tVal = thetaRawVal;
            int32_t rVal = radiusVal;
            int32_t pVal = probeVal;
            int32_t tNum = tubeNum;
            int32_t rNum = rowVal;
            int32_t cNum = colVal;

            if ( !quitting )
            {
                GetRThetaPositions( tVal, raw_tVal, rVal );
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
        }
    }
}

void SamplerSysTestDlg::StartSignalsUpdateThread( void )
{
    if ( !quitting )
    {
        if ( ( !signalUpdateStarting ) && ( !signalUpdateRunning ) )
        {
            signalUpdateStarting = true;
            pSignalsThread.reset( new std::thread( &SamplerSysTestDlg::ShowSignals, this ) );
            pSignalsThread->detach();
        }
    }
}

void SamplerSysTestDlg::ShowSignals( void )
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
                            // update the positional info, also
                            ShowRThetaPositions( thetaVal, thetaRawVal, radiusVal );
                            break;

                        case 4:
                            ShowProbePosition( probeVal );
                            break;

                        case 5:
                            ShowTubeNum( tubeNum );
                            break;

                        case 6:
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
void SamplerSysTestDlg::ShowHomeStates( void )
{
    int32_t thetaState   = BST_UNCHECKED;
    int32_t radiusState  = BST_UNCHECKED;
    int32_t probeState   = BST_UNCHECKED;

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

    if ( !quitting )
    {
        ThetaHomeChk.SetCheck( thetaState );
        RadiusHomeChk.SetCheck( radiusState );
        ProbeHomeChk.SetCheck( probeState );
    }
}

void SamplerSysTestDlg::UpdateProbePosition( void )
{
    int32_t probePos = probeVal;

    GetProbePosition( probePos );
    ShowProbePosition( probePos );
}

void SamplerSysTestDlg::GetProbePosition( int32_t probe_pos )
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

void SamplerSysTestDlg::ShowProbePosition( int32_t probe_pos )
{
    CString valStr;

    valStr.Format( _T( "%d" ), probe_pos );
    cntlEdit = true;
    CurrentProbeDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void SamplerSysTestDlg::UpdateStep( int stepIdx )
{
    cntlEdit = true;

    ShowStepNum( stepIdx + 1 );

    cntlEdit = false;
}

void SamplerSysTestDlg::ShowStepNum(int lineNum)
{
    CString valStr;

    StepNumSpin.SetPos32( lineNum );
    valStr.Empty();
    if ( lineNum >= 0 )
    {
        valStr.Format( _T( "%d" ), lineNum );
    }
    CmdStepEdit.SetWindowText( valStr );

    cntlEdit = false;
}

void SamplerSysTestDlg::UpdateRThetaPositions( void )
{
    int32_t tVal = thetaVal;
    int32_t raw_tVal = thetaRawVal;
    int32_t rVal = radiusVal;

    GetRThetaPositions( tVal, raw_tVal, rVal );
    ShowRThetaPositions(tVal, raw_tVal, rVal);
}

void SamplerSysTestDlg::GetRThetaPositions( int32_t & tVal, int32_t & raw_tVal, int32_t & rVal )
{
    if ( !simMode )
    {
        if ( plateSelected )
        {
            pPlateController->GetMotors( true, pPlateTheta, true, pPlateRadius, true, pPlateProbe );
            pPlateController->PlatePosition( thetaVal, radiusVal );
            thetaRawVal = pPlateTheta->GetPosition();
        }
        else
        {
            pCarouselController->GetMotors( true, pCarouselTheta, true, pCarouselRadius, true, pCarouselProbe );
            thetaRawVal = pCarouselTheta->GetPosition();                // get the raw thata position
            radiusVal = pCarouselRadius->GetPosition();
            if ( carouselSelected )
            {
                pCarouselController->CarouselPosition( thetaVal );          // get the normalized theta position
            }
            else
            {
                thetaVal = thetaRawVal;
                while ( thetaVal >= MaxThetaPosition )
                {
                    thetaVal -= MaxThetaPosition;
                }
                 
                while ( thetaVal < 0 )
                {
                    thetaVal += MaxThetaPosition;
                }
            }
        }
    }
    tVal = thetaVal;
    raw_tVal = thetaRawVal;
    rVal = radiusVal;
}

void SamplerSysTestDlg::ShowRThetaPositions( int32_t tPos, int32_t raw_tPos, int32_t rPos )
{
    CString valStr;

    cntlEdit = true;

    valStr.Format( _T( "%d" ), tPos );
    CurrentThetaDisp.SetWindowText( valStr );
    valStr.Format( _T( "%d" ), raw_tPos );
    RawThetaDisp.SetWindowText( valStr );
    valStr.Format( _T( "%d" ), rPos );
    CurrentRadiusDisp.SetWindowText( valStr );

    cntlEdit = false;
}

void SamplerSysTestDlg::UpdateRowColPositions( void )
{
    int32_t row = rowVal;
    int32_t col = colVal;

    GetRowColPositions(row, col);
    ShowRowColPositions( row, col );
}

void SamplerSysTestDlg::GetRowColPositions( int32_t & row, int32_t & col )
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

void SamplerSysTestDlg::ShowRowColPositions( int32_t row, int32_t col )
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

void SamplerSysTestDlg::UpdateTubeNum( void )
{
    int32_t tube = tubeNum;

    GetTubeNum ( tube );
    ShowTubeNum( tube );
}

void SamplerSysTestDlg::GetTubeNum( int32_t & tube )
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

void SamplerSysTestDlg::ShowTubeNum( int32_t tube_num )
{
    CString valStr;

    valStr.Format( _T( "%d" ), tube_num );

    cntlEdit = true;
    CurrentTubeDisp.SetWindowText( valStr );
    cntlEdit = false;
}

void SamplerSysTestDlg::ShowCarouselPresent( BOOL present )
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

void SamplerSysTestDlg::ShowTubePresent( BOOL present )
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

void SamplerSysTestDlg::DoPlateSelect( bool msgWait )
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
                PlateInitErrorChk.SetCheck( BST_UNCHECKED );
                SetSamplerControlEnable( FALSE );

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

void SamplerSysTestDlg::DoCarouselSelect( bool msgWait )
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
            SetPlateControlEnable( plateSelected );
            UpdateRowColPositions();
        }

        if ( !carouselSelected )
        {
            if ( pCarouselController->ControllerOk() )
            {
                if ( CarouselSelectRadio.GetCheck() == BST_UNCHECKED)
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

bool SamplerSysTestDlg::DoGotoThetaPos( bool relativeMove )
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
            if ( relativeMove )
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

        if ( relativeMove )
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
                if ( relativeMove )
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

bool SamplerSysTestDlg::DoGotoRadiusPos( bool relativeMove )
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

        if ( relativeMove )
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
                if ( relativeMove )
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

bool SamplerSysTestDlg::DoGotoRThetaPos( bool relativeMove )
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

void SamplerSysTestDlg::DoGotoTubeBtn( bool msgWait )
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

void SamplerSysTestDlg::DoPlateInit( bool msgWait )
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

            std::string logStr = boost::str( boost::format( "DoPlateInit:: initialization %s ") % ( ( initOk == true ) ? "succeeded" : " failed" ) );
            Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

            if ( ( !initOk ) && ( showInitErrors ) )
            {
                PlateInitErrorChk.SetCheck( BST_CHECKED );
            }
            else
            {
                initOk = true;
            }

            if ( ( initOk )  && ( !plateControlsInited ) )
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

void SamplerSysTestDlg::DoCarouselInit( bool msgWait )
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

void SamplerSysTestDlg::StartInfoDialog( void )
{
    if ( ( !dlgStarting ) && ( !dlgRunning ) && ( !scriptStarting ) && ( !scriptRunning ) )
    {
        SetAllControlEnable( FALSE );                       // disable ALL controls first
        PauseScriptBtn.EnableWindow( FALSE );               // disable the ones not handled by the generic control enable handler
        StopScriptBtn.EnableWindow( FALSE );

        dlgStarting = true;
        dlgRunning = false;
        pDlgThread.reset( new std::thread( &SamplerSysTestDlg::ShowInfoDialog, this ) );
        pDlgThread->detach();
    }
}

void SamplerSysTestDlg::ShowInfoDialog( void )
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
                                       _T( "First, manually adjust the radius mechanism to position the probe\r\n"),
                                       _T( "at the center calibration point of the plate calibration template.\r\n\r\n"),
                                       _T( "Theta adjustments may be made as necessary, but Theta calibration\r\n"),
                                       _T( "will occur later in the calibration process.\r\n\r\n"),
                                       _T( "Press 'Enter' or click/press the 'OK' button when done and ready to\r\n"),
                                       _T( "continue to the next step, or 'Cancel' to abort the calibration.") );

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

                        msgStr.Format(  _T( "%s%s%s%s%s%s%s%s" ),
                                        //   1        10        20        30        40        50        60        70        80
                                        //   |        |         |         |         |         |         |         |         |
                                        _T( "Next, manually adjust the radius and theta mechanisms to position the probe\r\n" ),
                                        _T( "at the row alignment calibration point of the plate calibration template.\r\n\r\n" ),
                                        _T( "The radius may be adjusted to position the probe at or beyond the end of the\r\n" ),
                                        _T( "calibration template.  Adjusting the radius to place the probe slightly beyond\r\n"),
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
                    msgStr.Format(  _T( "%s%s%s%s%s%s" ),
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

                    dlgResponse = MessageBox( msgStr,  _T( "Calibrate the Carousel radius and theta positions: Start" ), MB_OKCANCEL );

                    if ( dlgResponse == IDOK )
                    {
                        if ( !simMode )
                        {
                            pCarouselController->HomeTheta();
                            pCarouselController->HomeRadius();
                        }

                        msgStr.Format(  _T( "%s%s%s%s%s" ),
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
                        msgStr.Format(  _T( "%s%s%s%s" ),
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

void SamplerSysTestDlg::OnEnChangeThetaPosEdit()
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

void SamplerSysTestDlg::OnBnClickedGotoThetaPosBtn()
{
    DoGotoThetaPos( doRelativeMove );
}

void SamplerSysTestDlg::OnBnClickedZeroThetaBtn()
{
    msgType = MsgBoxTypeThetaZero;
    StartInfoDialog();
}

void SamplerSysTestDlg::OnBnClickedSetThetaCalBtn()
{
    msgType = MsgBoxTypeThetaCal;
    StartInfoDialog();
}

void SamplerSysTestDlg::OnBnClickedHomeThetaBtn()
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
        std::string logStr = boost::str( boost::format( "OnBnHomeThetaBtn: %s returned an error from '%s'.") % controllerName % methodName );

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

void SamplerSysTestDlg::OnEnChangeRadiusPosEdit()
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

void SamplerSysTestDlg::OnBnClickedGotoRadiusPosBtn()
{
    DoGotoRadiusPos( doRelativeMove );
}

void SamplerSysTestDlg::OnBnClickedZeroRadiusBtn()
{
    msgType = MsgBoxTypeRadiusZero;
    StartInfoDialog();
}

void SamplerSysTestDlg::OnBnClickedSetRadiusCalBtn()
{
    msgType = MsgBoxTypeRadiusCal;
    StartInfoDialog();
}

void SamplerSysTestDlg::OnBnClickedHomeRadiusBtn()
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

void SamplerSysTestDlg::OnBnClickedGotoRThetaPosBtn()
{
    DoGotoRThetaPos( doRelativeMove );
}

void SamplerSysTestDlg::OnBnClickedRelativeMoveChk()
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

void SamplerSysTestDlg::OnBnClickedProbeUpBtn()
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

void SamplerSysTestDlg::OnBnClickedProbeDownBtn()
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

void SamplerSysTestDlg::OnBnClickedProbeHomeBtn()
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

void SamplerSysTestDlg::OnBnClickedProbeInitBtn()
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

void SamplerSysTestDlg::OnEnChangeRowEdit()
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

void SamplerSysTestDlg::OnEnChangeColEdit()
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

void SamplerSysTestDlg::OnBnClickedGotoRowColBtn()
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

void SamplerSysTestDlg::OnBnClickedNextRowColBtn()
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

void SamplerSysTestDlg::OnEnChangeTubeNumEdit()
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

void SamplerSysTestDlg::OnBnClickedGotoTubeBtn()
{
    DoGotoTubeBtn( true );
}

void SamplerSysTestDlg::OnBnClickedNextTubeBtn()
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

void SamplerSysTestDlg::OnBnClickedFindTubeBtn()
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

void SamplerSysTestDlg::OnBnClickedPosUpdateBtn()
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

void SamplerSysTestDlg::OnBnClickedRadioPlateSelect()
{
    DoPlateSelect( true );
}

void SamplerSysTestDlg::OnBnClickedPlateCalBtn()
{
    if ( plateSelected )
    {
        msgType = MsgBoxTypeCalibratePlate;
        StartInfoDialog();
    }
}

void SamplerSysTestDlg::OnBnClickedPlateEjectBtn()
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

void SamplerSysTestDlg::OnBnClickedPlateLoadBtn()
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

void SamplerSysTestDlg::OnBnClickedPlateInitBtn()
{
    if ( plateSelected )
    {
        DoPlateInit( true );
    }
}

void SamplerSysTestDlg::OnBnClickedPlateThetaAdjustLeftBtn()
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

void SamplerSysTestDlg::OnBnClickedPlateThetaAdjustRightBtn()
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

void SamplerSysTestDlg::OnBnClickedPlateRadiusAdjustInBtn()
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

void SamplerSysTestDlg::OnBnClickedPlateRadiusAdjustOutBtn()
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

void SamplerSysTestDlg::OnEnChangePlateRadiusBacklashEdit()
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

void SamplerSysTestDlg::OnBnClickedPlateRadiusBacklashSetBtn()
{
    pPlateController->SetRadiusBacklash( plateRadiusBacklash );
}

void SamplerSysTestDlg::OnEnChangePlateThetaBacklashEdit()
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

void SamplerSysTestDlg::OnBnClickedPlateThetaBacklashSetBtn()
{
    if ( plateSelected )
    {
        pPlateController->SetThetaBacklash( plateThetaBacklash );
    }
}

void SamplerSysTestDlg::OnBnClickedRadioCarouselSelect()
{
    DoCarouselSelect( true );
}

void SamplerSysTestDlg::OnBnClickedCarouselCalBtn()
{
    if ( carouselSelected )
    {
        msgType = MsgBoxTypeCalibrateCarousel;
        StartInfoDialog();
    }
}

void SamplerSysTestDlg::OnBnClickedCarouselEjectBtn()
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

void SamplerSysTestDlg::OnBnClickedCarouselLoadBtn()
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

void SamplerSysTestDlg::OnBnClickedCarouselInitBtn()
{
    if ( carouselSelected )
    {
        DoCarouselInit( true );
    }
}

void SamplerSysTestDlg::OnBnClickedCarouselThetaAdjustLeftBtn()
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

void SamplerSysTestDlg::OnBnClickedCarouselThetaAdjustRightBtn()
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

void SamplerSysTestDlg::OnBnClickedCarouselRadiusAdjustInBtn()
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

void SamplerSysTestDlg::OnBnClickedCarouselRadiusAdjustOutBtn()
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

void SamplerSysTestDlg::OnEnChangeCarouselThetaBacklashEdit()
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

void SamplerSysTestDlg::OnBnClickedCarouselThetaBacklashSetBtn()
{
    if ( carouselSelected )
    {
        pCarouselController->SetThetaBacklash( carouselThetaBacklash );
    }
}

void SamplerSysTestDlg::OnBnClickedRunScriptBtn()
{
    if ( ( !scriptStarting ) && ( !scriptRunning ) )
    {
        StartScript();
    }
}

void SamplerSysTestDlg::OnBnClickedStepScriptBtn()
{
    if ( ( !scriptStarting ) && ( !scriptRunning ) )
    {
        int32_t stepIdx = cmdStepLine - 1;

        scriptLines = ScriptEdit.GetLineCount();

        if ( ( stepIdx >= 0 ) && ( stepIdx < scriptLines ) )
        {
            DoScriptStep( stepIdx, scriptLines );
        }
    }
}

void SamplerSysTestDlg::OnEnVscrollScriptEdit()
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

void SamplerSysTestDlg::OnEnChangeCmdStepEdit()
{
    CString valStr;

    if ( !cntlEdit )
    {
        cntlEdit = true;

        valStr.Empty();
        CmdStepEdit.GetWindowText( valStr );

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

void SamplerSysTestDlg::OnBnClickedPauseScriptBtn()
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

void SamplerSysTestDlg::OnBnClickedStopScriptBtn()
{
    stopRun = true;     // notify the script runner theread of the request to stop
}

void SamplerSysTestDlg::OnEnChangeScriptPathEdit()
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

void SamplerSysTestDlg::OnBnClickedLoadScriptBtn()
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

            int32_t lineCnt = ReadListFile(cmdBlock);

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

void SamplerSysTestDlg::OnBnClickedSaveScriptBtn()
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

void SamplerSysTestDlg::OnBnClickedClearScriptBtn()
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

void SamplerSysTestDlg::OnBnClickedExit()
{
    quitting = true;

    CDialogEx::OnOK();
}

