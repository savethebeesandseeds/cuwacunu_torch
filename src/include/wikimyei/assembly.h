// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cuwacunu::wikimyei::assembly {

enum class dock_direction_t {
  consumes,
  produces,
};

enum class dock_role_t {
  conditioning,
  supervised_target,
  output,
  diagnostic_sidecar,
};

enum class dock_domain_t {
  graph_edge_kline_evidence,
  future_graph_edge_kline_evidence,
  node_lifted_state,
  future_node_lifted_state,
  node_representation,
  future_node_distribution,
  price_residual_sidecar,
  activity_sidecar,
};

struct dock_t {
  std::string name{};
  dock_direction_t direction{dock_direction_t::consumes};
  dock_role_t role{dock_role_t::conditioning};
  dock_domain_t domain{dock_domain_t::graph_edge_kline_evidence};
  std::string coordinate_space{};
  std::string value_shape{};
  std::string mask_shape{};
  std::vector<std::string> variables{};
  bool required{true};
  bool target_side_only{false};
};

[[nodiscard]] inline const char *
dock_direction_name(dock_direction_t direction) {
  switch (direction) {
  case dock_direction_t::consumes:
    return "consumes";
  case dock_direction_t::produces:
    return "produces";
  }
  return "unknown";
}

[[nodiscard]] inline const char *dock_role_name(dock_role_t role) {
  switch (role) {
  case dock_role_t::conditioning:
    return "conditioning";
  case dock_role_t::supervised_target:
    return "supervised_target";
  case dock_role_t::output:
    return "output";
  case dock_role_t::diagnostic_sidecar:
    return "diagnostic_sidecar";
  }
  return "unknown";
}

[[nodiscard]] inline const char *dock_domain_name(dock_domain_t domain) {
  switch (domain) {
  case dock_domain_t::graph_edge_kline_evidence:
    return "graph_edge_kline_evidence";
  case dock_domain_t::future_graph_edge_kline_evidence:
    return "future_graph_edge_kline_evidence";
  case dock_domain_t::node_lifted_state:
    return "node_lifted_state";
  case dock_domain_t::future_node_lifted_state:
    return "future_node_lifted_state";
  case dock_domain_t::node_representation:
    return "node_representation";
  case dock_domain_t::future_node_distribution:
    return "future_node_distribution";
  case dock_domain_t::price_residual_sidecar:
    return "price_residual_sidecar";
  case dock_domain_t::activity_sidecar:
    return "activity_sidecar";
  }
  return "unknown";
}

inline void validate_dock(const dock_t &dock) {
  if (dock.name.empty()) {
    throw std::runtime_error("[wikimyei_assembly] dock name is required");
  }
  if (dock.coordinate_space.empty()) {
    throw std::runtime_error("[wikimyei_assembly] dock coordinate_space is "
                             "required for dock: " +
                             dock.name);
  }
  if (dock.value_shape.empty()) {
    throw std::runtime_error("[wikimyei_assembly] dock value_shape is required "
                             "for dock: " +
                             dock.name);
  }
  std::unordered_set<std::string> seen_variables;
  seen_variables.reserve(dock.variables.size());
  for (const auto &variable : dock.variables) {
    if (variable.empty()) {
      throw std::runtime_error("[wikimyei_assembly] dock variable is empty for "
                               "dock: " +
                               dock.name);
    }
    if (!seen_variables.insert(variable).second) {
      throw std::runtime_error("[wikimyei_assembly] duplicate dock variable '" +
                               variable + "' for dock: " + dock.name);
    }
  }
}

[[nodiscard]] inline bool dock_domain_compatible(const dock_t &producer,
                                                 const dock_t &consumer) {
  return producer.direction == dock_direction_t::produces &&
         consumer.direction == dock_direction_t::consumes &&
         producer.domain == consumer.domain &&
         producer.coordinate_space == consumer.coordinate_space;
}

[[nodiscard]] inline dock_t
make_dock(std::string name, dock_direction_t direction, dock_role_t role,
          dock_domain_t domain, std::string coordinate_space,
          std::string value_shape, std::string mask_shape = {},
          bool required = true, bool target_side_only = false,
          std::vector<std::string> variables = {}) {
  dock_t out{.name = std::move(name),
             .direction = direction,
             .role = role,
             .domain = domain,
             .coordinate_space = std::move(coordinate_space),
             .value_shape = std::move(value_shape),
             .mask_shape = std::move(mask_shape),
             .variables = std::move(variables),
             .required = required,
             .target_side_only = target_side_only};
  validate_dock(out);
  return out;
}

enum class assembly_trainability_t {
  deterministic,
  trainable,
};

struct wikimyei_assembly_t {
  std::string family{};
  std::string component_assembly_id{};
  std::string version_token{};
  assembly_trainability_t trainability{assembly_trainability_t::deterministic};
  std::vector<dock_t> docks{};
  std::vector<std::string> constraints{};
};

