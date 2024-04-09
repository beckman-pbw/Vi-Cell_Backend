#include "stdafx.h"

#include <functional>

#include "AnalysisDefinitionDLL.hpp"
#include "AuditLog.hpp"
#include "BlobLabel.h"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "Logger.hpp"
#include "SampleDefinitionDLL.hpp"
#include "SystemErrors.hpp"
#include "WorklistDLL.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_Worklist";

static std::unique_ptr<WorklistDLL> pWorklistDLL;

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetCurrentSample (SampleDefinition*& sd)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetCurrentSample: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "GetCurrentSample: <exit, no Worklist has been set>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "GetCurrentSample: <exit>");

	return pWorklistDLL->GetCurrentSample (sd);
}

//*****************************************************************************
void HawkeyeLogicImpl::SetWorklist (const Worklist& wl, std::function<void (HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "SetWorklist: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);
	HAWKEYE_ASSERT (MODULENAME, pHawkeyeServices_);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!pWorklistDLL)
	{
		pWorklistDLL = std::make_unique<WorklistDLL> (pHawkeyeServices_);
	}

	auto cb_wrapper = [=](HawkeyeError he)->void
	{
		pHawkeyeServices_->enqueueExternal (onComplete, he);
	};

	pWorklistDLL->Set (wl, cb_wrapper);
}

//*****************************************************************************
void HawkeyeLogicImpl::AddSampleSet (const SampleSet& ss, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "AddSampleSet: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "AddSampleSet: <exit, no Worklist has been set>");
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	pWorklistDLL->AddSampleSet (ss, [this, onComplete](HawkeyeError he) -> void
		{
			pHawkeyeServices_->enqueueExternal (onComplete, he);
		});
}

//*****************************************************************************
void HawkeyeLogicImpl::CancelSampleSet (
	uint16_t sampleSetIndex,
	std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "CancelSampleSet: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "CancelSampleSet: <exit, no Worklist has been set>");
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	pWorklistDLL->CancelSampleSet (sampleSetIndex, [this, onComplete](HawkeyeError he) -> void
		{
			pHawkeyeServices_->enqueueExternal (onComplete, he);
		});
}

//*****************************************************************************
void HawkeyeLogicImpl::StartProcessing (const char* username, const char* password,
	                                    WorklistDLL::sample_status_callback_DLL onSampleStatus,
	                                    WorklistDLL::sample_image_result_callback_DLL onSampleImageProcessed,
	                                    WorklistDLL::sample_status_callback_DLL onSampleComplete,
                                        WorklistDLL::worklist_completion_callback_DLL onWorklistComplete,
                                        std::function<void(HawkeyeError)> onComplete)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "StartProcessing: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);
	
	if (!onSampleStatus || !onSampleImageProcessed || !onSampleComplete || !onWorklistComplete)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "StartProcessing: <exit, invalid callback arguments>");
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
		return;
	}

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "StartProcessing: <exit, no Worklist has been set>");
		onComplete (HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	pWorklistDLL->Start (onSampleStatus, onSampleImageProcessed, onSampleComplete, onWorklistComplete,
		[this, onComplete](HawkeyeError he) -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "StartProcessing: <exit>");
			pHawkeyeServices_->enqueueExternal (onComplete, he);
		});
}

//*****************************************************************************
//NOTE: not currently used by UI...
//*****************************************************************************
void HawkeyeLogicImpl::SetImageOutputTypePreference (eImageOutputType image_type, HawkeyeErrorCallback callback)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "SetImageOutputTypePreference: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "SetImageOutputTypePreference: <exit, no Worklist has been set>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
	}

	pWorklistDLL->SetImageOutputTypePreference (image_type);

	pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::PauseProcessing(const char* username, const char* password)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "PauseProcessing: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "PauseProcessing: <exit, no Worklist has been set>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L ().Log (MODULENAME, severity_level::normal, "PauseProcessing: <exit>");

	return pWorklistDLL->Pause();
}

//*****************************************************************************
void HawkeyeLogicImpl::ResumeProcessing (const char* username, const char* password, HawkeyeErrorCallback callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "ResumeProcessing: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}


	if (!pWorklistDLL)
	{
		Logger::L ().Log (MODULENAME, severity_level::warning, "ResumeProcessing: <exit, no Worklist has been set>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
	}

	if (pWorklistDLL)
	{
		pWorklistDLL->Resume (callback);
	}
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::StopProcessing(const char* username, const char* password)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StopProcessing: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!pWorklistDLL)
	{
		Logger::L().Log( MODULENAME, severity_level::warning, "StopProcessing: <exit, no Worklist has been set>" );
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "StopProcessing: <exit>");

	return pWorklistDLL->Stop();
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSampleSet (SampleSet* list, uint32_t num_samplesets)
{
	for (uint32_t i = 0; i < num_samplesets; i++)
	{
		if (list[i].name)
		{
			delete[] list[i].name;
		}
		if (list[i].username)
		{
			delete[] list[i].username;
		}

		FreeSampleDefinition (list[i].samples, list[i].numSamples);
	}

	delete[] list;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeSampleDefinition (SampleDefinition* sampleList, uint32_t numSamples)
{
	//	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleDefinition: <enter>");

	if (!sampleList)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleDefinition: <exit, null_ptr>");
		return;
	}

	for (uint32_t i = 0; i < numSamples; i++)
	{
		sampleList[i].clear();
	}

	delete[] sampleList;

	//	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleDefinition: <exit>");
}
