#include "stdafx.h"

#include "GetAsStrFunctions.hpp"
#include "StageControllerBase.hpp"

static const char MODULENAME[] = "StageControllerBase";

StageControllerBase::StageControllerBase(
	std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(std::move(pCBOService))
	, stageControllerInitialized_(false)
	, cancelMove_(false)
	, cancelThetaMove_(false)
	, cancelRadiusMove_(false)
	, canSkipMotorCalibrationCheck_(false)
	// Set the slot of interest to 1 to indicate that there is a tube in that slot.
	//                             1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
	, carouselSlotsForSimulation_{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
{
	// initialize post command helper
	asyncCommandHelper_ = std::make_shared<AsyncCommandHelper>(pCBOService_->getInternalIos());
}

StageControllerBase::~StageControllerBase()
{
	canSkipMotorCalibrationCheck_ = false;

	thetaMotorStatus_.reset();
	radiusMotorStatus_.reset();
	probeMotorStatus_.reset();

	asyncCommandHelper_.reset();
}

void StageControllerBase::Init (std::string port, t_pPTree cfgTree,
                               bool apply, std::string cfgFile,
                               std::function<void(bool, bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::function<void(bool)> tempCallback = [this, callback](bool status)
	{
		callback(status, this->carouselTubeFoundDuringInit());
	};

	auto wrappedCallback = wrapCallback(tempCallback);
	pCBOService_->enqueueInternal([=]()
	{
		initAsync (wrappedCallback, port, cfgTree, apply, cfgFile);
	});
}

void StageControllerBase::SelectStage(std::function<void(bool)> callback, eCarrierType type)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback, type]()
	{
        MoveClear();
        selectStageAsync(wrappedCallback, type);
	});
}

void StageControllerBase::ClearErrors(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		clearErrorsAsync(wrappedCallback);
	});
}

void StageControllerBase::Stop(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

    CancelMove( callback );
}

bool StageControllerBase::MoveSetup (eCarrierType& type, bool skipCalCheck)
{
    bool setupOk = false;
    eCarrierType carrier = type;

    MoveClear();

    if ( type == eCarrierType::eUnknown )
    {
        carrier = eCarrierType::ePlate_96;
        if ( checkCarouselPresent() )
        {
            carrier = eCarrierType::eCarousel;
        }
        type = carrier;
    }

    if ( skipCalCheck )
    {
        return true;
    }

    if ( !IsStageCalibrated( carrier ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, 
			boost::str (boost::format ("MoveSetup : Failed <Stage is not Calibrated for %s>") % getCarrierTypeAsStr(carrier)));
    }
    else
    {
        setupOk = true;
    }

    return setupOk;
}

void StageControllerBase::MoveClear(void)
{
    cancelMove_ = false;
    cancelRadiusMove_ = false;
    cancelThetaMove_ = false;
}

void StageControllerBase::CancelMove(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	cancelMove_ = true;
	cancelRadiusMove_ = true;
	cancelThetaMove_ = true;

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		stopAsync(wrappedCallback);
	});
}

void StageControllerBase::GetMotorStatus(MotorStatus& thetaMotorStatus,
										 MotorStatus& radiusMotorStatus,
										 MotorStatus& probeMotorStatus)
{
	thetaMotorStatus_.ToCStyle(thetaMotorStatus);
	radiusMotorStatus_.ToCStyle(radiusMotorStatus);
	probeMotorStatus_.ToCStyle(probeMotorStatus);
}

void StageControllerBase::ProbeUp(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	
	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		probeUpAsync(wrappedCallback);
	});
}

void StageControllerBase::ProbeStepUp(std::function<void(bool)> callback, int32_t moveStep)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback, moveStep]()
	{
		probeStepUpAsync(wrappedCallback, moveStep);
	});
}

bool StageControllerBase::IsProbeUp()
{
	return isProbeUp();
}

