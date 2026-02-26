// test_tsi_board.cpp
//
// Walkthrough test:
// - explains the board/circuit DSL syntax
// - parses + validates instruction semantics
// - builds runtime board with typed tsi registry
// - prints runtime topology (nodes, directives, hops)
// - executes each circuit and prints event counts

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <torch/cuda.h>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

#include "tsiemene/board.builder.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

using Datatype = cuwacunu::camahjucunu::exchange::kline_t;
using Sampler = torch::data::samplers::SequentialSampler;

void section(const std::string& title) {
  std::cout << "\n============================================================\n";
  std::cout << title << "\n";
  std::cout << "============================================================\n";
}

std::string dir_token(tsiemene::DirectiveDir d) {
  return tsiemene::is_in(d) ? "in" : "out";
}

void print_syntax_quick_reference() {
  section("TSIEMENE BOARD DSL QUICK REFERENCE");
  std::cout
      << "1) Circuit block\n"
      << "   <circuit_name> = { ... }\n\n"
      << "2) Instance declaration\n"
      << "   <alias> = <tsi_type>\n"
      << "   Example: w_source = tsi.source.dataloader\n\n"
      << "3) Hop declaration\n"
      << "   <from_alias>@<out_directive>:<kind> -> <to_alias>@<in_directive>\n"
      << "   Target kind is inferred from source kind and cannot be written on RHS.\n"
      << "   Example: w_source@payload:tensor -> w_rep@step\n\n"
      << "4) Circuit invocation\n"
      << "   <circuit_name>(<payload>);\n"
      << "   Example: circuit_1(BTCUSDT[01.01.2009,31.12.2009]);\n\n"
      << "   Optional wave envelope: circuit_1(wave@symbol:BTCUSDT,episode:1,batch:0@batches=8);\n\n"
      << "5) Directives + kinds in this system\n"
      << "   directives: @step, @payload, @future, @loss, @meta, @info, @warn, @debug, @error, @init, @jkimyei, @weights\n"
      << "   kinds:      :tensor, :str\n"
      << "   note: same directive token can exist on out and in ports; direction gives meaning.\n";
}

void print_tsi_registry() {
  section("TYPED TSI TYPE REGISTRY (builder + DSL semantic layer)");
  std::cout << "Supported canonical tsi_type values:\n";
  for (const auto& item : tsiemene::kTsiTypeRegistry) {
    std::cout << "  - " << item.canonical
              << " | domain=" << tsiemene::domain_token(item.domain)
              << " | instances=" << tsiemene::instance_policy_token(item.instance_policy)
              << "\n";
  }
  std::cout << "Note: tsi.wikimyei.* canonicalization may append hash suffixes.\n";
}

void print_instruction_summary(const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& decoded) {
  section("DECODED INSTRUCTION SUMMARY");
  std::cout << decoded.str() << "\n";

  for (std::size_t ci = 0; ci < decoded.circuits.size(); ++ci) {
    const auto& c = decoded.circuits[ci];
    std::cout << "[circuit " << ci << "] name=" << c.name
              << " invoke=" << c.invoke_name
              << "(\"" << c.invoke_payload << "\")\n";
    std::cout << "  invoke symbol: "
              << cuwacunu::camahjucunu::circuit_invoke_symbol(c) << "\n";

    std::cout << "  instances:\n";
    for (const auto& inst : c.instances) {
      const auto path = cuwacunu::camahjucunu::decode_canonical_path(inst.tsi_type);
      std::cout << "    - alias=" << inst.alias
                << " raw_type=" << inst.tsi_type << "\n";
      if (!path.ok) {
        std::cout << "      canonical_path: INVALID (" << path.error << ")\n";
      } else {
        const auto type_id = tsiemene::parse_tsi_type_id(path.canonical_identity);
        std::cout << "      canonical_identity=" << path.canonical_identity << "\n";
        std::cout << "      typed_registry_match="
                  << (type_id.has_value() ? "yes" : "no");
        if (type_id.has_value()) {
          std::cout << " (" << tsiemene::tsi_type_token(*type_id) << ")";
        }
        std::cout << "\n";
      }
    }

    std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved;
    std::string resolve_error;
    const bool resolved_ok =
        cuwacunu::camahjucunu::resolve_hops(c, &resolved, &resolve_error);

    std::cout << "  hops:\n";
    for (const auto& h : c.hops) {
      std::cout << "    - " << h.from.instance << "@" << h.from.directive << ":" << h.from.kind
                << " -> " << h.to.instance;
      if (!h.to.directive.empty()) {
        std::cout << "@" << h.to.directive;
      }
      std::cout << "\n";
    }

    std::cout << "  resolved hop types: " << (resolved_ok ? "ok" : "error") << "\n";
    if (!resolved_ok) {
      std::cout << "    resolve error: " << resolve_error << "\n";
    } else {
      for (const auto& h : resolved) {
        std::cout << "    - out[" << h.from.instance
                  << h.from.directive << tsiemene::kind_token(h.from.kind)
                  << "] -> in["
                  << h.to.instance
                  << h.to.directive << tsiemene::kind_token(h.to.kind)
                  << "]\n";
      }
    }
  }
}

