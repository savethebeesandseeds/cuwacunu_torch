#define main mdn_frozen_affine_objective_ladder_embedded_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>
#include <functional>
#include <unordered_map>

#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace shadow_readout {

namespace production_mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

constexpr std::string_view kSchema =
    "synthetic_mdn_frozen_conditioned_affine_shadow_readout_v1";
constexpr std::string_view kExpectedDevelopmentSha =
    "ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851";
constexpr std::string_view kExpectedCaptureReportSha =
    "a584e1326c264429eb4c01c24e1794c8d6f4dbe54bc5db1a627cb42ffe211441";
constexpr std::string_view kExpectedRepresentationTrainingReportSha =
    "a1ef97f5850137e42d7e5623e3c44d0fffe4d55bf796429bdb289859183ff4f9";
constexpr std::string_view kExpectedMdnTrainingReportSha =
    "59db5a98b7f74ce343aab755e7a3d102a48dfdd0e8498028c5b89995ffb36b6f";
constexpr std::string_view kExpectedRepresentationCheckpointSha =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr std::string_view kExpectedMdnCheckpointSha =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr std::string_view kExpectedV2ArtifactSha =
    "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739";
constexpr std::string_view kExpectedDeploymentProbeSha =
    "00a382d060d20d7a8277d9f120fdc6578c572072264847157424c8a09207d3fb";
constexpr std::string_view kShadowExpectedGraphFingerprint = "d334e38b1887ae16";
constexpr std::string_view kExpectedArtifactSemanticDigest =
    "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38";
constexpr std::string_view kExpectedV2CpuPredictionDigest =
    "c2321514fa55fcc9e3fe17c3954105310b44f7c792f7a8e88f5252242cc52c7f";
constexpr std::string_view kExpectedV2CudaPredictionDigest =
    "1f4e343b99bd2ce7d64584100596fea54e474ec2a5fb718d6b873bbbe7c0c12e";
constexpr std::string_view kExpectedReadoutDigest =
    "1f0594e4aecfa1727f0231563b9e5032357a2d8dada9eeb30cd1b4a309b44362";
constexpr std::string_view kExpectedConditioningProbeSha =
    "1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1";
constexpr std::string_view kExpectedConditioningTransformSha =
    "3509385d6ed3c35678a511d29f441ea0ca9d77512a96b510aaa7a9757da0570c";

constexpr int64_t kShadowDevelopmentEnd = 730;
constexpr int64_t kShadowFitEnd = 554;
constexpr int64_t kShadowPurgeEnd = 584;
constexpr int64_t kShadowSealedHoldoutBegin = 1088;
constexpr int64_t kShadowSealedHoldoutEnd = 1170;
constexpr int64_t kShadowNodeCount = 4;
constexpr int64_t kShadowEdgeCount = 3;
constexpr int64_t kShadowChannelCount = 3;
constexpr int64_t kShadowContextDim = 32;
constexpr int64_t kShadowTargetDim = 9;
constexpr int64_t kShadowFutureHorizon = 1;
constexpr int64_t kShadowMixtureCount = 3;
constexpr int64_t kShadowHiddenWidth = 128;
constexpr int64_t kShadowResidualDepth = 2;
constexpr int64_t kShadowFeatureEmbeddingDim = 8;
constexpr int64_t kShadowChannelAdapterRank = 16;
constexpr int64_t kShadowIdentityEmbeddingDim = 16;
constexpr int64_t kShadowDirectAdapterHiddenDim = 128;
constexpr int64_t kShadowReadoutWidth = 400;
constexpr int64_t kShadowDynamicWidth = 384;
constexpr int64_t kShadowChunkSize = 37;
constexpr double kShadowSigmaMin = 1.0e-3;
constexpr double kShadowSigmaMax = 0.0;
constexpr double kShadowEps = 1.0e-6;
constexpr double kShadowDeploymentTolerance = 2.0e-5;
constexpr double kCaptureMetricTolerance = 2.0e-5;

const std::vector<int64_t> kShadowModelTargetCoords{0, 1, 2, 3, 4, 5, 6, 7, 8};
const std::vector<int64_t> kShadowLossTargetCoords{3};
const std::vector<std::string> kShadowNodeIds{"SYNUSD", "SYNALPHA", "SYNBETA",
                                              "SYNGAMMA"};
const std::vector<std::string> kShadowEdgeIds{"SYNALPHASYNUSD", "SYNBETASYNUSD",
                                              "SYNGAMMASYNUSD"};

struct ShadowOptions {
  std::filesystem::path development_input;
  std::filesystem::path capture_report;
  std::filesystem::path representation_training_report;
  std::filesystem::path mdn_training_report;
  std::filesystem::path representation_checkpoint;
  std::filesystem::path mdn_checkpoint;
  std::filesystem::path v2_artifact;
  std::filesystem::path deployment_probe;
  std::filesystem::path output;
  std::string schema_id;
};

using KvMap = std::map<std::string, std::string>;

struct TensorComparison {
  bool torch_equal{false};
  double maximum_abs_delta{std::numeric_limits<double>::infinity()};
  double mean_abs_delta{std::numeric_limits<double>::infinity()};
  double rmse_delta{std::numeric_limits<double>::infinity()};
  double threshold{kShadowDeploymentTolerance};
  bool pass{false};
};

struct LossSummary {
  double total{0.0};
  double regression{0.0};
  double direction{0.0};
  double rank{0.0};
  int64_t valid_count{0};
  int64_t pairwise_valid_count{0};
};

struct PredictionBundle {
  torch::Tensor cpu_full;
  torch::Tensor cpu_chunked;
  torch::Tensor cuda_full;
  torch::Tensor cuda_chunked;
};

std::filesystem::path normalized_cli_path(const std::filesystem::path &input) {
  std::error_code error;
  auto absolute = std::filesystem::absolute(input, error);
  if (error) {
    throw std::runtime_error("failed to normalize a shadow-readout CLI path");
  }
  return absolute.lexically_normal();
}

ShadowOptions parse_shadow_options(int argc, char **argv) {
  ShadowOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *name) -> std::string {
      if (index + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++index];
    };
    if (argument == "--development-input") {
      options.development_input = value("--development-input");
    } else if (argument == "--capture-report") {
      options.capture_report = value("--capture-report");
    } else if (argument == "--representation-training-report") {
      options.representation_training_report =
          value("--representation-training-report");
    } else if (argument == "--mdn-training-report") {
      options.mdn_training_report = value("--mdn-training-report");
    } else if (argument == "--representation-checkpoint") {
      options.representation_checkpoint = value("--representation-checkpoint");
    } else if (argument == "--mdn-checkpoint") {
      options.mdn_checkpoint = value("--mdn-checkpoint");
    } else if (argument == "--v2-artifact") {
      options.v2_artifact = value("--v2-artifact");
    } else if (argument == "--deployment-probe") {
      options.deployment_probe = value("--deployment-probe");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else if (argument == "--evaluation-input" ||
               argument == "--historical-input" ||
               argument == "--history-input" || argument == "--holdout-input" ||
               argument == "--policy-input" || argument == "--refit-input") {
      throw std::runtime_error(
          "forbidden non-development shadow-readout input: " + argument);
    } else {
      throw std::runtime_error("unknown shadow-readout argument: " + argument);
    }
  }
  if (options.development_input.empty() || options.capture_report.empty() ||
      options.representation_training_report.empty() ||
      options.mdn_training_report.empty() ||
      options.representation_checkpoint.empty() ||
      options.mdn_checkpoint.empty() || options.v2_artifact.empty() ||
      options.deployment_probe.empty() || options.output.empty() ||
      options.schema_id.empty()) {
    throw std::runtime_error(
        "--development-input --capture-report "
        "--representation-training-report --mdn-training-report "
        "--representation-checkpoint --mdn-checkpoint --v2-artifact "
        "--deployment-probe --output and --schema-id are required");
  }
  const std::set<std::filesystem::path> identities{
      normalized_cli_path(options.development_input),
      normalized_cli_path(options.capture_report),
      normalized_cli_path(options.representation_training_report),
      normalized_cli_path(options.mdn_training_report),
      normalized_cli_path(options.representation_checkpoint),
      normalized_cli_path(options.mdn_checkpoint),
      normalized_cli_path(options.v2_artifact),
      normalized_cli_path(options.deployment_probe),
      normalized_cli_path(options.output)};
  if (identities.size() != 9) {
    throw std::runtime_error(
        "all shadow-readout inputs and the output must be distinct");
  }
  return options;
}

