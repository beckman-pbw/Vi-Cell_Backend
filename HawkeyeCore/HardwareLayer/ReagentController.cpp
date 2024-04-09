#include "stdafx.h"

#include <stdio.h>
#include <boost/format.hpp>

#include "EnumConversion.hpp"
#include "ErrorCode.hpp"
#include "Logger.hpp"
#include "ReagentController.hpp"
#include "Registers.hpp"
#include "SignalStatus.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "ReagentController";
static const char CONTROLLERNODENAME[] = "reagent_controller";

const uint32_t ReagentPositionPositionInterval_ms = 25;

const std::map<ReagentController::OperationSequence, std::string>
EnumConversion<ReagentController::OperationSequence>::enumStrings<ReagentController::OperationSequence>::data =
{
	{ ReagentController::OperationSequence::UnlatchDoor, std::string("UnlatchDoor") },
	{ ReagentController::OperationSequence::ArmUp, std::string("ArmUp") },
	{ ReagentController::OperationSequence::ArmDown, std::string("Armdown") },
	{ ReagentController::OperationSequence::ArmPurge, std::string("ArmPurge") },
	{ ReagentController::OperationSequence::Enable, std::string("Enable") },
	{ ReagentController::OperationSequence::Disable, std::string("Disable") },
};

//*****************************************************************************
ReagentController::ReagentController(std::shared_ptr<CBOService> pCBOService)
	: ReagentControllerBase(pCBOService)
	, positionTolerance_(DefaultReagentArmPositionTolerance)
	, closing_(false)
	, controllerOk_(false)
	, configFile_("")
	, updateConfig_(false)
	, armHomePos_(0)
	, armHomeOffset_(0)
	, armDownPos_(0)
	, armDownOffset_(0)
	, armMaxTravel_(ReagentArmMaxTravel)
	, armPurgePosition_(0)
	
{
	reagentMotor_.reset (new MotorBase (pCBOService_, MotorTypeId::MotorTypeReagent));
	pRfid_ = std::make_shared<Rfid>(pCBOService);
	doorLatchTimer_ = std::make_shared <boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
	boardTimestampTimer_ = std::make_shared <boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
}
//*****************************************************************************
ReagentController::~ReagentController()
{
    Quit();
}

