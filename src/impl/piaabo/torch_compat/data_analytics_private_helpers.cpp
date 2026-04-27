#include "piaabo/torch_compat/data_analytics.h"

#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include <ATen/ops/linalg_eigvalsh.h>

#include <atomic>
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

constexpr double kNumericEpsilon = 1e-18;
constexpr std::string_view kRefRangeNonNegative = "[0,+inf)";
constexpr std::string_view kRefRangePositive = "[1,+inf)";
constexpr std::string_view kRefRangeSigned = "(-inf,+inf)";
constexpr std::string_view kRefRangeUnitInterval = "[0,1]";
constexpr std::size_t kSymbolicAlphabetSize = 3;
constexpr std::uint64_t kSymbolicMinEligibleSamples = 128;
constexpr std::size_t kSymbolicCompressionRawBitsPerToken = 8;
constexpr std::size_t kAutocorrelationMaxLag = 64;
constexpr std::size_t kPowerSpectrumMaxBins = 64;
constexpr double kAutocorrelationDecayThreshold = 0.36787944117144233;
constexpr double kPi = 3.14159265358979323846;

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

[[nodiscard]] inline double clamp_nonneg(double v) {
  return (v > 0.0) ? v : 0.0;
}

[[nodiscard]] std::string_view trim_ascii_ws_view_(std::string_view text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return text.substr(begin, end - begin);
}

[[nodiscard]] std::string contract_hash_path_token_(std::string_view contract_hash) {
  return cuwacunu::hashimyei::compact_contract_hash_path_token(contract_hash);
}

[[nodiscard]] std::string analytics_path_token_(std::string_view token_text) {
  const std::string_view token = trim_ascii_ws_view_(token_text);
  if (token.empty()) return {};

  std::string out;
  out.reserve(token.size());
  for (const unsigned char c : token) {
    const bool ok =
        (std::isalnum(c) != 0) || c == '_' || c == '-' || c == '.';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  return out;
}

void append_component_report_identity_entries_(
    runtime_lls_document_t* document,
    const tsiemene::component_report_identity_t& report_identity) {
  if (!document) return;
  cuwacunu::piaabo::latent_lineage_state::append_runtime_report_header_entries(
      document, tsiemene::make_runtime_report_header(report_identity));
}

void append_string_entry_if_nonempty_(runtime_lls_document_t* document,
                                      std::string_view key,
                                      std::string_view value) {
  if (!document || value.empty()) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

void append_bool_entry_(runtime_lls_document_t* document,
                        std::string_view key,
                        bool value) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          std::string(key), value));
}

void append_u64_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::uint64_t value,
                       std::string_view declared_domain = kRefRangeNonNegative) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, std::string(declared_domain)));
}

void append_i64_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::int64_t value,
                       std::string_view declared_domain = kRefRangeSigned) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value, std::string(declared_domain)));
}

void append_double_entry_(runtime_lls_document_t* document,
                          std::string_view key,
                          double value,
                          std::string_view declared_domain = kRefRangeSigned) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value, std::string(declared_domain)));
}

[[nodiscard]] std::string join_strings_csv_(
    const std::vector<std::string>& parts) {
  if (parts.empty()) return {};
  std::ostringstream oss;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) oss << ",";
    oss << parts[i];
  }
  return oss.str();
}

[[nodiscard]] const std::vector<std::string>& kline_feature_names_() {
  static const std::vector<std::string> names = {
      "open_price",
      "high_price",
      "low_price",
      "close_price",
      "volume",
      "quote_asset_volume",
      "number_of_trades",
      "taker_buy_base_volume",
      "taker_buy_quote_volume",
  };
  return names;
}

[[nodiscard]] const std::vector<std::string>& trade_feature_names_() {
  static const std::vector<std::string> names = {
      "price",
      "qty",
      "quoteQty",
      "isBuyerMaker",
      "isBestMatch",
  };
  return names;
}

[[nodiscard]] const std::vector<std::string>& basic_feature_names_() {
  static const std::vector<std::string> names = {"value"};
  return names;
}

[[nodiscard]] const std::vector<std::string>& empty_feature_names_() {
  static const std::vector<std::string> names{};
  return names;
}

[[nodiscard]] std::vector<std::uint8_t> symbolize_ternary_quantiles_(
    const std::vector<double>& values) {
  std::vector<std::uint8_t> out;
  out.reserve(values.size());
  if (values.empty()) return out;

  std::vector<double> sorted = values;
  std::sort(sorted.begin(), sorted.end());
  const std::size_t lower_idx = (sorted.size() - 1) / 3;
  const std::size_t upper_idx = ((sorted.size() - 1) * 2) / 3;
  const double lower_cut = sorted[lower_idx];
  const double upper_cut = sorted[upper_idx];

  for (double value : values) {
    if (value < lower_cut) {
      out.push_back(0);
    } else if (value < upper_cut) {
      out.push_back(1);
    } else {
      out.push_back(2);
    }
  }
  return out;
}

