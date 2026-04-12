/*
  Baseline plus buffer-averaging VICReg variant for vicreg.solo.

  Hypothesis:
    `iitepi.contract.base.dsl` is the current best-known candidate on the
    intended 1d + 4h + 1h dock, but held-out payload embeddings still look too
    noisy and unstable. Keep the baseline network design unchanged and enable
    VICReg buffer averaging as the smallest baseline-centered option-policy
    adjustment for a clean comparison against the current best-known anchor.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__future_target_dims = 3;
__future_target_weights = 0.5;
__vicreg_channel_expansion_dim = 64;
__vicreg_fused_feature_dim = 32;
__vicreg_hidden_dims = 24;
__vicreg_depth = 10;
__vicreg_projector_mlp_spec = 72-128-256-128;
__vicreg_projector_norm = LayerNorm;
__vicreg_projector_activation = SiLU;
__vicreg_projector_hidden_bias = false;
__vicreg_projector_last_bias = false;
__vicreg_config_dsl_file = tsi.wikimyei.representation.vicreg.buffer_averaging_v1.dsl;
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
