#pragma once

#include <vector>

#include "CommandParser.hpp"
#include "LoggerDB.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeError.hpp"
#include "HawkeyeServices.hpp"
#include "SystemErrors.hpp"

//*****************************************************************************
template<typename DLLTYPE, typename TLogger>
class LogReaderBase
{
public:
	virtual ~LogReaderBase() {}

	//*****************************************************************************
	virtual void LogReaderBase::readAsync(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> data_cb)
	{
		HAWKEYE_ASSERT (MODULENAME, data_cb);

		pHawkeyeService_->enqueueInternal([=]()
		{
			readInternal(starttime, endtime, data_cb);
		});
	}

	//NOTE: not currently used...  save for future use...
	////*****************************************************************************
	//virtual void LogReaderBase::archiveAsync(uint64_t archive_prior_to_time, const std::string& archiveLocation, std::function<void(HawkeyeError)> callback)
	//{
	//	HAWKEYE_ASSERT (MODULENAME, callback);

	//	pHawkeyeService_->enqueueInternal([=]()
	//	{
	//		archiveInternal(archive_prior_to_time, archiveLocation, callback);
	//	});
	//}

	//NOTE: not currently used...  save for future use...
	////*****************************************************************************
	//virtual void LogReaderBase::readArchiveAsync(const std::string& archiveLocation, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> callback)
	//{
	//	HAWKEYE_ASSERT (MODULENAME, callback);

	//	pHawkeyeService_->enqueueInternal([=]()
	//	{
	//		readArchiveInternal(archiveLocation, callback);
	//	});
	//}

protected:

	struct ReadParams
	{
		uint64_t startTime;
		uint64_t endTime;
		size_t currentIndex;
		std::vector<std::string> fileContent;
		std::vector<DLLTYPE> resultLogData;
	};

	LogReaderBase(std::shared_ptr<HawkeyeServices> pHawkeyeService)
		: pHawkeyeService_(pHawkeyeService)
	{
	}

	virtual void readInternal(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> data_cb);

	//NOTE: not currently used...  save for future use...
	//virtual void archiveInternal(uint64_t priorToTime, std::string archiveLocation, std::function<void(HawkeyeError)> callback);
	//virtual void readArchiveInternal(std::string archiveLocation, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> callback);

	virtual bool getFileContent(std::vector<std::string>& fileContent)
	{
		TLogger::L().ReadFile(fileContent);
		return true;
	}
	virtual boost::optional<uint64_t> getTimeFieldFromLine(const std::string& line) = 0;
	virtual  boost::optional<DLLTYPE> getDecodedData(const std::string& line) = 0;

	CommandParser commandParser_;
	std::shared_ptr<HawkeyeServices> pHawkeyeService_;

private:
	
	void encodeForMultilineEntries(std::string& message)
	{
		// Add "\t" along with any "\n" in "output" string This will help us differentiate actual line ending
		boost::algorithm::replace_all(message, "\n", "\n\t");

		// Now add "\n" (end of message) separately
		message += "\n";
	}
	
	void readInternalAsync(std::shared_ptr<ReadParams> pReadParams, std::function<void(bool)> onComplete);
};

template<typename DLLTYPE, typename TLogger>
void LogReaderBase<DLLTYPE, TLogger>::readInternal(uint64_t starttime, uint64_t endtime, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> data_cb)
{
	HAWKEYE_ASSERT (MODULENAME, data_cb);

	auto pReadParams = std::make_shared<ReadParams>();
	pReadParams->startTime = starttime;
	pReadParams->endTime = endtime > 0 ? endtime : (std::numeric_limits<uint64_t>::max)();
	pReadParams->currentIndex = 0;
	pReadParams->fileContent.clear();
	pReadParams->resultLogData.clear();

	if (!getFileContent(pReadParams->fileContent))
	{
		pHawkeyeService_->enqueueInternal(data_cb, HawkeyeError::eStorageFault, std::vector<DLLTYPE>{});
		return;
	}

	readInternalAsync(pReadParams, [this, pReadParams, data_cb](bool success)
	{
		auto he = HawkeyeError::eSuccess;
		if (!success)
		{
			pReadParams->resultLogData.clear();
			he = HawkeyeError::eStorageFault;
		}
		pHawkeyeService_->enqueueInternal(data_cb, he, std::move(pReadParams->resultLogData));
	});
}

