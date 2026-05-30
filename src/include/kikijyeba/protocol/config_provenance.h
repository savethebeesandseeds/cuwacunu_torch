// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::protocol::config_provenance {

namespace detail {

namespace fs = std::filesystem;

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] inline bool has_suffix(std::string_view value,
                                     std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] inline std::string strip_ini_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] inline fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  fs::path normalized = fs::weakly_canonical(path, ec);
  if (ec) {
    normalized = fs::absolute(path, ec);
    if (ec) {
      normalized = path;
    }
  }
  return normalized.lexically_normal();
}

[[nodiscard]] inline std::string read_text_file(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[config_provenance] unable to open file: " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::optional<std::string>
read_text_file_if_exists(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    return std::nullopt;
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::string digest_text(std::string_view schema,
                                             std::string_view text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, schema);
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string now_utc_token() {
  const auto now = std::chrono::system_clock::now();
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
  const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &seconds);
#else
  gmtime_r(&seconds, &tm);
#endif
  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << "." << std::setw(3)
      << std::setfill('0') << (millis % 1000) << "Z";
  return out.str();
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_text(const std::string &text) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(text);
  std::string line;
  while (std::getline(lines, line)) {
    line = trim_ascii(strip_ini_comment(line));
    if (line.empty() || (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    auto key = trim_ascii(line.substr(0, eq));
    auto value = trim_ascii(line.substr(eq + 1));
    if (!key.empty()) {
      out[std::move(key)] = std::move(value);
    }
  }
  return out;
}

[[nodiscard]] inline std::string
map_get(const std::unordered_map<std::string, std::string> &map,
        const std::string &key, const std::string &fallback = {}) {
  const auto it = map.find(key);
  return it == map.end() ? fallback : it->second;
}

inline void write_text_file_atomic(const fs::path &path,
                                   const std::string &text) {
  fs::create_directories(path.parent_path());
  const fs::path tmp = path.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) {
      throw std::runtime_error(
          "[config_provenance] unable to open temp file: " + tmp.string());
    }
    out << text;
  }
  std::error_code ec;
  fs::rename(tmp, path, ec);
  if (ec) {
    fs::remove(path, ec);
    ec.clear();
    fs::rename(tmp, path, ec);
  }
  if (ec) {
    throw std::runtime_error("[config_provenance] unable to install file: " +
                             path.string() + ": " + ec.message());
  }
}

} // namespace detail

struct config_bundle_file_t {
  std::string config_key{};
  std::filesystem::path path{};
  bool exists{false};
  bool is_regular_file{false};
  bool size_known{false};
  std::uintmax_t size{0};
  std::string content_digest{};
  std::string error{};
};

struct config_bundle_receipt_t {
  std::string schema{"kikijyeba.config.bundle_receipt.v1"};
  std::filesystem::path global_config_path{};
  std::string config_bundle_id{};
  std::string config_receipt_id{};
  std::string captured_at_utc{};
  bool complete{false};
  std::vector<config_bundle_file_t> files{};
  std::vector<std::string> issues{};
};

[[nodiscard]] inline std::string
canonical_config_bundle_file_text(const config_bundle_file_t &file) {
  std::ostringstream out;
  out << "config_key=" << file.config_key << "\n";
  out << "path=" << file.path.lexically_normal().string() << "\n";
  out << "exists=" << (file.exists ? "true" : "false") << "\n";
  out << "is_regular_file=" << (file.is_regular_file ? "true" : "false")
      << "\n";
  out << "size_known=" << (file.size_known ? "true" : "false") << "\n";
  out << "size=" << file.size << "\n";
  out << "content_digest=" << file.content_digest << "\n";
  out << "error=" << file.error << "\n";
  return out.str();
}

