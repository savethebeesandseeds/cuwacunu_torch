#pragma once

#include <cctype>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace hero {
namespace runtime_dev {

struct runtime_reset_targets_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hashimyei_hero_dsl_path{};
  std::filesystem::path lattice_hero_dsl_path{};
  std::filesystem::path runtime_hero_dsl_path{};
  std::vector<std::filesystem::path> store_roots{};
  std::vector<std::filesystem::path> catalog_paths{};
};

struct runtime_reset_result_t {
  std::vector<std::filesystem::path> removed_store_roots{};
  std::vector<std::filesystem::path> removed_catalog_paths{};
  std::uintmax_t removed_store_entries{0};
};

namespace detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] inline std::string strip_comment_line(std::string_view line,
                                                    bool* in_block_comment) {
  bool block_comment = (in_block_comment != nullptr) ? *in_block_comment : false;
  bool in_single = false;
  bool in_double = false;
  std::string out;
  out.reserve(line.size());

  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';

    if (block_comment) {
      if (c == '*' && next == '/') {
        block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }

    if (!in_single && !in_double && c == '/' && next == '*') {
      block_comment = true;
      i += 2;
      continue;
    }

    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      ++i;
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      ++i;
      continue;
    }
    if ((c == '#' || c == ';') && !in_single && !in_double) {
      break;
    }

    out.push_back(c);
    ++i;
  }

  if (in_block_comment != nullptr) *in_block_comment = block_comment;
  return out;
}

[[nodiscard]] inline std::string resolve_path_from_base_folder(
    std::string base_folder, std::string raw_path) {
  base_folder = trim_ascii(base_folder);
  raw_path = trim_ascii(raw_path);
  if (raw_path.empty()) return {};
  const std::filesystem::path p(raw_path);
  if (p.is_absolute()) return p.lexically_normal().string();
  if (base_folder.empty()) return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] inline std::string strip_ini_comment(std::string_view line) {
  std::string out;
  out.reserve(line.size());
  bool in_single = false;
  bool in_double = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
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
    if (!in_single && !in_double && (c == '#' || c == ';')) break;
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] inline bool read_ini_value(const std::filesystem::path& path,
                                         std::string_view section,
                                         std::string_view key,
                                         std::string* out_value) {
  if (out_value) out_value->clear();
  std::ifstream in(path);
  if (!in) return false;

  std::string current_section{};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key) continue;
    if (out_value) *out_value = trim_ascii(trimmed.substr(eq + 1));
    return true;
  }
  return false;
}

[[nodiscard]] inline std::string canonical_or_normalized(
    const std::filesystem::path& path) {
  std::error_code ec;
  const std::filesystem::path weak = std::filesystem::weakly_canonical(path, ec);
  if (!ec) return weak.string();
  ec.clear();
  std::filesystem::path absolute = path;
  if (!absolute.is_absolute()) {
    absolute = std::filesystem::absolute(absolute, ec);
    if (ec) ec.clear();
  }
  return absolute.lexically_normal().string();
}

[[nodiscard]] inline bool path_within(const std::filesystem::path& root,
                                      const std::filesystem::path& child) {
  const std::string root_text = canonical_or_normalized(root);
  const std::string child_text = canonical_or_normalized(child);
  if (root_text.empty() || child_text.empty()) return false;
  if (root_text == child_text) return true;
  if (child_text.size() <= root_text.size()) return false;
  if (child_text.compare(0, root_text.size(), root_text) != 0) return false;
  const char next = child_text[root_text.size()];
  return next == '/' || next == '\\';
}

[[nodiscard]] inline bool looks_safe_to_remove_tree(
    const std::filesystem::path& raw_path) {
  if (raw_path.empty()) return false;
  const std::filesystem::path normalized = raw_path.lexically_normal();
  if (!normalized.is_absolute()) return false;
  if (!normalized.has_filename()) return false;
  const std::filesystem::path filename = normalized.filename();
  if (filename.empty() || filename == "." || filename == "..") return false;
  const std::filesystem::path parent = normalized.parent_path();
  if (parent.empty() || parent == normalized.root_path()) return false;
  return true;
}

