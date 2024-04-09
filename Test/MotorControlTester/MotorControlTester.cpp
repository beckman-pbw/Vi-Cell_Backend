// MotorControlTester.cpp : Defines the entry point for the console application.
//


#include <conio.h>
#include <stdio.h>
#include <io.h>  
#include <ios>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/format.hpp>

#include "stdafx.h"

#include "Logger.hpp"
#include "Configuration.hpp"
#include "MotorControlTester.hpp"


static const char MODULENAME[] = "MotorControlTester";



MotorControlTester::MotorControlTester( std::shared_ptr<boost::asio::io_context> ios, std::shared_ptr<ControllerBoardInterface> pcbi )
    : pLocalIosvc( ios )
    , pCbi_( pcbi )
    , probeMotor( ios, pcbi, MotorTypeProbe )
    , radiusMotor( ios, pcbi, MotorTypeRadius )
    , thetaMotor( ios, pcbi, MotorTypeTheta )
    , focusMotor( ios, pcbi, MotorTypeFocus )
    , reagentMotor( ios, pcbi, MotorTypeReagent )
    , rack1Motor( ios, pcbi, MotorTypeRack1 )
    , rack2Motor( ios, pcbi, MotorTypeRack2 )
    , configFile( "MotorControl.info" )
    , pcbiOk( false )
    , quitDone( false )
    , smValueLine( "" )
    , cbiPort( CNTLR_SN_A_STR )
    , smState( smTop )
    , smMOTORADDR( MotorTypeRegAddr::MotorTypeThetaBaseRegAddr + MotorRegisterAddrOffsets::ErrorCodeAddr )
    , smMOTORBASEADDR( MotorTypeRegAddr::MotorTypeThetaBaseRegAddr )
    , smMOTOROFFSETADDR( MotorRegisterAddrOffsets::ErrorCodeAddr )
    , smGLOBALADDR( RegisterIds::HostCommError )
    , smVALUE( 0 )
    , smLENGTH( 0 )
    , smMOTORID( MotorTypeId::MotorTypeIllegal )
    , smCMDPARAM( 0 )
    , smSTATUS( 0 )
    , rdValue( 0 )
    , motorCmd( 0 )
    , pMotorRegs_( NULL )
{
    pLocalWork_.reset( new boost::asio::io_context::work( *pLocalIosvc ) );

    ptfilecfg_.reset();

    // Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
    auto THREAD = std::bind( static_cast <std::size_t( boost::asio::io_context::* )( void )> ( &boost::asio::io_context::run ), pLocalIosvc.get() );
    pThread_.reset( new std::thread( THREAD ) );

    pSignalSet = new boost::asio::signal_set( *pLocalIosvc );
    pSignalSet->add( SIGINT );
    pSignalSet->add( SIGTERM );
    pSignalSet->add( SIGBREAK );
    pSignalSet->add( SIGABRT );
    pSignals_.reset( pSignalSet );
    pSignals_->async_wait( std::bind( &MotorControlTester::signal_handler, this, std::placeholders::_1, std::placeholders::_2 ) );

    pTimer.reset( new boost::asio::deadline_timer( *pLocalIosvc ) );
    pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
    pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
}

MotorControlTester::~MotorControlTester()
{
    Quit();

    pLocalWork_.reset();    // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
    pLocalIosvc->stop();    // instruct io_service to stop processing (should exit ::run() and end thread.
}

void MotorControlTester::Quit()
{
    if ( !quitDone )
    {
        if ( pcbiOk )
        {
            CloseAllMotors();       // orderly shutdown of all the motor objects
        }

        if ( pCbi_ )
        {
            pCbi_->CancelQueue();
            pCbi_->Close();
            pCbi_.reset();
        }

        if ( pTimer )
        {
            pTimer->cancel();       // stop the keyboard input timer
            pTimer.reset();         // stop the keyboard input timer
        }

        quitDone = true;
    }
}

void MotorControlTester::AppQuit()
{
    Quit();

    // All done listening.
    pSignals_->cancel();

    PostQuitMessage( 0 );

    if ( pLocalWork_ )
    {
        pLocalWork_.reset();        // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
        pLocalIosvc->stop();        // instruct io_service to stop processing (should exit ::run() and end thread.
    }

    exit( 0 );
}

bool MotorControlTester::Init()
{
    Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str( boost::format( "%s::init:<enter>" ) % MODULENAME ) );

//    pcbiOk = Cbi_.Initialize();
    pcbiOk = pCbi_->Initialize();

    return pcbiOk;
}

bool MotorControlTester::Start( std::string config_file )
{
    if ( pcbiOk )
    {
        if ( !OpenCbi( cbiPort ) )
        {
            return false;
        }
    }

    if ( config_file.length() > 0 )
    {
        configFile = config_file;
    }

    if ( pcbiOk )
    {
        InitAllConfig();
        GetAllStatus( true, false );
    }

    Prompt();

    return true;
}

bool MotorControlTester::OpenCbi( std::string port )
{
    bool isOpen = false;

    isOpen = pCbi_->IsSerialOpen();

    if ( !isOpen )
    {
        boost::system::error_code ec = pCbi_->OpenSerial( port );
        if ( ec )
        {
            Logger::L().Log ( MODULENAME, severity_level::normal, "Unable to open controller board " + port + ": " + ec.message() );
            std::cout << "Unable to open controller board: " << ec.message() << std::endl;
        }
        else
        {
            isOpen = true;
        }
    }
    return isOpen;

}

void MotorControlTester::InitInfoNode( std::string controllerNodeName, t_opPTree & motorparam_node )
{
	boost::system::error_code ec;

	if ( !ptfilecfg_ )
	{
		ptfilecfg_ = ConfigUtils::OpenConfigFile( configFile, ec, true );
	}

	if ( !ptfilecfg_ )
	{
		Logger::L().Log ( MODULENAME, severity_level::error, "Error opening configuration file \"" + configFile + "\"" );
	}
	else
	{
		if ( !ptconfig )
		{
			ptconfig = ptfilecfg_->get_child_optional( "config" );
		}

		if ( !ptconfig )
		{
			Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile + "\" - \"config\" section not found" );
		}
		else
		{
			t_opPTree cfg_node;

			if ( ( !controllers_node ) && ( !motorcfg_node ) )
			{
				controllers_node = ptconfig->get_child_optional( "motor_controllers" );           // look for the controllers section for individualized parameters...
			}

			if ( controllers_node )
			{
				cfg_node = controllers_node->get_child_optional( controllerNodeName );        // look for the specifc controller requested
			}
			else
			{
				motorcfg_node = ptconfig;
				cfg_node = motorcfg_node;
			}

			if ( !cfg_node )         // look for the motor parameters sub-node...
			{
				cfg_node = ptconfig;
			}

			if ( cfg_node )         // look for the motor parameters sub-node...
			{
				motorparam_node = cfg_node->get_child_optional( "motor_parameters" );
				if ( !motorparam_node )
				{
					motorparam_node = ptconfig;
				}
			}
		}
	}
}

void MotorControlTester::signal_handler( const boost::system::error_code& ec, int signal_number )
{
    if ( ec )
    {
        std::cout << std::endl << "Signal listener received error \"" << ec.message() << "\"" << std::endl;
    }

    // Try to get out of here.
    pLocalIosvc->post( std::bind( &MotorControlTester::AppQuit, this ) );
}

void MotorControlTester::CloseAllMotors( void )
{
    DoAllMotors( true, false );
}

void MotorControlTester::GetAllStatus( bool initStatus, bool show )
{
    DoAllMotors( false, false, InitTypes::InitStatus, show );
}

void MotorControlTester::InitAllConfig( void )
{
    DoAllMotors( false, false, InitTypes::InitAllParams, false );
}

