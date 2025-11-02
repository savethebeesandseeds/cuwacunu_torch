/* test_memory_mapped_datafile.cpp */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include <cassert>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/data/memory_mapped_datafile.h"

namespace fs = std::filesystem;
using cuwacunu::camahjucunu::data::sanitize_csv_into_binary_file;
using cuwacunu::camahjucunu::data::is_bin_filename_normalized;

// ---------- Small helpers ----------------------------------------------------
template <typename T>
static bool bytes_equal(const T& a, const T& b) {
  static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}

template <typename T>
static std::vector<T> read_bin_all(const std::string& bin_path) {
  std::vector<T> out;
  std::ifstream in(bin_path, std::ios::binary);
  if (!in.is_open()) log_fatal("[test] Could not open BIN for reading: %s\n", bin_path.c_str());
  in.seekg(0, std::ios::end);
  const std::streamsize sz = in.tellg();
  in.seekg(0, std::ios::beg);
  if (sz % static_cast<std::streamsize>(sizeof(T)) != 0)
    log_fatal("[test] BIN size not multiple of sizeof(T) for %s\n", bin_path.c_str());
  const std::size_t n = static_cast<std::size_t>(sz / sizeof(T));
  out.resize(n);
  if (n) {
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(n * sizeof(T)));
    if (!in) log_fatal("[test] Failed reading BIN payload: %s\n", bin_path.c_str());
  }
  return out;
}

// Build expected sanitized (pre-normalized) sequence (duplicates skipped, gaps filled with nulls).
template <typename T>
static std::vector<T> sanitize_in_memory(const std::string& csv_path, char delimiter=',') {
  std::ifstream csv = cuwacunu::piaabo::dfiles::readFileToStream(csv_path);
  if (!csv.is_open()) log_fatal("[test] Could not open CSV: %s\n", csv_path.c_str());

  std::string line_p0, line_p1;
  std::size_t line_no = 0;
  if (!std::getline(csv, line_p0)) log_fatal("[test] CSV too short: %s\n", csv_path.c_str());

  T p0 = T::from_csv(line_p0, delimiter, ++line_no);

  std::vector<T> out; out.reserve(1024);

  bool have_regular_delta = false;
  long double regular_delta = 0.0L;
  constexpr long double tol = 1e-8L;

  while (std::getline(csv, line_p1)) {
    T p1 = T::from_csv(line_p1, delimiter, ++line_no);
    if (!p1.is_valid()) { p0 = p1; continue; } // skip invalid next (unknown key placement)

    const long double kv0 = static_cast<long double>(p0.key_value());
    const long double kv1 = static_cast<long double>(p1.key_value());
    const long double d   = kv1 - kv0;

    if (std::fabs(d) <= tol) { p0 = p1; continue; } // duplicate key -> collapsed to latest
    if (d < 0.0L) log_fatal("[test] Decreasing key in CSV %s\n", csv_path.c_str());
    if (!have_regular_delta) { regular_delta = d; have_regular_delta = true; }

    const auto steps = static_cast<std::int64_t>(std::llround(d / regular_delta));
    for (std::int64_t i = 0; i < steps; ++i) {
      const bool first = (i == 0);
      T px = first ? p0
                   : T::null_instance(static_cast<typename T::key_type_t>(
                        kv0 + static_cast<long double>(i) * regular_delta));
      out.push_back(px);
    }
    p0 = p1;
  }

  out.push_back(p0); // final anchor
  return out;
}

// Simulate the header's causal_keep_len normalization exactly, producing the expected sequence.
template <typename T>
static std::vector<T> simulate_causal_keep_len_normalization(const std::vector<T>& sanitized_seq,
                                                             std::size_t window_size) {
  auto pack = T::initialize_statistics_pack((unsigned)window_size);
  std::vector<T> out; out.reserve(sanitized_seq.size());

  std::size_t filled_valid = 0;
  for (std::size_t i=0; i<sanitized_seq.size(); ++i) {
    const T& x = sanitized_seq[i];
    T y = x;

    if (x.is_valid() && filled_valid >= window_size) {
      y = pack.normalize(x);
    }
    out.push_back(y);

    if (x.is_valid()) {
      pack.update(x);
      if (filled_valid < window_size) ++filled_valid;
    }
  }
  return out;
}