void StageControllerBase::ProbeDown(std::function<void(bool)> callback, bool downOnInvalidStagePos)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback, downOnInvalidStagePos]()
	{
		probeDownAsync(wrappedCallback, downOnInvalidStagePos);
	});
}

void StageControllerBase::ProbeStepDn(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback, moveStep, downOnInvalidStagePos]()
	{
		probeStepDnAsync(wrappedCallback, moveStep, downOnInvalidStagePos);
	});
}

bool StageControllerBase::IsProbeDown()
{
	return isProbeDown();
}

int32_t StageControllerBase::GetProbePosition()
{
	return getProbePosition();
}

void StageControllerBase::GetStageBacklashValues(int32_t& radiusBacklash, int32_t& thetaBacklash)
{
	radiusBacklash = getRadiusBacklash();
	thetaBacklash = getThetaBacklash();
}

void StageControllerBase::CalibrateMotorsHome(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		// Skip motor calibration check if calibration is in progress
		setCanSkipMotorCalibrationCheck(true);
		calibrateMotorsHomeAsync(wrappedCallback);
	});
}

void StageControllerBase::CalibrateStageRadius(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		// Skip motor calibration check if calibration is in progress
		setCanSkipMotorCalibrationCheck(true);
		calibrateStageRadiusAsync(wrappedCallback);
	});
}

void StageControllerBase::CalibrateStageTheta(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		// Skip motor calibration check if calibration is in progress
		setCanSkipMotorCalibrationCheck(true);
		calibrateStageThetaAsync(wrappedCallback);
	});
}

void StageControllerBase::FinalizeCalibrateStage(bool cancelCalibration, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
	pCBOService_->enqueueInternal([this, wrappedCallback, cancelCalibration]()
	{
		// Reset "SkipMotorCalibrationCheck" flag
		setCanSkipMotorCalibrationCheck(false);
		finalizeCalibrateStageAsync(wrappedCallback, cancelCalibration);
	});
}

void StageControllerBase::FindFirstTube(std::function<void(bool)> callback, boost::optional<uint32_t> finalTubePosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	
	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eCarousel;
    if (!MoveSetup(type))
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "FindFirstTube : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

	pCBOService_->enqueueInternal([this, wrappedCallback, finalTubePosition]()
	{
		findTubeAsync(wrappedCallback, FindTubeMode::FIRST, finalTubePosition);
	});
}

void StageControllerBase::FindNextTube(std::function<void(bool)> callback, boost::optional<uint32_t> finalTubePosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eCarousel;
    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "FindNextTube : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

	pCBOService_->enqueueInternal([this, wrappedCallback, finalTubePosition]()
	{
		findTubeAsync(wrappedCallback, FindTubeMode::NEXT, finalTubePosition);
	});
}

void StageControllerBase::GotoNextTube(std::function<void(int32_t)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eCarousel;
    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "GotoNextTube : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, -1 ) );
        return;
    }

	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		goToNextTubeAsync([=](boost::optional<uint32_t> opTubeNumber)
		{
			int32_t tubeNumber = static_cast<int32_t>(opTubeNumber.get_value_or(-1));
			pCBOService_->enqueueInternal(wrappedCallback, tubeNumber);
		});
	});
}

