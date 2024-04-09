#pragma once

#include <boost/system/error_code.hpp>


#define MAKE_ERRC(an_error) \
	boost::system::errc::make_error_code(an_error) 

#define MAKE_SUCCESS \
	MAKE_ERRC(boost::system::errc::success)