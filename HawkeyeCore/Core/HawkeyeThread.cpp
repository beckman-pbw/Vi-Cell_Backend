#include "stdafx.h"

#include "HawkeyeThread.hpp"
#include "logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "HawkeyeThread";

//*****************************************************************************
HawkeyeThread::HawkeyeThread(std::string threadOwnerName)
{
	const uint8_t num_of_thread = 1;
	pSingleThreadPool_ = std::make_unique<HawkeyeThreadPool>(true/*auto start*/, num_of_thread, threadOwnerName);
}

//*****************************************************************************
HawkeyeThread::~HawkeyeThread()
{
	if (pSingleThreadPool_)
	{
		pSingleThreadPool_->Stop();
	}
	pSingleThreadPool_.reset();
}

//*****************************************************************************
void HawkeyeThread::Pause() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	pSingleThreadPool_->Pause();
}

//*****************************************************************************
void HawkeyeThread::Resume() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	pSingleThreadPool_->Resume();
}

void HawkeyeThread::Enqueue(std::function<void(void)> work)
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	pSingleThreadPool_->Enqueue(work);
}

//*****************************************************************************
std::shared_ptr<boost::asio::io_context> HawkeyeThread::GetIoService() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	return pSingleThreadPool_->GetIoService();
}

//*****************************************************************************
bool HawkeyeThread::isRunningOnThisIos() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	return pSingleThreadPool_->isRunningOnThisIos();
}

//*****************************************************************************
boost::optional<std::thread::id> HawkeyeThread::getThreadId() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);

	const auto& ids = pSingleThreadPool_->getThreadIds();
	if (!ids.empty())
	{
		return ids.front();
	}
	return boost::none;
}

//*****************************************************************************
bool HawkeyeThread::IsActive() const
{
	HAWKEYE_ASSERT (MODULENAME, pSingleThreadPool_);
	return pSingleThreadPool_->IsActive();
}
