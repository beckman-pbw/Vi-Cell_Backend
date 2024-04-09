#include "stdafx.h"

#include "HawkeyeThreadPool.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "HawkeyeThreadPool";

//*****************************************************************************
HawkeyeThreadPool::HawkeyeThreadPool(bool autoStart, uint8_t numThread, std::string threadOwnerName)
	: numThread_(numThread)
	, threadOwnerName_(threadOwnerName)
{
	HAWKEYE_ASSERT (MODULENAME, (numThread_ > 0));

	if (autoStart)
	{
		Start();
	}
}

//*****************************************************************************
HawkeyeThreadPool::~HawkeyeThreadPool()
{ 
	Stop();
}

//*****************************************************************************
void HawkeyeThreadPool::Start()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Start: <enter> : " + threadOwnerName_);

	// Start local queue processing with no existing state.
	Stop();
	if (!plocal_iosvc_)
	{
		plocal_iosvc_.reset(new boost::asio::io_context());
	}

	plocal_work_.reset(new boost::asio::io_context::work(*plocal_iosvc_));

	auto THREAD = [p_ios = plocal_iosvc_.get()]() -> void
	{
		p_ios->run();
	};

	v_thread_pool_.resize(numThread_);
	v_thread_ids_.reserve(numThread_);
	for (uint8_t index = 0; index < numThread_; index++)
	{
		v_thread_pool_[index] = std::thread{ THREAD };
		Logger::L().Log (MODULENAME, severity_level::debug1, 
			boost::str(boost::format("Start: thread_owner_name: %s, thread_id: %08X") 
			% threadOwnerName_ 
			% v_thread_pool_[index].get_id()));
		v_thread_ids_.emplace_back(v_thread_pool_[index].get_id());
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "Start: <exit> : " + threadOwnerName_);
}

//*****************************************************************************
void HawkeyeThreadPool::Stop()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <enter> : " + threadOwnerName_);

	// Stop the local queue processing with no state retention.
	if (!plocal_iosvc_ || v_thread_pool_.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <exit, nothing to stop> : " + threadOwnerName_);
		return;
	}

	plocal_work_.reset();	// destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	plocal_iosvc_->stop();	// instruct io_service to stop processing (should exit ::run() and end thread.
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <waiting for threads to join> : " + threadOwnerName_);

	for (auto& p_thread : v_thread_pool_)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("Stop: thread_owner_name: %s, thread_id: %08X")
				% threadOwnerName_
				% p_thread.get_id()));
		if (p_thread.joinable())
		{
			p_thread.join();	// May take a moment to complete.
		}
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <thread stopped> : " + threadOwnerName_);

	plocal_iosvc_->reset(); // ready io_service to resume activity later.
	v_thread_pool_.clear();
	v_thread_ids_.clear();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <exit> : " + threadOwnerName_);
}

//*****************************************************************************
void HawkeyeThreadPool::Pause()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Pause: <enter> : " + threadOwnerName_);

	// Stop the local queue processing with no state retention.
	if (!plocal_iosvc_ || plocal_iosvc_->stopped())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Pause: <exit, already stopped>");
		return;
	}

	plocal_work_.reset();	// destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	plocal_iosvc_->stop();	// instruct io_service to stop processing (should exit ::run() and end thread.

	for (auto& p_thread : v_thread_pool_)
	{
		if (p_thread.joinable())
		{
			p_thread.join();	// May take a moment to complete.
		}
	}
	Logger::L().Log (MODULENAME, severity_level::debug1, "Pause: <exit> : " + threadOwnerName_);
}

//*****************************************************************************
void HawkeyeThreadPool::Resume()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Resume: <enter> : " + threadOwnerName_);

	// Stop the local queue processing with no state retention.
	if (!plocal_iosvc_)
	{
		Start();

		Logger::L().Log (MODULENAME, severity_level::debug1, "Resume: <exit, started new io_service>");
		return;
	}

	Pause();
	plocal_work_.reset(new boost::asio::io_context::work(*plocal_iosvc_));

	// Reset the io_service in preparation for a subsequent run() invocation.
	plocal_iosvc_->reset();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Resume: <exit> : " + threadOwnerName_);
}

//*****************************************************************************
void HawkeyeThreadPool::Enqueue(std::function<void(void)> work)
{
	HAWKEYE_ASSERT (MODULENAME, plocal_iosvc_);
	HAWKEYE_ASSERT (MODULENAME, work);

	plocal_iosvc_->post(work);
}

//***************************************************************************** 
std::shared_ptr<boost::asio::io_context> HawkeyeThreadPool::GetIoService()
{
	return plocal_iosvc_;
}

//*****************************************************************************
bool HawkeyeThreadPool::isRunningOnThisIos() const
{
	auto this_thd_id = std::this_thread::get_id();
	return std::find_if (
		v_thread_ids_.begin(), v_thread_ids_.end(),
		[this_thd_id](const std::thread::id& item) {return item == this_thd_id;}
	)!= v_thread_ids_.end();
}

//*****************************************************************************
std::vector<std::thread::id> HawkeyeThreadPool::getThreadIds() const
{
	return v_thread_ids_;
}

//*****************************************************************************
bool HawkeyeThreadPool::IsActive() const
{
	if (!plocal_iosvc_ || !plocal_work_ || v_thread_pool_.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Thread is not active : " + threadOwnerName_);
		return false;
	}
	return true;
}
