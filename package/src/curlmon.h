// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Curl to prometheus monitoring class
//

#pragma once

#ifndef PRMON_CURLMON_H
#define PRMON_CURLMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

class curlmon final : public Imonitor, public MessageBase {
 private:
  const prmon::parameter_list params = {
      {"ipmi_dcmi_power_consumption_watts", "W",  "DCMI power consumption"},
      {"ipmi_power_watts", "W",  "IPMI power consumption"},
      {"node_hwmon_power_watt", "W", "HWMON power consumption"},
      {"node_rapl_package_joules_total", "W",  "RAPL power consumption"},
  };

  prmon::monitored_list curl_stats;  // era monitored_value_map

  const std::vector<std::string> metrics_urls{
      "http://localhost:9100/metrics",
      "http://localhost:9290/metrics"
  };
  
  std::string fetch_metrics(const std::string& url) const;
  double parse_metric_line(const std::string& line,
                           const std::string& metric_name) const;
  
 public:
  curlmon();

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
REGISTER_MONITOR(Imonitor, curlmon, "Monitor power consumption - prometheus")


#endif  // PRMON_CURLMON_H