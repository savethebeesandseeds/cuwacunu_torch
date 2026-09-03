// SPDX-License-Identifier: MIT

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <torch/torch.h>

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

struct Options {
  std::string config_path{};
  std::string checkpoint_path{};
  fs::path output_dir{};
  int64_t begin{-1};
  int64_t end{-1};
};

struct PoolSpec {
  const char *artifact_id;
  mtf::mtf_serving_pool_policy_t policy;
};

constexpr std::array<PoolSpec, 4> kPools{{
    {"all_tokens", mtf::mtf_serving_pool_policy_t::all_tokens},
    {"pool_time_tokens", mtf::mtf_serving_pool_policy_t::time_only},
    {"pool_frequency_tokens", mtf::mtf_serving_pool_policy_t::frequency_only},
    {"pool_domain_balanced", mtf::mtf_serving_pool_policy_t::domain_balanced},
}};

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
  if (fs::exists(path)) {
    if (!fs::is_directory(path) || !fs::is_empty(path)) {
      throw std::runtime_error("output directory is not new and empty: " +
                               path.string());
    }
    return;
  }
  fs::create_directories(path);
}

template <typename KeyT>
[[nodiscard]] repstream::channel_representation_batch_t<KeyT>
make_representation_batch(const mtf::mtf_channel_node_input_t &input,
                          const mtf::mtf_serving_pool_output_t &pool,
                          const cuwacunu::wikimyei::expression::nodelift::srl::
                              stream::node_lifted_batch_t<KeyT> &lifted) {
  vicreg::graph_row_index_t row_index{
      .anchor_index = input.anchor_index,
      .node_index = input.node_index,
      .B_anchor = input.B_anchor,
      .N = input.N,
  };
  auto adapted = vicreg::make_channel_representation_batch(
      pool.values, pool.valid_mask, row_index, input.anchor_keys,
      input.node_ids);

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
  for (const auto &pool : kPools) {
    const auto path =
        options.output_dir / (std::string(pool.artifact_id) + ".probe");
    if (fs::exists(path)) {
      throw std::runtime_error("refusing to overwrite probe: " + path.string());
    }
  }

  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path);
  if (!protocol::active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    throw std::runtime_error("active protocol does not use MTF representation");
  }
  if (!bundle.mtf_jepa_mae_vicreg.config.use_frequency_tokens) {
    throw std::runtime_error(
        "serving-pool replay requires the frequency-enabled canonical model");
  }
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

  std::array<int64_t, kPools.size()> row_counts{};
  int64_t encoder_batch_passes = 0;
  int64_t streamed_anchor_count = 0;
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
    streamed_anchor_count += static_cast<int64_t>(lifted.cursor.anchor_count());
    auto input = mtf::make_mtf_channel_node_input(lifted);
    auto tensor_options =
        torch::TensorOptions().dtype(config.dtype).device(config.device);
    auto data = input.data.to(tensor_options);
    auto feature_mask = input.feature_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(config.device));
    torch::NoGradGuard no_grad;
    auto encoded = model->encode(data, feature_mask);
    ++encoder_batch_passes;

    const auto time = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::time_only, config);
    const auto frequency = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::frequency_only, config);
    if (!time.valid_mask.logical_and(frequency.valid_mask)
             .all()
             .template item<bool>()) {
      throw std::runtime_error(
          "canonical capture requires both serving domains for every "
          "sample/channel");
    }

    for (std::size_t index = 0; index < kPools.size(); ++index) {
      const auto &pool_spec = kPools[index];
      const auto selected =
          mtf::select_mtf_serving_pool(encoded, pool_spec.policy, config);
      auto representation_batch =
          make_representation_batch(input, selected, lifted);
      auto mdn_batch = mdnstream::make_channel_mdn_input_batch(
          representation_batch, builder.channel_mdn_adapter_options());
      row_counts[index] +=
          inference_detail::append_representation_edge_feature_probe_rows(
              options.output_dir /
                  (std::string(pool_spec.artifact_id) + ".probe"),
              representation_batch, mdn_batch,
              /*close_feature_index=*/3);
    }
  }

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_rows = expected_anchors * 9;
  if (streamed_anchor_count != expected_anchors) {
    throw std::runtime_error("streamed anchor count does not match range");
  }
  if (next_expected_anchor != static_cast<std::size_t>(options.end)) {
    throw std::runtime_error("streamed cursor does not end at requested range");
  }
  for (const auto rows : row_counts) {
    if (rows != expected_rows) {
      throw std::runtime_error("probe row count does not match range");
    }
  }

  std::ofstream report(options.output_dir / "capture.report", std::ios::trunc);
  if (!report) {
    throw std::runtime_error("failed to create capture report");
  }
  report << "schema_id=synthetic_v2_mtf_serving_pool_capture.v1\n";
  report << "status=complete\n";
  report << "config_path=" << options.config_path << "\n";
  report << "representation_checkpoint_path=" << options.checkpoint_path
         << "\n";
  report << "anchor_range=[" << options.begin << "," << options.end << ")\n";
  report << "anchor_count=" << streamed_anchor_count << "\n";
  report << "maximum_anchor_read=" << options.end - 1 << "\n";
  report << "probe_rows=" << expected_rows << "\n";
  report << "source_order_policy=sequential\n";
  report << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
  report << "encoder_batch_passes=" << encoder_batch_passes << "\n";
  report << "encoder_passes_per_anchor=1\n";
  report << "both_domains_required=true\n";
  report << "both_domains_valid=true\n";
  for (std::size_t index = 0; index < kPools.size(); ++index) {
    report << "pool." << kPools[index].artifact_id << ".policy="
           << mtf::mtf_serving_pool_policy_name(kPools[index].policy) << "\n";
    report << "pool." << kPools[index].artifact_id << ".probe_path="
           << (options.output_dir /
               (std::string(kPools[index].artifact_id) + ".probe"))
                  .string()
           << "\n";
    report << "pool." << kPools[index].artifact_id
           << ".probe_rows=" << row_counts[index] << "\n";
  }
  report << "mdn_model_constructed=false\n";
  report << "mdn_checkpoint_access=false\n";
  report << "mdn_execution=false\n";
  report << "policy_config_parsed_as_inert_dependency=true\n";
  report << "policy_model_constructed=false\n";
  report << "policy_checkpoint_access=false\n";
  report << "policy_execution=false\n";
  report << "policy_metric_access=false\n";
  report << "optimizer_steps=0\n";
  report << "model_state_mutated=false\n";
  report << "checkpoint_written=false\n";
  report.flush();
  if (!report) {
    throw std::runtime_error("failed to flush capture report");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    run(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "cuwacunu_mtf_serving_pool_capture: " << error.what() << "\n";
    return 1;
  }
}
