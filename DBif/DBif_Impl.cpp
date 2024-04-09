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



static const std::string MODULENAME = "DBif_Impl";



const uint32_t	MinTimeStrLen = 19;												// minimum length of time strings used in database insertion statements (without timezone); ALLOWS FOR NO AM/PM indicator
const uint32_t	MaxTimeStrLen = 25;												// maximum length of time strings used in database insertion statements

const char		DBLogFile[] = "\\Instrument\\Logs\\DB_Log.txt";
const char		DBAddrStr[] = "localhost";
const char		DBPortStr[] = "5432";
const char		DBNameStr[] = "ViCellDB";										// default database name for the instrument; the base of instrument database names
const char		DBDriverStr[] = "{PostgreSQL ODBC Driver(UNICODE)}";

const char		DBDefaultUser[] = "postgres";									// default installation superuser name and maintenance database name

const char		DbUtcTimeFmtStr[] = "'%Y-%m-%d %H:%M:%S UTC'";					// formatting string used for generation of time strings used in database insertion statements as UTC times
																				// DB time string format will be "YYYY-mm-dd HH:MM:SS UTC"
																				// e.g.     "'2017-01-01 12:00:00 UTC'"
const char		DbTimeFmtStr[] = "'%Y-%m-%d %H:%M:%S'";							// formatting string used for generation of time strings used in database insertion statements
																				// DB time string format will be "YYYY-mm-dd HH:MM:SS" where TZS is the 3 digit time zone designation
																				// e.g.     "'2017-01-01 12:00:00'"
																				// this constant includes the quoting characters required when used in an SQL
																				// string insertion operation, and adds the timezone designator by default

const char		DbFloatDataFmtStr[] = "%0.8f";									// formatting string used for generation of floating point value insertion/update strings
const char		DbFloatThresholdFmtStr[] = "%0.8f";								// formatting string used for generation of floating point value insertion/update strings
const char		DbDoubleDataFmtStr[] = "%0.15f";								// formatting string used for generation of double value insertion/update strings
const char		DbDoubleExpDataFmtStr[] = "%0.10E";								// formatting string used for generation of double value insertion/update strings when stored in exponential format

const char		DBEmptyUuidStr[] = "00000000-0000-0000-0000-000000000000";		// placeholder uuid string for DB fields requiring non-null uuid entries

const char		DbDefaultInstConfigSN[] = DbDefaultConfigSN;

const char		DBtoken[] = "database";											// database token; used in SQL command string creation
const char		DBTemplateToken[] = "template";									// template token; used for construction of other DB names 

const char		LogTimeFmtStr[] = "%Y-%b-%d %H:%M:%S";							// formatting string used for generation of time strings used in database logging statements

const char		TrueStr[] = "true";
const char		FalseStr[] = "false";

const char		DropStr[] = "drop";
const char		AlterStr[] = "alter";
const char		DeleteTableStr[] = "delete from";
const char		TruncateTableStr[] = "truncate";

const char		SelectStr[] = "SELECT";
const char		WhereStr[] = "WHERE";
const char		LimitStr[] = "LIMIT";

const char		AndStr[] = "AND";
const char		FromStr[] = "from";


const int32_t	MaxDuplicateChecks = 2;											// For duplicate checks to ensure no duplicates added

const int32_t	LoginTimeout = 20;
const int32_t	TagQueryTimeout = 15;
const int32_t	ReadQueryTimeout = 30;
const int32_t	WriteQueryTimeout = 15;
const int32_t	DefaultQueryTimeout = 10;

const int32_t	DefaultQueryRecordLimit = 2000;									// allow a high enough limit to handle all image results 100 is the max images... Shepherd campaign is ~1200 samples
const int32_t	MaxQueryRecordLimit = 2500;

const uint32_t	UserCellTypeStartIndex = 0x80000000;							// decimal 2147483648U for unsigned 32-bit int


// DBifImpl  database interface
DBifImpl::DBifImpl( void )
	: adminDb()
	, userDb()
	, instrumentDb()
	, pActiveDb (nullptr)
	, UuidUtil()
	, strUsername( "" )
	//PostgreSQL ODBC Driver
	, strDBDriver( DBDriverStr )
	, strDBName( DBNameStr )
	, strDBAddr( DBAddrStr )
	, dbAddr1( 127 )
	, dbAddr2( 0 )
	, dbAddr3( 0 )
	, dbAddr4( 1 )
	, strDBPort( DBPortStr )
	, dbPortNum( 5432 )
	, activeConnection( "" )
	, path( _T( "" ) )
	, token1( _T( "BCI" ) )
	, token2( _T( "Vi" ) )
	, token3( _T( "Cell" ) )
	, token4( _T( "BLU" ) )
	, token5( _T( "FL" ) )
	, token6( _T( "Instrument" ) )
	, token7( _T( "User" ) )
	, token8( _T( "DB" ) )
	, token9( _T( "Admin" ) )
	, token10( _T( "Service" ) )
	, queryResultsIdx( 0 )
{
	DBifInit();

#ifdef USE_LOGGER
	boost::system::error_code ec;
	std::string configFilename = "\\Instrument\\Config\\HawkeyeStatic.info";
	Logger::L().Initialize( ec, configFilename, "logger" );
	Logger::L().Log( MODULENAME, severity_level::debug1, MODULENAME );
#endif
}

