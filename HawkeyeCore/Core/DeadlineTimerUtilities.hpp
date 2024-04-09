#pragma once

#include <iostream>
#include <functional>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>

 typedef std::function<void(bool)> DeadlineTimerUtilities_callback;

typedef enum SupportedTimerCountdown : uint16_t
{
	timer_milliseconds = 0,
	timer_seconds = 1,
	timer_minutes = 2

}SupportedTimerCountdown;

typedef std::function<void(uint64_t, SupportedTimerCountdown)> timer_countdown_callback;

class TimerHandler;

class DeadlineTimerUtilities
{
public:

	DeadlineTimerUtilities();
	~DeadlineTimerUtilities();

	bool wait(
		boost::asio::io_context& io_svc,
		boost::posix_time::time_duration waitTime,
		DeadlineTimerUtilities_callback onComplete,
		boost::posix_time::time_duration feedbackTime,
		timer_countdown_callback cb);

	bool wait(boost::asio::io_context& io_svc,
		uint64_t waitMilisec, DeadlineTimerUtilities_callback onComplete);
	
	bool waitRepeat(
		boost::asio::io_context& io_svc,
		boost::posix_time::time_duration waitUntilRepeat,
		DeadlineTimerUtilities_callback onComplete,
		std::function<bool()> repeatConditionLamda,
		boost::optional<boost::posix_time::time_duration> maxWaitTime /*If not provided that means forever running or until repeatConditionLamda returns false*/);	

	bool cancel();
	bool isRunning();
	uint64_t timeRemaining_MilliSec();

private:

	bool wait(
		boost::asio::io_context& io_svc,
		uint64_t waitTime, SupportedTimerCountdown tcd,
		DeadlineTimerUtilities_callback onComplete,
		uint64_t feedbackTime, timer_countdown_callback cb);

	std::shared_ptr<TimerHandler> timers_;
	bool cancelExplicit_;
};