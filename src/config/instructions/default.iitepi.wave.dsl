WAVE train_vicreg_primary {
  MODE: run | train | debug ; // modes: run(0b001), train(0b010), debug(0x0100 == 0b1_0000_0000); combinable: train|debug
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 4000;
  
  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: 0;
      SAMPLER: sequential;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 256;
    };
    RUNTIME: {
      SYMBOL: BTCUSDT;
      FROM: 01.01.2020;
      TO: 31.08.2024;
      SOURCES_DSL_FILE: /cuwacunu/src/config/instructions/default.tsi.source.dataloader.sources.dsl;
      CHANNELS_DSL_FILE: /cuwacunu/src/config/instructions/default.tsi.source.dataloader.channels.dsl;
    };
  };
  
  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.vicreg;
    HASHIMYEI: 0x0000;
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
