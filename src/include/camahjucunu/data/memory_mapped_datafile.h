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

#ifdef __unix__
  #include <sys/stat.h>
#endif

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

namespace detail {

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

} // namespace detail

/*
 * normalize_binary_file<T>
 * ------------------------
 * In-place normalization of a binary file of records T using a rolling window
 * built from the **previous** window_size valid records.
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

  log_info("[normalize_binary_file] policy=causal_keep_len, W=%zu. File: %s%s%s\n",
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

  // We normalize record i using stats from previous valid records (count<=window_size).
  std::size_t filled_valid = 0;       // count of valid samples seen (caps at window_size)
  std::size_t normalized_count = 0;   // diagnostics
  std::size_t invalid_count = 0;      // diagnostics

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
    // **** Minimal causal change: normalize only when the window is FULL ****
    if (rec.is_valid() && filled_valid >= window_size) {
      out = stats_pack.normalize(rec);
      ++normalized_count;
    } else if (!rec.is_valid()) {
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

  const std::size_t burn_in = std::min(filled_valid, window_size);
  log_info("(normalize_binary_file) %sNormalization completed%s. File: %s%s%s | "
           "burn_in_valid=%zu, normalized=%zu, invalid_passthrough=%zu\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
           ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET,
           burn_in, normalized_count, invalid_count);
}

/*
 * sanitize_csv_into_binary_file<T>
 * --------------------------------
 * Validates monotone key, fills gaps with null instances at regular increments,
 * writes .bin, optionally normalizes with rolling window.
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
                                          bool        force_binarization    = false,
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

  std::ifstream csv_file = cuwacunu::piaabo::dfiles::readFileToStream(csv_filename);
  if (!csv_file.is_open()) {
    log_fatal("[sanitize_csv_into_binary_file] Could not open CSV: %s\n", csv_filename.c_str());
  }

  // Output path: robust extension replacement
  std::filesystem::path bin_path(csv_filename);
  bin_path.replace_extension(".bin");
  const std::string bin_filename = bin_path.string();

  // Skip if up-to-date and not forced
  if (!force_binarization && std::filesystem::exists(bin_path)) {
    const auto csv_time = std::filesystem::last_write_time(csv_filename);
    const auto bin_time = std::filesystem::last_write_time(bin_path);
    if (bin_time > csv_time) {
      log_info("[sanitize_csv_into_binary_file]\t %sSkipped:%s up-to-date: %s\n",
               ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, bin_filename.c_str());
      return bin_filename;
    }
  }

  std::ofstream bin_file(bin_filename, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!bin_file.is_open()) {
    csv_file.close();
    log_fatal("[sanitize_csv_into_binary_file] Could not open BIN for write: %s\n",
              bin_filename.c_str());
  }

#ifdef __unix__
  chmod(bin_filename.c_str(), S_IRUSR | S_IWUSR);
#endif

  std::vector<T> buffer;
  buffer.reserve(buffer_size);

  auto flush_buffer = [&]() {
    if (!buffer.empty()) {
      bin_file.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<std::streamsize>(buffer.size() * sizeof(T)));
      if (!bin_file) {
        log_fatal("[sanitize_csv_into_binary_file] Buffered write failed.\n");
      }
      buffer.clear();
    }
  };

  std::string line_p0, line_p1;

  std::size_t line_number = 0;
  if (!std::getline(csv_file, line_p0)) {
    log_fatal("[sanitize_csv_into_binary_file] File too short: %s\n", csv_filename.c_str());
  }
  line_number = 1;
  T obj_p0 = T::from_csv(line_p0, delimiter, line_number);

  START_LOADING_BAR(csv_file_preparation_progress_bar_, 60, "Preparing Binary data file");

  bool        have_regular_delta = false;
  long double regular_delta      = 0.0L;
  constexpr long double tol      = 1e-8L; // tolerance for floating increments

  std::size_t processed_lines = 1; // first line read

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
    if (!obj_p1.is_valid()) {
      // Skip invalid *next*; keep obj_p0 as previous anchor.
      // Rationale: invalid lines may not contain a trustworthy key to place on the lattice.
      continue;
    }

    const long double kv0 = static_cast<long double>(obj_p0.key_value());
    const long double kv1 = static_cast<long double>(obj_p1.key_value());
    const long double current_delta = kv1 - kv0;

    if (std::fabs(current_delta) <= tol) {
      log_warn("%s\t %s-%s [sanitize_csv_into_binary_file]%s zero/eps increment,"
               " line %s%zu%s in %s%s%s\n",
               ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
               ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
               ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      // Collapse duplicate key: keep the latest
      obj_p0 = obj_p1;
      line_p0 = line_p1;
      continue;
    }

    if (current_delta < 0.0L) {
      log_fatal("[sanitize_csv_into_binary_file] key_value must be non-decreasing."
                " At line %s%zu%s in %s%s%s\n",
                ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
    }

    if (!have_regular_delta) {
      regular_delta = current_delta;
      have_regular_delta = true;
    }

    // Irregular increment check (allow multiples within tolerance)
    bool irregular = false;
    if (std::fabs(regular_delta) <= tol) {
      irregular = true; // degenerate
    } else {
      long double m = std::fmod(std::fabs(current_delta), std::fabs(regular_delta));
      if (m > tol && std::fabs(m - std::fabs(regular_delta)) > tol) {
        irregular = true;
      }
    }

    if (irregular) {
      // Best-effort gap fill with rounded step count; warn if residual is large.
      const long double steps_ld = current_delta / regular_delta;
      const auto delta_steps = static_cast<std::int64_t>(std::llround(steps_ld));
      const long double residual = std::fabs(steps_ld - static_cast<long double>(delta_steps));
      log_err("%s\t %s-%s [sanitize_csv_into_binary_file]%s Irregular increment:"
              " (regular=%.15Lf, current=%.15Lf, steps≈%.9Lf, rounded=%lld, residual=%.3g)"
              " at line %s%zu%s in %s%s%s — filling by rounded steps.\n",
              ANSI_CLEAR_LINE, ANSI_COLOR_Red, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
              regular_delta, current_delta, steps_ld,
              static_cast<long long>(delta_steps), static_cast<double>(residual),
              ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);

      // Emit p0 and rounded-count nulls as usual:
      for (std::int64_t i = 0; i < delta_steps; ++i) {
        const bool first = (i == 0);
        T obj_px = first ? obj_p0
                         : T::null_instance(
                             detail::cast_key_longdouble<typename T::key_type_t>(
                               kv0 + static_cast<long double>(i) * regular_delta));
        buffer.push_back(obj_px);
        if (buffer.size() == buffer_size) flush_buffer();
      }
      // Advance anchor
      obj_p0 = obj_p1;
      line_p0 = line_p1;
      continue;
    }

    // Number of steps from p0 up to (but not including) p1
    const long double steps_ld = current_delta / regular_delta;
    const auto delta_steps = static_cast<std::int64_t>(std::llround(steps_ld));
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
                               kv0 + static_cast<long double>(i) * regular_delta
                             ));
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
    line_p0 = line_p1;
  }

  // Push the final record (last anchor)
  buffer.push_back(obj_p0);
  flush_buffer();

  FINISH_LOADING_BAR(csv_file_preparation_progress_bar_);

  csv_file.close();
  bin_file.close();

  // Optional normalization
  if (normalization_window > 0) {
    normalize_binary_file<T>(bin_filename, normalization_window);
  } else {
    log_info("(sanitize_csv_into_binary_file) No normalization configured. %s%s%s -> %s%s%s\n",
             ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
             ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  log_info("(sanitize_csv_into_binary_file) %sDone%s: %s%s%s -> %s%s%s\n",
           ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET,
           ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
           ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);

  return bin_filename;
}

} // namespace data
} // namespace camahjucunu
} // namespace cuwacunu
