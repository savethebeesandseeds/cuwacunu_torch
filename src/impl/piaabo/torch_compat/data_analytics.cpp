#include "piaabo/torch_compat/data_analytics.h"

#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

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
  std::string_view token = trim_ascii_ws_view_(contract_hash);
  if (token.size() >= 2 && token[0] == '0' &&
      (token[1] == 'x' || token[1] == 'X')) {
    token.remove_prefix(2);
  }
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
  if (!report_identity.report_kind.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "report_kind", report_identity.report_kind));
  }
  if (!report_identity.tsi_type.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "tsi_type", report_identity.tsi_type));
  }
  if (!report_identity.canonical_path.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "canonical_path", report_identity.canonical_path));
  }
  if (!report_identity.hashimyei.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "hashimyei", report_identity.hashimyei));
  }
  if (!report_identity.contract_hash.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "contract_hash", report_identity.contract_hash));
  }
  if (!report_identity.wave_hash.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "wave_hash", report_identity.wave_hash));
  }
  if (!report_identity.binding_id.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "binding_id", report_identity.binding_id));
  }
  if (!report_identity.run_id.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "run_id", report_identity.run_id));
  }
  if (!report_identity.wave_cursor_resolution.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "wave_cursor_resolution", report_identity.wave_cursor_resolution));
  }
  if (!report_identity.intersection_cursor.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "intersection_cursor", report_identity.intersection_cursor));
  }
  if (report_identity.has_wave_cursor) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave_cursor", report_identity.wave_cursor, "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_run) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.run", report_identity.wave_cursor_run, "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_episode) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.episode",
            report_identity.wave_cursor_episode,
            "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_batch) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.batch", report_identity.wave_cursor_batch, "[0,+inf)"));
  }
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
      "tsi source instance",
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

struct extracted_sequence_rows_t {
  std::vector<torch::Tensor> rows{};
  std::vector<torch::Tensor> validity_masks{};
  std::uint64_t sample_count{0};
  std::uint64_t skipped_sample_count{0};
  std::int64_t channels{0};
  std::int64_t timesteps{0};
  std::int64_t features_per_timestep{0};
  std::int64_t flat_feature_count{0};
  std::int64_t sampled_feature_count{0};
};

[[nodiscard]] torch::Tensor deterministic_feature_subsample_index_(
    std::int64_t feature_count,
    std::int64_t max_features) {
  if (feature_count <= 0 || max_features <= 0) return torch::Tensor{};
  if (feature_count <= max_features) {
    return torch::arange(
        feature_count,
        torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
  }

  if (max_features <= 1) {
    return torch::zeros(
        {1}, torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
  }

  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(max_features));
  const double span = static_cast<double>(feature_count - 1);
  const double denom = static_cast<double>(max_features - 1);
  for (std::int64_t i = 0; i < max_features; ++i) {
    const double pos = (denom > 0.0) ? (span * static_cast<double>(i) / denom)
                                     : 0.0;
    std::int64_t id = static_cast<std::int64_t>(std::llround(pos));
    if (id < 0) id = 0;
    if (id >= feature_count) id = feature_count - 1;
    idx.push_back(id);
  }

  return torch::tensor(
      idx,
      torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
}

[[nodiscard]] torch::Tensor sample_feature_vector_(
    const torch::Tensor& row,
    const torch::Tensor& index) {
  if (!row.defined() || row.dim() != 1 || row.numel() <= 0) return torch::Tensor{};
  if (!index.defined() || index.dim() != 1 || index.numel() <= 0) {
    return torch::Tensor{};
  }
  if (index.size(0) == row.size(0)) return row;
  return row.index_select(/*dim=*/0, index.to(row.device()));
}

[[nodiscard]] bool extract_row_vectors_(
    const torch::Tensor& features,
    const torch::Tensor& mask,
    const data_analytics_options_t& options,
    extracted_sequence_rows_t* out) {
  if (!out) return false;
  *out = extracted_sequence_rows_t{};

  torch::Tensor f;
  torch::Tensor m;
  if (!normalize_feature_and_mask_(features, mask, &f, &m)) {
    return false;
  }
  if (!tensor_is_finite_(f) || !tensor_is_finite_(m)) {
    return false;
  }

  const std::int64_t B = f.size(0);
  const std::int64_t C = f.size(1);
  const std::int64_t T = f.size(2);
  const std::int64_t D = f.size(3);
  const std::int64_t flat_features = C * T * D;
  const torch::Tensor sampled_index =
      deterministic_feature_subsample_index_(flat_features, options.max_features);
  if (!sampled_index.defined() || sampled_index.dim() != 1 ||
      sampled_index.numel() <= 0) {
    return false;
  }

  out->channels = C;
  out->timesteps = T;
  out->features_per_timestep = D;
  out->flat_feature_count = flat_features;
  out->sampled_feature_count = sampled_index.size(0);

  for (std::int64_t b = 0; b < B; ++b) {
    ++out->sample_count;

    torch::Tensor mask_b = m[b];
    torch::Tensor valid_b = mask_b > 0.0;
    double accepted_timestep_ratio = 0.0;
    try {
      accepted_timestep_ratio = valid_b.to(torch::kFloat64).mean().item<double>();
    } catch (...) {
      accepted_timestep_ratio = 0.0;
    }

    if (!std::isfinite(accepted_timestep_ratio) ||
        accepted_timestep_ratio <= options.mask_epsilon) {
      ++out->skipped_sample_count;
      continue;
    }

    torch::Tensor row = f[b].reshape({flat_features});
    torch::Tensor valid_row =
        valid_b.to(torch::kFloat64)
            .unsqueeze(-1)
            .expand({C, T, D})
            .reshape({flat_features});
    row = sample_feature_vector_(row, sampled_index);
    valid_row = sample_feature_vector_(valid_row, sampled_index);
    if (!row.defined() || row.dim() != 1 || row.numel() <= 0) {
      ++out->skipped_sample_count;
      continue;
    }
    if (!valid_row.defined() || valid_row.dim() != 1 ||
        valid_row.size(0) != row.size(0)) {
      ++out->skipped_sample_count;
      continue;
    }
    if (!tensor_is_finite_(row) || !tensor_is_finite_(valid_row)) {
      ++out->skipped_sample_count;
      continue;
    }
    const double sampled_valid_mass = valid_row.sum().item<double>();
    if (!std::isfinite(sampled_valid_mass) || sampled_valid_mass <= kNumericEpsilon) {
      ++out->skipped_sample_count;
      continue;
    }

    if (static_cast<std::int64_t>(out->rows.size()) >= options.max_samples) {
      ++out->skipped_sample_count;
      continue;
    }

    out->rows.push_back(std::move(row));
    out->validity_masks.push_back(std::move(valid_row));
  }

  return true;
}

[[nodiscard]] std::optional<double> parse_double_strict_(std::string_view s) {
  std::string text(trim_ascii_ws_view_(s));
  if (text.empty()) return std::nullopt;
  try {
    std::size_t pos = 0;
    const double v = std::stod(text, &pos);
    if (pos != text.size()) return std::nullopt;
    return v;
  } catch (...) {
    return std::nullopt;
  }
}

[[nodiscard]] bool write_text_file_atomically_(
    const std::string& payload,
    const std::filesystem::path& output_file,
    std::string_view artifact_name,
    std::string* error) {
  if (error) error->clear();

  std::error_code ec;
  const auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) {
        *error = "cannot create " + std::string(artifact_name) +
                 " directory: " + parent.string();
      }
      return false;
    }
  }

  static std::atomic<std::uint64_t> s_temp_counter{0};
  const auto temp_suffix =
      std::to_string(static_cast<std::uint64_t>(
                         std::chrono::steady_clock::now()
                             .time_since_epoch()
                             .count())) +
      "." + std::to_string(s_temp_counter.fetch_add(1, std::memory_order_relaxed));
  const std::filesystem::path temp_file =
      output_file.string() + ".tmp." + temp_suffix;

  {
    std::ofstream out(temp_file, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) {
        *error = "cannot open temporary " + std::string(artifact_name) +
                 " file: " + temp_file.string();
      }
      return false;
    }
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    out.close();
    if (!out) {
      std::error_code remove_ec;
      std::filesystem::remove(temp_file, remove_ec);
      if (error) {
        *error = "cannot write temporary " + std::string(artifact_name) +
                 " file: " + temp_file.string();
      }
      return false;
    }
  }

  std::filesystem::rename(temp_file, output_file, ec);
  if (ec) {
    std::error_code remove_ec;
    std::filesystem::remove(temp_file, remove_ec);
    if (error) {
      *error = "cannot replace " + std::string(artifact_name) +
               " file: " + output_file.string();
    }
    return false;
  }
  return true;
}

}  // namespace

