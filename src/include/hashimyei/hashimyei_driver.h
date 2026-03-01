// ./include/hashimyei/hashimyei_driver.h
// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace hashimyei {

// Generic action context passed from runtime/controller code to a component
// driver. The driver may use object_handle to call component-owned save/load
// routines without transferring ownership to hashimyei.
struct artifact_action_context_t {
  std::string canonical_type{};
  std::string family{};
  std::string model{};
  std::string artifact_id{};
  std::filesystem::path artifact_directory{};
  std::string canonical_action{};
  void* object_handle{nullptr}; // Component-owned object, optional.
  void* user_data{nullptr};     // Caller-owned auxiliary context, optional.
};

using artifact_save_callback_t =
    std::function<bool(const artifact_action_context_t&, std::string* error)>;
using artifact_load_callback_t =
    std::function<bool(const artifact_action_context_t&, std::string* error)>;

struct artifact_driver_t {
  std::string canonical_type{};
  std::string family{};
  std::string model{};
  artifact_save_callback_t save{};
  artifact_load_callback_t load{};
};

inline std::unordered_map<std::string, artifact_driver_t>& artifact_driver_registry() {
  static std::unordered_map<std::string, artifact_driver_t> registry{};
  return registry;
}

inline std::mutex& artifact_driver_registry_mutex() {
  static std::mutex mu{};
  return mu;
}

[[nodiscard]] inline bool register_artifact_driver(artifact_driver_t driver,
                                                   std::string* error = nullptr) {
  if (error) error->clear();
  if (driver.canonical_type.empty()) {
    if (error) *error = "artifact driver canonical_type is empty";
    return false;
  }
  if (!driver.save && !driver.load) {
    if (error) *error = "artifact driver has no save/load callbacks";
    return false;
  }

  std::lock_guard<std::mutex> lk(artifact_driver_registry_mutex());
  auto& registry = artifact_driver_registry();
  const auto it = registry.find(driver.canonical_type);
  if (it != registry.end()) {
    if (error) *error = "artifact driver already registered for canonical_type: " + driver.canonical_type;
    return false;
  }
  registry.emplace(driver.canonical_type, std::move(driver));
  return true;
}

[[nodiscard]] inline bool has_artifact_driver(std::string_view canonical_type) {
  std::lock_guard<std::mutex> lk(artifact_driver_registry_mutex());
  const auto& registry = artifact_driver_registry();
  return registry.find(std::string(canonical_type)) != registry.end();
}

[[nodiscard]] inline std::vector<std::string> registered_artifact_driver_types() {
  std::lock_guard<std::mutex> lk(artifact_driver_registry_mutex());
  const auto& registry = artifact_driver_registry();
  std::vector<std::string> out;
  out.reserve(registry.size());
  for (const auto& kv : registry) out.push_back(kv.first);
  return out;
}

[[nodiscard]] inline bool dispatch_artifact_save(std::string_view canonical_type,
                                                 const artifact_action_context_t& ctx,
                                                 std::string* error = nullptr) {
  if (error) error->clear();
  artifact_save_callback_t callback;
  {
    std::lock_guard<std::mutex> lk(artifact_driver_registry_mutex());
    const auto& registry = artifact_driver_registry();
    const auto it = registry.find(std::string(canonical_type));
    if (it == registry.end()) {
      if (error) *error = "no artifact driver registered for canonical_type: " + std::string(canonical_type);
      return false;
    }
    callback = it->second.save;
  }
  if (!callback) {
    if (error) *error = "artifact driver does not support save for canonical_type: " + std::string(canonical_type);
    return false;
  }
  return callback(ctx, error);
}

[[nodiscard]] inline bool dispatch_artifact_load(std::string_view canonical_type,
                                                 const artifact_action_context_t& ctx,
                                                 std::string* error = nullptr) {
  if (error) error->clear();
  artifact_load_callback_t callback;
  {
    std::lock_guard<std::mutex> lk(artifact_driver_registry_mutex());
    const auto& registry = artifact_driver_registry();
    const auto it = registry.find(std::string(canonical_type));
    if (it == registry.end()) {
      if (error) *error = "no artifact driver registered for canonical_type: " + std::string(canonical_type);
      return false;
    }
    callback = it->second.load;
  }
  if (!callback) {
    if (error) *error = "artifact driver does not support load for canonical_type: " + std::string(canonical_type);
    return false;
  }
  return callback(ctx, error);
}

}  // namespace hashimyei
}  // namespace cuwacunu
