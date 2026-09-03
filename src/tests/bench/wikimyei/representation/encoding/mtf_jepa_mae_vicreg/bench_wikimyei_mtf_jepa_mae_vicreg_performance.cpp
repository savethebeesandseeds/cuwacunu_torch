#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <torch/cuda.h>
#include <torch/version.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr std::int64_t kSeed = 17;
constexpr std::int64_t kGraphNodes = 3;
constexpr std::int64_t kServingAnchorBatch = 1;
constexpr std::int64_t kTrainingAnchorBatch = 32;

struct cli_options_t {
  std::string device{"cpu"};
  std::string mode{"encode"};
  std::int64_t warmup_iterations{-1};
  std::int64_t measured_iterations{-1};
  std::int64_t anchor_batch{-1};
  std::int64_t threads{0};
  bool help{false};
};

struct latency_stats_t {
  double total_ms{0.0};
  double mean_ms{0.0};
  double p50_ms{0.0};
  double p95_ms{0.0};
  double min_ms{0.0};
  double max_ms{0.0};
};

struct parameter_stats_t {
  std::int64_t total_count{0};
  std::int64_t trainable_count{0};
  std::uint64_t total_bytes{0};
  std::uint64_t trainable_bytes{0};
  std::int64_t buffer_count{0};
  std::uint64_t buffer_bytes{0};
};

struct benchmark_result_t {
  std::string workload_id{};
  std::string workload_scope{};
  std::int64_t anchor_batch{0};
  std::int64_t model_rows{0};
  std::int64_t tokens_per_model_row{0};
  std::int64_t warmup_iterations{0};
  std::int64_t measured_iterations{0};
  std::uint64_t input_data_bytes{0};
  std::uint64_t input_mask_bytes{0};
  std::uint64_t output_primary_bytes{0};
  double validation_value{0.0};
  latency_stats_t latency{};
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error("[bench_wikimyei_mtf_jepa_mae_vicreg_performance] " +
                           message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

std::int64_t parse_integer(const std::string &text, const char *label,
                           std::int64_t minimum) {
  try {
    std::size_t parsed = 0;
    const auto value = std::stoll(text, &parsed, 10);
    if (parsed != text.size() || value < minimum) {
      fail(std::string(label) +
           " must be an integer >= " + std::to_string(minimum));
    }
    return value;
  } catch (const std::invalid_argument &) {
    fail(std::string(label) + " must be an integer");
  } catch (const std::out_of_range &) {
    fail(std::string(label) + " is outside the supported integer range");
  }
}

cli_options_t parse_cli(int argc, char **argv) {
  cli_options_t options{};
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--help" || argument == "-h") {
      options.help = true;
      continue;
    }
    const auto next_value = [&](const char *name) -> std::string {
      if (i + 1 >= argc) {
        fail(std::string("missing value for ") + name);
      }
      return argv[++i];
    };
    if (argument == "--device") {
      options.device = next_value("--device");
    } else if (argument == "--mode") {
      options.mode = next_value("--mode");
    } else if (argument == "--warmup") {
      options.warmup_iterations =
          parse_integer(next_value("--warmup"), "--warmup", 0);
    } else if (argument == "--iterations") {
      options.measured_iterations =
          parse_integer(next_value("--iterations"), "--iterations", 1);
    } else if (argument == "--anchors") {
      options.anchor_batch =
          parse_integer(next_value("--anchors"), "--anchors", 1);
    } else if (argument == "--threads") {
      options.threads = parse_integer(next_value("--threads"), "--threads", 1);
      require(options.threads <= std::numeric_limits<int>::max(),
              "--threads exceeds the LibTorch integer range");
    } else {
      fail("unknown argument: " + argument);
    }
  }

  require(options.device == "cpu" || options.device == "cuda",
          "--device must be cpu or cuda");
  require(options.mode == "encode" || options.mode == "train",
          "--mode must be encode or train");
  if (options.warmup_iterations < 0) {
    options.warmup_iterations = options.mode == "encode" ? 10 : 3;
  }
  if (options.measured_iterations < 0) {
    options.measured_iterations = options.mode == "encode" ? 50 : 20;
  }
  if (options.anchor_batch < 0) {
    options.anchor_batch =
        options.mode == "encode" ? kServingAnchorBatch : kTrainingAnchorBatch;
  }
  require(options.anchor_batch <=
              std::numeric_limits<std::int64_t>::max() / kGraphNodes,
          "--anchors is too large");
  return options;
}

