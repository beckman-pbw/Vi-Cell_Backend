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


#define	ALLOW_ZERO_TOKENS

static const std::string MODULENAME = "DBif_Parse";


bool DBifImpl::DecodeBooleanRecordField( CRecordset& resultrec, CString & fieldTag )
{
	CString valStr = _T( "" );      // required for retrieval from recordset
	std::string val = "";
	bool boolFlag = false;
	CDBVariant fldRec;

	resultrec.GetFieldValue( fieldTag, fldRec );
	if ( fldRec.m_dwType == DBVT_BOOL )
	{
		boolFlag = ( fldRec.m_boolVal == TRUE ) ? true : false;
	}
	else
	{
		if ( fldRec.m_dwType == DBVT_STRING )
		{
			valStr = *fldRec.m_pstring;
		}
		else if ( fldRec.m_dwType == DBVT_ASTRING )
		{
			valStr = *fldRec.m_pstringA;
		}
		else if ( fldRec.m_dwType == DBVT_WSTRING )
		{
			valStr = *fldRec.m_pstringW;
		}

		if ( !valStr.IsEmpty() )
		{
			val = CT2A( valStr );
			BOOL boolVal = std::atol( val.c_str() );
			boolFlag = ( boolVal == TRUE ) ? true : false;
		}
	}
	return boolFlag;
}

////////////////////////////////////////////////////////////////////////////////
// Internal object parsing methods
////////////////////////////////////////////////////////////////////////////////

bool DBifImpl::ParseWorklist( DBApi::eQueryResult& queryresult, DB_WorklistRecord& wlrec,
							  CRecordset& resultrec, std::vector<std::string>& taglist, int32_t get_sets )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> idList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	wlrec.SSList.clear();

	for ( tagIndex = 0; tagIndex < tagListSize && parseOk; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == WL_IdNumTag )						// "WorklistIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			wlrec.WorklistIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == WL_IdTag )						// "WorklistID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.WorklistId );
		}
		else if ( tag == WL_StatusTag )					// "WorklistStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.WorklistStatus = static_cast< uint32_t >( fldRec.m_lVal );
		}
		else if ( tag == WL_NameTag )					// "WorklistName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			wlrec.WorklistNameStr = val;
		}
		else if ( tag == WL_CommentsTag )				// "ListComments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			wlrec.ListComments = val;
		}
		else if ( tag == WL_InstSNTag )					// "InstrumentSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			wlrec.InstrumentSNStr = val;
		}
		else if ( tag == WL_CreateUserIdTag )			// "CreationUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.CreationUserId );

			// TEMPORARY: don't fail on bad user id for now...
			if ( !GuidValid( wlrec.CreationUserId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( wlrec.CreationUserId, wlrec.CreationUserNameStr );
		}
		else if ( tag == WL_RunUserIdTag )				// "RunUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.RunUserId );

			// TEMPORARY: don't fail on bad user id for now...
			if ( !GuidValid( wlrec.RunUserId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( wlrec.RunUserId, wlrec.RunUserNameStr );
		}
		else if ( tag == WL_RunDateTag )				// "RunDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( wlrec.RunDateTP, val );
		}
		else if ( tag == WL_AcquireSampleTag )			// "AcquireSample"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			wlrec.AcquireSample = boolFlag;
		}
		else if ( tag == WL_CarrierTypeTag )			// "CarrierType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.CarrierType = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_PrecessionTag )				// "ByColumn"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			wlrec.ByColumn = boolFlag;
		}
		else if ( tag == WL_SaveImagesTag )				// "SaveImages"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.SaveNthImage = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_WashTypeTag )				// "WashType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.WashTypeIndex = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_DilutionTag )				// "Dilution"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.WashTypeIndex = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_DfltSetNameTag )			// "DefaultSetName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			wlrec.SampleSetNameStr = val;
		}
		else if ( tag == WL_DfltItemNameTag )			// "DefaultItemName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			wlrec.SampleItemNameStr = val;
		}
		else if ( tag == WL_ImageAnalysisParamIdTag )	// "ImageAnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.ImageAnalysisParamId );
		}
		else if ( tag == WL_AnalysisDefIdTag )			// "AnalysisDefinitionID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.AnalysisDefId );
		}
		else if ( tag == WL_AnalysisDefIdxTag )			// "AnalysisDefinitionIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.AnalysisDefIndex = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_AnalysisParamIdTag )		// "AnalysisParameterID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.AnalysisParamId );
		}
		else if ( tag == WL_CellTypeIdTag )				// "CellTypeID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.CellTypeId );
		}
		else if ( tag == WL_CellTypeIdxTag )			// "CellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			wlrec.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == WL_BioProcessIdTag )			// "BioProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.BioProcessId );
		}
		else if ( tag == WL_QcProcessIdTag )			// "QcProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.QcProcessId );
		}
		else if ( tag == WL_WorkflowIdTag )				// WorkflowID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, wlrec.WorkflowId );
		}
		else if ( tag == WL_SampleSetCntIdTag )			// "SampleSetCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.SampleSetCount = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_ProcessedSetCntTag )		// "ProcessedSetCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			wlrec.ProcessedSetCount = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == WL_SampleSetIdListTag )		// "SampleSetIDList"
		{
			uuid__t itemUuid = {};

			idList.clear();
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				idList.push_back( itemUuid );
			}
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			wlrec.Protected = boolFlag;
		}
	}

	if ( parseOk && get_sets )
	{
		if ( get_sets == FirstLevelObjs )
		{
			get_sets = NoSubObjs;
		}

		// allow worklists to have no sample sets...
		if ( idList.size() > 0 )
		{
			DB_SampleSetRecord ssr = {};
			size_t listSize = idList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( idList.at( i ) ) )
				{
					retrieveResult = GetSampleSet( ssr, idList.at( i ), NO_ID_NUM, get_sets );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						wlrec.SSList.push_back( ssr );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk )
			{
				wlrec.SampleSetCount = static_cast< int16_t >( listSize );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseSampleSet( DBApi::eQueryResult& queryresult, DB_SampleSetRecord& ssrec,
							   CRecordset& resultrec, std::vector<std::string>& taglist, int32_t get_items )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t ssSetCnt = -1;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> idList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	ssrec.SSItemsList.clear();

	for ( tagIndex = 0; tagIndex < tagListSize && parseOk; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SS_IdNumTag )						// "SampleSetIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ssrec.SampleSetIdNum = std::atoll( val.c_str());
		}
		else if ( tag == SS_IdTag )						// "SampleSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssrec.SampleSetId );
		}
		else if ( tag == SS_StatusTag )					// "SampleSetStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssrec.SampleSetStatus = static_cast< uint32_t >( fldRec.m_lVal );
		}
		else if ( tag == SS_NameTag )					// "SampleSetName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssrec.SampleSetNameStr = val;
		}
		else if ( tag == SS_LabelTag )					// "SampleSetLabel"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssrec.SampleSetLabel = val;
		}
		else if ( tag == SS_CommentsTag )				// "Comments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssrec.Comments = val;
		}
		else if ( tag == SS_CarrierTypeTag )			// "CarrierType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssrec.CarrierType = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == SS_OwnerIdTag )				// "OwnerID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssrec.OwnerId );

			if ( !GuidValid( ssrec.OwnerId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( ssrec.OwnerId, ssrec.OwnerNameStr );
		}
		else if ( tag == SS_CreateDateTag )				// "CreateDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( ssrec.CreateDateTP, val );
		}
		else if ( tag == SS_ModifyDateTag )				// "ModifyDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( ssrec.ModifyDateTP, val );
		}
		else if ( tag == SS_RunDateTag )				// "RunDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( ssrec.RunDateTP, val );
		}
		else if ( tag == SS_WorklistIdTag )				// "WorklistID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssrec.WorklistId );
		}
		else if ( tag == SS_SampleItemCntTag )			// "SampleItemCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssrec.SampleItemCount = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SS_ProcessedItemCntTag )		// "ProcessedItemCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssrec.ProcessedItemCount = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SS_SampleItemIdListTag )		// "SampleItemIDList"
		{
			uuid__t itemUuid = {};

			idList.clear();
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				idList.push_back( itemUuid );
			}
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			ssrec.Protected = boolFlag;
		}
	}

	if ( parseOk && get_items )
	{
		// allow sample sets to have 0 elements in their item list...
		if ( idList.size() >= 0 )
		{
			DB_SampleItemRecord ssir = {};
			size_t listSize = idList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( idList.at( i ) ) )
				{
					retrieveResult = GetSampleItem( ssir, idList.at( i ), NO_ID_NUM );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						ssrec.SSItemsList.push_back( ssir );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}
	else
	{
		// Set count to zero when "get_items" is NoSubObjs.
		ssrec.SampleItemCount = 0;
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseSampleItem( DBApi::eQueryResult& queryresult, DB_SampleItemRecord& ssirec,
								CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SI_IdNumTag )							// "SampleItemIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ssirec.SampleItemIdNum = std::atoll( val.c_str());
		}
		else if ( tag == SI_IdTag )							// "SampleItemID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.SampleItemId );
		}
		else if ( tag == SI_StatusTag )						// "SampleItemStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssirec.SampleItemStatus = static_cast< uint32_t >( fldRec.m_lVal );
		}
		else if ( tag == SI_NameTag )						// "SampleItemName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssirec.SampleItemNameStr = val;
		}
		else if ( tag == SI_CommentsTag )					// "Comments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssirec.Comments = val;
		}
		else if ( tag == SI_RunDateTag )					// RunDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( ssirec.RunDateTP, val );
		}
		else if ( tag == SI_SampleSetIdTag )				// "SampleSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.SampleSetId );
		}
		else if ( tag == SI_SampleIdTag )					// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.SampleId );
		}
		else if ( tag == SI_SaveImagesTag )					// "SaveImages"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssirec.SaveNthImage = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SI_WashTypeTag )					// "WashType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssirec.WashTypeIndex = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SI_DilutionTag )					// "Dilution"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssirec.Dilution = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SI_LabelTag )						// "ItemLabel"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ssirec.ItemLabel = val;
		}
		else if ( tag == SI_ImageAnalysisParamIdTag )		// "ImageAnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.ImageAnalysisParamId );
		}
		else if ( tag == SI_AnalysisDefIdTag )				// "AnalysisDefinitionID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.AnalysisDefId );
		}
		else if ( tag == SI_AnalysisDefIdxTag )				// "AnalysisDefinitionIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ssirec.AnalysisDefIndex = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == SI_AnalysisParamIdTag )			// "AnalysisParameterID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.AnalysisParamId );
		}
		else if ( tag == SI_CellTypeIdTag )					// "CellTypeID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.CellTypeId );
		}
		else if ( tag == SI_CellTypeIdxTag )				// "CellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ssirec.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == SI_BioProcessIdTag )				// "BioProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.BioProcessId );

			// BioProcess ID may be blank
			if ( !GuidValid( ssirec.BioProcessId ) )
			{
				continue;
			}

			parseOk = GetProcessNameString( DBApi::eListType::BioProcessList, ssirec.BioProcessId, ssirec.BioProcessNameStr );
		}
		else if ( tag == SI_QcProcessIdTag )				// "QcProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.QcProcessId );

			// QcProcess ID may be blank
			if ( !GuidValid( ssirec.QcProcessId ) )
			{
				continue;
			}

			parseOk = GetProcessNameString( DBApi::eListType::QcProcessList, ssirec.QcProcessId, ssirec.QcProcessNameStr );
		}
		else if ( tag == SI_WorkflowIdTag )					// "WorkflowID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ssirec.WorkflowId );
		}
		else if ( tag == SI_SamplePosTag )					// "SamplePosition"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );

			// expects a string formatted as "r-cc-n", where:
			//    'r' is the numeric row value; need to convert to alpha format
			//    'cc' is the two-digit column value with leading 0s
			//    and 'n' is the optional rotation number
			// 
			std::string sepStr = "";
			std::string sepChars = "-";
			char* pSepChars = ( char* ) sepChars.c_str();
			std::string trimChars = " ";
			std::vector<std::string> tokenlist = {};
			int32_t tokenCnt = 0;

			ssirec.SampleRow = 'Q';			// use a default value invalid for any carrier
			ssirec.SampleCol = 0;			// 0 is not a valid position, to indicate failure
			ssirec.RotationCount = 0;		// rotation 0 is the first rotation

			tokenCnt = ParseStringToTokenList( tokenlist, val, sepStr, pSepChars, true, trimChars );
			if ( tokenCnt >= 2 )
			{
				if ( tokenlist.at( 0 ).length() > 0 )
				{
					ssirec.SampleRow = tokenlist.at( 0 ).at( 0 );
				}
				ssirec.SampleCol = std::stoi( tokenlist.at( 1 ) );
				if ( tokenCnt > 2 )
				{
					ssirec.RotationCount = std::stoi( tokenlist.at( 2 ) );
				}
			}
			else
			{
				retrieveResult = DBApi::eQueryResult::MissingQueryKey;
				parseOk = false;
				break;
			}
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			ssirec.Protected = boolFlag;
		}
	}

	if ( !parseOk )
	{
		queryresult = DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseSample( DBApi::eQueryResult& queryresult, DB_SampleRecord& samplerec,
							CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	samplerec.ReagentTypeNameList.clear();
	samplerec.ReagentPackNumList.clear();
	samplerec.PackLotNumList.clear();
	samplerec.PackLotExpirationList.clear();
	samplerec.PackInServiceList.clear();
	samplerec.PackServiceExpirationList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SM_IdNumTag )								// "SampleIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			samplerec.SampleIdNum = std::atoll( val.c_str());
		}
		else if ( tag == SM_IdTag )								// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.SampleId );
		}
		else if ( tag == SM_StatusTag )							// "SampleStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			samplerec.SampleStatus = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == SM_NameTag )							// "SampleName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			samplerec.SampleNameStr = val;
		}
		else if ( tag == SM_CellTypeIdTag )						// "CellTypeID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.CellTypeId );
		}
		else if ( tag == SM_CellTypeIdxTag )					// "CellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			samplerec.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == SM_AnalysisDefIdTag )					// "AnalysisDefinitionID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.AnalysisDefId );
		}
		else if ( tag == SM_AnalysisDefIdxTag )					// "AnalysisDefinitionIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			samplerec.AnalysisDefIndex = static_cast<uint16_t>( fldRec.m_lVal );
		}
		else if ( tag == SM_LabelTag )							// "Label"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			samplerec.Label = val;
		}
		else if ( tag == SM_BioProcessIdTag )					// "BioProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.BioProcessId );
		}
		else if ( tag == SM_QcProcessIdTag )					// "QcProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.QcId );
		}
		else if ( tag == SM_WorkflowIdTag )						// "WorkflowID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.WorkflowId );
		}
		else if ( tag == SM_CommentsTag )						// "Comments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			samplerec.CommentsStr = val;
		}
		else if ( tag == SM_WashTypeTag )						// "WashType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			samplerec.WashTypeIndex = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SM_DilutionTag )						// "Dilution"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			samplerec.Dilution = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SM_OwnerUserIdTag )					// "OwnerUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.OwnerUserId );

			if ( !GuidValid( samplerec.OwnerUserId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( samplerec.OwnerUserId, samplerec.OwnerUserNameStr );
		}
		else if ( tag == SM_RunUserTag )						// "RunUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.RunUserId );

			if ( !GuidValid( samplerec.RunUserId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( samplerec.RunUserId, samplerec.RunUserNameStr );
		}
		else if ( tag == SM_AcquireDateTag )					// "AcquisitionDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( samplerec.AcquisitionDateTP, val );
		}
		else if ( tag == SM_ImageSetIdTag )						// "ImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.ImageSetId );
		}
		else if ( tag == SM_DustRefSetIdTag )					// "DistRefImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.DustRefImageSetId );
		}
		else if ( tag == SM_InstSNTag )							// "InstrumentSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			samplerec.AcquisitionInstrumentSNStr = val;
		}
		else if ( tag == SM_ImageAnalysisParamIdTag )			// ImageAnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, samplerec.ImageAnalysisParamId );
		}
		else if ( tag == SM_NumReagentsTag )					// "NumReagents"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			samplerec.NumReagents = static_cast< uint16_t >( fldRec.m_iVal );
		}
		else if ( tag == SM_ReagentTypeNameListTag )			// "ReagentTypeNameList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );

#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, samplerec.ReagentTypeNameList );
		}
		else if ( tag == SM_ReagentPackNumListTag )				// "ReagentPackNumList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, samplerec.ReagentPackNumList );
		}
		else if ( tag == SM_PackLotNumTag )						// "PackLotNumList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, samplerec.PackLotNumList );
		}
		else if ( tag == SM_PackLotExpirationListTag )			// "PackLotExpirationList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;
			uint64_t days = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				days = std::atoll( val.c_str() );
				samplerec.PackLotExpirationList.push_back( days );
			}
		}
		else if ( tag == SM_PackInServiceListTag )				// "PackInServiceList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;
			uint64_t days = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				days = std::atoll( val.c_str() );
				samplerec.PackInServiceList.push_back( days );
			}
		}
		else if ( tag == SM_PackServiceExpirationListTag )		// "PackServiceExpirationList"
		{
			std::vector<std::string> tokenList = {};
			int32_t tokenCnt = 0;
			uint64_t days = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				days = std::atoll( val.c_str() );
				samplerec.PackServiceExpirationList.push_back( days );
			}
		}
		else if ( tag == ProtectedTag )							// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			samplerec.Protected = boolFlag;
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}


bool DBifImpl::ParseAnalysis( DBApi::eQueryResult& queryresult, DB_AnalysisRecord& analysis,
							  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	uuid__t objId = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> seqIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == AN_IdNumTag )								// "AnalysisIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			analysis.AnalysisIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == AN_IdTag )								// "AnalysisID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.AnalysisId );
		}
		else if ( tag == AN_SampleIdTag )						// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.SampleId );
		}
		else if ( tag == AN_ImageSetIdTag )						// "ImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.ImageSetId );
		}
		else if ( tag == AN_SummaryResultIdTag )				// "SummaryResultID"
		{
			DB_SummaryResultRecord srRec = {};

			ClearGuid( objId );
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, objId );
			if ( GuidValid( objId ) )
			{
				retrieveResult = GetSummaryResult( srRec, objId, NO_ID_NUM );
				if ( retrieveResult == DBApi::eQueryResult::QueryOk )
				{
					analysis.SummaryResult = srRec;
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::NoResults;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
				break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
			}
		}
		else if ( tag == AN_SResultIdTag )						// "SResultID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.SResultId );
		}
		else if ( tag == AN_RunUserIdTag )						// "RunUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.AnalysisUserId );

			if ( !GuidValid( analysis.AnalysisUserId ) )
			{
#ifndef ALLOW_ID_FAILS
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
#endif // ALLOW_ID_FAILS
				continue;
			}

			parseOk = GetUserNameString( analysis.AnalysisUserId, analysis.AnalysisUserNameStr );
		}
		else if ( tag == SM_AcquireDateTag )					// "AcquisitionDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( analysis.AnalysisDateTP, val );
		}
		else if ( tag == AN_InstSNTag )							// "InstrumentSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			analysis.InstrumentSNStr = val;
		}
		else if ( tag == AN_BioProcessIdTag )					// "BioProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.BioProcessId );
		}
		else if ( tag == AN_QcProcessIdTag )					// "QcProcessID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.QcId );
		}
		else if ( tag == AN_WorkflowIdTag )						// "WorkflowID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, analysis.WorkflowId );
		}
		else if ( tag == AN_ImageSequenceCntTag )				// "ImageSequenceCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			analysis.ImageSequenceCount = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AN_ImageSequenceIdListTag )			// "ImageSequenceIDList"
		{
			int32_t tokenCnt = 0;
			std::vector<std::string> tokenList = {};
			uuid__t imgRecUuid = {};

			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			seqIdList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( imgRecUuid );
				uuid__t_from_DB_UUID_Str( val, imgRecUuid );
				seqIdList.push_back( imgRecUuid );
			}
		}
		else if ( tag == ProtectedTag )							// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			analysis.Protected = boolFlag;
		}
	}

