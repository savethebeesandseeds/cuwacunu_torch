/*
  .waves.dsl
  Objective-local wave catalog.

  The intent is to keep smoke, short ablation, eval-only, and primary-train
  loops in one colocated bundle so campaigns can bind them by objective.
  Time windows stay bind-controlled through __from / __to so one campaign can
  express train/eval splits without duplicating the wave catalog.
*/
WAVE train_vicreg_smoke {
  CIRCUIT: circuit_1;
  MODE: run | train | debug ;
  SAMPLER: % __sampler ? sequential %;
  EPOCHS: 1;
  BATCH_SIZE: 16;
  MAX_BATCHES_PER_EPOCH: 16;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: % __workers ? 0 %;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 32;
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
      FROM: % __from ? 01.01.2020 %;
      TO: % __to ? 31.08.2024 %;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.encoding.vicreg;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: % __jk_profile_id ? stable_pretrain %;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}

WAVE train_vicreg_ablation_short {
  CIRCUIT: circuit_1;
  MODE: run | train | debug ;
  SAMPLER: % __sampler ? sequential %;
  EPOCHS: 1;
  BATCH_SIZE: 64;
  MAX_BATCHES_PER_EPOCH: 256;

  SOURCE: <w_source> {
    FAMILY: tsi.source.dataloader;
    SETTINGS: {
      WORKERS: % __workers ? 0 %;
      FORCE_REBUILD_CACHE: true;
      RANGE_WARN_BATCHES: 128;
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
      FROM: % __from ? 01.01.2020 %;
      TO: % __to ? 31.08.2024 %;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.encoding.vicreg;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: % __jk_profile_id ? stable_pretrain %;
    };
  };

  SINK: <sink_null> {
    FAMILY: tsi.sink.null;
  };

  SINK: <probe_log> {
    FAMILY: tsi.probe.log;
  };
}

WAVE train_vicreg_primary {
  CIRCUIT: circuit_1;
  MODE: % __mode_flags ? run | train | debug % ;
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
      FROM: % __from ? 01.01.2020 %;
      TO: % __to ? 31.08.2024 %;
    };
  };
  
  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.encoding.vicreg;
    JKIMYEI: {
      HALT_TRAIN: false;
      PROFILE_ID: % __jk_profile_id ? stable_pretrain %;
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
      RUNTIME_INSTRUMENT_SIGNATURE: {
        SYMBOL: % __symbol %;
        RECORD_TYPE: kline;
        MARKET_TYPE: spot;
        VENUE: binance;
        BASE_ASSET: % __base_asset %;
        QUOTE_ASSET: % __quote_asset %;
      };
      FROM: % __from ? 01.01.2020 %;
      TO: % __to ? 31.08.2024 %;
    };
  };

  WIKIMYEI: <w_rep> {
    FAMILY: tsi.wikimyei.representation.encoding.vicreg;
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