void MotorControlTester::DoAllMotors( bool close, bool stop, InitTypes type, bool show )
{
    MotorTypeId idNext = MotorTypeUnknown;
    MotorTypeId idThis = MotorTypeUnknown;
    MotorBase * pMotor = NULL;
    MotorRegisters * pRegs = NULL;
    uint32_t regAddr = MotorTypeRegAddrUnknown;

    while ( idNext != MotorTypeIllegal )
    {
        switch ( idNext )
        {
            case MotorTypeId::MotorTypeUnknown:
            {
                pMotor = NULL;
                pRegs = NULL;
                idThis = idNext;
                idNext = MotorTypeRadius;
                break;
            }

            case MotorTypeId::MotorTypeRadius:
            {
                pMotor = &radiusMotor;
                pRegs = &radiusMotorRegs_;
                idThis = idNext;
                idNext = MotorTypeTheta;
                break;
            }

            case MotorTypeId::MotorTypeTheta:
            {
                pMotor = &thetaMotor;
                pRegs = &thetaMotorRegs_;
                idThis = idNext;
                idNext = MotorTypeProbe;
                break;
            }

            case MotorTypeId::MotorTypeProbe:
            {
                pMotor = &probeMotor;
                pRegs = &probeMotorRegs_;
                idThis = idNext;
                idNext = MotorTypeReagent;
                break;
            }

            case MotorTypeId::MotorTypeReagent:
            {
                pMotor = &reagentMotor;
                pRegs = &reagentMotorRegs_;
                idThis = idNext;
                idNext = MotorTypeFocus;
                break;
            }

            case MotorTypeId::MotorTypeFocus:
            {
                pMotor = &focusMotor;
                pRegs = &focusMotorRegs_;
                idThis = idNext;
                idNext = MotorTypeRack1;
                break;
            }

            case MotorTypeId::MotorTypeRack1:
            {
                pMotor = &rack1Motor;
                pRegs = &rack1MotorRegs_;
                idThis = idNext;
                idNext = MotorTypeRack2;
                break;
            }

            case MotorTypeId::MotorTypeRack2:
            {
                pMotor = &rack2Motor;
                pRegs = &rack2MotorRegs_;
                idThis = idNext;
                idNext = MotorTypeIllegal;
                break;
            }

            default:
            {
                pMotor = NULL;
                pRegs = NULL;
                idThis = MotorTypeUnknown;
                idNext = MotorTypeIllegal;
                break;
            }
        }

        if ( ( idThis != MotorTypeIllegal ) && ( idThis != MotorTypeUnknown ) && ( pMotor != NULL ) )
        {
            if ( close )
            {
                pMotor->Quit();
            }
            else
            {
                if ( ( type == InitTypes::InitTypeParamsLocal ) ||
                     ( type == InitTypes::InitAllParamsLocal ) ||
                     ( type == InitTypes::InitTypeParams ) ||
                     ( type == InitTypes::InitAllParams ) )
                {
                    InitTypes altType;
                    t_opPTree motor_cfg;
                    t_opPTree alt_cfg;

                    InitMotorParams( idThis, type, motor_cfg );
                    // now do the local registers also...
                    if ( ( type == InitTypes::InitTypeParamsLocal ) ||
                         ( type == InitTypes::InitAllParamsLocal ) )
                    {
                        if ( type == InitAllParamsLocal )
                        {
                            altType = InitAllParams;
                        }
                        else
                        {
                            altType = InitTypeParams;
                        }
                    }
                    else if ( ( type == InitTypes::InitTypeParams ) ||
                              ( type == InitTypes::InitAllParams ) )
                    {
                        if ( type == InitAllParams )
                        {
                            altType = InitAllParamsLocal;
                        }
                        else
                        {
                            altType = InitTypeParamsLocal;
                        }
                    }
                    InitMotorParams( idThis, altType, alt_cfg );

                    if ( ( type == InitTypes::InitTypeParams ) || ( type == InitTypes::InitAllParams ) )
                    {
                        // allow each motor controller to initialize its parameters from the external info file
                        pMotor->Init( cbiPort, ptfilecfg_, motor_cfg );
                    }
                }
                pMotor->ReadMotorRegs( *pRegs );    // request a read of the registers
                if ( show )                         // requesting to show status
                {
                    ShowMotorRegister( pMotor, pRegs );
                }
            }
        }
    }
}

void MotorControlTester::InitMotorParams( int32_t motorId, InitTypes type, t_opPTree & motorCfg )
{
    std::string controllerNodeName;

    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            controllerNodeName = "radius_controller";
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            controllerNodeName = "theta_controller";
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            controllerNodeName = "probe_controller";
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            controllerNodeName = "reagent_controller";
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            controllerNodeName = "focus_controller";
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            controllerNodeName = "rack1_controller";
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            controllerNodeName = "rack2_controller";
            break;
        }

        case MotorTypeId::MotorTypeUnknown:
        default:
        {
            controllerNodeName = "";
            break;
        }
    }

    if ( controllerNodeName.length() > 0 )
    {
        t_opPTree motor_cfg;

        // get the location of the parameter info in the external file
        InitInfoNode( controllerNodeName, motor_cfg );
        if ( ( type == InitTypes::InitTypeParamsLocal ) ||
            ( type == InitTypes::InitAllParamsLocal ) )
        {
            // initialize the motor controllers from the local config objects
            InitDefaultParams( motorId, motor_cfg );
        }
        else
        {
            motorCfg = motor_cfg;
        }
    }
}

void MotorControlTester::InitDefaultParams( int32_t motorId, t_opPTree configTree )
{
    MotorRegisters * pRegs = NULL;
    MotorCfgParams params;
    MotorCfgParams* pCfg = NULL;
    t_opPTree paramNode;
    std::string nodeName = "";
    boost::system::error_code ec;

    // first, initialize the internal default parameters to the lowest expected values...
    params.HomeDirection = 0;
    params.MotorFullStepsPerRev = 200;
    params.UnitsPerRev = 101600;
    params.GearheadRatio = 1;
    params.EncoderTicksPerRev = 1600;
    params.Deadband = 45;
    params.InvertedDirection = 0;
    params.StepSize = 8;
    params.commonRegs.Acceleration = 100000;
    params.commonRegs.Deceleration = 100000;
    params.MinSpeed = 0;
    params.commonRegs.MaxSpeed = 50000;
    params.OverCurrent = 1500000;
    params.StallCurrent = 0;
    params.HoldVoltageDivide = 60000;
    params.commonRegs.RunVoltageDivide = 1050000;
    params.commonRegs.AccVoltageDivide = 1050000;
    params.commonRegs.DecVoltageDivide = 1050000;
    params.CONFIG = 44696;
    params.INT_SPEED = 0;
    params.ST_SLP = 25;
    params.FN_SLP_ACC = 41;
    params.FN_SLP_DEC = 41;
    params.DelayAfterMove = 0;
    params.ProbeRegs.ProbeSpeed1 = 0;
    params.ProbeRegs.ProbeCurrent1 = 0;
    params.ProbeRegs.ProbeAbovePosition = 0;
    params.ProbeRegs.ProbeSpeed2 = 0;
    params.ProbeRegs.ProbeCurrent2 = 0;
    params.ProbeRegs.ProbeRaise = 0;
    params.ProbeStopPosition = 0;
    params.MaxTravelPosition = 140000;
    params.MaxMoveRetries = 0;

    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            pRegs = &radiusMotorRegs_;
            pCfg = &dfltRadiusParams_;
            nodeName = "radius_motor";
            params.UnitsPerRev = 320000;           // 32 mm /revolution based on 16 tooth gear and 2mm pitch
            params.EncoderTicksPerRev = -800;
//            params.EncoderTicksPerRev = 800;
            params.Deadband = 1200;
            params.commonRegs.Acceleration = 1500000;
            params.commonRegs.Deceleration = 1500000;
            params.commonRegs.MaxSpeed = 350000;
            params.OverCurrent = 5000000;
            params.MaxTravelPosition = RadiusMaxTravel;
            params.MaxMoveRetries = 3;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            pRegs = &thetaMotorRegs_;
            pCfg = &dfltThetaParams_;
            nodeName = "theta_motor";
            params.HomeDirection = 1;
            params.UnitsPerRev = 900;               // with gearing reduction, 90 degrees per rev
            params.GearheadRatio = 4;
            params.EncoderTicksPerRev = 800;
            params.Deadband = 5;
            params.InvertedDirection = 1;
            params.commonRegs.Acceleration = 1200;
            params.commonRegs.Deceleration = 1200;
            params.commonRegs.MaxSpeed = 150;
            params.MaxTravelPosition = 0;
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            pRegs = &probeMotorRegs_;
            pCfg = &dfltProbeParams_;
            nodeName = "probe_motor";
            params.EncoderTicksPerRev = -1440;
//            params.EncoderTicksPerRev = 1440;
            params.Deadband = 285;
            params.commonRegs.Acceleration = 9000000;
            params.commonRegs.Deceleration = 9000000;
            params.commonRegs.MaxSpeed = 1000000;
            params.OverCurrent = 5000000;
            params.commonRegs.RunVoltageDivide = 1000000;
            params.commonRegs.AccVoltageDivide = 2500000;
            params.commonRegs.DecVoltageDivide = 2500000;
            params.ProbeRegs.ProbeSpeed1 = 1000000;
            params.ProbeRegs.ProbeCurrent1 = 1000000;
            params.ProbeRegs.ProbeAbovePosition = 385000;
            params.ProbeRegs.ProbeSpeed2 = 20000;
            params.ProbeRegs.ProbeCurrent2 = 500000;
            params.ProbeRegs.ProbeRaise = 10000;
            params.ProbeStopPosition = 0;
            params.MaxTravelPosition = ProbeMaxTravel;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            pRegs = &reagentMotorRegs_;
            pCfg = &dfltReagentParams_;
            nodeName = "reagent_motor";
            params.UnitsPerRev = 320000;           // 32 mm /revolution based on 16 tooth gear and 2mm pitch
            params.GearheadRatio = 1;
            params.Deadband = 1000;
            params.InvertedDirection = 1;
            params.commonRegs.Acceleration = 640000;
            params.commonRegs.Deceleration = 640000;
            params.commonRegs.MaxSpeed = 320000;
            params.MaxTravelPosition = ReagentArmMaxTravel;
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            pRegs = &focusMotorRegs_;
            pCfg = &dfltFocusParams_;
            nodeName = "focus_motor";
            params.UnitsPerRev = 3000;             // 0.3 mm /rev from spec sheet
            params.Deadband = 4;
            params.commonRegs.Acceleration = 7500;
            params.commonRegs.Deceleration = 7500;
            params.commonRegs.MaxSpeed = 2500;
            params.OverCurrent = 750000;
            params.commonRegs.RunVoltageDivide = 500000;
            params.commonRegs.AccVoltageDivide = 500000;
            params.commonRegs.DecVoltageDivide = 500000;
            params.MaxTravelPosition = FocusMaxTravel;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            pRegs = &rack1MotorRegs_;
            pCfg = &dfltRack1Params_;
            nodeName = "rack1_motor";
            params.Deadband = 127;
            params.commonRegs.Acceleration = 8000000;
            params.commonRegs.Deceleration = 8000000;
            params.commonRegs.MaxSpeed = 1500000;
            params.OverCurrent = 5000000;
            params.commonRegs.RunVoltageDivide = 1500000;
            params.commonRegs.AccVoltageDivide = 2500000;
            params.commonRegs.DecVoltageDivide = 2500000;
            params.MaxTravelPosition = MaxLEDRackTravel;
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            pRegs = &rack2MotorRegs_;
            pCfg = &dfltRack2Params_;
            nodeName = "rack2_motor";
            params.Deadband = 127;
            params.commonRegs.Acceleration = 8000000;
            params.commonRegs.Deceleration = 8000000;
            params.commonRegs.MaxSpeed = 1500000;
            params.OverCurrent = 5000000;
            params.commonRegs.RunVoltageDivide = 1500000;
            params.commonRegs.AccVoltageDivide = 2500000;
            params.commonRegs.DecVoltageDivide = 2500000;
            params.MaxTravelPosition = MaxLEDRackTravel;
            break;
        }

        case MotorTypeId::MotorTypeUnknown:
        default:
        {
            pRegs = NULL;
            pCfg = NULL;
            nodeName = "";
            break;
        }
    }

    if ( ( motorId != MotorTypeIllegal ) && ( motorId != MotorTypeUnknown ) && ( pCfg != NULL ) )
    {
        memcpy( pCfg, &params, sizeof( MotorCfgParams ) );     // copy to default parameters object for this motor

        if ( ( nodeName.length() > 0 ) && ( configTree ) )
        {
            // now get the params from the external info file...
            paramNode = configTree->get_child_optional( nodeName );
            if ( !paramNode )
            {
                Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file - \"" + nodeName + "\" section not found" );
            }
            else
            {
                ReadMotorParamsInfo( pCfg, paramNode );       // update the defaults based on the info file
            }
        }

        GetMotorRegsForId( motorId, pRegs );
        if ( pRegs != NULL )
        {   // update the content of the read-only and non-parameter registers....
            pRegs->Command = 0;
            pRegs->CommandParam = 0;
            pRegs->ErrorCode = 0;
            pRegs->Position = 0;
            pRegs->ProbeStopPosition = 0;
            UpdateRegParams( pRegs, pCfg );                   // copy to the working register set...
            UpdateMotorDefaultParams( pCfg, motorId );        // send the default params to the motor controller...
        }
    }
}

