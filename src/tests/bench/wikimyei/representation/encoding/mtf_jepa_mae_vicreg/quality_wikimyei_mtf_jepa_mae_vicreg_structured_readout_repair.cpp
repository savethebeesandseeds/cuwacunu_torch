#include "structured_readout_repair_gate.h"
#include "structured_readout_shadow.h"
#include "piaabo/digest/sha256.h"

// Reuse the sealed PSM-1 data, capture, projection, probe, bootstrap, and
// reporting machinery without editing its source.  Renaming its entry point
// keeps SRR-1 a separate binary and preserves the parent artifact byte-for-byte.
#define main psm_parent_embedded_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp"
#undef main

namespace srr_shadow = mtf::structured_readout_shadow;
namespace srr_gate = cuwacunu::tests::structured_readout_repair_gate;
namespace digest = cuwacunu::piaabo::digest;

namespace {

constexpr std::size_t kSrrArmCount = 4;
constexpr std::array<const char *, kSrrArmCount> kSrrArmNames{
    "channel", "offline_cdsb", "shadow", "encoder"};

constexpr std::string_view kSrrPrerunManifestPath =
    ".build/tests/representation_srr_v1_prerun.sha256";

[[nodiscard]] std::string srr_read_binary_file(
    const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open SRR pre-run manifest: " +
                             path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

enum class SrrArm : std::size_t {
  channel = 0,
  offline_cdsb = 1,
  shadow = 2,
  encoder = 3,
};

[[nodiscard]] constexpr std::size_t srr_index(SrrArm arm) {
  return static_cast<std::size_t>(arm);
}

struct SrrEncodedCapture {
  RssmEncodedCapture parent{};
  torch::Tensor audit_by_channel{};  // [S,3,32], CPU float64
  torch::Tensor device_by_channel{}; // [S,3,32], CPU float64
  uint64_t audit_float64_hash{0};
  uint64_t device_float64_hash{0};
  bool shadow_input_unchanged{true};
  bool audit_contract_exact{true};
  bool device_contract_exact{true};
  bool repeated_device_exact{true};
  double device_relative_l2{0.0};
};

[[nodiscard]] uint64_t srr_metadata_hash(
    const mtf::mtf_token_metadata_t &metadata) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.start_index));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.width));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.scale_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.channel_id));
  mix_hash_value(hash, hash_tensor_stable_bytes(metadata.domain_id));
  return hash;
}

[[nodiscard]] SrrEncodedCapture
srr_capture_once(mtf::MtfJepaMaeVicreg &model, const Dataset &dataset,
                 const torch::Device &device,
                 const torch::Tensor &psm_projection,
                 bool check_direct_encoder) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> tokenizer_source_chunks;
  std::vector<torch::Tensor> encoder_source_chunks;
  std::vector<torch::Tensor> served_source_chunks;
  std::vector<torch::Tensor> audit_chunks;
  std::vector<torch::Tensor> device_chunks;
  uint64_t token_mask_structure_hash = 0xcbf29ce484222325ULL;
  uint64_t metadata_structure_hash = 0;
  torch::Tensor grouped_metadata_layout{};
  bool metadata_initialized = false;
  bool public_exact = true;
  bool direct_exact = true;
  bool production_order_exact = true;
  bool cardinality_exact = true;
  bool shadow_input_unchanged = true;
  bool audit_contract_exact = true;
  bool device_contract_exact = true;
  bool repeated_device_exact = true;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto data = dataset.data.narrow(0, begin, size).to(device);
    const auto feature_mask = dataset.mask.narrow(0, begin, size).to(device);
    const auto tokens_before = model->tokenize(data, feature_mask);
    const auto encoded = model->encode(data, feature_mask);
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());

    const uint64_t embeddings_before =
        hash_tensor_stable_bytes(encoded.embeddings);
    const uint64_t token_mask_before =
        hash_tensor_stable_bytes(encoded.token_mask);
    const uint64_t sample_mask_before =
        hash_tensor_stable_bytes(encoded.sample_valid_mask);
    const uint64_t channel_mask_before =
        hash_tensor_stable_bytes(encoded.channel_valid_mask);
    const uint64_t metadata_before = srr_metadata_hash(encoded.metadata);
    const auto audit =
        srr_shadow::readout_cpu64(encoded, psm_projection, model->config());
    const auto shadow =
        srr_shadow::readout(encoded, psm_projection, model->config());
    const auto shadow_repeat =
        srr_shadow::readout(encoded, psm_projection, model->config());
    shadow_input_unchanged =
        shadow_input_unchanged &&
        embeddings_before == hash_tensor_stable_bytes(encoded.embeddings) &&
        token_mask_before == hash_tensor_stable_bytes(encoded.token_mask) &&
        sample_mask_before ==
            hash_tensor_stable_bytes(encoded.sample_valid_mask) &&
        channel_mask_before ==
            hash_tensor_stable_bytes(encoded.channel_valid_mask) &&
        metadata_before == srr_metadata_hash(encoded.metadata);
    audit_contract_exact =
        audit_contract_exact &&
        audit.values.sizes() == torch::IntArrayRef({size, 3, 32}) &&
        audit.values.scalar_type() == torch::kFloat64 &&
        audit.values.device().is_cpu() && audit.values.is_contiguous() &&
        audit.valid_mask.sizes() == torch::IntArrayRef({size, 3}) &&
        audit.valid_mask.scalar_type() == torch::kBool &&
        audit.valid_mask.device().is_cpu() &&
        audit.valid_mask.all().item<bool>() &&
        torch::isfinite(audit.values).all().item<bool>();
    device_contract_exact =
        device_contract_exact &&
        shadow.values.sizes() == torch::IntArrayRef({size, 3, 32}) &&
        shadow.values.scalar_type() == encoded.embeddings.scalar_type() &&
        shadow.values.device() == encoded.embeddings.device() &&
        shadow.values.is_contiguous() &&
        shadow.valid_mask.sizes() == torch::IntArrayRef({size, 3}) &&
        shadow.valid_mask.scalar_type() == torch::kBool &&
        shadow.valid_mask.device() == encoded.embeddings.device() &&
        shadow.valid_mask.all().item<bool>() &&
        torch::isfinite(shadow.values).all().item<bool>() &&
        rssm_tensor_bytes_equal(audit.valid_mask, shadow.valid_mask);
    repeated_device_exact =
        repeated_device_exact &&
        rssm_tensor_bytes_equal(shadow.values, shadow_repeat.values) &&
        rssm_tensor_bytes_equal(shadow.valid_mask,
                                shadow_repeat.valid_mask);

    const auto tokens_after = model->tokenize(data, feature_mask);
    public_exact =
        public_exact &&
        rssm_token_batch_bytes_equal(tokens_before, tokens_after) &&
        rssm_tensor_bytes_equal(tokens_before.token_mask, encoded.token_mask) &&
        rssm_metadata_bytes_equal(tokens_before.metadata, encoded.metadata);
    mix_hash_value(token_mask_structure_hash,
                   hash_tensor_stable_bytes(tokens_before.token_mask));
    const uint64_t batch_metadata_hash =
        srr_metadata_hash(tokens_before.metadata);
    if (!metadata_initialized) {
      metadata_structure_hash = batch_metadata_hash;
      metadata_initialized = true;
    } else {
      public_exact =
          public_exact && metadata_structure_hash == batch_metadata_hash;
    }
    if (!tokens_before.token_mask.all().item<bool>() ||
        !encoded.sample_valid_mask.all().item<bool>() ||
        !encoded.channel_valid_mask.all().item<bool>() ||
        !served.valid_mask.all().item<bool>()) {
      throw std::runtime_error("SRR fully observed row became invalid");
    }
    const auto order =
        rssm_token_order(tokens_before.metadata, tokens_before.tokens.size(1));
    production_order_exact =
        production_order_exact && order.production_order_exact;
    cardinality_exact = cardinality_exact && order.cardinality_exact;
    const auto metadata_layout = torch::stack(
        {tokens_before.metadata.domain_id, tokens_before.metadata.scale_id,
         tokens_before.metadata.start_index, tokens_before.metadata.width},
        1).to(torch::kCPU, torch::kInt64).contiguous();
    std::vector<torch::Tensor> grouped_layout_channels;
    grouped_layout_channels.reserve(kChannels);
    for (const auto &indices : order.channel_indices) {
      grouped_layout_channels.push_back(metadata_layout.index_select(
          0, torch::tensor(indices, torch::kInt64)));
    }
    const auto batch_grouped_metadata_layout =
        torch::stack(grouped_layout_channels, 0).contiguous();
    if (!grouped_metadata_layout.defined()) {
      grouped_metadata_layout = batch_grouped_metadata_layout;
    } else {
      public_exact =
          public_exact && rssm_tensor_bytes_equal(
                              grouped_metadata_layout,
                              batch_grouped_metadata_layout);
    }
    const auto tokenizer_by_channel =
        rssm_group_tokens_by_channel(tokens_before.tokens, order);
    const auto encoder_by_channel =
        rssm_group_tokens_by_channel(encoded.embeddings, order);
    if (served.values.sizes() !=
        torch::IntArrayRef({size, kChannels, kLatentDim})) {
      throw std::runtime_error("SRR served tensor shape mismatch");
    }
    if (check_direct_encoder) {
      auto children = model->named_children();
      const auto *entry = children.find(std::string{"encoder"});
      if (entry == nullptr) {
        throw std::runtime_error("SRR registered online encoder is missing");
      }
      auto *online_encoder = (*entry)->as<mtf::SharedTokenEncoder>();
      if (online_encoder == nullptr) {
        throw std::runtime_error("SRR registered encoder type mismatch");
      }
      const auto direct_encoder = online_encoder->forward(
          tokens_before.tokens, tokens_before.token_mask);
      const auto direct_served = mtf::detail::pooled_by_channel(
          direct_encoder, tokens_before.token_mask, tokens_before.metadata,
          model->config());
      direct_exact =
          direct_exact &&
          rssm_tensor_bytes_equal(direct_encoder, encoded.embeddings) &&
          rssm_tensor_bytes_equal(direct_served, served.values);
    }
    tokenizer_source_chunks.push_back(
        tokenizer_by_channel.detach().to(torch::kCPU));
    encoder_source_chunks.push_back(
        encoder_by_channel.detach().to(torch::kCPU));
    served_source_chunks.push_back(served.values.detach().to(torch::kCPU));
    audit_chunks.push_back(audit.values.contiguous());
    device_chunks.push_back(
        shadow.values.detach().to(torch::kCPU, torch::kFloat64).contiguous());
  }
  model->train(was_training);
  auto tokenizer_source = torch::cat(tokenizer_source_chunks, 0).contiguous();
  auto encoder_source = torch::cat(encoder_source_chunks, 0).contiguous();
  auto served_source = torch::cat(served_source_chunks, 0).contiguous();
  auto tokenizer_float64 = tokenizer_source.to(torch::kFloat64).contiguous();
  auto encoder_float64 = encoder_source.to(torch::kFloat64).contiguous();
  auto served_float64 = served_source.to(torch::kFloat64).contiguous();
  auto audit_values = torch::cat(audit_chunks, 0).contiguous();
  auto device_values = torch::cat(device_chunks, 0).contiguous();
  const double denominator =
      std::max(1.0e-30, audit_values.pow(2).sum().sqrt().item<double>());
  const double relative_l2 =
      (device_values - audit_values).pow(2).sum().sqrt().item<double>() /
      denominator;
  return {
      .parent = {.tokenizer_by_channel = tokenizer_float64,
                 .encoder_by_channel = encoder_float64,
                 .served_by_channel = served_float64,
                 .grouped_metadata_layout = grouped_metadata_layout,
                 .tokenizer_source_hash =
                     hash_tensor_stable_bytes(tokenizer_source),
                 .encoder_source_hash =
                     hash_tensor_stable_bytes(encoder_source),
                 .served_source_hash = hash_tensor_stable_bytes(served_source),
                 .tokenizer_float64_hash =
                     hash_tensor_stable_bytes(tokenizer_float64),
                 .encoder_float64_hash =
                     hash_tensor_stable_bytes(encoder_float64),
                 .served_float64_hash =
                     hash_tensor_stable_bytes(served_float64),
                 .token_mask_structure_hash = token_mask_structure_hash,
                 .metadata_structure_hash = metadata_structure_hash,
                 .public_sandwich_exact = public_exact,
                 .direct_encoder_exact = direct_exact,
                 .production_order_exact = production_order_exact,
                 .cardinality_exact = cardinality_exact},
      .audit_by_channel = audit_values,
      .device_by_channel = device_values,
      .audit_float64_hash = hash_tensor_stable_bytes(audit_values),
      .device_float64_hash = hash_tensor_stable_bytes(device_values),
      .shadow_input_unchanged = shadow_input_unchanged,
      .audit_contract_exact = audit_contract_exact,
      .device_contract_exact = device_contract_exact,
      .repeated_device_exact = repeated_device_exact,
      .device_relative_l2 = relative_l2};
}

