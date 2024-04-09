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



static const std::string MODULENAME = "DBif_InsertInstrument";



////////////////////////////////////////////////////////////////////////////////
// Internal Instrument workflow object insertion methods
////////////////////////////////////////////////////////////////////////////////


DBApi::eQueryResult DBifImpl::InsertWorklistRecord( DB_WorklistRecord& wlr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	bool templateSave = false;

	if ( wlr.WorklistStatus == static_cast<int32_t>( DBApi::eWorklistStatus::WorklistTemplate ) )
	{
		templateSave = true;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( wlr.WorklistId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this worklist UNLESS;
	//   the stored object was a template, and this instance is NOT marked as a template, then we issue a new uuid...
	DB_WorklistRecord dbRec = {};
	int duplicateChecks = 0;

	do
	{
		queryResult = GetWorklistInternal( dbRec, wlr.WorklistId, NO_ID_NUM, FirstLevelObjs );
		if ( queryResult == DBApi::eQueryResult::QueryOk )		// found an object with this uuid...
		{
			// check the previous status for template source...
			// if
			//     previous object was not a template
			//                  OR
			//     previous was a template AND this one wants to be a template also
			// then
			//     disallow saving this as a new object using the same uuid
			if ( dbRec.WorklistStatus != static_cast<int32_t>( DBApi::eWorklistStatus::WorklistTemplate ) || templateSave )
			{
				// don't allow saving normal objects with duplicate UUIDs
				// or saving a new template with the same UUID as a prevous template (should use update or clear the uuid...)
				wlr.WorklistId = dbRec.WorklistId;
				wlr.WorklistIdNum = dbRec.WorklistIdNum;
				return DBApi::eQueryResult::InsertObjectExists;
			}

			// generate a new UUID if previous object was a template and new object isn't
			ClearGuid( wlr.WorklistId );	// clear UUID for generation of a new uuid
			wlr.WorklistIdNum = 0;

			queryResult = GenerateValidGuid( wlr.WorklistId );
			if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
			{
				return queryResult;
			}

			if ( ++duplicateChecks > MaxDuplicateChecks )
			{
				// multiple checks with duplicate UUID... error
				return DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	} while ( queryResult != DBApi::eQueryResult::NoResults );

	wlr.WorklistIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetWorklistQueryTag( schemaName, tableName, selectTag, idStr, wlr.WorklistId );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	bool runStarting = false;
	int32_t idFails = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t updateIdx = 0;
	int32_t wlSampleSetCnt = NoItemListCnt;
	int32_t numSampleSetsTagIdx = ListCntIdxNotSet;
	size_t listCnt = 0;
	size_t listSize = 0;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// status indicates starting the list run
	if ( wlr.WorklistStatus == static_cast<int32_t>( DBApi::eWorklistStatus::WorklistRunning ) )
	{
		runStarting = true;
	}

	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == WL_IdNumTag )						// "WorklistIdNum"
		{	// the DB generates this for new insertions... no handler here, just note seeing the tag...
			tagsSeen++;
		}
		else if ( tag == WL_IdTag )						// "WorklistID"
		{
			uuid__t_to_DB_UUID_Str( wlr.WorklistId, valStr );
			tagsSeen++;
		}
		else if ( tag == WL_StatusTag )					// "WorklistStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.WorklistStatus );
			tagsSeen++;
		}
		else if ( tag == WL_NameTag )					// "WorklistName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % wlr.WorklistNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == WL_CommentsTag )				// "ListComments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % wlr.ListComments );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == WL_InstSNTag )					// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % wlr.InstrumentSNStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == WL_CreateUserIdTag )			// "CreationUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( wlr.CreationUserId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_RunUserIdTag )				// "RunUserID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.RunUserId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_RunDateTag )				// "RunDate"
		{
			if ( runStarting )          // only update run date/time if the list is also being started
			{
				GetDbCurrentTimeString( valStr, wlr.RunDateTP );
			}
			else
			{
				system_TP zeroTP = {};

				if ( wlr.RunDateTP != zeroTP )
				{
					GetDbTimeString( wlr.RunDateTP, valStr );
				}
			}
			tagsSeen++;
		}
		else if ( tag == WL_AcquireSampleTag )			// "AcquireSample"
		{
			valStr = ( wlr.AcquireSample == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == WL_CarrierTypeTag )			// "CarrierType"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.CarrierType );
			tagsSeen++;
		}
		else if ( tag == WL_PrecessionTag )				// "ByColumn"
		{
			bool precessionState = false;
			if ( wlr.CarrierType == (uint16_t) eCarrierType::ePlate_96 )
			{
				precessionState = wlr.ByColumn;
			}
			valStr = ( precessionState == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == WL_SaveImagesTag )				// "SaveImages"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.SaveNthImage );
			tagsSeen++;
		}
		else if ( tag == WL_WashTypeTag )				// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.WashTypeIndex );
			tagsSeen++;
		}
		else if ( tag == WL_DilutionTag )				// "Dilution"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.Dilution );
			tagsSeen++;
		}
		else if ( tag == WL_DfltSetNameTag )			// "DefaultSetName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % wlr.SampleSetNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == WL_DfltItemNameTag )			// "DefaultItemName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % wlr.SampleItemNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == WL_ImageAnalysisParamIdTag )	// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.ImageAnalysisParamId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_AnalysisDefIdTag )			// "AnalysisDefinitionID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.AnalysisDefId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_AnalysisDefIdxTag )			// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % wlr.AnalysisDefIndex );
			tagsSeen++;
		}
		else if ( tag == WL_AnalysisParamIdTag )		// "AnalysisParameterID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.AnalysisParamId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_CellTypeIdTag )				// "CellTypeID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.CellTypeId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_CellTypeIdxTag )			// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) wlr.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == WL_BioProcessIdTag )			// "BioProcessID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			// empty is allowed...
			if ( !GuidInsertCheck( wlr.BioProcessId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_QcProcessIdTag )			// "QcProcessID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			// empty is allowed...
			if ( !GuidInsertCheck( wlr.QcProcessId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_WorkflowIdTag )				// WorkflowID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation if not starting immediately
			if ( !GuidInsertCheck( wlr.WorkflowId, valStr, errCode, !runStarting ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == WL_SampleSetCntIdTag )			// "SampleSetCount"
		{
			// will be the dynamic number of sdample sets added to the worklist; may be '0' on list creation
			if ( wlSampleSetCnt < ItemListCntOk )	// hasn't yet been validated against the object list; don't write
			{
				numSampleSetsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % wlSampleSetCnt );
				wlSampleSetCnt = wlr.SampleSetCount;
			}
			tagsSeen++;
		}
		else if ( tag == WL_ProcessedSetCntTag )		// "ProcessedSetCount"
		{
			// will be the actual number of elements processed by the work queue on completion; '0' to start, or the number of items entered into the work queue
			wlr.ProcessedSetCount = 0;
			valStr = boost::str( boost::format( "%d" ) % wlr.ProcessedSetCount );
			tagsSeen++;
		}
		else if ( tag == WL_SampleSetIdListTag )		// "SampleSetIDList"
		{
			std::vector<uuid__t> worklistSampleSetIdList = {};
			bool ssrFound = false;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = wlr.SSList.size();
			listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( wlSampleSetCnt < ItemListCntOk )
			{
				wlSampleSetCnt = wlr.SampleSetCount;
			}

			if ( wlSampleSetCnt != listSize )
			{
				if ( wlSampleSetCnt >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( size_t i = 0; i < listCnt; i++ )
			{
				ssrFound = false;
				DB_SampleSetRecord& ssr = wlr.SSList.at( i );

				if ( templateSave )
				{
					int32_t ss_status = static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate );
					ssr.SampleSetStatus = ss_status;
					wlr.SSList.at( i ).SampleSetStatus = ss_status;
				}

				ssr.WorklistId = wlr.WorklistId;
				wlr.SSList.at( i ).WorklistId = wlr.WorklistId;

				// check if this is an existing object
				if ( GuidValid( ssr.SampleSetId ) )
				{
					DB_SampleSetRecord chk_ssr = {};

					if ( GetSampleSetInternal( chk_ssr, ssr.SampleSetId ) == DBApi::eQueryResult::QueryOk )
					{
						ssr.SampleSetIdNum = chk_ssr.SampleSetIdNum;
						ssrFound = true;
						if ( UpdateSampleSetRecord( ssr, true ) != DBApi::eQueryResult::QueryOk )
						{
							dataOk = false;		// prevent insertion of identical object...
						}
					}
				}

				if ( !ssrFound && dataOk )
				{
					ssr.SampleSetIdNum = 0;
					ClearGuid( ssr.SampleSetId );

					// saving a new object; checks are performed in the sub-object insertions for existing objects...
					insertResult = InsertSampleSetRecord( ssr, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						// retrieval for check-after-insertion is done in the individual insertion method; no need to repeat here...
						ssrFound = true;
						wlr.SSList.at( i ).SampleSetIdNum = ssr.SampleSetIdNum;		// update the idnum in the passed-in object
						wlr.SSList.at( i ).SampleSetId = ssr.SampleSetId;			// update the id in the passed-in object
					}
				}

				if ( !ssrFound )		// also gets here when idNum == 0
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( wlSampleSetCnt != listCnt )
				{
					wlSampleSetCnt = static_cast<int32_t>( listCnt );
					wlr.SampleSetCount = static_cast<int16_t>( listCnt );
				}

				if ( numSampleSetsTagIdx >= ItemTagIdxOk )		// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSampleSetsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % wlr.SampleSetCount );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk&& idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		if ( runStarting )
		{
			for ( size_t i = 0; i < listCnt; i++ )
			{
				wlr.SSList.at( i ).SampleSetStatus = static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetNotRun );
			}
		}

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetWorklistInternal( dbRec, wlr.WorklistId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				wlr.WorklistIdNum = dbRec.WorklistIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( tagsSeen < tagList.size() )
	{
		std::string logStr = "Possible missing tag handler in 'InsertWorklistRecord'";
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertWorklistSampleSetRecord( uuid__t wlid, DB_SampleSetRecord& ssr )
{
	if ( !GuidValid( wlid ) )
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

	// first try to get the parent object
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord wlRec = {};

	queryResult = GetWorklistInternal( wlRec, wlid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// check the parent obj to see if it's a template
	// if the parent is a template, the contained object MUST be a template
	if ( wlRec.WorklistStatus == static_cast<int32_t>( DBApi::eWorklistStatus::WorklistTemplate ) )
	{
		ssr.SampleSetStatus = static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate );
	}
	else if ( ssr.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate ) )
	{
		ssr.SampleSetStatus = static_cast<int32_t>( DBApi::eSampleSetStatus::NoSampleSetStatus );
	}

	// sample set should not yet exist in the DB, but if it was a template,
	// the uuid and idnum will be cleared to ensure a new object insertion...
	queryResult = InsertSampleSetRecord( ssr );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::InsertObjectExists )
		{
			return DBApi::eQueryResult::InsertFailed;
		}
	}

	size_t listCnt = wlRec.SSList.size();

	for ( size_t i = 0; i < listCnt; i++ )
	{
		DB_SampleSetRecord& ssRec = wlRec.SSList.at( i );

		if ( GuidValid( ssRec.SampleSetId ) )
		{
			// shouldn't find the id of the object being inserted...
			if ( GuidsEqual( ssr.SampleSetId, ssRec.SampleSetId ) )
			{
				return DBApi::eQueryResult::InsertObjectExists;
			}
		}
	}

	wlRec.SSList.push_back( ssr );
	wlRec.SampleSetCount = (int16_t) wlRec.SSList.size();

	return UpdateWorklistRecord( wlRec );
}

DBApi::eQueryResult DBifImpl::InsertWorklistSampleSetRecord( uuid__t wlid, uuid__t ssid, int64_t ssridnum )
{
	if ( !GuidValid( wlid ) || !GuidValid( ssid ) )
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

	// first try to get the parent object
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_WorklistRecord wlRec = {};

	queryResult = GetWorklistInternal( wlRec, wlid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// now try to get the object whose reference is being added
	DB_SampleSetRecord ssRec = {};

	queryResult = GetSampleSetInternal( ssRec, ssid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	size_t listCnt = wlRec.SSList.size();

	for ( size_t i = 0; i < listCnt; i++ )
	{
		DB_SampleSetRecord& ssRec = wlRec.SSList.at( i );

		if ( GuidValid( ssRec.SampleSetId ) )
		{
			// shouldn't find the id of the object being added...
			if ( GuidsEqual( ssid, ssRec.SampleSetId ) )
			{
				return DBApi::eQueryResult::InsertObjectExists;
			}
		}
	}

	wlRec.SSList.push_back( ssRec );
	wlRec.SampleSetCount = (int16_t) wlRec.SSList.size();

	return UpdateWorklistRecord( wlRec );
}

DBApi::eQueryResult DBifImpl::InsertSampleSetRecord( DB_SampleSetRecord& ssr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	bool templateSave = false;

	if ( ssr.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate ) )
	{
		templateSave = true;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( ssr.SampleSetId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object UNLESS;
	//   the stored object was a template, and this instance is NOT marked as a template, then we issue a new uuid...
	DB_SampleSetRecord dbRec = {};
	int duplicateChecks = 0;

	do
	{
		queryResult = GetSampleSetInternal( dbRec, ssr.SampleSetId, NO_ID_NUM, FirstLevelObjs );
		if ( queryResult == DBApi::eQueryResult::QueryOk )		// found an object with this uuid...
		{
			// check the previous status for template source...
			// if
			//     previous object was not a template
			//                  OR
			//     previous was a template AND this one wants to be a template also
			// then
			//     disallow saving this as a new object using the same uuid
			if ( dbRec.SampleSetStatus != static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate ) || templateSave )
			{
				// don't allow saving normal objects with duplicate UUIDs
				// or saving a new template with the same UUID as a prevous template (should use update or clear the uuid...)
				ssr.SampleSetId = dbRec.SampleSetId;
				ssr.SampleSetIdNum = dbRec.SampleSetIdNum;
				return DBApi::eQueryResult::InsertObjectExists;
			}

			// generate a new UUID if previous object was a template and new object isn't
			ClearGuid( ssr.SampleSetId );		// clear UUID for generation of a new uuid
			ssr.SampleSetIdNum = 0;

			queryResult = GenerateValidGuid( ssr.SampleSetId );
			if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
			{
				return queryResult;
			}

			if ( ++duplicateChecks > MaxDuplicateChecks )
			{
				// multiple checks with duplicate UUID... error
				return DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	} while ( queryResult != DBApi::eQueryResult::NoResults );

	ssr.SampleSetIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSampleSetQueryTag( schemaName, tableName, selectTag, idStr, ssr.SampleSetId, 0 );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t ssSampleItemsCnt = NoItemListCnt;
	int32_t numSsItemsTagIdx = ListCntIdxNotSet;
	size_t listCnt = 0;
	size_t listSize = 0;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

#ifdef RUN_DATE_FIX
	if ( ssr.SampleSetStatus >= static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetActive ) )			// only update run date/time if the sample-set is also being started
	{
		system_TP zeroTP = {};

		// check for upgrade insertion error condition...
		if ( ssr.RunDateTP == zeroTP &&
			 ssr.CreateDateTP != zeroTP &&
			 ( ssr.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetComplete ) ||
			   ssr.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetCanceled ) ) )
		{
			ssr.RunDateTP = ssr.CreateDateTP;
		}
	}
#endif

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == SS_IdNumTag )								// "SampleSetIdNum"
		{	// the DB generates this for new insertions... no handler here, just note seeing the tag...
			tagsSeen++;
		}
		else if ( tag == SS_IdTag )								// "SampleSetID"
		{
			uuid__t_to_DB_UUID_Str( ssr.SampleSetId, valStr );
			tagsSeen++;
		}
		else if ( tag == SS_StatusTag )							// "SampleSetStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % ssr.SampleSetStatus );
			tagsSeen++;
		}
		else if ( tag == SS_NameTag )							// "SampleSetName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssr.SampleSetNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SS_LabelTag )							// "SampleSetLabel"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssr.SampleSetLabel );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SS_CommentsTag )						// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssr.Comments );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SS_CarrierTypeTag )					// "CarrierType"
		{
			valStr = boost::str( boost::format( "%d" ) % ssr.CarrierType );
			tagsSeen++;
		}
		else if ( tag == SS_OwnerIdTag )						// "OwnerID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ssr.OwnerId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SS_CreateDateTag )						// "CreateDate"
		{
			system_TP zeroTP = {};

			if ( ssr.CreateDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, ssr.CreateDateTP );
			}
			else
			{
				GetDbTimeString( ssr.CreateDateTP, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SS_ModifyDateTag )						// "ModifyDate"
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
			tagsSeen++;
		}
		else if ( tag == SS_RunDateTag )						// "RunDate" )
		{
			system_TP zeroTP = {};

			if ( ssr.SampleSetStatus >= static_cast<int32_t>(DBApi::eSampleSetStatus::SampleSetActive) )			// only update run date/time if the sample-set is also being started
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
			else
			{
				if ( ssr.RunDateTP != zeroTP )
				{
					GetDbTimeString( ssr.RunDateTP, valStr );
				}
			}
			tagsSeen++;
		}
		else if ( tag == SS_WorklistIdTag )						// "WorklistID"
		{
			int32_t errCode = 0;

			// template sample sets may not have an owner worklist... allow empty
			if ( !GuidInsertCheck( ssr.WorklistId, valStr, errCode, templateSave ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SS_SampleItemCntTag )					// "SampleItemCount"
		{
			if ( ssSampleItemsCnt < ItemListCntOk )			// hasn't yet been validated against the object list; don't write yet
			{
				numSsItemsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % ssr.SampleItemCount );
				ssSampleItemsCnt = ssr.SampleItemCount;
			}
			tagsSeen++;
		}
		else if ( tag == SS_ProcessedItemCntTag )				// "ProcessedItemCount"
		{
			valStr = boost::str( boost::format( "%d" ) % ssr.ProcessedItemCount );
			tagsSeen++;
		}
		else if ( tag == SS_SampleItemIdListTag )				// "SampleItemIDList"
		{
			std::vector<uuid__t> SampleItemsIdList = {};
			bool ssiFound = false;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = ssr.SSItemsList.size();
			listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( ssSampleItemsCnt <= ItemListCntOk )
			{
				ssSampleItemsCnt = ssr.SampleItemCount;
			}

			if ( ssSampleItemsCnt != listCnt )
			{
				if ( ssSampleItemsCnt >= ItemListCntOk )
				{
					// indicate the need to update and add the item count to the query, if everything else is OK...
					foundCnt = SetItemListCnt;
				}
			}

			for ( size_t i = 0; i < listCnt; i++ )
			{
				ssiFound = false;
				DB_SampleItemRecord& ssir = ssr.SSItemsList.at( i );

				if ( templateSave )
				{
					int32_t si_status = static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate );
					ssir.SampleItemStatus = si_status;
					ssr.SSItemsList.at( i ).SampleItemStatus = si_status;
				}

				ssir.SampleSetId = ssr.SampleSetId;
				ssr.SSItemsList.at( i ).SampleSetId = ssr.SampleSetId;

				// check if this is an existing object
				if ( GuidValid( ssir.SampleItemId ) )
				{
					DB_SampleItemRecord chk_ssir = {};

					if ( GetSampleItemInternal( chk_ssir, ssir.SampleItemId ) == DBApi::eQueryResult::QueryOk )
					{
						ssir.SampleItemIdNum = chk_ssir.SampleItemIdNum;
						ssiFound = true;
						if ( UpdateSampleItemRecord( ssir, true ) != DBApi::eQueryResult::QueryOk )
						{
							dataOk = false;		// prevent insertion of identical object...
						}
					}
				}

				if ( !ssiFound && dataOk )
				{
					ssir.SampleItemIdNum = 0;
					ClearGuid( ssir.SampleItemId );

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertSampleItemRecord( ssir, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						ssiFound = true;
						ssr.SSItemsList.at( i ).SampleItemIdNum = ssir.SampleItemIdNum;		// update the id in the passed-in object
						ssr.SSItemsList.at( i ).SampleItemId = ssir.SampleItemId;			// update the id in the passed-in object
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
					SampleItemsIdList.push_back( ssir.SampleItemId );
				}
			}

			listCnt = SampleItemsIdList.size();

			if ( dataOk  )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, SampleItemsIdList ) != listCnt )
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( ssSampleItemsCnt != listCnt )
				{
					ssSampleItemsCnt = static_cast<int32_t>( listCnt );
					ssr.SampleItemCount = static_cast<int16_t>( listCnt );
				}

				if ( numSsItemsTagIdx >= ItemTagIdxOk )		// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSsItemsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % ssr.SampleItemCount );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( ( tagsFound > TagsOk ) && ( idFails == 0 ) )
	{
		std::string queryStr = "";
		std::string selectTag = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSampleSetInternal( dbRec, ssr.SampleSetId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				ssr.SampleSetIdNum = dbRec.SampleSetIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( tagsSeen < tagList.size() )
	{
		std::string logStr = "Possible missing tag handler in 'InsertSampleSetRecord'";
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertSampleSetSampleItemRecord( uuid__t ssid, DB_SampleItemRecord& ssir )
{
	if ( !GuidValid( ssid ) )
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

	// first try to get the parent object
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord ssRec = {};

	queryResult = GetSampleSetInternal( ssRec, ssid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// check the parent obj to see if it's a template
	// if the parent is a template, the contained object MUST be a template
	if ( ssRec.SampleSetStatus == static_cast<int32_t>( DBApi::eSampleSetStatus::SampleSetTemplate ) )
	{
		ssir.SampleItemStatus = static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate );
	}
	else if ( ssir.SampleItemStatus == static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate ) )
	{
		ssir.SampleItemStatus = static_cast<int32_t>( DBApi::eSampleItemStatus::NoItemStatus );
	}

	// sample set item does not yet exist in the DB
	queryResult = InsertSampleItemRecord( ssir );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::InsertObjectExists )
		{
			return DBApi::eQueryResult::InsertFailed;
		}
		return queryResult;
	}

	size_t listCnt = ssRec.SSItemsList.size();

	for ( size_t i = 0; i < listCnt; i++ )
	{
		DB_SampleItemRecord& ssiRec = ssRec.SSItemsList.at( i );

		if ( GuidValid( ssiRec.SampleItemId ) )
		{
			// shouldn't find the id of the object being inserted...
			if ( GuidsEqual( ssir.SampleItemId, ssiRec.SampleItemId ) )
			{
				return DBApi::eQueryResult::InsertObjectExists;
			}
		}
	}

	ssRec.SSItemsList.push_back( ssir );
	ssRec.SampleItemCount = (int16_t) ssRec.SSItemsList.size();

	return UpdateSampleSetRecord( ssRec );
}

DBApi::eQueryResult DBifImpl::InsertSampleSetSampleItemRecord( uuid__t ssid, uuid__t ssiid, int64_t ssiidnum )
{
	if ( !GuidValid( ssid ) || !GuidValid( ssiid ) )
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

	// first try to get the parent object
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_SampleSetRecord ssRec = {};

	queryResult = GetSampleSetInternal( ssRec, ssid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	// now try to get the object whose reference is being added
	DB_SampleItemRecord siRec = {};

	queryResult = GetSampleItemInternal( siRec, ssiid );
	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			queryResult = DBApi::eQueryResult::QueryFailed;
		}
		return queryResult;
	}

	size_t listCnt = ssRec.SSItemsList.size();

	for ( size_t i = 0; i < listCnt; i++ )
	{
		DB_SampleItemRecord& ssiRec = ssRec.SSItemsList.at( i );

		if ( GuidValid( ssiRec.SampleItemId ) )
		{
			// shouldn't find the id of the object being inserted...
			if ( GuidsEqual( ssiid, ssiRec.SampleItemId ) )
			{
				return DBApi::eQueryResult::InsertObjectExists;
			}
		}
	}

	ssRec.SSItemsList.push_back( siRec );
	ssRec.SampleItemCount = (int16_t) ssRec.SSItemsList.size();

	return UpdateSampleSetRecord( ssRec );
}

// Sample Set Items are the low-level work list items processed by the backend.
// They contain all information to process a sample, but are not the sample record.
DBApi::eQueryResult DBifImpl::InsertSampleItemRecord( DB_SampleItemRecord& ssir, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	bool templateSave = false;

	if ( ssir.SampleItemStatus == static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate ) )
	{
		templateSave = true;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( ssir.SampleItemId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object UNLESS;
	//   the stored object was a template, and this instance is NOT marked as a template, then we issue a new uuid...
	// should never happen for a new UUID
	DB_SampleItemRecord dbRec = {};
	int duplicateChecks = 0;

	do
	{
		queryResult = GetSampleItemInternal( dbRec, ssir.SampleItemId, NO_ID_NUM );
		if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
		{
			// check the previous status for template source...
			// if
			//     previous object was not a template
			//                  OR
			//     previous was a template AND this one wants to be a template also
			// then
			//     disallow saving this as a new object using the same uuid
			if ( dbRec.SampleItemStatus != static_cast<int32_t>( DBApi::eSampleItemStatus::ItemTemplate ) || templateSave )
			{
				// don't allow saving normal objects with duplicate UUIDs
				// or saving a new template with the same UUID as a prevous template (should use update or clear the uuid...)
				ssir.SampleItemId = dbRec.SampleItemId;
				ssir.SampleItemIdNum = dbRec.SampleItemIdNum;
				return DBApi::eQueryResult::InsertObjectExists;
			}

			// generate a new UUID if previous object was a template and new object isn't
			ClearGuid( ssir.SampleItemId );	// clear UUID for generation of a new uuid
			ssir.SampleItemIdNum = 0;

			queryResult = GenerateValidGuid( ssir.SampleItemId );
			if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
			{
				return queryResult;
			}

			if ( ++duplicateChecks > MaxDuplicateChecks )
			{
				// multiple checks with duplicate UUID... error
				return DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	} while ( queryResult != DBApi::eQueryResult::NoResults );

	ssir.SampleItemIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSampleItemQueryTag( schemaName, tableName, selectTag, idStr, ssir.SampleItemId );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == SI_IdNumTag )								// "SampleItemIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == SI_IdTag )								// "SampleItemID"
		{
			uuid__t_to_DB_UUID_Str( ssir.SampleItemId, valStr );
			tagsSeen++;
		}
		else if ( tag == SI_StatusTag )							// "SampleItemStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % ssir.SampleItemStatus );
			tagsSeen++;
		}
		else if ( tag == SI_NameTag )							// "SampleItemName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssir.SampleItemNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SI_CommentsTag )						// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssir.Comments );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SI_RunDateTag )						// "RunDate"
		{
			system_TP zeroTP = {};

			if ( ssir.SampleItemStatus >= static_cast<int32_t>( DBApi::eSampleItemStatus::ItemRunning ) )			// only update run date/time if the item is also being started
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
			else
			{
				if ( ssir.RunDateTP != zeroTP )
				{
					GetDbTimeString( ssir.RunDateTP, valStr );
				}
			}
			tagsSeen++;
		}
		else if ( tag == SI_SampleSetIdTag )					// "SampleSetID"
		{
			// may be empty or nill at the time of item creation
			// MUST be filled-in prior to processing
			if ( GuidValid( ssir.SampleSetId ) )
			{
				uuid__t_to_DB_UUID_Str( ssir.SampleSetId, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SI_SampleIdTag )						// "SampleID"
		{
			// SHOULD be empty or nill at the time of item creation, but handle existing uuid, possibly doing re-analysis...
			if ( GuidValid( ssir.SampleId ) )
			{
				uuid__t_to_DB_UUID_Str( ssir.SampleId, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SI_SaveImagesTag )						// "SaveImages"
		{
			valStr = boost::str( boost::format( "%d" ) % ssir.SaveNthImage );
			tagsSeen++;
		}
		else if ( tag == SI_WashTypeTag )						// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % ssir.WashTypeIndex );
			tagsSeen++;
		}
		else if ( tag == SI_DilutionTag )						// "Dilution"
		{
			valStr = boost::str( boost::format( "%d" ) % ssir.Dilution );
			tagsSeen++;
		}
		else if ( tag == SI_LabelTag )							// "ItemLabel"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ssir.ItemLabel );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SI_ImageAnalysisParamIdTag )			// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ssir.ImageAnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SI_AnalysisDefIdTag )					// "AnalysisDefinitionID"
		{
			// TODO: ?? check validity of pased ID???
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ssir.AnalysisDefId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SI_AnalysisDefIdxTag )					// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % ssir.AnalysisDefIndex );
			tagsSeen++;
		}
		else if ( tag == SI_AnalysisParamIdTag )				// "AnalysisParameterID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( ssir.AnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SI_CellTypeIdTag )						// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ssir.CellTypeId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SI_CellTypeIdxTag )					// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) ssir.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == SI_BioProcessIdTag )					// "BioProcessID"
		{
			// empty is allowed
			if ( GuidValid( ssir.BioProcessId ) )
			{
				uuid__t_to_DB_UUID_Str( ssir.BioProcessId, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SI_QcProcessIdTag )					// "QcProcessID"
		{
			// empty is allowed
			if ( GuidValid( ssir.QcProcessId ) )
			{
				uuid__t_to_DB_UUID_Str( ssir.QcProcessId, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SI_WorkflowIdTag )						// "WorkflowID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ssir.WorkflowId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SI_SamplePosTag )						// "SamplePosition"
		{
			valStr = boost::str( boost::format( "'%c-%02d-%02d'" ) % ssir.SampleRow % (int) ssir.SampleCol % (int) ssir.RotationCount );          // make the position string expected by the database for position; add cast to avoid boost bug for byte values...
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSampleItemInternal( dbRec, ssir.SampleItemId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				ssir.SampleItemIdNum = dbRec.SampleItemIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( tagsSeen < tagList.size() )
	{
		std::string logStr = "Possible missing tag handler in 'InsertSampleItemRecord'";
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertCellTypeRecord( DB_CellTypeRecord& ctr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}


	// if there is as existing object with this UUID or index, don't allow creation of another instance of this object
	// should never happen for a new UUID, but may happen for duplicate indices
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_CellTypeRecord dbRec = {};
	uuid__t tmpId;

	ClearGuid( tmpId );

	// first, check for duplicate index
	queryResult = GetCellTypeInternal( dbRec, tmpId, ctr.CellTypeIndex );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same index
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	queryResult = GenerateValidGuid( ctr.CellTypeId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	queryResult = GetCellTypeInternal( dbRec, ctr.CellTypeId, ctr.CellTypeIndex );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	ctr.CellTypeIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetCellTypeQueryTag( schemaName, tableName, selectTag, idStr, ctr.CellTypeId, ctr.CellTypeIndex );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	std::string tmpStr = "";
	bool dataOk = false;
	bool queryOk = false;
	int32_t updateIdx = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t identParamCnt = -1;
	int32_t numCellIdentParamsTagIdx = -1;
	int32_t specializationCnt = -1;
	int32_t numSpecializationsTagIdx = -1;
	size_t listCnt = 0;
	size_t listSize = 0;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == CT_IdNumTag )									// "CellTypeIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == CT_IdTag )									// "CellTypeID"
		{
			uuid__t_to_DB_UUID_Str( ctr.CellTypeId, valStr );
			tagsSeen++;
		}
		else if ( tag == CT_IdxTag )								// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) ctr.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == CT_NameTag )								// "CellTypeName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ctr.CellTypeNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if (tag == CT_RetiredTag)								// "Retired"
		{
			valStr = (ctr.Retired == true) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == CT_MaxImagesTag )							// "MaxImages"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.MaxImageCount );
			tagsSeen++;
		}
		else if ( tag == CT_AspirationCyclesTag )					// "AspirationCycles"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.AspirationCycles );
			tagsSeen++;
		}
		else if ( tag == CT_MinDiamMicronsTag )						// "MinDiamMicrons"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MinDiam );
			tagsSeen++;
		}
		else if ( tag == CT_MaxDiamMicronsTag )						// "MaxDiamMicrons"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MaxDiam );
			tagsSeen++;
		}
		else if ( tag == CT_MinCircularityTag )						// "MinCircularity"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.MinCircularity );
			tagsSeen++;
		}
		else if ( tag == CT_SharpnessLimitTag )						// "SharpnessLimit"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.SharpnessLimit );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == CT_CellIdentParamIdListTag )				// "CellIdentParamIDList"
		{
			std::vector<uuid__t> identParamIdList = {};
			bool apFound = false;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = ctr.CellIdentParamList.size();
			listCnt = listSize;

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

			// insert doesn't need to check to add empty array statement...
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( identParamCnt != listCnt )
				{
					identParamCnt = static_cast<int32_t>( listCnt );
					ctr.NumCellIdentParams = static_cast<int8_t>( listCnt );
				}

				if ( numCellIdentParamsTagIdx >= ItemTagIdxOk )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numCellIdentParamsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == CT_DeclusterSettingsTag )					// "DeclusterSetting"
		{
			valStr = boost::str( boost::format( "%d" ) % ctr.DeclusterSetting );
			tagsSeen++;
		}
//		else if ( tag == CT_POIIdentParamTag )						// "POIIdentParam"
//		{
//			// TODO: determine what this will be and the proper container...
//			tagsSeen++;
//		}
		else if ( tag == CT_RoiExtentTag )							// "RoiExtent"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.RoiExtent );
			tagsSeen++;
		}
		else if ( tag == CT_RoiXPixelsTag )							// "RoiXPixels"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.RoiXPixels );
			tagsSeen++;
		}
		else if ( tag == CT_RoiYPixelsTag )							// "RoiYPixels"
		{
			valStr = boost::str( boost::format( "%u" ) % ctr.RoiYPixels );
			tagsSeen++;
		}
		else if ( tag == CT_NumAnalysisSpecializationsTag )			// "NumAnalysisSpecializations"
		{
			if ( specializationCnt <= 0 )   // hasn't yet been validated against the object list; don't write
			{
				numSpecializationsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % ctr.NumAnalysisSpecializations );
				specializationCnt = ctr.NumAnalysisSpecializations;
			}
			tagsSeen++;
		}
		else if ( tag == CT_AnalysisSpecializationIdListTag )		// "AnalysisSpecializationIDList"
		{
			std::vector<uuid__t> specializationsDefIdList = {};
			bool adFound = false;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = ctr.SpecializationsDefList.size();
			listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( specializationCnt <= 0 )
			{
				specializationCnt = ctr.NumAnalysisSpecializations;
			}

			if ( specializationCnt != listSize )
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( specializationCnt != listCnt )
				{
					specializationCnt = static_cast<int32_t>( listCnt );
					ctr.NumAnalysisSpecializations = static_cast<int32_t>( listCnt );
				}

				if ( numSpecializationsTagIdx >= ItemTagIdxOk )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numSpecializationsTagIdx ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == CT_CalcCorrectionFactorTag )				// "CalculationAdjustmentFactor"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % ctr.CalculationAdjustmentFactor );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			valStr = ( ctr.Protected == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			retrieveResult = GetCellTypeInternal( dbRec, ctr.CellTypeId, ctr.CellTypeIndex );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				ctr.CellTypeIdNum = dbRec.CellTypeIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
	}
	else
	{
		if ( tagsFound < TagsOk )
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	if ( tagsSeen < tagList.size() )
	{
		std::string logStr = "Possible missing tag handler in 'InsertSampleRecord'";
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& params )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( params.ParamId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_ImageAnalysisParamRecord dbRec = {};

	queryResult = GetImageAnalysisParamInternal( dbRec, params.ParamId );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetImageAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, params.ParamId, params.ParamIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == IAP_IdNumTag )													// "ImageAnalysisParamIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == IAP_IdTag )												// "ImageAnalysisParamID"
		{
			uuid__t_to_DB_UUID_Str( params.ParamId, valStr );
			tagsSeen++;
		}
		else if ( tag == IAP_AlgorithmModeTag )										// "AlgorithmMode"
		{
			valStr = boost::str( boost::format( "%d" ) % params.AlgorithmMode );
			tagsSeen++;
		}
		else if ( tag == IAP_BubbleModeTag )										// "BubbleMode"
		{
			valStr = ( params.BubbleMode == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterModeTag )										// "DeclusterMode"
		{
			valStr = ( params.DeclusterMode == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == IAP_SubPeakAnalysisModeTag )								// "SubPeakAnalysisMode"
		{
			valStr = ( params.SubPeakAnalysisMode == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == IAP_DilutionFactorTag )									// "DilutionFactor"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DilutionFactor );
			tagsSeen++;
		}
		else if ( tag == IAP_ROIXcoordsTag )										// "ROIXcoords"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ROI_Xcoords );
			tagsSeen++;
		}
		else if ( tag == IAP_ROIYcoordsTag )										// "ROIYcoords"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ROI_Ycoords );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterAccumuLowTag )								// "DeclusterAccumulatorThreshLow"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterAccumulatorThreshLow );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterMinDistanceLowTag )							// "DeclusterMinDistanceThreshLow"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterMinDistanceThreshLow );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterAccumuMedTag )								// "DeclusterAccumulatorThreshMed"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterAccumulatorThreshMed );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterMinDistanceMedTag )							// "DeclusterMinDistanceThreshMed"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterMinDistanceThreshMed );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterAccumuHighTag )								// "DeclusterAccumulatorThreshHigh"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterAccumulatorThreshHigh );
			tagsSeen++;
		}
		else if ( tag == IAP_DeclusterMinDistanceHighTag )							// "DeclusterMinDistanceThreshHigh"
		{
			valStr = boost::str( boost::format( "%d" ) % params.DeclusterMinDistanceThreshHigh );
			tagsSeen++;
		}
		else if ( tag == IAP_FovDepthTag )											// "FovDepthMM"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FovDepthMM );
			tagsSeen++;
		}
		else if ( tag == IAP_PixelFovTag )											// "PixelFovMM"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.PixelFovMM );
			tagsSeen++;
		}
		else if ( tag == IAP_SizingSlopeTag )										// "SizingSlope"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SizingSlope );
			tagsSeen++;
		}
		else if ( tag == IAP_SizingInterceptTag )									// "SizingIntercept"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SizingIntercept );
			tagsSeen++;
		}
		else if ( tag == IAP_ConcSlopeTag )											// "ConcSlope"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ConcSlope );
			tagsSeen++;
		}
		else if ( tag == IAP_ConcInterceptTag )										// "ConcIntercept"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ConcIntercept );
			tagsSeen++;
		}
		else if ( tag == IAP_ConcImageControlCntTag )								// "ConcImageControlCnt"
		{
			valStr = boost::str( boost::format( "%d" ) % params.ConcImageControlCount );
			tagsSeen++;
		}
		else if ( tag == IAP_BubbleMinSpotAreaPrcntTag )							// "BubbleMinSpotAreaPrcnt"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleMinSpotAreaPercent );
			tagsSeen++;
		}
		else if ( tag == IAP_BubbleMinSpotAreaBrightnessTag )						// "BubbleMinSpotAreaBrightness"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleMinSpotAvgBrightness );
			tagsSeen++;
		}
		else if ( tag == IAP_BubbleRejectImgAreaPrcntTag )							// "BubbleRejectImgAreaPrcnt"
		{
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % params.BubbleRejectImageAreaPercent );
			tagsSeen++;
		}
		else if ( tag == IAP_VisibleCellSpotAreaTag )								// "VisibleCellSpotArea"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.VisibleCellSpotArea );
			tagsSeen++;
		}
		else if ( tag == IAP_FlScalableROITag )										// "FlScalableROI"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FlScalableROI );
			tagsSeen++;
		}
		else if ( tag == IAP_FLPeakPercentTag )										// "FLPeakPercent"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.FlPeakPercent );
			tagsSeen++;
		}
		else if ( tag == IAP_NominalBkgdLevelTag )									// "NominalBkgdLevel"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.NominalBkgdLevel );
			tagsSeen++;
		}
		else if ( tag == IAP_BkgdIntensityToleranceTag )							// "BkgdIntensityTolerance"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.BkgdIntensityTolerance );
			tagsSeen++;
		}
		else if ( tag == IAP_CenterSpotMinIntensityTag )							// "CenterSpotMinIntensityLimit"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.CenterSpotMinIntensityLimit );
			tagsSeen++;
		}
		else if ( tag == IAP_PeakIntensitySelectionAreaTag )						// "PeakIntensitySelectionAreaLimit"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.PeakIntensitySelectionAreaLimit );
			tagsSeen++;
		}
		else if ( tag == IAP_CellSpotBrightnessExclusionTag )						// "CellSpotBrightnessExclusionThreshold"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.CellSpotBrightnessExclusionThreshold );
			tagsSeen++;
		}
		else if ( tag == IAP_HotPixelEliminationModeTag )							// "HotPixelEliminationMode"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.HotPixelEliminationMode );
			tagsSeen++;
		}
		else if ( tag == IAP_ImgBotAndRightBoundaryModeTag )						// "ImgBotAndRightBoundaryAnnotationMode"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.ImgBottomAndRightBoundaryAnnotationMode );
			tagsSeen++;
		}
		else if ( tag == IAP_SmallParticleSizeCorrectionTag )						// "SmallParticleSizingCorrection"
		{
			valStr = boost::str( boost::format( DbDoubleDataFmtStr ) % params.SmallParticleSizingCorrection );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetImageAnalysisParamInternal( dbRec, params.ParamId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				params.ParamIdNum = dbRec.ParamIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

#ifdef HANDLE_TRANSACTIONS
	CancelTransaction();
#endif // HANDLE_TRANSACTIONS

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& params, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( params.SettingsId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_AnalysisInputSettingsRecord dbRec = {};

	queryResult = GetAnalysisInputSettingsInternal( dbRec, params.SettingsId );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetAnalysisInputSettingsQueryTag( schemaName, tableName, selectTag, idStr, params.SettingsId, params.SettingsIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == AIP_IdNumTag )					// "AnalysisParamIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == AIP_IdTag )					//"AnalysisParamID" )
		{
			uuid__t_to_DB_UUID_Str( params.SettingsId, valStr );
			tagsSeen++;
		}
		else if ( tag == AIP_ConfigParamMapTag )
		{
			size_t mapSize = params.InputConfigParamMap.size();

			if ( CreateConfigParamMapValueArrayString( valStr, (int32_t) mapSize, params.InputConfigParamMap ) != mapSize )
			{
				tagsFound = OperationFailure;
				dataOk = false;
				valStr.clear();
				queryResult = DBApi::eQueryResult::InsertFailed;
				break;          // break out of this 'for' loop
			}
			tagsSeen++;
		}
		else if ( tag == AIP_CellIdentParamListTag )
		{
			size_t listSize = params.CellIdentParamList.size();

			if ( CreateCellIdentParamArrayString( valStr, (int32_t) listSize, params.CellIdentParamList ) != listSize )
			{
				tagsFound = OperationFailure;
				dataOk = false;
				valStr.clear();
				queryResult = DBApi::eQueryResult::InsertFailed;
				break;          // break out of this 'for' loop
			}
			tagsSeen++;
		}
		else if ( tag == AIP_PoiIdentParamListTag )
		{
			size_t listSize = params.POIIdentParamList.size();

			if ( CreateCellIdentParamArrayString( valStr, (int32_t) listSize, params.POIIdentParamList ) != listSize )
			{
				tagsFound = OperationFailure;
				dataOk = false;
				valStr.clear();
				queryResult = DBApi::eQueryResult::InsertFailed;
				break;          // break out of this 'for' loop
			}
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetAnalysisInputSettingsInternal( dbRec, params.SettingsId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				params.SettingsIdNum = dbRec.SettingsIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& def, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( def.AnalysisDefId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_AnalysisDefinitionRecord dbRec = {};

	queryResult = GetAnalysisDefinitionInternal( dbRec, def.AnalysisDefId, def.AnalysisDefIndex, def.AnalysisDefIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	def.AnalysisDefIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetAnalysisDefinitionQueryTag( schemaName, tableName, selectTag, idStr, def.AnalysisDefId, INVALID_INDEX, def.AnalysisDefIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	std::string tmpStr = "";
	bool dataOk = false;
	bool queryOk = false;
	bool apFound = false;
	int32_t updateIdx = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
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
	size_t listSize = 0;
	int64_t idNum = 0;
	int64_t populationParamIdNum = 0;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::NotConnected;

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

		if ( tag == AD_IdNumTag )						// "AnalysisDefinitionIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == AD_IdTag )						// "AnalysisDefinitionID"
		{
			uuid__t_to_DB_UUID_Str( def.AnalysisDefId, valStr );
			tagsSeen++;
		}
		else if ( tag == AD_IdxTag )					// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%u" ) % def.AnalysisDefIndex );
			tagsSeen++;
		}
		else if ( tag == AD_NameTag )					// "AnalysisDefinitionName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % def.AnalysisLabel );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == AD_NumReagentsTag )			// "NumReagents"
		{
			if ( numReagents <= 0 )     // hasn't yet been validated against the object list; don't write
			{
				numReagentsTagIndex = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % (int16_t) def.NumReagents );		// cast to avoid boost bug for byte values...
				numReagents = def.NumReagents;
			}
			tagsSeen++;
		}
		else if ( tag == AD_ReagentTypeIdxListTag )		// "ReagentTypeIndexList"
		{
			std::vector<int32_t> reagentIndexList = {};
			int32_t indexVal = 0;

			tagsSeen++;
			listSize = def.ReagentIndexList.size();
			listCnt = listSize;

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
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}

			if ( numReagents != listCnt )
			{
				numReagents = static_cast<int32_t>( listCnt );
				def.NumReagents = static_cast<int8_t>( listCnt );
			}
		}
		else if ( tag == AD_MixingCyclesTag )			// "MixingCycles"
		{
			valStr = boost::str( boost::format( "%d" ) % def.MixingCycles );
			tagsSeen++;
		}
		else if ( tag == AD_NumIlluminatorsTag )		// "NumIlluminators"
		{
			if ( numIlluminators <= 0 )         // hasn't yet been validated against the object list; don't write
			{
				numIlluminatorsTagIndex = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) numIlluminators );		// cast to avoid boost bug for byte values...
				numIlluminators = def.NumFlIlluminators;
			}
			tagsSeen++;
		}
		else if ( tag == AD_IlluminatorIdxListTag )		// "IlluminatorsIndexList"
		{
			std::vector<int16_t> ilIndexList = {};
			int16_t idx = INVALID_INDEX;
			std::string ilname = "";
			bool ilrFound = false;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = def.IlluminatorsList.size();
			listCnt = listSize;

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
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
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
				numAnalysisParamsTagIndex = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % (int16_t) numAnalysisParams );		// cast to avoid boost bug for byte values...
				numAnalysisParams = def.NumAnalysisParams;
			}
			tagsSeen++;
		}
		else if ( tag == AD_AnalysisParamIdListTag )		// "AnalysisParamIDList" )
		{
			std::vector<uuid__t> analysisParamIdList = {};

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = def.AnalysisParamList.size();
			listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( numAnalysisParams <= ItemListCntOk )
			{
				numAnalysisParams = def.NumAnalysisParams;
			}

			if ( numAnalysisParams != listSize )
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
						apFound = true;
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( numAnalysisParams != listCnt )
				{
					numAnalysisParams = static_cast<int32_t>( listCnt );
					def.NumAnalysisParams = static_cast<int8_t>( listCnt );
				}

				if ( numAnalysisParamsTagIndex > 0 )     // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numAnalysisParamsTagIndex ) );      // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == AD_PopulationParamIdTag )			// "PopulationParamID"
		{
			if ( def.PopulationParamExists )
			{
				DB_AnalysisParamRecord& pp = def.PopulationParam;
				bool ppFound = false;

				tagsSeen++;

				// checke if this is an existing object
				if ( GuidValid( pp.ParamId ) )
				{
					DB_AnalysisParamRecord chk_pp = {};

					if ( GetAnalysisParamInternal( chk_pp, pp.ParamId ) == DBApi::eQueryResult::QueryOk )
					{
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

				if ( !apFound )     // also gets here when idNum == 0
				{
					foundCnt = ListItemNotFound;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
				else
				{
					uuid__t_to_DB_UUID_Str( pp.ParamId, valStr );
				}
			}
		}
		else if ( tag == AD_PopulationParamExistsTag )
		{
			valStr = ( def.PopulationParamExists == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )
		{
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetAnalysisDefinitionInternal( dbRec, def.AnalysisDefId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				def.AnalysisDefIdNum = dbRec.AnalysisDefIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
	}
	else
	{
		if ( tagsFound < TagsOk )
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertAnalysisParamRecord( DB_AnalysisParamRecord& params, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( params.ParamId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_AnalysisParamRecord dbRec = {};

	queryResult = GetAnalysisParamInternal( dbRec, params.ParamId );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetAnalysisParamQueryTag( schemaName, tableName, selectTag, idStr, params.ParamId, params.ParamIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == AP_IdNumTag )					// "AnalysisParamIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == AP_IdTag )					//"AnalysisParamID" )
		{
			uuid__t_to_DB_UUID_Str( params.ParamId, valStr );
			tagsSeen++;
		}
		else if ( tag == AP_InitializedTag )		// IsInitialized"
		{
			valStr = ( params.IsInitialized == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == AP_LabelTag )				// "AnalysisParamLabel"
		{
			valStr = boost::str( boost::format( "'%s'" ) % params.ParamLabel );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == AP_KeyTag )				// "CharacteristicKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.key );
			tagsSeen++;
		}
		else if ( tag == AP_SKeyTag )				// "CharacteristicSKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.s_key );
			tagsSeen++;
		}
		else if ( tag == AP_SSKeyTag )				// "CharacteristicSSKey"
		{
			valStr = boost::str( boost::format( "%u" ) % params.Characteristics.s_s_key );
			tagsSeen++;
		}
		else if ( tag == AP_ThreshValueTag )		// "ThresholdValue"
		{
			valStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % params.ThresholdValue );
			tagsSeen++;
		}
		else if ( tag == AP_AboveThreshTag )		// "AboveThreshold"
		{
			valStr = ( params.AboveThreshold == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetAnalysisParamInternal( dbRec, params.ParamId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				params.ParamIdNum = dbRec.ParamIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertIlluminatorRecord( DB_IlluminatorRecord& ilr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	// must have selected an illuminator index or name
	if ( ilr.IlluminatorIndex < 0 && ilr.IlluminatorNameStr.length() == 0 )
	{
		WriteLogEntry( "InsertIlluminatorRecord: failed: no valid identifier supplied", InputErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_IlluminatorRecord dbRec = {};
	int64_t chkIdNum = NO_ID_NUM;

	// at this point idnum should not be set for a new insertion, but positive values will be checked against the db
	if ( ilr.IlluminatorIdNum > 0 )
	{
		chkIdNum = ilr.IlluminatorIdNum;
	}

	// if there is as existing object with this idnum, index, or other characteristics,
	// don't allow creation of another instance of this object
	// check if this is an existing object
	queryResult = GetIlluminatorInternal( dbRec, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, chkIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult == DBApi::eQueryResult::NoResults )
	{
		// not found by index and name; check by emissions and illuminator wavelengths... 
		queryResult = GetIlluminatorInternal( dbRec, ilr.EmissionWavelength, ilr.IlluminatorWavelength );
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			return DBApi::eQueryResult::InsertObjectExists;
		}
	}

	if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	// get the appropriate table and schema, and the field used to check for insertion duplicates
	// one or more of the id fields should be valid at this point; check each, and on failure, check the other before failing
	// check idnum first
	if ( !GetIlluminatorQueryTag( schemaName, tableName, selectTag, idStr, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, ilr.IlluminatorIdNum ) )
	{
		// idnum not valid; check index next
		if ( !GetIlluminatorQueryTag( schemaName, tableName, selectTag, idStr, ilr.EmissionWavelength, ilr.IlluminatorWavelength ) )
		{
			WriteLogEntry( "InsertIlluminatorRecord: failed: no valid identifier tags", InputErrorMsgType );
			return DBApi::eQueryResult::BadQuery;
		}
	}

	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == IL_IdNumTag )					// "IlluminatorIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == IL_IdxTag )				// "IlluminatorIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorIndex );
			tagsSeen++;
		}
		else if ( tag == IL_TypeTag )				// "IlluminatorType"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorType );
			tagsSeen++;
		}
		else if ( tag == IL_NameTag )				// "IlluminatorName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ilr.IlluminatorNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == IL_PosNumTag )				// "PositionNum"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.PositionNum );
			tagsSeen++;
		}
		else if ( tag == IL_ToleranceTag )			// "Tolerance"
		{
			valStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % ilr.Tolerance );
			tagsSeen++;
		}
		else if ( tag == IL_MaxVoltageTag )			// "MaxVoltage"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.MaxVoltage );
			tagsSeen++;
		}
		else if ( tag == IL_IllumWavelengthTag )	// "IlluminatorWavelength"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.IlluminatorWavelength );
			tagsSeen++;
		}
		else if ( tag == IL_EmitWavelengthTag )		// "EmissionWavelength"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.EmissionWavelength );
			tagsSeen++;
		}
		else if ( tag == IL_ExposureTimeMsTag )		// "ExposureTimeMs"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.ExposureTimeMs );
			tagsSeen++;
		}
		else if ( tag == IL_PercentPowerTag )		// "PercentPower"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.PercentPower );
			tagsSeen++;
		}
		else if ( tag == IL_SimmerVoltageTag )		// "SimmerVoltage"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.SimmerVoltage );
			tagsSeen++;
		}
		else if ( tag == IL_LtcdTag )				// "Ltcd"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.Ltcd );
			tagsSeen++;
		}
		else if ( tag == IL_CtldTag )				// "Ctld"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.Ctld );
			tagsSeen++;
		}
		else if ( tag == IL_FeedbackDiodeTag )		// "FeedbackPhotoDiode"
		{
			valStr = boost::str( boost::format( "%d" ) % ilr.FeedbackDiode );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		uuid__t tmpUuid = {};
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		ClearGuid( tmpUuid );
		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item to update the idnumber...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;
			DB_IlluminatorRecord chk_ilr = {};
			bool ilrOk = false;

			// check if this is an existing object
			retrieveResult = GetIlluminatorInternal( chk_ilr, ilr.IlluminatorNameStr, ilr.IlluminatorIndex, chkIdNum );
			if ( retrieveResult != DBApi::eQueryResult::QueryOk )
			{
				if ( chk_ilr.EmissionWavelength == ilr.EmissionWavelength &&
					 chk_ilr.IlluminatorWavelength == ilr.IlluminatorWavelength &&
					 chk_ilr.ExposureTimeMs == ilr.ExposureTimeMs &&
					 chk_ilr.IlluminatorNameStr == ilr.IlluminatorNameStr &&
					 chk_ilr.IlluminatorIndex == ilr.IlluminatorIndex )
				{
					ilrOk = true;
				}
			}

			if ( !ilrOk )
			{
				retrieveResult = GetIlluminatorInternal( chk_ilr, ilr.EmissionWavelength, ilr.IlluminatorWavelength );
				if ( retrieveResult == DBApi::eQueryResult::QueryOk )
				{
					if ( chk_ilr.EmissionWavelength == ilr.EmissionWavelength &&
						 chk_ilr.IlluminatorWavelength == ilr.IlluminatorWavelength &&
						 chk_ilr.ExposureTimeMs == ilr.ExposureTimeMs &&
						 chk_ilr.IlluminatorNameStr == ilr.IlluminatorNameStr &&
						 chk_ilr.IlluminatorIndex == ilr.IlluminatorIndex )
					{
						ilrOk = true;
					}
				}
			}

			if ( ilrOk )
			{
				ilr.IlluminatorIdNum = chk_ilr.IlluminatorIdNum;
			}
			else
			{
				ilr.IlluminatorIdNum = 0;
			}

			queryResult = retrieveResult;
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertUserRecord( DB_UserRecord& ur )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( ur.UserId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	DB_UserRecord dbRec = {};
	uuid__t tmpId = {};
	DBApi::eUserType userType = DBApi::eUserType::AllUsers;

	// FIRST: check for a duplicate name
	if ( ur.ADUser )
	{
		userType = DBApi::eUserType::AdUsers;
	}
	else
	{
		userType = DBApi::eUserType::LocalUsers;
	}

	queryResult = GetUserInternal( dbRec, tmpId, ur.UserNameStr, userType );
	if ( queryResult == DBApi::eQueryResult::QueryOk ||					// found existing object with same name
		 queryResult == DBApi::eQueryResult::MultipleObjectsFound )		// found multiple users with this name and user type; that's bad...
	{
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	// Now check if there is an existing object with this UUID; don't allow creation of another instance of this object using the same UUID
	// should never happen for a new UUID
	std::string blankName = "";

	queryResult = GetUserInternal( dbRec, ur.UserId, blankName );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	ur.UserIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetUserQueryTag( schemaName, tableName, selectTag, idStr, ur.UserId, ur.UserIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagIndex = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t numCellTypes = -1;
	int32_t numCellTypesTagIdx = -1;
	int32_t numProps = -1;
	int32_t numPropsTagIdx = -1;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == UR_IdNumTag )							// "UserIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == UR_IdTag )							// "UserID"
		{
			uuid__t_to_DB_UUID_Str( ur.UserId, valStr );
			tagsSeen++;
		}
		else if ( tag == UR_RetiredTag )					// "Retired"
		{
			valStr = ( ur.Retired == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == UR_ADUserTag )						// "ADUser"
		{
			valStr = ( ur.ADUser == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == UR_RoleIdTag )						// "RoleID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( ur.RoleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == UR_UserNameTag )					// "UserName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.UserNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_DisplayNameTag )				// "DisplayName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DisplayNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_CommentsTag )					// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.Comment );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_UserEmailTag )					// "UserEmail"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.UserEmailStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_AuthenticatorListTag )			// "AuthenticatorList"
		{
			size_t listSize = 0;
			size_t listCnt = 0;

			tagsSeen++;

			listSize = ur.AuthenticatorList.size();
			if ( listSize == 0 )	// MUST have at least 1 authenticator...
			{
				dataOk = false;
				foundCnt = NoItemListCnt;
				queryResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				break;  // break out of the tag 'for' loop...
			}

			foundCnt = tagsFound;
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

				if ( CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listSize ), 0, cleanList ) != listSize )
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

			if (ChronoUtilities::IsZero(ur.AuthenticatorDateTP))
			{ // for newly created records set the authenticator date to '0'
				GetDbTimeString(zeroTP, valStr);
			}
			else
			{
				GetDbTimeString (ur.AuthenticatorDateTP, valStr);
			}
			tagsSeen++;
		}
		else if ( tag == UR_LastLoginTag )					// "LastLogin"
		{
			system_TP zeroTP = {};

			if (ChronoUtilities::IsZero(ur.AuthenticatorDateTP))
			{ // for newly created records set the authenticator date to '0'
				GetDbTimeString(zeroTP, valStr);
			}
			else
			{
				GetDbTimeString (ur.LastLogin, valStr);
			}
			tagsSeen++;
		}
		else if ( tag == UR_AttemptCntTag )					// "AttemptCount"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.AttemptCount );
			tagsSeen++;
		}
		else if ( tag == UR_LanguageCodeTag )				// "LanguageCode"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.LanguageCode );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_DfltSampleNameTag )				// "DefaultSampleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DefaultSampleNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_UserImageSaveNTag )				// "SaveNthIImage"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.UserImageSaveN );
			tagsSeen++;
		}
		else if ( tag == UR_DisplayColumnsTag )				// "DisplayColumns"
		{
			size_t listSize = 0;

			tagsSeen++;

			listSize = ur.ColumnDisplayList.size();

			if ( listSize > 0 )		// if the list is empty, the display should be using the defaults...
			{
				int32_t listCnt = static_cast<int32_t>( listSize );

				foundCnt = tagsFound;
				valStr.clear();

				if ( CreateColumnDisplayInfoArrayString( valStr, static_cast<int32_t>( listCnt ), ur.ColumnDisplayList, true ) != listCnt )
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
			tagsSeen++;
		}
		else if ( tag == UR_ExportFolderTag		)			// "ExportFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.ExportFolderStr );
			SanitizePathString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_DfltResultFileNameStrTag )		// "DefaultResultFileName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.DefaultResultFileNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_CSVFolderTag )					// "CSVFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ur.CSVFolderStr );
			SanitizePathString( valStr );
			tagsSeen++;
		}
		else if ( tag == UR_PdfExportTag )					// "PdfExport"
		{
			valStr = ( ur.PdfExport == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == UR_AllowFastModeTag )				// "AllowFastMode"
		{
			valStr = ( ur.AllowFastMode == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == UR_WashTypeTag )					// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.WashType );
			tagsSeen++;
		}
		else if ( tag == UR_DilutionTag )					// "Dilution"
		{
			valStr = boost::str( boost::format( "%d" ) % ur.Dilution );
			tagsSeen++;
		}
		else if ( tag == UR_DefaultCellTypeIdxTag )			// "DefaultCellTypeIndex";
		{
			valStr = boost::str( boost::format( "%d" ) % ur.DefaultCellType );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == UR_CellTypeIdxListTag )		// "UserCellTypeIndexList"
		{
			std::vector<int32_t> ctIndexList = {};
			int32_t indexVal = 0;
			int64_t tmpIdx = 0;
			size_t listSize = 0;
			size_t listCnt = 0;
			bool ctFound = false;

			tagsSeen++;
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
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
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
			tagsSeen++;

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
				valStr = boost::str( boost::format( "%d" ) % (int16_t)numProps );		// cast to avoid boost bug for byte values...
				numProps = ur.NumUserProperties;
			}
			tagsSeen++;
		}
		else if ( tag == UR_UserPropsIdxListTag )		// "UserPropertiesIndexList"
		{
			std::vector<int16_t> upIndexList = {};
			int64_t propIdNum = 0;
			int16_t propIdx = INVALID_INDEX;
			std::string propName = "";
			size_t listSize = 0;
			size_t listCnt = 0;
			bool upFound = false;
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::NotConnected;

			tagsSeen++;
			foundCnt = tagsFound;
			listSize = ur.UserPropertiesList.size();
//			listSize = ur.UserPropertyIndexList.size();
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

					DB_UserPropertiesRecord chk_up = {};

					retrieveResult = GetUserPropertyInternal( chk_up, propIdx, propName, propIdNum );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
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
						}
					}
				}

				if ( !upFound && dataOk )
				{
					DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;
					up.PropertiesIdNum = 0;

					insertResult = InsertUserPropertyRecord( up, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						// retrieval for check-after-insertion is done in the individual insertion method; no need to repeat here...
						upFound = true;
						ur.UserPropertiesList.at( i ).PropertiesIdNum = up.PropertiesIdNum;		// update the idnum in the passed-in object
						propIdx = up.PropertyIndex;
					}
					else
					{
						dataOk = false;
						queryResult = DBApi::eQueryResult::InsertFailed;
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
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}   // else if ( tag == "UserPropertiesIndexList" )
		else if ( tag == UR_AppPermissionsTag )			// "AppPermissions"
		{
			valStr = boost::str( boost::format( "%lld" ) % ( (int64_t) ur.AppPermissions ) );
			tagsSeen++;
		}
		else if ( tag == UR_AppPermissionsHashTag )		// "AppPermissionsHash"
		{
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == UR_InstPermissionsHashTag )	// "InstrumentPermissionsHash"
		{
			tagsSeen++;
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
			tagsSeen++;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}   // for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetUserInternal( dbRec, ur.UserId, ur.UserNameStr );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				ur.UserIdNum = dbRec.UserIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertRoleRecord( DB_UserRoleRecord& rr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( rr.RoleId );
	if ( queryResult != DBApi::eQueryResult::QueryOk ) 		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_UserRoleRecord dbRec = {};

	queryResult = GetRoleInternal( dbRec, rr.RoleId, rr.RoleNameStr );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	rr.RoleIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetRoleQueryTag( schemaName, tableName, selectTag, idStr, rr.RoleId, rr.RoleIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == RO_IdNumTag )					// "RoleIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == RO_IdTag )					// "RoleID"
		{
			uuid__t_to_DB_UUID_Str( rr.RoleId, valStr );
			tagsSeen++;
		}
		else if ( tag == RO_NameTag )				// "RoleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rr.RoleNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == RO_RoleTypeTag )			// "RoleType"
		{
			valStr = boost::str( boost::format( "%d" ) % rr.RoleType );
			tagsSeen++;
		}
		else if ( tag == RO_GroupMapListTag )		// "GroupMapList"
		{
			size_t listSize = 0;

			tagsSeen++;

			listSize = rr.GroupMapList.size();
			if ( listSize == 0 )	// May be empty on non-networked systems
			{
				break;  // break out of the tag 'for' loop...
			}

			std::vector<std::string> groupMapList = {};
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
		else if ( tag == RO_CellTypeIdxListTag )		// "UserCellTypeIndexList"
		{
			std::vector<int32_t> ctIndexList = {};
			int32_t indexVal = 0;
			int64_t tmpIdx = 0;
			size_t listSize = 0;
			size_t listCnt = 0;
			bool ctFound = false;

			tagsSeen++;
			listSize = rr.CellTypeIndexList.size();
			listCnt = listSize;

			valStr.clear();

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				indexVal = (int32_t)rr.CellTypeIndexList.at( i );
				tmpIdx = (int64_t) rr.CellTypeIndexList.at( i );
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
				uuid__t tmpId = {};

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
			valStr = boost::str( boost::format( "%ld" ) % ((int32_t) rr.InstrumentPermissions) );
			tagsSeen++;
		}
		else if ( tag == RO_AppPermissionsTag )		// "AppPermisions"
		{
			valStr = boost::str( boost::format( "%ld" ) % ((int32_t) rr.ApplicationPermissions) );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
//			valStr = (rr.Protected == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetRoleInternal( dbRec, rr.RoleId, rr.RoleNameStr );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				rr.RoleIdNum = dbRec.RoleIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertUserPropertyRecord( DB_UserPropertiesRecord& up, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	// must have selected an index or name
	if ( up.PropertyIndex < 0 && up.PropertyNameStr.length() == 0 )
	{
		WriteLogEntry( "InsertUserPropertyRecord: failed: no index or name", InputErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_UserPropertiesRecord dbRec = {};
	int64_t chkIdNum = NO_ID_NUM;

	// at this point idnum should not be set for a new insertion, but positive values will be checked against the db
	if ( up.PropertiesIdNum > 0 )
	{
		chkIdNum = up.PropertiesIdNum;
	}

	// if there is as existing object with this idnum, index, or other characteristics,
	// don't allow creation of another instance of this object
	// check if this is an existing object
	queryResult = GetUserPropertyInternal( dbRec, up.PropertyIndex, up.PropertyNameStr, chkIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	// get the appropriate table and schema, and the field used to check for insertion duplicates
	// one or more of the id fields should be valid at this point; check each, and on failure, check the other before failing
	// check idnum first
	if ( !GetUserPropertyQueryTag( schemaName, tableName, selectTag, idStr, up.PropertyIndex, up.PropertyNameStr, chkIdNum ) )
	{
		WriteLogEntry( "InsertUserPropertyRecord: failed: no query identifier tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == UP_IdNumTag )				// "PropertyIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == UP_IdxTag )			// "PropertyIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % up.PropertyIndex );
			tagsSeen++;
		}
		else if ( tag == UP_NameTag )			// "PropertyName" )
		{
			valStr = boost::str( boost::format( "'%s'" ) % up.PropertyNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == UP_TypeTag )			// "PropertyType" )
		{
			valStr = boost::str( boost::format( "%d" ) % up.PropertyType );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
//			valStr = (up.Protected == true ) ? TrueStr : FalseStr;
			tagsSeen++;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetUserPropertyInternal( dbRec, up.PropertyIndex, up.PropertyNameStr, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				up.PropertiesIdNum = dbRec.PropertiesIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( !in_ext_transaction && queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertSignatureRecord( DB_SignatureRecord& sigr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( sigr.SignatureDefId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_SignatureRecord dbRec = {};

	queryResult = GetSignatureInternal( dbRec, sigr.SignatureDefId, sigr.SignatureDefIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	sigr.SignatureDefIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSignatureQueryTag( schemaName, tableName, selectTag, idStr, sigr.SignatureDefId, sigr.SignatureDefIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t numCellTypes = -1;
	int32_t numCellTypesTagIdx = -1;
	int32_t numProps = -1;
	int32_t numPropsTagIdx = -1;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == SG_IdNumTag )				// "SignatureDefIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == SG_IdTag )				// "SignatureDefID"
		{
			uuid__t_to_DB_UUID_Str( sigr.SignatureDefId, valStr );
			tagsSeen++;
		}
		else if ( tag == SG_ShortSigTag )		// "ShortSignature"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sigr.ShortSignatureStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SG_ShortSigHashTag )		// "ShortSignatureHash"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sigr.ShortSignatureHash );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SG_LongSigTag )		// "LongSignature"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sigr.LongSignatureStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SG_LongSigHashTag )		// "LongSignatureHash"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sigr.LongSignatureHash );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ( ( sigr.Protected == true ) ? TrueStr : FalseStr));
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSignatureInternal( dbRec, sigr.SignatureDefId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				sigr.SignatureDefIdNum = dbRec.SignatureDefIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertReagentTypeRecord( DB_ReagentTypeRecord& rxr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	if ( rxr.ContainerTagSn.length() < 1 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_ReagentTypeRecord dbRec = {};

	// new records MUST have a unique values...
	if ( rxr.ReagentIdNum > 0 || rxr.ContainerTagSn.length() > 0 )
	{
		queryResult = DBApi::eQueryResult::NoResults;			// don't fail on a bad check of the tag serial number... allow the insertion to proceed, but with caution...
		if ( rxr.ReagentIdNum > 0 )
		{
			// if the idnumber has been specified, search to see if there is an existing record with this id number...
			queryResult = GetReagentTypeInternal( dbRec, rxr.ReagentIdNum );
		}

		if ( queryResult == DBApi::eQueryResult::NoResults && rxr.ContainerTagSn.length() > 0 )
		{
			// no entry found  with a matching id number, or no id number specified; check for duplicate tag serial number
			queryResult = GetReagentTypeInternal( dbRec, NO_ID_NUM, rxr.ContainerTagSn );
			if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same tag serial number
			{
				WriteLogEntry( "Duplicate Container Tag Serial number found! May not be able to uniquely retrieve this record!", InfoMsgType );
			}
			queryResult = DBApi::eQueryResult::NoResults;			// don't fail on a bad check of the tag serial number... allow the insertion to proceed, but with caution...
		}

		if ( queryResult == DBApi::eQueryResult::QueryOk )				// found existing object with same id number
		{
			// don't allow saving objects with duplicate id numbers
			return DBApi::eQueryResult::InsertObjectExists;
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	}

	rxr.ReagentIdNum = 0;			// reset id number to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetReagentTypeQueryTag( schemaName, tableName, selectTag, idStr, 1 );		// get schemaname, tablename, and tags, forcing identifier tag to be the idnum value...
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	bool dataOk = true;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end() && dataOk; ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == RX_IdNumTag )						// "ReagentIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == RX_TypeNumTag )				// "ReagentTypeNum"
		{
			valStr = boost::str( boost::format( "%d" ) % rxr.ReagentTypeNum );
			tagsSeen++;
		}
		else if ( tag == RX_CurrentSnTag )				// "Current"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( rxr.Current == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == RX_ContainerRfidSNTag )		// "ContainerTagSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.ContainerTagSn );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == RX_IdxListTag )				// "ReagentIndexList"
		{
			size_t listSize = rxr.ReagentIndexList.size();

			tagsSeen++;
			valStr.clear();

			if ( CreateInt16ValueArrayString( valStr, "%ld", static_cast<int32_t>( listSize ), 0, rxr.ReagentIndexList ) != listSize )
			{
				foundCnt = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( tag == RX_NamesListTag )				// "ReagentNamesList"
		{
			size_t listSize = rxr.ReagentNamesList.size();
			std::vector<std::string> cleanList = {};

			tagsSeen++;
			valStr.clear();

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
		else if ( tag == RX_MixingCyclesTag )			// "MixingCycles"
		{
			size_t listSize = rxr.MixingCyclesList.size();

			tagsSeen++;
			valStr.clear();

			if ( CreateInt16ValueArrayString( valStr, "%ld", static_cast<int32_t>( listSize ), 0, rxr.MixingCyclesList ) != listSize )
			{
				foundCnt = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
			}
		}
		else if ( tag == RX_PackPartNumTag )			// "PackPartNum"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.PackPartNumStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == RX_LotNumTag )					// "LotNum"
		{
			valStr = boost::str( boost::format( "'%s'" ) % rxr.LotNumStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == RX_LotExpirationDateTag )		// "LotExpiration"
		{
			valStr = boost::str( boost::format( "%lld" ) % rxr.LotExpirationDate );
			tagsSeen++;
		}
		else if ( tag == RX_InServiceDateTag )			// "InService"
		{
			valStr = boost::str( boost::format( "%d" ) % rxr.InServiceDate );
			tagsSeen++;
		}
		else if ( tag == RX_InServiceDaysTag )			// "ServiceLife"
		{
			valStr = boost::str( boost::format( "%d" ) % rxr.InServiceExpirationLength );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( rxr.Current == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempts to use the tag id serial number, expecting that each tag will have a unique tag id...
			retrieveResult = GetReagentTypeInternal( dbRec, NO_ID_NUM, rxr.ContainerTagSn );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				rxr.ReagentIdNum = dbRec.ReagentIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}



DBApi::eQueryResult DBifImpl::InsertBioProcessRecord( DB_BioProcessRecord& bpr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertQcProcessRecord( DB_QcProcessRecord& qcr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( qcr.QcId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	DB_QcProcessRecord dbRec = {};

	// if there is as existing object with this Instrumet serial number, don't allow creation of another instance of this entry
	queryResult = GetQcProcessInternal( dbRec, qcr.QcId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same serial number
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	qcr.QcIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetQcProcessQueryTag( schemaName, tableName, selectTag, idStr, qcr.QcId, qcr.QcIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string fmtStr = "";
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t numConsumables = -1;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::NotConnected;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		tag = *tagIter;
		valStr.clear();

		if ( tag == QC_IdNumTag )
		{
			tagsSeen++;
		}
		else if ( tag == QC_IdTag )
		{
			uuid__t_to_DB_UUID_Str( qcr.QcId, valStr );
			tagsSeen++;
		}
		else if ( tag == QC_NameTag)
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.QcName );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == QC_TypeTag)
		{
			valStr = boost::str( boost::format( "%d" ) % qcr.QcType );
			tagsSeen++;
		}
		else if ( tag == QC_CellTypeIdTag)
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( qcr.CellTypeId, valStr, errCode /*, EMPTY_ID_NOT_ALLOWED*/ ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == QC_CellTypeIndexTag)
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) qcr.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == QC_LotInfoTag)
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.LotInfo );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == QC_LotExpirationTag)
		{
			valStr = boost::str( boost::format( "'%s'" ) % qcr.LotExpiration );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == QC_AssayValueTag)
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( "%lf" ) % qcr.AssayValue );
			tagsSeen++;
		}
		else if ( tag == QC_AllowablePercentTag)
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( "%lf" ) % qcr.AllowablePercentage );
			tagsSeen++;
		}
		else if ( tag == QC_SequenceTag)
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
			tagsSeen++;
		}
		else if ( tag == QC_CommentsTag)
		{
			if ( qcr.Comments.length() > 0 )
			{
				valStr = boost::str( boost::format( "'%s'" ) % qcr.Comments );
			}
			else
			{
				valStr = "' '";
			}
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if (tag == QC_RetiredTag)								// "Retired"
		{
			valStr = (qcr.Retired == true) ? TrueStr : FalseStr;
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ( ( qcr.Protected == true ) ? TrueStr : FalseStr));
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			dbRec = {};
			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetQcProcessInternal( dbRec, qcr.QcId, qcr.QcIdNum );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				qcr.QcIdNum = dbRec.QcIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertCalibrationRecord( DB_CalibrationRecord& car )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( car.CalId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	DB_CalibrationRecord dbRec = {};

	// if there is as existing object with this UUID, don't allow creation of another instance of this entry
	queryResult = GetCalibrationInternal( dbRec, car.CalId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	car.CalIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetCalibrationQueryTag( schemaName, tableName, selectTag, idStr, car.CalId, car.CalIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
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
	std::string fmtStr = "";
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t numConsumables = -1;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::NotConnected;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == CC_IdNumTag )					// "CalibrationIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == CC_IdTag )					// "CalibrationID"
		{
			uuid__t_to_DB_UUID_Str( car.CalId, valStr );
			tagsSeen++;
		}
		else if ( tag == CC_InstSNTag )				// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % car.InstrumentSNStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CC_CalDateTag )			// "CalibrationDate"
		{
			system_TP newTimePT = {};

			if ( car.CalDate == newTimePT )
			{
				GetDbCurrentTimeString( valStr, car.CalDate );
			}
			else
			{
				GetDbTimeString( car.CalDate, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == CC_CalUserIdTag )			// "CalibrationUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( car.CalUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == CC_CalTypeTag )			// "CalibrationType"
		{
			valStr = boost::str( boost::format( "%ld" ) % car.CalType );
			tagsSeen++;
		}
		else if ( tag == CC_SlopeTag )				// "Slope"
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( fmtStr ) % car.Slope );
			tagsSeen++;
		}
		else if ( tag == CC_InterceptTag )			// "Intercept"
		{
			fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
			valStr = boost::str( boost::format( fmtStr ) % car.Intercept );
			tagsSeen++;
		}
		else if ( tag == CC_ImageCntTag )			// "ImageCount"
		{
			valStr = boost::str( boost::format( "%ld" ) % car.ImageCnt );
			tagsSeen++;
		}
		else if ( tag == CC_QueueIdTag )			// "CalQueueID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( car.QueueId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == CC_ConsumablesListTag )	// "ConsumablesList"
		{
			size_t listSize = car.ConsumablesList.size();

			valStr.clear();

			if ( CreateCalConsumablesArrayString( valStr, static_cast<int32_t>( listSize ), car.ConsumablesList, true ) != listSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			valStr = FalseStr;
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			dbRec = {};
			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetCalibrationInternal( dbRec, car.CalId, car.CalIdNum );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				car.CalIdNum = dbRec.CalIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::InsertInstConfigRecord( DB_InstrumentConfigRecord& icr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	if ( icr.InstrumentSNStr.length() < 1 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	DB_InstrumentConfigRecord dbRec = {};

	// new records MUST have a unique values...
	if ( icr.InstrumentIdNum > 0 || icr.InstrumentSNStr.length() > 0 )
	{
		// first search to see if there is an existing record with this serial number...
		// if there is as existing object with this id number and Instrumet serial number, don't allow creation of another instance of this entry
		queryResult = GetInstConfigInternal( dbRec, icr.InstrumentSNStr, NO_ID_NUM );
		if ( queryResult == DBApi::eQueryResult::NoResults )
		{
			// no entry found  with this serial number; check for duplicate id number
			queryResult = GetInstConfigInternal( dbRec, "", icr.InstrumentIdNum );
		}

		if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same serial number or id number
		{
			// don't allow saving objects with duplicate UUIDs
			return DBApi::eQueryResult::InsertObjectExists;
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	}

	icr.InstrumentIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetInstConfigQueryTag( schemaName, tableName, selectTag, idStr, "", 1 );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

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

		if ( tag == CFG_IdNumTag )							// "InstrumentIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == CFG_InstSNTag )					// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.InstrumentSNStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_InstTypeTag )					// "InstrumentType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.InstrumentType );
			tagsSeen++;
		}
		else if ( tag == CFG_DeviceNameTag )				// "DeviceName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.DeviceName );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_UiVerTag )						// "UIVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.UIVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_SwVerTag )						// "SoftwareVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.SoftwareVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_AnalysisSwVerTag )				// "AnalysisSWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.AnalysisVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_FwVerTag )						// "FirmwareVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.FirmwareVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_CameraTypeTag )				// "CameraType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.CameraType );
			tagsSeen++;
		}

		else if (tag == CFG_BrightFieldLedTypeTag)				// "BrightFieldLedType"
		{
			valStr = boost::str(boost::format("%d") % icr.BrightFieldLedType);
			tagsSeen++;
		}
		else if ( tag == CFG_CameraFwVerTag )				// "CameraFWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraFWVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_CameraCfgTag )					// "CameraConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraConfig );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_PumpTypeTag )					// "PumpType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.PumpType );
			tagsSeen++;
		}
		else if ( tag == CFG_PumpFwVerTag )					// "PumpFWVersion"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.PumpFWVersion );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_PumpCfgTag )					// "PumpConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.PumpConfig );
			SanitizeDataString( valStr );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == CFG_IlluminatorCfgTag )			// "IlluminatorConfig"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.IlluminatorConfig );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_ConfigTypeTag )				// "ConfigType"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.ConfigType );
			tagsSeen++;
		}
		else if ( tag == CFG_LogNameTag )					// "LogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.LogName );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_LogMaxSizeTag )				// "LogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.LogMaxSize );
			tagsSeen++;
		}
		else if ( tag == CFG_LogLevelTag )					// "LogSensitivity"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.LogSensitivity );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_MaxLogsTag )					// "MaxLogs"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.MaxLogs );
			tagsSeen++;
		}
		else if ( tag == CFG_AlwaysFlushTag )				// "AlwaysFlush"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AlwaysFlush == true ) ? TrueStr : FalseStr));
			tagsSeen++;
		}
		else if ( tag == CFG_CameraErrLogNameTag )			// "CameraErrorLogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.CameraErrorLogName );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_CameraErrLogMaxSizeTag )		// "CameraErrorLogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CameraErrorLogMaxSize );
			tagsSeen++;
		}
		else if ( tag == CFG_StorageErrLogNameTag )			// "StorageErrorLogName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % icr.StorageErrorLogName );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == CFG_StorageErrLogMaxSizeTag )		// "StorageErrorLogMaxSize"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.StorageErrorLogMaxSize );
			tagsSeen++;
		}
		else if ( tag == CFG_CarouselThetaHomeTag )			// "CarouselThetaHomeOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CarouselThetaHomeOffset );
			tagsSeen++;
		}
		else if ( tag == CFG_CarouselRadiusOffsetTag )		// "CarouselRadiusOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.CarouselRadiusOffset );
			tagsSeen++;
		}
		else if ( tag == CFG_PlateThetaHomeTag )			// "PlateThetaHomePosOffset"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateThetaHomeOffset );
			tagsSeen++;
		}
		else if ( tag == CFG_PlateThetaCalTag )				// "PlateThetaCalPos"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateThetaCalPos );
			tagsSeen++;
		}
		else if ( tag == CFG_PlateRadiusCenterTag )			// "PlateRadiusCenterPos"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.PlateRadiusCenterPos );
			tagsSeen++;
		}
		else if ( tag == CFG_SaveImageTag )					// "SaveImage"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SaveImage );
			tagsSeen++;
		}
		else if ( tag == CFG_FocusPositionTag )				// "FocusPosition"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.FocusPosition );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == CFG_AbiMaxImageCntTag )			// "AbiMaxImageCount"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.AbiMaxImageCount );
			tagsSeen++;
		}
		else if ( tag == CFG_NudgeVolumeTag )				// "SampleNudgeVolume"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SampleNudgeVolume );
			tagsSeen++;
		}
		else if ( tag == CFG_NudeSpeedTag )					// "SampleNudgeSpeed"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SampleNudgeSpeed );
			tagsSeen++;
		}
		else if ( tag == CFG_FlowCellDepthTag )				// "FlowCellDepth"
		{
			valStr = boost::str( boost::format( "%0.2f" ) % icr.FlowCellDepth );
			tagsSeen++;
		}
		else if ( tag == CFG_FlowCellConstantTag )			// "FlowCellDepthConstant"
		{
			valStr = boost::str( boost::format( "%0.2f" ) % icr.FlowCellDepthConstant );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == CFG_LegacyDataTag )				// "LegacyData"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.LegacyData == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == CFG_CarouselSimTag )				// "CarouselSimulator"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.CarouselSimulator == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == CFG_NightlyCleanOffsetTag )		// "NightlyCleanOffset"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.NightlyCleanOffset );
			tagsSeen++;
		}
		else if ( tag == CFG_LastNightlyCleanTag )			// "LastNightlyClean"
		{
			system_TP newTimePT = {};

			// for newly created records, set the last nightly clean time to 'never' or a 0 TP value...
			icr.LastNightlyClean = newTimePT;
			tagsSeen++;
		}
		else if ( tag == CFG_SecurityModeTag )				// "SecurityMode"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.SecurityMode );
			tagsSeen++;
		}
		else if ( tag == CFG_InactivityTimeoutTag )			// "InactivityTimeout"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.InactivityTimeout );
			tagsSeen++;
		}
		else if ( tag == CFG_PwdExpirationTag )				// "PasswordExpiration"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.PasswordExpiration );
			tagsSeen++;
		}
		else if ( tag == CFG_NormalShutdownTag )			// "NormalShutdown"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.NormalShutdown == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == CFG_NextAnalysisDefIndexTag )		// "NextAnalysisDefIndex"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.NextAnalysisDefIndex );
			tagsSeen++;
		}
		else if ( tag == CFG_NextBCICellTypeIndexTag )		// "NextFactoryCellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.NextBCICellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == CFG_NextUserCellTypeIndexTag )		// "NextUserCellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % static_cast<int32_t>( icr.NextUserCellTypeIndex ) );
			tagsSeen++;
		}

		else if ( tag == CFG_TotSamplesProcessedTag )		// "SamplesProcessed"
		{
			valStr = boost::str( boost::format( "%ld" ) % icr.TotalSamplesProcessed );
			tagsSeen++;
		}
		else if ( tag == CFG_DiscardTrayCapacityTag )		// "DiscardCapacity"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.DiscardTrayCapacity );
			tagsSeen++;
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
			tagsSeen++;
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
			tagsSeen++;
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
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == CFG_AutomationInstalledTag )		// "AutomationInstalled"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AutomationInstalled == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == CFG_AutomationEnabledTag )			// "AutomationEnabled"
		{
			// if not setting installation now, don't allow enable
			if ( !icr.AutomationInstalled )
			{
				icr.AutomationEnabled = false;
			}
			valStr = boost::str( boost::format( "%s" ) % ( ( icr.AutomationEnabled == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if (tag == CFG_ACupEnabledTag)					// "ACupEnabled"
		{
			// if not setting installation now OR not enabling, don't allow ACup enable
			if ( !icr.AutomationInstalled || !icr.AutomationEnabled )
			{
				icr.ACupEnabled = false;
			}
			valStr = boost::str(boost::format("%s") % ((icr.ACupEnabled == true) ? TrueStr : FalseStr));
			tagsSeen++;
		}
		else if ( tag == CFG_AutomationPortTag )			// "AutomationPort"
		{
			valStr = boost::str( boost::format( "%d" ) % icr.AutomationPort );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
//			valStr = boost::str( boost::format( "%s" ) % ( ( icr.Protected == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}

		// now add the update statement component for the tag processed
		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			dbRec = {};
			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetInstConfigInternal( dbRec, icr.InstrumentSNStr, icr.InstrumentIdNum );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				icr.InstrumentIdNum = dbRec.InstrumentIdNum;		// update the idnum in the passed-in object
			}
		}

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
#ifdef HANDLE_TRANSACTIONS
		CancelTransaction();
#endif // HANDLE_TRANSACTIONS
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertLogEntryRecord( DB_LogEntryRecord& logr )
{
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
	DB_LogEntryRecord dbRec = {};
	int64_t chkIdNum = NO_ID_NUM;

	// at this point idnum should not be set for a new insertion, but positive values will be checked against the db
	if ( logr.IdNum > 0 )	// log entries ONLY have the idnum to identify them...
	{
		chkIdNum = logr.IdNum;

		// if there is as existing object with this idnum, index, or other characteristics,
		// don't allow creation of another instance of this object
		// check if this is an existing object
		queryResult = GetLogEntryInternal( dbRec, chkIdNum );
		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
			return DBApi::eQueryResult::InsertObjectExists;
		}
		else if ( queryResult != DBApi::eQueryResult::NoResults )
		{
			return DBApi::eQueryResult::QueryFailed;
		}
	}

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	// get the appropriate table and schema, and the field used to check for insertion duplicates
	// one or more of the id fields should be valid at this point; check each, and on failure, check the other before failing
	// check idnum first
	if ( !GetLogEntryQueryTag( schemaName, tableName, selectTag, idStr, chkIdNum ) )
	{
		WriteLogEntry( "InsertLogEntryRecord: failed: no valid query tags found", ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool dataOk = false;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;

	queryResult = DBApi::eQueryResult::QueryFailed;

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		dataOk = true;
		tag = *tagIter;
		valStr.clear();

		if ( tag == LOG_IdNumTag )					// "EntryIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == LOG_EntryTypeTag )			// "EntryType"
		{
			valStr = boost::str( boost::format( "%d" ) % logr.EntryType );
			tagsSeen++;
		}
		else if ( tag == LOG_EntryDateTag )			// "EntryDate"
		{
			system_TP zeroTP = {};

			if ( logr.EntryDate == zeroTP )
			{
				GetDbCurrentTimeString( valStr, logr.EntryDate );
			}
			else
			{
				GetDbTimeString (logr.EntryDate, valStr);
			}
			tagsSeen++;
		}
		else if ( tag == LOG_LogEntryTag )			// "EntryText"
		{
			valStr = boost::str( boost::format( "'%s'" ) % logr.EntryStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			// not a client-settable value, but note seeing the tag from the DB table
			valStr = boost::str( boost::format( "%s" ) % ( ( logr.Protected == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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

	if ( queryResult != DBApi::eQueryResult::QueryOk )
	{
		WriteLogEntry( "Failure writing DB log entry", QueryErrorMsgType );
	}

	loginType = SetLoginType( DBApi::eLoginType::InstrumentLoginType );		// restore standard connection type

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertSchedulerConfigRecord( DB_SchedulerConfigRecord& scr )
{
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

	queryResult = GenerateValidGuid( scr.ConfigId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	DB_SchedulerConfigRecord dbRec = {};

	// if there is as existing object with this UUID, don't allow creation of another instance of this entry
	queryResult = GetSchedulerConfigInternal( dbRec, scr.ConfigId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	scr.ConfigIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	// get the appropriate table and schema, and the field used to check for insertion duplicates
	GetSchedulerConfigQueryTag( schemaName, tableName, selectTag, idStr, scr.ConfigId, scr.ConfigIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
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
	int32_t tagsSeen = 0;
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

		if ( tag == SCH_IdNumTag )							// "SchedulerConfigIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == SCH_IdTag )						// "SchedulerConfigID"
		{
			uuid__t_to_DB_UUID_Str( scr.ConfigId, valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_NameTag )						// "SchedulerName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.Name );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_CommentsTag )					// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.Comments );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_FilenameTemplateTag )			// "OutputFilenameTemplate"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.FilenameTemplate );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_OwnerIdTag )					// "OwnerID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( scr.OwnerUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == SCH_CreationDateTag )				// "CreationDate"
		{
			if ( scr.CreationDate == zeroTP )
			{
				GetDbCurrentTimeString( valStr, scr.CreationDate );
			}
			else
			{
				GetDbTimeString( scr.CreationDate, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SCH_OutputTypeTag )				// "OutputType"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.OutputType );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == SCH_StartOffsetTag )				// "StartOffset"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.StartOffset );
			tagsSeen++;
		}
		else if ( tag == SCH_RepetitionIntervalTag )		// "RepetitionInterval"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.MultiRepeatInterval );
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == SCH_MonthlyRunDayTag )				// "MonthlyRunDay"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.MonthlyRunDay );
			tagsSeen++;
		}
		else if ( tag == SCH_DestinationFolderTag )			// "DestinationFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.DestinationFolder );
			SanitizePathString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_DataTypeTag )					// "DataType"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.DataType );
			tagsSeen++;
		}
		else if ( tag == SCH_FilterTypesTag ||				// "FilterTypesList"
				  tag == SCH_CompareOpsTag ||				// "CompareOpsList"
				  tag == SCH_CompareValsTag )				// "CompareValsList"
		{
			// NOTE that these three values need to be checked with respect to one another
			filterTypeTags = (int)scr.FilterTypesList.size();
			filterOpTags = (int) scr.CompareOpsList.size();
			filterValTags = (int) scr.CompareValsList.size();

			tagsSeen++;

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
			valStr = boost::str( boost::format( "%s" ) % ( ( scr.Enabled == true ) ? TrueStr : FalseStr));
			tagsSeen++;
		}
		else if ( tag == SCH_LastRunTimeTag )				// "LastRunTime"
		{
			// no special checks for a 'new' time point, as it is used to indicate the 'never run' condiditon;
			GetDbTimeString( scr.LastRunTime, valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_LastSuccessRunTimeTag )		// "LastSuccessRunTime"
		{
			// no special checks for a 'new' time point, as it is used to indicate the 'never run' condiditon;
			GetDbTimeString( scr.LastSuccessRunTime, valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_LastRunStatusTag )				// "LastRunStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % scr.LastRunStatus );
			tagsSeen++;
		}
		else if ( tag == SCH_NotificationEmailTag )			// "NotificationEmail"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.NotificationEmail );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_EmailServerTag )				// "EmailServer"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.EmailServerAddr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_EmailServerPortTag )			// "EmailServerPort"
		{
			valStr = boost::str( boost::format( "%ld" ) % scr.EmailServerPort );
			tagsSeen++;
		}
		else if ( tag == SCH_AuthenticateEmailTag )			// "AuthenticateEmail"
		{
			valStr = boost::str( boost::format( "%s" ) % ( ( scr.AuthenticateEmail == true ) ? TrueStr : FalseStr ) );
			tagsSeen++;
		}
		else if ( tag == SCH_EmailAccountTag )				// "EmailAccount"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.AccountUsername );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SCH_AccountAuthenticatorTag )		// "EmailAccountAuthenticator"
		{
			valStr = boost::str( boost::format( "'%s'" ) % scr.AccountPwdHash );
			SanitizeDataString( valStr );
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                 // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > TagsOk && idFails == 0 )
	{
		std::string queryStr = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryRec.IsOpen() )
		{
			queryRec.Close();
		}
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
		else if ( tagsFound == MissingCriticalObjectID )
		{
			queryResult = DBApi::eQueryResult::BadOrMissingListIds;
		}
		else
		{
			queryResult = DBApi::eQueryResult::NoQuery;
		}
	}

	loginType = SetLoginType( DBApi::eLoginType::InstrumentLoginType );		// restore standard connection type

	return queryResult;
}

