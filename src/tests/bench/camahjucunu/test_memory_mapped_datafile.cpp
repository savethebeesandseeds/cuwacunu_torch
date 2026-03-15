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
#include <limits>
#include <optional>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/data/memory_mapped_datafile.h"

namespace fs = std::filesystem;
using cuwacunu::camahjucunu::data::sanitize_csv_into_binary_file;
using cuwacunu::camahjucunu::data::is_bin_filename_normalized;
using cuwacunu::camahjucunu::data::canonical_normalization_policy;
using cuwacunu::camahjucunu::data::binary_record_type_t;

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

template <typename RawT>
static std::vector<binary_record_type_t<RawT>> materialize_binary_records(
    const std::vector<RawT>& raw_records) {
  using BinaryT = binary_record_type_t<RawT>;
  std::vector<BinaryT> out;
  out.reserve(raw_records.size());
  for (const auto& raw : raw_records) {
    out.push_back(BinaryT::from_raw(raw));
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

// Simulate the header's causal log-return preprocessing exactly,
// producing the expected sequence.
template <typename T>
static std::vector<T> simulate_log_returns_normalization(
    const std::vector<T>& sanitized_seq) {
  std::vector<T> out; out.reserve(sanitized_seq.size());
  std::optional<T> previous_valid{};
  for (std::size_t i=0; i<sanitized_seq.size(); ++i) {
    const T& x = sanitized_seq[i];
    T y = x;

    if (x.is_valid()) {
      if (previous_valid.has_value()) {
        y = T::normalize_log_returns(x, previous_valid ? &(*previous_valid) : nullptr);
      } else {
        T key_holder = x;
        y = T::null_instance(key_holder.key_value());
      }
      previous_valid = x;
    } else {
      previous_valid.reset();
    }
    out.push_back(y);
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

static std::string write_kline_bootstrap_outlier_csv(const fs::path& dir) {
  // close_time deltas: +180, +60, +60
  // bootstrap-min-step policy should infer regular_delta=60 (not 180).
  std::string path = (dir / "klines_bootstrap_outlier.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  auto emit = [&](int64_t ot,double o,double h,double l,double c,double v,
                  int64_t ct,double qv,int32_t n,double tb,double tq){
    os<<ot<<','<<o<<','<<h<<','<<l<<','<<c<<','<<v<<','<<ct<<','<<qv<<','<<n<<','<<tb<<','<<tq<<','<<0<<'\n';
  };
  emit(0,100,105, 99,102,1000,  60,102000,10,400,40800); // anchor
  emit(0,101,106,100,103,1100, 240,123000,12,420,43200); // +180
  emit(0,102,107,101,104,1200, 300,125000,12,425,44000); // +60
  emit(0,103,108,102,105,1300, 360,160000,15,450,48000); // +60
  return path;
}

static std::string write_trade_csv(const fs::path& dir) {
  std::string path = (dir / "trades.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  auto emit = [&](int64_t id, double price, double qty, double quote_qty,
                  int64_t time, bool is_buyer_maker, bool is_best_match) {
    os << id << ',' << price << ',' << qty << ',' << quote_qty << ','
       << time << ',' << std::boolalpha << is_buyer_maker << ','
       << is_best_match << '\n';
  };
  emit(1, 100.0, 1.5, 150.0, 60, false, true);
  emit(2, 101.0, 2.0, 202.0, 120, true, true);
  emit(3, 102.0, 2.2, 224.4, 120, true, true);   // duplicate
  emit(4, 104.0, 3.0, 312.0, 240, false, false); // +120 -> one null inserted
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
                     const std::string& normalization_policy) {
  log_info("\n[test][%s] CSV: %s\n", label.c_str(), csv_path.c_str());
  const std::string canonical_policy =
      canonical_normalization_policy(normalization_policy);

  // 1) Sanitize without normalization (byte-for-byte lattice/nulls test)
  std::string bin_no_norm = sanitize_csv_into_binary_file<T>(csv_path, /*normalization_policy=*/"none",
                                                             /*force_rebuild_cache=*/true,
                                                             /*buffer_size=*/4, /*delimiter=*/',');
  // Name detector: RAW should be not-normalized; policy_out must remain untouched.
  std::string detected_policy = "sentinel";
  bool is_norm = is_bin_filename_normalized(bin_no_norm, &detected_policy);
  if (is_norm) log_fatal("[test][%s] raw file unexpectedly marked as normalized: %s\n", label.c_str(), bin_no_norm.c_str());
  if (detected_policy != "sentinel") log_fatal("[test][%s] policy_out modified for raw file\n", label.c_str());

  using BinaryT = binary_record_type_t<T>;
  auto recs_no_norm = read_bin_all<BinaryT>(bin_no_norm);
  auto sanitized_raw = sanitize_in_memory<T>(csv_path, ',');
  auto sanitized_in_mem = materialize_binary_records<T>(sanitized_raw);

  if (sanitized_in_mem.size() != recs_no_norm.size())
    log_fatal("[test][%s] sanitize size mismatch: mem=%zu bin=%zu\n",
              label.c_str(), sanitized_in_mem.size(), recs_no_norm.size());
  for (size_t i=0;i<recs_no_norm.size();++i)
    if (!bytes_equal(recs_no_norm[i], sanitized_in_mem[i]))
      log_fatal("[test][%s] sanitize byte mismatch @%zu\n", label.c_str(), i);
  log_info("[test][%s] ✔ Sanitize byte-identical (no-norm)\n", label.c_str());

  // 2) Sanitize WITH normalization (causal log-return preprocessing, in-place on copy)
  std::string bin_norm = sanitize_csv_into_binary_file<T>(
      csv_path, /*normalization_policy=*/normalization_policy,
      /*force_rebuild_cache=*/true);
  std::string policy_got;
  bool is_norm2 = is_bin_filename_normalized(bin_norm, &policy_got);
  if (!is_norm2) log_fatal("[test][%s] normalized file not detected by name: %s\n", label.c_str(), bin_norm.c_str());
  if (policy_got != canonical_policy)
    log_fatal("[test][%s] policy parsed from name != requested policy (got %s, want %s) file=%s\n",
              label.c_str(), policy_got.c_str(), canonical_policy.c_str(), bin_norm.c_str());

  auto recs_norm = read_bin_all<BinaryT>(bin_norm);

  if (recs_norm.size() != recs_no_norm.size())
    log_fatal("[test][%s] normalized BIN changed record count (same lattice expected)\n",
              label.c_str());

  auto expected_norm = simulate_log_returns_normalization<BinaryT>(sanitized_in_mem);
  if (expected_norm.size() != recs_norm.size())
    log_fatal("[test][%s] simulate size mismatch\n", label.c_str());

  std::size_t normalized_valid = 0, masked_missing_context = 0, invalid_passthrough = 0;
  for (size_t i=0;i<recs_norm.size();++i) {
    const BinaryT& s = sanitized_in_mem[i];
    const BinaryT& y = recs_norm[i];
    const BinaryT& e = expected_norm[i];

    if (!bytes_equal(y, e))
      log_fatal("[test][%s] norm byte mismatch @%zu\n", label.c_str(), i);

    if (s.is_valid()) {
      if (e.is_valid()) {
        ++normalized_valid;
      } else {
        ++masked_missing_context;
      }
    } else {
      if (!bytes_equal(s, y))
        log_fatal("[test][%s] invalid record modified @%zu\n", label.c_str(), i);
      ++invalid_passthrough;
    }
  }

  log_info("[test][%s] ✔ Strict log-return policy=%s, normalized_valid=%zu, masked_missing_context=%zu, invalid_passthrough=%zu\n",
           label.c_str(), canonical_policy.c_str(), normalized_valid, masked_missing_context, invalid_passthrough);

  // 3) Idempotency: rerun without force should skip and yield identical bytes
  std::string bin_norm2 = sanitize_csv_into_binary_file<T>(
      csv_path, normalization_policy, /*force*/false);
  auto recs_norm2 = read_bin_all<BinaryT>(bin_norm2);
  if (recs_norm2.size() != recs_norm.size())
    log_fatal("[test][%s] Up-to-date skip changed size\n", label.c_str());
  for (size_t i=0;i<recs_norm.size();++i)
    if (!bytes_equal(recs_norm2[i], recs_norm[i]))
      log_fatal("[test][%s] Up-to-date skip changed bytes @%zu\n", label.c_str(), i);

  std::string policy_got2;
  bool is_norm3 = is_bin_filename_normalized(bin_norm2, &policy_got2);
  if (!is_norm3) log_fatal("[test][%s] up-to-date normalized name not detected\n", label.c_str());
  if (policy_got2 != canonical_policy) {
    log_fatal("[test][%s] policy parse mismatch on re-run (got %s, want %s)\n",
              label.c_str(), policy_got2.c_str(), canonical_policy.c_str());
  }

  log_info("[test][%s] ✔ All checks passed.\n", label.c_str());
}

int main() {
  try {
    // --- Name detector smoke tests (no file I/O needed) ---
    {
      std::string policy = "sentinel";
      bool n = is_bin_filename_normalized("/tmp/foo.bin", &policy);
      assert(!n && policy == "sentinel");

      policy = "sentinel";
      n = is_bin_filename_normalized("/tmp/foo.norm.bin", &policy);   // invalid: missing policy token
      assert(!n && policy == "sentinel");

      policy = "sentinel";
      n = is_bin_filename_normalized("/tmp/foo.norm.unexpected.bin", &policy);
      assert(!n && policy == "sentinel");

      policy = "sentinel";
      n = is_bin_filename_normalized("/tmp/foo.cache.norm.log_returns.bin", &policy);
      assert(n && policy == "log_returns");
    }

    fs::path tmp = fs::path("/cuwacunu/.data/tests");
    fs::create_directories(tmp);

    const std::string policy = "log_returns";

    // --- kline_t ---
    auto kline_csv = write_kline_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::kline_t>("kline_t", kline_csv, policy);
    {
      std::string kline_norm =
          sanitize_csv_into_binary_file<cuwacunu::camahjucunu::exchange::kline_t>(
              kline_csv, /*normalization_policy=*/policy,
              /*force_rebuild_cache=*/false);
      auto recs = read_bin_all<cuwacunu::camahjucunu::exchange::kline_cache_t>(kline_norm);
      if (recs.size() < 5) {
        log_fatal("[test][kline_t] expected at least 5 normalized records for explicit trade-count check\n");
      }
      if (recs[0].is_valid() ||
          !recs[1].is_valid() ||
          recs[4].is_valid() ||
          std::fabs(recs[1].number_of_trades - std::log1p(12.0)) > 1e-12) {
        log_fatal("[test][kline_t] strict context or number_of_trades normalization is incorrect under log_returns\n");
      }
      log_info("[test][kline_t] ✔ first-segment masking and number_of_trades log1p are correct.\n");
    }

    // --- kline_t bootstrap min-step regression (first delta outlier) ---
    {
      auto kline_outlier_csv = write_kline_bootstrap_outlier_csv(tmp);
      std::string out_bin = sanitize_csv_into_binary_file<cuwacunu::camahjucunu::exchange::kline_t>(
          kline_outlier_csv, /*normalization_policy=*/"none", /*force_rebuild_cache=*/true);
      auto recs = read_bin_all<cuwacunu::camahjucunu::exchange::kline_cache_t>(out_bin);
      std::vector<int64_t> got;
      got.reserve(recs.size());
      for (const auto& r : recs) got.push_back(r.close_time);
      const std::vector<int64_t> expected = {60, 120, 180, 240, 300, 360};
      if (got != expected) {
        log_fatal("[test][kline_t] bootstrap min-step regression failed: expected 60-grid expansion\n");
      }
      log_info("[test][kline_t] ✔ Bootstrap min-step inferred expected lattice under first-delta outlier.\n");
    }

    // --- kline_t gap continuation strict-context regression ---
    {
      auto kline_gap_csv = write_kline_bootstrap_outlier_csv(tmp);
      std::string gap_norm = sanitize_csv_into_binary_file<cuwacunu::camahjucunu::exchange::kline_t>(
          kline_gap_csv, /*normalization_policy=*/policy, /*force_rebuild_cache=*/true);
      auto recs = read_bin_all<cuwacunu::camahjucunu::exchange::kline_cache_t>(gap_norm);
      if (recs.size() != 6) {
        log_fatal("[test][kline_t] strict gap regression expected 6 lattice rows, got %zu\n", recs.size());
      }
      if (recs[0].is_valid() || recs[3].is_valid() || !recs[4].is_valid() || !recs[5].is_valid()) {
        log_fatal("[test][kline_t] strict gap masking/continuation behavior is incorrect\n");
      }
      if (std::fabs(recs[4].close_price - std::log(104.0 / 103.0)) > 1e-12 ||
          std::fabs(recs[5].close_price - std::log(105.0 / 104.0)) > 1e-12) {
        log_fatal("[test][kline_t] post-gap normalization did not reuse the masked raw predecessor as context\n");
      }
      log_info("[test][kline_t] ✔ Gap masking and post-gap context recovery are correct.\n");
    }

    // --- trade_t ---
    auto trade_csv = write_trade_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::trade_t>("trade_t", trade_csv, policy);

    // --- basic_t ---
    auto basic_csv = write_basic_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::basic_t>("basic_t", basic_csv, policy);

    // --- numeric-domain clamp behavior ---
    {
      using Kline = cuwacunu::camahjucunu::exchange::kline_cache_t;
      Kline prev{};
      prev.open_time = 59;
      prev.open_price = 99.0;
      prev.high_price = 101.0;
      prev.low_price = 98.0;
      prev.close_price = 100.0;
      prev.volume = 5.0;
      prev.close_time = 60;
      prev.quote_asset_volume = 10.0;
      prev.number_of_trades = 3.0;
      prev.taker_buy_base_volume = 2.0;
      prev.taker_buy_quote_volume = 4.0;

      Kline cur = prev;
      cur.open_time = 119;
      cur.close_time = 120;
      cur.open_price = 0.0;
      cur.high_price = -5.0;
      cur.low_price = std::numeric_limits<double>::infinity();
      cur.close_price = std::numeric_limits<double>::quiet_NaN();
      cur.volume = -0.5;
      cur.quote_asset_volume = -1.0;
      cur.number_of_trades = -5.0;
      cur.taker_buy_base_volume = std::numeric_limits<double>::quiet_NaN();
      cur.taker_buy_quote_volume = 0.0;

      const Kline normalized = Kline::normalize_log_returns(cur, &prev);
      const double kFloor = 1e-12;
      const double kLog1pFloor = std::log1p(-1.0 + kFloor);

      if (!std::isfinite(normalized.open_price) ||
          !std::isfinite(normalized.high_price) ||
          !std::isfinite(normalized.low_price) ||
          !std::isfinite(normalized.close_price) ||
          std::fabs(normalized.volume - std::log1p(-0.5)) > 1e-12 ||
          std::fabs(normalized.quote_asset_volume - kLog1pFloor) > 1e-12 ||
          std::fabs(normalized.number_of_trades - kLog1pFloor) > 1e-12 ||
          std::fabs(normalized.taker_buy_base_volume - kLog1pFloor) > 1e-12 ||
          std::fabs(normalized.taker_buy_quote_volume) > 1e-12) {
        log_fatal("[test][numeric-domain] clamp policy produced unexpected outputs\n");
      }
      if (std::fabs(normalized.open_price - std::log(kFloor / prev.close_price)) > 1e-12 ||
          std::fabs(normalized.high_price - std::log(kFloor / prev.close_price)) > 1e-12 ||
          std::fabs(normalized.low_price - std::log(kFloor / prev.close_price)) > 1e-12 ||
          std::fabs(normalized.close_price - std::log(kFloor / prev.close_price)) > 1e-12) {
        log_fatal("[test][numeric-domain] log-return clamp outputs do not match the configured floor\n");
      }
      log_info("[test][numeric-domain] ✔ Out-of-domain inputs clamp to finite values.\n");
    }

    log_info("\n[test] ✅ All test cases succeeded.\n");
    return 0;
  } catch (const std::exception& e) {
    log_err("[test] Exception: %s\n", e.what());
    return 2;
  }
}
