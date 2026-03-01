/* schema_catalog.h
 *
 * Batch-1 jkimyei C++ API catalog built from jkimyei_schema.def descriptors.
 * This keeps runtime builders aligned with the canonical schema surface.
 */
#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "jkimyei/specs/jkimyei_schema.h"

namespace cuwacunu {
namespace jkimyei {
namespace api {

struct owner_schema_t {
  std::unordered_map<std::string, specs::value_kind_e> key_kind{};
  std::unordered_set<std::string> required_keys{};
};

inline const std::unordered_map<std::string, owner_schema_t>& owner_schemas() {
  static const std::unordered_map<std::string, owner_schema_t> kOwners = [] {
    std::unordered_map<std::string, owner_schema_t> out;
    for (const auto& param : specs::kTypedParams) {
      owner_schema_t& owner = out[std::string(param.owner)];
      owner.key_kind[std::string(param.key)] = param.kind;
      if (param.required) owner.required_keys.insert(std::string(param.key));
    }
    return out;
  }();
  return kOwners;
}

inline bool has_owner(std::string_view owner) {
  return owner_schemas().find(std::string(owner)) != owner_schemas().end();
}

inline bool has_optimizer_type(std::string_view type) {
  return has_owner(std::string("optimizer.") + std::string(type));
}

inline bool has_scheduler_type(std::string_view type) {
  return has_owner(std::string("scheduler.") + std::string(type));
}

inline bool has_loss_type(std::string_view type) {
  return has_owner(std::string("loss.") + std::string(type));
}

inline std::vector<std::string> owners_with_prefix(std::string_view prefix) {
  std::vector<std::string> out;
  for (const auto& [owner, _] : owner_schemas()) {
    if (owner.rfind(prefix.data(), 0) == 0) out.push_back(owner);
  }
  return out;
}

inline std::unordered_set<std::string> supported_optimizer_types() {
  std::unordered_set<std::string> out;
  for (const auto& owner : owners_with_prefix("optimizer.")) {
    out.insert(owner.substr(std::string("optimizer.").size()));
  }
  return out;
}

inline std::unordered_set<std::string> supported_scheduler_types() {
  std::unordered_set<std::string> out;
  for (const auto& owner : owners_with_prefix("scheduler.")) {
    out.insert(owner.substr(std::string("scheduler.").size()));
  }
  return out;
}

inline std::unordered_set<std::string> supported_loss_types() {
  std::unordered_set<std::string> out;
  for (const auto& owner : owners_with_prefix("loss.")) {
    out.insert(owner.substr(std::string("loss.").size()));
  }
  return out;
}

inline const owner_schema_t& require_owner_schema(const std::string& owner) {
  auto it = owner_schemas().find(owner);
  if (it == owner_schemas().end()) {
    throw std::runtime_error("jkimyei schema owner not found: " + owner);
  }
  return it->second;
}

inline void require_optimizer_type_registered(const std::string& type) {
  if (!has_optimizer_type(type)) {
    throw std::runtime_error("optimizer type not registered in jkimyei_schema.def: " + type);
  }
}

inline void require_scheduler_type_registered(const std::string& type) {
  if (!has_scheduler_type(type)) {
    throw std::runtime_error("scheduler type not registered in jkimyei_schema.def: " + type);
  }
}

inline void require_loss_type_registered(const std::string& type) {
  if (!has_loss_type(type)) {
    throw std::runtime_error("loss type not registered in jkimyei_schema.def: " + type);
  }
}

} // namespace api
} // namespace jkimyei
} // namespace cuwacunu

