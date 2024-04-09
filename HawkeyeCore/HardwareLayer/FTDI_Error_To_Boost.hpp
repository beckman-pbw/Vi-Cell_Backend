#pragma once

#include <ftd2xx.h>

#include "ErrcHelpers.hpp"

boost::system::error_code FTDI_Error_To_Boost(DWORD FTDI);