[[nodiscard]] std::uint64_t observed_symbol_count_(
    const std::vector<std::uint8_t>& tokens) {
  std::array<bool, kSymbolicAlphabetSize> seen{};
  for (std::uint8_t token : tokens) {
    if (token < kSymbolicAlphabetSize) seen[token] = true;
  }
  std::uint64_t count = 0;
  for (bool flag : seen) {
    if (flag) ++count;
  }
  return count;
}

[[nodiscard]] std::uint64_t lz76_complexity_(
    const std::vector<std::uint8_t>& tokens) {
  if (tokens.empty()) return 0;

  std::string sequence;
  sequence.reserve(tokens.size());
  for (std::uint8_t token : tokens) {
    sequence.push_back(static_cast<char>('0' + std::min<std::uint8_t>(token, 9)));
  }

  const std::size_t n = sequence.size();
  std::size_t pos = 0;
  std::uint64_t complexity = 0;
  while (pos < n) {
    std::size_t phrase_len = 1;
    const std::string_view prefix(sequence.data(), pos);
    while (pos + phrase_len <= n) {
      const std::string_view candidate(sequence.data() + pos, phrase_len);
      if (pos == 0 || prefix.find(candidate) == std::string_view::npos) break;
      ++phrase_len;
    }
    if (pos + phrase_len > n) phrase_len = n - pos;
    if (phrase_len == 0) phrase_len = 1;
    ++complexity;
    pos += phrase_len;
  }
  return complexity;
}

[[nodiscard]] std::uint64_t ceil_log2_u64_(std::uint64_t value) {
  if (value <= 1) return 1;
  std::uint64_t width = 0;
  std::uint64_t v = value - 1;
  while (v > 0) {
    ++width;
    v >>= 1;
  }
  return width;
}

[[nodiscard]] double normalized_lz76_complexity_(
    std::uint64_t complexity,
    std::size_t token_count) {
  if (complexity == 0 || token_count < 2) return 0.0;
  const double n = static_cast<double>(token_count);
  const double log_base = std::log(static_cast<double>(kSymbolicAlphabetSize));
  if (!(log_base > 0.0)) return 0.0;
  const double log_n_base =
      std::log(std::max(1.0, n)) / log_base;
  if (!std::isfinite(log_n_base) || log_n_base <= 0.0) return 0.0;
  return (static_cast<double>(complexity) * log_n_base) / n;
}

[[nodiscard]] double entropy_rate_bits_from_bigrams_(
    const std::vector<std::uint8_t>& tokens) {
  if (tokens.size() < 2) return 0.0;

  std::array<std::uint64_t, kSymbolicAlphabetSize> prev_counts{};
  std::array<std::array<std::uint64_t, kSymbolicAlphabetSize>, kSymbolicAlphabetSize>
      pair_counts{};

  std::uint64_t transition_count = 0;
  for (std::size_t i = 1; i < tokens.size(); ++i) {
    const std::uint8_t prev = tokens[i - 1];
    const std::uint8_t cur = tokens[i];
    if (prev >= kSymbolicAlphabetSize || cur >= kSymbolicAlphabetSize) continue;
    ++prev_counts[prev];
    ++pair_counts[prev][cur];
    ++transition_count;
  }
  if (transition_count == 0) return 0.0;

  double entropy = 0.0;
  const double total = static_cast<double>(transition_count);
  for (std::size_t prev = 0; prev < kSymbolicAlphabetSize; ++prev) {
    if (prev_counts[prev] == 0) continue;
    const double prev_total = static_cast<double>(prev_counts[prev]);
    for (std::size_t cur = 0; cur < kSymbolicAlphabetSize; ++cur) {
      if (pair_counts[prev][cur] == 0) continue;
      const double joint = static_cast<double>(pair_counts[prev][cur]) / total;
      const double conditional =
          static_cast<double>(pair_counts[prev][cur]) / prev_total;
      entropy -= joint * std::log2(conditional);
    }
  }
  return std::isfinite(entropy) ? entropy : 0.0;
}

[[nodiscard]] double compression_ratio_lzw_(
    const std::vector<std::uint8_t>& tokens) {
  if (tokens.empty()) return 0.0;

  std::unordered_map<std::string, std::uint32_t> dictionary{};
  dictionary.reserve(tokens.size() * 2 + kSymbolicAlphabetSize);
  for (std::uint8_t token = 0; token < kSymbolicAlphabetSize; ++token) {
    dictionary.emplace(
        std::string(
            1,
            static_cast<char>('0' + std::min<std::uint8_t>(token, 9))),
        static_cast<std::uint32_t>(token));
  }

  std::uint64_t next_code = kSymbolicAlphabetSize;
  std::uint64_t encoded_bits = 0;
  std::string phrase(
      1,
      static_cast<char>('0' + std::min<std::uint8_t>(tokens.front(), 9)));
  for (std::size_t i = 1; i < tokens.size(); ++i) {
    const char symbol =
        static_cast<char>('0' + std::min<std::uint8_t>(tokens[i], 9));
    std::string candidate = phrase;
    candidate.push_back(symbol);
    if (dictionary.find(candidate) != dictionary.end()) {
      phrase = std::move(candidate);
      continue;
    }

    encoded_bits += ceil_log2_u64_(std::max<std::uint64_t>(2, next_code));
    dictionary.emplace(std::move(candidate), static_cast<std::uint32_t>(next_code));
    ++next_code;
    phrase.assign(1, symbol);
  }

  encoded_bits += ceil_log2_u64_(std::max<std::uint64_t>(2, next_code));
  const double raw_bits = static_cast<double>(
      tokens.size() * kSymbolicCompressionRawBitsPerToken);
  if (!(raw_bits > 0.0)) return 0.0;
  return static_cast<double>(encoded_bits) / raw_bits;
}

