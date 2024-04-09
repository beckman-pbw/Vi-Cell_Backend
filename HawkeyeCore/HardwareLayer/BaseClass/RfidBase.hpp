#pragma once

#include <stdint.h>
#include <map>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "ReagentRFIDLayout.hpp"
#include "CBOService.hpp"

//TODO: is this still needed???
#define CONTROLLER_BOARD_SEND_CMD_TIMEOUT 1263  //there are commands to RFID that could take more than a second

class RfidBase
{
public:
	RfidBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~RfidBase();

	virtual void initialize (std::function<void(bool)> callback);
	virtual void scan (std::function<void(bool)> callback);
	virtual void read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback);
	virtual void setTime (std::function<void(bool)> callback);
	virtual void setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap);
	bool IsSim() const { return isSim_; }

protected:
	void parseTag (size_t tagNum, RfAppdataRegisters& rfidData, RfidTag_t &rfidTag);

	bool isSim_;
	std::shared_ptr<CBOService> pCBOService_;
};
