#pragma once

#include "ReagentData.hpp"

////
/*
NOTE: This class needs to be rewritten for Hunter because it does not adequately
	  manage multiple tags and containers correctly
		 * Only a global "isExpired()" check
		 * Only a global "isEmpty()" check
		 * Only a global "isPackUsable" check
		 * Only a global "isPackInitialized" check
	  All of those need to be modified to ask the question of a single container within the system

	  reagentContainerStatesDLL, reagentValveMaps... the layout here is probably... sketchy.
*/
////

class ReagentPackPrivate
{
public:
	ReagentPackPrivate()
	{
		areRfidTagsScanned = false;
		isPackInitialized = false;
		isValveMapSet = false;
		isValveMapSetInProgress = false;
	}

	bool areRfidTagsScanned;
	bool isPackInitialized;
	std::atomic<bool> isValveMapSet;
	std::atomic<bool> isValveMapSetInProgress;

	std::mutex reagentDataMutex;
	std::shared_ptr<boost::asio::io_context> pUpstream;

	ReagentData reagentData;
	std::shared_ptr<HawkeyeServices> pReagentPackServices;
};
