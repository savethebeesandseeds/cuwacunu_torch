#pragma once

#include <cctype>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/wave_space_t.h"

namespace cuwacunu {
namespace iitepi {

struct observation_dsl_path_resolution_t {
  std::string sources_file{};
  std::string channels_file{};
  std::string sources_path{};
  std::string channels_path{};
  bool sources_from_contract{false};
  bool channels_from_contract{false};
};

[[nodiscard]] inline std::string observation_trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])) != 0)
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0)
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string observation_unquote_if_wrapped(std::string s) {
  s = observation_trim_ascii_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] inline std::string
observation_resolve_path_from_folder(std::string folder, std::string path) {
  folder = observation_trim_ascii_copy(std::move(folder));
  path = observation_trim_ascii_copy(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.string();
  if (folder.empty())
    return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] inline std::string contract_dsl_variable_or_empty(
    const std::shared_ptr<const contract_record_t> &contract_record,
    std::string_view name) {
  if (!contract_record)
    return {};
  const auto section_it = contract_record->config.find("ASSEMBLY");
  if (section_it == contract_record->config.end())
    return {};
  const auto value_it = section_it->second.find(std::string(name));
  if (value_it == section_it->second.end())
    return {};
  return observation_unquote_if_wrapped(value_it->second);
}

[[nodiscard]] inline bool resolve_observation_dsl_paths(
    const std::shared_ptr<const contract_record_t> &contract_record,
    const std::shared_ptr<const wave_record_t> &wave_record,
    const cuwacunu::camahjucunu::iitepi_wave_t &selected_wave,
    observation_dsl_path_resolution_t *out, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing observation DSL path output";
    return false;
  }
  *out = {};
  if (!wave_record) {
    if (error)
      *error = "missing wave record for observation DSL path resolution";
    return false;
  }
  if (selected_wave.sources.size() != 1) {
    if (error) {
      *error =
          "wave '" + selected_wave.name +
          "' must provide exactly one SOURCE block for dataloader ownership";
    }
    return false;
  }

  out->sources_file = contract_dsl_variable_or_empty(
      contract_record, "__observation_sources_dsl_file");
  out->channels_file = contract_dsl_variable_or_empty(
      contract_record, "__observation_channels_dsl_file");
  out->sources_from_contract = !out->sources_file.empty();
  out->channels_from_contract = !out->channels_file.empty();

  if (out->sources_file.empty()) {
    if (error) {
      *error = "wave '" + selected_wave.name +
               "' requires contract __observation_sources_dsl_file";
    }
    return false;
  }
  if (out->channels_file.empty()) {
    if (error) {
      *error = "wave '" + selected_wave.name +
               "' requires contract __observation_channels_dsl_file";
    }
    return false;
  }

  out->sources_path = observation_resolve_path_from_folder(
      std::filesystem::path(contract_record ? contract_record->config_file_path
                                            : wave_record->config_file_path)
          .parent_path()
          .string(),
      out->sources_file);
  out->channels_path = observation_resolve_path_from_folder(
      std::filesystem::path(contract_record ? contract_record->config_file_path
                                            : wave_record->config_file_path)
          .parent_path()
          .string(),
      out->channels_file);

  if (out->sources_path.empty() || out->channels_path.empty()) {
    if (error) {
      *error = "failed to resolve observation DSL paths for wave '" +
               selected_wave.name + "'";
    }
    return false;
  }

  return true;
}

} // namespace iitepi
} // namespace cuwacunu