sequence_analytics_accumulator_t::sequence_analytics_accumulator_t(
    data_analytics_options_t options)
    : options_(normalize_options_(options)) {}

void sequence_analytics_accumulator_t::reset() {
  rows_.clear();
  row_validity_masks_.clear();
  sample_count_ = 0;
  skipped_sample_count_ = 0;
  sequence_channels_ = 0;
  sequence_timesteps_ = 0;
  sequence_features_per_timestep_ = 0;
  sequence_flat_feature_count_ = 0;
  sequence_sampled_feature_count_ = 0;
}

bool sequence_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  extracted_sequence_rows_t extracted{};
  if (!extract_row_vectors_(features, mask, options_, &extracted)) {
    ++sample_count_;
    ++skipped_sample_count_;
    return false;
  }

  sample_count_ += extracted.sample_count;
  skipped_sample_count_ += extracted.skipped_sample_count;

  const bool geometry_locked = sequence_flat_feature_count_ > 0;
  const bool geometry_mismatch =
      geometry_locked &&
      (sequence_channels_ != extracted.channels ||
       sequence_timesteps_ != extracted.timesteps ||
       sequence_features_per_timestep_ != extracted.features_per_timestep ||
       sequence_flat_feature_count_ != extracted.flat_feature_count ||
       sequence_sampled_feature_count_ != extracted.sampled_feature_count);
  if (geometry_mismatch) {
    skipped_sample_count_ +=
        static_cast<std::uint64_t>(extracted.rows.size());
    return false;
  }

  if (!geometry_locked && !extracted.rows.empty()) {
    sequence_channels_ = extracted.channels;
    sequence_timesteps_ = extracted.timesteps;
    sequence_features_per_timestep_ = extracted.features_per_timestep;
    sequence_flat_feature_count_ = extracted.flat_feature_count;
    sequence_sampled_feature_count_ = extracted.sampled_feature_count;
  }

  if (!extracted.rows.empty()) {
    rows_.insert(
        rows_.end(),
        std::make_move_iterator(extracted.rows.begin()),
        std::make_move_iterator(extracted.rows.end()));
    row_validity_masks_.insert(
        row_validity_masks_.end(),
        std::make_move_iterator(extracted.validity_masks.begin()),
        std::make_move_iterator(extracted.validity_masks.end()));
  }

  return true;
}

