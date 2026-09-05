#include "lwm0b_spread_floor.h"
#include "retained_lwm0a.cpp"

namespace {
constexpr const char *kSpreadProtocol =
    "959d2b7aca6d46311d224a61c3a4b01a94022477a694a013141aa2208353cfd6";

bool spread_seed(int index, const RmcData &data, const LwmWindows &windows,
                 const RmcEvalTargets &targets, const torch::Device &device,
                 const std::map<std::string, std::string> &old,
                 const std::map<std::string, std::string> &attribution) {
  const auto seed = kLwmSeeds[index];
  const std::string root = "lwm0b.seed_" + std::to_string(seed);
  const std::string prior = "lwm0.seed_" + std::to_string(seed);
  const std::string replay = "lwm0a.seed_" + std::to_string(seed);
  OuterAugmentationGeneratorGuard rng(device);
  set_paired_rng(seed, device);
  auto model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  lwm_require(gpv_sha256_file(oca_archive_path(seed)) ==
                      kGpvAnchorSha256[index] &&
                  oca_load_archive(oca_archive_path(seed), model, device, seed,
                                   gpv_anchor_config_hash(device)),
              "certified archive identity");
  model->eval();
  const auto original = oca_snapshot_state(model);
  const auto fit = lwm_cache(model, windows, device);
  set_paired_rng(seed, device);
  auto predictor = LwmPredictor();
  predictor->to(device);
  lwm_warmup(predictor, fit, seed);
  std::vector<torch::Tensor> predictor_state;
  for (const auto &p : predictor->parameters())
    predictor_state.push_back(p.detach().clone());
  set_paired_rng(seed + 42000, device);
  const auto reference =
      lwm0b::fit_spread_floor(fit, lwm0::make_directions(device));
  bool pass =
      lwm0b::check_spread_floor_fixtures(fit, reference, root, std::cout).pass;

  const auto z = lwm_states(model, windows, 0, 64, device);
  const auto prediction = predictor->forward(z.narrow(1, 0, 2));
  const auto pred_loss = (prediction - z.select(1, 2)).square().mean();
  const auto floor_loss = lwm0b::spread_floor(z, reference);
  const auto temporal_mean = z.mean(1, true);
  const std::array<torch::Tensor, 3> losses{
      pred_loss + 0.09 * floor_loss,
      0.09 * lwm0b::spread_floor(0.1 * z, reference),
      0.09 * lwm0b::spread_floor(temporal_mean + 0.1 * (z - temporal_mean),
                                 reference)};
  const std::array<const char *, 3> names{
      "healthy_combined", "active_contraction", "active_temporal"};
  const auto parameters = gpv_trunk_parameters(model);
  const auto prediction_gradient = gpv_gradients(pred_loss, parameters, true);
  const auto prediction_norm = gpv_flatten_gradients(prediction_gradient)
                                   .to(torch::kFloat64)
                                   .norm()
                                   .item<double>();
  close_to(pred_loss.item<double>(),
           std::stod(old.at(prior + ".prediction.loss")), 1e-8, 1e-6,
           "prediction loss");
  close_to(prediction_norm, std::stod(old.at(prior + ".prediction.trunk_norm")),
           1e-8, 1e-6, "prediction gradient");
  const auto floor_gradient = gpv_gradients(floor_loss, parameters, true);
  const double floor_norm = gpv_flatten_gradients(floor_gradient)
                                .to(torch::kFloat64)
                                .norm()
                                .item<double>();
  std::cout << root << ".healthy_floor_loss=" << floor_loss.item<double>()
            << '\n'
            << root << ".healthy_floor_trunk_norm=" << floor_norm << '\n';
  pass = pass && floor_loss.item<double>() <= 1e-8 && floor_norm <= 1e-8;
  std::array<std::vector<torch::Tensor>, 3> gradients;
  for (int d = 0; d < 3; ++d)
    gradients[d] = gpv_gradients(losses[d], parameters, d != 2);

  const auto baseline = gpv_evaluate(model, data, targets, device, false);
  replay_evaluation(baseline, replay + ".baseline", attribution);
  gpv_emit_evaluation(root + ".baseline", baseline);
  const auto structure = lwm_protected(model, data, device);
  auto copy = gpv_clone_model(model, device);
  copy->eval();
  const auto copied = gpv_trunk_parameters(copy);
  std::vector<torch::Tensor> initial;
  for (const auto &p : parameters)
    initial.push_back(p.detach().clone());
  const double parameter_norm =
      gpv_flatten_gradients(initial).to(torch::kFloat64).norm().item<double>();
  for (int d = 0; d < 3; ++d) {
    const std::string key = root + '.' + names[d];
    const double norm = gpv_flatten_gradients(gradients[d])
                            .to(torch::kFloat64)
                            .norm()
                            .item<double>();
    std::cout << key << ".loss=" << losses[d].item<double>() << '\n'
              << key << ".trunk_norm=" << norm << '\n';
    lwm_require(std::isfinite(norm), "finite encoder direction");
    if (norm == 0) {
      pass = false;
      std::cout << key << ".direction_pass=false\n";
      continue;
    }
    {
      torch::NoGradGuard guard;
      for (std::size_t p = 0; p < copied.size(); ++p)
        copied[p].copy_(initial[p] -
                        0.001 * parameter_norm / norm * gradients[d][p]);
    }
    std::vector<torch::Tensor> displacement;
    for (std::size_t p = 0; p < copied.size(); ++p)
      displacement.push_back(copied[p].detach() - initial[p]);
    const double realized = gpv_flatten_gradients(displacement)
                                .to(torch::kFloat64)
                                .norm()
                                .item<double>();
    close_to(realized, 0.001 * parameter_norm, 0, 1e-4, "virtual displacement");
    std::cout << key << ".displacement_norm=" << realized << '\n';
    const auto evaluation = gpv_evaluate(copy, data, targets, device, false);
    if (d == 0)
      replay_evaluation(evaluation, replay + ".prediction", attribution);
    gpv_emit_evaluation(key, evaluation);
    bool direction_pass = lwm_quality_gate(baseline, evaluation, key);
    const auto after = lwm_protected(copy, data, device);
    for (int m = 0; m < 5; ++m) {
      const double change = after[m] / structure[m] - 1;
      lwm_require(std::isfinite(change), "finite structural change");
      direction_pass = direction_pass && change >= -0.02;
      std::cout << key << '.' << kLwmMetrics[m] << ".relative_change=" << change
                << '\n';
    }
    std::cout << key << ".direction_pass=" << direction_pass << '\n';
    pass = pass && direction_pass;
  }
  {
    torch::NoGradGuard guard;
    for (std::size_t p = 0; p < copied.size(); ++p)
      copied[p].copy_(initial[p]);
  }
  lwm_require(oca_state_exact(model, original) &&
                  oca_state_exact(copy, original) && !model->is_training(),
              "encoder/copy state preservation");
  for (const auto &p : model->parameters())
    lwm_require(!p.grad().defined(), "encoder gradient slots");
  for (std::size_t p = 0; p < predictor_state.size(); ++p)
    lwm_require(torch::equal(predictor_state[p], predictor->parameters()[p]) &&
                    !predictor->parameters()[p].grad().defined(),
                "predictor preservation");
  rng.restore();
  std::cout << root << ".state_pass=true\n"
            << root << ".candidate_pass=" << pass << '\n';
  return pass;
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
        {std::string(kLwmDirectory) + "TEMPORAL_SPREAD_FLOOR_PROTOCOL.md",
         kSpreadProtocol},
        {std::string(kLwmDirectory) +
             "quality_wikimyei_mtf_jepa_mae_vicreg_causal_online_target.cpp",
         "16ffb9acdb144f5be7e415ccb5e0ce9f78db7305acf1ab28b791d5adfe6925c1"},
        {std::string(kLwmDirectory) + "quality_wikimyei_mtf_jepa_mae_vicreg_"
                                      "family_direction_attribution.cpp",
         "fa1c141c3660fe67310b7811da9fe53d7aa94b57d5082a438009293227d89dd3"},
        {std::string(kLwmDirectory) + "lwm0_sigreg.h",
         "c0219e6e294dfe5cbb9675266ad6963f6f2800dcf52bab68ad79ac7cd373f09f"},
        {std::string(kGpvModulePath),
         "d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc4b"},
        {".build/tests/representation_lwm0_v1.log",
         "674ff96eade7bcbaa201492f741d3a3098b010f26d2e083abe209734bf754912"},
        {".build/tests/representation_lwm0a_v1.log",
         "7d3b92cde331452b656e53c2af04ca229f5645f02f40dac266f97815967e5625"}};
    for (const auto &[path, hash] : authorities)
      lwm_require(gpv_sha256_file(path) == hash, "authority: " + path);
    std::cout << "schema=lwm0b.spread_floor.v1\nlwm0b.protocol_sha256="
              << kSpreadProtocol << '\n';
    for (const auto *file :
         {"quality_wikimyei_mtf_jepa_mae_vicreg_temporal_spread_floor.cpp",
          "lwm0b_spread_floor.h"})
      std::cout << "lwm0b.source." << file << "="
                << gpv_sha256_file(std::string(kLwmDirectory) + file) << '\n';
    std::cout << "lwm0b.executable_sha256=" << gpv_sha256_file("/proc/self/exe")
              << "\nlwm0b.encoder_optimizer_updates=0\nlwm0b.ema_updates="
                 "0\nlwm0b.confirmation_opened=false\n";
    const auto data = rmc_make_data();
    lwm_require(!data.confirmation.data.defined() &&
                    digest::sha256_hex(gpv_dataset_manifest(data, false)) ==
                        "b52ea0028fbac2f7d59ec515c62e5e1238bced870644f148f2dde8"
                        "be910c2377",
                "frozen data");
    const auto windows = lwm_windows(lwm_raw(0, 256), data.normalization);
    const auto targets = rmc_make_targets(data, false);
    const auto old =
        read_fields(rmc_read_file(".build/tests/representation_lwm0_v1.log"));
    const auto attribution =
        read_fields(rmc_read_file(".build/tests/representation_lwm0a_v1.log"));
    bool pass = true;
    for (int seed = 0; seed < 3; ++seed) {
      const bool seed_pass =
          spread_seed(seed, data, windows, targets, device, old, attribution);
      pass = pass && seed_pass;
    }
    rng.restore();
    std::cout << "lwm0b.decision="
              << (pass ? "passes_local_checks" : "not_admitted")
              << "\nlwm0b.training_admitted=false\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "lwm0b.decision=invalid\nlwm0b.error=" << error.what() << '\n';
    return 2;
  }
}
