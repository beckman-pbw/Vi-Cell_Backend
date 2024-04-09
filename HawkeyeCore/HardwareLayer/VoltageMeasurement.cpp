#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "ErrorCode.hpp"
#include "Logger.hpp"
#include "Registers.hpp"
#include "VoltageMeasurement.hpp"
#include "SystemErrors.hpp"
#include "HawkeyeConfig.hpp"

static const char MODULENAME[] = "VoltageMeasurement";
constexpr uint32_t CHANNELS_TO_READ = (VOLTMON_BITMAP_SIZE / sizeof (uint32_t)) + (VOLT_MON_REG_SIZE / sizeof (uint32_t));


//NOTE: Values coming back from the ControllerBoard only need to be scaled by 1000.
//*****************************************************************************
VoltageMeasurement::VoltageMeasurement (std::shared_ptr<CBOService> pCBOService)
	: VoltageMeasurementBase (pCBOService)
{
}

//*****************************************************************************
VoltageMeasurement::~VoltageMeasurement()
{
}
//*****************************************************************************
bool VoltageMeasurement::Initialize (void) 
{
	return true;
}

//*****************************************************************************
void VoltageMeasurement::ReadVoltages (std::function<void (VoltageMeasurements_t vgs)> cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "ReadVoltages: <enter>");

	HAWKEYE_ASSERT (MODULENAME, cb);
	callback_ = cb;

	auto tid = pCBOService_->CBO()->Query(VoltageReadOperation(), std::bind(&VoltageMeasurement::cbVoltageReadComplete, this, std::placeholders::_1));
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("VoltageRead, task %d") % (*tid)));
	}
	else
	{
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, "ReadVoltages: <exit>");
}
//*****************************************************************************
void VoltageMeasurement::cbVoltageReadComplete (ControllerBoardOperation::CallbackData_t cbData)
{
	if ( cbData.status != ControllerBoardOperation::eStatus::Success )
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("cbVoltageReadComplete: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
		ReportSystemError::Instance().ReportError (BuildErrorInstance (instrument_error::controller_general_interface, instrument_error::cntrlr_general_instance::none, instrument_error::severity_level::notification));
		return;
	}

	uint32_t buf[CHANNELS_TO_READ];
	cbData.FromRx (buf, sizeof (buf));

	Logger::L ().Log (MODULENAME, severity_level::debug2, "onSendDirectComplete: <callback>");
	voltages_.v_5 = static_cast<float>(buf[1]) / 1000.0f;
	voltages_.v_12 = static_cast<float>(buf[2]) / 1000.0f;
	voltages_.v_24 = static_cast<float>(buf[3]) / 1000.0f;
	voltages_.P5Vsen = static_cast<float>(buf[4]) / 1000.0f;
	voltages_.neg_v_3 = static_cast<float>(buf[5]) / 1000.0f;
	voltages_.v_33 = static_cast<float>(buf[6]) / 1000.0f;
	voltages_.boardTemp = static_cast<float>(buf[8]) / 1000.0f; // Deg C
	voltages_.cpuTemp = static_cast<float>(buf[9]) / 1000.0f;  // Deg C

	if ( HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::InstrumentType::ViCELL_FL_Instrument)
	{
		voltages_.extTemp = static_cast<float>(buf[7]) / 1000.0f;  // Deg C
		voltages_.LED_SW = static_cast<float>(buf[12]) / 1000.0f;
		voltages_.LED_ISENSE = static_cast<float>(buf[13]) / 1000.0f;
		voltages_.LED_DR = static_cast<float>(buf[14]) / 1000.0f;
		voltages_.pd1Out = static_cast<float>(buf[15]) / 1000.0f;   // nA
		voltages_.pd2Out = static_cast<float>(buf[16]) / 1000.0f;   // nA
	}
	else
	{
		voltages_.extTemp = 0.0;
		voltages_.LED_SW = 0.0;
		voltages_.LED_ISENSE = 0.0;
		voltages_.LED_DR = 0.0;
		voltages_.pd1Out = 0.0;
		voltages_.pd2Out = 0.0;
	}

	if ( buf[0] != 0 )
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "Voltage Measurement Out of Range, Bit map : " + std::to_string (buf[0]));
		for ( uint8_t ii = 1; ii < CHANNELS_TO_READ; ii++ )
		{
			if ( (buf[0] & (1u << (ii - 1))) != 0 )
			{
				Logger::L ().Log (MODULENAME, severity_level::debug1, "Channel " + std::to_string (ii) + " - " + std::to_string (buf[ii]));
			}
		}
		ReportSystemError::Instance().ReportError (BuildErrorInstance (instrument_error::controller_general_hardware_health, instrument_error::cntrlr_general_instance::none, instrument_error::severity_level::warning));
		return;
	}

	pCBOService_->enqueueExternal (std::bind (callback_, voltages_));

};

