#pragma once

#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"

namespace cuwacunu {
namespace camahjucunu {

using tsiemene_board_contract_decl_t = runtime_binding_contract_decl_t;
using tsiemene_board_wave_decl_t = runtime_binding_wave_decl_t;
using tsiemene_board_bind_decl_t = runtime_binding_bind_decl_t;
using tsiemene_board_instruction_t = runtime_binding_instruction_t;

namespace dsl {

using tsiemeneBoards = runtimeBindingPipeline;

inline tsiemene_board_instruction_t decode_tsiemene_board_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  return decode_runtime_binding_from_dsl(
      std::move(grammar_text), std::move(instruction_text));
}

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
