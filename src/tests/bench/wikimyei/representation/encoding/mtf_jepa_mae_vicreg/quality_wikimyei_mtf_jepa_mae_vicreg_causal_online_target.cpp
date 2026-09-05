// Test-only temporal objective; reuse frozen model/data/probe helpers.
#define main lwm0_unused_gpv_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp"
#undef main
#include "lwm0_sigreg.h"

namespace {
constexpr const char *kLwmDirectory =
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/";
constexpr const char *kLwmProtocolHash =
    "7d011eb373e9eee2927fdf55347a43883c3092db72df04c332effda8095ae3c3";
constexpr std::array<int64_t, 3> kLwmStarts{0, 37, 74};
constexpr std::array<int64_t, 3> kLwmSeeds{17, 31, 47};
constexpr std::array<const char *, 7> kLwmDirections{"context",
                                                     "target",
                                                     "prediction",
                                                     "raw_regularizer",
                                                     "centered_regularizer",
                                                     "full_raw",
                                                     "full_centered"};
constexpr std::array<const char *, 5> kLwmMetrics{
    "participation_c0", "participation_c1", "participation_c2",
    "order_separation", "channel_contrast"};
using LwmWindows = std::array<Dataset, 3>;
using LwmMetrics = std::array<double, 5>;

void lwm_require(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error("LWM-0: " + message);
}

void lwm_plan(const std::array<int64_t, 3> &starts) {
  for (std::size_t w = 0; w < starts.size(); ++w) {
    lwm_require(starts[w] - 7 >= -7 && starts[w] + 29 <= 103,
                "raw support bounds");
    if (w > 0)
      lwm_require(starts[w - 1] + 29 < starts[w] - 7,
                  "ordered disjoint raw supports");
  }
}

torch::Tensor lwm_raw(int64_t group_begin, int64_t groups) {
  auto raw = torch::empty({groups, 3, 111}, torch::kFloat64);
  auto values = raw.accessor<double, 3>();
  for (int64_t b = 0; b < groups; ++b) {
    const auto factors = factors_for(group_begin + b, false);
    for (int64_t c = 0; c < 3; ++c)
      for (int64_t t = -7; t <= 103; ++t)
        values[b][c][t + 7] = observed_value(factors, group_begin + b, c, t, 0);
  }
  return raw;
}

LwmWindows lwm_windows(const torch::Tensor &raw,
                       const Normalization &normalization,
                       const std::array<int64_t, 3> &starts = kLwmStarts,
                       bool apply_normalization = true) {
  lwm_plan(starts);
  lwm_require(raw.device().is_cpu() && raw.scalar_type() == torch::kFloat64 &&
                  raw.dim() == 3 && raw.size(0) > 0 && raw.size(1) == 3 &&
                  raw.size(2) == 111 && torch::isfinite(raw).all().item<bool>(),
              "raw observation tensor contract");
  const auto values = raw.accessor<double, 3>();
  LwmWindows result;
  for (int64_t w = 0; w < 3; ++w) {
    auto &window = result[w];
    window.data = torch::empty({raw.size(0), 3, 30, 9}, torch::kFloat32);
    window.mask = torch::ones_like(window.data, torch::kBool);
    auto out = window.data.accessor<float, 4>();
    for (int64_t b = 0; b < raw.size(0); ++b) {
      for (int64_t c = 0; c < 3; ++c) {
        for (int64_t h = 0; h < 30; ++h) {
          const auto index = starts[w] + h + 7;
          const double current = values[b][c][index];
          const double previous = values[b][c][index - 1];
          double mean3 = 0, square3 = 0, mean8 = 0, square8 = 0;
          for (int64_t offset = 0; offset < 8; ++offset) {
            const double value = values[b][c][index - offset];
            mean8 += value;
            square8 += value * value;
            if (offset < 3) {
              mean3 += value;
              square3 += value * value;
            }
          }
          mean3 /= 3;
          mean8 /= 8;
          const std::array<double, 9> features{
              current,
              current - previous,
              std::fabs(current),
              current * current,
              mean3,
              std::sqrt(std::max(0.0, square3 / 3 - mean3 * mean3)),
              mean8,
              std::sqrt(std::max(0.0, square8 / 8 - mean8 * mean8)),
              current * values[b][(c + 1) % 3][index]};
          for (int64_t f = 0; f < 9; ++f)
            out[b][c][h][f] = static_cast<float>(features[f]);
        }
      }
    }
    if (apply_normalization)
      normalize(window, normalization);
  }
  return result;
}

torch::Tensor lwm_states(mtf::MtfJepaMaeVicreg &model,
                         const LwmWindows &windows, int64_t begin,
                         int64_t count, const torch::Device &device) {
  std::vector<torch::Tensor> states;
  for (const auto &window : windows) {
    const auto encoded =
        model->encode(window.data.narrow(0, begin, count).to(device),
                      window.mask.narrow(0, begin, count).to(device));
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::structured_cdsb_sparse_v1,
        model->config());
    lwm_require(served.valid_mask.all().item<bool>() &&
                    served.values.sizes() ==
                        torch::IntArrayRef({count, 3, 32}) &&
                    torch::isfinite(served.values).all().item<bool>(),
                "online structured states");
    states.push_back(served.values);
  }
  return torch::stack(states, 1);
}

