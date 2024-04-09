#pragma once

#include "VoltageMeasurementBase.hpp"


class VoltageMeasurement : public VoltageMeasurementBase
{
public:
	VoltageMeasurement (std::shared_ptr<CBOService> pCBOService);
	virtual ~VoltageMeasurement();

	bool Initialize (void);

	void ReadVoltages (std::function<void (VoltageMeasurements_t vgs)> cb);

private:
	VoltageMeasurements_t voltages_;
	std::function<void (VoltageMeasurements_t vgs)> callback_;

	void cbVoltageReadComplete (ControllerBoardOperation::CallbackData_t cbData);
};

