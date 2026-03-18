/*
  Contract-scoped __variables define hard-static docking compatibility for the
  colocated contract DSL graph. Changing them changes contract identity and the
  compatible hashimyei lineage.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__channel_expansion_dim = 64;
__fused_feature_dim = 32;
__encoder_hidden_dims = 24;
__encoder_depth = 10;
__projector_mlp_spec = 128-256-128;
__projector_dims = 72,128,256,128;
__observation_sources_dsl_file = tsi.source.dataloader.sources.dsl;
__observation_channels_dsl_file = tsi.source.dataloader.channels.dsl;
CIRCUIT_FILE: iitepi.contract.circuit.dsl;
AKNOWLEDGE: w_source = tsi.source.dataloader;
AKNOWLEDGE: w_rep = tsi.wikimyei.representation.vicreg;
AKNOWLEDGE: sink_null = tsi.sink.null;
AKNOWLEDGE: probe_log = tsi.probe.log;
-----END IITEPI CONTRACT-----