torch::Tensor lwm_cache(mtf::MtfJepaMaeVicreg &model, const LwmWindows &windows,
                        const torch::Device &device) {
  torch::NoGradGuard guard;
  std::vector<torch::Tensor> chunks;
  for (int64_t start = 0; start < windows[0].data.size(0); start += 64)
    chunks.push_back(lwm_states(
        model, windows, start,
        std::min<int64_t>(64, windows[0].data.size(0) - start), device));
  return torch::cat(chunks, 0).detach();
}

struct LwmPredictorImpl : torch::nn::Module {
  torch::nn::Sequential network{nullptr};
  LwmPredictorImpl() {
    network = register_module(
        "temporal_predictor",
        torch::nn::Sequential(torch::nn::Linear(192, 128), torch::nn::GELU(),
                              torch::nn::Linear(128, 96)));
  }
  torch::Tensor forward(const torch::Tensor &history) {
    lwm_require(history.dim() == 4 && history.size(1) == 2 &&
                    history.size(2) == 3 && history.size(3) == 32,
                "predictor receives exactly two history states");
    return network->forward(history.reshape({history.size(0), 192}))
        .view({history.size(0), 3, 32});
  }
};
TORCH_MODULE(LwmPredictor);

void lwm_warmup(LwmPredictor &predictor, const torch::Tensor &fit,
                int64_t seed) {
  lwm_require(!fit.requires_grad(), "detached predictor-only cache");
  torch::optim::Adam optimizer(predictor->parameters(),
                               torch::optim::AdamOptions(0.001));
  double first = 0, last = 0;
  for (int64_t step = 0; step < 512; ++step) {
    const auto rows =
        torch::randperm(256, torch::kInt64).narrow(0, 0, 64).to(fit.device());
    const auto batch = fit.index_select(0, rows);
    optimizer.zero_grad();
    const auto predicted = predictor->forward(batch.narrow(1, 0, 2));
    const auto loss = (predicted - batch.select(1, 2)).square().mean();
    lwm_require(torch::isfinite(loss).item<bool>(), "finite predictor warm-up");
    loss.backward();
    optimizer.step();
    if (step < 8)
      first += loss.item<double>() / 8;
    if (step >= 504)
      last += loss.item<double>() / 8;
  }
  for (auto &p : predictor->parameters()) {
    p.set_requires_grad(false);
    p.mutable_grad() = torch::Tensor{};
  }
  predictor->eval();
  std::cout << "lwm0.seed_" << seed << ".predictor_updates=512\n"
            << "lwm0.seed_" << seed << ".warmup_first8=" << first << '\n'
            << "lwm0.seed_" << seed << ".warmup_last8=" << last << '\n';
}

