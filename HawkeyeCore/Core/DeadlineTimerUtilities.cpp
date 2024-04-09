#include "stdafx.h"

#include <limits>
#include "DeadlineTimerUtilities.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "DeadlineTimerUtilities";

class TimerHandler
{
public:

	typedef struct TimerHandlerParams
	{
		// The total timer running amount time w.r.t. Supported type.
		// If waitTime is zero, then timer will run forever until stopped.
		std::pair<uint64_t, SupportedTimerCountdown> waitTime;

		// Callback to run when either timer is finished or stopped.
		DeadlineTimerUtilities_callback timerCompleteCallback;

		// The intermediate countdown feedback timer with callback.
		std::pair<uint64_t, timer_countdown_callback> countdown_feedback;

	}TimerHandlerParams;

	TimerHandler(boost::asio::io_context& svc, TimerHandlerParams params)
		: timer_(svc), isRunning_(false), params_(params)
	{ 
		initialize();
	}

	~TimerHandler()
	{
		stop();
	}

	bool start()
	{
		if (isRunning())
		{
			return false;
		}

		setIsRunning(true);
		timer_.expires_from_now(duration_);
		timer_.async_wait(
			std::bind(&TimerHandler::handler, this, std::placeholders::_1));

		return true;
	}

	void stop()
	{
		if (isRunning())
		{
			timer_.cancel();
			setIsRunning(false, false);
		}
	}

	bool isRunning() const { return isRunning_; }

	uint64_t timeRemaining_MilliSec() const
	{
		const uint64_t minValue = 0;
		uint64_t remainingTime = remainingDuration_.is_not_a_date_time() == false ? remainingDuration_.total_milliseconds() : 0;
		return (std::max)(minValue, ((timerCount_ * duration_.total_milliseconds()) + remainingTime));
	}

private:

	void setIsRunning(bool status, bool timercompleted = true)
	{
		if (isRunning() == status)
		{
			return;
		}

		isRunning_ = status;
		
		if (status == false
			&& params_.timerCompleteCallback != nullptr)
		{
			// Timer is stopped or cancelled.
			params_.timerCompleteCallback(timercompleted);
		}
	}

	void handler(const boost::system::error_code &ec)
	{
		timerCount_--;

		// timer was cancelled
		if (ec)
		{
			timerCount_ = 0;
			return;
		}

		// check if any timer count is remaining, otherwise timer is complete
		if (timerCount_ <= 0)
		{
			if (!remainingDuration_.is_not_a_date_time())
			{
				timerCount_++;
				timer_.expires_at(timer_.expires_at() + remainingDuration_);
				timer_.async_wait(boost::bind(&TimerHandler::handler, this, _1));
				remainingDuration_ = boost::posix_time::not_a_date_time;
			}
			else
			{
				// timer is complete now
				setIsRunning(false, true);
			}
			return;
		}

		timer_.expires_at(timer_.expires_at() + duration_);
		timer_.async_wait(boost::bind(&TimerHandler::handler, this, _1));

		if (isRunning() && params_.countdown_feedback.second != nullptr)
		{
			remainingTime_ -= params_.countdown_feedback.first;
			params_.countdown_feedback.second(remainingTime_, params_.waitTime.second);
		}
	}

	void initialize()
	{
		remainingDuration_ = boost::posix_time::not_a_date_time;
		remainingTime_ = 0;
		timerCount_ = 0;
		remainingTime_ = false;

		if (params_.countdown_feedback.first == 0
			|| params_.countdown_feedback.second == nullptr)
		{
			// No timer countdown feedback.
			params_.countdown_feedback.first = 1;
			params_.countdown_feedback.second = nullptr;
		}

		if (params_.waitTime.first == 0)
		{
			// Run timer for maximum time.
			params_.waitTime.first = (std::numeric_limits<unsigned int>::max)();
		}

		if (params_.countdown_feedback.second == nullptr)
		{
			params_.countdown_feedback.first = params_.waitTime.first;
		}

		timerCount_ = params_.waitTime.first / params_.countdown_feedback.first;
		
		// remainder : get the access time to account for total time.
		uint64_t remainder = params_.waitTime.first % params_.countdown_feedback.first;

		switch (params_.waitTime.second)
		{
			case timer_milliseconds:
			{
				duration_ = boost::posix_time::milliseconds(
					params_.countdown_feedback.first);
				if (remainder > 0)
				{
					remainingDuration_ = boost::posix_time::milliseconds(remainder);
				}
				break;
			}
			case timer_seconds:
			{
				duration_ = boost::posix_time::seconds(
					static_cast<long>(params_.countdown_feedback.first));
				if (remainder > 0)
				{
					remainingDuration_ = boost::posix_time::seconds(
						static_cast<long>(remainder));
				}
				break;
			}
			case timer_minutes:
			{
				duration_ = boost::posix_time::minutes(
					static_cast<long>(params_.countdown_feedback.first));
				if (remainder > 0)
				{
					remainingDuration_ = boost::posix_time::minutes(
						static_cast<long>(remainder));
				}
				break;
			}
			default:
				throw std::exception("CountDown Timer not supported!");
		}
		
		if (timerCount_ == 0 && params_.waitTime.first > 0)
		{
			timerCount_ = 1;
			duration_ = boost::posix_time::milliseconds(params_.waitTime.first);
		}

		remainingTime_ = params_.waitTime.first;
	}

