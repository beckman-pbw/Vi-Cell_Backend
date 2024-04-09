// Database interface : header file
//

#pragma once

#include "pch.h"


#include <guiddef.h>
#include <stdint.h>
#include <string>
#include <vector>


#include "ChronoUtilities.hpp"
#include "Configuration.hpp"
#include "DBif_Structs.hpp"
#include "DBif_QueryEnum.hpp"
#include "HawkeyeUUID.hpp"
#include "uuid__t.hpp"


#define	IR_COMPOSITE_FORMAT					0		// storage format specifier for image results
#define	IR_COMPOSITE_STRING_FORMAT			1		// storage format specifier for image results
#define	IR_COMPOSITE_LISTS_FORMAT			2		// storage format specifier for image results
#define	IR_COMPOSITE_STRING_LISTS_FORMAT	3		// storage format specifier for image results

#define	ALLOW_ID_FAILS
#define HANDLE_TRANSACTIONS
//#define USE_BOOST_FORMATTING
//#define CONFIRM_AFTER_WRITE
#define	VECTOR_FIX
//#define	RUN_DATE_FIX


extern const uint32_t	MinTimeStrLen;				// minimum length of time strings used in database insertion statements (without timezone)
extern const uint32_t	MaxTimeStrLen;				// maximum length of time strings used in database insertion statements

extern const char		DBLogFile[];

extern const char		DBAddrStr[];				// default address string for the target database
extern const char		DBPortStr[];				// default listening port number for target database
extern const char		DBNameStr[];				// default database name for the instrument; the base of instrument database names
extern const char		DBDriverStr[];				// default connection driver string

extern const char		DBDefaultUser[];			// default installation superuser name and maintenance database name

extern const char		DbUtcTimeFmtStr[];			// formatting string used for generation of time strings used in database insertion statements as UTC times
													// DB time string format will be "YYYY-mm-dd HH:MM:SS UTC"
													// e.g.     "'2017-01-01 12:00:00 UTC'"
extern const char		DbTimeFmtStr[];				// formatting string used for generation of time strings used in database insertion statements
													// DB time string format will be "YYYY-mm-dd HH:MM:SS" and DOES NOT contain a time zone designator
													// e.g.     "'2017-01-01 12:00:00'"
													// this constant includes the quoting characters required when used in an SQL
													// string insertion operation, and adds the timezone designator by default

extern const char		DbFloatDataFmtStr[];		// formatting string used for generation of floating point value insertion/update strings
extern const char		DbFloatThresholdFmtStr[];	// formatting string used for generation of floating point value insertion/update strings
extern const char		DbDoubleDataFmtStr[];		// formatting string used for generation of double value insertion/update strings
extern const char		DbDoubleExpDataFmtStr[];	// formatting string used for generation of double value insertion/update strings when stored in exponential format

extern const char		DBEmptyUuidStr[];			// placeholder uuid string for DB fields requiring non-null uuid entries

extern const char		DbDefaultInstConfigSN[];

extern const char		DBtoken[];					// database token; used in SQL command string creation
extern const char		DBTemplateToken[];			// template token; used for construction of other DB names 

extern const char		LogTimeFmtStr[];			// formatting string used for generation of time strings used in database logging statements

extern const char		TrueStr[];
extern const char		FalseStr[];

extern const char		DropStr[];
extern const char		CreateStr[];
extern const char		AlterStr[];
extern const char		DeleteTableStr[];
extern const char		TruncateTableStr[];

extern const char		SelectStr[];
extern const char		UpdateStr[];
extern const char		WhereStr[];
extern const char		LimitStr[];

extern const char		AndStr[];
extern const char		FromStr[];


extern const int32_t	MaxDuplicateChecks;			// For duplicate checks to ensure no duplicates added

extern const int32_t	LoginTimeout;
extern const int32_t	TagQueryTimeout;
extern const int32_t	ReadQueryTimeout;
extern const int32_t	DefaultQueryTimeout;
extern const int32_t	DefaultQueryRecordLimit;
extern const int32_t	MaxQueryRecordLimit;

extern const uint32_t	UserCellTypeStartIndex;


typedef std::vector<std::string>    StringList;

enum TagSearchState : int32_t
{
	OperationFailure = -10,
	BadCriticalObjectID = -9,
	MissingListItemValue = -8,
	MissingCriticalObjectID = -7,
	ItemValCreateError = -6,
	ListItemNotFound = -5,
	SetItemListCnt = -4,
	NoItemListCnt = -3,
	ListCntIdxNotSet = -2,
	NoObjectTagsFound = -1,
	TagsOk = 0,
	ListItemsOk = TagsOk,
	ItemListCntOk = TagsOk,
	ItemTagIdxOk = TagsOk,
};

enum LogMsgType : int32_t
{
	NoMsgType = 0,
	InfoMsgType,
	WarningMsgType,
	InputErrorMsgType,
	QueryErrorMsgType,
	ErrorMsgType,
	UndefinedMsgType
};

#define	INVALID_ID_NUM					 0
#define	NO_SQL_QUERY_LIMIT				 0		// used internally to limit query retrieval; anything greater than 0 will add a 'LIMIT' statement to the query string

#define	FORMAT_FOR_UPDATE				true
#define	FORMAT_FOR_INSERT				false

#define DO_SAMPLE_SET_UPDATES			true
#define NO_SAMPLE_SET_UPDATES			false
#define DO_SAMPLE_ITEM_UPDATES			true
#define NO_SAMPLE_ITEM_UPDATES			false
#define DO_PARAM_UPDATES				true
#define NO_PARAM_UPDATES				false

#define DELETE_PROTECTED_ALLOWED		true
#define DELETE_PROTECTED_NOT_ALLOWED	false

#define EMPTY_ID_ALLOWED				true
#define EMPTY_ID_NOT_ALLOWED			false

#define ID_UPDATE_ALLOWED				true
#define ID_UPDATE_NOT_ALLOWED			false

#define DELETE_DATA_TREE				true
#define NO_DATA_TREE_DELETE				false

using namespace DBApi;


// DBif database interface implementation
class DBifImpl
{
public:
	// Construction
	DBifImpl( void );
	~DBifImpl( void );


private:
	CDatabase   adminDb;
	CDatabase   userDb;
	CDatabase   instrumentDb;
	CDatabase * pActiveDb;

	HawkeyeUUID	UuidUtil;

	std::string strUsername;
	std::string strDBDriver;
	std::string strDBName;				// initialized by default to DBNameStr; this will be overwritten by the value supplied at initialization
	std::string strDBAddr;				// initialized by default to "127.0.0.1"; this will be overwritten by the value supplied at initialization
	uint32_t dbAddr1;					// constructed from the supplied or default DB addr string; initially 127
	uint32_t dbAddr2;					// constructed from the supplied or default DB addr string; initially 0
	uint32_t dbAddr3;					// constructed from the supplied or default DB addr string; initially 0
	uint32_t dbAddr4;					// constructed from the supplied or default DB addr string; initially 1
	std::string strDBPort;				// initialized by default to "5432"; this will be overwritten by the value supplied at initialization
	int32_t	dbPortNum;					// constructed from the supplied or default DB port string; initially 5432
	std::string activeConnection;
	CString path;
	CString token1;
	CString token2;
	CString token3;
	CString token4;
	CString token5;
	CString token6;
	CString token7;
	CString token8;
	CString token9;
	CString token10;

	int32_t	queryResultsIdx;


	// Implementation
private:
	bool    SetValidDbIpAddr( uint32_t addrWord1 = 0, uint32_t addrWord2 = 0, uint32_t addrWord3 = 0, uint32_t addrWord4 = 0 );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  utility support methods; DBif_UtilsData.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void    ClearGuid( GUID& gval );
	void    ClearGuid( uuid__t& uval );
	void    GUID_from_DB_UUID_Str( std::string& uuid_str, GUID& guidval );
	void	GUID_to_DB_UUID_Str( GUID& gval, std::string& guidstr );		// formats a GUID string and adds the surrounding single quotes required by the DB for single uuid insertions
	void    uuid__t_from_DB_UUID_Str( std::string& uuid_str, uuid__t& g );
	void    uuid__t_to_DB_UUID_Str( uuid__t& g, std::string& guidstr );		// formats a uuid string and adds the surrounding single quotes required by the DB for single uuid insertions
	void    FormatGuidString( GUID& gval, std::string& gvalstr );			// takes a GUID value and produces a standard-format string representation without surrounding quotes
																			// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
	void    FormatGuidString( uuid__t& uval, std::string& gvalstr );		// takes a uuid__t value and produces a standard-format string representation without surrounding quotes
																			// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
	void    uuid__t_to_GUID( uuid__t& g, GUID& guidval );
	void    GUID_to_uuid__t( GUID& guidval, uuid__t& g );
	bool    IsUuid__tValid( uuid__t& u );
	bool    GuidValid( uuid__t& g );
	bool    GuidValid( GUID& gval );
	DBApi::eQueryResult	GenerateValidGuid( uuid__t& recid );
	bool    GuidsEqual( uuid__t g1, uuid__t g2 );
	bool    GuidsEqual( GUID gval1, GUID gval2 );
	bool    GuidInsertCheck( uuid__t recid, std::string& idstr, int32_t errcode, bool allowempty = EMPTY_ID_NOT_ALLOWED );
	bool    GuidUpdateCheck( uuid__t dbid, uuid__t recid, std::string& idstr, int32_t errcode, bool allow_update = ID_UPDATE_NOT_ALLOWED, bool allow_empty = EMPTY_ID_NOT_ALLOWED );
	int32_t	TokenizeStr( std::vector<std::string>& tokenList, std::string& parsestr, std::string sepstr = "", char* sepchars = nullptr );
	void    RemoveSubStrFromStr( std::string& tgtstr, std::string substr );
	void    RemoveTgtCharFromStr( std::string& tgtstr, char tgtchar );
	void    TrimStr( std::string trim_chars, std::string& tgtstr );
	void    TrimWhiteSpace( std::string& tgtstr );
	void    StrToLower( std::string& tgtstr );
	void    StrToUpper( std::string& tgtstr );
	void	TrimSlashFromStr( std::string& valuestr, std::string trimstr );
	void	DoStringSanitize( std::string& valuestr, std::string chkchars, bool dofilter = false );
	bool	IsFilterTerminatingCharacter( char testchar );
	void	SanitizeFilterString( std::string& valuestr );
	void	SanitizePathString( std::string& valuestr );
	void	SanitizeDataString( std::string& valuestr );
	void	SanitizeDataStringList( std::vector<std::string> srclist, std::vector<std::string>& destlist );
	void	DeSanitizePathString( std::string& valuestr );
	void	DeSanitizeDataString( std::string& valuestr );
	void	DeSanitizeDataStringList( std::vector<std::string> srclist, std::vector<std::string>& destlist );

	int32_t	ParseStringToTokenList( std::vector<std::string>& tokenlist, std::string& parsestr,
									std::string sepstr = "", char* sepcchars = nullptr,
									bool dotrim = false, std::string trimchars = "" );
	int32_t	ParseStrToStrList( std::vector<std::string>& strlist, std::string& parsestr );
	int32_t ParseMultiStrArrayStrToStrList( std::vector<std::string>& subarraystrlist, std::string& arraystr );		// for multi-dimensional arrays of string values; does not remove embedded space characters
	int32_t	ParseStrArrayStrToStrList( std::vector<std::string>& arraystrlist, std::string& arraystr );				// for arrays of string values; does not remove embedded space characters
	int32_t	ParseMultiArrayStrToStrList( std::vector<std::string>& subarraylList, std::string& arraystr );			// for multi-dimensional arrays of non-string values
	int32_t	ParseArrayStrToStrList( std::vector<std::string>& arraystrlist, std::string& arraystr );				// for of arrays of non-string values
	int32_t ParseCompositeArrayStrToStrList( std::vector<std::string>& subarraystrlist, std::string& arraystr );
	int32_t ParseCompositeElementArrayStrToStrList( std::vector<std::string>& elementarraystrlist, std::string& elementarraystr );
	int32_t ParseBlobCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseClusterCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseSignatureCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseADSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseAFSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseEmailSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseRfidSimCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseLanguageInfoCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseRunOptionsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseConsumablesCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );
	int32_t ParseIlluminatorInfoCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr );

	void	WriteLogEntry( std::string entry_str, LogMsgType msgType = InfoMsgType );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB connect and query admin support; DBif_Connect.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool    DoConnect( CDatabase* pDB, std::string username, std::string password );
	void	DoDisconnect( CDatabase* pDB );
	void    DoLogoutAll( void );
	bool	DoLogoutType( DBApi::eLoginType usertype );

	DBApi::eLoginType	GetDBLoginConnection( CDatabase*& pDb, DBApi::eLoginType usertype );         // selects the connection type to use and returns the pointer to the database object selected


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB query creation support; DBif_UtilsQuery.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// determine if query is a select operation
	bool    IsSelect( std::string sqlstr );
	// determine if query requires admin privileges
	bool    IsAdminQuery( std::string sqlstr );

	// begin a complex transaction containing multiple actions
	void	BeginTransaction( DBApi::eLoginType usertype );												// require user type so admin-level transactions are apecified and limited
	// End a complex transaction containing multiple actions and commit the changes to the database
	void	EndTransaction( DBApi::eLoginType usertype = DBApi::eLoginType::AnyLoginType );
	// cancel an ongoing transaction and rollback any changes specified by the transaction
	void	CancelTransaction( DBApi::eLoginType usertype = DBApi::eLoginType::AnyLoginType );

	// correctly formats value-names and values for addition to UPDATE or INSERT strings
	// used by table manipulation functions
	void    AddToInsertUpdateString( bool format_for_update, int32_t paramNum,
									 std::string& namesstring, std::string& valuesstring,
									 std::string tagstr, std::string valuestr );

	// creates a formatted insert or update query string using the supplied object, target tablename,
	// and data values (key-value pair sets)
	//
	// for updates, guuid, numeric, or string key/match identifier values are handled;
	//
	// for inserts, takes a string defining the column identifier tags and a second string containing the associated column data values;
	//    e.g. tag string:   "tag1, tag2, tag3,..."
	//         value string: "value1,m value2, value3,..."
	// for updates, takes a single string formatted as "columntag1 = datavalue1, columntag2 = datavalue2, ...", for all columns to be updated 
	// return success = 1, error/failure = -1
	int32_t	MakeObjectInsertUpdateQuery( bool format_for_update, std::string& querystring,
										 std::string& schemaname, std::string& tablename,
										 uuid__t objuuid, int64_t objidnum, std::string idnamestr,
										 std::string keynamestr, std::string keymatchstr,
										 std::string coltagstr, std::string colvaluesstr );

	// creates a formatted insert query string using the object column tags and data values supplied,
	// and the target table name and schema name supplied
	// insert operations NEVER use a guid or an id number
	// return success = 1, error/failure = -1
	int32_t	MakeObjectAddQuery( std::string& insertstr, std::string& schemaname, std::string& tablename,
								std::string coltagstr, std::string colvaluesstr );

	// creates a formatted update query string using the column tags and data values supplied,
	// and the target table name and schema name supplied.
	// handles tables with guid identifiers, or numeric IDs as auto-generated by the DB;
	// identifier id priority is given to uuids
	// return success = 1, error/failure = -1
	int32_t	MakeObjectUpdateQuery( std::string& updatestring, std::string& schemaname, std::string& tablename,
								   uuid__t objuuid, int64_t objidnum, std::string idnamestr,
								   std::string keyname, std::string keymatch, std::string colvaluesstr );

	// creates a formatted insert query string for multiple column values using the column name tag string and
	// column values string supplied, and the target schema and tablename supplied
	// return success = QueryOk, error/failure = NoQuery (-5)
	void    MakeColumnValuesInsertUpdateQueryString( int32_t tagcount, std::string& querystr,
													 std::string schemaname, std::string tablename,
													 std::string namesstr, std::string valuesstr,
													 int64_t itemidnum, std::string idnumnamestr,
													 bool format_for_update = FORMAT_FOR_INSERT );

	// creates a formatted insert query string for multiple column values using the column name tag string and
	// column values string supplied, and the target schema and tablename supplied
	// return success = QueryOk, error/failure = NoQuery (-5)
	void    MakeColumnValuesInsertUpdateQueryString( int32_t tagcount, std::string& querystr,
													 std::string schemaname, std::string tablename,
													 std::string namesstr, std::string valuesstr,
													 uuid__t itemuuid, std::string guidnamestr,
													 bool update = FORMAT_FOR_INSERT );

	void    MakeColumnValuesInsertUpdateQueryString( int32_t tagCount, std::string& querystr,
													 std::string schemaname, std::string tablename,
													 std::string namesstr, std::string valuesstr,
													 std::string idnamestr, std::string idvaluestr,
													 bool update = FORMAT_FOR_INSERT );

	// creates a formatted deletion query string using the object id or match string supplied, and the target tablename supplied
	// return success = true, error/failure = false
	bool	MakeObjectRemoveQuery( std::string& deletestring,
								   std::string& schemaname, std::string& tablename,
								   uuid__t objuuid = {}, int64_t objidnum = NO_ID_NUM,
								   int64_t objindex = INVALID_INDEX, std::string objname = "",
								   std::string idnamestr = "", std::string wherestr = "",
								   bool deleteprotected = DELETE_PROTECTED_NOT_ALLOWED );

	// create the query string to delete objects from a list of UUIDs
	DBApi::eQueryResult	MakeObjectIdListRemoveQuery( std::string& deletestring,
													 std::string& schemaname, std::string& tablename,
													 std::string idtagstr, std::vector<uuid__t> id_list,
													 std::string wherestr = "",
													 bool deleteprotected = DELETE_PROTECTED_NOT_ALLOWED );

	// create the query string to delete objects from a list of idnums
	DBApi::eQueryResult MakeObjectIdNumListRemoveQuery( std::string& deletestr,
														std::string& schemaname, std::string& tablename,
														std::string idtagstr, std::vector<int64_t> idnum_list,
														std::string wherestr, bool deleteprotected );

