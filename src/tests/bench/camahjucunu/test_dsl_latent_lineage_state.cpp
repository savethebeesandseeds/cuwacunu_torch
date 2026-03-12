// test_dsl_latent_lineage_state.cpp
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"

namespace {

std::string read_text_file(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open file: " + path);
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

const cuwacunu::camahjucunu::latent_lineage_state_entry_t* find_entry(
    const cuwacunu::camahjucunu::latent_lineage_state_instruction_t& decoded,
    const std::string& key) {
  for (const auto& e : decoded.entries) {
    if (e.key == key) return &e;
  }
  return nullptr;
}

}  // namespace

int main() {
  try {
    const std::string grammar_path = "/cuwacunu/src/config/bnf/latent_lineage_state.bnf";
    const std::string vicreg_dsl_path =
        "/cuwacunu/src/config/instructions/default.tsi.wikimyei.representation.vicreg.dsl";
    const std::string mdn_dsl_path =
        "/cuwacunu/src/config/instructions/default.tsi.wikimyei.inference.mdn.value_estimation.dsl";
    const std::string probe_dsl_path =
        "/cuwacunu/src/config/instructions/default.tsi.wikimyei.inference.transfer_matrix_evaluation.dsl";

    const std::string grammar = read_text_file(grammar_path);
    const std::string vicreg_dsl = read_text_file(vicreg_dsl_path);
    const std::string mdn_dsl = read_text_file(mdn_dsl_path);
    const std::string probe_dsl = read_text_file(probe_dsl_path);

    const auto vicreg_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(grammar, vicreg_dsl);
    const auto vicreg_map = vicreg_decoded.to_map();
    assert(vicreg_decoded.entries.size() >= 10);
    assert(vicreg_map.at("encoding_dims") == "72");
    assert(vicreg_map.at("projector_mlp_spec") == "128-256-128");
    assert(vicreg_map.at("device") == "gpu");
    const auto* e_proj = find_entry(vicreg_decoded, "projector_mlp_spec");
    assert(e_proj != nullptr);
    assert(e_proj->declared_type == "str");

    const auto mdn_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(grammar, mdn_dsl);
    const auto mdn_map = mdn_decoded.to_map();
    assert(mdn_map.at("target_dims") == "3");
    assert(mdn_map.at("target_weights") == "0.5");
    const auto* e_target_dims = find_entry(mdn_decoded, "target_dims");
    assert(e_target_dims != nullptr);
    assert(e_target_dims->declared_type == "arr[int]");

    const auto probe_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(grammar, probe_dsl);
    const auto probe_map = probe_decoded.to_map();
    assert(probe_map.size() == 4);
    assert(probe_map.at("check_temporal_order") == "true");
    assert(probe_map.at("validate_vicreg_out") == "true");
    assert(probe_map.at("report_shapes") == "false");
    assert(probe_map.at("summary_every_steps") == "256");

    const std::string duplicate_keys =
        "alpha:int = 1\n"
        "alpha = 2\n";
    const auto duplicate_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar, duplicate_keys);
    const auto duplicate_map = duplicate_decoded.to_map();
    assert(duplicate_map.at("alpha") == "2");

    const std::string with_domain =
        "alpha(0,1):double = 0.5\n"
        "mode[a|b|c]:str = a\n"
        "legacy_key = keep\n";
    const auto with_domain_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar, with_domain);
    const auto* e_alpha = find_entry(with_domain_decoded, "alpha");
    assert(e_alpha != nullptr);
    assert(e_alpha->declared_domain == "(0,1)");
    assert(e_alpha->declared_type == "double");
    const auto* e_mode = find_entry(with_domain_decoded, "mode");
    assert(e_mode != nullptr);
    assert(e_mode->declared_domain == "[a|b|c]");
    assert(e_mode->declared_type == "str");
    const auto* e_legacy = find_entry(with_domain_decoded, "legacy_key");
    assert(e_legacy != nullptr);
    assert(e_legacy->declared_domain.empty());
    assert(e_legacy->declared_type.empty());

    const std::string malformed =
        "alpha:int = 1\n"
        "this line is malformed\n";
    bool malformed_rejected = false;
    try {
      (void)cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
          grammar, malformed);
    } catch (const std::exception&) {
      malformed_rejected = true;
    }
    assert(malformed_rejected);

    std::cout << "[test_dsl_latent_lineage_state] entries(vicreg/mdn/probe)="
              << vicreg_decoded.entries.size() << "/"
              << mdn_decoded.entries.size() << "/"
              << probe_decoded.entries.size() << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_latent_lineage_state] exception: " << e.what() << "\n";
    return 1;
  }
}
