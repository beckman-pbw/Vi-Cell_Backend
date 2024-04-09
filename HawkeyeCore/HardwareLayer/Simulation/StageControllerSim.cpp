#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include "StageControllerSim.hpp"
#include "SystemErrors.hpp"
#include "HawkeyeConfig.hpp"

static std::string MODULENAME = "StageControllerSim";

const int kINIT_TIME = 250;
const int kSTOP_TIME = 250;
const int kHOME_MOVE_TIME = 1000;
const int kSTAGE_EJECT_TIME = 3000;

const int kTUBE_MOVE_TIME = 900;
const int kPLATE_POS_TIME = 250;
const int kPLATE_DETECT_TIME = 6500;

const int kPROBE_DOWN_TIME = 3000;
const int kPROBE_UP_TIME = 1500;

const int kSHORT_DELAY_TIME = 15;

const int kSMALL_RAND_TIME = 25;
const int kMED_RAND_TIME = 500;
const int kBIG_RAND_TIME = 1500;

/**
 ***************************************************************************
 * \brief Block for a minimum amount of time and optionally add a random amount
 * \param min_ms - minimum number of milli-seconds to block for
 * \param rand_ms - a uniformly random amount of time up to this value is added to the delay
 */
static void Delay(uint32_t min_ms, uint32_t rand_ms)
{
	const int kSLEEP_TIME = 10;

	auto delayT = min_ms;
	if (delayT < kSLEEP_TIME)
		delayT = kSLEEP_TIME;

	if (rand_ms > 0)
		delayT += (rand() % rand_ms);

	// Fine tune the delay value by adjusting it by a percentage
#ifdef _DEBUG
	delayT /= 4;
#else
	delayT *= 8;
	delayT /= 10;
#endif

	// We want this to be a busy wait
	// this code in intentionally ineffecient 
	while (delayT > 0)
	{
		Sleep(kSLEEP_TIME);
		if (delayT <= kSLEEP_TIME)
			delayT = 0;
		else
			delayT -= kSLEEP_TIME;
	}
}


StageControllerSim::StageControllerSim(std::shared_ptr<CBOService> pCBOService)
	: StageControllerBase(pCBOService)
{
	probeUp_ = false;
	probePos_ = 0;
	radiusBacklash_ = 0;
	thetaBacklash_ = 0;
	isStageCalibrated_ = false;
	currentPos_ = SamplePositionDLL();
	currentType_ = eCarrierType::eUnknown;
	thetaPos_ = 0;
	radiusPos_ = 0;
	srand(static_cast<unsigned int>(time(NULL)));
}

StageControllerSim::~StageControllerSim()
{
	Quit();
}

void StageControllerSim::initAsync(std::function<void(bool)> cb, std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile)
{
	HAWKEYE_ASSERT (MODULENAME, cb);

	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);

	Delay(kINIT_TIME, kSMALL_RAND_TIME);

	auto cb_internal = [this, cb](bool status)
	{
		if (status)
		{
			probeUp_ = true;
			probePos_ = 0;

			thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, thetaPos_, ePositionDescription::Unknown);
			radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, radiusPos_, ePositionDescription::Unknown);
			probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, probePos_, ePositionDescription::Unknown);
		}
		pCBOService_->enqueueInternal(cb, status);
	};

	eCarrierType type = HawkeyeConfig::Instance().get().isSimulatorCarousel ? eCarrierType::eCarousel : eCarrierType::ePlate_96;
	selectStageAsync(cb_internal, type);
}

void StageControllerSim::Reinit()
{
	if (!ControllerOk())
	{
		return;
	}
	Delay(kSHORT_DELAY_TIME, kMED_RAND_TIME);
	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, thetaPos_, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, radiusPos_, ePositionDescription::Current);
	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, probePos_, ePositionDescription::Current);
}

void StageControllerSim::Quit(void)
{
	//Reset();
}

bool StageControllerSim::ControllerOk()
{
	return true;
}

void StageControllerSim::selectStageAsync(std::function<void(bool)> cb, eCarrierType type)
{
    MoveClear();

	currentType_ = type;

	switch (type)
	{		
		case eCarrierType::eCarousel:
		{
			moveStageToPosition (cb, SamplePositionDLL(eStageRows::CarouselRow, 1));
			break;
		}
		case eCarrierType::ePlate_96:
		{
			moveStageToPosition (cb, SamplePositionDLL(eStageRows::PlateRowA, 1));
			break;
		}

		case eCarrierType::eACup:
		{
			break;
		}

		case eCarrierType::eUnknown:
		default:
		{
			moveStageToPosition (cb, SamplePositionDLL());
			break;
		}
	}
}

