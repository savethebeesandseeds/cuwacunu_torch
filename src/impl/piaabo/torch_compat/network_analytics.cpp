/* network_analytics.cpp */
#include "piaabo/torch_compat/network_analytics.h"

#include "camahjucunu/dsl/network_design/network_design.h"
#include <torch/torch.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
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

void append_topk_key_values_(
    std::ostringstream* oss,
    std::string_view prefix,
    const std::vector<analytics_topk_entry_t>& entries) {
  if (oss == nullptr) return;
  *oss << prefix << "_count=" << entries.size() << "\n";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    *oss << prefix << "_" << (i + 1) << "_name=" << entries[i].tensor_name
         << "\n";
    *oss << prefix << "_" << (i + 1) << "_value=" << entries[i].value << "\n";
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
      if (bins > 1 && hmax > hmin) {
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
            auto svd_result = torch::linalg::svd(
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

[[nodiscard]] std::string as_ascii_key_value_(
    const network_analytics_report_t& report,
    const network_analytics_options_t& options,
    std::string_view checkpoint_filename) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  oss << "schema=" << report.schema << "\n";
  if (!checkpoint_filename.empty()) {
    oss << "checkpoint_file=" << checkpoint_filename << "\n";
  }
  oss << "tensor_count=" << report.tensor_count << "\n";
  oss << "trainable_tensor_count=" << report.trainable_tensor_count << "\n";
  oss << "total_parameter_count=" << report.total_parameter_count << "\n";
  oss << "finite_parameter_count=" << report.finite_parameter_count << "\n";
  oss << "nan_parameter_count=" << report.nan_parameter_count << "\n";
  oss << "inf_parameter_count=" << report.inf_parameter_count << "\n";
  oss << "finite_ratio=" << report.finite_ratio << "\n";
  oss << "non_finite_ratio=" << report.non_finite_ratio << "\n";
  oss << "stddev=" << report.stddev << "\n";
  oss << "min=" << report.min << "\n";
  oss << "max=" << report.max << "\n";
  oss << "l1_mean_abs=" << report.l1_mean_abs << "\n";
  oss << "l2_rms=" << report.l2_rms << "\n";
  oss << "max_abs=" << report.max_abs << "\n";
  oss << "max_abs_over_rms=" << report.max_abs_over_rms << "\n";
  oss << "near_zero_threshold=" << report.near_zero_threshold << "\n";
  oss << "near_zero_ratio=" << report.near_zero_ratio << "\n";
  oss << "exact_zero_ratio=" << report.exact_zero_ratio << "\n";
  oss << "abs_energy_entropy=" << report.abs_energy_entropy << "\n";
  oss << "log10_abs_histogram_entropy=" << report.log10_abs_histogram_entropy
      << "\n";
  oss << "log10_abs_histogram_bins=" << options.log10_abs_histogram_bins << "\n";
  oss << "log10_abs_histogram_min=" << options.log10_abs_histogram_min << "\n";
  oss << "log10_abs_histogram_max=" << options.log10_abs_histogram_max << "\n";
  oss << "max_abs_tensor_name=" << report.max_abs_tensor_name << "\n";
  oss << "max_abs_tensor_value=" << report.max_abs_tensor_value << "\n";

  oss << "include_buffers=" << bool_to_ascii(options.include_buffers) << "\n";
  oss << "enable_spectral_metrics=" << bool_to_ascii(options.enable_spectral_metrics)
      << "\n";
  oss << "spectral_max_elements=" << options.spectral_max_elements << "\n";
  oss << "anomaly_top_k=" << options.anomaly_top_k << "\n";

  oss << "tensor_rms_mean=" << report.tensor_rms_mean << "\n";
  oss << "tensor_rms_std=" << report.tensor_rms_std << "\n";
  oss << "tensor_rms_cv=" << report.tensor_rms_cv << "\n";
  oss << "tensor_rms_min=" << report.tensor_rms_min << "\n";
  oss << "tensor_rms_max=" << report.tensor_rms_max << "\n";
  oss << "tensor_rms_max_over_min=" << report.tensor_rms_max_over_min << "\n";

  oss << "abs_p50=" << report.abs_p50 << "\n";
  oss << "abs_p90=" << report.abs_p90 << "\n";
  oss << "abs_p99=" << report.abs_p99 << "\n";
  oss << "log10_abs_p50=" << report.log10_abs_p50 << "\n";
  oss << "log10_abs_iqr=" << report.log10_abs_iqr << "\n";

  oss << "buffer_tensor_count=" << report.buffer_tensor_count << "\n";
  oss << "total_buffer_count=" << report.total_buffer_count << "\n";
  oss << "finite_buffer_count=" << report.finite_buffer_count << "\n";
  oss << "nan_buffer_count=" << report.nan_buffer_count << "\n";
  oss << "inf_buffer_count=" << report.inf_buffer_count << "\n";
  oss << "finite_buffer_ratio=" << report.finite_buffer_ratio << "\n";
  oss << "max_abs_buffer_name=" << report.max_abs_buffer_name << "\n";
  oss << "max_abs_buffer_value=" << report.max_abs_buffer_value << "\n";

  oss << "matrix_tensor_count=" << report.matrix_tensor_count << "\n";
  oss << "spectral_tensor_count=" << report.spectral_tensor_count << "\n";
  oss << "spectral_skipped_tensor_count=" << report.spectral_skipped_tensor_count
      << "\n";
  oss << "spectral_failed_tensor_count=" << report.spectral_failed_tensor_count
      << "\n";
  oss << "spectral_norm_mean=" << report.spectral_norm_mean << "\n";
  oss << "spectral_norm_max=" << report.spectral_norm_max << "\n";
  oss << "stable_rank_mean=" << report.stable_rank_mean << "\n";
  oss << "stable_rank_min=" << report.stable_rank_min << "\n";
  oss << "effective_rank_mean=" << report.effective_rank_mean << "\n";
  oss << "effective_rank_min=" << report.effective_rank_min << "\n";
  oss << "row_norm_cv_mean=" << report.row_norm_cv_mean << "\n";
  oss << "col_norm_cv_mean=" << report.col_norm_cv_mean << "\n";
  oss << "network_global_entropic_capacity="
      << report.network_global_entropic_capacity << "\n";
  oss << "network_entropic_bottleneck_min="
      << report.network_entropic_bottleneck_min << "\n";
  oss << "network_effective_rank_p50=" << report.network_effective_rank_p50
      << "\n";
  oss << "network_effective_rank_p90=" << report.network_effective_rank_p90
      << "\n";
  oss << "network_capacity_tensor_count=" << report.network_capacity_tensor_count
      << "\n";

  append_topk_key_values_(&oss, "top_nonfinite_ratio", report.top_nonfinite_ratio_tensors);
  append_topk_key_values_(
      &oss, "top_max_abs_over_rms", report.top_max_abs_over_rms_tensors);
  append_topk_key_values_(&oss, "top_near_zero_ratio", report.top_near_zero_ratio_tensors);
  append_topk_key_values_(&oss, "top_low_stable_rank", report.top_low_stable_rank_tensors);
  append_topk_key_values_(
      &oss,
      "top_low_effective_rank",
      report.top_low_effective_rank_tensors);
  append_topk_key_values_(
      &oss,
      "top_high_spectral_norm",
      report.top_high_spectral_norm_tensors);

  return oss.str();
}

[[nodiscard]] std::string as_ascii_key_value_(
    const network_design_analytics_report_t& report,
    std::string_view source_label) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  oss << "schema=" << report.schema << "\n";
  if (!source_label.empty()) {
    oss << "source_label=" << source_label << "\n";
  }
  oss << "network_id=" << report.network_id << "\n";
  oss << "join_policy=" << report.join_policy << "\n";
  oss << "node_count=" << report.node_count << "\n";
  oss << "export_count=" << report.export_count << "\n";
  oss << "parameter_count=" << report.parameter_count << "\n";
  oss << "internal_edge_count=" << report.internal_edge_count << "\n";
  oss << "export_edge_count=" << report.export_edge_count << "\n";
  oss << "total_edge_count=" << report.total_edge_count << "\n";
  oss << "weakly_connected_components=" << report.weakly_connected_components
      << "\n";
  oss << "isolated_node_count=" << report.isolated_node_count << "\n";
  oss << "has_internal_cycle=" << bool_to_ascii(report.has_internal_cycle) << "\n";
  oss << "acyclic_longest_path_nodes=" << report.acyclic_longest_path_nodes
      << "\n";
  oss << "internal_edge_density=" << report.internal_edge_density << "\n";
  oss << "mean_in_degree=" << report.mean_in_degree << "\n";
  oss << "mean_out_degree=" << report.mean_out_degree << "\n";
  oss << "max_in_degree=" << report.max_in_degree << "\n";
  oss << "max_out_degree=" << report.max_out_degree << "\n";
  oss << "distinct_node_kind_count=" << report.distinct_node_kind_count << "\n";
  oss << "node_kind_entropy=" << report.node_kind_entropy << "\n";
  oss << "in_degree_entropy=" << report.in_degree_entropy << "\n";
  oss << "out_degree_entropy=" << report.out_degree_entropy << "\n";
  oss << "edge_kind_transition_entropy=" << report.edge_kind_transition_entropy
      << "\n";

  oss << "topological_order_count=" << report.topological_order_count << "\n";
  oss << "topological_order_ratio=" << report.topological_order_ratio << "\n";
  oss << "edge_surplus=" << report.edge_surplus << "\n";
  oss << "active_fanout_mean=" << report.active_fanout_mean << "\n";

  oss << "source_count=" << report.source_count << "\n";
  oss << "internal_sink_count=" << report.internal_sink_count << "\n";
  oss << "export_reachable_node_count=" << report.export_reachable_node_count
      << "\n";
  oss << "export_reachable_ratio=" << report.export_reachable_ratio << "\n";
  oss << "dead_end_node_count=" << report.dead_end_node_count << "\n";
  oss << "orphan_node_count=" << report.orphan_node_count << "\n";
  oss << "longest_source_to_export_path_nodes="
      << report.longest_source_to_export_path_nodes << "\n";
  oss << "median_source_to_export_path_nodes="
      << report.median_source_to_export_path_nodes << "\n";
  oss << "branch_node_count=" << report.branch_node_count << "\n";
  oss << "merge_node_count=" << report.merge_node_count << "\n";

  oss << "scc_count=" << report.scc_count << "\n";
  oss << "largest_scc_size=" << report.largest_scc_size << "\n";
  oss << "cyclic_node_count=" << report.cyclic_node_count << "\n";
  oss << "cyclic_node_ratio=" << report.cyclic_node_ratio << "\n";

  oss << "skip_edge_count=" << report.skip_edge_count << "\n";
  oss << "skip_edge_ratio=" << report.skip_edge_ratio << "\n";
  oss << "mean_skip_span=" << report.mean_skip_span << "\n";
  oss << "max_skip_span=" << report.max_skip_span << "\n";

  oss << "explicit_edge_count=" << report.explicit_edge_count << "\n";
  oss << "inferred_edge_count=" << report.inferred_edge_count << "\n";
  oss << "unresolved_identifier_token_count="
      << report.unresolved_identifier_token_count << "\n";
  oss << "self_reference_token_count=" << report.self_reference_token_count
      << "\n";

  return oss.str();
}

[[nodiscard]] std::string as_pretty_text_(
    const network_design_analytics_report_t& report,
    std::string_view source_label,
    bool use_color) {
  const char* c_reset = use_color ? "\x1b[0m" : "";
  const char* c_title = use_color ? "\x1b[1;96m" : "";
  const char* c_section = use_color ? "\x1b[1;94m" : "";
  const char* c_key = use_color ? "\x1b[90m" : "";
  const char* c_value = use_color ? "\x1b[97m" : "";
  const char* c_note = use_color ? "\x1b[36m" : "";
  const char* c_good = use_color ? "\x1b[92m" : "";
  const char* c_bad = use_color ? "\x1b[91m" : "";

  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  auto line = [&](std::string_view key,
                  const auto& value,
                  std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset << " : "
        << c_value << value << c_reset << "\t" << c_note << note << c_reset
        << "\n";
  };
  auto line_color = [&](std::string_view key,
                        const auto& value,
                        const char* color,
                        std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset << " : "
        << color << value << c_reset << "\t" << c_note << note << c_reset << "\n";
  };
  auto section = [&](std::string_view name) {
    oss << c_section << name << c_reset << "\n";
  };

  oss << c_title << "Network Design Analytics Report" << c_reset << "\n";
  if (!source_label.empty()) {
    line("source_label", source_label, "contract/key/path reference");
  }
  oss << "\t" << c_key << std::left << std::setw(34) << "metric" << c_reset
      << " : " << c_value << "value" << c_reset << "\t" << c_note
      << "comment + best guidance" << c_reset << "\n\n";

  section("Identity");
  line("schema", report.schema, "report schema id (best: fixed)");
  line("network_id", report.network_id, "canonical network id (best: stable)");
  line("join_policy", report.join_policy, "assembly policy tag (best: expected)");
  oss << "\n";

  section("Graph Size");
  line("node_count", report.node_count, "declared node blocks (best: task-dependent)");
  line("export_count", report.export_count, "declared exports (best: >=1)");
  line("parameter_count", report.parameter_count, "node params declared (best: minimal-complete)");
  line("internal_edge_count", report.internal_edge_count, "node-to-node edges (best: task-dependent)");
  line("export_edge_count", report.export_edge_count, "node-to-export edges (best: >=1)");
  line("total_edge_count", report.total_edge_count, "internal + export edges");
  oss << "\n";

  section("Topology");
  line("weakly_connected_components",
       report.weakly_connected_components,
       "disconnected groups (best: close to 1)");
  line("isolated_node_count",
       report.isolated_node_count,
       "nodes with no edges (best: 0)");
  line_color("has_internal_cycle",
             bool_to_ascii(report.has_internal_cycle),
             report.has_internal_cycle ? c_bad : c_good,
             "cycle in internal graph (best: false for feed-forward)");
  line("topological_order_count",
       report.topological_order_count,
       "sortable nodes count");
  line("topological_order_ratio",
       report.topological_order_ratio,
       "sortable coverage ratio in [0,1]");
  line("acyclic_longest_path_nodes",
       report.acyclic_longest_path_nodes,
       "DAG-only depth proxy");
  oss << "\n";

  section("Complexity");
  line("internal_edge_density",
       report.internal_edge_density,
       "normalized connectivity [0..1] (best: task-dependent)");
  line("edge_surplus",
       report.edge_surplus,
       "E-N+C edge surplus proxy");
  line("mean_in_degree",
       report.mean_in_degree,
       "avg incoming edges per node (best: balanced)");
  line("mean_out_degree",
       report.mean_out_degree,
       "avg outgoing edges per node (best: balanced)");
  line("max_in_degree",
       report.max_in_degree,
       "largest fan-in (best: avoid extreme bottlenecks)");
  line("max_out_degree",
       report.max_out_degree,
       "largest fan-out (best: avoid extreme branching)");
  line("active_fanout_mean",
       report.active_fanout_mean,
       "avg out-degree among nodes with out-degree>0");
  oss << "\n";

  section("Directionality");
  line("source_count", report.source_count, "nodes with in-degree 0");
  line("internal_sink_count", report.internal_sink_count, "nodes with internal out-degree 0");
  line("export_reachable_node_count",
       report.export_reachable_node_count,
       "nodes that can reach any export");
  line("export_reachable_ratio",
       report.export_reachable_ratio,
       "reachable/export-connected node ratio");
  line("dead_end_node_count", report.dead_end_node_count, "nodes reaching no export");
  line("orphan_node_count", report.orphan_node_count, "nodes unreachable from sources");
  line("longest_source_to_export_path_nodes",
       report.longest_source_to_export_path_nodes,
       "condensation-DAG longest path (node-count weighted)");
  line("median_source_to_export_path_nodes",
       report.median_source_to_export_path_nodes,
       "median export-path length");
  line("branch_node_count", report.branch_node_count, "nodes with internal out-degree > 1");
  line("merge_node_count", report.merge_node_count, "nodes with internal in-degree > 1");
  oss << "\n";

  section("Cycle Burden");
  line("scc_count", report.scc_count, "strongly connected component count");
  line("largest_scc_size", report.largest_scc_size, "largest SCC cardinality");
  line("cyclic_node_count", report.cyclic_node_count, "nodes in cyclic SCCs");
  line("cyclic_node_ratio", report.cyclic_node_ratio, "cyclic node fraction");
  oss << "\n";

  section("Skip Geometry");
  line("skip_edge_count", report.skip_edge_count, "cross-SCC edges with span > 1");
  line("skip_edge_ratio", report.skip_edge_ratio, "skip edges / internal edges");
  line("mean_skip_span", report.mean_skip_span, "mean span over skip edges");
  line("max_skip_span", report.max_skip_span, "max skip span");
  oss << "\n";

  section("Edge Evidence");
  line("explicit_edge_count", report.explicit_edge_count, "explicit references (phase-1 fixed at 0)");
  line("inferred_edge_count", report.inferred_edge_count, "token-inferred internal edges");
  line("unresolved_identifier_token_count",
       report.unresolved_identifier_token_count,
       "tokens not resolved to node ids");
  line("self_reference_token_count",
       report.self_reference_token_count,
       "tokens resolving to their own node id");
  oss << "\n";

  section("Entropy and Diversity");
  line("distinct_node_kind_count",
       report.distinct_node_kind_count,
       "unique node kinds (best: task-dependent)");
  line("node_kind_entropy",
       report.node_kind_entropy,
       "balance across node kinds (best: task-dependent)");
  line("in_degree_entropy",
       report.in_degree_entropy,
       "diversity of in-degree distribution (best: task-dependent)");
  line("out_degree_entropy",
       report.out_degree_entropy,
       "diversity of out-degree distribution (best: task-dependent)");
  line("edge_kind_transition_entropy",
       report.edge_kind_transition_entropy,
       "diversity of kind->kind transitions (best: task-dependent)");

  return oss.str();
}

}  // namespace

