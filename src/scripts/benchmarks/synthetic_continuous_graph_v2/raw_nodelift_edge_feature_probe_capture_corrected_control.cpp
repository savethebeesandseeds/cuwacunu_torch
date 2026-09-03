// SPDX-License-Identifier: MIT
// Phase 2B development-only corrected-control raw NodeLift feature capture.

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include <torch/torch.h>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "piaabo/digest/sha256.h"
#include "ujcamei/source/registry/types/data.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h"

namespace fs = std::filesystem;
namespace protocol = cuwacunu::kikijyeba::protocol;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
namespace digest = cuwacunu::piaabo::digest;

namespace {

constexpr int64_t kTrainBegin = 0;
constexpr int64_t kTrainEnd = 2496;
constexpr int64_t kValidationBegin = 2560;
constexpr int64_t kValidationEnd = 2816;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kHistoryLength = 30;
constexpr int64_t kFeatureWidth = 9;
constexpr int64_t kCloseCoordinate = 3;
constexpr int64_t kPaddedHistoryWidth = 32;
constexpr int64_t kProbeFeatureWidth = 3 * kPaddedHistoryWidth;
constexpr std::array<int64_t, kChannelCount> kConfiguredHistoryCapacities{4, 10, 30};

constexpr std::string_view kProbeHeader =
    "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
    "edge_id,base_node_id,quote_node_id,channel_index,"
    "target_edge_close_return,feature_count,feature_values";
constexpr std::string_view kRecordSchema =
    "kikijyeba.synthetic.raw_nodelift_edge_feature_probe.corrected_control.v1";
constexpr std::string_view kReportSchema =
    "synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_capture_v1";
constexpr std::string_view kMaskStreamSchema =
    "synthetic_v2_nodelift_close_mask_v1";
constexpr std::string_view kSealedConfigFilename =
    "synthetic_benchmark.frozen_feature_capture.isolated.config";

struct Options {
  fs::path config_path;
  fs::path output_probe;
  fs::path output_report;
  int64_t begin{-1};
  int64_t end{-1};
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

void reject_symlink_components(const fs::path &path, bool include_leaf,
                               const char *label) {
  if (!path.is_absolute() || path.lexically_normal() != path) {
    throw std::runtime_error(std::string(label) +
                             " must be an absolute lexically-clean path");
  }
  const auto checked_path = include_leaf ? path : path.parent_path();
  fs::path current = checked_path.root_path();
  for (const auto &component : checked_path.relative_path()) {
    current /= component;
    std::error_code error;
    const auto status = fs::symlink_status(current, error);
    if (!error && fs::is_symlink(status)) {
      throw std::runtime_error(std::string(label) +
                               " traverses a symlink: " + current.string());
    }
  }
}

[[nodiscard]] std::string read_sealed_config(const fs::path &path) {
  reject_symlink_components(path, true, "config path");
  std::error_code error;
  const auto status = fs::symlink_status(path, error);
  if (error || !fs::is_regular_file(status) || fs::is_symlink(status)) {
    throw std::runtime_error(
        "config must be an existing regular non-symlinked file");
  }
  if (path.filename().string() != kSealedConfigFilename) {
    throw std::runtime_error(
        "config is not the sealed isolated feature-capture configuration");
  }
  constexpr auto writable = fs::perms::owner_write | fs::perms::group_write |
                            fs::perms::others_write;
  if ((status.permissions() & writable) != fs::perms::none) {
    throw std::runtime_error("sealed isolated config is writable");
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open sealed isolated config");
  }
  std::ostringstream contents;
  contents << input.rdbuf();
  if (!input.good() && !input.eof()) {
    throw std::runtime_error("failed while reading sealed isolated config");
  }
  const auto text = contents.str();
  if (text.empty()) {
    throw std::runtime_error("sealed isolated config is empty");
  }
  if (text.find("/data/raw") != std::string::npos ||
      text.find("/data/final") != std::string::npos) {
    throw std::runtime_error("sealed isolated config exposes forbidden data");
  }
  return text;
}

void validate_new_output(const fs::path &path, const char *label) {
  reject_symlink_components(path, false, label);
  std::error_code error;
  const auto status = fs::symlink_status(path, error);
  if (!error && status.type() != fs::file_type::not_found) {
    throw std::runtime_error(std::string(label) + " must be absent: " +
                             path.string());
  }
  const auto parent = path.parent_path();
  const auto parent_status = fs::symlink_status(parent, error);
  if (error || !fs::is_directory(parent_status) ||
      fs::is_symlink(parent_status)) {
    throw std::runtime_error(std::string(label) +
                             " parent must be an existing non-symlinked "
                             "directory");
  }
}

[[nodiscard]] Options parse_options(int argc, char **argv) {
  Options out;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--config") {
      out.config_path = require_value(argc, argv, &index, argument);
    } else if (argument == "--output-probe") {
      out.output_probe = require_value(argc, argv, &index, argument);
    } else if (argument == "--output-report") {
      out.output_report = require_value(argc, argv, &index, argument);
    } else if (argument == "--anchor-index-begin") {
      out.begin =
          parse_i64(require_value(argc, argv, &index, argument), argument);
    } else if (argument == "--anchor-index-end") {
      out.end =
          parse_i64(require_value(argc, argv, &index, argument), argument);
    } else if (argument == "--input-representation-checkpoint" ||
               argument == "--input-mdn-checkpoint" ||
               argument == "--input-policy-checkpoint" ||
               argument == "--checkpoint") {
      throw std::runtime_error("checkpoint inputs are forbidden: " + argument);
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  if (out.config_path.empty() || out.output_probe.empty() ||
      out.output_report.empty()) {
    throw std::runtime_error(
        "required: --config ABS --output-probe ABS --output-report ABS "
        "--anchor-index-begin N --anchor-index-end N");
  }
  const bool train = out.begin == kTrainBegin && out.end == kTrainEnd;
  const bool validation =
      out.begin == kValidationBegin && out.end == kValidationEnd;
  if (!train && !validation) {
    throw std::runtime_error(
        "only exact development train [0,2496) or validation [2560,2816) "
        "is allowed");
  }
  if (!out.config_path.is_absolute() || !out.output_probe.is_absolute() ||
      !out.output_report.is_absolute()) {
    throw std::runtime_error("all paths must be absolute");
  }
  out.config_path = out.config_path.lexically_normal();
  out.output_probe = out.output_probe.lexically_normal();
  out.output_report = out.output_report.lexically_normal();
  if (out.config_path == out.output_probe ||
      out.config_path == out.output_report ||
      out.output_probe == out.output_report) {
    throw std::runtime_error("config, probe, and report paths must be distinct");
  }
  validate_new_output(out.output_probe, "output probe");
  validate_new_output(out.output_report, "output report");
  return out;
}

class ExclusiveOutput {
public:
  explicit ExclusiveOutput(fs::path path) : path_(std::move(path)) {
    descriptor_ = ::open(path_.c_str(),
                         O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                         0600);
    if (descriptor_ < 0) {
      throw std::runtime_error("exclusive output creation failed for " +
                               path_.string() + ": " + std::strerror(errno));
    }
  }