bool StageControllerSim::carouselTubeFoundDuringInit()
{
	return false;
}

void StageControllerSim::getStagePosition(SamplePositionDLL& position)
{
	Delay(kSHORT_DELAY_TIME, kSMALL_RAND_TIME);
	position = currentPos_;
}

StageControllerSim::CalibrationConfigState StageControllerSim::isStageCalibrated()
{
	// for simulation "isStageCalibrated_" is always true
	isStageCalibrated_ = true;

	if (isStageCalibrated_ || canSkipMotorCalibrationCheck())
		return CalibrationConfigState::ePlateAndCarousel;

	return CalibrationConfigState::eNone;
}

uint32_t StageControllerSim::getNearestTubePosition(void)
{
	Delay(kSHORT_DELAY_TIME, kSMALL_RAND_TIME);
	if (!checkCarouselPresent())
	{
		return 0;
	}

	if ( ( currentType_ == eCarrierType::eCarousel ) &&
		 ( !currentPos_.isValidForCarousel() ) )
	{
		double_t thetaAngle = ( thetaPos_ / 10 );
		int32_t tNum = (int32_t) ( (int32_t) ( thetaAngle / DefaultDegreesPerTube ) % (int32_t) ( 360 / DefaultDegreesPerTube ) ) + 1;
		currentPos_.setRowColumn( eStageRows::CarouselRow, static_cast<uint8_t>(tNum) );
	}

	return currentPos_.getColumn();
}

void StageControllerSim::updateRadiusThetaPos()
{
	Delay(kSHORT_DELAY_TIME, kSMALL_RAND_TIME);

	if (currentType_ == eCarrierType::eCarousel)
	{
		if (currentPos_.isValidForCarousel())
		{
			int32_t tPos, rPos;
			getCarouselPostionFromRowCol(currentPos_.getColumn(), tPos, rPos);

			updateCarouselStatus(tPos, rPos);
			thetaPos_ = tPos;		// preserve for post-eject recalculation of 'position'
			radiusPos_ = rPos;
		}
		else
		{
			updateCarouselStatus(-1, -1);
		}
	}
	else if (currentType_ == eCarrierType::ePlate_96)
	{
		uint32_t rowNum = 0;
		rowNum = (currentPos_.getRow() - eStageRows::PlateRowA) + 1;
		if (rowNum > MaxPlateRowNum)
		{
			rowNum = 0;
		}
		uint32_t colNum = currentPos_.getColumn();
		int32_t tPos, rPos;
		getPlatePostionFromRowCol(rowNum - 1, colNum - 1, tPos, rPos);

		updatePlateStatus(tPos, rPos);
	}
	else
	{
		thetaMotorStatus_.reset();
		radiusMotorStatus_.reset();
	}
}

bool StageControllerSim::getCarouselPostionFromRowCol(uint32_t tgtCol, int32_t& thetaPos, int32_t& radiusPos)
{
	double_t thetaAngle = 0.0;
	int32_t tPos = 0;
	uint32_t tube = tgtCol;
	int32_t maxCarouselPos = MaxThetaPosition;

	while (tube <= 0)
	{
		tube += MaxCarouselTubes;
	}
	tube--;     // convert to index

	tube %= MaxCarouselTubes;

	thetaAngle = (tube * DefaultDegreesPerTube);			// assume tube 1 is at the home position with home = 0

	tPos = (int32_t)(thetaAngle * 10);						// user units are in 0.1 degrees, 3600 user units per full final revolution

	while (tPos < 0)
	{
		tPos += maxCarouselPos;
	}
	while (tPos >= maxCarouselPos)
	{
		tPos -= maxCarouselPos;
	}

	thetaPos = tPos;
	radiusPos = DefaultCarouselRadiusPos;

	return true;
}

