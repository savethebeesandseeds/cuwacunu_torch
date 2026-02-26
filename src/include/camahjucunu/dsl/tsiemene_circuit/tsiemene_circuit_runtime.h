/* tsiemene_circuit_runtime.h */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "tsiemene/tsi.directive.registry.h"

namespace cuwacunu {
namespace camahjucunu {

struct tsiemene_resolved_endpoint_t {
  std::string instance;
  tsiemene::DirectiveId directive{};
  tsiemene::PayloadKind kind{tsiemene::PayloadKind::Tensor};
};

struct tsiemene_resolved_hop_t {
  tsiemene_resolved_endpoint_t from;
  tsiemene_resolved_endpoint_t to;
};

std::optional<tsiemene::DirectiveId> parse_directive_ref(std::string s);
std::optional<tsiemene::PayloadKind> parse_kind_ref(std::string s);
std::string circuit_invoke_symbol(const tsiemene_circuit_decl_t& circuit);

// Optional wave envelope syntax:
//   wave@episode:2,batch:0,from:01.01.2009,to:31.12.2009,symbol:BTCUSDT@batches=16
// Compact payloads remain valid and are treated as source commands directly.
struct tsiemene_wave_invoke_t {
  std::string source_command{};
  std::string source_symbol{};
  std::uint64_t wave_i{0};
  std::uint64_t episode{0};
  std::uint64_t batch{0};
  bool has_time_span{false};
  std::int64_t span_begin_ms{0};
  std::int64_t span_end_ms{0};
};

bool parse_circuit_invoke_wave(const tsiemene_circuit_decl_t& circuit,
                               tsiemene_wave_invoke_t* out,
                               std::string* error = nullptr);

std::string circuit_invoke_command(const tsiemene_circuit_decl_t& circuit);
bool resolve_hop_decl(const tsiemene_hop_decl_t& hop,
                      tsiemene_resolved_hop_t* out,
                      std::string* error = nullptr);
bool resolve_hops(const tsiemene_circuit_decl_t& circuit,
                  std::vector<tsiemene_resolved_hop_t>* out_hops,
                  std::string* error = nullptr);
bool validate_circuit_decl(const tsiemene_circuit_decl_t& circuit,
                           std::string* error = nullptr);
bool validate_circuit_instruction(const tsiemene_circuit_instruction_t& circuit_instruction,
                                  std::string* error = nullptr);

} /* namespace camahjucunu */
} /* namespace cuwacunu */
