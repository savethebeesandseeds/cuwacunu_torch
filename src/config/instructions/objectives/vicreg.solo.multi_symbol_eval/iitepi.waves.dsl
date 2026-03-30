/*
  vicreg.solo.multi_symbol_eval objective-local wave catalog.

  The optimized model-bearing bundle stays frozen in ../../optim/.
  Evaluation policy lives here because symbol-comparison execution is objective
  policy, not part of the optimized artifact identity.
*/
WAVE eval_vicreg_payload {
  CIRCUIT: circuit_1;
  MODE: run | debug ;
  SAMPLER: % __sampler ? sequential %;
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 512;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: % __workers ? 0 %;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 128;
    };
    RUNTIME: {
      SYMBOL: % __symbol ? BTCUSDT %;
      FROM: % __from ? 01.01.2024 %;
      TO: % __to ? 31.08.2024 %;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    HASHIMYEI: % __vicreg_hashimyei_slot ? 0x00FF %;
    JKIMYEI: {
      HALT_TRAIN: true;
      PROFILE_ID: % __jk_profile_id ? eval_payload_only %;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}