void MotorControlTester::ReadMotorParamsInfo( MotorCfgParams * pMotorCfg, t_opPTree paramNode )
{
    boost::property_tree::ptree & params = paramNode.get();

    pMotorCfg->HomeDirection = params.get<uint32_t>( "HomeDirection", pMotorCfg->HomeDirection );
    pMotorCfg->MotorFullStepsPerRev = params.get<uint32_t>( "MotorFullStepsPerRev", pMotorCfg->MotorFullStepsPerRev );
    pMotorCfg->UnitsPerRev = params.get<uint32_t>( "UnitsPerRev", pMotorCfg->UnitsPerRev );
    pMotorCfg->GearheadRatio = params.get<uint32_t>( "GearheadRatio", pMotorCfg->GearheadRatio );
    pMotorCfg->EncoderTicksPerRev = params.get<uint32_t>( "EncoderTicksPerRev", pMotorCfg->EncoderTicksPerRev );
    pMotorCfg->Deadband = params.get<uint32_t>( "Deadband", pMotorCfg->Deadband );
    pMotorCfg->InvertedDirection = params.get<uint32_t>( "InvertedDirection", pMotorCfg->InvertedDirection );
    pMotorCfg->StepSize = params.get<uint32_t>( "StepSize", pMotorCfg->StepSize );
    pMotorCfg->commonRegs.Acceleration = params.get<uint32_t>( "Acceleration", pMotorCfg->commonRegs.Acceleration );
    pMotorCfg->commonRegs.Deceleration = params.get<uint32_t>( "DeAcceleration", pMotorCfg->commonRegs.Deceleration );
    pMotorCfg->MinSpeed = params.get<uint32_t>( "MinSpeed", pMotorCfg->MinSpeed );
    pMotorCfg->commonRegs.MaxSpeed = params.get<uint32_t>( "MaxSpeed", pMotorCfg->commonRegs.MaxSpeed );
    pMotorCfg->OverCurrent = params.get<uint32_t>( "OverCurrent", pMotorCfg->OverCurrent );
    pMotorCfg->StallCurrent = params.get<uint32_t>( "StallCurrent", pMotorCfg->StallCurrent );
    pMotorCfg->HoldVoltageDivide = params.get<uint32_t>( "HoldVoltageDivide", pMotorCfg->HoldVoltageDivide );
    pMotorCfg->commonRegs.RunVoltageDivide = params.get<uint32_t>( "RunVoltageDivide", pMotorCfg->commonRegs.RunVoltageDivide );
    pMotorCfg->commonRegs.AccVoltageDivide = params.get<uint32_t>( "AccVoltageDivide", pMotorCfg->commonRegs.AccVoltageDivide );
    pMotorCfg->commonRegs.DecVoltageDivide = params.get<uint32_t>( "DecVoltageDivide", pMotorCfg->commonRegs.DecVoltageDivide );
    pMotorCfg->CONFIG = params.get<uint32_t>( "CONFIG", pMotorCfg->CONFIG );
    pMotorCfg->INT_SPEED = params.get<uint32_t>( "INT_SPEED", pMotorCfg->INT_SPEED );
    pMotorCfg->ST_SLP = params.get<uint32_t>( "ST_SLP", pMotorCfg->ST_SLP );
    pMotorCfg->FN_SLP_ACC = params.get<uint32_t>( "FN_SLP_ACC", pMotorCfg->FN_SLP_ACC );
    pMotorCfg->FN_SLP_DEC = params.get<uint32_t>( "FN_SLP_DEC", pMotorCfg->FN_SLP_DEC );
    pMotorCfg->DelayAfterMove = params.get<uint32_t>( "DelayAfterMove", pMotorCfg->DelayAfterMove );
    pMotorCfg->ProbeRegs.ProbeSpeed1 = params.get<uint32_t>( "ProbeSpeed1", pMotorCfg->ProbeRegs.ProbeSpeed1 );
    pMotorCfg->ProbeRegs.ProbeCurrent1 = params.get<uint32_t>( "ProbeCurrent1", pMotorCfg->ProbeRegs.ProbeCurrent1 );
    pMotorCfg->ProbeRegs.ProbeAbovePosition = params.get<uint32_t>( "ProbeAbovePosition", pMotorCfg->ProbeRegs.ProbeAbovePosition );
    pMotorCfg->ProbeRegs.ProbeSpeed2 = params.get<uint32_t>( "ProbeSpeed2", pMotorCfg->ProbeRegs.ProbeSpeed2 );
    pMotorCfg->ProbeRegs.ProbeCurrent2 = params.get<uint32_t>( "ProbeCurrent2", pMotorCfg->ProbeRegs.ProbeCurrent2 );
    pMotorCfg->ProbeRegs.ProbeRaise = params.get<uint32_t>( "ProbeRaise", pMotorCfg->ProbeRegs.ProbeRaise );
    pMotorCfg->MaxTravelPosition = params.get<uint32_t>( "MaxTravelPosition", pMotorCfg->MaxTravelPosition );
    pMotorCfg->MaxMoveRetries = params.get<uint32_t>( "MaxMoveRetries", pMotorCfg->MaxMoveRetries );
}

void MotorControlTester::UpdateRegParams( MotorRegisters * pRegs, MotorCfgParams * pMotorCfg )
{
    void * regAddr;

    regAddr = (void *)&pRegs->HomeDirection;
    memcpy( regAddr, pMotorCfg, sizeof( MotorCfgParams ) );                         // copy to register set object for this motor
}

void MotorControlTester::UpdateParamsFromRegs( MotorCfgParams * pCfg, int32_t motorId )
{
    MotorRegisters * pRegs = NULL;
    void * regAddr;

    regAddr = (void *)&pRegs->HomeDirection;
    memcpy( pCfg, regAddr, sizeof( MotorCfgParams ) );                         // copy to register set object for this motor
}

void MotorControlTester::UpdateMotorDefaultParams( MotorCfgParams * pCfg, int32_t motorId )
{
    MotorBase * pMotor = NULL;

    GetMotorObjForId( motorId, pMotor );
    if ( pMotor )
    {
        pMotor->UpdateDefaultParams( pCfg );      // update the defaults...
    }
}

