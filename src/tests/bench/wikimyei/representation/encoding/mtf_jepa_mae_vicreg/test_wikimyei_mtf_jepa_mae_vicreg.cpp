#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/assembly.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool finite_tensor(const torch::Tensor &value) {
  return value.defined() && torch::isfinite(value).all().item<bool>();
}

bool starts_with(const std::string &text, const std::string &prefix) {
  return text.rfind(prefix, 0) == 0;
}

mtf::mtf_jepa_mae_vicreg_config_t small_config() {
  mtf::mtf_jepa_mae_vicreg_config_t cfg{};
  cfg.channel_count = 1;
  cfg.history_length = 32;
  cfg.input_width = 3;
  cfg.d_model = 16;
  cfg.latent_dim = 12;
  cfg.projector_dim = 20;
  cfg.predictor_hidden_dim = 24;
  cfg.num_encoder_layers = 1;
  cfg.num_predictor_layers = 1;
  cfg.num_decoder_layers = 1;
  cfg.num_heads = 2;
  cfg.dropout = 0.0;
  cfg.time_scales = {4, 8};
  cfg.scale_strides = {2, 4};
  cfg.frequency_num_bins = 6;
  cfg.mask_ratio_time = 0.40;
  cfg.mask_ratio_frequency = 0.25;
  cfg.mask_ratio_channel = 0.0;
  cfg.min_context_ratio = 0.30;
  return cfg;
}

torch::Tensor synthetic_input(int64_t batch = 4) {
  constexpr double pi = 3.14159265358979323846;
  auto t = torch::arange(32, torch::kFloat32);
  auto wave = torch::sin(2.0 * pi * t / 8.0);
  auto trend = t / 31.0;
  auto noise = torch::randn({32}, torch::kFloat32) * 0.01;
  auto sample = torch::stack({wave, trend, noise}, /*dim=*/1);
  return sample.unsqueeze(0).unsqueeze(0).repeat({batch, 1, 1, 1});
}

torch::Tensor synthetic_multichannel_input(int64_t batch = 2,
                                           int64_t channels = 3) {
  auto base = synthetic_input(batch).repeat({1, channels, 1, 1});
  for (int64_t c = 0; c < channels; ++c) {
    base.index_put_(
        {torch::indexing::Slice(), c, torch::indexing::Slice(),
         torch::indexing::Slice()},
        base.index({torch::indexing::Slice(), c, torch::indexing::Slice(),
                    torch::indexing::Slice()}) +
            static_cast<double>(c) * 0.1);
  }
  return base;
}

mtf::mtf_input_t legacy_vicreg_weak_view_augmentation(
    const torch::Tensor &x, const torch::Tensor &feature_mask,
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  auto input = mtf::detail::canonicalize_input(x, feature_mask, config);
  auto mask = input.feature_mask.clone();
  const double feature_drop =
      std::min(0.20, std::max(0.0, config.mask_ratio_channel));
  if (feature_drop > 0.0) {
    const auto keep = torch::rand(input.data.sizes(), input.data.options())
                          .lt(1.0 - feature_drop);
    mask = mask.logical_and(keep);
  }
  const double time_drop =
      std::min(0.10, std::max(0.0, config.mask_ratio_time * 0.10));
  if (time_drop > 0.0) {
    auto keep_shape = input.data.sizes().vec();
    keep_shape.back() = 1;
    const auto keep = torch::rand(keep_shape, input.data.options())
                          .lt(1.0 - time_drop)
                          .expand_as(input.data);
    mask = mask.logical_and(keep);
  }
  auto data = torch::where(mask, input.data, torch::zeros_like(input.data));
  data = data + torch::where(mask, torch::randn_like(data) * 0.005,
                             torch::zeros_like(data));
  data = torch::where(mask, data, torch::zeros_like(data));
  return {data, mask};
}

