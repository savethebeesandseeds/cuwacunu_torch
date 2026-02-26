// test_tsi_basic.cpp

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <string>

#include <torch/torch.h>
#include <torch/cuda.h>

// Config / utils (adjust if your project uses different include paths)
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

// Datatype for the real dataloader (kline_t used in your existing test)
#include "camahjucunu/types/types_data.h"

// TSI runtime + nodes
#include "tsiemene/board.h"
#include "tsiemene/tsi.source.dataloader.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"
#include "tsiemene/tsi.sink.null.h"
#include "tsiemene/tsi.sink.log.sys.h"

/*
  circuit_1 = {
    w_source  = tsi.source.dataloader
    w_rep     = tsi.wikimyei.representation.vicreg
    w_null    = tsi.sink.null
    w_log     = tsi.sink.log.sys

    w_source@payload:tensor   -> w_rep@step
    w_rep@payload:tensor      -> w_null@step

    w_source@meta:str         -> w_log@warn
    w_rep@meta:str            -> w_log@debug
    w_null@meta:str           -> w_log@debug
    w_rep@loss:tensor         -> w_log@info
  }

  circuit_1( BTCUSDT[01.01.2009,31.12.2009] );
*/

int main() try {
  // ---- Torch runtime knobs ------------------------------------------------
  torch::autograd::AnomalyMode::set_enabled(false);

  // If you run on CUDA, these affect cuDNN algorithms. Adjust as desired.
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);

  // ---- Device selection ---------------------------------------------------
  const bool cuda_ok = torch::cuda::is_available();
  torch::Device device = cuda_ok ? torch::Device(torch::kCUDA) : torch::Device(torch::kCPU);

  std::printf("[main] torch::cuda::is_available() = %s\n", cuda_ok ? "true" : "false");
  std::printf("[main] using device = %s\n", device.is_cuda() ? "CUDA" : "CPU");

  // Tiny warm-up so first CUDA kernel isn't part of your timing noise.
  if (device.is_cuda()) {
    std::printf("[main] warming up CUDA...\n");
    auto x = torch::rand({1024, 1024}, torch::TensorOptions().device(device));
    x = x.matmul(x);
    torch::cuda::synchronize();
  }

  // ---- Load config --------------------------------------------------------
  const char* config_folder = "/cuwacunu/src/config/";

  std::printf("[main] loading config from: %s\n", config_folder);
  auto t0 = std::chrono::steady_clock::now();
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  auto t1 = std::chrono::steady_clock::now();

  const double ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
  std::printf("[main] config loaded in %.3f ms\n", ms);

  const int seed = cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed");
  torch::manual_seed(seed);
  std::printf("[main] torch_seed = %d\n", seed);

  tsiemene::TsiContext ctx{};

  // ---- Real dataloader-backed TSI ----------------------------------------
  const std::string INSTRUMENT = "BTCUSDT";
  std::printf("[main] instrument = %s\n", INSTRUMENT.c_str());

  // Choose record datatype used by the memory-mapped dataset/dataloader:
  using Datatype = cuwacunu::camahjucunu::exchange::kline_t;

  // Choose sampler:
  //   - SequentialSampler => deterministic iteration order
  //   - RandomSampler     => stochastic (seeded)
  using Sampler = torch::data::samplers::SequentialSampler;
  // using Sampler = torch::data::samplers::RandomSampler;

  tsiemene::Board board{};
  auto& c = board.contracts.emplace_back();
  c.name = "circuit_1 approximation";
  c.invoke_name = "circuit_1";
  const std::string instruction = "BTCUSDT[01.01.2009,31.12.2009]";
  c.invoke_payload = instruction;

  using DataloaderT = tsiemene::TsiSourceDataloader<Datatype, Sampler>;

  // constructor discovers C/T/D from the dataset.
  auto& dl = c.emplace_node<DataloaderT>(/*id=*/1, INSTRUMENT, device);

  std::printf("[dl] discovered dims: C=%lld T=%lld D=%lld\n",
              (long long)dl.C(), (long long)dl.T(), (long long)dl.D());

  // VICReg expects C,T,D (B comes from the batch)
  auto& vicreg = c.emplace_node<tsiemene::TsiWikimyeiRepresentationVicreg>(
      /*id=*/2,
      /*instance_name=*/"tsi.wikimyei.representation.vicreg",
      /*C=*/(int)dl.C(),
      /*T=*/(int)dl.T(),
      /*D=*/(int)dl.D(),
      /*train=*/false,
      /*use_swa=*/true,
      /*detach_to_cpu=*/true);

  auto& sink_null = c.emplace_node<tsiemene::TsiSinkNull>(/*id=*/5, "tsi.sink.null");
  auto& sink_log = c.emplace_node<tsiemene::TsiSinkLogSys>(/*id=*/6, "tsi.sink.log.sys");

  vicreg.set_train(true); // emit @loss for sink.log.sys

  // Single circuit approximation with branching at w_rep.
  // Source is the circuit root and consumes the board invoke payload.
  // - payload path: w_rep@payload -> w_null@step
  // - loss path:    w_rep@loss    -> w_log@info
  c.hops = {
    tsiemene::hop(
      tsiemene::ep(dl,     DataloaderT::OUT_PAYLOAD),
      tsiemene::ep(vicreg, tsiemene::TsiWikimyeiRepresentationVicreg::IN_STEP),
      tsiemene::query("")),

    tsiemene::hop(
      tsiemene::ep(vicreg, tsiemene::TsiWikimyeiRepresentationVicreg::OUT_PAYLOAD),
      tsiemene::ep(sink_null, tsiemene::TsiSinkNull::IN_STEP),
      tsiemene::query("")),

    tsiemene::hop(
      tsiemene::ep(vicreg, tsiemene::TsiWikimyeiRepresentationVicreg::OUT_LOSS),
      tsiemene::ep(sink_log, tsiemene::TsiSinkLogSys::IN_INFO),
      tsiemene::query("")),

    tsiemene::hop(
      tsiemene::ep(dl, DataloaderT::OUT_META),
      tsiemene::ep(sink_log, tsiemene::TsiSinkLogSys::IN_WARN),
      tsiemene::query("")),

    tsiemene::hop(
      tsiemene::ep(vicreg, tsiemene::TsiWikimyeiRepresentationVicreg::OUT_META),
      tsiemene::ep(sink_log, tsiemene::TsiSinkLogSys::IN_DEBUG),
      tsiemene::query("")),

    tsiemene::hop(
      tsiemene::ep(sink_null, tsiemene::TsiSinkNull::OUT_META),
      tsiemene::ep(sink_log, tsiemene::TsiSinkLogSys::IN_DEBUG),
      tsiemene::query("")),
  };

  c.wave0 = tsiemene::Wave{.id = 200, .i = 0};
  c.ingress0 = tsiemene::Ingress{
    .directive = tsiemene::pick_start_directive(c.view()),
    .signal = tsiemene::string_signal(instruction)
  };

  tsiemene::BoardIssue issue{};
  if (!tsiemene::validate_board(board, &issue)) {
    std::cerr << "[readme/circuit_1] invalid board at contract[" << issue.contract_index
              << "]: " << issue.circuit_issue.what
              << " at hop " << issue.circuit_issue.hop_index << "\n";
    return 1;
  }

  std::printf("[readme/circuit_1] running instruction=\"%s\"...\n", instruction.c_str());

  const std::uint64_t steps = tsiemene::run_contract(c, ctx);
  std::printf("[readme/circuit_1] events processed = %llu\n", (unsigned long long)steps);
  if (steps == 0) {
    std::cerr << "[readme/circuit_1] expected events > 0\n";
    return 1;
  }

  std::printf("[main] done.\n");
  return 0;

} catch (const std::exception& e) {
  std::cerr << "[main] exception: " << e.what() << "\n";
  return 1;
}
