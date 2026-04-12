/*
  vicreg.solo.train objective-local wave catalog.

  This bundle intentionally exposes one long-horizon train wave, one reusable
  validation wave, and one separate untouched-test wave. The optimized
  model-bearing bundle stays frozen in ../../optim/.
*/
WAVE train_vicreg_primary {
  CIRCUIT: circuit_1;
  MODE: run | train ;
  SAMPLER: sequential;
  EPOCHS: 1024;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 4000;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: 0;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 256;
    };
    RUNTIME: {
      SYMBOL: BTCUSDT;
      FROM: 01.01.2020;
      TO: 31.12.2023;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    HASHIMYEI: 0x00FF;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: stable_pretrain_linear_only;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}

WAVE eval_vicreg_payload {
  CIRCUIT: circuit_1;
  MODE: run | debug ;
  SAMPLER: sequential;
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 512;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: 0;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 128;
    };
    RUNTIME: {
      SYMBOL: BTCUSDT;
      FROM: 01.01.2024;
      TO: 31.08.2024;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    HASHIMYEI: 0x00FF;
    JKIMYEI: {
      HALT_TRAIN: true;
      PROFILE_ID: eval_payload_only;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}

/*
  Separate untouched final test. This final promotion-biased round spends it
  after reusable validation so the campaign ends with confirmation on the last
  held-out window.
*/
WAVE eval_vicreg_payload_untouched_test {
  CIRCUIT: circuit_1;
  MODE: run | debug ;
  SAMPLER: sequential;
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 512;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: 0;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 128;
    };
    RUNTIME: {
      SYMBOL: BTCUSDT;
      FROM: 01.09.2024;
      TO: 31.12.2024;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    HASHIMYEI: 0x00FF;
    JKIMYEI: {
      HALT_TRAIN: true;
      PROFILE_ID: eval_payload_only;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}