void test_vicreg_weak_view_controls_and_rng_parity() {
  auto config = small_config();
  config.mask_ratio_time = 0.10;
  config.mask_ratio_channel = 0.0;
  auto input = synthetic_input(3);
  auto feature_mask = torch::ones_like(input).to(torch::kBool);

  check(std::fabs(mtf::detail::resolved_vicreg_view_time_dropout_prob(config) -
                  0.01) < 1.0e-12,
        "default VICReg weak-view time drop does not match legacy behavior");

  torch::manual_seed(73);
  const auto legacy =
      legacy_vicreg_weak_view_augmentation(input, feature_mask, config);
  const auto legacy_next = torch::randn({8}, input.options());
  torch::manual_seed(73);
  const auto current = mtf::detail::apply_vicreg_weak_view_augmentation(
      input, feature_mask, config);
  const auto current_next = torch::randn({8}, input.options());
  check(torch::equal(legacy.data, current.data) &&
            torch::equal(legacy.feature_mask, current.feature_mask),
        "explicit VICReg weak-view defaults do not reproduce legacy output");
  check(torch::equal(legacy_next, current_next),
        "explicit VICReg weak-view defaults changed the RNG schedule");

  auto disabled = config;
  disabled.vicreg_view_gaussian_jitter_std = 0.0;
  disabled.vicreg_view_time_dropout_scale = 0.0;
  torch::manual_seed(73);
  const auto neutral = mtf::detail::apply_vicreg_weak_view_augmentation(
      input, feature_mask, disabled);
  const auto neutral_next = torch::randn({8}, input.options());
  check(torch::equal(neutral.data, input) &&
            torch::equal(neutral.feature_mask, feature_mask),
        "disabled VICReg weak-view controls are not an exact identity");
  check(torch::equal(current_next, neutral_next),
        "disabled VICReg weak-view effects changed the RNG schedule");

  auto default_tokens = mtf::TimeFrequencyViewBuilder(config)->forward(
      input, feature_mask);
  auto disabled_tokens = mtf::TimeFrequencyViewBuilder(disabled)->forward(
      input, feature_mask);
  torch::manual_seed(91);
  const auto default_masks =
      mtf::JEPAContextTargetMasker(config).create_masks(default_tokens);
  torch::manual_seed(91);
  const auto disabled_masks =
      mtf::JEPAContextTargetMasker(disabled).create_masks(disabled_tokens);
  check(torch::equal(default_masks.context_mask, disabled_masks.context_mask) &&
            torch::equal(default_masks.target_mask,
                         disabled_masks.target_mask),
        "VICReg weak-view controls changed primary JEPA masking");

  auto invalid_noise = config;
  invalid_noise.vicreg_view_gaussian_jitter_std = -0.001;
  bool rejected_noise = false;
  try {
    auto model = mtf::MtfJepaMaeVicreg(invalid_noise);
    (void)model;
  } catch (const std::runtime_error &) {
    rejected_noise = true;
  }
  check(rejected_noise, "negative VICReg weak-view noise was accepted");

  auto invalid_scale = config;
  invalid_scale.vicreg_view_time_dropout_scale =
      std::numeric_limits<double>::quiet_NaN();
  bool rejected_scale = false;
  try {
    auto model = mtf::MtfJepaMaeVicreg(invalid_scale);
    (void)model;
  } catch (const std::runtime_error &) {
    rejected_scale = true;
  }
  check(rejected_scale, "non-finite VICReg weak-view scale was accepted");
}

void assert_no_pair_leakage(const mtf::mtf_token_batch_t &tokens,
                            const mtf::jepa_context_target_mask_t &masks,
                            const std::string &message) {
  auto domain = tokens.metadata.domain_id.to(torch::kCPU).contiguous();
  auto channel = tokens.metadata.channel_id.to(torch::kCPU).contiguous();
  auto scale = tokens.metadata.scale_id.to(torch::kCPU).contiguous();
  auto start = tokens.metadata.start_index.to(torch::kCPU).contiguous();
  auto width = tokens.metadata.width.to(torch::kCPU).contiguous();
  auto target = masks.target_mask.to(torch::kCPU).contiguous();
  auto context = masks.context_mask.to(torch::kCPU).contiguous();
  auto d = domain.accessor<int64_t, 1>();
  auto c = channel.accessor<int64_t, 1>();
  auto s = scale.accessor<int64_t, 1>();
  auto st = start.accessor<int64_t, 1>();
  auto w = width.accessor<int64_t, 1>();
  auto tgt = target.accessor<bool, 2>();
  auto ctx = context.accessor<bool, 2>();
  for (int64_t b = 0; b < target.size(0); ++b) {
    for (int64_t i = 0; i < target.size(1); ++i) {
      if (!tgt[b][i]) {
        continue;
      }
      for (int64_t j = 0; j < target.size(1); ++j) {
        if (d[i] != d[j] && c[i] == c[j] && s[i] == s[j] && st[i] == st[j] &&
            w[i] == w[j]) {
          check(!ctx[b][j], message);
        }
      }
    }
  }
}