KvMap read_exact_kv(const std::filesystem::path &path, const char *surface) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error(std::string("failed to open ") + surface);
  }
  KvMap values;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      throw std::runtime_error(std::string(surface) +
                               " contains an empty line");
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos || separator == 0) {
      throw std::runtime_error(std::string(surface) +
                               " contains a malformed assignment");
    }
    const auto key = line.substr(0, separator);
    const auto value = line.substr(separator + 1);
    if (!values.emplace(key, value).second) {
      throw std::runtime_error(std::string(surface) +
                               " contains a duplicate key: " + key);
    }
  }
  if (!input.good() && !input.eof()) {
    throw std::runtime_error(std::string("failed while reading ") + surface);
  }
  return values;
}

const std::string &required_value(const KvMap &values, const std::string &key,
                                  const char *surface) {
  const auto iterator = values.find(key);
  if (iterator == values.end() || iterator->second.empty()) {
    throw std::runtime_error(std::string(surface) + " is missing " + key);
  }
  return iterator->second;
}

void require_value(const KvMap &values, const std::string &key,
                   const std::string &expected, const char *surface) {
  const auto &actual = required_value(values, key, surface);
  if (actual != expected) {
    throw std::runtime_error(std::string(surface) + " has unexpected " + key);
  }
}

void validate_capture_report(const KvMap &report) {
  constexpr const char *surface = "capture report";
  const std::array<std::pair<std::string_view, std::string_view>, 45> exact{{
      {"training_id", "real_channel_mdn_train_core_v1"},
      {"component_assembly_id", "mdn_v1"},
      {"input_representation_assembly_id", "mtf_jepa_mae_vicreg_v1"},
      {"context_contract", "graph_order.channel_node_representation.v1"},
      {"output_contract", "graph_order.channel_node_future_distribution.v1"},
      {"graph_order_fingerprint", kShadowExpectedGraphFingerprint},
      {"channel_count", "3"},
      {"context_dim", "32"},
      {"future_horizon", "1"},
      {"context_mode", "channel_context_strict"},
      {"target_domain", "channel_node_future"},
      {"target_mask_policy", "per_target_feature_valid"},
      {"activity_target", "node_feature_support_mean"},
      {"mixture_count", "3"},
      {"hidden_width", "128"},
      {"residual_depth", "2"},
      {"feature_embedding_dim", "8"},
      {"channel_adapter_rank", "16"},
      {"mdn_architecture",
       "shared_slot_trunk.channel_adapter.shared_feature_head.direct_edge_"
       "readout.edge_embedding_per_edge.v4"},
      {"sigma_min", "0.001"},
      {"sigma_max", "0"},
      {"eps", "1e-06"},
      {"direct_edge_return_readout_enabled", "true"},
      {"direct_edge_return_readout_loss_weight", "100"},
      {"direct_edge_return_readout_direction_weight", "5"},
      {"direct_edge_return_readout_rank_weight", "5"},
      {"direct_edge_return_readout_huber_beta", "0.5"},
      {"direct_edge_return_readout_logit_scale", "1"},
      {"direct_edge_return_readout_target_scale", "36"},
      {"direct_edge_return_readout_identity_mode", "edge_embedding_per_edge"},
      {"direct_edge_return_readout_base_edge_count", "3"},
      {"direct_edge_return_readout_identity_embedding_dim", "16"},
      {"direct_edge_return_readout_adapter_hidden_dim", "128"},
      {"dtype", "float32"},
      {"device", "cuda"},
      {"seed", "31"},
      {"representation_checkpoint_loaded", "true"},
      {"mdn_checkpoint_loaded", "true"},
      {"requested_anchor_index_begin", "0"},
      {"requested_anchor_index_end", "730"},
      {"wave_streamed_anchor_count", "730"},
      {"target_coords", "0,1,2,3,4,5,6,7,8"},
      {"node_ids", "SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA"},
      {"edge_ids", "SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD"},
      {"mdn_edge_context_feature_probe_written", "true"},
  }};
  for (const auto &[key, value] : exact) {
    require_value(report, std::string(key), std::string(value), surface);
  }
  require_value(report, "mdn_edge_context_feature_probe_row_count", "6570",
                surface);
  require_value(report, "direct_edge_return_readout_valid_count", "6570",
                surface);
  require_value(report, "direct_edge_return_readout_pairwise_rank_valid_count",
                "6570", surface);
  require_value(report, "edge_return_projection_close_feature_index", "3",
                surface);
  require_value(report, "direct_edge_return_readout_close_feature_index", "3",
                surface);
  require_value(report, "edge_return_projection_quote_node_index", "0",
                surface);
  require_value(report, "direct_edge_return_readout_quote_node_index", "0",
                surface);
}

void validate_training_report(
    const KvMap &report, const std::filesystem::path &expected_report,
    int64_t expected_steps, const std::filesystem::path &expected_checkpoint,
    const std::optional<std::filesystem::path> &expected_representation,
    const char *surface) {
  require_value(report, "target_action", "train", surface);
  require_value(report, "requested_anchor_index_begin", "0", surface);
  require_value(report, "requested_anchor_index_end", "730", surface);
  const auto steps = std::to_string(expected_steps);
  require_value(report, "steps_attempted", steps, surface);
  require_value(report, "steps_completed", steps, surface);
  require_value(report, "optimizer_steps", steps, surface);
  require_value(report, "checkpoint_written", "true", surface);
  const auto report_self_path =
      normalized_cli_path(required_value(report, "report_path", surface));
  if (report_self_path != normalized_cli_path(expected_report)) {
    throw std::runtime_error(std::string(surface) +
                             " report_path binding mismatch");
  }
  const auto report_checkpoint =
      normalized_cli_path(required_value(report, "checkpoint_path", surface));
  if (report_checkpoint != normalized_cli_path(expected_checkpoint)) {
    throw std::runtime_error(std::string(surface) +
                             " checkpoint_path binding mismatch");
  }
  if (expected_representation.has_value()) {
    const auto report_representation = normalized_cli_path(
        required_value(report, "representation_checkpoint_path", surface));
    if (report_representation !=
        normalized_cli_path(*expected_representation)) {
      throw std::runtime_error(std::string(surface) +
                               " representation checkpoint binding mismatch");
    }
  }
}

struct DeploymentBinding {
  double threshold{kShadowDeploymentTolerance};
  int64_t chunk_size{kShadowChunkSize};
  std::string cpu_prediction_digest;
  std::string cuda_prediction_digest;
  std::string semantic_digest;
  double canonical_maximum_abs_delta{0.0};
};

