#include <torch/torch.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

#include "iitepi/runtime_binding_space_t.h"
#include "piaabo/dconfig.h"
#include "wikimyei/representation/VICReg/vicreg_4d_dilated_conv.h"
#include "wikimyei/representation/VICReg/vicreg_4d.h"

namespace {

using cuwacunu::wikimyei::vicreg_4d::ActKind;
using cuwacunu::wikimyei::vicreg_4d::ConvBlock;
using cuwacunu::wikimyei::vicreg_4d::NormKind;
using cuwacunu::wikimyei::vicreg_4d::ProjectorOptions;
using cuwacunu::wikimyei::vicreg_4d::VICReg_4D;
using cuwacunu::wikimyei::vicreg_4d::WarpBaseCurve;
using cuwacunu::wikimyei::vicreg_4d::WarpPreset;

ProjectorOptions projector_options() {
  return ProjectorOptions{
      .norm_kind = NormKind::LayerNorm,
      .act_kind = ActKind::SiLU,
      .use_hidden_bias = false,
      .use_last_bias = false,
      .bn_in_fp32 = true,
  };
}

std::string load_contract_hash() {
  const char* global_config_path = "/cuwacunu/src/config/.config";
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
  cuwacunu::iitepi::config_space_t::update_config();
  return cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
      cuwacunu::iitepi::config_space_t::locked_campaign_hash(),
      cuwacunu::iitepi::config_space_t::locked_binding_id());
}

std::unique_ptr<VICReg_4D> make_cpu_model(const std::string& contract_hash,
                                          int C,
                                          int T,
                                          int D,
                                          int encoding_dims,
                                          int channel_expansion_dim,
                                          int fused_feature_dim,
                                          int hidden_dims,
                                          int depth,
                                          const std::string& projector_mlp_spec) {
  return std::make_unique<VICReg_4D>(contract_hash,
                                     "tsi.wikimyei.representation.vicreg",
                                     C,
                                     T,
                                     D,
                                     encoding_dims,
                                     channel_expansion_dim,
                                     fused_feature_dim,
                                     hidden_dims,
                                     depth,
                                     projector_mlp_spec,
                                     torch::kFloat32,
                                     torch::kCPU,
                                     /*optimizer_threshold_reset=*/-1,
                                     /*enable_buffer_averaging=*/false,
                                     projector_options());
}

bool nearly_equal(double lhs, double rhs, double tol = 1e-6) {
  return std::abs(lhs - rhs) <= tol;
}