void print_usage(const char *program) {
  std::cout << "usage: " << program
            << " [--device cpu|cuda] [--mode encode|train]"
               " [--warmup N] [--iterations N] [--anchors N] [--threads N]\n"
            << "defaults: --device cpu --mode encode; mode-specific warmup/"
               "iteration and anchor counts; current LibTorch thread count\n";
}

torch::Device resolve_device(const std::string &requested) {
  if (requested == "cpu") {
    return torch::Device(torch::kCPU);
  }
  require(torch::cuda::is_available(),
          "CUDA was requested but torch::cuda::is_available() is false");
  return torch::Device(torch::kCUDA, 0);
}

void synchronize(const torch::Device &device) {
  if (device.is_cuda()) {
    torch::cuda::synchronize(device.index());
  }
}

void seed_runtime(const torch::Device &device) {
  torch::manual_seed(kSeed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(kSeed);
  }
}

mtf::mtf_jepa_mae_vicreg_config_t
active_production_config(const torch::Device &device) {
  mtf::mtf_jepa_mae_vicreg_config_t config{};
  config.channel_count = 3;
  config.history_length = 30;
  config.input_width = 9;

  config.d_model = 32;
  config.latent_dim = 32;
  config.projector_dim = 64;
  config.predictor_hidden_dim = 64;
  config.num_encoder_layers = 2;
  config.num_predictor_layers = 2;
  config.num_decoder_layers = 1;
  config.num_heads = 4;
  config.dropout = 0.0;

  config.time_scales = {8, 16, 32, 64};
  config.scale_strides = {4, 8, 16, 32};
  config.use_frequency_tokens = true;
  config.frequency_num_bins = 16;
  config.frequency_log_magnitude = true;
  config.serving_pool_policy = mtf::mtf_serving_pool_policy_t::all_tokens;

  config.mask_ratio_time = 0.10;
  config.mask_ratio_frequency = 0.05;
  config.mask_ratio_channel = 0.0;
  config.min_context_ratio = 0.75;

  config.lambda_jepa = 1.0;
  config.lambda_mae = 0.25;
  config.lambda_tf_align = 0.10;
  config.lambda_vicreg = 0.05;
  config.lambda_global_vicreg = 0.25;
  config.lambda_channel_vicreg = 1.0;
  config.vicreg_sim_weight = 25.0;
  config.vicreg_var_weight = 25.0;
  config.vicreg_cov_weight = 1.0;
  config.vicreg_variance_floor = 1.0;
  config.vicreg_variance_epsilon = 0.0001;

  config.target_ema_tau = 0.990;
  config.use_target_ema = true;
  config.stop_gradient_target = true;
  config.return_diagnostics = true;
  config.use_mae_decoder = true;
  config.use_jepa_loss = true;
  config.use_tf_align_loss = true;
  config.use_vicreg_loss = true;
  config.use_global_vicreg = true;
  config.use_channel_vicreg = false;
  config.use_raw_reconstruction_targets = true;
  config.strict_finite_loss = true;
  config.couple_time_frequency_masks = false;
  config.mask_same_window_across_domains = false;
  config.mask_same_channel_block = false;
  config.max_context_target_time_overlap = 0.50;

  config.vicreg_view_gaussian_jitter_std = 0.005;
  config.vicreg_view_time_dropout_scale = 0.10;
  config.augmentation_profile = "light_phase_safe_v2";
  config.gaussian_jitter_std = 0.001;
  config.feature_dropout_prob = 0.0;
  config.history_dropout_prob = 0.0;
  config.time_crop_jitter_max = 0;
  config.time_dilation_min = 0.98;
  config.time_dilation_max = 1.02;
  config.time_warp_max = 0.01;
  config.amplitude_scale_min = 0.98;
  config.amplitude_scale_max = 1.02;
  config.amplitude_shift_std = 0.0;
  config.frequency_mask_ratio = 0.02;
  config.frequency_jitter_std = 0.01;
  config.phase_jitter_max = 0.0;
  config.channel_dropout_prob = 0.0;
  config.cross_channel_dropout_prob = 0.0;
  config.node_dropout_prob = 0.0;
  config.edge_dropout_prob = 0.0;
  config.magnitude_normalization_noise_std = 0.0;

  config.dtype = torch::kFloat32;
  config.device = device;
  return config;
}

