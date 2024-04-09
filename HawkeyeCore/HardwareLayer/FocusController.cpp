#include "stdafx.h"

#include <boost/format.hpp>
#include <stdio.h>

#include "FocusController.hpp"
#include "Logger.hpp"
#include "HawkeyeDirectory.hpp"
#ifndef NO_ERROR_REPORT
#include "SystemErrors.hpp"
#endif

static const char MODULENAME[] = "FocusController";
static const char CONTROLLERNODENAME[] = "focus_controller";

const uint32_t AsyncFocusPositionPositionInterval = 200;	// 200ms

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

FocusController::FocusController(std::shared_ptr<CBOService> pCBOService)
    : FocusControllerBase(pCBOService)
    , closing( false )
    , controllerOk( false )
    , updateConfig( false )
    , configFile( "" )
    , holdingEnabled( false )
    , PositionTolerance(DefaultFocusPositionTolerance)
    , focusMaxPos(FocusMaxTravel)
    , focusStartPos(DefaultFocusStartPos)
    , focusCoarseStepValue(DefaultFocusCoarseStep)      // should be 1/10 revolution of the motor; motor spec'd at 3000 user units per rev
    , focusFineStepValue(DefaultFocusFineStep)          // 0.5% of full revolution
    , focusMaxTravel(FocusMaxTravel)
    , focusStartTimeout_msec (MotorStartTimeout * TIMEOUT_FACTOR)
    , focusBusyTimeout_msec (FocusBusyTimeout * TIMEOUT_FACTOR)
    , focusDeadband(DefaultFocusDeadband)
{
	focusMotor = std::make_shared<MotorBase>(pCBOService_, MotorTypeId::MotorTypeFocus);

    parentTree.reset();
    configNode.reset();
    controllersNode.reset();
    controllerConfigNode.reset();
    thisControllerNode.reset();
    motorParamsNode.reset();
}

FocusController::~FocusController()
{
    Quit();
}

bool FocusController::FocusAtPosition( int32_t posTgt )
{
    bool posOk = false;
    int32_t currentPos = focusMotor->GetPosition();

	posOk = focusMotor->PosAtTgt( currentPos, posTgt, PositionTolerance );

    std::string logStr = boost::str( boost::format( "FocusAtPosition: posOk: %s  posTgt: %d  reported: %d" ) % ( ( posOk == true ) ? "true" : "false" ) % posTgt % currentPos );
    Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

    return posOk;
}

bool FocusController::FocusAtHome( void )
{
    bool posOk = false;

    int32_t currentPos = focusMotor->GetPosition();
    posOk = focusMotor->IsHome();

    std::string logStr = boost::str( boost::format( "FocusAtHome: posOk: %s" ) % ( ( posOk == true ) ? "true" : "false" ) );
    Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

	if (posOk)
	{
		focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, currentPos, ePositionDescription::Home);
	}
	else
	{
		focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, currentPos, ePositionDescription::Current );
	}

    return posOk;
}

void FocusController::ConfigControllerVariables( void )
{
    boost::property_tree::ptree & config = controllerConfigNode.get();

    PositionTolerance = config.get<int32_t>( "PositionTolerance", PositionTolerance );
    focusMaxPos = config.get<int32_t>( "FocusMaxPos", focusMaxPos );
    focusStartPos = config.get<int32_t>( "FocusStartPos", focusStartPos );
    focusCoarseStepValue = config.get<int32_t>( "FocusCoarseStepValue", focusCoarseStepValue );
    focusFineStepValue = config.get<int32_t>( "FocusFineStepValue", focusFineStepValue );
    focusMaxTravel = config.get<int32_t>( "FocusMaxTravel", focusMaxTravel );
    focusStartTimeout_msec = TIMEOUT_FACTOR * config.get<int32_t>( "FocusStartTimeout", focusStartTimeout_msec );
    focusBusyTimeout_msec = TIMEOUT_FACTOR * config.get<int32_t>( "FocusBusyTimeout", focusBusyTimeout_msec );
    int32_t holdingParam = -1;
    holdingParam = config.get<int32_t>( "HoldingCurrentEnabled", holdingParam );
    if ( holdingParam == -1 )
    {
        updateConfig = true;
        holdingEnabled = false;
    }
    else
    {
        if ( holdingParam == 0 )
        {
            holdingEnabled = false;
        }
        else if ( holdingParam == 1 )
        {
            holdingEnabled = true;
        }
		else
		{
			updateConfig = true;
			if ( holdingParam < 0 )
			{
				holdingEnabled = false;			// less-than-zero values other than -1 will be interpreted as 'disable', though values < 0 are illegal...
			}
			else if ( holdingParam > 1 )
			{
				holdingEnabled = true;			// non-zero values greater than 1 will be interpreted as 'enable', though values > 1 are illegal...
			}
		}
	}
}

int32_t FocusController::WriteControllerConfig( void )
{
    // Unable to save through this method if no configuration location was given!
    if ( !controllerConfigNode )
    {
        updateConfig = false;
        return -1;
    }

    // Update our pTree with the new values
    int32_t updateStatus = UpdateControllerConfig( thisControllerNode, controllerConfigNode );
    if ( updateStatus == 0 )
    {
        // Flush the cached file to disk
        boost::system::error_code ec;
        ConfigUtils::WriteCachedConfigFile( configFile, ec );

        if ( ec )
        {
            updateStatus = -2;
            ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror, 
				instrument_error::instrument_storage_instance::motor_config, 
				instrument_error::severity_level::error));
        }
    }
    else
    {
        updateStatus = -3;
    }

    return updateStatus;
}