void MotorControlTester::WriteMotorParams( MotorRegisters * pRegs, int32_t motorId )
{
    MotorBase * pMotor = NULL;

    GetMotorObjForId( motorId, pMotor );
    if ( pMotor )
    {
        pMotor->SetHomeDirection( pRegs->HomeDirection );
        pMotor->SetStepsPerRev( pRegs->MotorFullStepsPerRev );
        pMotor->SetUnitsPerRev( pRegs->UnitsPerRev );
        pMotor->SetGearRatio( pRegs->GearheadRatio );
        pMotor->SetTicksPerRev( pRegs->EncoderTicksPerRev );
        pMotor->SetDeadband( pRegs->Deadband );
        pMotor->SetInvertedDirection( pRegs->InvertedDirection );
        pMotor->SetStepSize( pRegs->StepSize );
        pMotor->SetAccel( pRegs->commonRegs.Acceleration );
        pMotor->SetDecel( pRegs->commonRegs.Deceleration );
        pMotor->SetMinSpeed( pRegs->MinSpeed );
        pMotor->SetMaxSpeed( pRegs->commonRegs.MaxSpeed );
        pMotor->SetOverCurrent( pRegs->OverCurrent );
        pMotor->SetStallCurrent( pRegs->StallCurrent );
        pMotor->SetHoldVoltDivide( pRegs->HoldVoltageDivide );
        pMotor->SetRunVoltDivide( pRegs->commonRegs.RunVoltageDivide );
        pMotor->SetAccelVoltDivide( pRegs->commonRegs.AccVoltageDivide );
        pMotor->SetDecelVoltDivide( pRegs->commonRegs.DecVoltageDivide );
        pMotor->SetConfig( pRegs->CONFIG );
        pMotor->SetIntSpd( pRegs->INT_SPEED );
        pMotor->SetStSlp( pRegs->ST_SLP );
        pMotor->SetFnSlpAccel( pRegs->FN_SLP_ACC );
        pMotor->SetFnSlpDecel( pRegs->FN_SLP_DEC );
        pMotor->SetDelayAfterMove( pRegs->DelayAfterMove );
        pMotor->AdjustProbeSpeed1( pRegs->ProbeRegs.ProbeSpeed1 );
        pMotor->AdjustProbeSpeed1Current( pRegs->ProbeRegs.ProbeCurrent1 );
        pMotor->AdjustProbeAbovePosition( pRegs->ProbeRegs.ProbeAbovePosition );
        pMotor->AdjustProbeSpeed2( pRegs->ProbeRegs.ProbeSpeed2 );
        pMotor->AdjustProbeSpeed2Current( pRegs->ProbeRegs.ProbeCurrent2 );
        pMotor->AdjustProbeRaise( pRegs->ProbeRegs.ProbeRaise );
        pMotor->AdjustMaxTravel( pRegs->MaxTravelPosition );
        pMotor->AdjustMaxRetries( pRegs->MaxMoveRetries );
        pMotor->ApplyAllParams();
    }
}

void MotorControlTester::ShowMotorRegister( MotorBase * pMotor, MotorRegisters * pRegs )
{
    std::string motorStr = "";
    uint32_t motorType = 0;
    uint32_t boardStatus = (uint32_t)pCbi_->GetBoardStatus().get();

    motorType = pMotor->GetMotorType();
//    pMotor->GetMotorTypeAsString((MotorTypeId)motorType, motorStr);
    motorStr = pMotor->GetMotorTypeAsString( (MotorTypeId)motorType );

    std::cout << std::endl << "Motor Register Content:  ";
    std::cout << boost::str( boost::format( " Id: %u" ) % motorType );
    std::cout << " - " << motorStr << std::endl;
    std::cout << boost::str( boost::format( "\tCommand: %u" ) % pRegs->Command ) << std::endl;
    std::cout << boost::str( boost::format( "\tCommandParam: %u" ) % pRegs->CommandParam ) << std::endl;
    std::cout << boost::str( boost::format( "\tErrorCode: 0x%08X" ) % pRegs->ErrorCode ) << std::endl;
    std::cout << boost::str( boost::format( "\tPosition: %d" ) % pRegs->Position ) << std::endl;
    std::cout << boost::str( boost::format( "\tHomeDirection: %u" ) % pRegs->HomeDirection ) << std::endl;
    std::cout << boost::str( boost::format( "\tMotorFullStepsPerRev: %u" ) % pRegs->MotorFullStepsPerRev ) << std::endl;
    std::cout << boost::str( boost::format( "\tUnitsPerRev: %u" ) % pRegs->UnitsPerRev ) << std::endl;
    std::cout << boost::str( boost::format( "\tGearheadRatio: %u" ) % pRegs->GearheadRatio ) << std::endl;
    std::cout << boost::str( boost::format( "\tEncoderTicksPerRev: %d" ) % pRegs->EncoderTicksPerRev ) << std::endl;
    std::cout << boost::str( boost::format( "\tDeadband: %u" ) % pRegs->Deadband ) << std::endl;
    std::cout << boost::str( boost::format( "\tInvertedDirection: %u" ) % pRegs->InvertedDirection ) << std::endl;
    std::cout << boost::str( boost::format( "\tStepSize: %u" ) % pRegs->StepSize ) << std::endl;;
    std::cout << boost::str( boost::format( "\tAcceleration: %u" ) % pRegs->commonRegs.Acceleration ) << std::endl;;
    std::cout << boost::str( boost::format( "\tDeAcceleration: %u" ) % pRegs->commonRegs.Deceleration ) << std::endl;
    std::cout << boost::str( boost::format( "\tMinSpeed: %u" ) % pRegs->MinSpeed ) << std::endl;
    std::cout << boost::str( boost::format( "\tMaxSpeed: %u" ) % pRegs->commonRegs.MaxSpeed ) << std::endl;
    std::cout << boost::str( boost::format( "\tOverCurrent: %u" ) % pRegs->OverCurrent ) << std::endl;
    std::cout << boost::str( boost::format( "\tStallCurrent: %u" ) % pRegs->StallCurrent ) << std::endl;
    std::cout << boost::str( boost::format( "\tHoldVoltageDivide: %u" ) % pRegs->HoldVoltageDivide ) << std::endl;
    std::cout << boost::str( boost::format( "\tRunVoltageDivide: %u" ) % pRegs->commonRegs.RunVoltageDivide ) << std::endl;
    std::cout << boost::str( boost::format( "\tAccVoltageDivide: %u" ) % pRegs->commonRegs.AccVoltageDivide ) << std::endl;
    std::cout << boost::str( boost::format( "\tDecVoltageDivide: %u" ) % pRegs->commonRegs.DecVoltageDivide ) << std::endl;
    std::cout << boost::str( boost::format( "\tCONFIG: %u" ) % pRegs->CONFIG ) << std::endl;
    std::cout << boost::str( boost::format( "\tINT_SPEED: %u" ) % pRegs->INT_SPEED ) << std::endl;
    std::cout << boost::str( boost::format( "\tST_SLP: %u" ) % pRegs->ST_SLP ) << std::endl;
    std::cout << boost::str( boost::format( "\tFN_SLP_ACC: %u" ) % pRegs->FN_SLP_ACC ) << std::endl;
    std::cout << boost::str( boost::format( "\tFN_SLP_DEC: %u" ) % pRegs->FN_SLP_DEC ) << std::endl;
    std::cout << boost::str( boost::format( "\tDelayAfterMove: %u" ) % pRegs->DelayAfterMove ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeSpeed1: %u" ) % pRegs->ProbeRegs.ProbeSpeed1 ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeCurrent1: %u" ) % pRegs->ProbeRegs.ProbeCurrent1 ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeAbovePosition: %u" ) % pRegs->ProbeRegs.ProbeAbovePosition ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeSpeed2: %u" ) % pRegs->ProbeRegs.ProbeSpeed2 ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeCurrent2: %u" ) % pRegs->ProbeRegs.ProbeCurrent2 ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeRaise: %u" ) % pRegs->ProbeRegs.ProbeRaise ) << std::endl;
    std::cout << boost::str( boost::format( "\tProbeStopPosition: %u" ) % pRegs->ProbeStopPosition ) << std::endl;
    std::cout << boost::str( boost::format( "\tMaxTravelPosition: %u" ) % pRegs->MaxTravelPosition ) << std::endl;
    std::cout << boost::str( boost::format( "\tMaxMoveRetries: %u" ) % pRegs->MaxMoveRetries ) << std::endl;
    std::cout << boost::str( boost::format( "\t(BoardStatus: %u)" ) % boardStatus ) << std::endl;

}

void MotorControlTester::Prompt()
{
    std::cout << std::endl;
    switch ( smState )
    {
        case smTop:
            std::cout << "R/I/M/S/A/P/W/U/O/V/C/E/Q/?: ";
            break;
        case smRdReg_MOTORADDREntry:
            std::cout << "Read Motor Register Address (" << smMOTORBASEADDR << ":" << smMOTOROFFSETADDR << "): ";
            break;
        case smRdReg_GLOBALADDREntry:
            std::cout << "Read Board Global Register Address (" << smGLOBALADDR << "): ";
            break;
        case smRdReg_LENEntry:
            std::cout << "Length (bytes) (" << smLENGTH << "): ";
            break;
        case smRdReg_MOTORIDEntry:
            std::cout << "Motor Id ((" << smMOTORID << "): ";
            std::cout << "Motor Id (Pr:1 R:2 Th:3 Focus:4 Reag:5 Rack1:6 Rack2:7 Obj:8] (" << smMOTORID << "): ";
            break;
        case smRdReg_MOTORStatus:
        case smRdReg_ALLStatus:
        case smWrReg_MOTORInit:
            break;
        case smWrReg_MOTORADDREntry:
        case smWrReg_PARAMADDREntry:
            std::cout << "Write Motor Register Address (" << smMOTORBASEADDR << ":" << smMOTOROFFSETADDR << "): ";
            break;
        case smWrReg_GLOBALADDREntry:
            std::cout << "Write Board Global Register Address (" << smGLOBALADDR << "): ";
            break;
        case smWrReg_VALEntry:          // Assume 32-bt uint for now.
            std::cout << "Write Value (" << smVALUE << "): ";
            break;
        case smWrReg_CMDEntry:
            std::cout << "Command Id: ";
            break;
        case smWrReg_POSEntry:          // Assume 32-bt uint for now.
            std::cout << "Command Parameter: Position Value (" << smCMDPARAM << "): ";
            break;
        case smWrReg_STOPEntry:         // Assume 32-bt uint for now.
            std::cout << "Command Parameter: Stop-type : 0 = soft, 1 = hard (" << smCMDPARAM << "): ";
            break;
        case smWrReg_ENABLEEntry:         // Assume 32-bt uint for now.
            std::cout << "Command Parameter: Holding Current Enable : 0 = off, 1 = on (" << smCMDPARAM << "): ";
            break;
        case smWrReg_ERRORClear:
            break;
    }
}