void assert_targets_have_context(const mtf::jepa_context_target_mask_t &masks,
                                 const std::string &message) {
  for (int64_t b = 0; b < masks.valid_mask.size(0); ++b) {
    const bool has_valid = masks.valid_mask.index({b}).any().item<bool>();
    const bool has_target = masks.target_mask.index({b}).any().item<bool>();
    const bool has_context = masks.context_mask.index({b}).any().item<bool>();
    check(!has_valid || !has_target || has_context, message);
  }
}

void test_config_construction() {
  mtf::mtf_jepa_mae_vicreg_config_t cfg{};
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  check(!model.is_empty(), "default config constructs model");

  cfg.time_scales = {3, 5, 9};
  cfg.scale_strides = {};
  cfg.history_length = 16;
  cfg.input_width = 2;
  cfg.d_model = 8;
  cfg.latent_dim = 8;
  cfg.projector_dim = 10;
  cfg.predictor_hidden_dim = 12;
  cfg.num_heads = 1;
  cfg.frequency_num_bins = 4;
  auto tokenizer = mtf::MultiScalePatchTokenizer(cfg);
  auto x = torch::randn({2, 1, 16, 2}, torch::kFloat32);
  auto tokens = tokenizer->forward(x);
  check(tokens.tokens.size(2) == 8, "custom scale config token dim");
  check(tokens.time_reconstruction_targets.size(2) == 4,
        "time raw reconstruction target dim");
  check(tokens.time_reconstruction_mask.size(2) == 4,
        "time raw reconstruction mask dim");

  auto assembly = mtf::make_mtf_jepa_mae_vicreg_assembly();
  check(assembly.family ==
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
        "assembly family is registered under a separate encoder name");
  check(assembly.docks.size() >= 3,
        "assembly exposes pooled and channel representation docks");
}

void test_multiscale_tokenizer_shape() {
  auto cfg = small_config();
  auto x = synthetic_input(2);

  auto single_cfg = cfg;
  single_cfg.time_scales = {8};
  single_cfg.scale_strides = {4};
  auto single = mtf::MultiScalePatchTokenizer(single_cfg)->forward(x);

  auto multi = mtf::MultiScalePatchTokenizer(cfg)->forward(x);
  check(multi.tokens.size(0) == 2, "multi-scale tokenizer preserves batch");
  check(multi.tokens.size(2) == cfg.d_model, "multi-scale token dim");
  check(multi.time_reconstruction_targets.size(2) == 2 * cfg.input_width,
        "time tokenizer stores fixed raw descriptors");
  check(multi.time_reconstruction_mask.sizes() ==
            multi.time_reconstruction_targets.sizes(),
        "time tokenizer stores descriptor masks");
  check(multi.tokens.size(1) > single.tokens.size(1),
        "multiple scales produce more tokens than one scale");
  check(finite_tensor(multi.tokens), "multi-scale tokens are finite");
  check(!multi.tokens.isnan().any().item<bool>(), "multi-scale no NaNs");
}

void test_frequency_tokenizer_shape() {
  auto cfg = small_config();
  auto x = synthetic_input(2);
  auto freq = mtf::FrequencyTokenizer(cfg)->forward(x);
  check(freq.tokens.size(0) == 2, "frequency tokenizer preserves batch");
  check(freq.tokens.size(2) == cfg.d_model, "frequency token dim");
  check(freq.frequency_reconstruction_targets.size(2) ==
            cfg.frequency_num_bins * cfg.input_width,
        "frequency tokenizer stores fixed raw descriptors");
  check(freq.frequency_reconstruction_mask.sizes() ==
            freq.frequency_reconstruction_targets.sizes(),
        "frequency tokenizer stores descriptor masks");
  check(freq.metadata.domain_id.eq(1).all().item<bool>(),
        "frequency tokens carry frequency domain id");
  check(finite_tensor(freq.tokens), "frequency tokens are finite");
}