[[nodiscard]] inline bool parse_latent_lineage_kv_file(
    const std::filesystem::path& path,
    std::map<std::string, std::string, std::less<>>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for parsed key-value entries";
    return false;
  }
  out->clear();

  std::ifstream in(path);
  if (!in) {
    if (error) *error = "cannot open runtime defaults file: " + path.string();
    return false;
  }

  std::string raw;
  bool in_block_comment = false;
  std::size_t line_no = 0;
  while (std::getline(in, raw)) {
    ++line_no;
    const std::string stripped = trim_ascii(strip_comment_line(raw, &in_block_comment));
    if (stripped.empty()) continue;

    const std::size_t eq = stripped.find('=');
    if (eq == std::string::npos) {
      if (error) {
        *error = "invalid line in runtime defaults " + path.string() + ":" +
                 std::to_string(line_no) + " (missing '=')";
      }
      return false;
    }

    const std::string lhs = trim_ascii(stripped.substr(0, eq));
    const std::string rhs = trim_ascii(stripped.substr(eq + 1));
    const auto parsed =
        cuwacunu::camahjucunu::dsl::parse_latent_lineage_state_lhs(lhs);
    if (parsed.key.empty()) {
      if (error) {
        *error = "invalid line in runtime defaults " + path.string() + ":" +
                 std::to_string(line_no) + " (empty key)";
      }
      return false;
    }
    (*out)[parsed.key] = rhs;
  }

  return true;
}

inline void push_unique_path(std::vector<std::filesystem::path>* out,
                             std::vector<std::string>* seen_keys,
                             const std::filesystem::path& path) {
  if (!out || !seen_keys) return;
  if (path.empty()) return;
  const std::string key = canonical_or_normalized(path);
  if (key.empty()) return;
  for (const auto& seen : *seen_keys) {
    if (seen == key) return;
  }
  out->push_back(path.lexically_normal());
  seen_keys->push_back(key);
}

}  // namespace detail

[[nodiscard]] inline bool resolve_runtime_reset_targets_from_active_config(
    runtime_reset_targets_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for runtime reset targets";
    return false;
  }

  runtime_reset_targets_t resolved{};
  resolved.global_config_path = cuwacunu::iitepi::config_space_t::config_file_path;
  const std::string config_folder = cuwacunu::iitepi::config_space_t::config_folder;

  std::vector<std::string> store_root_seen{};
  std::vector<std::string> catalog_seen{};

  try {
    const std::string general_store_root =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "GENERAL", "hashimyei_store_root", std::string{});
    detail::push_unique_path(
        &resolved.store_roots, &store_root_seen,
        std::filesystem::path(
            detail::resolve_path_from_base_folder(config_folder, general_store_root)));

    const std::string hashimyei_hero_dsl =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "REAL_HERO", "hashimyei_hero_dsl_filename",
            std::string("/cuwacunu/src/config/instructions/default.hero.hashimyei.dsl"));
    const std::string lattice_hero_dsl =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "REAL_HERO", "lattice_hero_dsl_filename",
            std::string("/cuwacunu/src/config/instructions/default.hero.lattice.dsl"));
    const std::string runtime_hero_dsl =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "REAL_HERO", "runtime_hero_dsl_filename",
            std::string("/cuwacunu/src/config/instructions/default.hero.runtime.dsl"));

    resolved.hashimyei_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, hashimyei_hero_dsl));
    resolved.lattice_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, lattice_hero_dsl));
    resolved.runtime_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, runtime_hero_dsl));

    std::map<std::string, std::string, std::less<>> hash_defaults{};
    if (!resolved.hashimyei_hero_dsl_path.empty()) {
      std::string parse_error;
      if (!detail::parse_latent_lineage_kv_file(
              resolved.hashimyei_hero_dsl_path, &hash_defaults, &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      const auto it_store_root = hash_defaults.find("store_root");
      if (it_store_root != hash_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.hashimyei_hero_dsl_path.parent_path().string(),
                it_store_root->second)));
      }
      const auto it_catalog = hash_defaults.find("catalog_path");
      if (it_catalog != hash_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.hashimyei_hero_dsl_path.parent_path().string(),
                it_catalog->second)));
      }
    }

    std::map<std::string, std::string, std::less<>> lattice_defaults{};
    if (!resolved.lattice_hero_dsl_path.empty()) {
      std::string parse_error;
      if (!detail::parse_latent_lineage_kv_file(
              resolved.lattice_hero_dsl_path, &lattice_defaults, &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      const auto it_store_root = lattice_defaults.find("store_root");
      if (it_store_root != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it_store_root->second)));
      }
      const auto it_catalog = lattice_defaults.find("catalog_path");
      if (it_catalog != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it_catalog->second)));
      }
      const auto it_hash_catalog = lattice_defaults.find("hashimyei_catalog_path");
      if (it_hash_catalog != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it_hash_catalog->second)));
      }
    }

    std::map<std::string, std::string, std::less<>> runtime_defaults{};
    if (!resolved.runtime_hero_dsl_path.empty()) {
      std::string parse_error;
      if (!detail::parse_latent_lineage_kv_file(
              resolved.runtime_hero_dsl_path, &runtime_defaults, &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      const auto it_jobs_root = runtime_defaults.find("jobs_root");
      if (it_jobs_root != runtime_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.runtime_hero_dsl_path.parent_path().string(),
                it_jobs_root->second)));
      }
    }
  } catch (const std::exception& e) {
    if (error) *error = e.what();
    return false;
  }

  if (resolved.store_roots.empty() && resolved.catalog_paths.empty()) {
    if (error) {
      *error = "no runtime store roots or catalog paths resolved from active config";
    }
    return false;
  }

  *out = std::move(resolved);
  return true;
}

