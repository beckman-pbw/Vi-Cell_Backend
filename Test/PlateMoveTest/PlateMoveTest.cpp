#include "stdafx.h"

#include <boost/filesystem.hpp>

#include <memory>
#include <iostream>
#include <sstream>

#include "Configuration.hpp"
#include "MotorBase.hpp"
#include "PlateMoveTest.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "PlateMoveTest";
static const char CONTROLLERNODENAME[] = "plate_controller";

//*****************************************************************************
PlateMoveTest::PlateMoveTest()
    : platePositionTolerance( DefaultPositionTolerance )
    , plateThetaHomePos( 0 )
    , plateThetaHomePosOffset( 0 )
    , plateThetaA1Pos(0)
    , plateThetaA1PosOffset( 0 )
    , plateThetaH12Pos(0)
    , plateThetaH12PosOffset(0)
    , plateThetaExtentPos( 0 )
    , plateThetaExtentPosOffset( 0 )
    , thetaStartTimeout( MotorStartTimeout )
    , thetaFullTimeout( ThetaFullTimeout )
    , plateRadiusHomePos( 0 )
    , plateRadiusHomePosOffset( 0 )
    , plateRadiusMaxTravel(0)
    , plateRadiusOutPos(0)
    , plateRadiusOutPosOffset(0)
    , plateRadiusA1Pos(0)
    , plateRadiusA1PosOffset( 0 )
    , plateRadiusH12Pos(0)
    , plateRadiusH12PosOffset(0)
    , plateRadiusExtentPos( 0 )
    , plateRadiusExtentPosOffset( 0 )
    , radiusStartTimeout( MotorStartTimeout )
    , radiusFullTimeout( RadiusFullTimeout )
{
    pLocalIosvc_.reset( new boost::asio::io_service() );
    pLocalWork_.reset( new boost::asio::io_service::work( *pLocalIosvc_ ) );

    // Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
    auto THREAD = std::bind( static_cast <std::size_t( boost::asio::io_service::* )( void )> ( &boost::asio::io_service::run ), pLocalIosvc_.get() );
    pThread_.reset( new std::thread( THREAD ) );

    pSignals_.reset( new boost::asio::signal_set( *pLocalIosvc_, SIGINT, SIGTERM ) );
    pCbi_.reset( new ControllerBoardInterface( *pLocalIosvc_, "BCI_001A", "BCI_001B" ) );
    pPlateController.reset( new PlateController( *pLocalIosvc_, pCbi_ ) );

    inited_ = false;
}

//*****************************************************************************
PlateMoveTest::~PlateMoveTest()
{
    pCbi_.reset();
    pLocalWork_.reset();   // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
    pLocalIosvc_->stop();  // instruct io_service to stop processing (should exit ::run() and end thread.
    pLocalIosvc_.reset();  // Destroys the queue
}

//*****************************************************************************
void PlateMoveTest::quit()
{
    pCbi_->CloseSerial();
}

//*****************************************************************************
void PlateMoveTest::signalHandler( const boost::system::error_code& ec, int signal_number )
{
    if ( ec )
    {
        Logger::L().Log( MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"" );
    }

    Logger::L().Log( MODULENAME, severity_level::critical, boost::str( boost::format( "Received signal no. %d" ) % signal_number ).c_str() );

    // All done listening.
    pSignals_->cancel();

    // Try to get out of here.
    pLocalIosvc_->post( std::bind( &PlateMoveTest::quit, this ) );
}

//*****************************************************************************
bool PlateMoveTest::init()
{
    Logger::L().Log( MODULENAME, severity_level::debug2, "init: <enter>" );

    if ( inited_ )
    {
        return true;
    }

    pCbi_->Initialize();

    configFile = "MotorControl.info";
    InitInfoFile();
    pPlateController->Init( "BCI_001A", ptfilecfg_, true, configFile );
    MotorBase * pTemp = NULL;
    pPlateController->GetMotors( true, pTheta, true, pRadius, false, pTemp );

    Logger::L().Log( MODULENAME, severity_level::debug2, "init: <exit>" );

    t_opPTree   controllersNode;
    t_opPTree   thisNode;

    controllerConfigNode.reset();
    controllersNode.reset();
    thisNode.reset();

    controllersNode = ptconfig->get_child_optional( "motor_controllers" );      // look for the controllers section for individualized parameters...
    if ( controllersNode )
    {
        thisNode = controllersNode->get_child_optional( CONTROLLERNODENAME );     // look for this specifc controller
        if ( thisNode )
        {
            controllerConfigNode = thisNode->get_child_optional( "controller_config" );
            if ( controllerConfigNode )
            {
                ConfigPlateVariables();
            }
        }
    }

    inited_ = true;

    return true;
}