//*****************************************************************************
void ReagentController::Initialize (std::function<void(bool)> callback, std::string cfgFile, bool apply)
{
	boost::system::error_code ec;
	std::string controllerName = CONTROLLERNODENAME;


	if (cfgFile.empty())
	{
		HAWKEYE_ASSERT (MODULENAME, false);
	}

	if (controllerConfigNode_) {
		pCBOService_->enqueueExternal (callback, true); // Configuration already loaded.
		return;
	}

	t_pPTree rootPTree = ConfigUtils::OpenConfigFile (cfgFile, ec, true);
	if (!rootPTree)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile_ + "\": - file could not be opened");
		reportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror, 
			instrument_error::instrument_storage_instance::motor_info, 
			instrument_error::severity_level::error));
		pCBOService_->enqueueExternal (callback, false);
		return;
	}

	configFile_ = cfgFile;

	configNode_ = rootPTree->get_child_optional ("config");
	if (!configNode_)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile_ + "\" - \"config\" section not found");
		reportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror, 
			instrument_error::instrument_storage_instance::motor_info, 
			instrument_error::severity_level::error));
		pCBOService_->enqueueExternal (callback, false);
		return;
	}
	else
	{
		// Look for the controllers section for individualized parameters.
		controllersNode_ = configNode_->get_child_optional ("motor_controllers");
		if (controllersNode_)
		{
			controllerNode_ = controllersNode_->get_child_optional (CONTROLLERNODENAME);
			if (controllerNode_)
			{
				// Look for the motor parameters sub-node.
				motorParamsNode_ = controllerNode_->get_child_optional("motor_parameters");
				if (!motorParamsNode_)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile_ + "\" - \"motor_parameters\" section not found");
					reportError (BuildErrorInstance(
						instrument_error::instrument_storage_readerror, 
						instrument_error::instrument_storage_instance::motor_info, 
						instrument_error::severity_level::error));
					pCBOService_->enqueueExternal (callback, false);
					return;
				}
				else
				{
					// Look for the controller configuration parameters.
					controllerConfigNode_ = controllerNode_->get_child_optional ("controller_config");
					if (!controllerConfigNode_)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Error parsing configuration file \"" + configFile_ + "\" - \"controller_config\" section not found");
						reportError (BuildErrorInstance(
							instrument_error::instrument_storage_readerror, 
							instrument_error::instrument_storage_instance::motor_info, 
							instrument_error::severity_level::error));
						pCBOService_->enqueueExternal (callback, false);
						return;
					}
				}
			}
		}
	}

	reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);

	auto onInitComplete = [this, callback](bool status) -> void
	{ 
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to initialize Reagent controller");
			reportError(BuildErrorInstance(
				instrument_error::motion_motor_initerror, 
				instrument_error::motion_motor_instances::reagent_probe, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueExternal (callback, false);
			return;
		}

		if (controllersNode_)
		{
			ConfigControllerVariables();
			if (armPurgePosition_ == 0)        // check for missing config file variable and update file if needed
			{
				armPurgePosition_ = ReagentArmPurgePosition;
				updateConfig_ = true;
				WriteControllerConfig();
			}
		}

		reagentMotor_->SetMotorTimeouts (reagentStartTimeout_msec, reagentBusyTimeout_msec);
		GetReagentArmPosition();
		reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		// First time calling this, invoke the Initialization completion callback and then set up the regular schedule for the timer.
		pRfid_->setTime ([this, callback](bool status) -> void
		{
			pCBOService_->enqueueExternal (callback, status);

			boardTimestampTimer_->expires_from_now(boost::posix_time::hours(1));
			boardTimestampTimer_->async_wait([this](boost::system::error_code ec)->void { SetControllerTimestamp(ec); });
		});
	};

	reagentMotor_->Init (onInitComplete, rootPTree, motorParamsNode_, apply);
}

void ReagentController::SetControllerTimestamp(boost::system::error_code ec)
{
	// If cancelled or failure in ASIO, then do not reschedule.
	if (ec)
		return;

	// Update the timestamp and schedule the next checkin for an hour later.
	pRfid_->setTime([this](bool status) -> void
	{
		if (status)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "Reagent controller timestamp updated");
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to update time on Reagent controller");
		}
		boardTimestampTimer_->expires_from_now(boost::posix_time::hours(1));
		boardTimestampTimer_->async_wait([this](boost::system::error_code ec)->void { SetControllerTimestamp(ec); });
	});

}

//*****************************************************************************
// check a position for coincidence with a supplied target using supplied or default tolerances
//*****************************************************************************
void ReagentController::ConfigControllerVariables()
{
    boost::property_tree::ptree& config = controllerConfigNode_.get();

    positionTolerance_ = config.get<int32_t>( "PositionTolerance", positionTolerance_);
    armHomePos_ = config.get<int32_t>( "ReagentHomePos", armHomePos_);
    armHomeOffset_ = config.get<int32_t>( "ReagentHomePosOffset", armHomeOffset_);
    armDownPos_ = config.get<int32_t>( "ReagentDownPos", armDownPos_);
    armDownOffset_ = config.get<int32_t>( "ReagentDownPosOffset", armDownOffset_);
    armMaxTravel_ = config.get<int32_t>( "ReagentArmMaxTravel", armMaxTravel_);
    armPurgePosition_ = config.get<int32_t>( "ReagentPurgePosition", armPurgePosition_);
    reagentStartTimeout_msec = TIMEOUT_FACTOR * config.get<int32_t>( "ReagentStartTimeout", reagentStartTimeout_msec);
    reagentBusyTimeout_msec = TIMEOUT_FACTOR * config.get<int32_t>( "ReagentBusyTimeout", reagentBusyTimeout_msec);
}

