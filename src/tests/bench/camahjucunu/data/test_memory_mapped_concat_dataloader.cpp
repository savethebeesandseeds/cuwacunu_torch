/* test_memory_mapped_concat_dataloader.cpp */
#include <torch/torch.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"

using Datatype_t  = cuwacunu::camahjucunu::exchange::kline_t;
using Dataset_t  = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
using Datasample_t  = cuwacunu::camahjucunu::data::observation_sample_t;

// ----- helpers to build a tiny, regular grid for kline_t --------------------
static Datatype_t make_kline(int64_t close_time, double base = 100.0, int i = 0) {
  Datatype_t r{};
  r.open_time              = close_time - 1;
  r.open_price             = base + i;
  r.high_price             = r.open_price + 1.0;
  r.low_price              = r.open_price - 1.0;
  r.close_price            = r.open_price + 0.5;
  r.volume                 = 1000.0 + 10.0 * i;
  r.close_time             = close_time;
  r.quote_asset_volume     = r.volume * ((r.open_price + r.close_price) * 0.5);
  r.number_of_trades       = 100 + i;
  r.taker_buy_base_volume  = r.volume * 0.6;
  r.taker_buy_quote_volume = r.quote_asset_volume * 0.6;
  return r;
}

static std::vector<Datatype_t> make_regular_rows(int64_t start_key, int64_t step, int n) {
  std::vector<Datatype_t> v; v.reserve(n);
  for (int i = 0; i < n; ++i) v.push_back(make_kline(start_key + i * step, 100.0, i));
  return v;
}

static std::string write_csv_kline(const std::vector<Datatype_t>& rows, const std::string& path, char delimiter = ',') {
  std::ofstream ofs(path);
  if (!ofs) throw std::runtime_error("Failed to open tmp csv for write: " + path);
  for (const auto& r : rows) {
    std::ostringstream line;
    r.to_csv(line, delimiter);
    std::string s = line.str();
    if (!s.empty() && s.back() == '\n') ofs << s;
    else                                ofs << s << '\n';
  }
  ofs.close();
  return path;
}

