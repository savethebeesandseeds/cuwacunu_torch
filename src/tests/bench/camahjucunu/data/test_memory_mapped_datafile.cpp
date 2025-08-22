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

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/data/memory_mapped_datafile.h"

namespace fs = std::filesystem;
using cuwacunu::camahjucunu::data::sanitize_csv_into_binary_file;


// ---------- Small helpers ----------------------------------------------------
template <typename T>
inline typename T::key_type_t key_of(const T& x) {
  return const_cast<T&>(x).key_value();
}

template <typename T>
static bool bytes_equal(const T& a, const T& b) {
  static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}

template <typename T>
static std::vector<T> read_bin_all(const std::string& bin_path) {
  std::vector<T> out;
  std::ifstream in(bin_path, std::ios::binary);
  if (!in.is_open()) {
    log_fatal("[test] Could not open BIN for reading: %s\n", bin_path.c_str());
  }
  in.seekg(0, std::ios::end);
  const std::streamsize sz = in.tellg();
  in.seekg(0, std::ios::beg);
  if (sz % static_cast<std::streamsize>(sizeof(T)) != 0) {
    log_fatal("[test] BIN size not multiple of sizeof(T) for %s\n", bin_path.c_str());
  }
  const std::size_t n = static_cast<std::size_t>(sz / sizeof(T));
  out.resize(n);
  if (n) {
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(n * sizeof(T)));
    if (!in) log_fatal("[test] Failed reading BIN payload: %s\n", bin_path.c_str());
  }
  return out;
}

// Build the expected sanitized (pre-normalized) sequence directly from CSV,
// matching the writer’s logic (duplicates skipped, gaps filled with nulls).
template <typename T>
static std::vector<T> sanitize_in_memory(const std::string& csv_path, char delimiter=',') {
  std::ifstream csv = cuwacunu::piaabo::dfiles::readFileToStream(csv_path);
  if (!csv.is_open()) log_fatal("[test] Could not open CSV: %s\n", csv_path.c_str());

  std::string line_p0, line_p1;
  std::size_t line_no = 0;
  if (!std::getline(csv, line_p0)) {
    log_fatal("[test] CSV too short: %s\n", csv_path.c_str());
  }
  T p0 = T::from_csv(line_p0, delimiter, line_no);

  std::vector<T> out; out.reserve(1024);

  bool have_regular_delta = false;
  long double regular_delta = 0.0L;
  const long double tol = 1e-8L;

  auto push_buffered = [&](const T& rec) { out.push_back(rec); };

  while (std::getline(csv, line_p1)) {
    T p1 = T::from_csv(line_p1, delimiter, ++line_no);
    if (!p1.is_valid()) { line_p0 = line_p1; p0 = p1; continue; }

    long double kv0 = (long double)p0.key_value();
    long double kv1 = (long double)p1.key_value();
    long double d   = kv1 - kv0;

    if (fabs(d) <= tol) {
      // duplicate — writer skips emit; anchor advances
      line_p0 = line_p1; p0 = p1;
      continue;
    }
    if (d < 0.0L) {
      log_fatal("[test] Decreasing key in CSV %s\n", csv_path.c_str());
    }
    if (!have_regular_delta) { regular_delta = d; have_regular_delta = true; }

    // steps
    std::int64_t steps = (std::int64_t) llround(d / regular_delta);
    // emit p0 and gaps as nulls (not p1; p1 becomes next anchor)
    for (std::int64_t i = 0; i < steps; ++i) {
      const bool first = (i == 0);
      T px = first ? p0
                   : T::null_instance(static_cast<typename T::key_type_t>(
                         kv0 + (long double)i * regular_delta));
      push_buffered(px);
    }

    line_p0 = line_p1; p0 = p1;
  }
  // final anchor
  out.push_back(p0);
  return out;
}