[[nodiscard]] inline const char *
assembly_trainability_name(assembly_trainability_t trainability) {
  switch (trainability) {
  case assembly_trainability_t::deterministic:
    return "deterministic";
  case assembly_trainability_t::trainable:
    return "trainable";
  }
  return "unknown";
}

[[nodiscard]] inline std::string
make_assembly_token(const std::string &family, const std::string &component_assembly_id,
                    const std::string &version_token) {
  if (family.empty() || component_assembly_id.empty() || version_token.empty()) {
    throw std::runtime_error("[wikimyei_assembly] family, component_assembly_id, and "
                             "version_token are required");
  }
  return family + "/" + component_assembly_id + "/" + version_token;
}

namespace assembly_detail {

inline constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnvPrime = 1099511628211ull;

inline void mix_hash_byte(std::uint64_t &hash, unsigned char byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= kFnvPrime;
}

inline void mix_hash_string(std::uint64_t &hash, std::string_view value) {
  for (const unsigned char c : value) {
    mix_hash_byte(hash, c);
  }
  mix_hash_byte(hash, 0xffu);
}

[[nodiscard]] inline std::string hash_hex(std::uint64_t hash) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

} // namespace assembly_detail

[[nodiscard]] inline std::string
canonical_assembly_text(const wikimyei_assembly_t &assembly) {
  std::ostringstream out;
  out << "family=" << assembly.family << "\n";
  out << "component_assembly_id=" << assembly.component_assembly_id << "\n";
  out << "version_token=" << assembly.version_token << "\n";
  out << "trainability=" << assembly_trainability_name(assembly.trainability)
      << "\n";
  for (const auto &dock : assembly.docks) {
    out << "dock=" << dock.name << "|" << dock_direction_name(dock.direction)
        << "|" << dock_role_name(dock.role) << "|"
        << dock_domain_name(dock.domain) << "|" << dock.coordinate_space << "|"
        << dock.value_shape << "|" << dock.mask_shape << "|"
        << (dock.required ? "required" : "optional") << "|"
        << (dock.target_side_only ? "target_side" : "same_anchor") << "\n";
    for (const auto &variable : dock.variables) {
      out << "dock_variable=" << dock.name << "|" << variable << "\n";
    }
  }
  for (const auto &constraint : assembly.constraints) {
    out << "constraint=" << constraint << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string
assembly_fingerprint(const wikimyei_assembly_t &assembly) {
  std::uint64_t hash = assembly_detail::kFnvOffsetBasis;
  assembly_detail::mix_hash_string(hash, "cuwacunu.wikimyei.assembly.v1");
  assembly_detail::mix_hash_string(hash, canonical_assembly_text(assembly));
  return assembly_detail::hash_hex(hash);
}

inline void validate_wikimyei_assembly(const wikimyei_assembly_t &assembly) {
  (void)make_assembly_token(assembly.family, assembly.component_assembly_id,
                            assembly.version_token);
  if (assembly.docks.empty()) {
    throw std::runtime_error("[wikimyei_assembly] assembly has no docks: " +
                             assembly.component_assembly_id);
  }

  std::unordered_set<std::string> seen_docks;
  seen_docks.reserve(assembly.docks.size());
  for (const auto &dock : assembly.docks) {
    validate_dock(dock);
    if (!seen_docks.insert(dock.name).second) {
      throw std::runtime_error("[wikimyei_assembly] duplicate dock name: " +
                               dock.name);
    }
  }
}

[[nodiscard]] inline std::optional<dock_t>
find_dock(const wikimyei_assembly_t &assembly, dock_direction_t direction,
          dock_domain_t domain) {
  for (const auto &dock : assembly.docks) {
    if (dock.direction == direction && dock.domain == domain) {
      return dock;
    }
  }
  return std::nullopt;
}

inline void require_dock(const wikimyei_assembly_t &assembly,
                         dock_direction_t direction, dock_domain_t domain,
                         const char *label) {
  if (!find_dock(assembly, direction, domain).has_value()) {
    throw std::runtime_error(std::string("[wikimyei_assembly] missing ") +
                             label + " dock for " + assembly.component_assembly_id);
  }
}

inline void require_compatible_docks(const wikimyei_assembly_t &producer,
                                     dock_domain_t produced_domain,
                                     const wikimyei_assembly_t &consumer,
                                     dock_domain_t consumed_domain,
                                     const char *label) {
  const auto produced =
      find_dock(producer, dock_direction_t::produces, produced_domain);
  const auto consumed =
      find_dock(consumer, dock_direction_t::consumes, consumed_domain);
  if (!produced.has_value() || !consumed.has_value() ||
      !dock_domain_compatible(*produced, *consumed)) {
    throw std::runtime_error(std::string("[wikimyei_assembly] incompatible ") +
                             label + " docks: " + producer.component_assembly_id +
                             " -> " + consumer.component_assembly_id);
  }
}

} // namespace cuwacunu::wikimyei::assembly
