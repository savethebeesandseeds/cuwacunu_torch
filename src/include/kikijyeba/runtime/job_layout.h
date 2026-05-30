// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu::kikijyeba::runtime::job_layout {

namespace fs = std::filesystem;

inline constexpr const char *k_runtime_layout_schema_v1 =
    "kikijyeba.runtime.layout.v1";
inline constexpr const char *k_component_spawn_ref_schema_v1 =
    "kikijyeba.runtime.component_spawn_ref.v1";

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

[[nodiscard]] inline std::string sanitize_path_component(std::string value) {
  value = trim_ascii(value);
  if (value.empty()) {
    return "unknown";
  }
  for (auto &ch : value) {
    const auto u = static_cast<unsigned char>(ch);
    if (!(std::isalnum(u) || ch == '_' || ch == '-' || ch == '.')) {
      ch = '_';
    }
  }
  return value;
}

[[nodiscard]] inline fs::path system_dir(const fs::path &runtime_root) {
  return runtime_root / "system";
}

[[nodiscard]] inline fs::path components_dir(const fs::path &runtime_root) {
  return runtime_root / "components";
}

[[nodiscard]] inline fs::path indexes_dir(const fs::path &runtime_root) {
  return runtime_root / "indexes";
}

[[nodiscard]] inline fs::path cache_dir(const fs::path &runtime_root) {
  return runtime_root / "cache";
}

[[nodiscard]] inline fs::path
runtime_layout_marker_path(const fs::path &runtime_root) {
  return system_dir(runtime_root) / "runtime_layout.v1.lls";
}

[[nodiscard]] inline std::string
spawn_ref_leaf(const std::string &component_spawn_id,
               const std::string &component_spawn_fingerprint) {
  const std::string id = sanitize_path_component(component_spawn_id.empty()
                                                     ? std::string{"unassigned"}
                                                     : component_spawn_id);
  const std::string fingerprint = sanitize_path_component(
      component_spawn_fingerprint.empty() ? std::string{"unknown_fingerprint"}
                                          : component_spawn_fingerprint);
  return id + "_" + fingerprint;
}

[[nodiscard]] inline fs::path
component_spawn_dir(const fs::path &runtime_root,
                    const std::string &component_family_id,
                    const std::string &component_spawn_id,
                    const std::string &component_spawn_fingerprint) {
  return components_dir(runtime_root) /
         sanitize_path_component(component_family_id) / "spawns" /
         spawn_ref_leaf(component_spawn_id, component_spawn_fingerprint);
}

[[nodiscard]] inline fs::path
job_dir(const fs::path &runtime_root, const std::string &component_family_id,
        const std::string &component_spawn_id,
        const std::string &component_spawn_fingerprint,
        const std::string &wave_action, const std::string &job_id) {
  return component_spawn_dir(runtime_root, component_family_id,
                             component_spawn_id, component_spawn_fingerprint) /
         "jobs" /
         sanitize_path_component(wave_action.empty() ? std::string{"unknown"}
                                                     : wave_action) /
         sanitize_path_component(job_id);
}

[[nodiscard]] inline fs::path
component_spawn_ref_path(const fs::path &runtime_root,
                         const std::string &component_family_id,
                         const std::string &component_spawn_id,
                         const std::string &component_spawn_fingerprint) {
  return component_spawn_dir(runtime_root, component_family_id,
                             component_spawn_id, component_spawn_fingerprint) /
         "component_spawn.ref";
}

inline void write_text_file_atomically(const fs::path &path,
                                       const std::string &text) {
  fs::create_directories(path.parent_path());
  const fs::path tmp = path.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) {
      throw std::runtime_error("[runtime_job_layout] unable to open: " +
                               tmp.string());
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
    throw std::runtime_error("[runtime_job_layout] unable to install: " +
                             path.string() + ": " + ec.message());
  }
}

inline void write_runtime_layout_marker(const fs::path &runtime_root) {
  std::ostringstream out;
  out << "schema=" << k_runtime_layout_schema_v1 << "\n";
  out << "runtime_root=" << runtime_root.lexically_normal().string() << "\n";
  out << "job_layout=components/<component_family>/spawns/"
         "<component_spawn_id>_<component_spawn_fingerprint>/jobs/"
         "<wave_action>/<job_id>\n";
  out << "system_dir=system\n";
  out << "components_dir=components\n";
  out << "indexes_dir=indexes\n";
  out << "cache_dir=cache\n";
  write_text_file_atomically(runtime_layout_marker_path(runtime_root),
                             out.str());
}

inline void write_component_spawn_ref(
    const fs::path &runtime_root, const std::string &component_family_id,
    const std::string &component_spawn_id,
    const std::string &component_spawn_label,
    const std::string &component_spawn_fingerprint,
    const std::string &component_spawn_registry_id,
    const std::string &config_bundle_id, const std::string &config_receipt_id) {
  std::ostringstream out;
  out << "schema=" << k_component_spawn_ref_schema_v1 << "\n";
  out << "component_family_id=" << component_family_id << "\n";
  out << "component_spawn_id=" << component_spawn_id << "\n";
  out << "component_spawn_label=" << component_spawn_label << "\n";
  out << "component_spawn_fingerprint=" << component_spawn_fingerprint << "\n";
  out << "component_spawn_registry_id=" << component_spawn_registry_id << "\n";
  out << "config_bundle_id=" << config_bundle_id << "\n";
  out << "config_receipt_id=" << config_receipt_id << "\n";
  out << "registry_path=system/component_spawn_registry.v1.lls\n";
  write_text_file_atomically(
      component_spawn_ref_path(runtime_root, component_family_id,
                               component_spawn_id, component_spawn_fingerprint),
      out.str());
}