void test_masker_correctness() {
  torch::manual_seed(11);
  auto cfg = small_config();
  auto x = synthetic_input(2);
  auto tokens = mtf::TimeFrequencyViewBuilder(cfg)->forward(x);
  mtf::JEPAContextTargetMasker masker(cfg);
  auto masks = masker.create_masks(tokens);
  check(!masks.context_mask.logical_and(masks.target_mask).any().item<bool>(),
        "context and target masks do not overlap");
  assert_targets_have_context(
      masks, "samples with targets retain at least one context token");
  check(masks.num_target_tokens > 0, "masker creates target tokens");
  for (int64_t b = 0; b < masks.valid_mask.size(0); ++b) {
    const double context = masks.context_mask.index({b}).sum().item<double>();
    const double valid = masks.valid_mask.index({b}).sum().item<double>();
    check(valid <= 0.0 || context / valid + 1e-12 >= cfg.min_context_ratio,
          "context ratio respects minimum");
  }

  auto tiny_cfg = cfg;
  tiny_cfg.history_length = 5;
  tiny_cfg.input_width = 2;
  tiny_cfg.time_scales = {8};
  tiny_cfg.scale_strides = {};
  tiny_cfg.frequency_num_bins = 3;
  auto tiny_x = torch::ones({1, 1, 5, 2}, torch::kFloat32);
  auto tiny_tokens = mtf::TimeFrequencyViewBuilder(tiny_cfg)->forward(tiny_x);
  auto tiny_masks =
      mtf::JEPAContextTargetMasker(tiny_cfg).create_masks(tiny_tokens);
  check(tiny_masks.num_context_tokens > 0,
        "small sequence still has context support");
  check(!tiny_masks.context_mask.logical_and(tiny_masks.target_mask)
             .any()
             .item<bool>(),
        "small sequence masks do not overlap");
  assert_targets_have_context(
      tiny_masks, "small sequence target is not kept without context");
}

void test_forward_pass_and_encode() {
  torch::manual_seed(17);
  auto cfg = small_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto x = synthetic_input(4);
  auto out = model->forward(x);
  check(finite_tensor(out.loss), "forward total loss is finite");
  check(finite_tensor(out.loss_jepa), "JEPA loss is finite");
  check(finite_tensor(out.loss_mae), "MAE loss is finite");
  check(finite_tensor(out.loss_tf_align), "TF alignment loss is finite");
  check(finite_tensor(out.loss_vicreg), "VICReg loss is finite");
  check(out.embeddings.size(0) == 4 && out.embeddings.size(2) == cfg.latent_dim,
        "forward embeddings have expected shape");
  check(out.pooled_embedding.sizes() == torch::IntArrayRef({4, cfg.latent_dim}),
        "forward pooled embedding shape");
  check(out.pooled_by_channel.sizes() ==
            torch::IntArrayRef({4, cfg.channel_count, cfg.latent_dim}),
        "forward channel pooled embedding shape");
  check(out.pooled_time.sizes() == torch::IntArrayRef({4, cfg.latent_dim}),
        "forward time pooled embedding shape");
  check(out.pooled_frequency.sizes() == torch::IntArrayRef({4, cfg.latent_dim}),
        "forward frequency pooled embedding shape");
  check(out.diagnostics.count("loss_jepa") == 1 &&
            out.diagnostics.count("vicreg_var_term") == 1 &&
            out.diagnostics.count("vicreg_valid_rows") == 1 &&
            out.diagnostics.count("tf_pair_count") == 1 &&
            out.diagnostics.count("valid_latent_rows") == 1 &&
            out.diagnostics.count("loss_mae_time") == 1 &&
            out.diagnostics.count("loss_mae_frequency") == 1,
        "forward returns named diagnostics");
  check(finite_tensor(out.loss_mae_time) &&
            finite_tensor(out.loss_mae_frequency),
        "split MAE losses are finite");
  check(finite_tensor(out.loss_vicreg_global) &&
            finite_tensor(out.loss_vicreg_channel),
        "split VICReg losses are finite");
  check(out.sample_valid_mask.sizes() == torch::IntArrayRef({4}),
        "forward returns sample validity mask");
  check(out.channel_valid_mask.sizes() ==
            torch::IntArrayRef({4, cfg.channel_count}),
        "forward returns channel validity mask");
  check(out.diagnostics["tf_pair_count"].item<double>() > 0.0,
        "frequency-enabled forward reports TF pairs");
  check(out.diagnostics["tf_pair_valid_count"].item<double>() > 0.0,
        "frequency-enabled forward reports valid TF pairs");
  check(out.diagnostics["valid_latent_rows"].item<double>() > 0.0,
        "forward reports valid latent rows");

  auto zeros = torch::zeros({2, 1, 32, 3}, torch::kFloat32);
  auto zero_out = model->forward(zeros);
  check(finite_tensor(zero_out.loss), "zero input loss is finite");
  auto constant = torch::ones({2, 1, 32, 3}, torch::kFloat32) * 7.0;
  auto constant_out = model->forward(constant);
  check(finite_tensor(constant_out.loss), "constant input loss is finite");

  auto enc = model->encode(x);
  check(enc.embeddings.size(0) == 4 && enc.embeddings.size(2) == cfg.latent_dim,
        "encode returns token embeddings");
  check(enc.pooled_embedding.sizes() == torch::IntArrayRef({4, cfg.latent_dim}),
        "encode returns pooled embedding");
  check(enc.pooled_by_channel.sizes() ==
            torch::IntArrayRef({4, cfg.channel_count, cfg.latent_dim}),
        "encode returns channel pooled embedding");
  check(enc.sample_valid_mask.sizes() == torch::IntArrayRef({4}),
        "encode returns sample validity mask");
  check(enc.channel_valid_mask.sizes() ==
            torch::IntArrayRef({4, cfg.channel_count}),
        "encode returns channel validity mask");
  check(finite_tensor(enc.pooled_embedding), "encode pooled embedding finite");
}

