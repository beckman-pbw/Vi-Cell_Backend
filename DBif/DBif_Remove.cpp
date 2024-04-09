// Database interface : implementation file
//

#pragma once

#include "pch.h"


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif




static const std::string MODULENAME = "DBif_Remove";



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// DBifImpl database interface internal private deletion methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DBApi::eQueryResult DBifImpl::DoRecordDelete( DBApi::eListType listtype, DBApi::eListFilterCriteria idfilter, uuid__t recid, int64_t recidnum, int64_t recindex, std::string recname, bool protected_delete_ok )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string whereStr = "";
	std::string selectTag = "";
	std::string idValStr = "";

	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName, selectTag, idValStr, recid, recidnum, recindex, recname ) )
	{
		WriteLogEntry( "DoRecordDelete: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	if ( !GetListFilterString( listtype, idfilter, whereStr, "=", idValStr ) )
	{
		WriteLogEntry( "DoRecordDelete: failed: filter creation failed", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	std::string queryStr = "";

	if ( !MakeObjectRemoveQuery( queryStr, schemaName, tableName, recid, recidnum, recindex, recname, selectTag, whereStr, protected_delete_ok ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	CRecordset resultRec( pDb );
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef DEBUG
	std::string logEntryStr = boost::str( boost::format( "DoRecordDelete: queryStr: '%s'" ) % queryStr );
	WriteLogEntry( logEntryStr, InfoMsgType );
#endif // DEBUG

	queryResult = RunQuery( loginType, queryStr, resultRec );
	if ( queryResult == DBApi::eQueryResult::NoResults )
	{
		queryResult = DBApi::eQueryResult::QueryOk;
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DoRecordListDelete( DBApi::eListType listtype, std::vector<uuid__t> idlist, bool protected_delete_ok )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idValStr = "";
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpId = idlist.at( 0 );	// use to get the ID tag string

	// get the schema and table names, and the ID tag string
	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName, selectTag, idValStr, tmpId ) )
	{
		WriteLogEntry( "DoRecordListDelete: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	std::string queryStr = "";
	std::string whereStr = "";

	queryResult = MakeObjectIdListRemoveQuery( queryStr, schemaName, tableName,
											   selectTag, idlist,
											   whereStr, protected_delete_ok );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		return queryResult;
	}

	CRecordset resultRec( pDb );

	queryResult = RunQuery( loginType, queryStr, resultRec );

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DoRecordListDelete( DBApi::eListType listtype, std::vector<int64_t> idnumlist, bool protected_delete_ok )
{
	// deleting using an idnum list
	size_t idListSize = idnumlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idValStr = "";
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpId = {};
	
	ClearGuid( tmpId );

	// get the schema and table names, and the ID tag string
	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName, selectTag, idValStr, tmpId, idnumlist.at( 0 ) ) )
	{
		WriteLogEntry( "DoRecordListDelete: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	std::string queryStr = "";
	std::string whereStr = "";

	queryResult = MakeObjectIdNumListRemoveQuery( queryStr, schemaName, tableName,
												  selectTag, idnumlist,
												  whereStr, protected_delete_ok );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		return queryResult;
	}

	CRecordset resultRec( pDb );

	queryResult = RunQuery( loginType, queryStr, resultRec );

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}


////////////////////////////////////////////////////////////////////////////////
// DBifImpl database interface internal private record-specific deletion methods
////////////////////////////////////////////////////////////////////////////////

// only a single identifier should be supplied;
// if multiple identifiers are supplied, the highest priority identifier available will be used;

// worklist record deletion will not delete embedded sample-set and sample-item records...
DBApi::eQueryResult DBifImpl::DeleteWorklistRecord( DB_WorklistRecord& wlr )
{
	if ( !GuidValid( wlr.WorklistId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteWorklistRecord( wlr.WorklistId );
}

// worklist record deletion will not delete embedded sample-set and sample-item records...
DBApi::eQueryResult DBifImpl::DeleteWorklistRecord( uuid__t listid )
{
	if ( !GuidValid( listid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::WorklistList, DBApi::eListFilterCriteria::IdFilter, listid );

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteWorklistRecords( std::vector<uuid__t>& idlist )
{
	if ( idlist.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	DBApi::eQueryResult queryResult = DoRecordListDelete( DBApi::eListType::WorklistList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	EndTransaction();
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteWorklistSampleSetRecord( DB_WorklistRecord& wlr, DB_SampleSetRecord& ssr, bool do_tree_delete )
{
	// IDs must be valid && sample set must belong to this worklist...
	if ( !GuidValid( wlr.WorklistId ) ||
		 !GuidValid( ssr.SampleSetId ) ||
		 !GuidsEqual( wlr.WorklistId, ssr.WorklistId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	// check the forward reference from the SampleSet list of items to the specified item...
	for ( auto idIter = wlr.SSList.begin(); idIter < wlr.SSList.end(); ++idIter )
	{
		if ( GuidValid( idIter->SampleSetId ) &&
			 GuidsEqual( idIter->SampleSetId, ssr.SampleSetId ) )
		{
			wlr.SSList.erase( idIter );		// remove the entry from the worklist...
			UpdateWorklistRecord(wlr);
			return DeleteSampleSetRecord( ssr.SampleSetId, do_tree_delete );
		}
	}

	return DBApi::eQueryResult::MissingQueryKey;
}

DBApi::eQueryResult DBifImpl::DeleteWorklistSampleSetRecord( uuid__t listid, uuid__t ssid, bool do_tree_delete )
{
	if ( !GuidValid( listid ) || !GuidValid( ssid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord dbRec = {};

	queryResult = GetWorklist( dbRec, listid, NO_ID_NUM, DBApi::eContainedObjectRetrieval::FirstLevelObjs );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	// check the forward reference from the SampleSet list of items to the specified item...
	for ( auto idIter = dbRec.SSList.begin(); idIter < dbRec.SSList.end(); ++idIter )
	{
		if ( GuidValid( idIter->SampleSetId ) &&
			 GuidsEqual( idIter->SampleSetId, ssid ) )
		{
			dbRec.SSList.erase( idIter );		// remove the entry from the worklist...
			UpdateWorklistRecord( dbRec );
			return DeleteSampleSetRecord( ssid, do_tree_delete );
		}
	}

	return DBApi::eQueryResult::MissingQueryKey;
}

// deletes ONLY the SampleSet records from the supplied idlist that belong to the worklist referenced by the supplied id;
DBApi::eQueryResult DBifImpl::DeleteWorklistSampleSetRecords( uuid__t listid, std::vector<uuid__t>& idlist, bool do_tree_delete )
{
	if ( !GuidValid( listid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord dbRec = {};

	queryResult = GetWorklist( dbRec, listid, NO_ID_NUM, DBApi::eContainedObjectRetrieval::FirstLevelObjs );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	// compile the list of SampleItems to delete by uuid list
	if ( dbRec.SSList.size() <= 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	std::vector<uuid__t> setIdList = {};

	for ( auto idIter = idlist.begin(); idIter < idlist.end(); ++idIter )
	{
		if ( GuidValid( *idIter ) )
		{
			for ( auto setIter = dbRec.SSList.begin(); setIter != dbRec.SSList.end(); ++setIter )
			{
				if ( GuidValid( setIter->SampleSetId ) && GuidsEqual( setIter->SampleSetId, *idIter ) )
				{
					setIdList.push_back( *idIter );
					dbRec.SSList.erase( setIter );		// remove the entry from the worklist...
				}
			}
		}
	}

	if ( setIdList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	UpdateWorklistRecord( dbRec );

	queryResult = DeleteSampleSetRecords( setIdList, do_tree_delete );

	return queryResult;
}

// Currently, the UI is centered on SampleSets, and there is no direct access to individual sample analyses.
// To support complete cleanup, SampleSet deletion will delete all sub-elements, including referenced elements,
// in particular, the SampleProperties objects and the indirectly referenced Analyses objects.  These objects
// will, in turn, delete their sub-objects like image-set, image sequence, and image objects, and result objects
// like SummaryResults, SResults, ImageResults, and DetailedResults.
DBApi::eQueryResult DBifImpl::DeleteSampleSetRecord( DB_SampleSetRecord& ssr, bool do_tree_delete )
{
	if ( !GuidValid( ssr.SampleSetId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

//	return DeleteSampleSetRecord( ssr.SampleSetId, do_tree_delete );
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<uuid__t> itemIdList = {};

	if ( do_tree_delete )
	{
		if ( ssr.SSItemsList.size() > 0 )
		{
			// from each SampleSet record actually retrieved, compile the TOTAL list of owned SampleItems to delete by uuid list
			for ( auto itemIter = ssr.SSItemsList.begin(); itemIter != ssr.SSItemsList.end(); ++itemIter )
			{
				if ( GuidValid( itemIter->SampleItemId ) )
				{
					itemIdList.push_back( itemIter->SampleItemId );
				}
			}
		}
	}

	bool inTransaction = false;
	if ( itemIdList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteSampleItemRecords( itemIdList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	queryResult = DoRecordDelete( DBApi::eListType::SampleSetList, DBApi::eListFilterCriteria::IdFilter, ssr.SampleSetId );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteSampleSetRecord( uuid__t samplesetid, bool do_tree_delete )
{
	if ( !GuidValid( samplesetid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord dbRec = {};

	queryResult = GetSampleSet( dbRec, samplesetid, NO_ID_NUM, DBApi::eContainedObjectRetrieval::AllSubObjs );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteSampleSetRecord( dbRec, do_tree_delete );
}

DBApi::eQueryResult DBifImpl::DeleteSampleSetRecords( std::vector<uuid__t>& idlist, bool do_tree_delete )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DB_SampleSetRecord> ssList = {};

	// get the list of SampleSet records indicated by the uuid list
	queryResult = GetSampleSetList( ssList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	std::vector<uuid__t> ssIdList = {};
	std::vector<uuid__t> itemIdList = {};

	// from each SampleSet record actually retrieved, compile the TOTAL list of owned SampleItems to delete by uuid list
	for ( auto ssIter = ssList.begin(); ssIter != ssList.end(); ++ssIter )
	{
		if ( GuidValid( ssIter->SampleSetId ) )
		{
			ssIdList.push_back( ssIter->SampleSetId );
		}

		if ( ssIter->SSItemsList.size() > 0 )
		{
			// from each SampleSet record actually retrieved, compile the TOTAL list of owned SampleItems to delete by uuid list
			for ( auto itemIter = ssIter->SSItemsList.begin(); itemIter != ssIter->SSItemsList.end(); ++itemIter )
			{
				if ( GuidValid( itemIter->SampleSetId ) )
				{
					itemIdList.push_back( itemIter->SampleItemId );
				}
			}
		}
	}

	bool inTransaction = false;
	if ( itemIdList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteSampleItemRecords( itemIdList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::SampleSetList, ssIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	EndTransaction();
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

// deletes a single SampleItem record IF it belongs to the supplied SampleSet
DBApi::eQueryResult DBifImpl::DeleteSampleSetSampleItemRecord( DB_SampleSetRecord& ssr, DB_SampleItemRecord& sir )
{
	if ( !GuidValid( ssr.SampleSetId ) ||
		 !GuidValid( sir.SampleItemId ) ||
		 !GuidsEqual( ssr.SampleSetId, sir.SampleSetId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	// check the forward reference from the SampleSet list of items to the specified item...
	for ( auto itemIter = ssr.SSItemsList.begin(); itemIter < ssr.SSItemsList.end(); ++itemIter )
	{
		if ( GuidValid( itemIter->SampleItemId ) && GuidsEqual( itemIter->SampleItemId, sir.SampleItemId ) )
		{
			ssr.SSItemsList.erase( itemIter );		// remove the entry from the sample set...
			UpdateSampleSetRecord( ssr );
			return DeleteSampleItemRecord( sir.SampleItemId );
		}
	}

	return DBApi::eQueryResult::MissingQueryKey;
}

DBApi::eQueryResult DBifImpl::DeleteSampleSetSampleItemRecord( uuid__t ssid, uuid__t ssitemid )
{
	if ( !GuidValid( ssid ) || !GuidValid( ssitemid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord dbRec = {};

	queryResult = GetSampleSet( dbRec, ssid, NO_ID_NUM, DBApi::eContainedObjectRetrieval::AllSubObjs );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	// check the forward reference from the SampleSet list of items to the specified item...
	for ( auto itemIter = dbRec.SSItemsList.begin(); itemIter < dbRec.SSItemsList.end(); ++itemIter )
	{
		if ( GuidValid( itemIter->SampleItemId ) && GuidsEqual( itemIter->SampleItemId, ssitemid ) )
		{
			dbRec.SSItemsList.erase( itemIter );
			UpdateSampleSetRecord( dbRec );
			return DeleteSampleItemRecord( ssitemid );
		}
	}

	return DBApi::eQueryResult::MissingQueryKey;
}

DBApi::eQueryResult DBifImpl::DeleteSampleSetSampleItemRecords( uuid__t ssid, std::vector<uuid__t>& idlist )
{
	if ( !GuidValid( ssid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord dbRec = {};

	queryResult = GetSampleSet( dbRec, ssid, NO_ID_NUM, DBApi::eContainedObjectRetrieval::AllSubObjs );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	std::vector< uuid__t > itemIdList = {};

	// check the forward reference from the SampleSet list of items to the specified item...
	for ( auto itemIter = dbRec.SSItemsList.begin(); itemIter < dbRec.SSItemsList.end(); ++itemIter )
	{
		uuid__t itemId = itemIter->SampleItemId;
		if ( GuidValid( itemId ) )
		{
			for ( auto idIter = idlist.begin(); idIter < idlist.end(); ++idIter )
			{
				if ( GuidValid( *idIter ) && GuidsEqual( itemId, *idIter ) )
				{
					dbRec.SSItemsList.erase( itemIter );
					itemIdList.push_back( itemId );
				}
			}
		}
	}

	if ( itemIdList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	UpdateSampleSetRecord( dbRec );

	return DeleteSampleItemRecords( itemIdList, false );
}

DBApi::eQueryResult DBifImpl::DeleteSampleItemRecord( DB_SampleItemRecord& sir )
{
	if ( !GuidValid( sir.SampleItemId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteSampleItemRecord( sir.SampleItemId );
}

DBApi::eQueryResult DBifImpl::DeleteSampleItemRecord( uuid__t itemid )
{
	if ( !GuidValid( itemid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::SampleSetList, DBApi::eListFilterCriteria::IdFilter, itemid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteSampleItemRecords( std::vector<uuid__t>& idlist, bool in_ext_transaction )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::SampleItemList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			EndTransaction();
		}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

// NOTE that data tree traversal for this record-type is enabled by default, but currently NOT selectable through the exposed API
DBApi::eQueryResult DBifImpl::DeleteSampleRecord( DB_SampleRecord& sr, bool do_tree_delete )
{
	if ( !GuidValid( sr.SampleId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	bool inTransaction = false;

	if ( do_tree_delete )
	{
		if ( GuidValid( sr.ImageSetId ) )
		{
#ifdef HANDLE_TRANSACTIONS
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
#endif // HANDLE_TRANSACTIONS

			queryResult = DeleteImageSetRecord( sr.ImageSetId, true );
			if ( queryResult != DBApi::eQueryResult::QueryOk )
			{
#ifdef HANDLE_TRANSACTIONS
				CancelTransaction();
#endif // HANDLE_TRANSACTIONS

				return queryResult;
			}
		}
	}

	queryResult = DoRecordDelete( DBApi::eListType::SampleList, DBApi::eListFilterCriteria::IdFilter, sr.SampleId );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

// NOTE that data tree traversal for this record-type is enabled by default, but currently NOT selectable through the exposed API
DBApi::eQueryResult DBifImpl::DeleteSampleRecord( uuid__t sampleid, bool do_tree_delete )
{
	if ( !GuidValid( sampleid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleRecord dbRec = {};

	queryResult = GetSample( dbRec, sampleid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteSampleRecord( dbRec, do_tree_delete );
}

// NOTE that data tree traversal for this record-type is enabled by default, but currently NOT selectable through the exposed API
DBApi::eQueryResult DBifImpl::DeleteSampleRecords( std::vector<uuid__t>& idlist, bool do_tree_delete )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<uuid__t> setIdList = {};
	std::vector<uuid__t> sampleIdList = {};
	std::vector<DB_SampleRecord> sampleList = {};

	// get the list of SampleItem records indicated by the uuid list
	queryResult = GetSampleList( sampleList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	// for each SampleItem record, compile the list of ALL sample records and analysis records to delete by uuid list
	for ( auto sampleIter = sampleList.begin(); sampleIter != sampleList.end(); ++sampleIter )
	{
		if ( GuidValid( sampleIter->SampleId ) )
		{
			sampleIdList.push_back( sampleIter->SampleId );
		}

		if ( do_tree_delete )
		{
			if ( GuidValid( sampleIter->ImageSetId ) )
			{
				setIdList.push_back( sampleIter->ImageSetId );
			}
		}
	}

	bool inTransaction = false;
	if ( do_tree_delete )
	{
		if ( setIdList.size() > 0 )
		{
#ifdef HANDLE_TRANSACTIONS
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
#endif // HANDLE_TRANSACTIONS

			DeleteImageSetRecords( true, setIdList );
			if ( queryResult != DBApi::eQueryResult::QueryOk &&
				 queryResult != DBApi::eQueryResult::NoTargets &&
				 queryResult != DBApi::eQueryResult::NoResults )
			{
#ifdef HANDLE_TRANSACTIONS
				CancelTransaction();
#endif // HANDLE_TRANSACTIONS

				return queryResult;
			}
		}
	}

	if ( sampleIdList.size() == 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			EndTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::SampleList, sampleIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	EndTransaction();
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisRecord( DB_AnalysisRecord& ar, bool do_tree_delete )
{
	if ( !GuidValid( ar.AnalysisId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	bool inTransaction = false;
	if ( do_tree_delete )
	{
		if ( GuidValid( ar.SResultId ) )
		{
#ifdef HANDLE_TRANSACTIONS
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
#endif // HANDLE_TRANSACTIONS

			queryResult = DeleteSResultRecord( ar.SResultId, true );
			if ( queryResult != DBApi::eQueryResult::QueryOk )
			{
#ifdef HANDLE_TRANSACTIONS
				CancelTransaction();
#endif // HANDLE_TRANSACTIONS

				return queryResult;
			}
		}

		if ( GuidValid( ar.SummaryResult.SummaryResultId ) )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !inTransaction )
			{
				BeginTransaction( DBApi::eLoginType::AnyLoginType );
				inTransaction = true;
			}
#endif // HANDLE_TRANSACTIONS

			queryResult = DeleteSummaryResultRecord( ar.SummaryResult.SummaryResultId );
			if ( queryResult != DBApi::eQueryResult::QueryOk )
			{
#ifdef HANDLE_TRANSACTIONS
				CancelTransaction();
#endif // HANDLE_TRANSACTIONS

				return queryResult;
			}
		}
	}

	queryResult = DoRecordDelete( DBApi::eListType::AnalysisList, DBApi::eListFilterCriteria::IdFilter, ar.AnalysisId );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisRecord( uuid__t analysisid, bool do_tree_delete )
{
	if ( !GuidValid( analysisid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_AnalysisRecord dbRec = {};

	queryResult = GetAnalysis( dbRec, analysisid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteAnalysisRecord( dbRec, do_tree_delete );
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisRecordsBySampleId( uuid__t sampleid, bool do_tree_delete )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<DB_AnalysisRecord> anList = {};
	std::string idStr = "";
	uuid__t_to_DB_UUID_Str( sampleid, idStr );

	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::SampleIdFilter;
	std::string compareOp = "=";

	queryResult = GetAnalysisList( anList, filterType, compareOp, idStr );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	if ( anList.size() > 0 )
	{
		std::vector<uuid__t> anIdList = {};

		for ( auto anIter = anList.begin(); anIter != anList.end(); ++anIter )
		{
			uuid__t anId = anIter->AnalysisId;

			anIdList.push_back( anId );
		}

		queryResult = DeleteAnalysisRecords( anIdList, do_tree_delete );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
			return queryResult;
		}
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisRecords( std::vector<uuid__t>& idlist, bool do_tree_delete )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<uuid__t> anIdList = {};
	std::vector<uuid__t> summaryIdList = {};
	std::vector<uuid__t> sresultIdList = {};
	std::vector<DB_AnalysisRecord> anList = {};

	// get the list of Analysis records indicated by the uuid list
	queryResult = GetAnalysisList( anList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	for ( auto anIter = anList.begin(); anIter != anList.end(); ++anIter )
	{
		uuid__t anId = anIter->AnalysisId;

		anIdList.push_back( anId );

		if ( GuidValid( anIter->SummaryResult.SummaryResultId ) )
		{
			summaryIdList.push_back( anIter->SummaryResult.SummaryResultId );
		}

		if ( do_tree_delete )
		{
			if ( GuidValid( anIter->SResultId ) )
			{
				sresultIdList.push_back( anIter->SResultId );
			}
		}
	}

	bool inTransaction = false;
	if ( summaryIdList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		DeleteSummaryResultRecords( summaryIdList, inTransaction );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	if ( do_tree_delete )
	{
		if ( sresultIdList.size() > 0 )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !inTransaction )
			{
				BeginTransaction( DBApi::eLoginType::AnyLoginType );
				inTransaction = true;
			}
#endif // HANDLE_TRANSACTIONS

			DeleteSResultRecords( true, sresultIdList );
			if ( queryResult != DBApi::eQueryResult::QueryOk &&
				 queryResult != DBApi::eQueryResult::NoTargets &&
				 queryResult != DBApi::eQueryResult::NoResults )
			{
#ifdef HANDLE_TRANSACTIONS
				CancelTransaction();
#endif // HANDLE_TRANSACTIONS

				return queryResult;
			}
		}
	}

	if ( anIdList.size() == 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( inTransaction )
		{
			EndTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::AnalysisList, anIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	EndTransaction();
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteSummaryResultRecord( DB_SummaryResultRecord& sr )
{
	if ( !GuidValid( sr.SummaryResultId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteSummaryResultRecord( sr.SummaryResultId );
}

DBApi::eQueryResult DBifImpl::DeleteSummaryResultRecord( uuid__t resultid )
{
	if ( !GuidValid( resultid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::SummaryResultList, DBApi::eListFilterCriteria::IdFilter, resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteSummaryResultRecords( std::vector<uuid__t>& idlist, bool in_ext_transaction )
{
	if ( idlist.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	bool inTransaction = in_ext_transaction;
#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
}
#endif // HANDLE_TRANSACTIONS

	DBApi::eQueryResult queryResult = DoRecordListDelete( DBApi::eListType::SummaryResultList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteDetailedResultRecord( DB_DetailedResultRecord& dr )
{
	if ( !GuidValid( dr.DetailedResultId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteDetailedResultRecord( dr.DetailedResultId );
}

DBApi::eQueryResult DBifImpl::DeleteDetailedResultRecord( uuid__t resultid )
{
	if ( !GuidValid( resultid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DoRecordDelete( DBApi::eListType::DetailedResultList, DBApi::eListFilterCriteria::IdFilter, resultid );
}

DBApi::eQueryResult DBifImpl::DeleteDetailedResultRecords( std::vector<uuid__t>& idlist, bool in_ext_transaction )
{
	if ( idlist.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
	}
#endif // HANDLE_TRANSACTIONS

	DBApi::eQueryResult queryResult = DoRecordListDelete( DBApi::eListType::DetailedResultList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteImageResultRecord( DB_ImageResultRecord& ir, bool in_ext_transaction )
{
	if ( !GuidValid( ir.ResultId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	bool inTransaction = in_ext_transaction;
	if ( GuidValid( ir.DetailedResult.DetailedResultId ) )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteDetailedResultRecord( ir.DetailedResult.DetailedResultId );
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	queryResult = DoRecordDelete( DBApi::eListType::ImageResultList, DBApi::eListFilterCriteria::IdFilter, ir.ResultId );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction && !in_ext_transaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteImageResultRecord( uuid__t resultid, bool in_ext_transaction )
{
	if ( !GuidValid( resultid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageResultRecord dbRec = {};

	// check that the record exists; this may not really be necessary...
	queryResult = GetImageResult( dbRec, resultid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteImageResultRecord( dbRec, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteImageResultRecords( std::vector<DB_ImageResultRecord>& irlist, bool in_ext_transaction )
{
	// deleting using a uuid list
	size_t irListSize = irlist.size();

	if ( irListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<uuid__t> irIdList = {};
	std::vector<uuid__t> drIdList = {};

	// use the complete record lists to alid redundant object list retrieval
	// for each image result record, extract the detailed result record id and add to the list
	for ( auto irIter = irlist.begin(); irIter != irlist.end(); ++irIter )
	{
		if ( GuidValid( irIter->ResultId ) )
		{
			irIdList.push_back( irIter->ResultId );
		}

		if ( GuidValid( irIter->DetailedResult.DetailedResultId ) )
		{
			drIdList.push_back( irIter->DetailedResult.DetailedResultId );
		}
	}

	bool inTransaction = in_ext_transaction;
	if ( drIdList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteDetailedResultRecords( drIdList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	if ( irIdList.size() == 0 )
	{

#ifdef HANDLE_TRANSACTIONS
		if ( inTransaction && !in_ext_transaction )
		{
			EndTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::ImageResultList, irIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteImageResultRecords( bool in_ext_transaction, std::vector<uuid__t>& idlist )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DB_ImageResultRecord> irList = {};

	// get the list of ImageResult records indicated by the uuid list
	queryResult = GetImageResultList( irList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DeleteImageResultRecords( irList, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteSResultRecord( DB_SResultRecord& sr, bool in_ext_transaction )
{
	if ( !GuidValid( sr.SResultId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<uuid__t> detailedResultIdList = {};
	std::vector<uuid__t> imgResultIdList = {};

	if ( GuidValid( sr.CumulativeDetailedResult.DetailedResultId ) )
	{
		detailedResultIdList.push_back( sr.CumulativeDetailedResult.DetailedResultId );
	}

	bool inTransaction = in_ext_transaction;
	if ( GuidValid( sr.CumulativeDetailedResult.DetailedResultId ) )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteDetailedResultRecord( sr.CumulativeDetailedResult );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	if ( sr.ImageResultList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteImageResultRecords( sr.ImageResultList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordDelete( DBApi::eListType::SResultList, DBApi::eListFilterCriteria::IdFilter, sr.SResultId, sr.SResultIdNum );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction && !in_ext_transaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteSResultRecord( uuid__t resultid, bool in_ext_transaction )
{
	if ( !GuidValid( resultid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SResultRecord dbRec = {};

	queryResult = GetSResult( dbRec, resultid, NO_ID_NUM );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteSResultRecord( dbRec, in_ext_transaction );
}

// SResult objects have nested sub-objects and sub-object lists; those must be handled as individual record deletes...
DBApi::eQueryResult DBifImpl::DeleteSResultRecords( std::vector<DB_SResultRecord>& srlist, bool in_ext_transaction )
{
	// deleting using a list of records
	size_t srListSize = srlist.size();

	if ( srListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<uuid__t> detailedResultIdList = {};
	std::vector<uuid__t> imgResultIdList = {};

	bool inTransaction = in_ext_transaction;
	// for each sresult record, perform the delete with associated deletion of ne=sted sub-components...
	for ( auto srIter = srlist.begin(); srIter != srlist.end(); ++srIter )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteSResultRecord( *srIter, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
				queryResult != DBApi::eQueryResult::NoTargets &&
				queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
		srlist.erase( srIter );
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteSResultRecords(  bool in_ext_transaction, std::vector<uuid__t>& idlist )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DB_SResultRecord> srList = {};
	std::vector<uuid__t> detailedResultIdList = {};
	std::vector<uuid__t> imgResultIdList = {};

	// get the list of SResult records indicated by the uuid list
	queryResult = GetSResultList( srList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DeleteSResultRecords( srList, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteImageSetRecord( DB_ImageSetRecord& isr, bool in_ext_transaction )
{
	if ( !GuidValid( isr.ImageSetId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	bool inTransaction = in_ext_transaction;
	if ( isr.ImageSequenceList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteImageSeqRecords( isr.ImageSequenceList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordDelete( DBApi::eListType::ImageSetList, DBApi::eListFilterCriteria::IdFilter, isr.ImageSetId, isr.ImageSetIdNum );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction && !in_ext_transaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteImageSetRecord( uuid__t imagesetid, bool in_ext_transaction )
{
	if ( !GuidValid( imagesetid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageSetRecord dbRec = {};

	queryResult = GetImageSet( dbRec, imagesetid, NO_ID_NUM );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteImageSetRecord( dbRec, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteImageSetRecords( std::vector<DB_ImageSetRecord>& islist, bool in_ext_transaction )
{
	// deleting using a uuid list
	size_t isListSize = islist.size();

	if ( isListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<uuid__t> isIdList = {};
	std::vector<uuid__t> seqIdList = {};

	bool inTransaction = in_ext_transaction;
	for ( auto isIter = islist.begin(); isIter != islist.end(); ++isIter )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteImageSeqRecords( isIter->ImageSequenceList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::ImageSetList, isIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS


	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteImageSetRecords( bool in_ext_transaction, std::vector<uuid__t>& idlist )
{
	// deleting using a uuid list
	size_t idListSize = idlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<uuid__t> isIdList = {};
	std::vector<uuid__t> seqIdList = {};
	std::vector<DB_ImageSetRecord> isList = {};

	// get the list of ImageSet records indicated by the uuid list
	queryResult = GetImageSetList( isList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DeleteImageSetRecords( isList, in_ext_transaction );
}

DBApi::eQueryResult	DBifImpl::DeleteImageSeqRecord( DB_ImageSeqRecord & isr, bool in_ext_transaction )
{
	if ( !GuidValid( isr.ImageSequenceId ) && isr.ImageSequenceIdNum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	bool inTransaction = in_ext_transaction;
	if ( isr.ImageList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteImageRecords( isr.ImageList, true );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	queryResult = DoRecordDelete( DBApi::eListType::ImageSequenceList, DBApi::eListFilterCriteria::IdFilter, isr.ImageSequenceId, isr.ImageSequenceIdNum );

#ifdef HANDLE_TRANSACTIONS
	if ( inTransaction && !in_ext_transaction )
	{
		if ( queryResult != DBApi::eQueryResult::QueryOk )
		{
			CancelTransaction();
		}
		else
		{
			EndTransaction();
		}
	}
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteImageSeqRecord( uuid__t imageseqid, bool in_ext_transaction )
{
	if ( !GuidValid( imageseqid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageSeqRecord dbRec = {};

	queryResult = GetImageSequence( dbRec, imageseqid, NO_ID_NUM );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	return DeleteImageSeqRecord( dbRec, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteImageSeqRecords( std::vector<DB_ImageSeqRecord>& seqlist, bool in_ext_transaction )
{
	// deleting using a uuid list
	size_t seqListSize = seqlist.size();

	if ( seqListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<uuid__t> imgIdList = {};
	std::vector<uuid__t> seqIdList = {};

	// for each sequence record, compile the list of owend images to delete by uuid list
	for ( auto seqIter = seqlist.begin(); seqIter != seqlist.end(); ++seqIter )
	{
		seqIdList.push_back( seqIter->ImageSequenceId );

		if ( seqIter->ImageList.size() > 0 )
		{
			for ( auto imgIter = seqIter->ImageList.begin(); imgIter != seqIter->ImageList.end(); ++imgIter )
			{
				imgIdList.push_back( imgIter->ImageId );
			}
		}
	}

	bool inTransaction = in_ext_transaction;
	if ( imgIdList.size() > 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !inTransaction )
		{
			BeginTransaction( DBApi::eLoginType::AnyLoginType );
			inTransaction = true;
		}
#endif // HANDLE_TRANSACTIONS

		queryResult = DeleteImageRecords( true, imgIdList );
		if ( queryResult != DBApi::eQueryResult::QueryOk &&
			 queryResult != DBApi::eQueryResult::NoTargets &&
			 queryResult != DBApi::eQueryResult::NoResults )
		{
#ifdef HANDLE_TRANSACTIONS
			if ( !in_ext_transaction )
			{
				CancelTransaction();
			}
#endif // HANDLE_TRANSACTIONS

			return queryResult;
		}
	}

	if ( seqIdList.size() == 0 )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( inTransaction && !in_ext_transaction )
		{
			EndTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !inTransaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
		inTransaction = true;
	}
#endif // HANDLE_TRANSACTIONS

	queryResult = DoRecordListDelete( DBApi::eListType::ImageSequenceList, seqIdList );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteImageSeqRecords( bool in_ext_transaction, std::vector<uuid__t>& seqidlist )
{
	// deleting using a uuid list
	size_t idListSize = seqidlist.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<DB_ImageSeqRecord> seqList = {};

	// get the list of sequence records indicated by the uuid list
	queryResult = GetImageSequenceList( seqList, seqidlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DeleteImageSeqRecords( seqList, in_ext_transaction );
}

DBApi::eQueryResult DBifImpl::DeleteImageRecord( DB_ImageRecord& imr )
{
	if ( !GuidValid( imr.ImageId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteImageRecord( imr.ImageId );
}

DBApi::eQueryResult DBifImpl::DeleteImageRecord( uuid__t imageid )
{
	if ( !GuidValid( imageid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::ImageList, DBApi::eListFilterCriteria::IdFilter, imageid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteImageRecords( std::vector<DB_ImageRecord>& imglist, bool in_ext_transaction )
{
	if ( imglist.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	std::vector<uuid__t> imgIdList = {};

	for ( auto imgIter = imglist.begin(); imgIter != imglist.end(); ++imgIter )
	{
		if ( GuidValid( imgIter->ImageId ) )
		{
			imgIdList.push_back( imgIter->ImageId );
		}
	}

	return DeleteImageRecords( in_ext_transaction, imgIdList );
}

DBApi::eQueryResult DBifImpl::DeleteImageRecords( bool in_ext_transaction, std::vector<uuid__t>& idlist )
{
	if ( idlist.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )
	{
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
	}
#endif // HANDLE_TRANSACTIONS

	DBApi::eQueryResult queryResult = DoRecordListDelete( DBApi::eListType::ImageList, idlist );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
#ifdef HANDLE_TRANSACTIONS
		if ( !in_ext_transaction )		// don't cancel here if started elsewhere
		{
			CancelTransaction();
		}
#endif // HANDLE_TRANSACTIONS

		return queryResult;
	}

#ifdef HANDLE_TRANSACTIONS
	if ( !in_ext_transaction )		// don't cancel here if started elsewhere
	{
		EndTransaction();
	}
#endif // HANDLE_TRANSACTIONS

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteCellTypeRecord( DB_CellTypeRecord& ctr )
{
	if ( !GuidValid( ctr.CellTypeId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteCellTypeRecord( ctr.CellTypeId, static_cast<int64_t>(ctr.CellTypeIndex) );
}

DBApi::eQueryResult DBifImpl::DeleteCellTypeRecord( uuid__t celltypeid, int64_t celltypeindex )
{
	if ( !GuidValid( celltypeid ) && celltypeindex <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	DB_CellTypeRecord dbRec = {};

	// check that the record exists
	queryResult = GetCellType( dbRec, celltypeid, celltypeindex );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	// check that it is not a factory cell type;
	// don't allow factory cell types to be 'deleted/retired'
	if ( dbRec.CellTypeIndex < UserCellTypeStartIndex )
	{
		// QUESTION: is this an error or do we just ignore the request? for now just ignore it
		return DBApi::eQueryResult::QueryOk;
	}

	dbRec.Retired = true;

	return UpdateCellTypeRecord( dbRec );
}

DBApi::eQueryResult DBifImpl::DeleteImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& apr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteImageAnalysisParamRecord( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& aisr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisInputSettingsRecord( uuid__t aisrfid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& adr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisDefinitionRecord( uuid__t defid, int32_t defindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisParamRecord( DB_AnalysisParamRecord& apr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteAnalysisParamRecord( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteIlluninatorRecord( DB_IlluminatorRecord& ilr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteIlluninatorRecord( std::string ilname, int32_t ilindex, int64_t ilidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

// user records are NEVER deleted; they are marked as 'retired' but kept to ensure no user is created with the same name
DBApi::eQueryResult DBifImpl::DeleteUserRecord( DB_UserRecord& ur )
{
	if ( !GuidValid( ur.UserId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eUserType user_type = DBApi::eUserType::LocalUsers;

	if ( ur.ADUser )
	{
		user_type = DBApi::eUserType::AdUsers;
	}

	return DeleteUserRecord( ur.UserId, ur.UserNameStr, user_type );
}

// user records are NEVER deleted; they are marked as 'retired' but kept to ensure no user is created with the same name
DBApi::eQueryResult DBifImpl::DeleteUserRecord( uuid__t userid, std::string username, DBApi::eUserType user_type )
{
	if ( !GuidValid( userid ) && username.length() <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_UserRecord dbRec = {};

	// check that the record exists; this may not really be necessary...
	queryResult = GetUser( dbRec, userid, username, user_type );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryOk;
		}
		return DBApi::eQueryResult::QueryFailed;
	}

	dbRec.Retired = true;

	return UpdateUserRecord( dbRec );
}

DBApi::eQueryResult DBifImpl::DeleteRoleRecord( DB_UserRoleRecord& rr )
{
	if ( !GuidValid( rr.RoleId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteRoleRecord( rr.RoleId, rr.RoleNameStr );
}

DBApi::eQueryResult DBifImpl::DeleteRoleRecord( uuid__t roleid, std::string rolename )
{
	if ( !GuidValid( roleid ) && rolename.length() <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::RolesList, DBApi::eListFilterCriteria::IdFilter, roleid, NO_ID_NUM, INVALID_INDEX, rolename );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteUserPropertyRecord( DB_UserPropertiesRecord& upr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteUserPropertyRecord( int32_t upindex, std::string upname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteSignatureRecord( DB_SignatureRecord& sig )
{
	if ( !GuidValid( sig.SignatureDefId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteSignatureRecord( sig.SignatureDefId );
}

DBApi::eQueryResult DBifImpl::DeleteSignatureRecord( uuid__t sigid )
{
	if ( !GuidValid( sigid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::SignatureDefList, DBApi::eListFilterCriteria::IdFilter, sigid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteReagentTypeRecord( DB_ReagentTypeRecord& rxr )
{
	if ( rxr.ReagentIdNum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteReagentTypeRecord( rxr.ReagentIdNum );
}

DBApi::eQueryResult DBifImpl::DeleteReagentTypeRecord( int64_t rxidnum )
{
	if ( rxidnum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpId = {};

	ClearGuid( tmpId );

	queryResult = DoRecordDelete( DBApi::eListType::ReagentInfoList, DBApi::eListFilterCriteria::IdNumFilter, tmpId, rxidnum );

	return queryResult;
}

//This is intended to delete only a single record;  will fail if multiple records are found
DBApi::eQueryResult DBifImpl::DeleteReagentTypeRecord( std::string tagid )
{
	if ( tagid.length() <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DB_ReagentTypeRecord> rxList = {};

	// get the list of ReagentType records with the matching Rfid TagSn field
	queryResult = GetReagentTypeList( rxList, DBApi::eListFilterCriteria::ItemNameFilter, "=", tagid );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	if ( rxList.size() > 1 )
	{
		return DBApi::eQueryResult::MultipleObjectsFound;
	}

	uuid__t tmpId = {};

	ClearGuid( tmpId );

	queryResult = DoRecordDelete( DBApi::eListType::ReagentInfoList, DBApi::eListFilterCriteria::ItemNameFilter, tmpId, NO_ID_NUM, INVALID_INDEX, tagid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteReagentTypeRecords( std::vector<int64_t> idnum_list )
{
	size_t listSize = idnum_list.size();

	// deleting using a idnum list
	if ( listSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<int64_t> idNumList = {};
	std::vector<DB_ReagentTypeRecord> rxList = {};

	// get the list of ReagentType records for the supplied list; if not found and in the returned list, no delete will be required...
	queryResult = GetReagentTypeList( rxList, idnum_list );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	// for each record, extract the id number into a list to be deleted
	for ( auto idIter = rxList.begin(); idIter != rxList.end(); ++idIter )
	{
		if ( idIter->ReagentIdNum > 0 )
		{
			for ( int i = 0; i < listSize; i++ )
			{
				if ( idIter->ReagentIdNum == idnum_list.at( i ) )
				{
					idNumList.push_back( idIter->ReagentIdNum );
				}
			}
		}
	}

	if ( idNumList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	queryResult = DoRecordListDelete( DBApi::eListType::ReagentInfoList, idNumList, DELETE_PROTECTED_ALLOWED );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteReagentTypeRecords( std::string tagid )
{
	// deleting using a idnum list
	if ( tagid.length() == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<int64_t> idNumList = {};
	std::vector<DB_ReagentTypeRecord> rxList = {};

	// get the list of ReagentType records with the matching Rfid TagSn field
	queryResult = GetReagentTypeList( rxList, DBApi::eListFilterCriteria::ItemNameFilter, "=", tagid );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	// for each record, extract the id number into a list to be deleted
	for ( auto idIter = rxList.begin(); idIter != rxList.end(); ++idIter )
	{
		if ( idIter->ReagentIdNum > 0 )
		{
			idNumList.push_back( idIter->ReagentIdNum );
		}
	}

	if ( idNumList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	queryResult = DoRecordListDelete( DBApi::eListType::ReagentInfoList, idNumList, DELETE_PROTECTED_ALLOWED );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteReagentTypeLotRecords( std::string lot_num )
{
	// deleting using a idnum list
	if ( lot_num.length() == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<int64_t> idNumList = {};
	std::vector<DB_ReagentTypeRecord> rxList = {};

	// get the list of ReagentType records with the matching Rfid TagSn field
	queryResult = GetReagentTypeList( rxList, DBApi::eListFilterCriteria::LotFilter, "=", lot_num );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	// for each record, extract the id number into a list to be deleted
	for ( auto idIter = rxList.begin(); idIter != rxList.end(); ++idIter )
	{
		if ( idIter->ReagentIdNum > 0 )
		{
			idNumList.push_back( idIter->ReagentIdNum );
		}
	}

	if ( idNumList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	queryResult = DoRecordListDelete( DBApi::eListType::ReagentInfoList, idNumList, DELETE_PROTECTED_ALLOWED );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteBioProcessRecord( DB_BioProcessRecord& bpr )
{
	if ( !GuidValid( bpr.BioProcessId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteBioProcessRecord( bpr.BioProcessId );
}

DBApi::eQueryResult DBifImpl::DeleteBioProcessRecord( uuid__t bpid )
{
	if ( !GuidValid( bpid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::BioProcessList, DBApi::eListFilterCriteria::IdFilter, bpid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteQcProcessRecord( DB_QcProcessRecord& qcr )
{
	if ( !GuidValid( qcr.QcId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteQcProcessRecord( qcr.QcId );
}

DBApi::eQueryResult DBifImpl::DeleteQcProcessRecord( uuid__t qcid )
{
	if ( !GuidValid( qcid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::QcProcessList, DBApi::eListFilterCriteria::IdFilter, qcid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteCalibrationRecord( DB_CalibrationRecord& car )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::DeleteCalibrationRecord( uuid__t calid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if (!GuidValid(calid))
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	queryResult = DoRecordDelete(DBApi::eListType::CalibrationsList, DBApi::eListFilterCriteria::IdFilter, calid, NO_ID_NUM, INVALID_INDEX, "", true);

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::DeleteInstConfigRecord( DB_InstrumentConfigRecord& cfr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::DeleteInstConfigRecord( std::string instsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DeleteLogEntryRecords( std::vector<DB_LogEntryRecord>& entrylist )
{
	if ( entrylist.size() == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	std::vector<int64_t> idNumList = {};

	// for each SampleItem record, compile the list of ALL sample records and analysis records to delete by uuid list
	for ( auto idIter = entrylist.begin(); idIter != entrylist.end(); ++idIter )
	{
		if ( idIter->IdNum > 0 )
		{
			idNumList.push_back( idIter->IdNum );
		}
	}

	return DeleteLogEntryRecords( idNumList );
}

DBApi::eQueryResult DBifImpl::DeleteLogEntryRecords( std::vector<int64_t>& idnum_list )
{
	size_t listSize = idnum_list.size();

	// deleting using a idnum list
	if ( listSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	// delete functions require admin access privileges.
	loginType = ConnectionCheck( pDb, DBApi::eLoginType::AdminLoginType );

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	std::vector<int64_t> idNumList = {};
	std::vector<DB_LogEntryRecord> logList = {};

	// get the list of SampleItem records indicated by the idnum list
	queryResult = GetLogList( logList, idnum_list );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	// for each LogEntry record, check against the list of requested id  numbers to add to the effective deletion list
	for ( auto idIter = logList.begin(); idIter != logList.end(); ++idIter )
	{
		if ( idIter->IdNum > 0 )
		{
			for ( int i = 0; i < listSize; i++ )
			{
				if ( idIter->IdNum == idnum_list.at( i ) )
				{
					idNumList.push_back( idIter->IdNum );
				}
			}
		}
	}

	if ( idNumList.size() == 0 )
	{
		return DBApi::eQueryResult::QueryOk;
	}

	queryResult = DoRecordListDelete( DBApi::eListType::LogEntryList, idNumList, DELETE_PROTECTED_ALLOWED );
	if ( queryResult != DBApi::eQueryResult::QueryOk &&
		 queryResult != DBApi::eQueryResult::NoTargets &&
		 queryResult != DBApi::eQueryResult::NoResults )
	{
		return queryResult;
	}

	return DBApi::eQueryResult::QueryOk;
}

DBApi::eQueryResult DBifImpl::DeleteSchedulerConfigRecord( DB_SchedulerConfigRecord& scr )
{
	if ( !GuidValid( scr.ConfigId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	return DeleteSchedulerConfigRecord( scr.ConfigId );
}

DBApi::eQueryResult DBifImpl::DeleteSchedulerConfigRecord( uuid__t scid )
{
	if ( !GuidValid( scid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DoRecordDelete( DBApi::eListType::SchedulerConfigList, DBApi::eListFilterCriteria::IdFilter, scid );

	return queryResult;
}