[[nodiscard]] double autocorrelation_decay_lag_(
    const std::vector<double>& values) {
  if (values.size() < 2) return 0.0;

  double mean = 0.0;
  for (double value : values) {
    mean += value;
  }
  mean /= static_cast<double>(values.size());

  std::vector<double> centered;
  centered.reserve(values.size());
  double denom = 0.0;
  for (double value : values) {
    const double c = value - mean;
    centered.push_back(c);
    denom += c * c;
  }
  if (!(denom > kNumericEpsilon)) return 0.0;

  const std::size_t max_lag =
      std::min<std::size_t>(kAutocorrelationMaxLag, values.size() - 1);
  for (std::size_t lag = 1; lag <= max_lag; ++lag) {
    double numer = 0.0;
    for (std::size_t i = lag; i < centered.size(); ++i) {
      numer += centered[i] * centered[i - lag];
    }
    const double acf = numer / denom;
    if (std::isfinite(acf) &&
        std::abs(acf) <= kAutocorrelationDecayThreshold) {
      return static_cast<double>(lag);
    }
  }
  return static_cast<double>(max_lag);
}

[[nodiscard]] double power_spectrum_entropy_(
    const std::vector<double>& values) {
  if (values.size() < 4) return 0.0;

  double mean = 0.0;
  for (double value : values) {
    mean += value;
  }
  mean /= static_cast<double>(values.size());

  std::vector<double> centered;
  centered.reserve(values.size());
  double total_energy = 0.0;
  for (double value : values) {
    const double c = value - mean;
    centered.push_back(c);
    total_energy += c * c;
  }
  if (!(total_energy > kNumericEpsilon)) return 0.0;

  const std::size_t bin_count =
      std::min<std::size_t>(kPowerSpectrumMaxBins, values.size() / 2);
  if (bin_count < 2) return 0.0;

  std::vector<double> powers(bin_count, 0.0);
  const double n = static_cast<double>(centered.size());
  double power_sum = 0.0;
  for (std::size_t k = 1; k <= bin_count; ++k) {
    double real = 0.0;
    double imag = 0.0;
    for (std::size_t t = 0; t < centered.size(); ++t) {
      const double angle =
          (2.0 * kPi * static_cast<double>(k) * static_cast<double>(t)) / n;
      real += centered[t] * std::cos(angle);
      imag -= centered[t] * std::sin(angle);
    }
    const double power = (real * real) + (imag * imag);
    if (std::isfinite(power) && power > 0.0) {
      powers[k - 1] = power;
      power_sum += power;
    }
  }
  if (!(power_sum > kNumericEpsilon)) return 0.0;

  double entropy = 0.0;
  for (double power : powers) {
    if (!(power > 0.0)) continue;
    const double prob = power / power_sum;
    entropy -= prob * std::log2(prob);
  }
  const double denom = std::log2(static_cast<double>(bin_count));
  if (!std::isfinite(entropy) || !(denom > 0.0)) return 0.0;
  return std::clamp(entropy / denom, 0.0, 1.0);
}

