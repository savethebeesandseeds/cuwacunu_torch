// test_dsl_tsiemene_circuit.cpp
#include <cassert>
#include <iostream>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    const std::string contract_hash =
        cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();

    std::string instruction =
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_dsl(
            contract_hash);

    TICK(tsiemene_circuit_loadGrammar);
    auto board = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_grammar(
            contract_hash));
    PRINT_TOCK_ns(tsiemene_circuit_loadGrammar);

    TICK(tsiemene_circuit_decodeInstruction);
    auto decoded = board.decode(instruction);
    PRINT_TOCK_ns(tsiemene_circuit_decodeInstruction);

    std::cout << "[test_dsl_tsiemene_circuit] instruction:\n";
    std::cout << instruction << "\n";

    std::cout << "[test_dsl_tsiemene_circuit] decoded.circuits.size="
              << decoded.circuits.size() << "\n";
    std::cout << decoded.str() << "\n";

    {
      std::string board_error;
      const bool board_ok = cuwacunu::camahjucunu::validate_circuit_instruction(decoded, &board_error);
      std::cout << "[test_dsl_tsiemene_circuit] semantic.board.valid="
                << (board_ok ? "true" : "false");
      if (!board_ok) std::cout << " error=\"" << board_error << "\"";
      std::cout << "\n";
    }

    {
      const std::string invalid_unique_instruction =
          "dup_log = {\n"
          "  w_source = tsi.source.dataloader\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log_1 = tsi.sink.log.sys\n"
          "  w_log_2 = tsi.sink.log.sys\n"
          "  w_source@payload:tensor -> w_rep@step\n"
          "  w_rep@loss:tensor -> w_log_1@info\n"
          "  w_rep@meta:str -> w_log_2@debug\n"
          "}\n";
      auto invalid_decoded = board.decode(invalid_unique_instruction);
      std::string invalid_error;
      const bool invalid_ok =
          cuwacunu::camahjucunu::validate_circuit_instruction(invalid_decoded, &invalid_error);
      std::cout << "[test_dsl_tsiemene_circuit] semantic.unique_policy.valid="
                << (invalid_ok ? "true" : "false");
      if (!invalid_error.empty()) std::cout << " error=\"" << invalid_error << "\"";
      std::cout << "\n";
      if (invalid_ok) {
        std::cerr << "[FAIL] expected duplicate unique sink.log.sys to be rejected\n";
        return 1;
      }
    }

    {
      cuwacunu::camahjucunu::tsiemene_circuit_decl_t spec{};
      spec.name = "wave_dispatch";
      spec.invoke_name = "wave_dispatch";
      spec.invoke_payload =
          "wave@symbol:BTCUSDT,episode:7,batch:3,max_batches:2,from:01.01.2009,to:31.12.2009@BTCUSDT[01.01.2009,31.12.2009]";
      cuwacunu::camahjucunu::tsiemene_wave_invoke_t parsed{};
      std::string invoke_error;
      const bool ok = cuwacunu::camahjucunu::parse_circuit_invoke_wave(spec, &parsed, &invoke_error);
      std::cout << "[test_dsl_tsiemene_circuit] invoke.wave.parse=" << (ok ? "true" : "false");
      if (!invoke_error.empty()) std::cout << " error=\"" << invoke_error << "\"";
      std::cout << "\n";
      if (!ok) return 1;
      assert(parsed.source_symbol == "BTCUSDT");
      assert(parsed.source_command == "BTCUSDT[01.01.2009,31.12.2009]");
      assert(parsed.episode == 7);
      assert(parsed.batch == 3);
      assert(parsed.max_batches_per_epoch == 2);
      assert(parsed.wave_i == 3);
      assert(parsed.has_time_span);
      assert(cuwacunu::camahjucunu::circuit_invoke_symbol(spec) == "BTCUSDT");
      assert(cuwacunu::camahjucunu::circuit_invoke_command(spec) ==
             "BTCUSDT[01.01.2009,31.12.2009]");
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
                  << " -> " << h.to.instance;
        if (!h.to.directive.empty()) {
          std::cout << "@" << h.to.directive;
        }
        std::cout << "\n";
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
        std::cout << "  [resolved " << r << "] out["
                  << h.from.instance << h.from.directive
                  << tsiemene::kind_token(h.from.kind)
                  << "] -> in["
                  << h.to.instance << h.to.directive
                  << tsiemene::kind_token(h.to.kind)
                  << "]\n";
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_tsiemene_circuit] exception: " << e.what() << "\n";
    return 1;
  }
}
