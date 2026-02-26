// ./include/tsiemene/tsi.type.registry.h
// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "tsiemene/tsi.directive.registry.h"
#include "tsiemene/tsi.domain.h"

namespace tsiemene {

enum class TsiTypeId : std::uint8_t {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY) TYPE_ID,
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
};

enum class TsiInstancePolicy : std::uint8_t {
  UniquePerCircuit = 0,
  ManyInstances = 1,
  HashimyeiInstances = 2,
};

[[nodiscard]] constexpr std::string_view instance_policy_token(TsiInstancePolicy policy) noexcept {
  switch (policy) {
    case TsiInstancePolicy::UniquePerCircuit: return "unique_per_circuit";
    case TsiInstancePolicy::ManyInstances: return "many_instances";
    case TsiInstancePolicy::HashimyeiInstances: return "hashimyei_instances";
  }
  return "unknown";
}

struct TsiTypeDescriptor {
  TsiTypeId id;
  std::string_view canonical;
  TsiDomain domain;
  TsiInstancePolicy instance_policy;
  std::string_view summary;
};

struct TsiTypeLaneDescriptor {
  TsiTypeId type_id;
  DirectiveSpec lane;
};

struct TsiTypeEndpointDescriptor {
  TsiTypeId type_id;
  DirectiveId directive;
  PayloadKind kind;
  std::string_view summary;
};

inline constexpr std::array kTsiTypeRegistry = {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY) \
  TsiTypeDescriptor{                                                         \
      TsiTypeId::TYPE_ID,                                                    \
      CANONICAL,                                                             \
      TsiDomain::DOMAIN,                                                     \
      TsiInstancePolicy::INSTANCE_POLICY,                                    \
      SUMMARY},
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
};

inline constexpr std::array kTsiTypeLanes = {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY) \
  TsiTypeLaneDescriptor{                                      \
      TsiTypeId::TYPE_ID,                                     \
      directive(directive_id::DIRECTIVE_ID,                   \
                DirectiveDir::DIR,                            \
                KindSpec::KIND(),                             \
                SUMMARY)},
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_LANE
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
};

inline constexpr std::array kTsiTypeEndpoints = {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY) \
  TsiTypeEndpointDescriptor{                                     \
      TsiTypeId::TYPE_ID,                                        \
      directive_id::DIRECTIVE_ID,                                \
      PayloadKind::KIND,                                         \
      SUMMARY},
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
};

[[nodiscard]] constexpr const TsiTypeDescriptor* find_tsi_type(TsiTypeId id) noexcept {
  for (const auto& item : kTsiTypeRegistry) {
    if (item.id == id) return &item;
  }
  return nullptr;
}

[[nodiscard]] constexpr std::size_t tsi_type_index(TsiTypeId id) noexcept {
  for (std::size_t i = 0; i < kTsiTypeRegistry.size(); ++i) {
    if (kTsiTypeRegistry[i].id == id) return i;
  }
  return 0;
}