// Verify lattice & nulls against CSV keys list (for quick correctness)
template <typename T>
static void verify_lattice_and_nulls(const std::vector<T>& recs,
                                     const std::vector<typename T::key_type_t>& csv_keys,
                                     const std::string& label) {
  const long double tol = 1e-8L;

  // Determine regular delta from first positive CSV gap
  bool have_reg = false; long double reg = 0.0L;
  for (size_t i=0; i+1<csv_keys.size(); ++i) {
    long double d = (long double)csv_keys[i+1] - (long double)csv_keys[i];
    if (d > tol) { reg = d; have_reg=true; break; }
  }
  if (!have_reg) log_fatal("[test][%s] Cannot infer regular delta.\n", label.c_str());

  // Build expected key lattice
  std::vector<typename T::key_type_t> expected;
  long double cur = (long double)csv_keys.front();
  expected.push_back(static_cast<typename T::key_type_t>(cur));
  for (size_t i=0; i+1<csv_keys.size(); ++i) {
    long double k0=(long double)csv_keys[i], k1=(long double)csv_keys[i+1], d=k1-k0;
    if (fabs(d)<=tol) continue;
    std::int64_t steps = (std::int64_t) llround(d / reg);
    for (std::int64_t s=1; s<steps; ++s)
      expected.push_back(static_cast<typename T::key_type_t>(k0 + s*reg));
    expected.push_back(static_cast<typename T::key_type_t>(k1));
  }

  if (recs.size() != expected.size())
    log_fatal("[test][%s] Count mismatch: got %zu exp %zu\n",
              label.c_str(), recs.size(), expected.size());

  for (size_t i=0;i<recs.size();++i) {
    auto got = key_of(recs[i]);
    auto exp = expected[i];
    if (got != exp)
      log_fatal("[test][%s] Key mismatch @%zu got=%Lf exp=%Lf\n",
                label.c_str(), i, (long double)got, (long double)exp);
    // Non-CSV keys must be null_instance
    bool is_csv = false;
    for (auto k: csv_keys) if (k==exp) { is_csv=true; break; }
    if (!is_csv) {
      T null_rec = T::null_instance(exp);
      if (!bytes_equal(recs[i], null_rec))
        log_fatal("[test][%s] Gap @%zu not null_instance\n", label.c_str(), i);
    }
  }
  log_info("[test][%s] ✔ Lattice & nulls OK (%zu)\n",
           label.c_str(), recs.size());
}

// Compare the normalized BIN against expected causal normalization computed
// from the sanitized (pre-normalized) sequence using stats_pack.
template <typename T>
static void verify_causal_normalization(const std::vector<T>& sanitized_seq,
                                        const std::vector<T>& normalized_bin,
                                        std::size_t window_size,
                                        const std::string& label) {
  if (sanitized_seq.size() != normalized_bin.size())
    log_fatal("[test][%s] Size mismatch for normalization check.\n", label.c_str());

  // Initialize the same statistics pack that normalize_binary_file uses.
  auto pack = T::initialize_statistics_pack((unsigned)window_size);
  const auto& accessors = pack.accessors; // public in your definition

  // Warm-up: update with the first window_size records
  // (normalize pass in file also warms up before writing)
  if (sanitized_seq.size() < window_size)
    log_fatal("[test][%s] Sequence shorter than window size.\n", label.c_str());

  for (std::size_t i=0; i<window_size; ++i) {
    pack.update(sanitized_seq[i]);
  }

  // Now walk from 0..N-1, but emulate the file’s behavior:
  // - For each i: normalize with CURRENT pack, write, then update with rec[i].
  // Note: That means for i < window_size, file also normalized (it warmed first).
  // We already warmed pack with the first window_size items; to align with file,
  // we reset and do a second pass exactly like the file does.

  // Reset pack and align precisely:
  auto pack2 = T::initialize_statistics_pack((unsigned)window_size);
  // Warm-up (file did this without writing)
  for (std::size_t i=0; i<window_size; ++i) pack2.update(sanitized_seq[i]);

  const double atol = 1e-9;
  const double rtol = 1e-9;

  for (std::size_t i=0; i<sanitized_seq.size(); ++i) {
    const T& x  = sanitized_seq[i];
    const T& y  = normalized_bin[i];

    if (!x.is_valid()) {
      // Null records should be untouched
      if (!bytes_equal(x, y))
        log_fatal("[test][%s] Null record modified at idx %zu\n", label.c_str(), i);
      continue;
    }

    // Expected normalized (from CURRENT stats), then update
    T exp_norm = pack2.normalize(x);

    // Compare only normalized fields (via accessors)
    for (std::size_t f=0; f<accessors.size(); ++f) {
      double got = accessors[f].getter(y);
      double exp = accessors[f].getter(exp_norm);
      double diff = std::fabs(got - exp);
      double tol = atol + rtol * std::max(std::fabs(got), std::fabs(exp));
      if (diff > tol) {
        log_fatal("[test][%s] Norm mismatch @idx %zu, field %zu: got=%.15f exp=%.15f (diff=%.3g, tol=%.3g)\n",
                  label.c_str(), i, f, got, exp, diff, tol);
      }
    }

    // Now advance rolling stats with the ORIGINAL x (as file does)
    pack2.update(x);
  }

  log_info("[test][%s] ✔ Causal normalization matches (%zu)\n",
           label.c_str(), sanitized_seq.size());
}

// ---------- CSV Writers ------------------------------------------------------

