// test_dsl_iitepi_wave.cpp
#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
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

    TICK(iitepi_wave_decode);
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
            grammar, instruction);
    PRINT_TOCK_ns(iitepi_wave_decode);

    std::cout << "[test_dsl_iitepi_wave] instruction:\n";
    std::cout << instruction << "\n";
    std::cout << "[test_dsl_iitepi_wave] decoded.waves.size="
              << decoded.waves.size() << "\n";
    std::cout << decoded.str() << "\n";

    assert(!decoded.waves.empty());

    std::unordered_set<std::string> manifest_paths;
    for (const auto& fp : wave_itself->dependency_manifest.files) {
      manifest_paths.insert(fp.canonical_path);
    }
    for (const auto& wave : decoded.waves) {
      assert(!wave.sources.empty());
      const std::string sources_path =
          std::filesystem::weakly_canonical(
              std::filesystem::path(wave_itself->config_folder) /
              wave.sources.front().sources_dsl_file)
              .string();
      const std::string channels_path =
          std::filesystem::weakly_canonical(
              std::filesystem::path(wave_itself->config_folder) /
              wave.sources.front().channels_dsl_file)
              .string();
      assert(manifest_paths.find(sources_path) != manifest_paths.end());
      assert(manifest_paths.find(channels_path) != manifest_paths.end());
    }

    const auto& p = decoded.waves.front();
    assert(!p.name.empty());
    assert(
        cuwacunu::camahjucunu::iitepi_wave_mode_has(
            p.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run) ||
        cuwacunu::camahjucunu::iitepi_wave_mode_has(
            p.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train));
    assert(p.sampler == "sequential" || p.sampler == "random");
    assert(p.epochs > 0);
    assert(p.batch_size > 0);
    assert(p.max_batches_per_epoch > 0);
    assert(!p.wikimyeis.empty());
    assert(!p.sources.empty());
    assert(!p.probes.empty());
    for (const auto& s : p.sources) {
      assert(!s.sources_dsl_file.empty());
      assert(!s.channels_dsl_file.empty());
      assert(s.range_warn_batches > 0);
    }
    for (const auto& w : p.wikimyeis) {
      assert(!w.profile_id.empty());
      assert(!w.wikimyei_path.empty());
    }
    for (const auto& probe : p.probes) {
      assert(!probe.probe_path.empty());
      assert(
          probe.policy.training_window ==
          cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e::
              IncomingBatch);
      assert(
          probe.policy.report_policy ==
          cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e::
              EpochEndLog);
      assert(
          probe.policy.objective ==
          cuwacunu::camahjucunu::iitepi_wave_probe_objective_e::
              FutureTargetDimsNll);
    }

    const auto expect_decode_fail = [&](const std::string& text) {
      try {
        (void)cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
            grammar, text);
      } catch (const std::exception&) {
        return;
      }
      throw std::runtime_error(
          "expected decode failure but decode succeeded");
    };

    const std::string kSourceBlock =
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "    WORKERS = 0;\n"
        "    FORCE_REBUILD_CACHE = true;\n"
        "    RANGE_WARN_BATCHES = 256;\n"
        "    SOURCES_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.sources.dsl;\n"
        "    CHANNELS_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.channels.dsl;\n"
        "  };\n";

    const std::string kProbeBlock =
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n";

    const auto wave_with_probe_source = [&](std::string wave_prefix) {
      wave_prefix += kProbeBlock;
      wave_prefix += kSourceBlock;
      wave_prefix += "}\n";
      return wave_prefix;
    };
    const auto wave_with_source_only = [&](std::string wave_prefix) {
      wave_prefix += kSourceBlock;
      wave_prefix += "}\n";
      return wave_prefix;
    };

    expect_decode_fail(wave_with_probe_source(
        "WAVE p {\n"
        "  MODE = run;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = false;\n"
        "  };\n"));
    expect_decode_fail(wave_with_source_only(
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
        "  };\n"));
    expect_decode_fail(wave_with_source_only(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"));
    expect_decode_fail(wave_with_source_only(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"));
    expect_decode_fail(wave_with_probe_source(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"));
    expect_decode_fail(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
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
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
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
        "    WORKERS = 0;\n"
        "    FORCE_REBUILD_CACHE = true;\n"
        "    RANGE_WARN_BATCHES = 0;\n"
        "    SOURCES_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.sources.dsl;\n"
        "    CHANNELS_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.channels.dsl;\n"
        "  };\n"
        "}\n");

    expect_decode_fail(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n"
        "  SOURCE tsi.source.dataloader {\n"
        "    PATH = tsi.source.dataloader;\n"
        "    SYMBOL = BTCUSDT;\n"
        "    FROM = 01.01.2009;\n"
        "    TO = 31.12.2009;\n"
        "    WORKERS = 0;\n"
        "    FORCE_REBUILD_CACHE = true;\n"
        "    RANGE_WARN_BATCHES = 256;\n"
        "    SOURCES_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.sources.dsl;\n"
        "    CHANNELS_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.channels.dsl;\n"
        "  };\n"
        "}\n");

    expect_decode_fail(wave_with_probe_source(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = stream_epoch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n"));

    expect_decode_fail(wave_with_source_only(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n"));

    expect_decode_fail(wave_with_probe_source(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = on_step_meta;\n"
        "    OBJECTIVE = future_target_dims_nll;\n"
        "  };\n"));

    expect_decode_fail(wave_with_probe_source(
        "WAVE p {\n"
        "  MODE = train;\n"
        "  SAMPLER = sequential;\n"
        "  EPOCHS = 1;\n"
        "  BATCH_SIZE = 4;\n"
        "  MAX_BATCHES_PER_EPOCH = 4;\n"
        "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
        "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
        "    TRAIN = true;\n"
        "    PROFILE_ID = stable_pretrain;\n"
        "  };\n"
        "  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {\n"
        "    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;\n"
        "    TRAINING_WINDOW = incoming_batch;\n"
        "    REPORT_POLICY = epoch_end_log;\n"
        "    OBJECTIVE = mse;\n"
        "  };\n"));

    const auto decode_single_wave_or_throw = [&](const std::string& text) {
      auto parsed =
          cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(grammar, text);
      if (parsed.waves.size() != 1) {
        throw std::runtime_error("expected one wave in decode_single_wave_or_throw");
      }
      return parsed.waves.front();
    };
    const auto make_mode_wave = [&](std::string mode_expr, bool train_value) {
      return std::string("WAVE mode_flags {\n") +
             "  MODE = " + mode_expr + ";\n"
             "  SAMPLER = sequential;\n"
             "  EPOCHS = 1;\n"
             "  BATCH_SIZE = 4;\n"
             "  MAX_BATCHES_PER_EPOCH = 4;\n"
             "  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {\n"
             "    PATH = tsi.wikimyei.representation.vicreg.0x0000;\n"
             "    TRAIN = " +
             std::string(train_value ? "true" : "false") + ";\n"
             "    PROFILE_ID = stable_pretrain;\n"
             "  };\n" +
             kProbeBlock + kSourceBlock + "}\n";
    };

    {
      const auto wave = decode_single_wave_or_throw(make_mode_wave("run | debug", false));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Debug));
      assert(!cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train));
    }
    {
      const auto wave =
          decode_single_wave_or_throw(make_mode_wave("0b001 | 0x0100", false));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Debug));
    }
    {
      const auto wave =
          decode_single_wave_or_throw(make_mode_wave("0b010 ^ 0x0100", true));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train));
      assert(cuwacunu::camahjucunu::iitepi_wave_mode_has(
          wave.mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Debug));
    }

    expect_decode_fail(make_mode_wave("0x0100", false));

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_iitepi_wave] exception: " << e.what() << "\n";
    return 1;
  }
}
