/* test_memory_mapped_concat_dataset.cpp */
#include <torch/torch.h>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"

using cuwacunu::camahjucunu::data::MemoryMappedConcatDataset;
using Kline = cuwacunu::camahjucunu::exchange::kline_t;

/* size recomputation consistent with concat’s implementation */
template <typename K>
static inline std::size_t expected_count(K left, K right, K step, K tol = static_cast<K>(1e-9)) {
  if constexpr (std::is_floating_point_v<K>) {
    if (step <= K(0) || right + tol < left) return 0;
    K k = std::floor(((right - left) / step) + tol);
    if (k < 0) return 0;
    return static_cast<std::size_t>(k) + 1;
  } else {
    if (step <= 0 || right < left) return 0;
    return static_cast<std::size_t>((right - left) / step) + 1;
  }
}

// ---------- helpers ----------
static Kline make_kline(int64_t close_time, double base = 100.0, int i = 0) {
  Kline r{};
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

static std::vector<Kline> make_regular_rows(int64_t start_key, int64_t step, int n) {
  std::vector<Kline> v; v.reserve(n);
  for (int i = 0; i < n; ++i) v.push_back(make_kline(start_key + i * step, 100.0, i));
  return v;
}

static std::string write_csv_kline(const std::vector<Kline>& rows, const std::string& path, char delimiter = ',') {
  std::ofstream ofs(path);
  if (!ofs) throw std::runtime_error("Failed to open tmp csv for write: " + path);
  for (const auto& r : rows) {
    std::ostringstream line;
    r.to_csv(line, delimiter);               // ensure exact format
    std::string s = line.str();
    if (!s.empty() && s.back() == '\n') ofs << s;
    else                                ofs << s << '\n';
  }
  ofs.close();
  return path;
}

// For row-wise equality checks
static torch::Tensor features_f32(const Kline& r) {
  const auto vd = r.tensor_features();
  std::vector<float> vf(vd.begin(), vd.end());
  return torch::from_blob(vf.data(), {(long)vf.size()}, torch::kFloat32).clone();
}

int main() {
  try {
    // 1) Build two identical sources on a regular 1-step grid and write CSVs
    const int64_t start_key = 1000;
    const int64_t step      = 1;
    const int     nrows     = 64;

    auto rows = make_regular_rows(start_key, step, nrows);
    const std::string fA = "/tmp/kline_regular_A.csv";
    const std::string fB = "/tmp/kline_regular_B.csv";
    write_csv_kline(rows, fA);
    write_csv_kline(rows, fB);

    // 2) Concat dataset: add two sources with different (Np, Nf) to exercise padding
    using CDS = MemoryMappedConcatDataset<Kline>;
    CDS cds;
    cds.add_dataset(/*csv*/ fA, /*Np*/ 5, /*Nf*/ 3, /*norm_window*/ 0, /*force_bin*/ true);
    cds.add_dataset(/*csv*/ fB, /*Np*/ 3, /*Nf*/ 5, /*norm_window*/ 0, /*force_bin*/ true);

    // 3) Sanity properties
    assert(cds.size().has_value());
    const std::size_t N = cds.size().value();
    assert(cds.max_N_past_   == 5);
    assert(cds.max_N_future_ == 5);

    using key_t = typename Kline::key_type_t;
    const key_t left  = cds.leftmost_key_value_;
    const key_t right = cds.rightmost_key_value_;
    const key_t gstep = cds.key_value_step_;
    assert(gstep > 0);
    assert(left <= right);

    // size formula matches
    const std::size_t Nexp = expected_count(left, right, gstep);
    assert(N == Nexp && "cds.size must equal (right-left)/step + 1");

    // 4) get_by_key_value smoke / shapes / keys on a middle anchor
    {
      const key_t mid_key = static_cast<key_t>(left + (N/2) * gstep);
      auto s = cds.get_by_key_value(mid_key);

      assert(s.features.dim()        == 3);                  // [K, max_N_past,   D]
      assert(s.future_features.dim() == 3);                  // [K, max_N_future, D]
      assert(s.mask.dim()            == 2);                  // [K, max_N_past]
      assert(s.future_mask.dim()     == 2);                  // [K, max_N_future]
      assert(s.past_keys.defined() && s.future_keys.defined());
      assert(s.past_keys.size(0)    == s.features.size(0));  // K
      assert(s.future_keys.size(0)  == s.features.size(0));  // K
      assert(!s.normalized);

      for (int c = 0; c < s.past_keys.size(0); ++c) {
        auto k_p = s.past_keys.index({c, s.past_keys.size(1)-1}).item<int64_t>(); // last past key (time t)
        auto k_f = s.future_keys.index({c, 0}).item<int64_t>();                   // first future key (t+1)
        assert(k_p == mid_key);
        assert(k_f == mid_key + gstep);
      }
    }

    // 5) Range slicing on concat (inclusive)
    {
      const key_t L = static_cast<key_t>(left + 2*gstep);
      const key_t R = static_cast<key_t>(left + 6*gstep);
      auto vec = cds.range_samples_by_keys(L, R);
      const std::size_t exp = expected_count(L, R, gstep);
      assert(vec.size() == exp);
      if (!vec.empty()) {
        const auto& s0 = vec.front();
        for (int c = 0; c < s0.past_keys.size(0); ++c) {
          auto k = s0.past_keys.index({c, s0.past_keys.size(1)-1}).item<int64_t>();
          assert(k == L);
        }
      }
    }

    // 6) index/key equivalence (get(i) == get_by_key_value(left + i*step))
    if (N > 0) {
      std::vector<std::size_t> idxs{0};
      if (N >= 3) idxs.push_back(N/2);
      if (N > 1)  idxs.push_back(N-1);

      for (auto i : idxs) {
        const key_t key_i = static_cast<key_t>(left + i * gstep);
        auto a = cds.get(i);
        auto b = cds.get_by_key_value(key_i);

        // shapes equal
        assert(a.features.sizes()        == b.features.sizes());
        assert(a.future_features.sizes() == b.future_features.sizes());
        assert(a.mask.sizes()            == b.mask.sizes());
        assert(a.future_mask.sizes()     == b.future_mask.sizes());

        // content: last past row (time t) & first future row (t+1) match for each channel
        for (int src = 0; src < a.features.size(0); ++src) {
          auto a_p = a.features.index({src, a.features.size(1)-1}).to(torch::kFloat32);
          auto b_p = b.features.index({src, b.features.size(1)-1}).to(torch::kFloat32);
          auto a_f = a.future_features.index({src, 0}).to(torch::kFloat32);
          auto b_f = b.future_features.index({src, 0}).to(torch::kFloat32);
          assert(torch::allclose(a_p, b_p, 1e-5, 1e-5));
          assert(torch::allclose(a_f, b_f, 1e-5, 1e-5));
        }
      }
    }

    std::cout << "[OK] memory_mapped_concat_dataset tests passed.\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