DeploymentBinding validate_deployment_probe(const KvMap &probe) {
  constexpr const char *surface = "deployment probe";
  require_value(probe, "schema_id",
                "synthetic_mdn_frozen_affine_deployment_bridge_v1", surface);
  require_value(probe, "status", "complete", surface);
  require_value(probe, "input.development.sha256",
                std::string(kExpectedDevelopmentSha), surface);
  require_value(probe, "input.representation_checkpoint.sha256",
                std::string(kExpectedRepresentationCheckpointSha), surface);
  require_value(probe, "input.mdn_checkpoint.sha256",
                std::string(kExpectedMdnCheckpointSha), surface);
  require_value(probe, "input.graph_order_fingerprint",
                std::string(kShadowExpectedGraphFingerprint), surface);
  require_value(probe, "data.record_schema",
                "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1",
                surface);
  require_value(probe, "data.readout_shape", "[730,3,3,400]", surface);
  require_value(probe, "data.live_readout_digest",
                std::string(kExpectedReadoutDigest), surface);
  require_value(probe, "data.maximum_anchor_loaded", "729", surface);
  require_value(probe, "boundary.development_only", "true", surface);
  require_value(probe, "boundary.holdout.opened", "false", surface);
  require_value(probe, "split.fit", "[0,554)", surface);
  require_value(probe, "split.purge", "[554,584)", surface);
  require_value(probe, "split.validation", "[584,730)", surface);
  require_value(probe, "split.holdout.opened", "false", surface);
  require_value(probe, "execution.chunk_size", "37", surface);
  require_value(probe, "execution.fit_computation_uses_validation", "false",
                surface);
  require_value(probe, "artifact.family",
                "wikimyei.inference.expected_value.mdn.per_edge_conditioned_"
                "affine_return_head.v2",
                surface);
  require_value(probe, "artifact.schema_version", "2", surface);
  require_value(probe, "artifact.run_only", "true", surface);
  require_value(probe, "artifact.semantic_digest",
                std::string(kExpectedArtifactSemanticDigest), surface);
  require_value(probe, "artifact.reloaded_cpu.prediction_digest",
                std::string(kExpectedV2CpuPredictionDigest), surface);
  require_value(probe, "artifact.reloaded_cuda.prediction_digest",
                std::string(kExpectedV2CudaPredictionDigest), surface);
  require_value(probe, "parity.cpu_to_cuda.pass", "true", surface);
  require_value(probe, "parity.cpu_full_to_chunked.torch_equal", "true",
                surface);
  require_value(probe, "parity.cuda_full_to_chunked.torch_equal", "true",
                surface);
  require_value(probe, "parity.in_memory_to_archive_reload.torch_equal", "true",
                surface);
  require_value(probe, "parity.archive_cpu_suffix_ignored.torch_equal", "true",
                surface);
  require_value(probe, "parity.archive_cuda_suffix_ignored.torch_equal", "true",
                surface);
  require_value(probe, "conclusion.live_centered_mapped_f64.pass", "true",
                surface);
  require_value(probe, "conclusion.artifact_semantic_match", "true", surface);
  require_value(probe, "conclusion.deployment_bridge_pass", "true", surface);

  DeploymentBinding binding;
  binding.threshold = parse_f64_strict(
      required_value(probe, "execution.deployment_threshold", surface),
      "execution.deployment_threshold");
  binding.chunk_size =
      parse_i64_strict(required_value(probe, "execution.chunk_size", surface),
                       "execution.chunk_size");
  binding.cpu_prediction_digest =
      required_value(probe, "artifact.reloaded_cpu.prediction_digest", surface);
  binding.cuda_prediction_digest = required_value(
      probe, "artifact.reloaded_cuda.prediction_digest", surface);
  binding.semantic_digest =
      required_value(probe, "artifact.semantic_digest", surface);
  binding.canonical_maximum_abs_delta = parse_f64_strict(
      required_value(probe,
                     "parity.cpu_reference_to_in_memory.maximum_abs_delta",
                     surface),
      "parity.cpu_reference_to_in_memory.maximum_abs_delta");
  if (binding.threshold != kShadowDeploymentTolerance ||
      binding.chunk_size != kShadowChunkSize ||
      binding.canonical_maximum_abs_delta > binding.threshold) {
    throw std::runtime_error("deployment probe operational gate drift");
  }
  return binding;
}

std::vector<int64_t> tensor_i64_vector(const torch::Tensor &input,
                                       const char *name) {
  if (!input.defined()) {
    throw std::runtime_error(std::string("undefined checkpoint tensor: ") +
                             name);
  }
  auto tensor = input.detach().to(torch::kCPU, torch::kInt64).contiguous();
  tensor = tensor.reshape({tensor.numel()});
  std::vector<int64_t> result(static_cast<std::size_t>(tensor.numel()));
  const auto *data = tensor.data_ptr<int64_t>();
  std::copy(data, data + tensor.numel(), result.begin());
  return result;
}

torch::Tensor read_archive_tensor(torch::serialize::InputArchive &archive,
                                  const std::string &key) {
  torch::Tensor tensor;
  archive.read(key, tensor);
  if (!tensor.defined()) {
    throw std::runtime_error("checkpoint metadata is undefined: " + key);
  }
  return tensor;
}

std::string archive_string(torch::serialize::InputArchive &archive,
                           const std::string &key) {
  const auto bytes =
      tensor_i64_vector(read_archive_tensor(archive, key), key.c_str());
  std::string result;
  result.reserve(bytes.size());
  for (const auto value : bytes) {
    if (value < 0 || value > 255) {
      throw std::runtime_error("checkpoint string byte is out of range: " +
                               key);
    }
    result.push_back(static_cast<char>(value));
  }
  return result;
}

int64_t archive_i64(torch::serialize::InputArchive &archive,
                    const std::string &key) {
  const auto tensor = read_archive_tensor(archive, key);
  if (tensor.numel() != 1) {
    throw std::runtime_error("checkpoint scalar metadata has wrong shape: " +
                             key);
  }
  return tensor.to(torch::kCPU, torch::kInt64).item<int64_t>();
}

double archive_f64(torch::serialize::InputArchive &archive,
                   const std::string &key) {
  const auto tensor = read_archive_tensor(archive, key);
  if (tensor.numel() != 1) {
    throw std::runtime_error("checkpoint scalar metadata has wrong shape: " +
                             key);
  }
  const auto value = tensor.to(torch::kCPU, torch::kFloat64).item<double>();
  if (!std::isfinite(value)) {
    throw std::runtime_error("checkpoint scalar metadata is non-finite: " +
                             key);
  }
  return value;
}

std::vector<std::string>
archive_string_list(torch::serialize::InputArchive &archive,
                    const std::string &length_key,
                    const std::string &byte_key) {
  const auto lengths = tensor_i64_vector(
      read_archive_tensor(archive, length_key), length_key.c_str());
  const auto bytes = tensor_i64_vector(read_archive_tensor(archive, byte_key),
                                       byte_key.c_str());
  std::vector<std::string> values;
  values.reserve(lengths.size());
  std::size_t offset = 0;
  for (const auto length : lengths) {
    if (length < 0 ||
        offset + static_cast<std::size_t>(length) > bytes.size()) {
      throw std::runtime_error("checkpoint string-list metadata is malformed");
    }
    std::string value;
    value.reserve(static_cast<std::size_t>(length));
    for (int64_t index = 0; index < length; ++index) {
      const auto byte = bytes[offset++];
      if (byte < 0 || byte > 255) {
        throw std::runtime_error("checkpoint string-list byte is out of range");
      }
      value.push_back(static_cast<char>(byte));
    }
    values.push_back(std::move(value));
  }
  if (offset != bytes.size()) {
    throw std::runtime_error(
        "checkpoint string-list metadata has trailing bytes");
  }
  return values;
}

void require_archive_i64(torch::serialize::InputArchive &archive,
                         const std::string &key, int64_t expected) {
  if (archive_i64(archive, key) != expected) {
    throw std::runtime_error("checkpoint integer metadata mismatch: " + key);
  }
}

void require_archive_f64(torch::serialize::InputArchive &archive,
                         const std::string &key, double expected) {
  if (std::fabs(archive_f64(archive, key) - expected) > 1.0e-12) {
    throw std::runtime_error("checkpoint floating metadata mismatch: " + key);
  }
}

void require_archive_string(torch::serialize::InputArchive &archive,
                            const std::string &key,
                            const std::string &expected) {
  if (archive_string(archive, key) != expected) {
    throw std::runtime_error("checkpoint string metadata mismatch: " + key);
  }
}

production_mdn::ChannelContextMdn
make_shadow_model(const torch::Device &device) {
  production_mdn::DirectEdgeReturnHeadOptions direct_options;
  direct_options.feature_dim = kShadowHiddenWidth;
  direct_options.quote_node_index = 0;
  direct_options.identity_mode = "edge_embedding_per_edge";
  direct_options.base_edge_count = kShadowEdgeCount;
  direct_options.identity_embedding_dim = kShadowIdentityEmbeddingDim;
  direct_options.adapter_hidden_dim = kShadowDirectAdapterHiddenDim;
  return production_mdn::ChannelContextMdn(
      kShadowContextDim, kShadowTargetDim, kShadowChannelCount,
      kShadowFutureHorizon, kShadowMixtureCount, kShadowHiddenWidth,
      kShadowResidualDepth, torch::kFloat32, device, kShadowFeatureEmbeddingDim,
      kShadowChannelAdapterRank, kShadowModelTargetCoords, kShadowSigmaMin,
      direct_options);
}

