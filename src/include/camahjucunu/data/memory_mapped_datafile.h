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

/*
 * Template Function: normalize_binary_file
 * ---------------------------------------
 * Reads a binary file and normalizes it in place using a rolling window.
 *
 * Requirements on T:
 *  - POD (no padding) and trivially copyable.
 *  - static auto initialize_statistics_pack(std::size_t window);
 *  - bool is_valid() const;
 *  - T  normalize(const T& rec) const;   // via stats_pack
 *  - stats_pack.update(const T& rec);    // sliding window update
 */
template <typename T>
void normalize_binary_file(const std::string& bin_filename,
                           std::size_t window_size = std::numeric_limits<std::size_t>::max()) {
  log_info("[normalize_binary_file] Normalizing binary file: %s%s%s\n",
           ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);

  std::fstream bin_file(bin_filename, std::ios::in | std::ios::out | std::ios::binary);
  if (!bin_file.is_open()) {
    log_fatal("[normalize_binary_file] Could not open: %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  // Size & record count
  bin_file.seekg(0, std::ios::end);
  const std::streamsize file_size = bin_file.tellg();
  bin_file.seekg(0, std::ios::beg);

  if (file_size < 0) {
    log_fatal("[normalize_binary_file] tellg() failed for: %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  const std::size_t total_records = static_cast<std::size_t>(file_size) / sizeof(T);
  if (static_cast<std::size_t>(file_size) % sizeof(T) != 0) {
    log_fatal("[normalize_binary_file] File size not multiple of sizeof(T): %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }
  if (total_records == 0) {
    log_info("[normalize_binary_file] Empty file, nothing to do.\n");
    return;
  }

  // Window size
  if (window_size == std::numeric_limits<std::size_t>::max() || window_size > total_records) {
    window_size = total_records;
  }

  bin_file.clear();
  bin_file.seekg(0, std::ios::beg);

  auto stats_pack = T::initialize_statistics_pack(window_size);

  // Warm-up: fill rolling stats with the first window_size records
  for (std::size_t w = 0; w < window_size; ++w) {
    T rec{};
    bin_file.read(reinterpret_cast<char*>(&rec), sizeof(T));
    if (!bin_file) {
      log_fatal("[normalize_binary_file] Failed to read during warm-up: %s%s%s\n",
                ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
    }
    stats_pack.update(rec);
  }

  // Start normalization pass from the beginning.
  bin_file.clear();
  bin_file.seekg(0, std::ios::beg);
  bin_file.seekp(0, std::ios::beg);

  START_LOADING_BAR(normalization_progress_bar_, 60, "Normalize binary file");

  for (std::size_t i = 0; i < total_records; ++i) {
    T rec{};
    bin_file.read(reinterpret_cast<char*>(&rec), sizeof(T));
    if (!bin_file) {
      log_fatal("[normalize_binary_file] Read failed at record %zu: %s%s%s\n",
                i, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
    }

    // Normalize with current window stats, then update the window.
    if (rec.is_valid()) {
      const T normalized = stats_pack.normalize(rec);

      // Move put pointer back to this record and write
      const auto write_pos = static_cast<std::streamoff>(i) * static_cast<std::streamoff>(sizeof(T));
      bin_file.seekp(write_pos, std::ios::beg);
      bin_file.write(reinterpret_cast<const char*>(&normalized), sizeof(T));
      if (!bin_file) {
        log_fatal("[normalize_binary_file] Write failed at record %zu: %s%s%s\n",
                  i, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
      }

      // Keep get pointer at next record (already there), advance rolling stats
      stats_pack.update(rec);
    }
    // progress
    double pct = (static_cast<double>(i + 1) / static_cast<double>(total_records)) * 100.0;
    pct = std::round(pct * 100.0) / 100.0;
    UPDATE_LOADING_BAR(normalization_progress_bar_, pct);
  }

  FINISH_LOADING_BAR(normalization_progress_bar_);
  bin_file.close();

  log_info("(normalize_binary_file) %sNormalization completed%s. File: %s%s%s\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
           ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
}

/*
 * sanitize_csv_into_binary_file:
 *  - Validates monotone key, fills gaps with null instances at regular increments,
 *  - Writes .bin, optionally normalizes with rolling window.
 *
 * Requirements on T:
 *  - static T from_csv(const std::string&, char, std::size_t line_no)
 *  - bool is_valid() const
 *  - using key_type_t = ...;  // numeric
 *  - key_type_t key_value() const
 *  - static T null_instance(key_type_t kv)
 */
template<typename T>
std::string sanitize_csv_into_binary_file(const std::string& csv_filename,
                                          std::size_t normalization_window = 0,
                                          bool        force_binarization = false,
                                          std::size_t buffer_size = 1024,
                                          char        delimiter   = ',') {
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

  // Output name
  std::string bin_filename = csv_filename;
  cuwacunu::piaabo::string_replace(bin_filename, ".csv", ".bin");

  // Skip if up-to-date
  if (!force_binarization && std::filesystem::exists(bin_filename)) {
    const auto csv_time = std::filesystem::last_write_time(csv_filename);
    const auto bin_time = std::filesystem::last_write_time(bin_filename);
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

  std::string line_p0, line_p1;
  std::size_t line_number = 0;

  if (!std::getline(csv_file, line_p0)) {
    log_fatal("[sanitize_csv_into_binary_file] File too short: %s\n", csv_filename.c_str());
  }
  T obj_p0 = T::from_csv(line_p0, delimiter, line_number);

  START_LOADING_BAR(csv_file_preparation_progress_bar_, 60, "Preparing Binary data file");

  bool have_regular_delta = false;
  long double regular_delta = 0.0L;
  long double tol = 1e-8L; // tolerance for floating increments

  std::size_t processed_lines = 1; // we already read the first line

  while (std::getline(csv_file, line_p1)) {
    ++processed_lines;
    double pct = (static_cast<double>(processed_lines) /
                  static_cast<double>(std::max<std::size_t>(1, total_records_hint))) * 100.0;
    pct = std::round(pct * 100.0) / 100.0;
    UPDATE_LOADING_BAR(csv_file_preparation_progress_bar_, pct);

    T obj_p1 = T::from_csv(line_p1, delimiter, ++line_number);

    if (!obj_p1.is_valid()) {
      // Skip invalid next; keep obj_p0 as previous anchor
      continue;
    }

    // Use long double to avoid signed/unsigned pitfalls and retain precision.
    const long double kv0 = static_cast<long double>(obj_p0.key_value());
    const long double kv1 = static_cast<long double>(obj_p1.key_value());
    const long double current_delta = kv1 - kv0;

    if (std::fabs(current_delta) <= tol) {
      log_warn("%s\t %s•%s [sanitize_csv_into_binary_file]%s zero/eps increment,"
               " line %s%zu%s in %s%s%s\n",
               ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
               ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
               ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      // Skip writing duplicate timestamp key; advance anchor to p1
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
      long double m = std::fmod(current_delta, regular_delta);
      m = std::fabs(m);
      if (m > tol && std::fabs(m - std::fabs(regular_delta)) > tol) {
        irregular = true;
      }
    }

    if (irregular) {
      log_err("%s\t %s•%s [sanitize_csv_into_binary_file]%s Irregular increment:"
              " (regular=%.15Lf, current=%.15Lf) at line %s%zu%s in %s%s%s\n",
              ANSI_CLEAR_LINE, ANSI_COLOR_Red, ANSI_COLOR_Dim_Gray, ANSI_COLOR_RESET,
              regular_delta, current_delta,
              ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      // Skip this transition; move anchor and continue.
      obj_p0 = obj_p1;
      line_p0 = line_p1;
      continue;
    }

    // Number of steps from p0 up to (but not including) p1
    const long double steps_ld = current_delta / regular_delta;
    const auto delta_steps = static_cast<std::int64_t>(std::llround(steps_ld));

    if (delta_steps != 1) {
      log_warn("%s\t %s•%s [sanitize_csv_into_binary_file]%s extra large step (d=%s%lld%s)"
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
                       : T::null_instance(static_cast<typename T::key_type_t>(
                             kv0 + static_cast<long double>(i) * regular_delta));
      try {
        buffer.push_back(obj_px);
        if (buffer.size() == buffer_size) {
          bin_file.write(reinterpret_cast<const char*>(buffer.data()),
                         buffer.size() * sizeof(T));
          if (!bin_file) {
            log_fatal("[sanitize_csv_into_binary_file] Buffered write failed.\n");
          }
          buffer.clear();
        }
      } catch (const std::exception& e) {
        log_fatal("[sanitize_csv_into_binary_file] Exception at line %s%zu%s in %s%s%s: %s\n",
                  ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                  ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
                  e.what());
      }
    }

    // Advance anchor
    obj_p0 = obj_p1;
    line_p0 = line_p1;
  }

  // Push the final record (last anchor)
  buffer.push_back(obj_p0);

  if (!buffer.empty()) {
    bin_file.write(reinterpret_cast<const char*>(buffer.data()),
                   buffer.size() * sizeof(T));
    if (!bin_file) {
      log_fatal("[sanitize_csv_into_binary_file] Final write failed.\n");
    }
    buffer.clear();
  }

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
