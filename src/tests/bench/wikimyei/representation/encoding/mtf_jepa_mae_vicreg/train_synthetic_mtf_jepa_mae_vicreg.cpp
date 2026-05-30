#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

struct synthetic_batch_t {
  torch::Tensor data{};
  torch::Tensor mask{};
};

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool finite_tensor(const torch::Tensor &value) {
  return value.defined() && torch::isfinite(value).all().item<bool>();
}

mtf::mtf_jepa_mae_vicreg_config_t synthetic_config() {
  mtf::mtf_jepa_mae_vicreg_config_t cfg{};
  cfg.channel_count = 3;
  cfg.history_length = 32;
  cfg.input_width = 3;
  cfg.d_model = 16;
  cfg.latent_dim = 12;
  cfg.projector_dim = 20;
  cfg.predictor_hidden_dim = 24;
  cfg.num_encoder_layers = 1;
  cfg.num_predictor_layers = 1;
  cfg.num_decoder_layers = 1;
  cfg.num_heads = 3;
  cfg.dropout = 0.05;
  cfg.time_scales = {4, 8};
  cfg.scale_strides = {2, 4};
  cfg.frequency_num_bins = 6;
  cfg.mask_ratio_time = 0.35;
  cfg.mask_ratio_frequency = 0.20;
  cfg.mask_ratio_channel = 0.02;
  cfg.min_context_ratio = 0.35;
  cfg.lambda_jepa = 1.0;
  cfg.lambda_mae = 0.20;
  cfg.lambda_tf_align = 0.05;
  cfg.lambda_vicreg = 0.02;
  cfg.target_ema_tau = 0.98;
  cfg.use_channel_vicreg = true;
  cfg.lambda_global_vicreg = 0.5;
  cfg.lambda_channel_vicreg = 0.5;
  return cfg;
}

torch::Tensor stack_features(const torch::Tensor &wave,
                             const torch::Tensor &trend,
                             const torch::Tensor &noise) {
  return torch::stack({wave, trend, noise}, /*dim=*/1);
}

synthetic_batch_t
make_synthetic_batch(const mtf::mtf_jepa_mae_vicreg_config_t &cfg,
                     int64_t batch_size, int64_t family, int64_t step) {
  using torch::indexing::Slice;
  constexpr double pi = 3.14159265358979323846;
  auto options = torch::TensorOptions().dtype(cfg.dtype).device(cfg.device);
  auto t = torch::arange(cfg.history_length, options);
  auto norm_t =
      t / static_cast<double>(std::max<int64_t>(1, cfg.history_length - 1));
  auto data = torch::zeros(
      {batch_size, cfg.channel_count, cfg.history_length, cfg.input_width},
      options);

  for (int64_t b = 0; b < batch_size; ++b) {
    for (int64_t c = 0; c < cfg.channel_count; ++c) {
      const double phase = 0.13 * static_cast<double>(step + b + c);
      const double channel_gain = 1.0 + 0.10 * static_cast<double>(c);
      auto noise = torch::randn({cfg.history_length}, options) * 0.03;
      auto trend = norm_t * (0.25 + 0.05 * static_cast<double>(c)) +
                   static_cast<double>(b) * 0.01;
      torch::Tensor wave;
      if (family == 0) {
        wave = channel_gain * torch::sin(2.0 * pi * norm_t * 4.0 + phase);
      } else if (family == 1) {
        auto slow = torch::sin(2.0 * pi * norm_t * 3.0 + phase);
        auto fast = torch::sin(2.0 * pi * norm_t * 7.0 + phase);
        wave = torch::where(t.lt(cfg.history_length / 2), slow, fast);
      } else if (family == 2) {
        auto common = torch::sin(2.0 * pi * norm_t * 5.0 + phase);
        auto local = torch::cos(2.0 * pi * norm_t * (2.0 + c) + phase);
        wave = channel_gain * common + 0.20 * local;
      } else {
        wave = channel_gain * torch::sin(2.0 * pi * norm_t * 4.0 + phase);
        trend = trend + 0.15 * torch::sin(2.0 * pi * norm_t);
        noise = torch::randn({cfg.history_length}, options) * 0.06;
      }
      data.index_put_({b, c, Slice(), Slice()},
                      stack_features(wave, trend, noise));
    }
  }

  auto mask = torch::ones(
      data.sizes(),
      torch::TensorOptions().dtype(torch::kBool).device(cfg.device));
  if (family == 3) {
    mask = torch::rand(data.sizes(), options).gt(0.30);
  } else if (step % 5 == 0) {
    mask = torch::rand(data.sizes(), options).gt(0.08);
  }
  data = torch::where(mask, data, torch::zeros_like(data));
  return {data, mask};
}

} // namespace

int main() {
  torch::manual_seed(101);
  auto cfg = synthetic_config();
  auto model = mtf::MtfJepaMaeVicreg(cfg);
  model->train();
  torch::optim::AdamW optimizer(
      model->parameters(),
      torch::optim::AdamWOptions(1.0e-3).weight_decay(1.0e-4));

  constexpr int64_t steps = 80;
  double final_loss = 0.0;
  double final_latent_std = 0.0;
  for (int64_t step = 0; step < steps; ++step) {
    auto batch = make_synthetic_batch(cfg, /*batch_size=*/4, step % 4, step);
    optimizer.zero_grad();
    auto out = model->forward(batch.data, batch.mask);
    check(finite_tensor(out.loss), "synthetic pretraining loss is finite");
    check(finite_tensor(out.loss_jepa), "synthetic JEPA loss is finite");
    check(finite_tensor(out.loss_mae), "synthetic MAE loss is finite");
    out.loss.backward();
    optimizer.step();
    model->update_target_network();

    final_loss = out.loss.item<double>();
    final_latent_std = out.diagnostics["latent_std"].item<double>();
    check(std::isfinite(final_loss), "synthetic scalar loss is finite");
    if (step >= 20) {
      check(final_latent_std > 1.0e-8,
            "latent_std remains positive after warmup");
    }
    if (step % 20 == 0 || step + 1 == steps) {
      std::cout
          << "synthetic_step=" << step << " loss=" << final_loss
          << " loss_jepa=" << out.loss_jepa.item<double>()
          << " loss_mae_time=" << out.loss_mae_time.item<double>()
          << " loss_mae_frequency=" << out.loss_mae_frequency.item<double>()
          << " loss_tf_align=" << out.loss_tf_align.item<double>()
          << " loss_vicreg=" << out.loss_vicreg.item<double>()
          << " latent_std=" << final_latent_std << " valid_latent_rows="
          << out.diagnostics["valid_latent_rows"].item<double>()
          << " tf_pair_valid_count="
          << out.diagnostics["tf_pair_valid_count"].item<double>()
          << " vicreg_valid_rows="
          << out.diagnostics["vicreg_valid_rows"].item<double>()
          << " context_starved_sample_count="
          << out.diagnostics["context_starved_sample_count"].item<double>()
          << " target_ema_distance="
          << out.diagnostics["target_ema_distance"].item<double>() << '\n';
    }
  }

  std::cout
      << "Synthetic MTF-JEPA-MAE-VICReg training smoke passed: final_loss="
      << final_loss << " final_latent_std=" << final_latent_std << '\n';
  return 0;
}
