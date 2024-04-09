#include "stdafx.h"

#include <map>

#include "CompletionHandlerUtilities.hpp"

static const char MODULENAME[] = "CompletionHandlerUtilities";

static std::map<uint64_t, std::shared_ptr<CompletionHandler>> mCompletionHandler_;
static uint64_t uinqueIndexCounter_ = 0;

static void clearInvalid()
{
	if (mCompletionHandler_.size() == 0)
	{
		return;
	}

	std::vector<uint64_t> indexToRemove;
	for (auto item : mCompletionHandler_)
	{
		if (item.second->isValid() == false)
		{
			item.second.reset();
			indexToRemove.push_back(item.first);
		}		
	}

	for (auto item : indexToRemove)
	{
		mCompletionHandler_.erase(item);
	}
}

static uint64_t getUniqueIndex()
{
	clearInvalid();
	return uinqueIndexCounter_++;
}

/*********************************************************************/
CompletionHandler::CompletionHandler(uint64_t index)
{
	index_ = index;
	promise_.reset();
	handler_ = nullptr;
	isValid_ = true;
	keepAlive_ = false;
}

CompletionHandler::~CompletionHandler()
{	
	promise_.reset();
	handler_ = nullptr;
	isValid_ = false;
	keepAlive_ = false;
}

void CompletionHandler::whenComplete(
	std::function<void(HawkeyeError)> handler)
{
	handler_ = handler;	
	promise_.reset();
}

void CompletionHandler::waitTillComplete()
{
	if (handler_ == nullptr || promise_.get() != nullptr)
	{
		return;
	}

	promise_ = std::make_shared<std::promise<HawkeyeError>>();
	auto future = promise_->get_future();
	future.wait();
}

void CompletionHandler::onComplete(HawkeyeError he)
{
	if (handler_ != nullptr)
	{
		handler_(he);
	}

	if (promise_.get() != nullptr)
	{
		promise_->set_value(he);
		promise_.reset();
	}

	isValid_ = keepAlive_;
}

uint64_t CompletionHandler::getIndex() const
{
	return index_;
}

bool CompletionHandler::isValid() const
{
	return isValid_;
}

void CompletionHandler::keepAlive()
{
	keepAlive_ = true;
}

CompletionHandler& CompletionHandler::createInstance()
{
	auto index = getUniqueIndex();	
	auto instance = std::make_shared<CompletionHandler>(index);
	mCompletionHandler_[index] = instance;

	return *mCompletionHandler_[index];
}

CompletionHandler& CompletionHandler::getInstance(uint64_t index)
{
	return *mCompletionHandler_[index];
}

bool CompletionHandler::getHandler(uint64_t index, ICompletionHandler*& handler)
{
	auto it = mCompletionHandler_.find(index);
	if (it == mCompletionHandler_.end())
	{
		return false;
	}

	handler = it->second.get();
	return true;
}