#if(0)
	analysis.ImageResultList.clear();
	if ( parseOk )
	{
		size_t listSize = resultIdList.size();

		if ( listSize != analysis.NumImageResults )
		{
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
		}
		else
		{
			// allow sample sets to have 0 elements in their item list...
			if ( listSize >= 0 )
			{
				DB_ImageResultRecord irRec = {};
				for ( int32_t i = 0; i < listSize; i++ )
				{
					ClearGuid( objId );
					objId = resultIdList.at( i );
					if ( GuidValid( objId ) )
					{
						retrieveResult = GetImageResult( irRec, objId, NO_ID_NUM );
						if ( retrieveResult == DBApi::eQueryResult::QueryOk )
						{
							analysis.ImageResultList.push_back( irRec );
						}
						else
						{
#ifdef ALLOW_ID_FAILS
							retrieveResult = DBApi::eQueryResult::QueryOk;
#else
							parseOk = false;
							retrieveResult = DBApi::eQueryResult::NoResults;
							break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
						}
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
			}
		}
	}
#endif

	analysis.ImageSequenceList.clear();
	if ( parseOk )
	{
		size_t listSize = seqIdList.size();

		if ( listSize != analysis.ImageSequenceCount )
		{
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
		}
		else
		{
			// allow sample sets to have 0 elements in their item list...
			if ( listSize >= 0 )
			{
				for ( int32_t i = 0; i < listSize; i++ )
				{
					DB_ImageSeqRecord seqRec = {};

					ClearGuid( objId );
					objId = seqIdList.at( i );
					if ( GuidValid( objId ) )
					{
						retrieveResult = GetImageSequence( seqRec, objId, NO_ID_NUM );
						if ( retrieveResult == DBApi::eQueryResult::QueryOk )
						{
							analysis.ImageSequenceList.push_back( seqRec );
						}
						else
						{
#ifdef ALLOW_ID_FAILS
							retrieveResult = DBApi::eQueryResult::QueryOk;
#else
							parseOk = false;
							retrieveResult = DBApi::eQueryResult::NoResults;
							break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
						}
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
			}
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseSummaryResult( DBApi::eQueryResult& queryresult, DB_SummaryResultRecord& sr,
								   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	sr.SignatureList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == RS_IdNumTag )							// "SummaryResultIdNum" )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			sr.SummaryResultIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == RS_IdTag )							// "SummaryResultID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.SummaryResultId );
		}
		else if ( tag == RS_SampleIdTag )					// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.SampleId );
		}
		else if ( tag == RS_ImageSetIdTag )					// "ImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.ImageSetId );
		}
		else if ( tag == RS_AnalysisIdTag )					// "AnalysisID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.AnalysisId );
		}
		else if ( tag == RS_ResultDateTag )					// "ResultDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( sr.ResultDateTP, val );
		}
		else if ( tag == RS_SigListTag )					// "SignatureList"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces and element container quotes; DO NOT REMOVE PARENTHESES AT THIS TIME!
			TrimStr( "{}\"", val );

			int32_t arrayCnt = 0;
			std::vector<std::string> signatureArrayStrList;

			signatureArrayStrList.clear();

			// now separate the array of signatures into the individual signature info
			arrayCnt = ParseCompositeArrayStrToStrList( signatureArrayStrList, val );
			if ( arrayCnt != signatureArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string signatureArrayStr = "";
			std::vector<std::string> elementStrList = {};
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				db_signature_t sig = {};
				int32_t eCnt = 0;
				std::string sigUserName = "";
//				uuid__t sigUserId = {};
				std::string sigShortTag = "";
				std::string sigLongTag = "";
				system_TP sigTime = {};
				std::string sigHash = "";

				signatureArrayStr.clear();
				signatureArrayStr = signatureArrayStrList.at( aidx );

				// next remove any leading and trailing array parentheses and quotes
				TrimStr( "{}()\"", signatureArrayStr );

				// Now break out the composite elements of this blob
				elementStrList.clear();
				eCnt = ParseSignatureCompositeElementStrToStrList( elementStrList, signatureArrayStr );
				if ( eCnt != 5 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

//				ClearGuid( sigUserId );

				// now parse the extracted signature elements
				for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
				{
					tokenStr.clear();
					tokenStr = elementStrList.at( eidx );

					switch ( eidx )
					{
						case 0:			// signature element enum 0 = eSigUserName
							DeSanitizeDataString( tokenStr );
							sigUserName = tokenStr;
							break;

						case 1:			// signature element enum 1 = eSigShortTag
							DeSanitizeDataString( tokenStr );
							sigShortTag = tokenStr;
							break;

						case 2:			// signature element enum 2 = eSigLongTag
							DeSanitizeDataString( tokenStr );
							sigLongTag = tokenStr;
							break;

						case 3:			// signature element enum 3 = eSigTime
							GetTimePtFromDbTimeString( sigTime, tokenStr );
							break;

						case 4:			// signature element enum 4 = eSigHash
							DeSanitizeDataString( tokenStr );
							sigHash = tokenStr;
							break;
					}
				}

				sig.userName = sigUserName;
				sig.shortSignature = sigShortTag;
				sig.longSignature = sigLongTag;
				sig.signatureTime = sigTime;
				sig.signatureHash = sigHash;

				sr.SignatureList.push_back( sig );
			}
		}
		else if ( tag == RS_ImgAnalysisParamIdTag )			// "ImageAnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.ImageAnalysisParamId );
		}
		else if ( tag == RS_AnalysisDefIdTag )				// "AnalysisDefinitionID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.AnalysisDefId );
		}
		else if ( tag == RS_AnalysisParamIdTag )			// "AnalysisParameterID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.AnalysisParamId );
		}
		else if ( tag == RS_CellTypeIdTag )					// "CellTypeID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.CellTypeId );
		}
		else if ( tag == RS_CellTypeIdxTag )				// "CellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			sr.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == RS_StatusTag )						// "ProcessingStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.ProcessingStatus = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == RS_TotCumulativeImgsTag )			// "TotCumulativeImages"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.TotalCumulativeImages = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == RS_TotCellsGPTag )					// "TotalCellsGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.TotalCells_GP = static_cast<uint32_t>( fldRec.m_lVal );
		}
		else if ( tag == RS_TotCellsPOITag )				// "TotalCellsPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.TotalCells_POI = static_cast<uint32_t>( fldRec.m_lVal );
		}
		else if ( tag == RS_POIPopPercentTag )				// "POIPopulationPercent"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.POI_PopPercent = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_CellConcGPTag )					// "CellConcGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.CellConc_GP = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_CellConcPOITag )				// "CellConcPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.CellConc_POI = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_AvgDiamGPTag )					// "AvgDiamGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgDiam_GP = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_AvgDiamPOITag )					// "AvgDiamPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgDiam_POI = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_AvgCircularityGPTag )			// "AvgCircularityGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgCircularity_GP = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_AvgCircularityPOITag )			// "AvgCircularityPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgCircularity_POI = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_CoeffOfVarTag )					// "CoefficientOfVariance"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.CoefficientOfVariance = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == RS_AvgCellsPerImgTag )				// "AvgCellsPerImage"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgCellsPerImage = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == RS_AvgBkgndIntensityTag )			// AvgBkgndIntensity"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.AvgBkgndIntensity = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == RS_TotBubbleCntTag )				// "TotalBubbleCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.TotalBubbleCount = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == RS_LrgClusterCntTag )				// "LargeClusterCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			sr.LargeClusterCount = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if (tag == RS_QcStatusTag)						// "QcStatus"
		{
			resultrec.GetFieldValue(tagStr, fldRec);
			sr.QcStatus = static_cast<uint16_t>(fldRec.m_iVal);
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			sr.Protected = boolFlag;
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseDetailedResult( DBApi::eQueryResult& queryresult, DB_DetailedResultRecord& dr,
										 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == RD_IdNumTag )									// "DetailedResultIdNum" )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			dr.DetailedResultIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == RD_IdTag )									// "DetailedResultID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, dr.DetailedResultId );
		}
		else if ( tag == RD_SampleIdTag )							// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, dr.SampleId );
		}
		else if ( tag == RD_ImageIdTag )							// "ImageSequenceID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, dr.ImageId );
		}
		else if ( tag == RD_AnalysisIdTag )							// "AnalysisID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, dr.AnalysisId );
		}
		else if ( tag == RD_OwnerIdTag )							// "OwnerID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, dr.OwnerUserId );
		}
		else if ( tag == RD_ResultDateTag )							// "ResultDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( dr.ResultDateTP, val );
		}
		else if ( tag == RD_ProcessingStatusTag )					// "ProcessingStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.ProcessingStatus = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == RD_TotCumulativeImgsTag )					// "TotCumulativeImages"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.TotalCumulativeImages = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == RD_TotCellsGPTag )							// "TotalCellsGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.TotalCells_GP = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == RD_TotCellsPOITag )						// "TotalCellsPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.TotalCells_POI = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == RD_POIPopPercentTag )						// "POIPopulationPercent"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.POI_PopPercent = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_CellConcGPTag )							// "CellConcGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.CellConc_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_CellConcPOITag )						// "CellConcPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.CellConc_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgDiamGPTag )							// "AvgDiamGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgDiam_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgDiamPOITag )							// "AvgDiamPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgDiam_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgCircularityGPTag )					// "AvgCircularityGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgCircularity_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgCircularityPOITag )					// "AvgCircularityPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgCircularity_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgSharpnessGPTag )						// "AvgSharpnessGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgSharpness_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgSharpnessPOITag )					// "AvgSharpnessPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgSharpness_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgEccentricityGPTag )					// "AvgEccentricityGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgEccentricity_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgEccentricityPOITag )					// "AvgEccentricityPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgEccentricity_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgAspectRatioGPTag )					// "AvgAspectRatioGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgAspectRatio_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgAspectRatioPOITag )					// "AvgAspectRatioPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgAspectRatio_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgRoundnessGPTag )						// "AvgRoundnessGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgRoundness_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgRoundnessPOITag )					// "AvgRoundnessPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgRoundness_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgRawCellSpotBrightnessGPTag )			// "AvgRawCellSpotBrightnessGP"
		{
		resultrec.GetFieldValue( tagStr, fldRec );
		dr.AvgRawCellSpotBrightness_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgRawCellSpotBrightnessPOITag )		// "AvgRawCellSpotBrightnessPOI"
		{
		resultrec.GetFieldValue( tagStr, fldRec );
		dr.AvgRawCellSpotBrightness_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgCellSpotBrightnessGPTag )			// "AvgCellSpotBrightnessGP"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgCellSpotBrightness_GP = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgCellSpotBrightnessPOITag )			// "AvgCellSpotBrightnessPOI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgCellSpotBrightness_POI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_AvgBkgndIntensityTag )					// AvgBkgndIntensity"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.AvgBkgndIntensity = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == RD_TotBubbleCntTag )						// "TotalBubbleCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.TotalBubbleCount = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == RD_LrgClusterCntTag )						// "LargeClusterCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			dr.LargeClusterCount = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == ProtectedTag )								// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			dr.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseImageResult( DBApi::eQueryResult& queryresult, DB_ImageResultRecord& irr,
								 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t numBlobs = 0;
	int32_t numClusters = 0;
	CDBVariant fldRec = {};
	uuid__t objId = {};
	std::vector<uuid__t> blobIdList = {};
	std::vector<uuid__t> clusterIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	std::vector< blob_data_t> irBlobInfoList = {};
	std::vector< blob_data_t> irBlobCenterList = {};
	std::vector< blob_data_t> irBlobOutlineList = {};
	std::vector< cluster_data_t> irClusterCellCntList = {};
	std::vector< cluster_data_t> irClusterPolygonList = {};
	std::vector< cluster_data_t> irClusterRectList = {};

	irr.BlobDataList.clear();
	irr.ClusterDataList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == RI_IdNumTag )									// "ResultIdNum" )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			irr.ResultIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == RI_IdTag )									// "ResultID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, irr.ResultId );
		}
		else if ( tag == RI_SampleIdTag )							// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, irr.SampleId );
		}
		else if ( tag == RI_ImageIdTag )							// "ImageSequenceID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, irr.ImageId );
		}
		else if ( tag == RI_AnalysisIdTag )							// "AnalysisID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, irr.AnalysisId );
		}
		else if ( tag == RI_ImageSeqNumTag )						// "ImageSeqNum"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			irr.ImageSeqNum = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == RI_DetailedResultIdTag )
		{
			DB_DetailedResultRecord drRec = {};

			ClearGuid( objId );
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, objId );

			if ( GuidValid( objId ) )
			{
				retrieveResult = GetDetailedResult( drRec, objId, NO_ID_NUM );
				if ( retrieveResult == DBApi::eQueryResult::QueryOk )
				{
					irr.DetailedResult = drRec;
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::NoResults;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
				break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
			}
		}
		else if ( tag == RI_MaxFlChanPeakMapTag )					// "MaxNumOfPeaksFlChanMap"

		{
			int32_t arrayCnt = 0;
			int32_t tokenCnt = 0;
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";

			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );

			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				tokenStr = tokenList.at( i );

				int16_t chan = 0;
				int16_t peaks = 0;

				tokenStr = tokenList.at( 0 );
				chan = static_cast<int16_t>( std::stoi( tokenStr ) );

				tokenStr = tokenList.at( 1 );
				peaks = static_cast<int16_t>( std::stoi( tokenStr ) );

				irr.MaxNumOfPeaksFlChansMap.emplace( chan, peaks );
			}
		}
		else if ( tag == RI_NumBlobsTag )							// "NumBlobs"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			numBlobs = static_cast<int16_t>( fldRec.m_iVal );
			irr.NumBlobs = numBlobs;
		}
		else if ( tag == RI_BlobInfoListStrTag )					// "BlobInfoListStr"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove any leading and trailing array brackets; DO NOT REMOVE PARENTHESES AT THIS TIME!
			std::string trimChars = "[]";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> blobInfoArrayStrList = {};

			blobInfoArrayStrList.clear();

			// next, separate the array of arrays of blob info into the individual blob info arrays
			std::string sepStr = "],[";
			trimChars = "";

			// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
			arrayCnt = ParseStringToTokenList( blobInfoArrayStrList, val, sepStr, nullptr, true, trimChars );
			if ( arrayCnt != blobInfoArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string blobInfoArrayStr = "";
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				blob_data_t irBlobInfo = {};

				blobInfoArrayStr.clear();
				blobInfoArrayStr = blobInfoArrayStrList.at( aidx );

				// clean up the characteristics array string...
				trimChars = "()";					// to remove the leading and trailing element delimiter parantheses
				TrimStr( trimChars, blobInfoArrayStr );

				std::vector<std::string> characteristicsArrayStrList = {};
				int32_t characteristicsCnt = 0;

				// separate the array of blob info elements into individual blob info elements
				characteristicsCnt = ParseMultiArrayStrToStrList( characteristicsArrayStrList, blobInfoArrayStr );
				if ( characteristicsCnt != characteristicsArrayStrList.size() )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t cidx = 0; cidx < characteristicsCnt; cidx++ )
				{
					Characteristic_t bci = {};
					float pairValue = 0.0;
					blob_info_pair biPair = {};
					int16_t key = 0;
					int16_t skey = 0;
					int16_t sskey = 0;

					tokenStr.clear();
					tokenStr = characteristicsArrayStrList.at( cidx );

					tokenList.clear();
					tokenCnt = ParseArrayStrToStrList( tokenList, tokenStr );
					if ( tokenCnt != 4 )	// each characteristic should have 4 elements
					{
						retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
						parseOk = false;
						break;
					}

					tokenStr = tokenList.at( 0 );
					key = static_cast<int16_t>( std::stoi( tokenStr ) );

					tokenStr = tokenList.at( 1 );
					skey = static_cast<int16_t>( std::stoi( tokenStr ) );

					tokenStr = tokenList.at( 2 );
					sskey = static_cast<int16_t>( std::stoi( tokenStr ) );

					tokenStr = tokenList.at( 3 );
					pairValue = static_cast<float>( std::stof( tokenStr ) );

					bci._Myfirst._Val = key;
					bci._Get_rest()._Myfirst._Val = skey;
					bci._Get_rest()._Get_rest()._Myfirst._Val = sskey;

					biPair.first = bci;
					biPair.second = pairValue;

					irBlobInfo.blob_info.push_back( biPair );
				}
				irBlobInfoList.push_back( irBlobInfo );
			}
		}
		else if ( tag == RI_BlobCenterListStrTag )					// "BlobCenterListStr"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// next remove any leading and trailing array brackets and the leading and trailing array element designators
			std::string trimChars = "[]()";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> blobCenterArrayStrList = {};

			blobCenterArrayStrList.clear();

			// next, separate the composite array of blob centers into the list of individual blob cneter objects
			arrayCnt = ParseMultiArrayStrToStrList( blobCenterArrayStrList, val );
			if ( arrayCnt != blobCenterArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string blobCenterArrayStr = "";
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				blob_data_t irBlobCenter = {};

				blobCenterArrayStr.clear();
				blobCenterArrayStr = blobCenterArrayStrList.at( aidx );

				tokenList.clear();
				tokenCnt = ParseArrayStrToStrList( tokenList, blobCenterArrayStr );
				if ( tokenCnt != 2 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				tokenStr = tokenList.at( 0 );
				irBlobCenter.blob_center.startx = static_cast<int16_t>( std::stoi( tokenStr ) );

				tokenStr = tokenList.at( 1 );
				irBlobCenter.blob_center.starty = static_cast<int16_t>( std::stoi( tokenStr ) );

				irBlobCenterList.push_back( irBlobCenter );
			}
		}
		else if ( tag == RI_BlobOutlineListStrTag )					// "BlobOutlineListStr"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// next remove any leading and trailing array brackets; DO NOT REMOVE PARENTHESES AT THIS TIME!
			std::string trimChars = "[]";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> blobOutlineArrayStrList = {};

			blobOutlineArrayStrList.clear();

			// next, separate the array of arrays of blob info into the individual blob info arrays
			std::string sepStr = "],[";
			trimChars = "";

			// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
			arrayCnt = ParseStringToTokenList( blobOutlineArrayStrList, val, sepStr, nullptr, true, trimChars );
			if ( arrayCnt != blobOutlineArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string blobOutlineArrayStr = "";
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				blob_data_t irBlobOutline = {};
				int32_t eCnt = 0;

				blobOutlineArrayStr.clear();
				blobOutlineArrayStr = blobOutlineArrayStrList.at( aidx );

				// cleanup the outline array string
				trimChars = "()";					// to remove the leading and trailing element delimiter parantheses
				TrimStr( trimChars, blobOutlineArrayStr );

				// now parse the extracted blob outline array
				std::vector<std::string> outlineVertexArrayStrList = {};
				int32_t vertexCnt = 0;

				vertexCnt = ParseMultiArrayStrToStrList( outlineVertexArrayStrList, blobOutlineArrayStr );
				if ( vertexCnt != outlineVertexArrayStrList.size() )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t ptidx = 0; ptidx < vertexCnt; ptidx++ )
				{
					blob_point pt = {};

					tokenStr.clear();
					tokenStr = outlineVertexArrayStrList.at( ptidx );

					tokenList.clear();
					tokenCnt = ParseArrayStrToStrList( tokenList, tokenStr );
					if ( tokenCnt != 2 )	// each blob point should have 2 elements
					{
						retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
						parseOk = false;
						break;
					}

					tokenStr = tokenList.at( 0 );
					pt.startx = static_cast<int16_t>( std::stoi( tokenStr ) );

					tokenStr = tokenList.at( 1 );
					pt.starty = static_cast<int16_t>( std::stoi( tokenStr ) );

					irBlobOutline.blob_outline.push_back( pt );
				}
				irBlobOutlineList.push_back( irBlobOutline );
			}
		}
		else if ( tag == RI_NumClustersTag )						// "NumClusters"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			numClusters = static_cast<int16_t>( fldRec.m_iVal );
			irr.NumClusters = numClusters;
		}
		else if ( tag == RI_ClusterCellCountListTag )				// "ClusterCellCountList"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks and the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces and element container quotes; DO NOT REMOVE PARENTHESES AT THIS TIME!
			std::string trimChars = "{}\"";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> clusterCellCntArrayStrList = {};

			clusterCellCntArrayStrList.clear();

			// first separate the array of cluster cell counets into the individual cell counts
			arrayCnt = ParseArrayStrToStrList( clusterCellCntArrayStrList, val );
			if ( arrayCnt != clusterCellCntArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				cluster_data_t irCellCntCluster = {};
				int32_t eCnt = 0;

				tokenStr.clear();
				tokenStr = clusterCellCntArrayStrList.at( aidx );

				// now parse the extracted cell count data elements
				irCellCntCluster.cell_count = static_cast<int16_t>( std::stoi( tokenStr ) );

				irClusterCellCntList.push_back( irCellCntCluster );
			}
		}
		else if ( tag == RI_ClusterPolygonListStrTag )				// "ClusterPolygonListStr"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// next remove any leading and trailing array brackets; DO NOT REMOVE PARENTHESES AT THIS TIME!
			std::string trimChars = "[]";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> clusterPolygonArrayStrList = {};

			clusterPolygonArrayStrList.clear();

			// next, separate the array of arrays of cluster outline info into the individual blob info arrays
			std::string sepStr = "],[";
			trimChars = "";

			// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
			arrayCnt = ParseStringToTokenList( clusterPolygonArrayStrList, val, sepStr, nullptr, true, trimChars );
			if ( arrayCnt != clusterPolygonArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string clusterPolygonArrayStr = "";
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				cluster_data_t irClusterPolygon = {};

				clusterPolygonArrayStr.clear();
				clusterPolygonArrayStr = clusterPolygonArrayStrList.at( aidx );

				// cleanup the outline array string
				trimChars = "()";					// to remove the leading and trailing element delimiter parantheses
				TrimStr( trimChars, clusterPolygonArrayStr );

				// now parse the extracted cluster polygon outline array
				std::vector<std::string> polygonVertexArrayStrList = {};
				int32_t vertexCnt = 0;

				vertexCnt = ParseMultiArrayStrToStrList( polygonVertexArrayStrList, clusterPolygonArrayStr );
				if ( vertexCnt != polygonVertexArrayStrList.size() )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t ptidx = 0; ptidx < vertexCnt; ptidx++ )
				{
					blob_point pt = {};

					tokenStr.clear();
					tokenStr = polygonVertexArrayStrList.at( ptidx );

					tokenList.clear();
					tokenCnt = ParseArrayStrToStrList( tokenList, tokenStr );
					if ( tokenCnt != 2 )	// each blob point should have 2 elements
					{
						retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
						parseOk = false;
						break;
					}

					tokenStr = tokenList.at( 0 );
					pt.startx = static_cast<int16_t>( std::stoi( tokenStr ) );

					tokenStr = tokenList.at( 1 );
					pt.starty = static_cast<int16_t>( std::stoi( tokenStr ) );

					irClusterPolygon.cluster_polygon.push_back( pt );
				}
				irClusterPolygonList.push_back( irClusterPolygon );
			}
		}
		else if ( tag == RI_ClusterRectListStrTag )					// "ClusterRectListStr"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// next remove any leading and trailing array brackets and the leading and trailing array element designators
			std::string trimChars = "[]()";
			TrimStr( trimChars, val );

			int32_t arrayCnt = 0;
			std::vector<std::string> clusterRectArrayStrList = {};

			clusterRectArrayStrList.clear();

			// next, separate the composite array of cluster rectangles into the list of individual cluster rectangle objects
			arrayCnt = ParseMultiArrayStrToStrList( clusterRectArrayStrList, val );
			if ( arrayCnt != clusterRectArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string clusterRectArrayStr = "";
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				cluster_data_t irClusterRect = {};

				clusterRectArrayStr.clear();
				clusterRectArrayStr = clusterRectArrayStrList.at( aidx );

				tokenList.clear();
				tokenCnt = ParseArrayStrToStrList( tokenList, clusterRectArrayStr );
				if ( tokenCnt != 4 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				// now extract each rectangle and parse the extracted cluster rectangle elements
				for ( int32_t eidx = 0; eidx < tokenCnt && parseOk; eidx++ )
				{
					tokenStr.clear();
					tokenStr = tokenList.at( eidx );

					switch ( eidx )
					{
						case 0:
						{
							irClusterRect.cluster_box.start.startx = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 1:
						{
							irClusterRect.cluster_box.start.starty = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 2:
						{
							irClusterRect.cluster_box.width = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 3:
						{
							irClusterRect.cluster_box.height = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}
					}
				}
				irClusterRectList.push_back( irClusterRect );
			}
		}
		else if ( tag == ProtectedTag )								// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			irr.Protected = boolFlag;
		}
	}

	if (parseOk)
	{
		// must have the same number of blob elements in each list...
		if ( irBlobInfoList.size() == irBlobCenterList.size() && irBlobCenterList.size() == irBlobOutlineList.size() )
		{
			blob_data_t blobItem = {};
			size_t listSize = irBlobInfoList.size();

			for ( int i = 0; i < listSize; i++ )
			{
				blob_data_t blobItem = {};
				
				blobItem.blob_info = irBlobInfoList.at( i ).blob_info;
				blobItem.blob_center = irBlobCenterList.at( i ).blob_center;
				blobItem.blob_outline = irBlobOutlineList.at( i ).blob_outline;

				irr.BlobDataList.push_back( blobItem );
			}

			size_t bdataListSize = irr.BlobDataList.size();
			if ( irr.NumBlobs != bdataListSize )
			{
				irr.NumBlobs = static_cast<int16_t>( bdataListSize );
			}
		}
		else
		{
			parseOk = false;
		}
	}

	if ( parseOk )
	{
		// must have the same number of cluster elements in each list...
		if ( irClusterCellCntList.size() == irClusterPolygonList.size() && irClusterPolygonList.size() == irClusterRectList.size() )
		{
			size_t listSize = irClusterCellCntList.size();

			for ( int i = 0; i < listSize; i++ )
			{
				cluster_data_t clusterItem = {};

				clusterItem.cell_count= irClusterCellCntList.at( i ).cell_count;
				clusterItem.cluster_polygon = irClusterPolygonList.at( i ).cluster_polygon;
				clusterItem.cluster_box = irClusterRectList.at( i ).cluster_box;

				irr.ClusterDataList.push_back( clusterItem );
			}

			size_t cdataListSize = irr.ClusterDataList.size();
			if ( irr.NumClusters != cdataListSize )
			{
				irr.NumClusters = static_cast<int16_t>( cdataListSize );
			}
		}
		else
		{
			parseOk = false;
		}
	}

	if ( parseOk )
	{
		if ( irr.NumBlobs != irr.BlobDataList.size() )
		{
			irr.NumBlobs = static_cast<int16_t>( irr.BlobDataList.size() );
		}

		if ( irr.NumClusters != irr.ClusterDataList.size() )
		{
			irr.NumClusters = static_cast<int16_t>( irr.ClusterDataList.size() );
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseSResult( DBApi::eQueryResult& queryresult, DB_SResultRecord& sr,
								   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	uuid__t objId = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> resultIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	sr.ImageResultList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SR_IdNumTag )							// "SummaryResultIdNum" )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			sr.SResultIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == SR_IdTag )							// "SummaryResultID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.SResultId );
		}
		else if ( tag == SR_SampleIdTag )					// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.SampleId );
		}
		else if ( tag == SR_AnalysisIdTag )					// "AnalysisID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sr.AnalysisId );
		}
		else if ( tag == SR_ProcessingSettingsIdTag )		// "ProcessingSettingsID"
		{
			DB_AnalysisInputSettingsRecord aisRec = {};

			ClearGuid( objId );
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, objId );
			if ( GuidValid( objId ) )
			{
				retrieveResult = GetAnalysisInputSettings( aisRec, objId, NO_ID_NUM );
				if ( retrieveResult == DBApi::eQueryResult::QueryOk )
				{
					sr.ProcessingSettings = aisRec;
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::NoResults;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
				break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
			}
		}
		else if ( tag == SR_CumDetailedResultIdTag )				// "CumulativeDetailedResultID"
		{
			DB_DetailedResultRecord drRec = {};

			ClearGuid( objId );
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, objId );
			if ( GuidValid( objId ) )
			{
				retrieveResult = GetDetailedResult( drRec, objId, NO_ID_NUM );
				if ( retrieveResult == DBApi::eQueryResult::QueryOk )
				{
					sr.CumulativeDetailedResult = drRec;
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::NoResults;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
			else
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
				break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
			}
		}
		else if ( tag == SR_CumMaxFlChanPeakMapTag )
		{
			int32_t arrayCnt = 0;
			int32_t tokenCnt = 0;
			std::vector<std::string> subArrayStrList = {};
			std::vector<std::string> tokenList = {};
			std::string subArrayStr = "";
			std::string tokenStr = "";

			subArrayStrList.clear();
			tokenList.clear();
			arrayCnt = 0;
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			arrayCnt = ParseMultiArrayStrToStrList( subArrayStrList, val );

			for ( int32_t i = 0; i < arrayCnt; i++ )
			{
				subArrayStr.clear();
				subArrayStr = subArrayStrList.at( i );
				tokenCnt = ParseArrayStrToStrList( tokenList, subArrayStr );

				if ( tokenCnt != 2 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				int16_t chan = 0;
				int16_t peaks = 0;

				tokenStr = tokenList.at( 0 );
				chan = static_cast<int16_t>( std::stoi( tokenStr ) );

				tokenStr = tokenList.at( 1 );
				peaks = static_cast<int16_t>( std::stoi( tokenStr ) );

				sr.CumulativeMaxNumOfPeaksFlChanMap.emplace( chan, peaks );
			}
		}
		else if ( tag == SR_ImageResultIdListTag )				// "ImageResultIDList"
		{
			int32_t tokenCnt = 0;
			std::vector<std::string> tokenList = {};
			uuid__t imgResultUuid = {};

			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			resultIdList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( imgResultUuid );
				uuid__t_from_DB_UUID_Str( val, imgResultUuid );
				resultIdList.push_back( imgResultUuid );
			}
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			sr.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		size_t listSize = resultIdList.size();

		sr.ImageResultList.clear();
		if ( listSize > 0 )
		{
			retrieveResult = GetImageResultList( sr.ImageResultList, resultIdList );
		}
	}

	if ( retrieveResult != DBApi::eQueryResult::QueryOk && retrieveResult != DBApi::eQueryResult::NoResults )
	{
		parseOk = false;
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseImageSet( DBApi::eQueryResult& queryresult, DB_ImageSetRecord& isr,
							  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	uuid__t itemUuid = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> idList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	isr.ImageSequenceList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == IC_IdNumTag )					// "ImageSetIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			isr.ImageSetIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == IC_IdTag )					// "ImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, isr.ImageSetId );
		}
		else if ( tag == IC_SampleIdTag )			// "SampleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, isr.SampleId );
		}
		else if ( tag == IC_CreationDateTag )		// "CreationDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( isr.CreationDateTP, val );
		}
		else if ( tag == IC_SetFolderTag )			// "ImageSetFolder"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizePathString( val );
			isr.ImageSetPathStr = val;
		}
		else if ( tag == IC_SequenceCntTag )		// "ImageSequenceCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			isr.ImageSequenceCount = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IC_SequenceIdListTag )		// "ImageSequenceIDList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			idList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				idList.push_back( itemUuid );
			}
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			isr.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		if ( idList.size() >= 0 )
		{
			DB_ImageSeqRecord seqRec = {};
			size_t listSize = idList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( idList.at( i ) ) )
				{
					retrieveResult = GetImageSequence( seqRec, idList.at( i ), NO_ID_NUM );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						isr.ImageSequenceList.push_back( seqRec );
					}
					else
					{
						retrieveResult = DBApi::eQueryResult::NoResults;
						parseOk = false;
						break;      // break out of the 'for' loop
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseImageSequence( DBApi::eQueryResult& queryresult, DB_ImageSeqRecord& isr,
								   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	uuid__t itemUuid = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> idList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	isr.ImageList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == IS_IdNumTag )					// "ImageSequenceIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			isr.ImageSequenceIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == IS_IdTag )					// "ImageSequenceID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, isr.ImageSequenceId );
		}
		else if ( tag == IS_SetIdTag )				// "ImageSetID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, isr.ImageSetId );
		}
		else if ( tag == IS_SequenceNumTag )		// "ImageSequenceSeqNum"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			isr.SequenceNum = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IS_ImageCntTag )			// "ImageCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			isr.ImageCount = static_cast<int8_t>( fldRec.m_iVal );
		}
		else if ( tag == IS_FlChansTag )			// "FlChannels"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			isr.FlChannels = static_cast<int8_t>( fldRec.m_iVal );
		}
		else if ( tag == IS_ImageIdListTag )		// "ImageIDList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			idList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				idList.push_back( itemUuid );
			}
		}
		else if ( tag == IS_SequenceFolderTag )		// "ImageSequenceFolder"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizePathString( val );
			isr.ImageSequenceFolderStr = val;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			isr.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		if ( idList.size() >= 0 )
		{
			DB_ImageRecord imgRec = {};
			size_t listSize = idList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( idList.at( i ) ) )
				{
					retrieveResult = GetImage( imgRec, idList.at( i ), NO_ID_NUM );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						isr.ImageList.push_back( imgRec );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseImage( DBApi::eQueryResult& queryresult, DB_ImageRecord& ir,
						   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == IM_IdNumTag )					// "ImageIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ir.ImageIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == IM_IdTag )					// "ImageID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ir.ImageId );
		}
		else if ( tag == IM_SequenceIdTag )			// "ImageSequenceID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ir.ImageSequenceId );
		}
		else if ( tag == IM_ImageChanTag )			// "ImageChannel"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ir.ImageChannel = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == IM_FileNameTag )			// "ImageFileName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ir.ImageFileNameStr = val;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			ir.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseCellType( DBApi::eQueryResult& queryresult, DB_CellTypeRecord& ctrec,
							  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	uuid__t itemUuid = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> cellIdentIdList = {};
	std::vector<uuid__t> specializationsIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	ctrec.CellIdentParamList.clear();
	ctrec.SpecializationsDefList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == CT_IdNumTag )									// "CellTypeIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ctrec.CellTypeIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == CT_IdTag )									// "CellTypeID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ctrec.CellTypeId );
		}
		else if ( tag == CT_IdxTag )								// "CellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ctrec.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == CT_NameTag )								// "CellTypeName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ctrec.CellTypeNameStr = val;
		}
		else if (tag == CT_RetiredTag)								// "Retired"
		{
			bool boolFlag = DecodeBooleanRecordField(resultrec, tagStr);
			ctrec.Retired = boolFlag;
		}
		else if ( tag == CT_MaxImagesTag )							// "MaxImages"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.MaxImageCount = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_AspirationCyclesTag )					// "AspirationCycles"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.AspirationCycles = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_MinDiamMicronsTag )						// "MinDiamMicrons"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.MinDiam = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CT_MaxDiamMicronsTag )						// "MaxDiamMicrons"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.MaxDiam = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CT_MinCircularityTag )						// "MinCircularity"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.MinCircularity = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CT_SharpnessLimitTag )						// "SharpnessLimit"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.SharpnessLimit = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CT_NumCellIdentParamsTag )					// "NumCellIdentParams"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.NumCellIdentParams = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_CellIdentParamIdListTag )				// "CellIdentParamIDList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			cellIdentIdList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				cellIdentIdList.push_back( itemUuid );
			}
		}
		else if ( tag == CT_DeclusterSettingsTag )					// "DeclusterSetting"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.DeclusterSetting = static_cast<uint16_t>( fldRec.m_iVal );
		}
