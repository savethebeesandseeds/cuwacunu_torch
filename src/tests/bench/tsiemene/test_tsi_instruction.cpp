// test_tsi_instruction.cpp

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>

#include <torch/cuda.h>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "tsiemene/board.builder.h"

namespace {

using Datatype = cuwacunu::camahjucunu::exchange::kline_t;
using Sampler = torch::data::samplers::SequentialSampler;

}  // namespace

int main() try {
  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);

  const bool cuda_ok = torch::cuda::is_available();
  torch::Device device = cuda_ok ? torch::Device(torch::kCUDA) : torch::Device(torch::kCPU);
  std::printf("[main] torch::cuda::is_available() = %s\n", cuda_ok ? "true" : "false");
  std::printf("[main] using device = %s\n", device.is_cuda() ? "CUDA" : "CPU");

  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  const std::string instruction =
      cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_dsl();
  std::cout << "[main] board instruction loaded\n" << instruction << "\n";

  auto board = cuwacunu::camahjucunu::dsl::tsiemeneCircuits();
  auto decoded = board.decode(instruction);
  std::cout << decoded.str() << "\n";

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_circuit_instruction(decoded, &semantic_error)) {
    std::cerr << "[main] invalid tsiemene circuit instruction: " << semantic_error << "\n";
    return 1;
  }

  tsiemene::Board runtime_board{};
  std::string build_error;
  if (!tsiemene::board_builder::build_runtime_board_from_instruction<Datatype, Sampler>(
          decoded, device, &runtime_board, &build_error)) {
    std::cerr << "[main] failed to build runtime board: " << build_error << "\n";
    return 1;
  }
  if (runtime_board.contracts.empty()) {
    std::cerr << "[main] no contracts produced from tsiemene_circuit.dsl\n";
    return 1;
  }
  std::unordered_set<std::string> unique_circuit_dsl;
  unique_circuit_dsl.reserve(runtime_board.contracts.size());

  const auto* shared_obs_inst =
      runtime_board.contracts.front().find_dsl_segment(tsiemene::kBoardContractObservationSourcesDslKey);
  const auto* shared_obs_inputs =
      runtime_board.contracts.front().find_dsl_segment(tsiemene::kBoardContractObservationChannelsDslKey);
  const auto* shared_train =
      runtime_board.contracts.front().find_dsl_segment(tsiemene::kBoardContractJkimyeiSpecsDslKey);
  if (!shared_obs_inst || shared_obs_inst->empty()) {
    std::cerr << "[main] missing shared observation sources DSL segment in contract[0]\n";
    return 1;
  }
  if (!shared_obs_inputs || shared_obs_inputs->empty()) {
    std::cerr << "[main] missing shared observation channels DSL segment in contract[0]\n";
    return 1;
  }
  if (!shared_train || shared_train->empty()) {
    std::cerr << "[main] missing shared jkimyei specs DSL segment in contract[0]\n";
    return 1;
  }

  for (std::size_t ci = 0; ci < runtime_board.contracts.size(); ++ci) {
    const auto& contract = runtime_board.contracts[ci];
    for (const std::string_view key : tsiemene::BoardContract::required_dsl_keys()) {
      const auto* value = contract.find_dsl_segment(key);
      if (!value || value->empty()) {
        std::cerr << "[main] contract[" << ci << "] missing required DSL segment key=" << key << "\n";
        return 1;
      }
    }

    const auto* circuit_dsl = contract.find_dsl_segment(tsiemene::kBoardContractCircuitDslKey);
    if (!circuit_dsl || circuit_dsl->empty()) {
      std::cerr << "[main] contract[" << ci << "] missing circuit DSL segment\n";
      return 1;
    }
    if (!unique_circuit_dsl.insert(*circuit_dsl).second) {
      std::cerr << "[main] contract[" << ci << "] duplicated board.contract.circuit@DSL:str\n";
      return 1;
    }

    const auto* obs_inst =
        contract.find_dsl_segment(tsiemene::kBoardContractObservationSourcesDslKey);
    const auto* obs_inputs =
        contract.find_dsl_segment(tsiemene::kBoardContractObservationChannelsDslKey);
    const auto* train =
        contract.find_dsl_segment(tsiemene::kBoardContractJkimyeiSpecsDslKey);
    if (!obs_inst || *obs_inst != *shared_obs_inst) {
      std::cerr << "[main] contract[" << ci << "] observation sources DSL drift\n";
      return 1;
    }
    if (!obs_inputs || *obs_inputs != *shared_obs_inputs) {
      std::cerr << "[main] contract[" << ci << "] observation channels DSL drift\n";
      return 1;
    }
    if (!train || *train != *shared_train) {
      std::cerr << "[main] contract[" << ci << "] jkimyei specs DSL drift\n";
      return 1;
    }
  }
  {
    const auto& spec0 = runtime_board.contracts.front().spec;
    if (!spec0.sourced_from_config) {
      std::cerr << "[main] contract[0].spec.sourced_from_config is false\n";
      return 1;
    }
    if (spec0.instrument.empty()) {
      std::cerr << "[main] contract[0].spec.instrument is empty\n";
      return 1;
    }
    if (spec0.sample_type.empty()) {
      std::cerr << "[main] contract[0].spec.sample_type is empty\n";
      return 1;
    }
    if (spec0.source_type.empty()) {
      std::cerr << "[main] contract[0].spec.source_type is empty\n";
      return 1;
    }
    if (spec0.representation_type.empty()) {
      std::cerr << "[main] contract[0].spec.representation_type is empty\n";
      return 1;
    }
    if (!spec0.representation_type.empty() && spec0.representation_hashimyei.empty()) {
      std::cerr << "[main] contract[0].spec.representation_hashimyei is empty\n";
      return 1;
    }
    if (spec0.component_types.empty()) {
      std::cerr << "[main] contract[0].spec.component_types is empty\n";
      return 1;
    }
    if (spec0.batch_size_hint <= 0 || spec0.channels <= 0 ||
        spec0.timesteps <= 0 || spec0.features <= 0) {
      std::cerr << "[main] contract[0].spec shape hints are invalid\n";
      return 1;
    }
    if (spec0.future_timesteps < 0) {
      std::cerr << "[main] contract[0].spec.future_timesteps is negative\n";
      return 1;
    }
    if (std::find(spec0.component_types.begin(), spec0.component_types.end(), spec0.source_type) ==
        spec0.component_types.end()) {
      std::cerr << "[main] contract[0].spec.source_type missing in component_types\n";
      return 1;
    }
    if (std::find(spec0.component_types.begin(), spec0.component_types.end(), spec0.representation_type) ==
        spec0.component_types.end()) {
      std::cerr << "[main] contract[0].spec.representation_type missing in component_types\n";
      return 1;
    }
  }

  tsiemene::BoardIssue bi{};
  if (!tsiemene::validate_board(runtime_board, &bi)) {
    std::cerr << "[main] invalid board at contract[" << bi.contract_index
              << "]: " << bi.circuit_issue.what
              << " hop=" << bi.circuit_issue.hop_index << "\n";
    return 1;
  }

  tsiemene::TsiContext ctx{};
  std::uint64_t total_events = 0;

  for (std::size_t ci = 0; ci < runtime_board.contracts.size(); ++ci) {
    auto& c = runtime_board.contracts[ci];
    c.wave0.id = 500 + ci;
    c.wave0.i = 0;

    std::cout << "[contract " << ci << "] name=" << c.name
              << " invoke=" << c.invoke_name
              << "(\"" << c.invoke_payload << "\")\n";

    const std::uint64_t steps = tsiemene::run_contract(c, ctx);
    std::cout << "[contract " << ci << "] events processed = " << steps << "\n";
    total_events += steps;
  }

  std::cout << "[main] total events processed = " << total_events << "\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "[main] exception: " << e.what() << "\n";
  return 1;
}