int32_t FocusController::UpdateControllerConfig( t_opPTree & controller_node, t_opPTree & config_node )
{
    updateConfig = false;

    if ( !config_node )
        return -3;

    // config_node points at the "xxx_controller.controller_config" node for this particular controller... (should be the same as the preserved 'controllerConfigNode')
    // NOTE that there is a redundant "motor_parameters" section for motors used outside of a controller.
    // These additional motor parameter sections MAY NOT represent the parameters used in the integrated controller instances!
    /*config
    *  logger
    *  motor_controllers
    *      xxx_controller
    *          motor_parameters
    *              xxx_motor
    *                  HomeDirection
    *                  MotorFullStepsPerRev;
    *                  UnitsPerRev;
    *                  GearheadRatio;
    *                  EncoderTicksPerRev;
    *                  Deadband;
    *                  InvertedDirection;
    *                      .
    *                      .
    *                      .
    *              yyy_motor
    *                  HomeDirection
    *                  MotorFullStepsPerRev;
    *                  UnitsPerRev;
    *                  GearheadRatio;
    *                  EncoderTicksPerRev;
    *                  Deadband;
    *                  InvertedDirection;
    *                      .
    *                      .
    *                      .
    *          controller_config
    *              xxxPositionTolerance 1000
    *              xxxHomePos 0
    *              xxxHomePosOffset 0
    *                  .
    *                  .
    *                  .
    *      yyy_controller
    *          motor_parameters
    *              xxx_motor
    *  motor_parameters
    *      xxx_motor
    *          HomeDirection
    *          MotorFullStepsPerRev;
    *          UnitsPerRev;
    *          GearheadRatio;
    *          EncoderTicksPerRev;
    *          Deadband;
    *          InvertedDirection;
    *              .
    *              .
    *              .
    *      yyy_motor
    *          HomeDirection
    *          MotorFullStepsPerRev;
    *          UnitsPerRev;
    *          GearheadRatio;
    *          EncoderTicksPerRev;
    *          Deadband;
    *          InvertedDirection;
    *              .
    *              .
    *              .
    */

//    boost::property_tree::ptree ptControllerCfgNode = *config_node;
    boost::property_tree::ptree & ptControllerCfgNode = config_node.get();

//    ptControllerCfgNode.put<int32_t>( "PositionTolerance", PositionTolerance );
//    ptControllerCfgNode.put<int32_t>( "FocusMaxPos", focusMaxPos );
    ptControllerCfgNode.put<int32_t>( "FocusStartPos", focusStartPos );
//    ptControllerCfgNode.put<int32_t>( "FocusCoarseStepValue", focusCoarseStepValue );
//    ptControllerCfgNode.put<int32_t>( "FocusFineStepValue", focusFineStepValue );
//    ptControllerCfgNode.put<int32_t>( "FocusMaxTravel", focusMaxTravel );
//    ptControllerCfgNode.put<int32_t>( "FocusStartTimeout", focusStartTimeout_msec );
//    ptControllerCfgNode.put<int32_t>( "FocusBusyTimeout", focusBusyTimeout_msec );
    int32_t holdingParam = 0;
    if ( holdingEnabled == true )
    {
        holdingParam = 1;
    }
    ptControllerCfgNode.put<int32_t>( "HoldingCurrentEnabled", holdingParam );

    ( *controller_node ).put_child( "controller_config", ptControllerCfgNode );

    return 0;
}

void FocusController::DoHome( std::function<void( bool )> cb, HomeMoveState opState, bool stateSuccess )
{
	assert( cb );

	std::string stateStr = boost::str( boost::format( " state: %ld" ) % opState );
	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoHome : <Enter>" + stateStr );

	switch ( opState )
	{
		case homeStartPolling:
		{
			// Set position update timer interval to "AsyncFocusPositionPositionInterval"
			focusMotor->ResumePositionUpdates( AsyncFocusPositionPositionInterval );
		}
		// INTENTIONAL FALLTHROUGH
		case homeDoMove:
		{
			auto homeCallback = [ this, cb ]( bool success ) -> void
			{
				if ( !success )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoHome : homeCallback : home command failed!" );
					pCBOService_->enqueueExternal ( cb, false );
					return;
				}

				// execute the next case
				auto postHomeComplete = [ this, cb ]( void ) -> void
				{
					DoHome( cb, homeMoveComplete, true );
				};

				pCBOService_->enqueueInternal( postHomeComplete );
			};

			focusMotor->Home( homeCallback, focusBusyTimeout_msec / 1000 );
			break;
		}

		case homeMoveComplete:
		{
			bool homeOk = false;
			if ( stateSuccess )		// physical move was successful; check the resulting position indications
			{
				homeOk = focusMotor->IsHome();
				if ( homeOk == false )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoHome: Focus motor homing failed" );
				}
			}

			// execute the next case
			auto postPausePoll = [this, cb](void) -> void
			{
				DoHome(cb, homePausePolling, true);
			};

			// turn holding current off if so configured
			DoEnable([this, homeOk, postPausePoll, cb](bool status) -> void
			{
				if (status == false)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "DoHome: Focus motor Enable/Disable failed");
				}

				pCBOService_->enqueueInternal(postPausePoll);
				pCBOService_->enqueueExternal (cb, homeOk);
			}, false);
			break;
		}

		case homePausePolling:
		{
			// Set position update timer interval to none, which will stop the timer
			// Focus position once set can not be changed so no need for positional timer
			focusMotor->PausePositionUpdates();
		}
	}

	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoHome : <Exit>" );
}