//		else if ( tag == CT_POIIdentParamTag )						// "POIIdentParam"
//		{
//			// TODO: determine what this will be and the proper container...
//		}
		else if ( tag == CT_RoiExtentTag )							// "RoiExtent"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.RoiExtent = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CT_RoiXPixelsTag )							// "RoiXPixels"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.RoiXPixels = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_RoiYPixelsTag )							// "RoiYPixels"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.RoiYPixels = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_NumAnalysisSpecializationsTag )			// "NumAnalysisSpecializations"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.NumAnalysisSpecializations = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CT_AnalysisSpecializationIdListTag )		// "AnalysisSpecializationsIDList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			specializationsIdList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				specializationsIdList.push_back( itemUuid );
			}
		}
		else if (tag == CT_CalcCorrectionFactorTag )				// "CalculationAdjustmentFactor"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ctrec.CalculationAdjustmentFactor = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == ProtectedTag )								// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ctrec.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		if ( cellIdentIdList.size() > 0 )
		{
			DB_AnalysisParamRecord ap = {};
			size_t listSize = cellIdentIdList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( specializationsIdList.at( i ) ) )
				{
					retrieveResult = GetAnalysisParam( ap, cellIdentIdList.at( i ) );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						ctrec.CellIdentParamList.push_back( ap );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
#endif // ALLOW_ID_FAILS
						break;      // break out of the 'for' loop
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk )
			{
				ctrec.NumCellIdentParams = static_cast<int8_t>( listSize );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	if ( parseOk )
	{
		if ( specializationsIdList.size() > 0 )
		{
			DB_AnalysisDefinitionRecord ad = {};
			size_t listSize = specializationsIdList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( specializationsIdList.at( i ) ) )
				{
					retrieveResult = GetAnalysisDefinition( ad, specializationsIdList.at( i ) );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						ctrec.SpecializationsDefList.push_back( ad );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk )
			{
				ctrec.NumAnalysisSpecializations = static_cast<int16_t>( listSize );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseImageAnalysisParam( DBApi::eQueryResult& queryresult, DB_ImageAnalysisParamRecord& iapr,
										 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == IAP_IdNumTag )													// "ImageAnalysisParamIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			iapr.ParamIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == IAP_IdTag )												// "ImageAnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, iapr.ParamId );
		}
		else if ( tag == IAP_AlgorithmModeTag )										// "AlgorithmMode"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.AlgorithmMode = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_BubbleModeTag )										// "BubbleMode"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			iapr.BubbleMode = boolFlag;
		}
		else if ( tag == IAP_DeclusterModeTag )										// "DeclusterMode"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			iapr.DeclusterMode = boolFlag;
		}
		else if ( tag == IAP_SubPeakAnalysisModeTag )								// "SubPeakAnalysisMode"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			iapr.SubPeakAnalysisMode = boolFlag;
		}
		else if ( tag == IAP_DilutionFactorTag )									// "DilutionFactor"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DilutionFactor = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_ROIXcoordsTag )										// "ROIXcoords"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ROI_Xcoords = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_ROIYcoordsTag )										// "ROIYcoords"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ROI_Ycoords = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterAccumuLowTag )								// "DeclusterAccumulatorThreshLow"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterAccumulatorThreshLow = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterMinDistanceLowTag )							// "DeclusterMinDistanceThreshLow"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterMinDistanceThreshLow = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterAccumuMedTag )								// "DeclusterAccumulatorThreshMed"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterAccumulatorThreshMed = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterMinDistanceMedTag )							// "DeclusterMinDistanceThreshMed"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterMinDistanceThreshMed = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterAccumuHighTag )								// "DeclusterAccumulatorThreshHigh"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterAccumulatorThreshHigh = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_DeclusterMinDistanceHighTag )							// "DeclusterMinDistanceThreshHigh"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.DeclusterMinDistanceThreshHigh = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_FovDepthTag )											// "FovDepthMM"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.FovDepthMM = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_PixelFovTag )											// "PixelFovMM"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.PixelFovMM = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_SizingSlopeTag )										// "SizingSlope"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.SizingSlope = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_SizingInterceptTag )									// "SizingIntercept"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.SizingIntercept = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_ConcSlopeTag )											// "ConcSlope"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ConcSlope = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_ConcInterceptTag )										// "ConcIntercept"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ConcIntercept = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_ConcImageControlCntTag )								// "ConcImageControlCnt"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ConcImageControlCount = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IAP_BubbleMinSpotAreaPrcntTag )							// "BubbleMinSpotAreaPrcnt"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.BubbleMinSpotAreaPercent = fldRec.m_fltVal;
		}
		else if ( tag == IAP_BubbleMinSpotAreaBrightnessTag )						// "BubbleMinSpotAreaBrightness"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.BubbleMinSpotAvgBrightness = fldRec.m_fltVal;
		}
		else if ( tag == IAP_BubbleRejectImgAreaPrcntTag )							// "BubbleRejectImgAreaPrcnt"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.BubbleRejectImageAreaPercent = fldRec.m_fltVal;
		}
		else if ( tag == IAP_VisibleCellSpotAreaTag )								// "VisibleCellSpotArea"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.VisibleCellSpotArea = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_FlScalableROITag )										// "FlScalableROI"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.FlScalableROI = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_FLPeakPercentTag )										// "FLPeakPercent"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.FlPeakPercent = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_NominalBkgdLevelTag )									// "NominalBkgdLevel"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.NominalBkgdLevel = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_BkgdIntensityToleranceTag )							// "BkgdIntensityTolerance"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.BkgdIntensityTolerance = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_CenterSpotMinIntensityTag )							// "CenterSpotMinIntensityLimit"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.CenterSpotMinIntensityLimit = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_PeakIntensitySelectionAreaTag )						// "PeakIntensitySelectionAreaLimit"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.PeakIntensitySelectionAreaLimit = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_CellSpotBrightnessExclusionTag )						// "CellSpotBrightnessExclusionThreshold"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.CellSpotBrightnessExclusionThreshold = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_HotPixelEliminationModeTag )							// "HotPixelEliminationMode"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.HotPixelEliminationMode = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_ImgBotAndRightBoundaryModeTag )						// "ImgBotAndRightBoundaryAnnotationMode"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.ImgBottomAndRightBoundaryAnnotationMode = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == IAP_SmallParticleSizeCorrectionTag )						// "SmallParticleSizingCorrection"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			iapr.SmallParticleSizingCorrection = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == ProtectedTag )				// "Protected"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			iapr.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return parseOk;
}

