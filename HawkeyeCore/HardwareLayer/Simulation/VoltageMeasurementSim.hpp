#pragma once

#include "VoltageMeasurementBase.hpp"


class VoltageMeasurementSim : public VoltageMeasurementBase
{
public:
	VoltageMeasurementSim (std::shared_ptr<CBOService> pCBOService);
	virtual ~VoltageMeasurementSim() = default;

	bool Initialize (void);

	void ReadVoltages (std::function<void (VoltageMeasurements_t vgs)> cb);

private:
	std::function<void (VoltageMeasurements_t vgs)> callback_;
};