[[nodiscard]] bool srr_local_capture_exact(const SrrEncodedCapture &left,
                                           const SrrEncodedCapture &right) {
  return rssm_capture_exact(left.parent, right.parent) &&
         left.audit_float64_hash == right.audit_float64_hash &&
         rssm_tensor_bytes_equal(left.audit_by_channel,
                                 right.audit_by_channel) &&
         left.shadow_input_unchanged == right.shadow_input_unchanged &&
         left.audit_contract_exact == right.audit_contract_exact;
}

[[nodiscard]] bool srr_device_capture_exact(const SrrEncodedCapture &left,
                                            const SrrEncodedCapture &right) {
  return left.device_float64_hash == right.device_float64_hash &&
         rssm_tensor_bytes_equal(left.device_by_channel,
                                 right.device_by_channel) &&
         left.device_contract_exact == right.device_contract_exact &&
         left.repeated_device_exact == right.repeated_device_exact;
}

[[nodiscard]] bool srr_capture_exact(const SrrEncodedCapture &left,
                                     const SrrEncodedCapture &right) {
  return srr_local_capture_exact(left, right) &&
         srr_device_capture_exact(left, right);
}

struct SrrFeatureSet {
  std::array<PsmSurfaceFeatures, kSrrArmCount> arms{};
  double offline_equivalence_max_abs{0.0};
  double device_translation_max_abs{0.0};
  double device_relative_l2{0.0};
  bool offline_bytes_exact{false};
  bool finite_and_shape_exact{false};
  bool pass{false};
};

[[nodiscard]] PsmSurfaceFeatures
srr_surface(const torch::Tensor &by_channel) {
  return {.by_channel = by_channel.contiguous(),
          .flat = by_channel.reshape({by_channel.size(0), kServedWidth})
                      .contiguous()};
}

[[nodiscard]] SrrFeatureSet
srr_feature_set(const SrrEncodedCapture &capture,
                const torch::Tensor &psm_projection,
                const torch::Tensor &rssm_projection) {
  const auto parent =
      psm_feature_set(capture.parent, psm_projection, rssm_projection);
  SrrFeatureSet result{};
  result.arms[srr_index(SrrArm::channel)] =
      parent.arms[psm_index(PsmArm::channel)];
  result.arms[srr_index(SrrArm::offline_cdsb)] =
      parent.arms[psm_index(PsmArm::channel_domain_scale_bin)];
  result.arms[srr_index(SrrArm::shadow)] =
      srr_surface(capture.device_by_channel);
  result.arms[srr_index(SrrArm::encoder)] =
      parent.arms[psm_index(PsmArm::encoder)];
  const auto &offline =
      result.arms[srr_index(SrrArm::offline_cdsb)].by_channel;
  result.offline_equivalence_max_abs =
      psm_max_abs(capture.audit_by_channel, offline);
  result.device_translation_max_abs =
      psm_max_abs(capture.device_by_channel, offline);
  result.device_relative_l2 = capture.device_relative_l2;
  result.offline_bytes_exact =
      rssm_tensor_bytes_equal(capture.audit_by_channel, offline);
  result.finite_and_shape_exact = parent.pass;
  for (const auto &arm : result.arms) {
    result.finite_and_shape_exact =
        result.finite_and_shape_exact &&
        arm.by_channel.sizes() ==
            torch::IntArrayRef({offline.size(0), kChannels, kLatentDim}) &&
        arm.flat.sizes() ==
            torch::IntArrayRef({offline.size(0), kServedWidth}) &&
        arm.by_channel.scalar_type() == torch::kFloat64 &&
        arm.by_channel.device().is_cpu() && arm.by_channel.is_contiguous() &&
        arm.flat.is_contiguous() && torch::isfinite(arm.flat).all().item<bool>();
  }
  result.pass =
      result.finite_and_shape_exact && result.offline_bytes_exact &&
      result.offline_equivalence_max_abs <=
          srr_gate::kOfflineEquivalenceTolerance &&
      result.device_translation_max_abs <=
          srr_gate::kDeviceTranslationTolerance &&
      capture.shadow_input_unchanged && capture.audit_contract_exact &&
      capture.device_contract_exact && capture.repeated_device_exact;
  return result;
}

void srr_configure_determinism() {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  auto &context = at::globalContext();
  context.setDeterministicCuDNN(true);
  context.setDeterministicAlgorithms(true, false);
  context.setAllowTF32CuBLAS(false);
  context.setAllowTF32CuDNN(false);
}

[[nodiscard]] bool srr_cublas_workspace_exact() {
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  return workspace != nullptr && std::string(workspace) == ":4096:8";
}

struct SrrEnvironmentReceipt {
  int cpu_threads{0};
  int cpu_interop_threads{0};
  bool deterministic_algorithms{false};
  bool deterministic_warn_only{true};
  bool deterministic_cudnn{false};
  bool tf32_cublas_disabled{false};
  bool tf32_cudnn_disabled{false};
  bool cublas_workspace_exact{false};
  bool pass{false};
};

[[nodiscard]] SrrEnvironmentReceipt srr_environment_receipt() {
  auto &context = at::globalContext();
  SrrEnvironmentReceipt result{};
  result.cpu_threads = at::get_num_threads();
  result.cpu_interop_threads = at::get_num_interop_threads();
  result.deterministic_algorithms = context.deterministicAlgorithms();
  result.deterministic_warn_only =
      context.deterministicAlgorithmsWarnOnly();
  result.deterministic_cudnn = context.deterministicCuDNN();
  result.tf32_cublas_disabled = !context.allowTF32CuBLAS();
  result.tf32_cudnn_disabled = !context.allowTF32CuDNN();
  result.cublas_workspace_exact = srr_cublas_workspace_exact();
  result.pass = result.cpu_threads == 1 && result.cpu_interop_threads == 1 &&
                result.deterministic_algorithms &&
                !result.deterministic_warn_only &&
                result.deterministic_cudnn &&
                result.tf32_cublas_disabled &&
                result.tf32_cudnn_disabled && result.cublas_workspace_exact;
  return result;
}

void srr_emit_environment(const std::string &prefix,
                          const SrrEnvironmentReceipt &receipt) {
  std::cout << prefix << ".device=cuda:0\n";
  std::cout << prefix << ".dtype=float32\n";
  std::cout << prefix << ".cpu_threads=" << receipt.cpu_threads << '\n';
  std::cout << prefix << ".cpu_interop_threads="
            << receipt.cpu_interop_threads << '\n';
  std::cout << prefix << ".deterministic_algorithms="
            << receipt.deterministic_algorithms << '\n';
  std::cout << prefix << ".deterministic_warn_only="
            << receipt.deterministic_warn_only << '\n';
  std::cout << prefix << ".deterministic_cudnn="
            << receipt.deterministic_cudnn << '\n';
  std::cout << prefix << ".tf32_cublas_disabled="
            << receipt.tf32_cublas_disabled << '\n';
  std::cout << prefix << ".tf32_cudnn_disabled="
            << receipt.tf32_cudnn_disabled << '\n';
  std::cout << prefix << ".cublas_workspace_exact="
            << receipt.cublas_workspace_exact << '\n';
  std::cout << prefix << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] bool
