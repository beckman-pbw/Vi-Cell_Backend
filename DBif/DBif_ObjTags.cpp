// Database interface : implementation file: Database table column tags external reference file
//

#pragma once

#include "pch.h"

#include "DBif_ObjTags.hpp"


const char ProtectedTag[] = "Protected";

////////////////////////////////////////////////////////////////////////////////
// Instrument configuration and analysis parameters
////////////////////////////////////////////////////////////////////////////////

// for DB_worklistRecord
const char WL_IdNumTag[] = "WorklistIdNum";
const char WL_IdTag[] = "WorklistID";
const char WL_StatusTag[] = "WorklistStatus";
const char WL_NameTag[] = "WorklistName";
const char WL_CommentsTag[] = "ListComments";
const char WL_InstSNTag[] = "InstrumentSN";
const char WL_CreateUserIdTag[] = "CreationUserID";
const char WL_RunUserIdTag[] = "RunUserID";
const char WL_RunDateTag[] = "RunDate";
const char WL_AcquireSampleTag[] = "AcquireSample";
const char WL_CarrierTypeTag[] = "CarrierType";
const char WL_PrecessionTag[] = "ByColumn";
const char WL_SaveImagesTag[] = "SaveImages";
const char WL_WashTypeTag[] = "WashType";
const char WL_DilutionTag[] = "Dilution";
const char WL_DfltSetNameTag[] = "DefaultSetName";
const char WL_DfltItemNameTag[] = "DefaultItemName";
const char WL_ImageAnalysisParamIdTag[] = "ImageAnalysisParamID";
const char WL_AnalysisDefIdTag[] = "AnalysisDefinitionID";
const char WL_AnalysisDefIdxTag[] = "AnalysisDefinitionIndex";
const char WL_AnalysisParamIdTag[] = "AnalysisParameterID";
const char WL_CellTypeIdTag[] = "CellTypeID";
const char WL_CellTypeIdxTag[] = "CellTypeIndex";
const char WL_BioProcessIdTag[] = "BioProcessID";
const char WL_QcProcessIdTag[] = "QcProcessID";
const char WL_WorkflowIdTag[] = "WorkflowID";
const char WL_SampleSetCntIdTag[] = "SampleSetCount";
const char WL_ProcessedSetCntTag[] = "ProcessedSetCount";
const char WL_SampleSetIdListTag[] = "SampleSetIDList";

// for DB_SampleSetRecord
const char SS_IdNumTag[] = "SampleSetIdNum";
const char SS_IdTag[] = "SampleSetID";
const char SS_StatusTag[] = "SampleSetStatus";
const char SS_NameTag[] = "SampleSetName";
const char SS_LabelTag[] = "SampleSetLabel";
const char SS_CommentsTag[] = "Comments";
const char SS_CarrierTypeTag[] = "CarrierType";
const char SS_OwnerIdTag[] = "OwnerID";
const char SS_CreateDateTag[] = "CreateDate";
const char SS_ModifyDateTag[] = "ModifyDate";
const char SS_RunDateTag[] = "RunDate";
const char SS_WorklistIdTag[] = "WorklistID";
const char SS_SampleItemCntTag[] = "SampleItemCount";
const char SS_ProcessedItemCntTag[] = "ProcessedItemCount";
const char SS_SampleItemIdListTag[] = "SampleItemIDList";

// for DB_SampleItemRecord
const char SI_IdNumTag[] = "SampleItemIdNum";
const char SI_IdTag[] = "SampleItemID";
const char SI_StatusTag[] = "SampleItemStatus";
const char SI_NameTag[] = "SampleItemName";
const char SI_CommentsTag[] = "Comments";
const char SI_RunDateTag[] = "RunDate";
const char SI_SampleSetIdTag[] = "SampleSetID";
const char SI_SampleIdTag[] = "SampleID";
const char SI_SaveImagesTag[] = "SaveImages";
const char SI_WashTypeTag[] = "WashType";
const char SI_DilutionTag[] = "Dilution";
const char SI_LabelTag[] = "ItemLabel";
const char SI_ImageAnalysisParamIdTag[] = "ImageAnalysisParamID";
const char SI_AnalysisDefIdTag[] = "AnalysisDefinitionID";
const char SI_AnalysisDefIdxTag[] = "AnalysisDefinitionIndex";
const char SI_AnalysisParamIdTag[] = "AnalysisParameterID";
const char SI_CellTypeIdTag[] = "CellTypeID";
const char SI_CellTypeIdxTag[] = "CellTypeIndex";
const char SI_BioProcessIdTag[] = "BioProcessID";
const char SI_QcProcessIdTag[] = "QcProcessID";
const char SI_WorkflowIdTag[] = "WorkflowID";
const char SI_SamplePosTag[] = "SamplePosition";

