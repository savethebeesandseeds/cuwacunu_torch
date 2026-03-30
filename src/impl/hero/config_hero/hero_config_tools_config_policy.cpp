#include "hero_config_tools_internal.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <unordered_set>
#include <utility>

namespace cuwacunu::hero::mcp::detail {

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  std::error_code ec{};
  if (!base.empty()) {
    const auto c = std::filesystem::weakly_canonical(base, ec);
    if (!ec) base = c;
  }
  ec.clear();
  if (!candidate.empty()) {
    const auto c = std::filesystem::weakly_canonical(candidate, ec);
    if (!ec) candidate = c;
  }
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c) return false;
  }
  return b == base.end();
}

[[nodiscard]] bool is_scope_violation(dsl_path_resolution_error_t err) {
  return err == dsl_path_resolution_error_t::kEscapesScope ||
         err == dsl_path_resolution_error_t::kHashimyeiPath ||
         err == dsl_path_resolution_error_t::kNotDefaultInstructionDsl ||
         err == dsl_path_resolution_error_t::kObjectiveRootEscapesAllowedScopes ||
         err == dsl_path_resolution_error_t::kNotObjectiveInstructionRoot ||
         err == dsl_path_resolution_error_t::kEscapesObjectiveRoot ||
         err == dsl_path_resolution_error_t::kNotObjectiveInstructionDsl;
}

[[nodiscard]] bool path_has_component(const std::filesystem::path& path,
                                      std::string_view component) {
  for (const auto& part : path.lexically_normal()) {
    if (part == component) return true;
  }
  return false;
}

[[nodiscard]] bool is_campaign_dsl_path(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename == "campaign.dsl" ||
         (filename.size() >= std::string(".campaign.dsl").size() &&
          filename.compare(filename.size() - std::string(".campaign.dsl").size(),
                           std::string(".campaign.dsl").size(),
                           ".campaign.dsl") == 0);
}

[[nodiscard]] bool is_super_objective_dsl_path(
    const std::filesystem::path& path) {
  const std::string filename = lowercase_copy(path.filename().string());
  if (filename == "super.objective.dsl" ||
      filename == "default.super.objective.dsl") {
    return true;
  }
  const std::string suffix = ".super.dsl";
  if (filename.size() >= suffix.size() &&
      filename.compare(filename.size() - suffix.size(), suffix.size(),
                       suffix) == 0) {
    return filename.find(".hero.super.dsl") == std::string::npos;
  }
  return false;
}

[[nodiscard]] bool split_csv_tokens(std::string_view raw,
                                    std::vector<std::string>* out_tokens) {
  if (!out_tokens) return false;
  out_tokens->clear();
  const std::string trimmed = trim_ascii(raw);
  std::size_t start = 0;
  while (start <= trimmed.size()) {
    const std::size_t comma = trimmed.find(',', start);
    const std::string token = trim_ascii(trimmed.substr(start, comma - start));
    if (!token.empty()) out_tokens->push_back(token);
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return !out_tokens->empty();
}

[[nodiscard]] bool collect_configured_root_paths(
    const hero_config_store_t& store, std::string_view key,
    std::vector<std::filesystem::path>* out_roots, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_roots) {
    if (out_error) *out_error = "configured roots output pointer is null";
    return false;
  }
  out_roots->clear();

  std::vector<std::string> tokens{};
  if (!split_csv_tokens(store.get_or_default(std::string(key)), &tokens)) {
    if (out_error) *out_error = std::string(key) + " is empty";
    return false;
  }
  for (const auto& token : tokens) {
    const std::filesystem::path resolved =
        resolve_path_near_config(token, store.config_path()).lexically_normal();
    if (!resolved.empty()) out_roots->push_back(resolved);
  }
  if (out_roots->empty()) {
    if (out_error) *out_error = std::string(key) + " resolved to no usable roots";
    return false;
  }
  return true;
}