void MotorControlTester::ShowHelp( void )
{
    std::cout << std::endl << "USAGE:\tR | I | M | S | A | P | W | U | O | V | C | E | Q | ?" << std::endl;
    std::cout << "\tR : Read Motor Control register Address for the selected motor" << std::endl;
    std::cout << "\t  :\tRegister offset address entry is automatically prompted" << std::endl;
    std::cout << "\t  :\tEntry value is Register offset address within the currently selected motor" << std::endl;
    std::cout << "\tI : Read controller absolute register Address" << std::endl;
    std::cout << "\t  :\tRegister absolute address entry is automatically prompted" << std::endl;
    std::cout << "\tM : Select motor by Id number " << std::endl;
    std::cout << "\t  :\tMotor Id entry is automatically prompted " << std::endl;
    std::cout << "\t\t1 : Probe" << std::endl;
    std::cout << "\t\t2 : Radius" << std::endl;
    std::cout << "\t\t3 : Theta" << std::endl;
    std::cout << "\t\t4 : Focus" << std::endl;
    std::cout << "\t\t5 : Reagent" << std::endl;
    std::cout << "\t\t6 : Rack1" << std::endl;
    std::cout << "\t\t7 : Rack2" << std::endl;
    std::cout << "\tS : Show status for selected motor" << std::endl;
    std::cout << "\tA : trigger controller read and show status for all motors" << std::endl;
    std::cout << "\tP : Initialize all motor parameters for the selected motor" << std::endl;
    std::cout << "\tW : Write Motor Control register Address for the selected motor" << std::endl;
    std::cout << "\t  :\tRegister offset address entry is automatically prompted" << std::endl;
    std::cout << "\t  :\tEntry value is Register offset address within the currently selected motor" << std::endl;
    std::cout << "\tU : Update default Motor Control register Address for the selected motor" << std::endl;
    std::cout << "\t  :\tRegister offset address entry is automatically prompted" << std::endl;
    std::cout << "\t  :\tEntry value is Register offset address within the currently selected motor" << std::endl;
    std::cout << "\tO : Write to controller absolute register Address" << std::endl;
    std::cout << "\t  :\tRegister absolute address entry is automatically prompted" << std::endl;
    std::cout << "\tV : Enter value for write actions" << std::endl;
    std::cout << "\t  :\tParameter value entry is automatically prompted" << std::endl;
    std::cout << "\tC : Execute Motor command:" << std::endl;
    std::cout << "\t\t1 : Find and mark home position" << std::endl;
    std::cout << "\t\t2 : Mark current position as home position" << std::endl;
    std::cout << "\t\t3 : Go to absolute position" << std::endl;
    std::cout << "\t\t  : \tPosition value entry is automatically prompted" << std::endl;
    std::cout << "\t\t4 : Clear error codes (deprecated in Rev 2 hardware)" << std::endl;
    std::cout << "\t\t5 : Reset controller using the current or supplied register values" << std::endl;
    std::cout << "\t\t6 : Move to relative position" << std::endl;
    std::cout << "\t\t  : \tPosition value entry is automatically prompted" << std::endl;
    std::cout << "\t\t7 : Stop; 0 = soft, 1 = hard" << std::endl;
    std::cout << "\t\t  : \tParameter value entry is automatically prompted" << std::endl;
    std::cout << "\t\t8 : Holding Current Enable; 0 = off, 1 = on" << std::endl;
    std::cout << "\t\t  : \tParameter value entry is automatically prompted" << std::endl;
    std::cout << "\t\t9 : Probe Down:\texecute the controller probe down logic to detect the" << std::endl;
    std::cout << "\t\t\t\ttube or well bottom and position the probe above it" << std::endl;
    std::cout << "\t\t10: Move to Limit Switch:\tMove the motor to the lower limit switch;" << std::endl;
    std::cout << "\t\t\t\t\tcurrently only used by the reagent arm" << std::endl;
    std::cout << "\tE : Clear all board and motor errors" << std::endl;
    std::cout << "\tQ : Exit the program" << std::endl;
    std::cout << "\t? : Display this help screen" << std::endl;
    std::cout << std::endl << std::endl;
    std::cout << "For all commands using parameters, the current value of the parameter is shown in parentheses in the entry prompt." << std::endl;
    std::cout << "If the current parameter value is acceptable, the 'Return' key may be pressed to accept the current value and bypass re-entry." << std::endl;
    std::cout << "The escape key may be pressed at any to cancel an action or entry sequence (or re-display the action key legend prompt)." << std::endl;
    std::cout << std::endl << std::endl;
}

