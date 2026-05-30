// test_network_analytics_metrics.cpp
// Deterministic checks for network analytics canonical fields.

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "piaabo/torch_compat/network_analytics.h"

namespace {

bool expect(bool cond, const std::string &message) {
  if (!cond) {
    std::cerr << "[test_network_analytics_metrics] FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

bool has_lhs_key(const std::string &payload, const std::string &key) {
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string::npos)
      line_end = payload.size();
    std::string line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const std::size_t eq = line.find('=');
    if (eq != std::string::npos) {
      const std::string lhs_key =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              std::string_view(line).substr(0, eq));
      if (lhs_key == key)
        return true;
    }
    if (line_end == payload.size())
      break;
    cursor = line_end + 1;
  }
  return false;
}

struct toy_network_t final : torch::nn::Module {
  toy_network_t() {
    register_parameter("w_diag",
                       torch::tensor({{3.0, 0.0}, {0.0, 1.0}}, torch::kFloat64),
                       true);
    register_parameter("w_sparse",
                       torch::tensor({0.0, 0.0, 0.0, 1e-12}, torch::kFloat64),
                       true);
    register_parameter(
        "w_nonfinite",
        torch::tensor({1.0, std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN()},
                      torch::kFloat64),
        true);
    register_parameter(
        "w_oversize",
        torch::ones({2, 3}, torch::TensorOptions().dtype(torch::kFloat64)),
        true);
    register_parameter("w_lowrank",
                       torch::tensor({{1.0, 0.0}, {0.0, 0.0}}, torch::kFloat64),
                       true);

    register_buffer("buf_ok", torch::tensor({1.0, -2.0}, torch::kFloat64));
    register_buffer(
        "buf_nonfinite",
        torch::tensor({0.0, std::numeric_limits<double>::infinity()},
                      torch::kFloat64));
  }
};

struct histogram_quantile_network_t final : torch::nn::Module {
  histogram_quantile_network_t() {
    register_parameter(
        "tiny", torch::tensor({1e-3, 1e-2, 1e-1}, torch::kFloat64), true);
  }
};

} // namespace

