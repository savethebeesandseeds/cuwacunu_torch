#define main mdn_final_holdout_embedded_objective_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>
#include <cstring>
#include <unordered_map>

#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace mdn_final_holdout {

namespace production_mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace production_launcher = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

constexpr std::string_view kSchema =
    "synthetic_mdn_frozen_conditioned_affine_final_holdout_v1";
constexpr std::string_view kRepresentationCheckpointSha =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr std::string_view kMdnCheckpointSha =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr std::string_view kV2ArtifactSha =
    "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739";
constexpr std::string_view kSemanticDigest =
    "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38";
constexpr std::string_view kGraphFingerprint = "d334e38b1887ae16";
constexpr std::string_view kDevelopmentProbeSha =
    "ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851";
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
constexpr std::string_view kSourceCursorToken =
    "version=ujcamei.graph_anchor_cursor_report.v1|graph=d334e38b1887ae16|"
    "reference_edge=SYNALPHASYNUSD|Hx=30|Hf=1|edges=3|accepted=1170|"
    "candidates=1170|skipped=0|first=1580428799999|last=1681430399999";
constexpr std::string_view kRepresentationProbeRecord =
    "kikijyeba.synthetic.representation_edge_feature_probe.v1";
constexpr std::string_view kMdnProbeRecord =
    "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1";
constexpr std::string_view kProbeHeader =
    "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
    "edge_id,base_node_id,quote_node_id,channel_index,"
    "target_edge_close_return,feature_count,feature_values";

constexpr int64_t kFinalBegin = 1088;
constexpr int64_t kFinalEnd = 1170;
constexpr int64_t kFinalAnchorCount = kFinalEnd - kFinalBegin;
constexpr int64_t kFirstBatchCount = 64;
constexpr int64_t kSecondBatchCount = 18;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kContextDim = 32;
constexpr int64_t kDynamicFeatureDim = 384;
constexpr int64_t kReadoutWidth = 400;
constexpr int64_t kExpectedRows =
    kFinalAnchorCount * kEdgeCount * kChannelCount;
constexpr int64_t kExpectedGroupRows = kFinalAnchorCount * kChannelCount;
constexpr int64_t kExpectedBestAssetCount = kFinalAnchorCount * kChannelCount;
constexpr double kFullFeatureDescriptiveTolerance = 1.0e-6;
constexpr double kMinimumDirection = 0.65;
constexpr double kMinimumRank = 0.65;
constexpr double kMinimumCorrelation = 0.50;
constexpr double kMaximumRmse = 0.025;
constexpr double kMinimumDirectionImprovement = 0.10;
constexpr double kMinimumRankImprovement = 0.10;
constexpr double kMaximumRmseRatio = 0.90;
constexpr double kMarginEpsilon = 0.001;

using KvMap = std::map<std::string, std::string>;

struct DirectObjectiveSummary {
  double total{0.0};
  double regression{0.0};
  double direction{0.0};
  double rank{0.0};
  int64_t valid_count{0};
  int64_t pairwise_valid_count{0};
};

struct Options {
  std::filesystem::path representation_probe;
  std::filesystem::path mdn_probe;
  std::filesystem::path capture_report;
  std::filesystem::path job_manifest;
  std::filesystem::path representation_checkpoint;
  std::filesystem::path mdn_checkpoint;
  std::filesystem::path v2_artifact;
  std::filesystem::path output;
  std::string representation_probe_sha256;
  std::string mdn_probe_sha256;
  std::string capture_report_sha256;
  std::string job_manifest_sha256;
  std::string schema_id;
};

struct RepresentationSurface {
  torch::Tensor context;
  torch::Tensor target;
  std::array<int64_t, kFinalAnchorCount> anchor_keys{};
  int64_t row_count{0};
  double difference_max_abs_delta{0.0};
  double quote_max_abs_delta{0.0};
};

struct MdnSurface {
  torch::Tensor readout_features;
  torch::Tensor target;
  std::array<int64_t, kFinalAnchorCount> anchor_keys{};
  int64_t row_count{0};
};

struct TensorComparison {
  bool torch_equal{false};
  bool raw_bytes_equal{false};
  double maximum_abs_delta{std::numeric_limits<double>::infinity()};
};

struct DescriptiveMetric {
  double signed_error{0.0};
  double prediction_population_std{0.0};
  double target_population_std{0.0};
  double prediction_to_target_std_ratio{0.0};
  int64_t best_asset_valid_count{0};
  int64_t best_asset_agreement_count{0};
  int64_t margin_valid_count{0};
  int64_t margin_direction_correct{0};
  int64_t near_zero_target_count{0};
  int64_t rank_margin_valid_count{0};
  int64_t rank_margin_correct{0};
};

