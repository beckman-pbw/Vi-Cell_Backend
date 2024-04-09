#include "stdafx.h"

#include "DataConversion.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "QualityControlsDLL.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_DataTemplates";

//*****************************************************************************
void HawkeyeLogicImpl::SaveSampleSetTemplate (const SampleSet& sampleSet, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "SaveSampleSetTemplate: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	auto he = HawkeyeError::eSuccess;
	std::shared_ptr<SampleSetDLL> sampleSetDLL = std::make_shared<SampleSetDLL>(sampleSet);

	sampleSetDLL->worklistUuid = {};

	DBApi::DB_SampleSetRecord dbSS = sampleSetDLL->ToDbStyle();
	dbSS.SampleSetStatus = static_cast<int32_t>(DBApi::eSampleSetStatus::SampleSetTemplate);

	DBApi::eQueryResult dbStatus = DBApi::DbSaveSampleSetTemplate (dbSS);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveSampleSetTemplate: <exit, DB write failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::workflow_design,
			instrument_error::severity_level::error));

		he = HawkeyeError::eStorageFault;
	}

	pHawkeyeServices_->enqueueExternal (onComplete, he);
}

//*****************************************************************************
void HawkeyeLogicImpl::GetSampleSetTemplateList(
	uint32_t skip, uint32_t take,
	SampleSet*& sampleSets,
	uint32_t& numSampleSets,
	uint32_t& totalSampleSetsAvailable,
	std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetSampleSetTemplateList: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	auto he = HawkeyeError::eSuccess;

	std::vector<DBApi::DB_SampleSetRecord> dbSampleSetList;

	// Set appropriate value for DB when not skipping any entries or taking everything.
	int32_t tSkip = skip;
	if (tSkip == 0 || tSkip < 0)
	{
		tSkip = -1;
	}
	int32_t tTake = take;
	if (tTake == 0 || tTake < 0)
	{
		tTake = -1;
	}

	std::string statusValStr = boost::str(boost::format("%ld") % (static_cast<int32_t>(DBApi::eSampleSetStatus::SampleSetTemplate)));

	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleSetList(
		dbSampleSetList,
		DBApi::eListFilterCriteria::StatusFilter,	// Filter type.
		"=",										// Filter compare operator: '=' '<' '>' etc.
		statusValStr,								// Filter target value for comparison: true false value etc
		tTake,										// Limit count.
		DBApi::eListSortCriteria::CreationDateSort,	// primary Sort type.
		DBApi::eListSortCriteria::SortNotDefined,	// secondary Sort type.
		DBApi::eContainedObjectRetrieval::FirstLevelObjs,	// first-level sub-items
		-1,											// Sort direction, descending.
		"",											// Order string.
		tSkip,										// Start index.
		-1 );										// Start ID num.

	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eEntryNotFound);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("GetSampleSetTemplateList: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::workflow_design,
			instrument_error::severity_level::error));

		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eStorageFault);
		return;
	}
	
	std::vector<SampleSetDLL> sampleSetsDLL;

	//Logger::L().Log (MODULENAME, severity_level::debug1, "GetSampleSetTemplateList: list of SampleSets");
	for (auto& v : dbSampleSetList)
	{
		//NOTE: save for debugging...
		//auto stime = ChronoUtilities::ConvertToString (v.CreateDateTP, "%Y-%m-%d %H:%M:%S");
		//Logger::L().Log (MODULENAME, severity_level::debug1, stime);

		SampleSetDLL sampleSet;
		sampleSet.FromDbStyle (v);
		sampleSetsDLL.push_back (sampleSet);
	}

	DataConversion::convert_vecOfDllType_to_listOfCType (sampleSetsDLL, sampleSets, numSampleSets);

	totalSampleSetsAvailable = numSampleSets;

	pHawkeyeServices_->enqueueExternal (onComplete, he);
}

//*****************************************************************************
void HawkeyeLogicImpl::GetSampleSetAndSampleList (const uuid__t uuid, SampleSet*& sampleSet, std::function<void(HawkeyeError)> onComplete)
{
	GetSampleSetTemplateAndSampleList (uuid, sampleSet, onComplete);
}

