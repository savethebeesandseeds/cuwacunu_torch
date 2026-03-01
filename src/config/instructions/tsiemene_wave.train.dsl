/* Draft wave DSL for train mode.
   This is a design target before parser implementation. */
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
