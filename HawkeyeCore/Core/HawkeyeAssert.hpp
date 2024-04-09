#pragma once

#define HAWKEYE_ASSERT(mod_name, cond) \
    if (!(cond))\
    { \
        Logger::L().Log (mod_name, severity_level::error, boost::str (boost::format("HAWKEYE_ASSERT FAILED @ %s ( %s )") % __FILE__ % __LINE__)); \
		Logger::L().Flush(); \
        std::terminate(); \
    }
