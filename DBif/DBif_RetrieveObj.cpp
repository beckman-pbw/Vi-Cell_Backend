// Database interface : implementation file
//

#pragma once

#include "pch.h"


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif // USE_LOGGER



static const std::string MODULENAME = "DBif_RetrieveObj";



////////////////////////////////////////////////////////////////////////////////
// Internal list and object retrieval and parsing methods
////////////////////////////////////////////////////////////////////////////////

bool DBifImpl::GetWorklistQueryTag( std::string& schemaname, std::string& tablename,
									std::string& selecttag, std::string& idstr,
									uuid__t wlid, int64_t wlidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "WorklistID";
	//	idnumtag = "WorklistIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::WorklistList, schemaname, tablename,
								selecttag, idstr, wlid, wlidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetWorklist( DB_WorklistRecord& wlr, uuid__t wlid, int64_t wlidnum, int32_t get_sets )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetWorklistInternal( wlr, wlid, wlidnum, get_sets );
}

DBApi::eQueryResult DBifImpl::GetWorklistInternal( DB_WorklistRecord& wlr, uuid__t wlid, int64_t wlidnum, int32_t get_sets )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetWorklistObj( wlr, resultRecs, tagList, wlid, wlidnum, get_sets );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetWorklistObj( DB_WorklistRecord& wlr, CRecordset& recset, std::vector<std::string>& taglist,
											  uuid__t wlid, int64_t wlidnum, int32_t get_sets )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr= "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( wlid ) )
	{
		wlidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetWorklistQueryTag( schemaName, tableName, selectTag, idStr, wlid, wlidnum ) )
	{
		WriteLogEntry( "GetWorklistObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, wlid, wlidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_WorklistRecord wlRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseWorklist( queryResult, wlRec, recset, taglist, get_sets );
				}
			}

			if ( parseOk )
			{
				wlr = wlRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSampleSetQueryTag( std::string& schemaname, std::string& tablename,
									 std::string& selecttag, std::string& idstr,
									 uuid__t ssid, int64_t ssidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SampleSetID";
	//	idnumtag = "SampleSetIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SampleSetList, schemaname, tablename,
								selecttag, idstr, ssid, ssidnum ) )
	{
		return false;
	}

	return true;
}

// get object with login; don't require taglist or result record objects
DBApi::eQueryResult DBifImpl::GetSampleSet( DB_SampleSetRecord& ssr, uuid__t ssid, int64_t ssidnum, int32_t get_items )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSampleSetInternal( ssr, ssid, ssidnum, get_items );
}