std::uint64_t logical_bytes(const torch::Tensor &tensor) {
  if (!tensor.defined()) {
    return 0;
  }
  return static_cast<std::uint64_t>(tensor.numel()) *
         static_cast<std::uint64_t>(tensor.element_size());
}

parameter_stats_t parameter_stats(const mtf::MtfJepaMaeVicreg &model) {
  parameter_stats_t stats{};
  for (const auto &parameter : model->parameters(/*recurse=*/true)) {
    const auto count = parameter.numel();
    const auto bytes = logical_bytes(parameter);
    stats.total_count += count;
    stats.total_bytes += bytes;
    if (parameter.requires_grad()) {
      stats.trainable_count += count;
      stats.trainable_bytes += bytes;
    }
  }
  for (const auto &buffer : model->buffers(/*recurse=*/true)) {
    stats.buffer_count += buffer.numel();
    stats.buffer_bytes += logical_bytes(buffer);
  }
  return stats;
}

std::uint64_t
encode_output_bytes(const mtf::mtf_jepa_mae_vicreg_encode_output_t &output) {
  return logical_bytes(output.embeddings) +
         logical_bytes(output.pooled_embedding) +
         logical_bytes(output.pooled_by_channel) +
         logical_bytes(output.pooled_time) +
         logical_bytes(output.pooled_frequency) +
         logical_bytes(output.token_mask) +
         logical_bytes(output.sample_valid_mask) +
         logical_bytes(output.channel_valid_mask) +
         logical_bytes(output.metadata.start_index) +
         logical_bytes(output.metadata.width) +
         logical_bytes(output.metadata.scale_id) +
         logical_bytes(output.metadata.channel_id) +
         logical_bytes(output.metadata.domain_id);
}

std::uint64_t
training_output_bytes(const mtf::mtf_jepa_mae_vicreg_output_t &output) {
  return logical_bytes(output.embeddings) +
         logical_bytes(output.pooled_embedding) +
         logical_bytes(output.pooled_by_channel) +
         logical_bytes(output.pooled_time) +
         logical_bytes(output.pooled_frequency) + logical_bytes(output.loss) +
         logical_bytes(output.loss_jepa) + logical_bytes(output.loss_mae) +
         logical_bytes(output.loss_mae_time) +
         logical_bytes(output.loss_mae_frequency) +
         logical_bytes(output.loss_tf_align) +
         logical_bytes(output.loss_vicreg) +
         logical_bytes(output.loss_vicreg_global) +
         logical_bytes(output.loss_vicreg_channel) +
         logical_bytes(output.sample_valid_mask) +
         logical_bytes(output.channel_valid_mask);
}

double percentile(const std::vector<double> &sorted, double fraction) {
  require(!sorted.empty(), "cannot compute a percentile of no samples");
  const double position =
      fraction * static_cast<double>(sorted.size() - std::size_t{1});
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  const double weight = position - static_cast<double>(lower);
  return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

latency_stats_t summarize_latencies(std::vector<double> samples) {
  require(!samples.empty(), "no timed latency samples were collected");
  latency_stats_t stats{};
  stats.total_ms = std::accumulate(samples.begin(), samples.end(), 0.0);
  stats.mean_ms = stats.total_ms / static_cast<double>(samples.size());
  std::sort(samples.begin(), samples.end());
  stats.min_ms = samples.front();
  stats.max_ms = samples.back();
  stats.p50_ms = percentile(samples, 0.50);
  stats.p95_ms = percentile(samples, 0.95);
  return stats;
}

template <typename Operation>
latency_stats_t
measure(const torch::Device &device, std::int64_t warmup_iterations,
        std::int64_t measured_iterations, Operation &&operation) {
  for (std::int64_t i = 0; i < warmup_iterations; ++i) {
    operation();
  }
  synchronize(device);

  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(measured_iterations));
  for (std::int64_t i = 0; i < measured_iterations; ++i) {
    synchronize(device);
    const auto begin = std::chrono::steady_clock::now();
    operation();
    synchronize(device);
    const auto end = std::chrono::steady_clock::now();
    samples.push_back(
        std::chrono::duration<double, std::milli>(end - begin).count());
  }
  return summarize_latencies(std::move(samples));
}