//*****************************************************************************
void ReagentController::WriteControllerConfig()
{
    // Unable to save through this method if no configuration location was given!
    if (!controllerConfigNode_)
    {
        updateConfig_ = false;
        return;
    }

    // Update our pTree with the new values
    if (UpdateControllerConfig())
    {
        // Flush the cached file to disk
        boost::system::error_code ec;
        ConfigUtils::WriteCachedConfigFile (configFile_, ec);
        if (ec)
        {
			Logger::L().Log (MODULENAME, severity_level::error, "failed to write controller configuration");
			reportError(BuildErrorInstance(
				instrument_error::instrument_storage_writeerror, 
				instrument_error::instrument_storage_instance::motor_config, 
				instrument_error::severity_level::error));
        }
    }
    else
    {
		Logger::L().Log (MODULENAME, severity_level::error, "failed to update controller configuration");
		reportError(BuildErrorInstance(
			instrument_error::instrument_storage_writeerror, 
			instrument_error::instrument_storage_instance::motor_config, 
			instrument_error::severity_level::notification));
    }

    return;
}

//*****************************************************************************
bool ReagentController::UpdateControllerConfig()
{
	if (!configNode_) {
		Logger::L().Log (MODULENAME, severity_level::error, "controller configuration is empty");
		return false;
	}

    // config_node points at the "xxx_controller.controller_config" node for this particular controller... ( should be the same as the preserved 'controllerConfigNode' )
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

//    configNode->put<int32_t>( "PositionTolerance", positionTolerance_);
//    configNode->put<int32_t>( "ReagentHomePos", armHomePos_);
//    configNode->put<int32_t>( "ReagentHomePosOffset", armHomeOffset_);
//    configNode->put<int32_t>( "ReagentDownPos", armDownPos_);
//    configNode->put<int32_t>( "ReagentDownPosOffset", armDownOffset_);
//    configNode->put<int32_t>( "ReagentArmMaxTravel", armMaxTravel_);

	configNode_->put<int32_t>("ReagentPurgePosition", armPurgePosition_);

//    configNode.put<int32_t>( "ReagentStartTimeout", reagentStartTimeout_msec);
//    configNode.put<int32_t>( "ReagentFullTimeout", reagentFullTimeout_msec);

    controllerNode_->put_child ("controller_config", *configNode_);

    return true;
}

//*****************************************************************************
int32_t ReagentController::GetReagentArmPosition()
{
	int32_t armPos = reagentMotor_->GetPosition();
    Logger::L().Log (MODULENAME, severity_level::debug1, "GetReagentArmPosition: armPos: " + std::to_string(armPos));

    return armPos;
}

//*****************************************************************************
void ReagentController::Quit()
{
    closing_ = true;

    if (updateConfig_)
    {
        WriteControllerConfig();
    }

	boardTimestampTimer_->cancel();

	reagentMotor_->Quit();

    configNode_.reset();
    controllersNode_.reset();
    controllerConfigNode_.reset();
    motorParamsNode_.reset();

    reagentMotorStatus_.reset();
	boardTimestampTimer_.reset();
}

//*****************************************************************************
void ReagentController::Reset()
{
}

//*****************************************************************************
void ReagentController::ClearErrors(std::function<void(bool)> callback) {
	
	reagentMotor_->ClearErrors(callback);
}

//*****************************************************************************
bool ReagentController::ControllerOk()
{
    return controllerOk_;
}