bool StageControllerSim::getPlatePostionFromRowCol(uint32_t tgtRow, uint32_t tgtCol, int32_t & thetaPos, int32_t & radiusPos) const
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
	int32_t plateWellSpacing = DefaultInterWellSpacing;

	deltaRow = ((MaxPlateRowNum / 2) - tgtRow);             // for calculations use the row as the x value
	deltaCol = ((MaxPlateColNum / 2) - tgtCol);             // for calculations use the col as the y value

															// account for the fact that even rows and columns are offset from the x and y axes
	deltaX = (double)(deltaRow)-0.5;
	deltaY = (double)(deltaCol)-0.5;

	// NOTE: all radius-related constants are specified in user units (1/10 micron);
	// the final radius offset values do not need to be corrected to user units
	xVal = (deltaX * plateWellSpacing);
	yVal = (deltaY * plateWellSpacing);

	angleRadians = atan2(yVal, xVal);
	angleDegrees = (angleRadians * 180.0 / M_PI);             // convert to degrees

															  // correct for quadrant
	if (tgtCol >= 6)
	{
		angleDegrees += 360;
	}
	userAngle = (int32_t)(angleDegrees * 10);                 // convert to user units (1/10 degree, integer value);

	xSquared = (xVal * xVal);
	ySquared = (yVal * yVal);
	radiusSquared = xSquared + ySquared;
	radiusOffset = (int32_t)sqrtl(radiusSquared);

	if ((radiusOffset < RadiusMaxTravel) &&
		(userAngle >= 0) && (userAngle <= 3600))
	{
		radiusPos = radiusOffset;
		thetaPos = userAngle;
	} else
	{
		calcOk = false;
	}
	return calcOk;
}

void StageControllerSim::updateCarouselStatus(const int32_t& tPos, const int32_t& rPos)
{
	if (tPos > 0)
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, tPos, ePositionDescription::Current);
	} 
	else if (tPos == 0)
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, tPos, ePositionDescription::Home);
	}
	else
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, tPos, ePositionDescription::Unknown);
	}

	if (rPos > 0)
	{
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, rPos, ePositionDescription::Current);
	} 
	else if (rPos == 0)
	{
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, rPos, ePositionDescription::Home);
	} 
	else
	{
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfErrorState, rPos, ePositionDescription::Unknown);
	}
}

void StageControllerSim::updatePlateStatus(const int32_t & tPos, const int32_t & rPos)
{
	if (tPos != 0)
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, tPos, ePositionDescription::Current);
	} 
	else
	{
		thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, tPos, ePositionDescription::Home);
	}

	if (rPos != 0)
	{
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, rPos, ePositionDescription::Current);
	} 
	else
	{
		radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, rPos, ePositionDescription::Home);
	}
}

void StageControllerSim::probeUpAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	Delay(kPROBE_UP_TIME, kBIG_RAND_TIME);

	probeUp_ = true;
	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, probePos_, ePositionDescription::Home);

	pCBOService_->enqueueInternal(callback, isProbeUp());
}

void StageControllerSim::probeDownAsync(std::function<void(bool)> callback, bool downOnInvalidStagePos)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	Delay(kPROBE_DOWN_TIME, kBIG_RAND_TIME);

	probeUp_ = false;
	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, probePos_, ePositionDescription::Current);

	pCBOService_->enqueueInternal(callback, isProbeDown());
}

void StageControllerSim::probeStepUpAsync(std::function<void(bool)> callback, int32_t moveStep)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if ((moveStep != 0))
	{
		if (moveStep > 0)
		{
			moveStep = -moveStep;
		}
		probePos_ += moveStep;
		probeUpAsync([=](bool status) { pCBOService_->enqueueInternal(callback, true); });
	}
	else
	{
		pCBOService_->enqueueInternal(callback, true);
	}
}

void StageControllerSim::probeStepDnAsync(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if ((moveStep != 0))
	{
		if (moveStep < 0)
		{
			moveStep = -moveStep;
		}
		probePos_ += moveStep;
		probeDownAsync([=](bool status) { pCBOService_->enqueueInternal(callback, true); }, downOnInvalidStagePos);
	}
	else
	{
		pCBOService_->enqueueInternal(callback, true);
	}
}

bool StageControllerSim::isProbeUp()
{
	return probeUp_;
}

bool StageControllerSim::isProbeDown()
{
	return !probeUp_;
}

int32_t StageControllerSim::getProbePosition()
{
	return probePos_;
}

int32_t StageControllerSim::getRadiusBacklash()
{
	return radiusBacklash_;
}

int32_t StageControllerSim::getThetaBacklash()
{
	return thetaBacklash_;
}

void StageControllerSim::calibrateMotorsHomeAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	homeStageAsync(callback);
}