void MotorControlTester::HandleInput( const boost::system::error_code& error )
{
    if ( error )
    {
        if ( error == boost::asio::error::operation_aborted )
        {
            Logger::L().Log ( MODULENAME, severity_level::debug2, "canceling Status Update timer" );
        }
        return;
    }

    if ( !_kbhit() )
    {
        switch ( smState )
        {
            case smRdReg_MOTORStatus:       // allow processing for status display commands
            case smRdReg_ALLStatus:         // allow processing for status display commands
            case smWrReg_MOTORInit:         // allow processing for the motor initialize command
            case smWrReg_CMDExecute:        // allow execution of motor commands
            case smWrReg_ERRORClear:
            {
                break;
            }
            default:
                pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
                pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
                return;
        }
    }

    int C = 0;
    switch ( smState )
    {
        case smRdReg_MOTORStatus:       // allow processing for status display commands
        case smRdReg_ALLStatus:         // allow processing for status display commands
        case smWrReg_MOTORInit:         // allow processing for the motor initialize command
        case smWrReg_CMDExecute:        // allow execution of motor commands
        case smWrReg_ERRORClear:
        {
            break;
        }
        default:
        {
            C = _getch();
            if ( isprint( C ) )
            {
                std::cout << (char)C;
                // First: process QUIT characters
                if ( C == 'q' || C == 'Q' )
                {
                    HWND mHwnd = NULL;
                    mHwnd = GetConsoleWindow();
                    std::cout << std::endl << "Detected quit request in input" << std::endl;
                    PostMessage( mHwnd, WM_CLOSE, 0, 0 );
                    return;
                }
                else if ( C == '?' )
                {
                    ShowHelp();
                    smState = smTop;
                    smValueLine.clear();
                    pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
                    pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
                    return;
                }
            }
            else
            {
                if ( ( C != ESC ) && ( C != '\r' ) && ( C != '\n' ) )
                {
                    if ( C == '\b' )
                    {
                        if ( smValueLine.length() > 0 )
                        {
                            std::cout << '\b';
                            std::cout << ( char )' ';
                            std::cout << '\b';
                            smValueLine.pop_back();
                        }
                    }
                    pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
                    pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
                    return;
                }
            }

            /*
            * Process multi-character entries
            */
            if ( smState != smTop )
            {
                // Append character to string and wait for next
                // ...unless this is the LF character which lets us continue
                if ( ( C != '\n' ) && ( C != '\r' ) && ( C != '\b' ) && ( C != ESC ) )
                {
                    smValueLine += (char)C;
                    pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
                    pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
                    return;
                }
            }
            break;
        }
    }

    if ( C == ESC )
    {
        smState = smTop;
    }

    int status = 0;
    MotorBase * pMotor = NULL;
    uint32_t * pAddr = 0;

    switch ( smState )
    {
        case smTop:
        {
            HandleTopState( C );
            break;
        }
        case smRdReg_MOTORADDREntry:
        {
            if ( !ReadMotorOffsetAddress( smValueLine ) )
            {
                break;
            }
            smState = smRdReg_LENEntry;
            prevState = smRdReg_MOTORADDREntry;
            break;
        }
        case smRdReg_GLOBALADDREntry:
        {
            if ( !ReadGlobalAddress( smValueLine ) )
            {
                break;
            }
            smState = smRdReg_LENEntry;
            prevState = smRdReg_GLOBALADDREntry;
            break;
        }
        case smRdReg_LENEntry:
        {
            if ( !smValueLine.empty() )
            {
                status = sscanf_s( smValueLine.c_str(), "%d", &smLENGTH );
                if ( status != 1 )
                {
                    std::cout << std::endl << "Not a legal value: " << smValueLine << std::endl;
                    smLENGTH = 0;
                    smState = smTop;
                    break;
                }
            }

            if ( smLENGTH > 0 )
            {
                pAddr = 0;
                if ( ( prevState == smRdReg_GLOBALADDREntry ) &&
                    ( smGLOBALADDR >= 0 ) )
                {
                    pAddr = (uint32_t*)&smGLOBALADDR;
                }
                else if ( ( prevState == smRdReg_MOTORADDREntry ) &&
                    ( smMOTORADDR > 0 ) )
                {
                    pAddr = (uint32_t*)&smMOTORADDR;
                }

                if ( pAddr )
                {
//                    Cbi_.ReadRegister( *pAddr, smLENGTH, ControllerBoardInterface::t_ptxrxbuf(),
                    pCbi_->ReadRegister( *pAddr, smLENGTH, ControllerBoardInterface::t_ptxrxbuf(),
                                         std::bind( &MotorControlTester::RdCallback, this,
                                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4 ) );
                }
            }
            else
            {
                std::cout << std::endl << "No length specified!" << std::endl;
            }
            smState = smTop;
            break;
        }
        case smRdReg_MOTORIDEntry:
        {
            int32_t motorId = 0;
            if ( !smValueLine.empty() )
            {
                status = sscanf_s( smValueLine.c_str(), "%d", &motorId );
                if ( status != 1 )
                {
                    std::cout << std::endl << "Not a legal value: " << smValueLine << std::endl;
                }
                else
                {
                    GetMotorObjForId( motorId, pMotor );
                    if ( pMotor )
                    {
                        smMOTORID = motorId;
                        smMOTORBASEADDR = GetMotorRegsAddrForId( motorId );
                        smState = smRdReg_MOTORStatus;
                    }
                    else
                    {
                        std::cout << std::endl << "Not a legal motor Id: " << smValueLine << std::endl;
                        smMOTORID = 0;
                        smMOTORBASEADDR = 0;
                    }
                }
            }
            else
            {
                smState = smTop;
            }
            break;
        }
        case smRdReg_MOTORStatus:
        {
            if ( smMOTORID > 0 )
            {
                GetMotorObjForId( smMOTORID, pMotor );
                pMotorRegs_ = &motorRegs_;      // use the disposable register read buffer
                if ( ( pMotor ) && ( pMotorRegs_ ) )
                {
                    pMotor->GetMotorRegs( *pMotorRegs_ );
                    ShowMotorRegister( pMotor, pMotorRegs_ );
                }
                else
                {
                    std::cout << std::endl << "Not a legal motor Id: " << smValueLine << std::endl;
                }
                pMotorRegs_ = NULL;
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            smState = smTop;
            break;
        }
        case smRdReg_ALLStatus:
        {
            GetAllStatus( false, true );      // refresh all the motor-specific register buffers
            smState = smTop;
            break;
        }
        case smWrReg_MOTORInit:             // allow processing for the motor initialize command
        {
            if ( smMOTORID > 0 )
            {
                GetMotorObjForId( smMOTORID, pMotor );
                pMotorRegs_ = &motorRegs_;      // use the disposable register read buffer
                if ( ( pMotor ) && ( pMotorRegs_ ) )
                {
                    pMotor->GetMotorRegs( *pMotorRegs_ );
                    ShowMotorRegister( pMotor, pMotorRegs_ );
                }
                else
                {
                    std::cout << std::endl << "Not a legal motor Id: " << smValueLine << std::endl;
                }
                pMotorRegs_ = NULL;
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            smState = smTop;
            break;
        }
        case smWrReg_MOTORADDREntry:
        {
            if ( !ReadMotorOffsetAddress( smValueLine ) )
            {
                break;
            }
            smState = smWrReg_VALEntry;
            prevState = smWrReg_MOTORADDREntry;
            break;
        }
        case smWrReg_GLOBALADDREntry:
        {
            if ( !ReadGlobalAddress( smValueLine ) )
            {
                break;
            }
            smState = smWrReg_VALEntry;
            prevState = smWrReg_GLOBALADDREntry;
            break;
        }
        case smWrReg_PARAMADDREntry:
        {
            if ( !ReadMotorOffsetAddress( smValueLine ) )
            {
                break;
            }
            smState = smWrReg_VALEntry;
            prevState = smWrReg_PARAMADDREntry;
            break;
        }
        case smWrReg_VALEntry:
        {
            if ( !smValueLine.empty() )
            {
                status = sscanf_s( smValueLine.c_str(), "%d", &smVALUE );
                if ( status != 1 )
                {
                    std::cout << std::endl << "Not a legal value: " << smValueLine << std::endl;
                    smState = smTop;
                    break;
                }
            }

            pAddr = 0;
            if ( ( prevState == smWrReg_GLOBALADDREntry ) &&
                 ( smGLOBALADDR >= 0 ) )
            {
                pAddr = (uint32_t*)&smGLOBALADDR;
            }
            else if ( ( prevState == smWrReg_MOTORADDREntry ) &&
                      ( smMOTORADDR > 0 ) )
            {
                pAddr = (uint32_t*)&smMOTORADDR;
            }

            if ( pAddr )
            {
                ControllerBoardInterface::t_ptxrxbuf txb = std::make_shared<ControllerBoardInterface::t_txrxbuf>();
                txb->resize( sizeof( smVALUE ) );
                uint32_t* p = (uint32_t*)txb->data();
                *p = smVALUE;

                pCbi_->WriteRegister( *pAddr, txb,
                                      std::bind( &MotorControlTester::WrtCallback, this,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4 ) );

                // don't use the normal motor reset, as it copies the default parameters
                GetMotorObjForId( smMOTORID, pMotor );
                pMotorRegs_ = NULL;
                pMotor->Reset( NULL );      // just reset the board after writing the parameter directly to it...
            }
            else if ( ( prevState == smWrReg_PARAMADDREntry ) &&
                      ( smMOTOROFFSETADDR > 0 ) )
            {
                UpdateMotorRegFromOffsetAddr( smMOTOROFFSETADDR, smVALUE );
            }
            smState = smTop;
            break;
        }
        case smWrReg_CMDEntry:
        {
            if ( !smValueLine.empty() )
            {
                uint32_t cmd = 0;
                status = sscanf_s( smValueLine.c_str(), "%d", &cmd );
                if ( status != 1 )
                {
                    std::cout << std::endl << "Not a legal value: " << smValueLine << std::endl;
                    smState = smTop;
                }
                else
                {
                    motorCmd = cmd;
                    switch ( cmd )
                    {
                        case MotorCommands::MotorCmd_Enable:
                        {
                            smState = smWrReg_ENABLEEntry;
                            break;
                        }
                        case MotorCommands::MotorCmd_GotoPosition:
                        case MotorCommands::MotorCmd_MoveRelative:
                        {
                            smState = smWrReg_POSEntry;
                            break;
                        }
                        case MotorCommands::MotorCmd_Stop:
                        {
                            smState = smWrReg_STOPEntry;
                            break;
                        }
                        case MotorCommands::MotorCmd_GotoHome:
                        case MotorCommands::MotorCmd_MarkPositionAsZero:
                        case MotorCommands::MotorCmd_ClearErrorCode:
                        case MotorCommands::MotorCmd_ProbeDown:
                        case MotorCommands::MotorCmd_GotoTravelLimit:
                        {
                            smState = smWrReg_CMDExecute;
                            break;
                        }
//                        case MotorCommands::MotorCmd_Reset:
//                        case ( MotorCommands::MotorCmd_Reset * 10 ):
                        default:
                        {
                            std::cout << std::endl << "Not a legal command: " << smValueLine << std::endl;
                            smState = smTop;
                            motorCmd = 0;
                            break;
                        }
                    }
                    smValueLine.clear();
                }
            }
            break;
        }
        case smWrReg_CMDExecute:
        {
            if ( smMOTORID > 0 )
            {
                GetMotorObjAndRegsForId( smMOTORID, pMotor, pMotorRegs_ );
                if ( pMotor )
                {
//                    if ( motorCmd == ( MotorCommands::MotorCmd_Reset * 10 ) )
//                    {
//                        motorCmd /= 10;
//                        pMotorRegs_ = &motorRegs_;      // to update the motor from the local register set
//                    }
//                    else
//                    {
                        pMotorRegs_ = NULL;
//                    }
                    DoMotorCmd( motorCmd );
                }
                else
                {
                    std::cout << std::endl << "Not a legal motor Id: " << smMOTORID << std::endl;
                }
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }

            smState = smTop;
            break;
        }
        case smWrReg_POSEntry:
        {
            if ( !smValueLine.empty() )
            {
                int32_t posParam = 0;
                status = sscanf_s( smValueLine.c_str(), "%d", &posParam );
                if ( status != 1 )
                {
                    std::cout << std::endl << "Not a legal value: " << smValueLine << std::endl;
                    smCMDPARAM = 0;
                    smValueLine.clear();
                    break;
                }
                else
                {
                    smCMDPARAM = posParam;
                }
            }
            smState = smWrReg_CMDExecute;
            smValueLine.clear();
            break;
        }
        case smWrReg_STOPEntry:
        case smWrReg_ENABLEEntry:
        {
            if ( !smValueLine.empty() )
            {
                uint32_t cmdParam = 0;
                status = sscanf_s( smValueLine.c_str(), "%d", &cmdParam );
                if ( ( status != 1 ) || ( ( cmdParam != 0 ) && ( cmdParam != 1 ) ) )
                {
                    std::cout << std::endl << "Not a legal value (0 or 1):" << smValueLine << std::endl;
                    smValueLine.clear();
                    smCMDPARAM = 0;
                    break;
                }
                else
                {
                    smCMDPARAM = cmdParam;
                }
            }
            smState = smWrReg_CMDExecute;
            smValueLine.clear();
            break;
        }
        case smWrReg_ERRORClear:
        {
            if ( smMOTORID > 0 )
            {
                GetMotorObjAndRegsForId( smMOTORID, pMotor, pMotorRegs_ );
                if ( ( pMotor ) && ( pMotorRegs_ ) )
                {
                    pMotor->ReadMotorRegs( *pMotorRegs_ );        // re-read the registers after clearing the errors...
                    ShowMotorRegister( pMotor, pMotorRegs_ );
                }
                else
                {
                    std::cout << std::endl << "Not a legal motor Id: " << smMOTORID << std::endl;
                }
                pMotorRegs_ = NULL;
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            smState = smTop;
            break;
        }
        default:
        {
            std::cout << std::endl << "Forgot to handle a state machine case!!" << std::endl;
            smState = smTop;
        }
    }

    if ( smState == smTop )
    {
        prevState = 0;
        smValueLine.clear();
    }

    switch ( smState )
    {
        case smRdReg_MOTORStatus:       // allow processing for status display commands
        case smRdReg_ALLStatus:         // allow processing for status display commands
        case smWrReg_CMDExecute:        // allow execution of motor commands
        {
            break;
        }
        default:
        {
            Prompt();
            break;
        }
    }

    pTimer->expires_from_now( boost::posix_time::milliseconds( 100 ), timerError );
    pTimer->async_wait( std::bind( &MotorControlTester::HandleInput, this, std::placeholders::_1 ) );
}

void MotorControlTester::HandleTopState( int C )
{
    MotorBase * pMotor = 0;

    switch ( C )
    {
        case 'r':
        case 'R':
        {
            smState = smRdReg_MOTORADDREntry;
            break;
        }
        case 'i':
        case 'I':
        {
            smState = smRdReg_GLOBALADDREntry;
            break;
        }
        case 'm':
        case 'M':
        {
            smState = smRdReg_MOTORIDEntry;
            break;
        }
        case 's':
        case 'S':
        {
            smState = smRdReg_MOTORStatus;
            break;
        }
        case 'a':
        case 'A':
        {
            smState = smRdReg_ALLStatus;
            break;
        }
        case 'p':
        case 'P':
        {
            smState = smWrReg_MOTORInit;
            if ( smMOTORID > 0 )
            {
                t_opPTree motor_cfg;

                GetMotorObjForId( smMOTORID, pMotor );
                InitMotorParams( smMOTORID, InitTypes::InitTypeParams, motor_cfg );   // get the motor_cfg tree pointer...
                                                                                      // allow each motor controller to initialize its parameters from the external info file
                if ( pMotor )
                {
                    pMotor->ApplyDefaultParams();
                    pMotorRegs_ = NULL;
                }
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            break;
        }
        case 'w':
        case 'W':
        {
            smState = smWrReg_MOTORADDREntry;
            break;
        }
        case 'u':
        case 'U':
        {
            smState = smWrReg_PARAMADDREntry;
            break;
        }
        case 'o':
        case 'O':
        {
            smState = smWrReg_GLOBALADDREntry;
            break;
        }
        case 'v':
        case 'V':
        {
            smState = smWrReg_VALEntry;
            break;
        }
        case 'c':
        case 'C':
        {
            smState = smWrReg_CMDEntry;
            break;
        }
        case 'e':
        case 'E':
        {
            smState = smWrReg_ERRORClear;
            if ( smMOTORID > 0 )
            {
                GetMotorObjForId( smMOTORID, pMotor );
                if ( pMotor )
                {
                    pMotor->ClearErrors();
                }
            }
            else
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            break;
        }
        default:
        {
            smState = smTop;
            break;
        }
    }
}

bool MotorControlTester::ReadMotorOffsetAddress( std::string & inputStr )
{
    if ( !inputStr.empty() )
    {
        int32_t addrOffset = 0;
        int status = sscanf_s( inputStr.c_str(), "%d", &addrOffset );
        if ( ( status != 1 ) ||
             ( smMOTORID <= 0 ) ||
             ( addrOffset < 0 ) ||
             ( addrOffset > ( sizeof( MotorRegisters ) - sizeof( uint32_t ) ) ) ||
             ( ( addrOffset % ( sizeof( uint32_t ) ) != 0 ) ) )
        {
            if ( status != 1 )
            {
                std::cout << std::endl << "Not a legal value: " << inputStr << std::endl;
                smMOTOROFFSETADDR = 0;
            }
            else if ( smMOTORID <= 0 )
            {
                std::cout << std::endl << "Motor Id not selected!" << std::endl;
            }
            else if ( ( addrOffset < 0 ) ||
                      ( addrOffset > ( sizeof( MotorRegisters ) - sizeof(uint32_t) ) ) ||
                      ( ( addrOffset % ( sizeof( uint32_t ) ) != 0 ) ) )
            {
                std::cout << std::endl << "Not a valid address offset value: " << inputStr << std::endl;
                smMOTOROFFSETADDR = 0;
            }
            smState = smTop;
            return false;
        }
        else
        {
            if ( smMOTORBASEADDR > 0 )
            {
                smMOTOROFFSETADDR = addrOffset;
                smMOTORADDR = smMOTORBASEADDR + addrOffset;
            }
            else
            {
                smMOTORBASEADDR = 0;
                smMOTOROFFSETADDR = 0;
            }
        }
        smValueLine.clear();
    }
    return true;
}

bool MotorControlTester::UpdateMotorRegFromOffsetAddr( uint32_t addr_offset, uint32_t newVal )
{
    uint32_t addrIdx = addr_offset / 4;
    MotorBase * pMotor = NULL;
    MotorRegisters * pRegs = NULL;
    bool updateValid = true;

    GetMotorObjAndRegsForId( smMOTORID, pMotor, pRegs );
    if ( pMotor )
    {
        switch ( addrIdx )
        {
            case HomeDirectionIdx:
                pMotor->SetHomeDirection( newVal );
                break;

            case MotorFullStepsPerRevIdx:
                pMotor->SetStepsPerRev( newVal );
                break;

            case UnitsPerRevIdx:
                pMotor->SetUnitsPerRev( newVal );
                break;

            case GearheadRatioIdx:
                pMotor->SetGearRatio( newVal );
                break;

            case EncoderTicksPerRevIdx:
                pMotor->SetTicksPerRev( newVal );
                break;

            case DeadbandIdx:
                pMotor->SetDeadband( newVal );
                break;

            case InvertedDirectionIdx:
                pMotor->SetInvertedDirection( newVal );
                break;

            case StepSizeIdx:
                pMotor->SetStepSize( newVal );
                break;

            case AccelerationIdx:
                pMotor->SetAccel( newVal );
                break;

            case DecelerationIdx:
                pMotor->SetDecel( newVal );
                break;

            case MinSpeedIdx:
                pMotor->SetMinSpeed( newVal );
                break;

            case MaxSpeedIdx:
                pMotor->SetMaxSpeed( newVal );
                break;

            case OverCurrentIdx:
                pMotor->SetOverCurrent( newVal );
                break;

            case StallCurrentIdx:
                pMotor->SetStallCurrent( newVal );
                break;

            case HoldVoltageDivideIdx:
                pMotor->SetHoldVoltDivide( newVal );
                break;

            case RunVoltageDivideIdx:
                pMotor->SetRunVoltDivide( newVal );
                break;

            case AccVoltageDivideIdx:
                pMotor->SetAccelVoltDivide( newVal );
                break;

            case DecVoltageDivideIdx:
                pMotor->SetDecelVoltDivide( newVal );
                break;

            case CONFIG_IDX:
                pMotor->SetConfig( newVal );
                break;

            case INT_SPEED_IDX:
                pMotor->SetIntSpd( newVal );
                break;

            case ST_SLP_IDX:
                pMotor->SetStSlp( newVal );
                break;

            case FN_SLP_ACC_IDX:
                pMotor->SetFnSlpAccel( newVal );
                break;

            case FN_SLP_DEC_IDX:
                pMotor->SetFnSlpDecel( newVal );
                break;

            case DelayAfterMoveIdx:
                pMotor->SetDelayAfterMove( newVal );
                break;

            case ProbeSpeed1Idx:
                pMotor->AdjustProbeSpeed1( newVal );
                break;

            case ProbeCurrent1Idx:
                pMotor->AdjustProbeSpeed1Current( newVal );
                break;

            case ProbeAbovePositionIdx:
                pMotor->AdjustProbeAbovePosition( newVal );
                break;

            case ProbeSpeed2Idx:
                pMotor->AdjustProbeSpeed2( newVal );
                break;

            case ProbeCurrent2Idx:
                pMotor->AdjustProbeSpeed2Current( newVal );
                break;

            case ProbeRaiseIdx:
                pMotor->AdjustProbeRaise( newVal );
                break;

            case ProbeStopPositionIdx:
                pMotor->SetDefaultProbeStop( newVal );
                break;

            case MaxTravelPosIdx:
                pMotor->AdjustMaxTravel( newVal );
                break;

            case MaxMoveRetriesIdx:
                pMotor->AdjustMaxRetries( newVal );
                break;

            case CommandIdx:
            case CommandParamIdx:
            case ErrorCodeIdx:
            case PositionIdx:
            default:
                updateValid = false;
                break;
        }

        if ( updateValid )
        {
            pMotor->SetAllParams();
            pMotor->ApplyAllParams();
        }
    }
    return updateValid;
}

bool MotorControlTester::ReadGlobalAddress( std::string & inputStr )
{
    if ( !inputStr.empty() )
    {
        int status = sscanf_s( inputStr.c_str(), "%d", &smGLOBALADDR );
        if ( status != 1 )
        {
            std::cout << std::endl << "Not a legal value: " << inputStr << std::endl;
            smGLOBALADDR = 0;
            smState = smTop;
            return false;
        }
        inputStr.clear();
    }
    return true;
}

void MotorControlTester::DoMotorCmd( uint32_t cmd )
{
    MotorBase * pMotor = 0;
    MotorRegisters * pRegs = NULL;

    GetMotorObjAndRegsForId( smMOTORID, pMotor, pRegs );

    if ( pMotor )
    {
        if ( !OpenCbi( cbiPort ) )
        {
            return;
        }

        switch ( cmd )
        {
            case MotorCommands::MotorCmd_GotoHome:
            {
                pMotor->Home();
                break;
            }
            case MotorCommands::MotorCmd_MarkPositionAsZero:
            {
                pMotor->MarkPosAsZero();
                break;
            }
            case MotorCommands::MotorCmd_GotoPosition:           // CommandParam: int32 (Config units)
            {
                pMotor->SetPosition( smCMDPARAM );
                break;
            }
            case MotorCommands::MotorCmd_ClearErrorCode:
            {
                pMotor->ClearErrors();
                break;
            }
//            case MotorCommands::MotorCmd_Reset:
//            {
//                pMotor->Reset( pMotorRegs_ );
//                break;
//            }
            case MotorCommands::MotorCmd_MoveRelative:           // CommandParam: int32 (Config units)
            {
                pMotor->MovePosRelative( smCMDPARAM );
                break;
            }
            case MotorCommands::MotorCmd_Stop:                   // CommandParam: 0/1 = soft/hard
            {
                if ( ( smCMDPARAM == 0 ) || ( smCMDPARAM == 1 ) )
                {
                    pMotor->Stop( smCMDPARAM );
                }
                else
                {
                    std::cout << std::endl << "Illegal parameter value: " << smCMDPARAM << std::endl;
                }
                break;
            }
            case MotorCommands::MotorCmd_Enable:                 // CommandParam: 0/1 = disable/enable, holding current= off/on
            {
                if ( ( smCMDPARAM == 0 ) || ( smCMDPARAM == 1 ) )
                {
                    bool enable = false;
                    if ( smCMDPARAM == 1 )
                    {
                        enable = true;
                    }
                    pMotor->Enable( enable );
                }
                else
                {
                    std::cout << std::endl << "Illegal parameter value: " << smCMDPARAM << std::endl;
                }
                break;
            }
            case MotorCommands::MotorCmd_ProbeDown:
            {
                if ( smMOTORID == MotorTypeProbe )
                {
                    pMotor->ProbeDown();
                }
                break;
            }
            case MotorCommands::MotorCmd_GotoTravelLimit:
            {
                if ( smMOTORID == MotorTypeReagent )
                {
                    pMotor->GotoTravelLimit();
                }
                pMotor->ProbeDown();
                break;
            }
            default:
            {
                std::cout << std::endl << "Not a legal command: " << cmd << std::endl;
                break;
            }
        }
    }
    else
    {
        std::cout << std::endl << "Not a legal motor Id: " << smMOTORID << std::endl;
    }

    if ( pRegs )
    {
        pRegs->Command = 0;
        pRegs->CommandParam = 0;
    }

    Prompt();

}

void MotorControlTester::GetMotorObjForId( int32_t motorId, MotorBase * & pMotor )
{
    MotorBase * pmotor_ = NULL;

    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            pmotor_ = &radiusMotor;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            pmotor_ = &thetaMotor;
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            pmotor_ = &probeMotor;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            pmotor_ = &reagentMotor;
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            pmotor_ = &focusMotor;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            pmotor_ = &rack1Motor;
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            pmotor_ = &rack2Motor;
            break;
        }

        default:
        {
            std::cout << std::endl << "No/Unrecognized motor Id specified: %d" << motorId << std::endl;
            pmotor_ = NULL;
            break;
        }
    }
    pMotor = pmotor_;
}

void MotorControlTester::GetMotorRegsForId( int32_t motorId, MotorRegisters * & pRegs )
{
    MotorRegisters * pregs_ = 0;

    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            pregs_ = &radiusMotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            pregs_ = &thetaMotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            pregs_ = &probeMotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            pregs_ = &reagentMotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            pregs_ = &focusMotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            pregs_ = &rack1MotorRegs_;
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            pregs_ = &rack2MotorRegs_;
            break;
        }

        default:
        {
            std::cout << std::endl << "No/Unrecognized motor Id specified: %d" << motorId << std::endl;
            pregs_ = 0;
            break;
        }
    }
    pRegs = pregs_;
}