void test_backward_pass() {
  torch::manual_seed(19);
  auto cfg = small_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto out = model->forward(synthetic_input(3));
  out.loss.backward();

  bool saw_online_grad = false;
  bool saw_target_grad = false;
  for (const auto &param : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(param.key(), "encoder.") &&
        param.value().grad().defined()) {
      auto grad = param.value().grad();
      if (finite_tensor(grad) && grad.abs().sum().item<double>() > 0.0) {
        saw_online_grad = true;
      }
    }
    if (starts_with(param.key(), "target_encoder.") &&
        param.value().grad().defined() &&
        param.value().grad().abs().sum().item<double>() > 0.0) {
      saw_target_grad = true;
    }
  }
  check(saw_online_grad, "backward creates finite nonzero online encoder grad");
  check(!saw_target_grad, "target encoder has no training gradient");
}

void test_branch_switches() {
  auto x = synthetic_input(3);
  {
    auto cfg = small_config();
    cfg.use_frequency_tokens = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "frequency-disabled forward finite");
    check(out.loss_tf_align.item<double>() == 0.0,
          "frequency-disabled TF alignment is zero");
    check(out.diagnostics["tf_pair_count"].item<double>() == 0.0,
          "frequency-disabled TF pair count is zero");
  }
  {
    auto cfg = small_config();
    cfg.use_mae_decoder = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "MAE-disabled forward finite");
    check(out.loss_mae.item<double>() == 0.0, "MAE-disabled loss is zero");
  }
  {
    auto cfg = small_config();
    cfg.use_jepa_loss = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "JEPA-disabled forward finite");
    check(out.loss_jepa.item<double>() == 0.0, "JEPA-disabled loss is zero");
  }
  {
    auto cfg = small_config();
    cfg.use_tf_align_loss = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "TF-disabled forward finite");
    check(out.loss_tf_align.item<double>() == 0.0, "TF-disabled loss is zero");
  }
  {
    auto cfg = small_config();
    cfg.use_vicreg_loss = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "VICReg-disabled forward finite");
    check(out.loss_vicreg.item<double>() == 0.0,
          "VICReg-disabled loss is zero");
  }
  {
    auto cfg = small_config();
    cfg.use_raw_reconstruction_targets = false;
    auto out = mtf::MtfJepaMaeVicreg(cfg)->forward(x);
    check(finite_tensor(out.loss), "projected-reconstruction fallback finite");
  }
}

void test_multichannel_encode() {
  auto cfg = small_config();
  cfg.channel_count = 3;
  auto x = synthetic_multichannel_input(2, 3);
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto enc = model->encode(x);
  check(enc.pooled_by_channel.sizes() ==
            torch::IntArrayRef({2, 3, cfg.latent_dim}),
        "multichannel encode returns [B,C,D]");
  for (int64_t c = 0; c < 3; ++c) {
    check(enc.metadata.channel_id.eq(c).any().item<bool>(),
          "metadata covers each channel");
  }
}

void test_all_and_partial_feature_masks() {
  auto cfg = small_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto x = synthetic_input(2);

  auto none_valid = torch::zeros_like(x).to(torch::kBool);
  auto empty_out = model->forward(x, none_valid);
  check(finite_tensor(empty_out.loss), "all-invalid mask forward finite");
  check(empty_out.diagnostics["vicreg_valid_rows"].item<double>() == 0.0,
        "all-invalid mask skips VICReg rows");
  check(empty_out.diagnostics["valid_latent_rows"].item<double>() == 0.0,
        "all-invalid mask reports zero valid latent rows");
  check(finite_tensor(empty_out.diagnostics["latent_std"]) &&
            finite_tensor(empty_out.diagnostics["latent_norm"]),
        "all-invalid latent diagnostics are finite");

  auto partial = torch::rand(x.sizes(), x.options()).gt(0.35);
  auto partial_out = model->forward(x, partial);
  check(finite_tensor(partial_out.loss), "partial feature mask forward finite");
  check(partial_out.diagnostics["valid_latent_rows"].item<double>() > 0.0,
        "partial feature mask reports valid latent rows");
  auto partial_tokens = model->tokenize(x, partial);
  check(partial_tokens.token_mask.any().item<bool>(),
        "partial feature mask leaves some token support");
}

