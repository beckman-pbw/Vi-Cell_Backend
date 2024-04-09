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



static const std::string MODULENAME = "DBif_InsertData";



////////////////////////////////////////////////////////////////////////////////
// Internal data object insertion methods
////////////////////////////////////////////////////////////////////////////////


DBApi::eQueryResult DBifImpl::InsertSampleRecord( DB_SampleRecord& sr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( sr.SampleId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_SampleRecord dbRec = {};

	queryResult = GetSampleInternal( dbRec, sr.SampleId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	sr.SampleIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSampleQueryTag( schemaName, tableName, selectTag, idStr, sr.SampleId, 0 );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	bool queryOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	std::vector<std::string> cleanList = {};

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	for ( auto tagIter = tagList.begin(); tagIter != tagList.end(); ++tagIter )
	{
		tag = *tagIter;
		valStr.clear();

		if ( tag == SM_IdNumTag )								// "SampleIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == SM_IdTag )								// "SampleID"
		{
			uuid__t_to_DB_UUID_Str( sr.SampleId, valStr );
			tagsSeen++;
		}
		else if ( tag == SM_StatusTag )							// "SampleStatus"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.SampleStatus );
			tagsSeen++;
		}
		else if ( tag == SM_NameTag )							// "SampleName"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.SampleNameStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SM_CellTypeIdTag )						// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.CellTypeId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_CellTypeIdxTag )					// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) sr.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == SM_AnalysisDefIdTag )					// "AnalysisDefinitionID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be empty or nill at the time of item creation, BUT
			// MUST be filled-in prior to processing
			if ( !GuidInsertCheck( sr.AnalysisDefId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_AnalysisDefIdxTag )					// "AnalysisDefinitionIndex"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.AnalysisDefIndex );
			tagsSeen++;
		}
		else if ( tag == SM_LabelTag )							// "Label"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.Label );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SM_BioProcessIdTag )					// "BioProcessID"
		{
			int32_t errCode = 0;

			// empty is allowed...
			if ( !GuidInsertCheck( sr.BioProcessId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_QcProcessIdTag )					// "QcProcessID"
		{
			int32_t errCode = 0;

			// empty is allowed...
			if ( !GuidInsertCheck( sr.QcId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_WorkflowIdTag )						// "WorkflowID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.WorkflowId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_CommentsTag )						// "Comments"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.CommentsStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SM_WashTypeTag )						// "WashType"
		{
			valStr = boost::str( boost::format( "%d" ) % sr.WashTypeIndex );
			tagsSeen++;
		}
		else if ( tag == SM_DilutionTag )						// "Dilution"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.Dilution );
			tagsSeen++;
		}
		else if ( tag == SM_OwnerUserIdTag )					// "OwnerUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.OwnerUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_RunUserTag )						// "RunUserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.RunUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_AcquireDateTag )					// "AcquisitionDate"
		{
			system_TP zeroTP = {};

			if ( sr.AcquisitionDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, sr.AcquisitionDateTP );
			}
			else
			{
				GetDbTimeString( sr.AcquisitionDateTP, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == SM_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( sr.ImageSetId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_DustRefSetIdTag )					// "DustRefImageSetID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( sr.DustRefImageSetId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_InstSNTag )							// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % sr.AcquisitionInstrumentSNStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == SM_ImageAnalysisParamIdTag )			// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// may be null at the time of creation
			if ( !GuidInsertCheck( sr.ImageAnalysisParamId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SM_NumReagentsTag )					// "NumReagents"
		{
			valStr = boost::str( boost::format( "%u" ) % sr.NumReagents );
			tagsSeen++;
		}
		else if ( tag == SM_ReagentTypeNameListTag )			// "ReagentTypeNameList"
		{
			size_t listCnt = sr.ReagentTypeNameList.size();

			cleanList.clear();
			SanitizeDataStringList( sr.ReagentTypeNameList, cleanList );

			if (  CreateStringDataValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, cleanList ) != listCnt )
			{
				valStr.clear();
			}
			tagsSeen++;
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
			tagsSeen++;
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
			tagsSeen++;
		}
		else if ( tag == SM_PackLotExpirationListTag )			// "PackLotExpirationList"
		{
			size_t listCnt = sr.PackLotExpirationList.size();
			std::vector<int64_t> expirationList = {};
			int64_t tmpDays = 0;

			for ( int i = 0; i < listCnt; i++ )
			{
				tmpDays = static_cast<int64_t>(sr.PackLotExpirationList.at(i) );
				expirationList.push_back( tmpDays );
			}

			if ( CreateInt64ValueArrayString( valStr, "%lld", static_cast<int32_t>( listCnt ), 0, expirationList ) != listCnt )
			{
				valStr.clear();
			}
			tagsSeen++;
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
			tagsSeen++;
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

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSampleInternal( dbRec, sr.SampleId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				sr.SampleIdNum = dbRec.SampleIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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
		std::string logStr = "Possible missing tag handler in 'InsertSampleRecord'";
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertAnalysisRecord( DB_AnalysisRecord& ar )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( ar.AnalysisId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this worklist
	// should never happen for a new UUID
	DB_AnalysisRecord dbRec = {};

	queryResult = GetAnalysisInternal( dbRec, ar.AnalysisId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	ar.AnalysisIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetAnalysisQueryTag( schemaName, tableName, selectTag, idStr, ar.AnalysisId, NO_ID_NUM );
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
	int32_t imgResultCnt = NoItemListCnt;
	int32_t numImgResultsTagIdx = ListCntIdxNotSet;
	int32_t imgSeqCnt = NoItemListCnt;
	int32_t numImgSeqsTagIdx = ListCntIdxNotSet;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == AN_IdNumTag )								// "AnalysisIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == AN_IdTag )								// "AnalysisID"
		{
			uuid__t_to_DB_UUID_Str( ar.AnalysisId, valStr );
			tagsSeen++;
		}
		else if ( tag == AN_SampleIdTag )						// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ar.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ar.ImageSetId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_SummaryResultIdTag )				// "SummaryResultID"
		{
			bool recFound = false;
			bool recsetChanged = false;

			// check if this is an existing object
			if ( GuidValid( ar.SummaryResult.SummaryResultId ) )
			{
				DB_SummaryResultRecord chk_sr = {};

				if ( GetSummaryResultInternal( chk_sr, ar.SummaryResult.SummaryResultId ) == DBApi::eQueryResult::QueryOk )
				{
					recFound = true;
				}
			}

			if ( !recFound && dataOk )
			{
				ar.SummaryResult.AnalysisId = ar.AnalysisId;
				ar.SummaryResult.SampleId = ar.SampleId;
				ar.SummaryResult.ImageSetId = ar.ImageSetId;
				ar.SummaryResult.SummaryResultIdNum = 0;
				ClearGuid( ar.SummaryResult.SummaryResultId );

				// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
				insertResult = InsertSummaryResultRecord( ar.SummaryResult, true );

				if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( ar.SummaryResult.SummaryResultId, valStr );
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
			tagsSeen++;
		}
		else if ( tag == AN_SResultIdTag )						// "SResultID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ar.SResultId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_RunUserIdTag )						// "UserID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ar.AnalysisUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_AnalysisDateTag )					// "AnalysisDate"
		{
			system_TP zeroTP = {};

			if ( ar.AnalysisDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, ar.AnalysisDateTP );
			}
			else
			{
				GetDbTimeString( ar.AnalysisDateTP, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == AN_InstSNTag )							// "InstrumentSN"
		{
			valStr = boost::str( boost::format( "'%s'" ) % ar.InstrumentSNStr );
			SanitizeDataString( valStr );
			tagsSeen++;
		}
		else if ( tag == AN_BioProcessIdTag )					// "BioProcessID"
		{
			int32_t errCode = 0;

			// empty is allowed...
			if ( !GuidInsertCheck( ar.BioProcessId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_QcProcessIdTag )					// "QcProcessID" )
		{
			int32_t errCode = 0;

			// empty is allowed...
			if ( !GuidInsertCheck( ar.QcId, valStr, errCode, EMPTY_ID_ALLOWED ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_WorkflowIdTag )						// "WorkflowID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( ar.WorkflowId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == AN_ImageSequenceCntTag )				// "ImageSequenceCount"
		{
			if ( imgSeqCnt < ItemListCntOk )
			{
				numImgSeqsTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%d" ) % ar.ImageSequenceCount );
				imgSeqCnt = ar.ImageSequenceCount;
			}
			tagsSeen++;
		}
		else if ( tag == AN_ImageSequenceIdListTag )			// "ImageSequenceIDList"
		{
			std::vector<uuid__t> imageRecordIdList = {};
			bool recFound = false;
			bool recsetChanged = false;

			tagsSeen++;
			int32_t foundCnt = tagsFound;
			size_t listSize = ar.ImageSequenceList.size();
			size_t listCnt = listSize;

			valStr.clear();
			// check against the number of objects indicated, not from the vector...
			if ( imgSeqCnt < ItemListCntOk )		// hasn't yet been validated against the object list; don't write yet
			{
				imgSeqCnt = ar.ImageSequenceCount;
			}

			if ( imgSeqCnt != listSize )
			{
				if ( imgSeqCnt >= ItemListCntOk )
				{
					foundCnt = SetItemListCnt;
				}
			}

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				recFound = false;
				DB_ImageSeqRecord& isr = ar.ImageSequenceList.at( i );

				// check if this is an existing object
				if ( GuidValid( isr.ImageSequenceId ) )
				{
					DB_ImageSeqRecord chk_isr = {};

					if ( GetImageSequenceInternal( chk_isr, isr.ImageSequenceId ) == DBApi::eQueryResult::QueryOk )
					{
						if ( !GuidsEqual( chk_isr.ImageSetId, ar.ImageSetId ) )
						{
							ar.ImageSetId = chk_isr.ImageSetId;
							if ( recsetChanged )
							{
								dataOk = false;
							}
							recsetChanged = true;
						}

						if ( UpdateImageSeqRecord( chk_isr, true ) == DBApi::eQueryResult::QueryOk )
						{
							recFound = true;
						}
						else
						{
							dataOk = false; // data update failed but object exists; don't do another insertion...
						}
					}
				}

				if ( !recFound && dataOk )
				{
					isr.ImageSequenceIdNum = 0;
					ClearGuid( isr.ImageSequenceId );

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertImageSeqRecord( isr, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						recFound = true;
						ar.ImageSequenceList.at( i ).ImageSequenceIdNum = isr.ImageSequenceIdNum;		// update the id in the passed-in object
						ar.ImageSequenceList.at( i ).ImageSequenceId = isr.ImageSequenceId;			// update the id in the passed-in object
					}
				}

				if ( !recFound )
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

			// insert doesn't need to check to add empty array statement...
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( imgSeqCnt != listCnt )
				{
					imgSeqCnt = static_cast<int32_t>( listCnt );
					ar.ImageSequenceCount = static_cast<int16_t>( listCnt );
				}

				if ( numImgSeqsTagIdx >= ItemTagIdxOk )      // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( numImgSeqsTagIdx ) );    // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
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
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string queryStr = "";
		std::string selectTag = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetAnalysisInternal( dbRec, ar.AnalysisId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				ar.AnalysisIdNum = dbRec.AnalysisIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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
		else if (tagsFound != OperationFailure )
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

DBApi::eQueryResult DBifImpl::InsertSummaryResultRecord( DB_SummaryResultRecord& sr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( sr.SummaryResultId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_SummaryResultRecord dbRec = {};

	queryResult = GetSummaryResultInternal( dbRec, sr.SummaryResultId, sr.SummaryResultIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	sr.SummaryResultIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSummaryResultQueryTag( schemaName, tableName, selectTag, idStr, sr.SummaryResultId, NO_ID_NUM );
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

		if ( tag == RS_IdNumTag )								// "SummaryResultIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == RS_IdTag )								// "SummaryResultID"
		{
			uuid__t_to_DB_UUID_Str( sr.SummaryResultId, valStr );
			tagsSeen++;
		}
		else if ( tag == RS_SampleIdTag )						// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_ImageSetIdTag )						// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.ImageSetId, valStr, errCode ) )		// image decimation may result in only a small sub-set of images being saved, but there should ALWAYS be an imageset ID
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_AnalysisIdTag )						// "AnalysisID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.AnalysisId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
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
			tagsSeen++;
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

			tagsSeen++;
		}
		else if ( tag == RS_ImgAnalysisParamIdTag )				// "ImageAnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.ImageAnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_AnalysisDefIdTag )					// "AnalysisDefID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.AnalysisDefId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_AnalysisParamIdTag )				//  "AnalysisParamID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.AnalysisParamId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_CellTypeIdTag )						// "CellTypeID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.CellTypeId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RS_CellTypeIdxTag )					// "CellTypeIndex"
		{
			valStr = boost::str( boost::format( "%ld" ) % (int32_t) sr.CellTypeIndex );
			tagsSeen++;
		}
		else if ( tag == RS_StatusTag )							// "ProcessingStatus"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % sr.ProcessingStatus );
		}
		else if ( tag == RS_TotCumulativeImgsTag )				// "TotalCumulativeImages"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCumulativeImages );
		}
		else if ( tag == RS_TotCellsGPTag )						// "TotalCellsGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCells_GP );
		}
		else if ( tag == RS_TotCellsPOITag )					// "TotalCellsPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % sr.TotalCells_POI );
		}
		else if ( tag == RS_POIPopPercentTag )					// "POIPopulationPercent"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.POI_PopPercent );
		}
		else if ( tag == RS_CellConcGPTag )						// "CellConcGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CellConc_GP );
		}
		else if ( tag == RS_CellConcPOITag )					// "CellConcPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CellConc_POI );
		}
		else if ( tag == RS_AvgDiamGPTag )						// "AvgDiamGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgDiam_GP );
		}
		else if ( tag == RS_AvgDiamPOITag )						// "AvgDiamPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgDiam_POI );
		}
		else if ( tag == RS_AvgCircularityGPTag )				// "AvgCircularityGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgCircularity_GP );
		}
		else if ( tag == RS_AvgCircularityPOITag )				// "AvgCircularityPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.AvgCircularity_POI );
		}
		else if ( tag == RS_CoeffOfVarTag )						// "CoefficientOfVariance"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbFloatDataFmtStr ) % sr.CoefficientOfVariance );
		}
		else if ( tag == RS_AvgCellsPerImgTag )					// "AvgCellsPerImage"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%u" ) % sr.AvgCellsPerImage );
		}
		else if ( tag == RS_AvgBkgndIntensityTag )				// "AvgBkgndIntensity"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%u" ) % sr.AvgBkgndIntensity );
		}
		else if ( tag == RS_TotBubbleCntTag )					// "TotalBubbleCount"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%u" ) % sr.TotalBubbleCount );
		}
		else if ( tag == RS_LrgClusterCntTag )					// "LargeClusterCount"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%u" ) % sr.LargeClusterCount );
		}
		else if (tag == RS_QcStatusTag)							// "QcStatus"
		{
			tagsSeen++;
			valStr = boost::str(boost::format("%u") % sr.QcStatus);
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );     // make the quoted string expected by the database for value names
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
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSummaryResultInternal( dbRec, sr.SummaryResultId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				sr.SummaryResultIdNum = dbRec.SummaryResultIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertDetailedResultRecord( DB_DetailedResultRecord& dr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( dr.DetailedResultId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_DetailedResultRecord dbRec = {};

	queryResult = GetDetailedResultInternal( dbRec, dr.DetailedResultId, dr.DetailedResultIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	dr.DetailedResultIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetDetailedResultQueryTag( schemaName, tableName, selectTag, idStr, dr.DetailedResultId, NO_ID_NUM );
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

		if ( tag == RD_IdNumTag )								// "DetailedResultIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == RD_IdTag )								// "detailedResultID"
		{
			uuid__t_to_DB_UUID_Str( dr.DetailedResultId, valStr );
			tagsSeen++;
		}
		else if ( tag == RD_SampleIdTag )						// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( dr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RD_ImageIdTag )						// "ImageID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( dr.ImageId, valStr, errCode, EMPTY_ID_ALLOWED ) )		// image decimation may result in no saved image and therefore no ID; also, if a cumulative-type record, no specific image will be referenced
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RD_AnalysisIdTag )						// "AnalysisID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( dr.AnalysisId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RD_OwnerIdTag )						// "OwnerID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( dr.OwnerUserId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RD_ResultDateTag )						// "ResultDate"
		{
			system_TP zeroTP = {};

			if ( dr.ResultDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, dr.ResultDateTP );
			}
			else
			{
				GetDbTimeString( dr.ResultDateTP, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == RD_ProcessingStatusTag )				// "ProcessingStatus"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.ProcessingStatus );
		}
		else if ( tag == RD_TotCumulativeImgsTag )				// "TotalCumulativeImages"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.TotalCumulativeImages );
		}
		else if ( tag == RD_TotCellsGPTag )						// "TotalCellsGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.TotalCells_GP );
		}
		else if ( tag == RD_TotCellsPOITag )					// "TotalCellsPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.TotalCells_POI );
		}
		else if ( tag == RD_POIPopPercentTag )					// "POIPopulationPercent"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.POI_PopPercent );
		}
		else if ( tag == RD_CellConcGPTag )						// "CellConcGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.CellConc_GP );
		}
		else if ( tag == RD_CellConcPOITag )					// "CellConcPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.CellConc_POI );
		}
		else if ( tag == RD_AvgDiamGPTag )						// "AvgDiamGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgDiam_GP );
		}
		else if ( tag == RD_AvgDiamPOITag )						// "AvgDiamPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgDiam_POI );
		}
		else if ( tag == RD_AvgCircularityGPTag )				// "AvgCircularityGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgCircularity_GP );
		}
		else if ( tag == RD_AvgCircularityPOITag )				// "AvgCircularityPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgCircularity_POI );
		}
		else if ( tag == RD_AvgSharpnessGPTag )					// "AvgSharpnessGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgSharpness_GP );
		}
		else if ( tag == RD_AvgSharpnessPOITag )				// "AvgSharpnessPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgSharpness_POI );
		}
		else if ( tag == RD_AvgEccentricityGPTag )				// "AvgEccentricityGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgEccentricity_GP );
		}
		else if ( tag == RD_AvgEccentricityPOITag )				// "AvgEccentricityPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgEccentricity_POI );
		}
		else if ( tag == RD_AvgAspectRatioGPTag )				// "AvgAspectRatioGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgAspectRatio_GP );
		}
		else if ( tag == RD_AvgAspectRatioPOITag )				// "AvgAspectRatioPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgAspectRatio_POI );
		}
		else if ( tag == RD_AvgRoundnessGPTag )					// "AvgRoundnessGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgRoundness_GP );
		}
		else if ( tag == RD_AvgRoundnessPOITag )				// "AvgRoundnessPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgRoundness_POI );
		}
		else if ( tag == RD_AvgRawCellSpotBrightnessGPTag )		// "AvgRawCellSpotBrightnessGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgRawCellSpotBrightness_GP );
		}
		else if ( tag == RD_AvgRawCellSpotBrightnessPOITag )	// "AvgRawCellSpotBrightnessPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgRawCellSpotBrightness_POI );
		}
		else if ( tag == RD_AvgCellSpotBrightnessGPTag )		// "AvgCellSpotBrightnessGP"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgCellSpotBrightness_GP );
		}
		else if ( tag == RD_AvgCellSpotBrightnessPOITag )		// "AvgCellSpotBrightnessPOI"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgCellSpotBrightness_POI );
		}
		else if ( tag == RD_AvgBkgndIntensityTag )				// "AvgBkgndIntensity"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( DbDoubleExpDataFmtStr ) % dr.AvgBkgndIntensity );
		}
		else if ( tag == RD_TotBubbleCntTag )					// "TotalBubbleCount"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.TotalBubbleCount );
		}
		else if ( tag == RD_LrgClusterCntTag )					// "LargeClusterCount"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % dr.LargeClusterCount );
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );     // make the quoted string expected by the database for value names
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
			if ( !in_ext_transaction )
			{
#ifdef HANDLE_TRANSACTIONS
				EndTransaction();
#endif // HANDLE_TRANSACTIONS
			}

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetDetailedResultInternal( dbRec, dr.DetailedResultId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				dr.DetailedResultIdNum = dbRec.DetailedResultIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertImageResultRecord( DB_ImageResultRecord& irr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( irr.ResultId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this worklist
	// should never happen for a new UUID
	DB_ImageResultRecord dbRec = {};

	queryResult = GetImageResultInternal( dbRec, irr.ResultId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	irr.ResultIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetImageResultQueryTag( schemaName, tableName, selectTag, idStr, irr.ResultId, NO_ID_NUM );
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
	int32_t numBlobsTagIdx = ListCntIdxNotSet;
	int32_t numClustersTagIdx = ListCntIdxNotSet;
	int64_t idNum = NO_ID_NUM;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

	if ( irr.NumBlobs == 0 || ( irr.NumBlobs != irr.BlobDataList.size() ) )
	{
		irr.NumBlobs = static_cast<int16_t>(irr.BlobDataList.size());
	}

	if ( irr.NumClusters == 0 || ( irr.NumClusters != irr.ClusterDataList.size() ) )
	{
		irr.NumClusters = static_cast<int16_t>( irr.ClusterDataList.size());
	}

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

		if ( tag == RI_IdNumTag )									// "ResultIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == RI_IdTag )									// "ResultID"
		{
			uuid__t_to_DB_UUID_Str( irr.ResultId, valStr );
			tagsSeen++;
		}
		else if ( tag == RI_SampleIdTag )							// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( irr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RI_ImageIdTag )							// "ImageID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( irr.ImageId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RI_AnalysisIdTag )							// "AnalysisID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( irr.AnalysisId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == RI_ImageSeqNumTag )
		{
			valStr = boost::str( boost::format( "%d" ) % irr.ImageSeqNum );
			tagsSeen++;
		}
		else if ( tag == RI_DetailedResultIdTag )					// "DetailedResultID"
		{
			bool recFound = false;

			// check if this is an existing object
			// Cumulative results are the summation/average of all individual image detailed results...
			if ( GuidValid( irr.DetailedResult.DetailedResultId ) )
			{
				DB_DetailedResultRecord chk_dr = {};

				if ( GetDetailedResultInternal( chk_dr, irr.DetailedResult.DetailedResultId ) == DBApi::eQueryResult::QueryOk )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( irr.DetailedResult.DetailedResultId, valStr );
				}
			}

			if ( !recFound && dataOk )
			{
				irr.DetailedResult.DetailedResultIdNum = 0;
				irr.DetailedResult.AnalysisId = irr.AnalysisId;
				irr.DetailedResult.SampleId = irr.SampleId;
				irr.DetailedResult.ImageId = irr.ImageId;
				ClearGuid( irr.DetailedResult.DetailedResultId );

				// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
				insertResult = InsertDetailedResultRecord( irr.DetailedResult, true );

				if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( irr.DetailedResult.DetailedResultId, valStr );
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
			tagsSeen++;
		}
		else if ( tag == RI_MaxFlChanPeakMapTag )					// "MaxNumOfPeaksFlChanMap"
		{
			size_t mapSize = irr.MaxNumOfPeaksFlChansMap.size();

			if ( CreateInt16MapValueArrayString( valStr, static_cast<int32_t>( mapSize ), irr.MaxNumOfPeaksFlChansMap ) != mapSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}

			tagsSeen++;
		}
		else if ( tag == RI_NumBlobsTag )							// "NumBlobs"
		{
			valStr = boost::str( boost::format( "%d" ) % irr.NumBlobs );
			tagsSeen++;
		}
		else if ( tag == RI_BlobInfoListStrTag )					// "BlobInfoListStr"
		{
			size_t listSize = irr.BlobDataList.size();

			if ( listSize > 0 )
			{
				valStr = "'";	// for 'string' single quote surround...

				if ( CreateBlobDataBlobInfoArrayString( valStr, static_cast<int32_t>( listSize ), irr.BlobDataList, false, IR_COMPOSITE_STRING_LISTS_FORMAT ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;
				}
				else
				{
					valStr.append( "'" );
				}
			}
			tagsSeen++;
		}
		else if ( tag == RI_BlobCenterListStrTag )					// "BlobCenterListStr"
		{
			size_t listSize = irr.BlobDataList.size();

			if ( listSize > 0 )
			{
				valStr = "'";	// for 'string' single quote surround...

				if ( CreateBlobDataBlobLocationArrayString( valStr, static_cast<int32_t>( listSize ), irr.BlobDataList, false, IR_COMPOSITE_STRING_LISTS_FORMAT ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;
				}
				else
				{
					valStr.append( "'" );
				}
			}
			tagsSeen++;
		}
		else if ( tag == RI_BlobOutlineListStrTag )					// "BlobOutlineListStr"
		{
			size_t listSize = irr.BlobDataList.size();

			if ( listSize > 0 )
			{
				valStr = "'";	// for 'string' single quote surround...

				if ( CreateBlobDataBlobOutlineArrayString( valStr, static_cast<int32_t>( listSize ), irr.BlobDataList, false, IR_COMPOSITE_STRING_LISTS_FORMAT ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;
				}
				else
				{
					valStr.append( "'" );
				}
			}
			tagsSeen++;
		}
		else if ( tag == RI_NumClustersTag )						// "NumClusters"
		{
			valStr = boost::str( boost::format( "%d" ) % irr.NumClusters );
			tagsSeen++;
		}
		else if ( tag == RI_ClusterCellCountListTag )				// "ClusterCellCountList"
		{
			size_t listSize = irr.ClusterDataList.size();

			if ( CreateClusterCellCountArrayString( valStr, static_cast<int32_t>( listSize ), irr.ClusterDataList, false ) != listSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}
			tagsSeen++;
		}
		else if ( tag == RI_ClusterPolygonListStrTag )				// "ClusterPolygonListStr"
		{
			size_t listSize = irr.ClusterDataList.size();

			if ( listSize > 0 )
			{
				valStr = "'";	// for 'string' single quote surround...

				if ( CreateClusterPolygonArrayString( valStr, static_cast<int32_t>( listSize ), irr.ClusterDataList, false, IR_COMPOSITE_STRING_LISTS_FORMAT ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;
				}
				else
				{
					valStr.append( "'" );
				}
			}
			tagsSeen++;
		}
		else if ( tag == RI_ClusterRectListStrTag )					// "ClusterRectListStr"
		{
			size_t listSize = irr.ClusterDataList.size();

			if ( listSize > 0 )
			{
				valStr = "'";	// for 'string' single quote surround...

				if ( CreateClusterRectArrayString( valStr, static_cast<int32_t>( listSize ), irr.ClusterDataList, false, IR_COMPOSITE_STRING_LISTS_FORMAT ) != listSize )
				{
					tagsFound = ItemValCreateError;
					valStr.clear();
					dataOk = false;
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;
				}
				else
				{
					valStr.append( "'" );
				}
			}
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
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
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

#ifdef CONFIRM_AFTER_WRITE		// this can take a long time for retrieval formatting...
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetImageResultInternal( dbRec, irr.ResultId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				irr.ResultIdNum = dbRec.ResultIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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
		else if ( tagsFound != OperationFailure )
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

DBApi::eQueryResult DBifImpl::InsertSResultRecord( DB_SResultRecord& sr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( sr.SResultId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this worklist
	// should never happen for a new UUID
	DB_SResultRecord dbRec = {};

	queryResult = GetSResultInternal( dbRec, sr.SResultId, NO_ID_NUM );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	sr.SResultIdNum = 0;

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetSResultQueryTag( schemaName, tableName, selectTag, idStr, sr.SResultId, NO_ID_NUM );
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
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::QueryFailed;

#ifdef HANDLE_TRANSACTIONS
	BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS

	// need to use tag index for tag position retrieval
	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		valStr.clear();

		if ( tag == SR_IdNumTag )									// "SResultIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == SR_IdTag )									// "SResultID"
		{
			uuid__t_to_DB_UUID_Str( sr.SResultId, valStr );
			tagsSeen++;
		}
		else if ( tag == SR_SampleIdTag )							// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SR_AnalysisIdTag )							// "AnalysisID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( sr.AnalysisId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == SR_ProcessingSettingsIdTag )				// "ProcessingSettingsID"
		{
			bool recFound = false;
			bool recsetChanged = false;

			int32_t errCode = 0;

			// check if this is an existing object
			if ( GuidValid( sr.ProcessingSettings.SettingsId ) )
			{
				DB_AnalysisInputSettingsRecord chk_ps = {};

				if ( GetAnalysisInputSettingsInternal( chk_ps, sr.ProcessingSettings.SettingsId ) == DBApi::eQueryResult::QueryOk )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( sr.ProcessingSettings.SettingsId, valStr );
				}
			}

			if ( !recFound && dataOk )
			{
				sr.ProcessingSettings.SettingsIdNum = 0;
				ClearGuid( sr.ProcessingSettings.SettingsId );

				// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
				insertResult = InsertAnalysisInputSettingsRecord( sr.ProcessingSettings, true );

				if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( sr.ProcessingSettings.SettingsId, valStr );
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
			tagsSeen++;
		}
		else if ( tag == SR_CumDetailedResultIdTag )				// "CumulativeDetailedResultID"
		{
			bool recFound = false;
			bool recsetChanged = false;

			// check if this is an existing object
			// Cumulative results are the summation/average of all individual image detailed results...
			if ( GuidValid( sr.CumulativeDetailedResult.DetailedResultId ) )
			{
				DB_DetailedResultRecord chk_dr = {};

				if ( GetDetailedResultInternal( chk_dr, sr.CumulativeDetailedResult.DetailedResultId ) == DBApi::eQueryResult::QueryOk )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( sr.CumulativeDetailedResult.DetailedResultId, valStr );
				}
			}

			if ( !recFound && dataOk )
			{
				sr.CumulativeDetailedResult.DetailedResultIdNum = 0;
				ClearGuid( sr.CumulativeDetailedResult.DetailedResultId );
				sr.CumulativeDetailedResult.AnalysisId = sr.AnalysisId;
				sr.CumulativeDetailedResult.SampleId = sr.SampleId;

				// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
				insertResult = InsertDetailedResultRecord( sr.CumulativeDetailedResult, true );

				if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
				{
					recFound = true;
					uuid__t_to_DB_UUID_Str( sr.CumulativeDetailedResult.DetailedResultId, valStr );
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
			tagsSeen++;
		}
		else if ( tag == SR_CumMaxFlChanPeakMapTag )				// "CumMaxNumOfPeaksFlChanMap"
		{
			size_t mapSize = sr.CumulativeMaxNumOfPeaksFlChanMap.size();

			if ( CreateInt16MapValueArrayString( valStr, static_cast<int32_t>( mapSize ), sr.CumulativeMaxNumOfPeaksFlChanMap ) != mapSize )
			{
				tagsFound = ItemValCreateError;
				valStr.clear();
				dataOk = false;
				queryResult = DBApi::eQueryResult::QueryFailed;
				break;
			}

			tagsSeen++;
		}
		else if ( tag == SR_ImageResultIdListTag )					// "ImageResultsIDList"
		{
			std::vector<uuid__t> imageResultIdList = {};
			bool recFound = false;
			bool recsetChanged = false;

			tagsSeen++;
			int32_t foundCnt = tagsFound;
			size_t listSize = sr.ImageResultList.size();
			size_t listCnt = listSize;

			valStr.clear();

			for ( int32_t i = 0; i < listCnt; i++ )
			{
				recFound = false;
				DB_ImageResultRecord& irr = sr.ImageResultList.at( i );

				// check if this is an existing object
				if ( GuidValid( irr.ResultId ) )
				{
					DB_ImageResultRecord chk_irr = {};

					if ( GetImageResultInternal( chk_irr, irr.ResultId ) == DBApi::eQueryResult::QueryOk )
					{
						recFound = true;
					}
				}

				if ( !recFound && dataOk )
				{
					irr.ResultIdNum = 0;
					ClearGuid( irr.ResultId );
					irr.SampleId = sr.SampleId;
					irr.AnalysisId = sr.AnalysisId;

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertImageResultRecord( irr, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						recFound = true;
						sr.ImageResultList.at( i ).ResultIdNum = irr.ResultIdNum;		// update the id in the passed-in object
						sr.ImageResultList.at( i ).ResultId = irr.ResultId;				// update the id in the passed-in object
					}
				}

				if ( !recFound )
				{
					foundCnt = ListItemNotFound;
					dataOk = false;
					valStr.clear();
					queryResult = DBApi::eQueryResult::QueryFailed;
					break;          // break out of this 'for' loop
				}
				else
				{
					imageResultIdList.push_back( irr.ResultId );
				}
			}

			listCnt = imageResultIdList.size();

			// insert doesn't need to check to add empty array statement...
			if ( dataOk )
			{
				if ( CreateGuidValueArrayString( valStr, static_cast<int32_t>( listCnt ), 0, imageResultIdList ) != listCnt )
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
				break;		// prevent overwrite of the error indicator
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
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );         // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0 )
	{
		std::string queryStr = "";
		std::string selectTag = "";
		CRecordset queryRec( pDb );

		MakeColumnValuesInsertUpdateQueryString( tagsFound, queryStr, schemaName, tableName,
												 namesStr, valuesStr, selectTag, idStr, FORMAT_FOR_INSERT );

		queryResult = RunQuery( loginType, queryStr, queryRec );

		if ( queryResult == DBApi::eQueryResult::QueryOk )
		{
#ifdef HANDLE_TRANSACTIONS
			EndTransaction();
#endif // HANDLE_TRANSACTIONS

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetSResultInternal( dbRec, sr.SResultId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				sr.SResultIdNum = dbRec.SResultIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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
		else if ( tagsFound != OperationFailure )
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

DBApi::eQueryResult DBifImpl::InsertImageSetRecord( DB_ImageSetRecord& isr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( isr.ImageSetId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_ImageSetRecord dbRec = {};

	queryResult = GetImageSetInternal( dbRec, isr.ImageSetId, isr.ImageSetIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	isr.ImageSetIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetImageSetQueryTag( schemaName, tableName, selectTag, idStr, isr.ImageSetId, isr.ImageSetIdNum );
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
	int32_t imageRecCount = NoItemListCnt;
	int32_t imageRecCountTagIdx = ListCntIdxNotSet;
	size_t listCnt = 0;
	size_t listSize = 0;
	int64_t idNum = NO_ID_NUM;
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

		if ( tag == IC_IdNumTag )						// "ImageSetIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == IC_IdTag )						// "ImageSetID"
		{
			uuid__t_to_DB_UUID_Str( isr.ImageSetId, valStr );
			tagsSeen++;
		}
		else if ( tag == IC_SampleIdTag )				// "SampleID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( isr.SampleId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == IC_CreationDateTag )			// "CreationDate"
		{
			system_TP zeroTP = {};

			if ( isr.CreationDateTP == zeroTP )
			{
				GetDbCurrentTimeString( valStr, isr.CreationDateTP );
			}
			else
			{
				GetDbTimeString( isr.CreationDateTP, valStr );
			}
			tagsSeen++;
		}
		else if ( tag == IC_SetFolderTag )				// "ImageSetFolder"
		{
			valStr = boost::str( boost::format( "'%s'" ) % isr.ImageSetPathStr );
			SanitizePathString( valStr );
			tagsSeen++;
		}
		else if ( tag == IC_SequenceCntTag )			// "ImageSequenceCount"
		{
			if ( imageRecCount < ItemListCntOk )				// hasn't yet been validated against the object list; don't write
			{
				imageRecCountTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % isr.ImageSequenceCount );
				imageRecCount = isr.ImageSequenceCount;
			}
			tagsSeen++;
		}
		else if ( tag == IC_SequenceIdListTag )			// "ImageSequenceIdList"
		{
			std::vector<uuid__t> imageRecIdList = {};
			int32_t indexVal = 0;
			size_t listSize = isr.ImageSequenceList.size();
			size_t listCnt = listSize;
			bool imgRecFound = false;

			valStr.clear();
			tagsSeen++;
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

				seqr.ImageSetId = isr.ImageSetId;
				isr.ImageSequenceList.at( i ).ImageSetId = isr.ImageSetId;

				// check if this is an existing object
				if ( GuidValid( seqr.ImageSequenceId ) )
				{
					DB_ImageSeqRecord chk_seqr = {};

					if ( GetImageSequenceInternal( chk_seqr, seqr.ImageSequenceId ) == DBApi::eQueryResult::QueryOk )
					{
						if ( UpdateImageSeqRecord( seqr, true ) == DBApi::eQueryResult::QueryOk )
						{
							imgRecFound = true;
						}
						else
						{
							dataOk = false;		// prevent insertion of identical object...
						}
					}
				}

				if ( !imgRecFound && dataOk )
				{
					seqr.ImageSequenceIdNum = 0;
					ClearGuid( seqr.ImageSequenceId );
					seqr.ImageSetId = isr.ImageSetId;

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertImageSeqRecord( seqr, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						imgRecFound = true;
						isr.ImageSequenceList.at( i ).ImageSequenceIdNum = seqr.ImageSequenceIdNum;		// update the id in the passed-in object
						isr.ImageSequenceList.at( i ).ImageSequenceId = seqr.ImageSequenceId;			// update the id in the passed-in object
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
					isr.ImageSequenceCount = static_cast<int16_t>( listCnt );
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
				if ( imageRecCount != listCnt )
				{
					imageRecCount = static_cast<int32_t>( listCnt );
					isr.ImageSequenceCount = static_cast<int16_t>( listCnt );
				}

				if ( imageRecCountTagIdx >= ItemTagIdxOk )			// the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( imageRecCountTagIdx ) );   // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
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

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetImageSetInternal( dbRec, isr.ImageSetId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				isr.ImageSetIdNum = dbRec.ImageSetIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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

DBApi::eQueryResult DBifImpl::InsertImageSeqRecord( DB_ImageSeqRecord& isr, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( isr.ImageSequenceId );
	if ( queryResult != DBApi::eQueryResult::QueryOk ) 		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_ImageSeqRecord dbRec = {};

	queryResult = GetImageSequenceInternal( dbRec, isr.ImageSequenceId, isr.ImageSequenceIdNum );
	if ( queryResult == DBApi::eQueryResult::QueryOk )		// found existing object with same UUID
	{
		// don't allow saving objects with duplicate UUIDs
		return DBApi::eQueryResult::InsertObjectExists;
	}
	else if ( queryResult != DBApi::eQueryResult::NoResults )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	isr.ImageSequenceIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string schemaName = "";
	std::string tableName = "";
	std::string selectTag = "";
	std::string idStr = "";
	int32_t tagCnt = 0;
	std::vector<std::string> tagList = {};

	GetImageSequenceQueryTag( schemaName, tableName, selectTag, idStr, isr.ImageSequenceId, isr.ImageSequenceIdNum );
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
	int32_t imageCount = -1;
	int32_t imageCountTagIdx = -1;
	int32_t flChanCount = -1;
	int32_t flChanCountTagIdx = -1;
	DBApi::eQueryResult insertResult = DBApi::eQueryResult::QueryFailed;

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

		if ( tag == IS_IdNumTag )						// "ImageSequenceIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == IS_IdTag )						// "ImageSequenceID"
		{
			uuid__t_to_DB_UUID_Str( isr.ImageSequenceId, valStr );
			tagsSeen++;
		}
		else if ( tag == IS_SetIdTag )					// "ImageSetID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( isr.ImageSetId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == IS_SequenceNumTag )			// "ImageSequenceSeqNum"
		{
			valStr = boost::str( boost::format( "%d" ) % isr.SequenceNum );
			tagsSeen++;
		}
		else if ( tag == IS_ImageCntTag )				// "ImageCount"
		{
			if ( imageCount < 0 )           // hasn't yet been validated against the object list; don't write
			{
				imageCountTagIdx = tagIndex;
			}
			else
			{
				valStr = boost::str( boost::format( "%u" ) % (int16_t)isr.ImageCount );			// cast to avoid boost bug for byte values...
				imageCount = isr.ImageCount;
			}
			tagsSeen++;
		}
		else if ( tag == IS_FlChansTag )				// "FlChannels"
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
			tagsSeen++;
		}
		else if ( tag == IS_ImageIdListTag )			// "ImageIdList"
		{
			std::vector<uuid__t> imageIdList = {};
			size_t idListSize = isr.ImageList.size();
			size_t listCnt = idListSize;
			bool imgFound = false;
			int32_t flChans = 0;

			valStr.clear();
			tagsSeen++;
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
				imgFound = false;
				DB_ImageRecord& ir = isr.ImageList.at( i );

				ir.ImageSequenceId = isr.ImageSequenceId;
				isr.ImageList.at( i ).ImageSequenceId = isr.ImageSequenceId;

				// if uuid__t is valid, do retrieval by using it
				if ( GuidValid( ir.ImageId ) )
				{
					DB_ImageRecord chk_ir = {};
					
					if ( GetImage( chk_ir, ir.ImageId ) == DBApi::eQueryResult::QueryOk )
					{
						if ( UpdateImageRecord( ir, true ) == DBApi::eQueryResult::QueryOk )
						{
							imgFound = true;
						}
						else
						{
							dataOk = false;		// prevent insertion of identical object...
						}

						if ( chk_ir.ImageChannel > 0 )			// assumes brightfield is channel index '0'
						{
							flChans++;
						}
					}
				}

				if ( !imgFound && dataOk )
				{
					ir.ImageIdNum = 0;
					ClearGuid( ir.ImageId );
					ir.ImageSequenceId = isr.ImageSequenceId;

					// when saving a new object, ALWAYS treat the entire object (and contained objects) as NEW
					insertResult = InsertImageRecord( ir, true );

					if ( insertResult == DBApi::eQueryResult::QueryOk || insertResult == DBApi::eQueryResult::InsertObjectExists )
					{
						imgFound = true;
						isr.ImageList.at( i ).ImageIdNum = ir.ImageIdNum;		// update the id in the passed-in object
						isr.ImageList.at( i ).ImageId = ir.ImageId;				// update the id in the passed-in object
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
				break;		// prevent overwrite of the error indicator
			}

			// need to do the 'num' field addition here, once it's been validated...
			if ( dataOk )
			{
				if ( imageCount != listCnt )
				{
					imageCount = static_cast<int32_t>( listCnt );
					isr.ImageCount = static_cast<int8_t>( listCnt );
				}

				if ( imageCountTagIdx >= ItemTagIdxOk )         // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( imageCountTagIdx ) );   // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % listCnt );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}

				if ( flChanCount != flChans )
				{
					flChanCount = flChans;
					isr.FlChannels = flChans;
				}

				if ( flChanCountTagIdx >= ItemTagIdxOk )         // the tag has already been handled
				{
					tagsFound++;
					cntTagStr = boost::str( boost::format( "\"%s\"" ) % tagList.at( flChanCountTagIdx ) );  // make the quoted string expected by the database for value names
					cntValStr = boost::str( boost::format( "%d" ) % ( listCnt - 1 ) );
					AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, cntTagStr, cntValStr );
				}
			}
		}
		else if ( tag == IS_SequenceFolderTag )			// "ImageSequenceFolder"
		{
			tagsSeen++;
			valStr = isr.ImageSequenceFolderStr;
			valStr = boost::str( boost::format( "'%s'" ) % isr.ImageSequenceFolderStr );     // make the quoted string expected by the database for string values
			SanitizePathString( valStr );
		}
		else if ( tag == ProtectedTag )
		{
			// not a client-settable value, but note seeing the tag from the DB table
			tagsSeen++;
		}

		if ( valStr.length() > 0 )
		{
			tagsFound++;
			tagStr = boost::str( boost::format( "\"%s\"" ) % tag );                     // make the quoted string expected by the database for value names
			AddToInsertUpdateString( FORMAT_FOR_INSERT, updateIdx++, namesStr, valuesStr, tagStr, valStr );
		}
	}

	if ( tagsFound > 0 && idFails == 0)
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

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetImageSequenceInternal( dbRec, isr.ImageSequenceId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				isr.ImageSequenceIdNum = dbRec.ImageSequenceIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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

	return queryResult;
}

DBApi::eQueryResult DBifImpl::InsertImageRecord( DB_ImageRecord& img, bool in_ext_transaction )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	loginType = ConnectionCheck( pDb );

	if ( loginType == DBApi::eLoginType::NoLogin )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GenerateValidGuid( img.ImageId );
	if ( queryResult != DBApi::eQueryResult::QueryOk )		// Generation could not generate a valid Uuid not matching the 'illegal' patterns
	{
		return queryResult;
	}

	// if there is as existing object with this UUID, don't allow creation of another instance of this object
	// should never happen for a new UUID
	DB_ImageRecord dbRec = {};

	queryResult = GetImageInternal( dbRec, img.ImageId, img.ImageIdNum );
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

	GetImageQueryTag( schemaName, tableName, selectTag, idStr, img.ImageId, img.ImageIdNum );
	if ( !RunColTagQuery( pDb, schemaName, tableName, tagCnt, tagList ) )
	{
		return DBApi::eQueryResult::QueryFailed;
	}

	img.ImageIdNum = 0;			// reset idnumber to 0 when saving as a new object

	std::string valuesStr = "";
	std::string valStr = "";
	std::string namesStr = "";
	std::string tag = "";
	std::string tagStr = "";
	std::string cntTagStr = "";
	std::string cntValStr = "";
	int32_t tagIndex = 0;
	bool dataOk = false;
	int32_t idFails = 0;
	int32_t updateIdx = 0;
	int32_t tagsFound = 0;
	int32_t tagsSeen = 0;
	int32_t foundCnt = 0;
	int32_t imageCount = -1;
	int32_t idListSize = 0;
	int32_t namesListSize = 0;
	int32_t listCnt = 0;

	if ( !in_ext_transaction )
	{
#ifdef HANDLE_TRANSACTIONS
		BeginTransaction( DBApi::eLoginType::AnyLoginType );
#endif // HANDLE_TRANSACTIONS
	}

	for ( tagIndex = 0; tagIndex < tagCnt; tagIndex++ )
	{
		dataOk = true;
		tag = tagList.at( tagIndex );
		tagStr = tag.c_str();
		valStr.clear();

		if ( tag == IM_IdNumTag )						// "ImageIdNum"
		{
			tagsSeen++;
		}
		else if ( tag == IM_IdTag )						// "ImageID"
		{
			tagsSeen++;
			uuid__t_to_DB_UUID_Str( img.ImageId, valStr );
		}
		else if ( tag == IM_SequenceIdTag )				// "ImageSequenceID"
		{
			int32_t errCode = 0;

			// SHOULD NOT be null at the time of creation
			if ( !GuidInsertCheck( img.ImageSequenceId, valStr, errCode ) )
			{
				tagsFound = errCode;
				idFails++;
			}
			tagsSeen++;
		}
		else if ( tag == IM_ImageChanTag )				// "ImageChannel"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "%d" ) % (int16_t)img.ImageChannel );		// cast to avoid boost bug for byte values...
		}
		else if ( tag == IM_FileNameTag )				// "ImageFileName"
		{
			tagsSeen++;
			valStr = boost::str( boost::format( "'%s'" ) % img.ImageFileNameStr );			// make the quoted string expected by the database for string values
		}
		else if ( tag == ProtectedTag )
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

	if ( tagsFound > 0 && idFails == 0 )
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

#ifdef CONFIRM_AFTER_WRITE
			// check for retrieval of inserted/found item by uuid__t value...
			DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryFailed;

			// NOTE: this retrieval attempt is likely to be happening too soon after the actual insertion for the DB to be able to respond corectly...
			retrieveResult = GetImageInternal( dbRec, img.ImageId, NO_ID_NUM );
			if ( retrieveResult == DBApi::eQueryResult::QueryOk )
			{
				img.ImageIdNum = dbRec.ImageIdNum;		// update the idnum in the passed-in object
			}
#endif // CONFIRM_AFTER_WRITE
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

	return queryResult;
}
