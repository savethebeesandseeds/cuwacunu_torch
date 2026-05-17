#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "jkimyei/training/inference/mdn_trainer.h"
#include "kikijyeba/composition/config_bundle.h"
#include "piaabo/parse/simple_kv_block.h"

namespace {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

std::string read_text(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open " + path);
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

std::unordered_map<std::string, std::string>
read_config_paths(const std::string &path) {
  std::unordered_map<std::string, std::string> out;
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open " + path);
  }
  std::string line;
  while (std::getline(in, line)) {
    line = kv::trim(line);
    if (line.empty() || line[0] == '[' || line[0] == '#') {
      continue;
    }
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = kv::trim(line.substr(0, comment));
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    auto key = kv::trim(line.substr(0, eq));
    auto value = kv::trim(line.substr(eq + 1));
    if (!key.empty() && !value.empty()) {
      out.emplace(std::move(key), std::move(value));
    }
  }
  return out;
}

template <typename Fn> void expect_throw(Fn &&fn, const char *label) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  if (!threw) {
    throw std::runtime_error(std::string("expected throw: ") + label);
  }
}

void test_config_backed_specs_decode_and_validate() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace composition = cuwacunu::kikijyeba::composition;
  namespace training = cuwacunu::jkimyei::training;

  const auto paths = read_config_paths("/cuwacunu/src/config/.config");
  const auto vicreg_spec =
      vicreg::decode_vicreg_node_representation_spec_from_dsl(
          read_text(paths.at("wikimyei_representation_vicreg_dsl_path")));
  const auto mdn_spec = mdn::decode_mdn_spec_from_dsl(
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_path")));
  const auto vicreg_training_spec = training::decode_training_run_spec_from_dsl(
      read_text(paths.at("jkimyei_representation_vicreg_dsl_path")));
  const auto mdn_training_spec = training::decode_training_run_spec_from_dsl(
      read_text(paths.at("jkimyei_inference_expected_value_mdn_dsl_path")));

  auto bundle = composition::load_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  bundle.vicreg = vicreg_spec;
  bundle.mdn = mdn_spec;
  bundle.vicreg_training = vicreg_training_spec;
  bundle.mdn_training = mdn_training_spec;
  composition::validate_graph_first_config_bundle(bundle);

  if (vicreg_spec.component_id != "node_vicreg_v1") {
    throw std::runtime_error("unexpected VICReg component id");
  }
  if (mdn_spec.input_representation_id != vicreg_spec.component_id) {
    throw std::runtime_error("MDN representation reference mismatch");
  }
  if (vicreg_training_spec.component_id != vicreg_spec.component_id) {
    throw std::runtime_error("VICReg training component reference mismatch");
  }
  if (mdn_training_spec.component_id != mdn_spec.component_id) {
    throw std::runtime_error("MDN training component reference mismatch");
  }

  const auto node_opts =
      vicreg::node_adapter_options_from_vicreg_spec(vicreg_spec,
                                                    /*training=*/true);
  if (!node_opts.compact_valid_nodes_for_training ||
      node_opts.required_feature_coords != std::vector<int64_t>({0, 1, 2, 3})) {
    throw std::runtime_error("VICReg node adapter options mismatch");
  }

  const auto mdn_adapter_opts = mdn::mdn_adapter_options_from_spec(mdn_spec);
  if (mdn_adapter_opts.context_reduction !=
      cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          context_reduction_policy_t::last) {
    throw std::runtime_error("MDN context reduction mismatch");
  }
  if (mdn_adapter_opts.target_coords !=
      std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8})) {
    throw std::runtime_error("MDN target coordinate mismatch");
  }

  const auto trainer_opts =
      training::mdn_training_options_from_specs(mdn_training_spec, mdn_spec);
  if (trainer_opts.target_coords !=
          std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8}) ||
      trainer_opts.nll_options.sigma_min != 0.001) {
    throw std::runtime_error("training options mismatch");
  }
}

void test_invalid_specs_fail_fast() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace training = cuwacunu::jkimyei::training;

  auto bad_vicreg = vicreg::vicreg_node_representation_spec_t{};
  bad_vicreg.component_id = "bad";
  bad_vicreg.input_width = 8;
  bad_vicreg.encoding_dim = 1;
  bad_vicreg.channel_expansion_dim = 1;
  bad_vicreg.fused_feature_dim = 1;
  bad_vicreg.encoder_hidden_dim = 1;
  bad_vicreg.encoder_depth = 1;
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg width");

  bad_vicreg.input_width = 9;
  bad_vicreg.dtype = "float16";
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg dtype");

  bad_vicreg.dtype = "float32";
  bad_vicreg.device = "gpu:0";
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg device");

  auto bad_mdn = mdn::mdn_spec_t{};
  bad_mdn.component_id = "mdn";
  bad_mdn.input_representation_id = "rep";
  bad_mdn.target_coords = {99};
  bad_mdn.mixture_count = 2;
  bad_mdn.hidden_width = 4;
  expect_throw([&] { mdn::validate_mdn_spec(bad_mdn); }, "MDN target coord");

  auto bad_training = training::training_run_spec_t{};
  bad_training.training_id = "train";
  bad_training.component_id = "mdn";
  bad_training.learning_rate = 0.01;
  bad_training.batch_size = 1;
  bad_training.freeze_representation = false;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "MDN freeze representation");

  bad_training.freeze_representation = true;
  bad_training.validation_every = 1;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "validation cadence");

  bad_training.validation_every = 0;
  bad_training.seed = -1;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "negative seed");
}

void test_cross_reference_failures() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace composition = cuwacunu::kikijyeba::composition;
  namespace training = cuwacunu::jkimyei::training;

  auto bundle = composition::load_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");

  bundle.mdn.input_representation_id = "other_rep";
  expect_throw([&] { composition::validate_graph_first_config_bundle(bundle); },
               "MDN representation mismatch");

  bundle.mdn.input_representation_id = bundle.vicreg.component_id;
  bundle.mdn_training.component_id = "other_mdn";
  expect_throw([&] { composition::validate_graph_first_config_bundle(bundle); },
               "MDN training component mismatch");
}

} // namespace

int main() {
  test_config_backed_specs_decode_and_validate();
  test_invalid_specs_fail_fast();
  test_cross_reference_failures();
  std::cout << "[test_wikimyei_graph_first_specs] all checks passed\n";
  return 0;
}