void FocusController::DoSetCenter( std::function<void( bool )> cb, SetCenterState opState, bool stateSuccess )
{
	assert( cb );

	std::string stateStr = boost::str( boost::format( " state: %ld" ) % opState );
	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoSetCenter : <Enter>" + stateStr );

	switch ( opState )
	{
		case setCenterStartPolling:
		{
			// Set position update timer interval to "AsyncFocusPositionPositionInterval"
			focusMotor->ResumePositionUpdates( AsyncFocusPositionPositionInterval );
		}
		// INTENTIONAL FALLTHROUGH
		case setCenterDoHome:
		{
			if ( focusMotor->IsHome() )		// if the motor is currently home, advance to the center move operation
			{
				// execute the next case
				auto postCtrHomeBypass = [ this, cb ]( void ) -> void
				{
					DoSetCenter( cb, setCenterSetPosition, true );
				};

				pCBOService_->enqueueInternal( postCtrHomeBypass );
				break;
			}

			auto ctrHomeCallback = [ this, cb ]( bool status ) -> void
			{
				if ( !status )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoSetCenter : homeCallback : home command failed!" );
					pCBOService_->enqueueExternal ( cb, status );
					return;
				}

				// execute the next case
				auto postCtrDoHomeNextStep = [ this, cb ]( void ) -> void
				{
					DoSetCenter( cb, setCenterHomeComplete, true );
				};

				pCBOService_->enqueueInternal( postCtrDoHomeNextStep );
			};

			focusMotor->Home( ctrHomeCallback, focusBusyTimeout_msec / 1000 );
			break;
		}

		case setCenterHomeComplete:
		{
			bool homeOk = focusMotor->IsHome();
			if ( homeOk == false )
			{
				Logger::L().Log ( MODULENAME, severity_level::error, "DoSetCenter: Focus motor homing failed" );
				pCBOService_->enqueueExternal ( cb, homeOk );
				return;
			}

			// execute the next case
			auto ctrHomeCompleteCallback = [ this, cb ]( void ) -> void
			{
				// execute the next case
				auto postCtrHomeNextStep = [ this, cb ]( void ) -> void
				{
					DoSetCenter( cb, setCenterSetPosition, true );
				};

				pCBOService_->enqueueInternal( postCtrHomeNextStep );
			};

			pCBOService_->enqueueInternal( ctrHomeCompleteCallback );
			break;
		}

		case setCenterSetPosition:
		{
			auto ctrSetCallback = [ this, cb ]( bool status ) -> void
			{
				if ( !status )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoSetPositiion : moveCallback : set position command failed!" );
					pCBOService_->enqueueExternal ( cb, status );
					return;
				};

				// execute the next case
				auto postCtrSetNextStep = [ this, cb ]( void ) -> void
				{
					DoSetCenter( cb, setCenterHomeComplete, true );
				};

				pCBOService_->enqueueInternal( postCtrSetNextStep );
			};

			focusMotor->SetPosition( ctrSetCallback, focusMaxPos / 2, focusBusyTimeout_msec / 1000 );
			break;
		}

		case setCenterSetComplete:
		{
			bool posOk = false;
			posOk = focusMotor->PosAtTgt (focusMotor->GetPosition (), focusMaxPos / 2, PositionTolerance);
			if ( posOk == false )
			{
				Logger::L ().Log (MODULENAME, severity_level::error, "DoSetCenter : Focus motor position not at target");
			}

			// execute the next case
			auto postPausePoll = [this, cb](void) -> void
			{
				DoSetCenter(cb, setCenterPausePolling, true);
			};

			// turn holding current off if so configured
			DoEnable([this, postPausePoll, posOk, cb](bool status) -> void
			{
				if (status == false)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "DoSetCenter: Focus motor Enable/Disable failed");
				}

				pCBOService_->enqueueInternal(postPausePoll);
				pCBOService_->enqueueExternal (cb, posOk);
			}, false);
			break;
		}

		case setCenterPausePolling:
		{
			// Set position update timer interval to none, which will stop the timer
			// Focus position once set can not be changed so no need for positional timer
			focusMotor->PausePositionUpdates();
		}
	}

	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoHome : <Exit>" );
}