// for future use by clients requiring formatted DB query string for direct DBif access
	DBApi::eQueryResult MakeFilterSortQuery( std::string& query_str,
											DBApi::eListType listtype,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );


	int32_t	GetRecordColumnTags( CRecordset& rs, std::vector<std::string>& taglist );
	bool	GetListQueryTags( DBApi::eListType listtype, std::string& schemaname, std::string& tablename,
							  std::string& selecttag, std::string& idvalstr );
	void    GetListSrcTableInfo( DBApi::eListType listtype, std::string& schemaname, std::string& tablename );
	void	GetSrcTableIdTags( DBApi::eListType listtype, std::string& idtag, std::string& idnumtag, std::string& indextag, std::string& nametag );
	bool	GetSrcTableQueryInfo( DBApi::eListType listtype, std::string& schemaname, std::string& tablename,
								  std::string& selecttag, std::string& idstr, uuid__t id, int64_t idnum = NO_ID_NUM, int64_t index = INVALID_INDEX, std::string name = "" );

	DBApi::eLoginType	ConnectionCheck( CDatabase*& pDb, DBApi::eLoginType requestedType = DBApi::eLoginType::InstOrAdminLoginTypes );
	DBApi::eQueryResult	FindSampleItemListMatch( DB_SampleSetRecord& ssr,
												 uuid__t ssitemid, int64_t ssitemidnum );

	bool	GetUserNameString( uuid__t userid, std::string& username );
	bool	GetProcessNameString( DBApi::eListType processtype, uuid__t processid, std::string& processname );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Object query execution methods; DBif_Exec.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool				DoDbRecreate( std::string dbname = "" );

	bool                RunColTagQuery( CDatabase* db, std::string schemaname, std::string tablename,
										int32_t& fieldcnt, std::vector<std::string>& taglist );
	bool                GetFullSelectQueryWithColTags( CDatabase* db, CRecordset& resultset,
													   std::string schemaname, std::string tablename,
													   int32_t& fieldcnt, std::vector<std::string>& taglist );
	bool                GetFullSelectQueryWithColTags( CDatabase* db, CRecordset& resultset, std::string querystr,
													   int32_t& fieldcnt, std::vector<std::string>& taglist );
	bool                GetFullSelectQuery( CDatabase* db, CRecordset& resultset, std::string query_str );

	DBApi::eQueryResult RunConfigQuery( DBApi::eLoginType usertype, std::string query_str,
										std::vector<std::string>& infolist );

	DBApi::eQueryResult DoExecute( DBApi::eLoginType usertype,
								   CDatabase* pDb,
								   std::string query_str,
								   std::vector<std::string>& resultlist,
								   bool gettags = true );						// execute query under the selected connection type
	DBApi::eQueryResult RunQuery( DBApi::eLoginType usertype,
								  std::string query_str,
								  CRecordset& resultrec );

	DBApi::eQueryResult SetDbBackupUserPwd( std::string & pwdstr );

	DBApi::eQueryResult DbTruncateTableContents(std::list<std::pair<std::string, std::string>> tableNames_name_schema);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// object and list data retrieval execution methods; DBif_Exec.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DBApi::eQueryResult DoListQuery( std::string schemaname, std::string tablename, CRecordset& resultrec,
									 std::vector<std::string>& taglist, DBApi::eListType listtype,
									 DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
									 std::string orderstring, std::string wherestr = "", std::string idNumTag = "",
									 int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startIndex = INVALID_SRCH_INDEX, int64_t startIdNum = NO_ID_NUM,
									 std::string custom_query = "" );
	DBApi::eQueryResult DoObjectQuery( std::string schemaname, std::string tablename,
									   CRecordset& resultrec, std::vector<std::string>& taglist, std::string selecttag,
									   uuid__t objuuid, int64_t itemidnum = NO_ID_NUM, int64_t itemindex = INVALID_INDEX,
									   std::string objname = "", std::string wherestr = "", std::string custom_query = "" );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  query formatting support methods; DBif_UtilsQuery.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// generic formatting routines that handle data values arrays for data values that are strings or data values not from strings
	// handle arrays of strings from string data contained in ARRAY containers
	int32_t	CreateStringDataArrayString( std::string& ValueArrayString,
										 const int32_t DataCount,
										 const uint32_t StartIndex,
										 const std::vector<std::string>& StrList );
	// handle arrays of strings for non-aarray containers
	int32_t CreateDataStringArrayString( std::string& ValueArrayString,
										 const int32_t DataCount,
										 const uint32_t StartIndex,
										 const std::vector<std::string>& StrList );

	// TODO: data insertion methods should check for user connection type and ONLY allow normal data insertions under a std user connection...
	// insertion data formatting support
	int32_t	CreateInt16ValueArrayString( std::string& datastring, const std::string fmtstr,
										 const int32_t datacount, const uint32_t startindex,
										 const std::vector<int16_t>& datalist );
	int32_t	CreateUint16ValueArrayString( std::string& datastring, const std::string fmtstr,
										  const int32_t datacount, const uint32_t startindex,
										  const std::vector<uint16_t>& datalist );
	int32_t	CreateInt32ValueArrayString( std::string& datastring, const std::string fmtstr,
										 const int32_t datacount, const uint32_t startindex,
										 const std::vector<int32_t>& datalist );
	int32_t	CreateUint32ValueArrayString( std::string& datastring, const std::string fmtstr,
										  const int32_t datacount, const uint32_t startindex,
										  const std::vector<uint32_t>& datalist );
	int32_t	CreateInt64ValueArrayString( std::string& datastring, const std::string fmtstr,
										 const int32_t datacount, const uint32_t startindex,
										 const std::vector<int64_t>& datalist );
	int32_t	CreateFloatValueArrayString( std::string& datastring, const std::string fmtstr,
										 const int32_t datacount, const uint32_t startindex,
										 const std::vector<float>& datalist );
	int32_t	CreateDoubleValueArrayString( std::string& datastring, const std::string fmtstr,
										  const int32_t datacount, const uint32_t startindex,
										  const std::vector<double>& datalist );
	int32_t	CreateDoubleExpValueArrayString( std::string& datastring, const std::string fmtstr,
											 const int32_t datacount, const uint32_t startindex,
											 const std::vector<double>& datalist );
	int32_t	CreateGuidValueArrayString( std::string& datastring, const int32_t datacount,
										const uint32_t startindex, const std::vector<uuid__t>& uuidlist );
	int32_t	CreateGuidValueArrayString( std::string& datastring, const int32_t datacount,
										const uint32_t startindex, const std::vector<GUID>& guidlist );
	int32_t	CreateGuidValueArrayString( std::string& datastring, const int32_t datacount,
										const uint32_t startindex, const std::vector<std::string>& uuidstrlist );

	int32_t	CreateGuidDataValueArrayString( std::string& DataString, const int32_t DataCount,
											const uint32_t StartIndex, const std::vector<std::string>& StrList );
	int32_t	CreateTimePtValueArrayString( std::string& datastring, const int32_t datacount,
										  const uint32_t startindex, const std::vector<system_TP>& tplist );
	int32_t	CreateStringDataValueArrayString( std::string& datastring,
											  const int32_t datacount,
											  const uint32_t startindex,
											  const std::vector<std::string>& strlist,
											  int32_t format = 0 );
	int32_t CreateDataValueStringArrayString( std::string& DataString,
											  const int32_t DataCount,
											  const uint32_t StartIndex,
											  const std::vector<std::string>& StrList,
											  int32_t format = 0 );

	////////////////////////////////////////////////////////////////////////////////
	// specialized array string constructors; these are sub-elements structures
	// entered into a single column of a table.  Individual array elements should be
	// constructed using the ROWx,y,z...) format inside the ARRAY[] container.
	////////////////////////////////////////////////////////////////////////////////

	// Generally, the composites that contain other composite DO NOT use thr 'ROW" preamble in the created string

	// to construct the array element fields for an <int16_t,int16_t> map
	int32_t	CreateInt16MapValueArrayString( std::string& datastring, const int32_t datacount,									// constructs an array containing multiple ROW() element containers for use in table column elements requiring structures (composite types) 
											const std::map<int16_t, int16_t>& datamap, bool use_rows = true );
	// to construct the array element fields for an <E_CONFIG_PARAMETER, doouble> map
	int32_t	CreateConfigParamMapValueArrayString( std::string& DataString, const int32_t DataCount,								// constructs an array containing multiple ROW() element containers for use in table column elements requiring structures (composite types)
												  const ConfigParamList_t& ConfigMap, bool use_rows = true );
	int32_t	CreateCellIdentParamArrayString( std::string& DataString, const int32_t DataCount,									// constructs an array containing multiple ROW() element containers for use in table column elements requiring structures (composite types)
											 const v_CellIdentParams_t& IdentParamList, bool use_rows = true );

	int32_t	CreateClusterDataArrayString( std::string& DataString, const int32_t DataCount,										// constructs an array containing multiple ROW() element containers for use in table column elements requiring structures (composite types)
										  const std::vector<cluster_data_t>& ClusterDataList,
										  bool use_rows = false, int32_t format = 0 );
	int32_t CreateClusterCellCountArrayString( std::string& DataString, const int32_t DataCount,
											   const std::vector<cluster_data_t>& ClusterDataList, bool use_rows );
	int32_t CreateClusterPolygonArrayString( std::string& DataString, const int32_t DataCount,
											 const std::vector<cluster_data_t>& ClusterDataList,
											 bool use_rows, int32_t format = 0 );
	int32_t CreateClusterRectArrayString( std::string& DataString, const int32_t DataCount,
										  const std::vector<cluster_data_t>& ClusterDataList,
										  bool use_rows, int32_t format = 0 );

	int32_t	CreateBlobDataArrayString( std::string& DataString, const int32_t DataCount,										// constructs an array containing multiple ROW() element containers for use in table column elements requiring structures (composite types)
									   const std::vector<blob_data_t>& BlobDataList,
									   bool use_rows = false, int32_t format = 0 );
	int32_t CreateBlobDataBlobInfoArrayString( std::string& DataString, const int32_t DataCount,
											   const std::vector<blob_data_t>& BlobDataList,
											   bool use_rows, int32_t format = 0 );
	int32_t CreateBlobDataBlobLocationArrayString( std::string& DataString, const int32_t DataCount,
												   const std::vector<blob_data_t>& BlobDataList,
												   bool use_rows, int32_t format = 0 );
	int32_t CreateBlobDataBlobOutlineArrayString( std::string& DataString, const int32_t DataCount,
												  const std::vector<blob_data_t>& BlobDataList,
												  bool use_rows, int32_t format = 0 );
	int32_t	CreateBlobCharacteristicsArrayString( std::string& DataString, const int32_t DataCount,								// constructs an array contained in a ROW() container for use in table column elements requiring structures (composite types)
												  const std::vector<blob_info_pair>& BlobInfoList,
												  bool use_rows = false, int32_t format = 0 );
	int32_t	CreateBlobPointArrayString( std::string& DataString, const int32_t DataCount,										// constructs an array contained in a ROW() container for use in table column elements requiring structures (composite types)
										const std::vector<blob_point>& BlobPointList,
										bool use_rows = false, int32_t format = 0 );
	int32_t	CreateBlobRectArrayString( std::string& DataString, const int32_t DataCount,										// constructs an array contained in a ROW() container for use in table column elements requiring structures (composite types)
									   const std::vector<blob_rect_t>& BlobRectList,
									   bool use_rows = false, int32_t format = 0 );

	int32_t CreateColumnDisplayInfoArrayString( std::string& DataString, const int32_t DataCount,
												const std::vector<display_column_info_t>& ColumnInfoList, bool use_rows = false );
	int32_t CreateSignatureArrayString( std::string& DataString, const int32_t DataCount,
										const std::vector<db_signature_t>& SignatureList, bool use_rows = false );
	int32_t CreateCalConsumablesArrayString( std::string& DataString, const int32_t DataCount,
											 const std::vector<cal_consumable_t>& CalConsumablesList, bool use_rows = false );
	int32_t CreateLanguageInfoArrayString( std::string& DataString, const int32_t DataCount,
										   const std::vector<language_info_t>& LanguageList, bool use_rows = false );
	int32_t CreateIlluminatorInfoArrayString( std::string& DataString, const int32_t DataCount,
											  const std::vector<illuminator_info_t>& IlInfoList, bool use_rows = false );

	// these construct individual composite-type element strings. they do not enclose the string in ANY Array or other container type
	bool	MakeBlobInfoString( std::string& DataString, blob_info_storage info, bool use_rows );
	bool	MakeBlobInfoPairString( std::string& DataString, blob_info_pair info, bool use_rows );
	bool	MakeBlobPointString( std::string& DataString, blob_point point, bool use_rows );									// constructs single point strings; must be enclosed in 'ROW() containers by caller
	bool	MakeBlobRectString( std::string& DataString, blob_rect_t rect, bool use_rows );										// constructs rect strings; must be enclosed in 'ROW() containers by caller
	bool	MakeAdSettingsString( std::string& DataString, ad_settings_t ad_settings, bool use_rows );
	bool	MakeAfSettingsString( std::string& DataString, af_settings_t af_settings, bool use_rows );
	bool	MakeEmailSettingsString( std::string& DataString, email_settings_t email_settings, bool use_rows );
	bool	MakeRfidSimInfoString( std::string& DataString, rfid_sim_info_t rfid_settings, bool use_rows );
	bool	MakeLanguageInfoString( std::string& DataString, language_info_t lang_info, bool use_rows );
	bool	MakeRunOptionsString( std::string& DataString, run_options_t options, bool use_rows );
	bool	MakeCalConsumablesString( std::string& DataString, cal_consumable_t consumables, bool use_rows );
	bool	MakeIlluminatorInfoString( std::string& DataString, illuminator_info il_info, bool use_rows );

	////////////////////////////////////////////////////////////////////////////////
	// Query filter string construction; DBif_Filter.cpp
	////////////////////////////////////////////////////////////////////////////////

	bool	GetWorklistListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetWorklistListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetSampleSetListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSampleSetListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									std::string compareop, std::string compareval );

	bool	GetSampleItemListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSampleItemListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									 std::string compareop, std::string compareval );

	bool	GetSampleListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSampleListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								 std::string compareop, std::string compareval );

	bool	GetAnalysisListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetAnalysisListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetSummaryResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSummaryResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										std::string compareop, std::string compareval );

	bool	GetDetailedResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetDetailedResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										 std::string compareop, std::string compareval );

	bool	GetImageResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetImageResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval );

	bool	GetSResultListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSResultListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								  std::string compareop, std::string compareval );

	bool	GetImageSetListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetImageSetListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetImageSequenceListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetImageSequenceListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										std::string compareop, std::string compareval );

	bool	GetImageListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetImageListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								std::string compareop, std::string compareval );

	bool	GetCellTypeListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetCellTypeListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetImageAnalysisParamListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetImageAnalysisParamListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											  std::string compareop, std::string compareval );

	bool	GetAnalysisInputSettingsListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetAnalysisInputSettingsListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
													 std::string compareop, std::string compareval );

	bool	GetAnalysisDefinitionListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
											 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetAnalysisDefinitionListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
											 std::string compareop, std::string compareval );

	bool	GetAnalysisParamListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetAnalysisParamListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										 std::string compareop, std::string compareval );

	bool	GetIlluminatorListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetIlluminatorListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval );

	bool	GetUserListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
							   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetUserListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
							   std::string compareop, std::string compareval );

	bool	GetRoleListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
							   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetRoleListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
							   std::string compareop, std::string compareval,
							   DBApi::eRoleClass = DBApi::eRoleClass::AllRoleClasses );

	bool	GetUserPropertiesListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetUserPropertiesListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										 std::string compareop, std::string compareval );

	bool	GetSignatureListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSignatureListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									std::string compareop, std::string compareval );

	bool	GetReagentInfoListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetReagentInfoListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval );

	bool	GetWorkflowListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetWorkflowListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetBioProcessListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									 std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetBioProcessListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									 std::string compareop, std::string compareval );

	bool	GetQcProcessListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetQcProcessListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									std::string compareop, std::string compareval );

	bool	GetCalibrationListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetCalibrationListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
									  std::string compareop, std::string compareval );

	bool	GetLogEntryListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
								   std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetLogEntryListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
								   std::string compareop, std::string compareval );

	bool	GetSchedulerConfigListFilter( std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals );
	bool	GetSchedulerConfigListFilter( std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										  std::string compareop, std::string compareval );

	bool	GetCellHealthReagentsListFilter(std::string& wherestr, std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals);
	bool	GetCellHealthReagentsListFilter(std::string& wherestr, DBApi::eListFilterCriteria filtertype,
										std::string compareop, std::string compareval);

	bool	GetListFilterString( DBApi::eListType listtype, DBApi::eListFilterCriteria filtertype,
								 std::string& wherestring, std::string compareop, std::string compareval );
	bool	GetListFilterString( DBApi::eListType listtype, std::vector<DBApi::eListFilterCriteria> filtertypes,
								 std::string& wherestring, std::vector<std::string> compareops, std::vector<std::string> comparevals );

	bool	FilterPreChecksOK( int index, std::vector<DBApi::eListFilterCriteria> filtertypes,
							   std::vector<std::string> compareops, std::vector<std::string> comparevals,
							   DBApi::eListFilterCriteria& filtertype, std::string& compareop, std::string& compareval );
	bool	CheckValueFilter( std::string& filterstr, std::string fieldtag, std::string compareop,
							  bool tfchk = false, std::string compareval = "" );
	bool	ValueStringOk( eListFilterCriteria filtertype, std::string compareval );
	bool	CompareOpValid( std::string& compareop, eListFilterCriteria filtertype );

	bool	IsSingleDateFilter( eListFilterCriteria filtertype );
	bool	FormatDateRangeFilter( std::string& filterstr, std::string fieldtag, std::string compareval );

	bool	CheckCarrierFilter( std::string& filterstr, std::string filtertag, std::string compareop, std::string compareval );
	bool	DbFilterValueIsString( DBApi::eListFilterCriteria filtertype );


	////////////////////////////////////////////////////////////////////////////////
	// Query sort string construction; DBif_Sort.cpp
	////////////////////////////////////////////////////////////////////////////////

	void	GetWorklistListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetSampleSetListSortString( std::string& orderstring,
										DBApi::eListSortCriteria primarysort,
										DBApi::eListSortCriteria secondarysort );

	void	GetSampleItemListSortString( std::string& orderstring,
										 DBApi::eListSortCriteria primarysort,
										 DBApi::eListSortCriteria secondarysort );

	void	GetSampleListSortString( std::string& orderstring,
									 DBApi::eListSortCriteria primarysort,
									 DBApi::eListSortCriteria secondarysort );

	void	GetAnalysisListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetSummaryResultListSortString( std::string& orderstring,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort );

	void	GetDetailedResultListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort );

	void	GetImageResultListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort );

	void	GetSResultListSortString( std::string& orderstring,
									  DBApi::eListSortCriteria primarysort,
									  DBApi::eListSortCriteria secondarysort );

	void	GetImageSetListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetImageSequenceListSortString( std::string& orderstring,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort );

	void	GetImageListSortString( std::string& orderstring,
									DBApi::eListSortCriteria primarysort,
									DBApi::eListSortCriteria secondarysort );

	void	GetCellTypeListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetImageAnalysisParamListSortString( std::string& orderstring,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort );

	void	GetAnalysisInputSettingsListSortString( std::string& orderstring,
													DBApi::eListSortCriteria primarysort,
													DBApi::eListSortCriteria secondarysort );

	void	GetAnalysisDefinitionListSortString( std::string& orderstring,
												 DBApi::eListSortCriteria primarysort,
												 DBApi::eListSortCriteria secondarysort );

	void	GetAnalysisParamListSortString( std::string& orderstring,
											DBApi::eListSortCriteria primarysort,
											DBApi::eListSortCriteria secondarysort );

	void	GetIlluminatorListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort );

	void	GetUserListSortString( std::string& orderstring,
								   DBApi::eListSortCriteria primarysort,
								   DBApi::eListSortCriteria secondarysort );

	void	GetRoleListSortString( std::string& orderstring,
								   DBApi::eListSortCriteria primarysort,
								   DBApi::eListSortCriteria secondarysort,
								   DBApi::eRoleClass = DBApi::eRoleClass::AllRoleClasses );

	void	GetUserPropertiesListSortString( std::string& orderstring,
											 DBApi::eListSortCriteria primarysort,
											 DBApi::eListSortCriteria secondarysort );

	void	GetSignatureListSortString( std::string& orderstring,
										DBApi::eListSortCriteria primarysort,
										DBApi::eListSortCriteria secondarysort );

	void	GetReagentInfoListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort );

	void	GetWorkflowListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetBioProcessListSortString( std::string& orderstring,
										 DBApi::eListSortCriteria primarysort,
										 DBApi::eListSortCriteria secondarysort );

	void	GetQcProcessListSortString( std::string& orderstring,
										DBApi::eListSortCriteria primarysort,
										DBApi::eListSortCriteria secondarysort );

	void	GetCalibrationListSortString( std::string& orderstring,
										  DBApi::eListSortCriteria primarysort,
										  DBApi::eListSortCriteria secondarysort );

	void	GetLogEntryListSortString( std::string& orderstring,
									   DBApi::eListSortCriteria primarysort,
									   DBApi::eListSortCriteria secondarysort );

	void	GetSchedulerConfigListSortString( std::string& orderstring,
											  DBApi::eListSortCriteria primarysort,
											  DBApi::eListSortCriteria secondarysort );

	// no filter for instrument config list, and no secondary sort...
	void	GetInstrumentConfigListSortString( std::string& orderstring,
											   DBApi::eListSortCriteria primarysort );


	void	GetListSortString( DBApi::eListType listtype, std::string& orderstring,
							   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
							   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
							   int32_t sortdir = 0 );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Discrete list query methods; DBif_ListRetrieve.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	DBApi::eQueryResult GetFilterAndList( DBApi::eListType listType, CRecordset& resultRecs, std::vector<std::string>& tagList,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult DoGuidListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
											 std::vector<uuid__t>& srcidlist, CRecordset & resultrecs,
											 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult DoIndexListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
											  std::vector<uint32_t>& srcidxlist, CRecordset& resultrecs,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult DoIdnumListRetrieval( DBApi::eListType listtype, std::vector<std::string>& taglist,
											  std::vector<int64_t>& idnumlist, CRecordset& resultrecs,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetWorklistList( std::vector<DB_WorklistRecord>& wllist,
										 std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetWorklistList( std::vector<DB_WorklistRecord>& wlrlist,
										 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										 std::string compareop = "", std::string compareval = "",
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetWorklistList( std::vector<DB_WorklistRecord>& wlrlist, std::vector<uuid__t>& wlidlist,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist,
										  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										  std::string compareop = "", std::string compareval = "",
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist, uuid__t wlid,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult GetSampleSetList( std::vector<DB_SampleSetRecord>& ssrlist, std::vector<uuid__t>& ssridlist,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::AllSubObjs,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist,
										   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										   std::string compareop = "", std::string compareval = "",
										   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist, uuid__t ssid,
										   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult GetSampleItemList( std::vector<DB_SampleItemRecord>& ssirlist, std::vector<uuid__t>& ssiridlist,
										   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSampleList( std::vector<DB_SampleRecord>& srlist,
									   std::vector<DBApi::eListFilterCriteria> filtertypes,
									   std::vector<std::string> compareops, std::vector<std::string> comparevals,
									   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleList( std::vector<DB_SampleRecord>& srlist,
									   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
									   std::string compareop = "", std::string compareval = "",
									   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSampleList( std::vector<DB_SampleRecord>& srlist, std::vector<uuid__t>& sridlist,
									   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetAnalysisList( std::vector<DB_AnalysisRecord>& anlist,
										 std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetAnalysisList( std::vector<DB_AnalysisRecord>& anlist,
										 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										 std::string compareop = "", std::string compareval = "",
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetAnalysisList( std::vector<DB_AnalysisRecord>& arlist, std::vector<uuid__t>& aridlist,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist,
											  std::vector<DBApi::eListFilterCriteria> filtertypes,
											  std::vector<std::string> compareops, std::vector<std::string> comparevals,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist,
											  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											  std::string compareop = "", std::string compareval = "",
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSummaryResultList( std::vector<DB_SummaryResultRecord>& srlist, std::vector<uuid__t>& sridlist,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetDetailedResultList( std::vector<DB_DetailedResultRecord>& drlist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetDetailedResultList( std::vector<DB_DetailedResultRecord>& drlist,
													DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													std::string compareop = "", std::string compareval = "",
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetDetailedResultList( std::vector<DB_DetailedResultRecord>& drlist, std::vector<uuid__t>& dridlist,
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetImageResultList( std::vector<DB_ImageResultRecord>& irlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageResultList( std::vector<DB_ImageResultRecord>& irlist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageResultList( std::vector<DB_ImageResultRecord>& irlist, std::vector<uuid__t>& iridlist,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSResultList( std::vector<DB_SResultRecord>& srlist,
										std::vector<DBApi::eListFilterCriteria> filtertypes,
										std::vector<std::string> compareops, std::vector<std::string> comparevals,
										DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSResultList( std::vector<DB_SResultRecord>& srlist,
										DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										std::string compareop = "", std::string compareval = "",
										DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSResultList( std::vector<DB_SResultRecord>& srlist, std::vector<uuid__t>& sridlist,
										DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist,
										 std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist,
										 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										 std::string compareop = "", std::string compareval = "",
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageSetList( std::vector<DB_ImageSetRecord>& isrlist, std::vector<uuid__t>& isridlist,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageSequenceList( std::vector<DB_ImageSeqRecord>& isrlist, std::vector<uuid__t>& isridlist,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetImageList( std::vector<DB_ImageRecord>& imrlist,
									  std::vector<DBApi::eListFilterCriteria> filtertypes,
									  std::vector<std::string> compareops, std::vector<std::string> comparevals,
									  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageList( std::vector<DB_ImageRecord>& imrlist,
									  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
									  std::string compareop = "", std::string compareval = "",
									  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageList( std::vector<DB_ImageRecord>& imrlist, std::vector<uuid__t>& sridlist,
									  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetCellTypeList( std::vector<DB_CellTypeRecord>& ctrlist,
										 std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetCellTypeList( std::vector<DB_CellTypeRecord>& ctlist,
										  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										  std::string compareop = "", std::string compareval = "",
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetImageAnalysisParamList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetImageAnalysisParamList( std::vector<DB_ImageAnalysisParamRecord>& aplist,
													DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													std::string compareop = "", std::string compareval = "",
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetAnalysisInputSettingsList( std::vector<DB_AnalysisInputSettingsRecord>& iaslist,
														   std::vector<DBApi::eListFilterCriteria> filtertypes,
														   std::vector<std::string> compareops, std::vector<std::string> comparevals,
														   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetAnalysisInputSettingsList( std::vector<DB_AnalysisInputSettingsRecord>& iaslist,
														   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
														   std::string compareop = "", std::string compareval = "",
														   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetAnalysisDefinitionList( std::vector<DB_AnalysisDefinitionRecord>& adlist,
												   std::vector<DBApi::eListFilterCriteria> filtertypes,
												   std::vector<std::string> compareops, std::vector<std::string> comparevals,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetAnalysisDefinitionList( std::vector<DB_AnalysisDefinitionRecord>& adlist,
												   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												   std::string compareop = "", std::string compareval = "",
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetAnalysisParamList( std::vector<DB_AnalysisParamRecord>& aplist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetAnalysisParamList( std::vector<DB_AnalysisParamRecord>& aplist,
											   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											   std::string compareop = "", std::string compareval = "",
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetIlluminatorList( std::vector<DB_IlluminatorRecord>& illist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetIlluminatorList( std::vector<DB_IlluminatorRecord>& illist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetUserList( std::vector<DB_UserRecord>& urlist,
									 std::vector<DBApi::eListFilterCriteria> filtertypes,
									 std::vector<std::string> compareops, std::vector<std::string> comparevals,
									 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetUserList( std::vector<DB_UserRecord>& urlist,
									 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
									 std::string compareop = "", std::string compareval = "",
									 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetUserList( std::vector<DB_UserRecord>& urlist, std::vector<uuid__t>& uridlist,
									 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetRoleList( std::vector<DB_UserRoleRecord>& rrlist,
									 std::vector<DBApi::eListFilterCriteria> filtertypes,
									 std::vector<std::string> compareops, std::vector<std::string> comparevals,
									 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetRoleList( std::vector<DB_UserRoleRecord>& rrlist,
									 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
									 std::string compareop = "", std::string compareval = "",
									 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									 std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
									 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist,
											   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											   std::string compareop = "", std::string compareval = "",
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t usrid,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, std::string usrname,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult GetUserPropertiesList( std::vector<DB_UserPropertiesRecord>& uplist, std::vector<int16_t>& propindexlist,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSignatureList( std::vector<DB_SignatureRecord>& siglist,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetSignatureList( std::vector<DB_SignatureRecord>& siglist,
										  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										  std::string compareop = "", std::string compareval = "",
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetReagentTypeList( std::vector<DB_ReagentTypeRecord>& rxlist, std::vector<int64_t>& idnumlist,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetCellHealthReagentsList(std::vector<DB_CellHealthReagentRecord>& chrlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "");

	DBApi::eQueryResult GetCellHealthReagentsList(std::vector<DB_CellHealthReagentRecord>& chrlist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "");
	
	DBApi::eQueryResult GetBioProcessList( std::vector<DB_BioProcessRecord >& bplist,
										   std::vector<DBApi::eListFilterCriteria> filtertypes,
										   std::vector<std::string> compareops, std::vector<std::string> comparevals,
										   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetBioProcessList( std::vector<DB_BioProcessRecord >& bplist,
										   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										   std::string compareop = "", std::string compareval = "",
										   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										   std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetQcProcessList( std::vector<DB_QcProcessRecord >& qclist,
										  std::vector<DBApi::eListFilterCriteria> filtertypes,
										  std::vector<std::string> compareops, std::vector<std::string> comparevals,
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetQcProcessList( std::vector<DB_QcProcessRecord >& qclist,
										  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
										  std::string compareop = "", std::string compareval = "",
										  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
										  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
										  std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
										  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetCalibrationList( std::vector<DB_CalibrationRecord >& carlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult GetCalibrationList( std::vector<DB_CalibrationRecord >& carlist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );

	DBApi::eQueryResult GetInstConfigList( std::vector<DB_InstrumentConfigRecord>& cfglist );

	DBApi::eQueryResult GetLogList( std::vector<DB_LogEntryRecord>& log_list,
									std::vector<DBApi::eListFilterCriteria> filtertypes,
									std::vector<std::string> compareops, std::vector<std::string> comparevals,
									DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT );
	DBApi::eQueryResult GetLogList( std::vector<DB_LogEntryRecord>& log_list,
									std::vector<int64_t> idnumlist,
									DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
									DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
									int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT );

	DBApi::eQueryResult GetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord>& sclist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );

	DBApi::eQueryResult GetSchedulerConfigList( std::vector<DB_SchedulerConfigRecord>& sclist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												std::string orderstring = "", int32_t sortdir = 0, int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Discrete object query methods; DBif_ObjRetrieve.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool                GetWorklistQueryTag( std::string& schemaname, std::string& tablename,
											 std::string& selecttag, std::string& idstr,
											 uuid__t worklistid, int64_t idnum = NO_ID_NUM );
	DBApi::eQueryResult GetWorklist( DB_WorklistRecord& wlr, uuid__t worklistid, int64_t idnum = NO_ID_NUM, int32_t get_sets = AllSubObjs );
	DBApi::eQueryResult GetWorklistInternal( DB_WorklistRecord& wlr, uuid__t worklistid, int64_t idnum = NO_ID_NUM, int32_t get_sets = AllSubObjs );
	DBApi::eQueryResult GetWorklistObj( DB_WorklistRecord& wlr, CRecordset& recset, std::vector<std::string>& taglist,
										uuid__t worklistid, int64_t idnum = NO_ID_NUM, int32_t get_sets = AllSubObjs );

	bool                GetSampleSetQueryTag( std::string& schemaname, std::string& tablename,
											  std::string& selecttag, std::string& idstr,
											  uuid__t itemid, int64_t itemidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleSet( DB_SampleSetRecord& sr, uuid__t samplesetid, int64_t samplesetidnum = NO_ID_NUM, int32_t get_items = AllSubObjs );
	DBApi::eQueryResult GetSampleSetInternal( DB_SampleSetRecord& sr, uuid__t samplesetid, int64_t samplesetidnum = NO_ID_NUM, int32_t get_items = AllSubObjs );
	DBApi::eQueryResult GetSampleSetObj( DB_SampleSetRecord& samplesetrec, CRecordset& recset, std::vector<std::string>& taglist,
										 uuid__t samplesetid, int64_t samplesetidnum = NO_ID_NUM, int32_t get_items = AllSubObjs );

	bool                GetSampleItemQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr,
											   uuid__t itemid, int64_t itemidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleItem( DB_SampleItemRecord& wlir, uuid__t itemid, int64_t itemidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleItemInternal( DB_SampleItemRecord& wlir, uuid__t itemid, int64_t itemidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleItemObj( DB_SampleItemRecord& wlir, CRecordset& recset,
										  std::vector<std::string>& taglist, uuid__t itemid, int64_t itemidnum = NO_ID_NUM );

	bool                GetSampleQueryTag( std::string& schemaname, std::string& tablename,
										   std::string& selecttag, std::string& idstr,
										   uuid__t itemid, int64_t itemidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSample( DB_SampleRecord& sr, uuid__t sampleid, int64_t sampleidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleInternal( DB_SampleRecord& sr, uuid__t sampleid, int64_t sampleidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSampleObj( DB_SampleRecord& samplerec, CRecordset& recset,
									  std::vector<std::string>& taglist, uuid__t sampleid, int64_t sampleidnum = NO_ID_NUM );

	bool                GetAnalysisQueryTag( std::string& schemaname, std::string& tablename,
											 std::string& selecttag, std::string& idstr,
											 uuid__t analysisid, int64_t analysisidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysis( DB_AnalysisRecord& params, uuid__t analysisid, int64_t analysisidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisInternal( DB_AnalysisRecord& params, uuid__t analysisid, int64_t analysisidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisObj( DB_AnalysisRecord& params, CRecordset& recset,
										std::vector<std::string>& taglist, uuid__t analysisid, int64_t analysisidnum = NO_ID_NUM );

	bool                GetSummaryResultQueryTag( std::string& schemaname, std::string& tablename,
												  std::string& selecttag, std::string& idstr,
												  uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSummaryResultInternal( DB_SummaryResultRecord& sr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSummaryResultObj( DB_SummaryResultRecord& sr, CRecordset& recset,
											 std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );

	bool                GetDetailedResultQueryTag( std::string& schemaname, std::string& tablename,
														std::string& selecttag, std::string& idstr,
														uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetDetailedResult( DB_DetailedResultRecord& ir, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetDetailedResultInternal( DB_DetailedResultRecord& ir, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetDetailedResultObj( DB_DetailedResultRecord& ir, CRecordset& recset,
												   std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );

	bool                GetImageResultQueryTag( std::string& schemaname, std::string& tablename,
												   std::string& selecttag, std::string& idstr,
												   uuid__t resultId, int64_t resultidnum );
	DBApi::eQueryResult GetImageResult( DB_ImageResultRecord& rmr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageResultInternal( DB_ImageResultRecord& rmr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageResultObj( DB_ImageResultRecord& rmr, CRecordset& recset,
										   std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );

	bool                GetSResultQueryTag( std::string& schemaname, std::string& tablename,
											std::string& selecttag, std::string& idstr,
											uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSResult( DB_SResultRecord& sr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSResultInternal( DB_SResultRecord& sr, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSResultObj( DB_SResultRecord& sr, CRecordset& recset,
									   std::vector<std::string>& taglist, uuid__t resultid, int64_t resultidnum = NO_ID_NUM );

	bool                GetImageSetQueryTag( std::string& schemaname, std::string& tablename,
											 std::string& selecttag, std::string& idstr,
											 uuid__t imagesetid, int64_t imagesetidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSet( DB_ImageSetRecord& isr, uuid__t imagesetid, int64_t imagesetidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSetInternal( DB_ImageSetRecord& isr, uuid__t imagesetid, int64_t imagesetidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSetObj( DB_ImageSetRecord& isr, CRecordset& recset,
										std::vector<std::string>& taglist, uuid__t imagesetid, int64_t imagesetidnum = NO_ID_NUM );

	bool                GetImageSequenceQueryTag( std::string& schemaname, std::string& tablename,
												  std::string& selecttag, std::string& idstr, uuid__t isrid, int64_t isridnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSequence( DB_ImageSeqRecord& isr, uuid__t isrid, int64_t isridnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSequenceInternal( DB_ImageSeqRecord& ir, uuid__t isrid, int64_t isridnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageSequenceObj( DB_ImageSeqRecord& ir, CRecordset& recset,
											 std::vector<std::string>& taglist, uuid__t isrid, int64_t isridnum = NO_ID_NUM );

	bool                GetImageQueryTag( std::string& schemaname, std::string& tablename,
										  std::string& selecttag, std::string& idstr, uuid__t imageid, int64_t imageidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImage( DB_ImageRecord& imr, uuid__t imageid, int64_t imageidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageInternal( DB_ImageRecord& imr, uuid__t imageid, int64_t imageidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageObj( DB_ImageRecord& imr, CRecordset& recset,
									 std::vector<std::string>& taglist, uuid__t imageid, int64_t imageidnum = NO_ID_NUM );

	bool                GetCellTypeQueryTag( std::string& schemaname, std::string& tablename,
											 std::string& selecttag, std::string& idstr,
											 uuid__t cellid, int64_t celltypeindex = INVALID_INDEX, int64_t celltypeidnum = NO_ID_NUM );
	DBApi::eQueryResult GetCellType( DB_CellTypeRecord& ctr, uuid__t celltypeid, int64_t celltypeindex = INVALID_INDEX, int64_t celltypeidnum = NO_ID_NUM );
	DBApi::eQueryResult GetCellTypeInternal( DB_CellTypeRecord& ctr, uuid__t celltypeid, int64_t celltypeindex = INVALID_INDEX, int64_t celltypeidnum = NO_ID_NUM );
	DBApi::eQueryResult GetCellTypeObj( DB_CellTypeRecord& ctr, CRecordset& recset, std::vector<std::string>& taglist,
										uuid__t celltypeid, int64_t celltypeindex = INVALID_INDEX, int64_t celltypeidnum = NO_ID_NUM );

	bool                GetImageAnalysisParamQueryTag( std::string& schemaname, std::string& tablename,
														std::string& selecttag, std::string& idstr,
														uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageAnalysisParam( DB_ImageAnalysisParamRecord& params, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageAnalysisParamInternal( DB_ImageAnalysisParamRecord& params, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetImageAnalysisParamObj( DB_ImageAnalysisParamRecord& params, CRecordset& recset,
												   std::vector<std::string>& taglist, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );

	bool                GetAnalysisInputSettingsQueryTag( std::string& schemaname, std::string& tablename,
														  std::string& selecttag, std::string& idstr,
														  uuid__t settingsid, int64_t settingsidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisInputSettings( DB_AnalysisInputSettingsRecord& settings, uuid__t settingsid, int64_t settingsidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisInputSettingsInternal( DB_AnalysisInputSettingsRecord& settings, uuid__t settingsid, int64_t settingsidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisInputSettingsObj( DB_AnalysisInputSettingsRecord& settings, CRecordset& recset,
													 std::vector<std::string>& taglist, uuid__t settingsid, int64_t settingsidnum = NO_ID_NUM );

	bool                GetAnalysisDefinitionQueryTag( std::string& schemaname, std::string& tablename,
													   std::string& selecttag, std::string& idstr,
													   uuid__t objguid, int32_t defindex = INVALID_INDEX, int64_t defidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisDefinition( DB_AnalysisDefinitionRecord& def,
											   uuid__t defguid, int32_t defindex = INVALID_INDEX, int64_t defidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisDefinitionInternal( DB_AnalysisDefinitionRecord& def,
													   uuid__t defguid, int32_t defindex = INVALID_INDEX, int64_t defidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisDefinitionObj( DB_AnalysisDefinitionRecord& def, CRecordset& recset, std::vector<std::string>& taglist,
												  uuid__t objguid, int32_t defindex = INVALID_INDEX, int64_t defidnum = NO_ID_NUM );

	bool                GetAnalysisParamQueryTag( std::string& schemaname, std::string& tablename,
												   std::string& selecttag, std::string& idstr,
												   uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisParam( DB_AnalysisParamRecord& params, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisParamInternal( DB_AnalysisParamRecord& params, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );
	DBApi::eQueryResult GetAnalysisParamObj( DB_AnalysisParamRecord& params, CRecordset& recset,
											  std::vector<std::string>& taglist, uuid__t paramid, int64_t paramidnum = NO_ID_NUM );

	bool                GetIlluminatorQueryTag( std::string& schemaname, std::string& tablename,
												std::string& selecttag, std::string& idstr,
												std::string ilname = "", int16_t ilindex = INVALID_INDEX, int64_t ilidnum = NO_ID_NUM );
	bool                GetIlluminatorQueryTag( std::string& schemaname, std::string& tablename,
												std::string& selecttag, std::string& idstr,
												int32_t emission_wvlength = 0, int32_t illuminator_wvlength = 0 );
	DBApi::eQueryResult GetIlluminator( DB_IlluminatorRecord& ilr, std::string ilname = "", int16_t ilindex = INVALID_INDEX, int64_t ilidnum = NO_ID_NUM );
	DBApi::eQueryResult GetIlluminatorInternal( DB_IlluminatorRecord& ilr, std::string ilname = "", int16_t ilindex = INVALID_INDEX, int64_t ilidnum = NO_ID_NUM );
	DBApi::eQueryResult GetIlluminator( DB_IlluminatorRecord& ilr, int16_t emission_wvlength = 0, int16_t illuminator_wvlength = 0 );
	DBApi::eQueryResult GetIlluminatorInternal( DB_IlluminatorRecord& ilr, int16_t emission_wvlength = 0, int16_t illuminator_wvlength = 0 );
	DBApi::eQueryResult GetIlluminatorObj( DB_IlluminatorRecord& ilr, CRecordset& recset, std::vector<std::string>& taglist,
										   std::string ilname = "", int16_t ilindex = INVALID_INDEX, int64_t ilidnum = NO_ID_NUM );
	DBApi::eQueryResult GetIlluminatorObj( DB_IlluminatorRecord& ilr, CRecordset& recset, std::vector<std::string>& taglist,
										   int16_t emission_wvlength = 0, int16_t illuminator_wvlength = 0 );

	bool                GetUserQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t userid, int64_t useridnum = NO_ID_NUM, std::string username = "" );
	DBApi::eQueryResult GetUser( DB_UserRecord& ur, uuid__t userid, std::string username = "",
								 DBApi::eUserType user_type = DBApi::eUserType::AllUsers );
	DBApi::eQueryResult GetUserInternal( DB_UserRecord& ur, uuid__t userid, std::string username = "",
										 DBApi::eUserType user_type = DBApi::eUserType::AllUsers );
	DBApi::eQueryResult GetUserObj( DB_UserRecord& ur, CRecordset& recset, std::vector<std::string>& taglist,
									uuid__t userid, int64_t useridnum = NO_ID_NUM, std::string username = "",
									DBApi::eUserType user_type = DBApi::eUserType::AllUsers );

	bool                GetRoleQueryTag( std::string& schemaname, std::string& tablename,
										 std::string& selecttag, std::string& idstr,
										 uuid__t roleid, int64_t roleidnum = NO_ID_NUM, std::string rolename = "" );
	DBApi::eQueryResult GetRole( DB_UserRoleRecord& rr, uuid__t roleid, std::string rolename = "" );
	DBApi::eQueryResult GetRoleInternal( DB_UserRoleRecord& rr, uuid__t roleid, std::string rolename = "" );
	DBApi::eQueryResult GetRoleObj( DB_UserRoleRecord& rr, CRecordset& recset, std::vector<std::string>& taglist,
									uuid__t roleid, int64_t roleidnum = NO_ID_NUM, std::string rolename = "" );

	bool                GetUserPropertyQueryTag( std::string& schemaname, std::string& tablename,
												 std::string& selecttag, std::string& idstr,
												 int32_t propindex = INVALID_INDEX, std::string username = "", int64_t idnum = NO_ID_NUM );
	DBApi::eQueryResult GetUserProperty( DB_UserPropertiesRecord& up, int32_t propindex = INVALID_INDEX, std::string propname = "", int64_t propidnum = NO_ID_NUM );
	DBApi::eQueryResult GetUserPropertyInternal( DB_UserPropertiesRecord& up, int32_t propindex = INVALID_INDEX, std::string propname = "", int64_t propidnum = NO_ID_NUM );
	DBApi::eQueryResult GetUserPropertyObj( DB_UserPropertiesRecord& up, CRecordset& recset, std::vector<std::string>& taglist,
											int32_t propindex = INVALID_INDEX, std::string propname = "", int64_t propidnum = NO_ID_NUM );

	bool                GetSignatureQueryTag( std::string& schemaname, std::string& tablename,
												  std::string& selecttag, std::string& idstr, uuid__t sigid, int64_t sigidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSignature( DB_SignatureRecord& sigr, uuid__t sigid, int64_t sigidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSignatureInternal( DB_SignatureRecord& sigr, uuid__t sigid, int64_t sigidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSignatureObj( DB_SignatureRecord& sigr, CRecordset& recset,
											 std::vector<std::string>& taglist, uuid__t sigid, int64_t sigidnum = NO_ID_NUM );

	bool                GetReagentTypeQueryTag( std::string& schemaname, std::string& tablename,
											  std::string& selecttag, std::string& idstr, int64_t rxidnum = NO_ID_NUM, std::string tagsn = "" );
	DBApi::eQueryResult GetReagentType( DB_ReagentTypeRecord& rxr, int64_t rxidnum = NO_ID_NUM, std::string tagsn = "" );
	DBApi::eQueryResult GetReagentTypeInternal( DB_ReagentTypeRecord& rxr, int64_t rxidnum = NO_ID_NUM, std::string tagsn = "" );
	DBApi::eQueryResult GetReagentTypeObj( DB_ReagentTypeRecord& rxr, CRecordset& recset,
										 std::vector<std::string>& taglist, int64_t rxidnum = NO_ID_NUM, std::string tagsn = "" );

	bool                GetCellHealthReagentsQueryTag(std::string& schemaname, std::string& tablename,
													std::string& selecttag, std::string& idstr,
													std::string ilname, int16_t ilindex, int64_t ilidnum);

	bool				GetCellHealthReagentsQueryTag(std::string& schemaname, std::string& tablename,
													std::string& selecttag, std::string& idstr, int16_t type);	

	DBApi::eQueryResult GetCellHealthReagent(DB_CellHealthReagentRecord& chr, int16_t type = 0);
	DBApi::eQueryResult GetCellHealthReagentInternal(DB_CellHealthReagentRecord& chr, int16_t type = 0);
	DBApi::eQueryResult GetCellHealthReagentObj(DB_CellHealthReagentRecord& chr, CRecordset& recset,
										std::vector<std::string>& taglist, int16_t type = 0);

	bool                GetBioProcessQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr, uuid__t bpid, int64_t bpidnum = NO_ID_NUM );
	DBApi::eQueryResult GetBioProcess( DB_BioProcessRecord& bpr, uuid__t bpid, int64_t bpidnum = NO_ID_NUM );
	DBApi::eQueryResult GetBioProcessInternal( DB_BioProcessRecord& bpr, uuid__t bpid, int64_t bpidnum = NO_ID_NUM );
	DBApi::eQueryResult GetBioProcessObj( DB_BioProcessRecord& bpr, CRecordset& recset,
										  std::vector<std::string>& taglist, uuid__t bpid, int64_t bpidnum = NO_ID_NUM );

	bool                GetQcProcessQueryTag( std::string& schemaname, std::string& tablename,
											  std::string& selecttag, std::string& idstr, uuid__t qcid, int64_t qcidnum = NO_ID_NUM, std::string qcname = "" );
	DBApi::eQueryResult GetQcProcess( DB_QcProcessRecord& qpr, uuid__t qcid, int64_t qcidnum = NO_ID_NUM, std::string qcname = "" );
	DBApi::eQueryResult GetQcProcessInternal( DB_QcProcessRecord& qpr, uuid__t qcid, int64_t qcidnum = NO_ID_NUM, std::string qcname = "" );
	DBApi::eQueryResult GetQcProcessObj( DB_QcProcessRecord& qpr, CRecordset& recset,
										 std::vector<std::string>& taglist, uuid__t qcid, int64_t qcidnum = NO_ID_NUM, std::string qcname = "" );

	bool				GetCalibrationQueryTag( std::string& schemaname, std::string& tablename,
												std::string& selecttag, std::string& idstr,
												uuid__t calid, int64_t calidnum = NO_ID_NUM );
	DBApi::eQueryResult	GetCalibration( DB_CalibrationRecord& car, uuid__t calid, int64_t calidnum = NO_ID_NUM );
	DBApi::eQueryResult	GetCalibrationInternal( DB_CalibrationRecord& car, uuid__t calid, int64_t calidnum = NO_ID_NUM );
	DBApi::eQueryResult	GetCalibrationObj( DB_CalibrationRecord& car, CRecordset& recset,
										   std::vector<std::string>& taglist, uuid__t calid, int64_t calidnum = NO_ID_NUM );

	bool                GetInstConfigQueryTag( std::string& schemaname, std::string& tablename,
											   std::string& selecttag, std::string& idstr, std::string instsn = "", int64_t cfgidnum = NO_ID_NUM );
	DBApi::eQueryResult GetInstConfig( DB_InstrumentConfigRecord& icr, std::string instsn = "", int64_t cfgidnum = NO_ID_NUM );

	DBApi::eQueryResult GetInstConfigInternal( DB_InstrumentConfigRecord& icr, std::string instsn = "", int64_t cfgidnum = NO_ID_NUM );
	DBApi::eQueryResult GetInstConfigObj( DB_InstrumentConfigRecord& icr, CRecordset& recset,
										  std::vector<std::string>& taglist, std::string instsn = "", int64_t cfgidnum = NO_ID_NUM );

	bool                GetLogEntryQueryTag( std::string& schemaname, std::string& tablename,
											 std::string& selecttag, std::string& idstr,
											 int64_t entry_idnum = NO_ID_NUM );
	DBApi::eQueryResult GetLogEntry( DB_LogEntryRecord& logpr, int64_t qcidnum = NO_ID_NUM );
	DBApi::eQueryResult GetLogEntryInternal( DB_LogEntryRecord& logr, int64_t qcidnum = NO_ID_NUM );
	DBApi::eQueryResult GetLogEntryObj( DB_LogEntryRecord& logr, CRecordset& recset,
										std::vector<std::string>& taglist, int64_t qcidnum = NO_ID_NUM );

	bool				GetSchedulerConfigQueryTag( std::string& schemaname, std::string& tablename,
													std::string& selecttag, std::string& idstr,
													uuid__t scid, int64_t scidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSchedulerConfig( DB_SchedulerConfigRecord& scr, uuid__t scid, int64_t scidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSchedulerConfigInternal( DB_SchedulerConfigRecord& scr, uuid__t scid, int64_t scidnum = NO_ID_NUM );
	DBApi::eQueryResult GetSchedulerConfigObj( DB_SchedulerConfigRecord& scr, CRecordset& recset,
											   std::vector<std::string>& taglist, uuid__t scid, int64_t scidnum = NO_ID_NUM );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Object parsing methods; DBif_Parse.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool	DecodeBooleanRecordField( CRecordset& resultrec, CString& fieldTag );		// utility for boolean value handling

	bool    ParseWorklist( DBApi::eQueryResult& queryresult, DB_WorklistRecord& wlrec,
						   CRecordset& resultrec, std::vector<std::string>& taglist, int32_t get_sets = AllSubObjs );
	bool    ParseSampleSet( DBApi::eQueryResult& queryresult, DB_SampleSetRecord& samplesetrec,
							CRecordset& resultrec, std::vector<std::string>& taglist, int32_t get_items = AllSubObjs );
	bool    ParseSampleItem( DBApi::eQueryResult& queryresult, DB_SampleItemRecord& wlirec,
								CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseSample( DBApi::eQueryResult& queryresult, DB_SampleRecord& samplerec,
						 CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseAnalysis( DBApi::eQueryResult& queryresult, DB_AnalysisRecord& analysis,
						   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseSummaryResult( DBApi::eQueryResult& queryresult, DB_SummaryResultRecord& sr,
								CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseDetailedResult( DBApi::eQueryResult& queryresult, DB_DetailedResultRecord& dr,
									  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseImageResult( DBApi::eQueryResult& queryresult, DB_ImageResultRecord& ir,
							  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseSResult( DBApi::eQueryResult& queryresult, DB_SResultRecord& sr,
						  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseImageSet( DBApi::eQueryResult& queryresult, DB_ImageSetRecord& isr,
						   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseImageSequence( DBApi::eQueryResult& queryresult, DB_ImageSeqRecord& isr,
								CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseImage( DBApi::eQueryResult& queryResult, DB_ImageRecord& ir,
						CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseCellType( DBApi::eQueryResult& queryresult, DB_CellTypeRecord& ctrec,
						   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseImageAnalysisParam( DBApi::eQueryResult& queryresult, DB_ImageAnalysisParamRecord& iapr,
									  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseAnalysisInputSettings( DBApi::eQueryResult& queryresult, DB_AnalysisInputSettingsRecord& aisr,
										CRecordset& resultrec, std::vector<std::string>& taglist );
//	bool    ParseImageAnalysisBlobIdentParam( DBApi::eQueryResult& queryresult, DB_BlobIdentParamRecord& bir,
//											   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseAnalysisDefinition( DBApi::eQueryResult& queryresult, DB_AnalysisDefinitionRecord& def,
									 CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseAnalysisParam( DBApi::eQueryResult& queryresult, DB_AnalysisParamRecord& params,
								 CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseIlluminator( DBApi::eQueryResult& queryresult, DB_IlluminatorRecord& ilr,
							  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseUser( DBApi::eQueryResult& queryresult, DB_UserRecord& ur,
					   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseRole( DBApi::eQueryResult& queryresult, DB_UserRoleRecord& rr,
					   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseUserProperty( DBApi::eQueryResult& queryresult, DB_UserPropertiesRecord& up,
							   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseSignature( DBApi::eQueryResult& queryresult, DB_SignatureRecord& sigr,
							CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseReagentType( DBApi::eQueryResult& queryresult, DB_ReagentTypeRecord& rxr,
							  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool	ParseCellHealthReagent(DBApi::eQueryResult& queryresult, DB_CellHealthReagentRecord& chr,
							CRecordset& resultrec, std::vector<std::string>& taglist);
	bool    ParseBioProcess( DBApi::eQueryResult& queryresult, DB_BioProcessRecord& bpr,
							   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool    ParseQcProcess( DBApi::eQueryResult& queryresult, DB_QcProcessRecord& qcr,
							   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool	ParseCalibration( DBApi::eQueryResult& queryresult, DB_CalibrationRecord& car,
							  CRecordset& resultrec, std::vector<std::string>& taglist );
	bool	ParseInstConfig( DBApi::eQueryResult& queryresult, DB_InstrumentConfigRecord& icr,
							 CRecordset& resultrec, std::vector<std::string>& taglist );
	bool	ParseLogEntry( DBApi::eQueryResult& queryresult, DB_LogEntryRecord& logr,
						   CRecordset& resultrec, std::vector<std::string>& taglist );
	bool	ParseSchedulerConfig( DBApi::eQueryResult& queryresult, DB_SchedulerConfigRecord& scr,
								  CRecordset& resultrec, std::vector<std::string>& taglist );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Discrete object insertion methods; DBif_Insert.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DBApi::eQueryResult InsertWorklistRecord( DB_WorklistRecord& wlr );
	DBApi::eQueryResult InsertWorklistSampleSetRecord( uuid__t wqid, DB_SampleSetRecord& ssr );							// insert a sample set reference into a worklist; create/save SampleSet record to the DB if necessary
	DBApi::eQueryResult InsertWorklistSampleSetRecord( uuid__t wqid, uuid__t ssrid, int64_t ssridnum = NO_ID_NUM );			// insert a sample set reference into a worklist; sample set must exist in the DB
	DBApi::eQueryResult InsertSampleSetRecord( DB_SampleSetRecord& ssr, bool in_ext_transaction = false );					// insert a sample set into the DB; does not associate with a worklist
	DBApi::eQueryResult InsertSampleSetSampleItemRecord( uuid__t ssid, DB_SampleItemRecord& ssir );						// insert a sample set item into a sample set; creates/saves to the DB if necessary
	DBApi::eQueryResult InsertSampleSetSampleItemRecord( uuid__t ssid, uuid__t ssiid, int64_t ssiidnum = NO_ID_NUM );			// insert a sample set item into a sample set; creates/saves to the DB if necessary
	DBApi::eQueryResult InsertSampleItemRecord( DB_SampleItemRecord& ssir, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertSampleRecord( DB_SampleRecord& sr );
	DBApi::eQueryResult InsertAnalysisRecord( DB_AnalysisRecord& ar );
	DBApi::eQueryResult InsertSummaryResultRecord( DB_SummaryResultRecord& sr, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertDetailedResultRecord( DB_DetailedResultRecord& ir, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertImageResultRecord( DB_ImageResultRecord& ir, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertSResultRecord( DB_SResultRecord& sr );
	DBApi::eQueryResult InsertImageSetRecord( DB_ImageSetRecord& isr );
	DBApi::eQueryResult InsertImageSeqRecord( DB_ImageSeqRecord& isr, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertImageRecord( DB_ImageRecord& img, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertCellTypeRecord( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult InsertImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& params );
	DBApi::eQueryResult InsertAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& params, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& def, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertAnalysisParamRecord( DB_AnalysisParamRecord& params, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertIlluminatorRecord( DB_IlluminatorRecord& isr, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertUserRecord( DB_UserRecord& ur );
	DBApi::eQueryResult InsertRoleRecord( DB_UserRoleRecord& rr );
	DBApi::eQueryResult InsertUserPropertyRecord( DB_UserPropertiesRecord& up, bool in_ext_transaction = false );
	DBApi::eQueryResult InsertSignatureRecord( DB_SignatureRecord& sig );
	DBApi::eQueryResult InsertReagentTypeRecord( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult InsertBioProcessRecord( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult InsertQcProcessRecord( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult	InsertCalibrationRecord( DB_CalibrationRecord& car );
	DBApi::eQueryResult	InsertInstConfigRecord( DB_InstrumentConfigRecord& cfr );
	DBApi::eQueryResult InsertLogEntryRecord( DB_LogEntryRecord& logr );
	DBApi::eQueryResult InsertSchedulerConfigRecord( DB_SchedulerConfigRecord& scr );


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  Discrete object update methods; DBif_Update.cpp
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DBApi::eQueryResult UpdateWorklistRecord( DB_WorklistRecord& wlr, bool doSampleSets = true );
	DBApi::eQueryResult UpdateWorklistStatus( uuid__t worklistid, DBApi::eWorklistStatus status );
	DBApi::eQueryResult UpdateWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesetid, DBApi::eSampleSetStatus status );
	DBApi::eQueryResult UpdateSampleSetRecord( DB_SampleSetRecord& ssr, bool doSampleItems = true, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status );
	DBApi::eQueryResult UpdateSampleItemRecord( DB_SampleItemRecord& wlir, bool doParams = true, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status );
	DBApi::eQueryResult UpdateSampleRecord( DB_SampleRecord& sr );
	DBApi::eQueryResult UpdateSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status );
	DBApi::eQueryResult UpdateSampleSetItemStatus( uuid__t samplesetid, uuid__t itemid, DBApi::eSampleItemStatus status );
	DBApi::eQueryResult UpdateAnalysisRecord( DB_AnalysisRecord& ar );
	DBApi::eQueryResult UpdateSummaryResultRecord( DB_SummaryResultRecord& sr, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateImageSetRecord( DB_ImageSetRecord& isr );
	DBApi::eQueryResult UpdateImageSeqRecord( DB_ImageSeqRecord& isr, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateImageRecord( DB_ImageRecord& ir, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateCellTypeRecord( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult UpdateImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& params );
	DBApi::eQueryResult UpdateAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& params );
	DBApi::eQueryResult UpdateAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& def, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateAnalysisParamRecord( DB_AnalysisParamRecord& params, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateIlluminatorRecord( DB_IlluminatorRecord& isr, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateUserRecord( DB_UserRecord& ur );
	DBApi::eQueryResult UpdateRoleRecord( DB_UserRoleRecord& rr );
	DBApi::eQueryResult UpdateUserPropertyRecord( DB_UserPropertiesRecord& up, bool in_ext_transaction = false );
	DBApi::eQueryResult UpdateSignatureRecord( DB_SignatureRecord& sig );
	DBApi::eQueryResult UpdateReagentTypeRecord( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult UpdateCellHealthReagentRecord(DB_CellHealthReagentRecord& rxr);
	DBApi::eQueryResult UpdateBioProcessRecord( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult UpdateQcProcessRecord( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult	UpdateInstConfigRecord( DB_InstrumentConfigRecord& cfr );
	DBApi::eQueryResult UpdateLogEntry( DB_LogEntryRecord& logr );
	DBApi::eQueryResult UpdateSchedulerConfigRecord( DB_SchedulerConfigRecord& scr );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// generic object removal methods; DBif_Remove.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DBApi::eQueryResult DoRecordDelete( DBApi::eListType listtype, DBApi::eListFilterCriteria idfilter, uuid__t recid = {}, int64_t recidnum = NO_ID_NUM, int64_t recindex = INVALID_INDEX, std::string recname = "", bool protected_delete_ok = DELETE_PROTECTED_NOT_ALLOWED );
	DBApi::eQueryResult DoRecordListDelete( DBApi::eListType listtype, std::vector<uuid__t> idlist, bool protected_delete_ok = DELETE_PROTECTED_NOT_ALLOWED );
	DBApi::eQueryResult DoRecordListDelete( DBApi::eListType listtype, std::vector<int64_t> idnumlist, bool protected_delete_ok = DELETE_PROTECTED_NOT_ALLOWED );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	//  Discrete object removal methods; DBif_Remove.cpp
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

	DBApi::eQueryResult DeleteWorklistRecord( DB_WorklistRecord& wlr );
	DBApi::eQueryResult DeleteWorklistRecord( uuid__t listid );
	DBApi::eQueryResult DeleteWorklistRecords( std::vector<uuid__t>& guidlist );
	DBApi::eQueryResult DeleteWorklistSampleSetRecord( DB_WorklistRecord& wlr, DB_SampleSetRecord& ssr, bool do_tree_delete );
	DBApi::eQueryResult DeleteWorklistSampleSetRecord( uuid__t listid, uuid__t ssid, bool do_tree_delete );
	DBApi::eQueryResult DeleteWorklistSampleSetRecords( uuid__t listid, std::vector<uuid__t>& guidlist, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleSetRecord( DB_SampleSetRecord& ssr, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleSetRecord( uuid__t samplesetid, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleSetRecords( std::vector<uuid__t>& idlist, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleSetSampleItemRecord( DB_SampleSetRecord& ssr, DB_SampleItemRecord& ssir );
	DBApi::eQueryResult DeleteSampleSetSampleItemRecord( uuid__t ssid, uuid__t ssitemid );
	DBApi::eQueryResult DeleteSampleSetSampleItemRecords( uuid__t ssid, std::vector<uuid__t>& guidlist );
	DBApi::eQueryResult DeleteSampleItemRecord( DB_SampleItemRecord& ssir );
	DBApi::eQueryResult DeleteSampleItemRecord( uuid__t ssitemid );
	DBApi::eQueryResult DeleteSampleItemRecords( std::vector<uuid__t>& guidlist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteSampleRecord( DB_SampleRecord& sr, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleRecord( uuid__t sampleid, bool do_tree_delete );
	DBApi::eQueryResult DeleteSampleRecords( std::vector<uuid__t>& guidlist, bool do_tree_delete );
	DBApi::eQueryResult DeleteAnalysisRecord( DB_AnalysisRecord& ar, bool do_tree_delete );
	DBApi::eQueryResult DeleteAnalysisRecord( uuid__t analysisid, bool do_tree_delete );
	DBApi::eQueryResult DeleteAnalysisRecordsBySampleId( uuid__t sampleid, bool do_tree_delete );
	DBApi::eQueryResult DeleteAnalysisRecords( std::vector<uuid__t>& idlist, bool do_tree_delete );
	DBApi::eQueryResult DeleteSummaryResultRecord( DB_SummaryResultRecord& sr );
	DBApi::eQueryResult DeleteSummaryResultRecord( uuid__t resultid );
	DBApi::eQueryResult DeleteSummaryResultRecords( std::vector<uuid__t>& idlist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteDetailedResultRecord( DB_DetailedResultRecord& dr );
	DBApi::eQueryResult DeleteDetailedResultRecord( uuid__t resultid );
	DBApi::eQueryResult DeleteDetailedResultRecords( std::vector<uuid__t>& idlist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageResultRecord( DB_ImageResultRecord& ir, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageResultRecord( uuid__t resultid, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageResultRecords( std::vector<DB_ImageResultRecord>& irList, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageResultRecords(bool in_ext_transaction, std::vector<uuid__t>& idlist );
	DBApi::eQueryResult DeleteSResultRecord( DB_SResultRecord& sr, bool in_ext_transaction );
	DBApi::eQueryResult DeleteSResultRecord( uuid__t resultid, bool in_ext_transaction );
	DBApi::eQueryResult DeleteSResultRecords( std::vector<DB_SResultRecord>& srlist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteSResultRecords(  bool in_ext_transaction, std::vector<uuid__t>& idlist );
	DBApi::eQueryResult DeleteImageSetRecord( DB_ImageSetRecord& isr, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSetRecord( uuid__t imagesetid, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSetRecords( std::vector<DB_ImageSetRecord>& islist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSetRecords(  bool in_ext_transaction, std::vector<uuid__t>& idlist );
	DBApi::eQueryResult DeleteImageSeqRecord( DB_ImageSeqRecord& isr, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSeqRecord( uuid__t imageseqid, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSeqRecords( std::vector<DB_ImageSeqRecord>& seqlist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageSeqRecords(  bool in_ext_transaction, std::vector<uuid__t>& idlist );
	DBApi::eQueryResult DeleteImageRecord( DB_ImageRecord& imr );
	DBApi::eQueryResult DeleteImageRecord( uuid__t imageid );
	DBApi::eQueryResult DeleteImageRecords( std::vector<DB_ImageRecord>& imglist, bool in_ext_transaction );
	DBApi::eQueryResult DeleteImageRecords(  bool in_ext_transaction, std::vector<uuid__t>& idlist );
	DBApi::eQueryResult DeleteCellTypeRecord( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult DeleteCellTypeRecord( uuid__t celltypeid, int64_t celltypeindex = INVALID_INDEX );
	DBApi::eQueryResult DeleteImageAnalysisParamRecord( DB_ImageAnalysisParamRecord& iapr );
	DBApi::eQueryResult DeleteImageAnalysisParamRecord( uuid__t paramid );
	DBApi::eQueryResult DeleteAnalysisInputSettingsRecord( DB_AnalysisInputSettingsRecord& aisr );
	DBApi::eQueryResult DeleteAnalysisInputSettingsRecord( uuid__t aisrid );
	DBApi::eQueryResult DeleteAnalysisDefinitionRecord( DB_AnalysisDefinitionRecord& adr );
	DBApi::eQueryResult DeleteAnalysisDefinitionRecord( uuid__t defid, int32_t defindex = INVALID_INDEX );
	DBApi::eQueryResult DeleteAnalysisParamRecord( DB_AnalysisParamRecord& apr );
	DBApi::eQueryResult DeleteAnalysisParamRecord( uuid__t paramid );
	DBApi::eQueryResult DeleteIlluninatorRecord( DB_IlluminatorRecord& ilr );
	DBApi::eQueryResult DeleteIlluninatorRecord( std::string ilname = "", int32_t ilindex = INVALID_INDEX, int64_t ilidnum = NO_ID_NUM );
	DBApi::eQueryResult DeleteUserRecord( DB_UserRecord& ur );
	DBApi::eQueryResult DeleteUserRecord( uuid__t userid, std::string username = "", DBApi::eUserType user_type = DBApi::eUserType::AllUsers );
	DBApi::eQueryResult DeleteRoleRecord( DB_UserRoleRecord& rr );
	DBApi::eQueryResult DeleteRoleRecord( uuid__t roleid, std::string rolename = "" );
	DBApi::eQueryResult DeleteUserPropertyRecord( DB_UserPropertiesRecord& upr );
	DBApi::eQueryResult DeleteUserPropertyRecord( int32_t upindex, std::string upname = "" );
	DBApi::eQueryResult DeleteSignatureRecord( DB_SignatureRecord& sig );
	DBApi::eQueryResult DeleteSignatureRecord( uuid__t sigid );
	DBApi::eQueryResult DeleteReagentTypeRecord( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult DeleteReagentTypeRecord( int64_t rxidnum );
	DBApi::eQueryResult DeleteReagentTypeRecord( std::string tagid );		// for single deletions; will fail if multiple records are found...
	DBApi::eQueryResult DeleteReagentTypeRecords( std::vector<int64_t> idnumlist );
	DBApi::eQueryResult DeleteReagentTypeRecords( std::string tagid );
	DBApi::eQueryResult DeleteReagentTypeLotRecords( std::string lot_num );
	DBApi::eQueryResult DeleteBioProcessRecord( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult DeleteBioProcessRecord( uuid__t bprid );
	DBApi::eQueryResult DeleteQcProcessRecord( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult DeleteQcProcessRecord( uuid__t qcrid );
	DBApi::eQueryResult DeleteCalibrationRecord( DB_CalibrationRecord& car );
	DBApi::eQueryResult DeleteCalibrationRecord( uuid__t calid );
	DBApi::eQueryResult	DeleteInstConfigRecord( DB_InstrumentConfigRecord& cfr );
	DBApi::eQueryResult	DeleteInstConfigRecord( std::string instsn = "" );
//	DBApi::eQueryResult DeleteLogEntryRecord( DB_LogEntryRecord& logr );
//	DBApi::eQueryResult DeleteLogEntryRecord( int64_t entry_idnum );
	DBApi::eQueryResult DeleteLogEntryRecords( std::vector<DB_LogEntryRecord>& entrylist );
	DBApi::eQueryResult DeleteLogEntryRecords( std::vector<int64_t>& idnumlist );
	DBApi::eQueryResult DeleteSchedulerConfigRecord( DB_SchedulerConfigRecord& scr );
	DBApi::eQueryResult DeleteSchedulerConfigRecord( uuid__t scid );


	////////////////////////////////////////////////////////////////////////////////
	// template functions
	////////////////////////////////////////////////////////////////////////////////

	// Build ptree from info stream
	template<class Ptree>
	void construct_ptree_info_from_text( StringList& infoList,
										 Ptree& pt,
										 const std::string& filename,
										 int32_t include_depth )
	{
		typedef std::basic_string<char> str_t;
		// Possible parser states
		enum state_t
		{
			s_key,              // Parser expects key
			s_data,             // Parser expects data
			s_data_cont         // Parser expects data continuation
		};

		unsigned long line_no = 0;
		state_t state = s_key;          // Parser state
		Ptree* last = NULL;             // Pointer to last created ptree
										// Define line here to minimize reallocations
		str_t line;
		std::basic_string<char>::size_type findIdx = std::string::npos;

		// Initialize ptree stack (used to handle nesting)
		std::stack<Ptree*> stack;
		stack.push( &pt );                // Push root ptree on stack initially
		size_t listIdx = 0;
		size_t listSize = infoList.size();

		try
		{
			// While there are characters in the stream
			while ( listIdx < listSize )
			{
				// Read one line from input string
				++line_no;
				line = infoList.at( listIdx++ );
				const char* text = line.c_str();

				// If directive found
				boost::property_tree::info_parser::skip_whitespace( text );
				if ( *text == char( '#' ) )
				{
					// Determine directive type
					++text;     // skip #
					std::basic_string<char> directive = boost::property_tree::info_parser::read_word( text );
					if ( directive == boost::property_tree::info_parser::convert_chtype<char, char>( "include" ) )
					{
						// #include not handled by string-list tree constructor...
						BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error(
							"cannot process include files in internal Ptree constructor",
							filename, line_no ) );
					}
					else
					{   // Unknown directive
						BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error(
							"unknown directive",
							filename, line_no ) );
					}

					// Directive must be followed by end of line
					boost::property_tree::info_parser::skip_whitespace( text );
					if ( *text != char( '\0' ) )
					{
						BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error(
							"expected end of line",
							filename, line_no ) );
					}

					// Go to next line
					continue;
				}

				// While there are characters left in line
				while ( 1 )
				{
					// Stop parsing on end of line or comment
					boost::property_tree::info_parser::skip_whitespace( text );
					if ( *text == char( '\0' ) || *text == char( ';' ) )
					{
						if ( state == s_data )    // If there was no data set state to s_key
							state = s_key;
						break;
					}

					// Process according to current parser state
					switch ( state )
					{
						// Parser expects key
						case s_key:
						{
							if ( *text == char( '{' ) )   // Brace opening found
							{
								if ( !last )
									BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( "unexpected {", "", 0 ) );
								stack.push( last );
								last = NULL;
								++text;
							}
							else if ( *text == char( '}' ) )  // Brace closing found
							{
								if ( stack.size() <= 1 )
									BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( "unmatched }", "", 0 ) );
								stack.pop();
								last = NULL;
								++text;
							}
							else    // Key text found
							{
								std::basic_string<char> key = boost::property_tree::info_parser::read_key( text );
								last = &stack.top()->push_back(
									std::make_pair( key, Ptree() ) )->second;
								state = s_data;
							}

						}; break;

						// Parser expects data
						case s_data:
						{
							// Last ptree must be defined because we are going to add data to it
							BOOST_ASSERT( last );

							if ( *text == char( '{' ) )   // Brace opening found
							{
								stack.push( last );
								last = NULL;
								++text;
								state = s_key;
							}
							else if ( *text == char( '}' ) )  // Brace closing found
							{
								if ( stack.size() <= 1 )
									BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( "unmatched }", "", 0 ) );
								stack.pop();
								last = NULL;
								++text;
								state = s_key;
							}
							else    // Data text found
							{
								bool need_more_lines;
								std::basic_string<char> data = boost::property_tree::info_parser::read_data( text, &need_more_lines );
								last->data() = data;
								state = need_more_lines ? s_data_cont : s_key;
							}


						}; break;

						// Parser expects continuation of data after \ on previous line
						case s_data_cont:
						{
							// Last ptree must be defined because we are going to update its data
							BOOST_ASSERT( last );

							if ( *text == char( '\"' ) )  // Continuation must start with "
							{
								bool need_more_lines;
								std::basic_string<char> data = boost::property_tree::info_parser::read_string( text, &need_more_lines );
								last->put_value( last->template get_value<std::basic_string<char> >() + data );
								state = need_more_lines ? s_data_cont : s_key;
							}
							else
								BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( "expected \" after \\ in previous line", "", 0 ) );

						}; break;

						// Should never happen
						default:
							BOOST_ASSERT( 0 );

					}
				}
			}

			// Check if stack has initial size, otherwise some {'s have not been closed
			if ( stack.size() != 1 )
				BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( "unmatched {", "", 0 ) );

		}
		catch ( boost::property_tree::info_parser::info_parser_error & e )
		{
			// If line undefined rethrow error with correct filename and line
			if ( e.line() == 0 )
			{
				BOOST_PROPERTY_TREE_THROW( boost::property_tree::info_parser::info_parser_error::info_parser_error( e.message(), filename, line_no ) );
			}
			else
				BOOST_PROPERTY_TREE_THROW( e );

		}
	}

	template <typename DataType>
	int32_t CreateDataValueArrayString( std::string& datastring, const std::string fmtstr,
										const int32_t datacount, const int32_t startindex,
										const std::vector<DataType>& datalist )
	{
		if ( fmtstr.length() <= 0 )
		{
			return 0;
		}

		int32_t x = 0;
		int32_t arrayIndex = startindex;

		// if < 0, the loop will terminate immediately, and return 0 elements added
		if ( ( datacount > 0 ) && ( ( int ) ( datacount + startindex ) <= datalist.size() ) )
		{
			std::string workingString;
			std::string valuesString;

			workingString.clear();
			valuesString.clear();

			// if < 0, the loop will terminate immediately, and return 0 elements added
			for ( x = 0; x < datacount; x++, arrayIndex++ )
			{
				// format the value and add to the working data values array content string
				// NOTE that the format definition and data value size is NOT automatically interpreted by the template!
				workingString = boost::str( boost::format( fmtstr ) % datalist.at( arrayIndex ) );
				if ( ( x + 1 ) < datacount )
				{
					workingString.append( "," );
				}
				valuesString.append( workingString );
			}

			if ( x > 0 )
			{
				datastring.append( "ARRAY[" );    // the data must be formatted using the ARRAY designator for PosstgreSQL array insertions (may also be SQL compliant)
				datastring.append( valuesString );
				datastring.append( "]" );         // the array data must be contained within the square brackets for SQL array insertions
			}
		}
		else
		{
			datastring.append( "ARRAY[]" );
		}

		return( x );  // return the number of data elements added to the string
	}


public:
	bool                DBifInit( void );
	void                GetDBConnectionProperties( std::string& dbaddr, std::string& dbport,
	                                               std::string& dbname, std::string& dbdriver );
	bool                SetDBConnectionProperties( std::string dbaddr = DBAddrStr, std::string dbport = DBPortStr,
	                                               std::string dbname = DBNameStr, std::string dbdriver = DBDriverStr );
	bool                SetDBName( std::string dbname = "" );
	bool                SetDBAddress( std::string addr = "" );
	bool                SetDBAddress( uint32_t addrWord = 0 );
	bool                SetDBAddress( uint32_t addrWord1 = 0, uint32_t addrWord2 = 0, uint32_t addrWord3 = 0, uint32_t addrWord4 = 0 );
	bool                SetDBPort( std::string portstr = "" );
	bool                SetDBPort( int32_t portnum = -1 );
	bool                RecreateDb( std::string dbname = "" );
	bool                SetBackupUserPwd( DBApi::eQueryResult& resultcode, std::string& pwdstr );
	bool                TruncateTableContents(DBApi::eQueryResult& resultcode, std::list<std::pair<std::string, std::string>> tableNames_name_schema);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB connection support; DBif_Connect.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool                IsDBPresent( void );                                                          // checks for connection to database
	void                Disconnect( void );
	bool                IsLoginType( DBApi::eLoginType usertype );                                    // connect to database as standard user
	DBApi::eLoginType   GetLoginType( void );                                                         // returns the connection user - type
	DBApi::eLoginType   SetLoginType( DBApi::eLoginType usertype );                                   // selects the connection type to use
	DBApi::eLoginType   LoginAsInstrument( void );                                                              // connect to the database under the instrument user account
	DBApi::eLoginType   LoginAsUser( std::string unamestr = "",
	                                 std::string passwdhash = "", std::string passwdstr = "" );       // connect to database as the specified user
	DBApi::eLoginType   LoginAsAdmin( std::string unamestr = "",
	                                  std::string passwdhash = "", std::string passwdstr = "" );      // connect to database as an Admin user
	bool                LogoutUserType( DBApi::eLoginType usertype );                                 // disconnect from database under std user login


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB utility support; DBif_UtilsData.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void                GetDbCurrentTimeString( std::string& time_string );
	void                GetDbCurrentTimeString( std::string& time_string, system_TP& new_TP );
	void                GetDbTimeString( system_TP time_pt, std::string& time_string, bool isutctp = false );
	bool                GetTimePtFromDbTimeString( system_TP& time_pt, std::string time_string );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB utility support; DBif_UtilsQuery.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void				GetQueryResultString( DBApi::eQueryResult resultcode, std::string& resultstring );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  DB query execution support; DBif_Exec.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DBApi::eQueryResult ExecuteQuery( std::string query_str,
									  std::vector<std::string>& resultlist );							// execute query under the automatically determined connection type
	DBApi::eQueryResult ExecuteQueryType( DBApi::eLoginType usertype,											// execute query under the selected login connection type
										  std::string query_str,
										  std::vector<std::string>& resultlist );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Work queue, Work Queue Item, and Sample primary interface methods; DBif_Impl.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DBApi::eQueryResult RetrieveList_Worklists( std::vector<DB_WorklistRecord>& wllist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Worklists( std::vector<DB_WorklistRecord>& wllist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::FirstLevelObjs,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindWorklist( DB_WorklistRecord& wlr, uuid__t worklistid );
	DBApi::eQueryResult AddWorklist( DB_WorklistRecord& wlr );
	DBApi::eQueryResult SaveWorklistTemplate( DB_WorklistRecord& wlr );
	DBApi::eQueryResult AddWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid );
	DBApi::eQueryResult AddWorklistSampleSet( uuid__t ssrid, uuid__t worklistid );
	DBApi::eQueryResult RemoveWorklistSampleSet( DB_SampleSetRecord& ssr, uuid__t worklistid );
	DBApi::eQueryResult RemoveWorklistSampleSet( uuid__t ssrid, uuid__t worklistid );
	DBApi::eQueryResult RemoveWorklistSampleSets( std::vector<uuid__t>& guidlist, uuid__t worklistid );
	DBApi::eQueryResult ModifyWorklist( DB_WorklistRecord& wlr );
	DBApi::eQueryResult ModifyWorklistTemplate( DB_WorklistRecord& ssr );
	DBApi::eQueryResult SetWorklistStatus( uuid__t worklistid, DBApi::eWorklistStatus status );
	DBApi::eQueryResult SetWorklistSampleSetStatus( uuid__t worklistid, uuid__t samplesetid, DBApi::eSampleSetStatus status );
	DBApi::eQueryResult RemoveWorklist( DB_WorklistRecord& wlr );
	DBApi::eQueryResult RemoveWorklist( uuid__t worklistid );
	DBApi::eQueryResult RemoveWorklists( std::vector<uuid__t>& guidlist );

	DBApi::eQueryResult RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist,
												 std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops, std::vector<std::string> comparevals,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist,
												 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												 std::string compareop = "", std::string compareval = "",
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::NoSubObjs,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, std::vector<uuid__t>& ssidlist,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eContainedObjectRetrieval get_sub_items = DBApi::eContainedObjectRetrieval::AllSubObjs,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, DB_WorklistRecord& wlr,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_SampleSets( std::vector<DB_SampleSetRecord>& ssrlist, uuid__t wlid,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult FindSampleSet( DB_SampleSetRecord& ssr, uuid__t samplesetid );
	DBApi::eQueryResult AddSampleSet( DB_SampleSetRecord& ssr );
	DBApi::eQueryResult SaveSampleSetTemplate( DB_SampleSetRecord& ssr );
	DBApi::eQueryResult AddSampleSetLineItem( DB_SampleItemRecord& ssir, uuid__t samplesetid );
	DBApi::eQueryResult AddSampleSetLineItem( uuid__t itemid, uuid__t samplesetid );
	DBApi::eQueryResult RemoveSampleSetLineItem( DB_SampleItemRecord& ssir, uuid__t samplesetid );
	DBApi::eQueryResult RemoveSampleSetLineItem( uuid__t itemid, uuid__t samplesetid );
	DBApi::eQueryResult RemoveSampleSetLineItems( std::vector<uuid__t>& guidlist, uuid__t samplesetid );
	DBApi::eQueryResult ModifySampleSet( DB_SampleSetRecord& ssr );
	DBApi::eQueryResult ModifySampleSetTemplate( DB_SampleSetRecord& ssr );
	DBApi::eQueryResult SetSampleSetStatus( uuid__t samplesetid, DBApi::eSampleSetStatus status );
	DBApi::eQueryResult SetSampleSetLineItemStatus( uuid__t samplesetid, uuid__t itemid, DBApi::eSampleItemStatus status );
	DBApi::eQueryResult RemoveSampleSet( DB_SampleSetRecord& ssr );
	DBApi::eQueryResult RemoveSampleSet( uuid__t samplesetid );
	DBApi::eQueryResult RemoveSampleSets( std::vector<uuid__t>& guidlist );

	DBApi::eQueryResult RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssilist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssilist,
												  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												  std::string compareop = "", std::string compareval = "",
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssilist, std::vector<uuid__t>& ssiidlist,
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssilist, DB_SampleSetRecord ssr,
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_SampleItems( std::vector<DB_SampleItemRecord>& ssilist, uuid__t ssid,
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult FindSampleItem( DB_SampleItemRecord& wlir, uuid__t itemid );
	DBApi::eQueryResult AddSampleItem( DB_SampleItemRecord& wlir );
	DBApi::eQueryResult ModifySampleItem( DB_SampleItemRecord& wlir );
	DBApi::eQueryResult SetSampleItemStatus( uuid__t itemid, DBApi::eSampleItemStatus status );
	DBApi::eQueryResult RemoveSampleItem( DB_SampleItemRecord& wlir );
	DBApi::eQueryResult RemoveSampleItem( uuid__t itemid );
	DBApi::eQueryResult RemoveSampleItems( std::vector<uuid__t>& guidlist );

	DBApi::eQueryResult RetrieveList_Samples( std::vector<DB_SampleRecord>& samplelist,
											  std::vector<DBApi::eListFilterCriteria> filtertypes,
											  std::vector<std::string> compareops, std::vector<std::string> comparevals,
											  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  int32_t sortdir = 0, std::string orderstring = "",
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Samples( std::vector<DB_SampleRecord>& samplelist,
											  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											  std::string compareop = "", std::string compareval = "",
											  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											  int32_t sortdir = 0, std::string orderstring = "",
											  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ExportSamples( std::vector<DB_SampleRecord>& samplelist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													system_TP lastruntime, std::string custom_query = "" );
	DBApi::eQueryResult FindSample( DB_SampleRecord& sr, uuid__t sampleid );
	DBApi::eQueryResult AddSample( DB_SampleRecord& sr );
	DBApi::eQueryResult ModifySample( DB_SampleRecord& sr );
	DBApi::eQueryResult SetSampleStatus( uuid__t sampleid, DBApi::eSampleStatus status );
	DBApi::eQueryResult RemoveSample( DB_SampleRecord& sr );
	DBApi::eQueryResult RemoveSample( uuid__t sampleid );
	DBApi::eQueryResult RemoveSamples( std::vector<uuid__t>& guidlist );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Analysis and data methods; DBif_Impl.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DBApi::eQueryResult RetrieveList_Analyses( std::vector<DB_AnalysisRecord>& anlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Analyses( std::vector<DB_AnalysisRecord>& anlist,
											   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											   std::string compareop = "", std::string compareval = "",
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ExportAnalyses( std::vector<DB_AnalysisRecord>& anlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 system_TP lastruntime, std::string custom_query = "" );
	DBApi::eQueryResult FindAnalysis( DB_AnalysisRecord& ar, uuid__t analysisid );
	DBApi::eQueryResult AddAnalysis( DB_AnalysisRecord& ar );
	DBApi::eQueryResult ModifyAnalysis( DB_AnalysisRecord& ar );
	DBApi::eQueryResult RemoveAnalysis( DB_AnalysisRecord& ar );
	DBApi::eQueryResult RemoveAnalysis( uuid__t analysisid );
	DBApi::eQueryResult RemoveAnalysisBySampleId( uuid__t sampleid );
	DBApi::eQueryResult RemoveAnalysisByUuidList( std::vector<uuid__t>& idlist );

	DBApi::eQueryResult RetrieveList_SummaryResults( std::vector<DB_SummaryResultRecord>& srlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SummaryResults( std::vector<DB_SummaryResultRecord>& srlist,
													 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													 std::string compareop = "", std::string compareval = "",
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindSummaryResult( DB_SummaryResultRecord& sr, uuid__t resultid );
	DBApi::eQueryResult AddSummaryResult( DB_SummaryResultRecord& sr );
	DBApi::eQueryResult ModifySummaryResult( DB_SummaryResultRecord& sr );
	DBApi::eQueryResult RemoveSummaryResult( DB_SummaryResultRecord& sr );
	DBApi::eQueryResult RemoveSummaryResult( uuid__t resultid );

	DBApi::eQueryResult RetrieveList_DetailedResults( std::vector<DB_DetailedResultRecord>& irlist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													  int32_t sortdir = 0, std::string orderstring = "",
													  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_DetailedResults( std::vector<DB_DetailedResultRecord>& irlist,
													  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													  std::string compareop = "", std::string compareval = "",
													  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													  int32_t sortdir = 0, std::string orderstring = "",
													  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindDetailedResult( DB_DetailedResultRecord& ir, uuid__t resultid );
	DBApi::eQueryResult AddDetailedResult( DB_DetailedResultRecord& ir );
	DBApi::eQueryResult RemoveDetailedResult( DB_DetailedResultRecord& ir );
	DBApi::eQueryResult RemoveDetailedResult( uuid__t resultid );

	DBApi::eQueryResult RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irmlist,
												   std::vector<DBApi::eListFilterCriteria> filtertypes,
												   std::vector<std::string> compareops, std::vector<std::string> comparevals,
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irmlist,
												   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												   std::string compareop = "", std::string compareval = "",
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageResults( std::vector<DB_ImageResultRecord>& irmlist, std::vector<uuid__t>& imridlist,
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult FindImageResult( DB_ImageResultRecord& ir, uuid__t irid );
	DBApi::eQueryResult FindImageResult( DB_ImageResultRecord& ir, int64_t idnum = NO_ID_NUM);
	DBApi::eQueryResult AddImageResult( DB_ImageResultRecord& ir );
	DBApi::eQueryResult RemoveImageResult( DB_ImageResultRecord& ir );
	DBApi::eQueryResult RemoveImageResult( uuid__t mapid );
	DBApi::eQueryResult RemoveImageResults( std::vector<uuid__t>& idlist );

	DBApi::eQueryResult RetrieveList_SResults( std::vector<DB_SResultRecord>& srlist,
												   std::vector<DBApi::eListFilterCriteria> filtertypes,
												   std::vector<std::string> compareops, std::vector<std::string> comparevals,
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SResults( std::vector<DB_SResultRecord>& srlist,
												   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												   std::string compareop = "", std::string compareval = "",
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindSResult( DB_SResultRecord& sr, uuid__t srid );
	DBApi::eQueryResult AddSResult( DB_SResultRecord& sr );
	DBApi::eQueryResult RemoveSResult( DB_SResultRecord& sr );
	DBApi::eQueryResult RemoveSResult( uuid__t srid );

	DBApi::eQueryResult RetrieveList_ImageSets( std::vector<DB_ImageSetRecord>& islist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageSets( std::vector<DB_ImageSetRecord>& islist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindImageSet( DB_ImageSetRecord& isr, uuid__t imagesetid );
	DBApi::eQueryResult AddImageSet( DB_ImageSetRecord& isr );
	DBApi::eQueryResult ModifyImageSet( DB_ImageSetRecord& isr );
	DBApi::eQueryResult RemoveImageSet( DB_ImageSetRecord& isr );        // also removes all image records and images belonging to the set
	DBApi::eQueryResult RemoveImageSet( uuid__t imagesetid );

	DBApi::eQueryResult RetrieveList_ImageSequences( std::vector<DB_ImageSeqRecord>& isrlist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageSequences( std::vector<DB_ImageSeqRecord>& isrlist,
													 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													 std::string compareop = "", std::string compareval = "",
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindImageSequence( DB_ImageSeqRecord& isr, uuid__t imageseqid );
	DBApi::eQueryResult AddImageSequence( DB_ImageSeqRecord& isr );
	DBApi::eQueryResult ModifyImageSequence( DB_ImageSeqRecord& isr );
	DBApi::eQueryResult RemoveImageSequence( DB_ImageSeqRecord& isr );     // also removes all images belonging to the record
	DBApi::eQueryResult RemoveImageSequence( uuid__t imageseqid );
	DBApi::eQueryResult RemoveImageSequences( std::vector<DB_ImageSeqRecord>& imageseqlist );
	DBApi::eQueryResult RemoveImageSequencesByIdList( std::vector<uuid__t>& imageseqidlist );

	DBApi::eQueryResult RetrieveList_Images( std::vector<DB_ImageRecord>& irlist,
											 std::vector<DBApi::eListFilterCriteria> filtertypes,
											 std::vector<std::string> compareops, std::vector<std::string> comparevals,
											 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											 int32_t sortdir = 0, std::string orderstring = "",
											 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Images( std::vector<DB_ImageRecord>& irlist,
											 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											 std::string compareop = "", std::string compareval = "",
											 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											 int32_t sortdir = 0, std::string orderstring = "",
											 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindImage( DB_ImageRecord& ir, uuid__t imageid );
	DBApi::eQueryResult AddImage( DB_ImageRecord& ir );
	DBApi::eQueryResult ModifyImage( DB_ImageRecord& ir );
	DBApi::eQueryResult RemoveImage( DB_ImageRecord& ir );               // removes the entire image reference record; should cascade up to image sets and image records
	DBApi::eQueryResult RemoveImage( uuid__t imageid );
	DBApi::eQueryResult RemoveImages( std::vector<DB_ImageRecord>& imagelist );
	DBApi::eQueryResult RemoveImagesByIdList( std::vector<uuid__t>& imageidlist );


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Instrument config and analysis parameter primary interface methods; DBif_Impl.cpp
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DBApi::eQueryResult RetrieveList_CellType( std::vector<DB_CellTypeRecord>& ctlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_CellType( std::vector<DB_CellTypeRecord>& ctlist,
											   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											   std::string compareop = "", std::string compareval = "",
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindCellType( DB_CellTypeRecord& ctr, uuid__t celltypeid );
	DBApi::eQueryResult FindCellType( DB_CellTypeRecord& ctr, int64_t celltypeindex );
	DBApi::eQueryResult AddCellType( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult ModifyCellType( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult RemoveCellType( DB_CellTypeRecord& ctr );
	DBApi::eQueryResult RemoveCellType( uuid__t celltypeid );

	DBApi::eQueryResult RetrieveList_ImageAnalysisParameters( std::vector<DB_ImageAnalysisParamRecord>& aplist,
															  std::vector<DBApi::eListFilterCriteria> filtertypes,
															  std::vector<std::string> compareops, std::vector<std::string> comparevals,
															  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															  int32_t sortdir = 0, std::string orderstring = "",
															  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageAnalysisParameters( std::vector<DB_ImageAnalysisParamRecord>& aplist,
															  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
															  std::string compareop = "", std::string compareval = "",
															  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
															  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
															  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
															  int32_t sortdir = 0, std::string orderstring = "",
															  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindImageAnalysisParameter( DB_ImageAnalysisParamRecord& params, uuid__t paramid );
	DBApi::eQueryResult AddImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
	DBApi::eQueryResult ModifyImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
	DBApi::eQueryResult RemoveImageAnalysisParameter( DB_ImageAnalysisParamRecord& params );
	DBApi::eQueryResult RemoveImageAnalysisParameter( uuid__t paramid );

	DBApi::eQueryResult RetrieveList_AnalysisInputSettings( std::vector<DB_AnalysisInputSettingsRecord>& islist,
																 std::vector<DBApi::eListFilterCriteria> filtertypes,
																 std::vector<std::string> compareops, std::vector<std::string> comparevals,
																 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																 int32_t sortdir = 0, std::string orderstring = "",
																 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_AnalysisInputSettings( std::vector<DB_AnalysisInputSettingsRecord>& islist,
																 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																 std::string compareop = "", std::string compareval = "",
																 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																 int32_t sortdir = 0, std::string orderstring = "",
																 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params, uuid__t paramid );
	DBApi::eQueryResult AddAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params );
	DBApi::eQueryResult ModifyAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params );
	DBApi::eQueryResult RemoveAnalysisInputSettings( DB_AnalysisInputSettingsRecord& params );
	DBApi::eQueryResult RemoveAnalysisInputSettings( uuid__t paramid );

#if(0)
	DBApi::eQueryResult RetrieveList_ImageAnalysisBlobIdentParam( std::vector<DB_BlobIdentParamRecord>& islist,
																   std::vector<DBApi::eListFilterCriteria> filtertypes,
																   std::vector<std::string> compareops, std::vector<std::string> comparevals,
																   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																   int32_t sortdir = 0, std::string orderstring = "",
																   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_ImageAnalysisBlobIdentParam( std::vector<DB_BlobIdentParamRecord>& islist,
																   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
																   std::string compareop = "", std::string compareval = "",
																   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
																   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
																   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
																   int32_t sortdir = 0, std::string orderstring = "",
																   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params, uuid__t paramid );
	DBApi::eQueryResult AddImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params );
	DBApi::eQueryResult ModifyImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params );
	DBApi::eQueryResult RemoveImageAnalysisBlobIdentParam( DB_BlobIdentParamRecord& params );
	DBApi::eQueryResult RemoveImageAnalysisBlobIdentParam( uuid__t paramid );
#endif

	DBApi::eQueryResult RetrieveList_AnalysisDefinitions( std::vector<DB_AnalysisDefinitionRecord>& adlist,
														  std::vector<DBApi::eListFilterCriteria> filtertypes,
														  std::vector<std::string> compareops, std::vector<std::string> comparevals,
														  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														  int32_t sortdir = 0, std::string orderstring = "",
														  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_AnalysisDefinitions( std::vector<DB_AnalysisDefinitionRecord>& adlist,
														  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
														  std::string compareop = "", std::string compareval = "",
														  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														  int32_t sortdir = 0, std::string orderstring = "",
														  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindAnalysisDefinition( DB_AnalysisDefinitionRecord& def, uuid__t defid );
	DBApi::eQueryResult FindAnalysisDefinition( DB_AnalysisDefinitionRecord& def, int32_t defindex );
	DBApi::eQueryResult AddAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
	DBApi::eQueryResult ModifyAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
	DBApi::eQueryResult RemoveAnalysisDefinition( DB_AnalysisDefinitionRecord& def );
	DBApi::eQueryResult RemoveAnalysisDefinition( uuid__t defid );

	DBApi::eQueryResult RetrieveList_AnalysisParameters( std::vector<DB_AnalysisParamRecord>& aplist,
														 std::vector<DBApi::eListFilterCriteria> filtertypes,
														 std::vector<std::string> compareops, std::vector<std::string> comparevals,
														 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														 int32_t sortdir = 0, std::string orderstring = "",
														 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_AnalysisParameters( std::vector<DB_AnalysisParamRecord>& aplist,
														 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
														 std::string compareop = "", std::string compareval = "",
														 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
														 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
														 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
														 int32_t sortdir = 0, std::string orderstring = "",
														 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindAnalysisParameter( DB_AnalysisParamRecord& params, uuid__t paramid );
	DBApi::eQueryResult AddAnalysisParameter( DB_AnalysisParamRecord& params );
	DBApi::eQueryResult ModifyAnalysisParameter( DB_AnalysisParamRecord& params );
	DBApi::eQueryResult RemoveAnalysisParameter( DB_AnalysisParamRecord& params );
	DBApi::eQueryResult RemoveAnalysisParameter( uuid__t paramid );

	DBApi::eQueryResult RetrieveList_Illuminators( std::vector<DB_IlluminatorRecord>& illist,
												   std::vector<DBApi::eListFilterCriteria> filtertypes,
												   std::vector<std::string> compareops, std::vector<std::string> comparevals,
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Illuminators( std::vector<DB_IlluminatorRecord>& illist,
												   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												   std::string compareop = "", std::string compareval = "",
												   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												   int32_t sortdir = 0, std::string orderstring = "",
												   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindIlluminator( DB_IlluminatorRecord& ilr, std::string ilname = "", int16_t ilindex = INVALID_INDEX );
	DBApi::eQueryResult FindIlluminator( DB_IlluminatorRecord& ilr, int16_t emission = 0, int16_t illuminator = 0 );
	DBApi::eQueryResult AddIlluminator( DB_IlluminatorRecord& ilr );
	DBApi::eQueryResult ModifyIlluminator( DB_IlluminatorRecord& ilr );
	DBApi::eQueryResult RemoveIlluminator( DB_IlluminatorRecord& ilr );
	DBApi::eQueryResult RemoveIlluminator( int16_t ilindex = INVALID_INDEX, int64_t idnum = NO_ID_NUM );
	DBApi::eQueryResult RemoveIlluminator( std::string ilname = "" );

	DBApi::eQueryResult RetrieveList_Users( std::vector<DB_UserRecord>& urlist,
											std::vector<DBApi::eListFilterCriteria> filtertypes,
											std::vector<std::string> compareops, std::vector<std::string> comparevals,
											int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											int32_t sortdir = 0, std::string orderstring = "",
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Users( std::vector<DB_UserRecord>& urlist,
											DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											std::string compareop = "", std::string compareval = "",
											int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											int32_t sortdir = 0, std::string orderstring = "",
											int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindUser( DB_UserRecord& ur, uuid__t userid );
	DBApi::eQueryResult FindUser( DB_UserRecord& ur, std::string username = "", DBApi::eUserType user_type = DBApi::eUserType::AllUsers );
	DBApi::eQueryResult AddUser( DB_UserRecord& ur );
	DBApi::eQueryResult ModifyUser( DB_UserRecord& ur );
	DBApi::eQueryResult RemoveUser( DB_UserRecord& ur );
	DBApi::eQueryResult RemoveUser( uuid__t userid );
	DBApi::eQueryResult RemoveUser( std::string username = "" );

	DBApi::eQueryResult RetrieveList_UserRoles( std::vector<DB_UserRoleRecord>& rrlist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_UserRoles( std::vector<DB_UserRoleRecord>& rrlist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindUserRole( DB_UserRoleRecord& rr, uuid__t roleid );
	DBApi::eQueryResult FindUserRole( DB_UserRoleRecord& rr, std::string rolename = "" );
	DBApi::eQueryResult AddUserRole( DB_UserRoleRecord& rr );
	DBApi::eQueryResult ModifyUserRole( DB_UserRoleRecord& rr );
	DBApi::eQueryResult RemoveUserRole( DB_UserRoleRecord& rr );
	DBApi::eQueryResult RemoveUserRole( uuid__t roleid );
	DBApi::eQueryResult RemoveUserRole( std::string username = "" );

	DBApi::eQueryResult RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist,
													 std::vector<DBApi::eListFilterCriteria> filtertypes,
													 std::vector<std::string> compareops, std::vector<std::string> comparevals,
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist,
													 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													 std::string compareop = "", std::string compareval = "",
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist, uuid__t userid,
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_UserProperties( std::vector<DB_UserPropertiesRecord>& uplist, std::string username = "",
													 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													 int32_t sortdir = 0, std::string orderstring = "",
													 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult FindUserProperty( DB_UserPropertiesRecord& up, int32_t propindex );
	DBApi::eQueryResult FindUserProperty( DB_UserPropertiesRecord& up, std::string propname = "" );
	DBApi::eQueryResult AddUserProperty( DB_UserPropertiesRecord& up );
	DBApi::eQueryResult RemoveUserProperty( DB_UserPropertiesRecord& up );
	DBApi::eQueryResult RemoveUserProperty( int32_t propindex );
	DBApi::eQueryResult RemoveUserProperty( std::string propname = "" );

	DBApi::eQueryResult RetrieveList_SignatureDefs( std::vector<DB_SignatureRecord>& siglist,
													std::vector<DBApi::eListFilterCriteria> filtertypes,
													std::vector<std::string> compareops, std::vector<std::string> comparevals,
													int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													int32_t sortdir = 0, std::string orderstring = "",
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_SignatureDefs( std::vector<DB_SignatureRecord>& siglist,
													DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													std::string compareop = "", std::string compareval = "",
													int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													int32_t sortdir = 0, std::string orderstring = "",
													int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindSignatureDef( DB_SignatureRecord& sigrec, uuid__t sigid );
	DBApi::eQueryResult AddSignatureDef( DB_SignatureRecord& sigrec );
	DBApi::eQueryResult RemoveSignatureDef( DB_SignatureRecord& sigrec );
	DBApi::eQueryResult RemoveSignatureDef( uuid__t sigid );


	DBApi::eQueryResult RetrieveList_Reagents( std::vector<DB_ReagentTypeRecord>& rxlist,
											   std::vector<DBApi::eListFilterCriteria> filtertypes,
											   std::vector<std::string> compareops, std::vector<std::string> comparevals,
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Reagents( std::vector<DB_ReagentTypeRecord>& rxlist,
											   DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
											   std::string compareop = "", std::string compareval = "",
											   int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
											   DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
											   DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
											   int32_t sortdir = 0, std::string orderstring = "",
											   int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindReagent( DB_ReagentTypeRecord& rxr, int64_t rxidnum = NO_ID_NUM );
	DBApi::eQueryResult FindReagent( DB_ReagentTypeRecord& rxr, std::string tagsn = "" );
	DBApi::eQueryResult AddReagent( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult ModifyReagent( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult RemoveReagent( DB_ReagentTypeRecord& rxr );
	DBApi::eQueryResult RemoveReagent( int64_t rxidnum );
	DBApi::eQueryResult RemoveReagent( std::string tagsn = "" );
	DBApi::eQueryResult RemoveReagents( std::vector<int64_t> idnumlist );
	DBApi::eQueryResult RemoveReagents( std::string tagsn = "" );
	DBApi::eQueryResult RemoveReagentLot( std::string lotnum = "" );

	DBApi::eQueryResult RetrieveList_CellHealthReagents(std::vector<DB_CellHealthReagentRecord>& chrlist,
		DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
		std::string compareop = "", std::string compareval = "",
		int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
		DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
		int32_t sortdir = 0, std::string orderstring = "",
		int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "");
	DBApi::eQueryResult FindCellHealthReagent(DB_CellHealthReagentRecord& rxr, int16_t type=0);
	DBApi::eQueryResult ModifyCellHealthReagent(DB_CellHealthReagentRecord& rxr);
	
	DBApi::eQueryResult RetrieveList_Workflows( std::vector<DB_WorkflowRecord>& wflist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Workflows( std::vector<DB_WorkflowRecord>& wflist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindWorkflow( DB_WorkflowRecord& wfr, uuid__t rxid );
	DBApi::eQueryResult FindWorkflow( DB_WorkflowRecord& wfr, std::string wfname = "" );
	DBApi::eQueryResult AddWorkflow( DB_WorkflowRecord& wfr );
	DBApi::eQueryResult ModifyWorkflow( DB_WorkflowRecord& wfr );
	DBApi::eQueryResult RemoveWorkflow( DB_WorkflowRecord& wfr );
	DBApi::eQueryResult RemoveWorkflow( uuid__t wfid );
	DBApi::eQueryResult RemoveWorkflow( std::string wfname = "" );

	DBApi::eQueryResult RetrieveList_BioProcess( std::vector<DB_BioProcessRecord>& bplist,
												 std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops, std::vector<std::string> comparevals,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_BioProcess( std::vector<DB_BioProcessRecord>& bplist,
												 DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												 std::string compareop = "", std::string compareval = "",
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 int32_t sortdir = 0, std::string orderstring = "",
												 int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindBioProcess( DB_BioProcessRecord& bpr, uuid__t bpid );
	DBApi::eQueryResult FindBioProcess( DB_BioProcessRecord& bpr, std::string bpname = "" );
	DBApi::eQueryResult AddBioProcess( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult ModifyBioProcess( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult RemoveBioProcess( DB_BioProcessRecord& bpr );
	DBApi::eQueryResult RemoveBioProcess( uuid__t bpid );
	DBApi::eQueryResult RemoveBioProcess( std::string bpname = "" );

	DBApi::eQueryResult RetrieveList_QcProcess( std::vector<DB_QcProcessRecord>& qclist,
												std::vector<DBApi::eListFilterCriteria> filtertypes,
												std::vector<std::string> compareops, std::vector<std::string> comparevals,
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_QcProcess( std::vector<DB_QcProcessRecord>& qclist,
												DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												std::string compareop = "", std::string compareval = "",
												int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												int32_t sortdir = 0, std::string orderstring = "",
												int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindQcProcess( DB_QcProcessRecord& qcr, uuid__t qcid );
	DBApi::eQueryResult FindQcProcess( DB_QcProcessRecord& qcr, std::string qcname = "" );
	DBApi::eQueryResult AddQcProcess( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult ModifyQcProcess( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult RemoveQcProcess( DB_QcProcessRecord& qcr );
	DBApi::eQueryResult RemoveQcProcess( uuid__t qcid );
	DBApi::eQueryResult RemoveQcProcess( std::string qcname = "" );

	DBApi::eQueryResult RetrieveList_Calibration( std::vector<DB_CalibrationRecord>& callist,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult RetrieveList_Calibration( std::vector<DB_CalibrationRecord>& callist,
												  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
												  std::string compareop = "", std::string compareval = "",
												  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												  int32_t sortdir = 0, std::string orderstring = "",
												  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END, std::string custom_query = "" );
	DBApi::eQueryResult FindCalibration( DB_CalibrationRecord& car, uuid__t calid );
	DBApi::eQueryResult AddCalibration( DB_CalibrationRecord& car );
	DBApi::eQueryResult RemoveCalibration( DB_CalibrationRecord& car );
	DBApi::eQueryResult RemoveCalibration( uuid__t calid );

	DBApi::eQueryResult RetrieveListInstConfig( std::vector<DB_InstrumentConfigRecord>& config_list );
	DBApi::eQueryResult FindInstrumentConfig( DB_InstrumentConfigRecord& icr, int64_t inst_idnum = NO_ID_NUM, std::string instsn = "" );
	DBApi::eQueryResult AddInstrumentConfig( DB_InstrumentConfigRecord& icr );
	DBApi::eQueryResult ModifyInstrumentConfig( DB_InstrumentConfigRecord& icr );
	DBApi::eQueryResult RemoveInstrumentConfig( DB_InstrumentConfigRecord& icr );
	DBApi::eQueryResult RemoveInstrumentConfig( std::string instsn );

	DBApi::eQueryResult RetrieveList_LogEntries( std::vector<DB_LogEntryRecord>& log_list,
												 std::vector<DBApi::eListFilterCriteria> filtertypes,
												 std::vector<std::string> compareops,
												 std::vector<std::string> comparevals,
												 int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
												 DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
												 DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
												 int32_t sortdir = 0 );
	DBApi::eQueryResult	AddLogEntry( DB_LogEntryRecord& log_entry );
	DBApi::eQueryResult	ModifyLogEntry( DB_LogEntryRecord& log_entry );
	DBApi::eQueryResult ClearLogEntries( std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 int64_t startidnum = ID_SRCH_FROM_END, int64_t endidnum = NO_ID_NUM );
	DBApi::eQueryResult ClearLogEntries( std::vector<DBApi::eListFilterCriteria> filtertypes,
										 std::vector<std::string> compareops, std::vector<std::string> comparevals,
										 std::vector<int64_t> idnumlist );

	DBApi::eQueryResult RetrieveList_SchedulerConfig( std::vector<DB_SchedulerConfigRecord>& sclist,
													  std::vector<DBApi::eListFilterCriteria> filtertypes,
													  std::vector<std::string> compareops, std::vector<std::string> comparevals,
													  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													  int32_t sortdir = 0, std::string orderstring = "",
													  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult RetrieveList_SchedulerConfig( std::vector<DB_SchedulerConfigRecord>& sclist,
													  DBApi::eListFilterCriteria filtertype = DBApi::eListFilterCriteria::NoFilter,
													  std::string compareop = "", std::string compareval = "",
													  int32_t limitcnt = DFLT_QUERY_NO_LIST_LIMIT,
													  DBApi::eListSortCriteria primarysort = DBApi::eListSortCriteria::SortNotDefined,
													  DBApi::eListSortCriteria secondarysort = DBApi::eListSortCriteria::SortNotDefined,
													  int32_t sortdir = 0, std::string orderstring = "",
													  int32_t startindex = INVALID_SRCH_INDEX, int64_t startidnum = ID_SRCH_FROM_END );
	DBApi::eQueryResult FindSchedulerConfig( DB_SchedulerConfigRecord& scr, uuid__t scid );
	DBApi::eQueryResult AddSchedulerConfig( DB_SchedulerConfigRecord& scr );
	DBApi::eQueryResult ModifySchedulerConfig( DB_SchedulerConfigRecord& scr );
	DBApi::eQueryResult RemoveSchedulerConfig( DB_SchedulerConfigRecord& scr );
	DBApi::eQueryResult RemoveSchedulerConfig( uuid__t scid );};