double lwm_r2(const torch::Tensor &prediction, const torch::Tensor &target,
              const torch::Tensor &mean) {
  const auto y = target.to(torch::kFloat64);
  const double denominator =
      (y - mean.to(torch::kFloat64)).square().sum().item<double>();
  lwm_require(std::isfinite(denominator) && denominator > 0,
              "R2 reference variance");
  const double result =
      1 - (prediction.to(torch::kFloat64) - y).square().sum().item<double>() /
              denominator;
  lwm_require(std::isfinite(result), "finite held-out R2");
  return result;
}

bool lwm_compatibility(LwmPredictor &predictor, const torch::Tensor &fit,
                       const torch::Tensor &test, const std::string &root) {
  torch::NoGradGuard guard;
  const auto history = test.narrow(1, 0, 2);
  const auto target = test.select(1, 2);
  const auto mean = fit.select(1, 2).mean(0, true);
  const auto prediction = predictor->forward(history);
  const double score = lwm_r2(prediction, target, mean);
  const double permuted =
      lwm_r2(predictor->forward(torch::roll(history, {1}, {0})), target, mean);
  const double swapped =
      lwm_r2(predictor->forward(history.flip({1})), target, mean);
  bool pass =
      score >= 0.05 && score - permuted >= 0.05 && score - swapped >= 0.005;
  std::cout << root << ".heldout_r2=" << score << '\n'
            << root << ".permuted_history_r2=" << permuted << '\n'
            << root << ".swapped_history_r2=" << swapped << '\n';
  for (int c = 0; c < 3; ++c) {
    const double channel =
        lwm_r2(prediction.select(1, c), target.select(1, c), mean.select(1, c));
    pass = pass && channel >= 0;
    std::cout << root << ".heldout_channel_" << c << "_r2=" << channel << '\n';
  }
  std::cout << root << ".predictor_compatible=" << pass << '\n';
  return pass;
}

