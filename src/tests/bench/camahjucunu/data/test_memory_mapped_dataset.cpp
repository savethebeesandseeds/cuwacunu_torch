/* test_memory_mapped_dataset.cpp */
#include <torch/torch.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <cstdint>   // for int64_t

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

using cuwacunu::camahjucunu::data::MemoryMappedDataset;
using cuwacunu::camahjucunu::data::observation_sample_t;
using Kline = cuwacunu::camahjucunu::exchange::kline_t;

// ---------- helpers to build tiny kline datasets ----------
static Kline make_kline(int64_t close_time, bool valid = true, double base = 100.0, int i = 0) {
  Kline r{};
  r.open_time              = close_time - 1;
  r.open_price             = base + i;
  r.high_price             = r.open_price + 1.0;
  r.low_price              = r.open_price - 1.0;
  r.close_price            = r.open_price + 0.5;
  r.volume                 = 1000.0 + 10.0 * i;
  r.close_time             = close_time;
  r.quote_asset_volume     = r.volume * ((r.open_price + r.close_price) * 0.5);
  r.number_of_trades       = valid ? (100 + i) : 0;  // heuristic only; may not decide validity
  r.taker_buy_base_volume  = r.volume * 0.6;
  r.taker_buy_quote_volume = r.quote_asset_volume * 0.6;
  return r;
}

static std::vector<Kline> make_regular_rows(int64_t start_key, int64_t step, int n) {
  std::vector<Kline> v; v.reserve(n);
  for (int i = 0; i < n; ++i)
    v.push_back(make_kline(start_key + i * step, /*valid=*/(i % 7 != 0), 100.0, i));
  return v;
}

template <class T>
static std::string write_binary(const std::vector<T>& rows, const std::string& tmp_path) {
  std::ofstream ofs(tmp_path, std::ios::binary);
  if (!ofs) throw std::runtime_error("Failed to open tmp file for write: " + tmp_path);
  ofs.write(reinterpret_cast<const char*>(rows.data()),
            static_cast<std::streamsize>(rows.size() * sizeof(T)));
  ofs.close();
  return tmp_path;
}

static torch::Tensor features_f32(const Kline& r) {
  const auto vd = r.tensor_features();
  std::vector<float> vf(vd.begin(), vd.end());
  return torch::from_blob(vf.data(), {(long)vf.size()}, torch::kFloat32).clone();
}

static void print_shape(const char* name, const torch::Tensor& t) {
  std::cout << name << ": [";
  for (auto i = 0; i < t.dim(); ++i) std::cout << t.size(i) << (i+1<t.dim() ? "x" : "");
  std::cout << "]\n";
}

// Find the last index with close_time <= target
static std::size_t find_le_idx(const std::vector<Kline>& v, int64_t target) {
  std::size_t idx = 0;
  for (std::size_t j = 0; j < v.size(); ++j) { if (v[j].close_time <= target) idx = j; else break; }
  return idx;
}

int main() try {
  std::cout << std::fixed << std::setprecision(0);

  // ---------- 1) Build rows and ensure at least one invalid FUTURE row ----------
  auto rows_regular = make_regular_rows(/*start_key=*/1000, /*step=*/1, /*n=*/32);

  // Pick an anchor i such that rows_regular[i+1] is invalid according to THE ACTUAL is_valid()
  int anchor_with_invalid_future = -1;
  for (int i = 0; i + 1 < (int)rows_regular.size(); ++i) {
    if (!rows_regular[i + 1].is_valid()) { anchor_with_invalid_future = i; break; }
  }
  if (anchor_with_invalid_future < 0) {
    // Guarantee at least one invalid by replacing a row with a canonical null/invalid record.
    // Keep the regular grid by preserving the original key.
    const int j = std::min(7, (int)rows_regular.size() - 2);
    const auto key = rows_regular[j + 1].close_time;
    rows_regular[j + 1] = Kline::null_instance(key);   // guaranteed invalid
    anchor_with_invalid_future = j;
  }

  // Write the finalized sequence to disk AFTER any forced invalidation.
  const std::string f_regular_bin = "/tmp/kline_regular.bin";
  write_binary(rows_regular, f_regular_bin);

  using DS = MemoryMappedDataset<Kline>;
  DS ds_regular(f_regular_bin);

  // Dataset defaults: Np=1, Nf=1 -> size = rows - 1
  const size_t expected = rows_regular.size() - (/*Np*/1 + /*Nf*/1) + 1;
  assert(ds_regular.size().value() == expected);
  assert(ds_regular.key_value_step_ == 1);

  // ---------- 2) (C) keys in get_sequences_around_key_value ----------
  {
    const int64_t target_key = 1015;
    const size_t  Np = 5, Nf = 3;
    auto s = ds_regular.get_sequences_around_key_value(target_key, Np, Nf);
    assert(s.past_keys.defined() && s.future_keys.defined());
    assert(s.past_keys.size(0)   == (long)Np);
    assert(s.future_keys.size(0) == (long)Nf);
    // last past key == target anchor
    assert(s.past_keys.index({(long)Np-1}).item<int64_t>() == target_key);
    assert(s.future_keys.index({0}).item<int64_t>() == target_key + 1);
  }

  // ---------- 3) (B) has_future_values() should be false when the next row is invalid ----------
  {
    auto s = ds_regular.get(/*index*/anchor_with_invalid_future); // default Np=1,Nf=1
    assert(!s.has_future_values() && "future mask is fully invalid");
  }

  // ---------- 4) (B) normalize/denormalize toggles (identity stats) ----------
  {
    auto s = ds_regular.get(/*index*/10);
    // identity stats
    s.feature_mean = torch::zeros({ s.features.size(-1) }, s.features.options());
    s.feature_std  = torch::ones ({ s.features.size(-1) }, s.features.options());
    s.normalize_inplace();
    assert(s.normalized);
    s.denormalize_inplace();
    assert(!s.normalized);
  }

  // ---------- 5) (A) range slicing on dataset (inclusive) ----------
  {
    const long left  = 1005;
    const long right = 1008;
    auto vec = ds_regular.range_samples_by_keys(left, right);
    assert(vec.size() == (size_t)(right - left + 1));
    for (int i = 0; i < (int)vec.size(); ++i) {
      const long anchor_key = left + i;
      const auto& s = vec[i];
      // default Np=Nf=1: past_keys[0]==anchor, future_keys[0]==anchor+1
      assert(s.past_keys.item<int64_t>()   == anchor_key);
      assert(s.future_keys.item<int64_t>() == anchor_key + 1);
      // last past row equals row[anchor], first future equals row[anchor+1]
      const std::size_t j = (std::size_t)(anchor_key - rows_regular.front().close_time);
      auto exp_p = features_f32(rows_regular[j]);
      auto exp_f = features_f32(rows_regular[j + 1]);
      auto got_p = s.features.index({0}).to(torch::kFloat32);
      auto got_f = s.future_features.index({0}).to(torch::kFloat32);
      assert(torch::allclose(got_p, exp_p, 1e-6, 1e-6));
      assert(torch::allclose(got_f, exp_f, 1e-6, 1e-6));
    }
  }

  std::cout << "[OK] memory_mapped_dataset<kline_t> tests passed.\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "Test failed with exception: " << e.what() << "\n";
  return 1;
}
