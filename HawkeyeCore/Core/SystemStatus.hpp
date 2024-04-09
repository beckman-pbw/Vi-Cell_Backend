#pragma once

#include <mutex>

#include "SystemStatusCommon.hpp"

class SystemStatus
{
public:
	static SystemStatus& Instance()
	{
		static SystemStatus instance;
		return instance;
	}

	bool initialize();

	bool incrementSampleProcessingCount(eCarrierType carrierType, uint32_t& totalSamplesProcessed);
	bool decrementSampleTubeCapacityCount();
	void sampleTubeDiscardTrayEmptied();
	SystemStatusData& getData();
	void ToCStyle (SystemStatusData*& systemStatus);
	void free (SystemStatusData* status);
	
	SystemVersion systemVersion;

private:
	void loadSystemStatusForSimulation (SystemStatusData& systemStatus);

	std::mutex systemStatusMutex_;
	SystemStatusData systemStatusData_;
};
