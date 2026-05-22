// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::topology {

namespace wikimyei_assembly = cuwacunu::wikimyei::assembly;

inline constexpr const char *kGraphTopologyDockBindingId =
    "kikijyeba.topology.graph.dock_binding.v1";

enum class dock_variable_scope_t {
  static_bound,
  runtime_bound,
};

struct dock_variable_t {
  std::string name{};
  std::string source{};
  dock_variable_scope_t scope{dock_variable_scope_t::static_bound};
  std::optional<std::int64_t> i64_value{std::nullopt};
};

struct dock_binding_t {
  std::string binding_id{kGraphTopologyDockBindingId};
  std::vector<dock_variable_t> variables{};
  std::vector<std::string> constraints{};
};

struct dock_binding_validation_report_t {
  std::vector<std::string> warnings{};
};

[[nodiscard]] inline const char *
dock_variable_scope_name(dock_variable_scope_t scope) {
  switch (scope) {
  case dock_variable_scope_t::static_bound:
    return "static";
  case dock_variable_scope_t::runtime_bound:
    return "runtime";
  }
  return "unknown";
}

[[nodiscard]] inline dock_variable_t
make_static_i64_variable(std::string name, std::string source,
                         std::int64_t value) {
  return dock_variable_t{.name = std::move(name),
                         .source = std::move(source),
                         .scope = dock_variable_scope_t::static_bound,
                         .i64_value = value};
}

[[nodiscard]] inline dock_variable_t make_runtime_variable(std::string name,
                                                           std::string source) {
  return dock_variable_t{.name = std::move(name),
                         .source = std::move(source),
                         .scope = dock_variable_scope_t::runtime_bound,
                         .i64_value = std::nullopt};
}

inline void validate_dock_binding(const dock_binding_t &binding) {
  if (binding.binding_id.empty()) {
    throw std::runtime_error("[dock_binding] binding_id is required");
  }
  if (binding.variables.empty()) {
    throw std::runtime_error(
        "[dock_binding] at least one variable is required");
  }
  std::unordered_set<std::string> seen;
  seen.reserve(binding.variables.size());
  for (const auto &variable : binding.variables) {
    if (variable.name.empty()) {
      throw std::runtime_error("[dock_binding] variable name is required");
    }
    if (variable.source.empty()) {
      throw std::runtime_error(
          "[dock_binding] source is required for variable " + variable.name);
    }
    if (!seen.insert(variable.name).second) {
      throw std::runtime_error("[dock_binding] duplicate variable: " +
                               variable.name);
    }
    if (variable.scope == dock_variable_scope_t::static_bound &&
        !variable.i64_value.has_value()) {
      throw std::runtime_error("[dock_binding] static variable has no value: " +
                               variable.name);
    }
    if (variable.scope == dock_variable_scope_t::runtime_bound &&
        variable.i64_value.has_value()) {
      throw std::runtime_error("[dock_binding] runtime variable has a static "
                               "value: " +
                               variable.name);
    }
  }
}

[[nodiscard]] inline std::optional<dock_variable_t>
find_dock_binding_variable(const dock_binding_t &binding,
                           const std::string &name) {
  for (const auto &variable : binding.variables) {
    if (variable.name == name) {
      return variable;
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline dock_binding_validation_report_t
analyze_dock_binding_against_assemblies(
    const dock_binding_t &binding,
    const std::vector<wikimyei_assembly::wikimyei_assembly_t> &assemblies) {
  validate_dock_binding(binding);
  if (assemblies.empty()) {
    throw std::runtime_error(
        "[dock_binding] cannot validate binding without assemblies");
  }

  std::unordered_set<std::string> bound_variables;
  bound_variables.reserve(binding.variables.size());
  for (const auto &variable : binding.variables) {
    bound_variables.insert(variable.name);
  }

  std::unordered_set<std::string> used_variables;
  for (const auto &assembly : assemblies) {
    wikimyei_assembly::validate_wikimyei_assembly(assembly);
    for (const auto &dock : assembly.docks) {
      for (const auto &variable : dock.variables) {
        used_variables.insert(variable);
        if (bound_variables.find(variable) == bound_variables.end()) {
          throw std::runtime_error(
              "[dock_binding] assembly " + assembly.component_id + " dock " +
              dock.name + " requires unbound variable: " + variable);
        }
      }
    }
  }

  dock_binding_validation_report_t report{};
  for (const auto &variable : binding.variables) {
    if (used_variables.find(variable.name) == used_variables.end()) {
      report.warnings.push_back("[dock_binding] variable '" + variable.name +
                                "' is bound but unused by active assemblies");
    }
  }
  return report;
}

[[nodiscard]] inline std::int64_t
require_static_i64_binding(const dock_binding_t &binding,
                           const std::string &name) {
  const auto variable = find_dock_binding_variable(binding, name);
  if (!variable.has_value()) {
    throw std::runtime_error("[dock_binding] missing variable: " + name);
  }
  if (variable->scope != dock_variable_scope_t::static_bound ||
      !variable->i64_value.has_value()) {
    throw std::runtime_error(
        "[dock_binding] variable is not statically bound: " + name);
  }
  return *variable->i64_value;
}

inline void require_static_i64_binding_value(const dock_binding_t &binding,
                                             const std::string &name,
                                             std::int64_t expected,
                                             const std::string &label) {
  const auto actual = require_static_i64_binding(binding, name);
  if (actual != expected) {
    throw std::runtime_error("[dock_binding] variable " + name +
                             " mismatch for " + label + ": expected " +
                             std::to_string(expected) + ", got " +
                             std::to_string(actual));
  }
}

inline void require_runtime_binding(const dock_binding_t &binding,
                                    const std::string &name,
                                    const std::string &label) {
  const auto variable = find_dock_binding_variable(binding, name);
  if (!variable.has_value()) {
    throw std::runtime_error("[dock_binding] missing variable: " + name);
  }
  if (variable->scope != dock_variable_scope_t::runtime_bound ||
      variable->i64_value.has_value()) {
    throw std::runtime_error("[dock_binding] variable " + name +
                             " must be runtime-bound for " + label);
  }
}

[[nodiscard]] inline std::string
canonical_dock_binding_text(const dock_binding_t &binding) {
  std::ostringstream out;
  out << "binding_id=" << binding.binding_id << "\n";
  for (const auto &variable : binding.variables) {
    out << "variable=" << variable.name << "|"
        << dock_variable_scope_name(variable.scope) << "|" << variable.source
        << "|";
    if (variable.i64_value.has_value()) {
      out << *variable.i64_value;
    } else {
      out << "runtime";
    }
    out << "\n";
  }
  for (const auto &constraint : binding.constraints) {
    out << "constraint=" << constraint << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string
dock_binding_fingerprint(const dock_binding_t &binding) {
  validate_dock_binding(binding);
  std::uint64_t hash = wikimyei_assembly::assembly_detail::kFnvOffsetBasis;
  wikimyei_assembly::assembly_detail::mix_hash_string(
      hash, "cuwacunu.kikijyeba.topology.dock_binding.v1");
  wikimyei_assembly::assembly_detail::mix_hash_string(
      hash, canonical_dock_binding_text(binding));
  return wikimyei_assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string
dock_binding_token(const dock_binding_t &binding) {
  validate_dock_binding(binding);
  return binding.binding_id + "/" + dock_binding_fingerprint(binding);
}

} // namespace cuwacunu::kikijyeba::topology