bool PlateMoveTest::InitInfoFile( void )
{
    bool success = true;
    boost::system::error_code ec;

    if ( !ptfilecfg_ )
    {
        ptfilecfg_ = ConfigUtils::OpenConfigFile( configFile, ec, true );
    }

    if ( !ptfilecfg_ )
    {
        Logger::L().Log( MODULENAME, severity_level::error, "Error opening configuration file \"" + configFile + "\"" );
        success = false;
    }
    else
    {
        if ( !ptconfig )
        {
            ptconfig = ptfilecfg_->get_child_optional( "config" );
        }

        if ( !ptconfig )
        {
            Logger::L().Log( MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile + "\" - \"config\" section not found" );
            success = false;
        }
    }
    return success;
}

void PlateMoveTest::ConfigPlateVariables( void )
{
    boost::property_tree::ptree & config = controllerConfigNode.get();

    platePositionTolerance = config.get<int32_t>( "PlatePositionTolerance", platePositionTolerance );
    plateThetaHomePos = config.get<int32_t>( "PlateThetaHomePos", plateThetaHomePos );
    plateThetaHomePosOffset = config.get<int32_t>( "PlateThetaHomePosOffset", plateThetaHomePosOffset );
    plateThetaA1Pos = config.get<int32_t>( "PlateThetaA1Pos", plateThetaA1Pos );
    plateThetaA1PosOffset = config.get<int32_t>( "PlateThetaA1PosOffset", plateThetaA1PosOffset );
    plateThetaH12Pos = config.get<int32_t>( "PlateThetaH12Pos", plateThetaH12Pos );
    plateThetaH12PosOffset = config.get<int32_t>( "PlateThetaH12PosOffset", plateThetaH12PosOffset );
    plateThetaExtentPos = config.get<int32_t>( "PlateThetaExtentPos", plateThetaExtentPos );
    plateThetaExtentPosOffset = config.get<int32_t>( "PlateThetaExtentPosOffset", plateThetaExtentPosOffset );
    thetaStartTimeout = config.get<int32_t>( "ThetaStartTimeout", thetaStartTimeout );
    thetaFullTimeout = config.get<int32_t>( "ThetaFullTimeout", thetaFullTimeout );
    plateRadiusHomePos = config.get<int32_t>( "PlateRadiusHomePos", plateRadiusHomePos );
    plateRadiusHomePosOffset = config.get<int32_t>( "PlateRadiusHomePosOffset", plateRadiusHomePosOffset );
    plateRadiusMaxTravel = config.get<int32_t>( "PlateRadiusMaxTravel", plateRadiusMaxTravel );
    plateRadiusOutPos = config.get<int32_t>( "PlateRadiusOutPos", plateRadiusOutPos );
    plateRadiusOutPosOffset = config.get<int32_t>( "PlateRadiusOutPosOffset", plateRadiusOutPosOffset );
    plateRadiusA1Pos = config.get<int32_t>( "PlateRadiusA1Pos", plateRadiusA1Pos );
    plateRadiusA1PosOffset = config.get<int32_t>( "PlateRadiusA1PosOffset", plateRadiusA1PosOffset );
    plateRadiusH12Pos = config.get<int32_t>( "PlateRadiusH12Pos", plateRadiusH12Pos );
    plateRadiusH12PosOffset = config.get<int32_t>( "PlateRadiusH12PosOffset", plateRadiusH12PosOffset );
    plateRadiusExtentPos = config.get<int32_t>( "PlateRadiusExtentPos", plateRadiusExtentPos );
    plateRadiusExtentPosOffset = config.get<int32_t>( "PlateRadiusExtentPosOffset", plateRadiusExtentPosOffset );
    radiusStartTimeout = config.get<int32_t>( "RadiusStartTimeout", radiusStartTimeout );
    radiusFullTimeout = config.get<int32_t>( "RadiusFullTimeout", radiusFullTimeout );
}