//NOTE: not currently used...  save for future use...
//template<typename DLLTYPE, typename TLogger>
//void LogReaderBase<DLLTYPE, TLogger>::archiveInternal(uint64_t priorToTime, std::string archiveLocation, std::function<void(HawkeyeError)> callback)
//{
//	HAWKEYE_ASSERT (MODULENAME, callback);
//
//	std::vector<std::string> dataToArchive;
//	bool success = TLogger::L().RetrieveAndDeleteFileData([this, priorToTime](const std::string& currentLine) -> bool
//	{
//		boost::optional<uint64_t> loggedTime = getTimeFieldFromLine(currentLine);
//		return loggedTime && (loggedTime.get() < priorToTime);
//	}, dataToArchive);
//
//	if (!success)
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "archiveInternal: Failed to retrieve data to archive");
//		pHawkeyeService_->enqueueInternal(callback, HawkeyeError::eStorageFault);
//		return;
//	}
//
//	if (dataToArchive.empty())
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "archiveInternal: No data found to archive prior to time : " + std::to_string(priorToTime));
//		pHawkeyeService_->enqueueInternal(callback, HawkeyeError::eSuccess);
//		return;
//	}
//
//	// Add special token to differentiate multiline data and new line
//	for (auto& line : dataToArchive)
//	{
//		encodeForMultilineEntries(line);
//	}
//
//	success = HDA_WriteEncryptedStringListFile (archiveLocation, dataToArchive);
//	if (!success)
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "archiveInternal: Failed to write to encrypted file");
//	}
//
//	pHawkeyeService_->enqueueInternal(callback, success ? HawkeyeError::eSuccess : HawkeyeError::eStorageFault);
//}

//NOTE: not currently used...  save for future use...
//template<typename DLLTYPE, typename TLogger>
//inline void LogReaderBase<DLLTYPE, TLogger>::readArchiveInternal(std::string archiveLocation, std::function<void(HawkeyeError, std::vector<DLLTYPE>)> callback)
//{
//	HAWKEYE_ASSERT (MODULENAME, callback);
//
//	std::string enArchiveLocation;
//	if (!GetEncryptedFileName(archiveLocation, enArchiveLocation))
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "readArchiveInternal: Failed to get encrypted file name from : " + archiveLocation);
//		pHawkeyeService_->enqueueInternal(callback, HawkeyeError::eStorageFault, std::vector<DLLTYPE>{});
//		return;
//	}
//
//	if (!boost::filesystem::exists(enArchiveLocation))
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "readArchiveInternal: No file exist to read at location : " + archiveLocation);
//		pHawkeyeService_->enqueueInternal(callback, HawkeyeError::eStorageFault, std::vector<DLLTYPE>{});
//		return;
//	}
//	
//	std::vector<std::string> readLogData;
//	bool success = HDA_ReadEncryptedStringListFile (archiveLocation, readLogData);
//	if (!success)
//	{
//		Logger::L().Log (MODULENAME, severity_level::error, "readArchiveInternal: Failed to read encrypted file : " + archiveLocation);
//		pHawkeyeService_->enqueueInternal(callback, HawkeyeError::eStorageFault, std::vector<DLLTYPE>{});
//		return;
//	}
//
//	std::vector<std::string> multiLineLogData;
//
//	// Remove special token which was to differentiate multiline data and new line
//	std::string multiLine;
//	for (auto& line : readLogData)
//	{
//		line += "\n"; // Append new line at end
//		if (LoggerDB::DecodeForMultilineEntries(line))
//		{
//			multiLine += line;
//			multiLineLogData.pop_back(); // remove last entry, a new multiline entry will be added
//		}
//		else
//		{
//			multiLine = line;
//		}
//		multiLineLogData.emplace_back(multiLine);
//	}
//	readLogData.clear();
//
//	auto pReadParams = std::make_shared<ReadParams>();
//	pReadParams->startTime = 0;
//	pReadParams->endTime = (std::numeric_limits<uint64_t>::max)();
//	pReadParams->currentIndex = 0;
//	pReadParams->fileContent = std::move(multiLineLogData);
//	pReadParams->resultLogData.clear();
//
//	readInternalAsync(pReadParams, [this, pReadParams, callback](bool success)
//	{
//		auto he = HawkeyeError::eSuccess;
//		if (!success)
//		{
//			pReadParams->resultLogData.clear();
//			he = HawkeyeError::eStorageFault;
//		}
//		pHawkeyeService_->enqueueInternal(callback, he, std::move(pReadParams->resultLogData));
//	});
//}

template<typename DLLTYPE, typename TLogger>
void LogReaderBase<DLLTYPE, TLogger>::readInternalAsync(std::shared_ptr<ReadParams> pReadParams, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, pReadParams);
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (pReadParams->currentIndex >= pReadParams->fileContent.size())
	{
		pHawkeyeService_->enqueueInternal(callback, true);
		return;
	}

	const auto& currentLine = pReadParams->fileContent[pReadParams->currentIndex];
	auto loggedTime = getTimeFieldFromLine(currentLine);
	if (!loggedTime)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "readInternalAsync: Failed to read time entry from line : " + currentLine);
		pHawkeyeService_->enqueueInternal(callback, false);
		return;
	}
	else if (loggedTime >= pReadParams->startTime && loggedTime <= pReadParams->endTime)
	{
		auto data = getDecodedData(currentLine);
		if (!data)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "readInternalAsync: Failed to read decoded data from line : " + currentLine);
			pHawkeyeService_->enqueueInternal(callback, false);
			return;
		}
		pReadParams->resultLogData.emplace_back(data.get());
	}
	else if (loggedTime.get() > pReadParams->endTime)
	{
		// break loop if loop is passed end time line
		pReadParams->currentIndex = pReadParams->fileContent.size();
	}

	pHawkeyeService_->enqueueInternal([this, pReadParams, callback]()
	{
		pReadParams->currentIndex++;
		readInternalAsync(pReadParams, callback);
	});
}
