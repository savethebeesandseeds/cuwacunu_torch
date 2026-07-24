#include "jkimyei/training/inference/channel_mdn_conditioned_affine_shadow.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <torch/torch.h>

namespace inference = cuwacunu::jkimyei::training::inference;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace shadow_detail = cuwacunu::jkimyei::training::inference::
    channel_mdn_conditioned_affine_shadow_detail;

namespace {

constexpr std::int64_t kBatch = 146;
constexpr std::int64_t kEdges = 3;
constexpr std::int64_t kNodes = kEdges + 1;
constexpr std::int64_t kChannels = 3;
constexpr std::int64_t kArtifactFeatureDim = 384;
constexpr std::int64_t kLiveReadoutDim = 400;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &message) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, message);
}

void write_binary(const std::filesystem::path &path,
                  const std::string &payload) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  check(output.is_open(), "failed to open binary fixture");
  output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  check(output.good(), "failed to write binary fixture");
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  check(input.is_open(), "failed to open report fixture");
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

mdn::PerEdgeConditionedAffineReturnHeadState make_state() {
  const auto f32 = torch::TensorOptions().dtype(torch::kFloat32);
  const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
  return {
      .feature_mean = torch::zeros({kArtifactFeatureDim}, f32),
      .feature_inv_std = torch::ones({kArtifactFeatureDim}, f32),
      .edge_center = torch::zeros({kEdges, kArtifactFeatureDim}, f64),
      .mapped_weights = torch::zeros({kEdges, kArtifactFeatureDim}, f64),
      .centered_bias = torch::tensor({0.01, 0.02, 0.03}, f64),
  };
}

mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata
make_metadata(const std::string &representation_sha,
              const std::string &mdn_sha) {
  mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = kArtifactFeatureDim;
  metadata.edge_count = kEdges;
  metadata.channel_count = kChannels;
  metadata.conditioning_alpha = 1.0e-10;
  metadata.fit_begin = 0;
  metadata.fit_end = 554;
  metadata.purge_begin = 554;
  metadata.purge_end = 584;
  metadata.validation_begin = 584;
  metadata.validation_end = 730;
  metadata.max_anchor = 729;
  metadata.graph_order_fingerprint = "conditioned-shadow-test-graph";
  metadata.edge_ids = {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  metadata.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  metadata.source_probe_schema_id =
      "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1";
  metadata.source_probe_sha256 = std::string(64, 'a');
  metadata.conditioning_probe_schema_id =
      "synthetic_mdn_frozen_affine_conditioning_parity_v1";
  metadata.conditioning_probe_sha256 = std::string(64, 'b');
  metadata.representation_checkpoint_sha256 = representation_sha;
  metadata.mdn_checkpoint_sha256 = mdn_sha;
  metadata.conditioning_transform_schema_id =
      "full_rank_damped_conditioning_transform.v1";
  metadata.conditioning_transform_sha256 = std::string(64, 'c');
  return metadata;
}

inference::channel_mdn_conditioned_affine_shadow_identity_t
make_identity(const std::filesystem::path &representation_checkpoint,
              const std::filesystem::path &mdn_checkpoint,
              const std::filesystem::path &forbidden_path = {}) {
  inference::channel_mdn_conditioned_affine_shadow_identity_t identity;
  identity.graph_order_fingerprint = "conditioned-shadow-test-graph";
  identity.node_ids = {"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"};
  identity.edge_ids = {"SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  identity.target_coords = {3};
  identity.channel_count = kChannels;
  identity.readout_feature_dim = kLiveReadoutDim;
  identity.requested_anchor_index_begin = 584;
  identity.requested_anchor_index_end = 730;
  identity.representation_checkpoint_path = representation_checkpoint;
  identity.mdn_checkpoint_path = mdn_checkpoint;
  identity.device = torch::Device(torch::kCPU);
  if (!forbidden_path.empty()) {
    identity.forbidden_report_paths.push_back(forbidden_path);
  }
  return identity;
}

inference::channel_mdn_conditioned_affine_shadow_options_t
make_options(const std::filesystem::path &artifact,
             const std::string &artifact_file_sha,
             const std::string &artifact_semantic_digest,
             const std::filesystem::path &representation_checkpoint,
             const std::string &representation_sha,
             const std::filesystem::path &mdn_checkpoint,
             const std::string &mdn_sha, const std::filesystem::path &report) {
  return {
      .artifact_path = artifact,
      .artifact_file_sha256 = artifact_file_sha,
      .artifact_semantic_digest = artifact_semantic_digest,
      .expected_representation_checkpoint_path = representation_checkpoint,
      .expected_representation_checkpoint_sha256 = representation_sha,
      .expected_mdn_checkpoint_path = mdn_checkpoint,
      .expected_mdn_checkpoint_sha256 = mdn_sha,
      .expected_source_cursor_token = "sealed-source-cursor-token",
      .report_path = report,
      .close_feature_coord = 3,
  };
}

void test_gates_suffix_metrics_and_atomic_report() {
  const auto temp_dir = std::filesystem::temp_directory_path() /
                        "cuwacunu_conditioned_affine_shadow_test";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  const auto representation_checkpoint = temp_dir / "representation.pt";
  const auto mdn_checkpoint = temp_dir / "mdn.pt";
  const auto artifact = temp_dir / "conditioned_affine.pt";
  write_binary(representation_checkpoint, "sealed representation checkpoint");
  write_binary(mdn_checkpoint, "sealed MDN checkpoint");
  const auto representation_sha = shadow_detail::sha256_regular_file(
      representation_checkpoint, "representation checkpoint test fixture");
  const auto mdn_sha = shadow_detail::sha256_regular_file(
      mdn_checkpoint, "MDN checkpoint test fixture");

  const mdn::PerEdgeConditionedAffineReturnHeadOptions head_options{
      .feature_dim = kArtifactFeatureDim,
      .edge_count = kEdges,
      .channel_count = kChannels,
  };
  auto head =
      mdn::PerEdgeConditionedAffineReturnHead(head_options, make_state());
  auto metadata = make_metadata(representation_sha, mdn_sha);
  const auto semantic_digest =
      mdn::conditioned_affine_semantic_tensor_digest(head, metadata);
  mdn::save_per_edge_conditioned_affine_return_head(artifact, head, metadata);
  const auto artifact_file_sha =
      shadow_detail::sha256_regular_file(artifact, "artifact test fixture");

  auto options = make_options(
      artifact, artifact_file_sha, semantic_digest, representation_checkpoint,
      representation_sha, mdn_checkpoint, mdn_sha, temp_dir / "shadow.report");
  auto identity = make_identity(representation_checkpoint, mdn_checkpoint);

  auto bad_options = options;
  bad_options.report_path = temp_dir / "bad_artifact_hash.report";
  bad_options.artifact_file_sha256 = std::string(64, 'f');
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 identity);
      },
      "artifact file hash mismatch rejected");

  bad_options = options;
  bad_options.report_path = temp_dir / "bad_checkpoint_hash.report";
  bad_options.expected_mdn_checkpoint_sha256 = std::string(64, 'e');
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 identity);
      },
      "checkpoint hash mismatch rejected");

  auto bad_identity = identity;
  bad_identity.requested_anchor_index_begin = 583;
  bad_options = options;
  bad_options.report_path = temp_dir / "bad_range.report";
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 bad_identity);
      },
      "non-validation range rejected");

  bad_identity = identity;
  bad_identity.graph_order_fingerprint = "different-graph";
  bad_options = options;
  bad_options.report_path = temp_dir / "bad_graph.report";
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 bad_identity);
      },
      "graph identity mismatch rejected");

  bad_identity = make_identity(representation_checkpoint, mdn_checkpoint,
                               temp_dir / "collision.report");
  bad_options = options;
  bad_options.report_path = temp_dir / "collision.report";
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 bad_identity);
      },
      "report path collision rejected");

  bad_options = options;
  bad_options.report_path = temp_dir / "bad_source.report";
  inference::channel_mdn_conditioned_affine_shadow_t bad_source(bad_options,
                                                                identity);
  expect_throw(
      [&] { bad_source.validate_source_cursor("different-source-token"); },
      "source cursor mismatch rejected");

  bad_options = options;
  bad_options.report_path = temp_dir / "wrong_close_coord.report";
  bad_options.close_feature_coord = 4;
  bad_identity = identity;
  bad_identity.target_coords = {3, 4};
  expect_throw(
      [&] {
        (void)inference::channel_mdn_conditioned_affine_shadow_t(bad_options,
                                                                 bad_identity);
      },
      "mislabelled close coordinate rejected");

  inference::channel_mdn_conditioned_affine_shadow_t shadow(options, identity);
  shadow.validate_source_cursor("sealed-source-cursor-token");
  auto readout = torch::zeros({kBatch, kEdges, kChannels, kLiveReadoutDim},
                              torch::TensorOptions().dtype(torch::kFloat32));
  readout.narrow(3, kArtifactFeatureDim, kLiveReadoutDim - kArtifactFeatureDim)
      .fill_(12345.0f);
  auto future = torch::zeros({kBatch, kNodes, kChannels, 1},
                             torch::TensorOptions().dtype(torch::kFloat32));
  for (std::int64_t edge = 0; edge < kEdges; ++edge) {
    future.select(1, edge + 1).fill_(0.01f * static_cast<float>(edge + 1));
  }
  auto combined_mask = torch::ones({kBatch, kNodes, kChannels, 1},
                                   torch::TensorOptions().dtype(torch::kBool));

  auto incomplete_options = options;
  incomplete_options.report_path = temp_dir / "incomplete_coverage.report";
  inference::channel_mdn_conditioned_affine_shadow_t incomplete_shadow(
      incomplete_options, identity);
  incomplete_shadow.validate_source_cursor("sealed-source-cursor-token");
  auto incomplete_mask = combined_mask.clone();
  incomplete_mask.index_put_({0, 1, 0, 0}, false);
  incomplete_shadow.observe(readout, future, incomplete_mask,
                            identity.target_coords,
                            /*begin_anchor_index=*/584,
                            /*anchor_count=*/kBatch);
  expect_throw(
      [&] {
        (void)incomplete_shadow.report_text(
            /*canonical_steps_attempted=*/1,
            /*canonical_steps_completed=*/1,
            /*canonical_streamed_anchor_count=*/kBatch);
      },
      "incomplete target coverage rejected");

  shadow.observe(readout, future, combined_mask, identity.target_coords,
                 /*begin_anchor_index=*/584, /*anchor_count=*/kBatch);

  const auto report = shadow.report_text(/*canonical_steps_attempted=*/1,
                                         /*canonical_steps_completed=*/1,
                                         /*canonical_streamed_anchor_count=*/
                                         kBatch);
  check(
      report.find(std::string("schema_id=") +
                  inference::kChannelMdnConditionedAffineShadowReportSchema) !=
          std::string::npos,
      "shadow report schema");
  check(report.find("artifact.feature_dim=384") != std::string::npos &&
            report.find("runtime.readout_feature_dim=400") != std::string::npos,
        "384-of-400 prefix contract accepted");
  check(report.find("metrics.valid_count=1314") != std::string::npos,
        "shadow report valid count");
  check(report.find("canonical_output_mutated=false") != std::string::npos &&
            report.find("inference_batch_observer_exposed=false") !=
                std::string::npos,
        "shadow isolation declarations");

  shadow.write_report_atomic(/*canonical_steps_attempted=*/1,
                             /*canonical_steps_completed=*/1,
                             /*canonical_streamed_anchor_count=*/kBatch);
  check(std::filesystem::exists(options.report_path),
        "atomic shadow report installed");
  check(!std::filesystem::exists(options.report_path.string() + ".tmp"),
        "atomic shadow report temporary removed");
  check(read_text(options.report_path) == report,
        "atomic shadow report exact payload");

  auto appeared_options = options;
  appeared_options.report_path = temp_dir / "appeared_after_preflight.report";
  inference::channel_mdn_conditioned_affine_shadow_t appeared_shadow(
      appeared_options, identity);
  appeared_shadow.validate_source_cursor("sealed-source-cursor-token");
  appeared_shadow.observe(readout, future, combined_mask,
                          identity.target_coords,
                          /*begin_anchor_index=*/584,
                          /*anchor_count=*/kBatch);
  write_binary(appeared_options.report_path, "do-not-clobber");
  expect_throw(
      [&] {
        appeared_shadow.write_report_atomic(
            /*canonical_steps_attempted=*/1,
            /*canonical_steps_completed=*/1,
            /*canonical_streamed_anchor_count=*/kBatch);
      },
      "report appearing after construction rejected");
  check(read_text(appeared_options.report_path) == "do-not-clobber",
        "appeared report was not clobbered");

  expect_throw(
      [&] {
        shadow.write_report_atomic(/*canonical_steps_attempted=*/1,
                                   /*canonical_steps_completed=*/1,
                                   /*canonical_streamed_anchor_count=*/kBatch);
      },
      "shadow report no-clobber enforced");

  std::filesystem::remove_all(temp_dir);
}

} // namespace

int main() {
  try {
    test_gates_suffix_metrics_and_atomic_report();
    std::cout << "[PASS] wikimyei MDN conditioned affine shadow\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] wikimyei MDN conditioned affine shadow: "
              << error.what() << '\n';
    return 1;
  }
}