network_analytics_report_t summarize_module_network_analytics(
    const torch::nn::Module& model,
    const network_analytics_options_t& options) {
  const network_analytics_options_t effective = normalize_options_(options);

  network_analytics_report_t out{};
  out.near_zero_threshold = effective.near_zero_epsilon;

  const std::int64_t bins =
      std::max<std::int64_t>(1, effective.log10_abs_histogram_bins);
  std::vector<std::uint64_t> hist_counts(static_cast<std::size_t>(bins), 0ULL);

  report_accumulator_t acc{};
  buffer_accumulator_t buffer_acc{};
  spectral_aggregate_t spectral_acc{};

  std::vector<analytics_topk_entry_t> nonfinite_candidates{};
  std::vector<analytics_topk_entry_t> max_abs_over_rms_candidates{};
  std::vector<analytics_topk_entry_t> near_zero_candidates{};
  std::vector<analytics_topk_entry_t> low_stable_rank_candidates{};
  std::vector<analytics_topk_entry_t> low_effective_rank_candidates{};
  std::vector<analytics_topk_entry_t> high_spectral_norm_candidates{};

  for (const auto& named_param : model.named_parameters(/*recurse=*/true)) {
    const std::string tensor_name = named_param.key();
    const auto& param = named_param.value();
    const tensor_snapshot_t snapshot = analyze_parameter_tensor_(
        tensor_name,
        param,
        effective,
        &hist_counts,
        &acc,
        &spectral_acc);

    if (snapshot.total_count > 0) {
      nonfinite_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.non_finite_ratio});
    }
    if (snapshot.finite_count > 0) {
      max_abs_over_rms_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.max_abs_over_rms});
      near_zero_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.near_zero_ratio});
    }
    if (snapshot.spectral_computed) {
      low_stable_rank_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.stable_rank});
      low_effective_rank_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.effective_rank});
      high_spectral_norm_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.spectral_norm});
    }
  }

  if (effective.include_buffers) {
    for (const auto& named_buffer : model.named_buffers(/*recurse=*/true)) {
      accumulate_buffer_tensor_(
          named_buffer.key(),
          named_buffer.value(),
          &buffer_acc);
    }
  }

  out.tensor_count = acc.tensor_count;
  out.trainable_tensor_count = acc.trainable_tensor_count;
  out.total_parameter_count = acc.total_parameter_count;
  out.finite_parameter_count = acc.finite_parameter_count;
  out.nan_parameter_count = acc.nan_parameter_count;
  out.inf_parameter_count = acc.inf_parameter_count;

  const double total = static_cast<double>(out.total_parameter_count);
  const double finite = static_cast<double>(out.finite_parameter_count);
  out.finite_ratio = safe_ratio(finite, total);
  out.non_finite_ratio = safe_ratio(
      static_cast<double>(out.nan_parameter_count + out.inf_parameter_count),
      total);

  if (out.finite_parameter_count > 0) {
    const long double finite_ld = static_cast<long double>(out.finite_parameter_count);
    const double mean = static_cast<double>(acc.sum / finite_ld);
    const double ex2 = static_cast<double>(acc.sum_sq / finite_ld);
    out.stddev = std::sqrt(std::max(0.0, ex2 - mean * mean));
    out.min = std::isfinite(acc.min) ? acc.min : 0.0;
    out.max = std::isfinite(acc.max) ? acc.max : 0.0;

    out.l1_mean_abs = static_cast<double>(acc.sum_abs / finite_ld);
    out.l2_rms = std::sqrt(std::max(0.0, static_cast<double>(acc.sum_sq / finite_ld)));
    out.max_abs = acc.max_abs;
    out.max_abs_over_rms = safe_ratio(out.max_abs, out.l2_rms + kNumericEpsilon);

    out.near_zero_ratio =
        safe_ratio(static_cast<double>(acc.near_zero_count), finite);
    out.exact_zero_ratio =
        safe_ratio(static_cast<double>(acc.exact_zero_count), finite);

    out.abs_energy_entropy = normalized_abs_energy_entropy_(
        acc.non_zero_abs_count,
        acc.sum_abs,
        acc.sum_abs_log_abs);
    out.log10_abs_histogram_entropy =
        normalized_histogram_entropy_(hist_counts, out.finite_parameter_count);

    const double p25 = histogram_quantile_log10_(
        hist_counts,
        effective.log10_abs_histogram_min,
        effective.log10_abs_histogram_max,
        0.25);
    const double p50 = histogram_quantile_log10_(
        hist_counts,
        effective.log10_abs_histogram_min,
        effective.log10_abs_histogram_max,
        0.50);
    const double p75 = histogram_quantile_log10_(
        hist_counts,
        effective.log10_abs_histogram_min,
        effective.log10_abs_histogram_max,
        0.75);
    const double p90 = histogram_quantile_log10_(
        hist_counts,
        effective.log10_abs_histogram_min,
        effective.log10_abs_histogram_max,
        0.90);
    const double p99 = histogram_quantile_log10_(
        hist_counts,
        effective.log10_abs_histogram_min,
        effective.log10_abs_histogram_max,
        0.99);

    out.log10_abs_p50 = p50;
    out.log10_abs_iqr = p75 - p25;
    out.abs_p50 = std::pow(10.0, p50);
    out.abs_p90 = std::pow(10.0, p90);
    out.abs_p99 = std::pow(10.0, p99);
  }

  if (!acc.tensor_rms.empty()) {
    const double mean = std::accumulate(acc.tensor_rms.begin(), acc.tensor_rms.end(), 0.0) /
                        static_cast<double>(acc.tensor_rms.size());
    double var = 0.0;
    for (const double v : acc.tensor_rms) {
      const double d = v - mean;
      var += d * d;
    }
    var /= static_cast<double>(acc.tensor_rms.size());

    out.tensor_rms_mean = mean;
    out.tensor_rms_std = std::sqrt(std::max(0.0, var));
    out.tensor_rms_cv = safe_ratio(out.tensor_rms_std, std::abs(mean) + kNumericEpsilon);

    const auto minmax =
        std::minmax_element(acc.tensor_rms.begin(), acc.tensor_rms.end());
    out.tensor_rms_min = *minmax.first;
    out.tensor_rms_max = *minmax.second;
    out.tensor_rms_max_over_min =
        safe_ratio(out.tensor_rms_max, out.tensor_rms_min + kNumericEpsilon);
  }

  out.max_abs_tensor_name = acc.max_abs_tensor_name;
  out.max_abs_tensor_value = acc.max_abs_tensor_value;

  out.buffer_tensor_count = buffer_acc.buffer_tensor_count;
  out.total_buffer_count = buffer_acc.total_buffer_count;
  out.finite_buffer_count = buffer_acc.finite_buffer_count;
  out.nan_buffer_count = buffer_acc.nan_buffer_count;
  out.inf_buffer_count = buffer_acc.inf_buffer_count;
  out.finite_buffer_ratio = safe_ratio(
      static_cast<double>(out.finite_buffer_count),
      static_cast<double>(out.total_buffer_count));
  out.max_abs_buffer_name = buffer_acc.max_abs_buffer_name;
  out.max_abs_buffer_value = buffer_acc.max_abs_buffer_value;

  out.matrix_tensor_count = spectral_acc.matrix_tensor_count;
  out.spectral_tensor_count = spectral_acc.spectral_tensor_count;
  out.spectral_skipped_tensor_count = spectral_acc.spectral_skipped_tensor_count;
  out.spectral_failed_tensor_count = spectral_acc.spectral_failed_tensor_count;
  out.spectral_norm_mean = safe_ratio(
      spectral_acc.sum_spectral_norm,
      static_cast<double>(out.spectral_tensor_count));
  out.spectral_norm_max = spectral_acc.max_spectral_norm;
  out.stable_rank_mean = safe_ratio(
      spectral_acc.sum_stable_rank,
      static_cast<double>(out.spectral_tensor_count));
  out.stable_rank_min = (out.spectral_tensor_count > 0)
                            ? spectral_acc.min_stable_rank
                            : 0.0;
  out.effective_rank_mean = safe_ratio(
      spectral_acc.sum_effective_rank,
      static_cast<double>(out.spectral_tensor_count));
  out.effective_rank_min = (out.spectral_tensor_count > 0)
                               ? spectral_acc.min_effective_rank
                               : 0.0;
  out.row_norm_cv_mean = safe_ratio(
      spectral_acc.sum_row_norm_cv,
      static_cast<double>(out.spectral_tensor_count));
  out.col_norm_cv_mean = safe_ratio(
      spectral_acc.sum_col_norm_cv,
      static_cast<double>(out.spectral_tensor_count));

  std::vector<double> effective_ranks{};
  effective_ranks.reserve(low_effective_rank_candidates.size());
  double inv_rank_sum = 0.0;
  for (const auto& item : low_effective_rank_candidates) {
    const double rank = std::max(0.0, item.value);
    effective_ranks.push_back(rank);
    inv_rank_sum += 1.0 / (rank + kNumericEpsilon);
  }
  out.network_capacity_tensor_count =
      static_cast<std::uint64_t>(effective_ranks.size());
  if (!effective_ranks.empty() && inv_rank_sum > 0.0) {
    out.network_global_entropic_capacity =
        static_cast<double>(effective_ranks.size()) / inv_rank_sum;
    out.network_entropic_bottleneck_min =
        *std::min_element(effective_ranks.begin(), effective_ranks.end());
    out.network_effective_rank_p50 = sorted_quantile_(effective_ranks, 0.50);
    out.network_effective_rank_p90 = sorted_quantile_(effective_ranks, 0.90);
  }

  const std::size_t top_k = static_cast<std::size_t>(effective.anomaly_top_k);
  out.top_nonfinite_ratio_tensors =
      take_top_k_(std::move(nonfinite_candidates), top_k, /*descending=*/true);
  out.top_max_abs_over_rms_tensors =
      take_top_k_(std::move(max_abs_over_rms_candidates), top_k, /*descending=*/true);
  out.top_near_zero_ratio_tensors =
      take_top_k_(std::move(near_zero_candidates), top_k, /*descending=*/true);
  out.top_low_stable_rank_tensors =
      take_top_k_(std::move(low_stable_rank_candidates), top_k, /*descending=*/false);
  out.top_low_effective_rank_tensors =
      take_top_k_(std::move(low_effective_rank_candidates), top_k, /*descending=*/false);
  out.top_high_spectral_norm_tensors =
      take_top_k_(std::move(high_spectral_norm_candidates), top_k, /*descending=*/true);

  return out;
}

