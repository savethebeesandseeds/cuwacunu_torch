#include "retained_lwm0.cpp"

namespace {
constexpr const char *kLwm0aProtocol =
    "ff250dfaacdddc120d099d0ed671ea231e39e7a81a18503789ff381eea647d98";
constexpr std::array<const char *, 5> kNames{"prediction", "raw_regularizer",
                                             "centered_regularizer", "full_raw",
                                             "full_centered"};

std::map<std::string, std::string> read_fields(const std::string &bytes) {
  std::map<std::string, std::string> fields;
  std::istringstream input(bytes);
  for (std::string line; std::getline(input, line);) {
    const auto p = line.find('=');
    if (p != std::string::npos)
      fields.emplace(line.substr(0, p), line.substr(p + 1));
  }
  return fields;
}

void close_to(double observed, double expected, double absolute,
              double relative, const std::string &name) {
  lwm_require(std::isfinite(observed) && std::isfinite(expected) &&
                  std::abs(observed - expected) <=
                      absolute + relative * std::abs(expected),
              "retained replay mismatch: " + name);
}

std::map<std::string, double> evaluation_fields(const RmcEvaluation &e) {
  std::map<std::string, double> fields{
      {"aulc", e.probe.area},
      {"order_aulc", e.order.area},
      {"continuous_shuffle_aulc", e.shuffled_probe.area},
      {"order_shuffle_aulc", e.shuffled_order.area}};
  const auto families = rssm_family_areas(e.probe);
  for (std::size_t f = 0; f < families.size(); ++f)
    fields[std::string("family_") + kFamilyNames[f]] = families[f];
  for (int c = 0; c < 3; ++c) {
    const auto prefix = "channel_" + std::to_string(c) + ".geometry.";
    fields[prefix + "effective"] = e.geometry[c].effective_rank_ratio;
    fields[prefix + "participation"] = e.geometry[c].participation_rank_ratio;
    fields[prefix + "top"] = e.geometry[c].top_eigenvalue_share;
    fields[prefix + "active"] = e.geometry[c].active_dimension_fraction;
  }
  return fields;
}

void replay_evaluation(const RmcEvaluation &e, const std::string &old_root,
                       const std::map<std::string, std::string> &old) {
  for (const auto &[key, value] : evaluation_fields(e))
    close_to(value, std::stod(old.at(old_root + '.' + key)), 1e-8, 0,
             old_root + '.' + key);
}

void run_seed(int index, const RmcData &data, const LwmWindows &windows,
              const RmcEvalTargets &targets, const torch::Device &device,
              const std::map<std::string, std::string> &old) {
  const auto seed = kLwmSeeds[index];
  const auto root = "lwm0a.seed_" + std::to_string(seed);
  const auto previous = "lwm0.seed_" + std::to_string(seed);
  OuterAugmentationGeneratorGuard rng(device);
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  lwm_require(gpv_sha256_file(oca_archive_path(seed)) ==
                      kGpvAnchorSha256[index] &&
                  oca_load_archive(oca_archive_path(seed), model, device, seed,
                                   gpv_anchor_config_hash(device)),
              "FSPA archive identity");
  model->eval();
  const auto reference = oca_snapshot_state(model);
  const auto fit = lwm_cache(model, windows, device);
  set_paired_rng(seed, device);
  auto predictor = LwmPredictor();
  predictor->to(device);
  lwm_warmup(predictor, fit, seed);
  std::vector<torch::Tensor> predictor_state;
  for (const auto &p : predictor->parameters())
    predictor_state.push_back(p.detach().clone());
  set_paired_rng(seed + 42000, device);
  const auto directions = lwm0::make_directions(device);
  const auto z = lwm_states(model, windows, 0, 64, device);
  const auto prediction = predictor->forward(z.narrow(1, 0, 2));
  const auto pred_loss = (prediction - z.select(1, 2)).square().mean();
  const auto raw = lwm0::kSigregWeight * lwm0::sigreg(z, directions, false);
  const auto centered = lwm0::kSigregWeight * lwm0::sigreg(z, directions, true);
  const std::array<torch::Tensor, 5> losses{
      pred_loss, raw, centered, pred_loss + raw, pred_loss + centered};
  const auto parameters = gpv_trunk_parameters(model);
  std::array<std::vector<torch::Tensor>, 5> gradients;
  std::array<double, 5> norms;
  for (int d = 0; d < 5; ++d) {
    gradients[d] = gpv_gradients(losses[d], parameters, d != 4);
    norms[d] = gpv_flatten_gradients(gradients[d])
                   .to(torch::kFloat64)
                   .norm()
                   .item<double>();
    const auto name = previous + '.' + kNames[d];
    close_to(losses[d].item<double>(), std::stod(old.at(name + ".loss")), 1e-8,
             1e-6, name + ".loss");
    close_to(norms[d], std::stod(old.at(name + ".trunk_norm")), 1e-8, 1e-6,
             name + ".norm");
  }
  const auto baseline = gpv_evaluate(model, data, targets, device, false);
  replay_evaluation(baseline, previous + ".quality_baseline", old);
  gpv_emit_evaluation(root + ".baseline", baseline);
  auto copy = gpv_clone_model(model, device);
  copy->eval();
  const auto copied = gpv_trunk_parameters(copy);
  std::vector<torch::Tensor> initial;
  for (const auto &p : parameters)
    initial.push_back(p.detach().clone());
  const double parameter_norm =
      gpv_flatten_gradients(initial).to(torch::kFloat64).norm().item<double>();
  std::array<RmcEvaluation, 5> evaluations;
  for (int d = 0; d < 5; ++d) {
    {
      torch::NoGradGuard guard;
      for (std::size_t p = 0; p < copied.size(); ++p)
        copied[p].copy_(initial[p] -
                        0.001 * parameter_norm / norms[d] * gradients[d][p]);
    }
    std::vector<torch::Tensor> displacement;
    for (std::size_t p = 0; p < copied.size(); ++p)
      displacement.push_back(copied[p].detach() - initial[p]);
    close_to(gpv_flatten_gradients(displacement)
                 .to(torch::kFloat64)
                 .norm()
                 .item<double>(),
             0.001 * parameter_norm, 0, 1e-4, "virtual displacement");
    const auto protected_values = lwm_protected(copy, data, device);
    for (int m = 0; m < 5; ++m) {
      const auto key = previous + ".virtual.radius_0.001." + kNames[d] + '.' +
                       kLwmMetrics[m] + ".value";
      close_to(protected_values[m], std::stod(old.at(key)), 1e-8, 0, key);
    }
    evaluations[d] = gpv_evaluate(copy, data, targets, device, false);
    if (d >= 3)
      replay_evaluation(evaluations[d], previous + ".quality_" + kNames[d],
                        old);
    const auto key = root + '.' + kNames[d];
    gpv_emit_evaluation(key, evaluations[d]);
    (void)lwm_quality_gate(baseline, evaluations[d], key);
  }
  const auto base_family = rssm_family_areas(baseline.probe);
  const auto prediction_family = rssm_family_areas(evaluations[0].probe);
  for (int candidate = 0; candidate < 2; ++candidate) {
    const auto combined_family =
        rssm_family_areas(evaluations[3 + candidate].probe);
    const auto regularizer_family =
        rssm_family_areas(evaluations[1 + candidate].probe);
    for (int family = 0; family < 4; ++family) {
      if (combined_family[family] - base_family[family] >= -0.005)
        continue;
      const bool p = prediction_family[family] - base_family[family] < -0.005;
      const bool r = regularizer_family[family] - base_family[family] < -0.005;
      const char *classification =
          p ? (r ? "shared_direction_failure" : "prediction_direction")
            : (r ? "regularizer_direction" : "combination_only_threshold");
      std::cout << root << ".attribution." << kNames[3 + candidate] << '.'
                << kFamilyNames[family] << '=' << classification << '\n';
    }
  }
  {
    torch::NoGradGuard guard;
    for (std::size_t p = 0; p < copied.size(); ++p)
      copied[p].copy_(initial[p]);
  }
  lwm_require(oca_state_exact(model, reference) &&
                  oca_state_exact(copy, reference) && !model->is_training(),
              "reference and virtual nontrunk state preservation");
  for (const auto &p : model->parameters())
    lwm_require(!p.grad().defined(), "encoder gradient slots");
  for (std::size_t p = 0; p < predictor_state.size(); ++p)
    lwm_require(torch::equal(predictor_state[p], predictor->parameters()[p]) &&
                    !predictor->parameters()[p].grad().defined(),
                "predictor state preservation");
  rng.restore();
  std::cout << root << ".replay_state_pass=true\n";
}
} // namespace

