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



static const std::string MODULENAME = "DBif_Filter";



// the following list tyes are defined for use;
/*

case DBApi::eListType::NoListType:
case DBApi::eListType::WorklistList:
case DBApi::eListType::SampleSetList:
case DBApi::eListType::SampleItemList:
case DBApi::eListType::SampleList:
case DBApi::eListType::AnalysisList:
case DBApi::eListType::SummaryResultList:
case DBApi::eListType::DetailedResultList:
case DBApi::eListType::ImageResultsList:
case DBApi::eListType::SResultList:
case DBApi::eListType::ClusterDataList:
case DBApi::eListType::BlobDataList:
case DBApi::eListType::ImageSetList:
case DBApi::eListType::ImageSequenceList:
case DBApi::eListType::ImageList:
case DBApi::eListType::CellTypeList:
case DBApi::eListType::ImageAnalysisParamList:
case DBApi::eListType::AnalysisInputSettingsList:
case DBApi::eListType::ImageAnalysisCellIdentParamList:
case DBApi::eListType::AnalysisDefinitionList:
case DBApi::eListType::AnalysisParamList:
case DBApi::eListType::IlluminatorList:
case DBApi::eListType::RolesList:
case DBApi::eListType::UserList:
case DBApi::eListType::UserPropertiesList:
case DBApi::eListType::SignatureDefList:
case DBApi::eListType::ReagentInfoList:
case DBApi::eListType::WorkflowList:
case DBApi::eListType::BioProcessList:
case DBApi::eListType::QcProcessList:
case DBApi::eListType::CalibrationsList:
case DBApi::eListType::InstrumentConfigList:
case DBApi::eListType::LogEntryList:
case DBApi::eListType::SchedulerConfigList:
*/


// the following list filter criteria are defined for use;
// list filter criteria are usually a retrieval qualification
/*

case DBApi::eListFilterCriteria::FilterNotDefined:
case DBApi::eListFilterCriteria::NoFilter:
case DBApi::eListFilterCriteria::IdFilter:
case DBApi::eListFilterCriteria::IdNumFilter:
case DBApi::eListFilterCriteria::IndexFilter:
case DBApi::eListFilterCriteria::ItemNameFilter:
case DBApi::eListFilterCriteria::LabelFilter:
case DBApi::eListFilterCriteria::StatusFilter:
case DBApi::eListFilterCriteria::OwnerFilter:
case DBApi::eListFilterCriteria::ParentFilter:
case DBApi::eListFilterCriteria::CarrierFilter:
case DBApi::eListFilterCriteria::CreationDateFilter:
case DBApi::eListFilterCriteria::CreationDateRangeFilter:
case DBApi::eListFilterCriteria::CreationUserIdFilter:
case DBApi::eListFilterCriteria::CreationUserNameFilter:
case DBApi::eListFilterCriteria::ModifyDateFilter:
case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
case DBApi::eListFilterCriteria::RunDateFilter:
case DBApi::eListFilterCriteria::RunDateRangeFilter:
case DBApi::eListFilterCriteria::RunUserIdFilter:
case DBApi::eListFilterCriteria::RunUserNameFilter:
case DBApi::eListFilterCriteria::SampleAcquiredFilter:
case DBApi::eListFilterCriteria::AcquisitionDateFilter:
case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
case DBApi::eListFilterCriteria::SampleIdFilter:
case DBApi::eListFilterCriteria::CellTypeIdxFilter:
case DBApi::eListFilterCriteria::CellTypeFilter:
case DBApi::eListFilterCriteria::CellTypeIdFilter:
case DBApi::eListFilterCriteria::LotFilter:
case DBApi::eListFilterCriteria::QcFilter:
case DBApi::eListFilterCriteria::InstrumentFilter:
case DBApi::eListFilterCriteria::CommentsFilter:
case DBApi::eListFilterCriteria::RoleFilter:
case DBApi::eListFilterCriteria::UserTypeFilter:
case DBApi::eListFilterCriteria::CalTypeFilter:
case DBApi::eListFilterCriteria::LogTypeFilter:

*/


////////////////////////////////////////////////////////////////////////////////
// Query filter string construction
////////////////////////////////////////////////////////////////////////////////

