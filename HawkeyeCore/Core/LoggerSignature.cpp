#include "stdafx.h"

#include "LoggerSignature.hpp"
#include "LoggerSignature_Impl.hxx"

// Instantiate exactly one default sign logger class for user and work logs
template class LoggerSignature<10>;
template class LoggerSignature<11>;
template class LoggerSignature<12>;
