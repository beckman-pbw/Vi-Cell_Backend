#include "stdafx.h"

#include "Logger.hpp"
#include "Registers.hpp"
#include "VoltageMeasurementBase.hpp"

static const char MODULENAME[] = "VoltageMeasurementBase";

//*****************************************************************************
VoltageMeasurementBase::VoltageMeasurementBase (
	std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(std::move(pCBOService))
{
	
}

//*****************************************************************************
VoltageMeasurementBase::~VoltageMeasurementBase()
{
}

//*****************************************************************************
bool VoltageMeasurementBase::Initialize (void)
{
	return true;
}

