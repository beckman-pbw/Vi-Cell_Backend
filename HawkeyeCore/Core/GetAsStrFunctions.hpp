#pragma once

#include <string>

#include "AuditEventType.hpp"
#include "CalibrationCommon.hpp"
#include "Fluidics.hpp"
#include "HawkeyeError.hpp"
#include "ReagentCommon.hpp"
#include "ReportsCommon.hpp"
#include "SampleDefinition.hpp"
#include "StageDefines.hpp"
#include "SystemStatusCommon.hpp"
#include "UserLevels.hpp"

std::string getAuditEventTypeAsStr (audit_event_type eType);
const char* getCalibrationStateAsStr (CalibrationState cs);
const char* getCarrierTypeAsStr (eCarrierType carrierType);
const char* getErrorAsStr (HawkeyeError he);
const char* getMotorStatusAsStr (MotorStatus& motorStatus);
const char* getPermissionLevelAsStr (UserPermissionLevel permission);
std::string getReagentContainerStatusAsStr (ReagentContainerStatus eType);
const char* getReagentFlushFlowCellStatusAsStr (eFlushFlowCellState status);
const char* getReagentPackDecontaminateFlowCellStatusAsStr (eDecontaminateFlowCellState status);
const char* getReagentPackDrainStatusAsStr (eDrainReagentPackState status);
const char* getReagentPackLoadStatusAsStr (ReagentLoadSequence status);
const char* getReagentPackUnloadStatusAsStr (ReagentUnloadSequence status);
const char* getCleanFluidicsStatusAsStr(eFlushFlowCellState status);
const char* getSensorStatusAsStr (eSensorStatus status);
const char* getSystemStatusAsStr (eSystemStatus wStatus);
const char* getSampleStatusAsStr (eSampleStatus status);