bool finite(const torch::Tensor &tensor) {
  return tensor.defined() && torch::isfinite(tensor).all().item<bool>();
}

benchmark_result_t run_encode(const cli_options_t &options,
                              const torch::Device &device,
                              const mtf::mtf_jepa_mae_vicreg_config_t &config,
                              mtf::MtfJepaMaeVicreg &model) {
  const std::int64_t model_rows = options.anchor_batch * kGraphNodes;
  const auto data_options =
      torch::TensorOptions().dtype(torch::kFloat32).device(device);
  const auto mask_options =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  const auto data = torch::randn({model_rows, config.channel_count,
                                  config.history_length, config.input_width},
                                 data_options);
  const auto feature_mask = torch::ones(data.sizes(), mask_options);

  model->eval();
  torch::NoGradGuard no_grad;
  const auto operation = [&]() {
    auto output = model->encode(data, feature_mask);
    (void)output;
  };

  benchmark_result_t result{};
  result.workload_id = "active_cwu_02v_encode_anchor" +
                       std::to_string(options.anchor_batch) + "_v1";
  result.workload_scope = options.anchor_batch == 64
                              ? "active_downstream_batch64"
                          : options.anchor_batch == 1 ? "serving_microbatch"
                                                      : "custom_encode_batch";
  result.anchor_batch = options.anchor_batch;
  result.model_rows = model_rows;
  result.warmup_iterations = options.warmup_iterations;
  result.measured_iterations = options.measured_iterations;
  result.input_data_bytes = logical_bytes(data);
  result.input_mask_bytes = logical_bytes(feature_mask);
  result.latency = measure(device, options.warmup_iterations,
                           options.measured_iterations, operation);

  auto validation = model->encode(data, feature_mask);
  synchronize(device);
  require(validation.embeddings.dim() == 3 &&
              validation.embeddings.size(0) == model_rows &&
              validation.embeddings.size(2) == config.latent_dim,
          "encode embeddings have an unexpected shape");
  require(validation.pooled_embedding.sizes() ==
              torch::IntArrayRef({model_rows, config.latent_dim}),
          "encode pooled embedding has an unexpected shape");
  require(validation.pooled_by_channel.sizes() ==
              torch::IntArrayRef(
                  {model_rows, config.channel_count, config.latent_dim}),
          "encode channel pool has an unexpected shape");
  require(validation.embeddings.device() == device,
          "encode output is on the wrong device");
  require(finite(validation.embeddings) &&
              finite(validation.pooled_embedding) &&
              finite(validation.pooled_by_channel),
          "encode output contains non-finite values");

  result.tokens_per_model_row = validation.embeddings.size(1);
  result.output_primary_bytes = encode_output_bytes(validation);
  result.validation_value = validation.pooled_embedding.sum().item<double>();
  require(std::isfinite(result.validation_value),
          "encode validation checksum is not finite");
  return result;
}

