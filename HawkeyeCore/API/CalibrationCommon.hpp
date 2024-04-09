#pragma once

#include <cstdint>

enum class CalibrationState
{
	eIdle = 0,
	eApplyDefaults,
	eHomingRadiusTheta,
	eWaitingForRadiusPos,
	eWaitingForThetaPos,
	eWaitingForRadiusThetaPos,
	eCalibrateRadius,
	eCalibrateTheta,
	eCalibrateRadiusTheta,
	eWaitingForFinish,
	eInitialize,
	eCompleted,
	eFault
};

enum calibration_type : uint16_t
{
	cal_All = 0,
	cal_Concentration,
	cal_Size,
	cal_ACupConcentration,
	cal_UNKNOWN = 99,
};