[[nodiscard]] inline bool resolve_runtime_reset_targets_from_global_config(
    std::string_view global_config_path, runtime_reset_targets_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for runtime reset targets";
    return false;
  }

  runtime_reset_targets_t resolved{};
  const std::string requested_global_config = detail::trim_ascii(global_config_path);
  if (requested_global_config.empty()) {
    resolved.global_config_path = cuwacunu::iitepi::config_space_t::config_file_path;
  } else {
    resolved.global_config_path = std::filesystem::path(requested_global_config);
  }
  resolved.global_config_path = resolved.global_config_path.lexically_normal();
  const std::string config_folder =
      resolved.global_config_path.parent_path().string();

  std::vector<std::string> store_root_seen{};
  std::vector<std::string> catalog_seen{};

  try {
    std::string general_store_root{};
    if (detail::read_ini_value(resolved.global_config_path, "GENERAL",
                               "hashimyei_store_root", &general_store_root)) {
      detail::push_unique_path(
          &resolved.store_roots, &store_root_seen,
          std::filesystem::path(detail::resolve_path_from_base_folder(
              config_folder, general_store_root)));
    }

    std::string hashimyei_hero_dsl = "/cuwacunu/src/config/instructions/default.hero.hashimyei.dsl";
    std::string lattice_hero_dsl = "/cuwacunu/src/config/instructions/default.hero.lattice.dsl";
    std::string runtime_hero_dsl = "/cuwacunu/src/config/instructions/default.hero.runtime.dsl";
    (void)detail::read_ini_value(resolved.global_config_path, "REAL_HERO",
                                 "hashimyei_hero_dsl_filename",
                                 &hashimyei_hero_dsl);
    (void)detail::read_ini_value(resolved.global_config_path, "REAL_HERO",
                                 "lattice_hero_dsl_filename",
                                 &lattice_hero_dsl);
    (void)detail::read_ini_value(resolved.global_config_path, "REAL_HERO",
                                 "runtime_hero_dsl_filename",
                                 &runtime_hero_dsl);

    resolved.hashimyei_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, hashimyei_hero_dsl));
    resolved.lattice_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, lattice_hero_dsl));
    resolved.runtime_hero_dsl_path = std::filesystem::path(
        detail::resolve_path_from_base_folder(config_folder, runtime_hero_dsl));

    std::map<std::string, std::string, std::less<>> hash_defaults{};
    if (!resolved.hashimyei_hero_dsl_path.empty()) {
      std::string parse_error{};
      if (!detail::parse_latent_lineage_kv_file(
              resolved.hashimyei_hero_dsl_path, &hash_defaults, &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      if (const auto it = hash_defaults.find("store_root");
          it != hash_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.hashimyei_hero_dsl_path.parent_path().string(),
                it->second)));
      }
      if (const auto it = hash_defaults.find("catalog_path");
          it != hash_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.hashimyei_hero_dsl_path.parent_path().string(),
                it->second)));
      }
    }

    std::map<std::string, std::string, std::less<>> lattice_defaults{};
    if (!resolved.lattice_hero_dsl_path.empty()) {
      std::string parse_error{};
      if (!detail::parse_latent_lineage_kv_file(
              resolved.lattice_hero_dsl_path, &lattice_defaults,
              &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      if (const auto it = lattice_defaults.find("store_root");
          it != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it->second)));
      }
      if (const auto it = lattice_defaults.find("catalog_path");
          it != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it->second)));
      }
      if (const auto it = lattice_defaults.find("hashimyei_catalog_path");
          it != lattice_defaults.end()) {
        detail::push_unique_path(
            &resolved.catalog_paths, &catalog_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.lattice_hero_dsl_path.parent_path().string(),
                it->second)));
      }
    }

    std::map<std::string, std::string, std::less<>> runtime_defaults{};
    if (!resolved.runtime_hero_dsl_path.empty()) {
      std::string parse_error{};
      if (!detail::parse_latent_lineage_kv_file(
              resolved.runtime_hero_dsl_path, &runtime_defaults,
              &parse_error)) {
        if (error) *error = parse_error;
        return false;
      }
      if (const auto it = runtime_defaults.find("jobs_root");
          it != runtime_defaults.end()) {
        detail::push_unique_path(
            &resolved.store_roots, &store_root_seen,
            std::filesystem::path(detail::resolve_path_from_base_folder(
                resolved.runtime_hero_dsl_path.parent_path().string(),
                it->second)));
      }
    }
  } catch (const std::exception& e) {
    if (error) *error = e.what();
    return false;
  }

  if (resolved.store_roots.empty() && resolved.catalog_paths.empty()) {
    if (error) {
      *error =
          "no runtime store roots or catalog paths resolved from global config";
    }
    return false;
  }

  *out = std::move(resolved);
  return true;
}

