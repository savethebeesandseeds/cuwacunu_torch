/*
  iitepi.wave.example.dsl
  =================
  Purpose:
    Execution policy layer for tsiemene. Topology belongs to circuit/contract.
    Waves define runtime intent: train/run mode, epoch budget, mini-batch size,
    optional per-epoch cap, and per-component train/profile toggles.

  Top-level structure:
    WAVE <wave_name> { ... }
    Multiple WAVE blocks are allowed in one file.

  Required wave fields:
    MODE = <mode_bitmask_expression>;
    SAMPLER = sequential | random;
    EPOCHS = <int>;
    BATCH_SIZE = <int>;
    MAX_BATCHES_PER_EPOCH = <int>;

  Mode bitmask expression:
    - symbolic tokens: run, train, debug
    - numeric masks: decimal, 0x<hex>, 0b<binary>
    - combiners: | , + ^ (treated as bitwise-OR composition)
    Examples:
      MODE = run;
      MODE = run | debug;
      MODE = 0b001 | 0x0100;

  Root scheduling semantics:
    MAX_BATCHES_PER_EPOCH > 0 bounds per-epoch batch production.

  WIKIMYEI block:
    WIKIMYEI <instance_label> {
      PATH = <canonical_component_path>;
      TRAIN = true | false;
      PROFILE_ID = <jkimyei_profile_id>;
    };
    - PATH must match an existing wikimyei instance in the selected contract.
    - PROFILE_ID selects the training policy row from jkimyei specs.

  SOURCE block:
    SOURCE <instance_label> {
      PATH = <canonical_source_path>;
      SYMBOL = <market_symbol>;
      FROM = DD.MM.YYYY;
      TO = DD.MM.YYYY;
      WORKERS = <int>=0;
      FORCE_REBUILD_CACHE = true | false;
      RANGE_WARN_BATCHES = <int>>0;
      SOURCES_DSL_FILE = <sources_dsl_file_path>;
      CHANNELS_DSL_FILE = <channels_dsl_file_path>;
    };
    - PATH must match a source instance in the selected contract.
    - Date range defines the source window for the wave episode.
    - Dataloader file paths may be absolute or relative to the bound wave config folder.

  PROBE block:
    PROBE <instance_label> {
      PATH = <canonical_probe_path_with_hashimyei>;
      TRAINING_WINDOW = incoming_batch;
      REPORT_POLICY = epoch_end_log;
      OBJECTIVE = future_target_dims_nll;
    };
    - PATH must match a probe instance in the selected contract and include explicit hashimyei suffix (`.0x<hex>`).
    - This probe policy is strict in this phase and validates exact tokens only.

  Validation intent:
    - Run-only mode (run without train) must not enable TRAIN=true components.
    - Train mode (train bit present) must enable at least one TRAIN=true component.
    - debug bit enables probe/report side effects (probe epoch reports and
      network-analytics sidecar generation during artifact save).
    - Unknown/missing PATH references fail compatibility validation.
*/

WAVE train_vicreg_primary {
  MODE = train | debug; // modes: run(0b001), train(0b010), debug(0x0100 == 0b1_0000_0000); combinable: train|debug
  EPOCHS = 3;

  SAMPLER = sequential;
  BATCH_SIZE = 64;
  MAX_BATCHES_PER_EPOCH = 4000;

  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {
    PATH = tsi.wikimyei.representation.vicreg.0x0000;
    TRAIN = true;
    PROFILE_ID = stable_pretrain;
  };

  PROBE tsi.probe.representation.transfer_matrix_evaluation.0x0000 {
    PATH = tsi.probe.representation.transfer_matrix_evaluation.0x0000;
    TRAINING_WINDOW = incoming_batch;
    REPORT_POLICY = epoch_end_log;
    OBJECTIVE = future_target_dims_nll;
  };

  SOURCE tsi.source.dataloader {
    PATH = tsi.source.dataloader;
    SYMBOL = BTCUSDT;
    FROM = 01.01.2020;
    TO = 31.08.2024;
    WORKERS = 0;
    FORCE_REBUILD_CACHE = true;
    RANGE_WARN_BATCHES = 256;
    SOURCES_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.sources.dsl;
    CHANNELS_DSL_FILE = /cuwacunu/src/config/instructions/tsi.source.dataloader.channels.dsl;
  };
}