sequence_analytics_report_t sequence_analytics_accumulator_t::summarize()
    const {
  sequence_analytics_report_t out{};
  out.sample_count = sample_count_;
  out.skipped_sample_count = skipped_sample_count_;
  out.valid_sample_count = static_cast<std::uint64_t>(rows_.size());
  out.sequence_channels = sequence_channels_;
  out.sequence_timesteps = sequence_timesteps_;
  out.sequence_features_per_timestep = sequence_features_per_timestep_;
  out.sequence_flat_feature_count = sequence_flat_feature_count_;
  out.sequence_effective_feature_count = 0;

  if (rows_.empty()) return out;
  if (rows_.size() != row_validity_masks_.size()) return out;

  torch::Tensor X;
  torch::Tensor V;
  try {
    X = torch::stack(rows_, /*dim=*/0);  // [N,F]
    V = torch::stack(row_validity_masks_, /*dim=*/0);  // [N,F]
  } catch (...) {
    return out;
  }

  if (!X.defined() || X.dim() != 2 || X.size(0) <= 0 || X.size(1) <= 0) {
    return out;
  }
  if (!V.defined() || V.dim() != 2 || V.sizes() != X.sizes()) {
    return out;
  }

  try {
    V = V.clamp(0.0, 1.0);
    torch::Tensor valid_mass = V.sum(/*dim=*/0);  // [F]
    const torch::Tensor active = valid_mass > kNumericEpsilon;
    const std::int64_t active_feature_count =
        active.sum().item<std::int64_t>();
    out.sequence_effective_feature_count = active_feature_count;
    if (active_feature_count <= 0) return out;

    const torch::Tensor active_index =
        torch::nonzero(active).reshape({active_feature_count});
    X = X.index_select(/*dim=*/1, active_index);
    V = V.index_select(/*dim=*/1, active_index);
    valid_mass = valid_mass.index_select(/*dim=*/0, active_index);

    const torch::Tensor mean = (X * V).sum(/*dim=*/0) / valid_mass;
    const torch::Tensor centered = (X - mean) * V;
    const torch::Tensor var =
        (centered * centered).sum(/*dim=*/0) / valid_mass;
    const torch::Tensor stdev =
        torch::sqrt(var.clamp_min(options_.standardize_epsilon));
    const torch::Tensor Z = centered / stdev;

    torch::Tensor support = V.transpose(0, 1).mm(V);
    support = support.clamp_min(1.0);
    const torch::Tensor cov = Z.transpose(0, 1).mm(Z) / support;

    torch::Tensor eigvals = torch::linalg::eigvalsh(cov, "L");
    eigvals = eigvals.clamp_min(0.0);

    const double trace = eigvals.sum().item<double>();
    out.sequence_cov_trace = std::isfinite(trace) ? trace : 0.0;

    if (!std::isfinite(trace) || trace <= kNumericEpsilon) return out;

    const torch::Tensor probs = eigvals / trace;
    const torch::Tensor probs_safe = probs.clamp_min(kNumericEpsilon);
    const double entropy =
        (-probs * torch::log(probs_safe)).sum().item<double>();

    if (std::isfinite(entropy)) {
      out.sequence_entropic_load = std::exp(entropy);
    }

    out.sequence_nonzero_eigen_count = static_cast<std::uint64_t>(
        (eigvals > options_.standardize_epsilon).sum().item<std::int64_t>());
  } catch (...) {
    return out;
  }

  return out;
}

data_source_analytics_accumulator_t::data_source_analytics_accumulator_t(
    data_analytics_options_t options)
    : core_(options) {}

void data_source_analytics_accumulator_t::reset() { core_.reset(); }

bool data_source_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  return core_.ingest(features, mask);
}

data_source_analytics_report_t data_source_analytics_accumulator_t::summarize()
    const {
  return make_data_source_analytics_report(core_.summarize());
}

sequence_symbolic_analytics_accumulator_t::sequence_symbolic_analytics_accumulator_t(
    std::vector<sequence_symbolic_stream_descriptor_t> stream_descriptors)
    : descriptors_(std::move(stream_descriptors)) {
  reset();
}

void sequence_symbolic_analytics_accumulator_t::reset() {
  stream_series_.assign(descriptors_.size(), {});
}

bool sequence_symbolic_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  torch::Tensor f;
  torch::Tensor m;
  if (!normalize_feature_and_mask_(features, mask, &f, &m)) {
    return false;
  }
  if (!tensor_is_finite_(f) || !tensor_is_finite_(m)) {
    return false;
  }
  if (f.size(1) != static_cast<std::int64_t>(descriptors_.size())) {
    return false;
  }

  const std::int64_t B = f.size(0);
  const std::int64_t C = f.size(1);
  const std::int64_t T = f.size(2);
  const std::int64_t D = f.size(3);
  if (T <= 0 || D <= 0) return false;

  for (std::int64_t b = 0; b < B; ++b) {
    for (std::int64_t c = 0; c < C; ++c) {
      const auto& descriptor = descriptors_[static_cast<std::size_t>(c)];
      if (descriptor.anchor_feature_index < 0 ||
          descriptor.anchor_feature_index >= D) {
        continue;
      }

      std::int64_t anchor_timestep = -1;
      for (std::int64_t t = T - 1; t >= 0; --t) {
        const double mask_value = m.index({b, c, t}).item<double>();
        if (std::isfinite(mask_value) && mask_value > 0.0) {
          anchor_timestep = t;
          break;
        }
      }
      if (anchor_timestep < 0) continue;

      // Symbolic heuristics operate on the anchor series induced by ingest order.
      const double anchor_value =
          f.index({b, c, anchor_timestep, descriptor.anchor_feature_index})
              .item<double>();
      if (!std::isfinite(anchor_value)) continue;

      stream_series_[static_cast<std::size_t>(c)].push_back(anchor_value);
    }
  }
  return true;
}

