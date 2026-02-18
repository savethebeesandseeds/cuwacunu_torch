// ./include/tsiemene/tsi.dataloader.instrument.h
// SPDX-License-Identifier: MIT
#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/utils/tsi.h"

// ---- Real dataloader hook -------------------------------------------------
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"

namespace tsiemene {

// NOTE:
//  - Datatype_t is your record struct type (e.g. exchange::kline_t).
//  - Sampler_t controls determinism/order (SequentialSampler vs RandomSampler).
template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::RandomSampler>
class TsiDataloaderInstrument final : public Tsi {
 public:
  static constexpr PortId IN_CMD    = 1;
  static constexpr PortId OUT_BATCH = 2;

  static constexpr std::string_view TAG_CMD   = "dataloader.cmd";
  static constexpr std::string_view TAG_BATCH = "vicreg4d.packed_batch";

  using Dataset_t    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Sample_t     = cuwacunu::camahjucunu::data::observation_sample_t;
  using Loader_t     = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Sample_t, Datatype_t, Sampler_t>;
  using Iterator_t   = decltype(std::declval<Loader_t&>().begin());

  TsiDataloaderInstrument(TsiId id,
                          std::string instrument,
                          torch::Device device = torch::kCPU)
      : id_(id),
        instrument_(std::move(instrument)),
        type_name_(std::string("tsi.dataloader.") + instrument_),
        instance_name_(type_name_),
        device_(device),
        dl_(make_loader_(instrument_)) {
    // Introspect shape from the real dataset:
    C_ = dl_.C_;
    T_ = dl_.T_;
    D_ = dl_.D_;

    // Batch size from DataLoaderOptions (used only for metadata / expectations).
    // NOTE: last batch may be smaller.
    B_hint_ = static_cast<int64_t>(dl_.inner().options().batch_size);

    // Start first epoch iterator.
    it_  = dl_.begin();
    end_ = dl_.end();
    iter_ready_ = true;
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return type_name_; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  // Optional: expose discovered dims so callers can instantiate VICReg consistently.
  [[nodiscard]] int64_t C() const noexcept { return C_; }
  [[nodiscard]] int64_t T() const noexcept { return T_; }
  [[nodiscard]] int64_t D() const noexcept { return D_; }
  [[nodiscard]] int64_t batch_size_hint() const noexcept { return B_hint_; }

  [[nodiscard]] std::span<const Port> ports() const noexcept override {
    static constexpr Port kPorts[] = {
      port(IN_CMD,    PortDir::In,  Schema::String(), TAG_CMD,
           "command for this wave (e.g. \"batches=10\")"),
      port(OUT_BATCH, PortDir::Out, Schema::Tensor(), TAG_BATCH,
           "packed [B,C,T,D+1] (last slot is mask 0/1; B may be <= batch_size on last batch)"),
    };
    return std::span<const Port>(kPorts, 2);
  }

  [[nodiscard]] Determinism determinism() const noexcept override {
    if constexpr (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>) {
      return Determinism::Deterministic;
    } else {
      return Determinism::SeededStochastic;
    }
  }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.port != IN_CMD) return;
    if (in.signal.kind != PayloadKind::String) return;

    const std::uint64_t n = parse_batches_(in.signal.text);
    if (n == 0) return;

    for (std::uint64_t k = 0; k < n; ++k) {
      torch::Tensor packed = next_packed_batch_();
      if (!packed.defined()) {
        // No data available (empty dataset or hard exhaustion).
        // For now, stop emitting further batches.
        break;
      }

      Wave witem{.id = wave.id, .i = wave.i + k};
      out.emit_tensor(witem, OUT_BATCH, std::move(packed));
    }
  }

 private:
  static std::uint64_t parse_batches_(std::string_view s) noexcept {
    // Accept: "batches=10" or "10" (parse first digit run).
    std::size_t pos = 0;
    while (pos < s.size() && (s[pos] < '0' || s[pos] > '9')) ++pos;
    if (pos == s.size()) return 0;

    std::uint64_t v = 0;
    auto sv = s.substr(pos);
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), v);
    if (res.ec != std::errc()) return 0;
    return v;
  }

  static Loader_t make_loader_(std::string_view instrument) {
    return cuwacunu::camahjucunu::data::make_obs_pipeline_mm_dataloader<Datatype_t, Sampler_t>(instrument);
  }

  torch::Tensor pack_batch_(std::vector<Sample_t>&& sample_batch) const {
    if (sample_batch.empty()) return torch::Tensor();

    // Collate to [B,C,T,D] and [B,C,T]
    Sample_t coll = Sample_t::collate_fn(sample_batch);

    torch::Tensor data = coll.features;                // float32 [B,C,T,D]
    torch::Tensor mask = coll.mask.to(torch::kFloat32); // 0/1    [B,C,T]

    if (!data.defined() || !mask.defined()) return torch::Tensor();

    if (device_ != torch::kCPU) {
      data = data.to(device_);
      mask = mask.to(device_);
    }

    // packed: [B,C,T,D+1] where last slot is mask (0/1 float)
    return torch::cat({data, mask.unsqueeze(-1)}, /*dim=*/3);
  }

  torch::Tensor next_packed_batch_() {
    if (!iter_ready_) {
      it_  = dl_.begin();
      end_ = dl_.end();
      iter_ready_ = true;
    }

    // If epoch ended, start a new one.
    // IMPORTANT: only do this after iterator is exhausted.
    if (it_ == end_) {
      it_  = dl_.begin();
      end_ = dl_.end();
      if (it_ == end_) return torch::Tensor(); // empty loader
    }

    // Move the produced batch out of the iterator to avoid copying.
    auto& batch_ref = *it_;
    std::vector<Sample_t> batch = std::move(batch_ref);
    ++it_;

    return pack_batch_(std::move(batch));
  }

 private:
    TsiId id_{};
    std::string instrument_;
    std::string type_name_;
    std::string instance_name_;

    torch::Device device_{torch::kCPU};

    // Real loader + iterator state
    Loader_t dl_;
    Iterator_t it_{dl_.end()};
    Iterator_t end_{dl_.end()};
    bool iter_ready_{false};

    // Discovered dims
    int64_t B_hint_{0};
    int64_t C_{0}, T_{0}, D_{0};
};

} // namespace tsiemene