[[nodiscard]] bool collect_allowed_extensions(
    const hero_config_store_t& store,
    std::vector<std::string>* out_extensions, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_extensions) {
    if (out_error) *out_error = "allowed extensions output pointer is null";
    return false;
  }
  out_extensions->clear();
  if (!split_csv_tokens(store.get_or_default("allowed_extensions"),
                        out_extensions)) {
    if (out_error) *out_error = "allowed_extensions is empty";
    return false;
  }
  for (auto& ext : *out_extensions) {
    ext = lowercase_copy(ext);
    if (ext.size() < 2 || ext[0] != '.') {
      if (out_error) {
        *out_error = "allowed_extensions entries must be dot-prefixed: " + ext;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool path_has_allowed_extension(
    const std::filesystem::path& path,
    const std::vector<std::string>& allowed_extensions) {
  const std::string ext = lowercase_copy(path.extension().string());
  for (const auto& allowed : allowed_extensions) {
    if (ext == allowed) return true;
  }
  return false;
}

[[nodiscard]] bool path_is_within_any_root(
    const std::vector<std::filesystem::path>& roots,
    const std::filesystem::path& candidate) {
  for (const auto& root : roots) {
    if (path_is_within(root, candidate)) return true;
  }
  return false;
}

[[nodiscard]] bool resolve_default_dsl_path_with_scope(
    const hero_config_store_t& store, std::string_view request_path,
    std::filesystem::path* out_path, std::string* out_error,
    dsl_path_resolution_error_t* out_reason) {
  if (out_error) out_error->clear();
  if (out_reason) *out_reason = dsl_path_resolution_error_t::kNone;
  if (!out_path) {
    if (out_error) *out_error = "dsl path output pointer is null";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kOutputPointerNull;
    return false;
  }
  *out_path = std::filesystem::path{};

  const std::string raw = trim_ascii(request_path);
  if (raw.empty()) {
    if (out_error) *out_error = "dsl path is empty";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEmptyPath;
    return false;
  }
  const std::filesystem::path resolved =
      resolve_path_near_config(raw, store.config_path()).lexically_normal();
  if (path_has_component(resolved, ".hashimyei")) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": dsl path under .hashimyei is not allowed: " +
                   resolved.string();
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kHashimyeiPath;
    return false;
  }
  std::vector<std::filesystem::path> default_roots{};
  std::string roots_error{};
  if (!collect_configured_root_paths(store, "default_roots", &default_roots,
                                     &roots_error)) {
    if (out_error) *out_error = roots_error;
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEscapesScope;
    return false;
  }
  std::vector<std::string> allowed_extensions{};
  std::string extensions_error{};
  if (!collect_allowed_extensions(store, &allowed_extensions,
                                  &extensions_error)) {
    if (out_error) *out_error = extensions_error;
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kNotDefaultInstructionDsl;
    return false;
  }
  if (!path_is_within_any_root(default_roots, resolved) ||
      !path_has_allowed_extension(resolved, allowed_extensions)) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": dsl path must stay within configured default_roots and use an allowed extension: " +
                   resolved.string();
    }
    if (out_reason) {
      *out_reason = dsl_path_resolution_error_t::kNotDefaultInstructionDsl;
    }
    return false;
  }

  *out_path = resolved;
  return true;
}

[[nodiscard]] bool resolve_objective_root_with_scope(
    const hero_config_store_t& store, std::string_view request_root,
    std::filesystem::path* out_root, std::string* out_error,
    dsl_path_resolution_error_t* out_reason) {
  if (out_error) out_error->clear();
  if (out_reason) *out_reason = dsl_path_resolution_error_t::kNone;
  if (!out_root) {
    if (out_error) *out_error = "objective_root output pointer is null";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kOutputPointerNull;
    return false;
  }
  *out_root = std::filesystem::path{};

  const std::string raw_root = trim_ascii(request_root);
  if (raw_root.empty()) {
    if (out_error) *out_error = "objective_root is empty";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEmptyObjectiveRoot;
    return false;
  }

  const std::filesystem::path resolved =
      resolve_path_near_config(raw_root, store.config_path()).lexically_normal();
  if (path_has_component(resolved, ".hashimyei")) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective_root under .hashimyei is not allowed: " +
                   resolved.string();
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kHashimyeiPath;
    return false;
  }

  std::vector<std::filesystem::path> objective_roots{};
  std::string roots_error{};
  if (!collect_configured_root_paths(store, "objective_roots", &objective_roots,
                                     &roots_error)) {
    if (out_error) *out_error = roots_error;
    if (out_reason) {
      *out_reason =
          dsl_path_resolution_error_t::kObjectiveRootEscapesAllowedScopes;
    }
    return false;
  }
  if (!path_is_within_any_root(objective_roots, resolved)) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective_root must stay within configured objective_roots: " +
                   resolved.string();
    }
    if (out_reason) {
      *out_reason =
          dsl_path_resolution_error_t::kObjectiveRootEscapesAllowedScopes;
    }
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(resolved, ec) ||
      !std::filesystem::is_directory(resolved, ec)) {
    if (out_error) {
      *out_error = "objective_root is not an existing directory: " +
                   resolved.string();
    }
    if (out_reason) {
      *out_reason = dsl_path_resolution_error_t::kObjectiveRootMissing;
    }
    return false;
  }

  *out_root = resolved;
  return true;
}