// for DB_CellTypeRecord
const char CT_IdNumTag[] = "CellTypeIdNum";
const char CT_IdTag[] = "CellTypeID";
const char CT_IdxTag[] = "CellTypeIndex";
const char CT_NameTag[] = "CellTypeName";
const char CT_RetiredTag[] = "Retired";
const char CT_MaxImagesTag[] = "MaxImages";
const char CT_AspirationCyclesTag[] = "AspirationCycles";
const char CT_MinDiamMicronsTag[] = "MinDiamMicrons";
const char CT_MaxDiamMicronsTag[] = "MaxDiamMicrons";
const char CT_MinCircularityTag[] = "MinCircularity";
const char CT_SharpnessLimitTag[] = "SharpnessLimit";
const char CT_NumCellIdentParamsTag[] = "NumCellIdentParams";
const char CT_CellIdentParamIdListTag[] = "CellIdentParamIDList";
const char CT_DeclusterSettingsTag[] = "DeclusterSetting";
const char CT_RoiExtentTag[] = "RoiExtent";
const char CT_RoiXPixelsTag[] = "RoiXPixels";
const char CT_RoiYPixelsTag[] = "RoiYPixels";
const char CT_NumAnalysisSpecializationsTag[] = "NumAnalysisSpecializations";
const char CT_AnalysisSpecializationIdListTag[] = "AnalysisSpecializationIDList";
const char CT_CalcCorrectionFactorTag[] = "CalculationAdjustmentFactor";

// for DB_AnalysisDefinitionRecord
const char AD_IdNumTag[] = "AnalysisDefinitionIdNum";
const char AD_IdTag[] = "AnalysisDefinitionID";
const char AD_IdxTag[] = "AnalysisDefinitionIndex";
const char AD_NameTag[] = "AnalysisDefinitionName";
const char AD_NumReagentsTag[] = "NumReagents";
const char AD_ReagentTypeIdxListTag[] = "ReagentTypeIndexList";
const char AD_MixingCyclesTag[] = "MixingCycles";
const char AD_NumIlluminatorsTag[] = "NumIlluminators";
const char AD_IlluminatorIdxListTag[] = "IlluminatorsIndexList";
const char AD_NumAnalysisParamsTag[] = "NumAnalysisParams";
const char AD_AnalysisParamIdListTag[] = "AnalysisParamIDList";
const char AD_PopulationParamIdTag[] = "PopulationParamID";
const char AD_PopulationParamExistsTag[] = "PopulationParamExists";

// for DB_AnalysisParamRecord
const char AP_IdNumTag[] = "AnalysisParamIdNum";
const char AP_IdTag[] = "AnalysisParamID";
const char AP_InitializedTag[] = "IsInitialized";
const char AP_LabelTag[] = "AnalysisParamLabel";
const char AP_KeyTag[] = "CharacteristicKey";
const char AP_SKeyTag[] = "CharacteristicSKey";
const char AP_SSKeyTag[] = "CharacteristicSSKey";
const char AP_ThreshValueTag[] = "ThresholdValue";
const char AP_AboveThreshTag[] = "AboveThreshold";

// DB_ImageAnalysisParamRecord
const char IAP_IdNumTag[] = "ImageAnalysisParamIdNum";
const char IAP_IdTag[] = "ImageAnalysisParamID";
const char IAP_AlgorithmModeTag[] = "AlgorithmMode";
const char IAP_BubbleModeTag[] = "BubbleMode";
const char IAP_DeclusterModeTag[] = "DeclusterMode";
const char IAP_SubPeakAnalysisModeTag[] = "SubPeakAnalysisMode";
const char IAP_DilutionFactorTag[] = "DilutionFactor";
const char IAP_ROIXcoordsTag[] = "ROIXcoords";
const char IAP_ROIYcoordsTag[] = "ROIYcoords";
const char IAP_DeclusterAccumuLowTag[] = "DeclusterAccumulatorThreshLow";
const char IAP_DeclusterMinDistanceLowTag[] = "DeclusterMinDistanceThreshLow";
const char IAP_DeclusterAccumuMedTag[] = "DeclusterAccumulatorThreshMed";
const char IAP_DeclusterMinDistanceMedTag[] = "DeclusterMinDistanceThreshMed";
const char IAP_DeclusterAccumuHighTag[] = "DeclusterAccumulatorThreshHigh";	
const char IAP_DeclusterMinDistanceHighTag[] = "DeclusterMinDistanceThreshHigh";
const char IAP_FovDepthTag[] = "FovDepthMM";
const char IAP_PixelFovTag[] = "PixelFovMM";
const char IAP_SizingSlopeTag[] = "SizingSlope";
const char IAP_SizingInterceptTag[] = "SizingIntercept";
const char IAP_ConcSlopeTag[] = "ConcSlope";
const char IAP_ConcInterceptTag[] = "ConcIntercept";
const char IAP_ConcImageControlCntTag[] = "ConcImageControlCnt";
const char IAP_BubbleMinSpotAreaPrcntTag[] = "BubbleMinSpotAreaPrcnt";
const char IAP_BubbleMinSpotAreaBrightnessTag[] = "BubbleMinSpotAreaBrightness";
const char IAP_BubbleRejectImgAreaPrcntTag[] = "BubbleRejectImgAreaPrcnt";
const char IAP_VisibleCellSpotAreaTag[] = "VisibleCellSpotArea";
const char IAP_FlScalableROITag[] = "FlScalableROI";
const char IAP_FLPeakPercentTag[] = "FLPeakPercent";
const char IAP_NominalBkgdLevelTag[] = "NominalBkgdLevel";
const char IAP_BkgdIntensityToleranceTag[] = "BkgdIntensityTolerance";
const char IAP_CenterSpotMinIntensityTag[] = "CenterSpotMinIntensityLimit";
const char IAP_PeakIntensitySelectionAreaTag[] = "PeakIntensitySelectionAreaLimit";
const char IAP_CellSpotBrightnessExclusionTag[] = "CellSpotBrightnessExclusionThreshold";
const char IAP_HotPixelEliminationModeTag[] = "HotPixelEliminationMode";
const char IAP_ImgBotAndRightBoundaryModeTag[] = "ImgBotAndRightBoundaryAnnotationMode";
const char IAP_SmallParticleSizeCorrectionTag[] = "SmallParticleSizingCorrection";

