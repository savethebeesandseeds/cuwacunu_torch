/* memory_mapped_datafile.h */
#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <limits>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <cerrno>
#include <algorithm>
#include <optional>
#include <cctype>   // std::isdigit

#ifdef __unix__
  #include <sys/stat.h>
#endif

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

namespace detail {

constexpr std::int64_t kMaxGapFillSteps = 10000000;

// Cast a long double key to T::key_type_t with correct rounding for integrals.
template <typename KeyT>
inline KeyT cast_key_longdouble(long double v) {
  if constexpr (std::is_integral<KeyT>::value) {
    return static_cast<KeyT>(std::llround(v));
  } else {
    return static_cast<KeyT>(v);
  }
}

// Update progress bar sparingly to reduce overhead.
inline bool should_tick_progress(std::size_t i) {
  // every ~1024 iterations (tunable)
  return (i & 0x3FFu) == 0u;
}

inline std::int64_t rounded_steps_or_fatal(long double steps_ld,
                                           std::size_t line_number,
                                           const std::string& csv_filename,
                                           const char* context_label) {
  if (!std::isfinite(steps_ld)) {
    log_fatal("[sanitize_csv_into_binary_file] Non-finite step ratio in %s at line %zu (%s)\n",
              csv_filename.c_str(), line_number, context_label);
  }
  const long double abs_steps = std::fabs(steps_ld);
  const long double i64_max_ld = static_cast<long double>(std::numeric_limits<std::int64_t>::max());
  if (abs_steps > i64_max_ld) {
    log_fatal("[sanitize_csv_into_binary_file] Step ratio overflow in %s at line %zu (%s): %.15Lf\n",
              csv_filename.c_str(), line_number, context_label, steps_ld);
  }

  const auto rounded = static_cast<std::int64_t>(std::llround(steps_ld));
  if (rounded <= 0) {
    log_fatal("[sanitize_csv_into_binary_file] Non-positive rounded steps in %s at line %zu (%s): %lld\n",
              csv_filename.c_str(), line_number, context_label, static_cast<long long>(rounded));
  }
  if (rounded > kMaxGapFillSteps) {
    log_fatal("[sanitize_csv_into_binary_file] Gap fill too large in %s at line %zu (%s): %lld > %lld\n",
              csv_filename.c_str(), line_number, context_label,
              static_cast<long long>(rounded),
              static_cast<long long>(kMaxGapFillSteps));
  }
  return rounded;
}

inline long double effective_tolerance(long double a,
                                       long double b,
                                       long double abs_tol,
                                       long double rel_tol) {
  const long double scale =
      std::max(1.0L, std::max(std::fabs(a), std::fabs(b)));
  return abs_tol + rel_tol * scale;
}

inline bool is_near_with_tolerance(long double a,
                                   long double b,
                                   long double abs_tol,
                                   long double rel_tol) {
  return std::fabs(a - b) <= effective_tolerance(a, b, abs_tol, rel_tol);
}

struct csv_step_policy_t {
  std::size_t bootstrap_deltas{64};
  long double abs_tol{1e-8L};
  long double rel_tol{1e-10L};
};

inline csv_step_policy_t load_csv_step_policy() {
  csv_step_policy_t out{};
  const int configured_bootstrap = cuwacunu::iitepi::config_space_t::get<int>(
      "DATA_LOADER", "dataloader_csv_bootstrap_deltas", std::optional<int>{64});
  const double configured_abs_tol = cuwacunu::iitepi::config_space_t::get<double>(
      "DATA_LOADER", "dataloader_csv_step_abs_tol", std::optional<double>{1e-8});
  const double configured_rel_tol = cuwacunu::iitepi::config_space_t::get<double>(
      "DATA_LOADER", "dataloader_csv_step_rel_tol", std::optional<double>{1e-10});

  out.bootstrap_deltas = static_cast<std::size_t>(std::max(2, configured_bootstrap));
  out.abs_tol = static_cast<long double>(
      (configured_abs_tol > 0.0) ? configured_abs_tol : 1e-8);
  out.rel_tol = static_cast<long double>(
      (configured_rel_tol >= 0.0) ? configured_rel_tol : 1e-10);
  return out;
}

template <typename T>
long double infer_regular_delta_from_csv(const std::string& csv_filename,
                                         char delimiter,
                                         const csv_step_policy_t& policy) {
  std::ifstream csv_file = cuwacunu::piaabo::dfiles::readFileToStream(csv_filename);
  if (!csv_file.is_open()) {
    log_fatal("[sanitize_csv_into_binary_file] Could not open CSV for step inference: %s\n",
              csv_filename.c_str());
  }

  std::vector<long double> positive_deltas;
  positive_deltas.reserve(policy.bootstrap_deltas);

  bool have_anchor = false;
  T anchor{};
  std::size_t line_number = 0;
  std::string line;
  while (std::getline(csv_file, line)) {
    ++line_number;
    T curr = T::from_csv(line, delimiter, line_number);
    if (!curr.is_valid()) continue;

    if (!have_anchor) {
      anchor = curr;
      have_anchor = true;
      continue;
    }

    const long double kv0 = static_cast<long double>(anchor.key_value());
    const long double kv1 = static_cast<long double>(curr.key_value());
    const long double delta = kv1 - kv0;

    if (delta < 0.0L) {
      log_fatal("[sanitize_csv_into_binary_file] key_value must be non-decreasing during step inference."
                " At line %s%zu%s in %s%s%s\n",
                ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
    }

    if (!is_near_with_tolerance(delta, 0.0L, policy.abs_tol, policy.rel_tol)) {
      positive_deltas.push_back(delta);
      if (positive_deltas.size() >= policy.bootstrap_deltas) break;
    }
    anchor = curr;
  }
  csv_file.close();

  if (!have_anchor) {
    log_fatal("[sanitize_csv_into_binary_file] No valid records found while inferring step on: %s\n",
              csv_filename.c_str());
  }
  if (positive_deltas.empty()) {
    log_fatal("[sanitize_csv_into_binary_file] Could not infer positive key delta from CSV: %s\n",
              csv_filename.c_str());
  }

  const long double regular_delta =
      *std::min_element(positive_deltas.begin(), positive_deltas.end());
  if (!std::isfinite(regular_delta) || regular_delta <= 0.0L) {
    log_fatal("[sanitize_csv_into_binary_file] Invalid inferred regular delta for %s: %.15Lf\n",
              csv_filename.c_str(), regular_delta);
  }
  return regular_delta;
}

// ----------------------------------------------------------------------------
// Filenaming helpers for normalized binaries
//   - Raw:   <stem>.bin
//   - Norm:  <stem>.normW<window>.bin    (with window >= 1)
// ----------------------------------------------------------------------------
inline std::string raw_bin_for_csv(const std::string& csv_filename) {
  std::filesystem::path p(csv_filename);
  p.replace_extension(".bin");
  return p.string();
}

inline std::string norm_bin_for_csv(const std::string& csv_filename, std::size_t window) {
  // Precondition: window >= 1 (callers must enforce)
  std::filesystem::path p(csv_filename);
  const auto parent = p.parent_path();
  const auto stem   = p.stem().string();
  std::string fname = stem + ".normW" + std::to_string(window) + ".bin";
  std::filesystem::path out = parent / fname;
  return out.string();
}

// Detector: file name must be .../something.normW<digits>.bin with digits >= 1
inline bool is_bin_filename_normalized_strict(const std::string& bin_filename, std::size_t* window_out = nullptr) {
  // IMPORTANT: do not touch *window_out unless the name is recognized as normalized.
  std::filesystem::path p(bin_filename);
  if (p.extension() != ".bin") return false;

  const std::string fname = p.filename().string();
  const auto pos_norm = fname.rfind(".normW");
  if (pos_norm == std::string::npos) return false;  // not a normalized name

  // parse digits after 'W' up to ".bin"
  const auto wpos = pos_norm + 5; // position of 'W' in ".normW"
  const auto end  = fname.rfind(".bin");
  if (end == std::string::npos || wpos + 1 >= end) return false;

  const std::string wstr = fname.substr(wpos + 1, end - (wpos + 1));
  const bool digits = !wstr.empty() &&
                      std::all_of(wstr.begin(), wstr.end(), [](unsigned char c){ return std::isdigit(c); });
  if (!digits) return false;

  std::size_t win = 0;
  try { win = static_cast<std::size_t>(std::stoull(wstr)); } catch (...) { return false; }
  if (win < 1) return false;

  if (window_out) *window_out = win;
  return true;
}

} // namespace detail

// Public shim so other headers/tests can use it without reaching into detail::
inline bool is_bin_filename_normalized(const std::string& bin_filename, std::size_t* window_out = nullptr) {
  return detail::is_bin_filename_normalized_strict(bin_filename, window_out);
}

/*
 * normalize_binary_file<T>
 * ------------------------
 * In-place normalization of a binary file of records T using a rolling window
 * built from the **previous** up-to-window_size valid records.
 *
 * At the beginning of the sequence (burn-in), when fewer than window_size
 * valid samples are available, normalization still runs against the partial
 * history. This keeps the file in a single normalized scale from the start
 * (instead of mixing raw-prefix + normalized-tail).
 *
 * Requirements on T:
 *  - POD / trivially copyable
 *  - static auto initialize_statistics_pack(std::size_t window);
 *  - bool is_valid() const;
 *  - T normalize(const T& rec) const;      // uses stats_pack
 *  - void stats_pack.update(const T& rec); // sliding window update (internal pop if needed)
 */
template <typename T>
void normalize_binary_file(const std::string& bin_filename,
                           std::size_t window_size = std::numeric_limits<std::size_t>::max()) {
  static_assert(std::is_trivially_copyable<T>::value,
                "normalize_binary_file<T>: T must be trivially copyable (POD-like).");

  log_info("[normalize_binary_file] policy=causal_partial_window_keep_len, W=%zu. File: %s%s%s\n",
           window_size, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);

  // Determine file size via filesystem (robust even if tellg would fail).
  std::error_code ec;
  const auto file_size_u = std::filesystem::file_size(bin_filename, ec);
  if (ec) {
    log_fatal("[normalize_binary_file] file_size() failed for: %s%s%s (ec=%d)\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET, (int)ec.value());
  }

  if (file_size_u % sizeof(T) != 0) {
    log_fatal("[normalize_binary_file] File size not multiple of sizeof(T): %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  const std::size_t total_records = static_cast<std::size_t>(file_size_u / sizeof(T));
  if (total_records == 0) {
    log_info("[normalize_binary_file] Empty file, nothing to do.\n");
    return;
  }

  if (window_size == std::numeric_limits<std::size_t>::max() || window_size > total_records) {
    window_size = total_records;
  }

  std::fstream io(bin_filename, std::ios::in | std::ios::out | std::ios::binary);
  if (!io.is_open()) {
    log_fatal("[normalize_binary_file] Could not open: %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  auto stats_pack = T::initialize_statistics_pack(window_size);

  // We normalize record i using stats from previous valid records
  // (count <= window_size). During burn-in, this is a partial history.
  std::size_t filled_valid = 0;         // count of valid samples seen (caps at window_size)
  std::size_t normalized_count = 0;     // diagnostics: valid records normalized
  std::size_t invalid_count = 0;        // diagnostics: invalid passthrough

  io.clear();
  io.seekg(0, std::ios::beg);
  io.seekp(0, std::ios::beg);

  START_LOADING_BAR(normalization_progress_bar_, 60, "Normalize binary file");

  for (std::size_t i = 0; i < total_records; ++i) {
    // Read original record
    T rec{};
    io.read(reinterpret_cast<char*>(&rec), sizeof(T));
    if (!io) {
      log_fatal("[normalize_binary_file] Read failed at record %zu: %s%s%s\n",
                i, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
    }

    // Decide output
    T out = rec;
    if (rec.is_valid()) {
      // Normalize from the first valid sample using whatever past context exists.
      // With zero/low context, per-field stddev can be zero and normalize() returns 0,
      // yielding a neutral burn-in instead of leaking raw-scale values.
      out = stats_pack.normalize(rec);
      ++normalized_count;
    } else {
      ++invalid_count; // passthrough invalids, do not update stats
    }

    // Write back in place at position i
    const auto write_pos = static_cast<std::streamoff>(i) * static_cast<std::streamoff>(sizeof(T));
    io.seekp(write_pos, std::ios::beg);
    io.write(reinterpret_cast<const char*>(&out), sizeof(T));
    if (!io) {
      log_fatal("[normalize_binary_file] Write failed at record %zu: %s%s%s\n",
                i, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
    }

    // Update stats with the original (un-normalized) record if valid
    if (rec.is_valid()) {
      stats_pack.update(rec);
      if (filled_valid < window_size) ++filled_valid;
    }

    // Standard streams require a seek between output->input sequences.
    const auto next_read_pos = static_cast<std::streamoff>(i + 1) * static_cast<std::streamoff>(sizeof(T));
    io.seekg(next_read_pos, std::ios::beg);

    // Progress
    if (detail::should_tick_progress(i) || i + 1 == total_records) {
      double pct = (static_cast<double>(i + 1) / static_cast<double>(total_records)) * 100.0;
      pct = std::round(pct * 100.0) / 100.0;
      UPDATE_LOADING_BAR(normalization_progress_bar_, pct);
    }
  }

  FINISH_LOADING_BAR(normalization_progress_bar_);
  io.close();

  const std::size_t partial_window_valid = std::min(filled_valid, window_size);
  log_info("(normalize_binary_file) %sNormalization completed%s. File: %s%s%s | "
           "partial_window_valid=%zu, normalized_valid=%zu, invalid_passthrough=%zu\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
           ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET,
           partial_window_valid, normalized_count, invalid_count);
}

/*
 * sanitize_csv_into_binary_file<T>
 * --------------------------------
 * Validates monotone key, fills gaps with null instances at regular increments,
 * writes raw .bin, and optionally produces a separate normalized .normW<window>.bin.
 *
 * Policy:
 *  - normalization_window == 0  -> NO normalized file. Return RAW .bin.
 *  - normalization_window >= 1  -> Produce <stem>.normW<window>.bin (copy raw, normalize in place). Return that path.
 *
 * Requirements on T:
 *  - static T from_csv(const std::string&, char, std::size_t line_no);
 *  - bool is_valid() const;
 *  - using key_type_t = ...;             // numeric
 *  - key_type_t key_value() const;
 *  - static T null_instance(key_type_t); // make a "gap filler"
 */
template<typename T>
std::string sanitize_csv_into_binary_file(const std::string& csv_filename,
                                          std::size_t normalization_window = 0,
                                          bool        force_rebuild_cache   = false,
                                          std::size_t buffer_size           = 1024,
                                          char        delimiter             = ',') {
  static_assert(std::is_trivially_copyable<T>::value,
                "sanitize_csv_into_binary_file<T>: T must be trivially copyable (POD-like).");

  log_info("[sanitize_csv_into_binary_file]\t %sPreparing binary%s from CSV: %s\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, csv_filename.c_str());

  if (buffer_size < 1) {
    log_fatal("[sanitize_csv_into_binary_file] buffer_size must be >= 1 for file: %s\n",
              csv_filename.c_str());
  }

  const std::size_t total_records_hint =
      cuwacunu::piaabo::dfiles::countLinesInFile(csv_filename);

  // Resolve output names
  const std::string raw_bin  = detail::raw_bin_for_csv(csv_filename);
  const bool want_norm       = (normalization_window >= 1);
  const std::string norm_bin = want_norm ? detail::norm_bin_for_csv(csv_filename, normalization_window)
                                         : std::string{};

  // If not forced, decide if target output is already up-to-date.
  auto newer_than = [](const std::string& a, const std::string& b) -> bool {
    std::error_code ec1, ec2;
    if (!std::filesystem::exists(a, ec1) || !std::filesystem::exists(b, ec2)) return false;
    return std::filesystem::last_write_time(a, ec1) > std::filesystem::last_write_time(b, ec2);
  };

  // We will *always* ensure the raw .bin is up-to-date w.r.t. the CSV,
  // because normalized files are derived from it.
  bool need_write_raw = force_rebuild_cache || !newer_than(raw_bin, csv_filename);

  if (need_write_raw) {
    const detail::csv_step_policy_t csv_step_policy =
        detail::load_csv_step_policy();
    const long double regular_delta = detail::infer_regular_delta_from_csv<T>(
        csv_filename, delimiter, csv_step_policy);
    log_info("[sanitize_csv_into_binary_file] inferred regular_delta=%.15Lf "
             "(bootstrap_deltas=%zu, abs_tol=%.3Le, rel_tol=%.3Le)\n",
             regular_delta,
             csv_step_policy.bootstrap_deltas,
             csv_step_policy.abs_tol,
             csv_step_policy.rel_tol);

    std::ifstream csv_file = cuwacunu::piaabo::dfiles::readFileToStream(csv_filename);
    if (!csv_file.is_open()) {
      log_fatal("[sanitize_csv_into_binary_file] Could not open CSV: %s\n", csv_filename.c_str());
    }

    std::ofstream bin_file(raw_bin, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!bin_file.is_open()) {
      csv_file.close();
      log_fatal("[sanitize_csv_into_binary_file] Could not open BIN for write: %s\n",
                raw_bin.c_str());
    }
#ifdef __unix__
    chmod(raw_bin.c_str(), S_IRUSR | S_IWUSR);
#endif

    std::vector<T> buffer;
    buffer.reserve(buffer_size);

    auto flush_buffer = [&]() {
      if (buffer.empty()) return;
      bin_file.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<std::streamsize>(buffer.size() * sizeof(T)));
      if (!bin_file) {
        log_fatal("[sanitize_csv_into_binary_file] Buffered write failed.\n");
      }
      buffer.clear();
    };

    START_LOADING_BAR(csv_file_preparation_progress_bar_, 60, "Preparing Binary data file");

    std::size_t processed_lines = 0;
    std::size_t line_number = 0;
    bool have_anchor = false;
    T obj_p0{};

    std::string line_p1;
    while (std::getline(csv_file, line_p1)) {
      ++processed_lines;
      ++line_number;

      if (detail::should_tick_progress(processed_lines) || csv_file.peek() == EOF) {
        double pct = (static_cast<double>(processed_lines) /
                      static_cast<double>(std::max<std::size_t>(1, total_records_hint))) * 100.0;
        pct = std::round(pct * 100.0) / 100.0;
        UPDATE_LOADING_BAR(csv_file_preparation_progress_bar_, pct);
      }

      T obj_p1 = T::from_csv(line_p1, delimiter, line_number);
      if (!obj_p1.is_valid()) continue;

      if (!have_anchor) {
        obj_p0 = obj_p1;
        have_anchor = true;
        continue;
      }

      const long double kv0 = static_cast<long double>(obj_p0.key_value());
      const long double kv1 = static_cast<long double>(obj_p1.key_value());
      const long double current_delta = kv1 - kv0;

      if (detail::is_near_with_tolerance(current_delta, 0.0L,
                                         csv_step_policy.abs_tol,
                                         csv_step_policy.rel_tol)) {
        log_warn("%s\t %s-%s [sanitize_csv_into_binary_file]%s zero/eps increment,"
                 " line %s%zu%s in %s%s%s\n",
                 ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
                 ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                 ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
        // Collapse duplicate key: keep the latest
        obj_p0 = obj_p1;
        continue;
      }

      if (current_delta < 0.0L) {
        log_fatal("[sanitize_csv_into_binary_file] key_value must be non-decreasing."
                  " At line %s%zu%s in %s%s%s\n",
                  ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                  ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      }

      // Number of steps from p0 up to (but not including) p1
      const long double steps_ld = current_delta / regular_delta;
      const auto delta_steps = detail::rounded_steps_or_fatal(
        steps_ld, line_number, csv_filename, "regular-increment");
      const long double rounded_ld = static_cast<long double>(delta_steps);
      const long double residual = std::fabs(steps_ld - rounded_ld);
      const long double allowed_residual = detail::effective_tolerance(
          steps_ld, rounded_ld,
          csv_step_policy.abs_tol,
          csv_step_policy.rel_tol);
      if (residual > allowed_residual) {
        log_fatal("[sanitize_csv_into_binary_file] Non-integer step ratio out of tolerance at line %zu in %s"
                  " (regular=%.15Lf current=%.15Lf ratio=%.15Lf rounded=%lld residual=%.3Le allowed=%.3Le)\n",
                  line_number, csv_filename.c_str(),
                  regular_delta, current_delta, steps_ld,
                  static_cast<long long>(delta_steps),
                  static_cast<long double>(residual),
                  static_cast<long double>(allowed_residual));
      }
      if (delta_steps != 1) {
        log_warn("%s\t %s-%s [sanitize_csv_into_binary_file]%s extra large step (d=%s%lld%s)"
                 " at line %s%zu%s in %s%s%s\n",
                 ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
                 ANSI_COLOR_Yellow, static_cast<long long>(delta_steps), ANSI_COLOR_RESET,
                 ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                 ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      }

      // Emit p0 and intermediate nulls (not p1; p1 becomes next anchor and is emitted later)
      for (std::int64_t i = 0; i < delta_steps; ++i) {
        const bool first = (i == 0);
        T obj_px = first ? obj_p0
                         : T::null_instance(
                             detail::cast_key_longdouble<typename T::key_type_t>(
                               kv0 + static_cast<long double>(i) * regular_delta));
        try {
          buffer.push_back(obj_px);
          if (buffer.size() == buffer_size) flush_buffer();
        } catch (const std::exception& e) {
          log_fatal("[sanitize_csv_into_binary_file] Exception at line %s%zu%s in %s%s%s: %s\n",
                    ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                    ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET, e.what());
        }
      }

      // Advance anchor
      obj_p0 = obj_p1;
    }

    if (!have_anchor) {
      log_fatal("[sanitize_csv_into_binary_file] No valid records found on CSV: %s\n",
                csv_filename.c_str());
    }

    // Push the final record (last anchor)
    buffer.push_back(obj_p0);
    flush_buffer();

    FINISH_LOADING_BAR(csv_file_preparation_progress_bar_);
    csv_file.close();
    bin_file.close();

    log_info("(sanitize_csv_into_binary_file) Raw lattice ready: %s%s%s\n",
             ANSI_COLOR_Dim_Gray, raw_bin.c_str(), ANSI_COLOR_RESET);
  } else {
    log_info("(sanitize_csv_into_binary_file) %sRaw up-to-date%s: %s\n",
             ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, raw_bin.c_str());
  }

  // Normalized output?
  if (!want_norm) {
    log_info("(sanitize_csv_into_binary_file) No normalization configured (W=0). %s%s%s -> %s%s%s\n",
             ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
             ANSI_COLOR_Dim_Gray, raw_bin.c_str(), ANSI_COLOR_RESET);
    return raw_bin;
  }

  // Produce or refresh normalized file from raw
  const bool need_norm = force_rebuild_cache
                      || !std::filesystem::exists(norm_bin)
                      || (std::filesystem::last_write_time(norm_bin) <= std::filesystem::last_write_time(raw_bin));

  if (need_norm) {
    // Copy raw -> norm and normalize in place
    std::error_code ec;
    std::filesystem::copy_file(raw_bin, norm_bin, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
      log_fatal("[sanitize_csv_into_binary_file] Failed to copy raw->norm: %s -> %s (%d)\n",
                raw_bin.c_str(), norm_bin.c_str(), (int)ec.value());
    }
#ifdef __unix__
    chmod(norm_bin.c_str(), S_IRUSR | S_IWUSR);
#endif
    normalize_binary_file<T>(norm_bin, normalization_window);
    log_info("(sanitize_csv_into_binary_file) %sNormalized%s: %s%s%s (W=%zu)\n",
             ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET,
             ANSI_COLOR_Dim_Gray, norm_bin.c_str(), ANSI_COLOR_RESET, normalization_window);
  } else {
    log_info("(sanitize_csv_into_binary_file) %sNormalized up-to-date%s: %s\n",
             ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, norm_bin.c_str());
  }

  return norm_bin;
}

} // namespace data
} // namespace camahjucunu
} // namespace cuwacunu
