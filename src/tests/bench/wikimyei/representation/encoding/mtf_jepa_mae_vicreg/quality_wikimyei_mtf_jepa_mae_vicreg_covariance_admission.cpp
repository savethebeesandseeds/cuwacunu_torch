// Reuse the frozen data/model/math helpers, never the legacy run/cache loader.
#define main lsa0_unused_gpv_main
#include "quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp"
#undef main

namespace {
constexpr const char *kLsaDirectory =
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/";
constexpr const char *kLsaProtocolHash =
    "37ab8774e3c4fa62c9d2886319237d7e0601bf6086a0c622ca621d54dfed4f31";
constexpr std::array<int64_t, 3> kLsaSeeds{17, 31, 47};
constexpr std::array<const char *, 3> kLsaEndpointHashes{
    "8793b3efb2e95d3b4b30838158c80a4cc00a42c9750817df3dd9294e47a8d582",
    "7976cbd0b70aa1e1db7febcf41955b28c595671e404dd5b1fa8eef8e32f6f1d4",
    "a875b547b125affe501f87b08610f66dac44194b9539468beb6132147a4147f1"};
constexpr std::array<const char *, 4> kLsaComponents{"invariance", "variance",
                                                     "covariance", "full"};
constexpr std::array<const char *, 5> kLsaMetrics{
    "participation_c0", "participation_c1", "participation_c2",
    "order_separation", "channel_contrast"};
using LsaMetrics = std::array<double, 5>;
using LsaGradients = std::vector<torch::Tensor>;

void lsa_require(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error("LSA-0: " + message);
}

std::map<std::string, std::string> lsa_authority(const RmcData &data) {
  const auto path = ".build/tests/representation_gpv1_v2_authoritative.log";
  lsa_require(
      gpv_sha256_file(path) ==
          "eb0b8a5821a9aa613ae60508574d10abc18dd37155c2bc68b0ead1d8a68eef27",
      "GPV authority hash");
  std::map<std::string, std::string> fields;
  std::istringstream lines(rmc_read_file(path));
  for (std::string line; std::getline(lines, line);) {
    const auto equals = line.find('=');
    if (equals != std::string::npos)
      fields.emplace(line.substr(0, equals), line.substr(equals + 1));
  }
  lsa_require(digest::sha256_hex(gpv_dataset_manifest(data, false)) ==
                      fields.at("gpv1.binding.dataset_sha256") &&
                  digest::sha256_hex(gpv_dataset_manifest(data, true)) ==
                      fields.at("gpv1.binding.splits_sha256"),
              "frozen dataset/split identity");
  lsa_require(gpv_sha256_file(std::filesystem::path(kGpvHarnessPath)) ==
                  fields.at("gpv1.binding.harness_sha256"),
              "historical GPV source changed");
  const std::string current_module =
      "d52356ba3f1eb8205402e801950f6ca7b3fb0060d5344201434b91f9b2c7cc4b";
  const std::string seam_hash =
      "e4de60edd42dc87013a9bbcf5cdf03960a720d2f43fef4572de09333fbe7cf75";
  const auto seam =
      std::string(kLsaDirectory) + "VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT";
  lsa_require(gpv_sha256_file(std::filesystem::path(kGpvModulePath)) ==
                      current_module &&
                  gpv_sha256_file(seam + ".md") == seam_hash &&
                  rmc_read_file(seam + ".sha256").substr(0, 64) == seam_hash,
              "authenticated post-GPV view-pairing seam");
  std::cout << "lsa0.historical_module_sha256="
            << fields.at("gpv1.binding.module_sha256")
            << "\nlsa0.current_module_sha256=" << current_module
            << "\nlsa0.seam_audit_sha256=" << seam_hash << '\n';
  return fields;
}

mtf::MtfJepaMaeVicreg
lsa_load(int seed_index, bool endpoint, const RmcData &data,
         const torch::Device &device,
         const std::map<std::string, std::string> &authority) {
  const auto seed = kLsaSeeds.at(seed_index);
  auto model = mtf::MtfJepaMaeVicreg(gpv_config(device));
  const auto path = endpoint ? gpv_cell_path(seed, 2) : oca_archive_path(seed);
  const std::string expected =
      endpoint ? kLsaEndpointHashes.at(seed_index)
               : std::string(kGpvAnchorSha256.at(seed_index));
  lsa_require(gpv_sha256_file(path) == expected, "retained archive hash");
  if (!endpoint) {
    lsa_require(oca_load_archive(path, model, device, seed,
                                 gpv_anchor_config_hash(device)),
                "FSPA archive metadata");
  } else {
    lsa_require(rmc_read_file(gpv_cell_marker_path(path)).substr(0, 64) ==
                    expected,
                "GPV completion marker");
    torch::serialize::InputArchive root;
    root.load_from(path.string(), device);
    const auto str = [&](const char *key) {
      return oca_tensor_string(gpv_read_tensor(root, key));
    };
    const auto integer = [&](const char *key) {
      auto value = gpv_read_tensor(root, key);
      gpv_require_archive_tensor(value, torch::kInt64, 1, 1, key);
      return value.item<int64_t>();
    };
    for (const char *field :
         {"harness_sha256", "executable_sha256", "module_sha256",
          "protocol_sha256", "scientific_manifest_sha256", "dataset_sha256",
          "splits_sha256", "bootstrap_sha256"}) {
      lsa_require(str((std::string("meta/") + field).c_str()) ==
                      authority.at(std::string("gpv1.binding.") + field),
                  std::string("historical binding ") + field);
    }
    const auto hashes = oca_seed_cache_ssl_hashes(data.ssl);
    lsa_require(
        str("meta/schema") == kGpvCellCacheSchema &&
            integer("meta/complete") == 1 && integer("meta/seed") == seed &&
            integer("meta/mask") == 2 && integer("meta/steps") == 512 &&
            str("meta/config_manifest") == gpv_config_manifest(device) &&
            str("meta/factor_manifest") == gpv_factor_manifest(2) &&
            str("meta/anchor_sha256") == kGpvAnchorSha256.at(seed_index) &&
            str("meta/ssl_data_hash") == hashes[0] &&
            str("meta/ssl_mask_hash") == hashes[1] &&
            str("meta/ssl_target_hash") == hashes[2],
        "GPV endpoint metadata");
    torch::serialize::InputArchive weights;
    root.read("model", weights);
    model->load(weights);
  }
  model->eval();
  lsa_require(gpv_model_finite(model), "finite retained model");
  std::cout << "lsa0.seed_" << seed << (endpoint ? ".endpoint" : ".start")
            << ".archive_sha256=" << expected << '\n';
  return model;
}

LsaMetrics lsa_metrics(mtf::MtfJepaMaeVicreg &model, const RmcData &data,
                       const torch::Device &device) {
  // RMC extraction authenticates shape, validity, and the sparse readout path.
  auto clean = data.development;
  auto reversed = data.reversed_development;
  clean.data = clean.data.narrow(0, 0, 256);
  clean.mask = clean.mask.narrow(0, 0, 256);
  reversed.data = reversed.data.narrow(0, 0, 256);
  reversed.mask = reversed.mask.narrow(0, 0, 256);
  const auto x = rmc_extract_sparse_embeddings(model, clean, device).by_channel;
  const auto r =
      rmc_extract_sparse_embeddings(model, reversed, device).by_channel;
  const auto centered = x - x.mean(0, true);
  const double energy = centered.square().mean().item<double>();
  lsa_require(std::isfinite(energy) && energy > 0, "positive centered energy");
  LsaMetrics result{};
  for (int channel = 0; channel < 3; ++channel) {
    const auto c = centered.select(1, channel);
    const auto covariance = c.transpose(0, 1).matmul(c) / (c.size(0) - 1);
    const double denominator = covariance.square().sum().item<double>() * 32;
    lsa_require(denominator > 0, "participation denominator");
    result[channel] =
        std::pow(covariance.trace().item<double>(), 2) / denominator;
  }
  result[3] = (x - r).square().mean().item<double>() / energy;
  result[4] =
      (centered - centered.mean(1, true)).square().mean().item<double>() /
      energy;
  for (const double value : result)
    lsa_require(std::isfinite(value) && value > 0,
                "finite positive protected metric");
  return result;
}

double lsa_cosine(const torch::Tensor &a, const torch::Tensor &b) {
  const double denominator = a.norm().item<double>() * b.norm().item<double>();
  lsa_require(std::isfinite(denominator) && denominator > 0,
              "active gradient direction");
  const double result = a.dot(b).item<double>() / denominator;
  lsa_require(std::isfinite(result), "finite gradient cosine");
  return result;
}

struct LsaResult {
  std::array<bool, 5> adverse{};
};

LsaResult lsa_audit(mtf::MtfJepaMaeVicreg &model, const RmcData &data,
                    const torch::Device &device, int seed, bool endpoint) {
  const std::string root =
      "lsa0.seed_" + std::to_string(seed) + (endpoint ? ".endpoint" : ".start");
  OuterAugmentationGeneratorGuard rng_guard(device);
  const auto rng_before = current_generator_state_snapshot(device);
  const auto state_before = oca_snapshot_state(model);
  std::vector<torch::Tensor> parameters;
  std::vector<std::string> names;
  std::vector<std::size_t> trunk_indices;
  for (const auto &item : model->named_parameters(true)) {
    lsa_require(!item.value().grad().defined(),
                "unexpected populated gradient slot");
    if (!item.value().requires_grad())
      continue;
    if (item.key().rfind("tokenizer.", 0) == 0 ||
        item.key().rfind("encoder.", 0) == 0)
      trunk_indices.push_back(parameters.size());
    names.push_back(item.key());
    parameters.push_back(item.value());
  }
  std::array<LsaGradients, 4> gradients;
  std::array<double, 4> losses{};
  for (auto &component : gradients)
    for (const auto &parameter : parameters)
      component.push_back(torch::zeros_like(parameter));
  for (const int step : {0, 255, 511}) {
    const auto indices =
        torch::tensor(training_rows(data.ssl, seed, step), torch::kInt64);
    const auto x = data.ssl.data.index_select(0, indices).to(device);
    const auto mask = data.ssl.mask.index_select(0, indices).to(device);
    const auto objective = gpv_route_retained_views(model, x, mask, x, mask, 2);
    lsa_require(
        objective.finite && objective.canonical_layout &&
            torch::equal(objective.projected_a, objective.projected_b) &&
            objective.invariance.item<double>() == 0,
        "clean-identical affine objective contract");
    const std::array<torch::Tensor, 4> terms{
        0.0125 * 25 * objective.invariance, 0.0125 * 25 * objective.variance,
        0.0125 * objective.covariance, objective.total};
    for (std::size_t component = 0; component < 4; ++component) {
      losses[component] += terms[component].item<double>() / 3;
      const auto contribution =
          gpv_gradients(terms[component], parameters, component != 3);
      for (std::size_t i = 0; i < parameters.size(); ++i)
        gradients[component][i].add_(contribution[i] / 3);
    }
  }
  std::array<torch::Tensor, 4> flat, trunk;
  for (std::size_t c = 0; c < 4; ++c) {
    flat[c] = gpv_flatten_gradients(gradients[c]).to(torch::kFloat64);
    LsaGradients selected;
    for (const auto i : trunk_indices)
      selected.push_back(gradients[c][i]);
    trunk[c] = gpv_flatten_gradients(selected).to(torch::kFloat64);
    const auto key = root + "." + kLsaComponents[c];
    std::cout << key << ".weighted_loss=" << losses[c] << '\n'
              << key
              << ".weighted_trunk_norm=" << trunk[c].norm().item<double>()
              << '\n';
    for (const std::string prefix :
         {"tokenizer.", "encoder.", "vicreg_stability_head."}) {
      double squared = 0;
      for (std::size_t i = 0; i < names.size(); ++i)
        if (names[i].rfind(prefix, 0) == 0)
          squared +=
              gradients[c][i].to(torch::kFloat64).square().sum().item<double>();
      std::cout << key << '.' << prefix
                << "weighted_norm=" << std::sqrt(squared) << '\n';
    }
  }
  const auto residual = flat[3] - flat[0] - flat[1] - flat[2];
  const double maximum = residual.abs().max().item<double>();
  const double relative =
      residual.norm().item<double>() / flat[3].norm().item<double>();
  lsa_require(flat[0].count_nonzero().item<int64_t>() == 0 && maximum <= 5e-5 &&
                  relative <= 1e-4,
              "component gradient reconstruction");
  const double norm_ratio =
      trunk[2].norm().item<double>() / trunk[3].norm().item<double>();
  lsa_require(std::isfinite(norm_ratio), "finite covariance/full norm ratio");
  const double cosine = lsa_cosine(trunk[2], trunk[1]);
  std::cout << root << ".reconstruction_max_abs=" << maximum << '\n'
            << root << ".reconstruction_relative_l2=" << relative << '\n'
            << root << ".covariance_full_norm_ratio=" << norm_ratio << '\n'
            << root << ".covariance_variance_cosine=" << cosine << '\n'
            << root
            << ".covariance_full_cosine=" << lsa_cosine(trunk[2], trunk[3])
            << '\n'
            << root
            << ".variance_full_cosine=" << lsa_cosine(trunk[1], trunk[3])
            << '\n';
  const auto baseline = lsa_metrics(model, data, device);
  lsa_require(baseline == lsa_metrics(model, data, device),
              "repeat baseline exactness");
  auto copy = gpv_clone_model(model, device);
  copy->eval();
  auto copy_trunk = gpv_trunk_parameters(copy);
  LsaGradients original;
  for (const auto i : trunk_indices)
    original.push_back(parameters[i].detach().clone());
  lsa_require(copy_trunk.size() == original.size(), "virtual trunk layout");
  const double parameter_norm =
      gpv_flatten_gradients(original).to(torch::kFloat64).norm().item<double>();
  lsa_require(std::isfinite(parameter_norm) && parameter_norm > 0,
              "finite trunk parameter norm");
  LsaResult result;
  result.adverse.fill(norm_ratio >= 0.01 && cosine < 0.95);
  for (const double radius : {0.0005, 0.001}) {
    std::array<LsaMetrics, 3> changes{};
    for (int direction = 1; direction <= 3; ++direction) {
      const double step =
          radius * parameter_norm / trunk[direction].norm().item<double>();
      lsa_require(std::isfinite(step) && step > 0, "finite virtual scale");
      {
        torch::NoGradGuard no_grad;
        for (std::size_t i = 0; i < copy_trunk.size(); ++i)
          copy_trunk[i].copy_(original[i] -
                              step * gradients[direction][trunk_indices[i]]);
      }
      LsaGradients displacement;
      for (std::size_t i = 0; i < copy_trunk.size(); ++i)
        displacement.push_back(copy_trunk[i].detach() - original[i]);
      const double realized = gpv_flatten_gradients(displacement)
                                  .to(torch::kFloat64)
                                  .norm()
                                  .item<double>();
      lsa_require(std::abs(realized / (radius * parameter_norm) - 1) <= 1e-4,
                  "realized equal-norm virtual displacement");
      std::cout << root << ".virtual.radius_" << radius << '.'
                << kLsaComponents[direction]
                << ".displacement_norm=" << realized << '\n';
      const auto values = lsa_metrics(copy, data, device);
      for (std::size_t metric = 0; metric < baseline.size(); ++metric) {
        const double delta = values[metric] / baseline[metric] - 1;
        changes[direction - 1][metric] = delta;
        std::cout << root << ".virtual.radius_" << radius << '.'
                  << kLsaComponents[direction] << '.' << kLsaMetrics[metric]
                  << ".baseline=" << baseline[metric] << '\n'
                  << root << ".virtual.radius_" << radius << '.'
                  << kLsaComponents[direction] << '.' << kLsaMetrics[metric]
                  << ".value=" << values[metric] << '\n'
                  << root << ".virtual.radius_" << radius << '.'
                  << kLsaComponents[direction] << '.' << kLsaMetrics[metric]
                  << ".relative_change=" << delta << '\n';
      }
    }
    for (std::size_t m = 0; m < baseline.size(); ++m)
      result.adverse[m] = result.adverse[m] && changes[1][m] <= -0.01 &&
                          changes[1][m] <= changes[0][m] - 0.005 &&
                          changes[2][m] < 0;
  }
  {
    torch::NoGradGuard no_grad;
    for (std::size_t i = 0; i < copy_trunk.size(); ++i)
      copy_trunk[i].copy_(original[i]);
  }
  lsa_require(oca_state_exact(copy, state_before),
              "virtual nontrunk parameters and buffers unchanged");
  lsa_require(oca_state_exact(model, state_before) && !model->is_training(),
              "reference state unchanged");
  for (const auto &p : model->parameters())
    lsa_require(!p.grad().defined(), "reference gradient slots unchanged");
  rng_guard.restore();
  lsa_require(generator_state_snapshot_equal(
                  rng_before, current_generator_state_snapshot(device)),
              "reference RNG unchanged");
  std::cout << root << ".state_gradients_rng_unchanged=true\n";
  return result;
}
} // namespace

