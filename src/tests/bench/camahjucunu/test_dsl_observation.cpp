// test_dsl_observation.cpp
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace {

std::string read_text_file(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

int main() {
  try {
    const std::string source_grammar_path =
        "/cuwacunu/src/config/bnf/tsi.source.dataloader.sources.bnf";
    const std::string source_dsl_path =
        "/cuwacunu/src/config/instructions/tsi.source.dataloader.sources.dsl";
    const std::string channel_grammar_path =
        "/cuwacunu/src/config/bnf/tsi.source.dataloader.channels.bnf";
    const std::string channel_dsl_path =
        "/cuwacunu/src/config/instructions/tsi.source.dataloader.channels.dsl";

    const std::string source_grammar = read_text_file(source_grammar_path);
    const std::string source_dsl = read_text_file(source_dsl_path);
    const std::string channel_grammar = read_text_file(channel_grammar_path);
    const std::string channel_dsl = read_text_file(channel_dsl_path);

    auto decoded = cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
        source_grammar,
        source_dsl,
        channel_grammar,
        channel_dsl);
    if (decoded.source_forms.empty()) {
      std::cerr << "[test_dsl_observation] source_forms is empty\n";
      return 1;
    }
    if (decoded.channel_forms.empty()) {
      std::cerr << "[test_dsl_observation] channel_forms is empty\n";
      return 1;
    }
    if (decoded.csv_bootstrap_deltas < 2) {
      std::cerr << "[test_dsl_observation] csv_bootstrap_deltas must be >=2\n";
      return 1;
    }
    if (!(decoded.csv_step_abs_tol > 0.0L)) {
      std::cerr << "[test_dsl_observation] csv_step_abs_tol must be >0\n";
      return 1;
    }
    if (decoded.csv_step_rel_tol < 0.0L) {
      std::cerr << "[test_dsl_observation] csv_step_rel_tol must be >=0\n";
      return 1;
    }
    if (!decoded.data_analytics_policy.declared) {
      std::cerr << "[test_dsl_observation] data_analytics_policy must be declared\n";
      return 1;
    }
    if (decoded.data_analytics_policy.max_samples <= 0 ||
        decoded.data_analytics_policy.max_features <= 0) {
      std::cerr << "[test_dsl_observation] data_analytics_policy limits must be >0\n";
      return 1;
    }
    if (!(decoded.data_analytics_policy.standardize_epsilon > 0.0L)) {
      std::cerr << "[test_dsl_observation] standardize_epsilon must be >0\n";
      return 1;
    }

    const auto expect_decode_fail = [&](std::string source_text) {
      try {
        (void)cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
            source_grammar,
            std::move(source_text),
            channel_grammar,
            channel_dsl);
      } catch (const std::exception&) {
        return;
      }
      throw std::runtime_error(
          "expected decode failure but decode succeeded");
    };
    expect_decode_fail(
        "/---------------------------------------------------------------------------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                                                     |\n"
        "|---------------------------------------------------------------------------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\---------------------------------------------------------------------------------------------------------/\n");
    expect_decode_fail(
        "CSV_POLICY {\n"
        "  CSV_BOOTSTRAP_DELTAS = 128;\n"
        "  CSV_STEP_ABS_TOL = 0;\n"
        "  CSV_STEP_REL_TOL = 1e-9;\n"
        "};\n"
        "DATA_ANALYTICS_POLICY {\n"
        "  MAX_SAMPLES = 4096;\n"
        "  MAX_FEATURES = 2048;\n"
        "  MASK_EPSILON = 1e-12;\n"
        "  STANDARDIZE_EPSILON = 1e-8;\n"
        "};\n"
        "/---------------------------------------------------------------------------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                                                     |\n"
        "|---------------------------------------------------------------------------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\---------------------------------------------------------------------------------------------------------/\n");
    expect_decode_fail(
        "CSV_POLICY {\n"
        "  CSV_BOOTSTRAP_DELTAS = 128;\n"
        "  CSV_STEP_ABS_TOL = 1e-7;\n"
        "};\n"
        "DATA_ANALYTICS_POLICY {\n"
        "  MAX_SAMPLES = 4096;\n"
        "  MAX_FEATURES = 2048;\n"
        "  MASK_EPSILON = 1e-12;\n"
        "  STANDARDIZE_EPSILON = 1e-8;\n"
        "};\n"
        "/---------------------------------------------------------------------------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                                                     |\n"
        "|---------------------------------------------------------------------------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\---------------------------------------------------------------------------------------------------------/\n");

    std::cout << "[test_dsl_observation] source_forms=" << decoded.source_forms.size()
              << " channel_forms=" << decoded.channel_forms.size() << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_observation] exception: " << e.what() << "\n";
    return 1;
  }
}
