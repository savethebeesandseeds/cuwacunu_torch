/*
  .contract.variant.encoder_capacity_projector_midwidth_v1.dsl
  Encoder-capacity plus midwidth-projector VICReg variant.

  Hypothesis:
    the compact projector reduced embedding noise proxies, but it likely
    overcompressed the projector path for MDN-oriented transfer skill. Keep
    the stronger encoder, preserve projector depth, and trim only the peak
    projector width to test whether a gentler reduction improves the
    stability-skill tradeoff without reopening the observation dock.
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
  __vicreg_channel_expansion_dim = 96;
  __vicreg_fused_feature_dim = 64;
  __vicreg_hidden_dims = 48;
  __vicreg_depth = 10;
  __vicreg_projector_mlp_spec = 72-128-192-128;
  __vicreg_projector_norm = LayerNorm;
  __vicreg_projector_activation = SiLU;
  __vicreg_projector_hidden_bias = false;
  __vicreg_projector_last_bias = false;
  __vicreg_config_dsl_file = tsi.wikimyei.representation.encoding.vicreg.dsl;
  __expected_value_config_dsl_file = ../../defaults/default.tsi.wikimyei.inference.expected_value.mdn.dsl;
  __embedding_sequence_analytics_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;
  __transfer_matrix_evaluation_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;
  __observation_sources_dsl_file = ../../defaults/default.tsi.source.dataloader.sources.dsl;
  __observation_channels_dsl_file = tsi.source.dataloader.channels.dsl;
  CIRCUIT_FILE: .circuit.dsl;
  AKNOWLEDGE: w_source = tsi.source.dataloader;
  AKNOWLEDGE: w_rep = tsi.wikimyei.representation.encoding.vicreg;
  AKNOWLEDGE: sink_null = tsi.sink.null;
  AKNOWLEDGE: probe_log = tsi.probe.log;
}
-----END IITEPI CONTRACT-----
