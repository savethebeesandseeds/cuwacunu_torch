// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "hero/runtime_hero/runtime/job_manifest.h"
#include "hero/runtime_hero/runtime/job_state.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::hero::runtime::terminal_facts {

inline constexpr const char *k_runtime_result_fact_leaf = "runtime.result.fact";
inline constexpr const char *k_runtime_checkpoint_io_fact_leaf =
    "runtime.checkpoint_io.fact";
inline constexpr const char *k_runtime_health_measurement_fact_leaf =
    "runtime.health_measurement.fact";

namespace detail {

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

[[nodiscard]] inline std::string
read_text_file_or_empty(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::map<std::string, std::string>
parse_assignment_text(std::string_view text) {
  std::map<std::string, std::string> out;
  std::size_t begin = 0;
  while (begin <= text.size()) {
    const auto newline = text.find('\n', begin);
    const auto end = newline == std::string_view::npos ? text.size() : newline;
    const auto line = text.substr(begin, end - begin);
    const auto eq = line.find('=');
    if (eq != std::string_view::npos) {
      const auto key = trim_ascii(line.substr(0, eq));
      const auto value = trim_ascii(line.substr(eq + 1U));
      if (!key.empty()) {
        out[key] = value;
      }
    }
    if (newline == std::string_view::npos) {
      break;
    }
    begin = newline + 1U;
  }
  return out;
}

[[nodiscard]] inline std::map<std::string, std::string>
parse_assignment_file(const std::filesystem::path &path) {
  return parse_assignment_text(read_text_file_or_empty(path));
}

[[nodiscard]] inline std::string
map_get(const std::map<std::string, std::string> &map, const std::string &key) {
  const auto it = map.find(key);
  return it == map.end() ? std::string{} : it->second;
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

template <typename ValueT>
[[nodiscard]] inline std::string to_text_value(const ValueT &value) {
  std::ostringstream out;
  out << value;
  return out.str();
}

[[nodiscard]] inline std::string bool_text(bool value) {
  return value ? "true" : "false";
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

[[nodiscard]] inline std::string
runtime_fact_digest_for_text(const std::string &domain,
                             const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.runtime.terminal_fact.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, domain);
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

inline void write_text_file_atomically(const std::filesystem::path &path,
                                       const std::string &text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  static std::atomic<std::uint64_t> tmp_counter{0};
  const auto tmp =
      path.string() + "." +
      runtime_fact_digest_for_text(
          "tmp",
          path.string() + "|" + text + "|" +
              std::to_string(
                  std::chrono::steady_clock::now().time_since_epoch().count()) +
              "|" + std::to_string(tmp_counter.fetch_add(1))) +
      ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc | std::ios::binary);
    if (!out) {
      throw std::runtime_error(
          "[runtime_terminal_facts] failed to open tmp path: " + tmp);
    }
    out << text;
    if (!out) {
      throw std::runtime_error(
          "[runtime_terminal_facts] failed to write tmp path: " + tmp);
    }
  }
  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    std::error_code remove_ec;
    std::filesystem::remove(tmp, remove_ec);
    throw std::runtime_error("[runtime_terminal_facts] failed to rename tmp " +
                             tmp + " to " + path.string() + ": " +
                             ec.message());
  }
}

[[nodiscard]] inline std::filesystem::path
report_path_for_job(const std::filesystem::path &job_dir,
                    const job_manifest_t &manifest, const job_state_t &state) {
  if (!state.report_path.empty()) {
    return std::filesystem::path(state.report_path);
  }
  if (manifest.job_kind == "channel_representation_vicreg") {
    return job_dir / "channel_representation.report";
  }
  if (manifest.job_kind == "channel_representation_mtf_jepa_mae_vicreg") {
    return job_dir / "channel_representation.report";
  }
  if (manifest.job_kind == "channel_inference_mdn") {
    return job_dir / "channel_inference.report";
  }
  return {};
}

[[nodiscard]] inline std::filesystem::path
config_pointer(const std::filesystem::path &config_path,
               const std::string &key) {
  const auto map = parse_assignment_file(config_path);
  auto value = map_get(map, key);
  value = strip_trailing_semicolon(value);
  return value.empty() ? std::filesystem::path{} : std::filesystem::path(value);
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

[[nodiscard]] inline std::string
grad_clip_norm_for_job(const job_manifest_t &manifest,
                       const std::map<std::string, std::string> &report) {
  if (manifest.job_kind != "channel_inference_mdn") {
    return {};
  }
  const auto report_config = map_get(report, "config_path");
  const auto config_path = report_config.empty()
                               ? std::filesystem::path(manifest.config_path)
                               : std::filesystem::path(report_config);
  auto jkimyei_path = config_pointer(
      config_path, "wikimyei_inference_expected_value_mdn_jkimyei_path");
  if (jkimyei_path.empty() &&
      config_path != std::filesystem::path(manifest.config_path)) {
    jkimyei_path =
        config_pointer(manifest.config_path,
                       "wikimyei_inference_expected_value_mdn_jkimyei_path");
  }
  if (jkimyei_path.empty()) {
    return {};
  }
  return dsl_assignment_value(jkimyei_path, "GRAD_CLIP_NORM");
}

[[nodiscard]] inline bool model_state_mutated(const job_state_t &state) {
  return state.status == "completed" &&
         (state.checkpoint_written || state.optimizer_steps > 0);
}

inline void put(std::map<std::string, std::string> *fields,
                const std::string &key, std::string value) {
  (*fields)[key] = std::move(value);
}

inline void put_common(std::map<std::string, std::string> *fields,
                       const std::string &fact_type,
                       const std::string &schema_version,
                       const job_manifest_t &manifest, const job_state_t &state,
                       const std::filesystem::path &report_path,
                       const std::string &source_report_hash) {
  put(fields, "fact_type", fact_type);
  put(fields, "schema_version", schema_version);
  put(fields, "producer", "runtime");
  put(fields, "job_id",
      manifest.job_id.empty() ? state.job_id : manifest.job_id);
  put(fields, "job_stable_id",
      manifest.job_stable_id.empty() ? state.job_stable_id
                                     : manifest.job_stable_id);
  put(fields, "job_attempt_id",
      manifest.job_attempt_id.empty() ? state.job_attempt_id
                                      : manifest.job_attempt_id);
  put(fields, "job_attempt_index",
      manifest.job_attempt_id.empty()
          ? std::to_string(state.job_attempt_index)
          : std::to_string(manifest.job_attempt_index));
  put(fields, "job_attempt_policy",
      manifest.job_attempt_policy.empty() ? state.job_attempt_policy
                                          : manifest.job_attempt_policy);
  put(fields, "job_kind",
      manifest.job_kind.empty() ? state.job_kind : manifest.job_kind);
  put(fields, "wave_id",
      manifest.wave_id.empty() ? state.wave_id : manifest.wave_id);
  put(fields, "wave_action",
      manifest.wave_action.empty() ? state.wave_action : manifest.wave_action);
  put(fields, "component_family_id",
      manifest.component_family_id.empty() ? manifest.target_component_family_id
                                           : manifest.component_family_id);
  put(fields, "target_component_family_id",
      manifest.target_component_family_id.empty()
          ? state.target_component_family_id
          : manifest.target_component_family_id);
  put(fields, "config_bundle_hash", manifest.config_bundle_id);
  put(fields, "runtime_policy_hash", "");
  put(fields, "runtime_handoff_id", manifest.runtime_handoff_id);
  put(fields, "runtime_handoff_digest", manifest.runtime_handoff_digest);
  put(fields, "marshal_target_driver_run_id",
      manifest.marshal_target_driver_run_id);
  put(fields, "source_report_path", report_path.string());
  put(fields, "source_report_hash", source_report_hash);
  put(fields, "created_at", current_utc_timestamp());
}

[[nodiscard]] inline std::string
canonical_fact_text(const std::map<std::string, std::string> &fields) {
  std::ostringstream out;
  for (const auto &[key, value] : fields) {
    if (key == "fact_digest") {
      continue;
    }
    out << key << "=" << value << "\n";
  }
  return out.str();
}

inline void write_fact_file(const std::filesystem::path &path,
                            std::map<std::string, std::string> fields) {
  auto canonical = canonical_fact_text(fields);
  const auto digest =
      runtime_fact_digest_for_text(map_get(fields, "fact_type"), canonical);
  std::ostringstream out;
  out << canonical << "fact_digest=" << digest << "\n";
  write_text_file_atomically(path, out.str());
}

[[nodiscard]] inline std::map<std::string, std::string> make_result_fact_fields(
    const job_manifest_t &manifest, const job_state_t &state,
    const std::map<std::string, std::string> &report,
    const std::filesystem::path &report_path,
    const std::string &source_report_hash, const std::string &grad_clip_norm) {
  std::map<std::string, std::string> fields;
  put_common(&fields, "runtime.result.fact", "1", manifest, state, report_path,
             source_report_hash);
  const auto final_loss = first_non_empty(
      {map_get(report, "last_loss"), to_text_value(state.last_loss)});
  put(&fields, "fact_id",
      runtime_fact_digest_for_text(
          "runtime.result.fact.id",
          (manifest.job_id.empty() ? state.job_id : manifest.job_id) + "|" +
              state.status + "|" + source_report_hash + "|" + final_loss));
  put(&fields, "status", state.status);
  put(&fields, "error_message", state.error_message);
  put(&fields, "optimizer_steps", to_text_value(state.optimizer_steps));
  put(&fields, "checkpoint_written", bool_text(state.checkpoint_written));
  put(&fields, "checkpoint_path", state.checkpoint_path);
  put(&fields, "model_state_mutated", bool_text(model_state_mutated(state)));
  put(&fields, "finite_parameter_check",
      map_get(report, "finite_parameter_check"));
  put(&fields, "nonfinite_output_count",
      map_get(report, "nonfinite_output_count"));
  put(&fields, "final_loss", final_loss);
  put(&fields, "mean_loss", map_get(report, "mean_loss"));
  put(&fields, "valid_target_fraction",
      first_non_empty({map_get(report, "mean_valid_target_fraction"),
                       map_get(report, "last_valid_target_fraction")}));
  put(&fields, "last_valid_target_count",
      map_get(report, "last_valid_target_count"));
  put(&fields, "total_valid_target_count",
      map_get(report, "total_valid_target_count"));
  put(&fields, "grad_norm_last", map_get(report, "last_grad_norm"));
  put(&fields, "grad_norm_max_pre_clip", map_get(report, "max_grad_norm"));
  put(&fields, "grad_clip_norm", grad_clip_norm);
  put(&fields, "sigma_min", map_get(report, "min_sigma_min"));
  put(&fields, "sigma_mean", map_get(report, "mean_sigma_mean"));
  put(&fields, "sigma_max", map_get(report, "max_sigma_max"));
  put(&fields, "sigma_min_valid", map_get(report, "min_sigma_min_valid"));
  put(&fields, "sigma_mean_valid", map_get(report, "mean_sigma_mean_valid"));
  put(&fields, "sigma_max_valid", map_get(report, "max_sigma_max_valid"));
  put(&fields, "mixture_entropy", map_get(report, "mean_mixture_entropy"));
  put(&fields, "mixture_usage", map_get(report, "mean_mixture_usage"));
  put(&fields, "mean_nll_per_channel", map_get(report, "mean_nll_per_channel"));
  put(&fields, "mean_nll_per_target_feature",
      map_get(report, "mean_nll_per_target_feature"));
  put(&fields, "mean_nll_per_channel_target_feature",
      map_get(report, "mean_nll_per_channel_target_feature"));
  put(&fields, "valid_target_count_per_channel",
      map_get(report, "valid_target_count_per_channel"));
  put(&fields, "valid_target_count_per_target_feature",
      map_get(report, "valid_target_count_per_target_feature"));
  put(&fields, "valid_target_count_per_channel_target_feature",
      map_get(report, "valid_target_count_per_channel_target_feature"));
  put(&fields, "representation_effective_rank_fraction",
      map_get(report, "representation_effective_rank_fraction"));
  return fields;
}

[[nodiscard]] inline std::map<std::string, std::string>
make_checkpoint_io_fact_fields(const job_manifest_t &manifest,
                               const job_state_t &state,
                               const std::map<std::string, std::string> &report,
                               const std::filesystem::path &report_path,
                               const std::string &source_report_hash) {
  std::map<std::string, std::string> fields;
  put_common(&fields, "runtime.checkpoint_io.fact", "1", manifest, state,
             report_path, source_report_hash);
  put(&fields, "fact_id",
      runtime_fact_digest_for_text(
          "runtime.checkpoint_io.fact.id",
          (manifest.job_id.empty() ? state.job_id : manifest.job_id) + "|" +
              state.status + "|" + state.checkpoint_path + "|" +
              map_get(report, "representation_checkpoint_path") + "|" +
              map_get(report, "mdn_checkpoint_path")));
  put(&fields, "checkpoint_written", bool_text(state.checkpoint_written));
  put(&fields, "checkpoint_path", state.checkpoint_path);
  put(&fields, "checkpoint_write_count",
      to_text_value(state.checkpoint_write_count));
  put(&fields, "checkpoint_artifact_hash",
      map_get(report, "checkpoint_digest_reported"));
  put(&fields, "checkpoint_path_reported",
      first_non_empty({map_get(report, "checkpoint_path_reported"),
                       state.checkpoint_path}));
  put(&fields, "checkpoint_digest_reported",
      map_get(report, "checkpoint_digest_reported"));
  put(&fields, "checkpoint_digest_verified",
      map_get(report, "checkpoint_digest_verified"));
  put(&fields, "checkpoint_file_exists",
      map_get(report, "checkpoint_file_exists"));
  put(&fields, "checkpoint_bytes", map_get(report, "checkpoint_bytes"));
  put(&fields, "checkpoint_mtime_ticks",
      map_get(report, "checkpoint_mtime_ticks"));
  put(&fields, "checkpoint_artifact_status",
      map_get(report, "checkpoint_artifact_status"));
  put(&fields, "representation_checkpoint_path",
      first_non_empty({map_get(report, "representation_checkpoint_path"),
                       manifest.input_representation_checkpoint_path}));
  put(&fields, "representation_checkpoint_loaded",
      map_get(report, "representation_checkpoint_loaded"));
  put(&fields, "mdn_checkpoint_path",
      first_non_empty({map_get(report, "mdn_checkpoint_path"),
                       manifest.input_mdn_checkpoint_path}));
  put(&fields, "mdn_checkpoint_loaded",
      map_get(report, "mdn_checkpoint_loaded"));
  put(&fields, "model_state_mutated", bool_text(model_state_mutated(state)));
  return fields;
}

[[nodiscard]] inline std::map<std::string, std::string> make_health_fact_fields(
    const job_manifest_t &manifest, const job_state_t &state,
    const std::map<std::string, std::string> &report,
    const std::filesystem::path &report_path,
    const std::string &source_report_hash, const std::string &grad_clip_norm) {
  std::map<std::string, std::string> fields;
  put_common(&fields, "runtime.health_measurement.fact", "1", manifest, state,
             report_path, source_report_hash);
  put(&fields, "fact_id",
      runtime_fact_digest_for_text(
          "runtime.health_measurement.fact.id",
          (manifest.job_id.empty() ? state.job_id : manifest.job_id) + "|" +
              state.status + "|" + source_report_hash));
  put(&fields, "finite_parameter_check",
      map_get(report, "finite_parameter_check"));
  put(&fields, "nonfinite_output_count",
      map_get(report, "nonfinite_output_count"));
  put(&fields, "grad_norm_last", map_get(report, "last_grad_norm"));
  put(&fields, "grad_norm_max_pre_clip", map_get(report, "max_grad_norm"));
  put(&fields, "grad_clip_norm", grad_clip_norm);
  put(&fields, "sigma_min", map_get(report, "min_sigma_min"));
  put(&fields, "sigma_mean", map_get(report, "mean_sigma_mean"));
  put(&fields, "sigma_max", map_get(report, "max_sigma_max"));
  put(&fields, "sigma_min_valid", map_get(report, "min_sigma_min_valid"));
  put(&fields, "sigma_mean_valid", map_get(report, "mean_sigma_mean_valid"));
  put(&fields, "sigma_max_valid", map_get(report, "max_sigma_max_valid"));
  put(&fields, "mixture_entropy", map_get(report, "mean_mixture_entropy"));
  put(&fields, "mixture_usage", map_get(report, "mean_mixture_usage"));
  put(&fields, "mean_nll_per_channel", map_get(report, "mean_nll_per_channel"));
  put(&fields, "mean_nll_per_target_feature",
      map_get(report, "mean_nll_per_target_feature"));
  put(&fields, "mean_nll_per_channel_target_feature",
      map_get(report, "mean_nll_per_channel_target_feature"));
  put(&fields, "valid_target_count_per_channel",
      map_get(report, "valid_target_count_per_channel"));
  put(&fields, "valid_target_count_per_target_feature",
      map_get(report, "valid_target_count_per_target_feature"));
  put(&fields, "valid_target_count_per_channel_target_feature",
      map_get(report, "valid_target_count_per_channel_target_feature"));
  put(&fields, "representation_effective_rank_fraction",
      map_get(report, "representation_effective_rank_fraction"));
  put(&fields, "representation_min_dimension_variance",
      map_get(report, "representation_min_dimension_variance"));
  put(&fields, "representation_condition_number",
      map_get(report, "representation_condition_number"));
  put(&fields, "representation_isotropy_score",
      map_get(report, "representation_isotropy_score"));
  return fields;
}

} // namespace detail

inline void write_terminal_fact_sidecars(const std::filesystem::path &job_dir,
                                         const job_manifest_t &manifest,
                                         job_state_t *state) {
  if (state == nullptr) {
    return;
  }
  try {
    const auto report_path =
        detail::report_path_for_job(job_dir, manifest, *state);
    const auto report_text = report_path.empty()
                                 ? std::string{}
                                 : detail::read_text_file_or_empty(report_path);
    const auto report = detail::parse_assignment_text(report_text);
    const auto source_report_hash = report_text.empty()
                                        ? std::string{}
                                        : detail::runtime_fact_digest_for_text(
                                              "source_report", report_text);
    const auto grad_clip_norm =
        detail::grad_clip_norm_for_job(manifest, report);

    const auto result_path = job_dir / k_runtime_result_fact_leaf;
    detail::write_fact_file(
        result_path,
        detail::make_result_fact_fields(manifest, *state, report, report_path,
                                        source_report_hash, grad_clip_norm));
    state->runtime_result_fact_written = true;
    state->runtime_result_fact_path = result_path.string();

    const auto checkpoint_io_path = job_dir / k_runtime_checkpoint_io_fact_leaf;
    detail::write_fact_file(
        checkpoint_io_path,
        detail::make_checkpoint_io_fact_fields(
            manifest, *state, report, report_path, source_report_hash));
    state->runtime_checkpoint_io_fact_written = true;
    state->runtime_checkpoint_io_fact_path = checkpoint_io_path.string();

    const auto health_path = job_dir / k_runtime_health_measurement_fact_leaf;
    detail::write_fact_file(
        health_path,
        detail::make_health_fact_fields(manifest, *state, report, report_path,
                                        source_report_hash, grad_clip_norm));
    state->runtime_health_measurement_fact_written = true;
    state->runtime_health_measurement_fact_path = health_path.string();
    state->runtime_terminal_fact_error.clear();
  } catch (const std::exception &ex) {
    state->runtime_terminal_fact_error = ex.what();
  }
}

} // namespace cuwacunu::hero::runtime::terminal_facts
