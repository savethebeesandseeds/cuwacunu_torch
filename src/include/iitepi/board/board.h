// ./include/iitepi/board.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "iitepi/board/board.contract.h"
#include "tsiemene/tsi.type.registry.h"
#include "tsiemene/tsi.wikimyei.h"

namespace tsiemene {

struct Board {
  std::string board_hash{};
  std::string board_path{};
  std::string board_binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::vector<BoardContract> contracts{};

  Board() = default;
  Board(Board&& other) noexcept
      : board_hash(std::move(other.board_hash)),
        board_path(std::move(other.board_path)),
        board_binding_id(std::move(other.board_binding_id)),
        contract_hash(std::move(other.contract_hash)),
        wave_hash(std::move(other.wave_hash)),
        contracts(std::move(other.contracts)) {}
  Board& operator=(Board&& other) noexcept {
    if (this != &other) {
      board_hash = std::move(other.board_hash);
      board_path = std::move(other.board_path);
      board_binding_id = std::move(other.board_binding_id);
      contract_hash = std::move(other.contract_hash);
      wave_hash = std::move(other.wave_hash);
      contracts = std::move(other.contracts);
    }
    return *this;
  }

  Board(const Board&) = delete;
  Board& operator=(const Board&) = delete;
};

[[nodiscard]] inline DirectiveId pick_start_directive(const Circuit& c) noexcept {
  if (!c.hops || c.hop_count == 0 || !c.hops[0].from.tsi) {
    return directive_id::Step;
  }

  const Tsi& t = *c.hops[0].from.tsi;
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In && d.kind.kind == PayloadKind::String) return d.id;
  }
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In) return d.id;
  }
  return directive_id::Step;
}

[[nodiscard]] inline bool validate_circuit(const BoardContract& c, CircuitIssue* issue = nullptr) noexcept {
  return validate(c.view(), issue);
}

struct BoardIssue {
  std::string_view what{};
  std::size_t contract_index{};
  std::size_t circuit_index{};
  CircuitIssue circuit_issue{};
};

[[nodiscard]] inline bool is_known_canonical_component_type(
    std::string_view canonical_type,
    TsiTypeId* out = nullptr) noexcept {
  const auto id = parse_tsi_type_id(canonical_type);
  if (!id) return false;
  const auto* desc = find_tsi_type(*id);
  if (!desc) return false;
  if (desc->canonical != canonical_type) return false;
  if (out) *out = *id;
  return true;
}

[[nodiscard]] inline bool runtime_node_canonical_type(
    const Tsi& node,
    std::string* out) noexcept {
  if (!out) return false;
  const std::string_view raw = node.type_name();
  const auto id = parse_tsi_type_id(raw);
  if (!id) return false;
  const auto* desc = find_tsi_type(*id);
  if (!desc) return false;
  *out = std::string(desc->canonical);
  return true;
}