/// Finds the tube in carousel stepping through each position
/// Returns a function handler, which is used to stop the searching/finding for tubes during additional sample search
std::function<void(void)> StageControllerBase::FindTubePosByPos (boost::optional<uint32_t> finalTubePosition,
                                                                std::function<void(uint32_t)> stageMoveCallback,
                                                                std::function<void(bool)> completionCallback)
{
	HAWKEYE_ASSERT (MODULENAME, completionCallback);
	HAWKEYE_ASSERT (MODULENAME, stageMoveCallback);

	auto wrappedCompletionCallback = wrapCallback(completionCallback); 
	const auto wrappedStageMoveCallback = wrapCallback(stageMoveCallback);

    eCarrierType type = eCarrierType::eCarousel;
    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "FindTubePosByPos : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCompletionCallback, false ) );
        return nullptr;
    }

	auto pSearchNext = std::make_shared<bool>(true);
	uint32_t startingTubePos = getNearestTubePosition();

	findTubeInternal (pSearchNext, startingTubePos, finalTubePosition, wrappedStageMoveCallback, [=](bool status)
	{
		if (cancelMove_)
		{
			wrappedCompletionCallback (false);
			return;
		}

		//Report tube Not found when the finalTubePosition is NOT DEFINED.
		if (!status && !finalTubePosition)
		{
			auto currentTubePos = getNearestTubePosition();

			//Move the stage to position 1, when no additional samples found and report if any tubes found in this round.
			findTubeInternal (pSearchNext, currentTubePos, 1, wrappedStageMoveCallback, [=](bool status)
			{ 
				wrappedCompletionCallback (status);
			});
			return;
		}

		//Report tube found status  when the finalTubePosition is DEFINED.
		wrappedCompletionCallback (status);
	});

	return [pSearchNext]() -> void
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "FindTubePosByPos : pausing tube search");
		*pSearchNext = false;
	};
}

void StageControllerBase::SetStageProfile(std::function<void(bool)> onCompletion, bool enable)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("SetStageProfile: enabled: %s") % (enable ? "true" : "false")));

	auto wrappedCompletionCallback = wrapCallback(onCompletion);

	auto setProfileComplete = [=](bool status) mutable
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("SetStageProfile: completion status: %s") % (status ? "true" : "false")));
		onCompletion(status);
	};

	pCBOService_->enqueueInternal([=]()
	{
		setStageProfile(setProfileComplete, enable);
	});
}

bool StageControllerBase::IsTubePresent()
{
	return checkTubePresent();
}

bool StageControllerBase::IsCarouselPresent()
{
	return checkCarouselPresent();
}

uint32_t StageControllerBase::GetNearestTubePosition()
{
	return getNearestTubePosition();
}

void StageControllerBase::IsPlatePresent(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::ePlate_96;
    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "IsPlatePresent : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

    pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		isPlatePresentAsync(wrappedCallback);
	});
}

void StageControllerBase::HomeStage(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

    auto wrappedCallback = wrapCallback( callback );
	pCBOService_->enqueueInternal([this, wrappedCallback]()
	{
		homeStageAsync(wrappedCallback);
	});
}

void StageControllerBase::EjectStage(std::function<void(bool)> callback, int32_t angle )
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);
    pCBOService_->enqueueInternal([this, wrappedCallback, angle]()
	{
		ejectStageAsync(wrappedCallback, angle );
	});
}

void StageControllerBase::MoveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = position.getcarrierType();
    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "MoveStageToPosition : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

	pCBOService_->enqueueInternal([this, wrappedCallback, position]()
	{
		if (cancelMove_)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "MoveStageToPosition: operation cancelled");
			pCBOService_->enqueueInternal(std::bind(wrappedCallback, false));
			return;
		}

		moveStageToPosition(wrappedCallback, position);
	});
}

SamplePositionDLL StageControllerBase::GetStagePosition()
{
	SamplePositionDLL pos = {};
	getStagePosition(pos);
	return pos;
}

