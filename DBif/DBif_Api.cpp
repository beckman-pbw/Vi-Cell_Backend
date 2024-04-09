// DBif_Api.cpp : Defines the initialization routines for the DLL.
//

#include "pch.h"


#include "DBif_Api.h"
#include "DBif_Impl.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//


static DBifImpl	dbif;

//*****************************************************************************
// These APIs are list is the same order as those in *DBif_Impl.hpp*
// Please continue to maintain this ordering.  "The Management"
//*****************************************************************************


namespace DBApi
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HAWKEYEDBIF_API bool DBifInit( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.DBifInit();
}

HAWKEYEDBIF_API bool DbGetConnectionProperties( std::string& dbaddr, std::string& dbport,
												std::string& dbname, std::string& dbdriver )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.GetDBConnectionProperties( dbaddr, dbport, dbname, dbdriver );

	return true;
}

// use all default parameter settings
HAWKEYEDBIF_API bool DbSetDefaultConnection( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetDBConnectionProperties();
}

// use discrete settings
// Allows specification of all parameters;
// defaults are empty strings so the address MUST be specified for the method to be successful
HAWKEYEDBIF_API bool DbSetFullConnection( std::string dbaddr, std::string dbport,
										  std::string dbname, std::string dbdriver )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetDBConnectionProperties( dbaddr, dbport, dbname, dbdriver );
}

HAWKEYEDBIF_API bool DbSetConnectionAddrByString( std::string dbaddr, std::string dbport )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	bool success = true;
	if ( !dbif.SetDBAddress( dbaddr ) )
	{
		success = false;
	}

	if ( !dbif.SetDBPort( dbport ) )
	{
		success = false;
	}

	return success;
}

HAWKEYEDBIF_API bool DbSetConnectionAddr( std::string dbaddr, int32_t dbport )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	bool success = true;
	if ( !dbif.SetDBAddress( dbaddr ) )
	{
		success = false;
	}

	if ( !dbif.SetDBPort( dbport ) )
	{
		success = false;
	}

	return success;
}

HAWKEYEDBIF_API bool DbSetDbName( std::string dbname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetDBName( dbname );
}

HAWKEYEDBIF_API bool DbClearDb( std::string dbname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RecreateDb( dbname );
}

HAWKEYEDBIF_API bool DbSetBackupUserPwd( DBApi::eQueryResult & resultcode, std::string & pwdstr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetBackupUserPwd( resultcode, pwdstr );
}


HAWKEYEDBIF_API bool DbTruncateTableContents(DBApi::eQueryResult& resultcode, std::list<std::pair<std::string, std::string>> tableNames_name_schema)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return dbif.TruncateTableContents(resultcode, tableNames_name_schema);
}

////////////////////////////////////////////////////////////////////////////////
// database connection methods
////////////////////////////////////////////////////////////////////////////////

// checks for connection to database and returns the connection user-type
HAWKEYEDBIF_API bool IsDBPresent( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.IsDBPresent();
}

// check connection for user type
HAWKEYEDBIF_API bool DbIsLoginType( DBApi::eLoginType userType )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.IsLoginType( userType );
}

// check connection for user type
HAWKEYEDBIF_API DBApi::eLoginType DbGetLoginType( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.GetLoginType();
}

// sets the connection type to use
// returns NoConnection if type is not yet connected or the 'Both' type is requested;
// returns the requested connection type on success
HAWKEYEDBIF_API DBApi::eLoginType DbSetLoginType( DBApi::eLoginType userType )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetLoginType( userType );
}

// connect to database as standard user; construct the username and password for the standard user
//
// NOTE: this method is structured to make it difficult to determine the constructed username and password,
// but is not intended to be completely secure!
//
HAWKEYEDBIF_API DBApi::eLoginType DbInstrumentLogin( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.LoginAsInstrument();
}

// connect to database as a specified user (may be service or admin or other supplied username / password combination.
// Does not guarantee the supplied username will have admin privileges;
// Checks for the standard instrument username and goes to the standard connect if found;
// Allows passing in either full usernames or username tags
// Allows passing passwords (not desirable...)
// If known username tags for the admin or service user are supplied, 
// constructs the valid username and password combination for those known user name tags.
// If the admin connection is currenty open, closes the existing connection and (attempts to) opens the new one
//
// NOTE: this method is structured to make it difficult to determine the constructed passwords,
// but is not intended to be completely secure!
//
HAWKEYEDBIF_API DBApi::eLoginType DbUserLogin( std::string unamestr, std::string hashstr, std::string pwdstr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.LoginAsUser( unamestr, hashstr, pwdstr );
}
// connect to database as a specified user (may be service or admin or other supplied username / password combination.
// Does not guarantee the supplied username will have admin privileges;
// Checks for the standard instrument username and goes to the standard connect if found;
// Allows passing in either full usernames or username tags
// Allows passing passwords (not desirable...)
// If known username tags for the admin or service user are supplied, 
// constructs the valid username and password combination for those known user name tags.
// If the admin connection is currenty open, closes the existing connection and (attempts to) opens the new one
//
// NOTE: this method is structured to make it difficult to determine the constructed passwords,
// but is not intended to be completely secure!
//
HAWKEYEDBIF_API DBApi::eLoginType DbAdminLogin( std::string unamestr, std::string hashstr, std::string pwdstr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.LoginAsAdmin( unamestr, hashstr, pwdstr );
}

HAWKEYEDBIF_API void DbLogoutUserType( DBApi::eLoginType userType )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.LogoutUserType( userType );
}

// disconnect from database
HAWKEYEDBIF_API void DbDisconnect( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.Disconnect();
}


////////////////////////////////////////////////////////////////////////////////
// Time string and query results string helper methods
////////////////////////////////////////////////////////////////////////////////

HAWKEYEDBIF_API void GetDbCurrentTimeString( std::string& time_string )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.GetDbCurrentTimeString( time_string );
}

HAWKEYEDBIF_API void GetDbCurrentTimePtAndString( std::string& time_string, system_TP& time_pt )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.GetDbCurrentTimeString( time_string, time_pt );
}

HAWKEYEDBIF_API void GetDbTimeString( system_TP time_pt, std::string& time_string, bool isutctp )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.GetDbTimeString( time_pt, time_string, isutctp );
}

HAWKEYEDBIF_API bool GetTimePtFromDbTimeString( system_TP& time_pt, std::string time_string )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.GetTimePtFromDbTimeString( time_pt, time_string );
}

HAWKEYEDBIF_API void GetQueryResultString( DBApi::eQueryResult resultcode, std::string& resultstring )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	dbif.GetQueryResultString( resultcode, resultstring );
}


////////////////////////////////////////////////////////////////////////////////
// query execution methods
////////////////////////////////////////////////////////////////////////////////