void FocusController::DoSetPosition( std::function<void( bool )> cb, SetPosState opState, bool stateSuccess, int32_t position )
{
	assert( cb );

	std::string stateStr = boost::str( boost::format( " state: %ld" ) % opState );
	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoSetPosition : <Enter>" + stateStr );

	int32_t currentPos = focusMotor->GetPosition();

	switch ( opState )
	{
		case setPosStartPolling:
		{
			bool isHomed = focusMotor->GetHomeStatus();

			if ( !isHomed || focusMaxPos <= 0 || position < 0 || position > focusMaxPos )
			{
				Logger::L ().Log (MODULENAME, severity_level::error, "DoSetPosition: failed due to not being homed or target position out of limit ");
				pCBOService_->enqueueExternal ( cb, false );
				break;
			}

			if ( focusMotor->PosAtTgt( currentPos, position, PositionTolerance ) )
			{
				Logger::L ().Log (MODULENAME, severity_level::debug1, "DoSetPosition: already at position ");
				pCBOService_->enqueueExternal ( cb, true );
				break;
			}

			SetPosState nextState = setPosTgtMove;

			if ( position < currentPos )									// if target position is below current position...
			{
				nextState = setPosFirstMove;
			}

			// execute the next case
			auto postSetPosNextStep = [ this, cb, nextState, position ]( void ) -> void
			{
				DoSetPosition( cb, nextState, false, position );
			};

			pCBOService_->enqueueInternal( postSetPosNextStep );
			break;
		}

		case setPosFirstMove:
		{
			auto firstMoveCallback = [ this, cb, position ]( bool status ) -> void
			{
				if ( !status )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoSetPositiion : firstMoveCallback : motor set position command failed!" );
					pCBOService_->enqueueExternal ( cb, status );
					return;
				}

				// execute the next case
				auto postSetFirstMoveNextStep = [ this, cb, position ]( void ) -> void
				{
					DoSetPosition( cb, setPosFirstMoveComplete, true, position );
				};

				pCBOService_->enqueueInternal( postSetFirstMoveNextStep );
			};

			// doing first move means target position is below current position...
			int32_t tgtPos = position - ( focusDeadband * 4 );				// to go beyond the desired position; deadband is currently set at approximately 0.4 micron...

			focusMotor->SetPosition( firstMoveCallback, tgtPos, focusBusyTimeout_msec / 1000 );
			break;
		}

		case setPosFirstMoveComplete:
		{
			// doing first move means target position is below current position...
			int32_t tgtPos = position - ( focusDeadband * 4 );				// to go beyond the desired position; deadband is currently set at approximately 0.4 micron...

			if ( !focusMotor->PosAtTgt( currentPos, tgtPos, PositionTolerance) )
			{
				Logger::L().Log ( MODULENAME, severity_level::error, "DoSetPosition : Focus motor position not at target" );
				pCBOService_->enqueueExternal ( cb, false );
				break;
			}

			// execute the next case
			auto postFirstMoveCompleteNextStep = [ this, cb, position ]( void ) -> void
			{
				DoSetPosition( cb, setPosTgtMove, true, position );
			};

			pCBOService_->enqueueInternal( postFirstMoveCompleteNextStep );
			break;
		}

		case setPosTgtMove:
		{
			auto tgtMoveCallback = [ this, cb, position ]( bool status ) -> void
			{
				if ( !status )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoSetPositiion : tgtMoveCallback : set position command failed!" );
					pCBOService_->enqueueExternal ( cb, status );
					return;
				}

				// execute the next case
				auto postSetTgtMoveNextStep = [ this, cb, position ]( void ) -> void
				{
					DoSetPosition( cb, setPosTgtMoveComplete, true, position );
				};

				pCBOService_->enqueueInternal( postSetTgtMoveNextStep );
			};

			focusMotor->SetPosition( tgtMoveCallback, position, focusBusyTimeout_msec / 1000 );
			break;
		}

		case setPosTgtMoveComplete:
		{
			bool posOk = focusMotor->PosAtTgt( currentPos, position, PositionTolerance);
			if ( posOk == false )
			{
				Logger::L().Log ( MODULENAME, severity_level::error, "DoSetPosition : Focus motor position not at final target" );
			}

			// execute the next case
			auto postPausePoll = [this, cb, position](void) -> void
			{
				DoSetPosition(cb, setPosPausePolling, true, position);
			};

			// turn holding current off if so configured
			DoEnable([this, posOk, postPausePoll, cb](bool status) -> void
			{
				if (status == false)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "DoSetPosition: Focus motor Enable/Disable failed");
				}

				pCBOService_->enqueueInternal(postPausePoll);
				pCBOService_->enqueueExternal (cb, posOk);
			}, false);
			break;
		}

		case setPosPausePolling:
		{
			// Set position update timer interval to none, which will stop the timer
			// Focus position once set can not be changed so no need for positional timer
			focusMotor->PausePositionUpdates();
		}
	}

	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoSetPosition : <Exit>" );
}

// NOTE that the pos value will change depending on the step executing; originally it is the step, but for confirmation it is passed in as the calculated target position
void FocusController::DoMoveRelativePosition( std::function<void( bool )> cb, MoveRelState opState, bool stateSuccess, int32_t pos)
{
	assert( cb );

	std::string stateStr = boost::str( boost::format( " state: %ld" ) % opState );
	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoMoveRelativePosition : <Enter>" + stateStr );

	int32_t currentPos = focusMotor->GetPosition();

	switch ( opState )
	{
		case moveRelStartPolling:
		{
			// Set position update timer interval to "AsyncFocusPositionPositionInterval"
			focusMotor->ResumePositionUpdates( AsyncFocusPositionPositionInterval );
		}
		// INTENTIONAL FALLTHROUGH
		case moveRelDoMove:
		{
			auto moveRelCallback = [ this, cb, pos, currentPos ]( bool status ) -> void
			{
				if ( !status )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "DoMoveRelativePosition : moveCallback : set position command failed!" );
					pCBOService_->enqueueExternal ( cb, status );
					return;
				}

				// execute the next case
				auto postMoveRelNextStep = [ this, cb, pos, currentPos ]( void ) -> void
				{
					int32_t tgtPos = currentPos + pos;
					DoMoveRelativePosition( cb, moveRelComplete, true, tgtPos );
				};
				pCBOService_->enqueueInternal( postMoveRelNextStep );
			};

			focusMotor->MovePosRelative( moveRelCallback, pos, focusBusyTimeout_msec / 1000 );
			break;
		}

		case moveRelComplete:
		{
			bool posOk = focusMotor->PosAtTgt( currentPos, pos, PositionTolerance );
			if ( posOk == false )
			{
				Logger::L().Log ( MODULENAME, severity_level::error, "DoMoveRelativePosition: Focus motor position not at target" );
			}

			// execute the next case
			auto postPausePoll = [this, cb, pos, currentPos](void) -> void
			{
				int32_t tgtPos = currentPos + pos;
				DoMoveRelativePosition(cb, moveRelPausePolling, true, tgtPos);
			};

			// turn holding current off if so configured
			DoEnable([this, cb, posOk, postPausePoll](bool status) -> void
			{
				if (status == false)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "DoMoveRelativePosition: Focus motor Enable/Disable failed");
				}

				pCBOService_->enqueueInternal(postPausePoll);
				pCBOService_->enqueueExternal (cb, posOk);
			}, false);
			break;
		}

		case moveRelPausePolling:
		{
			// Set position update timer interval to none, which will stop the timer
			// Focus position once set can not be changed so no need for positional timer
			focusMotor->PausePositionUpdates();
		}
	}

	Logger::L().Log ( MODULENAME, severity_level::debug1, "DoMoveRelativePosition : <Exit>" );
}

