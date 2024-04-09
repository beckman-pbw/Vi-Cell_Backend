// DBif_Structs.hpp; definitions for database-used structures
// DBif_Structs.hpp; definitions for database-used structures
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <list>

#include <CharacteristicTypedefs.h>
#include <ErrorCodes.h>
#include <InputConfigurationParams.h>

#include "DBif_QueryEnum.hpp"

#include "AnalysisDefinitionCommon.hpp"
#include "ChronoUtilities.hpp"
#include "uuid__t.hpp"



namespace DBApi
{

#define	DbDefaultConfigSN	"LocalInstrumentDefault"

	typedef struct DB_Illuminator
	{
		int64_t		IlluminatorIdNum;
		int16_t		IlluminatorIndex;
		bool		Protected;
		int16_t		IlluminatorType;
		std::string	IlluminatorNameStr;
		int16_t		PositionNum;
		float		Tolerance;
		int32_t		MaxVoltage;
		int16_t		IlluminatorWavelength;
		int16_t		EmissionWavelength;
		int16_t		ExposureTimeMs;
		int16_t		PercentPower;
		int32_t		SimmerVoltage;
		int16_t		Ltcd;
		int16_t		Ctld;
		int16_t		FeedbackDiode;
	} DB_IlluminatorRecord;

	typedef struct DB_AnalysisParam
	{
		int64_t		ParamIdNum;
		uuid__t		ParamId;
		bool		IsInitialized;
		bool		Protected;

		std::string	ParamLabel;
		Hawkeye::Characteristic_t	Characteristics;
		float		ThresholdValue;
		bool		AboveThreshold;
	} DB_AnalysisParamRecord;

	typedef struct DB_AnalysisDefinition
	{
		int64_t		AnalysisDefIdNum;
		uuid__t		AnalysisDefId;
		bool		Protected;
		uint16_t	AnalysisDefIndex;
		std::string	AnalysisLabel;
		uint8_t		NumReagents;
		std::vector<int32_t>					ReagentIndexList;
		uint16_t	MixingCycles;
		uint8_t		NumFlIlluminators;
		std::vector<DB_IlluminatorRecord>		IlluminatorsList;
		uint8_t		NumAnalysisParams;
		std::vector<DB_AnalysisParamRecord>		AnalysisParamList;
		bool		PopulationParamExists;
		DB_AnalysisParamRecord					PopulationParam;
	} DB_AnalysisDefinitionRecord;

	typedef struct DB_CellType
	{
		int64_t		CellTypeIdNum;
		uuid__t		CellTypeId;
		bool		Protected;
		bool		Retired;
		uint32_t	CellTypeIndex;
		std::string	CellTypeNameStr;
		uint16_t	MaxImageCount;
		uint16_t	AspirationCycles;
		float		MinDiam;
		float		MaxDiam;
		float		MinCircularity;
		float		SharpnessLimit;
		uint8_t		NumCellIdentParams;
		std::vector<DB_AnalysisParamRecord>			CellIdentParamList;
		uint16_t	DeclusterSetting;
		float		RoiExtent;
		uint16_t	RoiXPixels;
		uint16_t	RoiYPixels;
		int16_t		NumAnalysisSpecializations;
		std::vector<DB_AnalysisDefinitionRecord>	SpecializationsDefList;
		float		CalculationAdjustmentFactor;
	} DB_CellTypeRecord;

