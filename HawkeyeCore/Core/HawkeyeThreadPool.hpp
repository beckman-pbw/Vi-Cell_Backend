#pragma once

#include <memory>
#include <thread>
#include <vector>

#pragma warning(push, 0)
#include "boost/asio.hpp"
#pragma warning(pop)
#include "boost/optional.hpp"

class HawkeyeThreadPool
{

public:
	HawkeyeThreadPool(bool autoStart, uint8_t numThread, std::string threadOwnerName);
	virtual ~HawkeyeThreadPool();

	void Start();
	void Stop();
	void Pause();
	void Resume();

	void Enqueue(std::function<void(void)> work);
	std::shared_ptr<boost::asio::io_context> GetIoService();
	bool isRunningOnThisIos() const;
	std::vector<std::thread::id> getThreadIds() const;
	bool IsActive() const;

private:
	std::shared_ptr<boost::asio::io_context> plocal_iosvc_;
	std::shared_ptr<boost::asio::io_context::work> plocal_work_;
	std::vector<std::thread> v_thread_pool_;
	std::vector<std::thread::id> v_thread_ids_;

	const uint8_t numThread_;
	const std::string threadOwnerName_;
};