	boost::asio::deadline_timer timer_;
	bool isRunning_;
	boost::posix_time::time_duration duration_;
	boost::posix_time::time_duration remainingDuration_;
	uint64_t timerCount_;
	uint64_t remainingTime_;
	TimerHandlerParams params_;
};

/******************************************************************************/

DeadlineTimerUtilities::DeadlineTimerUtilities()
{
	timers_.reset();
	cancelExplicit_ = false;
}

DeadlineTimerUtilities::~DeadlineTimerUtilities()
{
	cancel();
	timers_.reset();
}

bool DeadlineTimerUtilities::wait(
	boost::asio::io_context& io_svc,
	boost::posix_time::time_duration waitTime,
	DeadlineTimerUtilities_callback onComplete,
	boost::posix_time::time_duration feedbackTime,
	timer_countdown_callback cb)
{
	auto waitTimeTicks = waitTime.total_milliseconds();
	auto feedbackTimeTicks = feedbackTime.total_milliseconds();

	if (waitTimeTicks <= 0)
	{
		return false;
	}
	
	if (feedbackTimeTicks > waitTimeTicks
		|| feedbackTimeTicks <= 0)
	{
		return wait(io_svc, waitTimeTicks, onComplete);
	}

	auto tcd = SupportedTimerCountdown::timer_milliseconds;
	
	// convert to seconds if possible
	if ((feedbackTimeTicks % 1000 == 0)
		&& (waitTimeTicks % 1000 == 0))
	{
		feedbackTimeTicks /= 1000;
		waitTimeTicks /= 1000;
		tcd = SupportedTimerCountdown::timer_seconds;
	}

	return wait(
		io_svc, (uint64_t)waitTimeTicks, tcd,
		onComplete, (uint64_t)feedbackTimeTicks, cb);
}

bool DeadlineTimerUtilities::wait(
	boost::asio::io_context&  io_svc,
	uint64_t waitMilisec, DeadlineTimerUtilities_callback onComplete)
{
	return wait(io_svc, waitMilisec, SupportedTimerCountdown::timer_milliseconds,
				onComplete, 0, nullptr);
}

bool DeadlineTimerUtilities::wait(
	boost::asio::io_context&  io_svc,
	uint64_t waitTime, SupportedTimerCountdown tcd,
	DeadlineTimerUtilities_callback onComplete,
	uint64_t feedbackTime, timer_countdown_callback cb)
{
	if (timers_.get() != nullptr && timers_->isRunning())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2,	"*** Timer is currently busy!!! ***");		
		return false;
	}	

	cancelExplicit_ = false;

	TimerHandler::TimerHandlerParams params = {};
	params.waitTime = std::make_pair(waitTime, tcd);
	params.timerCompleteCallback = onComplete;
	params.countdown_feedback = std::make_pair(feedbackTime, cb);

	timers_ = std::make_shared<TimerHandler>(io_svc, params);
	timers_->start();
	
	return true;
}

bool DeadlineTimerUtilities::waitRepeat(
	boost::asio::io_context&  io_svc,
	boost::posix_time::time_duration waitUntilRepeat,
	DeadlineTimerUtilities_callback onComplete,
	std::function<bool()> repeatConditionLamda,
	boost::optional<boost::posix_time::time_duration> maxWaitTime)
{
	if (!maxWaitTime)
	{
		// set max wait time as positive infinity;
		maxWaitTime = boost::posix_time::pos_infin;
	}

	auto tcd = [this, repeatConditionLamda](
		uint64_t rTime, SupportedTimerCountdown tcd) -> void
	{
		if (repeatConditionLamda() == false)
		{
			if (timers_)
			{
				// here timer is not cancelled explicitly, so don't call "Cancel()" method.
				timers_->stop();
			}
		}
	};

	DeadlineTimerUtilities_callback onCompleteInternal = nullptr;
	if (onComplete != nullptr)
	{
		onCompleteInternal = [=](bool status) -> void
		{
			// If status is true that means Timer is successfully executed
			// but the repeat condition is not success.
			if (status)
			{
				bool conditionMet = repeatConditionLamda() == false;
				onComplete(conditionMet);
			}
			else
			{
				// if status is false, that means timer was cancelled.
				
				// Check if timer was cancelled explicitly then that means repeat condition was not met
				// and someone cancelled the timer before expiring.
				// In this case; return false;
				if (cancelExplicit_)
				{
					onComplete(false);
				}
				else
				{
					// Else the repeat condition is met successfully
					// and timer was cancelled.
					// In this case; return true
					onComplete(true);
				}
			}
		};
	}

	return wait(io_svc, maxWaitTime.get(), onCompleteInternal, waitUntilRepeat, tcd);
}

bool DeadlineTimerUtilities::cancel()
{
	if (timers_.get() != nullptr)
	{
		cancelExplicit_ = true;
		timers_->stop();
		return true;
	}
	return false;
}

bool DeadlineTimerUtilities::isRunning()
{
	if (timers_)
	{
		return timers_->isRunning();
	}
	return false;
}

uint64_t DeadlineTimerUtilities::timeRemaining_MilliSec()
{
	if (timers_)
	{
		return timers_->timeRemaining_MilliSec();
	}
	return 0;
}