// ---------- CSV Writers ------------------------------------------------------

static std::string write_kline_csv(const fs::path& dir) {
  // open_time,open,high,low,close,vol,close_time,quote_vol,ntr,tb_base,tb_quote,ignore
  std::string path = (dir / "klines.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  auto emit = [&](int64_t ot,double o,double h,double l,double c,double v,
                  int64_t ct,double qv,int32_t n,double tb,double tq){
    os<<ot<<','<<o<<','<<h<<','<<l<<','<<c<<','<<v<<','<<ct<<','<<qv<<','<<n<<','<<tb<<','<<tq<<','<<0<<'\n';
  };
  emit(0,100,105, 99,102,1000,  60,102000,10,400,40800);
  emit(0,102,106,101,103,1100, 120,123000,12,420,43200); // +60
  emit(0,103,107,102,104,1200, 120,125000,12,425,44000); // duplicate
  emit(0,104,108,103,105,1300, 300,160000,15,450,48000); // +180 -> 2 nulls inserted (at 180, 240)
  return path;
}

static std::string write_basic_csv(const fs::path& dir) {
  // time,value  (delta=0.5; gap +1.5 -> two nulls)
  std::string path = (dir / "basic.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  os.setf(std::ios::fixed); os.precision(6);
  os << "0.000000, 10.0\n";
  os << "0.500000, 11.0\n";
  os << "0.500000, 11.5\n";  // duplicate (skip)
  os << "2.000000, 13.0\n";  // +1.5 -> nulls at 1.0, 1.5
  return path;
}

// ---------- Per-type runner --------------------------------------------------

template <typename T>
static void run_case(const std::string& label,
                     const std::string& csv_path,
                     std::size_t norm_window) {
  log_info("\n[test][%s] CSV: %s\n", label.c_str(), csv_path.c_str());

  // 1) Sanitize without normalization (byte-for-byte lattice/nulls test)
  std::string bin_no_norm = sanitize_csv_into_binary_file<T>(csv_path, /*norm_window=*/0,
                                                             /*force_binarization=*/true,
                                                             /*buffer_size=*/4, /*delimiter=*/',');
  // Name detector: RAW should be not-normalized; window_out must remain untouched.
  std::size_t Wdet = 123456;
  bool is_norm = is_bin_filename_normalized(bin_no_norm, &Wdet);
  if (is_norm) log_fatal("[test][%s] raw file unexpectedly marked as normalized: %s\n", label.c_str(), bin_no_norm.c_str());
  if (Wdet != 123456) log_fatal("[test][%s] window_out modified for raw file\n", label.c_str());

  auto recs_no_norm = read_bin_all<T>(bin_no_norm);
  auto sanitized_in_mem = sanitize_in_memory<T>(csv_path, ',');

  if (sanitized_in_mem.size() != recs_no_norm.size())
    log_fatal("[test][%s] sanitize size mismatch: mem=%zu bin=%zu\n",
              label.c_str(), sanitized_in_mem.size(), recs_no_norm.size());
  for (size_t i=0;i<recs_no_norm.size();++i)
    if (!bytes_equal(recs_no_norm[i], sanitized_in_mem[i]))
      log_fatal("[test][%s] sanitize byte mismatch @%zu\n", label.c_str(), i);
  log_info("[test][%s] ✔ Sanitize byte-identical (no-norm)\n", label.c_str());

  // 2) Sanitize WITH normalization (causal_keep_len, in-place on copy)
  const std::size_t W = norm_window;
  std::string bin_norm = sanitize_csv_into_binary_file<T>(csv_path, /*norm_window=*/W,
                                                          /*force_binarization=*/true);
  std::size_t Wgot = 0;
  bool is_norm2 = is_bin_filename_normalized(bin_norm, &Wgot);
  if (!is_norm2) log_fatal("[test][%s] normalized file not detected by name: %s\n", label.c_str(), bin_norm.c_str());
  if (Wgot != W)
    log_fatal("[test][%s] window parsed from name != requested window (got %zu, want %zu) file=%s\n",
              label.c_str(), Wgot, W, bin_norm.c_str());

  auto recs_norm = read_bin_all<T>(bin_norm);

  if (recs_norm.size() != recs_no_norm.size())
    log_fatal("[test][%s] normalized BIN changed record count (keep_len policy expected same size)\n",
              label.c_str());

  auto expected_norm = simulate_causal_keep_len_normalization<T>(sanitized_in_mem, W);
  if (expected_norm.size() != recs_norm.size())
    log_fatal("[test][%s] simulate size mismatch\n", label.c_str());

  std::size_t valid_seen = 0, normalized = 0, invalid_passthrough = 0;
  for (size_t i=0;i<recs_norm.size();++i) {
    const T& s = sanitized_in_mem[i];
    const T& y = recs_norm[i];
    const T& e = expected_norm[i];

    if (!bytes_equal(y, e))
      log_fatal("[test][%s] norm byte mismatch @%zu\n", label.c_str(), i);

    if (s.is_valid()) {
      if (valid_seen >= W) ++normalized;
      ++valid_seen;
    } else {
      if (!bytes_equal(s, y))
        log_fatal("[test][%s] invalid record modified @%zu\n", label.c_str(), i);
      ++invalid_passthrough;
    }
  }

  log_info("[test][%s] ✔ Causal keep_len matches. W=%zu, burn_in_valid=%zu, normalized=%zu, invalid_passthrough=%zu\n",
           label.c_str(), W, std::min(valid_seen, W), normalized, invalid_passthrough);

  // 3) Idempotency: rerun without force should skip and yield identical bytes
  std::string bin_norm2 = sanitize_csv_into_binary_file<T>(csv_path, W, /*force*/false);
  auto recs_norm2 = read_bin_all<T>(bin_norm2);
  if (recs_norm2.size() != recs_norm.size())
    log_fatal("[test][%s] Up-to-date skip changed size\n", label.c_str());
  for (size_t i=0;i<recs_norm.size();++i)
    if (!bytes_equal(recs_norm2[i], recs_norm[i]))
      log_fatal("[test][%s] Up-to-date skip changed bytes @%zu\n", label.c_str(), i);

  std::size_t Wgot2 = 0;
  bool is_norm3 = is_bin_filename_normalized(bin_norm2, &Wgot2);
  if (!is_norm3) log_fatal("[test][%s] up-to-date normalized name not detected\n", label.c_str());
  if (Wgot2 != W) log_fatal("[test][%s] window parse mismatch on re-run (got %zu, want %zu)\n", label.c_str(), Wgot2, W);

  log_info("[test][%s] ✔ All checks passed.\n", label.c_str());
}

int main() {
  try {
    // --- Name detector smoke tests (no file I/O needed) ---
    {
      std::size_t w = 777;
      bool n = is_bin_filename_normalized("/tmp/foo.bin", &w);
      assert(!n && w == 777);

      w = 777;
      n = is_bin_filename_normalized("/tmp/foo.norm.bin", &w);   // legacy/invalid
      assert(!n && w == 777);

      w = 777;
      n = is_bin_filename_normalized("/tmp/foo.normW0.bin", &w); // window 0 -> not normalized
      assert(!n && w == 777);

      w = 777;
      n = is_bin_filename_normalized("/tmp/foo.normW64.bin", &w);
      assert(n && w == 64);
    }

    fs::path tmp = fs::path("/cuwacunu/data/tests");
    fs::create_directories(tmp);

    const std::size_t W = 3; // normalization window >= 1 => normalized

    // --- kline_t ---
    auto kline_csv = write_kline_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::kline_t>("kline_t", kline_csv, W);

    // --- basic_t ---
    auto basic_csv = write_basic_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::basic_t>("basic_t", basic_csv, W);

    log_info("\n[test] ✅ All test cases succeeded.\n");
    return 0;
  } catch (const std::exception& e) {
    log_err("[test] Exception: %s\n", e.what());
    return 2;
  }
}
