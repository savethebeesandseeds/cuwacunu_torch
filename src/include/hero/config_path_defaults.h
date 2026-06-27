// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace cuwacunu::hero::config_paths {
namespace detail {

[[nodiscard]] inline std::filesystem::path
normalize_path(const std::filesystem::path &path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  if (!ec) {
    return canonical;
  }
  return path.lexically_normal();
}

[[nodiscard]] inline bool file_exists(const std::filesystem::path &path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec) && !ec;
}

[[nodiscard]] inline bool
path_is_under_root_child(const std::filesystem::path &path,
                         std::string_view child_name) {
  const auto normalized = normalize_path(path);
  const auto root = normalized.root_path();
  if (root.empty()) {
    return false;
  }
  const auto relative = normalized.lexically_relative(root);
  auto it = relative.begin();
  return it != relative.end() && it->string() == child_name;
}

[[nodiscard]] inline bool
path_is_under_src_config_child(const std::filesystem::path &path,
                               std::string_view child_name) {
  const auto normalized = normalize_path(path);
  const auto root = normalized.root_path();
  if (root.empty()) {
    return false;
  }
  const auto relative = normalized.lexically_relative(root);
  auto it = relative.begin();
  while (it != relative.end()) {
    if (it->string() != "src") {
      ++it;
      continue;
    }
    auto config_it = it;
    ++config_it;
    if (config_it == relative.end() || config_it->string() != "config") {
      ++it;
      continue;
    }
    auto child_it = config_it;
    ++child_it;
    return child_it != relative.end() && child_it->string() == child_name;
  }
  return false;
}

[[nodiscard]] inline std::filesystem::path
first_config_beside_anchor(const std::filesystem::path &anchor) {
  static constexpr std::array<std::string_view, 4> kRelativeCandidates{
      "src/config/.config",
      "../src/config/.config",
      "../../src/config/.config",
      "../../../src/config/.config",
  };
  for (const auto rel : kRelativeCandidates) {
    const auto candidate = normalize_path(anchor / rel);
    if (file_exists(candidate)) {
      return candidate;
    }
  }
  return {};
}

} // namespace detail

[[nodiscard]] inline std::filesystem::path
default_global_config_path(const char *argv0 = nullptr) {
#if defined(__linux__)
  {
    std::array<char, 4096> buffer{};
    const ssize_t n =
        ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (n > 0) {
      const auto exe_path =
          std::filesystem::path(std::string(buffer.data(), n));
      if (const auto found =
              detail::first_config_beside_anchor(exe_path.parent_path());
          !found.empty()) {
        return found;
      }
    }
  }
#endif

  if (argv0 != nullptr && argv0[0] != '\0') {
    std::filesystem::path exe_path(argv0);
    if (exe_path.is_relative()) {
      exe_path = std::filesystem::current_path() / exe_path;
    }
    if (const auto found =
            detail::first_config_beside_anchor(exe_path.parent_path());
        !found.empty()) {
      return found;
    }
  }

  if (const auto found =
          detail::first_config_beside_anchor(std::filesystem::current_path());
      !found.empty()) {
    return found;
  }

  return detail::normalize_path(std::filesystem::current_path() /
                                "src/config/.config");
}

[[nodiscard]] inline std::filesystem::path
default_config_sibling_path(const std::filesystem::path &global_config_path,
                            std::string_view sibling_filename) {
  std::filesystem::path base = global_config_path.empty()
                                   ? default_global_config_path()
                                   : global_config_path;
  if (base.is_relative()) {
    base = std::filesystem::current_path() / base;
  }
  base = detail::normalize_path(base);
  return detail::normalize_path(base.parent_path() /
                                std::filesystem::path(sibling_filename));
}

[[nodiscard]] inline std::filesystem::path default_runtime_root_path(
    const std::filesystem::path &global_config_path = {}) {
  const auto candidate = default_config_sibling_path(
      global_config_path, "../../.runtime/cuwacunu_exec");
  if (!global_config_path.empty() &&
      (detail::path_is_under_root_child(candidate, ".runtime") ||
       detail::path_is_under_src_config_child(candidate, ".runtime"))) {
    return default_config_sibling_path(default_global_config_path(),
                                       "../../.runtime/cuwacunu_exec");
  }
  return candidate;
}

[[nodiscard]] inline std::filesystem::path default_runtime_exec_path(
    const std::filesystem::path &global_config_path = {}) {
  const auto candidate = default_config_sibling_path(
      global_config_path, "../../.build/exec/cuwacunu_exec");
  if (!global_config_path.empty() &&
      (detail::path_is_under_root_child(candidate, ".build") ||
       detail::path_is_under_src_config_child(candidate, ".build"))) {
    return default_config_sibling_path(default_global_config_path(),
                                       "../../.build/exec/cuwacunu_exec");
  }
  return candidate;
}

[[nodiscard]] inline std::filesystem::path default_hero_runtime_mcp_path(
    const std::filesystem::path &global_config_path = {}) {
  return default_config_sibling_path(global_config_path,
                                     "../../.build/hero/hero_runtime.mcp");
}

[[nodiscard]] inline std::filesystem::path default_hero_environment_mcp_path(
    const std::filesystem::path &global_config_path = {}) {
  return default_config_sibling_path(global_config_path,
                                     "../../.build/hero/hero_environment.mcp");
}

} // namespace cuwacunu::hero::config_paths
