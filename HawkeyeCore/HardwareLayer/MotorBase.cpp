#include "stdafx.h"

#include <stdio.h>
#include <boost/format.hpp>

#include "ErrorCode.hpp"
#include "Logger.hpp"
#include "MotorBase.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "MotorBase";

static const char UnknownMotorNameStr[] = "Unknown";
static const char ProbeMotorNameStr[] = "Probe";
static const char RadiusMotorNameStr[] = "Radius";
static const char ThetaMotorNameStr[] = "Theta";
static const char FocusMotorNameStr[] = "Focus";
static const char ReagentMotorNameStr[] = "Reagent";
static const char Rack1MotorNameStr[] = "Rack1";
static const char Rack2MotorNameStr[] = "Rack2";

static instrument_error::motion_motor_instances getMotorErrorInstace(MotorTypeId id)
{
	switch (id)
	{
		case MotorTypeRadius:
			return instrument_error::motion_motor_instances::radius;
		case MotorTypeTheta:
			return instrument_error::motion_motor_instances::theta;
		case MotorTypeProbe:
			return instrument_error::motion_motor_instances::sample_probe;
		case MotorTypeReagent:
			return instrument_error::motion_motor_instances::reagent_probe;
		case MotorTypeFocus:
			return instrument_error::motion_motor_instances::focus;
		case MotorTypeRack1:
			return instrument_error::motion_motor_instances::fl_rack1;
		case MotorTypeRack2:
			return instrument_error::motion_motor_instances::fl_rack2;
		default:
			break;
	}
	HAWKEYE_ASSERT (MODULENAME, false);
}

MotorBase::MotorBase(std::shared_ptr<CBOService> pCBOService, MotorTypeId mtype )
    : pCBOService_(std::move(pCBOService))
    , regUpdTimer( pCBOService_->getInternalIosRef())
    , posUpdTimer( pCBOService_->getInternalIosRef())
    , updateConfig(false)
    , motorNodeName( "" )
    , configFile( "" )
    , cbiPort("")
    , initComplete(false)
    , closing( false )
    , positionTolerance( UnitsChange_0_1_mm )               // set default position tolerance to 0.1mm
    , probeStopPos(0)
    , motorStartTimeout_msec(MotorStartTimeout * TIMEOUT_FACTOR)
    , motorBusyTimeout_msec (ThetaFullTimeout * TIMEOUT_FACTOR)
    , homeDirectionParam_(0)
    , regUpdateInterval_ms(MotorRegUpdateStdInterval)
    , posUpdateInterval_ms(DefaultPosUpdateStdInterval)    
    , motorTypeId_(MotorTypeUnknown)
    , motorRegsAddr_(MotorTypeRegAddrUnknown)
 