//*****************************************************************************
bool PlateMoveTest::runMoveList( std::string runFile )
{
    uint32_t width = 0;
    uint32_t height = 0;
    bool readList = true;

    if ( !moveList_.open( runFile ) )
    {
        Logger::L().Log( MODULENAME, severity_level::error, "Unable to open Plate Move RunFile: " + runFile );
        return false;
    }

    boost::filesystem::path p( runFile );
    runFileName_ = p.stem().string();

    std::string cmd;
    uint32_t param1 = 0;
    uint32_t param2 = 0;
    uint32_t param3 = 0;

    while ( moveList_.read( cmd, param1, param2, param3 ) && (readList) )
    {
        // Initialize.
        if ( cmd.compare( PlateMoveList::CmdInitialize ) == 0 )
        {
            pRadius->Home();
            pRadius->DoMotorWait();

            pTheta->Home();
            pTheta->DoMotorWait();
        }
        // Home the Theta motor
        else if ( cmd.compare( PlateMoveList::CmdHomeTheta ) == 0 )
        {
            pTheta->Home();
            pTheta->DoMotorWait();
        }
        // Set the current position as the Theta zero point
        else if ( cmd.compare( PlateMoveList::CmdZeroTheta ) == 0 )
        {
            pTheta->MarkPosAsZero();
        }
        // Home the Radius motor
        else if ( cmd.compare( PlateMoveList::CmdHomeRadius ) == 0 )
        {
            pRadius->Home();
            pRadius->DoMotorWait();
        }
        // Set the current position as the Radius zero point
        else if ( cmd.compare( PlateMoveList::CmdZeroRadius ) == 0 )
        {
            pRadius->MarkPosAsZero();
        }
        // move the Theta and Radius motors to the A1 location
        else if ( cmd.compare( PlateMoveList::CmdA1 ) == 0 )
        {
            pPlateController->GoTo( ( plateThetaA1Pos + plateThetaA1PosOffset ),
                                    ( plateRadiusA1Pos + plateRadiusA1PosOffset ) );
        }
        // move the Theta and Radius motors to the H12 location
        else if ( cmd.compare( PlateMoveList::CmdH12 ) == 0 )
        {
            pPlateController->GoTo( ( plateThetaH12Pos + plateThetaH12PosOffset ),
                                    ( plateRadiusH12Pos + plateRadiusH12PosOffset ) );
        }
        // move the Theta motor to the specified location
        else if ( cmd.compare( PlateMoveList::CmdT ) == 0 )
        {
            pTheta->SetPosition(param1);
            pTheta->DoMotorWait();
        }
        // move the Radius motor to the specified location
        else if ( cmd.compare( PlateMoveList::CmdR ) == 0 )
        {
            pRadius->SetPosition(param1);
            pRadius->DoMotorWait();
        }
        // move the Theta motor to the cal1 position
        // move the Theta motor to the cal2 position
        else if ( ( cmd.compare( PlateMoveList::CmdCal1Theta ) == 0 ) ||
                  ( cmd.compare( PlateMoveList::CmdCal2Theta ) == 0 ) )
        {
            pTheta->SetPosition( param1 );
            pTheta->DoMotorWait();
        }
        // move the Radius motor to the cal1 position
        // move the Radius motor to the cal2 position
        else if ( ( cmd.compare( PlateMoveList::CmdCal1Radius ) == 0 ) ||
                  ( cmd.compare( PlateMoveList::CmdCal2Radius ) == 0 ) )
        {
            pRadius->SetPosition( param1 );
            pRadius->DoMotorWait();
        }
        // Move plate to absolute radius, theta position
        else if ( cmd.compare( PlateMoveList::CmdMovePlate ) == 0 )
        {
            pPlateController->GoTo( param1, param2 );
        }
        // Move plate to relative radius, theta position change values
        else if ( cmd.compare( PlateMoveList::CmdMoveRelative ) == 0 )
        {
            pPlateController->MoveTo( param1, param2 );
        }
        // Wait for a keypress.
        else if ( cmd.compare( PlateMoveList::CmdWaitForKeyPress ) == 0 )
        {
            std::cin.get();
        }
        // insert a delay into the command stream processing
        else if ( cmd.compare( PlateMoveList::CmdSleep ) == 0 )
        {
            Sleep( param1 );
        }
        // Stop processing the run list and exit the program.
        else if ( cmd.compare( PlateMoveList::CmdExit ) == 0 )
        {
            readList = false;
            break;
        }

        param1 = param2 = param3 = 0;

    } // End "while (true)"

    Logger::L().Log( MODULENAME, severity_level::debug1, boost::str( boost::format( "RunFile complete." ) ) );

    return true;
}

//*****************************************************************************
int main( int argc, char *argv[] )
{
    std::string listFile = "";

    boost::asio::io_service io_svc;
    std::shared_ptr<boost::asio::io_service::work> io_svc_work;

    boost::system::error_code ec;
    Logger::L().Initialize( ec, "PlateMoveTest.info" );

    Logger::L().Log( MODULENAME, severity_level::normal, "Starting PlateMoveTest" );

    io_svc_work.reset( new boost::asio::io_service::work( io_svc ) );

    if ( argc < 2 )
    {
        Logger::L().Log( MODULENAME, severity_level::normal, "No Move List RunFile specified." );
//        exit( 0 );
        listFile = "PlateMoveList.txt";
    }
    else
    {
        listFile = argv[1];
    }

    PlateMoveTest plateMoveTest;
    plateMoveTest.init();

    if ( !plateMoveTest.runMoveList( listFile ) )
    {
        Logger::L().Log( MODULENAME, severity_level::normal, "error processing Move List RunFile, exiting..." );
        Logger::L().Flush();
        exit( 0 );
    }

    Logger::L().Log( MODULENAME, severity_level::normal, "Before io_svc.run() in *main*" );

    Logger::L().Flush();

//    io_svc.run();

    return 0;	// Never get here normally.
}