void lwm_support_audit(mtf::MtfJepaMaeVicreg &model, LwmPredictor &predictor,
                       const torch::Tensor &raw,
                       const Normalization &normalization,
                       const torch::Device &device, const std::string &root) {
  torch::NoGradGuard guard;
  const auto original = raw.narrow(0, 0, 4).clone();
  auto hidden_future = original.clone();
  hidden_future.narrow(2, 74, 37).add_(3);
  auto hidden_history = original.clone();
  hidden_history.narrow(2, 0, 74).sub_(2);
  const auto a = lwm_windows(original, normalization);
  const auto b = lwm_windows(hidden_future, normalization);
  const auto c = lwm_windows(hidden_history, normalization);
  lwm_require(torch::equal(a[0].data, b[0].data) &&
                  torch::equal(a[1].data, b[1].data) &&
                  torch::equal(a[2].data, c[2].data) &&
                  !torch::equal(a[2].data, b[2].data) &&
                  !torch::equal(a[0].data, c[0].data),
              "non-vacuous hidden raw-support interventions");
  const auto za = lwm_states(model, a, 0, 4, device);
  const auto zb = lwm_states(model, b, 0, 4, device);
  const auto zc = lwm_states(model, c, 0, 4, device);
  lwm_require(torch::equal(za.narrow(1, 0, 2), zb.narrow(1, 0, 2)) &&
                  torch::equal(predictor->forward(za.narrow(1, 0, 2)),
                               predictor->forward(zb.narrow(1, 0, 2))) &&
                  torch::equal(za.select(1, 2), zc.select(1, 2)) &&
                  !torch::equal(za.select(1, 2), zb.select(1, 2)),
              "causal online target/prediction legality");
  for (const std::array<int64_t, 3> bad :
       {std::array<int64_t, 3>{0, 37, 73}, {37, 0, 74}, {0, 37, 75}}) {
    bool rejected = false;
    try {
      (void)lwm_windows(original, normalization, bad);
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    lwm_require(rejected, "fail-closed invalid support plan");
  }
  std::cout << root << ".support_interventions_pass=true\n";
}

LwmMetrics lwm_protected(mtf::MtfJepaMaeVicreg &model, const RmcData &data,
                         const torch::Device &device) {
  const auto x =
      rmc_extract_sparse_embeddings(model, data.development, device).by_channel;
  const auto r =
      rmc_extract_sparse_embeddings(model, data.reversed_development, device)
          .by_channel;
  const auto centered = x - x.mean(0, true);
  const double energy = centered.square().mean().item<double>();
  lwm_require(std::isfinite(energy) && energy > 0, "protected centered energy");
  LwmMetrics result;
  for (int c = 0; c < 3; ++c) {
    const auto values = centered.select(1, c);
    const auto covariance =
        values.transpose(0, 1).matmul(values) / (values.size(0) - 1);
    const double denominator = covariance.square().sum().item<double>() * 32;
    lwm_require(std::isfinite(denominator) && denominator > 0,
                "protected participation denominator");
    result[c] = std::pow(covariance.trace().item<double>(), 2) / denominator;
  }
  result[3] = (x - r).square().mean().item<double>() / energy;
  result[4] =
      (centered - centered.mean(1, true)).square().mean().item<double>() /
      energy;
  for (const auto value : result)
    lwm_require(std::isfinite(value) && value > 0, "finite protected metric");
  return result;
}

bool lwm_noncollapse(const torch::Tensor &states, const std::string &root) {
  const auto x = states.to(torch::kCPU, torch::kFloat64);
  const double total = (x - x.mean(0, true)).square().mean().item<double>();
  const double temporal = (x - x.mean(1, true)).square().mean().item<double>();
  lwm_require(std::isfinite(total) && total > 0 && std::isfinite(temporal),
              "temporal energy denominator");
  bool pass = temporal / total >= 1e-4;
  std::cout << root << ".temporal_total_energy_ratio=" << temporal / total
            << '\n';
  for (int t = 0; t < 3; ++t)
    for (int c = 0; c < 3; ++c) {
      const auto g = geometry_for_channel(x.select(1, t).select(1, c));
      validate_geometry_finite(g, "LWM temporal state");
      pass = pass && g.participation_rank_ratio >= 0.10;
      std::cout << root << ".time_" << t << ".channel_" << c
                << ".participation=" << g.participation_rank_ratio << '\n';
    }
  std::cout << root << ".temporal_noncollapse_pass=" << pass << '\n';
  return pass;
}

bool lwm_quality_gate(const RmcEvaluation &baseline, const RmcEvaluation &after,
                      const std::string &root) {
  for (const auto *value : {&baseline, &after})
    for (const double score :
         {value->probe.area, value->order.area, value->shuffled_probe.area,
          value->shuffled_order.area})
      lwm_require(std::isfinite(score), "finite quality guard scores");
  bool pass =
      after.probe.area - baseline.probe.area >= -0.005 &&
      after.order.area - baseline.order.area >= -0.005 &&
      after.shuffled_probe.area - baseline.shuffled_probe.area <= 0.005 &&
      after.shuffled_order.area - baseline.shuffled_order.area <= 0.005;
  const auto old_family = rssm_family_areas(baseline.probe);
  const auto new_family = rssm_family_areas(after.probe);
  for (std::size_t f = 0; f < old_family.size(); ++f) {
    const double delta = new_family[f] - old_family[f];
    lwm_require(std::isfinite(delta), "finite family change");
    pass = pass && delta >= -0.005;
    std::cout << root << ".family_" << kFamilyNames[f] << "_delta=" << delta
              << '\n';
  }
  for (std::size_t c = 0; c < after.geometry.size(); ++c) {
    pass = pass && after.geometry[c].passed;
    std::cout << root << ".channel_" << c
              << ".geometry_pass=" << after.geometry[c].passed << '\n';
  }
  std::cout << root << ".quality_guard_pass=" << pass << '\n';
  return pass;
}

struct LwmSeedResult {
  bool compatible{false};
  std::array<bool, 2> candidate{false, false};
};

LwmSeedResult
lwm_audit_seed(int index, const RmcData &data, const LwmWindows &fit_windows,
               const LwmWindows &test_windows, const torch::Tensor &fit_raw,
               const RmcEvalTargets &targets, const torch::Device &device) {
  const int64_t seed = kLwmSeeds[index];
  const std::string root = "lwm0.seed_" + std::to_string(seed);
  OuterAugmentationGeneratorGuard rng_guard(device);
  const auto rng_before = current_generator_state_snapshot(device);
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  const auto archive = oca_archive_path(seed);
  lwm_require(gpv_sha256_file(archive) == kGpvAnchorSha256[index] &&
                  oca_load_archive(archive, model, device, seed,
                                   gpv_anchor_config_hash(device)),
              "authenticated FSPA-4 archive");
  model->eval();
  const auto original_state = oca_snapshot_state(model);
  const auto fit = lwm_cache(model, fit_windows, device);
  const auto test = lwm_cache(model, test_windows, device);
  set_paired_rng(seed, device);
  auto predictor = LwmPredictor();
  predictor->to(device);
  lwm_warmup(predictor, fit, seed);
  std::vector<torch::Tensor> predictor_state;
  for (const auto &p : predictor->parameters())
    predictor_state.push_back(p.detach().clone());
  LwmSeedResult result;
  result.compatible = lwm_compatibility(predictor, fit, test, root);
  const bool noncollapse = lwm_noncollapse(test, root);
  lwm_support_audit(model, predictor, fit_raw, data.normalization, device,
                    root);

  set_paired_rng(seed + 42000, device);
  const auto directions = lwm0::make_directions(device);
  const auto fixtures = lwm0::check_sigreg_fixtures(device, directions);
  lwm_require(fixtures.pass, "SIGReg analytic/control fixtures");
  std::cout << root << ".centering_invariance_error="
            << fixtures.centering_invariance_error << '\n'
            << root
            << ".batch_permutation_error=" << fixtures.batch_permutation_error
            << '\n'
            << root << ".channel_permutation_error="
            << fixtures.channel_permutation_error << '\n';
  std::cout << root << ".sigreg_fixtures_pass=true\n"
            << root << ".collapse_penalty=" << fixtures.collapsed_loss << '\n'
            << root << ".gaussian_penalty=" << fixtures.gaussian_loss << '\n'
            << root << ".static_raw_penalty=" << fixtures.static_raw_loss
            << '\n'
            << root
            << ".static_centered_penalty=" << fixtures.static_centered_loss
            << '\n'
            << root
            << ".exact_collapse_gradient=" << fixtures.collapsed_gradient_norm
            << '\n';
  const auto z = lwm_states(model, fit_windows, 0, 64, device);
  const auto prediction = predictor->forward(z.narrow(1, 0, 2));
  const auto target = z.select(1, 2);
  const auto context_loss = (prediction - target.detach()).square().mean();
  const auto target_loss = (prediction.detach() - target).square().mean();
  const auto prediction_loss = (prediction - target).square().mean();
  lwm_require(torch::equal(context_loss, prediction_loss) &&
                  torch::equal(target_loss, prediction_loss),
              "detached/full target forward parity");
  const auto raw = lwm0::kSigregWeight * lwm0::sigreg(z, directions, false);
  const auto centered = lwm0::kSigregWeight * lwm0::sigreg(z, directions, true);
  const std::array<torch::Tensor, 7> losses{
      context_loss, target_loss,           prediction_loss,           raw,
      centered,     prediction_loss + raw, prediction_loss + centered};
  const auto parameters = gpv_trunk_parameters(model);
  std::array<std::vector<torch::Tensor>, 7> gradients;
  std::array<torch::Tensor, 7> flat;
  std::array<double, 7> norms{};
  for (std::size_t d = 0; d < gradients.size(); ++d) {
    gradients[d] =
        gpv_gradients(losses[d], parameters, d + 1 < gradients.size());
    flat[d] = gpv_flatten_gradients(gradients[d]).to(torch::kFloat64);
    norms[d] = flat[d].norm().item<double>();
    lwm_require(std::isfinite(norms[d]) && norms[d] > 1e-10 &&
                    torch::isfinite(losses[d]).item<bool>(),
                "finite connected objective gradient");
    std::cout << root << '.' << kLwmDirections[d]
              << ".loss=" << losses[d].item<double>() << '\n'
              << root << '.' << kLwmDirections[d] << ".trunk_norm=" << norms[d]
              << '\n';
  }
  for (const auto triple :
       {std::array<int, 3>{2, 0, 1}, {5, 2, 3}, {6, 2, 4}}) {
    const auto residual = flat[triple[0]] - flat[triple[1]] - flat[triple[2]];
    const double absolute = residual.abs().max().item<double>();
    const double relative = residual.norm().item<double>() / norms[triple[0]];
    lwm_require(absolute <= 5e-5 && relative <= 1e-4,
                "branch/objective gradient decomposition");
    std::cout << root << '.' << kLwmDirections[triple[0]]
              << ".decomposition_absolute=" << absolute << '\n'
              << root << '.' << kLwmDirections[triple[0]]
              << ".decomposition_relative=" << relative << '\n';
  }
  const double coherent = norms[2] / (norms[0] + norms[1]);
  lwm_require(std::isfinite(coherent) && coherent >= 0.001,
              "nonvanishing combined target direction");
  std::cout << root << ".forward_value_parity=true\n"
            << root << ".full_branch_norm_ratio=" << coherent << '\n';
  for (int d = 1; d < 7; ++d) {
    const double cosine =
        flat[0].dot(flat[d]).item<double>() / (norms[0] * norms[d]);
    lwm_require(std::isfinite(cosine), "finite branch cosine");
    std::cout << root << ".context_cosine_" << kLwmDirections[d] << '='
              << cosine << '\n';
  }

  const auto baseline = lwm_protected(model, data, device);
  const auto quality = gpv_evaluate(model, data, targets, device, false);
  gpv_emit_evaluation(root + ".quality_baseline", quality);
  for (const auto &g : quality.geometry)
    lwm_require(g.passed, "retained anchor geometry");
  auto copy = gpv_clone_model(model, device);
  copy->eval();
  const auto copied = gpv_trunk_parameters(copy);
  std::vector<torch::Tensor> initial;
  for (const auto &p : parameters)
    initial.push_back(p.detach().clone());
  const double parameter_norm =
      gpv_flatten_gradients(initial).to(torch::kFloat64).norm().item<double>();
  lwm_require(std::isfinite(parameter_norm) && parameter_norm > 0,
              "trunk parameter norm");
  result.candidate.fill(noncollapse);
  for (const double radius : {0.0005, 0.001}) {
    for (std::size_t d = 0; d < gradients.size(); ++d) {
      {
        torch::NoGradGuard guard;
        for (std::size_t p = 0; p < copied.size(); ++p)
          copied[p].copy_(initial[p] -
                          radius * parameter_norm / norms[d] * gradients[d][p]);
      }
      std::vector<torch::Tensor> displacement;
      for (std::size_t p = 0; p < copied.size(); ++p)
        displacement.push_back(copied[p].detach() - initial[p]);
      const double realized = gpv_flatten_gradients(displacement)
                                  .to(torch::kFloat64)
                                  .norm()
                                  .item<double>();
      lwm_require(std::isfinite(realized) &&
                      std::abs(realized / (radius * parameter_norm) - 1) <=
                          1e-4,
                  "equal-norm virtual displacement");
      const auto after = lwm_protected(copy, data, device);
      std::cout << root << ".virtual.radius_" << radius << '.'
                << kLwmDirections[d] << ".displacement_norm=" << realized
                << '\n';
      bool protected_pass = true;
      for (std::size_t m = 0; m < baseline.size(); ++m) {
        const double change = after[m] / baseline[m] - 1;
        lwm_require(std::isfinite(change), "finite protected metric change");
        protected_pass = protected_pass && change >= -0.02;
        std::cout << root << ".virtual.radius_" << radius << '.'
                  << kLwmDirections[d] << '.' << kLwmMetrics[m]
                  << ".baseline=" << baseline[m] << '\n'
                  << root << ".virtual.radius_" << radius << '.'
                  << kLwmDirections[d] << '.' << kLwmMetrics[m]
                  << ".value=" << after[m] << '\n'
                  << root << ".virtual.radius_" << radius << '.'
                  << kLwmDirections[d] << '.' << kLwmMetrics[m]
                  << ".relative_change=" << change << '\n';
      }
      if (d == 3 || d == 5)
        result.candidate[0] = result.candidate[0] && protected_pass;
      if (d == 4 || d == 6)
        result.candidate[1] = result.candidate[1] && protected_pass;
      if (radius == 0.001 && d >= 5) {
        const auto evaluated = gpv_evaluate(copy, data, targets, device, false);
        const auto key = root + ".quality_" + kLwmDirections[d];
        gpv_emit_evaluation(key, evaluated);
        result.candidate[d - 5] = lwm_quality_gate(quality, evaluated, key) &&
                                  result.candidate[d - 5];
      }
    }
  }
  {
    torch::NoGradGuard guard;
    for (std::size_t p = 0; p < copied.size(); ++p)
      copied[p].copy_(initial[p]);
  }
  lwm_require(oca_state_exact(copy, original_state) &&
                  oca_state_exact(model, original_state) &&
                  !model->is_training(),
              "encoder/EMA state and virtual nontrunk preservation");
  for (const auto &p : model->parameters())
    lwm_require(!p.grad().defined(), "encoder gradient slots preserved");
  for (std::size_t p = 0; p < predictor_state.size(); ++p)
    lwm_require(torch::equal(predictor_state[p], predictor->parameters()[p]) &&
                    !predictor->parameters()[p].grad().defined(),
                "fixed shared predictor state");
  rng_guard.restore();
  lwm_require(generator_state_snapshot_equal(
                  rng_before, current_generator_state_snapshot(device)),
              "seed RNG restored");
  std::cout << root << ".state_gradients_rng_unchanged=true\n"
            << root << ".raw_candidate_pass=" << result.candidate[0] << '\n'
            << root << ".centered_candidate_pass=" << result.candidate[1]
            << '\n';
  return result;
}
} // namespace

