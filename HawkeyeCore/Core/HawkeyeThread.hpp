#pragma once

#include "HawkeyeThreadPool.hpp"

class HawkeyeThread
{

public:
	HawkeyeThread(std::string threadOwnerName);
	virtual ~HawkeyeThread();

	void Pause() const;
	void Resume() const;

	void Enqueue(std::function<void(void)> work);
	std::shared_ptr<boost::asio::io_context> GetIoService() const;
	bool isRunningOnThisIos() const;
	boost::optional<std::thread::id> HawkeyeThread::getThreadId() const;
	bool IsActive() const;

private:

	std::unique_ptr<HawkeyeThreadPool> pSingleThreadPool_;
};