	typedef struct DB_ImageAnalysisParam
	{
		int64_t		ParamIdNum;
		uuid__t		ParamId;
		bool		Protected;

		int32_t		AlgorithmMode;
		bool		BubbleMode;
		bool		DeclusterMode;
		bool		SubPeakAnalysisMode;
		int32_t		DilutionFactor;
		int32_t		ROI_Xcoords;
		int32_t		ROI_Ycoords;
		int32_t		DeclusterAccumulatorThreshLow;
		int32_t		DeclusterMinDistanceThreshLow;
		int32_t		DeclusterAccumulatorThreshMed;
		int32_t		DeclusterMinDistanceThreshMed;
		int32_t		DeclusterAccumulatorThreshHigh;
		int32_t		DeclusterMinDistanceThreshHigh;
		double		FovDepthMM;
		double		PixelFovMM;
		double		SizingSlope;
		double		SizingIntercept;
		double		ConcSlope;
		double		ConcIntercept;
		int32_t		ConcImageControlCount;
		float		BubbleMinSpotAreaPercent;
		float		BubbleMinSpotAvgBrightness;
		float		BubbleRejectImageAreaPercent;

		double		VisibleCellSpotArea;
		double		FlScalableROI;
		double		FlPeakPercent;

		double		NominalBkgdLevel;
		double		BkgdIntensityTolerance;
		double		CenterSpotMinIntensityLimit;
		double		PeakIntensitySelectionAreaLimit;
		double		CellSpotBrightnessExclusionThreshold;
		double		HotPixelEliminationMode;
		double		ImgBottomAndRightBoundaryAnnotationMode;
		double		SmallParticleSizingCorrection;
	} DB_ImageAnalysisParamRecord;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This describes the various structures or elements used in some of the binary data storage.  These elements have some conflicting type definitions
// between the definitions provided by the Cell Counting algorithm project code and the definitions used in the backend DLL code.  This has been resolved
// by splitting the typedef file provided by the cell counting project into two targetted component headers, CharacteristicTypedef.h and ImageTypedef.h,
// and wrapping the conflicting definitions from the backend side in the Hawkeye:: namespace.
//
// Characteristic_t as defined in the typedef file from Cell Counting (here pulled from the extracted CharacteristicTypedef.hpp):
// a standard tuple with anonomymous elements
//
//	typedef std::tuple<uint16_t, uint16_t, uint16_t> Characteristic_t;
//
// Characteristic_t as used in the derived tuple for cell identification parameters from Cell Counting (here pulled from the extracted CharacteristicTypedef.hpp):
//
//	typedef std::vector<std::tuple<Characteristic_t, float, E_POLARITY>> v_IdentificationParams_t;
//
// as defined in the typedef file from CellCounting; using Characteristic_t with anonomymous tuple elements (Here pulled from CellCountingOutputParams.h)
//
//	struct SInputSettings
//	{
//		std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> map_InputConfigParameters; /*!< Input configuration params*/
//		std::vector<std::tuple< Characteristic_t, float, E_POLARITY>> v_CellIdentificationParameters;/*!< Input Parameter definition for General population selection */
//		std::vector<std::tuple< Characteristic_t, float, E_POLARITY>> v_POIIdentificationParameters; /*!< Input Parameter definition for population of interest selection*/
//	};
//
// this is what is used in the SResult object used by the backend for data storage
//
// as defined in the typedef file from CellCounting (from CellCountingOutpputParams.hpp)
// this is how it is used in the SResult objects in the backend
//
//	typedef std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>	ConfigParamList_t;
//
//----------------------------------------------------------------------------------------------------------------------------------------------------
//
// Characteristic_t as defined in AnalysisDefinitionCommon.hpp in the backend
// Characteristic_T is now a structure with named elements
//
//	struct Characteristic_t
//	{
//		uint16_t key;
//		uint16_t s_key;
//		uint16_t s_s_key;
//
//		bool operator==( const Characteristic_t& rhs ) const
//		{
//			return ( ( key == rhs.key ) &&
//				( s_key == rhs.s_key ) &&
//					 ( s_s_key == rhs.s_s_key ) );
//		}
//	};
//
// The named element Characteristic_t structure does NOT appear to be used in the SResult structure used for storage by the backend...
//
//----------------------------------------------------------------------------------------------------------------------------------------------------
//
// a tuple for cell identification parameters derived from the named Characteristic_t structure and the other common elements
//
//	typedef std::tuple<Hawkeye::Characteristic_t, float, E_POLARITY> analysis_input_params_tuple;
//
// this was the basis for the derived analysis_input_params_storage structure defined below and used for database storage
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// how it is handed to the insertion operation, and what the backend expects to retrieve
//	typedef std::vector<std::tuple<Characteristic_t, float, E_POLARITY>> v_IdentificationParams_t;
	typedef std::tuple<Characteristic_t, float, E_POLARITY> analysis_input_params_t;

	// how it is stored in the database, and what will be returned on retrieve from the db
	typedef struct analysis_input_params_storage
	{
		uint16_t	key;
		uint16_t	s_key;
		uint16_t	s_s_key;
		float		value;
		E_POLARITY	polarity;

	} analysis_input_params_storage;

//	typedef std::vector<analysis_input_params_storage> v_AnalysisInputParamsStorage_t;
#if(0)
	typedef v_IdentificationParams_t v_CellIdentParams_t;
	typedef analysis_input_params_t CellIdentParams_t;
#else
//	typedef v_AnalysisInputParamsStorage_t v_CellIdentParams_t;
	typedef analysis_input_params_storage CellIdentParams_t;
	typedef std::vector<CellIdentParams_t> v_CellIdentParams_t;
#endif

	// goes to table AnalysisInputsettings which contains the defined composite types for the map and analysis_input_params_storage elements
	typedef struct DB_AnalysisInputSettings
	{
		int64_t		SettingsIdNum;
		uuid__t		SettingsId;
		bool		Protected;

		ConfigParamList_t	InputConfigParamMap;				// cell counting Input configuration params
		v_CellIdentParams_t	CellIdentParamList;					// cell counting Input Parameter definition for General population selection
		v_CellIdentParams_t	POIIdentParamList;					// cell counting Input Parameter definition for population of interest selection
	} DB_AnalysisInputSettingsRecord;



	////////////////////////////////////////////////////////////////////////////////
	// the following are 3 objects used to organize and describe the data acquisition
	// and processing actions.  These objects do not contain any sample data artifacts,
	// generated by the analysis process, other than a reference to the object capturing
	// the conditions used to acquire the sample. They can be deleted without affecting
	// sample results.  They are convenient for organinzing the activities of an instrument.
	////////////////////////////////////////////////////////////////////////////////

	typedef struct DB_SampleItem
	{
		int64_t		SampleItemIdNum;
		uuid__t		SampleItemId;
		int32_t		SampleItemStatus;
		std::string	SampleItemNameStr;
		std::string	Comments;
		system_TP	RunDateTP;
		uuid__t		SampleSetId;									// owning Sample Set UUID (may not be valid if from a sample-set template; should ALWAYS belong to a sample set!)
		uuid__t		SampleId;										// reference to the data output object produced by this sample analysis definition
		uuid__t		OwnerId;										// owner/creator ID; may not be valid in a template
		std::string	OwnerNameStr;									// owner/creator name; may not be valid in a template
		uint16_t	SaveNthImage;
		uint16_t	WashTypeIndex;
		uint16_t	Dilution;
		std::string	ItemLabel;
		uuid__t		ImageAnalysisParamId;							// parameters used to analyze the image
		uuid__t		AnalysisDefId;
		uint16_t	AnalysisDefIndex;
		DB_AnalysisDefinitionRecord	AnalysisDef;
		uuid__t		AnalysisParamId;
		DB_AnalysisParamRecord		AnalysisParam;
		uuid__t		CellTypeId;			// necessary if we have the cell type object?
		uint32_t	CellTypeIndex;		// necessary if we have the cell type object?
		DB_CellTypeRecord			CellType;
		uuid__t		BioProcessId;
		std::string	BioProcessNameStr;						// optional
		uuid__t		QcProcessId;
		std::string	QcProcessNameStr;						// optional
		uuid__t		WorkflowId;
		char		SampleRow;
		uint8_t		SampleCol;
		uint8_t		RotationCount;
	} DB_SampleItemRecord;