[[nodiscard]] inline bool validate_board(const Board& b, BoardIssue* issue = nullptr) noexcept {
  if (b.contracts.empty()) {
    if (issue) {
      issue->what = "empty board";
      issue->contract_index = 0;
      issue->circuit_index = 0;
      issue->circuit_issue = CircuitIssue{.what = "empty board", .hop_index = 0};
    }
    return false;
  }

  for (std::size_t i = 0; i < b.contracts.size(); ++i) {
    const BoardContract& c = b.contracts[i];
    auto fail = [&](std::string_view what, std::size_t hop_index) noexcept {
      if (issue) {
        issue->what = what;
        issue->contract_index = i;
        issue->circuit_index = i;
        issue->circuit_issue = CircuitIssue{.what = what, .hop_index = hop_index};
      }
      return false;
    };

    if (c.name.empty()) return fail("contract circuit name is empty", 0);
    if (c.invoke_name.empty()) return fail("contract invoke_name is empty", 0);
    if (c.invoke_payload.empty()) return fail("contract invoke_payload is empty", 0);
    if (c.nodes.empty()) return fail("contract has no nodes", 0);
    std::string_view missing_dsl{};
    if (!c.has_required_dsl_segments(&missing_dsl)) {
      if (missing_dsl == kBoardContractCircuitDslKey) {
        return fail("contract missing board.contract.circuit@DSL:str", 0);
      }
      if (missing_dsl == kBoardContractObservationSourcesDslKey) {
        return fail("contract missing board.contract.observation_sources@DSL:str", 0);
      }
      if (missing_dsl == kBoardContractObservationChannelsDslKey) {
        return fail("contract missing board.contract.observation_channels@DSL:str", 0);
      }
      if (missing_dsl == kBoardContractJkimyeiSpecsDslKey) {
        return fail("contract missing board.contract.jkimyei_specs@DSL:str", 0);
      }
      return fail("contract missing required DSL segment", 0);
    }

    std::unordered_set<const Tsi*> owned_nodes;
    std::unordered_set<TsiId> node_ids;
    owned_nodes.reserve(c.nodes.size());
    node_ids.reserve(c.nodes.size());
    std::unordered_set<std::string> runtime_component_types;
    std::unordered_set<std::string> runtime_source_types;
    std::unordered_set<std::string> runtime_representation_types;
    runtime_component_types.reserve(c.nodes.size());
    runtime_source_types.reserve(c.nodes.size());
    runtime_representation_types.reserve(c.nodes.size());
    std::size_t runtime_source_count = 0;
    std::size_t runtime_representation_count = 0;
    for (const auto& n : c.nodes) {
      if (!n) return fail("null node in contract nodes", 0);
      if (!owned_nodes.insert(n.get()).second) return fail("duplicated node pointer in contract nodes", 0);
      if (!node_ids.insert(n->id()).second) return fail("duplicated tsi id in contract nodes", 0);

      std::string canonical_runtime_type;
      if (runtime_node_canonical_type(*n, &canonical_runtime_type)) {
        runtime_component_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Source) runtime_source_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Wikimyei) runtime_representation_types.insert(canonical_runtime_type);
      }
      if (n->domain() == TsiDomain::Source) ++runtime_source_count;
      if (n->domain() == TsiDomain::Wikimyei) ++runtime_representation_count;
    }

    std::unordered_set<const Tsi*> wired_nodes;
    wired_nodes.reserve(c.nodes.size());
    for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
      const Hop& h = c.hops[hi];
      if (!owned_nodes.count(h.from.tsi) || !owned_nodes.count(h.to.tsi)) {
        return fail("hop endpoint is not owned by contract nodes", hi);
      }
      wired_nodes.insert(h.from.tsi);
      wired_nodes.insert(h.to.tsi);
    }
    if (wired_nodes.size() != owned_nodes.size()) {
      return fail("orphan node not referenced by any contract hop", 0);
    }

    CircuitIssue ci{};
    if (!validate_circuit(c, &ci)) {
      if (issue) {
        issue->what = "invalid circuit";
        issue->contract_index = i;
        issue->circuit_index = i;
        issue->circuit_issue = ci;
      }
      return false;
    }

    const Circuit cv = c.view();
    if (!cv.hops || cv.hop_count == 0 || !cv.hops[0].from.tsi) {
      return fail("contract has no start tsi", 0);
    }

    if (c.seed_ingress.directive.empty()) return fail("contract seed_ingress.directive is empty", 0);

    const DirectiveSpec* start_in =
        find_directive(*cv.hops[0].from.tsi, c.seed_ingress.directive, DirectiveDir::In);
    if (!start_in) return fail("contract seed_ingress directive not found on root tsi", 0);

    if (start_in->kind.kind != c.seed_ingress.signal.kind) {
      return fail("contract seed_ingress kind mismatch with root tsi input", 0);
    }

    if (!c.spec.sourced_from_config) continue;

    if (c.spec.sample_type.empty()) return fail("contract spec.sample_type is empty", 0);
    if (runtime_source_count > 0 && c.spec.instrument.empty()) {
      return fail("contract spec.instrument is empty", 0);
    }
    if (runtime_source_count > 0 && c.spec.source_type.empty()) {
      return fail("contract spec.source_type is empty", 0);
    }
    if (runtime_representation_count > 0 && c.spec.representation_type.empty()) {
      return fail("contract spec.representation_type is empty", 0);
    }
    if (c.spec.component_types.empty()) {
      return fail("contract spec.component_types is empty", 0);
    }
    if (c.spec.future_timesteps < 0) {
      return fail("contract spec.future_timesteps must be >= 0", 0);
    }

    std::unordered_set<std::string> spec_component_types;
    spec_component_types.reserve(c.spec.component_types.size());
    for (const auto& type_name : c.spec.component_types) {
      if (type_name.empty()) return fail("contract spec.component_types has empty type", 0);
      if (!spec_component_types.insert(type_name).second) {
        return fail("contract spec.component_types has duplicate type", 0);
      }
      if (!is_known_canonical_component_type(type_name, nullptr)) {
        return fail("contract spec.component_types has unknown canonical type", 0);
      }
    }

    if (!c.spec.source_type.empty()) {
      TsiTypeId source_id{};
      if (!is_known_canonical_component_type(c.spec.source_type, &source_id)) {
        return fail("contract spec.source_type is not canonical/known", 0);
      }
      if (tsi_type_domain(source_id) != TsiDomain::Source) {
        return fail("contract spec.source_type domain mismatch", 0);
      }
      if (!runtime_source_types.empty() &&
          runtime_source_types.find(c.spec.source_type) == runtime_source_types.end()) {
        return fail("contract spec.source_type does not match runtime source nodes", 0);
      }
      if (c.spec.source_type == std::string(tsi_type_token(TsiTypeId::SourceDataloader)) &&
          !c.spec.has_positive_shape_hints()) {
        return fail("contract spec dataloader shape hints are incomplete", 0);
      }
    }

    if (!c.spec.representation_type.empty()) {
      TsiTypeId rep_id{};
      if (!is_known_canonical_component_type(c.spec.representation_type, &rep_id)) {
        return fail("contract spec.representation_type is not canonical/known", 0);
      }
      if (tsi_type_domain(rep_id) != TsiDomain::Wikimyei) {
        return fail("contract spec.representation_type domain mismatch", 0);
      }
      if (!runtime_representation_types.empty() &&
          runtime_representation_types.find(c.spec.representation_type) ==
              runtime_representation_types.end()) {
        return fail("contract spec.representation_type does not match runtime wikimyei nodes", 0);
      }
      if (tsi_type_instance_policy(rep_id) == TsiInstancePolicy::HashimyeiInstances &&
          runtime_representation_count > 0 &&
          c.spec.representation_hashimyei.empty()) {
        return fail("contract spec.representation_hashimyei is empty for hashimyei type", 0);
      }
    }

    if (!c.spec.source_type.empty() &&
        spec_component_types.find(c.spec.source_type) == spec_component_types.end()) {
      return fail("contract spec.source_type missing in spec.component_types", 0);
    }
    if (!c.spec.representation_type.empty() &&
        spec_component_types.find(c.spec.representation_type) == spec_component_types.end()) {
      return fail("contract spec.representation_type missing in spec.component_types", 0);
    }

    if (!runtime_component_types.empty()) {
      for (const auto& runtime_type : runtime_component_types) {
        if (spec_component_types.find(runtime_type) == spec_component_types.end()) {
          return fail("runtime canonical component missing from spec.component_types", 0);
        }
      }
      for (const auto& spec_type : spec_component_types) {
        if (runtime_component_types.find(spec_type) == runtime_component_types.end()) {
          return fail("spec.component_types contains type absent from runtime graph", 0);
        }
      }
    }
  }
  return true;
}