static std::string write_trade_csv(const fs::path& dir) {
  // id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch
  std::string path = (dir / "trades.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  os << "1,100.0,0.5,50.0,1000,0,1\n";
  os << "2,101.0,0.4,40.4,1001,1,1\n"; // +1
  os << "3,101.5,0.6,60.9,1001,0,1\n"; // duplicate (skip)
  os << "4,102.0,0.7,71.4,1004,0,0\n"; // +3 -> expect nulls at 1002,1003
  return path;
}

static std::string write_kline_csv(const fs::path& dir) {
  // open_time,open,high,low,close,vol,close_time,quote_vol,ntr,tb_base,tb_quote
  std::string path = (dir / "klines.csv").string();
  std::ofstream os(path);
  if (!os.is_open()) log_fatal("[test] Could not open %s\n", path.c_str());
  auto emit = [&](int64_t ot,double o,double h,double l,double c,double v,
                  int64_t ct,double qv,int32_t n,double tb,double tq){
    os<<ot<<','<<o<<','<<h<<','<<l<<','<<c<<','<<v<<','<<ct<<','<<qv<<','<<n<<','<<tb<<','<<tq<<','<<'0'<<'\n';
  };
  emit(0,100,105, 99,102,1000,  60,102000,10,400,40800);
  emit(0,102,106,101,103,1100, 120,123000,12,420,43200); // +60
  emit(0,103,107,102,104,1200, 120,125000,12,425,44000); // duplicate
  emit(0,104,108,103,105,1300, 300,160000,15,450,48000); // +180 -> 2 nulls
  return path;
}

static std::string write_basic_csv(const fs::path& dir) {
  // time,value  (delta=0.5; use two-step gap)
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
                     const std::vector<typename T::key_type_t>& csv_keys_for_expect,
                     std::size_t norm_window) {
  log_info("\n[test][%s] CSV: %s\n", label.c_str(), csv_path.c_str());

  // 1) Sanitize without normalization (to test lattice/nulls)
  std::string bin_no_norm = sanitize_csv_into_binary_file<T>(csv_path, /*norm_window=*/0,
                                                             /*force_binarization=*/true,
                                                             /*buffer_size=*/4, /*delimiter=*/',');
  auto recs_no_norm = read_bin_all<T>(bin_no_norm);
  verify_lattice_and_nulls<T>(recs_no_norm, csv_keys_for_expect, label + " (no-norm)");

  // 2) Sanitize WITH normalization
  std::string bin_norm = sanitize_csv_into_binary_file<T>(csv_path, /*norm_window=*/norm_window,
                                                          /*force_binarization=*/true);
  auto recs_norm = read_bin_all<T>(bin_norm);

  // 3) Build the pre-normalized sanitized sequence in-memory (must match writer logic)
  auto sanitized_seq = sanitize_in_memory<T>(csv_path, ',');

  if (sanitized_seq.size() != recs_norm.size())
    log_fatal("[test][%s] Size mismatch between sanitized_seq and normalized BIN.\n", label.c_str());

  // 4) Verify causal normalization field-wise using your statistics_pack_t<T>
  verify_causal_normalization<T>(sanitized_seq, recs_norm, norm_window, label);

  // 5) Idempotency: rerun without force should skip; sizes must match
  std::string bin_norm2 = sanitize_csv_into_binary_file<T>(csv_path, norm_window, /*force*/false);
  auto recs_norm2 = read_bin_all<T>(bin_norm2);
  if (recs_norm2.size() != recs_norm.size())
    log_fatal("[test][%s] Up-to-date skip produced different size.\n", label.c_str());

  log_info("[test][%s] ✔ All checks passed.\n", label.c_str());
}

int main() {
  try {
    fs::path tmp = fs::path("/cuwacunu/data/tests");
    fs::create_directories(tmp);

    // Window used for normalization tests
    const std::size_t W = 3;

    // --- trade_t ---
    // auto trade_csv = write_trade_csv(tmp);
    // run_case<cuwacunu::camahjucunu::exchange::trade_t>("trade_t", trade_csv,
    //   std::vector<cuwacunu::camahjucunu::exchange::trade_t::key_type_t>{ 1000, 1001, 1001, 1004 }, W);

    // --- kline_t ---
    auto kline_csv = write_kline_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::kline_t>("kline_t", kline_csv,
      std::vector<cuwacunu::camahjucunu::exchange::kline_t::key_type_t>{ 60, 120, 120, 300 }, W);

    // --- basic_t ---
    auto basic_csv = write_basic_csv(tmp);
    run_case<cuwacunu::camahjucunu::exchange::basic_t>("basic_t", basic_csv,
      std::vector<cuwacunu::camahjucunu::exchange::basic_t::key_type_t>{ 0.0, 0.5, 0.5, 2.0 }, W);

    log_info("\n[test] ✅ All test cases succeeded.\n");
    return 0;
  } catch (const std::exception& e) {
    log_err("[test] Exception: %s\n", e.what());
    return 2;
  }
}
