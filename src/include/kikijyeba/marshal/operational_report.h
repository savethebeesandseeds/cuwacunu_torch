// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "kikijyeba/marshal/digest.h"
#include "kikijyeba/marshal/runtime_hero_handoff.h"
#include "kikijyeba/runtime/job_layout.h"

namespace cuwacunu::kikijyeba::marshal {

inline constexpr const char *k_marshal_operational_report_schema_v1 =
    "kikijyeba.marshal.operational_report.v1";

struct marshal_lattice_target_status_t {
  std::string target_id{};
  std::string status{"unavailable"};
  bool proof_certificate_check_passed{false};
  std::vector<std::string> proof_certificate_issues{};
  std::vector<std::string> deficit_keys{};
  std::vector<std::string> warning_ids{};
};

struct marshal_operational_report_options_t {
  std::filesystem::path runtime_root{"/cuwacunu/.runtime/cuwacunu_exec"};
  std::filesystem::path config_path{"/cuwacunu/src/config/.config"};
  std::vector<std::string> job_ids{};
  std::vector<std::string> target_ids{};
  std::vector<marshal_lattice_target_status_t> target_statuses{};
  bool include_machine_payload{false};
};

namespace operational_report_detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] inline std::string strip_trailing_semicolon(std::string value) {
  value = trim_ascii(value);
  if (!value.empty() && value.back() == ';') {
    value.pop_back();
  }
  return trim_ascii(value);
}

[[nodiscard]] inline std::map<std::string, std::string>
read_kv_file(const std::filesystem::path &path) {
  std::map<std::string, std::string> out;
  std::ifstream in(path);
  if (!in.is_open()) {
    return out;
  }
  std::string line;
  while (std::getline(in, line)) {
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto key = trim_ascii(std::string_view(line).substr(0, eq));
    const auto value = trim_ascii(std::string_view(line).substr(eq + 1U));
    if (!key.empty()) {
      out[key] = value;
    }
  }
  return out;
}

[[nodiscard]] inline std::string
get(const std::map<std::string, std::string> &m, const std::string &key) {
  const auto it = m.find(key);
  return it == m.end() ? std::string{} : it->second;
}

[[nodiscard]] inline bool bool_value(const std::string &value) {
  const auto text = trim_ascii(value);
  if (text == "true" || text == "1") {
    return true;
  }
  if (text == "false" || text == "0" || text.empty()) {
    return false;
  }
  char *end = nullptr;
  const double parsed = std::strtod(text.c_str(), &end);
  if (end != text.c_str() && *end == '\0' && std::isfinite(parsed)) {
    return parsed != 0.0;
  }
  return false;
}

[[nodiscard]] inline bool json_number_is_valid(const std::string &value) {
  const auto text = trim_ascii(value);
  if (text.empty() || text == "nan" || text == "NaN" || text == "inf" ||
      text == "-inf" || text == "Infinity" || text == "-Infinity") {
    return false;
  }
  char *end = nullptr;
  const double parsed = std::strtod(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(parsed);
}

[[nodiscard]] inline std::string json_number_or_null(const std::string &value) {
  const auto text = trim_ascii(value);
  return json_number_is_valid(text) ? text : "null";
}

[[nodiscard]] inline std::string json_bool(const std::string &value) {
  return bool_value(value) ? "true" : "false";
}

[[nodiscard]] inline std::string json_bool_or_null(const std::string &value) {
  return trim_ascii(value).empty() ? "null" : json_bool(value);
}

[[nodiscard]] inline std::vector<std::string> split_csv(std::string_view text) {
  std::vector<std::string> out;
  std::size_t begin = 0;
  while (begin <= text.size()) {
    const auto comma = text.find(',', begin);
    const auto end = comma == std::string_view::npos ? text.size() : comma;
    out.push_back(trim_ascii(text.substr(begin, end - begin)));
    if (comma == std::string_view::npos) {
      break;
    }
    begin = comma + 1U;
  }
  if (out.size() == 1U && out.front().empty()) {
    out.clear();
  }
  return out;
}

[[nodiscard]] inline std::string
json_number_array_or_string_array(const std::string &csv) {
  const auto values = split_csv(csv);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    if (json_number_is_valid(values[i])) {
      out << values[i];
    } else {
      out << detail::json_quote(values[i]);
    }
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string
json_string_array(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << detail::json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string current_utc_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&now_time, &tm);
  char buffer[32]{};
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buffer);
}

[[nodiscard]] inline std::optional<std::filesystem::path>
config_pointer(const std::filesystem::path &config_path,
               const std::string &key) {
  std::ifstream in(config_path);
  if (!in.is_open()) {
    return std::nullopt;
  }
  std::string line;
  while (std::getline(in, line)) {
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto parsed_key = trim_ascii(std::string_view(line).substr(0, eq));
    if (parsed_key != key) {
      continue;
    }
    const auto value = strip_trailing_semicolon(
        std::string(std::string_view(line).substr(eq + 1U)));
    if (value.empty()) {
      return std::nullopt;
    }
    return std::filesystem::path(value);
  }
  return std::nullopt;
}

[[nodiscard]] inline std::string
dsl_assignment_value(const std::filesystem::path &path,
                     const std::string &key) {
  std::ifstream in(path);
  if (!in.is_open()) {
    return {};
  }
  std::string line;
  while (std::getline(in, line)) {
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto parsed_key = trim_ascii(std::string_view(line).substr(0, eq));
    if (parsed_key != key) {
      continue;
    }
    return strip_trailing_semicolon(
        std::string(std::string_view(line).substr(eq + 1U)));
  }
  return {};
}

struct job_summary_t {
  std::filesystem::path job_dir{};
  std::map<std::string, std::string> manifest{};
  std::map<std::string, std::string> state{};
  std::map<std::string, std::string> report{};
  std::map<std::string, std::string> result_fact{};
  std::map<std::string, std::string> checkpoint_io_fact{};
  std::map<std::string, std::string> health_fact{};
  std::string role{"other"};
  std::string grad_clip_norm{};
};