//*****************************************************************************
void HawkeyeLogicImpl::GetSampleSetTemplateAndSampleList (const uuid__t uuid, SampleSet*& sampleSet, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug2, "GetSampleSetTemplateAndSampleList: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	DBApi::DB_SampleSetRecord dbSampleSet;

	DBApi::eQueryResult dbStatus = DBApi::DbFindSampleSet (dbSampleSet, uuid);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("GetSampleSetTemplateAndSampleList: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::workflow_design,
			instrument_error::severity_level::error));

		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eStorageFault);
		return;
	}

	// Remove any entries that have been marked as deleted.
	for (auto iter = dbSampleSet.SSItemsList.begin(); iter != dbSampleSet.SSItemsList.end();)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetSampleSetTemplateAndSampleList: <SampleSet " + Uuid::ToStr (iter->SampleSetId)+">");

		if (iter->SampleItemStatus == static_cast<uint32_t>(DBApi::eSampleItemStatus::ItemDeleted))
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "GetSampleSetTemplateAndSampleList: <SampleItem " + Uuid::ToStr (iter->SampleItemId) + " was deleted>");
			iter = dbSampleSet.SSItemsList.erase (iter);
			dbSampleSet.SampleItemCount--;
		}
		else
		{
			++ iter;
		}
	}

	SampleSetDLL sampleSetDLL;
	sampleSetDLL.FromDbStyle (dbSampleSet);
	DataConversion::convert_dllType_to_cType (sampleSetDLL, sampleSet);

	Logger::L ().Log (MODULENAME, severity_level::debug2, "GetSampleSetTemplateAndSampleList: <exit>");

	pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eSuccess);
}

//*****************************************************************************
void HawkeyeLogicImpl::DeleteSampleSetTemplate (uuid__t uuid, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "DeleteSampleSetTemplate: <enter>");

	auto  he = HawkeyeError::eSuccess;

	DBApi::eQueryResult dbStatus = DBApi::DbRemoveSampleSetByUuid (uuid);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("GetSampleSetTemplateList: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::workflow_design,
			instrument_error::severity_level::error));

		he = HawkeyeError::eStorageFault;
	}

	pHawkeyeServices_->enqueueExternal (onComplete, he);
}

