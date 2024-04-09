#include "stdafx.h"

#include "AuditLog.hpp"
#include "DataImporter.hpp"
#include "DLLVersion.hpp"
#include "EnumConversion.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"

//*****************************************************************************
bool CorrectErroneousSampleTimes()
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: <enter>");

	try
	{
		auto startT = system_TP::min();
		auto endT = system_TP::clock::now();
		auto start = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(startT);
		auto end = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(endT);

		std::string sTimeStr;
		std::string eTimeStr;
		ChronoUtilities::getListTimestampAsStr(start, end, sTimeStr, eTimeStr);

		std::vector<DBApi::eListFilterCriteria> filterTypes = {};
		std::vector<std::string> compareOperations = {};
		std::vector<std::string> compareValues = {};

		filterTypes.push_back(DBApi::eListFilterCriteria::CreationDateRangeFilter);
		compareOperations.push_back("=");
		compareValues.push_back(boost::str(boost::format("%s;%s") % sTimeStr % eTimeStr));

		filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
		compareOperations.push_back("=");
		compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleSetStatus::SampleSetComplete));

		std::vector<DBApi::DB_SampleSetRecord> dbSampleSetList = {};

		Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: B4 GetSampleSetList ");

		auto dbStatus = DBApi::DbGetSampleSetListEnhanced(
			dbSampleSetList,
			filterTypes,
			compareOperations,
			compareValues,
			-2,
			DBApi::eListSortCriteria::CreationDateSort,
			DBApi::eListSortCriteria::SortNotDefined,
			DBApi::eContainedObjectRetrieval::FirstLevelObjs,
			-1,		// Descending sort order
			"",
			-1,
			-1,
			"");

		Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: after GetSampleSetList ");

		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: <exit> error finding sets");
			return false;
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: num sets found: " + std::to_string(dbSampleSetList.size()));

		int sampleCount = 0;
		int setCount = 0;
		int emptySetCount = 0;
		bool checkComplete = true;
		if (dbSampleSetList.size() > 0)
		{
			for (int j = 0; j < dbSampleSetList.size(); j++)
			{
				DBApi::DB_SampleSetRecord& sset = dbSampleSetList[j];

				auto delta1 = std::chrono::abs(sset.CreateDateTP - sset.ModifyDateTP);
				auto delta2 = std::chrono::abs(sset.CreateDateTP - sset.RunDateTP);
				auto delta3 = std::chrono::abs(sset.ModifyDateTP - sset.RunDateTP);

				if (sset.SSItemsList.size() == 0)
				{
					if (DBApi::DbRemoveSampleSet(sset) == DBApi::eQueryResult::QueryOk)
					{
						Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: Empty Set removed: " + Uuid::ToStr(sset.SampleSetId));
						emptySetCount++;
					}
					else
					{
						Logger::L().Log(MODULENAME, severity_level::error, "CorrectAuditLogEventTypes: Error removing empty Set: " + Uuid::ToStr(sset.SampleSetId));
						checkComplete = false;
					}
				}
				else if ((delta1 > std::chrono::hours(24)) && (delta2 > std::chrono::hours(24)) && (delta3 < std::chrono::minutes(1)))
				{
					Logger::L().Log(MODULENAME, severity_level::debug1, "CorrectAuditLogEventTypes: Set: " + Uuid::ToStr(sset.SampleSetId) + " - inconsistent dates");

					// modify all samples in the set
					int errCnt = 0;
					for (int k = 0; k < sset.SSItemsList.size(); k++)
					{
						DBApi::DB_SampleRecord srec = {};
						if (DBApi::DbFindSample(srec, sset.SSItemsList[k].SampleId) == DBApi::eQueryResult::QueryOk)
						{
							auto delta = std::chrono::abs(srec.AcquisitionDateTP - sset.CreateDateTP);
							if (delta > std::chrono::hours(24))
							{
								srec.AcquisitionDateTP = sset.CreateDateTP;
								if (DBApi::DbModifySample(srec) == DBApi::eQueryResult::QueryOk)
									sampleCount++;
								else
									errCnt++;
							}
						}
						else
						{
							Logger::L().Log(MODULENAME, severity_level::error, "CorrectAuditLogEventTypes: failed to find sample: " + Uuid::ToStr(sset.SSItemsList[k].SampleId));
						}
					}
					if (errCnt == 0)
					{
						// Only modify the Set dates if there were no errors updating sample dates
						sset.ModifyDateTP = sset.CreateDateTP;
						sset.RunDateTP = sset.CreateDateTP;
						if (DBApi::DbModifySampleSet(sset) == DBApi::eQueryResult::QueryOk)
						{
							setCount++;
						}
						else
						{
							checkComplete = false;
							Logger::L().Log(MODULENAME, severity_level::error, "CorrectAuditLogEventTypes: Error in DbModifySampleSet");
						}
					}
					else
					{
						checkComplete = false;
						Logger::L().Log(MODULENAME, severity_level::error, "CorrectAuditLogEventTypes: Errors in DbModifySample: " + std::to_string(errCnt));
					}
				}
			}
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, std::string("CorrectAuditLogEventTypes: <exit> modified Set count: ") + std::to_string(setCount) + " modified sample count: " + std::to_string(sampleCount));

		return checkComplete;
	}
	catch (std::exception& e)
	{
		Logger::L().Log(MODULENAME, severity_level::error, std::string("CorrectAuditLogEventTypes: <exit> exception") + e.what());
	}

	return false;
}