// DB_AnalysisInputSettingsRecord
const char AIP_IdNumTag[] = "SettingsIdNum";
const char AIP_IdTag[] = "SettingsID";
const char AIP_ConfigParamMapTag[] = "InputConfigParamMap";
const char AIP_CellIdentParamListTag[] = "CellIdentParamList";
const char AIP_PoiIdentParamListTag[] = "POIIdentParamList";

// for DB_BlobIdentParamRecord

// for DB_FlIlluminatorRecord
const char IL_IdNumTag[] = "IlluminatorIdNum";
const char IL_IdxTag[] = "IlluminatorIndex";
const char IL_TypeTag[] = "IlluminatorType";
const char IL_NameTag[] = "IlluminatorName";
const char IL_PosNumTag[] = "PositionNum";
const char IL_ToleranceTag[] = "Tolerance";
const char IL_MaxVoltageTag[] = "MaxVoltage";
const char IL_IllumWavelengthTag[] = "IlluminatorWavelength";
const char IL_EmitWavelengthTag[] = "EmissionWavelength";
const char IL_ExposureTimeMsTag[] = "ExposureTimeMs";
const char IL_PercentPowerTag[] = "PercentPower";
const char IL_SimmerVoltageTag[] = "SimmerVoltage";
const char IL_LtcdTag[] = "Ltcd";
const char IL_CtldTag[] = "Ctld";
const char IL_FeedbackDiodeTag[] = "FeedbackPhotoDiode";

// for DB_UserRecord
const char UR_IdNumTag[] = "UserIdNum";
const char UR_IdTag[] = "UserID";
const char UR_RetiredTag[] = "Retired";
const char UR_ADUserTag[] = "ADUser";
const char UR_RoleIdTag[] = "RoleID";
const char UR_UserNameTag[] = "UserName";
const char UR_DisplayNameTag[] = "DisplayName";
const char UR_CommentsTag[] = "Comments";
const char UR_UserEmailTag[] = "UserEmail";
const char UR_AuthenticatorListTag[] = "AuthenticatorList";
const char UR_AuthenticatorDateTag[] = "AuthenticatorDate";
const char UR_LastLoginTag[] = "LastLogin";
const char UR_AttemptCntTag[] = "AttemptCount";
const char UR_LanguageCodeTag[] = "LanguageCode";
const char UR_DfltSampleNameTag[] = "DefaultSampleName";
const char UR_UserImageSaveNTag[] = "SaveNthIImage";
const char UR_DisplayColumnsTag[] = "DisplayColumns";
const char UR_DecimalPrecisionTag[] = "DecimalPrecision";
const char UR_ExportFolderTag[] = "ExportFolder";
const char UR_DfltResultFileNameStrTag[] = "DefaultResultFileName";
const char UR_CSVFolderTag[] = "CSVFolder";
const char UR_PdfExportTag[] = "PdfExport";
const char UR_AllowFastModeTag[] = "AllowFastMode";
const char UR_WashTypeTag[] = "WashType";
const char UR_DilutionTag[] = "Dilution";
const char UR_DefaultCellTypeIdxTag[] = "DefaultCellTypeIndex";
const char UR_NumCellTypesTag[] = "NumUserCellTypes";
const char UR_CellTypeIdxListTag[] = "UserCellTypeIndexList";
const char UR_AnalysisDefIdxListTag[] = "UserAnalysisDefIndexList";
const char UR_NumUserPropsTag[] = "NumUserProperties";
const char UR_UserPropsIdxListTag[] = "UserPropertiesIndexList";
const char UR_AppPermissionsTag[] = "AppPermissions";
const char UR_AppPermissionsHashTag[] = "AppPermissionsHash";
const char UR_InstPermissionsTag[] = "InstrumentPermissions";
const char UR_InstPermissionsHashTag[] = "InstrumentPermissionsHash";

