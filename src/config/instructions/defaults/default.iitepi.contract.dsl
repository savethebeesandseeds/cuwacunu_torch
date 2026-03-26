/*
  Contract-scoped __variables define hard-static docking compatibility and the
  explicit module DSL graph owned by this contract. Changing them changes
  contract identity and the compatible hashimyei lineage.

  Current intended public docking boundary:
    __obs_channels     == active row count in observation_channels DSL
    __obs_seq_length   == max active seq_length in observation_channels DSL
    __obs_feature_dim  == explicit feature width at the VICReg INPUT boundary
    __embedding_dims   == VICReg embedding width exposed to downstream docks

  Additional contract-owned architecture defaults:
    __vicreg_projector_* == contract-owned projector option policy defaults for
                            the canonical network_design bundle

  Fail-fast contract rule:
    because VICReg network_design is required, contract registration aborts
    unless the decoded channel table and network_design agree with these
    docking __variables.

  Future note:
    future_seq_length is already decoded from observation_channels DSL, but it
    is not yet a public docking __variable in the current VICReg contract.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__vicreg_projector_norm = LayerNorm;
__vicreg_projector_activation = SiLU;
__vicreg_projector_hidden_bias = false;
__vicreg_projector_last_bias = false;
__vicreg_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.dsl;
__expected_value_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.inference.mdn.expected_value.dsl;
__embedding_sequence_analytics_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl;
__transfer_matrix_evaluation_config_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl;
__observation_sources_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.sources.dsl;
__observation_channels_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.channels.dsl;
CIRCUIT_FILE: /cuwacunu/src/config/instructions/defaults/default.iitepi.circuit.dsl;
AKNOWLEDGE: w_source = tsi.source.dataloader;
AKNOWLEDGE: w_rep = tsi.wikimyei.representation.vicreg;
AKNOWLEDGE: sink_null = tsi.sink.null;
AKNOWLEDGE: probe_log = tsi.probe.log;
-----END IITEPI CONTRACT-----
