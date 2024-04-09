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



static const std::string MODULENAME = "DBif_Sort";



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


// the following sort criteria are defined for use;
// not all sort modes may be used on every table;
// secondary sorts may use any of the applicable predefined modes excluding the primary sort mode
/*

case DBApi::eListSortCriteria::SortNotDefined:
case DBApi::eListSortCriteria::NoSort:
case DBApi::eListSortCriteria::GuidSort:
case DBApi::eListSortCriteria::IdNumSort:
case DBApi::eListSortCriteria::IndexSort:
case DBApi::eListSortCriteria::OwnerSort:
case DBApi::eListSortCriteria::CarrierSort:
case DBApi::eListSortCriteria::CreationDateSort:
case DBApi::eListSortCriteria::CreationUserSort:
case DBApi::eListSortCriteria::RunDateSort:
case DBApi::eListSortCriteria::UserIdSort:
case DBApi::eListSortCriteria::UserIdNumSort:
case DBApi::eListSortCriteria::UserNameSort:
case DBApi::eListSortCriteria::SampleIdSort:
case DBApi::eListSortCriteria::SampleIdNumSort:
case DBApi::eListSortCriteria::LabelSort:
case DBApi::eListSortCriteria::RxTypeNumSort:
case DBApi::eListSortCriteria::RxLotNumSort:
case DBApi::eListSortCriteria::RxLotExpSort:
case DBApi::eListSortCriteria::ItemNameSort:
case DBApi::eListSortCriteria::CellTypeSort:
case DBApi::eListSortCriteria::InstrumentSort:
case DBApi::eListSortCriteria::QcSort:
case DBApi::eListSortCriteria::CalTypeSort:
case DBApi::eListSortCriteria::LogTypeSort:

*/



////////////////////////////////////////////////////////////////////////////////
// Query sort string construction
////////////////////////////////////////////////////////////////////////////////

