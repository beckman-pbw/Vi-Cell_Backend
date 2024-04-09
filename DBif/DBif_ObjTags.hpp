// Database interface : implementation file: Database table column tags
//

#pragma once

#include "pch.h"




extern const char ProtectedTag[];

////////////////////////////////////////////////////////////////////////////////
// Instrument configuration and analysis parameters
////////////////////////////////////////////////////////////////////////////////

// for DB_WorklistRecord
extern const char WL_IdNumTag[];
extern const char WL_IdTag[];
extern const char WL_StatusTag[];
extern const char WL_NameTag[];
extern const char WL_CommentsTag[];
extern const char WL_InstSNTag[];
extern const char WL_CreateUserIdTag[];
extern const char WL_RunUserIdTag[];
extern const char WL_RunDateTag[];
extern const char WL_AcquireSampleTag[];
extern const char WL_CarrierTypeTag[];
extern const char WL_PrecessionTag[];
extern const char WL_SaveImagesTag[];
extern const char WL_WashTypeTag[];
extern const char WL_DilutionTag[];
extern const char WL_DfltSetNameTag[];
extern const char WL_DfltItemNameTag[];
extern const char WL_ImageAnalysisParamIdTag[];
extern const char WL_AnalysisDefIdTag[];
extern const char WL_AnalysisDefIdxTag[];
extern const char WL_AnalysisParamIdTag[];
extern const char WL_CellTypeIdTag[];
extern const char WL_CellTypeIdxTag[];
extern const char WL_BioProcessIdTag[];
extern const char WL_QcProcessIdTag[];
extern const char WL_WorkflowIdTag[];
extern const char WL_SampleSetCntIdTag[];
extern const char WL_ProcessedSetCntTag[];
extern const char WL_SampleSetIdListTag[];

// for DB_SampleSetRecord
extern const char SS_IdNumTag[];
extern const char SS_IdTag[];
extern const char SS_StatusTag[];
extern const char SS_NameTag[];
extern const char SS_LabelTag[];
extern const char SS_CommentsTag[];
extern const char SS_CarrierTypeTag[];
extern const char SS_OwnerIdTag[];
extern const char SS_CreateDateTag[];
extern const char SS_ModifyDateTag[];
extern const char SS_RunDateTag[];
extern const char SS_WorklistIdTag[];
extern const char SS_SampleItemCntTag[];
extern const char SS_ProcessedItemCntTag[];
extern const char SS_SampleItemIdListTag[];

// for DB_SampleItemRecord
extern const char SI_IdNumTag[];
extern const char SI_IdTag[];
extern const char SI_StatusTag[];
extern const char SI_NameTag[];
extern const char SI_CommentsTag[];
extern const char SI_RunDateTag[];
extern const char SI_SampleSetIdTag[];
extern const char SI_SampleIdTag[];
extern const char SI_SaveImagesTag[];
extern const char SI_WashTypeTag[];
extern const char SI_DilutionTag[];
extern const char SI_LabelTag[];
extern const char SI_ImageAnalysisParamIdTag[];
extern const char SI_AnalysisDefIdTag[];
extern const char SI_AnalysisDefIdxTag[];
extern const char SI_AnalysisParamIdTag[];
extern const char SI_CellTypeIdTag[];
extern const char SI_CellTypeIdxTag[];
extern const char SI_BioProcessIdTag[];
extern const char SI_QcProcessIdTag[];
extern const char SI_WorkflowIdTag[];
extern const char SI_SamplePosTag[];

// for DB_CellTypeRecord
extern const char CT_IdNumTag[];
extern const char CT_IdTag[];
extern const char CT_IdxTag[];
extern const char CT_NameTag[];
extern const char CT_RetiredTag[];
extern const char CT_MaxImagesTag[];
extern const char CT_AspirationCyclesTag[];
extern const char CT_MinDiamMicronsTag[];
extern const char CT_MaxDiamMicronsTag[];
extern const char CT_MinCircularityTag[];
extern const char CT_SharpnessLimitTag[];
extern const char CT_NumCellIdentParamsTag[];
extern const char CT_CellIdentParamIdListTag[];
extern const char CT_DeclusterSettingsTag[];
extern const char CT_RoiExtentTag[];
extern const char CT_RoiXPixelsTag[];
extern const char CT_RoiYPixelsTag[];
extern const char CT_NumAnalysisSpecializationsTag[];
extern const char CT_AnalysisSpecializationIdListTag[];
extern const char CT_CalcCorrectionFactorTag[];