benchmark_result_t run_train(const cli_options_t &options,
                             const torch::Device &device,
                             const mtf::mtf_jepa_mae_vicreg_config_t &config,
                             mtf::MtfJepaMaeVicreg &model) {
  const std::int64_t model_rows = options.anchor_batch * kGraphNodes;
  const auto data_options =
      torch::TensorOptions().dtype(torch::kFloat32).device(device);
  const auto mask_options =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  const auto data = torch::randn({model_rows, config.channel_count,
                                  config.history_length, config.input_width},
                                 data_options);
  const auto feature_mask = torch::ones(data.sizes(), mask_options);

  model->train();
  torch::optim::Adam optimizer(model->parameters(),
                               torch::optim::AdamOptions(0.001));
  const auto operation = [&]() {
    optimizer.zero_grad();
    auto output = model->forward(data, feature_mask);
    output.loss.backward();
    optimizer.step();
    model->update_target_network();
  };

  benchmark_result_t result{};
  result.workload_id = "active_cwu_02v_train_anchor" +
                       std::to_string(options.anchor_batch) + "_v1";
  result.workload_scope = options.anchor_batch == 32
                              ? "active_train_core_batch32"
                              : "custom_train_batch";
  result.anchor_batch = options.anchor_batch;
  result.model_rows = model_rows;
  result.warmup_iterations = options.warmup_iterations;
  result.measured_iterations = options.measured_iterations;
  result.input_data_bytes = logical_bytes(data);
  result.input_mask_bytes = logical_bytes(feature_mask);
  result.latency = measure(device, options.warmup_iterations,
                           options.measured_iterations, operation);

  auto validation = model->forward(data, feature_mask);
  synchronize(device);
  require(validation.embeddings.dim() == 3 &&
              validation.embeddings.size(0) == model_rows &&
              validation.embeddings.size(2) == config.latent_dim,
          "training embeddings have an unexpected shape");
  require(validation.pooled_embedding.sizes() ==
              torch::IntArrayRef({model_rows, config.latent_dim}),
          "training pooled embedding has an unexpected shape");
  require(validation.pooled_by_channel.sizes() ==
              torch::IntArrayRef(
                  {model_rows, config.channel_count, config.latent_dim}),
          "training channel pool has an unexpected shape");
  require(validation.loss.device() == device,
          "training loss is on the wrong device");
  require(finite(validation.loss) && finite(validation.embeddings) &&
              finite(validation.pooled_embedding),
          "training validation output contains non-finite values");

  result.tokens_per_model_row = validation.embeddings.size(1);
  result.output_primary_bytes = training_output_bytes(validation);
  result.validation_value = validation.loss.item<double>();
  require(std::isfinite(result.validation_value),
          "training validation loss is not finite");
  return result;
}