network_design_analytics_report_t summarize_network_design_analytics(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction) {
  network_design_analytics_report_t out{};
  out.network_id = instruction.network_id;
  out.join_policy = instruction.join_policy;
  out.node_count = static_cast<std::uint64_t>(instruction.nodes.size());
  out.export_count = static_cast<std::uint64_t>(instruction.exports.size());

  const std::size_t n = instruction.nodes.size();
  if (n == 0) {
    out.topological_order_count = 0;
    out.topological_order_ratio = 0.0;
    out.active_fanout_mean = 0.0;
    out.edge_surplus = 0.0;
    out.explicit_edge_count = 0;
    out.inferred_edge_count = out.internal_edge_count;
    return out;
  }

  std::unordered_map<std::string, std::size_t> node_index;
  node_index.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    node_index[instruction.nodes[i].id] = i;
  }

  std::vector<std::unordered_set<std::size_t>> internal_out(n);
  std::vector<std::uint64_t> in_degree(n, 0ULL);
  std::vector<std::uint64_t> out_degree_internal(n, 0ULL);
  std::vector<std::uint64_t> out_degree_export(n, 0ULL);

  std::unordered_map<std::string, std::uint64_t> node_kind_counts;
  std::unordered_map<std::string, std::uint64_t> edge_transition_counts;

  node_kind_counts.reserve(n);
  for (const auto& node : instruction.nodes) {
    ++node_kind_counts[node.kind];
  }
  out.distinct_node_kind_count =
      static_cast<std::uint64_t>(node_kind_counts.size());

  out.unresolved_identifier_token_count = 0;
  out.self_reference_token_count = 0;

  for (std::size_t dst = 0; dst < n; ++dst) {
    const auto& node = instruction.nodes[dst];
    for (const auto& param : node.params) {
      ++out.parameter_count;
      const auto tokens = extract_identifier_tokens_(param.value);
      std::unordered_set<std::size_t> source_candidates{};
      for (const auto& token : tokens) {
        const auto it = node_index.find(token);
        if (it == node_index.end()) {
          ++out.unresolved_identifier_token_count;
          continue;
        }
        if (it->second == dst) {
          ++out.self_reference_token_count;
          continue;
        }
        source_candidates.insert(it->second);
      }
      for (const std::size_t src : source_candidates) {
        if (!internal_out[src].insert(dst).second) continue;
        ++in_degree[dst];
        ++out_degree_internal[src];
      }
    }
  }

  out.internal_edge_count = 0;
  for (const auto& targets : internal_out) {
    out.internal_edge_count += static_cast<std::uint64_t>(targets.size());
  }

  out.export_edge_count = 0;
  for (const auto& ex : instruction.exports) {
    const auto it = node_index.find(ex.node_id);
    if (it == node_index.end()) continue;
    ++out_degree_export[it->second];
    ++out.export_edge_count;
    const std::string relation = instruction.nodes[it->second].kind + "->EXPORT";
    ++edge_transition_counts[relation];
  }

  for (std::size_t src = 0; src < n; ++src) {
    for (const std::size_t dst : internal_out[src]) {
      const std::string relation =
          instruction.nodes[src].kind + "->" + instruction.nodes[dst].kind;
      ++edge_transition_counts[relation];
    }
  }

  out.total_edge_count = out.internal_edge_count + out.export_edge_count;
  out.explicit_edge_count = 0;
  out.inferred_edge_count = out.internal_edge_count;

  std::vector<std::uint64_t> out_degree_total(n, 0ULL);
  std::uint64_t isolated = 0;
  std::uint64_t max_in = 0;
  std::uint64_t max_out = 0;
  long double sum_in = 0.0L;
  long double sum_out = 0.0L;
  long double branch_sum = 0.0L;
  std::uint64_t branch_count = 0;

  out.source_count = 0;
  out.internal_sink_count = 0;
  out.branch_node_count = 0;
  out.merge_node_count = 0;

  for (std::size_t i = 0; i < n; ++i) {
    out_degree_total[i] = out_degree_internal[i] + out_degree_export[i];
    if (in_degree[i] == 0 && out_degree_total[i] == 0) ++isolated;
    if (in_degree[i] == 0) ++out.source_count;
    if (out_degree_internal[i] == 0) ++out.internal_sink_count;
    if (out_degree_internal[i] > 1) ++out.branch_node_count;
    if (in_degree[i] > 1) ++out.merge_node_count;

    max_in = std::max(max_in, in_degree[i]);
    max_out = std::max(max_out, out_degree_total[i]);
    sum_in += static_cast<long double>(in_degree[i]);
    sum_out += static_cast<long double>(out_degree_total[i]);
    if (out_degree_total[i] > 0) {
      branch_sum += static_cast<long double>(out_degree_total[i]);
      ++branch_count;
    }
  }

  out.isolated_node_count = isolated;
  out.max_in_degree = max_in;
  out.max_out_degree = max_out;
  out.mean_in_degree = safe_ratio(static_cast<double>(sum_in), static_cast<double>(n));
  out.mean_out_degree = safe_ratio(static_cast<double>(sum_out), static_cast<double>(n));
  out.active_fanout_mean =
      safe_ratio(static_cast<double>(branch_sum), static_cast<double>(branch_count));

  if (n > 1) {
    const long double max_edges =
        static_cast<long double>(n) * static_cast<long double>(n - 1);
    out.internal_edge_density = clamp01(safe_ratio(
        static_cast<double>(out.internal_edge_count),
        static_cast<double>(max_edges)));
  }

  std::vector<std::vector<std::size_t>> internal_adj(n);
  for (std::size_t src = 0; src < n; ++src) {
    internal_adj[src].reserve(internal_out[src].size());
    for (const std::size_t dst : internal_out[src]) {
      internal_adj[src].push_back(dst);
    }
  }

  std::vector<std::vector<std::size_t>> undirected(n);
  std::vector<std::vector<std::size_t>> reverse_adj(n);
  for (std::size_t src = 0; src < n; ++src) {
    for (const std::size_t dst : internal_adj[src]) {
      undirected[src].push_back(dst);
      undirected[dst].push_back(src);
      reverse_adj[dst].push_back(src);
    }
  }

  std::vector<unsigned char> visited(n, static_cast<unsigned char>(0));
  std::uint64_t components = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (visited[i] != 0) continue;
    ++components;
    std::queue<std::size_t> q;
    q.push(i);
    visited[i] = 1;
    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      for (const auto v : undirected[u]) {
        if (visited[v] != 0) continue;
        visited[v] = 1;
        q.push(v);
      }
    }
  }
  out.weakly_connected_components = components;
  out.edge_surplus = static_cast<double>(
      static_cast<long double>(out.internal_edge_count) -
      static_cast<long double>(n) +
      static_cast<long double>(components));

  std::vector<std::uint64_t> in_degree_work = in_degree;
  std::queue<std::size_t> topo_queue;
  for (std::size_t i = 0; i < n; ++i) {
    if (in_degree_work[i] == 0) topo_queue.push(i);
  }
  std::vector<std::size_t> topo_order;
  topo_order.reserve(n);
  while (!topo_queue.empty()) {
    const std::size_t u = topo_queue.front();
    topo_queue.pop();
    topo_order.push_back(u);
    for (const auto v : internal_adj[u]) {
      if (in_degree_work[v] == 0) continue;
      --in_degree_work[v];
      if (in_degree_work[v] == 0) topo_queue.push(v);
    }
  }

  out.topological_order_count = static_cast<std::uint64_t>(topo_order.size());
  out.topological_order_ratio = safe_ratio(
      static_cast<double>(out.topological_order_count),
      static_cast<double>(n));
  out.has_internal_cycle = (topo_order.size() != n);
  if (!out.has_internal_cycle) {
    std::vector<std::uint64_t> longest_to(n, static_cast<std::uint64_t>(1));
    for (const std::size_t u : topo_order) {
      for (const auto v : internal_adj[u]) {
        longest_to[v] =
            std::max(longest_to[v], longest_to[u] + static_cast<std::uint64_t>(1));
      }
    }
    out.acyclic_longest_path_nodes =
        *std::max_element(longest_to.begin(), longest_to.end());
  }

  {
    std::vector<unsigned char> export_reachable(n, static_cast<unsigned char>(0));
    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < n; ++i) {
      if (out_degree_export[i] == 0) continue;
      if (export_reachable[i] != 0) continue;
      export_reachable[i] = 1;
      q.push(i);
    }
    while (!q.empty()) {
      const std::size_t node = q.front();
      q.pop();
      for (const std::size_t pred : reverse_adj[node]) {
        if (export_reachable[pred] != 0) continue;
        export_reachable[pred] = 1;
        q.push(pred);
      }
    }

    std::uint64_t reachable_count = 0;
    for (const auto flag : export_reachable) {
      if (flag != 0) ++reachable_count;
    }
    out.export_reachable_node_count = reachable_count;
    out.export_reachable_ratio = safe_ratio(
        static_cast<double>(reachable_count),
        static_cast<double>(n));
    out.dead_end_node_count = static_cast<std::uint64_t>(n) - reachable_count;
  }

  {
    std::vector<unsigned char> source_reachable(n, static_cast<unsigned char>(0));
    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < n; ++i) {
      if (in_degree[i] != 0) continue;
      if (source_reachable[i] != 0) continue;
      source_reachable[i] = 1;
      q.push(i);
    }
    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      for (const std::size_t v : internal_adj[u]) {
        if (source_reachable[v] != 0) continue;
        source_reachable[v] = 1;
        q.push(v);
      }
    }

    std::uint64_t orphan_count = 0;
    for (const auto flag : source_reachable) {
      if (flag == 0) ++orphan_count;
    }
    out.orphan_node_count = orphan_count;
  }

  std::vector<int> tarjan_index(n, -1);
  std::vector<int> tarjan_low(n, -1);
  std::vector<unsigned char> tarjan_on_stack(n, static_cast<unsigned char>(0));
  std::vector<std::size_t> tarjan_stack{};
  std::vector<int> node_to_scc(n, -1);
  std::vector<std::vector<std::size_t>> scc_nodes{};
  int next_tarjan_index = 0;

  std::function<void(std::size_t)> strong_connect = [&](std::size_t v) {
    tarjan_index[v] = next_tarjan_index;
    tarjan_low[v] = next_tarjan_index;
    ++next_tarjan_index;

    tarjan_stack.push_back(v);
    tarjan_on_stack[v] = 1;

    for (const std::size_t w : internal_adj[v]) {
      if (tarjan_index[w] < 0) {
        strong_connect(w);
        tarjan_low[v] = std::min(tarjan_low[v], tarjan_low[w]);
      } else if (tarjan_on_stack[w] != 0) {
        tarjan_low[v] = std::min(tarjan_low[v], tarjan_index[w]);
      }
    }

    if (tarjan_low[v] != tarjan_index[v]) return;

    const int scc_id = static_cast<int>(scc_nodes.size());
    std::vector<std::size_t> component{};
    while (!tarjan_stack.empty()) {
      const std::size_t w = tarjan_stack.back();
      tarjan_stack.pop_back();
      tarjan_on_stack[w] = 0;
      node_to_scc[w] = scc_id;
      component.push_back(w);
      if (w == v) break;
    }
    scc_nodes.push_back(std::move(component));
  };

  for (std::size_t v = 0; v < n; ++v) {
    if (tarjan_index[v] < 0) strong_connect(v);
  }

  out.scc_count = static_cast<std::uint64_t>(scc_nodes.size());
  out.largest_scc_size = 0;
  out.cyclic_node_count = 0;
  for (const auto& component : scc_nodes) {
    out.largest_scc_size =
        std::max(out.largest_scc_size, static_cast<std::uint64_t>(component.size()));
    bool has_self_loop = false;
    if (component.size() == 1) {
      const std::size_t u = component.front();
      for (const std::size_t v : internal_adj[u]) {
        if (v == u) {
          has_self_loop = true;
          break;
        }
      }
    }
    if (component.size() > 1 || has_self_loop) {
      out.cyclic_node_count += static_cast<std::uint64_t>(component.size());
    }
  }
  out.cyclic_node_ratio = safe_ratio(
      static_cast<double>(out.cyclic_node_count),
      static_cast<double>(n));

  const std::size_t scc_n = scc_nodes.size();
  std::vector<std::unordered_set<std::size_t>> dag_out_sets(scc_n);
  std::vector<std::vector<std::size_t>> dag_out(scc_n);
  std::vector<std::uint64_t> dag_in_degree(scc_n, 0ULL);

  for (std::size_t src = 0; src < n; ++src) {
    const std::size_t src_scc = static_cast<std::size_t>(node_to_scc[src]);
    for (const std::size_t dst : internal_adj[src]) {
      const std::size_t dst_scc = static_cast<std::size_t>(node_to_scc[dst]);
      if (src_scc == dst_scc) continue;
      if (!dag_out_sets[src_scc].insert(dst_scc).second) continue;
      dag_out[src_scc].push_back(dst_scc);
      ++dag_in_degree[dst_scc];
    }
  }

  std::vector<unsigned char> source_scc(scc_n, static_cast<unsigned char>(0));
  std::vector<unsigned char> export_scc(scc_n, static_cast<unsigned char>(0));
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t sid = static_cast<std::size_t>(node_to_scc[i]);
    if (in_degree[i] == 0) source_scc[sid] = 1;
    if (out_degree_export[i] > 0) export_scc[sid] = 1;
  }

  std::vector<std::uint64_t> dag_in_work = dag_in_degree;
  std::queue<std::size_t> dag_queue;
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (dag_in_work[sid] == 0) dag_queue.push(sid);
  }
  std::vector<std::size_t> dag_topo_order{};
  dag_topo_order.reserve(scc_n);
  while (!dag_queue.empty()) {
    const std::size_t sid = dag_queue.front();
    dag_queue.pop();
    dag_topo_order.push_back(sid);
    for (const std::size_t tid : dag_out[sid]) {
      if (dag_in_work[tid] == 0) continue;
      --dag_in_work[tid];
      if (dag_in_work[tid] == 0) dag_queue.push(tid);
    }
  }

  std::vector<unsigned char> path_reachable(scc_n, static_cast<unsigned char>(0));
  std::vector<std::uint64_t> path_nodes(scc_n, 0ULL);
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (source_scc[sid] == 0) continue;
    path_reachable[sid] = 1;
    path_nodes[sid] = static_cast<std::uint64_t>(scc_nodes[sid].size());
  }

  for (const std::size_t sid : dag_topo_order) {
    for (const std::size_t tid : dag_out[sid]) {
      if (path_reachable[sid] == 0) continue;
      const std::uint64_t candidate =
          path_nodes[sid] + static_cast<std::uint64_t>(scc_nodes[tid].size());
      if (path_reachable[tid] == 0 || candidate > path_nodes[tid]) {
        path_reachable[tid] = 1;
        path_nodes[tid] = candidate;
      }
    }
  }

  out.longest_source_to_export_path_nodes = 0;
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (export_scc[sid] == 0) continue;
    if (path_reachable[sid] == 0) continue;
    out.longest_source_to_export_path_nodes =
        std::max(out.longest_source_to_export_path_nodes, path_nodes[sid]);
  }

  {
    std::vector<std::uint64_t> export_lengths{};
    export_lengths.reserve(instruction.exports.size());
    for (const auto& ex : instruction.exports) {
      const auto it = node_index.find(ex.node_id);
      if (it == node_index.end()) continue;
      const std::size_t sid = static_cast<std::size_t>(node_to_scc[it->second]);
      if (path_reachable[sid] == 0) continue;
      export_lengths.push_back(path_nodes[sid]);
    }

    if (!export_lengths.empty()) {
      std::sort(export_lengths.begin(), export_lengths.end());
      const std::size_t mid = export_lengths.size() / 2;
      if ((export_lengths.size() % 2) == 1) {
        out.median_source_to_export_path_nodes = export_lengths[mid];
      } else {
        out.median_source_to_export_path_nodes =
            (export_lengths[mid - 1] + export_lengths[mid]) / 2;
      }
    }
  }

  {
    std::vector<unsigned char> depth_reachable(scc_n, static_cast<unsigned char>(0));
    std::vector<std::uint64_t> depth(scc_n, 0ULL);
    for (std::size_t sid = 0; sid < scc_n; ++sid) {
      if (source_scc[sid] == 0) continue;
      depth_reachable[sid] = 1;
      depth[sid] = 0;
    }

    for (const std::size_t sid : dag_topo_order) {
      for (const std::size_t tid : dag_out[sid]) {
        if (depth_reachable[sid] == 0) continue;
        const std::uint64_t candidate = depth[sid] + 1;
        if (depth_reachable[tid] == 0 || candidate > depth[tid]) {
          depth_reachable[tid] = 1;
          depth[tid] = candidate;
        }
      }
    }

    std::uint64_t skip_count = 0;
    long double skip_span_sum = 0.0L;
    std::uint64_t max_skip_span = 0;
    for (std::size_t sid = 0; sid < scc_n; ++sid) {
      if (depth_reachable[sid] == 0) continue;
      for (const std::size_t tid : dag_out[sid]) {
        if (depth_reachable[tid] == 0) continue;
        if (depth[tid] <= depth[sid]) continue;
        const std::uint64_t span = depth[tid] - depth[sid];
        if (span <= 1) continue;
        ++skip_count;
        skip_span_sum += static_cast<long double>(span);
        max_skip_span = std::max(max_skip_span, span);
      }
    }

    out.skip_edge_count = skip_count;
    out.skip_edge_ratio = safe_ratio(
        static_cast<double>(out.skip_edge_count),
        static_cast<double>(out.internal_edge_count));
    out.mean_skip_span = safe_ratio(
        static_cast<double>(skip_span_sum),
        static_cast<double>(out.skip_edge_count));
    out.max_skip_span = max_skip_span;
  }

  std::unordered_map<std::uint64_t, std::uint64_t> in_degree_counts;
  std::unordered_map<std::uint64_t, std::uint64_t> out_degree_counts;
  for (std::size_t i = 0; i < n; ++i) {
    ++in_degree_counts[in_degree[i]];
    ++out_degree_counts[out_degree_total[i]];
  }

  out.node_kind_entropy = normalized_shannon_entropy_from_map_(node_kind_counts);
  out.in_degree_entropy = normalized_shannon_entropy_from_map_(in_degree_counts);
  out.out_degree_entropy = normalized_shannon_entropy_from_map_(out_degree_counts);
  out.edge_kind_transition_entropy =
      normalized_shannon_entropy_from_map_(edge_transition_counts);

  return out;
}