{
    configTree.reset();
    paramNode.reset();
    motorNode.reset();

    memset( &motorRegs_, 0, sizeof( MotorRegisters ) );     // clear the motor register set initially
    SetMotorType(mtype);

	memset (&motorRegisterValuesCache_, 0, sizeof(MotorRegisterValues));

    switch ( mtype )
    {
        case MotorTypeId::MotorTypeProbe:
        {
            posUpdateInterval_ms = ProbePosUpdateInterval;
            break;
        }

        case MotorTypeId::MotorTypeRadius:
        {            
            posUpdateInterval_ms = RadiusPosUpdateInterval;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {            
            positionTolerance = ThetaChange1Degree;         // for the theta motor, set the defalt position tolerance to 1.0 degrees...            
            posUpdateInterval_ms = ThetaPosUpdateInterval;
            break;
        }

        case MotorTypeId::MotorTypeFocus:
        {            
            posUpdateInterval_ms = FocusPosUpdateInterval;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {            
            posUpdateInterval_ms = ReagentPosUpdateInterval;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        case MotorTypeId::MotorTypeRack2:
        {            
            posUpdateInterval_ms = LedRackPosUpdateInterval;
            break;
        }

        default:
        {
			posUpdateInterval_ms = 0;
            break;
        }
    }

}

MotorBase::~MotorBase()
{
	Quit();
}

void MotorBase::FormatMotorRegister( MotorRegisters * pRegs, std::string & regStr ) const
{
	std::string motorStr = "";

    GetMotorTypeAsString( motorTypeId_, motorStr );

    regStr = "Register Content:  ";
    regStr.append( boost::str( boost::format( " Id: %u - %s\n" ) % motorTypeId_  % motorStr ) );
    regStr.append( boost::str( boost::format( "\tCommand: %u" ) % pRegs->Command ) );
    regStr.append( boost::str( boost::format( "\tCommandParam: %u" ) % pRegs->CommandParam ) );
    regStr.append( boost::str( boost::format( "\tErrorCode: 0x%08X" ) % pRegs->ErrorCode ) );
    regStr.append( boost::str( boost::format( "\tPosition: %d" ) % pRegs->Position ) );
	regStr.append( boost::str( boost::format( "\tHomedStatus: %d") % pRegs->HomedStatus ) );
	regStr.append( boost::str( boost::format( "\tProbeStopPosition: %u") % pRegs->ProbeStopPosition ) );
    regStr.append( boost::str( boost::format( "\tHomeDirection: %u" ) % pRegs->HomeDirection ) );
    regStr.append( boost::str( boost::format( "\tMotorFullStepsPerRev: %u" ) % pRegs->MotorFullStepsPerRev ) );
    regStr.append( boost::str( boost::format( "\tUnitsPerRev: %u" ) % pRegs->UnitsPerRev ) );
    regStr.append( boost::str( boost::format( "\tGearheadRatio: %u" ) % pRegs->GearheadRatio ) );
    regStr.append( boost::str( boost::format( "\tEncoderTicksPerRev: %d" ) % pRegs->EncoderTicksPerRev ) );
    regStr.append( boost::str( boost::format( "\tDeadband: %u" ) % pRegs->Deadband ) );
    regStr.append( boost::str( boost::format( "\tInvertedDirection: %u" ) % pRegs->InvertedDirection ) );
    regStr.append( boost::str( boost::format( "\tStepSize: %u" ) % pRegs->StepSize ) );
	regStr.append( boost::str( boost::format( "\tMaxSpeed: %u") % pRegs->profileRegs.MaxSpeed ) );
    regStr.append( boost::str( boost::format( "\tAcceleration: %u" ) % pRegs->profileRegs.Acceleration ) );
    regStr.append( boost::str( boost::format( "\tDeceleration: %u") % pRegs->profileRegs.Deceleration ) );
	regStr.append( boost::str( boost::format( "\tRunVoltageDivide: %u") % pRegs->profileRegs.RunVoltageDivide ) );
	regStr.append( boost::str( boost::format( "\tAccVoltageDivide: %u") % pRegs->profileRegs.AccVoltageDivide ) );
	regStr.append( boost::str( boost::format( "\tDecVoltageDivide: %u") % pRegs->profileRegs.DecVoltageDivide ) );
    regStr.append( boost::str( boost::format( "\tMinSpeed: %u" ) % pRegs->MinSpeed ) );    
    regStr.append( boost::str( boost::format( "\tOverCurrent: %u" ) % pRegs->OverCurrent ) );
    regStr.append( boost::str( boost::format( "\tStallCurrent: %u" ) % pRegs->StallCurrent ) );
    regStr.append( boost::str( boost::format( "\tHoldVoltageDivide: %u" ) % pRegs->HoldVoltageDivide ) );    
    regStr.append( boost::str( boost::format( "\tCONFIG: %u" ) % pRegs->CONFIG ) );
    regStr.append( boost::str( boost::format( "\tINT_SPEED: %u" ) % pRegs->INT_SPEED ) );
    regStr.append( boost::str( boost::format( "\tST_SLP: %u" ) % pRegs->ST_SLP ) );
    regStr.append( boost::str( boost::format( "\tFN_SLP_ACC: %u" ) % pRegs->FN_SLP_ACC ) );
    regStr.append( boost::str( boost::format( "\tFN_SLP_DEC: %u" ) % pRegs->FN_SLP_DEC ) );    
    regStr.append( boost::str( boost::format( "\tDelayAfterMove: %u" ) % pRegs->DelayAfterMove ) );	
    regStr.append( boost::str( boost::format( "\tMaxMoveRetries: %u" ) % pRegs->MaxMoveRetries ) );
    regStr.append( boost::str( boost::format( "\tProbeSpeed1: %u" ) % pRegs->ProbeRegs.ProbeSpeed1 ) );
    regStr.append( boost::str( boost::format( "\tProbeCurrent1: %u" ) % pRegs->ProbeRegs.ProbeCurrent1 ) );
    regStr.append( boost::str( boost::format( "\tProbeAbovePosition: %u" ) % pRegs->ProbeRegs.ProbeAbovePosition ) );
    regStr.append( boost::str( boost::format( "\tProbeSpeed2: %u" ) % pRegs->ProbeRegs.ProbeSpeed2 ) );
    regStr.append( boost::str( boost::format( "\tProbeCurrent2: %u" ) % pRegs->ProbeRegs.ProbeCurrent2 ) );
    regStr.append( boost::str( boost::format( "\tProbeRaise: %u" ) % pRegs->ProbeRegs.ProbeRaise ) );    
    regStr.append( boost::str( boost::format( "\tMaxTravelPosition: %u" ) % pRegs->MaxTravelPosition ) );
}

void MotorBase::ReadMotorParamsFromInfo(MotorCfgParams & pMotorCfg, t_opPTree motorNode)
{
    boost::property_tree::ptree & params = motorNode.get();

	pMotorCfg.ProbeStopPosition =            params.get<uint32_t>("ProbeStopPosition",    pMotorCfg.ProbeStopPosition);
    pMotorCfg.HomeDirection =                params.get<uint32_t>("HomeDirection",        pMotorCfg.HomeDirection);
    pMotorCfg.MotorFullStepsPerRev =         params.get<uint32_t>("MotorFullStepsPerRev", pMotorCfg.MotorFullStepsPerRev);
    pMotorCfg.UnitsPerRev =                  params.get<uint32_t>("UnitsPerRev",          pMotorCfg.UnitsPerRev);
    pMotorCfg.GearheadRatio =                params.get<uint32_t>("GearheadRatio",        pMotorCfg.GearheadRatio);
    pMotorCfg.EncoderTicksPerRev =           params.get<uint32_t>("EncoderTicksPerRev",   pMotorCfg.EncoderTicksPerRev);
    pMotorCfg.Deadband =                     params.get<uint32_t>("Deadband",             pMotorCfg.Deadband);
    pMotorCfg.InvertedDirection =            params.get<uint32_t>("InvertedDirection",    pMotorCfg.InvertedDirection);
    pMotorCfg.StepSize =                     params.get<uint32_t>("StepSize",             pMotorCfg.StepSize);
	pMotorCfg.profileRegs.MaxSpeed =         params.get<uint32_t>("MaxSpeed",             pMotorCfg.profileRegs.MaxSpeed);
    pMotorCfg.profileRegs.Acceleration =     params.get<uint32_t>("Acceleration",         pMotorCfg.profileRegs.Acceleration);
    pMotorCfg.profileRegs.Deceleration =     params.get<uint32_t>("Deceleration",         pMotorCfg.profileRegs.Deceleration);
	pMotorCfg.profileRegs.RunVoltageDivide = params.get<uint32_t>("RunVoltageDivide",     pMotorCfg.profileRegs.RunVoltageDivide);
	pMotorCfg.profileRegs.AccVoltageDivide = params.get<uint32_t>("AccVoltageDivide",     pMotorCfg.profileRegs.AccVoltageDivide);
	pMotorCfg.profileRegs.DecVoltageDivide = params.get<uint32_t>("DecVoltageDivide",     pMotorCfg.profileRegs.DecVoltageDivide);
    pMotorCfg.MinSpeed =                     params.get<uint32_t>("MinSpeed",             pMotorCfg.MinSpeed);    
    pMotorCfg.OverCurrent =                  params.get<uint32_t>("OverCurrent",          pMotorCfg.OverCurrent);
    pMotorCfg.StallCurrent =                 params.get<uint32_t>("StallCurrent",         pMotorCfg.StallCurrent);
    pMotorCfg.HoldVoltageDivide =            params.get<uint32_t>("HoldVoltageDivide",    pMotorCfg.HoldVoltageDivide);    
    pMotorCfg.CONFIG =                       params.get<uint32_t>("CONFIG",               pMotorCfg.CONFIG);
    pMotorCfg.INT_SPEED =                    params.get<uint32_t>("INT_SPEED",            pMotorCfg.INT_SPEED);
    pMotorCfg.ST_SLP =                       params.get<uint32_t>("ST_SLP",               pMotorCfg.ST_SLP);
    pMotorCfg.FN_SLP_ACC =                   params.get<uint32_t>("FN_SLP_ACC",           pMotorCfg.FN_SLP_ACC);
    pMotorCfg.FN_SLP_DEC =                   params.get<uint32_t>("FN_SLP_DEC",           pMotorCfg.FN_SLP_DEC);    
    pMotorCfg.DelayAfterMove =               params.get<uint32_t>("DelayAfterMove",       pMotorCfg.DelayAfterMove);
	pMotorCfg.MaxMoveRetries =               params.get<uint32_t>("MaxMoveRetries",       pMotorCfg.MaxMoveRetries);
    pMotorCfg.ProbeRegs.ProbeSpeed1 =        params.get<uint32_t>("ProbeSpeed1",          pMotorCfg.ProbeRegs.ProbeSpeed1);
    pMotorCfg.ProbeRegs.ProbeCurrent1 =      params.get<uint32_t>("ProbeCurrent1",        pMotorCfg.ProbeRegs.ProbeCurrent1);
    pMotorCfg.ProbeRegs.ProbeAbovePosition = params.get<uint32_t>("ProbeAbovePosition",   pMotorCfg.ProbeRegs.ProbeAbovePosition);
    pMotorCfg.ProbeRegs.ProbeSpeed2 =        params.get<uint32_t>("ProbeSpeed2",          pMotorCfg.ProbeRegs.ProbeSpeed2);
    pMotorCfg.ProbeRegs.ProbeCurrent2 =      params.get<uint32_t>("ProbeCurrent2",        pMotorCfg.ProbeRegs.ProbeCurrent2);
    pMotorCfg.ProbeRegs.ProbeRaise =         params.get<uint32_t>("ProbeRaise",           pMotorCfg.ProbeRegs.ProbeRaise);
    pMotorCfg.MaxTravelPosition =            params.get<uint32_t>( "MaxTravelPosition",   pMotorCfg.MaxTravelPosition );
}

int32_t MotorBase::WriteMotorConfig( void )
{
    // Unable to save through this method if no configuration location was given!
    if ( !motorNode )
        return -1;

    // Update our pTree with the new values
    if (UpdateMotorConfig( &dfltParams_, motorNode ) != 1)
		return -3;

    // Flush the cached file to disk
    boost::system::error_code ec;
    ConfigUtils::WriteCachedConfigFile( configFile, ec );

    if ( ec )
    {
        return -2;
    }

    return 0;
}

int32_t MotorBase::UpdateMotorConfig( MotorCfgParams * pMotorCfg, t_opPTree & motor_node ) const
{
    if ( !motor_node )
        return -3;

// controller_node points at the "motor_node" node for this particular motor type in its parent controller, (or the standalone motor)
// NOTE that the motor node MAY come from the redundant "motor_parameters" section for motors used outside of a controller.
/*  config
*       logger
*           motor_controllers
*               xxx_controller
*                   motor_parameters
*                       xxx_motor
*                           HomeDirection
*                           MotorFullStepsPerRev;
*                           UnitsPerRev;
*                           GearheadRatio;
*                           EncoderTicksPerRev;
*                           Deadband;
*                           InvertedDirection;
*                               .
*                               .
*                               .
*                       yyy_motor
*                           HomeDirection
*                           MotorFullStepsPerRev;
*                           UnitsPerRev;
*                           GearheadRatio;
*                           EncoderTicksPerRev;
*                           Deadband;
*                           InvertedDirection;
*                               .
*                               .
*                               .
*                   controller_config
*                       xxxPositionTolerance 1000
*                       xxxHomePos 0
*                       xxxHomePosOffset 0
*                           .
*                           .
*                           .
*               yyy_controller
*                   motor_parameters
*                       xxx_motor
*                           HomeDirection
*                           MotorFullStepsPerRev;
*                           UnitsPerRev;
*                           GearheadRatio;
*                           EncoderTicksPerRev;
*                           Deadband;
*                           InvertedDirection;
*                               .
*                               .
*                               .
*   motor_parameters
*       xxx_motor
*           HomeDirection
*           MotorFullStepsPerRev;
*           UnitsPerRev;
*           GearheadRatio;
*           EncoderTicksPerRev;
*           Deadband;
*           InvertedDirection;
*               .
*               .
*               .
*       yyy_motor
*           HomeDirection
*           MotorFullStepsPerRev;
*           UnitsPerRev;
*           GearheadRatio;
*           EncoderTicksPerRev;
*           Deadband;
*           InvertedDirection;
*               .
*               .
*               .
*/
    boost::property_tree::ptree & paramNode = motorNode.get();

	paramNode.put<uint32_t>( "ProbeStopPosition", pMotorCfg->ProbeStopPosition);
    paramNode.put<uint32_t>( "HomeDirection", pMotorCfg->HomeDirection );
    paramNode.put<uint32_t>( "MotorFullStepsPerRev", pMotorCfg->MotorFullStepsPerRev );
    paramNode.put<uint32_t>( "UnitsPerRev", pMotorCfg->UnitsPerRev );
    paramNode.put<uint32_t>( "GearheadRatio", pMotorCfg->GearheadRatio );
    paramNode.put<uint32_t>( "EncoderTicksPerRev", pMotorCfg->EncoderTicksPerRev );
    paramNode.put<uint32_t>( "Deadband", pMotorCfg->Deadband );
    paramNode.put<uint32_t>( "InvertedDirection", pMotorCfg->InvertedDirection );
    paramNode.put<uint32_t>( "StepSize", pMotorCfg->StepSize );
	paramNode.put<uint32_t>( "MaxSpeed", pMotorCfg->profileRegs.MaxSpeed);
    paramNode.put<uint32_t>( "Acceleration", pMotorCfg->profileRegs.Acceleration );
    paramNode.put<uint32_t>( "Deceleration", pMotorCfg->profileRegs.Deceleration );
	paramNode.put<uint32_t>( "RunVoltageDivide", pMotorCfg->profileRegs.RunVoltageDivide);
	paramNode.put<uint32_t>( "AccVoltageDivide", pMotorCfg->profileRegs.AccVoltageDivide);
	paramNode.put<uint32_t>( "DecVoltageDivide", pMotorCfg->profileRegs.DecVoltageDivide);
    paramNode.put<uint32_t>( "MinSpeed", pMotorCfg->MinSpeed );    
    paramNode.put<uint32_t>( "OverCurrent", pMotorCfg->OverCurrent );
    paramNode.put<uint32_t>( "StallCurrent", pMotorCfg->StallCurrent );
    paramNode.put<uint32_t>( "HoldVoltageDivide", pMotorCfg->HoldVoltageDivide );
    paramNode.put<uint32_t>( "CONFIG", pMotorCfg->CONFIG );
    paramNode.put<uint32_t>( "INT_SPEED", pMotorCfg->INT_SPEED );
    paramNode.put<uint32_t>( "ST_SLP", pMotorCfg->ST_SLP );
    paramNode.put<uint32_t>( "FN_SLP_ACC", pMotorCfg->FN_SLP_ACC );
    paramNode.put<uint32_t>( "FN_SLP_DEC", pMotorCfg->FN_SLP_DEC );    
    paramNode.put<uint32_t>( "DelayAfterMove", pMotorCfg->DelayAfterMove );
	paramNode.put<uint32_t>( "MaxMoveRetries", pMotorCfg->MaxMoveRetries);
    paramNode.put<uint32_t>( "ProbeSpeed1", pMotorCfg->ProbeRegs.ProbeSpeed1 );
    paramNode.put<uint32_t>( "ProbeCurrent1", pMotorCfg->ProbeRegs.ProbeCurrent1 );
    paramNode.put<uint32_t>( "ProbeAbovePosition", pMotorCfg->ProbeRegs.ProbeAbovePosition );
    paramNode.put<uint32_t>( "ProbeSpeed2", pMotorCfg->ProbeRegs.ProbeSpeed2 );
    paramNode.put<uint32_t>( "ProbeCurrent2", pMotorCfg->ProbeRegs.ProbeCurrent2 );
    paramNode.put<uint32_t>( "ProbeRaise", pMotorCfg->ProbeRegs.ProbeRaise );
    paramNode.put<uint32_t>( "MaxTravelPosition", pMotorCfg->MaxTravelPosition );   

    return 1;
}

bool MotorBase::CfgTreeValid( t_pPTree cfgtree, std::string controllerName )
{
	if (!cfgtree)
		return false;
    
    t_opPTree cfgNode = cfgtree->get_child_optional( "config" );  // top level parent node

	if (!cfgNode)
		return false;
    
    t_opPTree ctrlrsNode = cfgNode->get_child_optional( "motor_controllers" );            // look for the controllers section for individualized parameters...
	if (!ctrlrsNode)
		return false;

	t_opPTree thisTgtNode = ctrlrsNode->get_child_optional( controllerName );         // look for this specifc controller
	if (!thisTgtNode)
		return false;
    
    t_opPTree paramsNode = thisTgtNode->get_child_optional( "motor_parameters" );
	if (!paramsNode)
		return false;
	    
    t_opPTree subCfgNode = thisTgtNode->get_child_optional( "controller_config" );
	if (!subCfgNode)
		return false;
    
    return true;
}

void MotorBase::InitDefaultParams(std::function<void(bool)> cb, MotorTypeId motorId, t_pPTree cfgTree, t_opPTree cfgNode, bool apply )
{
    MotorCfgParams params;
    std::string motorName = "";
    
    
    switch ( motorId )
    {
        case MotorTypeId::MotorTypeRadius:
        {
            motorName = "radius_motor";                             // node names are NOT the same as the return for motor type name!
            params.UnitsPerRev = 320000;                            // 32 mm /revolution based on 16 tooth gear and 2mm pitch
            params.EncoderTicksPerRev = -800;
            params.Deadband = 1600;
            params.profileRegs.Acceleration = 1500000;
            params.profileRegs.Deceleration = 1500000;
            params.profileRegs.MaxSpeed = 350000;
            params.MaxTravelPosition = RadiusMaxTravel;
            motorBusyTimeout_msec = RadiusFullTimeout *TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeTheta:
        {
            motorName = "theta_motor";                              // node names are NOT the same as the return for motor type name!
            params.HomeDirection = 1;
            params.UnitsPerRev = 900;                               // with gearing reduction, 90 degrees per rev
            params.GearheadRatio = 4;
            params.EncoderTicksPerRev = 800;
            params.Deadband = 5;
            params.InvertedDirection = 1;
            params.profileRegs.Acceleration = 1200;
            params.profileRegs.Deceleration = 1200;
            params.profileRegs.MaxSpeed = 150;
            motorBusyTimeout_msec = ThetaFullTimeout * TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeProbe:
        {
            motorName = "probe_motor";                              // node names are NOT the same as the return for motor type name!
            params.EncoderTicksPerRev = -1440;
            params.Deadband = 285;
            params.profileRegs.Acceleration = 1500000;
            params.profileRegs.Deceleration = 1500000;
            params.profileRegs.MaxSpeed = 500000;           
            params.profileRegs.RunVoltageDivide = 1000000;
            params.profileRegs.AccVoltageDivide = 1000000;
            params.profileRegs.DecVoltageDivide = 1000000;
			params.OverCurrent = 1875000;
			params.HoldVoltageDivide = 200000;
			params.ProbeRegs.ProbeSpeed1 = 500000;
            params.ProbeRegs.ProbeCurrent1 = 1000000;
            params.ProbeRegs.ProbeAbovePosition = 385000;
            params.ProbeRegs.ProbeSpeed2 = 20000;
            params.ProbeRegs.ProbeCurrent2 = 275000;
            params.ProbeRegs.ProbeRaise = 10000;
            params.ProbeStopPosition = 0;
            params.MaxTravelPosition = ProbeMaxTravel;
            motorBusyTimeout_msec = ProbeBusyTimeout *TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeReagent:
        {
            motorName = "reagent_motor";                            // node names are NOT the same as the return for motor type name!
            params.UnitsPerRev = 320000;                            // 32 mm /revolution based on 16 tooth gear and 2mm pitch
            params.GearheadRatio = 1;
            params.Deadband = 1000;
            params.InvertedDirection = 1;
			params.HoldVoltageDivide = 300000;
			params.profileRegs.Acceleration = 640000;
            params.profileRegs.Deceleration = 640000;
            params.profileRegs.MaxSpeed = 320000;
            params.MaxTravelPosition = ReagentArmMaxTravel;
			params.MaxMoveRetries = 0;
			motorBusyTimeout_msec = ReagentBusyTimeout * TIMEOUT_FACTOR;
			break;
        }

        case MotorTypeId::MotorTypeFocus:
        {
            motorName = "focus_motor";                              // node names are NOT the same as the return for motor type name!
            params.UnitsPerRev = 3000;                              // 0.3 mm /rev from spec sheet
            params.Deadband = 4;
            params.profileRegs.Acceleration = 7500;
            params.profileRegs.Deceleration = 7500;
            params.profileRegs.MaxSpeed = 2500;
			params.OverCurrent = 1125000;
			params.HoldVoltageDivide = 32000;
			params.profileRegs.RunVoltageDivide = 500000;
            params.profileRegs.AccVoltageDivide = 500000;
            params.profileRegs.DecVoltageDivide = 500000;
            params.MaxTravelPosition = FocusMaxTravel;
            motorBusyTimeout_msec = FocusBusyTimeout * TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeRack1:
        {
            motorName = "rack1_motor";                              // node names are NOT the same as the return for motor type name!
            params.Deadband = 127;
			params.profileRegs.Acceleration = 8000000;
            params.profileRegs.Deceleration = 8000000;
            params.profileRegs.MaxSpeed = 1500000;
			params.OverCurrent = 1875000;
			params.HoldVoltageDivide = 200000;
			params.profileRegs.RunVoltageDivide = 1000000;
			params.profileRegs.AccVoltageDivide = 1000000;
			params.profileRegs.DecVoltageDivide = 1000000;
            params.MaxTravelPosition = MaxLEDRackTravel;
			params.MaxMoveRetries = 0;
			motorBusyTimeout_msec = LedRackBusyTimeout *TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeRack2:
        {
            motorName = "rack2_motor";                              // node names are NOT the same as the return for motor type name!
            params.Deadband = 127;
			params.profileRegs.Acceleration = 8000000;
            params.profileRegs.Deceleration = 8000000;
            params.profileRegs.MaxSpeed = 1500000;
			params.OverCurrent = 1875000;
			params.HoldVoltageDivide = 200000;
			params.profileRegs.RunVoltageDivide = 1000000;
			params.profileRegs.AccVoltageDivide = 1000000;
			params.profileRegs.DecVoltageDivide = 1000000;
            params.MaxTravelPosition = MaxLEDRackTravel;
			params.MaxMoveRetries = 0;
			motorBusyTimeout_msec = LedRackBusyTimeout * TIMEOUT_FACTOR;
            break;
        }

        case MotorTypeId::MotorTypeUnknown:
        default:
        {
            motorName = "";                                         // set the name to empty length to signal processing below...
            break;
        }
    }

	if ( (cfgNode) && ( motorName.length() > 0 ) )                  // motor type name is known and config file tree is not empty; update the motor with specifics
    {
        // update the params from the external info file...
        motorNode = cfgNode->get_child_optional(motorName);
        if ( !motorNode )
        {
            Logger::L().Log (GetModuleName(), severity_level::debug1, boost::str(boost::format("Error parsing configuration file : '%s': section not found!") % motorName));
        }
        else
        {
            paramNode = cfgNode;
            motorNodeName = motorName;
            // get the params from the external info file...
            ReadMotorParamsFromInfo(params, motorNode);             // update the defaults based on the info file
        }
    }

    UpdateDefaultParams( &params );                                 // copy to the default config parameters object for this motor
    SetAllParams();                                                 // copy the default params to the default register set

	if ( apply )
	{
		auto onReinitMotorDefaultsComplete = [this, cb](bool status) -> void
		{ 
			updateHost(cb, status);
		};
		// update the entire set of parameters from the current defaults, and apply them to the specific motor;
		ReinitializeMotorDefaults(onReinitMotorDefaultsComplete, dfltMotorRegs_ );
	}
	else
	{
		updateHost(cb, true);
	}
}

void MotorBase::UpdateParamsFromRegs(MotorCfgParams * pMotorCfg, MotorRegisters * pRegs)
{
    void * regAddr;
	    
	regAddr = (void *)&(pRegs->ProbeStopPosition);
    memcpy(pMotorCfg, regAddr, sizeof(MotorCfgParams));                         // copy to register set object for this motor
}

void MotorBase::UpdateRegsFromParams(MotorRegisters * pRegs, MotorCfgParams * pMotorCfg)
{
    void * regAddr;

    regAddr = ( void * )pRegs;
	regAddr = (void *)&(pRegs->ProbeStopPosition);
    memcpy(regAddr, pMotorCfg, sizeof(MotorCfgParams));                         // copy to register set object for this motor
}

// returns the motor register base address for the supplied Id, or 0 (MotorTypeRegAddrUnknown) if Id is not valid
uint32_t MotorBase::GetMotorAddrForId(uint32_t motorId)
{
    switch ( motorId )
    {
        case MotorTypeId::MotorTypeProbe: return MotorTypeProbeBaseRegAddr;
        case MotorTypeId::MotorTypeRadius:return MotorTypeRadiusBaseRegAddr;
        case MotorTypeId::MotorTypeTheta: return MotorTypeThetaBaseRegAddr;
        case MotorTypeId::MotorTypeFocus: return MotorTypeFocusBaseRegAddr;
        case MotorTypeId::MotorTypeReagent: return MotorTypeReagentBaseRegAddr;
        case MotorTypeId::MotorTypeRack1: return MotorTypeRack1BaseRegAddr;
        case MotorTypeId::MotorTypeRack2: return MotorTypeRack2BaseRegAddr;
        default:
        {
            std::string errStr = boost::str(boost::format("No / Unrecognized motor Id specified :  %d") % motorId);
            Logger::L().Log (MODULENAME, severity_level::debug1, errStr);
			return MotorTypeRegAddrUnknown;
        }
    }
	return MotorTypeRegAddrUnknown;
}

// returns the motor Id for the supplied motor register base address, or 0 if the supplied address is not valid
uint32_t MotorBase::GetMotorIdForAddr(uint32_t motorAddr)
{
    switch ( motorAddr )
    {
        case MotorTypeRegAddr::MotorTypeProbeBaseRegAddr:   return MotorTypeProbe;
        case MotorTypeRegAddr::MotorTypeRadiusBaseRegAddr:  return MotorTypeRadius;
        case MotorTypeRegAddr::MotorTypeThetaBaseRegAddr:   return MotorTypeTheta;
        case MotorTypeRegAddr::MotorTypeFocusBaseRegAddr:   return MotorTypeFocus;
        case MotorTypeRegAddr::MotorTypeReagentBaseRegAddr: return MotorTypeReagent;
        case MotorTypeRegAddr::MotorTypeRack1BaseRegAddr:   return MotorTypeRack1;
        case MotorTypeRegAddr::MotorTypeRack2BaseRegAddr:   return MotorTypeRack2;
        default:
        {
            std::string errStr = boost::str(boost::format("No / Unrecognized motor address specified :  0x%08X") % motorAddr);
            Logger::L().Log (MODULENAME, severity_level::debug1, errStr);
			return MotorTypeUnknown;
        }
    }

	// Should not reach here.
    return MotorTypeUnknown;
}

// validate the supplied motor id
// returns true if valid, false otherwise;
// does not validate the id against the current motor object
bool MotorBase::MotorIdValid(uint32_t motorId)
{
    switch ( motorId )
    {
        case MotorTypeRadius:
        case MotorTypeTheta:
        case MotorTypeProbe:
        case MotorTypeFocus:
        case MotorTypeReagent:
        case MotorTypeRack1:
        case MotorTypeRack2:
        {
            return true;
        }
        case MotorTypeIllegal:
        case MotorTypeUnknown:
        default:
        {
            return false;
        }
    }

	// Should not reach here.
    return false;
}

// validate the supplied motor register base address
// returns true if valid, false otherwise;
// does not validate the address against the current motor object
bool MotorBase::MotorRegAddrValid( MotorTypeId type, uint32_t regAddr )
{
	uint32_t lower;
    uint32_t upper;

    switch ( type )
    {
        case MotorTypeRadius:
            lower = MotorTypeRadiusBaseRegAddr;
            break;
        case MotorTypeTheta:
            lower = MotorTypeThetaBaseRegAddr;
            break;
        case MotorTypeProbe:
            lower = MotorTypeProbeBaseRegAddr;
            break;
        case MotorTypeFocus:
            lower = MotorTypeFocusBaseRegAddr;
            break;
        case MotorTypeReagent:
            lower = MotorTypeReagentBaseRegAddr;
            break;
        case MotorTypeRack1:
            lower = MotorTypeRack1BaseRegAddr;
            break;
        case MotorTypeRack2:
            lower = MotorTypeRack2BaseRegAddr;
            break;
        case MotorTypeIllegal:
        case MotorTypeUnknown:
        default:
        {
			return false;
        }
    }
	upper = lower + MOTOR_REG_SIZE - 1;

    return ( ( regAddr >= lower ) && ( regAddr <= upper ) );
}

void MotorBase::UpdateMotorPositionStatus( const boost::system::error_code error)
{
    std::string motorStr = boost::str( boost::format( "%s (motor type %d)" ) % GetMotorTypeAsString( motorTypeId_ ) % motorTypeId_);
	Logger::L().Log (GetModuleName(), severity_level::debug2, "UpdateMotorPositionStatus: <enter, " + motorStr + ">" );

    if ( error == boost::asio::error::operation_aborted )
    {
		Logger::L().Log (GetModuleName(), severity_level::debug2, "Position status update timer cancelled: " + motorStr );
        return;
    }

	if ( closing )
	{
		Logger::L().Log (GetModuleName(), severity_level::debug2, "UpdateMotorPositionStatus: " + motorStr + " <closing: exit>" );
		return;
	}

	//TO DO Shall we keep a AsyncCommandHelper::sync_wait
	RdMotorPositionStatus([](bool status) {}, error);

	// Schedule next try.
	if (posUpdateInterval_ms)
	{
		// start the timer to do automatic periodic status updates...
		posUpdTimer.expires_from_now(boost::posix_time::milliseconds(posUpdateInterval_ms.get()));     // should cancel any pending async operations
		posUpdTimer.async_wait([=](boost::system::error_code ec)->void { this->UpdateMotorPositionStatus(ec); });        // restart the periodic update timer as time from now...
	}
	Logger::L().Log (GetModuleName(), severity_level::debug2, "UpdateMotorPositionStatus: " + motorStr + " <exit>" );
}

void MotorBase::RdMotorPositionStatus( std::function<void( bool )> cb, const boost::system::error_code error )
{
	HAWKEYE_ASSERT (MODULENAME, cb);

	std::string motorStr = boost::str(boost::format("%s (motor type %d)") % GetMotorTypeAsString(motorTypeId_) % motorTypeId_);
	Logger::L().Log (GetModuleName(), severity_level::debug2, "RdMotorPositionStatus: <enter, " + motorStr + ">" );

	auto onPositionUpdateComplete = [ this, cb, motorStr ]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug3, "RdMotorPosition::onPositionUpdateComplete: " + motorStr + " <callback>");

		bool success = ( cbData.status == ControllerBoardOperation::Success ) && ( cbData.rxBuf );
		if ( !success )
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("RdMotorPositionStatus: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			Logger::L().Log (GetModuleName(), severity_level::error, "RdMotorPositionStatus: unable to read motor position!" );
			HandleRdCallbackError( cbData );
		}
		else
		{
			// ONLY HANDLE POSITION INFORMATION
			MotorRegisterValues* p = ( MotorRegisterValues* ) cbData.rxBuf->data();

			motorRegs_.ErrorCode = p->ErrorCode;
			motorRegs_.Position = p->Position;
			motorRegs_.HomedStatus = p->HomedStatus;
			motorRegs_.ProbeStopPosition = p->ProbeStopPosition;

			if (Logger::L().IsOfInterest(severity_level::debug1)) {

				// Only log when there is a change in the data.
				if (motorRegisterValuesCache_.ErrorCode != motorRegs_.ErrorCode ||
					motorRegisterValuesCache_.Position != motorRegs_.Position ||
					motorRegisterValuesCache_.HomedStatus != motorRegs_.HomedStatus ||
					motorRegisterValuesCache_.ProbeStopPosition != motorRegs_.ProbeStopPosition) {

					Logger::L().Log (GetModuleName(), severity_level::debug1,
						boost::str (boost::format ("RdMotorPositionStatus: %s, pos: %d, isHomed: %s, probeStopPosition: %d")
							% motorStr
							% motorRegs_.Position
							% (motorRegs_.HomedStatus ? "true" : "false")
							% motorRegs_.ProbeStopPosition));

					motorRegisterValuesCache_.ErrorCode = motorRegs_.ErrorCode;
					motorRegisterValuesCache_.Position = motorRegs_.Position;
					motorRegisterValuesCache_.HomedStatus = motorRegs_.HomedStatus;
					motorRegisterValuesCache_.ProbeStopPosition = motorRegs_.ProbeStopPosition;
				}
			}
		}

		pCBOService_->enqueueInternal( cb, success );

	};

	auto tid = pCBOService_->CBO()->Query(MotorReadStatusOperation((RegisterIds)(uint32_t)motorRegsAddr_), onPositionUpdateComplete);
	if (tid)
	{
		Logger::L().Log (GetModuleName(), severity_level::debug2, boost::str(boost::format("ReadStatus, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "RdMotorPosition: CBO Query failed!" );
		pCBOService_->enqueueInternal (cb, false);
	}
}

// timer-called function for updating the entire motor register set
void MotorBase::UpdateMotorRegisters(const boost::system::error_code error)
{
	std::string idStr = boost::str( boost::format( "%s (motor type %d)" ) % GetMotorTypeAsString( motorTypeId_ ) % motorTypeId_);
	//Logger::L().Log (GetModuleName(), severity_level::debug3, "UpdateMotorRegisters: " + idStr + " <enter>" );

	if ( error == boost::asio::error::operation_aborted )
	{
		Logger::L().Log (GetModuleName(), severity_level::debug3, "UpdateMotorRegisters: cancelling register update timer:  " + idStr + " <exit>" );
		return;
	}

	if (closing)
	{
		Logger::L().Log (GetModuleName(), severity_level::notification, "UpdateMotorRegisters: <exit> closing" );
		return;
	}

	RdMotorRegisters();

	// set timer to do automatic periodic status updates...
	Logger::L().Log (GetModuleName(), severity_level::debug3, "UpdateMotorRegisters:  timer start" );
	regUpdTimer.expires_from_now( boost::posix_time::milliseconds( regUpdateInterval_ms ) );					// should cancel any pending async operations
	regUpdTimer.async_wait( std::bind( &MotorBase::UpdateMotorRegisters, this, std::placeholders::_1 ) );		// restart the periodic update timer as time from now...

	Logger::L().Log (GetModuleName(), severity_level::debug3, "UpdateMotorRegisters: <exit>" );
}

// schedules a task to perform the read of the complete register set for a motor
void MotorBase::RdMotorRegisters(void)
{
	//std::string idStr = boost::str( boost::format( "%s (motor type %d)" ) % GetMotorTypeAsString( motorTypeId_ ) % motorTypeId_);
	//Logger::L().Log (GetModuleName(), severity_level::debug3, "RdMotorRegisters: " + idStr + " <enter>" );

	auto onReadComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug3, "RdMotorRegisters::onReadComplete: <callback>" );

		if ( ( cbData.status != ControllerBoardOperation::Success ) || ( !cbData.rxBuf ) )
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("RdMotorRegisters: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			HandleRdCallbackError( cbData );
			return;
		}

		cbData.FromRx( (void *)&motorRegs_, sizeof( MotorRegisters ) );
		initComplete = true;

		HandleCallbackStatus( cbData, false );
	};

	if ( !closing )
	{
		auto tid = pCBOService_->CBO()->Query(MotorReadFullRegisterOperation(static_cast<RegisterIds>(motorRegsAddr_)), onReadComplete);
		if (tid)
		{
			Logger::L().Log (GetModuleName(), severity_level::debug2, boost::str(boost::format("ReadFullRegisters, task %d") % (*tid)));
		}
		else
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "RdMotorRegisters: CBO Query failed!" );
		}
	}

	//Logger::L().Log (GetModuleName(), severity_level::debug3, "RdMotorRegisters: <exit>" );
}

// handle the complete motor register set read operation completion callback
bool MotorBase::HandleCallbackStatus( ControllerBoardOperation::CallbackData_t cbData, bool regWrite )
{
	bool retStatus = true;
	std::string logStr;

	if (cbData.errorStatus.isSet(ErrorStatus::HostComm))
    {
		if (regWrite)
		{
			logStr.append("  - write callback ");
		}
		else
		{
			logStr.append("  - register read ");
		}
		logStr.append(boost::str(boost::format(" returned Host Comm error. Motor error code register: 0x%08X") % cbData.errorCode));
		triggerErrorReportCb (BuildErrorInstance(
			instrument_error::controller_general_hostcommerror, 
			instrument_error::cntrlr_general_instance::none, 
			instrument_error::severity_level::warning));
		retStatus = false;
	}

	if ((cbData.errorCode != 0) && (cbData.errorCode != ErrorType::StateMachineBusy))
	{
		ErrorType err(cbData.errorCode);

		if ( err.isSet( ErrorType::TimeoutError ) )
		{
			logStr.append(boost::str(boost::format(" returned timeout error, error code register: 0x%08X") % cbData.errorCode));
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::warning));
			retStatus = false;
		}

		if ( err.isSet( ErrorType::HardwareError ) )
		{
			logStr.append(boost::str(boost::format(" returned motor driver error, error code register: 0x%08X") % cbData.errorCode));
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_drivererror, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::warning));
			retStatus = false;
		}

		if ( err.isSet( ErrorType::CommandFailedError ) )
		{
			logStr.append(boost::str(boost::format("returned command failure, error code register: 0x%08X") % cbData.errorCode));
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_drivererror, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::warning));
			retStatus = false;
		}
	}

	std::string statusStr = cbData.errorStatus.getAsString();

	logStr.append(boost::str(boost::format("\n\tBoardStatus: 0x%04X ( %s )") % cbData.errorStatus.get() % statusStr.c_str()));
	if (!retStatus)
    {
        Logger::L().Log (GetModuleName(), severity_level::debug1, logStr );
    }
    else
    {
        Logger::L().Log (GetModuleName(), severity_level::debug2, logStr );
    }

    return retStatus;
}

