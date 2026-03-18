// test_dsl_tsiemene_circuit.cpp
#include <cassert>
#include <iostream>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "iitepi/runtime_binding_space_t.h"

int main() {
  try {
    const char* global_config_path = "/cuwacunu/src/config/.config";
    cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
    cuwacunu::iitepi::config_space_t::update_config();
    const std::string contract_hash =
        cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
            cuwacunu::iitepi::config_space_t::locked_campaign_hash(),
            cuwacunu::iitepi::config_space_t::locked_binding_id());
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);

    std::string instruction = contract_itself->circuit.dsl;

    TICK(tsiemene_circuit_loadGrammar);
    auto circuits = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(
        contract_itself->circuit.grammar);
    PRINT_TOCK_ns(tsiemene_circuit_loadGrammar);

    TICK(tsiemene_circuit_decodeInstruction);
    auto decoded = circuits.decode(instruction);
    PRINT_TOCK_ns(tsiemene_circuit_decodeInstruction);

    std::cout << "[test_dsl_tsiemene_circuit] instruction:\n";
    std::cout << instruction << "\n";

    std::cout << "[test_dsl_tsiemene_circuit] decoded.circuits.size="
              << decoded.circuits.size() << "\n";
    std::cout << decoded.str() << "\n";

    {
      std::string circuit_error;
      const bool circuit_ok =
          cuwacunu::camahjucunu::validate_circuit_instruction(decoded, &circuit_error);
      std::cout << "[test_dsl_tsiemene_circuit] semantic.circuit.valid="
                << (circuit_ok ? "true" : "false");
      if (!circuit_ok) std::cout << " error=\"" << circuit_error << "\"";
      std::cout << "\n";
      if (!circuit_ok) {
        std::cerr << "[FAIL] expected primary circuit instruction to be semantically valid\n";
        return 1;
      }
    }

    {
      const std::string invalid_unique_instruction =
          "dup_log = {\n"
          "  w_source = tsi.source.dataloader\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log_1 = tsi.probe.log\n"
          "  w_log_2 = tsi.probe.log\n"
          "  w_source@response:cargo -> w_rep@impulse\n"
          "  w_rep@loss:tensor -> w_log_1@info\n"
          "  w_rep@meta:str -> w_log_2@debug\n"
          "}\n";
      auto invalid_decoded = circuits.decode(invalid_unique_instruction);
      std::string invalid_error;
      const bool invalid_ok =
          cuwacunu::camahjucunu::validate_circuit_instruction(invalid_decoded, &invalid_error);
      std::cout << "[test_dsl_tsiemene_circuit] semantic.unique_policy.valid="
                << (invalid_ok ? "true" : "false");
      if (!invalid_error.empty()) std::cout << " error=\"" << invalid_error << "\"";
      std::cout << "\n";
      if (invalid_ok) {
        std::cerr << "[FAIL] expected duplicate unique tsi.probe.log to be rejected\n";
        return 1;
      }
    }

    {
      const std::string mode_instruction =
          "log_mode_parse = {\n"
          "  w_source = tsi.source.dataloader\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log = tsi.probe.log(mode=epoch)\n"
          "  w_source@response:cargo -> w_rep@impulse\n"
          "  w_rep@loss:tensor -> w_log@info\n"
          "}\n";
      auto mode_decoded = circuits.decode(mode_instruction);
      std::string mode_error;
      const bool mode_ok =
          cuwacunu::camahjucunu::validate_circuit_instruction(mode_decoded, &mode_error);
      if (!mode_ok) {
        std::cerr << "[FAIL] expected tsi.probe.log(mode=epoch) to validate: "
                  << mode_error << "\n";
        return 1;
      }
      const auto* parsed_log = [&]() -> const cuwacunu::camahjucunu::tsiemene_instance_decl_t* {
        if (mode_decoded.circuits.empty()) return nullptr;
        for (const auto& inst : mode_decoded.circuits[0].instances) {
          if (inst.alias == "w_log") return &inst;
        }
        return nullptr;
      }();
      if (!parsed_log ||
          parsed_log->args.size() != 1 ||
          parsed_log->args[0] != "mode=epoch") {
        std::cerr << "[FAIL] expected parser to preserve tsi.probe.log instance args\n";
        return 1;
      }
    }

    {
      const std::string multi_circuit_instruction =
          "circuit_batch = {\n"
          "  w_source = tsi.source.dataloader\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log = tsi.probe.log(mode=batch)\n"
          "  w_source@response:cargo -> w_rep@impulse\n"
          "  w_rep@loss:tensor -> w_log@info\n"
          "}\n"
          "circuit_epoch = {\n"
          "  w_source = tsi.source.dataloader\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log = tsi.probe.log(mode=epoch)\n"
          "  w_source@response:cargo -> w_rep@impulse\n"
          "  w_rep@loss:tensor -> w_log@info\n"
          "}\n";
      auto multi_circuit_decoded = circuits.decode(multi_circuit_instruction);
      std::string multi_circuit_error;
      const bool multi_circuit_ok =
          cuwacunu::camahjucunu::validate_circuit_instruction(
              multi_circuit_decoded, &multi_circuit_error);
      if (!multi_circuit_ok) {
        std::cerr << "[FAIL] expected multi-circuit instruction to validate: "
                  << multi_circuit_error << "\n";
        return 1;
      }
      if (multi_circuit_decoded.circuits.size() != 2) {
        std::cerr << "[FAIL] expected multi-circuit instruction to decode both circuits\n";
        return 1;
      }
    }

    {
      const std::string multiline_comments_instruction =
          "# instruction-level comment\n"
          "circuit_comment = {\n"
          "  /* block comment before declarations */\n"
          "  w_source = tsi.source.dataloader\n"
          "  # hash comment before declaration\n"
          "  w_rep = tsi.wikimyei.representation.vicreg.0x0000\n"
          "  w_log = tsi.probe.log(mode=batch)\n"
          "  # multiline hop expression across line break\n"
          "  w_source@response:cargo\n"
          "    -> w_rep@impulse\n"
          "  w_rep@loss:tensor ->\n"
          "    w_log@info\n"
          "  w_rep@meta:str -> w_log@debug # trailing comment\n"
          "}\n";
      auto multiline_decoded = circuits.decode(multiline_comments_instruction);
      std::string multiline_error;
      const bool multiline_ok = cuwacunu::camahjucunu::validate_circuit_instruction(
          multiline_decoded, &multiline_error);
      if (!multiline_ok) {
        std::cerr << "[FAIL] expected multiline/comment circuit to validate: "
                  << multiline_error << "\n";
        return 1;
      }
      if (multiline_decoded.circuits.empty() ||
          multiline_decoded.circuits[0].hops.size() != 3) {
        std::cerr
            << "[FAIL] expected multiline/comment circuit to decode exactly three hops\n";
        return 1;
      }
    }

    {
      cuwacunu::camahjucunu::tsiemene_circuit_decl_t spec{};
      spec.name = "wave_dispatch";
      spec.invoke_name = "wave_dispatch";
      spec.invoke_payload =
          "wave@symbol:BTCUSDT,episode:7,batch:3,max_batches:2,from:01.01.2009,to:31.12.2009@BTCUSDT[01.01.2009,31.12.2009]";
      cuwacunu::camahjucunu::iitepi_wave_invoke_t parsed{};
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
      if (!resolved_ok) {
        std::cerr << "[FAIL] expected hop resolution to pass for decoded circuit\n";
        return 1;
      }
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