sequence_symbolic_analytics_report_t
sequence_symbolic_analytics_accumulator_t::summarize() const {
  sequence_symbolic_analytics_report_t out{};
  out.stream_count = static_cast<std::uint64_t>(descriptors_.size());
  out.reported_stream_count = out.stream_count;
  out.omitted_stream_count = 0;
  out.stream_report_reduced = false;
  out.streams.reserve(descriptors_.size());

  double lz_sum = 0.0;
  double info_sum = 0.0;
  double compression_sum = 0.0;
  double autocorr_decay_sum = 0.0;
  double spectrum_entropy_sum = 0.0;
  double hurst_sum = 0.0;
  bool have_eligible = false;

  for (std::size_t i = 0; i < descriptors_.size(); ++i) {
    const auto& descriptor = descriptors_[i];
    const auto& series = stream_series_[i];

    sequence_symbolic_stream_report_t stream{};
    stream.label = descriptor.label;
    stream.stream_family = descriptor.stream_family;
    stream.anchor_feature = descriptor.anchor_feature;
    stream.feature_names = descriptor.feature_names;
    stream.valid_count = static_cast<std::uint64_t>(series.size());

    const std::vector<std::uint8_t> tokens = symbolize_ternary_quantiles_(series);
    stream.observed_symbol_count = observed_symbol_count_(tokens);
    stream.eligible = stream.valid_count >= kSymbolicMinEligibleSamples;

    if (stream.eligible) {
      stream.lz76_complexity = lz76_complexity_(tokens);
      stream.lz76_normalized = clamp_nonneg(
          normalized_lz76_complexity_(stream.lz76_complexity, tokens.size()));
      stream.entropy_rate_bits =
          clamp_nonneg(entropy_rate_bits_from_bigrams_(tokens));
      const double density =
          stream.entropy_rate_bits /
          std::log2(static_cast<double>(kSymbolicAlphabetSize));
      if (std::isfinite(density)) {
        stream.information_density = std::clamp(density, 0.0, 1.0);
      }
      stream.compression_ratio = clamp_nonneg(compression_ratio_lzw_(tokens));
      stream.autocorrelation_decay_lag =
          clamp_nonneg(autocorrelation_decay_lag_(series));
      stream.power_spectrum_entropy =
          std::clamp(power_spectrum_entropy_(series), 0.0, 1.0);
      stream.hurst_exponent =
          std::clamp(hurst_exponent_rs_(series), 0.0, 1.0);

      lz_sum += stream.lz76_normalized;
      info_sum += stream.information_density;
      compression_sum += stream.compression_ratio;
      autocorr_decay_sum += stream.autocorrelation_decay_lag;
      spectrum_entropy_sum += stream.power_spectrum_entropy;
      hurst_sum += stream.hurst_exponent;
      ++out.eligible_stream_count;

      if (!have_eligible) {
        out.lz76_normalized_min = stream.lz76_normalized;
        out.lz76_normalized_max = stream.lz76_normalized;
        out.lz76_normalized_min_label = stream.label;
        out.lz76_normalized_max_label = stream.label;
        out.information_density_min = stream.information_density;
        out.information_density_max = stream.information_density;
        out.information_density_min_label = stream.label;
        out.information_density_max_label = stream.label;
        out.compression_ratio_min = stream.compression_ratio;
        out.compression_ratio_max = stream.compression_ratio;
        out.compression_ratio_min_label = stream.label;
        out.compression_ratio_max_label = stream.label;
        out.autocorrelation_decay_lag_min = stream.autocorrelation_decay_lag;
        out.autocorrelation_decay_lag_max = stream.autocorrelation_decay_lag;
        out.autocorrelation_decay_lag_min_label = stream.label;
        out.autocorrelation_decay_lag_max_label = stream.label;
        out.power_spectrum_entropy_min = stream.power_spectrum_entropy;
        out.power_spectrum_entropy_max = stream.power_spectrum_entropy;
        out.power_spectrum_entropy_min_label = stream.label;
        out.power_spectrum_entropy_max_label = stream.label;
        out.hurst_exponent_min = stream.hurst_exponent;
        out.hurst_exponent_max = stream.hurst_exponent;
        out.hurst_exponent_min_label = stream.label;
        out.hurst_exponent_max_label = stream.label;
        have_eligible = true;
      } else {
        if (stream.lz76_normalized < out.lz76_normalized_min) {
          out.lz76_normalized_min = stream.lz76_normalized;
          out.lz76_normalized_min_label = stream.label;
        }
        if (stream.lz76_normalized > out.lz76_normalized_max) {
          out.lz76_normalized_max = stream.lz76_normalized;
          out.lz76_normalized_max_label = stream.label;
        }
        if (stream.information_density < out.information_density_min) {
          out.information_density_min = stream.information_density;
          out.information_density_min_label = stream.label;
        }
        if (stream.information_density > out.information_density_max) {
          out.information_density_max = stream.information_density;
          out.information_density_max_label = stream.label;
        }
        if (stream.compression_ratio < out.compression_ratio_min) {
          out.compression_ratio_min = stream.compression_ratio;
          out.compression_ratio_min_label = stream.label;
        }
        if (stream.compression_ratio > out.compression_ratio_max) {
          out.compression_ratio_max = stream.compression_ratio;
          out.compression_ratio_max_label = stream.label;
        }
        if (stream.autocorrelation_decay_lag <
            out.autocorrelation_decay_lag_min) {
          out.autocorrelation_decay_lag_min = stream.autocorrelation_decay_lag;
          out.autocorrelation_decay_lag_min_label = stream.label;
        }
        if (stream.autocorrelation_decay_lag >
            out.autocorrelation_decay_lag_max) {
          out.autocorrelation_decay_lag_max = stream.autocorrelation_decay_lag;
          out.autocorrelation_decay_lag_max_label = stream.label;
        }
        if (stream.power_spectrum_entropy < out.power_spectrum_entropy_min) {
          out.power_spectrum_entropy_min = stream.power_spectrum_entropy;
          out.power_spectrum_entropy_min_label = stream.label;
        }
        if (stream.power_spectrum_entropy > out.power_spectrum_entropy_max) {
          out.power_spectrum_entropy_max = stream.power_spectrum_entropy;
          out.power_spectrum_entropy_max_label = stream.label;
        }
        if (stream.hurst_exponent < out.hurst_exponent_min) {
          out.hurst_exponent_min = stream.hurst_exponent;
          out.hurst_exponent_min_label = stream.label;
        }
        if (stream.hurst_exponent > out.hurst_exponent_max) {
          out.hurst_exponent_max = stream.hurst_exponent;
          out.hurst_exponent_max_label = stream.label;
        }
      }
    }

    out.streams.push_back(std::move(stream));
  }

  if (out.eligible_stream_count > 0) {
    const double denom = static_cast<double>(out.eligible_stream_count);
    out.lz76_normalized_mean = lz_sum / denom;
    out.information_density_mean = info_sum / denom;
    out.compression_ratio_mean = compression_sum / denom;
    out.autocorrelation_decay_lag_mean = autocorr_decay_sum / denom;
    out.power_spectrum_entropy_mean = spectrum_entropy_sum / denom;
    out.hurst_exponent_mean = hurst_sum / denom;
  }

  return out;
}