int main() {
  try {
    std::cout << std::setprecision(17) << std::boolalpha << std::unitbuf;
    lwm_require(torch::cuda::is_available(), "canonical CUDA required");
    torch::set_num_threads(1);
    const torch::Device device(torch::kCUDA, 0);
    OuterAugmentationGeneratorGuard rng_guard(device);
    const auto process_rng_before = current_generator_state_snapshot(device);
    const auto protocol = std::string(kLwmDirectory) +
                          "CAUSAL_ONLINE_TARGET_COMPATIBILITY_PROTOCOL.md";
    lwm_require(gpv_sha256_file(protocol) == kLwmProtocolHash,
                "sealed protocol");
    lwm_require(
        gpv_sha256_file(std::filesystem::path(kGpvModulePath)) ==
                "d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc"
                "4b" &&
            gpv_sha256_file(std::string(kLwmDirectory) +
                            "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.md") ==
                "e4de60edd42dc87013a9bbcf5cdf03960a720d2f43fef4572de09333fbe7cf"
                "75",
        "current encoder and seam custody");
    std::cout
        << "schema=lwm0.causal_online_target.v1\nlwm0.protocol_sha256="
        << kLwmProtocolHash << "\nlwm0.source_sha256="
        << gpv_sha256_file(
               std::string(kLwmDirectory) +
               "quality_wikimyei_mtf_jepa_mae_vicreg_causal_online_target.cpp")
        << "\nlwm0.sigreg_sha256="
        << gpv_sha256_file(std::string(kLwmDirectory) + "lwm0_sigreg.h")
        << "\nlwm0.executable_sha256=" << gpv_sha256_file("/proc/self/exe")
        << "\nlwm0.encoder_optimizer_updates=0\nlwm0.ema_updates=0\nlwm0."
           "augmentation_calls=0\n"
           "lwm0.legacy_objectives_called=0\nlwm0.confirmation_opened=false\n";
    const auto data = rmc_make_data();
    lwm_require(!data.confirmation.data.defined() &&
                    digest::sha256_hex(gpv_dataset_manifest(data, false)) ==
                        "b52ea0028fbac2f7d59ec515c62e5e1238bced870644f148f2dde8"
                        "be910c2377",
                "frozen data and unopened confirmation");
    const auto fit_raw = lwm_raw(0, 256);
    const auto test_raw = lwm_raw(6000000, 128);
    const auto unnormalized =
        lwm_windows(fit_raw, data.normalization, kLwmStarts, false);
    const auto original_features = generate_dataset(0, 256);
    lwm_require(torch::equal(unnormalized[0].data, original_features.data),
                "pre-normalization feature generator exact parity");
    const auto fit_windows = lwm_windows(fit_raw, data.normalization);
    const auto test_windows = lwm_windows(test_raw, data.normalization);
    lwm_require(torch::equal(fit_windows[0].data, data.ssl.data),
                "original feature generator exact parity");
    std::cout << "lwm0.original_feature_parity=true\nlwm0.fit_raw_hash="
              << oca_hex_u64(hash_tensor_stable_bytes(fit_raw))
              << "\nlwm0.heldout_raw_hash="
              << oca_hex_u64(hash_tensor_stable_bytes(test_raw)) << '\n';
    const auto targets = rmc_make_targets(data, false);
    bool compatible = true;
    std::array<bool, 2> candidate{true, true};
    for (int seed = 0; seed < 3; ++seed) {
      const auto result = lwm_audit_seed(seed, data, fit_windows, test_windows,
                                         fit_raw, targets, device);
      compatible = compatible && result.compatible;
      for (int c = 0; c < 2; ++c)
        candidate[c] = candidate[c] && result.candidate[c];
    }
    const char *decision = !compatible    ? "not_admitted_predictor"
                           : candidate[1] ? "admitted_temporal_centered"
                           : candidate[0] ? "admitted_raw"
                                          : "not_admitted_regularizer";
    rng_guard.restore();
    lwm_require(
        generator_state_snapshot_equal(
            process_rng_before, current_generator_state_snapshot(device)),
        "process RNG restored exactly");
    std::cout << "lwm0.predictor_compatible_3of3=" << compatible
              << "\nlwm0.raw_candidate_safe_3of3=" << candidate[0]
              << "\nlwm0.centered_candidate_safe_3of3=" << candidate[1]
              << "\nlwm0.mechanics_custody_pass=true\nlwm0.process_rng_"
                 "restored=true\nlwm0.decision="
              << decision << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "lwm0.decision=invalid\nlwm0.error=" << error.what() << '\n';
    return 2;
  }
}
