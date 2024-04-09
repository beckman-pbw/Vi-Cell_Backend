#include "stdafx.h"

#include "SystemStatus.hpp"

// A proxy member definitions for RFAPPS, 
// Since, we need SystemStatus.hpp included in SystemErrors.cpp which is used in Rfid.cpp, including actual SystemStatus.cpp will need the whole lot of HawkeyeLogicImpl related stuff.

//*****************************************************************************
void SystemStatus::loadSystemStatusForSimulation (SystemStatusData& systemStatus) {
    // Do nothing
}

//*****************************************************************************
SystemStatus::SystemStatus() {

	systemStatus_.health = shInitializing;
}

//*****************************************************************************
bool SystemStatus::initialize (bool withHardware) {

    // Do nothing
	return true;
}

//*****************************************************************************
void SystemStatus::free (SystemStatusData* status) {
    // Do nothing
}

//*****************************************************************************
SystemStatusData& SystemStatus::get() {

	return systemStatus_;
}

//*****************************************************************************
void SystemStatus::get (SystemStatusData*& systemStatus) {
    // Do nothing
}

//*****************************************************************************
bool SystemStatus::decrementSampleTubeCapacityCount() {
    // Do nothing
	return true;
}

//*****************************************************************************
bool SystemStatus::incrementSampleProcessingCount(
	eCarrierType carrierType,
	uint32_t& totalSamplesProcessed) {

    // Do nothing
	return true;
}

//*****************************************************************************
void SystemStatus::sampleTubeDiscardTrayEmptied() {
    // Do nothing
}