void validate_and_load_checkpoint(const std::filesystem::path &path,
                                  production_mdn::ChannelContextMdn &model) {
  torch::serialize::InputArchive root;
  root.load_from(path.string(), model->device);

  require_archive_i64(root, "meta/channel_count", kShadowChannelCount);
  require_archive_i64(root, "meta/context_dim", kShadowContextDim);
  require_archive_i64(root, "meta/future_horizon", kShadowFutureHorizon);
  require_archive_i64(root, "meta/mixture_count", kShadowMixtureCount);
  require_archive_i64(root, "meta/hidden_width", kShadowHiddenWidth);
  require_archive_i64(root, "meta/residual_depth", kShadowResidualDepth);
  require_archive_i64(root, "meta/feature_embedding_dim",
                      kShadowFeatureEmbeddingDim);
  require_archive_i64(root, "meta/channel_adapter_rank",
                      kShadowChannelAdapterRank);
  require_archive_i64(root, "meta/direct_edge_return_readout_base_edge_count",
                      kShadowEdgeCount);
  require_archive_i64(root,
                      "meta/direct_edge_return_readout_identity_embedding_dim",
                      kShadowIdentityEmbeddingDim);
  require_archive_i64(root,
                      "meta/direct_edge_return_readout_adapter_hidden_dim",
                      kShadowDirectAdapterHiddenDim);
  require_archive_string(root, "meta/component_assembly_id_bytes", "mdn_v1");
  require_archive_string(root, "meta/input_representation_assembly_id_bytes",
                         "mtf_jepa_mae_vicreg_v1");
  require_archive_string(root, "meta/context_contract_bytes",
                         "graph_order.channel_node_representation.v1");
  require_archive_string(root, "meta/output_contract_bytes",
                         "graph_order.channel_node_future_distribution.v1");
  require_archive_string(root, "meta/context_mode_bytes",
                         "channel_context_strict");
  require_archive_string(root, "meta/target_domain_bytes",
                         "channel_node_future");
  require_archive_string(root, "meta/target_mask_policy_bytes",
                         "per_target_feature_valid");
  require_archive_string(root, "meta/activity_target_bytes",
                         "node_feature_support_mean");
  require_archive_string(root,
                         "meta/direct_edge_return_readout_identity_mode_bytes",
                         "edge_embedding_per_edge");
  require_archive_string(root, "meta/graph_order_fingerprint_bytes",
                         std::string(kShadowExpectedGraphFingerprint));
  require_archive_f64(root, "meta/sigma_min", kShadowSigmaMin);
  require_archive_f64(root, "meta/sigma_max", kShadowSigmaMax);
  require_archive_f64(root, "meta/eps", kShadowEps);

  const auto coords = tensor_i64_vector(
      read_archive_tensor(root, "meta/target_coords"), "meta/target_coords");
  if (coords != kShadowModelTargetCoords) {
    throw std::runtime_error("checkpoint target coordinate metadata mismatch");
  }
  const auto nodes =
      archive_string_list(root, "meta/node_id_lengths", "meta/node_id_bytes");
  if (nodes != kShadowNodeIds) {
    throw std::runtime_error("checkpoint node order metadata mismatch");
  }

  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);
  model->eval();
  if (model->is_training()) {
    throw std::runtime_error("loaded shadow model did not enter eval mode");
  }
}

void append_int64_tensor(std::string &bytes, const std::string &name,
                         const torch::Tensor &input) {
  auto tensor = input.detach().to(torch::kCPU, torch::kInt64).contiguous();
  bytes += "tensor=" + name +
           "\ndtype=int64\nrank=" + std::to_string(tensor.dim()) + "\n";
  for (int64_t dimension = 0; dimension < tensor.dim(); ++dimension) {
    bytes += "shape." + std::to_string(dimension) + "=" +
             std::to_string(tensor.size(dimension)) + "\n";
  }
  const auto *values = tensor.data_ptr<int64_t>();
  for (int64_t index = 0; index < tensor.numel(); ++index) {
    append_u64_be(bytes, static_cast<std::uint64_t>(values[index]));
  }
}

