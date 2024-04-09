#pragma once

#include "SyringePumpBase.hpp"

class SyringePumpSim : public SyringePumpBase
{
public:
	SyringePumpSim (std::shared_ptr<CBOService> pCBOService);
	virtual ~SyringePumpSim();

	virtual void setPosition(std::function<void(bool)> callback, uint32_t volume, uint32_t speed) override;
	virtual void setValve(std::function<void(bool)> callback, SyringePumpPort port, SyringePumpDirection direction) override;

private:

};
