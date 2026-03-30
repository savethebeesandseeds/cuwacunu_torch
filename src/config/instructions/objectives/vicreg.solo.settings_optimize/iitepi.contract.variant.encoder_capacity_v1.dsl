/*
  Encoder-capacity VICReg variant for vicreg.solo.

  Hypothesis:
    the baseline encoder bottlenecks too early for the frozen 3 x 30 x 9
    observation dock, leaving downstream evaluation worse than the null model.
    This variant widens the private encoder path while preserving the public
    docking boundary and projector policy for a clean ablation.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__future_target_dims = 3;
__future_target_weights = 0.5;
__vicreg_channel_expansion_dim = 96;
__vicreg_fused_feature_dim = 64;
__vicreg_hidden_dims = 48;
__vicreg_depth = 10;
__vicreg_projector_mlp_spec = 72-128-256-128;
__vicreg_projector_norm = LayerNorm;
__vicreg_projector_activation = SiLU;
__vicreg_projector_hidden_bias = false;
__vicreg_projector_last_bias = false;
__vicreg_config_dsl_file = tsi.wikimyei.representation.vicreg.dsl;
__expected_value_config_dsl_file = ../../defaults/default.tsi.wikimyei.inference.mdn.expected_value.dsl;
__embedding_sequence_analytics_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;
__transfer_matrix_evaluation_config_dsl_file = ../../defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;
__observation_sources_dsl_file = ../../defaults/default.tsi.source.dataloader.sources.dsl;
__observation_channels_dsl_file = tsi.source.dataloader.channels.dsl;
CIRCUIT_FILE: iitepi.circuit.dsl;
AKNOWLEDGE: w_source = tsi.source.dataloader;
AKNOWLEDGE: w_rep = tsi.wikimyei.representation.vicreg;
AKNOWLEDGE: sink_null = tsi.sink.null;
AKNOWLEDGE: probe_log = tsi.probe.log;
-----END IITEPI CONTRACT-----