int main() try {
  // --- 1) Build two sources on a regular grid and write CSVs ----------------
  const int64_t start_key = 1000;
  const int64_t step      = 1;
  const int     nrows     = 64;

  auto rows = make_regular_rows(start_key, step, nrows);
  const std::string fA = "/tmp/kline_regular_A.csv";
  const std::string fB = "/tmp/kline_regular_B.csv";
  write_csv_kline(rows, fA);
  write_csv_kline(rows, fB);

  // --- 2) Build the concat dataset with different (Np, Nf) to exercise padding
  Dataset_t cds;
  cds.add_dataset(/*csv*/ fA, /*Np*/ 5, /*Nf*/ 3, /*norm_window*/ 0, /*force_bin*/ true);
  cds.add_dataset(/*csv*/ fB, /*Np*/ 3, /*Nf*/ 5, /*norm_window*/ 0, /*force_bin*/ true);

  // Expected global properties
  assert(cds.size().has_value());
  const std::size_t N = cds.size().value();         // number of anchors in intersection
  const int64_t     Kchannels = 2;
  const int64_t     Tp = (int64_t)cds.max_N_past_;
  const int64_t     Tf = (int64_t)cds.max_N_future_;
  const int64_t     key_left  = (int64_t)cds.leftmost_key_value_;
  const int64_t     key_step  = (int64_t)cds.key_value_step_;

  // --- 3) Build dataloaders (sequential and random) from the dataset --------
  const std::size_t batch_size = 8;
  const std::size_t workers    = 0; // single-thread for deterministic tests

  // Sequential
  {
    using S = torch::data::samplers::SequentialSampler;
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, S> dl(
      cds, cds.SequentialSampler(), cds.SequentialSampler_options(batch_size, workers));

    // Introspection from the dataloader must match dataset
    assert(dl.C_ == Kchannels);
    assert(dl.T_ == Tp);
    // D_ checked once we see a sample below.

    std::vector<char> visited(N, 0);
    std::size_t total_seen = 0;
    bool have_prev = false;
    int64_t prev_anchor = 0;

    for (auto& sample_batch : dl) {
      total_seen += sample_batch.size();

      // Check per-sample anchors are strictly increasing across the whole epoch.
      for (auto& s : sample_batch) {
        // Each sample is unbatched: past_keys shape is [Kchannels, Tp]
        assert(s.past_keys.defined());
        assert(s.past_keys.dim() == 2);
        assert(s.past_keys.size(0) == Kchannels);
        assert(s.past_keys.size(1) == Tp);

        const int64_t anchor_key = s.past_keys.index({0, Tp - 1}).item<int64_t>(); // channel 0, time t
        if (have_prev) { assert(anchor_key == prev_anchor + key_step); }
        prev_anchor = anchor_key; have_prev = true;

        // Map anchor key to sliding index and mark visited
        const std::size_t j = (std::size_t)((anchor_key - key_left) / key_step);
        assert(j < N);
        visited[j] = 1;

        // Sanity on features/masks shapes for each sample
        assert(s.features.size(0)        == Kchannels);
        assert(s.features.size(1)        == Tp);
        assert(s.future_features.size(0) == Kchannels);
        assert(s.future_features.size(1) == Tf);
      }

      // Collate and check batch shapes
      auto coll = Datasample_t::collate_fn(sample_batch);
      const int64_t B = coll.features.size(0);
      const int64_t D = coll.features.size(3);  // infer D from collated tensor
      assert(B > 0 && B <= (int64_t)batch_size);
      assert(coll.features.sizes()        == torch::IntArrayRef({B, Kchannels, Tp, D}));
      assert(coll.mask.sizes()            == torch::IntArrayRef({B, Kchannels, Tp}));
      assert(coll.future_features.sizes() == torch::IntArrayRef({B, Kchannels, Tf, D}));
      assert(coll.future_mask.sizes()     == torch::IntArrayRef({B, Kchannels, Tf}));

      // Cross-check dataloader’s D_ discovery
      assert(dl.D_ == D);
    }

    // Coverage: we must have visited all anchors exactly once
    assert(total_seen == N);
    for (auto v : visited) assert(v);
  }

  // Random
  {
    using S = torch::data::samplers::RandomSampler;
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, S> dl(
      cds, cds.RandomSampler(), cds.RandomSampler_options(batch_size, workers));

    std::vector<char> visited(N, 0);
    std::size_t total_seen = 0;

    for (auto& sample_batch : dl) {
      total_seen += sample_batch.size();

      // Anchors come in random order; just validate coverage & shapes
      for (auto& s : sample_batch) {
        const int64_t anchor_key = s.past_keys.index({0, Tp - 1}).item<int64_t>();
        const std::size_t j = (std::size_t)((anchor_key - key_left) / key_step);
        assert(j < N);
        visited[j] = 1;

        assert(s.features.size(0)        == Kchannels);
        assert(s.features.size(1)        == Tp);
        assert(s.future_features.size(0) == Kchannels);
        assert(s.future_features.size(1) == Tf);
      }

      auto coll = Datasample_t::collate_fn(sample_batch);
      const int64_t B = coll.features.size(0);
      const int64_t D = coll.features.size(3);
      assert(B > 0 && B <= (int64_t)batch_size);
      assert(coll.features.sizes()        == torch::IntArrayRef({B, Kchannels, Tp, D}));
      assert(coll.mask.sizes()            == torch::IntArrayRef({B, Kchannels, Tp}));
      assert(coll.future_features.sizes() == torch::IntArrayRef({B, Kchannels, Tf, D}));
      assert(coll.future_mask.sizes()     == torch::IntArrayRef({B, Kchannels, Tf}));
    }

    assert(total_seen == N);
    for (auto v : visited) assert(v);
  }

  std::cout << "[OK] memory_mapped_concat_dataloader tests passed.\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "Test failed with exception: " << e.what() << "\n";
  return 1;
}