bool DBifImpl::ParseAnalysisInputSettings( DBApi::eQueryResult& queryresult, DB_AnalysisInputSettingsRecord& settings,
										   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t arrayCnt = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> subArrayStrList = {};
	std::string subArrayStr = "";
	std::vector<std::string> tokenList = {};
	CDBVariant fldRec = {};
	std::vector<uuid__t> cellIdentIdList = {};
	std::vector<uuid__t> specializationsIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	settings.POIIdentParamList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize && parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == AIP_IdNumTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			settings.SettingsIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == AIP_IdTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, settings.SettingsId );
		}
		else if ( tag == AIP_ConfigParamMapTag )
		{
			subArrayStrList.clear();
			tokenList.clear();
			arrayCnt = 0;
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			arrayCnt = ParseMultiArrayStrToStrList( subArrayStrList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( arrayCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			for ( int32_t i = 0; i < arrayCnt; i++ )
			{
				subArrayStr.clear();
				subArrayStr = subArrayStrList.at( i );
				tokenCnt = ParseArrayStrToStrList( tokenList, subArrayStr );
				if ( tokenCnt != 2 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				ConfigParameters::E_CONFIG_PARAMETERS enumVal = ConfigParameters::E_CONFIG_PARAMETERS::eInvalidKey;
				double mapVal = 0.0;

				val = tokenList.at( 0 );
				enumVal = static_cast<ConfigParameters::E_CONFIG_PARAMETERS>( std::stol( val ) );

				val = tokenList.at( 1 );
				mapVal = std::stod( val );
				settings.InputConfigParamMap.emplace( enumVal, mapVal );
			}
		}
		else if ( tag == AIP_CellIdentParamListTag )
		{
			subArrayStrList.clear();
			tokenList.clear();
			arrayCnt = 0;
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			arrayCnt = ParseMultiArrayStrToStrList( subArrayStrList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( arrayCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			settings.CellIdentParamList.clear();
			for ( int32_t arrayIndex = 0; arrayIndex < arrayCnt; arrayIndex++ )
			{
#if(0)
				analysis_input_params_t cellIdentParam;
				Characteristic_t characteristics;
#else
				analysis_input_params_storage cellIdentParam = {};
#endif
				float value = 0.0;
				E_POLARITY polarity = E_POLARITY::eInvalidPolarity;
				uint16_t key = 0;
				uint16_t skey = 0;
				uint16_t sskey = 0;

				subArrayStr.clear();
				subArrayStr = subArrayStrList.at( arrayIndex );
				tokenCnt = ParseArrayStrToStrList( tokenList, subArrayStr );

				if ( tokenCnt != 5 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t tokenIndex = 0; tokenIndex < tokenCnt; tokenIndex++ )
				{
					val = tokenList.at( tokenIndex );

					switch ( tokenIndex )
					{
						case 0:
							key = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 1:
							skey = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 2:
							sskey = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 3:
							value = static_cast<float>( std::stof( val ) );
							break;

						case 4:
							polarity = static_cast<E_POLARITY>( std::stol( val ) );
							break;

					}
				}

#if(0)
				characteristics._Myfirst._Val = key;
				characteristics._Get_rest()._Myfirst._Val = skey;
				characteristics._Get_rest()._Get_rest()._Myfirst._Val = sskey;

				cellIdentParam._Myfirst._Val = characteristics;
				cellIdentParam._Get_rest()._Myfirst._Val = value;
				cellIdentParam._Get_rest()._Get_rest()._Myfirst._Val = polarity;
#else
				cellIdentParam.key = key;
				cellIdentParam.s_key = skey;
				cellIdentParam.s_s_key = sskey;
				cellIdentParam.value = value;
				cellIdentParam.polarity = polarity;
#endif
				settings.CellIdentParamList.push_back( cellIdentParam );
			}
		}
		else if ( tag == AIP_PoiIdentParamListTag )
		{
			subArrayStrList.clear();
			tokenList.clear();
			arrayCnt = 0;
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			arrayCnt = ParseMultiArrayStrToStrList( subArrayStrList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( arrayCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			for ( int32_t i = 0; i < arrayCnt; i++ )
			{
#if(0)
				analysis_input_params_t poiIdentParam;
				Characteristic_t characteristics;
#else
				analysis_input_params_storage poiIdentParam = {};
#endif
				float value = 0.0;
				E_POLARITY polarity = E_POLARITY::eInvalidPolarity;
				uint16_t key = 0;
				uint16_t skey = 0;
				uint16_t sskey = 0;

				subArrayStr.clear();
				subArrayStr = subArrayStrList.at( i );
				tokenCnt = ParseArrayStrToStrList( tokenList, subArrayStr );

				if ( tokenCnt != 5 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t i = 0; i < tokenCnt; i++ )
				{
					val = tokenList.at( i );

					switch ( i )
					{
						case 0:
							key = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 1:
							skey = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 2:
							sskey = static_cast<uint16_t>( std::stoi( val ) );
							break;

						case 3:
							value = static_cast<float>( std::stof( val ) );
							break;

						case 4:
							polarity = static_cast<E_POLARITY>( std::stol( val ) );
							break;

					}
				}

#if(0)
				characteristics._Myfirst._Val = key;
				characteristics._Get_rest()._Myfirst._Val = skey;
				characteristics._Get_rest()._Get_rest()._Myfirst._Val = sskey;

				poiIdentParam._Myfirst._Val = characteristics;
				poiIdentParam._Get_rest()._Myfirst._Val = value;
				poiIdentParam._Get_rest()._Get_rest()._Myfirst._Val = polarity;
#else
				poiIdentParam.key = key;
				poiIdentParam.s_key = skey;
				poiIdentParam.s_s_key = sskey;
				poiIdentParam.value = value;
				poiIdentParam.polarity = polarity;
#endif
				settings.POIIdentParamList.push_back( poiIdentParam );
			}
		}
		else if ( tag == ProtectedTag )								// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			settings.Protected = boolFlag;
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

#if(0)
bool DBifImpl::ParseImageAnalysisBlobIdentParam( DBApi::eQueryResult& queryresult, DB_BlobIdentParamRecord& settings,
												  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;

	queryresult = DBApi::eQueryResult::QueryOk;

	return parseOk;
}
#endif

bool DBifImpl::ParseAnalysisDefinition( DBApi::eQueryResult& queryresult, DB_AnalysisDefinitionRecord& adrec,
										CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	int32_t indexVal = 0;
	std::vector<std::string> tokenList = {};
	CDBVariant fldRec = {};
	std::vector<int32_t> illuminatorsIndexList = {};
	std::vector<uuid__t> analysisParamIdList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;
	uuid__t ppUuid = {};
	bool parseOk = true;

	ClearGuid( ppUuid );

	adrec.ReagentIndexList.clear();
	adrec.IlluminatorsList.clear();
	adrec.AnalysisParamList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == AD_IdNumTag )							// "AnalysisDefinitionIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			adrec.AnalysisDefIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == AD_IdTag )							// "AnalysisDefinitionID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, adrec.AnalysisDefId );
		}
		else if ( tag == AD_IdxTag )						// "AnalysisDefinitionIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			adrec.AnalysisDefIndex = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AD_NameTag )						// "AnalysisDefinitionName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			adrec.AnalysisLabel = val;
		}
		else if ( tag == AD_NumReagentsTag )				// "NumReagents"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			adrec.NumReagents = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == AD_ReagentTypeIdxListTag )			// "ReagentTypeIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::atol( val.c_str() );
				adrec.ReagentIndexList.push_back( indexVal );
			}
		}
		else if ( tag == AD_MixingCyclesTag )				// "MixingCycles"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			adrec.MixingCycles = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AD_NumIlluminatorsTag )			// "NumIlluminators"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			adrec.NumFlIlluminators = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == AD_IlluminatorIdxListTag )			// "IlluminatorsIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			illuminatorsIndexList.clear();
			indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::atol( val.c_str() );
				illuminatorsIndexList.push_back( indexVal );
			}
		}
		else if ( tag == AD_NumAnalysisParamsTag )			// "NumAnalysisParams"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			adrec.NumAnalysisParams = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == AD_AnalysisParamIdListTag )		// "AnalysisParamIDList"
		{
			uuid__t itemUuid = {};

			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			analysisParamIdList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				ClearGuid( itemUuid );
				uuid__t_from_DB_UUID_Str( val, itemUuid );
				analysisParamIdList.push_back( itemUuid );
			}
		}
		else if ( tag == AD_PopulationParamIdTag )			// "PopulationParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ClearGuid( ppUuid );
			uuid__t_from_DB_UUID_Str( val, ppUuid );
		}
		else if ( tag == AD_PopulationParamExistsTag )
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			adrec.PopulationParamExists = boolFlag;
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			adrec.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		if ( illuminatorsIndexList.size() > 0 )
		{
			DB_IlluminatorRecord ilRec = {};
			size_t listSize = illuminatorsIndexList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				indexVal = illuminatorsIndexList.at( i );

				if ( indexVal >= 0 )
				{
					retrieveResult = GetIlluminatorInternal( ilRec, "", indexVal, NO_ID_NUM );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						adrec.IlluminatorsList.push_back( ilRec );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk  && adrec.IlluminatorsList.size() != adrec.NumFlIlluminators )
			{
				adrec.NumFlIlluminators = static_cast<uint8_t>( adrec.IlluminatorsList.size() );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	if ( parseOk )
	{
		if ( analysisParamIdList.size() > 0 )
		{
			DB_AnalysisParamRecord ap = {};
			size_t listSize = analysisParamIdList.size();

			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( GuidValid( analysisParamIdList.at( i ) ) )
				{
					retrieveResult = GetAnalysisParamInternal( ap, analysisParamIdList.at( i ) );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						adrec.AnalysisParamList.push_back( ap );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk && adrec.AnalysisParamList.size() != adrec.NumAnalysisParams )
			{
				adrec.NumAnalysisParams = static_cast<uint8_t>( adrec.AnalysisParamList.size() );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	if ( parseOk )
	{
		if ( adrec.PopulationParamExists && GuidValid( ppUuid ) )
		{
			DB_AnalysisParamRecord apRec = {};

			apRec.ParamId = ppUuid;
			retrieveResult = GetAnalysisParamInternal( apRec, ppUuid );
			adrec.PopulationParam = apRec;
			if ( retrieveResult != DBApi::eQueryResult::QueryOk )
			{
#ifdef ALLOW_ID_FAILS
				retrieveResult = DBApi::eQueryResult::QueryOk;
#else
				parseOk = false;
				retrieveResult = DBApi::eQueryResult::NoResults;
				break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
			break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseAnalysisParam( DBApi::eQueryResult& queryresult, DB_AnalysisParamRecord& aprec,
									CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == AP_IdNumTag )					// "AnalysisParamIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			aprec.ParamIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == AP_IdTag )					// "AnalysisParamID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, aprec.ParamId );
		}
		else if ( tag == AP_InitializedTag )		// "IsInitialized"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			aprec.IsInitialized = boolFlag;
		}
		else if ( tag == AP_LabelTag )				// "AnalysisParamLabel"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			aprec.ParamLabel = val;
		}
		else if ( tag == AP_KeyTag )				// "CharacteristicKey"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			aprec.Characteristics.key = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AP_SKeyTag )				// "CharacteristicSKey"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			aprec.Characteristics.s_key = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AP_SSKeyTag )				// "CharacteristicSSKey"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			aprec.Characteristics.s_s_key = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == AP_ThreshValueTag )		// "ThresholdValue"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			aprec.ThresholdValue = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == AP_AboveThreshTag )		// "AboveThreshold"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			aprec.AboveThreshold = boolFlag;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			aprec.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseIlluminator( DBApi::eQueryResult& queryresult, DB_IlluminatorRecord& ilrec,
								 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == IL_IdNumTag )					// "IlluminatorIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ilrec.IlluminatorIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == IL_IdxTag )				// "IlluminatorIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.IlluminatorIndex = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_TypeTag )				// "IlluminatorType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.IlluminatorType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_NameTag )				// "IlluminatorName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ilrec.IlluminatorNameStr = val;
		}
		else if ( tag == IL_PosNumTag )				// "PositionNum"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.PositionNum = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_ToleranceTag )			// "Tolerance"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.Tolerance = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == IL_MaxVoltageTag )			// "MaxVoltage"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.MaxVoltage = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IL_IllumWavelengthTag )	// "IlluminatorWavelength"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.IlluminatorWavelength = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_EmitWavelengthTag )		// "EmissionWavelength"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.EmissionWavelength = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_ExposureTimeMsTag )		// "ExposureTimeMs"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.ExposureTimeMs = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_PercentPowerTag )		// "PercentPower"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.PercentPower = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_SimmerVoltageTag )		// "SimmerVoltage"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.SimmerVoltage = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == IL_LtcdTag )				// "Ltcd"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.Ltcd = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_CtldTag )				// "Ctld"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.Ctld = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == IL_FeedbackDiodeTag )		// "FeedbackPhotoDiode"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ilrec.FeedbackDiode= fldRec.m_iVal;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ilrec.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseUser( DBApi::eQueryResult& queryresult, DB_UserRecord& ur,
						  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	int32_t tokenCnt = 0;
	uint32_t indexVal = 0;
	std::vector<std::string> tokenList = {};
	std::vector<int32_t> idxList = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	ur.AuthenticatorList.clear();
	ur.ColumnDisplayList.clear();
	ur.UserCellTypeIndexList.clear();
	ur.UserPropertiesList.clear();

	for ( tagIndex = 0; ( tagIndex < tagListSize ) && ( parseOk ); tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == UR_IdNumTag )							// "UserIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ur.UserIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == UR_IdTag )							// "UserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ur.UserId );
		}
		else if ( tag == UR_RetiredTag )					// "Retired"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ur.Retired = boolFlag;
		}
		else if ( tag == UR_ADUserTag )						// "ADUser"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ur.ADUser = boolFlag;
		}
		else if ( tag == UR_RoleIdTag )						// "RoleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, ur.RoleId );
		}
		else if ( tag == UR_UserNameTag )					// "UserName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.UserNameStr = val;
		}
		else if ( tag == UR_DisplayNameTag )				// "DisplayName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.DisplayNameStr = val;
		}
		else if ( tag == UR_CommentsTag )					// "Comments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.Comment = val;
		}
		else if ( tag == UR_UserEmailTag )					// "UserEmail"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.UserEmailStr = val;
		}
		else if ( tag == UR_AuthenticatorListTag )			// "AuthenticatorList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList ( tokenList, ur.AuthenticatorList );
		}
		else if ( tag == UR_AuthenticatorDateTag )			// "AuthenticatorDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				system_TP zeroTP = {};

				GetTimePtFromDbTimeString( zeroTP, val );
			}
			else
			{
				GetTimePtFromDbTimeString( ur.AuthenticatorDateTP, val );
			}
		}
		else if ( tag == UR_LastLoginTag )					// "LastLogin"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				system_TP zeroTP = {};

				GetTimePtFromDbTimeString( zeroTP, val );
			}
			else
			{
				GetTimePtFromDbTimeString( ur.LastLogin, val );
			}
		}
		else if ( tag == UR_AttemptCntTag )					// "AttemptCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.AttemptCount = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_LanguageCodeTag )				// "LanguageCode"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.LanguageCode = val;
		}
		else if ( tag == UR_DfltSampleNameTag )				// "DefauultSampleName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.DefaultSampleNameStr = val;
		}
		else if ( tag == UR_UserImageSaveNTag )				// "SaveNthImage"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.UserImageSaveN = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_DisplayColumnsTag )				// "DisplayColumns"
		{
			std::vector<std::string> subArrayStrList = {};
			std::string subArrayStr = "";
			std::vector<std::string> tokenList = {};
			int32_t arrayCnt = 0;
			int32_t tokenCnt = 0;

			subArrayStrList.clear();
			tokenList.clear();
			arrayCnt = 0;
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			arrayCnt = ParseMultiArrayStrToStrList( subArrayStrList, val );

			for ( int32_t arrayIndex = 0; arrayIndex < arrayCnt; arrayIndex++ )
			{
				display_column_info_t colInfo = {};
				int32_t colType = eDisplayColumns::NoColType;
				int16_t colIndex = -1;
				int16_t colWidth = 0;
				bool visible = true;

				subArrayStr.clear();
				subArrayStr = subArrayStrList.at( arrayIndex );
				tokenCnt = ParseArrayStrToStrList( tokenList, subArrayStr );

				if ( tokenCnt != 4 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				for ( int32_t tokenIndex = 0; tokenIndex < tokenCnt; tokenIndex++ )
				{
					val = tokenList.at( tokenIndex );

					switch ( tokenIndex )
					{
						case 0:
							colType = std::stoi( val );
							break;

						case 1:
							colIndex = static_cast<int16_t>( std::stoi( val ) );
							break;

						case 2:
							colWidth = static_cast<int16_t>( std::stoi( val ) );
							break;

						case 3:
							StrToLower( val );
							if ( val == TrueStr || val == "t" || val == "1" )
							{
								visible = true;
							}
							else if ( val == FalseStr || val == "f" || val == "0" )
							{
								visible = false;
							}
							break;
					}
				}

				colInfo.ColumnType = colType;
				colInfo.OrderIndex = colIndex;
				colInfo.Width = colWidth;
				colInfo.Visible = visible;
				ur.ColumnDisplayList.push_back( colInfo );
			}
		}
		else if ( tag == UR_DecimalPrecisionTag )			// "DecimalPrecision"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.DecimalPrecision = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_ExportFolderTag )				// "ExportFolder"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizePathString( val );
			ur.ExportFolderStr = val;
		}
		else if ( tag == UR_DfltResultFileNameStrTag )		// "DefauultResultFileName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.DefaultResultFileNameStr = val;
		}
		else if ( tag == UR_CSVFolderTag )					// "CSVFolder"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizePathString( val );
			ur.CSVFolderStr = val;
		}
		else if ( tag == UR_PdfExportTag )					// "PdfExport"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ur.PdfExport = boolFlag;
		}
		else if ( tag == UR_AllowFastModeTag )				// "AllowFastMode"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ur.AllowFastMode = boolFlag;
		}
		else if ( tag == UR_WashTypeTag )					// "WashType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.WashType = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_DilutionTag )					// "Dilution"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.Dilution = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_DefaultCellTypeIdxTag )			// "DefaultCellTypeIndex";
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ur.DefaultCellType = std::stoul( val.c_str() );
		}
		else if ( tag == UR_NumCellTypesTag )				// "NumUserCellTypes"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.NumUserCellTypes = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_CellTypeIdxListTag )			// "UserCellTypeIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::stoul( val.c_str() );
				ur.UserCellTypeIndexList.push_back( indexVal );
			}
		}
		else if ( tag == UR_AnalysisDefIdxListTag )			// "AnalysisDefIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::stol( val.c_str() );
				ur.UserAnalysisIndexList.push_back( indexVal );
			}
		}
		else if ( tag == UR_NumUserPropsTag )				// "NumUserProperties"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			ur.NumUserProperties = static_cast<uint8_t>( fldRec.m_iVal );
		}
		else if ( tag == UR_UserPropsIdxListTag )			// "UserPropertiesIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			idxList.clear();
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				indexVal = 0;
				val = tokenList.at( i );
				if ( val.length() > 0 )
				{
					indexVal = std::stoul( val.c_str() );
				}
				idxList.push_back( indexVal );
			}
		}
		else if ( tag == UR_AppPermissionsTag )				// "AppPermissions"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ur.AppPermissions = std::stoull( val.c_str() );
		}
		else if ( tag == UR_AppPermissionsHashTag )			// "AppPermissionsHash"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.AppPermissionsHash = val;
		}
		else if ( tag == UR_InstPermissionsTag )			// "InstrumentPermissions"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			ur.InstPermissions = std::stoull( val.c_str() );
		}
		else if ( tag == UR_InstPermissionsHashTag )		// "InstrumentPermissionsHash"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			ur.InstPermissionsHash = val;
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			ur.Protected = boolFlag;
		}
	}

	if ( parseOk )
	{
		size_t listSize = idxList.size();

		// allow worklists to have no sample sets...
		if ( listSize > 0 )
		{
			DB_UserPropertiesRecord up = {};
			uuid__t tmpId;

			ClearGuid( tmpId );
			for ( int32_t i = 0; i < listSize; i++ )
			{
				if ( idxList.at( i ) >= 0 )
				{
					retrieveResult = GetUserPropertyInternal( up, idxList.at( i ), "", NO_ID_NUM );
					if ( retrieveResult == DBApi::eQueryResult::QueryOk )
					{
						ur.UserPropertiesList.push_back( up );
					}
					else
					{
#ifdef ALLOW_ID_FAILS
						retrieveResult = DBApi::eQueryResult::QueryOk;
#else
						parseOk = false;
						retrieveResult = DBApi::eQueryResult::NoResults;
						break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
					}
				}
				else
				{
#ifdef ALLOW_ID_FAILS
					retrieveResult = DBApi::eQueryResult::QueryOk;
#else
					parseOk = false;
					retrieveResult = DBApi::eQueryResult::BadOrMissingListIds;
					break;      // break out of the 'for' loop
#endif // ALLOW_ID_FAILS
				}
			}

			if ( parseOk )
			{
				ur.NumUserProperties = static_cast<int8_t>( listSize );
			}
		}
		else
		{
#ifdef ALLOW_ID_FAILS
			retrieveResult = DBApi::eQueryResult::QueryOk;
#else
			parseOk = false;
			retrieveResult = DBApi::eQueryResult::QueryFailed;
#endif // ALLOW_ID_FAILS
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseRole( DBApi::eQueryResult& queryresult, DB_UserRoleRecord& rr,
						  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	std::vector<std::string> tokenList = {};
	int32_t tokenCnt = 0;
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	rr.GroupMapList.clear();

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == RO_IdNumTag )					// "RoleIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rr.RoleIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == RO_IdTag )					// "RoleID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, rr.RoleId );
		}
		else if ( tag == RO_NameTag )				// "RoleName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			rr.RoleNameStr = val;
		}
		else if ( tag == RO_RoleTypeTag )			// "RoleType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			rr.RoleType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == RO_GroupMapListTag )		// "GroupMapList"
		{
			tokenList.clear();

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			// the group map is a list of strings; use the array parser designed for strings (does not remove embedded spaces)
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, rr.GroupMapList );
		}
		else if ( tag == RO_CellTypeIdxListTag )	// "UserCellTypeIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			uint32_t indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::stoul( val.c_str() );
				rr.CellTypeIndexList.push_back( indexVal );
			}
		}
		else if ( tag == RO_InstPermissionsTag )	// "InstPermissions"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rr.InstrumentPermissions = std::stoull( val.c_str() );
		}
		else if ( tag == RO_AppPermissionsTag )		// "AppPermisions"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rr.ApplicationPermissions = std::stoull( val.c_str() );
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			rr.Protected = boolFlag;
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseUserProperty( DBApi::eQueryResult& queryresult, DB_UserPropertiesRecord& up,
								  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == UP_IdNumTag )				// "PropertyIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			up.PropertiesIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == UP_IdxTag )			// "PropertyIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			up.PropertyIndex = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == UP_NameTag )			// "PropertyName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			up.PropertyNameStr = val;
		}
		else if ( tag == UP_TypeTag )			// "PropertyType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			up.PropertyType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == ProtectedTag )			// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			up.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseSignature( DBApi::eQueryResult& queryresult, DB_SignatureRecord& sigr,
							   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SG_IdNumTag )					// "SignatureDefIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			sigr.SignatureDefIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == SG_IdTag )					// "SignatureDefID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, sigr.SignatureDefId );
		}
		else if ( tag == SG_ShortSigTag )			// "ShortSignature"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			sigr.ShortSignatureStr = val;
		}
		else if ( tag == SG_ShortSigHashTag )		// "ShortSignatureHash"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			sigr.ShortSignatureHash = val;
		}
		else if ( tag == SG_LongSigTag )			// "LongSignature"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			sigr.LongSignatureStr = val;
		}
		else if ( tag == SG_LongSigHashTag )		// "LongSignatureHash"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			sigr.LongSignatureHash = val;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			sigr.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseReagentType( DBApi::eQueryResult& queryresult, DB_ReagentTypeRecord& rxr,
								 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	std::vector<int32_t> reagentsIndexList = {};
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == RX_IdNumTag )						// "ReagentIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rxr.ReagentIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == RX_TypeNumTag )				// "ReagentTypeNum"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			rxr.ReagentTypeNum = fldRec.m_lVal;
		}
		else if ( tag == RX_CurrentSnTag )				// "Current"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			rxr.Current = boolFlag;
		}
		else if ( tag == RX_ContainerRfidSNTag )		// "ContainerTagSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			rxr.ContainerTagSn = val;
		}
		else if ( tag == RX_IdxListTag )				// "ReagentIndexList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			rxr.ReagentIndexList.clear();
			int16_t indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::atoi( val.c_str() );
				rxr.ReagentIndexList.push_back( indexVal );
			}
		}
		else if ( tag == RX_NamesListTag )				// "ReagentNamesList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseStrArrayStrToStrList( tokenList, val );