	typedef struct DB_SampleSet
	{
		int64_t		SampleSetIdNum;
		uuid__t		SampleSetId;
		int32_t		SampleSetStatus;								// overall status for the sample set; not-processed//processing/in-progress(for non-contiguous sample positions...)/complete, etc.
		std::string	SampleSetNameStr;
		std::string	SampleSetLabel;							// optional;
		std::string	Comments;
		uint16_t	CarrierType;
		uuid__t		OwnerId;										// owner/creator ID; may not be valid in a template
		std::string	OwnerNameStr;									// owner/creator name; may not be valid in a template
		system_TP	CreateDateTP;
		system_TP	ModifyDateTP;
		system_TP	RunDateTP;
		uuid__t		WorklistId;										// owning worklist UUID (may not be valid if from a list template or not assigned to a list)
		uint16_t	SampleItemCount;
		uint16_t	ProcessedItemCount;
		std::vector<DB_SampleItemRecord> SSItemsList;			// the list of explicitly defined sample set items in the sample set
	} DB_SampleSetRecord;

	typedef struct DB_Worklist
	{
		int64_t		WorklistIdNum;
		uuid__t		WorklistId;
		int32_t		WorklistStatus;
		std::string	WorklistNameStr;
		std::string	ListComments;
		std::string	InstrumentSNStr;
		uuid__t		CreationUserId;
		std::string	CreationUserNameStr;					// optional
		uuid__t		RunUserId;
		std::string	RunUserNameStr;							// optional
		system_TP	RunDateTP;
		bool		AcquireSample;
		uint16_t	CarrierType;
		bool		ByColumn;
		uint16_t	SaveNthImage;
		uint16_t	WashTypeIndex;
		uint16_t	Dilution;
		std::string	SampleSetNameStr;								// default base-name applied to the generic sample set object containing 'found' samples
		std::string	SampleItemNameStr;								// default base-name applied to otherwise unspecified or 'found' samples; resulting individual names may include an ordinal value to indicate processing order or carrier position
		uuid__t		ImageAnalysisParamId;							// defalt parameters used to analyze the image for otherwise unspecified  or 'found' samples
		uuid__t		AnalysisDefId;									// default parameters used to analyze the image for otherwise unspecified  or 'found' samples
		uint16_t	AnalysisDefIndex;
		uuid__t		AnalysisParamId;								// default parameters used to analyze the image for otherwise unspecified  or 'found' samples
		uuid__t		CellTypeId;										// default cell type to be used for otherwise unspecified or 'found' samples
		uint32_t	CellTypeIndex;
		uuid__t		BioProcessId;									// default Bio process workflow to be used for otherwise unspecified or 'found' samples
		std::string	BioProcessNameStr;						// optional
		uuid__t		QcProcessId;									// default QC process workflow to be used for otherwise unspecified or 'found' samples
		std::string	QcProcessNameStr;						// optional
		uuid__t		WorkflowId;										// default workflow to be used for otherwise unspecified or 'found' samples
		int16_t		SampleSetCount;									// number of sample sets defined or dynamically added to the worklist
		uint16_t	ProcessedSetCount;
		std::vector<DB_SampleSetRecord> SSList;						// the list of defined sample-sets
	} DB_WorklistRecord;



	////////////////////////////////////////////////////////////////////////////////
	// the following are objects generated by sample acquisition and/or analysis.
	// They represent a sample and its primary acquisition data, and are independent
	// of the worklist-related objects above.
	////////////////////////////////////////////////////////////////////////////////

	typedef struct DB_Image
	{
		int64_t		ImageIdNum;
		uuid__t		ImageId;
		uuid__t		ImageSequenceId;								// image record to which the image belong
		uint8_t		ImageChannel;									// the brightfield or fluorescence channel of the image (0 = unknown or no channel)
		std::string	ImageFileNameStr;								// record images are grouped into a single folder under the image set folder; record file name should not contain the parent path
	} DB_ImageRecord;
	
	typedef struct DB_ImageSeq										// corresponds to the ImageSetRecordDLL structure (and related non-DLL structure)
	{
		int64_t		ImageSequenceIdNum;
		uuid__t		ImageSequenceId;
		uuid__t		ImageSetId;										// image set to which the image belong
		int16_t		SequenceNum;									// order this image was taken in the entire set
		uint8_t		ImageCount;										// the number of images in the record; may be up to 5 for fluorescence
		uint8_t		FlChannels;										// the number of fluorescence channels (channels from 2-5)
		std::string	ImageSequenceFolderStr;							// record images are grouped into a single folder under the image set folder; record folder name should not contain the parent path
		std::vector<DB_ImageRecord> ImageList;						// Individual record images; number is available from ImageCount; assumes brightfield is always present on 'channel' 1; total Ids should match 'FlChannels + 1'; no fl image Ids will be present if FlChannels = 0
	} DB_ImageSeqRecord;

