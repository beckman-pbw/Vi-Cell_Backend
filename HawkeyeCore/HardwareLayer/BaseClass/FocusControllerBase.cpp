#include "stdafx.h"

#include "FocusControllerBase.hpp"

static const char MODULENAME[] = "FocusController";

FocusControllerBase::FocusControllerBase(
	std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(std::move(pCBOService))
	, isInitialized_(false)
{

}

FocusControllerBase::~FocusControllerBase()
{
}

void FocusControllerBase::setInitialization(bool status)
{
	if (isInitialized_ != status)
	{
		isInitialized_ = status;
	}
}

bool FocusControllerBase::IsInitialized(void) const
{
	return isInitialized_;
}
