// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "curlmon.h"

#include <curl/curl.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils.h"

#define MONITOR_NAME "curlmon"

// libcurl write, appending data to a std::string
static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb,
                             void* userdata) {
  auto* buf = reinterpret_cast<std::string*>(userdata);
  buf->append(ptr, size * nmemb);
  return size * nmemb;
}


// Constructor; uses RAII pattern to be valid after construction
curlmon::curlmon() : curl_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    curl_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
  }
}

// Fetch Prometheus text from the ctt URL
std::string curlmon::fetch_metrics(const std::string& url) const {
  std::string response{};
  CURL* curl = curl_easy_init();
  if (!curl) throw std::runtime_error("Failed to initialise libcurl");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
    throw std::runtime_error(std::string("curl request failed: ") +
                             curl_easy_strerror(res));
  return response;
}

// Parse a single Prometheus line: "metric_name{labels} value timestamp"
// Returns the value if the line starts with metric_name, otherwise -1.0
double curlmon::parse_metric_line(const std::string& line,
                                  const std::string& metric_name) const {
  if (line.empty() || line[0] == '#') return -1.0;

  if (line.rfind(metric_name, 0) != 0) return -1.0;

  char next = line[metric_name.size()];
  if (next != '{' && next != ' ' && next != '\t') return -1.0;

  size_t value_start = line.find('}');
  if (value_start == std::string::npos)
    value_start = metric_name.size();
  else
    value_start += 1;

  while (value_start < line.size() && std::isspace(line[value_start]))
    ++value_start;

  try {
    return std::stod(line.substr(value_start));
  } catch (...) {
    return -1.0;
  }
}

void curlmon::update_stats(const std::vector<pid_t>& /*pids*/,
                              const std::string read_path) {
    std::unordered_map<std::string, double> accumulated{};
  for (const auto& param : params) accumulated[param.get_name()] = 0.0;

  for (const auto& url : metrics_urls) {
    try {
      const std::string body = fetch_metrics(url);
      std::istringstream stream{body};
      std::string line{};

      while (std::getline(stream, line)) {
        for (const auto& param : params) {
          double val = parse_metric_line(line, param.get_name());
          if (val >= 0.0) accumulated[param.get_name()] += val;
        }
      }
    } catch (const std::exception& e) {
      error(std::string("Error fetching metrics from ") + url + ": " + e.what());
    }
  }

  for (const auto& param : params) {
    auto val = static_cast<prmon::mon_value>(accumulated[param.get_name()] * 1000.0);
    curl_stats.at(param.get_name()).set_value(val);
  }
}


// Return the monitored values
prmon::monitored_value_map const curlmon::get_text_stats() {
  prmon::monitored_value_map stat_map{};
  for (const auto& value : curl_stats) {
    stat_map[value.first] = value.second.get_value();
  }
  return stat_map;
}

prmon::monitored_value_map const curlmon::get_json_total_stats() {
  return curlmon::get_text_stats();
}

prmon::monitored_average_map const curlmon::get_json_average_stats(
    unsigned long long /*elapsed_clock_ticks*/) {
  static const prmon::monitored_average_map empty{};
  return empty;
}

prmon::parameter_list const curlmon::get_parameter_list() { return params; }

void const curlmon::get_hardware_info(nlohmann::json& /*hw_json*/) { return; }

void const curlmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}