void StageControllerBase::getDefaultStagePosition(std::function<void(SamplePositionDLL)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eUnknown;

    if ( !MoveSetup(type) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "getDefaultStagePosition : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, SamplePositionDLL() ) );
        return;
    }

	// If carousel is not present
	if (type == eCarrierType::ePlate_96)
	{
		//Initialize stage if not already initialized
		selectStageAsync([this, wrappedCallback](bool status)
		{
			if (cancelMove_)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultStagePosition: operation cancelled");
				pCBOService_->enqueueInternal(wrappedCallback, GetStagePosition());
				return;
			}

			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "getDefaultStagePosition: Unable to select plate controller");
				pCBOService_->enqueueInternal(wrappedCallback, SamplePositionDLL());
				return;
			}

			Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultStagePosition: Finding plate top");
			isPlatePresentAsync([=](bool plateOk)
			{
				if (cancelMove_)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultStagePosition: operation cancelled");
					pCBOService_->enqueueInternal(wrappedCallback, GetStagePosition());
					return;
				}

				SamplePositionDLL position = SamplePositionDLL();
				if (!plateOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "getDefaultStagePosition: No plate found!");
				}
				else
				{
					// Set the default position of plate (i.e. A1)
					position.setRowColumn(eStageRows::PlateRowA, 1);
				}
				pCBOService_->enqueueInternal(wrappedCallback, position);
			});
		}, type);
		return;
	}

	//Initialize stage if not already initialized
	selectStageAsync([this, wrappedCallback](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultStagePosition: Unable to select carousel controller");
			pCBOService_->enqueueInternal(wrappedCallback, SamplePositionDLL());
			return;
		}
		
		findTubeAsync([=](bool tubeFound)
		{
			SamplePositionDLL endPos = GetStagePosition();

			if (cancelMove_ || !tubeFound)
			{

				if (cancelMove_)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultStagePosition: operation cancelled");
				}
				else
				{
                    Logger::L().Log ( MODULENAME, severity_level::debug1, "getDefaultStagePosition: Unable to find tube" );
					endPos = SamplePositionDLL();
				}

                pCBOService_->enqueueInternal( wrappedCallback, endPos );
                return;
			}

            Logger::L().Log ( MODULENAME, severity_level::debug1, "getDefaultStagePosition: tube found" );
			pCBOService_->enqueueInternal(wrappedCallback, endPos);
			return;
		}, FindTubeMode::FIRST, boost::none);
		
	}, type);
}

void StageControllerBase::moveToDefaultStagePosition(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eUnknown;

    if ( !MoveSetup( type ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "moveToDefaultStagePosition : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

	getDefaultStagePosition([this, wrappedCallback, type](SamplePositionDLL pos)
	{
		if (cancelMove_)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "moveToDefaultStagePosition: operation cancelled");
			pCBOService_->enqueueInternal(std::bind(wrappedCallback, false));
			return;
		}

		bool posValid = false;
		if (type == eCarrierType::eCarousel)
		{
			posValid = pos.isValidForCarousel();
		}
		else if (type == eCarrierType::ePlate_96)
		{
			posValid = pos.isValidForPlate96();
		}

		if (!posValid)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "moveToDefaultStagePosition: returned invalid position");
			pCBOService_->enqueueInternal(std::bind(wrappedCallback, false));
			return;
		}
		moveStageToPosition(wrappedCallback, pos);
	});
}

// A dedicated API to be called from service level
void StageControllerBase::MoveStageToServicePosition(std::function<void(bool)> callback, SamplePositionDLL position)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCallback = wrapCallback(callback);

    eCarrierType type = eCarrierType::eUnknown;

    if ( !MoveSetup( type, true ) )
    {
        Logger::L().Log ( MODULENAME, severity_level::notification, "MoveStageToServicePosition : Move setup failed" );
        pCBOService_->enqueueInternal( std::bind( wrappedCallback, false ) );
        return;
    }

	// For service call skip the motor registration check
	setCanSkipMotorCalibrationCheck(true);
	MoveStageToPosition([this, wrappedCallback](bool moveOk)
	{
		pCBOService_->enqueueInternal([=]()
		{
			enableStageHoldingCurrent([=](bool) -> void {}, false);	// disable holding currents after any service move operation
			setCanSkipMotorCalibrationCheck(false);
			wrappedCallback(moveOk);
		});
	}, position);
}

