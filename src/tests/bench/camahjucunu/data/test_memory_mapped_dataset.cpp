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

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

using cuwacunu::camahjucunu::data::MemoryMappedDataset;
using cuwacunu::camahjucunu::data::MemoryMappedConcatDataset;
using cuwacunu::camahjucunu::data::observation_sample_t;
using Kline = cuwacunu::camahjucunu::exchange::kline_t;

// We assume close_time appears at this column in kline_t::tensor_features()
static constexpr int CLOSE_TIME_COL = 6;

// ---------- helpers to build tiny kline datasets ----------
static Kline make_kline(int64_t close_time, bool valid = true, double base = 100.0, int i = 0) {
  Kline r{};
  r.open_time            = close_time - 1; // arbitrary
  r.open_price           = base + i;
  r.high_price           = r.open_price + 1.0;
  r.low_price            = r.open_price - 1.0;
  r.close_price          = r.open_price + 0.5;
  r.volume               = 1000.0 + 10.0 * i;
  r.close_time           = close_time;
  r.quote_asset_volume   = r.volume * ((r.open_price + r.close_price) * 0.5);
  r.number_of_trades     = valid ? (100 + i) : 0;  // heuristically makes is_valid() true/false
  r.taker_buy_base_volume  = r.volume * 0.6;
  r.taker_buy_quote_volume = r.quote_asset_volume * 0.6;
  return r;
}

static std::vector<Kline> make_regular_rows(int64_t start_key, int64_t step, int n) {
  std::vector<Kline> v; v.reserve(n);
  for (int i = 0; i < n; ++i) v.push_back(make_kline(start_key + i * step, /*valid=*/(i % 7 != 0), 100.0, i));
  return v;
}

static std::vector<Kline> make_duplicate_key_rows(int64_t start_key, int n, int dup_at_idx) {
  auto v = make_regular_rows(start_key, /*step=*/1, n);
  if (dup_at_idx > 0 && dup_at_idx < n) {
    v[dup_at_idx].close_time = v[dup_at_idx - 1].close_time; // force duplicate
  }
  return v;
}