[[nodiscard]] inline std::string
role_for(const std::map<std::string, std::string> &state) {
  const auto kind = get(state, "job_kind");
  const auto action = get(state, "wave_action");
  if (kind == "channel_representation_vicreg" && action == "train") {
    return "vicreg_train";
  }
  if (kind == "channel_inference_mdn" && action == "train") {
    return "mdn_train";
  }
  if (kind == "channel_inference_mdn" && action == "run") {
    return "mdn_validation_eval";
  }
  return "other";
}

[[nodiscard]] inline std::string
report_path_for(const std::filesystem::path &job_dir,
                const std::map<std::string, std::string> &state) {
  const auto from_state = get(state, "report_path");
  if (!from_state.empty()) {
    return from_state;
  }
  const auto kind = get(state, "job_kind");
  if (kind == "channel_representation_vicreg") {
    return (job_dir / "channel_representation.report").string();
  }
  if (kind == "channel_inference_mdn") {
    return (job_dir / "channel_inference.report").string();
  }
  return {};
}

[[nodiscard]] inline job_summary_t
read_job(const std::filesystem::path &job_dir,
         const std::filesystem::path &config_path) {
  job_summary_t out{};
  out.job_dir = job_dir;
  out.manifest = read_kv_file(job_dir / "job.manifest");
  out.state = read_kv_file(job_dir / "job.state");
  out.role = role_for(out.state);
  const auto report_path = report_path_for(job_dir, out.state);
  if (!report_path.empty()) {
    out.report = read_kv_file(report_path);
  }
  out.result_fact = read_kv_file(job_dir / "runtime.result.fact");
  out.checkpoint_io_fact = read_kv_file(job_dir / "runtime.checkpoint_io.fact");
  out.health_fact = read_kv_file(job_dir / "runtime.health_measurement.fact");

  if (out.role == "mdn_train" || out.role == "mdn_validation_eval") {
    const auto job_config = get(out.report, "config_path");
    std::filesystem::path config_to_read =
        job_config.empty() ? config_path : std::filesystem::path(job_config);
    auto jkimyei_path = config_pointer(
        config_to_read, "wikimyei_inference_expected_value_mdn_jkimyei_path");
    if (!jkimyei_path.has_value()) {
      jkimyei_path = config_pointer(
          config_path, "wikimyei_inference_expected_value_mdn_jkimyei_path");
    }
    if (jkimyei_path.has_value()) {
      out.grad_clip_norm =
          dsl_assignment_value(*jkimyei_path, "GRAD_CLIP_NORM");
    }
  }
  return out;
}

[[nodiscard]] inline std::string
first_non_empty(const std::vector<std::string> &values) {
  for (const auto &value : values) {
    if (!value.empty()) {
      return value;
    }
  }
  return {};
}

[[nodiscard]] inline bool has_terminal_facts(const job_summary_t &job) {
  return !job.result_fact.empty() || !job.checkpoint_io_fact.empty() ||
         !job.health_fact.empty();
}

[[nodiscard]] inline std::string
result_or_report(const job_summary_t &job, const std::string &result_key,
                 const std::string &report_key) {
  return first_non_empty(
      {get(job.result_fact, result_key), get(job.report, report_key)});
}

[[nodiscard]] inline std::string
health_or_report(const job_summary_t &job, const std::string &health_key,
                 const std::string &report_key) {
  return first_non_empty({get(job.health_fact, health_key),
                          get(job.result_fact, health_key),
                          get(job.report, report_key)});
}

[[nodiscard]] inline std::string
job_value(const job_summary_t &job,
          std::initializer_list<std::string_view> keys) {
  for (const auto key : keys) {
    const std::string key_text(key);
    for (const auto *source :
         {&job.state, &job.manifest, &job.checkpoint_io_fact, &job.result_fact,
          &job.health_fact, &job.report}) {
      const auto value = get(*source, key_text);
      if (!value.empty()) {
        return value;
      }
    }
  }
  return {};
}

[[nodiscard]] inline bool job_completed(const job_summary_t &job) {
  return get(job.state, "status") == "completed";
}

[[nodiscard]] inline bool
optimizer_steps_mutated_model_state(const std::string &optimizer_steps) {
  if (!json_number_is_valid(optimizer_steps)) {
    return false;
  }
  return std::strtoll(optimizer_steps.c_str(), nullptr, 10) > 0;
}

[[nodiscard]] inline bool job_mutated_model_state(const job_summary_t &job) {
  return bool_value(first_non_empty(
             {get(job.result_fact, "model_state_mutated"),
              get(job.checkpoint_io_fact, "model_state_mutated")})) ||
         bool_value(
             first_non_empty({get(job.result_fact, "checkpoint_written"),
                              get(job.checkpoint_io_fact, "checkpoint_written"),
                              get(job.state, "checkpoint_written")})) ||
         optimizer_steps_mutated_model_state(
             first_non_empty({get(job.result_fact, "optimizer_steps"),
                              get(job.state, "optimizer_steps")}));
}

[[nodiscard]] inline std::vector<job_summary_t>
discover_jobs(const marshal_operational_report_options_t &options) {
  std::vector<job_summary_t> jobs;
  if (!options.job_ids.empty()) {
    for (const auto &job_id : options.job_ids) {
      const auto explicit_path = std::filesystem::path(job_id);
      std::filesystem::path job_dir;
      if (explicit_path.is_absolute()) {
        job_dir = explicit_path;
      } else {
        const auto found = cuwacunu::kikijyeba::runtime::job_layout::
            find_runtime_job_dir_by_id(options.runtime_root, job_id);
        if (!found.has_value()) {
          continue;
        }
        job_dir = *found;
      }
      if (std::filesystem::exists(job_dir / "job.state")) {
        jobs.push_back(read_job(job_dir, options.config_path));
      }
    }
    return jobs;
  }

  if (!std::filesystem::exists(options.runtime_root)) {
    return jobs;
  }
  std::map<std::string, job_summary_t> latest_by_role;
  std::vector<job_summary_t> other_jobs;
  for (const auto &entry :
       cuwacunu::kikijyeba::runtime::job_layout::discover_runtime_job_dirs(
           options.runtime_root)) {
    if (!std::filesystem::exists(entry.state_path)) {
      continue;
    }
    auto job = read_job(entry.dir, options.config_path);
    if (job.role == "vicreg_train" || job.role == "mdn_train" ||
        job.role == "mdn_validation_eval") {
      if (latest_by_role.find(job.role) == latest_by_role.end()) {
        latest_by_role[job.role] = std::move(job);
      }
    } else {
      other_jobs.push_back(std::move(job));
    }
  }
  for (const auto &role :
       {"vicreg_train", "mdn_train", "mdn_validation_eval"}) {
    const auto found = latest_by_role.find(role);
    if (found != latest_by_role.end()) {
      jobs.push_back(found->second);
    }
  }
  jobs.insert(jobs.end(), other_jobs.begin(), other_jobs.end());
  return jobs;
}

