/* test_memory_mapped_edge_cases.cpp */
#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"

using Kline = cuwacunu::camahjucunu::exchange::kline_t;
using Obs = cuwacunu::camahjucunu::data::observation_sample_t;

template <class T>
static std::string write_binary(const std::vector<T>& rows, const std::string& path) {
  std::ofstream ofs(path, std::ios::binary);
  if (!ofs) throw std::runtime_error("Failed to open binary for write: " + path);
  ofs.write(reinterpret_cast<const char*>(rows.data()),
            static_cast<std::streamsize>(rows.size() * sizeof(T)));
  ofs.close();
  return path;
}

static Kline make_kline(int64_t close_time, double base = 100.0) {
  Kline r{};
  r.open_time              = close_time - 1;
  r.open_price             = base;
  r.high_price             = base + 1.0;
  r.low_price              = base - 1.0;
  r.close_price            = base + 0.5;
  r.volume                 = 1000.0;
  r.close_time             = close_time;
  r.quote_asset_volume     = 1000.0 * base;
  r.number_of_trades       = 100;
  r.taker_buy_base_volume  = 600.0;
  r.taker_buy_quote_volume = 600.0 * base;
  return r;
}

static std::vector<Kline> make_kline_rows(int64_t start, int64_t step, int n, double base = 100.0) {
  std::vector<Kline> v;
  v.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) v.push_back(make_kline(start + i * step, base));
  return v;
}

static std::string write_kline_csv(const std::vector<Kline>& rows, const std::string& path, char delimiter = ',') {
  std::ofstream ofs(path);
  if (!ofs) throw std::runtime_error("Failed to open csv for write: " + path);
  for (const auto& r : rows) {
    std::ostringstream line;
    r.to_csv(line, delimiter);
    std::string s = line.str();
    if (!s.empty() && s.back() == '\n') ofs << s;
    else ofs << s << '\n';
  }
  ofs.close();
  return path;
}

template <typename T>
static std::vector<T> read_bin_all(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("Failed to open binary for read: " + path);
  in.seekg(0, std::ios::end);
  std::streamsize sz = in.tellg();
  in.seekg(0, std::ios::beg);
  if (sz < 0 || sz % static_cast<std::streamsize>(sizeof(T)) != 0) {
    throw std::runtime_error("Invalid binary size for file: " + path);
  }
  std::vector<T> out(static_cast<std::size_t>(sz / static_cast<std::streamsize>(sizeof(T))));
  if (!out.empty()) {
    in.read(reinterpret_cast<char*>(out.data()),
            static_cast<std::streamsize>(out.size() * sizeof(T)));
  }
  return out;
}

template <typename Fn>
static void expect_child_failure(Fn&& fn, const std::string& label) {
  const pid_t pid = fork();
  if (pid < 0) throw std::runtime_error("fork() failed for " + label);
  if (pid == 0) {
    try {
      fn();
      _exit(0);
    } catch (...) {
      _exit(1);
    }
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    throw std::runtime_error("waitpid() failed for " + label);
  }
  const bool success = WIFEXITED(status) && (WEXITSTATUS(status) == 0);
  if (success) {
    throw std::runtime_error("Expected child failure but got success for: " + label);
  }
}

