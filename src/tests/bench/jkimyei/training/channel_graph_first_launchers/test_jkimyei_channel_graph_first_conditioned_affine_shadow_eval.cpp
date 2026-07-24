#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "piaabo/log/dlogs.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <torch/torch.h>

namespace protocol = cuwacunu::kikijyeba::protocol;
namespace inference = cuwacunu::jkimyei::training::inference;
namespace runtime = cuwacunu::hero::runtime::settings;
namespace types = cuwacunu::ujcamei::source::registry::types;
namespace shadow_detail = cuwacunu::jkimyei::training::inference::
    channel_mdn_conditioned_affine_shadow_detail;

namespace {

using Kline = types::kline_t;

constexpr std::string_view kSchemaId =
    "synthetic_mdn_channel_graph_first_conditioned_affine_shadow_eval_v1";
constexpr std::string_view kWaveId = "cwu_02v_certified_replay_eval_mdn";
constexpr std::int64_t kValidationBegin = 584;
constexpr std::int64_t kValidationEnd = 730;
constexpr std::int64_t kValidationAnchors = kValidationEnd - kValidationBegin;
constexpr std::int64_t kExpectedBatchCount = 3;
constexpr std::int64_t kExpectedTargetCount = 1314;
constexpr double kContinuousMetricOperationalTolerance = 1.0e-3;
constexpr std::int64_t kDirectionDecisionOperationalTolerance = 1;
constexpr std::int64_t kRankDecisionOperationalTolerance = 2;

constexpr std::string_view kExpectedConfigSha256 =
    "7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6";
constexpr std::string_view kExpectedRepresentationCheckpointSha256 =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr std::string_view kExpectedMdnCheckpointSha256 =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr std::string_view kExpectedArtifactSha256 =
    "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739";
constexpr std::string_view kExpectedArtifactSemanticDigest =
    "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38";
constexpr std::string_view kExpectedSourceCursorToken =
    "version=ujcamei.graph_anchor_cursor_report.v1|graph=d334e38b1887ae16|"
    "reference_edge=SYNALPHASYNUSD|Hx=30|Hf=1|edges=3|accepted=1170|"
    "candidates=1170|skipped=0|first=1580428799999|last=1681430399999";

struct cli_options_t {
  std::filesystem::path config{};
  std::filesystem::path representation_checkpoint{};
  std::filesystem::path mdn_checkpoint{};
  std::filesystem::path artifact{};
  std::filesystem::path work_dir{};
  std::filesystem::path output{};
};

struct captured_batch_t {
  torch::Tensor log_pi{};
  torch::Tensor mu{};
  torch::Tensor sigma{};
  torch::Tensor direct_edge_return{};
  std::int64_t begin_anchor_index{-1};
  std::int64_t anchor_count{0};
  std::int64_t pulse{0};
  std::vector<std::size_t> anchor_indices{};
  std::vector<types::ms_t> anchor_keys{};
};

struct tensor_parity_t {
  std::int64_t batch_count{0};
  bool torch_equal{true};
  bool raw_byte_equal{true};
  double maximum_abs_delta{0.0};
};

struct shadow_metrics_t {
  std::int64_t count{0};
  std::int64_t pair_count{0};
  double mae{std::numeric_limits<double>::quiet_NaN()};
  double rmse{std::numeric_limits<double>::quiet_NaN()};
  double direction{std::numeric_limits<double>::quiet_NaN()};
  double rank{std::numeric_limits<double>::quiet_NaN()};
  double correlation{std::numeric_limits<double>::quiet_NaN()};
  double prediction_std{std::numeric_limits<double>::quiet_NaN()};
};

struct launcher_result_t {
  inference::channel_graph_first_inference_training_report_t report{};
  std::vector<captured_batch_t> batches{};
  std::string canonical_report_text{};
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(
      "[channel_graph_first_conditioned_affine_shadow_eval] " + message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  require(input.is_open(), "failed to open input: " + path.string());
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

std::filesystem::path canonical_input(const std::filesystem::path &path,
                                      const char *name) {
  std::error_code ec;
  const auto canonical = std::filesystem::canonical(path, ec);
  require(!ec && std::filesystem::is_regular_file(canonical, ec) && !ec,
          std::string(name) + " must be an existing regular file");
  return canonical;
}

std::filesystem::path normalized_new_path(const std::filesystem::path &path,
                                          const char *name) {
  require(!path.empty(), std::string(name) + " path is required");
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(path, ec).lexically_normal();
  require(!ec, std::string(name) + " path cannot be made absolute");
  require(!std::filesystem::exists(absolute, ec) && !ec,
          std::string(name) + " path already exists");
  require(!std::filesystem::exists(absolute.string() + ".tmp", ec) && !ec,
          std::string(name) + " temporary path already exists");
  return absolute;
}

std::string file_sha256(const std::filesystem::path &path, const char *name) {
  return shadow_detail::sha256_regular_file(path, name);
}

void require_sha(const std::filesystem::path &path, std::string_view expected,
                 const char *name) {
  require(file_sha256(path, name) == expected,
          std::string(name) + " SHA-256 mismatch");
}

cli_options_t parse_cli(int argc, char **argv) {
  const std::map<std::string, std::filesystem::path cli_options_t::*> allowed{
      {"--config", &cli_options_t::config},
      {"--representation-checkpoint",
       &cli_options_t::representation_checkpoint},
      {"--mdn-checkpoint", &cli_options_t::mdn_checkpoint},
      {"--artifact", &cli_options_t::artifact},
      {"--work-dir", &cli_options_t::work_dir},
      {"--output", &cli_options_t::output},
  };
  require(argc == 1 + 2 * static_cast<int>(allowed.size()),
          "requires exactly --config, --representation-checkpoint, "
          "--mdn-checkpoint, --artifact, --work-dir, and --output");
  cli_options_t out{};
  std::map<std::string, bool> seen;
  for (int i = 1; i < argc; i += 2) {
    const std::string option = argv[i];
    const auto found = allowed.find(option);
    require(found != allowed.end(),
            "unknown argument rejected: " + option +
                " (training, policy, and holdout arguments are forbidden)");
    require(!seen[option], "duplicate argument: " + option);
    require(argv[i + 1] != nullptr && argv[i + 1][0] != '\0',
            "empty argument value: " + option);
    out.*(found->second) = argv[i + 1];
    seen[option] = true;
  }
  for (const auto &[name, member] : allowed) {
    require(!(out.*member).empty(), "missing argument: " + name);
  }
  return out;
}

void configure_determinism(std::int64_t seed) {
  torch::manual_seed(seed);
  if (torch::cuda::is_available()) {
    torch::cuda::manual_seed_all(seed);
  }
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setBenchmarkCuDNN(false);
  at::globalContext().setDeterministicAlgorithms(true, false);
  at::globalContext().setDeterministicFillUninitializedMemory(true);
  at::globalContext().setAllowTF32CuDNN(false);
  at::globalContext().setAllowTF32CuBLAS(false);
}

protocol::channel_graph_first_config_bundle_t
make_validation_bundle(const cli_options_t &paths) {
  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      paths.config.string());
  auto wave = runtime::decode_wave_settings_from_dsl(
      read_text(bundle.runtime_wave_dsl_path), std::string(kWaveId),
      protocol::runtime_wave_protocol_bindings(bundle.protocol_variant),
      read_text(bundle.ujcamei_source_cursor_dsl_path),
      read_text(bundle.ujcamei_source_splits_dsl_path));
  require(wave.action == runtime::wave_action_t::run,
          "selected wave is not run-only");
  require(wave.target == runtime::wave_target_t::inference_channel_mdn,
          "selected wave does not target Channel MDN inference");
  wave.source_range_policy = runtime::wave_source_range_policy_t::anchor_index;
  wave.anchor_index_begin = kValidationBegin;
  wave.anchor_index_end = kValidationEnd;
  wave.source_key_begin.reset();
  wave.source_key_end.reset();
  runtime::validate_wave_settings(wave);

  bundle.runtime_wave_id = std::string(kWaveId);
  bundle.wave_settings = std::move(wave);
  bundle.channel_mdn_training.input_representation_checkpoint_path =
      paths.representation_checkpoint.string();
  bundle.channel_mdn_training.input_mdn_checkpoint_path =
      paths.mdn_checkpoint.string();
  require(bundle.channel_mdn_training.batch_size == 64,
          "sealed Channel MDN training spec batch size is not 64");

  bundle.dock_binding = protocol::make_channel_graph_first_dock_binding(bundle);
  bundle.dock_binding_report =
      protocol::make_channel_graph_first_dock_binding_report(bundle);
  protocol::validate_channel_graph_first_protocol_contract(bundle);
  return bundle;
}

torch::Tensor cpu_clone(const torch::Tensor &value, const char *name) {
  require(value.defined(), std::string(name) + " is undefined");
  auto clone = value.detach().to(torch::kCPU).contiguous().clone();
  require(torch::isfinite(clone).all().item<bool>(),
          std::string(name) + " contains non-finite values");
  return clone;
}

inference::channel_graph_first_inference_launcher_t<
    Kline>::inference_batch_observer_t
make_observer(std::vector<captured_batch_t> &batches) {
  return [&batches](const inference::channel_graph_first_inference_launcher_t<
                        Kline>::mdn_out_t &out,
                    const inference::channel_graph_first_inference_launcher_t<
                        Kline>::channel_mdn_input_batch_t &,
                    const inference::channel_graph_first_inference_launcher_t<
                        Kline>::channel_representation_batch_t &channel_batch,
                    std::int64_t pulse) {
    const auto require_canonical_tensor = [](const torch::Tensor &value,
                                             const char *name) {
      require(value.defined() && value.device().is_cuda() &&
                  value.scalar_type() == torch::kFloat32,
              std::string(name) + " is not a CUDA float32 tensor");
    };
    require_canonical_tensor(out.log_pi, "log_pi");
    require_canonical_tensor(out.mu, "mu");
    require_canonical_tensor(out.sigma, "sigma");
    require_canonical_tensor(out.direct_edge_return, "direct_edge_return");
    batches.push_back(captured_batch_t{
        .log_pi = cpu_clone(out.log_pi, "log_pi"),
        .mu = cpu_clone(out.mu, "mu"),
        .sigma = cpu_clone(out.sigma, "sigma"),
        .direct_edge_return =
            cpu_clone(out.direct_edge_return, "direct_edge_return"),
        .begin_anchor_index =
            static_cast<std::int64_t>(channel_batch.cursor.begin_anchor_index),
        .anchor_count =
            static_cast<std::int64_t>(channel_batch.cursor.anchor_count()),
        .pulse = pulse,
        .anchor_indices = channel_batch.cursor.anchor_indices,
        .anchor_keys = channel_batch.cursor.anchor_keys,
    });
  };
}

void validate_launcher_result(const launcher_result_t &result,
                              const char *label) {
  const auto &report = result.report;
  require(report.target_action == "run",
          std::string(label) + " target action is not run");
  require(report.effective_batch_size == 64 &&
              report.batch_size_source == "derived",
          std::string(label) + " did not derive batch size 64");
  require(report.dtype == "float32" && report.device == "cuda" &&
              report.runtime_report_mode == "debug" &&
              report.wave_source_order_policy == "sequential",
          std::string(label) + " runtime dtype/device/mode/order mismatch");
  require(report.source_cursor_token == kExpectedSourceCursorToken &&
              report.source_anchor_count == 1170,
          std::string(label) + " source cursor seal mismatch");
  require(report.graph_order_fingerprint == "d334e38b1887ae16" &&
              report.node_ids == std::vector<std::string>{"SYNUSD", "SYNALPHA",
                                                          "SYNBETA",
                                                          "SYNGAMMA"} &&
              report.edge_ids == std::vector<std::string>{"SYNALPHASYNUSD",
                                                          "SYNBETASYNUSD",
                                                          "SYNGAMMASYNUSD"} &&
              report.target_coords ==
                  std::vector<std::int64_t>{0, 1, 2, 3, 4, 5, 6, 7, 8},
          std::string(label) + " graph or target identity mismatch");
  require(report.requested_anchor_index_begin == kValidationBegin &&
              report.requested_anchor_index_end == kValidationEnd,
          std::string(label) + " range is not [584,730)");
  require(report.steps_attempted == kExpectedBatchCount &&
              report.steps_completed == kExpectedBatchCount &&
              report.wave_streamed_anchor_count == kValidationAnchors,
          std::string(label) + " did not complete exactly three/146");
  require(report.total_valid_target_count == 12264 &&
              report.forecast_ev_valid_count == 12264,
          std::string(label) + " canonical target/EV coverage is not 12264");
  require(report.edge_return_projection_valid_count == kExpectedTargetCount &&
              report.direct_edge_return_readout_valid_count ==
                  kExpectedTargetCount,
          std::string(label) +
              " canonical edge/direct target coverage is not 1314");
  require(report.optimizer_steps == 0 && !report.checkpoint_written,
          std::string(label) + " performed training or wrote a checkpoint");
  require(!report.report_written && report.report_path.empty() &&
              !report.representation_edge_feature_probe_written &&
              report.representation_edge_feature_probe_path.empty() &&
              report.representation_edge_feature_probe_error.empty() &&
              !report.mdn_edge_context_feature_probe_written &&
              report.mdn_edge_context_feature_probe_path.empty() &&
              report.mdn_edge_context_feature_probe_error.empty(),
          std::string(label) + " wrote a canonical report or feature probe");
  require(report.representation_checkpoint_loaded &&
              report.mdn_checkpoint_loaded &&
              report.representation_parameter_device_check &&
              report.mdn_parameter_device_check,
          std::string(label) + " did not load both sealed checkpoints");
  require(result.batches.size() ==
              static_cast<std::size_t>(kExpectedBatchCount),
          std::string(label) + " observer batch count mismatch");
  std::int64_t next_anchor = kValidationBegin;
  std::int64_t anchor_sum = 0;
  for (std::size_t i = 0; i < result.batches.size(); ++i) {
    const auto &batch = result.batches[i];
    require(batch.begin_anchor_index == next_anchor && batch.anchor_count > 0,
            std::string(label) + " observer range is not contiguous");
    require(batch.pulse == static_cast<std::int64_t>(i + 1),
            std::string(label) + " observer pulse index mismatch");
    const std::int64_t expected_batch_size = i < 2 ? 64 : 18;
    require(batch.anchor_count == expected_batch_size,
            std::string(label) + " observer batches are not 64+64+18");
    require(batch.anchor_indices.size() ==
                    static_cast<std::size_t>(batch.anchor_count) &&
                batch.anchor_keys.size() ==
                    static_cast<std::size_t>(batch.anchor_count),
            std::string(label) + " observer cursor vector size mismatch");
    for (std::size_t j = 0; j < batch.anchor_indices.size(); ++j) {
      require(batch.anchor_indices[j] ==
                      static_cast<std::size_t>(next_anchor) + j &&
                  batch.anchor_indices[j] <
                      static_cast<std::size_t>(kValidationEnd),
              std::string(label) + " observer anchor indices are not 584..729");
    }
    require(batch.log_pi.dim() == 5 &&
                batch.mu.sizes() == batch.log_pi.sizes() &&
                batch.sigma.sizes() == batch.log_pi.sizes() &&
                batch.log_pi.size(0) == batch.anchor_count &&
                batch.log_pi.size(1) == 4 && batch.log_pi.size(2) == 3 &&
                batch.log_pi.size(3) == 9 && batch.log_pi.size(4) == 3 &&
                batch.direct_edge_return.dim() == 3 &&
                batch.direct_edge_return.size(0) == batch.anchor_count &&
                batch.direct_edge_return.size(1) == 3 &&
                batch.direct_edge_return.size(2) == 3,
            std::string(label) + " observer tensor shape mismatch");
    next_anchor += batch.anchor_count;
    anchor_sum += batch.anchor_count;
  }
  require(next_anchor == kValidationEnd && anchor_sum == kValidationAnchors,
          std::string(label) + " observer did not cover [584,730)");
}

launcher_result_t run_launcher(
    protocol::channel_graph_first_config_bundle_t bundle,
    std::optional<inference::channel_mdn_conditioned_affine_shadow_options_t>
        shadow_options) {
  protocol::graph_first_pipeline_builder_options_t builder_options{};
  builder_options.batch_size = 0;
  builder_options.compute_alignment_diagnostics = true;
  builder_options.dtype = torch::kFloat32;
  builder_options.device = torch::Device(torch::kCUDA);
  builder_options.runtime_report_mode =
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::debug;
  protocol::channel_graph_first_pipeline_builder_t<Kline> builder(
      std::move(bundle), builder_options);

  inference::channel_graph_first_inference_launcher_options_t options{};
  options.train_target_from_wave = false;
  options.train_target = false;
  options.write_report = false;
  options.report_path.clear();
  options.conditioned_affine_shadow = std::move(shadow_options);
  options.runtime_report_mode =
      cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t::debug;

  launcher_result_t result{};
  inference::channel_graph_first_inference_launcher_t<Kline> launcher(
      std::move(builder), std::move(options), make_observer(result.batches));
  result.report = launcher.run();
  result.canonical_report_text = result.report.to_text();
  return result;
}

tensor_parity_t compare_tensor_batches(
    const std::vector<captured_batch_t> &control,
    const std::vector<captured_batch_t> &treatment,
    const std::function<const torch::Tensor &(const captured_batch_t &)> &get) {
  require(control.size() == treatment.size(),
          "canonical observer batch counts differ");
  tensor_parity_t out{};
  out.batch_count = static_cast<std::int64_t>(control.size());
  for (std::size_t i = 0; i < control.size(); ++i) {
    const auto &lhs = get(control[i]);
    const auto &rhs = get(treatment[i]);
    require(lhs.sizes() == rhs.sizes() &&
                lhs.scalar_type() == rhs.scalar_type() &&
                lhs.device() == rhs.device() && lhs.is_contiguous() &&
                rhs.is_contiguous(),
            "canonical tensor shape/dtype/device/contiguity differs");
    out.torch_equal = out.torch_equal && torch::equal(lhs, rhs);
    const auto byte_count = static_cast<std::size_t>(lhs.numel()) *
                            static_cast<std::size_t>(lhs.element_size());
    out.raw_byte_equal =
        out.raw_byte_equal &&
        std::memcmp(lhs.data_ptr(), rhs.data_ptr(), byte_count) == 0;
    if (lhs.numel() > 0) {
      const auto delta = (lhs.to(torch::kFloat64) - rhs.to(torch::kFloat64))
                             .abs()
                             .max()
                             .item<double>();
      out.maximum_abs_delta = std::max(out.maximum_abs_delta, delta);
    }
  }
  return out;
}

std::map<std::string, std::string> parse_kv(const std::string &text) {
  std::map<std::string, std::string> out;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const auto separator = line.find('=');
    require(separator != std::string::npos && separator > 0,
            "malformed shadow report line");
    const auto key = line.substr(0, separator);
    const auto value = line.substr(separator + 1);
    require(out.emplace(key, value).second,
            "duplicate shadow report key: " + key);
  }
  return out;
}

std::string required_kv(const std::map<std::string, std::string> &values,
                        const std::string &key) {
  const auto found = values.find(key);
  require(found != values.end(), "missing shadow report key: " + key);
  return found->second;
}

std::int64_t required_i64(const std::map<std::string, std::string> &values,
                          const std::string &key) {
  const auto text = required_kv(values, key);
  std::size_t consumed = 0;
  const auto value = std::stoll(text, &consumed);
  require(consumed == text.size(), "invalid integer shadow key: " + key);
  return value;
}

double required_double(const std::map<std::string, std::string> &values,
                       const std::string &key) {
  const auto text = required_kv(values, key);
  std::size_t consumed = 0;
  const auto value = std::stod(text, &consumed);
  require(consumed == text.size() && std::isfinite(value),
          "invalid numeric shadow key: " + key);
  return value;
}

shadow_metrics_t parse_shadow_metrics(const std::string &report_text) {
  const auto values = parse_kv(report_text);
  require(
      required_kv(values, "schema_id") ==
              inference::kChannelMdnConditionedAffineShadowReportSchema &&
          required_kv(values, "status") == "complete" &&
          required_kv(values, "canonical_output_mutated") == "false" &&
          required_kv(values, "inference_batch_observer_exposed") == "false" &&
          required_kv(values, "replay_or_policy_output_exposed") == "false" &&
          required_kv(values, "artifact.validation_begin") == "584" &&
          required_kv(values, "artifact.validation_end") == "730" &&
          required_kv(values, "artifact.max_anchor") == "729" &&
          required_kv(values, "execution.observed_batch_count") == "3" &&
          required_kv(values, "execution.observed_anchor_count") == "146",
      "shadow isolation, interval, or coverage declaration mismatch");
  require(required_kv(values, "metrics.valid_count_per_edge") ==
                  "438,438,438" &&
              required_kv(values, "metrics.valid_count_per_channel") ==
                  "438,438,438" &&
              required_i64(values, "metrics.best_asset_valid_count") == 438,
          "shadow per-edge, per-channel, or best-asset coverage mismatch");
  return {
      .count = required_i64(values, "metrics.valid_count"),
      .pair_count = required_i64(values, "metrics.pairwise_rank_valid_count"),
      .mae = required_double(values, "metrics.mae"),
      .rmse = required_double(values, "metrics.rmse"),
      .direction = required_double(values, "metrics.directional_accuracy"),
      .rank = required_double(values, "metrics.pairwise_rank_accuracy"),
      .correlation = required_double(values, "metrics.correlation"),
      .prediction_std = required_double(values, "metrics.prediction_std"),
  };
}

double abs_delta(double actual, double expected) {
  require(std::isfinite(actual) && std::isfinite(expected),
          "non-finite parity value");
  return std::fabs(actual - expected);
}

void append_tensor_parity(std::ostringstream &out, const char *name,
                          const tensor_parity_t &parity) {
  out << "parity." << name << ".batch_count=" << parity.batch_count << '\n';
  out << "parity." << name
      << ".torch_equal=" << (parity.torch_equal ? "true" : "false") << '\n';
  out << "parity." << name
      << ".raw_byte_equal=" << (parity.raw_byte_equal ? "true" : "false")
      << '\n';
  out << "parity." << name << ".maximum_abs_delta=" << parity.maximum_abs_delta
      << '\n';
}

void write_exclusive(const std::filesystem::path &output,
                     const std::string &payload) {
  const int descriptor =
      ::open(output.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
  require(descriptor >= 0, "failed to exclusively create output fact");
  std::size_t offset = 0;
  while (offset < payload.size()) {
    const auto written =
        ::write(descriptor, payload.data() + offset, payload.size() - offset);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      (void)::close(descriptor);
      fail("failed to write output fact");
    }
    offset += static_cast<std::size_t>(written);
  }
  const bool flush_ok = ::fsync(descriptor) == 0;
  const bool close_ok = ::close(descriptor) == 0;
  require(flush_ok && close_ok, "failed to flush output fact");
}

int run(int argc, char **argv) {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
  auto paths = parse_cli(argc, argv);
  paths.config = canonical_input(paths.config, "config");
  paths.representation_checkpoint = canonical_input(
      paths.representation_checkpoint, "representation checkpoint");
  paths.mdn_checkpoint =
      canonical_input(paths.mdn_checkpoint, "MDN checkpoint");
  paths.artifact = canonical_input(paths.artifact, "conditioned artifact");
  paths.work_dir = normalized_new_path(paths.work_dir, "work directory");
  paths.output = normalized_new_path(paths.output, "output fact");
  require(paths.output.parent_path().empty() ||
              std::filesystem::is_directory(paths.output.parent_path()),
          "output fact parent directory does not exist");

  require_sha(paths.config, kExpectedConfigSha256, "config");
  require_sha(paths.representation_checkpoint,
              kExpectedRepresentationCheckpointSha256,
              "representation checkpoint");
  require_sha(paths.mdn_checkpoint, kExpectedMdnCheckpointSha256,
              "MDN checkpoint");
  require_sha(paths.artifact, kExpectedArtifactSha256, "conditioned artifact");
  require(torch::cuda::is_available(), "CUDA is required for this parity run");

  std::error_code ec;
  require(std::filesystem::create_directory(paths.work_dir, ec) && !ec,
          "failed to exclusively create work directory");
  const auto canonical_report = paths.work_dir / "canonical.report";
  const auto shadow_report =
      paths.work_dir / "conditioned_affine_shadow.report";

  auto base_bundle = make_validation_bundle(paths);
  auto control_bundle = base_bundle;
  auto treatment_bundle = base_bundle;

  configure_determinism(base_bundle.channel_mdn_training.seed);
  auto control = run_launcher(std::move(control_bundle), std::nullopt);
  validate_launcher_result(control, "control");

  inference::channel_mdn_conditioned_affine_shadow_options_t shadow_options{
      .artifact_path = paths.artifact,
      .artifact_file_sha256 = std::string(kExpectedArtifactSha256),
      .artifact_semantic_digest = std::string(kExpectedArtifactSemanticDigest),
      .expected_representation_checkpoint_path =
          paths.representation_checkpoint,
      .expected_representation_checkpoint_sha256 =
          std::string(kExpectedRepresentationCheckpointSha256),
      .expected_mdn_checkpoint_path = paths.mdn_checkpoint,
      .expected_mdn_checkpoint_sha256 =
          std::string(kExpectedMdnCheckpointSha256),
      .expected_source_cursor_token = std::string(kExpectedSourceCursorToken),
      .report_path = shadow_report,
      .close_feature_coord = 3,
  };

  configure_determinism(base_bundle.channel_mdn_training.seed);
  auto treatment =
      run_launcher(std::move(treatment_bundle), std::move(shadow_options));
  validate_launcher_result(treatment, "treatment");
  require(control.report.source_cursor_token ==
              treatment.report.source_cursor_token,
          "control/treatment source cursor tokens differ");
  require(control.batches.size() == treatment.batches.size(),
          "control/treatment observer batch counts differ");
  for (std::size_t i = 0; i < control.batches.size(); ++i) {
    require(control.batches[i].anchor_indices ==
                    treatment.batches[i].anchor_indices &&
                control.batches[i].anchor_keys ==
                    treatment.batches[i].anchor_keys,
            "control/treatment observer cursor identities differ");
  }

  const auto log_pi = compare_tensor_batches(
      control.batches, treatment.batches,
      [](const captured_batch_t &batch) -> const torch::Tensor & {
        return batch.log_pi;
      });
  const auto mu = compare_tensor_batches(
      control.batches, treatment.batches,
      [](const captured_batch_t &batch) -> const torch::Tensor & {
        return batch.mu;
      });
  const auto sigma = compare_tensor_batches(
      control.batches, treatment.batches,
      [](const captured_batch_t &batch) -> const torch::Tensor & {
        return batch.sigma;
      });
  const auto direct_edge_return = compare_tensor_batches(
      control.batches, treatment.batches,
      [](const captured_batch_t &batch) -> const torch::Tensor & {
        return batch.direct_edge_return;
      });
  require(log_pi.torch_equal && log_pi.raw_byte_equal &&
              log_pi.maximum_abs_delta == 0.0 && mu.torch_equal &&
              mu.raw_byte_equal && mu.maximum_abs_delta == 0.0 &&
              sigma.torch_equal && sigma.raw_byte_equal &&
              sigma.maximum_abs_delta == 0.0 &&
              direct_edge_return.torch_equal &&
              direct_edge_return.raw_byte_equal &&
              direct_edge_return.maximum_abs_delta == 0.0,
          "conditioned shadow mutated canonical MDN outputs");
  require(control.canonical_report_text == treatment.canonical_report_text,
          "conditioned shadow mutated the canonical launcher report");
  write_exclusive(canonical_report, control.canonical_report_text);

  const auto shadow = parse_shadow_metrics(read_text(shadow_report));
  require(shadow.count == kExpectedTargetCount &&
              shadow.pair_count == kExpectedTargetCount,
          "shadow target coverage is not exactly 1314/1314");
  constexpr shadow_metrics_t sealed{
      .count = kExpectedTargetCount,
      .pair_count = kExpectedTargetCount,
      .mae = 0.0161224808525453761,
      .rmse = 0.0208361389280214276,
      .direction = 0.805936073059360769,
      .rank = 0.793759512937595169,
      .correlation = 0.687224947833571354,
      .prediction_std = 0.0242951996069399045,
  };
  const double mae_delta = abs_delta(shadow.mae, sealed.mae);
  const double rmse_delta = abs_delta(shadow.rmse, sealed.rmse);
  const double direction_delta = abs_delta(shadow.direction, sealed.direction);
  const double rank_delta = abs_delta(shadow.rank, sealed.rank);
  const double correlation_delta =
      abs_delta(shadow.correlation, sealed.correlation);
  const double prediction_std_delta =
      abs_delta(shadow.prediction_std, sealed.prediction_std);
  const double maximum_shadow_delta =
      std::max({mae_delta, rmse_delta, direction_delta, rank_delta,
                correlation_delta, prediction_std_delta});
  const double maximum_continuous_shadow_delta = std::max(
      {mae_delta, rmse_delta, correlation_delta, prediction_std_delta});
  const auto decision_count = [](double accuracy, std::int64_t count) {
    require(accuracy >= 0.0 && accuracy <= 1.0 && count > 0,
            "invalid decision accuracy/count");
    return static_cast<std::int64_t>(
        std::llround(accuracy * static_cast<double>(count)));
  };
  const auto direction_decision_delta =
      std::llabs(decision_count(shadow.direction, shadow.count) -
                 decision_count(sealed.direction, sealed.count));
  const auto rank_decision_delta =
      std::llabs(decision_count(shadow.rank, shadow.pair_count) -
                 decision_count(sealed.rank, sealed.pair_count));
  const bool shadow_exact_parity = maximum_shadow_delta == 0.0;
  const bool shadow_operational_parity_pass =
      mae_delta <= kContinuousMetricOperationalTolerance &&
      rmse_delta <= kContinuousMetricOperationalTolerance &&
      correlation_delta <= kContinuousMetricOperationalTolerance &&
      prediction_std_delta <= kContinuousMetricOperationalTolerance &&
      direction_decision_delta <= kDirectionDecisionOperationalTolerance &&
      rank_decision_delta <= kRankDecisionOperationalTolerance;
  require(shadow_operational_parity_pass,
          "live shadow metrics exceed the count-aware cached-surface "
          "operational tolerance");
  const bool shadow_absolute_quality_pass =
      shadow.direction >= 0.65 && shadow.rank >= 0.65 &&
      shadow.correlation >= 0.50 && shadow.rmse <= 0.025;
  require(shadow_absolute_quality_pass,
          "live shadow misses the preregistered absolute quality gates");

  std::ostringstream fact;
  fact.imbue(std::locale::classic());
  fact << std::scientific << std::setprecision(17);
  fact << "schema_id=" << kSchemaId << '\n';
  fact << "status=complete\n";
  fact << "scope=validation_only\n";
  fact << "execution.control.completed=true\n";
  fact << "execution.treatment.completed=true\n";
  fact << "training_performed=false\n";
  fact << "refit_performed=false\n";
  fact << "policy_path_used=false\n";
  fact << "final_holdout_opened=false\n";
  fact << "boundary.validation=[584,730)\n";
  fact << "boundary.maximum_anchor_loaded=729\n";
  for (const auto &[name, result] :
       std::vector<std::pair<std::string, const launcher_result_t *>>{
           {"control", &control}, {"treatment", &treatment}}) {
    fact << "execution." << name
         << ".steps_attempted=" << result->report.steps_attempted << '\n';
    fact << "execution." << name
         << ".steps_completed=" << result->report.steps_completed << '\n';
    fact << "execution." << name << ".streamed_anchor_count="
         << result->report.wave_streamed_anchor_count << '\n';
    fact << "execution." << name
         << ".optimizer_steps=" << result->report.optimizer_steps << '\n';
    fact << "execution." << name << ".checkpoint_written="
         << (result->report.checkpoint_written ? "true" : "false") << '\n';
  }
  fact << "parity.canonical_report.byte_equal=true\n";
  append_tensor_parity(fact, "log_pi", log_pi);
  append_tensor_parity(fact, "mu", mu);
  append_tensor_parity(fact, "sigma", sigma);
  append_tensor_parity(fact, "direct_edge_return", direct_edge_return);
  fact << "shadow.count=" << shadow.count << '\n';
  fact << "shadow.pair_count=" << shadow.pair_count << '\n';
  fact << "shadow.mae=" << shadow.mae << '\n';
  fact << "shadow.rmse=" << shadow.rmse << '\n';
  fact << "shadow.directional_accuracy=" << shadow.direction << '\n';
  fact << "shadow.pairwise_rank_accuracy=" << shadow.rank << '\n';
  fact << "shadow.correlation=" << shadow.correlation << '\n';
  fact << "shadow.prediction_population_std=" << shadow.prediction_std << '\n';
  fact << "parity.shadow_to_sealed_report.count.exact=true\n";
  fact << "parity.shadow_to_sealed_report.pair_count.exact=true\n";
  fact << "parity.shadow_to_sealed_report.mae.maximum_abs_delta=" << mae_delta
       << '\n';
  fact << "parity.shadow_to_sealed_report.rmse.maximum_abs_delta=" << rmse_delta
       << '\n';
  fact << "parity.shadow_to_sealed_report.direction.maximum_abs_delta="
       << direction_delta << '\n';
  fact << "parity.shadow_to_sealed_report.direction.decision_count_delta="
       << direction_decision_delta << '\n';
  fact << "parity.shadow_to_sealed_report.rank.maximum_abs_delta=" << rank_delta
       << '\n';
  fact << "parity.shadow_to_sealed_report.rank.decision_count_delta="
       << rank_decision_delta << '\n';
  fact << "parity.shadow_to_sealed_report.correlation.maximum_abs_delta="
       << correlation_delta << '\n';
  fact << "parity.shadow_to_sealed_report.prediction_std.maximum_abs_delta="
       << prediction_std_delta << '\n';
  fact << "parity.shadow_to_sealed_report.maximum_abs_delta="
       << maximum_shadow_delta << '\n';
  fact << "parity.shadow_to_sealed_report."
          "maximum_continuous_metric_abs_delta="
       << maximum_continuous_shadow_delta << '\n';
  fact << "parity.shadow_to_sealed_report.exact="
       << (shadow_exact_parity ? "true" : "false") << '\n';
  fact << "parity.shadow_to_sealed_report.continuous_metric_tolerance="
       << kContinuousMetricOperationalTolerance << '\n';
  fact << "parity.shadow_to_sealed_report.direction_decision_count_tolerance="
       << kDirectionDecisionOperationalTolerance << '\n';
  fact << "parity.shadow_to_sealed_report.rank_decision_count_tolerance="
       << kRankDecisionOperationalTolerance << '\n';
  fact << "parity.shadow_to_sealed_report.tolerance_basis="
          "count_aware_live_batch_partition_operational_equivalence\n";
  fact << "parity.shadow_to_sealed_report.pass=true\n";
  fact << "quality.preregistered_absolute_gates.pass=true\n";
  fact << "input.config.sha256=" << kExpectedConfigSha256 << '\n';
  fact << "input.representation_checkpoint.sha256="
       << kExpectedRepresentationCheckpointSha256 << '\n';
  fact << "input.mdn_checkpoint.sha256=" << kExpectedMdnCheckpointSha256
       << '\n';
  fact << "input.artifact.sha256=" << kExpectedArtifactSha256 << '\n';
  fact << "input.artifact.semantic_tensor_digest="
       << kExpectedArtifactSemanticDigest << '\n';
  fact << "equivalence.pass=true\n";

  write_exclusive(paths.output, fact.str());
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  try {
    return run(argc, argv);
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] Channel Graph-First conditioned affine shadow "
                 "control/treatment equivalence: "
              << error.what() << '\n';
    return 1;
  }
}
