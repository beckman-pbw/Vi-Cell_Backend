// Logger implementation 'auxiliary' constants and functions
// NOT part of the Logger API!

#pragma once

#include "Logger.hpp"

namespace Logger_Impl {
  constexpr const auto STR_TIMESTAMP_START  = "[";
  constexpr const auto STR_TIMESTAMP_END    = "]";
  constexpr const auto STR_THREAD_START     = "[";
  constexpr const auto STR_THREAD_END       = "]";
  constexpr const auto STR_SEVERITY_START   = "[";
  constexpr const auto STR_SEVERITY_END     = "]";
  constexpr const auto STR_COMPONENT_START  = "<";
  // Leave a space before the message itself
  constexpr const auto STR_COMPONENT_END    = "> ";


  inline const char* severity_string(severity_level level)
  {
    switch(level)
    {
      case severity_level::debug3:
        return "DBG3";
      case severity_level::debug2:
        return "DBG2";
      case severity_level::debug1:
        return "DBG1";
      case severity_level::normal:
        return "NORM";
      case severity_level::notification:
        return "NTFY";
      case severity_level::warning:
        return "WARN";
      case severity_level::error:
        return "ERRR";
      case severity_level::critical:
        return "CRIT";
      case severity_level::console:
        return "CONS";
      default:
        break;
    }

	  return "????";
  }

  inline static bool severity_level_from_str(
      severity_level& level, const std::string& lvl_str)
  {
    if (lvl_str == "DBG3")
      level = severity_level::debug3;
    else if (lvl_str == "DBG2")
      level = severity_level::debug2;
    else if (lvl_str == "DBG1")
      level = severity_level::debug1;
    else if (lvl_str == "NORM")
      level = severity_level::normal;
    else if (lvl_str == "NTFY")
      level = severity_level::notification;
    else if (lvl_str == "WARN")
      level = severity_level::warning;
    else if (lvl_str == "ERRR")
      level = severity_level::error;
    else if (lvl_str == "CRIT")
      level = severity_level::critical;
    else if (lvl_str == "CONS")
      level = severity_level::console;
    else
      return false;  // Do not modify `level`

    return true;
  }

}