[[nodiscard]] bool resolve_objective_path_with_scope(
    const hero_config_store_t& store, std::string_view request_root,
    std::string_view request_path, bool allow_missing_target,
    std::filesystem::path* out_path, std::string* out_error,
    dsl_path_resolution_error_t* out_reason) {
  if (out_error) out_error->clear();
  if (out_reason) *out_reason = dsl_path_resolution_error_t::kNone;
  if (!out_path) {
    if (out_error) *out_error = "objective path output pointer is null";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kOutputPointerNull;
    return false;
  }
  *out_path = std::filesystem::path{};

  std::filesystem::path objective_root{};
  if (!resolve_objective_root_with_scope(store, request_root, &objective_root,
                                         out_error, out_reason)) {
    return false;
  }

  const std::string raw_path = trim_ascii(request_path);
  if (raw_path.empty()) {
    if (out_error) *out_error = "objective path is empty";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEmptyPath;
    return false;
  }
  const std::filesystem::path rel_path(raw_path);
  if (rel_path.is_absolute()) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective path must be relative to objective_root: " +
                   raw_path;
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEscapesObjectiveRoot;
    return false;
  }

  const std::filesystem::path resolved =
      (objective_root / rel_path).lexically_normal();
  if (path_has_component(resolved, ".hashimyei")) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective path under .hashimyei is not allowed: " +
                   resolved.string();
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kHashimyeiPath;
    return false;
  }
  if (!path_is_within(objective_root, resolved)) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective path escapes objective_root: " +
                   resolved.string();
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEscapesObjectiveRoot;
    return false;
  }
  std::vector<std::string> allowed_extensions{};
  std::string extensions_error{};
  if (!collect_allowed_extensions(store, &allowed_extensions,
                                  &extensions_error)) {
    if (out_error) *out_error = extensions_error;
    if (out_reason) {
      *out_reason = dsl_path_resolution_error_t::kNotObjectiveInstructionDsl;
    }
    return false;
  }
  if (!path_has_allowed_extension(resolved, allowed_extensions)) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": objective path must use an allowed extension: " +
                   resolved.string();
    }
    if (out_reason) {
      *out_reason = dsl_path_resolution_error_t::kNotObjectiveInstructionDsl;
    }
    return false;
  }
  if (!allow_missing_target) {
    std::error_code ec{};
    if (!std::filesystem::exists(resolved, ec) ||
        !std::filesystem::is_regular_file(resolved, ec)) {
      if (out_error) {
        *out_error = "objective file does not exist: " + resolved.string();
      }
      if (out_reason) {
        *out_reason = dsl_path_resolution_error_t::kNotObjectiveInstructionDsl;
      }
      return false;
    }
  }

  *out_path = resolved;
  return true;
}

[[nodiscard]] std::string make_write_policy_error(std::string_view detail) {
  return std::string(kConfigWritePolicyErrorTag) + ": " + std::string(detail);
}

[[nodiscard]] bool read_bool_config_key_or_default(
    const hero_config_store_t& store, std::string_view key, bool default_value,
    bool* out_value, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_value) {
    if (out_error) *out_error = "missing destination for bool config value";
    return false;
  }

  *out_value = default_value;
  const std::string raw = trim_ascii(store.get_or_default(std::string(key)));
  if (raw.empty()) return true;
  if (parse_bool_ascii(raw, out_value)) return true;

  if (out_error) {
    *out_error =
        make_write_policy_error("invalid " + std::string(key) + " value: " + raw);
  }
  return false;
}

