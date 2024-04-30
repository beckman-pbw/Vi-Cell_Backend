#pragma once
#include <cstdint>

enum eFlushFlowCellState : uint16_t
{
	ffc_Idle = 0,
	ffc_FlushingCleaner,
	ffc_FlushingConditioningSolution,
	ffc_FlushingBuffer,
	ffc_FlushingAir,
	ffc_Completed,
	ffc_Failed
};

enum eDecontaminateFlowCellState : uint16_t
{
	dfc_Idle = 0,
	dfc_AspiratingBleach,
	dfc_Dispensing1,
	dfc_DecontaminateDelay,
	dfc_Dispensing2,
	dfc_FlushingBuffer,
	dfc_FlushingAir,
	dfc_Completed,
	dfc_Failed,
	dfc_FindingTube,
	dfc_TfComplete,	// "Tf" here is referring to the "tube find" process.
	dfc_TfCancelled,
};

enum ePrimeReagentLinesState : uint16_t
{
	prl_Idle = 0,
	prl_PrimingCleaner1,
	prl_PrimingCleaner2,
	prl_PrimingCleaner3,
	prl_PrimingReagent1,
	prl_PrimingDiluent,	// May be skipped if not present
	prl_Completed,
	prl_Failed
};

enum ePurgeReagentLinesState : uint16_t
{
	dprl_Idle = 0,

	dprl_PurgeCleaner1,
	dprl_PurgeCleaner2,
	dprl_PurgeCleaner3,
	dprl_PurgeReagent1,
	dprl_PurgeDiluent,	// May be skipped if not present
	dprl_Completed,
	dprl_Failed
};

enum eDrainReagentPackState : uint16_t
{
	drp_Idle = 0,

	drp_DrainCleaner1,
	drp_DrainCleaner2,
	drp_DrainCleaner3,
	drp_DrainReagent1,
	drp_DrainDiluent,	// May be skipped if not present

	drp_Completed,
	drp_Failed
};