// for DB_UserRoleRecord
const char RO_IdNumTag[] = "RoleIdNum";
const char RO_IdTag[] = "RoleID";
const char RO_NameTag[] = "RoleName";
const char RO_RoleTypeTag[] = "RoleType";
const char RO_GroupMapListTag[] = "GroupMapList";
const char RO_CellTypeIdxListTag[] = "CellTypeIndexList";
const char RO_InstPermissionsTag[] = "InstrumentPermissions";
const char RO_AppPermissionsTag[] = "ApplicationPermissions";

// for DB_UserPropertiesRecord
const char UP_IdNumTag[] = "PropertyIdNum";
const char UP_IdxTag[] = "PropertyIndex";
const char UP_NameTag[] = "PropertyName";
const char UP_TypeTag[] = "PropertyType";

// for DB_UserSignatureRecord
const char SG_IdNumTag[] = "SignatureDefIdNum";
const char SG_IdTag[] = "SignatureDefID";
const char SG_ShortSigTag[] = "ShortSignature";
const char SG_ShortSigHashTag[] = "ShortSignatureHash";
const char SG_LongSigTag[] = "LongSignature";
const char SG_LongSigHashTag[] = "LongSignatureHash";

// for DB_ReagentInfoRecord
const char RX_IdNumTag[] = "ReagentIdNum";
const char RX_TypeNumTag[] = "ReagentTypeNum";
const char RX_CurrentSnTag[] = "Current";
const char RX_ContainerRfidSNTag[] = "ContainerTagSN";
const char RX_IdxListTag[] = "ReagentIndexList";
const char RX_NamesListTag[] = "ReagentNamesList";
const char RX_MixingCyclesTag[] = "MixingCycles";
const char RX_PackPartNumTag[] = "PackPartNum";
const char RX_LotNumTag[] = "LotNum";
const char RX_LotExpirationDateTag[] = "LotExpiration";
const char RX_InServiceDateTag[] = "InService";
const char RX_InServiceDaysTag[] = "ServiceLife";

// for DB_CellHealthReagentRecord
const char CH_IdNumTag[] = "IdNum";
const char CH_IdTag[] = "ID";
const char CH_TypeTag[] = "Type";
const char CH_NameTag[] = "Name";
const char CH_VolumeTag[] = "Volume";

// for DB_WorkflowRecord
const char WF_IdNumTag[] = "WorkflowIdNum";
const char WF_IdTag[] = "WorkflowID";
const char WF_NameTag[] = "WorkflowName";
const char WF_NumReagentsTag[] = "NumReagents";
const char WF_ReagentTypeIdxList[] = "ReagentTypeIndexList";
const char WF_SeqListTag[] = "WorkflowSeqList";

// for DB_BioProcessRecord
const char BP_IdNumTag[] = "BioProcessIdNum";
const char BP_IdTag[] = "BioProcessID";
const char BP_NameTag[] = "BioProcessName";

// for DB_QcProcessRecord
const char QC_IdNumTag[] = "QcIdNum";
const char QC_IdTag[] = "QcID";
const char QC_NameTag[] = "QcName";
const char QC_TypeTag[] = "QcType";
const char QC_CellTypeIdTag[] = "CellTypeID";
const char QC_CellTypeIndexTag[] = "CellTypeIndex";
const char QC_LotInfoTag[] = "LotInfo";
const char QC_LotExpirationTag[] = "LotExpiration";
const char QC_AssayValueTag[] = "AssayValue";
const char QC_AllowablePercentTag[] = "AllowablePercentage";
const char QC_SequenceTag[] = "QcSequence";
const char QC_CommentsTag[] = "Comments";
const char QC_RetiredTag[] = "Retired";

