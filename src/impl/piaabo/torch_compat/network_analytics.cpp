/* network_analytics.cpp */
#include "piaabo/torch_compat/network_analytics.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include <torch/torch.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

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
  long double sum_abs_log_abs{0.0L};  // sum(a*log(a)) for a>0

  std::uint64_t near_zero_count{0};
  std::uint64_t positive_count{0};
  std::uint64_t negative_count{0};
  std::uint64_t exact_zero_count{0};
  std::uint64_t non_zero_abs_count{0};

  double min{std::numeric_limits<double>::infinity()};
  double max{-std::numeric_limits<double>::infinity()};
  double max_abs{0.0};
  double max_abs_tensor_value{0.0};
  std::string max_abs_tensor_name{};

  std::vector<double> layer_rms{};
};

[[nodiscard]] inline double safe_ratio(double num, double den) {
  return (den > 0.0) ? (num / den) : 0.0;
}

[[nodiscard]] inline double clamp01(double x) {
  if (x < 0.0) return 0.0;
  if (x > 1.0) return 1.0;
  return x;
}

[[nodiscard]] network_analytics_options_t normalize_options_(
    const network_analytics_options_t& options) {
  network_analytics_options_t out = options;
  if (out.near_zero_epsilon < 0.0) out.near_zero_epsilon = 0.0;
  if (out.log10_abs_histogram_bins < 1) out.log10_abs_histogram_bins = 1;
  if (out.log10_abs_histogram_max <= out.log10_abs_histogram_min) {
    out.log10_abs_histogram_max = out.log10_abs_histogram_min + 1.0;
  }
  return out;
}