int main() {
  try {
    std::cout << std::setprecision(17) << std::boolalpha << std::unitbuf;
    lsa_require(torch::cuda::is_available(),
                "canonical CUDA environment required");
    const torch::Device device(torch::kCUDA, 0);
    torch::set_num_threads(1);
    OuterAugmentationGeneratorGuard all_rng(device);
    const auto protocol = std::string(kLsaDirectory) +
                          "CLEAN_IDENTITY_COVARIANCE_ADMISSION_PROTOCOL.md";
    lsa_require(gpv_sha256_file(protocol) == kLsaProtocolHash,
                "sealed protocol");
    std::cout
        << "schema=lsa0.covariance_admission.v1\nlsa0.protocol_sha256="
        << kLsaProtocolHash << "\nlsa0.source_sha256="
        << gpv_sha256_file(
               std::string(kLsaDirectory) +
               "quality_wikimyei_mtf_jepa_mae_vicreg_covariance_admission.cpp")
        << "\nlsa0.executable_sha256=" << gpv_sha256_file("/proc/self/exe")
        << "\nlsa0.encoder_optimizer_updates=0\nlsa0.ema_updates=0\n"
           "lsa0.optimizers_constructed=0\nlsa0.confirmation_opened=false\n";
    const auto data = rmc_make_data();
    lsa_require(!data.confirmation.data.defined(),
                "confirmation remains unopened");
    const auto authority = lsa_authority(data);
    std::array<std::array<int, 5>, 2> counts{};
    for (int seed_index = 0; seed_index < 3; ++seed_index) {
      for (int endpoint = 0; endpoint < 2; ++endpoint) {
        auto model =
            lsa_load(seed_index, endpoint != 0, data, device, authority);
        const auto result = lsa_audit(model, data, device,
                                      kLsaSeeds[seed_index], endpoint != 0);
        for (std::size_t m = 0; m < result.adverse.size(); ++m)
          counts[endpoint][m] += result.adverse[m];
      }
    }
    bool admitted = false;
    for (int state = 0; state < 2; ++state)
      for (std::size_t m = 0; m < kLsaMetrics.size(); ++m) {
        admitted = admitted || counts[state][m] >= 2;
        std::cout << "lsa0." << (state ? "endpoint" : "start") << '.'
                  << kLsaMetrics[m] << ".adverse_seeds=" << counts[state][m]
                  << '\n';
      }
    all_rng.restore();
    std::cout
        << "lsa0.custody_mechanics_pass=true\nlsa0.process_rng_restored=true\n"
        << "lsa0.decision=" << (admitted ? "admitted" : "not_admitted") << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "lsa0.decision=invalid\nlsa0.error=" << error.what() << '\n';
    return 2;
  }
}