	typedef struct DB_ImageSet
	{
		int64_t		ImageSetIdNum;
		uuid__t		ImageSetId;
		uuid__t		SampleId;										// sample to which the images belong
		system_TP	CreationDateTP;									// creation date for this image set
		std::string	ImageSetPathStr;								// all images for a sample are stored under a parent folder; individual sequence images (image records) are stored in sub-folder; full path to the folder
		int16_t		ImageSequenceCount;								// record count value represents the number of image records stored; may be less than the value original acquired after storage decimation
		std::vector<DB_ImageSeqRecord> ImageSequenceList;			// List of ImageSequences; list count value is available from ImageSequenceCount;
	} DB_ImageSetRecord;

	typedef struct DB_Sample
	{
		int64_t		SampleIdNum;
		uuid__t		SampleId;
		std::string	SampleNameStr;
		int32_t		SampleStatus;
		uuid__t		CellTypeId;										// the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
		uint32_t	CellTypeIndex;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
		uuid__t		AnalysisDefId;
		uint16_t	AnalysisDefIndex;
		std::string	Label;
		uuid__t		BioProcessId;
		uuid__t		QcId;
		uuid__t		WorkflowId;
		std::string	CommentsStr;
		uint16_t	WashTypeIndex;
		uint16_t	Dilution;
		uuid__t		OwnerUserId;
		std::string	OwnerUserNameStr;						// optional
		uuid__t		RunUserId;									// user running the worklist/sample-set containing the sample 
		std::string	RunUserNameStr;							// optional
		system_TP	AcquisitionDateTP;
		uuid__t		ImageSetId;
		uuid__t		DustRefImageSetId;
		std::string	AcquisitionInstrumentSNStr;
		uuid__t		ImageAnalysisParamId;							// parameters used to analyze the image (shouldn't change, since reanalysis doesn't go beck to the images...)
		uint16_t	NumReagents;
		std::vector<std::string>	ReagentTypeNameList;			// list of the names of the reagents in the packs formatted as “type - idnum" for each list entry
		std::vector<std::string>	ReagentPackNumList;				// reagent pack numbers used for the acquisition of this sample
		std::vector<std::string>	PackLotNumList;					// pack lot numbers used during acquisition of this sample
		std::vector<uint64_t>		PackLotExpirationList;			// array of days since 1/1/1970; pack manufacture expiration dates
		std::vector<uint64_t>		PackInServiceList;				// array of days since 1/1/1970; date placed in service
		std::vector<uint64_t>		PackServiceExpirationList;		// array of days since 1/1/1970; expiration after being placed in service
	} DB_SampleRecord;



	////////////////////////////////////////////////////////////////////////////////
	// the following are objects generated by image processing.  They represent the
	// results of the image analysis for a sample using the analysis settings contained
	// in the results.  They reference the primary sample artifacts from whcih they are
	// generated.  The referenced objects are required for integrated review of the
	// images used to generate the results, and the image analysis results, but are
	// otherwise independent of the primary sample acquisition artifacts above.
	////////////////////////////////////////////////////////////////////////////////

	typedef struct db_signature
	{
		std::string userName;
		std::string shortSignature;
		std::string longSignature;
		system_TP   signatureTime;
		std::string signatureHash;
	} db_signature_t;

	// Summary Results appear to be the summation/average of all the individual image results, thus the imageSetID was included...
	typedef struct DB_SummaryResults
	{
		int64_t		SummaryResultIdNum;
		uuid__t		SummaryResultId;
		uuid__t		SampleId;
		uuid__t		ImageSetId;
		uuid__t		AnalysisId;
		system_TP	ResultDateTP;

		std::vector<db_signature_t>	SignatureList;

		uuid__t		ImageAnalysisParamId;
		uuid__t		AnalysisDefId;
		uuid__t		AnalysisParamId;
		uuid__t		CellTypeId;
		uint32_t	CellTypeIndex;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)

		int16_t		ProcessingStatus;
		int16_t		TotalCumulativeImages;

		int32_t		TotalCells_GP;
		int32_t		TotalCells_POI;
		float		POI_PopPercent;
		float		CellConc_GP;									// value x 10^6
		float		CellConc_POI;									// value x 10^6

		float		AvgDiam_GP;
		float		AvgDiam_POI;
		float		AvgCircularity_GP;
		float		AvgCircularity_POI;

		float		CoefficientOfVariance;
		uint16_t	AvgCellsPerImage;
		uint16_t	AvgBkgndIntensity;