//*****************************************************************************
void ReagentController::Stop()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <enter>");

	std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
	asyncTaskList.clear();

	// add clear errors for all the motors to list
	asyncTaskList.push_back([this](auto cb) { this->ClearErrors(cb); });

	if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::ReagentMotorBusy))
	{
		// add probe clear error to list
		asyncTaskList.push_back([this](auto cb) { reagentMotor_->Stop(cb, false, 1000); });
	}

	AsyncCommandHelper::queueASynchronousTask(pCBOService_->getInternalIos(), [this](bool status)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <exit>");
		reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, reagentMotor_->GetPosition(), ePositionDescription::Current);
	}, asyncTaskList, false);
}

//*****************************************************************************
bool ReagentController::IsDoorClosed() {

	return pCBOService_->CBO()->GetSignalStatus().isSet (SignalStatus::ReagentDoorClosed);
}

//*****************************************************************************
bool ReagentController::IsPackInstalled() {

	return pCBOService_->CBO()->GetSignalStatus().isSet (SignalStatus::ReagentPackInstalled);
};

//*****************************************************************************
bool ReagentController::IsHome() {

	bool homeOk = (reagentMotor_->GetHomeStatus() && reagentMotor_->IsHome());

	std::string logStr = boost::str(boost::format("homeOk <home status & sensor occlusion>: %s, ") % ((homeOk == true) ? "true" : "false"));
	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("IsHome: %s \t %s") % ((homeOk == true) ? "true" : "false") % logStr));

	return homeOk;
}

//*****************************************************************************
bool ReagentController::IsUp()
{
    return IsHome();
}

//*****************************************************************************
bool ReagentController::IsDown()
{
	return pCBOService_->CBO()->GetSignalStatus().isSet (SignalStatus::SignalStatus::ReagentMotorLimit);
}

//*****************************************************************************
void ReagentController::GetMotorStatus (MotorStatus& reagentMotorStatus) {

	reagentMotorStatus_.ToCStyle (reagentMotorStatus);
}

//*****************************************************************************
// Must use relative position change, since the machine may be power-cycled
// between arm-down and purge operations.
//*****************************************************************************
void ReagentController::ArmPurge (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onArmPurgeComplete = [this, callback](bool status) -> void {
		if (!status) {
			Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor not at purge position");
			pCBOService_->enqueueExternal (callback, status);
			return;
		}

		pCBOService_->getInternalIosRef().post ([this, callback]() -> void {
			auto onEnableComplete = [this, callback](bool status) -> void {
				if (!status) {
					Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor holding current not enabled");
				}
				pCBOService_->enqueueExternal (callback, status);
			};

			processAsync (onEnableComplete, OperationSequence::Enable);
		});	
	};

	pCBOService_->getInternalIosRef().post ([this, callback, onArmPurgeComplete]() -> void {
		processAsync (onArmPurgeComplete, OperationSequence::ArmPurge);
	});
}

//*****************************************************************************
void ReagentController::ArmHome (std::function<void(bool)> callback) {
	ArmUp (callback);
}

//*****************************************************************************
void ReagentController::ArmUp (std::function<void(bool)> callback) {
	
	HAWKEYE_ASSERT (MODULENAME, callback);

	bool upOk = IsHome() && IsUp();
	if (upOk) {
		pCBOService_->enqueueExternal (callback, upOk);
		return;
	}

	auto onArmUpComplete = [this, callback](bool status) -> void {
		if (!status) {
			Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor not at home sensor");
			pCBOService_->enqueueExternal (callback, status);
			return;
		}

		pCBOService_->getInternalIosRef().post ([this, callback]() -> void {
			auto onEnableComplete = [this, callback](bool status) -> void {
				if (!status) {
					Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor holding current not enabled");
				}
				pCBOService_->enqueueExternal (callback, status);
			};

			processAsync (onEnableComplete, OperationSequence::Enable);
		});
	};

	pCBOService_->getInternalIosRef().post ([this, onArmUpComplete]() -> void {
		processAsync (onArmUpComplete, OperationSequence::ArmUp);
	});
}