// DBifImpl database interface
DBifImpl::~DBifImpl( void )
{
	DoLogoutAll();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool DBifImpl::DBifInit( void )
{
	TCHAR tc[260];
	GetCurrentDirectory( 260, tc );
	path = tc;

	return true;
}

void DBifImpl::GetDBConnectionProperties( std::string& dbaddr, std::string& dbport,
										  std::string& dbname, std::string& dbdriver )
{
	dbname.clear();
	if ( strDBName.length() > 0 )
	{
		dbname = strDBName;
	}

	dbaddr.clear();
	if ( strDBAddr.length() <= 0 )
	{
		if ( ( dbAddr1 > 0 && dbAddr1 < 255 ) &&
			 ( dbAddr2 >= 0 && dbAddr2 < 255 ) &&
			 ( dbAddr3 >= 0 && dbAddr3 < 255 ) &&
			 ( dbAddr4 > 0 && dbAddr4 < 255 ) )
		{
			strDBAddr = boost::str( boost::format( "%d.%d.%d.%d" ) % dbAddr1 % dbAddr2 % dbAddr3 % dbAddr4 );
		}
	}

	if ( strDBAddr.length() > 0 )
	{
		if ( strDBAddr == "localhost" )
		{
			dbaddr = "127.0.0.1";
		}
		else
		{
			dbaddr = strDBAddr;
		}
	}

	dbport.clear();
	if ( strDBPort.length() <= 0 )
	{
		if ( dbPortNum > 0 && dbPortNum < INT16_MAX )
		{
			strDBPort = boost::str( boost::format( "%d" ) % dbPortNum );
		}
	}

	if ( strDBPort.length() > 0 )
	{
		dbport = strDBPort;
	}

	dbdriver.clear();
	if ( strDBDriver.length() > 0 )
	{
		dbdriver = strDBDriver;
	}
}

bool DBifImpl::SetDBConnectionProperties( std::string dbaddr, std::string dbport,
										  std::string dbname, std::string dbdriver )
{
	bool reconnect = false;

	if ( dbname.length() <= 0 && dbaddr.length() <= 0 && dbport.length() <= 0 && dbdriver.length() <= 0 )
	{
		return false;
	}

	if ( ( strDBName.length() > 0 && dbname.length() > 0 && strDBName != dbname ) ||
		 ( strDBAddr.length() > 0 && dbaddr.length() > 0 && strDBAddr != dbaddr ) ||
		 ( strDBPort.length() > 0 && dbport.length() > 0 && strDBPort != dbport ) ||
		 ( strDBDriver.length() > 0 && dbdriver.length() > 0 && strDBDriver != dbdriver ) )
	{
		reconnect = true;
	}

	if ( dbname.length() > 0 && ( strDBName.length() <= 0 || strDBName != dbname ) )
	{
		strDBName = dbname;
	}

	if ( dbaddr.length() > 0 && ( strDBAddr.length() <= 0 || strDBAddr != dbaddr ) )
	{
		strDBAddr = dbaddr;
	}

	if ( dbport.length() > 0 && ( strDBPort.length() <= 0 || strDBPort != dbport ) )
	{
		strDBPort = dbport;
	}

	if ( dbdriver.length() > 0 && ( strDBDriver.length() <= 0 || strDBDriver != dbdriver ) )
	{
		strDBDriver = dbdriver;
	}

	if ( activeConnection.length() > 0 )
	{
		DoLogoutAll();
	}
	return true;
}

bool DBifImpl::SetDBName( std::string dbname )
{
	if ( dbname.length() > 0 )
	{
		strDBName = dbname;
		return true;
	}
	return false;
}

bool DBifImpl::SetDBAddress( std::string addr )
{
	if ( addr.length() <= 0 )
	{
		return false;
	}

	if ( addr == "localhost" )
	{
		dbAddr1 = static_cast< uint32_t >( 127 );
		dbAddr2 = static_cast< uint32_t >( 0 );
		dbAddr3 = static_cast< uint32_t >( 0 );
		dbAddr4 = static_cast< uint32_t >( 1 );
		strDBAddr = addr;
		return true;
	}

	std::string sepStr = "";
	std::string sepChars = ".";
	char* pSepChars = ( char* ) sepChars.c_str();
	std::string trimChars = " ";
	std::vector<std::string> tokenlist;
	int32_t tokenCnt = 0;

	tokenCnt = ParseStringToTokenList( tokenlist, addr, sepStr, pSepChars, true, trimChars );

	if ( tokenCnt == 4 )
	{
		uint32_t b1 = std::stoi( tokenlist.at( 0 ) );
		uint32_t b2 = std::stoi( tokenlist.at( 1 ) );
		uint32_t b3 = std::stoi( tokenlist.at( 2 ) );
		uint32_t b4 = std::stoi( tokenlist.at( 3 ) );

		return SetValidDbIpAddr( b1, b2, b3, b4 );
	}
	return false;
}

bool DBifImpl::SetDBAddress( uint32_t addrWord )
{
	uint32_t b1;
	uint32_t b2;
	uint32_t b3;
	uint32_t b4;

	b1 = ( addrWord & 0xff000000 ) >> 24;
	b2 = ( addrWord & 0x00ff0000 ) >> 16;
	b3 = ( addrWord & 0x0000ff00 ) >> 8;
	b4 = ( addrWord & 0x000000ff );

	return SetValidDbIpAddr( b1, b2, b3, b4 );
}

bool DBifImpl::SetDBAddress( uint32_t addrWord1, uint32_t addrWord2, uint32_t addrWord3, uint32_t addrWord4 )
{
	return SetValidDbIpAddr( addrWord1, addrWord2, addrWord3, addrWord4 );
}

bool DBifImpl::SetValidDbIpAddr( uint32_t addrWord1, uint32_t addrWord2, uint32_t addrWord3, uint32_t addrWord4 )
{
	if ( ( addrWord1 > 0 && addrWord1 < 255 ) &&
		 ( addrWord2 >= 0 && addrWord2 < 255 ) &&
		 ( addrWord3 >= 0 && addrWord3 < 255 ) &&
		 ( addrWord4 > 0 && addrWord4 < 255 ) )
	{
		dbAddr1 = static_cast< uint32_t >( addrWord1 );
		dbAddr2 = static_cast< uint32_t >( addrWord2 );
		dbAddr3 = static_cast< uint32_t >( addrWord3 );
		dbAddr4 = static_cast< uint32_t >( addrWord4 );

		strDBAddr = boost::str( boost::format( "%d.%d.%d.%d" ) % addrWord1 % addrWord2 % addrWord3 % addrWord4 );
		return true;
	}
	return false;
}

bool DBifImpl::SetDBPort( std::string portstr )
{
	if ( portstr.length() > 0 )
	{
		int32_t portnum = std::stoi( portstr );

		return SetDBPort( portnum );
	}
	return false;
}

bool DBifImpl::SetDBPort( int32_t portnum )
{
	if ( portnum < INT16_MAX )
	{
		dbPortNum = portnum;
		strDBPort = boost::str( boost::format( "%d" ) % portnum );
		return true;
	}
	return false;
}

bool DBifImpl::RecreateDb( std::string dbname )
{
	return DoDbRecreate( dbname );
}

bool DBifImpl::SetBackupUserPwd( DBApi::eQueryResult & resultcode, std::string & pwdstr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = SetDbBackupUserPwd( pwdstr );
	resultcode = queryResult;

	return (queryResult == DBApi::eQueryResult::QueryOk);
}

bool DBifImpl::TruncateTableContents(DBApi::eQueryResult& resultcode, std::list<std::pair<std::string, std::string>> tableNames_name_schema)
{
	resultcode = DbTruncateTableContents(tableNames_name_schema);
	return (resultcode == DBApi::eQueryResult::QueryOk);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible retrieval, insertion, and update methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Work Queue and sample methods
////////////////////////////////////////////////////////////////////////////////

DBApi::eQueryResult DBifImpl::RetrieveList_Worklists( std::vector<DB_WorklistRecord>& wllist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  DBApi::eContainedObjectRetrieval get_sub_items, int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetWorklistList( wllist,
								   filtertypes, compareops, comparevals,
								   primarysort, secondarysort,
								   get_sub_items, orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Worklists( std::vector<DB_WorklistRecord>& wllist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  DBApi::eContainedObjectRetrieval get_sub_items, int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetWorklistList( wllist,
								   filtertype, compareop, compareval,
								   primarysort, secondarysort,
								   get_sub_items, orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindWorklist( DB_WorklistRecord& wlr, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetWorklist( wlr, worklistid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddWorklist( DB_WorklistRecord& wlr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertWorklistRecord( wlr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SaveWorklistTemplate( DB_WorklistRecord& wlr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	ClearGuid( wlr.WorklistId );
	wlr.WorklistIdNum = 0;
	wlr.WorklistStatus = static_cast<int32_t>(DBApi::eWorklistStatus::WorklistTemplate);

	queryResult = InsertWorklistRecord( wlr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertWorklistSampleSetRecord( worklistid, ssr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddWorklistSampleSet( uuid__t ssid, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertWorklistSampleSetRecord( worklistid, ssid, NO_ID_NUM );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistSampleSetRecord( worklistid, ssr.SampleSetId, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklistSampleSet( uuid__t ssid, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistSampleSetRecord( worklistid, ssid, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklistSampleSets( std::vector<uuid__t>& guidlist, uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistSampleSetRecords( worklistid, guidlist, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyWorklist( DB_WorklistRecord& wlr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateWorklistRecord( wlr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyWorklistTemplate( DB_WorklistRecord& wlr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	ClearGuid( wlr.WorklistId );
	wlr.WorklistStatus = static_cast< int32_t >( DBApi::eWorklistStatus::WorklistTemplate);

	queryResult = UpdateWorklistRecord( wlr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetWorklistStatus( uuid__t worklistid, DBApi::eWorklistStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateWorklistStatus( worklistid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateWorklistSampleSetStatus( worklistid, samplesetid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklist( DB_WorklistRecord& wlr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistRecord( wlr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklist( uuid__t worklistid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistRecord( worklistid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorklists( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteWorklistRecords( guidlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist,
													   std::vector<DBApi::eListFilterCriteria> filtertypes,
													   std::vector<std::string> compareops, std::vector<std::string> comparevals,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   DBApi::eContainedObjectRetrieval get_sub_items, int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSetList( ssrlist,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort,
									get_sub_items, orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist,
													   DBApi::eListFilterCriteria filtertype,
													   std::string compareop, std::string compareval,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   DBApi::eContainedObjectRetrieval get_sub_items, int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSetList( ssrlist,
									filtertype, compareop, compareval,
									primarysort, secondarysort,
									get_sub_items, orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, std::vector<uuid__t>& ssidlist,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   DBApi::eContainedObjectRetrieval get_sub_items, int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSetList( ssrlist, ssidlist,
									primarysort, secondarysort,
									get_sub_items, orderstring, sortdir, limitcnt,
									startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, DB_WorklistRecord& wlr,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSetList( ssrlist, wlr.WorklistId,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, uuid__t wlid,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSetList( ssrlist, wlid,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSampleSet( DB_SampleSetRecord& ssr, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleSet( ssr, samplesetid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSampleSet( DB_SampleSetRecord& ssr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSampleSetRecord( ssr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SaveSampleSetTemplate( DB_SampleSetRecord& ssr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	ClearGuid( ssr.SampleSetId );
	ssr.SampleSetIdNum = 0;
	ssr.SampleSetStatus = static_cast< int32_t >( DBApi::eSampleSetStatus::SampleSetTemplate);

	queryResult = InsertSampleSetRecord( ssr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSampleSetLineItem( DB_SampleItemRecord& ssir, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSampleSetSampleItemRecord( samplesetid, ssir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSampleSetLineItem( uuid__t ssiid, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSampleSetSampleItemRecord( samplesetid, ssiid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSetLineItem( DB_SampleItemRecord& ssir, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleSetSampleItemRecord( samplesetid, ssir.SampleSetId );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSetLineItem( uuid__t ssiid, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleSetSampleItemRecord( samplesetid, ssiid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSetLineItems( std::vector<uuid__t>& guidlist, uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;
//	queryResult = DeleteSampleSetSampleItemRecords( ssGuid, guidlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySampleSet( DB_SampleSetRecord& ssr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleSetRecord( ssr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySampleSetTemplate( DB_SampleSetRecord& ssr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

//	ssr.SampleSetStatus = static_cast< int32_t >( DBApi::eSampleSetStatus::SampleSetTemplate);

	queryResult = UpdateSampleSetRecord( ssr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleSetStatus( samplesetid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetSampleSetLineItemStatus( uuid__t samplesetid, uuid__t itemid, DBApi::eSampleItemStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleSetItemStatus( samplesetid, itemid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSet( DB_SampleSetRecord& ssr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleSetRecord( ssr, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSet( uuid__t samplesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleSetRecord( samplesetid, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleSets( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleSetRecords( guidlist, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssirlist,
														std::vector<DBApi::eListFilterCriteria> filtertypes,
														std::vector<std::string> compareops, std::vector<std::string> comparevals,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItemList( ssirlist,
									 filtertypes, compareops, comparevals,
									 primarysort, secondarysort,
									 orderstring, sortdir, limitcnt,
									 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssirlist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop, std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItemList( ssirlist,
									 filtertype, compareop, compareval,
									 primarysort, secondarysort,
									 orderstring, sortdir, limitcnt,
									 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssirlist, std::vector<uuid__t>& ssiridlist,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum )

{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItemList( ssirlist, ssiridlist,
									 primarysort, secondarysort,
									 orderstring, sortdir, limitcnt,
									 startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssirlist, DB_SampleSetRecord ssr,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItemList( ssirlist, ssr.SampleSetId,
									 primarysort, secondarysort,
									 orderstring, sortdir, limitcnt,
									 startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssirlist, uuid__t ssrid,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItemList( ssirlist, ssrid,
									 primarysort, secondarysort,
									 orderstring, sortdir, limitcnt,
									 startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSampleItem( DB_SampleItemRecord& ssir, uuid__t itemid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleItem( ssir, itemid, NO_ID_NUM );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSampleItem( DB_SampleItemRecord& ssir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSampleItemRecord( ssir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySampleItem( DB_SampleItemRecord& ssir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleItemRecord( ssir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleItemStatus( itemid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleItem( DB_SampleItemRecord& ssir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleItemRecord( ssir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleItem( uuid__t itemid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleItemRecord( itemid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSampleItems( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleItemRecords( guidlist, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Samples( std::vector<DB_SampleRecord>& samplelist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													int32_t limitcnt,
													DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													int32_t sortdir, std::string orderstring,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleList( samplelist,
								 filtertypes, compareops, comparevals,
								 primarysort, secondarysort,
								 orderstring, sortdir, limitcnt,
								 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Samples( std::vector<DB_SampleRecord>& samplelist,
													DBApi::eListFilterCriteria filtertype,
													std::string compareop, std::string compareval,
													int32_t limitcnt,
													DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													int32_t sortdir, std::string orderstring,
													int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSampleList( samplelist,
								 filtertype, compareop, compareval,
								 primarysort, secondarysort,
								 orderstring, sortdir, limitcnt,
								 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ExportSamples( std::vector<DB_SampleRecord>& samplelist,
														  std::vector<DBApi::eListFilterCriteria> filtertypes,
														  std::vector<std::string> compareops,
														  std::vector<std::string> comparevals,
														  system_TP lastruntime,
														  std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	//  check for the 'SinceLastDate.." filter type parameter and substitute the last successful run date...

	size_t filterSize = filtertypes.size();
	if ( compareops.size() != filterSize || comparevals.size() != filterSize )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::NoFilter;
	for ( size_t listIdx = 0; listIdx < filterSize; listIdx++ )
	{
		filterType = filtertypes.at( listIdx );

		if ( filterType == DBApi::eListFilterCriteria::SinceLastDateFilter )
		{
			std::string lastRunTimeStr = "";

			GetDbTimeString( lastruntime, lastRunTimeStr );

			filtertypes.at( listIdx ) = DBApi::eListFilterCriteria::RunDateFilter;
			compareops.at( listIdx ) = ">";
			comparevals.at( listIdx ) = lastRunTimeStr;
		}
	}

	queryResult = GetSampleList( samplelist,
								 filtertypes, compareops, comparevals,
								 DBApi::eListSortCriteria::RunDateSort,
								 DBApi::eListSortCriteria::SortNotDefined,
								 "", 0, DFLT_QUERY_NO_LIST_LIMIT, INVALID_INDEX, ID_SRCH_FROM_END, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSample( DB_SampleRecord& sr, uuid__t sampleid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSample( sr, sampleid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSample( DB_SampleRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSampleRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySample( DB_SampleRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::SetSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSampleStatus( sampleid, status );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSample( DB_SampleRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleRecord( sr, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSample( uuid__t sampleid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleRecord( sampleid, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSamples( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSampleRecords( guidlist, DELETE_DATA_TREE );

	return queryResult;
}


////////////////////////////////////////////////////////////////////////////////
// analysis and data methods
////////////////////////////////////////////////////////////////////////////////

DBApi::eQueryResult DBifImpl::RetrieveList_Analyses( std::vector<DB_AnalysisRecord>& anlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisList( anlist,
								   filtertypes, compareops, comparevals,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Analyses( std::vector<DB_AnalysisRecord>& anlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisList( anlist,
								   filtertype, compareop, compareval,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ExportAnalyses( std::vector<DB_AnalysisRecord>& anlist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops,
														   std::vector<std::string> comparevals,
														   system_TP lastruntime,
														   std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

//  check for the 'SinceLastDate.." filter type parameter and substitute the last successful run date...

	size_t filterSize = filtertypes.size();
	if ( compareops.size() != filterSize || comparevals.size() != filterSize )
	{
		return DBApi::eQueryResult::BadOrMissingArrayVals;
	}

	DBApi::eListFilterCriteria filterType = DBApi::eListFilterCriteria::NoFilter;
	for ( size_t listIdx = 0; listIdx < filterSize; listIdx++ )
	{
		filterType = filtertypes.at( listIdx );

		if ( filterType == DBApi::eListFilterCriteria::SinceLastDateFilter )
		{
			std::string lastRunTimeStr = "";

			GetDbTimeString( lastruntime, lastRunTimeStr );

			filtertypes.at( listIdx ) = DBApi::eListFilterCriteria::RunDateFilter;
			compareops.at( listIdx ) = ">";
			comparevals.at( listIdx ) = lastRunTimeStr;
		}
	}

	queryResult = GetAnalysisList( anlist,
								   filtertypes, compareops, comparevals,
								   DBApi::eListSortCriteria::RunDateSort,
								   DBApi::eListSortCriteria::SortNotDefined,
								   "", 0, DFLT_QUERY_NO_LIST_LIMIT, INVALID_INDEX, ID_SRCH_FROM_END, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindAnalysis( DB_AnalysisRecord& ar, uuid__t analysisid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysis( ar, analysisid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddAnalysis( DB_AnalysisRecord& ar )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertAnalysisRecord( ar );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyAnalysis( DB_AnalysisRecord& ar )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateAnalysisRecord( ar );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysis( DB_AnalysisRecord& ar )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisRecord( ar, DELETE_DATA_TREE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysis( uuid__t analysisid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisRecord( analysisid, DELETE_DATA_TREE);

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisBySampleId( uuid__t sampleid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisRecordsBySampleId( sampleid, NO_DATA_TREE_DELETE );

	return queryResult;
}

// may use SampleList, ImageSetList, AnalysisList
DBApi::eQueryResult DBifImpl::RemoveAnalysisByUuidList( std::vector<uuid__t>& idlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisRecords( idlist, NO_DATA_TREE_DELETE );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SummaryResults( std::vector<DB_SummaryResultRecord>& srlist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops, std::vector<std::string> comparevals,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSummaryResultList( srlist,
										filtertypes, compareops, comparevals,
										primarysort, secondarysort,
										orderstring, sortdir, limitcnt,
										startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SummaryResults( std::vector<DB_SummaryResultRecord>& srlist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop, std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSummaryResultList( srlist,
										filtertype, compareop, compareval,
										primarysort, secondarysort,
										orderstring, sortdir, limitcnt,
										startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSummaryResult( sr, resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSummaryResult( DB_SummaryResultRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSummaryResultRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySummaryResult( DB_SummaryResultRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSummaryResultRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSummaryResult( DB_SummaryResultRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSummaryResultRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSummaryResult( uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSummaryResultRecord( resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_DetailedResults( std::vector<DB_DetailedResultRecord>& drlist,
															std::vector<DBApi::eListFilterCriteria> filtertypes,
															std::vector<std::string> compareops, std::vector<std::string> comparevals,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetDetailedResultList( drlist,
											  filtertypes, compareops, comparevals,
											  primarysort, secondarysort,
											  orderstring, sortdir, limitcnt,
											  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_DetailedResults( std::vector<DB_DetailedResultRecord>& drlist,
															DBApi::eListFilterCriteria filtertype,
															std::string compareop, std::string compareval,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetDetailedResultList( drlist,
											  filtertype, compareop, compareval,
											  primarysort, secondarysort,
											  orderstring, sortdir, limitcnt,
											  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindDetailedResult( DB_DetailedResultRecord& dr, uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetDetailedResult( dr, resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddDetailedResult( DB_DetailedResultRecord& dr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertDetailedResultRecord( dr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveDetailedResult( DB_DetailedResultRecord& dr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteDetailedResultRecord( dr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveDetailedResult( uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteDetailedResultRecord( resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irlist,
														 std::vector<DBApi::eListFilterCriteria> filtertypes,
														 std::vector<std::string> compareops, std::vector<std::string> comparevals,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageResultList( irlist,
									  filtertypes, compareops, comparevals,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irlist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop, std::string compareval,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageResultList( irlist,
									  filtertype, compareop, compareval,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irlist, std::vector<uuid__t>& iridlist,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort,
														 DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageResultList( irlist, iridlist,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageResult( DB_ImageResultRecord& ir, uuid__t irid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageResult( ir, irid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageResult( DB_ImageResultRecord& ir, int64_t idnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpId = {};

	ClearGuid( tmpId );
	queryResult = GetImageResult( ir, tmpId, idnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImageResult( DB_ImageResultRecord& ir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertImageResultRecord( ir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageResult( DB_ImageResultRecord& ir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageResultRecord( ir, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageResult( uuid__t irid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageResultRecord( irid, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageResults( std::vector<uuid__t>& idlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageResultRecords( false, idlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SResults( std::vector<DB_SResultRecord>& srlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSResultList( srlist,
								  filtertypes, compareops, comparevals,
								  primarysort, secondarysort,
								  orderstring, sortdir, limitcnt,
								  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SResults( std::vector<DB_SResultRecord>& srlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSResultList( srlist,
								  filtertype, compareop, compareval,
								  primarysort, secondarysort,
								  orderstring, sortdir, limitcnt,
								  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSResult( DB_SResultRecord& sr, uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSResult( sr, resultid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSResult( DB_SResultRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSResultRecord( sr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSResult( DB_SResultRecord& sr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSResultRecord( sr, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSResult( uuid__t resultid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSResultRecord( resultid, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageSets( std::vector<DB_ImageSetRecord>& islist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSetList( islist,
								   filtertypes, compareops, comparevals,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageSets( std::vector<DB_ImageSetRecord>& islist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSetList( islist,
								   filtertype, compareop, compareval,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageSet( DB_ImageSetRecord& isr, uuid__t imagesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSet( isr, imagesetid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImageSet( DB_ImageSetRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertImageSetRecord( isr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyImageSet( DB_ImageSetRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateImageSetRecord( isr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSet( DB_ImageSetRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSetRecord( isr, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSet( uuid__t imagesetid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSetRecord( imagesetid, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageSequences( std::vector<DB_ImageSeqRecord>& isrlist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops, std::vector<std::string> comparevals,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSequenceList( isrlist,
										filtertypes, compareops, comparevals,
										primarysort, secondarysort,
										orderstring, sortdir, limitcnt,
										startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageSequences( std::vector<DB_ImageSeqRecord>& isrlist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop, std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSequenceList( isrlist,
										filtertype, compareop, compareval,
										primarysort, secondarysort,
										orderstring, sortdir, limitcnt,
										startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageSequence( DB_ImageSeqRecord& isr, uuid__t imageseqid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageSequence( isr, imageseqid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImageSequence( DB_ImageSeqRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertImageSeqRecord( isr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyImageSequence( DB_ImageSeqRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateImageSeqRecord( isr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSequence( DB_ImageSeqRecord& isr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSeqRecord( isr, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSequence( uuid__t imageseqid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSeqRecord( imageseqid, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSequences( std::vector<DB_ImageSeqRecord>& seqlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSeqRecords( seqlist, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageSequencesByIdList( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageSeqRecords( false, guidlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Images( std::vector<DB_ImageRecord>& imrlist,
												   std::vector<DBApi::eListFilterCriteria> filtertypes,
												   std::vector<std::string> compareops, std::vector<std::string> comparevals,
												   int32_t limitcnt,
												   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												   int32_t sortdir, std::string orderstring,
												   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageList( imrlist,
								filtertypes, compareops, comparevals,
								primarysort, secondarysort,
								orderstring, sortdir, limitcnt,
								startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Images( std::vector<DB_ImageRecord>& imrlist,
												   DBApi::eListFilterCriteria filtertype,
												   std::string compareop, std::string compareval,
												   int32_t limitcnt,
												   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												   int32_t sortdir, std::string orderstring,
												   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageList( imrlist,
								filtertype, compareop, compareval,
								primarysort, secondarysort,
								orderstring, sortdir, limitcnt,
								startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImage( DB_ImageRecord& imr, uuid__t imageid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImage( imr, imageid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImage( DB_ImageRecord& imr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertImageRecord( imr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyImage( DB_ImageRecord& ir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateImageRecord( ir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImage( DB_ImageRecord& ir )
// removes the entire image reference record; should cascade up to image sets and image records
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageRecord( ir );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImage( uuid__t imageid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageRecord( imageid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImages( std::vector<DB_ImageRecord>& imglist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageRecords( imglist, false );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImagesByIdList( std::vector<uuid__t>& guidlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageRecords( false, guidlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_CellType( std::vector<DB_CellTypeRecord>& ctlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCellTypeList( ctlist,
								   filtertypes, compareops, comparevals,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_CellType( std::vector<DB_CellTypeRecord>& ctlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCellTypeList( ctlist,
								   filtertype, compareop, compareval,
								   primarysort, secondarysort,
								   orderstring, sortdir, limitcnt,
								   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindCellType( DB_CellTypeRecord& ctr, uuid__t celltypeid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = GetCellType( ctr, celltypeid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindCellType( DB_CellTypeRecord& ctr, int64_t celltypeindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = GetCellType( ctr, tmpUuid, celltypeindex );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddCellType( DB_CellTypeRecord& ctr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;

	queryResult = InsertCellTypeRecord( ctr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyCellType( DB_CellTypeRecord& ctr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateCellTypeRecord( ctr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveCellType( DB_CellTypeRecord& ctr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteCellTypeRecord( ctr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveCellType( uuid__t celltypeid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteCellTypeRecord( celltypeid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageAnalysisParameters( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																	std::vector<DBApi::eListFilterCriteria> filtertypes,
																	std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	int32_t limitcnt,
																	DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																	int32_t sortdir, std::string orderstring,
																	int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageAnalysisParamList( aplist,
											  filtertypes, compareops, comparevals,
											  primarysort, secondarysort,
											  orderstring, sortdir, limitcnt,
											  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageAnalysisParameters( std::vector<DB_ImageAnalysisParamRecord>& aplist,
																	DBApi::eListFilterCriteria filtertype,
																	std::string compareop, std::string compareval,
																	int32_t limitcnt,
																	DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																	int32_t sortdir, std::string orderstring,
																	int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageAnalysisParamList( aplist,
											  filtertype, compareop, compareval,
											  primarysort, secondarysort,
											  orderstring, sortdir, limitcnt,
											  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageAnalysisParameter( DB_ImageAnalysisParamRecord& params, uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetImageAnalysisParam( params, paramid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertImageAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateImageAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageAnalysisParameter( DB_ImageAnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageAnalysisParameter( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteImageAnalysisParamRecord( paramid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisInputSettings( std::vector<DB_AnalysisInputSettingsRecord>& islist,
																	   std::vector<DBApi::eListFilterCriteria> filtertypes,
																	   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																	   int32_t limitcnt,
																	   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																	   int32_t sortdir, std::string orderstring,
																	   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisInputSettingsList( islist,
												filtertypes, compareops, comparevals,
												primarysort, secondarysort,
												orderstring, sortdir, limitcnt,
												startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisInputSettings( std::vector<DB_AnalysisInputSettingsRecord>& islist,
																	   DBApi::eListFilterCriteria filtertype,
																	   std::string compareop, std::string compareval,
																	   int32_t limitcnt,
																	   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																	   int32_t sortdir, std::string orderstring,
																	   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisInputSettingsList( islist,
												filtertype, compareop, compareval,
												primarysort, secondarysort,
												orderstring, sortdir, limitcnt,
												startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params, uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisInputSettings( params, paramid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertAnalysisInputSettingsRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateAnalysisInputSettingsRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisInputSettingsRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisInputSettings( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisInputSettingsRecord( paramid );

	return queryResult;
}

#if(0)
DBApi::eQueryResult DBifImpl::RetrieveList_ImageAnalysisBlobIdentParam( std::vector<DB_BlobIdentParamRecord>& islist,
																		 std::vector<DBApi::eListFilterCriteria> filtertypes,
																		 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																		 int32_t limitcnt,
																		 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																		 int32_t sortdir, std::string orderstring,
																		 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_ImageAnalysisBlobIdentParam( std::vector<DB_BlobIdentParamRecord>& islist,
																		 DBApi::eListFilterCriteria filtertype,
																		 std::string compareop, std::string compareval,
																		 int32_t limitcnt,
																		 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																		 int32_t sortdir, std::string orderstring,
																		 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params, uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveImageAnalysisBlobIdentParam( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::ApiNotSupported;

	return queryResult;
}
#endif

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisDefinitions( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																std::vector<DBApi::eListFilterCriteria> filtertypes,
																std::vector<std::string> compareops, std::vector<std::string> comparevals,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisDefinitionList( adlist,
											 filtertypes, compareops, comparevals,
											 primarysort, secondarysort,
											 orderstring, sortdir, limitcnt,
											 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisDefinitions( std::vector<DB_AnalysisDefinitionRecord>& adlist,
																DBApi::eListFilterCriteria filtertype,
																std::string compareop, std::string compareval,
																int32_t limitcnt,
																DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
																int32_t sortdir, std::string orderstring,
																int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisDefinitionList( adlist,
											 filtertype, compareop, compareval,
											 primarysort, secondarysort,
											 orderstring, sortdir, limitcnt,
											 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindAnalysisDefinition( DB_AnalysisDefinitionRecord& def, uuid__t defid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisDefinition( def, defid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindAnalysisDefinition( DB_AnalysisDefinitionRecord& def, int32_t defindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = GetAnalysisDefinition( def, tmpUuid, defindex );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertAnalysisDefinitionRecord( def );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateAnalysisDefinitionRecord( def );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisDefinition( DB_AnalysisDefinitionRecord& def )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisDefinitionRecord( def );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisDefinition( uuid__t defid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisDefinitionRecord( defid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisParameters( std::vector<DB_AnalysisParamRecord>& aplist,
															   std::vector<DBApi::eListFilterCriteria> filtertypes,
															   std::vector<std::string> compareops, std::vector<std::string> comparevals,
															   int32_t limitcnt,
															   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															   int32_t sortdir, std::string orderstring,
															   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisParamList( aplist,
										 filtertypes, compareops, comparevals,
										 primarysort, secondarysort,
										 orderstring, sortdir, limitcnt,
										 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_AnalysisParameters( std::vector<DB_AnalysisParamRecord>& aplist,
															   DBApi::eListFilterCriteria filtertype,
															   std::string compareop, std::string compareval,
															   int32_t limitcnt,
															   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															   int32_t sortdir, std::string orderstring,
															   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisParamList( aplist,
										 filtertype, compareop, compareval,
										 primarysort, secondarysort,
										 orderstring, sortdir, limitcnt,
										 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindAnalysisParameter( DB_AnalysisParamRecord& params, uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetAnalysisParam( params, paramid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddAnalysisParameter( DB_AnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyAnalysisParameter( DB_AnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisParameter( DB_AnalysisParamRecord& params )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisParamRecord( params );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveAnalysisParameter( uuid__t paramid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteAnalysisParamRecord( paramid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Illuminators( std::vector<DB_IlluminatorRecord>& illist,
														 std::vector<DBApi::eListFilterCriteria> filtertypes,
														 std::vector<std::string> compareops, std::vector<std::string> comparevals,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetIlluminatorList( illist,
									  filtertypes, compareops, comparevals,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Illuminators( std::vector<DB_IlluminatorRecord>& illist,
														 DBApi::eListFilterCriteria filtertype,
														 std::string compareop, std::string compareval,
														 int32_t limitcnt,
														 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														 int32_t sortdir, std::string orderstring,
														 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetIlluminatorList( illist,
									  filtertype, compareop, compareval,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindIlluminator( DB_IlluminatorRecord& ilr, std::string ilname, int16_t ilindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetIlluminator( ilr, ilname, ilindex );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindIlluminator( DB_IlluminatorRecord& ilr, int16_t emission, int16_t illuminator )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetIlluminator( ilr, emission, illuminator );

	return queryResult;
}


DBApi::eQueryResult DBifImpl::AddIlluminator( DB_IlluminatorRecord& ilr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertIlluminatorRecord( ilr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyIlluminator( DB_IlluminatorRecord& ilr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateIlluminatorRecord( ilr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveIlluminator( DB_IlluminatorRecord& ilr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteIlluninatorRecord( ilr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveIlluminator( int16_t ilindex, int64_t ilidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteIlluninatorRecord( "", ilindex, ilidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveIlluminator( std::string ilname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteIlluninatorRecord( ilname, INVALID_INDEX, NO_ID_NUM );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Users( std::vector<DB_UserRecord>& urlist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  int32_t limitcnt,
												  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												  int32_t sortdir, std::string orderstring,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserList( urlist,
							   filtertypes, compareops, comparevals,
							   primarysort, secondarysort,
							   orderstring, sortdir, limitcnt,
							   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Users( std::vector<DB_UserRecord>& urlist,
												  DBApi::eListFilterCriteria filtertype,
												  std::string compareop, std::string compareval,
												  int32_t limitcnt,
												  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												  int32_t sortdir, std::string orderstring,
												  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserList( urlist,
							   filtertype, compareop, compareval,
							   primarysort, secondarysort,
							   orderstring, sortdir, limitcnt,
							   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUser( DB_UserRecord& ur, uuid__t userid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUser( ur, userid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUser( DB_UserRecord& ur, std::string username, DBApi::eUserType user_type )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = GetUser( ur, tmpUuid, username, user_type );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddUser( DB_UserRecord& ur )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertUserRecord( ur );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyUser( DB_UserRecord& ur )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateUserRecord( ur );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUser( DB_UserRecord& ur )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteUserRecord( ur );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUser( uuid__t userid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteUserRecord( userid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUser( std::string username )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = DeleteUserRecord( tmpUuid, username );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserRoles( std::vector<DB_UserRoleRecord>& rrlist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetRoleList( rrlist,
							   filtertypes, compareops, comparevals,
							   primarysort, secondarysort,
							   orderstring, sortdir, limitcnt,
							   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserRoles( std::vector<DB_UserRoleRecord>& rrlist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetRoleList( rrlist,
							   filtertype, compareop, compareval,
							   primarysort, secondarysort,
							   orderstring, sortdir, limitcnt,
							   startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUserRole( DB_UserRoleRecord& rr, uuid__t roleid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetRole( rr, roleid, "" );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUserRole( DB_UserRoleRecord& rr, std::string rolename )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = GetRole( rr, tmpUuid, rolename );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddUserRole( DB_UserRoleRecord& rr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertRoleRecord( rr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyUserRole( DB_UserRoleRecord& rr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateRoleRecord( rr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserRole( DB_UserRoleRecord& rr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteRoleRecord( rr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserRole( uuid__t roleid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteRoleRecord( roleid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserRole( std::string rolename )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = DeleteRoleRecord( tmpUuid, rolename );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops, std::vector<std::string> comparevals,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserPropertiesList( uplist,
										 filtertypes, compareops, comparevals,
										 primarysort, secondarysort,
										 orderstring, sortdir, limitcnt,
										 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop, std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserPropertiesList( uplist,
										 filtertype, compareop, compareval,
										 primarysort, secondarysort,
										 orderstring, sortdir, limitcnt,
										 startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t userid,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserPropertiesList( uplist, userid,
											primarysort, secondarysort,
											orderstring, sortdir, limitcnt,
											startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist, std::string username,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserPropertiesList( uplist, username,
										 primarysort, secondarysort,
										 orderstring, sortdir, limitcnt,
										 startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUserProperty( DB_UserPropertiesRecord& up, int32_t propindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserProperty( up, propindex );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindUserProperty( DB_UserPropertiesRecord& up, std::string propname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetUserProperty( up, INVALID_INDEX, propname );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddUserProperty( DB_UserPropertiesRecord& up )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertUserPropertyRecord( up );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserProperty( DB_UserPropertiesRecord& up )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteUserPropertyRecord( up );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserProperty( int32_t propindex )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteUserPropertyRecord( propindex );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveUserProperty( std::string propname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteUserPropertyRecord( INVALID_INDEX, propname );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SignatureDefs( std::vector<DB_SignatureRecord>& siglist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops, std::vector<std::string> comparevals,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSignatureList( siglist,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SignatureDefs( std::vector<DB_SignatureRecord>& siglist,
														   DBApi::eListFilterCriteria filtertype,
														   std::string compareop, std::string compareval,
														   int32_t limitcnt,
														   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														   int32_t sortdir, std::string orderstring,
														   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSignatureList( siglist,
									filtertype, compareop, compareval,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSignatureDef( DB_SignatureRecord& sigrec, uuid__t sigid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSignature( sigrec, sigid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSignatureDef( DB_SignatureRecord& sigrec )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSignatureRecord( sigrec );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSignatureDef( DB_SignatureRecord& sigrec )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSignatureRecord( sigrec );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSignatureDef( uuid__t sigid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSignatureRecord( sigid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Reagents( std::vector<DB_ReagentTypeRecord>& rxlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetReagentTypeList( rxlist,
									  filtertypes, compareops, comparevals,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Reagents( std::vector<DB_ReagentTypeRecord>& rxlist,
													 DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval,
													 int32_t limitcnt,
													 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													 int32_t sortdir, std::string orderstring,
													 int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetReagentTypeList( rxlist,
									  filtertype, compareop, compareval,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );


	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindReagent( DB_ReagentTypeRecord& rxr, int64_t rxidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetReagentType( rxr, rxidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindReagent( DB_ReagentTypeRecord& rxr, std::string tagsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetReagentType( rxr, NO_ID_NUM, tagsn );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddReagent( DB_ReagentTypeRecord& rxr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertReagentTypeRecord( rxr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyReagent( DB_ReagentTypeRecord& rxr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateReagentTypeRecord( rxr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagent( DB_ReagentTypeRecord& rxr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeRecord( rxr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagent( int64_t rxidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeRecord( rxidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagent( std::string tagsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeRecord( tagsn );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagents( std::vector<int64_t> idnumlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeRecords( idnumlist );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagents( std::string tagsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeRecords( tagsn );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveReagentLot( std::string lotnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteReagentTypeLotRecords( lotnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_CellHealthReagents(std::vector<DB_CellHealthReagentRecord>& chrlist,
	DBApi::eListFilterCriteria filtertype,
	std::string compareop, std::string compareval,
	int32_t limitcnt,
	DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
	int32_t sortdir, std::string orderstring,
	int32_t startindex, int64_t startidnum, std::string custom_query)
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCellHealthReagentsList(chrlist,
		filtertype, compareop, compareval,
		primarysort, secondarysort,
		orderstring, sortdir, limitcnt,
		startindex, startidnum, custom_query);

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindCellHealthReagent(DB_CellHealthReagentRecord& chr, int16_t type)
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCellHealthReagent(chr, type);

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyCellHealthReagent(DB_CellHealthReagentRecord& rxr)
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateCellHealthReagentRecord(rxr);

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Workflows( std::vector<DB_WorkflowRecord>& wflist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Workflows( std::vector<DB_WorkflowRecord>& wflist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindWorkflow( DB_WorkflowRecord& wfr, uuid__t rxid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindWorkflow( DB_WorkflowRecord& wfr, std::string wfname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddWorkflow( DB_WorkflowRecord& wfr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyWorkflow( DB_WorkflowRecord& wfr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorkflow( DB_WorkflowRecord& wfr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorkflow( uuid__t wfid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveWorkflow( std::string wfname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_BioProcess( std::vector<DB_BioProcessRecord>& bplist,
													   std::vector<DBApi::eListFilterCriteria> filtertypes,
													   std::vector<std::string> compareops, std::vector<std::string> comparevals,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_BioProcess( std::vector<DB_BioProcessRecord>& bplist,
													   DBApi::eListFilterCriteria filtertype,
													   std::string compareop, std::string compareval,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir, std::string orderstring,
													   int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindBioProcess( DB_BioProcessRecord& bpr, uuid__t bpid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindBioProcess( DB_BioProcessRecord& bpr, std::string bpname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddBioProcess( DB_BioProcessRecord& bpr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyBioProcess( DB_BioProcessRecord& bpr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveBioProcess( DB_BioProcessRecord& bpr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveBioProcess( uuid__t bpid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveBioProcess( std::string bpname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_QcProcess( std::vector<DB_QcProcessRecord>& qclist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetQcProcessList( qclist,
									filtertypes, compareops, comparevals,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_QcProcess( std::vector<DB_QcProcessRecord>& qclist,
													  DBApi::eListFilterCriteria filtertype,
													  std::string compareop, std::string compareval,
													  int32_t limitcnt,
													  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
													  int32_t sortdir, std::string orderstring,
													  int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetQcProcessList( qclist,
									filtertype, compareop, compareval,
									primarysort, secondarysort,
									orderstring, sortdir, limitcnt,
									startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindQcProcess( DB_QcProcessRecord& qcr, uuid__t qcid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetQcProcess( qcr, qcid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindQcProcess( DB_QcProcessRecord& qcr, std::string qcname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	uuid__t tmpUuid;

	ClearGuid( tmpUuid );
	queryResult = GetQcProcess( qcr, tmpUuid, NO_ID_NUM, qcname );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddQcProcess( DB_QcProcessRecord& qcr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertQcProcessRecord( qcr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyQcProcess( DB_QcProcessRecord& qcr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateQcProcessRecord( qcr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveQcProcess( DB_QcProcessRecord& qcr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteQcProcessRecord( qcr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveQcProcess( uuid__t qcid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteQcProcessRecord( qcid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveQcProcess( std::string qcname )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DBApi::eQueryResult::InternalNotSupported;

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Calibration( std::vector<DB_CalibrationRecord>& callist,
														std::vector<DBApi::eListFilterCriteria> filtertypes,
														std::vector<std::string> compareops, std::vector<std::string> comparevals,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCalibrationList( callist,
									  filtertypes, compareops, comparevals,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_Calibration( std::vector<DB_CalibrationRecord>& callist,
														DBApi::eListFilterCriteria filtertype,
														std::string compareop, std::string compareval,
														int32_t limitcnt,
														DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
														int32_t sortdir, std::string orderstring,
														int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCalibrationList( callist,
									  filtertype, compareop, compareval,
									  primarysort, secondarysort,
									  orderstring, sortdir, limitcnt,
									  startindex, startidnum, custom_query );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindCalibration( DB_CalibrationRecord& car, uuid__t calid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetCalibration( car, calid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddCalibration( DB_CalibrationRecord& car )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertCalibrationRecord( car );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveCalibration( DB_CalibrationRecord& car )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteCalibrationRecord( car );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveCalibration( uuid__t calid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteCalibrationRecord( calid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveListInstConfig( std::vector<DB_InstrumentConfigRecord>& config_list )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetInstConfigList( config_list );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindInstrumentConfig( DB_InstrumentConfigRecord& icr, int64_t inst_idnum, std::string instsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetInstConfig( icr, instsn, inst_idnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertInstConfigRecord( icr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateInstConfigRecord( icr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveInstrumentConfig( DB_InstrumentConfigRecord& icr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteInstConfigRecord( icr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveInstrumentConfig( std::string instsn )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteInstConfigRecord( instsn );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_LogEntries( std::vector<DB_LogEntryRecord>& log_list,
													   std::vector<DBApi::eListFilterCriteria> filtertypes,
													   std::vector<std::string> compareops,
													   std::vector<std::string> comparevals,
													   int32_t limitcnt,
													   DBApi::eListSortCriteria primarysort,
													   DBApi::eListSortCriteria secondarysort,
													   int32_t sortdir )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetLogList( log_list,
							  filtertypes, compareops, comparevals,
							  primarysort, secondarysort,
							  sortdir, limitcnt );

	return queryResult;
}

DBApi::eQueryResult	DBifImpl::AddLogEntry( DB_LogEntryRecord& log_entry )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertLogEntryRecord( log_entry );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifyLogEntry( DB_LogEntryRecord& log_entry )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateLogEntry( log_entry );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ClearLogEntries( std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   int64_t startidnum, int64_t endidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DB_LogEntryRecord> logList = {};

	queryResult = GetLogList( logList,
							  filtertypes, compareops, comparevals,
							  DBApi::eListSortCriteria::NoSort,
							  DBApi::eListSortCriteria::NoSort,
							  0, DFLT_QUERY_NO_LIST_LIMIT );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		DeleteLogEntryRecords( logList );
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ClearLogEntries( std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   std::vector<int64_t> idnumlist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		DeleteLogEntryRecords( idnumlist );
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SchedulerConfig( std::vector<DB_SchedulerConfigRecord>& sclist,
															std::vector<DBApi::eListFilterCriteria> filtertypes,
															std::vector<std::string> compareops, std::vector<std::string> comparevals,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSchedulerConfigList( sclist,
										  filtertypes, compareops, comparevals,
										  primarysort, secondarysort,
										  orderstring, sortdir, limitcnt,
										  startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RetrieveList_SchedulerConfig( std::vector<DB_SchedulerConfigRecord>& sclist,
															DBApi::eListFilterCriteria filtertype,
															std::string compareop, std::string compareval,
															int32_t limitcnt,
															DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
															int32_t sortdir, std::string orderstring,
															int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSchedulerConfigList( sclist,
										  filtertype, compareop, compareval,
										  primarysort, secondarysort,
										  orderstring, sortdir, limitcnt,
										  startindex, startidnum );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::FindSchedulerConfig( DB_SchedulerConfigRecord& scr, uuid__t scid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = GetSchedulerConfig( scr, scid );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::AddSchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = InsertSchedulerConfigRecord( scr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::ModifySchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = UpdateSchedulerConfigRecord( scr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSchedulerConfig( DB_SchedulerConfigRecord& scr )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSchedulerConfigRecord( scr );

	return queryResult;
}

DBApi::eQueryResult DBifImpl::RemoveSchedulerConfig( uuid__t scid )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	queryResult = DeleteSchedulerConfigRecord( scid );

	return queryResult;
}