void test_mask_leakage_policy() {
  torch::manual_seed(31);
  auto cfg = small_config();
  cfg.min_context_ratio = 0.10;
  cfg.couple_time_frequency_masks = true;
  cfg.mask_same_window_across_domains = true;
  auto tokens = mtf::TimeFrequencyViewBuilder(cfg)->forward(synthetic_input(2));
  auto masks = mtf::JEPAContextTargetMasker(cfg).create_masks(tokens);
  assert_targets_have_context(
      masks, "mask leakage policy keeps context for remaining targets");
  assert_no_pair_leakage(tokens, masks,
                         "paired time/frequency counterpart is not visible "
                         "context");

  auto capped_cfg = cfg;
  capped_cfg.mask_ratio_time = 0.95;
  capped_cfg.mask_ratio_frequency = 0.95;
  capped_cfg.min_context_ratio = 0.75;
  auto capped_tokens =
      mtf::TimeFrequencyViewBuilder(capped_cfg)->forward(synthetic_input(2));
  auto capped_masks =
      mtf::JEPAContextTargetMasker(capped_cfg).create_masks(capped_tokens);
  assert_targets_have_context(capped_masks,
                              "post-capping targets retain context");
  assert_no_pair_leakage(capped_tokens, capped_masks,
                         "post-capping paired counterpart is not visible "
                         "context");
  check(capped_masks.hard_forbidden_count > 0,
        "post-capping mask recomputes hard forbids");

  auto soft_cfg = cfg;
  soft_cfg.mask_ratio_time = 0.95;
  soft_cfg.mask_ratio_frequency = 0.95;
  soft_cfg.min_context_ratio = 0.90;
  soft_cfg.max_context_target_time_overlap = 0.0;
  auto soft_tokens =
      mtf::TimeFrequencyViewBuilder(soft_cfg)->forward(synthetic_input(1));
  auto soft_masks =
      mtf::JEPAContextTargetMasker(soft_cfg).create_masks(soft_tokens);
  assert_targets_have_context(soft_masks,
                              "soft-relaxed targets retain context");
  assert_no_pair_leakage(soft_tokens, soft_masks,
                         "hard same-window forbids are not relaxed");
  check(soft_masks.hard_forbidden_count > 0,
        "hard forbidden diagnostics are populated");
  check(soft_masks.relaxed_soft_forbidden_count > 0,
        "soft forbids can relax for min-context pressure");
}

void test_context_starvation_reduces_targets() {
  auto cfg = small_config();
  cfg.history_length = 4;
  cfg.input_width = 1;
  cfg.d_model = 8;
  cfg.latent_dim = 8;
  cfg.projector_dim = 10;
  cfg.predictor_hidden_dim = 12;
  cfg.num_heads = 2;
  cfg.time_scales = {4};
  cfg.scale_strides = {};
  cfg.frequency_num_bins = 3;
  cfg.mask_ratio_time = 1.0;
  cfg.mask_ratio_frequency = 0.0;
  cfg.min_context_ratio = 0.5;
  cfg.couple_time_frequency_masks = true;
  cfg.mask_same_window_across_domains = true;
  auto x = torch::ones({1, 1, 4, 1}, torch::kFloat32);
  auto tokens = mtf::TimeFrequencyViewBuilder(cfg)->forward(x);
  auto masks = mtf::JEPAContextTargetMasker(cfg).create_masks(tokens);
  check(masks.reduced_targets_for_context_count > 0,
        "hard-forbid infeasible case reduces targets");
  check(masks.context_starved_sample_count == 0,
        "target reduction avoids context starvation");
  check(masks.min_context_satisfied_count == 1,
        "minimum context is satisfied after target reduction");
  check(masks.num_context_tokens >= 1, "context remains available");
  check(masks.num_target_tokens == 0,
        "tiny paired-view case sacrifices target instead of context");
}

