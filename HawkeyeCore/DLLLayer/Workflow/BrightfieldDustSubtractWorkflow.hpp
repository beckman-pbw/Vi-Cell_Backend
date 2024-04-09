#pragma once

#include <iostream>

#include "ImageWrapperDLL.hpp"
#include "Workflow.hpp"

class BrightfieldDustSubtractWorkflow : public Workflow
{
public:
	enum eBrightfieldDustSubtractionState : uint16_t
	{
		bds_Idle = 0,
		bds_AspiratingBuffer,
		bds_DispensingBufferToFlowCell,
		bds_AcquiringImages,
		bds_ProcessingImages,
		bds_WaitingOnUserApproval,
		bds_SettingReferenceImage,
		bds_Cancelling,
		bds_Completed,
		bds_Failed,
		bds_FindingTube,
		bds_TfComplete,
		bds_TfCancelled,
	};

	typedef std::function<void (eBrightfieldDustSubtractionState, ImageWrapperDLL /*dust_ref*/,
		uint16_t /*num_dust_images*/, std::vector<ImageWrapperDLL> /*source_dust_images*/)> brightfield_dustsubtraction_callback_DLL;

	BrightfieldDustSubtractWorkflow (brightfield_dustsubtraction_callback_DLL callback);
	virtual ~BrightfieldDustSubtractWorkflow();

	virtual HawkeyeError execute() override;
	virtual HawkeyeError execute(uint16_t jumpToState) override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;
	virtual void processImage (const std::vector<cv::Mat>& acquiredImages, std::function<void(HawkeyeError he)> callback);
	virtual bool settingDustImage(const cv::Mat& image);
	virtual std::function<void(cv::Mat)> getCameraCallback() override;
	virtual bool executeCleaningCycleOnTermination(
		size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete) override;

private:
	void onBDSCameraTrigger(cv::Mat image);
	void updateHost(uint16_t currentState);
	void setCleaningCycleIndices();
	void cleanupBeforeExit(std::function<void(bool)> callback);
	void reset();

	brightfield_dustsubtraction_callback_DLL callback_;
	std::vector<cv::Mat> acquiredImages_;
	std::shared_ptr<cv::Mat> bdsImage_;
	std::function<void(cv::Mat)> cameraCaptureCallback_;
};
