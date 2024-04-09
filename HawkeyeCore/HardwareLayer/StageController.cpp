#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <boost/format.hpp>

#include "Configuration.hpp"
#include "DBif_Api.h"
#include "EnumConversion.hpp"
#include "GetAsStrFunctions.hpp"
#include "HawkeyeDirectory.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "StageController.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "StageController";
static const char CAROUSELCONTROLLERNODENAME[] = "carousel_controller";
static const char PLATECONTROLLERNODENAME[] = "plate_controller";

const int32_t DefaultPositionTolerance = 1000;    // allow 0.1 mm of inaccuracy?
const int32_t PositionalTryCnt = 10;              // HACK: allow up to 10 tries to read the position to allow it to catch up and stabilize

// NOTE: the motor control parameter for retries should not be used for initial initialization
//       retry attempts, as it will be 0 for many normal use-case conditions
const int InitTryCnt = 5;		// # of times to perform the external retry operations
const int InitRetryCnt = 4;		// # of internal retries; this will allow 5 total internal retries

///////////////////////////////////////////////////////////////////////////////
////////////////////////Initialize Enums///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const std::map<StageController::ThetaMotorOperationSequence, std::string>
EnumConversion<StageController::ThetaMotorOperationSequence>::enumStrings<StageController::ThetaMotorOperationSequence>::data =
{
	{ StageController::ThetaMotorOperationSequence::th_mo_entry_point, std::string("Theta_Sequence_Entry_Point") },
	{ StageController::ThetaMotorOperationSequence::th_mo_clear_error, std::string("Theta_Sequence_Clear_Error") },
	{ StageController::ThetaMotorOperationSequence::th_mo_can_do_work, std::string("Theta_Sequence_Can_Do_Work") },
	{ StageController::ThetaMotorOperationSequence::th_mo_do_work, std::string("Theta_Sequence_Do_Work") },
	{ StageController::ThetaMotorOperationSequence::th_mo_do_work_wait, std::string("Theta_Sequence_Wait_For_Work") },
	{ StageController::ThetaMotorOperationSequence::th_mo_cancel, std::string("Theta_Sequence_Cancel") },
	{ StageController::ThetaMotorOperationSequence::th_mo_complete, std::string("Theta_Sequence_Complete") },
	{ StageController::ThetaMotorOperationSequence::th_mo_timeout, std::string("Theta_Sequence_TimedOut") },
	{ StageController::ThetaMotorOperationSequence::th_mo_error, std::string("Theta_Sequence_Error") }
};

const std::map<StageController::RadiusMotorOperationSequence, std::string>
EnumConversion<StageController::RadiusMotorOperationSequence>::enumStrings<StageController::RadiusMotorOperationSequence>::data =
{
	{ StageController::RadiusMotorOperationSequence::ra_mo_entry_point, std::string("Radius_Sequence_Entry_Point") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_clear_error, std::string("Radius_Sequence_Clear_Error") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_can_do_work, std::string("Radius_Sequence_Can_Do_Work") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_do_work, std::string("Radius_Sequence_Do_Work") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_do_work_wait, std::string("Radius_Sequence_Wait_For_Work") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_cancel, std::string("Radius_Sequence_Cancel") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_complete, std::string("Radius_Sequence_Complete") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_timeout, std::string("Radius_Sequence_TimedOut") },
	{ StageController::RadiusMotorOperationSequence::ra_mo_error, std::string("Radius_Sequence_Error") }
};

const std::map<StageController::ProbeOperationSequence, std::string>
EnumConversion<StageController::ProbeOperationSequence>::enumStrings<StageController::ProbeOperationSequence>::data =
{
	{ StageController::ProbeOperationSequence::po_entry_point, std::string("Probe_Sequence_Entry_Point") },
	{ StageController::ProbeOperationSequence::po_clear_error, std::string("Probe_Sequence_Clear_Error") },
	{ StageController::ProbeOperationSequence::po_can_do_work, std::string("Probe_Sequence_Can_Do_Work") },
	{ StageController::ProbeOperationSequence::po_do_work, std::string("Probe_Sequence_Do_Work") },
	{ StageController::ProbeOperationSequence::po_wait_for_work, std::string("Probe_Sequence_Wait_For_Work") },
	{ StageController::ProbeOperationSequence::po_complete, std::string("Probe_Sequence_Complete") },
	{ StageController::ProbeOperationSequence::po_timeout, std::string("Probe_Sequence_TimedOut") },
	{ StageController::ProbeOperationSequence::po_error, std::string("Probe_Sequence_Error") }
};

const std::map<StageController::StageControllerOperationSequence, std::string>
EnumConversion<StageController::StageControllerOperationSequence>::enumStrings<StageController::StageControllerOperationSequence>::data =
{
	{ StageController::StageControllerOperationSequence::sc_entry_point, std::string("StageController_Sequence_Entry_Point") },
	{StageController::StageControllerOperationSequence::sc_move_probe, std::string("StageController_Sequence_Move_Probe")},
	{ StageController::StageControllerOperationSequence::sc_update_values, std::string("StageController_Sequence_Update_Values") },
	{ StageController::StageControllerOperationSequence::sc_move_radius_chk, std::string("StageController_Sequence_Move_Radius_Chk") },			// state is not currently used
	{ StageController::StageControllerOperationSequence::sc_move_radius, std::string("StageController_Sequence_Move_Radius") },
	{ StageController::StageControllerOperationSequence::sc_check_retry_on_timeout_radius, std::string("StageController_Sequence_CheckRetryOnTimeout_Radius") },
	{ StageController::StageControllerOperationSequence::sc_move_theta_chk, std::string("StageController_Sequence_Move_Theta_Chk") },			// state is not currently used
	{ StageController::StageControllerOperationSequence::sc_move_theta, std::string("StageController_Sequence_Move_Theta") },
	{ StageController::StageControllerOperationSequence::sc_check_retry_on_timeout_theta, std::string("StageController_Sequence_CheckRetryOnTimeout_Theta") },
	{ StageController::StageControllerOperationSequence::sc_cancel, std::string("StageController_Sequence_Cancel") },
	{ StageController::StageControllerOperationSequence::sc_complete, std::string("StageController_Sequence_Complete") },
	{ StageController::StageControllerOperationSequence::sc_timeout, std::string("StageController_Sequence_TimedOut") },
	{ StageController::StageControllerOperationSequence::sc_error, std::string("StageController_Sequence_Error") }
};

const std::map<StageController::CarouselOperationSequence, std::string>
EnumConversion<StageController::CarouselOperationSequence>::enumStrings<StageController::CarouselOperationSequence>::data =
{
	{ StageController::CarouselOperationSequence::cc_entry_point, std::string("Carousel_Sequence_Entry_Point") },
	{ StageController::CarouselOperationSequence::cc_clear_error, std::string("Carousel_Sequence_Clear_Error") },
	{ StageController::CarouselOperationSequence::cc_do_work, std::string("Carousel_Sequence_Do_Work") },
	{ StageController::CarouselOperationSequence::cc_wait_for_work, std::string("Carousel_Sequence_Wait_For_Work") },
	{ StageController::CarouselOperationSequence::cc_cancel, std::string("Carousel_Sequence_Cancel") },
	{ StageController::CarouselOperationSequence::cc_no_tube_found, std::string("Carousel_Sequence_No_Tube_Found") },
	{ StageController::CarouselOperationSequence::cc_complete, std::string("Carousel_Sequence_Complete") },
	{ StageController::CarouselOperationSequence::cc_timeout, std::string("Carousel_Sequence_TimedOut") },
	{ StageController::CarouselOperationSequence::cc_error, std::string("Carousel_Sequence_Error") },
};


///////////////////////////////////////////////////////////////////////////////
////////////////////////Private Structures/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// The structure encapsulates the parameters/method required to move stage
/// </summary>
struct MoveStageParams
{
	/// <summary>
	/// Functor with signature [void(*)(bool)] to get retry status
	/// </summary>
	typedef std::function<void(bool)> retryCb;

	/// <summary>
	/// constructor to create instance of <see cref="MoveStageParams"/>
	/// </summary>
	/// <param name="retryLambda">The functor to check if retry is possible</param>
	/// <param name="targetPos">The target position of motor</param>
	/// <param name="timeout_Secs">The timeout for motor movement</param>
	/// <param name="tolerance">The tolerance level to indicate the motor is in target position</param>
	/// <param name="applyDeadBand">The dead band setting parameter</param>
	MoveStageParams(std::function<bool(uint16_t)> retryLambda,
					int32_t targetPos, int32_t timeout_Secs, int32_t tolerance = 0, bool applyDeadBand = false)
	{
		this->targetPos = targetPos;
		this->timeout_Secs = timeout_Secs;
		this->tolerance = tolerance;
		this->applyDeadBand = applyDeadBand;
		this->retryLambda_ = retryLambda;
	}

	int32_t targetPos;
	int32_t tolerance;
	bool applyDeadBand;
	int32_t timeout_Secs;

	/// <summary>
	/// Call this method to check if retry is possible
	/// </summary>
	/// <param name="onComplete">Callback to indicate caller if retry is possible</param>
	/// <param name="retriesDone">The input parameter indicating about previous retries done</param>
	bool canRetry(uint16_t retriesDone) const
	{
		Logger::L().Log ("StageController", severity_level::debug1, "canRetry : retriesDone : " + std::to_string(retriesDone));
		if (retryLambda_ != nullptr)
		{
			return retryLambda_(retriesDone);
		}
		else
		{
			return false;
		}
	}

private:
	std::function<bool(uint16_t)> retryLambda_;
};


///////////////////////////////////////////////////////////////////////////////
////////////////////////Theta Motor Methods////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Call this method to reinitialize theta motor
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::ReinitTheta()
{
	// Check if stage controller is before applying default parameters
	if (ControllerOk())
	{
		thetaMotor->ApplyDefaultParams();
		Logger::L().Log (MODULENAME, severity_level::debug2, "ReinitTheta: apply default prams");
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetThetaPosition(), ePositionDescription::Current);
		return true;
	}
	return false;
}

/// <summary>
/// Call this method to update theta properties <see cref="StageController::ThetaProperties"/>
/// from motor info file
/// </summary>
/// <param name="config">The input ptree pointing to theta properties in motor info file</param>
/// <param name="eCarrierType">The input stage controller type</param>
/// <param name="defaultValues">The default stage controller theta properties</param>
/// <returns>The theta properties for input stage controller type</returns>
StageController::ThetaProperties StageController::ConfigThetaVariables(
	boost::property_tree::ptree& config, 
	eCarrierType carrierType,
	boost::optional<StageController::ThetaProperties> defaultValues)
{
	ThetaProperties tp;
	if (defaultValues)
	{
		tp = defaultValues.get();
	}
	else
	{
		tp = getDefaultThetaProperties(carrierType);
	}

	const DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();

	if (carrierType == eCarrierType::eCarousel)
	{	
		tp.thetaHomePosOffset = instCfg.CarouselThetaHomeOffset;

		tp.thetaPositionTolerance = config.get<int32_t>("CarouselPositionTolerance", tp.thetaPositionTolerance);
		tp.thetaHomePos = config.get<int32_t>("CarouselThetaHomePos", tp.thetaHomePos);
		
		tp.thetaBacklash = config.get<int32_t>("CarouselThetaBacklash", tp.thetaBacklash);

		tp.thetaStartTimeout_msec /= TIMEOUT_FACTOR;
		tp.thetaStartTimeout_msec = config.get<int32_t>("ThetaStartTimeout", tp.thetaStartTimeout_msec) * TIMEOUT_FACTOR;

		tp.thetaFullTimeout_msec /= TIMEOUT_FACTOR;
		tp.thetaFullTimeout_msec = config.get<int32_t>("ThetaFullTimeout", tp.thetaFullTimeout_msec) * TIMEOUT_FACTOR;
	}
	else if (carrierType == eCarrierType::ePlate_96)
	{
		tp.thetaHomePosOffset = instCfg.PlateThetaHomeOffset;
		tp.thetaCalPos = instCfg.PlateThetaCalPos;

		tp.thetaPositionTolerance = config.get<int32_t>("PlateThetaPositionTolerance", tp.thetaPositionTolerance);	
		tp.thetaBacklash = config.get<int32_t>("PlateThetaBacklash", tp.thetaBacklash);
		tp.thetaStartTimeout_msec /= TIMEOUT_FACTOR;
		tp.thetaStartTimeout_msec = config.get<int32_t>("ThetaStartTimeout", tp.thetaStartTimeout_msec) * TIMEOUT_FACTOR;

		tp.thetaFullTimeout_msec /= TIMEOUT_FACTOR;
		tp.thetaFullTimeout_msec = config.get<int32_t>("ThetaFullTimeout", tp.thetaFullTimeout_msec) * TIMEOUT_FACTOR;
	}

	return tp;
}

/// <summary>
/// Call this method to update motor info file from
/// theta properties <see cref="StageController::ThetaProperties"/>
/// </summary>
/// <param name="config">The input ptree pointing to theta properties in motor info file</param>
/// <param name="tp">The input theta properties to be updated</param>
/// <param name="carrierType">The input stage controller type</param>
/// <returns><c>[0]</c> if success; otherwise failed</returns>
int32_t StageController::UpdateThetaControllerConfig (boost::property_tree::ptree& config, const ThetaProperties& tp, eCarrierType carrierType)
{
	DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();
	if (carrierType == eCarrierType::eCarousel)
	{
		properties_[eCarrierType::eCarousel].thetaProperties.thetaHomePosOffset = tp.thetaHomePosOffset;

		instCfg.CarouselThetaHomeOffset = tp.thetaHomePosOffset;
		InstrumentConfig::Instance().Set();
	}
	else if (carrierType == eCarrierType::ePlate_96)
	{
		properties_[eCarrierType::ePlate_96].thetaProperties.thetaHomePosOffset = tp.thetaHomePosOffset;
		properties_[eCarrierType::ePlate_96].thetaProperties.thetaCalPos = tp.thetaCalPos;

		instCfg.PlateThetaHomeOffset = tp.thetaHomePosOffset;
		instCfg.PlateThetaCalPos = tp.thetaCalPos;
		InstrumentConfig::Instance().Set();
	}

	return 0;
}

/// <summary>
/// Call this method to get the theta backlash value.
/// </summary>
int32_t StageController::getThetaBacklash()
{
	return properties_[getCurrentType()].thetaProperties.thetaBacklash;
}

/// <summary>
/// Call this method to check if theta is at home position
/// </summary>
/// <returns><c>[true]</c> if all indicators show theta motor is at home; otherwise <c>[false]</c></returns>
bool StageController::ChkThetaHome(void)
{
	// get the theta position
	int32_t tPos = this->GetThetaPosition();

	// For theta the normalized home is set to be "0", so check if theta is actually at "0"
	// check if home status is not zero and home limit switch is true and position is the expected 0
	bool homeOk = this->ThetaAtPosition(tPos, 0, 0, true) && thetaMotor->GetHomeStatus() && thetaMotor->IsHome();

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("ChkThetaHome: %s") % ((homeOk == true) ? "true" : "false")));

	if (homeOk)
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, GetThetaPosition(), ePositionDescription::Home);
	}

	return homeOk;
}

/// <summary>
/// Call this method to check if theta motor is at given position
/// </summary>
/// <param name="pos">The current theta position</param>
/// <param name="targetPos">The target theta position</param>
/// <param name="tolerance">The theta position tolerance</param>
/// <param name="deadband">The dead band parameter <c>[true]</c>; if dead band needs to be applied</param>
/// <returns><c>[true]</c> if theta motor is with-in given limit; otherwise <c>[false]</c></returns>
bool StageController::ThetaAtPosition(int32_t pos, int32_t targetPos, int32_t tolerance, bool deadband)
{
	// get the max theta motor position
	int32_t maxThetaPos = getThetaProperties().maxThetaPos;

	// Normalize the input theta position
	normalizeThetaPosition(pos, maxThetaPos);

	// Normalize the target theta position
	normalizeThetaPosition(targetPos, maxThetaPos);
	int32_t tgtPos2 = targetPos;

	bool posOk = thetaMotor->PosAtTgt(pos, targetPos, tolerance, deadband);
	if (!posOk)
	{
		if (pos > targetPos)
		{
			tgtPos2 = targetPos + maxThetaPos;
		}
		else
		{
			pos += maxThetaPos;
		}
		posOk = thetaMotor->PosAtTgt(pos, tgtPos2, tolerance, deadband);
	}

	if (Logger::L().IsOfInterest(severity_level::debug2))
	{
		std::string logStr = boost::str(
			boost::format("ThetaAtPosition: posOk: %s with <theta position : %d> <targetPos : %d (%d)> <tolerance : %d> <deadband : %s>")
			% ((posOk == true) ? "true" : "false")
			% pos
			% targetPos
			% tgtPos2
			% tolerance
			% (deadband ? "true" : "false"));
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	return posOk;
}

/// <summary>
/// Call this method to get the cached theta motor position
/// </summary>
/// <returns>The cached theta motor position value</returns>
int32_t StageController::GetThetaPosition(void) const
{
	int32_t tPos = thetaMotor->GetPosition();
	normalizeThetaPosition(tPos, maxCarouselPos);
	return tPos;
}

/// <summary>
/// Normalize the negative theta position to fall within valid range
/// valid range : (0 : (Max Theta Position - 1)) <see cref="MaxThetaPosition"/>
/// </summary>
/// <remarks>Used mostly for "Carousel"</remarks>
/// <param name="currentPos">The reference of current theta position to be updated</param>
/// <param name="maxThetaPos">The maximum allowed theta position</param>
void StageController::normalizeThetaPosition(int32_t& currentPos, const int32_t& maxThetaPos)
{
	int32_t oldTargetPos = currentPos;
	while (currentPos < 0)
	{
		currentPos += maxThetaPos;
	}

	if (currentPos >= maxThetaPos)
	{
		currentPos %= maxThetaPos;
	}
}

/// <summary>
/// Initialize the theta motor asynchronously
/// </summary>
/// <param name="ex">The entry point for initialization enum states</param>
/// <param name="numRetries">The maximum number of retries allowed incase of original call failure</param>
void StageController::initThetaAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries)
{
	// get the current state
	auto curState = (ThetaMotorOperationSequence) ex.currentState;
	HawkeyeError he = HawkeyeError::eSuccess;
	std::string errStr = std::string();

	Logger::L().Log (MODULENAME, severity_level::debug2, "initThetaAsync : " + EnumConversion<ThetaMotorOperationSequence>::enumToString(curState));

	if (cancelThetaMove_)
	{
		ex.nextState = ThetaMotorOperationSequence::th_mo_cancel;
		curState = ThetaMotorOperationSequence::th_mo_cancel;
	}

	switch (curState)
	{
		// This is the entry point for initialization enum execution
		case ThetaMotorOperationSequence::th_mo_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			// check and initialize if not already initialized
			if (probePosInited)
			{
				ex.nextState = ThetaMotorOperationSequence::th_mo_can_do_work;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			Logger::L().Log (MODULENAME, severity_level::debug2, "initThetaAsync : Initializing Probe");

			// callback to be executed when probe initialization is done
			auto cb = [ ex ](bool status) mutable -> void
			{
				// If status is true that means probe is initialized so we set the next state as "th_mo_entry_point"
				// This is being done because there exist more work in "th_mo_entry_point" state.
				// If status is false, that means we cannot continue further without initializing probe
				// so set the next state as error state "th_mo_error"
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_can_do_work;
				}
				else
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_error;
				}

				// Once the next state is set then call "executeNextState" with "HawkeyeError::eSuccess" to run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			};

			// Call Initialize probe and exit from current execution
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initProbeAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				cb);

			// Here we need to exit because we cannot go further until probe is initialized
			// Once probe is initialized, callback "cb" will resume this execution.
			return;
		}
		case ThetaMotorOperationSequence::th_mo_can_do_work:
		{
			// Check and move probe up if not already in up position.
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If probe is up then move to next state else set next state as error "th_mo_error"
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_do_work;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initThetaAsync : failure moving probe up");
					ex.nextState = ThetaMotorOperationSequence::th_mo_error;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ThetaMotorOperationSequence::th_mo_do_work:
		{
			// home the theta motor
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doHomeThetaAsync, this, std::placeholders::_1, true /*fail on tube detect*/, 0), // set the retries count to "0" here since this method will already retry if homing failed
				ThetaMotorOperationSequence::th_mo_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If theta motor is homed, then move to next state, else move to error state "th_mo_error"
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_do_work_wait;
				}
				else
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_error;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Here we need to exit because we cannot go further until theta motor is homed
			// Once theta motor is homed, callback will resume this execution.
			return;
		}
		case ThetaMotorOperationSequence::th_mo_do_work_wait:
		{
			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
			asyncTaskList.clear();

			// add mark theta position as zero task to list
			asyncTaskList.push_back(
				std::bind(&StageController::markThetaPositionAsZeroAsync, this, std::placeholders::_1, (getThetaProperties().thetaFullTimeout_msec)));

			// add check theta position at "0" to list
			asyncTaskList.push_back(
				std::bind(&StageController::thetaAtPositionAsync, this, std::placeholders::_1, 0, 0, false));

			// execute the queued tasks
			asyncCommandHelper_->queueASynchronousTask([ = ](bool status) mutable -> void
			{
				int32_t currentTPos = GetThetaPosition();
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_complete;
					thetaPosInited = true;
				}
				else
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_error;
				}
				Logger::L().Log (MODULENAME, severity_level::debug2, "initThetaAsync : Mark Position As Zero tPos : " + std::to_string(currentTPos) + (status ? " Success" : " Failure"));
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from here and "queueASynchronousTask" callback will resume the execution
			return;
		}
		case ThetaMotorOperationSequence::th_mo_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::notification, "initThetaAsync : Initialization of theta motor cancelled!");

			// create a lambda to catch the status of the theta motor stop command
			auto cancelStop = [ this ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initThetaAsync : theta motor init cancel: stop command failed!");

					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::theta, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			thetaMotor->Stop(cancelStop, true, getThetaProperties().thetaFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation
																							// set the complete to stop enum execution
			ex.setComplete = true;
			break;
		}
		case ThetaMotorOperationSequence::th_mo_complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initThetaAsync : Initialization of theta motor completed!");
			// set the complete to stop enum execution
			thetaPosInited = true;
			ex.setComplete = true;
			cancelThetaMove_ = false;
			break;
		}
		case ThetaMotorOperationSequence::th_mo_timeout:
		case ThetaMotorOperationSequence::th_mo_error:
		{
			thetaMotorStatus_.reset();
			// check if we can retry from beginning
			if (ex.canRetry(ThetaMotorOperationSequence::th_mo_entry_point)
				&& carouselTubeFoundDuringInit_ == false) // If initialization failed due to tube present while initialization then no need to retry
			{
				ex.setComplete = false;
				he = HawkeyeError::eSuccess;
				ex.nextState = ThetaMotorOperationSequence::th_mo_entry_point;
				errStr.append("initThetaAsync : Initialization of theta motor failed - Retry initialization!");

				// clear the previous errors occurred for theta motors since we are retrying
				properties_[getCurrentType()].thetaErrorCodes.clear();

				break;
			}
			errStr.append("initThetaAsync : Initialization of theta motor failed!");

			// Theta motion initialization has failed even after retries, report error to user
			thetaMotor->triggerErrorReportCb (BuildErrorInstance(
				instrument_error::motion_motor_initerror, 
				instrument_error::motion_motor_instances::theta, 
				instrument_error::severity_level::error));
			reportErrorsAsync();
		}
		default:
		{
			// set Hawkeye error to "eHardwareFault" to indicate that theta motor initialization is failed
			he = HawkeyeError::eHardwareFault;
			// set the complete to stop enum execution
			ex.setComplete = true;
			cancelThetaMove_ = false;
			break;
		}
	}

	if (!errStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, errStr);
	}

	// run the next state asynchronously with input "HawkeyeError"
	ex.executeNextState(he);
}

/// <summary>
/// Home the theta motor asynchronously
/// </summary>
/// <param name="ex">The entry point for homing enum states</param>
/// <param name="failonTubeDetect">Whether or not a tube detection event during this move is fatal</param>
/// <param name="numRetries">The maximum number of retries allowed in case of original call failure</param>
void StageController::doHomeThetaAsync(AsyncCommandHelper::EnumStateParams ex, bool failOnTubeDetect, uint8_t numRetries)
{
	auto curState = (ThetaMotorOperationSequence) ex.currentState;
	std::string errStr = std::string();
	HawkeyeError he = HawkeyeError::eSuccess;

	Logger::L().Log (MODULENAME, severity_level::debug2, "doHomeThetaAsync : " + EnumConversion<ThetaMotorOperationSequence>::enumToString(curState));

	if (cancelThetaMove_)
	{
		ex.nextState = ThetaMotorOperationSequence::th_mo_cancel;
		curState = ThetaMotorOperationSequence::th_mo_cancel;
	}

	switch (curState)
	{
		// This is the entry point for homing enum execution
		case ThetaMotorOperationSequence::th_mo_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;
			carouselTubeFoundDuringInit_ = false;

			// Check and move probe up if not already in up position.
			// Exit from current execution here. callback will resume this execution.
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If probe is up then move to next state else set next state as error "th_mo_error"
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_do_work;
				}
				else
				{
					std::string tmp_errStr = boost::str(boost::format("doHomeThetaAsync: error moving probe"));
					Logger::L().Log (MODULENAME, severity_level::error, tmp_errStr);
					ex.nextState = ThetaMotorOperationSequence::th_mo_error;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ThetaMotorOperationSequence::th_mo_do_work:
		{
			// calculate the theta motor timeout(seconds)
			uint32_t timeout_sec = getThetaProperties().thetaFullTimeout_msec / 1000;

			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, 0, ePositionDescription::Target);

			// Trigger this lambda when theta motor homing operation is complete
			auto onHomingComplete = [ = ](bool status, bool tubeFound) mutable
			{
				// If status is success that means homing is complete; else operation got timed out
				if (status)
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_complete;
				}
				else
				{
					ex.nextState = ThetaMotorOperationSequence::th_mo_timeout;
					carouselTubeFoundDuringInit_ = tubeFound;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			};

			if (checkCarouselPresent())
			{
				// For Homing Operation, theta motor may take one full revolution
				// If carousel is present then we need to avoid throwing tubes in bins when doing homing.
				// So check for tube present and stop homing the theta motor

				// Wrap theta motor homing operation inside lambda
				auto homeThetaLambda = [ = ](std::function<void(bool)> localCallback)
				{
					// Call the theta motor home command and wait for callback to return status
					thetaMotor->Home(localCallback, timeout_sec);
				};

				// Begin the theta move to home
				carouselMoveTheta(failOnTubeDetect, ThetaMotionTubeDetectStates::eEntryPoint, homeThetaLambda, onHomingComplete);
			}
			else
			{
				thetaMotor->Home([ onHomingComplete ](bool status) mutable
				{
					onHomingComplete(status, false);
				}, timeout_sec);
			}

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ThetaMotorOperationSequence::th_mo_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::notification, "doHomeThetaAsync : Homing of theta motor cancelled!");

			// create a lambda to catch the status of the theta motor stop command
			auto cancelStop = [ this ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "doHomeThetaAsync : theta motor home cancel: stop command failed!");

					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::theta, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			thetaMotor->Stop(cancelStop, true, getThetaProperties().thetaFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation
																							// set the complete to stop enum execution
			ex.setComplete = true;
			break;
		}
		case ThetaMotorOperationSequence::th_mo_complete:
		{
			// Check if motor home status is success or set the next state to error state "th_mo_error"
			bool isHome = this->ChkThetaHome();
			if (isHome)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("doHomeThetaAsync : succeeded")));
				thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, GetThetaPosition(), ePositionDescription::Home);
				ex.setComplete = true;
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("doHomeThetaAsync : Home status is not set")));
				ex.nextState = ThetaMotorOperationSequence::th_mo_error;
			}

			ex.executeNextState(HawkeyeError::eSuccess);
			return;
		}
		case ThetaMotorOperationSequence::th_mo_timeout:
		{
			errStr.append("doHomeThetaAsync : unable to home theta motor : motor movement timed out!");
			// fallthrough
		}
		case ThetaMotorOperationSequence::th_mo_error:
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, GetThetaPosition(), ePositionDescription::Current);
			// check if we can retry from beginning
			if (ex.canRetry(ThetaMotorOperationSequence::th_mo_entry_point)
				&& carouselTubeFoundDuringInit_ == false) // If initialization failed due to tube present while homing then no need to retry
			{
				ex.setComplete = false;
				cancelThetaMove_ = false;
				he = HawkeyeError::eSuccess;
				ex.nextState = ThetaMotorOperationSequence::th_mo_entry_point;
				errStr.append("doHomeThetaAsync : Retry homing theta motor!");

				// clear the previous errors occurred for theta motors since we are retrying
				properties_[getCurrentType()].thetaErrorCodes.clear();

				break;
			}

			if (thetaPosInited)
			{
				thetaMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_homefail, 
					instrument_error::motion_motor_instances::theta, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
			errStr.append("doHomeThetaAsync : unable to home theta motor!");
		}
		default:
		{
			he = HawkeyeError::eHardwareFault;
			ex.setComplete = true;
			cancelThetaMove_ = false;
			break;
		}
	}

	if (!errStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, errStr);
	}

	// run the next state asynchronously
	ex.executeNextState(he);
}

/// <summary>
/// Validates if theta motor is at target position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether validate is success/failure</param>
/// <param name="targetPos">Theta motor target position</param>
/// <param name="tolerance">Theta motor position tolerance</param>
/// <param name="deadband">The dead band parameter <c>[true]</c>; if dead band needs to be applied</param>
void StageController::thetaAtPositionAsync(std::function<void(bool)> cb, int32_t targetPos, int32_t tolerance, bool deadband)
{
	assert(cb);

	// Get the current theta position and validate position
	int32_t position = GetThetaPosition();
	cb(this->ThetaAtPosition(position, targetPos, tolerance, deadband));
}

/// <summary>
/// Sets the theta motor position to target position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether target position is achieved</param>
/// <param name="tgtPos">Theta motor target position</param>
/// <param name="waitMilliSec">Theta motor time out in milliseconds</param>
void StageController::gotoThetaPositionAsync(std::function<void(bool)> cb, int32_t tgtPos, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoThetaPositionAsync : <Enter> tgtPos : " + std::to_string(tgtPos));

	if (!isProbeUp())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "gotoThetaPositionAsync : <Probe is not up>");
		cb(false);
		return;
	}

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// Ask theta motor to go to target position asynchronously
	thetaMotor->SetPosition([=](bool status) -> void
	{
		int32_t tPos = this->GetThetaPosition();

		std::string logStr = boost::str(boost::format("gotoThetaPositionAsync : <SetPosition>  with tgtPos <%d> , currentPos <%d>") % tgtPos % tPos);
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, tPos, ePositionDescription::AtPosition);
		}
		else
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, tPos, ePositionDescription::Current);
			thetaMotor->triggerErrorReportCb (BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::theta, 
				instrument_error::severity_level::error));
			logStr.append(" <Operation Failed>");
			Logger::L().Log (MODULENAME, severity_level::error, logStr);
		}

		cb(status);

	}, tgtPos, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoThetaPositionAsync : <Exit>");
}

/// <summary>
/// Moves the theta motor from its current position to certain steps asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether target position is achieved</param>
/// <param name="moveStep">Theta motor move step</param>
/// <param name="waitMilliSec">Theta motor time out in milliseconds</param>
void StageController::moveThetaPositionRelativeAsync(std::function<void(bool)> cb, int32_t moveStep, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveThetaPositionRelativeAsync : <Enter> moveStep : " + std::to_string(moveStep));

	// Check asynchronously if probe is up or not
	if (!isProbeUp())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "moveThetaPositionRelativeAsync : <Probe is not up>");
		cb(false);
		return;
	}

	// Calculate the target theta position using current theta position and target step
	// This will help in deciding whether theta motor has successfully moved to target position or not
	int32_t tgtPos = this->GetThetaPosition() + moveStep;

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// Move the theta motor from current position
	thetaMotor->MovePosRelative([ = ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("moveThetaPositionRelativeAsync : %s with tgtPos <%d> , currentPos <%d>") % status % tgtPos % GetThetaPosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, this->GetThetaPosition(), ePositionDescription::AtPosition);
		}
		else
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, this->GetThetaPosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
			thetaMotor->triggerErrorReportCb (BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::theta, 
				instrument_error::severity_level::error));
		}

		cb(status);

	}, moveStep, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveThetaPositionRelativeAsync : <Exit>");
}

/// <summary>
/// Mark the current theta position as zeroth position or home position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether position is marked</param>
/// <param name="waitMilliSec">Theta motor time out in milliseconds</param>
void StageController::markThetaPositionAsZeroAsync(std::function<void(bool)> cb, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "markThetaPositionAsZeroAsync : <Enter> currentPos : " + std::to_string(GetThetaPosition()));

	thetaMotor->MarkPosAsZero([ this, cb ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("markThetaPositionAsZeroAsync : %s currentPos <%d>") % status % this->GetThetaPosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, 0, ePositionDescription::AtPosition);
		}
		else
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, this->GetThetaPosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
		}

		cb(status);

	}, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "markThetaPositionAsZeroAsync : <Exit>");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////Radius Motor Methods////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Call this method to reinitialize radius motor
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::ReinitRadius()
{
	if (ControllerOk())
	{
		radiusMotor->ApplyDefaultParams();
		Logger::L().Log (MODULENAME, severity_level::debug2, "ReinitRadius: apply default params");
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetRadiusPosition(), ePositionDescription::Current);
		return true;
	}
	return false;
}

