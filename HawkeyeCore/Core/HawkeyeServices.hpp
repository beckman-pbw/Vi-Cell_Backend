#pragma once

#include <functional>
#include <memory>

#include "HawkeyeThread.hpp"
#include "Logger.hpp"

class HawkeyeServices
{
public:
	HawkeyeServices (std::shared_ptr<boost::asio::io_context> pExternalIos, const std::string internalThreadName = "HawkeyeServices_Thread")
		: pExternalQueue_(pExternalIos)
	{
		assert(pExternalQueue_.lock());

		internalThread_ = std::make_shared<HawkeyeThread>(internalThreadName);
	}

	virtual ~HawkeyeServices() = default;

	bool InternalActive() const
	{
		if (!internalThread_)
			return false;

		return internalThread_->IsActive();
	}
	
	boost::asio::io_context& getExternalIosRef() const
	{
		const auto ptr = pExternalQueue_.lock();
		assert(ptr);
		return *(ptr);
	}

	std::shared_ptr<boost::asio::io_context> getMainIos() const
	{
		const auto ptr = pExternalQueue_.lock();
		assert(ptr);
		return ptr;
	}

	//TODO: Needs to be worked OUT of the code- shouldn't have introspection into my internal guts!
	boost::asio::io_context& getInternalIosRef() const
	{
		assert(internalThread_);
		return *(internalThread_->GetIoService());
	}

	std::shared_ptr<boost::asio::io_context> getInternalIos() const
	{
		assert(internalThread_);
		Logger::L ().Log ("HawkeyeServices", severity_level::debug2,
			boost::str (boost::format ("getInternalIos: thread_id: %08X") % internalThread_->getThreadId().get()));

		return internalThread_->GetIoService();
	}

	void enqueueInternal(std::function<void()> callback) const
	{
		assert(internalThread_);
		internalThread_->Enqueue(callback);
	}

	template<typename ... Args>
	void enqueueInternal(std::function<void(Args...)> callback, Args... args) const
	{
		assert(internalThread_);
		assert(callback);
		
		internalThread_->Enqueue([=]() -> void
		{
//NOTE: save for timing testing...
			//Logger::L ().Log (MODULENAME, severity_level::debug1, "enqueueInternal: <enter>");
			callback(args...);
			//Logger::L ().Log (MODULENAME, severity_level::debug1, "enqueueInternal: <exit>");
		});
	}

	void enqueueExternal (std::function<void()> callback) const
	{
		auto io_svc = pExternalQueue_.lock();
		assert(io_svc);
		assert(callback);

		// this workflow does not require the user id, so do not use the transient user technique
		io_svc->post(callback);
	}

	template<typename ... Args>
	void enqueueExternal (std::function<void(Args...)> callback, Args... args) const
	{
		auto io_svc = pExternalQueue_.lock();
		assert(io_svc);
		assert(callback);

		// this workflow does not require the user id, so do not use the transient user technique
		io_svc->post([=]() -> void
		{
//NOTE: save for timing testing...
			//Logger::L ().Log (MODULENAME, severity_level::debug1, "enqueueExternal: <enter>");
			callback(args...);
			//Logger::L ().Log (MODULENAME, severity_level::debug1, "enqueueExternal: <exit>");
			});
	}

	void StopServices() const
	{
		internalThread_->Pause();
	}
	
	void StartServices() const
	{
		internalThread_->Resume();
	}

	HawkeyeServices get() {
		return *this;
	}

protected:
	std::shared_ptr<HawkeyeThread> internalThread_;
	std::weak_ptr<boost::asio::io_context> pExternalQueue_;
};