[[nodiscard]] inline std::string
canonical_config_bundle_id_text(config_bundle_receipt_t receipt) {
  std::sort(receipt.files.begin(), receipt.files.end(),
            [](const auto &a, const auto &b) {
              if (a.config_key != b.config_key) {
                return a.config_key < b.config_key;
              }
              return a.path.lexically_normal().string() <
                     b.path.lexically_normal().string();
            });
  std::ostringstream out;
  out << "schema=kikijyeba.config.bundle_id.v1\n";
  out << "global_config_path="
      << receipt.global_config_path.lexically_normal().string() << "\n";
  for (const auto &file : receipt.files) {
    out << "[file]\n" << canonical_config_bundle_file_text(file);
  }
  return out.str();
}

[[nodiscard]] inline std::string
canonical_config_bundle_receipt_text(config_bundle_receipt_t receipt) {
  std::sort(receipt.files.begin(), receipt.files.end(),
            [](const auto &a, const auto &b) {
              if (a.config_key != b.config_key) {
                return a.config_key < b.config_key;
              }
              return a.path.lexically_normal().string() <
                     b.path.lexically_normal().string();
            });
  std::ostringstream out;
  out << "schema=" << receipt.schema << "\n";
  out << "global_config_path="
      << receipt.global_config_path.lexically_normal().string() << "\n";
  out << "config_bundle_id=" << receipt.config_bundle_id << "\n";
  out << "config_receipt_id=" << receipt.config_receipt_id << "\n";
  out << "captured_at_utc=" << receipt.captured_at_utc << "\n";
  out << "complete=" << (receipt.complete ? "true" : "false") << "\n";
  out << "file_count=" << receipt.files.size() << "\n";
  for (std::size_t i = 0; i < receipt.files.size(); ++i) {
    const auto prefix = std::string("file_") + std::to_string(i);
    const auto &file = receipt.files[i];
    out << prefix << "_config_key=" << file.config_key << "\n";
    out << prefix << "_path=" << file.path.lexically_normal().string() << "\n";
    out << prefix << "_exists=" << (file.exists ? "true" : "false") << "\n";
    out << prefix
        << "_is_regular_file=" << (file.is_regular_file ? "true" : "false")
        << "\n";
    out << prefix << "_size_known=" << (file.size_known ? "true" : "false")
        << "\n";
    out << prefix << "_size=" << file.size << "\n";
    out << prefix << "_content_digest=" << file.content_digest << "\n";
    out << prefix << "_error=" << file.error << "\n";
  }
  out << "issue_count=" << receipt.issues.size() << "\n";
  for (std::size_t i = 0; i < receipt.issues.size(); ++i) {
    out << "issue_" << i << "=" << receipt.issues[i] << "\n";
  }
  return out.str();
}

[[nodiscard]] inline config_bundle_file_t
make_config_bundle_file(std::string config_key,
                        const std::filesystem::path &raw,
                        const std::filesystem::path &base_dir) {
  namespace fs = std::filesystem;
  fs::path path = raw;
  if (!path.is_absolute()) {
    path = base_dir / path;
  }
  path = detail::normalize_path(path);

  config_bundle_file_t out{};
  out.config_key = std::move(config_key);
  out.path = path;
  std::error_code ec;
  out.exists = fs::exists(path, ec) && !ec;
  if (!out.exists) {
    out.error = ec ? ec.message() : "missing";
    return out;
  }
  std::error_code type_ec;
  out.is_regular_file = fs::is_regular_file(path, type_ec) && !type_ec;
  if (!out.is_regular_file) {
    out.error = type_ec ? type_ec.message() : "not a regular file";
    return out;
  }
  std::error_code size_ec;
  out.size = fs::file_size(path, size_ec);
  out.size_known = !size_ec;
  if (size_ec) {
    out.error = size_ec.message();
  }
  try {
    out.content_digest =
        detail::digest_text("kikijyeba.config.bundle_file_content.v1",
                            detail::read_text_file(path));
  } catch (const std::exception &ex) {
    out.error = ex.what();
  }
  return out;
}

