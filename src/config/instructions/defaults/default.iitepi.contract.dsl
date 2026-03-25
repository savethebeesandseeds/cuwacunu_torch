/*
  Contract-scoped __variables define hard-static docking compatibility for the
  colocated contract DSL graph. Changing them changes contract identity and the
  compatible hashimyei lineage.

  Current intended public docking boundary:
    __obs_channels     == active row count in observation_channels DSL
    __obs_seq_length   == max active seq_length in observation_channels DSL
    __obs_feature_dim  == explicit feature width at the VICReg INPUT boundary
    __embedding_dims   == VICReg embedding width exposed to downstream docks

  Fail-fast contract rule:
    when __observation_channels_dsl_file and VICReg network_design are both
    present, contract registration aborts unless the decoded channel table and
    network_design agree with these docking __variables.

  Future note:
    future_seq_length is already decoded from observation_channels DSL, but it
    is not yet a public docking __variable in the current VICReg contract.
*/
-----BEGIN IITEPI CONTRACT-----
__obs_channels = 3;
__obs_seq_length = 30;
__obs_feature_dim = 9;
__embedding_dims = 72;
__observation_sources_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.sources.dsl;
__observation_channels_dsl_file = /cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.channels.dsl;
CIRCUIT_FILE: /cuwacunu/src/config/instructions/defaults/default.iitepi.contract.circuit.dsl;
AKNOWLEDGE: w_source = tsi.source.dataloader;
AKNOWLEDGE: w_rep = tsi.wikimyei.representation.vicreg;
AKNOWLEDGE: sink_null = tsi.sink.null;
AKNOWLEDGE: probe_log = tsi.probe.log;
-----END IITEPI CONTRACT-----
