/* Draft wave DSL for run mode.
   This is a design target before parser implementation. */
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