bool resolve_network_analytics_options_from_network_design(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction,
    network_analytics_options_t* out_options,
    std::string* error) {
  if (error) error->clear();
  if (out_options == nullptr) {
    if (error) {
      *error =
          "resolve_network_analytics_options_from_network_design requires non-null output";
    }
    return false;
  }

  const auto policy_nodes =
      instruction.find_nodes_by_kind("NETWORK_ANALYTICS_POLICY");
  if (policy_nodes.empty()) {
    if (error) {
      *error =
          "network_design is missing required node kind NETWORK_ANALYTICS_POLICY";
    }
    return false;
  }
  if (policy_nodes.size() != 1) {
    if (error) {
      *error =
          "network_design must declare exactly one NETWORK_ANALYTICS_POLICY node";
    }
    return false;
  }

  const auto& node = *policy_nodes.front();
  std::unordered_map<std::string, std::string> kv{};
  kv.reserve(node.params.size());

  const auto is_known_key = [](const std::string& key) {
    return key == "near_zero_epsilon" ||
           key == "log10_abs_histogram_bins" ||
           key == "log10_abs_histogram_min" ||
           key == "log10_abs_histogram_max" || key == "include_buffers" ||
           key == "enable_spectral_metrics" ||
           key == "spectral_max_elements" || key == "anomaly_top_k";
  };

  for (const auto& param : node.params) {
    const std::string key =
        lower_ascii_copy_(std::string(trim_ascii_ws_view_(param.key)));
    const std::string value =
        std::string(trim_ascii_ws_view_(param.value));
    if (key.empty()) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY contains an empty parameter key";
      }
      return false;
    }
    if (!is_known_key(key)) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY has unknown key `" + key + "`";
      }
      return false;
    }
    if (!kv.emplace(key, value).second) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY duplicates key `" + key + "`";
      }
      return false;
    }
  }

  const auto require_value = [&](const char* key, std::string* out_value) {
    if (out_value) out_value->clear();
    const auto it = kv.find(key);
    if (it == kv.end()) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY missing required key `" +
            std::string(key) + "`";
      }
      return false;
    }
    if (out_value) *out_value = it->second;
    return true;
  };

  std::string raw{};
  network_analytics_options_t parsed{};

  if (!require_value("near_zero_epsilon", &raw) ||
      !parse_double_strict_(raw, &parsed.near_zero_epsilon)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.near_zero_epsilon must be finite float";
    }
    return false;
  }
  if (parsed.near_zero_epsilon < 0.0) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.near_zero_epsilon must be >= 0";
    }
    return false;
  }

  if (!require_value("log10_abs_histogram_bins", &raw) ||
      !parse_i64_strict_(raw, &parsed.log10_abs_histogram_bins)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_bins must be int";
    }
    return false;
  }
  if (parsed.log10_abs_histogram_bins < 1) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_bins must be >= 1";
    }
    return false;
  }

  if (!require_value("log10_abs_histogram_min", &raw) ||
      !parse_double_strict_(raw, &parsed.log10_abs_histogram_min)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_min must be finite float";
    }
    return false;
  }
  if (!require_value("log10_abs_histogram_max", &raw) ||
      !parse_double_strict_(raw, &parsed.log10_abs_histogram_max)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_max must be finite float";
    }
    return false;
  }
  if (parsed.log10_abs_histogram_max <= parsed.log10_abs_histogram_min) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_max must be > log10_abs_histogram_min";
    }
    return false;
  }

  if (!require_value("include_buffers", &raw) ||
      !parse_bool_strict_(raw, &parsed.include_buffers)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.include_buffers must be bool";
    }
    return false;
  }

  if (!require_value("enable_spectral_metrics", &raw) ||
      !parse_bool_strict_(raw, &parsed.enable_spectral_metrics)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.enable_spectral_metrics must be bool";
    }
    return false;
  }

  if (!require_value("spectral_max_elements", &raw) ||
      !parse_i64_strict_(raw, &parsed.spectral_max_elements)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.spectral_max_elements must be int";
    }
    return false;
  }
  if (parsed.spectral_max_elements < 1) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.spectral_max_elements must be >= 1";
    }
    return false;
  }

  if (!require_value("anomaly_top_k", &raw) ||
      !parse_i64_strict_(raw, &parsed.anomaly_top_k)) {
    if (error && error->empty()) {
      *error = "NETWORK_ANALYTICS_POLICY.anomaly_top_k must be int";
    }
    return false;
  }
  if (parsed.anomaly_top_k < 0) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.anomaly_top_k must be >= 0";
    }
    return false;
  }

  *out_options = parsed;
  return true;
}

