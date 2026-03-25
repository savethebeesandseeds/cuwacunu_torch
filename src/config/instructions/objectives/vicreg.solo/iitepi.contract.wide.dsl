/*
  Wide architecture candidate for vicreg.solo.

  Keeps big_span_v1 observation docking fixed while increasing internal VICReg
  width to test whether the baseline is under-capacity for the frozen input
  surface.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__future_target_dims = 3;
__future_target_weights = 0.5;
__vicreg_channel_expansion_dim = 96;
__vicreg_fused_feature_dim = 48;
__vicreg_hidden_dims = 32;
__vicreg_depth = 10;
__vicreg_projector_mlp_spec = 72-160-320-160;
__observation_sources_dsl_file = ../../defaults/default.tsi.source.dataloader.sources.dsl;
__observation_channels_dsl_file = tsi.source.dataloader.channels.dsl;
CIRCUIT_FILE: iitepi.contract.circuit.dsl;
AKNOWLEDGE: w_source = tsi.source.dataloader;
AKNOWLEDGE: w_rep = tsi.wikimyei.representation.vicreg;
AKNOWLEDGE: sink_null = tsi.sink.null;
AKNOWLEDGE: probe_log = tsi.probe.log;
-----END IITEPI CONTRACT-----
