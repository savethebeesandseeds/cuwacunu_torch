// test_dsl_latent_lineage_state.cpp
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"

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
    const std::string embedding_eval_dsl_path =
        "/cuwacunu/src/config/instructions/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl";
    const std::string evaluation_dsl_path =
        "/cuwacunu/src/config/instructions/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl";

    const std::string grammar = read_text_file(grammar_path);
    const std::string vicreg_dsl = read_text_file(vicreg_dsl_path);
    const std::string mdn_dsl = read_text_file(mdn_dsl_path);
    const std::string embedding_eval_dsl = read_text_file(embedding_eval_dsl_path);
    const std::string evaluation_dsl = read_text_file(evaluation_dsl_path);

    std::string resolved_vicreg_dsl;
    std::string resolve_error;
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> no_variables;
    if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
            vicreg_dsl, no_variables, &resolved_vicreg_dsl, &resolve_error)) {
      throw std::runtime_error("failed to resolve vicreg dsl defaults: " +
                               resolve_error);
    }

    const auto vicreg_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar, resolved_vicreg_dsl);
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

    const auto embedding_eval_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar, embedding_eval_dsl);
    const auto embedding_eval_map = embedding_eval_decoded.to_map();
    assert(embedding_eval_map.size() == 4);
    assert(embedding_eval_map.at("max_samples") == "4096");
    assert(embedding_eval_map.at("max_features") == "2048");
    assert(embedding_eval_map.at("mask_epsilon") == "1e-12");
    assert(embedding_eval_map.at("standardize_epsilon") == "1e-8");

    const auto evaluation_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(grammar, evaluation_dsl);
    const auto evaluation_map = evaluation_decoded.to_map();
    assert(evaluation_map.size() == 4);
    assert(evaluation_map.at("check_temporal_order") == "true");
    assert(evaluation_map.at("validate_vicreg_out") == "true");
    assert(evaluation_map.at("report_shapes") == "false");
    assert(evaluation_map.at("summary_every_steps") == "256");

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

    const auto mixed_interval_lhs =
        cuwacunu::camahjucunu::dsl::parse_latent_lineage_state_lhs(
            "sample_count[0,+inf):uint");
    assert(mixed_interval_lhs.key == "sample_count");
    assert(mixed_interval_lhs.declared_domain == "[0,+inf)");
    assert(mixed_interval_lhs.declared_type == "uint");
    const std::string mixed_interval_domain =
        "sample_count[0,+inf):uint = 0\n";
    const auto mixed_interval_decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar, mixed_interval_domain);
    const auto* e_sample_count = find_entry(mixed_interval_decoded, "sample_count");
    assert(e_sample_count != nullptr);
    assert(e_sample_count->declared_domain == "[0,+inf)");
    assert(e_sample_count->declared_type == "uint");

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

    std::cout << "[test_dsl_latent_lineage_state] entries(vicreg/mdn/embedding/evaluation)="
              << vicreg_decoded.entries.size() << "/"
              << mdn_decoded.entries.size() << "/"
              << embedding_eval_decoded.entries.size() << "/"
              << evaluation_decoded.entries.size() << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_latent_lineage_state] exception: " << e.what() << "\n";
    return 1;
  }
}