/// <summary>
/// Call this method to update radius properties <see cref="StageController::RadiusProperties"/>
/// from motor info file
/// </summary>
/// <param name="config">The input ptree pointing to radius properties in motor info file</param>
/// <param name="carrierType">The input stage controller type</param>
/// <param name="defaultValues">The default stage controller radius properties</param>
/// <returns>The radius properties for input stage controller type</returns>
StageController::RadiusProperties StageController::ConfigRadiusVariables (boost::property_tree::ptree& config, eCarrierType carrierType,
                                                                          boost::optional<StageController::RadiusProperties> defaultValues)
{
	RadiusProperties rp;
	if (defaultValues)
	{
		rp = defaultValues.get();
	}
	else
	{
		rp = getDefaultRadiusProperties(carrierType);
	}

	const DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();

	if (carrierType == eCarrierType::eCarousel)
	{
		rp.radiusOffset = instCfg.CarouselRadiusOffset;

		rp.radiusMaxTravel = config.get<int32_t>("CarouselRadiusMaxTravel", rp.radiusMaxTravel);

		rp.radiusStartTimeout_msec /= TIMEOUT_FACTOR;
		rp.radiusStartTimeout_msec = config.get<int32_t>("RadiusStartTimeout", rp.radiusStartTimeout_msec) * TIMEOUT_FACTOR;

		rp.radiusFullTimeout_msec /= TIMEOUT_FACTOR;
		rp.radiusFullTimeout_msec = config.get<int32_t>("RadiusFullTimeout", rp.radiusFullTimeout_msec) * TIMEOUT_FACTOR;
	}
	else if (carrierType == eCarrierType::ePlate_96)
	{		
		rp.radiusCenterPos = instCfg.PlateRadiusCenterPos;

		rp.radiusOffset = config.get<int32_t>("PlateRadiusOffset", rp.radiusOffset);
		rp.radiusPositionTolerance = config.get<int32_t>("PlateRadiusPositionTolerance", rp.radiusPositionTolerance);
		// these do not need to be read from the configuration file...
		rp.radiusBacklash = config.get<int32_t>("PlateRadiusBacklash", rp.radiusBacklash);
		rp.radiusMaxTravel = config.get<int32_t>("PlateRadiusMaxTravel", rp.radiusMaxTravel);

		rp.radiusStartTimeout_msec /= TIMEOUT_FACTOR;
		rp.radiusStartTimeout_msec = config.get<int32_t>("RadiusStartTimeout", rp.radiusStartTimeout_msec) * TIMEOUT_FACTOR;

		rp.radiusFullTimeout_msec /= TIMEOUT_FACTOR;
		rp.radiusFullTimeout_msec = config.get<int32_t>("RadiusFullTimeout", rp.radiusFullTimeout_msec) * TIMEOUT_FACTOR;
	}

	return rp;
}

/// <summary>
/// Call this method to update motor info file from
/// radius properties <see cref="StageController::RadiusProperties"/>
/// </summary>
/// <param name="config">The input ptree pointing to radius properties in motor info file</param>
/// <param name="rp">The input radius properties to be updated</param>
/// <param name="carrierType">The input stage controller type</param>
/// <returns><c>[0]</c> if success; otherwise failed</returns>
int32_t StageController::UpdateRadiusControllerConfig (boost::property_tree::ptree& config, const RadiusProperties& rp, eCarrierType carrierType)
{
	DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();
	if (carrierType ==eCarrierType::eCarousel)
	{
		properties_[eCarrierType::eCarousel].radiusProperties.radiusOffset = rp.radiusOffset;

		instCfg.CarouselRadiusOffset = rp.radiusOffset;
		InstrumentConfig::Instance().Set();
	}
	else if (carrierType == eCarrierType::ePlate_96)
	{
		properties_[eCarrierType::eCarousel].radiusProperties.radiusCenterPos = rp.radiusCenterPos;

		instCfg.PlateRadiusCenterPos = rp.radiusCenterPos;
		InstrumentConfig::Instance().Set();
	}

	return 0;
}

/// <summary>
/// Call this method to get the radius backlash value.
/// </summary>
int32_t StageController::getRadiusBacklash()
{
	return properties_[getCurrentType()].radiusProperties.radiusBacklash;
}

/// <summary>
/// Call this method to get the cached radius motor position
/// </summary>
/// <returns>The cached radius motor position value</returns>
int32_t StageController::GetRadiusPosition(void) const
{
	return radiusMotor->GetPosition();
}

/// <summary>
/// performs a radius motor position read asynchronously
/// </summary>
/// <param name="posCb">Callback to indicate the read operation success/failure</param>
void StageController::readRadiusPositionAsync( std::function<void( bool )> posCb )
{
    assert( posCb );

    Logger::L().Log ( MODULENAME, severity_level::debug2, "readRadiusPositionAsync: <enter>" );

    // perform a controller read for the current theta position
    auto positionRdComplete = [this, posCb]( bool rdStatus ) -> void
    {
        if ( !rdStatus )
        {
            Logger::L().Log ( MODULENAME, severity_level::error, "readRadiusPositionAsync: ReadPosition: unable to get motor position!" );
        }

        posCb( true );
    };

    pCBOService_->enqueueInternal( [=]( void ) -> void
    {
        boost::system::error_code error;
        radiusMotor->ReadPosition( positionRdComplete, error );
    } );

    Logger::L().Log ( MODULENAME, severity_level::debug2, "readRadiusPositionAsync: <exit>" );
}

/// <summary>
/// Call this method to check if radius motor is at given position
/// </summary>
/// <param name="pos">The current radius position</param>
/// <param name="targetPos">The target radius position</param>
/// <param name="tolerance">The radius position tolerance</param>
/// <param name="deadband">The dead band parameter <c>[true]</c>; if dead band needs to be applied</param>
/// <returns><c>[true]</c> if radius motor is with-in given limit; otherwise <c>[false]</c></returns>
bool StageController::RadiusAtPosition(int32_t pos, int32_t targetPos, int32_t tolerance, bool deadband) const
{
	bool posOk = radiusMotor->PosAtTgt(pos, targetPos, tolerance, deadband);

	if (!posOk && Logger::L().IsOfInterest(severity_level::debug2))
	{
		std::string logStr = boost::str(
			boost::format("RadiusAtPosition: posOk: %s with <radius position : %d> <targetPos : %d> <tolerance : %d> <deadband : %s>")
			% ((posOk == true) ? "true" : "false")
			% pos
			% targetPos
			% tolerance
			% (deadband ? "true" : "false"));
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	return posOk;
}

/// <summary>
/// Initialize the radius motor asynchronously
/// </summary>
/// <param name="ex">The entry point for initialization enum states</param>
/// <param name="numRetries">The maximum number of retries allowed incase of original call failure</param>
void StageController::initRadiusAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries)
{
	// get the current state
	auto curState = (RadiusMotorOperationSequence) ex.currentState;
	HawkeyeError he = HawkeyeError::eSuccess;
	std::string errStr = std::string();

	Logger::L().Log (MODULENAME, severity_level::debug2, "initRadiusAsync : " + EnumConversion<RadiusMotorOperationSequence>::enumToString(curState));

	if (cancelRadiusMove_)
	{
		ex.nextState = RadiusMotorOperationSequence::ra_mo_cancel;
		curState = RadiusMotorOperationSequence::ra_mo_cancel;
		cancelRadiusMove_ = false;
	}

	switch (curState)
	{
		// This is the entry point for initialization enum execution
		case RadiusMotorOperationSequence::ra_mo_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			// check and initialize if not already initialized
			if (probePosInited)
			{
				ex.nextState = RadiusMotorOperationSequence::ra_mo_can_do_work;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			// callback to be executed when probe initialization is done
			auto cb = [ ex ](bool status) mutable -> void
			{
				// If status is true that means probe is initialized so we set the next state as "ra_mo_entry_point"
				// This is being done because there exist more work in "ra_mo_error" state.
				// If status is false, that means we cannot continue further without initializing probe
				// so set the next state as error state "th_mo_error"
				ex.nextState = status
					? RadiusMotorOperationSequence::ra_mo_can_do_work
					: RadiusMotorOperationSequence::ra_mo_error;

				// Once the next state is set then call "executeNextState" with "HawkeyeError::eSuccess" to run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			};

			// Call Initialize probe and exit from current execution
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initProbeAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				cb);

			// Here we need to exit because we cannot go further until probe is initialized
			// Once probe is initialized, callback "cb" will resume this execution.
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_can_do_work:
		{
			// Check and move probe up if not already in up position.
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If probe is up then move to next state else set next state as error "ra_mo_error"
				if (status)
				{
					ex.nextState = RadiusMotorOperationSequence::ra_mo_do_work;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initRadiusAsync : failure moving probe up");
					ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
				}
				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_do_work:
		{
			// home the radius motor
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doHomeRadiusAsync, this, std::placeholders::_1, 0), // set the retries count to "0" here since this method will already retry if homing failed
				RadiusMotorOperationSequence::ra_mo_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If radius motor is homed, then move to next state, else move to error state "ra_mo_error"
				if (status)
				{
					ex.nextState = RadiusMotorOperationSequence::ra_mo_do_work_wait;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initRadiusAsync : radius home reports not at position");
					ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Here we need to exit because we cannot go further until radius motor is homed
			// Once radius motor is homed, callback will resume this execution.
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_do_work_wait:
		{
			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
			asyncTaskList.clear();

			// add mark radius position as zero task to list
			asyncTaskList.push_back(
				std::bind(&StageController::markRadiusPosAsZeroAsync, this, std::placeholders::_1, (getRadiusProperties().radiusFullTimeout_msec)));

			// add check radius position at "0" to list
			asyncTaskList.push_back(
				std::bind(&StageController::radiusAtPositionAsync, this, std::placeholders::_1, 0, 0, false));

			// execute the queued tasks
			asyncCommandHelper_->queueASynchronousTask([ this, ex ](bool status) mutable -> void
			{
				if (status)
				{
					Logger::L().Log (MODULENAME, severity_level::debug2, "initRadiusAsync : Mark Position As Zero - Success");
					ex.nextState = RadiusMotorOperationSequence::ra_mo_complete;
					radiusPosInited = true;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initRadiusAsync : Mark Position As Zero - Failed");
					ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from here and "queueASynchronousTask" callback will resume the execution
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "initRadiusAsync : Initialization of radius motor cancelled!");

			// create a lambda to catch the status of the theta motor stop command
			auto cancelStop = [ this ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initRadiusAsync : radius motor init cancel: stop command failed!");

					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::radius, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			radiusMotor->Stop(cancelStop, true, getRadiusProperties().radiusFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation

			ex.setComplete = true;		// set the complete to stop enum execution
			break;
		}
		case RadiusMotorOperationSequence::ra_mo_complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initRadiusAsync : Initialization of radius motor completed!");
			// set the complete to stop enum execution
			radiusPosInited = true;
			ex.setComplete = true;
			break;
		}
		case RadiusMotorOperationSequence::ra_mo_timeout:
		case RadiusMotorOperationSequence::ra_mo_error:
		{
			radiusMotorStatus_.reset();
			// check if we can retry from beginning
			if (ex.canRetry(RadiusMotorOperationSequence::ra_mo_entry_point))
			{
				ex.setComplete = false;
				he = HawkeyeError::eSuccess;
				ex.nextState = RadiusMotorOperationSequence::ra_mo_entry_point;
				errStr.append("initRadiusAsync : Initialization of radius motor failed - Retry initialization!");

				// clear the previous errors occurred for radius motors since we are retrying
				properties_[getCurrentType()].radiusErrorCodes.clear();

				break;
			}

			errStr.append("initRadiusAsync : Initialization of radius motor failed!");

			// Radius motion initialization has failed even after retries, report error to user
			radiusMotor->triggerErrorReportCb(BuildErrorInstance(
				instrument_error::motion_motor_initerror, 
				instrument_error::motion_motor_instances::radius, 
				instrument_error::severity_level::error));
			reportErrorsAsync();
		}
		default:
		{
			// set Hawkeye error to "eHardwareFault" to indicate that radius motor initialization is failed
			he = HawkeyeError::eHardwareFault;
			// set the complete to stop enum execution
			ex.setComplete = true;
			break;
		}
	}

	if (!errStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, errStr);
	}

	// run the next state asynchronously with input "HawkeyeError"
	ex.executeNextState(he);
}

/// <summary>
/// Home the radius motor asynchronously
/// </summary>
/// <param name="ex">The entry point for homing enum states></param>
/// <param name="numRetries">The maximum number of retries allowed incase of original call failure</param>
void StageController::doHomeRadiusAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries)
{
	auto curState = (RadiusMotorOperationSequence) ex.currentState;
	HawkeyeError he = HawkeyeError::eSuccess;
	std::string errStr = std::string();

	Logger::L().Log (MODULENAME, severity_level::debug2, "doHomeRadiusAsync : " + EnumConversion<RadiusMotorOperationSequence>::enumToString(curState));

	if (cancelRadiusMove_)
	{
		ex.nextState = RadiusMotorOperationSequence::ra_mo_cancel;
		curState = RadiusMotorOperationSequence::ra_mo_cancel;
		cancelRadiusMove_ = false;
	}

	switch (curState)
	{
		// This is the entry point for homing enum execution
		case RadiusMotorOperationSequence::ra_mo_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			// Check and move probe up if not already in up position.
			// Exit from current execution here. callback will resume this execution.
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, numRetries),
				ProbeOperationSequence::po_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If probe is up then move to next state else set next state as error "ra_mo_error"
				if (status)
				{
					ex.nextState = RadiusMotorOperationSequence::ra_mo_do_work;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "doHomeRadiusAsync : error moving probe");
					ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
				}
				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_do_work:
		{
			// calculate the radius motor timeout(seconds)
			uint32_t timeout_sec = getRadiusProperties().radiusFullTimeout_msec / 1000;

			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, 0, ePositionDescription::Target);

			// Call the radius motor home command and wait for callback to return status
			radiusMotor->Home([ = ](bool status) mutable -> void
			{
				// If status is success that means homing is complete; else operation got timed out
				if (status)
				{
					ex.nextState = RadiusMotorOperationSequence::ra_mo_complete;
				}
				else
				{
					ex.nextState = RadiusMotorOperationSequence::ra_mo_timeout;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			}, timeout_sec);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case RadiusMotorOperationSequence::ra_mo_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "doHomeRadiusAsync : Homing of radius motor cancelled!");

			// create a lambda to catch the status of the theta motor stop command
			auto cancelStop = [ this ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "doHomeRadiusAsync : radius motor home cancel: stop command failed!");

					thetaMotor->triggerErrorReportCb(BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::radius, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			radiusMotor->Stop(cancelStop, true, getRadiusProperties().radiusFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation

			ex.setComplete = true;		// set the complete to stop enum execution
			break;
		}
		case RadiusMotorOperationSequence::ra_mo_complete:
		{
			// Check if motor home status is success or set the next state to error state "ra_mo_error"
			bool isHome = (radiusMotor->GetHomeStatus() && radiusMotor->IsHome());
			if (isHome)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("doHomeRadiusAsync : succeeded")));
				radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, GetRadiusPosition(), ePositionDescription::Home);
				ex.setComplete = true;
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("doHomeRadiusAsync: Home status is not set")));
				ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
			}

			he = HawkeyeError::eSuccess;
			break;

		}
		case RadiusMotorOperationSequence::ra_mo_timeout:
		{
			if (radiusMotor->IsBusy())
			{
				errStr.append("doHomeRadiusAsync : Motor timed out reporting busy.\n\t");
			}
			else
			{
				errStr.append("doHomeRadiusAsync : Motor does not report correct position.\n\t");
			}
			ex.nextState = RadiusMotorOperationSequence::ra_mo_error;
			break;
		}
		case RadiusMotorOperationSequence::ra_mo_error:
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, GetRadiusPosition(), ePositionDescription::Current);
			// check if we can retry from beginning
			if (ex.canRetry(RadiusMotorOperationSequence::ra_mo_entry_point))
			{
				ex.setComplete = false;
				he = HawkeyeError::eSuccess;
				ex.nextState = RadiusMotorOperationSequence::ra_mo_entry_point;
				errStr.append("doHomeRadiusAsync : Retry homing radius motor!");

				// clear the previous errors occurred for radius motors since we are retrying
				properties_[getCurrentType()].radiusErrorCodes.clear();

				break;
			}

			if (radiusPosInited)
			{
				radiusMotor->triggerErrorReportCb(BuildErrorInstance(
					instrument_error::motion_motor_homefail, 
					instrument_error::motion_motor_instances::radius, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
			errStr.append("doHomeRadiusAsync : Failed");
		}
		default:
		{
			he = HawkeyeError::eHardwareFault;
			ex.setComplete = true;
			break;
		}
	}

	if (!errStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, errStr);
	}

	// run the next state asynchronously
	ex.executeNextState(he);
}

/// <summary>
/// Validates if radius motor is at target position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether validate is success/failure</param>
/// <param name="targetPos">Radius motor target position</param>
/// <param name="tolerance">Radius motor position tolerance</param>
/// <param name="deadband">The dead band parameter <c>[true]</c>; if dead band needs to be applied</param>
void StageController::radiusAtPositionAsync(std::function<void(bool)> cb, int32_t targetPos, int32_t tolerance, bool deadband)
{
	assert(cb);

	cb(radiusMotor->PosAtTgt(radiusMotor->GetPosition(), targetPos, tolerance, deadband));
}

/// <summary>
/// Sets the radius motor position to target position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether target position is achieved</param>
/// <param name="tgtPos">Radius motor target position</param>
/// <param name="waitMilliSec">Radius motor time out in milliseconds</param>
void StageController::gotoRadiusPositionAsync(std::function<void(bool)> cb, int32_t tgtPos, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoRadiusPositionAsync : <Enter> tgtPos : " + std::to_string(tgtPos));

	if (!isProbeUp())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "gotoRadiusPositionAsync : <Probe is not up>");
		cb(false);
		return;
	}

	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// Ask radius motor to go to target position asynchronously
	radiusMotor->SetPosition([ = ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("gotoRadiusPositionAsync : %s with tgtPos <%d> , currentPos <%d>") % status % tgtPos % GetRadiusPosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, this->GetRadiusPosition(), ePositionDescription::AtPosition);
		}
		else
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, this->GetRadiusPosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
			radiusMotor->triggerErrorReportCb(BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::radius, 
				instrument_error::severity_level::error));
		}

		cb(status);

	}, tgtPos, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoRadiusPositionAsync : <Exit>");
}

/// <summary>
/// Moves the radius motor from its current position to certain steps asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether target position is achieved</param>
/// <param name="moveStep">Radius motor move step</param>
/// <param name="waitMilliSec">Radius motor time out in milliseconds</param>
void StageController::moveRadiusPositionRelativeAsync(std::function<void(bool)> cb, int32_t moveStep, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveRadiusPositionRelativeAsync : <Enter> moveStep : " + std::to_string(moveStep));

	if (!isProbeUp())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "moveRadiusPositionRelativeAsync : <Probe is not up>");
		cb(false);
		return;
	}

	// Calculate the target radius position using current radius position and target step
	// This will help in deciding whether theta motor has successfully moved to target position or not
	int32_t tgtPos = this->GetRadiusPosition() + moveStep;

	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, tgtPos, ePositionDescription::Target);

	// Move the radius motor from current position
	radiusMotor->MovePosRelative([ = ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("moveRadiusPositionRelativeAsync : %s with tgtPos <%d> , currentPos <%d>") % status % tgtPos % GetRadiusPosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, this->GetRadiusPosition(), ePositionDescription::AtPosition);
		}
		else
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, this->GetRadiusPosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
			radiusMotor->triggerErrorReportCb(BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::radius, 
				instrument_error::severity_level::error));
		}

		cb(status);

	}, moveStep, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveRadiusPositionRelativeAsync : <Exit>");
}

/// <summary>
/// Mark the current radius position as zeroth position or home position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether position is marked</param>
/// <param name="waitMilliSec">Radius motor time out in milliseconds</param>
void StageController::markRadiusPosAsZeroAsync(std::function<void(bool)> cb, int32_t waitMilliSec)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "markRadiusPosAsZeroAsync : <Enter> currentPos : " + std::to_string(GetRadiusPosition()));

	radiusMotor->MarkPosAsZero([ this, cb, waitMilliSec ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("markRadiusPosAsZeroAsync : %s currentPos <%d>") % status % GetRadiusPosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, 0, ePositionDescription::AtPosition);
		}
		else
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, GetRadiusPosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
		}

		cb(status);

	}, (waitMilliSec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "markRadiusPosAsZeroAsync : <Exit>");
}


///////////////////////////////////////////////////////////////////////////////
/////////////////////Probe Controller Methods//////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Call this method to reinitialize probe motor
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::ReinitProbe(void)
{
	if (ControllerOk())
	{
		probeMotor->ApplyDefaultParams();
		Logger::L().Log (MODULENAME, severity_level::debug2, "ReinitProbe : apply default params");
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, getProbePosition(), ePositionDescription::Current);
	}
}

/// <summary>
/// Call this method to update probe properties <see cref="StageController::ProbeProperties"/>
/// from motor info file
/// </summary>
/// <param name="config">The input ptree pointing to probe properties in motor info file</param>
/// <param name="carrierType">The input stage controller type</param>
/// <param name="defaultValues">The default stage controller probe properties</param>
/// <returns>The probe properties for input stage controller type</returns>
StageController::ProbeProperties StageController::ConfigProbeVariables (boost::property_tree::ptree& config, eCarrierType carrierType,
																	   boost::optional<StageController::ProbeProperties> defaultValues)
{
	ProbeProperties pt;
	if (defaultValues)
	{
		pt = defaultValues.get();
	}
	else
	{
		pt = getDefaultProbeProperties(carrierType);
	}

	if (carrierType == eCarrierType::eCarousel || carrierType == eCarrierType::ePlate_96)
	{
		pt.probePositionTolerance = config.get<int32_t>("ProbePositionTolerance", pt.probePositionTolerance);
		pt.probeHomePos = config.get<int32_t>("ProbeHomePos", pt.probeHomePos);
		pt.probeHomePosOffset = config.get<int32_t>("ProbeHomePosOffset", pt.probeHomePosOffset);
		pt.probeStopPos = config.get<int32_t>("ProbeDownPos", pt.probeStopPos);
		pt.probeMaxTravel = config.get<int32_t>("ProbeMaxTravel", pt.probeMaxTravel);

		pt.probeStartTimeout_msec /= TIMEOUT_FACTOR;
		pt.probeStartTimeout_msec = config.get<int32_t>("ProbeStartTimeout", pt.probeStartTimeout_msec) * TIMEOUT_FACTOR;

		pt.probeBusyTimeout_msec /= TIMEOUT_FACTOR;
		pt.probeBusyTimeout_msec = config.get<int32_t>("ProbeBusyTimeout", pt.probeBusyTimeout_msec) * TIMEOUT_FACTOR;
		pt.probeRetries = config.get<int32_t>("ProbeInternalRetries", pt.probeRetries);
	}

	return pt;
}

/// <summary>
/// Call this method to update motor info file from
/// probe properties <see cref="StageController::ProbeProperties"/>
/// </summary>
/// <param name="config">The input ptree pointing to probe properties in motor info file</param>
/// <param name="tp">The input probe properties to be updated</param>
/// <param name="carrierType">The input stage controller type</param>
/// <returns><c>[0]</c> if success; otherwise failed</returns>
int32_t StageController::UpdateProbeControllerConfig (boost::property_tree::ptree& config, const ProbeProperties& pp, eCarrierType type)
{
	config.put<int32_t>("ProbeHomePosOffset", pp.probeHomePosOffset);

	return 0;
}

/// <summary>
/// Gets the end position of probe motor
/// </summary>
/// <returns>The end probe position</returns>
int32_t StageController::getProbePosition(void)
{
	int32_t currentPos = probeMotor->GetPosition();
	Logger::L().Log (MODULENAME, severity_level::debug3, boost::str(boost::format("getProbePosition : currentPos: %d") % currentPos));

	return currentPos;
}

/// <summary>
/// Call this method to check if probe is at home position
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
bool StageController::isProbeHome()
{
	// check if home status is not zero and IsHome is true
	bool homeOk = probeMotor->GetHomeStatus() && probeMotor->IsHome();
	Logger::L().Log (MODULENAME, severity_level::debug3, boost::str(boost::format("isProbeHome : %s") % ((homeOk == true) ? "true" : "false")));

	if (homeOk)
	{
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, getProbePosition(), ePositionDescription::Home);
	}

	return homeOk;
}

void StageController::probeUpAsync(std::function<void(bool)> cb)
{
	assert(cb);

	std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
	asyncTaskList.clear();

	if (!probePosInited)
	{
		// Add task to initialize probe
		asyncTaskList.push_back([ this ](std::function<void(bool)> initCB) -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "Initializing Probe motor!");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initProbeAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				initCB);
		});
	}

	// Add task to move probe up
	asyncTaskList.push_back([ this ](std::function<void(bool)> moveUpCB) -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "Moving Probe motor up!");
		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync(
			std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
			ProbeOperationSequence::po_entry_point,
			moveUpCB);
	});

	asyncCommandHelper_->queueASynchronousTask([ cb ](bool status) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Error moving probe up.");
		}
		cb(status);
	}, asyncTaskList, false);
}

/// <summary>
/// Call this method to check if probe is at up location, typically 0
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
bool StageController::isProbeUp()
{
	// Don't do anything is probe is not initialized yet
	if (!probePosInited)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isProbeUp : probe is not initialized");
		return false;
	}

	// If probe is at home, check offset before consider it up
	if (!isProbeHome())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isProbeUp : probe is not home");
		return false;
	}

	if (this->getProbeProperties().probeHomePosOffset != 0)   // check by position to allow for offsets that result in sensor non-occlusion...
	{
		return probeAtPosition(getProbeProperties().probeHomePos);
	}
	return true;
}

void StageController::probeDownAsync(std::function<void(bool)> cb, bool downOnInvalidStagePos)
{
	assert(cb);

	std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
	asyncTaskList.clear();

	if (!probePosInited)
	{
		// Add task to initialize probe
		asyncTaskList.push_back([ this ](std::function<void(bool)> initCB) -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "Initializing Probe motor!");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initProbeAsync, this, std::placeholders::_1, this->getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				initCB);
		});
	}

	// Add task to move probe down
	asyncTaskList.push_back([ this, downOnInvalidStagePos ](std::function<void(bool)> moveDownCB) -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "Moving Probe motor down!");
		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync(
			std::bind(&StageController::doProbeDownAsync, this, std::placeholders::_1, downOnInvalidStagePos, this->getCurrentProperties().retriesCount),
			ProbeOperationSequence::po_entry_point,
			moveDownCB);
	});

	asyncCommandHelper_->queueASynchronousTask([ cb ](bool status) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Error moving probe down.");
		}
		cb(status);
	}, asyncTaskList, false);
}

/// <summary>
/// Call this method to check if probe is at down location
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
bool StageController::isProbeDown()
{
	// Don't do anything is probe is not initialized yet
	if (!probePosInited)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isProbeDown : probe is not initialized");
		return false;
	}

	// Get the probe stop position
	int32_t stopPos = probeMotor->GetProbeStop();
	if (stopPos > 0)
	{
		// update the stop position if valid
		properties_[getCurrentType()].probeProperties.probeStopPos = stopPos;
	}

	return probeAtPosition((this->getProbeProperties().probeStopPos - probeMotor->GetProbeRaise()));
}

/// <summary>
/// Move the probe up certain steps from its current position
/// </summary>
/// <param name="moveStep">Probe motor move step</param>
/// <returns>The final probe motor position</returns>
void StageController::probeStepUpAsync(std::function<void(bool)> callback, int32_t moveStep)
{
	// pass in POSITIVE change values;
	// the method will convert to negative (upward) movement for the desired change step
	// returns the end position

	// Check if probe is up or not.
	if ((!isProbeUp()) && (moveStep != 0))
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "Distance_to_travel : " + std::to_string(moveStep));

		if (moveStep > 0)
		{
			moveStep = -moveStep;
		}

		auto current_pos = probeMotor->GetPosition();
		if ((-moveStep) > current_pos)
			moveStep = -current_pos;

		Logger::L().Log (MODULENAME, severity_level::debug2, "current_pos : " + std::to_string(current_pos));
		Logger::L().Log (MODULENAME, severity_level::debug2, "actual_travel_distance : " + std::to_string(moveStep));

		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync(
			std::bind(&StageController::stepProbeAsync, this, std::placeholders::_1, moveStep, false),
			ProbeOperationSequence::po_entry_point,
			callback);
	}
	else
	{
		pCBOService_->enqueueInternal(callback, true);
	}
}

/// <summary>
/// Move the probe down certain steps from its current position
/// </summary>
/// <param name="moveStep">Probe motor move step</param>
/// <param name="downOnInvalidStagePos">Input parameter for stepping probe down forcefully</param>
/// <returns>The final probe motor position</returns>
void StageController::probeStepDnAsync(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos)
{
	// pass in POSITIVE change values;
	// the method will check and convert to positive (downward) movement for the desired change step
	// returns the end position

	// Check is probe is down or not.
	if ((!isProbeDown()) && (moveStep != 0))
	{
		if (moveStep < 0)
		{
			moveStep = -moveStep;
		}

		auto max_travel_distance = probeMotor->GetMaxTravel();
		auto current_pos = probeMotor->GetPosition();
		auto can_travel_distance = max_travel_distance - current_pos;

		if (moveStep > can_travel_distance)
			moveStep = can_travel_distance;

		Logger::L().Log (MODULENAME, severity_level::debug2, "max_travel_distance : " + std::to_string(max_travel_distance));
		Logger::L().Log (MODULENAME, severity_level::debug2, "current_pos : " + std::to_string(current_pos));
		Logger::L().Log (MODULENAME, severity_level::debug2, "can_travel_distance : " + std::to_string(can_travel_distance));

		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync(
			std::bind(&StageController::stepProbeAsync, this, std::placeholders::_1, moveStep, downOnInvalidStagePos),
			ProbeOperationSequence::po_entry_point,
			callback);
	}
	else
	{
		pCBOService_->enqueueInternal(callback, true);
	}
}

/// <summary>
/// Get the probe parameters
/// </summary>
/// <param name="speed1Value">The probe speed-1 value</param>
/// <param name="speed1Current">The probe speed-1 current</param>
/// <param name="speed2Value">The probe speed-2 value</param>
/// <param name="speed2Current">The probe speed-2 current</param>
void StageController::GetProbeSpeedParams(uint32_t & speed1Value, uint32_t & speed1Current, uint32_t & speed2Value, uint32_t & speed2Current) const
{
	MotorRegisters regs = {};

	// get the probe motor parameters from registers
	probeMotor->GetMotorRegs(regs);

	// update the placeholders
	speed1Value = regs.ProbeRegs.ProbeSpeed1;
	speed1Current = regs.ProbeRegs.ProbeCurrent1;
	speed2Value = regs.ProbeRegs.ProbeSpeed2;
	speed2Current = regs.ProbeRegs.ProbeCurrent2;
}

/// <summary>
/// Sets the probe parameters
/// </summary>
/// <param name="speed1Value">The probe speed-1 value</param>
/// <param name="speed1Current">The probe speed-1 current</param>
/// <param name="speed2Value">The probe speed-2 value</param>
/// <param name="speed2Current">The probe speed-2 current</param>
void StageController::AdjustProbeSpeeds(uint32_t speed1Value, uint32_t speed1Current, uint32_t speed2Value, uint32_t speed2Current) const
{
	probeMotor->AdjustProbeSpeed1Current(speed1Current, true);
	probeMotor->AdjustProbeSpeed2Current(speed2Current, true);
	probeMotor->AdjustProbeSpeed1(speed1Value, true);
	probeMotor->AdjustProbeSpeed2(speed2Value, true);
}

/// <summary>
/// Gets the probe positions
/// </summary>
/// <param name="abovePosition">The probe above position</param>
/// <param name="probeRaise">The probe raise position</param>
void StageController::GetProbeStopPositions(int32_t& abovePosition, int32_t& probeRaise) const
{
	abovePosition = probeMotor->GetProbeAbovePosition();
	probeRaise = probeMotor->GetProbeRaise();
}

/// <summary>
/// Sets the probe positions
/// </summary>
/// <param name="abovePosition">The probe above position</param>
/// <param name="probeRaise">The probe raise position</param>
void StageController::AdjustProbeStop(int32_t abovePosition, int32_t probeRaise) const
{
	probeMotor->AdjustProbeAbovePosition(abovePosition, true);
	probeMotor->AdjustProbeRaise(probeRaise, true);
}

/// <summary>
/// Call this method to check if probe motor is at given position
/// </summary>
/// <param name="posTgt">The target probe position</param>
/// <returns><c>[true]</c> if probe motor is with-in given limit; otherwise <c>[false]</c></returns>
bool StageController::probeAtPosition(int32_t posTgt)
{
	int32_t currentProbePos = getProbePosition();
	return probeMotor->PosAtTgt(currentProbePos, posTgt, getProbeProperties().probePositionTolerance);
}

