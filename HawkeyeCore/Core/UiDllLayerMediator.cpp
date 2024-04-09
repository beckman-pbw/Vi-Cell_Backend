#include "stdafx.h"

#include "UiDllLayerMediator.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "UiDllLayerMediator";
static const uint8_t NumberOfCallbackThread = 3;

UiDllLayerMediator::UiDllLayerMediator(
	std::shared_ptr<boost::asio::io_context> pMainIoSvc)
	: pMainIoSvc_(pMainIoSvc)
{ }

UiDllLayerMediator::~UiDllLayerMediator()
{
	pMainIoSvc_.reset();
	if (pThreadPool_)
	{
		pThreadPool_->Stop();
	}
}

void UiDllLayerMediator::runCallbackOnUniqueThread(std::function<void()> work)
{
	HAWKEYE_ASSERT (MODULENAME, work);

	if (!pThreadPool_)
	{
		pThreadPool_ = std::make_unique<HawkeyeThreadPool>(true, NumberOfCallbackThread, "UiDllLayerMediator_Thread");
	}
	pThreadPool_->Enqueue(work);
}

void UiDllLayerMediator::printLog(std::string & logInfo, bool enableLog)
{
	auto severity = enableLog ? severity_level::debug1 : severity_level::debug3;
	if (Logger::L().IsOfInterest(severity))
	{
		Logger::L().Log (MODULENAME, severity, logInfo);
	}
}