[[nodiscard]] inline config_bundle_receipt_t
capture_config_bundle_receipt(const std::filesystem::path &global_config_path,
                              std::string receipt_nonce = {}) {
  namespace fs = std::filesystem;
  config_bundle_receipt_t out{};
  out.global_config_path = detail::normalize_path(global_config_path);
  out.captured_at_utc = detail::now_utc_token();
  if (receipt_nonce.empty()) {
    receipt_nonce = out.captured_at_utc + "|" +
                    std::to_string(std::chrono::high_resolution_clock::now()
                                       .time_since_epoch()
                                       .count());
  }

  const fs::path base_dir = out.global_config_path.parent_path();
  out.files.push_back(make_config_bundle_file(
      "GLOBAL_CONFIG", out.global_config_path, base_dir));

  std::string config_text;
  try {
    config_text = detail::read_text_file(out.global_config_path);
  } catch (const std::exception &ex) {
    out.issues.push_back(ex.what());
  }

  std::string section;
  std::istringstream lines(config_text);
  std::string line;
  while (std::getline(lines, line)) {
    const std::string trimmed =
        detail::trim_ascii(detail::strip_ini_comment(line));
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      section = detail::trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    const auto eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = detail::trim_ascii(trimmed.substr(0, eq));
    const std::string value = detail::trim_ascii(trimmed.substr(eq + 1));
    if (!detail::has_suffix(key, "_path") &&
        !detail::has_suffix(key, "_filename")) {
      continue;
    }
    const std::string qualified_key =
        section.empty() ? key : section + "." + key;
    out.files.push_back(
        make_config_bundle_file(qualified_key, fs::path(value), base_dir));
  }

  std::set<std::string> seen;
  std::vector<config_bundle_file_t> unique_files;
  unique_files.reserve(out.files.size());
  for (auto &file : out.files) {
    const std::string key =
        file.config_key + "\n" + file.path.lexically_normal().string();
    if (seen.insert(key).second) {
      if (!file.exists || !file.is_regular_file ||
          file.content_digest.empty()) {
        out.issues.push_back(file.config_key + "=" +
                             file.path.lexically_normal().string() + ": " +
                             (file.error.empty() ? "unreadable" : file.error));
      }
      unique_files.push_back(std::move(file));
    }
  }
  out.files = std::move(unique_files);
  out.complete = out.issues.empty();
  out.config_bundle_id = detail::digest_text(
      "kikijyeba.config.bundle_id.v1", canonical_config_bundle_id_text(out));
  out.config_receipt_id = detail::digest_text(
      "kikijyeba.config.bundle_receipt_id.v1",
      out.config_bundle_id + "\n" + out.captured_at_utc + "\n" + receipt_nonce +
          "\n" + out.global_config_path.string());
  return out;
}

struct component_spawn_tuple_t {
  std::string schema{"kikijyeba.component_spawn.v2"};
  std::string component_family_id{};
  std::string protocol_contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string component_assembly_fingerprint{};
};

[[nodiscard]] inline std::string
canonical_component_spawn_text(const component_spawn_tuple_t &spawn) {
  std::ostringstream out;
  out << "schema=" << spawn.schema << "\n";
  out << "component_family_id=" << spawn.component_family_id << "\n";
  out << "protocol_contract_fingerprint=" << spawn.protocol_contract_fingerprint
      << "\n";
  out << "graph_order_fingerprint=" << spawn.graph_order_fingerprint << "\n";
  out << "component_assembly_fingerprint="
      << spawn.component_assembly_fingerprint << "\n";
  return out.str();
}

[[nodiscard]] inline std::string
component_spawn_fingerprint(const component_spawn_tuple_t &spawn) {
  return detail::digest_text("kikijyeba.component_spawn.v2",
                             canonical_component_spawn_text(spawn));
}

struct component_spawn_registry_entry_t {
  std::string component_spawn_registry_id{};
  std::string component_family_id{};
  std::string component_spawn_id{};
  std::string component_spawn_label{};
  std::string component_spawn_fingerprint{};
  std::string config_bundle_id{};
  std::string config_receipt_id{};
  std::string first_seen_runtime_root{};
  std::string first_seen_timestamp{};
  std::string last_seen_timestamp{};
};

