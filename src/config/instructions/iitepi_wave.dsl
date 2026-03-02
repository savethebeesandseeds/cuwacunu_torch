/*
  iitepi_wave.dsl
  =================
  Purpose:
    Execution policy layer for tsiemene. Topology belongs to circuit/contract.
    Waves define runtime intent: train/run mode, epoch budget, mini-batch size,
    optional per-epoch cap, and per-component train/profile toggles.

  Top-level structure:
    WAVE <wave_name> { ... }
    Multiple WAVE blocks are allowed in one file.

  Required wave fields:
    MODE = train | run;
    SAMPLER = sequential | random;
    EPOCHS = <int>;
    BATCH_SIZE = <int>;

  Optional wave fields:
    MAX_BATCHES_PER_EPOCH = <int>;
      - If omitted or <= 0: consume full source range for each epoch.
      - If > 0: bounded epoch (cap yielded batches per epoch).

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
    };
    - PATH must match a source instance in the selected contract.
    - Date range defines the source window for the wave episode.

  Validation intent:
    - MODE=run must not enable TRAIN=true components.
    - MODE=train must enable at least one TRAIN=true component.
    - Unknown/missing PATH references fail compatibility validation.
*/

WAVE train_vicreg_primary {
  MODE = train;
  SAMPLER = sequential;
  EPOCHS = 1;
  BATCH_SIZE = 64;
  MAX_BATCHES_PER_EPOCH = 4;

  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {
    PATH = tsi.wikimyei.representation.vicreg.0x0000;
    TRAIN = true;
    PROFILE_ID = stable_pretrain;
  };

  SOURCE tsi.source.dataloader {
    PATH = tsi.source.dataloader;
    SYMBOL = BTCUSDT;
    FROM = 01.01.2020;
    TO = 31.03.2020;
  };
}

WAVE run_vicreg_embedding {
  MODE = run;
  SAMPLER = sequential;
  EPOCHS = 1;
  BATCH_SIZE = 64;
  MAX_BATCHES_PER_EPOCH = 4;

  WIKIMYEI tsi.wikimyei.representation.vicreg.0x0000 {
    PATH = tsi.wikimyei.representation.vicreg.0x0000;
    TRAIN = false;
    PROFILE_ID = eval_payload_only;
  };

  SOURCE tsi.source.dataloader {
    PATH = tsi.source.dataloader;
    SYMBOL = BTCUSDT;
    FROM = 01.01.2020;
    TO = 31.03.2020;
  };
}