// for DB_AnalysisDefinitionRecord
extern const char AD_IdNumTag[];
extern const char AD_IdTag[];
extern const char AD_IdxTag[];
extern const char AD_NameTag[];
extern const char AD_NumReagentsTag[];
extern const char AD_ReagentTypeIdxListTag[];
extern const char AD_MixingCyclesTag[];
extern const char AD_NumIlluminatorsTag[];
extern const char AD_IlluminatorIdxListTag[];
extern const char AD_NumAnalysisParamsTag[];
extern const char AD_AnalysisParamIdListTag[];
extern const char AD_PopulationParamIdTag[];
extern const char AD_PopulationParamExistsTag[];

// for DB_AnalysisParamRecord
extern const char AP_IdNumTag[];
extern const char AP_IdTag[];
extern const char AP_InitializedTag[];
extern const char AP_LabelTag[];
extern const char AP_KeyTag[];
extern const char AP_SKeyTag[];
extern const char AP_SSKeyTag[];
extern const char AP_ThreshValueTag[];
extern const char AP_AboveThreshTag[];

// DB_ImageAnalysisParamRecord
extern const char IAP_IdNumTag[];
extern const char IAP_IdTag[];
extern const char IAP_AlgorithmModeTag[];
extern const char IAP_BubbleModeTag[];
extern const char IAP_DeclusterModeTag[];
extern const char IAP_SubPeakAnalysisModeTag[];
extern const char IAP_DilutionFactorTag[];
extern const char IAP_ROIXcoordsTag[];
extern const char IAP_ROIYcoordsTag[];
extern const char IAP_DeclusterAccumuLowTag[];
extern const char IAP_DeclusterMinDistanceLowTag[];
extern const char IAP_DeclusterAccumuMedTag[];
extern const char IAP_DeclusterMinDistanceMedTag[];
extern const char IAP_DeclusterAccumuHighTag[];
extern const char IAP_DeclusterMinDistanceHighTag[];
extern const char IAP_FovDepthTag[];
extern const char IAP_PixelFovTag[];
extern const char IAP_SizingSlopeTag[];
extern const char IAP_SizingInterceptTag[];
extern const char IAP_ConcSlopeTag[];
extern const char IAP_ConcInterceptTag[];
extern const char IAP_ConcImageControlCntTag[];
extern const char IAP_BubbleMinSpotAreaPrcntTag[];
extern const char IAP_BubbleMinSpotAreaBrightnessTag[];
extern const char IAP_BubbleRejectImgAreaPrcntTag[];
extern const char IAP_VisibleCellSpotAreaTag[];
extern const char IAP_FlScalableROITag[];
extern const char IAP_FLPeakPercentTag[];
extern const char IAP_NominalBkgdLevelTag[];
extern const char IAP_BkgdIntensityToleranceTag[];
extern const char IAP_CenterSpotMinIntensityTag[];
extern const char IAP_PeakIntensitySelectionAreaTag[];
extern const char IAP_CellSpotBrightnessExclusionTag[];
extern const char IAP_HotPixelEliminationModeTag[];
extern const char IAP_ImgBotAndRightBoundaryModeTag[];
extern const char IAP_SmallParticleSizeCorrectionTag[];

// DB_AnalysisInputSettingsRecord
extern const char AIP_IdNumTag[];
extern const char AIP_IdTag[];
extern const char AIP_ConfigParamMapTag[];
extern const char AIP_CellIdentParamListTag[];
extern const char AIP_PoiIdentParamListTag[];

// for DB_BlobIdentParamRecord

// for DB_FlIlluminatorRecord
extern const char IL_IdNumTag[];
extern const char IL_IdxTag[];
extern const char IL_TypeTag[];
extern const char IL_NameTag[];
extern const char IL_PosNumTag[];
extern const char IL_ToleranceTag[];
extern const char IL_MaxVoltageTag[];
extern const char IL_IllumWavelengthTag[];
extern const char IL_EmitWavelengthTag[];
extern const char IL_ExposureTimeMsTag[];
extern const char IL_PercentPowerTag[];
extern const char IL_SimmerVoltageTag[];
extern const char IL_LtcdTag[];
extern const char IL_CtldTag[];
extern const char IL_FeedbackDiodeTag[];