// handle error conditions returned by a read operation completion callback
void MotorBase::HandleRdCallbackError( ControllerBoardOperation::CallbackData_t cbData )
{
    std::string errStr = "HandleRdCallbackError:: ";

    if ( cbData.status == ControllerBoardOperation::Error )
    {
		uint32_t errCode = cbData.errorCode;
		ErrorType err( errCode );

		if ( err.isSet( ErrorType::TimeoutError ) )
		{
			errStr.append( boost::str( boost::format( " returned timeout error, error code register: 0x%08X \r\n\t" ) % errCode ) );
		}

		if ( err.isSet( ErrorType::HardwareError ) )
		{
			errStr.append( boost::str( boost::format( " returned motor driver error, error code register: 0x%08X \r\n\t" ) % errCode ) );
		}

		if ( err.isSet( ErrorType::CommandFailedError ) )
		{
			errStr.append( boost::str( boost::format( " returned command failure, error code register: 0x%08X \r\n\t" ) % errCode ) );
		}

//		errStr.append( boost::str( boost::format( " returned error: %s" ) % err.getAsString() ) );
    }
	else if ( cbData.status == ControllerBoardOperation::Timeout )
	{
		errStr.append( boost::str( boost::format( " returned timeout error \r\n\t" ) ) );
	}
	else if ( !cbData.rxBuf )
    {
        errStr.append( " returned error: no receive buffer!" );
    }
    else if ( !cbData.bytesRead )
    {
        errStr.append( " returned error: no data received!" );
    }
    else
    {
        errStr.append( "Read Callback unknown error!" );
    }
    Logger::L().Log (GetModuleName(), severity_level::error, errStr);
}

