// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>
#include <unistd.h>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "ujcamei/source/registry/types/data.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"
#include "wikimyei/representation/encoding/vicreg/channel_representation_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_batch.h"

namespace fs = std::filesystem;
namespace protocol = cuwacunu::kikijyeba::protocol;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
namespace repstream =
    cuwacunu::wikimyei::representation::encoding::vicreg::stream;
namespace mdnstream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;
namespace inference_detail = cuwacunu::jkimyei::training::inference::
    channel_graph_first_inference_launcher_detail;

namespace {

constexpr int64_t kTrainBegin = 0;
constexpr int64_t kTrainEnd = 2496;
constexpr int64_t kValidationBegin = 2560;
constexpr int64_t kValidationEnd = 2816;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kDomainCount = 2;
constexpr int64_t kScaleCount = 4;
constexpr int64_t kLatentWidth = 32;
constexpr int64_t kSummaryGroupsPerChannel = kDomainCount * kScaleCount;
constexpr int64_t kSummaryWidth = kSummaryGroupsPerChannel * kLatentWidth;
constexpr int64_t kEdgeFeatureWidth = 3 * kSummaryWidth;
constexpr int64_t kReferenceEdgeFeatureWidth = 3 * kLatentWidth;
constexpr int64_t kExpectedTokenCount = 72;
constexpr std::array<int64_t, kScaleCount> kExpectedWindowsByScale{7, 3, 1, 1};
constexpr std::array<int64_t, kScaleCount> kExpectedTimeScales{8, 16, 32, 64};
constexpr std::array<int64_t, kScaleCount> kExpectedScaleStrides{4, 8, 16, 32};

constexpr const char *kReferenceArtifact = "all_tokens_reference";
constexpr const char *kSummaryArtifact = "prepool_domain_scale";

struct Options {
  std::string config_path{};
  std::string checkpoint_path{};
  fs::path output_dir{};
  int64_t begin{-1};
  int64_t end{-1};
};

struct DomainScaleSummary {
  torch::Tensor values{};     // [M,C,2*4*32] = [M,3,256].
  torch::Tensor valid_mask{}; // [M,C], bool; true iff all eight cells exist.
  int64_t sample_node_count{0};
  int64_t cell_count{0};
  int64_t valid_cell_count{0};
  int64_t minimum_valid_tokens_per_cell{0};
  int64_t maximum_valid_tokens_per_cell{0};
};

struct CaptureCounters {
  int64_t row_count_reference{0};
  int64_t row_count_summary{0};
  int64_t encoder_batch_passes{0};
  int64_t streamed_anchor_count{0};
  int64_t sample_node_count{0};
  int64_t summary_cell_count{0};
  int64_t summary_valid_cell_count{0};
  int64_t minimum_valid_tokens_per_cell{std::numeric_limits<int64_t>::max()};
  int64_t maximum_valid_tokens_per_cell{0};
};

[[nodiscard]] std::string require_value(int argc, char **argv, int *index,
                                        const std::string &flag) {
  if (*index + 1 >= argc) {
    throw std::runtime_error("missing value for " + flag);
  }
  return argv[++(*index)];
}

[[nodiscard]] int64_t parse_i64(const std::string &text,
                                const std::string &flag) {
  std::size_t consumed = 0;
  const auto value = std::stoll(text, &consumed, 10);
  if (consumed != text.size()) {
    throw std::runtime_error("invalid integer for " + flag + ": " + text);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char **argv) {
  Options out{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--config") {
      out.config_path = require_value(argc, argv, &index, argument);
    } else if (argument == "--input-representation-checkpoint") {
      out.checkpoint_path = require_value(argc, argv, &index, argument);
    } else if (argument == "--output-dir") {
      out.output_dir = require_value(argc, argv, &index, argument);
    } else if (argument == "--anchor-index-begin") {
      out.begin =
          parse_i64(require_value(argc, argv, &index, argument), argument);
    } else if (argument == "--anchor-index-end") {
      out.end =
          parse_i64(require_value(argc, argv, &index, argument), argument);
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  if (out.config_path.empty() || out.checkpoint_path.empty() ||
      out.output_dir.empty() || out.begin < 0 || out.end <= out.begin) {
    throw std::runtime_error(
        "required: --config PATH --input-representation-checkpoint PATH "
        "--output-dir PATH --anchor-index-begin N --anchor-index-end N");
  }
  const bool is_train = out.begin == kTrainBegin && out.end == kTrainEnd;
  const bool is_validation =
      out.begin == kValidationBegin && out.end == kValidationEnd;
  if (!is_train && !is_validation) {
    throw std::runtime_error(
        "only the preregistered development train or validation range is "
        "allowed");
  }
  if (!fs::path(out.config_path).is_absolute() ||
      !fs::path(out.checkpoint_path).is_absolute() ||
      !out.output_dir.is_absolute()) {
    throw std::runtime_error("all paths must be absolute");
  }
  return out;
}

void require_new_output_dir(const fs::path &path) {
  if (fs::exists(path) || fs::is_symlink(path)) {
    throw std::runtime_error("output directory must be absent: " +
                             path.string());
  }
  std::error_code error;
  if (!fs::create_directories(path, error) || error) {
    throw std::runtime_error("failed to create output directory: " +
                             path.string() + ": " + error.message());
  }
}

void reserve_exclusive_file(const fs::path &path) {
  const int descriptor =
      ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (descriptor < 0) {
    throw std::runtime_error("output path must be exclusively creatable: " +
                             path.string() + ": " + std::strerror(errno));
  }
  if (::close(descriptor) != 0) {
    const auto message = std::string(std::strerror(errno));
    (void)::unlink(path.c_str());
    throw std::runtime_error("failed to close reserved output: " + message);
  }
}

void write_exclusive_file(const fs::path &path, const std::string &contents) {
  const int descriptor =
      ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (descriptor < 0) {
    throw std::runtime_error("output path must be exclusively creatable: " +
                             path.string() + ": " + std::strerror(errno));
  }

  int open_descriptor = descriptor;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written = ::write(open_descriptor, contents.data() + offset,
                                   contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error("failed while writing exclusive output: " +
                                 std::string(std::strerror(errno)));
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::close(open_descriptor) != 0) {
      open_descriptor = -1;
      throw std::runtime_error("failed while closing exclusive output: " +
                               std::string(std::strerror(errno)));
    }
    open_descriptor = -1;
  } catch (...) {
    if (open_descriptor >= 0) {
      (void)::close(open_descriptor);
    }
    (void)::unlink(path.c_str());
    throw;
  }
}

void validate_fixed_architecture(
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  if (config.channel_count != kChannelCount || config.history_length != 30 ||
      config.latent_dim != kLatentWidth || !config.use_frequency_tokens ||
      config.time_scales.size() != kScaleCount ||
      config.scale_strides.size() != kScaleCount) {
    throw std::runtime_error(
        "capture requires the frozen 3-channel, H30, 4-scale, frequency-"
        "enabled, latent32 architecture");
  }
  for (int64_t scale = 0; scale < kScaleCount; ++scale) {
    const auto index = static_cast<std::size_t>(scale);
    if (config.time_scales[index] != kExpectedTimeScales[index] ||
        config.scale_strides[index] != kExpectedScaleStrides[index]) {
      throw std::runtime_error("frozen scale or stride contract changed");
    }
    const auto windows = mtf::detail::window_plan(config.history_length,
                                                  config.time_scales[index],
                                                  config.scale_strides[index]);
    if (static_cast<int64_t>(windows.size()) !=
        kExpectedWindowsByScale[index]) {
      throw std::runtime_error("frozen token window count changed");
    }
  }
}

[[nodiscard]] DomainScaleSummary make_domain_scale_summary(
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  validate_fixed_architecture(config);
  TORCH_CHECK(encoded.embeddings.defined() && encoded.embeddings.dim() == 3,
              "pre-pool summary requires embeddings [M,N,32]");
  TORCH_CHECK(encoded.token_mask.defined() && encoded.token_mask.dim() == 2,
              "pre-pool summary requires token_mask [M,N]");
  TORCH_CHECK(encoded.embeddings.size(0) == encoded.token_mask.size(0) &&
                  encoded.embeddings.size(1) == encoded.token_mask.size(1) &&
                  encoded.embeddings.size(2) == kLatentWidth,
              "pre-pool embedding and mask shapes differ");
  TORCH_CHECK(encoded.embeddings.size(1) == kExpectedTokenCount,
              "frozen token count must be 72");

  const auto &metadata = encoded.metadata;
  TORCH_CHECK(metadata.channel_id.defined() && metadata.domain_id.defined() &&
                  metadata.scale_id.defined(),
              "pre-pool summary requires channel/domain/scale metadata");
  TORCH_CHECK(metadata.channel_id.dim() == 1 && metadata.domain_id.dim() == 1 &&
                  metadata.scale_id.dim() == 1 &&
                  metadata.channel_id.size(0) == kExpectedTokenCount &&
                  metadata.domain_id.size(0) == kExpectedTokenCount &&
                  metadata.scale_id.size(0) == kExpectedTokenCount,
              "pre-pool metadata shape changed");

  const auto mask = encoded.token_mask.to(torch::kBool);
  const auto channel_ids = metadata.channel_id.to(mask.device());
  const auto domain_ids = metadata.domain_id.to(mask.device());
  const auto scale_ids = metadata.scale_id.to(mask.device());
  std::vector<torch::Tensor> channel_summaries;
  std::vector<torch::Tensor> channel_validity;
  channel_summaries.reserve(kChannelCount);
  channel_validity.reserve(kChannelCount);

  int64_t valid_cell_count = 0;
  int64_t minimum_valid_tokens = std::numeric_limits<int64_t>::max();
  int64_t maximum_valid_tokens = 0;
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    std::vector<torch::Tensor> groups;
    std::vector<torch::Tensor> group_validity;
    groups.reserve(kSummaryGroupsPerChannel);
    group_validity.reserve(kSummaryGroupsPerChannel);
    for (int64_t domain = 0; domain < kDomainCount; ++domain) {
      for (int64_t scale = 0; scale < kScaleCount; ++scale) {
        const auto metadata_selector = channel_ids.eq(channel)
                                           .logical_and(domain_ids.eq(domain))
                                           .logical_and(scale_ids.eq(scale));
        const int64_t metadata_count =
            metadata_selector.sum().template item<int64_t>();
        if (metadata_count !=
            kExpectedWindowsByScale[static_cast<std::size_t>(scale)]) {
          throw std::runtime_error(
              "channel/domain/scale metadata cardinality changed");
        }

        const auto selected =
            mask.logical_and(metadata_selector.unsqueeze(/*dim=*/0));
        const auto counts = selected.sum(/*dim=*/1);
        const auto valid = counts.gt(0);
        if (!valid.all().template item<bool>()) {
          throw std::runtime_error(
              "every sample-node channel/domain/scale cell must be valid");
        }
        const auto weights =
            selected.to(encoded.embeddings.dtype()).unsqueeze(-1);
        const auto mean =
            (encoded.embeddings * weights).sum(/*dim=*/1) /
            counts.to(encoded.embeddings.dtype()).clamp_min(1).unsqueeze(-1);
        groups.push_back(mean);
        group_validity.push_back(valid);
        valid_cell_count += valid.sum().template item<int64_t>();
        minimum_valid_tokens = std::min(minimum_valid_tokens,
                                        counts.min().template item<int64_t>());
        maximum_valid_tokens = std::max(maximum_valid_tokens,
                                        counts.max().template item<int64_t>());
      }
    }
    // Exact feature order: time_s0..time_s3, frequency_s0..frequency_s3;
    // latent coordinate 0..31 is contiguous inside every group.
    channel_summaries.push_back(torch::cat(groups, /*dim=*/1).unsqueeze(1));
    channel_validity.push_back(
        torch::stack(group_validity, /*dim=*/1).all(/*dim=*/1));
  }

  auto values = torch::cat(channel_summaries, /*dim=*/1);
  auto valid_mask = torch::stack(channel_validity, /*dim=*/1);
  TORCH_CHECK(values.sizes() ==
                  torch::IntArrayRef({encoded.embeddings.size(0), kChannelCount,
                                      kSummaryWidth}),
              "pre-pool summary shape must be [M,3,256]");
  TORCH_CHECK(valid_mask.all().template item<bool>(),
              "pre-pool channel validity must be complete");

  const int64_t sample_node_count = encoded.embeddings.size(0);
  const int64_t cell_count =
      sample_node_count * kChannelCount * kSummaryGroupsPerChannel;
  if (valid_cell_count != cell_count) {
    throw std::runtime_error("pre-pool valid-cell counter mismatch");
  }
  return {.values = std::move(values),
          .valid_mask = std::move(valid_mask),
          .sample_node_count = sample_node_count,
          .cell_count = cell_count,
          .valid_cell_count = valid_cell_count,
          .minimum_valid_tokens_per_cell = minimum_valid_tokens,
          .maximum_valid_tokens_per_cell = maximum_valid_tokens};
}

template <typename KeyT>
[[nodiscard]] repstream::channel_representation_batch_t<KeyT>
make_representation_batch(const mtf::mtf_channel_node_input_t &input,
                          const torch::Tensor &values,
                          const torch::Tensor &valid_mask,
                          const cuwacunu::wikimyei::expression::nodelift::srl::
                              stream::node_lifted_batch_t<KeyT> &lifted) {
  vicreg::graph_row_index_t row_index{
      .anchor_index = input.anchor_index,
      .node_index = input.node_index,
      .B_anchor = input.B_anchor,
      .N = input.N,
  };
  auto adapted = vicreg::make_channel_representation_batch(
      values, valid_mask, row_index, input.anchor_keys, input.node_ids);

  repstream::channel_representation_batch_t<KeyT> out{};
  out.node_encoding = std::move(adapted.node_encoding);
  out.node_encoding_mask = std::move(adapted.node_encoding_mask);
  out.anchor_keys = lifted.anchor_keys;
  out.node_ids = lifted.node_ids;
  out.edge_ids = lifted.edge_ids;
  out.graph_order_fingerprint = lifted.graph_order_fingerprint;
  out.cursor = lifted.cursor;
  out.nodelift_stream_report = lifted.stream_report;
  out.nodelift_runtime_lls = lifted.runtime_lls;
  out.future_node_features = lifted.future_node_features;
  out.future_node_mask = lifted.future_node_mask;
  return out;
}

void run(const Options &options) {
  require_new_output_dir(options.output_dir);
  const auto reference_path =
      options.output_dir / (std::string(kReferenceArtifact) + ".probe");
  const auto summary_path =
      options.output_dir / (std::string(kSummaryArtifact) + ".probe");
  const auto report_path = options.output_dir / "capture.report";
  reserve_exclusive_file(reference_path);
  reserve_exclusive_file(summary_path);

  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path);
  if (!protocol::active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    throw std::runtime_error("active protocol does not use MTF representation");
  }
  validate_fixed_architecture(bundle.mtf_jepa_mae_vicreg.config);
  bundle.wave_settings.source_range_policy = cuwacunu::hero::runtime::settings::
      wave_source_range_policy_t::anchor_index;
  bundle.wave_settings.source_order_policy =
      cuwacunu::hero::runtime::settings::wave_source_order_policy_t::sequential;
  bundle.wave_settings.source_order_policy_explicit = true;
  bundle.wave_settings.anchor_index_begin =
      static_cast<std::size_t>(options.begin);
  bundle.wave_settings.anchor_index_end = static_cast<std::size_t>(options.end);
  bundle.wave_settings.source_key_begin = std::nullopt;
  bundle.wave_settings.source_key_end = std::nullopt;
  cuwacunu::hero::runtime::settings::validate_wave_settings(
      bundle.wave_settings);
  protocol::validate_channel_graph_first_config_bundle(bundle);