struct EvaluationPass {
  torch::Tensor checkpoint_prediction;
  torch::Tensor conditioned_prediction;
  torch::Tensor suffix_prediction;
  Metric checkpoint_metric;
  Metric conditioned_metric;
  std::array<Metric, kEdgeCount> checkpoint_per_edge;
  std::array<Metric, kEdgeCount> conditioned_per_edge;
  std::array<Metric, kChannelCount> checkpoint_per_channel;
  std::array<Metric, kChannelCount> conditioned_per_channel;
  DirectObjectiveSummary checkpoint_objective;
  DirectObjectiveSummary conditioned_objective;
  DescriptiveMetric checkpoint_descriptive;
  DescriptiveMetric conditioned_descriptive;
  TensorComparison dynamic_prefix;
  TensorComparison full_features;
  TensorComparison suffix_ignored;
  std::string checkpoint_digest;
  std::string conditioned_digest;
  std::string suffix_digest;
  bool model_eval{false};
  bool checkpoint_head_eval{false};
  bool conditioned_head_eval{false};
  bool conditioned_parameters_frozen{false};
  bool suffix_input_genuinely_changed{false};
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
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
  identity.graph_order_fingerprint = std::string(kGraphFingerprint);
  identity.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  identity.target_coords = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  identity.channel_count = kChannelCount;
  identity.context_dim = kContextDim;
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
  require(
      metadata.schema_version == 2 && metadata.run_only &&
          !metadata.policy_eligible && metadata.conditioning_alpha == 1.0e-10 &&
          metadata.feature_dim == kDynamicFeatureDim &&
          metadata.edge_count == kEdgeCount &&
          metadata.channel_count == kChannelCount && metadata.fit_begin == 0 &&
          metadata.fit_end == 554 && metadata.purge_begin == 554 &&
          metadata.purge_end == 584 && metadata.validation_begin == 584 &&
          metadata.validation_end == 730 && metadata.max_anchor == 729 &&
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

bool is_sha256(const std::string &value) {
  return value.size() == 64 &&
         std::all_of(value.begin(), value.end(), [](char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

std::filesystem::path normalized(const std::filesystem::path &path) {
  std::error_code error;
  const auto absolute = std::filesystem::absolute(path, error);
  require(!error, "failed to normalize a final-holdout path");
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
    if (argument == "--representation-probe") {
      options.representation_probe = value("--representation-probe");
    } else if (argument == "--mdn-probe") {
      options.mdn_probe = value("--mdn-probe");
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
    } else if (argument == "--representation-probe-sha256") {
      options.representation_probe_sha256 =
          value("--representation-probe-sha256");
    } else if (argument == "--mdn-probe-sha256") {
      options.mdn_probe_sha256 = value("--mdn-probe-sha256");
    } else if (argument == "--capture-report-sha256") {
      options.capture_report_sha256 = value("--capture-report-sha256");
    } else if (argument == "--job-manifest-sha256") {
      options.job_manifest_sha256 = value("--job-manifest-sha256");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--source-csv" || argument == "--historical-input" ||
               argument == "--fit-input" || argument == "--refit-input" ||
               argument == "--training-output" ||
               argument == "--checkpoint-output" ||
               argument == "--policy-input") {
      fail("forbidden final-holdout argument: " + argument);
    } else {
      fail("unknown final-holdout argument: " + argument);
    }
  }
  require(!options.representation_probe.empty() && !options.mdn_probe.empty() &&
              !options.capture_report.empty() &&
              !options.job_manifest.empty() &&
              !options.representation_checkpoint.empty() &&
              !options.mdn_checkpoint.empty() && !options.v2_artifact.empty() &&
              !options.output.empty() && !options.schema_id.empty(),
          "all final-holdout paths, output, and schema are required");
  require(is_sha256(options.representation_probe_sha256) &&
              is_sha256(options.mdn_probe_sha256) &&
              is_sha256(options.capture_report_sha256) &&
              is_sha256(options.job_manifest_sha256),
          "all runtime-created final-holdout SHA-256 values are required");
  const std::set<std::filesystem::path> paths{
      normalized(options.representation_probe),
      normalized(options.mdn_probe),
      normalized(options.capture_report),
      normalized(options.job_manifest),
      normalized(options.representation_checkpoint),
      normalized(options.mdn_checkpoint),
      normalized(options.v2_artifact),
      normalized(options.output)};
  require(paths.size() == 8, "all final-holdout paths must be distinct");
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

void validate_capture_provenance(const KvMap &report, const KvMap &manifest,
                                 const Options &options) {
  const std::array<std::pair<std::string, std::string>, 34> report_values{{
      {"training_id", "real_channel_mdn_train_core_v1"},
      {"component_assembly_id", "mdn_v1"},
      {"input_representation_assembly_id", "mtf_jepa_mae_vicreg_v1"},
      {"context_contract", "graph_order.channel_node_representation.v1"},
      {"output_contract", "graph_order.channel_node_future_distribution.v1"},
      {"graph_order_fingerprint", std::string(kGraphFingerprint)},
      {"node_ids", "SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA"},
      {"edge_ids", "SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD"},
      {"channel_count", "3"},
      {"context_dim", "32"},
      {"dtype", "float32"},
      {"device", "cuda"},
      {"effective_batch_size", "64"},
      {"batch_size_source", "derived"},
      {"target_action", "run"},
      {"requested_anchor_index_begin", "1088"},
      {"requested_anchor_index_end", "1170"},
      {"source_anchor_count", "1170"},
      {"wave_streamed_anchor_count", "82"},
      {"steps_attempted", "2"},
      {"steps_completed", "2"},
      {"skipped_batches", "0"},
      {"optimizer_steps", "0"},
      {"representation_checkpoint_loaded", "true"},
      {"mdn_checkpoint_loaded", "true"},
      {"checkpoint_written", "false"},
      {"checkpoint_write_count", "0"},
      {"representation_edge_feature_probe_written", "true"},
      {"representation_edge_feature_probe_row_count", "738"},
      {"representation_edge_feature_probe_error", ""},
      {"mdn_edge_context_feature_probe_written", "true"},
      {"mdn_edge_context_feature_probe_row_count", "738"},
      {"mdn_edge_context_feature_probe_error", ""},
      {"nonfinite_output_count", "0"},
  }};
  for (const auto &[key, expected] : report_values) {
    require_kv(report, key, expected, "capture report");
  }
  require_kv(report, "source_cursor_token", std::string(kSourceCursorToken),
             "capture report");
  require_kv(report, "finite_parameter_check", "1", "capture report");
  require_kv(report, "representation_parameter_device_check", "true",
             "capture report");
  require_kv(report, "mdn_parameter_device_check", "true", "capture report");
  require_kv(report, "runtime_lls_emitted", "true", "capture report");
  require_kv(report, "representation_checkpoint_path",
             std::string(kRepresentationCheckpointOrigin), "capture report");
  require_kv(report, "mdn_checkpoint_path", std::string(kMdnCheckpointOrigin),
             "capture report");

  const auto representation_probe_path = std::filesystem::path(kv_value(
      report, "representation_edge_feature_probe_path", "capture report"));
  const auto mdn_probe_path = std::filesystem::path(kv_value(
      report, "mdn_edge_context_feature_probe_path", "capture report"));
  const auto report_path =
      std::filesystem::path(kv_value(report, "report_path", "capture report"));
  require(representation_probe_path.filename() ==
                  "representation_edge_features.probe" &&
              mdn_probe_path.filename() == "mdn_edge_context_features.probe" &&
              report_path.filename() == "channel_inference.report" &&
              representation_probe_path.parent_path() ==
                  mdn_probe_path.parent_path() &&
              representation_probe_path.parent_path() ==
                  report_path.parent_path(),
          "capture report output path contract mismatch");
  require(normalized(representation_probe_path) ==
                  normalized(options.representation_probe) &&
              normalized(mdn_probe_path) == normalized(options.mdn_probe) &&
              normalized(report_path) == normalized(options.capture_report),
          "capture report paths do not identify the sealed helper inputs");

  const std::array<std::pair<std::string, std::string>, 20> manifest_values{{
      {"manifest_format", "kikijyeba.runtime.job_manifest.v1"},
      {"job_kind", "channel_inference_mdn"},
      {"protocol_kind", "channel_graph_first"},
      {"wave_action", "run"},
      {"wave_mode", "run|debug"},
      {"mutated_components", ""},
      {"frozen_components",
       "wikimyei.representation.encoding.mtf_jepa_mae_vicreg"},
      {"source_range_policy", "anchor_index"},
      {"source_order_policy", "sequential"},
      {"resolved_anchor_index_begin", "1088"},
      {"resolved_anchor_index_end", "1170"},
      {"accepted_anchor_count", "1170"},
      {"candidate_anchor_count", "1170"},
      {"skipped_outside_common_range", "0"},
      {"skipped_missing_edge_coverage", "0"},
      {"skipped_failed_fetch_probe", "0"},
      {"duplicate_anchor_count", "0"},
      {"graph_order_fingerprint", std::string(kGraphFingerprint)},
      {"node_ids", "SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA"},
      {"edge_ids", "SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD"},
  }};
  for (const auto &[key, expected] : manifest_values) {
    require_kv(manifest, key, expected, "job manifest");
  }
  require_kv(manifest, "source_cursor_token", std::string(kSourceCursorToken),
             "job manifest");
  require_kv(manifest, "input_representation_checkpoint_path",
             std::string(kRepresentationCheckpointOrigin), "job manifest");
  require_kv(manifest, "input_mdn_checkpoint_path",
             std::string(kMdnCheckpointOrigin), "job manifest");
  require_kv(manifest, "probe_sidecar_enabled", "true", "job manifest");
  require_kv(manifest, "policy_family_id", "", "job manifest");
  require(kv_value(report, "source_cursor_token", "capture report") ==
              kv_value(manifest, "source_cursor_token", "job manifest"),
          "capture report and job manifest cursor identities differ");
}

RepresentationSurface
read_representation_probe(const std::filesystem::path &path) {
  std::ifstream input(path);
  require(static_cast<bool>(input),
          "failed to open final representation probe");
  std::string line;
  require(std::getline(input, line) && line == kProbeHeader,
          "final representation probe header mismatch");

  RepresentationSurface surface;
  surface.context =
      torch::full({kFinalAnchorCount, kNodeCount, kChannelCount, kContextDim},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  surface.target =
      torch::full({kFinalAnchorCount, kEdgeCount, kChannelCount},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  auto context = surface.context.accessor<float, 4>();
  auto target = surface.target.accessor<float, 3>();
  const std::array<std::string, kEdgeCount> edge_ids{
      "SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  const std::array<std::string, kEdgeCount> base_ids{"SYNALPHA", "SYNBETA",
                                                     "SYNGAMMA"};

  while (std::getline(input, line)) {
    require(!line.empty(), "final representation probe contains an empty row");
    const auto columns = split_exact(line, ',');
    require(columns.size() == 12,
            "final representation probe row must contain 12 columns");
    require(columns[0] == kRepresentationProbeRecord,
            "final representation probe record schema mismatch");
    const auto anchor_key = parse_i64_strict(columns[1], "anchor_key");
    const auto anchor = parse_i64_strict(columns[2], "anchor_index");
    const auto local_anchor =
        parse_i64_strict(columns[3], "anchor_local_index");
    const auto edge = parse_i64_strict(columns[4], "edge_index");
    const auto channel = parse_i64_strict(columns[8], "channel_index");
    require(anchor >= kFinalBegin && anchor < kFinalEnd && edge >= 0 &&
                edge < kEdgeCount && channel >= 0 && channel < kChannelCount &&
                local_anchor == (anchor - kFinalBegin) % kFirstBatchCount,
            "final representation probe coordinate is outside [1088,1170)");
    const auto anchor_offset = anchor - kFinalBegin;
    const auto expected_ordinal =
        (anchor_offset * kEdgeCount + edge) * kChannelCount + channel;
    require(expected_ordinal == surface.row_count,
            "final representation probe rows are not in canonical order");
    if (edge == 0 && channel == 0) {
      require(anchor_key > 0 &&
                  (anchor_offset == 0 ||
                   anchor_key > surface.anchor_keys[static_cast<std::size_t>(
                                    anchor_offset - 1)]),
              "final representation anchor keys are not strictly increasing");
      surface.anchor_keys[static_cast<std::size_t>(anchor_offset)] = anchor_key;
    } else {
      require(surface.anchor_keys[static_cast<std::size_t>(anchor_offset)] ==
                  anchor_key,
              "final representation rows disagree on their anchor key");
    }
    require(columns[5] == edge_ids[static_cast<std::size_t>(edge)] &&
                columns[6] == base_ids[static_cast<std::size_t>(edge)] &&
                columns[7] == "SYNUSD",
            "final representation probe graph identity mismatch");
    require(parse_i64_strict(columns[10], "feature_count") == 3 * kContextDim,
            "final representation feature count is not 96");
    const auto features = split_exact(columns[11], ';');
    require(static_cast<int64_t>(features.size()) == 3 * kContextDim,
            "final representation feature vector width mismatch");
    target[anchor_offset][edge][channel] =
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
        context[anchor_offset][0][channel][feature] = quote;
      } else {
        surface.quote_max_abs_delta =
            std::max(surface.quote_max_abs_delta,
                     std::fabs(static_cast<double>(
                         quote - context[anchor_offset][0][channel][feature])));
      }
      context[anchor_offset][edge + 1][channel][feature] = base;
    }
    ++surface.row_count;
  }
  require(input.good() || input.eof(),
          "failed while reading final representation probe");
  require(surface.row_count == kExpectedRows &&
              torch::isfinite(surface.context).all().item<bool>() &&
              torch::isfinite(surface.target).all().item<bool>() &&
              surface.difference_max_abs_delta <=
                  kFullFeatureDescriptiveTolerance &&
              surface.quote_max_abs_delta == 0.0,
          "final representation probe is incomplete or invalid");
  return surface;
}

MdnSurface read_mdn_probe(const std::filesystem::path &path) {
  std::ifstream input(path);
  require(static_cast<bool>(input), "failed to open final MDN probe");
  std::string line;
  require(std::getline(input, line) && line == kProbeHeader,
          "final MDN probe header mismatch");

  MdnSurface surface;
  surface.readout_features =
      torch::full({kFinalAnchorCount, kEdgeCount, kChannelCount, kReadoutWidth},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  surface.target =
      torch::full({kFinalAnchorCount, kEdgeCount, kChannelCount},
                  std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
  auto readout = surface.readout_features.accessor<float, 4>();
  auto target = surface.target.accessor<float, 3>();
  const std::array<std::string, kEdgeCount> edge_ids{
      "SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  const std::array<std::string, kEdgeCount> base_ids{"SYNALPHA", "SYNBETA",
                                                     "SYNGAMMA"};

  while (std::getline(input, line)) {
    require(!line.empty(), "final MDN probe contains an empty row");
    const auto columns = split_exact(line, ',');
    require(columns.size() == 12,
            "final MDN probe row must contain 12 columns");
    require(columns[0] == kMdnProbeRecord,
            "final MDN probe record schema mismatch");
    const auto anchor_key = parse_i64_strict(columns[1], "anchor_key");
    const auto anchor = parse_i64_strict(columns[2], "anchor_index");
    const auto local_anchor =
        parse_i64_strict(columns[3], "anchor_local_index");
    const auto edge = parse_i64_strict(columns[4], "edge_index");
    const auto channel = parse_i64_strict(columns[8], "channel_index");
    require(anchor >= kFinalBegin && anchor < kFinalEnd && edge >= 0 &&
                edge < kEdgeCount && channel >= 0 && channel < kChannelCount &&
                local_anchor == (anchor - kFinalBegin) % kFirstBatchCount,
            "final MDN probe coordinate is outside [1088,1170)");
    const auto anchor_offset = anchor - kFinalBegin;
    const auto expected_ordinal =
        (anchor_offset * kEdgeCount + edge) * kChannelCount + channel;
    require(expected_ordinal == surface.row_count,
            "final MDN probe rows are not in canonical order");
    if (edge == 0 && channel == 0) {
      require(anchor_key > 0 &&
                  (anchor_offset == 0 ||
                   anchor_key > surface.anchor_keys[static_cast<std::size_t>(
                                    anchor_offset - 1)]),
              "final MDN anchor keys are not strictly increasing");
      surface.anchor_keys[static_cast<std::size_t>(anchor_offset)] = anchor_key;
    } else {
      require(surface.anchor_keys[static_cast<std::size_t>(anchor_offset)] ==
                  anchor_key,
              "final MDN rows disagree on their anchor key");
    }
    require(columns[5] == edge_ids[static_cast<std::size_t>(edge)] &&
                columns[6] == base_ids[static_cast<std::size_t>(edge)] &&
                columns[7] == "SYNUSD",
            "final MDN probe graph identity mismatch");
    require(parse_i64_strict(columns[10], "feature_count") == kReadoutWidth,
            "final MDN feature count is not 400");
    const auto features = split_exact(columns[11], ';');
    require(static_cast<int64_t>(features.size()) == kReadoutWidth,
            "final MDN feature vector width mismatch");
    target[anchor_offset][edge][channel] =
        static_cast<float>(parse_f64_strict(columns[9], "target"));
    for (int64_t feature = 0; feature < kReadoutWidth; ++feature) {
      readout[anchor_offset][edge][channel][feature] =
          static_cast<float>(parse_f64_strict(
              features[static_cast<std::size_t>(feature)], "readout feature"));
    }
    ++surface.row_count;
  }
  require(input.good() || input.eof(), "failed while reading final MDN probe");
  require(surface.row_count == kExpectedRows &&
              torch::isfinite(surface.readout_features).all().item<bool>() &&
              torch::isfinite(surface.target).all().item<bool>(),
          "final MDN probe is incomplete or non-finite");
  return surface;
}

TensorComparison compare_tensors(const torch::Tensor &lhs_input,
                                 const torch::Tensor &rhs_input) {
  const auto lhs =
      lhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto rhs =
      rhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  require(lhs.sizes() == rhs.sizes() && lhs.numel() > 0 &&
              torch::isfinite(lhs).all().item<bool>() &&
              torch::isfinite(rhs).all().item<bool>(),
          "invalid final-holdout tensor comparison");
  const auto delta = (lhs - rhs).abs();
  return {.torch_equal = torch::equal(lhs, rhs),
          .raw_bytes_equal =
              std::memcmp(lhs.data_ptr<float>(), rhs.data_ptr<float>(),
                          static_cast<std::size_t>(lhs.nbytes())) == 0,
          .maximum_abs_delta = delta.max().item<double>()};
}

torch::Tensor
recompute_readout(production_mdn::ChannelContextMdn &model,
                  const torch::Tensor &representation_context_cpu) {
  require(representation_context_cpu.sizes() ==
              torch::IntArrayRef(
                  {kFinalAnchorCount, kNodeCount, kChannelCount, kContextDim}),
          "final representation context shape mismatch");
  std::vector<torch::Tensor> chunks;
  chunks.reserve(2);
  for (int64_t begin = 0; begin < kFinalAnchorCount;
       begin += kFirstBatchCount) {
    const auto count =
        std::min<int64_t>(kFirstBatchCount, kFinalAnchorCount - begin);
    const auto context =
        representation_context_cpu.narrow(0, begin, count).to(torch::kCUDA);
    require(context.scalar_type() == torch::kFloat32 && context.is_cuda(),
            "final live context is not CUDA float32");
    const auto mask = torch::ones(
        {count, kNodeCount, kChannelCount},
        torch::TensorOptions().dtype(torch::kBool).device(torch::kCUDA));
    chunks.push_back(model->direct_edge_context_features(context, mask));
  }
  auto readout = torch::cat(chunks, 0).contiguous();
  require(chunks.size() == 2 && chunks[0].size(0) == kFirstBatchCount &&
              chunks[1].size(0) == kSecondBatchCount &&
              readout.sizes() ==
                  torch::IntArrayRef({kFinalAnchorCount, kEdgeCount,
                                      kChannelCount, kReadoutWidth}) &&
              readout.scalar_type() == torch::kFloat32 && readout.is_cuda() &&
              torch::isfinite(readout).all().item<bool>(),
          "recomputed final MDN readout surface is invalid");
  return readout;
}

torch::Tensor make_suffix_perturbation(const torch::Tensor &input) {
  auto result = input.clone();
  auto suffix =
      result.narrow(3, kDynamicFeatureDim, kReadoutWidth - kDynamicFeatureDim);
  for (int64_t feature = 0; feature < suffix.size(3); ++feature) {
    suffix.select(3, feature)
        .fill_((feature % 2 == 0 ? 1.0f : -1.0f) *
               (12345.0f + static_cast<float>(feature)));
  }
  return result;
}

double population_std(double sum, double square_sum, int64_t count) {
  require(count > 0, "cannot compute population standard deviation");
  const auto n = static_cast<double>(count);
  return std::sqrt(std::max(square_sum / n - std::pow(sum / n, 2.0), 0.0));
}

DescriptiveMetric compute_descriptive(const torch::Tensor &prediction_input,
                                      const torch::Tensor &target_input) {
  const auto prediction =
      prediction_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto target =
      target_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  require(prediction.sizes() == target.sizes() && prediction.dim() == 3,
          "descriptive prediction/target shape mismatch");
  const auto p = prediction.accessor<float, 3>();
  const auto t = target.accessor<float, 3>();
  const auto sign = [](double value) {
    return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
  };
  DescriptiveMetric result;
  double prediction_sum = 0.0;
  double prediction_square_sum = 0.0;
  double target_sum = 0.0;
  double target_square_sum = 0.0;
  for (int64_t anchor = 0; anchor < prediction.size(0); ++anchor) {
    for (int64_t channel = 0; channel < prediction.size(2); ++channel) {
      int64_t predicted_best = 0;
      int64_t target_best = 0;
      for (int64_t edge = 0; edge < prediction.size(1); ++edge) {
        const double predicted = p[anchor][edge][channel];
        const double realized = t[anchor][edge][channel];
        result.signed_error += predicted - realized;
        prediction_sum += predicted;
        prediction_square_sum += predicted * predicted;
        target_sum += realized;
        target_square_sum += realized * realized;
        if (edge > 0 && predicted > p[anchor][predicted_best][channel]) {
          predicted_best = edge;
        }
        if (edge > 0 && realized > t[anchor][target_best][channel]) {
          target_best = edge;
        }
        if (std::fabs(realized) > kMarginEpsilon) {
          ++result.margin_valid_count;
          if (sign(predicted) == sign(realized)) {
            ++result.margin_direction_correct;
          }
        } else {
          ++result.near_zero_target_count;
        }
      }
      ++result.best_asset_valid_count;
      if (predicted_best == target_best) {
        ++result.best_asset_agreement_count;
      }
      for (int64_t lhs = 0; lhs < prediction.size(1); ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < prediction.size(1); ++rhs) {
          const double realized_delta =
              t[anchor][lhs][channel] - t[anchor][rhs][channel];
          if (std::fabs(realized_delta) > kMarginEpsilon) {
            ++result.rank_margin_valid_count;
            const double predicted_delta =
                p[anchor][lhs][channel] - p[anchor][rhs][channel];
            if (sign(predicted_delta) == sign(realized_delta)) {
              ++result.rank_margin_correct;
            }
          }
        }
      }
    }
  }
  const auto count = prediction.numel();
  result.signed_error /= static_cast<double>(count);
  result.prediction_population_std =
      population_std(prediction_sum, prediction_square_sum, count);
  result.target_population_std =
      population_std(target_sum, target_square_sum, count);
  result.prediction_to_target_std_ratio =
      result.target_population_std > 0.0
          ? result.prediction_population_std / result.target_population_std
          : std::numeric_limits<double>::max();
  require(result.best_asset_valid_count == kExpectedBestAssetCount,
          "unexpected final best-asset domain");
  return result;
}

bool metric_finite(const Metric &metric) {
  return metric.count > 0 && metric.pair_count >= 0 &&
         std::isfinite(metric.abs_error) && std::isfinite(metric.sq_error) &&
         std::isfinite(metric.prediction_sum) &&
         std::isfinite(metric.prediction_sq_sum) &&
         std::isfinite(metric.target_sum) &&
         std::isfinite(metric.target_sq_sum) &&
         std::isfinite(metric.cross_sum) &&
         std::isfinite(metric_rmse(metric)) &&
         std::isfinite(metric_direction(metric)) &&
         std::isfinite(metric_rank(metric)) &&
         std::isfinite(metric_correlation(metric));
}

bool objective_finite(const DirectObjectiveSummary &summary) {
  return std::isfinite(summary.total) && std::isfinite(summary.regression) &&
         std::isfinite(summary.direction) && std::isfinite(summary.rank);
}

bool descriptive_finite(const DescriptiveMetric &metric) {
  return std::isfinite(metric.signed_error) &&
         std::isfinite(metric.prediction_population_std) &&
         std::isfinite(metric.target_population_std) &&
         std::isfinite(metric.prediction_to_target_std_ratio);
}

EvaluationPass run_evaluation_pass(const Options &options,
                                   const RepresentationSurface &representation,
                                   const MdnSurface &mdn_surface) {
  reset_deterministic_seed();
  auto model = make_production_model(torch::Device(torch::kCUDA));
  load_production_checkpoint(options.mdn_checkpoint, model);

  torch::NoGradGuard no_grad;
  EvaluationPass result;
  result.model_eval = !model->is_training();
  result.checkpoint_head_eval = !model->direct_edge_head->is_training();
  const auto recomputed = recompute_readout(model, representation.context);
  const auto cached = mdn_surface.readout_features.to(torch::kCUDA);
  require(cached.scalar_type() == torch::kFloat32 && cached.is_cuda(),
          "final cached readout is not CUDA float32");
  result.dynamic_prefix =
      compare_tensors(recomputed.narrow(3, 0, kDynamicFeatureDim),
                      cached.narrow(3, 0, kDynamicFeatureDim));
  require(result.dynamic_prefix.torch_equal &&
              result.dynamic_prefix.raw_bytes_equal &&
              result.dynamic_prefix.maximum_abs_delta == 0.0,
          "reconstructed final dynamic prefix is not bit-exact");
  result.full_features = compare_tensors(recomputed, cached);

  const auto checkpoint_prediction =
      model->direct_edge_head->forward_readout_features(cached);
  auto conditioned =
      production_mdn::load_per_edge_conditioned_affine_return_head(
          options.v2_artifact);
  validate_v2_metadata(conditioned.metadata);
  conditioned.head->to(torch::kCUDA);
  conditioned.head->eval();
  conditioned.head->validate_registered_state();
  result.conditioned_head_eval = !conditioned.head->is_training();
  const auto conditioned_parameters = conditioned.head->parameters();
  result.conditioned_parameters_frozen =
      std::all_of(conditioned_parameters.begin(), conditioned_parameters.end(),
                  [](const torch::Tensor &parameter) {
                    return !parameter.requires_grad();
                  });
  const auto conditioned_prediction =
      conditioned.head->forward_readout_features(cached);
  const auto suffix_perturbed = make_suffix_perturbation(cached);
  const auto suffix_prefix_comparison =
      compare_tensors(cached.narrow(3, 0, kDynamicFeatureDim),
                      suffix_perturbed.narrow(3, 0, kDynamicFeatureDim));
  const auto suffix_full_input_comparison =
      compare_tensors(cached, suffix_perturbed);
  result.suffix_input_genuinely_changed =
      suffix_prefix_comparison.raw_bytes_equal &&
      !suffix_full_input_comparison.raw_bytes_equal;
  require(result.suffix_input_genuinely_changed,
          "conditioned suffix perturbation did not genuinely change input");
  const auto suffix_prediction =
      conditioned.head->forward_readout_features(suffix_perturbed);
  require(checkpoint_prediction.sizes() == mdn_surface.target.sizes() &&
              conditioned_prediction.sizes() == mdn_surface.target.sizes() &&
              suffix_prediction.sizes() == mdn_surface.target.sizes() &&
              checkpoint_prediction.scalar_type() == torch::kFloat32 &&
              conditioned_prediction.scalar_type() == torch::kFloat32 &&
              checkpoint_prediction.is_cuda() &&
              conditioned_prediction.is_cuda() &&
              torch::isfinite(checkpoint_prediction).all().item<bool>() &&
              torch::isfinite(conditioned_prediction).all().item<bool>() &&
              torch::isfinite(suffix_prediction).all().item<bool>(),
          "final prediction surface is invalid");
  result.suffix_ignored =
      compare_tensors(conditioned_prediction, suffix_prediction);
  require(result.suffix_ignored.torch_equal &&
              result.suffix_ignored.raw_bytes_equal &&
              result.suffix_ignored.maximum_abs_delta == 0.0,
          "conditioned final prediction depends on the ignored suffix");
  require(result.model_eval && result.checkpoint_head_eval &&
              result.conditioned_head_eval &&
              result.conditioned_parameters_frozen,
          "a final-holdout head is not frozen in eval mode");

  result.checkpoint_prediction =
      checkpoint_prediction.detach().to(torch::kCPU).contiguous();
  result.conditioned_prediction =
      conditioned_prediction.detach().to(torch::kCPU).contiguous();
  result.suffix_prediction =
      suffix_prediction.detach().to(torch::kCPU).contiguous();
  result.checkpoint_metric =
      evaluate_prediction(result.checkpoint_prediction, mdn_surface.target);
  result.conditioned_metric =
      evaluate_prediction(result.conditioned_prediction, mdn_surface.target);
  require_metric_counts(result.checkpoint_metric, kExpectedRows, kExpectedRows,
                        "final checkpoint head");
  require_metric_counts(result.conditioned_metric, kExpectedRows, kExpectedRows,
                        "final conditioned head");
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    result.checkpoint_per_edge[static_cast<std::size_t>(edge)] =
        evaluate_prediction(result.checkpoint_prediction.narrow(1, edge, 1),
                            mdn_surface.target.narrow(1, edge, 1));
    result.conditioned_per_edge[static_cast<std::size_t>(edge)] =
        evaluate_prediction(result.conditioned_prediction.narrow(1, edge, 1),
                            mdn_surface.target.narrow(1, edge, 1));
    require(
        result.checkpoint_per_edge[static_cast<std::size_t>(edge)].count ==
                kExpectedGroupRows &&
            result.conditioned_per_edge[static_cast<std::size_t>(edge)].count ==
                kExpectedGroupRows,
        "unexpected final per-edge metric domain");
  }
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    result.checkpoint_per_channel[static_cast<std::size_t>(channel)] =
        evaluate_prediction(result.checkpoint_prediction.narrow(2, channel, 1),
                            mdn_surface.target.narrow(2, channel, 1));
    result.conditioned_per_channel[static_cast<std::size_t>(channel)] =
        evaluate_prediction(result.conditioned_prediction.narrow(2, channel, 1),
                            mdn_surface.target.narrow(2, channel, 1));
    require(
        result.checkpoint_per_channel[static_cast<std::size_t>(channel)]
                    .count == kExpectedGroupRows &&
            result.conditioned_per_channel[static_cast<std::size_t>(channel)]
                    .count == kExpectedGroupRows,
        "unexpected final per-channel metric domain");
  }
  result.checkpoint_objective = compute_production_direct_objective(
      checkpoint_prediction, mdn_surface.target);
  result.conditioned_objective = compute_production_direct_objective(
      conditioned_prediction, mdn_surface.target);
  require(
      result.checkpoint_objective.valid_count == kExpectedRows &&
          result.checkpoint_objective.pairwise_valid_count == kExpectedRows &&
          result.conditioned_objective.valid_count == kExpectedRows &&
          result.conditioned_objective.pairwise_valid_count == kExpectedRows,
      "final production objective domain mismatch");
  result.checkpoint_descriptive =
      compute_descriptive(result.checkpoint_prediction, mdn_surface.target);
  result.conditioned_descriptive =
      compute_descriptive(result.conditioned_prediction, mdn_surface.target);
  result.checkpoint_digest = prediction_digest(result.checkpoint_prediction);
  result.conditioned_digest = prediction_digest(result.conditioned_prediction);
  result.suffix_digest = prediction_digest(result.suffix_prediction);

  require(metric_finite(result.checkpoint_metric) &&
              metric_finite(result.conditioned_metric) &&
              objective_finite(result.checkpoint_objective) &&
              objective_finite(result.conditioned_objective) &&
              descriptive_finite(result.checkpoint_descriptive) &&
              descriptive_finite(result.conditioned_descriptive),
          "final required metric is non-finite");
  for (const auto &metric : result.checkpoint_per_edge) {
    require(metric_finite(metric), "non-finite checkpoint per-edge metric");
  }
  for (const auto &metric : result.conditioned_per_edge) {
    require(metric_finite(metric), "non-finite conditioned per-edge metric");
  }
  for (const auto &metric : result.checkpoint_per_channel) {
    require(metric_finite(metric), "non-finite checkpoint per-channel metric");
  }
  for (const auto &metric : result.conditioned_per_channel) {
    require(metric_finite(metric), "non-finite conditioned per-channel metric");
  }
  return result;
}

double metric_mae(const Metric &metric) {
  return metric.abs_error / static_cast<double>(metric.count);
}

double metric_prediction_std(const Metric &metric) {
  return population_std(metric.prediction_sum, metric.prediction_sq_sum,
                        metric.count);
}

void emit_final_metric(std::ostream &output, const std::string &prefix,
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
                    const DirectObjectiveSummary &summary) {
  output << prefix << ".total=" << summary.total << '\n';
  output << prefix << ".regression=" << summary.regression << '\n';
  output << prefix << ".direction=" << summary.direction << '\n';
  output << prefix << ".rank=" << summary.rank << '\n';
  output << prefix << ".valid_count=" << summary.valid_count << '\n';
  output << prefix << ".pairwise_valid_count=" << summary.pairwise_valid_count
         << '\n';
}

void emit_descriptive(std::ostream &output, const std::string &prefix,
                      const DescriptiveMetric &metric) {
  output << prefix << ".signed_error=" << metric.signed_error << '\n';
  output << prefix << ".target_population_std=" << metric.target_population_std
         << '\n';
  output << prefix << ".prediction_to_target_std_ratio="
         << metric.prediction_to_target_std_ratio << '\n';
  output << prefix
         << ".best_asset_valid_count=" << metric.best_asset_valid_count << '\n';
  output << prefix << ".best_asset_agreement="
         << static_cast<double>(metric.best_asset_agreement_count) /
                static_cast<double>(metric.best_asset_valid_count)
         << '\n';
  output << prefix << ".margin_epsilon=" << kMarginEpsilon << '\n';
  output << prefix << ".margin_valid_count=" << metric.margin_valid_count
         << '\n';
  output << prefix << ".margin_directional_accuracy="
         << (metric.margin_valid_count > 0
                 ? static_cast<double>(metric.margin_direction_correct) /
                       static_cast<double>(metric.margin_valid_count)
                 : 0.0)
         << '\n';
  output << prefix
         << ".near_zero_target_count=" << metric.near_zero_target_count << '\n';
  output << prefix
         << ".rank_margin_valid_count=" << metric.rank_margin_valid_count
         << '\n';
  output << prefix << ".rank_margin_accuracy="
         << (metric.rank_margin_valid_count > 0
                 ? static_cast<double>(metric.rank_margin_correct) /
                       static_cast<double>(metric.rank_margin_valid_count)
                 : 0.0)
         << '\n';
}

std::string serialize_evaluation(const EvaluationPass &result) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::scientific << std::setprecision(17);
  output << "checkpoint.digest=" << result.checkpoint_digest << '\n';
  output << "conditioned.digest=" << result.conditioned_digest << '\n';
  output << "suffix.digest=" << result.suffix_digest << '\n';
  output << "dynamic_prefix.torch_equal="
         << bool_text(result.dynamic_prefix.torch_equal) << '\n';
  output << "dynamic_prefix.raw_bytes_equal="
         << bool_text(result.dynamic_prefix.raw_bytes_equal) << '\n';
  output << "dynamic_prefix.maximum_abs_delta="
         << result.dynamic_prefix.maximum_abs_delta << '\n';
  output << "full_features.torch_equal="
         << bool_text(result.full_features.torch_equal) << '\n';
  output << "full_features.raw_bytes_equal="
         << bool_text(result.full_features.raw_bytes_equal) << '\n';
  output << "full_features.maximum_abs_delta="
         << result.full_features.maximum_abs_delta << '\n';
  output << "suffix_ignored.torch_equal="
         << bool_text(result.suffix_ignored.torch_equal) << '\n';
  output << "suffix_ignored.raw_bytes_equal="
         << bool_text(result.suffix_ignored.raw_bytes_equal) << '\n';
  output << "suffix_ignored.maximum_abs_delta="
         << result.suffix_ignored.maximum_abs_delta << '\n';
  output << "suffix_input_genuinely_changed="
         << bool_text(result.suffix_input_genuinely_changed) << '\n';
  emit_final_metric(output, "checkpoint", result.checkpoint_metric);
  emit_final_metric(output, "conditioned", result.conditioned_metric);
  emit_objective(output, "objective.checkpoint", result.checkpoint_objective);
  emit_objective(output, "objective.conditioned", result.conditioned_objective);
  emit_descriptive(output, "descriptive.checkpoint",
                   result.checkpoint_descriptive);
  emit_descriptive(output, "descriptive.conditioned",
                   result.conditioned_descriptive);
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    emit_final_metric(
        output, "checkpoint.per_edge." + std::to_string(edge),
        result.checkpoint_per_edge[static_cast<std::size_t>(edge)]);
    emit_final_metric(
        output, "conditioned.per_edge." + std::to_string(edge),
        result.conditioned_per_edge[static_cast<std::size_t>(edge)]);
  }
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_final_metric(
        output, "checkpoint.per_channel." + std::to_string(channel),
        result.checkpoint_per_channel[static_cast<std::size_t>(channel)]);
    emit_final_metric(
        output, "conditioned.per_channel." + std::to_string(channel),
        result.conditioned_per_channel[static_cast<std::size_t>(channel)]);
  }
  return output.str();
}

std::string interpretation_outcome(bool absolute_pass, bool improvement_pass,
                                   bool direction_pass, bool rank_pass,
                                   bool correlation_pass, bool rmse_pass,
                                   const Metric &checkpoint,
                                   const Metric &conditioned) {
  if (absolute_pass && improvement_pass) {
    return "helper_validity_and_seven_scientific_gates_pass_"
           "runner_closure_required";
  }
  if (absolute_pass && !improvement_pass) {
    return "absolute_gates_pass_improvement_gates_fail";
  }
  if (!absolute_pass && improvement_pass) {
    return "improvement_gates_pass_absolute_gates_fail";
  }
  if (direction_pass && rank_pass && (!correlation_pass || !rmse_pass)) {
    return "direction_rank_pass_calibration_or_correlation_fail";
  }
  if (correlation_pass && rmse_pass && (!direction_pass || !rank_pass)) {
    return "rmse_correlation_pass_direction_or_rank_fail";
  }
  if (metric_direction(conditioned) <= metric_direction(checkpoint) &&
      metric_rank(conditioned) <= metric_rank(checkpoint) &&
      metric_rmse(conditioned) >= metric_rmse(checkpoint)) {
    return "candidate_equals_or_loses_to_checkpoint";
  }
  if (!absolute_pass && !improvement_pass) {
    return "both_absolute_and_relative_gate_sets_fail";
  }
  return "mixed_scientific_gate_failure_no_full_acceptance";
}

int run(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_options(argc, argv);
    require(options.schema_id == kSchema, "final-holdout schema mismatch");
    require(!std::filesystem::exists(options.output) &&
                !std::filesystem::exists(options.output.string() + ".tmp"),
            "refusing to replace a final-holdout output");

    const std::array<
        std::tuple<std::string, std::filesystem::path, std::string>, 7>
        inputs{{
            {"representation_probe", options.representation_probe,
             options.representation_probe_sha256},
            {"mdn_probe", options.mdn_probe, options.mdn_probe_sha256},
            {"capture_report", options.capture_report,
             options.capture_report_sha256},
            {"job_manifest", options.job_manifest, options.job_manifest_sha256},
            {"representation_checkpoint", options.representation_checkpoint,
             std::string(kRepresentationCheckpointSha)},
            {"mdn_checkpoint", options.mdn_checkpoint,
             std::string(kMdnCheckpointSha)},
            {"v2_artifact", options.v2_artifact, std::string(kV2ArtifactSha)},
        }};
    std::map<std::string, std::string> input_hashes_before;
    for (const auto &[name, path, expected] : inputs) {
      require(std::filesystem::is_regular_file(path),
              "missing final-holdout input: " + name);
      const auto actual = sha256_file(path);
      require(actual == expected,
              "final-holdout input identity mismatch: " + name);
      input_hashes_before.emplace(name, actual);
    }

    const auto capture_report =
        read_kv(options.capture_report, "capture report");
    const auto job_manifest = read_kv(options.job_manifest, "job manifest");
    validate_capture_provenance(capture_report, job_manifest, options);
    const auto representation =
        read_representation_probe(options.representation_probe);
    const auto mdn_surface = read_mdn_probe(options.mdn_probe);
    require(representation.anchor_keys == mdn_surface.anchor_keys,
            "final representation and MDN probes disagree on anchor keys");
    const auto target_comparison =
        compare_tensors(representation.target, mdn_surface.target);
    require(target_comparison.torch_equal &&
                target_comparison.raw_bytes_equal &&
                target_comparison.maximum_abs_delta == 0.0,
            "final representation and MDN probe targets are not bit-exact");

    const auto runtime = configure_deterministic_runtime();
    const auto primary =
        run_evaluation_pass(options, representation, mdn_surface);
    const auto primary_payload = serialize_evaluation(primary);
    const auto replay =
        run_evaluation_pass(options, representation, mdn_surface);
    const auto replay_payload = serialize_evaluation(replay);
    const auto checkpoint_replay = compare_tensors(
        primary.checkpoint_prediction, replay.checkpoint_prediction);
    const auto conditioned_replay = compare_tensors(
        primary.conditioned_prediction, replay.conditioned_prediction);
    const auto suffix_replay =
        compare_tensors(primary.suffix_prediction, replay.suffix_prediction);
    require(primary_payload == replay_payload &&
                checkpoint_replay.raw_bytes_equal &&
                conditioned_replay.raw_bytes_equal &&
                suffix_replay.raw_bytes_equal,
            "internal final-holdout primary/replay drift");

    for (const auto &[name, path, expected] : inputs) {
      const auto after = sha256_file(path);
      require(after == expected && input_hashes_before.at(name) == after,
              "final-holdout input changed during evaluation: " + name);
    }

    const auto direction_improvement =
        metric_direction(primary.conditioned_metric) -
        metric_direction(primary.checkpoint_metric);
    const auto rank_improvement = metric_rank(primary.conditioned_metric) -
                                  metric_rank(primary.checkpoint_metric);
    const auto checkpoint_rmse = metric_rmse(primary.checkpoint_metric);
    const auto checkpoint_rmse_positive =
        checkpoint_rmse > 0.0 && std::isfinite(checkpoint_rmse);
    const auto rmse_ratio =
        checkpoint_rmse_positive
            ? metric_rmse(primary.conditioned_metric) / checkpoint_rmse
            : std::numeric_limits<double>::max();
    require(std::isfinite(direction_improvement) &&
                std::isfinite(rank_improvement) && std::isfinite(rmse_ratio),
            "a final scientific comparison is non-finite");

    const bool direction_gate =
        metric_direction(primary.conditioned_metric) >= kMinimumDirection;
    const bool rank_gate =
        metric_rank(primary.conditioned_metric) >= kMinimumRank;
    const bool correlation_gate =
        metric_correlation(primary.conditioned_metric) >= kMinimumCorrelation;
    const bool rmse_gate =
        metric_rmse(primary.conditioned_metric) <= kMaximumRmse;
    const bool direction_improvement_gate =
        direction_improvement >= kMinimumDirectionImprovement;
    const bool rank_improvement_gate =
        rank_improvement >= kMinimumRankImprovement;
    const bool rmse_improvement_gate =
        checkpoint_rmse_positive && rmse_ratio <= kMaximumRmseRatio;
    const bool absolute_gate =
        direction_gate && rank_gate && correlation_gate && rmse_gate;
    const bool improvement_gate = direction_improvement_gate &&
                                  rank_improvement_gate &&
                                  rmse_improvement_gate;
    const bool scientific_gate = absolute_gate && improvement_gate;
    const auto interpretation = interpretation_outcome(
        absolute_gate, improvement_gate, direction_gate, rank_gate,
        correlation_gate, rmse_gate, primary.checkpoint_metric,
        primary.conditioned_metric);

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::scientific << std::setprecision(17);
    output << "schema_id=" << kSchema << '\n';
    output << "status=complete\n";
    output << "result_role=sealed_final_holdout_no_refit_evaluation\n";
    output << "scientific_result_published_regardless_of_gate_outcome=true\n";
    output << "candidate.count=1\n";
    output << "candidate.artifact_family=wikimyei.inference.expected_value.mdn."
              "per_edge_conditioned_affine_return_head.v2\n";
    output << "candidate.semantic_tensor_digest=" << kSemanticDigest << '\n';
    output << "candidate.conditioning_alpha=1.00000000000000004e-10\n";
    output << "candidate.fit_range=[0,554)\n";
    output << "candidate.purge_range=[554,584)\n";
    output << "candidate.selection_range=[584,730)\n";
    output << "candidate.maximum_fit_or_selection_anchor=729\n";
    output << "candidate.run_only=true\n";
    output << "candidate.policy_eligible=false\n";
    output << "control.kind=frozen_production_checkpoint_direct_head\n";
    output << "boundary.final_holdout=[1088,1170)\n";
    output << "boundary.minimum_anchor_loaded=1088\n";
    output << "boundary.maximum_anchor_loaded=1169\n";
    output << "data.anchor_count=" << kFinalAnchorCount << '\n';
    output << "data.batch_count=2\n";
    output << "data.batch_sizes=64,18\n";
    output << "data.skipped_batches=0\n";
    output << "data.row_count=" << mdn_surface.row_count << '\n';
    output << "data.representation_shape=[82,4,3,32]\n";
    output << "data.feature_shape=[82,3,3,400]\n";
    output << "data.target_shape=[82,3,3]\n";
    output << "data.target_torch_equal_across_probes=true\n";
    output << "data.target_raw_bytes_equal_across_probes=true\n";
    output << "data.representation_base_quote_difference.maximum_abs_delta="
           << representation.difference_max_abs_delta << '\n';
    output << "data.representation_quote_cross_edge.maximum_abs_delta="
           << representation.quote_max_abs_delta << '\n';
    output << "data.dynamic_prefix_torch_equal="
           << bool_text(primary.dynamic_prefix.torch_equal) << '\n';
    output << "data.dynamic_prefix_raw_bytes_equal="
           << bool_text(primary.dynamic_prefix.raw_bytes_equal) << '\n';
    output << "data.dynamic_prefix_maximum_abs_delta="
           << primary.dynamic_prefix.maximum_abs_delta << '\n';
    output << "data.full_recomputed_to_cached_torch_equal="
           << bool_text(primary.full_features.torch_equal) << '\n';
    output << "data.full_recomputed_to_cached_raw_bytes_equal="
           << bool_text(primary.full_features.raw_bytes_equal) << '\n';
    output << "data.full_recomputed_to_cached_maximum_abs_delta="
           << primary.full_features.maximum_abs_delta << '\n';
    output << "data.full_recomputed_to_cached_descriptive_tolerance="
           << kFullFeatureDescriptiveTolerance << '\n';
    output << "input.representation_probe.sha256="
           << options.representation_probe_sha256 << '\n';
    output << "input.mdn_probe.sha256=" << options.mdn_probe_sha256 << '\n';
    output << "input.capture_report.sha256=" << options.capture_report_sha256
           << '\n';
    output << "input.job_manifest.sha256=" << options.job_manifest_sha256
           << '\n';
    output << "input.representation_checkpoint.sha256="
           << kRepresentationCheckpointSha << '\n';
    output << "input.mdn_checkpoint.sha256=" << kMdnCheckpointSha << '\n';
    output << "input.v2_artifact.sha256=" << kV2ArtifactSha << '\n';
    output << "input.hashes_identical_before_after=true\n";
    output << "execution.production_checkpoint_loader_executed=true\n";
    output << "execution.checkpoint_expected_identity_nonnull=true\n";
    output << "execution.checkpoint_full_expected_identity_populated=true\n";
    output << "execution.mdn_direct_feature_forward_executed=true\n";
    output << "execution.mdn_distribution_forward_executed=false\n";
    output << "execution.checkpoint_head_forward_executed=true\n";
    output << "execution.conditioned_head_forward_executed=true\n";
    output << "execution.training_performed=false\n";
    output << "execution.refit_performed=false\n";
    output << "execution.optimizer_steps=0\n";
    output << "execution.checkpoint_written=false\n";
    output << "execution.policy_path_used=false\n";
    output << "execution.no_grad=true\n";
    output << "execution.model_eval=" << bool_text(primary.model_eval) << '\n';
    output << "execution.checkpoint_head_eval="
           << bool_text(primary.checkpoint_head_eval) << '\n';
    output << "execution.conditioned_head_eval="
           << bool_text(primary.conditioned_head_eval) << '\n';
    output << "execution.conditioned_parameters_frozen="
           << bool_text(primary.conditioned_parameters_frozen) << '\n';
    output << "execution.device=cuda\n";
    output << "execution.live_input_dtype=float32\n";
    output << "runtime.torch_version=" << TORCH_VERSION << '\n';
    output << "runtime.cuda_runtime_version=" << runtime.cuda_runtime_version
           << '\n';
    output << "runtime.cuda_driver_version=" << runtime.cuda_driver_version
           << '\n';
    output << "prediction.checkpoint.digest=" << primary.checkpoint_digest
           << '\n';
    output << "prediction.conditioned.digest=" << primary.conditioned_digest
           << '\n';
    output << "prediction.conditioned_suffix_perturbed.digest="
           << primary.suffix_digest << '\n';
    output << "parity.conditioned_suffix_ignored.torch_equal="
           << bool_text(primary.suffix_ignored.torch_equal) << '\n';
    output << "parity.conditioned_suffix_ignored.raw_bytes_equal="
           << bool_text(primary.suffix_ignored.raw_bytes_equal) << '\n';
    output << "parity.conditioned_suffix_ignored.maximum_abs_delta="
           << primary.suffix_ignored.maximum_abs_delta << '\n';
    output << "parity.suffix_input_genuinely_changed="
           << bool_text(primary.suffix_input_genuinely_changed) << '\n';
    output << "replay.internal_evaluation_payload.sha256="
           << cuwacunu::piaabo::digest::sha256_hex(primary_payload) << '\n';
    output << "replay.internal_payload_byte_identical=true\n";
    output << "replay.checkpoint_prediction_torch_equal=true\n";
    output << "replay.conditioned_prediction_torch_equal=true\n";
    output << "replay.suffix_prediction_torch_equal=true\n";
    output << "validity.anchor_range.pass=true\n";
    output << "validity.anchor_count.pass=true\n";
    output << "validity.batch_shape.pass=true\n";
    output << "validity.zero_skipped_batches.pass=true\n";
    output << "validity.target_and_rank_counts.pass=true\n";
    output << "validity.per_edge_and_channel_counts.pass=true\n";
    output << "validity.dynamic_prefix_bit_exact.pass=true\n";
    output << "validity.conditioned_suffix_ignored.pass=true\n";
    output << "validity.production_checkpoint_identity.pass=true\n";
    output << "validity.eval_no_grad_cuda_float32.pass=true\n";
    output << "validity.no_training_refit_optimizer.pass=true\n";
    output << "validity.no_checkpoint_or_policy.pass=true\n";
    output << "validity.input_hash_stability.pass=true\n";
    output << "validity.predictions_and_metrics_finite.pass=true\n";
    output << "validity.internal_primary_replay.pass=true\n";
    output << "validity.helper_level.pass=true\n";
    output << "validity.external_runner_closure_manifest.required=true\n";
    output << "validity.external_empty_success_logs.required=true\n";
    output
        << "validity.external_primary_replay_report_identity.required=true\n";
    emit_final_metric(output, "final.checkpoint", primary.checkpoint_metric);
    emit_final_metric(output, "final.conditioned", primary.conditioned_metric);
    emit_objective(output, "production_direct_objective.checkpoint",
                   primary.checkpoint_objective);
    emit_objective(output, "production_direct_objective.conditioned",
                   primary.conditioned_objective);
    emit_descriptive(output, "final.checkpoint.descriptive",
                     primary.checkpoint_descriptive);
    emit_descriptive(output, "final.conditioned.descriptive",
                     primary.conditioned_descriptive);
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      emit_final_metric(
          output, "final.checkpoint.per_edge." + std::to_string(edge),
          primary.checkpoint_per_edge[static_cast<std::size_t>(edge)]);
      emit_final_metric(
          output, "final.conditioned.per_edge." + std::to_string(edge),
          primary.conditioned_per_edge[static_cast<std::size_t>(edge)]);
    }
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      emit_final_metric(
          output, "final.checkpoint.per_channel." + std::to_string(channel),
          primary.checkpoint_per_channel[static_cast<std::size_t>(channel)]);
      emit_final_metric(
          output, "final.conditioned.per_channel." + std::to_string(channel),
          primary.conditioned_per_channel[static_cast<std::size_t>(channel)]);
    }
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
    output << "scientific_gate.absolute.pass=" << bool_text(absolute_gate)
           << '\n';
    output << "scientific_gate.improvement.pass=" << bool_text(improvement_gate)
           << '\n';
    output << "scientific_gate.pass=" << bool_text(scientific_gate) << '\n';
    output << "interpretation.outcome=" << interpretation << '\n';
    output << "interpretation.full_acceptance=" << bool_text(scientific_gate)
           << '\n';
    output << "interpretation.requires_runner_closure_pass=true\n";
    output << "scope.validates_mdn_distribution_output=false\n";
    output << "scope.validates_policy=false\n";
    output << "scope.validates_forecast_to_action_path=false\n";
    output << "scope.validates_other_synthetic_seeds=false\n";
    output << "scope.validates_real_market_data=false\n";
    output << "conclusion.final_interval_consumed=true\n";
    output << "conclusion.candidate_may_not_be_tuned_on_this_interval=true\n";
    write_probe_atomic(options.output, output.str());
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "final-holdout helper failure: "
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "final-holdout helper failure: " << error.what() << '\n';
  }
  return 1;
}

} // namespace mdn_final_holdout

int main(int argc, char **argv) { return mdn_final_holdout::run(argc, argv); }
