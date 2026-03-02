// test_dsl_tsiemene_wave.cpp
#include <cassert>
#include <iostream>
#include <stdexcept>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"
#include "piaabo/dconfig.h"

namespace {

const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t* find_bind(
    const cuwacunu::camahjucunu::tsiemene_board_instruction_t& instruction,
    const std::string& binding_id) {
  for (const auto& bind : instruction.binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

}  // namespace

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();
    const std::string board_hash =
        cuwacunu::iitepi::config_space_t::locked_board_hash();
    const std::string binding_id =
        cuwacunu::iitepi::config_space_t::locked_board_binding_id();
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    const auto& board_instruction = board_itself->board.decoded();
    const auto* bind = find_bind(board_instruction, binding_id);
    assert(bind != nullptr);
    const std::string wave_hash =
        cuwacunu::iitepi::board_space_t::wave_hash_for_binding(
            board_hash, binding_id);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);

    const std::string grammar = wave_itself->wave.grammar;
    const std::string instruction = wave_itself->wave.dsl;

    TICK(tsiemene_wave_decode);
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_tsiemene_wave_from_dsl(
            grammar, instruction);
    PRINT_TOCK_ns(tsiemene_wave_decode);

    std::cout << "[test_dsl_tsiemene_wave] instruction:\n";
    std::cout << instruction << "\n";
    std::cout << "[test_dsl_tsiemene_wave] decoded.waves.size="
              << decoded.waves.size() << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.waves.empty());
    const auto& p = decoded.waves.front();
    assert(!p.name.empty());
    assert(p.mode == "train" || p.mode == "run");
    assert(p.sampler == "sequential" || p.sampler == "random");
    assert(p.epochs > 0);
    assert(p.batch_size > 0);
    assert(p.max_batches_per_epoch > 0);
    assert(!p.wikimyeis.empty());
    assert(!p.sources.empty());
    for (const auto& w : p.wikimyeis) {
      assert(!w.profile_id.empty());
      assert(!w.wikimyei_path.empty());
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
        "WAVE p {\n"
        "  MODE = run;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = false;\n"
        "  };\n"
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");
    expect_decode_fail(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 0;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");
    expect_decode_fail(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "  };\n"
        "}\n");
    expect_decode_fail(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
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