//*****************************************************************************
void ReagentController::ArmDown (std::function<void(bool)> callback) {
	
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (IsDown()) {
		pCBOService_->enqueueExternal (callback, true);
		return;
	}

	// Safety: Do not allow positive/downward moves if the door is open (USER HAZARD) or pack is not installed (DON'T ACCIDENTALLY PIERCE PACK)
	if (!IsDoorClosed() || !IsPackInstalled())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "ArmDown: downward move prohibited if door open or pack not installed.");
		pCBOService_->enqueueExternal (callback, false);
		return;
	}

	auto onArmDownComplete = [this, callback](bool status) -> void {
		if (!status) {
			Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor not at limit sensor");
			pCBOService_->enqueueExternal (callback, status);
			return;
		}

		pCBOService_->getInternalIosRef().post ([this, callback]() -> void {
			auto onEnableComplete = [this, callback](bool status) -> void {
				if (!status) {
					Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor holding current not enabled");
				}
				pCBOService_->enqueueExternal (callback, status);
			};

			processAsync (onEnableComplete, OperationSequence::Enable);
		});
	};

	pCBOService_->getInternalIosRef().post ([this, callback, onArmDownComplete]() -> void {
		processAsync (onArmDownComplete, OperationSequence::ArmDown);
	});
}

//*****************************************************************************
void ReagentController::UnlatchDoor (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "UnlatchDoor : <enter>");

	if (!IsHome()) {
		Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor not at home sensor");
		pCBOService_->enqueueExternal (callback, false);
		return;
	}

	auto onUnlatchComplete = [this, callback](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug1, 
			boost::str (boost::format ("onUnlatchComplete: <callback, %s>") % (cbData.status == ControllerBoardOperation::Success ? "success" : "failure")));

		bool status = false;
		if (cbData.status != ControllerBoardOperation::Success) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("UnlatchDoor::onUnlatchComplete: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			pCBOService_->enqueueExternal (callback, status);
			return;
		}

		// Set timer to wait a short period for the door to spring open before resetting the latch.
		doorLatchTimer_->expires_from_now(boost::posix_time::milliseconds(3000));

		boost::system::error_code ec;
		doorLatchTimer_->async_wait ([this, callback](const boost::system::error_code& ec) -> void {
			auto onLatchComplete = [this, callback](ControllerBoardOperation::CallbackData_t cbData) -> void {
				Logger::L().Log (MODULENAME, severity_level::debug1,
					 boost::str (boost::format ("onLatchComplete: <callback, %s>") % (cbData.status == ControllerBoardOperation::Success ? "success" : "failure")));

				bool status = false;
				if (cbData.status == ControllerBoardOperation::Success) {
					status = true;
				} else {
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("UnlatchDoor::onUnlatchComplete: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
				}
				pCBOService_->enqueueExternal (callback, status);
			};

			auto tid = pCBOService_->CBO()->Execute(LatchDoorOperation(), 3000, onLatchComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("LatchDoor, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "LatchDoor: CBO Execute failed");
				pCBOService_->enqueueExternal (callback, false);
			}
		});
	};
	
	auto tid = pCBOService_->CBO()->Execute(UnlatchDoorOperation(), 3000, onUnlatchComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("UnlatchDoor, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "UnlatchDoor: CBO Execute failed");
		pCBOService_->enqueueExternal (callback, false);
	}
}

//*****************************************************************************
void ReagentController::MoveReagentPositionRelative (std::function<void(bool)> callback, int32_t moveStep) {

	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "MoveReagentPositionRelative: <enter, moveStep: " + std::to_string(moveStep) + ">");

	bool isDoorOpen = !IsDoorClosed();
	bool isPackNotPresent = !IsPackInstalled();
	// Safety: Do not allow positive/downward moves if the door is open (USER HAZARD) or pack is not installed (DON'T ACCIDENTALLY PIERCE PACK)
	if (moveStep > 0 && (isDoorOpen || isPackNotPresent)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "MoveReagentPositionRelative: downward move prohibited if door open or pack installed.");
		if (isDoorOpen)
		{
			reportError(BuildErrorInstance(
				instrument_error::instrument_precondition_notmet, 
				instrument_error::instrument_precondition_instance::reagent_door, 
				instrument_error::severity_level::warning));
		}
		if (isPackNotPresent)
		{
			reportError(BuildErrorInstance(
				instrument_error::reagent_pack_nopack,
				instrument_error::reagent_pack_instance::general, 
				instrument_error::severity_level::warning));
		}
		pCBOService_->enqueueExternal (callback, false);
		return;
	}

	reagentMotor_->ResumePositionUpdates (ReagentPositionPositionInterval_ms);

	pCBOService_->getInternalIosRef().post ([this, callback, moveStep]() -> void {
		int32_t startPosition = GetReagentArmPosition();
		int32_t targetPosition = startPosition + moveStep;
		reagentMotor_->MovePosRelative ([this, callback, startPosition, targetPosition](bool status) -> void {
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("reagentMotor_->MovePosRelative: %s with tgtArmPos <%d> , currentArmPos <%d>")\
				% (status ? "true" : "false")
				% targetPosition
				% startPosition));

			reagentMotor_->PausePositionUpdates();

			pCBOService_->enqueueExternal (callback, status);

		}, moveStep, getTimeoutInMs());
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "MoveReagentPositionRelative : <Exit>");
}