// for DB_UserRecord
extern const char UR_IdNumTag[];
extern const char UR_IdTag[];
extern const char UR_RetiredTag[];
extern const char UR_ADUserTag[];
extern const char UR_RoleIdTag[];
extern const char UR_UserNameTag[];
extern const char UR_DisplayNameTag[];
extern const char UR_CommentsTag[];
extern const char UR_UserEmailTag[];
extern const char UR_AuthenticatorListTag[];
extern const char UR_AuthenticatorDateTag[];
extern const char UR_LastLoginTag[];
extern const char UR_AttemptCntTag[];
extern const char UR_LanguageCodeTag[];
extern const char UR_DfltSampleNameTag[];
extern const char UR_UserImageSaveNTag[];
extern const char UR_DisplayColumnsTag[];
extern const char UR_DecimalPrecisionTag[];
extern const char UR_ExportFolderTag[];
extern const char UR_DfltResultFileNameStrTag[];
extern const char UR_CSVFolderTag[];
extern const char UR_PdfExportTag[];
extern const char UR_AllowFastModeTag[];
extern const char UR_WashTypeTag[];
extern const char UR_DilutionTag[];
extern const char UR_DefaultCellTypeIdxTag[];
extern const char UR_NumCellTypesTag[];
extern const char UR_CellTypeIdxListTag[];
extern const char UR_AnalysisDefIdxListTag[];
extern const char UR_NumUserPropsTag[];
extern const char UR_UserPropsIdxListTag[];
extern const char UR_AppPermissionsTag[];
extern const char UR_AppPermissionsHashTag[];
extern const char UR_InstPermissionsTag[];
extern const char UR_InstPermissionsHashTag[];

// for DB_UserRoleRecord
extern const char RO_IdNumTag[];
extern const char RO_IdTag[];
extern const char RO_NameTag[];
extern const char RO_RoleTypeTag[];
extern const char RO_GroupMapListTag[];
extern const char RO_CellTypeIdxListTag[];
extern const char RO_InstPermissionsTag[];
extern const char RO_AppPermissionsTag[];

// for DB_UserPropertiesRecord
extern const char UP_IdNumTag[];
extern const char UP_IdxTag[];
extern const char UP_NameTag[];
extern const char UP_TypeTag[];

// for DB_UserSignatureRecord
extern const char SG_IdNumTag[];
extern const char SG_IdTag[];
extern const char SG_ShortSigTag[];
extern const char SG_ShortSigHashTag[];
extern const char SG_LongSigTag[];
extern const char SG_LongSigHashTag[];

// for DB_ReagentInfoRecord
extern const char RX_IdNumTag[];
extern const char RX_TypeNumTag[];
extern const char RX_CurrentSnTag[];
extern const char RX_ContainerRfidSNTag[];
extern const char RX_IdxListTag[];
extern const char RX_NamesListTag[];
extern const char RX_MixingCyclesTag[];
extern const char RX_PackPartNumTag[];
extern const char RX_LotNumTag[];
extern const char RX_LotExpirationDateTag[];
extern const char RX_InServiceDateTag[];
extern const char RX_InServiceDaysTag[];

// for DB_CellHealthReagentRecord
extern const char CH_IdNumTag[];
extern const char CH_IdTag[];
extern const char CH_TypeTag[];
extern const char CH_NameTag[];
extern const char CH_VolumeTag[];

// for DB_WorkflowRecord
extern const char WF_IdNumTag[];
extern const char WF_IdTag[];
extern const char WF_NameTag[];
extern const char WF_NumReagentsTag[];
extern const char WF_ReagentTypeIdxList[];
extern const char WF_SeqListTag[];

// for DB_BioProcessRecord
extern const char BP_IdNumTag[];
extern const char BP_IdTag[];
extern const char BP_NameTag[];

// for DB_QcProcessRecord
extern const char QC_IdNumTag[];
extern const char QC_IdTag[];
extern const char QC_NameTag[];
extern const char QC_TypeTag[];
extern const char QC_CellTypeIdTag[];
extern const char QC_CellTypeIndexTag[];
extern const char QC_LotInfoTag[];
extern const char QC_LotExpirationTag[];
extern const char QC_AssayValueTag[];
extern const char QC_AllowablePercentTag[];
extern const char QC_SequenceTag[];
extern const char QC_CommentsTag[];
extern const char QC_RetiredTag[];