void MotorBase::HandleConnectOrAddressError( uint32_t addr, bool isMotorAddr, bool isAddrNeeded, void * pData, std::string & logStr )
{
    if ( isMotorAddr )
    {
        if ( motorRegsAddr_ == MotorTypeRegAddrUnknown )
        {
            logStr.append( boost::str( boost::format( " Error: unknown motor register address: %u" ) % motorRegsAddr_ ) );
        }
        if ( !MotorRegAddrValid( motorTypeId_, addr ) )
        {
            logStr.append( boost::str( boost::format( " Error: unknown motor register address '%u' for %s (motor type %d)" ) % addr % GetMotorTypeAsString( motorTypeId_ ) % motorTypeId_) );
        }
        if ( isAddrNeeded )
        {
            if ( !pData )
            {
                logStr.append( boost::str( boost::format( " Error: invalid register address pointer '%x'" ) % pData ) );
            }
        }
    }
    else
    {
        if ( !REG_ADDR_VALID( addr ) )
        {
            logStr.append( boost::str( boost::format( " Error: invalid register address '%u'" ) % addr ) );
        }
        if ( isAddrNeeded )
        {
            if ( !pData )
            {
                logStr.append( " Error: bad data pointer" );
            }
        }
    }

}

// writes a single 32-bit value to the address specified
void MotorBase::WrtDataValue( uint32_t addr, uint32_t value, boost::optional<std::function<void(bool)>> opCallback)
{
	Logger::L().Log (GetModuleName(), severity_level::debug3, "WrtDataValue: <enter>" );

	if (!REG_ADDR_VALID(addr))
	{
		std::string logStr = "WrtDataValue: ";
		HandleConnectOrAddressError(addr, false, false, nullptr, logStr);
		Logger::L().Log (GetModuleName(), severity_level::error, logStr );
		if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), false);
		}
		return;
	}

	auto onWrtDataValueComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onWrtDataValueComplete: <callback>" );

		if ( cbData.status != ControllerBoardOperation::Success )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "onWrtDataValueComplete: operation failed" );
		}

		if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), cbData.status == ControllerBoardOperation::Success);
		}
	};

	const uint32_t timeout_sec = 1;
	if ( !executeWriteOperation( MotorWriteRegisterValueOperation( static_cast<RegisterIds>( addr ), value ),
						  timeout_sec, onWrtDataValueComplete ) )
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "WrtDataValue pCbo Execute failed" );
		if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), false);
		}
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug3, "WrtDataValue: <exit>" );
}

// writes a block of data to a specified motor register offset address
void MotorBase::WrtDataBlock( std::function<void( bool )> cb, uint32_t addr, void * dataAddr, uint32_t blklen )
{
	Logger::L().Log (GetModuleName(), severity_level::debug3, "WrtDataBlock: <enter>" );

	if ( !REG_ADDR_VALID( addr ) )
	{
		updateHost( cb, false );
		std::string logStr = "WrtDataBlock: ";
		HandleConnectOrAddressError( addr, false, false, nullptr, logStr );
		Logger::L().Log (GetModuleName(), severity_level::error, logStr );
		return;
	}

	auto onWrtDataBlockComplete = [ = ]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onWrtDataBlockComplete: <callback>" );

		bool status = cbData.status == ControllerBoardOperation::Success;
		if ( !status )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "onWrtDataBlockComplete: block write operation failed" );
		}
		updateHost( cb, status );
	};

	const uint32_t timeout_sec = 1;
	if ( !executeWriteOperation( MotorWriteRegisterBlockOperation( static_cast<RegisterIds>( addr ), dataAddr, blklen ),
											  timeout_sec, onWrtDataBlockComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "WrtDataBlock: <exit> pCbo Execute failed" );
		return;
	}
}

// decodes error code bit pattern to extract motor busy indicators
// for the current motor object using the object motorTypeId_ value
bool MotorBase::MotorBusy(BoardStatus brdstatus, uint32_t mtype)
{
    bool motorBusy = false;

    if ( MotorIdValid(mtype) )
    {
        BoardStatus::StatusBit statusBit = BoardStatus::DoNotCheckAnyBit;

        switch ( mtype )
        {
            case MotorTypeRadius:
            {
                statusBit = BoardStatus::RadiusMotorBusy;
                break;
            }
            case MotorTypeTheta:
            {
                statusBit = BoardStatus::ThetaMotorBusy;
                break;
            }
            case MotorTypeProbe:
            {
                statusBit = BoardStatus::ProbeMotorBusy;
                break;
            }
            case MotorTypeReagent:
            {
                statusBit = BoardStatus::ReagentMotorBusy;
                break;
            }
            case MotorTypeFocus:
            {
                statusBit = BoardStatus::FocusMotorBusy;
                break;
            }
            case MotorTypeRack1:
            {
                statusBit = BoardStatus::Rack1MotorBusy;
                break;
            }
            case MotorTypeRack2:
            {
                statusBit = BoardStatus::Rack2MotorBusy;
                break;
            }
            default:
                break;
        }
        motorBusy = brdstatus.isSet( statusBit );
    }
    return motorBusy;
}

// decodes bit pattern of the ErrorStatus registers to extract the motor error
// bit for the current motor object using the object motorTypeId_ value
bool MotorBase::MotorStatusError( ErrorStatus estatus, uint32_t mtype )
{
    bool motorError = false;

    if ( MotorIdValid(mtype) )
    {
        ErrorStatus::StatusBit motorBit = ErrorStatus::DoNotCheckAnyBit;

        switch ( mtype )
        {
            case MotorTypeRadius:
            {
                motorBit = ErrorStatus::RadiusMotor;
                break;
            }
            case MotorTypeTheta:
            {
                motorBit = ErrorStatus::ThetaMotor;
                break;
            }
            case MotorTypeProbe:
            {
                motorBit = ErrorStatus::ProbeMotor;
                break;
            }
            case MotorTypeReagent:
            {
                motorBit = ErrorStatus::ReagentMotor;
                break;
            }
            case MotorTypeFocus:
            {
                motorBit = ErrorStatus::FocusMotor;
                break;
            }
            case MotorTypeRack1:
            {
                motorBit = ErrorStatus::Rack1Motor;
                break;
            }
            case MotorTypeRack2:
            {
                motorBit = ErrorStatus::Rack2Motor;
                break;
            }
            default:
                break;
        }
        motorError = estatus.isSet( motorBit );
    }
    return motorError;
}

// decodes bit pattern of the ErrorStatus registers to extract the motor error
// bit for the current motor object using the object motorTypeId_ value
bool MotorBase::MotorHomeSwitch( SignalStatus sigstatus, uint32_t mtype )
{
    bool homeSwitch = false;

    if ( MotorIdValid( mtype ) )
    {
        SignalStatus::SignalStatusBits signalBit = SignalStatus::DoNotCheckAnyBit;

        switch ( mtype )
        {
            case MotorTypeRadius:
            {
                signalBit = SignalStatus::RadiusMotorHome;
                break;
            }
            case MotorTypeTheta:
            {
                signalBit = SignalStatus::ThetaMotorHome;
                break;
            }
            case MotorTypeProbe:
            {
                signalBit = SignalStatus::ProbeMotorHome;
                break;
            }
            case MotorTypeReagent:
            {
                signalBit = SignalStatus::ReagentMotorHome;
                break;
            }
            case MotorTypeFocus:
            {
                signalBit = SignalStatus::FocusMotorHome;
                break;
            }
            case MotorTypeRack1:
            {
                signalBit = SignalStatus::Rack1MotorHome;
                break;
            }
            case MotorTypeRack2:
            {
                signalBit = SignalStatus::Rack2MotorHome;
                break;
            }
            default:
                break;
        }
        homeSwitch = sigstatus.isSet( signalBit );
    }
    return homeSwitch;
}

// decodes bit pattern of the ErrorStatus registers to extract the motor error
// bit for the current motor object using the object motorTypeId_ value
bool MotorBase::MotorLimitSwitch( SignalStatus sigstatus, uint32_t mtype )
{
    bool limitSwitch = false;

    if ( MotorIdValid( mtype ) )
    {
        SignalStatus::SignalStatusBits signalBit = SignalStatus::DoNotCheckAnyBit;

        switch ( mtype )
        {
            case MotorTypeRadius:
            {
                signalBit = SignalStatus::RadiusMotorLimit;
                break;
            }
            case MotorTypeTheta:
            {
                signalBit = SignalStatus::ThetaMotorLimit;
                break;
            }
            case MotorTypeProbe:
            {
                signalBit = SignalStatus::ProbeMotorLimit;
                break;
            }
            case MotorTypeReagent:
            {
                signalBit = SignalStatus::ReagentMotorLimit;
                break;
            }
            case MotorTypeFocus:
            {
                signalBit = SignalStatus::FocusMotorLimit;
                break;
            }
            case MotorTypeRack1:
            {
                signalBit = SignalStatus::Rack1MotorLimit;
                break;
            }
            case MotorTypeRack2:
            {
                signalBit = SignalStatus::Rack2MotorLimit;
                break;
            }
            default:
                break;
        }
        limitSwitch = sigstatus.isSet( signalBit );
    }
    return limitSwitch;
}

// Check if the current position is within the specified tolerance.
bool MotorBase::PosInTolerance( int32_t currentPos, int32_t tgtPos, int32_t tolerance )
{
    bool inLim = false;
    int32_t lowLim = tgtPos - tolerance;
    int32_t hiLim = tgtPos + tolerance;

    // check a position for coincidence with a supplied target using supplied tolerances
    if ( ( currentPos >= lowLim ) && ( currentPos <= hiLim ) )
    {
        inLim = true;
    }

	Logger::L().Log (MODULENAME, severity_level::debug2,
		boost::str (boost::format ("PosInTolerance: posOk: %s with curpos: %d, targetPos: %d, tolerance: %d")
			% (inLim ? "true" : "false")
			% currentPos
			% tgtPos
			% tolerance));

    return inLim;
}


////////////////////////////////////////////////////////////////////////////////
// Motor Object Configuration
////////////////////////////////////////////////////////////////////////////////