data_symbolic_analytics_accumulator_t::data_symbolic_analytics_accumulator_t(
    std::vector<data_symbolic_channel_descriptor_t> channel_descriptors)
    : descriptors_(std::move(channel_descriptors)) {
  std::vector<sequence_symbolic_stream_descriptor_t> sequence_descriptors;
  sequence_descriptors.reserve(descriptors_.size());
  for (const auto& descriptor : descriptors_) {
    sequence_descriptors.push_back(
        make_sequence_symbolic_stream_descriptor(descriptor));
  }
  core_ = sequence_symbolic_analytics_accumulator_t(
      std::move(sequence_descriptors));
}

void data_symbolic_analytics_accumulator_t::reset() { core_.reset(); }

bool data_symbolic_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  return core_.ingest(features, mask);
}

data_symbolic_analytics_report_t data_symbolic_analytics_accumulator_t::summarize()
    const {
  return make_data_symbolic_analytics_report(core_.summarize());
}

sequence_analytics_report_t make_sequence_analytics_report(
    const data_source_analytics_report_t& report) {
  sequence_analytics_report_t out{};
  out.sample_count = report.sample_count;
  out.valid_sample_count = report.valid_sample_count;
  out.skipped_sample_count = report.skipped_sample_count;
  out.sequence_channels = report.source_channels;
  out.sequence_timesteps = report.source_timesteps;
  out.sequence_features_per_timestep = report.source_features_per_timestep;
  out.sequence_flat_feature_count = report.source_flat_feature_count;
  out.sequence_effective_feature_count = report.source_effective_feature_count;
  out.sequence_entropic_load = report.source_entropic_load;
  out.sequence_cov_trace = report.source_cov_trace;
  out.sequence_nonzero_eigen_count = report.source_nonzero_eigen_count;
  return out;
}

data_source_analytics_report_t make_data_source_analytics_report(
    const sequence_analytics_report_t& report) {
  data_source_analytics_report_t out{};
  out.sample_count = report.sample_count;
  out.valid_sample_count = report.valid_sample_count;
  out.skipped_sample_count = report.skipped_sample_count;
  out.source_channels = report.sequence_channels;
  out.source_timesteps = report.sequence_timesteps;
  out.source_features_per_timestep = report.sequence_features_per_timestep;
  out.source_flat_feature_count = report.sequence_flat_feature_count;
  out.source_effective_feature_count = report.sequence_effective_feature_count;
  out.source_entropic_load = report.sequence_entropic_load;
  out.source_cov_trace = report.sequence_cov_trace;
  out.source_nonzero_eigen_count = report.sequence_nonzero_eigen_count;
  return out;
}

sequence_symbolic_stream_descriptor_t make_sequence_symbolic_stream_descriptor(
    const data_symbolic_channel_descriptor_t& descriptor) {
  sequence_symbolic_stream_descriptor_t out{};
  out.label = descriptor.label;
  out.stream_family = descriptor.record_type;
  out.anchor_feature = descriptor.anchor_feature;
  out.feature_names = descriptor.feature_names;
  out.anchor_feature_index = descriptor.anchor_feature_index;
  return out;
}

data_symbolic_channel_descriptor_t make_data_symbolic_channel_descriptor(
    const sequence_symbolic_stream_descriptor_t& descriptor) {
  data_symbolic_channel_descriptor_t out{};
  out.label = descriptor.label;
  out.record_type = descriptor.stream_family;
  out.anchor_feature = descriptor.anchor_feature;
  out.feature_names = descriptor.feature_names;
  out.anchor_feature_index = descriptor.anchor_feature_index;
  return out;
}