/// <summary>
/// Initialize the probe motor asynchronously
/// </summary>
/// <param name="ex">The entry point for initialization enum states</param>
/// <param name="numRetries">The maximum number of retries allowed incase of original call failure</param>
void StageController::initProbeAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries)
{
	// get the current state
	auto curState = (ProbeOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "initProbeAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point for initialization enum execution
		case ProbeOperationSequence::po_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			ex.nextState = ProbeOperationSequence::po_do_work;
			break;
		}
		case ProbeOperationSequence::po_do_work:
		{
			// home the probe motor asynchronously
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeHomeAsync, this, std::placeholders::_1),
				ProbeOperationSequence::po_entry_point,
				[ ex ](bool status) mutable -> void
			{
				// If probe motor is homed, then move to next state, else move to "po_error"
				ex.nextState = status ? ProbeOperationSequence::po_wait_for_work
					: ProbeOperationSequence::po_error;

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Here we need to exit because we cannot go further until probe motor is homed
			// Once probe motor is homed, callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_wait_for_work:
		{
			// get the probe home offset position
			int32_t offSet = getProbeProperties().probeHomePosOffset;
			if (offSet == 0)
			{
				ex.nextState = ProbeOperationSequence::po_complete;
				break;
			}

			gotoProbePositionAsync([ this, ex, offSet ](bool status) mutable -> void
			{
				status = status && probeAtPosition(offSet);
				if (status)
				{
					ex.nextState = ProbeOperationSequence::po_complete;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initProbeAsync : Initialize probe Sequence : " + std::string("Setting probe position to <probeHomePosOffset> timed out"));
					ex.nextState = ProbeOperationSequence::po_timeout;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, offSet);

			// Exit from here and "queueASynchronousTask" callback will resume the execution
			return;
		}
		case ProbeOperationSequence::po_complete:
		{
			markProbePosAsZeroAsync([ this, ex ](bool status) mutable
			{
				status = status && probeAtPosition(0);
				if (!status)
				{
					ex.nextState = ProbeOperationSequence::po_error;
				}
				probePosInited = true;
				ex.setComplete = status;
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from here and "queueASynchronousTask" callback will resume the execution
			return;
		}
		case ProbeOperationSequence::po_timeout:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "initProbeAsync : Initialize probe Sequence : probe sequence timed out!");
			ex.nextState = ProbeOperationSequence::po_error;
			break;
		}
		case ProbeOperationSequence::po_error:
		{
			// check if we can retry from beginning
			if (ex.canRetry(ProbeOperationSequence::po_entry_point))
			{
				ex.setComplete = false;
				ex.nextState = ProbeOperationSequence::po_entry_point;
				Logger::L().Log (MODULENAME, severity_level::error, "initProbeAsync : Initialization of probe motor failed - Retry initialization!");

				// clear the previous errors occurred for probe motors since we are retrying
				properties_[getCurrentType()].probeErrorCodes.clear();

				break;
			}

			Logger::L().Log (MODULENAME, severity_level::error, "initProbeAsync : Initialize probe Sequence : Error initializing probe!");

			// Sample Probe motion initialization has failed even after retries, report error to user
			probeMotor->triggerErrorReportCb (BuildErrorInstance(
				instrument_error::motion_motor_initerror, 
				instrument_error::motion_motor_instances::sample_probe, 
				instrument_error::severity_level::error));
			reportErrorsAsync();

			// set the complete to stop enum execution
			ex.setComplete = true;
			probeMotorStatus_.reset();

			// set Hawkeye error to "eHardwareFault" to indicate that probe motor initialization is failed
			ex.executeNextState(HawkeyeError::eHardwareFault);
			return;
		}
		default:
		{
			// set the complete to stop enum execution
			ex.setComplete = true;

			// set Hawkeye error to "eInvalidArgs" to indicate that probe motor initialization is failed
			ex.executeNextState(HawkeyeError::eInvalidArgs);
			return;
		}
	}

	// run the next state asynchronously with input "HawkeyeError"
	ex.executeNextState(HawkeyeError::eSuccess);
}

/// <summary>
/// Step the probe to certain units from its current position asynchronously
/// </summary>
/// <param name="ex">The entry point for probe step enum states</param>
/// <param name="moveStep">Probe motor move step</param>
/// <param name="downOnInvalidStagePos">Input parameter for stepping probe down forcefully</param>
void StageController::stepProbeAsync(AsyncCommandHelper::EnumStateParams ex, int32_t moveStep, bool downOnInvalidStagePos)
{
	// get current state
	auto curState = (ProbeOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "stepProbeAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point for enum execution
		case ProbeOperationSequence::po_entry_point:
		{
			// If the move step is negative that means probe is moving up.
			bool isProbeStepUp = moveStep < 0;

			// If probe is not initialized the set the next state to error state "po_error"
			if (probePosInited)
			{
				// If probe is moving up then there is no need to check whether 
				// stage is at valid position or not. Just move the probe up.
				if (isProbeStepUp)
				{
					ex.nextState = ProbeOperationSequence::po_do_work;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_can_do_work;
				}
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "stepProbeAsync : Step probe Sequence : Probe is not initialized yet!");
				ex.nextState = ProbeOperationSequence::po_error;
			}

			// run the next state asynchronously and exit from current execution
			ex.executeNextState(HawkeyeError::eSuccess);
			return;
		}
		case ProbeOperationSequence::po_can_do_work:
		{
			if (downOnInvalidStagePos)
			{
				// No need to check if stage is at valid position or not.
				doProbeOperationAsync(ex);
				return;
			}

			// Lambda to check if current sample position (theta motor/radius motor) is valid or not
			// Probe should not come down if there exist no valid carousel/plate position beneath it		
			auto samplePosLambda = [ ex ](SamplePositionDLL pos) mutable -> void
			{
				// If position is valid, then move to next step otherwise set the next state to error state "po_error"
				if (pos.isValid())
				{
					ex.nextState = ProbeOperationSequence::po_do_work;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_error;
					Logger::L().Log (MODULENAME, severity_level::error, "stepProbeAsync : Step probe Sequence : Invalid stage position!");
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			};

			// Get the current stage position
			SamplePositionDLL pos = {};
			getStagePosition(pos);
			samplePosLambda(pos);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_do_work:
		{
			// calculate the probe motor timeout(seconds)
			uint32_t timeout_sec = getProbeProperties().probeBusyTimeout_msec / 1000;

			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, (getProbePosition() + moveStep), ePositionDescription::Target);

			// Move the probe motor from its current position to input step
			probeMotor->MovePosRelative([ this, ex ](bool status) mutable -> void
			{
				// If status is success, then move to next step otherwise set the next state to timeout state "po_timeout"
				if (status)
				{
					ex.nextState = ProbeOperationSequence::po_complete;
					probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, getProbePosition(), ePositionDescription::AtPosition);
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_timeout;
					probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			}, moveStep, timeout_sec);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "stepProbeAsync : Step probe Sequence : Error stepping probe!");
			if (probePosInited)
			{
				probeMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_positionfail, 
					instrument_error::motion_motor_instances::sample_probe, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
		}
		default:
		{
			doProbeOperationAsync(ex);
		}
	}
}

/// <summary>
/// Do the probe up asynchronously
/// </summary>
/// <param name="ex">The entry point for probe up enum states</param>
void StageController::doProbeUpAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries)
{
	// get the current state
	auto curState = (ProbeOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeUpAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point for probe up enum execution
		case ProbeOperationSequence::po_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			// Check if probe is already up
			// if status is success that means probe is already up so move to final state "po_complete"
			// else move to next state.
			if (isProbeUp())
			{
				ex.nextState = ProbeOperationSequence::po_complete;
				ex.executeNextState(HawkeyeError::eSuccess);
			}
			else
			{
				this->doProbeOperationAsync(ex);
			}

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_do_work:
		{
			// calculate the probe motor timeout(seconds)
			uint32_t timeout_sec = getProbeProperties().probeBusyTimeout_msec / 1000;

			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, getProbeProperties().probeHomePos, ePositionDescription::Target);

			// do the probe up and wait for callback
			probeMotor->ProbeUp([ ex ](bool status) mutable -> void
			{
				// If status is success then move to next step otherwise set the next state to timeout state "po_timeout"
				if (status)
				{
					ex.nextState = ProbeOperationSequence::po_complete;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_timeout;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, timeout_sec);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_complete:
		{
			// Check if probe is up
			// If probe is up then complete the operation otherwise set the next state to error state "po_error"
			if (isProbeUp())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeUpAsync : Probe Up Sequence : Success");

				// set the complete to stop enum execution
				ex.setComplete = true;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, getProbePosition(), ePositionDescription::AtPosition);
			}
			else
			{
				ex.nextState = ProbeOperationSequence::po_error;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
			}

			// run the next state asynchronously
			ex.executeNextState(HawkeyeError::eSuccess);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_error:
		{
			// check if we can retry from beginning
			if (ex.canRetry(ProbeOperationSequence::po_entry_point))
			{
				ex.setComplete = false;
				ex.nextState = ProbeOperationSequence::po_entry_point;
				Logger::L().Log (MODULENAME, severity_level::error, "doProbeUpAsync : Probe Up Sequence - Retrying probe up!");

				// clear the previous errors occurred for probe motors since we are retrying
				properties_[getCurrentType()].probeErrorCodes.clear();
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "doProbeUpAsync : Probe Up Sequence : Error Moving Up!");
				if (probePosInited)
				{
					probeMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_positionfail, 
						instrument_error::motion_motor_instances::sample_probe, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
			}
		}
		default:
		{
			doProbeOperationAsync(ex);
		}
	}
}

/// <summary>
/// Do the probe down asynchronously
/// </summary>
/// <param name="ex">The entry point for probe down enum states</param>
/// <param name="downOnInvalidStagePos">Input parameter for setting probe down forcefully</param>
void StageController::doProbeDownAsync(AsyncCommandHelper::EnumStateParams ex, bool downOnInvalidStagePos, uint8_t numRetries)
{
	// get the current state
	auto curState = (ProbeOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeDownAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point for probe down enum execution
		case ProbeOperationSequence::po_entry_point:
		{
			// Set the maximum number of retries allowed plus (+1) the original call
			ex.maxRetries = numRetries + 1;

			// Check if probe is already down
			// if status is success that means probe is already down so move to final state "po_complete"
			// else move to next state.
			if (isProbeDown())
			{
				ex.nextState = ProbeOperationSequence::po_complete;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			setProbeSearchMode(downOnInvalidStagePos, [ this, ex, downOnInvalidStagePos ](bool status) mutable
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "doProbeDownAsync : Failed to set parameters for probe search mode : " + std::string(downOnInvalidStagePos ? "Slow Mode" : "Normal Mode"));
					ex.nextState = ProbeOperationSequence::po_error;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_can_do_work;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_can_do_work:
		{
			if (downOnInvalidStagePos)
			{
				// No need to check if stage is at valid position or not.
				doProbeOperationAsync(ex);
				return;
			}

			// Lambda to check if current sample position (theta motor/radius motor) is valid or not
			// Probe should not come down if there exist no valid carousel/plate position beneath it
			auto samplePosLambda = [ this, ex ](SamplePositionDLL pos) mutable -> void
			{
				// If position is valid, then move to next step otherwise set the next state to error state "po_error"
				if (!pos.isValid())
				{
					ex.nextState = ProbeOperationSequence::po_error;
					Logger::L().Log (MODULENAME, severity_level::error, "doProbeDownAsync : Probe Down Sequence : Invalid stage position!");
					ex.executeNextState(HawkeyeError::eSuccess);
				}
				else
				{
					this->doProbeOperationAsync(ex);
				}
			};

			// Get the current stage position
			SamplePositionDLL pos = {};
			getStagePosition(pos);
			samplePosLambda(pos);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_do_work:
		{
			// calculate the probe motor timeout(seconds)
			uint32_t timeout_sec = getProbeProperties().probeBusyTimeout_msec / 1000;

			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, getProbeProperties().probeStopPos, ePositionDescription::Target);

			// do the probe down and wait for callback
			probeMotor->ProbeDown([ ex ](bool status) mutable -> void
			{
				// If status is success then move to next step otherwise set the next state to timeout state "po_timeout"
				if (status)
				{
					ex.nextState = ProbeOperationSequence::po_complete;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_timeout;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, timeout_sec);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_complete:
		{
			// Check if probe is down
			// If probe is down then complete the operation otherwise set the next state to error state "po_error"
			if (isProbeDown())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeDownAsync : Probe Down Sequence : Success");

				// set the complete to stop enum execution
				ex.setComplete = true;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, getProbePosition(), ePositionDescription::AtPosition);
			}
			else
			{
				ex.nextState = ProbeOperationSequence::po_error;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
			}

			// run the next state asynchronously
			ex.executeNextState(HawkeyeError::eSuccess);
			return;
		}
		case ProbeOperationSequence::po_error:
		{
			// check if we can retry from beginning
			if (ex.canRetry(ProbeOperationSequence::po_entry_point))
			{
				ex.setComplete = false;

				// store the current probe motor errors
				auto currentProbeErrors = getCurrentProperties().probeErrorCodes;

				// clear the previous errors occurred for probe motors since we are retrying
				properties_[getCurrentType()].probeErrorCodes.clear();

				Logger::L().Log (MODULENAME, severity_level::error, "doProbeDownAsync : Probe down Sequence - Retrying probe down. Doing probe up first");

				// Do the probe up first before retrying for probe down again
				// this workflow does not require the user id, so do not use the transient user technique
				asyncCommandHelper_->postAsync(
					std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, numRetries),
					ProbeOperationSequence::po_entry_point,
					[ this, ex, currentProbeErrors ](bool probeUp) mutable -> void
				{
					if (probeUp)
					{
						ex.nextState = ProbeOperationSequence::po_entry_point;
					}
					else
					{
						for (auto ec : this->getCurrentProperties().probeErrorCodes)
						{
							// now "currentProbeErrors" has all the previous (probe down) error codes and new probe up error codes
							currentProbeErrors.push_back(ec);
						}

						properties_[getCurrentType()].probeErrorCodes = currentProbeErrors;
						currentProbeErrors.clear();

						Logger::L().Log (MODULENAME, severity_level::error, "doProbeDownAsync : Probe down Sequence - Failed to move probe up when doing retrying for probe down!");
						ex.nextState = ProbeOperationSequence::po_error;

						if (probePosInited)
						{
							probeMotor->triggerErrorReportCb (BuildErrorInstance(
								instrument_error::motion_motor_positionfail, 
								instrument_error::motion_motor_instances::sample_probe, 
								instrument_error::severity_level::error));
							reportErrorsAsync();
						}
					}
					doProbeOperationAsync(ex);
				});

				return;
			}

			Logger::L().Log (MODULENAME, severity_level::error, "doProbeDownAsync : Probe Down Sequence : Error Moving Probe Down!");
			if (probePosInited)
			{
				probeMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_positionfail, 
					instrument_error::motion_motor_instances::sample_probe, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
		}
		default:
		{
			doProbeOperationAsync(ex);
		}
	}
}

/// <summary>
/// Do the probe home operation asynchronously
/// </summary>
/// <param name="ex">The entry point for homing enum states</param>
void StageController::doProbeHomeAsync(AsyncCommandHelper::EnumStateParams ex)
{
	// get the current state
	auto curState = (ProbeOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeHomeAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point for probe home enum execution
		case ProbeOperationSequence::po_do_work:
		{
			// calculate the probe motor timeout(seconds)
			uint32_t timeout_sec = getProbeProperties().probeBusyTimeout_msec / 1000;

			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, getProbeProperties().probeHomePos, ePositionDescription::Target);

			// do the probe home and wait for callback
			probeMotor->Home([ ex ](bool status) mutable -> void
			{
				// If status is success then move to next step otherwise set the next state to timeout state "po_timeout"
				if (status)
				{
					ex.nextState = ProbeOperationSequence::po_complete;
				}
				else
				{
					ex.nextState = ProbeOperationSequence::po_timeout;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, timeout_sec);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case ProbeOperationSequence::po_complete:
		{
			// Check if probe is home
			// If probe is home then complete the operation otherwise set the next state to error state "po_error"
			if (isProbeHome())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeHomeAsync : Probe Home Sequence : Success");

				// set the complete to stop enum execution
				ex.setComplete = true;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, getProbePosition(), ePositionDescription::Home);
			}
			else
			{
				ex.nextState = ProbeOperationSequence::po_error;
				probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
			}
			ex.executeNextState(HawkeyeError::eSuccess);
			return;
		}
		case ProbeOperationSequence::po_error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "doProbeHomeAsync : Probe Home Sequence : Error homing probe!");

			if (probePosInited)
			{
				probeMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_homefail, 
					instrument_error::motion_motor_instances::sample_probe, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
		}
		default:
		{
			doProbeOperationAsync(ex);
		}
	}
}

/// <summary>
/// Base probe operation for asynchronous execution
/// </summary>
/// <param name="ex">The entry point for enum states</param>
void StageController::doProbeOperationAsync(AsyncCommandHelper::EnumStateParams ex)
{
	// get the current state
	auto curState = (ProbeOperationSequence) ex.currentState;
	std::string errStr = std::string();
	HawkeyeError he = HawkeyeError::eSuccess;

	Logger::L().Log (MODULENAME, severity_level::debug2, "doProbeOperationAsync : " + EnumConversion<ProbeOperationSequence>::enumToString(curState));

	switch (curState)
	{
		// This is the entry point
		case ProbeOperationSequence::po_entry_point:
		{
			// If probe motor is busy that means we cannot execute further
			// so set the next state as error state "po_error"
			if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::StatusBit::ProbeMotorBusy))
			{
				errStr.append("Motor motor reports busy prior to start.\n\t");
				ex.nextState = ProbeOperationSequence::po_error;
			}
			else
			{
				ex.nextState = ProbeOperationSequence::po_can_do_work;
			}
			break;
		}
		case ProbeOperationSequence::po_clear_error:
		{
			//probeMotor->ClearErrors();
			ex.nextState = ProbeOperationSequence::po_do_work;
			break;
		}
		case ProbeOperationSequence::po_can_do_work:
		{
			ex.nextState = ProbeOperationSequence::po_do_work;
			break;
		}
		case ProbeOperationSequence::po_do_work:
		{
			ex.nextState = ProbeOperationSequence::po_wait_for_work;
			break;
		}
		case ProbeOperationSequence::po_wait_for_work:
		{
			// If probe motor is busy that means we cannot execute further
			// so set the next state as error state "po_error"
			if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::StatusBit::ProbeMotorBusy))
			{
				errStr.append("Motor timed out reporting busy.\n\t");
				ex.nextState = ProbeOperationSequence::po_error;
			}
			else
			{
				ex.nextState = ProbeOperationSequence::po_complete;
			}
			break;
		}
		case ProbeOperationSequence::po_complete:
		{
			std::string logStr = std::string();
			logStr.append(boost::str(boost::format("doProbeOperationAsync : currentPos: %d") % probeMotor->GetPosition()));
			Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

			ex.setComplete = true;
			break;
		}
		case ProbeOperationSequence::po_timeout:
		{
			if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::StatusBit::ProbeMotorBusy))
			{
				errStr.append("Motor timed out reporting busy.\n\t");
			}
			else
			{
				errStr.append("Motor does not report correct position.\n\t");
			}
			ex.nextState = ProbeOperationSequence::po_error;
			break;
		}
		case ProbeOperationSequence::po_error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "doProbeOperationAsync: error");
			if (probePosInited)
			{
				probeMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_positionfail, 
					instrument_error::motion_motor_instances::sample_probe, 
					instrument_error::severity_level::error));
				reportErrorsAsync();
			}
		}
		default:
		{
			he = HawkeyeError::eHardwareFault;
			ex.setComplete = true;
			break;
		}
	}

	if (errStr.empty() == false)
	{
		std::string logStr = "doProbeOperationAsync : Error during probe operation : ";
		logStr.append(errStr);
		Logger::L().Log (MODULENAME, severity_level::error, logStr);
	}

	// run the next state asynchronously
	ex.executeNextState(he);
}

/// <summary>
/// Sets the probe motor position to target position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller whether target position is achieved</param>
/// <param name="targetPos">Probe motor target position</param>
void StageController::gotoProbePositionAsync(std::function<void(bool)> cb, int32_t targetPos)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoProbePositionAsync : <Enter> tgtPos : " + std::to_string(targetPos));

	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfInMotion, targetPos, ePositionDescription::Target);

	// Ask probe motor to go to target position asynchronously
	probeMotor->SetPosition([ this, targetPos, cb ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("gotoProbePositionAsync : %s with tgtPos <%d> , currentPos <%d>") % status % targetPos % getProbePosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, getProbePosition(), ePositionDescription::AtPosition);
		}
		else
		{
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
			probeMotor->triggerErrorReportCb (BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::sample_probe, 
				instrument_error::severity_level::error));
		}

		cb(status);

	}, targetPos, (this->getProbeProperties().probeBusyTimeout_msec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "gotoProbePositionAsync : <Exit>");
}

/// <summary>
/// Mark the current probe position as zeroth position
/// </summary>
/// <param name="cb">Callback to indicate caller whether position is marked</param>
void StageController::markProbePosAsZeroAsync(std::function<void(bool)> cb)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "markProbePosAsZeroAsync : <Enter> currentPos : " + std::to_string(getProbePosition()));

	probeMotor->MarkPosAsZero([ this, cb ](bool status) -> void
	{
		std::string logStr = boost::str(boost::format("markProbePosAsZeroAsync : %s currentPos <%d>") % status % getProbePosition());
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

		if (status)
		{
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, 0, ePositionDescription::AtPosition);
		}
		else
		{
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, getProbePosition(), ePositionDescription::Current);
			Logger::L().Log (MODULENAME, severity_level::error, logStr + " <Operation Failed>");
		}

		cb(status);

	}, (this->getProbeProperties().probeBusyTimeout_msec / 1000));

	Logger::L().Log (MODULENAME, severity_level::debug2, "markProbePosAsZeroAsync : <Exit>");
}

///////////////////////////////////////////////////////////////////////////////
////////////////////Carousel Controller Methods////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Initialize the carousel
/// <remarks>
/// This methods reads the carousel controller properties from "motor info" and "controller info" file
/// and save them as carouselProperties <see cref="StageController::StageControllerProperties"/>
/// </remarks>
/// </summary>
bool StageController::InitCarousel()
{
	// This is the old comment for this function: 
	// -- Now Load the carousel properties from "ControllerCal.info" file
	// -- There should not be any change in default "MotorControl.info" file
	// -- so reset all the node pointer to "ControllerCal.info" file
	//
	// After refactoring, we don't use the ControllerCal ptree anymore.
	// The ptree is still used for default params from the MotorControl file. 
	// The parameters that can be changed are now stored in the DB. 


	carouselControllerNode.reset();
	carouselControllerConfigNode.reset();

	if (!configNode)
	{
		return false;
	}

	controllersNode = configNode->get_child_optional("motor_controllers");      // look for the controllers section for individualized parameters...
	if (!controllersNode)
	{
		return false;
	}

	carouselControllerNode = controllersNode->get_child_optional(CAROUSELCONTROLLERNODENAME);     // look for this specific controller
	if (!carouselControllerNode)       // look for the motor parameters sub-node...
	{
		return false;
	}

	carouselControllerConfigNode = carouselControllerNode->get_child_optional("controller_config");
	if (!carouselControllerConfigNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "InitCarousel : Error parsing configuration file \"" + configFile + "\" - \"controller_config\" section not found");
		return false;
	}

	StageControllerProperties carouselProperties = ConfigCarouselVariables(properties_[eCarrierType::eCarousel]);

	properties_[eCarrierType::eCarousel] = carouselProperties;
	cachedProperties_[eCarrierType::eCarousel] = properties_[eCarrierType::eCarousel];

	return true;
}

/// <summary>
/// Initialize the carousel with default properties
/// <remarks>
/// This methods reads the carousel controller properties from motor info file
/// and save them as carouselProperties <see cref="StageController::StageControllerProperties"/>
/// </remarks>
/// </summary>
bool StageController::InitCarouselDefault()
{
	carouselControllerNode.reset();
	carouselMotorParamsNode.reset();
	carouselControllerConfigNode.reset();

	if (!configNode)
	{
		return false;
	}

	controllersNode = configNode->get_child_optional("motor_controllers");      // look for the controllers section for individualized parameters...
	if (!controllersNode)
	{
		return false;
	}

	carouselControllerNode = controllersNode->get_child_optional(CAROUSELCONTROLLERNODENAME);     // look for this specific controller
	if (!carouselControllerNode)       // look for the motor parameters sub-node...
	{
		return false;
	}

	carouselMotorParamsNode = carouselControllerNode->get_child_optional("motor_parameters");
	if (!carouselMotorParamsNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "InitCarouselDefault : Error parsing configuration file \"" + configFile + "\" - \"motor_parameters\" section not found");
		return false;
	}

	carouselControllerConfigNode = carouselControllerNode->get_child_optional("controller_config"); // look for the controller configuration parameters
	if (!carouselControllerConfigNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "InitCarouselDefault : Error parsing configuration file \"" + configFile + "\" - \"controller_config\" section not found");
		return false;
	}

	StageControllerProperties carouselProperties = {};
	carouselProperties = ConfigCarouselVariables();
	carouselProperties.controllerOk = false;
	properties_[eCarrierType::eCarousel] = carouselProperties;

	return true;
}

/// <summary>
/// Set the carousel properties as current stage controller properties
/// <remarks>
/// This method will set the theta and radius motors motion profiles
/// Initially "apply_" will be true and it will initialize the theta/radius/probe motors
/// after that we will make the "apply_" to avoid initialization of these motors
/// It will just set the change the motor profiles for these motors 
/// <see cref="StageController::StageControllerProperties::CommonMotorRegisters"/>
/// </remarks>
/// <param name="callback">Callback to return the status of the operation/param>
/// </summary>
void StageController::selectCarouselAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::vector<std::function<void(std::function<void(bool)>)>> asyncSelectCarouselTaskList;
	asyncSelectCarouselTaskList.clear();

	// Init Theta
	asyncSelectCarouselTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectCarouselAsync :  Initialize Theta");
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->thetaMotor->Init(cb, parentTree, carouselMotorParamsNode, apply_);
	});

	// Init Radius
	asyncSelectCarouselTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectCarouselAsync :  Initialize Radius");
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->radiusMotor->Init(cb, parentTree, carouselMotorParamsNode, apply_);
	});

	// Init Probe
	asyncSelectCarouselTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectCarouselAsync :  Initialize Probe");
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->probeMotor->Init(cb, parentTree, carouselMotorParamsNode, apply_);
	});

	// Update Theta/ Radius/ probe properties for Carousel
	auto UpdateMotorPropertiesForCarousel = [ this ]()
	{
		auto& carouselProperties = properties_[eCarrierType::eCarousel];

		/*Theta Properties*/
		// theta motor must always move in CW direction for carousel control...
		thetaMotor->SetHomeDirection(MotorDirection::Forward, true);      // set and apply the direction parameter if required

		thetaMotor->SetPosTolerance(carouselProperties.thetaProperties.thetaPositionTolerance);
		thetaMotor->SetMotorTimeouts(carouselProperties.thetaProperties.thetaStartTimeout_msec, carouselProperties.thetaProperties.thetaFullTimeout_msec);

		auto gearRatio = thetaMotor->GetGearRatio();
		auto unitsPerRev = thetaMotor->GetUnitsPerRev();
		if ((gearRatio) && (unitsPerRev))             // ensure non-zero values
		{
			maxCarouselPos = (int32_t) (unitsPerRev * gearRatio);
		}

		auto deadband = thetaMotor->GetDeadband();
		if (deadband > 0)
		{
			thetaDeadband = deadband;
		}

		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		degreesPerTube = (double_t) (360.0 / (double_t) maxTubes);

		if (maxCarouselPos == 0)
		{
			maxCarouselPos = MaxThetaPosition;
			unitsPerTube = ((double) maxCarouselPos / (double) maxTubes);
		}
		else
		{
			unitsPerTube = (((double) unitsPerRev * (double) gearRatio * degreesPerTube) / 360);
		}
		tubeCalcTolerance = (unitsPerTube * 0.65);
		tubePosTolerance = (unitsPerTube * 0.35);

		thetaMotor->SetMaxMotorPositionValue(maxCarouselPos);
#ifdef DYNAMIC_MOTOR_PARAMETERS
// currently not effective at applying running profile or other parameter changes; will reset positional awareness information
		thetaMotor->UpdateMotorParams([](bool) -> void {});
#endif // DYNAMIC_MOTOR_PARAMETERS

		/*Radius Properties*/
		radiusMotor->SetMotorTimeouts(carouselProperties.radiusProperties.radiusStartTimeout_msec, carouselProperties.radiusProperties.radiusFullTimeout_msec);

		int32_t maxTravel = radiusMotor->GetMaxTravel();             // get from motor parameters first
		if (maxTravel != 0)
		{
			carouselProperties.radiusProperties.radiusMaxTravel = maxTravel;                    // initialize from motor parameters first
		}

		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		/*Probe Properties*/
		probeMotor->SetPosTolerance(carouselProperties.probeProperties.probePositionTolerance);
		probeMotor->SetMotorTimeouts(carouselProperties.probeProperties.probeStartTimeout_msec, carouselProperties.probeProperties.probeBusyTimeout_msec);
		probeMotor->SetDefaultProbeStop(carouselProperties.probeProperties.probeStopPos);
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		// Get Updated Motor proifile for Carousel
		apply_ = false;
		carouselProperties.controllerOk = true;
		thetaMotor->GetMotorProfileRegisters(carouselProperties.thetaMotionProfile);
		radiusMotor->GetMotorProfileRegisters(carouselProperties.radiusMotionProfile);
		probeMotor->GetMotorProfileRegisters(carouselProperties.probeMotionProfile);
	};

	// execute the queued tasks
	asyncCommandHelper_->queueASynchronousTask([ this, callback, UpdateMotorPropertiesForCarousel ](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "selectCarouselAsync : Failed to Select the carousel!");
		}

		UpdateMotorPropertiesForCarousel();

		pCBOService_->enqueueInternal(callback, status);
	}, asyncSelectCarouselTaskList, false);

}

/// <summary>
/// Enables or disables the stage controller holding currents
/// Disabled during normal operations, but enabled during sample processing
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::enableStageHoldingCurrent(std::function<void(bool)> onCompletion, bool enable)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	Logger::L().Log (MODULENAME, severity_level::debug2, "enableStageHoldingCurrent: <enter>");

	if (!properties_[getCurrentType()].controllerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "enableStageHoldingCurrent: controller is not OK; profile not set");
		pCBOService_->enqueueInternal(onCompletion, false);
		return;
	}

	// Enable Theta / Radius holding current during work queue processing; disable at other times
	auto EnableHoldingCurrentForStage = [this, onCompletion, enable]() -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("enableStageHoldingCurrent: %s") % (enable ? "enable" : "disable")));

		// setup the list of callbacks with their own individual parameter requirements...
		auto enableRadiusHoldVoltageComplete = [this, onCompletion](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "enableStageHoldingCurrent: Failure enabling radius holding current");
			}
			
			Logger::L().Log (MODULENAME, severity_level::debug2, "enableStageHoldingCurrent: update radius holding current complete");
			onCompletion(true);
		};

		auto enableThetaHoldVoltageComplete = [this, enable, enableRadiusHoldVoltageComplete](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "enableStageHoldingCurrent: Failure enabling theta holding current");
			}

			Logger::L().Log (MODULENAME, severity_level::debug2, "enableStageHoldingCurrent: update theta holding current complete");
			radiusMotor->Enable(enableRadiusHoldVoltageComplete, enable, getRadiusProperties().radiusFullTimeout_msec);
		};

		// start the update process with the first operation
		thetaMotor->Enable(enableThetaHoldVoltageComplete, enable, getThetaProperties().thetaFullTimeout_msec);
	};

	pCBOService_->enqueueInternal(EnableHoldingCurrentForStage);

	Logger::L().Log (MODULENAME, severity_level::debug2, "enableStageHoldingCurrent: <exit>");
}

/// <summary>
/// Sets the stage controller motion profiles to carousel controller
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::setStageProfile(std::function<void(bool)> onCompletion, bool enable)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	Logger::L().Log (MODULENAME, severity_level::debug2, "setStageProfile: <enter>");

	if (!properties_[getCurrentType()].controllerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile: controller is not OK; profile not set");
		pCBOService_->enqueueInternal(onCompletion, false);
		return;
	}

#ifdef DYNAMIC_MOTOR_PARAMETERS
	// This section is intended to dynamically update the motor holding current values
	// It will require the firmware to lick up the new values, when sent, or otherwise
	// note the changes and pass them to the individual controller chips;
	// alternatively, the 'UpdateMotorParams' methods may be used to aply all changes made,
	// but the 'Init' controller command must NOT clear the position value!

	// Update Theta/ Radius holding current values during work queue processing
	auto EnableHoldingCurrentForStage = [this, onCompletion, enable]() -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("setStageProfile: EnableHoldingCurrentForCarousel: with enable %s") % (enable ? "true" : "false")));

		// setup the list of callbacks with their own individual parameter requirements...
		auto enableRadiusHoldVoltageComplete = [this, onCompletion](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure enabling radius holding current");
			}
			Logger::L().Log (MODULENAME, severity_level::debug1, "setStageProfile : update complete");

			onCompletion(true);
		};

		auto enableThetaHoldVoltageComplete = [this, enable, enableRadiusHoldVoltageComplete](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure enabling theta holding current");
			}

			radiusMotor->Enable(enableRadiusHoldVoltageComplete, enable, getRadiusProperties().radiusFullTimeout_msec);
		};

		// start the update process with the first operation
		thetaMotor->Enable(enableThetaHoldVoltageComplete, enable, getThetaProperties().thetaFullTimeout_msec);
	};

	// Update Theta/ Radius Carousel during work queue processing
	auto UpdateHoldingCurrentForStage = [this, onCompletion, enable]() -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("setStageProfile: UpdateHoldingCurrentForStage: with enable %s") % (enable ? "true" : "false")));

		uint32_t thetaRunVoltage = thetaMotor->GetRunVoltageDivide();
		uint32_t radiusRunVoltage = radiusMotor->GetRunVoltageDivide();
		uint32_t thetaHolding = 0;
		uint32_t radiusHolding = 0;

		if (enable)
		{
			thetaHolding = thetaRunVoltage / 2;
			radiusHolding = radiusRunVoltage / 2;
		}
		else
		{
			thetaHolding = thetaRunVoltage / 20;
			radiusHolding = radiusRunVoltage / 10;

			t_opPTree thetaMotorNode;
			t_opPTree radiusMotorNode;

			if (getCurrentType() ==eCarrierType::eCarousel)
			{
				thetaMotorNode = carouselControllerNode->get_child_optional("theta_motor");
				radiusMotorNode = carouselControllerNode->get_child_optional("radius_motor");
			}
			else
			{
				thetaMotorNode = plateControllerNode->get_child_optional("theta_motor");
				radiusMotorNode = plateControllerNode->get_child_optional("radius_motor");
			}

			if (thetaMotorNode)
			{
				boost::property_tree::ptree & thetaParams = thetaMotorNode.get();

				thetaHolding = thetaParams.get<uint32_t>("HoldVoltageDivide", thetaHolding);
			}

			if (radiusMotorNode)
			{
				boost::property_tree::ptree & radiusParams = motorNode.get();

				radiusHolding = radiusParams.get<uint32_t>("HoldVoltageDivide", radiusHolding);
			}
		}

		// setup the list of callbacks with their own individual parameter requirements...
		auto enableRadiusHoldVoltageComplete = [this, onCompletion](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure enabling radius holding current");
			}
			Logger::L().Log (MODULENAME, severity_level::debug1, "setStageProfile : update complete");

			onCompletion(true);
		};

		auto enableThetaHoldVoltageComplete = [this, thetaHolding, enableRadiusHoldVoltageComplete](bool enableOk) -> void
		{
			if (!enableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure enabling theta holding current");
			}

			radiusMotor->Enable(enableRadiusHoldVoltageComplete, true, getRadiusProperties().radiusFullTimeout_msec);
		};

		auto updateRadiusHoldVoltageComplete = [this, enableThetaHoldVoltageComplete](bool updateOk) -> void
		{
			if (!updateOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure applying radius holding current");
			}

			thetaMotor->Enable(enableThetaHoldVoltageComplete, true, getThetaProperties().thetaFullTimeout_msec);
		};

		boost::optional<std::function<void(bool)>> setRadiusHoldVoltageComplete = [this, updateRadiusHoldVoltageComplete](bool setOk) -> void
		{
			if (!setOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure setting radius holding current");
			}

			updateRadiusHoldVoltageComplete(true);		// to bypass the update operation that resets the position to '0'
														//			radiusMotor->UpdateMotorParams(updateRadiusHoldVoltageComplete);
		};

		auto disableRadiusHoldVoltageComplete = [this, radiusHolding, setRadiusHoldVoltageComplete](bool disableOk) -> void
		{
			if (!disableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure disabling radius holding current");
			}

			radiusMotor->SetHoldVoltDivide(radiusHolding, true, setRadiusHoldVoltageComplete);
		};

		auto updateThetaHoldVoltageComplete = [this, disableRadiusHoldVoltageComplete](bool updateOk) -> void
		{
			if (!updateOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure applying theta holding current");
			}
			radiusMotor->Enable(disableRadiusHoldVoltageComplete, false, getRadiusProperties().radiusFullTimeout_msec);
		};

		boost::optional<std::function<void(bool)>> setThetaHoldVoltageComplete = [this, updateThetaHoldVoltageComplete](bool setOk) -> void
		{
			if (!setOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure setting theta holding current");
			}

			updateThetaHoldVoltageComplete(true);		// to bypass the update operation that resets the position to '0'
														//			thetaMotor->UpdateMotorParams(updateThetaHoldVoltageComplete);
		};

		auto disableThetaHoldVoltageComplete = [this, thetaHolding, setThetaHoldVoltageComplete](bool disableOk) -> void
		{
			if (!disableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setStageProfile : Failure disabling theta holding current");
			}

			thetaMotor->SetHoldVoltDivide(thetaHolding, true, setThetaHoldVoltageComplete);
		};

		// start the update process with the first operation
		thetaMotor->Enable(disableThetaHoldVoltageComplete, false, getThetaProperties().thetaFullTimeout_msec);
	};

	pCBOService_->enqueueInternal(UpdateHoldingCurrentForStage);