// initialize motor variables from external configuration file;
// read motor controller configuration, and starts motor status update timer
// returns true if serial port is opened successfully
void MotorBase::Init(std::function<void(bool)> callback, t_pPTree cfgTree, t_opPTree cfgNode, bool apply)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

    boost::system::error_code ec;

    if ( cfgTree )
    {
        configTree = cfgTree;
    }

    initComplete = false;

	// DO NOT CONTINUE WITHOUT A VALID ADDRESS / MOTOR TYPE.  OMG.WTF.BBQ.
	if (!MotorIdValid(motorTypeId_))
	{
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

    uint32_t addr = GetMotorAddrForId( motorTypeId_ );

	// DO NOT CONTINUE WITHOUT A VALID ADDRESS / MOTOR TYPE.  OMG.WTF.BBQ.
	if (!MotorRegAddrValid(motorTypeId_, addr))
	{
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	if (addr != motorRegsAddr_)
	{
		motorRegsAddr_ = (MotorTypeRegAddr)addr;
	}

	auto onInitDefaultParamsComplete = [this, callback, apply](bool status) -> void
	{ 
		Logger::L().Log (GetModuleName(), severity_level::debug1,
			boost::str (boost::format("onInitDefaultParamsComplete: %s") % (status ? "success" : "fail")));
		if (!status)
		{
			updateHost(callback, false);
			return;
		}

		// start the timer to do automatic periodic status updates...
		regUpdTimer.expires_from_now(boost::posix_time::milliseconds(1));     // should cancel any pending async operations
		regUpdTimer.async_wait([=](boost::system::error_code ec)->void
		{
			this->UpdateMotorRegisters(ec);
		});       // restart the periodic update timer as time from now...

		if (posUpdateInterval_ms)
		{
			// start the timer to do automatic periodic status updates...
			posUpdTimer.expires_from_now(boost::posix_time::milliseconds(posUpdateInterval_ms.get()));     // should cancel any pending async operations
			posUpdTimer.async_wait([=](boost::system::error_code ec)->void
			{
				this->UpdateMotorPositionStatus(ec);
			});        // restart the periodic update timer as time from now...
		}

		updateHost(callback, true);
	};

	// set default params from the internal expected values and external config file
	InitDefaultParams(onInitDefaultParamsComplete, motorTypeId_, cfgTree, cfgNode, apply );
}

// stops current asynchronous operations; stops update timer
//  
void MotorBase::Quit(void)
{
    boost::system::error_code ecReg;
    boost::system::error_code ecPos;
    boost::system::error_code ecStop;

    closing = true;

    if ( IsBusy() )
    {
		auto onStopCmdComplete( [this]( bool status ) -> void
		{
			std::string cboutput = "Quit/onStopCmdComplete: <callback>";
			if ( !status )
			{
				Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nQuit: unable to stop motor!" );
			}
			else
				Logger::L().Log (GetModuleName(), severity_level::debug1, cboutput);

		} );

		Stop( onStopCmdComplete, false, 1000 );
	}

    regUpdTimer.cancel( ecReg );
    posUpdTimer.cancel( ecPos );  

    if ( updateConfig )
    {
        WriteMotorConfig();
    }

    configTree.reset();
    paramNode.reset();
    motorNode.reset();
}

// Higher level API that checks if the current position is within the specified tolerance.
bool MotorBase::PosAtTgt(int32_t currentPos, int32_t tgtPos, int32_t tolerance, bool useDeadband) const
{
    if ( tolerance == 0 )
    {
        tolerance = positionTolerance;
		Logger::L().Log (GetModuleName(), severity_level::debug3, "PosAtTgt: positionTolerance = " + std::to_string(positionTolerance));
    }

    if ( useDeadband )
    {
        tolerance = GetDeadband();
		Logger::L().Log (GetModuleName(), severity_level::debug3, "PosAtTgt: GetDeadband = " + std::to_string(tolerance));
    }

    return PosInTolerance( currentPos, tgtPos, tolerance );
}

void MotorBase::SetPosTolerance( int32_t tolerance )
{
    positionTolerance = tolerance;
}

// set the externally configurable motor startup timeout value
void MotorBase::SetMotorTimeouts( int32_t startTimeout_msec, int32_t busyTimeout_msec )
{
    motorStartTimeout_msec = startTimeout_msec;
    motorBusyTimeout_msec = busyTimeout_msec;
}

void MotorBase::SetMotorPositionUpdateInterval( boost::optional<uint32_t> interval )
{
	if (interval && (interval.get() != 0))
	{
		posUpdateInterval_ms = interval;
		// set timer to do automatic periodic status updates...
		// Set the timer to 1ms here in order to get the updates started quickly.
		posUpdTimer.expires_from_now(boost::posix_time::milliseconds (1));						// should cancel any pending async operations
		posUpdTimer.async_wait(std::bind(&MotorBase::UpdateMotorPositionStatus, this, std::placeholders::_1));		// restart the periodic update timer as time from now...
	}
	else
	{
		posUpdTimer.cancel();
	}
}

void MotorBase::registerToErrorReportCb( std::function<void( uint32_t )> cb )
{
	errorReportingCb_ = cb;
}

void MotorBase::triggerErrorReportCb( uint32_t errorCode )
{
#ifdef NO_ERROR_REPORT
	return;
#endif

	if ( errorReportingCb_ )
	{
		// Log the error occurred in log file but don't let host know yet about error
		ReportSystemError::DecodeAndLogErrorCode( errorCode );
		pCBOService_->enqueueInternal( [this, errorCode]() -> void
		{
			this->errorReportingCb_ (errorCode);
		} );
	}
	else
	{
		ReportSystemError::Instance().ReportError (errorCode);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Motor Operating Configuration
////////////////////////////////////////////////////////////////////////////////

// configure the motor object for a specific motor type as specified
// initializes the internal motor register address for the motor type
// returns the motor type ID on success, or -1 (MotorTypeIllegal) on failure
MotorTypeId MotorBase::SetMotorType (const MotorTypeId mtype)
{
    MotorTypeId setType = mtype;

    switch (mtype)
    {
        case MotorTypeUnknown:
        case MotorTypeRadius:
        case MotorTypeTheta:
        case MotorTypeProbe:
        case MotorTypeReagent:
        case MotorTypeFocus:
        case MotorTypeRack1:
        case MotorTypeRack2:
            break;

        default:
            setType = MotorTypeIllegal;
            break;
    }

	if (setType != MotorTypeIllegal)
    {
        motorTypeId_ = mtype;
        uint32_t addr = MotorTypeRegAddrUnknown;

        addr = GetMotorAddrForId(motorTypeId_);
        if ( addr )
        {
            motorRegsAddr_ = (MotorTypeRegAddr)addr;
            RdMotorRegisters();
        }
    }

    return setType;
}

std::string MotorBase::GetModuleName() const
{
	return MODULENAME + GetMotorNameAsString(motorTypeId_);
}

// returns the motor object motor type ID; may not yet be initialized to a valid motor type
MotorTypeId MotorBase::GetMotorType (void) const
{
    return motorTypeId_;
}

// returns the string representation for the supplied motor type ID;
// returns 'Unknown' for unrecognized motor type Ids
void MotorBase::GetMotorTypeAsString(const MotorTypeId mtype, std::string & typeStr)
{
    typeStr = UnknownMotorNameStr;

	GetMotorNameAsString(mtype, typeStr);

	typeStr.append(" motor");
}

// returns the string representation for the supplied motor type ID;
// returns 'Unknown' for unrecognized motor type Ids
std::string & MotorBase::GetMotorTypeAsString(const MotorTypeId mtype) const
{
	static std::string motorTypeStr = UnknownMotorNameStr;

	GetMotorNameAsString(mtype, motorTypeStr);

	motorTypeStr.append(" motor");

	return motorTypeStr;
}

// returns the string representation for the supplied motor type ID;
// returns 'Unknown' for unrecognized motor type Ids
void MotorBase::GetMotorNameAsString(const MotorTypeId mtype, std::string & nameStr)
{
	nameStr = UnknownMotorNameStr;

	switch (mtype)
	{
		case MotorTypeProbe:
		{
			nameStr = ProbeMotorNameStr;
			break;
		}

		case MotorTypeRadius:
		{
			nameStr = RadiusMotorNameStr;
			break;
		}

		case MotorTypeTheta:
		{
			nameStr = ThetaMotorNameStr;
			break;
		}

		case MotorTypeFocus:
		{
			nameStr = FocusMotorNameStr;
			break;
		}

		case MotorTypeReagent:
		{
			nameStr = ReagentMotorNameStr;
			break;
		}

		case MotorTypeRack1:
		{
			nameStr = Rack1MotorNameStr;
			break;
		}

		case MotorTypeRack2:
		{
			nameStr = Rack2MotorNameStr;
			break;
		}

		case MotorTypeUnknown:
		default:
		{
			break;
		}
	}
}

// returns the string representation for the supplied motor type ID;
// returns 'Unknown' for unrecognized motor type Ids
std::string & MotorBase::GetMotorNameAsString(const MotorTypeId mtype) const
{
	static std::string motorNameStr = UnknownMotorNameStr;

	GetMotorNameAsString(mtype, motorNameStr);

	return motorNameStr;
}



////////////////////////////////////////////////////////////////////////////////
// Motor Status
////////////////////////////////////////////////////////////////////////////////

// returns true if motor is at home position, false otherwise
bool MotorBase::IsHome(void)
{
    return HomeLimitSwitch();
}

// returns true if motor is at home position, false otherwise
bool MotorBase::IsDown( void )
{
	return DownLimitSwitch();
}

bool MotorBase::HomeLimitSwitch( void )
{
	SignalStatus switchStatus = pCBOService_->CBO()->GetSignalStatus().get();

	return MotorHomeSwitch( switchStatus, motorTypeId_ );
}

bool MotorBase::DownLimitSwitch( void )
{
	// Only really applies to reagent motor
	if (motorTypeId_ != MotorTypeReagent)
		return false;
    
	SignalStatus switchStatus = pCBOService_->CBO()->GetSignalStatus().get();
	return MotorLimitSwitch(switchStatus, motorTypeId_);
}

bool MotorBase::IsBusy(void) const
{
    return MotorBusy(pCBOService_->CBO()->GetBoardStatus(), motorTypeId_);
}

// retrieves the complete motor register set cached in the motor object;
// returns true if the object has been initialized with a valid motor type id and register-set address, false otherwise
bool MotorBase::GetMotorRegs(MotorRegisters & motorRegs) const
{
    bool regOk = false;

    if ( ( motorTypeId_ != MotorTypeIllegal ) &&
        ( motorTypeId_ != MotorTypeUnknown ) &&
        ( motorRegsAddr_ != MotorTypeRegAddrUnknown ) )
    {
        motorRegs = motorRegs_;
        regOk = true;
    }
    return regOk;
}

//NOTE: this is not called by any HawkeyeCore code, only called by the test applications.
// executes the read register operation to retrieve the complete motor register set from the K70;
// returns true if the object has been initialized with a valid motor type id and register-set address, false otherwise
bool MotorBase::ReadMotorRegs(MotorRegisters & motorRegs)
{
	if (!this->MotorIdValid(motorTypeId_) ||
		!this->MotorRegAddrValid(motorTypeId_, motorRegsAddr_))
		return false;

	// This will request an update, but it will return the CURRENT data!
	posUpdTimer.cancel();
	posUpdTimer.expires_from_now(boost::posix_time::milliseconds(1));     // should cancel any pending async operations
	posUpdTimer.async_wait([=](boost::system::error_code ec)->void { this->UpdateMotorPositionStatus(ec); });        // restart the periodic update timer as time from now...
    motorRegs = motorRegs_;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Motor Commands
////////////////////////////////////////////////////////////////////////////////


// attempts to clear error conditions on the K70 board...
void MotorBase::ClearErrors(std::function<void(bool)> callback,bool clr_host_comm)
{
	Logger::L().Log (GetModuleName(), severity_level::debug1, "ClearErrors:  <enter>" );

	auto onClearComplete = [this, clr_host_comm, callback]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onClearComplete: <callback>" );

		if ( cbData.status != ControllerBoardOperation::Success )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "ClearErrors: unable to clear error registers!" );
			callback(false); 
			return;
		}

		if (clr_host_comm)
		{
			WrtDataValue(RegisterIds::HostCommError, 0);
		}

		callback(true);
	};

	const uint32_t timeout_sec = 1;
	if ( !executeWriteOperation( MotorClearErrorOperation(static_cast<RegisterIds>( motorRegsAddr_ )),
	                             timeout_sec, onClearComplete) )
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "ClearErrors: CBO Execute failed!" );
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "ClearErrors: <exit>" );
}

////////////////////////////////////////////////////////////////////////////////
// Motor Properties / Parameters
//
// NOTE: the parameter setting methods for read-write or write-only parameters
//       load the new value(s) to the K70, as well as updating the cached motor
//       register value set.
//
//       The property 'getters' return the value from the cached motor register set.
//       The cached values are updated whenever a command is completed!
//
////////////////////////////////////////////////////////////////////////////////

// update the entire set of parameters; DOES NOT APPLY THEM!
// if null is passed for the source, copies from the current default parameter set
void MotorBase::SetAllParams( MotorCfgParams * pMotorCfg, MotorRegisters * pRegs )
{
    MotorRegisters regs;
    MotorCfgParams * pCfgSrc;

    if ( pMotorCfg )    // if a source is specified
    {
        pCfgSrc = pMotorCfg;
    }
    else
    {
        pCfgSrc = &dfltParams_;
    }
    // update the register set from the specified or default configuration parameter source
    UpdateRegsFromParams( &regs, pCfgSrc );

    // update the non-parameter values to initial settings...
    regs.Command = 0;
    regs.CommandParam = 0;
    regs.ErrorCode = motorRegs_.ErrorCode;
    regs.Position = motorRegs_.Position;
	regs.HomedStatus = motorRegs_.HomedStatus;

    if ( pRegs == nullptr )        // if no destination specified...
    {
        pRegs = &dfltMotorRegs_;
    }
    memcpy( pRegs, &regs, sizeof( MotorRegisters ) );             // copy to register set object for this motor
}

// write the complete register set, excluding the command and parameter registers
// sends ALL motor configuration params to the controller in a single operation
// Does NOT produce a busy bit
void MotorBase::ApplyAllParams(void)
{
	Logger::L().Log (GetModuleName(), severity_level::debug1, "ApplyAllParams: <enter>" );

	if ( !MotorIdValid( motorTypeId_ ) )
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "ApplyAllParams: invalid motor id" );
	}

	auto onApplyAllParamsComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onApplyAllParamsComplete: <callback>" );

		if ( cbData.status != ControllerBoardOperation::Success )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "onApplyAllParamsComplete: operation failed" );
		}
	};

	const uint32_t timeout_sec = 1;
	// this operation is a data block write operation, and has no 'Init' command; it should not produce a busy-bit block
	if ( !executeWriteOperation( MotorWriteFullRegisterOperation( static_cast<RegisterIds>( motorRegsAddr_ ), motorRegs_ ),
	                             timeout_sec, onApplyAllParamsComplete ) )
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "ApplyAllParams: pCbo execute failed" );
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "ApplyAllParams: <exit>" );
}

