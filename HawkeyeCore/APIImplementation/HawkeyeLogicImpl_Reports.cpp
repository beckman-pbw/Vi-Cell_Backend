#include "stdafx.h"

#include "AuditLog.hpp"
#include "ErrorLog.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "SampleLog.hpp"

#include "EnumConversion.hpp"	// This must be here in order to build correctly.

static const char MODULENAME[] = "Reports";

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveAuditTrailLogRange(
	uint64_t starttime, uint64_t endtime, uint32_t& num_entries, audit_log_entry*& log_entries,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveAuditTrailLogRange: <enter>");

	log_entries = nullptr;
	num_entries = 0;

	auto pAuditLog = std::make_shared<AuditLog>(pHawkeyeServices_);
	pAuditLog->readAsync(
		starttime, endtime,
		[this, pAuditLog, callback, &num_entries, &log_entries](
			HawkeyeError he, std::vector<audit_log_entryDLL> auditLogData)
	{
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "RetrieveAuditTrailLogRange: Failed to read audit log");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::audit, 
				instrument_error::severity_level::error));
		}
		else if (auditLogData.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "RetrieveAuditTrailLogRange: No entry found");
		}

		// Sort the audit log newest to oldest.
		std::sort (auditLogData.rbegin(), auditLogData.rend(),
			[](const audit_log_entryDLL& a, const audit_log_entryDLL& b)
		{
			return (a.timestamp < b.timestamp);
		});

		DataConversion::convert_vecOfDllType_to_listOfCType(auditLogData, log_entries, num_entries);

		Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveAuditTrailLogRange: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, he);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveInstrumentErrorLogRange(
	uint64_t starttime, uint64_t endtime, uint32_t& num_entries, error_log_entry*& log_entries,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveInstrumentErrorLogRange: <enter>");

	log_entries = nullptr;
	num_entries = 0;

	auto pErrorLog = std::make_shared<ErrorLog>(pHawkeyeServices_);
	pErrorLog->readAsync(
		starttime, endtime, [this, pErrorLog, callback, &num_entries, &log_entries](
			HawkeyeError he, std::vector<error_log_entryDLL> errorLogData)
	{
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "RetrieveInstrumentErrorLogRange: Failed to read error log");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror, 
				instrument_error::instrument_storage_instance::error,
				instrument_error::severity_level::error));
		}
		else if (errorLogData.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "RetrieveInstrumentErrorLogRange: No entry found");
		}

		// Sort the error log newest to oldest.
		std::sort (errorLogData.rbegin(), errorLogData.rend(),
			[](const error_log_entryDLL& a, const error_log_entryDLL& b)
		{
			return (a.timestamp < b.timestamp);
		});

		DataConversion::convert_vecOfDllType_to_listOfCType(errorLogData, log_entries, num_entries);

		Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveInstrumentErrorLogRange: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, he);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleActivityLogRange(
	uint64_t starttime, uint64_t endtime, uint32_t& num_entries, sample_activity_entry*& log_entries,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleActivityLogRange: <enter>");

	num_entries = 0;
	log_entries = nullptr;

//TODO: should thhe Vi-Cell code be used ???
	auto pSampleLog = std::make_shared<SampleLog>(pHawkeyeServices_);
	pSampleLog->readAsync(
		starttime, endtime, [this, pSampleLog, callback, &num_entries, &log_entries](
			HawkeyeError he, std::vector<sample_activity_entryDLL> sampleLogData)
	{
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "RetrieveSampleActivityLogRange: Failed to read sample log");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror, 
				instrument_error::instrument_storage_instance::sample, 
				instrument_error::severity_level::error));
		}
		else if (sampleLogData.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "RetrieveSampleActivityLogRange: No entry found");
		}

		// Sort the sample log newest to oldest.
		std::sort (sampleLogData.rbegin(), sampleLogData.rend(),
			[](const sample_activity_entryDLL& a, const sample_activity_entryDLL& b)
		{
			return (a.timestamp < b.timestamp);
		});

		DataConversion::convert_vecOfDllType_to_listOfCType(sampleLogData, log_entries, num_entries);

		Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleActivityLogRange: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, he);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeAuditLogEntry (audit_log_entry* entries, uint32_t num_entries)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeAuditLogEntry: <enter>");

	if (entries == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "FreeAuditLogEntry: <exit, no entries>");
		return;
	}

	for (uint32_t index = 0; index < num_entries; index++)
	{
		delete[] entries[index].active_username;
		delete[] entries[index].event_message;
	}

	delete[] entries;
	entries = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeAuditLogEntry: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::freeSampleWorklistEntry (worklist_sample_entry* entries, uint32_t num_entries)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "freeSampleWorklistEntry: <enter>");

	if (entries == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "freeSampleWorklistEntry: <exit, null ptr>");
		return;
	}

	for (uint32_t index = 0; index < num_entries; index++)
	{
		delete[] entries[index].sample_label;
		delete[] entries[index].celltype_name;
		delete[] entries[index].analysis_name;
	}

	delete[] entries;
	entries = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "freeSampleWorklistEntry: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSampleActivityEntry (sample_activity_entry* entries, uint32_t num_entries)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleActivityEntry: <enter>");

	if (entries == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "FreeSampleActivityEntry: <exit, null ptr>");
		return;
	}

	for (uint32_t index = 0; index < num_entries; index++)
	{
		delete[] entries[index].username;
		delete[] entries[index].worklistlabel;
		freeSampleWorklistEntry (entries[index].samples, entries[index].number_of_samples);
	}

	delete[] entries;
	entries = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleActivityEntry: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeErrorLogEntry (error_log_entry* entries, uint32_t num_entries)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeErrorLogEntry: <enter>");

	if (entries == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "FreeErrorLogEntry: <exit, null ptr>");
		return;
	}

	for (uint32_t index = 0; index < num_entries; index++)
	{
		delete[] entries[index].username;
		delete[] entries[index].message;
	}

	delete[] entries;
	entries = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeErrorLogEntry: <exit>");
}