void FocusController::DoEnable( std::function<void( bool )> cb, bool enable)
{
	// Set position update timer interval to "AsyncFocusPositionPositionInterval"
	focusMotor->ResumePositionUpdates(AsyncFocusPositionPositionInterval);

	Logger::L().Log (MODULENAME, severity_level::debug3, "DoEnable: Focus motor Enable/Disable ");

	// the enable command does not use a callback at the motorbase level, since it is a fast-execution command
	focusMotor->Enable([this, cb](bool status) -> void
	{
		pCBOService_->enqueueInternal( cb, status );
	}, enable, (focusBusyTimeout_msec / 1000));

	// Set position update timer interval to none, which will stop the timer
	// Focus position once set can not be changed so no need for positional timer
	focusMotor->PausePositionUpdates();
}

void FocusController::Init(std::function<void(bool)> callback, t_pPTree cfgTree, bool apply, std::string cfgFile)
{
    boost::system::error_code ec;
    bool fileChg = false;
    std::string controllerName = CONTROLLERNODENAME;
    t_pPTree cfg;

    if ( cfgFile.empty() )
    {
        cfgFile = configFile;
    }

    if ( cfgFile.empty() )
    {
        cfgFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl);
    }
    else
    {
        if ( ( !configFile.empty() ) && ( cfgFile.compare(configFile) != 0 ) )
        {
            fileChg = true;
        }
    }

    cfg.reset();
    if ( ( fileChg ) || ( ( cfgTree ) && ( focusMotor->CfgTreeValid( cfgTree, controllerName ) ) ) )     // config tree passed in may not be valid for motor controllers...
    {
        cfg = cfgTree;
    }
    else
    {
        if ( parentTree )
        {
            cfg = parentTree;
        }
    }

    if ( !cfg )
    {
        cfg = ConfigUtils::OpenConfigFile(cfgFile, ec, true);
        if (!cfg)
        {
            ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror, 
				instrument_error::instrument_storage_instance::motor_info, 
				instrument_error::severity_level::error));
        }
    }
    Logger::L().Log ( MODULENAME, severity_level::normal, MODULENAME );

    if ( cfg )
    {
        configFile = cfgFile;
    }

    parentTree = cfg;

    if ( !parentTree )
    {
        Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file: - \"config\" section not found or not given");
    }
    else
    {
        if ( !configNode )
        {
            configNode = parentTree->get_child_optional("config");		// top level parent node
        }

        if ( !configNode )
        {
            Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile + "\" - \"config\" section not found");
            ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror, 
				instrument_error::instrument_storage_instance::motor_info, 
				instrument_error::severity_level::error));
        }
        else
        {
            thisControllerNode.reset();

            controllersNode = configNode->get_child_optional("motor_controllers");				// look for the controllers section for individualized parameters...
            if ( controllersNode )
            {
                thisControllerNode = controllersNode->get_child_optional(CONTROLLERNODENAME);	// look for this specifc controller
                if ( thisControllerNode )		// look for the motor parameters sub-node...
                {
                    motorParamsNode = thisControllerNode->get_child_optional( "motor_parameters" );
                    if ( !motorParamsNode )
                    {
                        Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile + "\" - \"motor_parameters\" section not found" );
                        ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::instrument_storage_readerror, 
							instrument_error::instrument_storage_instance::motor_info, 
							instrument_error::severity_level::error));
                    }
                    else						// look for the controller configuration parameters
                    {
                        controllerConfigNode = thisControllerNode->get_child_optional( "controller_config" );
                        if ( !controllerConfigNode )
                        {
                            Logger::L().Log ( MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile + "\" - \"controller_config\" section not found" );
                            ReportSystemError::Instance().ReportError (BuildErrorInstance(
								instrument_error::instrument_storage_readerror, 
								instrument_error::instrument_storage_instance::motor_info,
								instrument_error::severity_level::error));
                        }
                    }
                }
            }
        }
    }

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);

	auto onInitComplete = [this, callback] (bool status) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to initialize Focus controller");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_motor_initerror, 
				instrument_error::motion_motor_instances::focus, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueExternal (callback, false);
			return;
		}

		// initialize based on the basic motor parameters
		int32_t tmpVal = 0;

		tmpVal = focusMotor->GetMaxTravel();             // the motor low-level max travel value (updated by controller variables, below)
		if (tmpVal > 0)
		{
			focusMaxTravel = FocusMaxTravel;
		}

		tmpVal = focusMotor->GetUnitsPerRev();
		if (tmpVal)
		{
			focusCoarseStepValue = (tmpVal / 10);		// 1/10 revolution of the motor
			focusFineStepValue = (tmpVal / 200);		// start by calculating the value as 0.5% of full revolution
		}

		tmpVal = focusMotor->GetDeadband();              // minimum usefull step is above this value
		if (tmpVal)
		{
			focusDeadband = tmpVal;						// minimum usefull step is above this value
		}

		if (controllerConfigNode)
		{
			// now try to read specific updates from the configuration file...
			ConfigControllerVariables();
			if (updateConfig)								// illegal or missing holding current parameter... apply correction
			{
				WriteControllerConfig();
			}
		}

		while (focusFineStepValue <= focusDeadband)		// adjust as necessary to a value which will produce movement...
		{
			focusFineStepValue++;
		}

		if (focusCoarseStepValue < (focusFineStepValue * 10))
		{
			focusCoarseStepValue = focusFineStepValue * 10;
		}

		controllerOk = true;
		setInitialization(true);
		focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		// set holding current as requested
		DoEnable([this, callback](bool status) -> void
		{
			if (!status)
				Logger::L().Log (MODULENAME, severity_level::error, "Init: Focus motor Enable/Disable failed");

			pCBOService_->enqueueInternal (callback, true);
		}, holdingEnabled);
	};

	// initialize the motor from the default and external configuration info
	focusMotor->Init(onInitComplete, parentTree, motorParamsNode, apply);

}