srr_order_permutation_balanced(const torch::Tensor &permutation) {
  const auto values =
      permutation.to(torch::kCPU, torch::kInt64).contiguous().reshape({-1});
  if (values.numel() == 0 || values.numel() % 2 != 0) {
    return false;
  }
  const auto access = values.accessor<int64_t, 1>();
  int64_t even = 0;
  int64_t odd = 0;
  for (int64_t index = 0; index < values.numel(); ++index) {
    if ((access[index] & 1) == 0) {
      ++even;
    } else {
      ++odd;
    }
  }
  return even == values.numel() / 2 && odd == values.numel() / 2;
}

void srr_emit_permutation_receipt(const std::string &prefix,
                                  const torch::Tensor &permutation) {
  const auto receipt = rssm_permutation_receipt(permutation);
  std::cout << prefix << ".rows=" << receipt.rows << '\n';
  emit_fingerprint(prefix + ".hash", receipt.hash);
  std::cout << prefix << ".pass=" << receipt.pass << '\n';
}

// Preflight is deliberately input-only: unlike generate_dataset(), this
// helper never allocates or writes scientific targets.  It reproduces the
// inherited deterministic feature/mask construction for out-of-band groups.
[[nodiscard]] Dataset srr_generate_preflight_inputs(int64_t group_begin,
                                                    int64_t groups,
                                                    int64_t view = 0) {
  auto data = torch::empty({groups, kChannels, kHistory, kFeatures},
                           torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::ones({groups, kChannels, kHistory, kFeatures},
                          torch::TensorOptions().dtype(torch::kBool));
  auto x = data.accessor<float, 4>();
  auto valid = mask.accessor<bool, 4>();

  for (int64_t row = 0; row < groups; ++row) {
    const int64_t group = group_begin + row;
    const auto factors = factors_for(group, false);
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      for (int64_t history = 0; history < kHistory; ++history) {
        const double current =
            observed_value(factors, group, channel, history, view);
        const double previous =
            observed_value(factors, group, channel, history - 1, view);
        double mean3 = 0.0;
        double square3 = 0.0;
        double mean8 = 0.0;
        double square8 = 0.0;
        for (int64_t offset = 0; offset < 8; ++offset) {
          const double value =
              observed_value(factors, group, channel, history - offset, view);
          mean8 += value;
          square8 += value * value;
          if (offset < 3) {
            mean3 += value;
            square3 += value * value;
          }
        }
        mean3 /= 3.0;
        mean8 /= 8.0;
        const double std3 =
            std::sqrt(std::max(0.0, square3 / 3.0 - mean3 * mean3));
        const double std8 =
            std::sqrt(std::max(0.0, square8 / 8.0 - mean8 * mean8));
        const int64_t other = (channel + 1) % kChannels;
        const double other_value =
            observed_value(factors, group, other, history, view);
        const std::array<double, kFeatures> features{
            current, current - previous, std::fabs(current), current * current,
            mean3, std3, mean8, std8, current * other_value};
        for (int64_t feature = 0; feature < kFeatures; ++feature) {
          x[row][channel][history][feature] =
              static_cast<float>(features[static_cast<std::size_t>(feature)]);
          if (view == 1 &&
              uniform01(key(group, 40 + channel,
                            history * kFeatures + feature, view)) < 0.04) {
            valid[row][channel][history][feature] = false;
          }
        }
      }
    }
  }
  return {.data = std::move(data),
          .mask = std::move(mask),
          .target = torch::Tensor{},
          .group_begin = group_begin};
}

void srr_validate_preflight_inputs(const Dataset &dataset) {
  if (dataset.data.dim() != 4 || dataset.data.size(1) != kChannels ||
      dataset.data.size(2) != kHistory ||
      dataset.data.size(3) != kFeatures ||
      dataset.mask.sizes() != dataset.data.sizes() ||
      dataset.target.defined() ||
      !torch::isfinite(dataset.data).all().item<bool>()) {
    throw std::runtime_error("SRR preflight input-only contract failed");
  }
}