  ExclusiveOutput(const ExclusiveOutput &) = delete;
  ExclusiveOutput &operator=(const ExclusiveOutput &) = delete;

  ~ExclusiveOutput() {
    if (descriptor_ >= 0) {
      (void)::close(descriptor_);
    }
    if (!committed_) {
      (void)::unlink(path_.c_str());
    }
  }

  void write_all(const std::string &contents) {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written =
          ::write(descriptor_, contents.data() + offset,
                  contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error("exclusive output write failed for " +
                                 path_.string() + ": " +
                                 std::strerror(errno));
      }
      offset += static_cast<std::size_t>(written);
    }
  }

  void finish_io() {
    if (::fsync(descriptor_) != 0) {
      throw std::runtime_error("exclusive output fsync failed for " +
                               path_.string() + ": " + std::strerror(errno));
    }
    const int descriptor = descriptor_;
    descriptor_ = -1;
    if (::close(descriptor) != 0) {
      throw std::runtime_error("exclusive output close failed for " +
                               path_.string() + ": " + std::strerror(errno));
    }
  }

  void commit() { committed_ = true; }

private:
  fs::path path_;
  int descriptor_{-1};
  bool committed_{false};
};

void validate_cursor(const auto &lifted, std::size_t expected_begin) {
  if (lifted.cursor.anchor_indices.size() != lifted.cursor.anchor_count() ||
      lifted.cursor.begin_anchor_index != expected_begin) {
    throw std::runtime_error(
        "lifted cursor does not begin at the next expected anchor");
  }
  for (std::size_t index = 0; index < lifted.cursor.anchor_indices.size();
       ++index) {
    if (lifted.cursor.anchor_indices[index] != expected_begin + index) {
      throw std::runtime_error(
          "lifted cursor is not an exact contiguous sequential range");
    }
  }
  if (lifted.cursor.end_anchor_index !=
      expected_begin + lifted.cursor.anchor_count()) {
    throw std::runtime_error(
        "lifted cursor end does not match its contiguous range");
  }
}

void validate_input_contract(const mtf::mtf_channel_node_input_t &input) {
  if (!input.data.defined() || !input.feature_mask.defined() ||
      input.data.dim() != 4 || input.feature_mask.sizes() != input.data.sizes() ||
      input.B_anchor <= 0 || input.N != kNodeCount ||
      input.data.size(0) != input.B_anchor * input.N ||
      input.data.size(1) != kChannelCount ||
      input.data.size(2) != kHistoryLength ||
      input.data.size(3) != kFeatureWidth ||
      input.data.scalar_type() != torch::kFloat32 ||
      input.node_ids.size() != static_cast<std::size_t>(kNodeCount) ||
      !input.anchor_keys.defined() || input.anchor_keys.dim() != 1 ||
      input.anchor_keys.size(0) != input.B_anchor) {
    throw std::runtime_error("raw mtf_channel_node_input contract mismatch");
  }
}

struct CloseMaskDiagnostics {
  std::array<int64_t, kChannelCount> observed_min{
      std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::max()};
  std::array<int64_t, kChannelCount> observed_max{0, 0, 0};
  int64_t observed_rows{0};
  std::string split{};
  std::string canonical_stream{};
};

[[nodiscard]] CloseMaskDiagnostics make_close_mask_diagnostics(
    std::string split, int64_t begin, int64_t end) {
  CloseMaskDiagnostics out;
  out.split = std::move(split);
  out.canonical_stream = std::string(kMaskStreamSchema) + "\n" +
                         "split=" + out.split + "\n" + "anchor_range=[" +
                         std::to_string(begin) + ',' + std::to_string(end) +
                         ")\n";
  return out;
}

void validate_close_masks(const mtf::mtf_channel_node_input_t &input,
                          const torch::Tensor &data,
                          const torch::Tensor &mask,
                          int64_t absolute_anchor_begin,
                          CloseMaskDiagnostics *diagnostics) {
  if (diagnostics == nullptr) {
    throw std::runtime_error("close-mask diagnostics sink is required");
  }
  const auto anchor_index = input.anchor_index.to(torch::kCPU)
                                .to(torch::kInt64)
                                .contiguous();
  const auto node_index = input.node_index.to(torch::kCPU)
                              .to(torch::kInt64)
                              .contiguous();
  for (int64_t b = 0; b < input.B_anchor; ++b) {
    for (int64_t node = 0; node < input.N; ++node) {
      const int64_t row = b * input.N + node;
      if (anchor_index.index({row}).item<int64_t>() != b ||
          node_index.index({row}).item<int64_t>() != node) {
        throw std::runtime_error(
            "mtf_channel_node_input row index is not anchor-major/node-minor");
      }
      for (int64_t channel = 0; channel < kChannelCount; ++channel) {
        const int64_t capacity =
            kConfiguredHistoryCapacities[static_cast<std::size_t>(channel)];
        const int64_t structural_prefix = kHistoryLength - capacity;
        int64_t observed = 0;
        std::string bits;
        bits.reserve(static_cast<std::size_t>(kHistoryLength));
        for (int64_t history = 0; history < kHistoryLength; ++history) {
          const bool valid =
              mask.index({row, channel, history, kCloseCoordinate}).item<bool>();
          const double value =
              data.index({row, channel, history, kCloseCoordinate})
                  .item<double>();
          bits.push_back(valid ? '1' : '0');
          if (history < structural_prefix && valid) {
            throw std::runtime_error(
                "close-coordinate mask is active in configured structural "
                "history padding");
          }
          if (valid) {
            ++observed;
            if (!std::isfinite(value)) {
              throw std::runtime_error(
                  "non-finite value under an active close-coordinate mask");
            }
          } else if (value != 0.0) {
            throw std::runtime_error(
                "masked close-coordinate value was not zero-filled");
          }
        }
        const auto index = static_cast<std::size_t>(channel);
        diagnostics->observed_min[index] =
            std::min(diagnostics->observed_min[index], observed);
        diagnostics->observed_max[index] =
            std::max(diagnostics->observed_max[index], observed);
        diagnostics->canonical_stream +=
            std::to_string(absolute_anchor_begin + b) + ',' +
            std::to_string(node) + ',' + std::to_string(channel) + ',' + bits +
            '\n';
      }
      ++diagnostics->observed_rows;
    }
  }
}

[[nodiscard]] std::array<double, kPaddedHistoryWidth>
padded_close_history(const torch::Tensor &data, const torch::Tensor &mask,
                     int64_t row, int64_t channel) {
  std::array<double, kPaddedHistoryWidth> out{};
  constexpr int64_t left_padding = kPaddedHistoryWidth - kHistoryLength;
  for (int64_t history = 0; history < kHistoryLength; ++history) {
    const bool valid =
        mask.index({row, channel, history, kCloseCoordinate}).item<bool>();
    out[static_cast<std::size_t>(left_padding + history)] =
        valid ? data.index({row, channel, history, kCloseCoordinate})
                    .item<double>()
              : 0.0;
  }
  return out;
}

[[nodiscard]] std::array<double, kProbeFeatureWidth>
raw_feature_vector(const torch::Tensor &data, const torch::Tensor &mask,
                   int64_t base_row, int64_t quote_row, int64_t channel) {
  const auto base = padded_close_history(data, mask, base_row, channel);
  const auto quote = padded_close_history(data, mask, quote_row, channel);
  std::array<double, kProbeFeatureWidth> out{};
  for (std::size_t feature = 0; feature < base.size(); ++feature) {
    out[feature] = base[feature];
    out[base.size() + feature] = quote[feature];
    out[2 * base.size() + feature] = base[feature] - quote[feature];
  }
  return out;
}

void emit_feature_values(std::ostream &output,
                         const std::array<double, kProbeFeatureWidth> &values) {
  for (std::size_t feature = 0; feature < values.size(); ++feature) {
    if (feature != 0) {
      output << ';';
    }
    output << values[feature];
  }
}

template <typename KeyT>
int64_t append_batch(std::ostringstream &probe,
                     const mtf::mtf_channel_node_input_t &input,
                     const cuwacunu::wikimyei::expression::nodelift::srl::stream::
                         node_lifted_batch_t<KeyT> &lifted,
                     CloseMaskDiagnostics *close_mask_diagnostics) {
  validate_input_contract(input);
  if (lifted.edge_ids.size() != static_cast<std::size_t>(kEdgeCount) ||
      lifted.node_ids != input.node_ids ||
      !lifted.future_node_features.defined() ||
      !lifted.future_node_mask.defined() ||
      lifted.future_node_features.dim() != 5 ||
      lifted.future_node_mask.sizes() != lifted.future_node_features.sizes() ||
      lifted.future_node_features.size(0) != input.B_anchor ||
      lifted.future_node_features.size(1) != kChannelCount ||
      lifted.future_node_features.size(2) != 1 ||
      lifted.future_node_features.size(3) != kNodeCount ||
      lifted.future_node_features.size(4) != kFeatureWidth ||
      lifted.future_node_features.scalar_type() != torch::kFloat32) {
    throw std::runtime_error("future raw NodeLift target contract mismatch");
  }

  const auto data =
      input.data.detach().to(torch::kCPU).to(torch::kFloat64).contiguous();
  const auto mask = input.feature_mask.detach()
                        .to(torch::kCPU)
                        .to(torch::kBool)
                        .contiguous();
  const auto future = lifted.future_node_features.detach()
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  const auto future_mask = lifted.future_node_mask.detach()
                               .to(torch::kCPU)
                               .to(torch::kBool)
                               .contiguous();
  const auto anchor_keys = input.anchor_keys.detach()
                               .to(torch::kCPU)
                               .to(torch::kInt64)
                               .contiguous();
  validate_close_masks(input, data, mask,
                       static_cast<int64_t>(lifted.cursor.begin_anchor_index),
                       close_mask_diagnostics);

  constexpr int64_t quote_node = 0;
  int64_t rows = 0;
  for (int64_t b = 0; b < input.B_anchor; ++b) {
    const auto anchor_key = anchor_keys.index({b}).item<int64_t>();
    const auto anchor_index =
        static_cast<int64_t>(lifted.cursor.begin_anchor_index) + b;
    for (int64_t base_node = 1; base_node < kNodeCount; ++base_node) {
      const int64_t edge_index = base_node - 1;
      const auto &edge_id =
          lifted.edge_ids[static_cast<std::size_t>(edge_index)];
      const auto &base_node_id =
          input.node_ids[static_cast<std::size_t>(base_node)];
      const auto &quote_node_id =
          input.node_ids[static_cast<std::size_t>(quote_node)];
      for (int64_t channel = 0; channel < kChannelCount; ++channel) {
        const bool base_target_valid =
            future_mask
                .index({b, channel, 0, base_node, kCloseCoordinate})
                .template item<bool>();
        const bool quote_target_valid =
            future_mask
                .index({b, channel, 0, quote_node, kCloseCoordinate})
                .template item<bool>();
        if (!base_target_valid || !quote_target_valid) {
          throw std::runtime_error(
              "future base/quote close target is not valid for every "
              "canonical coordinate");
        }
        const double target =
            future.index({b, channel, 0, base_node, kCloseCoordinate})
                .template item<double>() -
            future.index({b, channel, 0, quote_node, kCloseCoordinate})
                .template item<double>();
        if (!std::isfinite(target)) {
          throw std::runtime_error("future base-minus-quote target is non-finite");
        }

        const int64_t base_row = b * kNodeCount + base_node;
        const int64_t quote_row = b * kNodeCount + quote_node;
        const auto features =
            raw_feature_vector(data, mask, base_row, quote_row, channel);
        probe << kRecordSchema << ',' << anchor_key << ',' << anchor_index << ','
              << b << ',' << edge_index << ',' << edge_id << ',' << base_node_id
              << ',' << quote_node_id << ',' << channel << ',' << target << ','
              << kProbeFeatureWidth << ',';
        emit_feature_values(probe, features);
        probe << '\n';
        ++rows;
      }
    }
  }
  return rows;
}

void run(const Options &options) {
  const std::string config_text = read_sealed_config(options.config_path);
  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      options.config_path.string());
  if (!protocol::active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    throw std::runtime_error(
        "sealed capture config does not declare the canonical MTF protocol");
  }
  const auto &mtf_config = bundle.mtf_jepa_mae_vicreg.config;
  if (mtf_config.channel_count != kChannelCount ||
      mtf_config.history_length != kHistoryLength ||
      mtf_config.input_width != kFeatureWidth ||
      mtf_config.dtype != torch::kFloat32) {
    throw std::runtime_error(
        "sealed MTF input contract is not [C=3,Hx=30,F=9,float32]");
  }
  const auto active_channels =
      protocol::graph_first_source_resolution_detail::active_channels(
          bundle.source_dock);
  if (active_channels.size() != static_cast<std::size_t>(kChannelCount)) {
    throw std::runtime_error(
        "sealed source dock does not have exactly three active channels");
  }
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto configured = protocol::graph_first_dock_parse_positive_i64(
        active_channels[static_cast<std::size_t>(channel)]->input_length,
        "input_length");
    if (configured !=
        kConfiguredHistoryCapacities[static_cast<std::size_t>(channel)]) {
      throw std::runtime_error(
          "sealed source dock history capacities are not exactly 4/10/30");
    }
  }

  bundle.wave_settings.source_range_policy = cuwacunu::hero::runtime::settings::
      wave_source_range_policy_t::anchor_index;
  bundle.wave_settings.source_order_policy = cuwacunu::hero::runtime::settings::
      wave_source_order_policy_t::sequential;
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

  std::ostringstream probe;
  probe.imbue(std::locale::classic());
  probe << std::setprecision(17) << kProbeHeader << '\n';
  int64_t streamed_anchors = 0;
  int64_t probe_rows = 0;
  int64_t lifted_batches = 0;
  std::size_t next_expected_anchor = static_cast<std::size_t>(options.begin);
  std::string graph_order_fingerprint;
  const std::string split = options.begin == kTrainBegin ? "train" : "validation";
  CloseMaskDiagnostics close_mask_diagnostics =
      make_close_mask_diagnostics(split, options.begin, options.end);
  while (lifted_stream.has_next()) {
    auto lifted = lifted_stream.next();
    validate_cursor(lifted, next_expected_anchor);
    if (graph_order_fingerprint.empty()) {
      graph_order_fingerprint = lifted.graph_order_fingerprint;
    } else if (lifted.graph_order_fingerprint != graph_order_fingerprint) {
      throw std::runtime_error(
          "graph order fingerprint changed during raw capture");
    }
    next_expected_anchor += lifted.cursor.anchor_count();
    streamed_anchors += static_cast<int64_t>(lifted.cursor.anchor_count());
    auto input = mtf::make_mtf_channel_node_input(lifted);
    probe_rows +=
        append_batch(probe, input, lifted, &close_mask_diagnostics);
    ++lifted_batches;
  }

  const int64_t expected_anchors = options.end - options.begin;
  const int64_t expected_rows = expected_anchors * kEdgeCount * kChannelCount;
  if (streamed_anchors != expected_anchors ||
      next_expected_anchor != static_cast<std::size_t>(options.end) ||
      probe_rows != expected_rows || lifted_batches <= 0) {
    throw std::runtime_error(
        "raw NodeLift capture did not cover the exact requested cube");
  }
  if (!probe) {
    throw std::runtime_error("failed while rendering raw NodeLift probe");
  }
  if (close_mask_diagnostics.observed_rows !=
      streamed_anchors * kNodeCount) {
    throw std::runtime_error(
        "close-mask diagnostics did not cover every anchor/node row");
  }

  std::ostringstream report;
  report.imbue(std::locale::classic());
  report << "schema_id=" << kReportSchema << '\n';
  report << "status=complete\n";
  report << "benchmark_id=synthetic_continuous_graph_v2\n";
  report << "diagnostic_phase=2B\n";
  report << "diagnostic_authority=development_only\n";
  report << "benchmark_acceptance_authority=false\n";
  report << "config_path=" << options.config_path.string() << '\n';
  report << "config_sha256=" << digest::sha256_hex(config_text) << '\n';
  report << "config_immutable=true\n";
  report << "anchor_range=[" << options.begin << ',' << options.end << ")\n";
  report << "anchor_count=" << streamed_anchors << '\n';
  report << "maximum_anchor_read=" << options.end - 1 << '\n';
  report << "source_order_policy=sequential\n";
  report << "cursor_contiguous=true\n";
  report << "lifted_batch_count=" << lifted_batches << '\n';
  report << "graph_order_fingerprint=" << graph_order_fingerprint << '\n';
  report << "probe_path=" << options.output_probe.string() << '\n';
  report << "probe_record_schema=" << kRecordSchema << '\n';
  report << "probe_header=" << kProbeHeader << '\n';
  report << "probe_rows=" << probe_rows << '\n';
  report << "probe_feature_count=96\n";
  report << "feature_layout=base_32,quote_32,base_minus_quote_32\n";
  report << "row_order=anchor_base_edge_channel\n";
  report << "canonical_coordinate_order=true\n";
  report << "channel_count=3\n";
  report << "node_count=4\n";
  report << "edge_count=3\n";
  report << "history_length=30\n";
  report << "padded_history_width=32\n";
  report << "history_right_aligned=true\n";
  report << "source_dtype=float32\n";
  report << "canonical_target_serialization_dtype=float32\n";
  report << "close_coordinate=3\n";
  report << "structural_padding_contract_passed=true\n";
  report << "variable_close_mask_within_capacity_allowed=true\n";
  report << "masked_close_values_zero=true\n";
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const std::string prefix = "close_mask_count." + split + ".channel_" +
                               std::to_string(channel);
    report << prefix << ".min="
           << close_mask_diagnostics.observed_min[index] << '\n';
    report << prefix << ".max="
           << close_mask_diagnostics.observed_max[index] << '\n';
    report << prefix << ".capacity="
           << kConfiguredHistoryCapacities[index] << '\n';
  }
  report << "close_mask_stream_schema=" << kMaskStreamSchema << '\n';
  report << "close_mask_stream_order=anchor_index,node_index,channel_index\n";
  report << "close_mask_sha256." << split << '='
         << digest::sha256_hex(close_mask_diagnostics.canonical_stream) << '\n';
  report << "future_horizon=1\n";
  report << "target_definition=future_base_minus_quote_close_coordinate\n";
  report << "target_mask_complete=true\n";
  report << "pre_encoder_mtf_channel_node_input_used=true\n";
  report << "representation_config_parsed_as_inert_dependency=true\n";
  report << "representation_model_constructed=false\n";
  report << "representation_checkpoint_access=false\n";
  report << "representation_execution=false\n";
  report << "mdn_config_parsed_as_inert_dependency=true\n";
  report << "mdn_model_constructed=false\n";
  report << "mdn_checkpoint_access=false\n";
  report << "mdn_execution=false\n";
  report << "policy_config_parsed_as_inert_dependency=true\n";
  report << "policy_model_constructed=false\n";
  report << "policy_checkpoint_access=false\n";
  report << "policy_execution=false\n";
  report << "checkpoint_cli_accepted=false\n";
  report << "optimizer_steps=0\n";
  report << "checkpoint_written=false\n";
  if (!report) {
    throw std::runtime_error("failed while rendering raw NodeLift report");
  }

  ExclusiveOutput probe_output(options.output_probe);
  ExclusiveOutput report_output(options.output_report);
  probe_output.write_all(probe.str());
  report_output.write_all(report.str());
  probe_output.finish_io();
  report_output.finish_io();
  probe_output.commit();
  report_output.commit();
}