void FocusController::Quit(void)
{
    closing = true;

    if ( updateConfig )
    {
        WriteControllerConfig();
    }

	focusMotorStatus_.reset();
    focusMotor->Quit();

    parentTree.reset();
    configNode.reset();
    controllersNode.reset();
    controllerConfigNode.reset();
    thisControllerNode.reset();
    motorParamsNode.reset();
}

bool FocusController::ControllerOk(void)
{
    return controllerOk;
}

void FocusController::GetMotorStatus( MotorStatus& focusMotorStatus )
{

	focusMotorStatus_.ToCStyle (focusMotorStatus);
}

bool FocusController::IsHome( void )
{
	return focusMotor->IsHome();
}

bool FocusController::IsMax( void )
{
	int32_t currentPos = focusMotor->GetPosition();

	return  ( focusMotor->PosAtTgt( currentPos, focusMaxPos, PositionTolerance ) || currentPos > focusMaxPos );
}

int32_t FocusController::Position( void )
{
	return focusMotor->GetPosition();	// get the motor's cached value
}

int32_t FocusController::GetFocusMax( void )
{
	return focusMaxPos;
}

// returns the configurable auto-focus sweep start position
int32_t FocusController::GetFocusStart( void )
{
	return focusStartPos;
}

// returns the configurable auto-focus sweep start position
void FocusController::SetFocusStart( int32_t start_pos )
{
	updateConfig = true;
	focusStartPos = start_pos;
}

// returns the resulting step size;
// returned value may be the same as current if stepValue = 0 (the default parameter),
// or if the stepValue is out of the allowable range
uint32_t FocusController::AdjustCoarseStep( uint32_t stepValue )
{
	if ( ( stepValue > 0 ) &&
		( stepValue >= ( focusMotor->GetUnitsPerRev() / 20 ) ) &&   // try to ensure a reasonable value of at least 5% of a revolution...
		 ( stepValue <= ( focusMotor->GetUnitsPerRev() / 4 ) ) )     // no more than 1/4 revolution...
	{
		focusCoarseStepValue = stepValue;
	}
	return focusCoarseStepValue;
}

// returns the resulting step size;
// returned value may be the same as current if stepValue = 0 (the default parameter),
// or if the stepValue is out of the allowable range
uint32_t FocusController::AdjustFineStep( uint32_t stepValue )
{
	if ( ( stepValue > 0 ) &&
		( stepValue > focusMotor->GetDeadband() ) &&                // ensure a value that will result in a move
		 ( stepValue <= ( focusCoarseStepValue / 10 ) ) )           // should be 1/10 the coarse step (or less)
	{
		focusFineStepValue = stepValue;
	}
	return focusFineStepValue;
}

void FocusController::AdjustSpeed( uint32_t maxSpeed, uint32_t speedCurrent,
								   uint32_t accel, uint32_t accelCurrent,
								   uint32_t decel, uint32_t decelCurrent )
{
	auto onComplete = [=](bool)
	{
		focusMotor->SetMaxSpeed(maxSpeed);
		focusMotor->SetRunVoltDivide(speedCurrent);
		focusMotor->SetAccel(accel);
		focusMotor->SetAccelVoltDivide(accelCurrent);
		focusMotor->SetDecel(decel);
		focusMotor->SetAccelVoltDivide(decelCurrent);
		focusMotor->ApplyAllParams();
	};

	pCBOService_->enqueueInternal(std::bind(&FocusController::ClearErrors, this, onComplete));
}

void FocusController::Reset( void )
{
	auto onComplete = [this](bool)
	{
		focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, focusMotor->GetPosition(), ePositionDescription::Current);
	};

	// TODO - Call motor base reset function here, note, reset command resets all motor drivers 
	pCBOService_->enqueueInternal(std::bind(&FocusController::ClearErrors, this, onComplete));
}

void FocusController::ClearErrors(std::function<void(bool)> callback)
{
	focusMotor->ClearErrors(callback);
}

void FocusController::SetHoldingCurrent( std::function<void( bool )> cb, bool enabled )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str(boost::format("SetHoldingCurrent: Enable: %s") % (enabled ? "true" : "false") ) );

	if ( enabled != holdingEnabled )
	{
		holdingEnabled = enabled;
	}

	// set holding current on or off as requested
	DoEnable( [ this, cb ]( bool status ) -> void
	{
		if ( status == false )
		{
			Logger::L().Log ( MODULENAME, severity_level::error, "SetHoldingCurrent: Focus motor Enable/Disable failed" );
			pCBOService_->enqueueInternal( cb, status );
		}
	}, enabled );
}

// stop type: true = hard stop no deceleration); false = soft stop (using deceleration)
void FocusController::Stop( std::function<void( bool )> cb, bool stopType )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "Stop: type: %s" ) % ( stopType ? "hard" : "soft" ) ) );

