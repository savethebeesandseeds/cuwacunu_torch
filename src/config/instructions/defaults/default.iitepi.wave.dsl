/*
  WAVE declares runtime policy and required component slots.
  It does not choose concrete component revisions.
  Concrete revision selection belongs to CAMPAIGN.BIND.MOUNT.
*/
WAVE train_vicreg_primary {
  CIRCUIT: circuit_1;
  MODE: run | train | debug ;
  SAMPLER: % __sampler ? sequential %;
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 4000;
  
  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: % __workers ? 0 %;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 256;
    };
    RUNTIME: {
      SYMBOL: % __symbol ? BTCUSDT %;
      FROM: 01.01.2020;
      TO: 31.08.2024;
    };
  };
  
  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: stable_pretrain;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}
