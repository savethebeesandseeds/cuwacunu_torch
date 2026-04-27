/*
  .contract.dsl
  Objective-local train contract.

  The model-bearing VICReg structure remains frozen in ../../optim/, but this
  contract swaps in an objective-local VICReg runtime wrapper so Marshal can tune
  training policy without mutating the optimized bundle directly.
*/
-----BEGIN IITEPI CONTRACT-----
DOCK {
  INSTRUMENT_SIGNATURE <w_source> {
    SYMBOL: ANY;
    RECORD_TYPE: kline;
    MARKET_TYPE: spot;
    VENUE: binance;
    BASE_ASSET: ANY;
    QUOTE_ASSET: ANY;
  };

  INSTRUMENT_SIGNATURE <w_rep> {
    SYMBOL: ANY;
    RECORD_TYPE: kline;
    MARKET_TYPE: spot;
    VENUE: binance;
    BASE_ASSET: ANY;
    QUOTE_ASSET: ANY;
  };

  INSTRUMENT_SIGNATURE <sink_null> {
    SYMBOL: ANY;
    RECORD_TYPE: ANY;
    MARKET_TYPE: ANY;
    VENUE: ANY;
    BASE_ASSET: ANY;
    QUOTE_ASSET: ANY;
  };

  INSTRUMENT_SIGNATURE <probe_log> {
    SYMBOL: ANY;
    RECORD_TYPE: ANY;
    MARKET_TYPE: ANY;
    VENUE: ANY;
    BASE_ASSET: ANY;
    QUOTE_ASSET: ANY;
  };

  __obs_channels = 3;
  __obs_seq_length = 30;
  __obs_feature_dim = 9;
  __embedding_dims = 72;
  __future_target_dims = 3;
}

ASSEMBLY {
  __future_target_weights = 0.5;
  __vicreg_config_dsl_file = tsi.wikimyei.representation.encoding.vicreg.dsl;
  __expected_value_config_dsl_file = ../../defaults/default.tsi.wikimyei.inference.expected_value.mdn.dsl;
  __embedding_sequence_analytics_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;
  __transfer_matrix_evaluation_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;
  __observation_sources_dsl_file = ../../defaults/default.tsi.source.dataloader.sources.dsl;
  __observation_channels_dsl_file = ../../optim/optim.vicreg.solo.tsi.source.dataloader.channels.dsl;
  CIRCUIT_FILE: ../../optim/optim.vicreg.solo.iitepi.circuit.dsl;
  AKNOWLEDGE: w_source = tsi.source.dataloader;
  AKNOWLEDGE: w_rep = tsi.wikimyei.representation.encoding.vicreg;
  AKNOWLEDGE: sink_null = tsi.sink.null;
  AKNOWLEDGE: probe_log = tsi.probe.log;
}
-----END IITEPI CONTRACT-----