// for DB_InstrumentConfig
const char CFG_IdNumTag[] = "InstrumentIdNum";
const char CFG_InstSNTag[] = "InstrumentSN";
const char CFG_InstTypeTag[] = "InstrumentType";
const char CFG_DeviceNameTag[] = "DeviceName";
const char CFG_UiVerTag[] = "UIVersion";
const char CFG_SwVerTag[] = "SoftwareVersion";
const char CFG_AnalysisSwVerTag[] = "AnalysisSWVersion";
const char CFG_FwVerTag[] = "FirmwareVersion";
const char CFG_CameraTypeTag[] = "CameraType";
const char CFG_BrightFieldLedTypeTag[] = "BrightFieldLedType";
const char CFG_CameraFwVerTag[] = "CameraFWVersion";
const char CFG_CameraCfgTag[] = "CameraConfig";
const char CFG_PumpTypeTag[] = "PumpType";
const char CFG_PumpFwVerTag[] = "PumpFWVersion";
const char CFG_PumpCfgTag[] = "PumpConfig";
const char CFG_IlluminatorsInfoListTag[] = "IlluminatorsInfoList";
const char CFG_IlluminatorCfgTag[] = "IlluminatorConfig";
const char CFG_ConfigTypeTag[] = "ConfigType";
const char CFG_LogNameTag[] = "LogName";
const char CFG_LogMaxSizeTag[] = "LogMaxSize";
const char CFG_LogLevelTag[] = "LogSensitivity";
const char CFG_MaxLogsTag[] = "MaxLogs";
const char CFG_AlwaysFlushTag[] = "AlwaysFlush";
const char CFG_CameraErrLogNameTag[] = "CameraErrorLogName";
const char CFG_CameraErrLogMaxSizeTag[] = "CameraErrorLogMaxSize";
const char CFG_StorageErrLogNameTag[] = "StorageErrorLogName";
const char CFG_StorageErrLogMaxSizeTag[] = "StorageErrorLogMaxSize";
const char CFG_CarouselThetaHomeTag[] = "CarouselThetaHomeOffset";
const char CFG_CarouselRadiusOffsetTag[] = "CarouselRadiusOffset";
const char CFG_PlateThetaHomeTag[] = "PlateThetaHomePosOffset";
const char CFG_PlateThetaCalTag[] = "PlateThetaCalPos";
const char CFG_PlateRadiusCenterTag[] = "PlateRadiusCenterPos";
const char CFG_SaveImageTag[] = "SaveImage";
const char CFG_FocusPositionTag[] = "FocusPosition";
const char CFG_AutoFocusTag[] = "AutoFocus";
const char CFG_AbiMaxImageCntTag[] = "AbiMaxImageCount";
const char CFG_NudgeVolumeTag[] = "SampleNudgeVolume";
const char CFG_NudeSpeedTag[] = "SampleNudgeSpeed";
const char CFG_FlowCellDepthTag[] = "FlowCellDepth";
const char CFG_FlowCellConstantTag[] = "FlowCellDepthConstant";
const char CFG_RfidSimTag[] = "RfidSim";
const char CFG_LegacyDataTag[] = "LegacyData";
const char CFG_CarouselSimTag[] = "CarouselSimulator";
const char CFG_NightlyCleanOffsetTag[] = "NightlyCleanOffset";
const char CFG_LastNightlyCleanTag[] = "LastNightlyClean";
const char CFG_SecurityModeTag[] = "SecurityMode";
const char CFG_InactivityTimeoutTag[] = "InactivityTimeout";
const char CFG_PwdExpirationTag[] = "PasswordExpiration";
const char CFG_NormalShutdownTag[] = "NormalShutdown";
const char CFG_NextAnalysisDefIndexTag[] = "NextAnalysisDefIndex";
const char CFG_NextBCICellTypeIndexTag[] = "NextFactoryCellTypeIndex";
const char CFG_NextUserCellTypeIndexTag[] = "NextUserCellTypeIndex";
const char CFG_TotSamplesProcessedTag[] = "SamplesProcessed";
const char CFG_DiscardTrayCapacityTag[] = "DiscardCapacity";
const char CFG_EmailServerTag[] = "EmailServer";
const char CFG_AdSettingsTag[] = "ADSettings";
const char CFG_LanguageListTag[] = "LanguageList";
const char CFG_RunOptionsTag[] = "RunOptionDefaults";
const char CFG_AutomationEnabledTag[] = "AutomationEnabled";
const char CFG_ACupEnabledTag[] = "ACupEnabled";
const char CFG_AutomationInstalledTag[] = "AutomationInstalled";
const char CFG_AutomationPortTag[] = "AutomationPort";

// for DB_Calibration
const char CC_IdNumTag[] = "CalibrationIdNum";
const char CC_IdTag[] = "CalibrationID";
const char CC_InstSNTag[] = "InstrumentSN";
const char CC_CalDateTag[] = "CalibrationDate";
const char CC_CalUserIdTag[] = "CalibrationUserID";
const char CC_CalTypeTag[] = "CalibrationType";
const char CC_SlopeTag[] = "Slope";
const char CC_InterceptTag[] = "Intercept";
const char CC_ImageCntTag[] = "ImageCount";
const char CC_QueueIdTag[] = "CalQueueID";
const char CC_ConsumablesListTag[] = "ConsumablesList";

