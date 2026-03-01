// ./include/iitepi/board.contract.h
// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iitepi/board/board.runtime.h"

namespace tsiemene {

namespace board_contract_dsl_key {
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY) inline constexpr const char ID[] = KEY;
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
} // namespace board_contract_dsl_key

inline constexpr const char* kBoardContractCircuitDslKey =
    board_contract_dsl_key::ContractCircuit;
inline constexpr const char* kBoardContractObservationSourcesDslKey =
    board_contract_dsl_key::ContractObservationSources;
inline constexpr const char* kBoardContractObservationChannelsDslKey =
    board_contract_dsl_key::ContractObservationChannels;
inline constexpr const char* kBoardContractJkimyeiSpecsDslKey =
    board_contract_dsl_key::ContractJkimyeiSpecs;
inline constexpr const char* kBoardContractWaveDslKey =
    board_contract_dsl_key::ContractWave;

inline constexpr std::array<std::string_view, 5> kBoardContractRequiredDslKeys = {
    kBoardContractCircuitDslKey,
    kBoardContractObservationSourcesDslKey,
    kBoardContractObservationChannelsDslKey,
    kBoardContractJkimyeiSpecsDslKey,
    kBoardContractWaveDslKey,
};

// Runtime circuit payload owned by a board contract.
struct BoardContractCircuit {
  std::string name{};
  std::string invoke_name{};
  std::string invoke_payload{};
  std::string invoke_source_command{};

  std::vector<std::unique_ptr<Tsi>> nodes{};
  std::vector<Hop> hops{};

  // Default execution seed for this circuit.
  Wave seed_wave{};
  Ingress seed_ingress{};

  // Persistent runtime cache for fast routing.
  CompiledCircuit compiled_runtime{};
  std::size_t compiled_signature{};
  bool compiled_ready{false};
  std::uint64_t compiled_build_count{};

  void invalidate_compiled_runtime() noexcept {
    compiled_runtime = CompiledCircuit{};
    compiled_signature = 0;
    compiled_ready = false;
  }

  template <class NodeT, class... Args>
  NodeT& emplace_node(Args&&... args) {
    auto p = std::make_unique<NodeT>(std::forward<Args>(args)...);
    NodeT& ref = *p;
    nodes.push_back(std::move(p));
    invalidate_compiled_runtime();
    return ref;
  }

  [[nodiscard]] Circuit view() const noexcept {
    return Circuit{
      .hops = hops.data(),
      .hop_count = hops.size(),
      .doc = name
    };
  }

  [[nodiscard]] std::size_t topology_signature() const noexcept {
    return circuit_topology_signature(view());
  }

  [[nodiscard]] bool ensure_compiled(CircuitIssue* issue = nullptr) {
    const Circuit cv = view();
    const std::size_t sig = circuit_topology_signature(cv);
    if (compiled_ready && sig == compiled_signature) return true;

    CompiledCircuit fresh{};
    if (!compile_circuit(cv, &fresh, issue)) {
      compiled_ready = false;
      return false;
    }

    compiled_runtime = std::move(fresh);
    compiled_signature = sig;
    compiled_ready = true;
    ++compiled_build_count;
    return true;
  }
};

// First-class board coordination contract.
// It wraps one executable circuit plus runtime/build metadata used to
// coordinate source/sample/wave/component dimensions coherently.
struct BoardContract : public BoardContractCircuit {
  using DslSegments = std::map<std::string, std::string>;

  struct Execution {
    // Wave execution controls selected for this contract.
    std::uint64_t epochs{1};
    std::uint64_t batch_size{0};
  };

  struct Spec {
    // Source identity for this contract's data stream (e.g., BTCUSDT).
    std::string instrument{};
    // Concrete sample record type used by source dataloader.
    std::string sample_type{};
    // Canonical source tsi type (manifest aligned).
    std::string source_type{};
    // Canonical wikimyei tsi type (manifest aligned).
    std::string representation_type{};
    // Optional hashimyei identifier for hashimyei-based representation types.
    std::string representation_hashimyei{};
    // Selected runtime training component key resolved for wikimyei representation.
    std::string representation_component_name{};
    // Canonical component type set present in this contract circuit.
    std::vector<std::string> component_types{};

    // Training toggles are intentionally kept as soft knobs for now.
    bool vicreg_train{true};
    bool vicreg_use_swa{true};
    bool vicreg_detach_to_cpu{true};

    // Shape hints coordinated across source/representation wiring.
    std::int64_t batch_size_hint{0};
    std::int64_t channels{0};
    std::int64_t timesteps{0};
    std::int64_t features{0};
    std::int64_t future_timesteps{0};

    // True when built from config/DSL and expected to pass strict spec checks.
    bool sourced_from_config{true};

    [[nodiscard]] bool has_positive_shape_hints() const noexcept {
      return batch_size_hint > 0 && channels > 0 && timesteps > 0 && features > 0;
    }
  } spec{};
  Execution execution{};
  DslSegments dsl_segments{};

  [[nodiscard]] static constexpr const std::array<std::string_view, 5>& required_dsl_keys() noexcept {
    return kBoardContractRequiredDslKeys;
  }

  void set_dsl_segment(std::string key, std::string dsl_text) {
    dsl_segments[std::move(key)] = std::move(dsl_text);
  }

  [[nodiscard]] const std::string* find_dsl_segment(std::string_view key) const noexcept {
    const auto it = dsl_segments.find(std::string(key));
    return (it == dsl_segments.end()) ? nullptr : &it->second;
  }

  [[nodiscard]] std::string dsl_segment_or(std::string_view key,
                                           std::string fallback = {}) const {
    if (const auto* value = find_dsl_segment(key)) return *value;
    return fallback;
  }

  [[nodiscard]] bool has_non_empty_dsl_segment(std::string_view key) const noexcept {
    const auto* value = find_dsl_segment(key);
    return value && !value->empty();
  }

  [[nodiscard]] bool has_required_dsl_segments(std::string_view* missing_key = nullptr) const noexcept {
    for (const std::string_view key : required_dsl_keys()) {
      if (!has_non_empty_dsl_segment(key)) {
        if (missing_key) *missing_key = key;
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::string render_dsl_segment(std::string_view key) const {
    const auto* value = find_dsl_segment(key);
    if (!value) return {};

    std::ostringstream oss;
    oss << "BEGIN " << key << "\n";
    oss << *value;
    if (value->empty() || value->back() != '\n') oss << "\n";
    oss << "END " << key << "\n";
    return oss.str();
  }

  [[nodiscard]] std::string render_dsl_segments() const {
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : dsl_segments) {
      if (!first) oss << "\n";
      first = false;
      oss << "BEGIN " << key << "\n";
      oss << value;
      if (value.empty() || value.back() != '\n') oss << "\n";
      oss << "END " << key << "\n";
    }
    return oss.str();
  }

  [[nodiscard]] BoardContractCircuit& circuit() noexcept {
    return static_cast<BoardContractCircuit&>(*this);
  }
  [[nodiscard]] const BoardContractCircuit& circuit() const noexcept {
    return static_cast<const BoardContractCircuit&>(*this);
  }
};

} // namespace tsiemene
