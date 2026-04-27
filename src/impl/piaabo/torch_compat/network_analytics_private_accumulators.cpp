/* network_analytics.cpp */
#include "piaabo/torch_compat/network_analytics.h"

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include <ATen/ops/linalg_svd.h>
#include <torch/torch.h>

#include <atomic>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

constexpr double kNumericEpsilon = 1e-18;
constexpr std::string_view kRefRangeUnitInterval = "[0,1]";
constexpr std::string_view kRefRangeNonNegative = "[0,+inf)";
constexpr std::string_view kRefRangePositive = "[1,+inf)";
constexpr std::string_view kRefRangeSigned = "(-inf,+inf)";

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

struct report_accumulator_t {
  std::uint64_t tensor_count{0};
  std::uint64_t trainable_tensor_count{0};
  std::uint64_t total_parameter_count{0};
  std::uint64_t finite_parameter_count{0};
  std::uint64_t nan_parameter_count{0};
  std::uint64_t inf_parameter_count{0};

  long double sum{0.0L};
  long double sum_sq{0.0L};
  long double sum_abs{0.0L};
  long double sum_abs_log_abs{0.0L};

  std::uint64_t near_zero_count{0};
  std::uint64_t exact_zero_count{0};
  std::uint64_t non_zero_abs_count{0};

  double min{std::numeric_limits<double>::infinity()};
  double max{-std::numeric_limits<double>::infinity()};
  double max_abs{0.0};
  double max_abs_tensor_value{0.0};
  std::string max_abs_tensor_name{};

  std::vector<double> tensor_rms{};
};

struct buffer_accumulator_t {
  std::uint64_t buffer_tensor_count{0};
  std::uint64_t total_buffer_count{0};
  std::uint64_t finite_buffer_count{0};
  std::uint64_t nan_buffer_count{0};
  std::uint64_t inf_buffer_count{0};
  double max_abs_buffer_value{0.0};
  std::string max_abs_buffer_name{};
};

struct spectral_aggregate_t {
  std::uint64_t matrix_tensor_count{0};
  std::uint64_t spectral_tensor_count{0};
  std::uint64_t spectral_skipped_tensor_count{0};
  std::uint64_t spectral_failed_tensor_count{0};

  double sum_spectral_norm{0.0};
  double max_spectral_norm{0.0};

  double sum_stable_rank{0.0};
  double min_stable_rank{std::numeric_limits<double>::infinity()};

  double sum_effective_rank{0.0};
  double min_effective_rank{std::numeric_limits<double>::infinity()};

  double sum_row_norm_cv{0.0};
  double sum_col_norm_cv{0.0};
};

struct tensor_snapshot_t {
  std::string tensor_name{};

  std::uint64_t total_count{0};
  std::uint64_t finite_count{0};
  std::uint64_t nan_count{0};
  std::uint64_t inf_count{0};

  double near_zero_ratio{0.0};
  double max_abs_over_rms{0.0};
  double non_finite_ratio{0.0};

  bool matrix_like{false};
  bool spectral_computed{false};
  double spectral_norm{0.0};
  double stable_rank{0.0};
  double effective_rank{0.0};
};

[[nodiscard]] inline double safe_ratio(double num, double den) {
  return (den > 0.0) ? (num / den) : 0.0;
}

[[nodiscard]] inline double clamp01(double x) {
  if (x < 0.0) return 0.0;
  if (x > 1.0) return 1.0;
  return x;
}

[[nodiscard]] inline std::string bool_to_ascii(bool value) {
  return value ? "true" : "false";
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

void append_int_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::int64_t value,
                       std::string_view declared_domain = kRefRangeSigned) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value, std::string(declared_domain)));
}

void append_u64_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::uint64_t value,
                       std::string_view declared_domain = kRefRangeNonNegative) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
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

void append_ratio_entry_(runtime_lls_document_t* document,
                         std::string_view key,
                         double value) {
  append_double_entry_(document, key, value, kRefRangeUnitInterval);
}