// update the entire set of default parameters; DOES NOT APPLY THEM!
// if null is passed for the source, copies from the current register values
void MotorBase::UpdateDefaultParams( MotorCfgParams * pMotorCfg )
{
    if ( pMotorCfg )
    {
        memcpy( &dfltParams_, pMotorCfg, sizeof( MotorCfgParams ) );            // copy to register set object for this motor
    }
    else
    {
        UpdateParamsFromRegs( &dfltParams_, &motorRegs_ );
    }
}

// update the entire set of parameters from the current defaults, and applies them to the specific motor;
void MotorBase::ApplyDefaultParams(void)
{
    SetAllParams();                         // apply the default params to the default motor register object...
	SetAllParams(nullptr, &motorRegs_);     // apply the default params to the standard working motor register object...

	// apply the standard params...
	// writes the complete register set, excluding the command and parameter registers
	// sends ALL motor configuration params to the controller in a single operation
	// Does NOT produce a busy bit
	ApplyAllParams();
}

////////////////////////////////////////////////////////////////////////////////
// read-write properties

// sets the home direction property
// returns the old value of the property
uint32_t MotorBase::SetHomeDirection(uint32_t homeDir, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.HomeDirection;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    addr += HomeDirectionAddr;
    dfltParams_.HomeDirection = homeDir;
    if ( apply )
    {
        WrtDataValue(addr, homeDir, opCallback);
        motorRegs_.HomeDirection = homeDir;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the home direction property
uint32_t MotorBase::GetHomeDirection(void) const
{
    return motorRegs_.HomeDirection;
}

// sets the MotorFullStepsPerRev property
// returns the old value of the property
uint32_t MotorBase::SetStepsPerRev(uint32_t count, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.MotorFullStepsPerRev;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    addr += MotorFullStepsPerRevAddr;
    dfltParams_.MotorFullStepsPerRev = count;
    if ( apply )
    {
        WrtDataValue(addr, count, opCallback);
        motorRegs_.MotorFullStepsPerRev = count;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the MotorFullStepsPerRev property
uint32_t MotorBase::GetStepsPerRev(void) const
{
    return motorRegs_.MotorFullStepsPerRev;
}

// sets the UnitsPerRev property
// returns the old value of the property
uint32_t MotorBase::SetUnitsPerRev(uint32_t count, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.UnitsPerRev;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.UnitsPerRev = count;
    addr += UnitsPerRevAddr;
    if ( apply )
    {
        WrtDataValue(addr, count, opCallback);
        motorRegs_.UnitsPerRev = count;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the UnitsPerRev property
uint32_t MotorBase::GetUnitsPerRev(void) const
{
    return motorRegs_.UnitsPerRev;
}

// sets the GearheadRatio property
// returns the old value of the property
uint32_t MotorBase::SetGearRatio(uint32_t ratio, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.GearheadRatio;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.GearheadRatio = ratio;
    addr += GearheadRatioAddr;
    if ( apply )
    {
        WrtDataValue(addr, ratio, opCallback);
        motorRegs_.GearheadRatio = ratio;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the GearheadRatio property
uint32_t MotorBase::GetGearRatio(void) const
{
    return motorRegs_.GearheadRatio;
}

// sets the EncoderTicksPerRev property
// returns the old value of the property
uint32_t MotorBase::SetTicksPerRev(uint32_t count, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.EncoderTicksPerRev;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.EncoderTicksPerRev = count;
    addr += EncoderTicksPerRevAddr;
    if ( apply )
    {
        WrtDataValue(addr, count, opCallback);
        motorRegs_.EncoderTicksPerRev = count;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the EncoderTicksPerRev property
uint32_t MotorBase::GetTicksPerRev(void) const
{
    return motorRegs_.EncoderTicksPerRev;
}

// sets the Deadband property
// returns the old value of the property
uint32_t MotorBase::SetDeadband(uint32_t units, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.Deadband;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.Deadband = units;
    addr += DeadbandAddr;
    if ( apply )
    {
        WrtDataValue(addr, units, opCallback);
        motorRegs_.Deadband = units;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the Deadband property
uint32_t MotorBase::GetDeadband(void) const
{
    return motorRegs_.Deadband;
}

// sets the InvertedDirection property
// returns the old value of the property
uint32_t MotorBase::SetInvertedDirection(uint32_t reversed, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.InvertedDirection;
    if ( ( reversed == 0 ) || ( reversed == 1 ) )
    {
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.InvertedDirection = reversed;
        addr += InvertedDirectionAddr;
        if ( apply )
        {
            WrtDataValue(addr, reversed, opCallback);
            motorRegs_.InvertedDirection = reversed;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// returns the InvertedDirection property
uint32_t MotorBase::GetInvertedDirection(void) const
{
    return motorRegs_.InvertedDirection;
}

// sets the motor step size property;
// returns the old value of the property
uint32_t MotorBase::SetStepSize(uint32_t size, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.StepSize;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.StepSize = size;
    addr += StepSizeAddr;
    if ( apply )
    {
        WrtDataValue(addr, size, opCallback);
        motorRegs_.StepSize = size;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the motor step size property value
uint32_t MotorBase::GetStepSize(void) const
{
    return motorRegs_.StepSize;
}

// sets the motor acceleration property;
// returns the old value of the property
uint32_t MotorBase::SetAccel(uint32_t accel, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.Acceleration;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.Acceleration = accel;
    addr += AccelerationAddr;
    if ( apply )
    {
        WrtDataValue(addr, accel, opCallback);
        motorRegs_.profileRegs.Acceleration = accel;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the motor acceleration property value
uint32_t MotorBase::GetAccel(void) const
{    
	return motorRegs_.profileRegs.Acceleration;
}

// sets the motor Deceleration property;
// returns the old value of the property
uint32_t MotorBase::SetDecel(uint32_t decel, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.Deceleration;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.Deceleration = decel;
    addr += DecelerationAddr;
    if ( apply )
    {
        WrtDataValue(addr, decel, opCallback);
        motorRegs_.profileRegs.Deceleration = decel;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the motor Deceleration property value
uint32_t MotorBase::GetDecel(void) const
{
    //return motorRegs_.Deceleration;
	return motorRegs_.profileRegs.Deceleration;
}

// sets the motor minimum speed property;
// returns the old value of the property
uint32_t MotorBase::SetMinSpeed(uint32_t minSpeed, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.MinSpeed;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.MinSpeed = minSpeed;
    addr += MinSpeedAddr;
    if ( apply )
    {
        WrtDataValue(addr, minSpeed, opCallback);
        motorRegs_.MinSpeed = minSpeed;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the motor minimum speed property value
uint32_t MotorBase::GetMinSpeed(void) const
{
    return motorRegs_.MinSpeed;
}

// sets the motor maximum speed property;
// returns the old value of the property
uint32_t MotorBase::SetMaxSpeed(uint32_t maxSpeed, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.MaxSpeed;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.MaxSpeed = maxSpeed;
    addr += MaxSpeedAddr;
    if ( apply )
    {
        WrtDataValue(addr, maxSpeed, opCallback);
        motorRegs_.profileRegs.MaxSpeed = maxSpeed;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the motor maximum speed property value
uint32_t MotorBase::GetMaxSpeed(void) const
{    
	return motorRegs_.profileRegs.MaxSpeed;
}

// sets the OverCurrent property
// returns the old value of the property
uint32_t MotorBase::SetOverCurrent(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.OverCurrent;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.OverCurrent = current;
    addr += OverCurrentAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.OverCurrent = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the OverCurrent property
uint32_t MotorBase::GetOverCurrent(void) const
{
    return motorRegs_.OverCurrent;
}

// sets the StallCurrent property
// returns the old value of the property
uint32_t MotorBase::SetStallCurrent(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.StallCurrent;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.StallCurrent = current;
    addr += StallCurrentAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.StallCurrent = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the StallCurrent property
uint32_t MotorBase::GetStallCurrent(void) const
{
    return motorRegs_.StallCurrent;
}

// sets the HoldVoltageDivide property
// returns the old value of the property
uint32_t MotorBase::SetHoldVoltDivide(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.HoldVoltageDivide;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.HoldVoltageDivide = current;
    addr += HoldVoltageDivideAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.HoldVoltageDivide = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the HoldVoltageDivide property
uint32_t MotorBase::GetHoldVoltageDivide(void) const
{
    return motorRegs_.HoldVoltageDivide;
}

// sets the RunVoltageDivide property
// returns the old value of the property
uint32_t MotorBase::SetRunVoltDivide(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.RunVoltageDivide;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.RunVoltageDivide = current;
    addr += RunVoltageDivideAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.profileRegs.RunVoltageDivide = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the RunVoltageDivide property
uint32_t MotorBase::GetRunVoltageDivide(void) const
{    
	return motorRegs_.profileRegs.RunVoltageDivide;
}

// sets the AccVoltageDivide property
// returns the old value of the property
uint32_t MotorBase::SetAccelVoltDivide(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.AccVoltageDivide;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.AccVoltageDivide = current;
    addr += AccVoltageDivideAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.profileRegs.AccVoltageDivide = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the AccVoltageDivide property
uint32_t MotorBase::GetAccelVoltageDivide(void) const
{    
	return motorRegs_.profileRegs.AccVoltageDivide;
}

// sets the DecVoltageDivide property
// returns the old value of the property
uint32_t MotorBase::SetDecelVoltDivide(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = dfltParams_.profileRegs.DecVoltageDivide;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.profileRegs.DecVoltageDivide = current;
    addr += DecVoltageDivideAddr;
    if ( apply )
    {
        WrtDataValue(addr, current, opCallback);
        motorRegs_.profileRegs.DecVoltageDivide = current;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

// returns the DecVoltageDivide property
uint32_t MotorBase::GetDecelVoltageDivide(void) const
{    
	return motorRegs_.profileRegs.DecVoltageDivide;
}

// sets the CONFIG raw property
// returns the old value of the property
uint32_t MotorBase::SetConfig(uint32_t config, bool apply)
{
    uint32_t oldVal = dfltParams_.CONFIG;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.CONFIG = config;
    addr += CONFIG_ADDR;
    if ( apply )
    {
        WrtDataValue(addr, config);
        motorRegs_.CONFIG = config;
    }
    return oldVal;
}

// returns the CONFIG raw property
uint32_t MotorBase::GetConfig(void) const
{
    return motorRegs_.CONFIG;
}

// sets the INT_SPEED raw property
// returns the old value of the property
uint32_t MotorBase::SetIntSpd(uint32_t spdCfg, bool apply)
{
    uint32_t oldVal = dfltParams_.INT_SPEED;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.INT_SPEED = spdCfg;
    addr += INT_SPEED_ADDR;
    if ( apply )
    {
        WrtDataValue(addr, spdCfg);
        motorRegs_.INT_SPEED = spdCfg;
    }
    return oldVal;
}

// returns the INT_SPEED raw property
uint32_t MotorBase::GetIntSpd(void) const
{
    return motorRegs_.INT_SPEED;
}

// sets the INT_SPEED raw property
// returns the old value of the property
uint32_t MotorBase::SetStSlp(uint32_t slpCfg, bool apply)
{
    uint32_t oldVal = dfltParams_.ST_SLP;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.ST_SLP = slpCfg;
    addr += ST_SLP_ADDR;
    if ( apply )
    {
        WrtDataValue(addr, slpCfg);
        motorRegs_.ST_SLP = slpCfg;
    }
    return oldVal;
}

// returns the INT_SPEED raw property
uint32_t MotorBase::GetStSlp(void) const
{
    return motorRegs_.ST_SLP;
}

// sets the FN_SLP_ACC raw property
// returns the old value of the property
uint32_t MotorBase::SetFnSlpAccel(uint32_t slpCfg, bool apply)
{
    uint32_t oldVal = dfltParams_.FN_SLP_ACC;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.FN_SLP_ACC = slpCfg;
    addr += FN_SLP_ACC_ADDR;
    if ( apply )
    {
        WrtDataValue(addr, slpCfg);
        motorRegs_.FN_SLP_ACC = slpCfg;
    }
    return oldVal;
}

// returns the FN_SLP_ACC raw property
uint32_t MotorBase::GetFnSlpAccel(void) const
{
    return motorRegs_.FN_SLP_ACC;
}

// sets the FN_SLP_DEC raw property
// returns the old value of the property
uint32_t MotorBase::SetFnSlpDecel(uint32_t slpCfg, bool apply)
{
    uint32_t oldVal = dfltParams_.FN_SLP_DEC;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.FN_SLP_DEC = slpCfg;
    addr += FN_SLP_DEC_ADDR;
    if ( apply )
    {
        WrtDataValue(addr, slpCfg);
        motorRegs_.FN_SLP_DEC = slpCfg;
    }
    return oldVal;
}

// returns the FN_SLP_DEC raw property
uint32_t MotorBase::GetFnSlpdecel(void) const
{
    return motorRegs_.FN_SLP_DEC;
}

// sets the motor delay after move property;
// controls the delay between the completion of a move and the reporting of the completion;
// returns the old value of the property
uint32_t MotorBase::SetDelayAfterMove(uint32_t delay, bool apply)
{
    uint32_t oldVal = dfltParams_.DelayAfterMove;
    uint32_t addr = GetMotorAddrForId(motorTypeId_);
    dfltParams_.DelayAfterMove = delay;
    addr += DelayAfterMoveAddr;
    if ( apply )
    {
        WrtDataValue(addr, delay);
        motorRegs_.DelayAfterMove = delay;
    }
    return oldVal;
}

// returns the motor delay after move property value;
uint32_t MotorBase::GetDelayAfterMove(void) const
{
    return motorRegs_.DelayAfterMove;
}


////////////////////////////////////////////////////////////////////////////////
// write-only properties

// sets the probe-speed1 property;
// returns old probe-speed1 property value or zero is not probe-type motor
uint32_t MotorBase::AdjustProbeSpeed1(uint32_t speed, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = dfltParams_.ProbeRegs.ProbeSpeed1;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeSpeed1 = speed;
        addr += ProbeSpeed1Addr;
        if ( apply )
        {
            WrtDataValue(addr, speed, opCallback);
            motorRegs_.ProbeRegs.ProbeSpeed1 = speed;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// sets the probe-speed1 current property;
// returns old probe-speed1 current property value or zero is not probe-type motor
uint32_t MotorBase::AdjustProbeSpeed1Current(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = dfltParams_.ProbeRegs.ProbeCurrent1;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeCurrent1 = current;
        addr += ProbeCurrent1Addr;
        if ( apply )
        {
            WrtDataValue(addr, current, opCallback);
            motorRegs_.ProbeRegs.ProbeCurrent1 = current;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// sets the probe-speed2 property;
// returns old probe-speed2 property value or zero is not probe-type motor
uint32_t MotorBase::AdjustProbeSpeed2(uint32_t speed, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = dfltParams_.ProbeRegs.ProbeSpeed2;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeSpeed2 = speed;
        addr += ProbeSpeed2Addr;
        if ( apply )
        {
            WrtDataValue(addr, speed, opCallback);
            motorRegs_.ProbeRegs.ProbeSpeed2 = speed;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// sets the probe-speed2 current property;
// returns old probe-speed2 current property value or zero is not probe-type motor
uint32_t MotorBase::AdjustProbeSpeed2Current(uint32_t current, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = dfltParams_.ProbeRegs.ProbeCurrent2;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeCurrent2 = current;
        addr += ProbeCurrent2Addr;
        if ( apply )
        {
            WrtDataValue(addr, current, opCallback);
            motorRegs_.ProbeRegs.ProbeCurrent2 = current;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// sets the probe-above-position property;
// returns old probe-above-position property value or zero is not probe-type motor
int32_t MotorBase::AdjustProbeAbovePosition(int32_t pos, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    int32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = (int32_t)dfltParams_.ProbeRegs.ProbeAbovePosition;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeAbovePosition = pos;
        addr += ProbeAbovePositionAddr;
        if ( apply )
        {
            WrtDataValue(addr, pos, opCallback);
            motorRegs_.ProbeRegs.ProbeAbovePosition = pos;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// returns probe-above-position property value or zero is not probe-type motor
int32_t MotorBase::GetProbeAbovePosition(void) const
{
    int32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = (int32_t)motorRegs_.ProbeRegs.ProbeAbovePosition;
        if ( oldVal == 0 )
        {
            oldVal = (int32_t)dfltParams_.ProbeRegs.ProbeAbovePosition;
        }
    }
    return oldVal;
}

// sets the probe-raise property;
// returns old probe-raise value or zero is not probe-type motor
int32_t MotorBase::AdjustProbeRaise(int32_t distance, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    int32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = (int32_t)dfltParams_.ProbeRegs.ProbeRaise;
        uint32_t addr = GetMotorAddrForId(motorTypeId_);
        dfltParams_.ProbeRegs.ProbeRaise = distance;
        addr += ProbeRaiseAddr;
        if ( apply )
        {
            WrtDataValue(addr, distance, opCallback);
            motorRegs_.ProbeRegs.ProbeRaise = distance;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

// returns the probe-raise value or zero is not probe-type motor
int32_t MotorBase::GetProbeRaise(void) const
{
    int32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = (int32_t)motorRegs_.ProbeRegs.ProbeRaise;
        if ( oldVal == 0 )
        {
            oldVal = (int32_t)dfltParams_.ProbeRegs.ProbeRaise;
        }
    }
    return oldVal;
}

// local value for default reference (and use where limit switches and firmware is not working properly)
int32_t MotorBase::SetDefaultProbeStop( int32_t stopPos )
{
    int32_t oldVal = 0;
    if ( motorTypeId_ == MotorTypeProbe )
    {
        oldVal = (int32_t)dfltParams_.ProbeStopPosition;
        dfltParams_.ProbeStopPosition = stopPos;
        motorRegs_.ProbeStopPosition = stopPos;
        probeStopPos = stopPos;
    }
    return oldVal;
}

int32_t MotorBase::AdjustMaxTravel( int32_t maxPos, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    int32_t oldVal = 0;
    if ( ( motorTypeId_ == MotorTypeProbe ) || ( motorTypeId_ == MotorTypeReagent ) )
    {
        oldVal = (int32_t)dfltParams_.MaxTravelPosition;
        uint32_t addr = GetMotorAddrForId( motorTypeId_ );
        dfltParams_.MaxTravelPosition = maxPos;
        addr += MaxTravelPosAddr;
        if ( apply )
        {
            WrtDataValue( addr, maxPos, opCallback );
            motorRegs_.MaxTravelPosition = maxPos;
        }
		else if (opCallback)
		{
			pCBOService_->enqueueInternal(opCallback.get(), true);
		}
	}
    return oldVal;
}

int32_t MotorBase::GetMaxTravel( void ) const
{
    int32_t oldVal = 0;
    if (( motorTypeId_ == MotorTypeProbe ) || ( motorTypeId_ == MotorTypeReagent ))
    {
        oldVal = (int32_t)motorRegs_.MaxTravelPosition;
        if ( oldVal == 0 )
        {
            oldVal = (int32_t)dfltParams_.MaxTravelPosition;
        }
    }
    return oldVal;
}

uint32_t MotorBase::AdjustMaxRetries( uint32_t retries, bool apply, boost::optional<std::function<void(bool)>> opCallback)
{
    uint32_t oldVal = 0;

    oldVal = (int32_t)dfltParams_.MaxMoveRetries;
    uint32_t addr = GetMotorAddrForId( motorTypeId_ );
    dfltParams_.MaxMoveRetries = retries;
    addr += MaxMoveRetriesAddr;
    if ( apply )
    {
        WrtDataValue( addr, retries, opCallback );
        motorRegs_.MaxMoveRetries = retries;
    }
	else if (opCallback)
	{
		pCBOService_->enqueueInternal(opCallback.get(), true);
	}
	return oldVal;
}

uint32_t MotorBase::GetMaxRetries( void ) const
{
    int32_t oldVal = 0;

    oldVal = (int32_t)motorRegs_.MaxMoveRetries;
    if ( oldVal == 0 )
    {
        oldVal = (int32_t)dfltParams_.MaxMoveRetries;
    }

    return oldVal;
}

void MotorBase::SetMotorProfileRegisters( std::function<void( bool )> cb, MotorProfileRegisters profileRegisters, bool apply )
{
	Logger::L().Log (GetModuleName(), severity_level::debug3, "SetMotorProfileRegisters: <enter>" );

	dfltParams_.profileRegs = profileRegisters;
	motorRegs_.profileRegs = profileRegisters;

	if ( apply )
	{
		uint32_t addr = GetMotorAddrForId( motorTypeId_ );

		if ( !MotorRegAddrValid( motorTypeId_, addr ) )
		{
			updateHost( cb, false );
			Logger::L().Log (GetModuleName(), severity_level::error, "SetMotorProfileRegisters: invalid motor address or Id" );
			return;
		}

		addr += ProfileRegsAddr;

		auto onWrtProfileRegsComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
		{
			Logger::L().Log (GetModuleName(), severity_level::debug1, "onWrtProfileRegsComplete: <callback>" );

			bool status = cbData.status == ControllerBoardOperation::Success;
			if (!status)
			{
				Logger::L().Log (GetModuleName(), severity_level::error, "SetMotorProfileRegisters: unable to set motor profile parameters!");
			}

			updateHost( cb, status );
		};

		const uint32_t timeout_sec = 1;
		if ( !executeWriteOperation( MotorWriteProfileRegistersOperation( static_cast<RegisterIds>( addr ), (void *)&profileRegisters ),
		                             timeout_sec, onWrtProfileRegsComplete ) )
		{
			updateHost( cb, false );
			Logger::L().Log (GetModuleName(), severity_level::error, "SetMotorProfileRegisters: <exit> pCbo Execute failed" );
			return;
		}
	}
	else
	{
		updateHost( cb, true );
	}

	Logger::L().Log (GetModuleName(), severity_level::debug3, "SetMotorProfileRegisters: <exit>" );
}

void MotorBase::GetMotorProfileRegisters( MotorProfileRegisters & profileRegisters ) const
{
	profileRegisters = motorRegs_.profileRegs;
}



////////////////////////////////////////////////////////////////////////////////
// read-only properties

// returns the motor state machine error code
uint32_t MotorBase::GetErrorCode(void) const
{
	return motorRegs_.ErrorCode;
}

// returns the current motor position value
int32_t MotorBase::GetPosition(void) const
{
	return motorRegs_.Position;
}

// returns the current motor position value
void MotorBase::ReadPosition (std::function<void (bool)> cb, const boost::system::error_code error)
{
	return RdMotorPositionStatus (cb, error);
}

// returns last stop position value or zero is not probe-type motor
// value may be 0 if the probe has not been moved
int32_t MotorBase::GetProbeStop(void) const
{
    if (motorTypeId_ != MotorTypeProbe)
		return 0;

	// substitute the default stop position value if it hasn't yet been read
    return motorRegs_.ProbeStopPosition != 0 ? motorRegs_.ProbeStopPosition : probeStopPos;
}

bool MotorBase::GetHomeStatus(void) const
{
	return motorRegs_.HomedStatus != 0;
}

void MotorBase::updateHost(std::function<void(bool)> callback, bool status) const
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	pCBOService_->enqueueInternal(callback, status);
}

void MotorBase::SetMaxMotorPositionValue(boost::optional<int32_t> maxPos)
{
	this->maxPos = maxPos;
}

// initialize the motor with a complete register set
void MotorBase::InitializeMotor( std::function<void( bool )> cb, uint32_t timeout_Sec, MotorRegisters srcRegs )
{
	assert( cb );

	std::string motorStr = boost::str(boost::format("%s (motor type %d)") % GetMotorTypeAsString(motorTypeId_) % motorTypeId_);
	Logger::L().Log (GetModuleName(), severity_level::debug1, "InitializeMotor: <enter " + motorStr + ">");

	if ( !MotorIdValid( motorTypeId_ ) || !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::debug1, "InitializeMotor: <exit> Invalid ID/Address!" );
		return;
	}

	memcpy( &motorRegs_, &srcRegs, sizeof( MotorRegisters ) );               // copy to working register set object for this motor

	auto onInitializeComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onInitializeComplete: <callback>" );

		bool status = cbData.status == ControllerBoardOperation::Success;
		if (!status)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "InitializeMotor: unable to initialize motor parameters!");
		}

		updateHost( cb, status );
	};

	if ( !executeWriteOperation( MotorInitializeFullRegisterOperation( static_cast<RegisterIds>( motorRegsAddr_ ), srcRegs ),
	                             timeout_Sec, onInitializeComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "InitializeMotor: <exit> CBO execute failed" );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "InitializeMotor: <exit>" );
}

// apply updated motor parameters/register already sent to the controller
void MotorBase::UpdateMotorParams(std::function<void(bool)> cb )
{
	// Reset is no longer supported; it is replaced by the initialization command to firmware.
	Logger::L().Log (GetModuleName(), severity_level::debug1, "UpdateMotorParams: <enter>");

	auto onUpdateComplete = [this, cb](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onUpdateComplete: <callback>");

		bool status = cbData.status == ControllerBoardOperation::Success;
		if (!status)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "UpdateMotorParams: unable to update motor parameters!");
		}
		updateHost(cb, status);
	};

	if ( !executeWriteOperation( MotorInitializeOperation(static_cast<RegisterIds>(motorRegsAddr_) ),
	                             100, onUpdateComplete) )
	{
		updateHost(cb, false);
		Logger::L().Log (GetModuleName(), severity_level::error, "InitializeMotor: <exit> CBO execute failed");
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "UpdateMotorParams: <exit>");
}

// apply full updated motor parameters/register contents to the motor
void MotorBase::UpdateMotorParamsFromDefaultRegs(std::function<void(bool)> cb)
{
	// Reset is no longer supported; it is replaced by the initialization command to firmware.
	Logger::L().Log (GetModuleName(), severity_level::debug1, "UpdateMotorParamsFromDefaultRegs: <enter>");

	auto onUpdateComplete = [this, cb](bool status) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onUpdateComplete: <callback>");

		if (!status)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "UpdateMotorParamsFromDefaultRegs: unable to update motor parameters!");
		}
		updateHost(cb, status);
	};

	InitializeMotor(onUpdateComplete, 100, dfltMotorRegs_);

	Logger::L().Log (GetModuleName(), severity_level::debug1, "UpdateMotorParamsFromDefaultRegs: <exit>");
}

// update the default parameters/register contents and apply to the motor
void MotorBase::ReinitializeMotorDefaults(std::function<void(bool)> cb, MotorRegisters srcRegs )
{
	// Reset is no longer supported; it is replaced by the initialization command to firmware.
	Logger::L().Log (GetModuleName(), severity_level::debug1, "ReinitializeMotorDefaults: <enter>" );

	UpdateParamsFromRegs( &dfltParams_, &srcRegs );
	UpdateRegsFromParams( &dfltMotorRegs_, &dfltParams_ );

	auto onReinitComplete = [this, cb]( bool status ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onReinitComplete: <callback>" );

		if ( !status )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "ReinitializeMotorDefaults: unable to reinitialize motor parameters!" );
		}
		updateHost(cb, status);
	};

	InitializeMotor( onReinitComplete, 1000, dfltMotorRegs_ );

	Logger::L().Log (GetModuleName(), severity_level::debug1, "ReinitializeMotorDefaults: <exit>" );
}

void MotorBase::Home(std::function<void(bool)> cb, uint32_t timeout_Sec)
{
	assert(cb);

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Home: <enter>" );

	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "Home: <exit> Invalid ID/Address" );
		return;
	}

	if ( IsBusy() )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::debug1, "Home: <exit> motor busy!" );
		return;
	}

	auto onMotorHomeComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onMotorHomeComplete: <callback>" );

		bool success = cbData.status == ControllerBoardOperation::Success;
		if ( !success )
		{
			updateHost( cb, false );

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_homefail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}

			Logger::L().Log (GetModuleName(), severity_level::error, "Home: unable to home motor!" );
			return;
		}

		// CBO operation succeeded to get here; do some sanity checks on status values...
		// CBO GetSignalStatus may not update as quickly as the value in the returned motor registers structure...
		bool homedOk = MotorHomeSwitch( pCBOService_->CBO()->GetSignalStatus().get(), motorTypeId_ );		// do a call to get the updated 'homed' flag
		if ( !homedOk )		// CBO motor status shows not yet homed...
//		if ( !homedOk && motorRegs_.HomedStatus )
		{
			if ( motorRegs_.HomedStatus )		// controller returned 'homed' in the register status value...
			{
				Logger::L().Log( GetModuleName(), severity_level::error, "Home: homed signal indicator does not agree with homed value returned!  Status may not yet have been updated!" );
			}
			else
			{	// if motor registers homed status does not show successful homing, then opertion has likely failed...
				updateHost( cb, false );

				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_homefail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));

				Logger::L().Log (GetModuleName(), severity_level::error, "Home: unable to home motor!" );
				return;
			}
		}

		// Assume that the action completed successfully since the cbData.status value was 'Success'
		updateHost(cb, success);
	};

	if ( !executeWriteOperation( MotorHomeOperation( static_cast<RegisterIds>( motorRegsAddr_ ) ),
                                 timeout_Sec, onMotorHomeComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "Home: <exit> pCbo execute failed" );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Home: <exit>" );
}

void MotorBase::Stop(std::function<void (bool)> cb, bool hardStop, uint32_t timeout_Sec)
{
	assert( cb );

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Stop: <enter>" );

	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "Stop: <exit> Invalid ID/Address" );
		return;
	}

	auto onMotorStopComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onMotorStopComplete: <callback>" );

		bool status = cbData.status == ControllerBoardOperation::Success;
		if (!status)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "Stop: unable to stop motor!");
		}
		updateHost( cb, status );
	};

	if ( !executeWriteOperation( MotorStopOperation( static_cast<RegisterIds>( motorRegsAddr_ ), hardStop ),
                                 timeout_Sec, onMotorStopComplete ) )
	{
		Logger::L().Log (GetModuleName(), severity_level::error, "Stop: <exit> pCbo execute failed" );
		updateHost( cb, false );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Stop: <exit>" );
}

void MotorBase::MarkPosAsZero(std::function<void(bool)> cb, uint32_t timeout_Sec)
{
	assert( cb );

	Logger::L().Log (GetModuleName(), severity_level::debug1, "MarkPosAsZero: <enter>" );

	if (!MotorRegAddrValid(motorTypeId_, motorRegsAddr_))
	{
		updateHost(cb, false);
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "MarkPosAsZero: <exit> Invalid ID/Address");
		return;
	}

	auto onMarkZeroPosComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onMarkZeroPosComplete: <callback>" );

		bool status = cbData.status == ControllerBoardOperation::Success;
		if (!status)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "MarkPosAsZero: unable to zero motor!");
		}
		updateHost( cb, status );
	};

	if ( !executeWriteOperation( MotorMarkAsZeroOperation( static_cast<RegisterIds>( motorRegsAddr_ ) ),
	                             timeout_Sec, onMarkZeroPosComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "MarkPosAsZero: <exit> Refused to execute operation");
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "MarkPosAsZero: <exit>" );
}

void MotorBase::SetPosition(std::function<void(bool)> cb, const int32_t pos, uint32_t timeout_Sec)
{
	assert( cb );

	std::string output = boost::str(boost::format("SetPosition: postiion: %d, timeout: %dsec") % pos %timeout_Sec);

	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nInvalid ID/Address" );
		return;
	}

	auto onMotorSetPosComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		std::string cboutput = "onMotorSetPosComplete: <callback>";
		
		bool success = cbData.status == ControllerBoardOperation::Success;
		if ( !success )
		{
			updateHost( cb, success );

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}

			Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nunable to set motor position!" );
			return;
		}

		int32_t tempTgtPos = pos;

		if (motorTypeId_ == MotorTypeTheta)
		{
			// "Normalize target position from "0 - maxPos" if so configured (rotational positioning)
			if (this->maxPos && tempTgtPos < 0)
			{
				while (tempTgtPos <= this->maxPos.get())
				{
					tempTgtPos += this->maxPos.get();
				}
			}

			// "Normalize target position from "0 - maxPos" if so configured (rotational positioning)
			if (this->maxPos && tempTgtPos > this->maxPos.get())
			{
				tempTgtPos %= this->maxPos.get();
			}
		}

		bool posOk = this->PosAtTgt(this->GetPosition(), tempTgtPos, 0, true);
		if (!posOk)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nfinal position not at expected!");
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
		}
		updateHost(cb, posOk);
	};

	if ( !executeWriteOperation( MotorGoToPositionOperation( static_cast<RegisterIds>( motorRegsAddr_ ), pos ),
	                             timeout_Sec, onMotorSetPosComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nRefused to execute operation" );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, output );

}

void MotorBase::MovePosRelative(std::function<void(bool)> cb, const int32_t posStep, uint32_t timeout_Sec)
{
	assert( cb );
	std::string output = boost::str(boost::format("MovePosRelative: %d, %dsec") % posStep %timeout_Sec);

	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nInvalid ID/Address" );
		return;
	}

	int32_t tgtPos = GetPosition() + posStep;

	auto onMotorSetPosRelComplete = [this, cb, posStep, tgtPos ]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		std::string cboutput = "onMotorSetPosRelComplete: <callback>";

		if ( cbData.status != ControllerBoardOperation::Success)
		{
			this->updateHost( cb, false );

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}

			Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nunable to set position!" );
			return;
		}

		int32_t tempTgtPos = tgtPos;

		if (motorTypeId_ == MotorTypeTheta)
		{
			// "Normalize target position from "0 - maxPos" if so configured (rotational positioning)
			if (this->maxPos && tempTgtPos < 0)
			{
				while (tempTgtPos <= this->maxPos.get())
				{
					tempTgtPos += this->maxPos.get();
				}
			}

			// "Normalize target position from "0 - maxPos" if so configured (rotational positioning)
			if (this->maxPos && tempTgtPos > this->maxPos.get())
			{
				tempTgtPos %= this->maxPos.get();
			}
		}

		bool posOk = this->PosAtTgt(this->GetPosition(), tempTgtPos, 0, true);
		if (!posOk)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nfinal position not at expected!");
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
		}
		updateHost(cb, posOk);
	};

	if ( !executeWriteOperation( MotorRelativeMoveOperation ( static_cast<RegisterIds>( motorRegsAddr_ ), posStep ),
	                             timeout_Sec, onMotorSetPosRelComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nRefused to execute operation" );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, output );

}

void MotorBase::ProbeDown(std::function<void(bool)> cb, uint32_t timeout_Sec)
{
	assert(cb);

	std::string output = "ProbeDown";
	
	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nInvalid ID/Address" );
		return;
	}

	if ( motorTypeId_ != MotorTypeProbe )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\ninvalid motor type" );
		return;
	}

	auto onProbeDownComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		std::string cboutput = "onProbeDownComplete: <callback>";

		bool success = cbData.status == ControllerBoardOperation::Success;
		if (!success)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, cboutput + "\nunable to move probe down!");

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
		}
		Logger::L().Log (GetModuleName(), severity_level::debug1, cboutput);
		updateHost(cb, success);
	};

	if ( !executeWriteOperation( MotorProbeDownOperation( static_cast<RegisterIds>( motorRegsAddr_ ) ),
	                             timeout_Sec, onProbeDownComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, output + "\nRefused to execute operation" );
		return;
	}
	Logger::L().Log (GetModuleName(), severity_level::debug1, output);
}

void MotorBase::ProbeUp(std::function<void(bool)> cb, uint32_t timeout_Sec)
{
	assert(cb);

	Logger::L().Log (GetModuleName(), severity_level::debug1, "ProbeUp: <enter>" );

	if ( !MotorRegAddrValid( motorTypeId_, motorRegsAddr_ ) )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "ProbeUp: <exit> Invalid ID/Address" );
		return;
	}

	if ( motorTypeId_ != MotorTypeProbe )
	{
		updateHost( cb, false );
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "ProbeUp: <exit> invalid motor type" );
		return;
	}

	auto onProbeUpComplete = [this, cb]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onProbeUpComplete: <callback>" );

		bool success = cbData.status == ControllerBoardOperation::Success;
		if ( !success )
		{
			updateHost( cb, success );

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_homefail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}

			Logger::L().Log (GetModuleName(), severity_level::error, "ProbeUp: unable to move probe up!" );
			return;
		}

		bool posOk = (PosAtTgt(GetPosition(), 0, 0, true) && MotorHomeSwitch(pCBOService_->CBO()->GetSignalStatus().get(), motorTypeId_));
		if (!posOk)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "ProbeUp: final position not at expected!");
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_homefail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
		}
		updateHost(cb, posOk);
	};

	if ( !executeWriteOperation( MotorGoToPositionOperation( static_cast<RegisterIds>( motorRegsAddr_ ), 0 ),
	                             timeout_Sec, onProbeUpComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "ProbeUp: <exit> Refused to execute operation" );
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "ProbeUp: <exit>" );

}