#else

	enableStageHoldingCurrent(onCompletion, enable);

#endif // DYNAMIC_MOTOR_PARAMETERS

	Logger::L().Log (MODULENAME, severity_level::debug3, "setStageProfile: <exit>");
}

/// <summary>
/// Sets the stage controller motion profiles to carousel controller
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::setCarouselMotionProfile(std::function<void(bool)> onCompletion)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	if (!properties_[eCarrierType::eCarousel].controllerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setCarouselMotionProfile: controller is not OK; profile not set");
		pCBOService_->enqueueInternal(onCompletion, false);
		return;
	}

	// Radius Motor completion
	auto radiusComplete = [ this, onCompletion ](bool status)->void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setCarouselMotionProfile: unable to set radius motor motion profile!");
		}

		onCompletion(status);
	};


	// Theta Motor completion (will trigger radius)
	auto thetaComplete = [ this, radiusComplete, onCompletion ](bool status)->void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setCarouselMotionProfile: unable to set theta motor motion profile!");
			onCompletion(false);
			return;
		}

		radiusMotor->SetMotorProfileRegisters(radiusComplete, properties_[eCarrierType::eCarousel].radiusMotionProfile);

	};

	thetaMotor->SetMotorProfileRegisters(thetaComplete, properties_[eCarrierType::eCarousel].thetaMotionProfile);
}

/// <summary>
/// Call this method to update carousel properties <see cref="StageController::StageControllerProperties"/>
/// from motor info file
/// </summary>
/// <param name="defaultValues">The default stage controller properties</param>
/// <returns>The carousel controller properties</returns>
StageController::StageControllerProperties StageController::ConfigCarouselVariables(boost::optional<StageControllerProperties> defaultValues)
{
	StageControllerProperties carouselProperties;
	boost::property_tree::ptree& config = carouselControllerConfigNode.get();

	if (!defaultValues)
	{
		carouselProperties = {};
		carouselProperties.thetaProperties = ConfigThetaVariables (config, eCarrierType::eCarousel);
		carouselProperties.radiusProperties = ConfigRadiusVariables (config, eCarrierType::eCarousel);
		carouselProperties.probeProperties = ConfigProbeVariables (config, eCarrierType::eCarousel);
	}
	else
	{
		carouselProperties = defaultValues.get();
		carouselProperties.thetaProperties = ConfigThetaVariables (config,eCarrierType::eCarousel, carouselProperties.thetaProperties);
		carouselProperties.radiusProperties = ConfigRadiusVariables (config,eCarrierType::eCarousel, carouselProperties.radiusProperties);
		carouselProperties.probeProperties = ConfigProbeVariables (config,eCarrierType::eCarousel, carouselProperties.probeProperties);
	}

	maxTubes = config.get<int32_t>("CarouselTubes", maxTubes);
	carouselProperties.retriesCount = config.get<int32_t>("ControllerInternalRetries", carouselProperties.retriesCount);

	return carouselProperties;
}

/// <summary>
/// Call this method to update motor info file from
/// carousel controller properties <see cref="StageController::StageControllerProperties"/>
/// </summary>
/// <returns><c>[0]</c> if success; otherwise failed</returns>
int32_t StageController::WriteCarouselControllerConfig()
{
	if (!updateCarouselConfig)
	{
		return 0;
	}

	// Unable to save through this method if no configuration location was given!
	if (!carouselControllerConfigNode)
	{
		updateCarouselConfig = false;
		return -1;
	}


	//
	// Update the DB with new values
	//
	int32_t updateStatus = UpdateControllerConfig (eCarrierType::eCarousel, carouselControllerNode, carouselControllerConfigNode);
	if (updateStatus == 0)
	{
		auto v1 = properties_[eCarrierType::eCarousel].thetaProperties.thetaHomePosOffset;
		auto v2 = properties_[eCarrierType::eCarousel].radiusProperties.radiusOffset;
		DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();
		instCfg.CarouselThetaHomeOffset = v1;
		instCfg.CarouselRadiusOffset = v2;		
		if (!InstrumentConfig::Instance().Set())
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
	updateCarouselConfig = false;

	return updateStatus;
}

/// <summary>
/// Find a carousel tube
/// </summary>
/// <param name="mode">if set to `FIRST`, include the current position in the search for a position holding a tube</param>
/// <param name="finalTubePosition">optional parameter to indicate the final tube position</param>
/// <remarks>
/// If we are unable to find any tube between current carousel position to
/// "finalTubePosition" then we will return false.
/// if optional parameter is empty then carousel will take full rotation to find tube present
/// </remarks>
/// <returns><c>[true]</c> if carousel tube found; otherwise <c>[false]</c></returns>
void StageController::FindNextCarouselTube(std::function<void(bool)> callback,
                                           StageControllerBase::FindTubeMode mode,
                                           boost::optional<uint32_t> finalTubePosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug2, "FindNextCarouselTube: <enter>");
	// look for an occupied tube position
	// if requested, search includes the current position
	// return true if a tube is found, or false if the entire carousel is searched with no found tube

	// If carousel is not present then no need to search for tubes
	if (!checkCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "FindNextCarouselTube:  Carousel is not detected!");
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	StagePositionOk([ this, callback, mode, finalTubePosition ](SamplePositionDLL currentPos) -> void
	{
		uint32_t currentTubeNumber = currentPos.getColumn();
		if (currentPos.isValidForCarousel() == false)	// should NEVER get an invalid position if StagePositionOk completed successfully; ensure a valid position
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "FindNextCarouselTube:  Invalid sample position after StagePositionOk!");

			int32_t homeThetaPos = 0;
			int32_t homeRadiusPos = 0;
			int32_t tgtTubeNum = 0;

			getCarouselHomePosition(homeThetaPos, homeRadiusPos);
			int32_t tPos = GetThetaPosition();

			currentTubeNumber = getApproxCarouselTubeNumber(tPos, homeThetaPos);

			currentPos.setRowColumn(eStageRows::CarouselRow, static_cast<uint8_t>(currentTubeNumber));
		}

		// If current carousel tube position is same as "finalTubePosition"
		// then check for tube present here and return
		if (currentPos.isValidForCarousel() && finalTubePosition && currentPos.getColumn() == finalTubePosition.get())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "FindNextCarouselTube: <exit: current tube position is same as final tube position>");
			checkTubePresent_DelayedAsync(callback);
			return;
		}

		if (!cancelMove_ && (!checkTubePresent() || mode == FindTubeMode::NEXT))
		{
			// get the final tube number
			uint32_t finalTubeNumber = finalTubePosition.get_value_or(currentTubeNumber);

			MoveClear();

			// Run asynchronous "findNextCarouselTubeAsync()" method and wait for it to finish.
			// This call will block the thread from which it is being called
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::findNextCarouselTubeAsync, this, std::placeholders::_1, currentTubeNumber, finalTubeNumber),
				CarouselOperationSequence::cc_entry_point,
				callback);
			return;
		}
		pCBOService_->enqueueInternal(callback, true);
	});
}

/// <summary>
/// Gets the nearest carousel tube position
/// </summary>
/// <returns>If <c>[Carousel Present]</c> nearest carousel tube position; otherwise "0"</returns>
uint32_t StageController::getNearestTubePosition()
{
	static uint32_t prevCurrentTubePosition = 0;

	if (!checkCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "getNearestTubePosition:  Carousel is not detected!");
		return 0;
	}
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;

	// get the cached current theta position
	int32_t currentTPos = GetThetaPosition();

	// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	getCarouselHomePosition(homeThetaPos, homeRadiusPos);

	// Find the approximate carousel position from current theta position
	uint32_t currentTubeNum = getApproxCarouselTubeNumber(currentTPos, homeThetaPos);

	if (currentTubeNum != prevCurrentTubePosition)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "getNearestTubePosition:  Nearest carousel tube position : " + std::to_string(currentTubeNum));
		prevCurrentTubePosition = currentTubeNum;
	}

	return currentTubeNum;
}

/// <summary>
/// Search for a carousel position containing a tube
/// </summary>
/// <param name="mode">if set to `FIRST`, include the current position in the search for a position holding a tube</param>
/// <param name="finalTubePosition">optional parameter to indicate the final tube position</param>
/// <remarks>
/// If we are unable to find any tube between current carousel position to
/// "finalTubePosition" then we will return false.
/// if optional parameter is empty then carousel will take full rotation to find tube present
/// </remarks>
/// <returns><c>[true]</c> if carousel tube found; otherwise <c>[false]</c></returns>
void StageController::findTubeAsync(std::function<void(bool)> callback, StageControllerBase::FindTubeMode mode, boost::optional<uint32_t> finalTubePosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// If carousel is not present then no need to search for tubes
	if (!checkCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "findTubeAsync : Carousel is not detected!");
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	auto onComplete = [ this, callback, finalTubePosition ](bool tubeFound) -> void
	{
		if (cancelMove_)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "findTubeAsync : operation cancelled");
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		if (!tubeFound && !finalTubePosition)	// If "finalTubePosition" has valid value then don't force go to position 1
		{
// TODO: determine if tube position really needs to be reset to position 1 after a completed tube search
			auto tubeCallback = [ this, callback, tubeFound ](bool status)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "findTubeAsync : tube not found and no final tube position");
				pCBOService_->enqueueInternal(callback, tubeFound);
			};
			
			SamplePositionDLL pos = SamplePositionDLL(eStageRows::CarouselRow, 1);
			moveStageToPosition(tubeCallback, pos);	// GotoTubeNum(1);
			return;
		}

		if (!tubeFound)
		{
			std::string logStr = (boost::str(boost::format("findTubeAsync: tube found: %s") % ((tubeFound == true) ? "yes" : "no")));
			Logger::L().Log (MODULENAME, severity_level::debug1, logStr);
		}

		pCBOService_->enqueueInternal(callback, tubeFound);
	};

	probeUpAsync([ = ](bool success)
	{
		if (!success)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "findTubeAsync : Failed to move probe up!");
			pCBOService_->enqueueInternal(callback, false);
			return;
		}
		pCBOService_->enqueueInternal(
			std::bind(&StageController::FindNextCarouselTube, this, onComplete, mode, finalTubePosition));
	});
}

/// <summary>
/// Call this method to check if current stage is of type carousel <see cref="StageController::eCarrierType::eCarousel"/>
/// </summary>
/// <returns><c>[true]</c> if carousel present; otherwise <c>[false]</c></returns>
bool StageController::checkCarouselPresent(void)
{
	bool isCarouselPresent = false;
	if (ControllerOk())
	{
		isCarouselPresent = pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::CarouselPresent);
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, boost::str(boost::format("checkCarouselPresent : %s") % ((isCarouselPresent == true) ? "true" : "false")));

	return isCarouselPresent;
}

/// <summary>
/// Call this method to check if carousel tube is present at stage position or not
/// </summary>
/// <returns><c>[true]</c> if carousel tube present; otherwise <c>[false]</c></returns>
bool StageController::checkTubePresent()
{
	bool tubeOk = false;

	if (checkCarouselPresent())
	{
		tubeOk = pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::CarouselTube);
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, boost::str(boost::format("checkTubePresent : %s") % ((tubeOk == true) ? "true" : "false")));

	return tubeOk;
}

/// <summary>
/// Move the stage to next carousel tube position
/// </summary>
/// <returns>The next carousel tube position</returns>
void StageController::goToNextTubeAsync(std::function<void(boost::optional<uint32_t>)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// If carousel is not present then no need to search for tubes
	if (!checkCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "GotoNextTubeAsync : Invalid operation - Carousel is not detected!");
		pCBOService_->enqueueInternal(std::bind(callback, boost::none));
		return;
	}

	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;

	// get the cached current theta position
	int32_t currentTPos = GetThetaPosition();

	// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	getCarouselHomePosition(homeThetaPos, homeRadiusPos);

	// Find the approximate carousel position from current theta and radius position
	uint32_t currentTubeNum = getApproxCarouselTubeNumber(currentTPos, homeThetaPos);
	SamplePositionDLL pos(eStageRows::CarouselRow, static_cast<uint8_t>(currentTubeNum));

	std::string logStr = boost::str(boost::format("GotoNextTubeAsync : current tube: %d") % currentTubeNum);

	// Increment current tube number by 1 to get the next carousel position
	pos.incrementColumn();
	currentTubeNum = pos.getColumn();

	// Move the stage controller to new position
	auto onMoveStageComplete = [ this, callback, currentTubeNum ](bool status)
	{
		boost::optional<uint32_t> opTubeNum = boost::none;

		if (cancelMove_)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "GotoNextTubeAsync : operation cancelled");
			pCBOService_->enqueueInternal(std::bind(callback, opTubeNum));
			return;
		}

		if (!status)
		{
			std::string logStr;
			logStr.append(boost::str(boost::format("\n Failed to moved to tube number: %d") % currentTubeNum));
			Logger::L().Log (MODULENAME, severity_level::error, logStr);
		}
		else
		{
			opTubeNum = currentTubeNum;
		}
		pCBOService_->enqueueInternal(std::bind(callback, opTubeNum));
	};

	pCBOService_->enqueueInternal([ this, onMoveStageComplete, pos ]()
	{
		moveStageToPosition(onMoveStageComplete, pos);
	});
}

/// <summary>
/// Find the next carousel tube position asynchronously
/// </summary>
/// <param name="ex">The entry point for find next carousel tube operation enum states</param>
/// <param name="initialTubeNum">
/// The current carousel tube position. This position may contain tube in it but will not be considered
/// System will find the next tube present.
/// </param>
///<param name="finalTubeNumber">
/// The execution will stop if we reach at finalTubeNumber and couldn't find any tube in between "initialTubeNum"
/// and "finalTubeNumber".
/// If "finalTubeNumber" is invalid or equal to "initialTubeNum", it will be ignored
/// </param>
void StageController::findNextCarouselTubeAsync(AsyncCommandHelper::EnumStateParams ex, uint32_t initialTubeNum, uint32_t finalTubeNumber)
{
	// get the current state
	auto curState = (CarouselOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "findNextCarouselTubeAsync : " + EnumConversion<CarouselOperationSequence>::enumToString(curState));

	if (cancelMove_)
	{
		ex.nextState = CarouselOperationSequence::cc_cancel;
		curState = CarouselOperationSequence::cc_cancel;
		cancelThetaMove_ = true;
		cancelRadiusMove_ = true;
	}

	// Get the current theta position
	int32_t tPos = GetThetaPosition();

	switch (curState)
	{
		// This is the entry point for "find next carousel tube operation" enum execution
		case CarouselOperationSequence::cc_entry_point:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "findNextCarouselTubeAsync : Initial tube number : " + std::to_string(initialTubeNum));

			// If carousel is not present then no need to search for tubes.
			// Set the next state to error state "cc_error"
			if (checkCarouselPresent() == false)
			{
				Logger::L().Log (MODULENAME, severity_level::normal, "findNextCarouselTubeAsync : Unable to perform operation : Carousel is not detected!");
				ex.nextState = CarouselOperationSequence::cc_error;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			// Set the max retries to max tube count
			// This will allow the same states to be executed multiple times with stopping the execution
			ex.maxRetries = maxTubes + 1; // Plus 1 to compensate for (original run + retries)

			uint32_t currentTubeNum = getNearestTubePosition();

			// Check if the carousel is actually at the tube number position which is being
			// passed as the 'initialTubeNum' value.
			//
			// If the position is different, then the carousel might have been moved, or the
			// theta and/or radius position value may have 'settled' to an encoder tick value
			// differing from that used to determine the pre-method position parameters.
			//
			// The method now checks on process entry for the nearest tube position number, since
			// the returned tube number value will be the tube to which a valid carousel move can
			// be performed, allowing small positional corrections to center a tube.  The method
			// also compares the supplied initial tube number with the determined tube number,
			// and if not a match, updates the initial tube number, checks whether the specified
			// final tube needs to be (or can be) updated, and updates if appropriate.
			//
			// The method does NOT require the current carousel sample position (row-col) value
			// be 'valid' (i.e at a location whose theta and radius positions corresponded EXACTLY
			// to a tube location).  Requiring valid can cause position rejection errors which
			// could allow the carousel to advance to the next tube position, and potentially
			// allow discard of unprocessed tubes, depending on the workflow invoking the tube
			// search operation.

			if (currentTubeNum != initialTubeNum)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("findNextCarouselTubeAsync : entry tube position change: initial - %d  current - %d") % initialTubeNum % currentTubeNum));
				if (initialTubeNum == finalTubeNumber)
				{
					finalTubeNumber = currentTubeNum;
				}
				initialTubeNum = currentTubeNum;
			}
			ex.nextState = CarouselOperationSequence::cc_do_work;

			ex.executeNextState(HawkeyeError::eSuccess);

			return;
		}
		case CarouselOperationSequence::cc_do_work:
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, tPos, ePositionDescription::Current);

			// Find the approximate carousel tube position
			uint32_t currentTubeNum = getNearestTubePosition();

			// Increment current tube number by 1 to get the next carousel position
			currentTubeNum++;
			if (currentTubeNum > maxTubes)
			{
				currentTubeNum = 1;
			}
			SamplePositionDLL pos = SamplePositionDLL(eStageRows::CarouselRow, static_cast<uint8_t>(currentTubeNum));

			// If tube number is same as initial carousel tube number
			// that means carousel has taken a full revolution and couldn't find any tubes.
			// Set the next state as error state "cc_error"
			if (currentTubeNum == initialTubeNum)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : No tube found!");
				ex.nextState = CarouselOperationSequence::cc_error;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			// If final tube position has been traversed without any tube found in-between,
			// and final tube position is neither invalid nor equal to initial tube position,
			// stop execution
			bool ignoreFinalTubeCheck = initialTubeNum == finalTubeNumber || finalTubeNumber > maxTubes || finalTubeNumber == 0;
			uint32_t finalTubeCheck = finalTubeNumber + 1;
			if (finalTubeCheck > maxTubes)
			{
				finalTubeCheck = 1;
			}
			if (!ignoreFinalTubeCheck && currentTubeNum == finalTubeCheck)
			{
				ex.nextState = CarouselOperationSequence::cc_no_tube_found;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			// Otherwise, move the stage to new position
			moveCarouselToPositionAsync([ ex ](bool status) mutable -> void
			{
				if (status)
				{
					ex.nextState = CarouselOperationSequence::cc_wait_for_work;
				}
				else
				{
					ex.nextState = CarouselOperationSequence::cc_error;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			}, pos);

			return;
		}
		case CarouselOperationSequence::cc_wait_for_work:
		{
			// check if tube is present
			checkTubePresent_DelayedAsync([ ex ](bool tubeFound) mutable -> void
			{
				// If tube is not found then again move stage and check for tube.
				// If tube is found the complete the execution
				if (tubeFound)
				{
					ex.nextState = CarouselOperationSequence::cc_complete;
				}
				else
				{
					ex.nextState = CarouselOperationSequence::cc_do_work;
				}
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Here we need to exit. Callback will resume this execution.
			return;
		}
		case CarouselOperationSequence::cc_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : tube find cancelled!");

			// create a lambda to catch the status of the radius motor stop command
			auto cancelRadiusStop = [ this ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "findNextCarouselTubeAsync : tube find cancel: radius stop command failed!");

					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::radius, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}

				// create a lambda to catch the status of the theta motor stop command
				auto cancelThetaStop = [ this ](bool status) -> void
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "findNextCarouselTubeAsync : tube find cancel: theta stop command failed!");

						thetaMotor->triggerErrorReportCb (BuildErrorInstance(
							instrument_error::motion_motor_operationlogic, 
							instrument_error::motion_motor_instances::theta, 
							instrument_error::severity_level::error));
						reportErrorsAsync();
					}
					return;
				};

				thetaMotor->Stop(cancelThetaStop, true, getThetaProperties().thetaFullTimeout_msec);		// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation

				return;
			};

			// assume both motors may be moving
			radiusMotor->Stop(cancelRadiusStop, true, getRadiusProperties().radiusFullTimeout_msec);		// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation

																											// set the complete to stop enum execution
			ex.setComplete = true;
			break;
		}
		case CarouselOperationSequence::cc_complete:
		{
			// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
			int32_t homeThetaPos = 0;
			int32_t homeRadiusPos = 0;
			getCarouselHomePosition(homeThetaPos, homeRadiusPos);

			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, tPos, ePositionDescription::Current);

			// Find the approximate carousel position from current theta and radius position
			uint32_t currentTubeNum = this->getApproxCarouselTubeNumber(tPos, homeThetaPos);

			Logger::L().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : Tube Found Successfully at position : " + std::to_string(currentTubeNum));

			// set the complete to stop enum execution
			ex.setComplete = true;
			MoveClear();
			break;
		}
		case CarouselOperationSequence::cc_no_tube_found:
		{
			Logger::L ().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : no tube found");
		
			// set the complete to stop enum execution
			ex.setComplete = true;
			MoveClear();

			// run the next state asynchronously
			ex.executeNextState(HawkeyeError::eHardwareFault);
			return;
		}
		case CarouselOperationSequence::cc_timeout:
		case CarouselOperationSequence::cc_error:
		{
			if (curState == CarouselOperationSequence::cc_timeout)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : Unable to perform operation : timeout occurred!");
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "findNextCarouselTubeAsync : Unable to perform operation : error occurred!");
			}
		}
		default:
		{
			// set the complete to stop enum execution
			ex.setComplete = true;
			MoveClear();

			// run the next state asynchronously
			ex.executeNextState(HawkeyeError::eHardwareFault);
			return;
		}
	}

	ex.executeNextState(HawkeyeError::eSuccess);
}

/// <summary>
/// Get the carousel stage controller home position
/// </summary>
/// <remarks>
/// This carousel theta and radius home position is different from actual theta/radius home position (i.e. "0")
/// </remarks>
/// <param name="thetaPos">The home theta position</param>
/// <param name="radiusPos">The home radius position</param>
void StageController::getCarouselHomePosition(int32_t& thetaPos, int32_t& radiusPos)
{
	auto thetaProperties = properties_[eCarrierType::eCarousel].thetaProperties;
	int32_t thetaHomePosOffset = thetaProperties.thetaHomePosOffset;

	int32_t tgtOffset = 0;
	int32_t tgtPos = 0;

	if (thetaHomePosOffset != 0)
	{
		tgtOffset = thetaHomePosOffset;
		tgtPos = thetaHomePosOffset;

		if (thetaHomePosOffset < 0)
		{
			// correct for backlash
			tgtOffset += thetaProperties.thetaBacklash;

			// move beyond to make final move in normal direction; 
			// this 'step' is really an absolute position relative to the just-marked '0'
			tgtPos = tgtOffset - (int32_t) tubePosTolerance;
		}

		// check for potential backlash from reverse direction movement
		if (tgtOffset < 0)
		{
			// convert to same range as tgtOffset
			int32_t pos = GetThetaPosition() - maxCarouselPos;

			// calculate the step back to the desired location; include backlash correction
			tgtPos = (tgtOffset - pos);
		}
	}
	thetaPos = tgtPos;

	int32_t tgtRadiusPos = properties_[eCarrierType::eCarousel].radiusProperties.radiusOffset;
	radiusPos = tgtRadiusPos;
}

/// <summary>
/// Calculates the theta motor position for the input carousel stage position (column number / tube position)
/// </summary>
/// <param name="tgtCol">The input column number (tube position) for carousel controller</param>
/// <param name="thetaPos">The placeholder reference for the returned theta motor position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::getCarouselPositionFromRowCol(uint32_t tgtCol, int32_t& thetaPos) const
{
	double_t thetaAngle = 0.0;
	int32_t tPos = 0;
	uint32_t tube = tgtCol;

	while (tube <= 0)
	{
		tube += maxTubes;
	}
	tube--;     // convert to index

	tube %= maxTubes;

	thetaAngle = (tube * degreesPerTube);               // assume tube 1 is at the home position with home = 0

	tPos = (int32_t) (thetaAngle * 10);                 // user units are in 0.1 degrees, 3600 user units per full final revolution

	normalizeThetaPosition(tPos, maxCarouselPos);

	thetaPos = tPos;

	return true;
}

/// <summary>
/// Normalize the current theta position w.r.t. carousel controller maximum position
/// </summary>
/// <param name="currentThetaPos">The current theta position</param>
/// <param name="thetaPos">The placeholder for normalized theta position</param>
void StageController::doCarouselThetaCW(const int32_t& currentThetaPos, int32_t& thetaPos) const
{
	// Prevent the theta motor to running counter clockwise
	while (thetaPos < currentThetaPos)
	{
		thetaPos += maxCarouselPos;
	}
}

/// <summary>
/// Calculate carousel theta position from actual current theta position
/// </summary>
/// <remarks>
/// Theta home is different then carousel theta home
/// </remarks>
/// <param name="currentThetaPos">The actual current theta position</param>
/// <param name="thetaPos">The placeholder for calculated carousel theta position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::adjustCarouselThetaPos(const int32_t& currentThetaPos, int32_t& tgtThetaPos) const
{
	int32_t currentTPos = currentThetaPos;
	normalizeThetaPosition(currentTPos, maxCarouselPos);

	// allow backwards movement within the tolerance
	int32_t chkPos = currentTPos - (int32_t) tubePosTolerance;
	if (tgtThetaPos < chkPos)
	{
		while (tgtThetaPos < chkPos)
		{
			tgtThetaPos += maxCarouselPos;
		}
	}
	else
	{
		while (tgtThetaPos > maxCarouselPos)
		{
			tgtThetaPos -= maxCarouselPos;
		}
	}

	int32_t absChg = abs(tgtThetaPos - chkPos);
	if ((absChg > 0) && (absChg <= ((int32_t) tubePosTolerance)))		// change is small enough to allow move backward
	{
		if (tgtThetaPos < currentTPos)
		{
			tgtThetaPos -= (thetaDeadband / 2);
		}
		else if (tgtThetaPos > currentTPos)
		{
			tgtThetaPos += (thetaDeadband / 2);
		}
	}
	else
	{
		doCarouselThetaCW(currentTPos, tgtThetaPos);
	}

	return true;
}

/// <summary>
/// Calculate carousel radius position
/// </summary>
/// <remarks>
/// Radius home is different then carousel radius home
/// </remarks>
/// <param name="radiusPos">The placeholder for calculated carousel radius home position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::adjustCarouselRadiusPos(int32_t& radiusPos) const
{
	if (radiusPos > properties_.at(eCarrierType::eCarousel).radiusProperties.radiusOffset)
	{
		radiusPos = properties_.at(eCarrierType::eCarousel).radiusProperties.radiusOffset;
	}
	return true;
}

/// <summary>
/// Move carousel stage controller to home position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::doCarouselHomeAsync(std::function<void(bool)> cb)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselHomeAsync : <Enter>");

	MoveClear();

	eCarrierType currentType = getCurrentType();
	if (currentType !=eCarrierType::eCarousel)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselHomeAsync : Invalid controller type!");
		cb(false);
		return;
	}

	// get latest radius and theta position
	int32_t currentTPos = 0;
	int32_t currentRPos = 0;
	getThetaRadiusPosition(currentTPos, currentRPos);

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);

	// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t tgtThetaPos = 0;
	int32_t tgtRadiusPos = 0;
	this->getCarouselHomePosition(tgtThetaPos, tgtRadiusPos);

	// Adjust theta position to prevent carousel counter clockwise movement
	doCarouselThetaCW(currentTPos, tgtThetaPos);
	this->adjustCarouselRadiusPos(tgtRadiusPos);

	// Set carousel stage controller target theta properties
	auto thetaProperties = properties_[currentType].thetaProperties;
	MoveStageParams thetaParams = MoveStageParams([ this, tgtThetaPos ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForCarouselMovement(retriesDone, false, tgtThetaPos, true);
	}, tgtThetaPos, (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);			// use deadband as check tolerance

	// Set carousel stage controller target radius properties
	auto radiusProperties = properties_[currentType].radiusProperties;
	MoveStageParams radiusParams = MoveStageParams([ this, tgtRadiusPos ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForCarouselMovement(retriesDone, true, tgtRadiusPos);
	}, tgtRadiusPos, (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);		// use deadband as check tolerance

	// Move stage to target position asynchronously
	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams ](auto ex)
	{
		moveStageToPositionAsync(ex, thetaParams, radiusParams, false);
	}, StageControllerOperationSequence::sc_entry_point, cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselHomeAsync : <Exit>");
}

/// <summary>
/// Move carousel controller radius motor to home position asynchronously
/// </summary>
/// <remarks>
/// This method will only move radius motor</remarks>
/// <param name="cb">Callback to indicate caller</param>
void StageController::doCarouselRadiusHomeAsync(std::function<void(bool)> cb)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselRadiusHomeAsync : <Enter>");
	eCarrierType currentType = getCurrentType();
	if (currentType !=eCarrierType::eCarousel)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselRadiusHomeAsync : Invalid controller type!");
		cb(false);
		return;
	}

	// get the carousel radius home position
	int32_t tgtRadiusPos = properties_[eCarrierType::eCarousel].radiusProperties.radiusOffset;
	adjustCarouselRadiusPos(tgtRadiusPos);

	// move radius motor to carouse radius home position
	uint32_t waitMilliSec = getRadiusProperties().radiusFullTimeout_msec;
	gotoRadiusPositionAsync(cb, tgtRadiusPos, waitMilliSec);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doCarouselRadiusHomeAsync : <Exit>");
}

/// <summary>
/// Call this method to check if carousel stage movement retries are available for target position
/// If carousel stage is not able to move to target position in multiple attempts
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
/// <param name="retriesDone">The number of retries already done for target position</param>
/// <param name="radius">The value indicating if current motor movement is for radius motor</param>
/// <param name="tgtPos">The target motor position</param>
bool StageController::canDoRetryForCarouselMovement(uint16_t retriesDone, bool radius, int32_t tgtPos, bool enableTubeCheck)
{
	// If we have already exhausted the maximum retries allowed then no need to go further
	if (retriesDone >= getCurrentProperties().retriesCount + 1) // Plus 1 to compensate for (original run + retries)
	{
		return false;
	}

	if (radius)
	{
		return true;
	}

	if (enableTubeCheck && checkTubePresent())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "canDoRetryForCarouselMovement : Tube found : Failed to retry theta motion");
		return false;
	}

	// For Carousel, theta motor should not move counter clockwise to
	// prevent unprocessed tube dumping into discard tray.
	int32_t currentTPos = GetThetaPosition();
	int32_t currentRPos = GetRadiusPosition();

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);

	int32_t targetPosition = tgtPos % this->getThetaProperties().maxThetaPos;
	if (ThetaAtPosition(currentTPos, targetPosition, 0, true))	// Check here for deadband only
	{
		return true;
	}

	if (currentTPos < targetPosition)
	{
		return true;
	}

	// If theta motor has move passed the target position, then check for 
	// if it has not moved (from desired location) more "tubePosTolerance" from target
	// If and only if that is the case then move carousel backward.
	int32_t offSet = (int32_t) (std::round(tubePosTolerance));
	return ThetaAtPosition(currentTPos, (targetPosition + offSet), 0, true);
}