[[nodiscard]] inline const job_summary_t *
find_role(const std::vector<job_summary_t> &jobs, std::string_view role) {
  for (const auto &job : jobs) {
    if (job.role == role) {
      return &job;
    }
  }
  return nullptr;
}

inline void append_metric_field(std::ostringstream &out, bool *first,
                                const std::string &key,
                                const std::string &value) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << detail::json_quote(key) << ":" << json_number_or_null(value);
}

inline void append_bool_field(std::ostringstream &out, bool *first,
                              const std::string &key,
                              const std::string &value) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << detail::json_quote(key) << ":" << json_bool(value);
}

inline void append_bool_field_if_present(std::ostringstream &out, bool *first,
                                         const std::string &key,
                                         const std::string &value) {
  if (value.empty()) {
    return;
  }
  append_bool_field(out, first, key, value);
}

inline void append_string_field(std::ostringstream &out, bool *first,
                                const std::string &key,
                                const std::string &value) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << detail::json_quote(key) << ":" << detail::json_quote(value);
}

[[nodiscard]] inline std::string job_json(const job_summary_t &job) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  append_string_field(out, &first, "role", job.role);
  append_string_field(out, &first, "job_id", get(job.state, "job_id"));
  append_string_field(out, &first, "job_dir", job.job_dir.string());
  append_string_field(out, &first, "status", get(job.state, "status"));
  append_string_field(out, &first, "job_kind", get(job.state, "job_kind"));
  append_string_field(out, &first, "component",
                      get(job.state, "target_component_family_id"));
  append_string_field(out, &first, "wave_action",
                      get(job.state, "wave_action"));
  append_string_field(out, &first, "wave_id", get(job.state, "wave_id"));
  append_metric_field(out, &first, "anchor_index_begin",
                      get(job.state, "resolved_anchor_index_begin"));
  append_metric_field(out, &first, "anchor_index_end",
                      get(job.state, "resolved_anchor_index_end"));
  append_metric_field(out, &first, "optimizer_steps",
                      get(job.state, "optimizer_steps"));
  append_bool_field(out, &first, "checkpoint_written",
                    get(job.state, "checkpoint_written"));
  append_string_field(out, &first, "checkpoint_path",
                      get(job.state, "checkpoint_path"));
  append_string_field(out, &first, "report_path",
                      get(job.state, "report_path"));
  append_bool_field(out, &first, "terminal_facts_available",
                    has_terminal_facts(job) ? "true" : "false");
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string compact_job_json(const job_summary_t &job) {
  std::ostringstream out;
  out << "{\"role\":" << detail::json_quote(job.role)
      << ",\"job_id\":" << detail::json_quote(get(job.state, "job_id"))
      << ",\"status\":" << detail::json_quote(get(job.state, "status"))
      << ",\"component\":"
      << detail::json_quote(get(job.state, "target_component_family_id"))
      << ",\"wave_action\":"
      << detail::json_quote(get(job.state, "wave_action"))
      << ",\"anchor_index_begin\":"
      << json_number_or_null(get(job.state, "resolved_anchor_index_begin"))
      << ",\"anchor_index_end\":"
      << json_number_or_null(get(job.state, "resolved_anchor_index_end"))
      << ",\"optimizer_steps\":"
      << json_number_or_null(get(job.state, "optimizer_steps"))
      << ",\"checkpoint_written\":"
      << json_bool(
             first_non_empty({get(job.result_fact, "checkpoint_written"),
                              get(job.checkpoint_io_fact, "checkpoint_written"),
                              get(job.state, "checkpoint_written")}))
      << ",\"model_state_mutated\":"
      << (job_mutated_model_state(job) ? "true" : "false")
      << ",\"terminal_facts_available\":"
      << (has_terminal_facts(job) ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] inline std::string
job_chain_json(const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << compact_job_json(jobs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string
chain_summary_job_json(const job_summary_t &job) {
  const auto checkpoint_written =
      first_non_empty({get(job.checkpoint_io_fact, "checkpoint_written"),
                       get(job.result_fact, "checkpoint_written"),
                       get(job.state, "checkpoint_written")});
  const auto checkpoint_path =
      first_non_empty({get(job.checkpoint_io_fact, "checkpoint_path"),
                       get(job.state, "checkpoint_path")});
  const auto representation_checkpoint = first_non_empty(
      {get(job.checkpoint_io_fact, "representation_checkpoint_path"),
       get(job.report, "representation_checkpoint_path"),
       get(job.manifest, "input_representation_checkpoint_path")});
  const auto mdn_checkpoint =
      first_non_empty({get(job.checkpoint_io_fact, "mdn_checkpoint_path"),
                       get(job.report, "mdn_checkpoint_path"),
                       get(job.manifest, "input_mdn_checkpoint_path")});
  const auto handoff_id = job_value(
      job, {"runtime_handoff_id", "handoff_id", "marshal_handoff_id"});
  const auto handoff_digest = job_value(
      job, {"runtime_handoff_digest", "handoff_digest",
            "runtime_handoff_arguments_digest", "marshal_handoff_digest"});

  std::ostringstream out;
  out << "{\"role\":" << detail::json_quote(job.role)
      << ",\"job_id\":" << detail::json_quote(job_value(job, {"job_id"}))
      << ",\"status\":" << detail::json_quote(job_value(job, {"status"}))
      << ",\"component\":{\"family_id\":"
      << detail::json_quote(
             first_non_empty({job_value(job, {"component_family_id"}),
                              job_value(job, {"target_component_family_id"})}))
      << ",\"target_family_id\":"
      << detail::json_quote(job_value(job, {"target_component_family_id"}))
      << ",\"spawn_registry_id\":"
      << detail::json_quote(job_value(job, {"component_spawn_registry_id"}))
      << ",\"spawn_id\":"
      << detail::json_quote(job_value(job, {"component_spawn_id"}))
      << ",\"spawn_label\":"
      << detail::json_quote(job_value(job, {"component_spawn_label"}))
      << ",\"spawn_fingerprint\":"
      << detail::json_quote(job_value(job, {"component_spawn_fingerprint"}))
      << ",\"spawn_available\":"
      << (!job_value(job, {"component_spawn_fingerprint"}).empty() ? "true"
                                                                   : "false")
      << "}"
      << ",\"wave\":{\"wave_id\":"
      << detail::json_quote(job_value(job, {"wave_id"}))
      << ",\"action\":" << detail::json_quote(job_value(job, {"wave_action"}))
      << ",\"mode\":" << detail::json_quote(job_value(job, {"wave_mode"}))
      << ",\"execution_chain\":"
      << detail::json_quote(job_value(job, {"execution_chain"})) << "}"
      << ",\"source_range\":{\"policy\":"
      << detail::json_quote(
             job_value(job, {"source_range_policy", "wave_source_range_policy",
                             "source_range"}))
      << ",\"source_order\":"
      << detail::json_quote(
             job_value(job, {"source_order_policy", "wave_source_order_policy",
                             "source_order"}))
      << ",\"anchor_index_begin\":"
      << json_number_or_null(job_value(job, {"resolved_anchor_index_begin"}))
      << ",\"anchor_index_end\":"
      << json_number_or_null(job_value(job, {"resolved_anchor_index_end"}))
      << ",\"accepted_anchor_count\":"
      << json_number_or_null(job_value(job, {"accepted_anchor_count"}))
      << ",\"requested_source_key_begin\":"
      << detail::json_quote(job_value(job, {"requested_source_key_begin"}))
      << ",\"requested_source_key_end\":"
      << detail::json_quote(job_value(job, {"requested_source_key_end"})) << "}"
      << ",\"checkpoint_io\":{\"checkpoint_written\":"
      << json_bool(checkpoint_written)
      << ",\"checkpoint_path\":" << detail::json_quote(checkpoint_path)
      << ",\"representation_checkpoint_loaded\":"
      << json_bool_or_null(first_non_empty(
             {get(job.checkpoint_io_fact, "representation_checkpoint_loaded"),
              get(job.report, "representation_checkpoint_loaded")}))
      << ",\"representation_checkpoint_path\":"
      << detail::json_quote(representation_checkpoint)
      << ",\"mdn_checkpoint_loaded\":"
      << json_bool_or_null(first_non_empty(
             {get(job.checkpoint_io_fact, "mdn_checkpoint_loaded"),
              get(job.report, "mdn_checkpoint_loaded")}))
      << ",\"mdn_checkpoint_path\":" << detail::json_quote(mdn_checkpoint)
      << "}"
      << ",\"handoff_identity\":{\"available\":"
      << (!handoff_id.empty() || !handoff_digest.empty() ? "true" : "false")
      << ",\"handoff_id\":" << detail::json_quote(handoff_id)
      << ",\"handoff_digest\":" << detail::json_quote(handoff_digest) << "}"
      << ",\"terminal_facts\":{\"job_state\":"
      << (!job.state.empty() ? "true" : "false")
      << ",\"job_manifest\":" << (!job.manifest.empty() ? "true" : "false")
      << ",\"runtime_result_fact\":"
      << (!job.result_fact.empty() ? "true" : "false")
      << ",\"runtime_checkpoint_io_fact\":"
      << (!job.checkpoint_io_fact.empty() ? "true" : "false")
      << ",\"runtime_health_measurement_fact\":"
      << (!job.health_fact.empty() ? "true" : "false") << "}}";
  return out.str();
}

[[nodiscard]] inline std::string
chain_summary_json(const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << chain_summary_job_json(jobs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string
checkpoint_rows_json(const std::vector<job_summary_t> &jobs, bool produced) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto &job : jobs) {
    const auto checkpoint_written =
        first_non_empty({get(job.checkpoint_io_fact, "checkpoint_written"),
                         get(job.state, "checkpoint_written")});
    const auto checkpoint_path =
        first_non_empty({get(job.checkpoint_io_fact, "checkpoint_path"),
                         get(job.state, "checkpoint_path")});
    if (produced && !bool_value(checkpoint_written)) {
      continue;
    }
    if (!produced && job.role != "mdn_train" &&
        job.role != "mdn_validation_eval") {
      continue;
    }
    if (produced) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "{\"producer_job_id\":"
          << detail::json_quote(get(job.state, "job_id")) << ",\"component\":"
          << detail::json_quote(get(job.state, "target_component_family_id"))
          << ",\"checkpoint_path\":" << detail::json_quote(checkpoint_path)
          << "}";
      continue;
    }
    const auto rep = first_non_empty(
        {get(job.checkpoint_io_fact, "representation_checkpoint_path"),
         get(job.report, "representation_checkpoint_path")});
    const auto mdn =
        first_non_empty({get(job.checkpoint_io_fact, "mdn_checkpoint_path"),
                         get(job.report, "mdn_checkpoint_path")});
    if (!rep.empty()) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "{\"consumer_job_id\":"
          << detail::json_quote(get(job.state, "job_id"))
          << ",\"role\":\"representation_checkpoint\",\"loaded\":"
          << json_bool(first_non_empty(
                 {get(job.checkpoint_io_fact,
                      "representation_checkpoint_loaded"),
                  get(job.report, "representation_checkpoint_loaded")}))
          << ",\"checkpoint_path\":" << detail::json_quote(rep) << "}";
    }
    if (!mdn.empty()) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "{\"consumer_job_id\":"
          << detail::json_quote(get(job.state, "job_id"))
          << ",\"role\":\"mdn_checkpoint\",\"loaded\":"
          << json_bool(first_non_empty(
                 {get(job.checkpoint_io_fact, "mdn_checkpoint_loaded"),
                  get(job.report, "mdn_checkpoint_loaded")}))
          << ",\"checkpoint_path\":" << detail::json_quote(mdn) << "}";
    }
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string
evidence_scope_json(const marshal_operational_report_options_t &options,
                    const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "{\"runtime_root\":"
      << detail::json_quote(options.runtime_root.string())
      << ",\"config_path\":" << detail::json_quote(options.config_path.string())
      << ",\"job_count\":" << jobs.size() << ",\"job_ids\":[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << detail::json_quote(get(jobs[i].state, "job_id"));
  }
  out << "],\"lattice_targets\":" << json_string_array(options.target_ids)
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string metric_json_for_job(const job_summary_t &job) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  append_metric_field(out, &first, "optimizer_steps",
                      first_non_empty({get(job.result_fact, "optimizer_steps"),
                                       get(job.report, "optimizer_steps"),
                                       get(job.state, "optimizer_steps")}));
  append_metric_field(out, &first, "last_loss",
                      first_non_empty({get(job.result_fact, "final_loss"),
                                       get(job.report, "last_loss"),
                                       get(job.state, "last_loss")}));
  append_metric_field(out, &first, "mean_loss",
                      result_or_report(job, "mean_loss", "mean_loss"));
  append_metric_field(
      out, &first, "mean_valid_target_fraction",
      first_non_empty({get(job.result_fact, "valid_target_fraction"),
                       get(job.report, "mean_valid_target_fraction")}));
  append_metric_field(out, &first, "last_valid_target_count",
                      result_or_report(job, "last_valid_target_count",
                                       "last_valid_target_count"));
  append_metric_field(out, &first, "total_valid_target_count",
                      result_or_report(job, "total_valid_target_count",
                                       "total_valid_target_count"));
  append_metric_field(out, &first, "sigma_min",
                      result_or_report(job, "sigma_min", "min_sigma_min"));
  append_metric_field(out, &first, "mean_sigma_mean",
                      result_or_report(job, "sigma_mean", "mean_sigma_mean"));
  append_metric_field(out, &first, "sigma_max",
                      result_or_report(job, "sigma_max", "max_sigma_max"));
  append_metric_field(
      out, &first, "sigma_mean_valid",
      result_or_report(job, "sigma_mean_valid", "mean_sigma_mean_valid"));
  append_metric_field(
      out, &first, "mean_mixture_entropy",
      result_or_report(job, "mixture_entropy", "mean_mixture_entropy"));
  const auto mixture_usage =
      result_or_report(job, "mixture_usage", "mean_mixture_usage");
  if (!mixture_usage.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"mean_mixture_usage\":"
        << json_number_array_or_string_array(mixture_usage);
  }
  const auto nll_per_channel =
      result_or_report(job, "mean_nll_per_channel", "mean_nll_per_channel");
  if (!nll_per_channel.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"mean_nll_per_channel\":"
        << json_number_array_or_string_array(nll_per_channel);
  }
  const auto nll_per_target_feature = result_or_report(
      job, "mean_nll_per_target_feature", "mean_nll_per_target_feature");
  if (!nll_per_target_feature.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"mean_nll_per_target_feature\":"
        << json_number_array_or_string_array(nll_per_target_feature);
  }
  const auto nll_per_channel_target_feature =
      result_or_report(job, "mean_nll_per_channel_target_feature",
                       "mean_nll_per_channel_target_feature");
  if (!nll_per_channel_target_feature.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"mean_nll_per_channel_target_feature\":"
        << json_number_array_or_string_array(nll_per_channel_target_feature);
  }
  const auto valid_per_channel = result_or_report(
      job, "valid_target_count_per_channel", "valid_target_count_per_channel");
  if (!valid_per_channel.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"valid_target_count_per_channel\":"
        << json_number_array_or_string_array(valid_per_channel);
  }
  const auto valid_per_target_feature =
      result_or_report(job, "valid_target_count_per_target_feature",
                       "valid_target_count_per_target_feature");
  if (!valid_per_target_feature.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"valid_target_count_per_target_feature\":"
        << json_number_array_or_string_array(valid_per_target_feature);
  }
  const auto valid_per_channel_target_feature =
      result_or_report(job, "valid_target_count_per_channel_target_feature",
                       "valid_target_count_per_channel_target_feature");
  if (!valid_per_channel_target_feature.empty()) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"valid_target_count_per_channel_target_feature\":"
        << json_number_array_or_string_array(valid_per_channel_target_feature);
  }
  append_metric_field(
      out, &first, "nonfinite_output_count",
      first_non_empty({get(job.health_fact, "nonfinite_output_count"),
                       get(job.result_fact, "nonfinite_output_count"),
                       get(job.report, "nonfinite_output_count")}));
  append_metric_field(
      out, &first, "representation_effective_rank_fraction",
      health_or_report(job, "representation_effective_rank_fraction",
                       "representation_effective_rank_fraction"));
  append_metric_field(
      out, &first, "representation_min_dimension_variance",
      health_or_report(job, "representation_min_dimension_variance",
                       "representation_min_dimension_variance"));
  append_metric_field(
      out, &first, "max_grad_norm",
      health_or_report(job, "grad_norm_max_pre_clip", "max_grad_norm"));
  append_metric_field(out, &first, "grad_clip_norm",
                      first_non_empty({get(job.health_fact, "grad_clip_norm"),
                                       get(job.result_fact, "grad_clip_norm"),
                                       job.grad_clip_norm}));
  append_bool_field_if_present(out, &first, "finite_parameter_check",
                               health_or_report(job, "finite_parameter_check",
                                                "finite_parameter_check"));
  append_bool_field(
      out, &first, "checkpoint_written",
      first_non_empty({get(job.result_fact, "checkpoint_written"),
                       get(job.checkpoint_io_fact, "checkpoint_written"),
                       get(job.state, "checkpoint_written")}));
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
metrics_json(const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto &role :
       {"vicreg_train", "mdn_train", "mdn_validation_eval"}) {
    const auto *job = find_role(jobs, role);
    if (!job) {
      continue;
    }
    if (!first) {
      out << ",";
    }
    first = false;
    out << detail::json_quote(role) << ":" << metric_json_for_job(*job);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string target_statuses_json(
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < statuses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << detail::json_quote(statuses[i].target_id)
        << ":{\"status\":" << detail::json_quote(statuses[i].status)
        << ",\"proof_certificate_check_passed\":"
        << (statuses[i].proof_certificate_check_passed ? "true" : "false")
        << ",\"proof_certificate_issues\":"
        << json_string_array(statuses[i].proof_certificate_issues)
        << ",\"deficit_keys\":" << json_string_array(statuses[i].deficit_keys)
        << ",\"warning_ids\":" << json_string_array(statuses[i].warning_ids)
        << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::vector<std::string>
target_blocker_reasons(const marshal_lattice_target_status_t &status) {
  std::vector<std::string> reasons;
  if (status.status != "satisfied") {
    reasons.push_back(status.status == "unavailable"
                          ? "target_status_unavailable"
                          : "target_not_satisfied");
  }
  if (!status.proof_certificate_check_passed) {
    reasons.push_back("proof_certificate_check_failed");
  }
  for (const auto &deficit : status.deficit_keys) {
    reasons.push_back("deficit:" + deficit);
  }
  for (const auto &warning : status.warning_ids) {
    reasons.push_back("warning:" + warning);
  }
  return reasons;
}

[[nodiscard]] inline std::string
target_next_safe_action(const marshal_lattice_target_status_t &status) {
  if (status.status == "satisfied" && status.proof_certificate_check_passed &&
      status.warning_ids.empty()) {
    return "inspect_lattice_certificate";
  }
  if (status.status == "unavailable") {
    return "ask_lattice_to_recheck_targets";
  }
  if (!status.proof_certificate_check_passed) {
    return "inspect_lattice_certificate_failure";
  }
  if (!status.deficit_keys.empty()) {
    return "reach_lattice_target";
  }
  if (!status.warning_ids.empty()) {
    return "inspect_lattice_warnings";
  }
  return "reach_lattice_target";
}

[[nodiscard]] inline std::string target_blockers_json(
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < statuses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    const auto reasons = target_blocker_reasons(statuses[i]);
    out << "{\"target_id\":" << detail::json_quote(statuses[i].target_id)
        << ",\"status\":" << detail::json_quote(statuses[i].status)
        << ",\"source\":\"lattice_evaluate_targets\""
        << ",\"proof_certificate_check_passed\":"
        << (statuses[i].proof_certificate_check_passed ? "true" : "false")
        << ",\"blockers\":" << json_string_array(reasons)
        << ",\"proof_certificate_issues\":"
        << json_string_array(statuses[i].proof_certificate_issues)
        << ",\"deficit_keys\":" << json_string_array(statuses[i].deficit_keys)
        << ",\"warning_ids\":" << json_string_array(statuses[i].warning_ids)
        << ",\"next_safe_action\":"
        << detail::json_quote(target_next_safe_action(statuses[i]))
        << ",\"target_satisfaction_claimed_by_marshal\":false}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline bool all_targets_satisfied(
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  return !statuses.empty() &&
         std::all_of(statuses.begin(), statuses.end(), [](const auto &row) {
           return row.status == "satisfied" &&
                  row.proof_certificate_check_passed;
         });
}

[[nodiscard]] inline std::string
health_observations_json(const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto &job : jobs) {
    if (job.role != "mdn_train") {
      continue;
    }
    const auto max_grad =
        health_or_report(job, "grad_norm_max_pre_clip", "max_grad_norm");
    const auto clip_norm = first_non_empty(
        {get(job.health_fact, "grad_clip_norm"),
         get(job.result_fact, "grad_clip_norm"), job.grad_clip_norm});
    if (!json_number_is_valid(max_grad) || !json_number_is_valid(clip_norm)) {
      continue;
    }
    const double observed = std::strtod(max_grad.c_str(), nullptr);
    const double clip = std::strtod(clip_norm.c_str(), nullptr);
    if (clip > 0.0 && observed > clip * 100.0) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << "{\"id\":\"high_pre_clip_grad_norm\",\"severity\":\"watch\""
          << ",\"blocking\":false"
          << ",\"component\":"
          << detail::json_quote(get(job.state, "target_component_family_id"))
          << ",\"job_id\":" << detail::json_quote(get(job.state, "job_id"))
          << ",\"observed_value\":" << json_number_or_null(max_grad)
          << ",\"clip_norm\":" << json_number_or_null(clip_norm)
          << ",\"ratio_to_clip_norm\":" << observed / clip
          << ",\"note\":\"run stayed finite but gradient pressure should be "
             "tracked\"}";
    }
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::vector<std::string> next_safe_actions(
    const std::vector<job_summary_t> &jobs,
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  std::vector<std::string> actions;
  if (!all_targets_satisfied(statuses)) {
    actions.push_back("ask_lattice_to_recheck_targets");
  }
  if (find_role(jobs, "vicreg_train") && find_role(jobs, "mdn_train") &&
      find_role(jobs, "mdn_validation_eval")) {
    const bool all_chain_jobs_have_terminal_facts =
        has_terminal_facts(*find_role(jobs, "vicreg_train")) &&
        has_terminal_facts(*find_role(jobs, "mdn_train")) &&
        has_terminal_facts(*find_role(jobs, "mdn_validation_eval"));
    actions.push_back(all_chain_jobs_have_terminal_facts
                          ? "inspect_runtime_terminal_result_facts"
                          : "implement_runtime_terminal_result_facts");
    actions.push_back(
        "run_additional_validation_eval_only_through_checkpoint_bound_handoff");
    actions.push_back("compare_against_next_candidate_when_run_facts_exist");
  } else {
    actions.push_back("inspect_missing_train_eval_chain_jobs");
  }
  return actions;
}

[[nodiscard]] inline std::string current_state_json(
    const std::vector<job_summary_t> &jobs,
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  const auto *vicreg = find_role(jobs, "vicreg_train");
  const auto *mdn = find_role(jobs, "mdn_train");
  const auto *eval = find_role(jobs, "mdn_validation_eval");
  const bool chain_completed = vicreg && mdn && eval &&
                               job_completed(*vicreg) && job_completed(*mdn) &&
                               job_completed(*eval);
  bool model_state_mutated = false;
  for (const auto &job : jobs) {
    model_state_mutated = model_state_mutated || job_mutated_model_state(job);
  }
  const bool eval_mutated =
      eval != nullptr &&
      (bool_value(first_non_empty(
           {get(eval->result_fact, "model_state_mutated"),
            get(eval->checkpoint_io_fact, "model_state_mutated")})) ||
       bool_value(
           first_non_empty({get(eval->result_fact, "checkpoint_written"),
                            get(eval->checkpoint_io_fact, "checkpoint_written"),
                            get(eval->state, "checkpoint_written")})) ||
       optimizer_steps_mutated_model_state(
           first_non_empty({get(eval->result_fact, "optimizer_steps"),
                            get(eval->state, "optimizer_steps")})));
  std::ostringstream out;
  out << "{\"proof_status\":"
      << detail::json_quote(all_targets_satisfied(statuses) ? "clean"
                                                            : "unknown")
      << ",\"execution_status\":"
      << detail::json_quote(chain_completed ? "completed_chain"
                                            : "partial_or_unknown")
      << ",\"model_state_mutated\":" << (model_state_mutated ? "true" : "false")
      << ",\"validation_eval_mutated_model_state\":"
      << (eval_mutated ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] inline std::string operator_summary_json(
    const std::vector<job_summary_t> &jobs,
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  const auto *vicreg = find_role(jobs, "vicreg_train");
  const auto *mdn = find_role(jobs, "mdn_train");
  const auto *eval = find_role(jobs, "mdn_validation_eval");
  const bool chain_completed = vicreg && mdn && eval &&
                               job_completed(*vicreg) && job_completed(*mdn) &&
                               job_completed(*eval);
  const bool proof_clean = all_targets_satisfied(statuses);
  const auto actions = next_safe_actions(jobs, statuses);
  std::ostringstream out;
  out << "{\"headline\":"
      << detail::json_quote(chain_completed
                                ? "End-to-end train/eval chain completed."
                                : "Training/eval chain is partial or unknown.")
      << ",\"proof_status\":"
      << detail::json_quote(proof_clean ? "clean" : "unknown")
      << ",\"execution_status\":"
      << detail::json_quote(chain_completed ? "completed_chain"
                                            : "partial_or_unknown")
      << ",\"job_count\":" << jobs.size()
      << ",\"target_count\":" << statuses.size() << ",\"next_safe_action\":"
      << detail::json_quote(actions.empty() ? std::string{} : actions.front())
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
stop_reason_json(const std::vector<job_summary_t> &jobs,
                 const std::vector<marshal_lattice_target_status_t> &statuses) {
  const bool chain_completed =
      std::all_of(jobs.begin(), jobs.end(), job_completed);
  const bool proof_clean = all_targets_satisfied(statuses);
  const auto actions = next_safe_actions(jobs, statuses);
  std::string terminal_state = "report_ready";
  std::string blocking_owner = "none";
  std::string human_reason = "Read-only operator report generated.";
  if (!statuses.empty() && !proof_clean) {
    terminal_state = "report_ready_with_lattice_blockers";
    blocking_owner = "lattice";
    human_reason =
        "Lattice target status or certificate checks still need inspection.";
  } else if (!jobs.empty() && !chain_completed) {
    terminal_state = "report_ready_with_runtime_gaps";
    blocking_owner = "runtime";
    human_reason =
        "Runtime job chain is partial, incomplete, or missing terminal state.";
  }
  std::ostringstream out;
  out << "{\"terminal_state\":" << detail::json_quote(terminal_state)
      << ",\"human_reason\":" << detail::json_quote(human_reason)
      << ",\"blocking_owner\":" << detail::json_quote(blocking_owner)
      << ",\"next_safe_action\":"
      << detail::json_quote(actions.empty() ? std::string{} : actions.front())
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
runtime_panel_json(const std::vector<job_summary_t> &jobs) {
  std::int64_t completed = 0;
  std::int64_t mutated = 0;
  std::int64_t terminal_facts = 0;
  std::int64_t checkpoints_written = 0;
  for (const auto &job : jobs) {
    if (job_completed(job)) {
      ++completed;
    }
    if (job_mutated_model_state(job)) {
      ++mutated;
    }
    if (has_terminal_facts(job)) {
      ++terminal_facts;
    }
    if (bool_value(
            first_non_empty({get(job.result_fact, "checkpoint_written"),
                             get(job.checkpoint_io_fact, "checkpoint_written"),
                             get(job.state, "checkpoint_written")}))) {
      ++checkpoints_written;
    }
  }
  std::ostringstream out;
  out << "{\"job_count\":" << jobs.size()
      << ",\"completed_job_count\":" << completed
      << ",\"model_state_mutating_job_count\":" << mutated
      << ",\"terminal_fact_job_count\":" << terminal_facts
      << ",\"checkpoint_written_count\":" << checkpoints_written
      << ",\"jobs\":" << job_chain_json(jobs)
      << ",\"checkpoint_io\":{\"produced\":" << checkpoint_rows_json(jobs, true)
      << ",\"loaded\":" << checkpoint_rows_json(jobs, false) << "}}";
  return out.str();
}

[[nodiscard]] inline std::string lattice_panel_json(
    const std::vector<marshal_lattice_target_status_t> &statuses) {
  std::int64_t satisfied = 0;
  std::int64_t proof_failed = 0;
  std::int64_t warning_count = 0;
  for (const auto &status : statuses) {
    if (status.status == "satisfied" && status.proof_certificate_check_passed) {
      ++satisfied;
    }
    if (!status.proof_certificate_check_passed) {
      ++proof_failed;
    }
    warning_count += static_cast<std::int64_t>(status.warning_ids.size());
  }
  std::ostringstream out;
  out << "{\"source\":\"hero.lattice.evaluate_targets\""
      << ",\"target_count\":" << statuses.size()
      << ",\"satisfied_count\":" << satisfied
      << ",\"proof_certificate_failure_count\":" << proof_failed
      << ",\"warning_count\":" << warning_count << ",\"proof_status\":"
      << detail::json_quote(all_targets_satisfied(statuses) ? "clean"
                                                            : "needs_review")
      << ",\"target_statuses\":" << target_statuses_json(statuses)
      << ",\"target_blockers\":" << target_blockers_json(statuses) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
audit_panel_json(const marshal_operational_report_options_t &options,
                 const std::string &report_id, const std::string &generated_at,
                 const std::vector<job_summary_t> &jobs) {
  std::ostringstream out;
  out << "{\"report_id\":" << detail::json_quote(report_id)
      << ",\"generated_at\":" << detail::json_quote(generated_at)
      << ",\"runtime_root\":"
      << detail::json_quote(options.runtime_root.string())
      << ",\"config_path\":" << detail::json_quote(options.config_path.string())
      << ",\"job_count\":" << jobs.size() << ",\"full_payload_available\":true"
      << ",\"machine_payload_included\":"
      << (options.include_machine_payload ? "true" : "false")
      << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"target_satisfaction_claimed\":false"
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement) << "}";
  return out.str();
}

} // namespace operational_report_detail

[[nodiscard]] inline std::vector<std::string>
default_marshal_operational_report_target_ids() {
  return {"vicreg_train_core_ready", "channel_mdn_train_core_ready",
          "channel_mdn_train_core_no_validation_leakage",
          "channel_mdn_train_core_no_test_leakage",
          "channel_mdn_validation_eval_ready"};
}

[[nodiscard]] inline std::string build_marshal_operational_report_json(
    marshal_operational_report_options_t options) {
  if (options.target_ids.empty()) {
    options.target_ids = default_marshal_operational_report_target_ids();
  }
  const auto jobs = operational_report_detail::discover_jobs(options);
  const auto generated_at = operational_report_detail::current_utc_timestamp();
  std::ostringstream digest_basis;
  digest_basis << "runtime_root=" << options.runtime_root.string() << "\n";
  for (const auto &job : jobs) {
    digest_basis << "job="
                 << operational_report_detail::get(job.state, "job_id") << "|"
                 << operational_report_detail::get(job.state, "status") << "\n";
  }
  for (const auto &status : options.target_statuses) {
    digest_basis << "target=" << status.target_id << "|" << status.status
                 << "|proof="
                 << (status.proof_certificate_check_passed ? "true" : "false")
                 << "\n";
  }
  const auto report_id = marshal_digest_for_text(
      "kikijyeba.marshal.operational_report_id.v1", digest_basis.str());

  std::ostringstream out;
  out << "{\"ok\":true"
      << ",\"report_kind\":\"training_state\""
      << ",\"schema_version\":"
      << detail::json_quote(k_marshal_operational_report_schema_v1)
      << ",\"report_id\":" << detail::json_quote(report_id)
      << ",\"generated_at\":" << detail::json_quote(generated_at)
      << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"target_satisfaction_claimed\":false"
      << ",\"evidence_scope\":"
      << operational_report_detail::evidence_scope_json(options, jobs);
  out << ",\"operator_summary\":"
      << operational_report_detail::operator_summary_json(
             jobs, options.target_statuses);
  out << ",\"stop_reason\":"
      << operational_report_detail::stop_reason_json(jobs,
                                                     options.target_statuses);
  out << ",\"runtime_panel\":"
      << operational_report_detail::runtime_panel_json(jobs);
  out << ",\"lattice_panel\":"
      << operational_report_detail::lattice_panel_json(options.target_statuses);
  out << ",\"audit_panel\":"
      << operational_report_detail::audit_panel_json(options, report_id,
                                                     generated_at, jobs);
  out << ",\"current_state\":"
      << operational_report_detail::current_state_json(jobs,
                                                       options.target_statuses);
  out << ",\"job_chain\":" << operational_report_detail::job_chain_json(jobs);
  out << ",\"chain_summary\":"
      << operational_report_detail::chain_summary_json(jobs);
  out << ",\"target_blockers\":"
      << operational_report_detail::target_blockers_json(
             options.target_statuses);
  out << ",\"warnings\":{\"lattice\":[],\"health_observations\":"
      << operational_report_detail::health_observations_json(jobs) << "}";
  out << ",\"suspicious_items\":"
      << operational_report_detail::health_observations_json(jobs);
  out << ",\"next_safe_actions\":"
      << operational_report_detail::json_string_array(
             operational_report_detail::next_safe_actions(
                 jobs, options.target_statuses));
  out << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (options.include_machine_payload) {
    out << ",\"machine_payload\":{\"target_status_count\":"
        << options.target_statuses.size() << ",\"job_count\":" << jobs.size()
        << ",\"latest_jobs\":[";
    for (std::size_t i = 0; i < jobs.size(); ++i) {
      if (i != 0) {
        out << ",";
      }
      out << operational_report_detail::job_json(jobs[i]);
    }
    out << "]";
    out << ",\"produced_checkpoints\":"
        << operational_report_detail::checkpoint_rows_json(jobs, true);
    out << ",\"loaded_checkpoints\":"
        << operational_report_detail::checkpoint_rows_json(jobs, false);
    out << ",\"target_statuses\":"
        << operational_report_detail::target_statuses_json(
               options.target_statuses);
    out << ",\"metrics\":" << operational_report_detail::metrics_json(jobs)
        << "}";
  }
  out << "}";
  return out.str();
}

} // namespace cuwacunu::kikijyeba::marshal