void test_all_masked_attention_zeroes_context() {
  auto q = torch::randn({2, 3, 8}, torch::kFloat32);
  auto k = torch::randn({2, 4, 8}, torch::kFloat32);
  auto v = torch::ones({2, 4, 8}, torch::kFloat32) * 7.0;
  auto mask = torch::zeros({2, 4}, torch::kBool);
  auto out = mtf::detail::multi_head_context_attention(q, k, v, mask, 2);
  check(finite_tensor(out), "all-masked attention stays finite");
  check(out.abs().max().item<double>() == 0.0,
        "all-masked attention returns exactly zero context");
}

void test_descriptor_masks() {
  using torch::indexing::Slice;
  auto cfg = small_config();
  auto x = synthetic_input(1);
  auto feature_mask = torch::ones_like(x).to(torch::kBool);
  feature_mask.index_put_({Slice(), Slice(), Slice(), 1}, false);

  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto batch = model->tokenize(x, feature_mask);
  auto domain = batch.metadata.domain_id.to(torch::kCPU).contiguous();
  auto domain_acc = domain.accessor<int64_t, 1>();
  for (int64_t n = 0; n < domain.size(0); ++n) {
    if (domain_acc[n] == 0) {
      check(!batch.time_reconstruction_mask.index({0, n, 1}).item<bool>(),
            "time descriptor mask excludes invalid feature mean");
      check(!batch.time_reconstruction_mask.index({0, n, cfg.input_width + 1})
                 .item<bool>(),
            "time descriptor mask excludes invalid feature std");
    } else {
      auto invalid_freq_block = batch.frequency_reconstruction_mask.index(
          {0, n, Slice(cfg.frequency_num_bins, 2 * cfg.frequency_num_bins)});
      check(!invalid_freq_block.any().item<bool>(),
            "frequency descriptor mask excludes invalid feature bins");
    }
  }
  auto out = model->forward(x, feature_mask);
  check(finite_tensor(out.loss_mae), "raw MAE with descriptor masks is finite");
}

void test_frequency_small_window_and_strict_loss() {
  auto cfg = small_config();
  cfg.history_length = 8;
  cfg.input_width = 2;
  cfg.time_scales = {4};
  cfg.scale_strides = {2};
  cfg.frequency_num_bins = 16;
  auto x = torch::randn({2, 1, 8, 2}, torch::kFloat32);
  auto freq = mtf::FrequencyTokenizer(cfg)->forward(x);
  check(freq.frequency_reconstruction_targets.size(2) == 32,
        "small-window frequency descriptor keeps fixed padded width");
  check(finite_tensor(freq.frequency_reconstruction_targets),
        "small-window frequency descriptor is finite");

  bool threw = false;
  try {
    (void)mtf::detail::checked_loss(
        torch::tensor(std::numeric_limits<double>::quiet_NaN(),
                      torch::kFloat32),
        "intentional_nan", true);
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, "strict finite loss rejects NaN");
  auto sanitized = mtf::detail::checked_loss(
      torch::tensor(std::numeric_limits<double>::quiet_NaN(), torch::kFloat32),
      "intentional_nan", false);
  check(sanitized.item<double>() == 0.0,
        "non-strict finite loss sanitizes NaN");
}

void test_vicreg_stability() {
  auto z = torch::zeros({4, 1, 8}, torch::kFloat32);
  auto mask = torch::ones({4, 1}, torch::kBool);
  auto result = mtf::compute_vicreg_stability_loss(z, mask, z, mask);
  check(finite_tensor(result.loss), "local VICReg-style loss is finite");
  check(result.variance_loss.item<double>() > 0.0,
        "collapsed representation has nonzero variance penalty");

  auto cfg = small_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto out = model->forward(synthetic_input(4));
  check(finite_tensor(out.loss_vicreg),
        "integrated VICReg branch contributes finite loss");
}

void test_channel_vicreg_rows() {
  torch::manual_seed(37);
  auto cfg = small_config();
  cfg.channel_count = 3;
  cfg.use_channel_vicreg = true;
  cfg.mask_ratio_channel = 0.0;
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  auto out = model->forward(synthetic_multichannel_input(2, 3));
  check(finite_tensor(out.loss_vicreg), "channel VICReg loss is finite");
  check(out.diagnostics["vicreg_mode"].item<double>() == 3.0,
        "global and channel VICReg diagnostic mode is set");
  check(finite_tensor(out.diagnostics["loss_vicreg_global"]) &&
            finite_tensor(out.diagnostics["loss_vicreg_channel"]),
        "composable VICReg diagnostics are finite");
  const auto rows = out.diagnostics["vicreg_channel_valid_rows"].item<double>();
  check(rows > 2.0 && rows <= 6.0,
        "channel VICReg sees channel rows instead of only sample rows");
}

