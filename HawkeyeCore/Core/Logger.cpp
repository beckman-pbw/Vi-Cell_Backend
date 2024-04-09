#include "stdafx.h"

#include "Logger.hpp"
#include "Logger_Impl.hxx"

// Instantiate exactly one default logger class
template class IndexedLogger<0, true>;
template class IndexedLogger<1, false>;
