#define main mdn_historical_embedded_objective_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>
#include <unordered_map>

#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace mdn_historical_confirmation {

namespace production_mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace production_launcher = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

constexpr std::string_view kSchema =
    "synthetic_mdn_frozen_conditioned_affine_historical_confirmation_v1";
constexpr std::string_view kHistoricalProbeSha =
    "477758c9a25e05138c32f70c7fb4ded1aac855aa7cd2beeff7f9060708866ac5";
constexpr std::string_view kCaptureReportSha =
    "9b99039ca0f15c82256b5664e108725b66142e92da8eaf87be4de6fd54e5317c";
constexpr std::string_view kJobManifestSha =
    "d48c94c2bd78b0f4c71441d9ac86ac816580c61592796092e713b1f0732f196d";
constexpr std::string_view kRepresentationCheckpointSha =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr std::string_view kMdnCheckpointSha =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr std::string_view kV2ArtifactSha =
    "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739";
constexpr std::string_view kDevelopmentProbeSha =
    "ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851";
constexpr std::string_view kSemanticDigest =
    "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38";
constexpr std::string_view kGraphFingerprint = "d334e38b1887ae16";
constexpr std::string_view kHistoricalProbeOrigin =
    "/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v1/"
    "synthetic_mdn_frozen_feature_capture.historical_760_1088.v1/"
    "anchor_760_1088/mdn_edge_context_features.probe";
constexpr std::string_view kRepresentationCheckpointOrigin =
    "/cuwacunu/.runtime/cuwacunu_exec/components/"
    "wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/"
    "diz_b8a87dee0c986487/jobs/train/"
    "train_core_mtf_jepa_mae_vicreg.train."
    "channel_representation_mtf_jepa_mae_vicreg.attempt_000001/"
    "channel_representation.report.mtf_jepa_mae_vicreg.pt";
constexpr std::string_view kMdnCheckpointOrigin =
    "/cuwacunu/.runtime/cuwacunu_exec/components/"
    "wikimyei.inference.expected_value.mdn/spawns/"
    "syq_fd0cba7ed6f1feb8/jobs/train/"
    "train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/"
    "channel_inference.report.channel_mdn.pt";

constexpr int64_t kHistoricalBegin = 760;
constexpr int64_t kHistoricalEnd = 1088;
constexpr int64_t kHoldoutBegin = 1088;
constexpr int64_t kFitEnd = 554;
constexpr int64_t kPurgeEnd = 584;
constexpr int64_t kDevelopmentEnd = 730;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kReadoutWidth = 400;
constexpr int64_t kConditionedFeatureDim = 384;

constexpr double kMinimumDirection = 0.65;
constexpr double kMinimumRank = 0.65;
constexpr double kMinimumCorrelation = 0.50;
constexpr double kMaximumRmse = 0.025;
constexpr double kMinimumDirectionImprovement = 0.10;
constexpr double kMinimumRankImprovement = 0.10;
constexpr double kMaximumRmseRatio = 0.90;
constexpr double kCheckpointReportTolerance = 5.0e-6;

struct Options {
  std::filesystem::path historical_probe;
  std::filesystem::path capture_report;
  std::filesystem::path job_manifest;
  std::filesystem::path representation_checkpoint;
  std::filesystem::path mdn_checkpoint;
  std::filesystem::path v2_artifact;
  std::filesystem::path output;
  std::string schema_id;
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
  const auto absolute = std::filesystem::absolute(path, error);
  require(!error, "failed to normalize a historical-confirmation path");
  return absolute.lexically_normal();
}