std::string extract_analytics_kv_schema(std::string_view payload) {
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();

    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    line = trim_ascii_ws_view_(line);

    if (!line.empty()) {
      const std::size_t sep = line.find('=');
      if (sep != std::string_view::npos) {
        const std::string_view key = trim_ascii_ws_view_(line.substr(0, sep));
        if (key == "schema") {
          const std::string_view value =
              trim_ascii_ws_view_(line.substr(sep + 1));
          return std::string(value);
        }
      }
    }

    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return {};
}

bool is_supported_network_analytics_schema(std::string_view schema) {
  return schema == kNetworkAnalyticsSchemaV1 ||
         schema == kNetworkAnalyticsSchemaV2 ||
         schema == kNetworkAnalyticsSchemaV3 ||
         schema == kNetworkAnalyticsSchemaV4;
}

bool is_supported_network_design_analytics_schema(std::string_view schema) {
  return schema == kNetworkDesignAnalyticsSchemaV1 ||
         schema == kNetworkDesignAnalyticsSchemaV2 ||
         schema == kNetworkDesignAnalyticsSchemaV3;
}

std::string network_analytics_to_key_value_text(
    const network_analytics_report_t& report,
    const network_analytics_options_t& options,
    std::string_view checkpoint_filename) {
  return as_ascii_key_value_(report, options, checkpoint_filename);
}