//*****************************************************************************
void ReagentController::MoveToPosition (std::function<void(bool)> callback, int32_t targetPosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "MoveToPosition: <enter, targetPosition: " + std::to_string(targetPosition) + ">");

	// Safety: Do not allow positive/downward moves if the door is open (USER HAZARD) or pack is not installed (DON'T ACCIDENTALLY PIERCE PACK)
	if ((targetPosition > reagentMotor_->GetPosition()) && (!IsDoorClosed() || !IsPackInstalled()))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "MoveToPosition: downward move prohibited if door open or pack installed.");
		pCBOService_->enqueueExternal (callback, false);
		return;
	}

	reagentMotor_->ResumePositionUpdates (ReagentPositionPositionInterval_ms);

	pCBOService_->getInternalIosRef().post ([this, callback, targetPosition]() -> void {
		reagentMotor_->SetPosition ([this, callback, targetPosition](bool status) -> void {
			std::string logStr = boost::str (boost::format ("reagentMotor_->SetPosition: %s targetPosition <%d> , currentPosition <%d>")\
				% (status ? "true" : "false")
				% targetPosition
				% GetReagentArmPosition());

			Logger::L().Log (MODULENAME, severity_level::debug1, logStr);

			reagentMotor_->PausePositionUpdates();

			pCBOService_->enqueueExternal (callback, status);

		}, targetPosition, getTimeoutInMs());
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "moveToPositionAsync : <Exit>");
}