[[nodiscard]] bool collect_allowed_write_roots(
    const hero_config_store_t& store,
    std::vector<std::filesystem::path>* out_roots, std::string* out_error) {
  if (out_error) out_error->clear();
  bool allow_local_write = false;
  const std::string allow_raw = trim_ascii(store.get_or_default("allow_local_write"));
  if (!parse_bool_ascii(allow_raw, &allow_local_write)) {
    if (out_error) {
      *out_error =
          make_write_policy_error("invalid allow_local_write value: " + allow_raw);
    }
    return false;
  }
  if (!allow_local_write) {
    if (out_error) {
      *out_error =
          make_write_policy_error("local filesystem mutation denied: allow_local_write=false");
    }
    return false;
  }
  if (!out_roots) return true;

  out_roots->clear();
  const std::string roots_raw = trim_ascii(store.get_or_default("write_roots"));
  if (roots_raw.empty()) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "local filesystem mutation denied: write_roots is empty");
    }
    return false;
  }

  std::size_t start = 0;
  while (start <= roots_raw.size()) {
    const std::size_t comma = roots_raw.find(',', start);
    const std::string token =
        trim_ascii(roots_raw.substr(start, comma - start));
    if (!token.empty()) {
      const std::filesystem::path resolved =
          resolve_path_near_config(token, store.config_path());
      if (!resolved.empty()) {
        out_roots->push_back(resolved.lexically_normal());
      }
    }
    if (comma == std::string::npos) break;
    start = comma + 1;
  }

  if (out_roots->empty()) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "local filesystem mutation denied: write_roots resolved to no usable paths");
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool list_instruction_files_under_root(
    const std::filesystem::path& root,
    const std::vector<std::string>& allowed_extensions,
    std::vector<std::filesystem::path>* out_paths, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_paths) {
    if (out_error) *out_error = "listed paths output pointer is null";
    return false;
  }
  out_paths->clear();

  std::error_code ec{};
  if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
    if (out_error) *out_error = "listing root is not an existing directory: " + root.string();
    return false;
  }

  std::unordered_set<std::string> dedup{};
  const auto options = std::filesystem::directory_options::skip_permission_denied;
  std::filesystem::recursive_directory_iterator it(root, options, ec);
  std::filesystem::recursive_directory_iterator end{};
  if (ec) {
    if (out_error) {
      *out_error = "failed to open listing root: " + root.string();
    }
    return false;
  }

  for (; it != end; it.increment(ec)) {
    if (ec) {
      if (out_error) {
        *out_error =
            "failed while listing files under root: " + root.string();
      }
      return false;
    }
    const auto& entry = *it;
    std::error_code status_ec{};
    if (!entry.is_regular_file(status_ec) || status_ec) continue;
    const std::filesystem::path path = entry.path().lexically_normal();
    if (!path_is_within(root, path)) continue;
    if (!path_has_allowed_extension(path, allowed_extensions)) continue;
    if (path_has_component(path, ".hashimyei")) continue;
    if (!dedup.insert(path.string()).second) continue;
    out_paths->push_back(path);
  }

  std::sort(out_paths->begin(), out_paths->end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return a.string() < b.string();
            });
  return true;
}

[[nodiscard]] bool enforce_write_target_allowed(
    const hero_config_store_t& store, const std::filesystem::path& raw_target,
    std::string* out_error) {
  if (out_error) out_error->clear();
  std::vector<std::filesystem::path> allowed_roots{};
  std::string err{};
  if (!collect_allowed_write_roots(store, &allowed_roots, &err)) {
    if (out_error) *out_error = std::move(err);
    return false;
  }

  std::filesystem::path target = raw_target;
  if (!target.is_absolute()) {
    target = resolve_path_near_config(target.string(), store.config_path());
  }
  target = target.lexically_normal();
  for (const auto& root : allowed_roots) {
    if (path_is_within(root, target)) return true;
  }

  if (out_error) {
    *out_error = make_write_policy_error("write target escapes write_roots: " +
                                         target.string());
  }
  return false;
}

[[nodiscard]] bool parse_int64_ascii(std::string_view raw, std::int64_t* out) {
  if (!out) return false;
  const std::string trimmed = trim_ascii(raw);
  if (trimmed.empty()) return false;
  std::int64_t value = 0;
  const auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
  if (ec != std::errc{} || ptr != trimmed.data() + trimmed.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] bool read_int64_config_key_or_default(
    const hero_config_store_t& store, std::string_view key,
    std::int64_t default_value, std::int64_t* out_value,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_value) {
    if (out_error) *out_error = "missing destination for int config value";
    return false;
  }

  *out_value = default_value;
  const std::string raw = trim_ascii(store.get_or_default(std::string(key)));
  if (raw.empty()) return true;
  if (parse_int64_ascii(raw, out_value)) return true;

  if (out_error) {
    *out_error =
        make_write_policy_error("invalid " + std::string(key) + " value: " + raw);
  }
  return false;
}

