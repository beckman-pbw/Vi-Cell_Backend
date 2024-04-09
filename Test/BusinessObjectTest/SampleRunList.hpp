#pragma once

#include <iostream>
#include <fstream>

#include <string>

#include "stdafx.h"


class SampleRunList
{
public:
	
	static const std::string InitializeCmd;
	static const std::string CameraCmd;
	static const std::string CameraParamCapture;
    static const std::string CameraParamTakePicture;
    static const std::string CameraParamSetExposure;
	static const std::string CameraParamSetGain;
	static const std::string CameraParamRemoveGainLimits;
    static const std::string CameraAdjustBackgroundIntensity;
	static const std::string CarrierSelectCmd;
	static const std::string CarrierSelectParamCarousel;
	static const std::string CarrierSelectParam96WellPlate;
	static const std::string FocusCmd;
	static const std::string FocusParamMoveHome;
	static const std::string FocusParamMoveCenter;
	static const std::string FocusParamMoveMax;
	static const std::string FocusParamMoveUpFast;
	static const std::string FocusParamMoveDownFast;
	static const std::string FocusParamMoveUpCoarse;
	static const std::string FocusParamMoveDownCoarse;
	static const std::string FocusParamMoveUpFine;
	static const std::string FocusParamMoveDownFine;
//	static const std::string FocusParamAdjust;	// ???
	static const std::string FocusParamSetCoarseStep;
	static const std::string FocusParamSetFineStep;
    static const std::string FocusParamSetPosition;
    static const std::string FocusParamRun;
    static const std::string FocusParamRunAnalysis;
    static const std::string DirectoryCmd;
	static const std::string LedCmd;
//    static const std::string LedParamSetLedToUse;
	static const std::string LedParamSetPower;
	static const std::string LedParamSetOnTime;
	static const std::string RunScriptCmd;
	static const std::string RunScriptReturnCmd;
	static const std::string ProbeCmd;
	static const std::string ProbeParamUp;
	static const std::string ProbeParamDown;
	static const std::string ReagentCmd;
	static const std::string ReagentParamUp;
	static const std::string ReagentParamDown;
    static const std::string ReagentParamPurge;
    static const std::string ReagentParamDoor;
    static const std::string RfidCmd;
	static const std::string RfidParamScan;
	static const std::string RfidParamRead;
	static const std::string SyringeCmd;
	static const std::string SyringeParamInitialize;
	static const std::string SyringeParamMove;
	static const std::string SyringeParamValve;
	static const std::string TubeCmd;
//    static const std::string TubeParamSetHomePosition;
	static const std::string StopCmd;
	static const std::string WaitCmd;
	static const std::string WaitForKeyPressCmd;

	static const std::string ImageAnalysisCmd;
	static const std::string ImageAnalysisParamEnable;
	static const std::string ImageAnalysisParamInstrumentConfigFile;
	static const std::string ImageAnalysisParamCellAnalysisConfigFile;
	static const std::string ImageAnalysisParamRun;

	static const std::string DownloadK70;

	SampleRunList::SampleRunList();
	virtual SampleRunList::~SampleRunList();

	bool open (std::string filename);
    bool read (std::string& cmd, std::vector<std::string>& strParams);
	bool readNew (std::string& cmd, std::vector<std::string>& strParams);
	bool loadWorkflow (std::string filename);
	bool close() { cmdFileHandle_.close(); }

	bool getWorkflowType();

private:
	typedef struct {
		std::string cmdFilename;
		uint32_t    lineNum;
	} CmdFileEntry_t;

	std::string cmdFilename_;
	std::ifstream cmdFileHandle_;
	uint32_t curLineNum_;
	std::vector<CmdFileEntry_t> cmdFileList_;
};
