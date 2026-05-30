/*
  WAVE declares runtime policy and required component slots.
  It does not choose concrete component revisions; component hashimyeis are
  derived from the selected contract content during campaign staging.
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
      RUNTIME_INSTRUMENT_SIGNATURE: {
        SYMBOL: % __symbol %;
        RECORD_TYPE: kline;
        MARKET_TYPE: spot;
        VENUE: binance;
        BASE_ASSET: % __base_asset %;
        QUOTE_ASSET: % __quote_asset %;
      };
      FROM: 01.01.2020;
      TO: 31.08.2024;
    };
  };
  
  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.encoding.vicreg;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: stable_pretrain;
    };
  };

  WIKIMYEI: <w_ev> {
    FAMILY: tsi.wikimyei.inference.expected_value.mdn;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: stable_expectation;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}