#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, rxr.ReagentNamesList );
		}
		else if ( tag == RX_MixingCyclesTag )			// "MixingCycles"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			rxr.MixingCyclesList.clear();
			int16_t indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::atoi( val.c_str() );
				rxr.MixingCyclesList.push_back( indexVal );
			}
		}
		else if ( tag == RX_PackPartNumTag )			// "PackPartNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			rxr.PackPartNumStr = val;
		}
		else if ( tag == RX_LotNumTag )					// "LotNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			rxr.LotNumStr = val;
		}
		else if ( tag == RX_LotExpirationDateTag )		// "LotExpiration"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rxr.LotExpirationDate = std::atoll( val.c_str() );
		}
		else if ( tag == RX_InServiceDateTag )			// "InService"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			rxr.InServiceDate = std::atoll( val.c_str() );
		}
		else if ( tag == RX_InServiceDaysTag )			// "ServiceLife"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			rxr.InServiceExpirationLength = fldRec.m_iVal;
		}
		else if ( tag == ProtectedTag )					// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			rxr.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseCellHealthReagent(
	DBApi::eQueryResult& queryresult, DB_CellHealthReagentRecord& chr, CRecordset& resultrec, std::vector<std::string>& taglist)
{
	size_t tagListSize = taglist.size();

	if (tagListSize <= 0)
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T("");      // required for retrieval from recordset
	CString valStr = _T("");      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	std::vector<std::string> tokenList = {};
	std::vector<int32_t> reagentsIndexList = {};
	CDBVariant fldRec = {};

	for (tagIndex = 0; tagIndex < tagListSize; tagIndex++)
	{
		tag = taglist.at(tagIndex);
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if (tag == CH_IdNumTag)
		{
			resultrec.GetFieldValue(tagStr, valStr);
			val = CT2A(valStr);
			chr.IdNum = std::atoll(val.c_str());
		}
		else if (tag == CH_IdTag)						// "IdNum"
		{
			resultrec.GetFieldValue(tagStr, valStr);
			val = CT2A(valStr);
			uuid__t_from_DB_UUID_Str(val, chr.Id);
		}
		else if (tag == CH_TypeTag)						// "Type"
		{
			//chr.type.port_number = static_cast<int16_t>(std::stoi(tokenStr));
			resultrec.GetFieldValue(tagStr, fldRec);
			chr.Type = fldRec.m_iVal;
		}
		else if (tag == CH_NameTag)						// "Name"
		{
			resultrec.GetFieldValue(tagStr, valStr);
			val = CT2A(valStr);
			DeSanitizeDataString(val);
			chr.Name = val;
		}
		else if (tag == CH_VolumeTag)					// "Volume"
		{
			resultrec.GetFieldValue(tagStr, fldRec);
			chr.Volume = fldRec.m_lVal;
		}
		else if (tag == ProtectedTag)					// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField(resultrec, tagStr);
			chr.Protected = boolFlag;
		}
	}

	queryresult = DBApi::eQueryResult::QueryOk;

	return true;
}

bool DBifImpl::ParseBioProcess( DBApi::eQueryResult& queryresult, DB_BioProcessRecord& bpr,
								CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;

	queryresult = DBApi::eQueryResult::QueryOk;

	return parseOk;
}

bool DBifImpl::ParseQcProcess( DBApi::eQueryResult& queryresult, DB_QcProcessRecord& qcr,
							   CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == QC_IdNumTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			qcr.QcIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == QC_IdTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, qcr.QcId );
		}
		else if ( tag == QC_NameTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			qcr.QcName = val;
		}
		else if ( tag == QC_TypeTag )
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			qcr.QcType = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == QC_CellTypeIdTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, qcr.CellTypeId );
		}
		else if ( tag == QC_CellTypeIndexTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			qcr.CellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == QC_LotInfoTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			qcr.LotInfo = val;
		}
		else if ( tag == QC_LotExpirationTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			qcr.LotExpiration = val;
		}
		else if ( tag == QC_AssayValueTag )
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			qcr.AssayValue = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == QC_AllowablePercentTag )
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			qcr.AllowablePercentage = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == QC_SequenceTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			qcr.QcSequence = val;
		}
		else if ( tag == QC_CommentsTag )
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			qcr.Comments = val;
		}
		else if (tag == QC_RetiredTag)								// "Retired"
		{
			bool boolFlag = DecodeBooleanRecordField(resultrec, tagStr);
			qcr.Retired = boolFlag;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			qcr.Protected = boolFlag;
		}
	}

	bool parseOk = true;

	queryresult = DBApi::eQueryResult::QueryOk;

	return parseOk;
}