Options parse_options(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *name) -> std::string {
      require(index + 1 < argc, std::string("missing value for ") + name);
      return argv[++index];
    };
    if (argument == "--historical-probe") {
      options.historical_probe = value("--historical-probe");
    } else if (argument == "--capture-report") {
      options.capture_report = value("--capture-report");
    } else if (argument == "--job-manifest") {
      options.job_manifest = value("--job-manifest");
    } else if (argument == "--representation-checkpoint") {
      options.representation_checkpoint = value("--representation-checkpoint");
    } else if (argument == "--mdn-checkpoint") {
      options.mdn_checkpoint = value("--mdn-checkpoint");
    } else if (argument == "--v2-artifact") {
      options.v2_artifact = value("--v2-artifact");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else if (argument == "--holdout-input" || argument == "--policy-input" ||
               argument == "--training-output" ||
               argument == "--checkpoint-output" || argument == "--fit-input") {
      fail("forbidden historical-confirmation argument: " + argument);
    } else {
      fail("unknown historical-confirmation argument: " + argument);
    }
  }
  require(
      !options.historical_probe.empty() && !options.capture_report.empty() &&
          !options.job_manifest.empty() &&
          !options.representation_checkpoint.empty() &&
          !options.mdn_checkpoint.empty() && !options.v2_artifact.empty() &&
          !options.output.empty() && !options.schema_id.empty(),
      "all historical-confirmation inputs, output, and schema are required");
  const std::set<std::filesystem::path> paths{
      normalized(options.historical_probe),
      normalized(options.capture_report),
      normalized(options.job_manifest),
      normalized(options.representation_checkpoint),
      normalized(options.mdn_checkpoint),
      normalized(options.v2_artifact),
      normalized(options.output)};
  require(paths.size() == 7,
          "all historical-confirmation paths must be distinct");
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

const std::string &kv_value(const KvMap &values, const std::string &key,
                            const char *surface) {
  const auto found = values.find(key);
  require(found != values.end(), std::string(surface) + " is missing " + key);
  return found->second;
}

void require_kv(const KvMap &values, const std::string &key,
                const std::string &expected, const char *surface) {
  require(kv_value(values, key, surface) == expected,
          std::string(surface) + " identity mismatch for " + key);
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
      32, 9, kChannelCount, 1, 3, 128, 2, torch::kFloat32, device, 8, 16,
      std::vector<int64_t>{0, 1, 2, 3, 4, 5, 6, 7, 8}, 1.0e-3, direct);
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
  identity.graph_order_fingerprint = std::string(kGraphFingerprint);
  identity.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  identity.target_coords = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  identity.channel_count = kChannelCount;
  identity.context_dim = 32;
  identity.future_horizon = 1;
  identity.mixture_count = 3;
  identity.hidden_width = 128;
  identity.residual_depth = 2;
  identity.feature_embedding_dim = 8;
  identity.channel_adapter_rank = 16;
  identity.direct_edge_return_readout_identity_mode = "edge_embedding_per_edge";
  identity.direct_edge_return_readout_base_edge_count = kEdgeCount;
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

