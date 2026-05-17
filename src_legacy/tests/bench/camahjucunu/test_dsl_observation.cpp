// test_dsl_observation.cpp
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace {

std::string read_text_file(const std::string &path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

} // namespace

int main() {
  try {
    const std::string source_grammar_path =
        "/cuwacunu/src/config/bnf/tsi.source.dataloader.sources.bnf";
    const std::string source_dsl_path =
        "/cuwacunu/src/config/instructions/defaults/"
        "default.tsi.source.dataloader.sources.dsl";
    const std::string channel_grammar_path =
        "/cuwacunu/src/config/bnf/tsi.source.dataloader.channels.bnf";
    const std::string channel_dsl_path =
        "/cuwacunu/src/config/instructions/defaults/"
        "default.tsi.source.dataloader.channels.dsl";

    const std::string source_grammar = read_text_file(source_grammar_path);
    const std::string source_dsl = read_text_file(source_dsl_path);
    const std::string channel_grammar = read_text_file(channel_grammar_path);
    const std::string channel_dsl = read_text_file(channel_dsl_path);

    auto decoded =
        cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
            source_grammar, source_dsl, channel_grammar, channel_dsl);
    if (decoded.source_forms.empty()) {
      std::cerr << "[test_dsl_observation] source_forms is empty\n";
      return 1;
    }
    if (decoded.channel_forms.empty()) {
      std::cerr << "[test_dsl_observation] channel_forms is empty\n";
      return 1;
    }
    if (decoded.source_forms.front().market_type != "spot" ||
        decoded.source_forms.front().venue != "binance" ||
        decoded.source_forms.front().base_asset != "BTC" ||
        decoded.source_forms.front().quote_asset != "USDT") {
      std::cerr << "[test_dsl_observation] first source signature mismatch\n";
      return 1;
    }
    for (const auto &channel : decoded.channel_forms) {
      if (channel.normalization_policy != "none" &&
          channel.normalization_policy != "log_returns") {
        std::cerr << "[test_dsl_observation] unexpected normalization_policy: "
                  << channel.normalization_policy << "\n";
        return 1;
      }
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
      std::cerr
          << "[test_dsl_observation] data_analytics_policy must be declared\n";
      return 1;
    }
    if (decoded.data_analytics_policy.max_samples <= 0 ||
        decoded.data_analytics_policy.max_features <= 0) {
      std::cerr
          << "[test_dsl_observation] data_analytics_policy limits must be >0\n";
      return 1;
    }
    if (!(decoded.data_analytics_policy.standardize_epsilon > 0.0L)) {
      std::cerr << "[test_dsl_observation] standardize_epsilon must be >0\n";
      return 1;
    }
    const cuwacunu::camahjucunu::instrument_signature_t btcusdt_spot{
        .symbol = "BTCUSDT",
        .record_type = "kline",
        .market_type = "spot",
        .venue = "binance",
        .base_asset = "BTC",
        .quote_asset = "USDT",
    };
    std::string runtime_match_error{};
    if (!decoded.active_sources_match_runtime_signature(btcusdt_spot,
                                                        &runtime_match_error)) {
      std::cerr << "[test_dsl_observation] default sources should match "
                   "BTCUSDT spot runtime signature: "
                << runtime_match_error << "\n";
      return 1;
    }
    const cuwacunu::camahjucunu::instrument_signature_t btcusdt_futures{
        .symbol = "BTCUSDT",
        .record_type = "kline",
        .market_type = "futures",
        .venue = "binance",
        .base_asset = "BTC",
        .quote_asset = "USDT",
    };
    if (decoded.active_sources_match_runtime_signature(btcusdt_futures,
                                                       &runtime_match_error)) {
      std::cerr << "[test_dsl_observation] default sources should reject "
                   "BTCUSDT futures runtime signature\n";
      return 1;
    }

    const auto expect_decode_fail = [&](std::string source_text) {
      try {
        (void)cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
            source_grammar, std::move(source_text), channel_grammar,
            channel_dsl);
      } catch (const std::exception &) {
        return;
      }
      throw std::runtime_error("expected decode failure but decode succeeded");
    };
    expect_decode_fail(
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                 "
        "                                    |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  "
        "/cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n");
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
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                 "
        "                                    |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  "
        "/cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n");
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
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  source                 "
        "                                    |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     |  "
        "/cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n");
    expect_decode_fail(
        "CSV_POLICY {\n"
        "  CSV_BOOTSTRAP_DELTAS = 128;\n"
        "  CSV_STEP_ABS_TOL = 1e-7;\n"
        "  CSV_STEP_REL_TOL = 1e-9;\n"
        "};\n"
        "DATA_ANALYTICS_POLICY {\n"
        "  MAX_SAMPLES = 4096;\n"
        "  MAX_FEATURES = 2048;\n"
        "  MASK_EPSILON = 1e-12;\n"
        "  STANDARDIZE_EPSILON = 1e-8;\n"
        "};\n"
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  market_type  |  venue  "
        "|  "
        "base_asset  |  quote_asset  |  source                 |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     | spot | binance | ETH | "
        "USDT |  "
        "/cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n");
    expect_decode_fail(
        "CSV_POLICY {\n"
        "  CSV_BOOTSTRAP_DELTAS = 128;\n"
        "  CSV_STEP_ABS_TOL = 1e-7;\n"
        "  CSV_STEP_REL_TOL = 1e-9;\n"
        "};\n"
        "DATA_ANALYTICS_POLICY {\n"
        "  MAX_SAMPLES = 4096;\n"
        "  MAX_FEATURES = 2048;\n"
        "  MASK_EPSILON = 1e-12;\n"
        "  STANDARDIZE_EPSILON = 1e-8;\n"
        "};\n"
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  market_type  |  venue  "
        "|  "
        "base_asset  |  quote_asset  |  source                 |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     | spot | binance | BTC |  "
        "|  "
        "/cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n");

    const std::string one_channel_dsl =
        "/---------------------------------------------------------------------"
        "-------------------------------\\\n"
        "|  interval  |  active  |  record_type  |  seq_length  |  "
        "future_seq_length  |  channel_weight  |  normalization_policy  |\n"
        "|---------------------------------------------------------------------"
        "-------------------------------|\n"
        "|    1d      |   true   |     kline     |      4       |          "
        "2          |       1.0        |          none          |\n"
        "\\--------------------------------------------------------------------"
        "--------------------------------/\n";
    const std::string ambiguous_symbol_sources =
        "CSV_POLICY {\n"
        "  CSV_BOOTSTRAP_DELTAS = 128;\n"
        "  CSV_STEP_ABS_TOL = 1e-7;\n"
        "  CSV_STEP_REL_TOL = 1e-9;\n"
        "};\n"
        "DATA_ANALYTICS_POLICY {\n"
        "  MAX_SAMPLES = 64;\n"
        "  MAX_FEATURES = 32;\n"
        "  MASK_EPSILON = 1e-12;\n"
        "  STANDARDIZE_EPSILON = 1e-8;\n"
        "};\n"
        "/---------------------------------------------------------------------"
        "------------------------------------\\\n"
        "|  instrument  |  interval  |  record_type  |  market_type  |  venue  "
        "|  base_asset  |  quote_asset  |  source                 |\n"
        "|---------------------------------------------------------------------"
        "------------------------------------|\n"
        "|   BTCUSDT    |    1d      |     kline     | spot | binance | BTC | "
        "USDT |  /tmp/BTCUSDT-spot-1d.csv     |\n"
        "|   BTCUSDT    |    1d      |     kline     | futures | binance | BTC "
        "| USDT |  /tmp/BTCUSDT-futures-1d.csv  |\n"
        "\\--------------------------------------------------------------------"
        "-------------------------------------/\n";
    auto ambiguous_decoded =
        cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
            source_grammar, ambiguous_symbol_sources, channel_grammar,
            one_channel_dsl);
    using cuwacunu::camahjucunu::exchange::interval_type_e;
    const auto spot_rows = ambiguous_decoded.filter_source_forms(
        btcusdt_spot, interval_type_e::interval_1d);
    const auto futures_rows = ambiguous_decoded.filter_source_forms(
        btcusdt_futures, interval_type_e::interval_1d);
    if (spot_rows.size() != 1 ||
        spot_rows.front().source != "/tmp/BTCUSDT-spot-1d.csv") {
      std::cerr << "[test_dsl_observation] exact spot signature did not select "
                   "the spot row\n";
      return 1;
    }
    if (futures_rows.size() != 1 ||
        futures_rows.front().source != "/tmp/BTCUSDT-futures-1d.csv") {
      std::cerr << "[test_dsl_observation] exact futures signature did not "
                   "select the futures row\n";
      return 1;
    }
    if (!ambiguous_decoded.active_sources_match_runtime_signature(
            btcusdt_spot, &runtime_match_error) ||
        !ambiguous_decoded.active_sources_match_runtime_signature(
            btcusdt_futures, &runtime_match_error)) {
      std::cerr << "[test_dsl_observation] exact active source validation "
                   "should accept both exact BTCUSDT signatures: "
                << runtime_match_error << "\n";
      return 1;
    }
    const cuwacunu::camahjucunu::instrument_signature_t btcusdt_coinbase{
        .symbol = "BTCUSDT",
        .record_type = "kline",
        .market_type = "spot",
        .venue = "coinbase",
        .base_asset = "BTC",
        .quote_asset = "USDT",
    };
    if (ambiguous_decoded.active_sources_match_runtime_signature(
            btcusdt_coinbase, &runtime_match_error)) {
      std::cerr << "[test_dsl_observation] exact active source validation "
                   "should reject missing venue\n";
      return 1;
    }

    std::cout << "[test_dsl_observation] source_forms="
              << decoded.source_forms.size()
              << " channel_forms=" << decoded.channel_forms.size() << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_dsl_observation] exception: " << e.what() << "\n";
    return 1;
  }
}
