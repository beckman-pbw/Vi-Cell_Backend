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




static const std::string MODULENAME = "DBif_RetrieveList";



////////////////////////////////////////////////////////////////////////////////
// Internal list retrieval and parsing methods
////////////////////////////////////////////////////////////////////////////////

DBApi::eQueryResult DBifImpl::GetFilterAndList( DBApi::eListType listtype, CRecordset& resultrecs, std::vector<std::string>& taglist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string schemaName = "";
	std::string tableName = "";
	std::string whereStr = "";
	std::string idTagStr = "";
	std::string idValStr = "";

	if ( !GetListQueryTags( listtype, schemaName, tableName, idTagStr, idValStr ) )
	{
		WriteLogEntry( "GetFilterAndList: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	if ( filtertypes.size() > 0 )
	{
		if ( !GetListFilterString( listtype, filtertypes, whereStr, compareops, comparevals ) )
		{
			WriteLogEntry( "GetFilterAndList: failed: filter creation failed", ErrorMsgType );
			return DBApi::eQueryResult::BadQuery;
		}
	}

	// check for special limit requests.
	// if limit specified as -2, just pass the negative value to the DoListQuery method...
	if ( limitcnt == DFLT_QUERY_AND_LIST_LIMIT || limitcnt == DFLT_QUERY_NO_LIST_LIMIT )		// look for explicitly specified or default parameter default limit request
	{
		limitcnt = DefaultQueryRecordLimit;
	}
	else if ( limitcnt > MaxQueryRecordLimit )
	{
		limitcnt = MaxQueryRecordLimit;
	}

	return DoListQuery( schemaName, tableName, resultrecs, taglist, listtype,
						primarysort, secondarysort, orderstring, whereStr, idTagStr,
						sortdir, limitcnt, startindex, startidnum, custom_query );
}

DBApi::eQueryResult DBifImpl::DoGuidListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
												   std::vector<uuid__t>& srcidlist, CRecordset & resultrecs,
												   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												   std::string orderstring, int32_t sortdir, int32_t limitcnt,
												   int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	std::string guidStr = "";
	std::vector<std::string> uuidList = {};
	uint32_t listIndex = 0;
	size_t listCnt = 0;
	size_t idListSize = srcidlist.size();
	uuid__t uid;

	ClearGuid( uid );

	// ensure all list entries are valid uuids
	for ( listIndex = 0, listCnt = 0; listIndex < idListSize; listIndex++ )
	{
		if ( ( limitcnt < DFLT_QUERY_AND_LIST_LIMIT ) || ( listCnt < limitcnt ) )
		{
			ClearGuid( uid );
			uid = srcidlist.at( listIndex );

			if ( !GuidValid( uid ) )
			{
				return DBApi::eQueryResult::BadOrMissingListIds;
			}

			uuid__t_to_DB_UUID_Str( uid, guidStr );

			uuidList.push_back( guidStr );
			listCnt++;		// increment the number of top-level objects for limit count check; always get entire contents of objects
		}
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string schemaName = "";
	std::string tableName = "";
	std::string queryStr = "";
	std::string guidArrayStr = {};
	std::string guidListStr = {};
	std::string idTagStr = {};

	// just getting the schema and table names... uid should be valid here
	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName,
								idTagStr, guidStr, uid, NO_ID_NUM ) )
	{
		WriteLogEntry( "DoGuidListRetrieval: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	listCnt = uuidList.size();

	if ( listCnt > 0 )
	{		// using the 'In ( set-values)' format works even when there is only a single value
		guidArrayStr = " ( ";
		for ( listIndex = 0; listIndex < listCnt; )
		{
			guidListStr = boost::str( boost::format( "%s" ) % uuidList.at( listIndex ) );
			guidArrayStr.append( guidListStr );
			if ( ++listIndex < listCnt )
			{
				guidArrayStr.append( ", " );
			}
		}
		guidArrayStr.append( " )" );

		if ( limitcnt > MaxQueryRecordLimit )
		{
			limitcnt = MaxQueryRecordLimit;
		}

		std::string whereStr = boost::str( boost::format( "\"%s\" IN %s" ) % idTagStr % guidArrayStr );

		queryResult = DoListQuery( schemaName, tableName, resultrecs, taglist, listtype,
								   primarysort, secondarysort, orderstring, whereStr, idTagStr,
								   sortdir, limitcnt, startindex, startidnum );

	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DoIndexListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
													std::vector<uint32_t>& srcidxlist, CRecordset& resultrecs,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum )
{
	std::string idxStr = "";
	std::vector<std::string> idxStrList = {};
	uint32_t listIndex = 0;
	size_t listCnt = 0;
	size_t idListSize = srcidxlist.size();
	int32_t idxVal = INVALID_INDEX;

	// ensure all list entries are valid uuids
	for ( listIndex = 0, listCnt = 0; listIndex < idListSize; listIndex++ )
	{
		if ( ( limitcnt < DFLT_QUERY_AND_LIST_LIMIT ) || ( listCnt < limitcnt ) )
		{
			idxVal = srcidxlist.at( listIndex );

			if ( idxVal <= INVALID_INDEX )
			{
				return DBApi::eQueryResult::MissingQueryKey;
			}

			idxStr = boost::str( boost::format( "%d" ) % idxVal );

			idxStrList.push_back( idxStr );
			listCnt++;		// increment the number of top-level objects for limit count check; always get entire contents of objects
		}
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string schemaName = "";
	std::string tableName = "";
	std::string queryStr = "";
	std::string idxArrayStr = "";
	std::string idTagStr = "";
	uuid__t tmpUid;

	ClearGuid( tmpUid );

	// just getting the schema and table names... ixVald should be valid here
	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName,
								idTagStr, idxStr, tmpUid, NO_ID_NUM, idxVal ) )
	{
		WriteLogEntry( "DoIndexListRetrieval: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	listCnt = idxStrList.size();

	if ( listCnt > 0 )
	{
		if ( listCnt > 1 )
		{
			idxArrayStr = " ( ";
			for ( listIndex = 0; listIndex < listCnt; )
			{
				idxArrayStr.append( idxStrList.at( listIndex ) );
				if ( ++listIndex < listCnt )
				{
					idxArrayStr.append( ", " );
				}
			}
			idxArrayStr.append( " )" );
		}
		else
		{
			idxArrayStr = idxStrList.at( 0 );
		}

		if ( limitcnt > MaxQueryRecordLimit )
		{
			limitcnt = MaxQueryRecordLimit;
		}

		std::string whereStr = boost::str( boost::format( "\"%s\" IN %s" ) % idTagStr % idxArrayStr );

		queryResult = DoListQuery( schemaName, tableName, resultrecs, taglist, listtype,
								   primarysort, secondarysort, orderstring, whereStr, idTagStr,
								   sortdir, limitcnt, startindex, startidnum );
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DoIdnumListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
													std::vector<int64_t>& idnumlist, CRecordset& resultrecs,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum )
{
	std::string idNumStr = "";
	std::vector<std::string> idNumStrList = {};
	uint32_t listIndex = 0;
	size_t listCnt = 0;
	size_t idListSize = idnumlist.size();
	int64_t idNumVal = NO_ID_NUM;

	// ensure all list entries are valid uuids
	for ( listIndex = 0, listCnt = 0; listIndex < idListSize; listIndex++ )
	{
		if ( ( limitcnt < DFLT_QUERY_AND_LIST_LIMIT ) || ( listCnt < limitcnt ) )
		{
			idNumVal = idnumlist.at( listIndex );

			if ( idNumVal <= INVALID_ID_NUM )
			{
				return DBApi::eQueryResult::MissingQueryKey;
			}

			idNumStr = boost::str( boost::format( "%lld" ) % idNumVal );

			idNumStrList.push_back( idNumStr );
			listCnt++;		// increment the number of top-level objects for limit count check; always get entire contents of objects
		}
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string schemaName = "";
	std::string tableName = "";
	std::string queryStr = "";
	std::string idNumArrayStr = "";
	std::string idTagStr = "";
	uuid__t tmpUid;

	ClearGuid( tmpUid );

	// just getting the schema and table names... idNumVal should be valid here
	if ( !GetSrcTableQueryInfo( listtype, schemaName, tableName,
								idTagStr, idNumStr, tmpUid, idNumVal, INVALID_INDEX ) )
	{
		WriteLogEntry( "DoIdNumListRetrieval: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	listCnt = idNumStrList.size();

	if ( listCnt > 0 )
	{
		if ( listCnt > 1 )
		{
			idNumArrayStr = " ( ";
			for ( listIndex = 0; listIndex < listCnt; )
			{
				idNumArrayStr.append( idNumStrList.at( listIndex ) );
				if ( ++listIndex < listCnt )
				{
					idNumArrayStr.append( ", " );
				}
			}
			idNumArrayStr.append( " )" );
		}
		else
		{
			idNumArrayStr = idNumStrList.at( 0 );
		}

		if ( limitcnt > MaxQueryRecordLimit )
		{
			limitcnt = MaxQueryRecordLimit;
		}

		std::string whereStr = boost::str( boost::format( "\"%s\" IN %s" ) % idTagStr % idNumArrayStr );

		queryResult = DoListQuery( schemaName, tableName, resultrecs, taglist, listtype,
								   primarysort, secondarysort, orderstring, whereStr, idTagStr,
								   sortdir, limitcnt, startindex, startidnum );
	}

	return queryResult;
}


// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetWorklistList( std::vector<DB_WorklistRecord>& wllist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   DBApi::eContainedObjectRetrieval get_sub_items,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::WorklistList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		if ( resultRecs.GetRecordCount() > 0 && resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_WorklistRecord wqRec = {};

				queryOk = ParseWorklist( queryResult, wqRec, resultRecs, tagList, get_sub_items );
				if ( queryOk )
				{
					wllist.push_back( wqRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetWorklistList( std::vector<DB_WorklistRecord>& wllist,
											   DBApi::eListFilterCriteria filtertype,
											   std::string compareop, std::string compareval,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   DBApi::eContainedObjectRetrieval get_sub_items,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetWorklistList( wllist, filtertypes, compareops, comparevals,
							primarysort, secondarysort, get_sub_items,
							orderstring,  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of work list records using a pair list of record ids and record id numbers
// Only one of the pair needs to be specified, but UUID has priority over ID number, if both are specified.
DBApi::eQueryResult DBifImpl::GetWorklistList( std::vector<DB_WorklistRecord>& wlrlist, std::vector<uuid__t>& wlidlist,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = wlidlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::WorklistList, tagList, wlidlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_WorklistRecord wlRec = {};

			queryOk = ParseWorklist( queryResult, wlRec, resultRecs, tagList, FirstLevelObjs );
			if ( queryOk )
			{
				wlrlist.push_back( wlRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops,
												std::vector<std::string> comparevals,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												DBApi::eContainedObjectRetrieval get_sub_items,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SampleSetList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SampleSetRecord ssRec = {};

				queryOk = ParseSampleSet( queryResult, ssRec, resultRecs, tagList, get_sub_items );
				if ( queryOk )
				{
					ssrlist.push_back( ssRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist,
												DBApi::eListFilterCriteria filtertype,
												std::string compareop, std::string compareval,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												DBApi::eContainedObjectRetrieval get_sub_items,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSampleSetList( ssrlist, filtertypes, compareops, comparevals,
							 primarysort, secondarysort, get_sub_items,
							 orderstring, sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of sample-set records using a pair list of record ids and record id numbers
// Only one of the pair needs to be specified, but UUID has priority over ID number, if both are specified.
DBApi::eQueryResult DBifImpl::GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist, std::vector<uuid__t>& ssridlist,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												DBApi::eContainedObjectRetrieval get_sub_items,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = ssridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::SampleSetList, tagList, ssridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_SampleSetRecord ssRec = {};

			queryOk = ParseSampleSet( queryResult, ssRec, resultRecs, tagList, get_sub_items );
			if ( queryOk )
			{
				ssrlist.push_back( ssRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// retrieve the sample sets from a specified worklist
DBApi::eQueryResult DBifImpl::GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist, uuid__t wlid,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !GuidValid( wlid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DB_WorklistRecord dbWl = {};

	queryResult = GetWorklistInternal( dbWl, wlid, NO_ID_NUM, AllSubObjs );
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		for ( auto ssIter = dbWl.SSList.begin(); ssIter != dbWl.SSList.end(); ++ssIter )
		{
			DB_SampleSetRecord ssRec = *ssIter;
			ssrlist.push_back( ssRec );
		}
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
												 std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops, std::vector<std::string> comparevals,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SampleItemList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SampleItemRecord ssiRec = {};

				queryOk = ParseSampleItem( queryResult, ssiRec, resultRecs, tagList );
				if ( queryOk )
				{
					ssirlist.push_back( ssiRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
												 DBApi::eListFilterCriteria filtertype,
												 std::string compareop, std::string compareval,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSampleItemList( ssirlist, filtertypes, compareops, comparevals,
							  primarysort, secondarysort, orderstring,
							  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// retrieve the sample items from a specified sample-set
DBApi::eQueryResult DBifImpl::GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist, uuid__t ssid,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !GuidValid( ssid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DB_SampleSetRecord dbSs = {};

	queryResult = GetSampleSetInternal( dbSs, ssid, NO_ID_NUM, AllSubObjs );
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		for ( auto ssiIter = dbSs.SSItemsList.begin(); ssiIter != dbSs.SSItemsList.end(); ++ssiIter )
		{
			DB_SampleItemRecord ssiRec = *ssiIter;
			ssirlist.push_back( ssiRec );
		}
	}

	return queryResult;
}

// internal retrieval method for pulling a list of sample item records using a pair list of record ids and record id numbers
// Only one of the pair needs to be specified, but UUID has priority over ID number, if both are specified.
DBApi::eQueryResult DBifImpl::GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist, std::vector<uuid__t>& ssiridlist,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = ssiridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::SampleItemList, tagList, ssiridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_SampleItemRecord ssiRec = {};

			queryOk = ParseSampleItem( queryResult, ssiRec, resultRecs, tagList );
			if ( queryOk )
			{
				ssirlist.push_back( ssiRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleList( std::vector<DB_SampleRecord>& samplelist,
											 std::vector<DBApi::eListFilterCriteria> filtertypes,
											 std::vector<std::string> compareops, std::vector<std::string> comparevals,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort,
											 std::string orderstring, int32_t sortdir, int32_t limitcnt,
											 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SampleList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SampleRecord sampleRec = {};

				queryOk = ParseSample( queryResult, sampleRec, resultRecs, tagList );
				if ( queryOk )
				{
					samplelist.push_back( sampleRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSampleList( std::vector<DB_SampleRecord>& samplelist,
											 DBApi::eListFilterCriteria filtertype,
											 std::string compareop, std::string compareval,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort,
											 std::string orderstring, int32_t sortdir, int32_t limitcnt,
											 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSampleList( samplelist, filtertypes, compareops, comparevals,
						  primarysort, secondarysort, orderstring,
						  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of sample records using a list of record ids
DBApi::eQueryResult DBifImpl::GetSampleList( std::vector<DB_SampleRecord>& srlist, std::vector<uuid__t>& sridlist,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort,
											 std::string orderstring, int32_t sortdir, int32_t limitcnt,
											 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = sridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::SampleList, tagList, sridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		GetRecordColumnTags( resultRecs, tagList );

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_SampleRecord sRec = {};

			queryOk = ParseSample( queryResult, sRec, resultRecs, tagList );
			if ( queryOk )
			{
				srlist.push_back( sRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisList( std::vector<DB_AnalysisRecord>& anlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::AnalysisList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		if ( resultRecs.GetRecordCount() > 0 && resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_AnalysisRecord anRec = {};

				queryOk = ParseAnalysis( queryResult, anRec, resultRecs, tagList );
				if ( queryOk )
				{
					anlist.push_back( anRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisList( std::vector<DB_AnalysisRecord>& anlist,
											   DBApi::eListFilterCriteria filtertype,
											   std::string compareop, std::string compareval,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetAnalysisList( anlist, filtertypes, compareops, comparevals,
							primarysort, secondarysort, orderstring,
							sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of analysis records using a list of record ids
DBApi::eQueryResult DBifImpl::GetAnalysisList( std::vector<DB_AnalysisRecord>& arlist, std::vector<uuid__t>& aridlist,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum )

{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = aridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::AnalysisList, tagList, aridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_AnalysisRecord anRec = {};

			queryOk = ParseAnalysis( queryResult, anRec, resultRecs, tagList );
			if ( queryOk )
			{
				arlist.push_back( anRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SummaryResultList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SummaryResultRecord srRec = {};

				queryOk = ParseSummaryResult( queryResult, srRec, resultRecs, tagList );
				if ( queryOk )
				{
					srlist.push_back( srRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist,
													DBApi::eListFilterCriteria filtertype,
													std::string compareop, std::string compareval,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSummaryResultList( srlist, filtertypes, compareops, comparevals,
								 primarysort, secondarysort, orderstring,
								 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of summary result records using a list of record ids
DBApi::eQueryResult DBifImpl::GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist, std::vector<uuid__t>& sridlist,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = sridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::SummaryResultList, tagList, sridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_SummaryResultRecord srRec = {};

			queryOk = ParseSummaryResult( queryResult, srRec, resultRecs, tagList );
			if ( queryOk )
			{
				srlist.push_back( srRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetDetailedResultList( std::vector<DB_DetailedResultRecord>& irlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::DetailedResultList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_DetailedResultRecord irRec = {};

				queryOk = ParseDetailedResult( queryResult, irRec, resultRecs, tagList );
				if ( queryOk )
				{
					irlist.push_back( irRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetDetailedResultList( std::vector<DB_DetailedResultRecord>& irlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetDetailedResultList( irlist, filtertypes, compareops, comparevals,
									   primarysort, secondarysort, orderstring,
									   sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list image result records using a list of record ids
DBApi::eQueryResult DBifImpl::GetDetailedResultList( std::vector<DB_DetailedResultRecord>& drlist, std::vector<uuid__t>& dridlist,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = dridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::DetailedResultList, tagList, dridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_DetailedResultRecord drRec = {};

			queryOk = ParseDetailedResult( queryResult, drRec, resultRecs, tagList );
			if ( queryOk )
			{
				drlist.push_back( drRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageResultList( std::vector<DB_ImageResultRecord>& irmlist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ImageResultList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ImageResultRecord irmRec = {};

				queryOk = ParseImageResult( queryResult, irmRec, resultRecs, tagList );
				if ( queryOk )
				{
					irmlist.push_back( irmRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageResultList( std::vector<DB_ImageResultRecord>& irmlist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetImageResultList( irmlist, filtertypes, compareops, comparevals,
							   primarysort, secondarysort, orderstring,
							   sortdir, limitcnt, startindex, startidnum, custom_query );
}

DBApi::eQueryResult DBifImpl::GetImageResultList( std::vector<DB_ImageResultRecord>& irmlist, std::vector<uuid__t>& irmidlist,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = irmidlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::ImageResultList, tagList, irmidlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_ImageResultRecord irmRec = {};

			queryOk = ParseImageResult( queryResult, irmRec, resultRecs, tagList );
			if ( queryOk )
			{
				irmlist.push_back( irmRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}


// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSResultList( std::vector<DB_SResultRecord>& srlist,
											  std::vector<DBApi::eListFilterCriteria> filtertypes,
											  std::vector<std::string> compareops, std::vector<std::string> comparevals,
											  DBApi::eListSortCriteria primarysort,
											  DBApi::eListSortCriteria secondarysort,
											  std::string orderstring, int32_t sortdir, int32_t limitcnt,
											  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SResultList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SResultRecord srRec = {};

				queryOk = ParseSResult( queryResult, srRec, resultRecs, tagList );
				if ( queryOk )
				{
					srlist.push_back( srRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSResultList( std::vector<DB_SResultRecord>& srlist,
											  DBApi::eListFilterCriteria filtertype,
											  std::string compareop, std::string compareval,
											  DBApi::eListSortCriteria primarysort,
											  DBApi::eListSortCriteria secondarysort,
											  std::string orderstring, int32_t sortdir, int32_t limitcnt,
											  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSResultList( srlist, filtertypes, compareops, comparevals,
								 primarysort, secondarysort, orderstring,
								 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of summary result records using a list of record ids
DBApi::eQueryResult DBifImpl::GetSResultList( std::vector<DB_SResultRecord>& srlist, std::vector<uuid__t>& sridlist,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = sridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::SResultList, tagList, sridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_SResultRecord srRec = {};

			queryOk = ParseSResult( queryResult, srRec, resultRecs, tagList );
			if ( queryOk )
			{
				srlist.push_back( srRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ImageSetList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ImageSetRecord isrRec = {};

				queryOk = ParseImageSet( queryResult, isrRec, resultRecs, tagList );
				if ( queryOk )
				{
					isrlist.push_back( isrRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist,
											   DBApi::eListFilterCriteria filtertype,
											   std::string compareop, std::string compareval,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetImageSetList( isrlist, filtertypes, compareops, comparevals,
							primarysort, secondarysort, orderstring,
							sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of image set records using a list of record ids
DBApi::eQueryResult DBifImpl::GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist, std::vector<uuid__t>& isridlist,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = isridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::ImageSetList, tagList, isridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_ImageSetRecord isrRec = {};

			queryOk = ParseImageSet( queryResult, isrRec, resultRecs, tagList );
			if ( queryOk )
			{
				isrlist.push_back( isrRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ImageSequenceList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ImageSeqRecord seqRec = {};

				queryOk = ParseImageSequence( queryResult, seqRec, resultRecs, tagList );
				if ( queryOk )
				{
					isrlist.push_back( seqRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
													DBApi::eListFilterCriteria filtertype,
													std::string compareop, std::string compareval,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetImageSequenceList( isrlist, filtertypes, compareops, comparevals,
								 primarysort, secondarysort, orderstring,
								 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of image record records using a list of record ids
DBApi::eQueryResult DBifImpl::GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist, std::vector<uuid__t>& isridlist,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum )

{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = isridlist.size();

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::ImageSequenceList, tagList, isridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_ImageSeqRecord seqRec = {};

			queryOk = ParseImageSequence( queryResult, seqRec, resultRecs, tagList );
			if ( queryOk )
			{
				isrlist.push_back( seqRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageList( std::vector<DB_ImageRecord>& imrlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort,
											std::string orderstring, int32_t sortdir, int32_t limitcnt,
											int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ImageList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ImageRecord imrRec = {};

				queryOk = ParseImage( queryResult, imrRec, resultRecs, tagList );
				if ( queryOk )
				{
					imrlist.push_back( imrRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageList( std::vector<DB_ImageRecord>& imrlist,
											DBApi::eListFilterCriteria filtertype,
											std::string compareop, std::string compareval,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort,
											std::string orderstring, int32_t sortdir, int32_t limitcnt,
											int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetImageList( imrlist, filtertypes, compareops, comparevals,
						 primarysort, secondarysort, orderstring,
						 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of image reference records using a list of record ids
DBApi::eQueryResult DBifImpl::GetImageList( std::vector<DB_ImageRecord>& imrlist, std::vector<uuid__t>& imridlist,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort,
											std::string orderstring, int32_t sortdir, int32_t limitcnt,
											int32_t startindex, int64_t startidnum )

{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = imridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::ImageList, tagList, imridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_ImageRecord imrRec = {};

			queryOk = ParseImage( queryResult, imrRec, resultRecs, tagList );
			if ( queryOk )
			{
				imrlist.push_back( imrRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCellTypeList( std::vector<DB_CellTypeRecord>& ctlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::CellTypeList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_CellTypeRecord ctRec = {};

				queryOk = ParseCellType( queryResult, ctRec, resultRecs, tagList );
				if ( queryOk )
				{
					ctlist.push_back( ctRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCellTypeList( std::vector<DB_CellTypeRecord>& ctlist,
											   DBApi::eListFilterCriteria filtertype,
											   std::string compareop, std::string compareval,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort,
											   std::string orderstring, int32_t sortdir, int32_t limitcnt,
											   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetCellTypeList( ctlist, filtertypes, compareops, comparevals,
							primarysort, secondarysort, orderstring,
							sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageAnalysisParamList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
														 std::vector<DBApi::eListFilterCriteria> filtertypes,
														 std::vector<std::string> compareops, std::vector<std::string> comparevals,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 std::string orderstring, int32_t sortdir, int32_t limitcnt,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ImageAnalysisParamList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ImageAnalysisParamRecord apRec = {};

				queryOk = ParseImageAnalysisParam( queryResult, apRec, resultRecs, tagList );
				if ( queryOk )
				{
					aplist.push_back( apRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetImageAnalysisParamList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop, std::string compareval,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 std::string orderstring, int32_t sortdir, int32_t limitcnt,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetImageAnalysisParamList( aplist, filtertypes, compareops, comparevals,
									  primarysort, secondarysort, orderstring,
									  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisInputSettingsList( std::vector<DB_AnalysisInputSettingsRecord>& aislist,
															std::vector<DBApi::eListFilterCriteria> filtertypes,
															std::vector<std::string> compareops, std::vector<std::string> comparevals,
															DBApi::eListSortCriteria primarysort,
															DBApi::eListSortCriteria secondarysort,
															std::string orderstring, int32_t sortdir, int32_t limitcnt,
															int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::AnalysisInputSettingsList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		if ( resultRecs.GetRecordCount() > 0 && resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_AnalysisInputSettingsRecord aisRec = {};

				queryOk = ParseAnalysisInputSettings( queryResult, aisRec, resultRecs, tagList );
				if ( queryOk )
				{
					aislist.push_back( aisRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisInputSettingsList( std::vector<DB_AnalysisInputSettingsRecord>& aislist,
															DBApi::eListFilterCriteria filtertype,
															std::string compareop, std::string compareval,
															DBApi::eListSortCriteria primarysort,
															DBApi::eListSortCriteria secondarysort,
															std::string orderstring, int32_t sortdir, int32_t limitcnt,
															int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetAnalysisInputSettingsList( aislist, filtertypes, compareops, comparevals,
										 primarysort, secondarysort, orderstring,
										 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisDefinitionList( std::vector<DB_AnalysisDefinitionRecord >& adlist,
														 std::vector<DBApi::eListFilterCriteria> filtertypes,
														 std::vector<std::string> compareops, std::vector<std::string> comparevals,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 std::string orderstring, int32_t sortdir, int32_t limitcnt,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::AnalysisDefinitionList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_AnalysisDefinitionRecord adRec = {};

				queryOk = ParseAnalysisDefinition( queryResult, adRec, resultRecs, tagList );
				if ( queryOk )
				{
					adlist.push_back( adRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisDefinitionList( std::vector<DB_AnalysisDefinitionRecord >& adlist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop, std::string compareval,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 std::string orderstring, int32_t sortdir, int32_t limitcnt,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetAnalysisDefinitionList( adlist, filtertypes, compareops, comparevals,
									  primarysort, secondarysort, orderstring,
									  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisParamList( std::vector<DB_AnalysisParamRecord>& aplist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::AnalysisParamList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_AnalysisParamRecord apRec = {};

				queryOk = ParseAnalysisParam( queryResult, apRec, resultRecs, tagList );
				if ( queryOk )
				{
					aplist.push_back( apRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetAnalysisParamList( std::vector<DB_AnalysisParamRecord>& aplist,
													DBApi::eListFilterCriteria filtertype,
													std::string compareop, std::string compareval,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													std::string orderstring, int32_t sortdir, int32_t limitcnt,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetAnalysisParamList( aplist, filtertypes, compareops, comparevals,
								 primarysort, secondarysort, orderstring,
								 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetIlluminatorList( std::vector<DB_IlluminatorRecord >& illist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::IlluminatorList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_IlluminatorRecord ilRec = {};

				queryOk = ParseIlluminator( queryResult, ilRec, resultRecs, tagList );
				if ( queryOk )
				{
					illist.push_back( ilRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetIlluminatorList( std::vector<DB_IlluminatorRecord >& illist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetIlluminatorList( illist, filtertypes, compareops, comparevals,
							   primarysort, secondarysort, orderstring,
							   sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetUserList( std::vector<DB_UserRecord>& urlist,
										   std::vector<DBApi::eListFilterCriteria> filtertypes,
										   std::vector<std::string> compareops, std::vector<std::string> comparevals,
										   eListSortCriteria primarysort,
										   eListSortCriteria secondarysort,
										   std::string orderstring, int32_t sortdir, int32_t limitcnt,
										   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::UserList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_UserRecord uRec = {};

				queryOk = ParseUser( queryResult, uRec, resultRecs, tagList );
				if ( queryOk )
				{
					urlist.push_back( uRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetUserList( std::vector<DB_UserRecord>& urlist,
										   eListFilterCriteria filtertype,
										   std::string compareop, std::string compareval,
										   eListSortCriteria primarysort,
										   eListSortCriteria secondarysort,
										   std::string orderstring, int32_t sortdir, int32_t limitcnt,
										   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetUserList( urlist, filtertypes, compareops, comparevals,
						primarysort, secondarysort, orderstring,
						sortdir, limitcnt, startindex, startidnum, custom_query );
}

// internal retrieval method for pulling a list of user records using a list of record ids
DBApi::eQueryResult DBifImpl::GetUserList( std::vector<DB_UserRecord>& urlist, std::vector<uuid__t>& uridlist,
										   DBApi::eListSortCriteria primarysort,
										   DBApi::eListSortCriteria secondarysort,
										   std::string orderstring, int32_t sortdir, int32_t limitcnt,
										   int32_t startindex, int64_t startidnum )

{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idListSize = uridlist.size();
	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = DoGuidListRetrieval( DBApi::eListType::UserList, tagList, uridlist,
									   resultRecs, primarysort, secondarysort,
									   orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_UserRecord urRec = {};

			queryOk = ParseUser( queryResult, urRec, resultRecs, tagList );
			if ( queryOk )
			{
				urlist.push_back( urRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetRoleList( std::vector<DB_UserRoleRecord>& rrlist,
										   std::vector<DBApi::eListFilterCriteria> filtertypes,
										   std::vector<std::string> compareops, std::vector<std::string> comparevals,
										   eListSortCriteria primarysort,
										   eListSortCriteria secondarysort,
										   std::string orderstring, int32_t sortdir, int32_t limitcnt,
										   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::RolesList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_UserRoleRecord rrRec = {};

				queryOk = ParseRole( queryResult, rrRec, resultRecs, tagList );
				if ( queryOk )
				{
					rrlist.push_back( rrRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetRoleList( std::vector<DB_UserRoleRecord>& rrlist,
										   eListFilterCriteria filtertype,
										   std::string compareop, std::string compareval,
										   eListSortCriteria primarysort,
										   eListSortCriteria secondarysort,
										   std::string orderstring, int32_t sortdir, int32_t limitcnt,
										   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetRoleList( rrlist, filtertypes, compareops, comparevals,
						primarysort, secondarysort, orderstring,
						sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::UserPropertiesList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_UserPropertiesRecord upRec = {};

				queryOk = ParseUserProperty( queryResult, upRec, resultRecs, tagList );
				if ( queryOk )
				{
					uplist.push_back( upRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetUserPropertiesList( uplist, filtertypes, compareops, comparevals,
								  primarysort, secondarysort, orderstring,
								  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// get the properties for a specific user; the user object just returns list of property indices, names, and types;
// this method retrieves a list of the full property objects...
DBApi::eQueryResult DBifImpl::GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t usrid,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !GuidValid( usrid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DB_UserRecord dbUser = {};

	queryResult = GetUserInternal( dbUser, usrid, "" );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		return queryResult;
	}

	size_t idxListSize = dbUser.UserPropertiesList.size();

	if ( idxListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingListIds;
	}

	uint32_t listIndex = 0;
	DB_UserPropertiesRecord upRec = {};

	for ( listIndex = 0; listIndex < idxListSize; listIndex++ )
	{
		upRec = dbUser.UserPropertiesList.at( listIndex );
		uplist.push_back( upRec );
	}

	return queryResult;
}

// get the properties for a specific user; the user object just returns list of property indices, names, and types;
// this method retrieves a list of the full property objects...
DBApi::eQueryResult DBifImpl::GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, std::string username,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( username.empty() )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DB_UserRecord dbUser = {};
	uuid__t tmpId;

	ClearGuid( tmpId );

	queryResult = GetUserInternal( dbUser, tmpId, username );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		return queryResult;
	}

	size_t idxListSize = dbUser.UserPropertiesList.size();

	if ( idxListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingListIds;
	}

	uint32_t listIndex = 0;
	DB_UserPropertiesRecord upRec = {};

	for ( listIndex = 0; listIndex < idxListSize; listIndex++ )
	{
		upRec = dbUser.UserPropertiesList.at( listIndex );
		uplist.push_back( upRec );
	}

	return queryResult;
}

// internal retrieval method for pulling a list of user properties records using a list of record indices
DBApi::eQueryResult DBifImpl::GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, std::vector<int16_t>& propindexlist,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 std::string orderstring, int32_t sortdir, int32_t limitcnt,
													 int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idxListSize = propindexlist.size();
	if ( idxListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method
	uint32_t listIndex = 0;
	int32_t idxVal = -1;
	std::vector<uint32_t> idxList = {};

	// check the passed-in list for valid indices
	// upconvert from int16_t to a vector of int32_t values for the list retrieval method
	do
	{
		idxVal = propindexlist.at( listIndex );
//		if ( idxVal < 0 || idxVal > MaxUserPropertyIndex )
		if ( idxVal < 0 )
		{
			return DBApi::eQueryResult::BadOrMissingListIds;
		}
		idxList.push_back( (uint32_t)idxVal );
	} while ( ++listIndex < idxListSize );

	queryResult = DoIndexListRetrieval( DBApi::eListType::UserPropertiesList, tagList,
										idxList, resultRecs, primarysort, secondarysort,
										orderstring, sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_UserPropertiesRecord upRec = {};

			queryOk = ParseUserProperty( queryResult, upRec, resultRecs, tagList );
			if ( queryOk )
			{
				uplist.push_back( upRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSignatureList( std::vector<DB_SignatureRecord>& siglist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SignatureDefList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SignatureRecord sigRec = {};

				queryOk = ParseSignature( queryResult, sigRec, resultRecs, tagList );
				if ( queryOk )
				{
					siglist.push_back( sigRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSignatureList( std::vector<DB_SignatureRecord>& siglist,
												DBApi::eListFilterCriteria filtertype,
												std::string compareop, std::string compareval,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSignatureList( siglist, filtertypes, compareops, comparevals,
							 primarysort, secondarysort, orderstring,
							 sortdir, limitcnt, startindex, startidnum, custom_query );
}



// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::ReagentInfoList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_ReagentTypeRecord rxRec = {};

				queryOk = ParseReagentType( queryResult, rxRec, resultRecs, tagList );
				if ( queryOk )
				{
					rxlist.push_back( rxRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetReagentTypeList( rxlist, filtertypes, compareops, comparevals,
							   primarysort, secondarysort, orderstring,
							   sortdir, limitcnt, startindex, startidnum, custom_query );
}

DBApi::eQueryResult DBifImpl::GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist,
												  std::vector<int64_t>& idnumlist,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idnumListSize = idnumlist.size();
	if ( idnumListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method
	uint32_t listIndex = 0;
	int64_t idNumVal = NO_ID_NUM;
	std::vector<int64_t> idNumList = {};

	// check the passed-in list for valid indices
	// upconvert from int16_t to a vector of int32_t values for the list retrieval method
	do
	{
		idNumVal = idnumlist.at( listIndex );
		if ( idNumVal <= INVALID_ID_NUM )
		{
			return DBApi::eQueryResult::BadOrMissingListIds;
		}
		idNumList.push_back( idNumVal );
	} while ( ++listIndex < idnumListSize );

	queryResult = DoIdnumListRetrieval( DBApi::eListType::ReagentInfoList,
										tagList, idNumList, resultRecs,
										primarysort, secondarysort, "",
										sortdir, limitcnt );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_ReagentTypeRecord rxRec = {};

			queryOk = ParseReagentType( queryResult, rxRec, resultRecs, tagList );
			if ( queryOk )
			{
				rxlist.push_back( rxRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCellHealthReagentsList( std::vector<DB_CellHealthReagentRecord>& chrlist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetCellHealthReagentsList(chrlist, filtertypes, compareops, comparevals,
							   primarysort, secondarysort, orderstring,
							   sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCellHealthReagentsList(std::vector<DB_CellHealthReagentRecord>& chrlist,
	std::vector<DBApi::eListFilterCriteria> filtertypes,
	std::vector<std::string> compareops, std::vector<std::string> comparevals,
	DBApi::eListSortCriteria primarysort,
	DBApi::eListSortCriteria secondarysort,
	std::string orderstring, int32_t sortdir, int32_t limitcnt,
	int32_t startindex, int64_t startidnum, std::string custom_query)
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList(DBApi::eListType::CellHealthReagentsList, resultRecs, tagList,
		filtertypes, compareops, comparevals,
		primarysort, secondarysort, orderstring,
		sortdir, limitcnt, startindex, startidnum, custom_query);

	if (queryResult == DBApi::eQueryResult::QueryOk)
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if (rec_cnt <= 0)
		{
			if (resultRecs.IsOpen())
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if (resultRecs.CanScroll())
		{
			resultRecs.MoveFirst();
		}

		while ((!resultRecs.IsEOF()) && (queryOk))
		{
			if ((startindex <= 0) || (listIndex >= startindex))
			{
				DB_CellHealthReagentRecord chRec = {};

				queryOk = ParseCellHealthReagent(queryResult, chRec, resultRecs, tagList);
				if (queryOk)
				{
					chrlist.push_back(chRec);
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if (resultRecs.CanScroll())
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if (resultRecs.IsOpen())
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetBioProcessList( std::vector<DB_BioProcessRecord >& bplist,
												 std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops, std::vector<std::string> comparevals,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::BioProcessList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_BioProcessRecord bpRec = {};

				queryOk = ParseBioProcess( queryResult, bpRec, resultRecs, tagList );
				if ( queryOk )
				{
					bplist.push_back( bpRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetBioProcessList( std::vector<DB_BioProcessRecord >& bplist,
												 DBApi::eListFilterCriteria filtertype,
												 std::string compareop, std::string compareval,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort,
												 std::string orderstring, int32_t sortdir, int32_t limitcnt,
												 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetBioProcessList( bplist, filtertypes, compareops, comparevals,
							  primarysort, secondarysort, orderstring,
							  sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetQcProcessList( std::vector<DB_QcProcessRecord >& qclist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::QcProcessList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_QcProcessRecord qcRec = {};

				queryOk = ParseQcProcess( queryResult, qcRec, resultRecs, tagList );
				if ( queryOk )
				{
					qclist.push_back( qcRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetQcProcessList( std::vector<DB_QcProcessRecord >& qclist,
												DBApi::eListFilterCriteria filtertype,
												std::string compareop, std::string compareval,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort,
												std::string orderstring, int32_t sortdir, int32_t limitcnt,
												int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetQcProcessList( qclist, filtertypes, compareops, comparevals,
							 primarysort, secondarysort, orderstring,
							 sortdir, limitcnt, startindex, startidnum, custom_query );
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCalibrationList( std::vector<DB_CalibrationRecord >& callist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::CalibrationsList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum, custom_query );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_CalibrationRecord calRec = {};

				queryOk = ParseCalibration( queryResult, calRec, resultRecs, tagList );
				if ( queryOk )
				{
					callist.push_back( calRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetCalibrationList( std::vector<DB_CalibrationRecord >& callist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  DBApi::eListSortCriteria primarysort,
												  DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetCalibrationList( callist, filtertypes, compareops, comparevals,
							   primarysort, secondarysort, orderstring,
							   sortdir, limitcnt, startindex, startidnum, custom_query );
}

// list retrieval has limited capabilities, since the list should not be long...
DBApi::eQueryResult DBifImpl::GetInstConfigList( std::vector<DB_InstrumentConfigRecord>& cfglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	// no filter for config list...
	filtertypes.clear();
	compareops.clear();
	comparevals.clear();

	queryResult = GetFilterAndList( DBApi::eListType::InstrumentConfigList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									DBApi::eListSortCriteria::InstrumentSort );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_InstrumentConfigRecord cfgRec = {};

			queryOk = ParseInstConfig( queryResult, cfgRec, resultRecs, tagList );
			if ( queryOk )
			{
				cfglist.push_back( cfgRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetLogList( std::vector<DB_LogEntryRecord>& log_list,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort,
										  int32_t sortdir, int32_t limitcnt )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::LogEntryList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, "",
									sortdir, limitcnt );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_LogEntryRecord logRec = {};

			queryOk = ParseLogEntry( queryResult, logRec, resultRecs, tagList );
			if ( queryOk )
			{
				log_list.push_back( logRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetLogList( std::vector<DB_LogEntryRecord>& log_list,
										  std::vector<int64_t> idnumlist,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort,
										  int32_t sortdir, int32_t limitcnt )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	size_t idxListSize = idnumlist.size();
	if ( idxListSize == 0 )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	CRecordset resultRecs;		// no default initializer; initialized by the method
	uint32_t listIndex = 0;
	int64_t idNumVal = NO_ID_NUM;
	std::vector<int64_t> idNumList = {};

	// check the passed-in list for valid indices
	// upconvert from int16_t to a vector of int32_t values for the list retrieval method
	do
	{
		idNumVal = idnumlist.at( listIndex );
		if ( idNumVal <= INVALID_ID_NUM )
		{
			return DBApi::eQueryResult::BadOrMissingListIds;
		}
		idNumList.push_back( idNumVal );
	} while ( ++listIndex < idxListSize );

	queryResult = DoIdnumListRetrieval( DBApi::eListType::LogEntryList,
										tagList, idNumList, resultRecs,
										primarysort, secondarysort, "",
										sortdir, limitcnt );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			DB_LogEntryRecord logRec = {};

			queryOk = ParseLogEntry( queryResult, logRec, resultRecs, tagList );
			if ( queryOk )
			{
				log_list.push_back( logRec );
			}
			else
			{
				queryResult = DBApi::eQueryResult::ParseFailure;
			}

			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}




// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=" (and variants e.g. ">=", "<="), and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord >& sclist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  DBApi::eListSortCriteria primarysort,
													  DBApi::eListSortCriteria secondarysort,
													  std::string orderstring, int32_t sortdir, int32_t limitcnt,
													  int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};

	queryResult = GetFilterAndList( DBApi::eListType::SchedulerConfigList, resultRecs, tagList,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort, orderstring,
									sortdir, limitcnt, startindex, startidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		bool queryOk = true;
		int32_t listIndex = 0;

		int32_t rec_cnt = resultRecs.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			if ( resultRecs.IsOpen() )
			{
				resultRecs.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		queryResult = DBApi::eQueryResult::QueryOk;

		if ( resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( ( !resultRecs.IsEOF() ) && ( queryOk ) )
		{
			if ( ( startindex <= 0 ) || ( listIndex >= startindex ) )
			{
				DB_SchedulerConfigRecord scRec = {};

				queryOk = ParseSchedulerConfig( queryResult, scRec, resultRecs, tagList );
				if ( queryOk )
				{
					sclist.push_back( scRec );
				}
				else
				{
					queryResult = DBApi::eQueryResult::ParseFailure;
				}
			}
			listIndex++;
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// filter and sort types are limited by the list type;
// filter comparison action/direction is a string that may be ">", "<", "=", and may also include
// "LIKE" (future) or NEAR (possible date comparisons?) or other potentially usefuld SQL operators 
DBApi::eQueryResult DBifImpl::GetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord >& sclist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  DBApi::eListSortCriteria primarysort,
													  DBApi::eListSortCriteria secondarysort,
													  std::string orderstring, int32_t sortdir, int32_t limitcnt,
													  int32_t startindex, int64_t startidnum )
{
	std::vector<DBApi::eListFilterCriteria> filtertypes = {};
	std::vector<std::string> compareops = {};
	std::vector<std::string> comparevals = {};

	filtertypes.push_back( filtertype );
	compareops.push_back( compareop );
	comparevals.push_back( compareval );

	return GetSchedulerConfigList( sclist, filtertypes, compareops, comparevals,
								   primarysort, secondarysort, orderstring,
								   sortdir, limitcnt, startindex, startidnum );
}