int main() try {
  bool ok = true;

  toy_network_t model{};
  cuwacunu::piaabo::torch_compat::network_analytics_options_t options{};
  options.spectral_max_elements = 4;
  options.anomaly_top_k = 3;

  const auto report =
      cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
          model, options);

  ok = ok &&
       expect(report.schema == std::string(cuwacunu::piaabo::torch_compat::
                                               kNetworkAnalyticsSchemaCurrent),
              "network analytics schema should be current");
  ok = ok && expect(report.total_parameter_count == 21,
                    "total parameter count mismatch");
  ok = ok && expect(report.finite_parameter_count == 19,
                    "finite parameter count mismatch");
  ok = ok &&
       expect(report.nan_parameter_count == 1, "nan parameter count mismatch");
  ok = ok &&
       expect(report.inf_parameter_count == 1, "inf parameter count mismatch");
  ok = ok && expect(approx(report.finite_ratio, 19.0 / 21.0, 1e-10),
                    "finite ratio mismatch");
  ok = ok && expect(approx(report.non_finite_ratio, 2.0 / 21.0, 1e-10),
                    "non-finite ratio mismatch");

  ok = ok &&
       expect(report.tensor_rms_cv > 0.0, "tensor_rms_cv should be populated");
  ok = ok && expect(report.tensor_rms_mean > 0.0,
                    "tensor_rms_mean should be populated");

  ok = ok &&
       expect(report.buffer_tensor_count == 2, "buffer tensor count mismatch");
  ok = ok &&
       expect(report.total_buffer_count == 4, "buffer element count mismatch");
  ok = ok &&
       expect(report.finite_buffer_count == 3, "finite buffer count mismatch");
  ok = ok && expect(report.inf_buffer_count == 1, "inf buffer count mismatch");
  ok = ok && expect(approx(report.finite_buffer_ratio, 0.75, 1e-12),
                    "finite buffer ratio mismatch");
  ok = ok && expect(report.max_abs_buffer_name == "buf_ok",
                    "max abs buffer name mismatch");

  ok = ok &&
       expect(report.matrix_tensor_count == 3, "matrix tensor count mismatch");
  ok = ok && expect(report.spectral_tensor_count == 2,
                    "spectral tensor count mismatch");
  ok = ok && expect(report.spectral_skipped_tensor_count >= 1,
                    "expected at least one spectral skip");
  ok = ok && expect(report.spectral_failed_tensor_count == 0,
                    "unexpected spectral failures");
  ok = ok && expect(approx(report.spectral_norm_max, 3.0, 1e-9),
                    "spectral norm max mismatch");
  ok = ok && expect(report.stable_rank_min >= 0.999,
                    "stable rank min should be near 1");
  ok = ok && expect(report.effective_rank_min >= 0.999,
                    "effective rank min should be near 1");
  ok = ok && expect(report.network_capacity_tensor_count ==
                        report.spectral_tensor_count,
                    "capacity tensor count mismatch");
  ok = ok && expect(report.network_global_entropic_capacity > 0.0,
                    "network global entropic capacity should be positive");
  ok = ok && expect(report.network_effective_rank_p90 >=
                        report.network_effective_rank_p50,
                    "network effective rank quantiles should be ordered");

  ok = ok && expect(!report.top_nonfinite_ratio_tensors.empty(),
                    "top non-finite table should be populated");
  ok = ok && expect(report.top_nonfinite_ratio_tensors.front().tensor_name ==
                        "w_nonfinite",
                    "w_nonfinite should lead top non-finite ratio table");
  ok = ok && expect(!report.top_high_spectral_norm_tensors.empty(),
                    "top spectral-norm table should be populated");
  ok = ok && expect(report.top_high_spectral_norm_tensors.front().tensor_name ==
                        "w_diag",
                    "w_diag should lead top spectral norm table");

  ok = ok && expect(report.normalized_options.spectral_max_elements == 4,
                    "report should store normalized options");
  ok = ok && expect(report.normalized_options.anomaly_top_k == 3,
                    "report normalized options anomaly_top_k mismatch");

  const std::string params_kv = cuwacunu::piaabo::torch_compat::
      network_analytics_to_latent_lineage_state_text(report, "weights.init.pt");
  const auto report_identity = tsiemene::make_component_report_identity(
      "tsi.wikimyei.representation.encoding.vicreg.0x0001", "bind.train.v1",
      "embedding.network");
  const std::string params_kv_with_identity = cuwacunu::piaabo::torch_compat::
      network_analytics_to_latent_lineage_state_text(report, "weights.init.pt",
                                                     report_identity);
  const std::string extracted_schema =
      cuwacunu::piaabo::torch_compat::extract_analytics_kv_schema(params_kv);
  ok = ok && expect(extracted_schema ==
                        std::string(cuwacunu::piaabo::torch_compat::
                                        kNetworkAnalyticsSchemaCurrent),
                    "kv output missing current schema");
  ok = ok && expect(has_lhs_key(params_kv, "tensor_rms_cv"),
                    "kv output missing tensor_rms key");
  ok = ok && expect(!has_lhs_key(params_kv, "layer_rms_cv"),
                    "kv output should not include removed deprecated key");
  ok = ok && expect(has_lhs_key(params_kv, "network_global_entropic_capacity"),
                    "kv output missing network global entropic capacity");
  ok = ok && expect(has_lhs_key(params_kv, "network_capacity_tensor_count"),
                    "kv output missing capacity tensor count");
  ok = ok && expect(has_lhs_key(params_kv, "top_nonfinite_ratio_count"),
                    "kv output missing top-k count");
  ok = ok && expect(has_lhs_key(params_kv, "top_high_spectral_norm_1_name"),
                    "kv output missing indexed top-k key");
  ok = ok && expect(contains(params_kv, "finite_ratio[0,1]:double"),
                    "finite_ratio should use a closed unit interval domain");
  ok = ok && expect(contains(params_kv, "tensor_count[0,+inf):uint"),
                    "tensor_count should use a nonnegative domain");
  ok = ok && expect(has_lhs_key(params_kv_with_identity, "semantic_taxon") &&
                        contains(params_kv_with_identity, "embedding.network"),
                    "component-aware kv output missing semantic_taxon");
  ok =
      ok &&
      expect(has_lhs_key(params_kv_with_identity, "canonical_path") &&
                 contains(params_kv_with_identity,
                          "canonical_path:str = "
                          "tsi.wikimyei.representation.encoding.vicreg.0x0001"),
             "component-aware kv output missing canonical_path");
  ok = ok && expect(has_lhs_key(params_kv_with_identity, "binding_id") &&
                        contains(params_kv_with_identity,
                                 "binding_id:str = bind.train.v1"),
                    "component-aware kv output missing binding_id");
  const std::string param_schema = extracted_schema;
  ok = ok &&
       expect(param_schema == std::string(cuwacunu::piaabo::torch_compat::
                                              kNetworkAnalyticsSchemaCurrent),
              "schema extractor should recover parameter analytics schema");
  ok =
      ok &&
      expect(
          cuwacunu::piaabo::torch_compat::is_supported_network_analytics_schema(
              param_schema),
          "current parameter schema should be supported");
  ok = ok && expect(!cuwacunu::piaabo::torch_compat::
                        is_supported_network_analytics_schema(
                            "piaabo.torch_compat.network_analytics.v4"),
                    "legacy parameter schema should be unsupported");
  ok = ok && expect(!cuwacunu::piaabo::torch_compat::
                        is_supported_network_analytics_schema(
                            "piaabo.torch_compat.network_analytics.v999"),
                    "unknown parameter schema should be unsupported");

  {
    cuwacunu::piaabo::torch_compat::network_analytics_options_t
        normalized_options{};
    normalized_options.near_zero_epsilon = -1.0;
    normalized_options.log10_abs_histogram_bins = 0;
    normalized_options.spectral_max_elements = 0;
    normalized_options.anomaly_top_k = -5;
    normalized_options.log10_abs_histogram_min = 2.0;
    normalized_options.log10_abs_histogram_max = 1.0;

    const auto normalized_report =
        cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
            model, normalized_options);
    ok = ok &&
         expect(approx(normalized_report.normalized_options.near_zero_epsilon,
                       0.0, 1e-15),
                "negative near_zero_epsilon should normalize to zero");
    ok = ok &&
         expect(normalized_report.normalized_options.log10_abs_histogram_bins ==
                    1,
                "histogram bins should normalize to one");
    ok = ok &&
         expect(normalized_report.normalized_options.spectral_max_elements == 1,
                "spectral_max_elements should normalize to one");
    ok = ok && expect(normalized_report.normalized_options.anomaly_top_k == 0,
                      "anomaly_top_k should normalize to zero");
    const std::string normalized_kv = cuwacunu::piaabo::torch_compat::
        network_analytics_to_latent_lineage_state_text(normalized_report,
                                                       "weights.norm.pt");
    ok = ok && expect(contains(normalized_kv,
                               "log10_abs_histogram_bins[1,+inf):int = 1"),
                      "serialized histogram bins should come from normalized "
                      "report options");
    ok =
        ok &&
        expect(contains(normalized_kv, "spectral_max_elements[1,+inf):int = 1"),
               "serialized spectral_max_elements should use a positive domain");
    ok = ok && expect(contains(normalized_kv, "anomaly_top_k[0,+inf):int = 0"),
                      "serialized anomaly_top_k should allow zero");
  }

  {
    histogram_quantile_network_t histogram_model{};
    cuwacunu::piaabo::torch_compat::network_analytics_options_t
        one_bin_options{};
    one_bin_options.log10_abs_histogram_bins = 1;
    one_bin_options.enable_spectral_metrics = false;
    one_bin_options.include_buffers = false;

    const auto one_bin_report =
        cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
            histogram_model, one_bin_options);
    ok = ok &&
         expect(approx(one_bin_report.log10_abs_histogram_entropy, 0.0, 1e-15),
                "one-bin histogram entropy should be zero");
    ok = ok && expect(one_bin_report.log10_abs_p50 < -1.5,
                      "one-bin p50 should come from exact quantiles, not the "
                      "single bin midpoint");
    ok = ok &&
         expect(one_bin_report.abs_p50 < 0.02,
                "one-bin absolute p50 should track actual tensor magnitudes");
    ok = ok && expect(one_bin_report.abs_p99 > one_bin_report.abs_p50,
                      "one-bin quantiles should remain ordered");
  }

  cuwacunu::camahjucunu::network_design_instruction_t policy_design{};
  policy_design.network_id = "toy.policy";
  policy_design.nodes = {
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "analytics",
          .kind = "NETWORK_ANALYTICS_POLICY",
          .params =
              {
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "near_zero_epsilon",
                      .declared_type = "float",
                      .value = "1e-8",
                      .line = 1,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "log10_abs_histogram_bins",
                      .declared_type = "int",
                      .value = "72",
                      .line = 2,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "log10_abs_histogram_min",
                      .declared_type = "float",
                      .value = "-12.0",
                      .line = 3,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "log10_abs_histogram_max",
                      .declared_type = "float",
                      .value = "6.0",
                      .line = 4,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "include_buffers",
                      .declared_type = "bool",
                      .value = "true",
                      .line = 5,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "enable_spectral_metrics",
                      .declared_type = "bool",
                      .value = "true",
                      .line = 6,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "spectral_max_elements",
                      .declared_type = "int",
                      .value = "1048576",
                      .line = 7,
                  },
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "anomaly_top_k",
                      .declared_type = "int",
                      .value = "5",
                      .line = 8,
                  },
              },
      },
  };
  cuwacunu::piaabo::torch_compat::network_analytics_options_t
      resolved_options{};
  std::string options_error;
  ok = ok && expect(cuwacunu::piaabo::torch_compat::
                        resolve_network_analytics_options_from_network_design(
                            policy_design, &resolved_options, &options_error),
                    "network analytics policy decode should succeed");
  ok = ok && expect(approx(resolved_options.near_zero_epsilon, 1e-8, 1e-15),
                    "near_zero_epsilon decode mismatch");
  ok = ok && expect(resolved_options.log10_abs_histogram_bins == 72,
                    "histogram bins decode mismatch");
  ok = ok && expect(resolved_options.spectral_max_elements == 1048576,
                    "spectral_max_elements decode mismatch");
  ok = ok && expect(resolved_options.anomaly_top_k == 5,
                    "anomaly_top_k decode mismatch");

  cuwacunu::camahjucunu::network_design_instruction_t missing_policy_design{};
  missing_policy_design.network_id = "missing.policy";
  options_error.clear();
  ok = ok &&
       expect(!cuwacunu::piaabo::torch_compat::
                  resolve_network_analytics_options_from_network_design(
                      missing_policy_design, &resolved_options, &options_error),
              "missing policy node should fail strict decode");
  ok = ok && expect(contains(options_error, "NETWORK_ANALYTICS_POLICY"),
                    "missing policy error should mention policy node");

  cuwacunu::camahjucunu::network_design_instruction_t design{};
  design.network_id = "toy.network";
  design.assembly_tag = "concat";
  design.nodes = {
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "A",
          .kind = "Input",
          .params = {},
      },
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "B",
          .kind = "Block",
          .params =
              {
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "from",
                      .declared_type = "str",
                      .value = "A C",
                      .line = 1,
                  },
              },
      },
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "C",
          .kind = "Block",
          .params =
              {
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "from",
                      .declared_type = "str",
                      .value = "B",
                      .line = 2,
                  },
              },
      },
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "D",
          .kind = "Head",
          .params =
              {
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "from",
                      .declared_type = "str",
                      .value = "A B C",
                      .line = 3,
                  },
              },
      },
      cuwacunu::camahjucunu::network_design_node_t{
          .id = "E",
          .kind = "Dead",
          .params =
              {
                  cuwacunu::camahjucunu::network_design_param_t{
                      .key = "probe",
                      .declared_type = "str",
                      .value = "E unresolved_token",
                      .line = 4,
                  },
              },
      },
  };
  design.exports = {
      cuwacunu::camahjucunu::network_design_export_t{
          .name = "repr",
          .node_id = "D",
      },
  };

  const auto design_report =
      cuwacunu::piaabo::torch_compat::summarize_network_design_analytics(
          design);
  ok = ok && expect(design_report.schema ==
                        std::string(cuwacunu::piaabo::torch_compat::
                                        kNetworkDesignAnalyticsSchemaCurrent),
                    "network design schema should be current");
  ok = ok && expect(design_report.node_count == 5, "node count mismatch");
  ok = ok &&
       expect(design_report.analysis_valid, "design report should be valid");
  ok = ok && expect(design_report.internal_edge_count == 7,
                    "internal edge count mismatch");
  ok = ok && expect(design_report.inferred_edge_count == 7,
                    "inferred edge count mismatch");
  ok = ok && expect(design_report.explicit_edge_count == 0,
                    "explicit edge count should be zero");
  ok = ok && expect(design_report.source_count == 1, "source count mismatch");
  ok = ok && expect(design_report.internal_sink_count == 1,
                    "internal sink count mismatch");
  ok = ok && expect(design_report.export_reachable_node_count == 4,
                    "export reachable node count mismatch");
  ok = ok && expect(design_report.dead_end_node_count == 1,
                    "dead-end count mismatch");
  ok = ok &&
       expect(design_report.orphan_node_count == 1, "orphan count mismatch");
  ok = ok && expect(design_report.branch_node_count == 3,
                    "branch node count mismatch");
  ok = ok &&
       expect(design_report.merge_node_count == 2, "merge node count mismatch");
  ok = ok && expect(design_report.scc_count == 4, "SCC count mismatch");
  ok = ok &&
       expect(design_report.largest_scc_size == 2, "largest SCC size mismatch");
  ok = ok && expect(design_report.cyclic_node_count == 3,
                    "cyclic node count mismatch");
  ok = ok && expect(approx(design_report.cyclic_node_ratio, 3.0 / 5.0, 1e-12),
                    "cyclic node ratio mismatch");
  ok = ok &&
       expect(design_report.skip_edge_count == 1, "skip edge count mismatch");
  ok = ok && expect(design_report.max_skip_span == 2, "max skip span mismatch");
  ok = ok && expect(design_report.longest_source_to_export_path_nodes == 4,
                    "longest source-to-export path mismatch");
  ok = ok && expect(design_report.median_source_to_export_path_nodes == 4,
                    "median source-to-export path mismatch");
  ok =
      ok &&
      expect(approx(design_report.topological_order_ratio,
                    static_cast<double>(design_report.topological_order_count) /
                        5.0,
                    1e-12),
             "topological order ratio mismatch");
  ok = ok && expect(design_report.unresolved_identifier_token_count == 1,
                    "unresolved identifier token count mismatch");
  ok = ok && expect(design_report.self_reference_token_count == 1,
                    "self reference token count mismatch");
  ok = ok && expect(design_report.topological_order_count == 1,
                    "self-loop should reduce the topological order count");

  const std::string design_kv = cuwacunu::piaabo::torch_compat::
      network_design_analytics_to_latent_lineage_state_text(design_report,
                                                            "toy-source");
  const std::string design_schema =
      cuwacunu::piaabo::torch_compat::extract_analytics_kv_schema(design_kv);
  ok = ok && expect(design_schema ==
                        std::string(cuwacunu::piaabo::torch_compat::
                                        kNetworkDesignAnalyticsSchemaCurrent),
                    "design kv missing current schema");
  ok =
      ok &&
      expect(!has_lhs_key(design_kv, "topological_order_coverage"),
             "design kv should not include removed deprecated topological key");
  ok = ok && expect(has_lhs_key(design_kv, "topological_order_ratio"),
                    "design kv missing ratio key");
  ok = ok && expect(has_lhs_key(design_kv, "skip_edge_count"),
                    "design kv missing skip key");
  ok = ok && expect(has_lhs_key(design_kv, "unresolved_identifier_token_count"),
                    "design kv missing token evidence key");
  ok = ok && expect(has_lhs_key(design_kv, "analysis_valid"),
                    "design kv should include the validity state");
  ok = ok && expect(has_lhs_key(design_kv, "assembly_tag"),
                    "design kv should include assembly_tag");
  ok = ok && expect(!has_lhs_key(design_kv, "join_policy"),
                    "design kv should not include legacy join_policy key");
  ok = ok && expect(design_schema ==
                        std::string(cuwacunu::piaabo::torch_compat::
                                        kNetworkDesignAnalyticsSchemaCurrent),
                    "schema extractor should recover design analytics schema");
  ok = ok &&
       expect(cuwacunu::piaabo::torch_compat::
                  is_supported_network_design_analytics_schema(design_schema),
              "current design schema should be supported");
  ok = ok && expect(!cuwacunu::piaabo::torch_compat::
                        is_supported_network_design_analytics_schema(
                            "piaabo.torch_compat.network_design_analytics.v4"),
                    "legacy design schema should be unsupported");
  ok =
      ok && expect(!cuwacunu::piaabo::torch_compat::
                       is_supported_network_design_analytics_schema(
                           "piaabo.torch_compat.network_design_analytics.v999"),
                   "unknown design schema should be unsupported");

  {
    cuwacunu::camahjucunu::network_design_instruction_t cyclic_entry_design{};
    cyclic_entry_design.network_id = "cyclic.entry";
    cyclic_entry_design.assembly_tag = "sum";
    cyclic_entry_design.nodes = {
        cuwacunu::camahjucunu::network_design_node_t{
            .id = "A",
            .kind = "Block",
            .params = {cuwacunu::camahjucunu::network_design_param_t{
                .key = "from",
                .declared_type = "str",
                .value = "B",
                .line = 1,
            }},
        },
        cuwacunu::camahjucunu::network_design_node_t{
            .id = "B",
            .kind = "Head",
            .params = {cuwacunu::camahjucunu::network_design_param_t{
                .key = "from",
                .declared_type = "str",
                .value = "A",
                .line = 2,
            }},
        },
    };
    cyclic_entry_design.exports = {
        cuwacunu::camahjucunu::network_design_export_t{
            .name = "repr",
            .node_id = "B",
        },
    };

    const auto cyclic_entry_report =
        cuwacunu::piaabo::torch_compat::summarize_network_design_analytics(
            cyclic_entry_design);
    ok = ok &&
         expect(
             cyclic_entry_report.source_count == 0,
             "cyclic entry SCC should keep node-level source_count semantics");
    ok = ok &&
         expect(cyclic_entry_report.longest_source_to_export_path_nodes == 2,
                "cyclic entry SCC should still seed source-to-export paths");
    ok = ok &&
         expect(cyclic_entry_report.median_source_to_export_path_nodes == 2,
                "cyclic entry SCC median path should use the source SCC");
  }

  {
    cuwacunu::camahjucunu::network_design_instruction_t duplicate_design{};
    duplicate_design.network_id = "duplicate.ids";
    duplicate_design.nodes = {
        cuwacunu::camahjucunu::network_design_node_t{
            .id = "dup",
            .kind = "Input",
            .params = {},
        },
        cuwacunu::camahjucunu::network_design_node_t{
            .id = "dup",
            .kind = "Head",
            .params = {},
        },
    };

    const auto duplicate_report =
        cuwacunu::piaabo::torch_compat::summarize_network_design_analytics(
            duplicate_design);
    ok = ok && expect(!duplicate_report.analysis_valid,
                      "duplicate node ids should invalidate the report");
    ok = ok && expect(duplicate_report.analysis_error == "duplicate_node_id",
                      "duplicate node id error should be explicit");
    ok = ok && expect(duplicate_report.duplicate_node_id_count == 1,
                      "duplicate node id count mismatch");
    ok = ok && expect(duplicate_report.duplicate_node_id_example == "dup",
                      "duplicate node id example mismatch");
    ok = ok && expect(duplicate_report.internal_edge_count == 0 &&
                          duplicate_report.skip_edge_count == 0,
                      "invalid duplicate-id reports should leave graph metrics "
                      "uncomputed");
  }

  {
    const auto tmp_dir = std::filesystem::temp_directory_path() /
                         "cuwacunu_network_analytics_atomic";
    std::error_code ec;
    std::filesystem::remove_all(tmp_dir, ec);
    std::filesystem::create_directories(tmp_dir, ec);
    const auto failing_target = tmp_dir / "network_atomic_target";
    std::filesystem::create_directories(failing_target, ec);
    std::string write_error;
    ok = ok &&
         expect(
             !cuwacunu::piaabo::torch_compat::write_network_analytics_file(
                 model, failing_target, options, &write_error),
             "atomic network analytics writer should fail on directory target");
    ok = ok &&
         expect(std::filesystem::is_directory(failing_target),
                "atomic write failure should preserve the existing target");
    bool left_temp_file = false;
    for (const auto &entry : std::filesystem::directory_iterator(tmp_dir)) {
      const auto name = entry.path().filename().string();
      if (name.find("network_atomic_target.tmp.") == 0) {
        left_temp_file = true;
        break;
      }
    }
    ok = ok && expect(!left_temp_file,
                      "atomic write failure should clean up temporary files");
  }

  if (!ok)
    return 1;
  std::cout << "[test_network_analytics_metrics] All checks passed.\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << "[test_network_analytics_metrics] exception: " << e.what()
            << "\n";
  return 1;
}
