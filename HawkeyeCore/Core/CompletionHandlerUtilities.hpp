#pragma once

#include <functional>
#include <future>

#include "HawkeyeError.hpp"

class ICompletionHandler
{
public:
	virtual ~ICompletionHandler() =default;
	virtual void whenComplete(std::function<void(HawkeyeError)> completehandler) = 0;
	virtual void waitTillComplete() = 0;	
};

class CompletionHandler : public ICompletionHandler
{
public:
	virtual ~CompletionHandler() override;
	CompletionHandler(uint64_t index);

	// Inherited via ICompletionHandler
	virtual void whenComplete(
		std::function<void(HawkeyeError)> completehandler) override;
	virtual void waitTillComplete() override;

	void onComplete(HawkeyeError he);
	uint64_t getIndex() const;
	bool isValid() const;
	void keepAlive();

	static CompletionHandler& createInstance();
	static CompletionHandler& getInstance(uint64_t index);
	static bool getHandler(uint64_t index, ICompletionHandler*& handler);		

private:
	std::function<void(HawkeyeError)> handler_;
	std::shared_ptr<std::promise<HawkeyeError>> promise_;
	uint64_t index_;
	bool isValid_;
	bool keepAlive_;
};
