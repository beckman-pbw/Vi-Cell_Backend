#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "Logger.hpp"
#include "VoltageMeasurementSim.hpp"
#include "HawkeyeConfig.hpp"

static const char MODULENAME[] = "VoltageMeasurementSim";

//*****************************************************************************
VoltageMeasurementSim::VoltageMeasurementSim (std::shared_ptr<CBOService> pCBOService)
	: VoltageMeasurementBase (pCBOService)
{
}

//*****************************************************************************
bool VoltageMeasurementSim::Initialize (void)
{
	return true;
}

//*****************************************************************************
void VoltageMeasurementSim::ReadVoltages (std::function<void (VoltageMeasurements_t vgs)> cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "ReadVoltages: <enter>");

    HAWKEYE_ASSERT (MODULENAME, cb);
	callback_ = cb;

    Sleep(111);

    VoltageMeasurements_t voltages;

    voltages.v_5     = 4.98f;
    voltages.v_12    = 11.67f;
    voltages.v_24    = 24.01f;
    voltages.P5Vsen  = 5.02f;
    voltages.neg_v_3 = -2.99f;
    voltages.v_33    = 3.31f;
    voltages.extTemp = 25.1f;
    voltages.boardTemp = 25.2f;
    voltages.cpuTemp = 25.3f;
    pCBOService_->enqueueExternal (std::bind (callback_, voltages));

	Logger::L().Log (MODULENAME, severity_level::debug3, "ReadVoltages: <exit>");
}

