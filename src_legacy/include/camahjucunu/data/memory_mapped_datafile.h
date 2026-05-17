/* memory_mapped_datafile.h */
#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <limits>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <cerrno>
#include <algorithm>
#include <optional>
#include <cctype>

#ifdef __unix__
  #include <sys/stat.h>
#endif

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

namespace detail {

constexpr std::int64_t kMaxGapFillSteps = 10000000;

template <typename T>
struct binary_record_type {
  using type = T;
};

template <>
struct binary_record_type<cuwacunu::camahjucunu::exchange::trade_t> {
  using type = cuwacunu::camahjucunu::exchange::trade_cache_t;
};

template <>
struct binary_record_type<cuwacunu::camahjucunu::exchange::kline_t> {
  using type = cuwacunu::camahjucunu::exchange::kline_cache_t;
};

template <>
struct binary_record_type<cuwacunu::camahjucunu::exchange::basic_t> {
  using type = cuwacunu::camahjucunu::exchange::basic_cache_t;
};

template <typename T>
using binary_record_type_t = typename binary_record_type<T>::type;

template <typename T>
struct raw_csv_record_type {
  using type = T;
};

template <>
struct raw_csv_record_type<cuwacunu::camahjucunu::exchange::trade_cache_t> {
  using type = cuwacunu::camahjucunu::exchange::trade_t;
};

template <>
struct raw_csv_record_type<cuwacunu::camahjucunu::exchange::kline_cache_t> {
  using type = cuwacunu::camahjucunu::exchange::kline_t;
};

template <>
struct raw_csv_record_type<cuwacunu::camahjucunu::exchange::basic_cache_t> {
  using type = cuwacunu::camahjucunu::exchange::basic_t;
};

template <typename T>
using raw_csv_record_type_t = typename raw_csv_record_type<T>::type;

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

enum class rounded_step_eval_status_t {
  ok,
  non_finite,
  overflow,
  non_positive,
  gap_too_large,
};

struct rounded_step_eval_t {
  rounded_step_eval_status_t status{rounded_step_eval_status_t::non_finite};
  std::int64_t rounded{0};
};

inline rounded_step_eval_t evaluate_rounded_steps(long double steps_ld) {
  if (!std::isfinite(steps_ld)) {
    return {rounded_step_eval_status_t::non_finite, 0};
  }
  const long double abs_steps = std::fabs(steps_ld);
  const long double i64_max_ld =
      static_cast<long double>(std::numeric_limits<std::int64_t>::max());
  if (abs_steps > i64_max_ld) {
    return {rounded_step_eval_status_t::overflow, 0};
  }

  const auto rounded = static_cast<std::int64_t>(std::llround(steps_ld));
  if (rounded <= 0) {
    return {rounded_step_eval_status_t::non_positive, rounded};
  }
  if (rounded > kMaxGapFillSteps) {
    return {rounded_step_eval_status_t::gap_too_large, rounded};
  }
  return {rounded_step_eval_status_t::ok, rounded};
}

inline void rounded_steps_eval_or_fatal(const rounded_step_eval_t& eval,
                                        long double steps_ld,
                                        std::size_t line_number,
                                        const std::string& csv_filename,
                                        const char* context_label) {
  switch (eval.status) {
    case rounded_step_eval_status_t::ok:
      return;
    case rounded_step_eval_status_t::non_finite:
      log_fatal(
          "[sanitize_csv_into_binary_file] Non-finite step ratio in %s at line %zu (%s)\n",
          csv_filename.c_str(), line_number, context_label);
    case rounded_step_eval_status_t::overflow:
      log_fatal(
          "[sanitize_csv_into_binary_file] Step ratio overflow in %s at line %zu (%s): %.15Lf\n",
          csv_filename.c_str(), line_number, context_label, steps_ld);
    case rounded_step_eval_status_t::non_positive:
      log_fatal(
          "[sanitize_csv_into_binary_file] Non-positive rounded steps in %s at line %zu (%s): %lld\n",
          csv_filename.c_str(), line_number, context_label,
          static_cast<long long>(eval.rounded));
    case rounded_step_eval_status_t::gap_too_large:
      log_fatal(
          "[sanitize_csv_into_binary_file] Gap fill too large in %s at line %zu (%s): %lld > %lld\n",
          csv_filename.c_str(), line_number, context_label,
          static_cast<long long>(eval.rounded),
          static_cast<long long>(kMaxGapFillSteps));
  }
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

template <typename RawT>
struct csv_lattice_recovery {
  static std::optional<long double> bootstrap_delta(
      const RawT&,
      const RawT&,
      long double,
      const csv_step_policy_t&) {
    return std::nullopt;
  }

  static void log_bootstrap_delta_override(const RawT&,
                                           const RawT&,
                                           long double,
                                           long double,
                                           std::size_t,
                                           const std::string&) {}

  static std::optional<long double> alternate_key_value(const RawT&,
                                                        long double) {
    return std::nullopt;
  }

  template <typename BinaryT>
  static BinaryT anchor_binary(const RawT& rec,
                               long double resolved_key,
                               bool rewrite_key) {
    (void)resolved_key;
    (void)rewrite_key;
    return BinaryT::from_raw(rec);
  }

  static void log_key_rewrite(const RawT&,
                              long double,
                              std::size_t,
                              const std::string&) {}
};

template <>
struct csv_lattice_recovery<cuwacunu::camahjucunu::exchange::kline_t> {
  using raw_type = cuwacunu::camahjucunu::exchange::kline_t;

  static std::optional<long double> bootstrap_delta(
      const raw_type& anchor,
      const raw_type& curr,
      long double raw_delta,
      const csv_step_policy_t& policy) {
    const long double open_delta =
        static_cast<long double>(curr.open_time) -
        static_cast<long double>(anchor.open_time);
    if (is_near_with_tolerance(open_delta, 0.0L, policy.abs_tol,
                               policy.rel_tol)) {
      return std::nullopt;
    }
    if (open_delta > 0.0L) {
      return open_delta;
    }
    (void)raw_delta;
    return std::nullopt;
  }

  static void log_bootstrap_delta_override(const raw_type& anchor,
                                           const raw_type& curr,
                                           long double raw_delta,
                                           long double resolved_delta,
                                           std::size_t line_number,
                                           const std::string& csv_filename) {
    log_warn(
        "%s\t %s-%s [sanitize_csv_into_binary_file]%s bootstrap inferred "
        "kline step from open_time delta at line %s%zu%s in %s%s%s "
        "(raw_close_delta=%s%.0Lf%s open_delta=%s%.0Lf%s "
        "anchor_open=%s%lld%s curr_open=%s%lld%s)\n",
        ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray,
        ANSI_COLOR_RESET, ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
        ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
        ANSI_COLOR_Yellow, raw_delta, ANSI_COLOR_RESET, ANSI_COLOR_Green,
        resolved_delta, ANSI_COLOR_RESET, ANSI_COLOR_Yellow,
        static_cast<long long>(anchor.open_time), ANSI_COLOR_RESET,
        ANSI_COLOR_Green, static_cast<long long>(curr.open_time),
        ANSI_COLOR_RESET);
  }

  static std::optional<long double> alternate_key_value(
      const raw_type& rec,
      long double regular_delta) {
    if (!std::isfinite(regular_delta) || regular_delta <= 0.0L) {
      return std::nullopt;
    }
    return static_cast<long double>(rec.open_time) + regular_delta - 1.0L;
  }

  template <typename BinaryT>
  static BinaryT anchor_binary(const raw_type& rec,
                               long double resolved_key,
                               bool rewrite_key) {
    BinaryT out = BinaryT::from_raw(rec);
    if (rewrite_key) {
      out.close_time =
          cast_key_longdouble<typename BinaryT::key_type_t>(resolved_key);
    }
    return out;
  }

  static void log_key_rewrite(const raw_type& rec,
                              long double resolved_key,
                              std::size_t line_number,
                              const std::string& csv_filename) {
    log_warn(
        "%s\t %s-%s [sanitize_csv_into_binary_file]%s canonicalized kline "
        "close_time at line %s%zu%s in %s%s%s (raw=%s%lld%s canonical=%s%lld%s)\n",
        ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Gray,
        ANSI_COLOR_RESET, ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
        ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET,
        ANSI_COLOR_Yellow, static_cast<long long>(rec.close_time),
        ANSI_COLOR_RESET, ANSI_COLOR_Green,
        static_cast<long long>(std::llround(resolved_key)), ANSI_COLOR_RESET);
  }
};

template <typename T>
long double infer_regular_delta_from_csv(const std::string& csv_filename,
                                         char delimiter,
                                         const csv_step_policy_t& policy) {
  using RecoveryT = csv_lattice_recovery<T>;
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
    const long double raw_delta = kv1 - kv0;
    long double delta = raw_delta;
    if (const auto recovered_delta =
            RecoveryT::bootstrap_delta(anchor, curr, raw_delta, policy);
        recovered_delta.has_value()) {
      delta = *recovered_delta;
      if (!is_near_with_tolerance(delta, raw_delta, policy.abs_tol,
                                  policy.rel_tol)) {
        RecoveryT::log_bootstrap_delta_override(anchor, curr, raw_delta, delta,
                                                line_number, csv_filename);
      }
    }

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

enum class normalization_policy_kind_e : std::uint8_t {
  None = 0,
  LogReturns = 1,
};

struct normalization_policy_t {
  normalization_policy_kind_e kind{normalization_policy_kind_e::None};

  [[nodiscard]] bool enabled() const {
    return kind != normalization_policy_kind_e::None;
  }

  [[nodiscard]] std::string canonical_name() const {
    switch (kind) {
      case normalization_policy_kind_e::LogReturns:
        return "log_returns";
      case normalization_policy_kind_e::None:
      default:
        return "none";
    }
  }
};

[[nodiscard]] inline std::string trim_ascii_ws_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] inline std::string lowercase_ascii_copy(std::string_view text) {
  std::string out(text);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] inline normalization_policy_t parse_normalization_policy_or_fatal(
    std::string_view raw_policy,
    const char* context_label) {
  const std::string normalized =
      lowercase_ascii_copy(trim_ascii_ws_copy(raw_policy));
  if (normalized.empty() || normalized == "none" || normalized == "raw" ||
      normalized == "off" || normalized == "disabled") {
    return normalization_policy_t{};
  }
  if (normalized == "log_returns") {
    return normalization_policy_t{normalization_policy_kind_e::LogReturns};
  }
  log_fatal("[%s] Unsupported normalization policy: %s%s%s\n",
            context_label,
            ANSI_COLOR_Yellow,
            normalized.c_str(),
            ANSI_COLOR_RESET);
  return normalization_policy_t{};
}

[[nodiscard]] inline std::optional<normalization_policy_t>
try_parse_normalization_policy(std::string_view raw_policy) {
  const std::string normalized =
      lowercase_ascii_copy(trim_ascii_ws_copy(raw_policy));
  if (normalized.empty() || normalized == "none" || normalized == "raw" ||
      normalized == "off" || normalized == "disabled") {
    return normalization_policy_t{};
  }
  if (normalized == "log_returns") {
    return normalization_policy_t{normalization_policy_kind_e::LogReturns};
  }
  return std::nullopt;
}

// ----------------------------------------------------------------------------
// Filenaming helpers for cache binaries
//   - Raw cache:   <stem>.cache.bin
//   - Norm cache:  <stem>.cache.norm.<policy>.bin
// ----------------------------------------------------------------------------
inline std::string raw_bin_for_csv(const std::string& csv_filename) {
  std::filesystem::path p(csv_filename);
  const auto parent = p.parent_path();
  const auto stem = p.stem().string();
  return (parent / (stem + ".cache.bin")).string();
}

inline std::string norm_bin_for_csv(const std::string& csv_filename,
                                    std::string_view normalization_policy) {
  const normalization_policy_t policy =
      parse_normalization_policy_or_fatal(normalization_policy,
                                          "norm_bin_for_csv");
  if (!policy.enabled()) return {};
  std::filesystem::path p(csv_filename);
  const auto parent = p.parent_path();
  const auto stem   = p.stem().string();
  std::string fname = stem + ".cache.norm." + policy.canonical_name() + ".bin";
  std::filesystem::path out = parent / fname;
  return out.string();
}

// Detector: file name must be .../something.cache.norm.<policy>.bin with a
// canonical non-empty policy token.
inline bool is_bin_filename_normalized_strict(const std::string& bin_filename,
                                              std::string* policy_out = nullptr) {
  // IMPORTANT: do not touch *policy_out unless the name is recognized as normalized.
  std::filesystem::path p(bin_filename);
  if (p.extension() != ".bin") return false;

  const std::string fname = p.filename().string();
  constexpr std::string_view kNormMarker = ".cache.norm.";
  const auto pos_norm = fname.rfind(kNormMarker);
  if (pos_norm == std::string::npos) return false;

  const auto policy_pos = pos_norm + kNormMarker.size();
  const auto end  = fname.rfind(".bin");
  if (end == std::string::npos || policy_pos >= end) return false;

  const std::string token = fname.substr(policy_pos, end - policy_pos);
  if (token.empty()) return false;

  const auto parsed = try_parse_normalization_policy(token);
  if (!parsed.has_value() || !parsed->enabled()) return false;

  if (policy_out) *policy_out = parsed->canonical_name();
  return true;
}

} // namespace detail

// Public shims so other headers/tests can use the policy helpers.
inline bool is_bin_filename_normalized(const std::string& bin_filename,
                                       std::string* policy_out = nullptr) {
  return detail::is_bin_filename_normalized_strict(bin_filename, policy_out);
}

inline std::string canonical_normalization_policy(
    std::string_view normalization_policy) {
  return detail::parse_normalization_policy_or_fatal(
             normalization_policy, "canonical_normalization_policy")
      .canonical_name();
}

inline bool normalization_policy_enabled(std::string_view normalization_policy) {
  return detail::parse_normalization_policy_or_fatal(
             normalization_policy, "normalization_policy_enabled")
      .enabled();
}

inline std::string normalized_bin_for_csv(
    const std::string& csv_filename,
    std::string_view normalization_policy) {
  return detail::norm_bin_for_csv(csv_filename, normalization_policy);
}

inline std::string raw_binary_for_csv(const std::string& csv_filename) {
  return detail::raw_bin_for_csv(csv_filename);
}

template <typename T>
using binary_record_type_t = detail::binary_record_type_t<T>;

/*
 * normalize_binary_file<T>
 * ------------------------
 * In-place normalization of a binary file of records T using the configured
 * causal binary preprocessing policy.
 *
 * Requirements on T:
 *  - POD / trivially copyable
 *  - bool is_valid() const;
 *  - static T null_instance(key_type_t key_value);
 *  - static T normalize_log_returns(const T& current, const T* previous_valid);
 */
template <typename T>
void normalize_binary_file(const std::string& bin_filename,
                           std::string_view normalization_policy) {
  static_assert(std::is_trivially_copyable<T>::value,
                "normalize_binary_file<T>: T must be trivially copyable (POD-like).");

  const detail::normalization_policy_t policy =
      detail::parse_normalization_policy_or_fatal(normalization_policy,
                                                  "normalize_binary_file");
  if (!policy.enabled()) return;

  log_dbg("[normalize_binary_file] policy=%s. File: %s%s%s\n",
          policy.canonical_name().c_str(),
          ANSI_COLOR_Dim_Gray,
          bin_filename.c_str(),
          ANSI_COLOR_RESET);

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
    log_dbg("[normalize_binary_file] Empty file, nothing to do.\n");
    return;
  }