[[nodiscard]] constexpr std::optional<TsiTypeId> parse_tsi_type_id(std::string_view token) noexcept {
  for (const auto& item : kTsiTypeRegistry) {
    if (token == item.canonical) return item.id;
    if (item.instance_policy == TsiInstancePolicy::HashimyeiInstances) {
      const auto p = item.canonical;
      if (token.size() > p.size() && token.substr(0, p.size()) == p && token[p.size()] == '.') {
        return item.id;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] constexpr std::string_view tsi_type_token(TsiTypeId id) noexcept {
  const auto* item = find_tsi_type(id);
  return item ? item->canonical : std::string_view{"unknown"};
}

[[nodiscard]] constexpr TsiDomain tsi_type_domain(TsiTypeId id) noexcept {
  const auto* item = find_tsi_type(id);
  return item ? item->domain : TsiDomain::Source;
}

[[nodiscard]] constexpr TsiInstancePolicy tsi_type_instance_policy(TsiTypeId id) noexcept {
  const auto* item = find_tsi_type(id);
  return item ? item->instance_policy : TsiInstancePolicy::ManyInstances;
}

[[nodiscard]] constexpr bool is_sink_type(TsiTypeId id) noexcept {
  return tsi_type_domain(id) == TsiDomain::Sink;
}

[[nodiscard]] constexpr bool is_unique_instance_type(TsiTypeId id) noexcept {
  return tsi_type_instance_policy(id) == TsiInstancePolicy::UniquePerCircuit;
}

[[nodiscard]] inline const std::array<std::vector<DirectiveSpec>, kTsiTypeRegistry.size()>&
tsi_type_inputs_cache() {
  static const auto cache = [] {
    std::array<std::vector<DirectiveSpec>, kTsiTypeRegistry.size()> out{};
    for (const auto& lane : kTsiTypeLanes) {
      if (is_in(lane.lane.dir)) {
        out[tsi_type_index(lane.type_id)].push_back(lane.lane);
      }
    }
    return out;
  }();
  return cache;
}

[[nodiscard]] inline const std::array<std::vector<DirectiveSpec>, kTsiTypeRegistry.size()>&
tsi_type_outputs_cache() {
  static const auto cache = [] {
    std::array<std::vector<DirectiveSpec>, kTsiTypeRegistry.size()> out{};
    for (const auto& lane : kTsiTypeLanes) {
      if (is_out(lane.lane.dir)) {
        out[tsi_type_index(lane.type_id)].push_back(lane.lane);
      }
    }
    return out;
  }();
  return cache;
}

[[nodiscard]] inline std::span<const DirectiveSpec> tsi_type_inputs(TsiTypeId id) noexcept {
  const auto& v = tsi_type_inputs_cache()[tsi_type_index(id)];
  return std::span<const DirectiveSpec>(v.data(), v.size());
}

[[nodiscard]] inline std::span<const DirectiveSpec> tsi_type_outputs(TsiTypeId id) noexcept {
  const auto& v = tsi_type_outputs_cache()[tsi_type_index(id)];
  return std::span<const DirectiveSpec>(v.data(), v.size());
}

[[nodiscard]] inline const DirectiveSpec* find_input_spec(TsiTypeId id,
                                                          DirectiveId directive,
                                                          PayloadKind kind) noexcept {
  for (const auto& d : tsi_type_inputs(id)) {
    if (d.id == directive && d.kind.kind == kind && is_in(d.dir)) return &d;
  }
  return nullptr;
}

[[nodiscard]] inline const DirectiveSpec* find_output_spec(TsiTypeId id,
                                                           DirectiveId directive,
                                                           PayloadKind kind) noexcept {
  for (const auto& d : tsi_type_outputs(id)) {
    if (d.id == directive && d.kind.kind == kind && is_out(d.dir)) return &d;
  }
  return nullptr;
}

[[nodiscard]] inline bool type_accepts_input(TsiTypeId id,
                                             DirectiveId directive,
                                             PayloadKind kind) noexcept {
  return find_input_spec(id, directive, kind) != nullptr;
}

[[nodiscard]] inline bool type_emits_output(TsiTypeId id,
                                            DirectiveId directive,
                                            PayloadKind kind) noexcept {
  return find_output_spec(id, directive, kind) != nullptr;
}

[[nodiscard]] inline bool type_accepts_endpoint(TsiTypeId id,
                                                DirectiveId directive,
                                                PayloadKind kind) noexcept {
  for (const auto& ep : kTsiTypeEndpoints) {
    if (ep.type_id == id && ep.directive == directive && ep.kind == kind) return true;
  }
  return false;
}

// Typed counterpart of Tsi::is_compatible(target_in, source_out_kind).
// For now compatibility is strict (exact kind).
[[nodiscard]] inline bool type_is_compatible(TsiTypeId target_type,
                                             DirectiveId target_incoming_directive,
                                             PayloadKind source_outgoing_kind) noexcept {
  return type_accepts_input(target_type, target_incoming_directive, source_outgoing_kind);
}

[[nodiscard]] inline std::optional<DirectiveSpec>
infer_target_input_from_output(TsiTypeId target_type,
                               DirectiveId source_directive,
                               PayloadKind source_kind) noexcept {
  const auto inputs = tsi_type_inputs(target_type);
  const DirectiveSpec* same_name = nullptr;
  std::size_t same_name_count = 0;
  const DirectiveSpec* any_kind = nullptr;
  std::size_t any_kind_count = 0;

  for (const auto& in : inputs) {
    if (!is_in(in.dir)) continue;
    if (in.kind.kind != source_kind) continue;
    ++any_kind_count;
    if (!any_kind) any_kind = &in;
    if (in.id == source_directive) {
      ++same_name_count;
      if (!same_name) same_name = &in;
    }
  }

  if (same_name_count == 1 && same_name) return *same_name;
  if (any_kind_count == 1 && any_kind) return *any_kind;
  return std::nullopt;
}

} // namespace tsiemene