[[nodiscard]] double hurst_exponent_rs_(
    const std::vector<double>& values) {
  if (values.size() < 16) return 0.0;

  std::vector<double> log_scales;
  std::vector<double> log_rs;
  for (std::size_t scale = 8; scale <= values.size() / 4; scale *= 2) {
    const std::size_t segment_count = values.size() / scale;
    if (segment_count == 0) continue;

    double rs_sum = 0.0;
    std::size_t rs_count = 0;
    for (std::size_t segment = 0; segment < segment_count; ++segment) {
      const std::size_t begin = segment * scale;
      const std::size_t end = begin + scale;

      double mean = 0.0;
      for (std::size_t i = begin; i < end; ++i) {
        mean += values[i];
      }
      mean /= static_cast<double>(scale);

      double sq_sum = 0.0;
      double cumulative = 0.0;
      double min_cumulative = 0.0;
      double max_cumulative = 0.0;
      for (std::size_t i = begin; i < end; ++i) {
        const double deviation = values[i] - mean;
        sq_sum += deviation * deviation;
        cumulative += deviation;
        min_cumulative = std::min(min_cumulative, cumulative);
        max_cumulative = std::max(max_cumulative, cumulative);
      }

      const double stdev = std::sqrt(sq_sum / static_cast<double>(scale));
      const double range = max_cumulative - min_cumulative;
      if (!(stdev > kNumericEpsilon) || !(range > kNumericEpsilon)) continue;

      rs_sum += range / stdev;
      ++rs_count;
    }

    if (rs_count == 0) continue;
    const double mean_rs = rs_sum / static_cast<double>(rs_count);
    if (!(mean_rs > 0.0) || !std::isfinite(mean_rs)) continue;

    log_scales.push_back(std::log(static_cast<double>(scale)));
    log_rs.push_back(std::log(mean_rs));
  }

  if (log_scales.size() < 2 || log_scales.size() != log_rs.size()) return 0.0;

  double x_mean = 0.0;
  double y_mean = 0.0;
  for (std::size_t i = 0; i < log_scales.size(); ++i) {
    x_mean += log_scales[i];
    y_mean += log_rs[i];
  }
  const double denom_n = static_cast<double>(log_scales.size());
  x_mean /= denom_n;
  y_mean /= denom_n;

  double cov = 0.0;
  double var = 0.0;
  for (std::size_t i = 0; i < log_scales.size(); ++i) {
    const double dx = log_scales[i] - x_mean;
    cov += dx * (log_rs[i] - y_mean);
    var += dx * dx;
  }
  if (!(var > kNumericEpsilon)) return 0.0;
  return std::clamp(cov / var, 0.0, 1.0);
}

struct sequence_report_key_config_t {
  std::string_view label_key{};
  std::string_view channels_key{};
  std::string_view timesteps_key{};
  std::string_view features_per_timestep_key{};
  std::string_view flat_feature_count_key{};
  std::string_view effective_feature_count_key{};
  std::string_view entropic_load_key{};
  std::string_view cov_trace_key{};
  std::string_view nonzero_eigen_count_key{};
};

struct symbolic_report_key_config_t {
  std::string_view label_key{};
  std::string_view count_key{};
  std::string_view eligible_count_key{};
  std::string_view item_prefix{};
  std::string_view family_key{};
  bool emit_reduction_metadata{false};
};

struct symbolic_pretty_config_t {
  std::string_view title{};
  std::string_view label_key{};
  std::string_view count_key{};
  std::string_view eligible_count_key{};
  std::string_view label_note{};
  std::string_view count_note{};
  std::string_view eligible_count_note{};
  std::string_view family_key{};
  std::string_view family_note{};
  std::string_view item_bracket_key{};
  bool emit_generalized_context_comment{false};
  bool emit_reduction_metadata{false};
};

[[nodiscard]] const sequence_report_key_config_t& generic_sequence_report_keys_() {
  static const sequence_report_key_config_t keys = {
      "sequence_label",
      "sequence_channels",
      "sequence_timesteps",
      "sequence_features_per_timestep",
      "sequence_flat_feature_count",
      "sequence_effective_feature_count",
      "sequence_entropic_load",
      "sequence_cov_trace",
      "sequence_nonzero_eigen_count",
  };
  return keys;
}

[[nodiscard]] const sequence_report_key_config_t& source_sequence_report_keys_() {
  static const sequence_report_key_config_t keys = {
      "source_label",
      "source_channels",
      "source_timesteps",
      "source_features_per_timestep",
      "source_flat_feature_count",
      "source_effective_feature_count",
      "source_entropic_load",
      "source_cov_trace",
      "source_nonzero_eigen_count",
  };
  return keys;
}

[[nodiscard]] const symbolic_report_key_config_t&
generic_symbolic_report_keys_() {
  static const symbolic_report_key_config_t keys = {
      "sequence_label",
      "stream_count",
      "eligible_stream_count",
      "stream_",
      "stream_family",
      true,
  };
  return keys;
}

[[nodiscard]] const symbolic_report_key_config_t& source_symbolic_report_keys_() {
  static const symbolic_report_key_config_t keys = {
      "source_label",
      "channel_count",
      "eligible_channel_count",
      "channel_",
      "record_type",
      false,
  };
  return keys;
}

[[nodiscard]] const symbolic_pretty_config_t&
generic_symbolic_pretty_config_() {
  static const symbolic_pretty_config_t cfg = {
      "Sequence Symbolic Analytics Report",
      "sequence_label",
      "stream_count",
      "eligible_stream_count",
      "sequence instance",
      "streams considered",
      "streams with enough anchor samples",
      "stream_family",
      "stream family",
      "stream",
      true,
      true,
  };
  return cfg;
}

[[nodiscard]] const symbolic_pretty_config_t&
source_symbolic_pretty_config_() {
  static const symbolic_pretty_config_t cfg = {
      "Source Symbolic Data Analytics Report",
      "source_label",
      "channel_count",
      "eligible_channel_count",
      "configured source label",
      "channels considered",
      "channels with enough anchor samples",
      "record_type",
      "record family",
      "channel",
      false,
      false,
  };
  return cfg;
}