static std::vector<Kline> make_irregular_rows(int64_t start_key) {
  // keys: t, t+1, t+3, t+6, t+7, t+11  (irregular deltas)
  std::vector<int64_t> keys = { start_key+0, start_key+1, start_key+3, start_key+6, start_key+7, start_key+11 };
  std::vector<Kline> v; v.reserve(keys.size());
  for (int i = 0; i < (int)keys.size(); ++i) v.push_back(make_kline(keys[i], true, 200.0, i));
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

static std::string write_csv_kline(const std::vector<Kline>& rows, const std::string& path, char delimiter = ',') {
  std::ofstream ofs(path);
  if (!ofs) throw std::runtime_error("Failed to open tmp csv for write: " + path);

  for (const auto& r : rows) {
    std::ostringstream line;
    r.to_csv(line, delimiter);               // whatever r.to_csv writes
    std::string s = line.str();

    // ensure exactly ONE newline per record
    if (!s.empty() && s.back() == '\n') {
      ofs << s;                              // already newline-terminated
    } else {
      ofs << s << '\n';                      // add newline
    }
  }
  ofs.close();
  return path;
}

static void print_shape(const char* name, const torch::Tensor& t) {
  std::cout << name << ": [";
  for (auto i = 0; i < t.dim(); ++i) std::cout << t.size(i) << (i+1<t.dim() ? "x" : "");
  std::cout << "]\n";
}

// Find the last index with close_time <= target (works for regular/irregular/duplicates)
static std::size_t find_le_idx(const std::vector<Kline>& v, int64_t target) {
  std::size_t idx = 0;
  for (std::size_t j = 0; j < v.size(); ++j) {
    if (v[j].close_time <= target) idx = j; else break;
  }
  return idx;
}

// Build a float32 tensor from kline_t::tensor_features() for robust row-wise comparison
static torch::Tensor features_f32(const Kline& r) {
  const auto vd = r.tensor_features();                 // std::vector<double>
  std::vector<float> vf(vd.begin(), vd.end());         // convert to float
  return torch::from_blob(vf.data(), {(long)vf.size()}, torch::kFloat32).clone(); // own the memory
}


int main() try {
  std::cout << std::fixed << std::setprecision(0);

  // ---------- 1) Basic dataset: regular keys ----------
  const std::string f_regular_bin = "/tmp/kline_regular.bin";
  auto rows_regular = make_regular_rows(/*start_key=*/1000, /*step=*/1, /*n=*/32);
  write_binary(rows_regular, f_regular_bin);

  using DS = MemoryMappedDataset<Kline>;
  DS ds_regular(f_regular_bin);

  // sanity
  const size_t expected = rows_regular.size() - (/*Np*/1 + /*Nf*/1) + 1; // = rows-1
  assert(ds_regular.size().value() == expected);
  assert(ds_regular.key_value_step_ == 1);

  // pick a target inside the series
  const int64_t target_key = 1015; // exact key exists
  const size_t  Np = 5;
  const size_t  Nf = 3;

  auto sample_mid = ds_regular.get_sequences_around_key_value(target_key, Np, Nf);
  // shapes
  assert(sample_mid.features.size(0)        == (long)Np);
  assert(sample_mid.future_features.size(0) == (long)Nf);
  assert(sample_mid.mask.size(0)            == (long)Np);
  assert(sample_mid.future_mask.size(0)     == (long)Nf);
  // semantics: last past row is at t (closest <= target); first future row is at t+1
  {
    // since rows_regular is 1000..1000+31 step=1, index for target is exact:
    const std::size_t i = static_cast<std::size_t>(target_key - rows_regular.front().close_time);

    auto exp_past   = features_f32(rows_regular[i]);
    auto exp_future = features_f32(rows_regular[i + 1]);

    auto got_past   = sample_mid.features.index({(long)Np - 1}).to(torch::kFloat32);
    auto got_future = sample_mid.future_features.index({0}).to(torch::kFloat32);

    assert(got_past.sizes()   == exp_past.sizes());
    assert(got_future.sizes() == exp_future.sizes());
    assert(torch::allclose(got_past,   exp_past,   1e-6, 1e-6));
    assert(torch::allclose(got_future, exp_future, 1e-6, 1e-6));
  }

  // too-early request should throw (no enough history for Np=5 at the very first key)
  bool threw_low = false;
  try {
    (void)ds_regular.get_sequences_around_key_value(rows_regular.front().close_time, Np, Nf);
  } catch (const std::out_of_range&) { threw_low = true; }
  assert(threw_low && "expect out_of_range for left edge when Np window doesn't fit");

  // leftmost valid target key = first index where a full past window exists
  const int64_t safe_low_key =
      rows_regular.front().close_time + static_cast<int64_t>(Np - 1);
  auto s_low = ds_regular.get_sequences_around_key_value(safe_low_key, Np, Nf);
  assert(s_low.features.size(0)        == (long)Np);
  assert(s_low.future_features.size(0) == (long)Nf);

  // outside high bound should throw when Nf would overflow
  bool threw = false;
  try {
    auto s_high = ds_regular.get_sequences_around_key_value(1000 + (int64_t)rows_regular.size() + 100, Np, Nf);
    (void)s_high;
  } catch (const std::out_of_range&) { threw = true; }
  assert(threw && "expect out_of_range for far right key");

  // ---------- 2) Duplicate keys ----------
  const std::string f_dup_bin = "/tmp/kline_dup.bin";
  auto rows_dup = make_duplicate_key_rows(/*start_key=*/2000, /*n=*/16, /*dup_at_idx=*/8);
  write_binary(rows_dup, f_dup_bin);
  DS ds_dup(f_dup_bin);
  const int64_t dup_key = rows_dup[8].close_time; // equals rows_dup[7].close_time
  auto s_dup = ds_dup.get_sequences_around_key_value(dup_key, Np, Nf);
  {
    auto got_past   = s_dup.features.index({(long)Np - 1}).to(torch::kFloat32);
    auto got_future = s_dup.future_features.index({0}).to(torch::kFloat32);

    bool matched = false;
    for (std::size_t j = 0; j + 1 < rows_dup.size(); ++j) {
      if (rows_dup[j].close_time <= dup_key) {
        auto exp_p = features_f32(rows_dup[j]);
        auto exp_f = features_f32(rows_dup[j + 1]);
        if (torch::allclose(got_past, exp_p, 1e-6, 1e-6) &&
            torch::allclose(got_future, exp_f, 1e-6, 1e-6)) {
          matched = true;
          break;
        }
      } else {
        break; // rows are increasing; past keys after target won't match
      }
    }
    assert(matched && "past must be some j with key<=target and future must be j+1");
  }


  // ---------- 3) Irregular step (still increasing) ----------
  const std::string f_irreg_bin = "/tmp/kline_irreg.bin";
  auto rows_irreg = make_irregular_rows(/*start_key=*/3000);
  write_binary(rows_irreg, f_irreg_bin);
  DS ds_irreg(f_irreg_bin);
  // pick a target between 3003 and 3006, e.g. 3005 → closest <= is 3003
  auto s_ir = ds_irreg.get_sequences_around_key_value(/*target=*/3005, /*Np=*/3, /*Nf=*/2);
  {
    const std::size_t i = find_le_idx(rows_irreg, /*target=*/3005);
    auto exp_past   = features_f32(rows_irreg[i]);
    auto exp_future = features_f32(rows_irreg[i + 1]);

    auto got_past   = s_ir.features.index({2}).to(torch::kFloat32);   // Np=3 -> last row is index 2
    auto got_future = s_ir.future_features.index({0}).to(torch::kFloat32);

    assert(torch::allclose(got_past,   exp_past,   1e-6, 1e-6));
    assert(torch::allclose(got_future, exp_future, 1e-6, 1e-6));
  }

  // ---------- 4) Concat dataset: two sources via CSV, different (Npast,Nfuture), padding ----------
  using CDS = MemoryMappedConcatDataset<Kline>;
  CDS cds;

  // Build tiny CSVs from regular rows (so add_dataset can sanitize CSV -> BIN internally)
  const std::string f_regular_csv_A = "/tmp/kline_regular_A.csv";
  const std::string f_regular_csv_B = "/tmp/kline_regular_B.csv";
  write_csv_kline(rows_regular, f_regular_csv_A);
  write_csv_kline(rows_regular, f_regular_csv_B);

  // source A: Np=5, Nf=3
  cds.add_dataset(/*csv*/ f_regular_csv_A, /*Np*/ 5, /*Nf*/ 3, /*norm_window*/ 0, /*force_bin*/ true);
  // source B: Np=3, Nf=5
  cds.add_dataset(/*csv*/ f_regular_csv_B, /*Np*/ 3, /*Nf*/ 5, /*norm_window*/ 0, /*force_bin*/ true);

  // Verify padding: max_N_past=5, max_N_future=5
  assert(cds.max_N_past_   == 5);
  assert(cds.max_N_future_ == 5);

  // Choose a key in the middle of the intersection
  auto s_cat = cds.get_by_key_value(/*key*/1018);
  // shape: [K=2, max_N_past, D] and [K=2, max_N_future, D]
  assert(s_cat.features.dim()        == 3 && s_cat.features.size(0) == 2 && s_cat.features.size(1) == 5);
  assert(s_cat.future_features.dim() == 3 && s_cat.future_features.size(0) == 2 && s_cat.future_features.size(1) == 5);
  assert(s_cat.mask.sizes()          == torch::IntArrayRef({2, 5}));
  assert(s_cat.future_mask.sizes()   == torch::IntArrayRef({2, 5}));

  // Row-wise match: last past row should equal features at j = last index with key<=target,
  // and first future row should equal features at j+1.
  {
    const int64_t target = 1018;
    const std::size_t j = find_le_idx(rows_regular, target);    // last <= target

    auto exp_p = features_f32(rows_regular[j]);      // time t
    auto exp_f = features_f32(rows_regular[j + 1]);  // time t+1

    for (int src = 0; src < 2; ++src) {
      auto got_p = s_cat.features.index({src, 4}).to(torch::kFloat32);  // last of past window
      auto got_f = s_cat.future_features.index({src, 0}).to(torch::kFloat32); // first of future

      assert(got_p.sizes() == exp_p.sizes());
      assert(got_f.sizes() == exp_f.sizes());
      assert(torch::allclose(got_p, exp_p, 1e-6, 1e-6));
      assert(torch::allclose(got_f, exp_f, 1e-6, 1e-6));
    }
  }


  // ---------- 5) Simple single-index get() smoke ----------
  auto g = ds_regular.get(10);
  assert(g.features.size(0)        == 1);
  assert(g.future_features.size(0) == 1);
  assert(g.mask.size(0)            == 1);
  assert(g.future_mask.size(0)     == 1);

  std::cout << "[OK] All memory_mapped_dataset<kline_t> tests passed.\n";
  return 0;

} catch (const std::exception& e) {
  std::cerr << "Test failed with exception: " << e.what() << "\n";
  return 1;
}