sequence_symbolic_analytics_report_t make_sequence_symbolic_analytics_report(
    const data_symbolic_analytics_report_t& report) {
  sequence_symbolic_analytics_report_t out{};
  out.stream_count = report.channel_count;
  out.eligible_stream_count = report.eligible_channel_count;
  out.reported_stream_count = report.channel_count;
  out.omitted_stream_count = 0;
  out.stream_report_reduced = false;
  out.lz76_normalized_mean = report.lz76_normalized_mean;
  out.lz76_normalized_min = report.lz76_normalized_min;
  out.lz76_normalized_min_label = report.lz76_normalized_min_label;
  out.lz76_normalized_max = report.lz76_normalized_max;
  out.lz76_normalized_max_label = report.lz76_normalized_max_label;
  out.information_density_mean = report.information_density_mean;
  out.information_density_min = report.information_density_min;
  out.information_density_min_label = report.information_density_min_label;
  out.information_density_max = report.information_density_max;
  out.information_density_max_label = report.information_density_max_label;
  out.compression_ratio_mean = report.compression_ratio_mean;
  out.compression_ratio_min = report.compression_ratio_min;
  out.compression_ratio_min_label = report.compression_ratio_min_label;
  out.compression_ratio_max = report.compression_ratio_max;
  out.compression_ratio_max_label = report.compression_ratio_max_label;
  out.autocorrelation_decay_lag_mean = report.autocorrelation_decay_lag_mean;
  out.autocorrelation_decay_lag_min = report.autocorrelation_decay_lag_min;
  out.autocorrelation_decay_lag_min_label =
      report.autocorrelation_decay_lag_min_label;
  out.autocorrelation_decay_lag_max = report.autocorrelation_decay_lag_max;
  out.autocorrelation_decay_lag_max_label =
      report.autocorrelation_decay_lag_max_label;
  out.power_spectrum_entropy_mean = report.power_spectrum_entropy_mean;
  out.power_spectrum_entropy_min = report.power_spectrum_entropy_min;
  out.power_spectrum_entropy_min_label =
      report.power_spectrum_entropy_min_label;
  out.power_spectrum_entropy_max = report.power_spectrum_entropy_max;
  out.power_spectrum_entropy_max_label =
      report.power_spectrum_entropy_max_label;
  out.hurst_exponent_mean = report.hurst_exponent_mean;
  out.hurst_exponent_min = report.hurst_exponent_min;
  out.hurst_exponent_min_label = report.hurst_exponent_min_label;
  out.hurst_exponent_max = report.hurst_exponent_max;
  out.hurst_exponent_max_label = report.hurst_exponent_max_label;
  out.streams.reserve(report.channels.size());
  for (const auto& channel : report.channels) {
    sequence_symbolic_stream_report_t stream{};
    stream.label = channel.label;
    stream.stream_family = channel.record_type;
    stream.anchor_feature = channel.anchor_feature;
    stream.feature_names = channel.feature_names;
    stream.valid_count = channel.valid_count;
    stream.observed_symbol_count = channel.observed_symbol_count;
    stream.eligible = channel.eligible;
    stream.lz76_complexity = channel.lz76_complexity;
    stream.lz76_normalized = channel.lz76_normalized;
    stream.entropy_rate_bits = channel.entropy_rate_bits;
    stream.information_density = channel.information_density;
    stream.compression_ratio = channel.compression_ratio;
    stream.autocorrelation_decay_lag = channel.autocorrelation_decay_lag;
    stream.power_spectrum_entropy = channel.power_spectrum_entropy;
    stream.hurst_exponent = channel.hurst_exponent;
    out.streams.push_back(std::move(stream));
  }
  return out;
}

sequence_symbolic_analytics_report_t compact_sequence_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report,
    sequence_symbolic_report_compaction_options_t options) {
  auto normalized = normalize_sequence_symbolic_report_compaction_options_(options);
  if (report.streams.empty()) return report;
  if (report.stream_report_reduced) return report;
  if (normalized.max_reported_streams <= 0) return report;
  if (report.stream_count <=
      static_cast<std::uint64_t>(normalized.reduce_when_stream_count_exceeds)) {
    return report;
  }
  if (report.streams.size() <=
      static_cast<std::size_t>(normalized.max_reported_streams)) {
    return report;
  }

  sequence_symbolic_analytics_report_t out = report;
  const std::size_t max_reported =
      std::min<std::size_t>(
          static_cast<std::size_t>(normalized.max_reported_streams),
          report.streams.size());

  std::vector<std::size_t> selected;
  selected.reserve(max_reported);
  add_stream_index_for_label_(report, report.lz76_normalized_min_label, &selected);
  add_stream_index_for_label_(report, report.lz76_normalized_max_label, &selected);
  add_stream_index_for_label_(report, report.information_density_min_label, &selected);
  add_stream_index_for_label_(report, report.information_density_max_label, &selected);
  add_stream_index_for_label_(report, report.compression_ratio_min_label, &selected);
  add_stream_index_for_label_(report, report.compression_ratio_max_label, &selected);
  add_stream_index_for_label_(
      report, report.autocorrelation_decay_lag_min_label, &selected);
  add_stream_index_for_label_(
      report, report.autocorrelation_decay_lag_max_label, &selected);
  add_stream_index_for_label_(
      report, report.power_spectrum_entropy_min_label, &selected);
  add_stream_index_for_label_(
      report, report.power_spectrum_entropy_max_label, &selected);
  add_stream_index_for_label_(report, report.hurst_exponent_min_label, &selected);
  add_stream_index_for_label_(report, report.hurst_exponent_max_label, &selected);

  if (selected.size() > max_reported) {
    selected.resize(max_reported);
  }
  fill_remaining_stream_indices_evenly_(report, max_reported, &selected);
  if (selected.size() > max_reported) {
    selected.resize(max_reported);
  }
  std::sort(selected.begin(), selected.end());
  selected.erase(std::unique(selected.begin(), selected.end()), selected.end());

  if (selected.size() < max_reported) {
    fill_remaining_stream_indices_evenly_(report, max_reported, &selected);
    std::sort(selected.begin(), selected.end());
    selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
  }

  out.streams.clear();
  out.streams.reserve(selected.size());
  for (std::size_t index : selected) {
    out.streams.push_back(report.streams[index]);
  }
  out.reported_stream_count = static_cast<std::uint64_t>(out.streams.size());
  out.omitted_stream_count =
      (out.stream_count > out.reported_stream_count)
          ? (out.stream_count - out.reported_stream_count)
          : 0;
  out.stream_report_reduced = out.omitted_stream_count > 0;
  if (out.stream_report_reduced) {
    out.stream_report_reduction_mode = "extrema_plus_even_coverage";
  } else {
    out.stream_report_reduction_mode.clear();
  }
  return out;
}

