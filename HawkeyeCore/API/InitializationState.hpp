#pragma once

enum InitializationState : uint16_t
{
	eInitializationInProgress = 0,
	eInitializationFailed,
	eFirmwareUpdateInProgress,
	eInitializationComplete,
	eFirmwareUpdateFailed,
	eInitializationStopped_CarosuelTubeDetected,
};