[[nodiscard]] inline bool reset_runtime_state(
    const runtime_reset_targets_t& targets,
    runtime_reset_result_t* out_result = nullptr, std::string* error = nullptr) {
  if (error) error->clear();
  runtime_reset_result_t result{};

  std::vector<std::filesystem::path> removed_roots{};
  removed_roots.reserve(targets.store_roots.size());

  for (const auto& root : targets.store_roots) {
    if (root.empty()) continue;
    if (!detail::looks_safe_to_remove_tree(root)) {
      if (error) {
        *error = "refusing to remove runtime store root: " +
                 root.lexically_normal().string();
      }
      return false;
    }

    std::error_code ec{};
    const std::uintmax_t removed = std::filesystem::remove_all(root, ec);
    if (ec) {
      if (error) {
        *error = "failed to remove runtime store root " + root.string() + ": " +
                 ec.message();
      }
      return false;
    }
    result.removed_store_roots.push_back(root.lexically_normal());
    result.removed_store_entries += removed;
    removed_roots.push_back(root.lexically_normal());
  }

  for (const auto& catalog : targets.catalog_paths) {
    if (catalog.empty()) continue;

    bool already_removed_with_store_root = false;
    for (const auto& root : removed_roots) {
      if (detail::path_within(root, catalog)) {
        already_removed_with_store_root = true;
        break;
      }
    }
    if (already_removed_with_store_root) continue;

    std::error_code ec{};
    std::filesystem::remove(catalog, ec);
    if (ec && std::filesystem::exists(catalog)) {
      if (error) {
        *error = "failed to remove runtime catalog " + catalog.string() + ": " +
                 ec.message();
      }
      return false;
    }
    result.removed_catalog_paths.push_back(catalog.lexically_normal());
  }

  if (out_result) *out_result = std::move(result);
  return true;
}

[[nodiscard]] inline bool reset_runtime_state_from_active_config(
    runtime_reset_result_t* out_result = nullptr, std::string* error = nullptr) {
  runtime_reset_targets_t targets{};
  if (!resolve_runtime_reset_targets_from_active_config(&targets, error)) {
    return false;
  }
  return reset_runtime_state(targets, out_result, error);
}

}  // namespace runtime_dev
}  // namespace hero
}  // namespace cuwacunu