int main() {
  try {
    std::cout << std::setprecision(17) << std::boolalpha << std::unitbuf;
    torch::set_num_threads(1);
    lwm_require(torch::cuda::is_available(), "CUDA environment");
    const torch::Device device(torch::kCUDA, 0);
    OuterAugmentationGeneratorGuard rng(device);
    const std::map<std::string, std::string> authorities{
        {std::string(kLwmDirectory) +
             "TEMPORAL_FAMILY_DIRECTION_ATTRIBUTION_PROTOCOL.md",
         kLwm0aProtocol},
        {std::string(kLwmDirectory) +
             "quality_wikimyei_mtf_jepa_mae_vicreg_causal_online_target.cpp",
         "16ffb9acdb144f5be7e415ccb5e0ce9f78db7305acf1ab28b791d5adfe6925c1"},
        {std::string(kLwmDirectory) + "lwm0_sigreg.h",
         "c0219e6e294dfe5cbb9675266ad6963f6f2800dcf52bab68ad79ac7cd373f09f"},
        {std::string(kLwmDirectory) +
             "CAUSAL_ONLINE_TARGET_COMPATIBILITY_PROTOCOL.md",
         kLwmProtocolHash},
        {std::string(kGpvModulePath),
         "d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc4b"},
        {std::string(kLwmDirectory) +
             "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.md",
         "e4de60edd42dc87013a9bbcf5cdf03960a720d2f43fef4572de09333fbe7cf75"},
        {".build/tests/representation_lwm0_v1.log",
         "674ff96eade7bcbaa201492f741d3a3098b010f26d2e083abe209734bf754912"},
        {".build/tests/"
         "quality_wikimyei_mtf_jepa_mae_vicreg_causal_online_target",
         "63afe55bc86d9c231ed7fbf299d5b86566c8b0928069ccddab9f9099c2e30090"}};
    for (const auto &[path, hash] : authorities)
      lwm_require(gpv_sha256_file(path) == hash, "authority: " + path);
    std::cout << "schema=lwm0a.family_direction.v1\nlwm0a.protocol_sha256="
              << kLwm0aProtocol << "\nlwm0a.source_sha256="
              << gpv_sha256_file(std::string(kLwmDirectory) +
                                 "quality_wikimyei_mtf_jepa_mae_vicreg_family_"
                                 "direction_attribution.cpp")
              << "\nlwm0a.executable_sha256="
              << gpv_sha256_file("/proc/self/exe")
              << "\nlwm0a.encoder_optimizer_updates=0\nlwm0a.ema_updates="
                 "0\nlwm0a.confirmation_opened=false\n";
    const auto old =
        read_fields(rmc_read_file(".build/tests/representation_lwm0_v1.log"));
    const auto data = rmc_make_data();
    lwm_require(!data.confirmation.data.defined() &&
                    digest::sha256_hex(gpv_dataset_manifest(data, false)) ==
                        "b52ea0028fbac2f7d59ec515c62e5e1238bced870644f148f2dde8"
                        "be910c2377",
                "frozen data");
    const auto windows = lwm_windows(lwm_raw(0, 256), data.normalization);
    const auto targets = rmc_make_targets(data, false);
    for (int seed = 0; seed < 3; ++seed)
      run_seed(seed, data, windows, targets, device, old);
    rng.restore();
    std::cout << "lwm0a.decision=attribution_complete\nlwm0a.training_admitted="
                 "false\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "lwm0a.decision=invalid\nlwm0a.error=" << error.what() << '\n';
    return 2;
  }
}