data_symbolic_analytics_report_t make_data_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report) {
  data_symbolic_analytics_report_t out{};
  out.channel_count = report.stream_count;
  out.eligible_channel_count = report.eligible_stream_count;
  out.lz76_normalized_mean = report.lz76_normalized_mean;
  out.lz76_normalized_min = report.lz76_normalized_min;
  out.lz76_normalized_min_label = report.lz76_normalized_min_label;
  out.lz76_normalized_max = report.lz76_normalized_max;
  out.lz76_normalized_max_label = report.lz76_normalized_max_label;
  out.information_density_mean = report.information_density_mean;
  out.information_density_min = report.information_density_min;
  out.information_density_min_label = report.information_density_min_label;
  out.information_density_max = report.information_density_max;
  out.information_density_max_label = report.information_density_max_label;
  out.compression_ratio_mean = report.compression_ratio_mean;
  out.compression_ratio_min = report.compression_ratio_min;
  out.compression_ratio_min_label = report.compression_ratio_min_label;
  out.compression_ratio_max = report.compression_ratio_max;
  out.compression_ratio_max_label = report.compression_ratio_max_label;
  out.autocorrelation_decay_lag_mean = report.autocorrelation_decay_lag_mean;
  out.autocorrelation_decay_lag_min = report.autocorrelation_decay_lag_min;
  out.autocorrelation_decay_lag_min_label =
      report.autocorrelation_decay_lag_min_label;
  out.autocorrelation_decay_lag_max = report.autocorrelation_decay_lag_max;
  out.autocorrelation_decay_lag_max_label =
      report.autocorrelation_decay_lag_max_label;
  out.power_spectrum_entropy_mean = report.power_spectrum_entropy_mean;
  out.power_spectrum_entropy_min = report.power_spectrum_entropy_min;
  out.power_spectrum_entropy_min_label =
      report.power_spectrum_entropy_min_label;
  out.power_spectrum_entropy_max = report.power_spectrum_entropy_max;
  out.power_spectrum_entropy_max_label =
      report.power_spectrum_entropy_max_label;
  out.hurst_exponent_mean = report.hurst_exponent_mean;
  out.hurst_exponent_min = report.hurst_exponent_min;
  out.hurst_exponent_min_label = report.hurst_exponent_min_label;
  out.hurst_exponent_max = report.hurst_exponent_max;
  out.hurst_exponent_max_label = report.hurst_exponent_max_label;
  out.channels.reserve(report.streams.size());
  for (const auto& stream : report.streams) {
    data_symbolic_channel_report_t channel{};
    channel.label = stream.label;
    channel.record_type = stream.stream_family;
    channel.anchor_feature = stream.anchor_feature;
    channel.feature_names = stream.feature_names;
    channel.valid_count = stream.valid_count;
    channel.observed_symbol_count = stream.observed_symbol_count;
    channel.eligible = stream.eligible;
    channel.lz76_complexity = stream.lz76_complexity;
    channel.lz76_normalized = stream.lz76_normalized;
    channel.entropy_rate_bits = stream.entropy_rate_bits;
    channel.information_density = stream.information_density;
    channel.compression_ratio = stream.compression_ratio;
    channel.autocorrelation_decay_lag = stream.autocorrelation_decay_lag;
    channel.power_spectrum_entropy = stream.power_spectrum_entropy;
    channel.hurst_exponent = stream.hurst_exponent;
    out.channels.push_back(std::move(channel));
  }
  return out;
}

const std::vector<std::string>& data_feature_names_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return kline_feature_names_();
  if (record_type == "trade") return trade_feature_names_();
  if (record_type == "basic") return basic_feature_names_();
  return empty_feature_names_();
}

std::string_view data_symbolic_anchor_feature_name_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return "close_price";
  if (record_type == "trade") return "price";
  if (record_type == "basic") return "value";
  return {};
}

std::int64_t data_symbolic_anchor_feature_index_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return 3;
  if (record_type == "trade") return 0;
  if (record_type == "basic") return 0;
  return -1;
}

std::string sequence_analytics_to_latent_lineage_state_text(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view sequence_label,
    const tsiemene::component_report_identity_t& report_identity) {
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_sequence_runtime_lls_document_(
          report,
          options,
          generic_sequence_report_keys_(),
          sequence_label,
          report_identity));
}

