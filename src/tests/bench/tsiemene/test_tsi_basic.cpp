// test_tsi_vicreg_chain.cpp

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
#include "tsiemene/utils/runtime.h"
#include "tsiemene/utils/circuits.h"
#include "tsiemene/tsi.dataloader.instrument.h"
#include "tsiemene/tsi.representation.vicreg.h"
#include "tsiemene/tsi.sink.tensor.h"

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

  // constructor discovers C/T/D from the dataset.
  tsiemene::TsiDataloaderInstrument<Datatype, Sampler> dl(/*id=*/1, INSTRUMENT, device);

  std::printf("[dl] discovered dims: C=%lld T=%lld D=%lld\n",
              (long long)dl.C(), (long long)dl.T(), (long long)dl.D());

  // VICReg expects C,T,D (B comes from the batch)
  tsiemene::TsiVicreg4D vicreg(
      /*id=*/2,
      /*instance_name=*/"tsi.representation.vicreg4d",
      /*C=*/(int)dl.C(),
      /*T=*/(int)dl.T(),
      /*D=*/(int)dl.D(),
      /*train=*/false,
      /*use_swa=*/true,
      /*detach_to_cpu=*/true);

  // Sinks: one for inspecting packed batches, another for representations
  tsiemene::TsiSinkTensor sink_packed(/*id=*/3, "sink.packed_batch", /*capacity=*/8);
  tsiemene::TsiSinkTensor sink_repr  (/*id=*/4, "sink.repr",        /*capacity=*/64);

  // ---- Circuit A: dataloader -> sink (inspect packed batch shape) ---------
  {
    const tsiemene::Hop hops_dbg[] = {
      tsiemene::hop(
        tsiemene::ep(dl,        decltype(dl)::OUT_BATCH),
        tsiemene::ep(sink_packed, tsiemene::TsiSinkTensor::IN),
        tsiemene::query(""))
    };
    const tsiemene::Circuit c_dbg = tsiemene::circuit(hops_dbg, "debug: dl -> sink(packed)");

    tsiemene::CircuitIssue issue{};
    if (!tsiemene::validate(c_dbg, &issue)) {
      std::cerr << "[debug circuit] invalid: " << issue.what
                << " at hop " << issue.hop_index << "\n";
      return 1;
    }

    std::printf("[debug circuit] running batches=1 (expect packed [B,C,T,D+1])...\n");

    tsiemene::Wave w{.id = 100, .i = 0};
    tsiemene::Ingress start{
      .port   = decltype(dl)::IN_CMD,
      .signal = tsiemene::string_signal("batches=1")
    };

    const std::uint64_t steps = tsiemene::run_wave(c_dbg, w, std::move(start), ctx);
    std::printf("[debug circuit] events processed = %llu\n", (unsigned long long)steps);
    std::printf("[debug circuit] sink_packed stored = %zu\n", sink_packed.size());

    if (sink_packed.size() > 0) {
      const auto& item = sink_packed.at(0);
      const auto& t = item.tensor;

      std::cout << "[debug circuit] first packed tensor sizes = " << t.sizes() << "\n";
      std::cout << "[debug circuit] first packed wave         = id=" << item.wave.id
                << " i=" << item.wave.i << "\n";

      if (t.defined() && t.dim() == 4) {
        const auto B = t.size(0);
        const auto C = t.size(1);
        const auto T = t.size(2);
        const auto DD1 = t.size(3);
        std::printf("[debug circuit] parsed: B=%lld C=%lld T=%lld (D+1)=%lld\n",
                    (long long)B, (long long)C, (long long)T, (long long)DD1);
      }
    }
  }

  // ---- Circuit B: dataloader -> vicreg -> sink(repr) ----------------------
  {
    const tsiemene::Hop hops[] = {
      tsiemene::hop(
        tsiemene::ep(dl,     decltype(dl)::OUT_BATCH),
        tsiemene::ep(vicreg, tsiemene::TsiVicreg4D::IN_BATCH),
        tsiemene::query("")),

      tsiemene::hop(
        tsiemene::ep(vicreg, tsiemene::TsiVicreg4D::OUT_REPR),
        tsiemene::ep(sink_repr, tsiemene::TsiSinkTensor::IN),
        tsiemene::query("")),
    };
    const tsiemene::Circuit c = tsiemene::circuit(hops, "dl -> vicreg -> sink(repr)");

    tsiemene::CircuitIssue issue{};
    if (!tsiemene::validate(c, &issue)) {
      std::cerr << "[vicreg circuit] invalid: " << issue.what
                << " at hop " << issue.hop_index << "\n";
      return 1;
    }

    std::printf("[vicreg circuit] running batches=3...\n");

    tsiemene::Wave w{.id = 200, .i = 0};
    tsiemene::Ingress start{
      .port   = decltype(dl)::IN_CMD,
      .signal = tsiemene::string_signal("batches=3")
    };

    const std::uint64_t steps = tsiemene::run_wave(c, w, std::move(start), ctx);

    std::printf("[vicreg circuit] events processed = %llu\n", (unsigned long long)steps);
    std::printf("[vicreg circuit] sink_repr stored = %zu\n", sink_repr.size());

    if (sink_repr.size() > 0) {
      const auto& item = sink_repr.at(0);
      std::cout << "[vicreg circuit] first repr tensor sizes = " << item.tensor.sizes() << "\n";
      std::cout << "[vicreg circuit] first repr wave         = id=" << item.wave.id
                << " i=" << item.wave.i << "\n";
    }
  }

  std::printf("[main] done.\n");
  return 0;

} catch (const std::exception& e) {
  std::cerr << "[main] exception: " << e.what() << "\n";
  return 1;
}