void print_runtime_board(const tsiemene::Board& board) {
  section("RUNTIME BOARD TOPOLOGY");
  for (std::size_t ci = 0; ci < board.contracts.size(); ++ci) {
    const auto& c = board.contracts[ci];
    std::cout << "[runtime contract " << ci << "] name=" << c.name
              << " invoke=" << c.invoke_name
              << "(\"" << c.invoke_payload << "\")\n";
    std::cout << "  contract.spec:"
              << " instrument=" << c.spec.instrument
              << " sample=" << c.spec.sample_type
              << " source=" << c.spec.source_type
              << " repr=" << c.spec.representation_type
              << " hashimyei=" << c.spec.representation_hashimyei
              << " shape=[B~" << c.spec.batch_size_hint
              << ",C=" << c.spec.channels
              << ",T=" << c.spec.timesteps
              << ",D=" << c.spec.features
              << ",Tf=" << c.spec.future_timesteps
              << "]\n";
    std::cout << "  contract.spec.vicreg:"
              << " train=" << (c.spec.vicreg_train ? "true" : "false")
              << " use_swa=" << (c.spec.vicreg_use_swa ? "true" : "false")
              << " detach_to_cpu=" << (c.spec.vicreg_detach_to_cpu ? "true" : "false")
              << "\n";
    if (!c.spec.component_types.empty()) {
      std::cout << "  contract.spec.components:";
      for (const auto& t : c.spec.component_types) std::cout << " " << t;
      std::cout << "\n";
    }
    if (!c.dsl_segments.empty()) {
      std::cout << "  contract.dsl.keys:";
      for (const auto& [key, _] : c.dsl_segments) std::cout << " " << key;
      std::cout << "\n";
      std::cout << "  contract.dsl.render:\n";
      std::cout << c.render_dsl_segments();
    }

    std::cout << "  nodes:\n";
    for (const auto& node_ptr : c.nodes) {
      const tsiemene::Tsi& node = *node_ptr;
      std::cout << "    - id=" << node.id()
                << " instance=" << node.instance_name()
                << " type=" << node.type_name()
                << " domain=" << tsiemene::domain_token(node.domain())
                << " root=" << (node.can_be_circuit_root() ? "yes" : "no")
                << " terminal=" << (node.can_be_circuit_terminal() ? "yes" : "no")
                << "\n";
      for (const auto& d : node.directives()) {
        std::cout << "      " << dir_token(d.dir)
                  << " " << d.id << tsiemene::kind_token(d.kind.kind);
        if (!d.doc.empty()) std::cout << " | " << d.doc;
        std::cout << "\n";
      }
    }

    std::cout << "  hops:\n";
    for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
      const auto& h = c.hops[hi];
      const auto* out_spec = tsiemene::find_directive(
          *h.from.tsi, h.from.directive, tsiemene::DirectiveDir::Out);
      const auto* in_spec = tsiemene::find_directive(
          *h.to.tsi, h.to.directive, tsiemene::DirectiveDir::In);

      std::cout << "    - [" << hi << "] out[" << h.from.tsi->instance_name() << h.from.directive;
      if (out_spec) std::cout << tsiemene::kind_token(out_spec->kind.kind);
      std::cout << "] -> in[" << h.to.tsi->instance_name() << h.to.directive;
      if (in_spec) std::cout << tsiemene::kind_token(in_spec->kind.kind);
      std::cout << "]\n";
    }

    std::cout << "  start wave: id=" << c.wave0.id << " i=" << c.wave0.i << "\n";
    std::cout << "  ingress0: directive=" << c.ingress0.directive
              << " kind=" << tsiemene::kind_token(c.ingress0.signal.kind) << "\n";
  }
}

}  // namespace