struct component_spawn_registry_t {
  std::string schema{"kikijyeba.component_spawn_registry.v1"};
  std::string component_spawn_registry_id{};
  std::vector<component_spawn_registry_entry_t> entries{};
};

[[nodiscard]] inline std::string component_spawn_registry_id_for_runtime_root(
    const std::filesystem::path &runtime_root) {
  return detail::digest_text("kikijyeba.component_spawn_registry_id.v1",
                             detail::normalize_path(runtime_root).string());
}

[[nodiscard]] inline std::filesystem::path
component_spawn_registry_path(const std::filesystem::path &runtime_root) {
  return runtime_root / "system" / "component_spawn_registry.v1.lls";
}

[[nodiscard]] inline std::string
canonical_component_spawn_registry_text(component_spawn_registry_t registry) {
  std::sort(
      registry.entries.begin(), registry.entries.end(),
      [](const auto &a, const auto &b) {
        if (a.component_spawn_registry_id != b.component_spawn_registry_id) {
          return a.component_spawn_registry_id < b.component_spawn_registry_id;
        }
        if (a.component_family_id != b.component_family_id) {
          return a.component_family_id < b.component_family_id;
        }
        return a.component_spawn_fingerprint < b.component_spawn_fingerprint;
      });
  std::ostringstream out;
  out << "schema=" << registry.schema << "\n";
  out << "component_spawn_registry_id=" << registry.component_spawn_registry_id
      << "\n";
  out << "entry_count=" << registry.entries.size() << "\n";
  for (std::size_t i = 0; i < registry.entries.size(); ++i) {
    const auto prefix = std::string("entry_") + std::to_string(i);
    const auto &entry = registry.entries[i];
    out << prefix
        << "_component_spawn_registry_id=" << entry.component_spawn_registry_id
        << "\n";
    out << prefix << "_component_family_id=" << entry.component_family_id
        << "\n";
    out << prefix << "_component_spawn_id=" << entry.component_spawn_id << "\n";
    out << prefix << "_component_spawn_label=" << entry.component_spawn_label
        << "\n";
    out << prefix
        << "_component_spawn_fingerprint=" << entry.component_spawn_fingerprint
        << "\n";
    out << prefix << "_config_bundle_id=" << entry.config_bundle_id << "\n";
    out << prefix << "_config_receipt_id=" << entry.config_receipt_id << "\n";
    out << prefix
        << "_first_seen_runtime_root=" << entry.first_seen_runtime_root << "\n";
    out << prefix << "_first_seen_timestamp=" << entry.first_seen_timestamp
        << "\n";
    out << prefix << "_last_seen_timestamp=" << entry.last_seen_timestamp
        << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::uint64_t parse_hex_u64(std::string_view hex) {
  std::uint64_t out = 0;
  for (const char c : hex) {
    out <<= 4;
    if (c >= '0' && c <= '9') {
      out |= static_cast<std::uint64_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      out |= static_cast<std::uint64_t>(10 + c - 'a');
    } else if (c >= 'A' && c <= 'F') {
      out |= static_cast<std::uint64_t>(10 + c - 'A');
    }
  }
  return out;
}

[[nodiscard]] inline std::string base26_token(std::uint64_t value,
                                              std::size_t length) {
  std::string out(length, 'a');
  for (std::size_t i = 0; i < length; ++i) {
    out[length - 1 - i] = static_cast<char>('a' + (value % 26u));
    value /= 26u;
  }
  return out;
}

[[nodiscard]] inline std::string
component_spawn_id_candidate(const std::string &component_spawn_fingerprint,
                             std::size_t attempt, std::size_t length = 3) {
  const auto seed = detail::digest_text(
      "kikijyeba.component_spawn_id_candidate.v1",
      component_spawn_fingerprint + "\n" + std::to_string(attempt));
  return base26_token(parse_hex_u64(seed), length);
}

inline void assign_component_spawn_ids(
    std::vector<component_spawn_registry_entry_t> *entries) {
  if (entries == nullptr) {
    return;
  }
  for (auto &entry : *entries) {
    entry.component_spawn_id.clear();
    entry.component_spawn_label.clear();
  }
  std::vector<std::size_t> order(entries->size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
    const auto &a = (*entries)[lhs];
    const auto &b = (*entries)[rhs];
    if (a.component_spawn_registry_id != b.component_spawn_registry_id) {
      return a.component_spawn_registry_id < b.component_spawn_registry_id;
    }
    if (a.component_family_id != b.component_family_id) {
      return a.component_family_id < b.component_family_id;
    }
    return a.component_spawn_fingerprint < b.component_spawn_fingerprint;
  });
  std::unordered_map<std::string, std::set<std::string>> used_by_scope;
  for (const std::size_t index : order) {
    auto &entry = (*entries)[index];
    if (entry.component_spawn_fingerprint.empty()) {
      continue;
    }
    const std::string scope =
        entry.component_spawn_registry_id + "\n" + entry.component_family_id;
    auto &used = used_by_scope[scope];
    for (std::size_t length = 3; length <= 4; ++length) {
      for (std::size_t attempt = 0; attempt < 256; ++attempt) {
        const std::string candidate = component_spawn_id_candidate(
            entry.component_spawn_fingerprint, attempt, length);
        if (used.insert(candidate).second) {
          entry.component_spawn_id = candidate;
          entry.component_spawn_label =
              entry.component_family_id + "#" + entry.component_spawn_id;
          break;
        }
      }
      if (!entry.component_spawn_id.empty()) {
        break;
      }
    }
    if (entry.component_spawn_id.empty()) {
      throw std::runtime_error(
          "[config_provenance] unable to assign component_spawn_id");
    }
  }
}

[[nodiscard]] inline component_spawn_registry_t
read_component_spawn_registry(const std::filesystem::path &runtime_root) {
  const auto path = component_spawn_registry_path(runtime_root);
  component_spawn_registry_t registry{};
  registry.component_spawn_registry_id =
      component_spawn_registry_id_for_runtime_root(runtime_root);
  const auto text = detail::read_text_file_if_exists(path);
  if (!text.has_value()) {
    return registry;
  }
  const auto map = detail::parse_assignment_text(*text);
  registry.schema = detail::map_get(map, "schema", registry.schema);
  registry.component_spawn_registry_id = detail::map_get(
      map, "component_spawn_registry_id", registry.component_spawn_registry_id);
  const auto count_text = detail::map_get(map, "entry_count", "0");
  std::int64_t count = 0;
  try {
    count = std::stoll(count_text);
  } catch (...) {
    count = 0;
  }
  for (std::int64_t i = 0; i < count; ++i) {
    const auto prefix = std::string("entry_") + std::to_string(i);
    component_spawn_registry_entry_t entry{};
    entry.component_spawn_registry_id =
        detail::map_get(map, prefix + "_component_spawn_registry_id");
    entry.component_family_id =
        detail::map_get(map, prefix + "_component_family_id");
    entry.component_spawn_id =
        detail::map_get(map, prefix + "_component_spawn_id");
    entry.component_spawn_label =
        detail::map_get(map, prefix + "_component_spawn_label");
    entry.component_spawn_fingerprint =
        detail::map_get(map, prefix + "_component_spawn_fingerprint");
    entry.config_bundle_id = detail::map_get(map, prefix + "_config_bundle_id");
    entry.config_receipt_id =
        detail::map_get(map, prefix + "_config_receipt_id");
    entry.first_seen_runtime_root =
        detail::map_get(map, prefix + "_first_seen_runtime_root");
    entry.first_seen_timestamp =
        detail::map_get(map, prefix + "_first_seen_timestamp");
    entry.last_seen_timestamp =
        detail::map_get(map, prefix + "_last_seen_timestamp");
    if (entry.component_spawn_registry_id.empty()) {
      entry.component_spawn_registry_id = registry.component_spawn_registry_id;
    }
    if (!entry.component_family_id.empty() &&
        !entry.component_spawn_fingerprint.empty()) {
      registry.entries.push_back(std::move(entry));
    }
  }
  assign_component_spawn_ids(&registry.entries);
  return registry;
}

inline void
write_component_spawn_registry(const std::filesystem::path &runtime_root,
                               const component_spawn_registry_t &registry) {
  detail::write_text_file_atomic(
      component_spawn_registry_path(runtime_root),
      canonical_component_spawn_registry_text(registry));
}

[[nodiscard]] inline component_spawn_registry_entry_t
upsert_component_spawn_registry_entry(const std::filesystem::path &runtime_root,
                                      component_spawn_registry_entry_t entry) {
  auto registry = read_component_spawn_registry(runtime_root);
  entry.component_spawn_registry_id = registry.component_spawn_registry_id;
  entry.first_seen_runtime_root = detail::normalize_path(runtime_root).string();
  const auto now = detail::now_utc_token();
  if (entry.first_seen_timestamp.empty()) {
    entry.first_seen_timestamp = now;
  }
  entry.last_seen_timestamp = now;

  bool replaced = false;
  for (auto &existing : registry.entries) {
    if (existing.component_spawn_registry_id ==
            entry.component_spawn_registry_id &&
        existing.component_family_id == entry.component_family_id &&
        existing.component_spawn_fingerprint ==
            entry.component_spawn_fingerprint) {
      if (!existing.first_seen_timestamp.empty()) {
        entry.first_seen_timestamp = existing.first_seen_timestamp;
      }
      if (!existing.first_seen_runtime_root.empty()) {
        entry.first_seen_runtime_root = existing.first_seen_runtime_root;
      }
      existing = entry;
      replaced = true;
      break;
    }
  }
  if (!replaced) {
    registry.entries.push_back(entry);
  }
  assign_component_spawn_ids(&registry.entries);
  for (const auto &existing : registry.entries) {
    if (existing.component_spawn_registry_id ==
            entry.component_spawn_registry_id &&
        existing.component_family_id == entry.component_family_id &&
        existing.component_spawn_fingerprint ==
            entry.component_spawn_fingerprint) {
      entry = existing;
      break;
    }
  }
  write_component_spawn_registry(runtime_root, registry);
  return entry;
}

struct component_spawn_resolution_t {
  bool resolved{false};
  std::string issue{};
  component_spawn_registry_entry_t entry{};
  std::vector<component_spawn_registry_entry_t> matches{};
};

[[nodiscard]] inline component_spawn_resolution_t
resolve_component_spawn_id(const std::filesystem::path &runtime_root,
                           const std::string &component_spawn_registry_id,
                           const std::string &component_family_id,
                           const std::string &component_spawn_id) {
  component_spawn_resolution_t out{};
  if (component_spawn_id.empty()) {
    out.issue = "empty component_spawn_id";
    return out;
  }
  const auto registry = read_component_spawn_registry(runtime_root);
  for (const auto &entry : registry.entries) {
    if (entry.component_spawn_registry_id == component_spawn_registry_id &&
        entry.component_family_id == component_family_id &&
        entry.component_spawn_id == component_spawn_id) {
      out.matches.push_back(entry);
    }
  }
  if (out.matches.empty()) {
    out.issue = "component spawn id not found";
    return out;
  }
  if (out.matches.size() != 1) {
    out.issue = "ambiguous component spawn id";
    return out;
  }
  out.entry = out.matches.front();
  if (out.entry.component_spawn_fingerprint.empty()) {
    out.issue = "component spawn fingerprint missing";
    return out;
  }
  if (out.entry.config_bundle_id.empty() ||
      out.entry.config_receipt_id.empty()) {
    out.issue = "config provenance link missing";
    return out;
  }
  out.resolved = true;
  return out;
}

} // namespace cuwacunu::kikijyeba::protocol::config_provenance