void run_self_test() {
  auto data = torch::zeros(
      {2, kChannelCount, kHistoryLength, kFeatureWidth},
      torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  auto mask = torch::zeros(
      {2, kChannelCount, kHistoryLength, kFeatureWidth},
      torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU));

  mtf::mtf_channel_node_input_t input{};
  input.B_anchor = 1;
  input.N = 2;
  input.anchor_index = torch::zeros(
      {2}, torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));
  input.node_index = torch::arange(
      2, torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));

  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const int64_t capacity =
        kConfiguredHistoryCapacities[static_cast<std::size_t>(channel)];
    const int64_t begin = kHistoryLength - capacity;
    const int64_t interior = begin + 1;
    mask.index_put_({0, channel, interior, kCloseCoordinate}, true);
    mask.index_put_({0, channel, kHistoryLength - 1, kCloseCoordinate}, true);
    mask.index_put_({1, channel, kHistoryLength - 1, kCloseCoordinate}, true);
    data.index_put_({0, channel, interior, kCloseCoordinate},
                    1.0 + static_cast<double>(channel));
    data.index_put_({0, channel, kHistoryLength - 1, kCloseCoordinate},
                    2.0 + static_cast<double>(channel));
    data.index_put_({1, channel, kHistoryLength - 1, kCloseCoordinate},
                    5.0 + static_cast<double>(channel));
    if (mask.index({0, channel, begin, kCloseCoordinate}).item<bool>() ||
        data.index({0, channel, begin, kCloseCoordinate}).item<double>() != 0.0) {
      throw std::runtime_error(
          "corrected-control self-test oldest in-capacity cell is not false/zero");
    }
  }

  CloseMaskDiagnostics accepted =
      make_close_mask_diagnostics("train", 0, 1);
  validate_close_masks(input, data, mask, 0, &accepted);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    if (accepted.observed_min[index] != 1 ||
        accepted.observed_max[index] != 2) {
      throw std::runtime_error(
          "corrected-control self-test valid-case count mismatch");
    }
  }
  if (accepted.observed_rows != 2) {
    throw std::runtime_error(
        "corrected-control self-test mask row count mismatch");
  }

  const auto expected_bits = [](int64_t capacity, bool quote_row) {
    std::string bits(static_cast<std::size_t>(kHistoryLength), '0');
    if (quote_row) {
      bits[static_cast<std::size_t>(kHistoryLength - capacity + 1)] = '1';
    }
    bits.back() = '1';
    return bits;
  };
  std::string expected_stream = std::string(kMaskStreamSchema) +
                                "\nsplit=train\nanchor_range=[0,1)\n";
  for (int64_t node = 0; node < 2; ++node) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      expected_stream += "0," + std::to_string(node) + ',' +
                         std::to_string(channel) + ',' +
                         expected_bits(kConfiguredHistoryCapacities[
                                           static_cast<std::size_t>(channel)],
                                       node == 0) +
                         '\n';
    }
  }
  if (accepted.canonical_stream != expected_stream) {
    throw std::runtime_error(
        "corrected-control self-test canonical mask stream mismatch");
  }
  const std::string canonical_hash =
      digest::sha256_hex(accepted.canonical_stream);
  if (!digest::is_sha256_hex(canonical_hash) ||
      canonical_hash != digest::sha256_hex(expected_stream)) {
    throw std::runtime_error(
        "corrected-control self-test canonical mask hash mismatch");
  }

  const auto raw96 = raw_feature_vector(data, mask, 1, 0, 0);
  std::array<double, kProbeFeatureWidth> expected_raw96{};
  expected_raw96[31] = 5.0;
  expected_raw96[32 + 29] = 1.0;
  expected_raw96[32 + 31] = 2.0;
  expected_raw96[64 + 29] = -1.0;
  expected_raw96[64 + 31] = 3.0;
  if (raw96 != expected_raw96) {
    throw std::runtime_error(
        "corrected-control self-test raw96 placement/difference mismatch");
  }
  std::ostringstream serialized;
  serialized.imbue(std::locale::classic());
  serialized << std::setprecision(17);
  emit_feature_values(serialized, raw96);
  std::vector<std::string> cells;
  std::istringstream serialized_input(serialized.str());
  for (std::string cell; std::getline(serialized_input, cell, ';');) {
    cells.push_back(cell);
  }
  if (cells.size() != static_cast<std::size_t>(kProbeFeatureWidth)) {
    throw std::runtime_error(
        "corrected-control self-test raw96 serialization width mismatch");
  }
  for (std::size_t feature = 0; feature < cells.size(); ++feature) {
    std::string expected = "0";
    if (feature == 31) expected = "5";
    if (feature == 32 + 29) expected = "1";
    if (feature == 32 + 31) expected = "2";
    if (feature == 64 + 29) expected = "-1";
    if (feature == 64 + 31) expected = "3";
    if (cells[feature] != expected) {
      throw std::runtime_error(
          "corrected-control self-test raw96 exact serialization mismatch");
    }
  }

  const auto must_reject = [&](const torch::Tensor &candidate_data,
                               const torch::Tensor &candidate_mask,
                               const char *label,
                               std::string_view expected_message) {
    try {
      CloseMaskDiagnostics rejected =
          make_close_mask_diagnostics("train", 0, 1);
      validate_close_masks(input, candidate_data, candidate_mask, 0, &rejected);
    } catch (const std::runtime_error &error) {
      if (std::string_view(error.what()).find(expected_message) !=
          std::string_view::npos) {
        return;
      }
      throw;
    }
    throw std::runtime_error(std::string("corrected-control self-test accepted ") +
                             label);
  };

  auto structural_data = data.clone();
  auto structural_mask = mask.clone();
  const int64_t forbidden_history =
      kHistoryLength - kConfiguredHistoryCapacities[0] - 1;
  structural_mask.index_put_(
      {0, 0, forbidden_history, kCloseCoordinate}, true);
  structural_data.index_put_(
      {0, 0, forbidden_history, kCloseCoordinate}, 1.0);
  must_reject(structural_data, structural_mask,
              "an active structural-padding cell",
              "active in configured structural history padding");

  auto nonzero_masked_data = data.clone();
  auto nonzero_masked_mask = mask.clone();
  const int64_t oldest_in_capacity =
      kHistoryLength - kConfiguredHistoryCapacities[0];
  nonzero_masked_data.index_put_(
      {0, 0, oldest_in_capacity, kCloseCoordinate}, 7.0);
  must_reject(nonzero_masked_data, nonzero_masked_mask,
              "a nonzero masked cell", "was not zero-filled");

  auto nonfinite_data = data.clone();
  const int64_t active_history = oldest_in_capacity + 1;
  nonfinite_data.index_put_(
      {0, 0, active_history, kCloseCoordinate},
      std::numeric_limits<double>::infinity());
  must_reject(nonfinite_data, mask, "a non-finite active cell",
              "non-finite value under an active close-coordinate mask");

  std::cout << "schema_id=synthetic_v2_raw_nodelift_edge_feature_probe_"
               "corrected_control_self_test_v1\n";
  std::cout << "status=passed\n";
  std::cout << "expected_case_count=8\n";
  std::cout << "expected_cases=false_structural_padding,oldest_in_capacity_"
               "false,multiple_true_finite,raw96_placement_and_serialization,"
               "canonical_stream_minmax_hash,reject_outside_capacity,reject_"
               "nonzero_false,reject_nonfinite_true\n";
  std::cout << "observed_canonical_output_sha256=" << canonical_hash << '\n';
  std::cout << "source_binary_binding_required_in_immutable_runner_receipt=true\n";
  std::cout << "project_artifact_access=false\n";
  std::cout << "status_line=corrected-control mask self-test passed\n";
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
      run_self_test();
      return 0;
    }
    for (int index = 1; index < argc; ++index) {
      if (std::string_view(argv[index]) == "--self-test") {
        throw std::runtime_error("--self-test must be used alone");
      }
    }
    run(parse_options(argc, argv));
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "corrected-control raw NodeLift edge feature probe capture: "
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "corrected-control raw NodeLift edge feature probe capture: "
              << error.what() << '\n';
  }
  return 1;
}
