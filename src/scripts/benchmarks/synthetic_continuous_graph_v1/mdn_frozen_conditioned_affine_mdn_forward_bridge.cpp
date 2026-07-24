#define main mdn_frozen_forward_bridge_embedded_objective_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>
#include <unordered_map>

#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace mdn_forward_bridge {

namespace production_mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace production_launcher = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

constexpr std::string_view kSchema =
    "synthetic_mdn_frozen_conditioned_affine_mdn_forward_bridge_v1";
constexpr std::string_view kRepresentationProbeSha =
    "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed";
constexpr std::string_view kCachedProbeSha =
    "ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851";
constexpr std::string_view kMdnCheckpointSha =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr std::string_view kV2ArtifactSha =
    "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739";
constexpr std::string_view kShadowReportSha =
    "c62aad32d77e0bef3ebb1b2a749624925149e2aeea46f2f1709dbbbf1615a839";

constexpr int64_t kDevelopmentEnd = 730;
constexpr int64_t kFitEnd = 554;
constexpr int64_t kPurgeEnd = 584;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kContextDim = 32;
constexpr int64_t kReadoutWidth = 400;
constexpr double kParityTolerance = 1.0e-6;
constexpr double kCachedMetricTolerance = 1.0e-12;
constexpr double kLiveMetricTolerance = 1.0e-6;
constexpr double kCachedObjectiveTolerance = 1.0e-5;
constexpr double kLiveObjectiveTolerance = 1.0e-3;

struct Options {
  std::filesystem::path representation_probe;
  std::filesystem::path cached_probe;
  std::filesystem::path mdn_checkpoint;
  std::filesystem::path v2_artifact;
  std::filesystem::path shadow_report;
  std::filesystem::path output;
  std::string schema_id;
};

struct RepresentationSurface {
  torch::Tensor context;
  torch::Tensor target;
  int64_t row_count{0};
  double difference_max_abs_delta{0.0};
  double quote_max_abs_delta{0.0};
};

using KvMap = std::map<std::string, std::string>;

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

std::filesystem::path normalized(const std::filesystem::path &path) {
  std::error_code error;
  auto result = std::filesystem::absolute(path, error);
  require(!error, "failed to normalize a bridge path");
  return result.lexically_normal();
}

Options parse_options(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *name) -> std::string {
      require(index + 1 < argc, std::string("missing value for ") + name);
      return argv[++index];
    };
    if (argument == "--representation-probe") {
      options.representation_probe = value("--representation-probe");
    } else if (argument == "--cached-probe") {
      options.cached_probe = value("--cached-probe");
    } else if (argument == "--mdn-checkpoint") {
      options.mdn_checkpoint = value("--mdn-checkpoint");
    } else if (argument == "--v2-artifact") {
      options.v2_artifact = value("--v2-artifact");
    } else if (argument == "--shadow-report") {
      options.shadow_report = value("--shadow-report");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else if (argument == "--history-input" ||
               argument == "--historical-input" ||
               argument == "--evaluation-input" ||
               argument == "--holdout-input" || argument == "--policy-input" ||
               argument == "--training-output" ||
               argument == "--checkpoint-output") {
      fail("forbidden forward-bridge argument: " + argument);
    } else {
      fail("unknown forward-bridge argument: " + argument);
    }
  }
  require(!options.representation_probe.empty() &&
              !options.cached_probe.empty() &&
              !options.mdn_checkpoint.empty() && !options.v2_artifact.empty() &&
              !options.shadow_report.empty() && !options.output.empty() &&
              !options.schema_id.empty(),
          "all forward-bridge inputs, output, and schema-id are required");
  const std::set<std::filesystem::path> identities{
      normalized(options.representation_probe),
      normalized(options.cached_probe),
      normalized(options.mdn_checkpoint),
      normalized(options.v2_artifact),
      normalized(options.shadow_report),
      normalized(options.output)};
  require(identities.size() == 6,
          "all forward-bridge paths must have distinct identities");
  return options;
}

KvMap read_kv(const std::filesystem::path &path, const char *surface) {
  std::ifstream input(path);
  require(static_cast<bool>(input), std::string("failed to open ") + surface);
  KvMap values;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const auto separator = line.find('=');
    require(separator != std::string::npos && separator > 0,
            std::string(surface) + " contains a malformed line");
    require(
        values.emplace(line.substr(0, separator), line.substr(separator + 1))
            .second,
        std::string(surface) + " contains a duplicate key");
  }
  require(input.good() || input.eof(),
          std::string("failed while reading ") + surface);
  return values;
}

const std::string &kv_required(const KvMap &values, const std::string &key,
                               const char *surface) {
  const auto found = values.find(key);
  require(found != values.end() && !found->second.empty(),
          std::string(surface) + " is missing " + key);
  return found->second;
}