bool run_conv_block_internal_mask_test() {
  torch::manual_seed(77);
  ConvBlock block(/*in_channels=*/1, /*out_channels=*/1, /*kernel_size=*/3,
                  /*dilation=*/1, /*final=*/false);
  block->eval();

  {
    torch::NoGradGuard no_grad;
    block->conv1->conv->weight.zero_();
    block->conv1->conv->bias.fill_(1.0);

    block->conv2->conv->weight.fill_(1.0);
    block->conv2->conv->bias.zero_();
  }

  auto x = torch::zeros({1, 1, 3}, torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::tensor({{{false, true, false}}},
                            torch::TensorOptions().dtype(torch::kBool));

  auto out = block->forward(x, mask);

  const double expected_center =
      torch::gelu(torch::tensor(1.0f)).item<double>();
  const double center = out.index({0, 0, 1}).item<double>();
  if (!nearly_equal(center, expected_center, 1e-5)) {
    std::cerr << "[phase1.1][conv_block] center output mismatch: expected "
              << expected_center << " got " << center
              << " (likely within-block mask leak)\n";
    return false;
  }

  const double left = out.index({0, 0, 0}).item<double>();
  const double right = out.index({0, 0, 2}).item<double>();
  if (!nearly_equal(left, 0.0) || !nearly_equal(right, 0.0)) {
    std::cerr << "[phase1.1][conv_block] invalid outputs are not zero: left="
              << left << " right=" << right << "\n";
    return false;
  }

  return true;
}

bool run_loss_wiring_test(const std::string& contract_hash) {
  torch::manual_seed(1234);
  auto model =
      make_cpu_model(contract_hash, /*C=*/2, /*T=*/3, /*D=*/2, /*E=*/4,
                     /*channel_expansion_dim=*/3, /*fused_feature_dim=*/2,
                     /*hidden_dims=*/3, /*depth=*/1, "4-6-4");

  model->aug.warp_presets = {
      WarpPreset{WarpBaseCurve::Linear,
                 /*curve_param=*/0.0,
                 /*noise_scale=*/0.0,
                 /*smoothing_kernel_size=*/1,
                 /*point_drop_prob=*/0.0,
                 /*value_jitter_std=*/0.0,
                 /*time_mask_band_frac=*/0.0,
                 /*channel_dropout_prob=*/0.0}};

  auto data = torch::tensor(
      {{{{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}},
        {{0.5f, 1.5f}, {2.5f, 3.5f}, {4.5f, 5.5f}}}},
      torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::tensor({{{true, true, false}, {true, true, false}}},
                            torch::TensorOptions().dtype(torch::kBool));

  model->_encoder_net->train();
  model->_projector_net->train();
  model->_swa_encoder_net->encoder().train();

  constexpr std::uint64_t seed = 4321;
  torch::manual_seed(seed);
  auto expected_terms = [&]() {
    torch::NoGradGuard no_grad;
    auto data_d = data.to(model->device);
    auto mask_d = mask.to(model->device);

    auto view1 = model->aug.augment(data_d, mask_d);
    auto view2 = model->aug.augment(data_d, mask_d);
    auto d12 = torch::cat({view1.first, view2.first}, /*dim=*/0);
    auto m12 = torch::cat({view1.second, view2.second}, /*dim=*/0);
    auto k12 = model->_encoder_net(d12, m12);

    const auto batch = view1.first.size(0);
    auto k1 = k12.narrow(/*dim=*/0, /*start=*/0, /*length=*/batch);
    auto k2 = k12.narrow(/*dim=*/0, /*start=*/batch, /*length=*/batch);
    auto valid_bt = (view1.second & view2.second).all(/*dim=*/1);

    auto expand_E = [&](int64_t E) {
      return valid_bt.unsqueeze(-1).expand({k1.size(0), k1.size(1), E});
    };
    auto k1v =
        k1.masked_select(expand_E(k1.size(-1))).view({-1, k1.size(-1)});
    auto k2v =
        k2.masked_select(expand_E(k2.size(-1))).view({-1, k2.size(-1)});

    constexpr double cov_boost = 3.0;
    auto z1v = model->_projector_net->forward_flat(k1v);
    auto z2v = model->_projector_net->forward_flat(k2v);
    return model->loss_obj->forward_terms(z1v, z2v, cov_boost);
  }();

  const double expected_total = expected_terms.total.item<double>();
  const double cov_weighted = expected_terms.cov_weighted.item<double>();
  if (!nearly_equal(cov_weighted, 0.0)) {
    std::cerr << "[phase1][loss] expected cov_weighted to be zero for tiny N, got "
              << cov_weighted << "\n";
    return false;
  }

  const double inv = expected_terms.inv.item<double>();
  const double var = expected_terms.var.item<double>();
  const double cov_raw = expected_terms.cov_raw.item<double>();
  const double corrected_total =
      model->loss_obj->sim_coeff_ * inv +
      model->loss_obj->std_coeff_ * var +
      cov_weighted;
  if (!nearly_equal(corrected_total, expected_total)) {
    std::cerr << "[phase1][loss] corrected total mismatch: expected "
              << expected_total << " got " << corrected_total << "\n";
    return false;
  }

  constexpr double cov_boost = 3.0;
  const double buggy_total =
      model->loss_obj->sim_coeff_ * inv +
      model->loss_obj->std_coeff_ * var +
      (model->loss_obj->cov_coeff_ * cov_boost) * cov_raw;
  if (nearly_equal(buggy_total, expected_total)) {
    std::cerr << "[phase1][loss] safeguard regression: buggy total "
              << buggy_total << " unexpectedly matches safeguarded total "
              << expected_total << "\n";
    return false;
  }

  return true;
}

bool run_encoder_mask_test(const std::string& contract_hash) {
  torch::manual_seed(9001);
  auto model =
      make_cpu_model(contract_hash, /*C=*/2, /*T=*/4, /*D=*/2, /*E=*/5,
                     /*channel_expansion_dim=*/3, /*fused_feature_dim=*/2,
                     /*hidden_dims=*/3, /*depth=*/2, "5-7-5");
  model->eval();

  const float nan = std::numeric_limits<float>::quiet_NaN();
  auto base = torch::tensor(
      {{{{1.0f, 10.0f}, {2.0f, 20.0f}, {3.0f, 30.0f}, {4.0f, 40.0f}},
        {{5.0f, 50.0f}, {6.0f, 60.0f}, {7.0f, 70.0f}, {8.0f, 80.0f}}}},
      torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::tensor(
      {{{true, true, false, false}, {true, false, true, false}}},
      torch::TensorOptions().dtype(torch::kBool));

  auto masked_variant_a = base.clone();
  auto masked_variant_b = base.clone();

  masked_variant_a.index_put_({0, 0, 2, 0}, 111.0f);
  masked_variant_a.index_put_({0, 0, 2, 1}, -222.0f);
  masked_variant_a.index_put_({0, 1, 1, 0}, 333.0f);
  masked_variant_a.index_put_({0, 1, 1, 1}, -444.0f);
  masked_variant_a.index_put_({0, 0, 3, 0}, 555.0f);
  masked_variant_a.index_put_({0, 0, 3, 1}, -666.0f);
  masked_variant_a.index_put_({0, 1, 3, 0}, 777.0f);
  masked_variant_a.index_put_({0, 1, 3, 1}, -888.0f);

  masked_variant_b.index_put_({0, 0, 2, 0}, nan);
  masked_variant_b.index_put_({0, 0, 2, 1}, nan);
  masked_variant_b.index_put_({0, 1, 1, 0}, nan);
  masked_variant_b.index_put_({0, 1, 1, 1}, nan);
  masked_variant_b.index_put_({0, 0, 3, 0}, nan);
  masked_variant_b.index_put_({0, 0, 3, 1}, nan);
  masked_variant_b.index_put_({0, 1, 3, 0}, nan);
  masked_variant_b.index_put_({0, 1, 3, 1}, nan);

  auto out_a = model->encode(masked_variant_a, mask, /*use_swa=*/false,
                             /*detach_to_cpu=*/true);
  auto out_b = model->encode(masked_variant_b, mask, /*use_swa=*/false,
                             /*detach_to_cpu=*/true);

  if (!torch::isfinite(out_a).all().item<bool>() ||
      !torch::isfinite(out_b).all().item<bool>()) {
    std::cerr << "[phase1][mask] outputs are not finite after masked NaN sanitization\n";
    return false;
  }

  const auto time_mask = mask.any(/*dim=*/1);  // [B,T]
  const auto valid_mask =
      time_mask.unsqueeze(-1).expand({out_a.size(0), out_a.size(1), out_a.size(2)});
  const auto invalid_mask = ~valid_mask;

  const auto valid_diff =
      (out_a.masked_select(valid_mask) - out_b.masked_select(valid_mask))
          .abs()
          .max()
          .item<double>();
  if (!nearly_equal(valid_diff, 0.0)) {
    std::cerr << "[phase1][mask] valid outputs diverged by " << valid_diff << "\n";
    return false;
  }

  const auto invalid_abs_max_a = out_a.masked_select(invalid_mask).abs().max().item<double>();
  const auto invalid_abs_max_b = out_b.masked_select(invalid_mask).abs().max().item<double>();
  if (!nearly_equal(invalid_abs_max_a, 0.0) || !nearly_equal(invalid_abs_max_b, 0.0)) {
    std::cerr << "[phase1][mask] invalid outputs are not zero: max_a="
              << invalid_abs_max_a << " max_b=" << invalid_abs_max_b << "\n";
    return false;
  }

  return true;
}

}  // namespace

int main() {
  try {
    const std::string contract_hash = load_contract_hash();

    if (!run_conv_block_internal_mask_test()) return 1;
    if (!run_loss_wiring_test(contract_hash)) return 1;
    if (!run_encoder_mask_test(contract_hash)) return 1;

    std::cout << "[phase1] VICReg stabilization checks passed\n";
    return 0;
  } catch (const c10::Error& e) {
    std::cerr << "[phase1] torch exception: " << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "[phase1] exception: " << e.what() << "\n";
    return 1;
  }
}
