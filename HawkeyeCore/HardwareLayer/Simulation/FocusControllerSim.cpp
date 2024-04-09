#include "stdafx.h"

#include <algorithm>

#include "FocusControllerSim.hpp"
#include "HawkeyeDirectory.hpp"

static const char MODULENAME[] = "FocusControllerSim";
static t_opPTree pConfig_;

extern const int32_t DefaultFocusStartPos = 30000;

static int32_t focusMaxPos_;
static int32_t focusStartPosition_;
static int32_t coarseStepValue_;
static int32_t fineStepValue_;
static int32_t focusHomePos_;
static int32_t focusPosition_;

FocusControllerSim::FocusControllerSim(std::shared_ptr<CBOService> pCBOService) : FocusControllerBase(pCBOService)
{
	focusMaxPos_ = FocusMaxTravel;
	focusStartPosition_ = DefaultFocusStartPos;
	coarseStepValue_ = 300;
	fineStepValue_ = 15;
	focusHomePos_ = 0;
	focusPosition_ = focusStartPosition_;
}

FocusControllerSim::~FocusControllerSim()
{
	Quit();
}

void FocusControllerSim::Init (std::function<void(bool)> callback, t_pPTree cfgTree, bool apply, std::string cfgFile)
{
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);
	
	setInitialization(true);

	FocusHome([this, callback](bool homeOk)
	{
		if (!homeOk || !IsHome())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to home focus motor!");
			pCBOService_->enqueueExternal (callback, false);
			return;
		}
		pCBOService_->enqueueExternal (callback, true);
	});
}

void FocusControllerSim::Quit(void)
{
	Reset();
}

void FocusControllerSim::Reset(void)
{
	focusMotorStatus_.reset();
}

void FocusControllerSim::ClearErrors(std::function<void(bool)> callback)
{
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, focusPosition_, ePositionDescription::Current);
	pCBOService_->enqueueInternal(callback,true);
}

bool FocusControllerSim::ControllerOk(void)
{
	return true;
}

void FocusControllerSim::GetMotorStatus( MotorStatus& focusMotorStatus )
{
	focusMotorStatus_.ToCStyle (focusMotorStatus);
}

bool FocusControllerSim::IsHome(void)
{
	bool homeOk = focusPosition_ == focusHomePos_;
	if (homeOk)
	{
		focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, focusPosition_, ePositionDescription::Home);
	}
	return homeOk;
}

bool FocusControllerSim::IsMax(void)
{
	return focusPosition_ == focusMaxPos_;
}

int32_t FocusControllerSim::Position( void )
{
	return focusPosition_;
}

int32_t FocusControllerSim::GetFocusMax( void )
{
	return focusMaxPos_;
}

int32_t FocusControllerSim::GetFocusStart( void )
{
	return focusStartPosition_;
}

void FocusControllerSim::SetFocusStart( int32_t start_pos )
{
	focusStartPosition_ = start_pos;
	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition );
}

uint32_t FocusControllerSim::AdjustCoarseStep( uint32_t stepSize )
{
	fineStepValue_ = stepSize;
	return fineStepValue_;
}

uint32_t FocusControllerSim::AdjustFineStep( uint32_t stepSize )
{
	coarseStepValue_ = stepSize;
	return coarseStepValue_;
}

void FocusControllerSim::Stop( std::function<void( bool )> cb, bool stopType )
{
	focusMotorStatus_.UpdateMotorHealth( eMotorFlags::mfPositionKnown, focusPosition_, ePositionDescription::Current );
	pCBOService_->enqueueExternal ( cb, true );
}

void FocusControllerSim::FocusHome( std::function<void( bool )> cb )
{
	focusPosition_ = focusHomePos_;
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfHomed, focusPosition_, ePositionDescription::Home);
	pCBOService_->enqueueExternal ( cb, true );
}

void FocusControllerSim::FocusCenter( std::function<void( bool )> cb )
{
	focusPosition_ = focusMaxPos_ / 2;
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true );
}

void FocusControllerSim::FocusMax( std::function<void( bool )> cb )
{
	focusPosition_ = focusMaxPos_;
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true );
}

void FocusControllerSim::SetPosition( std::function<void( bool, int32_t endPos )> cb, int32_t new_pos )
{
	focusPosition_ = new_pos;
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepUpFast( std::function<void( bool, int32_t endPos )> cb )
{
	auto fastStep = coarseStepValue_ * 5;
	focusPosition_ += fastStep;
	focusPosition_ = std::min (focusPosition_, focusMaxPos_);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepDnFast( std::function<void( bool, int32_t endPos )> cb )
{
	auto fastStep = coarseStepValue_ * 5;
	focusPosition_ -= fastStep;
	focusPosition_ = std::min (focusPosition_, focusHomePos_);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepUpCoarse( std::function<void( bool, int32_t endPos )> cb )
{
	focusPosition_ += coarseStepValue_;
	focusPosition_ = std::min (focusPosition_, focusMaxPos_);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepDnCoarse( std::function<void( bool, int32_t endPos )> cb )
{
	focusPosition_ -= coarseStepValue_;
	focusPosition_ = (std::max)(focusPosition_, focusHomePos_);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepUpFine( std::function<void( bool, int32_t endPos )> cb )
{
	focusPosition_ += fineStepValue_;
	focusPosition_ = std::min (focusPosition_, focusMaxPos_);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}

void FocusControllerSim::FocusStepDnFine( std::function<void( bool, int32_t endPos )> cb )
{
	focusPosition_ -= fineStepValue_;
	focusPosition_ = std::min (focusPosition_, focusHomePos_);
	focusMotorStatus_.UpdateMotorHealth(eMotorFlags::mfAtPosition, focusPosition_, ePositionDescription::AtPosition);
	pCBOService_->enqueueExternal ( cb, true, focusPosition_ );
}