// for DB_LogEntry record
const char LOG_IdNumTag[] = "EntryIdNum";
const char LOG_EntryTypeTag[] = "EntryType";
const char LOG_EntryDateTag[] = "EntryDate";
const char LOG_LogEntryTag[] = "EntryText";



////////////////////////////////////////////////////////////////////////////////
// Automatic export scheduler configuration
////////////////////////////////////////////////////////////////////////////////

const char SCH_IdNumTag[] = "SchedulerConfigIdNum";
const char SCH_IdTag[] = "SchedulerConfigID";
const char SCH_NameTag[] = "SchedulerName";
const char SCH_CommentsTag[] = "Comments";
const char SCH_FilenameTemplateTag[] = "OutputFilenameTemplate";
const char SCH_OwnerIdTag[] = "OwnerID";
const char SCH_CreationDateTag[] = "CreationDate";
const char SCH_OutputTypeTag[] = "OutputType";
const char SCH_StartDateTag[] = "StartDate";
const char SCH_StartOffsetTag[] = "StartOffset";
const char SCH_RepetitionIntervalTag[] = "RepetitionInterval";
const char SCH_DayWeekIndicatorTag[] = "DayWeekIndicator";
const char SCH_MonthlyRunDayTag[] = "MonthlyRunDay";
const char SCH_DestinationFolderTag[] = "DestinationFolder";
const char SCH_DataTypeTag[] = "DataType";
const char SCH_FilterTypesTag[] = "FilterTypesList";
const char SCH_CompareOpsTag[] = "CompareOpsList";
const char SCH_CompareValsTag[] = "CompareValsList";
const char SCH_EnabledTag[] = "Enabled";
const char SCH_LastRunTimeTag[] = "LastRunTime";
const char SCH_LastSuccessRunTimeTag[] = "LastSuccessRunTime";
const char SCH_LastRunStatusTag[] = "LastRunStatus";
const char SCH_NotificationEmailTag[] = "NotificationEmail";
const char SCH_EmailServerTag[] = "EmailServer";
const char SCH_EmailServerPortTag[] = "EmailServerPort";
const char SCH_AuthenticateEmailTag[] = "AuthenticateEmail";
const char SCH_EmailAccountTag[] = "EmailAccount";
const char SCH_AccountAuthenticatorTag[] = "EmailAccountAuthenticator";



////////////////////////////////////////////////////////////////////////////////
// Analysis or acquisition generated data
////////////////////////////////////////////////////////////////////////////////

// for DB_SampleRecord
const char SM_IdNumTag[] = "SampleIdNum";
const char SM_IdTag[] = "SampleID";
const char SM_StatusTag[] = "SampleStatus";
const char SM_NameTag[] = "SampleName";
const char SM_CellTypeIdTag[] = "CellTypeID";
const char SM_CellTypeIdxTag[] = "CellTypeIndex";
const char SM_AnalysisDefIdTag[] = "AnalysisDefinitionID";
const char SM_AnalysisDefIdxTag[] = "AnalysisDefinitionIndex";
const char SM_LabelTag[] = "Label";
const char SM_BioProcessIdTag[] = "BioProcessID";
const char SM_QcProcessIdTag[] = "QcProcessID";
const char SM_WorkflowIdTag[] = "WorkflowID";
const char SM_CommentsTag[] = "Comments";
const char SM_WashTypeTag[] = "WashType";
const char SM_DilutionTag[] = "Dilution";
const char SM_OwnerUserIdTag[] = "OwnerUserID";
const char SM_RunUserTag[] = "RunUserID";
const char SM_AcquireDateTag[] = "AcquisitionDate";
const char SM_ImageSetIdTag[] = "ImageSetID";
const char SM_DustRefSetIdTag[] = "DustRefImageSetID";
const char SM_InstSNTag[] = "InstrumentSN";
const char SM_ImageAnalysisParamIdTag[] = "ImageAnalysisParamID";
const char SM_NumReagentsTag[] = "NumReagents";
const char SM_ReagentTypeNameListTag[] = "ReagentTypeNameList";
const char SM_ReagentPackNumListTag[] = "ReagentPackNumList";
const char SM_PackLotNumTag[] = "PackLotNumList";
const char SM_PackLotExpirationListTag[] = "PackLotExpirationList";
const char SM_PackInServiceListTag[] = "PackInServiceList";
const char SM_PackServiceExpirationListTag[] = "PackServiceExpirationList";

