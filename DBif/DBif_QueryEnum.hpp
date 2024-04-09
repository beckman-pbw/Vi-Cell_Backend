// DBif_QueryEnum.h : header file define status and action enumerations for the DBif library
//

#pragma once

#include <stdint.h>


#define	DFLT_QUERY_AND_LIST_LIMIT		 0		// limit query retrieval to the specified default query limit value AND limit returned record list length
#define	DFLT_QUERY_NO_LIST_LIMIT		-1		// limit query retrieval to the specified default query limit value, but ALLOW unlimited returned record list
#define	NO_QUERY_OR_LIST_LIMIT			-2		// allow a query not limited to the specified value for maximum query length; also returns an unlimited record list

#define	INVALID_SRCH_INDEX				-1
#define	ID_SRCH_FROM_BEGINNING			 0		// for search by ID number, to start from the beginning of the records (lowest IdNum -> highest IdNum) to retrieve the most recent 'n' results in the record list
#define	ID_SRCH_FROM_END				-1		// for search by ID number, to start from the end of the records (Highest IdNum -> lowest IdNum) to retrieve the first 'n' results in the record list

#define	NO_ID_NUM						-1
#define	INVALID_INDEX					-1


namespace DBApi
{

	enum eDayBitShift
	{
		SundayShift		= 0,
		MondayShift		= SundayShift + 1,
		TuesdayShift	= MondayShift + 1,
		WednesdayShift	= TuesdayShift + 1,
		ThursdayShift	= WednesdayShift + 1,
		FridayShift		= ThursdayShift + 1,
		SaturdayShift	= FridayShift + 1,
	};

	enum eDayWeekIndicatorBits : uint16_t
	{
		DayBit					= 0x0001,	// first day indicator bit (for Sunday); Sunday = bit0 Saturday = bit6
		DayIndicatorMask		= 0x007F,	// bits 0-6; high bit not included
		AllDays					= DayIndicatorMask,	// bits 0-6; high bit not included
		WeekBit					= 0x0080,	// first week indicator bit; weekly=bit7, 2-weeks=bit8, 3-weeks=bit9, 4-weeks=bit10
		WeekIndicatorMask		= 0x0780,	// bits 7-10; high bit not included
		DayWeekIndicatorMask	= 0x07FF,	// bits 7-10; high bit not included
		WeekMonthIndicatorMask	= 0x0F80,	// bits 7-11; high bit not included
		MonthBit				= 0x0800,	// monthly repetition indicator bit (bit11)
		FullIndicatorMask		= 0x0FFF,	// to mask-off high bit
		IndicatorErrorBit		= 0x8000,	// Error indicator bit
		IndicatorErrorClearMask	= 0x7FFF,	// to mask-off high bit
	};

	enum eContainedObjectRetrieval : int32_t
	{
		FirstLevelObjs = -1,
		NoSubObjs = 0,
		AllSubObjs = 1,
	};

	enum eUserType : int32_t
	{
		LocalUsers = -1,
		AllUsers = 0,
		AdUsers = 1,
	};
	
	enum eDisplayColumns : int32_t
	{
		NoColType = -1,
        SampleStatus,
        SamplePosition,
		SampleName,		// should always be present in GUI
		TotalCells,
		TotalViableCells,
        TotalViability,
        CellTypeQcName,
		Dilution,
        AverageDiameter,
        AverageViableDiameter,
        WashType,
        SampleTag,
	};

	enum class eRoleClass : uint16_t
	{
		NoRoleClass			= 0xFFFF,
		RoleClassUndefined	= 0x0000,		// (0)
		UserClass1			= 0x0001,		// (1)
		UserClass2			= 0x0002,		// (2)
		UserClass3			= 0x0004,		// (4)
		UserClass4			= 0x0008,		// (8)
		AllUserClasses		= 0x000F,		// (15)
		ElevatedClass1		= 0x0010,		// (16)
		ElevatedClass2		= 0x0020,		// (32)
		ElevatedClass3		= 0x0040,		// (64)
		ElevatedClass4		= 0x0080,		// (128)
		AllElevatedClasses	= 0x00F0,		// (240)
		AdminClass1			= 0x0100,		// (256)
		AdminClass2			= 0x0200,		// (512)
		AdminClass3			= 0x0400,		// (1024)
		AdminClass4			= 0x0800,		// (2048)
		AllAdminClasses		= 0x0F00,		// (3840)
		AllRoleClasses		= 0x0FFF,		// (4095)
	};

	enum class eWorklistStatus : int32_t
	{
		NoWorklistStatus = 0,
		WorklistTemplate,
		WorklistNotRun,
		WorklistRunning,
		WorklistComplete,
	};

	enum class eSampleSetStatus : int32_t
	{
		NoSampleSetStatus = 0,
		SampleSetTemplate,
		SampleSetNotRun,
		SampleSetActive,        // sample set is part of the actively running worklist, but none of the contained items may have been processed yet.
		SampleSetInProgress,
		SampleSetRunning,
		SampleSetCanceled,		// for sample sets 'removed' from the processing list
		SampleSetComplete,
	};

	enum class eSampleItemStatus : int32_t
	{
		NoItemStatus = 0,
		ItemTemplate,
		ItemNotRun,
		ItemRunning,
		ItemError,
		ItemSkipped,
		ItemCanceled,
		ItemComplete,
		ItemDeleted,
	};

