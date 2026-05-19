// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Full CPU monitoring class
//

#pragma once

#ifndef PRMON_MACHMON_H
#define PRMON_MACHMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

class machmon final : public Imonitor, public MessageBase {
 private:
  const prmon::parameter_list params = {
      {"utime_total", "s", "Total user CPU time from /proc/stat"},
      {"stime_total", "s", "Total system CPU time from /proc/stat"},
      {"ntime_total", "s", "Total nice CPU time from /proc/stat"},
  };

  prmon::monitored_list mach_stats;  // era monitored_value_map

  long initial_utime{0L};
  long initial_stime{0L};
  long initial_ntime{0L};
  bool initialised{false};

 public:
  machmon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }
};
REGISTER_MONITOR(Imonitor, machmon, "Monitor full machine cpu time")


#endif  // PRMON_MACHMON_H