[[nodiscard]] inline std::optional<fs::path>
runtime_root_from_nested_job_dir(const fs::path &candidate_job_dir) {
  fs::path prefix;
  for (const auto &part : candidate_job_dir.lexically_normal()) {
    if (part == "components") {
      return prefix.empty() ? fs::path{} : prefix;
    }
    prefix /= part;
  }
  return std::nullopt;
}

[[nodiscard]] inline fs::path
runtime_root_for_job_dir(const fs::path &candidate_job_dir,
                         const fs::path &fallback_runtime_root) {
  if (const auto root = runtime_root_from_nested_job_dir(candidate_job_dir);
      root.has_value() && !root->empty()) {
    return *root;
  }
  return fallback_runtime_root;
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_kv_file(const fs::path &path) {
  std::unordered_map<std::string, std::string> out;
  std::ifstream in(path);
  if (!in) {
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
map_get(const std::unordered_map<std::string, std::string> &map,
        const std::string &key) {
  const auto it = map.find(key);
  return it == map.end() ? std::string{} : it->second;
}

struct runtime_job_dir_t {
  fs::path dir{};
  fs::path manifest_path{};
  fs::path state_path{};
  fs::path marker_path{};
  fs::file_time_type marker_time{fs::file_time_type::min()};
};

[[nodiscard]] inline bool metadata_tree_leaf(std::string_view leaf) {
  return leaf == "system" || leaf == "indexes" || leaf == "cache" ||
         leaf == "artifacts";
}

[[nodiscard]] inline std::vector<runtime_job_dir_t>
discover_runtime_job_dirs(const fs::path &runtime_root) {
  std::vector<runtime_job_dir_t> out;
  std::error_code ec;
  if (!fs::exists(runtime_root, ec) || ec) {
    return out;
  }
  const auto push_job_dir = [&](const fs::path &dir) {
    const fs::path manifest_path = dir / "job.manifest";
    if (!fs::is_regular_file(manifest_path, ec) || ec) {
      ec.clear();
      return;
    }
    const fs::path state_path = dir / "job.state";
    const fs::path marker_path =
        fs::is_regular_file(state_path, ec) && !ec ? state_path : manifest_path;
    ec.clear();
    auto marker_time = fs::last_write_time(marker_path, ec);
    if (ec) {
      marker_time = fs::file_time_type::min();
      ec.clear();
    }
    out.push_back(runtime_job_dir_t{.dir = dir,
                                    .manifest_path = manifest_path,
                                    .state_path = state_path,
                                    .marker_path = marker_path,
                                    .marker_time = marker_time});
  };

  if (fs::is_directory(runtime_root, ec) && !ec) {
    if (fs::is_regular_file(runtime_root / "job.manifest", ec) && !ec) {
      push_job_dir(runtime_root);
      return out;
    }
    ec.clear();
    const bool marked_runtime_root =
        fs::is_regular_file(runtime_layout_marker_path(runtime_root), ec) &&
        !ec;
    ec.clear();
    const fs::path scan_root =
        marked_runtime_root ? components_dir(runtime_root) : runtime_root;
    if (!fs::exists(scan_root, ec) || ec) {
      ec.clear();
      return out;
    }
    fs::recursive_directory_iterator it(
        scan_root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    for (; !ec && it != end; it.increment(ec)) {
      if (!it->is_directory(ec)) {
        ec.clear();
        continue;
      }
      const auto leaf = it->path().filename().string();
      if (metadata_tree_leaf(leaf)) {
        it.disable_recursion_pending();
        continue;
      }
      if (fs::is_regular_file(it->path() / "job.manifest", ec) && !ec) {
        push_job_dir(it->path());
        it.disable_recursion_pending();
      }
      ec.clear();
    }
  }
  std::sort(out.begin(), out.end(),
            [](const runtime_job_dir_t &a, const runtime_job_dir_t &b) {
              if (a.marker_time != b.marker_time) {
                return a.marker_time > b.marker_time;
              }
              return a.dir.string() < b.dir.string();
            });
  return out;
}

[[nodiscard]] inline std::optional<fs::path>
find_runtime_job_dir_by_id(const fs::path &runtime_root,
                           const std::string &job_id) {
  const std::string wanted = trim_ascii(job_id);
  if (wanted.empty()) {
    return std::nullopt;
  }
  for (const auto &row : discover_runtime_job_dirs(runtime_root)) {
    const auto manifest = parse_kv_file(row.manifest_path);
    const auto observed = map_get(manifest, "job_id");
    const auto stable = map_get(manifest, "job_stable_id");
    const auto attempt = map_get(manifest, "job_attempt_id");
    if (observed == wanted || stable == wanted || attempt == wanted ||
        row.dir.filename().string() == wanted) {
      return row.dir;
    }
  }
  return std::nullopt;
}

} // namespace cuwacunu::kikijyeba::runtime::job_layout