// get object without login; don't require taglist or result record objects
DBApi::eQueryResult DBifImpl::GetSampleSetInternal( DB_SampleSetRecord& ssr, uuid__t ssid, int64_t ssidnum, int32_t get_items )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSampleSetObj( ssr, resultRecs, tagList, ssid, ssidnum, get_items );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSampleSetObj( DB_SampleSetRecord& ssr, CRecordset& recset, std::vector<std::string>& taglist,
											   uuid__t ssid, int64_t ssidnum, int32_t get_items )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( ssid ) )
	{
		ssidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSampleSetQueryTag( schemaName, tableName, selectTag, idStr, ssid, ssidnum ) )
	{
		WriteLogEntry( "GetSampleSetObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, ssid, ssidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SampleSetRecord ssRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSampleSet( queryResult, ssRec, recset, taglist, get_items );
				}
			}

			if ( parseOk )
			{
				ssr = ssRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSampleItemQueryTag( std::string& schemaname, std::string& tablename,
									  std::string& selecttag, std::string& idstr,
									  uuid__t sitemid, int64_t sitemidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SampleItemID";
	//	idnumtag = "SampleItemIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SampleItemList, schemaname, tablename,
								selecttag, idstr, sitemid, sitemidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSampleItem( DB_SampleItemRecord& ssir, uuid__t ssitemid, int64_t ssitemidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSampleItemInternal( ssir, ssitemid, ssitemidnum );
}

DBApi::eQueryResult DBifImpl::GetSampleItemInternal( DB_SampleItemRecord& ssir, uuid__t ssitemid, int64_t ssitemidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSampleItemObj( ssir, resultRecs, tagList, ssitemid, ssitemidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSampleItemObj( DB_SampleItemRecord& ssir, CRecordset& recset,
												std::vector<std::string>& taglist, uuid__t ssitemid, int64_t ssitemidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( ssitemid ) )
	{
		ssitemidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSampleItemQueryTag( schemaName, tableName, selectTag, idStr, ssitemid, ssitemidnum ) )
	{
		WriteLogEntry( "GetSampleItemObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, ssitemid, ssitemidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SampleItemRecord ssiRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSampleItem( queryResult, ssiRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				ssir = ssiRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSampleQueryTag( std::string& schemaname, std::string& tablename,
								  std::string& selecttag, std::string& idstr,
								  uuid__t sampleid, int64_t sampleidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SampleID";
	//	idnumtag = "SampleIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SampleList, schemaname, tablename,
								selecttag, idstr, sampleid, sampleidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSample( DB_SampleRecord& sr, uuid__t sampleid, int64_t sampleidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSampleInternal( sr, sampleid, sampleidnum );
}

DBApi::eQueryResult DBifImpl::GetSampleInternal( DB_SampleRecord& sr, uuid__t sampleid, int64_t sampleidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSampleObj( sr, resultRecs, tagList, sampleid, sampleidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSampleObj( DB_SampleRecord& sr, CRecordset& recset,
											std::vector<std::string>& taglist, uuid__t sampleid, int64_t sampleidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( sampleid ) )
	{
		sampleidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSampleQueryTag( schemaName, tableName, selectTag, idStr, sampleid, sampleidnum ) )
	{
		WriteLogEntry( "GetSampleObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, sampleid, sampleidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SampleRecord sampleRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSample( queryResult, sampleRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				sr = sampleRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetAnalysisQueryTag( std::string& schemaname, std::string& tablename,
									std::string& selecttag, std::string& idstr,
									uuid__t analysisid, int64_t analysisidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "AnalysisID";
	//	idnumtag = "AnalysisIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::AnalysisList, schemaname, tablename,
								selecttag, idstr, analysisid, analysisidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetAnalysis( DB_AnalysisRecord& analysis, uuid__t analysisid, int64_t analysisidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetAnalysisInternal( analysis, analysisid, analysisidnum );
}

DBApi::eQueryResult DBifImpl::GetAnalysisInternal( DB_AnalysisRecord& analysis, uuid__t analysisid, int64_t analysisidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetAnalysisObj( analysis, resultRecs, tagList, analysisid, analysisidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetAnalysisObj( DB_AnalysisRecord& analysis, CRecordset& recset,
											  std::vector<std::string>& taglist, uuid__t analysisid, int64_t analysisidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( analysisid ) )
	{
		analysisidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetAnalysisQueryTag( schemaName, tableName, selectTag, idStr, analysisid, analysisidnum ) )
	{
		WriteLogEntry( "GetAnalysisObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, analysisid, analysisidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_AnalysisRecord anRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseAnalysis( queryResult, anRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				analysis = anRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSummaryResultQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t resultid, int64_t resultidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SummaryResultID";
	//	idnumtag = "SummaryResultIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SummaryResultList, schemaname, tablename,
								selecttag, idstr, resultid, resultidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSummaryResultInternal( sr, resultid, resultidnum );
}

DBApi::eQueryResult DBifImpl::GetSummaryResultInternal( DB_SummaryResultRecord& sr, uuid__t resultid, int64_t resultidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSummaryResultObj( sr, resultRecs, tagList, resultid, resultidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSummaryResultObj( DB_SummaryResultRecord& srr, CRecordset& recset,
												   std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( resultid ) )
	{
		resultidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSummaryResultQueryTag( schemaName, tableName, selectTag, idStr, resultid, resultidnum ) )
	{
		WriteLogEntry( "GetSummaryResultObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, resultid, resultidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SummaryResultRecord srRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSummaryResult( queryResult, srRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				srr = srRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetDetailedResultQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr,
											   uuid__t resultid, int64_t resultidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "DetailedResultID";
	//	idnumtag = "DetailedResultIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::DetailedResultList, schemaname, tablename,
								selecttag, idstr, resultid, resultidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetDetailedResult( DB_DetailedResultRecord& ir, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetDetailedResultInternal( ir, resultid, resultidnum );
}

DBApi::eQueryResult DBifImpl::GetDetailedResultInternal( DB_DetailedResultRecord& ir, uuid__t resultid, int64_t resultidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetDetailedResultObj( ir, resultRecs, tagList, resultid, resultidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetDetailedResultObj( DB_DetailedResultRecord& drr, CRecordset& recset,
														 std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( resultid ) )
	{
		resultidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetDetailedResultQueryTag( schemaName, tableName, selectTag, idStr, resultid, resultidnum ) )
	{
		WriteLogEntry( "GetDetainedResultObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, resultid, resultidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_DetailedResultRecord drRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseDetailedResult( queryResult, drRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				drr = drRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetImageResultQueryTag( std::string& schemaname, std::string& tablename,
										  std::string& selecttag, std::string& idstr,
										  uuid__t mapid, int64_t mapidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "ResultsMapID";
	//	idnumtag = "ResultsMapIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ImageResultList, schemaname, tablename,
								selecttag, idstr, mapid, mapidnum ) )
	{
		return false;
	}


	return true;
}

DBApi::eQueryResult DBifImpl::GetImageResult( DB_ImageResultRecord& rmr,
											  uuid__t mapid, int64_t mapidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetImageResultInternal( rmr, mapid, mapidnum );
}

DBApi::eQueryResult DBifImpl::GetImageResultInternal( DB_ImageResultRecord& rmr,
													  uuid__t mapid, int64_t mapidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetImageResultObj( rmr, resultRecs, tagList, mapid, mapidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageResultObj( DB_ImageResultRecord& rmr, CRecordset& recset,
												 std::vector<std::string>& taglist, uuid__t mapid, int64_t mapidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( mapid ) )
	{
		mapidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetImageResultQueryTag( schemaName, tableName, selectTag, idStr, mapid, mapidnum ) )
	{
		WriteLogEntry( "GetImageResultObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, mapid, mapidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ImageResultRecord rmRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseImageResult( queryResult, rmRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				rmr = rmRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSResultQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t resultid, int64_t resultidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SummaryResultID";
	//	idnumtag = "SummaryResultIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SResultList, schemaname, tablename,
								selecttag, idstr, resultid, resultidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSResult( DB_SResultRecord& sr, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSResultInternal( sr, resultid, resultidnum );
}

DBApi::eQueryResult DBifImpl::GetSResultInternal( DB_SResultRecord& sr, uuid__t resultid, int64_t resultidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSResultObj( sr, resultRecs, tagList, resultid, resultidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSResultObj( DB_SResultRecord& srr, CRecordset& recset,
												   std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( resultid ) )
	{
		resultidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSResultQueryTag( schemaName, tableName, selectTag, idStr, resultid, resultidnum ) )
	{
		WriteLogEntry( "GetSResultObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, resultid, resultidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SResultRecord srRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSResult( queryResult, srRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				srr = srRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetImageSetQueryTag( std::string& schemaname, std::string& tablename,
									std::string& selecttag, std::string& idstr,
									uuid__t imagesetid, int64_t imagesetidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "ImageSetID";
	//	idnumtag = "ImageSetIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ImageSetList, schemaname, tablename,
								selecttag, idstr, imagesetid, imagesetidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetImageSet( DB_ImageSetRecord& isr,
										   uuid__t imagesetid, int64_t imagesetidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetImageSetInternal( isr, imagesetid, imagesetidnum );
}

DBApi::eQueryResult DBifImpl::GetImageSetInternal( DB_ImageSetRecord& isr,
										   uuid__t imagesetid, int64_t imagesetidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetImageSetObj( isr, resultRecs, tagList, imagesetid, imagesetidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageSetObj( DB_ImageSetRecord& isr, CRecordset& recset,
											  std::vector<std::string>& taglist, uuid__t isrid, int64_t isridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( isrid ) )
	{
		isridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetImageSetQueryTag( schemaName, tableName, selectTag, idStr, isrid, isridnum ) )
	{
		WriteLogEntry( "GetImageSetObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, isrid, isridnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ImageSetRecord isRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseImageSet( queryResult, isRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				isr = isRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetImageSequenceQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t imagerecid, int64_t imagerecidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "ImageSequenceID";
	//	idnumtag = "ImageSequenceIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ImageSequenceList, schemaname, tablename,
								selecttag, idstr, imagerecid, imagerecidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetImageSequence( DB_ImageSeqRecord& isr,
												uuid__t imageseqid, int64_t imageseqidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetImageSequenceInternal( isr, imageseqid, imageseqidnum );
}

DBApi::eQueryResult DBifImpl::GetImageSequenceInternal( DB_ImageSeqRecord& isr,
														uuid__t imageseqid, int64_t imageseqidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetImageSequenceObj( isr, resultRecs, tagList, imageseqid, imageseqidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageSequenceObj( DB_ImageSeqRecord& isr, CRecordset& recset,
												   std::vector<std::string>& taglist, uuid__t isrid, int64_t isridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( isrid ) )
	{
		isridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetImageSequenceQueryTag( schemaName, tableName, selectTag, idStr, isrid, isridnum ) )
	{
		WriteLogEntry( "GetImageSequenceObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, isrid, isridnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ImageSeqRecord seqRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseImageSequence( queryResult, seqRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				isr = seqRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetImageQueryTag( std::string& schemaname, std::string& tablename,
								 std::string& selecttag, std::string& idstr,
								 uuid__t imageid, int64_t imageidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idTag = "ImageID";
	//	idnumTag = "ImageIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ImageList, schemaname, tablename,
								selecttag, idstr, imageid, imageidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetImage( DB_ImageRecord& img,
										uuid__t imageid, int64_t imageidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetImageInternal( img, imageid, imageidnum );
}

DBApi::eQueryResult DBifImpl::GetImageInternal( DB_ImageRecord& img,
										uuid__t imageid, int64_t imageidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetImageObj( img, resultRecs, tagList, imageid, imageidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageObj( DB_ImageRecord& imr, CRecordset& recset,
										   std::vector<std::string>& taglist, uuid__t imageid, int64_t imageidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( imageid ) )
	{
		imageidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetImageQueryTag( schemaName, tableName, selectTag, idStr, imageid, imageidnum ) )
	{
		WriteLogEntry( "GetImageObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, imageid, imageidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ImageRecord imgRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseImage( queryResult, imgRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				imr = imgRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetCellTypeQueryTag( std::string& schemaname, std::string& tablename,
									 std::string& selecttag, std::string& idstr,
									 uuid__t celltypeid, int64_t celltypeindex, int64_t celltypeidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idTtag = "CellTypeID";
	//	idnumTag = "CellTypeIdNum";
	//	indexTag = "CellTypeIndex";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::CellTypeList, schemaname, tablename,
								selecttag, idstr, celltypeid, celltypeidnum, celltypeindex ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetCellType( DB_CellTypeRecord& ctr, uuid__t celltypeid, int64_t celltypeindex, int64_t celltypeidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetCellTypeInternal( ctr, celltypeid, celltypeindex, celltypeidnum );
}

DBApi::eQueryResult DBifImpl::GetCellTypeInternal( DB_CellTypeRecord& ctr, uuid__t celltypeid, int64_t celltypeindex, int64_t celltypeidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetCellTypeObj( ctr, resultRecs, tagList, celltypeid, celltypeindex, celltypeidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetCellTypeObj( DB_CellTypeRecord& ctr, CRecordset& recset, std::vector<std::string>& taglist,
											  uuid__t cellguid, int64_t celltypeindex, int64_t celltypeidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( cellguid )  || celltypeindex >= 0 )
	{
		celltypeidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetCellTypeQueryTag( schemaName, tableName, selectTag, idStr, cellguid, celltypeindex, celltypeidnum ) )
	{
		WriteLogEntry( "GetCellTypeObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	DB_CellTypeRecord ctRec = {};

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, cellguid, celltypeidnum, celltypeindex );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		int recCnt = recset.GetRecordCount();

		if ( recCnt > 0 )
		{
			bool parseOk = false;

			if ( recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseCellType( queryResult, ctRec, recset, taglist );
				}
			}

			if (!parseOk)
			{
				ctRec = {};
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	ctr = ctRec;		// this may now be returning an empty record on error

	return queryResult;
}

bool DBifImpl::GetImageAnalysisParamQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr,
											   uuid__t aprid, int64_t apridnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idTag = "ImageAnalysisParamID";
	//	idnumTag = "ImageAnalysisParamIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ImageAnalysisParamList, schemaname, tablename,
								selecttag, idstr, aprid, apridnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetImageAnalysisParam( DB_ImageAnalysisParamRecord& apr, uuid__t aprid, int64_t apridnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetImageAnalysisParamInternal( apr, aprid, apridnum );
}

DBApi::eQueryResult DBifImpl::GetImageAnalysisParamInternal( DB_ImageAnalysisParamRecord& apr, uuid__t aprid, int64_t apridnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetImageAnalysisParamObj( apr, resultRecs, tagList, aprid, apridnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetImageAnalysisParamObj( DB_ImageAnalysisParamRecord& apr, CRecordset& recset,
														 std::vector<std::string>& taglist, uuid__t aprid, int64_t apridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( aprid ) )
	{
		apridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetImageAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, aprid, apridnum ) )
	{
		WriteLogEntry( "GetImageAnalysisParamObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, aprid, apridnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ImageAnalysisParamRecord apRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseImageAnalysisParam( queryResult, apRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				apr = apRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetAnalysisInputSettingsQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr,
											   uuid__t aprid, int64_t apridnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idTag = "ImageAnalysisInputParamID";
	//	idnumTag = "ImageAnalysisParamIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::AnalysisInputSettingsList, schemaname, tablename,
								selecttag, idstr, aprid, apridnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetAnalysisInputSettings( DB_AnalysisInputSettingsRecord& apr, uuid__t aprid, int64_t apridnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetAnalysisInputSettingsInternal( apr, aprid, apridnum );
}

DBApi::eQueryResult DBifImpl::GetAnalysisInputSettingsInternal( DB_AnalysisInputSettingsRecord& apr, uuid__t aprid, int64_t apridnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetAnalysisInputSettingsObj( apr, resultRecs, tagList, aprid, apridnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetAnalysisInputSettingsObj( DB_AnalysisInputSettingsRecord& aisr, CRecordset& recset,
																std::vector<std::string>& taglist, uuid__t aprid, int64_t apridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( aprid ) )
	{
		apridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetAnalysisInputSettingsQueryTag( schemaName, tableName, selectTag, idStr, aprid, apridnum ) )
	{
		WriteLogEntry( "GetAnalysisInputSettingsObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, aprid, apridnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_AnalysisInputSettingsRecord aisRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseAnalysisInputSettings( queryResult, aisRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				aisr = aisRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetAnalysisDefinitionQueryTag( std::string& schemaname, std::string& tablename,
											  std::string& selecttag, std::string& idstr,
											  uuid__t adrid, int32_t adrindex, int64_t adridnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "AnalysisDefinitionID";
	//	idnumtag = "AnalysisDefinitionIdNum";
	//	indextag = "AnalysisDefinitionIndex";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::AnalysisDefinitionList, schemaname, tablename,
								selecttag, idstr, adrid, adridnum, adrindex ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetAnalysisDefinition( DB_AnalysisDefinitionRecord& def,
													 uuid__t adrid, int32_t adrindex, int64_t adridnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetAnalysisDefinitionInternal( def, adrid, adrindex, adridnum );
}

DBApi::eQueryResult DBifImpl::GetAnalysisDefinitionInternal( DB_AnalysisDefinitionRecord& def,
													 uuid__t adrid, int32_t adrindex, int64_t adridnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetAnalysisDefinitionObj( def, resultRecs, tagList, adrid, adrindex, adridnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetAnalysisDefinitionObj( DB_AnalysisDefinitionRecord& def, CRecordset& recset, std::vector<std::string>& taglist,
														uuid__t adrid, int32_t adrindex, int64_t adridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( adrid ) )
	{
		adridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetAnalysisDefinitionQueryTag( schemaName, tableName, selectTag, idStr, adrid, adrindex, adridnum ) )
	{
		WriteLogEntry( "GetAnalysisDefinitionObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, adrid, adridnum, adrindex );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_AnalysisDefinitionRecord adRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseAnalysisDefinition( queryResult, adRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				def = adRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetAnalysisParamQueryTag( std::string& schemaname, std::string& tablename,
										  std::string& selecttag, std::string& idstr,
										  uuid__t aprid, int64_t apridnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "AnalysisParamID";
	//	idnumtag = "AnalysisParamIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::AnalysisParamList, schemaname, tablename,
								selecttag, idstr, aprid, apridnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetAnalysisParam( DB_AnalysisParamRecord& apr, uuid__t aprid, int64_t apridnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetAnalysisParamInternal( apr, aprid, apridnum );
}

DBApi::eQueryResult DBifImpl::GetAnalysisParamInternal( DB_AnalysisParamRecord& apr, uuid__t aprid, int64_t apridnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetAnalysisParamObj( apr, resultRecs, tagList, aprid, apridnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetAnalysisParamObj( DB_AnalysisParamRecord& apr, CRecordset& recset,
													std::vector<std::string>& taglist, uuid__t aprid, int64_t apridnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( aprid ) )
	{
		apridnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, aprid, apridnum ) )
	{
		WriteLogEntry( "GetAnalysisParamObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, aprid, apridnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_AnalysisParamRecord apRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseAnalysisParam( queryResult, apRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				apr = apRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetIlluminatorQueryTag( std::string& schemaname, std::string& tablename,
									   std::string& selecttag, std::string& idstr,
									   std::string ilname, int16_t ilindex, int64_t ilidnum )
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId;

	ClearGuid( tmpId );

	// EXPECTED INFO:
	//	idnumtag = "IlluminatorIdNum";
	//	indextag = "IlluminatorIndex";
	//	nametag = "IlluminatorName";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::IlluminatorList, schemaname, tablename,
								selecttag, idstr, tmpId, ilidnum, ilindex, ilname ) )
	{
		return false;
	}

	return true;
}

bool DBifImpl::GetIlluminatorQueryTag( std::string& schemaname, std::string& tablename,
									   std::string& selecttag, std::string& idstr,
									   int32_t emission_wavelength, int32_t illuminator_wavelength )
{
	selecttag.clear();
	idstr.clear();

	GetListSrcTableInfo( DBApi::eListType::IlluminatorList, schemaname, tablename );

	// shouldn't ever have multiple identifiers specified...
	// but process in priority order
	if ( emission_wavelength > 0 )
	{
		selecttag = "EmissionWavelength";
		idstr = boost::str( boost::format( "%ld" ) % emission_wavelength );
	}
	else if ( illuminator_wavelength > 0 )
	{
		selecttag = "IlluminatorWavelength";
		idstr = boost::str( boost::format( "%ld" ) % illuminator_wavelength );
	}
	else
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetIlluminator( DB_IlluminatorRecord& ilr, std::string ilname, int16_t ilindex, int64_t ilidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetIlluminatorInternal( ilr, ilname, ilindex, ilidnum );
}

DBApi::eQueryResult DBifImpl::GetIlluminatorInternal( DB_IlluminatorRecord& ilr, std::string ilname, int16_t ilindex, int64_t ilidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetIlluminatorObj( ilr, resultRecs, tagList, ilname, ilindex, ilidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetIlluminator( DB_IlluminatorRecord& ilr, int16_t emission_wavelength, int16_t illuminator_wavelength )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetIlluminatorInternal( ilr, emission_wavelength, illuminator_wavelength );
}

DBApi::eQueryResult DBifImpl::GetIlluminatorInternal( DB_IlluminatorRecord& ilr, int16_t emission_wavelength, int16_t illuminator_wavelength )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetIlluminatorObj( ilr, resultRecs, tagList, emission_wavelength, illuminator_wavelength );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetIlluminatorObj( DB_IlluminatorRecord& ilr, CRecordset& recset, std::vector<std::string>& taglist,
												 std::string ilname, int16_t ilindex, int64_t ilidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	uuid__t tmpUid;

	ClearGuid( tmpUid );

	if ( ilname.length() > 0 || ilindex >= 0 )
	{
		ilidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetIlluminatorQueryTag( schemaName, tableName, selectTag, idStr, ilname, ilindex, ilidnum ) )
	{
		WriteLogEntry( "GetIlluminatorObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpUid, ilidnum, ilindex, ilname );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_IlluminatorRecord ilRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseIlluminator( queryResult, ilRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				ilr = ilRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetIlluminatorObj( DB_IlluminatorRecord& ilr, CRecordset& recset, std::vector<std::string>& taglist,
												 int16_t emission_wvlength, int16_t illuminator_wvlength )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	uuid__t tmpUid;

	ClearGuid( tmpUid );

	if ( emission_wvlength == 0 && illuminator_wvlength == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetIlluminatorQueryTag( schemaName, tableName, selectTag, idStr, emission_wvlength, illuminator_wvlength ) )
	{
		WriteLogEntry( "GetIlluminatorObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpUid, NO_ID_NUM, INVALID_INDEX, idStr );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_IlluminatorRecord ilRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseIlluminator( queryResult, ilRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				ilr = ilRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetUserQueryTag( std::string& schemaname, std::string& tablename,
								std::string& selecttag, std::string& idstr,
								uuid__t userid, int64_t useridnum, std::string username )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "UserID";
	//	idnumtag = "UserIdNum";
	//	nametag = "UserName";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::UserList, schemaname, tablename,
								selecttag, idstr, userid, useridnum, INVALID_INDEX, username ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetUser( DB_UserRecord& ur, uuid__t userid, std::string username, DBApi::eUserType user_type )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetUserInternal( ur, userid, username, user_type );
}

DBApi::eQueryResult DBifImpl::GetUserInternal( DB_UserRecord& ur, uuid__t userid, std::string username, DBApi::eUserType user_type )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetUserObj( ur, resultRecs, tagList, userid, NO_ID_NUM, username, user_type );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetUserObj( DB_UserRecord& ur, CRecordset& recset, std::vector<std::string>& taglist,
										  uuid__t userid, int64_t useridnum, std::string username, DBApi::eUserType user_type )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	std::string whereStr = "";

	if ( GuidValid( userid ) )
	{
		useridnum = NO_ID_NUM;
	}
	else
	{
		if ( useridnum <= 0 && username.length() <= 0 )
		{
			WriteLogEntry( "GetUserObj: failed: no valid identifiers supplied", InputErrorMsgType );
			return DBApi::eQueryResult::BadQuery;
		}

		if (username.length() > 0)
		{
			useridnum = NO_ID_NUM;

			if ( user_type != DBApi::eUserType::AllUsers )
			{
				std::string adState = "";

				switch( user_type )
				{
					case DBApi::eUserType::LocalUsers:
					{
						adState = FalseStr;
					}
					break;

					case DBApi::eUserType::AdUsers:
					{
						adState = TrueStr;
					}
					break;
				}

				if ( adState.length() > 0 )
				{
					std::vector<DBApi::eListFilterCriteria> filterTypes = {};
					std::vector<std::string> compareOps = {};
					std::vector<std::string> compareVals = {};

					filterTypes.push_back( DBApi::eListFilterCriteria::UserTypeFilter );
					compareOps.push_back( "=" );
					compareVals.push_back( adState );

					if ( !GetUserListFilter( whereStr, filterTypes, compareOps, compareVals ) )
					{
						whereStr.clear();
					}
				}
			}
		}
	}

	// identifier validation is performed in the tag creation method
	if ( !GetUserQueryTag( schemaName, tableName, selectTag, idStr, userid, useridnum, username ) )
	{
		WriteLogEntry( "GetUserObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, userid, useridnum, INVALID_INDEX, username, whereStr );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		int recCnt = recset.GetRecordCount();

		if ( recCnt > 0 )
		{
			DB_UserRecord uRec = {};
			bool parseOk = false;
			std::vector<DB_UserRecord> userList = {};

			if ( recset.CanScroll() )
			{
				recset.MoveFirst();

				parseOk = true;
				while ( ( !recset.IsEOF() ) && ( parseOk ) )
				{
					parseOk = ParseUser( queryResult, uRec, recset, taglist );
					if ( parseOk )
					{
						userList.push_back( uRec );
					}
					else
					{
						queryResult = DBApi::eQueryResult::ParseFailure;
					}

					if ( recset.CanScroll() )
					{
						recset.MoveNext();
					}
					else
					{
						break;
					}
				}

				if ( userList.size() > 1 )
				{
					queryResult = DBApi::eQueryResult::MultipleObjectsFound;
				}
			}

			if ( parseOk )
			{
				ur = userList.at(0);
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetRoleQueryTag( std::string& schemaname, std::string& tablename,
								std::string& selecttag, std::string& idstr,
								uuid__t userid, int64_t useridnum, std::string username )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "UserID";
	//	idnumtag = "UserIdNum";
	//	nametag = "UserName";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::RolesList, schemaname, tablename,
								selecttag, idstr, userid, useridnum, INVALID_INDEX, username ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetRole( DB_UserRoleRecord& rr, uuid__t roleid, std::string rolename )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetRoleInternal( rr, roleid, rolename );
}

DBApi::eQueryResult DBifImpl::GetRoleInternal( DB_UserRoleRecord& rr, uuid__t roleid, std::string rolename )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetRoleObj( rr, resultRecs, tagList, roleid, NO_ID_NUM, rolename );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetRoleObj( DB_UserRoleRecord& rr, CRecordset& recset, std::vector<std::string>& taglist,
										  uuid__t roleid, int64_t roleidnum, std::string rolename )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( roleid ) )
	{
		roleidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetRoleQueryTag( schemaName, tableName, selectTag, idStr, roleid, roleidnum, rolename) )
	{
		WriteLogEntry( "GetRoleObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, roleid, roleidnum, INVALID_INDEX, rolename );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_UserRoleRecord rRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseRole( queryResult, rRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				rr = rRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetUserPropertyQueryTag( std::string& schemaname, std::string& tablename,
										std::string& selecttag, std::string& idstr,
										int32_t propindex, std::string propname, int64_t idnum )
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId;

	ClearGuid( tmpId );

	// EXPECTED INFO:
	//	idnumtag = "PropertyIdNum";		// not currently used...
	//	indextag = "PropertyIndex";
	//	nametag = "PropertyName";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::UserPropertiesList, schemaname, tablename,
								selecttag, idstr, tmpId, idnum, propindex, propname ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetUserProperty( DB_UserPropertiesRecord& upr, int32_t propindex, std::string propname, int64_t propidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetUserPropertyInternal( upr, propindex, propname, propidnum );
}

DBApi::eQueryResult DBifImpl::GetUserPropertyInternal( DB_UserPropertiesRecord& upr, int32_t propindex, std::string propname, int64_t propidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetUserPropertyObj( upr, resultRecs, tagList, propindex, propname, propidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetUserPropertyObj( DB_UserPropertiesRecord& upr, CRecordset& recset, std::vector<std::string>& taglist,
												  int32_t propindex, std::string propname, int64_t propidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	// identifier validation is performed in the tag creation method
	if ( !GetUserPropertyQueryTag( schemaName, tableName, selectTag, idStr, propindex, propname, propidnum ) )
	{
		WriteLogEntry( "GetUserPropertyObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	uuid__t tmpId;

	ClearGuid( tmpId );
	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpId, propidnum, propindex, propname );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_UserPropertiesRecord upRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseUserProperty( queryResult, upRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				upr = upRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSignatureQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t sigid, int64_t sigidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SignatureID";
	//	idnumtag = "SignatureIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::SignatureDefList, schemaname, tablename,
								selecttag, idstr, sigid, sigidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSignature( DB_SignatureRecord& sigr, uuid__t sigid, int64_t sigidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSignatureInternal( sigr, sigid, sigidnum );
}

DBApi::eQueryResult DBifImpl::GetSignatureInternal( DB_SignatureRecord& sigr, uuid__t sigid, int64_t sigidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSignatureObj( sigr, resultRecs, tagList, sigid, sigidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSignatureObj( DB_SignatureRecord& sigr, CRecordset& recset,
											   std::vector<std::string>& taglist, uuid__t sigid, int64_t sigidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( sigid ) )
	{
		sigidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSignatureQueryTag( schemaName, tableName, selectTag, idStr, sigid, sigidnum ) )
	{
		WriteLogEntry( "GetSignatureObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, sigid, sigidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SignatureRecord sigRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSignature( queryResult, sigRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				sigr = sigRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetReagentTypeQueryTag( std::string& schemaname, std::string& tablename,
									   std::string& selecttag, std::string& idstr,
									   int64_t rxidnum, std::string tagsn )
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId = {};

	ClearGuid( tmpId );

	// EXPECTED INFO:
	//	idnumtag = "ReagentIdNum";
	//	indextag = "ReagentTypeNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::ReagentInfoList, schemaname, tablename,
								selecttag, idstr, tmpId, rxidnum, INVALID_INDEX, tagsn ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetReagentType( DB_ReagentTypeRecord& rxr, int64_t rxidnum, std::string tagsn )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetReagentTypeInternal( rxr, rxidnum, tagsn );
}

DBApi::eQueryResult DBifImpl::GetReagentTypeInternal( DB_ReagentTypeRecord& rxr, int64_t rxidnum, std::string tagsn )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetReagentTypeObj( rxr, resultRecs, tagList, rxidnum, tagsn );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetReagentTypeObj( DB_ReagentTypeRecord& rxr, CRecordset& recset,
												 std::vector<std::string>& taglist, int64_t rxidnum, std::string tagsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	uuid__t tmpId = {};

	ClearGuid( tmpId );

	// identifier validation is performed in the tag creation method
	if ( !GetReagentTypeQueryTag( schemaName, tableName, selectTag, idStr, rxidnum, tagsn ) )
	{
		WriteLogEntry( "GetReagentTypeObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpId, rxidnum, INVALID_INDEX, tagsn );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_ReagentTypeRecord rxRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseReagentType( queryResult, rxRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				rxr = rxRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetCellHealthReagentsQueryTag(std::string& schemaname, std::string& tablename,
	std::string& selecttag, std::string& idstr,
	std::string ilname, int16_t ilindex, int64_t ilidnum)
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId;

	ClearGuid(tmpId);

	if (!GetSrcTableQueryInfo(DBApi::eListType::CellHealthReagentsList, schemaname, tablename,
		selecttag, idstr, tmpId, ilidnum, ilindex, ilname))
	{
		return false;
	}

	return true;
}

bool DBifImpl::GetCellHealthReagentsQueryTag (std::string& schemaname, std::string& tablename,
                                             std::string& selecttag, std::string& idstr,
	                                         int16_t type)
{
	selecttag.clear();
	idstr.clear();

	GetListSrcTableInfo(DBApi::eListType::CellHealthReagentsList, schemaname, tablename);

	selecttag = "Type";
	idstr = boost::str(boost::format("%ld") % type);

	return true;
}

DBApi::eQueryResult DBifImpl::GetCellHealthReagent(DB_CellHealthReagentRecord& chr, int16_t type)
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck(pDb);

	if (loginType == DBApi::eLoginType::NoLogin)
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetCellHealthReagentInternal(chr, type);
}

DBApi::eQueryResult DBifImpl::GetCellHealthReagentInternal(DB_CellHealthReagentRecord& chr, int16_t type)
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetCellHealthReagentObj(chr, resultRecs, tagList, type);

	if (resultRecs.IsOpen())
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetCellHealthReagentObj (DB_CellHealthReagentRecord& chr, 
	CRecordset& recset, std::vector<std::string>& taglist, int16_t type)
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	uuid__t tmpId = {};

	ClearGuid(tmpId);

	// identifier validation is performed in the tag creation method
	if (!GetCellHealthReagentsQueryTag(schemaName, tableName, selectTag, idStr, type))
	{
		WriteLogEntry("GetCellHealthReagentObj: failed: no valid query tags found", ErrorMsgType);
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery(schemaName, tableName, recset, taglist, selectTag, tmpId, type);

	if (queryResult == DBApi::eQueryResult::QueryOk)
	{
		if (recset.GetRecordCount() > 0)
		{
			DB_CellHealthReagentRecord rxRec = {};
			bool parseOk = false;

			if (recset.GetRecordCount() > 0 && recset.CanScroll())
			{
				recset.MoveFirst();

				if (!recset.IsEOF())
				{
					parseOk = ParseCellHealthReagent(queryResult, rxRec, recset, taglist);
				}
			}

			if (parseOk)
			{
				chr = rxRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetBioProcessQueryTag( std::string& schemaname, std::string& tablename,
									  std::string& selecttag, std::string& idstr,
									  uuid__t bpid, int64_t bpidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "BioProcessID";
	//	idnumtag = "BioProcessIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::BioProcessList, schemaname, tablename,
								selecttag, idstr, bpid, bpidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetBioProcess( DB_BioProcessRecord& bpr, uuid__t bpid, int64_t bpidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetBioProcessInternal( bpr, bpid, bpidnum );
}

DBApi::eQueryResult DBifImpl::GetBioProcessInternal( DB_BioProcessRecord& bpr, uuid__t bpid, int64_t bpidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetBioProcessObj( bpr, resultRecs, tagList, bpid, bpidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetBioProcessObj( DB_BioProcessRecord& bpr, CRecordset& recset,
												std::vector<std::string>& taglist, uuid__t bpid, int64_t bpidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( bpid ) )
	{
		bpidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetBioProcessQueryTag( schemaName, tableName, selectTag, idStr, bpid, bpidnum ) )
	{
		WriteLogEntry( "GetBioProcessObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, bpid, bpidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_BioProcessRecord bpRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseBioProcess( queryResult, bpRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				bpr = bpRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetQcProcessQueryTag( std::string& schemaname, std::string& tablename,
									 std::string& selecttag, std::string& idstr,
									 uuid__t qcid, int64_t qcidnum, std::string qcname )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "QcProcessID";
	//	idnumtag = "QcProcessIdNum";
	//	nametag = "QcName";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::QcProcessList, schemaname, tablename,
								selecttag, idstr, qcid, qcidnum, INVALID_INDEX, qcname ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetQcProcess( DB_QcProcessRecord& qpr, uuid__t qcid, int64_t qcidnum, std::string qcname )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetQcProcessInternal( qpr, qcid, qcidnum, qcname );
}

DBApi::eQueryResult DBifImpl::GetQcProcessInternal( DB_QcProcessRecord& qpr, uuid__t qcid, int64_t qcidnum, std::string qcname )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetQcProcessObj( qpr, resultRecs, tagList, qcid, qcidnum, qcname );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetQcProcessObj( DB_QcProcessRecord& qpr, CRecordset& recset,
											   std::vector<std::string>& taglist, uuid__t qcid, int64_t qcidnum, std::string qcname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	std::string whereStr = "";

	if ( GuidValid( qcid ) )
	{
		qcidnum = NO_ID_NUM;
	}
	else
	{
		if ( qcidnum <= 0 && qcname.length() <= 0 )
		{
			WriteLogEntry( "GetQcProcessObj: failed: no valid identifiers supplied", InputErrorMsgType );
			return DBApi::eQueryResult::BadQuery;
		}

		if ( qcname.length() > 0 )
		{
			qcidnum = NO_ID_NUM;

			std::vector<DBApi::eListFilterCriteria> filterTypes = {};
			std::vector<std::string> compareOps = {};
			std::vector<std::string> compareVals = {};

			filterTypes.push_back( DBApi::eListFilterCriteria::ItemNameFilter );
			compareOps.push_back( "=" );
			compareVals.push_back( qcname );

			if ( !GetQcProcessListFilter( whereStr, filterTypes, compareOps, compareVals ) )
			{
				whereStr.clear();
			}
		}
	}

	// identifier validation is performed in the tag creation method
	if ( !GetQcProcessQueryTag( schemaName, tableName, selectTag, idStr, qcid, qcidnum, qcname ) )
	{
		WriteLogEntry( "GetQcProcessObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, qcid, qcidnum, INVALID_INDEX, qcname, whereStr );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		int recCnt = recset.GetRecordCount();

		if ( recCnt > 0 )
		{
			DB_QcProcessRecord qcRec = {};
			bool parseOk = false;
			std::vector<DB_QcProcessRecord> qcList = {};

			if ( recset.CanScroll() )
			{
				recset.MoveFirst();

				parseOk = true;
				while ( ( !recset.IsEOF() ) && ( parseOk ) )
				{
					parseOk = ParseQcProcess( queryResult, qcRec, recset, taglist );
					if ( parseOk )
					{
						qcList.push_back( qcRec );
					}
					else
					{
						queryResult = DBApi::eQueryResult::ParseFailure;
					}

					if ( recset.CanScroll() )
					{
						recset.MoveNext();
					}
					else
					{
						break;
					}
				}

				if ( qcList.size() > 1 )
				{
					queryResult = DBApi::eQueryResult::MultipleObjectsFound;
				}
			}

			if ( parseOk )
			{
				qpr = qcList.at( 0 );
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetCalibrationQueryTag( std::string& schemaname, std::string& tablename,
									   std::string& selecttag, std::string& idstr,
									   uuid__t calid, int64_t calidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "CalibrationID";
	//	idnumtag = "CalibrationIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::CalibrationsList, schemaname, tablename,
								selecttag, idstr, calid, calidnum ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetCalibration( DB_CalibrationRecord& car, uuid__t calid, int64_t calidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetCalibrationInternal( car, calid, calidnum );
}

DBApi::eQueryResult DBifImpl::GetCalibrationInternal( DB_CalibrationRecord& car, uuid__t calid, int64_t calidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetCalibrationObj( car, resultRecs, tagList, calid, calidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetCalibrationObj( DB_CalibrationRecord& car, CRecordset& recset,
												 std::vector<std::string>& taglist, uuid__t calid, int64_t calidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( calid ) )
	{
		calidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetCalibrationQueryTag( schemaName, tableName, selectTag, idStr, calid, calidnum ) )
	{
		WriteLogEntry( "GetCalibrationObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, calid, calidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_CalibrationRecord calRec = {};
			bool parseOk = false;

			if ( recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseCalibration( queryResult, calRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				car = calRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetInstConfigQueryTag( std::string& schemaname, std::string& tablename,
									  std::string& selecttag, std::string& idstr,
									  std::string instsn, int64_t cfgidnum )
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId;

	ClearGuid( tmpId );

	// EXPECTED INFO:
	//	idtag = "InstrumentSN";
	//	idnumtag = "InstrumentIdNum";

	if ( !GetSrcTableQueryInfo( DBApi::eListType::InstrumentConfigList, schemaname, tablename,
								selecttag, idstr, tmpId, cfgidnum, INVALID_INDEX, instsn ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetInstConfig( DB_InstrumentConfigRecord& icr, std::string instsn, int64_t cfgidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetInstConfigInternal( icr, instsn, cfgidnum );
}

DBApi::eQueryResult DBifImpl::GetInstConfigInternal( DB_InstrumentConfigRecord& icr, std::string instsn, int64_t cfgidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetInstConfigObj( icr, resultRecs, tagList, instsn, cfgidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetInstConfigObj( DB_InstrumentConfigRecord& icr, CRecordset& recset,
												std::vector<std::string>& taglist, std::string instsn, int64_t cfgidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	uuid__t tmpUid = {};

	ClearGuid( tmpUid );

	if ( cfgidnum <= 0 && instsn.length() <= 0 )
	{
		cfgidnum = 1;			// requesting the default record; 
	}
	else if ( instsn.length() > 0 )
	{
		cfgidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetInstConfigQueryTag( schemaName, tableName, selectTag, idStr, instsn, cfgidnum ) )
	{
		WriteLogEntry( "GetInstConfigObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpUid, cfgidnum, INVALID_INDEX, instsn );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_InstrumentConfigRecord cfgRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseInstConfig( queryResult, cfgRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				icr = cfgRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetLogEntryQueryTag( std::string& schemaname, std::string& tablename, std::string& selecttag, std::string& idstr, int64_t entry_idnum )
{
	selecttag.clear();
	idstr.clear();

	uuid__t tmpId;

	ClearGuid( tmpId );

	// EXPECTED INFO:
	//	idtag = "InstrumentSN";
	//	idnumtag = "InstrumentIdNum";

	if (entry_idnum <= 0)
	{
		entry_idnum = 1;
	}
	if ( !GetSrcTableQueryInfo( DBApi::eListType::LogEntryList, schemaname, tablename,
								selecttag, idstr, tmpId, entry_idnum, INVALID_INDEX, "" ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetLogEntry( DB_LogEntryRecord& logr, int64_t logidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetLogEntryInternal( logr, logidnum );
}

DBApi::eQueryResult DBifImpl::GetLogEntryInternal( DB_LogEntryRecord& logr, int64_t logidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetLogEntryObj( logr, resultRecs, tagList, logidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetLogEntryObj( DB_LogEntryRecord& logr, CRecordset& recset,
												std::vector<std::string>& taglist, int64_t logidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";
	uuid__t tmpUid = {};

	ClearGuid( tmpUid );

	if ( logidnum <= 0 )
	{
		WriteLogEntry( "GetLogEntryObj: failed: no valid identifier supplied", InputErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetLogEntryQueryTag( schemaName, tableName, selectTag, idStr, logidnum ) )
	{
		WriteLogEntry( "GetLogEntryObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, tmpUid, logidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_LogEntryRecord logRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseLogEntry( queryResult, logRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				logr = logRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}

bool DBifImpl::GetSchedulerConfigQueryTag( std::string& schemaname, std::string& tablename,
										   std::string& selecttag, std::string& idstr,
										   uuid__t scid, int64_t scidnum )
{
	selecttag.clear();
	idstr.clear();

	// EXPECTED INFO:
	//	idtag = "SchedulerConfigID";
	//	idnumtag = "SchedulerConfigIdNum";

	if ( scidnum <= 0 )
	{
		scidnum = 1;
	}
	if ( !GetSrcTableQueryInfo( DBApi::eListType::SchedulerConfigList, schemaname, tablename,
								selecttag, idstr, scid, scidnum, INVALID_INDEX, "" ) )
	{
		return false;
	}

	return true;
}

DBApi::eQueryResult DBifImpl::GetSchedulerConfig( DB_SchedulerConfigRecord& scr, uuid__t scid, int64_t scidnum )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	return GetSchedulerConfigInternal( scr, scid, scidnum );
}

DBApi::eQueryResult DBifImpl::GetSchedulerConfigInternal( DB_SchedulerConfigRecord& scr, uuid__t scid, int64_t scidnum )
{
	CRecordset resultRecs;		// no default initializer; initialized by the method
	std::vector<std::string> tagList = {};
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetSchedulerConfigObj( scr, resultRecs, tagList, scid, scidnum );

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::GetSchedulerConfigObj( DB_SchedulerConfigRecord& scr, CRecordset& recset,
													 std::vector<std::string>& taglist, uuid__t scid, int64_t scidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	std::string selectTag = "";
	std::string idStr = "";
	std::string schemaName = "";
	std::string tableName = "";

	if ( GuidValid( scid ) )
	{
		scidnum = NO_ID_NUM;
	}

	// identifier validation is performed in the tag creation method
	if ( !GetSchedulerConfigQueryTag( schemaName, tableName, selectTag, idStr, scid, scidnum ) )
	{
		WriteLogEntry( "GetSchedulerConfigObj: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	queryResult = DoObjectQuery( schemaName, tableName, recset, taglist, selectTag, scid, scidnum );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		if ( recset.GetRecordCount() > 0 )
		{
			DB_SchedulerConfigRecord scRec = {};
			bool parseOk = false;

			if ( recset.GetRecordCount() > 0 && recset.CanScroll() )
			{
				recset.MoveFirst();

				if ( !recset.IsEOF() )
				{
					parseOk = ParseSchedulerConfig( queryResult, scRec, recset, taglist );
				}
			}

			if ( parseOk )
			{
				scr = scRec;
			}
			else
			{
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoData;
		}
	}

	return queryResult;
}