void test_num_heads_validation() {
  auto cfg = small_config();
  cfg.num_heads = 3;
  auto ok = mtf::MtfJepaMaeVicreg(cfg);
  auto out = ok->forward(synthetic_input(2));
  check(finite_tensor(out.loss), "divisible num_heads config runs");

  cfg.num_heads = 5;
  bool threw = false;
  try {
    (void)mtf::MtfJepaMaeVicreg(cfg);
  } catch (const std::exception &) {
    threw = true;
  }
  check(threw, "invalid num_heads divisibility throws");
}

void test_ema_target_update() {
  torch::manual_seed(29);
  auto cfg = small_config();
  cfg.use_target_ema = true;
  auto model = mtf::MtfJepaMaeVicreg(cfg);

  torch::Tensor tokenizer_before;
  bool saw_frozen_target_tokenizer = false;
  for (const auto &param : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(param.key(), "target_tokenizer.")) {
      if (!tokenizer_before.defined()) {
        tokenizer_before = param.value().detach().clone();
      }
      check(!param.value().requires_grad(),
            "target tokenizer parameter is frozen");
      saw_frozen_target_tokenizer = true;
    }
  }
  check(tokenizer_before.defined() && saw_frozen_target_tokenizer,
        "found frozen target tokenizer parameters");

  torch::Tensor before;
  for (const auto &param : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(param.key(), "target_encoder.")) {
      check(!param.value().requires_grad(),
            "target encoder parameter is frozen");
      before = param.value().detach().clone();
      break;
    }
  }
  check(before.defined(), "found target encoder parameter");

  bool mutated_online = false;
  bool mutated_online_tokenizer = false;
  for (const auto &param : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(param.key(), "tokenizer.") && !mutated_online_tokenizer) {
      param.value().data().add_(0.25);
      mutated_online_tokenizer = true;
      continue;
    }
    if (starts_with(param.key(), "encoder.")) {
      param.value().data().add_(0.5);
      mutated_online = true;
      break;
    }
  }
  check(mutated_online, "mutated online encoder parameter for EMA test");
  check(mutated_online_tokenizer,
        "mutated online tokenizer parameter for EMA test");
  check(model->update_target_network(0.5),
        "EMA target network update reports applied");

  torch::Tensor after;
  torch::Tensor tokenizer_after;
  for (const auto &param : model->named_parameters(/*recurse=*/true)) {
    if (starts_with(param.key(), "target_tokenizer.") &&
        !tokenizer_after.defined()) {
      tokenizer_after = param.value().detach().clone();
    }
    if (starts_with(param.key(), "target_encoder.")) {
      after = param.value().detach().clone();
    }
    if (after.defined() && tokenizer_after.defined()) {
      break;
    }
  }
  check(!torch::allclose(tokenizer_before, tokenizer_after),
        "target tokenizer parameter changes after EMA update");
  check(!torch::allclose(before, after),
        "target encoder parameter changes after EMA update");

  cfg.use_target_ema = false;
  auto disabled = mtf::MtfJepaMaeVicreg(cfg);
  check(!disabled->update_target_encoder(),
        "disabled EMA update reports no-op");
}

void test_target_branch_deterministic_in_train_mode() {
  torch::manual_seed(41);
  auto cfg = small_config();
  cfg.dropout = 0.75;
  cfg.use_target_ema = true;
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  model->train();
  auto x = synthetic_input(2);
  auto first = model->target_encode(x);
  auto second = model->target_encode(x);
  check(torch::allclose(first, second, 1e-7, 1e-7),
        "EMA target branch is deterministic while parent is training");
}

} // namespace

int main() {
  torch::manual_seed(7);
  test_config_construction();
  test_vicreg_weak_view_controls_and_rng_parity();
  test_multiscale_tokenizer_shape();
  test_frequency_tokenizer_shape();
  test_masker_correctness();
  test_forward_pass_and_encode();
  test_backward_pass();
  test_branch_switches();
  test_multichannel_encode();
  test_all_and_partial_feature_masks();
  test_mask_leakage_policy();
  test_context_starvation_reduces_targets();
  test_all_masked_attention_zeroes_context();
  test_descriptor_masks();
  test_frequency_small_window_and_strict_loss();
  test_vicreg_stability();
  test_channel_vicreg_rows();
  test_num_heads_validation();
  test_ema_target_update();
  test_target_branch_deterministic_in_train_mode();
  std::cout << "MTF-JEPA-MAE-VICReg smoke tests passed\n";
  return 0;
}