[[nodiscard]] sequence_symbolic_report_compaction_options_t
normalize_sequence_symbolic_report_compaction_options_(
    const sequence_symbolic_report_compaction_options_t& in) {
  sequence_symbolic_report_compaction_options_t out = in;
  if (out.reduce_when_stream_count_exceeds < 1) {
    out.reduce_when_stream_count_exceeds = 1;
  }
  if (out.max_reported_streams < 0) {
    out.max_reported_streams = 0;
  }
  return out;
}

[[nodiscard]] bool contains_index_(
    const std::vector<std::size_t>& values,
    std::size_t index) {
  return std::find(values.begin(), values.end(), index) != values.end();
}

void add_stream_index_for_label_(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view label,
    std::vector<std::size_t>* out_indices) {
  if (!out_indices || label.empty()) return;
  for (std::size_t i = 0; i < report.streams.size(); ++i) {
    if (report.streams[i].label == label && !contains_index_(*out_indices, i)) {
      out_indices->push_back(i);
      return;
    }
  }
}

void fill_remaining_stream_indices_evenly_(
    const sequence_symbolic_analytics_report_t& report,
    std::size_t target_count,
    std::vector<std::size_t>* out_indices) {
  if (!out_indices) return;
  if (target_count == 0 || out_indices->size() >= target_count) return;

  std::vector<std::size_t> remaining;
  remaining.reserve(report.streams.size());
  for (std::size_t i = 0; i < report.streams.size(); ++i) {
    if (!contains_index_(*out_indices, i)) remaining.push_back(i);
  }
  if (remaining.empty()) return;

  const std::size_t slots = target_count - out_indices->size();
  if (slots >= remaining.size()) {
    out_indices->insert(out_indices->end(), remaining.begin(), remaining.end());
    return;
  }

  if (slots == 1) {
    out_indices->push_back(remaining.front());
    return;
  }

  const double span = static_cast<double>(remaining.size() - 1);
  const double denom = static_cast<double>(slots - 1);
  for (std::size_t i = 0; i < slots; ++i) {
    const double pos = (denom > 0.0) ? (span * static_cast<double>(i) / denom)
                                     : 0.0;
    const std::size_t idx = static_cast<std::size_t>(std::llround(pos));
    const std::size_t selected = remaining[std::min(idx, remaining.size() - 1)];
    if (!contains_index_(*out_indices, selected)) {
      out_indices->push_back(selected);
    }
  }

  if (out_indices->size() < target_count) {
    for (std::size_t idx : remaining) {
      if (!contains_index_(*out_indices, idx)) out_indices->push_back(idx);
      if (out_indices->size() >= target_count) break;
    }
  }
}

[[nodiscard]] runtime_lls_document_t make_sequence_runtime_lls_document_(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    const sequence_report_key_config_t& keys,
    std::string_view report_label,
    const tsiemene::component_report_identity_t& report_identity) {
  runtime_lls_document_t document{};
  document.entries.reserve(18);
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  append_string_entry_if_nonempty_(&document, keys.label_key, report_label);

  append_u64_entry_(&document, "sample_count", report.sample_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "valid_sample_count", report.valid_sample_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "skipped_sample_count", report.skipped_sample_count, kRefRangeNonNegative);

  append_i64_entry_(
      &document, keys.channels_key, report.sequence_channels, kRefRangeNonNegative);
  append_i64_entry_(
      &document, keys.timesteps_key, report.sequence_timesteps, kRefRangeNonNegative);
  append_i64_entry_(
      &document,
      keys.features_per_timestep_key,
      report.sequence_features_per_timestep,
      kRefRangeNonNegative);
  append_i64_entry_(
      &document,
      keys.flat_feature_count_key,
      report.sequence_flat_feature_count,
      kRefRangeNonNegative);
  append_i64_entry_(
      &document,
      keys.effective_feature_count_key,
      report.sequence_effective_feature_count,
      kRefRangeNonNegative);

  append_double_entry_(
      &document,
      keys.entropic_load_key,
      report.sequence_entropic_load,
      kRefRangeNonNegative);
  append_double_entry_(
      &document, keys.cov_trace_key, report.sequence_cov_trace, kRefRangeNonNegative);
  append_u64_entry_(
      &document,
      keys.nonzero_eigen_count_key,
      report.sequence_nonzero_eigen_count,
      kRefRangeNonNegative);

  append_i64_entry_(&document, "max_samples", options.max_samples, kRefRangePositive);
  append_i64_entry_(&document, "max_features", options.max_features, kRefRangePositive);
  append_double_entry_(&document, "mask_epsilon", options.mask_epsilon, kRefRangeNonNegative);
  append_double_entry_(
      &document, "standardize_epsilon", options.standardize_epsilon, kRefRangeNonNegative);

  return document;
}