bool DBifImpl::ParseCalibration( DBApi::eQueryResult& queryresult, DB_CalibrationRecord& car,
								 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == CC_IdNumTag  )					// "CalibrationIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			car.CalIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == CC_IdTag )					// "CalibrationID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, car.CalId );
		}
		else if ( tag == CC_InstSNTag )				// "InstrumentSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			car.InstrumentSNStr = val;
		}
		else if ( tag == CC_CalDateTag )			// "CalibrationDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( car.CalDate, val );
		}
		else if ( tag == CC_CalUserIdTag )			// "CalibrationUserID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, car.CalUserId );
		}
		else if ( tag == CC_CalTypeTag )			// "CalibrationType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			car.CalType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CC_SlopeTag )				// "Slope"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			car.Slope = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == CC_InterceptTag )			// "Intercept"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			car.Intercept = static_cast<double>( fldRec.m_dblVal );
		}
		else if ( tag == CC_ImageCntTag )			// "ImageCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			car.ImageCnt = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CC_QueueIdTag )			// "CalQueueID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, car.QueueId );
		}
		else if ( tag == CC_ConsumablesListTag )	// "ConsumablesList"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces and element container quotes; DO NOT REMOVE PARENTHESES AT THIS TIME!
			TrimStr( "{}\"", val );

			int32_t arrayCnt = 0;
			std::vector<std::string> consumablesArrayStrList = {};

			consumablesArrayStrList.clear();

			// first separate the array of blob data into the individual blob info
			arrayCnt = ParseCompositeArrayStrToStrList( consumablesArrayStrList, val );
			if ( arrayCnt != consumablesArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string consumablesArrayStr = "";
			std::vector<std::string> elementStrList = {};
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				cal_consumable_t consumable = {};
				int32_t eCnt = 0;

				consumablesArrayStr.clear();
				consumablesArrayStr = consumablesArrayStrList.at( aidx );

				// next remove any leading and trailing array parentheses and quotes
				TrimStr( "{}()\"", consumablesArrayStr );

				// Now break out the composite elements of this blob
				elementStrList.clear();
				eCnt = ParseConsumablesCompositeElementStrToStrList( elementStrList, consumablesArrayStr );
				if ( eCnt != elementStrList.size() || eCnt != 5 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				// now parse the extracted blob data elements
				for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
				{
					tokenStr.clear();
					tokenStr = elementStrList.at( eidx );

					switch ( eidx )
					{
						case 0:			// consumable element enum 0 = eConsumableLabel
						{
							DeSanitizeDataString( tokenStr );
							consumable.label = tokenStr;
							break;
						}

						case 1:			// consumable element enum 1 = eConsumableLotId
						{
							DeSanitizeDataString( tokenStr );
							consumable.lot_id = tokenStr;
							break;
						}

						case 2:			// consumable element enum 2 = eConsumableCalibratorType
						{
							consumable.cal_type = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 3:			// consumable element enum 3 = eConsumableExpiration
						{
							GetTimePtFromDbTimeString( consumable.expiration_date, tokenStr );
							break;
						}

						case 4:			// consumable element enum 2 = eConsumableAssayValue
						{
							consumable.assay_value = static_cast<float>( std::stof( tokenStr ) );
							break;
						}
					}
				}
				car.ConsumablesList.push_back( consumable );
			}
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			sigr.Protected = boolFlag;
		}
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseInstConfig( DBApi::eQueryResult& queryresult, DB_InstrumentConfigRecord& icr,
								CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == CFG_IdNumTag )							// "InstrumentIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			icr.InstrumentIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == CFG_InstSNTag )					// "InstrumentSN"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.InstrumentSNStr = val;
		}
		else if ( tag == CFG_InstTypeTag )					// "InstrumentType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.InstrumentType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_DeviceNameTag )				// "DeviceName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.DeviceName = val;
		}
		else if ( tag == CFG_UiVerTag )						// "UIVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.UIVersion = val;
		}
		else if ( tag == CFG_SwVerTag )						// "SoftwareVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.SoftwareVersion = val;
		}
		else if ( tag == CFG_AnalysisSwVerTag )				// "AnalysisSWVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.AnalysisVersion = val;
		}
		else if ( tag == CFG_FwVerTag )						// "FirmwareVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.FirmwareVersion = val;
		}
		else if ( tag == CFG_CameraTypeTag )				// "CameraType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.CameraType = static_cast<int16_t>( fldRec.m_iVal );
		}

		else if (tag == CFG_BrightFieldLedTypeTag)				// "BrightFieldLedType"
		{
			resultrec.GetFieldValue(tagStr, fldRec);
			icr.BrightFieldLedType = static_cast<int16_t>(fldRec.m_iVal);
		}
		else if ( tag == CFG_CameraFwVerTag )				// "CameraFWVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.CameraFWVersion = val;
		}
		else if ( tag == CFG_CameraCfgTag )					// "CameraConfig"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.CameraConfig = val;
		}
		else if ( tag == CFG_PumpTypeTag )					// "PumpType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.PumpType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_PumpFwVerTag )					// "PumpFWVersion"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.PumpFWVersion = val;
		}
		else if ( tag == CFG_PumpCfgTag )					// "PumpConfig"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.PumpConfig = val;
		}
		else if ( tag == CFG_IlluminatorsInfoListTag )		// "IlluminatorsInfoList"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// the original composite array string as provided by the DB should appear like the following line
			//  "{\"(t1,i1)\",\"(t2,i2)\",\"(t3,i3)\",...\"(tn,in)\"}"

			// first, remove all the escaped quote marks and the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// the 'cleaned' array string should appear like the following line...
			//  "{(t1,i1),(t2,i2),(t3,i3),...(tn,in)}"
			// next remove any leading and trailing array curly braces and element container quotes, if any; DO NOT REMOVE PARENTHESES AT THIS TIME!
			TrimStr( "{}\"", val );

			// the 'cleaned' array string should appear like the following line...
			//  "(t1,i1),(t2,i2),(t3,i3),...(tn,in)"

			int32_t arrayCnt = 0;
			std::vector<std::string> infoArrayStrList = {};

			infoArrayStrList.clear();

			// now separate the array of illuminator info into the individual info elements
			arrayCnt = ParseCompositeArrayStrToStrList( infoArrayStrList, val );
			if ( arrayCnt != infoArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string infoArrayStr = "";
			std::vector<std::string> elementStrList = {};
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				illuminator_info_t ilInfo = {};
				int32_t eCnt = 0;

				infoArrayStr.clear();
				infoArrayStr = infoArrayStrList.at( aidx );

				// remove any leading and trailing array parentheses and quotes
				TrimStr( "{}()\"", infoArrayStr );

				elementStrList.clear();
				// break out the composite elements of this illuminator info
				// separate the illuminator info element string into the individual structure components
				eCnt = ParseIlluminatorInfoCompositeElementStrToStrList( elementStrList, infoArrayStr );
				if ( eCnt != elementStrList.size() || eCnt != 2 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				// now parse the extracted Illuminator info elements
				for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
				{
					tokenStr.clear();
					tokenStr = elementStrList.at( eidx );

					switch ( eidx )
					{
						case 0:			// illuminator-info element enum 0 = eIlluminatorType
						{
							ilInfo.type = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 1:			// illuminator-info element enum 1 = eIlluminatorIndex
						{
							ilInfo.index = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}
					}
				}
				icr.IlluminatorsInfoList.push_back( ilInfo );
			}
		}
		else if ( tag == CFG_IlluminatorCfgTag )			// "IlluminatorConfig"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.IlluminatorConfig = val;
		}
		else if ( tag == CFG_ConfigTypeTag )				// "ConfigType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.ConfigType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_LogNameTag )					// "LogName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.LogName = val;
		}
		else if ( tag == CFG_LogMaxSizeTag )				// "LogMaxSize"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.LogMaxSize = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_LogLevelTag )					// "LogSensitivity"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.LogSensitivity = val;
		}
		else if ( tag == CFG_MaxLogsTag )					// "MaxLogs"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.MaxLogs = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_AlwaysFlushTag )				// "AlwaysFlush"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.AlwaysFlush = boolFlag;
		}
		else if ( tag == CFG_CameraErrLogNameTag )			// "CameraErrorLogName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.CameraErrorLogName = val;
		}
		else if ( tag == CFG_CameraErrLogMaxSizeTag )		// "CameraErrorLogMaxSize"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.CameraErrorLogMaxSize = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_StorageErrLogNameTag )			// "StorageErrorLogName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			icr.StorageErrorLogName = val;
		}
		else if ( tag == CFG_StorageErrLogMaxSizeTag )		// "StorageErrorLogMaxSize"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.StorageErrorLogMaxSize = static_cast<int32_t>( fldRec.m_lVal );
		}

		else if ( tag == CFG_CarouselThetaHomeTag )			// "CarouselThetaHomeOffset"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.CarouselThetaHomeOffset = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_CarouselRadiusOffsetTag )		// "CarouselRadiusOffset"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.CarouselRadiusOffset = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_PlateThetaHomeTag )			// "PlateThetaHomePosOffset"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.PlateThetaHomeOffset = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_PlateThetaCalTag )				// "PlateThetaCalPos"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.PlateThetaCalPos = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_PlateRadiusCenterTag )			// "PlateRadiusCenterPos"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.PlateRadiusCenterPos = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_SaveImageTag )					// "SaveImage"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.SaveImage = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_FocusPositionTag )				// "FocusPosition"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.FocusPosition = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_AutoFocusTag )					// "AutoFocus"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces, parentheses, and element container quotes
			TrimStr( "{}()\"", val );

			std::vector<std::string> elementStrList = {};
			int32_t eCnt = 0;

			// first separate the af settings string into the individual structure components
			eCnt = ParseAFSettingsCompositeElementStrToStrList( elementStrList, val );
			if ( eCnt != elementStrList.size() || eCnt != 7 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			// now parse the extracted auto-focus settings elements
			for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
			{
				tokenStr.clear();
				tokenStr = elementStrList.at( eidx );

				switch ( eidx )
				{
					case 0:			// af-settings element enum 0 = eSaveImage
					{
						bool imgSave = true;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							imgSave = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							imgSave = false;
						}
						icr.AF_Settings.save_image = imgSave;
						break;
					}

					case 1:			// af-settings element enum 1 = eCoarseStart
					{
						icr.AF_Settings.coarse_start = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}

					case 2:			// af-settings element enum 2 = eCoarseEnd
					{
						icr.AF_Settings.coarse_end = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}

					case 3:			// af-settings element enum 3 = eCoarseStep
					{
						icr.AF_Settings.coarse_step = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 4:			// af-settings element enum 4 = eFineRange
					{
						icr.AF_Settings.fine_range = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}

					case 5:			// af-settings element enum 5 = eFineStep
					{
						icr.AF_Settings.fine_step = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 6:			// af-settings element enum 6 = eSharpnessthresh
					{
						icr.AF_Settings.sharpness_low_threshold = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}
				}
			}
		}
		else if ( tag == CFG_AbiMaxImageCntTag )			// "AbiMaxImageCount"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.AbiMaxImageCount = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_NudgeVolumeTag )				// "SampleNudgeVolume"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.SampleNudgeVolume = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_NudeSpeedTag )					// "SampleNudgeSpeed"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.SampleNudgeSpeed = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_FlowCellDepthTag )				// "FlowCellDepth"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.FlowCellDepth = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CFG_FlowCellConstantTag )			// "FlowCellDepthConstant"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.FlowCellDepthConstant = static_cast<float>( fldRec.m_fltVal );
		}
		else if ( tag == CFG_RfidSimTag )					// "RfidSim"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces, parentheses, and element container quotes
			TrimStr( "{}()\"", val );

			std::vector<std::string> elementStrList = {};
			int32_t eCnt = 0;

			// first separate the RFID-sim settings string into the individual structure components
			eCnt = ParseRfidSimCompositeElementStrToStrList( elementStrList, val );
			if ( eCnt != elementStrList.size() || eCnt != 5 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			// now parse the extracted RFID-sim settings elements
			for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
			{
				tokenStr.clear();
				tokenStr = elementStrList.at( eidx );

				switch ( eidx )
				{
					case 0:			// RFID-sim settings element enum 0 = eServerAddr
					{
						bool setValid = true;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							setValid = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							setValid = false;
						}
						icr.RfidSim.set_valid_tag_data = setValid;
						break;
					}

					case 1:			// RFID-sim settings element enum 1 = ePortNum
					{
						icr.RfidSim.total_tags = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 2:			// RFID-sim settings element enum 2 = eMainBayFile
					{
						DeSanitizeDataString( tokenStr );
						icr.RfidSim.main_bay_file = tokenStr;
						break;
					}

					case 3:			// RFID-sim settings element enum 3 = eDoorLeftFile
					{
						DeSanitizeDataString( tokenStr );
						icr.RfidSim.door_left_file = tokenStr;
						break;
					}

					case 4:			// RFID-sim settings element enum 4 = eDoorRightFile
					{
						DeSanitizeDataString( tokenStr );
						icr.RfidSim.door_right_file = tokenStr;
						break;
					}
				}
			}
		}
		else if ( tag == CFG_LegacyDataTag )				// "LegacyData"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.LegacyData = boolFlag;
		}
		else if ( tag == CFG_CarouselSimTag )				// "CarouselSimulator"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.CarouselSimulator = boolFlag;
		}
		else if ( tag == CFG_NightlyCleanOffsetTag )		// "NightlyCleanOffset"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.NightlyCleanOffset = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_LastNightlyCleanTag )			// "LastNightlyClean"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( icr.LastNightlyClean, val );
		}
		else if ( tag == CFG_SecurityModeTag )				// "SecurityMode"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.SecurityMode = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_InactivityTimeoutTag )			// "InactivityTimeout"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.InactivityTimeout = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_PwdExpirationTag )				// "PasswordExpiration"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.PasswordExpiration = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_NormalShutdownTag )			// "NormalShutdown"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.NormalShutdown = boolFlag;
		}
		else if ( tag == CFG_NextAnalysisDefIndexTag )		// "NextAnalysisDefIndex"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.NextAnalysisDefIndex = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_NextBCICellTypeIndexTag )		// "NextFactoryCellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )						// these should never be empty, as the default config record has the starting values and defaults
			{
				continue;
			}
			icr.NextBCICellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == CFG_NextUserCellTypeIndexTag )		// "NextUserCellTypeIndex"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )						// these should never be empty, as the default config record has the starting values and defaults
			{
				continue;
			}
			icr.NextUserCellTypeIndex = std::stoul( val.c_str() );
		}
		else if ( tag == CFG_TotSamplesProcessedTag )		// "SamplesProcessed"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.TotalSamplesProcessed = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == CFG_DiscardTrayCapacityTag )		// "DiscardCapacity"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.DiscardTrayCapacity = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == CFG_EmailServerTag )				// "EmailServer"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing element container quotes
			TrimStr( "\"", val );

			// next remove any leading and trailing array curly braces, parentheses
			TrimStr( "{}()", val );

			std::vector<std::string> elementStrList = {};
			int32_t eCnt = 0;

			// first separate the email settings string into the individual structure components
			eCnt = ParseEmailSettingsCompositeElementStrToStrList( elementStrList, val );
			if ( eCnt != elementStrList.size() || eCnt != 5 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			// now parse the extracted email settings elements
			for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
			{
				tokenStr.clear();
				tokenStr = elementStrList.at( eidx );

				switch ( eidx )
				{
					case 0:			// email-settings element enum 0 = eServerAddr
					{
						// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.Email_Settings.server_addr = tokenStr;
						break;
					}

					case 1:			// email-settings element enum 1 = ePortNum
					{
						icr.Email_Settings.port_number = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}

					case 2:			// email-settings element enum 2 = eAuthenticate
					{
						bool authenticate = true;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							authenticate = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							authenticate = false;
						}
						icr.Email_Settings.authenticate = authenticate;
						break;
					}

					case 3:			// email-settings element enum 3 = eUserName
					{
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.Email_Settings.username = tokenStr;
						break;
					}

					case 4:			// email-settings element enum 4 = ePwdHash
					{
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.Email_Settings.pwd_hash = tokenStr;
						break;
					}
				}
			}
		}
		else if ( tag == CFG_AdSettingsTag )				// "ADSettings"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing element container quotes
			TrimStr( "\"", val );

			// next remove any leading and trailing array curly braces, parentheses
			TrimStr( "{}()", val );

			std::vector<std::string> elementStrList = {};
			int32_t eCnt = 0;

			// first separate the active directory settings string into the individual structure components
			eCnt = ParseADSettingsCompositeElementStrToStrList( elementStrList, val );
			if ( eCnt != elementStrList.size() || eCnt != 5 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			icr.AD_Settings.enabled = false;		// set the default (for empty parse field or missing field)

			// now parse the extracted active-directory settings elements
			for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
			{
				tokenStr.clear();
				tokenStr = elementStrList.at( eidx );

				switch ( eidx )
				{
					case 0:			// ad-settings element enum 0 = eServerName
					{
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.AD_Settings.servername = tokenStr;
						break;
					}

					case 1:			// ad-settings element enum 1 = eServerAddr
					{
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.AD_Settings.server_addr = tokenStr;
						break;
					}

					case 2:			// ad-settings element enum 2 = ePortNum
					{
						icr.AD_Settings.port_number = static_cast<int32_t>( std::stoi( tokenStr ) );
						break;
					}

					case 3:			// ad-settings element enum 3 = eBaseDn
					{
						RemoveTgtCharFromStr( tokenStr, '\"' );
						DeSanitizeDataString( tokenStr );
						icr.AD_Settings.base_dn = tokenStr;
						break;
					}

					case 4:			// ad-settings element enum 3 = eEnabled
					{
						bool enabled = false;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							enabled = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							enabled = false;
						}
						icr.AD_Settings.enabled = enabled;
						break;
					}
				}
			}
		}
		else if ( tag == CFG_LanguageListTag )				// "LanguageList"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces and element container quotes; DO NOT REMOVE PARENTHESES AT THIS TIME!
			TrimStr( "{}\"", val );

			int32_t arrayCnt = 0;
			std::vector<std::string> langArrayStrList = {};

			langArrayStrList.clear();

			// first separate the array of blob data into the individual blob info
			arrayCnt = ParseCompositeArrayStrToStrList( langArrayStrList, val );
			if ( arrayCnt != langArrayStrList.size() )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string langArrayStr = "";
			std::vector<std::string> elementStrList = {};
			std::vector<std::string> tokenList = {};
			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			for ( int32_t aidx = 0; aidx < arrayCnt && parseOk; aidx++ )
			{
				language_info_t langInfo = {};
				int32_t eCnt = 0;

				langArrayStr.clear();
				langArrayStr = langArrayStrList.at( aidx );

				// next remove any leading and trailing array parentheses and quotes
				TrimStr( "{}()\"", langArrayStr );

				// Now break out the composite elements of this blob
				elementStrList.clear();
				eCnt = ParseLanguageInfoCompositeElementStrToStrList( elementStrList, langArrayStr );
				if ( eCnt != elementStrList.size() || eCnt != 4 )
				{
					retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
					parseOk = false;
					break;
				}

				// now parse the extracted blob data elements
				for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
				{
					tokenStr.clear();
					tokenStr = elementStrList.at( eidx );

					// NOTE: language ddesignator strings are not user accessible and are based on fixed values, so they are not sanitized/desanitized
					switch ( eidx )
					{
						case 0:			// language info element enum 0 = eLanguageId
						{
							langInfo.language_id = static_cast<int16_t>( std::stoi( tokenStr ) );
							break;
						}

						case 1:			// language info element enum 1 = eLanguageName
						{
							langInfo.language_name = tokenStr;
							break;
						}

						case 2:			// language info element enum 2 = eLanguageTag
						{
							langInfo.locale_tag = tokenStr;
							break;
						}

						case 3:			// language info element enum 3 = eLanguageActive
						{
							bool active = false;

							StrToLower( tokenStr );
							if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
							{
								active = true;
							}
							else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
							{
								active = false;
							}
							langInfo.active = active;
							break;
						}
					}
				}
				icr.LanguageList.push_back( langInfo );
			}
		}
		else if ( tag == CFG_RunOptionsTag )				// "RunOptionDefaults"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			if ( val.length() <= 0 )
			{
				continue;
			}

			// first, remove all the escaped quote marks ant the escaping backslash characters from the composite-containing array string
			RemoveSubStrFromStr( val, "\\\"" );

			// next remove any leading and trailing array curly braces, parentheses, and element container quotes
			TrimStr( "{}()\"", val );

			std::vector<std::string> elementStrList = {};
			int32_t eCnt = 0;

			// first separate the af settings string into the individual structure components
			eCnt = ParseRunOptionsCompositeElementStrToStrList( elementStrList, val );
			if ( eCnt != elementStrList.size() || eCnt != 15 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}

			std::string tokenStr = "";
			int32_t tokenCnt = 0;

			// now parse the extracted auto-focus settings elements
			for ( int32_t eidx = 0; eidx < eCnt && parseOk; eidx++ )
			{
				tokenStr.clear();
				tokenStr = elementStrList.at( eidx );

				switch ( eidx )
				{
					case 0:			// run_options element enum 0 = eSamleSetName
					{
						DeSanitizeDataString( tokenStr );
						icr.RunOptions.sample_set_name = tokenStr;
						break;
					}

					case 1:			// run_options element enum 1 = eSamleName
					{
						DeSanitizeDataString( tokenStr );
						icr.RunOptions.sample_name = tokenStr;
						break;
					}

					case 2:			// run_options element enum 2 = eSaveImageCount
					{
						icr.RunOptions.save_image_count = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 3:			// run_options element enum 3 = eSaveNthImage
					{
						icr.RunOptions.save_nth_image = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 4:			// run_options element enum 4 = eResultsExport
					{
						bool doExport = true;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							doExport = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							doExport = false;
						}
						icr.RunOptions.results_export = doExport;
						break;
					}

					case 5:			// run_options element enum 5 = eResultsExportFolder
					{
						RemoveSubStrFromStr( tokenStr, "\"" );
						DeSanitizePathString( tokenStr );
						icr.RunOptions.results_export_folder = tokenStr;
						break;
					}

					case 6:			// run_options element enum 6 = eAppendExport
					{
						bool doAppend = true;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							doAppend = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							doAppend = false;
						}
						icr.RunOptions.append_results_export = doAppend;
						break;
					}

					case 7:			// run_options element enum 7 = eAppendExportFolder
					{
						RemoveSubStrFromStr( tokenStr, "\"" );
						DeSanitizePathString( tokenStr );
						icr.RunOptions.append_results_export_folder = tokenStr;
						break;
					}

					case 8:			// run_options element enum 8 = eResultFilename
					{
						DeSanitizeDataString( tokenStr );
						icr.RunOptions.result_filename = tokenStr;
						break;
					}

					case 9:			// run_options element enum 9 = eResultsFolder
					{
						RemoveSubStrFromStr( tokenStr, "\"" );
						DeSanitizePathString( tokenStr );
						icr.RunOptions.results_folder = tokenStr;
						break;
					}

					case 10:		// run_options element enum 10 = ePdfExport
					{
						bool pdfExport = false;

						StrToLower( tokenStr );
						if ( tokenStr == TrueStr || tokenStr == "t" || tokenStr == "1" )
						{
							pdfExport = true;
						}
						else if ( tokenStr == FalseStr || tokenStr == "f" || tokenStr == "0" )
						{
							pdfExport = false;
						}
						icr.RunOptions.auto_export_pdf = pdfExport;
						break;
					}

					case 11:		// run_options element enum 11 = eCsvFolder
					{
						RemoveSubStrFromStr( tokenStr, "\"" );
						DeSanitizePathString( tokenStr );
						icr.RunOptions.csv_folder = tokenStr;
						break;
					}

					case 12:		// run_options element enum 12 = eWashType
					{
						icr.RunOptions.wash_type = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 13:		// run_options element enum 13 = eDilution
					{
						icr.RunOptions.dilution = static_cast<int16_t>( std::stoi( tokenStr ) );
						break;
					}

					case 14:		// run_options element enum 14 = eBpQcCellTypeIndex
					{
						icr.RunOptions.bpqc_cell_type_index = static_cast<uint32_t>( std::stoul( tokenStr ) );
						break;
					}
				}
			}
		}
		else if ( tag == CFG_AutomationInstalledTag )		// "AutomationInstalled"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.AutomationInstalled = boolFlag;
		}
		else if ( tag == CFG_AutomationEnabledTag )			// "AutomationEnabled"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			icr.AutomationEnabled = boolFlag;
		}
		else if (tag == CFG_ACupEnabledTag)					// "ACupEnabled"
		{
			bool boolFlag = DecodeBooleanRecordField(resultrec, tagStr);
			icr.ACupEnabled = boolFlag;
		}
		else if ( tag == CFG_AutomationPortTag )			// "AutomationPort"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			icr.AutomationPort = fldRec.m_lVal;
		}
		else if ( tag == ProtectedTag )						// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