// execute query; determine appropriate user type automatically
HAWKEYEDBIF_API DBApi::eQueryResult DbExecuteQuery( std::string query_str, std::vector<std::string>& resultList )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ExecuteQuery( query_str, resultList );
}

// execute qurery
HAWKEYEDBIF_API DBApi::eQueryResult DbExecuteQueryType( DBApi::eLoginType usertype, std::string query_str, std::vector<std::string>& resultlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ExecuteQueryType( usertype, query_str, resultlist );
}


////////////////////////////////////////////////////////////////////////////////
// retrieval, insertion, and update methods
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Work Queue and sample methods
////////////////////////////////////////////////////////////////////////////////

HAWKEYEDBIF_API DBApi::eQueryResult DbGetWorklistListEnhanced( std::vector<DB_WorklistRecord>& wlist,
															   std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops,
															   std::vector<std::string> comparevals,
															   int32_t limitcnt,
															   DBApi::eListSortCriteria primarysort,
															   DBApi::eListSortCriteria secondarysort,
															   DBApi::eContainedObjectRetrieval get_sub_items,
															   int32_t sortdir, std::string orderstring,
															   int32_t startindex, int64_t startidnum,
															   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Worklists( wlist,
										filtertypes, compareops, comparevals,
										limitcnt,
										primarysort, secondarysort,
										get_sub_items, sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetWorklistList( std::vector<DB_WorklistRecord>& wlist,
													   DBApi::eListFilterCriteria filtertype,
													   std::string compareop,
													   std::string compareval,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   DBApi::eContainedObjectRetrieval get_sub_items,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum,
													   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Worklists( wlist,
										filtertype, compareop, compareval,
										limitcnt,
										primarysort, secondarysort,
										get_sub_items, sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindWorklist( DB_WorklistRecord& wlr, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindWorklist( wlr, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddWorklist( DB_WorklistRecord& wlr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddWorklist( wlr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSaveWorklistTemplate( DB_WorklistRecord& wlr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SaveWorklistTemplate( wlr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddWorklistSampleSet( ssr, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddWorklistSampleSetByUuid( uuid__t samplesetid, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddWorklistSampleSet( samplesetid, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklistSampleSet( ssr, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklistSampleSetByUuid( uuid__t samplesetid, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklistSampleSet( samplesetid, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklistSampleSets( std::vector<uuid__t>& idlist, uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklistSampleSets( idlist, worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyWorklist( DB_WorklistRecord& wlr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyWorklist( wlr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyWorklistTemplate( DB_WorklistRecord& wlr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyWorklistTemplate( wlr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetWorklistStatus(uuid__t worklistid, DBApi::eWorklistStatus status)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());

	return dbif.SetWorklistStatus ( worklistid, status );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetWorklistSampleSetStatus( worklistid, samplesetid, status );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklist( DB_WorklistRecord& wlr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklist( wlr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklistByUuid( uuid__t worklistid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklist( worklistid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveWorklists( std::vector<uuid__t>& idlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveWorklists( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleSetListEnhanced( std::vector<DB_SampleSetRecord>& samplesetlist,
																std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort,
																DBApi::eListSortCriteria secondarysort,
																DBApi::eContainedObjectRetrieval get_sub_items,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum,
																std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleSets( samplesetlist,
										 filtertypes, compareops, comparevals,
										 limitcnt,
										 primarysort, secondarysort,
										 get_sub_items, sortdir, orderstring,
										 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleSetList( std::vector<DB_SampleSetRecord>& samplesetlist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop,
														std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort,
														DBApi::eListSortCriteria secondarysort,
														DBApi::eContainedObjectRetrieval get_sub_items,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum,
														std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleSets( samplesetlist,
										 filtertype, compareop, compareval,
										 limitcnt,
										 primarysort, secondarysort,
										 get_sub_items, sortdir, orderstring,
										 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleSetListByUuidList( std::vector<DB_SampleSetRecord>& samplesetlist, std::vector<uuid__t> ssidlist,
																  int32_t limitcnt,
																  DBApi::eListSortCriteria primarysort,
																  DBApi::eListSortCriteria secondarysort,
																  DBApi::eContainedObjectRetrieval get_sub_items,
																  int32_t sortdir, std::string orderstring,
																  int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleSets( samplesetlist, ssidlist,
										 limitcnt,
										 primarysort, secondarysort,
										 get_sub_items, sortdir, orderstring,
										 startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleSetListForWorklist( std::vector<DB_SampleSetRecord>& samplesetlist, DB_WorklistRecord& wlr,
																   int32_t limitcnt,
																   DBApi::eListSortCriteria primarysort,
																   DBApi::eListSortCriteria secondarysort,
																   int32_t sortdir, std::string orderstring,
																   int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleSets( samplesetlist, wlr,
										 limitcnt,
										 primarysort, secondarysort,
										 sortdir, orderstring,
										 startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleSetListForWorklistId( std::vector<DB_SampleSetRecord>& samplesetlist, uuid__t wlid,
																	 int32_t limitcnt,
																	 DBApi::eListSortCriteria primarysort,
																	 DBApi::eListSortCriteria secondarysort,
																	 int32_t sortdir, std::string orderstring,
																	 int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleSets( samplesetlist, wlid,
										 limitcnt,
										 primarysort, secondarysort,
										 sortdir, orderstring,
										 startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSampleSet( DB_SampleSetRecord& ssr, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSampleSet( ssr, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSampleSet( DB_SampleSetRecord& ssr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSampleSet( ssr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSaveSampleSetTemplate( DB_SampleSetRecord& ssr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SaveSampleSetTemplate( ssr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSampleSetSample( DB_SampleItemRecord& ssir, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSampleSetLineItem( ssir, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSampleSetSampleByUuid( uuid__t ssitemid, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSampleSetLineItem( ssitemid, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSetSample( DB_SampleItemRecord& ssir, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSetLineItem( ssir, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSetSampleByUuid( uuid__t ssitemid, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSetLineItem( ssitemid, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSetSamples( std::vector<uuid__t>& idlist, uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSetLineItems( idlist, samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySampleSet( DB_SampleSetRecord& ssr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySampleSet( ssr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySampleSetTemplate( DB_SampleSetRecord& ssr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySampleSetTemplate( ssr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetSampleSetStatus( samplesetid, status );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetSampleSetSampleItemStatus( uuid__t samplesetid, uuid__t itemid, DBApi::eSampleItemStatus status )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetSampleSetLineItemStatus(samplesetid, itemid, status);
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSet( DB_SampleSetRecord& ssr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSet( ssr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSetByUuid( uuid__t samplesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSet( samplesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleSets( std::vector<uuid__t>& idlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleSets( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleItemListEnhanced( std::vector<DB_SampleItemRecord>& ssirlist,
																 std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops,
																 std::vector<std::string> comparevals,
																 int32_t limitcnt,
																 DBApi::eListSortCriteria primarysort,
																 DBApi::eListSortCriteria secondarysort,
																 int32_t sortdir, std::string orderstring,
																 int32_t startindex, int64_t startidnum,
																 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleItems( ssirlist,
										  filtertypes, compareops, comparevals,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop,
														 std::string compareval,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum,
														 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleItems( ssirlist,
										  filtertype, compareop, compareval,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleItemListByUuidList( std::vector<DB_SampleItemRecord>& ssirlist, std::vector<uuid__t> ssidlist,
																   int32_t limitcnt,
																   DBApi::eListSortCriteria primarysort,
																   DBApi::eListSortCriteria secondarysort,
																   int32_t sortdir, std::string orderstring,
																   int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleItems( ssirlist, ssidlist,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleItemListForSampleSet( std::vector<DB_SampleItemRecord>& ssirlist, DB_SampleSetRecord& ssr,
																	 int32_t limitcnt,
																	 DBApi::eListSortCriteria primarysort,
																	 DBApi::eListSortCriteria secondarysort,
																	 int32_t sortdir, std::string orderstring,
																	 int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleItems( ssirlist, ssr,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleItemListForSampleSetUuid( std::vector<DB_SampleItemRecord>& ssirlist, uuid__t ssid,
																		 int32_t limitcnt,
																		 DBApi::eListSortCriteria primarysort,
																		 DBApi::eListSortCriteria secondarysort,
																		 int32_t sortdir, std::string orderstring,
																		 int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SampleItems( ssirlist, ssid,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSampleItem( DB_SampleItemRecord& ssir, uuid__t itemid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSampleItem( ssir, itemid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSampleItem( DB_SampleItemRecord& ssir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSampleItem( ssir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySampleItem( DB_SampleItemRecord& ssir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySampleItem( ssir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetSampleItemStatus( itemid, status );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleItem( DB_SampleItemRecord& ssir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleItem( ssir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleItemByUuid( uuid__t itemid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleItem( itemid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleItems( std::vector<uuid__t>& idlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSampleItems( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleListEnhanced( std::vector<DB_SampleRecord>& samplelist,
															 std::vector<DBApi::eListFilterCriteria> filtertypes,
															 std::vector<std::string> compareops,
															 std::vector<std::string> comparevals,
															 int32_t limitcnt,
															 DBApi::eListSortCriteria primarysort,
															 DBApi::eListSortCriteria secondarysort,
															 int32_t sortdir, std::string orderstring,
															 int32_t startindex, int64_t startidnum,
															 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Samples( samplelist,
									  filtertypes, compareops, comparevals,
									  limitcnt,
									  primarysort, secondarysort,
									  sortdir, orderstring,
									  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleList( std::vector<DB_SampleRecord>& samplelist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop,
													 std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum,
													 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Samples( samplelist,
									  filtertype, compareop, compareval,
									  limitcnt,
									  primarysort, secondarysort,
									  sortdir, orderstring,
									  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSample( DB_SampleRecord& sr, uuid__t sampleid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSample( sr, sampleid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSample( DB_SampleRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSample( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySample( DB_SampleRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySample( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbSetSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.SetSampleStatus( sampleid, status );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSample( DB_SampleRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSample( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSampleByUuid( uuid__t sampleid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSample( sampleid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSamples( std::vector<uuid__t>& idlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSamples( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysesListEnhanced( std::vector<DB_AnalysisRecord>& anlist,
															   std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops,
															   std::vector<std::string> comparevals,
															   int32_t limitcnt,
															   DBApi::eListSortCriteria primarysort,
															   DBApi::eListSortCriteria secondarysort,
															   int32_t sortdir, std::string orderstring,
															   int32_t startindex, int64_t startidnum,
															   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Analyses( anlist,
									   filtertypes, compareops, comparevals,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysesList( std::vector<DB_AnalysisRecord>& anlist,
													   DBApi::eListFilterCriteria filtertype,
													   std::string compareop,
													   std::string compareval,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum,
													   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Analyses( anlist,
									   filtertype, compareop, compareval,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindAnalysis( DB_AnalysisRecord& ar, uuid__t analysisid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindAnalysis( ar, analysisid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddAnalysis( DB_AnalysisRecord& ar )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddAnalysis( ar );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyAnalysis( DB_AnalysisRecord& ar )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyAnalysis( ar );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysis( DB_AnalysisRecord& ar )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysis( ar );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisByUuid( uuid__t analysisid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysis( analysisid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisBySampleId( uuid__t sampleid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisBySampleId( sampleid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisByUuidList( std::vector<uuid__t> idlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisByUuidList( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSummaryResultsListEnhanced( std::vector<DB_SummaryResultRecord>& srlist,
																	 std::vector<DBApi::eListFilterCriteria> filtertypes,
																	 std::vector<std::string> compareops,
																	 std::vector<std::string> comparevals,
																	 int32_t limitcnt,
																	 DBApi::eListSortCriteria primarysort,
																	 DBApi::eListSortCriteria secondarysort,
																	 int32_t sortdir, std::string orderstring,
																	 int32_t startindex, int64_t startidnum,
																	 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SummaryResults( srlist,
											 filtertypes, compareops, comparevals,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSummaryResultsList( std::vector<DB_SummaryResultRecord>& srlist,
															 DBApi::eListFilterCriteria filtertype,
															 std::string compareop,
															 std::string compareval,
															 int32_t limitcnt,
															 DBApi::eListSortCriteria primarysort,
															 DBApi::eListSortCriteria secondarysort,
															 int32_t sortdir, std::string orderstring,
															 int32_t startindex, int64_t startidnum,
															 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SummaryResults( srlist,
											 filtertype, compareop, compareval,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSummaryResult( sr, resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSummaryResult( DB_SummaryResultRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSummaryResult( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySummaryResult( DB_SummaryResultRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySummaryResult( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSummaryResult( DB_SummaryResultRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSummaryResult( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSummaryResultByUuid( uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSummaryResult( resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetDetailedResultsListEnhanced( std::vector<DB_DetailedResultRecord>& drlist,
																	  std::vector<DBApi::eListFilterCriteria> filtertypes,
																	  std::vector<std::string> compareops,
																	  std::vector<std::string> comparevals,
																	  int32_t limitcnt,
																	  DBApi::eListSortCriteria primarysort,
																	  DBApi::eListSortCriteria secondarysort,
																	  int32_t sortdir, std::string orderstring,
																	  int32_t startindex, int64_t startidnum,
																	  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_DetailedResults( drlist,
											  filtertypes, compareops, comparevals,
											  limitcnt,
											  primarysort, secondarysort,
											  sortdir, orderstring,
											  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetDetailedResultsList( std::vector<DB_DetailedResultRecord>& drlist,
															  DBApi::eListFilterCriteria filtertype,
															  std::string compareop,
															  std::string compareval,
															  int32_t limitcnt,
															  DBApi::eListSortCriteria primarysort,
															  DBApi::eListSortCriteria secondarysort,
															  int32_t sortdir, std::string orderstring,
															  int32_t startindex, int64_t startidnum,
															  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_DetailedResults( drlist,
											  filtertype, compareop, compareval,
											  limitcnt,
											  primarysort, secondarysort,
											  sortdir, orderstring,
											  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindDetailedResult( DB_DetailedResultRecord& dr, uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindDetailedResult( dr, resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddDetailedResult( DB_DetailedResultRecord& dr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddDetailedResult( dr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveDetailedResult( DB_DetailedResultRecord& dr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveDetailedResult( dr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveDetailedResultByUuid( uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveDetailedResult( resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageResultsListEnhanced( std::vector<DB_ImageResultRecord>& irlist,
																   std::vector<DBApi::eListFilterCriteria> filtertypes,
																   std::vector<std::string> compareops,
																   std::vector<std::string> comparevals,
																   int32_t limitcnt,
																   DBApi::eListSortCriteria primarysort,
																   DBApi::eListSortCriteria secondarysort,
																   int32_t sortdir, std::string orderstring,
																   int32_t startindex, int64_t startidnum,
																   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageResults( irlist,
										   filtertypes, compareops, comparevals,
										   limitcnt,
										   primarysort, secondarysort,
										   sortdir, orderstring,
										   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageResultsList( std::vector<DB_ImageResultRecord>& irlist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop,
														   std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort,
														   DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum,
														   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageResults( irlist,
										   filtertype, compareop, compareval,
										   limitcnt,
										   primarysort, secondarysort,
										   sortdir, orderstring,
										   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageResultsListByUuidList( std::vector<DB_ImageResultRecord>& irlist, std::vector<uuid__t> iridlist,
																	 int32_t limitcnt,
																	 DBApi::eListSortCriteria primarysort,
																	 DBApi::eListSortCriteria secondarysort,
																	 int32_t sortdir, std::string orderstring,
																	 int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageResults( irlist, iridlist,
										   limitcnt,
										   primarysort, secondarysort,
										   sortdir, orderstring,
										   startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImageResult( DB_ImageResultRecord& ir, uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImageResult( ir, resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImageResultByIdNum( DB_ImageResultRecord& ir, int64_t idnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImageResult( ir, idnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddImageResult( DB_ImageResultRecord& ir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddImageResult( ir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageResult( DB_ImageResultRecord& ir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageResult( ir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageResultByUuid( uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageResult( resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSResultListEnhanced( std::vector<DB_SResultRecord>& srlist,
															  std::vector<DBApi::eListFilterCriteria> filtertypes,
															  std::vector<std::string> compareops,
															  std::vector<std::string> comparevals,
															  int32_t limitcnt,
															  DBApi::eListSortCriteria primarysort,
															  DBApi::eListSortCriteria secondarysort,
															  int32_t sortdir, std::string orderstring,
															  int32_t startindex, int64_t startidnum,
															  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SResults( srlist,
									   filtertypes, compareops, comparevals,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSResultList( std::vector<DB_SResultRecord>& srlist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop,
													  std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort,
													  DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum,
													  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SResults( srlist,
									   filtertype, compareop, compareval,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSResult( DB_SResultRecord& sr, uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSResult( sr, resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSResult( DB_SResultRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSResult( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSResult( DB_SResultRecord& sr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSResult( sr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSResultByUuid( uuid__t resultid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSResult( resultid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageSetsListEnhanced( std::vector<DB_ImageSetRecord>& islist,
																std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort,
																DBApi::eListSortCriteria secondarysort,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum,
																std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageSets( islist,
										filtertypes, compareops, comparevals,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageSetsList( std::vector<DB_ImageSetRecord>& islist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop,
														std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort,
														DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum,
														std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageSets( islist,
										filtertype, compareop, compareval,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImageSet( DB_ImageSetRecord& isr, uuid__t imagesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImageSet( isr, imagesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddImageSet( DB_ImageSetRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddImageSet( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyImageSet( DB_ImageSetRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyImageSet( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSet( DB_ImageSetRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSet( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSetByUuid( uuid__t imagesetid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSet( imagesetid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageSequenceListEnhanced( std::vector<DB_ImageSeqRecord>& isrlist,
																	std::vector<DBApi::eListFilterCriteria> filtertypes,
																	std::vector<std::string> compareops,
																	std::vector<std::string> comparevals,
																	int32_t limitcnt,
																	DBApi::eListSortCriteria primarysort,
																	DBApi::eListSortCriteria secondarysort,
																	int32_t sortdir, std::string orderstring,
																	int32_t startindex, int64_t startidnum,
																	std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageSequences( isrlist,
											 filtertypes, compareops, comparevals,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
															DBApi::eListFilterCriteria filtertype,
															std::string compareop,
															std::string compareval,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort,
															DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum,
															std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageSequences( isrlist,
											 filtertype, compareop, compareval,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImageSequence( DB_ImageSeqRecord& isr, uuid__t imageseqid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImageSequence( isr, imageseqid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddImageSequence( DB_ImageSeqRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddImageSequence( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyImageSequence( DB_ImageSeqRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyImageSequence( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSequence( DB_ImageSeqRecord& isr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSequence( isr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSequenceByUuid( uuid__t imageseqid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSequence( imageseqid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSequences( std::vector<DB_ImageSeqRecord>& seqlist )
// removes the linkage information to the out-of-db storage location for images removed from the stored set
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSequences( seqlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageSequencesByUuidList( std::vector<uuid__t>& idlist )
// removes the linkage information to the out-of-db storage location for images removed from the stored set
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageSequencesByIdList( idlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImagesListEnhanced( std::vector<DB_ImageRecord>& irlist,
															 std::vector<DBApi::eListFilterCriteria> filtertypes,
															 std::vector<std::string> compareops,
															 std::vector<std::string> comparevals,
															 int32_t limitcnt,
															 DBApi::eListSortCriteria primarysort,
															 DBApi::eListSortCriteria secondarysort,
															 int32_t sortdir, std::string orderstring,
															 int32_t startindex, int64_t startidnum,
															 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Images( irlist,
									 filtertypes, compareops, comparevals,
									 limitcnt,
									 primarysort, secondarysort,
									 sortdir, orderstring,
									 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImagesList( std::vector<DB_ImageRecord>& irlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop,
													 std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort,
													 DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum,
													 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Images( irlist,
									 filtertype, compareop, compareval,
									 limitcnt,
									 primarysort, secondarysort,
									 sortdir, orderstring,
									 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImage( DB_ImageRecord& ir, uuid__t imageid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImage( ir, imageid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddImage( DB_ImageRecord& ir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddImage( ir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyImage( DB_ImageRecord& ir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyImage( ir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImage( DB_ImageRecord& ir )
// removes the entire image reference record; should cascade up to image sets and image records
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImage( ir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageByUuid( uuid__t imageid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImage( imageid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImages( std::vector<DB_ImageRecord>& imglist )
// removes the linkage information to the out-of-db storage location for images removed from the stored set
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImages( imglist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImagesByUuidList( std::vector<uuid__t>& idlist )
// removes the linkage information to the out-of-db storage location for images removed from the stored set
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImagesByIdList( idlist );
}


////////////////////////////////////////////////////////////////////////////////
// Instrument config, workflow, and parameter methods
////////////////////////////////////////////////////////////////////////////////

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCellTypeListEnhanced( std::vector<DB_CellTypeRecord>& ctlist,
															   std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops,
															   std::vector<std::string> comparevals,
															   int32_t limitcnt,
															   DBApi::eListSortCriteria primarysort,
															   DBApi::eListSortCriteria secondarysort,
															   int32_t sortdir, std::string orderstring,
															   int32_t startindex, int64_t startidnum,
															   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_CellType( ctlist,
									   filtertypes, compareops, comparevals,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCellTypeList( std::vector<DB_CellTypeRecord>& ctlist,
													   DBApi::eListFilterCriteria filtertype,
													   std::string compareop,
													   std::string compareval,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum,
													   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_CellType( ctlist,
									   filtertype, compareop, compareval,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindCellTypeByUuid( DB_CellTypeRecord& ctr, uuid__t cellid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindCellType( ctr, cellid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindCellTypeByIndex( DB_CellTypeRecord& ctr, uint32_t celltypeindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindCellType( ctr, celltypeindex );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddCellType( DB_CellTypeRecord& ctr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddCellType( ctr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyCellType( DB_CellTypeRecord& ctr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyCellType( ctr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveCellType( DB_CellTypeRecord& ctr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveCellType( ctr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveCellTypeByUuid( uuid__t celltypeid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveCellType( celltypeid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageAnalysisParameterListEnhanced( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																			  std::vector<DBApi::eListFilterCriteria> filtertypes,
																			  std::vector<std::string> compareops,
																			  std::vector<std::string> comparevals,
																			  int32_t limitcnt,
																			  DBApi::eListSortCriteria primarysort,
																			  DBApi::eListSortCriteria secondarysort,
																			  int32_t sortdir, std::string orderstring,
																			  int32_t startindex, int64_t startidnum,
																			  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageAnalysisParameters( aplist,
													  filtertypes, compareops, comparevals,
													  limitcnt,
													  primarysort, secondarysort,
													  sortdir, orderstring,
													  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetImageAnalysisParameterList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																	  DBApi::eListFilterCriteria filtertype,
																	  std::string compareop,
																	  std::string compareval,
																	  int32_t limitcnt,
																	  DBApi::eListSortCriteria primarysort,
																	  DBApi::eListSortCriteria secondarysort,
																	  int32_t sortdir, std::string orderstring,
																	  int32_t startindex, int64_t startidnum,
																	  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_ImageAnalysisParameters( aplist,
													  filtertype, compareop, compareval,
													  limitcnt,
													  primarysort, secondarysort,
													  sortdir, orderstring,
													  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindImageAnalysisParameter( DB_ImageAnalysisParamRecord& params, uuid__t paramid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindImageAnalysisParameter( params, paramid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddImageAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyImageAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveImageAnalysisParameterByUuid( uuid__t paramid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveImageAnalysisParameter( paramid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysisParameterListEnhanced( std::vector<DB_AnalysisParamRecord>& aplist,
																		 std::vector<DBApi::eListFilterCriteria> filtertypes,
																		 std::vector<std::string> compareops,
																		 std::vector<std::string> comparevals,
																		 int32_t limitcnt,
																		 DBApi::eListSortCriteria primarysort,
																		 DBApi::eListSortCriteria secondarysort,
																		 int32_t sortdir, std::string orderstring,
																		 int32_t startindex, int64_t startidnum,
																		 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_AnalysisParameters( aplist,
												 filtertypes, compareops, comparevals,
												 limitcnt,
												 primarysort, secondarysort,
												 sortdir, orderstring,
												 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysisParameterList( std::vector<DB_AnalysisParamRecord>& aplist,
																 DBApi::eListFilterCriteria filtertype,
																 std::string compareop,
																 std::string compareval,
																 int32_t limitcnt,
																 DBApi::eListSortCriteria primarysort,
																 DBApi::eListSortCriteria secondarysort,
																 int32_t sortdir, std::string orderstring,
																 int32_t startindex, int64_t startidnum,
																 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_AnalysisParameters( aplist,
												 filtertype, compareop, compareval,
												 limitcnt,
												 primarysort, secondarysort,
												 sortdir, orderstring,
												 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindAnalysisParameter( DB_AnalysisParamRecord& params, uuid__t paramid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindAnalysisParameter( params, paramid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddAnalysisParameter( DB_AnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyAnalysisParameter( DB_AnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisParameter( DB_AnalysisParamRecord& params )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisParameter( params );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisParameterByUuid( uuid__t paramid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisParameter( paramid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysisDefinitionsListEnhanced( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																		  std::vector<DBApi::eListFilterCriteria> filtertypes,
																		  std::vector<std::string> compareops,
																		  std::vector<std::string> comparevals,
																		  int32_t limitcnt,
																		  DBApi::eListSortCriteria primarysort,
																		  DBApi::eListSortCriteria secondarysort,
																		  int32_t sortdir, std::string orderstring,
																		  int32_t startindex, int64_t startidnum,
																		  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_AnalysisDefinitions( adlist,
												  filtertypes, compareops, comparevals,
												  limitcnt,
												  primarysort, secondarysort,
												  sortdir, orderstring,
												  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAnalysisDefinitionsList( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																  DBApi::eListFilterCriteria filtertype,
																  std::string compareop,
																  std::string compareval,
																  int32_t limitcnt,
																  DBApi::eListSortCriteria primarysort,
																  DBApi::eListSortCriteria secondarysort,
																  int32_t sortdir, std::string orderstring,
																  int32_t startindex, int64_t startidnum,
																  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_AnalysisDefinitions( adlist,
												  filtertype, compareop, compareval,
												  limitcnt,
												  primarysort, secondarysort,
												  sortdir, orderstring,
												  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindAnalysisDefinitionByUuid( DB_AnalysisDefinitionRecord& def, uuid__t defid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindAnalysisDefinition( def, defid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindAnalysisDefinitionByIndex( DB_AnalysisDefinitionRecord& def, int32_t defindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindAnalysisDefinition( def, defindex );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddAnalysisDefinition( def );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyAnalysisDefinition( def );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisDefinition( def );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveAnalysisDefinitionByUuid( uuid__t defid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveAnalysisDefinition( defid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetIlluminatorsListEnhanced( std::vector<DB_IlluminatorRecord>& ilrlist,
																   std::vector<DBApi::eListFilterCriteria> filtertypes,
																   std::vector<std::string> compareops,
																   std::vector<std::string> comparevals,
																   int32_t limitcnt,
																   DBApi::eListSortCriteria primarysort,
																   DBApi::eListSortCriteria secondarysort,
																   int32_t sortdir, std::string orderstring,
																   int32_t startindex, int64_t startidnum,
																   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Illuminators( ilrlist,
										   filtertypes, compareops, comparevals,
										   limitcnt,
										   primarysort, secondarysort,
										   sortdir, orderstring,
										   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetIlluminatorsList( std::vector<DB_IlluminatorRecord>& ilrlist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop,
														   std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort,
														   DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum,
														   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Illuminators( ilrlist,
										   filtertype, compareop, compareval,
										   limitcnt,
										   primarysort, secondarysort,
										   sortdir, orderstring,
										   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindIlluminatorsByIndex( DB_IlluminatorRecord& ilr, int16_t ilindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindIlluminator( ilr, "", ilindex);
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindIlluminatorsByName( DB_IlluminatorRecord& ilr, std::string ilname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindIlluminator( ilr, ilname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindIlluminatorsByWavelength( DB_IlluminatorRecord& ilr, int16_t emission, int16_t illuminator )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindIlluminator( ilr, emission, illuminator );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddIlluminator( DB_IlluminatorRecord& ilr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddIlluminator( ilr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyIlluminator( DB_IlluminatorRecord& ilr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyIlluminator( ilr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveIlluminator( DB_IlluminatorRecord& ilr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveIlluminator( ilr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveIlluminatorByIndex( int16_t ilindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveIlluminator( ilindex, NO_ID_NUM );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveIlluminatorByName( std::string ilname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveIlluminator( ilname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserListEnhanced( std::vector<DB_UserRecord>& urlist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops,
														   std::vector<std::string> comparevals,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort,
														   DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum,
														   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Users( urlist,
									filtertypes, compareops, comparevals,
									limitcnt,
									primarysort, secondarysort,
									sortdir, orderstring,
									startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserList( std::vector<DB_UserRecord>& urlist,
												   DBApi::eListFilterCriteria filtertype,
												   std::string compareop,
												   std::string compareval,
												   int32_t limitcnt,
												   DBApi::eListSortCriteria primarysort,
												   DBApi::eListSortCriteria secondarysort,
												   int32_t sortdir, std::string orderstring,
												   int32_t startindex, int64_t startidnum,
												   std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Users( urlist,
									filtertype, compareop, compareval,
									limitcnt,
									primarysort, secondarysort,
									sortdir, orderstring,
									startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserByUuid( DB_UserRecord& ur, uuid__t userid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUser( ur, userid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserByName( DB_UserRecord& ur, std::string username, DBApi::eUserType user_type )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUser( ur, username, user_type );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddUser( DB_UserRecord& ur )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddUser( ur );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyUser( DB_UserRecord& ur )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyUser( ur );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUser( DB_UserRecord& ur )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUser( ur );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserByUuid( uuid__t userid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUser( userid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserByName( std::string username )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUser( username );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetRolesListEnhanced( std::vector<DB_UserRoleRecord>& rrlist,
															std::vector<DBApi::eListFilterCriteria> filtertypes,
															std::vector<std::string> compareops,
															std::vector<std::string> comparevals,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort,
															DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum,
															std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserRoles( rrlist,
										filtertypes, compareops, comparevals,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetRolesList( std::vector<DB_UserRoleRecord>& rrlist,
													DBApi::eListFilterCriteria filtertype,
													std::string compareop,
													std::string compareval,
													int32_t limitcnt,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort,
													int32_t sortdir, std::string orderstring,
													int32_t startindex, int64_t startidnum,
													std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserRoles( rrlist,
										filtertype, compareop, compareval,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserRoleByUuid( DB_UserRoleRecord& rr, uuid__t roleid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUserRole( rr, roleid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserRoleByName( DB_UserRoleRecord& rr, std::string rolename )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUserRole( rr, rolename );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddUserRole( DB_UserRoleRecord& rr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddUserRole( rr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyUserRole( DB_UserRoleRecord& rr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyUserRole( rr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserRole( DB_UserRoleRecord& rr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserRole( rr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserRoleByIdByUuid( uuid__t roleid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserRole( roleid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserRoleByName( std::string rolename )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserRole( rolename );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserPropertiesListEnhanced( std::vector<DB_UserPropertiesRecord>& uplist,
																	 std::vector<DBApi::eListFilterCriteria> filtertypes,
																	 std::vector<std::string> compareops,
																	 std::vector<std::string> comparevals,
																	 int32_t limitcnt,
																	 DBApi::eListSortCriteria primarysort,
																	 DBApi::eListSortCriteria secondarysort,
																	 int32_t sortdir, std::string orderstring,
																	 int32_t startindex, int64_t startidnum,
																	 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserProperties( uplist,
											 filtertypes, compareops, comparevals,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
															 DBApi::eListFilterCriteria filtertype,
															 std::string compareop,
															 std::string compareval,
															 int32_t limitcnt,
															 DBApi::eListSortCriteria primarysort,
															 DBApi::eListSortCriteria secondarysort,
															 int32_t sortdir, std::string orderstring,
															 int32_t startindex, int64_t startidnum,
															 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserProperties( uplist,
											 filtertype, compareop, compareval,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserPropertiesListForUserUuid( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t userid,
																		int32_t limitcnt,
																		DBApi::eListSortCriteria primarysort,
																		DBApi::eListSortCriteria secondarysort,
																		int32_t sortdir, std::string orderstring,
																		int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserProperties( uplist, userid,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetUserPropertiesListForUserName( std::vector<DB_UserPropertiesRecord>& uplist, std::string username,
																		int32_t limitcnt,
																		DBApi::eListSortCriteria primarysort,
																		DBApi::eListSortCriteria secondarysort,
																		int32_t sortdir, std::string orderstring,
																		int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_UserProperties( uplist, username,
											 limitcnt,
											 primarysort, secondarysort,
											 sortdir, orderstring,
											 startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserPropertyByIndex( DB_UserPropertiesRecord& up, int16_t propindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUserProperty( up, propindex );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindUserPropertyByName( DB_UserPropertiesRecord& up, std::string propname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindUserProperty( up, propname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddUserProperty( DB_UserPropertiesRecord& up )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddUserProperty( up );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserProperty( DB_UserPropertiesRecord& up )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserProperty( up );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserPropertyByIndex( int16_t propindex )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserProperty( propindex );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveUserPropertyByName( std::string propname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveUserProperty( propname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSignatureListEnhanced( std::vector<DB_SignatureRecord>& siglist,
																std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort,
																DBApi::eListSortCriteria secondarysort,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum,
																std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SignatureDefs( siglist,
											filtertypes, compareops, comparevals,
											limitcnt,
											primarysort, secondarysort,
											sortdir, orderstring,
											startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSignatureList( std::vector<DB_SignatureRecord>& siglist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop,
														std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort,
														DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum,
														std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SignatureDefs( siglist,
											filtertype, compareop, compareval,
											limitcnt,
											primarysort, secondarysort,
											sortdir, orderstring,
											startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSignatureByUuid( DB_SignatureRecord& sigrec, uuid__t sigid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSignatureDef( sigrec, sigid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSignature( DB_SignatureRecord& sigrec )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSignatureDef( sigrec );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSignature( DB_SignatureRecord& sigrec )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSignatureDef( sigrec );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSignatureByUuid( uuid__t sigid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSignatureDef( sigid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetReagentInfoListEnhanced( std::vector<DB_ReagentTypeRecord>& rxlist,
																  std::vector<DBApi::eListFilterCriteria> filtertypes,
																  std::vector<std::string> compareops,
																  std::vector<std::string> comparevals,
																  int32_t limitcnt,
																  DBApi::eListSortCriteria primarysort,
																  DBApi::eListSortCriteria secondarysort,
																  int32_t sortdir, std::string orderstring,
																  int32_t startindex, int64_t startidnum,
																  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Reagents( rxlist,
									   filtertypes, compareops, comparevals,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetReagentInfoList( std::vector<DB_ReagentTypeRecord>& rxlist,
														  DBApi::eListFilterCriteria filtertype,
														  std::string compareop,
														  std::string compareval,
														  int32_t limitcnt,
														  DBApi::eListSortCriteria primarysort,
														  DBApi::eListSortCriteria secondarysort,
														  int32_t sortdir, std::string orderstring,
														  int32_t startindex, int64_t startidnum,
														  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Reagents( rxlist,
									   filtertype, compareop, compareval,
									   limitcnt,
									   primarysort, secondarysort,
									   sortdir, orderstring,
									   startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetReagentInfoByIdNum( DB_ReagentTypeRecord& rxr, int64_t rxidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindReagent( rxr, rxidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetReagentInfoBySN( DB_ReagentTypeRecord& rxr, std::string tagsn )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindReagent( rxr, tagsn );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddReagentInfo( DB_ReagentTypeRecord& rxr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddReagent( rxr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyReagentInfo( DB_ReagentTypeRecord& rxr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyReagent( rxr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveReagentInfo( DB_ReagentTypeRecord& rxr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveReagent( rxr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveReagentInfoByIdNum( int64_t rxidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveReagent( rxidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveReagentInfoBySN( std::string tagsn )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveReagent( tagsn );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveReagentInfoByIdNumList( std::vector<int64_t> idnumlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveReagents( idnumlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveReagentInfoByLotNum( std::string lotnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveReagentLot( lotnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCellHealthReagentsList(std::vector<DB_CellHealthReagentRecord>& chrlist,
	DBApi::eListFilterCriteria filtertype,
	std::string compareop,
	std::string compareval,
	int32_t limitcnt,
	DBApi::eListSortCriteria primarysort,
	DBApi::eListSortCriteria secondarysort,
	int32_t sortdir, std::string orderstring,
	int32_t startindex, int64_t startidnum,
	std::string custom_query)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return dbif.RetrieveList_CellHealthReagents(chrlist,
		filtertype, compareop, compareval,
		limitcnt,
		primarysort, secondarysort,
		sortdir, orderstring,
		startindex, startidnum, custom_query);
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCellHealthReagentByType(DB_CellHealthReagentRecord& chr, int16_t type)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return dbif.FindCellHealthReagent(chr, type);
}	

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyCellHealthReagent (DB_CellHealthReagentRecord& chr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return dbif.ModifyCellHealthReagent(chr);
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetBioProcessListEnhanced( std::vector<DB_BioProcessRecord>& bplist,
																 std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops,
																 std::vector<std::string> comparevals,
																 int32_t limitcnt,
																 DBApi::eListSortCriteria primarysort,
																 DBApi::eListSortCriteria secondarysort,
																 int32_t sortdir, std::string orderstring,
																 int32_t startindex, int64_t startidnum,
																 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_BioProcess( bplist,
										 filtertypes, compareops, comparevals,
										 limitcnt,
										 primarysort, secondarysort,
										 sortdir, orderstring,
										 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetBioProcessList( std::vector<DB_BioProcessRecord>& bplist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop,
														 std::string compareval,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum,
														 std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_BioProcess( bplist,
										 filtertype, compareop, compareval,
										 limitcnt,
										 primarysort, secondarysort,
										 sortdir, orderstring,
										 startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindBioProcessByUuid( DB_BioProcessRecord& bpr, uuid__t bpid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindBioProcess( bpr, bpid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindBioProcessByName( DB_BioProcessRecord& bpr, std::string bpname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindBioProcess( bpr, bpname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddBioProcess( DB_BioProcessRecord& bpr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddBioProcess( bpr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyBioProcess( DB_BioProcessRecord& bpr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyBioProcess( bpr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveBioProcess( DB_BioProcessRecord& bpr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveBioProcess( bpr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveBioProcessByUuid( uuid__t bpid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveBioProcess( bpid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveBioProcessByName( std::string bpname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveBioProcess( bpname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetQcProcessListEnhanced( std::vector<DB_QcProcessRecord>& qclist,
																std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort,
																DBApi::eListSortCriteria secondarysort,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum,
																std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_QcProcess( qclist,
										filtertypes, compareops, comparevals,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetQcProcessList( std::vector<DB_QcProcessRecord>& qclist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop,
														std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort,
														DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum,
														std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_QcProcess( qclist,
										filtertype, compareop, compareval,
										limitcnt,
										primarysort, secondarysort,
										sortdir, orderstring,
										startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindQcProcessByUuid( DB_QcProcessRecord& qcr, uuid__t qcid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindQcProcess( qcr, qcid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindQcProcessByName( DB_QcProcessRecord& qcr, std::string qcname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindQcProcess( qcr, qcname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddQcProcess( DB_QcProcessRecord& qcr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddQcProcess( qcr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyQcProcess( DB_QcProcessRecord& qcr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyQcProcess( qcr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveQcProcess( DB_QcProcessRecord& qcr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveQcProcess( qcr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveQcProcessByUuid( uuid__t qcid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveQcProcess( qcid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveQcProcessByName( std::string qcname )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveBioProcess( qcname );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCalibrationListEnhanced( std::vector<DB_CalibrationRecord>& callist,
																  std::vector<DBApi::eListFilterCriteria> filtertypes,
																  std::vector<std::string> compareops,
																  std::vector<std::string> comparevals,
																  int32_t limitcnt,
																  DBApi::eListSortCriteria primarysort,
																  DBApi::eListSortCriteria secondarysort,
																  int32_t sortdir, std::string orderstring,
																  int32_t startindex, int64_t startidnum,
																  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Calibration( callist,
										  filtertypes, compareops, comparevals,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetCalibrationList( std::vector<DB_CalibrationRecord>& callist,
														  DBApi::eListFilterCriteria filtertype,
														  std::string compareop,
														  std::string compareval,
														  int32_t limitcnt,
														  DBApi::eListSortCriteria primarysort,
														  DBApi::eListSortCriteria secondarysort,
														  int32_t sortdir, std::string orderstring,
														  int32_t startindex, int64_t startidnum,
														  std::string custom_query )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_Calibration( callist,
										  filtertype, compareop, compareval,
										  limitcnt,
										  primarysort, secondarysort,
										  sortdir, orderstring,
										  startindex, startidnum, custom_query );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindCalibrationByUuid( DB_CalibrationRecord& car, uuid__t calid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	return dbif.FindCalibration( car, calid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddCalibration( DB_CalibrationRecord& car )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddCalibration( car );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveCalibration( DB_CalibrationRecord& car )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveCalibration( car );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveCalibrationByUuid( uuid__t calid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveCalibration( calid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetInstrumentConfigList( std::vector<DB_InstrumentConfigRecord>& cfglist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveListInstConfig( cfglist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindInstrumentConfig( DB_InstrumentConfigRecord& icr, int64_t inst_idnum, std::string instsn )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindInstrumentConfig( icr, inst_idnum, instsn );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindInstrumentConfigBySN( DB_InstrumentConfigRecord& icr, std::string instsn )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindInstrumentConfig( icr, NO_ID_NUM, instsn );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddInstrumentConfig( icr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifyInstrumentConfig( icr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveInstrumentConfig( icr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveInstrumentConfigBySN( std::string instsn )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveInstrumentConfig( instsn );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetAuditLogList( std::vector<DB_LogEntryRecord>& loglist,
													   std::vector<DBApi::eListFilterCriteria> filtertypes,
													   std::vector<std::string> compareops,
													   std::vector<std::string> comparevals,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.RetrieveList_LogEntries( loglist,
										 filtertypes, compareops, comparevals,
										 limitcnt, primarysort, secondarysort,
										 sortdir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddAuditLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = true;

	return dbif.AddLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyAuditLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = true;

	return dbif.ModifyLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearAuditLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops,
													 std::vector<std::string> comparevals )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearAuditLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																std::vector <int64_t> idnumlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::AuditLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals, idnumlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetErrorLogList( std::vector<DB_LogEntryRecord>& loglist,
													   std::vector<DBApi::eListFilterCriteria> filtertypes,
													   std::vector<std::string> compareops,
													   std::vector<std::string> comparevals,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.RetrieveList_LogEntries( loglist,
										 filtertypes, compareops, comparevals,
										 limitcnt, primarysort, secondarysort,
										 sortdir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddErrorLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = false;

	return dbif.AddLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifyErrorLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = true;

	return dbif.ModifyLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearErrorLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops,
													  std::vector<std::string> comparevals )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearErrorLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops,
																std::vector<std::string> comparevals,
																std::vector <int64_t> idnumlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::ErrorLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals, idnumlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSampleLogList( std::vector<DB_LogEntryRecord>& loglist,
														std::vector<DBApi::eListFilterCriteria> filtertypes,
														std::vector<std::string> compareops,
														std::vector<std::string> comparevals,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort,
														DBApi::eListSortCriteria secondarysort,
														int32_t sortdir )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.RetrieveList_LogEntries( loglist,
										 filtertypes, compareops, comparevals,
										 limitcnt, primarysort, secondarysort,
										 sortdir );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSampleLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = false;

	return dbif.AddLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySampleLogEntry( DB_LogEntryRecord& log_entry )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if ( log_entry.EntryType <= static_cast<int16_t>( DBApi::eLogEntryType::AllLogTypes ) )
	{
		log_entry.EntryType = static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType );
	}
	else if ( log_entry.EntryType != static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}
	log_entry.Protected = true;

	return dbif.ModifyLogEntry( log_entry );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearSampleLog( std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops,
													  std::vector<std::string> comparevals )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbClearSampleLogByIdNumList( std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops,
																 std::vector<std::string> comparevals,
																 std::vector <int64_t> idnumlist )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	DBApi::eListFilterCriteria type = DBApi::eListFilterCriteria::LogTypeFilter;
	std::string ops = "=";
	std::string val = boost::str( boost::format( "%d" ) % static_cast<int16_t>( DBApi::eLogEntryType::SampleLogType ) );

	filtertypes.insert( filtertypes.begin(), type );			// setup the first entry in the filter list for this type of log entry
	compareops.insert( compareops.begin(), ops );				// setup the first entry in the filter list for this type of log entry
	comparevals.insert( comparevals.begin(), val );				// setup the first entry in the filter list for this type of log entry

	return dbif.ClearLogEntries( filtertypes, compareops, comparevals, idnumlist );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSchedulerConfigListEnhanced( std::vector<DB_SchedulerConfigRecord>& scheduler_list,
																	  std::vector<DBApi::eListFilterCriteria> filtertypes,
																	  std::vector<std::string> compareops,
																	  std::vector<std::string> comparevals,
																	  int32_t limitcnt,
																	  DBApi::eListSortCriteria primarysort,
																	  DBApi::eListSortCriteria secondarysort,
																	  int32_t sortdir, std::string orderstring,
																	  int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SchedulerConfig( scheduler_list,
											  filtertypes,
											  compareops, comparevals,
											  limitcnt,
											  primarysort,
											  secondarysort,
											  sortdir, orderstring,
											  startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbGetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord>& scheduler_list,
															  DBApi::eListFilterCriteria filtertype,
															  std::string compareop,
															  std::string compareval,
															  int32_t limitcnt,
															  DBApi::eListSortCriteria primarysort,
															  DBApi::eListSortCriteria secondarysort,
															  int32_t sortdir, std::string orderstring,
															  int32_t startindex, int64_t startidnum )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RetrieveList_SchedulerConfig( scheduler_list,
											  filtertype,
											  compareop, compareval,
											  limitcnt,
											  primarysort,
											  secondarysort,
											  sortdir, orderstring,
											  startindex, startidnum );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbFindSchedulerConfigByUuid( DB_SchedulerConfigRecord& scr, uuid__t scid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.FindSchedulerConfig( scr, scid );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbAddSchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.AddSchedulerConfig( scr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbModifySchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.ModifySchedulerConfig( scr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSchedulerConfig( scr );
}

HAWKEYEDBIF_API DBApi::eQueryResult DbRemoveSchedulerByUuid( uuid__t scid )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return dbif.RemoveSchedulerConfig( scid );
}


}