  protocol::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.compute_alignment_diagnostics = true;
  builder_options.runtime_report_mode =
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::normal;
  protocol::channel_graph_first_pipeline_builder_t<types::kline_t> builder(
      std::move(bundle), builder_options);
  auto source = builder.make_graph_source();
  auto lifted_stream = builder.make_node_lifted_stream(
      std::move(source),
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::normal);

  const auto &config = builder.bundle().mtf_jepa_mae_vicreg.config;
  auto model = mtf::MtfJepaMaeVicreg(config);
  model->to(config.device, config.dtype);
  inference_detail::load_mtf_jepa_mae_vicreg_checkpoint_file(
      options.checkpoint_path, model, builder.options().device,
      config.channel_count, config.history_length, config.input_width,
      config.d_model, config.latent_dim, config.projector_dim);
  inference_detail::freeze_vicreg_encoder(model);
  if (model->is_training()) {
    throw std::runtime_error(
        "representation model did not enter evaluation mode");
  }

  CaptureCounters counters{};
  std::size_t next_expected_anchor = static_cast<std::size_t>(options.begin);
  std::string graph_order_fingerprint{};
  while (lifted_stream.has_next()) {
    auto lifted = lifted_stream.next();
    if (lifted.cursor.anchor_indices.size() != lifted.cursor.anchor_count() ||
        lifted.cursor.begin_anchor_index != next_expected_anchor) {
      throw std::runtime_error(
          "lifted cursor does not begin at the next expected anchor");
    }
    for (std::size_t index = 0; index < lifted.cursor.anchor_indices.size();
         ++index) {
      if (lifted.cursor.anchor_indices[index] != next_expected_anchor + index) {
        throw std::runtime_error(
            "lifted cursor is not an exact contiguous sequential range");
      }
    }
    next_expected_anchor += lifted.cursor.anchor_count();
    if (lifted.cursor.end_anchor_index != next_expected_anchor) {
      throw std::runtime_error(
          "lifted cursor end does not match its contiguous range");
    }
    if (graph_order_fingerprint.empty()) {
      graph_order_fingerprint = lifted.graph_order_fingerprint;
    } else if (lifted.graph_order_fingerprint != graph_order_fingerprint) {
      throw std::runtime_error(
          "graph order fingerprint changed during capture");
    }

    counters.streamed_anchor_count +=
        static_cast<int64_t>(lifted.cursor.anchor_count());
    auto input = mtf::make_mtf_channel_node_input(lifted);
    auto tensor_options =
        torch::TensorOptions().dtype(config.dtype).device(config.device);
    auto data = input.data.to(tensor_options);
    auto feature_mask = input.feature_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config.device));
    torch::NoGradGuard no_grad;
    auto encoded = model->encode(data, feature_mask);
    ++counters.encoder_batch_passes;

    const auto reference = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, config);
    if (!reference.valid_mask.all().template item<bool>()) {
      throw std::runtime_error(
          "all-token reference requires every sample/channel");
    }
    auto summary = make_domain_scale_summary(encoded, config);
    counters.sample_node_count += summary.sample_node_count;
    counters.summary_cell_count += summary.cell_count;
    counters.summary_valid_cell_count += summary.valid_cell_count;
    counters.minimum_valid_tokens_per_cell =
        std::min(counters.minimum_valid_tokens_per_cell,
                 summary.minimum_valid_tokens_per_cell);
    counters.maximum_valid_tokens_per_cell =
        std::max(counters.maximum_valid_tokens_per_cell,
                 summary.maximum_valid_tokens_per_cell);

    auto reference_batch = make_representation_batch(
        input, reference.values, reference.valid_mask, lifted);
    auto reference_mdn_batch = mdnstream::make_channel_mdn_input_batch(
        reference_batch, builder.channel_mdn_adapter_options());
    counters.row_count_reference +=
        inference_detail::append_representation_edge_feature_probe_rows(
            reference_path, reference_batch, reference_mdn_batch,
            /*close_feature_index=*/3);

    auto summary_batch = make_representation_batch(input, summary.values,
                                                   summary.valid_mask, lifted);
    auto summary_mdn_batch = mdnstream::make_channel_mdn_input_batch(
        summary_batch, builder.channel_mdn_adapter_options());
    counters.row_count_summary +=
        inference_detail::append_representation_edge_feature_probe_rows(
            summary_path, summary_batch, summary_mdn_batch,
            /*close_feature_index=*/3);
  }

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_rows = expected_anchors * 9;
  const int64_t expected_encoder_batches = (expected_anchors + 63) / 64;
  if (counters.streamed_anchor_count != expected_anchors ||
      next_expected_anchor != static_cast<std::size_t>(options.end)) {
    throw std::runtime_error("streamed anchor range is incomplete");
  }
  if (counters.row_count_reference != expected_rows ||
      counters.row_count_summary != expected_rows) {
    throw std::runtime_error("probe row count does not match range");
  }
  if (counters.encoder_batch_passes != expected_encoder_batches) {
    throw std::runtime_error("encoder batch count does not match range");
  }
  if (counters.summary_cell_count != counters.summary_valid_cell_count ||
      counters.minimum_valid_tokens_per_cell <= 0) {
    throw std::runtime_error("summary validity counters are incomplete");
  }

  std::ostringstream report;
  report
      << "schema_id=synthetic_v2_frozen_mtf_prepool_domain_scale_capture.v1\n";
  report << "status=complete\n";
  report << "config_path=" << options.config_path << "\n";
  report << "representation_checkpoint_path=" << options.checkpoint_path
         << "\n";
  report << "anchor_range=[" << options.begin << "," << options.end << ")\n";
  report << "anchor_count=" << counters.streamed_anchor_count << "\n";
  report << "maximum_anchor_read=" << options.end - 1 << "\n";
  report << "probe_rows=" << expected_rows << "\n";
  report << "source_order_policy=sequential\n";
  report << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
  report << "checkpoint_load_count=1\n";
  report << "stream_batch_count=" << counters.encoder_batch_passes << "\n";
  report << "encoder_batch_passes=" << counters.encoder_batch_passes << "\n";
  report << "encoder_forward_calls=" << counters.encoder_batch_passes << "\n";
  report << "each_anchor_encoded_once=true\n";
  report << "same_encode_artifact_count=2\n";
  report << "sample_node_count=" << counters.sample_node_count << "\n";
  report << "token_count_per_sample_node=" << kExpectedTokenCount << "\n";
  report << "channel_count=" << kChannelCount << "\n";
  report << "domain_count=" << kDomainCount << "\n";
  report << "scale_count=" << kScaleCount << "\n";
  report << "latent_width=" << kLatentWidth << "\n";
  report << "window_count.scale_0=" << kExpectedWindowsByScale[0] << "\n";
  report << "window_count.scale_1=" << kExpectedWindowsByScale[1] << "\n";
  report << "window_count.scale_2=" << kExpectedWindowsByScale[2] << "\n";
  report << "window_count.scale_3=" << kExpectedWindowsByScale[3] << "\n";
  report << "token_count_per_channel_domain=12\n";
  report << "token_count_per_domain=36\n";
  report << "summary_groups_per_channel=" << kSummaryGroupsPerChannel << "\n";
  report << "summary_layout=domain_major_scale_minor_latent_minor\n";
  report << "summary_group_order=time_s0,time_s1,time_s2,time_s3,"
            "frequency_s0,frequency_s1,frequency_s2,frequency_s3\n";
  report
      << "summary_formula=masked_mean_encoded_tokens_by_channel_domain_scale\n";
  report << "summary_shape=[M,3,256]\n";
  report << "summary_node_channel_width=" << kSummaryWidth << "\n";
  report << "summary_cell_count=" << counters.summary_cell_count << "\n";
  report << "summary_valid_cell_count=" << counters.summary_valid_cell_count
         << "\n";
  report << "summary_all_cells_valid=true\n";
  report << "minimum_valid_tokens_per_cell="
         << counters.minimum_valid_tokens_per_cell << "\n";
  report << "maximum_valid_tokens_per_cell="
         << counters.maximum_valid_tokens_per_cell << "\n";
  report << "edge_feature_layout=base_256,quote_256,base_minus_quote_256\n";
  report << "edge_feature_width=" << kEdgeFeatureWidth << "\n";
  report << "probe_file_creation_policy=exclusive\n";
  report << "capture_report_creation_policy=exclusive\n";
  report << "output.all_tokens_reference.policy=all_tokens\n";
  report << "output.all_tokens_reference.probe_path=" << reference_path.string()
         << "\n";
  report << "output.all_tokens_reference.probe_rows="
         << counters.row_count_reference << "\n";
  report << "output.all_tokens_reference.node_channel_width=" << kLatentWidth
         << "\n";
  report << "output.all_tokens_reference.edge_feature_width="
         << kReferenceEdgeFeatureWidth << "\n";
  report << "output.prepool_domain_scale.policy=channel_domain_scale_mean\n";
  report << "output.prepool_domain_scale.probe_path=" << summary_path.string()
         << "\n";
  report << "output.prepool_domain_scale.probe_rows="
         << counters.row_count_summary << "\n";
  report << "output.prepool_domain_scale.node_channel_width=" << kSummaryWidth
         << "\n";
  report << "output.prepool_domain_scale.edge_feature_width="
         << kEdgeFeatureWidth << "\n";
  report << "mdn_adapter_calls=" << 2 * counters.encoder_batch_passes << "\n";
  report << "mdn_model_constructed=false\n";
  report << "mdn_checkpoint_access=false\n";
  report << "mdn_execution=false\n";
  report << "policy_config_parsed_as_inert_dependency=true\n";
  report << "policy_model_constructed=false\n";
  report << "policy_checkpoint_access=false\n";
  report << "policy_execution=false\n";
  report << "policy_metric_access=false\n";
  report << "optimizer_steps=0\n";
  report << "model_parameter_or_buffer_value_mutated_after_checkpoint_load="
            "false\n";
  report << "checkpoint_written=false\n";
  if (!report) {
    throw std::runtime_error("failed while rendering capture report");
  }
  write_exclusive_file(report_path, report.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    run(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "cuwacunu_mtf_prepool_domain_scale_capture: " << error.what()
              << "\n";
    return 1;
  }
}