// for DB_AnalysisRecord
const char AN_IdNumTag[] = "AnalysisIdNum";
const char AN_IdTag[] = "AnalysisID";
const char AN_SampleIdTag[] = "SampleID";
const char AN_ImageSetIdTag[] = "ImageSetID";
const char AN_SummaryResultIdTag[] = "SummaryResultID";
const char AN_SResultIdTag[] = "SResultID";
const char AN_RunUserIdTag[] = "RunUserID";
const char AN_AnalysisDateTag[] = "AnalysisDate";
const char AN_InstSNTag[] = "InstrumentSN";
const char AN_BioProcessIdTag[] = "BioProcessID";
const char AN_QcProcessIdTag[] = "QcProcessID";
const char AN_WorkflowIdTag[] = "WorkflowID";
const char AN_ImageSequenceCntTag[] = "ImageSequenceCount";
const char AN_ImageSequenceIdListTag[] = "ImageSequenceIDList";

// DB_SummaryResultRecord
const char RS_IdNumTag[] = "SummaryResultIdNum";
const char RS_IdTag[] = "SummaryResultID";
const char RS_SampleIdTag[] = "SampleID";
const char RS_ImageSetIdTag[] = "ImageSetID";
const char RS_AnalysisIdTag[] = "AnalysisID";
const char RS_ResultDateTag[] = "ResultDate";
const char RS_SigListTag[] = "SignatureList";
const char RS_ImgAnalysisParamIdTag[] = "ImageAnalysisParamID";
const char RS_AnalysisDefIdTag[] = "AnalysisDefID";
const char RS_AnalysisParamIdTag[] = "AnalysisParamID";
const char RS_CellTypeIdTag[] = "CellTypeID";
const char RS_CellTypeIdxTag[] = "CellTypeIndex";
const char RS_StatusTag[] = "ProcessingStatus";
const char RS_TotCumulativeImgsTag[] = "TotCumulativeImages";
const char RS_TotCellsGPTag[] = "TotalCellsGP";
const char RS_TotCellsPOITag[] = "TotalCellsPOI";
const char RS_POIPopPercentTag[] = "POIPopulationPercent";
const char RS_CellConcGPTag[] = "CellConcGP";
const char RS_CellConcPOITag[] = "CellConcPOI";
const char RS_AvgDiamGPTag[] = "AvgDiamGP";
const char RS_AvgDiamPOITag[] = "AvgDiamPOI";
const char RS_AvgCircularityGPTag[] = "AvgCircularityGP";
const char RS_AvgCircularityPOITag[] = "AvgCircularityPOI";
const char RS_CoeffOfVarTag[] = "CoefficientOfVariance";
const char RS_AvgCellsPerImgTag[] = "AvgCellsPerImage";
const char RS_AvgBkgndIntensityTag[] = "AvgBkgndIntensity";
const char RS_TotBubbleCntTag[] = "TotalBubbleCount";
const char RS_LrgClusterCntTag[] = "LargeClusterCount";
const char RS_QcStatusTag[] = "QcStatus";

// DB_DetailedResultRecord
const char RD_IdNumTag[] = "DetailedResultIdNum";
const char RD_IdTag[] = "DetailedResultID";
const char RD_SampleIdTag[] = "SampleID";
const char RD_ImageIdTag[] = "ImageID";
const char RD_AnalysisIdTag[] = "AnalysisID";
const char RD_OwnerIdTag[] = "OwnerID";
const char RD_ResultDateTag[] = "ResultDate";
const char RD_ProcessingStatusTag[] = "ProcessingStatus";
const char RD_TotCumulativeImgsTag[] = "TotCumulativeImages";
const char RD_TotCellsGPTag[] = "TotalCellsGP";
const char RD_TotCellsPOITag[] = "TotalCellsPOI";
const char RD_POIPopPercentTag[] = "POIPopulationPercent";
const char RD_CellConcGPTag[] = "CellConcGP";
const char RD_CellConcPOITag[] = "CellConcPOI";
const char RD_AvgDiamGPTag[] = "AvgDiamGP";
const char RD_AvgDiamPOITag[] = "AvgDiamPOI";
const char RD_AvgCircularityGPTag[] = "AvgCircularityGP";
const char RD_AvgCircularityPOITag[] = "AvgCircularityPOI";
const char RD_AvgSharpnessGPTag[] = "AvgSharpnessGP";
const char RD_AvgSharpnessPOITag[] = "AvgSharpnessPOI";
const char RD_AvgEccentricityGPTag[] = "AvgEccentricityGP";
const char RD_AvgEccentricityPOITag[] = "AvgEccentricityPOI";
const char RD_AvgAspectRatioGPTag[] = "AvgAspectRatioGP";
const char RD_AvgAspectRatioPOITag[] = "AvgAspectRatioPOI";
const char RD_AvgRoundnessGPTag[] = "AvgRoundnessGP";
const char RD_AvgRoundnessPOITag[] = "AvgRoundnessPOI";
const char RD_AvgRawCellSpotBrightnessGPTag[] = "AvgRawCellSpotBrightnessGP";
const char RD_AvgRawCellSpotBrightnessPOITag[] = "AvgRawCellSpotBrightnessPOI";
const char RD_AvgCellSpotBrightnessGPTag[] = "AvgCellSpotBrightnessGP";
const char RD_AvgCellSpotBrightnessPOITag[] = "AvgCellSpotBrightnessPOI";
const char RD_AvgBkgndIntensityTag[] = "AvgBkgndIntensity";
const char RD_TotBubbleCntTag[] = "TotalBubbleCount";
const char RD_LrgClusterCntTag[] = "LargeClusterCount";