//*****************************************************************************
void ReagentController::processAsync (std::function<void(bool)> callback, OperationSequence sequence) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "processAsync : " + EnumConversion<OperationSequence>::enumToString(sequence));

	switch (sequence)
	{
		case OperationSequence::ArmUp:
		{
			reagentMotor_->ResumePositionUpdates (ReagentPositionPositionInterval_ms);

			reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfInMotion, 0, ePositionDescription::Target);

			reagentMotor_->Home ([this, callback](bool status) -> void
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, 
					boost::str (boost::format ("reagentMotor_->Home: %s") % (status ? "success" : "failure")));

				if (!status) {
					Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor up failed");
					reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfErrorState, reagentMotor_->GetPosition(), ePositionDescription::Current);
					pCBOService_->enqueueExternal (callback, status);
					return;
				} 

				pCBOService_->getInternalIosRef().post ([this, callback]() -> void {
					auto onMoveRelativeComplete = [this, callback](bool status) -> void {
						reagentMotor_->PausePositionUpdates();

						if (!status) {
							Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor to position failed");
							reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfErrorState, reagentMotor_->GetPosition(), ePositionDescription::Current);
						} else {
							reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfAtPosition, reagentMotor_->GetPosition(), ePositionDescription::AtPosition);
						}

						pCBOService_->enqueueExternal (callback, status);
					};

					MoveReagentPositionRelative (onMoveRelativeComplete, armHomeOffset_);
				});

			}, getTimeoutInMs());

			break;
		}

		case OperationSequence::ArmDown:
		{
			if (IsDown())
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "Reagent Arm is already at down position");
				pCBOService_->enqueueExternal (callback, true);
				return;
			}
			reagentMotor_->ResumePositionUpdates (ReagentPositionPositionInterval_ms);

			reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfInMotion, 0, ePositionDescription::Target);

			reagentMotor_->GotoTravelLimit ([this, callback](bool status) -> void
			{
				Logger::L().Log (MODULENAME, severity_level::debug1,
					boost::str (boost::format ("reagentMotor_->GotoTravelLimit: %s") % (status ? "success" : "failure")));

				reagentMotor_->PausePositionUpdates();

				if (!status) {
					Logger::L().Log (MODULENAME, severity_level::error, "Reagent motor down failed");
					reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfErrorState, reagentMotor_->GetPosition(), ePositionDescription::Current);
				} else {
					reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfAtPosition, reagentMotor_->GetPosition(), ePositionDescription::AtPosition);
				}

				pCBOService_->enqueueExternal (callback, status);
			}, getTimeoutInMs());

			break;
		}

		case OperationSequence::ArmPurge:
		{
			auto onMoveRelativeComplete = [this, callback](bool status) -> void
			{
				reagentMotor_->PausePositionUpdates();

				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "processAsync : OperationSequence::ArmPurge : Reagent arm purge operation failed!");
					reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, reagentMotor_->GetPosition(), ePositionDescription::Current);
				}
				else
				{
					reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, reagentMotor_->GetPosition(), ePositionDescription::AtPosition);
				}

				pCBOService_->enqueueExternal (callback, status);
			};

			Logger::L().Log (MODULENAME, severity_level::debug1, "processAsync : OperationSequence::ArmPurge : Moving reagent arm to down position!");

			// Do arm down first
			processAsync([this, onMoveRelativeComplete, callback](bool downOk)
			{
				reagentMotor_->PausePositionUpdates();

				if (!downOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "processAsync : OperationSequence::ArmPurge : Reagent arm down move failed!");
					pCBOService_->enqueueExternal (callback, downOk);
					return;
				}

				reagentMotor_->ResumePositionUpdates(ReagentPositionPositionInterval_ms);
				reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, 0, ePositionDescription::Target);

				pCBOService_->enqueueInternal(
					std::bind(&ReagentController::MoveReagentPositionRelative, this, onMoveRelativeComplete, armPurgePosition_));

			}, ReagentController::OperationSequence::ArmDown);

			break;
		}

		case OperationSequence::Enable:
		{
			reagentMotor_->Enable ([this, callback](bool status) -> void
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, 
					boost::str (boost::format("reagentMotor_->Enable: %s") % (status ? "success" : "failure")));

				pCBOService_->enqueueInternal (callback, status);
			}, true, getTimeoutInMs());

			break;
		}

		case OperationSequence::Disable:
		{
			reagentMotor_->Enable ([this, callback](bool status) -> void
			{
				Logger::L().Log (MODULENAME, severity_level::debug1,
					boost::str (boost::format("reagentMotor_->Enable: %s") % (status ? "success" : "failure")));

				pCBOService_->enqueueInternal (callback, status);
			}, false, getTimeoutInMs());

			break;
		}

	} // End "switch (sequence)"
}

//*****************************************************************************
void ReagentController::reportError (uint32_t errorCode) {
	ReportSystemError::Instance().ReportError(errorCode);
}
