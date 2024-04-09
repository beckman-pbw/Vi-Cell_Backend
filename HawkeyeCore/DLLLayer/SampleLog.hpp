#pragma once

#include "LoggerDB.hpp"
#include "LogReaderBase.hpp"
#include "ReportsDLL.hpp"
#include "SampleDefinitionDLL.hpp"
#include "WorkListDLL.hpp"

std::string generateSampleWriteData (const WorklistDLL::WorklistData& wl, const std::vector<WorklistDLL::SampleIndices>& indices);

class SampleLog :public LoggerDB
{
public:
	SampleLog (std::shared_ptr<HawkeyeServices> pHawkeyeService);
	~SampleLog();

	static SampleLog& L()
	{
		static SampleLog INSTANCE;
		return INSTANCE;
	}

	void readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<sample_activity_entryDLL>)> data_cb);

protected:
	SampleLog() : LoggerDB(DBLogType::eLOG_SAMPLE) {}

	void readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<sample_activity_entryDLL>)> data_cb);

	boost::optional<uint64_t> getTimeFieldFromLine(const std::string& line);
	boost::optional<sample_activity_entryDLL> getDecodedData(const std::string& line);

private:
	const std::string Delimiter;
};

using SampleLogger = SampleLog;