void validate_v2_metadata(
    const production_mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata
        &metadata) {
  require(metadata.schema_version == 2 && metadata.run_only &&
              !metadata.policy_eligible &&
              metadata.feature_dim == kConditionedFeatureDim &&
              metadata.edge_count == kEdgeCount &&
              metadata.channel_count == kChannelCount &&
              metadata.fit_begin == 0 && metadata.fit_end == kFitEnd &&
              metadata.purge_begin == kFitEnd &&
              metadata.purge_end == kPurgeEnd &&
              metadata.validation_begin == kPurgeEnd &&
              metadata.validation_end == kDevelopmentEnd &&
              metadata.max_anchor == kDevelopmentEnd - 1 &&
              metadata.graph_order_fingerprint == kGraphFingerprint &&
              metadata.source_probe_sha256 == kDevelopmentProbeSha &&
              metadata.representation_checkpoint_sha256 ==
                  kRepresentationCheckpointSha &&
              metadata.mdn_checkpoint_sha256 == kMdnCheckpointSha &&
              metadata.semantic_tensor_digest == kSemanticDigest &&
              metadata.edge_ids ==
                  std::vector<std::string>(
                      {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"}) &&
              metadata.node_ids ==
                  std::vector<std::string>(
                      {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"}),
          "conditioned-affine v2 provenance identity mismatch");
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

double metric_mae(const Metric &metric) {
  return metric.abs_error / static_cast<double>(metric.count);
}

double metric_prediction_std(const Metric &metric) {
  const auto count = static_cast<double>(metric.count);
  const auto variance = metric.prediction_sq_sum / count -
                        std::pow(metric.prediction_sum / count, 2.0);
  return std::sqrt(std::max(variance, 0.0));
}

void emit_historical_metric(std::ostream &output, const std::string &prefix,
                            const Metric &metric) {
  output << prefix << ".count=" << metric.count << '\n';
  output << prefix << ".pair_count=" << metric.pair_count << '\n';
  output << prefix << ".mae=" << metric_mae(metric) << '\n';
  output << prefix << ".rmse=" << metric_rmse(metric) << '\n';
  output << prefix << ".directional_accuracy=" << metric_direction(metric)
         << '\n';
  output << prefix << ".pairwise_rank_accuracy=" << metric_rank(metric) << '\n';
  output << prefix << ".correlation=" << metric_correlation(metric) << '\n';
  output << prefix
         << ".prediction_population_std=" << metric_prediction_std(metric)
         << '\n';
}

void emit_objective(std::ostream &output, const std::string &prefix,
                    const DirectObjectiveSummary &objective) {
  output << prefix << ".total=" << objective.total << '\n';
  output << prefix << ".regression=" << objective.regression << '\n';
  output << prefix << ".direction=" << objective.direction << '\n';
  output << prefix << ".rank=" << objective.rank << '\n';
  output << prefix << ".valid_count=" << objective.valid_count << '\n';
  output << prefix << ".pairwise_valid_count=" << objective.pairwise_valid_count
         << '\n';
}

void validate_capture_provenance(const KvMap &report, const KvMap &manifest) {
  require_kv(report, "target_action", "run", "capture report");
  require_kv(report, "requested_anchor_index_begin", "760", "capture report");
  require_kv(report, "requested_anchor_index_end", "1088", "capture report");
  require_kv(report, "wave_streamed_anchor_count", "328", "capture report");
  require_kv(report, "steps_attempted", "6", "capture report");
  require_kv(report, "steps_completed", "6", "capture report");
  require_kv(report, "optimizer_steps", "0", "capture report");
  require_kv(report, "checkpoint_written", "false", "capture report");
  require_kv(report, "checkpoint_write_count", "0", "capture report");
  require_kv(report, "representation_checkpoint_loaded", "true",
             "capture report");
  require_kv(report, "mdn_checkpoint_loaded", "true", "capture report");
  require_kv(report, "mdn_edge_context_feature_probe_written", "true",
             "capture report");
  require_kv(report, "mdn_edge_context_feature_probe_row_count", "2952",
             "capture report");
  require_kv(report, "mdn_edge_context_feature_probe_error", "",
             "capture report");
  require_kv(report, "graph_order_fingerprint", std::string(kGraphFingerprint),
             "capture report");
  require_kv(report, "node_ids", "SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA",
             "capture report");
  require_kv(report, "edge_ids", "SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD",
             "capture report");
  require_kv(report, "representation_checkpoint_path",
             std::string(kRepresentationCheckpointOrigin), "capture report");
  require_kv(report, "mdn_checkpoint_path", std::string(kMdnCheckpointOrigin),
             "capture report");
  require_kv(report, "mdn_edge_context_feature_probe_path",
             std::string(kHistoricalProbeOrigin), "capture report");

  require_kv(manifest, "wave_action", "run", "job manifest");
  require_kv(manifest, "resolved_anchor_index_begin", "760", "job manifest");
  require_kv(manifest, "resolved_anchor_index_end", "1088", "job manifest");
  require_kv(manifest, "accepted_anchor_count", "1170", "job manifest");
  require_kv(manifest, "graph_order_fingerprint",
             std::string(kGraphFingerprint), "job manifest");
  require_kv(manifest, "node_ids", "SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA",
             "job manifest");
  require_kv(manifest, "edge_ids",
             "SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD", "job manifest");
  require_kv(manifest, "input_representation_checkpoint_path",
             std::string(kRepresentationCheckpointOrigin), "job manifest");
  require_kv(manifest, "input_mdn_checkpoint_path",
             std::string(kMdnCheckpointOrigin), "job manifest");
  require(kv_value(report, "source_cursor_token", "capture report") ==
              kv_value(manifest, "source_cursor_token", "job manifest"),
          "capture report and manifest source cursor tokens differ");
}

void validate_checkpoint_metric_against_capture(const Metric &metric,
                                                const KvMap &report) {
  require(
      parse_i64_strict(kv_value(report,
                                "direct_edge_return_readout_valid_count",
                                "capture report"),
                       "checkpoint count") == metric.count &&
          parse_i64_strict(
              kv_value(report,
                       "direct_edge_return_readout_pairwise_rank_valid_count",
                       "capture report"),
              "checkpoint pair count") == metric.pair_count,
      "checkpoint prediction metric domain differs from capture report");
  const std::array<std::pair<const char *, double>, 4> comparisons{{
      {"direct_edge_return_readout_ev_rmse", metric_rmse(metric)},
      {"direct_edge_return_readout_directional_accuracy",
       metric_direction(metric)},
      {"direct_edge_return_readout_pairwise_rank_accuracy",
       metric_rank(metric)},
      {"direct_edge_return_readout_correlation", metric_correlation(metric)},
  }};
  for (const auto &[key, actual] : comparisons) {
    const auto expected =
        parse_f64_strict(kv_value(report, key, "capture report"), key);
    require(std::fabs(actual - expected) <= kCheckpointReportTolerance,
            std::string("checkpoint metric differs from capture report: ") +
                key);
  }
}

int run(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_options(argc, argv);
    require(options.schema_id == kSchema,
            "historical-confirmation schema mismatch");
    require(!std::filesystem::exists(options.output) &&
                !std::filesystem::exists(options.output.string() + ".tmp"),
            "refusing to replace a historical-confirmation output");

    const std::array<std::pair<std::filesystem::path, std::string_view>, 6>
        pinned_inputs{{
            {options.historical_probe, kHistoricalProbeSha},
            {options.capture_report, kCaptureReportSha},
            {options.job_manifest, kJobManifestSha},
            {options.representation_checkpoint, kRepresentationCheckpointSha},
            {options.mdn_checkpoint, kMdnCheckpointSha},
            {options.v2_artifact, kV2ArtifactSha},
        }};
    for (const auto &[path, expected] : pinned_inputs) {
      require(std::filesystem::is_regular_file(path) &&
                  sha256_file(path) == expected,
              "pinned historical-confirmation input identity mismatch");
    }

    const auto report = read_kv(options.capture_report, "capture report");
    const auto manifest = read_kv(options.job_manifest, "job manifest");
    validate_capture_provenance(report, manifest);
    const auto dataset =
        read_probe(options.historical_probe, kHistoricalBegin, kHistoricalEnd);
    require(dataset.record_schema ==
                    "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1" &&
                dataset.row_count == 2952 &&
                dataset.anchors.front() == kHistoricalBegin &&
                dataset.anchors.back() == kHistoricalEnd - 1 &&
                dataset.readout_features.sizes() ==
                    torch::IntArrayRef({kHistoricalEnd - kHistoricalBegin,
                                        kEdgeCount, kChannelCount,
                                        kReadoutWidth}) &&
                dataset.prefix_torch_equal &&
                dataset.prefix_max_abs_delta == 0.0,
            "historical feature probe contract mismatch");

    const auto runtime = configure_deterministic_runtime();
    reset_deterministic_seed();
    auto model = make_production_model(torch::Device(torch::kCUDA));
    load_production_checkpoint(options.mdn_checkpoint, model);

    torch::NoGradGuard no_grad;
    const auto features = dataset.readout_features.to(torch::kCUDA);
    const auto checkpoint_prediction =
        model->direct_edge_head->forward_readout_features(features);
    auto conditioned =
        production_mdn::load_per_edge_conditioned_affine_return_head(
            options.v2_artifact);
    validate_v2_metadata(conditioned.metadata);
    conditioned.head->to(torch::kCUDA);
    conditioned.head->eval();
    conditioned.head->validate_registered_state();
    const auto conditioned_prediction =
        conditioned.head->forward_readout_features(features);
    require(checkpoint_prediction.sizes() == dataset.target.sizes() &&
                conditioned_prediction.sizes() == dataset.target.sizes() &&
                torch::isfinite(checkpoint_prediction).all().item<bool>() &&
                torch::isfinite(conditioned_prediction).all().item<bool>(),
            "historical prediction surface is invalid");

    const auto checkpoint_metric =
        evaluate_prediction(checkpoint_prediction, dataset.target);
    const auto conditioned_metric =
        evaluate_prediction(conditioned_prediction, dataset.target);
    require_metric_counts(checkpoint_metric, 2952, 2952,
                          "historical checkpoint head");
    require_metric_counts(conditioned_metric, 2952, 2952,
                          "historical conditioned head");
    validate_checkpoint_metric_against_capture(checkpoint_metric, report);
    const auto checkpoint_objective = compute_production_direct_objective(
        checkpoint_prediction, dataset.target);
    const auto conditioned_objective = compute_production_direct_objective(
        conditioned_prediction, dataset.target);
    require(checkpoint_objective.valid_count == 2952 &&
                checkpoint_objective.pairwise_valid_count == 2952 &&
                conditioned_objective.valid_count == 2952 &&
                conditioned_objective.pairwise_valid_count == 2952,
            "historical production-objective domain mismatch");

    const auto direction_improvement = metric_direction(conditioned_metric) -
                                       metric_direction(checkpoint_metric);
    const auto rank_improvement =
        metric_rank(conditioned_metric) - metric_rank(checkpoint_metric);
    const auto rmse_ratio =
        metric_rmse(conditioned_metric) / metric_rmse(checkpoint_metric);
    const bool direction_gate =
        metric_direction(conditioned_metric) >= kMinimumDirection;
    const bool rank_gate = metric_rank(conditioned_metric) >= kMinimumRank;
    const bool correlation_gate =
        metric_correlation(conditioned_metric) >= kMinimumCorrelation;
    const bool rmse_gate = metric_rmse(conditioned_metric) <= kMaximumRmse;
    const bool direction_improvement_gate =
        direction_improvement >= kMinimumDirectionImprovement;
    const bool rank_improvement_gate =
        rank_improvement >= kMinimumRankImprovement;
    const bool rmse_improvement_gate = rmse_ratio <= kMaximumRmseRatio;
    const bool scientific_gate = direction_gate && rank_gate &&
                                 correlation_gate && rmse_gate &&
                                 direction_improvement_gate &&
                                 rank_improvement_gate && rmse_improvement_gate;

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::scientific << std::setprecision(17);
    output << "schema_id=" << kSchema << '\n';
    output << "status=complete\n";
    output << "result_role=unseen_historical_no_refit_confirmation\n";
    output << "diagnostic_authority=diagnostic_only\n";
    output << "benchmark_acceptance_authority=false\n";
    output << "execution.provenance_pass=true\n";
    output << "execution.production_checkpoint_loader_executed=true\n";
    output << "execution.checkpoint_head_forward_executed=true\n";
    output << "execution.conditioned_head_forward_executed=true\n";
    output << "execution.training_performed=false\n";
    output << "execution.refit_performed=false\n";
    output << "execution.optimizer_steps=0\n";
    output << "execution.policy_path_used=false\n";
    output << "execution.checkpoint_written=false\n";
    output << "execution.no_grad=true\n";
    output << "execution.eval=true\n";
    output << "execution.device=cuda\n";
    output << "boundary.development=[0,730)\n";
    output << "boundary.unseen_historical=[760,1088)\n";
    output << "boundary.maximum_anchor_loaded=1087\n";
    output << "boundary.final_holdout=[1088,1170)\n";
    output << "boundary.final_holdout_opened=false\n";
    output << "data.anchor_count=328\n";
    output << "data.row_count=" << dataset.row_count << '\n';
    output << "data.feature_shape=[328,3,3,400]\n";
    output << "data.dynamic_prefix_torch_equal=true\n";
    output << "data.dynamic_prefix_maximum_abs_delta="
           << dataset.prefix_max_abs_delta << '\n';
    output << "input.historical_probe.sha256=" << kHistoricalProbeSha << '\n';
    output << "input.capture_report.sha256=" << kCaptureReportSha << '\n';
    output << "input.job_manifest.sha256=" << kJobManifestSha << '\n';
    output << "input.representation_checkpoint.sha256="
           << kRepresentationCheckpointSha << '\n';
    output << "input.mdn_checkpoint.sha256=" << kMdnCheckpointSha << '\n';
    output << "input.v2_artifact.sha256=" << kV2ArtifactSha << '\n';
    output << "artifact.semantic_tensor_digest=" << kSemanticDigest << '\n';
    output << "artifact.fit_range=[0,554)\n";
    output << "artifact.purge_range=[554,584)\n";
    output << "artifact.validation_range=[584,730)\n";
    output << "artifact.maximum_anchor=729\n";
    output << "artifact.run_only=true\n";
    output << "artifact.policy_eligible=false\n";
    output << "runtime.torch_version=" << TORCH_VERSION << '\n';
    output << "runtime.cuda_runtime_version=" << runtime.cuda_runtime_version
           << '\n';
    output << "prediction.checkpoint.digest="
           << prediction_digest(checkpoint_prediction) << '\n';
    output << "prediction.conditioned.digest="
           << prediction_digest(conditioned_prediction) << '\n';
    emit_historical_metric(output, "historical.checkpoint", checkpoint_metric);
    emit_historical_metric(output, "historical.conditioned",
                           conditioned_metric);
    emit_objective(output, "production_direct_objective.checkpoint",
                   checkpoint_objective);
    emit_objective(output, "production_direct_objective.conditioned",
                   conditioned_objective);
    output << "improvement.directional_accuracy=" << direction_improvement
           << '\n';
    output << "improvement.pairwise_rank_accuracy=" << rank_improvement << '\n';
    output << "improvement.rmse_ratio=" << rmse_ratio << '\n';
    output << "scientific_gate.threshold.minimum_direction="
           << kMinimumDirection << '\n';
    output << "scientific_gate.threshold.minimum_rank=" << kMinimumRank << '\n';
    output << "scientific_gate.threshold.minimum_correlation="
           << kMinimumCorrelation << '\n';
    output << "scientific_gate.threshold.maximum_rmse=" << kMaximumRmse << '\n';
    output << "scientific_gate.threshold.minimum_direction_improvement="
           << kMinimumDirectionImprovement << '\n';
    output << "scientific_gate.threshold.minimum_rank_improvement="
           << kMinimumRankImprovement << '\n';
    output << "scientific_gate.threshold.maximum_rmse_ratio="
           << kMaximumRmseRatio << '\n';
    output << "scientific_gate.direction.pass=" << bool_text(direction_gate)
           << '\n';
    output << "scientific_gate.rank.pass=" << bool_text(rank_gate) << '\n';
    output << "scientific_gate.correlation.pass=" << bool_text(correlation_gate)
           << '\n';
    output << "scientific_gate.rmse.pass=" << bool_text(rmse_gate) << '\n';
    output << "scientific_gate.direction_improvement.pass="
           << bool_text(direction_improvement_gate) << '\n';
    output << "scientific_gate.rank_improvement.pass="
           << bool_text(rank_improvement_gate) << '\n';
    output << "scientific_gate.rmse_improvement.pass="
           << bool_text(rmse_improvement_gate) << '\n';
    output << "scientific_gate.pass=" << bool_text(scientific_gate) << '\n';
    output << "conclusion.final_holdout_remains_sealed=true\n";
    output << "conclusion.no_refit_historical_confirmation_complete=true\n";
    write_probe_atomic(options.output, output.str());
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "historical confirmation failure: "
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "historical confirmation failure: " << error.what() << '\n';
  }
  return 1;
}

} // namespace mdn_historical_confirmation

int main(int argc, char **argv) {
  return mdn_historical_confirmation::run(argc, argv);
}