std::string data_analytics_to_latent_lineage_state_text(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view source_label,
    const tsiemene::component_report_identity_t& report_identity) {
  auto sequence_report = make_sequence_analytics_report(report);
  sequence_report.schema = report.schema;
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_sequence_runtime_lls_document_(
          sequence_report,
          options,
          source_sequence_report_keys_(),
          source_label,
          report_identity));
}

std::string sequence_symbolic_analytics_to_latent_lineage_state_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label,
    const tsiemene::component_report_identity_t& report_identity,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  const auto compacted =
      compact_sequence_symbolic_analytics_report(report, compaction_options);
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_symbolic_runtime_lls_document_(
          compacted,
          generic_symbolic_report_keys_(),
          sequence_label,
          report_identity));
}

std::string data_symbolic_analytics_to_latent_lineage_state_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label,
    const tsiemene::component_report_identity_t& report_identity) {
  auto sequence_report = make_sequence_symbolic_analytics_report(report);
  sequence_report.schema = report.schema;
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_symbolic_runtime_lls_document_(
          sequence_report,
          source_symbolic_report_keys_(),
          source_label,
          report_identity));
}

std::string sequence_symbolic_analytics_to_pretty_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label,
    std::string_view output_filename,
    bool use_color,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  const auto compacted =
      compact_sequence_symbolic_analytics_report(report, compaction_options);
  return symbolic_analytics_to_pretty_text_(
      compacted,
      generic_symbolic_pretty_config_(),
      sequence_label,
      output_filename,
      use_color);
}

std::string data_symbolic_analytics_to_pretty_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label,
    std::string_view output_filename,
    bool use_color) {
  auto sequence_report = make_sequence_symbolic_analytics_report(report);
  sequence_report.schema = report.schema;
  return symbolic_analytics_to_pretty_text_(
      sequence_report,
      source_symbolic_pretty_config_(),
      source_label,
      output_filename,
      use_color);
}

bool write_sequence_analytics_file(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view sequence_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = sequence_analytics_to_latent_lineage_state_text(
        report, options, sequence_label, report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize sequence analytics report: " + std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "sequence analytics", error);
}

bool write_data_analytics_file(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view source_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = data_analytics_to_latent_lineage_state_text(
        report, options, source_label, report_identity);
  } catch (const std::exception& e) {
    if (error) *error = "cannot serialize data analytics report: " + std::string(e.what());
    return false;
  }
  return write_text_file_atomically_(payload, output_file, "data analytics", error);
}

bool write_sequence_symbolic_analytics_file(
    const sequence_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view sequence_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = sequence_symbolic_analytics_to_latent_lineage_state_text(
        report, sequence_label, report_identity, compaction_options);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize sequence symbolic analytics report: " +
          std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "sequence symbolic analytics", error);
}

bool write_data_symbolic_analytics_file(
    const data_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view source_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = data_symbolic_analytics_to_latent_lineage_state_text(
        report, source_label, report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize symbolic data analytics report: " +
          std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "symbolic data analytics", error);
}

std::string extract_data_analytics_kv_schema(std::string_view payload) {
  runtime_lls_document_t document{};
  std::string parse_error;
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, &parse_error)) {
    return {};
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, &parse_error)) {
    return {};
  }
  if (const auto it = kv.find("schema"); it != kv.end()) return it->second;
  return {};
}

std::string extract_sequence_analytics_kv_schema(std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

std::string extract_sequence_symbolic_analytics_kv_schema(
    std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

std::string extract_data_symbolic_analytics_kv_schema(std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

bool is_supported_sequence_analytics_schema(std::string_view schema) {
  return schema == kSequenceAnalyticsSchemaCurrent ||
         schema == kEmbeddingSequenceAnalyticsSchemaCurrent;
}

bool is_supported_data_analytics_schema(std::string_view schema) {
  return schema == kDataAnalyticsSchemaCurrent;
}

bool is_supported_sequence_symbolic_analytics_schema(std::string_view schema) {
  return schema == kSequenceAnalyticsSymbolicSchemaCurrent ||
         schema == kEmbeddingSequenceAnalyticsSymbolicSchemaCurrent;
}

bool is_supported_data_symbolic_analytics_schema(std::string_view schema) {
  return schema == kDataAnalyticsSymbolicSchemaCurrent;
}

std::filesystem::path source_data_analytics_root_directory() {
  return cuwacunu::hashimyei::store_root() / "tsi.source" / "data_analytics.v2";
}

std::filesystem::path source_data_analytics_contract_directory(
    std::string_view contract_hash) {
  const std::string token = contract_hash_path_token_(contract_hash);
  if (token.empty()) return {};
  return source_data_analytics_root_directory() / token;
}

std::filesystem::path source_data_analytics_instance_directory(
    std::string_view contract_hash,
    std::string_view source_instance) {
  if (source_instance.empty()) return {};
  const auto contract_dir = source_data_analytics_contract_directory(contract_hash);
  if (contract_dir.empty()) return {};
  return contract_dir /
         std::string(source_instance);
}

std::filesystem::path source_data_analytics_latest_file_path(
    std::string_view contract_hash,
    std::string_view source_instance) {
  const auto base = source_data_analytics_instance_directory(
      contract_hash,
      source_instance);
  if (base.empty()) return {};
  return base / std::string(kDataAnalyticsLatestReportFilename);
}

std::filesystem::path source_data_analytics_symbolic_latest_file_path(
    std::string_view contract_hash,
    std::string_view source_instance) {
  const auto base = source_data_analytics_instance_directory(
      contract_hash,
      source_instance);
  if (base.empty()) return {};
  return base / std::string(kDataAnalyticsSymbolicLatestReportFilename);
}

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
