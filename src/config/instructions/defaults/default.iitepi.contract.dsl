/*
  CONTRACT = DOCK + ASSEMBLY

  Read this file in two layers:
    DOCK = the public compatibility interface.
    ASSEMBLY = the concrete realization owned by the contract.

  Compatible component revisions share DOCK.
  ASSEMBLY differences stay in realization/provenance.
*/
-----BEGIN IITEPI CONTRACT-----
DOCK {
  __obs_channels = 3;
  __obs_seq_length = 30;
  __obs_feature_dim = 9;
  __embedding_dims = 72;
  __future_target_feature_indices = 3;

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

  INSTRUMENT_SIGNATURE <w_ev> {
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
}

ASSEMBLY {
  __vicreg_projector_norm = LayerNorm;
  __vicreg_projector_activation = SiLU;
  __vicreg_projector_hidden_bias = false;
  __vicreg_projector_last_bias = false;
  __vicreg_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.encoding.vicreg.dsl;
  __expected_value_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.inference.expected_value.mdn.dsl;
  __embedding_sequence_analytics_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;
  __transfer_matrix_evaluation_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;
  __observation_sources_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.sources.dsl;
  __observation_channels_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.channels.dsl;
  CIRCUIT_FILE: /cuwacunu/src/config/instructions/defaults/default.iitepi.circuit.dsl;
  AKNOWLEDGE: w_source = tsi.source.dataloader;
  AKNOWLEDGE: w_rep = tsi.wikimyei.representation.encoding.vicreg;
  AKNOWLEDGE: w_ev = tsi.wikimyei.inference.expected_value.mdn;
  AKNOWLEDGE: sink_null = tsi.sink.null;
  AKNOWLEDGE: probe_log = tsi.probe.log;
}
-----END IITEPI CONTRACT-----
