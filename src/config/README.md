# Cuwacunu Config

`src/config/.config` is the central fresh config file for migrated runtime
paths. Grammar files live under `src/config/grammar/*.bnf`; authored DSL payloads
remain in `src/config/*.dsl`.

Current migrated entries keep Ujcamei focused on the authored source universe:

- `ujcamei_sources_bnf_path`
- `ujcamei_sources_dsl_path`

Kikijyeba protocol `CWU-01v` is separate from Ujcamei identity. Its settings
dock asks which Ujcamei source families are sampled, while its topology file
declares how active nodes and discoverable edges are arranged for the current
node-value procedure.

- `kikijyeba_protocol_cwu_01v_settings_dock_channels_bnf_path`
- `kikijyeba_protocol_cwu_01v_settings_dock_channels_dsl_path`
- `kikijyeba_protocol_cwu_01v_topology_graph_bnf_path`
- `kikijyeba_protocol_cwu_01v_topology_graph_dsl_path`

The graph-first Wikimyei/Jkimyei surface is also explicit:

- `wikimyei_representation_vicreg_bnf_path`
- `wikimyei_representation_vicreg_dsl_path`
- `wikimyei_inference_expected_value_mdn_bnf_path`
- `wikimyei_inference_expected_value_mdn_dsl_path`
- `jkimyei_representation_vicreg_bnf_path`
- `jkimyei_representation_vicreg_dsl_path`
- `jkimyei_inference_expected_value_mdn_bnf_path`
- `jkimyei_inference_expected_value_mdn_dsl_path`

Fresh config filenames use dotted names and avoid old runtime prefixes. BNF
paths should point through `src/config/grammar/`; only DSL payloads should stay
at the config root.

Ujcamei cursors are runtime dataloader/reporting selectors, not authored DSL
files. Source-range and anchor-range cursor objects live in code and are
attached to retrieval reports and graph-anchor batches. The graph-first channel
DSL uses `input_length` and `future_length` for the graph-source window
contract. The source DSL carries `source_kind` provenance
(`real`, `synthetic`, or `derived`) so graph/NodeLift validation can distinguish
real reverse-edge evidence from synthetic or derived rows.
Every graph-first channel dock row must resolve to at least one Ujcamei source
row with the same `interval` and `record_type`.
`channel_weight` is reserved; v1 rejects values other than `1.0` because
weighted channel contribution is not implemented yet.
`DATA_ANALYTICS_POLICY` is decoded and validated with the source contract; the
Jkimyei source analytics reporting surface still needs its fresh report-identity
dependency repaired before those values can drive analytics emission.

The `CWU-01v` topology graph DSL declares active asset nodes and directed
instrument edges. For v1, active graph edge ids must match their
`source_instrument`, and each active edge must resolve to exact real Ujcamei
source rows for every active dock channel.
`GRAPH_POLICY.EDGE_RESOLUTION_POLICY` controls whether the graph edge table is
authoritative. `explicit_only` keeps the authored edge table. `edge_discovery`
allows a nodes-only graph DSL and resolves every active directed edge that has
exact source coverage for all active dock channels and the configured
`GRAPH_POLICY.EDGE_SOURCE_KIND`. Discovery is deterministic and reports missing
ordered node pairs, reverse pairs, connected components, and cycle dimension in
the graph-first dry-run report. V1 uses `EDGE_SOURCE_KIND = real`.
`GRAPH_POLICY.FETCH_MODE` controls graph-anchor edge retrieval. `serial` reads
edges in graph order on the caller thread. `parallel_by_edge` reads staged edge
datasets concurrently, preserves anchor-major/edge-major output order, and is
enabled once `B * L >= PARALLEL_MIN_WORK_ITEMS`. `MAX_FETCH_WORKERS = 0` lets
the runtime choose a conservative worker cap.

`wikimyei.representation.vicreg.*` is the authored representation surface for
graph-first VICReg workers. It configures the rank-4 representation worker and
its node-stream adapter policy, including the Torch dtype/device used when the
graph-first builder materializes representation and MDN modules. The legacy
generic `network_design` DSL is not part of the fresh VICReg surface.

`wikimyei.inference.expected_value.mdn.*` describes the node-centered
ExpectedValue MDN. Its v1 target domain is `node_future`, meaning
target-side future NodeLift tensors:

- `future_node_features [B,C,Hf,N,9]`
- `future_node_mask [B,C,Hf,N,9]`

Future edge tensors remain source evidence for target-side NodeLift and
diagnostics; they are not the active ExpectedValue target.

`jkimyei.representation.vicreg.*` describes representation training
orchestration. `jkimyei.inference.expected_value.mdn.*` describes
node ExpectedValue training orchestration. V1 keeps MDN inference training
frozen with respect to representation and does not expose joint VICReg/MDN
training. `VALIDATION_EVERY` must be `0` in v1 because validation cadence is not
implemented. MDN `SEED`, `REPORT_EVERY`, and `CHECKPOINT_EVERY` are runtime
inputs for the graph-first inference launcher. `SEED` drives `torch::manual_seed`
and, for CUDA devices, `torch::cuda::manual_seed_all`; v1 does not claim full
deterministic CUDA kernel behavior. `CHECKPOINT_EVERY` saves the current
node-MDN state every N optimizer steps to the launcher checkpoint path, replacing
the previous checkpoint at that path. `REPORT_EVERY` rewrites the launcher report
every N attempted batches and writes one final report if the last cadence write
does not already describe the final attempted step.

There are currently no active `.man` files for the fresh graph-first config
surface; this README and the component files in `src/config/grammar/*.bnf` are
the maintained config documentation for the migrated path.
