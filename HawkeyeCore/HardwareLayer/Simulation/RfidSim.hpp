#pragma once

#include "RfidBase.hpp"

class RfidSim : public RfidBase
{
public:
	RfidSim (std::shared_ptr<CBOService> pCBOService);
	virtual ~RfidSim();

	virtual void read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback) override;
};

