// test_dsl_tsiemene_board.cpp
#include <cassert>
#include <iostream>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "piaabo/dconfig.h"

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();
    cuwacunu::iitepi::board_space_t::init();
    const std::string board_hash =
        cuwacunu::iitepi::board_space_t::locked_board_hash();
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);

    const std::string grammar = board_itself->board.grammar;
    const std::string instruction = board_itself->board.dsl;

    TICK(tsiemene_board_decode);
    const auto decoded = cuwacunu::camahjucunu::dsl::decode_tsiemene_board_from_dsl(
        grammar, instruction);
    PRINT_TOCK_ns(tsiemene_board_decode);

    std::cout << "[test_dsl_tsiemene_board] instruction:\n";
    std::cout << instruction << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.contracts.empty());
    assert(!decoded.waves.empty());
    assert(!decoded.binds.empty());
    for (const auto& b : decoded.binds) {
      assert(!b.contract_ref.empty());
      assert(!b.wave_ref.empty());
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_tsiemene_board] exception: " << e.what() << "\n";
    return 1;
  }
}