	enum class eSampleStatus : int32_t
	{
		NoSampleStatus = 0,
		SampleNotRun,
		SampleRunning,
		SampleError,			// may contain partial results, or images acquired but analysis errored
		SampleComplete,
	};

	enum class eLoginType : uint32_t
	{
		NoLogin = 0,
		InstrumentLoginType = 0x01,
		UserLoginType = 0x02,
		AdminLoginType = 0x080,
		BothUserLoginTypes = 0x82,
		EitherUserLoginType = 0x08082,
		InstPlusUserLoginTypes = 0x03,
		InstOrUserLoginTypes = 0x08003,
		InstPlusAdminLoginTypes = 0x081,
		InstOrAdminLoginTypes = 0x08081,
		AllLoginTypes = 0x083,
		AnyLoginType = 0x08083,
		OrTypeMask = 0x08000,
		UserTypeMask = 0x07fff,
	};

	enum class eQueryResult : int32_t
	{
		InvalidUUIDError = -18,
		StatusRegressionNotAllowed = -17,
		DeleteProtectedFailed = -16,
		ParseFailure = -15,
		MultipleObjectsFound = -14,
		InsertObjectExists = -13,
		BadOrMissingListIds = -12,
		BadOrMissingArrayVals = -11,
		MissingQueryKey = -10,
		NoQuery = -9,
		BadQuery = -8,
		NoTargets = -7,
		InsertFailed = -6,
		QueryFailed = -5,
		NotConnected = -4,
		NoData = -3,		// query returned a result record with 0 array elements
		InternalNotSupported = -2,
		ApiNotSupported = -1,
		NoResults = 0,      // no results returned by the query
		QueryOk = 1
	};

	enum class eListType : int32_t
	{
		NoListType = 0,
		WorklistList,
		SampleSetList,
		SampleItemList,
		SampleList,
		AnalysisList,
		SummaryResultList,
		DetailedResultList,
		ImageResultList,
		SResultList,
		ClusterDataList,
		BlobDataList,
		ImageSetList,
		ImageSequenceList,
		ImageList,
		CellTypeList,
		ImageAnalysisParamList,
		AnalysisInputSettingsList,
		ImageAnalysisCellIdentParamList,
		AnalysisDefinitionList,
		AnalysisParamList,
		IlluminatorList,
		RolesList,
		UserList,
		UserPropertiesList,
		SignatureDefList,
		ReagentInfoList,
		WorkflowList,
		BioProcessList,
		QcProcessList,
		CalibrationsList,
		InstrumentConfigList,
		LogEntryList,
		SchedulerConfigList,
		CellHealthReagentsList,
		// always add any new/additional stored table object types to the end of the list
	};

	enum class eListSortCriteria : int32_t
	{
		SortNotDefined = -1,
		NoSort = 0,
		GuidSort,           // not practical using UUIDs to sort
		IdNumSort,
		IndexSort,
		OwnerSort,
		ParentSort,
		CarrierSort,
		CreationDateSort,
		CreationUserSort,
		RunDateSort,
		UserIdSort,         // not practical using UUIDs to sort
		UserIdNumSort,
		UserNameSort,
		SampleIdSort,       // not practical using UUIDs to sort
		SampleIdNumSort,
		LabelSort,
		RxTypeNumSort,
		RxLotNumSort,
		RxLotExpSort,
		ItemNameSort,
		CellTypeSort,
		InstrumentSort,
		QcSort,
		CalTypeSort,
		LogTypeSort,

		MaxSortTypes		// Place all new sort types ahead of this entry
	};

	enum class eListFilterCriteria : int32_t
	{
		FilterNotDefined = -1,
		NoFilter = 0,
		IdFilter,
		IdNumFilter,
		IndexFilter,
		ItemNameFilter,
		LabelFilter,
		StatusFilter,
		OwnerFilter,
		ParentFilter,
		CarrierFilter,
		CreationDateFilter,
		CreationDateRangeFilter,
		CreationUserIdFilter,
		CreationUserNameFilter,
		ModifyDateFilter,
		ModifyDateRangeFilter,
		RunDateFilter,
		RunDateRangeFilter,
		RunUserIdFilter,
		RunUserNameFilter,
		SampleAcquiredFilter,
		AcquisitionDateFilter,
		AcquisitionDateRangeFilter,
		SampleIdFilter,
		CellTypeIdxFilter,
		CellTypeFilter = CellTypeIdxFilter,
		CellTypeIdFilter,
		LotFilter,
		QcFilter,
		InstrumentFilter,
		CommentsFilter,
		RoleFilter,
		UserTypeFilter,
		CalTypeFilter,
		LogTypeFilter,
		SinceLastDateFilter,	// only used by scheduler operations
		IlluminatorType,
		MaxFilterTypes		// Place all new filter types ahead of this entry
	};

	enum class eReagentIdType : int32_t
	{
		NoReagentIdType = 0,
		ReagentNameId,
		ReagentLotId,
		ReagentPackId,
	};

	enum class eCalibrationType : int32_t
	{
		NoCalibrationType = 0,
		ConcCalType,
		SizeCalType,
	};

	enum class eLogEntryType : int16_t
	{
		NoLogType = -1,
		AllLogTypes = 0,
		AuditLogType,
		ErrorLogType,
		SampleLogType,
	};

	enum class eExportRunStatus : int16_t
	{
		NotRun = 0,
		Running,
		Runsuccess,
		RunError,
	};

	enum class eExportTypes : int16_t
	{
		NoExportType = 0,
		ExcryptedExport,
		UnencryptedCsvExport,
	};
}