void accumulate_parameter_tensor_(
    const std::string& tensor_name,
    const torch::Tensor& param,
    const network_analytics_options_t& options,
    std::vector<std::uint64_t>* hist_counts,
    report_accumulator_t* acc) {
  if (acc == nullptr) return;
  if (!param.defined()) return;

  ++acc->tensor_count;
  if (param.requires_grad()) ++acc->trainable_tensor_count;

  auto p = param.detach().to(torch::kCPU).to(torch::kFloat64).contiguous().flatten();
  const std::int64_t n64 = p.numel();
  if (n64 <= 0) return;
  const std::uint64_t n = static_cast<std::uint64_t>(n64);
  acc->total_parameter_count += n;

  auto nan_mask = torch::isnan(p);
  auto inf_mask = torch::isinf(p);
  auto finite_mask = torch::isfinite(p);

  const std::uint64_t nan_count =
      static_cast<std::uint64_t>(nan_mask.sum().item<std::int64_t>());
  const std::uint64_t inf_count =
      static_cast<std::uint64_t>(inf_mask.sum().item<std::int64_t>());
  const std::uint64_t finite_count =
      static_cast<std::uint64_t>(finite_mask.sum().item<std::int64_t>());

  acc->nan_parameter_count += nan_count;
  acc->inf_parameter_count += inf_count;
  acc->finite_parameter_count += finite_count;

  if (finite_count == 0) return;

  auto finite_values = p.masked_select(finite_mask);
  auto abs_values = finite_values.abs();

  acc->sum += finite_values.sum().item<double>();
  acc->sum_sq += finite_values.pow(2).sum().item<double>();

  const double tensor_min = finite_values.min().item<double>();
  const double tensor_max = finite_values.max().item<double>();
  if (tensor_min < acc->min) acc->min = tensor_min;
  if (tensor_max > acc->max) acc->max = tensor_max;

  const double tensor_max_abs = abs_values.max().item<double>();
  if (tensor_max_abs > acc->max_abs) acc->max_abs = tensor_max_abs;
  if (tensor_max_abs > acc->max_abs_tensor_value) {
    acc->max_abs_tensor_value = tensor_max_abs;
    acc->max_abs_tensor_name = tensor_name;
  }

  const double abs_sum = abs_values.sum().item<double>();
  acc->sum_abs += abs_sum;

  const double eps = std::max(options.near_zero_epsilon, 1e-16);
  auto positive_abs = abs_values.masked_select(abs_values.gt(0.0));
  const std::uint64_t non_zero_abs_count =
      static_cast<std::uint64_t>(positive_abs.numel());
  acc->non_zero_abs_count += non_zero_abs_count;
  if (non_zero_abs_count > 0) {
    acc->sum_abs_log_abs +=
        (positive_abs * positive_abs.log()).sum().item<double>();
  }

  acc->near_zero_count += static_cast<std::uint64_t>(
      abs_values.le(options.near_zero_epsilon).sum().item<std::int64_t>());
  acc->positive_count += static_cast<std::uint64_t>(
      finite_values.gt(0.0).sum().item<std::int64_t>());
  acc->negative_count += static_cast<std::uint64_t>(
      finite_values.lt(0.0).sum().item<std::int64_t>());
  acc->exact_zero_count += static_cast<std::uint64_t>(
      finite_values.eq(0.0).sum().item<std::int64_t>());

  const double tensor_rms =
      std::sqrt(std::max(0.0, finite_values.pow(2).mean().item<double>()));
  acc->layer_rms.push_back(tensor_rms);

  if (hist_counts != nullptr && !hist_counts->empty()) {
    const std::int64_t bins = static_cast<std::int64_t>(hist_counts->size());
    const double hmin = options.log10_abs_histogram_min;
    const double hmax = options.log10_abs_histogram_max;
    if (bins > 1 && hmax > hmin) {
      auto logabs = (abs_values + eps).log10().clamp(hmin, hmax);
      auto scaled = ((logabs - hmin) / (hmax - hmin)) * static_cast<double>(bins);
      auto idx = torch::clamp(scaled.to(torch::kLong), 0, bins - 1);
      auto bincount = torch::bincount(idx, c10::nullopt, bins).to(torch::kCPU);
      for (std::int64_t i = 0; i < bins; ++i) {
        (*hist_counts)[static_cast<std::size_t>(i)] +=
            static_cast<std::uint64_t>(bincount[i].item<std::int64_t>());
      }
    }
  }
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
  const long double h =
      std::log(sum_abs) - (sum_abs_log_abs / sum_abs);
  const long double norm =
      std::log(static_cast<long double>(non_zero_abs_count));
  if (norm <= 0.0L) return 0.0;
  return clamp01(static_cast<double>(h / norm));
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
  oss << "mean=" << report.mean << "\n";
  oss << "stddev=" << report.stddev << "\n";
  oss << "min=" << report.min << "\n";
  oss << "max=" << report.max << "\n";
  oss << "l1_mean_abs=" << report.l1_mean_abs << "\n";
  oss << "l2_rms=" << report.l2_rms << "\n";
  oss << "max_abs=" << report.max_abs << "\n";
  oss << "max_abs_over_rms=" << report.max_abs_over_rms << "\n";
  oss << "near_zero_threshold=" << report.near_zero_threshold << "\n";
  oss << "near_zero_ratio=" << report.near_zero_ratio << "\n";
  oss << "positive_ratio=" << report.positive_ratio << "\n";
  oss << "negative_ratio=" << report.negative_ratio << "\n";
  oss << "exact_zero_ratio=" << report.exact_zero_ratio << "\n";
  oss << "abs_energy_entropy=" << report.abs_energy_entropy << "\n";
  oss << "log10_abs_histogram_entropy=" << report.log10_abs_histogram_entropy
      << "\n";
  oss << "log10_abs_histogram_bins=" << options.log10_abs_histogram_bins
      << "\n";
  oss << "log10_abs_histogram_min=" << options.log10_abs_histogram_min << "\n";
  oss << "log10_abs_histogram_max=" << options.log10_abs_histogram_max << "\n";
  oss << "layer_rms_mean=" << report.layer_rms_mean << "\n";
  oss << "layer_rms_std=" << report.layer_rms_std << "\n";
  oss << "layer_rms_cv=" << report.layer_rms_cv << "\n";
  oss << "layer_rms_min=" << report.layer_rms_min << "\n";
  oss << "layer_rms_max=" << report.layer_rms_max << "\n";
  oss << "layer_rms_max_over_min=" << report.layer_rms_max_over_min << "\n";
  oss << "max_abs_tensor_name=" << report.max_abs_tensor_name << "\n";
  oss << "max_abs_tensor_value=" << report.max_abs_tensor_value << "\n";

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
  oss << "has_internal_cycle=" << (report.has_internal_cycle ? "true" : "false")
      << "\n";
  oss << "topological_order_coverage=" << report.topological_order_coverage
      << "\n";
  oss << "acyclic_longest_path_nodes=" << report.acyclic_longest_path_nodes
      << "\n";
  oss << "internal_edge_density=" << report.internal_edge_density << "\n";
  oss << "cyclomatic_complexity=" << report.cyclomatic_complexity << "\n";
  oss << "mean_in_degree=" << report.mean_in_degree << "\n";
  oss << "mean_out_degree=" << report.mean_out_degree << "\n";
  oss << "max_in_degree=" << report.max_in_degree << "\n";
  oss << "max_out_degree=" << report.max_out_degree << "\n";
  oss << "branching_factor=" << report.branching_factor << "\n";
  oss << "distinct_node_kind_count=" << report.distinct_node_kind_count << "\n";
  oss << "node_kind_entropy=" << report.node_kind_entropy << "\n";
  oss << "in_degree_entropy=" << report.in_degree_entropy << "\n";
  oss << "out_degree_entropy=" << report.out_degree_entropy << "\n";
  oss << "edge_kind_transition_entropy=" << report.edge_kind_transition_entropy
      << "\n";
  oss << "parameter_key_entropy=" << report.parameter_key_entropy << "\n";
  oss << "parameter_value_token_entropy=" << report.parameter_value_token_entropy
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
  const char* c_warn = use_color ? "\x1b[93m" : "";
  const char* c_bad = use_color ? "\x1b[91m" : "";

  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  auto line = [&](std::string_view key,
                  const auto& value,
                  std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset
        << " : " << c_value << value << c_reset
        << "\t" << c_note << note << c_reset << "\n";
  };
  auto line_color = [&](std::string_view key,
                        const auto& value,
                        const char* color,
                        std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset
        << " : " << color << value << c_reset
        << "\t" << c_note << note << c_reset << "\n";
  };
  auto section = [&](std::string_view name) {
    oss << c_section << name << c_reset << "\n";
  };

  oss << c_title << "Network Design Analytics Report" << c_reset << "\n";
  if (!source_label.empty()) {
    line("source_label", source_label, "contract/key/path reference");
  }
  oss << "\t" << c_key << std::left << std::setw(34) << "metric" << c_reset
      << " : " << c_value << "value" << c_reset
      << "\t" << c_note << "comment + best guidance" << c_reset << "\n";
  oss << "\n";

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
             report.has_internal_cycle ? "true" : "false",
             report.has_internal_cycle ? c_bad : c_good,
             "cycle in internal graph (best: false for feed-forward)");
  line("topological_order_coverage",
       report.topological_order_coverage,
       "nodes sortable without cycles (best: node_count)");
  line("acyclic_longest_path_nodes",
       report.acyclic_longest_path_nodes,
       "DAG depth proxy (best: task-dependent)");
  oss << "\n";

  section("Complexity");
  line("internal_edge_density",
       report.internal_edge_density,
       "normalized connectivity [0..1] (best: task-dependent)");
  line("cyclomatic_complexity",
       report.cyclomatic_complexity,
       "E-N+C control complexity proxy (best: lower if equivalent)");
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
  line_color("branching_factor",
             report.branching_factor,
             (report.branching_factor > 1.5) ? c_warn : c_good,
             "avg out-degree for branching nodes (best: moderate)");
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
  line("parameter_key_entropy",
       report.parameter_key_entropy,
       "diversity of param keys (best: task-dependent)");
  line("parameter_value_token_entropy",
       report.parameter_value_token_entropy,
       "diversity of param value tokens (best: task-dependent)");

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
  std::vector<std::uint64_t> hist_counts(
      static_cast<std::size_t>(bins),
      0ULL);
  report_accumulator_t acc{};

  for (const auto& named_param : model.named_parameters(/*recurse=*/true)) {
    const std::string tensor_name = named_param.key();
    const auto& param = named_param.value();
    accumulate_parameter_tensor_(tensor_name, param, effective, &hist_counts, &acc);
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
    const long double finite_ld =
        static_cast<long double>(out.finite_parameter_count);
    out.mean = static_cast<double>(acc.sum / finite_ld);
    const double ex2 =
        static_cast<double>(acc.sum_sq / finite_ld);
    out.stddev = std::sqrt(std::max(0.0, ex2 - out.mean * out.mean));
    out.min = std::isfinite(acc.min) ? acc.min : 0.0;
    out.max = std::isfinite(acc.max) ? acc.max : 0.0;

    out.l1_mean_abs =
        static_cast<double>(acc.sum_abs / finite_ld);
    out.l2_rms = std::sqrt(std::max(
        0.0,
        static_cast<double>(acc.sum_sq / finite_ld)));
    out.max_abs = acc.max_abs;
    out.max_abs_over_rms =
        safe_ratio(out.max_abs, out.l2_rms + 1e-18);

    out.near_zero_ratio = safe_ratio(
        static_cast<double>(acc.near_zero_count),
        finite);
    out.positive_ratio = safe_ratio(
        static_cast<double>(acc.positive_count),
        finite);
    out.negative_ratio = safe_ratio(
        static_cast<double>(acc.negative_count),
        finite);
    out.exact_zero_ratio = safe_ratio(
        static_cast<double>(acc.exact_zero_count),
        finite);

    out.abs_energy_entropy = normalized_abs_energy_entropy_(
        acc.non_zero_abs_count,
        acc.sum_abs,
        acc.sum_abs_log_abs);
    out.log10_abs_histogram_entropy =
        normalized_histogram_entropy_(hist_counts, out.finite_parameter_count);
  }

  if (!acc.layer_rms.empty()) {
    const double m = std::accumulate(
        acc.layer_rms.begin(), acc.layer_rms.end(), 0.0) /
        static_cast<double>(acc.layer_rms.size());
    double var = 0.0;
    for (const double v : acc.layer_rms) {
      const double d = v - m;
      var += d * d;
    }
    var /= static_cast<double>(acc.layer_rms.size());
    out.layer_rms_mean = m;
    out.layer_rms_std = std::sqrt(std::max(0.0, var));
    out.layer_rms_cv = safe_ratio(out.layer_rms_std, std::abs(m) + 1e-18);
    auto minmax = std::minmax_element(acc.layer_rms.begin(), acc.layer_rms.end());
    out.layer_rms_min = *minmax.first;
    out.layer_rms_max = *minmax.second;
    out.layer_rms_max_over_min =
        safe_ratio(out.layer_rms_max, out.layer_rms_min + 1e-18);
  }

  out.max_abs_tensor_name = acc.max_abs_tensor_name;
  out.max_abs_tensor_value = acc.max_abs_tensor_value;

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
  if (n == 0) return out;

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
  std::unordered_map<std::string, std::uint64_t> parameter_key_counts;
  std::unordered_map<std::string, std::uint64_t> parameter_token_counts;

  node_kind_counts.reserve(n);
  for (const auto& node : instruction.nodes) {
    ++node_kind_counts[node.kind];
  }
  out.distinct_node_kind_count =
      static_cast<std::uint64_t>(node_kind_counts.size());

  for (std::size_t dst = 0; dst < n; ++dst) {
    const auto& node = instruction.nodes[dst];
    for (const auto& param : node.params) {
      ++out.parameter_count;
      ++parameter_key_counts[param.key];
      const auto tokens = extract_identifier_tokens_(param.value);
      std::unordered_set<std::size_t> source_candidates{};
      for (const auto& token : tokens) {
        ++parameter_token_counts[token];
        const auto it = node_index.find(token);
        if (it == node_index.end()) continue;
        if (it->second == dst) continue;
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
    const std::string relation =
        instruction.nodes[it->second].kind + "->EXPORT";
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

  std::vector<std::uint64_t> out_degree_total(n, 0ULL);
  std::uint64_t isolated = 0;
  std::uint64_t max_in = 0;
  std::uint64_t max_out = 0;
  long double sum_in = 0.0L;
  long double sum_out = 0.0L;
  long double branch_sum = 0.0L;
  std::uint64_t branch_count = 0;
  for (std::size_t i = 0; i < n; ++i) {
    out_degree_total[i] = out_degree_internal[i] + out_degree_export[i];
    if (in_degree[i] == 0 && out_degree_total[i] == 0) ++isolated;
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
  out.mean_in_degree = safe_ratio(
      static_cast<double>(sum_in),
      static_cast<double>(n));
  out.mean_out_degree = safe_ratio(
      static_cast<double>(sum_out),
      static_cast<double>(n));
  out.branching_factor = safe_ratio(
      static_cast<double>(branch_sum),
      static_cast<double>(branch_count));

  if (n > 1) {
    const long double max_edges =
        static_cast<long double>(n) * static_cast<long double>(n - 1);
    out.internal_edge_density = clamp01(
        safe_ratio(static_cast<double>(out.internal_edge_count),
                   static_cast<double>(max_edges)));
  }

  std::vector<std::vector<std::size_t>> undirected(n);
  for (std::size_t src = 0; src < n; ++src) {
    for (const std::size_t dst : internal_out[src]) {
      undirected[src].push_back(dst);
      undirected[dst].push_back(src);
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
  out.cyclomatic_complexity = static_cast<double>(
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
    for (const auto v : internal_out[u]) {
      if (in_degree_work[v] == 0) continue;
      --in_degree_work[v];
      if (in_degree_work[v] == 0) topo_queue.push(v);
    }
  }

  out.topological_order_coverage =
      static_cast<std::uint64_t>(topo_order.size());
  out.has_internal_cycle = (topo_order.size() != n);
  if (!out.has_internal_cycle) {
    std::vector<std::uint64_t> longest_to(n, static_cast<std::uint64_t>(1));
    for (const std::size_t u : topo_order) {
      for (const auto v : internal_out[u]) {
        longest_to[v] = std::max(
            longest_to[v], longest_to[u] + static_cast<std::uint64_t>(1));
      }
    }
    out.acyclic_longest_path_nodes =
        *std::max_element(longest_to.begin(), longest_to.end());
  }

  std::unordered_map<std::uint64_t, std::uint64_t> in_degree_counts;
  std::unordered_map<std::uint64_t, std::uint64_t> out_degree_counts;
  for (std::size_t i = 0; i < n; ++i) {
    ++in_degree_counts[in_degree[i]];
    ++out_degree_counts[out_degree_total[i]];
  }

  out.node_kind_entropy =
      normalized_shannon_entropy_from_map_(node_kind_counts);
  out.in_degree_entropy =
      normalized_shannon_entropy_from_map_(in_degree_counts);
  out.out_degree_entropy =
      normalized_shannon_entropy_from_map_(out_degree_counts);
  out.edge_kind_transition_entropy =
      normalized_shannon_entropy_from_map_(edge_transition_counts);
  out.parameter_key_entropy =
      normalized_shannon_entropy_from_map_(parameter_key_counts);
  out.parameter_value_token_entropy =
      normalized_shannon_entropy_from_map_(parameter_token_counts);

  return out;
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
    if (error) *error = "network analytics summarization failed: " + std::string(e.what());
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

  const std::string payload = network_analytics_to_key_value_text(
      report, effective, {});
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
    if (error) *error = "network analytics summarization failed: " + std::string(e.what());
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
