#include "stdafx.h"

#include "SampleDefinitionDLL.hpp"
#include "SampleLog.hpp"
#include "WorkListDLL.hpp"

static const char MODULENAME[] = "SampleLog";

//*****************************************************************************
SampleLog::SampleLog(std::shared_ptr<HawkeyeServices> pHawkeyeService)
	: LoggerDB(DBLogType::eLOG_SAMPLE, pHawkeyeService),
	 Delimiter("|")
{ }

//*****************************************************************************
SampleLog::~SampleLog()
{ }

void SampleLog::readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<sample_activity_entryDLL>)> data_cb)
{
	HAWKEYE_ASSERT(MODULENAME, data_cb);

	pHawkeyeService_->enqueueInternal([=]()
		{
			readInternal(pHawkeyeService_, starttime, endtime, data_cb);
		});
}

void SampleLog::readInternal(std::shared_ptr<HawkeyeServices> pHawkeyeService, uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<sample_activity_entryDLL>)> data_cb)
{
	std::vector<std::string> logRecs = {};
	retrieveRecords(starttime, endtime, logRecs);
	std::vector<sample_activity_entryDLL> data = {};
	for (auto s : logRecs)
	{
		if (auto v = getDecodedData(s))
			data.push_back(*v);
	}
	pHawkeyeService->enqueueInternal(data_cb, HawkeyeError::eSuccess, data);
	return;
}

//*****************************************************************************
boost::optional<uint64_t> SampleLog::getTimeFieldFromLine(const std::string& line)
{
	std::string timeField;
	if (!commandParser_.parse(Delimiter, line) || !commandParser_.getByIndex(0, timeField))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getTimeFieldFromLine : Failed to parse time field in line :" + line);
		return boost::none;
	}
	return std::stoull(timeField);
}

//*****************************************************************************
boost::optional<sample_activity_entryDLL> SampleLog::getDecodedData (const std::string& line)
{
	if (!commandParser_.parse(Delimiter, line))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Failed to parse line :" + line);
		return boost::none;
	}

	std::vector<std::string> fields;
	for (auto index = 0; index < commandParser_.size(); index++)
	{
		fields.emplace_back(commandParser_.getByIndex(index));
	}

	// Initial read of the Worklist data is 4 fields long.
	const std::size_t WL_HEADER_SIZE = 4;
	if (fields.size() < WL_HEADER_SIZE)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Invalid entry count :" + std::to_string(fields.size()));
		return boost::none;
	}

	sample_activity_entryDLL logData = {};
	
	logData.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(std::stoull(fields[0]));
	logData.username = fields[1];
	logData.worklistLabel = fields[2];
	logData.samples.resize(std::stol(fields[3]));

	// Check validity of size now that we know how many samples to read in
	const std::size_t WQI_ENTRY_SIZE = 5;
	if (fields.size() != (WL_HEADER_SIZE + (logData.samples.size() * WQI_ENTRY_SIZE)))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getDecodedData : Invalid entry count :" + std::to_string(fields.size()));
		return boost::none;
	}

	std::size_t idx2 = 0;
	for (auto idx = WL_HEADER_SIZE; idx < fields.size(); idx += WQI_ENTRY_SIZE)
	{
		logData.samples[idx2].timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(std::stoull(fields[idx]));
		logData.samples[idx2].sample_label = fields[idx + 1];
		logData.samples[idx2].celltype_name = fields[idx + 2];
		logData.samples[idx2].analysis_name = fields[idx + 3];
		logData.samples[idx2].completion = static_cast<sample_completion_status>(std::stoi(fields[idx + 4]));
		idx2++;
	}

	return logData;
}

//*****************************************************************************
std::string generateSampleWriteData (const WorklistDLL::WorklistData& wl, const std::vector<WorklistDLL::SampleIndices>& indices) {
	std::stringstream ss;

	ss << boost::str (boost::format ("%s|%s|%s|%d")
		% ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(wl.timestamp)
		% wl.username
		% wl.label
		% indices.size()
	);

	for (auto& v : indices) {

		SampleDefinitionDLL sample = WorklistDLL::getSampleByIndices (v);

		// work queue item will have "eSampleStatus" as current status
		// but we need to save "sample_comp_status" in sample logger
		const auto sampleCompletionStatus = sample.ToSampleLogCompletionStatus();

		std::string cellTypeOrQCName = "";

		if (sample.parameters.bp_qc_name.empty())
		{
			cellTypeOrQCName = sample.parameters.celltype.label;
		}
		else
		{			
			cellTypeOrQCName = boost::str (boost::format("%s (%s)") % sample.parameters.bp_qc_name % sample.parameters.celltype.label);
		}
				
		ss << "|";
		ss << boost::str (boost::format ("%s|%s|%s|%s|%s")
			% ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(sample.timestamp)
			% sample.parameters.label
			% cellTypeOrQCName
			% sample.parameters.analysis.label
			% DataConversion::enumToRawValueString (sampleCompletionStatus)
		);
	}

	return ss.str();
}
