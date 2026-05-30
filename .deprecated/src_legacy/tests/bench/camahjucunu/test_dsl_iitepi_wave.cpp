// test_dsl_iitepi_wave.cpp
#include <cassert>
#include <iostream>
#include <stdexcept>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "piaabo/dfiles.h"

namespace {

void expect_decode_fail(const std::string &grammar, const std::string &text) {
  try {
    (void)cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(grammar,
                                                                  text);
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error("expected decode failure but decode succeeded");
}

} // namespace

int main() {
  try {
    const std::string grammar = cuwacunu::piaabo::dfiles::readFileToString(
        "/cuwacunu/src/config/bnf/iitepi.wave.bnf");
    const std::string instruction_raw =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/instructions/defaults/"
            "default.iitepi.wave.dsl");
    const std::string campaign_grammar =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/bnf/iitepi.campaign.bnf");
    const std::string campaign_instruction =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/instructions/defaults/"
            "default.iitepi.campaign.dsl");

    assert(!grammar.empty());
    assert(!instruction_raw.empty());
    assert(!campaign_grammar.empty());
    assert(!campaign_instruction.empty());

    const auto decoded_campaign =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            campaign_grammar, campaign_instruction);
    assert(!decoded_campaign.binds.empty());

    std::string instruction{};
    std::string resolve_error{};
    const bool resolved_ok =
        cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
            instruction_raw, decoded_campaign.binds.front(), &instruction,
            &resolve_error);
    assert(resolved_ok);
    assert(resolve_error.empty());
    assert(!instruction.empty());

    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(grammar,
                                                                instruction);

    std::cout << "[test_dsl_iitepi_wave] decoded.waves.size="
              << decoded.waves.size() << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.waves.empty());
    const auto &wave = decoded.waves.front();

    assert(!wave.name.empty());
    assert(wave.epochs > 0);
    assert(wave.batch_size > 0);
    assert(wave.max_batches_per_epoch > 0);
    assert(wave.sampler == "sequential" || wave.sampler == "random");

    assert(!wave.wikimyeis.empty());
    assert(!wave.sources.empty());
    assert(wave.probes.empty());
    assert(!wave.sinks.empty());

    for (const auto &w : wave.wikimyeis) {
      assert(!w.binding_id.empty());
      assert(!w.wikimyei_path.empty());
    }
    for (const auto &s : wave.sources) {
      assert(!s.binding_id.empty());
      assert(!s.source_path.empty());
      assert(s.range_warn_batches > 0);
    }
    for (const auto &s : wave.sinks) {
      assert(!s.binding_id.empty());
      assert(!s.sink_path.empty());
    }

    expect_decode_fail(
        grammar,
        "WAVE bad_missing_alias {\n"
        "  MODE: run;\n"
        "  SAMPLER: sequential;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: { FAMILY: tsi.source.dataloader; SETTINGS: { WORKERS: 0; "
        "FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; RUNTIME: { "
        "RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; RECORD_TYPE: kline; "
        "MARKET_TYPE: spot; VENUE: binance; BASE_ASSET: BTC; "
        "QUOTE_ASSET: USDT; }; FROM: 01.01.2020; TO: 02.01.2020; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: "
        "tsi.wikimyei.representation.encoding.vicreg; JKIMYEI: { HALT_TRAIN: "
        "false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_legacy_train_key {\n"
        "  MODE: run|train;\n"
        "  SAMPLER: sequential;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { "
        "WORKERS: 0; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; "
        "RUNTIME: { RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
        "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
        "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: 01.01.2020; "
        "TO: 02.01.2020; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: "
        "tsi.wikimyei.representation.encoding.vicreg; TRAIN: false; "
        "PROFILE_ID: stable_pretrain; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_mode_run_train_flag {\n"
        "  MODE: run;\n"
        "  SAMPLER: sequential;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { "
        "WORKERS: 0; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; "
        "RUNTIME: { RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
        "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
        "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: 01.01.2020; "
        "TO: 02.01.2020; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: "
        "tsi.wikimyei.representation.encoding.vicreg; JKIMYEI: { HALT_TRAIN: "
        "false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_hashimyei_key {\n"
        "  MODE: run|train;\n"
        "  SAMPLER: sequential;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { "
        "WORKERS: 0; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; "
        "RUNTIME: { RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
        "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
        "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: 01.01.2020; "
        "TO: 02.01.2020; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: "
        "tsi.wikimyei.representation.encoding.vicreg; HASHIMYEI: 0x0000; "
        "JKIMYEI: { HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    {
      const auto inferred_profile =
          cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
              grammar, "WAVE infer_profile {\n"
                       "  MODE: run|train;\n"
                       "  SAMPLER: sequential;\n"
                       "  EPOCHS: 1;\n"
                       "  BATCH_SIZE: 4;\n"
                       "  MAX_BATCHES_PER_EPOCH: 4;\n"
                       "  CIRCUIT: circuit_1;\n"
                       "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; "
                       "SETTINGS: { WORKERS: 0; FORCE_REBUILD_CACHE: true; "
                       "RANGE_WARN_BATCHES: 8; }; RUNTIME: { "
                       "RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
                       "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
                       "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: "
                       "01.01.2020; TO: 02.01.2020; }; };\n"
                       "  WIKIMYEI: <w_rep> { FAMILY: "
                       "tsi.wikimyei.representation.encoding.vicreg; JKIMYEI: "
                       "{ HALT_TRAIN: false; }; };\n"
                       "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
                       "}\n");
      assert(inferred_profile.waves.size() == 1);
      assert(inferred_profile.waves.front().wikimyeis.size() == 1);
      assert(
          inferred_profile.waves.front().wikimyeis.front().profile_id.empty());
    }

    {
      const auto staged_wave =
          cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
              grammar,
              "WAVE staged_rep {\n"
              "  MODE: run|train;\n"
              "  SAMPLER: sequential;\n"
              "  EPOCHS: 1;\n"
              "  BATCH_SIZE: 4;\n"
              "  MAX_BATCHES_PER_EPOCH: 4;\n"
              "  CIRCUIT: circuit_1;\n"
              "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: "
              "{ WORKERS: 0; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; "
              "}; RUNTIME: { RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
              "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
              "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: 01.01.2020; TO: "
              "02.01.2020; }; };\n"
              "  WIKIMYEI: <w_rep> { PATH: "
              "tsi.wikimyei.representation.encoding.vicreg.0x00ff; JKIMYEI: { "
              "HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
              "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
              "}\n");
      assert(staged_wave.waves.size() == 1);
      assert(staged_wave.waves.front().wikimyeis.size() == 1);
      const auto &staged_rep = staged_wave.waves.front().wikimyeis.front();
      std::string normalized_hash{};
      assert(cuwacunu::hashimyei::normalize_hex_hash_name("0x00ff",
                                                          &normalized_hash));
      assert(staged_rep.family ==
             "tsi.wikimyei.representation.encoding.vicreg");
      assert(staged_rep.wikimyei_path ==
             "tsi.wikimyei.representation.encoding.vicreg." + normalized_hash);
    }

    expect_decode_fail(
        grammar,
        "WAVE bad_path_family_mismatch {\n"
        "  MODE: run|train;\n"
        "  SAMPLER: sequential;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { "
        "WORKERS: 0; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; "
        "RUNTIME: { RUNTIME_INSTRUMENT_SIGNATURE: { SYMBOL: BTCUSDT; "
        "RECORD_TYPE: kline; MARKET_TYPE: spot; VENUE: binance; "
        "BASE_ASSET: BTC; QUOTE_ASSET: USDT; }; FROM: 01.01.2020; "
        "TO: 02.01.2020; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: "
        "tsi.wikimyei.representation.encoding.vicreg; PATH: tsi.probe.log; "
        "JKIMYEI: { HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    std::cout << "[test_dsl_iitepi_wave] ok\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_dsl_iitepi_wave] exception: " << e.what() << "\n";
    return 1;
  }
}