void append_nonneg_double_entry_(runtime_lls_document_t* document,
                                 std::string_view key,
                                 double value) {
  append_double_entry_(document, key, value, kRefRangeNonNegative);
}

[[nodiscard]] std::string_view topk_value_reference_domain_(
    std::string_view prefix) {
  if (prefix == "top_nonfinite_ratio" || prefix == "top_near_zero_ratio") {
    return kRefRangeUnitInterval;
  }
  return kRefRangeNonNegative;
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

[[nodiscard]] std::string lower_ascii_copy_(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

[[nodiscard]] bool parse_i64_strict_(
    std::string_view text,
    std::int64_t* out) {
  if (out == nullptr) return false;
  text = trim_ascii_ws_view_(text);
  if (text.empty()) return false;
  std::int64_t value = 0;
  const auto r =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (r.ec != std::errc{} || r.ptr != text.data() + text.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] bool parse_double_strict_(
    std::string_view text,
    double* out) {
  if (out == nullptr) return false;
  text = trim_ascii_ws_view_(text);
  if (text.empty()) return false;
  std::string owned(text);
  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(owned.c_str(), &end);
  if (errno != 0 || end == nullptr || end != owned.c_str() + owned.size()) {
    return false;
  }
  *out = value;
  return std::isfinite(value);
}

[[nodiscard]] bool parse_bool_strict_(
    std::string_view text,
    bool* out) {
  if (out == nullptr) return false;
  std::string v = lower_ascii_copy_(std::string(trim_ascii_ws_view_(text)));
  if (v == "true" || v == "1" || v == "yes" || v == "on") {
    *out = true;
    return true;
  }
  if (v == "false" || v == "0" || v == "no" || v == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] network_analytics_options_t normalize_options_(
    const network_analytics_options_t& options) {
  network_analytics_options_t out = options;
  if (out.near_zero_epsilon < 0.0) out.near_zero_epsilon = 0.0;
  if (out.log10_abs_histogram_bins < 1) out.log10_abs_histogram_bins = 1;
  if (out.log10_abs_histogram_max <= out.log10_abs_histogram_min) {
    out.log10_abs_histogram_max = out.log10_abs_histogram_min + 1.0;
  }
  if (out.spectral_max_elements < 1) out.spectral_max_elements = 1;
  if (out.anomaly_top_k < 0) out.anomaly_top_k = 0;
  return out;
}

[[nodiscard]] std::vector<std::string> extract_identifier_tokens_(
    std::string_view text) {
  std::vector<std::string> out;
  std::string current;
  auto is_ident = [](unsigned char c) {
    return std::isalnum(c) || c == '_' || c == '.' || c == '-' || c == ':';
  };
  for (const char ch : text) {
    const unsigned char c = static_cast<unsigned char>(ch);
    if (is_ident(c)) {
      current.push_back(static_cast<char>(c));
    } else if (!current.empty()) {
      out.push_back(current);
      current.clear();
    }
  }
  if (!current.empty()) out.push_back(current);
  return out;
}

[[nodiscard]] double normalized_histogram_entropy_(
    const std::vector<std::uint64_t>& counts,
    std::uint64_t total) {
  if (counts.empty() || total == 0) return 0.0;
  double h = 0.0;
  for (const std::uint64_t c : counts) {
    if (c == 0) continue;
    const double p = static_cast<double>(c) / static_cast<double>(total);
    h -= p * std::log(p);
  }
  const double norm = std::log(static_cast<double>(counts.size()));
  if (norm <= 0.0) return 0.0;
  return clamp01(h / norm);
}

[[nodiscard]] double normalized_abs_energy_entropy_(
    std::uint64_t non_zero_abs_count,
    long double sum_abs,
    long double sum_abs_log_abs) {
  if (non_zero_abs_count <= 1 || sum_abs <= 0.0L) return 0.0;
  const long double h = std::log(sum_abs) - (sum_abs_log_abs / sum_abs);
  const long double norm = std::log(static_cast<long double>(non_zero_abs_count));
  if (norm <= 0.0L) return 0.0;
  return clamp01(static_cast<double>(h / norm));
}

[[nodiscard]] double normalized_shannon_entropy_from_counts_(
    const std::vector<std::uint64_t>& counts) {
  if (counts.empty()) return 0.0;
  std::uint64_t total = 0;
  std::size_t active = 0;
  for (const std::uint64_t c : counts) {
    total += c;
    if (c > 0) ++active;
  }
  if (total == 0 || active <= 1) return 0.0;

  double h = 0.0;
  for (const std::uint64_t c : counts) {
    if (c == 0) continue;
    const double p = static_cast<double>(c) / static_cast<double>(total);
    h -= p * std::log(p);
  }
  const double denom = std::log(static_cast<double>(active));
  if (denom <= 0.0) return 0.0;
  return clamp01(h / denom);
}

template <class Key_t>
[[nodiscard]] double normalized_shannon_entropy_from_map_(
    const std::unordered_map<Key_t, std::uint64_t>& counts) {
  std::vector<std::uint64_t> values;
  values.reserve(counts.size());
  for (const auto& kv : counts) {
    values.push_back(kv.second);
  }
  return normalized_shannon_entropy_from_counts_(values);
}

[[nodiscard]] double histogram_quantile_log10_(
    const std::vector<std::uint64_t>& counts,
    double hmin,
    double hmax,
    double quantile) {
  if (counts.empty() || hmax <= hmin) return 0.0;

  std::uint64_t total = 0;
  for (const std::uint64_t c : counts) total += c;
  if (total == 0) return 0.0;

  const double q = clamp01(quantile);
  const double target = q * static_cast<double>(total - 1);
  const double bin_w = (hmax - hmin) / static_cast<double>(counts.size());

  std::uint64_t cumulative = 0;
  for (std::size_t i = 0; i < counts.size(); ++i) {
    const std::uint64_t count = counts[i];
    const std::uint64_t prev = cumulative;
    cumulative += count;
    if (count == 0) continue;
    if (target <= static_cast<double>(cumulative - 1) ||
        i + 1 == counts.size()) {
      double inside = (target - static_cast<double>(prev)) /
                      static_cast<double>(count);
      if (!std::isfinite(inside)) inside = 0.0;
      if (inside < 0.0) inside = 0.0;
      if (inside > 1.0) inside = 1.0;
      return hmin + (static_cast<double>(i) + inside) * bin_w;
    }
  }

  return hmax;
}

[[nodiscard]] double sorted_quantile_(std::vector<double> values,
                                      double quantile) {
  if (values.empty()) return 0.0;
  std::sort(values.begin(), values.end());
  const double q = clamp01(quantile);
  const double pos = q * static_cast<double>(values.size() - 1);
  const std::size_t lo = static_cast<std::size_t>(std::floor(pos));
  const std::size_t hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo >= values.size()) return values.back();
  if (hi >= values.size()) return values.back();
  if (lo == hi) return values[lo];
  const double t = pos - static_cast<double>(lo);
  return values[lo] * (1.0 - t) + values[hi] * t;
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

[[nodiscard]] std::vector<analytics_topk_entry_t> take_top_k_(
    std::vector<analytics_topk_entry_t> candidates,
    std::size_t k,
    bool descending) {
  if (k == 0 || candidates.empty()) return {};
  std::sort(
      candidates.begin(),
      candidates.end(),
      [descending](const analytics_topk_entry_t& a,
                   const analytics_topk_entry_t& b) {
        if (a.value == b.value) return a.tensor_name < b.tensor_name;
        return descending ? (a.value > b.value) : (a.value < b.value);
      });
  if (candidates.size() > k) {
    candidates.resize(k);
  }
  return candidates;
}

void append_topk_entries_(
    runtime_lls_document_t* document,
    std::string_view prefix,
    const std::vector<analytics_topk_entry_t>& entries) {
  if (document == nullptr) return;
  append_u64_entry_(
      document, std::string(prefix) + "_count", entries.size(), kRefRangeNonNegative);
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const std::string index = std::to_string(i + 1);
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            std::string(prefix) + "_" + index + "_name", entries[i].tensor_name));
    append_double_entry_(
        document,
        std::string(prefix) + "_" + index + "_value",
        entries[i].value,
        topk_value_reference_domain_(prefix));
  }
}

void accumulate_buffer_tensor_(
    const std::string& tensor_name,
    const torch::Tensor& tensor,
    buffer_accumulator_t* out) {
  if (out == nullptr) return;
  if (!tensor.defined()) return;

  auto flattened = tensor.detach().to(torch::kCPU).to(torch::kFloat64).contiguous().flatten();
  const std::int64_t n64 = flattened.numel();
  if (n64 <= 0) return;

  ++out->buffer_tensor_count;
  out->total_buffer_count += static_cast<std::uint64_t>(n64);

  auto nan_mask = torch::isnan(flattened);
  auto inf_mask = torch::isinf(flattened);
  auto finite_mask = torch::isfinite(flattened);

  const std::uint64_t nan_count =
      static_cast<std::uint64_t>(nan_mask.sum().item<std::int64_t>());
  const std::uint64_t inf_count =
      static_cast<std::uint64_t>(inf_mask.sum().item<std::int64_t>());
  const std::uint64_t finite_count =
      static_cast<std::uint64_t>(finite_mask.sum().item<std::int64_t>());

  out->nan_buffer_count += nan_count;
  out->inf_buffer_count += inf_count;
  out->finite_buffer_count += finite_count;

  if (finite_count == 0) return;
  auto finite_values = flattened.masked_select(finite_mask);
  const double tensor_max_abs = finite_values.abs().max().item<double>();
  if (tensor_max_abs > out->max_abs_buffer_value) {
    out->max_abs_buffer_value = tensor_max_abs;
    out->max_abs_buffer_name = tensor_name;
  }
}

[[nodiscard]] tensor_snapshot_t analyze_parameter_tensor_(
    const std::string& tensor_name,
    const torch::Tensor& param,
    const network_analytics_options_t& options,
    std::vector<std::uint64_t>* hist_counts,
    std::vector<double>* exact_logabs_samples,
    report_accumulator_t* acc,
    spectral_aggregate_t* spectral_acc) {
  tensor_snapshot_t snapshot{};
  snapshot.tensor_name = tensor_name;

  if (acc == nullptr) return snapshot;
  if (!param.defined()) return snapshot;

  ++acc->tensor_count;
  if (param.requires_grad()) ++acc->trainable_tensor_count;

  auto flattened = param.detach().to(torch::kCPU).to(torch::kFloat64).contiguous().flatten();
  const std::int64_t n64 = flattened.numel();
  if (n64 <= 0) return snapshot;

  const std::uint64_t total_count = static_cast<std::uint64_t>(n64);
  snapshot.total_count = total_count;
  acc->total_parameter_count += total_count;

  auto nan_mask = torch::isnan(flattened);
  auto inf_mask = torch::isinf(flattened);
  auto finite_mask = torch::isfinite(flattened);

  const std::uint64_t nan_count =
      static_cast<std::uint64_t>(nan_mask.sum().item<std::int64_t>());
  const std::uint64_t inf_count =
      static_cast<std::uint64_t>(inf_mask.sum().item<std::int64_t>());
  const std::uint64_t finite_count =
      static_cast<std::uint64_t>(finite_mask.sum().item<std::int64_t>());

  snapshot.nan_count = nan_count;
  snapshot.inf_count = inf_count;
  snapshot.finite_count = finite_count;

  acc->nan_parameter_count += nan_count;
  acc->inf_parameter_count += inf_count;
  acc->finite_parameter_count += finite_count;

  snapshot.non_finite_ratio = safe_ratio(
      static_cast<double>(nan_count + inf_count),
      static_cast<double>(total_count));

  double tensor_rms = 0.0;
  double tensor_max_abs = 0.0;

  if (finite_count > 0) {
    auto finite_values = flattened.masked_select(finite_mask);
    auto abs_values = finite_values.abs();

    acc->sum += finite_values.sum().item<double>();
    acc->sum_sq += finite_values.pow(2).sum().item<double>();

    const double tensor_min = finite_values.min().item<double>();
    const double tensor_max = finite_values.max().item<double>();
    if (tensor_min < acc->min) acc->min = tensor_min;
    if (tensor_max > acc->max) acc->max = tensor_max;

    tensor_max_abs = abs_values.max().item<double>();
    if (tensor_max_abs > acc->max_abs) acc->max_abs = tensor_max_abs;
    if (tensor_max_abs > acc->max_abs_tensor_value) {
      acc->max_abs_tensor_value = tensor_max_abs;
      acc->max_abs_tensor_name = tensor_name;
    }

    const double abs_sum = abs_values.sum().item<double>();
    acc->sum_abs += abs_sum;

    auto positive_abs = abs_values.masked_select(abs_values.gt(0.0));
    const std::uint64_t non_zero_abs_count =
        static_cast<std::uint64_t>(positive_abs.numel());
    acc->non_zero_abs_count += non_zero_abs_count;
    if (non_zero_abs_count > 0) {
      acc->sum_abs_log_abs +=
          (positive_abs * positive_abs.log()).sum().item<double>();
    }

    const double near_zero_epsilon = std::max(options.near_zero_epsilon, 0.0);
    const std::uint64_t near_zero_count = static_cast<std::uint64_t>(
        abs_values.le(near_zero_epsilon).sum().item<std::int64_t>());
    const std::uint64_t exact_zero_count = static_cast<std::uint64_t>(
        finite_values.eq(0.0).sum().item<std::int64_t>());

    acc->near_zero_count += near_zero_count;
    acc->exact_zero_count += exact_zero_count;

    snapshot.near_zero_ratio = safe_ratio(
        static_cast<double>(near_zero_count),
        static_cast<double>(finite_count));

    tensor_rms =
        std::sqrt(std::max(0.0, finite_values.pow(2).mean().item<double>()));
    snapshot.max_abs_over_rms = safe_ratio(
        tensor_max_abs,
        tensor_rms + kNumericEpsilon);

    acc->tensor_rms.push_back(tensor_rms);

    if (hist_counts != nullptr && !hist_counts->empty()) {
      const std::int64_t bins = static_cast<std::int64_t>(hist_counts->size());
      const double hmin = options.log10_abs_histogram_min;
      const double hmax = options.log10_abs_histogram_max;
      if (bins > 0 && hmax > hmin) {
        const double eps = std::max(options.near_zero_epsilon, 1e-16);
        auto logabs = (abs_values + eps).log10().clamp(hmin, hmax);
        auto scaled =
            ((logabs - hmin) / (hmax - hmin)) * static_cast<double>(bins);
        auto idx = torch::clamp(scaled.to(torch::kLong), 0, bins - 1);
        auto bincount = torch::bincount(idx, c10::nullopt, bins).to(torch::kCPU);
        for (std::int64_t i = 0; i < bins; ++i) {
          (*hist_counts)[static_cast<std::size_t>(i)] +=
              static_cast<std::uint64_t>(bincount[i].item<std::int64_t>());
        }

        if (bins == 1 && exact_logabs_samples != nullptr) {
          const auto sample_count = static_cast<std::size_t>(logabs.numel());
          exact_logabs_samples->reserve(
              exact_logabs_samples->size() + sample_count);
          auto flattened_logabs = logabs.contiguous().flatten();
          const double* begin = flattened_logabs.data_ptr<double>();
          exact_logabs_samples->insert(
              exact_logabs_samples->end(), begin, begin + sample_count);
        }
      }
    }
  }

  if (param.dim() >= 2) {
    snapshot.matrix_like = true;
    if (spectral_acc != nullptr) {
      ++spectral_acc->matrix_tensor_count;
      if (options.enable_spectral_metrics) {
        const std::int64_t rows = param.size(0);
        const std::int64_t cols = (rows > 0) ? (n64 / rows) : 0;
        const std::int64_t matrix_elements = rows * cols;

        bool spectral_skipped = false;
        if (rows <= 0 || cols <= 0) {
          spectral_skipped = true;
        } else if (matrix_elements > options.spectral_max_elements) {
          spectral_skipped = true;
        } else if (finite_count != total_count) {
          spectral_skipped = true;
        }

        if (spectral_skipped) {
          ++spectral_acc->spectral_skipped_tensor_count;
        } else {
          try {
            auto matrix =
                param.detach().to(torch::kCPU).to(torch::kFloat64).contiguous().view(
                    {rows, cols});
            auto svd_result = at::linalg_svd(
                matrix,
                /*full_matrices=*/false,
                c10::nullopt);
            auto singular_values =
                std::get<1>(svd_result).to(torch::kFloat64).contiguous();

            if (singular_values.numel() > 0) {
              const double spectral_norm = singular_values.max().item<double>();
              const double fro_norm_sq = matrix.pow(2).sum().item<double>();
              const double stable_rank = safe_ratio(
                  fro_norm_sq,
                  spectral_norm * spectral_norm + kNumericEpsilon);

              double effective_rank = 0.0;
              const double sigma_sum = singular_values.sum().item<double>();
              if (sigma_sum > 0.0) {
                auto probs = singular_values / sigma_sum;
                auto positive = probs.masked_select(probs.gt(0.0));
                if (positive.numel() > 0) {
                  const double entropy =
                      -(positive * positive.log()).sum().item<double>();
                  effective_rank = std::exp(entropy);
                }
              }

              const auto row_norms = matrix.pow(2).sum(1).sqrt();
              const auto col_norms = matrix.pow(2).sum(0).sqrt();

              const double row_mean = row_norms.mean().item<double>();
              const double col_mean = col_norms.mean().item<double>();
              const double row_var =
                  (row_norms - row_mean).pow(2).mean().item<double>();
              const double col_var =
                  (col_norms - col_mean).pow(2).mean().item<double>();
              const double row_norm_cv =
                  std::sqrt(std::max(0.0, row_var)) /
                  (std::abs(row_mean) + kNumericEpsilon);
              const double col_norm_cv =
                  std::sqrt(std::max(0.0, col_var)) /
                  (std::abs(col_mean) + kNumericEpsilon);

              snapshot.spectral_computed = true;
              snapshot.spectral_norm = spectral_norm;
              snapshot.stable_rank = stable_rank;
              snapshot.effective_rank = effective_rank;

              ++spectral_acc->spectral_tensor_count;
              spectral_acc->sum_spectral_norm += spectral_norm;
              spectral_acc->max_spectral_norm =
                  std::max(spectral_acc->max_spectral_norm, spectral_norm);

              spectral_acc->sum_stable_rank += stable_rank;
              spectral_acc->min_stable_rank =
                  std::min(spectral_acc->min_stable_rank, stable_rank);

              spectral_acc->sum_effective_rank += effective_rank;
              spectral_acc->min_effective_rank =
                  std::min(spectral_acc->min_effective_rank, effective_rank);

              spectral_acc->sum_row_norm_cv += row_norm_cv;
              spectral_acc->sum_col_norm_cv += col_norm_cv;
            } else {
              ++spectral_acc->spectral_skipped_tensor_count;
            }
          } catch (...) {
            ++spectral_acc->spectral_failed_tensor_count;
          }
        }
      }
    }
  }

  return snapshot;
}

[[nodiscard]] runtime_lls_document_t make_runtime_lls_document_(
    const network_analytics_report_t& report,
    std::string_view checkpoint_filename,
    const tsiemene::component_report_identity_t& report_identity) {
  runtime_lls_document_t document{};
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  append_string_entry_if_nonempty_(&document, "checkpoint_file", checkpoint_filename);
  append_u64_entry_(&document, "tensor_count", report.tensor_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "trainable_tensor_count", report.trainable_tensor_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "total_parameter_count", report.total_parameter_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "finite_parameter_count", report.finite_parameter_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "nan_parameter_count", report.nan_parameter_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "inf_parameter_count", report.inf_parameter_count, kRefRangeNonNegative);
  append_ratio_entry_(&document, "finite_ratio", report.finite_ratio);
  append_ratio_entry_(&document, "non_finite_ratio", report.non_finite_ratio);
  append_nonneg_double_entry_(&document, "stddev", report.stddev);
  append_double_entry_(&document, "min", report.min, kRefRangeSigned);
  append_double_entry_(&document, "max", report.max, kRefRangeSigned);
  append_nonneg_double_entry_(&document, "l1_mean_abs", report.l1_mean_abs);
  append_nonneg_double_entry_(&document, "l2_rms", report.l2_rms);
  append_nonneg_double_entry_(&document, "max_abs", report.max_abs);
  append_nonneg_double_entry_(&document, "max_abs_over_rms", report.max_abs_over_rms);
  append_nonneg_double_entry_(&document, "near_zero_threshold", report.near_zero_threshold);
  append_ratio_entry_(&document, "near_zero_ratio", report.near_zero_ratio);
  append_ratio_entry_(&document, "exact_zero_ratio", report.exact_zero_ratio);
  append_ratio_entry_(&document, "abs_energy_entropy", report.abs_energy_entropy);
  append_ratio_entry_(
      &document, "log10_abs_histogram_entropy", report.log10_abs_histogram_entropy);
  append_int_entry_(
      &document,
      "log10_abs_histogram_bins",
      report.normalized_options.log10_abs_histogram_bins,
      kRefRangePositive);
  append_double_entry_(
      &document,
      "log10_abs_histogram_min",
      report.normalized_options.log10_abs_histogram_min,
      kRefRangeSigned);
  append_double_entry_(
      &document,
      "log10_abs_histogram_max",
      report.normalized_options.log10_abs_histogram_max,
      kRefRangeSigned);
  append_string_entry_if_nonempty_(&document, "max_abs_tensor_name", report.max_abs_tensor_name);
  append_nonneg_double_entry_(&document, "max_abs_tensor_value", report.max_abs_tensor_value);

  append_bool_entry_(
      &document, "include_buffers", report.normalized_options.include_buffers);
  append_bool_entry_(
      &document,
      "enable_spectral_metrics",
      report.normalized_options.enable_spectral_metrics);
  append_int_entry_(
      &document,
      "spectral_max_elements",
      report.normalized_options.spectral_max_elements,
      kRefRangePositive);
  append_int_entry_(
      &document,
      "anomaly_top_k",
      report.normalized_options.anomaly_top_k,
      kRefRangeNonNegative);

  append_nonneg_double_entry_(&document, "tensor_rms_mean", report.tensor_rms_mean);
  append_nonneg_double_entry_(&document, "tensor_rms_std", report.tensor_rms_std);
  append_nonneg_double_entry_(&document, "tensor_rms_cv", report.tensor_rms_cv);
  append_nonneg_double_entry_(&document, "tensor_rms_min", report.tensor_rms_min);
  append_nonneg_double_entry_(&document, "tensor_rms_max", report.tensor_rms_max);
  append_nonneg_double_entry_(
      &document, "tensor_rms_max_over_min", report.tensor_rms_max_over_min);

  append_nonneg_double_entry_(&document, "abs_p50", report.abs_p50);
  append_nonneg_double_entry_(&document, "abs_p90", report.abs_p90);
  append_nonneg_double_entry_(&document, "abs_p99", report.abs_p99);
  append_double_entry_(&document, "log10_abs_p50", report.log10_abs_p50, kRefRangeSigned);
  append_nonneg_double_entry_(&document, "log10_abs_iqr", report.log10_abs_iqr);

  append_u64_entry_(&document, "buffer_tensor_count", report.buffer_tensor_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "total_buffer_count", report.total_buffer_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "finite_buffer_count", report.finite_buffer_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "nan_buffer_count", report.nan_buffer_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "inf_buffer_count", report.inf_buffer_count, kRefRangeNonNegative);
  append_ratio_entry_(&document, "finite_buffer_ratio", report.finite_buffer_ratio);
  append_string_entry_if_nonempty_(&document, "max_abs_buffer_name", report.max_abs_buffer_name);
  append_nonneg_double_entry_(&document, "max_abs_buffer_value", report.max_abs_buffer_value);

  append_u64_entry_(&document, "matrix_tensor_count", report.matrix_tensor_count, kRefRangeNonNegative);
  append_u64_entry_(&document, "spectral_tensor_count", report.spectral_tensor_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document,
      "spectral_skipped_tensor_count",
      report.spectral_skipped_tensor_count,
      kRefRangeNonNegative);
  append_u64_entry_(
      &document,
      "spectral_failed_tensor_count",
      report.spectral_failed_tensor_count,
      kRefRangeNonNegative);
  append_nonneg_double_entry_(&document, "spectral_norm_mean", report.spectral_norm_mean);
  append_nonneg_double_entry_(&document, "spectral_norm_max", report.spectral_norm_max);
  append_nonneg_double_entry_(&document, "stable_rank_mean", report.stable_rank_mean);
  append_nonneg_double_entry_(&document, "stable_rank_min", report.stable_rank_min);
  append_nonneg_double_entry_(&document, "effective_rank_mean", report.effective_rank_mean);
  append_nonneg_double_entry_(&document, "effective_rank_min", report.effective_rank_min);
  append_nonneg_double_entry_(&document, "row_norm_cv_mean", report.row_norm_cv_mean);
  append_nonneg_double_entry_(&document, "col_norm_cv_mean", report.col_norm_cv_mean);
  append_nonneg_double_entry_(
      &document, "network_global_entropic_capacity", report.network_global_entropic_capacity);
  append_nonneg_double_entry_(
      &document, "network_entropic_bottleneck_min", report.network_entropic_bottleneck_min);
  append_nonneg_double_entry_(
      &document, "network_effective_rank_p50", report.network_effective_rank_p50);
  append_nonneg_double_entry_(
      &document, "network_effective_rank_p90", report.network_effective_rank_p90);
  append_u64_entry_(
      &document,
      "network_capacity_tensor_count",
      report.network_capacity_tensor_count,
      kRefRangeNonNegative);

  append_topk_entries_(&document, "top_nonfinite_ratio", report.top_nonfinite_ratio_tensors);
  append_topk_entries_(
      &document, "top_max_abs_over_rms", report.top_max_abs_over_rms_tensors);
  append_topk_entries_(&document, "top_near_zero_ratio", report.top_near_zero_ratio_tensors);
  append_topk_entries_(&document, "top_low_stable_rank", report.top_low_stable_rank_tensors);
  append_topk_entries_(
      &document, "top_low_effective_rank", report.top_low_effective_rank_tensors);
  append_topk_entries_(
      &document, "top_high_spectral_norm", report.top_high_spectral_norm_tensors);

  return document;
}