inline bool load_contract_wikimyei_artifacts(BoardContract& c,
                                             std::string* error = nullptr);
inline bool save_contract_wikimyei_artifacts(BoardContract& c,
                                             std::string* error = nullptr);

inline std::uint64_t run_circuit(BoardContract& c, BoardContext& ctx) {
  if (!c.ensure_compiled(nullptr)) return 0;
  std::string artifact_error;
  if (!load_contract_wikimyei_artifacts(c, &artifact_error)) return 0;
  const std::uint64_t steps =
      run_wave_compiled(c.compiled_runtime, c.seed_wave, c.seed_ingress, ctx);
  for (auto& node : c.nodes) {
    if (node) node->on_epoch_end(ctx);
  }
  if (!save_contract_wikimyei_artifacts(c, &artifact_error)) return 0;
  return steps;
}

inline void reset_contract_nodes(BoardContract& c, BoardContext& ctx) {
  for (auto& node : c.nodes) {
    if (node) node->reset(ctx);
  }
}

[[nodiscard]] inline Wave wave_for_epoch(const Wave& seed, std::uint64_t epoch_index) {
  Wave out = seed;
  if (epoch_index >
      (std::numeric_limits<std::uint64_t>::max() - seed.cursor.episode)) {
    out.cursor.episode = std::numeric_limits<std::uint64_t>::max();
  } else {
    out.cursor.episode = seed.cursor.episode + epoch_index;
  }
  return out;
}