RepresentationSurface
read_representation_probe(const std::filesystem::path &path) {
  constexpr std::string_view kHeader =
      "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
      "edge_id,base_node_id,quote_node_id,channel_index,"
      "target_edge_close_return,feature_count,feature_values";
  constexpr std::string_view kRecord =
      "kikijyeba.synthetic.representation_edge_feature_probe.v1";
  constexpr int64_t kFeatureCount = 3 * kContextDim;
  const std::array<std::string, kEdgeCount> edge_ids{
      "SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  const std::array<std::string, kEdgeCount> base_ids{"SYNALPHA", "SYNBETA",
                                                     "SYNGAMMA"};

  std::ifstream input(path);
  require(static_cast<bool>(input), "failed to open representation probe");
  std::string line;
  require(std::getline(input, line) && line == kHeader,
          "representation probe header mismatch");

  RepresentationSurface surface;
  surface.context =
      torch::full({kDevelopmentEnd, kNodeCount, kChannelCount, kContextDim},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  surface.target =
      torch::full({kDevelopmentEnd, kEdgeCount, kChannelCount},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  auto context = surface.context.accessor<float, 4>();
  auto target = surface.target.accessor<float, 3>();
  std::vector<bool> seen(
      static_cast<std::size_t>(kDevelopmentEnd * kEdgeCount * kChannelCount),
      false);

  while (std::getline(input, line)) {
    require(!line.empty(), "representation probe contains an empty row");
    const auto columns = split_exact(line, ',');
    require(columns.size() == 12,
            "representation probe row must contain 12 columns");
    require(columns[0] == kRecord,
            "representation probe record schema mismatch");
    (void)parse_i64_strict(columns[1], "anchor_key");
    const auto anchor = parse_i64_strict(columns[2], "anchor_index");
    const auto local_anchor =
        parse_i64_strict(columns[3], "anchor_local_index");
    const auto edge = parse_i64_strict(columns[4], "edge_index");
    const auto channel = parse_i64_strict(columns[8], "channel_index");
    require(anchor >= 0 && anchor < kDevelopmentEnd && edge >= 0 &&
                edge < kEdgeCount && channel >= 0 && channel < kChannelCount &&
                local_anchor == anchor % 64,
            "representation probe coordinate is outside [0,730)");
    require(columns[5] == edge_ids[static_cast<std::size_t>(edge)] &&
                columns[6] == base_ids[static_cast<std::size_t>(edge)] &&
                columns[7] == "SYNUSD",
            "representation probe graph identity mismatch");
    require(parse_i64_strict(columns[10], "feature_count") == kFeatureCount,
            "representation feature count is not 96");
    const auto ordinal = (anchor * kEdgeCount + edge) * kChannelCount + channel;
    require(ordinal == surface.row_count,
            "representation rows are not in canonical order");
    require(!seen[static_cast<std::size_t>(ordinal)],
            "duplicate representation coordinate");
    seen[static_cast<std::size_t>(ordinal)] = true;

    const auto features = split_exact(columns[11], ';');
    require(static_cast<int64_t>(features.size()) == kFeatureCount,
            "representation feature vector width mismatch");
    target[anchor][edge][channel] =
        static_cast<float>(parse_f64_strict(columns[9], "target"));
    for (int64_t feature = 0; feature < kContextDim; ++feature) {
      const auto base = static_cast<float>(parse_f64_strict(
          features[static_cast<std::size_t>(feature)], "base feature"));
      const auto quote = static_cast<float>(parse_f64_strict(
          features[static_cast<std::size_t>(kContextDim + feature)],
          "quote feature"));
      const auto difference = static_cast<float>(parse_f64_strict(
          features[static_cast<std::size_t>(2 * kContextDim + feature)],
          "difference feature"));
      surface.difference_max_abs_delta =
          std::max(surface.difference_max_abs_delta,
                   std::fabs(static_cast<double>(base - quote - difference)));
      if (edge == 0) {
        context[anchor][0][channel][feature] = quote;
      } else {
        surface.quote_max_abs_delta =
            std::max(surface.quote_max_abs_delta,
                     std::fabs(static_cast<double>(
                         quote - context[anchor][0][channel][feature])));
      }
      context[anchor][edge + 1][channel][feature] = base;
    }
    ++surface.row_count;
  }
  require(input.good() || input.eof(),
          "failed while reading representation probe");
  require(surface.row_count == kDevelopmentEnd * kEdgeCount * kChannelCount &&
              std::find(seen.begin(), seen.end(), false) == seen.end() &&
              torch::isfinite(surface.context).all().item<bool>() &&
              torch::isfinite(surface.target).all().item<bool>(),
          "representation probe is incomplete or non-finite");
  require(surface.difference_max_abs_delta <= kParityTolerance &&
              surface.quote_max_abs_delta == 0.0,
          "representation base/quote/difference contract mismatch");
  return surface;
}

production_mdn::ChannelContextMdn
make_production_model(const torch::Device &device) {
  production_mdn::DirectEdgeReturnHeadOptions direct;
  direct.feature_dim = 128;
  direct.quote_node_index = 0;
  direct.identity_mode = "edge_embedding_per_edge";
  direct.base_edge_count = kEdgeCount;
  direct.identity_embedding_dim = 16;
  direct.adapter_hidden_dim = 128;
  return production_mdn::ChannelContextMdn(
      kContextDim, 9, kChannelCount, 1, 3, 128, 2, torch::kFloat32, device, 8,
      16, std::vector<int64_t>{0, 1, 2, 3, 4, 5, 6, 7, 8}, 1.0e-3, direct);
}

void load_production_checkpoint(const std::filesystem::path &path,
                                production_mdn::ChannelContextMdn &model) {
  production_launcher::channel_mdn_checkpoint_identity_t identity{};
  identity.component_assembly_id = "mdn_v1";
  identity.input_representation_assembly_id = "mtf_jepa_mae_vicreg_v1";
  identity.context_contract = "graph_order.channel_node_representation.v1";
  identity.output_contract = "graph_order.channel_node_future_distribution.v1";
  identity.context_mode = "channel_context_strict";
  identity.target_domain = "channel_node_future";
  identity.target_mask_policy = "per_target_feature_valid";
  identity.activity_target = "node_feature_support_mean";
  identity.graph_order_fingerprint = "d334e38b1887ae16";
  identity.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  identity.target_coords = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  identity.channel_count = 3;
  identity.context_dim = 32;
  identity.future_horizon = 1;
  identity.mixture_count = 3;
  identity.hidden_width = 128;
  identity.residual_depth = 2;
  identity.feature_embedding_dim = 8;
  identity.channel_adapter_rank = 16;
  identity.direct_edge_return_readout_identity_mode = "edge_embedding_per_edge";
  identity.direct_edge_return_readout_base_edge_count = 3;
  identity.direct_edge_return_readout_identity_embedding_dim = 16;
  identity.direct_edge_return_readout_adapter_hidden_dim = 128;
  identity.sigma_min = 1.0e-3;
  identity.sigma_max = 0.0;
  identity.eps = 1.0e-6;
  production_launcher::load_channel_mdn_checkpoint_file(path, model, nullptr,
                                                        &identity, nullptr);
  model->eval();
  require(!model->is_training(),
          "loaded production MDN remained in train mode");
}

struct Comparison {
  bool torch_equal{false};
  double maximum_abs_delta{std::numeric_limits<double>::infinity()};
  double mean_abs_delta{std::numeric_limits<double>::infinity()};
  bool pass{false};
};

Comparison compare(const torch::Tensor &lhs_input,
                   const torch::Tensor &rhs_input) {
  const auto lhs =
      lhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto rhs =
      rhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  require(lhs.sizes() == rhs.sizes() && lhs.numel() > 0 &&
              torch::isfinite(lhs).all().item<bool>() &&
              torch::isfinite(rhs).all().item<bool>(),
          "invalid bridge tensor comparison");
  const auto delta = (lhs - rhs).abs();
  Comparison result;
  result.torch_equal = torch::equal(lhs, rhs);
  result.maximum_abs_delta = delta.max().item<double>();
  result.mean_abs_delta = delta.mean().item<double>();
  result.pass = result.maximum_abs_delta <= kParityTolerance;
  return result;
}

void validate_v2_metadata(
    const production_mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata
        &metadata) {
  require(metadata.schema_version == 2 && metadata.run_only &&
              !metadata.policy_eligible && metadata.feature_dim == 384 &&
              metadata.edge_count == kEdgeCount &&
              metadata.channel_count == kChannelCount &&
              metadata.fit_begin == 0 && metadata.fit_end == kFitEnd &&
              metadata.purge_begin == kFitEnd &&
              metadata.purge_end == kPurgeEnd &&
              metadata.validation_begin == kPurgeEnd &&
              metadata.validation_end == kDevelopmentEnd &&
              metadata.max_anchor == kDevelopmentEnd - 1 &&
              metadata.graph_order_fingerprint == "d334e38b1887ae16" &&
              metadata.source_probe_sha256 == kCachedProbeSha &&
              metadata.mdn_checkpoint_sha256 == kMdnCheckpointSha &&
              metadata.semantic_tensor_digest ==
                  "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af"
                  "0c38" &&
              metadata.edge_ids ==
                  std::vector<std::string>(
                      {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"}) &&
              metadata.node_ids ==
                  std::vector<std::string>(
                      {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"}),
          "conditioned-affine v2 provenance identity mismatch");
}

double metric_field(const Metric &metric, std::string_view field) {
  if (field == "mae") {
    return metric.abs_error / static_cast<double>(metric.count);
  }
  if (field == "rmse") {
    return metric_rmse(metric);
  }
  if (field == "directional_accuracy") {
    return metric_direction(metric);
  }
  if (field == "pairwise_rank_accuracy") {
    return metric_rank(metric);
  }
  if (field == "correlation") {
    return metric_correlation(metric);
  }
  if (field == "prediction_population_std") {
    const auto count = static_cast<double>(metric.count);
    const auto variance = metric.prediction_sq_sum / count -
                          std::pow(metric.prediction_sum / count, 2.0);
    return std::sqrt(std::max(variance, 0.0));
  }
  fail("unknown metric field");
}

double validate_metric_against_report(const Metric &metric, const KvMap &report,
                                      const std::string &prefix,
                                      double tolerance) {
  require(std::isfinite(tolerance) && tolerance >= 0.0,
          "invalid shadow metric parity tolerance");
  require(
      parse_i64_strict(kv_required(report, prefix + ".count", "shadow report"),
                       "shadow count") == metric.count &&
          parse_i64_strict(
              kv_required(report, prefix + ".pair_count", "shadow report"),
              "shadow pair count") == metric.pair_count,
      "shadow metric count mismatch");
  constexpr std::array<std::string_view, 6> fields{"mae",
                                                   "rmse",
                                                   "directional_accuracy",
                                                   "pairwise_rank_accuracy",
                                                   "correlation",
                                                   "prediction_population_std"};
  double maximum = 0.0;
  for (const auto field : fields) {
    const auto expected = parse_f64_strict(
        kv_required(report, prefix + "." + std::string(field), "shadow report"),
        "shadow metric");
    maximum =
        std::max(maximum, std::fabs(metric_field(metric, field) - expected));
  }
  require(maximum <= tolerance,
          "metric does not reproduce the cached shadow report");
  return maximum;
}

struct DirectObjectiveSummary {
  double total{0.0};
  double regression{0.0};
  double direction{0.0};
  double rank{0.0};
  int64_t valid_count{0};
  int64_t pairwise_valid_count{0};
};

DirectObjectiveSummary
compute_production_direct_objective(const torch::Tensor &prediction,
                                    const torch::Tensor &target) {
  require(prediction.sizes() == target.sizes() && prediction.dim() == 3,
          "direct-objective prediction/target shape mismatch");
  production_mdn::channel_context_mdn_train_options_t options;
  options.direct_edge_return_readout_enabled = true;
  options.direct_edge_return_readout_loss_weight = 100.0;
  options.direct_edge_return_readout_direction_weight = 5.0;
  options.direct_edge_return_readout_rank_weight = 5.0;
  options.direct_edge_return_readout_huber_beta = 0.5;
  options.direct_edge_return_readout_logit_scale = 1.0;
  options.direct_edge_return_readout_target_scale = 36.0;

  const auto target_on_device = target.to(prediction.device());
  const auto quote = torch::zeros({target.size(0), 1, target.size(2)},
                                  target_on_device.options());
  const auto future =
      torch::cat({quote, target_on_device}, 1).unsqueeze(3).contiguous();
  const auto mask = torch::ones_like(future, torch::kBool);
  production_mdn::MdnOut output{};
  output.direct_edge_return = prediction;
  const auto direct = production_mdn::channel_context_mdn_train_detail::
      compute_direct_edge_return_readout_loss(output, future, mask, options,
                                              std::vector<int64_t>{3});
  return {.total = direct.loss.item<double>(),
          .regression = direct.regression_loss.item<double>(),
          .direction = direct.direction_loss.item<double>(),
          .rank = direct.rank_loss.item<double>(),
          .valid_count = direct.valid_count,
          .pairwise_valid_count = direct.pairwise_valid_count};
}

double validate_objective_against_report(const DirectObjectiveSummary &summary,
                                         const KvMap &report,
                                         const std::string &prefix,
                                         double tolerance) {
  const auto expected_count =
      parse_i64_strict(kv_required(report, prefix + ".count", "shadow report"),
                       "shadow objective count");
  const auto expected_pair_count = parse_i64_strict(
      kv_required(report, prefix + ".pair_count", "shadow report"),
      "shadow objective pair count");
  require(summary.valid_count == expected_count &&
              summary.pairwise_valid_count == expected_pair_count,
          "production direct-objective domain differs from shadow report");
  const auto expected_total = parse_f64_strict(
      kv_required(report, prefix + ".production_objective", "shadow report"),
      "shadow production objective");
  const auto delta = std::fabs(summary.total - expected_total);
  require(delta <= tolerance,
          "production direct objective differs from shadow report");
  return delta;
}

void emit_direct_objective(std::ostream &output, const std::string &prefix,
                           const DirectObjectiveSummary &summary) {
  output << prefix << ".total=" << summary.total << '\n';
  output << prefix << ".regression=" << summary.regression << '\n';
  output << prefix << ".direction=" << summary.direction << '\n';
  output << prefix << ".rank=" << summary.rank << '\n';
  output << prefix << ".valid_count=" << summary.valid_count << '\n';
  output << prefix << ".pairwise_valid_count=" << summary.pairwise_valid_count
         << '\n';
}

void emit_comparison(std::ostream &output, const std::string &prefix,
                     const Comparison &comparison) {
  output << prefix << ".torch_equal=" << bool_text(comparison.torch_equal)
         << '\n';
  output << prefix << ".maximum_abs_delta=" << comparison.maximum_abs_delta
         << '\n';
  output << prefix << ".mean_abs_delta=" << comparison.mean_abs_delta << '\n';
  output << prefix << ".threshold=" << kParityTolerance << '\n';
  output << prefix << ".pass=" << bool_text(comparison.pass) << '\n';
}

torch::Tensor
recompute_readout_in_capture_chunks(production_mdn::ChannelContextMdn &model,
                                    const torch::Tensor &context_cpu) {
  require(context_cpu.sizes() ==
              torch::IntArrayRef(
                  {kDevelopmentEnd, kNodeCount, kChannelCount, kContextDim}),
          "representation context shape mismatch");
  std::vector<torch::Tensor> chunks;
  for (int64_t begin = 0; begin < kDevelopmentEnd; begin += 64) {
    const auto count = std::min<int64_t>(64, kDevelopmentEnd - begin);
    const auto context = context_cpu.narrow(0, begin, count).to(torch::kCUDA);
    const auto mask = torch::ones(
        {count, kNodeCount, kChannelCount},
        torch::TensorOptions().dtype(torch::kBool).device(torch::kCUDA));
    chunks.push_back(model->direct_edge_context_features(context, mask));
  }
  auto readout = torch::cat(chunks, 0).contiguous();
  require(readout.sizes() ==
                  torch::IntArrayRef({kDevelopmentEnd, kEdgeCount,
                                      kChannelCount, kReadoutWidth}) &&
              torch::isfinite(readout).all().item<bool>(),
          "recomputed production readout surface is invalid");
  return readout;
}

void emit_bridge_metric(std::ostream &output, const std::string &prefix,
                        const Metric &metric) {
  output << prefix << ".count=" << metric.count << '\n';
  output << prefix << ".pair_count=" << metric.pair_count << '\n';
  for (const auto field : {std::string_view("mae"), std::string_view("rmse"),
                           std::string_view("directional_accuracy"),
                           std::string_view("pairwise_rank_accuracy"),
                           std::string_view("correlation"),
                           std::string_view("prediction_population_std")}) {
    output << prefix << '.' << field << '=' << metric_field(metric, field)
           << '\n';
  }
}

int run(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_options(argc, argv);
    require(options.schema_id == kSchema, "forward-bridge schema mismatch");
    require(!std::filesystem::exists(options.output) &&
                !std::filesystem::exists(options.output.string() + ".tmp"),
            "refusing to replace a forward-bridge output");

    const std::array<std::pair<std::filesystem::path, std::string_view>, 5>
        pinned_inputs{{
            {options.representation_probe, kRepresentationProbeSha},
            {options.cached_probe, kCachedProbeSha},
            {options.mdn_checkpoint, kMdnCheckpointSha},
            {options.v2_artifact, kV2ArtifactSha},
            {options.shadow_report, kShadowReportSha},
        }};
    for (const auto &[path, expected] : pinned_inputs) {
      require(std::filesystem::is_regular_file(path) &&
                  sha256_file(path) == expected,
              "pinned forward-bridge input identity mismatch");
    }

    const auto shadow_report = read_kv(options.shadow_report, "shadow report");
    require(
        kv_required(shadow_report, "schema_id", "shadow report") ==
                "synthetic_mdn_frozen_conditioned_affine_shadow_readout_v1" &&
            kv_required(shadow_report, "status", "shadow report") ==
                "complete" &&
            kv_required(shadow_report, "boundary.development",
                        "shadow report") == "[0,730)" &&
            kv_required(shadow_report, "boundary.fit", "shadow report") ==
                "[0,554)" &&
            kv_required(shadow_report, "boundary.purge", "shadow report") ==
                "[554,584)" &&
            kv_required(shadow_report, "boundary.validation",
                        "shadow report") == "[584,730)" &&
            kv_required(shadow_report, "boundary.unconsumed_holdout_opened",
                        "shadow report") == "false",
        "shadow report boundary/provenance contract mismatch");

    const auto representation =
        read_representation_probe(options.representation_probe);
    const auto cached = read_probe(options.cached_probe, 0, kDevelopmentEnd);
    require(cached.record_schema ==
                    "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1" &&
                cached.row_count ==
                    kDevelopmentEnd * kEdgeCount * kChannelCount &&
                cached.readout_features.sizes() ==
                    torch::IntArrayRef({kDevelopmentEnd, kEdgeCount,
                                        kChannelCount, kReadoutWidth}) &&
                cached.target.sizes() == representation.target.sizes() &&
                torch::equal(cached.target, representation.target),
            "representation/cached target or surface identity mismatch");

    const auto runtime = configure_deterministic_runtime();
    reset_deterministic_seed();
    auto model = make_production_model(torch::Device(torch::kCUDA));
    load_production_checkpoint(options.mdn_checkpoint, model);

    torch::NoGradGuard no_grad;
    const auto recomputed =
        recompute_readout_in_capture_chunks(model, representation.context);
    const auto cached_cuda = cached.readout_features.to(torch::kCUDA);
    const auto feature_parity = compare(recomputed, cached_cuda);
    require(feature_parity.pass,
            "production MDN forward does not reproduce cached readout within "
            "the predeclared 1e-6 tolerance");

    auto v2 = production_mdn::load_per_edge_conditioned_affine_return_head(
        options.v2_artifact);
    validate_v2_metadata(v2.metadata);
    v2.head->to(torch::kCUDA);
    v2.head->eval();
    v2.head->validate_registered_state();
    const auto cached_prediction =
        v2.head->forward_readout_features(cached_cuda);
    const auto recomputed_prediction =
        v2.head->forward_readout_features(recomputed);
    const auto prediction_parity =
        compare(recomputed_prediction, cached_prediction);
    require(prediction_parity.pass,
            "conditioned-affine predictions drift across the MDN forward "
            "bridge");

    const auto cached_fit_metric =
        evaluate_prediction(cached_prediction.narrow(0, 0, kFitEnd),
                            cached.target.narrow(0, 0, kFitEnd));
    const auto cached_validation_metric = evaluate_prediction(
        cached_prediction.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        cached.target.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto recomputed_fit_metric =
        evaluate_prediction(recomputed_prediction.narrow(0, 0, kFitEnd),
                            representation.target.narrow(0, 0, kFitEnd));
    const auto recomputed_validation_metric = evaluate_prediction(
        recomputed_prediction.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        representation.target.narrow(0, kPurgeEnd,
                                     kDevelopmentEnd - kPurgeEnd));
    require_metric_counts(cached_fit_metric, 4986, 4986, "cached v2 fit");
    require_metric_counts(cached_validation_metric, 1314, 1314,
                          "cached v2 validation");
    require_metric_counts(recomputed_fit_metric, 4986, 4986,
                          "recomputed v2 fit");
    require_metric_counts(recomputed_validation_metric, 1314, 1314,
                          "recomputed v2 validation");
    const auto fit_report_max_delta = validate_metric_against_report(
        cached_fit_metric, shadow_report, "conditioned_v2.fit",
        kCachedMetricTolerance);
    const auto validation_report_max_delta = validate_metric_against_report(
        cached_validation_metric, shadow_report, "conditioned_v2.validation",
        kCachedMetricTolerance);
    const auto recomputed_fit_report_max_delta = validate_metric_against_report(
        recomputed_fit_metric, shadow_report, "conditioned_v2.fit",
        kLiveMetricTolerance);
    const auto recomputed_validation_report_max_delta =
        validate_metric_against_report(
            recomputed_validation_metric, shadow_report,
            "conditioned_v2.validation", kLiveMetricTolerance);

    const auto cached_fit_objective = compute_production_direct_objective(
        cached_prediction.narrow(0, 0, kFitEnd),
        cached.target.narrow(0, 0, kFitEnd));
    const auto cached_validation_objective =
        compute_production_direct_objective(
            cached_prediction.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
            cached.target.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto recomputed_fit_objective = compute_production_direct_objective(
        recomputed_prediction.narrow(0, 0, kFitEnd),
        representation.target.narrow(0, 0, kFitEnd));
    const auto recomputed_validation_objective =
        compute_production_direct_objective(
            recomputed_prediction.narrow(0, kPurgeEnd,
                                         kDevelopmentEnd - kPurgeEnd),
            representation.target.narrow(0, kPurgeEnd,
                                         kDevelopmentEnd - kPurgeEnd));
    const auto cached_fit_objective_report_delta =
        validate_objective_against_report(cached_fit_objective, shadow_report,
                                          "conditioned_v2.fit",
                                          kCachedObjectiveTolerance);
    const auto cached_validation_objective_report_delta =
        validate_objective_against_report(
            cached_validation_objective, shadow_report,
            "conditioned_v2.validation", kCachedObjectiveTolerance);
    const auto recomputed_fit_objective_report_delta =
        validate_objective_against_report(recomputed_fit_objective,
                                          shadow_report, "conditioned_v2.fit",
                                          kLiveObjectiveTolerance);
    const auto recomputed_validation_objective_report_delta =
        validate_objective_against_report(
            recomputed_validation_objective, shadow_report,
            "conditioned_v2.validation", kLiveObjectiveTolerance);

    std::ostringstream probe;
    probe.imbue(std::locale::classic());
    probe << std::scientific << std::setprecision(17);
    probe << "schema_id=" << kSchema << '\n';
    probe << "status=complete\n";
    probe << "result_role=development_only_mdn_forward_bridge\n";
    probe << "diagnostic_authority=diagnostic_only\n";
    probe << "benchmark_acceptance_authority=false\n";
    probe << "policy_path_used=false\n";
    probe << "training_performed=false\n";
    probe << "execution.production_checkpoint_loader_executed=true\n";
    probe << "execution.mdn_backbone_adapters_direct_feature_forward_"
             "executed=true\n";
    probe << "execution.representation_forward_executed=false\n";
    probe << "execution.mdn_distribution_forward_executed=false\n";
    probe << "execution.end_to_end_forecast_path_executed=false\n";
    probe << "execution.conditioned_artifact_loaded=true\n";
    probe << "execution.training_performed=false\n";
    probe << "execution.policy_path_used=false\n";
    probe << "checkpoint_written=false\n";
    probe << "history_input_used=false\n";
    probe << "holdout_input_used=false\n";
    probe << "input.representation_probe.sha256=" << kRepresentationProbeSha
          << '\n';
    probe << "input.cached_probe.sha256=" << kCachedProbeSha << '\n';
    probe << "input.mdn_checkpoint.sha256=" << kMdnCheckpointSha << '\n';
    probe << "input.v2_artifact.sha256=" << kV2ArtifactSha << '\n';
    probe << "input.shadow_report.sha256=" << kShadowReportSha << '\n';
    probe << "boundary.development=[0,730)\n";
    probe << "boundary.fit=[0,554)\n";
    probe << "boundary.purge=[554,584)\n";
    probe << "boundary.validation=[584,730)\n";
    probe << "boundary.maximum_anchor_loaded=729\n";
    probe << "data.max_anchor=729\n";
    probe << "boundary.anchor_730_or_later_opened=false\n";
    probe << "boundary.unconsumed_holdout=[1088,1170)\n";
    probe << "boundary.unconsumed_holdout_opened=false\n";
    probe << "data.representation_shape=[730,4,3,32]\n";
    probe << "data.cached_readout_shape=[730,3,3,400]\n";
    probe << "data.representation_row_count=" << representation.row_count
          << '\n';
    probe << "data.cached_row_count=" << cached.row_count << '\n';
    probe << "data.target_torch_equal=true\n";
    probe << "data.base_quote_difference.maximum_abs_delta="
          << representation.difference_max_abs_delta << '\n';
    probe << "data.quote_cross_edge.maximum_abs_delta="
          << representation.quote_max_abs_delta << '\n';
    probe << "model.component=ChannelContextMdn\n";
    probe << "model.shape=De32,Df9,C3,Hf1,K3,H128,depth2\n";
    probe << "model.target_coords=0,1,2,3,4,5,6,7,8\n";
    probe << "model.checkpoint_loader=channel_graph_first_inference_launcher_"
             "detail::load_channel_mdn_checkpoint_file\n";
    probe << "model.checkpoint_expected_identity_nonnull=true\n";
    probe << "model.checkpoint_full_expected_identity_populated=true\n";
    probe << "execution.device=cuda\n";
    probe << "execution.capture_batch_size=64\n";
    probe << "execution.context_mask=all_true\n";
    probe << "execution.no_grad=true\n";
    probe << "execution.eval=true\n";
    probe << "runtime.torch_version=" << TORCH_VERSION << '\n';
    probe << "runtime.cuda_runtime_version=" << runtime.cuda_runtime_version
          << '\n';
    emit_comparison(probe, "parity.recomputed_to_cached.readout",
                    feature_parity);
    emit_comparison(probe, "parity.recomputed_to_cached.prediction",
                    prediction_parity);
    probe << "parity.mdn_features.pass=true\n";
    probe << "parity.conditioned_predictions.pass=true\n";
    probe << "parity.cached_fit_to_shadow_report.maximum_abs_delta="
          << fit_report_max_delta << '\n';
    probe << "parity.cached_validation_to_shadow_report.maximum_abs_delta="
          << validation_report_max_delta << '\n';
    probe << "parity.recomputed_fit_to_shadow_report.maximum_abs_delta="
          << recomputed_fit_report_max_delta << '\n';
    probe << "parity.recomputed_validation_to_shadow_report.maximum_abs_delta="
          << recomputed_validation_report_max_delta << '\n';
    probe << "parity.cached_shadow_metric.threshold=" << kCachedMetricTolerance
          << '\n';
    probe << "parity.recomputed_shadow_metric.threshold="
          << kLiveMetricTolerance << '\n';
    probe << "parity.shadow_report.pass=true\n";
    probe << "parity.cached_shadow_metrics.pass=true\n";
    probe << "parity.recomputed_shadow_metrics.pass=true\n";
    probe << "parity.cached_fit_objective_to_shadow_report.maximum_abs_delta="
          << cached_fit_objective_report_delta << '\n';
    probe << "parity.cached_validation_objective_to_shadow_report.maximum_abs_"
             "delta="
          << cached_validation_objective_report_delta << '\n';
    probe << "parity.recomputed_fit_objective_to_shadow_report.maximum_abs_"
             "delta="
          << recomputed_fit_objective_report_delta << '\n';
    probe << "parity.recomputed_validation_objective_to_shadow_report.maximum_"
             "abs_delta="
          << recomputed_validation_objective_report_delta << '\n';
    probe << "parity.cached_shadow_objective.threshold="
          << kCachedObjectiveTolerance << '\n';
    probe << "parity.recomputed_shadow_objective.threshold="
          << kLiveObjectiveTolerance << '\n';
    probe << "parity.production_direct_objectives.pass=true\n";
    probe << "prediction.cached.digest=" << prediction_digest(cached_prediction)
          << '\n';
    probe << "prediction.recomputed.digest="
          << prediction_digest(recomputed_prediction) << '\n';
    emit_bridge_metric(probe, "conditioned_v2.cached.fit", cached_fit_metric);
    emit_bridge_metric(probe, "conditioned_v2.cached.validation",
                       cached_validation_metric);
    emit_bridge_metric(probe, "conditioned_v2.recomputed.fit",
                       recomputed_fit_metric);
    emit_bridge_metric(probe, "conditioned_v2.recomputed.validation",
                       recomputed_validation_metric);
    emit_direct_objective(probe, "production_direct_objective.cached.fit",
                          cached_fit_objective);
    emit_direct_objective(probe,
                          "production_direct_objective.cached.validation",
                          cached_validation_objective);
    emit_direct_objective(probe, "production_direct_objective.recomputed.fit",
                          recomputed_fit_objective);
    emit_direct_objective(probe,
                          "production_direct_objective.recomputed.validation",
                          recomputed_validation_objective);
    probe << "caveat.upstream_representation_trained_on_development_0_730="
             "true\n";
    probe << "caveat.upstream_mdn_trained_on_development_0_730=true\n";
    probe << "caveat.validation_withheld_from_affine_fit=true\n";
    probe << "caveat.validation_withheld_from_upstream_training=false\n";
    probe << "caveat.transductive_development_only=true\n";
    probe << "caveat.unseen_future_generalization_measured=false\n";
    probe << "caveat.policy_performance_measured=false\n";
    probe << "conclusion.production_loader_exercised=true\n";
    probe << "conclusion.mdn_backbone_adapters_direct_feature_forward_"
             "executed=true\n";
    probe << "conclusion.representation_forward_executed=false\n";
    probe << "conclusion.mdn_distribution_forward_executed=false\n";
    probe << "conclusion.end_to_end_forecast_path_executed=false\n";
    probe << "conclusion.cached_readout_parity_pass=true\n";
    probe << "conclusion.conditioned_affine_prediction_parity_pass=true\n";
    probe << "conclusion.shadow_metrics_reproduced=true\n";
    probe << "conclusion.production_direct_objectives_reproduced=true\n";
    probe << "conclusion.unconsumed_holdout_remains_sealed=true\n";
    probe << "conclusion.live_mdn_direct_feature_forward_bridge_valid=true\n";
    write_probe_atomic(options.output, probe.str());
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "mdn-forward bridge failure: "
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "mdn-forward bridge failure: " << error.what() << '\n';
  }
  return 1;
}

} // namespace mdn_forward_bridge

int main(int argc, char **argv) { return mdn_forward_bridge::run(argc, argv); }
