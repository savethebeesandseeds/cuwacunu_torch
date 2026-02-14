#include <iostream>
#include <torch/torch.h>

#include "tsiemene/runtime.h"
#include "tsiemene/tsi.dataloader.instrument.h"
#include "tsiemene/tsi.representation.vicreg.h"
#include "tsiemene/tsi.sink.tensor.h"

int main() {
  torch::manual_seed(0);

  tsiemene::TsiContext ctx{};

  const int B = 2, C = 1, T = 8, D = 16;

  tsiemene::TsiDataloaderInstrument dl(1, "BTCUSDT", B, C, T, D, torch::kCPU);
  tsiemene::TsiVicreg4D vicreg(2, "tsi.representation.vicreg4d", C, T, D,
                               /*train=*/false, /*use_swa=*/true, /*detach_to_cpu=*/true);
  tsiemene::TsiSinkTensor sink(3, "tsi_sink.tensor", 64);

  const tsiemene::Hop hops[] = {
    tsiemene::hop(tsiemene::ep(dl,    tsiemene::TsiDataloaderInstrument::OUT_BATCH),
                  tsiemene::ep(vicreg,tsiemene::TsiVicreg4D::IN_BATCH),
                  tsiemene::query("")),

    tsiemene::hop(tsiemene::ep(vicreg,tsiemene::TsiVicreg4D::OUT_REPR),
                  tsiemene::ep(sink,  tsiemene::TsiSinkTensor::IN),
                  tsiemene::query("")),
  };
  const tsiemene::Circuit c = tsiemene::circuit(hops, "vicreg no-train chain");

  tsiemene::CircuitIssue issue{};
  if (!tsiemene::validate(c, &issue)) {
    std::cerr << "Circuit invalid: " << issue.what << " at hop " << issue.hop_index << "\n";
    return 1;
  }

  tsiemene::Wave w{.id = 1, .i = 0};

  // Start ingress is the dataloader command
  tsiemene::Ingress start{
    .port = tsiemene::TsiDataloaderInstrument::IN_CMD,
    .signal = tsiemene::string_signal("batches=3")
  };

  const std::uint64_t steps = tsiemene::run_wave(c, w, std::move(start), ctx);

  std::cout << "events processed = " << steps << "\n";
  std::cout << "sink stored = " << sink.size() << "\n";
  if (sink.size() > 0) {
    std::cout << "first tensor sizes: " << sink.at(0).tensor.sizes() << "\n";
    std::cout << "first wave: id=" << sink.at(0).wave.id << " i=" << sink.at(0).wave.i << "\n";
  }

  return 0;
}