[[nodiscard]] runtime_lls_document_t make_symbolic_runtime_lls_document_(
    const sequence_symbolic_analytics_report_t& report,
    const symbolic_report_key_config_t& keys,
    std::string_view report_label,
    const tsiemene::component_report_identity_t& report_identity) {
  runtime_lls_document_t document{};
  document.entries.reserve(40 + report.streams.size() * 15);
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  append_string_entry_if_nonempty_(&document, keys.label_key, report_label);

  append_u64_entry_(&document, keys.count_key, report.stream_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document,
      keys.eligible_count_key,
      report.eligible_stream_count,
      kRefRangeNonNegative);
  if (keys.emit_reduction_metadata) {
    append_u64_entry_(
        &document,
        "reported_stream_count",
        report.reported_stream_count,
        kRefRangeNonNegative);
    append_u64_entry_(
        &document,
        "omitted_stream_count",
        report.omitted_stream_count,
        kRefRangeNonNegative);
    append_bool_entry_(
        &document, "stream_report_reduced", report.stream_report_reduced);
    append_string_entry_if_nonempty_(
        &document,
        "stream_report_reduction_mode",
        report.stream_report_reduction_mode);
  }
  append_double_entry_(
      &document, "lz76_normalized_mean", report.lz76_normalized_mean, kRefRangeNonNegative);
  append_double_entry_(
      &document, "lz76_normalized_min", report.lz76_normalized_min, kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document, "lz76_normalized_min_label", report.lz76_normalized_min_label);
  append_double_entry_(
      &document, "lz76_normalized_max", report.lz76_normalized_max, kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document, "lz76_normalized_max_label", report.lz76_normalized_max_label);
  append_double_entry_(
      &document,
      "information_density_mean",
      report.information_density_mean,
      kRefRangeUnitInterval);
  append_double_entry_(
      &document,
      "information_density_min",
      report.information_density_min,
      kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document,
      "information_density_min_label",
      report.information_density_min_label);
  append_double_entry_(
      &document,
      "information_density_max",
      report.information_density_max,
      kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document,
      "information_density_max_label",
      report.information_density_max_label);
  append_double_entry_(
      &document, "compression_ratio_mean", report.compression_ratio_mean, kRefRangeNonNegative);
  append_double_entry_(
      &document, "compression_ratio_min", report.compression_ratio_min, kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document, "compression_ratio_min_label", report.compression_ratio_min_label);
  append_double_entry_(
      &document, "compression_ratio_max", report.compression_ratio_max, kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document, "compression_ratio_max_label", report.compression_ratio_max_label);
  append_double_entry_(
      &document,
      "autocorrelation_decay_lag_mean",
      report.autocorrelation_decay_lag_mean,
      kRefRangeNonNegative);
  append_double_entry_(
      &document,
      "autocorrelation_decay_lag_min",
      report.autocorrelation_decay_lag_min,
      kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document,
      "autocorrelation_decay_lag_min_label",
      report.autocorrelation_decay_lag_min_label);
  append_double_entry_(
      &document,
      "autocorrelation_decay_lag_max",
      report.autocorrelation_decay_lag_max,
      kRefRangeNonNegative);
  append_string_entry_if_nonempty_(
      &document,
      "autocorrelation_decay_lag_max_label",
      report.autocorrelation_decay_lag_max_label);
  append_double_entry_(
      &document,
      "power_spectrum_entropy_mean",
      report.power_spectrum_entropy_mean,
      kRefRangeUnitInterval);
  append_double_entry_(
      &document,
      "power_spectrum_entropy_min",
      report.power_spectrum_entropy_min,
      kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document,
      "power_spectrum_entropy_min_label",
      report.power_spectrum_entropy_min_label);
  append_double_entry_(
      &document,
      "power_spectrum_entropy_max",
      report.power_spectrum_entropy_max,
      kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document,
      "power_spectrum_entropy_max_label",
      report.power_spectrum_entropy_max_label);
  append_double_entry_(
      &document, "hurst_exponent_mean", report.hurst_exponent_mean, kRefRangeUnitInterval);
  append_double_entry_(
      &document, "hurst_exponent_min", report.hurst_exponent_min, kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document, "hurst_exponent_min_label", report.hurst_exponent_min_label);
  append_double_entry_(
      &document, "hurst_exponent_max", report.hurst_exponent_max, kRefRangeUnitInterval);
  append_string_entry_if_nonempty_(
      &document, "hurst_exponent_max_label", report.hurst_exponent_max_label);

  for (std::size_t i = 0; i < report.streams.size(); ++i) {
    const std::string prefix =
        std::string(keys.item_prefix) + std::to_string(i + 1) + "_";
    const auto& stream = report.streams[i];
    append_string_entry_if_nonempty_(&document, prefix + "label", stream.label);
    append_string_entry_if_nonempty_(
        &document, prefix + std::string(keys.family_key), stream.stream_family);
    append_string_entry_if_nonempty_(
        &document, prefix + "anchor_feature", stream.anchor_feature);
    append_string_entry_if_nonempty_(
        &document, prefix + "feature_names", stream.feature_names);
    append_bool_entry_(&document, prefix + "eligible", stream.eligible);
    append_u64_entry_(
        &document, prefix + "valid_count", stream.valid_count, kRefRangeNonNegative);
    append_u64_entry_(
        &document,
        prefix + "observed_symbol_count",
        stream.observed_symbol_count,
        kRefRangeNonNegative);
    append_u64_entry_(
        &document,
        prefix + "lz76_complexity",
        stream.lz76_complexity,
        kRefRangeNonNegative);
    append_double_entry_(
        &document,
        prefix + "lz76_normalized",
        stream.lz76_normalized,
        kRefRangeNonNegative);
    append_double_entry_(
        &document,
        prefix + "entropy_rate_bits",
        stream.entropy_rate_bits,
        kRefRangeNonNegative);
    append_double_entry_(
        &document,
        prefix + "information_density",
        stream.information_density,
        kRefRangeUnitInterval);
    append_double_entry_(
        &document,
        prefix + "compression_ratio",
        stream.compression_ratio,
        kRefRangeNonNegative);
    append_double_entry_(
        &document,
        prefix + "autocorrelation_decay_lag",
        stream.autocorrelation_decay_lag,
        kRefRangeNonNegative);
    append_double_entry_(
        &document,
        prefix + "power_spectrum_entropy",
        stream.power_spectrum_entropy,
        kRefRangeUnitInterval);
    append_double_entry_(
        &document,
        prefix + "hurst_exponent",
        stream.hurst_exponent,
        kRefRangeUnitInterval);
  }

  return document;
}