  std::fstream io(bin_filename, std::ios::in | std::ios::out | std::ios::binary);
  if (!io.is_open()) {
    log_fatal("[normalize_binary_file] Could not open: %s%s%s\n",
              ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  std::size_t normalized_count = 0;     // diagnostics: valid records normalized
  std::size_t masked_count = 0;         // diagnostics: valid records masked for missing context
  std::size_t invalid_count = 0;        // diagnostics: invalid passthrough
  std::optional<T> previous_valid{};    // previous immediate on-grid raw valid record

  io.clear();
  io.seekg(0, std::ios::beg);
  io.seekp(0, std::ios::beg);

  START_LOADING_BAR(normalization_progress_bar_, 60, "Normalize binary file");
  cuwacunu::camahjucunu::exchange::begin_normalization_warning_scope(
      bin_filename, policy.canonical_name());

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
      if (previous_valid.has_value()) {
        switch (policy.kind) {
          case detail::normalization_policy_kind_e::LogReturns:
            out = T::normalize_log_returns(
                rec, previous_valid ? &(*previous_valid) : nullptr);
            break;
          case detail::normalization_policy_kind_e::None:
          default:
            break;
        }
        ++normalized_count;
      } else {
        out = T::null_instance(rec.key_value());
        ++masked_count;
      }
      previous_valid = rec;
    } else {
      ++invalid_count; // passthrough invalids, do not update stats
      previous_valid.reset();
    }

    // Write back in place at position i
    const auto write_pos = static_cast<std::streamoff>(i) * static_cast<std::streamoff>(sizeof(T));
    io.seekp(write_pos, std::ios::beg);
    io.write(reinterpret_cast<const char*>(&out), sizeof(T));
    if (!io) {
      log_fatal("[normalize_binary_file] Write failed at record %zu: %s%s%s\n",
                i, ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET);
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

  const auto warning_summary =
      cuwacunu::camahjucunu::exchange::end_normalization_warning_scope();
  if (warning_summary.any()) {
    log_warn("[normalize_binary_file] warning summary policy=%s file=%s%s%s | "
             "value_nonpositive=%zu value_nonfinite=%zu "
             "reference_nonpositive=%zu reference_nonfinite=%zu "
             "log1p_below_floor=%zu log1p_nonfinite=%zu\n",
             policy.canonical_name().c_str(),
             ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET,
             warning_summary.log_return_value_nonpositive,
             warning_summary.log_return_value_nonfinite,
             warning_summary.log_return_reference_nonpositive,
             warning_summary.log_return_reference_nonfinite,
             warning_summary.log1p_below_floor,
             warning_summary.log1p_nonfinite);
  }

  log_dbg("(normalize_binary_file) %sNormalization completed%s. File: %s%s%s | "
          "policy=%s, normalized_valid=%zu, masked_missing_context=%zu, invalid_passthrough=%zu\n",
          ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
          ANSI_COLOR_Dim_Gray, bin_filename.c_str(), ANSI_COLOR_RESET,
          policy.canonical_name().c_str(), normalized_count, masked_count, invalid_count);
}

/*
 * sanitize_csv_into_binary_file<T>
 * --------------------------------
 * Validates monotone key, fills gaps with null instances at regular increments,
 * writes a raw cache `.cache.bin`, and optionally produces a separate
 * normalized cache `.cache.norm.<policy>.bin`.
 *
 * Policy:
 *  - normalization_policy == none         -> NO normalized file. Return RAW cache.
 *  - normalization_policy == log_returns  -> Produce
 *      `<stem>.cache.norm.log_returns.bin`
 *      (copy raw cache, normalize in place). Return that path.
 *
 * Requirements on T:
 *  - RawT::from_csv(const std::string&, char, std::size_t line_no)
 *  - RawT::is_valid() const
 *  - RawT::key_type_t matches BinaryT::key_type_t
 *  - BinaryT::from_raw(const RawT&)
 *  - BinaryT::null_instance(key_type_t)
 */
template<typename T>
std::string sanitize_csv_into_binary_file(const std::string& csv_filename,
                                          std::string normalization_policy = "none",
                                          bool        force_rebuild_cache   = false,
                                          std::size_t buffer_size           = 1024,
                                          char        delimiter             = ',',
                                          const detail::csv_step_policy_t& csv_step_policy =
                                              detail::csv_step_policy_t{}) {
  using RawT = detail::raw_csv_record_type_t<T>;
  using BinaryT = detail::binary_record_type_t<T>;
  using RecoveryT = detail::csv_lattice_recovery<RawT>;
  static_assert(std::is_trivially_copyable<RawT>::value,
                "sanitize_csv_into_binary_file<T>: RawT must be trivially copyable (POD-like).");
  static_assert(std::is_trivially_copyable<BinaryT>::value,
                "sanitize_csv_into_binary_file<T>: BinaryT must be trivially copyable (POD-like).");
  static_assert(std::is_same_v<typename RawT::key_type_t, typename BinaryT::key_type_t>,
                "sanitize_csv_into_binary_file<T>: raw and binary key types must match.");

  const detail::normalization_policy_t policy =
      detail::parse_normalization_policy_or_fatal(normalization_policy,
                                                  "sanitize_csv_into_binary_file");
  normalization_policy = policy.canonical_name();

  if (buffer_size < 1) {
    log_fatal("[sanitize_csv_into_binary_file] buffer_size must be >= 1 for file: %s\n",
              csv_filename.c_str());
  }

  const std::size_t total_records_hint =
      cuwacunu::piaabo::dfiles::countLinesInFile(csv_filename);

  // Resolve output names
  const std::string raw_bin  = detail::raw_bin_for_csv(csv_filename);
  const bool want_norm       = policy.enabled();
  const std::string norm_bin = want_norm
                                   ? detail::norm_bin_for_csv(csv_filename,
                                                              normalization_policy)
                                   : std::string{};

  // If not forced, decide if target output is already up-to-date.
  auto newer_than = [](const std::string& a, const std::string& b) -> bool {
    std::error_code ec1, ec2;
    if (!std::filesystem::exists(a, ec1) || !std::filesystem::exists(b, ec2)) return false;
    return std::filesystem::last_write_time(a, ec1) > std::filesystem::last_write_time(b, ec2);
  };

  // We will *always* ensure the raw cache is up-to-date w.r.t. the CSV,
  // because normalized caches are derived from it.
  bool need_write_raw = force_rebuild_cache || !newer_than(raw_bin, csv_filename);

  if (need_write_raw) {
    log_dbg("[sanitize_csv_into_binary_file]\t %sPreparing binary%s from CSV: %s\n",
            ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, csv_filename.c_str());
    const long double regular_delta = detail::infer_regular_delta_from_csv<RawT>(
        csv_filename, delimiter, csv_step_policy);
    log_dbg("[sanitize_csv_into_binary_file] inferred regular_delta=%.15Lf "
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

    std::vector<BinaryT> buffer;
    buffer.reserve(buffer_size);

    auto flush_buffer = [&]() {
      if (buffer.empty()) return;
      bin_file.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<std::streamsize>(buffer.size() * sizeof(BinaryT)));
      if (!bin_file) {
        log_fatal("[sanitize_csv_into_binary_file] Buffered write failed.\n");
      }
      buffer.clear();
    };

    START_LOADING_BAR(csv_file_preparation_progress_bar_, 60, "Preparing Binary data file");

    std::size_t processed_lines = 0;
    std::size_t line_number = 0;
    bool have_anchor = false;
    RawT obj_p0{};
    std::size_t obj_p0_line_number = 0;
    long double obj_p0_resolved_key = 0.0L;
    bool obj_p0_rewrite_key = false;

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

      RawT obj_p1 = RawT::from_csv(line_p1, delimiter, line_number);
      if (!obj_p1.is_valid()) continue;

      if (!have_anchor) {
        obj_p0 = obj_p1;
        obj_p0_line_number = line_number;
        obj_p0_resolved_key =
            static_cast<long double>(obj_p0.key_value());
        obj_p0_rewrite_key = false;
        have_anchor = true;
        continue;
      }

      const long double kv0_raw = static_cast<long double>(obj_p0.key_value());
      const long double kv1_raw = static_cast<long double>(obj_p1.key_value());
      long double kv0 = obj_p0_resolved_key;
      long double kv1 = kv1_raw;
      bool rewrite_p0_key = obj_p0_rewrite_key;
      bool rewrite_p1_key = false;
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
        obj_p0_line_number = line_number;
        obj_p0_resolved_key = kv1_raw;
        obj_p0_rewrite_key = false;
        continue;
      }

      if (current_delta < 0.0L) {
        log_fatal("[sanitize_csv_into_binary_file] key_value must be non-decreasing."
                  " At line %s%zu%s in %s%s%s\n",
                  ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET,
                  ANSI_COLOR_Dim_Gray, csv_filename.c_str(), ANSI_COLOR_RESET);
      }

      // Number of steps from p0 up to (but not including) p1
      long double steps_ld = current_delta / regular_delta;
      auto step_eval = detail::evaluate_rounded_steps(steps_ld);
      auto delta_steps = step_eval.rounded;
      long double rounded_ld = static_cast<long double>(delta_steps);
      long double residual = std::fabs(steps_ld - rounded_ld);
      long double allowed_residual = detail::effective_tolerance(
          steps_ld, rounded_ld,
          csv_step_policy.abs_tol,
          csv_step_policy.rel_tol);
      if (step_eval.status != detail::rounded_step_eval_status_t::ok ||
          residual > allowed_residual) {
        bool recovered = false;
        const auto alt_kv0 =
            RecoveryT::alternate_key_value(obj_p0, regular_delta);
        const auto alt_kv1 =
            RecoveryT::alternate_key_value(obj_p1, regular_delta);
        if (alt_kv0.has_value() && alt_kv1.has_value()) {
          const long double alt_delta = *alt_kv1 - *alt_kv0;
          if (alt_delta > 0.0L) {
            const long double alt_steps_ld = alt_delta / regular_delta;
            const auto alt_step_eval =
                detail::evaluate_rounded_steps(alt_steps_ld);
            const auto alt_delta_steps = alt_step_eval.rounded;
            const long double alt_rounded_ld =
                static_cast<long double>(alt_delta_steps);
            const long double alt_residual =
                std::fabs(alt_steps_ld - alt_rounded_ld);
            const long double alt_allowed_residual =
                detail::effective_tolerance(
                    alt_steps_ld, alt_rounded_ld, csv_step_policy.abs_tol,
                    csv_step_policy.rel_tol);
            if (alt_step_eval.status ==
                    detail::rounded_step_eval_status_t::ok &&
                alt_residual <= alt_allowed_residual) {
              kv0 = *alt_kv0;
              kv1 = *alt_kv1;
              steps_ld = alt_steps_ld;
              delta_steps = alt_delta_steps;
              step_eval = alt_step_eval;
              rounded_ld = alt_rounded_ld;
              residual = alt_residual;
              allowed_residual = alt_allowed_residual;
              const bool new_rewrite_p0 =
                  !detail::is_near_with_tolerance(
                      kv0_raw, kv0, csv_step_policy.abs_tol,
                      csv_step_policy.rel_tol);
              rewrite_p0_key = rewrite_p0_key || new_rewrite_p0;
              rewrite_p1_key =
                  !detail::is_near_with_tolerance(
                      kv1_raw, kv1, csv_step_policy.abs_tol,
                      csv_step_policy.rel_tol);
              if (new_rewrite_p0 && !obj_p0_rewrite_key) {
                RecoveryT::log_key_rewrite(
                    obj_p0, kv0, obj_p0_line_number, csv_filename);
              }
              if (rewrite_p1_key) {
                RecoveryT::log_key_rewrite(obj_p1, kv1, line_number,
                                           csv_filename);
              }
              recovered = true;
            }
          }
        }
        if (!recovered) {
          if (step_eval.status != detail::rounded_step_eval_status_t::ok) {
            detail::rounded_steps_eval_or_fatal(
                step_eval, steps_ld, line_number, csv_filename,
                "regular-increment");
          }
          log_fatal(
              "[sanitize_csv_into_binary_file] Non-integer step ratio out of tolerance at line %zu in %s"
              " (regular=%.15Lf current=%.15Lf ratio=%.15Lf rounded=%lld residual=%.3Le allowed=%.3Le)\n",
              line_number, csv_filename.c_str(), regular_delta, current_delta,
              steps_ld, static_cast<long long>(delta_steps),
              static_cast<long double>(residual),
              static_cast<long double>(allowed_residual));
        }
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
        BinaryT obj_px = first ? RecoveryT::template anchor_binary<BinaryT>(
                                   obj_p0, kv0, rewrite_p0_key)
                               : BinaryT::null_instance(
                             detail::cast_key_longdouble<typename BinaryT::key_type_t>(
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
      obj_p0_line_number = line_number;
      obj_p0_resolved_key = kv1;
      obj_p0_rewrite_key = rewrite_p1_key;
    }

    if (!have_anchor) {
      log_fatal("[sanitize_csv_into_binary_file] No valid records found on CSV: %s\n",
                csv_filename.c_str());
    }

    // Push the final record (last anchor)
    buffer.push_back(
        RecoveryT::template anchor_binary<BinaryT>(
            obj_p0, obj_p0_resolved_key, obj_p0_rewrite_key));
    flush_buffer();

    FINISH_LOADING_BAR(csv_file_preparation_progress_bar_);
    csv_file.close();
    bin_file.close();

    log_dbg("(sanitize_csv_into_binary_file) Raw cache lattice ready: %s%s%s\n",
            ANSI_COLOR_Dim_Gray, raw_bin.c_str(), ANSI_COLOR_RESET);
  } else {
    log_dbg("(sanitize_csv_into_binary_file) %sRaw cache up-to-date%s: %s\n",
            ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, raw_bin.c_str());
  }

  // Normalized output?
  if (!want_norm) {
    log_dbg("(sanitize_csv_into_binary_file) No normalization configured (policy=none). %s%s%s -> %s%s%s\n",
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
    normalize_binary_file<BinaryT>(norm_bin, normalization_policy);
    log_dbg("(sanitize_csv_into_binary_file) %sNormalized%s: %s%s%s (policy=%s)\n",
            ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET,
            ANSI_COLOR_Dim_Gray, norm_bin.c_str(), ANSI_COLOR_RESET,
            normalization_policy.c_str());
  } else {
    log_dbg("(sanitize_csv_into_binary_file) %sNormalized up-to-date%s: %s\n",
            ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, norm_bin.c_str());
  }

  return norm_bin;
}

} // namespace data
} // namespace camahjucunu
} // namespace cuwacunu
