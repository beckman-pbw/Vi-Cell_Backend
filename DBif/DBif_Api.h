// DBif_Api.h : header file for use by external library clients of the DBif library
//

#pragma once


#include "DBif_QueryEnum.hpp"
#include "DBif_Structs.hpp"


#ifndef HAWKEYEDBIF_API
#define HAWKEYEDBIF_API __declspec(dllimport)
#endif



namespace DBApi
{

	extern "C"
	{
		HAWKEYEDBIF_API bool                  DBifInit( void );
		HAWKEYEDBIF_API bool                  DbGetConnectionProperties( std::string& dbaddr, std::string& dbport,
																		 std::string& dbname, std::string& dbdriver );
		HAWKEYEDBIF_API bool                  DbSetDefaultConnection( void );
		HAWKEYEDBIF_API bool                  DbSetFullConnection( std::string dbaddr = "", std::string dbport = "",
																   std::string dbname = "", std::string dbdriver = "" );
		HAWKEYEDBIF_API bool                  DbSetConnectionAddrByString( std::string dbaddr = "", std::string dbport = "" );
		HAWKEYEDBIF_API bool                  DbSetConnectionAddr( std::string dbaddr = "", int32_t dbport = 5432 );
		HAWKEYEDBIF_API bool                  DbSetDbName( std::string dbname );
		HAWKEYEDBIF_API bool                  DbClearDb( std::string dbname = "" );

		HAWKEYEDBIF_API bool                  DbSetBackupUserPwd( DBApi::eQueryResult & resultcode, std::string & pwdstr );
		HAWKEYEDBIF_API bool                  DbTruncateTableContents(DBApi::eQueryResult& resultcode, std::list<std::pair<std::string, std::string>> tableNames_name_schema);

		HAWKEYEDBIF_API bool                  IsDBPresent( void );													// checks for connection to database and returns the connection user-type
		HAWKEYEDBIF_API bool                  DbIsLoginType( DBApi::eLoginType usertype );							// checks for connection to database as the supplied user-type
		HAWKEYEDBIF_API DBApi::eLoginType     DbGetLoginType( void );
		HAWKEYEDBIF_API DBApi::eLoginType     DbSetLoginType( DBApi::eLoginType usertype );							// selects the connection type to use
		HAWKEYEDBIF_API DBApi::eLoginType     DbInstrumentLogin( void );											// connect to database as standard user
		HAWKEYEDBIF_API DBApi::eLoginType     DbUserLogin( std::string unamestr = "",
														 std::string hashstr = "", std::string pwdstr = "" );		// connect to database as standard user using specified user account
		HAWKEYEDBIF_API DBApi::eLoginType     DbAdminLogin( std::string unamestr = "",
														  std::string hashstr = "", std::string pwdstr = "" );		// connect to database as an Admin user
		HAWKEYEDBIF_API void                  DbLogoutUserType( DBApi::eLoginType usertype );						// disconnect from database for specified user login type
		HAWKEYEDBIF_API void                  DbDisconnect( void );													// disconnect from database for all user logins


		HAWKEYEDBIF_API void                  GetDbCurrentTimeString( std::string& time_string );
		HAWKEYEDBIF_API void                  GetDbCurrentTimePtAndString( std::string& time_string, system_TP& time_pt );
		HAWKEYEDBIF_API void                  GetDbTimeString( system_TP time_pt, std::string& time_string, bool isutctp = false );
		HAWKEYEDBIF_API bool                  GetTimePtFromDbTimeString( system_TP& time_pt, std::string time_string );
		HAWKEYEDBIF_API void                  GetQueryResultString( DBApi::eQueryResult resultcode, std::string& resultstring );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbExecuteQuery( std::string query_str,								// execute query under the automatically determined connection type
															  std::vector<std::string>& resultlist );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbExecuteQueryType( DBApi::eLoginType usertype,						// execute query under the selected connection type
																  std::string query_str,
																  std::vector<std::string>& resultlist );

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// List retrieval parameter use:
		//
		// standard filter types and sort-order types are listed in DBif_QueryEnum.h
		//
		// Note that in the 'enhanced' method variants, the vectors for the three filter, comparison, and sort criteria must contain the same number of elements;
		// this may require passing placeholders for the comparison value,  e.g. " " or use "=" for date RANGE filters.
		//
		// filtertype or filtertypes:	enumeration of generic filter-type designators; not all tables support all filter types; the filtertypes variable is a vector
		//								in the 'enhanced' method variants allowing multi-parameter filters
		// compareop or compareops:		operation strings may be '=', '>', and '<', and combinations (as <=, >=), the "<>" and "!=" values for not equal,
		//								the 'between' operator, the '~' and '!~' operators for case-sensitive contained sub-string comparisons,
		//								and the '~*' and '!~*' operators for case-insensitive contained sub-string comparisons;
		//								The compareops variable is a vector in the 'enhanced' method variants allowing multi-parameter filters;
		//								NOTE: if specifying a date range filter, the contents of this parameter are ignored, but a polaceholder is still required!;
		// compareval or comparevals:	string contains the specific value, date, or condition tag; the comparevals variable is is a vector in the 'enhanced' method
		//								variants allowing multi-parameter filters;
		//								expected pre-defined tags are 'true' and 'false' (allowed with the '=', '!=', or '<>' operators only);
		//								when filtering by carrier-type, the enumeration integer value should be supplied;
		//								the 'true' and 'false' values and standard numeric values DO NOT need to be quoted (single or double);
		//								text values (including UUID strings) need to be supplied in single-quoted format; however, date-time strings supplied as
		//								filter parameters DO NOT require quoting either for single date specifiers or date range specifiers,
		//								and should be supplied as "2020-May-01;2020-June-01" NOT "'2020-May-01;2020-June-01'";
		//								date strings should be formatted as "May 10, 2020" or "2020-May-05" using the Alpha month designator and 4-digit year to
		//								avoid inconsistent interpretation or format ambiguity where necessary; Date range specifiers may also
		//								contain a time field to further limit search scope; (e.g. '2020-May-01 12:00:00;2020-May-01 18:00:00')
		//								NOTE: if specifying a date range filter, BOTH date values must be specified as a single string in the 'compareval' or
		//								'comparevals' string parameter, with the dates separated by a semicolon;
		//										e.g. "May 8,2020;May 10, 2020" or "2020-May-01 12:00:00;2020-May-01 18:00:00";
		//								NOTE: when specifying a date rage filter, the DB interface automatically supplies the 'between' operator, but a 'dummy'
		//								comparison operator must be supplied to keep the operand vectors aligned;
		// primarysort:					enumeration value for the field-type to be used for sorting (converted to table-specific field tag (oor none)
		// secondarysort:				enumeration for secondary sort order field type
		// orderstring:					client defined order statement; overrides constructed order statement;
		// sortdir:						integer value indicating sort direction where > 0 is forward (ASC), < 0 is reverse sort (DESC), and 0 is default order;
		// limitcnt:					maximum number of records to retrieve in a single query operation;
		//								if limitcnt is unspecified (limitcnt = DEFAULT_QUERY_NO_LIST_LIMIT (-1) or limitcnt = DEFAULT_QUERY_AND_LIST_LIMIT (0)),
		//								the default limit is used; current default is 1000 records (and their contained child objects); if not using the default limit
		//								shortcut values, the maximum specifiable limit is 2500; For special case queries, the limit may be bypasseed completely using the
		//								special value NO_QUERY_OR_LIST_LIMIT (-2), which will enable retrieval and return of ALL records otherwise matching the query;
		//								queries will always return a complete object with all contained sub-objects, except worklists, which return only the first-level
		//								contained SampleSet records without their contained SampleItem records;
		//								NOTE that an exception to the limit logic is applied for log entry list retrieval, which will use NO_QUERY_OR_LIST_LIMIT (-2)
		//								to indicate no limit subject to any given filter criteria.
		// startindex:					starting index value for query; used to retrieve successive pages; currently not supported/implemented
		// startidnum:					starting idnum value for query; used to retrieve successive pages; note that '-1' or '0' indicates either the
		//								'start at beginning/start at end' condition, depending on the specified sort order.
		//
		//	e.g.	(single param methods)									(multi-param vector methods)
		//			filtertype:		eStatusfilter					OR		filtertypes:	eStatusFilter,eCreationDateRange
		//			compareop:	'='									OR		compareops:		"=","=" ( dummy parameter for date ranges )
		//			compareval:		SampleSetComplete -> '7'		OR		comparevals:	SampleSetComplete,'2020 05 25;2020 07 01'
		//
		//			primaarysort:	OwnerSort
		//			secondarysort:	IdNumSort (insertion order)
		//			orderstr:		""
		//			sortdir:		0
		//			limitcnt:		50
		//			startindex:		D/C
		//			startidnum:		0
		//
		//	results in a query where-statement SIMILAR to (for multi-parameter mode):
		//
		//	select * from "ViCellInstrument"."SampleSets" where "SampleSetStatus" = '7' AND "CreateDate" between '2020 05 25' '2020 07 01' ORDER BY "SampleSetIdNum" ASC LIMIT '50'
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Record removal methods:
		//
		// Record removal methods will remove ONLY the specific record (or list of records) requested.  NO DATA TREE TRAVERSAL WILL OCCUR with ONE exception:
		//
		//		Removal of a Sample record will remove all Image records associated with that Sample record, thus preventing any further reanalysis.
		//		The deleted sub-objects are:
		//			-The single top-level ImageSet record describing the images, sequencing and image types (Brightfield, Fluorescence channel)
		//			- All the image sequence records taken when the sample was initially acquired, and remaining in the image set
		//			-All the image records describing the individual images that make up an image sequence record.
		//
		// All consistency maintenance and other data curation actions are left to the backend DLL logic.
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetWorklistListEnhanced( std::vector<DB_WorklistRecord>& wlist,
																		 std::vector<DBApi::eListFilterCriteria> filtertypes,
																		 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
																		 int32_t sortdir = 0, std::string orderstring = "",
																		 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetWorklistList( std::vector<DB_WorklistRecord>& wlist,
																 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																 std::string compareop = "", std::string compareval = "",
																 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
																 int32_t sortdir = 0, std::string orderstring = "",
																 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindWorklist( DB_WorklistRecord& wlr, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddWorklist( DB_WorklistRecord& wlr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSaveWorklistTemplate( DB_WorklistRecord& wlr );		// this method should be used to ensure all status indicator fields are set to the appropriate 'template' status
																									// for template saves:
																									//    sets the value of the '.WorklistStatus' variable in the worklist record to DBApi::eWorklistStatus::WorklistTemplate
																									//    sets the value of the '.SamplesSeStatus' variables in all contained SampleSet records to DBApi::eSampleSetStatus::SampleSetTemplate
																									//    sets the value of the '.SampleItemStatus' variable in all contained SampleItem records to DBApi::eSampleItemStatus::SampleItemTemplate
																									//
																									// Items saved as templates should not have a UUID assigned until they are copied and used in actual worklist runs
																									//
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddWorklistSampleSetByUuid( uuid__t samplesetid, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklistSampleSetByUuid( uuid__t samplesetid, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklistSampleSets( std::vector<uuid__t>& idlist, uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyWorklist( DB_WorklistRecord& wlr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyWorklistTemplate( DB_WorklistRecord& wlr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetWorklistStatus( uuid__t worklistid, DBApi::eWorklistStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesettid, DBApi::eSampleSetStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklist( DB_WorklistRecord& wlr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklistByUuid( uuid__t worklistid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveWorklists( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleSetListEnhanced( std::vector<DB_SampleSetRecord>& samplelist,
																		  std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
																		  int32_t sortdir = 0, std::string orderstring = "",
																		  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleSetList( std::vector<DB_SampleSetRecord>& samplelist,
																  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																  std::string compareop = "", std::string compareval = "",
																  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																  DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
																  int32_t sortdir = 0, std::string orderstring = "",
																  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult	  DbGetSampleSetListByUuidList( std::vector<DB_SampleSetRecord>& sslist, std::vector<uuid__t> ssidlist,
																			int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
																			int32_t sortdir = 0, std::string orderstring = "",
																			int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleSetListForWorklist( std::vector<DB_SampleSetRecord>& samplesetlist, DB_WorklistRecord& wlr,
																			 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 int32_t sortdir = 0, std::string orderstring = "",
																			 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleSetListForWorklistId( std::vector<DB_SampleSetRecord>& samplesetlist, uuid__t wlid,
																			   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   int32_t sortdir = 0, std::string orderstring = "",
																			   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSampleSet( DB_SampleSetRecord& ssr, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSampleSet( DB_SampleSetRecord& ssr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSaveSampleSetTemplate( DB_SampleSetRecord& ssr );	// this method should be used to ensure all status indicator fields are set to the appropriate 'template' status
																									// for template saves:
																									//    sets the value of the '.SamplesSeStatus' variable in the SampleSet records to DBApi::eSampleSetStatus::SampleSetTemplate
																									//    sets the value of the '.SampleItemStatus' variable in all contained SampleItem records to DBApi::eSampleItemStatus::SampleItemTemplate
																									//
																									// Items saved as templates should not have a UUID assigned until they are copied and used in actual worklist runs
																									//
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSampleSetSample( DB_SampleItemRecord& ssir, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSampleSetSampleByUuid( uuid__t ssitemid, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSetSample( DB_SampleItemRecord& ssir, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSetSampleByUuid( uuid__t ssitemid, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSetSamples( std::vector<uuid__t>& idlist, uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySampleSet( DB_SampleSetRecord& ssr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySampleSetTemplate( DB_SampleSetRecord& ssr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetSampleSetSampleItemStatus( uuid__t samplesettid, uuid__t itemid, DBApi::eSampleItemStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSet( DB_SampleSetRecord& ssr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSetByUuid( uuid__t samplesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleSets( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleItemListEnhanced( std::vector<DB_SampleItemRecord>& ssirlist,
																		   std::vector<DBApi::eListFilterCriteria> filtertypes,
																		   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   int32_t sortdir = 0, std::string orderstring = "",
																		   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
																   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																   std::string compareop = "", std::string compareval = "",
																   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																   int32_t sortdir = 0, std::string orderstring = "",
																   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult	  DbGetSampleItemListByUuidList( std::vector<DB_SampleSetRecord>& sslist, std::vector<uuid__t> ssidlist,
																			 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 int32_t sortdir = 0, std::string orderstring = "",
																			 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleItemListForSampleSet( std::vector<DB_SampleItemRecord>& ssirlist, DB_SampleSetRecord& ssr,
																			   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   int32_t sortdir = 0, std::string orderstring = "",
																			   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleItemListForSampleSetUuid( std::vector<DB_SampleItemRecord>& ssirlist, uuid__t ssid,
																				   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				   int32_t sortdir = 0, std::string orderstring = "",
																				   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSampleItem( DB_SampleItemRecord& ssir, uuid__t itemid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSampleItem( DB_SampleItemRecord& ssir );			// for template saves:
																									//    set the value of the '.SampleItemStatus' variable in the SampleItem records to DBApi::eSampleItemStatus::SampleItemTemplate
																									//
																									// Items saved as templates should not have a UUID assigned until they are copied and used in actual worklist runs
																									//
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySampleItem( DB_SampleItemRecord& ssir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleItem( DB_SampleItemRecord& ssir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleItemByUuid( uuid__t ssiitemid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleItems( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleListEnhanced( std::vector<DB_SampleRecord>& samplelist,
																	   std::vector<DBApi::eListFilterCriteria> filtertypes,
																	   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   int32_t sortdir = 0, std::string orderstring = "",
																	   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleList( std::vector<DB_SampleRecord>& samplelist,
															   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
															   std::string compareop = "", std::string compareval = "",
															   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															   int32_t sortdir = 0, std::string orderstring = "",
															   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSample( DB_SampleRecord& sr, uuid__t sampleid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSample( DB_SampleRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySample( DB_SampleRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbSetSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSample( DB_SampleRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSampleByUuid( uuid__t sampleid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSamples( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysesListEnhanced( std::vector<DB_AnalysisRecord>& anlist,
																		 std::vector<DBApi::eListFilterCriteria> filtertypes,
																		 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 int32_t sortdir = 0, std::string orderstring = "",
																		 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysesList( std::vector<DB_AnalysisRecord>& anlist,
																 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																 std::string compareop = "", std::string compareval = "",
																 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																 int32_t sortdir = 0, std::string orderstring = "",
																 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindAnalysis( DB_AnalysisRecord& ar, uuid__t analysisid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddAnalysis( DB_AnalysisRecord& ar );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyAnalysis( DB_AnalysisRecord& ar );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysis( DB_AnalysisRecord& ar );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisByUuid( uuid__t analysisid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisBySampleId( uuid__t sampleid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisByUuidList( std::vector<uuid__t> idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSummaryResultsListEnhanced( std::vector<DB_SummaryResultRecord>& srlist,
																			   std::vector<DBApi::eListFilterCriteria> filtertypes,
																			   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   int32_t sortdir = 0, std::string orderstring = "",
																			   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSummaryResultsList( std::vector<DB_SummaryResultRecord>& srlist,
																	   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	   std::string compareop = "", std::string compareval = "",
																	   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   int32_t sortdir = 0, std::string orderstring = "",
																	   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSummaryResult( DB_SummaryResultRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySummaryResult( DB_SummaryResultRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSummaryResult( DB_SummaryResultRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSummaryResultByUuid( uuid__t resultid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetDetailedResultsListEnhanced( std::vector<DB_DetailedResultRecord>& drlist,
																				std::vector<DBApi::eListFilterCriteria> filtertypes,
																				std::vector<std::string> compareops, std::vector<std::string> comparevals,
																				int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				int32_t sortdir = 0, std::string orderstring = "",
																				int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetDetailedResultsList( std::vector<DB_DetailedResultRecord>& drlist,
																		DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																		std::string compareop = "", std::string compareval = "",
																		int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		int32_t sortdir = 0, std::string orderstring = "",
																		int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindDetailedResult( DB_DetailedResultRecord& dr, uuid__t resultid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddDetailedResult( DB_DetailedResultRecord& dr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveDetailedResult( DB_DetailedResultRecord& dr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveDetailedResultByUuid( uuid__t resultid );

		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageResultsListEnhanced( std::vector<DB_ImageResultRecord>& irlist,
																			 std::vector<DBApi::eListFilterCriteria> filtertypes,
																			 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 int32_t sortdir = 0, std::string orderstring = "",
																			 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageResultsList( std::vector<DB_ImageResultRecord>& irlist,
																	 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	 std::string compareop = "", std::string compareval = "",
																	 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 int32_t sortdir = 0, std::string orderstring = "",
																	 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageResultsListByUuidList( std::vector<DB_ImageResultRecord>& irlist, std::vector<uuid__t> iridlist,
																			   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   int32_t sortdir = 0, std::string orderstring = "",
																			   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImageResult( DB_ImageResultRecord& br, uuid__t resultid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImageResult( DB_ImageResultRecord& br );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImageResultByIdNum( DB_ImageResultRecord& br, int64_t idnum );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImageResult( DB_ImageResultRecord& br );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageResult( DB_ImageResultRecord& br );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageResultByUuid( uuid__t resultid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSResultListEnhanced( std::vector<DB_SResultRecord>& srlist,
																		std::vector<DBApi::eListFilterCriteria> filtertypes,
																		std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		int32_t sortdir = 0, std::string orderstring = "",
																		int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSResultList( std::vector<DB_SResultRecord>& srlist,
																DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																std::string compareop = "", std::string compareval = "",
																int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																int32_t sortdir = 0, std::string orderstring = "",
																int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSResult( DB_SResultRecord& sr, uuid__t resultid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSResult( DB_SResultRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSResult( DB_SResultRecord& sr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSResultByUuid( uuid__t resultid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageSetsListEnhanced( std::vector<DB_ImageSetRecord>& islist,
																		  std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  int32_t sortdir = 0, std::string orderstring = "",
																		  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageSetsList( std::vector<DB_ImageSetRecord>& islist,
																  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																  std::string compareop = "", std::string compareval = "",
																  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																  int32_t sortdir = 0, std::string orderstring = "",
																  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImageSet( DB_ImageSetRecord& isr, uuid__t imagesetid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImageSet( DB_ImageSetRecord& isr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyImageSet( DB_ImageSetRecord& isr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSet( DB_ImageSetRecord& isr );          // also removes all image records and image reference objects belonging to the set
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSetByUuid( uuid__t imagesetid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageSequenceListEnhanced( std::vector<DB_ImageSeqRecord>& isrlist,
																			  std::vector<DBApi::eListFilterCriteria> filtertypes,
																			  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			  int32_t sortdir = 0, std::string orderstring = "",
																			  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
																	  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	  std::string compareop = "", std::string compareval = "",
																	  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	  int32_t sortdir = 0, std::string orderstring = "",
																	  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImageSequence( DB_ImageSeqRecord& isr, uuid__t imageid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImageSequence( DB_ImageSeqRecord& isr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyImageSequence( DB_ImageSeqRecord& isr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSequence( DB_ImageSeqRecord& isr );       // also removes all image reference objects belonging to the record
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSequenceByUuid( uuid__t imageseqid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSequences( std::vector<DB_ImageSeqRecord>& idlist );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageSequencesByUuidList( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImagesListEnhanced( std::vector<DB_ImageRecord>& irlist,
																	   std::vector<DBApi::eListFilterCriteria> filtertypes,
																	   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   int32_t sortdir = 0, std::string orderstring = "",
																	   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImagesList( std::vector<DB_ImageRecord>& irlist,
															   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
															   std::string compareop = "", std::string compareval = "",
															   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															   int32_t sortdir = 0, std::string orderstring = "",
															   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImage( DB_ImageRecord& ir, uuid__t imageid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImage( DB_ImageRecord& ir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyImage( DB_ImageRecord& ir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImage( DB_ImageRecord& ir );                 // removes the entire image reference record; should cascade up to image sets and image records
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageByUuid( uuid__t imageid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImages( std::vector<DB_ImageRecord>& idlist );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImagesByUuidList( std::vector<uuid__t>& idlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetCellTypeListEnhanced( std::vector<DB_CellTypeRecord>& ctlist,
																		 std::vector<DBApi::eListFilterCriteria> filtertypes,
																		 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		 int32_t sortdir = 0, std::string orderstring = "",
																		 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetCellTypeList( std::vector<DB_CellTypeRecord>& ctlist,
																 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																 std::string compareop = "", std::string compareval = "",
																 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																 int32_t sortdir = 0, std::string orderstring = "",
																 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindCellTypeByUuid( DB_CellTypeRecord& ctr, uuid__t celltypeid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindCellTypeByIndex( DB_CellTypeRecord& ctr, uint32_t celltypeindex );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddCellType( DB_CellTypeRecord& ctr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyCellType( DB_CellTypeRecord& ctr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveCellType( DB_CellTypeRecord& ctr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveCellTypeByUuid( uuid__t celltypeid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageAnalysisParameterListEnhanced( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																						std::vector<DBApi::eListFilterCriteria> filtertypes,
																						std::vector<std::string> compareops, std::vector<std::string> comparevals,
																						int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																						DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																						DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																						int32_t sortdir = 0, std::string orderstring = "",
																						int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetImageAnalysisParameterList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																				DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																				std::string compareop = "", std::string compareval = "",
																				int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				int32_t sortdir = 0, std::string orderstring = "",
																				int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindImageAnalysisParameter( DB_ImageAnalysisParamRecord& params, uuid__t paramid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveImageAnalysisParameterByUuid( uuid__t paramid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysisParameterListEnhanced( std::vector<DB_AnalysisParamRecord>& aplist,
																				   std::vector<DBApi::eListFilterCriteria> filtertypes,
																				   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																				   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				   int32_t sortdir = 0, std::string orderstring = "",
																				   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysisParameterList( std::vector<DB_AnalysisParamRecord>& aplist,
																		   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																		   std::string compareop = "", std::string compareval = "",
																		   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   int32_t sortdir = 0, std::string orderstring = "",
																		   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindAnalysisParameter( DB_AnalysisParamRecord& params, uuid__t paramid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddAnalysisParameter( DB_AnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyAnalysisParameter( DB_AnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisParameter( DB_AnalysisParamRecord& params );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisParameterByUuid( uuid__t paramid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysisDefinitionsListEnhanced( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																					std::vector<DBApi::eListFilterCriteria> filtertypes,
																					std::vector<std::string> compareops, std::vector<std::string> comparevals,
																					int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																					DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																					DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																					int32_t sortdir = 0, std::string orderstring = "",
																					int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAnalysisDefinitionsList( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																			std::string compareop = "", std::string compareval = "",
																			int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			int32_t sortdir = 0, std::string orderstring = "",
																			int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindAnalysisDefinitionByUuid( DB_AnalysisDefinitionRecord& def, uuid__t defid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindAnalysisDefinitionByIndex( DB_AnalysisDefinitionRecord& def, int32_t defindex = INVALID_INDEX );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveAnalysisDefinitionByUuid( uuid__t defid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetIlluminatorsListEnhanced( std::vector<DB_IlluminatorRecord>& irlist,
																			 std::vector<DBApi::eListFilterCriteria> filtertypes,
																			 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			 int32_t sortdir = 0, std::string orderstring = "",
																			 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetIlluminatorsList( std::vector<DB_IlluminatorRecord>& irlist,
																	 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	 std::string compareop = "", std::string compareval = "",
																	 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 int32_t sortdir = 0, std::string orderstring = "",
																	 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindIlluminatorsByIndex( DB_IlluminatorRecord& ilr, int16_t ilindex = INVALID_INDEX );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindIlluminatorsByName( DB_IlluminatorRecord& ilr, std::string ilname = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindIlluminatorsByWavelength( DB_IlluminatorRecord& ilr, int16_t emission = 0, int16_t illuminator = 0 );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddIlluminator( DB_IlluminatorRecord& ilr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyIlluminator( DB_IlluminatorRecord& ilr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveIlluminator( DB_IlluminatorRecord& ilr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveIlluminatorByIndex( int16_t ilindex );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveIlluminatorByName( std::string ilname = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserListEnhanced( std::vector<DB_UserRecord>& urlist,
																	 std::vector<DBApi::eListFilterCriteria> filtertypes,
																	 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	 int32_t sortdir = 0, std::string orderstring = "",
																	 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserList( std::vector<DB_UserRecord>& urlist,
															 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
															 std::string compareop = "", std::string compareval = "",
															 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															 int32_t sortdir = 0, std::string orderstring = "",
															 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserByUuid( DB_UserRecord& ur, uuid__t userid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserByName( DB_UserRecord& ur, std::string username = "", DBApi::eUserType user_type = DBApi::eUserType::AllUsers );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddUser( DB_UserRecord& ur );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyUser( DB_UserRecord& ur );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUser( DB_UserRecord& ur );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserByUuid( uuid__t userid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserByName( std::string username = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetRolesListEnhanced( std::vector<DB_UserRoleRecord>& rrlist,
																	  std::vector<DBApi::eListFilterCriteria> filtertypes,
																	  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	  int32_t sortdir = 0, std::string orderstring = "",
																	  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetRolesList( std::vector<DB_UserRoleRecord>& rrlist,
															  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
															  std::string compareop = "", std::string compareval = "",
															  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															  int32_t sortdir = 0, std::string orderstring = "",
															  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserRoleByUuid( DB_UserRoleRecord& rr, uuid__t roleid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserRoleByName( DB_UserRoleRecord& rr, std::string rolename );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddUserRole( DB_UserRoleRecord& rr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyUserRole( DB_UserRoleRecord& rr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserRole( DB_UserRoleRecord& rr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserRoleByIdByUuid( uuid__t roleid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserRoleByName( std::string rolename );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserPropertiesListEnhanced( std::vector<DB_UserPropertiesRecord>& uplist,
																			   std::vector<DBApi::eListFilterCriteria> filtertypes,
																			   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			   int32_t sortdir = 0, std::string orderstring = "",
																			   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
																	   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	   std::string compareop = "", std::string compareval = "",
																	   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	   int32_t sortdir = 0, std::string orderstring = "",
																	   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserPropertiesListForUserUuid( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t userid,
																				  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				  int32_t sortdir = 0, std::string orderstring = "",
																				  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetUserPropertiesListForUserName( std::vector<DB_UserPropertiesRecord>& uplist, std::string username,
																				  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				  int32_t sortdir = 0, std::string orderstring = "",
																				  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserPropertyByIndex( DB_UserPropertiesRecord& up, int16_t userindex );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindUserPropertyByName( DB_UserPropertiesRecord& up, std::string username = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddUserProperty( DB_UserPropertiesRecord& up );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserProperty( DB_UserPropertiesRecord& up );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserPropertyByIndex( int16_t propindex );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveUserPropertyByName( std::string propname = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSignatureListEnhanced( std::vector<DB_SignatureRecord>& siglist,
																		  std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  int32_t sortdir = 0, std::string orderstring = "",
																		  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSignatureList( std::vector<DB_SignatureRecord>& siglist,
																  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																  std::string compareop = "", std::string compareval = "",
																  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																  int32_t sortdir = 0, std::string orderstring = "",
																  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSignatureByUuid( DB_SignatureRecord& up, uuid__t sigid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSignature( DB_SignatureRecord& up );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSignature( DB_SignatureRecord& up );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSignatureByUuid( uuid__t sigid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetReagentInfoListEnhanced( std::vector<DB_ReagentTypeRecord>& rxlist,
																			std::vector<DBApi::eListFilterCriteria> filtertypes,
																			std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			int32_t sortdir = 0, std::string orderstring = "",
																			int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetReagentInfoList( std::vector<DB_ReagentTypeRecord>& rxlist,
																	DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	std::string compareop = "", std::string compareval = "",
																	int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	int32_t sortdir = 0, std::string orderstring = "",
																	int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetReagentInfoByIdNum( DB_ReagentTypeRecord& icr, int64_t rxidnum = 0 );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetReagentInfoBySN( DB_ReagentTypeRecord& rxr, std::string tagsn = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddReagentInfo( DB_ReagentTypeRecord& rxr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyReagentInfo( DB_ReagentTypeRecord& rxr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveReagentInfo( DB_ReagentTypeRecord& rxr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveReagentInfoByIdNum( int64_t rxidnum = NO_ID_NUM );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveReagentInfoBySN( std::string tagsn = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveReagentInfoByIdNumList( std::vector<int64_t> idnumlist );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveReagentInfoByLotNum( std::string lotnum = "" );

		HAWKEYEDBIF_API DBApi::eQueryResult DbGetCellHealthReagentsList(
			std::vector<DB_CellHealthReagentRecord>& chrlist,
			DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
			std::string compareop = "", std::string compareval = "",
			int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
			int32_t sortdir = 0, std::string orderstring = "",
			int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "");
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetCellHealthReagentByType(DB_CellHealthReagentRecord& rxr, int16_t type);
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyCellHealthReagent(DB_CellHealthReagentRecord& chr);
		
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetBioProcessListEnhanced( std::vector<DB_BioProcessRecord>& bplist,
																		   std::vector<DBApi::eListFilterCriteria> filtertypes,
																		   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		   int32_t sortdir = 0, std::string orderstring = "",
																		   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetBioProcessList( std::vector<DB_BioProcessRecord>& bplist,
																   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																   std::string compareop = "", std::string compareval = "",
																   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																   int32_t sortdir = 0, std::string orderstring = "",
																   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindBioProcessByUuid( DB_BioProcessRecord& bpr, uuid__t bpid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindBioProcessByName( DB_BioProcessRecord& bpr, std::string bpname = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddBioProcess( DB_BioProcessRecord& bpr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyBioProcess( DB_BioProcessRecord& bpr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveBioProcess( DB_BioProcessRecord& bpr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveBioProcessByUuid( uuid__t bpid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveBioProcessByName( std::string bpname = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetQcProcessListEnhanced( std::vector<DB_QcProcessRecord>& qclist,
																		  std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		  int32_t sortdir = 0, std::string orderstring = "",
																		  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetQcProcessList( std::vector<DB_QcProcessRecord>& qclist,
																  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																  std::string compareop = "", std::string compareval = "",
																  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																  int32_t sortdir = 0, std::string orderstring = "",
																  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindQcProcessByUuid( DB_QcProcessRecord& qcr, uuid__t qcid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindQcProcessByName( DB_QcProcessRecord& qcr, std::string qcname = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddQcProcess( DB_QcProcessRecord& qcr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyQcProcess( DB_QcProcessRecord& qcr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveQcProcess( DB_QcProcessRecord& qcr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveQcProcessByUuid( uuid__t qcid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveQcProcessByName( std::string qcname = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetCalibrationListEnhanced( std::vector<DB_CalibrationRecord>& callist,
																			std::vector<DBApi::eListFilterCriteria> filtertypes,
																			std::vector<std::string> compareops, std::vector<std::string> comparevals,
																			int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																			DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																			DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																			int32_t sortdir = 0, std::string orderstring = "",
																			int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetCalibrationList( std::vector<DB_CalibrationRecord>& callist,
																	DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																	std::string compareop = "", std::string compareval = "",
																	int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																	DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																	DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																	int32_t sortdir = 0, std::string orderstring = "",
																	int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindCalibrationByUuid( DB_CalibrationRecord& car, uuid__t calid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddCalibration( DB_CalibrationRecord& car );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveCalibration( DB_CalibrationRecord& car );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveCalibrationByUuid( uuid__t calid );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetInstrumentConfigList( std::vector<DB_InstrumentConfigRecord>& cfglist );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindInstrumentConfig( DB_InstrumentConfigRecord& icr, int64_t inst_idnum = 0, std::string instsn = "" );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindInstrumentConfigBySN( DB_InstrumentConfigRecord& icr, std::string instsn );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddInstrumentConfig( DB_InstrumentConfigRecord& icr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyInstrumentConfig( DB_InstrumentConfigRecord& icr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveInstrumentConfig( DB_InstrumentConfigRecord& icr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveInstrumentConfigBySN( std::string instsn = "" );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetAuditLogList( std::vector<DB_LogEntryRecord>& loglist,
																 std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops,
																 std::vector<std::string> comparevals,
																 int32_t limitcnt,
																 DBApi::eListSortCriteria primarysort,
																 DBApi::eListSortCriteria secondarysort,
																 int32_t sortdir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddAuditLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyAuditLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearAuditLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops,
															   std::vector<std::string> comparevals );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearAuditLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops,
																		  std::vector<std::string> comparevals,
																		  std::vector <int64_t> idnumlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetErrorLogList( std::vector<DB_LogEntryRecord>& loglist,
																 std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops,
																 std::vector<std::string> comparevals,
																 int32_t limitcnt,
																 DBApi::eListSortCriteria primarysort,
																 DBApi::eListSortCriteria secondarysort,
																 int32_t sortdir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddErrorLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifyErrorLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearErrorLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops,
															   std::vector<std::string> comparevals );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearErrorLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops,
																		  std::vector<std::string> comparevals,
																		  std::vector <int64_t> idnumlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSampleLogList( std::vector<DB_LogEntryRecord>& loglist,
																  std::vector<DBApi::eListFilterCriteria> filtertypes,
																  std::vector<std::string> compareops,
																  std::vector<std::string> comparevals,
																  int32_t limitcnt,
																  DBApi::eListSortCriteria primarysort,
																  DBApi::eListSortCriteria secondarysort,
																  int32_t sortdir );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSampleLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySampleLogEntry( DB_LogEntryRecord& log_entry );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearSampleLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbClearSampleLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																		   std::vector<std::string> compareops,
																		   std::vector<std::string> comparevals,
																		   std::vector <int64_t> idnumlist );


		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSchedulerConfigListEnhanced( std::vector<DB_SchedulerConfigRecord>& scheduler_list,
																				std::vector<DBApi::eListFilterCriteria> filtertypes,
																				std::vector<std::string> compareops, std::vector<std::string> comparevals,
																				int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																				DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																				DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																				int32_t sortdir = 0, std::string orderstring = "",
																				int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbGetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord>& scheduler_list,
																		DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																		std::string compareop = "", std::string compareval = "",
																		int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																		DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																		DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																		int32_t sortdir = 0, std::string orderstring = "",
																		int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbFindSchedulerConfigByUuid( DB_SchedulerConfigRecord& scr, uuid__t scid );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbAddSchedulerConfig( DB_SchedulerConfigRecord& scr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbModifySchedulerConfig( DB_SchedulerConfigRecord& scr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSchedulerConfig( DB_SchedulerConfigRecord& scr );
		HAWKEYEDBIF_API DBApi::eQueryResult   DbRemoveSchedulerByUuid( uuid__t scid );
	}
}