[[nodiscard]] std::string symbolic_analytics_to_pretty_text_(
    const sequence_symbolic_analytics_report_t& report,
    const symbolic_pretty_config_t& cfg,
    std::string_view report_label,
    std::string_view output_filename,
    bool use_color) {
  const char* c_reset = use_color ? "\x1b[0m" : "";
  const char* c_title = use_color ? "\x1b[1;96m" : "";
  const char* c_key = use_color ? "\x1b[90m" : "";
  const char* c_value = use_color ? "\x1b[97m" : "";
  const char* c_note = use_color ? "\x1b[36m" : "";
  const char* c_focus = use_color ? "\x1b[95m" : "";

  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(6);

  auto line = [&](std::string_view key, const auto& value, std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(30) << key << c_reset
        << " : " << c_value << value << c_reset
        << "\t" << c_note << note << c_reset << "\n";
  };

  oss << c_title << cfg.title << c_reset << "\n";
  if (!report_label.empty()) {
    line(cfg.label_key, report_label, cfg.label_note);
  }
  line("schema", report.schema, "report schema id");
  if (!output_filename.empty()) {
    line("file", output_filename, "persisted symbolic report");
  }
  line(cfg.count_key, report.stream_count, cfg.count_note);
  line(
      cfg.eligible_count_key,
      report.eligible_stream_count,
      cfg.eligible_count_note);
  if (cfg.emit_reduction_metadata) {
    line(
        "reported_stream_count",
        report.reported_stream_count,
        "streams serialized in this artifact");
    line(
        "omitted_stream_count",
        report.omitted_stream_count,
        "streams omitted after reduction");
    line(
        "stream_report_reduced",
        report.stream_report_reduced ? "true" : "false",
        "whether the stream list was reduced");
    if (!report.stream_report_reduction_mode.empty()) {
      line(
          "stream_report_reduction_mode",
          report.stream_report_reduction_mode,
          "representative stream selection strategy");
    }
  }
  line(
      "lz76_normalized_mean",
      report.lz76_normalized_mean,
      "mean across eligible streams");
  line(
      "information_density_mean",
      report.information_density_mean,
      "mean across eligible streams");
  line(
      "compression_ratio_mean",
      report.compression_ratio_mean,
      "mean LZW token compression ratio");
  line(
      "autocorrelation_decay_lag_mean",
      report.autocorrelation_decay_lag_mean,
      "mean first lag with |acf| <= exp(-1)");
  line(
      "power_spectrum_entropy_mean",
      report.power_spectrum_entropy_mean,
      "mean normalized positive-frequency entropy");
  line(
      "hurst_exponent_mean",
      report.hurst_exponent_mean,
      "mean R/S H estimate");
  if (cfg.emit_generalized_context_comment) {
    oss << "\t" << c_note
        << "/* generalized stream view: labels may represent latent or derived sequence streams rather than raw source channels */"
        << c_reset << "\n";
  }
  if (cfg.emit_reduction_metadata && report.stream_report_reduced) {
    oss << "\t" << c_note << "/* reduced stream view: showing "
        << report.reported_stream_count << " representative streams out of "
        << report.stream_count
        << "; aggregate metrics still summarize the full stream set */"
        << c_reset << "\n";
    if (!report.stream_report_reduction_mode.empty()) {
      oss << "\t" << c_note << "/* reduction_mode: "
          << report.stream_report_reduction_mode << " */" << c_reset << "\n";
    }
  }

  for (std::size_t i = 0; i < report.streams.size(); ++i) {
    const auto& stream = report.streams[i];
    oss << "\t" << c_note << "/* " << stream.label << " */" << c_reset << "\n";
    oss << "\t" << c_note << "/* features: " << stream.feature_names << " */"
        << c_reset << "\n";
    oss << "\t" << c_note << "/* anchor_feature: " << stream.anchor_feature
        << " */" << c_reset << "\n";

    std::ostringstream key_prefix;
    key_prefix << cfg.item_bracket_key << "[" << (i + 1) << "]";
    line(key_prefix.str() + "." + std::string(cfg.family_key),
         stream.stream_family,
         cfg.family_note);
    line(key_prefix.str() + ".eligible", stream.eligible ? "true" : "false", "metric eligibility");
    line(key_prefix.str() + ".valid_count", stream.valid_count, "valid anchor points");
    line(
        key_prefix.str() + ".observed_symbol_count",
        stream.observed_symbol_count,
        "unique tertile symbols observed");
    line(
        key_prefix.str() + ".lz76_complexity",
        stream.lz76_complexity,
        "raw phrase count");
    oss << "\t" << c_key << std::left << std::setw(30)
        << (key_prefix.str() + ".lz76_normalized") << c_reset << " : "
        << c_focus << stream.lz76_normalized << c_reset << "\t" << c_note
        << "normalized LZ76 complexity" << c_reset << "\n";
    line(
        key_prefix.str() + ".entropy_rate_bits",
        stream.entropy_rate_bits,
        "H(X_t | X_{t-1})");
    line(
        key_prefix.str() + ".information_density",
        stream.information_density,
        "entropy_rate_bits / log2(3)");
    line(
        key_prefix.str() + ".compression_ratio",
        stream.compression_ratio,
        "LZW-coded token bits / raw token bits");
    line(
        key_prefix.str() + ".autocorrelation_decay_lag",
        stream.autocorrelation_decay_lag,
        "first lag with |acf| <= exp(-1)");
    line(
        key_prefix.str() + ".power_spectrum_entropy",
        stream.power_spectrum_entropy,
        "normalized entropy over positive frequency bins");
    line(
        key_prefix.str() + ".hurst_exponent",
        stream.hurst_exponent,
        "R/S dyadic-scale estimate");
  }

  return oss.str();
}