bool DBifImpl::GetWorklistListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;
	
	if ( compareops.size() != comparevals.size() || compareops.size() == 0)
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = WL_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = WL_NameTag;
				break;

			case DBApi::eListFilterCriteria::StatusFilter:
				tagStr = WL_StatusTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
				tagStr = WL_CreateUserIdTag;
				break;

			case DBApi::eListFilterCriteria::CarrierFilter:
				filterOk = CheckCarrierFilter( tagStr, WL_CarrierTypeTag, opStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CreationUserIdFilter:
				tagStr = WL_CreateUserIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationUserNameFilter:
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::ModifyDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
				filterOk = CheckValueFilter( tagStr, WL_RunDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = WL_RunDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::RunUserIdFilter:
				tagStr = WL_RunUserIdTag;
				break;

			case DBApi::eListFilterCriteria::RunUserNameFilter:
				break;

			case DBApi::eListFilterCriteria::SampleAcquiredFilter:
				filterOk = CheckValueFilter( tagStr, WL_AcquireSampleTag, opStr, true, valStr );
				break;

			case DBApi::eListFilterCriteria::CellTypeIdxFilter:
				tagStr = WL_CellTypeIdxTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdFilter:
				tagStr = WL_CellTypeIdTag;
				break;

			case DBApi::eListFilterCriteria::QcFilter:
				tagStr = WL_QcProcessIdTag;
				break;

			case DBApi::eListFilterCriteria::InstrumentFilter:
				tagStr = WL_InstSNTag;
				break;

			case DBApi::eListFilterCriteria::CommentsFilter:
				tagStr = WL_CommentsTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetWorklistListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetWorklistListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSampleSetListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									   std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SS_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = SS_NameTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = SS_LabelTag;
				break;

			case DBApi::eListFilterCriteria::StatusFilter:
				tagStr = SS_StatusTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
				tagStr = SS_OwnerIdTag;
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = SS_WorklistIdTag;
				break;

			case DBApi::eListFilterCriteria::CarrierFilter:
				filterOk = CheckCarrierFilter( tagStr, SS_CarrierTypeTag, opStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
				filterOk = CheckValueFilter( tagStr, SS_CreateDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
				tagStr = SS_CreateDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CreationUserIdFilter:
				tagStr = SS_OwnerIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationUserNameFilter:
				break;

			case DBApi::eListFilterCriteria::ModifyDateFilter:
				filterOk = CheckValueFilter( tagStr, SS_ModifyDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
				tagStr = SS_ModifyDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
				filterOk = CheckValueFilter( tagStr, SS_RunDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = SS_RunDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CommentsFilter:
				tagStr = SS_CommentsTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSampleSetListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSampleSetListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSampleItemListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SI_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = SI_NameTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = SI_LabelTag;
				break;

			case DBApi::eListFilterCriteria::StatusFilter:
				tagStr = SI_StatusTag;
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = SI_SampleSetIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::ModifyDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
				filterOk = CheckValueFilter( tagStr, SI_RunDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = SI_RunDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = SI_SampleIdTag;
				break;

			case DBApi::eListFilterCriteria::CommentsFilter:
				tagStr = SI_CommentsTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdxFilter:
				tagStr = SI_CellTypeIdxTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdFilter:
				tagStr = SI_CellTypeIdTag;
				break;

			case DBApi::eListFilterCriteria::QcFilter:
				tagStr = SI_QcProcessIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSampleItemListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSampleItemListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSampleListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SM_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = SM_NameTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = SM_LabelTag;
				break;

			case DBApi::eListFilterCriteria::StatusFilter:
				tagStr = SM_StatusTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
				tagStr = SM_OwnerUserIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
			case DBApi::eListFilterCriteria::SinceLastDateFilter:
				filterOk = CheckValueFilter( tagStr, SM_AcquireDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = SM_AcquireDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CreationUserIdFilter:
			case DBApi::eListFilterCriteria::RunUserIdFilter:
				tagStr = SM_RunUserTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdxFilter:
			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = SM_CellTypeIdxTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdFilter:
				tagStr = SM_CellTypeIdTag;
				break;

			case DBApi::eListFilterCriteria::LotFilter:
				break;

			case DBApi::eListFilterCriteria::QcFilter:
				tagStr = SM_QcProcessIdTag;
				break;

			case DBApi::eListFilterCriteria::InstrumentFilter:
				tagStr = SM_InstSNTag;
				break;

			case DBApi::eListFilterCriteria::CommentsFilter:
				tagStr = SM_CommentsTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSampleListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSampleListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetAnalysisListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = AN_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::SinceLastDateFilter:
				filterOk = CheckValueFilter( tagStr, AN_AnalysisDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
				tagStr = AN_AnalysisDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
			case DBApi::eListFilterCriteria::CreationUserIdFilter:
			case DBApi::eListFilterCriteria::RunUserIdFilter:
				tagStr = AN_RunUserIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationUserNameFilter:
			case DBApi::eListFilterCriteria::RunUserNameFilter:
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = AN_SampleIdTag;
				break;

			case DBApi::eListFilterCriteria::QcFilter:
				tagStr = AN_QcProcessIdTag;
				break;

			case DBApi::eListFilterCriteria::InstrumentFilter:
				tagStr = AN_InstSNTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetAnalysisListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetAnalysisListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSummaryResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = RS_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = RS_AnalysisIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
				filterOk = CheckValueFilter( tagStr, RS_ResultDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
				tagStr = RS_ResultDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = RS_SampleIdTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdxFilter:
				tagStr = RS_CellTypeIdxTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdFilter:
				tagStr = RS_CellTypeIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSummaryResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSummaryResultListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetDetailedResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = RD_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
				tagStr = RD_OwnerIdTag;
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = RD_AnalysisIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
				filterOk = CheckValueFilter( tagStr, RD_ResultDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
				tagStr = RD_ResultDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = RD_SampleIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetDetailedResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetDetailedResultListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetImageResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = RI_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = RI_AnalysisIdTag;
				break;

			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = RI_SampleIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;




	return filterOk;
}

bool DBifImpl::GetImageResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes;
	std::vector<std::string> compareOps;
	std::vector<std::string> compareVals;

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetImageResultListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SR_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = SR_AnalysisIdTag;
				break;

			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = SR_SampleIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									 std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSResultListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetImageSetListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IC_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
				filterOk = CheckValueFilter( tagStr, IC_CreationDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
				tagStr = IC_CreationDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::OwnerFilter:
			case DBApi::eListFilterCriteria::SampleIdFilter:
				tagStr = IC_SampleIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetImageSetListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetImageSetListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetImageSequenceListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										   std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IS_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = IS_SequenceNumTag;
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = IS_SetIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetImageSequenceListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetImageSequenceListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetImageListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IM_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ParentFilter:
				tagStr = IM_SequenceIdTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetImageListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetImageListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetCellTypeListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = CT_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
			case DBApi::eListFilterCriteria::CellTypeIdxFilter:
				tagStr = CT_IdxTag;
				break;

			case DBApi::eListFilterCriteria::CellTypeIdFilter:
				tagStr = CT_IdTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = CT_NameTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetCellTypeListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetCellTypeListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetImageAnalysisParamListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IAP_IdNumTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetImageAnalysisParamListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
												 std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetImageAnalysisParamListFilter( wherestr, filterTypes, compareOps, compareVals );
}


bool DBifImpl::GetAnalysisInputSettingsListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
														std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	bool filterOk = true;
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IAP_IdNumTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetAnalysisInputSettingsListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
														std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetAnalysisInputSettingsListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetAnalysisDefinitionListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = AD_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = AD_IdxTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = AD_NameTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetAnalysisDefinitionListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
												std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetAnalysisDefinitionListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetAnalysisParamListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = AP_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = AP_LabelTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetAnalysisParamListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetAnalysisParamListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetIlluminatorListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IL_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = IL_IdxTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = IL_NameTag;
				break;
			case DBApi::eListFilterCriteria::IlluminatorType:
				tagStr = IL_TypeTag;
				break;

		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetIlluminatorListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										 std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetIlluminatorListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetUserListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = UR_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = UR_UserNameTag;
				break;

			case DBApi::eListFilterCriteria::UserTypeFilter:
				tagStr = UR_ADUserTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetUserListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								  std::string compareop, std::string compareval)
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetUserListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetRoleListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = RO_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = RO_NameTag;
				break;

		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetRoleListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								  std::string compareop, std::string compareval,
								  DBApi::eRoleClass byrole )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetRoleListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetUserPropertiesListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = UP_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = UP_IdxTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = UP_NameTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = UP_TypeTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetUserPropertiesListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetUserPropertiesListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSignatureListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									   std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SG_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = SG_LongSigTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSignatureListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSignatureListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetReagentInfoListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = RX_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = RX_ContainerRfidSNTag;
				break;

			case DBApi::eListFilterCriteria::LabelFilter:
				tagStr = RX_PackPartNumTag;
				break;

			case DBApi::eListFilterCriteria::LotFilter:
				tagStr = RX_LotNumTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::ModifyDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
				filterOk = CheckValueFilter( tagStr, RX_LotExpirationDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = RX_LotExpirationDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}


bool DBifImpl::GetReagentInfoListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetReagentInfoListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetWorkflowListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	bool filterOk = true;

	return filterOk;
}

bool DBifImpl::GetWorkflowListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetWorkflowListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetBioProcessListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = BP_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = BP_NameTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetBioProcessListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetBioProcessListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetQcProcessListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									   std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = QC_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = QC_NameTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetQcProcessListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									   std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetQcProcessListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetCalibrationListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = CC_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::CreationUserIdFilter:
			case DBApi::eListFilterCriteria::RunUserIdFilter:
				tagStr = CC_CalUserIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
			case DBApi::eListFilterCriteria::ModifyDateFilter:
			case DBApi::eListFilterCriteria::RunDateFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateFilter:
				filterOk = CheckValueFilter( tagStr, CC_CalDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
			case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
			case DBApi::eListFilterCriteria::RunDateRangeFilter:
			case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
				tagStr = CC_CalDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::InstrumentFilter:
				tagStr = CC_InstSNTag;
				break;

			case DBApi::eListFilterCriteria::CalTypeFilter:
				tagStr = CC_CalTypeTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetCalibrationListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										 std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetCalibrationListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetCellHealthReagentsListFilter(std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
	std::vector<std::string> compareops, std::vector<std::string> comparevals)
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if (compareops.size() != comparevals.size() || compareops.size() == 0)
	{
		return false;
	}

	for (opIdx = 0; opIdx < opCnt && filterOk; opIdx++)
	{
		if (!FilterPreChecksOK(opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr))
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch (filterType)
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = IL_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::IndexFilter:
				tagStr = IL_IdxTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = IL_NameTag;
				break;
		}

		if (filterOk)
		{
			if (tagStr.length() > 0 && filterStr.empty())
			{
				filterStr.append(boost::str(boost::format("\"%s\" %s %s") % tagStr % opStr % valStr));
			}

			if (filterStr.length() > 0)
			{
				if (whereStr.length() > 0)
				{
					whereStr.append(" AND ");
				}
				whereStr.append(boost::str(boost::format("%s") % filterStr));
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetCellHealthReagentsListFilter(std::string& wherestr, DBApi::eListFilterCriteria filtertype,
	std::string compareop, std::string compareval)
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back(filtertype);
	compareOps.push_back(compareop);
	compareVals.push_back(compareval);

	return GetCellHealthReagentsListFilter(wherestr, filterTypes, compareOps, compareVals);
}

bool DBifImpl::GetLogEntryListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = LOG_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
				filterOk = CheckValueFilter( tagStr, LOG_EntryDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
				tagStr = LOG_EntryDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::LogTypeFilter:
				tagStr = LOG_EntryTypeTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetLogEntryListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetLogEntryListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetSchedulerConfigListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											 std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	std::string whereStr = "";
	std::string filterStr = "";
	std::string tagStr = "";
	std::string opStr = "";
	std::string valStr = "";
	bool filterOk = true;
	size_t opCnt = compareops.size();
	int32_t opIdx = 0;
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::FilterNotDefined;

	if ( compareops.size() != comparevals.size() || compareops.size() == 0 )
	{
		return false;
	}

	for ( opIdx = 0; opIdx < opCnt && filterOk; opIdx++ )
	{
		if ( !FilterPreChecksOK( opIdx, filtertypes, compareops, comparevals, filterType, opStr, valStr ) )
		{
			continue;
		}

		tagStr.clear();
		filterStr.clear();

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::IdNumFilter:
				tagStr = SCH_IdNumTag;
				break;

			case DBApi::eListFilterCriteria::ItemNameFilter:
				tagStr = SCH_NameTag;
				break;

			case DBApi::eListFilterCriteria::StatusFilter:
				tagStr = SCH_LastRunStatusTag;
				break;

			case DBApi::eListFilterCriteria::CreationUserIdFilter:
			case DBApi::eListFilterCriteria::RunUserIdFilter:
				tagStr = SCH_OwnerIdTag;
				break;

			case DBApi::eListFilterCriteria::CreationDateFilter:
				filterOk = CheckValueFilter( tagStr, SCH_CreationDateTag, opStr );
				break;

			case DBApi::eListFilterCriteria::RunDateFilter:
				filterOk = CheckValueFilter( tagStr, SCH_LastRunTimeTag, opStr );
				break;

			case DBApi::eListFilterCriteria::CreationDateRangeFilter:
				tagStr = SCH_CreationDateTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::RunDateRangeFilter:
				tagStr = SCH_LastRunTimeTag;
				filterOk = FormatDateRangeFilter( filterStr, tagStr, valStr );
				break;

			case DBApi::eListFilterCriteria::CommentsFilter:
				tagStr = SCH_CommentsTag;
				break;
		}

		if ( filterOk )
		{
			if ( tagStr.length() > 0 && filterStr.empty() )
			{
				if ( IsSingleDateFilter( filterType ) )
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s '%s'" ) % tagStr % opStr % valStr ) );
				}
				else
				{
					filterStr.append( boost::str( boost::format( "\"%s\" %s %s" ) % tagStr % opStr % valStr ) );
				}
			}

			if ( filterStr.length() > 0 )
			{
				if ( whereStr.length() > 0 )
				{
					whereStr.append( " AND " );
				}
				whereStr.append( boost::str( boost::format( "%s" ) % filterStr ) );
			}
		}
	}
	wherestr = whereStr;

	return filterOk;
}

bool DBifImpl::GetSchedulerConfigListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											 std::string compareop, std::string compareval )
{
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOps = {};
	std::vector<std::string> compareVals = {};

	filterTypes.push_back( filtertype );
	compareOps.push_back( compareop );
	compareVals.push_back( compareval );

	return GetSchedulerConfigListFilter( wherestr, filterTypes, compareOps, compareVals );
}

bool DBifImpl::GetListFilterString( DBApi::eListType listtype, std::vector<DBApi::eListFilterCriteria> filtertypes,
									std::string& wherestring, std::vector<std::string> compareops, std::vector<std::string> comparevals )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string whereStr = "";
	bool filterOk = true;

	// never overwrite an existing filter string...
	if ( wherestring.length() == 0 && filtertypes.size() > 0 )
	{
		switch ( listtype )
		{
			case DBApi::eListType::WorklistList:
				filterOk = GetWorklistListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SampleSetList:
				filterOk = GetSampleSetListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SampleItemList:
				filterOk = GetSampleItemListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SampleList:
				filterOk = GetSampleListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::AnalysisList:
				filterOk = GetAnalysisListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SummaryResultList:
				filterOk = GetSummaryResultListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::DetailedResultList:
				filterOk = GetDetailedResultListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ImageResultList:
				filterOk = GetImageResultListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SResultList:
				filterOk = GetSResultListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ImageSetList:
				filterOk = GetImageSetListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ImageSequenceList:
				filterOk = GetImageSequenceListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ImageList:
				filterOk = GetImageListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::CellTypeList:
				filterOk = GetCellTypeListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ImageAnalysisParamList:
				filterOk = GetImageAnalysisParamListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::AnalysisInputSettingsList:
				filterOk = GetAnalysisInputSettingsListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::AnalysisDefinitionList:
				filterOk = GetAnalysisDefinitionListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::AnalysisParamList:
				filterOk = GetAnalysisParamListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::IlluminatorList:
				filterOk = GetIlluminatorListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::UserList:
				filterOk = GetUserListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::UserPropertiesList:
				filterOk = GetUserPropertiesListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::RolesList:
				filterOk = GetRoleListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SignatureDefList:
				filterOk = GetSignatureListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::ReagentInfoList:
				filterOk = GetReagentInfoListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::WorkflowList:
//				filterOk = GetWorkflowListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::BioProcessList:
				filterOk = GetBioProcessListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::QcProcessList:
				filterOk = GetQcProcessListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::LogEntryList:
				filterOk = GetLogEntryListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::SchedulerConfigList:
				filterOk = GetSchedulerConfigListFilter( whereStr, filtertypes, compareops, comparevals );
				break;

			case DBApi::eListType::CellHealthReagentsList:
				filterOk = GetCellHealthReagentsListFilter(whereStr, filtertypes, compareops, comparevals);
			
				break;

			case DBApi::eListType::InstrumentConfigList:
			case DBApi::eListType::NoListType:
			default:
				break;
		}
	}

	if ( filterOk && ( wherestring.length() > 0 || whereStr.length() > 0 ) )
	{
		std::string whereFilterStr = "";

		// pre-pend any passed-in order string
		if ( wherestring.length() > 0 )
		{
			whereFilterStr.append( wherestring );
			whereFilterStr.append( " AND " );
		}

		if ( whereStr.length() > 0 )
		{
			whereFilterStr.append( whereStr );
		}

		wherestring = whereFilterStr;
	}
	return filterOk;
}

bool DBifImpl::GetListFilterString( DBApi::eListType listtype, DBApi::eListFilterCriteria filtertype,
									std::string& wherestring, std::string compareop, std::string compareval )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string whereStr = "";
	bool filterOk = true;

	// never overwrite an existing filter string...
	if ( ( wherestring.length() == 0 ) && ( filtertype != DBApi::eListFilterCriteria::FilterNotDefined ) )
	{
		switch ( listtype )
		{
			case DBApi::eListType::WorklistList:
				filterOk = GetWorklistListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SampleSetList:
				filterOk = GetSampleSetListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SampleItemList:
				filterOk = GetSampleItemListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SampleList:
				filterOk = GetSampleListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::AnalysisList:
				filterOk = GetAnalysisListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SummaryResultList:
				filterOk = GetSummaryResultListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::DetailedResultList:
				filterOk = GetDetailedResultListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ImageResultList:
				filterOk = GetImageResultListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SResultList:
				filterOk = GetSResultListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ImageSetList:
				filterOk = GetImageSetListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ImageSequenceList:
				filterOk = GetImageSequenceListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ImageList:
				filterOk = GetImageListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::CellTypeList:
				filterOk = GetCellTypeListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ImageAnalysisParamList:
				filterOk = GetImageAnalysisParamListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::AnalysisInputSettingsList:
				filterOk = GetAnalysisInputSettingsListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::AnalysisDefinitionList:
				filterOk = GetAnalysisDefinitionListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::AnalysisParamList:
				filterOk = GetAnalysisParamListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::IlluminatorList:
				filterOk = GetIlluminatorListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::UserList:
				filterOk = GetUserListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::UserPropertiesList:
				filterOk = GetUserPropertiesListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::RolesList:
				filterOk = GetRoleListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SignatureDefList:
				filterOk = GetSignatureListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::ReagentInfoList:
				filterOk = GetReagentInfoListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::WorkflowList:
//				filterOk = GetWorkflowListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::BioProcessList:
				filterOk = GetBioProcessListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::QcProcessList:
				filterOk = GetQcProcessListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::CalibrationsList:
				filterOk = GetCalibrationListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::LogEntryList:
				filterOk = GetLogEntryListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::SchedulerConfigList:
				filterOk = GetSchedulerConfigListFilter( whereStr, filtertype, compareop, compareval );
				break;

			case DBApi::eListType::InstrumentConfigList:
			case DBApi::eListType::NoListType:
			default:
				break;
		}
	}

	if ( filterOk && ( wherestring.length() > 0 || whereStr.length() > 0 ) )
	{
		std::string whereFilterStr = "";

		// pre-pend any passed-in order string
		if ( wherestring.length() > 0 )
		{
			whereFilterStr.append( wherestring );
			whereFilterStr.append( " AND " );
		}

		if ( whereStr.length() > 0 )
		{
			whereFilterStr.append( whereStr );
		}

		wherestring = whereFilterStr;
	}
	return filterOk;
}

// validate non-string value comparators...
bool DBifImpl::FilterPreChecksOK( int index, std::vector<DBApi::eListFilterCriteria> filtertypes,
								  std::vector<std::string> compareops, std::vector<std::string> comparevals,
								  DBApi::eListFilterCriteria& filtertype, std::string& compareop, std::string& compareval )
{
	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::NoFilter;
	std::string opStr = "";
	std::string valStr = "";

	filtertype = DBApi::eListFilterCriteria::FilterNotDefined;
	compareop.clear();
	compareval.clear();

	filterType = filtertypes.at( index );

	if ( filterType == DBApi::eListFilterCriteria::FilterNotDefined ||
		 filterType == DBApi::eListFilterCriteria::NoFilter )
	{
		return false;
	}

	opStr = compareops.at( index );

	if ( !CompareOpValid( opStr, filterType ) )
	{
		return false;
	}

	valStr = comparevals.at( index );

	if ( !ValueStringOk( filterType, valStr ) )		// check for the '*' comparison value as an illegal/unnecessary comparison target
	{
		filterType = DBApi::eListFilterCriteria::NoFilter;
		opStr.clear();
		valStr.clear();
	}

	if ( filterType != DBApi::eListFilterCriteria::FilterNotDefined &&
		 filterType != DBApi::eListFilterCriteria::NoFilter )
	{
		if ( !DbFilterValueIsString( filterType ) )		// do not force case for string values; compare operator will determine case-sensitivity of search
		{
			StrToLower( valStr );
		}

		switch ( filterType )
		{
			case DBApi::eListFilterCriteria::ItemNameFilter:
			case DBApi::eListFilterCriteria::LabelFilter:
			case DBApi::eListFilterCriteria::LotFilter:
			case DBApi::eListFilterCriteria::CommentsFilter:
				SanitizeFilterString( valStr );
				break;
		}
	}

	filtertype = filterType;
	compareop = opStr;
	compareval = valStr;

	return true;
}

// validate non-string value comparators...
bool DBifImpl::CheckValueFilter( std::string& filterstr, std::string fieldtag, std::string compareop, bool tfchk, std::string compareval )
{
	bool compareOpOk = false;

	if ( compareop == ">" ||
		 compareop == "<" ||
		 compareop == "=" ||
		 compareop == "<=" ||
		 compareop == ">=" ||
		 compareop == "<>" ||
		 compareop == "!=" )
	{
		if ( tfchk )
		{
			if ( compareop == "=" && ( compareval == TrueStr || compareval == FalseStr ) )
			{
				compareOpOk = true;
			}
		}
		else
		{
			compareOpOk = true;
		}
	}

	if ( compareOpOk )
	{
		filterstr.append( fieldtag );
	}
	else
	{
		filterstr.clear();
	}

	return compareOpOk;
}

// validate non-string value comparators...
bool DBifImpl::ValueStringOk( eListFilterCriteria filtertype , std::string compareval )
{
	// for filter-types acting on string fields, '*' is not a valid search operand value; then search will default to the 'all' condition normaly
	switch ( filtertype )
	{
		case DBApi::eListFilterCriteria::ItemNameFilter:
		case DBApi::eListFilterCriteria::LabelFilter:
		case DBApi::eListFilterCriteria::CreationDateFilter:
		case DBApi::eListFilterCriteria::CreationDateRangeFilter:
		case DBApi::eListFilterCriteria::CreationUserNameFilter:
		case DBApi::eListFilterCriteria::ModifyDateFilter:
		case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
		case DBApi::eListFilterCriteria::RunDateFilter:
		case DBApi::eListFilterCriteria::RunDateRangeFilter:
		case DBApi::eListFilterCriteria::RunUserNameFilter:
		case DBApi::eListFilterCriteria::AcquisitionDateFilter:
		case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
		case DBApi::eListFilterCriteria::LotFilter:
		case DBApi::eListFilterCriteria::InstrumentFilter:
		case DBApi::eListFilterCriteria::CommentsFilter:
			if ( compareval == "*" )
			{
				return false;
			}
			break;
	}

	return true;
}

// validate the allowed possible comparison opertors based on the list filter criteria
bool DBifImpl::CompareOpValid( std::string& compareop, eListFilterCriteria filtertype )
{
	bool compareOpOk = false;

	StrToLower( compareop );	// case conversion OK as this is a copy of the op string...

	switch ( filtertype )
	{
		case DBApi::eListFilterCriteria::IdNumFilter:
		case DBApi::eListFilterCriteria::IndexFilter:
		case DBApi::eListFilterCriteria::StatusFilter:
		case DBApi::eListFilterCriteria::IlluminatorType:
		case DBApi::eListFilterCriteria::CellTypeIdxFilter:
		{
			if ( compareop == "=" ||
				 compareop == "<" || compareop == "<=" ||
				 compareop == ">" || compareop == ">=" ||
				 compareop == "<>" || compareop == "!=" )
			{
				compareOpOk = true;
			}
			break;
		}

		case DBApi::eListFilterCriteria::IdFilter:
		case DBApi::eListFilterCriteria::OwnerFilter:
		case DBApi::eListFilterCriteria::ParentFilter:
		case DBApi::eListFilterCriteria::CarrierFilter:
		case DBApi::eListFilterCriteria::CreationUserIdFilter:
		case DBApi::eListFilterCriteria::CreationUserNameFilter:
		case DBApi::eListFilterCriteria::RunUserIdFilter:
		case DBApi::eListFilterCriteria::RunUserNameFilter:
		case DBApi::eListFilterCriteria::SampleAcquiredFilter:
		case DBApi::eListFilterCriteria::SampleIdFilter:
		case DBApi::eListFilterCriteria::CellTypeIdFilter:
		case DBApi::eListFilterCriteria::RoleFilter:
		case DBApi::eListFilterCriteria::UserTypeFilter:
		case DBApi::eListFilterCriteria::CalTypeFilter:
		case DBApi::eListFilterCriteria::LogTypeFilter:
		{
			if ( compareop == "=" || compareop == "<>" || compareop == "!=" )
			{
				compareOpOk = true;
			}
			break;
		}

		case DBApi::eListFilterCriteria::ItemNameFilter:
		case DBApi::eListFilterCriteria::LabelFilter:
		case DBApi::eListFilterCriteria::LotFilter:
		case DBApi::eListFilterCriteria::QcFilter:
		case DBApi::eListFilterCriteria::InstrumentFilter:
		case DBApi::eListFilterCriteria::CommentsFilter:
		{
			if ( compareop == "=" || compareop == "<>" || compareop == "!=" ||
				 compareop == "similar to" || compareop == "not similar to" ||
				 compareop == "like" || compareop == "not like" ||
				 compareop == "ilike" || compareop == "not ilike" ||		// case-insensitive variants...
				 compareop == "~" || compareop == "!~" ||					// case-sensitive sub-string 'contains' variants...
				 compareop == "~*" || compareop == "!~*" )					// case-insensitive sub-string 'contains' variants...
			{
				compareOpOk = true;
			}
			break;
		}

		case DBApi::eListFilterCriteria::CreationDateFilter:
		case DBApi::eListFilterCriteria::CreationDateRangeFilter:
		case DBApi::eListFilterCriteria::ModifyDateFilter:
		case DBApi::eListFilterCriteria::ModifyDateRangeFilter:
		case DBApi::eListFilterCriteria::RunDateFilter:
		case DBApi::eListFilterCriteria::RunDateRangeFilter:
		case DBApi::eListFilterCriteria::AcquisitionDateFilter:
		case DBApi::eListFilterCriteria::AcquisitionDateRangeFilter:
		{
			if ( compareop == "=" || compareop == "<" || compareop == "<=" ||
				 compareop == ">" || compareop == ">=" || compareop == "<>" || compareop == "!=" ||
				 compareop == "between" )
			{
				compareOpOk = true;
			}
			break;
		}

		case DBApi::eListFilterCriteria::NoFilter:
		{
			compareOpOk = true;
		}

		case DBApi::eListFilterCriteria::FilterNotDefined:
		default:
			break;
	}

	return compareOpOk;
}

 // identify those filters that use single date comparison values for date string formatting
bool DBifImpl::IsSingleDateFilter( eListFilterCriteria filtertype )
{
	bool isDate = false;

	switch ( filtertype )
	{
		case DBApi::eListFilterCriteria::CreationDateFilter:
		case DBApi::eListFilterCriteria::ModifyDateFilter:
		case DBApi::eListFilterCriteria::RunDateFilter:
		case DBApi::eListFilterCriteria::AcquisitionDateFilter:
			isDate = true;
			break;
	}

	return isDate;
}

/*
 * \brief Generate an SQL date filter expression for use in a WHERE clause and append it to an existing string.
 * \param filterstr output variable to append the generated SQL WHERE clause expression.
 * \param fieldtag is the SQL column name on which the data filter is being created.
 * \param compareval string containing the parameters to be used in the expression. Parameters are separated with a
 * semicolon. The parameters can either be in the format 2020-09-07, or '2020-09-07 00:00:00'::timestamp.
 * \return true if successful. false if there was an error, such as an incorrect number of parameters in compareval.
 */
bool DBifImpl::FormatDateRangeFilter( std::string& filterstr, std::string fieldtag, std::string compareval )
{
	bool dateOk = true;
	std::string formatStr = "";

	if ( compareval.length() > 0 )
	{
		std::string sepChars = ";";
		char* pSepChars = (char*) sepChars.c_str();
		std::string sepStr = "";
		std::string trimChars = "";
		std::string parseStr = compareval;
		std::vector<std::string> tokenList = {};
		int32_t tokenCnt = 0;
		std::string logStr = "";

		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, pSepChars, true, trimChars );

		if ( tokenCnt <= 0 || tokenCnt > 2 )
		{
			logStr = "Failure parsing date range.";

#ifdef USE_LOGGER
			Logger::L().Log( MODULENAME, severity_level::debug1, logStr );
#endif
			filterstr.clear();
			return false;
		}

		const auto* DATE_CLAUSE = "\"%s\" between '%s' AND '%s'";
		const auto* TIMESTAMP_CLAUSE = "\"%s\" between %s AND %s";
		auto beginDate = tokenList.at(0);
		formatStr = boost::str( boost::format( beginDate.length() > 10 ? TIMESTAMP_CLAUSE : DATE_CLAUSE) % fieldtag % tokenList.at( 0 ) % tokenList.at( 1 ) );
	}

	if ( dateOk )
	{
		filterstr.append( formatStr );
	}
	else
	{
		filterstr.clear();
		dateOk = false;
	}

	return dateOk;
}

// validate carrier type by enum value and comparison operator (must be "=")
bool DBifImpl::CheckCarrierFilter( std::string& filterstr, std::string filtertag, std::string compareop, std::string compareval )
{
	bool carrierOk = true;

	if ( ( compareop == "=" ) &&
		 ( ( compareval == "1" ) || ( compareval == "2" ) || ( compareval == "3" ) ) )
	{
		filterstr.append( filtertag );
	}
	else
	{
		carrierOk = false;
		filterstr.clear();
	}

	return carrierOk;
}

// identify those filter comparison targets that use case sensitve strings to prevent case modification
bool DBifImpl::DbFilterValueIsString( DBApi::eListFilterCriteria filtertype )
{
	bool strType = false;

	switch ( filtertype )
	{
		case DBApi::eListFilterCriteria::ItemNameFilter:
		case DBApi::eListFilterCriteria::LabelFilter:
		case DBApi::eListFilterCriteria::CreationUserNameFilter:
		case DBApi::eListFilterCriteria::RunUserNameFilter:
		case DBApi::eListFilterCriteria::LotFilter:
		case DBApi::eListFilterCriteria::InstrumentFilter:
		case DBApi::eListFilterCriteria::CommentsFilter:
		{
			strType = true;
			break;
		}
	}

	return strType;
}

