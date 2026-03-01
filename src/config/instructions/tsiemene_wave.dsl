/* Canonical wave instruction set for tsiemene orchestration.
   Topology is contract/circuit; waves define execution and training intent. */

WAVE_PROFILE train_vicreg_primary {
  MODE = train;
  EPOCHS = 1;
  BATCH_SIZE = 64;
  MAX_BATCHES_PER_EPOCH = 4;

  WIKIMYEI w_rep {
    TRAIN = true;
    PROFILE_ID = stable_pretrain;
  };

  SOURCE w_source {
    SYMBOL = BTCUSDT;
    FROM = 01.01.2009;
    TO = 31.12.2009;
  };
}

WAVE_PROFILE run_vicreg_embedding {
  MODE = run;
  EPOCHS = 1;
  BATCH_SIZE = 64;
  MAX_BATCHES_PER_EPOCH = 4;

  WIKIMYEI w_rep {
    TRAIN = false;
    PROFILE_ID = eval_payload_only;
  };

  SOURCE w_source {
    SYMBOL = BTCUSDT;
    FROM = 01.01.2009;
    TO = 31.12.2009;
  };
}