/// <summary>
/// Move the carousel stage to target position
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
/// <param name="pos">Target carousel stage position</param>
void StageController::moveCarouselToPositionAsync(std::function<void(bool)> cb, SamplePositionDLL pos)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveCarouselToPositionAsync : <Enter>");
	bool canMoveCarousel = true;

	MoveClear();

	eCarrierType currentType = getCurrentType();
	if (currentType !=eCarrierType::eCarousel)
	{
		canMoveCarousel = false;
		Logger::L().Log (MODULENAME, severity_level::error, "moveCarouselToPositionAsync : Invalid controller type!");
	}

	if (pos.isValidForCarousel() == false)
	{
		canMoveCarousel = false;
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("moveCarouselToPositionAsync : Invalid Carousel position! row <%c>, col <%d>") % pos.getRow() % pos.getColumn()));
	}

	// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;
	getCarouselHomePosition(homeThetaPos, homeRadiusPos);

	static boost::optional<uint32_t> prevCol = boost::none;
	uint32_t tgtCol = pos.getColumn();
	int32_t tgtThetaPos = 0;
	int32_t tgtRadiusPos = 0;

	// calculate target theta position from carousel column (tube) position
	bool posOk = getCarouselPositionFromRowCol(tgtCol, tgtThetaPos);
	if (posOk == false)
	{
		canMoveCarousel = false;
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("moveCarouselToPositionAsync : Invalid Carousel position! tgtCol <%d>") % tgtCol));
	}

	if (!canMoveCarousel)
	{
		cb(false);
		Logger::L().Log (MODULENAME, severity_level::debug2, "moveCarouselToPositionAsync : <Exit> canMoveCarousel");
		return;
	}

	// theta/radius motors actual home position is different from carousel theta/radius motors home positions.
	// adjust the target theta/radius positions w.r.t. carousel home positions
	tgtThetaPos += homeThetaPos;
	tgtRadiusPos += homeRadiusPos;

	// get current theta and radius positions
	int32_t currentTPos = 0;
	int32_t currentRPos = 0;
	getThetaRadiusPosition(currentTPos, currentRPos);

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);

	// Adjust theta position to prevent carousel counter clockwise movement
	int32_t tgtTPos = tgtThetaPos;
	this->adjustCarouselThetaPos(currentTPos, tgtThetaPos);
	this->adjustCarouselRadiusPos(tgtRadiusPos);

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("moveCarouselToPositionAsync : target: col <%d>  (pos:  current: <%d>  target: <%d:(%d)>)") % pos.getColumn() % currentTPos % tgtThetaPos % tgtTPos));

	int32_t lastCol = ( prevCol.get_value_or(pos.getColumn()) );

	if (lastCol == MaxCarouselTubes && tgtCol == 1)
	{
		lastCol = 0;	// allow normal wrap of tube numbers
	}

	// check against maximum expected single tube advance forward movement
	if ((abs(lastCol - (int) tgtCol) > 1) || (abs(tgtThetaPos - currentTPos) > (unitsPerTube + tubeCalcTolerance + thetaDeadband)))
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "moveCarouselToPositionAsync : unexpected movement extent");
	}

	prevCol = pos.getColumn();

	// Set carousel stage controller target theta properties
	auto thetaProperties = properties_[currentType].thetaProperties;
	MoveStageParams thetaParams = MoveStageParams([ this, tgtThetaPos](uint16_t retriesDone) -> bool
	{
		return canDoRetryForCarouselMovement(retriesDone, false, tgtThetaPos);
	}, tgtThetaPos, (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);				// use deadband

	// Set carousel stage controller target radius properties
	auto radiusProperties = properties_[currentType].radiusProperties;
	MoveStageParams radiusParams = MoveStageParams([ this, tgtRadiusPos](uint16_t retriesDone) -> bool
	{
		return canDoRetryForCarouselMovement(retriesDone, true, tgtRadiusPos);
	}, tgtRadiusPos, (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);			// use deadband

	// Move stage to target position asynchronously
	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams ](auto ex)
	{
		moveStageToPositionAsync(ex, thetaParams, radiusParams, false);
	}, StageControllerOperationSequence::sc_entry_point, cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveCarouselToPositionAsync : <Exit>");
}

/// <summary>
/// Calculates the carousel location from current theta/radius motors positions
/// </summary>
void StageController::getCarouselPosition(SamplePositionDLL& pos)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "getCarouselPosition : <enter>");

	if (getCurrentType() !=eCarrierType::eCarousel)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "getCarouselPosition : <exit> Invalid controller type!");
		pos = SamplePositionDLL();
		return;
	}

	// Get current theta/radius motors positions
	int32_t thetaPos = GetThetaPosition();
	int32_t radiusPos = GetRadiusPosition();

	// Get the carousel controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;
	getCarouselHomePosition(homeThetaPos, homeRadiusPos);

	// If radius motor is not at carousel radius home position then carousel location is invalid
	if (!RadiusAtPosition(radiusPos, homeRadiusPos, 0, true))
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "getCarouselPosition : <exit> radius not at position : " + std::to_string(radiusPos) + "  expected : " + std::to_string(homeRadiusPos));
		pos = SamplePositionDLL();
		return;
	}

	// Find the approximate carousel position from current theta and radius position
	uint32_t tNum = getApproxCarouselTubeNumber(thetaPos, homeThetaPos);

	// Calculate target carousel theta position from calculated tube number
	int32_t tgtThetaPos = 0;
	if (!getCarouselPositionFromRowCol(tNum, tgtThetaPos))
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "getCarouselPosition : <exit> failed to get tube number from position");
		pos = SamplePositionDLL();
		return;
	}
	tgtThetaPos += homeThetaPos;

	// If calculated theta position and input theta positions are different then carousel location is invalid
	if (!ThetaAtPosition(thetaPos, tgtThetaPos, 0, true))
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "getCarouselPosition : <exit> theta not at tube position : " + std::to_string(thetaPos) + "  target : " + std::to_string(tgtThetaPos));
		pos = SamplePositionDLL();
		return;
	}

	pos = SamplePositionDLL(eStageRows::CarouselRow, static_cast<uint8_t>(tNum));
	Logger::L().Log (MODULENAME, severity_level::debug2, "getCarouselPosition : <exit>");
}

/// <summary>
/// Read the Signal status and check for tube present bit set
/// </summary>
/// This will wait for controller board to get updated with latest signal status.
/// <param name="cb">Callback to indicate caller</param>
void StageController::checkTubePresent_DelayedAsync(std::function<void(bool)> cb)
{
	assert(cb);

	auto pTimer = std::make_shared<boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());

	auto onTimer = [ this, cb, pTimer ](boost::system::error_code ec) -> void
	{
		if (!ec)
			cb(this->checkTubePresent());
	};

	pTimer->expires_from_now(boost::posix_time::milliseconds(ControllerStatusUpdateInterval + 20));
	pTimer->async_wait(onTimer);
}

/// <summary>
/// Calculates the approximate carousel tube number from input theta position/// 
/// </summary>
/// <param name="thetaPos">The theta motor position</param>
/// <param name="homeThetaPos">The carousel theta motor home position</param>
/// <returns>The carousel tube number</returns>
uint32_t StageController::getApproxCarouselTubeNumber(int32_t thetaPos, int32_t homeThetaPos) const
{
	int32_t thetaPosFromCarouselHome = thetaPos - homeThetaPos;

	normalizeThetaPosition(thetaPosFromCarouselHome, maxCarouselPos);

	// allow a little overshoot, and a moderate amount of undershoot
	double pos = (double) thetaPosFromCarouselHome + tubeCalcTolerance;

	double_t thetaAngle = (pos / 10);
	int32_t tNum = (int32_t) (thetaAngle / degreesPerTube) % maxTubes;    // get the index to allow modulo

	tNum++; // Adjust for 0 based index

	return tNum;
}

void StageController::carouselMoveTheta(bool failonTubeDetect,
                                        ThetaMotionTubeDetectStates currentState,
                                        std::function<void(std::function<void(bool)>)> thetaMoveWork,
                                        std::function<void(bool /*status*/, bool /*tube_found*/)> onWorkComplete,
                                        boost::optional<uint32_t> thetaMotorMaxSpeed)
{
	HAWKEYE_ASSERT (MODULENAME, thetaMoveWork);
	HAWKEYE_ASSERT (MODULENAME, onWorkComplete);

	auto onCurrentStateComplete = [ this, thetaMoveWork, onWorkComplete, thetaMotorMaxSpeed, failonTubeDetect ](ThetaMotionTubeDetectStates nextState)
	{
		pCBOService_->enqueueInternal(
			std::bind(&StageController::carouselMoveTheta, this, failonTubeDetect, nextState, thetaMoveWork, onWorkComplete, thetaMotorMaxSpeed));
	};

	Logger::L().Log (MODULENAME, severity_level::debug2, "carouselMoveTheta : Current state is : " + DataConversion::enumToRawValueString(currentState));

	switch (currentState)
	{
		case ThetaMotionTubeDetectStates::eEntryPoint:
		{
			eCarrierType currentType = getCurrentType();
			if (currentType !=eCarrierType::eCarousel)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : eEntryPoint : Invalid controller type!");
				onCurrentStateComplete(ThetaMotionTubeDetectStates::eThetaMotionError);
				return;
			}

			if (failonTubeDetect && checkTubePresent())
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "carouselMoveTheta : eEntryPoint : Tube found even before theta motion started");
				onCurrentStateComplete(ThetaMotionTubeDetectStates::eTubeFoundError);
				return;
			}

			onCurrentStateComplete(ThetaMotionTubeDetectStates::eSetThetaMotorSpeed);
			return;
		}
		case ThetaMotionTubeDetectStates::eSetThetaMotorSpeed:
		{
			if (thetaMotorMaxSpeed)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "carouselMoveTheta : eSetThetaMotorSpeed : Updating theta motor max speed to : " + std::to_string(thetaMotorMaxSpeed.get()));
				std::function<void(bool)> onSetMaxSpeedComplete = [this, onCurrentStateComplete, maxSpd = thetaMotorMaxSpeed.get()](bool status)
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : eSetThetaMotorSpeed : Failed to set theta motor max speed to : " + std::to_string(maxSpd));
						onCurrentStateComplete(ThetaMotionTubeDetectStates::eThetaMotionError);
						return;
					}
					onCurrentStateComplete(ThetaMotionTubeDetectStates::eStartTubeDetectTimer);
				};
				thetaMotor->SetMaxSpeed(thetaMotorMaxSpeed.get(), true, onSetMaxSpeedComplete);
				return;
			}

			onCurrentStateComplete(ThetaMotionTubeDetectStates::eStartTubeDetectTimer);
			return;
		}
		case ThetaMotionTubeDetectStates::eStartTubeDetectTimer:
		{
			// Skip ahead to starting stage motion if tube detection is not required
			if (!failonTubeDetect)
			{
				onCurrentStateComplete(ThetaMotionTubeDetectStates::eStartThetaMotion);
				return;
			}
			
			// If tube detection is required, start a timer which will check tube sensort status every 25 milliseconds

			// Lambda to invoke when tube is detected
			auto onComplete = [ onCurrentStateComplete ](bool status) -> void
			{
				// stop theta motion if tube is detected
				if (status)
				{
					onCurrentStateComplete(ThetaMotionTubeDetectStates::eStopTheta);
					return;
				}

				// Don't do anything here, Theta motion callback handler will take care of it
			};

			// Lambda to check if carousel tube is present
			auto repeatLamda = [ this ]() -> bool
			{
				return !pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::CarouselTube);
			};

			const uint32_t TubeCheckTimer = 25; // 25 millisecond
			auto timerOk = asyncCommandHelper_->getTimer()->waitRepeat(
				asyncCommandHelper_->getIoServiceRef(),
				boost::posix_time::milliseconds(TubeCheckTimer),
				onComplete,
				repeatLamda,
				boost::posix_time::milliseconds(getThetaProperties().thetaFullTimeout_msec));

			if (!timerOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : Unable to run tube detection timer while homing theta");
				onCurrentStateComplete(ThetaMotionTubeDetectStates::eThetaMotionError);
				return;
			}

			onCurrentStateComplete(ThetaMotionTubeDetectStates::eStartThetaMotion);
			return;
		}
		case ThetaMotionTubeDetectStates::eStartThetaMotion:
		{
			// Move the theta motor
			thetaMoveWork([ this, onCurrentStateComplete, failonTubeDetect ](bool status)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "carouselMoveTheta : thetaMoveWork status is : " + std::string(status ? "Success" : "Failure"));

				// Cancel the tube detect timer
				asyncCommandHelper_->getTimer()->cancel();

				if (status)
				{
					onCurrentStateComplete(ThetaMotionTubeDetectStates::eCompleted);
				}
				else
				{
					// Theta motion has failed, it can be cause of either tube was found or some other issue

					// check if tube is found by either checking the tube sensor status or check if tube detect timer is stopped.
					// Since the timer will be stopped as soon as tube is being detected
					bool tubeFound = pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::CarouselTube) ||
					                 !asyncCommandHelper_->getTimer()->isRunning();

					onCurrentStateComplete((failonTubeDetect && tubeFound) ? ThetaMotionTubeDetectStates::eTubeFoundError : ThetaMotionTubeDetectStates::eThetaMotionError );
				}
			});
			return;
		}
		case ThetaMotionTubeDetectStates::eStopTheta:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "carouselMoveTheta : eStopTheta : Tube is detected, stopping theta motion");

			// create a lambda to catch the status of the theta motor stop command
			auto tubeDetectStop = [ this, onCurrentStateComplete ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : tubeDetectedStop : stop command failed!");
				}

				// Don't do anything here, theta move callback handler will be triggered anyways and that will take care of further operation
			};

			thetaMotor->Stop(tubeDetectStop, true, getThetaProperties().thetaFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting the tube -present check
			return;
		}
		case ThetaMotionTubeDetectStates::eCompleted:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "carouselMoveTheta : eCompleted : Success");
			// Intentional Fall thru
		}
		case ThetaMotionTubeDetectStates::eTubeFoundError:
		{
			if (currentState == ThetaMotionTubeDetectStates::eTubeFoundError)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "carouselMoveTheta : eTubeFoundError : Tube found when theta motor motion was active");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::motion_sampledeck_tubedetected, 
					instrument_error::motion_sample_deck_instances::carousel, 
					instrument_error::severity_level::warning));
			}
			// Intentional Fall thru
		}
		case ThetaMotionTubeDetectStates::eThetaMotionError:
		{
			bool status = true;
			bool tubeFound = currentState == ThetaMotionTubeDetectStates::eTubeFoundError;
			if (currentState == ThetaMotionTubeDetectStates::eThetaMotionError
				|| tubeFound /*tube found state is error state*/)
			{
				status = false;
				Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : eThetaMotionError : Error occurred");
			}

			if (thetaMotorMaxSpeed && thetaMotorMaxSpeed.get() == thetaMotor->GetMaxSpeed())
			{
				// Restore the speed back to original speed
				uint32_t orgMaxSpeed = getCurrentProperties().thetaMotionProfile.MaxSpeed;
				Logger::L().Log (MODULENAME, severity_level::debug1, "carouselMoveTheta : Restoring back theta motor max speed to : " + std::to_string(orgMaxSpeed));

				std::function<void(bool)> onSetSpeedComplete = [ this, status, orgMaxSpeed, tubeFound, onWorkComplete ](bool setOk)
				{
					if (!setOk)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : eSetThetaMotorSpeed : Failed to set theta motor max speed to : " + std::to_string(orgMaxSpeed));
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::motion_motor_operationlogic, 
							instrument_error::motion_motor_instances::theta, 
							instrument_error::severity_level::warning));
					}

					bool setStatus = (status && setOk);
					thetaMotor->Enable([this, setStatus, tubeFound, onWorkComplete](bool disableOk)
					{
						if (!disableOk)
						{
							Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveThetaAndDetectTube : Failed to disable holding current");
						}
						pCBOService_->enqueueInternal(onWorkComplete, setStatus, tubeFound);
					}, false, getThetaProperties().thetaFullTimeout_msec);
				};
				thetaMotor->SetMaxSpeed(orgMaxSpeed, true, onSetSpeedComplete);
			}
			else
			{
				thetaMotor->Enable([this, status, tubeFound, onWorkComplete](bool disableOk)
				{
					if (!disableOk)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveThetaAndDetectTube : Failed to disable holding current");
					}
					pCBOService_->enqueueInternal(onWorkComplete, status, tubeFound);
				}, false, getThetaProperties().thetaFullTimeout_msec);
			}
			return;
		}
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveTheta : Invalid/Unknown ThetaMotionTubeDetectStates");

			thetaMotor->Enable([this, onCurrentStateComplete](bool disableOk)
			{
				if (!disableOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "carouselMoveThetaAndDetectTube : Failed to disable holding current");
				}
			}, false, getThetaProperties().thetaFullTimeout_msec);
		}
	}

	// Unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////Plate Controller Methods////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Convert from plate well specified as row/col to polar coordinates;
// apply to the radius position and theta angle value;
// assumes the row is the x value, and the column is the y value, so quadrants are rotated by 90 degrees
//
//           1              col              12
//                          (+)
//         ------------------------------------
//  A      | .  .  .  .  .  .|.  .  .  .  .  .|
//         | .  .  .  .  .  .|.  .  .  .  .  .|
//         | .  .  .1 .  .  .|.  .  .4 .  .  .|
//         | .  .  .  .  .  .|.  .  .  .  .  .|
// row (-) ------------------------------------ (+)
//         | .  .  .  .  .  .|.  .  .  .  .  .|
//         | .  .  .  .  .  .|.  .  .  .  .  .|
//         | .  .  .2 .  .  .|.  .  .3 .  .  .|
//  H      | .  .  .  .  .  .|.  .  .  .  .  .|
//         ------------------------------------
//                          (-)
//


/// <summary>
/// Calculates plate stage controller theta/radius position from input row/column
/// </summary>
/// <param name="rowIdx">The input plate row number</param>
/// <param name="colIdx">The input plate column number</param>
/// <param name="thetaPos">The placeholder for plate theta position</param>
/// <param name="radiusPos">The placeholder for plate radius position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::CalculateRowColPos(
	const uint32_t rowIdx, const uint32_t colIdx,
	int32_t& thetaPos, int32_t& radiusPos) const
{
	bool calcOk = true;
	int32_t deltaRow = 0;
	int32_t deltaCol = 0;
	double deltaX = 0.0;
	double deltaY = 0.0;
	double xVal = 0.0;
	long double xSquared = 0;
	double yVal = 0.0;
	long double ySquared = 0.0;
	long double radiusSquared = 0.0;
	int32_t radiusOffset = 0;
	double angleRadians = 0.0;
	double angleDegrees = 0.0;
	int32_t userAngle = 0;

	deltaRow = ((MaxPlateRowNum / 2) - rowIdx);             // for calculations use the row as the x value
	deltaCol = ((MaxPlateColNum / 2) - colIdx);             // for calculations use the col as the y value

															// account for the fact that even rows and columns are offset from the x and y axes
	deltaX = (double) (deltaRow) -0.5;
	deltaY = (double) (deltaCol) -0.5;

	// NOTE: all radius-related constants are specified in user units (1/10 micron);
	// the final radius offset values do not need to be corrected to user units
	xVal = (deltaX * plateWellSpacing);
	yVal = (deltaY * plateWellSpacing);

	angleRadians = atan2(yVal, xVal);
	angleDegrees = (angleRadians * 180.0 / M_PI);             // convert to degrees

															  // correct for quadrant
	if (colIdx >= 6)
	{
		angleDegrees += 360;
	}
	userAngle = (int32_t) (angleDegrees * 10);                 // convert to user units (1/10 degree, integer value);

	xSquared = (xVal * xVal);
	ySquared = (yVal * yVal);
	radiusSquared = xSquared + ySquared;
	radiusOffset = (int32_t) sqrtl(radiusSquared);

	// This should be set if/when we home.  If it has NOT yet been set, then do it now from the configuration file values
	auto localRadiusMaxTravel = getRadiusProperties().radiusMaxTravel - getRadiusProperties().radiusOffset - getRadiusProperties().radiusCenterPos;

	if ((radiusOffset < localRadiusMaxTravel) &&
		(userAngle >= 0) && (userAngle <= 3600))
	{
		radiusPos = radiusOffset;
		thetaPos = userAngle;
	}
	else
	{
		calcOk = false;
	}

	std::string logStr = boost::str(boost::format("CalculateRowColPos: row: %d  col: %d  thetaPos: %d  radiusPos: %d") % (rowIdx + 1) % (colIdx + 1) % userAngle % radiusOffset);
	severity_level logLevel = severity_level::debug2;
	if (!calcOk)
	{
		logStr.append(boost::str(boost::format(" failed: radiusOffset: %d  radiusCurrentMaxTravel: %d") % radiusOffset % localRadiusMaxTravel));
		logLevel = severity_level::error;
	}

	Logger::L().Log (MODULENAME, logLevel, logStr);

	return calcOk;
}

/// <summary>
/// Initialize the plate controller
/// <remarks>
/// This methods reads the plate controller properties from "motor info" and "controller calibration" file
/// and save them as plateProperties <see cref="StageController::StageControllerProperties"/>
/// </remarks>
/// </summary>
bool StageController::InitPlate()
{
	// Now Load the plate properties from "ControllerCal.info" file
	// There should not be any change in default "MotorControl.info" file
	// so reset all the node pointer to "ControllerCal.info" file

	plateControllerNode.reset();
	plateControllerConfigNode.reset();

	if (!configNode)
	{
		return false;
	}

	controllersNode = configNode->get_child_optional("motor_controllers");      // look for the controllers section for individualized parameters...
	if (!controllersNode)
	{
		return false;
	}

	plateControllerNode = controllersNode->get_child_optional(PLATECONTROLLERNODENAME);     // look for this specific controller
	if (!plateControllerNode)
	{
		return false;
	}

	plateControllerConfigNode = plateControllerNode->get_child_optional("controller_config"); // look for the motor parameters sub-node...
	if (!plateControllerConfigNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "InitPlate : Error parsing configuration file \"" + configFile + "\" - \"controller_config\" section not found");
		return false;
	}

	StageControllerProperties plateProperties = ConfigPlateVariables(properties_[eCarrierType::ePlate_96]);

	properties_[eCarrierType::ePlate_96] = plateProperties;
	cachedProperties_[eCarrierType::ePlate_96] = properties_[eCarrierType::ePlate_96];
	return true;
}

/// <summary>
/// Initialize the plate controller
/// <remarks>
/// This methods reads the plate controller properties from motor info file
/// and save them as plateProperties <see cref="StageController::StageControllerProperties"/>
/// </remarks>
/// </summary>
bool StageController::InitPlateDefault()
{
	plateControllerNode.reset();
	plateMotorParamsNode.reset();
	plateControllerConfigNode.reset();

	if (!configNode)
	{
		return false;
	}

	controllersNode = configNode->get_child_optional("motor_controllers");      // look for the controllers section for individualized parameters...
	if (!controllersNode)
	{
		return false;
	}

	plateControllerNode = controllersNode->get_child_optional(PLATECONTROLLERNODENAME);     // look for this specific controller
	if (!plateControllerNode)       // look for the motor parameters sub-node...
	{
		return false;
	}

	plateMotorParamsNode = plateControllerNode->get_child_optional("motor_parameters");
	if (!plateMotorParamsNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Init Plate Default : Error parsing configuration file \"" + configFile + "\" - \"motor_parameters\" section not found");
		return false;
	}

	plateControllerConfigNode = plateControllerNode->get_child_optional("controller_config"); // look for the controller configuration parameters
	if (!plateControllerConfigNode)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Init Plate Default : Error parsing configuration file \"" + configFile + "\" - \"controller_config\" section not found");
		return false;
	}

	StageControllerProperties plateProperties = {};
	plateProperties = ConfigPlateVariables();
	plateProperties.controllerOk = false;
	properties_[eCarrierType::ePlate_96] = plateProperties;

	return true;
}

/// <summary>
/// Set the plate properties as current stage controller properties
/// <remarks>
/// This method will set the theta and radius motors motion profiles
/// Initially "apply_" will be true and it will initialize the theta/radius/probe motors
/// after that we will make the "apply_" to avoid initialization of these motors
/// It will just set the change the motor profiles for these motors 
/// <see cref="StageController::StageControllerProperties::CommonMotorRegisters"/>
/// </remarks>
/// </summary>
/// <param name="callback">Callback to return the status of the operation/param>
void StageController::selectPlateAsync(std::function<void(bool)> callback)
{
	std::vector<std::function<void(std::function<void(bool)>)>> asyncSelectPlateTaskList;
	asyncSelectPlateTaskList.clear();

	// Init Theta
	asyncSelectPlateTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectPlateAsync :  Initialize Theta");
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->thetaMotor->Init(cb, parentTree, plateMotorParamsNode, apply_);
	});

	// Init Radius
	asyncSelectPlateTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectPlateAsync :  Initialize Radius");
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->radiusMotor->Init(cb, parentTree, plateMotorParamsNode, apply_);
	});

	// Init Probe
	asyncSelectPlateTaskList.push_back([ this ](auto cb)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectPlateAsync :  Initialize Probe");
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		this->probeMotor->Init(cb, parentTree, plateMotorParamsNode, apply_);
	});

	// Update Theta/ Radius/ probe properties for Plate
	auto UpdateMotorPropertiesForPlate = [ this ]()
	{
		auto& plateProperties = properties_[eCarrierType::ePlate_96];

		/*Theta Properties*/
		thetaMotor->SetPosTolerance(plateProperties.thetaProperties.thetaPositionTolerance);
		thetaMotor->SetMotorTimeouts(plateProperties.thetaProperties.thetaStartTimeout_msec, plateProperties.thetaProperties.thetaFullTimeout_msec);

		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		/*Radius Properties*/
		radiusMotor->SetPosTolerance(plateProperties.radiusProperties.radiusPositionTolerance);
		radiusMotor->SetMotorTimeouts(plateProperties.radiusProperties.radiusStartTimeout_msec, plateProperties.radiusProperties.radiusFullTimeout_msec);

		int32_t maxTravel = radiusMotor->GetMaxTravel();             // get from motor parameters first
		if (maxTravel != 0)
		{
			plateProperties.radiusProperties.radiusMaxTravel = maxTravel;                       // initialize from motor parameters first
		}

		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		/*Probe Properties*/
		probeMotor->SetPosTolerance(plateProperties.probeProperties.probePositionTolerance);
		probeMotor->SetMotorTimeouts(plateProperties.probeProperties.probeStartTimeout_msec, plateProperties.probeProperties.probeBusyTimeout_msec);
		probeMotor->SetDefaultProbeStop(plateProperties.probeProperties.probeStopPos);

		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);

		// Update MaxThetaPosition and RadiusDeadband for Plate
		auto gearRatio = thetaMotor->GetGearRatio();
		auto unitsPerRev = thetaMotor->GetUnitsPerRev();

		radiusDeadband = radiusMotor->GetDeadband();

		if ((gearRatio) && (unitsPerRev))       // ensure non-zero values
		{
			properties_[eCarrierType::ePlate_96]
				.thetaProperties.maxThetaPos
				= (int32_t) (unitsPerRev * gearRatio);
		}

		// Get Updated Motor profile for Plate
		apply_ = false;
		plateProperties.controllerOk = true;
		platePosInited = true;
		thetaMotor->GetMotorProfileRegisters(plateProperties.thetaMotionProfile);
		radiusMotor->GetMotorProfileRegisters(plateProperties.radiusMotionProfile);
		probeMotor->GetMotorProfileRegisters(plateProperties.probeMotionProfile);
	};

	// execute the queued tasks
	asyncCommandHelper_->queueASynchronousTask([ this, callback, UpdateMotorPropertiesForPlate ](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "selectPlateAsync : Failed to Select the plate!");
		}

		UpdateMotorPropertiesForPlate();

		pCBOService_->enqueueInternal(callback, status);
	}, asyncSelectPlateTaskList, false);
}

/// <summary>
/// Sets the stage controller motion profiles to plate controller
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::setPlateMotionProfile(std::function<void(bool)> onCompletion)
{
	if (!properties_[eCarrierType::ePlate_96].controllerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setPlateMotionProfile: controller is not OK; profile not set");
		pCBOService_->enqueueInternal(onCompletion, false);
		return;
	}

	// Radius Motor completion
	auto radiusComplete = [ this, onCompletion ](bool status)->void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setPlateMotionProfile: unable to set radius motor motion profile!");
		}

		onCompletion(status);
	};

	// Theta Motor completion (will trigger radius)
	auto thetaComplete = [ this, radiusComplete, onCompletion ](bool status)->void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setPlateMotionProfile: unable to set theta motor motion profile!");
			onCompletion(false);
			return;
		}

		radiusMotor->SetMotorProfileRegisters(radiusComplete, properties_[eCarrierType::ePlate_96].radiusMotionProfile);

	};

	thetaMotor->SetMotorProfileRegisters(thetaComplete, properties_[eCarrierType::ePlate_96].thetaMotionProfile);
}

/// <summary>
/// Call this method to update plate properties <see cref="StageController::StageControllerProperties"/>
/// from motor info file
/// </summary>
/// <param name="defaultValues">The default stage controller properties</param>
/// <returns>The plate controller properties</returns>
StageController::StageControllerProperties StageController::ConfigPlateVariables(boost::optional<StageControllerProperties> defaultValues)
{
	StageControllerProperties plateProperties;
	boost::property_tree::ptree& config = plateControllerConfigNode.get();

	if (!defaultValues)
	{
		plateProperties = {};
		plateProperties.thetaProperties = ConfigThetaVariables(config, eCarrierType::ePlate_96);
		plateProperties.radiusProperties = ConfigRadiusVariables(config, eCarrierType::ePlate_96);
		plateProperties.probeProperties = ConfigProbeVariables(config, eCarrierType::ePlate_96);
	}
	else
	{
		plateProperties = properties_[eCarrierType::ePlate_96];
		plateProperties.thetaProperties = ConfigThetaVariables(config, eCarrierType::ePlate_96, plateProperties.thetaProperties);
		plateProperties.radiusProperties = ConfigRadiusVariables(config, eCarrierType::ePlate_96, plateProperties.radiusProperties);
		plateProperties.probeProperties = ConfigProbeVariables(config, eCarrierType::ePlate_96, plateProperties.probeProperties);
	}

	plateWellSpacing = config.get<int32_t>("PlateWellSpacing", plateWellSpacing);
	plateProperties.retriesCount = config.get<int32_t>("ControllerInternalRetries", plateProperties.retriesCount);

	return plateProperties;
}