int main() try {
  section("BOOT");

  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);

  const bool cuda_ok = torch::cuda::is_available();
  torch::Device device = cuda_ok ? torch::Device(torch::kCUDA) : torch::Device(torch::kCPU);
  std::printf("[env] torch::cuda::is_available() = %s\n", cuda_ok ? "true" : "false");
  std::printf("[env] runtime device = %s\n", device.is_cuda() ? "CUDA" : "CPU");
  std::cout << "[note] existing subsystems can emit extra warnings/progress logs.\n"
            << "       follow the section banners for the guided board walkthrough.\n";

  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  std::printf("[env] config folder = %s\n", config_folder);

  print_syntax_quick_reference();
  print_tsi_registry();

  section("LOAD BOARD INSTRUCTION");
  const std::string instruction =
      cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_dsl();
  std::cout << instruction << "\n";

  section("PARSE + SEMANTIC VALIDATION");
  auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits();
  auto decoded = parser.decode(instruction);
  print_instruction_summary(decoded);

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_circuit_instruction(decoded, &semantic_error)) {
    std::cerr << "[error] invalid board instruction: " << semantic_error << "\n";
    return 1;
  }
  std::cout << "[ok] board instruction semantics validated\n";

  section("STRICT INPUT LANE VALIDATION");
  const std::string invalid_instruction =
      "strict_fail = {\n"
      "  w_source = tsi.source.dataloader\n"
      "  w_rep = tsi.wikimyei.representation.vicreg\n"
      "  w_null = tsi.sink.null\n"
      "  w_log = tsi.sink.log.sys\n"
      "  w_source@payload:tensor -> w_rep@payload\n"
      "  w_rep@payload:tensor -> w_null@step\n"
      "  w_rep@loss:tensor -> w_log@info\n"
      "  w_source@meta:str -> w_log@warn\n"
      "  w_rep@meta:str -> w_log@debug\n"
      "  w_null@meta:str -> w_log@debug\n"
      "}\n"
      "strict_fail(BTCUSDT[01.01.2009,31.12.2009]);\n";
  auto invalid_decoded = parser.decode(invalid_instruction);
  std::string invalid_error;
  const bool invalid_ok =
      cuwacunu::camahjucunu::validate_circuit_instruction(invalid_decoded, &invalid_error);
  if (invalid_ok) {
    std::cerr << "[error] strict lane check expected failure, but instruction validated\n";
    return 1;
  }
  std::cout << "[ok] rejected invalid hop target lane: " << invalid_error << "\n";

  section("BUILD RUNTIME BOARD");
  tsiemene::Board runtime_board{};
  std::string build_error;
  if (!tsiemene::board_builder::build_runtime_board_from_instruction<Datatype, Sampler>(
          decoded, device, &runtime_board, &build_error)) {
    std::cerr << "[error] failed to build runtime board: " << build_error << "\n";
    return 1;
  }
  std::cout << "[ok] runtime board built with contracts=" << runtime_board.contracts.size() << "\n";

  section("RUNTIME VALIDATION");
  tsiemene::BoardIssue bi{};
  if (!tsiemene::validate_board(runtime_board, &bi)) {
    std::cerr << "[error] invalid runtime board at contract[" << bi.contract_index
              << "]: " << bi.circuit_issue.what
              << " hop=" << bi.circuit_issue.hop_index << "\n";
    return 1;
  }
  std::cout << "[ok] runtime board validation passed\n";
  print_runtime_board(runtime_board);

  section("EXECUTION");
  tsiemene::TsiContext ctx{};
  std::uint64_t total_steps = 0;
  for (std::size_t ci = 0; ci < runtime_board.contracts.size(); ++ci) {
    auto& c = runtime_board.contracts[ci];
    c.wave0.id = static_cast<tsiemene::WaveId>(700 + ci);
    c.wave0.i = 0;

    std::cout << "[run] contract[" << ci << "] " << c.name
              << " invoke=" << c.invoke_name
              << "(\"" << c.invoke_payload << "\")\n";
    const std::uint64_t steps = tsiemene::run_contract(c, ctx);
    std::cout << "[run] contract[" << ci << "] steps=" << steps << "\n";
    total_steps += steps;
  }

  section("CAPABILITIES RECAP");
  std::cout
      << "This test demonstrated:\n"
      << "  - DSL parse/decode of circuits, instances and strict hops (-> <to_alias>@<in_directive>)\n"
      << "  - semantic validation before runtime build\n"
      << "  - typed tsi_type registry matching\n"
      << "  - runtime board topology (nodes, directives, edges)\n"
      << "  - execution of each contract from ingress0 + wave0\n"
      << "  - directional hop semantics: out[...] -> in[...] (e.g. logger consumes in[@info:tensor])\n"
      << "  - total processed steps = " << total_steps << "\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "[fatal] exception: " << e.what() << "\n";
  return 1;
}