void MotorControlTester::GetMotorObjAndRegsForId( int32_t motorId, MotorBase * & pMotor, MotorRegisters * & pRegs )
{
    MotorRegisters * pregs_ = 0;
    MotorBase * pmotor_;

    GetMotorObjForId( motorId, pmotor_ );
    GetMotorRegsForId( motorId, pregs_ );

    pMotor = pmotor_;
    pRegs = pregs_;
}

uint32_t MotorControlTester::GetMotorRegsAddrForId( int32_t motorId )
{
    int32_t addr = 0;

    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            addr = MotorTypeRegAddr::MotorTypeRadiusBaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            addr = MotorTypeRegAddr::MotorTypeThetaBaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            addr = MotorTypeRegAddr::MotorTypeProbeBaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            addr = MotorTypeRegAddr::MotorTypeReagentBaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            addr = MotorTypeRegAddr::MotorTypeFocusBaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            addr = MotorTypeRegAddr::MotorTypeRack1BaseRegAddr;
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            addr = MotorTypeRegAddr::MotorTypeRack2BaseRegAddr;
            break;
        }

        default:
        {
            std::cout << std::endl << "No/Unrecognized motor Id specified: %d" << motorId << std::endl;
            break;
        }
    }
    return addr;
}

void MotorControlTester::RdCallback( boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx )
{
    uint32_t * p = NULL;

    if ( ( !ec ) && ( rx ) )
    {
        p = (uint32_t *)rx->data();
    }

    if ( ( ec ) || ( !rx ) || ( !p ) )
    {
        std::string errStr = "";
        if ( ec )
        {
            errStr = boost::str( boost::format( "Read Callback reports an error: %s" ) % ec.message() );
        }
        else if ( !rx )
        {
            errStr = "MCT Read Callback received no buffer!";
        }
        else if ( !p )
        {
            errStr = "MCT Read Callback received no data!";
        }
        std::cout << std::endl << errStr << std::endl;
    }
    else
    {
        rdValue = *p;
        std::string data = boost::str( boost::format( "MCT Read Callback\n\tValue: 0x%08X\n" ) % rdValue );
        std::cout << std::endl << data << std::endl;
    }
    std::string data = boost::str( boost::format( "MCT Read Callback\n\tStatus: 0x%04X" ) % status );
    std::cout << std::endl << data << std::endl;
}