		uint16_t	TotalBubbleCount;
		uint16_t	LargeClusterCount;
		uint16_t	QcStatus;
	} DB_SummaryResultRecord;

	typedef struct DB_DetailedResult								// per-image detailed results...  corresponds to SImageStats content
	{
		int64_t		DetailedResultIdNum;
		uuid__t		DetailedResultId;
		uuid__t		SampleId;
		uuid__t		ImageId;										// the specific image used to generate these these results 
		uuid__t		AnalysisId;
		uuid__t		OwnerUserId;									// user owning this data
		system_TP	ResultDateTP;

		int16_t		ProcessingStatus;
		uint16_t	TotalCumulativeImages;

		int32_t		TotalCells_GP;
		int32_t		TotalCells_POI;
		double		POI_PopPercent;
		double		CellConc_GP;
		double		CellConc_POI;

		double		AvgDiam_GP;
		double		AvgDiam_POI;
		double		AvgCircularity_GP;
		double		AvgCircularity_POI;

		double		AvgSharpness_GP;
		double		AvgSharpness_POI;
		double		AvgEccentricity_GP;
		double		AvgEccentricity_POI;
		double		AvgAspectRatio_GP;
		double		AvgAspectRatio_POI;
		double		AvgRoundness_GP;
		double		AvgRoundness_POI;
		double		AvgRawCellSpotBrightness_GP;
		double		AvgRawCellSpotBrightness_POI;
		double		AvgCellSpotBrightness_GP;
		double		AvgCellSpotBrightness_POI;
		double		AvgBkgndIntensity;

		int32_t		TotalBubbleCount;
		int32_t		LargeClusterCount;
	} DB_DetailedResultRecord;

	// how it is stored in the database, and what will be returned on retrieve from the db
	typedef struct blob_point_struct
	{
		int16_t		startx;
		int16_t		starty;
	} blob_point;

	// how it is stored in the database, and what will be returned on retrieve from the db
	typedef struct blob_rect_storage_struct
	{
		int16_t		startx;
		int16_t		starty;
		int16_t		width;
		int16_t		height;
	} blob_rect_storage;

	// how it is handed to the insertion object, and what the backend expects to retrieve
	typedef struct blob_rect_t_struct
	{
		blob_point	start;
		int16_t		width;
		int16_t		height;
	} blob_rect_t;

	// how it is handed to the insertion object, and what the backend expects to retrieve
	typedef struct cluster_data_t_struct
	{
		int16_t					cell_count;
		std::vector<blob_point> cluster_polygon;	// all vertices of a (potentially) irregular outline
		blob_rect_t				cluster_box;
	} cluster_data_t;

	// how it is stored in the database, and what will be returned on retrieve from the db