/// <summary>
/// Call this method to update motor info file from
/// plate controller properties <see cref="StageController::StageControllerProperties"/>
/// </summary>
/// <returns><c>[0]</c> if success; otherwise failed</returns>
int32_t StageController::WritePlateControllerConfig()
{
	if (!updatePlateConfig)
	{
		return 0;
	}

	// Unable to save through this method if no configuration location was given!
	if (!plateControllerConfigNode)
	{
		updatePlateConfig = false;
		return -1;
	}

	// Update our pTree with the new values
	int32_t updateStatus = UpdateControllerConfig(
		eCarrierType::ePlate_96, plateControllerNode, plateControllerConfigNode);
	if (updateStatus == 0)
	{
		auto v1 = properties_[eCarrierType::ePlate_96].thetaProperties.thetaHomePosOffset;
		auto v2 = properties_[eCarrierType::ePlate_96].radiusProperties.radiusCenterPos;
		auto v3 = properties_[eCarrierType::ePlate_96].thetaProperties.thetaCalPos;
		DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();
		instCfg.PlateThetaHomeOffset = v1;
		instCfg.PlateRadiusCenterPos = v2;
		instCfg.PlateThetaCalPos = v3;
		if (!InstrumentConfig::Instance().Set())
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
	updatePlateConfig = false;

	return updateStatus;
}

/// <summary>
/// Gets the plate controller row position as integer.
/// <see cref="StageController::PlateRows"/>
/// </summary>
/// <param name="rowChar">The plate controller row position</param>
/// <returns>The plate controller row position</returns>
uint32_t StageController::GetRowOrdinal(const TCHAR rowChar)
{
	uint32_t rowNum = 0;
	switch (rowChar)
	{
		case eStageRows::PlateRowA:
		case eStageRows::PlateRowB:
		case eStageRows::PlateRowC:
		case eStageRows::PlateRowD:
		case eStageRows::PlateRowE:
		case eStageRows::PlateRowF:
		case eStageRows::PlateRowG:
		case eStageRows::PlateRowH:
			rowNum = (rowChar - eStageRows::PlateRowA) + 1;
			if (rowNum > MaxPlateRowNum)
			{
				rowNum = 0;
			}
			break;
	}
	return rowNum;
}

/// <summary>
/// Get the plate stage controller home position
/// </summary>
/// <remarks>
/// This plate theta and radius home position is different from actual theta/radius home position (i.e. "0")
/// </remarks>
/// <param name="thetaPos">The home theta position</param>
/// <param name="radiusPos">The home radius position</param>
void StageController::getPlateHomePosition(int32_t& thetaPos, int32_t& radiusPos)
{
	auto thetaProperties = properties_[eCarrierType::ePlate_96].thetaProperties;
	int32_t tgtThetaPos = thetaProperties.thetaHomePosOffset;
	thetaPos = tgtThetaPos;

	auto radiusProperties = properties_[eCarrierType::ePlate_96].radiusProperties;
	int32_t tgtRadiusPos = radiusProperties.radiusOffset + radiusProperties.radiusCenterPos;
	radiusPos = tgtRadiusPos;
}

/// <summary>
/// Gets the plate controller theta/radius position from input row/columns values
/// </summary>
/// <param name="tgtRow">The input plate row number</param>
/// <param name="tgtCol"The input plate columns number></param>
/// <param name="thetaPos">The placeholder for plate theta motor position</param>
/// <param name="radiusPos">The placeholder for plate radius motor position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::getPlatePositionFromRowCol(uint32_t tgtRow, uint32_t tgtCol, int32_t& thetaPos, int32_t& radiusPos) const
{
	bool calcOk = false;

	std::string logStr = "getPlatePositionFromRowCol(u,u):";
	std::string logStr2 = boost::str(boost::format(" tgtRow: %d, tgtCol: %d") % tgtRow % tgtCol);
	std::string errStr = "";

	thetaPos = 0;
	radiusPos = 0;

	if (((tgtRow > 0) && (tgtRow <= MaxPlateRowNum)) &&
		((tgtCol > 0) && (tgtCol <= MaxPlateColNum)))
	{
		uint32_t row = tgtRow;
		uint32_t col = tgtCol;

		// TODO:  calculate the position to pass to the theta and radius motors to actually move to a row/col pair
		calcOk = CalculateRowColPos(row - 1, col - 1, thetaPos, radiusPos);
		if (!calcOk)
		{
			errStr = boost::str(boost::format(" : ERROR: row-col position calculation reports illegal values: tPos: %d  rPos: %d") % thetaPos % radiusPos);
		}
	}
	else
	{
		if (((tgtRow <= 0) || (tgtRow > MaxPlateRowNum)) &&
			((tgtCol <= 0) || (tgtCol > MaxPlateColNum)))
		{
			errStr = boost::str(boost::format(" : Illegal row-col values: row: %d  col: %d") % tgtRow % tgtCol);
		}
		else if ((tgtRow <= 0) || (tgtRow > MaxPlateRowNum))
		{
			errStr = boost::str(boost::format(" : Not a legal row value: %d") % tgtRow);
		}
		else
		{
			errStr = boost::str(boost::format(" : Not a legal col value: %d") % tgtCol);
		}
	}

	if (!calcOk)
	{
		severity_level logLevel = severity_level::normal;

		if ((Logger::L().IsOfInterest(severity_level::debug2)) && (errStr.length() > 0))
		{
			logStr.append(logStr2);
			logStr.append(errStr);
			logLevel = severity_level::debug2;
		}

		Logger::L().Log (MODULENAME, logLevel, logStr);
	}

	return calcOk;
}

/// <summary>
/// Gets the plate controller row/columns values from input theta/radius position
/// </summary>
/// <param name="tgtThetaPos">The theta motor position</param>
/// <param name="tgtRadiusPos">The radius motor position</param>
/// <param name="row">The placeholder for plate row number</param>
/// <param name="col">The placeholder for plate column number</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::getRowColFromPlatePosition(int32_t tgtThetaPos, int32_t tgtRadiusPos, uint32_t& row, uint32_t& col) const
{
	bool calcOk = false;
	double angleDegrees = 0.0;
	double angleRadians = 0;
	double cosTheta = 0.0;
	int64_t radius = 0;
	long double radiusSquared = 0.0;
	double xVal = 0.0;
	long double xSquared = 0;
	double yVal = 0.0;
	long double ySquared = 0.0;
	double deltaX = 0.0;
	double deltaY = 0.0;
	int32_t deltaRowIdx = 0;
	int32_t deltaColIdx = 0;
	int32_t calcRow = 0;
	int32_t calcCol = 0;

	Logger::L().Log (MODULENAME, severity_level::debug2, "getRowColfromPlatePosition: <enter>");


	if ((tgtThetaPos != 0) && (tgtRadiusPos != 0))
	{
		calcOk = true;
		normalizeThetaPosition(tgtThetaPos, maxCarouselPos);		// ensure a positive value in the expected bounds

		angleDegrees = ((double) tgtThetaPos / 10.0);				// convert from user units (1/10 degree, integer value) to degrees;
		angleRadians = (angleDegrees * M_PI) / 180.0;				// convert to radians
		cosTheta = cos(angleRadians);

		radius = tgtRadiusPos;										// up-convert current position to 64 bit
		radiusSquared = ((double) radius * (double) radius);

		xVal = cosTheta * radius;
		xSquared = (xVal * xVal);
		ySquared = (radiusSquared - xSquared);
		yVal = sqrtl(ySquared);

		if (angleDegrees > 180)
		{
			yVal = -(yVal);
		}

		deltaX = (xVal / plateWellSpacing);
		deltaY = (yVal / plateWellSpacing);

		deltaRowIdx = (int32_t) round(deltaX + 0.5);
		deltaColIdx = (int32_t) round(deltaY + 0.5);

		calcRow = ((MaxPlateRowNum / 2) - deltaRowIdx) + 1;
		calcCol = ((MaxPlateColNum / 2) - deltaColIdx) + 1;

		// set return flag
		if ((calcRow <= 0) || (calcRow > MaxPlateRowNum) ||
			(calcCol <= 0) || (calcCol > MaxPlateColNum))
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "getRowColfromPlatePosition: calculated row/col out of bounds: row: " + std::to_string(calcRow) + "  col: " + std::to_string(calcCol));
			calcOk = false;
			// set returned row and col value... may be 0 if not valid input
			calcRow = 0;
			calcCol = 0;
		}
	}
	else
	{
		if (Logger::L().IsOfInterest(severity_level::debug2))
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "getRowColfromPlatePosition: invalid radius/theta: radius: " + std::to_string(tgtRadiusPos) + "  theta: " + std::to_string(tgtThetaPos));
		}
	}

	// set returned row and col value... may be 0 if not valid input
	row = calcRow;
	col = calcCol;


	Logger::L().Log (MODULENAME, severity_level::debug2, "getRowColfromPlatePosition: <exit>");

	return calcOk;
}

/// <summary>
/// Adjust the plate radius position
/// </summary>
/// <remarks>Radius home is different then plate radius home</remarks>
/// <param name="currentRadiusPos">The current radius motor position</param>
/// <param name="radiusPos">The placeholder for calculated plate radius home position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::adjustPlateRadiusPos(const int32_t& currentRadiusPos, int32_t& radiusPos)
{
	// adjust the target positions to compensate for backlash
	int32_t rChgDir = 0;
	int32_t rPos = currentRadiusPos;
	int32_t radiusChg = radiusPos - rPos;

	// don't act unless target position will require a move...
	if (abs(radiusChg) > radiusDeadband)
	{
		if (radiusPos < rPos)
		{
			rChgDir = -1;
		}
		else if (radiusPos > rPos)
		{
			rChgDir = 1;
		}
	}

	// only act on real direction changes, not transitions resulting in a no-direction move
	if (rChgDir != 0)
	{
		// new move is real and in a different direction from the last
		if (rChgDir != lastRadiusDir)
		{
			// new move is in positive direction and not the first move
			if ((rChgDir > 0) && (lastRadiusDir != 0))
			{
				// initialize the compensation value
				radiusBacklashCompensation = (rChgDir * properties_[eCarrierType::ePlate_96].radiusProperties.radiusBacklash);
			}
			else
			{
				// zero out the applied compensation after a transition to negative or a 0 positional change
				radiusBacklashCompensation = 0;
			}
		}
		lastRadiusDir = rChgDir;
	}

	radiusPos += radiusBacklashCompensation;
	return true;
}

/// <summary>
/// Adjust the plate theta position
/// </summary>
/// <remarks>Theta home is different then plate radius home</remarks>
/// <param name="currentThetaPos">The current theta motor position</param>
/// <param name="thetaPos">The placeholder for calculated plate theta home position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
bool StageController::adjustPlateThetaPos(const int32_t& currentThetaPos, int32_t& thetaPos)
{
	// calculate change from normalized position values...
	int32_t tPos = currentThetaPos;
	int32_t thetaChg = thetaPos - tPos;
	int32_t absChg = abs(thetaChg);
	int32_t shortestChg = 0;
	int32_t maxThetaPos = properties_[eCarrierType::ePlate_96].thetaProperties.maxThetaPos;
	int32_t rawThetaPos = currentThetaPos;

	// now convert to change relative to raw position to handle the
	// continual increment or decrement condition possible with theta encoder..
	if (absChg > (maxThetaPos / 2))
	{
		if (thetaChg < 0)
		{
			// generate opposite sign move value
			shortestChg = thetaChg + maxThetaPos;
		}
		else
		{
			// generate opposite sign move value
			shortestChg = thetaChg - maxThetaPos;
		}
		thetaPos = rawThetaPos + shortestChg;
	}
	else if (absChg == (maxThetaPos / 2))
	{
		shortestChg = (maxThetaPos / 2);
		if (rawThetaPos > 0)
		{
			// force decrementing direction to keep values from continual increase
			shortestChg = -(shortestChg);
		}
		thetaPos = rawThetaPos + shortestChg;
	}
	else
	{
		thetaPos = rawThetaPos + thetaChg;
	}

	return true;
}

/// <summary>
/// Move plate stage controller to home position asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::doPlateHomeAsync(std::function<void(bool)> cb)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doPlateHomeAsync : <Enter>");

	MoveClear();

	eCarrierType currentType = getCurrentType();
	if (currentType != eCarrierType::ePlate_96)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "doPlateHomeAsync : Invalid controller type!");
		cb(false);
		return;
	}

	// Get the plate controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t tgtThetaPos = 0;
	int32_t tgtRadiusPos = 0;
	getPlateHomePosition(tgtThetaPos, tgtRadiusPos);

	// Set plate stage controller target theta properties
	auto thetaProperties = properties_[currentType].thetaProperties;
	MoveStageParams thetaParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForPlateMovement(retriesDone);
	}, tgtThetaPos, (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);

	// Set plate stage controller target radius properties
	auto radiusProperties = properties_[currentType].radiusProperties;
	MoveStageParams radiusParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForPlateMovement(retriesDone);
	}, tgtRadiusPos, (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);

	// Set the radius max travel
	//	radiusCurrentMaxTravel = radiusProperties.radiusMaxTravel - radiusProperties.radiusOffset - radiusProperties.radiusCenterPos;

	// Move stage to target position asynchronously
	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams ](auto ex)
	{
		moveStageToPositionAsync(ex, thetaParams, radiusParams, false);
	}, StageControllerOperationSequence::sc_entry_point, cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "doPlateHomeAsync : <Exit>");
}

/// <summary>
/// Call this method to check if plate stage movement retries are available for target position
/// If plate stage is not able to move to target position in multiple attempts
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
/// <param name="retriesDone">The number of retries already done for target position</param>
bool StageController::canDoRetryForPlateMovement(uint16_t retriesDone)
{
	// If we have already exhausted the maximum retries allowed then no need to go further
	return (retriesDone < getCurrentProperties().retriesCount + 1); // Plus 1 to compensate for (original run + retries)
}

/// <summary>
/// Move the plate stage to target position
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
/// <param name="pos">Target plate stage row:col position</param>
void StageController::movePlateToPositionAsync(std::function<void(bool)> cb, SamplePositionDLL pos)
{
	assert(cb);

	MoveClear();

	Logger::L().Log (MODULENAME, severity_level::debug2, "movePlateToPositionAsync : <Enter>");
	bool canMovePlate = true;
	eCarrierType currentType = getCurrentType();
	if (currentType != eCarrierType::ePlate_96)
	{
		canMovePlate = false;
		Logger::L().Log (MODULENAME, severity_level::error, "movePlateToPositionAsync : Invalid controller type!");
	}
	if (pos.isValidForPlate96() == false)
	{
		canMovePlate = false;
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("movePlateToPositionAsync : Invalid plate position! row <%c>, col <%d>") % pos.getRow() % pos.getColumn()));
	}

	uint32_t tgtRow = GetRowOrdinal(pos.getRow());
	uint32_t tgtCol = pos.getColumn();
	int32_t tgtThetaPos = 0;
	int32_t tgtRadiusPos = 0;

	bool posOk = getPlatePositionFromRowCol(tgtRow, tgtCol, tgtThetaPos, tgtRadiusPos);
	if (posOk == false)
	{
		canMovePlate = false;
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("movePlateToPositionAsync : Invalid plate position! tgtRow <%d>, tgtCol <%d>") % tgtRow % tgtCol));
	}

	if (!canMovePlate)
	{
		cb(false);
		return;
	}
	movePlateToRawPosAsync(cb, tgtThetaPos, tgtRadiusPos);
}

/// <summary>
/// Move the plate stage to a raw r-theta position target
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
/// <param name="pos">Target r-theta stage position</param>
void StageController::movePlateToRawPosAsync(std::function<void(bool)> cb, int32_t tgtThetaPos, int32_t tgtRadiusPos)
{
	assert(cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "movePlateToRawPosAsync : <Enter>");

	MoveClear();

	eCarrierType currentType = getCurrentType();
	if (currentType != eCarrierType::ePlate_96)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "movePlateToRawPosAsync : Invalid controller type!");
		cb(false);
		return;
	}

	// Get the plate controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;
	getPlateHomePosition(homeThetaPos, homeRadiusPos);

	// theta/radius motors actual home position is different from plate theta/radius motors home positions.
	// adjust the target theta/radius positions w.r.t. plate home positions
	tgtThetaPos += homeThetaPos;
	tgtRadiusPos += homeRadiusPos;

	// get current theta and radius positions

	int32_t currentTPos = 0;
	int32_t currentRPos = 0;
	getThetaRadiusPosition(currentTPos, currentRPos);

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);

	int32_t tPos = tgtThetaPos;
	int32_t rPos = tgtRadiusPos;

	adjustPlateThetaPos(currentTPos, tPos);
	adjustPlateRadiusPos(currentRPos, rPos);

	// Set plate stage controller target theta properties
	auto thetaProperties = properties_[currentType].thetaProperties;
	MoveStageParams thetaParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForPlateMovement(retriesDone);
	}, tPos, (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);			// use deadband for radius

	// Set plate stage controller target radius properties
	auto radiusProperties = properties_[currentType].radiusProperties;
	MoveStageParams radiusParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
	{
		return canDoRetryForPlateMovement(retriesDone);
	}, rPos, (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);		// use deadband for radius

	// Move stage to target position asynchronously
	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams ](auto ex)
	{
		moveStageToPositionAsync(ex, thetaParams, radiusParams, false);
	}, StageControllerOperationSequence::sc_entry_point, cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "movePlateToRawPosAsync : <Exit>");
}

/// <summary>
/// Calculates the plate location from current theta/radius motors positions
/// </summary>
/// <param name="pos">Row/Col sample position</param>
void StageController::getPlatePosition(SamplePositionDLL& pos)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "getPlatePosition : <enter>");
	if (getCurrentType() != eCarrierType::ePlate_96)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getPlatePosition : Invalid controller type!");
		pos = SamplePositionDLL();
		return;
	}

	// Get current theta/radius motors positions
	int32_t thetaPos = GetThetaPosition();
	int32_t radiusPos = GetRadiusPosition();

	// Get the plate controller theta and radius home position w.r.t. actual theta/radius home position (i.e. "0")
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;
	getPlateHomePosition(homeThetaPos, homeRadiusPos);

	// Adjust current theta/radius position w.r.t. plate home positions
	int32_t relativeThetaPos = thetaPos - homeThetaPos;
	int32_t relativeRadiusPos = radiusPos - homeRadiusPos;

	normalizeThetaPosition(relativeThetaPos, maxCarouselPos);

	SamplePositionDLL position = SamplePositionDLL();
	uint32_t row = 0;
	uint32_t col = 0;
	if (!getRowColFromPlatePosition(relativeThetaPos, relativeRadiusPos, row, col))
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "getPlatePosition : stage position does not correspond to a row / col location!");
		pos = position;
		return;
	}

	char rowChar = row + (eStageRows::PlateRowA - 1);
	pos.setRowColumn(rowChar, static_cast<uint8_t>(col));

	Logger::L().Log (MODULENAME, severity_level::debug2, "getPlatePosition : <exit>");
}

void StageController::findPlateTop(std::function<void(bool)> cb, PlateDetectState state, bool stateSuccess, bool plateFound, int32_t stopPos)
{
	HAWKEYE_ASSERT (MODULENAME, cb);

	Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str(boost::format("findPlateTop : <enter> State: %d, status: %s") % (int)state % (stateSuccess ? "success" : "fail")));

	if (cancelMove_)
	{
		if (state != pdsCancelComplete)
		{
			state = pdsCancelStart;
		}
		cancelThetaMove_ = true;
		cancelRadiusMove_ = true;
	}

	switch (state)
	{
		case pdsValidityCheck:
		{
			eCarrierType currentType = getCurrentType();
			if (currentType != eCarrierType::ePlate_96)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed; Invalid controller type!");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}
			// INTENTIONAL FALLTHROUGH
		}
		case pdsCalculations:
		{
			// Calculate the position of the point at 90 degrees clockwise rotation ( half-way between
			// row 'D' and 'E'), and slightly more than 1/2 well separation spacing beyond column 12 to
			// find a location with a flat surface that should always be present on every plate
			int32_t tgtRow = GetRowOrdinal('D');
			int32_t tgtCol = 12;
			int32_t tPos1 = 0;
			int32_t tPos2 = 0;
			int32_t rPos = 0;
			int32_t plateProbePos = 0;

			// getPlatePositionFromRowCol returns the 0-center based position;
			// value is corrected in movePlateToRawPositionAsync
			if (!getPlatePositionFromRowCol(tgtRow, tgtCol, tPos1, rPos))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed getting test position1");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			tgtRow++;
			if (!getPlatePositionFromRowCol(tgtRow, tgtCol, tPos2, rPos))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed getting test position2");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			pCBOService_->enqueueInternal([ = ]()
			{
				findPlateTop(cb, pdsProbeUp1, stateSuccess, plateFound, stopPos);
			});
			break;
		}
		case pdsProbeUp1:
		{
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				[ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsOnProbeUp1Complete, success, plateFound, stopPos);
			});
			break;
		}
		case pdsOnProbeUp1Complete:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed moving probe up");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}
			// INTENTIONAL FALL-THROUGH
		}
		case pdsMovePlateToDetectLocation:
		{
			int32_t tgtRow = GetRowOrdinal('D');
			int32_t tgtCol = 12;
			int32_t tPos1 = 0;
			int32_t tPos2 = 0;
			int32_t rPos = 0;
			int32_t plateProbePos = 0;

			// getPlatePositionFromRowCol returns the 0-center based position;
			// value is corrected in movePlateToRawPositionAsync
			if (!getPlatePositionFromRowCol(tgtRow, tgtCol, tPos1, rPos))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed getting test position1");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			tgtRow++;
			if (!getPlatePositionFromRowCol(tgtRow, tgtCol, tPos2, rPos))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed getting test position2");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			int32_t tPos = (tPos1 + tPos2) / 2;
			rPos += ((plateWellSpacing * 3) / 4);							// estimate a position just outside of row 12

																			// move the plate to the test position.
																			// This method will block until completion
			movePlateToRawPosAsync([ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsOnMoveToDetectComplete, success, plateFound, stopPos);
			}, tPos, rPos);

			break;
		}
		case pdsOnMoveToDetectComplete:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed moving plate to test position");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			// INTENTIONAL FALLTHROUGH
		}
		case pdsDisableProbe:
		{
			// disable holding current to prevent probe position change due to stepping
			probeMotor->Enable([ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsProbeDown, success, plateFound, stopPos);
			}, false, 10);
			break;
		}
		case pdsProbeDown:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed to disable holding current on probe");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			// bypass the plate position check
			bool downOnInvalidStagePos = true;

			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeDownAsync, this, std::placeholders::_1, downOnInvalidStagePos, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				[ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsOnProbeDownComplete, success, plateFound, stopPos);
			});
			break;
		}
		case pdsOnProbeDownComplete:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed to drive probe down");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			// INTENTIONAL FALLTHROUGH
		}
		case pdsCheckPlateDetection:
		{
			stopPos = probeMotor->GetProbeStop();
			int32_t maxPos = properties_[eCarrierType::ePlate_96].probeProperties.probeMaxTravel;
			plateFound = (stopPos > 0 && stopPos < (maxPos - ProbeChange_10_0_mm));

			// INTENTIONAL FALLTHROUGH
		}
		case pdsEnableProbe2:
		{
			// re-enable holding current for normal operations
			probeMotor->Enable([ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsProbeUp2, success, plateFound, stopPos);
			}, true, 1);
			break;
		}
		case pdsProbeUp2:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed to enable probe holding current after detection");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}

			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				[ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsOnProbeUp2Complete, success, plateFound, stopPos);
			});
			break;
		}
		case pdsOnProbeUp2Complete:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed to raise probe after detection");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}
			// INTENTIONAL FALLTHROUGH
		}
		case pdsHomePlate:
		{
			doPlateHomeAsync([ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsOnHomePlateComplete, success, plateFound, stopPos);
			});
			break;
		}
		case pdsCancelStart:
		{
			// ensure the probe is raised on cancel
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				[ this, cb, plateFound, stopPos ](bool success)->void
			{
				findPlateTop(cb, pdsCancelComplete, success, plateFound, stopPos);
			});
			break;
		}
		case pdsCancelComplete:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : <exit> : operation cancelled");
			pCBOService_->enqueueInternal(cb, plateFound);	// The detection process may have already completed, so use whatever plateFound state is given
			break;
		}
		case pdsOnHomePlateComplete:
		{
			if (!stateSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "findPlateTop : failed restoring plate position");
				pCBOService_->enqueueInternal(cb, false);
				return;
			}
			// INTENTIONAL FALLTHROUGH
		}
		case pdsReport:
		{
			std::string logStr = "findPlateTop : <exit> : plate top";
			if (plateFound)
			{
				logStr.append(boost::str(boost::format(" found at depth %d") % stopPos));
			}
			else
			{
				logStr.append(boost::str(boost::format(" not found: stop position %d") % stopPos));
			}
			Logger::L().Log (MODULENAME, severity_level::debug1, logStr);

			pCBOService_->enqueueInternal(cb, plateFound);
			break;
		}
	}
}

void StageController::selectStageInternal(std::function<void(bool)> callback, SelectStageStates state)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [ this, callback ](bool status, SelectStageStates nextState)
	{
		if (!status)
		{
			nextState = SelectStageStates::eError;
		}

		// Execute next state of SelectStage
		pCBOService_->enqueueInternal([ = ]()
		{
			selectStageInternal(callback, nextState);
		});
	};

	switch (state)
	{
		case SelectStageStates::eEntryPoint:
		{
			// Set the busy status to true for each state.
			stageControllerInitBusy_ = true;
			pCBOService_->enqueueInternal(std::bind(onCurrentStateComplete, true, SelectStageStates::eStopStage));
			return;
		}

		case SelectStageStates::eStopStage:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <State - eStopStage>");

			// If stage is not yet initialized then no need to Stop/Clear
			if (stageControllerInitialized_)
			{
				// call "getThetaPosition_DelayAsync" to provide delay to get latest values
				getThetaPosition_DelayAsync([ this, onCurrentStateComplete ](int32_t)
				{
					auto& bStatus = pCBOService_->CBO()->GetBoardStatus();
					bool anyMotorBusy = bStatus.isSet(BoardStatus::ProbeMotorBusy) ||
										bStatus.isSet(BoardStatus::RadiusMotorBusy) ||
										bStatus.isSet(BoardStatus::ThetaMotorBusy);

					if (anyMotorBusy)
					{
						stopAsync(std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eSetMotionProfile));
					}
					else
					{
						onCurrentStateComplete(true, SelectStageStates::eSetMotionProfile);
					}
				});
			}
			else
			{
				onCurrentStateComplete(true, SelectStageStates::eSetMotionProfile);
			}
			return;
		}

		case SelectStageStates::eSetMotionProfile:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <State - eSetMotionProfile>");

			lastCarouselPresentStatus_ = pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::SignalStatusBits::CarouselPresent);
			Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("selectStageInternal : sensor status: %s") % (lastCarouselPresentStatus_ ? "Carousel" : "Plate")));

			if (stageControllerInitialized_)
			{
				// If stage controller is already initialized then set the current stage controller motion profiles
				// Do not home the motors again
				setMotionProfileInternalIntializedStage(
					lastCarouselPresentStatus_.load(),
					std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eCompleted),
					sm_SetMotionProfileInitialized::smpi_ClearState);
			}
			else
			{
				setMotionProfileInternalUnInitializedStage(
					lastCarouselPresentStatus_.load(),
					std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eInitializeAndHomeMotors),
					sm_SetMotionProfileUninitialized::smpu_ClearState);
			}
			return;
		}

		case SelectStageStates::eInitializeAndHomeMotors:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <State - eInitializeAndHomeMotors>");
			initializeAndHomeMotorsAsync(
				std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eHomeStage),
				InitializeAndHomeStates::eInitializeProbe);
			return;
		}

		case SelectStageStates::eHomeStage:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <State - eHomeStage>");
			homeStageAsync(std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eEjectStage));
			return;
		}

		case SelectStageStates::eEjectStage:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <State - eEjectStage>");
			ejectStageAsync(std::bind(onCurrentStateComplete, std::placeholders::_1, SelectStageStates::eCompleted));
			return;
		}

		case SelectStageStates::eCompleted:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageInternal : <Stage Selection completed>");
			stageControllerInitBusy_ = false;
			stageControllerInitialized_ = true;
			pCBOService_->enqueueInternal(callback, true);

			MoveClear();
			return;
		}

		case SelectStageStates::eError:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "selectStageInternal : <Failed to complete stage selection>");
			stageControllerInitBusy_ = false;
			stageControllerInitialized_ = false;
			pCBOService_->enqueueInternal(callback, false);

			MoveClear();
			return;
		}
	}

	//unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void StageController::setMotionProfileInternalIntializedStage(bool carousel_present, std::function<void(bool)> onComplete, sm_SetMotionProfileInitialized state, bool success)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalIntializedStage: <enter>");

	std::string stagename = carousel_present ? "Carousel" : "Plate";
	eCarrierType stagetype = carousel_present ?eCarrierType::eCarousel : eCarrierType::ePlate_96;

	switch (state)
	{
		case sm_SetMotionProfileInitialized::smpi_ClearState:
		{
			properties_[eCarrierType::eCarousel].controllerOk = false;
			properties_[eCarrierType::ePlate_96].controllerOk = false;

			//INTENTIONAL FALLTHROUGH
		}
		case sm_SetMotionProfileInitialized::smpi_SelectStage:
		{
			if (carousel_present)
			{
				selectCarouselAsync([ this, carousel_present, onComplete, stagename ](bool stageOK)
				{
					auto next = sm_SetMotionProfileInitialized::smpi_SelectStageComplete;
					if (!stageOK)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalIntializedStage: " + stagename + " : Failed to select stage");
						next = sm_SetMotionProfileInitialized::smpi_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, stageOK ]()
					{
						setMotionProfileInternalIntializedStage(carousel_present, onComplete, next, stageOK);
					});

				});
			}
			else
			{
				selectPlateAsync([ this, carousel_present, onComplete, stagename ](bool stageOK)
				{
					auto next = sm_SetMotionProfileInitialized::smpi_SelectStageComplete;
					if (!stageOK)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalIntializedStage: " + stagename + " : Failed to select stage");
						next = sm_SetMotionProfileInitialized::smpi_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, stageOK ]()
					{
						setMotionProfileInternalIntializedStage(carousel_present, onComplete, next, stageOK);
					});
				});
			}
			break;
		}
		case sm_SetMotionProfileInitialized::smpi_SelectStageComplete:
		{
			//INTENTIONAL FALLTHROUGH - add code when selectCarousel/selectPlate are async.
		}
		case sm_SetMotionProfileInitialized::smpi_SetMotionProfile:
		{
			auto onSetComplete = [ this, carousel_present, onComplete ](bool succeeded)
			{
				setMotionProfileInternalIntializedStage(carousel_present, onComplete, sm_SetMotionProfileInitialized::smpi_SetMotionProfileComplete, succeeded);
			};

			if (carousel_present)
				setCarouselMotionProfile(onSetComplete);
			else
				setPlateMotionProfile(onSetComplete);
			break;
		}
		case sm_SetMotionProfileInitialized::smpi_SetMotionProfileComplete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalIntializedStage: " + stagename + " : " + std::string(success ? "Success" : "Failure"));

			properties_[stagetype].controllerOk = success;

			//INTENTIONAL FALLTHROUGH
		}
		case sm_SetMotionProfileInitialized::smpi_Cleanup:
		{
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetThetaPosition(), ePositionDescription::Current);
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetRadiusPosition(), ePositionDescription::Current);
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, getProbePosition(), ePositionDescription::Current);

			stageControllerInitBusy_ = false;

			Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalIntializedStage: <completed>");

			pCBOService_->enqueueInternal(onComplete, success);
			break;
		}
	}
}

void StageController::setMotionProfileInternalUnInitializedStage(bool carousel_present, std::function<void(bool)> onComplete, sm_SetMotionProfileUninitialized state, bool success)
{
	// Do this Initially when stage is not initialized.
	Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnInitializedStage: <enter>");

	std::string firststagename = carousel_present ? "Carousel" : "Plate";
	std::string secondstagename = !carousel_present ? "Carousel" : "Plate";
	eCarrierType firststagetype = carousel_present ?eCarrierType::eCarousel : eCarrierType::ePlate_96;
	eCarrierType secondstagetype = !carousel_present ?eCarrierType::eCarousel : eCarrierType::ePlate_96;

	switch (state)
	{
		case sm_SetMotionProfileUninitialized::smpu_ClearState:
		{
			properties_[eCarrierType::eCarousel].initializedOk = false;
			properties_[eCarrierType::ePlate_96].initializedOk = false;
			//INTENTIONAL FALLTHROUGH;
		}
		case sm_SetMotionProfileUninitialized::smpu_SelectStage1:
		{
			if (carousel_present)
			{
				selectCarouselAsync([ this, carousel_present, onComplete, firststagename ](bool success)
				{
					auto next = sm_SetMotionProfileUninitialized::smpu_SelectStage1Complete;
					if (!success)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnIntializedStage: " + firststagename + " : Failed to select first stage");
						next = sm_SetMotionProfileUninitialized::smpu_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, success ]()
					{
						setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, next, success);
					});

				});
			}
			else
			{
				selectPlateAsync([ this, carousel_present, onComplete, firststagename ](bool success)
				{
					auto next = sm_SetMotionProfileUninitialized::smpu_SelectStage1Complete;
					if (!success)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnIntializedStage: " + firststagename + " : Failed to select first stage");
						next = sm_SetMotionProfileUninitialized::smpu_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, success ]()
					{
						setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, next, success);
					});
				});
			}
			break;
		}
		case sm_SetMotionProfileUninitialized::smpu_SelectStage1Complete:
		{
			//INTENTIONAL FALLTHROUGH - add code when selectCarousel/selectPlate are async.
		}
		case sm_SetMotionProfileUninitialized::smpu_SetMotionProfile1:
		{
			auto onSetComplete = [ this, carousel_present, onComplete ](bool succeeded)
			{
				setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, sm_SetMotionProfileUninitialized::smpu_SetMotionProfile1Complete, succeeded);
			};

			if (carousel_present)
				setCarouselMotionProfile(onSetComplete);
			else
				setPlateMotionProfile(onSetComplete);
			break;
		}
		case sm_SetMotionProfileUninitialized::smpu_SetMotionProfile1Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnInitializedStage: " + firststagename + " : setPMotionProfile1 " + std::string(success ? "Success" : "Failure"));

			properties_[firststagetype].controllerOk = success;

			//INTENTIONAL FALLTHROUGH
		}
		case sm_SetMotionProfileUninitialized::smpu_SelectStage2:
		{
			if (!carousel_present)
			{
				selectCarouselAsync([ this, carousel_present, onComplete, secondstagename ](bool success)
				{
					auto next = sm_SetMotionProfileUninitialized::smpu_SelectStage2Complete;
					if (!success)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnIntializedStage: " + secondstagename + " : Failed to select second stage");
						next = sm_SetMotionProfileUninitialized::smpu_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, success ]()
					{
						setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, next, success);
					});

				});
			}
			else
			{
				selectPlateAsync([ this, carousel_present, onComplete, secondstagename ](bool success)
				{
					auto next = sm_SetMotionProfileUninitialized::smpu_SelectStage2Complete;
					if (!success)
					{
						Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnIntializedStage: " + secondstagename + " : Failed to select second stage");
						next = sm_SetMotionProfileUninitialized::smpu_Cleanup;
					}

					pCBOService_->enqueueInternal([ this, carousel_present, onComplete, next, success ]()
					{
						setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, next, success);
					});
				});
			}
			break;
		}
		case sm_SetMotionProfileUninitialized::smpu_SelectStage2Complete:
		{
			//INTENTIONAL FALLTHROUGH - add code when selectCarousel/selectPlate are async.
		}
		case sm_SetMotionProfileUninitialized::smpu_SetMotionProfile2:
		{
			auto onSetComplete = [ this, carousel_present, onComplete ](bool succeeded)
			{
				setMotionProfileInternalUnInitializedStage(carousel_present, onComplete, sm_SetMotionProfileUninitialized::smpu_SetMotionProfile2Complete, succeeded);
			};

			if (!carousel_present)
				setCarouselMotionProfile(onSetComplete);
			else
				setPlateMotionProfile(onSetComplete);
			break;
		}
		case sm_SetMotionProfileUninitialized::smpu_SetMotionProfile2Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnInitializedStage: " + secondstagename + " : setPMotionProfile2 " + std::string(success ? "Success" : "Failure"));

			properties_[secondstagetype].controllerOk = success;

			//INTENTIONAL FALLTHROUGH
		}
		case sm_SetMotionProfileUninitialized::smpu_Cleanup:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setMotionProfileInternalUnInitializedStage: <exit>");

			// use this hack to disable holding current EXCEPT during Worklist processing.
			enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(onComplete, success);
			break;
		}
	}
}