// for DB_InstrumentConfig
extern const char CFG_IdNumTag[];
extern const char CFG_InstSNTag[];
extern const char CFG_InstTypeTag[];
extern const char CFG_DeviceNameTag[];
extern const char CFG_UiVerTag[];
extern const char CFG_SwVerTag[];
extern const char CFG_AnalysisSwVerTag[];
extern const char CFG_FwVerTag[];
extern const char CFG_CameraTypeTag[];
extern const char CFG_BrightFieldLedTypeTag[];
extern const char CFG_CameraFwVerTag[];
extern const char CFG_CameraCfgTag[];
extern const char CFG_PumpTypeTag[];
extern const char CFG_PumpFwVerTag[];
extern const char CFG_PumpCfgTag[];
extern const char CFG_IlluminatorsInfoListTag[];
extern const char CFG_IlluminatorCfgTag[];
extern const char CFG_ConfigTypeTag[];
extern const char CFG_LogNameTag[];
extern const char CFG_LogMaxSizeTag[];
extern const char CFG_LogLevelTag[];
extern const char CFG_MaxLogsTag[];
extern const char CFG_AlwaysFlushTag[];
extern const char CFG_CameraErrLogNameTag[];
extern const char CFG_CameraErrLogMaxSizeTag[];
extern const char CFG_StorageErrLogNameTag[];
extern const char CFG_StorageErrLogMaxSizeTag[];
extern const char CFG_CarouselThetaHomeTag[];
extern const char CFG_CarouselRadiusOffsetTag[];
extern const char CFG_PlateThetaHomeTag[];
extern const char CFG_PlateThetaCalTag[];
extern const char CFG_PlateRadiusCenterTag[];
extern const char CFG_SaveImageTag[];
extern const char CFG_FocusPositionTag[];
extern const char CFG_AutoFocusTag[];
extern const char CFG_AbiMaxImageCntTag[];
extern const char CFG_NudgeVolumeTag[];
extern const char CFG_NudeSpeedTag[];
extern const char CFG_FlowCellDepthTag[];
extern const char CFG_FlowCellConstantTag[];
extern const char CFG_RfidSimTag[];
extern const char CFG_LegacyDataTag[];
extern const char CFG_CarouselSimTag[];
extern const char CFG_NightlyCleanOffsetTag[];
extern const char CFG_LastNightlyCleanTag[];
extern const char CFG_SecurityModeTag[];
extern const char CFG_InactivityTimeoutTag[];
extern const char CFG_PwdExpirationTag[];
extern const char CFG_NormalShutdownTag[];
extern const char CFG_NextAnalysisDefIndexTag[];
extern const char CFG_NextBCICellTypeIndexTag[];
extern const char CFG_NextUserCellTypeIndexTag[];
extern const char CFG_TotSamplesProcessedTag[];
extern const char CFG_DiscardTrayCapacityTag[];
extern const char CFG_EmailServerTag[];
extern const char CFG_AdSettingsTag[];
extern const char CFG_LanguageListTag[];
extern const char CFG_RunOptionsTag[];
extern const char CFG_AutomationEnabledTag[];
extern const char CFG_ACupEnabledTag[];
extern const char CFG_AutomationInstalledTag[];
extern const char CFG_AutomationPortTag[];

// for DB_Calibration
extern const char CC_IdNumTag[];
extern const char CC_IdTag[];
extern const char CC_InstSNTag[];
extern const char CC_CalDateTag[];
extern const char CC_CalUserIdTag[];
extern const char CC_CalTypeTag[];
extern const char CC_SlopeTag[];
extern const char CC_InterceptTag[];
extern const char CC_ImageCntTag[];
extern const char CC_QueueIdTag[];
extern const char CC_ConsumablesListTag[];

// for DB_LogEntry record
extern const char LOG_IdNumTag[];
extern const char LOG_EntryTypeTag[];
extern const char LOG_EntryDateTag[];
extern const char LOG_LogEntryTag[];



////////////////////////////////////////////////////////////////////////////////
// Automatic export scheduler configuration
////////////////////////////////////////////////////////////////////////////////

extern const char SCH_IdNumTag[];
extern const char SCH_IdTag[];
extern const char SCH_NameTag[];
extern const char SCH_CommentsTag[];
extern const char SCH_FilenameTemplateTag[];
extern const char SCH_OwnerIdTag[];
extern const char SCH_CreationDateTag[];
extern const char SCH_OutputTypeTag[];
extern const char SCH_StartDateTag[];
extern const char SCH_StartOffsetTag[];
extern const char SCH_RepetitionIntervalTag[];
extern const char SCH_DayWeekIndicatorTag[];
extern const char SCH_MonthlyRunDayTag[];
extern const char SCH_DestinationFolderTag[];
extern const char SCH_DataTypeTag[];
extern const char SCH_FilterTypesTag[];
extern const char SCH_CompareOpsTag[];
extern const char SCH_CompareValsTag[];
extern const char SCH_EnabledTag[];
extern const char SCH_LastRunTimeTag[];
extern const char SCH_LastSuccessRunTimeTag[];
extern const char SCH_LastRunStatusTag[];
extern const char SCH_NotificationEmailTag[];
extern const char SCH_EmailServerTag[];
extern const char SCH_EmailServerPortTag[];
extern const char SCH_AuthenticateEmailTag[];
extern const char SCH_EmailAccountTag[];
extern const char SCH_AccountAuthenticatorTag[];