//	typedef struct cluster_data_storage_struct
//	{
//		int16_t					cell_count;
//		std::vector<blob_point> cluster_polygon;	// all vertices of a (potentially) irregular outline
//		blob_rect_storage		cluster_box;
//	} cluster_data_storage;

	// how it is handed to the insertion object, and what the backend expects to retrieve
	typedef std::pair<Characteristic_t, float> blob_info_pair;

	// how it is stored in the database, and what will be returned on retrieve from the db
	typedef struct blob_info_storage
	{
		uint16_t	key;
		uint16_t	s_key;
		uint16_t	s_s_key;
		float		value;
	} blob_info_storage;

	// how it is handed to the insertion object, and what the backend expects to retrieve
	typedef struct blob_data_t_struct
	{
		std::vector<blob_info_pair>	blob_info;
		blob_point					blob_center;
		std::vector<blob_point>		blob_outline;
	} blob_data_t;

	// how it is stored in the database, and what will be returned on retrieve from the db
	typedef struct blob_data_storage_struct
	{
		std::vector<blob_info_storage>	blob_info;
		blob_point						blob_center;
		std::vector<blob_point>			blob_outline;
	} blob_data_storage;

	// per-image detailed results, blob data, and fl channel data;
	// this consolidates elements 3-6 of the SResult structure and removes the per-image maps
	typedef struct DB_ImageResult
	{
		int64_t		ResultIdNum;
		uuid__t		ResultId;
		uuid__t		SampleId;
		uuid__t		ImageId;
		uuid__t		AnalysisId;
		int32_t		ImageSeqNum;

		DB_DetailedResultRecord			DetailedResult;

		std::map<int16_t, int16_t>		MaxNumOfPeaksFlChansMap;		// <chan number, max peaks>

		int16_t		NumBlobs;
		std::vector<blob_data_t>		BlobDataList;
		int16_t		NumClusters;
		std::vector<cluster_data_t>		ClusterDataList;
	} DB_ImageResultRecord;

	typedef struct DB_BlobData
	{
		int64_t		BlobIdNum;
		uuid__t		BlobId;
		uuid__t		ImageResultId;

		std::vector<blob_info_storage>	BlobInfo;
		blob_point						BlobCenter;
		std::vector<blob_point>			BlobOutline;
	} DB_BlobDataRecord;

	typedef struct DB_BlobIdentParam					// contained in the ImageAnalysisCellIdentParams table in the DB
	{
		int64_t		IdentParamIdNum;
		uuid__t		IdentParamId;

		v_IdentificationParams_t	paramVals;
	} DB_BlobIdentParamRecord;

	typedef struct DB_ClusterData
	{
		int64_t		ClusterIdNum;
		uuid__t		ClusterId;
		uuid__t		ImageResultId;

		int16_t					cell_count;
		std::vector<blob_point> cluster_polygon;	// all vertices of a (potentially) irregular outline
		blob_rect_storage		cluster_box;
	} DB_ClusterDataRecord;

	typedef struct DB_SResult
	{
		int64_t	SResultIdNum;
		uuid__t	SResultId;
		uuid__t	SampleId;
		uuid__t	AnalysisId;

		DB_AnalysisInputSettings			ProcessingSettings;
		DB_DetailedResultRecord				CumulativeDetailedResult;
		std::map<int16_t, int16_t>			CumulativeMaxNumOfPeaksFlChanMap;		// should be <fl-chan, max_num_peaks>
		std::vector<DB_ImageResultRecord>	ImageResultList;
	} DB_SResultRecord;

	typedef struct DB_Analysis
	{
		int64_t		AnalysisIdNum;
		uuid__t		AnalysisId;
		uuid__t		SampleId;
		uuid__t		ImageSetId;
		DB_SummaryResultRecord	SummaryResult;
		uuid__t		SResultId;

		uuid__t		AnalysisUserId;
		std::string	AnalysisUserNameStr;						// optional
		system_TP	AnalysisDateTP;

		std::string	InstrumentSNStr;								// instrument serial number or workstation iidentifier
		uuid__t		BioProcessId;
		uuid__t		QcId;
		uuid__t		WorkflowId;

		int16_t		ImageSequenceCount;								// the number of image records used for the analysis; note that not all images in a set may be used for an analysis
		std::vector<DB_ImageSeqRecord> ImageSequenceList;			// list of image records used for this analysis; note that not all images in a set may be used for an analysis
	} DB_AnalysisRecord;



	////////////////////////////////////////////////////////////////////////////////
	// the following are objects generated or modified by the instrument to capture user
	// information and permissions, instrument configuration, and defined system processes.
	// NOTE that structures ending in '_t' are composite data type structures for database
	// columns containg a structure rather than a native data type.  They should correspond
	// to the public data types in the 'public' schemas in the ViCELL database.
	////////////////////////////////////////////////////////////////////////////////

	typedef struct ad_role_map
	{
		uint16_t	RoleType;										// name associated with this role/user; should not allow duplicate names
		uuid__t		RoleId;											// ID for this user
		std::string	ADGrpName;										// name associated with this role/user; should not allow duplicate names
	} ad_role_map_t;

	typedef struct DB_UserRole
	{
		int64_t		RoleIdNum;
		uuid__t		RoleId;											// ID for this user
		bool		Protected;
		std::string	RoleNameStr;									// name associated with this role/user; should not allow duplicate names
		uint16_t	RoleType;										// BCI defined user type for instrument permissions
		std::vector<std::string> GroupMapList;						// list of AD group mnames that map to this role
		std::vector<uint32_t> CellTypeIndexList;					// Indices for the cell types the role is permitted to use by default
		uint64_t	InstrumentPermissions;							// BCI defined & user defined permissions bit-field
		uint64_t	ApplicationPermissions;							// BCI defined & user defined permissions bit-field
	} DB_UserRoleRecord;

	typedef struct DB_UserProperties								// BCI-defined properties values
	{
		int64_t		PropertiesIdNum;
		int16_t		PropertyIndex;									// Index for this user property
		bool		Protected;
		std::string	PropertyNameStr;								// name associated with this property
		int16_t		PropertyType;									// BCI defined value for the property; may need to be a bitfield?
	} DB_UserPropertiesRecord;

	typedef struct display_column_info
	{
		int32_t		ColumnType;
		int16_t		OrderIndex;			// the order the column appears in the GUI on the DataGrid; index 0 indicates the left-most item;, index 1 item second from the left, etc.
		int16_t		Width;
		bool		Visible;
	} display_column_info_t;

	typedef struct DB_User
	{
		int64_t		UserIdNum;
		uuid__t		UserId;											// ID for this user
		bool		Protected;
		bool		Retired;										// previously used entry, now 'burnt'
		bool		ADUser;											// user entry comes from the ActiveDirectory user list;  may be removed and/or replaced when another ActiveDirectory user logs in;
		uuid__t		RoleId;											// ID of role containing instrument and application permissions
		std::string	UserNameStr;									// login name associated with this role/user; should not allow duplicate names
		std::string	DisplayNameStr;									// Display name associated with this role/user; Duplicates are allowed, althgouh potentially should be discouraged
		std::string Comment;
		std::string	UserEmailStr;
		std::vector<std::string>			AuthenticatorList;		// list of the current and previously used authenticators; list size controlled by system configuration for previous paassword list
		system_TP	AuthenticatorDateTP;							// used for the password creation date
		system_TP	LastLogin;										// login time for authentication and inactivity timeout
		int16_t		AttemptCount;									// failed login counter
		std::string	LanguageCode;									// identifier for user language setting
		std::string	DefaultSampleNameStr;							// user-specific default sample name
		int16_t		UserImageSaveN;									// user defaultvalue for save every Nth image
		std::vector<display_column_info_t>	ColumnDisplayList;		// the map of the columns to display and the position to display the content
		int16_t		DecimalPrecision;								// number of places following the decimal point to display
		std::string	ExportFolderStr;								// user-specific result and export data folder name
		std::string	DefaultResultFileNameStr;						// user-specific default sample name
		std::string	CSVFolderStr;									// user-specific CSV result folder name
		bool		PdfExport;										// also export as pdf flag
		bool		AllowFastMode;									// if the user is allowed to use fast mode
		int16_t		WashType;										// user default wash type
		int16_t		Dilution;										// user default dilution factor
		uint32_t	DefaultCellType;								// the default cell type for the user
		uint16_t	NumUserCellTypes;								// the number of cell types the user is permitted tu use
		std::vector<uint32_t>				UserCellTypeIndexList;	// Indices for the cell types the user is permitted to use
		std::vector<int32_t>				UserAnalysisIndexList;	// Indices for the AnalysisDefinitions the user is permitted to use
		uint8_t		NumUserProperties;
		std::vector<DB_UserPropertiesRecord> UserPropertiesList;
		uint64_t	AppPermissions;									// bit field for complex permission assignments
		std::string	AppPermissionsHash;								// hash of permission bits
		uint64_t	InstPermissions;								// bit field for complex permission assignments
		std::string	InstPermissionsHash;							// hash of permission bits
	} DB_UserRecord;

	typedef struct DB_Signature
	{
		int64_t		SignatureDefIdNum;
		uuid__t		SignatureDefId;									// ID for this user signature
		std::string	LongSignatureStr;								// character array containing the long signature tag string
		std::string	LongSignatureHash;								// character array containing the long signature hash
		std::string	ShortSignatureStr;								// character array containing the short signature tag string
		std::string	ShortSignatureHash;								// character array containing the short signature hash
	} DB_SignatureRecord;

	typedef struct DB_ReagentType									// -> ReagentInfo table; describes the characteristics of a reagent type by lot number.
	{
		int64_t						ReagentIdNum;
		int32_t						ReagentTypeNum;
		bool						Current;
		std::string					ContainerTagSn;
		std::vector<int16_t>		ReagentIndexList;
		std::vector<std::string>	ReagentNamesList;
		std::vector <int16_t>		MixingCyclesList;
		std::string					PackPartNumStr;
		std::string					LotNumStr;
		int64_t						LotExpirationDate;
		int64_t						InServiceDate;
		int16_t						InServiceExpirationLength;						// maximum in-service life in days; may be shortened by pack expiration date
		bool						Protected;
	} DB_ReagentTypeRecord;

	typedef struct DB_CellHealthReagent
	{
		int64_t		IdNum;
		uuid__t		Id;									// Not currently used.
		int16_t		Type;								// Reagent type
		std::string	Name;
		int32_t		Volume;
		bool		Protected;
	} DB_CellHealthReagentRecord;

	typedef struct DB_Workflow
	{
		int64_t		WorkflowIdNum;
		uuid__t		WorkflowId;
		bool		Protected;
		std::string	WorkflowName;
		int16_t		NumReagents;
		std::vector<int32_t>		ReagentTypeList;
		std::vector<std::string>	WorkflowSequenceList;
	} DB_WorkflowRecord;

	typedef struct DB_BioProcess
	{
		int64_t		BioProcessIdNum;
		uuid__t		BioProcessId;
		bool		Protected;
		std::string	BioProcessName;
		std::string	BioProcessSequence;
		std::string	ReactorName;
		uuid__t		CellTypeId;
		uint32_t	CellTypeIndex;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
	} DB_BioProcessRecord;

	typedef struct DB_QcProcess
	{
		int64_t		QcIdNum;
		uuid__t		QcId;
		bool		Protected;
		std::string	QcName;
		uint16_t	QcType;
		uuid__t		CellTypeId;
		uint32_t	CellTypeIndex;									// shortcut to the original cell type of the sample as specified at acquisition (shouldn't change with re-analysis...)
		std::string	LotInfo;
		std::string	LotExpiration;
		double		AssayValue;
		double		AllowablePercentage;
		std::string	QcSequence;
		std::string	Comments;
		bool		Retired;
	} DB_QcProcessRecord;

	typedef struct DB_AppConfig
	{
		uuid__t		ConfigId;
		std::string	AppNameStr;
		std::string	ConfigParamsJsonStr;
		std::string	LogParamsJsonStr;
	} DB_AppConfigRecord;

	typedef struct ad_settings
	{
		std::string	servername;
		std::string	server_addr;
		int32_t		port_number;
		std::string	base_dn;
		bool		enabled;
	} ad_settings_t;

	typedef struct af_settings
	{
		bool		save_image;
		int32_t		coarse_start;
		int32_t		coarse_end;
		int16_t		coarse_step;
		int32_t		fine_range;
		int16_t		fine_step;
		int32_t		sharpness_low_threshold;
	} af_settings_t;

	typedef struct email_settings
	{
		std::string	server_addr;
		int32_t		port_number;
		bool		authenticate;
		std::string	username;
		std::string	pwd_hash;
	} email_settings_t;

	typedef struct rfid_sim_info
	{
		bool		set_valid_tag_data;
		int16_t		total_tags;
		std::string	main_bay_file;
		std::string	door_left_file;
		std::string	door_right_file;
	} rfid_sim_info_t;

	typedef struct language_info
	{
		int16_t		language_id;
		std::string language_name;
		std::string locale_tag;
		bool active;
	} language_info_t;

	typedef struct run_options
	{
		std::string sample_set_name;
		std::string sample_name;
		int16_t		save_image_count;
		int16_t		save_nth_image;
		bool		results_export;
		std::string	results_export_folder;
		bool		append_results_export;
		std::string append_results_export_folder;
		std::string result_filename;
		std::string results_folder;
		bool		auto_export_pdf;
		std::string	csv_folder;
		int16_t		wash_type;
		int16_t		dilution;
		uint32_t	bpqc_cell_type_index;
	} run_options_t;

	typedef struct illuminator_info
	{
		int16_t type;
		int16_t index;
	} illuminator_info_t;

	typedef struct DB_InstrumentConfig
	{
		int64_t			InstrumentIdNum;
		std::string		InstrumentSNStr;
		int16_t			InstrumentType;
		std::string		DeviceName;
		std::string		UIVersion;
		std::string		SoftwareVersion;
		std::string		AnalysisVersion;
		std::string		FirmwareVersion;
		int16_t			CameraType;
		std::string		CameraFWVersion;
		std::string		CameraConfig;
		int16_t			PumpType;
		std::string		PumpFWVersion;
		std::string		PumpConfig;
		std::vector<illuminator_info_t>	IlluminatorsInfoList;
		std::string		IlluminatorConfig;
		int16_t			BrightFieldLedType;
		int16_t			ConfigType;
		std::string		LogName;
		int32_t			LogMaxSize;
		std::string		LogSensitivity;
		int16_t			MaxLogs;
		bool			AlwaysFlush;
		std::string		CameraErrorLogName;
		int32_t			CameraErrorLogMaxSize;
		std::string		StorageErrorLogName;
		int32_t			StorageErrorLogMaxSize;
		int32_t			CarouselThetaHomeOffset;
		int32_t			CarouselRadiusOffset;
		int32_t			PlateThetaHomeOffset;
		int32_t			PlateThetaCalPos;
		int32_t			PlateRadiusCenterPos;
		int16_t			SaveImage;
		int32_t			FocusPosition;
		af_settings_t	AF_Settings;
		int16_t			AbiMaxImageCount;
		int16_t			SampleNudgeVolume;
		int16_t			SampleNudgeSpeed;
		float			FlowCellDepth;
		float			FlowCellDepthConstant;
		rfid_sim_info_t	RfidSim;
		bool			LegacyData;
		bool			CarouselSimulator;
		int16_t			NightlyCleanOffset;		// offset from midnight in minutes
		system_TP		LastNightlyClean;
		int16_t			SecurityMode;
		int16_t			InactivityTimeout;		// inactivity timeout in seconds?
		int16_t			PasswordExpiration;		// password expiration in days
		bool			NormalShutdown;
		int16_t			NextAnalysisDefIndex;
		int32_t			NextBCICellTypeIndex;
		uint32_t		NextUserCellTypeIndex;
		int32_t			TotalSamplesProcessed;
		int16_t			DiscardTrayCapacity;
		email_settings_t				Email_Settings;
		ad_settings_t					AD_Settings;
		std::vector<language_info_t>	LanguageList;
		run_options_t					RunOptions;
		bool			AutomationEnabled;
		bool			ACupEnabled;
		bool			AutomationInstalled;
		int32_t			AutomationPort;
	} DB_InstrumentConfigRecord;

	typedef	struct cal_consumable_struct
	{
		std::string	label;
		std::string	lot_id;
		int16_t		cal_type;
		system_TP	expiration_date;
		float		assay_value;
	} cal_consumable_t;

	typedef struct DB_Calibration
	{
		int64_t		CalIdNum;					// "CalibrationIdNum" bigint NOT NULL,
		uuid__t		CalId;						// "CalibrationID" uuid NOT NULL,
		std::string	InstrumentSNStr;			// "InstrumentSN" character varying,
		system_TP	CalDate;					//	"CalibrationDate" timestamp without time zone,
		uuid__t		CalUserId;					//	"CalibrationUserID" uuid,
		int16_t		CalType;					//	"CalibrationType" smallint, concentration vs size?
		double		Slope;						//	"Slope" double precision,
		double		Intercept;					//	"Intercept" double precision,
		int16_t		ImageCnt;					//	"ImageCount" smallint,
		uuid__t		QueueId;					//	"CalQueueID" uuid,
		std::vector<cal_consumable_t>	ConsumablesList;	// "ConsumablesList" public.cal_consumable[],
	} DB_CalibrationRecord;


	typedef struct DB_LogEntry
	{
		int64_t		IdNum;						// "IdNum" bigint NOT NULL,
		int16_t		EntryType;					// "EntryType" smallint
		system_TP	EntryDate;					// "EntryDate" timestamp without time zone
		std::string	EntryStr;					// "EntryStr" character varying
		bool		Protected;
	} DB_LogEntryRecord;

	typedef struct DB_SchedulerConfig
	{
		int64_t		ConfigIdNum;
		uuid__t		ConfigId;					// ID for this scheduled operation configuration
		std::string	Name;						// character array containing the descriptive name for the scheduled operation
		std::string	Comments;					// character array containing comments regarding this scheduled operation
		std::string	FilenameTemplate;			// character array containing the default filename template (for exports)
		uuid__t		OwnerUserId;				// ID for the owner/creator of this scheduled operation
		system_TP	CreationDate;				// Creation date of this scheduled operation configuration - timestamp without time zone
		int16_t		OutputType;					// Export output type selector (currently encrypted or non-encrypted)
		system_TP	StartDate;					// absolute start time for this scheduled operation (do not run prior to this time; also needed for 'run-once' operations); entered as the date without time;
		int16_t		StartOffset;				// Offset in minutes from midnight of the scheduled operation
		int16_t		MultiRepeatInterval;		// repetition interval with a single day in minutes
		uint16_t	DayWeekIndicator;			// bitfield representing days of week and week repetition interval (bit 0-6 = Sun - Sat; bit 8 = 1 weekly; bit 9 = 2 bi-weekly; bit 10 = every 3 weeks; bit 11 = every 4 weeks; bit 12 = monthly, bits 13-15=0)
		int16_t		MonthlyRunDay;				// for monthly interval, the day of the month to execute task
		std::string	DestinationFolder;			// destination of output; should be mapped drive format, not URL
		int32_t		DataType;					// the data type to be retrieved by this scheduler; this is intended to let the client know what type of data/results are the key for this scheduled export
		std::vector<DBApi::eListFilterCriteria> FilterTypesList;
		std::vector<std::string> CompareOpsList;
		std::vector<std::string> CompareValsList;
		bool		Enabled;					// enabled/disabled state of export operation configuration
		system_TP	LastRunTime;				// last execution time for this scheduled operation (may be '0' for never run)
		system_TP	LastSuccessRunTime;			// execution time for the last successful execution of this scheduled operation (may be '0' for never run)
		int16_t		LastRunStatus;				// Completion status for last run (or never run)
		std::string	NotificationEmail;
		std::string	EmailServerAddr;
		int32_t		EmailServerPort;
		bool		AuthenticateEmail;
		std::string	AccountUsername;
		std::string	AccountPwdHash;
	} DB_SchedulerConfigRecord;

}