int run_srr_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("SRR preflight requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "SRR preflight accepts no training-step/seed/weak-view override");
  }
  const torch::Device device(torch::kCUDA, 0);
  srr_configure_determinism();
  const auto environment = srr_environment_receipt();

  auto normalizer_rows = srr_generate_preflight_inputs(4500000, 32);
  auto capture_rows = srr_generate_preflight_inputs(4600000, 101, 1);
  const auto normalization = fit_normalization(normalizer_rows);
  normalize(normalizer_rows, normalization);
  normalize(capture_rows, normalization);
  srr_validate_preflight_inputs(normalizer_rows);
  srr_validate_preflight_inputs(capture_rows);

  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  const auto token_layout = psm_token_layout_receipt();
  const auto partitions = psm_partition_receipt();
  const auto rssm_projection = rssm_make_token_projection();
  const auto psm_projection = psm_make_projection(rssm_projection);
  const auto projection =
      psm_projection_receipt(rssm_projection, psm_projection);
  const auto shuffle_train =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto shuffle_validation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto shuffle_test =
      rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_validation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_test =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  bool permutations_valid =
      rssm_permutation_receipt(shuffle_train).pass &&
      rssm_permutation_receipt(shuffle_validation).pass &&
      rssm_permutation_receipt(shuffle_test).pass &&
      rssm_permutation_receipt(order_validation).pass &&
      rssm_permutation_receipt(order_test).pass;
  for (const auto &permutation : order_fit_permutations) {
    permutations_valid =
        permutations_valid && rssm_permutation_receipt(permutation).pass;
  }
  bool order_shuffle_balanced =
      srr_order_permutation_balanced(order_validation) &&
      srr_order_permutation_balanced(order_test);
  for (const auto &permutation : order_fit_permutations) {
    order_shuffle_balanced =
        order_shuffle_balanced && srr_order_permutation_balanced(permutation);
  }
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);

  set_paired_rng(1701, device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
  const bool initial_mode = model->is_training();
  const auto parameters_before = snapshot_parameters(model);
  const uint64_t parameter_hash_before =
      rssm_parameter_snapshot_hash(parameters_before);
  const auto generator_before = current_generator_state_snapshot(device);
  const auto first = srr_capture_once(model, capture_rows, device,
                                      psm_projection, true);
  const auto second = srr_capture_once(model, capture_rows, device,
                                       psm_projection, true);
  const auto generator_after = current_generator_state_snapshot(device);
  const auto parameters_after = snapshot_parameters(model);
  const uint64_t parameter_hash_after =
      rssm_parameter_snapshot_hash(parameters_after);
  const bool repeated_capture_exact = srr_capture_exact(first, second);
  const bool parameters_exact =
      parameter_max_abs_diff(model, parameters_before) == 0.0 &&
      parameter_hash_before == parameter_hash_after;
  const bool generator_exact =
      generator_state_snapshot_equal(generator_before, generator_after);
  const bool mode_exact = model->is_training() == initial_mode;
  const auto features =
      srr_feature_set(first, psm_projection, rssm_projection);
  const bool mechanics =
      environment.pass && tokenizer_plan.pass && token_layout.pass &&
      partitions.pass && projection.pass &&
      permutations_valid && order_shuffle_balanced && bootstrap_valid &&
      first.parent.public_sandwich_exact &&
      second.parent.public_sandwich_exact &&
      first.parent.direct_encoder_exact &&
      second.parent.direct_encoder_exact &&
      first.parent.production_order_exact &&
      second.parent.production_order_exact &&
      first.parent.cardinality_exact && second.parent.cardinality_exact &&
      psm_capture_layout_exact(first.parent, token_layout) &&
      psm_capture_layout_exact(second.parent, token_layout) &&
      repeated_capture_exact && parameters_exact && generator_exact &&
      mode_exact && features.pass;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.srr_preflight.v1\n";
  std::cout << "srr.preflight.scientific_rows_used=false\n";
  std::cout << "srr.preflight.target_constructed=false\n";
  std::cout << "optimizer_constructed=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "backward_calls=0\n";
  std::cout << "scientific_probe_fits=0\n";
  srr_emit_environment("srr.preflight.environment", environment);
  std::cout << "srr.preflight.tokenizer_plan_pass=" << tokenizer_plan.pass
            << '\n';
  std::cout << "srr.preflight.token_layout_pass=" << token_layout.pass
            << '\n';
  emit_fingerprint("srr.preflight.token_layout_hash",
                   token_layout.layout_hash);
  std::cout << "srr.preflight.partition_pass=" << partitions.pass << '\n';
  std::cout << "srr.preflight.projection_pass=" << projection.pass << '\n';
  emit_fingerprint("srr.preflight.projection_hash",
                   hash_tensor_stable_bytes(psm_projection));
  std::cout << "srr.preflight.public_sandwich_exact="
            << first.parent.public_sandwich_exact << '\n';
  std::cout << "srr.preflight.direct_encoder_exact="
            << first.parent.direct_encoder_exact << '\n';
  std::cout << "srr.preflight.shadow_input_unchanged="
            << first.shadow_input_unchanged << '\n';
  std::cout << "srr.preflight.audit_contract_exact="
            << first.audit_contract_exact << '\n';
  std::cout << "srr.preflight.device_contract_exact="
            << first.device_contract_exact << '\n';
  std::cout << "srr.preflight.repeated_device_exact="
            << first.repeated_device_exact << '\n';
  std::cout << "srr.preflight.repeated_capture_exact="
            << repeated_capture_exact << '\n';
  std::cout << "srr.preflight.offline_bytes_exact="
            << features.offline_bytes_exact << '\n';
  std::cout << "srr.preflight.offline_equivalence_max_abs="
            << features.offline_equivalence_max_abs << '\n';
  std::cout << "srr.preflight.device_translation_max_abs="
            << features.device_translation_max_abs << '\n';
  std::cout << "srr.preflight.device_relative_l2="
            << features.device_relative_l2 << '\n';
  std::cout << "srr.preflight.parameters_exact=" << parameters_exact << '\n';
  std::cout << "srr.preflight.generator_exact=" << generator_exact << '\n';
  std::cout << "srr.preflight.model_mode_exact=" << mode_exact << '\n';
  std::cout << "srr.preflight.permutations_valid=" << permutations_valid
            << '\n';
  std::cout << "srr.preflight.order_shuffle_balanced="
            << order_shuffle_balanced << '\n';
  emit_fingerprint("srr.preflight.bootstrap_table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "srr.preflight.bootstrap_valid=" << bootstrap_valid << '\n';
  srr_emit_permutation_receipt("srr.preflight.permutation.continuous_fit",
                               shuffle_train);
  srr_emit_permutation_receipt(
      "srr.preflight.permutation.continuous_validation",
      shuffle_validation);
  srr_emit_permutation_receipt("srr.preflight.permutation.continuous_test",
                               shuffle_test);
  for (std::size_t index = 0; index < order_fit_permutations.size(); ++index) {
    srr_emit_permutation_receipt(
        "srr.preflight.permutation.order_fit_n_" +
            std::to_string(kRssmSampleLadder[index]),
        order_fit_permutations[index]);
  }
  srr_emit_permutation_receipt(
      "srr.preflight.permutation.order_validation", order_validation);
  srr_emit_permutation_receipt("srr.preflight.permutation.order_test",
                               order_test);
  std::cout << "srr.preflight.ridge_fixture_executed=false\n";
  emit_fingerprint("srr.preflight.parameter_hash_before",
                   parameter_hash_before);
  emit_fingerprint("srr.preflight.parameter_hash_after",
                   parameter_hash_after);
  std::cout << "srr.preflight.authoritative_command=CUBLAS_WORKSPACE_CONFIG=:4096:8 .build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair --experiment structured-readout-repair --device cuda\n";
  std::cout << "srr.preflight.pass=" << mechanics << '\n';
  std::cout << "training_authorized=false\n";
  std::cout << "augmentation_change_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_production_repair_authorized=false\n";
  return mechanics ? 0 : 3;
}

constexpr std::size_t kSrrDatasetCount = 6;
constexpr std::size_t kSrrTrainDataset = 0;
constexpr std::size_t kSrrValidationDataset = 1;
constexpr std::size_t kSrrTestDataset = 2;
constexpr std::size_t kSrrReversedTrainDataset = 3;
constexpr std::size_t kSrrReversedValidationDataset = 4;
constexpr std::size_t kSrrReversedTestDataset = 5;
constexpr std::array<const char *, kSrrDatasetCount> kSrrDatasetNames{
    "probe_train", "probe_validation", "test", "reversed_train",
    "reversed_validation", "reversed_test"};

constexpr std::array<std::array<uint64_t, kSrrDatasetCount>, 3>
    kExpectedChannelFeatureHashes{{
        {0xfe6e0bbf68d0e2bbULL, 0x8b927bbea2a05e18ULL,
         0x24e78b9508ca9ad5ULL, 0x08d4a488856bfbfaULL,
         0x44009c8af1d66aadULL, 0x3e3679b5fdaad6f4ULL},
        {0x62784cdaa6f2732eULL, 0xf0b5c7c7af09fff5ULL,
         0xea9be90953eedf4aULL, 0x46a4d684c810882fULL,
         0xf3e44c0c317ff150ULL, 0xe4fd7eb5e2c72f03ULL},
        {0x424c78606a71382dULL, 0xf82f08eaec0c6213ULL,
         0x37a741a2fbc7e4dfULL, 0x45d385b28a1e4166ULL,
         0xd1892879d98f20c3ULL, 0x602292ee53e190fbULL},
    }};
constexpr std::array<std::array<uint64_t, kSrrDatasetCount>, 3>
    kExpectedCdsbFeatureHashes{{
        {0xfd676633045bf4ebULL, 0xf5b1f42cff608cf8ULL,
         0x91a23982f8cad89eULL, 0xbcd5d393929ff872ULL,
         0xa29adca524fa637eULL, 0x49302fd5dca87d0fULL},
        {0xee2ebca7b09e54dfULL, 0x7e63ad5dd9a2f37fULL,
         0x624cdc1ea7ef3613ULL, 0x613ca3e7e2c85d9aULL,
         0x2ea912456e086a43ULL, 0x4b11e03466aff5e7ULL},
        {0x5faebeb61db5bc58ULL, 0x7bd13bbd5cce225cULL,
         0x991a19ec290ac589ULL, 0x16a8bebec4dc971aULL,
         0x71ba6bd2a496c0e2ULL, 0x613fec93300fbfaaULL},
    }};
constexpr std::array<std::array<uint64_t, kSrrDatasetCount>, 3>
    kExpectedEncoderFeatureHashes{{
        {0x5b7c5c6867b5ff74ULL, 0xe02960bf326459c0ULL,
         0x82a99b7c3efc300aULL, 0x38960d1504b0326cULL,
         0xbd6a4dfc66c366eeULL, 0xb9f97e5840ed2695ULL},
        {0x073f00492d9e8fb5ULL, 0x6cf977e339103618ULL,
         0x35590246ab888f96ULL, 0x8368c27e52f3d040ULL,
         0x902bf5f0d4f7c47dULL, 0x3c7396f398ba01f9ULL},
        {0x5a12b15978382cdcULL, 0x73060fa5b72f84b8ULL,
         0x04a7a048ff5eaef2ULL, 0x50c222b643466356ULL,
         0x5a774b3f3046eb6dULL, 0xb54ad14f242f294dULL},
    }};

// C, canonical D, and E in that order.  These are literal PSM-1 endpoints.
constexpr std::array<std::array<double, 3>, 3> kExpectedAulc{{
    {0.51029806802395417, 0.51214336890601575, 0.53534605970628402},
    {0.60310336284296084, 0.58334872682440442, 0.59273298270071495},
    {0.59528657538535634, 0.57992865599289245, 0.57468040681240407},
}};
constexpr std::array<std::array<double, 3>, 3> kExpectedOrderAulc{{
    {0.56884765625, 0.5849609375, 0.56982421875},
    {0.92236328125, 0.93701171875, 0.92919921875},
    {0.97021484375, 0.93603515625, 0.96435546875},
}};
constexpr std::array<std::array<double, 4>, 3> kExpectedFamilyAulc{{
    {0.4045698296448732, 0.47928054446074547, 0.53872892886456747,
     0.65447069254481927},
    {0.59464087875418847, 0.508459546484064,
     0.45963705102330704, 0.80950928689588075},
    {0.57462964827738061, 0.50661709817337297,
     0.43553250772990609, 0.81641493007354382},
}};

constexpr std::array<SrrArm, 3> kSrrReferenceArms{
    SrrArm::channel, SrrArm::offline_cdsb, SrrArm::encoder};

[[nodiscard]] uint64_t srr_expected_feature_hash(
    SrrArm arm, std::size_t seed, std::size_t dataset) {
  switch (arm) {
  case SrrArm::channel:
    return kExpectedChannelFeatureHashes[seed][dataset];
  case SrrArm::offline_cdsb:
    return kExpectedCdsbFeatureHashes[seed][dataset];
  case SrrArm::encoder:
    return kExpectedEncoderFeatureHashes[seed][dataset];
  case SrrArm::shadow:
    break;
  }
  throw std::runtime_error("SRR shadow has no parent feature hash");
}

struct SrrSeedMeasurement {
  int64_t seed{0};
  std::array<ProbeCurve, kSrrArmCount> real{};
  std::array<ProbeCurve, kSrrArmCount> shuffled{};
  std::array<RssmOrderCurve, kSrrArmCount> order{};
  std::array<RssmOrderCurve, kSrrArmCount> order_shuffled{};
  bool public_sandwich_exact{true};
  bool direct_encoder_exact{true};
  bool repeated_capture_exact{true};
  bool parameters_and_rng_unchanged{true};
  bool production_order_exact{true};
  bool cardinality_exact{true};
  bool token_layout_exact{true};
  bool shadow_contracts_exact{true};
  bool parent_feature_hashes_exact{true};
};

using SrrMeasurements =
    std::array<SrrSeedMeasurement, psm_gate::kSeedCount>;

[[nodiscard]] RssmCurveBySeed
srr_curves_for(const SrrMeasurements &measurements, SrrArm arm,
               bool shuffled) {
  RssmCurveBySeed result{};
  for (std::size_t seed = 0; seed < result.size(); ++seed) {
    result[seed] = shuffled
                       ? measurements[seed].shuffled[srr_index(arm)]
                       : measurements[seed].real[srr_index(arm)];
  }
  return result;
}

[[nodiscard]] RssmOrderCurveBySeed
srr_order_curves_for(const SrrMeasurements &measurements, SrrArm arm,
                     bool shuffled) {
  RssmOrderCurveBySeed result{};
  for (std::size_t seed = 0; seed < result.size(); ++seed) {
    result[seed] = shuffled
                       ? measurements[seed].order_shuffled[srr_index(arm)]
                       : measurements[seed].order[srr_index(arm)];
  }
  return result;
}

void srr_emit_tensor_csv(const std::string &key, const torch::Tensor &tensor) {
  const auto flat =
      tensor.detach().to(torch::kCPU, torch::kFloat64).contiguous().view({-1});
  const auto values = flat.accessor<double, 1>();
  std::cout << key << '=';
  for (int64_t index = 0; index < flat.numel(); ++index) {
    if (index != 0) {
      std::cout << ',';
    }
    std::cout << values[index];
  }
  std::cout << '\n';
}

void srr_emit_curve_predictions(const std::string &prefix,
                                const ProbeCurve &curve) {
  for (const auto &point : curve.points) {
    srr_emit_tensor_csv(prefix + ".n_" + std::to_string(point.samples) +
                            ".prediction_csv",
                        point.prediction);
  }
}

void srr_emit_order_predictions(const std::string &prefix,
                                const RssmOrderCurve &curve) {
  for (const auto &point : curve.points) {
    srr_emit_tensor_csv(prefix + ".n_" + std::to_string(point.samples) +
                            ".prediction_csv",
                        point.prediction);
  }
}

[[nodiscard]] bool srr_displayed_reference_scalars_exact(
    const SrrMeasurements &measurements) {
  bool exact = true;
  for (std::size_t reference = 0; reference < kSrrReferenceArms.size();
       ++reference) {
    const auto arm = kSrrReferenceArms[reference];
    std::array<double, kFamilies> family_mean{};
    for (std::size_t seed = 0; seed < measurements.size(); ++seed) {
      exact = exact &&
              psm_close(measurements[seed].real[srr_index(arm)].area,
                        kExpectedAulc[reference][seed]) &&
              psm_close(measurements[seed].order[srr_index(arm)].area,
                        kExpectedOrderAulc[reference][seed]);
      const auto families =
          rssm_family_areas(measurements[seed].real[srr_index(arm)]);
      for (std::size_t family = 0; family < family_mean.size(); ++family) {
        family_mean[family] += families[family];
      }
    }
    for (std::size_t family = 0; family < family_mean.size(); ++family) {
      family_mean[family] /= static_cast<double>(measurements.size());
      exact = exact &&
              psm_close(family_mean[family],
                        kExpectedFamilyAulc[reference][family]);
    }
  }
  return exact;
}

int run_srr_impl(const Options &options, bool &attempt_consumed) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("SRR authoritative mode requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "SRR authoritative mode accepts no training-step/seed/weak-view "
        "override");
  }
  // The manifest is sealed after the binaries and preflight are final.  Read
  // and hash those exact bytes before constructing any scientific dataset so
  // the authoritative log is bound to the pre-run seal it actually used.
  const std::string prerun_manifest =
      srr_read_binary_file(std::filesystem::path(kSrrPrerunManifestPath));
  if (prerun_manifest.empty()) {
    throw std::runtime_error("SRR pre-run manifest is empty");
  }
  const std::string prerun_manifest_sha256 =
      digest::sha256_hex(prerun_manifest);
  const torch::Device device(torch::kCUDA, 0);
  srr_configure_determinism();
  const auto environment = srr_environment_receipt();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.srr.v1\n";
  std::cout << "experiment=structured-readout-repair\n";
  std::cout << "device=cuda:0\n";
  std::cout << "dtype=float32\n";
  std::cout << "srr.prerun_manifest.bytes=" << prerun_manifest.size()
            << '\n';
  std::cout << "srr.prerun_manifest.sha256=" << prerun_manifest_sha256
            << '\n';
  std::cout << "optimizer_constructed=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "backward_calls=0\n";
  std::cout << "training_loop_calls=0\n";
  std::cout << "augmentation_launcher_calls=0\n";
  std::cout << "end_to_end_calls=0\n";
  srr_emit_environment("srr.environment", environment);

  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  const auto token_layout = psm_token_layout_receipt();
  const auto partitions = psm_partition_receipt();
  const auto rssm_projection = rssm_make_token_projection();
  const auto psm_projection = psm_make_projection(rssm_projection);
  const auto projection =
      psm_projection_receipt(rssm_projection, psm_projection);
  emit_fingerprint("srr.projection.rssm_hash",
                   hash_tensor_stable_bytes(rssm_projection));
  emit_fingerprint("srr.projection.psm_hash",
                   hash_tensor_stable_bytes(psm_projection));
  emit_fingerprint("srr.token_layout.hash", token_layout.layout_hash);
  std::cout << "srr.tokenizer_plan.pass=" << tokenizer_plan.pass << '\n';
  std::cout << "srr.token_layout.pass=" << token_layout.pass << '\n';
  std::cout << "srr.partitions.pass=" << partitions.pass << '\n';
  std::cout << "srr.projection.pass=" << projection.pass << '\n';

  auto normalizer = generate_dataset(0, 256);
  auto probe_train = generate_dataset(1000000, 256);
  auto probe_validation = generate_dataset(2000000, 128);
  auto test = generate_dataset(3000000, 256);
  const std::array<uint64_t, 4> mask_hashes_before{
      hash_tensor_stable_bytes(normalizer.mask),
      hash_tensor_stable_bytes(probe_train.mask),
      hash_tensor_stable_bytes(probe_validation.mask),
      hash_tensor_stable_bytes(test.mask)};
  const std::array<uint64_t, 4> target_hashes_before{
      hash_tensor_stable_bytes(normalizer.target),
      hash_tensor_stable_bytes(probe_train.target),
      hash_tensor_stable_bytes(probe_validation.target),
      hash_tensor_stable_bytes(test.target)};
  rssm_emit_dataset_identity("srr.data.normalizer", normalizer,
                             "unnormalized");
  rssm_emit_dataset_identity("srr.data.probe_train", probe_train,
                             "unnormalized");
  rssm_emit_dataset_identity("srr.data.probe_validation", probe_validation,
                             "unnormalized");
  rssm_emit_dataset_identity("srr.data.test", test, "unnormalized");
  const auto normalization = fit_normalization(normalizer);
  for (Dataset *dataset :
       {&normalizer, &probe_train, &probe_validation, &test}) {
    normalize(*dataset, normalization);
    validate_dataset(*dataset);
  }
  auto reversed_train = rssm_reversed_dataset(probe_train);
  auto reversed_validation = rssm_reversed_dataset(probe_validation);
  auto reversed_test = rssm_reversed_dataset(test);
  rssm_emit_dataset_identity("srr.data.normalizer", normalizer, "normalized");
  rssm_emit_dataset_identity("srr.data.probe_train", probe_train,
                             "normalized");
  rssm_emit_dataset_identity("srr.data.probe_validation", probe_validation,
                             "normalized");
  rssm_emit_dataset_identity("srr.data.test", test, "normalized");
  rssm_emit_dataset_identity("srr.data.reversed_train", reversed_train,
                             "normalized");
  rssm_emit_dataset_identity("srr.data.reversed_validation",
                             reversed_validation, "normalized");
  rssm_emit_dataset_identity("srr.data.reversed_test", reversed_test,
                             "normalized");
  const std::array<const Dataset *, 4> normalized{
      &normalizer, &probe_train, &probe_validation, &test};
  bool normalization_preserved_identity = true;
  for (std::size_t index = 0; index < normalized.size(); ++index) {
    normalization_preserved_identity =
        normalization_preserved_identity &&
        mask_hashes_before[index] ==
            hash_tensor_stable_bytes(normalized[index]->mask) &&
        target_hashes_before[index] ==
            hash_tensor_stable_bytes(normalized[index]->target);
  }
  const bool dataset_identity_exact =
      normalization_preserved_identity && normalizer.group_begin == 0 &&
      normalizer.data.size(0) == 256 && probe_train.group_begin == 1000000 &&
      probe_train.data.size(0) == 256 &&
      probe_validation.group_begin == 2000000 &&
      probe_validation.data.size(0) == 128 && test.group_begin == 3000000 &&
      test.data.size(0) == 256 &&
      rssm_group_pair_exact(probe_train, reversed_train) &&
      rssm_group_pair_exact(probe_validation, reversed_validation) &&
      rssm_group_pair_exact(test, reversed_test);
  std::cout << "srr.data.normalization_preserved_identity="
            << normalization_preserved_identity << '\n';
  std::cout << "srr.data.identity_exact=" << dataset_identity_exact << '\n';
  emit_fingerprint("srr.normalization.mean_hash",
                   hash_tensor_stable_bytes(normalization.mean));
  emit_fingerprint("srr.normalization.inv_std_hash",
                   hash_tensor_stable_bytes(normalization.inv_std));

  const std::array<const Dataset *, kSrrDatasetCount> datasets{
      &probe_train, &probe_validation, &test, &reversed_train,
      &reversed_validation, &reversed_test};
  const auto shuffle_train =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto shuffle_validation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto shuffle_test =
      rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto shuffled_train_target =
      probe_train.target.index_select(0, shuffle_train).contiguous();
  const auto shuffled_validation_target =
      probe_validation.target.index_select(0, shuffle_validation).contiguous();
  const auto shuffled_test_target =
      test.target.index_select(0, shuffle_test).contiguous();
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_fit_targets = rssm_order_fit_targets(nullptr);
  const auto shuffled_order_fit_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto order_validation_target = rssm_order_labels(128);
  const auto order_test_target = rssm_order_labels(256);
  const auto order_shuffle_validation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_shuffle_test =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  const auto shuffled_order_validation_target =
      order_validation_target.index_select(0, order_shuffle_validation)
          .contiguous();
  const auto shuffled_order_test_target =
      order_test_target.index_select(0, order_shuffle_test).contiguous();
  bool order_shuffle_balanced =
      rssm_order_target_balanced(shuffled_order_validation_target) &&
      rssm_order_target_balanced(shuffled_order_test_target);
  bool permutations_valid =
      rssm_permutation_receipt(shuffle_train).pass &&
      rssm_permutation_receipt(shuffle_validation).pass &&
      rssm_permutation_receipt(shuffle_test).pass &&
      rssm_permutation_receipt(order_shuffle_validation).pass &&
      rssm_permutation_receipt(order_shuffle_test).pass;
  for (std::size_t index = 0; index < order_fit_permutations.size(); ++index) {
    permutations_valid =
        permutations_valid &&
        rssm_permutation_receipt(order_fit_permutations[index]).pass;
    order_shuffle_balanced =
        order_shuffle_balanced &&
        rssm_order_target_balanced(shuffled_order_fit_targets[index]);
  }
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);
  const auto ridge = rssm_ridge_equivalence_fixture();
  std::cout << "srr.permutations_valid=" << permutations_valid << '\n';
  std::cout << "srr.order_shuffle_balanced=" << order_shuffle_balanced
            << '\n';
  emit_fingerprint("srr.bootstrap.table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "srr.bootstrap.valid=" << bootstrap_valid << '\n';
  std::cout << "srr.ridge.pass=" << ridge.pass << '\n';
  srr_emit_permutation_receipt("srr.permutation.continuous_fit",
                               shuffle_train);
  srr_emit_permutation_receipt("srr.permutation.continuous_validation",
                               shuffle_validation);
  srr_emit_permutation_receipt("srr.permutation.continuous_test",
                               shuffle_test);
  for (std::size_t index = 0; index < order_fit_permutations.size(); ++index) {
    srr_emit_permutation_receipt(
        "srr.permutation.order_fit_n_" +
            std::to_string(kRssmSampleLadder[index]),
        order_fit_permutations[index]);
  }
  srr_emit_permutation_receipt("srr.permutation.order_validation",
                               order_shuffle_validation);
  srr_emit_permutation_receipt("srr.permutation.order_test",
                               order_shuffle_test);

  SrrMeasurements measurements{};
  std::array<std::array<SrrFeatureSet, kSrrDatasetCount>,
             psm_gate::kSeedCount>
      features_by_seed{};
  bool all_public_exact = true;
  bool all_direct_exact = true;
  bool all_repeated_exact = true;
  bool all_parameters_rng_exact = true;
  bool all_order_exact = true;
  bool all_cardinality_exact = true;
  bool all_capture_layout_exact = true;
  bool all_shadow_inputs_unchanged = true;
  bool all_audit_contracts = true;
  bool all_device_contracts = true;
  bool all_repeated_device_exact = true;
  bool all_device_capture_exact = true;
  bool all_parent_feature_hashes = true;
  bool all_offline_bytes_exact = true;
  bool cross_seed_token_structure_exact = true;
  bool metadata_plan_exact = true;
  bool metadata_initialized = false;
  uint64_t metadata_reference = 0;
  std::array<uint64_t, kSrrDatasetCount> token_mask_references{};
  double offline_equivalence_max_abs = 0.0;
  double device_translation_max_abs = 0.0;
  double device_relative_l2_max = 0.0;

  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    measurement.seed = kAttributionSeeds[seed_index];
    set_paired_rng(measurement.seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
    const bool initial_mode = model->is_training();
    const auto parameters_before = snapshot_parameters(model);
    const uint64_t parameter_hash_before =
        rssm_parameter_snapshot_hash(parameters_before);
    const auto generator_before = current_generator_state_snapshot(device);
    for (std::size_t dataset_index = 0; dataset_index < datasets.size();
         ++dataset_index) {
      const auto first = srr_capture_once(model, *datasets[dataset_index],
                                          device, psm_projection, true);
      const auto second = srr_capture_once(model, *datasets[dataset_index],
                                           device, psm_projection, true);
      measurement.public_sandwich_exact =
          measurement.public_sandwich_exact &&
          first.parent.public_sandwich_exact &&
          second.parent.public_sandwich_exact;
      measurement.direct_encoder_exact =
          measurement.direct_encoder_exact &&
          first.parent.direct_encoder_exact &&
          second.parent.direct_encoder_exact;
      measurement.repeated_capture_exact =
          measurement.repeated_capture_exact &&
          srr_local_capture_exact(first, second);
      measurement.production_order_exact =
          measurement.production_order_exact &&
          first.parent.production_order_exact &&
          second.parent.production_order_exact;
      measurement.cardinality_exact =
          measurement.cardinality_exact && first.parent.cardinality_exact &&
          second.parent.cardinality_exact;
      measurement.token_layout_exact =
          measurement.token_layout_exact &&
          psm_capture_layout_exact(first.parent, token_layout) &&
          psm_capture_layout_exact(second.parent, token_layout);
      measurement.shadow_contracts_exact =
          measurement.shadow_contracts_exact && first.shadow_input_unchanged &&
          first.audit_contract_exact && first.device_contract_exact &&
          first.repeated_device_exact && second.shadow_input_unchanged &&
          second.audit_contract_exact && second.device_contract_exact &&
          second.repeated_device_exact;
      all_shadow_inputs_unchanged =
          all_shadow_inputs_unchanged && first.shadow_input_unchanged &&
          second.shadow_input_unchanged;
      all_audit_contracts = all_audit_contracts && first.audit_contract_exact &&
                            second.audit_contract_exact;
      all_device_contracts =
          all_device_contracts && first.device_contract_exact &&
          second.device_contract_exact;
      all_repeated_device_exact =
          all_repeated_device_exact && first.repeated_device_exact &&
          second.repeated_device_exact;
      all_device_capture_exact =
          all_device_capture_exact && srr_device_capture_exact(first, second);
      if (seed_index == 0) {
        token_mask_references[dataset_index] =
            first.parent.token_mask_structure_hash;
      } else {
        cross_seed_token_structure_exact =
            cross_seed_token_structure_exact &&
            token_mask_references[dataset_index] ==
                first.parent.token_mask_structure_hash;
      }
      if (!metadata_initialized) {
        metadata_reference = first.parent.metadata_structure_hash;
        metadata_initialized = true;
      } else {
        metadata_plan_exact =
            metadata_plan_exact &&
            metadata_reference == first.parent.metadata_structure_hash;
      }
      auto features =
          srr_feature_set(first, psm_projection, rssm_projection);
      offline_equivalence_max_abs =
          std::max(offline_equivalence_max_abs,
                   features.offline_equivalence_max_abs);
      device_translation_max_abs =
          std::max(device_translation_max_abs,
                   features.device_translation_max_abs);
      device_relative_l2_max =
          std::max(device_relative_l2_max, features.device_relative_l2);
      all_offline_bytes_exact =
          all_offline_bytes_exact && features.offline_bytes_exact;
      measurement.shadow_contracts_exact =
          measurement.shadow_contracts_exact &&
          features.finite_and_shape_exact;
      features_by_seed[seed_index][dataset_index] = std::move(features);
      const std::string prefix =
          "srr.seed_" + std::to_string(measurement.seed) + ".capture." +
          kSrrDatasetNames[dataset_index];
      emit_fingerprint(prefix + ".encoder_hash",
                       first.parent.encoder_source_hash);
      emit_fingerprint(prefix + ".served_hash",
                       first.parent.served_source_hash);
      emit_fingerprint(prefix + ".metadata_structure_hash",
                       first.parent.metadata_structure_hash);
      emit_fingerprint(prefix + ".audit_hash", first.audit_float64_hash);
      emit_fingerprint(prefix + ".shadow_hash", first.device_float64_hash);
      std::cout << prefix << ".offline_equivalence_max_abs="
                << features_by_seed[seed_index][dataset_index]
                       .offline_equivalence_max_abs
                << '\n';
      std::cout << prefix << ".device_translation_max_abs="
                << features_by_seed[seed_index][dataset_index]
                       .device_translation_max_abs
                << '\n';
      for (const auto arm : kSrrReferenceArms) {
        const uint64_t observed = hash_tensor_stable_bytes(
            features_by_seed[seed_index][dataset_index]
                .arms[srr_index(arm)]
                .by_channel);
        const uint64_t expected =
            srr_expected_feature_hash(arm, seed_index, dataset_index);
        const bool exact = observed == expected;
        measurement.parent_feature_hashes_exact =
            measurement.parent_feature_hashes_exact && exact;
        emit_fingerprint(prefix + ".arm." + kSrrArmNames[srr_index(arm)] +
                             ".float64_hash",
                         observed);
        emit_fingerprint(prefix + ".arm." + kSrrArmNames[srr_index(arm)] +
                             ".expected_float64_hash",
                         expected);
        std::cout << prefix << ".arm." << kSrrArmNames[srr_index(arm)]
                  << ".parent_hash_exact=" << exact << '\n';
      }
      emit_fingerprint(
          prefix + ".arm.shadow.float64_hash",
          hash_tensor_stable_bytes(
              features_by_seed[seed_index][dataset_index]
                  .arms[srr_index(SrrArm::shadow)]
                  .by_channel));
    }
    const auto generator_after = current_generator_state_snapshot(device);
    const auto parameters_after = snapshot_parameters(model);
    const uint64_t parameter_hash_after =
        rssm_parameter_snapshot_hash(parameters_after);
    measurement.parameters_and_rng_unchanged =
        parameter_max_abs_diff(model, parameters_before) == 0.0 &&
        parameter_hash_before == parameter_hash_after &&
        generator_state_snapshot_equal(generator_before, generator_after) &&
        model->is_training() == initial_mode;
    all_public_exact =
        all_public_exact && measurement.public_sandwich_exact;
    all_direct_exact = all_direct_exact && measurement.direct_encoder_exact;
    all_repeated_exact =
        all_repeated_exact && measurement.repeated_capture_exact;
    all_parameters_rng_exact =
        all_parameters_rng_exact && measurement.parameters_and_rng_unchanged;
    all_order_exact =
        all_order_exact && measurement.production_order_exact;
    all_cardinality_exact =
        all_cardinality_exact && measurement.cardinality_exact;
    all_capture_layout_exact =
        all_capture_layout_exact && measurement.token_layout_exact;
    all_parent_feature_hashes =
        all_parent_feature_hashes && measurement.parent_feature_hashes_exact;
    const std::string prefix =
        "srr.seed_" + std::to_string(measurement.seed);
    std::cout << prefix << ".public_sandwich_exact="
              << measurement.public_sandwich_exact << '\n';
    std::cout << prefix << ".direct_encoder_exact="
              << measurement.direct_encoder_exact << '\n';
    std::cout << prefix << ".repeated_capture_exact="
              << measurement.repeated_capture_exact << '\n';
    std::cout << prefix << ".parameters_and_rng_unchanged="
              << measurement.parameters_and_rng_unchanged << '\n';
    std::cout << prefix << ".token_layout_exact="
              << measurement.token_layout_exact << '\n';
    std::cout << prefix << ".shadow_contracts_exact="
              << measurement.shadow_contracts_exact << '\n';
    std::cout << prefix << ".parent_feature_hashes_exact="
              << measurement.parent_feature_hashes_exact << '\n';
  }

  const bool local_prefit_mechanics =
      environment.pass && tokenizer_plan.pass && token_layout.pass &&
      partitions.pass && projection.pass &&
      dataset_identity_exact && permutations_valid && order_shuffle_balanced &&
      bootstrap_valid && ridge.pass && all_public_exact && all_direct_exact &&
      all_repeated_exact && all_parameters_rng_exact && all_order_exact &&
      all_cardinality_exact && all_capture_layout_exact &&
      all_shadow_inputs_unchanged && all_audit_contracts &&
      cross_seed_token_structure_exact &&
      metadata_plan_exact;
  const bool offline_prefit_reference =
      all_parent_feature_hashes && all_offline_bytes_exact &&
      offline_equivalence_max_abs <=
          srr_gate::kOfflineEquivalenceTolerance;
  const bool device_prefit_translation =
      all_device_contracts && all_repeated_device_exact &&
      all_device_capture_exact &&
      device_translation_max_abs <= srr_gate::kDeviceTranslationTolerance;
  const bool prefit_mechanics = local_prefit_mechanics &&
                                offline_prefit_reference &&
                                device_prefit_translation;
  std::cout << "srr.prefit.cross_seed_token_structure_exact="
            << cross_seed_token_structure_exact << '\n';
  std::cout << "srr.prefit.metadata_plan_exact=" << metadata_plan_exact
            << '\n';
  std::cout << "srr.prefit.offline_bytes_exact="
            << all_offline_bytes_exact << '\n';
  std::cout << "srr.prefit.parent_feature_hashes_exact="
            << all_parent_feature_hashes << '\n';
  std::cout << "srr.prefit.offline_equivalence_max_abs="
            << offline_equivalence_max_abs << '\n';
  std::cout << "srr.prefit.device_translation_max_abs="
            << device_translation_max_abs << '\n';
  std::cout << "srr.prefit.device_relative_l2_max="
            << device_relative_l2_max << '\n';
  std::cout << "srr.prefit.shadow_input_unchanged="
            << all_shadow_inputs_unchanged << '\n';
  std::cout << "srr.prefit.audit_contract_exact=" << all_audit_contracts
            << '\n';
  std::cout << "srr.prefit.device_contract_exact=" << all_device_contracts
            << '\n';
  std::cout << "srr.prefit.repeated_device_exact="
            << (all_repeated_device_exact && all_device_capture_exact) << '\n';
  std::cout << "srr.prefit.local_validity_pass=" << local_prefit_mechanics
            << '\n';
  std::cout << "srr.prefit.offline_reference_pass="
            << offline_prefit_reference << '\n';
  std::cout << "srr.prefit.device_translation_pass="
            << device_prefit_translation << '\n';
  std::cout << "srr.prefit.mechanics_pass=" << prefit_mechanics << '\n';
  if (!prefit_mechanics) {
    const char *failure_stage = !local_prefit_mechanics
                                    ? "invalid_mechanics"
                                    : (!offline_prefit_reference
                                           ? "offline_reference_failure"
                                           : "device_translation_failure");
    std::cout << "srr.prefit.failure_stage=" << failure_stage << '\n';
    std::cout << "srr.attempt.consumed=false\n";
    std::cout << "training_authorized=false\n";
    std::cout << "augmentation_change_authorized=false\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
    std::cout << "follow_on_production_repair_authorized=false\n";
    std::cout << "execution_status=srr_prefit_mechanics_failed\n";
    return 3;
  }

  srr_emit_tensor_csv("srr.audit.target.test_csv", test.target);
  srr_emit_tensor_csv("srr.audit.target.shuffled_test_csv",
                      shuffled_test_target);
  srr_emit_tensor_csv("srr.audit.target.order_test_csv", order_test_target);
  srr_emit_tensor_csv("srr.audit.target.shuffled_order_test_csv",
                      shuffled_order_test_target);
  attempt_consumed = true;
  std::cout << "srr.attempt.consumed=true\n" << std::flush;
  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    const auto &features = features_by_seed[seed_index];
    for (std::size_t arm_index = 0; arm_index < kSrrArmCount; ++arm_index) {
      const auto &train = features[kSrrTrainDataset].arms[arm_index].flat;
      const auto &validation =
          features[kSrrValidationDataset].arms[arm_index].flat;
      const auto &test_features =
          features[kSrrTestDataset].arms[arm_index].flat;
      measurement.real[arm_index] = rssm_probe_curve(
          train, validation, test_features, probe_train.target,
          probe_validation.target, test.target, false);
      measurement.shuffled[arm_index] = rssm_probe_curve(
          train, validation, test_features, shuffled_train_target,
          shuffled_validation_target, shuffled_test_target, false);
      const auto order_train = rssm_interleave_pairs(
          train, features[kSrrReversedTrainDataset].arms[arm_index].flat);
      const auto order_validation_features = rssm_interleave_pairs(
          validation,
          features[kSrrReversedValidationDataset].arms[arm_index].flat);
      const auto order_test_features = rssm_interleave_pairs(
          test_features,
          features[kSrrReversedTestDataset].arms[arm_index].flat);
      measurement.order[arm_index] = rssm_order_curve(
          order_train, order_validation_features, order_test_features,
          order_fit_targets, order_validation_target, order_test_target,
          false);
      measurement.order_shuffled[arm_index] = rssm_order_curve(
          order_train, order_validation_features, order_test_features,
          shuffled_order_fit_targets, shuffled_order_validation_target,
          shuffled_order_test_target, false);
      validate_probe_curve_finite(measurement.real[arm_index],
                                  "SRR real probe");
      validate_probe_curve_finite(measurement.shuffled[arm_index],
                                  "SRR shuffled probe");
      rssm_validate_order_curve_finite(measurement.order[arm_index],
                                       "SRR order probe");
      rssm_validate_order_curve_finite(
          measurement.order_shuffled[arm_index],
          "SRR shuffled order probe");
      const std::string prefix =
          "srr.seed_" + std::to_string(measurement.seed) + ".arm." +
          kSrrArmNames[arm_index];
      rssm_emit_probe_curve(prefix + ".probe",
                            measurement.real[arm_index]);
      rssm_emit_probe_curve(prefix + ".shuffled_probe",
                            measurement.shuffled[arm_index]);
      rssm_emit_order_curve(prefix + ".order_probe",
                            measurement.order[arm_index]);
      rssm_emit_order_curve(prefix + ".order_shuffled_probe",
                            measurement.order_shuffled[arm_index]);
      srr_emit_curve_predictions(prefix + ".probe",
                                 measurement.real[arm_index]);
      srr_emit_curve_predictions(prefix + ".shuffled_probe",
                                 measurement.shuffled[arm_index]);
      srr_emit_order_predictions(prefix + ".order_probe",
                                 measurement.order[arm_index]);
      srr_emit_order_predictions(prefix + ".order_shuffled_probe",
                                 measurement.order_shuffled[arm_index]);
    }
  }

  const bool endpoint_scalars_exact =
      srr_displayed_reference_scalars_exact(measurements);
  const auto channel_curves =
      srr_curves_for(measurements, SrrArm::channel, false);
  const auto offline_curves =
      srr_curves_for(measurements, SrrArm::offline_cdsb, false);
  const auto shadow_curves =
      srr_curves_for(measurements, SrrArm::shadow, false);
  const auto encoder_curves =
      srr_curves_for(measurements, SrrArm::encoder, false);
  const auto encoder_minus_channel = psm_continuous_input(rssm_contrast(
      encoder_curves, test.target, channel_curves, test.target,
      bootstrap_rows));
  const auto offline_minus_channel = psm_continuous_input(rssm_contrast(
      offline_curves, test.target, channel_curves, test.target,
      bootstrap_rows));
  const auto offline_minus_encoder = psm_continuous_input(rssm_contrast(
      offline_curves, test.target, encoder_curves, test.target,
      bootstrap_rows));
  const auto shadow_minus_channel = psm_continuous_input(rssm_contrast(
      shadow_curves, test.target, channel_curves, test.target,
      bootstrap_rows));
  const auto shadow_minus_encoder = psm_continuous_input(rssm_contrast(
      shadow_curves, test.target, encoder_curves, test.target,
      bootstrap_rows));

  std::array<bool, kSrrArmCount> continuous_shuffle_by_arm{};
  std::array<bool, kSrrArmCount> order_shuffle_by_arm{};
  std::array<psm_gate::OrderInput, kSrrArmCount> order_inputs{};
  for (std::size_t arm_index = 0; arm_index < kSrrArmCount; ++arm_index) {
    const auto arm = static_cast<SrrArm>(arm_index);
    const auto real = srr_curves_for(measurements, arm, false);
    const auto shuffled = srr_curves_for(measurements, arm, true);
    const auto real_summary =
        rssm_curve_interval(real, test.target, bootstrap_rows);
    const auto shuffled_summary =
        rssm_curve_interval(shuffled, shuffled_test_target, bootstrap_rows);
    const auto order = srr_order_curves_for(measurements, arm, false);
    const auto shuffled_order =
        srr_order_curves_for(measurements, arm, true);
    order_inputs[arm_index] =
        psm_order_input(order, order_test_target, bootstrap_rows);
    const auto shuffled_order_input = psm_order_input(
        shuffled_order, shuffled_order_test_target, bootstrap_rows);
    const bool arm_continuous_shuffle =
        shuffled_summary.point <= 0.02 &&
        shuffled_summary.interval.high <= 0.05;
    const bool arm_order_shuffle =
        shuffled_order_input.point <= 0.55 &&
        shuffled_order_input.high <= 0.60;
    continuous_shuffle_by_arm[arm_index] = arm_continuous_shuffle;
    order_shuffle_by_arm[arm_index] = arm_order_shuffle;
    const std::string prefix =
        "srr.summary.arm." + std::string(kSrrArmNames[arm_index]);
    std::cout << prefix << ".aulc.point=" << real_summary.point << '\n';
    std::cout << prefix << ".aulc.bootstrap_95_low="
              << real_summary.interval.low << '\n';
    std::cout << prefix << ".aulc.bootstrap_95_high="
              << real_summary.interval.high << '\n';
    std::array<double, kFamilies> family_mean{};
    for (std::size_t seed = 0; seed < real.size(); ++seed) {
      std::cout << prefix << ".aulc.seed_" << kAttributionSeeds[seed] << '='
                << real[seed].area << '\n';
      const auto families = rssm_family_areas(real[seed]);
      for (std::size_t family = 0; family < family_mean.size(); ++family) {
        family_mean[family] += families[family];
      }
    }
    for (std::size_t family = 0; family < family_mean.size(); ++family) {
      family_mean[family] /= static_cast<double>(real.size());
      std::cout << prefix << ".family_" << kFamilyNames[family] << "_aulc="
                << family_mean[family] << '\n';
    }
    std::cout << prefix << ".shuffled_aulc.point="
              << shuffled_summary.point << '\n';
    std::cout << prefix << ".shuffled_aulc.bootstrap_95_low="
              << shuffled_summary.interval.low << '\n';
    std::cout << prefix << ".shuffled_aulc.bootstrap_95_high="
              << shuffled_summary.interval.high << '\n';
    std::cout << prefix << ".continuous_shuffle_pass="
              << arm_continuous_shuffle << '\n';
    psm_emit_order(prefix + ".order", order_inputs[arm_index]);
    psm_emit_order(prefix + ".order_shuffled", shuffled_order_input);
    std::cout << prefix << ".order_shuffle_pass=" << arm_order_shuffle
              << '\n';
  }
  const bool continuous_shuffle_pass =
      std::all_of(continuous_shuffle_by_arm.begin(),
                  continuous_shuffle_by_arm.end(), [](bool value) {
                    return value;
                  });
  const bool order_shuffle_pass =
      std::all_of(order_shuffle_by_arm.begin(), order_shuffle_by_arm.end(),
                  [](bool value) { return value; });

  const auto channel_order =
      order_inputs[srr_index(SrrArm::channel)];
  const auto offline_order =
      order_inputs[srr_index(SrrArm::offline_cdsb)];
  const auto shadow_order = order_inputs[srr_index(SrrArm::shadow)];
  const auto encoder_order = order_inputs[srr_index(SrrArm::encoder)];
  const auto encoder_minus_channel_result =
      psm_gate::evaluate_continuous(encoder_minus_channel);
  const auto offline_minus_channel_result =
      psm_gate::evaluate_continuous(offline_minus_channel);
  const auto offline_minus_encoder_result =
      psm_gate::evaluate_continuous(offline_minus_encoder);
  const auto shadow_minus_channel_result =
      psm_gate::evaluate_continuous(shadow_minus_channel);
  const auto shadow_minus_encoder_result =
      psm_gate::evaluate_continuous(shadow_minus_encoder);
  const auto channel_order_result = psm_gate::evaluate_order(channel_order);
  const auto offline_order_result = psm_gate::evaluate_order(offline_order);
  const auto shadow_order_result = psm_gate::evaluate_order(shadow_order);
  const auto encoder_order_result = psm_gate::evaluate_order(encoder_order);
  const bool encoder_material_gain_over_channel =
      encoder_minus_channel_result.classification ==
      psm_gate::ContinuousClassification::material_gain;
  const bool channel_not_order_decodable =
      channel_order_result.classification !=
      psm_gate::OrderClassification::order_decodable;
  const bool encoder_order_decodable =
      encoder_order_result.classification ==
      psm_gate::OrderClassification::order_decodable;
  const bool offline_material_gain_over_channel =
      offline_minus_channel_result.classification ==
      psm_gate::ContinuousClassification::material_gain;
  const bool offline_noninferior_to_encoder =
      offline_minus_encoder_result.classification ==
          psm_gate::ContinuousClassification::noninferior ||
      offline_minus_encoder_result.classification ==
          psm_gate::ContinuousClassification::material_gain;
  const bool offline_order_decodable =
      offline_order_result.classification ==
      psm_gate::OrderClassification::order_decodable;
  const bool displayed_reference_subset_exact =
      endpoint_scalars_exact && encoder_material_gain_over_channel &&
      channel_not_order_decodable && encoder_order_decodable &&
      offline_material_gain_over_channel &&
      offline_noninferior_to_encoder && offline_order_decodable;
  const bool shadow_material_gain_over_channel =
      shadow_minus_channel_result.classification ==
      psm_gate::ContinuousClassification::material_gain;
  const bool shadow_noninferior_to_encoder =
      shadow_minus_encoder_result.classification ==
          psm_gate::ContinuousClassification::noninferior ||
      shadow_minus_encoder_result.classification ==
          psm_gate::ContinuousClassification::material_gain;
  const bool shadow_order_decodable =
      shadow_order_result.classification ==
      psm_gate::OrderClassification::order_decodable;
  const bool shadow_quality_candidate = shadow_material_gain_over_channel &&
                                        shadow_noninferior_to_encoder &&
                                        shadow_order_decodable;
  std::cout << "srr.reference.displayed_scalars_exact="
            << endpoint_scalars_exact << '\n';
  std::cout << "srr.reference.encoder_material_gain_over_channel="
            << encoder_material_gain_over_channel << '\n';
  std::cout << "srr.reference.channel_not_order_decodable="
            << channel_not_order_decodable << '\n';
  std::cout << "srr.reference.encoder_order_decodable="
            << encoder_order_decodable << '\n';
  std::cout << "srr.reference.offline_material_gain_over_channel="
            << offline_material_gain_over_channel << '\n';
  std::cout << "srr.reference.offline_noninferior_to_encoder="
            << offline_noninferior_to_encoder << '\n';
  std::cout << "srr.reference.offline_order_decodable="
            << offline_order_decodable << '\n';
  std::cout << "srr.reference.displayed_subset_exact="
            << displayed_reference_subset_exact << '\n';
  std::cout << "srr.reference.all_reference_keys_exact=unchecked\n";
  psm_emit_continuous("srr.summary.contrast.encoder_minus_channel",
                      encoder_minus_channel);
  psm_emit_continuous("srr.summary.contrast.offline_cdsb_minus_channel",
                      offline_minus_channel);
  psm_emit_continuous("srr.summary.contrast.offline_cdsb_minus_encoder",
                      offline_minus_encoder);
  psm_emit_continuous("srr.summary.contrast.shadow_minus_channel",
                      shadow_minus_channel);
  psm_emit_continuous("srr.summary.contrast.shadow_minus_encoder",
                      shadow_minus_encoder);

  srr_gate::GateInput gate_input{};
  gate_input.offline_minus_channel = offline_minus_channel;
  gate_input.offline_minus_encoder = offline_minus_encoder;
  gate_input.shadow_minus_channel = shadow_minus_channel;
  gate_input.shadow_minus_encoder = shadow_minus_encoder;
  gate_input.channel_order = channel_order;
  gate_input.offline_order = offline_order;
  gate_input.shadow_order = shadow_order;
  gate_input.encoder_order = encoder_order;
  gate_input.offline_equivalence_max_abs = offline_equivalence_max_abs;
  gate_input.device_translation_max_abs = device_translation_max_abs;
  gate_input.validity = {
      .no_training_or_end_to_end = true,
      .local_contracts_exact =
          dataset_identity_exact && all_public_exact && all_direct_exact &&
          all_repeated_exact && all_order_exact && all_cardinality_exact &&
          all_capture_layout_exact && cross_seed_token_structure_exact &&
          metadata_plan_exact && all_shadow_inputs_unchanged &&
          all_audit_contracts,
      .parameters_and_rng_unchanged = all_parameters_rng_exact,
      .partition_and_projection_valid = partitions.pass && projection.pass,
      .deterministic_tables_valid =
          permutations_valid && order_shuffle_balanced && bootstrap_valid &&
          ridge.pass && environment.pass,
      .manifest_exact = false,
      .parent_artifacts_exact = false,
      .parent_classification_exact = false,
      .parent_attempt_count_exact = false,
      .parent_audit_pass = false,
      .parent_authorizations_false = false,
      .all_reference_keys_exact = false,
      .offline_feature_hashes_exact = all_parent_feature_hashes,
      .offline_bytes_exact = all_offline_bytes_exact,
      .device_contracts_exact =
          all_device_contracts && all_repeated_device_exact &&
          all_device_capture_exact,
      .continuous_shuffle_pass = continuous_shuffle_by_arm,
      .order_shuffle_pass = order_shuffle_by_arm};
  const auto gate = srr_gate::evaluate(gate_input);
  std::cout << "srr.summary.preaudit.numeric_inputs="
            << gate.numeric_inputs_valid << '\n';
  std::cout << "srr.summary.preaudit.manifest_exact=false\n";
  std::cout << "srr.summary.preaudit.parent_evidence=unchecked\n";
  std::cout << "srr.summary.preaudit.all_reference_keys_exact=unchecked\n";
  std::cout << "srr.summary.preaudit.mechanics=" << gate.mechanics_valid
            << '\n';
  std::cout << "srr.summary.validity.continuous_shuffle_pass="
            << continuous_shuffle_pass << '\n';
  std::cout << "srr.summary.validity.order_shuffle_pass="
            << order_shuffle_pass << '\n';
  std::cout << "srr.summary.gate.material_gain_over_channel="
            << shadow_material_gain_over_channel << '\n';
  std::cout << "srr.summary.gate.noninferior_to_encoder="
            << shadow_noninferior_to_encoder << '\n';
  std::cout << "srr.summary.gate.order_decodable="
            << shadow_order_decodable << '\n';
  std::cout << "srr.summary.gate.conditional_quality_candidate="
            << shadow_quality_candidate << '\n';
  std::cout << "srr.summary.gate.preaudit_classification="
            << srr_gate::terminal_classification_name(gate.classification)
            << '\n';
  std::cout << "srr.summary.gate.preaudit_failure_reason="
            << srr_gate::failure_reason_name(gate.failure_reason) << '\n';
  std::cout << "srr.summary.gate.final_classification_requires_postrun_audit=true\n";
  std::cout << "training_authorized=false\n";
  std::cout << "augmentation_change_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_production_repair_authorized=false\n";
  std::cout << "execution_status=srr_measurements_complete\n";
  return 0;
}

int run_srr(const Options &options) {
  bool attempt_consumed = false;
  try {
    return run_srr_impl(options, attempt_consumed);
  } catch (...) {
    if (!attempt_consumed) {
      std::cout << std::boolalpha;
      std::cout << "srr.attempt.consumed=false\n";
      std::cout << "training_authorized=false\n";
      std::cout << "augmentation_change_authorized=false\n";
      std::cout << "long_run_authorized=false\n";
      std::cout << "production_or_end_to_end_authorized=false\n";
      std::cout << "follow_on_production_repair_authorized=false\n";
      std::cout << "execution_status=srr_prefit_exception\n";
    }
    throw;
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "structured-readout-repair-preflight") {
      return run_srr_preflight(options);
    }
    if (options.experiment == "structured-readout-repair") {
      return run_srr(options);
    }
    throw std::runtime_error(
        "--experiment must be structured-readout-repair-preflight or "
        "structured-readout-repair");
  } catch (const c10::Error &error) {
    std::cerr << "structured_readout_repair_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "structured_readout_repair_error=" << error.what() << '\n';
  }
  return 2;
}