std::string network_design_analytics_to_key_value_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label) {
  return as_ascii_key_value_(report, source_label);
}

std::string network_design_analytics_to_pretty_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label,
    bool use_color) {
  return as_pretty_text_(report, source_label, use_color);
}

bool write_network_analytics_file(
    const torch::nn::Module& model,
    const std::filesystem::path& output_file,
    const network_analytics_options_t& options,
    std::string* error) {
  if (error) error->clear();
  const network_analytics_options_t effective = normalize_options_(options);
  network_analytics_report_t report{};
  try {
    report = summarize_module_network_analytics(model, effective);
  } catch (const std::exception& e) {
    if (error) {
      *error = "network analytics summarization failed: " + std::string(e.what());
    }
    return false;
  } catch (...) {
    if (error) *error = "network analytics summarization failed: unknown error";
    return false;
  }

  std::error_code ec;
  auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create report directory: " + parent.string();
      return false;
    }
  }

  std::ofstream out(output_file, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open report file for write: " + output_file.string();
    return false;
  }

  const std::string payload =
      network_analytics_to_key_value_text(report, effective, {});
  out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (!out) {
    if (error) *error = "cannot write report file: " + output_file.string();
    return false;
  }
  return true;
}

bool write_network_analytics_sidecar_for_checkpoint(
    const torch::nn::Module& model,
    const std::filesystem::path& checkpoint_file,
    std::filesystem::path* out_sidecar_file,
    const network_analytics_options_t& options,
    std::string* error) {
  if (error) error->clear();
  const network_analytics_options_t effective = normalize_options_(options);
  std::filesystem::path sidecar = checkpoint_file;
  if (sidecar.extension() == ".pt") {
    sidecar.replace_extension(".network_analytics.kv");
  } else {
    sidecar += ".network_analytics.kv";
  }

  network_analytics_report_t report{};
  try {
    report = summarize_module_network_analytics(model, effective);
  } catch (const std::exception& e) {
    if (error) {
      *error = "network analytics summarization failed: " + std::string(e.what());
    }
    return false;
  } catch (...) {
    if (error) *error = "network analytics summarization failed: unknown error";
    return false;
  }

  std::error_code ec;
  auto parent = sidecar.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create report directory: " + parent.string();
      return false;
    }
  }

  std::ofstream out(sidecar, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open report file for write: " + sidecar.string();
    return false;
  }

  const std::string payload = network_analytics_to_key_value_text(
      report, effective, checkpoint_file.filename().string());
  out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (!out) {
    if (error) *error = "cannot write report file: " + sidecar.string();
    return false;
  }

  if (out_sidecar_file) *out_sidecar_file = sidecar;
  return true;
}

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