std::string module_state_digest(torch::nn::Module &module,
                                const std::string &role) {
  std::vector<std::tuple<std::string, std::string, torch::Tensor>> tensors;
  for (const auto &item : module.named_parameters(/*recurse=*/true)) {
    tensors.emplace_back("parameter", item.key(), item.value());
  }
  for (const auto &item : module.named_buffers(/*recurse=*/true)) {
    tensors.emplace_back("buffer", item.key(), item.value());
  }
  std::sort(tensors.begin(), tensors.end(),
            [](const auto &lhs, const auto &rhs) {
              return std::tie(std::get<0>(lhs), std::get<1>(lhs)) <
                     std::tie(std::get<0>(rhs), std::get<1>(rhs));
            });
  std::string bytes =
      "synthetic_mdn_frozen_conditioned_affine_shadow_module_state.v1\nrole=" +
      role + "\n";
  for (const auto &[kind, name, tensor] : tensors) {
    bytes += "kind=" + kind + "\nname=" + name + "\n";
    if (tensor.is_floating_point()) {
      bytes += "dtype=float32\n";
      append_float32_tensor(bytes, name, tensor);
    } else if (tensor.scalar_type() == torch::kInt64) {
      append_int64_tensor(bytes, name, tensor);
    } else {
      throw std::runtime_error("unsupported module-state tensor dtype");
    }
  }
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string shadow_readout_digest(const torch::Tensor &readout) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_deployment_bridge_tensor_float32.v1\n"
      "role=live_readout_features\n";
  append_float32_tensor(bytes, "live_readout_features", readout);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

TensorComparison compare_tensors(const torch::Tensor &lhs_input,
                                 const torch::Tensor &rhs_input,
                                 double threshold) {
  const auto lhs =
      lhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto rhs =
      rhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  if (lhs.sizes() != rhs.sizes() || lhs.numel() <= 0 ||
      !torch::isfinite(lhs).all().item<bool>() ||
      !torch::isfinite(rhs).all().item<bool>()) {
    throw std::runtime_error("shadow prediction comparison is invalid");
  }
  const auto delta = (lhs - rhs).abs();
  TensorComparison result;
  result.torch_equal = torch::equal(lhs, rhs);
  result.maximum_abs_delta = delta.max().item<double>();
  result.mean_abs_delta = delta.mean().item<double>();
  result.rmse_delta = delta.square().mean().sqrt().item<double>();
  result.threshold = threshold;
  result.pass = std::isfinite(result.maximum_abs_delta) &&
                result.maximum_abs_delta <= threshold;
  return result;
}

template <typename Forward>
torch::Tensor run_chunked(const torch::Tensor &cpu_readout,
                          const torch::Device &device, int64_t chunk_size,
                          Forward &&forward) {
  if (cpu_readout.dim() != 4 || cpu_readout.size(0) <= 0 || chunk_size <= 0) {
    throw std::runtime_error("invalid chunked shadow-readout request");
  }
  std::vector<torch::Tensor> outputs;
  for (int64_t begin = 0; begin < cpu_readout.size(0); begin += chunk_size) {
    const auto count = std::min(chunk_size, cpu_readout.size(0) - begin);
    auto chunk = cpu_readout.narrow(0, begin, count).to(device);
    outputs.push_back(forward(chunk));
  }
  return torch::cat(outputs, 0);
}

PredictionBundle
evaluate_model_head(production_mdn::ChannelContextMdn &cpu_model,
                    production_mdn::ChannelContextMdn &cuda_model,
                    const torch::Tensor &readout_cpu) {
  PredictionBundle bundle;
  bundle.cpu_full =
      cpu_model->direct_edge_head->forward_readout_features(readout_cpu);
  bundle.cpu_chunked = run_chunked(
      readout_cpu, torch::Device(torch::kCPU), kShadowChunkSize,
      [&](const torch::Tensor &chunk) {
        return cpu_model->direct_edge_head->forward_readout_features(chunk);
      });
  const auto readout_cuda = readout_cpu.to(torch::kCUDA);
  bundle.cuda_full =
      cuda_model->direct_edge_head->forward_readout_features(readout_cuda);
  bundle.cuda_chunked = run_chunked(
      readout_cpu, torch::Device(torch::kCUDA), kShadowChunkSize,
      [&](const torch::Tensor &chunk) {
        return cuda_model->direct_edge_head->forward_readout_features(chunk);
      });
  return bundle;
}

PredictionBundle
evaluate_v2_head(production_mdn::PerEdgeConditionedAffineReturnHead &cpu_head,
                 production_mdn::PerEdgeConditionedAffineReturnHead &cuda_head,
                 const torch::Tensor &readout_cpu) {
  PredictionBundle bundle;
  bundle.cpu_full = cpu_head->forward_readout_features(readout_cpu);
  bundle.cpu_chunked =
      run_chunked(readout_cpu, torch::Device(torch::kCPU), kShadowChunkSize,
                  [&](const torch::Tensor &chunk) {
                    return cpu_head->forward_readout_features(chunk);
                  });
  const auto readout_cuda = readout_cpu.to(torch::kCUDA);
  bundle.cuda_full = cuda_head->forward_readout_features(readout_cuda);
  bundle.cuda_chunked =
      run_chunked(readout_cpu, torch::Device(torch::kCUDA), kShadowChunkSize,
                  [&](const torch::Tensor &chunk) {
                    return cuda_head->forward_readout_features(chunk);
                  });
  return bundle;
}

torch::Tensor make_suffix_perturbation(const torch::Tensor &input) {
  auto result = input.clone();
  auto suffix = result.narrow(3, kShadowDynamicWidth,
                              kShadowReadoutWidth - kShadowDynamicWidth);
  for (int64_t feature = 0; feature < suffix.size(3); ++feature) {
    suffix.select(3, feature)
        .fill_(feature % 2 == 0 ? std::numeric_limits<float>::quiet_NaN()
                                : std::numeric_limits<float>::infinity());
  }
  return result;
}

double metric_mae(const Metric &metric) {
  return metric.count > 0 ? metric.abs_error / static_cast<double>(metric.count)
                          : std::numeric_limits<double>::infinity();
}

double metric_prediction_std(const Metric &metric) {
  if (metric.count <= 0) {
    return 0.0;
  }
  const auto count = static_cast<double>(metric.count);
  const auto variance =
      metric.prediction_sq_sum / count -
      (metric.prediction_sum / count) * (metric.prediction_sum / count);
  return std::sqrt(std::max(variance, 0.0));
}

LossSummary compute_production_loss(const torch::Tensor &prediction,
                                    const torch::Tensor &future_cpu,
                                    const torch::Tensor &mask_cpu) {
  production_mdn::channel_context_mdn_train_options_t options;
  options.direct_edge_return_readout_enabled = true;
  options.direct_edge_return_readout_loss_weight = 100.0;
  options.direct_edge_return_readout_direction_weight = 5.0;
  options.direct_edge_return_readout_rank_weight = 5.0;
  options.direct_edge_return_readout_huber_beta = 0.5;
  options.direct_edge_return_readout_logit_scale = 1.0;
  options.direct_edge_return_readout_target_scale = 36.0;
  production_mdn::MdnOut output{};
  output.direct_edge_return = prediction;
  const auto direct = production_mdn::channel_context_mdn_train_detail::
      compute_direct_edge_return_readout_loss(
          output, future_cpu.to(prediction.device()),
          mask_cpu.to(prediction.device()), options, kShadowLossTargetCoords);
  return {.total = direct.loss.item<double>(),
          .regression = direct.regression_loss.item<double>(),
          .direction = direct.direction_loss.item<double>(),
          .rank = direct.rank_loss.item<double>(),
          .valid_count = direct.valid_count,
          .pairwise_valid_count = direct.pairwise_valid_count};
}

void emit_shadow_comparison(std::ostream &output, const std::string &prefix,
                            const TensorComparison &comparison) {
  output << prefix << ".torch_equal=" << bool_text(comparison.torch_equal)
         << '\n';
  output << prefix << ".maximum_abs_delta=" << comparison.maximum_abs_delta
         << '\n';
  output << prefix << ".mean_abs_delta=" << comparison.mean_abs_delta << '\n';
  output << prefix << ".rmse_delta=" << comparison.rmse_delta << '\n';
  output << prefix << ".threshold=" << comparison.threshold << '\n';
  output << prefix << ".pass=" << bool_text(comparison.pass) << '\n';
}

void emit_shadow_metric(std::ostream &output, const std::string &prefix,
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

void emit_shadow_loss(std::ostream &output, const std::string &prefix,
                      const LossSummary &loss) {
  output << prefix << ".total=" << loss.total << '\n';
  output << prefix << ".regression=" << loss.regression << '\n';
  output << prefix << ".direction=" << loss.direction << '\n';
  output << prefix << ".rank=" << loss.rank << '\n';
  output << prefix << ".valid_count=" << loss.valid_count << '\n';
  output << prefix << ".pairwise_valid_count=" << loss.pairwise_valid_count
         << '\n';
}

void require_loss_counts(const LossSummary &loss, int64_t anchor_count,
                         const char *surface) {
  const auto expected = anchor_count * kShadowEdgeCount * kShadowChannelCount;
  if (loss.valid_count != expected || loss.pairwise_valid_count != expected) {
    throw std::runtime_error(
        std::string("production objective domain drift: ") + surface);
  }
}

} // namespace shadow_readout

int main(int argc, char **argv) {
  using namespace shadow_readout;
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_shadow_options(argc, argv);
    if (options.schema_id != kSchema) {
      throw std::runtime_error("shadow-readout schema does not match contract");
    }
    if (std::filesystem::exists(options.output) ||
        std::filesystem::exists(options.output.string() + ".tmp")) {
      throw std::runtime_error("refusing to replace a shadow-readout output");
    }

    const std::array<std::pair<std::filesystem::path, std::string_view>, 8>
        pinned_inputs{{
            {options.development_input, kExpectedDevelopmentSha},
            {options.capture_report, kExpectedCaptureReportSha},
            {options.representation_training_report,
             kExpectedRepresentationTrainingReportSha},
            {options.mdn_training_report, kExpectedMdnTrainingReportSha},
            {options.representation_checkpoint,
             kExpectedRepresentationCheckpointSha},
            {options.mdn_checkpoint, kExpectedMdnCheckpointSha},
            {options.v2_artifact, kExpectedV2ArtifactSha},
            {options.deployment_probe, kExpectedDeploymentProbeSha},
        }};
    for (const auto &[path, expected] : pinned_inputs) {
      if (!std::filesystem::is_regular_file(path) ||
          sha256_file(path) != expected) {
        throw std::runtime_error(
            "pinned shadow-readout input identity mismatch");
      }
    }

    const auto capture_report =
        read_exact_kv(options.capture_report, "capture report");
    validate_capture_report(capture_report);
    const auto representation_training_report =
        read_exact_kv(options.representation_training_report,
                      "representation training report");
    const auto mdn_training_report =
        read_exact_kv(options.mdn_training_report, "MDN training report");
    const auto capture_representation_checkpoint = normalized_cli_path(
        required_value(capture_report, "representation_checkpoint_path",
                       "capture report"));
    const auto capture_mdn_checkpoint = normalized_cli_path(required_value(
        capture_report, "mdn_checkpoint_path", "capture report"));
    const auto capture_report_self_path = normalized_cli_path(
        required_value(capture_report, "report_path", "capture report"));
    const auto capture_probe_path = normalized_cli_path(
        required_value(capture_report, "mdn_edge_context_feature_probe_path",
                       "capture report"));
    if (capture_representation_checkpoint !=
            normalized_cli_path(options.representation_checkpoint) ||
        capture_mdn_checkpoint != normalized_cli_path(options.mdn_checkpoint) ||
        capture_report_self_path !=
            normalized_cli_path(options.capture_report) ||
        capture_probe_path != normalized_cli_path(options.development_input)) {
      throw std::runtime_error(
          "capture report self/probe/checkpoint path binding mismatch");
    }
    validate_training_report(representation_training_report,
                             options.representation_training_report, 3000,
                             options.representation_checkpoint, std::nullopt,
                             "representation training report");
    validate_training_report(mdn_training_report, options.mdn_training_report,
                             3500, options.mdn_checkpoint,
                             options.representation_checkpoint,
                             "MDN training report");
    const auto deployment_probe =
        read_exact_kv(options.deployment_probe, "deployment probe");
    const auto deployment_binding = validate_deployment_probe(deployment_probe);

    const auto runtime = configure_deterministic_runtime();
    const auto development =
        read_probe(options.development_input, 0, kShadowDevelopmentEnd);
    if (development.record_schema != kProbeRecordSchema ||
        development.row_count !=
            kShadowDevelopmentEnd * kShadowEdgeCount * kShadowChannelCount ||
        development.readout_features.sizes() !=
            torch::IntArrayRef({kShadowDevelopmentEnd, kShadowEdgeCount,
                                kShadowChannelCount, kShadowReadoutWidth}) ||
        development.target.sizes() !=
            torch::IntArrayRef({kShadowDevelopmentEnd, kShadowEdgeCount,
                                kShadowChannelCount}) ||
        development.edge_ids != kShadowEdgeIds ||
        development.base_node_ids !=
            std::vector<std::string>(kShadowNodeIds.begin() + 1,
                                     kShadowNodeIds.end()) ||
        development.quote_node_id != kShadowNodeIds.front() ||
        development.anchors.front() != 0 ||
        development.anchors.back() != kShadowDevelopmentEnd - 1 ||
        !development.prefix_torch_equal ||
        development.prefix_max_abs_delta != 0.0) {
      throw std::runtime_error("development capture contract mismatch");
    }
    const auto readout_digest =
        shadow_readout_digest(development.readout_features);
    if (readout_digest != kExpectedReadoutDigest) {
      throw std::runtime_error("development readout tensor digest mismatch");
    }

    reset_deterministic_seed();
    auto model_cpu = make_shadow_model(torch::Device(torch::kCPU));
    validate_and_load_checkpoint(options.mdn_checkpoint, model_cpu);
    const auto model_cpu_state_digest =
        module_state_digest(*model_cpu, "full_channel_context_mdn");
    const auto direct_cpu_state_digest = module_state_digest(
        *model_cpu->direct_edge_head, "checkpoint_direct_edge_return_head");
    if (!model_cpu->direct_edge_head->uses_edge_embedding ||
        model_cpu->direct_edge_head->edge_embedding.is_empty()) {
      throw std::runtime_error(
          "loaded checkpoint direct head has no required edge embedding");
    }
    const auto cached_identity_suffix = development.readout_features.narrow(
        3, kShadowDynamicWidth, kShadowReadoutWidth - kShadowDynamicWidth);
    const auto checkpoint_identity_suffix =
        model_cpu->direct_edge_head->edge_embedding->weight.detach()
            .to(torch::kCPU, torch::kFloat32)
            .view({1, kShadowEdgeCount, 1, kShadowIdentityEmbeddingDim})
            .expand({kShadowDevelopmentEnd, kShadowEdgeCount,
                     kShadowChannelCount, kShadowIdentityEmbeddingDim});
    const bool identity_suffix_torch_equal =
        torch::equal(cached_identity_suffix, checkpoint_identity_suffix);
    const double identity_suffix_maximum_abs_delta =
        (cached_identity_suffix - checkpoint_identity_suffix)
            .abs()
            .max()
            .item<double>();
    if (!identity_suffix_torch_equal ||
        identity_suffix_maximum_abs_delta != 0.0) {
      throw std::runtime_error(
          "cached identity suffix does not match checkpoint edge embedding");
    }

    reset_deterministic_seed();
    auto model_cuda = make_shadow_model(torch::Device(torch::kCUDA));
    validate_and_load_checkpoint(options.mdn_checkpoint, model_cuda);
    const auto model_cuda_state_digest =
        module_state_digest(*model_cuda, "full_channel_context_mdn");
    const auto direct_cuda_state_digest = module_state_digest(
        *model_cuda->direct_edge_head, "checkpoint_direct_edge_return_head");
    if (model_cpu_state_digest != model_cuda_state_digest ||
        direct_cpu_state_digest != direct_cuda_state_digest) {
      throw std::runtime_error("CPU/CUDA loaded checkpoint state mismatch");
    }

    auto v2_cpu = production_mdn::load_per_edge_conditioned_affine_return_head(
        options.v2_artifact);
    auto v2_cuda = production_mdn::load_per_edge_conditioned_affine_return_head(
        options.v2_artifact);
    v2_cuda.head->to(torch::kCUDA);
    v2_cuda.head->eval();
    v2_cuda.head->validate_registered_state();
    if (production_mdn::canonical_conditioned_affine_metadata_text(
            v2_cpu.metadata) !=
        production_mdn::canonical_conditioned_affine_metadata_text(
            v2_cuda.metadata)) {
      throw std::runtime_error("CPU/CUDA v2 artifact metadata mismatch");
    }
    const auto &metadata = v2_cpu.metadata;
    if (metadata.schema_version != 2 ||
        metadata.artifact_family !=
            production_mdn::kPerEdgeConditionedAffineReturnHeadArtifactFamily ||
        !metadata.run_only || metadata.policy_eligible ||
        metadata.feature_dim != kShadowDynamicWidth ||
        metadata.edge_count != kShadowEdgeCount ||
        metadata.channel_count != kShadowChannelCount ||
        metadata.fit_begin != 0 || metadata.fit_end != kShadowFitEnd ||
        metadata.purge_begin != kShadowFitEnd ||
        metadata.purge_end != kShadowPurgeEnd ||
        metadata.validation_begin != kShadowPurgeEnd ||
        metadata.validation_end != kShadowDevelopmentEnd ||
        metadata.max_anchor != kShadowDevelopmentEnd - 1 ||
        metadata.graph_order_fingerprint != kShadowExpectedGraphFingerprint ||
        metadata.edge_ids != kShadowEdgeIds ||
        metadata.node_ids != kShadowNodeIds ||
        metadata.source_probe_sha256 != kExpectedDevelopmentSha ||
        metadata.conditioning_probe_sha256 != kExpectedConditioningProbeSha ||
        metadata.representation_checkpoint_sha256 !=
            kExpectedRepresentationCheckpointSha ||
        metadata.mdn_checkpoint_sha256 != kExpectedMdnCheckpointSha ||
        metadata.conditioning_transform_sha256 !=
            kExpectedConditioningTransformSha ||
        metadata.semantic_tensor_digest != kExpectedArtifactSemanticDigest ||
        metadata.semantic_tensor_digest != deployment_binding.semantic_digest) {
      throw std::runtime_error("v2 artifact provenance contract mismatch");
    }

    torch::NoGradGuard no_grad;
    const auto mlp = evaluate_model_head(model_cpu, model_cuda,
                                         development.readout_features);
    const auto v2 = evaluate_v2_head(v2_cpu.head, v2_cuda.head,
                                     development.readout_features);
    const auto v2_suffix_cpu = v2_cpu.head->forward_readout_features(
        make_suffix_perturbation(development.readout_features));
    const auto v2_suffix_cuda = v2_cuda.head->forward_readout_features(
        make_suffix_perturbation(development.readout_features)
            .to(torch::kCUDA));

    const auto mlp_cpu_chunk = compare_tensors(mlp.cpu_full, mlp.cpu_chunked,
                                               kShadowDeploymentTolerance);
    const auto mlp_cuda_chunk = compare_tensors(mlp.cuda_full, mlp.cuda_chunked,
                                                kShadowDeploymentTolerance);
    const auto mlp_cpu_cuda = compare_tensors(mlp.cpu_full, mlp.cuda_full,
                                              kShadowDeploymentTolerance);
    const auto v2_cpu_chunk = compare_tensors(v2.cpu_full, v2.cpu_chunked,
                                              kShadowDeploymentTolerance);
    const auto v2_cuda_chunk = compare_tensors(v2.cuda_full, v2.cuda_chunked,
                                               kShadowDeploymentTolerance);
    const auto v2_cpu_cuda =
        compare_tensors(v2.cpu_full, v2.cuda_full, kShadowDeploymentTolerance);
    const auto v2_cpu_suffix =
        compare_tensors(v2.cpu_full, v2_suffix_cpu, kShadowDeploymentTolerance);
    const auto v2_cuda_suffix = compare_tensors(v2.cuda_full, v2_suffix_cuda,
                                                kShadowDeploymentTolerance);

    const auto v2_cpu_digest = prediction_digest(v2.cpu_full);
    const auto v2_cuda_digest = prediction_digest(v2.cuda_full);
    const bool v2_strict_operational_pass =
        v2_cpu_digest == deployment_binding.cpu_prediction_digest &&
        v2_cuda_digest == deployment_binding.cuda_prediction_digest &&
        v2_cpu_digest == kExpectedV2CpuPredictionDigest &&
        v2_cuda_digest == kExpectedV2CudaPredictionDigest &&
        v2_cpu_chunk.torch_equal && v2_cuda_chunk.torch_equal &&
        v2_cpu_suffix.torch_equal && v2_cuda_suffix.torch_equal &&
        v2_cpu_cuda.pass;
    const bool mlp_operational_pass =
        mlp_cpu_chunk.pass && mlp_cuda_chunk.pass && mlp_cpu_cuda.pass;
    if (!v2_strict_operational_pass || !mlp_operational_pass) {
      throw std::runtime_error("shadow-readout operational parity failed");
    }

    auto quote = torch::zeros({kShadowDevelopmentEnd, 1, kShadowChannelCount},
                              torch::kFloat32);
    const auto future =
        torch::cat({quote, development.target}, 1).unsqueeze(3).contiguous();
    const auto future_mask = torch::ones_like(future, torch::kBool);
    const auto reconstructed_target =
        future.select(3, 0).narrow(1, 1, kShadowEdgeCount) -
        future.select(3, 0).select(1, 0).unsqueeze(1);
    if (!torch::equal(reconstructed_target, development.target)) {
      throw std::runtime_error(
          "synthetic future does not preserve edge targets");
    }

    const auto fit_target = development.target.narrow(0, 0, kShadowFitEnd);
    const auto validation_target = development.target.narrow(
        0, kShadowPurgeEnd, kShadowDevelopmentEnd - kShadowPurgeEnd);
    const auto mlp_fit_prediction = mlp.cuda_full.narrow(0, 0, kShadowFitEnd);
    const auto mlp_validation_prediction = mlp.cuda_full.narrow(
        0, kShadowPurgeEnd, kShadowDevelopmentEnd - kShadowPurgeEnd);
    const auto v2_fit_prediction = v2.cuda_full.narrow(0, 0, kShadowFitEnd);
    const auto v2_validation_prediction = v2.cuda_full.narrow(
        0, kShadowPurgeEnd, kShadowDevelopmentEnd - kShadowPurgeEnd);

    const auto mlp_development_metric =
        evaluate_prediction(mlp.cuda_full, development.target);
    const auto mlp_fit_metric =
        evaluate_prediction(mlp_fit_prediction, fit_target);
    const auto mlp_validation_metric =
        evaluate_prediction(mlp_validation_prediction, validation_target);
    const auto v2_fit_metric =
        evaluate_prediction(v2_fit_prediction, fit_target);
    const auto v2_validation_metric =
        evaluate_prediction(v2_validation_prediction, validation_target);
    require_metric_counts(mlp_development_metric, 6570, 6570,
                          "checkpoint MLP development");
    require_metric_counts(mlp_fit_metric, 4986, 4986, "checkpoint MLP fit");
    require_metric_counts(mlp_validation_metric, 1314, 1314,
                          "checkpoint MLP validation");
    require_metric_counts(v2_fit_metric, 4986, 4986, "v2 fit");
    require_metric_counts(v2_validation_metric, 1314, 1314, "v2 validation");

    const std::array<std::pair<double, double>, 6> capture_metric_checks{{
        {metric_mae(mlp_development_metric), 0.0223379},
        {metric_rmse(mlp_development_metric), 0.0282661},
        {metric_direction(mlp_development_metric), 0.483257},
        {metric_rank(mlp_development_metric), 0.512938},
        {metric_correlation(mlp_development_metric), -0.00328374},
        {metric_prediction_std(mlp_development_metric), 0.00546615},
    }};
    double capture_metric_max_abs_delta = 0.0;
    for (const auto &[actual, expected] : capture_metric_checks) {
      capture_metric_max_abs_delta =
          std::max(capture_metric_max_abs_delta, std::fabs(actual - expected));
    }
    if (capture_metric_max_abs_delta > kCaptureMetricTolerance) {
      throw std::runtime_error("checkpoint MLP capture-report metric drift");
    }

    const auto fit_future = future.narrow(0, 0, kShadowFitEnd);
    const auto fit_mask = future_mask.narrow(0, 0, kShadowFitEnd);
    const auto validation_future = future.narrow(
        0, kShadowPurgeEnd, kShadowDevelopmentEnd - kShadowPurgeEnd);
    const auto validation_mask = future_mask.narrow(
        0, kShadowPurgeEnd, kShadowDevelopmentEnd - kShadowPurgeEnd);
    const auto mlp_fit_loss =
        compute_production_loss(mlp_fit_prediction, fit_future, fit_mask);
    const auto mlp_validation_loss = compute_production_loss(
        mlp_validation_prediction, validation_future, validation_mask);
    const auto v2_fit_loss =
        compute_production_loss(v2_fit_prediction, fit_future, fit_mask);
    const auto v2_validation_loss = compute_production_loss(
        v2_validation_prediction, validation_future, validation_mask);
    require_loss_counts(mlp_fit_loss, kShadowFitEnd, "checkpoint MLP fit");
    require_loss_counts(mlp_validation_loss,
                        kShadowDevelopmentEnd - kShadowPurgeEnd,
                        "checkpoint MLP validation");
    require_loss_counts(v2_fit_loss, kShadowFitEnd, "v2 fit");
    require_loss_counts(v2_validation_loss,
                        kShadowDevelopmentEnd - kShadowPurgeEnd,
                        "v2 validation");

    std::ostringstream probe;
    probe.imbue(std::locale::classic());
    probe << std::scientific << std::setprecision(17);
    probe << "schema_id=" << kSchema << '\n';
    probe << "status=complete\n";
    probe << "result_role=development_only_conditioned_affine_shadow_readout\n";
    probe << "diagnostic_authority=diagnostic_only\n";
    probe << "benchmark_acceptance_authority=false\n";
    probe << "policy_path_used=false\n";
    probe << "fit_performed=false\n";
    probe << "refit_performed=false\n";
    probe << "history_input_used=false\n";
    probe << "holdout_input_used=false\n";
    probe << "input.development.sha256=" << kExpectedDevelopmentSha << '\n';
    probe << "input.capture_report.sha256=" << kExpectedCaptureReportSha
          << '\n';
    probe << "input.representation_training_report.sha256="
          << kExpectedRepresentationTrainingReportSha << '\n';
    probe << "input.mdn_training_report.sha256="
          << kExpectedMdnTrainingReportSha << '\n';
    probe << "input.representation_checkpoint.sha256="
          << kExpectedRepresentationCheckpointSha << '\n';
    probe << "input.mdn_checkpoint.sha256=" << kExpectedMdnCheckpointSha
          << '\n';
    probe << "input.v2_artifact.sha256=" << kExpectedV2ArtifactSha << '\n';
    probe << "input.deployment_probe.sha256=" << kExpectedDeploymentProbeSha
          << '\n';
    probe << "data.record_schema=" << development.record_schema << '\n';
    probe << "data.readout_shape=[730,3,3,400]\n";
    probe << "data.target_shape=[730,3,3]\n";
    probe << "data.row_count=" << development.row_count << '\n';
    probe << "data.readout_digest=" << readout_digest << '\n';
    probe << "data.graph_order_fingerprint=" << kShadowExpectedGraphFingerprint
          << '\n';
    probe << "data.maximum_anchor_loaded=729\n";
    probe << "data.identity_suffix_matches_checkpoint.torch_equal="
          << bool_text(identity_suffix_torch_equal) << '\n';
    probe << "data.identity_suffix_matches_checkpoint.maximum_abs_delta="
          << identity_suffix_maximum_abs_delta << '\n';
    probe << "boundary.development=[0,730)\n";
    probe << "boundary.fit=[0,554)\n";
    probe << "boundary.purge=[554,584)\n";
    probe << "boundary.validation=[584,730)\n";
    probe << "boundary.unconsumed_holdout=[1088,1170)\n";
    probe << "boundary.unconsumed_holdout_opened=false\n";
    probe << "provenance.representation_training.target_action=train\n";
    probe << "provenance.representation_training.anchor_range=[0,730)\n";
    probe << "provenance.representation_training.steps_completed=3000\n";
    probe << "provenance.representation_training.optimizer_steps=3000\n";
    probe << "provenance.representation_training.checkpoint_written=true\n";
    probe << "provenance.mdn_training.target_action=train\n";
    probe << "provenance.mdn_training.anchor_range=[0,730)\n";
    probe << "provenance.mdn_training.steps_completed=3500\n";
    probe << "provenance.mdn_training.optimizer_steps=3500\n";
    probe << "provenance.mdn_training.checkpoint_written=true\n";
    probe << "provenance.report_path_bindings_match=true\n";
    probe << "provenance.capture_report_path_bindings_match=true\n";
    probe << "provenance.checkpoint_path_bindings_match=true\n";
    probe << "representation_checkpoint.input_hash_verified=true\n";
    probe << "representation_checkpoint.path_bound_to_reports=true\n";
    probe << "representation_checkpoint.loaded_by_shadow=false\n";
    probe << "caveat.cached_upstream_representation_trained_on_development_0_"
             "730=true\n";
    probe << "caveat.cached_upstream_mdn_trained_on_development_0_730=true\n";
    probe << "caveat.validation_584_730_withheld_from_v2_affine_fit=true\n";
    probe
        << "caveat.validation_584_730_withheld_from_upstream_training=false\n";
    probe << "caveat.unseen_future_generalization_measured=false\n";
    probe << "caveat.results_are_descriptive=true\n";
    probe << "caveat.policy_performance_measured=false\n";
    probe << "caveat.cached_readout_only=true\n";
    probe << "caveat.representation_forward_executed=false\n";
    probe << "caveat.mdn_backbone_and_distribution_forward_executed=false\n";
    probe << "caveat.mdn_distribution_nll_measured=false\n";
    probe << "caveat.end_to_end_forecast_path_executed=false\n";
    probe << "runtime.torch_version=" << TORCH_VERSION << '\n';
    probe << "runtime.cuda_runtime_version=" << runtime.cuda_runtime_version
          << '\n';
    probe << "runtime.cuda_driver_version=" << runtime.cuda_driver_version
          << '\n';
    probe << "runtime.cuda_device_name=" << runtime.cuda_device_name << '\n';
    probe << "runtime.cpu_thread_count=" << at::get_num_threads() << '\n';
    probe << "runtime.cpu_interop_thread_count="
          << at::get_num_interop_threads() << '\n';
    probe << "runtime.deterministic_algorithms="
          << bool_text(at::globalContext().deterministicAlgorithms()) << '\n';
    probe << "runtime.seed=31\n";
    probe << "execution.chunk_size=37\n";
    probe << "execution.operational_tolerance=" << kShadowDeploymentTolerance
          << '\n';
    probe << "model.channel_count=3\n";
    probe << "model.context_dim=32\n";
    probe << "model.target_feature_dim=9\n";
    probe << "model.future_horizon=1\n";
    probe << "model.mixture_count=3\n";
    probe << "model.hidden_width=128\n";
    probe << "model.residual_depth=2\n";
    probe << "model.feature_embedding_dim=8\n";
    probe << "model.channel_adapter_rank=16\n";
    probe << "model.target_coords=0,1,2,3,4,5,6,7,8\n";
    probe << "model.sigma_min=" << kShadowSigmaMin << '\n';
    probe << "model.direct_head.identity_mode=edge_embedding_per_edge\n";
    probe << "model.direct_head.base_edge_count=3\n";
    probe << "model.direct_head.identity_embedding_dim=16\n";
    probe << "model.direct_head.adapter_hidden_dim=128\n";
    probe << "mdn_checkpoint.metadata_validated=true\n";
    probe << "mdn_checkpoint.full_model_state_digest=" << model_cpu_state_digest
          << '\n';
    probe << "mdn_checkpoint.direct_head_state_digest="
          << direct_cpu_state_digest << '\n';
    probe << "mdn_checkpoint.cpu_cuda_state_match=true\n";
    probe << "mdn_checkpoint.capture_report_metric_parity.maximum_abs_delta="
          << capture_metric_max_abs_delta << '\n';
    probe << "mdn_checkpoint.capture_report_metric_parity.threshold="
          << kCaptureMetricTolerance << '\n';
    probe << "mdn_checkpoint.capture_report_metric_parity.pass=true\n";
    probe << "objective.function=compute_direct_edge_return_readout_loss\n";
    probe << "objective.regression_weight=" << 100.0 << '\n';
    probe << "objective.direction_weight=" << 5.0 << '\n';
    probe << "objective.rank_weight=" << 5.0 << '\n';
    probe << "objective.huber_beta=" << 0.5 << '\n';
    probe << "objective.logit_scale=" << 1.0 << '\n';
    probe << "objective.target_scale=" << 36.0 << '\n';
    probe << "objective.synthetic_future_schema=quote_zero_bases_equal_edge_"
             "targets\n";
    probe << "objective.synthetic_future_target_coords=3\n";
    probe << "objective.synthetic_future_edge_target_torch_equal=true\n";
    probe << "objective.capture_target_binding.raw_close_coord=3\n";
    probe << "objective.capture_target_binding.quote_node_index=0\n";
    probe << "v2.family=" << metadata.artifact_family << '\n';
    probe << "v2.schema_version=" << metadata.schema_version << '\n';
    probe << "v2.run_only=" << bool_text(metadata.run_only) << '\n';
    probe << "v2.policy_eligible=" << bool_text(metadata.policy_eligible)
          << '\n';
    probe << "v2.semantic_digest=" << metadata.semantic_tensor_digest << '\n';
    probe << "v2.conditioning_probe.sha256="
          << metadata.conditioning_probe_sha256 << '\n';
    probe << "v2.conditioning_transform.sha256="
          << metadata.conditioning_transform_sha256 << '\n';
    probe << "v2.sealed_canonical.maximum_abs_delta="
          << deployment_binding.canonical_maximum_abs_delta << '\n';
    probe << "v2.sealed_canonical.threshold=" << deployment_binding.threshold
          << '\n';
    probe << "v2.sealed_canonical.pass=true\n";

    probe << "checkpoint_mlp.cpu.full.prediction_digest="
          << prediction_digest(mlp.cpu_full) << '\n';
    probe << "checkpoint_mlp.cpu.chunked.prediction_digest="
          << prediction_digest(mlp.cpu_chunked) << '\n';
    probe << "checkpoint_mlp.cuda.full.prediction_digest="
          << prediction_digest(mlp.cuda_full) << '\n';
    probe << "checkpoint_mlp.cuda.chunked.prediction_digest="
          << prediction_digest(mlp.cuda_chunked) << '\n';
    emit_shadow_comparison(probe, "checkpoint_mlp.parity.cpu_full_to_chunked",
                           mlp_cpu_chunk);
    emit_shadow_comparison(probe, "checkpoint_mlp.parity.cuda_full_to_chunked",
                           mlp_cuda_chunk);
    emit_shadow_comparison(probe, "checkpoint_mlp.parity.cpu_to_cuda",
                           mlp_cpu_cuda);

    probe << "conditioned_v2.cpu.full.prediction_digest=" << v2_cpu_digest
          << '\n';
    probe << "conditioned_v2.cpu.chunked.prediction_digest="
          << prediction_digest(v2.cpu_chunked) << '\n';
    probe << "conditioned_v2.cpu.suffix_perturbed.prediction_digest="
          << prediction_digest(v2_suffix_cpu) << '\n';
    probe << "conditioned_v2.cuda.full.prediction_digest=" << v2_cuda_digest
          << '\n';
    probe << "conditioned_v2.cuda.chunked.prediction_digest="
          << prediction_digest(v2.cuda_chunked) << '\n';
    probe << "conditioned_v2.cuda.suffix_perturbed.prediction_digest="
          << prediction_digest(v2_suffix_cuda) << '\n';
    emit_shadow_comparison(probe, "conditioned_v2.parity.cpu_full_to_chunked",
                           v2_cpu_chunk);
    emit_shadow_comparison(probe, "conditioned_v2.parity.cuda_full_to_chunked",
                           v2_cuda_chunk);
    emit_shadow_comparison(probe, "conditioned_v2.parity.cpu_to_cuda",
                           v2_cpu_cuda);
    emit_shadow_comparison(probe, "conditioned_v2.parity.cpu_suffix_ignored",
                           v2_cpu_suffix);
    emit_shadow_comparison(probe, "conditioned_v2.parity.cuda_suffix_ignored",
                           v2_cuda_suffix);

    emit_shadow_metric(probe, "checkpoint_mlp.fit.metric", mlp_fit_metric);
    emit_shadow_metric(probe, "checkpoint_mlp.validation.metric",
                       mlp_validation_metric);
    emit_shadow_metric(probe, "conditioned_v2.fit.metric", v2_fit_metric);
    emit_shadow_metric(probe, "conditioned_v2.validation.metric",
                       v2_validation_metric);
    emit_shadow_loss(probe, "checkpoint_mlp.fit.production_objective",
                     mlp_fit_loss);
    emit_shadow_loss(probe, "checkpoint_mlp.validation.production_objective",
                     mlp_validation_loss);
    emit_shadow_loss(probe, "conditioned_v2.fit.production_objective",
                     v2_fit_loss);
    emit_shadow_loss(probe, "conditioned_v2.validation.production_objective",
                     v2_validation_loss);
    probe << "comparison.role=descriptive_no_beats_mlp_gate\n";
    probe << "conclusion.provenance_pass=true\n";
    probe << "conclusion.checkpoint_mlp_operational_parity_pass="
          << bool_text(mlp_operational_pass) << '\n';
    probe << "conclusion.conditioned_v2_operational_parity_pass="
          << bool_text(v2_strict_operational_pass) << '\n';
    probe << "conclusion.fit_performed=false\n";
    probe << "conclusion.selection_gate_applied=false\n";
    probe << "conclusion.beats_mlp_gate_applied=false\n";
    probe << "conclusion.policy_performance_measured=false\n";
    probe << "conclusion.unconsumed_holdout_remains_sealed=true\n";
    probe << "conclusion.shadow_replay_complete=true\n";

    write_probe_atomic(options.output, probe.str());
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "shadow-readout failure: " << error.what_without_backtrace()
              << '\n';
  } catch (const std::exception &error) {
    std::cerr << "shadow-readout failure: " << error.what() << '\n';
  }
  return 1;
}