void DBifImpl::GetWorklistListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort )
{
	// do not overwrite a suppolied order string
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = WL_IdNumTag;
			break;

		case DBApi::eListSortCriteria::CarrierSort:
			primarySortStr = WL_CarrierTypeTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::CreationUserSort:
					secondarySortStr = WL_CreateUserIdTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = WL_RunUserIdTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = WL_RunDateTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = WL_InstSNTag;
					break;

				case DBApi::eListSortCriteria::NoSort:
				case DBApi::eListSortCriteria::GuidSort:
				case DBApi::eListSortCriteria::IdNumSort:
				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::CarrierSort:
				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::UserIdNumSort:
				case DBApi::eListSortCriteria::UserNameSort:
				case DBApi::eListSortCriteria::SampleIdSort:
				case DBApi::eListSortCriteria::SampleIdNumSort:
				case DBApi::eListSortCriteria::LabelSort:
				case DBApi::eListSortCriteria::RxLotNumSort:
				case DBApi::eListSortCriteria::ItemNameSort:
				case DBApi::eListSortCriteria::CellTypeSort:
				case DBApi::eListSortCriteria::QcSort:
				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationUserSort:
			primarySortStr = WL_CreateUserIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = WL_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CarrierSort:
					secondarySortStr = WL_CarrierTypeTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = WL_RunDateTag;
					break;

				case DBApi::eListSortCriteria::NoSort:
				case DBApi::eListSortCriteria::GuidSort:
				case DBApi::eListSortCriteria::UserIdSort:
				case DBApi::eListSortCriteria::UserNameSort:
				case DBApi::eListSortCriteria::SampleIdSort:
				case DBApi::eListSortCriteria::CreationDateSort:
				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = WL_RunDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::GuidSort:
					secondarySortStr = WL_IdTag;
					break;

				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = WL_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CarrierSort:
					secondarySortStr = WL_CarrierTypeTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = WL_RunUserIdTag;
					break;

				case DBApi::eListSortCriteria::NoSort:
				case DBApi::eListSortCriteria::RunDateSort:
				case DBApi::eListSortCriteria::UserNameSort:
				case DBApi::eListSortCriteria::SampleIdSort:
				case DBApi::eListSortCriteria::CreationDateSort:
				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = WL_RunUserIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::GuidSort:
					secondarySortStr = WL_IdTag;
					break;

				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = WL_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CarrierSort:
					secondarySortStr = WL_CarrierTypeTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = WL_RunDateTag;
					break;

				case DBApi::eListSortCriteria::NoSort:
				case DBApi::eListSortCriteria::UserIdSort:
				case DBApi::eListSortCriteria::UserNameSort:
				case DBApi::eListSortCriteria::SampleIdSort:
				case DBApi::eListSortCriteria::CreationDateSort:
				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::NoSort:
		case DBApi::eListSortCriteria::GuidSort:
		case DBApi::eListSortCriteria::OwnerSort:
		case DBApi::eListSortCriteria::CreationDateSort:
		case DBApi::eListSortCriteria::UserNameSort:
		case DBApi::eListSortCriteria::SampleIdSort:
		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetSampleSetListSortString( std::string& orderstring,
										   DBApi::eListSortCriteria primarysort,
										   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SS_IdNumTag;
			break;

		case DBApi::eListSortCriteria::CarrierSort:
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
			primarySortStr = SS_CreateDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SS_RunDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SS_OwnerIdTag;
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationUserSort:
		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = SS_OwnerIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CarrierSort:
					secondarySortStr = "CarrierType";
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SS_RunDateTag;
					break;
			}
			break;

		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = SS_RunDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CarrierSort:
					secondarySortStr = SS_CarrierTypeTag;
					break;

				case DBApi::eListSortCriteria::CreationUserSort:
				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SS_OwnerIdTag;
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetSampleItemListSortString( std::string& orderstring,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SI_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = SI_SampleSetIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SI_IdNumTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SI_RunDateTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = SI_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = SI_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SI_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = SI_SampleSetIdTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SI_RunDateTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = SI_RunDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SI_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = SI_SampleSetIdTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = SI_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetSampleListSortString( std::string& orderstring,
										DBApi::eListSortCriteria primarysort,
										DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SM_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = SM_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = SM_CellTypeIdxTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SM_LabelTag;
					break;

				case DBApi::eListSortCriteria::QcSort:
					secondarySortStr = SM_QcProcessIdTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SM_AcquireDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SM_OwnerUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CellTypeSort:
			primarySortStr = SM_CellTypeIdxTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SM_NameTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SM_LabelTag;
					break;

				case DBApi::eListSortCriteria::QcSort:
					secondarySortStr = SM_QcProcessIdTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SM_AcquireDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SM_OwnerUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::LabelSort:
			primarySortStr = SM_LabelTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SM_NameTag;
					break;

				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = SM_CellTypeIdxTag;
					break;

				case DBApi::eListSortCriteria::QcSort:
					secondarySortStr = SM_QcProcessIdTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SM_AcquireDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SM_OwnerUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::QcSort:
			primarySortStr = SM_QcProcessIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SM_NameTag;
					break;

				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = SM_CellTypeIdxTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SM_LabelTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SM_AcquireDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SM_OwnerUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
			primarySortStr = SM_AcquireDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SM_NameTag;
					break;

				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = SM_CellTypeIdxTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SM_LabelTag;
					break;

				case DBApi::eListSortCriteria::QcSort:
					secondarySortStr = SM_QcProcessIdTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SM_OwnerUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = SM_OwnerUserIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SM_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SM_NameTag;
					break;

				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = SM_CellTypeIdxTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SM_LabelTag;
					break;

				case DBApi::eListSortCriteria::QcSort:
					secondarySortStr = SM_QcProcessIdTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SM_AcquireDateTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetAnalysisListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = AN_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = AN_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AN_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = AN_AnalysisDateTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = AN_InstSNTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = AN_RunUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = AN_AnalysisDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AN_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = AN_SampleIdTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = AN_InstSNTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = AN_RunUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::InstrumentSort:
			primarySortStr = AN_InstSNTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AN_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = AN_AnalysisDateTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = AN_SampleIdTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = AN_RunUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = AN_RunUserIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AN_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = AN_AnalysisDateTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = AN_InstSNTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = AN_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetSummaryResultListSortString( std::string& orderstring,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = RS_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = RS_AnalysisIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = RS_ResultDateTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = RS_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = RS_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = RS_ResultDateTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = RS_AnalysisIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = RS_ResultDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RS_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = RS_AnalysisIdTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = RS_SampleIdTag;
					break;

				default:
					break;
			}
			break;


		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetDetailedResultListSortString( std::string& orderstring,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = RD_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = RD_OwnerIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RD_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = RD_ResultDateTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = RD_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = RD_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RD_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = RD_ResultDateTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = RD_OwnerIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = RD_ResultDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RD_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = RD_OwnerIdTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = RD_SampleIdTag;
					break;

				default:
					break;
			}
			break;


		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetImageResultListSortString( std::string& orderstring,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = RI_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
		case DBApi::eListSortCriteria::ParentSort:
			primarySortStr = RI_AnalysisIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RI_IdNumTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = RI_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = RI_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RI_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::ParentSort:
					secondarySortStr = RI_AnalysisIdTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}

}

void DBifImpl::GetSResultListSortString( std::string& orderstring,
										 DBApi::eListSortCriteria primarysort,
										 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SR_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = SR_AnalysisIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SR_IdNumTag;
					break;

				case DBApi::eListSortCriteria::SampleIdSort:
					secondarySortStr = SR_SampleIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = SR_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SR_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
					secondarySortStr = SR_AnalysisIdTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetImageSetListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = IC_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
		case DBApi::eListSortCriteria::SampleIdSort:
			primarySortStr = IC_SampleIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = IC_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetImageSequenceListSortString( std::string& orderstring,
											   DBApi::eListSortCriteria primarysort,
											   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = IS_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = IS_SetIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = IS_SequenceNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetImageListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = IM_IdNumTag;
			break;

		case DBApi::eListSortCriteria::OwnerSort:
			primarySortStr = IM_SequenceIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = IM_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetCellTypeListSortString( std::string& orderstring,
										   DBApi::eListSortCriteria primarysort,
										   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = CT_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = CT_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = CT_IdNumTag;
					break;

				case DBApi::eListSortCriteria::IndexSort:
				case DBApi::eListSortCriteria::CellTypeSort:
					secondarySortStr = CT_IdxTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::IndexSort:
		case DBApi::eListSortCriteria::CellTypeSort:
			primarySortStr = CT_IdxTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = CT_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = CT_NameTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetImageAnalysisParamListSortString( std::string& orderstring,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = IAP_IdNumTag;
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetAnalysisInputSettingsListSortString( std::string& orderstring,
															DBApi::eListSortCriteria primarysort,
															DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = AIP_IdNumTag;
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetAnalysisDefinitionListSortString( std::string& orderstring,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = AD_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = AD_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AD_IdNumTag;
					break;

				case DBApi::eListSortCriteria::IndexSort:
					secondarySortStr = AD_IdxTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::IndexSort:
			primarySortStr = AD_IdxTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AD_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = AD_NameTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetAnalysisParamListSortString( std::string& orderstring,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = AP_IdNumTag;
			break;

		case DBApi::eListSortCriteria::LabelSort:
			primarySortStr = AP_LabelTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = AD_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetIlluminatorListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = IL_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = IL_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = IL_IdNumTag;
					break;

				case DBApi::eListSortCriteria::IndexSort:
					secondarySortStr = IL_IdxTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::IndexSort:
			primarySortStr = IL_IdxTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = IL_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = IL_NameTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetUserListSortString( std::string& orderstring,
									  DBApi::eListSortCriteria primarysort,
									  DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = UR_IdNumTag;
			break;


		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = UR_UserNameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = UR_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetRoleListSortString( std::string& orderstring,
									  DBApi::eListSortCriteria primarysort,
									  DBApi::eListSortCriteria secondarysort,
									  DBApi::eRoleClass byrole )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = RO_IdNumTag;
			break;


		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = RO_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RO_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetUserPropertiesListSortString( std::string& orderstring,
												DBApi::eListSortCriteria primarysort,
												DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = UP_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = UP_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = UP_IdNumTag;
					break;

				case DBApi::eListSortCriteria::IndexSort:
					secondarySortStr = UP_IdxTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::IndexSort:
			primarySortStr = UP_IdxTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = UP_IdNumTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = UP_NameTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetSignatureListSortString( std::string& orderstring, DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SG_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = SG_LongSigTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SG_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetReagentInfoListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = RX_IdNumTag;
			break;

		case DBApi::eListSortCriteria::RxTypeNumSort:
			primarySortStr = RX_TypeNumTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RX_IdNumTag;
					break;

				case DBApi::eListSortCriteria::RxLotNumSort:
					secondarySortStr = RX_LotNumTag;
					break;

				case DBApi::eListSortCriteria::RxLotExpSort:
					secondarySortStr = RX_LotExpirationDateTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::RxLotNumSort:
			primarySortStr = RX_LotNumTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RX_IdNumTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::RxLotExpSort:
			primarySortStr = RX_LotExpirationDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = RX_IdNumTag;
					break;

				case DBApi::eListSortCriteria::RxLotNumSort:
					secondarySortStr = RX_LotNumTag;
					break;

				case DBApi::eListSortCriteria::RxTypeNumSort:
					secondarySortStr = RX_TypeNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetWorkflowListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = WF_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = WF_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = WF_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetBioProcessListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = BP_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = BP_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = BP_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetQcProcessListSortString( std::string& orderstring,
										   DBApi::eListSortCriteria primarysort,
										   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = QC_IdNumTag;
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = QC_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = QC_IdNumTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetCalibrationListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = CC_IdNumTag;
			break;

		case DBApi::eListSortCriteria::InstrumentSort:
			primarySortStr = CFG_InstSNTag;
			break;

		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = CC_CalDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = CC_IdNumTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = CC_InstSNTag;
					break;

				case DBApi::eListSortCriteria::CalTypeSort:
					secondarySortStr = CC_CalTypeTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = CC_CalUserIdTag;
					break;

				default:
					break;
			}

		case DBApi::eListSortCriteria::CalTypeSort:
			primarySortStr = CC_CalTypeTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = CC_IdNumTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = CC_InstSNTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = CC_CalDateTag;
					break;

				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = CC_CalUserIdTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = CC_CalUserIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = CC_IdNumTag;
					break;

				case DBApi::eListSortCriteria::InstrumentSort:
					secondarySortStr = CC_InstSNTag;
					break;

				case DBApi::eListSortCriteria::CalTypeSort:
					secondarySortStr = CC_CalTypeTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = CC_CalDateTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetLogEntryListSortString( std::string& orderstring,
										   DBApi::eListSortCriteria primarysort,
										   DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = LOG_IdNumTag;
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
			primarySortStr = LOG_EntryDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = LOG_IdNumTag;
					break;

				case DBApi::eListSortCriteria::LogTypeSort:
					secondarySortStr = LOG_EntryTypeTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::LogTypeSort:
			primarySortStr = LOG_EntryTypeTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = LOG_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = LOG_EntryDateTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

void DBifImpl::GetInstrumentConfigListSortString( std::string& orderstring,
												  DBApi::eListSortCriteria primarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = CFG_IdNumTag;
			break;

		case DBApi::eListSortCriteria::InstrumentSort:
			primarySortStr = CFG_InstSNTag;
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
	}
}

void DBifImpl::GetSchedulerConfigListSortString( std::string& orderstring,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort )
{
	if ( orderstring.length() > 0 ||
		 primarysort == DBApi::eListSortCriteria::SortNotDefined )
	{
		return;
	}

	std::string primarySortStr = "";
	std::string secondarySortStr = "";

	switch ( primarysort )
	{
		case DBApi::eListSortCriteria::IdNumSort:
			primarySortStr = SCH_IdNumTag;
			break;

		case DBApi::eListSortCriteria::CreationDateSort:
			primarySortStr = SCH_CreationDateTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SCH_IdNumTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::CreationUserSort:
				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SCH_OwnerIdTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SCH_LastRunTimeTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SCH_CommentsTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SCH_NameTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::OwnerSort:
		case DBApi::eListSortCriteria::CreationUserSort:
		case DBApi::eListSortCriteria::UserIdSort:
			primarySortStr = SCH_OwnerIdTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SCH_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SCH_CreationDateTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SCH_LastRunTimeTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SCH_CommentsTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SCH_NameTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::RunDateSort:
			primarySortStr = SCH_LastRunTimeTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SCH_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SCH_CreationDateTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::CreationUserSort:
				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SCH_OwnerIdTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SCH_CommentsTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SCH_NameTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::LabelSort:
			primarySortStr = SCH_CommentsTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SCH_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SCH_CreationDateTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SCH_LastRunTimeTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::CreationUserSort:
				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SCH_OwnerIdTag;
					break;

				case DBApi::eListSortCriteria::ItemNameSort:
					secondarySortStr = SCH_NameTag;
					break;

				default:
					break;
			}
			break;

		case DBApi::eListSortCriteria::ItemNameSort:
			primarySortStr = SCH_NameTag;
			switch ( secondarysort )
			{
				case DBApi::eListSortCriteria::IdNumSort:
					secondarySortStr = SCH_IdNumTag;
					break;

				case DBApi::eListSortCriteria::CreationDateSort:
					secondarySortStr = SCH_CreationDateTag;
					break;

				case DBApi::eListSortCriteria::RunDateSort:
					secondarySortStr = SCH_LastRunTimeTag;
					break;

				case DBApi::eListSortCriteria::OwnerSort:
				case DBApi::eListSortCriteria::CreationUserSort:
				case DBApi::eListSortCriteria::UserIdSort:
					secondarySortStr = SCH_OwnerIdTag;
					break;

				case DBApi::eListSortCriteria::LabelSort:
					secondarySortStr = SCH_CommentsTag;
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	if ( primarySortStr.length() > 0 )
	{
		orderstring = boost::str( boost::format( "\"%s\"" ) % primarySortStr );
		if ( secondarySortStr.length() > 0 )
		{
			orderstring.append( boost::str( boost::format( " AND \"%s\"" ) % secondarySortStr ) );
		}
	}
}

// Generic entry point for list sort string generation

void DBifImpl::GetListSortString( DBApi::eListType listtype, std::string& orderstring,
								  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort, int32_t sortdir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::string orderStr = "";
	std::string defaultOrderStr = "";

	// never overwrite an existing order string... otherwise always go through these to get the default if not the specific sort order tag
	if ( orderstring.length() == 0 )
	{
		std::string defaultOrderTagStr = "";

		switch ( listtype )
		{
			case DBApi::eListType::WorklistList:
				GetWorklistListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = WL_IdNumTag;			// "WorklistIdNum"
				break;

			case DBApi::eListType::SampleSetList:
				GetSampleSetListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SS_IdNumTag;			// "SampleSetIdNum"
				break;

			case DBApi::eListType::SampleItemList:
				GetSampleItemListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SI_IdNumTag;			// "SampleItemIdNum"
				break;

			case DBApi::eListType::SampleList:
				GetSampleListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SM_IdNumTag;			// "SampleIdNum"
				break;

			case DBApi::eListType::AnalysisList:
				GetAnalysisListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = AN_IdNumTag;			// "AnalysisIdNum"
				break;

			case DBApi::eListType::SummaryResultList:
				GetSummaryResultListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = RS_IdNumTag;			// "SummaryResultIdNum"
				break;

			case DBApi::eListType::DetailedResultList:
				GetDetailedResultListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = RD_IdNumTag;			// "DetailedResultIdNum"
				break;

			case DBApi::eListType::ImageResultList:
				GetImageResultListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = RI_IdNumTag;			// "ImageResultIdNum"
				break;

			case DBApi::eListType::SResultList:
				GetSResultListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SR_IdNumTag;			// "SResultIdNum"
				break;

			case DBApi::eListType::ImageSetList:
				GetImageSetListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = IC_IdNumTag;			// "ImageSetIdNum"
				break;

			case DBApi::eListType::ImageSequenceList:
				GetImageSequenceListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = IS_IdNumTag;			// "ImageSequenceIdNum"
				break;

			case DBApi::eListType::ImageList:
				GetImageListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = IM_IdNumTag;			// "ImageIdNum"
				break;

			case DBApi::eListType::CellTypeList:
				GetCellTypeListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = CT_IdNumTag;			// "CellTypeIdNum"
				break;

			case DBApi::eListType::ImageAnalysisParamList:
				GetImageAnalysisParamListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = IAP_IdNumTag;			// "ImageAnalysisParamIdNum"
				break;

			case DBApi::eListType::AnalysisDefinitionList:
				GetAnalysisDefinitionListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = AD_IdNumTag;			// "AnalysisDefinitionIdNum"
				break;

			case DBApi::eListType::AnalysisParamList:
				GetAnalysisParamListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = AP_IdNumTag;			// "AnalysisParamIdNum"
				break;

			case DBApi::eListType::IlluminatorList:
				GetIlluminatorListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = IL_IdNumTag;			// "IlluminatorIdNum"
				break;

			case DBApi::eListType::UserList:
				GetUserListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = UR_IdNumTag;			// "UserIdNum"
				break;

			case DBApi::eListType::UserPropertiesList:
				GetUserPropertiesListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = UP_IdNumTag;			// "PropertyIdNum"
				break;

			case DBApi::eListType::RolesList:
				GetRoleListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = RO_IdNumTag;			// "RoleIdNum"
				break;

			case DBApi::eListType::SignatureDefList:
				GetSignatureListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SG_IdNumTag;			// "SignatureDefIdNum"
				break;

			case DBApi::eListType::ReagentInfoList:
				GetReagentInfoListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = RX_IdNumTag;			// "ReagentIdNum"
				break;

			case DBApi::eListType::WorkflowList:
				GetWorkflowListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = WF_IdNumTag;			// "WorkflowIdNum"
				break;

			case DBApi::eListType::BioProcessList:
				GetBioProcessListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = BP_IdNumTag;			// "BioProcessIdNum"
				break;

			case DBApi::eListType::QcProcessList:
				GetQcProcessListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = QC_IdNumTag;			// "QcProcessIdNum"
				break;

			case DBApi::eListType::CalibrationsList:
				GetCalibrationListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = CC_IdNumTag;
				break;

			case DBApi::eListType::InstrumentConfigList:
				GetInstrumentConfigListSortString( orderStr, primarysort );
				defaultOrderTagStr = CFG_InstSNTag;			// really no other value to use...
				break;

			case DBApi::eListType::LogEntryList:
				GetLogEntryListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = LOG_EntryDateTag;		// "EntryDate"
				break;

			case DBApi::eListType::SchedulerConfigList:
				GetSchedulerConfigListSortString( orderStr, primarysort, secondarysort );
				defaultOrderTagStr = SCH_CreationDateTag;	// "CreationDate"
				break;

			case DBApi::eListType::CellHealthReagentsList:
				GetCalibrationListSortString(orderStr, primarysort, secondarysort);
				defaultOrderTagStr = CH_IdNumTag;
				break;

			case DBApi::eListType::NoListType:
			default:
				break;
		}

		defaultOrderStr = boost::str( boost::format( "\"%s\"" ) % defaultOrderTagStr );
		if ( orderStr.length() == 0 &&								// didn't generate a sort type string
			 primarysort != DBApi::eListSortCriteria::NoSort )		// specifically requesting no sort
		{
			primarysort = DBApi::eListSortCriteria::SortNotDefined;
		}
	}

	if ( ( orderstring.length() > 0 ) || ( orderStr.length() > 0 ) || ( primarysort == DBApi::eListSortCriteria::SortNotDefined ) )
	{
		std::string queryOrderStr = " ORDER BY ";

		// pre-pend any passed-in order string
		if ( orderstring.length() > 0 )
		{
			queryOrderStr.append( orderstring );
			queryOrderStr.append( " AND " );
		}

		if ( orderStr.length() > 0 )
		{
			queryOrderStr.append( orderStr );
		}
		else if ( primarysort == DBApi::eListSortCriteria::SortNotDefined )
		{
			queryOrderStr.append( defaultOrderStr );
		}

		if ( ( sortdir >= 0 ) ||				// don't care sort direction or increasing numerical order or oldest first
			 ( primarysort == DBApi::eListSortCriteria::SortNotDefined ) )	// don't care sort type
		{
			queryOrderStr.append( " ASC" );
		}
		else
		{
			queryOrderStr.append( " DESC" );
		}

		orderstring = queryOrderStr;
	}
}