void StageControllerSim::calibrateStageRadiusAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(callback, true);
}

void StageControllerSim::calibrateStageThetaAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(callback, true);
}

void StageControllerSim::finalizeCalibrateStageAsync(std::function<void(bool)> callback, bool cancelCalibration)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!cancelCalibration)
	{
		updateRadiusThetaPos();
		isStageCalibrated_ = true;
	}
	pCBOService_->enqueueInternal(callback, true);
}

bool StageControllerSim::checkTubePresent()
{
	updateRadiusThetaPos();
	if (currentType_ == eCarrierType::eCarousel)
	{
		return true;
	}
	return false;
}

bool StageControllerSim::checkCarouselPresent()
{
	return currentType_ == eCarrierType::eCarousel;
}

//*****************************************************************************
void StageControllerSim::findTubeAsync (std::function<void(bool)> callback, FindTubeMode mode, boost::optional<uint32_t> finalTubePosition)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto startpos = currentPos_.getColumn();
	auto endpos = finalTubePosition.get();
	auto skips = 0;
	if (endpos >= startpos) 
	{
		skips = endpos - startpos;
	}
	else
	{
		skips = 24 - startpos;
		skips += endpos;		
	}
	Delay((skips * kTUBE_MOVE_TIME), kMED_RAND_TIME);

	auto testCarouselSlot = [this]()
	{
		bool found = false;

		std::lock_guard<std::mutex> m{ carouselSlotsForSimulationMutex_ };

		if ( !currentPos_.isValidForCarousel() )
		{
			double_t thetaAngle = ( thetaPos_ / 10 );
			int32_t tNum = (int32_t) ( (int32_t) ( thetaAngle / DefaultDegreesPerTube ) % (int32_t) ( 360.0 / DefaultDegreesPerTube ) ) + 1;
			currentPos_.setRowColumn( eStageRows::CarouselRow, static_cast<uint8_t>(tNum) );
		}

		uint8_t curCol = static_cast<uint8_t>(currentPos_.getColumn());
		currentPos_.setRowColumn( eStageRows::CarouselRow, curCol );
		if ( curCol > 0 )
		{
			uint32_t colIdx = curCol - 1;
			if (carouselSlotsForSimulation_[colIdx])
			{
				found = true;
				carouselSlotsForSimulation_[colIdx] = 0;
			}
		}

		std::string logStr = (boost::str(boost::format("findTubeAsync: tube found: %s") % ((found == true) ? "yes" : "no")));
		Logger::L().Log (MODULENAME, severity_level::debug1, logStr);

		return found;
	};

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("findTubeAsync: current position (%d), finalPosition(%d)")
		% currentPos_.getColumn() % finalTubePosition.get_value_or(0)));

	if (finalTubePosition)
	{
		if (currentPos_.getColumn() != finalTubePosition.get())
		{
			currentPos_.incrementColumn();
			currentPos_.setRowColumn( eStageRows::CarouselRow, static_cast<uint8_t>(currentPos_.getColumn()) );
			updateRadiusThetaPos();
		}
	}

	pCBOService_->enqueueInternal (callback, testCarouselSlot());
}

//*****************************************************************************
void StageControllerSim::goToNextTubeAsync(std::function<void(boost::optional<uint32_t>)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (currentType_ != eCarrierType::eCarousel)
	{
		pCBOService_->enqueueInternal (std::bind(callback, boost::none));
		return;
	}

	Delay(kTUBE_MOVE_TIME, kMED_RAND_TIME);

	currentPos_.incrementColumn();
	currentPos_.setRowColumn( eStageRows::CarouselRow, static_cast<uint8_t>(currentPos_.getColumn()) );
	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(std::bind(callback, currentPos_.getColumn()));
}

void StageControllerSim::enableStageHoldingCurrent(std::function<void(bool)> onCompletion, bool enable)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	// Enable/Disable Carousel Theta/ Radius holding currents
	auto EnableHoldingCurrentForStage = [this, onCompletion, enable]() -> void
	{
		onCompletion(true);
	};

	pCBOService_->enqueueInternal(EnableHoldingCurrentForStage);
}

void StageControllerSim::setStageProfile(std::function<void(bool)> onCompletion, bool enable)
{
	HAWKEYE_ASSERT (MODULENAME, onCompletion);

	// Update Carousel Theta/ Radius holding currents during work queue processing
	auto UpdateHoldingCurrentForCarousel = [this, onCompletion, enable]() -> void
	{
		onCompletion(true);
	};

	pCBOService_->enqueueInternal(UpdateHoldingCurrentForCarousel);
}

