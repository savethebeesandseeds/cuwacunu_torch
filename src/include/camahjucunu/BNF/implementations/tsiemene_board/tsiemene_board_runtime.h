/* tsiemene_board_runtime.h */
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "tsiemene/utils/directives.h"

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
bool resolve_hop_decl(const tsiemene_hop_decl_t& hop,
                      tsiemene_resolved_hop_t* out,
                      std::string* error = nullptr);
bool resolve_hops(const tsiemene_circuit_decl_t& circuit,
                  std::vector<tsiemene_resolved_hop_t>* out_hops,
                  std::string* error = nullptr);
bool validate_circuit_decl(const tsiemene_circuit_decl_t& circuit,
                           std::string* error = nullptr);
bool validate_board_instruction(const tsiemene_board_instruction_t& board,
                                std::string* error = nullptr);

} /* namespace camahjucunu */
} /* namespace cuwacunu */