void MotorBase::Enable(std::function<void(bool)> cb, bool currentOn, uint32_t timeout_Sec)
{
	assert( cb );

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Enable: <enter>" );

	if (!MotorRegAddrValid(motorTypeId_, motorRegsAddr_))
	{
		updateHost(cb, false);
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "Enable: <exit> Invalid ID/Address");
		return;
	}

	auto onEnableComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onEnableComplete: <callback>" );

		bool status = cbData.status == ControllerBoardOperation::Success;
		if ( !status )
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "Enable: unable to set holding current state!" );
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_holdingcurrentfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
		}
		updateHost(cb, status);
	};

	if ( !executeWriteOperation( MotorEnableOperation( static_cast<RegisterIds>( motorRegsAddr_ ), currentOn ),
	                             timeout_Sec, onEnableComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "Enable: <exit> Refused to execute operation");
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "Enable: <exit>" );

}

void MotorBase::GotoTravelLimit(std::function<void(bool)> cb, uint32_t timeout_Sec)
{
	assert( cb );

	Logger::L().Log (GetModuleName(), severity_level::debug1, "GotoTravelLimit: <enter>" );

	if (!MotorRegAddrValid(motorTypeId_, motorRegsAddr_))
	{
		updateHost(cb, false);
		triggerErrorReportCb(BuildErrorInstance(instrument_error::instrument_integrity_softwarefault, instrument_error::severity_level::error));
		Logger::L().Log (GetModuleName(), severity_level::error, "GotoTravelLimit: <exit> Invalid ID/Address");
		return;
	}

	auto onGotoTravelLimitComplete = [=]( ControllerBoardOperation::CallbackData_t cbData ) -> void
	{
		Logger::L().Log (GetModuleName(), severity_level::debug1, "onGotoTravelLimitComplete: <callback>" );

		bool success = cbData.status == ControllerBoardOperation::Success;
		if ( !success )
		{
			updateHost( cb, success );

			if (cbData.status == ControllerBoardOperation::Timeout)
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_timeout, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}
			else
			{
				triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
			}

			Logger::L().Log (GetModuleName(), severity_level::error, "GotoTravelLimit: <exit> unable to set motor position!" );
			return;
		}

		bool posOk = MotorLimitSwitch(pCBOService_->CBO()->GetSignalStatus(), motorTypeId_);
		if (!posOk)
		{
			Logger::L().Log (GetModuleName(), severity_level::error, "GotoTravelLimit: final position not at expected!");
			triggerErrorReportCb(BuildErrorInstance(instrument_error::motion_motor_positionfail, getMotorErrorInstace(motorTypeId_), instrument_error::severity_level::error));
		}
		updateHost(cb, posOk);
	};

	if ( !executeWriteOperation( MotorGoToTravelLimitOperation( static_cast<RegisterIds>( motorRegsAddr_ ) ),
	                             timeout_Sec, onGotoTravelLimitComplete ) )
	{
		updateHost( cb, false );
		Logger::L().Log (GetModuleName(), severity_level::error, "GotoTravelLimit: <exit> Refused to execute operation");
		return;
	}

	Logger::L().Log (GetModuleName(), severity_level::debug1, "GotoTravelLimit: <exit>" );
}

boost::optional<uint16_t> MotorBase::executeWriteOperation( const ControllerBoardOperation::Operation& op,
                                                            uint32_t timeout_secs, 
                                                            std::function<void(ControllerBoardOperation::CallbackData_t)> onCompleteCallback)
{
	HAWKEYE_ASSERT (MODULENAME, onCompleteCallback);

	auto wrapCallback = [this, onCompleteCallback](ControllerBoardOperation::CallbackData_t cbData)
	{
		if (cbData.status != ControllerBoardOperation::Success) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("executeWriteOperation: %d, 0x%08X, %s") % cbData.status % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
		}

		// Wait for motor position to be updated
		boost::system::error_code error;
		RdMotorPositionStatus([=](bool status)
		{
			if (!status)
			{
				Logger::L().Log (GetModuleName(), severity_level::error, "executeWriteOperation : RdMotorPositionStatus operation failed!");
			}
			pCBOService_->enqueueInternal(onCompleteCallback, cbData);
		}, error);
	};
	
	return pCBOService_->CBO()->Execute(op, timeout_secs * 1000, wrapCallback);
}