inline bool load_contract_wikimyei_artifacts(BoardContract& c,
                                             std::string* error) {
  if (error) error->clear();
  if (c.spec.representation_hashimyei.empty()) return true;

  for (auto& node : c.nodes) {
    auto* wik = dynamic_cast<TsiWikimyei*>(node.get());
    if (!wik) continue;
    if (!wik->supports_init_artifacts()) continue;
    if (!wik->runtime_autoload_artifacts()) continue;

    std::string local_error;
    if (!wik->runtime_load_from_hashimyei(c.spec.representation_hashimyei,
                                          &local_error)) {
      if (error) {
        *error = "failed to load wikimyei artifacts for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}

inline bool save_contract_wikimyei_artifacts(BoardContract& c,
                                             std::string* error) {
  if (error) error->clear();
  if (c.spec.representation_hashimyei.empty()) return true;

  for (auto& node : c.nodes) {
    auto* wik = dynamic_cast<TsiWikimyei*>(node.get());
    if (!wik) continue;
    if (!wik->supports_init_artifacts()) continue;
    if (!wik->runtime_autosave_artifacts()) continue;

    std::string local_error;
    if (!wik->runtime_save_to_hashimyei(c.spec.representation_hashimyei,
                                        &local_error)) {
      if (error) {
        *error = "failed to save wikimyei artifacts for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}

inline std::uint64_t run_contract(BoardContract& c, BoardContext& ctx) {
  if (!c.ensure_compiled(nullptr)) return 0;
  std::string artifact_error;
  if (!load_contract_wikimyei_artifacts(c, &artifact_error)) return 0;
  const std::uint64_t epochs = std::max<std::uint64_t>(1, c.execution.epochs);
  std::uint64_t total_steps = 0;
  for (std::uint64_t epoch = 0; epoch < epochs; ++epoch) {
    reset_contract_nodes(c, ctx);
    const Wave start_wave = wave_for_epoch(c.seed_wave, epoch);
    total_steps +=
        run_wave_compiled(c.compiled_runtime, start_wave, c.seed_ingress, ctx);
    for (auto& node : c.nodes) {
      if (node) node->on_epoch_end(ctx);
    }
  }
  if (!save_contract_wikimyei_artifacts(c, &artifact_error)) return 0;
  return total_steps;
}

} // namespace tsiemene