//*****************************************************************************
bool CorrectAuditLogEventTypes()
{
	std::vector<audit_log_entryDLL> logRecs = {};
	std::vector<DBApi::DB_LogEntryRecord> dbLogList;
	const std::vector<DBApi::eListFilterCriteria> filtertypelist = {};
	const std::vector<std::string> filtercomparelist = {};
	const std::vector<std::string> filtertgtlist = {};
	const DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::IdNumSort;
	const DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::NoSort;
	const int32_t sortdir = 1;	// Descending sort order.

	DBApi::eQueryResult qr = DBApi::DbGetAuditLogList(dbLogList, filtertypelist, filtercomparelist, filtertgtlist, -2, primarysort, secondarysort, sortdir);
	if (qr == DBApi::eQueryResult::QueryOk)
	{
		for (auto& entry : dbLogList)
		{
			auto rec = AuditLog::getDecodedData(entry.EntryStr, "|");
			rec->entry_id_num = entry.IdNum;
			logRecs.push_back(rec.get());
		}

		bool startingPointFound = false;
		constexpr int broken_evt_firmwareupdate = 36;
		constexpr int EventCorrectionFactor = 2;

		// Look for the broken evt_firmwareupdate enum (value 36).
		// Each entry after "broken_evt_firmwareupdate" that is > evt_logoutforced is off by two.
		// Two (2) will be added to each entry.
		for (size_t i = 0; i < logRecs.size(); i++)
		{
			if (!startingPointFound)
			{
				if (logRecs[i].event_type == broken_evt_firmwareupdate &&
					(logRecs[i].event_message.find("Previous:") != std::string::npos) &&
					(logRecs[i].event_message.find("Current:") != std::string::npos))
				{
					startingPointFound = true;
					DataImporter::Log ("Updating Audit Log Entires...");
				}
			}
			
			if (startingPointFound)
			{
				// Since we've found the beginning of the erroneous entries add two (2) to every
				// subsequent entry ID num (except for events <= evt_logoutforced) to account
				// for the adjustment in the audit_event_type enumeration.

				if (logRecs[i].event_type <= evt_logoutforced)
					continue;
				
				int temp = logRecs[i].event_type;	// A little hack since we cannot add to an enumeration.
				temp += EventCorrectionFactor;
				logRecs[i].event_type = static_cast<audit_event_type>(temp);

				DataImporter::Log (boost::str (boost::format ("Entry %d: %d, %s, %s")
						% logRecs[i].entry_id_num
						% logRecs[i].event_type
						% EnumConversion<audit_event_type>::enumToString (logRecs[i].event_type)
						% logRecs[i].event_message));

//NOTE: keep for debugging...
				//dbLogList[i].EntryStr = encodeAuditData(logRecs[i]);
				//DataImporter::Log (dbLogList[i].EntryStr);
				
				const DBApi::eQueryResult dbStatus = DBApi::DbModifyAuditLogEntry (dbLogList[i]);
				if (dbStatus != DBApi::eQueryResult::QueryOk)
				{
					DataImporter::Log ("Error modifying Audit entry: " + dbLogList[i].EntryStr);
				}
			}
		}
	}
	else
	{
		DataImporter::Log ("CorrectAuditLogEventTypes: <failed to read audit log>");
		return false;
	}

	return true;
}

//*****************************************************************************
bool GetVersionFromString(const std::string verStr, int& major, int& minor, int& rev, int& build)
{
	std::vector<std::string> result;
	boost::split(result, verStr, boost::is_any_of("."));

	major = 0;
	minor = 0;
	rev = 0;
	build = 0;
	
	size_t count = result.size();
	if (count == 0)
		return false;
	
	if (count >= 1)
	{
		major = std::atoi(result[0].c_str());
		if (count >= 2)
		{
			minor = std::atoi(result[1].c_str());
			if (count >= 3)
			{
				rev = std::atoi(result[2].c_str());
				if (count >= 4)
					build = std::atoi(result[3].c_str());
			}
		}
	}
	
	return true;
}

//*****************************************************************************
void PerformDataIntegrityChecks()
{	
#if 0  // This code is not currently needed, keep for future use.
	UINT16 major = 0;
	UINT16 minor = 0;
	UINT16 rev = 0;
	UINT16 build = 0;
	const std::string dllFullVersion = GetDLLVersion (true);

	if (GetVersionFromString(dllFullVersion, major, minor, rev, build))
	{
	}
#endif

	DataImporter::Log ("PerformDataIntegrityChecks: SW version: " + InstrumentConfig::Instance().Get().SoftwareVersion);

	if (InstrumentConfig::Instance().Get().SoftwareVersion.empty())
	{
		// Software version is not set - run all data integrity checks.
		DataImporter::Log("Running data integrity checks...");
		const bool auditLogStatusOk = CorrectAuditLogEventTypes();
		const bool dataStatusOk = CorrectErroneousSampleTimes();
		
		if (auditLogStatusOk && dataStatusOk)
		{
			const std::string swVersion = InstrumentConfig::Instance().GetDLLVersion();
			InstrumentConfig::Instance().Get().SoftwareVersion = swVersion;
			InstrumentConfig::Instance().Set();
			Logger::L().Log(MODULENAME, severity_level::debug1, "PerformDataIntegrityChecks: set SW Version to " + swVersion);
		}

		DataImporter::Log("Data integrity checks done");
	}
}