void StageControllerSim::isPlatePresentAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Delay(kPLATE_DETECT_TIME, kBIG_RAND_TIME);
	currentPos_.setRowColumn(eStageRows::PlateRowD, 11);

	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(callback, currentType_ == eCarrierType::ePlate_96);
}

void StageControllerSim::homeStageAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	Delay(kHOME_MOVE_TIME, kMED_RAND_TIME);

	updateRadiusThetaPos();
	if (currentType_ == eCarrierType::eCarousel)
	{
		auto status = currentPos_.setRowColumn(eStageRows::CarouselRow, 1);
		pCBOService_->enqueueInternal(callback, status);
		return;
	}

	if (currentType_ == eCarrierType::ePlate_96)
	{
		auto status = currentPos_.setRowColumn(eStageRows::PlateRowA, 1);
		pCBOService_->enqueueInternal(callback, status);
		return;
	}

	pCBOService_->enqueueInternal(callback, false);
}

void StageControllerSim::ejectStageAsync (std::function<void(bool)> callback, int32_t angle )
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	currentPos_ = SamplePositionDLL();
	Delay(kSTAGE_EJECT_TIME, kMED_RAND_TIME);
	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(callback, true);
}

void StageControllerSim::moveStageToPosition (std::function<void(bool)> callback, SamplePositionDLL position)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto crow = currentPos_.getRow();
	if ((crow != '?') && (crow != eStageRows::CarouselRow))
	{
		auto trow = position.getRow();
		auto tcol = position.getColumn();
		auto ccol = currentPos_.getColumn();
		int colsteps = 0;
		if (ccol >= tcol)
			colsteps = ccol - tcol;
		else
			colsteps = tcol - ccol;
		int rowsteps = 0;
		if (crow >= trow)
			rowsteps = crow - trow;
		else
			rowsteps = trow - crow;

		auto moveT = (kPLATE_POS_TIME * (colsteps + rowsteps + 2));
		Delay(moveT, kMED_RAND_TIME);
	}
	currentPos_ = position;

	if ( ( currentType_ == eCarrierType::eCarousel ) &&
		 ( !position.isValidForCarousel() ) )
	{
		double_t thetaAngle = ( thetaPos_ / 10 );
		int32_t tNum = (int32_t) ( (int32_t) ( thetaAngle / DefaultDegreesPerTube ) % (int32_t) ( 360.0 / DefaultDegreesPerTube ) ) + 1;
		currentPos_.setRowColumn( eStageRows::CarouselRow, static_cast<uint8_t>(tNum) );
	}
	else
	{
		currentPos_ = position;
	}

	updateRadiusThetaPos();
	pCBOService_->enqueueInternal(callback, true);
}

void StageControllerSim::clearErrorsAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	pCBOService_->enqueueInternal(callback, true);
}

void StageControllerSim::stopAsync (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	Delay(kSTOP_TIME, kMED_RAND_TIME);
	thetaMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, thetaPos_, ePositionDescription::Current);
	radiusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, radiusPos_, ePositionDescription::Current);
	probeMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, -1, ePositionDescription::Current);
	pCBOService_->enqueueInternal(callback, true);
}


void StageControllerSim::setCarsouselSlots(const std::vector<bool>& slots)
{
	std::lock_guard<std::mutex> m{ carouselSlotsForSimulationMutex_ };
	//
	// Note: We use zero based indices here.
	//       Add 1 for the carousel position or 'slot'
	//
	for (int i = 0; i < slots.size(); i++)
	{
		// If there is an entry in slot 21 don't add it
		// this will cause the sample to be 'skipped - missing' 
		if ((slots[i]) && (i != 20))
		{
			carouselSlotsForSimulation_[i] = true;
		}

		//
		// Generate orphans under certain conditions
		// Define samples for slots 3, 11 and 17 and Not 1, 2 nor 4
		// This will cause 'tubes' to be found at slot 13 and 23
		// If either is already defined then it will not be an orphan
		//
		if (slots[2] && slots[10] && slots[16] && !slots[0] && !slots[1] && !slots[3])
		{
			carouselSlotsForSimulation_[12] = true;
			carouselSlotsForSimulation_[22] = true;
		}
	}
}