/// <summary>
/// Call this method to check if a plate can be detected on the carrier
/// </summary>
/// <returns><c>[true]</c> if a plate is detected; otherwise <c>[false]</c></returns>
void StageController::isPlatePresentAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (checkCarouselPresent())
	{
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	pCBOService_->enqueueInternal([ this, callback ]()
	{
		findPlateTop(callback, PlateDetectState::pdsValidityCheck, true, false, 0);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////Stage Controller Methods////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// <summary>
/// constructor to create instance of <see cref="StageController"/>
/// </summary>
/// <param name="pCBOService">The shared instance of hardware service <see cref="CBOService"/></param>
StageController::StageController(std::shared_ptr<CBOService> pCBOService)
	: StageControllerBase(pCBOService)
{
	thetaMotor = std::make_shared<MotorBase>(pCBOService_, MotorTypeId::MotorTypeTheta);

	thetaMotor->registerToErrorReportCb([ this ](uint32_t errorCode) -> void
	{
		this->properties_[this->getCurrentType()].thetaErrorCodes.push_back(errorCode);
	});

	radiusMotor.reset(new MotorBase(pCBOService_, MotorTypeId::MotorTypeRadius));
	radiusMotor->registerToErrorReportCb([ this ](uint32_t errorCode) -> void
	{
		properties_[getCurrentType()].radiusErrorCodes.push_back(errorCode);
	});

	probeMotor.reset(new MotorBase(pCBOService_, MotorTypeId::MotorTypeProbe));
	probeMotor->registerToErrorReportCb([ this ](uint32_t errorCode) -> void
	{
		properties_[getCurrentType()].probeErrorCodes.push_back(errorCode);
	});

	properties_[eCarrierType::eCarousel] = getDefaultProperties(eCarrierType::eCarousel);
	properties_[eCarrierType::ePlate_96] = getDefaultProperties(eCarrierType::ePlate_96);
	properties_[eCarrierType::eUnknown]  = getDefaultProperties(eCarrierType::eUnknown);

	resetInitParameters();

	enableStageHoldingCurrent([=](bool) -> void {}, false);	// disable holding currents after init/reinit

	stageCalibrated_ = false;
}

/// <summary>
/// Destructor to delete instance and release all resources
/// </summary>
StageController::~StageController()
{
	Quit();
}

void StageController::loadConfiguration(std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile)
{
	boost::system::error_code ec;
	bool fileChg = false;
	t_pPTree cfg;

	if (cfgFile.empty())
	{
		cfgFile = configFile;
	}
	if (cfgFile.empty())
	{
		cfgFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl);
	}
	else if ((!configFile.empty()) && (cfgFile.compare(configFile) != 0))
	{
		fileChg = true;
	}

	cfg.reset();
	if (fileChg ||
		(cfgTree
		 && thetaMotor->CfgTreeValid(cfgTree, CAROUSELCONTROLLERNODENAME)
		 && thetaMotor->CfgTreeValid(cfgTree, PLATECONTROLLERNODENAME))
		)     // configuration tree passed in may not be valid for motor controllers...
	{
		cfg = cfgTree;
	}
	else
	{
		if (parentTree)
		{
			cfg = parentTree;
		}
	}

	if (!cfg)
	{
		cfg = ConfigUtils::OpenConfigFile(cfgFile, ec, true);
	}
	Logger::L().Log (MODULENAME, severity_level::normal, MODULENAME);

	if (cfg)
	{
		configFile = cfgFile;
	}

	parentTree = cfg;

	if (!parentTree)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Init : Error parsing configuration file: - \"config\" section not found or not given");
	}
	else
	{
		if (!configNode)
		{
			configNode = parentTree->get_child_optional("config");      // top level parent node
		}

		if (!configNode)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Init : Error parsing configuration file \"" + configFile + "\" - \"config\" section not found");
		}
	}

	if ((port.length() > 0) && (cbiPort.compare(port) != 0))      // empty port strings in internal configuration means we haven't opened the serial port...
	{
		cbiPort = port;
	}
	apply_ = apply;

	// Load the default carousel properties from "MotorControl.info" file	
	InitCarouselDefault();
	// Load the default plate properties from "MotorControl.info" file	
	InitPlateDefault();

	// Load the carousel properties from "ControllerCal.info" file
	InitCarousel();
	// Load the plate properties from "ControllerCal.info" file
	InitPlate();
}

/// <summary>
/// Call this method to initialize and home theta, radius and probe motors
/// </summary>
/// <returns><c>[true]</c> if all motors are homed successfully; otherwise <c>[false]</c></returns>
void StageController::initializeAndHomeMotorsAsync(std::function<void(bool)> callback, InitializeAndHomeStates currentState)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [ this, callback ](bool status, InitializeAndHomeStates nextState)
	{
		if (!status)
		{
			// Some error has occurred so set the next state as error state
			nextState = InitializeAndHomeStates::eError;
		}

		// Execute next state of initializeAndHomeMotorsAsync
		pCBOService_->enqueueInternal(
			std::bind(&StageController::initializeAndHomeMotorsAsync, this, callback, nextState));
	};

	switch (currentState)
	{
		case InitializeAndHomeStates::eInitializeProbe:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <State - eInitializeProbe>");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initProbeAsync, this, std::placeholders::_1, InitRetryCnt),
				ProbeOperationSequence::po_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, InitializeAndHomeStates::eIntializeRadius));
			return;
		}

		case InitializeAndHomeStates::eIntializeRadius:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <State - eIntializeRadius>");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initRadiusAsync, this, std::placeholders::_1, InitRetryCnt),
				RadiusMotorOperationSequence::ra_mo_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, InitializeAndHomeStates::eCarouselRadiusHome));
			return;
		}

		case InitializeAndHomeStates::eCarouselRadiusHome:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <State - eCarouselRadiusHome>");
			if (!checkCarouselPresent())
			{
				pCBOService_->enqueueInternal(std::bind(onCurrentStateComplete, true, InitializeAndHomeStates::eInitializeTheta));
				return;
			}

			// Go to the Carousel Radius Home
			doCarouselRadiusHomeAsync(
				std::bind(onCurrentStateComplete, std::placeholders::_1, InitializeAndHomeStates::eInitializeTheta));
			return;
		}

		case InitializeAndHomeStates::eInitializeTheta:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <State - eInitializeTheta>");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::initThetaAsync, this, std::placeholders::_1, InitRetryCnt),
				ThetaMotorOperationSequence::th_mo_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, InitializeAndHomeStates::eCarouselThetaHome));
			return;
		}

		case InitializeAndHomeStates::eCarouselThetaHome:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <State - eCarouselThetaHome>");
			if (!checkCarouselPresent())
			{
				pCBOService_->enqueueInternal(std::bind(onCurrentStateComplete, true, InitializeAndHomeStates::eCompleted));
				return;
			}

			auto onHomeThetaForCarousel = [ this, onCurrentStateComplete ](bool status, bool tubeFound)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "initializeAndHomeMotorsAsync : <State - eCarouselThetaHome> : Failed to move theta");
					carouselTubeFoundDuringInit_ = tubeFound;
				}
				onCurrentStateComplete(status, InitializeAndHomeStates::eCompleted);
			};

			// Move theta motor to home position (tube 1 under probe) for carousel
			auto doHomeThetaForCarousel = [ this ](std::function<void(bool)> localCallback)
			{
				doCarouselHomeAsync(localCallback);
			};

			// Since we need to search for tube so reduce the theta max speed by half
			// "carouselMoveThetaAndDetectTube" will take care of restoring the speed to original
			const uint32_t newThetaMaxSpeed = getCurrentProperties().thetaMotionProfile.MaxSpeed / 2;
			const bool failonTubeDetect = true;
			carouselMoveTheta(failonTubeDetect, ThetaMotionTubeDetectStates::eEntryPoint, doHomeThetaForCarousel, onHomeThetaForCarousel, newThetaMaxSpeed);
			return;
		}

		case InitializeAndHomeStates::eCompleted:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "initializeAndHomeMotorsAsync : <Motors initialization and homing  completed>");
			enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, true);
			return;
		}

		case InitializeAndHomeStates::eError:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "initializeAndHomeMotorsAsync : <Failed to complete initialize and home motors>");
			enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, false);
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void StageController::finalizeStageCalibrationAsync(std::function<void(bool)> callback, FinalizeStageCalibrationStates state)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [ this, callback ](bool status, FinalizeStageCalibrationStates nextstate)
	{
		if (!status)
		{
			// error occurred so set the next state to "error state"
			nextstate = FinalizeStageCalibrationStates::eError;
		}

		// Execute next state of SelectStage
		pCBOService_->enqueueInternal([ = ]()
		{
			finalizeStageCalibrationAsync(callback, nextstate);
		});
	};

	switch (state)
	{
		case FinalizeStageCalibrationStates::eProbeHome:
		{
			// home the probe motor
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <State - eProbeHome>");
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				ProbeOperationSequence::po_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, FinalizeStageCalibrationStates::eRadiusHome));
			return;
		}

		case FinalizeStageCalibrationStates::eRadiusHome:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <State - eRadiusHome>");
			// home the radius motor
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doHomeRadiusAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
				RadiusMotorOperationSequence::ra_mo_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, FinalizeStageCalibrationStates::eCarouselRadiusHome));
			return;
		}

		case FinalizeStageCalibrationStates::eCarouselRadiusHome:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <State - eCarouselRadiusHome>");
			// This is only necessary for Carousel Controller
			// So that tube can be dumped into discard tray if needed 
			if (!checkCarouselPresent())
			{
				pCBOService_->enqueueInternal(std::bind(onCurrentStateComplete, true, FinalizeStageCalibrationStates::eThetaHome));
				return;
			}

			doCarouselRadiusHomeAsync(std::bind(onCurrentStateComplete, std::placeholders::_1, FinalizeStageCalibrationStates::eThetaHome));
			return;
		}

		case FinalizeStageCalibrationStates::eThetaHome:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <State - eThetaHome>");

			// home the theta motor
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doHomeThetaAsync, this, std::placeholders::_1, false /*no fail for tube detect*/, getCurrentProperties().retriesCount),
				ThetaMotorOperationSequence::th_mo_entry_point,
				std::bind(onCurrentStateComplete, std::placeholders::_1, FinalizeStageCalibrationStates::eHomeStage));
			return;
		}

		case FinalizeStageCalibrationStates::eHomeStage:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <State - eHomeStage>");
			// Now home the stage (carousel/plate)
			homeStageAsync(std::bind(onCurrentStateComplete, std::placeholders::_1, FinalizeStageCalibrationStates::eCompleted));
		}

		case FinalizeStageCalibrationStates::eCompleted:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <Finalize Stage calibration completed>");
			enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, true);
			return;
		}

		case FinalizeStageCalibrationStates::eError:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "finalizeStageCalibrationAsync : <Finalize Stage calibration error occurred>");
			enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, false);
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void StageController::homeStageAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (checkCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "homeStageAsync : Carousel detected!");
		pCBOService_->enqueueInternal(std::bind(&StageController::doCarouselHomeAsync, this, callback));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "homeStageAsync : Plate detected!");
		pCBOService_->enqueueInternal(std::bind(&StageController::doPlateHomeAsync, this, callback));
	}
}

/// <summary>
/// Resets the local fields to defaults values
/// </summary>
void StageController::resetInitParameters()
{
	parentTree.reset();
	configNode.reset();
	controllersNode.reset();

	updateCarouselConfig = false;
	updatePlateConfig = false;
	lastCarouselPresentStatus_ = false;
	carouselTubeFoundDuringInit_ = false;
	radiusPosInited = false;
	probePosInited = false;
	thetaPosInited = false;
	platePosInited = false;
	stageControllerInitialized_ = false;
	stageControllerInitBusy_ = false;
	thetaPosUpdated_ = false;
	radiusPosUpdated_ = false;
	cancelMove_ = false;
	cancelThetaMove_ = false;
	cancelRadiusMove_ = false;

	maxTubes = MaxCarouselTubes;
	degreesPerTube = DegreesPerTube;
	unitsPerTube = PosStepPerTube;
	maxCarouselPos = MaxThetaPosition;
	thetaDeadband = ThetaChange1Degree;

	tubeCalcTolerance =
		((DefaultThetaMotorUnitsPerRev * DefaultGearRatio * DegreesPerTube)
		 / 360)
		* 0.65;
	tubePosTolerance =
		((DefaultThetaMotorUnitsPerRev * DefaultGearRatio * DegreesPerTube)
		 / 360)
		* 0.35;

	plateWellSpacing = DefaultInterWellSpacing;
	radiusDeadband = 0;
	lastRadiusDir = 0;
	radiusBacklashCompensation = 0;
}

/// <summary>
/// Initialize the stage controller
/// </summary>
/// <returns><c>[true]</c> if success; otherwise <c>[false]<c> through callback</c></returns>
void StageController::initAsync(std::function<void(bool)> cb, std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile)
{
	HAWKEYE_ASSERT (MODULENAME, cb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "Init: <enter>");

	resetInitParameters();

	// Load default configuration 
	loadConfiguration(port, cfgTree, apply, cfgFile);

	// Lambda to check if stage is changed from carousel to plate to vice versa	
	auto CarrierPresent = [ this ]() -> bool
	{
		bool isSet = pCBOService_->CBO()->GetSignalStatus().isSet(SignalStatus::CarouselPresent);
		if (isSet != lastCarouselPresentStatus_.load())
		{
			eCarrierType type = isSet ? eCarrierType::eCarousel : eCarrierType::ePlate_96;
			// Select the stage with new stage controller type
			pCBOService_->enqueueInternal([ this, type ]()
			{
				MoveClear();
				selectStageAsync([ this ](bool status)
				{
					if (!status)
					{
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::motion_sampledeck_initerror, 
							instrument_error::motion_sample_deck_instances::general, 
							instrument_error::severity_level::error));
					}

					enableStageHoldingCurrent([=](bool) -> void {}, false);	// disable holding currents after init/reinit
				}, type);
			});
		}

		this->lastCarouselPresentStatus_ = isSet;
		return true;
	};

	MoveClear();

	// Set the stage
	selectStageAsync([ this, CarrierPresent, cb ](bool status)
	{
		if (status)
		{
			// Set a timer to periodically (1 seconds) check is stage controller is changed
			if (!pStageDetectUpdateTimer || !pStageDetectUpdateTimer->isRunning())
			{
				pStageDetectUpdateTimer = std::make_shared<DeadlineTimerUtilities>();
				bool timerOk = pStageDetectUpdateTimer->waitRepeat(*asyncCommandHelper_->getIoService(), boost::posix_time::milliseconds(1000), [](bool)
				// deliberately empty callback...?
				{
				}, CarrierPresent, boost::none);

				if (!timerOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Init: pStageDetectUpdateTimer: failed to start timer!");
				}
			}
		}

		enableStageHoldingCurrent([=](bool) -> void {}, false);	// disable holding currents after init/reinit
		pCBOService_->enqueueInternal(cb, status);
	}, eCarrierType::eCarousel);

	Logger::L().Log (MODULENAME, severity_level::debug2, "Init: <exit>");
}

void StageController::Reinit()
{
	ReinitTheta();
	ReinitRadius();
	ReinitProbe();

	enableStageHoldingCurrent([=](bool) -> void {}, false);	// disable holding currents after init/reinit
}

/// <summary>
/// Write the plate/carousel properties to motor info file
/// </summary>
/// <returns>The status greater or equal to "0" if success</returns>
int32_t StageController::WriteControllerConfig()
{
	int32_t status = 0;

	int32_t carouselStatus = WriteCarouselControllerConfig();
	int32_t	plateStatus = WritePlateControllerConfig();
	if (carouselStatus < 0 || plateStatus < 0)
	{
		status = (std::min)(carouselStatus, plateStatus);
	}

	return status;
}

/// <summary>
/// updates the plate/carousel controller configurations in motor info file
/// </summary>
/// <param name="type">The stage controller type</param>
/// <param name="controller_node">The controller node</param>
/// <param name="config_node">The configuration node</param>
/// <returns>The status greater or equal to "0" if success</returns>
int32_t StageController::UpdateControllerConfig (eCarrierType type, t_opPTree & controller_node, t_opPTree & config_node)
{
	if (!config_node)
	{
		return -3;
	}

	// controller_node points at the "xxx_controller" node for this particular controller... (should be the same as the preserved 'thisControllerNode')
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

	boost::property_tree::ptree& ptControllerCfgNode = config_node.get();

	UpdateThetaControllerConfig(ptControllerCfgNode, properties_[type].thetaProperties, type);
	UpdateRadiusControllerConfig(ptControllerCfgNode, properties_[type].radiusProperties, type);
	UpdateProbeControllerConfig(ptControllerCfgNode, properties_[type].probeProperties, type);

	(*controller_node).put_child("controller_config", ptControllerCfgNode);

	// Finally, update the cached properties for this type
	cachedProperties_[type] = properties_[type];

	return 0;
}

/// <summary>
/// Destroy the current instance and release all resources
/// </summary>
void StageController::Quit(void)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Quit: enter");

	if (pStageDetectUpdateTimer)
	{
		pStageDetectUpdateTimer->cancel();
	}

	if (updateCarouselConfig || updatePlateConfig)
	{
		WriteControllerConfig();
	}

	thetaMotorStatus_.reset();
	radiusMotorStatus_.reset();
	probeMotorStatus_.reset();

	thetaMotor->Quit();
	radiusMotor->Quit();
	probeMotor->Quit();

	parentTree.reset();
	configNode.reset();
	controllersNode.reset();

	carouselControllerNode.reset();
	carouselMotorParamsNode.reset();
	carouselControllerConfigNode.reset();

	plateControllerNode.reset();
	plateMotorParamsNode.reset();
	plateControllerConfigNode.reset();

	if (pStageDetectUpdateTimer)
	{
		pStageDetectUpdateTimer.reset();
	}
}

/// <summary>
/// Gets the current stage controller OK status
/// </summary>
/// <returns><c>[true]</c> if current stage controller is OK; otherwise <c>[false]</c></returns>
bool StageController::ControllerOk()
{
	return getCurrentProperties().controllerOk;
}

/// <summary>
/// Select the current stage controller
/// This method initialize the stage controller if not initialized and home the stage controller motors
/// If the stage controller is initialized then it will set the motion profiles of current stage controller type
/// but it will not rehome any of the motor
/// </summary>
/// <param name="type">
/// Input parameter to set stage controller type. Default is Carousel <see cref="eCarrierType::eCarousel"/>
/// This parameter is only used in "Simulator Mode" <see cref="StageControllerSim"/>
/// </param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::selectStageAsync(std::function<void(bool)> callback, eCarrierType type)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageAsync : <Enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (stageControllerInitBusy_)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageAsync : Stage controller is busy <exit>");
		pCBOService_->enqueueInternal(callback, stageControllerInitialized_);
		return;
	}

	pCBOService_->enqueueInternal(
		std::bind(&StageController::selectStageInternal, this, callback, SelectStageStates::eEntryPoint));

	Logger::L().Log (MODULENAME, severity_level::debug2, "selectStageAsync : <exit>");
}

/// <summary>
/// Gets the value indicating whether carousel tube was found during theta motor homing operation
/// </summary>
/// <returns><c>[true]</c> if tube found; otherwise <c>[false]</c></returns>
bool StageController::carouselTubeFoundDuringInit()
{
	return carouselTubeFoundDuringInit_;
}

/// <summary>
/// Move the stage controller to target position
/// This method will block the thread from which it is being called
/// </summary>
/// <param name="position">The target position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::moveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "moveStageToPosition: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (position.isValidForCarousel() && getCurrentType() ==eCarrierType::eCarousel)
	{
		pCBOService_->enqueueInternal (std::bind(&StageController::moveCarouselToPositionAsync, this, callback, position));
	}
	else if (position.isValidForPlate96() && getCurrentType() == eCarrierType::ePlate_96)
	{
		pCBOService_->enqueueInternal (std::bind(&StageController::movePlateToPositionAsync, this, callback, position));
	}
	else
	{
		pCBOService_->enqueueInternal (callback, false);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveStageToPosition: <exit>");
}

void StageController::stopAsync(std::function<void(bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "stopAsync: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
	asyncTaskList.clear();

	// add clear errors for all the motors to list
	asyncTaskList.push_back([ this ](auto cb)
	{
		this->clearErrorsAsync(cb);
	});

	if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::ProbeMotorBusy))
	{
		// add probe clear error to list
		asyncTaskList.push_back([ this ](auto cb)
		{
			probeMotor->Stop(cb, true, getProbeProperties().probeBusyTimeout_msec / 1000);
		});
	}

	if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::RadiusMotorBusy))
	{
		// add radius clear error to list
		asyncTaskList.push_back([ this ](auto cb)
		{
			radiusMotor->Stop(cb, true, getRadiusProperties().radiusFullTimeout_msec / 1000);
		});
	}

	if (pCBOService_->CBO()->GetBoardStatus().isSet(BoardStatus::ThetaMotorBusy))
	{
		// add theta clear error to list
		asyncTaskList.push_back([ this ](auto cb)
		{
			thetaMotor->Stop(cb, true, getThetaProperties().thetaFullTimeout_msec / 1000);
		});
	}

	// execute the queued tasks
	asyncCommandHelper_->queueASynchronousTask([ this, callback ](bool status)
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetThetaPosition(), ePositionDescription::Current);
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, GetRadiusPosition(), ePositionDescription::Current);
		probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, getProbePosition(), ePositionDescription::Current);

		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "stopAsync : Failed to stop the motors!");
		}

		pCBOService_->enqueueInternal(callback, status);
	}, asyncTaskList, false);

	Logger::L().Log (MODULENAME, severity_level::debug2, "stopAsync: <exit>");
}

void StageController::clearErrorsAsync(std::function<void(bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "clearErrorsAsync: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;
	asyncTaskList.clear();

	// add probe clear error to list
	asyncTaskList.push_back([ this ](auto cb)
	{
		probeMotor->ClearErrors(cb);
	});

	// add radius clear error to list
	asyncTaskList.push_back([ this ](auto cb)
	{
		radiusMotor->ClearErrors(cb);
	});

	// add theta clear error to list
	asyncTaskList.push_back([ this ](auto cb)
	{
		thetaMotor->ClearErrors(cb);
	});

	// execute the queued tasks
	asyncCommandHelper_->queueASynchronousTask(callback, asyncTaskList, false);

	Logger::L().Log (MODULENAME, severity_level::debug2, "clearErrorsAsync: <exit>");
}

/// <summary>
/// Gets the current stage controller position
/// This method will block the thread from which it is being called
/// </summary>
/// <param name="position">The placeholder for stage controller current position</param>
void StageController::getStagePosition(SamplePositionDLL& position)
{
	eCarrierType currentType = getCurrentType();
	position = SamplePositionDLL();

	if (currentType == eCarrierType::eCarousel)
	{
		getCarouselPosition(position);
	}
	else if (currentType == eCarrierType::ePlate_96)
	{
		getPlatePosition(position);
	}
}

StageController::CalibrationConfigState StageController::isStageCalibrated()
{
	//If the stage is calibrated or canSkipMotorCalibrationCheck() is enabled then return "ePlateAndCarousel".
	if (stageCalibrated_ || canSkipMotorCalibrationCheck())
		return CalibrationConfigState::ePlateAndCarousel;

	bool plateCalibrated = false;
	const DBApi::DB_InstrumentConfig& instCfg = InstrumentConfig::Instance().Get();
	if ((instCfg.PlateThetaHomeOffset != 0) || (instCfg.PlateRadiusCenterPos != 0) || (instCfg.PlateThetaCalPos != 0))
	{
		plateCalibrated = true;
	}
	bool carouselCalibrated = false;
	if ((instCfg.CarouselThetaHomeOffset != 0) || (instCfg.CarouselRadiusOffset != 0))
	{
		carouselCalibrated = true;
	}

	stageCalibrated_ = plateCalibrated && carouselCalibrated;
	if (stageCalibrated_)
		return CalibrationConfigState::ePlateAndCarousel;
	if (plateCalibrated)
		return CalibrationConfigState::eOnlyPlate;
	if (carouselCalibrated)
		return CalibrationConfigState::eOnlyCarousel;
	return CalibrationConfigState::eNone;
}

/// <summary>
/// Eject the stage controller
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::ejectStageAsync(std::function<void(bool)> callback, int32_t angle )
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug2, "EjectStage <Enter>");

	// get the hardware-detected carrier type; this will NEVER be A-Cup, but might be UNKNOWN, CAROUSEL, or PLATE 
	eCarrierType type = getCurrentType();

	if (!MoveSetup(type, true))
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "EjectStage : Move setup failed");
		pCBOService_->enqueueInternal(std::bind(callback, false));
		return;
	}

	std::function<bool(uint16_t)> retryCb = nullptr;
	bool noThetaEject = true;

	MoveStageParams thetaParams = MoveStageParams(retryCb, 0, 0);
	MoveStageParams radiusParams = MoveStageParams(retryCb, 0, 0);

	if (type == eCarrierType::eCarousel)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "EjectStage : Carousel is detected");

		// move to the carousel eject location, but do not rotate the carousel
		// for carousel, don't move theta - so pass the current theta value as input
		auto thetaProperties = properties_[eCarrierType::eCarousel].thetaProperties;
		thetaParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
		{
			return canDoRetryForCarouselMovement(retriesDone, false, GetThetaPosition());
		}, GetThetaPosition(), (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);				// use deadband

		auto radiusProperties = properties_[eCarrierType::eCarousel].radiusProperties;
		radiusParams = MoveStageParams([this, rmaxT = radiusProperties.radiusMaxTravel](uint16_t retriesDone) -> bool
		{
			return canDoRetryForCarouselMovement(retriesDone, true, rmaxT);
		}, radiusProperties.radiusMaxTravel, (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);		// use deadband
	}
	else	// treat UNKNOWN as a plate
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "EjectStage : Plate is detected");

		int32_t homeThetaPos = 0;
		int32_t homeRadiusPos = 0;
		getPlateHomePosition(homeThetaPos, homeRadiusPos);
		noThetaEject = false;

		// for plate, do rotation to the home position or adjust to the request angular position
		auto thetaProperties = properties_[eCarrierType::ePlate_96].thetaProperties;
		homeThetaPos += (angle * ThetaChange1Degree);

		thetaParams = MoveStageParams( [ this, homeThetaPos ]( uint16_t retriesDone ) -> bool
									   {
										   return canDoRetryForPlateMovement( retriesDone );
									   }, homeThetaPos, ( thetaProperties.thetaFullTimeout_msec / 1000 ), 0, true );					// use deadband

		auto radiusProperties = properties_[eCarrierType::ePlate_96].radiusProperties;
		radiusParams = MoveStageParams([ this ](uint16_t retriesDone) -> bool
										{
											return canDoRetryForPlateMovement( retriesDone );
										}, radiusCurrentMaxTravel(), ( radiusProperties.radiusFullTimeout_msec / 1000 ), 0, true );			// use deadband
	}

	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams, noThetaEject ](auto ex)
	{
		moveStageToPositionAsync(ex, thetaParams, radiusParams, noThetaEject);
	}, StageControllerOperationSequence::sc_entry_point, callback);

	Logger::L().Log (MODULENAME, severity_level::normal, "EjectStage <Exit> ");
}

/// <summary>
/// check that the stage is at the correct position for the tube or well requested.
/// performed just prior to putting the probe down.
/// This method adjusts to the nearest possible tube position forwards or backwards.
/// If it can't move backwards to the expected approximate tube position, it will
/// attempt to move forward a single tube position, or fail if it can't
/// This method will block the thread from which it is being called
/// </summary>
/// <param name="pos">Input parameter to return the resulting carousel position</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::StagePositionOk(std::function<void(SamplePositionDLL)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	bool posOk = false;
	bool adjustOk = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "StagePositionOk : <Enter>");

	eCarrierType currentType = getCurrentType();
	auto thetaProperties = properties_[currentType].thetaProperties;
	auto radiusProperties = properties_[currentType].radiusProperties;

	SamplePositionDLL tempPos;

	// Get the current stage row and column
	getStagePosition(tempPos);

	int32_t currentThetaPos = 0;
	int32_t currentRadiusPos = 0;
	int32_t tgtThetaPos = 0;
	int32_t tgtRadiusPos = 0;
	int32_t homeThetaPos = 0;
	int32_t homeRadiusPos = 0;
	bool thetaOk = false;
	bool radiusOk = false;

	// Get the current stage theta and radius
	getThetaRadiusPosition(currentThetaPos, currentRadiusPos);

	MoveClear();

	if (currentType ==eCarrierType::eCarousel)
	{
		int32_t tgtTubeNum = 0;

		getCarouselHomePosition(homeThetaPos, homeRadiusPos);

		if (tempPos.isValidForCarousel())		// are the row/column values valid for a carousel position?
		{
			tgtTubeNum = tempPos.getColumn();
		}
		else
		{
			tgtTubeNum = getApproxCarouselTubeNumber(currentThetaPos, homeThetaPos);
			tempPos.setRowColumn(eStageRows::CarouselRow, static_cast<uint8_t>(tgtTubeNum));
		}

		bool doTubeIncrementCheck = false;
		do
		{
			if (getCarouselPositionFromRowCol(tgtTubeNum, tgtThetaPos))					// get the ideal target position of the current tube to check for positioning tolerances
			{
				// getCarouselPositionFromRowCol returns the tube 1 based offset position; need to correct for comparison
				tgtThetaPos += homeThetaPos;
				tgtRadiusPos = homeRadiusPos;

				normalizeThetaPosition(tgtThetaPos, maxCarouselPos);

				// Is current position near enough to ideal?
				radiusOk = RadiusAtPosition(currentRadiusPos, tgtRadiusPos, 0, true);
				thetaOk = ThetaAtPosition(currentThetaPos, tgtThetaPos, 0, true);

				if (thetaOk && radiusOk)
				{
					posOk = true;
					doTubeIncrementCheck = false;
				}
				else	// position calculated, but not currently at the position
				{
					if (!radiusOk)
					{
						if (!adjustCarouselRadiusPos(tgtRadiusPos))
						{
							adjustOk = false;
						}
					}

					// Adjust theta position to prevent carousel counter clockwise movement
					if (!thetaOk)
					{
						if (!adjustCarouselThetaPos(currentThetaPos, tgtThetaPos))
						{
							adjustOk = false;
						}
					}

					if (adjustOk)
					{
						// check that we're not trying to move more than a single tube position here...
						// if move distance is greater than a single tube position, we're probably too far from
						// the previous tube position to move backwards, so try to move to the next tube position...
						if (abs(tgtThetaPos - currentThetaPos) > unitsPerTube)
						{
							if (doTubeIncrementCheck)
							{
								tempPos = SamplePositionDLL();
								pCBOService_->enqueueInternal(callback, tempPos);
								doTubeIncrementCheck = false;
							}
							else
							{
								tgtTubeNum++;
								tempPos.setRowColumn(eStageRows::CarouselRow, static_cast<uint8_t>(tgtTubeNum));
								doTubeIncrementCheck = true;
							}
						}
						else
						{
							doTubeIncrementCheck = false;
						}
					}
					else
					{
						doTubeIncrementCheck = false;
					}
				}
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "StagePositionOk : error getting target positions from sample position!");
				adjustOk = false;
				doTubeIncrementCheck = false;
			}
		} while (doTubeIncrementCheck);
	}
	else
	{
		if (!tempPos.isValidForPlate96())
		{
			getPlateHomePosition(homeThetaPos, homeRadiusPos);

			// Adjust current theta/radius position w.r.t. plate home positions
			int32_t relativeThetaPos = currentThetaPos - homeThetaPos;
			int32_t relativeRadiusPos = currentRadiusPos - homeRadiusPos;

			normalizeThetaPosition(relativeThetaPos, maxCarouselPos);

			uint32_t row = 0;
			uint32_t col = 0;
			if (!getRowColFromPlatePosition(relativeThetaPos, relativeRadiusPos, row, col))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "StagePositionOk : stage position does not correspond to a row / col location!");
				pCBOService_->enqueueInternal(callback, SamplePositionDLL());
				return;
			}

			char rowChar = row + (eStageRows::PlateRowA - 1);
			tempPos.setRowColumn(rowChar, static_cast<uint8_t>(col));

			tgtThetaPos = 0;
			tgtRadiusPos = 0;

			// do back calculation to check if at an exact position
			if (getPlatePositionFromRowCol(row, col, tgtThetaPos, tgtRadiusPos))
			{
				// getPlatePositionFromRowCol returns the 0-center based position; need to correct for comparison
				tgtThetaPos += homeThetaPos;
				tgtRadiusPos += homeRadiusPos;

				thetaOk = ThetaAtPosition(currentThetaPos, tgtThetaPos, 0, true);
				// Adjust theta position to prevent carousel counter clockwise movement
				if (!thetaOk)
				{
					if (!adjustPlateThetaPos(currentThetaPos, tgtThetaPos))
					{
						adjustOk = false;
					}
				}

				radiusOk = RadiusAtPosition(currentRadiusPos, tgtRadiusPos, 0, true);
				if (!radiusOk)
				{
					if (!adjustPlateRadiusPos(currentRadiusPos, tgtRadiusPos))
					{
						adjustOk = false;
					}
				}

				if (thetaOk && radiusOk)
				{
					posOk = true;
				}
			}
			else
			{
				adjustOk = false;
			}
		}
		else
		{
			posOk = true;
		}
	}

	// position calculated, but not currently at the position
	if (adjustOk && !posOk)
	{
		std::function<bool(uint16_t)> retryLambdaTheta;
		std::function<bool(uint16_t)> retryLambdaRadius;
		if (currentType ==eCarrierType::eCarousel)
		{
			retryLambdaTheta = [ this, tgtThetaPos ](uint16_t retriesDone) -> bool
			{
				return canDoRetryForCarouselMovement(retriesDone, false, tgtThetaPos);
			};
			retryLambdaRadius = [ this, tgtRadiusPos ](uint16_t retriesDone) -> bool
			{
				return canDoRetryForCarouselMovement(retriesDone, true, tgtRadiusPos);
			};
		}
		else
		{
			retryLambdaTheta = [ this ](uint16_t retriesDone) -> bool
			{
				return canDoRetryForPlateMovement(retriesDone);
			};
			retryLambdaRadius = [ this ](uint16_t retriesDone) -> bool
			{
				return canDoRetryForPlateMovement(retriesDone);
			};
		}

		// Set stage controller target theta properties
		MoveStageParams thetaParams = MoveStageParams(retryLambdaTheta,
		                                              tgtThetaPos,
		                                              (thetaProperties.thetaFullTimeout_msec / 1000), 0, true);			// use deadband as check tolerance

		// Set stage controller target radius properties
		MoveStageParams radiusParams = MoveStageParams(retryLambdaRadius,
		                                               tgtRadiusPos,
		                                               (radiusProperties.radiusFullTimeout_msec / 1000), 0, true);		// use deadband as check tolerance

		// Move stage to target position asynchronously
		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync([ this, thetaParams, radiusParams ](auto ex)
		{
			moveStageToPositionAsync(ex, thetaParams, radiusParams, false);
		}, StageControllerOperationSequence::sc_entry_point,
		// method call using an additional lambda callback parameter...
		[ this, tempPos, callback ](bool status) mutable
		{
			std::string statusStr = (status ? "true" : "false");
			Logger::L().Log (MODULENAME, severity_level::debug2, "StagePositionOk : <Exit>: positionOK: " + statusStr);

			if (!status)
			{
				tempPos = SamplePositionDLL();
			}
			pCBOService_->enqueueInternal(callback, tempPos);
			return;
		});
		return;
	}

	if (!adjustOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "StagePositionOk : target position cannot be adjust to valid position!");
		tempPos = SamplePositionDLL();
	}

	pCBOService_->enqueueInternal(callback, tempPos);
}

