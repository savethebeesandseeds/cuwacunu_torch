// test_dsl_tsiemene_wave.cpp
#include <cassert>
#include <iostream>
#include <stdexcept>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    const std::string contract_hash =
        cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();

    const std::string grammar =
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_wave_grammar(
            contract_hash);
    const std::string instruction =
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_wave_dsl(
            contract_hash);

    TICK(tsiemene_wave_decode);
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_tsiemene_wave_from_dsl(
            grammar, instruction);
    PRINT_TOCK_ns(tsiemene_wave_decode);

    std::cout << "[test_dsl_tsiemene_wave] instruction:\n";
    std::cout << instruction << "\n";
    std::cout << "[test_dsl_tsiemene_wave] decoded.profiles.size="
              << decoded.profiles.size() << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.profiles.empty());
    const auto& p = decoded.profiles.front();
    assert(!p.name.empty());
    assert(p.mode == "train" || p.mode == "run");
    assert(p.epochs > 0);
    assert(p.batch_size > 0);
    assert(p.max_batches_per_epoch > 0);
    assert(!p.wikimyeis.empty());
    assert(!p.sources.empty());
    for (const auto& w : p.wikimyeis) {
      assert(!w.profile_id.empty());
    }

    const auto expect_decode_fail = [&](const std::string& text) {
      try {
        (void)cuwacunu::camahjucunu::dsl::decode_tsiemene_wave_from_dsl(
            grammar, text);
      } catch (const std::exception&) {
        return;
      }
      throw std::runtime_error(
          "expected decode failure but decode succeeded");
    };

    expect_decode_fail(
        "WAVE_PROFILE p {\n"
        "  MODE = run;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  WIKIMYEI w_rep {\n"
        "    TRAIN = false;\n"
        "  };\n"
        "  SOURCE w_source {\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");
    expect_decode_fail(
        "WAVE_PROFILE p {\n"
        "  MODE = train;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 0;\n"
        "  WIKIMYEI w_rep {\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  SOURCE w_source {\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");
    expect_decode_fail(
        "WAVE_PROFILE p {\n"
        "  MODE = train;\n"
        "  EPOCHS = 1;\n"
        "  WIKIMYEI w_rep {\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  SOURCE w_source {\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_tsiemene_wave] exception: " << e.what() << "\n";
    return 1;
  }
}