//			icr.Protected = boolFlag;
		}
	}

	// if current setting is clearing the installed state, don't allow enable
	if ( !icr.AutomationInstalled )
	{
		icr.AutomationEnabled = false;
	}

	// if current setting is clearing the enabled state, don't allow ACup enable
	if ( !icr.AutomationEnabled )
	{
		icr.ACupEnabled = false;
	}

	queryresult = retrieveResult;

	return parseOk;
}

bool DBifImpl::ParseLogEntry( DBApi::eQueryResult& queryresult, DB_LogEntryRecord& logr,
							  CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	CDBVariant fldRec = {};

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == LOG_IdNumTag )					// "EntryIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			logr.IdNum = std::atoll( val.c_str() );
		}
		else if ( tag == LOG_EntryTypeTag )			// "EntryType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			logr.EntryType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == LOG_EntryDateTag )			// "EntryDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( logr.EntryDate, val );
		}
		else if ( tag == LOG_LogEntryTag )			// "EntryText"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			logr.EntryStr = val;
		}
		else if ( tag == ProtectedTag )				// "Protected" - not currently used
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			logr.Protected = boolFlag;
		}
	}

	bool parseOk = true;

	queryresult = DBApi::eQueryResult::QueryOk;

	return parseOk;
}

bool DBifImpl::ParseSchedulerConfig( DBApi::eQueryResult& queryresult, DB_SchedulerConfigRecord& scr,
									 CRecordset& resultrec, std::vector<std::string>& taglist )
{
	size_t tagListSize = taglist.size();

	if ( tagListSize <= 0 )
	{
		queryresult = DBApi::eQueryResult::NoData;
		return false;
	}

	bool parseOk = true;
	std::string tag = "";
	std::string val = "";
	CString tagStr = _T( "" );      // required for retrieval from recordset
	CString valStr = _T( "" );      // required for retrieval from recordset
	int32_t tagIndex = 0;
	int32_t tokenCnt = 0;
	uint32_t indexVal = 0;
	std::vector<std::string> tokenList = {};
	CDBVariant fldRec = {};
	DBApi::eQueryResult retrieveResult = DBApi::eQueryResult::QueryOk;

	for ( tagIndex = 0; tagIndex < tagListSize; tagIndex++ )
	{
		tag = taglist.at( tagIndex );
		tagStr = tag.c_str();
		fldRec.Clear();
		valStr.Empty();

		if ( tag == SCH_IdNumTag )							// "SchedulerConfigIdNum"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			scr.ConfigIdNum = std::atoll( val.c_str() );
		}
		else if ( tag == SCH_IdTag )						// "SchedulerConfigID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, scr.ConfigId );
		}
		else if ( tag == SCH_NameTag )						// "SchedulerName"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.Name = val;
		}
		else if ( tag == SCH_CommentsTag )					// "Comments"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.Comments = val;
		}
		else if ( tag == SCH_FilenameTemplateTag )			// "OutputFilenameTemplate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.FilenameTemplate = val;
		}
		else if ( tag == SCH_OwnerIdTag )					// "OwnerID"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			uuid__t_from_DB_UUID_Str( val, scr.OwnerUserId );
		}
		else if ( tag == SCH_CreationDateTag )				// "CreationDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( scr.CreationDate, val );
		}
		else if ( tag == SCH_OutputTypeTag )				// "OutputType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.OutputType = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_StartDateTag )					// "StartDate"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( scr.StartDate, val );
		}
		else if ( tag == SCH_StartOffsetTag )				// "StartOffset"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.StartOffset = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_RepetitionIntervalTag )		// "RepetitionInterval"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.MultiRepeatInterval = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_DayWeekIndicatorTag )			// "DayWeekIndicator"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.DayWeekIndicator = static_cast<uint16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_MonthlyRunDayTag )				// "MonthlyRunDay"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.MonthlyRunDay = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_DestinationFolderTag )			// "DestinationFolder"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizePathString( val );
			scr.DestinationFolder = val;
		}
		else if ( tag == SCH_DataTypeTag )					// "DataType"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.DataType = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == SCH_FilterTypesTag )				// "FilterTypesList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif

			indexVal = 0;
			for ( int32_t i = 0; i < tokenCnt; i++ )
			{
				val = tokenList.at( i );
				indexVal = std::stol( val.c_str() );
				scr.FilterTypesList.push_back( static_cast<eListFilterCriteria>(indexVal) );
			}
		}
		else if ( tag == SCH_CompareOpsTag )				// "CompareOpsList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, scr.CompareOpsList );
		}
		else if ( tag == SCH_CompareValsTag )				// "CompareValsList"
		{
			tokenList.clear();
			tokenCnt = 0;

			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			tokenCnt = ParseArrayStrToStrList( tokenList, val );
#ifndef ALLOW_ZERO_TOKENS
			if ( tokenCnt <= 0 )
			{
				retrieveResult = DBApi::eQueryResult::BadOrMissingArrayVals;
				parseOk = false;
				break;
			}
#endif
			DeSanitizeDataStringList( tokenList, scr.CompareValsList );
		}
		else if ( tag == SCH_EnabledTag )					// "Enabled"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			scr.Enabled = boolFlag;
		}
		else if ( tag == SCH_LastRunTimeTag )				// "LastRunTime"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( scr.LastRunTime, val );
		}
		else if ( tag == SCH_LastSuccessRunTimeTag )		// "LastSuccessRunTime"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			GetTimePtFromDbTimeString( scr.LastSuccessRunTime, val );
		}
		else if ( tag == SCH_LastRunStatusTag )				// "LastRunStatus"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.LastRunStatus = static_cast<int16_t>( fldRec.m_iVal );
		}
		else if ( tag == SCH_NotificationEmailTag )			// "NotificationEmail"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.NotificationEmail = val;
		}
		else if ( tag == SCH_EmailServerTag )				// "EmailServer"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.EmailServerAddr = val;
		}
		else if ( tag == SCH_EmailServerPortTag )			// "EmailServerPort"
		{
			resultrec.GetFieldValue( tagStr, fldRec );
			scr.EmailServerPort = static_cast<int32_t>( fldRec.m_lVal );
		}
		else if ( tag == SCH_AuthenticateEmailTag )			// "AuthenticateEmail"
		{
			bool boolFlag = DecodeBooleanRecordField( resultrec, tagStr );
			scr.AuthenticateEmail = boolFlag;
		}
		else if ( tag == SCH_EmailAccountTag )				// "EmailAccount"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.AccountUsername = val;
		}
		else if ( tag == SCH_AccountAuthenticatorTag )		// "EmailAccountAuthenticator"
		{
			resultrec.GetFieldValue( tagStr, valStr );
			val = CT2A( valStr );
			DeSanitizeDataString( val );
			scr.AccountPwdHash = val;
		}
	}

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

	// the following section attempts to fix a problem encountered during testing where the search filter
	// parameter lsits contain differing numbers of parameters due to the use of the 'SinceLastRun' date filter,
	// which automatically fills-in some parameters.  The problem SHOULD NOT exist under normal conditions, but
	// this ensures lists  are properly populated on return to the calling client.
#if defined(VECTOR_FIX)
	int32_t filterTypeTags = (int) scr.FilterTypesList.size();
	int32_t filterOpTags = (int) scr.CompareOpsList.size();
	int32_t filterValTags = (int) scr.CompareValsList.size();

	// hack for unequal vector contents... key value is the filter-types list
	if ( filterTypeTags > 0 && ( filterTypeTags != filterOpTags || filterTypeTags != filterValTags ) )
	{
		size_t tagIdx = 0;
		bool tagFound = false;
		system_TP zeroTP = {};
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
#endif

	queryresult = retrieveResult;

	return parseOk;
}

