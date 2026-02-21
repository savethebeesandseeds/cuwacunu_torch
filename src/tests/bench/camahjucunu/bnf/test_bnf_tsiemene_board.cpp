// test_bnf_tsiemene_board.cpp
#include <iostream>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::tsiemene_board_instruction();

    TICK(tsiemene_board_loadGrammar);
    auto board = cuwacunu::camahjucunu::BNF::tsiemeneBoard();
    PRINT_TOCK_ns(tsiemene_board_loadGrammar);

    TICK(tsiemene_board_decodeInstruction);
    auto decoded = board.decode(instruction);
    PRINT_TOCK_ns(tsiemene_board_decodeInstruction);

    std::cout << "[test_bnf_tsiemene_board] instruction:\n";
    std::cout << instruction << "\n";

    std::cout << "[test_bnf_tsiemene_board] decoded.circuits.size="
              << decoded.circuits.size() << "\n";
    std::cout << decoded.str() << "\n";

    {
      std::string board_error;
      const bool board_ok = cuwacunu::camahjucunu::validate_board_instruction(decoded, &board_error);
      std::cout << "[test_bnf_tsiemene_board] semantic.board.valid="
                << (board_ok ? "true" : "false");
      if (!board_ok) std::cout << " error=\"" << board_error << "\"";
      std::cout << "\n";
    }

    for (std::size_t ci = 0; ci < decoded.circuits.size(); ++ci) {
      const auto& c = decoded.circuits[ci];
      std::cout << "[circuit " << ci << "] name=" << c.name << "\n";
      std::cout << "[circuit " << ci << "] invoke=" << c.invoke_name
                << "(\"" << c.invoke_payload << "\")\n";
      std::cout << "[circuit " << ci << "] invoke_symbol="
                << cuwacunu::camahjucunu::circuit_invoke_symbol(c) << "\n";

      std::cout << "[circuit " << ci << "] instances.size="
                << c.instances.size() << "\n";
      for (std::size_t ii = 0; ii < c.instances.size(); ++ii) {
        const auto& inst = c.instances[ii];
        std::cout << "  [instance " << ii << "] alias=" << inst.alias
                  << " type=" << inst.tsi_type << "\n";
      }

      std::cout << "[circuit " << ci << "] hops.size="
                << c.hops.size() << "\n";
      for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
        const auto& h = c.hops[hi];
        std::cout << "  [hop " << hi << "] "
                  << h.from.instance << "@" << h.from.directive << ":" << h.from.kind
                  << " -> "
                  << h.to.instance << "@" << h.to.directive << ":" << h.to.kind
                  << "\n";
      }

      std::string circuit_error;
      const bool circuit_ok = cuwacunu::camahjucunu::validate_circuit_decl(c, &circuit_error);
      std::cout << "[circuit " << ci << "] semantic.valid="
                << (circuit_ok ? "true" : "false");
      if (!circuit_ok) std::cout << " error=\"" << circuit_error << "\"";
      std::cout << "\n";

      std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved;
      std::string resolve_error;
      const bool resolved_ok = cuwacunu::camahjucunu::resolve_hops(c, &resolved, &resolve_error);
      std::cout << "[circuit " << ci << "] resolved_hops.ok="
                << (resolved_ok ? "true" : "false");
      if (!resolved_ok) std::cout << " error=\"" << resolve_error << "\"";
      std::cout << " count=" << resolved.size() << "\n";
      for (std::size_t r = 0; r < resolved.size(); ++r) {
        const auto& h = resolved[r];
        std::cout << "  [resolved " << r << "] "
                  << h.from.instance << h.from.directive
                  << tsiemene::kind_token(h.from.kind)
                  << " -> "
                  << h.to.instance << h.to.directive
                  << tsiemene::kind_token(h.to.kind)
                  << "\n";
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_bnf_tsiemene_board] exception: " << e.what() << "\n";
    return 1;
  }
}