//	focusMotor->ClearErrors();

	focusMotor->Stop([this, cb]( bool status ) -> void
	{
		if ( !status )
		{
			pCBOService_->enqueueExternal (cb, false);
			Logger::L().Log ( MODULENAME, severity_level::error, "Stop: unable to stop motor!" );
			return;
		}
		else
		{
			if ( holdingEnabled != false )
			{
				holdingEnabled = false;
			}

			// set holding current off as requested
			DoEnable( [ this, cb ]( bool cb_status ) -> void
			{
				if ( cb_status == false )
				{
					Logger::L().Log ( MODULENAME, severity_level::error, "Stop: Focus motor Enable/Disable failed" );
				}
			}, false );
		}

		auto positionRdComplete = [this, cb](bool rdStatus) -> void
		{
			if ( !rdStatus )
			{
				pCBOService_->enqueueExternal (cb, false);
				Logger::L ().Log (MODULENAME, severity_level::error, "motor SetPosition: unable to get motor position!");
				return;
			}

			pCBOService_->enqueueExternal (cb, rdStatus);

			focusMotorStatus_.UpdateMotorHealth (eMotorFlags::mfPositionKnown, focusMotor->GetPosition (), ePositionDescription::Current);
		};

		pCBOService_->enqueueInternal ([=](void) -> void
		{
			boost::system::error_code error;
			focusMotor->ReadPosition (positionRdComplete, error);
		});

	}, stopType, ( focusBusyTimeout_msec / 1000 ) );
}

void FocusController::FocusHome( std::function<void( bool )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "FocusHome: <enter>" );

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, 0, ePositionDescription::Target);
	
	// create a lambda to catch the status of the Focus motor home command complete
	auto homeComplete = [ this, cb ]( bool isHome ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( isHome )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfHomed, cbPos, ePositionDescription::Home );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}

		pCBOService_->enqueueExternal ( cb, isHome );
	};

	DoHome( homeComplete, homeStartPolling, false );
}

void FocusController::FocusCenter( std::function<void( bool )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "FocusCenter: <enter>" );

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, (focusMaxPos / 2), ePositionDescription::Target);

	// create a lambda to catch the status of the Focus motor move complete
	auto centerComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfHomed, cbPos, ePositionDescription::Home );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}

		pCBOService_->enqueueExternal ( cb, posOk );
	};

	DoSetCenter( centerComplete, setCenterStartPolling, false );
}

void FocusController::FocusMax( std::function<void( bool )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "FocusMax: <enter>" );

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, focusMaxPos, ePositionDescription::Target);

	// create a lambda to catch the status of the Focus motor move complete
	auto setMaxComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}

		pCBOService_->enqueueExternal ( cb, posOk );
	};

	DoSetPosition( setMaxComplete, setPosStartPolling, false, focusMaxPos );
}

void FocusController::SetPosition( std::function<void( bool, int32_t endPos )> cb, int32_t new_pos )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, boost::str( boost::format( "SetPosition: new_pos: %ld" ) % new_pos ) );

	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfInMotion, new_pos, ePositionDescription::Target );

	// create a lambda to catch the status of the Focus motor move complete
	auto setPosComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
	};

	DoSetPosition( setPosComplete, setPosStartPolling, false, new_pos );
}

// pass in POSITIVE change values; converts to upward movement;
// for EP2 system hardware assumes 'upward' means movement away from the home (or 0) position, thus increasing values.
// for EP1 system hardware, assumes 'upward' means movement toward the home (or 0) position, thus decreasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepUpFast( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug2, "FocusStepUpFast: <enter>" );

	int32_t moveStep = focusCoarseStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev();
    }
    else
    {
        moveStep = focusCoarseStepValue * 5;        // assume the coarse-step default of 1/10 turn, this results in a 1/2 turn step
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
	if ( ( currentPos + moveStep ) > FocusMaxTravel )
	{
		moveStep = ( FocusMaxTravel - currentPos );

		if ( moveStep < 0 || moveStep < ( int32_t ) focusDeadband )
		{
			moveStep = 0;
		}
	}

	// if already at the maximum motor position, don't move to avoid hitting objective
	if ( ( IsMax() ) || ( moveStep == 0 ) )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

	int32_t tgtPos = currentPos + moveStep;

	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target );

	uint32_t maxSpd = focusMotor->GetMaxSpeed();
	uint32_t tmpSpd = maxSpd * 3;

	focusMotor->SetMaxSpeed( tmpSpd, true );

	// create a lambda to catch the status of the Focus motor move complete
	auto stepUpFastComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition ( stepUpFastComplete, setPosStartPolling, false, tgtPos );
}

// pass in POSITIVE change values; converts to downward movement;
// for EP2 system hardware assumes 'downward' means movement toward the home (or 0) position, thus decreasing values.
// for EP1 system hardware, assumes 'downward' means movement away from the home (or 0) position, thus increasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepDnFast( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug2, "FocusStepDnFast: <enter>" );

	int32_t moveStep = focusCoarseStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev();
    }
    else
    {
        moveStep = focusCoarseStepValue * 5;        // assume the coarse-step default of 1/10 turn, this results in a 1/2 turn step
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
    if ( ( currentPos - moveStep ) > FocusMaxTravel )
    {
        moveStep = ( FocusMaxTravel - currentPos );

		if ( moveStep < 0 || moveStep < ( int32_t ) focusDeadband )
		{
			moveStep = 0;
		}
	}

	// if already at or below the home position, don't move to avoid hitting condenser
	if ( IsHome() || currentPos <= 0 || moveStep == 0 )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

	int32_t tgtPos = currentPos - moveStep;

	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target );

	uint32_t maxSpd = focusMotor->GetMaxSpeed();
	uint32_t tmpSpd = maxSpd * 3;

	focusMotor->SetMaxSpeed( tmpSpd, true );

	// create a lambda to catch the status of the Focus motor move complete
	auto stepDnFastComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition ( stepDnFastComplete, setPosStartPolling, false, tgtPos );
}