void emit_report(const cli_options_t &options, const torch::Device &device,
                 const mtf::mtf_jepa_mae_vicreg_config_t &config,
                 const parameter_stats_t &parameters,
                 const benchmark_result_t &result) {
  const double elapsed_seconds = result.latency.total_ms / 1000.0;
  require(elapsed_seconds > 0.0 && std::isfinite(elapsed_seconds),
          "measured elapsed time is invalid");
  const double operations = static_cast<double>(result.measured_iterations);
  const double anchors_per_second =
      operations * static_cast<double>(result.anchor_batch) / elapsed_seconds;
  const double node_rows_per_second =
      operations * static_cast<double>(result.model_rows) / elapsed_seconds;
  const double tokens_per_second =
      operations * static_cast<double>(result.model_rows) *
      static_cast<double>(result.tokens_per_model_row) / elapsed_seconds;
  const double optimizer_steps_per_second =
      options.mode == "train" ? operations / elapsed_seconds : 0.0;
  const std::uint64_t static_logical_bytes =
      parameters.total_bytes + parameters.buffer_bytes +
      result.input_data_bytes + result.input_mask_bytes +
      result.output_primary_bytes;

  std::cout << std::boolalpha << std::setprecision(12);
  std::cout << "schema=cuwacunu.wikimyei.mtf_jepa_mae_vicreg.performance.v1\n";
  std::cout << "status=pass\n";
  std::cout << "execution_validated=true\n";
  std::cout << "performance_threshold_applied=false\n";
  std::cout << "workload_id=" << result.workload_id << "\n";
  std::cout << "workload_scope=" << result.workload_scope << "\n";
  std::cout << "mode=" << options.mode << "\n";
  std::cout << "requested_device=" << options.device << "\n";
  std::cout << "actual_device=" << device.str() << "\n";
  std::cout << "dtype=float32\n";
  std::cout << "libtorch_version=" << TORCH_VERSION << "\n";
  std::cout << "cuda_available=" << torch::cuda::is_available() << "\n";
  std::cout << "seed=" << kSeed << "\n";
  std::cout << "intraop_threads=" << at::get_num_threads() << "\n";
  std::cout << "interop_threads=" << at::get_num_interop_threads() << "\n";
  std::cout << "anchor_batch=" << result.anchor_batch << "\n";
  std::cout << "graph_nodes=" << kGraphNodes << "\n";
  std::cout << "model_rows=" << result.model_rows << "\n";
  std::cout << "input_shape=" << result.model_rows << ","
            << config.channel_count << "," << config.history_length << ","
            << config.input_width << "\n";
  std::cout << "channels=" << config.channel_count << "\n";
  std::cout << "history_length=" << config.history_length << "\n";
  std::cout << "input_width=" << config.input_width << "\n";
  std::cout << "latent_dim=" << config.latent_dim << "\n";
  std::cout << "tokens_per_model_row=" << result.tokens_per_model_row << "\n";
  std::cout << "feature_valid_fraction=1\n";
  std::cout << "timed_boundary="
            << (options.mode == "encode" ? "model_encode"
                                         : "forward_backward_adam_ema")
            << "\n";
  std::cout << "outer_augmentation_included=false\n";
  std::cout << "launcher_overheads_included=false\n";
  std::cout << "encode_autograd_mode="
            << (options.mode == "encode" ? "no_grad" : "not_applicable")
            << "\n";
  std::cout << "serving_pool_policy="
            << mtf::mtf_serving_pool_policy_name(config.serving_pool_policy)
            << "\n";
  std::cout << "warmup_iterations=" << result.warmup_iterations << "\n";
  std::cout << "measured_iterations=" << result.measured_iterations << "\n";
  std::cout << "parameter_count=" << parameters.total_count << "\n";
  std::cout << "trainable_parameter_count=" << parameters.trainable_count
            << "\n";
  std::cout << "parameter_logical_bytes=" << parameters.total_bytes << "\n";
  std::cout << "trainable_parameter_logical_bytes="
            << parameters.trainable_bytes << "\n";
  std::cout << "buffer_count=" << parameters.buffer_count << "\n";
  std::cout << "buffer_logical_bytes=" << parameters.buffer_bytes << "\n";
  std::cout << "input_data_logical_bytes=" << result.input_data_bytes << "\n";
  std::cout << "input_mask_logical_bytes=" << result.input_mask_bytes << "\n";
  std::cout << "output_primary_logical_bytes=" << result.output_primary_bytes
            << "\n";
  std::cout << "static_logical_bytes=" << static_logical_bytes << "\n";
  std::cout << "logical_bytes_scope=parameters_buffers_input_primary_output\n";
  std::cout << "runtime_peak_memory_included=false\n";
  std::cout << "training_state_memory_included=false\n";
  std::cout << "latency_ms_mean=" << result.latency.mean_ms << "\n";
  std::cout << "latency_ms_p50=" << result.latency.p50_ms << "\n";
  std::cout << "latency_ms_p95=" << result.latency.p95_ms << "\n";
  std::cout << "latency_ms_min=" << result.latency.min_ms << "\n";
  std::cout << "latency_ms_max=" << result.latency.max_ms << "\n";
  std::cout << "anchors_per_second=" << anchors_per_second << "\n";
  std::cout << "node_rows_per_second=" << node_rows_per_second << "\n";
  std::cout << "encoded_tokens_per_second=" << tokens_per_second << "\n";
  std::cout << "optimizer_steps_per_second=" << optimizer_steps_per_second
            << "\n";
  std::cout << "validation_value=" << result.validation_value << "\n";
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_cli(argc, argv);
    if (options.help) {
      print_usage(argv[0]);
      return 0;
    }

    if (options.threads > 0) {
      at::set_num_threads(static_cast<int>(options.threads));
    }
    const auto device = resolve_device(options.device);
    seed_runtime(device);
    const auto config = active_production_config(device);
    auto model = mtf::MtfJepaMaeVicreg(config);
    const auto parameters = parameter_stats(model);
    require(parameters.total_count > 0 && parameters.trainable_count > 0,
            "model has no trainable parameters");

    const auto result = options.mode == "encode"
                            ? run_encode(options, device, config, model)
                            : run_train(options, device, config, model);
    emit_report(options, device, config, parameters, result);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[bench_wikimyei_mtf_jepa_mae_vicreg_performance] failure: "
              << error.what() << "\n";
    return 1;
  }
}