////////////////////////////////////////////////////////////////////////////////
// Analysis or acquisition generated data
////////////////////////////////////////////////////////////////////////////////

// for DB_SampleRecord
extern const char SM_IdNumTag[];
extern const char SM_IdTag[];
extern const char SM_StatusTag[];
extern const char SM_NameTag[];
extern const char SM_CellTypeIdTag[];
extern const char SM_CellTypeIdxTag[];
extern const char SM_AnalysisDefIdTag[];
extern const char SM_AnalysisDefIdxTag[];
extern const char SM_LabelTag[];
extern const char SM_BioProcessIdTag[];
extern const char SM_QcProcessIdTag[];
extern const char SM_WorkflowIdTag[];
extern const char SM_CommentsTag[];
extern const char SM_WashTypeTag[];
extern const char SM_DilutionTag[];
extern const char SM_OwnerUserIdTag[];
extern const char SM_RunUserTag[];
extern const char SM_AcquireDateTag[];
extern const char SM_ImageSetIdTag[];
extern const char SM_DustRefSetIdTag[];
extern const char SM_InstSNTag[];
extern const char SM_ImageAnalysisParamIdTag[];
extern const char SM_NumReagentsTag[];
extern const char SM_ReagentTypeNameListTag[];
extern const char SM_ReagentPackNumListTag[];
extern const char SM_PackLotNumTag[];
extern const char SM_PackLotExpirationListTag[];
extern const char SM_PackInServiceListTag[];
extern const char SM_PackServiceExpirationListTag[];

// for DB_AnalysisRecord
extern const char AN_IdNumTag[];
extern const char AN_IdTag[];
extern const char AN_SampleIdTag[];
extern const char AN_ImageSetIdTag[];
extern const char AN_SummaryResultIdTag[];
extern const char AN_SResultIdTag[];
extern const char AN_RunUserIdTag[];
extern const char AN_AnalysisDateTag[];
extern const char AN_InstSNTag[];
extern const char AN_BioProcessIdTag[];
extern const char AN_QcProcessIdTag[];
extern const char AN_WorkflowIdTag[];
extern const char AN_ImageSequenceCntTag[];
extern const char AN_ImageSequenceIdListTag[];

// DB_SummaryResultRecord
extern const char RS_IdNumTag[];
extern const char RS_IdTag[];
extern const char RS_SampleIdTag[];
extern const char RS_ImageSetIdTag[];
extern const char RS_AnalysisIdTag[];
extern const char RS_ResultDateTag[];
extern const char RS_SigListTag[];
extern const char RS_ImgAnalysisParamIdTag[];
extern const char RS_AnalysisDefIdTag[];
extern const char RS_AnalysisParamIdTag[];
extern const char RS_CellTypeIdTag[];
extern const char RS_CellTypeIdxTag[];
extern const char RS_StatusTag[];
extern const char RS_TotCumulativeImgsTag[];
extern const char RS_TotCellsGPTag[];
extern const char RS_TotCellsPOITag[];
extern const char RS_POIPopPercentTag[];
extern const char RS_CellConcGPTag[];
extern const char RS_CellConcPOITag[];
extern const char RS_AvgDiamGPTag[];
extern const char RS_AvgDiamPOITag[];
extern const char RS_AvgCircularityGPTag[];
extern const char RS_AvgCircularityPOITag[];
extern const char RS_CoeffOfVarTag[];
extern const char RS_AvgCellsPerImgTag[];
extern const char RS_AvgBkgndIntensityTag[];
extern const char RS_TotBubbleCntTag[];
extern const char RS_LrgClusterCntTag[];
extern const char RS_QcStatusTag[];