int main() {
  try {
    // 1) single-record dataset should not read OOB; N_future=0 allows one sample
    {
      const std::string one_bin = "/tmp/mm_single_kline.bin";
      auto rows = make_kline_rows(/*start*/2000, /*step*/60, /*n*/1);
      write_binary(rows, one_bin);

      cuwacunu::camahjucunu::data::MemoryMappedDataset<Kline> ds(one_bin, /*N_past*/1, /*N_future*/0);
      assert(ds.size().has_value());
      assert(ds.size().value() == 1);

      auto s = ds.get(0);
      assert(s.features.defined() && s.features.dim() == 2 && s.features.size(0) == 1);
      assert(s.future_features.defined() && s.future_features.dim() == 2 && s.future_features.size(0) == 0);
      assert(s.past_keys.defined() && s.past_keys.numel() == 1);
      assert(s.future_keys.defined() && s.future_keys.numel() == 0);
      assert(s.past_keys.item<int64_t>() == rows[0].close_time);
      assert(!s.has_future_values());
    }

    // 2) N_past=0 constructor guard should fail (fatal path)
    {
      const std::string one_bin = "/tmp/mm_single_kline_guard.bin";
      auto rows = make_kline_rows(/*start*/3000, /*step*/60, /*n*/1);
      write_binary(rows, one_bin);
      expect_child_failure([&]() {
        cuwacunu::camahjucunu::data::MemoryMappedDataset<Kline> ds(one_bin, /*N_past*/0, /*N_future*/0);
        (void)ds;
      }, "MemoryMappedDataset N_past=0 guard");
    }

    // 3) one-anchor concat range must be valid (boundary case)
    {
      using CDS = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Kline>;
      const auto rows = make_kline_rows(/*start*/100, /*step*/1, /*n*/3);
      const std::string csv_a = "/tmp/mm_one_anchor_a.csv";
      const std::string csv_b = "/tmp/mm_one_anchor_b.csv";
      write_kline_csv(rows, csv_a);
      write_kline_csv(rows, csv_b);

      CDS cds;
      cds.add_dataset(csv_a, /*N_past*/2, /*N_future*/1, /*norm*/0, /*force*/true);
      cds.add_dataset(csv_b, /*N_past*/2, /*N_future*/1, /*norm*/0, /*force*/true);
      assert(cds.size().has_value());
      assert(cds.size().value() == 1);
      assert(cds.leftmost_key_value_ == 101);
      assert(cds.rightmost_key_value_ == 101);

      auto s = cds.get(0);
      assert(s.past_keys.defined() && s.past_keys.dim() == 2);
      assert(s.future_keys.defined() && s.future_keys.dim() == 2);
      for (int c = 0; c < s.past_keys.size(0); ++c) {
        assert(s.past_keys.index({c, s.past_keys.size(1)-1}).item<int64_t>() == 101);
        assert(s.future_keys.index({c, 0}).item<int64_t>() == 102);
      }

      auto rng = cds.range_samples_by_keys(101, 101);
      assert(rng.size() == 1);
    }

    // 4) normalized-zero payload must remain valid for kline_t
    {
      const std::string csv_const = "/tmp/mm_kline_constant.csv";
      const auto rows = make_kline_rows(/*start*/5000, /*step*/60, /*n*/5, /*base*/123.0);
      write_kline_csv(rows, csv_const);
      const std::string norm_bin = cuwacunu::camahjucunu::data::sanitize_csv_into_binary_file<Kline>(
        csv_const, /*normalization_window*/3, /*force*/true);

      auto norm_rows = read_bin_all<Kline>(norm_bin);
      assert(norm_rows.size() == rows.size());
      for (std::size_t i = 0; i < norm_rows.size(); ++i) {
        const auto& r = norm_rows[i];
        assert(r.close_time == rows[i].close_time);
        assert(r.open_time == rows[i].open_time);
        assert(r.open_time != INT64_MIN);
        // constant series -> z-score is 0.0 in current normalization policy
        assert(std::fabs(r.open_price) < 1e-12);
      }
    }

    // 5) collate/decollate keeps key tensors and normalization metadata aligned
    {
      constexpr int64_t C = 2, T = 3, Tf = 2, D = 4;
      Obs s0{
        torch::zeros({C, T, D}, torch::kFloat32),
        torch::ones({C, T}, torch::kBool),
        torch::ones({C, Tf, D}, torch::kFloat32),
        torch::ones({C, Tf}, torch::kBool),
        torch::Tensor(),
        /*normalized=*/true,
        torch::tensor({10.0f, 11.0f, 12.0f, 13.0f}, torch::kFloat32),
        torch::tensor({ 1.0f,  2.0f,  3.0f,  4.0f}, torch::kFloat32),
        torch::tensor({{100, 101, 102}, {100, 101, 102}}, torch::kInt64),
        torch::tensor({{103, 104}, {103, 104}}, torch::kInt64)
      };
      Obs s1{
        torch::full({C, T, D}, 2.0f, torch::kFloat32),
        torch::ones({C, T}, torch::kBool),
        torch::full({C, Tf, D}, 3.0f, torch::kFloat32),
        torch::ones({C, Tf}, torch::kBool),
        torch::Tensor(),
        /*normalized=*/true,
        torch::tensor({20.0f, 21.0f, 22.0f, 23.0f}, torch::kFloat32),
        torch::tensor({ 5.0f,  6.0f,  7.0f,  8.0f}, torch::kFloat32),
        torch::tensor({{200, 201, 202}, {200, 201, 202}}, torch::kInt64),
        torch::tensor({{203, 204}, {203, 204}}, torch::kInt64)
      };

      auto coll = Obs::collate_fn({s0, s1});
      assert(coll.features.sizes() == torch::IntArrayRef({2, C, T, D}));
      assert(coll.past_keys.sizes() == torch::IntArrayRef({2, C, T}));
      assert(coll.future_keys.sizes() == torch::IntArrayRef({2, C, Tf}));
      assert(coll.feature_mean.sizes() == torch::IntArrayRef({2, D}));
      assert(coll.feature_std.sizes() == torch::IntArrayRef({2, D}));
      assert(coll.normalized);

      auto back = Obs::decollate_fn(coll, /*clone_tensors=*/true);
      assert(back.size() == 2);
      assert(back[0].past_keys.sizes() == torch::IntArrayRef({C, T}));
      assert(back[1].past_keys.sizes() == torch::IntArrayRef({C, T}));
      assert(back[0].future_keys.sizes() == torch::IntArrayRef({C, Tf}));
      assert(back[1].future_keys.sizes() == torch::IntArrayRef({C, Tf}));
      assert(back[0].feature_mean.sizes() == torch::IntArrayRef({D}));
      assert(back[1].feature_mean.sizes() == torch::IntArrayRef({D}));
      assert(back[0].feature_std.sizes() == torch::IntArrayRef({D}));
      assert(back[1].feature_std.sizes() == torch::IntArrayRef({D}));
      assert(back[0].normalized && back[1].normalized);

      assert(torch::equal(back[0].past_keys, s0.past_keys));
      assert(torch::equal(back[1].past_keys, s1.past_keys));
      assert(torch::equal(back[0].future_keys, s0.future_keys));
      assert(torch::equal(back[1].future_keys, s1.future_keys));
      assert(torch::allclose(back[0].feature_mean, s0.feature_mean));
      assert(torch::allclose(back[1].feature_mean, s1.feature_mean));
      assert(torch::allclose(back[0].feature_std, s0.feature_std));
      assert(torch::allclose(back[1].feature_std, s1.feature_std));
    }

    // 6) CSV sanitize should fail when step ratio is not near an integer.
    {
      const std::string csv_bad_ratio = "/tmp/mm_bad_step_ratio.csv";
      std::vector<Kline> rows;
      rows.push_back(make_kline(/*close_time*/0,   100.0));
      rows.push_back(make_kline(/*close_time*/60,  101.0));
      rows.push_back(make_kline(/*close_time*/150, 102.0)); // +90 against base +60 => ratio 1.5
      write_kline_csv(rows, csv_bad_ratio);

      expect_child_failure([&]() {
        (void)cuwacunu::camahjucunu::data::sanitize_csv_into_binary_file<Kline>(
            csv_bad_ratio, /*normalization_window*/0, /*force*/true);
      }, "sanitize non-near-integer step ratio");
    }

    std::cout << "[OK] memory_mapped_edge_cases tests passed.\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
