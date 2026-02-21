#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

using cuwacunu::camahjucunu::tsiemene_board_instruction_t;
using cuwacunu::camahjucunu::tsiemene_resolved_hop_t;

struct BoardState {
  bool ok{false};
  std::string error{};
  std::string raw_instruction{};
  tsiemene_board_instruction_t board{};
  std::vector<std::vector<tsiemene_resolved_hop_t>> resolved_hops{};
  std::size_t selected_circuit{0};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
