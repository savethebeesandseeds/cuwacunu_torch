// test_tsi_instruction.cpp

#include <cstdio>
#include <iostream>
#include <string>

#include <torch/cuda.h>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "tsiemene/utils/board.tsiemene_board.h"

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
      cuwacunu::piaabo::dconfig::config_space_t::tsiemene_board_instruction();
  std::cout << "[main] board instruction loaded\n" << instruction << "\n";

  auto board = cuwacunu::camahjucunu::BNF::tsiemeneBoard();
  auto decoded = board.decode(instruction);
  std::cout << decoded.str() << "\n";

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_board_instruction(decoded, &semantic_error)) {
    std::cerr << "[main] invalid tsiemene board instruction: " << semantic_error << "\n";
    return 1;
  }

  tsiemene::Board runtime_board{};
  std::string build_error;
  if (!tsiemene::board_builder::build_runtime_board_from_instruction<Datatype, Sampler>(
          decoded, device, &runtime_board, &build_error)) {
    std::cerr << "[main] failed to build runtime board: " << build_error << "\n";
    return 1;
  }
  if (runtime_board.circuits.empty()) {
    std::cerr << "[main] no circuits produced from tsiemene_board.instruction\n";
    return 1;
  }

  tsiemene::BoardIssue bi{};
  if (!tsiemene::validate_board(runtime_board, &bi)) {
    std::cerr << "[main] invalid board at circuit[" << bi.circuit_index
              << "]: " << bi.circuit_issue.what
              << " hop=" << bi.circuit_issue.hop_index << "\n";
    return 1;
  }

  tsiemene::TsiContext ctx{};
  std::uint64_t total_events = 0;

  for (std::size_t ci = 0; ci < runtime_board.circuits.size(); ++ci) {
    auto& c = runtime_board.circuits[ci];
    c.wave0.id = 500 + ci;
    c.wave0.i = 0;

    std::cout << "[circuit " << ci << "] name=" << c.name
              << " invoke=" << c.invoke_name
              << "(\"" << c.invoke_payload << "\")\n";

    const std::uint64_t steps = tsiemene::run_circuit(c, ctx);
    std::cout << "[circuit " << ci << "] events processed = " << steps << "\n";
    total_events += steps;
  }

  std::cout << "[main] total events processed = " << total_events << "\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "[main] exception: " << e.what() << "\n";
  return 1;
}