// pass in POSITIVE change values; converts to upward movement;
// for EP2 system hardware assumes 'upward' means movement away from the home (or 0) position, thus increasing values.
// for EP1 system hardware, assumes 'upward' means movement toward the home (or 0) position, thus decreasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepUpCoarse( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug2, "FocusStepUpCoarse: <enter>" );

	int32_t moveStep = focusCoarseStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev() / 10;        // 10% of full revolution
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
	if ( ( currentPos + moveStep ) > FocusMaxTravel )
	{
		moveStep = ( FocusMaxTravel - currentPos );

		if ( moveStep < 0 || moveStep < ( int32_t ) focusDeadband )
		{
			moveStep = 0;
		}
	}

	// if already at the maximum motor position, don't move to avoid hitting objective
	if ( ( IsMax() ) || ( moveStep == 0 ) )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

	int32_t tgtPos = currentPos + moveStep;

	Logger::L ().Log (MODULENAME, severity_level::debug2, "FocusStepUpCoarse: current = " + std::to_string(currentPos) + "target = " + std::to_string (tgtPos));
				
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// create a lambda to catch the status of the Focus motor move complete
	auto stepUpCoarseComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		Logger::L ().Log (MODULENAME, severity_level::debug2, "FocusStepUpCoarse: stepUpCoarseComplete posOk = " + posOk ? "true" : "false" );

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition ( stepUpCoarseComplete, setPosStartPolling, false, tgtPos );
}

// pass in POSITIVE change values; converts to downward movement;
// for EP2 system hardware assumes 'downward' means movement toward the home (or 0) position, thus decreasing values.
// for EP1 system hardware, assumes 'downward' means movement away from the home (or 0) position, thus increasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepDnCoarse( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug2, "FocusStepDnCoarse: <enter>" );

	int32_t moveStep = focusCoarseStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev() / 10;        // 10% of full revolution
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
    if ( ( currentPos - moveStep ) < 0 )
    {
        moveStep = currentPos;

		if ( moveStep < 0 || moveStep < ( int32_t ) focusDeadband )
		{
			moveStep = 0;
		}
	}

	// if already at or below the home position, don't move to avoid hitting condenser
	if ( IsHome() || currentPos <= 0 || moveStep == 0 )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

	int32_t tgtPos = currentPos - moveStep;

	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target );

	// create a lambda to catch the status of the Focus motor move complete
	auto stepDnCoarseComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition ( stepDnCoarseComplete, setPosStartPolling, false, tgtPos );
}

// pass in POSITIVE change values; converts to upward movement;
// for EP2 system hardware assumes 'upward' means movement away from the home (or 0) position, thus increasing values.
// for EP1 system hardware, assumes 'upward' means movement toward the home (or 0) position, thus decreasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepUpFine( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "FocusStepUpFine: <enter>" );

	int32_t moveStep = focusFineStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev() / 200;       // 0.5% of full revolution
        while ( moveStep <= (int32_t)focusDeadband )
        {
            moveStep++;
        }
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
	if ( ( currentPos + moveStep ) > FocusMaxTravel )
	{
		moveStep = ( FocusMaxTravel - currentPos );

		if ( moveStep < 0 || moveStep  < ( int32_t ) focusDeadband)
		{
			moveStep = 0;
		}
	}

	// if already at the maximum motor position, don't move to avoid hitting objective
	if ( ( IsMax() ) || ( moveStep == 0 ) )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

    int32_t tgtPos = currentPos + moveStep;

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// create a lambda to catch the status of the Focus motor move complete
	auto stepUpFineComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition ( stepUpFineComplete, setPosStartPolling, false, tgtPos );
}

// pass in POSITIVE change values; converts to downward movement;
// for EP2 system hardware assumes 'downward' means movement toward the home (or 0) position, thus decreasing values.
// for EP1 system hardware, assumes 'downward' means movement away from the home (or 0) position, thus increasing values.
// the method will convert to the correct direction movement for the desired change step
// returns the end position
void FocusController::FocusStepDnFine( std::function<void( bool, int32_t endPos )> cb )
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "FocusStepDnFine: <enter>" );

	int32_t moveStep = focusFineStepValue;

    if ( moveStep == 0 )
    {
        moveStep = focusMotor->GetUnitsPerRev() / 200;       // 0.5% of full revolution
        while ( moveStep <= (int32_t)focusDeadband )
        {
            moveStep++;
        }
    }

	if ( moveStep < 0 )
	{
		moveStep = -moveStep;
	}

	int32_t currentPos = focusMotor->GetPosition();
	if ( ( currentPos - moveStep ) < 0 )
	{
		moveStep = currentPos;

		if ( moveStep < 0 || moveStep < ( int32_t ) focusDeadband )
		{
			moveStep = 0;
		}
	}

	// if already at or below the home position, don't move to avoid hitting condenser
	if ( currentPos <= 0 || moveStep == 0 )
	{
		pCBOService_->enqueueExternal ( cb, false, currentPos );
		return;
	}

	int32_t tgtPos = currentPos - moveStep;

	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// create a lambda to catch the status of the Focus motor move complete
	auto stepDnFineComplete = [ this, cb ]( bool posOk ) -> void
	{
		int32_t cbPos = focusMotor->GetPosition();

		if ( posOk )
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, cbPos, ePositionDescription::AtPosition );
		}
		else
		{
			focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfErrorState, cbPos, ePositionDescription::Current );
		}
		pCBOService_->enqueueExternal ( cb, posOk, cbPos );
		return;
	};

	DoSetPosition (stepDnFineComplete, setPosStartPolling, false, tgtPos);
}