[[nodiscard]] data_analytics_options_t normalize_options_(
    const data_analytics_options_t& in) {
  data_analytics_options_t out = in;
  if (out.max_samples < 1) out.max_samples = 1;
  if (out.max_features < 1) out.max_features = 1;
  if (out.mask_epsilon < 0.0) out.mask_epsilon = 0.0;
  if (out.standardize_epsilon <= 0.0) out.standardize_epsilon = 1e-8;
  return out;
}

[[nodiscard]] inline bool tensor_is_finite_(const torch::Tensor& t) {
  if (!t.defined() || t.numel() == 0) return false;
  try {
    return torch::isfinite(t).all().item<bool>();
  } catch (...) {
    return false;
  }
}

[[nodiscard]] bool normalize_feature_and_mask_(
    const torch::Tensor& features,
    const torch::Tensor& mask,
    torch::Tensor* out_features,
    torch::Tensor* out_mask) {
  if (!out_features || !out_mask) return false;
  out_features->reset();
  out_mask->reset();

  if (!features.defined() || features.numel() == 0) return false;

  torch::Tensor f = features;
  if (f.dim() == 2) {
    f = f.unsqueeze(0).unsqueeze(0);  // [1,1,T,D]
  } else if (f.dim() == 3) {
    f = f.unsqueeze(0);  // [1,C,T,D]
  } else if (f.dim() != 4) {
    return false;
  }
  if (f.dim() != 4 || f.size(0) <= 0 || f.size(1) <= 0 || f.size(2) <= 0 ||
      f.size(3) <= 0) {
    return false;
  }

  torch::Tensor m;
  if (mask.defined() && mask.numel() > 0) {
    m = mask;
    if (m.dim() == 1) {
      m = m.unsqueeze(0).unsqueeze(0);  // [1,1,T]
    } else if (m.dim() == 2) {
      m = m.unsqueeze(0);  // [1,C,T]
    } else if (m.dim() != 3) {
      return false;
    }

    if (m.size(0) != f.size(0) || m.size(1) != f.size(1) || m.size(2) != f.size(2)) {
      return false;
    }

    m = m.to(torch::kCPU, torch::kFloat64);
  } else {
    m = torch::ones(
        {f.size(0), f.size(1), f.size(2)},
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  }

  *out_features = f.to(torch::kCPU, torch::kFloat64);
  *out_mask = m;
  return true;
}