// DB_DetailedResultRecord
extern const char RD_IdNumTag[];
extern const char RD_IdTag[];
extern const char RD_SampleIdTag[];
extern const char RD_ImageIdTag[];
extern const char RD_AnalysisIdTag[];
extern const char RD_OwnerIdTag[];
extern const char RD_ResultDateTag[];
extern const char RD_ProcessingStatusTag[];
extern const char RD_TotCumulativeImgsTag[];
extern const char RD_TotCellsGPTag[];
extern const char RD_TotCellsPOITag[];
extern const char RD_POIPopPercentTag[];
extern const char RD_CellConcGPTag[];
extern const char RD_CellConcPOITag[];
extern const char RD_AvgDiamGPTag[];
extern const char RD_AvgDiamPOITag[];
extern const char RD_AvgCircularityGPTag[];
extern const char RD_AvgCircularityPOITag[];
extern const char RD_AvgSharpnessGPTag[];
extern const char RD_AvgSharpnessPOITag[];
extern const char RD_AvgEccentricityGPTag[];
extern const char RD_AvgEccentricityPOITag[];
extern const char RD_AvgAspectRatioGPTag[];
extern const char RD_AvgAspectRatioPOITag[];
extern const char RD_AvgRoundnessGPTag[];
extern const char RD_AvgRoundnessPOITag[];
extern const char RD_AvgRawCellSpotBrightnessGPTag[];
extern const char RD_AvgRawCellSpotBrightnessPOITag[];
extern const char RD_AvgCellSpotBrightnessGPTag[];
extern const char RD_AvgCellSpotBrightnessPOITag[];
extern const char RD_AvgBkgndIntensityTag[];
extern const char RD_TotBubbleCntTag[];
extern const char RD_LrgClusterCntTag[];

// DB_ImageResultRecord
extern const char RI_IdNumTag[];
extern const char RI_IdTag[];
extern const char RI_SampleIdTag[];
extern const char RI_ImageIdTag[];
extern const char RI_AnalysisIdTag[];
extern const char RI_ImageSeqNumTag[];
extern const char RI_DetailedResultIdTag[];
extern const char RI_MaxFlChanPeakMapTag[];
extern const char RI_NumBlobsTag[];
extern const char RI_BlobInfoListStrTag[];
extern const char RI_BlobCenterListStrTag[];
extern const char RI_BlobOutlineListStrTag[];
extern const char RI_NumClustersTag[];
extern const char RI_ClusterCellCountListTag[];
extern const char RI_ClusterPolygonListStrTag[];
extern const char RI_ClusterRectListStrTag[];

// DB_SResultRecord
extern const char SR_IdNumTag[];
extern const char SR_IdTag[];
extern const char SR_SampleIdTag[];
extern const char SR_ImageIdTag[];
extern const char SR_AnalysisIdTag[];
extern const char SR_ProcessingSettingsIdTag[];
extern const char SR_CumDetailedResultIdTag[];
extern const char SR_CumMaxFlChanPeakMapTag[];
extern const char SR_ImageResultIdListTag[];

// for DB_ImageSetRecord
extern const char IC_IdNumTag[];
extern const char IC_IdTag[];
extern const char IC_SampleIdTag[];
extern const char IC_CreationDateTag[];
extern const char IC_SetFolderTag[];
extern const char IC_SequenceCntTag[];
extern const char IC_SequenceIdListTag[];

// for DB_ImageSeqRecord
extern const char IS_IdNumTag[];
extern const char IS_IdTag[];
extern const char IS_SetIdTag[];
extern const char IS_SequenceNumTag[];
extern const char IS_ImageCntTag[];
extern const char IS_FlChansTag[];
extern const char IS_ImageIdListTag[];
extern const char IS_SequenceFolderTag[];

// for DB_ImageRecord
extern const char IM_IdNumTag[];
extern const char IM_IdTag[];
extern const char IM_SequenceIdTag[];
extern const char IM_ImageChanTag[];
extern const char IM_FileNameTag[];

// DB_ClusterDataRecord
extern const char CD_IdNumTag[];
extern const char CD_IdTag[];
extern const char CD_ImageResultIdTag[];
extern const char CD_CellCountTag[];
extern const char CD_ClusterPolygonTag[];
extern const char CD_ClusterBoxStartXTag[];
extern const char CD_ClusterBoxStartYTag[];
extern const char CD_ClusterBoxHeightTag[];
extern const char CD_ClusterBoxWidthTag[];

// DB_BlobDataRecord
extern const char BD_IdNumTag[];
extern const char BD_IdTag[];
extern const char BD_ImageResultIdTag[];
extern const char BD_BlobInfoTag[];
extern const char BD_BlobCenterTag[];
extern const char BD_BlobOutlineTag[];