[[nodiscard]] bool enforce_runtime_config_write_policy(
    const hero_config_store_t& store, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!enforce_write_target_allowed(store, store.config_path(), out_error)) {
    return false;
  }

  bool backup_enabled = true;
  const std::string backup_enabled_raw =
      trim_ascii(store.get_or_default("backup_enabled"));
  if (!backup_enabled_raw.empty() &&
      !parse_bool_ascii(backup_enabled_raw, &backup_enabled)) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "invalid backup_enabled value: " + backup_enabled_raw);
    }
    return false;
  }
  if (!backup_enabled) return true;

  const std::string backup_dir_raw = trim_ascii(store.get_or_default("backup_dir"));
  if (backup_dir_raw.empty()) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "backup_dir must be non-empty when backup_enabled=true");
    }
    return false;
  }
  const std::filesystem::path backup_dir =
      resolve_path_near_config(backup_dir_raw, store.config_path());
  if (!enforce_write_target_allowed(store, backup_dir, out_error)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool file_name_has_prefix(std::string_view value,
                                        std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

[[nodiscard]] bool backup_previous_write_target_with_cap(
    const hero_config_store_t& store, const std::filesystem::path& source_path,
    std::string* out_error) {
  if (out_error) out_error->clear();

  bool backup_enabled = true;
  const std::string backup_enabled_raw =
      trim_ascii(store.get_or_default("backup_enabled"));
  if (!backup_enabled_raw.empty() &&
      !parse_bool_ascii(backup_enabled_raw, &backup_enabled)) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "invalid backup_enabled value: " + backup_enabled_raw);
    }
    return false;
  }
  if (!backup_enabled) return true;

  std::int64_t backup_max_entries = 20;
  const std::string backup_max_entries_raw =
      trim_ascii(store.get_or_default("backup_max_entries"));
  if (!backup_max_entries_raw.empty() &&
      !parse_int64_ascii(backup_max_entries_raw, &backup_max_entries)) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "invalid backup_max_entries value: " + backup_max_entries_raw);
    }
    return false;
  }
  if (backup_max_entries < 1) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "backup_max_entries must be >= 1 when backup_enabled=true");
    }
    return false;
  }

  const std::string backup_dir_raw = trim_ascii(store.get_or_default("backup_dir"));
  if (backup_dir_raw.empty()) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "backup_dir must be non-empty when backup_enabled=true");
    }
    return false;
  }
  const std::filesystem::path backup_dir =
      resolve_path_near_config(backup_dir_raw, store.config_path());
  if (!enforce_write_target_allowed(store, backup_dir, out_error)) return false;

  std::error_code ec{};
  if (!std::filesystem::exists(source_path, ec)) {
    if (ec) {
      if (out_error) {
        *out_error = "failed to inspect source file for backup: " +
                     source_path.string();
      }
      return false;
    }
    return true;
  }
  if (!std::filesystem::is_regular_file(source_path, ec)) {
    if (ec) {
      if (out_error) {
        *out_error = "failed to inspect source file for backup: " +
                     source_path.string();
      }
      return false;
    }
    return true;
  }

  std::filesystem::create_directories(backup_dir, ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to create backup directory: " + backup_dir.string();
    }
    return false;
  }

  const std::string prefix = source_path.filename().string() + ".bak.";
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  std::filesystem::path backup_path =
      backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)) + ".dsl");
  int disambiguator = 1;
  while (std::filesystem::exists(backup_path, ec)) {
    if (ec) {
      if (out_error) {
        *out_error = "failed to probe backup path: " + backup_path.string();
      }
      return false;
    }
    backup_path =
        backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)) +
                      "." + std::to_string(disambiguator++) + ".dsl");
  }

  std::filesystem::copy_file(source_path, backup_path,
                             std::filesystem::copy_options::none, ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to write backup file: " + backup_path.string();
    }
    return false;
  }

  std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>>
      backups{};
  const std::filesystem::directory_iterator begin(backup_dir, ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to enumerate backup directory: " + backup_dir.string();
    }
    return false;
  }
  for (const auto& entry : begin) {
    if (!entry.is_regular_file(ec) || ec) continue;
    const std::string name = entry.path().filename().string();
    if (!file_name_has_prefix(name, prefix)) continue;
    const auto mtime = entry.last_write_time(ec);
    if (ec) {
      if (out_error) {
        *out_error = "failed to stat backup file: " + entry.path().string();
      }
      return false;
    }
    backups.push_back({mtime, entry.path()});
  }

  std::sort(backups.begin(), backups.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.first == rhs.first) return lhs.second.string() < rhs.second.string();
    return lhs.first < rhs.first;
  });

  const std::size_t keep = static_cast<std::size_t>(backup_max_entries);
  while (backups.size() > keep) {
    const std::filesystem::path doomed = backups.front().second;
    backups.erase(backups.begin());
    std::filesystem::remove(doomed, ec);
    if (ec) {
      if (out_error) {
        *out_error = "failed to prune backup file: " + doomed.string();
      }
      return false;
    }
  }
  return true;
}

}  // namespace cuwacunu::hero::mcp::detail
