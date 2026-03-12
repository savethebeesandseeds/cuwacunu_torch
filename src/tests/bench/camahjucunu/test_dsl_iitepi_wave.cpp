// test_dsl_iitepi_wave.cpp
#include <cassert>
#include <iostream>
#include <stdexcept>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "piaabo/dfiles.h"

namespace {

void expect_decode_fail(const std::string& grammar, const std::string& text) {
  try {
    (void)cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(grammar, text);
  } catch (const std::exception&) {
    return;
  }
  throw std::runtime_error("expected decode failure but decode succeeded");
}

}  // namespace

int main() {
  try {
    const std::string grammar =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/bnf/iitepi.wave.bnf");
    const std::string instruction =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/instructions/default.iitepi.wave.dsl");

    assert(!grammar.empty());
    assert(!instruction.empty());

    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
            grammar, instruction);

    std::cout << "[test_dsl_iitepi_wave] decoded.waves.size="
              << decoded.waves.size() << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.waves.empty());
    const auto& wave = decoded.waves.front();

    assert(!wave.name.empty());
    assert(wave.epochs > 0);
    assert(wave.batch_size > 0);
    assert(wave.max_batches_per_epoch > 0);
    assert(wave.sampler == "sequential" || wave.sampler == "random");

    assert(!wave.wikimyeis.empty());
    assert(!wave.sources.empty());
    assert(wave.probes.empty());
    assert(!wave.sinks.empty());

    for (const auto& w : wave.wikimyeis) {
      assert(!w.binding_alias.empty());
      assert(!w.wikimyei_path.empty());
      assert(!w.profile_id.empty());
    }
    for (const auto& s : wave.sources) {
      assert(!s.binding_alias.empty());
      assert(!s.source_path.empty());
      assert(!s.sources_dsl_file.empty());
      assert(!s.channels_dsl_file.empty());
      assert(s.range_warn_batches > 0);
    }
    for (const auto& s : wave.sinks) {
      assert(!s.binding_alias.empty());
      assert(!s.sink_path.empty());
    }

    expect_decode_fail(
        grammar,
        "WAVE bad_missing_alias {\n"
        "  MODE: run;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: { FAMILY: tsi.source.dataloader; SETTINGS: { WORKERS: 0; SAMPLER: sequential; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; RUNTIME: { SYMBOL: BTCUSDT; FROM: 01.01.2020; TO: 02.01.2020; SOURCES_DSL_FILE: /tmp/a.dsl; CHANNELS_DSL_FILE: /tmp/b.dsl; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: tsi.wikimyei.representation.vicreg; HASHIMYEI: 0x0000; JKIMYEI: { HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_wikimyei_hash {\n"
        "  MODE: run|train;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { WORKERS: 0; SAMPLER: sequential; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; RUNTIME: { SYMBOL: BTCUSDT; FROM: 01.01.2020; TO: 02.01.2020; SOURCES_DSL_FILE: /tmp/a.dsl; CHANNELS_DSL_FILE: /tmp/b.dsl; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: tsi.wikimyei.representation.vicreg; JKIMYEI: { HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_legacy_train_key {\n"
        "  MODE: run|train;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { WORKERS: 0; SAMPLER: sequential; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; RUNTIME: { SYMBOL: BTCUSDT; FROM: 01.01.2020; TO: 02.01.2020; SOURCES_DSL_FILE: /tmp/a.dsl; CHANNELS_DSL_FILE: /tmp/b.dsl; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: tsi.wikimyei.representation.vicreg; HASHIMYEI: 0x0000; TRAIN: false; PROFILE_ID: stable_pretrain; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    expect_decode_fail(
        grammar,
        "WAVE bad_mode_run_train_flag {\n"
        "  MODE: run;\n"
        "  EPOCHS: 1;\n"
        "  BATCH_SIZE: 4;\n"
        "  MAX_BATCHES_PER_EPOCH: 4;\n"
        "  SOURCE: <w_source> { FAMILY: tsi.source.dataloader; SETTINGS: { WORKERS: 0; SAMPLER: sequential; FORCE_REBUILD_CACHE: true; RANGE_WARN_BATCHES: 8; }; RUNTIME: { SYMBOL: BTCUSDT; FROM: 01.01.2020; TO: 02.01.2020; SOURCES_DSL_FILE: /tmp/a.dsl; CHANNELS_DSL_FILE: /tmp/b.dsl; }; };\n"
        "  WIKIMYEI: <w_rep> { FAMILY: tsi.wikimyei.representation.vicreg; HASHIMYEI: 0x0000; JKIMYEI: { HALT_TRAIN: false; PROFILE_ID: stable_pretrain; }; };\n"
        "  SINK: <sink_null> { FAMILY: tsi.sink.null; };\n"
        "}\n");

    std::cout << "[test_dsl_iitepi_wave] ok\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_iitepi_wave] exception: " << e.what() << "\n";
    return 1;
  }
}
