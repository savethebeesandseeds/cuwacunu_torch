// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace cuwacunu::hero::config_derivation {

namespace fs = std::filesystem;

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         (in[begin] == ' ' || in[begin] == '\t' || in[begin] == '\n' ||
          in[begin] == '\r' || in[begin] == '\f' || in[begin] == '\v')) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         (in[end - 1] == ' ' || in[end - 1] == '\t' || in[end - 1] == '\n' ||
          in[end - 1] == '\r' || in[end - 1] == '\f' || in[end - 1] == '\v')) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] inline bool ends_with(std::string_view text,
                                    std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.substr(text.size() - suffix.size()) == suffix;
}

[[nodiscard]] inline bool starts_with(std::string_view text,
                                      std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] inline std::optional<std::string>
paired_data_path_key(std::string_view grammar_key) {
  constexpr std::string_view suffix = "_bnf_path";
  std::string key = trim_ascii(grammar_key);
  if (!ends_with(key, suffix)) {
    return std::nullopt;
  }
  key.resize(key.size() - suffix.size());
  key += "_path";
  return key;
}

[[nodiscard]] inline std::optional<std::string>
paired_grammar_path_key(std::string_view data_key) {
  constexpr std::string_view suffix = "_path";
  std::string key = trim_ascii(data_key);
  if (!ends_with(key, suffix) || ends_with(key, "_bnf_path")) {
    return std::nullopt;
  }
  key.resize(key.size() - suffix.size());
  key += "_bnf_path";
  return key;
}

[[nodiscard]] inline fs::path
resolve_against_config(const fs::path &config_path, std::string_view raw_path) {
  fs::path path(trim_ascii(raw_path));
  if (path.empty() || path.is_absolute()) {
    return path.lexically_normal();
  }
  return (config_path.parent_path() / path).lexically_normal();
}

[[nodiscard]] inline std::string
canonical_grammar_filename_for_data_filename(std::string_view filename) {
  constexpr std::string_view protocol_prefix = "kikijyeba.protocol.";
  constexpr std::string_view protocol_suffix = ".dsl";
  if (starts_with(filename, protocol_prefix) &&
      ends_with(filename, protocol_suffix) &&
      filename != "kikijyeba.protocol.dsl") {
    return "kikijyeba.protocol.dsl.bnf";
  }
  return std::string(filename) + ".bnf";
}

[[nodiscard]] inline fs::path
default_grammar_path_for_data_path(const fs::path &config_path,
                                   const fs::path &data_path) {
  const std::string data_filename = data_path.filename().string();
  if (data_filename.empty()) {
    return {};
  }
  const std::string filename =
      canonical_grammar_filename_for_data_filename(data_filename);
  const fs::path grammar_root = data_path.has_parent_path()
                                    ? data_path.parent_path()
                                    : config_path.parent_path();
  return (grammar_root / "grammar" / filename).lexically_normal();
}

template <typename Map>
[[nodiscard]] inline std::optional<fs::path>
resolved_grammar_path_for_key(const Map &cfg, std::string_view grammar_key,
                              const fs::path &config_path,
                              bool values_are_already_resolved) {
  const std::string bnf_key = trim_ascii(grammar_key);
  const auto find_nonempty =
      [&cfg](const std::string &key) -> std::optional<std::string> {
    const auto it = cfg.find(key);
    if (it == cfg.end()) {
      return std::nullopt;
    }
    std::string value = trim_ascii(it->second);
    if (value.empty()) {
      return std::nullopt;
    }
    return value;
  };

  if (const auto authored = find_nonempty(bnf_key)) {
    return values_are_already_resolved
               ? fs::path(*authored).lexically_normal()
               : resolve_against_config(config_path, *authored);
  }

  const auto data_key = paired_data_path_key(bnf_key);
  if (!data_key.has_value()) {
    return std::nullopt;
  }
  const auto data_path_value = find_nonempty(*data_key);
  if (!data_path_value.has_value()) {
    return std::nullopt;
  }
  const fs::path data_path =
      values_are_already_resolved
          ? fs::path(*data_path_value).lexically_normal()
          : resolve_against_config(config_path, *data_path_value);
  return default_grammar_path_for_data_path(config_path, data_path);
}

} // namespace cuwacunu::hero::config_derivation
