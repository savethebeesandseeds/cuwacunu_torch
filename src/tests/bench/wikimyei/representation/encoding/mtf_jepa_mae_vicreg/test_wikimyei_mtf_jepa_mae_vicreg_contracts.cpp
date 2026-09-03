#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr std::array<std::string_view, 5> kOnlinePrefixes{
    "tokenizer.", "encoder.", "predictor.", "mae_decoder.",
    "vicreg_stability_head."};

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool starts_with(const std::string &text, std::string_view prefix) {
  return text.rfind(prefix, 0) == 0;
}

int online_group(const std::string &name) {
  for (std::size_t i = 0; i < kOnlinePrefixes.size(); ++i) {
    if (starts_with(name, kOnlinePrefixes[i])) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool is_target_parameter(const std::string &name) {
  return starts_with(name, "target_tokenizer.") ||
         starts_with(name, "target_encoder.");
}

std::string online_name_for_target(const std::string &name) {
  if (starts_with(name, "target_tokenizer.")) {
    return "tokenizer." + name.substr(std::string("target_tokenizer.").size());
  }
  if (starts_with(name, "target_encoder.")) {
    return "encoder." + name.substr(std::string("target_encoder.").size());
  }
  throw std::runtime_error("not a target parameter: " + name);
}

bool finite(const torch::Tensor &tensor) {
  return tensor.defined() &&
         torch::isfinite(tensor).all().template item<bool>();
}

void check_close(const torch::Tensor &actual, const torch::Tensor &expected,
                 const std::string &message, double rtol = 1e-5,
                 double atol = 1e-6) {
  check(actual.defined() && expected.defined(), message + ": undefined tensor");
  check(actual.sizes() == expected.sizes(), message + ": shape mismatch");
  check(torch::allclose(actual, expected, rtol, atol), message);
}

void check_metadata_equal(const mtf::mtf_token_metadata_t &actual,
                          const mtf::mtf_token_metadata_t &expected,
                          const std::string &message) {
  check(torch::equal(actual.start_index, expected.start_index) &&
            torch::equal(actual.width, expected.width) &&
            torch::equal(actual.scale_id, expected.scale_id) &&
            torch::equal(actual.channel_id, expected.channel_id) &&
            torch::equal(actual.domain_id, expected.domain_id),
        message);
}

void check_token_batches_equal(const mtf::mtf_token_batch_t &actual,
                               const mtf::mtf_token_batch_t &expected,
                               const std::string &message) {
  check_close(actual.tokens, expected.tokens, message + ": tokens");
  check_close(actual.reconstruction_targets, expected.reconstruction_targets,
              message + ": projected targets");
  check_close(actual.time_reconstruction_targets,
              expected.time_reconstruction_targets, message + ": time targets");
  check_close(actual.frequency_reconstruction_targets,
              expected.frequency_reconstruction_targets,
              message + ": frequency targets");
  check(torch::equal(actual.time_reconstruction_mask,
                     expected.time_reconstruction_mask) &&
            torch::equal(actual.frequency_reconstruction_mask,
                         expected.frequency_reconstruction_mask) &&
            torch::equal(actual.token_mask, expected.token_mask),
        message + ": masks");
  check_metadata_equal(actual.metadata, expected.metadata,
                       message + ": metadata");
}

void check_encode_equal(
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &actual,
    const mtf::mtf_jepa_mae_vicreg_encode_output_t &expected,
    const std::string &message) {
  check_close(actual.embeddings, expected.embeddings, message + ": embeddings",
              1e-6, 1e-7);
  check_close(actual.pooled_embedding, expected.pooled_embedding,
              message + ": pooled", 1e-6, 1e-7);
  check_close(actual.pooled_by_channel, expected.pooled_by_channel,
              message + ": channel pool", 1e-6, 1e-7);
  check_close(actual.pooled_time, expected.pooled_time, message + ": time pool",
              1e-6, 1e-7);
  check_close(actual.pooled_frequency, expected.pooled_frequency,
              message + ": frequency pool", 1e-6, 1e-7);
  check(
      torch::equal(actual.token_mask, expected.token_mask) &&
          torch::equal(actual.sample_valid_mask, expected.sample_valid_mask) &&
          torch::equal(actual.channel_valid_mask, expected.channel_valid_mask),
      message + ": validity masks");
  check_metadata_equal(actual.metadata, expected.metadata,
                       message + ": metadata");
}

void check_forward_equal(const mtf::mtf_jepa_mae_vicreg_output_t &actual,
                         const mtf::mtf_jepa_mae_vicreg_output_t &expected,
                         const std::string &message) {
  check_close(actual.embeddings, expected.embeddings, message + ": embeddings");
  check_close(actual.pooled_embedding, expected.pooled_embedding,
              message + ": pooled");
  check_close(actual.pooled_by_channel, expected.pooled_by_channel,
              message + ": channel pool");
  check_close(actual.pooled_time, expected.pooled_time,
              message + ": time pool");
  check_close(actual.pooled_frequency, expected.pooled_frequency,
              message + ": frequency pool");
  const std::array<std::pair<const torch::Tensor *, const torch::Tensor *>, 9>
      losses{{{&actual.loss, &expected.loss},
              {&actual.loss_jepa, &expected.loss_jepa},
              {&actual.loss_mae, &expected.loss_mae},
              {&actual.loss_mae_time, &expected.loss_mae_time},
              {&actual.loss_mae_frequency, &expected.loss_mae_frequency},
              {&actual.loss_tf_align, &expected.loss_tf_align},
              {&actual.loss_vicreg, &expected.loss_vicreg},
              {&actual.loss_vicreg_global, &expected.loss_vicreg_global},
              {&actual.loss_vicreg_channel, &expected.loss_vicreg_channel}}};
  for (const auto &[lhs, rhs] : losses) {
    check_close(*lhs, *rhs, message + ": loss replay");
  }
  check(
      torch::equal(actual.sample_valid_mask, expected.sample_valid_mask) &&
          torch::equal(actual.channel_valid_mask, expected.channel_valid_mask),
      message + ": validity masks");
  check(actual.diagnostics.size() == expected.diagnostics.size(),
        message + ": diagnostic count");
  for (const auto &[key, value] : actual.diagnostics) {
    const auto it = expected.diagnostics.find(key);
    check(it != expected.diagnostics.end(),
          message + ": missing diagnostic " + key);
    check_close(value, it->second, message + ": diagnostic " + key);
  }
}

mtf::mtf_jepa_mae_vicreg_config_t small_config() {
  mtf::mtf_jepa_mae_vicreg_config_t cfg{};
  cfg.channel_count = 2;
  cfg.history_length = 8;
  cfg.input_width = 3;
  cfg.d_model = 8;
  cfg.latent_dim = 8;
  cfg.projector_dim = 8;
  cfg.predictor_hidden_dim = 12;
  cfg.num_encoder_layers = 1;
  cfg.num_predictor_layers = 1;
  cfg.num_decoder_layers = 1;
  cfg.num_heads = 2;
  cfg.dropout = 0.0;
  cfg.time_scales = {4, 8};
  cfg.scale_strides = {2, 4};
  cfg.frequency_num_bins = 4;
  cfg.mask_ratio_time = 0.35;
  cfg.mask_ratio_frequency = 0.35;
  cfg.mask_ratio_channel = 0.0;
  cfg.min_context_ratio = 0.30;
  cfg.use_channel_vicreg = true;
  cfg.lambda_global_vicreg = 0.5;
  cfg.lambda_channel_vicreg = 0.5;
  cfg.serving_pool_policy = mtf::mtf_serving_pool_policy_t::domain_balanced;
  cfg.dtype = torch::kFloat32;
  cfg.device = torch::Device(torch::kCPU);
  return cfg;
}

torch::Tensor deterministic_input(const mtf::mtf_jepa_mae_vicreg_config_t &cfg,
                                  int64_t batch_size) {
  const auto options =
      torch::TensorOptions().dtype(cfg.dtype).device(torch::kCPU);
  auto t = torch::arange(cfg.history_length, options).view({1, 1, -1, 1});
  auto b = torch::arange(batch_size, options).view({-1, 1, 1, 1});
  auto c = torch::arange(cfg.channel_count, options).view({1, -1, 1, 1});
  auto f = torch::arange(cfg.input_width, options).view({1, 1, 1, -1});
  return torch::sin((t + 1.0) * (f + 1.0) * 0.19 + b * 0.23 + c * 0.31) +
         0.03 * t + 0.07 * b - 0.05 * c + 0.02 * f;
}

std::map<std::string, torch::Tensor>
snapshot_parameters(const mtf::MtfJepaMaeVicreg &model) {
  std::map<std::string, torch::Tensor> out;
  for (const auto &parameter : model->named_parameters(/*recurse=*/true)) {
    out.emplace(parameter.key(), parameter.value().detach().clone());
  }
  return out;
}

std::map<std::string, torch::Tensor>
snapshot_trainable_gradients(const mtf::MtfJepaMaeVicreg &model) {
  std::map<std::string, torch::Tensor> out;
  for (const auto &parameter : model->named_parameters(/*recurse=*/true)) {
    if (!parameter.value().requires_grad()) {
      continue;
    }
    const auto gradient = parameter.value().grad();
    out.emplace(parameter.key(), gradient.defined()
                                     ? gradient.detach().clone()
                                     : torch::zeros_like(parameter.value()));
  }
  return out;
}

void test_channel_stratified_vicreg_contracts() {
  const auto tensor_options = torch::TensorOptions().dtype(torch::kFloat32);
  mtf::vicreg_stability_loss_options_t loss_options{};
  loss_options.invariance_weight = 2.0;
  loss_options.variance_weight = 3.0;
  loss_options.covariance_weight = 0.5;
  loss_options.variance_floor = 0.75;
  loss_options.eps = 1e-4;

  const auto coordinates =
      torch::arange(5 * 3 * 4, tensor_options).view({5, 3, 4});
  const auto z1 = torch::sin(coordinates * 0.17) + coordinates * 0.01;
  const auto z2 = torch::cos(coordinates * 0.11) - coordinates * 0.005;
  auto mask = torch::ones({5, 3}, torch::kBool);
  mask.index_put_({3, 1}, false);
  mask.index_put_({4, 1}, false);
  mask.index_put_({2, 2}, false);
  mask.index_put_({3, 2}, false);
  mask.index_put_({4, 2}, false);

  const auto stratified = mtf::compute_channel_stratified_vicreg_stability_loss(
      z1, mask, z2, mask, loss_options);
  auto manual_loss = torch::zeros({}, tensor_options);
  auto manual_invariance = torch::zeros({}, tensor_options);
  auto manual_variance = torch::zeros({}, tensor_options);
  auto manual_covariance = torch::zeros({}, tensor_options);
  int64_t manual_rows = 0;
  int64_t manual_groups = 0;
  for (int64_t channel = 0; channel < z1.size(1); ++channel) {
    const auto channel_result = mtf::compute_vicreg_stability_loss(
        z1.narrow(/*dim=*/1, channel, /*length=*/1),
        mask.narrow(/*dim=*/1, channel, /*length=*/1),
        z2.narrow(/*dim=*/1, channel, /*length=*/1),
        mask.narrow(/*dim=*/1, channel, /*length=*/1), loss_options);
    manual_rows += channel_result.valid_rows;
    check(channel_result.valid_rows >= 2,
          "manual stratified fixture has fewer than two rows");
    manual_loss = manual_loss + channel_result.loss;
    manual_invariance = manual_invariance + channel_result.invariance_loss;
    manual_variance = manual_variance + channel_result.variance_loss;
    manual_covariance = manual_covariance + channel_result.covariance_loss;
    ++manual_groups;
  }
  const auto channel_count = static_cast<double>(z1.size(1));
  manual_loss = manual_loss / channel_count;
  manual_invariance = manual_invariance / channel_count;
  manual_variance = manual_variance / channel_count;
  manual_covariance = manual_covariance / channel_count;
  check_close(stratified.loss, manual_loss,
              "stratified VICReg loss differs from manual channel mean");
  check_close(stratified.invariance_loss, manual_invariance,
              "stratified VICReg invariance differs from manual channel mean");
  check_close(stratified.variance_loss, manual_variance,
              "stratified VICReg variance differs from manual channel mean");
  check_close(stratified.covariance_loss, manual_covariance,
              "stratified VICReg covariance differs from manual channel mean");
  check(stratified.valid_rows == manual_rows &&
            stratified.active_groups == manual_groups && manual_rows == 10 &&
            manual_groups == 3,
        "stratified VICReg row/group accounting is incorrect");

  const auto expect_insufficient_rows_rejected =
      [&](int64_t retained_rows, const std::string &fixture_name) {
        auto invalid_mask = torch::ones({5, 3}, torch::kBool);
        invalid_mask.select(/*dim=*/1, /*index=*/2).fill_(false);
        for (int64_t row = 0; row < retained_rows; ++row) {
          invalid_mask.index_put_({row, 2}, true);
        }
        bool rejected = false;
        try {
          (void)mtf::compute_channel_stratified_vicreg_stability_loss(
              z1, invalid_mask, z2, invalid_mask, loss_options);
        } catch (const c10::Error &) {
          rejected = true;
        }
        check(rejected, fixture_name +
                            " did not reject a channel with fewer than two "
                            "jointly valid rows");
      };
  expect_insufficient_rows_rejected(/*retained_rows=*/0, "zero-row fixture");
  expect_insufficient_rows_rejected(/*retained_rows=*/1, "one-row fixture");

  auto channel_offsets = torch::zeros({4, 2, 3}, tensor_options);
  channel_offsets.select(/*dim=*/1, /*index=*/0).fill_(-4.0);
  channel_offsets.select(/*dim=*/1, /*index=*/1).fill_(4.0);
  const auto offset_mask = torch::ones({4, 2}, torch::kBool);
  mtf::vicreg_stability_loss_options_t variance_only{};
  variance_only.invariance_weight = 0.0;
  variance_only.variance_weight = 1.0;
  variance_only.covariance_weight = 0.0;
  variance_only.variance_floor = 1.0;
  variance_only.eps = 1e-4;
  const auto flattened_offsets = mtf::compute_vicreg_stability_loss(
      channel_offsets, offset_mask, channel_offsets, offset_mask,
      variance_only);
  const auto stratified_offsets =
      mtf::compute_channel_stratified_vicreg_stability_loss(
          channel_offsets, offset_mask, channel_offsets, offset_mask,
          variance_only);
  check(flattened_offsets.variance_loss.item<double>() <= 1e-7,
        "between-channel identity offsets did not satisfy flattened variance");
  check(stratified_offsets.variance_loss.item<double>() > 0.98,
        "channel identity offsets incorrectly satisfied within-channel "
        "variance");
  check(stratified_offsets.active_groups == 2,
        "channel identity fixture did not retain both channel groups");

  check(!mtf::mtf_jepa_mae_vicreg_config_t{}.stratify_channel_vicreg_by_channel,
        "channel-stratified VICReg must remain default-off");
  auto global_config = small_config();
  global_config.use_global_vicreg = true;
  global_config.use_channel_vicreg = false;
  global_config.stratify_channel_vicreg_by_channel = false;
  torch::manual_seed(503);
  auto global_default = mtf::MtfJepaMaeVicreg(global_config);
  auto global_config_with_switch = global_config;
  global_config_with_switch.stratify_channel_vicreg_by_channel = true;
  torch::manual_seed(503);
  auto global_switched = mtf::MtfJepaMaeVicreg(global_config_with_switch);
  const auto default_parameters = snapshot_parameters(global_default);
  const auto switched_parameters = snapshot_parameters(global_switched);
  check(default_parameters.size() == switched_parameters.size(),
        "channel statistics switch changed model topology");
  for (const auto &[name, value] : default_parameters) {
    check(torch::equal(value, switched_parameters.at(name)),
          "channel statistics switch changed initialization: " + name);
  }
  const auto global_input =
      deterministic_input(global_config, /*batch_size=*/4);
  const auto global_mask = torch::ones_like(global_input).to(torch::kBool);
  torch::manual_seed(509);
  const auto global_default_output =
      global_default->forward(global_input, global_mask);
  torch::manual_seed(509);
  const auto global_switched_output =
      global_switched->forward(global_input, global_mask);
  check_close(global_default_output.loss_vicreg_global,
              global_switched_output.loss_vicreg_global,
              "channel statistics switch changed global VICReg");
  check_close(global_default_output.loss, global_switched_output.loss,
              "channel statistics switch changed global-only total loss");
  check(finite(global_default_output.loss) &&
            finite(global_switched_output.loss),
        "global-only default regression produced a non-finite loss");

  auto flat_config = small_config();
  flat_config.use_global_vicreg = false;
  flat_config.use_channel_vicreg = true;
  flat_config.stratify_channel_vicreg_by_channel = false;
  torch::manual_seed(521);
  auto flat_model = mtf::MtfJepaMaeVicreg(flat_config);
  auto stratified_config = flat_config;
  stratified_config.stratify_channel_vicreg_by_channel = true;
  torch::manual_seed(521);
  auto stratified_model = mtf::MtfJepaMaeVicreg(stratified_config);
  const auto channel_input = deterministic_input(flat_config, /*batch_size=*/4);
  const auto channel_mask = torch::ones_like(channel_input).to(torch::kBool);
  torch::manual_seed(523);
  const auto flat_output = flat_model->forward(channel_input, channel_mask);
  torch::manual_seed(523);
  const auto stratified_output =
      stratified_model->forward(channel_input, channel_mask);
  check(finite(flat_output.loss_vicreg_channel) &&
            finite(stratified_output.loss_vicreg_channel),
        "channel VICReg statistics modes produced a non-finite loss");
  check(
      flat_output.diagnostics.at("vicreg_channel_stratified").item<double>() ==
              0.0 &&
          stratified_output.diagnostics.at("vicreg_channel_stratified")
                  .item<double>() == 1.0,
      "channel VICReg statistics mode diagnostic is incorrect");
  check(flat_output.diagnostics.at("vicreg_channel_active_groups")
                    .item<double>() == 1.0 &&
            stratified_output.diagnostics.at("vicreg_channel_active_groups")
                    .item<double>() ==
                static_cast<double>(stratified_config.channel_count),
        "channel VICReg active-group diagnostic is incorrect");
  const auto reconstructed_channel_loss =
      stratified_config.vicreg_sim_weight *
          stratified_output.diagnostics.at("vicreg_channel_sim_term") +
      stratified_config.vicreg_var_weight *
          stratified_output.diagnostics.at("vicreg_channel_var_term") +
      stratified_config.vicreg_cov_weight *
          stratified_output.diagnostics.at("vicreg_channel_cov_term");
  check_close(stratified_output.loss_vicreg_channel, reconstructed_channel_loss,
              "stratified channel component diagnostics do not reconstruct "
              "the channel loss");
}

void test_vicreg_debug_projection_and_weighting_contracts() {
  auto default_config = small_config();
  default_config.projector_dim = 64;
  check(!default_config.return_vicreg_debug_tensors,
        "VICReg debug tensors must remain default-off");
  torch::manual_seed(601);
  auto default_model = mtf::MtfJepaMaeVicreg(default_config);
  auto debug_config = default_config;
  debug_config.return_vicreg_debug_tensors = true;
  torch::manual_seed(601);
  auto debug_model = mtf::MtfJepaMaeVicreg(debug_config);
  const auto default_parameters = snapshot_parameters(default_model);
  const auto debug_parameters = snapshot_parameters(debug_model);
  check(default_parameters.size() == debug_parameters.size(),
        "VICReg debug switch changed model topology");
  for (const auto &[name, value] : default_parameters) {
    check(torch::equal(value, debug_parameters.at(name)),
          "VICReg debug switch changed initialization: " + name);
  }

  const auto input = deterministic_input(default_config, /*batch_size=*/4);
  const auto mask = torch::ones_like(input).to(torch::kBool);
  torch::manual_seed(607);
  const auto default_output = default_model->forward(input, mask);
  const std::array<const torch::Tensor *, 22> default_debug_tensors{
      &default_output.jepa_target_mask,
      &default_output.jepa_context_mask,
      &default_output.vicreg_drawn_a_data,
      &default_output.vicreg_drawn_a_feature_mask,
      &default_output.vicreg_drawn_b_data,
      &default_output.vicreg_drawn_b_feature_mask,
      &default_output.vicreg_view_a_data,
      &default_output.vicreg_view_a_feature_mask,
      &default_output.vicreg_view_b_data,
      &default_output.vicreg_view_b_feature_mask,
      &default_output.vicreg_view_a_token_mask,
      &default_output.vicreg_view_b_token_mask,
      &default_output.vicreg_view_a_sample_valid_mask,
      &default_output.vicreg_view_b_sample_valid_mask,
      &default_output.vicreg_view_a_pooled_by_channel,
      &default_output.vicreg_view_b_pooled_by_channel,
      &default_output.vicreg_view_a_pooled_global,
      &default_output.vicreg_view_b_pooled_global,
      &default_output.vicreg_view_a_projected_global,
      &default_output.vicreg_view_b_projected_global,
      &default_output.vicreg_global_joint_mask,
      &default_output.vicreg_channel_joint_mask};
  for (const auto *tensor : default_debug_tensors) {
    check(!tensor->defined(),
          "default-off VICReg debug output unexpectedly defined a tensor");
  }
  check(default_output.vicreg_encoder_call_count == 0 &&
            default_output.vicreg_projector_call_count == 0,
        "default-off VICReg debug output exposed call counters");

  torch::manual_seed(607);
  const auto debug_output = debug_model->forward(input, mask);
  const auto independent_post_rng =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  torch::manual_seed(607);
  const auto debug_replay = debug_model->forward(input, mask);
  check_forward_equal(default_output, debug_output,
                      "VICReg debug switch changed the default independent "
                      "forward");
  check(debug_output.vicreg_view_a_data.sizes() == input.sizes() &&
            debug_output.vicreg_view_b_data.sizes() == input.sizes() &&
            debug_output.vicreg_drawn_a_data.sizes() == input.sizes() &&
            debug_output.vicreg_drawn_b_data.sizes() == input.sizes() &&
            debug_output.vicreg_drawn_a_feature_mask.sizes() == input.sizes() &&
            debug_output.vicreg_drawn_b_feature_mask.sizes() == input.sizes() &&
            debug_output.vicreg_view_a_feature_mask.sizes() == input.sizes() &&
            debug_output.vicreg_view_b_feature_mask.sizes() == input.sizes(),
        "VICReg debug weak-view shape mismatch");
  check(debug_output.jepa_target_mask.sizes() ==
                torch::IntArrayRef(
                    {input.size(0), debug_output.embeddings.size(1)}) &&
            debug_output.jepa_context_mask.sizes() ==
                debug_output.jepa_target_mask.sizes() &&
            debug_output.jepa_target_mask.scalar_type() == torch::kBool &&
            debug_output.jepa_context_mask.scalar_type() == torch::kBool,
        "JEPA debug mask shape/dtype mismatch");
  check(debug_output.vicreg_view_a_pooled_by_channel.sizes() ==
                torch::IntArrayRef({input.size(0), debug_config.channel_count,
                                    debug_config.latent_dim}) &&
            debug_output.vicreg_view_b_pooled_by_channel.sizes() ==
                torch::IntArrayRef({input.size(0), debug_config.channel_count,
                                    debug_config.latent_dim}) &&
            debug_output.vicreg_channel_joint_mask.sizes() ==
                torch::IntArrayRef({input.size(0), debug_config.channel_count}),
        "VICReg debug channel shape mismatch");
  check(debug_output.vicreg_view_a_token_mask.sizes() ==
                debug_output.jepa_target_mask.sizes() &&
            debug_output.vicreg_view_b_token_mask.sizes() ==
                debug_output.vicreg_view_a_token_mask.sizes() &&
            debug_output.vicreg_view_a_sample_valid_mask.sizes() ==
                torch::IntArrayRef({input.size(0)}) &&
            debug_output.vicreg_view_b_sample_valid_mask.sizes() ==
                debug_output.vicreg_view_a_sample_valid_mask.sizes() &&
            debug_output.vicreg_encoder_call_count == 2 &&
            debug_output.vicreg_projector_call_count == 4,
        "VICReg debug branch-mask shape or call-count mismatch");
  check(debug_output.vicreg_view_a_pooled_global.sizes() ==
                torch::IntArrayRef(
                    {input.size(0), debug_config.latent_dim}) &&
            debug_output.vicreg_view_b_pooled_global.sizes() ==
                debug_output.vicreg_view_a_pooled_global.sizes() &&
            debug_output.vicreg_view_a_projected_global.sizes() ==
                torch::IntArrayRef(
                    {input.size(0), 1, debug_config.projector_dim}) &&
            debug_output.vicreg_view_b_projected_global.sizes() ==
                debug_output.vicreg_view_a_projected_global.sizes() &&
            debug_output.vicreg_global_joint_mask.sizes() ==
                torch::IntArrayRef({input.size(0), 1}),
        "VICReg debug global shape mismatch");
  const std::array<const torch::Tensor *, 16> detached_debug_tensors{
      &debug_output.jepa_target_mask,
      &debug_output.jepa_context_mask,
      &debug_output.vicreg_drawn_a_data,
      &debug_output.vicreg_drawn_a_feature_mask,
      &debug_output.vicreg_drawn_b_data,
      &debug_output.vicreg_drawn_b_feature_mask,
      &debug_output.vicreg_view_a_data,
      &debug_output.vicreg_view_a_feature_mask,
      &debug_output.vicreg_view_b_data,
      &debug_output.vicreg_view_b_feature_mask,
      &debug_output.vicreg_view_a_token_mask,
      &debug_output.vicreg_view_b_token_mask,
      &debug_output.vicreg_view_a_sample_valid_mask,
      &debug_output.vicreg_view_b_sample_valid_mask,
      &debug_output.vicreg_global_joint_mask,
      &debug_output.vicreg_channel_joint_mask};
  for (const auto *tensor : detached_debug_tensors) {
    check(tensor->defined() && !tensor->requires_grad(),
          "VICReg value-only debug tensor is undefined or attached to "
          "autograd");
  }
  check(debug_output.vicreg_view_a_pooled_by_channel.defined() &&
            debug_output.vicreg_view_b_pooled_by_channel.defined() &&
            debug_output.vicreg_view_a_pooled_by_channel.requires_grad() &&
            debug_output.vicreg_view_b_pooled_by_channel.requires_grad(),
        "VICReg debug channel pools are undefined or detached from autograd");
  check(debug_output.vicreg_view_a_pooled_global.defined() &&
            debug_output.vicreg_view_b_pooled_global.defined() &&
            debug_output.vicreg_view_a_projected_global.defined() &&
            debug_output.vicreg_view_b_projected_global.defined() &&
            debug_output.vicreg_view_a_pooled_global.requires_grad() &&
            debug_output.vicreg_view_b_pooled_global.requires_grad() &&
            debug_output.vicreg_view_a_projected_global.requires_grad() &&
            debug_output.vicreg_view_b_projected_global.requires_grad(),
        "VICReg debug global pools/projections are undefined or detached from "
        "autograd");
  check(torch::equal(debug_output.vicreg_view_a_data,
                     debug_replay.vicreg_view_a_data) &&
            torch::equal(debug_output.vicreg_drawn_a_data,
                         debug_replay.vicreg_drawn_a_data) &&
            torch::equal(debug_output.vicreg_drawn_a_feature_mask,
                         debug_replay.vicreg_drawn_a_feature_mask) &&
            torch::equal(debug_output.vicreg_drawn_b_data,
                         debug_replay.vicreg_drawn_b_data) &&
            torch::equal(debug_output.vicreg_drawn_b_feature_mask,
                         debug_replay.vicreg_drawn_b_feature_mask) &&
            torch::equal(debug_output.jepa_target_mask,
                         debug_replay.jepa_target_mask) &&
            torch::equal(debug_output.jepa_context_mask,
                         debug_replay.jepa_context_mask) &&
            torch::equal(debug_output.vicreg_view_a_feature_mask,
                         debug_replay.vicreg_view_a_feature_mask) &&
            torch::equal(debug_output.vicreg_view_b_data,
                         debug_replay.vicreg_view_b_data) &&
            torch::equal(debug_output.vicreg_view_b_feature_mask,
                         debug_replay.vicreg_view_b_feature_mask) &&
            torch::equal(debug_output.vicreg_view_a_token_mask,
                         debug_replay.vicreg_view_a_token_mask) &&
            torch::equal(debug_output.vicreg_view_b_token_mask,
                         debug_replay.vicreg_view_b_token_mask) &&
            torch::equal(debug_output.vicreg_view_a_sample_valid_mask,
                         debug_replay.vicreg_view_a_sample_valid_mask) &&
            torch::equal(debug_output.vicreg_view_b_sample_valid_mask,
                         debug_replay.vicreg_view_b_sample_valid_mask) &&
            torch::equal(debug_output.vicreg_view_a_pooled_by_channel,
                         debug_replay.vicreg_view_a_pooled_by_channel) &&
            torch::equal(debug_output.vicreg_view_b_pooled_by_channel,
                         debug_replay.vicreg_view_b_pooled_by_channel) &&
            torch::equal(debug_output.vicreg_view_a_pooled_global,
                         debug_replay.vicreg_view_a_pooled_global) &&
            torch::equal(debug_output.vicreg_view_b_pooled_global,
                         debug_replay.vicreg_view_b_pooled_global) &&
            torch::equal(debug_output.vicreg_view_a_projected_global,
                         debug_replay.vicreg_view_a_projected_global) &&
            torch::equal(debug_output.vicreg_view_b_projected_global,
                         debug_replay.vicreg_view_b_projected_global) &&
            torch::equal(debug_output.vicreg_global_joint_mask,
                         debug_replay.vicreg_global_joint_mask) &&
            torch::equal(debug_output.vicreg_channel_joint_mask,
                         debug_replay.vicreg_channel_joint_mask),
        "paired seed did not replay exact VICReg debug tensors");

  torch::manual_seed(607);
  const auto preview_tokens = debug_model->tokenize(input, mask);
  const auto preview_masks = debug_model->create_masks(preview_tokens);
  check(
      torch::equal(debug_output.jepa_target_mask, preview_masks.target_mask) &&
          torch::equal(debug_output.jepa_context_mask,
                       preview_masks.context_mask),
      "JEPA debug masks are not the exact masks used by the seeded forward");

  const auto encoded_view_a = debug_model->encode(
      debug_output.vicreg_view_a_data, debug_output.vicreg_view_a_feature_mask);
  const auto encoded_view_b = debug_model->encode(
      debug_output.vicreg_view_b_data, debug_output.vicreg_view_b_feature_mask);
  check(encoded_view_a.pooled_by_channel.requires_grad() &&
            encoded_view_b.pooled_by_channel.requires_grad(),
        "re-encoded weak-view channel pools are detached from autograd");
  check(torch::equal(debug_output.vicreg_view_a_pooled_by_channel,
                     encoded_view_a.pooled_by_channel),
        "debug view A pool is not the exact re-encoded weak view");
  check(torch::equal(debug_output.vicreg_view_b_pooled_by_channel,
                     encoded_view_b.pooled_by_channel),
        "debug view B pool is not the exact re-encoded weak view");
  check(torch::equal(debug_output.vicreg_view_a_pooled_global,
                     encoded_view_a.pooled_embedding) &&
            torch::equal(debug_output.vicreg_view_b_pooled_global,
                         encoded_view_b.pooled_embedding),
        "debug global pools are not the exact re-encoded weak views");
  check(torch::equal(debug_output.vicreg_view_a_projected_global,
                     debug_model
                         ->project_vicreg(encoded_view_a.pooled_embedding)
                         .unsqueeze(1)) &&
            torch::equal(debug_output.vicreg_view_b_projected_global,
                         debug_model
                         ->project_vicreg(encoded_view_b.pooled_embedding)
                         .unsqueeze(1)),
        "debug global projections are not the exact projected weak views");
  check(torch::equal(
            debug_output.vicreg_global_joint_mask,
            encoded_view_a.token_mask.any(/*dim=*/1)
                .logical_and(encoded_view_b.token_mask.any(/*dim=*/1))
                .unsqueeze(1)),
        "debug global mask is not the weak-view joint mask");
  check(torch::equal(debug_output.vicreg_view_a_token_mask,
                     encoded_view_a.token_mask) &&
            torch::equal(debug_output.vicreg_view_b_token_mask,
                         encoded_view_b.token_mask) &&
            torch::equal(debug_output.vicreg_view_a_sample_valid_mask,
                         encoded_view_a.sample_valid_mask) &&
            torch::equal(debug_output.vicreg_view_b_sample_valid_mask,
                         encoded_view_b.sample_valid_mask),
        "debug branch masks are not the exact encoded weak-view masks");

  auto tied_config = debug_config;
  tied_config.vicreg_view_pairing_policy =
      mtf::mtf_vicreg_view_pairing_policy_t::tied_weak;
  auto clean_config = debug_config;
  clean_config.vicreg_view_pairing_policy =
      mtf::mtf_vicreg_view_pairing_policy_t::clean_identical;
  torch::manual_seed(601);
  auto tied_model = mtf::MtfJepaMaeVicreg(tied_config);
  torch::manual_seed(601);
  auto clean_model = mtf::MtfJepaMaeVicreg(clean_config);
  torch::manual_seed(607);
  const auto tied_output = tied_model->forward(input, mask);
  const auto tied_post_rng =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  torch::manual_seed(607);
  const auto clean_output = clean_model->forward(input, mask);
  const auto clean_post_rng =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  const auto canonical = mtf::detail::canonicalize_input(input, mask,
                                                          clean_config);
  check(torch::equal(independent_post_rng, tied_post_rng) &&
            torch::equal(independent_post_rng, clean_post_rng),
        "VICReg view pairing policies did not consume the same RNG schedule");
  check(torch::equal(debug_output.vicreg_drawn_a_data,
                     tied_output.vicreg_drawn_a_data) &&
            torch::equal(debug_output.vicreg_drawn_a_data,
                         clean_output.vicreg_drawn_a_data) &&
            torch::equal(debug_output.vicreg_drawn_a_feature_mask,
                         tied_output.vicreg_drawn_a_feature_mask) &&
            torch::equal(debug_output.vicreg_drawn_a_feature_mask,
                         clean_output.vicreg_drawn_a_feature_mask) &&
            torch::equal(debug_output.vicreg_drawn_b_data,
                         tied_output.vicreg_drawn_b_data) &&
            torch::equal(debug_output.vicreg_drawn_b_data,
                         clean_output.vicreg_drawn_b_data) &&
            torch::equal(debug_output.vicreg_drawn_b_feature_mask,
                         tied_output.vicreg_drawn_b_feature_mask) &&
            torch::equal(debug_output.vicreg_drawn_b_feature_mask,
                         clean_output.vicreg_drawn_b_feature_mask),
        "post-draw substitution changed the ordinary weak draws");
  check(torch::equal(debug_output.vicreg_view_a_data,
                     debug_output.vicreg_drawn_a_data) &&
            torch::equal(debug_output.vicreg_view_b_data,
                         debug_output.vicreg_drawn_b_data) &&
            torch::equal(debug_output.vicreg_view_a_feature_mask,
                         debug_output.vicreg_drawn_a_feature_mask) &&
            torch::equal(debug_output.vicreg_view_b_feature_mask,
                         debug_output.vicreg_drawn_b_feature_mask),
        "independent policy did not use both ordinary weak draws");
  check(torch::equal(tied_output.vicreg_view_a_data,
                     debug_output.vicreg_view_a_data) &&
            torch::equal(tied_output.vicreg_view_a_feature_mask,
                         debug_output.vicreg_view_a_feature_mask),
        "tied policy did not retain ordinary weak draw A");
  check(torch::equal(tied_output.vicreg_view_a_data,
                     tied_output.vicreg_view_b_data) &&
            torch::equal(tied_output.vicreg_view_a_feature_mask,
                         tied_output.vicreg_view_b_feature_mask) &&
            torch::equal(tied_output.vicreg_view_a_pooled_global,
                         tied_output.vicreg_view_b_pooled_global) &&
            torch::equal(tied_output.vicreg_view_a_projected_global,
                         tied_output.vicreg_view_b_projected_global) &&
            torch::equal(tied_output.vicreg_view_a_token_mask,
                         tied_output.vicreg_view_b_token_mask) &&
            torch::equal(tied_output.vicreg_view_a_sample_valid_mask,
                         tied_output.vicreg_view_b_sample_valid_mask) &&
            tied_output.vicreg_encoder_call_count == 2 &&
            tied_output.vicreg_projector_call_count == 4,
        "tied policy did not present identical used views to the loss");
  check(torch::equal(clean_output.vicreg_view_a_data, canonical.data) &&
            torch::equal(clean_output.vicreg_view_b_data, canonical.data) &&
            torch::equal(clean_output.vicreg_view_a_feature_mask,
                         canonical.feature_mask) &&
            torch::equal(clean_output.vicreg_view_b_feature_mask,
                         canonical.feature_mask) &&
            torch::equal(clean_output.vicreg_view_a_pooled_global,
                         clean_output.vicreg_view_b_pooled_global) &&
            torch::equal(clean_output.vicreg_view_a_projected_global,
                         clean_output.vicreg_view_b_projected_global) &&
            torch::equal(clean_output.vicreg_view_a_token_mask,
                         clean_output.vicreg_view_b_token_mask) &&
            torch::equal(clean_output.vicreg_view_a_sample_valid_mask,
                         clean_output.vicreg_view_b_sample_valid_mask) &&
            clean_output.vicreg_encoder_call_count == 2 &&
            clean_output.vicreg_projector_call_count == 4,
        "clean-identical policy did not present canonical identical views");
  check(!torch::equal(debug_output.vicreg_view_a_data,
                      debug_output.vicreg_view_b_data),
        "independent weak-view fixture unexpectedly produced identical views");
  check(tied_output.diagnostics.at("vicreg_global_sim_term")
                    .item<double>() <= 1.0e-12 &&
            clean_output.diagnostics.at("vicreg_global_sim_term")
                    .item<double>() <= 1.0e-12,
        "identical VICReg views produced nonzero global invariance loss");
  check(torch::equal(debug_output.vicreg_channel_joint_mask,
                     encoded_view_a.channel_valid_mask.logical_and(
                         encoded_view_b.channel_valid_mask)),
        "debug channel mask is not the weak-view joint mask");

  debug_model->zero_grad();
  auto rank2 = torch::randn(
      {3, debug_config.latent_dim},
      torch::TensorOptions().dtype(debug_config.dtype).requires_grad(true));
  auto rank3 = torch::randn(
      {3, debug_config.channel_count, debug_config.latent_dim},
      torch::TensorOptions().dtype(debug_config.dtype).requires_grad(true));
  const auto projected_rank2 = debug_model->project_vicreg(rank2);
  const auto projected_rank3 = debug_model->project_vicreg(rank3);
  check(projected_rank2.sizes() == torch::IntArrayRef({3, 64}) &&
            projected_rank3.sizes() ==
                torch::IntArrayRef({3, debug_config.channel_count, 64}),
        "public VICReg projector wrapper shape mismatch");
  check(projected_rank2.requires_grad() && projected_rank3.requires_grad(),
        "public VICReg projector wrapper detached its result");
  (projected_rank2.pow(2).mean() + projected_rank3.pow(2).mean()).backward();
  check(rank2.grad().defined() && rank3.grad().defined() &&
            finite(rank2.grad()) && finite(rank3.grad()) &&
            rank2.grad().abs().sum().item<double>() > 0.0 &&
            rank3.grad().abs().sum().item<double>() > 0.0,
        "public VICReg projector wrapper did not retain input gradients");

  auto weighted_config = small_config();
  weighted_config.lambda_jepa = 0.0;
  weighted_config.lambda_mae = 0.0;
  weighted_config.lambda_tf_align = 0.0;
  weighted_config.lambda_vicreg = 0.05;
  weighted_config.use_global_vicreg = false;
  weighted_config.use_channel_vicreg = true;
  weighted_config.lambda_channel_vicreg = 0.25;
  weighted_config.stratify_channel_vicreg_by_channel = true;
  weighted_config.return_vicreg_debug_tensors = true;
  torch::manual_seed(613);
  auto weighted_model = mtf::MtfJepaMaeVicreg(weighted_config);
  const auto weighted_input =
      deterministic_input(weighted_config, /*batch_size=*/6);
  const auto weighted_mask = torch::ones_like(weighted_input).to(torch::kBool);
  torch::manual_seed(617);
  const auto weighted_output =
      weighted_model->forward(weighted_input, weighted_mask);
  const auto expected_internal = weighted_config.lambda_channel_vicreg *
                                 weighted_output.loss_vicreg_channel;
  const auto expected_total = weighted_config.lambda_vicreg * expected_internal;
  check(torch::equal(weighted_output.loss_vicreg, expected_internal),
        "internal channel VICReg weight was not applied exactly once");
  check(torch::equal(weighted_output.loss, expected_total),
        "outer VICReg weight was not applied exactly once");
  check_close(weighted_output.loss,
              (0.05 * 0.25) * weighted_output.loss_vicreg_channel,
              "nested VICReg 0.05*0.25 decomposition is incorrect", 1e-6, 1e-7);
  check(finite(weighted_output.loss) &&
            weighted_output.loss.item<double>() > 0.0,
        "weighted stratified VICReg loss is non-positive or non-finite");
  weighted_output.loss.backward();
  bool tokenizer_gradient = false;
  bool encoder_gradient = false;
  bool projector_gradient = false;
  for (const auto &parameter :
       weighted_model->named_parameters(/*recurse=*/true)) {
    const auto gradient = parameter.value().grad();
    if (!gradient.defined()) {
      continue;
    }
    check(finite(gradient),
          "weighted stratified VICReg produced a non-finite gradient: " +
              parameter.key());
    const bool nonzero = gradient.abs().sum().item<double>() > 0.0;
    tokenizer_gradient =
        tokenizer_gradient ||
        (starts_with(parameter.key(), "tokenizer.") && nonzero);
    encoder_gradient = encoder_gradient ||
                       (starts_with(parameter.key(), "encoder.") && nonzero);
    projector_gradient =
        projector_gradient ||
        (starts_with(parameter.key(), "vicreg_stability_head.") && nonzero);
  }
  check(tokenizer_gradient && encoder_gradient && projector_gradient,
        "weighted stratified VICReg did not reach tokenizer, encoder, and "
        "shared projector");

  auto no_variance_config = weighted_config;
  no_variance_config.vicreg_var_weight = 0.0;
  torch::manual_seed(619);
  auto full_variance_model = mtf::MtfJepaMaeVicreg(weighted_config);
  torch::manual_seed(619);
  auto no_variance_model = mtf::MtfJepaMaeVicreg(no_variance_config);
  const auto full_parameters = snapshot_parameters(full_variance_model);
  const auto no_variance_parameters = snapshot_parameters(no_variance_model);
  check(full_parameters.size() == no_variance_parameters.size(),
        "zero variance weight changed model topology");
  for (const auto &[name, value] : full_parameters) {
    check(torch::equal(value, no_variance_parameters.at(name)),
          "zero variance weight changed initialization: " + name);
  }

  torch::manual_seed(631);
  const auto full_variance_output =
      full_variance_model->forward(weighted_input, weighted_mask);
  torch::manual_seed(631);
  const auto no_variance_output =
      no_variance_model->forward(weighted_input, weighted_mask);
  for (const std::string component : {"sim", "var", "cov"}) {
    check(torch::equal(full_variance_output.diagnostics.at("vicreg_channel_" +
                                                           component + "_term"),
                       no_variance_output.diagnostics.at("vicreg_channel_" +
                                                         component + "_term")),
          "zero variance weight changed raw " + component + " term");
  }
  check(torch::equal(full_variance_output.vicreg_view_a_data,
                     no_variance_output.vicreg_view_a_data) &&
            torch::equal(full_variance_output.vicreg_view_b_data,
                         no_variance_output.vicreg_view_b_data) &&
            torch::equal(full_variance_output.vicreg_view_a_feature_mask,
                         no_variance_output.vicreg_view_a_feature_mask) &&
            torch::equal(full_variance_output.vicreg_view_b_feature_mask,
                         no_variance_output.vicreg_view_b_feature_mask),
        "zero variance weight changed paired weak views");
  const auto raw_sim =
      no_variance_output.diagnostics.at("vicreg_channel_sim_term");
  const auto raw_var =
      no_variance_output.diagnostics.at("vicreg_channel_var_term");
  const auto raw_cov =
      no_variance_output.diagnostics.at("vicreg_channel_cov_term");
  check(finite(raw_var) && raw_var.item<double>() > 0.0,
        "zero variance weight suppressed the raw variance diagnostic");
  check_close(no_variance_output.loss_vicreg_channel, 25.0 * raw_sim + raw_cov,
              "zero variance weight did not remove exactly the weighted "
              "variance component");
  check_close(full_variance_output.loss_vicreg_channel -
                  no_variance_output.loss_vicreg_channel,
              25.0 * raw_var,
              "full/no-variance channel-loss difference is not 25*variance");
  check_close(full_variance_output.loss - no_variance_output.loss,
              (0.05 * 0.25 * 25.0) * raw_var,
              "full/no-variance total-loss difference has incorrect nested "
              "weighting");

  full_variance_model->zero_grad();
  no_variance_model->zero_grad();
  full_variance_output.loss.backward();
  no_variance_output.loss.backward();
  const auto full_gradients = snapshot_trainable_gradients(full_variance_model);
  const auto no_variance_gradients =
      snapshot_trainable_gradients(no_variance_model);
  full_variance_model->zero_grad();
  torch::manual_seed(631);
  const auto raw_variance_replay =
      full_variance_model->forward(weighted_input, weighted_mask);
  raw_variance_replay.diagnostics.at("vicreg_channel_var_term").backward();
  const auto raw_variance_gradients =
      snapshot_trainable_gradients(full_variance_model);
  for (const auto &[name, full_gradient] : full_gradients) {
    check_close(full_gradient - no_variance_gradients.at(name),
                (0.05 * 0.25 * 25.0) * raw_variance_gradients.at(name),
                "full/no-variance gradient difference is not the effective "
                "variance gradient: " +
                    name,
                1e-5, 1e-6);
  }
}

void test_optimizer_ema_and_archive_contracts() {
  torch::manual_seed(101);
  auto cfg = small_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  model->train();

  int64_t target_tokenizer_parameter_count = 0;
  int64_t target_encoder_parameter_count = 0;
  for (const auto &parameter : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(parameter.key(), "target_tokenizer.")) {
      ++target_tokenizer_parameter_count;
      check(!parameter.value().requires_grad(),
            "target tokenizer parameter is not frozen: " + parameter.key());
    } else if (starts_with(parameter.key(), "target_encoder.")) {
      ++target_encoder_parameter_count;
      check(!parameter.value().requires_grad(),
            "target encoder parameter is not frozen: " + parameter.key());
    }
  }
  check(target_tokenizer_parameter_count > 0,
        "target tokenizer has no registered parameters");
  check(target_encoder_parameter_count > 0,
        "target encoder has no registered parameters");
  check(model->target_ema_distance().item<double>() <= 1e-12,
        "fresh target networks do not exactly match online networks");

  const auto input = deterministic_input(cfg, /*batch_size=*/4);
  const auto mask = torch::ones_like(input).to(torch::kBool);
  const auto before_step = snapshot_parameters(model);
  torch::optim::Adam optimizer(model->parameters(),
                               torch::optim::AdamOptions(1e-3));
  optimizer.zero_grad();
  torch::manual_seed(103);
  auto output = model->forward(input, mask);

  check(output.diagnostics.at("num_target_tokens").item<double>() > 0.0,
        "gradient fixture has no JEPA targets");
  check(output.diagnostics.at("mae_time_active_dims").item<double>() > 0.0,
        "gradient fixture has no time MAE coordinates");
  check(output.diagnostics.at("mae_frequency_active_dims").item<double>() > 0.0,
        "gradient fixture has no frequency MAE coordinates");
  check(output.diagnostics.at("tf_pair_valid_count").item<double>() > 0.0,
        "gradient fixture has no time/frequency pairs");
  check(output.diagnostics.at("vicreg_global_valid_rows").item<double>() > 1.0,
        "gradient fixture has too few global VICReg rows");
  check(output.diagnostics.at("vicreg_channel_valid_rows").item<double>() > 1.0,
        "gradient fixture has too few channel VICReg rows");

  const std::array<std::pair<const char *, const torch::Tensor *>, 9>
      active_losses{{{"total_loss", &output.loss},
                     {"loss_jepa", &output.loss_jepa},
                     {"loss_mae", &output.loss_mae},
                     {"loss_mae_time", &output.loss_mae_time},
                     {"loss_mae_frequency", &output.loss_mae_frequency},
                     {"loss_tf_align", &output.loss_tf_align},
                     {"loss_vicreg", &output.loss_vicreg},
                     {"loss_vicreg_global", &output.loss_vicreg_global},
                     {"loss_vicreg_channel", &output.loss_vicreg_channel}}};
  for (const auto &[name, value] : active_losses) {
    check(finite(*value) && value->item<double>() > 0.0,
          std::string("active loss is zero or non-finite: ") + name);
    const auto &diagnostic = output.diagnostics.at(name);
    check(finite(diagnostic) && diagnostic.item<double>() > 0.0,
          std::string("active loss diagnostic is zero or non-finite: ") + name);
    check(torch::equal(diagnostic, *value),
          std::string("loss diagnostic differs from public field: ") + name);
  }
  const auto &pairwise_tf = output.diagnostics.at("loss_tf_align_pairwise");
  check(finite(pairwise_tf) && pairwise_tf.item<double>() > 0.0,
        "pairwise TF diagnostic is zero or non-finite");
  check(torch::equal(pairwise_tf, output.loss_tf_align),
        "pairwise TF diagnostic differs from public field");

  const auto expected_mae = output.loss_mae_time + output.loss_mae_frequency;
  check(torch::equal(output.loss_mae, expected_mae),
        "MAE loss does not equal its time and frequency sublosses");
  const auto expected_vicreg =
      cfg.lambda_global_vicreg * output.loss_vicreg_global +
      cfg.lambda_channel_vicreg * output.loss_vicreg_channel;
  check(torch::equal(output.loss_vicreg, expected_vicreg),
        "VICReg loss does not equal its weighted public sublosses");

  const auto expected_loss = cfg.lambda_jepa * output.loss_jepa +
                             cfg.lambda_mae * output.loss_mae +
                             cfg.lambda_tf_align * output.loss_tf_align +
                             cfg.lambda_vicreg * output.loss_vicreg;
  check(torch::equal(output.loss, expected_loss),
        "total loss does not exactly match configured branch weights");
  output.loss.backward();

  std::array<bool, kOnlinePrefixes.size()> saw_nonzero_gradient{};
  for (const auto &parameter : model->named_parameters(/*recurse=*/true)) {
    const auto &name = parameter.key();
    const auto grad = parameter.value().grad();
    if (is_target_parameter(name)) {
      check(!grad.defined() || grad.abs().sum().item<double>() == 0.0,
            "target parameter received a gradient: " + name);
      continue;
    }
    if (grad.defined()) {
      check(finite(grad), "non-finite gradient: " + name);
      const int group = online_group(name);
      if (group >= 0 && grad.abs().sum().item<double>() > 0.0) {
        saw_nonzero_gradient[static_cast<std::size_t>(group)] = true;
      }
    }
  }
  for (std::size_t i = 0; i < kOnlinePrefixes.size(); ++i) {
    check(saw_nonzero_gradient[i],
          "no nonzero gradient reached " + std::string(kOnlinePrefixes[i]));
  }

  optimizer.step();
  std::array<bool, kOnlinePrefixes.size()> saw_parameter_update{};
  for (const auto &parameter : model->named_parameters(/*recurse=*/true)) {
    const auto before = before_step.at(parameter.key());
    check(finite(parameter.value()),
          "optimizer produced a non-finite parameter: " + parameter.key());
    if (is_target_parameter(parameter.key())) {
      check(torch::equal(parameter.value().detach(), before),
            "optimizer changed a frozen target parameter: " + parameter.key());
      continue;
    }
    const int group = online_group(parameter.key());
    if (group >= 0 && !torch::equal(parameter.value().detach(), before)) {
      saw_parameter_update[static_cast<std::size_t>(group)] = true;
    }
  }
  for (std::size_t i = 0; i < kOnlinePrefixes.size(); ++i) {
    check(saw_parameter_update[i],
          "optimizer did not update " + std::string(kOnlinePrefixes[i]));
  }

  const double distance_before_ema =
      model->target_ema_distance().item<double>();
  check(std::isfinite(distance_before_ema) && distance_before_ema > 0.0,
        "optimizer step did not separate online and target networks");
  const auto target_before_ema = snapshot_parameters(model);
  check(model->update_target_network(0.5), "EMA update was not applied");
  const double distance_after_ema = model->target_ema_distance().item<double>();
  const double expected_distance = 0.25 * distance_before_ema;
  check(std::abs(distance_after_ema - expected_distance) <=
            std::max(1e-12, std::abs(expected_distance) * 1e-3),
        "EMA squared-distance ratio does not match tau^2");

  const auto after_ema = snapshot_parameters(model);
  for (const auto &[name, before] : target_before_ema) {
    if (online_group(name) < 0) {
      continue;
    }
    const auto it = after_ema.find(name);
    check(it != after_ema.end() && torch::equal(it->second, before),
          "EMA changed an online parameter: " + name);
  }
  for (const auto &[name, old_target] : target_before_ema) {
    if (!is_target_parameter(name)) {
      continue;
    }
    const auto &online = after_ema.at(online_name_for_target(name));
    const auto expected = 0.5 * old_target + 0.5 * online;
    check_close(after_ema.at(name), expected,
                "EMA interpolation mismatch: " + name, 1e-5, 1e-7);
  }

  model->eval();
  const auto reference_encode = model->encode(input, mask);
  const auto reference_target = model->target_encode(input, mask);
  const double reference_ema_distance =
      model->target_ema_distance().item<double>();

  torch::serialize::OutputArchive output_archive;
  model->save(output_archive);
  std::stringstream state(std::ios::in | std::ios::out | std::ios::binary);
  output_archive.save_to(state);
  check(!state.str().empty(), "in-memory model archive is empty");
  state.clear();
  state.seekg(0, std::ios::beg);

  torch::manual_seed(107);
  auto restored = mtf::MtfJepaMaeVicreg(cfg);
  torch::serialize::InputArchive input_archive;
  input_archive.load_from(state, torch::Device(torch::kCPU));
  restored->load(input_archive);
  restored->eval();

  const auto original_parameters = snapshot_parameters(model);
  const auto restored_parameters = snapshot_parameters(restored);
  check(original_parameters.size() == restored_parameters.size(),
        "archive changed parameter count");
  for (const auto &[name, value] : original_parameters) {
    const auto it = restored_parameters.find(name);
    check(it != restored_parameters.end(),
          "archive omitted parameter: " + name);
    check(torch::equal(value, it->second),
          "archive changed parameter: " + name);
  }

  std::map<std::string, torch::Tensor> original_buffers;
  std::map<std::string, torch::Tensor> restored_buffers;
  for (const auto &buffer : model->named_buffers(/*recurse=*/true)) {
    original_buffers.emplace(buffer.key(), buffer.value().detach().clone());
  }
  for (const auto &buffer : restored->named_buffers(/*recurse=*/true)) {
    restored_buffers.emplace(buffer.key(), buffer.value().detach().clone());
  }
  check(original_buffers.size() == restored_buffers.size(),
        "archive changed buffer count");
  for (const auto &[name, value] : original_buffers) {
    const auto it = restored_buffers.find(name);
    check(it != restored_buffers.end() && torch::equal(value, it->second),
          "archive changed buffer: " + name);
  }

  const auto restored_encode = restored->encode(input, mask);
  check_encode_equal(restored_encode, reference_encode,
                     "archive encode equivalence");
  const std::array<mtf::mtf_serving_pool_policy_t, 4> policies{
      mtf::mtf_serving_pool_policy_t::all_tokens,
      mtf::mtf_serving_pool_policy_t::time_only,
      mtf::mtf_serving_pool_policy_t::frequency_only,
      mtf::mtf_serving_pool_policy_t::domain_balanced};
  for (const auto policy : policies) {
    const auto reference_pool =
        mtf::select_mtf_serving_pool(reference_encode, policy, cfg);
    const auto restored_pool =
        mtf::select_mtf_serving_pool(restored_encode, policy, cfg);
    check_close(restored_pool.values, reference_pool.values,
                "archive serving-pool equivalence", 1e-6, 1e-7);
    check(torch::equal(restored_pool.valid_mask, reference_pool.valid_mask),
          "archive serving validity changed");
  }
  check_close(restored->target_encode(input, mask), reference_target,
              "archive target-encoder equivalence", 1e-6, 1e-7);
  check(std::abs(restored->target_ema_distance().item<double>() -
                 reference_ema_distance) <= 1e-12,
        "archive changed target EMA distance");
}

void test_determinism_and_mask_contracts() {
  torch::manual_seed(211);
  auto cfg = small_config();
  cfg.dropout = 0.75;
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  model->eval();
  constexpr int64_t batch_size = 3;
  constexpr int64_t expected_token_count = 16;
  const auto input = deterministic_input(cfg, batch_size);
  const auto full_mask = torch::ones_like(input).to(torch::kBool);

  const auto encode_a = model->encode(input, full_mask);
  const auto encode_b = model->encode(input, full_mask);
  check_encode_equal(encode_a, encode_b, "eval encode is not deterministic");

  torch::manual_seed(223);
  const auto forward_a = model->forward(input, full_mask);
  torch::manual_seed(223);
  const auto forward_b = model->forward(input, full_mask);
  check_forward_equal(forward_a, forward_b,
                      "fixed-seed forward is not reproducible");

  auto partial_mask = full_mask.clone();
  auto dirty = input.clone();
  partial_mask.index_put_({0, 0, 0, 0}, false);
  partial_mask.index_put_({0, 1, 1, 1}, false);
  partial_mask.index_put_({1, 0, 2, 2}, false);
  partial_mask.index_put_({2, 1, 3, 0}, false);
  dirty.index_put_({0, 0, 0, 0}, std::numeric_limits<float>::quiet_NaN());
  dirty.index_put_({0, 1, 1, 1}, std::numeric_limits<float>::infinity());
  dirty.index_put_({1, 0, 2, 2}, -std::numeric_limits<float>::infinity());
  dirty.index_put_({2, 1, 3, 0}, 1.0e30f);

  const auto clean_tokens = model->tokenize(input, partial_mask);
  const auto dirty_tokens = model->tokenize(dirty, partial_mask);
  check_token_batches_equal(dirty_tokens, clean_tokens,
                            "masked sentinels changed tokenization");
  const auto clean_encode = model->encode(input, partial_mask);
  const auto dirty_encode = model->encode(dirty, partial_mask);
  check_encode_equal(dirty_encode, clean_encode,
                     "masked sentinels changed encoding");

  check(partial_mask.index({0, 0, 4, 1}).item<bool>(),
        "sensitivity fixture selected a masked coordinate");
  auto perturbed = input.clone();
  perturbed.index_put_({0, 0, 4, 1},
                       perturbed.index({0, 0, 4, 1}).item<float>() + 7.0f);
  const auto perturbed_tokens = model->tokenize(perturbed, partial_mask);
  check(finite(perturbed_tokens.time_reconstruction_targets) &&
            finite(perturbed_tokens.tokens),
        "valid finite perturbation produced non-finite tokens");
  check(!torch::allclose(perturbed_tokens.time_reconstruction_targets,
                         clean_tokens.time_reconstruction_targets, 1e-7, 1e-8),
        "valid finite perturbation did not change raw token targets");
  check(!torch::allclose(perturbed_tokens.tokens, clean_tokens.tokens, 1e-7,
                         1e-8),
        "valid finite perturbation did not change learned tokens");
  const auto perturbed_encode = model->encode(perturbed, partial_mask);
  check(finite(perturbed_encode.embeddings) &&
            finite(perturbed_encode.pooled_by_channel),
        "valid finite perturbation produced a non-finite encoding");
  check(!torch::allclose(perturbed_encode.embeddings, clean_encode.embeddings,
                         1e-7, 1e-8),
        "valid finite perturbation did not change encoded tokens");
  check(!torch::allclose(perturbed_encode.pooled_by_channel,
                         clean_encode.pooled_by_channel, 1e-7, 1e-8),
        "valid finite perturbation did not change served representation");

  torch::manual_seed(227);
  const auto clean_forward = model->forward(input, partial_mask);
  torch::manual_seed(227);
  const auto dirty_forward = model->forward(dirty, partial_mask);
  check_forward_equal(dirty_forward, clean_forward,
                      "masked sentinels changed losses");

  const auto empty_mask = torch::zeros_like(input).to(torch::kBool);
  torch::manual_seed(229);
  const auto empty = model->forward(input, empty_mask);
  const auto empty_tokens = model->tokenize(input, empty_mask);
  const auto empty_encode = model->encode(input, empty_mask);
  check(empty_tokens.tokens.sizes() ==
                torch::IntArrayRef(
                    {batch_size, expected_token_count, cfg.d_model}) &&
            empty_tokens.token_mask.sizes() ==
                torch::IntArrayRef({batch_size, expected_token_count}),
        "all-invalid token batch has an unexpected shape");
  check(empty.embeddings.sizes() ==
                torch::IntArrayRef(
                    {batch_size, expected_token_count, cfg.latent_dim}) &&
            empty.pooled_embedding.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty.pooled_by_channel.sizes() ==
                torch::IntArrayRef(
                    {batch_size, cfg.channel_count, cfg.latent_dim}) &&
            empty.pooled_time.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty.pooled_frequency.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty.sample_valid_mask.sizes() ==
                torch::IntArrayRef({batch_size}) &&
            empty.channel_valid_mask.sizes() ==
                torch::IntArrayRef({batch_size, cfg.channel_count}),
        "all-invalid forward output has an unexpected shape");
  check(empty_encode.embeddings.sizes() ==
                torch::IntArrayRef(
                    {batch_size, expected_token_count, cfg.latent_dim}) &&
            empty_encode.pooled_embedding.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty_encode.pooled_by_channel.sizes() ==
                torch::IntArrayRef(
                    {batch_size, cfg.channel_count, cfg.latent_dim}) &&
            empty_encode.pooled_time.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty_encode.pooled_frequency.sizes() ==
                torch::IntArrayRef({batch_size, cfg.latent_dim}) &&
            empty_encode.token_mask.sizes() ==
                torch::IntArrayRef({batch_size, expected_token_count}) &&
            empty_encode.sample_valid_mask.sizes() ==
                torch::IntArrayRef({batch_size}) &&
            empty_encode.channel_valid_mask.sizes() ==
                torch::IntArrayRef({batch_size, cfg.channel_count}),
        "all-invalid encode output has an unexpected shape");
  const std::array<torch::Tensor, 9> losses{
      empty.loss,          empty.loss_jepa,          empty.loss_mae,
      empty.loss_mae_time, empty.loss_mae_frequency, empty.loss_tf_align,
      empty.loss_vicreg,   empty.loss_vicreg_global, empty.loss_vicreg_channel};
  for (const auto &loss : losses) {
    check(finite(loss) && loss.item<double>() == 0.0,
          "all-invalid input produced a nonzero or non-finite loss");
  }
  const std::array<torch::Tensor, 10> values{empty.embeddings,
                                             empty.pooled_embedding,
                                             empty.pooled_by_channel,
                                             empty.pooled_time,
                                             empty.pooled_frequency,
                                             empty_encode.embeddings,
                                             empty_encode.pooled_embedding,
                                             empty_encode.pooled_by_channel,
                                             empty_encode.pooled_time,
                                             empty_encode.pooled_frequency};
  for (const auto &value : values) {
    check(finite(value) && value.eq(0).all().item<bool>(),
          "all-invalid input produced a nonzero representation");
  }
  check(!empty_tokens.token_mask.any().item<bool>() &&
            !empty_encode.token_mask.any().item<bool>() &&
            !empty_encode.sample_valid_mask.any().item<bool>() &&
            !empty_encode.channel_valid_mask.any().item<bool>() &&
            !empty.sample_valid_mask.any().item<bool>() &&
            !empty.channel_valid_mask.any().item<bool>(),
        "all-invalid input was marked valid");

  const std::array<mtf::mtf_serving_pool_policy_t, 4> policies{
      mtf::mtf_serving_pool_policy_t::all_tokens,
      mtf::mtf_serving_pool_policy_t::time_only,
      mtf::mtf_serving_pool_policy_t::frequency_only,
      mtf::mtf_serving_pool_policy_t::domain_balanced};
  for (const auto policy : policies) {
    const auto pool = mtf::select_mtf_serving_pool(empty_encode, policy, cfg);
    const auto policy_name =
        std::string(mtf::mtf_serving_pool_policy_name(policy));
    check(pool.values.sizes() ==
                  torch::IntArrayRef(
                      {batch_size, cfg.channel_count, cfg.latent_dim}) &&
              pool.valid_mask.sizes() ==
                  torch::IntArrayRef({batch_size, cfg.channel_count}),
          "all-invalid serving pool has an unexpected shape: " + policy_name);
    check(finite(pool.values) && pool.values.eq(0).all().item<bool>() &&
              !pool.valid_mask.any().item<bool>(),
          "all-invalid serving pool is nonzero or valid: " + policy_name);
  }
  check(empty.diagnostics.at("valid_latent_rows").item<double>() == 0.0 &&
            empty.diagnostics.at("num_context_tokens").item<double>() == 0.0 &&
            empty.diagnostics.at("num_target_tokens").item<double>() == 0.0 &&
            empty.diagnostics.at("tf_pair_valid_count").item<double>() == 0.0 &&
            empty.diagnostics.at("vicreg_valid_rows").item<double>() == 0.0,
        "all-invalid diagnostics report active rows");
}

} // namespace

int main() {
  try {
    test_channel_stratified_vicreg_contracts();
    test_vicreg_debug_projection_and_weighting_contracts();
    test_optimizer_ema_and_archive_contracts();
    test_determinism_and_mask_contracts();
    std::cout << "[PASS] isolated MTF core contracts\n";
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "[FAIL] torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
  }
  return 1;
}