void MotorControlTester::WrtCallback( boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx )
{
    if ( ec )
    {
        std::string errStr = boost::str( boost::format( "Motor Control Tester Write Callback reports an error: %s" ) % ec.message() );
        Logger::L().Log ( MODULENAME, severity_level::normal, errStr );
        std::cout << std::endl << errStr << std::endl;
    }
    std::string data = boost::str( boost::format( "Write Callback\n\tStatus: 0x%04X\n" ) % status );
    std::cout << std::endl << data << std::endl;
}











int main()
{
    std::shared_ptr<boost::asio::io_context> io_svc;
    std::shared_ptr<boost::asio::io_context::work> io_svc_work;

	io_svc.reset( new boost::asio::io_context() );
	io_svc_work.reset( new boost::asio::io_context::work( *io_svc ) );

    boost::system::error_code ec;
    Logger::L().Initialize( ec, boost::str( boost::format( "%s.info" ) % MODULENAME ) );
	Logger::L().Log ( MODULENAME, severity_level::normal, boost::str( boost::format( "Starting %s" ) % MODULENAME ) );

    std::shared_ptr<ControllerBoardInterface> pcbi;
    pcbi.reset( new ControllerBoardInterface( io_svc, CNTLR_SN_A_STR, CNTLR_SN_B_STR ) );
    MotorControlTester mct( io_svc, pcbi );
	std::string logStr = "";
 
	if ( !mct.Init() )
    {
		logStr = "No FTDI devices found ";
		Logger::L().Log ( MODULENAME, severity_level::normal, logStr );
		std::cout << logStr << std::endl;
    }

    if ( !mct.Start() )
    {
        logStr = "Unable to open controller board ";
		Logger::L().Log ( MODULENAME, severity_level::normal, logStr );
		std::cout << logStr << std::endl;
	}

	std::cout << std::endl;

	io_svc->run();

	io_svc_work.reset();    // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	io_svc->stop();			// instruct io_service to stop processing (should exit ::run() and end thread.

	Logger::L().Flush();
	std::cout.flush();

	return 0;	// Never get here normally.
}