/******************************************************************************
	Returns SampleSets that meet the search criteria.
	SampleSets with Samples that with completion status: completed or error.

	SampleSet Filter
		username (OwnerID)
		date range (RunDate)
		SampleSet name (SampleSetName)

	Sample Filter
		username (no owner field)
		date range (RunDate)
		Sample name (SampleItemName)
		Tag (ItemLabel)
		CelType (CellTypeID)
******************************************************************************/
//*****************************************************************************
void HawkeyeLogicImpl::GetSampleSetList(
	eFilterItem filterItem,
	uint64_t start,
	uint64_t end,
	std::string username,
	std::string nameSearchString,	// SampleSet: SampleSetNameStr, Sample: SampleItemNameStr
	std::string tagSearchString,	// SampleSet: not applicable, Sample: SampeItemLabel
	std::string cellTypeOrQualityControlName,
	uint32_t skip,
	uint32_t take,
	uint32_t& totalSampleSetsAvailable,
	SampleSet*& sampleSets,
	uint32_t& numSampleSets,
	std::function<void(HawkeyeError)> onComplete)
{
	//NOTE: for testing...
		//filterItem = eFilterItem::eSample;

	std::string sTimeStr;
	std::string eTimeStr;
	ChronoUtilities::getListTimestampAsStr(start, end, sTimeStr, eTimeStr);

	//TODO: not currently supported...
		//// Set appropriate value for DB when not skipping any entries or taking everything.
	int32_t tSkip = skip;
	//if (tSkip == 0 || tSkip < 0)
	//{
	//	tSkip = -1;
	//}
	int32_t tTake = take;
	//if (tTake == 0 || tTake < 0)
	//{
	//	tTake = -1;
	//}

	tSkip = -1;
	tTake = -1;

	uuid__t userUUID = {};

	// Set up filtering criteria.
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOperations = {};
	std::vector<std::string> compareValues = {};

	filterTypes.push_back(DBApi::eListFilterCriteria::RunDateRangeFilter);
	compareOperations.push_back("=");
	compareValues.push_back(boost::str(boost::format("%s;%s") % sTimeStr % eTimeStr));

	if (filterItem == eFilterItem::eSampleSet)
	{ // SampleSet.
		filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
		compareOperations.push_back("=");
		compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleSetStatus::SampleSetComplete));
	}
	else
	{ // SampleItem.
		filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
		compareOperations.push_back(">");
		compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleItemStatus::ItemRunning));

		filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
		compareOperations.push_back("<");
		compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleItemStatus::ItemDeleted));
	}

	if (!username.empty())
	{
		UserList::Instance().GetUserUUIDByName(username, userUUID);
		std::string temp = boost::str(boost::format("'%s'") % Uuid::ToStr(userUUID));
		// The sample item table in the database doesn't have an OwnerID field.
		if (filterItem == eFilterItem::eSampleSet)
		{
			filterTypes.push_back(DBApi::eListFilterCriteria::OwnerFilter);
			compareOperations.push_back("=");
			compareValues.push_back(temp);
		}
	}

	// SampleSet uses SampleSetNameStr and  Sample uses SampleItemNameStr.
	if (nameSearchString.length())
	{
		filterTypes.push_back(DBApi::eListFilterCriteria::ItemNameFilter);
		compareOperations.push_back("~*");	// Substring search.
		std::string temp = boost::str(boost::format("'%s'") % nameSearchString);
		compareValues.push_back(temp);
	}

	if (filterItem == eFilterItem::eSample)
	{
		if (tagSearchString.length())
		{
			filterTypes.push_back(DBApi::eListFilterCriteria::LabelFilter);
			compareOperations.push_back("~*");	// Substring search.
			std::string temp = boost::str(boost::format("'%s'") % tagSearchString);
			compareValues.push_back(temp);
		}

		if (cellTypeOrQualityControlName.length())
		{
			// Determine if QC or Cell Type
			bool cellTypeFilter = true;
			bool qcFilter = false;
			CellTypeDLL celltype = CellTypesDLL::getCellTypeByName(cellTypeOrQualityControlName);
			if (celltype.label.empty())
			{
				uuid__t qcUuid;
				cellTypeFilter = false;
				std::vector<QualityControlDLL>& qcsDLL = QualityControlsDLL::Get();
				for (auto element : qcsDLL)
				{
					if (element.qc_name == cellTypeOrQualityControlName)
					{
						qcFilter = true;
						memcpy(qcUuid.u, element.uuid.u, sizeof(qcUuid.u));
						filterTypes.push_back(DBApi::eListFilterCriteria::QcFilter);
						compareOperations.push_back("=");
						compareValues.push_back(boost::str(boost::format("'%s'") % Uuid::ToStr(qcUuid)));
						break;
					}
				}
			}
			else
			{
				filterTypes.push_back(DBApi::eListFilterCriteria::CellTypeFilter);
				compareOperations.push_back("=");
				// Cell types are stored in the database as signed integers and 
				// user defined cell types are < 0, so if user defined convert like the DB does.
				if (CellTypeDLL::isUserDefined(celltype.celltype_index))
				{
					std::string valStr = boost::str(boost::format("%ld") % (int32_t)celltype.celltype_index);
					compareValues.push_back(valStr);
				}
				else
				{
					compareValues.push_back(std::to_string(celltype.celltype_index));
				}
			}
			if (!cellTypeFilter && !qcFilter)
			{
				Logger::L().Log(MODULENAME, severity_level::warning, "GetSampleSetList: <exit, DbGetSampleList failed, cell type or QC name not found");
				onComplete(HawkeyeError::eEntryNotFound);
				return;
			}
		}
	}

	std::string str = "\nGetSampleSetList: filter criteria...\n";
	for (int idx = 0; idx < filterTypes.size(); idx++)
	{
		str += boost::str(boost::format("%d : %s : %s\n")
			% (int)filterTypes[idx]
			% compareOperations[idx]
			% compareValues[idx]);
	}
	Logger::L().Log(MODULENAME, severity_level::debug1, str);

	Logger::L().Log(MODULENAME, severity_level::debug2, "GetSampleSetList: START QUERY...");

	std::vector<DBApi::DB_SampleSetRecord> dbSampleSetList = {};
	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};

	DBApi::eQueryResult dbStatus = {};

	if (filterItem == eFilterItem::eSampleSet)
	{
		dbStatus = DBApi::DbGetSampleSetListEnhanced(
			dbSampleSetList,
			filterTypes,
			compareOperations,
			compareValues,
			-1,
			DBApi::eListSortCriteria::RunDateSort,
			DBApi::eListSortCriteria::SortNotDefined,
			DBApi::eContainedObjectRetrieval::FirstLevelObjs,
			-1,		// Descending sort order
			"",
			-1,
			-1,
			"");
	}
	else
	{
		dbStatus = DBApi::DbGetSampleItemListEnhanced(
			dbSampleItemList,
			filterTypes,
			compareOperations,
			compareValues,
			-1,
			DBApi::eListSortCriteria::RunDateSort,
			DBApi::eListSortCriteria::SortNotDefined,
			-1,		// Descending sort order
			"",
			-1,
			-1,
			"");
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, "GetSampleSetList: END QUERY...");

	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			onComplete(HawkeyeError::eEntryNotFound);
			return;
		}

		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("GetSampleSetList: <exit, DbGetSampleList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample_set,
			instrument_error::severity_level::error));

		onComplete(HawkeyeError::eStorageFault);
		return;
	}

	std::vector<SampleSetDLL> sampleSetsDLL = {};

	if (filterItem == eFilterItem::eSampleSet)
	{
		for (auto& v : dbSampleSetList)
		{
			//NOTE: save for debugging...  comment out 
			//auto stime = ChronoUtilities::ConvertToString (v.CreateDateTP, "%Y-%m-%d %H:%M:%S");
			//Logger::L().Log (MODULENAME, severity_level::debug1, stime);
			//std::string tUsername = {};
			//UserList::Instance().GetUsernameByUUID (v.OwnerId, tUsername);
			//Logger::L().Log (MODULENAME, severity_level::debug1, "User: " + tUsername);

			Logger::L().Log(MODULENAME, severity_level::debug1, "GetSampleSetList: <SampleSet " + Uuid::ToStr(v.SampleSetId) + ">");

			SampleSetDLL sampleSet;
			sampleSet.FromDbStyle(v);
			// If status is "Running" or "Complete" and sample set is empty, don't include in sample sets
			if ((sampleSet.samples.size() > 0) || ((sampleSet.status != DBApi::eSampleSetStatus::SampleSetRunning) &&
				(sampleSet.status != DBApi::eSampleSetStatus::SampleSetComplete)))
				sampleSetsDLL.push_back(sampleSet);
		}

		Logger::L().Log(MODULENAME, severity_level::debug1,
			boost::str(boost::format("numSampleSets: %d") % sampleSetsDLL.size()));
	}
	else
	{
		std::vector<uuid__t> sampleSetUuids = {};

		// Get the UUIDs of the SampleSets to retrieve.
		for (auto& v : dbSampleItemList)
		{
			// Only return the SampleSet once for multiple samples.
			auto item = std::find_if(
				sampleSetUuids.begin(), sampleSetUuids.end(), [v](const auto& item)
				{ return Uuid(v.SampleSetId) == Uuid(item); });

			if (item == sampleSetUuids.end())
			{
				sampleSetUuids.push_back(v.SampleSetId);
			}
		}

		dbStatus = DBApi::DbGetSampleSetListByUuidList(
			dbSampleSetList,
			sampleSetUuids,
			-1,
			DBApi::eListSortCriteria::CreationDateSort,
			DBApi::eListSortCriteria::SortNotDefined,
			DBApi::eContainedObjectRetrieval::FirstLevelObjs,
			-1,		// Descending sort order
			"",
			-1,
			-1);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("GetSampleSetList: <exit, DbFindSampleSet failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::sample_set,
				instrument_error::severity_level::error));

			onComplete(HawkeyeError::eStorageFault);
			return;
		}

		// Check for valid user name specified (empty username is used for 'all users')
		if (!Uuid::IsClear((userUUID)) || username.empty())
		{
			// Convert to SampleSetDLLs.
			for (auto& v : dbSampleSetList)
			{
				SampleSetDLL sampleSet;
				if (!Uuid::IsClear(userUUID))
				{ // A username was specified for Sample filter.
					if (Uuid(userUUID) == Uuid(v.OwnerId))
					{ // SampleSet username matches the Sample filter username.
						sampleSet.FromDbStyle(v);
						// If status is "Running" or "Complete" and sample set is empty, don't include in sample sets
						if ((sampleSet.samples.size() > 0) || ((sampleSet.status != DBApi::eSampleSetStatus::SampleSetRunning) &&
							sampleSet.status != DBApi::eSampleSetStatus::SampleSetComplete))
							sampleSetsDLL.push_back(sampleSet);
					}
				}
				else
				{
					if (v.SampleSetStatus == (int32_t)DBApi::eSampleSetStatus::SampleSetComplete)
					{
						sampleSet.FromDbStyle(v);
						// If status is "Running" and sample set is empty, don't include in sample sets
						if ((sampleSet.samples.size() > 0) || ((sampleSet.status != DBApi::eSampleSetStatus::SampleSetRunning) &&
							sampleSet.status != DBApi::eSampleSetStatus::SampleSetComplete))
							sampleSetsDLL.push_back(sampleSet);
					}
				}
			}
		}

		Logger::L().Log(MODULENAME, severity_level::debug1,
			boost::str(boost::format("numSampleSets: %d") % sampleSetsDLL.size()));
	}

	if (!sampleSetsDLL.empty())
	{
		DataConversion::convert_vecOfDllType_to_listOfCType(sampleSetsDLL, sampleSets, numSampleSets);
	}

	totalSampleSetsAvailable = numSampleSets;
	onComplete(HawkeyeError::eSuccess);
	return;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetSampleDefinitionBySampleId (uuid__t sampleItemUuid, SampleDefinition*& sampleDef)
{
	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};
	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleItemList(
		dbSampleItemList,
		DBApi::eListFilterCriteria::SampleIdFilter,
		"=",
		boost::str (boost::format ("'%s'") % Uuid::ToStr(sampleItemUuid)),
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1,
		"");
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("GetSampleDefinitionBySampleId: <exit, DbGetSampleItemList failed, status: %ld>") % (int32_t)dbStatus));
		return HawkeyeError::eEntryNotFound;
	}

	SampleDefinitionDLL sdDLL;
	sdDLL.FromDbStyle (dbSampleItemList[0]);

	DataConversion::convert_dllType_to_cType (sdDLL, sampleDef);
	
	return HawkeyeError::eSuccess;
}