// DB_ImageResultRecord
const char RI_IdNumTag[] = "ResultIdNum";
const char RI_IdTag[] = "ResultID";
const char RI_SampleIdTag[] = "SampleID";
const char RI_ImageIdTag[] = "ImageID";
const char RI_AnalysisIdTag[] = "AnalysisID";
const char RI_ImageSeqNumTag[] = "ImageSeqNum";
const char RI_DetailedResultIdTag[] = "DetailedResultID";
const char RI_MaxFlChanPeakMapTag[] = "MaxNumOfPeaksFlChanMap";
const char RI_NumBlobsTag[] = "NumBlobs";
const char RI_BlobInfoListStrTag[] = "BlobInfoListStr";
const char RI_BlobCenterListStrTag[] = "BlobCenterListStr";
const char RI_BlobOutlineListStrTag[] = "BlobOutlineListStr";
const char RI_NumClustersTag[] = "NumClusters";
const char RI_ClusterCellCountListTag[] = "ClusterCellCountList";
const char RI_ClusterPolygonListStrTag[] = "ClusterPolygonListStr";
const char RI_ClusterRectListStrTag[] = "ClusterRectListStr";

// DB_SResultRecord
const char SR_IdNumTag[] = "SResultIdNum";
const char SR_IdTag[] = "SResultID";
const char SR_SampleIdTag[] = "SampleID";
const char SR_ImageIdTag[] = "ImageID";
const char SR_AnalysisIdTag[] = "AnalysisID";
const char SR_ProcessingSettingsIdTag[] = "ProcessingSettingsID";
const char SR_CumDetailedResultIdTag[] = "CumulativeDetailedResultID";
const char SR_CumMaxFlChanPeakMapTag[] = "CumMaxNumOfPeaksFlChanMap";
const char SR_ImageResultIdListTag[] = "ImageResultIDList";

// for DB_ImageSetRecord
const char IC_IdNumTag[] = "ImageSetIdNum";
const char IC_IdTag[] = "ImageSetID";
const char IC_SampleIdTag[] = "SampleID";
const char IC_CreationDateTag[] = "CreationDate";
const char IC_SetFolderTag[] = "ImageSetFolder";
const char IC_SequenceCntTag[] = "ImageSequenceCount";
const char IC_SequenceIdListTag[] = "ImageSequenceIDList";

// for DB_ImageSeqRecord
const char IS_IdNumTag[] = "ImageSequenceIdNum";
const char IS_IdTag[] = "ImageSequenceID";
const char IS_SetIdTag[] = "ImageSetID";
const char IS_SequenceNumTag[] = "SequenceNum";
const char IS_ImageCntTag[] = "ImageCount";
const char IS_FlChansTag[] = "FlChannels";
const char IS_ImageIdListTag[] = "ImageIDList";
const char IS_SequenceFolderTag[] = "ImageSequenceFolder";

// for DB_ImageRecord
const char IM_IdNumTag[] = "ImageIdNum";
const char IM_IdTag[] = "ImageID";
const char IM_SequenceIdTag[] = "ImageSequenceID";
const char IM_ImageChanTag[] = "ImageChannel";
const char IM_FileNameTag[] = "ImageFileName";

// DB_ClusterDataRecord
const char CD_IdNumTag[] = "ClusterIdNum";
const char CD_IdTag[] = "ClusterID";
const char CD_ImageResultIdTag[] = "ImageResultID";
const char CD_CellCountTag[] = "CellCount";
const char CD_ClusterPolygonTag[] = "ClusterPolygon";
const char CD_ClusterBoxStartXTag[] = "ClusterBoxStartX";
const char CD_ClusterBoxStartYTag[] = "ClusterBoxStartY";
const char CD_ClusterBoxHeightTag[] = "ClusterBoxHeight";
const char CD_ClusterBoxWidthTag[] = "ClusterBoxWidth";

// DB_BlobDataRecord
const char BD_IdNumTag[] = "BlobIdNum";
const char BD_IdTag[] = "BlobID";
const char BD_ImageResultIdTag[] = "ImageResultID";
const char BD_BlobInfoTag[] = "BlobInfo";
const char BD_BlobCenterTag[] = "BlobCenter";
const char BD_BlobOutlineTag[] = "BlobOutline";

