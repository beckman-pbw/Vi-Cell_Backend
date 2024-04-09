// Database interface : implementation file
//

#pragma once

#include "pch.h"


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#include "DBif_ObjTags.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif
#include "StageDefines.hpp"



static const std::string MODULENAME = "DBif_Update";




////////////////////////////////////////////////////////////////////////////////
// Internal object updating methods
////////////////////////////////////////////////////////////////////////////////


DBApi::eQueryResult DBifImpl::UpdateWorklistRecord( DB_WorklistRecord& wlr, bool doSampleSets )
{
	if ( !GuidValid( wlr.WorklistId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_WorklistRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetWorklistObj( dbRec, resultRec, tagList, wlr.WorklistId, wlr.WorklistIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	if ( dbRec.WorklistStatus != static_cast<int32_t>( wlr.WorklistStatus ) &&
		 dbRec.WorklistStatus > static_cast<int32_t>( wlr.WorklistStatus ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	bool dataOk = false;
	bool ssrFound = false;
	bool runDateUpdate = false;
	bool runStarting = false;
	int32_t idFails = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t updateIdx = 0;
	int32_t wlSampleSetCnt = NoItemListCnt;
	int32_t numSampleSetsTagIdx = ListCntIdxNotSet;
	int32_t wlProcessedSetCnt = NoItemListCnt;
	int32_t processedSetCntTagIdx = ListCntIdxNotSet;
	size_t listCnt = 0;
	size_t listSize = 0;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
	size_t tagListSize = tagList.size();
	system_TP zeroTP = {};

	// new status indicates just starting the list run
	if ( dbRec.WorklistStatus < static_cast<int32_t>( DBApi::eWorklistStatus::WorklistRunning ) &&
		 wlr.WorklistStatus >= static_cast<int32_t>( DBApi::eWorklistStatus::WorklistRunning ) )
	{
		runStarting = true;
		runDateUpdate = true;
	}

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		// updating the following items is not allowed once the worklist has been started
		if ( dbRec.WorklistStatus < static_cast<int32_t>( DBApi::eWorklistStatus::WorklistRunning ) )
		{
			if ( tag == WL_NameTag )						// "WorklistName"
			{
				if ( wlr.WorklistNameStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % wlr.WorklistNameStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == WL_CommentsTag )				// "ListComments"
			{
				if ( wlr.ListComments.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % wlr.ListComments );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == WL_InstSNTag )					// "InstrumentSN"
			{
				if ( wlr.InstrumentSNStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % wlr.InstrumentSNStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == WL_CreateUserIdTag )			// "CreationUserID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.CreationUserId, wlr.CreationUserId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_RunUserIdTag )				// "RunUserID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				if ( !GuidUpdateCheck( dbRec.RunUserId, wlr.RunUserId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_AcquireSampleTag )			// "AcquireSample"
			{
				valStr = ( wlr.AcquireSample == true ) ? TrueStr : FalseStr;
			}
			else if ( tag == WL_CarrierTypeTag )			// "CarrierType"
			{
				valStr = boost::str( boost::format( "%d" ) % wlr.CarrierType );
			}
			else if ( tag == WL_PrecessionTag )				// "ByColumn"
			{
				bool precessionState = false;
				if ( wlr.CarrierType == (uint16_t) eCarrierType::ePlate_96 )
				{
					precessionState = wlr.ByColumn;
				}
				valStr = ( precessionState == true ) ? TrueStr : FalseStr;
			}
			else if ( tag == WL_SaveImagesTag )				// "SaveImages"
			{
				valStr = boost::str( boost::format( "%d" ) % wlr.SaveNthImage );
			}
			else if ( tag == WL_WashTypeTag )				// "WashType"
			{
				valStr = boost::str( boost::format( "%d" ) % wlr.WashTypeIndex );
			}
			else if ( tag == WL_DilutionTag )				// "Dilution"
			{
				valStr = boost::str( boost::format( "%d" ) % wlr.Dilution );
			}
			else if ( tag == WL_DfltSetNameTag )			// "DefaultSetName"
			{
				if ( wlr.SampleSetNameStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % wlr.SampleSetNameStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == WL_DfltItemNameTag )			// "DefaultItemName"
			{
				if ( wlr.SampleItemNameStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % wlr.SampleItemNameStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == WL_ImageAnalysisParamIdTag )	// "ImageAnalysisParamID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				if ( !GuidUpdateCheck( dbRec.ImageAnalysisParamId, wlr.ImageAnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_AnalysisDefIdTag )			// "AnalysisDefinitionID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.AnalysisDefId, wlr.AnalysisDefId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_AnalysisDefIdxTag )			// "AnalysisDefinitionIndex"
			{
				valStr = boost::str( boost::format( "'%u'" ) % wlr.AnalysisDefIndex );
			}
			else if ( tag == WL_AnalysisParamIdTag )		// "AnalysisParameterID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.AnalysisParamId, wlr.AnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_CellTypeIdTag )				// "CellTypeID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				if ( !GuidUpdateCheck( dbRec.CellTypeId, wlr.CellTypeId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_CellTypeIdxTag )			// "CellTypeIndex"
			{
				valStr = boost::str( boost::format( "%ld" ) % (int32_t) wlr.CellTypeIndex );
			}
			else if ( tag == WL_BioProcessIdTag )			// "BioProcessID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.BioProcessId, wlr.BioProcessId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_QcProcessIdTag )			// "QcProcessID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.QcProcessId, wlr.QcProcessId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == WL_WorkflowIdTag )				// WorkflowID"
			{
				int32_t errCode = 0;

				if ( !GuidUpdateCheck( dbRec.WorkflowId, wlr.WorkflowId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
		}

		// all the following items may be updated while a worklist is running...
		if ( tag == WL_StatusTag )						// "WorklistStatus"
		{
			if ( wlr.WorklistStatus >= dbRec.WorklistStatus )
			{
				valStr = boost::str( boost::format( "%d" ) % wlr.WorklistStatus );
			}
		}
		else if ( tag == WL_RunDateTag )				// "RunDate"
		{    // update just prior to beginning sample processing; follow with status update
			if ( runDateUpdate )        // only update run date/time when list is first started
			{
				if ( wlr.RunDateTP == zeroTP )
				{
					GetDbCurrentTimeString( valStr, wlr.RunDateTP );
				}
				else
				{
					GetDbTimeString( wlr.RunDateTP, valStr );
				}
				runDateUpdate = false;
			}
			else
			{
				if ( wlr.RunDateTP != zeroTP )
				{
					GetDbTimeString( wlr.RunDateTP, valStr );
				}
			}
		}
		else if ( tag == WL_SampleSetCntIdTag )			// "SampleSetCount"
		{
			if ( wlSampleSetCnt < ItemListCntOk )      // hasn't yet been validated against the object list; don't write yet
			{
				numSampleSetsTagIdx = tagIndex;
			}
			else
			{
				// will be the actual number of elements processed by the work queue on completion; '0' to start
				if ( wlr.SampleSetCount >= dbRec.SampleSetCount )
				{
					valStr = boost::str( boost::format( "'%u'" ) % wlr.SampleSetCount );
				}
				wlSampleSetCnt = wlr.SampleSetCount;
			}
		}
		else if ( tag == WL_ProcessedSetCntTag )		// "ProcessedSetCount"
		{
			if ( wlProcessedSetCnt < ItemListCntOk )      // hasn't yet been validated against the object list; don't write yet
			{
				processedSetCntTagIdx = tagIndex;
			}
			else
			{
				// will be the actual number of elements processed by the work queue on completion; '0' to start
				if ( wlr.ProcessedSetCount >= dbRec.ProcessedSetCount )
				{
					valStr = boost::str( boost::format( "'%u'" ) % wlr.ProcessedSetCount );
				}
				wlProcessedSetCnt = wlr.ProcessedSetCount;
			}

		}
		else if ( tag == WL_SampleSetIdListTag )		// "SampleSetIDList"
		{    // DB stores a list of Sample Set Item IDs in the parent sample Set object, not the item objects
			if ( doSampleSets )
			{
				std::vector<uuid__t> worklistSampleSetIdList = {};
				int32_t processedCnt = 0;

				foundCnt = tagsFound;
				listSize = wlr.SSList.size();
				listCnt = listSize;

				valStr.clear();

				if ( wlProcessedSetCnt < ItemListCntOk )
				{
					wlProcessedSetCnt = wlr.ProcessedSetCount;
				}

				// check against the number of objects indicated, not from the vector...
				if ( wlSampleSetCnt < ItemListCntOk )
				{
					wlSampleSetCnt = wlr.SampleSetCount;
				}

				if ( wlSampleSetCnt != listSize )
				{
					if ( wlSampleSetCnt >= ItemListCntOk )
					{
						foundCnt = SetItemListCnt;
					}
				}

				for ( size_t i = 0; i < listCnt; i++ )
				{
					ssrFound = false;
					DB_SampleSetRecord& ssr = wlr.SSList.at( i );

					if ( GuidValid( ssr.SampleSetId ) )
					{
						idNum = ssr.SampleSetIdNum;
					}
					else
					{
						// appears to be a new record; add it to the db and the list
						insertResult = InsertSampleSetRecord( ssr, true );
						if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( ssr.SampleSetIdNum > INVALID_ID_NUM ) )
						{
							idNum = ssr.SampleSetIdNum;
							wlr.SSList.at( i ).SampleSetIdNum = idNum;      // update the id in the passed-in object
						}
					}

					if ( idNum > INVALID_ID_NUM )
					{
						// check for retrieval of inserted/found item by uuid__t value...
						// if uuid__t is valid, do retrieval by using it, otherwise
						// just check for retrieval by idnum
						if ( GuidValid( ssr.SampleSetId ) )
						{
							idNum = NO_ID_NUM;
						}

						DB_SampleSetRecord chk_ssr = {};

						if ( GetSampleSetInternal( chk_ssr, ssr.SampleSetId ) == DBApi::eQueryResult::QueryOk )
						{
							ssrFound = true;
							ssr.SampleSetIdNum = chk_ssr.SampleSetIdNum;
							ssr.SampleSetStatus = chk_ssr.SampleSetStatus;
							idNum = chk_ssr.SampleSetIdNum;	// indicate this sample has already been inserted

							if ( ssr.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetComplete ) )
							{
								processedCnt++;
							}
						}
					}

					if ( !ssrFound )     // also gets here when idNum == 0
					{
						foundCnt = ListItemNotFound;
						valStr.clear();
						dataOk = false;
						queryResult = DBApi::eQueryResult::QueryFailed;
					}
					else
					{
						worklistSampleSetIdList.push_back( ssr.SampleSetId );
					}
				}

				listCnt = worklistSampleSetIdList.size();

				if ( dataOk )
				{
					if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, worklistSampleSetIdList ) != listCnt )
					{
						foundCnt = ItemValCreateError;
						valStr.clear();
						dataOk = false;
						queryResult = DBApi::eQueryResult::QueryFailed;
					}

					if ( foundCnt == SetItemListCnt )
					{
						wlr.SampleSetCount = static_cast<int16_t>( listCnt );
						foundCnt = tagsFound;
					}
				}

				if ( foundCnt < ListItemsOk )
				{
					tagsFound = foundCnt;
					valStr.clear();
					dataOk = false;
					break;
				}

				if ( listCnt != wlSampleSetCnt || listCnt != wlr.SampleSetCount )
				{
					wlSampleSetCnt = static_cast<int32_t>( listCnt );
					wlr.SampleSetCount = static_cast<int16_t>( listCnt );
				}

				if ( processedCnt != wlProcessedSetCnt || processedCnt != wlr.ProcessedSetCount )
				{
					wlProcessedSetCnt = processedCnt;
					wlr.ProcessedSetCount = wlProcessedSetCnt;
				}

				// need to do the 'num' field addition here, once it's been validated...
				if ( dataOk )
				{

					if ( numSampleSetsTagIdx >= ItemTagIdxOk )			// the number of sample sets tag has already been handled
					{
						tagsFound++;
						cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSampleSetsTagIdx ) );		// make the quoted string expected by the database for value names
						cntValStr = boost::str( boost::format( "%d" ) % wlr.SampleSetCount );
						AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
					}

					if ( processedSetCntTagIdx >= ItemTagIdxOk )		// the processed sample sets tag has already been handled
					{
						if ( wlr.ProcessedSetCount >= dbRec.ProcessedSetCount )
						{
							tagsFound++;
							cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( processedSetCntTagIdx ) );		// make the quoted string expected by the database for value names
							cntValStr = boost::str( boost::format( "%d" ) % wlr.ProcessedSetCount );
							AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
						}
					}
				}
			}
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );					// make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0)
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		if ( runStarting )
		{
			for ( size_t i = 0; i < listCnt; i++ )
			{
				wlr.SSList.at( i ).SampleSetStatus = static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetNotRun );
			}
		}

		GetWorklistQueryTag( schemaName, tableName, selectTag, idStr, wlr.WorklistId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{	// this may be removed if there are valid reasons to allow an update that does no updating...
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateWorklistStatus( uuid__t worklistid, DBApi::eWorklistStatus status )
{
	if ( !GuidValid( worklistid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord dbRec = {};

	queryResult = GetWorklist( dbRec, worklistid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// TODO: do we want to prevent status from regressing?

	if ( dbRec.WorklistStatus == static_cast< int32_t >( DBApi::eWorklistStatus::WorklistComplete ) ||
		 dbRec.WorklistStatus > static_cast< int32_t >( status ) )
	{
		return DBApi::eQueryResult::StatusRegressionNotAllowed;
	}
	dbRec.WorklistStatus = static_cast<int32_t>(status);

	queryResult = UpdateWorklistRecord( dbRec, NO_SAMPLE_SET_UPDATES );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	if ( !GuidValid( worklistid ) | !GuidValid( samplesetid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord wlRec = {};

	queryResult = GetWorklist( wlRec, worklistid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	if ( wlRec.WorklistStatus == static_cast<int32_t>( DBApi::eWorklistStatus::WorklistComplete ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	bool ssFound = false;
	bool updateWl = false;

	for ( auto itemIter = wlRec.SSList.begin(); itemIter != wlRec.SSList.end(); ++itemIter )
	{
		if ( GuidsEqual( samplesetid, itemIter->SampleSetId ) )
		{
			ssFound = true;
		}
	}

	if ( ssFound )
	{
		DB_SampleSetRecord sampleRec = {};

		queryResult = GetSampleSetInternal( sampleRec, samplesetid );
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			// previous status was not complete; new status is complete
			if ( sampleRec.SampleSetStatus != static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetComplete ) &&
					status == DBApi::eSampleSetStatus::SampleSetComplete )
			{
				updateWl = true;	// note that the 'processed' count in the worklist needs to be updated...
			}
		}

		// write the status change for the sample set; must do this prior to worklist update, if required
		queryResult = UpdateSampleSetStatus( samplesetid, status );
		if ( queryResult == DBApi::eQueryResult::QueryOk && updateWl )
		{
			UpdateWorklistRecord( wlRec );
		}
	}
	else
	{
		// matching id not found in list...
		queryResult = DBApi::eQueryResult::NoResults;
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleSetRecord( DB_SampleSetRecord& ssr, bool doSampleItems, bool in_ext_transaction )
{
	if ( !GuidValid( ssr.SampleSetId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_SampleSetRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetSampleSetObj( dbRec, resultRec, tagList, ssr.SampleSetId, ssr.SampleSetIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	if ( dbRec.SampleSetStatus != ssr.SampleSetStatus &&
		 dbRec.SampleSetStatus == static_cast< int32_t >( DBApi::eSampleSetStatus::SampleSetComplete) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	bool dataOk = false;
	bool ssiFound = false;
	int32_t idFails = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t updateIdx = 0;
	int32_t ssSampleItemsCnt = NoItemListCnt;
	int32_t numSsItemsTagIdx = ListCntIdxNotSet;
	int32_t ssProcessedItemCnt = NoItemListCnt;
	int32_t processedItemCntTagIdx = ListCntIdxNotSet;
	int32_t processedCnt = 0;
	int32_t listCnt = 0;
	size_t listSize = 0;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
	size_t tagListSize = tagList.size();
	system_TP zeroTP = {};

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		// NOTE: Orphan SampleSets will have sample items aded dynamically as they are "found", so don't reject updates to the following fields based on SampleSet status
		// Non-orphan SampleSets SHOULD NOT be adding items dynamically, but still must allow status processed item count, and run date updates.
		if ( tag == SS_StatusTag )							// "SampleSetStatus"
		{
			if ( ssr.SampleSetStatus >= dbRec.SampleSetStatus )
			{
				valStr = boost::str( boost::format( "%d" ) % ssr.SampleSetStatus );
			}
		}
		else if ( tag == SS_ProcessedItemCntTag )			// "ProcessedItemsCount"
		{
			if ( ssProcessedItemCnt < ItemListCntOk )		// hasn't yet been validated against the object list; don't write yet
			{
				processedItemCntTagIdx = tagIndex;
			}
			else
			{
				if ( ssr.ProcessedItemCount >= dbRec.ProcessedItemCount )
				{
					valStr = boost::str( boost::format( "'%u'" ) % ssr.ProcessedItemCount );
				}
				ssProcessedItemCnt = ssr.ProcessedItemCount;
			}
		}
		else if ( tag == SS_RunDateTag )					// "RunDate"
		{	// update just prior to beginning sample processing; follow with status update
			// only update run date/time when list is first started
			if ( ssr.SampleSetStatus > dbRec.SampleSetStatus &&
				 dbRec.SampleSetStatus < static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetActive ) &&
				 ssr.SampleSetStatus >= static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetActive ) )
			{
				if ( ssr.RunDateTP == zeroTP )
				{
					GetDbCurrentTimeString( valStr, ssr.RunDateTP );
				}
				else
				{
					GetDbTimeString( ssr.RunDateTP, valStr );
				}
			}
			else if ( ssr.RunDateTP != zeroTP )
			{
				GetDbTimeString( ssr.RunDateTP, valStr );
			}
		}
		else if ( tag == SS_ModifyDateTag )				// "ModifyDate"
		{
			system_TP zeroTP = {};

			if ( ssr.ModifyDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, ssr.ModifyDateTP );
			}
			else
			{
				GetDbTimeString( ssr.ModifyDateTP, valStr );
			}
		}
		else if ( tag == SS_SampleItemCntTag )			// "SampleItemCount"
		{
			if ( ssSampleItemsCnt < ItemListCntOk )		// hasn't yet been validated against the object list; don't write yet
			{
				numSsItemsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "'%u'" ) % ssr.SampleItemCount );
				ssSampleItemsCnt = ssr.SampleItemCount;
			}
		}
		else if ( tag == SS_SampleItemIdListTag )		// "SampleItemIDList"
		{
			if ( doSampleItems )
			{
				std::vector<uuid__t> sampleSetItemsIdList = {};

				foundCnt = tagsFound;
				listSize = ssr.SSItemsList.size();
				listCnt = (int32_t) listSize;

				valStr.clear();

				if ( ssProcessedItemCnt < ItemListCntOk )		// hasn't yet been validated against the object list; don't write yet
				{
					ssProcessedItemCnt = ssr.ProcessedItemCount;
				}

				// check against the number of objects indicated, not from the vector...
				if ( ssSampleItemsCnt < ItemListCntOk )
				{
					ssSampleItemsCnt = ssr.SampleItemCount;
				}

				if ( ssSampleItemsCnt != listCnt )
				{
					if ( ssSampleItemsCnt >= ItemListCntOk )
					{
						foundCnt = SetItemListCnt;
					}
				}

				for ( size_t i = 0; i < listSize; i++ )
				{
					ssiFound = false;
					DB_SampleItemRecord& ssir = ssr.SSItemsList.at( i );

					//						if ( ssir.SampleItemIdNum > INVALID_ID_NUM )
					if ( GuidValid( ssir.SampleItemId ) )
					{
						idNum = ssir.SampleItemIdNum;
					}
					else
					{
						insertResult = InsertSampleItemRecord( ssir, true );
						if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( ssir.SampleItemIdNum > INVALID_ID_NUM ) )
						{
							idNum = ssir.SampleItemIdNum;
							ssr.SSItemsList.at( i ).SampleItemIdNum = idNum;    // update the id in the passed-in object
						}
					}

					if ( idNum > INVALID_ID_NUM )
					{
						// check for retrieval of inserted/found item by uuid__t value...
						// if uuid__t is valid, do retrieval by using it, otherwise
						// just check for retrieval by idnum
						if ( GuidValid( ssir.SampleItemId ) )
						{
							idNum = NO_ID_NUM;
						}

						DB_SampleItemRecord chk_ssir = {};

						if ( GetSampleItemInternal( chk_ssir, ssir.SampleItemId, idNum ) == DBApi::eQueryResult::QueryOk )
						{
							ssiFound = true;
							ssir.SampleItemIdNum = chk_ssir.SampleItemIdNum;
							ssir.SampleItemStatus = chk_ssir.SampleItemStatus;
							idNum = ssir.SampleItemIdNum;

							if ( ssir.SampleItemStatus == static_cast<int32_t>( DBApi::eSampleItemStatus::ItemComplete ) )
							{
								processedCnt++;
							}
						}
					}

					if ( !ssiFound )     // also gets here when idNum == 0
					{
						foundCnt = ListItemNotFound;
						dataOk = false;
						valStr.clear();
						queryResult = DBApi::eQueryResult::QueryFailed;
						break;          // break out of this 'for' loop
					}
					else
					{
						sampleSetItemsIdList.push_back( ssir.SampleItemId );
					}
				}

				listCnt = (int32_t) sampleSetItemsIdList.size();

				if ( dataOk )
				{
					if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, sampleSetItemsIdList ) != listCnt )
					{
						foundCnt = ItemValCreateError;
						valStr.clear();
						dataOk = false;
						queryResult = DBApi::eQueryResult::QueryFailed;
					}

					if ( foundCnt == SetItemListCnt )
					{
						ssr.SampleItemCount = static_cast<int16_t>( listCnt );
						foundCnt = tagsFound;
					}
				}

				if ( foundCnt < ListItemsOk )
				{
					tagsFound = foundCnt;
					valStr.clear();
					dataOk = false;
				}

				if ( listCnt != ssSampleItemsCnt || listCnt != ssr.SampleItemCount )
				{
					ssSampleItemsCnt = static_cast<int32_t>( listCnt );
					ssr.SampleItemCount = static_cast<int16_t>( listCnt );
				}

				if ( processedCnt != ssProcessedItemCnt || processedCnt != ssr.ProcessedItemCount )
				{
					ssProcessedItemCnt = processedCnt;
					ssr.ProcessedItemCount = ssProcessedItemCnt;
				}

				// need to do the 'num' field addition here, once it's been validated...
				if ( dataOk )
				{
					if ( numSsItemsTagIdx >= ItemTagIdxOk )		// the tag has already been handled
					{
						tagsFound++;
						cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSsItemsTagIdx ) );      // make the quoted string expected by the database for value names
						cntValStr = boost::str( boost::format( "%d" ) % ssr.SampleItemCount );
						AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
					}

					if ( processedItemCntTagIdx >= ItemTagIdxOk )		// the processed sample sets tag has already been handled
					{
						if ( ssr.ProcessedItemCount >= dbRec.ProcessedItemCount )
						{
							tagsFound++;
							cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( processedItemCntTagIdx ) );		// make the quoted string expected by the database for value names
							cntValStr = boost::str( boost::format( "%d" ) % ssr.ProcessedItemCount );
							AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
						}
					}
				}
			}
		}

		// These items may not be updated once a sampleSet has begun processing
		if ( dbRec.SampleSetStatus < static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetActive ) )
		{
			if ( tag == SS_NameTag )						// "SampleSetName"
			{
				if ( ssr.SampleSetNameStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssr.SampleSetNameStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == SS_LabelTag )					// "SampleSetLabel"
			{
				if ( ssr.SampleSetLabel.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssr.SampleSetLabel );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == SS_CommentsTag )				// "Comments"
			{
				if ( ssr.Comments.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssr.Comments );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == SS_CarrierTypeTag )			// "CarrierType"
			{
				valStr = boost::str( boost::format( "%d" ) % ssr.CarrierType );
			}
			else if ( tag == SS_OwnerIdTag )				// "OwnerID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				if ( !GuidUpdateCheck( dbRec.OwnerId, ssr.OwnerId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SS_WorklistIdTag )				// "WorklistID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				if ( !GuidUpdateCheck( dbRec.WorklistId, ssr.WorklistId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );					// make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0)
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSampleSetQueryTag( schemaName, tableName, selectTag, idStr, ssr.SampleSetId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{	// this may be removed if there are valid reasons to allow an update that does no updating...
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	if ( !GuidValid( samplesetid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord dbRec = {};

	queryResult = GetSampleSet( dbRec, samplesetid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// SampleSet status will regress, going between active, inprogress, and running;
	// don't prevent status regressions...
//	if ( dbRec.SampleSetStatus == static_cast< int32_t >( DBApi::eSampleSetStatus::SampleSetComplete ) )
//	{
//		return DBApi::eQueryResult::StatusRegressionNotAllowed;
//	}
	dbRec.SampleSetStatus = static_cast< int32_t >( status );

	queryResult = UpdateSampleSetRecord( dbRec, NO_SAMPLE_ITEM_UPDATES );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleSetItemStatus( uuid__t samplesetid, uuid__t itemid, DBApi::eSampleItemStatus status )
{
	if ( !GuidValid( samplesetid ) | !GuidValid( itemid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord ssRec = {};

	queryResult = GetSampleSet( ssRec, samplesetid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// SampleSet status will regress, going between active, inprogress, and running;
	// don't prevent status regressions...
//	if ( ssRec.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetComplete ) )
//	{
//		return DBApi::eQueryResult::StatusRegressionNotAllowed;
//	}

	bool siFound = false;
	bool updateSS = false;

	for ( auto itemIter = ssRec.SSItemsList.begin(); itemIter != ssRec.SSItemsList.end(); ++itemIter )
	{
		if ( GuidsEqual( itemid, itemIter->SampleItemId ) )
		{
			siFound = true;
		}
	}

	if ( siFound )
	{
		DB_SampleItemRecord itemRec = {};

		queryResult = GetSampleItemInternal( itemRec, itemid );
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			// previous status was not complete; new status is complete
			if ( itemRec.SampleItemStatus != static_cast<int32_t>( DBApi::eSampleItemStatus::ItemComplete ) &&
				 status == DBApi::eSampleItemStatus::ItemComplete )
			{
				updateSS = true;	// note that the 'processed' count in the worklist needs to be updated...
			}
		}

		// write the status change for the sample item; must do this prior to sample-set update, if required
		queryResult = UpdateSampleItemStatus( itemid, status );
		if ( queryResult == DBApi::eQueryResult::QueryOk && updateSS )
		{
			UpdateSampleSetRecord( ssRec );
		}
	}
	else
	{
		// matching id not found in list...
		queryResult = DBApi::eQueryResult::NoResults;
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleItemRecord( DB_SampleItemRecord& ssir, bool doParams, bool in_ext_transaction )
{
	if ( !GuidValid( ssir.SampleItemId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList;
	DB_SampleItemRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetSampleItemObj( dbRec, resultRec, tagList, ssir.SampleItemId, ssir.SampleItemIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	// TODO: do we want to prevent status from regressing?
	if ( dbRec.SampleItemStatus == static_cast<int32_t>( DBApi::eSampleItemStatus::ItemDeleted ) ||
		 dbRec.SampleItemStatus > static_cast<int32_t>( ssir.SampleItemStatus ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t tagIndex = 0;
	int32_t updateIdx = 0;
	size_t tagListSize = tagList.size();
	int32_t tagsFound = 0;
	system_TP zeroTP = {};

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == SI_StatusTag )								// "SampleItemStatus"
		{
			if ( ssir.SampleItemStatus >= dbRec.SampleItemStatus )
			{
				valStr = boost::str( boost::format( "%d" ) % ssir.SampleItemStatus );
			}
		}
		else if ( tag == SI_SampleIdTag )						// "SampleID"
		{
			int32_t errCode = 0;

			// WILL be empty or nill at the time of item creation
			// but will fill-in after processing; once a valid ID is written, don't allow updates...
			if ( !GuidUpdateCheck( dbRec.SampleId, ssir.SampleId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SI_RunDateTag )						// "RunDate"
		{	// update just prior to beginning sample processing; follow with status update
			// only update run date/time when list is first started
			if ( ssir.SampleItemStatus > dbRec.SampleItemStatus&&
				 dbRec.SampleItemStatus < static_cast<int32_t>( DBApi::eSampleItemStatus::ItemRunning ) &&
				 ssir.SampleItemStatus >= static_cast<int32_t>( DBApi::eSampleItemStatus::ItemRunning ) )
			{
				if ( ssir.RunDateTP == zeroTP )
				{
					GetDbCurrentTimeString( valStr, ssir.RunDateTP );
				}
				else
				{
					GetDbTimeString( ssir.RunDateTP, valStr );
				}
			}
			else if ( ssir.RunDateTP != zeroTP )
			{
				GetDbTimeString( ssir.RunDateTP, valStr );
			}
		}
#ifdef ALLOW_ID_FAILS
		else if ( tag == SI_ImageAnalysisParamIdTag )		// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.ImageAnalysisParamId, ssir.ImageAnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SI_AnalysisDefIdTag )				// "AnalysisDefinitionID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.AnalysisDefId, ssir.AnalysisDefId, valStr, errCode, ID_UPDATE_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SI_AnalysisDefIdxTag )				// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%u" ) % ssir.AnalysisDefIndex );
		}
		else if ( tag == SI_AnalysisParamIdTag )			// "AnalysisParameterID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.AnalysisParamId, ssir.AnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SI_CellTypeIdTag )					// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.CellTypeId, ssir.CellTypeId, valStr, errCode, ID_UPDATE_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
#endif // ALLOW_ID_FAILS

		if ( ( dbRec.SampleItemStatus == static_cast< int32_t >( DBApi::eSampleItemStatus::NoItemStatus ) ) ||
			 ( dbRec.SampleItemStatus == static_cast< int32_t >( DBApi::eSampleItemStatus::ItemNotRun ) ) )
		{
			if ( tag == SI_NameTag )							// "SampleItemName"
			{
				if ( ssir.SampleItemNameStr.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssir.SampleItemNameStr );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == SI_CommentsTag )					// "Comments"
			{
				if ( ssir.Comments.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssir.Comments );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
			else if ( tag == SI_SampleSetIdTag )				// "SampleSetID"
			{
				int32_t errCode = 0;

				// may be empty or nill at the time of item creation
				// MUST be filled-in prior to processing; once a valid ID is written, don't update
				if ( !GuidUpdateCheck( dbRec.SampleSetId, ssir.SampleSetId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_SaveImagesTag )					// "SaveImages"
			{
				valStr = boost::str( boost::format( "%d" ) % ssir.SaveNthImage );
			}
			else if ( tag == SI_WashTypeTag )					// "WashType"
			{
				valStr = boost::str( boost::format( "%d" ) % ssir.WashTypeIndex );
			}
			else if ( tag == SI_DilutionTag )					// "Dilution"
			{
				valStr = boost::str( boost::format( "%d" ) % ssir.Dilution );
			}
			else if ( tag == SI_LabelTag )						// "ItemLabel"
			{
				if ( ssir.ItemLabel.length() > 0 )
				{
					valStr = boost::str( boost::format( "'%s'" ) % ssir.ItemLabel );
					SanitizeDataString( valStr );
				}
				else
				{
					valStr = "' '";
				}
			}
#ifndef ALLOW_ID_FAILS
			else if ( tag == SI_ImageAnalysisParamIdTag )		// "ImageAnalysisParamID"
			{
				if ( ( !GuidValid( dbRec.ImageAnalysisParamId ) && GuidValid( ssir.ImageAnalysisParamId ) ) ||
					 ( !GuidsEqual( dbRec.ImageAnalysisParamId, ssir.ImageAnalysisParamId ) ) )
				{
					uuid__t_to_DB_UUID_Str( ssir.ImageAnalysisParamId, valStr );
				}
				int32_t errCode = 0;

				// SHOULD NOT be empty or nill at the time of item creation, BUT
				// MUST be filled-in prior to processing
				if ( !GuidUpdateCheck( dbRec.ImageAnalysisParamId, ssir.ImageAnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_AnalysisDefIdTag )				// "AnalysisDefinitionID"
			{
				int32_t errCode = 0;

				// SHOULD NOT be empty or nill at the time of item creation, BUT
				// MUST be filled-in prior to processing
				if ( !GuidUpdateCheck( dbRec.AnalysisDefId, ssir.AnalysisDefId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_AnalysisDefIdxTag )				// "AnalysisDefinitionIndex"
			{
				valStr = boost::str( boost::format( "%u" ) % ssir.AnalysisDefIndex );
			}
			else if ( tag == SI_AnalysisParamIdTag )			// "AnalysisParameterID"
			{
				int32_t errCode = 0;

				// SHOULD NOT be empty or nill at the time of item creation, BUT
				// MUST be filled-in prior to processing
				if ( !GuidUpdateCheck( dbRec.AnalysisParamId, ssir.AnalysisParamId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_CellTypeIdTag )					// "CellTypeID"
			{
				int32_t errCode = 0;

				// SHOULD NOT be empty or nill at the time of item creation, BUT
				// MUST be filled-in prior to processing
				if ( !GuidUpdateCheck( dbRec.CellTypeId, ssir.CellTypeId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
#endif // ALLOW_ID_FAILS
			else if ( tag == SI_CellTypeIdxTag )				// "CellTypeIndex"
			{
				valStr = boost::str( boost::format( "%ld" ) % (int32_t) ssir.CellTypeIndex );
			}
			else if ( tag == SI_BioProcessIdTag )				// "BioProcessID"
			{
				int32_t errCode = 0;

				// allowed to be blank, but handle update
				if ( !GuidUpdateCheck( dbRec.BioProcessId, ssir.BioProcessId, valStr, errCode, ID_UPDATE_ALLOWED, EMPTY_ID_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_QcProcessIdTag )				// "QcProcessID"
			{
				int32_t errCode = 0;

				// allowed to be blank, but handle update
				if ( !GuidUpdateCheck( dbRec.QcProcessId, ssir.QcProcessId, valStr, errCode, ID_UPDATE_ALLOWED, EMPTY_ID_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_WorkflowIdTag )					// "WorkflowID"
			{
				int32_t errCode = 0;

				// SHOULD NOT be empty or nill at the time of item creation, BUT
				// MUST be filled-in prior to processing
				if ( !GuidUpdateCheck( dbRec.WorkflowId, ssir.WorkflowId, valStr, errCode, ID_UPDATE_ALLOWED ) )
				{
					tagsFound = errCode;
					idFails++;
					break;
				}
			}
			else if ( tag == SI_SamplePosTag )					// "SamplePosition"
			{
				valStr = boost::str( boost::format( "'%c-%02d-%02d'" ) % ssir.SampleRow % (int) ssir.SampleCol % (int) ssir.RotationCount );            // make the position string expected by the database for position; add cast to avoid boost bug for byte values...
			}
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSampleItemQueryTag( schemaName, tableName, selectTag, idStr, ssir.SampleItemId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{	// this may be removed if there are valid reasons to allow an update that does no updating...
		if ( idFails > 0 || tagsFound < TagsOk )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status )
{
	if ( !GuidValid( itemid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleItemRecord dbRec = {};

	queryResult = GetSampleItem( dbRec, itemid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// TODO: do we want to prevent status from regressing?
	if ( dbRec.SampleItemStatus == static_cast< int32_t >( DBApi::eSampleItemStatus::ItemComplete ) ||
		 dbRec.SampleItemStatus > static_cast< int32_t >( status ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}
	dbRec.SampleItemStatus = static_cast< int32_t >( status );

	queryResult = UpdateSampleItemRecord( dbRec, NO_PARAM_UPDATES );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleRecord( DB_SampleRecord& sr )
{
	if ( !GuidValid( sr.SampleId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_SampleRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetSampleObj( dbRec, resultRec, tagList, sr.SampleId, sr.SampleIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	// TODO: do we want to prevent status update regression, particularly for reanalysis after an error where the images are OK?
//	if ( dbRec.SampleStatus == static_cast< int32_t >( DBApi::eSampleStatus::SampleComplete ) ||
//		 dbRec.SampleStatus > static_cast< int32_t >( status ) )
//	if ( dbRec.SampleStatus == static_cast<int32_t>( DBApi::eSampleStatus::SampleComplete ) )
//	{
//		return DBApi::eQueryResult::StatusRegressionNotAllowed;
//	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	system_TP zeroTP = {};
	std::vector<std::string> cleanList = {};

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		tag = *tagIter;
		valStr.clear();

		if ( tag == SM_StatusTag )								// "SampleStatus"
		{
			if ( sr.SampleStatus >= dbRec.SampleStatus )
			{
				valStr = boost::str( boost::format( "%d" ) % sr.SampleStatus );
			}
		}
		else if ( tag == SM_NameTag )							// "SampleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.SampleNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SM_CellTypeIdTag )						// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing; once a valid ID is written, don't update
			if ( !GuidUpdateCheck( dbRec.CellTypeId, sr.CellTypeId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_CellTypeIdxTag )					// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) sr.CellTypeIndex );
		}
		else if ( tag == SM_AnalysisDefIdTag )					// "AnalysisDefinitionID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.AnalysisDefId, sr.AnalysisDefId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_AnalysisDefIdxTag )					// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.AnalysisDefIndex );
		}
		else if ( tag == SM_LabelTag )							// "Label"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.Label );
			SanitizeDataString( valStr );
		}
		else if ( tag == SM_BioProcessIdTag )					// "BioProcessID"
		{
			int32_t errCode = 0;

			// allowed to be blank, but handle update; once a valid ID is written, don't allow update
			if ( !GuidUpdateCheck( dbRec.BioProcessId, sr.BioProcessId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_QcProcessIdTag )					// "QcProcessID"
		{
			int32_t errCode = 0;

			// allowed to be blank, but handle update; once a valid ID is written, don't allow update
			if ( !GuidUpdateCheck( dbRec.QcId, sr.QcId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_WorkflowIdTag )						// "WorkflowID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing; once a valid ID is written don't update
			if ( !GuidUpdateCheck( dbRec.WorkflowId, sr.WorkflowId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_CommentsTag )						// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.CommentsStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SM_WashTypeTag )						// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.WashTypeIndex );
		}
		else if ( tag == SM_DilutionTag )						// "Dilution"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.Dilution );
		}
		else if ( tag == SM_OwnerUserIdTag )					// "OwnerUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.OwnerUserId, sr.OwnerUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_RunUserTag )						// "RunUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.RunUserId, sr.RunUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_AcquireDateTag )					// "AcquisitionDate"
		{
			if ( sr.AcquisitionDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, sr.AcquisitionDateTP );
			}
			else
			{
				GetDbTimeString( sr.AcquisitionDateTP, valStr );
			}
		}
		else if ( tag == SM_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in after processing
			if ( !GuidUpdateCheck( dbRec.ImageSetId, sr.ImageSetId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_DustRefSetIdTag )					// "DustRefImageSetID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation; may not be a dust reference image...
			if ( !GuidUpdateCheck( dbRec.DustRefImageSetId, sr.DustRefImageSetId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_InstSNTag )							// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.AcquisitionInstrumentSNStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SM_ImageAnalysisParamIdTag )			// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.ImageAnalysisParamId, sr.ImageAnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == SM_NumReagentsTag )					// "NumReagents"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.NumReagents );
		}
		else if ( tag == SM_ReagentTypeNameListTag )			// "ReagentTypeNameList"
		{
			size_t listCnt = sr.ReagentTypeNameList.size();

			cleanList.clear();
			SanitizeDataStringList( sr.ReagentTypeNameList, cleanList );

			if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
			{
				valStr.clear();
			}
		}
		else if ( tag == SM_ReagentPackNumListTag )				// "ReagentPackNumList"
		{
			size_t listCnt = sr.ReagentPackNumList.size();

			cleanList.clear();
			SanitizeDataStringList( sr.ReagentPackNumList, cleanList );

			if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
			{
				valStr.clear();
			}
		}
		else if ( tag == SM_PackLotNumTag )						// "PackLotNumList"
		{
			size_t listCnt = sr.PackLotNumList.size();

			cleanList.clear();
			SanitizeDataStringList( sr.PackLotNumList, cleanList );

			if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
			{
				valStr.clear();
			}
		}
		else if ( tag == SM_PackLotExpirationListTag )			// "PackLotExpirationList"
		{
			size_t listCnt = sr.PackLotExpirationList.size();
			std::vector<int64_t> expirationList = {};
			int64_t tmpDays = 0;

			for ( int i = 0; i < listCnt; i++ )
			{
				tmpDays = static_cast<int64_t>( sr.PackLotExpirationList.at( i ) );
				expirationList.push_back( tmpDays );
			}

			if ( CreateInt64ValueArrayString( valStr, "%lld", static_cast<int32_t>( listCnt ), 0, expirationList ) != listCnt )
			{
				valStr.clear();
			}
		}
		else if ( tag == SM_PackInServiceListTag )				// "PackInServiceList"
		{
			size_t listCnt = sr.PackInServiceList.size();
			std::vector<int64_t> inServiceList = {};
			int64_t days = 0;

			for ( int i = 0; i < listCnt; i++ )
			{
				days = static_cast<int64_t>( sr.PackInServiceList.at( i ) );
				inServiceList.push_back( days );
			}

			if ( CreateInt64ValueArrayString( valStr, "%lld", static_cast<int32_t>( listCnt ), 0, inServiceList ) != listCnt )
			{
				valStr.clear();
			}
		}
		else if ( tag == SM_PackServiceExpirationListTag )		// "PackServiceExpirationList"
		{
			size_t listCnt = sr.PackServiceExpirationList.size();
			std::vector<int64_t> expirationList = {};
			int64_t days = 0;

			for ( int i = 0; i < listCnt; i++ )
			{
				days = static_cast<int64_t>( sr.PackServiceExpirationList.at( i ) );
				expirationList.push_back( days );
			}

			if ( CreateInt64ValueArrayString( valStr, "%lld", static_cast<int32_t>( listCnt ), 0, expirationList ) != listCnt )
			{
				valStr.clear();
			}
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSampleQueryTag( schemaName, tableName, selectTag, idStr, sr.SampleId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{	// this may be removed if there are valid reasons to allow an update that does no updating...
		if ( idFails > 0 || tagsFound < TagsOk )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status )
{
	if ( !GuidValid( sampleid ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleRecord dbRec = {};

	queryResult = GetSample( dbRec, sampleid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// TODO: do we want to prevent status update regression, particularly for reanalysis after an error where the images are OK?
//	if ( dbRec.SampleStatus == static_cast< int32_t >( DBApi::eSampleStatus::SampleComplete ) ||
//		 dbRec.SampleStatus > static_cast< int32_t >( status ) )
//	if ( dbRec.SampleStatus == static_cast< int32_t >( DBApi::eSampleStatus::SampleComplete ) )
//	{
//		return DBApi::eQueryResult::StatusRegressionNotAllowed;
//	}
	dbRec.SampleStatus = static_cast< int32_t >( status );

	queryResult = UpdateSampleRecord( dbRec );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateAnalysisRecord( DB_AnalysisRecord& ar )
{
	if ( !GuidValid( ar.AnalysisId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_AnalysisRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetAnalysisObj( dbRec, resultRec, tagList, ar.AnalysisId, ar.AnalysisIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t tagIndex = 0;
	int32_t updateIdx = 0;
	size_t tagListSize = tagList.size();
	int32_t tagsFound = 0;
	int32_t imgRecordCnt = NoItemListCnt;
	int32_t numImgRecordsTagIdx = ListCntIdxNotSet;
	system_TP zeroTP = {};

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == AN_SampleIdTag )							// "SampleID"
		{
			int32_t errCode = 0;

			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.SampleId, ar.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.ImageSetId, ar.ImageSetId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_SummaryResultIdTag )				// "SummaryResultID"
		{
			DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
			bool recFound = false;
			int64_t idNum = NO_ID_NUM;

			valStr.clear();

			// check to update an existing record
			if ( ar.SummaryResult.SummaryResultIdNum > INVALID_ID_NUM || GuidValid( ar.SummaryResult.SummaryResultId ) )
			{
				DB_SummaryResultRecord sr_chk = {};

				if ( GetSummaryResultInternal( sr_chk, ar.SummaryResult.SummaryResultId, ar.SummaryResult.SummaryResultIdNum ) == DBApi::eQueryResult::QueryOk )
				{
					if ( !GuidsEqual( sr_chk.SummaryResultId, ar.SummaryResult.SummaryResultId ) )		// shouldn't happen, but update the local with the DB ID
					{
						ar.SummaryResult.SummaryResultId = sr_chk.SummaryResultId;
						ar.SummaryResult.SummaryResultIdNum = sr_chk.SummaryResultIdNum;
					}

					if ( UpdateSummaryResultRecord( ar.SummaryResult, true ) != DBApi::eQueryResult::QueryOk )
					{
						tagsFound = OperationFailure;
						dataOk = false;
						valStr.clear();
						queryResult = DBApi::eQueryResult::QueryFailed;
						break;          // break out of the 'for' loop
					}
					recFound = true;
				}

				if ( recFound )
				{
					idNum = sr_chk.SummaryResultIdNum;
				}
			}

			// record not found or update failed
			if ( idNum <= INVALID_ID_NUM )
			{
				ar.SummaryResult.AnalysisId = ar.AnalysisId;
				ar.SummaryResult.SampleId = ar.SampleId;

				insertResult = InsertSummaryResultRecord( ar.SummaryResult, true );

				if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( ar.SummaryResult.SummaryResultIdNum > INVALID_ID_NUM ) )
				{
					idNum = ar.SummaryResult.SummaryResultIdNum;

					int32_t errCode = 0;

					// may be empty or nill at the time of item creation
					// MUST be filled-in prior to processing
					if ( !GuidInsertCheck( ar.SummaryResult.SummaryResultId, valStr, errCode ) )
					{
						tagsFound = errCode;
						idFails++;
						break;
					}
				}
			}

			if ( idNum > INVALID_ID_NUM )
			{
				// check for retrieval of inserted/found item by uuid__t value...
				// if uuid__t is valid, do retrieval by using it, otherwise
				// just check for retrieval by idnum
				if ( GuidValid( ar.SummaryResult.SummaryResultId ) )
				{
					idNum = NO_ID_NUM;
				}

				DB_SummaryResultRecord sr_chk = {};
				if ( GetSummaryResultInternal( sr_chk, ar.SummaryResult.SummaryResultId, idNum ) == DBApi::eQueryResult::QueryOk )
				{
					recFound = true;
					idNum = ar.SummaryResult.SummaryResultIdNum;
				}
			}

			if ( !recFound )
			{
				tagsFound = OperationFailure;
				dataOk = false;
				valStr.clear();
				queryResult = DBApi::eQueryResult::InsertFailed;
				break;          // break out of this 'for' loop
			}
		}
		else if ( tag == AN_SResultIdTag )					// "SResultID"
		{
			int32_t errCode = 0;

			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.SResultId, ar.SResultId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_RunUserIdTag )							// "RunUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.AnalysisUserId, ar.AnalysisUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_AnalysisDateTag )					// "AnalysisDate"
		{
			if ( ar.AnalysisDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, ar.AnalysisDateTP );
			}
			else
			{
				GetDbTimeString( ar.AnalysisDateTP, valStr );
			}
		}
		else if ( tag == AN_InstSNTag )							// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ar.InstrumentSNStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == AN_BioProcessIdTag )					// "BioProcessID"
		{
			int32_t errCode = 0;

			// allowed to be blank
			if ( !GuidUpdateCheck( dbRec.BioProcessId, ar.BioProcessId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_QcProcessIdTag )					// "QcProcessID"
		{
			int32_t errCode = 0;

			// allowed to be blank
			if ( !GuidUpdateCheck( dbRec.QcId, ar.QcId, valStr, errCode, ID_UPDATE_NOT_ALLOWED, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_WorkflowIdTag )						// "WorkflowID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.WorkflowId, ar.WorkflowId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == AN_ImageSequenceCntTag )				// "ImageSequenceCount"
		{
			if ( imgRecordCnt < ItemListCntOk )
			{
				numImgRecordsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % ar.ImageSequenceCount );
				imgRecordCnt = ar.ImageSequenceCount;
			}
		}
		else if ( tag == AN_ImageSequenceIdListTag )				// "ImageSequenceIDList"
		{
			DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
			std::vector<uuid__t> imageRecordIdList = {};
			bool recFound = false;
			bool recsetChanged = false;
			int64_t idNum = NO_ID_NUM;
			int32_t foundCnt = tagsFound;
			//			size_t listSize = ar.ImageSequenceIdList.size();
			size_t listSize = ar.ImageSequenceList.size();
			size_t listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( imgRecordCnt < ItemListCntOk )		// hasn't yet been validated against the object list; don't write yet
			{
				imgRecordCnt = ar.ImageSequenceCount;
			}

			if ( imgRecordCnt != listSize )
			{
				if ( imgRecordCnt >= ItemListCntOk )
				{
					foundCnt = SetItemListCnt;
				}
			}

			for ( size_t i = 0; i < listCnt; i++ )
			{
				recFound = false;
				DB_ImageSeqRecord& isr = ar.ImageSequenceList.at( i );

				if ( isr.ImageSequenceIdNum > INVALID_ID_NUM )
				{
					idNum = isr.ImageSequenceIdNum;
				}
				else
				{
					insertResult = InsertImageSeqRecord( isr, true );

					if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( isr.ImageSequenceIdNum > INVALID_ID_NUM ) )
					{
						idNum = isr.ImageSequenceIdNum;
						ar.ImageSequenceList.at( i ).ImageSequenceIdNum = idNum;    // update the id in the passed-in object
					}
				}

				if ( idNum > INVALID_ID_NUM )
				{
					// check for retrieval of inserted/found item by uuid__t value...
					// if uuid__t is valid, do retrieval by using it, otherwise
					// just check for retrieval by idnum
					if ( GuidValid( isr.ImageSequenceId ) )
					{
						idNum = NO_ID_NUM;
					}

					if ( GetImageSequenceInternal( isr, isr.ImageSequenceId, idNum ) == DBApi::eQueryResult::QueryOk )
					{
						recFound = true;
						idNum = isr.ImageSequenceIdNum;
					}
				}

				if ( !recFound )     // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					imageRecordIdList.push_back( isr.ImageSequenceId );
				}
			}

			listCnt = imageRecordIdList.size();

			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, imageRecordIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					ar.ImageSequenceCount = static_cast<int16_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			if ( listCnt != imgRecordCnt || listCnt != ar.ImageSequenceCount )
			{
				imgRecordCnt = static_cast<int32_t>( listCnt );
				ar.ImageSequenceCount = static_cast<int16_t>( listCnt );
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numImgRecordsTagIdx >= ItemTagIdxOk )		// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numImgRecordsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetAnalysisQueryTag( schemaName, tableName, selectTag, idStr, ar.AnalysisId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{	// this may be removed if there are valid reasons to allow an update that does no updating...
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

// Summary record is the one data record that requires an update operatin;  Signing the data will cause an update to the record;
DBApi::eQueryResult DBifImpl::UpdateSummaryResultRecord( DB_SummaryResultRecord& sr, bool in_ext_transaction )
{
	if ( !GuidValid( sr.SummaryResultId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_SummaryResultRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetSummaryResultObj( dbRec, resultRec, tagList, sr.SummaryResultId, sr.SummaryResultIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == RS_SampleIdTag )							// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.SampleId, sr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.ImageSetId, sr.ImageSetId, valStr, errCode ) )		// image decimation may result in only a small sub-set of images being saved, but there should ALWAYS be an imageset ID
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_AnalysisIdTag )						// "AnalysisID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.AnalysisId, sr.AnalysisId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_ResultDateTag )						// "ResultDate"
		{
			system_TP zeroTP = {};

			if ( sr.ResultDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, sr.ResultDateTP );
			}
			else
			{
				GetDbTimeString( sr.ResultDateTP, valStr );
			}
		}
		else if ( tag == RS_SigListTag )						// "SignatureList"
		{
			size_t listSize = sr.SignatureList.size();

			if ( CreateSignatureArrayString( valStr, static_cast<int32_t>( listSize ), sr.SignatureList, true ) != listSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == RS_ImgAnalysisParamIdTag )				// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.ImageAnalysisParamId, sr.ImageAnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_AnalysisDefIdTag )					// "AnalysisDefID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.AnalysisDefId, sr.AnalysisDefId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_AnalysisParamIdTag )				//  "AnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.AnalysisParamId, sr.AnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_CellTypeIdTag )						// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidUpdateCheck( dbRec.CellTypeId, sr.CellTypeId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == RS_CellTypeIdxTag )					// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t)sr.CellTypeIndex );
		}
		else if ( tag == RS_StatusTag )							// "ProcessingStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.ProcessingStatus );
		}
		else if ( tag == RS_TotCumulativeImgsTag )				// "TotalCumulativeImages"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCumulativeImages );
		}
		else if ( tag == RS_TotCellsGPTag )						// "TotalCellsGP"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCells_GP );
		}
		else if ( tag == RS_TotCellsPOITag )					// "TotalCellsPOI"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCells_POI );
		}
		else if ( tag == RS_POIPopPercentTag )					// "POIPopulationPercent"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.POI_PopPercent );
		}
		else if ( tag == RS_CellConcGPTag )						// "CellConcGP"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CellConc_GP );
		}
		else if ( tag == RS_CellConcPOITag )					// "CellConcPOI"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CellConc_POI );
		}
		else if ( tag == RS_AvgDiamGPTag )						// "AvgDiamGP"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgDiam_GP );
		}
		else if ( tag == RS_AvgDiamPOITag )						// "AvgDiamPOI"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgDiam_POI );
		}
		else if ( tag == RS_AvgCircularityGPTag )				// "AvgCircularityGP"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgCircularity_GP );
		}
		else if ( tag == RS_AvgCircularityPOITag )				// "AvgCircularityPOI"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgCircularity_POI );
		}
		else if ( tag == RS_CoeffOfVarTag )						// "CoefficientOfVariance"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CoefficientOfVariance );
		}
		else if ( tag == RS_AvgCellsPerImgTag )					// "AvgCellsPerImage"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.AvgCellsPerImage );
		}
		else if ( tag == RS_AvgBkgndIntensityTag )				// "AvgBkgndIntensity"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.AvgBkgndIntensity );
		}
		else if ( tag == RS_TotBubbleCntTag )					// "TotalBubbleCount"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.TotalBubbleCount );
		}
		else if ( tag == RS_LrgClusterCntTag )					// "LargeClusterCount"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.LargeClusterCount );
		}
		else if (tag == RS_QcStatusTag)							// "QcStatus"
		{
			valStr = boost::str(boost::format("%u") % sr.QcStatus);
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );     // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSummaryResultQueryTag( schemaName, tableName, selectTag, idStr, sr.SummaryResultId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateImageSetRecord( DB_ImageSetRecord& isr )
{
	if ( !GuidValid( isr.ImageSetId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageSetRecord dbRec = {};
	std::vector<std::string> tagList = {};
	CRecordset resultRec( pDb );

	queryResult = GetImageSetObj( dbRec, resultRec, tagList, isr.ImageSetId, isr.ImageSetIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	size_t tagCnt = 0;
	uint32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t imageRecCount = NoItemListCnt;
	int32_t imageRecCountTagIdx = ListCntIdxNotSet;

	queryResult = DBApi::eQueryResult::QueryFailed;

	tagCnt = tagList.size();

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == IC_SampleIdTag )						// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.SampleId, isr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == IC_SetFolderTag )					// "ImageSetFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % isr.ImageSetPathStr );
			SanitizePathString( valStr );
		}
		else if ( tag == IC_SequenceCntTag )					// "ImageSequenceCount"
		{
			if ( imageRecCount < ItemListCntOk )			// hasn't yet been validated against the object list; don't write
			{
				imageRecCountTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % isr.ImageSequenceCount );
				imageRecCount = isr.ImageSequenceCount;
			}
		}
		else if ( tag == IC_SequenceIdListTag )				// "ImageSequenceIdList"
		{
			DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
			std::vector<uuid__t> imageRecIdList = {};
			int32_t indexVal = 0;
			size_t listSize = isr.ImageSequenceList.size();
			size_t listCnt = listSize;
			bool imgRecFound = false;
			int64_t idNum = NO_ID_NUM;

			valStr.clear();
			foundCnt = tagsFound;

			// check against the number of objects indicated, not from the vector...
			if ( imageRecCount <= ItemListCntOk )
			{
				imageRecCount = isr.ImageSequenceCount;
			}

			if ( imageRecCount != listSize )
			{
				if ( imageRecCount >= ItemListCntOk )
				{
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				imgRecFound = false;
				DB_ImageSeqRecord& seqr = isr.ImageSequenceList.at( i );

				if (seqr.ImageSequenceIdNum > INVALID_ID_NUM)
				{
					idNum = seqr.ImageSequenceIdNum;
				}
				else
				{
					insertResult = InsertImageSeqRecord( seqr, true );

					if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( seqr.ImageSequenceIdNum > INVALID_ID_NUM ) )
					{
						idNum = seqr.ImageSequenceIdNum;
						isr.ImageSequenceList.at( i ).ImageSequenceIdNum = seqr.ImageSequenceIdNum;		// update the id in the passed-in object
					}
				}

				if ( idNum > INVALID_ID_NUM )
				{
					// check for retrieval of inserted/found item by uuid__t value...
					// if uuid__t is valid, do retrieval by using it, otherwise
					// just check for retrieval by idnum
					if ( GuidValid( seqr.ImageSequenceId ) )
					{
						idNum = NO_ID_NUM;
					}

					if ( GetImageSequenceInternal( seqr, seqr.ImageSequenceId, idNum ) == DBApi::eQueryResult::QueryOk )
					{
						imgRecFound = true;
						idNum = seqr.ImageSequenceIdNum;
					}
				}

				if ( !imgRecFound )
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					imageRecIdList.push_back( seqr.ImageSequenceId );
				}
			}

			listCnt = imageRecIdList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, imageRecIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					isr.ImageSequenceCount = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( listCnt != imageRecCount || listCnt != isr.ImageSequenceCount )
				{
					imageRecCount = static_cast<int32_t>( listCnt );
					isr.ImageSequenceCount = static_cast<int16_t>( listCnt );
				}

				if ( imageRecCountTagIdx >= ItemTagIdxOk )			// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( imageRecCountTagIdx ) );   // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetImageSetQueryTag( schemaName, tableName, selectTag, idStr, isr.ImageSetId, isr.ImageSetIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateImageSeqRecord( DB_ImageSeqRecord& isr, bool in_ext_transaction )
{
	if ( !GuidValid( isr.ImageSequenceId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageSeqRecord dbRec = {};
	std::vector<std::string> tagList = {};
	CRecordset resultRec( pDb );

	queryResult = GetImageSequenceObj( dbRec, resultRec, tagList, isr.ImageSequenceId, isr.ImageSequenceIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagCnt = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t imageCount = -1;
	int32_t imageCountTagIdx = -1;
	int32_t flChanCount = -1;
	int32_t flChanCountTagIdx = -1;

	queryResult = DBApi::eQueryResult::QueryFailed;

	tagCnt = (int32_t)tagList.size();

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == IS_SetIdTag )							// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.ImageSetId, isr.ImageSetId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == IS_SequenceNumTag )				// "ImageSequenceSeqNum"
		{
			valStr = boost::str( boost::format( "%d" ) % isr.SequenceNum );
		}
		else if ( tag == IS_ImageCntTag )					// "ImageCount"
		{
			if ( imageCount < 0 )           // hasn't yet been validated against the object list; don't write
			{
				imageCountTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % (int16_t) isr.ImageCount );		// cast to avoid boost bug for byte values...
				imageCount = isr.ImageCount;
			}
		}
		else if ( tag == IS_FlChansTag )					// "FlChannels"
		{
			if ( flChanCount < 0 )          // hasn't yet been validated against the object list; don't write
			{
				flChanCountTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) isr.FlChannels );		// cast to avoid boost bug for byte values...
				flChanCount = isr.FlChannels;
			}
		}
		else if ( tag == IS_ImageIdListTag )				// "ImageIdList"
		{
			DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
			std::vector<uuid__t> imageIdList = {};
			size_t idListSize = isr.ImageList.size();
			size_t listCnt = idListSize;
			bool imgFound = false;
			int32_t flChans = 0;
			int64_t idNum = NO_ID_NUM;

			valStr.clear();
			foundCnt = tagsFound;

			// check against the number of objects indicated, not from the vector...
			if ( imageCount < ItemListCntOk )
			{
				imageCount = isr.ImageCount;
			}

			if ( flChanCount < 0 )
			{
				flChanCount = isr.FlChannels;
			}

			if ( imageCount != idListSize )
			{
				if ( imageCount >= ItemListCntOk )
				{
					foundCnt = SetItemListCnt;
				}
			}

			// one image per FL channel, so a record may have up to 4 channel images...
			for ( size_t i = 0; i < listCnt; i++ )
			{
				DB_ImageRecord& ir = isr.ImageList.at( i );
				imgFound = false;

				if ( ir.ImageIdNum > INVALID_ID_NUM )
				{
					idNum = ir.ImageIdNum;
				}
				else
				{
					insertResult = InsertImageRecord( ir, true );

					if ( ( insertResult == DBApi::eQueryResult::QueryOk ) && ( isr.ImageSequenceIdNum > INVALID_ID_NUM ) )
					{
						idNum = ir.ImageIdNum;
						isr.ImageList.at( i ).ImageIdNum = idNum;		// update the id in the passed-in object
					}
				}

				if ( idNum > INVALID_ID_NUM )
				{
					// check for retrieval of inserted/found item by uuid__t value...
					// if uuid__t is valid, do retrieval by using it, otherwise
					// just check for retrieval by idnum
					if ( GuidValid( ir.ImageId ) )
					{
						idNum = NO_ID_NUM;
					}

					DB_ImageRecord chk_ir = {};

					if ( GetImageInternal( ir, ir.ImageId, idNum ) == DBApi::eQueryResult::QueryOk )
					{
						imgFound = true;
						idNum = ir.ImageIdNum;
						flChans++;
					}
				}

				if ( !imgFound )
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					imageIdList.push_back( ir.ImageId );
				}
			}

			listCnt = imageIdList.size();

			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, imageIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					isr.ImageCount = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( listCnt != imageCount || listCnt != isr.ImageCount )
				{
					imageCount = static_cast<int32_t>( listCnt );
					isr.ImageCount = static_cast<int8_t>( listCnt );
				}

				// flChans SHOULD equal listCnt...
				if ( flChans != flChanCount || flChans != isr.FlChannels )
				{
					flChanCount = flChans;
					isr.FlChannels = flChans;
				}

				if ( imageCountTagIdx >= ItemTagIdxOk )         // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( imageCountTagIdx ) );   // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}

				if ( flChanCountTagIdx >= ItemTagIdxOk )         // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( flChanCountTagIdx ) );  // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % ( flChans - 1 ) );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == IS_SequenceFolderTag )				// "ImageSequenceFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % isr.ImageSequenceFolderStr );
			SanitizePathString( valStr );
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                     // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0  )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetImageSequenceQueryTag( schemaName, tableName, selectTag, idStr, isr.ImageSequenceId, isr.ImageSequenceIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateImageRecord( DB_ImageRecord& ir, bool in_ext_transaction )
{
	if ( !GuidValid( ir.ImageId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ImageRecord dbRec = {};
	std::vector<std::string> tagList = {};
	CRecordset resultRec( pDb );

	queryResult = GetImageObj( dbRec, resultRec, tagList, ir.ImageId, ir.ImageIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
//		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == IM_SequenceIdTag )					// "ImageSequenceID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.ImageId, ir.ImageId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == IM_ImageChanTag )				// "ImageChannel"
		{
			valStr = boost::str( boost::format( "%d" ) % (int16_t) ir.ImageChannel );		// cast to avoid boost bug for byte values...
		}
		else if ( tag == IM_FileNameTag )				// "ImageFileName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ir.ImageFileNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetImageQueryTag( schemaName, tableName, selectTag, idStr, ir.ImageId, ir.ImageIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( idFails > 0 || tagsFound < TagsOk )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateCellTypeRecord( DB_CellTypeRecord& ctr )
{
	if ( !GuidValid( ctr.CellTypeId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_CellTypeRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetCellTypeObj( dbRec, resultRec, tagList, ctr.CellTypeId, INVALID_INDEX );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	std::string tmpStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagIndex = 0;
	int32_t foundCnt = 0;
	int32_t identParamCnt = -1;
	int32_t numCellIdentParamsTagIdx = -1;
	int32_t specializationCnt = -1;
	int32_t numSpecializationsTagIdx = -1;
	size_t listCnt = 0;
	size_t listSize = tagList.size();
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( tagIndex = 0; tagIndex < listSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == CT_IdxTag )										// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t)ctr.CellTypeIndex );		// must be written as signed to fit in 32-bit value...
		}
		else if ( tag == CT_NameTag )								// "CellTypeName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ctr.CellTypeNameStr );
			SanitizeDataString( valStr );
		}
		else if (tag == CT_RetiredTag)								// "Retired"
		{
			valStr = (ctr.Retired == true) ? TrueStr : FalseStr;
		}
		else if ( tag == CT_MaxImagesTag )							// "MaxImages"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.MaxImageCount );
		}
		else if ( tag == CT_AspirationCyclesTag )					// "AspirationCycles"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.AspirationCycles );
		}
		else if ( tag == CT_MinDiamMicronsTag )						// "MinDiamMicrons"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MinDiam );
		}
		else if ( tag == CT_MaxDiamMicronsTag )						// "MaxDiamMicrons"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MaxDiam );
		}
		else if ( tag == CT_MinCircularityTag )						// "MinCircularity"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MinCircularity );
		}
		else if ( tag == CT_SharpnessLimitTag )						// "SharpnessLimit"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.SharpnessLimit );
		}
		else if ( tag == CT_NumCellIdentParamsTag )					// "NumCellIdentParams"
		{
			if ( identParamCnt <= 0 )   // hasn't yet been validated against the object list; don't write
			{
				numCellIdentParamsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) identParamCnt );		// cast to avoid boost bug for byte values...
				identParamCnt = ctr.NumCellIdentParams;
			}
		}
		else if ( tag == CT_CellIdentParamIdListTag )				//  "CellIdentParamIDList"
		{
			std::vector<uuid__t> identParamIdList = {};
			bool apFound = false;

			foundCnt = tagsFound;
			listCnt = ctr.CellIdentParamList.size();

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( identParamCnt <= 0 )
			{
				identParamCnt = ctr.NumCellIdentParams;
			}

			if ( identParamCnt != listCnt )
			{
				if ( identParamCnt >= 0 )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( size_t i = 0; i < listCnt; i++ )
			{
				apFound = false;
				DB_AnalysisParamRecord& ap = ctr.CellIdentParamList.at( i );

				// check if this is an existing object
				if ( GuidValid( ap.ParamId ) )
				{
					DB_AnalysisParamRecord chk_ap = {};

					if ( GetAnalysisParamInternal( chk_ap, ap.ParamId ) == DBApi::eQueryResult::QueryOk )
					{
						if ( UpdateAnalysisParamRecord( ap, true ) == DBApi::eQueryResult::QueryOk )
						{
							apFound = true;
						}
						else
						{
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !apFound && dataOk )
				{
					ap.ParamIdNum = 0;
					ClearGuid( ap.ParamId );

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertAnalysisParamRecord( ap, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						apFound = true;
						ctr.CellIdentParamList.at( i ).ParamIdNum = ap.ParamIdNum;		// update the id in the passed-in object
						ctr.CellIdentParamList.at( i ).ParamId = ap.ParamId;			// update the id in the passed-in object
					}
				}

				if ( !apFound )     // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					identParamIdList.push_back( ap.ParamId );
				}
			}

			listCnt = identParamIdList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, identParamIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					ctr.NumCellIdentParams = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numCellIdentParamsTagIdx >= ItemTagIdxOk )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numCellIdentParamsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( identParamCnt != listCnt )
			{
				identParamCnt = static_cast<int32_t>( listCnt );
				ctr.NumCellIdentParams = static_cast<int8_t>( listCnt );
			}
		}
		else if ( tag == CT_DeclusterSettingsTag )					// "DeclusterSetting"
		{
			valStr = boost::str( boost::format( "%d" ) % ctr.DeclusterSetting );
		}
//		else if ( tag == CT_POIIdentParamTag )						// "POIIdentParams"
//		{
//		// TODO: determine what this will be and the proper container...
//		}
		else if ( tag == CT_RoiExtentTag )							// "RoiExtent"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.RoiExtent );
		}
		else if ( tag == CT_RoiXPixelsTag )							// "RoiXPixels"
		{
		valStr = boost::str( boost::format( "%u" ) % ctr.RoiXPixels );
		}
		else if ( tag == CT_RoiYPixelsTag )							// "RoiYPixels"
		{
		valStr = boost::str( boost::format( "%u" ) % ctr.RoiYPixels );
		}
		else if ( tag == CT_NumAnalysisSpecializationsTag )			// "NumAnalysisSpecializations"
		{
			if ( specializationCnt <= 0 )   // hasn't yet been validated against the object list; don't write
			{
				specializationCnt = ctr.NumAnalysisSpecializations;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % ctr.NumAnalysisSpecializations );
			}
			numSpecializationsTagIdx = tagIndex;
		}
		else if ( tag == CT_AnalysisSpecializationIdListTag )		// "AnalysisSpecializationsIDList"
		{
			std::vector<uuid__t> specializationsDefIdList = {};
			bool adFound = false;

			foundCnt = tagsFound;
			listCnt = ctr.SpecializationsDefList.size();

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( specializationCnt <= 0 )
			{
				specializationCnt = ctr.NumAnalysisSpecializations;
			}

			if ( specializationCnt != listCnt)
			{
				if ( specializationCnt >= 0 )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( int i = 0; i < listCnt; i++ )
			{
				adFound = false;
				DB_AnalysisDefinitionRecord& ad = ctr.SpecializationsDefList.at( i );

				// check if this is an existing object
				if ( GuidValid( ad.AnalysisDefId ) )
				{
					DB_AnalysisDefinitionRecord chk_ad = {};

					if ( GetAnalysisDefinitionInternal( chk_ad, ad.AnalysisDefId ) == DBApi::eQueryResult::QueryOk )
					{
						if ( UpdateAnalysisDefinitionRecord( ad, true ) == DBApi::eQueryResult::QueryOk )
						{
							adFound = true;
						}
						else
						{
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !adFound && dataOk )
				{
					ad.AnalysisDefIdNum = 0;
					ClearGuid( ad.AnalysisDefId );

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertAnalysisDefinitionRecord( ad, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						adFound = true;
						ctr.SpecializationsDefList.at( i ).AnalysisDefIdNum = ad.AnalysisDefIdNum;		// update the id in the passed-in object
						ctr.SpecializationsDefList.at( i ).AnalysisDefId = ad.AnalysisDefId;			// update the id in the passed-in object
					}
				}

				if ( !adFound )
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					specializationsDefIdList.push_back( ad.AnalysisDefId );
				}
			}

			listCnt = specializationsDefIdList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, specializationsDefIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					ctr.NumAnalysisSpecializations = static_cast<int16_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < 0 )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numSpecializationsTagIdx >= ItemTagIdxOk )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSpecializationsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( specializationCnt != listCnt )
			{
				specializationCnt = static_cast<int32_t>( listCnt );
				ctr.NumAnalysisSpecializations = static_cast<int32_t>( listCnt );
			}
		}
		else if ( tag == CT_CalcCorrectionFactorTag )				// "CalculationAdjustmentFactor"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.CalculationAdjustmentFactor );
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		// get the schema name, table name, and key identifier tags
		GetCellTypeQueryTag( schemaName, tableName, selectTag, idStr, ctr.CellTypeId, ctr.CellTypeIndex );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& params )
{
	if ( !GuidValid( params.ParamId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList;
	DB_ImageAnalysisParamRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetImageAnalysisParamObj( dbRec, resultRec, tagList, params.ParamId, INVALID_INDEX );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == IAP_AlgorithmModeTag )											// "AlgorithmMode"
		{
			valStr = boost::str( boost::format( "%d" ) % params.AlgorithmMode );
		}
		else if ( tag == IAP_BubbleModeTag )										// "BubbleMode"
		{
			valStr = ( params.BubbleMode == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == IAP_DeclusterModeTag )										// "DeclusterMode"
		{
			valStr = ( params.DeclusterMode == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == IAP_SubPeakAnalysisModeTag )								// "SubPeakAnalysisMode"
		{
			valStr = ( params.SubPeakAnalysisMode == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == IAP_DilutionFactorTag )									// "DilutionFactor"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DilutionFactor );
		}
		else if ( tag == IAP_ROIXcoordsTag )										// "ROIXcoords"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ROI_Xcoords );
		}
		else if ( tag == IAP_ROIYcoordsTag )										// "ROIYcoords"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ROI_Ycoords );
		}
		else if ( tag == IAP_DeclusterAccumuLowTag )								// "DeclusterAccumulatorThreshLow"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterAccumulatorThreshLow ) );
		}
		else if ( tag == IAP_DeclusterMinDistanceLowTag )							// "DeclusterMinDistanceThreshLow"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterMinDistanceThreshLow ) );
		}
		else if ( tag == IAP_DeclusterAccumuMedTag )								// "DeclusterAccumulatorThreshMed"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterAccumulatorThreshMed ) );
		}
		else if ( tag == IAP_DeclusterMinDistanceMedTag )							// "DeclusterMinDistanceThreshMed"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterMinDistanceThreshMed ) );
		}
		else if ( tag == IAP_DeclusterAccumuHighTag )								// "DeclusterAccumulatorThreshHigh"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterAccumulatorThreshHigh ) );
		}
		else if ( tag == IAP_DeclusterMinDistanceHighTag )							// "DeclusterMinDistanceThreshHigh"
		{
			valStr = boost::str( boost::format( "%d" ) % static_cast<int32_t>( params.DeclusterMinDistanceThreshHigh ) );
		}
		else if ( tag == IAP_FovDepthTag )											// "FovDepthMM"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FovDepthMM );
		}
		else if ( tag == IAP_PixelFovTag )											// "PixelFovMM"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.PixelFovMM );
		}
		else if ( tag == IAP_SizingSlopeTag )										// "SizingSlope"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SizingSlope );
		}
		else if ( tag == IAP_SizingInterceptTag )									// "SizingIntercept"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SizingIntercept );
		}
		else if ( tag == IAP_ConcSlopeTag )											// "ConcSlope"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ConcSlope );
		}
		else if ( tag == IAP_ConcInterceptTag )										// "ConcIntercept"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ConcIntercept );
		}
		else if ( tag == IAP_ConcImageControlCntTag )								// "ConcImageControlCnt"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ConcImageControlCount );
		}
		else if ( tag == IAP_BubbleMinSpotAreaPrcntTag )							// "BubbleMinSpotAreaPrcnt"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleMinSpotAreaPercent );
		}
		else if ( tag == IAP_BubbleMinSpotAreaBrightnessTag )						// "BubbleMinSpotAreaBrightness"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleMinSpotAvgBrightness );
		}
		else if ( tag == IAP_BubbleRejectImgAreaPrcntTag )							// "BubbleRejectImgAreaPrcnt"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleRejectImageAreaPercent );
		}
		else if ( tag == IAP_VisibleCellSpotAreaTag )								// "VisibleCellSpotArea"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.VisibleCellSpotArea );
		}
		else if ( tag == IAP_FlScalableROITag )										// "FlScalableROI"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FlScalableROI );
		}
		else if ( tag == IAP_FLPeakPercentTag )										// "FLPeakPercent"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FlPeakPercent );
		}
		else if ( tag == IAP_NominalBkgdLevelTag )									// "NominalBkgdLevel"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.NominalBkgdLevel );
		}
		else if ( tag == IAP_BkgdIntensityToleranceTag )							// "BkgdIntensityTolerance"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.BkgdIntensityTolerance );
		}
		else if ( tag == IAP_CenterSpotMinIntensityTag )							// "CenterSpotMinIntensityLimit"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.CenterSpotMinIntensityLimit );
		}
		else if ( tag == IAP_PeakIntensitySelectionAreaTag )						// "PeakIntensitySelectionAreaLimit"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.PeakIntensitySelectionAreaLimit );
		}
		else if ( tag == IAP_CellSpotBrightnessExclusionTag )						// "CellSpotBrightnessExclusionThreshold"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.CellSpotBrightnessExclusionThreshold );
		}
		else if ( tag == IAP_HotPixelEliminationModeTag )							// "HotPixelEliminationMode"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.HotPixelEliminationMode );
		}
		else if ( tag == IAP_ImgBotAndRightBoundaryModeTag )						// "ImgBotAndRightBoundaryAnnotationMode"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ImgBottomAndRightBoundaryAnnotationMode );
		}
		else if ( tag == IAP_SmallParticleSizeCorrectionTag )						// "SmallParticleSizingCorrection"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SmallParticleSizingCorrection );
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		// get the schema name, table name, and key identifier tags
		GetImageAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, params.ParamId, params.ParamIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound < TagsOk )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& params )
{
	if ( !GuidValid( params.SettingsId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& def, bool in_ext_transaction )
{
	if ( !GuidValid( def.AnalysisDefId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_AnalysisDefinitionRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetAnalysisDefinitionObj( dbRec, resultRec, tagList, def.AnalysisDefId, def.AnalysisDefIndex, def.AnalysisDefIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	std::string tmpStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagIndex = 0;
	int32_t foundCnt = 0;
	int32_t numReagents = -1;
	int32_t numReagentsTagIndex = -1;
	int32_t numIlluminators = -1;
	int32_t numIlluminatorsTagIndex = -1;
	int32_t numAnalysisParams = 0;
	int32_t numAnalysisParamsTagIndex = 0;
	int32_t numOptionalParams = 0;
	int32_t numOptionalParamsTagIndex = 0;
	int32_t itemCnt = 0;
	int32_t tokenCnt = 0;
	size_t listCnt = 0;
	size_t listSize = tagList.size();
	int64_t idNum = 0;
	int64_t populationParamsId = 0;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::NotConnected;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( tagIndex = 0; tagIndex < listSize; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == AD_IdxTag )								// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%u" ) % def.AnalysisDefIndex );
		}
		else if ( tag == AD_NameTag )						// "AnalysisDefinitionName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % def.AnalysisLabel );
			SanitizeDataString( valStr );
		}
		else if ( tag == AD_NumReagentsTag )				// "NumReagents"
		{
			if ( numReagents <= 0 )     // hasn't yet been validated against the object list; don't write
			{
				numReagents = def.NumReagents;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % (int16_t) def.NumReagents );		// cast to avoid boost bug for byte values...
			}
			numReagentsTagIndex = tagIndex;
		}
		else if ( tag == AD_ReagentTypeIdxListTag )			// "ReagentTypeIndexList"
		{
			std::vector<int32_t> reagentIndexList = {};
			int32_t indexVal = 0;

			listCnt = def.ReagentIndexList.size();

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( numReagents <= ItemListCntOk )
			{
				numReagents = def.NumReagents;
			}

			if ( numReagents != listCnt )
			{
				if ( numReagents >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				indexVal = def.ReagentIndexList.at( i );
				reagentIndexList.push_back( indexVal );
			}

			listCnt = reagentIndexList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateInt32ValueArrayString( valStr, "%ld", static_cast<int32_t>( listCnt ), 0, reagentIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					def.NumReagents = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numReagentsTagIndex > 0 )      // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numReagentsTagIndex ) );    // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( numReagents != listCnt )
			{
				numReagents = static_cast<int32_t>( listCnt );
				def.NumReagents = static_cast<int8_t>( listCnt );
			}
		}
		else if ( tag == AD_MixingCyclesTag )				// "MixingCycles"
		{
			valStr = boost::str( boost::format( "%d" ) % def.MixingCycles );
		}
		else if ( tag == AD_NumIlluminatorsTag )			// "NumIlluminators"
		{
			if ( numIlluminators <= 0 )         // hasn't yet been validated against the object list; don't write
			{
				numIlluminators = def.NumFlIlluminators;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) numIlluminators );		// cast to avoid boost bug for byte values...
			}
			numIlluminatorsTagIndex = tagIndex;
		}
		else if ( tag == AD_IlluminatorIdxListTag )			// "IlluminatorsIndexList"
		{
			std::vector<int16_t> ilIndexList = {};
			int16_t idx = -1;
			std::string ilname = "";
			bool ilrFound = false;

			foundCnt = tagsFound;
			listCnt = def.IlluminatorsList.size();

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( numIlluminators <= ItemListCntOk )
			{
				numIlluminators = def.NumFlIlluminators;
			}

			if ( numIlluminators != listCnt )
			{
				if ( numIlluminators >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				ilrFound = false;
				DB_IlluminatorRecord& ilr = def.IlluminatorsList.at( i );

				// check if this is an existing object
				if ( ilr.IlluminatorIdNum != 0 )       // having an id number indicates it's already in the db
				{
					DB_IlluminatorRecord chk_ilr = {};

					if ( GetIlluminatorInternal( chk_ilr, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, ilr.IlluminatorIdNum ) != DBApi::eQueryResult::QueryOk )
					{
						if ( GetIlluminatorInternal( chk_ilr, ilr.EmissionWavelength, ilr.IlluminatorWavelength ) == DBApi::eQueryResult::QueryOk )
						{
							ilrFound = true;
						}
					}
					else
					{
						ilrFound = true;
					}

					if ( ilrFound )
					{
						if ( UpdateIlluminatorRecord( ilr, true ) != DBApi::eQueryResult::QueryOk )
						{
							ilrFound = false;
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !ilrFound && dataOk )
				{
					ilr.IlluminatorIdNum = 0;
					insertResult = InsertIlluminatorRecord( ilr, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						// retrieval for check-after-insertion is done in the individual insertion method; no need to repeat here...
						ilrFound = true;
						idx = ilr.IlluminatorIndex;
						idNum = ilr.IlluminatorIdNum;
						def.IlluminatorsList.at( i ).IlluminatorIndex = idx;        // update the id in the passed-in object
						def.IlluminatorsList.at( i ).IlluminatorIdNum = idNum;      // update the id in the passed-in object
					}
				}

				if ( !ilrFound )        // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					ilIndexList.push_back( idx );
				}
			}

			listCnt = ilIndexList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateInt16ValueArrayString( valStr, "%d", static_cast<int32_t>( listCnt ), 0, ilIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					def.NumFlIlluminators = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numIlluminatorsTagIndex > 0 )       // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numIlluminatorsTagIndex ) );        // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( numIlluminators != listCnt )
			{
				numIlluminators = static_cast<int32_t>( listCnt );
				def.NumFlIlluminators = static_cast<int8_t>( listCnt );
			}
		}
		else if ( tag == AD_NumAnalysisParamsTag )			// "NumAnalysisParams"
		{
			if ( numAnalysisParams <= 0 )   // hasn't yet been validated against the object list; don't write
			{
				numAnalysisParams = def.NumAnalysisParams;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) numAnalysisParams );		// cast to avoid boost bug for byte values...
			}
			numAnalysisParamsTagIndex = tagIndex;
		}
		else if ( tag == AD_AnalysisParamIdListTag )		// "AnalysisParamIDList"
		{
			std::vector<uuid__t> analysisParamIdList = {};
			bool apFound = false;

			foundCnt = tagsFound;
			listCnt = def.AnalysisParamList.size();

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( numAnalysisParams <= ItemListCntOk )
			{
				numAnalysisParams = def.NumAnalysisParams;
			}

			if ( numAnalysisParams != listCnt)
			{
				if ( numAnalysisParams >= ItemListCntOk )
				{
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				apFound = false;
				DB_AnalysisParamRecord& ap = def.AnalysisParamList.at( i );

				if ( GuidValid( ap.ParamId ) )
				{
					DB_AnalysisParamRecord chk_ap = {};

					if ( GetAnalysisParamInternal( chk_ap, ap.ParamId ) == DBApi::eQueryResult::QueryOk )
					{
						chk_ap.ParamId = ap.ParamId;		// update with this sample-set ID
						if ( UpdateAnalysisParamRecord( ap, true ) == DBApi::eQueryResult::QueryOk )
						{
							apFound = true;
						}
						else
						{
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !apFound && dataOk )
				{
					insertResult = InsertAnalysisParamRecord( ap, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						def.AnalysisParamList.at( i ).ParamIdNum = ap.ParamIdNum;      // update the id in the passed-in object
						def.AnalysisParamList.at( i ).ParamId = ap.ParamId;			// update the id in the passed-in object
					}
				}

				if ( !apFound )     // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					analysisParamIdList.push_back( ap.ParamId );
				}
			}

			listCnt = analysisParamIdList.size();

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, analysisParamIdList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					def.NumAnalysisParams = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numAnalysisParamsTagIndex > 0 )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numAnalysisParamsTagIndex ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( numAnalysisParams != listCnt )
			{
				numAnalysisParams = static_cast<int32_t>( listCnt );
				def.NumAnalysisParams = static_cast<int8_t>( listCnt );
			}
		}
		else if ( tag == AD_PopulationParamIdTag )			// "PopulationParamID"
		{
			if ( def.PopulationParamExists )
			{
				DB_AnalysisParamRecord& pp = def.PopulationParam;
				bool ppFound = false;

				// checke if this is an existing object
				if ( GuidValid( pp.ParamId ) )
				{
					DB_AnalysisParamRecord chk_pp = {};

					if ( GetAnalysisParamInternal( chk_pp, pp.ParamId ) == DBApi::eQueryResult::QueryOk )
					{
						chk_pp.ParamId = pp.ParamId;		// update with this sample-set ID
						if ( UpdateAnalysisParamRecord( pp, true ) == DBApi::eQueryResult::QueryOk )
						{
							ppFound = true;
						}
						else
						{
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !ppFound && dataOk )
				{
					pp.ParamIdNum = 0;
					ClearGuid( pp.ParamId );


					insertResult = InsertAnalysisParamRecord( pp, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						// retrieval for check-after-insertion is done in the individual insertion method; no need to repeat here...
						ppFound = true;
						def.PopulationParam.ParamId = pp.ParamId;				// update the idnum in the passed-in object
						def.PopulationParam.ParamIdNum = pp.ParamIdNum;		// update the id in the passed-in object
					}
				}

				if ( !ppFound )     // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					int32_t errCode = 0;

					// may be empty or nill at the time of item creation
					// MUST be filled-in prior to processing
					if ( !GuidInsertCheck( def.PopulationParam.ParamId, valStr, errCode ) )
					{
						tagsFound = errCode;
						break;
					}
				}
			}
		}
		else if ( tag == AD_PopulationParamExistsTag )
		{
			valStr = ( def.PopulationParamExists == true ) ? TrueStr : FalseStr;
		}

		if (dataOk && (valStr.length() > 0))
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		// get the schema name, table name, and key identifier tags
		GetAnalysisDefinitionQueryTag( schemaName, tableName, selectTag, idStr, def.AnalysisDefId );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateAnalysisParamRecord( DB_AnalysisParamRecord& params, bool in_ext_transaction )
{
	if ( !GuidValid( params.ParamId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_AnalysisParamRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetAnalysisParamObj( dbRec, resultRec, tagList, params.ParamId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == AP_InitializedTag )						// "IsInitialized"
		{
			valStr = ( params.IsInitialized == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == AP_LabelTag )						// "AnalysisParamLabel"
		{
			valStr = boost::str( boost::format( "'%s'" ) % params.ParamLabel );
			SanitizeDataString( valStr );
		}
		else if ( tag == AP_KeyTag )						// "CharacteristicKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.key );
		}
		else if ( tag == AP_SKeyTag )						// "CharacteristicSKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.s_key );
		}
		else if ( tag == AP_SSKeyTag )						// "CharacteristicSSKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.s_s_key );
		}
		else if ( tag == AP_ThreshValueTag )				// "ThresholdValue"
		{
			valStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % params.ThresholdValue );
		}
		else if ( tag == AP_AboveThreshTag )				// "AboveThreshold"
		{
			valStr = ( params.AboveThreshold == true ) ? TrueStr : FalseStr;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		// get the schema name, table name, and key identifier tags
		GetAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, params.ParamId, params.ParamIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateIlluminatorRecord(DB_IlluminatorRecord& ilr, bool in_ext_transaction )
{
	if ( ilr.IlluminatorIdNum == 0 || ilr.IlluminatorIndex < 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_IlluminatorRecord dbRec = {};
	CRecordset resultRec( pDb );
	int64_t chkIdNum = NO_ID_NUM;

	if ( ilr.IlluminatorIdNum > 0 )
	{
		chkIdNum = ilr.IlluminatorIdNum;
	}

	// get the column tags and current db record
	queryResult = GetIlluminatorObj( dbRec, resultRec, tagList, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, chkIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		queryResult = GetIlluminatorObj( dbRec, resultRec, tagList, ilr.EmissionWavelength, ilr.IlluminatorWavelength );
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == IL_IdxTag )						// "IlluminatorIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorIndex );
		}
		else if ( tag == IL_TypeTag )				// "IlluminatorType"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorType );
		}
		else if ( tag == IL_NameTag )				// "IlluminatorName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ilr.IlluminatorNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == IL_PosNumTag )				// "PositionNum"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.PositionNum );
		}
		else if ( tag == IL_ToleranceTag )			// "Tolerance"
		{
			valStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % ilr.Tolerance );
		}
		else if ( tag == IL_MaxVoltageTag )			// "MaxVoltage"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.MaxVoltage );
		}
		else if ( tag == IL_IllumWavelengthTag )	// "IlluminatorWavelength"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorWavelength );
		}
		else if ( tag == IL_EmitWavelengthTag )		// "EmissionWavelength"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.EmissionWavelength );
		}
		else if ( tag == IL_ExposureTimeMsTag )		// "ExposureTimeMs"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.ExposureTimeMs );
		}
		else if ( tag == IL_PercentPowerTag )		// "PercentPower"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.PercentPower );
		}
		else if ( tag == IL_SimmerVoltageTag )		// "SimmerVoltage"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.SimmerVoltage );
		}
		else if ( tag == IL_LtcdTag )				// "Ltcd"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.Ltcd );
		}
		else if ( tag == IL_CtldTag )				// "Ctld"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.Ctld );
		}
		else if ( tag == IL_FeedbackDiodeTag )		// "FeedbackPhotoDiode"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.FeedbackDiode );
		}
		else if ( tag == ProtectedTag )
		{
			valStr = ( ilr.Protected ) ? TrueStr : FalseStr;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		// get the schema name, table name, and key identifier tags
		GetIlluminatorQueryTag( schemaName, tableName, selectTag, idStr, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, ilr.IlluminatorIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateUserRecord( DB_UserRecord& ur )
{
	if ( !GuidValid( ur.UserId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_UserRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetUserObj( dbRec, resultRec, tagList, ur.UserId, INVALID_INDEX );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	std::string tmpStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagCnt = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t numCellTypes = -1;
	int32_t numCellTypesTagIdx = -1;
	int32_t numProps = -1;
	int32_t numPropsTagIdx = -1;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

	tagCnt = (int32_t)tagList.size();

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == UR_RetiredTag )							// "Retired"
		{
			if ( dbRec.Retired == false &&		// Do not allow reactivation of a retired user account
				 ur.Retired == true )			// no action required if not changing state...
			{
				valStr = ( ur.Retired == true ) ? TrueStr : FalseStr;		// an active account can be update
			}
		}
		else if ( tag == UR_RoleIdTag )						// "RoleID"; the instrument role for this user ID;  multiple roles not supported here; AD mapping will handle many-to-one mapping
		{
			int32_t errCode = 0;

			if ( !GuidUpdateCheck( dbRec.RoleId, ur.RoleId, valStr, errCode, ID_UPDATE_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == UR_DisplayNameTag )				// "DisplayName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DisplayNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_UserEmailTag )					// "UserEmail"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.UserEmailStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_CommentsTag )					// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.Comment );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_AuthenticatorListTag )			// "AuthenticatorList"
		{
			size_t listSize = 0;
			size_t listCnt = 0;

			foundCnt = tagsFound;
			listSize = ur.AuthenticatorList.size();
			if ( listSize == 0 )	// MUST have at least 1 authenticator...
			{
				dataOk = false;
				foundCnt = NoItemListCnt;
				queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				break;  // break out of the tag 'for' loop...
			}

			listCnt = listSize;
			valStr.clear();

			// validate that the list of user authenticators is valid, if any have been saved
			for ( uint32_t i = 0; i < listCnt; i++ )
			{
				if ( ur.AuthenticatorList.at( i ).empty() )			// cannot have an empty authenticator hash string
				{
					dataOk = false;
					foundCnt = MissingListItemValue;
					queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					break;  // break out of the tag 'for' loop...
				}
			}

			if ( dataOk )
			{
				std::vector<std::string> cleanList = {};

				cleanList.clear();
				SanitizeDataStringList( ur.AuthenticatorList, cleanList );

				if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}

			if ( foundCnt < 0 )
			{
				tagsFound = foundCnt;
				dataOk = false;
				break;  // break out of the tag check 'for' loop
			}
		}
		else if ( tag == UR_AuthenticatorDateTag )			// "UserAuthenticatorDate"
		{
			system_TP zeroTP = {};

			if ( ur.AuthenticatorDateTP != zeroTP )
			{
				GetDbTimeString( ur.AuthenticatorDateTP, valStr );
			}
		}
		else if ( tag == UR_LastLoginTag )					// "LastLogin"
		{
			system_TP zeroTP = {};

			if ( ur.LastLogin != zeroTP )
			{
				GetDbTimeString( ur.LastLogin, valStr );
			}
		}
		else if ( tag == UR_AttemptCntTag )					// "AttemptCount"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.AttemptCount );
		}
		else if ( tag == UR_LanguageCodeTag )				// "LanguageCode"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.LanguageCode );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_DfltSampleNameTag )				// "DefaultSampleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DefaultSampleNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_UserImageSaveNTag )				// "SaveNthIImage"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.UserImageSaveN );
		}
		else if ( tag == UR_DisplayColumnsTag )				// "DisplayColumns"
		{
			size_t listSize = 0;

			listSize = ur.ColumnDisplayList.size();		// if the list is empty, the display should be using the defaults...

			if ( listSize > 0 )
			{
				int32_t listCnt = static_cast<int32_t>(listSize);

				foundCnt = tagsFound;
				valStr.clear();

				if ( CreateColumnDisplayInfoArrayString( valStr, static_cast<int32_t>( listCnt ), ur.ColumnDisplayList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt < 0 )
				{
					tagsFound = foundCnt;
					valStr.clear();
					dataOk = false;
					break;  // break out of the tag check 'for' loop
				}
			}
		}
		else if ( tag == UR_DecimalPrecisionTag )			// "DecimalPrecision"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.DecimalPrecision );
		}
		else if ( tag == UR_ExportFolderTag )				// "ExportFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.ExportFolderStr );
			SanitizePathString( valStr );
		}
		else if ( tag == UR_DfltResultFileNameStrTag )		// "DefaultResultFileName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DefaultResultFileNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_CSVFolderTag )					// "CSVFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.CSVFolderStr );
			SanitizePathString( valStr );
		}
		else if ( tag == UR_PdfExportTag )					// "PdfExport"
		{
			valStr = ( ur.PdfExport == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == UR_AllowFastModeTag )				// "AllowFastMode"
		{
			valStr = ( ur.AllowFastMode == true ) ? TrueStr : FalseStr;
		}
		else if ( tag == UR_WashTypeTag )					// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.WashType );
		}
		else if ( tag == UR_DilutionTag )					// "Dilution"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.Dilution );
		}
		else if ( tag == UR_DefaultCellTypeIdxTag )			// "DefaultCellTypeIndex";
		{
			valStr = boost::str( boost::format( "%d" ) % ur.DefaultCellType );
		}
		else if ( tag == UR_NumCellTypesTag )				// "NumUserCellTypes"
		{
			if ( numCellTypes <= 0 )		// hasn't yet been validated against the object list; don't write
			{
				numCellTypesTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % numCellTypes );
				numCellTypes = ur.NumUserCellTypes;
			}
		}
		else if ( tag == UR_CellTypeIdxListTag )			// "UserCellTypeIndexList"
		{
			std::vector<int32_t> ctIndexList = {};
			int32_t indexVal = 0;
			int64_t tmpIdx = 0;
			size_t listSize = 0;
			size_t listCnt = 0;
			bool ctFound = false;

			listSize = ur.UserCellTypeIndexList.size();
			listCnt = listSize;

			foundCnt = tagsFound;

			valStr.clear();

			// check against the number of objects indicated, not from the vector...
			if ( numCellTypes <= ItemListCntOk )
			{
				numCellTypes = ur.NumUserCellTypes;
			}

			if ( numCellTypes != listCnt )
			{
				if ( numCellTypes >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				indexVal = (int32_t) ur.UserCellTypeIndexList.at( i );
				tmpIdx = (int64_t) ur.UserCellTypeIndexList.at( i );
				ctFound = false;

				// check if this is an existing object
				DB_CellTypeRecord chk_ct = {};
				uuid__t tmpId;

				ClearGuid( tmpId );

				if ( GetCellTypeInternal( chk_ct, tmpId, tmpIdx ) == DBApi::eQueryResult::QueryOk )
				{
					ctFound = true;
				}

				if ( !ctFound )
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					ctIndexList.push_back( indexVal );
				}
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateInt32ValueArrayString( valStr, "%ld", static_cast<int32_t>( listCnt ), 0, ctIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )
				{
					ur.NumUserCellTypes = static_cast<int16_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numCellTypes != listCnt )
				{
					numCellTypes = static_cast<int32_t>( listCnt );
					ur.NumUserCellTypes = static_cast<int16_t>( listCnt );
				}

				if ( numCellTypesTagIdx > 0 )			// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numCellTypesTagIdx ) );		// make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == UR_AnalysisDefIdxListTag )			// "AnalysisDefIndexList"
		{
			std::vector<int32_t> adIndexList = {};
			int32_t indexVal = 0;
			size_t listSize = 0;
			size_t listCnt = 0;
			bool adFound = false;

			listSize = ur.UserAnalysisIndexList.size();
			listCnt = listSize;

			foundCnt = tagsFound;

			valStr.clear();

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				indexVal = (int32_t) ur.UserAnalysisIndexList.at( i );
				adFound = false;

				// check if this is an existing object
				DB_AnalysisDefinitionRecord chk_ad = {};
				uuid__t tmpId;

				ClearGuid( tmpId );

				if ( GetAnalysisDefinitionInternal( chk_ad, tmpId, INVALID_ID_NUM ) == DBApi::eQueryResult::QueryOk )
				{
					adFound = true;
				}

				if ( !adFound )
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					adIndexList.push_back( indexVal );
				}
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( CreateInt32ValueArrayString( valStr, "%ld", static_cast<int32_t>( listCnt ), 0, adIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}
		}
		else if ( tag == UR_NumUserPropsTag )			// "NumUserProperties"
		{
			if ( numProps <= 0 )         // hasn't yet been validated against the object list; don't write
			{
				numPropsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) numProps );		// cast to avoid boost bug for byte values...
				numProps = ur.NumUserProperties;
			}
		}
		else if ( tag == UR_UserPropsIdxListTag )		// "UserPropertiesIndexList"
		{
			std::vector<int16_t> upIndexList = {};
			int64_t propIdNum = 0;
			int16_t propIdx = -1;
			std::string propName = "";
			size_t listSize = 0;
			size_t listCnt = 0;
			bool upFound = false;

			foundCnt = tagsFound;
			listSize = ur.UserPropertiesList.size();
			listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( numProps < ItemListCntOk )
			{
				numProps = ur.NumUserProperties;
			}

			if ( numProps != listSize )
			{
				if ( numProps >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			// validate that the indicated user properties exist in the db and add if they don't
			for ( int32_t i = 0; i < listCnt; i++ )
			{
				DB_UserPropertiesRecord& up = ur.UserPropertiesList.at( i );
				upFound = false;

				// index MUST be valid
				if ( up.PropertyIndex < 0 )
				{
					dataOk = false;
					queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					break;
				}

				propIdNum = NO_ID_NUM;
				propIdx = INVALID_INDEX;
				propName = "";

				// if it appear to have been inserted previously...
				if ( up.PropertiesIdNum > INVALID_ID_NUM )
				{
					propIdNum = up.PropertiesIdNum;

					if ( up.PropertyIndex >= 0 )
					{
						propIdx = up.PropertyIndex;
					}

					if ( up.PropertyNameStr.length() > 0 )
					{
						propName = up.PropertyNameStr;
					}
				}
				else
				{
					propIdNum = NO_ID_NUM;
					up.PropertiesIdNum = 0;

					insertResult = InsertUserPropertyRecord( up, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk && ( up.PropertiesIdNum > INVALID_ID_NUM ) )
					{
						upFound = true;
						propIdNum = up.PropertiesIdNum;
						propIdx = up.PropertyIndex;
						if ( up.PropertyNameStr.length() > 0 )
						{
							propName = up.PropertyNameStr;
						}
						ur.UserPropertiesList.at( i ).PropertiesIdNum = up.PropertiesIdNum;		// update the idnum in the passed-in object
					}
					else
					{
						dataOk = false;
						queryResult = DBApi::eQueryResult::InsertFailed;
					}
				}

				if ( up.PropertiesIdNum > INVALID_ID_NUM )
				{
					DB_UserPropertiesRecord chk_up = {};

					// check for retrieval of inserted/found item by the idnum, id, and index...
					if ( GetUserPropertyInternal( chk_up, propIdx, propName, propIdNum ) == DBApi::eQueryResult::QueryOk )
					{
						// fail if the idnum has been previously set and any fields have changed...
						if ( chk_up.PropertiesIdNum != up.PropertiesIdNum || propIdx != chk_up.PropertyIndex || propName != chk_up.PropertyNameStr )
						{
							dataOk = false;
							queryResult = DBApi::eQueryResult::InsertObjectExists;
						}
						else
						{
							upFound = true;
							propIdNum = chk_up.PropertiesIdNum;
							propIdx = chk_up.PropertyIndex;
							if ( chk_up.PropertyNameStr.length() > 0 )
							{
								propName = chk_up.PropertyNameStr;
							}
						}
					}
				}

				if ( !upFound )
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					upIndexList.push_back( propIdx );
				}
			}

			listCnt = upIndexList.size();

			if ( dataOk )
			{
				if ( CreateInt16ValueArrayString( valStr, "%d", static_cast<int32_t>( listCnt ), 0, upIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}

				if ( foundCnt == SetItemListCnt )  // need to update the suppied object...
				{
					ur.NumUserProperties = static_cast<int8_t>( listCnt );
					foundCnt = tagsFound;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;  // break out of the tag check 'for' loop
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numProps != listCnt )
				{
					numProps = static_cast<int32_t>( listCnt );
					ur.NumUserProperties = static_cast<int8_t>( listCnt );
				}

				if ( numPropsTagIdx >= 0 )       //  the tag has already been handled prior to the object validation
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numPropsTagIdx ) );         // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}   // else if ( tag == "UserPropertiesIndexList" )
		else if ( tag == UR_AppPermissionsTag )			// "AppPermissions"
		{
			valStr = boost::str( boost::format( "%lld" ) % ((int64_t) ur.AppPermissions ) );
		}
		else if ( tag == UR_AppPermissionsHashTag )		// "AppPermissionsHash"
		{
			// SHOULD NOT be null at the time of creation
			//if ( ur.AppPermissionsHash.length() == 0 )
			//{
			//	tagsFound = MissingCriticalObjectID;			// cannot have an empty hash
			//	break;  // break out of the tag check 'for' loop
			//}
			valStr = boost::str( boost::format( "'%s'" ) % ur.AppPermissionsHash );
			SanitizeDataString( valStr );
		}
		else if ( tag == UR_InstPermissionsTag )		// "InstrumentPermissions"
		{
			valStr = boost::str( boost::format( "%lld" ) % ( (int64_t) ur.InstPermissions ) );
		}
		else if ( tag == UR_InstPermissionsHashTag )	// "InstrumentPermissionsHash"
		{
			//// SHOULD NOT be null at the time of creation
			//if ( ur.InstPermissionsHash.length() == 0 )
			//{
			//	tagsFound = MissingCriticalObjectID;			// cannot have an empty hash
			//	break;  // break out of the tag check 'for' loop
			//}
			valStr = boost::str( boost::format( "'%s'" ) % ur.InstPermissionsHash );
			SanitizeDataString( valStr );
		}
		else if ( tag == ProtectedTag )					// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
//			valStr = ( ilr.Protected ) ? TrueStr : FalseStr;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}   // for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetUserQueryTag( schemaName, tableName, selectTag, idStr, ur.UserId, ur.UserIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( idFails > 0 )
		{
			queryResult = DBApi::eQueryResult::MissingQueryKey;
		}
		else if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateRoleRecord( DB_UserRoleRecord& rr )
{
	if ( !GuidValid( rr.RoleId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_UserRoleRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetRoleObj( dbRec, resultRec, tagList, rr.RoleId, rr.RoleIdNum, rr.RoleNameStr );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;

	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == RO_NameTag )						// "RoleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rr.RoleNameStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == RO_RoleTypeTag )				// "RoleType"
		{
			valStr = boost::str( boost::format( "%d" ) % rr.RoleType );
		}
		else if ( tag == RO_GroupMapListTag )			// "GroupMapList"
		{
			size_t listSize = 0;

			listSize = rr.GroupMapList.size();
			if ( listSize == 0 )	// May be empty on non-networked systems
			{
				break;  // break out of the tag 'for' loop...
			}

			std::vector<std::string> groupMapList;
			std::string groupTagStr = "";
			size_t listCnt = 0;

			foundCnt = tagsFound;
			listCnt = listSize;
			valStr.clear();

			// validate that the list of user authenticators is valid, if any have been saved
			for ( uint32_t i = 0; i < listCnt; i++ )
			{
				if ( rr.GroupMapList.at( i ).empty() )		// cannot have an empty map tag string
				{
					foundCnt = MissingListItemValue;
					dataOk = false;
				}
			}

			if ( dataOk )
			{
				std::vector<std::string> cleanList = {};

				cleanList.clear();
				SanitizeDataStringList( rr.GroupMapList, cleanList );

				if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					dataOk = false;
				}
			}

			if ( foundCnt < 0 )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;  // break out of the tag check 'for' loop
			}
		}
		else if ( tag == RO_CellTypeIdxListTag )		// "CellTypeIndexList"
		{
			std::vector<int32_t> ctIndexList = {};
			int32_t indexVal = 0;
			int64_t tmpIdx = 0;
			size_t listSize = 0;
			size_t listCnt = 0;
			bool ctFound = false;

			listSize = rr.CellTypeIndexList.size();
			listCnt = listSize;

			valStr.clear();

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				tmpIdx = (int64_t) rr.CellTypeIndexList.at( i );
				indexVal = (int32_t) rr.CellTypeIndexList.at( i );
				ctFound = false;

				// index MUST be valid
				if ( indexVal < 0 )
				{
					dataOk = false;
					queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					break;
				}

				// check if this is an existing object
				DB_CellTypeRecord chk_ct = {};
				uuid__t tmpId;

				ClearGuid( tmpId );

				if ( GetCellTypeInternal( chk_ct, tmpId, tmpIdx ) == DBApi::eQueryResult::QueryOk )
				{
					ctFound = true;
				}

				if ( !ctFound )
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					ctIndexList.push_back( indexVal );
				}
			}

			if ( dataOk )
			{
				if ( CreateInt32ValueArrayString( valStr, "%ld", static_cast<int32_t>( listCnt ), 0, ctIndexList ) != listCnt )
				{
					foundCnt = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}

			if ( foundCnt < ListItemsOk )
			{
				tagsFound = foundCnt;
				valStr.clear();
				dataOk = false;
				break;
			}
		}
		else if ( tag == RO_InstPermissionsTag )		// "InstPermissions"
		{
			valStr = boost::str( boost::format( "%lld" ) % ( (int64_t) rr.InstrumentPermissions ) );
		}
		else if ( tag == RO_AppPermissionsTag )			// "AppPermisions"
		{
			valStr = boost::str( boost::format( "%lld" ) % ( (int64_t) rr.ApplicationPermissions ) );
		}
		else if ( tag == ProtectedTag )					// "Protected" - not currently used
		{
//			if ( dbRec.Protected != true )
//			{
//				valStr = ( rr.Protected == true ) ? TrueStr : FalseStr;
//			}
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetRoleQueryTag( schemaName, tableName, selectTag, idStr, rr.RoleId, rr.RoleIdNum, rr.RoleNameStr );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else if ( tagsFound == MissingListItemValue )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateUserPropertyRecord( DB_UserPropertiesRecord& up, bool in_ext_transaction )
{
	if ( up.PropertiesIdNum == 0 || up.PropertyIndex < 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	// transaction cleanup
	if ( !in_ext_transaction )
	{
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS
		}
		else
		{
#ifdef HANDLE_TRANSACTIONS
			CancelTransaction();
#endif // HANDLE_TRANSACTIONS
		}
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSignatureRecord( DB_SignatureRecord& sig )
{
	if ( !GuidValid( sig.SignatureDefId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_SignatureRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetSignatureObj( dbRec, resultRec, tagList, sig.SignatureDefId, INVALID_INDEX );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == SG_ShortSigTag )			// "ShortSignature"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sig.ShortSignatureStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SG_ShortSigHashTag )	// "ShortSignatureHash"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sig.ShortSignatureHash );
			SanitizeDataString( valStr );
		}
		else if ( tag == SG_LongSigTag )		// "LongSignature"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sig.LongSignatureStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SG_LongSigHashTag )	// "LongSignatureHash"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sig.LongSignatureHash );
			SanitizeDataString( valStr );
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ((sig.Protected == true ) ? TrueStr : FalseStr) );
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSignatureQueryTag( schemaName, tableName, selectTag, idStr, sig.SignatureDefId, sig.SignatureDefIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}



DBApi::eQueryResult DBifImpl::UpdateReagentTypeRecord( DB_ReagentTypeRecord& rxr )
{
	if ( rxr.ReagentIdNum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_ReagentTypeRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetReagentTypeObj( dbRec, resultRec, tagList, rxr.ReagentIdNum, rxr.ContainerTagSn );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	if ( dbRec.ContainerTagSn != rxr.ContainerTagSn )
	{
		WriteLogEntry( "Container Tag Serial number does not match record found by id number!", InfoMsgType );
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = true;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t updateIdx = 0;

	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end() && dataOk; ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == RX_TypeNumTag )							// "ReagentTypeNum"
		{
			valStr = boost::str( boost::format( "%d" ) % rxr.ReagentTypeNum );
		}
		else if ( tag == RX_CurrentSnTag )					// "Current"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( rxr.Current == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == RX_ContainerRfidSNTag )			// "ContainerTagSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.ContainerTagSn );
			SanitizeDataString( valStr );
		}
		else if ( tag == RX_IdxListTag )					// "ReagentIndexList"
		{
			size_t listSize = rxr.ReagentIndexList.size();

			if ( CreateInt16ValueArrayString( valStr, "%ld", static_cast<int32_t>( listSize ), 0, rxr.ReagentIndexList ) != listSize )
			{
				foundCnt = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( tag == RX_NamesListTag )					// "ReagentNamesList"
		{
			size_t listSize = rxr.ReagentNamesList.size();
			std::vector<std::string> cleanList = {};

			cleanList.clear();
			SanitizeDataStringList( rxr.ReagentNamesList, cleanList );

			if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listSize ), 0, cleanList ) != listSize )
			{
				foundCnt = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( tag == RX_MixingCyclesTag )				// "MixingCycles"
		{
			size_t listSize = rxr.MixingCyclesList.size();

			if ( CreateInt16ValueArrayString( valStr, "%ld", static_cast<int32_t>( listSize ), 0, rxr.MixingCyclesList ) != listSize )
			{
				foundCnt = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( tag == RX_PackPartNumTag )				// "PackPartNum"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.PackPartNumStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == RX_LotNumTag )						// "LotNum"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.LotNumStr );
			SanitizeDataString( valStr );
		}
		// NOTE that this is in raw epoch counts for the day the pack will expire, and SHOULD NOT use
		// a true tiom point to avoid shifting the expiration date day due to timeszone differences
		else if ( tag == RX_LotExpirationDateTag )			// "LotExpiration"
		{
			valStr = boost::str( boost::format( "%lld" ) % rxr.LotExpirationDate );
		}
		// NOTE that this is in raw epoch counts for the day the pack is placed in-service, and SHOULD NOT be
		// a true tiom point to avoid shifting the expiration date day due to timeszone differences
		else if ( tag == RX_InServiceDateTag )				// "InService"
		{
			valStr = boost::str( boost::format( "%lld" ) % rxr.InServiceDate );
		}
		// This is in the number of DAYS the pack may be used after being placed in service, NOT a time point
		else if ( tag == RX_InServiceDaysTag )				// "ServiceLife"
		{
			valStr = boost::str( boost::format( "%d" ) % rxr.InServiceExpirationLength );
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( rxr.Current == true ) ? TrueStr : FalseStr ) );
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetReagentTypeQueryTag( schemaName, tableName, selectTag, idStr, rxr.ReagentIdNum, rxr.ContainerTagSn );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateCellHealthReagentRecord(DB_CellHealthReagentRecord& chr)
{
	if (chr.IdNum == 0 || chr.Type < 0)
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck(pDb);

	if (loginType == DBApi::eLoginType::NoLogin)
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_CellHealthReagentRecord dbRec = {};
	CRecordset resultRec(pDb);
	int64_t chkIdNum = NO_ID_NUM;

	if (chr.IdNum > 0)
	{
		chkIdNum = chr.IdNum;
	}

	// current db record
	queryResult = GetCellHealthReagentObj(dbRec, resultRec, tagList, chr.Type);
	if (queryResult != DBApi::eQueryResult::QueryOk)
	{
		if (queryResult != DBApi::eQueryResult::NoResults)
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if (resultRec.IsOpen())
		{
			resultRec.Close();
		}

		return queryResult;
	}

	if (dbRec.Type != chr.Type)
	{
//TODO:
		WriteLogEntry("Reagent type does not match record found by id number!", InfoMsgType);
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = true;
	int32_t idFails = 0;
	int32_t tagsFound = 0;
	int32_t foundCnt = 0;
	int32_t updateIdx = 0;

	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction(DBApi::eLoginType::AnyLoginType);
#endif // HANDLE_TRANSACTIONS

	for (auto tagIter = tagList.begin(); tagIter != tagList.end() && dataOk; ++tagIter)
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if (tag == CH_IdTag)
		{
			int32_t errCode = 0;

			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if (!GuidUpdateCheck(dbRec.Id, chr.Id, valStr, errCode))
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if (tag == CH_NameTag)
		{
			valStr = boost::str(boost::format("'%s'") % chr.Name);
			SanitizeDataString(valStr);
		}
		else if (tag == CH_TypeTag)						// "Type"
		{
			valStr = boost::str(boost::format("'%d'") % chr.Type);
			SanitizeDataString(valStr);
		}
		else if (tag == CH_VolumeTag)					// "Volume"
		{
			valStr = boost::str(boost::format("'%ld'") % chr.Volume);
			SanitizeDataString(valStr);
		}
		else if (tag == ProtectedTag)					// "Protected" - not currently used
		{
			valStr = (chr.Protected) ? TrueStr : FalseStr;
		}

		// now add the update statement component for the tag processed
		if (valStr.length() > 0)
		{
			tagsFound++;
			tagStr = boost::str(boost::format("\"%s\"") % tag);         // make the quoted string expected by the database for value names
			AddToInsertUpdateString(FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr);
		}
	}

	if (tagsFound > TagsOk)
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetCellHealthReagentsQueryTag(schemaName, tableName, selectTag, idStr, chr.Type);

		MakeColumnValuesInsertUpdateQueryString(tagsFound, queryStr, schemaName, tableName,
			namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE);

		queryResult = RunQuery(loginType, queryStr, resultRec);
	}
	else
	{
		if (tagsFound == 0)
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if (queryResult == DBApi::eQueryResult::QueryOk)
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if (resultRec.IsOpen())
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateBioProcessRecord( DB_BioProcessRecord& bpr )
{
	if ( !GuidValid( bpr.BioProcessId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateQcProcessRecord( DB_QcProcessRecord& qcr )
{
	if ( !GuidValid( qcr.QcId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_QcProcessRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetQcProcessObj( dbRec, resultRec, tagList, qcr.QcId, INVALID_INDEX );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string fmtStr = "";
	std::string tag = "";
	std::string tagStr = "";
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		tag = *tagIter;
		valStr.clear();

		if ( tag == QC_NameTag )
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.QcName );
			SanitizeDataString( valStr );
		}
		else if ( tag == QC_TypeTag )
		{
			valStr = boost::str( boost::format( "%d" ) % qcr.QcType );
		}
		else if ( tag == QC_CellTypeIdTag )
		{
			int32_t errCode = 0;

			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if ( !GuidUpdateCheck( dbRec.QcId, qcr.QcId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
		}
		else if ( tag == QC_CellTypeIndexTag )
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) qcr.CellTypeIndex );
		}
		else if ( tag == QC_LotInfoTag )
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.LotInfo );
			SanitizeDataString( valStr );
		}
		else if ( tag == QC_LotExpirationTag )
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.LotExpiration );
			SanitizeDataString( valStr );
		}
		else if ( tag == QC_AssayValueTag )
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( "%lf" ) % qcr.AssayValue );
		}
		else if ( tag == QC_AllowablePercentTag )
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( "%lf" ) % qcr.AllowablePercentage );
		}
		else if ( tag == QC_SequenceTag )
		{
			if ( qcr.QcSequence.length() > 0 )
			{
				valStr = boost::str( boost::format( "'%s'" ) % qcr.QcSequence );
				SanitizeDataString( valStr );
			}
			else
			{
				valStr = "' '";
			}
		}
		else if ( tag == QC_CommentsTag )
		{
			if ( qcr.Comments.length() > 0 )
			{
				valStr = boost::str( boost::format( "'%s'" ) % qcr.Comments );
				SanitizeDataString( valStr );
			}
			else
			{
				valStr = "' '";
			}
		}
		else if (tag == QC_RetiredTag)								// "Retired"
		{
			valStr = (qcr.Retired == true) ? TrueStr : FalseStr;
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ( ( qcr.Protected == true ) ? TrueStr : FalseStr));
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetQcProcessQueryTag( schemaName, tableName, selectTag, idStr, qcr.QcId, qcr.QcIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == MissingCriticalObjectID )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingListIds;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::UpdateInstConfigRecord( DB_InstrumentConfigRecord& icr )
{
	if ( icr.InstrumentIdNum == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_InstrumentConfigRecord dbRec = {};
	CRecordset resultRec( pDb );

	queryResult = GetInstConfigObj( dbRec, resultRec, tagList, "", icr.InstrumentIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string namesStr = "";
	std::string valStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// if current setting is clearing the installed or enabled state, don't allow ACup enable
	if ( !icr.AutomationInstalled || !icr.AutomationEnabled )
	{
		icr.AutomationEnabled = false;
		icr.ACupEnabled = false;
	}

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == CFG_InstSNTag )							// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.InstrumentSNStr );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_InstTypeTag )					// "InstrumentType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.InstrumentType );
		}
		else if ( tag == CFG_DeviceNameTag )				// "DeviceName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.DeviceName );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_UiVerTag )						// "UIVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.UIVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_SwVerTag )						// "SoftwareVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.SoftwareVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_AnalysisSwVerTag )				// "AnalysisSWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.AnalysisVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_FwVerTag )						// "FirmwareVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.FirmwareVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_CameraTypeTag )				// "CameraType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.CameraType );
		}

		else if (tag == CFG_BrightFieldLedTypeTag)				// "BrightFieldLedType"
		{
			valStr = boost::str(boost::format("%d") % icr.BrightFieldLedType);
		}
		else if ( tag == CFG_CameraFwVerTag )				// "CameraFWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraFWVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_CameraCfgTag )					// "CameraConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraConfig );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_PumpTypeTag )					// "PumpType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.PumpType );
		}
		else if ( tag == CFG_PumpFwVerTag )					// "PumpFWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.PumpFWVersion );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_PumpCfgTag )					// "PumpConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.PumpConfig );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_IlluminatorsInfoListTag )		// "IlluminatorsInfoList"
		{
			size_t listSize = icr.IlluminatorsInfoList.size();

			valStr.clear();

			if ( CreateIlluminatorInfoArrayString( valStr, static_cast<int32_t>( listSize ), icr.IlluminatorsInfoList, true ) != listSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_IlluminatorCfgTag )			// "IlluminatorConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.IlluminatorConfig );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_ConfigTypeTag )				// "ConfigType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.ConfigType );
		}
		else if ( tag == CFG_LogNameTag )					// "LogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.LogName );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_LogMaxSizeTag )				// "LogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.LogMaxSize );
		}
		else if ( tag == CFG_LogLevelTag )					// "LogSensitivity"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.LogSensitivity );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_MaxLogsTag )					// "MaxLogs"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.MaxLogs );
		}
		else if ( tag == CFG_AlwaysFlushTag )				// "AlwaysFlush"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AlwaysFlush == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == CFG_CameraErrLogNameTag )			// "CameraErrorLogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraErrorLogName );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_CameraErrLogMaxSizeTag )		// "CameraErrorLogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CameraErrorLogMaxSize );
		}
		else if ( tag == CFG_StorageErrLogNameTag )			// "StorageErrorLogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.StorageErrorLogName );
			SanitizeDataString( valStr );
		}
		else if ( tag == CFG_StorageErrLogMaxSizeTag )		// "StorageErrorLogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.StorageErrorLogMaxSize );
		}
		else if ( tag == CFG_CarouselThetaHomeTag )			// "CarouselThetaHomeOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CarouselThetaHomeOffset );
		}
		else if ( tag == CFG_CarouselRadiusOffsetTag )		// "CarouselRadiusOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CarouselRadiusOffset );
		}
		else if ( tag == CFG_PlateThetaHomeTag )			// "PlateThetaHomePosOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateThetaHomeOffset );
		}
		else if ( tag == CFG_PlateThetaCalTag )				// "PlateThetaCalPos"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateThetaCalPos );
		}
		else if ( tag == CFG_PlateRadiusCenterTag )			// "PlateRadiusCenterPos"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateRadiusCenterPos );
		}
		else if ( tag == CFG_SaveImageTag )					// "SaveImage"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SaveImage );
		}
		else if ( tag == CFG_FocusPositionTag )				// "FocusPosition"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.FocusPosition );
		}
		else if ( tag == CFG_AutoFocusTag )					// "AutoFocus"
		{
			valStr.clear();

			if ( !MakeAfSettingsString( valStr, icr.AF_Settings, true ) )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_AbiMaxImageCntTag )			// "AbiMaxImageCount"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.AbiMaxImageCount );
		}
		else if ( tag == CFG_NudgeVolumeTag )				// "SampleNudgeVolume"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SampleNudgeVolume );
		}
		else if ( tag == CFG_NudeSpeedTag )					// "SampleNudgeSpeed"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SampleNudgeSpeed );
		}
		else if ( tag == CFG_FlowCellDepthTag )				// "FlowCellDepth"
		{
			valStr = boost::str( boost::format( "%0.2f" ) % icr.FlowCellDepth );
		}
		else if ( tag == CFG_FlowCellConstantTag )			// "FlowCellDepthConstant"
		{
			valStr = boost::str( boost::format( "%0.2f" ) % icr.FlowCellDepthConstant );
		}
		else if ( tag == CFG_RfidSimTag )					// "RfidSim"
		{
			valStr.clear();

			if ( !MakeRfidSimInfoString( valStr, icr.RfidSim, true ) )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_LegacyDataTag )				// "LegacyData"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.LegacyData == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == CFG_CarouselSimTag )				// "CarouselSimulator"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.CarouselSimulator == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == CFG_NightlyCleanOffsetTag )		// "NightlyCleanOffset"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.NightlyCleanOffset );
		}
		else if ( tag == CFG_LastNightlyCleanTag )			// "LastNightlyClean"
		{
			system_TP newTimePT = {};

			if ( icr.LastNightlyClean != newTimePT )
			{
				GetDbTimeString( icr.LastNightlyClean, valStr );
			}
		}
		else if ( tag == CFG_SecurityModeTag )				// "SecurityMode"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SecurityMode );
		}
		else if ( tag == CFG_InactivityTimeoutTag )			// "InactivityTimeout"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.InactivityTimeout );
		}
		else if ( tag == CFG_PwdExpirationTag )				// "PasswordExpiration"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.PasswordExpiration );
		}
		else if ( tag == CFG_NormalShutdownTag )			// "NormalShutdown"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.NormalShutdown == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == CFG_NextAnalysisDefIndexTag )		// "NextAnalysisDefIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.NextAnalysisDefIndex );
		}
		else if ( tag == CFG_NextBCICellTypeIndexTag )		// "NextFactoryCellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.NextBCICellTypeIndex );
		}
		else if ( tag == CFG_NextUserCellTypeIndexTag )		// "NextUserCellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % static_cast<int32_t>( icr.NextUserCellTypeIndex ) );
		}
		else if ( tag == CFG_TotSamplesProcessedTag )		// "SamplesProcessed"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.TotalSamplesProcessed );
		}
		else if ( tag == CFG_DiscardTrayCapacityTag )		// "DiscardCapacity"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.DiscardTrayCapacity );
		}
		else if ( tag == CFG_EmailServerTag )				// "EmailServer"
		{
			valStr.clear();

			if ( !MakeEmailSettingsString( valStr, icr.Email_Settings, true ) )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_AdSettingsTag )				// "ADSettings"
		{
			valStr.clear();

			if ( !MakeAdSettingsString( valStr, icr.AD_Settings, true ) )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_LanguageListTag )				// "LanguageList"
		{
			size_t listSize = icr.LanguageList.size();

			valStr.clear();

			if ( CreateLanguageInfoArrayString( valStr, static_cast<int32_t>( listSize ), icr.LanguageList, true ) != listSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_RunOptionsTag )				// "RunOptionDefaults"
		{
			valStr.clear();

			if ( !MakeRunOptionsString( valStr, icr.RunOptions, true ) )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
		}
		else if ( tag == CFG_AutomationInstalledTag )		// "AutomationInstalled"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AutomationInstalled == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == CFG_AutomationEnabledTag )			// "AutomationEnabled"
		{
			// if not already installed AND not setting installation now, don't allow enable
			if ( !dbRec.AutomationInstalled && !icr.AutomationInstalled )
			{
				icr.AutomationEnabled = false;
			}
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AutomationEnabled == true ) ? TrueStr : FalseStr ) );
		}
		else if (tag == CFG_ACupEnabledTag)					// "ACupEnabled"
		{
			// if not already installed AND not setting installation now OR current setting fo renabled is NOT true, don't allow ACup enable
			if ((!dbRec.AutomationInstalled && !icr.AutomationInstalled) || !icr.AutomationEnabled)
			{
				icr.ACupEnabled = false;
			}
			valStr = boost::str(boost::format("%s") % ((icr.ACupEnabled == true) ? TrueStr : FalseStr));
		}
		else if ( tag == CFG_AutomationPortTag )			// "AutomationPort"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.AutomationPort );
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ( ( icr.Protected == true ) ? TrueStr : FalseStr ) );
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}   // for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )

	if ( tagsFound > 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetInstConfigQueryTag( schemaName, tableName, selectTag, idStr, icr.InstrumentSNStr, icr.InstrumentIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == ListItemNotFound )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else if ( tagsFound == BadCriticalObjectID )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingListIds;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateLogEntry( DB_LogEntryRecord& logr )
{
	if ( logr.IdNum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		loginType = LoginAsAdmin();
	}
	else
	{
		loginType = SetLoginType( DBApi::eLoginType::AdminLoginType );
	}

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_LogEntryRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetLogEntryObj( dbRec, resultRec, tagList, logr.IdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		tag = *tagIter;
		valStr.clear();

		if ( tag == LOG_LogEntryTag )			// "EntryText"
		{
			valStr = boost::str( boost::format( "'%s'" ) % logr.EntryStr );
			SanitizeDataString( valStr );
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetLogEntryQueryTag( schemaName, tableName, selectTag, idStr, logr.IdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	// transaction cleanup
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		EndTransaction();
#endif // HANDLE_TRANSACTIONS
	}
	else
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::UpdateSchedulerConfigRecord( DB_SchedulerConfigRecord& scr )
{
	if ( !GuidValid( scr.ConfigId ) )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		loginType = LoginAsAdmin();
	}
	else
	{
		loginType = SetLoginType( DBApi::eLoginType::AdminLoginType );
	}

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<std::string> tagList = {};
	DB_SchedulerConfigRecord dbRec = {};
	CRecordset resultRec( pDb );

	// get the column tags and current db record
	queryResult = GetSchedulerConfigObj( dbRec, resultRec, tagList, scr.ConfigId, scr.ConfigIdNum );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}

		if ( resultRec.IsOpen() )
		{
			resultRec.Close();
		}

		return queryResult;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	size_t listSize = 0;
	int32_t filterTypeTags = 0;
	int32_t filterOpTags = 0;
	int32_t filterValTags = 0;
	system_TP zeroTP = {};

	queryResult = DBApi::eQueryResult::QueryFailed;

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == SCH_NameTag )							// "SchedulerName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.Name );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_CommentsTag )					// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.Comments );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_FilenameTemplateTag )			// "OutputFilenameTemplate"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.FilenameTemplate );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_OutputTypeTag )				// "OutputType"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.OutputType );
		}
		else if ( tag == SCH_StartDateTag )					// "StartDate"
		{
			if ( scr.StartDate == zeroTP )
			{
				GetDbCurrentTimeString( valStr, scr.StartDate );
			}
			else
			{
				GetDbTimeString( scr.StartDate, valStr );
			}
		}
		else if ( tag == SCH_StartOffsetTag )				// "StartOffset"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.StartOffset );
		}
		else if ( tag == SCH_RepetitionIntervalTag )		// "RepetitionInterval"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.MultiRepeatInterval );
		}
		else if ( tag == SCH_DayWeekIndicatorTag )			// "DayWeekIndicator"
		{
			// if the monthly bit and the run day are set, clear the weekday bits...
			if ( scr.DayWeekIndicator & DBApi::eDayWeekIndicatorBits::MonthBit )
			{
				if ( scr.MonthlyRunDay > 0 )
				{
					scr.DayWeekIndicator &= ~( DBApi::eDayWeekIndicatorBits::DayIndicatorMask );
				}
				else
				{
					scr.DayWeekIndicator &= ~( DBApi::eDayWeekIndicatorBits::MonthBit );
				}
			}
			else
			{
				scr.MonthlyRunDay = 0;
			}
			valStr = boost::str( boost::format( "%d" ) % scr.DayWeekIndicator );
		}
		else if ( tag == SCH_MonthlyRunDayTag )				// "MonthlyRunDay"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.MonthlyRunDay );
		}
		else if ( tag == SCH_DestinationFolderTag )			// "DestinationFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.DestinationFolder );
			SanitizePathString( valStr );
		}
		else if ( tag == SCH_DataTypeTag )					// "DataType"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.DataType );
		}
		else if ( tag == SCH_FilterTypesTag ||				// "FilterTypesList"
				  tag == SCH_CompareOpsTag ||				// "CompareOpsList"
				  tag == SCH_CompareValsTag )				// "CompareValsList"
		{
			// NOTE that these three values need to be checked with respect to one another
			filterTypeTags = (int) scr.FilterTypesList.size();
			filterOpTags = (int) scr.CompareOpsList.size();
			filterValTags = (int) scr.CompareValsList.size();

			// the following section attempts to fix a problem encountered during testing where the search filter
			// parameter lsits contain differing numbers of parameters due to the use of the 'SinceLastRun' date filter,
			// which automatically fills-in some parameters.  The problem SHOULD NOT exist under normal conditions, but
			// this ensures lists are properly populated when submitted/stored to the DB.
#if defined(VECTOR_FIX)
			// hack for unequal vector contents... key value is the filtertypes list
			if ( filterTypeTags > 0 && ( filterTypeTags != filterOpTags || filterTypeTags != filterValTags ) )
			{
				size_t tagIdx = 0;
				bool tagFound = false;
				std::string filterOpStr = ">";
				std::string filterValStr = "'0000-00-00 00:00:00'";

				GetDbTimeString( zeroTP, filterValStr );

				WriteLogEntry( "Unequal filter vector contents detected...", WarningMsgType );

				do
				{
					tagFound = false;
					for ( size_t listIdx = tagIdx; listIdx < filterTypeTags; listIdx++ )
					{
						if ( scr.FilterTypesList.at( listIdx ) == DBApi::eListFilterCriteria::SinceLastDateFilter )
						{
							tagIdx = listIdx;		// preserve for next iteration in tag search
							tagFound = true;
							break;
						}
					}

					if ( tagFound )
					{
						if ( filterTypeTags > filterOpTags )
						{
							if ( tagIdx >= filterOpTags )
							{
								do
								{
									scr.CompareOpsList.push_back( filterOpStr );
									filterOpTags = (int) scr.CompareOpsList.size();
								} while ( tagIdx >= filterOpTags );
							}
							else
							{
								scr.CompareOpsList.insert( scr.CompareOpsList.begin() + tagIdx, filterOpStr );
								filterOpTags = (int) scr.CompareOpsList.size();
							}
						}

						if ( filterTypeTags > filterValTags )
						{
							if ( tagIdx >= filterValTags )
							{
								do
								{
									scr.CompareValsList.push_back( filterValStr );
									filterValTags = (int) scr.CompareValsList.size();
								} while ( tagIdx >= filterValTags );
							}
							else
							{
								scr.CompareValsList.insert( scr.CompareValsList.begin() + tagIdx, filterValStr );
								filterValTags = (int) scr.CompareValsList.size();
							}
						}
						tagIdx++;	// to search for the next tag
					}
				} while ( tagFound );
			}
#endif // defined(VECTOR_FIX)

			if ( filterTypeTags != filterOpTags || filterTypeTags != filterValTags )
			{
				tagsFound = MissingListItemValue;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				WriteLogEntry( "Unequal filter vector contents!", InputErrorMsgType );
			}

			for ( size_t listIdx = 0; listIdx < filterTypeTags; listIdx++ )
			{
				if ( scr.FilterTypesList.at( listIdx ) == DBApi::eListFilterCriteria::SinceLastDateFilter )
				{
					std::string lastRunTimeStr = "";

					GetDbTimeString( scr.LastSuccessRunTime, lastRunTimeStr );

					// Don't change the filter type value; it will be handled by the sample or analysis list retrieval as a RunDateFilter
					scr.CompareOpsList.at( listIdx ) = ">";
					scr.CompareValsList.at( listIdx ) = lastRunTimeStr;
				}
			}

			if ( tag == SCH_FilterTypesTag )				// "FilterTypesList"
			{
				listSize = filterTypeTags;
			}
			else if ( tag == SCH_CompareOpsTag )			// "CompareOpsList"
			{
				listSize = filterOpTags;
			}
			else if ( tag == SCH_CompareValsTag )			// "CompareValsList"
			{
				listSize = filterValTags;
			}

			if ( listSize == 0 )
			{
				valStr.clear();
				break;
			}

			if ( tag == SCH_FilterTypesTag )				// "FilterTypesList"
			{
				int32_t filterType = 0;

				std::vector<int32_t> filterList = {};

				// validate that the list of user authenticators is valid, if any have been saved
				for ( size_t i = 0; i < listSize; i++ )
				{
					filterType = static_cast<int32_t>( scr.FilterTypesList.at( i ) );
					filterList.push_back( filterType );
				}

				if ( CreateInt32ValueArrayString( valStr, "%ld", static_cast<int32_t>( listSize ), 0, filterList ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}
			else if ( tag == SCH_CompareOpsTag )			// "CompareOpsList"
			{
				std::vector<std::string> cleanList = {};

				SanitizeDataStringList( scr.CompareOpsList, cleanList );

				// filter operator strings are not quoted; this method add the single quotes
				if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listSize ), 0, cleanList ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}
			else if ( tag == SCH_CompareValsTag )			// "CompareValsList"
			{
				std::vector<std::string> cleanList = {};

				SanitizeDataStringList( scr.CompareValsList, cleanList );

				// filter values are already single quoted; this method does not add more single quotes 
				if ( CreateDataValueStringArrayString( valStr, static_cast<int32_t>( listSize ), 0, cleanList ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}
		}
		else if ( tag == SCH_EnabledTag )					// "Enabled"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( scr.Enabled == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == SCH_LastRunTimeTag )				// "LastRunTime"
		{
			// no special checks for a 'new' time point, as it is used to indicate the 'never run' condiditon;
			if (scr.LastRunTime == zeroTP)
			{
				GetDbCurrentTimeString(valStr, scr.LastRunTime);
			}
			else
			{
				GetDbTimeString(scr.LastRunTime, valStr);
			}
		}
		else if ( tag == SCH_LastSuccessRunTimeTag )		// "LastSuccessRunTime"
		{
			// no special checks for a 'new' time point, as it is used to indicate the 'never run' condiditon;
			if (scr.LastSuccessRunTime == zeroTP)
			{
				GetDbCurrentTimeString(valStr, scr.LastSuccessRunTime);
			}
			else
			{
				GetDbTimeString(scr.LastSuccessRunTime, valStr);
			}
		}
		else if ( tag == SCH_LastRunStatusTag )				// "LastRunStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.LastRunStatus );
		}
		else if ( tag == SCH_NotificationEmailTag )			// "NotificationEmail"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.NotificationEmail );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_EmailServerTag )				// "EmailServer"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.EmailServerAddr );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_EmailServerPortTag )			// "EmailServerPort"
		{
			valStr = boost::str( boost::format( "%ld" ) % scr.EmailServerPort );
		}
		else if ( tag == SCH_AuthenticateEmailTag )			// "AuthenticateEmail"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( scr.AuthenticateEmail == true ) ? TrueStr : FalseStr ) );
		}
		else if ( tag == SCH_EmailAccountTag )				// "EmailAccount"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.AccountUsername );
			SanitizeDataString( valStr );
		}
		else if ( tag == SCH_AccountAuthenticatorTag )		// "EmailAccountAuthenticator"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.AccountPwdHash );
			SanitizeDataString( valStr );
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_UPDATE, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string schemaName = "";
		std::string tableName = "";
		std::string queryStr = "";
		std::string selectTag = "";
		std::string idStr = "";

		GetSchedulerConfigQueryTag( schemaName, tableName, selectTag, idStr, scr.ConfigId, scr.ConfigIdNum );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_UPDATE );

		queryResult = RunQuery( loginType, queryStr, resultRec );
	}
	else
	{
		if ( tagsFound == 0 )
		{
			queryResult = DBApi::eQueryResult::NoTargets;
		}
		else if ( tagsFound == ItemValCreateError )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		else if ( tagsFound == MissingListItemValue )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	if ( resultRec.IsOpen() )
	{
		resultRec.Close();
	}

	loginType = SetLoginType( DBApi::eLoginType::InstrumentLoginType );		// restore standard connection type

	return queryResult;
}