bool StageControllerBase::IsStageCalibrated(eCarrierType sample_carrier)
{
	auto calibration_status = isStageCalibrated();

	if (calibration_status == CalibrationConfigState::ePlateAndCarousel)
		return true;

	auto error_instance = instrument_error::motion_sample_deck_instances::general;

	if (sample_carrier == eCarrierType::eCarousel)
	{
		if (calibration_status == CalibrationConfigState::eOnlyCarousel)
			return true;

		error_instance = instrument_error::motion_sample_deck_instances::carousel;
	}
	else if (sample_carrier == eCarrierType::ePlate_96)
	{
		if (calibration_status == CalibrationConfigState::eOnlyPlate)
			return true;

		error_instance = instrument_error::motion_sample_deck_instances::plate;
	}

	ReportSystemError::Instance().ReportError( BuildErrorInstance(
		instrument_error::motion_sampledeck_notcalibrated,
		error_instance, 
		instrument_error::severity_level::error));

	return false;
}

void StageControllerBase::findTubeInternal(std::shared_ptr<bool> pSearchNext,
										   uint32_t currentTubePosition,
										   boost::optional<uint32_t> finalTubePosition,
                                           std::function<void(uint32_t)> stageMoveCallback,
                                           std::function<void(bool)> completionCallback)
{
	HAWKEYE_ASSERT (MODULENAME, completionCallback);
	HAWKEYE_ASSERT (MODULENAME, stageMoveCallback);

	Logger::L().Log (MODULENAME, severity_level::debug1, 
		boost::str(boost::format("findTubeInternal: current position (%d), finalPosition(%d)")
			% currentTubePosition % finalTubePosition.get_value_or(0)));

	auto findTubeComplete = [this, completionCallback, stageMoveCallback, pSearchNext, currentTubePosition, finalTubePosition](bool tubeFound)mutable
	{
		uint32_t tubePos = getNearestTubePosition();
		Logger::L().Log (MODULENAME, severity_level::debug2, "findTubeInternal::findTubeComplete: tubePos: " + std::to_string(tubePos));

		pCBOService_->enqueueInternal(stageMoveCallback, tubePos);

		if (cancelMove_)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "findTubeInternal : operation cancelled");
			pCBOService_->enqueueInternal(completionCallback, true);
			return;
		}

		if (tubeFound)
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "findTubeInternal : tube found at column : " + std::to_string(tubePos));
			pCBOService_->enqueueInternal(completionCallback, true);
			return;
		}

		if (pSearchNext && *pSearchNext == false)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "findTubeInternal : Pausing the tube search mode");
			pCBOService_->enqueueInternal(completionCallback, false);
			return;
		}

		if (finalTubePosition)
		{
			if (currentTubePosition != tubePos)				// position has changed...  may have been moved manually
			{												// update the end of search position to ensure all tube positions are checked
				finalTubePosition = tubePos;
			}

			if (tubePos == finalTubePosition.get())
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "findTubeInternal : No tube found till column : " + std::to_string(finalTubePosition.get()));
				pCBOService_->enqueueInternal(completionCallback, false);
				return;
			}
		}
		else
		{
			// There is no defined tube location to go, it means search for additional samples
			// End tube position for additional samples is "CurrentTubePosition".
			finalTubePosition = finalTubePosition.get_value_or(currentTubePosition);
		}

		SamplePositionDLL nextTubePos(eStageRows::CarouselRow, static_cast<uint8_t>(tubePos));
		nextTubePos.incrementColumn();
		findTubeInternal (pSearchNext, nextTubePos.getColumn(), finalTubePosition, stageMoveCallback, completionCallback);
	};

	pCBOService_->enqueueInternal([=]()
	{
		findTubeAsync(findTubeComplete, FindTubeMode::FIRST, currentTubePosition);
	});
}

void StageControllerBase::setCanSkipMotorCalibrationCheck(bool canSkip)
{
	canSkipMotorCalibrationCheck_ = canSkip;
}

bool StageControllerBase::canSkipMotorCalibrationCheck()
{
	return canSkipMotorCalibrationCheck_.load();
}

void StageControllerBase::PauseMotorPolling()
{
}

void StageControllerBase::ResumeMotorPolling()
{
}