/// <summary>
/// Home the motors (theta & radius) for calibration
/// This is different from Stage home <see cref="StageController::HomeStage"/> where
/// we will home either plate or carousel along with the their home offset values
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::calibrateMotorsHomeAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!probePosInited)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Probe Motor is not initialized");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::motion_motor_initerror, 
			instrument_error::motion_motor_instances::sample_probe, 
			instrument_error::severity_level::warning));
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	if (!radiusPosInited)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Radius Motor is not initialized");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::motion_motor_initerror, 
			instrument_error::motion_motor_instances::radius, 
			instrument_error::severity_level::warning));
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	if (!thetaPosInited)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Theta Motor is not initialized");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::motion_motor_initerror,
			instrument_error::motion_motor_instances::theta, 
			instrument_error::severity_level::warning));
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	auto onRadiusHomeComplete = [ this, callback ](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "CalibrateMotorsHome : <Failed to Home Radius>");
			this->enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		auto onThetaHomeComplete = [this, callback](bool status)
		{
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "CalibrateMotorsHome : <Failed to Home Theta>");
			}
			this->enableStageHoldingCurrent([=](bool) -> void {}, false);
			pCBOService_->enqueueInternal(callback, status);
		};

		// Do the theta motor home
		// this workflow does not require the user id, so do not use the transient user technique
		asyncCommandHelper_->postAsync(
			std::bind(&StageController::doHomeThetaAsync, this, std::placeholders::_1, true /*fail on tube detect*/, getCurrentProperties().retriesCount),
			ThetaMotorOperationSequence::th_mo_entry_point, onThetaHomeComplete);
	};

	// Do the radius motor home
	// this workflow does not require the user id, so do not use the transient user technique
	asyncCommandHelper_->postAsync(
		std::bind(&StageController::doHomeRadiusAsync, this, std::placeholders::_1, getCurrentProperties().retriesCount),
		RadiusMotorOperationSequence::ra_mo_entry_point, onRadiusHomeComplete);
}

/// <summary>
/// Calibrate the stage controller radius positions asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::calibrateStageRadiusAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// This will fetch the latest radius motor position
    int32_t currentRPos = GetRadiusPosition();

	// store the current radius properties
	auto& rp = properties_[this->getCurrentType()].radiusProperties;

	// For Carousel - update the radius offset value
	if (checkCarouselPresent())
	{
		rp.radiusOffset = currentRPos;
		updateCarouselConfig = true;
	}
	else
	{
		// For Plate - update the plate center position
		rp.radiusCenterPos = currentRPos;
		updatePlateConfig = true;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("updateRadiusConfiguration : RadiusPos <%d>") % currentRPos));

	pCBOService_->enqueueInternal(callback, true);
}

/// <summary>
/// Calibrate the stage controller theta positions asynchronously
/// </summary>
/// <param name="cb">Callback to indicate caller</param>
void StageController::calibrateStageThetaAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// This will fetch the latest theta motor position
    // Get the current stage theta and radius
	int32_t thetaPos = 0;
	getRawThetaPosition(thetaPos);

	// store the current theta properties
	auto& tp = properties_[this->getCurrentType()].thetaProperties;

	// For Carousel - update the theta home offset position
	if (checkCarouselPresent())
	{
		// if theta position is negative:
		//        the user has moved the carousel (while calibrating) counter-clockwise which compensates for
		//        the theta motor backlash.  Normalize negative theta position and don't add theta backlash.
		// If theta value is positive:
		//        add theta backlash in offset position
		if (thetaPos >= 0)
		{
			thetaPos += tp.thetaBacklash;
		}
		this->normalizeThetaPosition(thetaPos, maxCarouselPos);
		tp.thetaHomePosOffset = thetaPos;
		this->updateCarouselConfig = true;
	}
	else
	{
		this->normalizeThetaPosition(thetaPos, maxCarouselPos);

		// For Plate - update the theta calibration position (90 degree from theta home)
		tp.thetaCalPos = thetaPos;

		// Update the theta home offset position from plate calibration position (90 degree from theta home)
		tp.thetaHomePosOffset = tp.thetaCalPos - (tp.maxThetaPos / 4);
		this->updatePlateConfig = true;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("CalibrateStageTheta : homeThetaPos <%d>") % thetaPos));

	pCBOService_->enqueueInternal(callback, true);
}

/// <summary>
/// Finalize the calibration of radius and theta motor
/// This method will either finish the ongoing calibration and
/// write the updated values to configuration file
/// </summary>
/// <param name="cancelCalibration">Input parameter to cancel calibration</param>
/// <returns><c>[true]</c> if success; otherwise <c>[false]</c></returns>
void StageController::finalizeCalibrateStageAsync(std::function<void(bool)>callback, bool cancelCalibration)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (cancelCalibration)
	{
		// Restore from the last cache
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str (boost::format("finalizeCalibrateStageAsync(cancelled) - restoring %s")
			% (getCarrierTypeAsStr(getCurrentType()))));
		properties_[getCurrentType()].thetaProperties = cachedProperties_[getCurrentType()].thetaProperties;
		properties_[getCurrentType()].radiusProperties = cachedProperties_[getCurrentType()].radiusProperties;

		if (getCurrentType() == eCarrierType::eCarousel)
		{
			updateCarouselConfig = false;
		}
		else
		{
			updatePlateConfig = false;
		}

		enableStageHoldingCurrent([=](bool) -> void {}, false);
		pCBOService_->enqueueInternal(callback, true);
		return;
	}

	if (getCurrentType() == eCarrierType::eCarousel &&  updateCarouselConfig)
	{
		WriteCarouselControllerConfig();
	}
	else if (updatePlateConfig)
	{
		WritePlateControllerConfig();
	}

	pCBOService_->enqueueInternal([ this, callback ]()
	{
		finalizeStageCalibrationAsync(callback, FinalizeStageCalibrationStates::eProbeHome);
	});
}

void StageController::setProbeSearchMode(bool enable, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::function<void(bool)> onComplete = [ this, callback, enable ](bool status)
	{
		pCBOService_->enqueueInternal(callback, status);
	};

	if (!opDefaultAbovePosition_)
	{
		opDefaultAbovePosition_ = probeMotor->GetProbeAbovePosition();
	}

	int32_t abovePosToSet = opDefaultAbovePosition_.get();
	if (enable)
	{
		abovePosToSet = ProbeChange_10_0_mm;
	}

	if (abovePosToSet == probeMotor->GetProbeAbovePosition())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "setProbeSearchMode : Already at requested above position : " + std::to_string(abovePosToSet));
		onComplete(true);
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "setProbeSearchMode : enable = " + std::to_string(enable) + ", abovePosToSet = " + std::to_string(abovePosToSet));
	probeMotor->AdjustProbeAbovePosition(abovePosToSet, true, onComplete);
}

/// <summary>
/// Move the stage controller to target theta and radius position asynchronously
/// </summary>
/// <param name="ex">The entry point for move stage operation enum states</param>
/// <param name="thetaParams">The target theta motor parameters</param>
/// <param name="radiusParams">The target radius motor parameters</param>
void StageController::moveStageToPositionAsync(AsyncCommandHelper::EnumStateParams ex, MoveStageParams thetaParams, MoveStageParams radiusParams, bool eject)
{
	// Get the current state
	auto curState = (StageControllerOperationSequence) ex.currentState;

	Logger::L().Log (MODULENAME, severity_level::debug2, "moveStageToPositionAsync : " + EnumConversion<StageControllerOperationSequence>::enumToString(curState));

	if (cancelMove_)
	{
		ex.nextState = StageControllerOperationSequence::sc_cancel;
		curState = StageControllerOperationSequence::sc_cancel;
		cancelThetaMove_ = true;
		cancelRadiusMove_ = true;
	}

	static int32_t startingTPos = 0;
	static int32_t startingRPos = 0;

	int32_t currentTPos = 0;
	int32_t currentRPos = 0;
	getThetaRadiusPosition(currentTPos, currentRPos);

	switch (curState)
	{
		// This is the entry point for stage movement
		case StageControllerOperationSequence::sc_entry_point:
		{
			std::string logStr = std::string();

			startingTPos = currentTPos;
			startingRPos = currentRPos;

			logStr.append(boost::str(boost::format("\nRadiusPos: %d Radius Params <targetPos: %d timeout_Secs: %d tolerance: %d applyDeadBand: %s>")
									 % currentRPos % radiusParams.targetPos % radiusParams.timeout_Secs % radiusParams.tolerance % (radiusParams.applyDeadBand ? "true" : "false")));
			logStr.append(boost::str(boost::format("\nThetaPos: %d Theta Params <targetPos: %d timeout_Secs: %d tolerance: %d applyDeadBand: %s>")
									 % currentTPos % thetaParams.targetPos % thetaParams.timeout_Secs % thetaParams.tolerance % (thetaParams.applyDeadBand ? "true" : "false")));
			Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

			std::string errStr = std::string();
			if (!radiusPosInited)
			{
				errStr.append("moveStageToPositionAsync: radius position not initialized");
			}

			if (!thetaPosInited)
			{
				if (!errStr.empty())
				{
					errStr.append("\n");
				}
				errStr.append("moveStageToPositionAsync: theta position not initialized ");
			}

			if (!probePosInited)
			{
				if (!errStr.empty())
				{
					errStr.append("\n");
				}
				errStr.append("moveStageToPositionAsync: probe position not initialized ");
			}

			if (!errStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				if (!radiusPosInited || !probePosInited || (!thetaPosInited && !eject))		// probe and radius are always fatal; if theta and not ejecting the stage, fail
				{
					ex.nextState = StageControllerOperationSequence::sc_error;
					ex.executeNextState(HawkeyeError::eSuccess);
					return;
				}
			}

			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;

			// Queue task to read radius motor position from controller, not cached value
			asyncTaskList.push_back(
				std::bind(&StageController::readRadiusPositionAsync, this, std::placeholders::_1));

			// Queue task to read theta motor position from controller, not cached value
			asyncTaskList.push_back(
				std::bind(&StageController::readThetaPositionAsync, this, std::placeholders::_1));

			// Run the queued tasks asynchronously
			asyncCommandHelper_->queueASynchronousTask([ex](bool status) mutable -> void
			{
				// run the next state asynchronously
				ex.nextState = StageControllerOperationSequence::sc_move_probe;
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_move_probe:
		{
			// Check and move probe up if not already in up position.
			// Exit from current execution here. callback will resume this execution.
			// this workflow does not require the user id, so do not use the transient user technique
			asyncCommandHelper_->postAsync(
				std::bind(&StageController::doProbeUpAsync, this, std::placeholders::_1, getProbeProperties().probeRetries),
				ProbeOperationSequence::po_entry_point,
				[ ex, this ](bool status) mutable -> void
			{
				// If probe is up then move to next state else set next state as error "sc_error"
				if (status)
				{
					ex.nextState = StageControllerOperationSequence::sc_update_values;
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : Unable to move probe up");
					probeMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_positionfail, 
						instrument_error::motion_motor_instances::sample_probe, 
						instrument_error::severity_level::error));
					ex.nextState = StageControllerOperationSequence::sc_error;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			});

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_update_values:
		{
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);

			if (checkCarouselPresent())
			{
				if (abs(currentTPos - startingTPos) > thetaDeadband)		// if position has changed since the start of this process, may not ba a valid move anymore; update the target
				{
					Logger::L().Log (MODULENAME, severity_level::debug2,
					                boost::str(boost::format("moveStageToPositionAsync <sc_update_values> : theta position changed since entry prior to move operation; Current Theta Position <%d> Starting Theta Position <%d>") % currentTPos % startingTPos));
				}
				startingTPos = currentTPos;
			}
			startingRPos = currentRPos;

			ex.nextState = StageControllerOperationSequence::sc_move_radius;
			ex.executeNextState(HawkeyeError::eSuccess);
			break;
		}
		case StageControllerOperationSequence::sc_move_radius_chk:		// this section is not currently used; the discrete position update reads are triggered in the entry section to ensure completion by the time they are needed
		{
			bool posOk = RadiusAtPosition(currentRPos, radiusParams.targetPos, radiusParams.tolerance, radiusParams.applyDeadBand);

			// If radius is already at target position then no need to move radius motor
			if (posOk)
			{
				radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::AtPosition);

				if (eject)
				{
					ex.nextState = StageControllerOperationSequence::sc_complete;
					ex.executeNextState(HawkeyeError::eSuccess);
					return;
				}

				ex.nextState = StageControllerOperationSequence::sc_move_theta;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;

			// Queue task to read radius motor position from controller
			asyncTaskList.push_back(
				std::bind(&StageController::readRadiusPositionAsync, this, std::placeholders::_1));

			// Run the queued tasks asynchronously
			asyncCommandHelper_->queueASynchronousTask([ex](bool status) mutable -> void
			{
				// run the next state asynchronously
				ex.nextState = StageControllerOperationSequence::sc_move_radius;
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_move_radius:
		{
			bool posOk = RadiusAtPosition(currentRPos, radiusParams.targetPos, radiusParams.tolerance, radiusParams.applyDeadBand);

			// If radius is already at target position then no need to move radius motor
			if (posOk)
			{
				radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::AtPosition);

				if (eject)
				{
					ex.nextState = StageControllerOperationSequence::sc_complete;
					ex.executeNextState(HawkeyeError::eSuccess);
					return;
				}

				ex.nextState = StageControllerOperationSequence::sc_move_theta;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;

			// Queue task to move radius motor to target position
			asyncTaskList.push_back(
				std::bind(&StageController::gotoRadiusPositionAsync, this, std::placeholders::_1, radiusParams.targetPos, (radiusParams.timeout_Secs * 1000)));

			// Queue task to check if radius motor is at target position
			asyncTaskList.push_back(
				std::bind(&StageController::radiusAtPositionAsync, this, std::placeholders::_1, radiusParams.targetPos, radiusParams.tolerance, radiusParams.applyDeadBand));

			// Run the queued tasks asynchronously
			asyncCommandHelper_->queueASynchronousTask([ex, eject](bool status) mutable -> void
			{
				// If status is success then move to next state other wise set the next state to retry state "sc_check_retry_on_timeout_radius"
				if (!status)
				{
					ex.nextState = StageControllerOperationSequence::sc_check_retry_on_timeout_radius;
				}
				else
				{
					if (eject)
					{
						ex.nextState = StageControllerOperationSequence::sc_complete;
						ex.executeNextState(HawkeyeError::eSuccess);
						return;
					}

					ex.nextState = StageControllerOperationSequence::sc_move_theta;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_check_retry_on_timeout_radius:
		{
			// Check if radius motor retry is allowed
			bool status = radiusParams.canRetry(static_cast<uint16_t>(ex.currentStateRetries));

			// If status is success then move to next state other wise set the next state timeout "sc_timeout"
			if (status)
			{
				ex.nextState = StageControllerOperationSequence::sc_move_radius;
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : sc_move_radius timed out!");
				radiusMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_positionfail, 
					instrument_error::motion_motor_instances::radius, 
					instrument_error::severity_level::error));
				ex.nextState = StageControllerOperationSequence::sc_timeout;
			}
			ex.executeNextState(HawkeyeError::eSuccess);
			break;
		}
		case StageControllerOperationSequence::sc_move_theta_chk:		// this section is not currently used; the discrete position update reads are triggered in the entry section to ensure completion by the time they are needed
		{
			bool posOk = ThetaAtPosition(currentTPos, thetaParams.targetPos, thetaParams.tolerance, thetaParams.applyDeadBand);

			// If theta is already at target position or ejecting then no need to move theta motor
			if ((posOk) || (eject))
			{
				ex.nextState = StageControllerOperationSequence::sc_complete;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;

			// Queue task to read theta motor position from controller
			asyncTaskList.push_back(
				std::bind(&StageController::readThetaPositionAsync, this, std::placeholders::_1));

			// Run the queued tasks asynchronously
			asyncCommandHelper_->queueASynchronousTask([ex](bool status) mutable -> void
			{
				// run the next state asynchronously
				ex.nextState = StageControllerOperationSequence::sc_move_theta;
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_move_theta:
		{
			bool posOk = ThetaAtPosition(currentTPos, thetaParams.targetPos, thetaParams.tolerance, thetaParams.applyDeadBand);

			// If theta is already at target position or ejecting then no need to move theta motor
			if ((posOk) || (eject))
			{
				ex.nextState = StageControllerOperationSequence::sc_complete;
				ex.executeNextState(HawkeyeError::eSuccess);
				return;
			}

			if (checkCarouselPresent())
			{
				if (abs(currentTPos - startingTPos) > thetaDeadband)		// if position has changed since the start of this process, may not ba a valid move anymore...
				{
					Logger::L().Log (MODULENAME, severity_level::debug2,
					                boost::str(boost::format("moveStageToPositionAsync <sc_move_theta> : theta position changed since entry prior to move operation; Current Theta Position <%d> Starting Theta Position <%d>") % currentTPos % startingTPos));
				}
			}

			std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList;

			// Queue task to move theta motor to target position
			asyncTaskList.push_back(
				std::bind(&StageController::gotoThetaPositionAsync, this, std::placeholders::_1, thetaParams.targetPos, (thetaParams.timeout_Secs * 1000)));

			// Queue task to check if theta motor is at target position
			asyncTaskList.push_back(
				std::bind(&StageController::thetaAtPositionAsync, this, std::placeholders::_1, thetaParams.targetPos, thetaParams.tolerance, thetaParams.applyDeadBand));

			// Run the queued tasks asynchronously
			asyncCommandHelper_->queueASynchronousTask([ex](bool status) mutable -> void
			{
				// If status is success then move to next state other wise set the next state to retry state "sc_check_retry_on_timeout_theta"
				if (!status)
				{
					ex.nextState = StageControllerOperationSequence::sc_check_retry_on_timeout_theta;
				}
				else
				{
					ex.nextState = StageControllerOperationSequence::sc_complete;
				}

				// run the next state asynchronously
				ex.executeNextState(HawkeyeError::eSuccess);
			}, asyncTaskList, false);

			// Exit from current execution here. callback will resume this execution.
			return;
		}
		case StageControllerOperationSequence::sc_check_retry_on_timeout_theta:
		{
			// Check if theta motor retry is allowed
			bool status = thetaParams.canRetry(static_cast<uint16_t>(ex.currentStateRetries));

			// If status is success then move to next state other wise set the next state timeout "sc_timeout"
			if (status)
			{
				ex.nextState = StageControllerOperationSequence::sc_move_theta;
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : sc_move_theta timed out!");
				thetaMotor->triggerErrorReportCb (BuildErrorInstance(
					instrument_error::motion_motor_positionfail, 
					instrument_error::motion_motor_instances::theta, 
					instrument_error::severity_level::error));
				ex.nextState = StageControllerOperationSequence::sc_timeout;
			}

			ex.executeNextState(HawkeyeError::eSuccess);
			break;
		}
		case StageControllerOperationSequence::sc_cancel:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "moveStageToPositionAsync : stage move cancelled!");

			// create a lambda to catch the status of the radius motor stop command
			auto cbRadiusStopforCancel = [ this, currentRPos ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : stage move cancel: radius stop command failed!");

					radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);
					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::radius, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			// create a lambda to catch the status of the theta motor stop command
			auto cbThetaStopforCancel = [ this, currentTPos ](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : stage move cancel: theta stop command failed!");

					thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);
					thetaMotor->triggerErrorReportCb (BuildErrorInstance(
						instrument_error::motion_motor_operationlogic, 
						instrument_error::motion_motor_instances::theta, 
						instrument_error::severity_level::error));
					reportErrorsAsync();
				}
				return;
			};

			// assume both motors may be moving
			radiusMotor->Stop(cbRadiusStopforCancel, true, getRadiusProperties().radiusFullTimeout_msec);	// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation
																											// set the complete to stop enum execution

			thetaMotor->Stop(cbThetaStopforCancel, true, getThetaProperties().thetaFullTimeout_msec);		// (true) 'hard' stop (no deceleration) when aborting/cancelling a move operation
																											// set the complete to stop enum execution
			ex.setComplete = true;
			break;
		}
		case StageControllerOperationSequence::sc_complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("moveStageToPositionAsync : <complete> Current Radius Position <%d> Current Theta Position <%d>") % currentRPos % currentTPos));

			// clear the previous errors occurred for motors since move operation is successful now
			properties_[getCurrentType()].thetaErrorCodes.clear();
			properties_[getCurrentType()].radiusErrorCodes.clear();
			properties_[getCurrentType()].probeErrorCodes.clear();

			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::AtPosition);
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::AtPosition);

			// set the complete to stop enum execution
			ex.setComplete = true;
			MoveClear();
			ex.executeNextState(HawkeyeError::eSuccess);

			return;
		}
		case StageControllerOperationSequence::sc_timeout:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : Operation timed out!");
		}
		// intentional fallthrough
		case StageControllerOperationSequence::sc_error:
		{
			if (curState == StageControllerOperationSequence::sc_error)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "moveStageToPositionAsync : error occurred!");
			}

			MoveClear();

			reportErrorsAsync(); // Motors motion is failed, report errors to user
		}
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("moveStageToPositionAsync : Current Radius Position <%d> Current Theta Position <%d>") % currentRPos % currentTPos));

			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentRPos, ePositionDescription::Current);
			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, currentTPos, ePositionDescription::Current);

			// set the complete to stop enum execution
			ex.setComplete = true;
			MoveClear();

			// set Hawkeye error to "eHardwareFault" to indicate that stage controller move is failed
			ex.executeNextState(HawkeyeError::eHardwareFault);
		}
	}
}

/// <summary>
/// performs a theta motor position read asynchronously
/// </summary>
/// <param name="posCb">Callback to indicate the read operation success/failure</param>
void StageController::readThetaPositionAsync(std::function<void(bool)> posCb)
{
	assert(posCb);

	Logger::L().Log (MODULENAME, severity_level::debug2, "readThetaPositionAsync: <enter>");

	// perform a controller read for the current theta position
	auto positionRdComplete = [this, posCb](bool rdStatus) -> void
	{
		if (!rdStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "readThetaPositionAsync: ReadPosition: unable to get motor position!");
		}

		posCb(true);
	};

	pCBOService_->enqueueInternal([=](void) -> void
	{
		boost::system::error_code error;
		thetaMotor->ReadPosition(positionRdComplete, error);
	});

	Logger::L().Log (MODULENAME, severity_level::debug2, "readThetaPositionAsync: <exit>");
}

/// <summary>
/// Gets the latest normalized theta motor positions asynchronously
/// </summary>
/// <param name="posCb">Callback to indicate caller</param>
/// <param name="waitForLatest">If <c>[true]</c> then fetch latest theta position; otherwise return stored value</param>
void StageController::getThetaPosition_DelayAsync(std::function<void(int32_t)> posCb, boost::asio::deadline_timer::duration_type delay)
{
	assert(posCb);

	auto delaytmr = std::make_shared<boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
	auto updateHost = [ this, posCb, delaytmr ](boost::system::error_code ec) -> void
	{
		int32_t tPos = this->GetThetaPosition();
		pCBOService_->enqueueInternal(posCb, tPos);
	};

	delaytmr->expires_from_now(delay);
	delaytmr->async_wait(updateHost);
}

/// <summary>
/// Gets the latest non-normalized theta motor position asynchronously
/// </summary>
/// <param name="posCb">Callback to indicate caller</param>
/// <param name="waitForLatest">If <c>[true]</c> then fetch latest theta position; otherwise return stored value</param>
void StageController::getRawThetaPosition_DelayAsync(std::function<void(int32_t)> posCb, boost::asio::deadline_timer::duration_type delay)
{
	assert(posCb);

	auto delaytmr = std::make_shared<boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
	auto updateHost = [ this, posCb, delaytmr ](boost::system::error_code ec) -> void
	{
		pCBOService_->enqueueInternal(posCb, thetaMotor->GetPosition());
	};

	delaytmr->expires_from_now(delay);
	delaytmr->async_wait(updateHost);
}

/// <summary>
/// Gets the latest theta and radius motor positions asynchronously
/// </summary>
/// <param name="posCb">Callback to indicate caller</param>
/// <param name="waitForLatest">If <c>[true]</c> then fetch latest theta position; otherwise return stored value</param>
void StageController::getThetaRadiusPosition_DelayAsync(std::function<void(int32_t, int32_t)> posCb, boost::asio::deadline_timer::duration_type delay)
{
	HAWKEYE_ASSERT (MODULENAME, posCb);

	// Not going to request a brand new position update.  Instead, delay for a short while and then
	// return the most recent data available.

	auto delaytmr = std::make_shared<boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
	auto updateHost = [ this, posCb, delaytmr ](boost::system::error_code ec) -> void
	{
		pCBOService_->enqueueInternal(posCb, GetThetaPosition(), GetRadiusPosition());
	};

	delaytmr->expires_from_now(delay);
	delaytmr->async_wait(updateHost);
}

/// <summary>
/// Gets the latest non-normalized theta motor position
/// </summary>
void StageController::getRawThetaPosition(int32_t & thetaPos)
{
	thetaPos = thetaMotor->GetPosition();
}

/// <summary>
/// Gets the latest theta and radius motor positions
/// </summary>
/// <param name="posCb">Callback to indicate caller</param>
void StageController::getThetaRadiusPosition(int32_t & thetaPos, int32_t & radiusPos)
{
	// Get the current position values with no delay
	thetaPos = GetThetaPosition();
	radiusPos = GetRadiusPosition();
}

/// <summary>
/// Gets the current stage controller type
/// </summary>
/// <returns>The current stage controller type</returns>
eCarrierType StageController::getCurrentType() const
{
	eCarrierType currentType = eCarrierType::ePlate_96;
	if (lastCarouselPresentStatus_.load())
	{
		currentType =eCarrierType::eCarousel;
	}
	return currentType;
}

/// <summary>
/// Gets the current stage controller properties <see cref="StageController::StageControllerProperties"/>
/// </summary>
/// <returns>The current stage controller properties</returns>
const StageController::StageControllerProperties& StageController::getCurrentProperties() const
{
	return properties_.at(getCurrentType());
}

/// <summary>
/// Gets the current stage controller theta properties <see cref="StageController::ThetaProperties"/>
/// </summary>
/// <returns>The current stage controller theta properties</returns>
const StageController::ThetaProperties& StageController::getThetaProperties() const
{
	return getCurrentProperties().thetaProperties;
}

/// <summary>
/// Gets the current stage controller radius properties <see cref="StageController::RadiusProperties"/>
/// </summary>
/// <returns>The current stage controller theta properties</returns>
const StageController::RadiusProperties& StageController::getRadiusProperties() const
{
	return getCurrentProperties().radiusProperties;
}

/// <summary>
/// Gets the current stage controller probe properties <see cref="StageController::ProbeProperties"/>
/// </summary>
/// <returns>The current stage controller theta properties</returns>
const StageController::ProbeProperties& StageController::getProbeProperties() const
{
	return getCurrentProperties().probeProperties;
}


/// <summary>
/// Gets the current stage controller default properties <see cref="StageController::StageControllerProperties"/>
/// </summary>
/// <param name="type">The stage controller type</param>
/// <returns>The current stage controller default properties</returns>
StageController::StageControllerProperties StageController::getDefaultProperties (eCarrierType type)
{
	auto properties = StageControllerProperties();

	properties.thetaProperties = getDefaultThetaProperties(type);
	properties.radiusProperties = getDefaultRadiusProperties(type);
	properties.probeProperties = getDefaultProbeProperties(type);
	properties.controllerOk = false;
	properties.initializedOk = false;
	properties.retriesCount = 0;

	return properties;
}

/// <summary>
/// Gets the current stage controller default theta properties <see cref="StageController::ThetaProperties"/>
/// </summary>
/// <param name="type">The stage controller type</param>
/// <returns>The current stage controller default theta properties</returns>
StageController::ThetaProperties StageController::getDefaultThetaProperties (eCarrierType type)
{
	ThetaProperties thetaProperties = {};

	// Common fields
	thetaProperties.thetaPositionTolerance = DefaultThetaPositionTolerance;
	thetaProperties.thetaStartTimeout_msec = MotorStartTimeout * TIMEOUT_FACTOR;
	thetaProperties.thetaFullTimeout_msec = ThetaFullTimeout * TIMEOUT_FACTOR;
	thetaProperties.thetaHomePos = 0;
	thetaProperties.thetaHomePosOffset = 0;
	thetaProperties.maxThetaPos = MaxThetaPosition;

	if (type ==eCarrierType::eCarousel)
	{
		thetaProperties.thetaCalPos = 0;
		thetaProperties.thetaBacklash = DefaultCarouselThetaBacklash;
	}
	else if (type == eCarrierType::ePlate_96)
	{
		thetaProperties.thetaCalPos = (int32_t) DefaultPlateThetaCalPos;
		thetaProperties.thetaBacklash = DefaultPlateThetaBacklash;
	}
	else
	{
		return ThetaProperties();
	}

	return thetaProperties;
}

/// <summary>
/// Gets the current stage controller default radius properties <see cref="StageController::RadiusProperties"/>
/// </summary>
/// <param name="type">The stage controller type</param>
/// <returns>The current stage controller default radius properties</returns>
StageController::RadiusProperties StageController::getDefaultRadiusProperties (eCarrierType type)
{
	RadiusProperties radiusProperties = {};

	radiusProperties.radiusMaxTravel = RadiusMaxTravel;
	radiusProperties.radiusStartTimeout_msec = MotorStartTimeout * TIMEOUT_FACTOR;
	radiusProperties.radiusFullTimeout_msec = RadiusFullTimeout * TIMEOUT_FACTOR;

	if (type == eCarrierType::eCarousel)
	{
		radiusProperties.radiusOffset = DefaultCarouselRadiusPos;
		radiusProperties.radiusCenterPos = 0;
		radiusProperties.radiusPositionTolerance = 0;
		radiusProperties.radiusBacklash = 0;
	}
	else if (type == eCarrierType::ePlate_96)
	{
		radiusProperties.radiusOffset = 0;
		radiusProperties.radiusCenterPos = 0;
		radiusProperties.radiusPositionTolerance = DefaultRadiusPositionTolerance;
		radiusProperties.radiusBacklash = DefaultPlateRadiusBacklash;
	}
	else
	{
		return RadiusProperties();
	}

	return radiusProperties;
}

/// <summary>
/// Gets the current stage controller default probe properties <see cref="StageController::ProbeProperties"/>
/// </summary>
/// <param name="type">The stage controller type</param>
/// <returns>The current stage controller default probe properties</returns>
StageController::ProbeProperties StageController::getDefaultProbeProperties (eCarrierType type)
{
	ProbeProperties probeProperties = {};
	if (type == eCarrierType::eUnknown)
	{
		return probeProperties;
	}

	probeProperties.probeHomePosOffset = 0;
	probeProperties.probeHomePos = 0;
	probeProperties.probePositionTolerance = DefaultProbePositionTolerance;
	probeProperties.probeStopPos = ProbeMaxTravel;
	probeProperties.probeBusyTimeout_msec = ProbeBusyTimeout * TIMEOUT_FACTOR;
	probeProperties.probeStartTimeout_msec = MotorStartTimeout * TIMEOUT_FACTOR;
	probeProperties.probeMaxTravel = ProbeMaxTravel;
	probeProperties.probeRetries = 0;

	return probeProperties;
}

/// <summary>
/// Reports the stored motor error codes asynchronously
/// </summary>
/// <param name="clearAfterReporting">Clear the stored error codes after reporting</param>
void StageController::reportErrorsAsync(bool clearAfterReporting)
{
	// We are just report the error asynchronously
	// Error has been already logged at the time of occurrence
	bool logDescription = false;

	pCBOService_->enqueueInternal([ = ]() -> void
	{
		reportErrors(logDescription, clearAfterReporting);
	});
}

/// <summary>
/// Reports the stored motor error codes
/// </summary>
/// <param name="logDescription">If set to <c>[true]</c> Logs the error description</param>
/// <param name="clearAfterReporting">Clear the stored error codes after reporting</param>
void StageController::reportErrors(bool logDescription, bool clearAfterReporting)
{
	// Theta motor errors
	for (const uint32_t& code : getCurrentProperties().thetaErrorCodes)
	{
		ReportSystemError::Instance().ReportError(code, logDescription);
	}

	// Radius motor errors
	for (const uint32_t& code : getCurrentProperties().radiusErrorCodes)
	{
		ReportSystemError::Instance().ReportError(code, logDescription);
	}

	// Probe motor errors
	for (const uint32_t& code : getCurrentProperties().probeErrorCodes)
	{
		ReportSystemError::Instance().ReportError(code, logDescription);
	}

	if (clearAfterReporting)
	{
		properties_[getCurrentType()].thetaErrorCodes.clear();
		properties_[getCurrentType()].radiusErrorCodes.clear();
		properties_[getCurrentType()].probeErrorCodes.clear();
	}
}

void StageController::PauseMotorPolling()
{
	probeMotor->PausePositionUpdates();
	radiusMotor->PausePositionUpdates();
	thetaMotor->PausePositionUpdates();
	Logger::L ().Log (MODULENAME, severity_level::debug1, "PauseMotorPolling: <exit>");
}

void StageController::ResumeMotorPolling()
{
	probeMotor->ResumePositionUpdates (250);
	radiusMotor->ResumePositionUpdates (250);
	thetaMotor->ResumePositionUpdates (250);

	Logger::L ().Log (MODULENAME, severity_level::debug1, "ResumeMotorPolling: <exit>");
}
