# Synthetic Benchmark Learning Failure Audit

Date: 2026-06-29

Completed audit goal:
`synthetic_mdn_direct_readout_training_surface_deep_audit_v3`

This is the root handoff artifact for the persistent
`synthetic_continuous_graph_v1` learning failure. It is intentionally a findings
document. It does not lower benchmark thresholds and does not implement fixes.

Review update:
This file was rechecked after `synthetic_mdn_direct_readout_intervention_eval.v1`
completed through the durable Marshal/Runtime path. The new result strengthens,
rather than weakens, the diagnosis below: target scaling, direct-head-only
warmup, and early NLL suppression were active and observable, but edge-return
direction/rank stayed random.

Second review update:
`synthetic_mdn_edge_identity_readout_comparator.v1` has now been run on the
same fresh `mdn_edge_context_features.probe`. It confirms that per-edge
specialization recovers more signal than the shared runtime-style readout, but
not enough to solve the deterministic benchmark. A one-hot edge id alone did
not materially improve the shared linear readout. The failure therefore should
not be simplified to "the head is missing only an edge id"; the MDN direct-head
feature surface still lacks a clean, robust edge/phase signal.

Third review update:
`synthetic_mdn_identity_conditioned_direct_readout_ablation.v1` has now run
through the fresh durable synthetic train/eval path. The non-default
`edge_embedding_per_edge` direct readout and adapter were active, gradients
flowed, parameters moved, loss scale was comparable to NLL, and prediction
variance was present. The result is still a negative ablation: train-side
direction/rank stayed near random (`direction=0.514161`,
`margin_direction=0.516307`, `rank=0.508168`, `margin_rank=0.508807`,
`correlation=-0.00177698`). This narrows the next branch away from "the head
only needed edge identity/per-edge projection" and toward objective/readout
alignment, target weighting, or earlier representation/interface signal loss.

## Scope

- Repository: `/cuwacunu`
- Benchmark: `src/config/benchmarks/synthetic_continuous_graph_v1/`
- Original inspection mode: read-only source/config/artifact inspection, plus
  this root Markdown file as explicitly requested.
- Original commands intentionally not run: builds, tests, training, dev
  reset/nuke, process kill, process restart, or parent-session job
  interference.
- Later review updates in this file may record implementation status from
  follow-up goals; the original artifact findings remain tied to their stated
  Runtime reports and probes.
- The worktree is dirty with unrelated changes. Those are not reverted or
  treated as audit edits.

## Executive Conclusion

The evidence points most strongly to:

- **B. Representation/interface signal loss:** primary.
- **C. MDN objective/readout bug:** co-primary.

The evidence does **not** currently point primarily to:

- **A. Synthetic data/target bug:** lower probability.
- **D. Metric/probe bug:** lower probability.
- **E. Range/split/config issue:** lower probability.

The strongest completed-run finding is that the direct edge-return head received
almost edge-invariant feature vectors at a fixed anchor/channel while the
realized edge targets differed materially. The failed runtime direct readout was
a single shared head with no explicit edge/base-node identity input. That makes
the observed near-random direction/rank metrics plausible even though losses
fell, gradients existed, parameters moved, and target scaling/warmup were
active.

Important refinement:
This is not yet proof of total representation collapse. The representation edge
surface has some edge separation, and prior frozen-representation probes found
partial decodability. The sharper failure is the interface/readout path: by the
time the MDN direct head sees its `[base, quote, base - quote]` features, the
edge-specific information is nearly gone, and the shared head has no explicit
edge identity to recover it.

## Fresh Artifact Snapshot

Fresh artifacts inspected:

- [MDN eval report](/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/wrn_f15bdbebc3e23594/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001/channel_inference.report:1)
  - mtime observed in prior audit pass: `2026-06-29 15:19:21 -0300`
  - `mdn_checkpoint_loaded=true`
  - eval range: `requested_anchor_index_begin=760`,
    `requested_anchor_index_end=1088`
  - streamed anchors: `328`
  - source cursor: `Hx=30|Hf=1|edges=3|accepted=1170`

- [MDN edge context feature probe](/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/wrn_f15bdbebc3e23594/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001/mdn_edge_context_features.probe:1)
  - rows: `2952` data rows plus header
  - feature count: `384`
  - target delta mean across same-anchor/channel edge pairs: `0.032426614`
  - target delta max: `0.091444194`
  - feature L2 mean across same-anchor/channel edge pairs: `0.000694828`
  - feature L2 max: `0.003239541`
  - feature max absolute delta across all compared coordinates: `0.001105547`
  - mean feature norm: `15.981062279`

- [Representation edge feature probe](/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/wrn_f15bdbebc3e23594/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001/representation_edge_features.probe:1)
  - rows: `2952` data rows plus header
  - feature count: `96`
  - target delta mean across same-anchor/channel edge pairs: `0.032426614`
  - target delta max: `0.091444194`
  - feature L2 mean across same-anchor/channel edge pairs: `0.018997782`
  - feature L2 max: `0.089504763`
  - feature max absolute delta across all compared coordinates: `0.023669362`
  - mean feature norm: `12.783897168`

- [Direct readout intervention probe](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_direct_readout_intervention_eval.v1.probe:1)
  - target scale: `36`
  - warmup steps configured and observed: `800`
  - warmup NLL weight: `0`
  - direct-head-only warmup: `true`
  - direct head receives gradients and parameters move
  - train stream direction/rank/correlation remain near random:
    - `directional_accuracy=0.498849`
    - `margin_directional_accuracy=0.499075`
    - `pairwise_rank_accuracy=0.505641`
    - `margin_pairwise_rank_accuracy=0.503318`
    - `correlation=0.00053337`
  - protected eval also remains weak:
    - `direct_edge_return_readout_directional_accuracy=0.51084`
    - `direct_edge_return_readout_pairwise_rank_accuracy=0.498645`
    - `direct_edge_return_readout_correlation=0.00461366`
    - `direct_edge_return_readout_pred_to_realized_std_ratio=0.0117962`

- [MDN edge identity readout comparator](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_edge_identity_readout_comparator.v1.probe:1)
  - input rows: `2952`
  - feature count: `384`
  - comparator fit range: `[760,989)`
  - comparator holdout range: `[989,1088)`
  - shared no-edge-id holdout:
    - `directional_accuracy=0.581369`
    - `pairwise_rank_accuracy=0.590348`
    - `best_asset_agreement=0.434343`
    - `correlation=0.254592`
  - shared edge-id holdout:
    - `directional_accuracy=0.582492`
    - `pairwise_rank_accuracy=0.581369`
    - `best_asset_agreement=0.437710`
    - `correlation=0.254598`
  - per-edge holdout:
    - `directional_accuracy=0.685746`
    - `pairwise_rank_accuracy=0.672278`
    - `best_asset_agreement=0.572391`
    - `correlation=0.163083`
  - interpretation: per-edge specialization helps, but it remains far below the
    `0.95` deterministic benchmark expectation and is sensitive to regularizer
    strength.

- [Identity-conditioned direct readout ablation](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_identity_conditioned_direct_readout_ablation.v1.probe:1)
  - mode: `edge_embedding_per_edge`
  - base edge count: `3`
  - identity embedding dim: `16`
  - adapter hidden dim: `128`
  - direct head receives gradients and parameters move
  - prediction variance is present, not fully collapsed:
    `pred_to_realized_std_ratio=0.44949`
  - train stream direction/rank/correlation remain near random:
    - `directional_accuracy=0.514161`
    - `margin_directional_accuracy=0.516307`
    - `pairwise_rank_accuracy=0.508168`
    - `margin_pairwise_rank_accuracy=0.508807`
    - `correlation=-0.00177698`
  - interpretation: explicit edge identity, per-edge projection, and the
    residual direct-readout adapter are not sufficient by themselves.

## Confirmed Findings

### 1. Direct-head inputs are nearly edge-invariant while targets differ

Classification: **B primary, C primary**

Evidence:

- At eval anchor `760`, channel `0`, the MDN direct-head input vectors are almost
  identical across edges while the edge targets are not:
  - `SYNALPHASYNUSD` target: `0.0047713136300444603`
  - `SYNBETASYNUSD` target: `-0.040471012704074383`
  - `SYNGAMMASYNUSD` target: `-0.0028574159368872643`
  - alpha/beta target delta: `0.045242326`
  - alpha/beta 384-feature L2: `0.000320744`
  - alpha/beta max coordinate delta: `0.000107199`

- Across all eval same-anchor/channel edge pairs:
  - mean target delta: `0.032426614`
  - max target delta: `0.091444194`
  - mean 384-feature L2: `0.000694828`
  - max 384-feature L2: `0.003239541`
  - mean feature norm: `15.981062279`

- The runtime direct head constructs edge features as `[base, quote, base -
  quote]` from the post-backbone hidden tensor:
  [DirectEdgeReturnHeadImpl::edge_features](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h:212).

- The MDN probe uses the same direct-head feature surface after the MDN backbone
  and channel adapters:
  [ChannelContextMdnImpl::direct_edge_context_features](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h:202).

- The probe target is the same base-minus-quote future close target:
  [append_mdn_edge_context_feature_probe_rows](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:1402)
  and [target construction](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:1514).

Why it matters:

A shared head cannot reliably learn materially different edge targets from
near-identical inputs unless edge identity is recoverable somewhere else. The
current evidence says it is not recoverable at the direct-head input surface.

Completed diagnostics:

The offline comparator showed that per-edge specialization helps more than a
shared readout, but remains far below oracle-grade. The Runtime ablation then
made edge identity, per-edge projection, and a residual direct-readout adapter
first-class `.jkimyei` settings. That path still stayed near random on
direction/rank. Therefore the old "add edge id/per-edge heads" hypothesis is
not sufficient by itself.

Smallest likely next fix branch:

Keep thresholds strict and move to objective/readout alignment: verify whether
target weighting, feature-family weighting, readout target space, or the MDN
NLL/readout objective coupling is still optimizing a variable that is not the
tradable edge-return sign/rank. If that does not explain the failure, move the
fix earlier into the representation/MDN interface.

### 2. The representation is weakly edge-separated, and the MDN trunk compresses it further

Classification: **B primary, C secondary**

Evidence:

- The raw representation edge probe is not fully collapsed, but edge separation
  is still small relative to target differences:
  - feature count: `96`
  - mean same-anchor/channel feature L2: `0.018997782`
  - mean feature norm: `12.783897168`
  - max feature L2: `0.089504763`

- After the MDN backbone/adapters and direct-head edge construction, the same
  comparison collapses further:
  - feature count: `384`
  - mean same-anchor/channel feature L2: `0.000694828`
  - mean feature norm: `15.981062279`
  - max feature L2: `0.003239541`

- The representation edge probe is built from
  `representation_batch.node_encoding` and writes `[base, quote, base - quote]`:
  [append_representation_edge_feature_probe_rows](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:1247)
  and [feature write loop](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:1373).

- The MTF input adapter carries `node_index` metadata:
  [mtf_channel_node_input_t](/cuwacunu/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h:24)
  and [node_index creation](/cuwacunu/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/channel_node_stream_adapter.h:94).

- That `node_index` is used to reshape outputs back to `[B,N,C,De]`, not as an
  explicit learned node identity in the tokenizer:
  [reshape_channel_encoded](/cuwacunu/src/include/wikimyei/representation/encoding/vicreg/channel_representation_adapter.h:66).

- The MTF token construction adds scale, channel, domain, and position
  embeddings, but no node embedding:
  [time-domain token construction](/cuwacunu/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h:753)
  and [frequency-domain token construction](/cuwacunu/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h:902).

- Channel VICReg is disabled in the active representation config:
  [USE_CHANNEL_VICREG=false](/cuwacunu/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei:46).

Why it matters:

The representation is not dead, but "non-degenerate" is not enough. The
policy-relevant question is whether edge identity and phase/sign survive in a
form a shared downstream readout can use. The fresh probes say that information
is weak at the representation edge surface and almost absent at the direct-head
input surface.

Smallest next diagnostic:

Emit feature-separation probes at each boundary: raw NodeLift future/context,
MTF node encoding, MDN backbone output before adapters, after adapters,
direct-head edge features before LayerNorm, and direct-head hidden activations.
Measure same-anchor/channel edge-pair L2, target delta, and per-edge linear
decodability at each boundary.

Smallest likely fix:

Add explicit node-id or edge-id conditioning to the representation/MDN interface,
or add an auxiliary objective that forces edge/base identity and close-return
phase to survive the representation.

### 3. The final eval direct readout is effectively a tiny-variance negative predictor

Classification: **C symptom, D unlikely**

Evidence from the fresh eval report:
[channel_inference.report](/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/wrn_f15bdbebc3e23594/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001/channel_inference.report:181)

- `direct_edge_return_readout_valid_count=2952`
- `direct_edge_return_readout_directional_accuracy=0.51084`
- `direct_edge_return_readout_pairwise_rank_accuracy=0.498645`
- `direct_edge_return_readout_correlation=0.00461366`
- `direct_edge_return_readout_pred_mean=-0.00758913`
- `direct_edge_return_readout_pred_std=0.000327275`
- `direct_edge_return_readout_pred_min=-0.00784268`
- `direct_edge_return_readout_pred_max=-0.00712639`
- `direct_edge_return_readout_realized_std=0.0277442`
- `direct_edge_return_readout_pred_to_realized_std_ratio=0.0117962`

The per-edge directional accuracies are exactly consistent with an almost
constant negative predictor:

- `0.416667`
- `0.487805`
- `0.628049`

Those match the negative-target fractions for the three edges in the inspected
eval range.

The metrics are computed through the runtime evaluation path, not a separate
stale report path:

- MDN forward/evaluation calls direct readout metrics:
  [launcher metric call](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:3486).
- Report aggregation accumulates the same direct-readout counters:
  [direct readout aggregation](/cuwacunu/src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h:3920).
- The metric accumulator computes `realized_edge = realized_base -
  realized_quote` and compares it against `mdn_out.direct_edge_return`:
  [accumulate_direct_edge_return_readout_metrics](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:476).

Why it matters:

This is a model/output failure, not just a reporting artifact. The final
checkpoint is producing almost the same negative value everywhere on eval.

Smallest next diagnostic:

Inspect direct-head pre-LayerNorm input, post-LayerNorm input, hidden activation,
and final projection output histograms by edge. If the pre-LayerNorm features are
already invariant but the hidden layer adds no edge separation, the head cannot
solve the task without identity/context changes.

### 4. The direct loss and metric target construction are internally consistent

Classification: **A unlikely, D unlikely**

Evidence:

- Direct readout loss uses quote node index `0`, close feature index `3`, and
  base-minus-quote future close:
  [compute_direct_edge_return_readout_loss](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:867)
  and [realized_edge construction](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:884).

- Training target scaling is applied inside the loss only:
  [target scaling](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:902).

- Direction and rank losses use the same scaled prediction/target differences:
  [direction loss](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:912)
  and [rank loss](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:939).

- Metrics use the unscaled direct output and unscaled realized edge:
  [metric target construction](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:503).

- NodeLift uses base `+1`, quote `-1`, and reconstructs edge return as
  `y_base - y_quote`:
  [synthetic_reference_lift.cpp](/cuwacunu/src/impl/wikimyei/expression/nodelift/srl/synthetic_reference_lift.cpp:413).

Why it matters:

The original concern that target scaling, close slot, sign convention, or
NodeLift projection might be the primary cause is not supported by the inspected
code. A target bug remains possible in a broader sense, but the direct readout
loss, direct readout metrics, NodeLift sign, and probe target all align on
base-minus-quote close feature `3`.

Smallest next diagnostic:

Add a temporary read-only assertion/probe in a future session that logs one
batch's direct loss target, direct metric target, MDN probe target, and raw CSV
edge close return for the same anchor/edge/channel. This would close the last
target-path doubt without changing training behavior.

### 5. Direct-head-only warmup freezes the upstream transform feeding the direct head

Classification: **C confirmed, secondary**

Evidence:

- Active MDN config enables direct readout with warmup and direct-head-only
  masking:
  [wikimyei.inference.expected_value.mdn.jkimyei](/cuwacunu/src/config/wikimyei.inference.expected_value.mdn.jkimyei:24).

- The MDN forward path runs `backbone`, then `channel_adapters`, then both the
  MDN head and direct edge head:
  [ChannelContextMdnImpl::forward](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h:193).

- Warmup is active for the configured step range and can set NLL weight to the
  warmup value:
  [direct_edge_return_readout_warmup_active](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:1178)
  and [scheduled NLL weight](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:1189).

- Direct-head-only warmup zeros every gradient whose parameter name does not
  contain `direct_edge_head`:
  [zero_non_direct_edge_head_gradients](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:1203).

- The gradient mask is applied after `out.loss.backward()`:
  [training step](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h:1424).

Why it matters:

This confirms that warmup can train only the final direct head while freezing the
MDN backbone and channel adapters that shape the head's input. Since the fresh
probe shows the direct-head input is almost edge-invariant, this warmup cannot
repair the upstream representation/readout interface. It is a secondary issue,
not the full root cause.

The completed intervention run proves this empirically. It observed exactly
`800` warmup steps, `0` scheduled NLL weight during warmup, direct-head-only
masking enabled, and scheduled NLL restored to `1` afterward. The final
train-range metrics still ended at `directional_accuracy=0.498849`,
`pairwise_rank_accuracy=0.505641`, and `correlation=0.00053337`. So changing
scale and early NLL competition did not unlock the signal.

Smallest likely fix:

If warmup remains enabled, consider allowing gradients into the MDN backbone and
channel adapters while freezing only the mixture/NLL-specific head, or provide a
separate direct-readout adapter that can learn edge-separating features during
warmup.

### 6. The synthetic data and range configuration look sufficient and internally linked

Classification: **A unlikely, E unlikely**

Evidence:

- The generator writes deterministic periodic zero-drift log-price-like close
  series:
  [generate_synthetic_klines.sh](/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v1/generate_synthetic_klines.sh:28).

- The generator writes kline-shaped rows with close in the expected CSV position:
  [CSV row write](/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v1/generate_synthetic_klines.sh:55).

- The source registry declares kline-shaped synthetic instruments with base
  assets `SYNALPHA`, `SYNBETA`, `SYNGAMMA` and quote asset `SYNUSD`:
  [ujcamei.source.registry.dsl](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.registry.dsl:24).

- Active retrieval channels use `record_type=kline`, future length `1`, and
  `log_returns` normalization:
  [ujcamei.source.retrieval.channels.dsl](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.retrieval.channels.dsl:8).

- The manifest reports enough cycles:
  [synthetic_periodic_chart_manifest.v1.report](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_periodic_chart_manifest.v1.report:1)
  - `row_count=1200`
  - `accepted_anchor_count=1170`
  - `train_anchor_end_exclusive=730`
  - `eval_anchor_begin=760`
  - `eval_anchor_end_exclusive=1088`
  - `minimum_train_cycles=60.833333`
  - `minimum_eval_cycles=27.333333`
  - `periodic_chart_sufficient_cycles=true`

- The fresh eval report uses the intended eval range and matching graph/source
  cursor:
  [eval report source cursor](/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/wrn_f15bdbebc3e23594/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001/channel_inference.report:68).

Why it matters:

The failure does not currently look like an insufficient-cycle, stale-range, or
wrong-split problem. The data is simple, periodic, kline-compatible, and the
fresh artifacts point at the intended eval range.

Smallest next diagnostic:

If target doubts remain, compute a single anchor/edge/channel trace from raw CSV
close values through normalized log return, NodeLift potential, MDN future
target, direct loss target, and eval metric target.

### 7. Older control probes are useful but not decisive for the runtime direct head

Classification: **C comparator caveat**

Evidence:

- The supervised control script caps feature count and adds an intercept:
  [read_features](/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v1/run_direct_edge_return_supervised_probe.sh:377).

- It accumulates and solves separate normal equations per edge:
  [accumulate](/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v1/run_direct_edge_return_supervised_probe.sh:389)
  and [solve_edge](/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v1/run_direct_edge_return_supervised_probe.sh:404).

- The runtime direct head is not per-edge. It is one shared `LayerNorm -> Linear
  -> SiLU -> Linear` readout over `[base, quote, base - quote]` features:
  [DirectEdgeReturnHeadImpl](/cuwacunu/src/include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h:198).

Why it matters:

Previous probes showing that an offline/per-edge or capped linear readout can
decode signal should not be treated as proof that the shared runtime direct head
can learn the same function. The clean next comparison must be performed on the
exact runtime `mdn_edge_context_features.probe` with shared versus edge-aware
readout variants.

### 8. The edge-identity comparator narrows the fix but does not solve the benchmark

Classification: **B/C refinement**

Evidence:

- The comparator was run on the exact fresh MDN direct-head input surface:
  [synthetic_mdn_edge_identity_readout_comparator.v1.probe](/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_edge_identity_readout_comparator.v1.probe:1).

- A shared linear readout without edge id reaches only:
  - holdout direction: `0.581369`
  - holdout pairwise rank: `0.590348`
  - holdout correlation: `0.254592`

- Adding a simple one-hot edge id to the shared linear readout does not
  materially improve the holdout result:
  - holdout direction: `0.582492`
  - holdout pairwise rank: `0.581369`
  - holdout correlation: `0.254598`

- Three per-edge linear readouts improve the result:
  - holdout direction: `0.685746`
  - holdout pairwise rank: `0.672278`
  - holdout best-asset agreement: `0.572391`

- The per-edge result is still far below the deterministic benchmark threshold
  and shows unstable scale under low regularization:
  - default per-edge `pred_to_target_std_ratio=8.324974`
  - with a stronger manual ridge check at `lambda=1e-2`, direction remains only
    about `0.629`, pairwise rank about `0.620`, and
    `pred_to_target_std_ratio` about `1.219`

Why it matters:

This validates part of the identity/readout hypothesis: a single shared linear
readout over the current MDN direct-head features is not enough, and per-edge
specialization extracts more signal. But identity by itself is not a complete
fix. The signal available at this surface is partial and poorly conditioned.

Smallest likely fix:

The next code-change branch should not only append an edge id to the final
readout. It should test an identity-conditioned or per-edge readout together
with an information-preserving pathway into that readout, such as learned
node/edge embeddings, direct-readout adapters, or gradients into the MDN
backbone/adapters during the direct objective.

## Suspicious But Unproven

1. **Node identity may be missing too early.**
   The code carries `node_index` for reshaping, but the inspected MTF token
   construction does not add a learned node embedding. This is likely relevant,
   but the exact point where identity is lost still needs a boundary-by-boundary
   probe.

2. **LayerNorm/hidden activations inside the direct head were not fully probed.**
   The `mdn_edge_context_features.probe` captures direct-head inputs before the
   direct head's internal LayerNorm and hidden layer. Those internal activations
   could be inspected next to verify no edge separation appears inside the head.

3. **The full train probe was not exhaustively scanned in this cleanup pass.**
   The fresh eval probe is enough to explain eval failure, and the intervention
   probe shows train-stream metrics remain near random. A full train/eval
   feature-distance comparison would still be useful before a final code fix.

4. **Operational path confusion exists in benchmark docs/scripts, but is not the
   main explanation for the fresh run.**
   The fresh eval report, train report, checkpoint path, graph fingerprint, and
   source cursor are internally linked. Path hygiene can still be improved later.

## Recommended Next Steps

1. **Do not lower benchmark thresholds.**
   The benchmark data is intended to be simple; the observed failure is
   diagnostic signal, not a threshold calibration issue.

2. **Run the objective/readout-alignment ablation next.**
   The identity-conditioned/per-edge intervention has now been run and stayed
   near random despite gradients, parameter movement, comparable loss scale, and
   nontrivial prediction variance. The next branch is not another identity
   scaffold; it is an objective-schedule ablation that can suppress ordinary MDN
   NLL after warmup and test whether the edge-return objective is still being
   drowned or redirected by post-warmup NLL competition.

3. **Probe the feature surface by boundary if the next intervention still fails.**
   For each boundary, report same-anchor/channel edge-pair target delta, feature
   L2, feature norm, per-edge decodability, and shared-readout decodability:
   - raw normalized edge returns,
   - NodeLift node context,
   - MTF node encoding,
   - MDN backbone output before adapters,
   - MDN output after channel adapters,
   - direct-head input,
   - direct-head post-LayerNorm input,
   - direct-head hidden activation.

4. **Do not treat implementation as success until the oracle gates improve.**
   The focused launcher test proves the non-default mode is wired. It does not
   prove that the deterministic edge-return direction/rank problem is solved.

5. **Revisit warmup only after the input surface is fixed.**
   Direct-head-only warmup is confirmed too narrow, but changing warmup alone is
   unlikely to solve near-identical direct-head inputs.

6. **Use `synthetic_mdn_objective_readout_alignment_ablation.v1` for the next
   bounded run.**
   The MDN `.jkimyei` now exposes
   `MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT`, and
   `run_mdn_objective_readout_alignment_ablation.sh` can run the
   `edge_dominant_post_nll_0` variant through the durable synthetic
   Marshal/Runtime path. This should be interpreted narrowly: if the metrics
   remain random with post-warmup NLL suppressed, the root cause moves away from
   objective competition and toward representation/interface signal availability
   or target/readout structure.

## Classification Summary

- **A. Synthetic data/target bug:** unlikely primary. Data, close slot, NodeLift
  sign, direct loss target, probe target, and metric target are internally
  aligned.
- **B. Representation signal loss:** strongest current explanation. Edge/identity
  signal is weak at representation output and only partially recoverable at MDN
  direct-head input. This should be read as interface-surface signal loss, not
  proof that the representation network is entirely collapsed.
- **C. MDN objective/readout bug:** co-primary. The direct head is shared, lacks
  strong identity conditioning, receives weakly separated inputs, and
  direct-head-only warmup freezes the upstream transform.
- **D. Metric/probe bug:** unlikely primary. The final eval metrics use the same
  checkpoint and target convention, and the report behavior matches a constant
  negative predictor.
- **E. Range/split/config issue:** unlikely primary. The fresh artifacts use the
  intended eval range and accepted-anchor cursor, with sufficient periodic
  cycles.

## Audit Hygiene

- Only the benchmark-local comparator helper was compiled and run.
- No project builds were run.
- No project tests were run.
- No training was launched by this audit update.
- No reset/nuke command was run.
- No runtime or parent-session job was killed, restarted, or interfered with.
- This Markdown file is the only intended audit artifact edit from this cleanup.

## 2026-07-13 Cached-Feature Parity Update

This update supersedes the earlier ordering that put the post-warmup NLL
ablation first. A new diagnostic now trains the unchanged production
`DirectEdgeReturnHead` directly on its surviving cached post-adapter features,
without the MDN NLL, trunk, representation, or policy in the optimization loop.

The surviving probe contains 328 complete anchors in `[760,1088)`, with three
edges, three channels, and 400 features per row. The first 384 values are the
exact `[base, quote, base - quote]` dynamic surface for `H=128`; the final 16
are the old checkpoint's edge embedding. Reconstructing the adapted node
context gives both:

```text
base_minus_quote_reconstruction_max_abs_error = 0
repeated_quote_consistency_max_abs_error = 0
```

The diagnostic chronologically splits this already-protected eval surface into
fit `[760,989)` and holdout `[989,1088)`. This internal split is diagnostic
only; it does not replace the benchmark train/eval gate.

### Exact Production Head Result

With the production Adam rate, 3,500 steps, clip `5`, and direct objective
weights `100/5/5`, every production identity mode fails on its own fit rows.
The active `edge_embedding_per_edge` result is:

```text
fit.directional_accuracy = 0.508976
fit.pairwise_rank_accuracy = 0.508491
fit.correlation = 0.002458
holdout.directional_accuracy = 0.517396
holdout.pairwise_rank_accuracy = 0.507295
holdout.correlation = -0.020625
```

The head also fails small-subset memorization:

```text
16 anchors, 2,000 steps: direction 0.784722, rank 0.708333
 4 anchors, 5,000 steps: direction 0.805556, rank 0.777778
```

This is direct evidence that the immediate failure is not upstream signal loss
or NLL competition. The production readout cannot reliably extract or memorize
the cached signal when trained in isolation.

### Conditioning Boundary

A globally standardized, closed-form per-edge linear decoder on the identical
384 dynamic features reaches:

```text
fit.directional_accuracy = 0.859292
fit.pairwise_rank_accuracy = 0.842310
fit.correlation = 0.850094
holdout.directional_accuracy = 0.640853
holdout.pairwise_rank_accuracy = 0.650954
```

Applying the production-style per-sample LayerNorm before the same linear solve
drops the result to:

```text
fit.directional_accuracy = 0.629306
fit.pairwise_rank_accuracy = 0.603105
fit.correlation = 0.377413
holdout.directional_accuracy = 0.545455
holdout.pairwise_rank_accuracy = 0.561167
```

LayerNorm is therefore a confirmed major conditioning loss, but not the whole
failure. Feature BatchNorm before the existing hidden SiLU path improves fit
correlation only to `0.292616`; scaling the difference branch by `100` does not
help. Both prototypes were removed rather than retained as unused production
modes.

### Optimizer Boundary

The same fit-only standardized per-edge linear architecture was then optimized
with the Runtime objective. It still reached only direction/rank/correlation
`0.556526/0.525473/0.134594` on fit. Removing clipping, using a purely
quadratic loss, increasing the learning rate, extending mini-batch training to
20,000 steps, using a full batch, and zero-initializing the projections all
failed to approach the closed-form solution. The strongest full-batch result
was still only about `0.632/0.580/0.387` on fit.

The readout problem is thus both architectural and numerical:

1. per-sample LayerNorm suppresses a large part of the decodable variation;
2. the hidden SiLU readout loses more signal;
3. the current Adam recipe is ill-conditioned even for the remaining convex,
   standardized linear problem.

The next bounded forecast experiment was therefore fixed as a fit-only
standardized per-edge analytic ridge calibration over the same frozen Runtime
features. Its result is recorded below. The feature-cache provenance gap was
then closed by replaying the frozen checkpoints over the actual training
range, as recorded in the following train-to-eval gate.

### Ridge Calibration Result

The diagnostic now uses CPU `float64` Cholesky solves with an unpenalized
intercept and normalized positive ridge penalties. Alpha is selected only by
raw-target RMSE on `[943,989)` after fitting `[760,943)`. The chosen model is
then refit on `[760,989)` and described on `[989,1088)`.

All 328 anchors still come from the previously protected evaluation probe.
The last block is unseen by alpha selection, but it is a diagnostic
confirmation block, not a new benchmark test.

Validation RMSE selects `alpha=0.01`:

```text
selection validation:
  RMSE      = 0.027788
  direction = 0.500000
  rank      = 0.485507
  corr      = 0.095491
  scale     = 0.184826

refit diagnostic confirmation:
  RMSE      = 0.028203
  direction = 0.529742
  rank      = 0.565657
  corr      = 0.074866
  scale     = 0.312642
```

Low penalties expose the trade-off hidden by the earlier near-OLS result. For
example, `alpha=1e-8` reaches validation direction `0.683575`, but rank is only
`0.591787`, RMSE worsens to `0.043643`, and prediction scale rises to
`1.329233`. No candidate simultaneously provides calibrated error and stable
direction/rank. Two independent executions produced byte-identical probes,
and the maximum normalized solve residual was below the printed precision.

This refines the diagnosis:

1. the production head and Adam recipe still fail to extract even the
   high-variance linear solution;
2. the strong closed-form fit result proves capacity to interpolate this
   cached surface, not stable forecast generalization;
3. the frozen representation cannot yet be called production-ready;
4. a production calibrated readout would be premature.

The next honest gate needs post-adapter caches from the benchmark training
range and its protected evaluation range. Alpha and statistics must be chosen
using training anchors only, followed by a one-shot protected evaluation. That
cache capture should be attempted before paying for a new representation
training run.

### Train-to-Eval Ridge Gate

The frozen representation and MDN checkpoints were replayed over `[0,730)` in
run/debug mode. Runtime loaded both checkpoints, performed zero optimizer
steps, wrote no checkpoint, and emitted exactly `6,570` post-adapter probe
rows. The already-open protected eval probe supplies `[760,1088)`. The
unconsumed `[1088,1170)` holdout was not captured, and the capture wrapper now
refuses that range unless it is explicitly unsealed.

Alpha selection stays wholly inside training with the required history/future
purge:

```text
selection fit:        [0,554)
purge:                [554,584)
selection validation: [584,730)
refit:                [0,730)
diagnostic eval:      [760,1088)
```

The unchanged benchmark feasibility gate is direction `>=0.95` and rank
`>=0.95`. No ridge candidate clears it. The lowest-RMSE diagnostic fallback is
`alpha=1e-10`:

```text
inner train validation:
  RMSE      = 0.020837
  direction = 0.805936
  rank      = 0.793760
  corr      = 0.687220
  scale     = 0.877468

refit protected-eval diagnostic:
  RMSE      = 0.018693
  direction = 0.810976
  rank      = 0.785908
  corr      = 0.740333
  scale     = 0.785278
```

This supersedes the pessimistic interpretation of the small eval-only ridge
split. With enough genuine training rows, the frozen post-adapter surface has
material, stable signal. It is still not oracle-grade.

The failure is therefore two-part:

1. the production readout stays near random despite a frozen linear decoder
   reaching about `0.81/0.79` direction/rank;
2. the frozen representation/context surface itself remains far below the
   deterministic `0.95/0.95` requirement.

A calibrated readout could recover a large amount of current signal but cannot
solve this benchmark alone. The controlled outer-input augmentation ablation
described below has now repeated the same train-only selection and historical
eval diagnostic while leaving the unconsumed holdout sealed.

### Augmentation Characterization

A new deterministic CPU test also measures temporal augmentation by history:

```text
light_phase_safe_v2 retention: H4=0.5, H10=0.8, H30=0.9
period-8 oracle direction:     H4=0.96875, H10=1.0, H30=1.0
```

The outer temporal augmentation hides disproportionate H4 loss inside its
aggregate retention, but it does not destroy a simple periodic oracle. The
full outer-stack control below shows that removing the group does not improve
task-aligned decodability, so this unit-scale damage is not sufficient to
explain the random direct head.

Durable evidence:

```text
src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/
  synthetic_mdn_cached_feature_runtime_head_parity.v1.report
  synthetic_mdn_frozen_affine_optimizer_parity_v1.report
  synthetic_mdn_frozen_feature_ridge_diagnostic.v1.report
  synthetic_mdn_frozen_train_eval_ridge_gate.v1.report
  synthetic_mtf_augmentation_phase_diagnostic.v1.report
  synthetic_mtf_outer_input_augmentation_off_v1.report
  synthetic_mtf_vicreg_weak_view_augmentation_off_v1.report
  synthetic_mtf_vicreg_gradient_off_v1.report
```

New reproducible surfaces:

```text
src/scripts/benchmarks/synthetic_continuous_graph_v1/
  run_mdn_cached_feature_runtime_head_parity.sh
  run_mdn_frozen_affine_optimizer_parity.sh
  run_mdn_frozen_feature_capture.sh
  run_mtf_outer_input_augmentation_off_ablation.sh
  run_mtf_vicreg_weak_view_augmentation_off_ablation.sh
  run_mtf_vicreg_gradient_off_ablation.sh
src/config/benchmarks/synthetic_continuous_graph_v1/
  wikimyei.representation.mtf_jepa_mae_vicreg.outer_input_augmentation_off_v1.jkimyei
  wikimyei.representation.mtf_jepa_mae_vicreg.vicreg_weak_view_augmentation_off_v1.jkimyei
  wikimyei.representation.mtf_jepa_mae_vicreg.vicreg_gradient_off_v1.jkimyei
src/tests/bench/jkimyei/training/channel_graph_first_launchers/
  test_jkimyei_mtf_jepa_mae_vicreg_augmentations.cpp
src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/
  test_wikimyei_mtf_jepa_mae_vicreg.cpp
```

## 2026-07-14 Outer-Input Augmentation-Off Ablation

This fixed-seed diagnostic retrained the representation for 3,000 steps and a
matched MDN for 3,500 steps. It changed only the six non-neutral,
launcher-owned outer controls: Gaussian jitter, time dilation, time warp,
amplitude scaling, frequency masking, and frequency jitter. The canonical
masking and losses stayed fixed. This was not a complete no-augmentation run:
the model-internal VICReg weak-view time drop (`0.01`) and Gaussian jitter
(`0.005`) remained active.

Both checkpoints were trained from scratch on `[0,730)`. Frozen replay then
produced 6,570 train rows and 2,952 historical-eval rows on both the raw
representation and post-MDN surfaces. The captures loaded the exact requested
checkpoints, performed zero optimizer steps, wrote no checkpoints, reconstructed
`base - quote` exactly, and contained no anchor at or above 1088. Alpha
selection again used only `[0,554)`, purge `[554,584)`, and validation
`[584,730)`; `[760,1088)` remained previously consumed diagnostic confirmation.

Values below are `direction / rank / correlation / RMSE`:

| Surface and split | `light_phase_safe_v2` | Outer input off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.696347 / 0.694064 / 0.546996 / 0.023653` | `-0.034247 / -0.006849 / -0.031106 / +0.000041` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.707656 / 0.693428 / 0.584996 / 0.022502` | `-0.035569 / -0.029472 / -0.042065 / +0.000809` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.765601 / 0.763318 / 0.662225 / 0.021203` | `-0.040335 / -0.030441 / -0.024995 / +0.000366` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.775068 / 0.758130 / 0.706236 / 0.019694` | `-0.035908 / -0.027778 / -0.034097 / +0.001001` |

The raw representation selected `alpha=1e-8` instead of the baseline
`1e-10`, but this does not explain the result. A fixed-`1e-10` control also
worsened historical direction/rank to `0.718157/0.701220`. The post-MDN ridge
kept `alpha=1e-10`. Both selected solvers reproduced byte-for-byte, and neither
surface approached the unchanged `0.95/0.95` feasibility gate.

The raw probe has one metadata caveat: the comparator emits
`probe_boundary=post_adapter_direct_edge_features` for every input. Its actual
raw boundary is the pre-MDN-adapter `H=32` representation node encoding with
96 features laid out as `[base, quote, base - quote]`. The 400-feature MDN
probe is the true post-adapter boundary: 384 dynamic values plus 16 cached
identity values ignored by the per-edge ridge.

The narrow result is consistent across validation and historical eval:
disabling the outer stack hurts both feature surfaces. At seed 17 the outer
profile appears to provide useful regularization; this single run does not
prove a universal causal benefit. It does rule out the outer stack as the
primary explanation for the learning failure. Intrinsic geometry is also not
enough: effective-rank fraction improved from `0.665276` to `0.713172` and
conditioning improved, while task-aligned decodability worsened.

The next clean representation test was therefore to restore
`light_phase_safe_v2` and disable only the internal VICReg weak views while
keeping masks, losses, seed, budgets, splits, and both ridge gates fixed. That
controlled result is recorded below. The final `[1088,1170)` holdout remains
sealed.

## 2026-07-14 Hidden VICReg Weak-View Augmentation-Off Ablation

This second fixed-seed control restored the canonical outer augmentation and
changed only the model-internal VICReg view perturbations:

```text
Gaussian jitter:       0.005 -> 0
time-dropout scale:    0.10  -> 0
effective dropout:     0.01  -> 0
```

The zero-effect path still consumed the legacy time-uniform and Gaussian RNG
draws, so the intervention preserved the random schedule. Channel masking was
already zero, making the internal feature-drop arm inactive. Every loss,
mask, optimizer setting, seed, outer augmentation, and training budget stayed
fixed. Unit coverage verified both legacy-default output/RNG parity and
zero-effect identity/RNG parity.

The new representation completed 3,000 steps, and its matched MDN completed
3,500 while the representation remained frozen. Both stages were finite and
strictly used `[0,730)`. Frozen capture again produced 6,570 train rows and
2,952 historical-diagnostic rows per surface, with exact checkpoint identities,
zero optimizer steps, no mutation, raw width 96, post-MDN width 400, and no
anchor at or above 1088. All stage, capture, ridge, and master seals verified.

Values below are `direction / rank / correlation / RMSE`:

| Surface and split | Canonical weak views | Hidden weak views off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.688737 / 0.694064 / 0.550651 / 0.024272` | `-0.041857 / -0.006849 / -0.027450 / +0.000661` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.724255 / 0.708333 / 0.610111 / 0.022026` | `-0.018970 / -0.014566 / -0.016950 / +0.000332` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.805936 / 0.785388 / 0.682145 / 0.020944` | `+0.000000 / -0.008371 / -0.005075 / +0.000108` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.798780 / 0.778794 / 0.753269 / 0.018278` | `-0.012195 / -0.007114 / +0.012936 / -0.000415` |

Both surfaces kept `alpha=1e-10`, both failed the unchanged `0.95/0.95` gate,
and both selected outputs replayed byte-for-byte. The post-MDN historical
correlation and RMSE improved slightly, so the metric effect is not uniform.
The task-defining direction/rank result is nevertheless consistent: disabling
the hidden perturbations worsens raw signal and does not improve post-MDN
signal. They are not the primary bottleneck and may provide modest useful
regularization at seed 17. This one seed does not prove a universal benefit.

The representation trajectory also barely changed: canonical versus control
mean loss was `0.241364` versus `0.240950`, retention stayed exactly
`0.864117`, and effective-rank fraction moved only `0.665276` to `0.669935`.
The ablation therefore did not expose a hidden training instability. More
importantly, the global VICReg term still contributed about `0.197399`, or
82% of the mean `0.240950` objective after weighting. It is the dominant
remaining single upstream pressure.

A post-run machinery audit found no issue that invalidates these artifacts.
It did identify an incomplete hermeticity claim: the ridge source and header
were sealed, but the helper executable and its dynamic libraries were not in
the launch manifest. The two observed helper binaries were byte-identical
(`157de80b...`), and both main/replay outputs matched exactly. The current MDN
manifest also explicitly records the representation as frozen, although the
runner's verifier should assert that field in future experiments. These are
future provenance/verifier hardening items, not observed confounds in this run.

The next narrow test should keep both augmentation layers active and set only
`LAMBDA_VICREG=0.05 -> 0`, while leaving the VICReg branch enabled so its
forwards and RNG schedule remain intact. That directly tests the dominant
gradient pressure without mixing in a branch, mask, optimizer, or readout
change. The final `[1088,1170)` holdout remains sealed.

## 2026-07-14 Aggregate VICReg Gradient-Off Ablation

The next fixed-seed control made exactly that one-line change:

```text
LAMBDA_VICREG: 0.05 -> 0
```

The VICReg branch remained enabled. It still constructed both canonical weak
views, consumed their time-uniform and Gaussian RNG draws, encoded both views,
ran the stability projector, computed the similarity/variance/covariance
terms, and reported the raw global aggregate. Outer augmentation remained
`light_phase_safe_v2`; hidden time drop remained `0.01`; hidden Gaussian
jitter remained `0.005`; and masks, model structure, optimizer, seeds, budgets,
and splits were unchanged. The intervention therefore removes the aggregate
VICReg gradient contribution without changing the branch or data stream.

The control representation completed 3,000 finite steps and its matched MDN
completed 3,500 while the representation remained frozen. Both used only
`[0,730)`. Frozen capture produced 6,570 train rows and 2,952 historical-
diagnostic rows per surface, with exact checkpoint identities, zero optimizer
steps, no mutation, raw width 96, post-MDN width 400, and maximum anchor 1087.
The runner sealed the helper executable, compiler, linker, normalized `ldd`,
dynamic section, resolved library hashes, captures, and ridge outputs before
jointly sealing the final report and master manifest. Every seal verified.

The representation objective changed sharply, but the change did not expose
an instability. Canonical versus control mean loss was `0.241364` versus
`0.042310`; the difference is almost exactly the removed weighted global
VICReg contribution (`0.197836`). The control still computed a raw global
aggregate of `24.7341`, but its coefficient was zero. Maximum pre-clip gradient
norms were `3.08042` and `3.09976`, both below the unchanged clip threshold 5,
so global clipping did not confound this run.

Geometry improved substantially when the gradient was removed:

```text
effective-rank fraction: 0.665276 -> 0.715283
condition number:        94.1924  -> 31.1743
isotropy score:          0.014639 -> 0.0373614
```

That geometry improvement did not improve task-aligned decodability. Values
below are `direction / rank / correlation / RMSE`:

| Surface and split | Canonical VICReg | VICReg gradient off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.729072 / 0.701674 / 0.556664 / 0.023451` | `-0.001522 / +0.000761 / -0.021438 / -0.000161` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.741531 / 0.695799 / 0.602476 / 0.022208` | `-0.001694 / -0.027100 / -0.024585 / +0.000514` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.799087 / 0.755708 / 0.658574 / 0.021759` | `-0.006849 / -0.038052 / -0.028646 / +0.000922` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.786924 / 0.762195 / 0.722576 / 0.019257` | `-0.024051 / -0.023713 / -0.017756 / +0.000564` |

The raw selector moved from canonical `alpha=1e-10` to `1e-9`. A predeclared
fixed-`1e-10` control reaches validation `0.728311/0.702435` and historical
`0.742886/0.697493` direction/rank, so the historical rank loss persists when
alpha is held fixed. Raw selected, raw fixed-alpha, and post-MDN outputs each
replayed byte-for-byte. Both selected surfaces still fail the unchanged
`0.95/0.95` gate.

The matched production direct head remains a separate failure. Its final
direction/rank is `0.515033/0.509114` with correlation `-0.001655`, essentially
the canonical chance result despite ordinary loss reduction and parameter
updates. Aggregate VICReg removal does not rescue it.

At seed 17, the aggregate VICReg gradient is therefore not the primary
bottleneck and appears useful for task decodability, especially after the MDN
context adapter. More broad augmentation or VICReg removal is not supported.
The result also repeats an important warning: latent rank, conditioning, and
isotropy can improve while the actual forecasting signal worsens.

The active MTF launcher records only aggregate global VICReg loss. Although
the model computes `vicreg_sim_term`, `vicreg_var_term`, and `vicreg_cov_term`,
their means are not accumulated into this launcher report. This experiment
therefore cannot identify a component cause, and the encoder `latent_std`
cannot be substituted for projector-space VICReg variance. Any later component
study should first add no-math-change telemetry and a train-only shared-encoder
gradient census rather than launching component-off arms from loss magnitude
alone.

The priority now shifts back to the confirmed production-readout gap: retain
canonical augmentation and `LAMBDA_VICREG=0.05`, then compare a minimal frozen-
feature affine production readout against the closed-form decoder on the same
train-selected surface before paying for more representation retraining. The
remaining representation/context ceiling near `0.81/0.79`, still below
`0.95/0.95`, should be addressed after that readout path is made honest. The
final `[1088,1170)` holdout remains sealed.

## 2026-07-14 Actual-Train-Range Affine Optimizer Parity

The recommended frozen-feature control is now complete. It reused the exact
canonical post-adapter MDN captures, kept representation and MDN checkpoints
frozen, and compared three readout paths on the real development range
`[0,730)` with historical diagnostic confirmation on `[760,1088)`:

1. the unchanged production `DirectEdgeReturnHead`, trained with the existing
   Adam and `100/5/5` regression/direction/rank objective;
2. a registered, zero-initialized, standardized per-edge `nn::Linear` head,
   trained with the same Adam objective; and
3. the train-only-selected float64 analytic ridge decoder.

The Adam settings were fixed before confirmation and were not selected on the
historical range. Ridge retained its purged inner split: fit `[0,554)`, purge
`[554,584)`, validation `[584,730)`, then refit `[0,730)`. Values are
`direction / rank / RMSE`:

| Readout and split | Direction | Rank | RMSE |
|---|---:|---:|---:|
| Production head, development | `0.483257` | `0.512938` | `0.028359` |
| Production head, historical confirmation | `0.481030` | `0.508130` | `0.028395` |
| Trainable affine Adam, development | `0.543836` | `0.553425` | `0.035386` |
| Trainable affine Adam, historical confirmation | `0.537940` | `0.550813` | `0.035622` |
| Analytic affine, refit development | `0.841705` | `0.824962` | `0.016640` |
| Analytic affine, historical confirmation | `0.810976` | `0.785908` | `0.018693` |

This closes the earlier range-mismatch caveat. The trainable affine Adam path
is not failing only on the small protected-evaluation slice: on the actual
training range it trails the representable analytic solution by about `0.298`
direction and `0.272` rank, with `2.13x` RMSE. On historical confirmation the
gaps remain about `0.273`, `0.235`, and `1.91x`. Its final composite loss also
rose from `65.7087` to `70.5290`. The nonlinear production head is worse than
the trainable affine and remains at chance.

The decisive control then copied the analytic refit's fit-only mean, inverse
standard deviation, per-edge coefficients, and biases into fresh registered
`nn::Linear` modules. Those modules computed on CPU float64 and returned the
same float32 contract as the analytic head. The maximum prediction difference
was `4.55e-13` on development and `4.66e-10` on historical confirmation.
Direction and rank were identical; RMSE differed only by `1.80e-16` and
`6.34e-14`. `torch::equal` is false because GEMM and elementwise reductions
accumulate floating-point terms in a different order, not because the
parameterization or feature reconstruction differs.

The registered affine parameterization and cached feature wiring are therefore
sound. The immediate failure is the current Adam plus production-objective
training path: it does not find a solution the same affine module can express.
This experiment does not separate Adam from the composite objective, does not
install the affine module into the live MDN checkpoint route, and does not
establish a universal upstream ceiling: it shows only that affine decodability
remains around `0.81/0.79`, below the `0.95/0.95` gate. Whether an honest
nonlinear readout can exceed that result remains open.

Both full GPU baselines replayed byte-for-byte, as did both coefficient-copy
runs. The hardened runner seals its compiler, binary, production header,
linked libraries, exact options, outputs, and a content manifest covering all
extant files in the two legacy capture directories plus their checkpoints.
The captures predate modern stage sealing and the original temporary train
config no longer exists, so their authority is explicitly
`verified_legacy_content_manifest_not_original_stage_seal`; no stronger claim
is made. Preflight revalidated 6,570 and 2,952 unique finite rows of width 400,
with maximum anchor 1087. `[1088,1170)` remains sealed.

The next useful step is not another broad representation ablation. First make
the readout smoke test honest with a deterministic, train-only calibrated
affine sidecar that can be exported and reloaded through the exact post-adapter
feature contract. Then test objective/optimizer repairs against the explicit
requirement of staying within `0.01` direction/rank and `5e-4` RMSE of the
analytic solution. Honest nonlinear-readout and upstream-feature work can then
be separated cleanly against the remaining `0.95/0.95` gap.

## 2026-07-14 Exportable Diagnostic Affine Calibration Sidecar

That deterministic smoke-test artifact is now implemented and sealed. It is a
new diagnostic-only `PerEdgeAffineReturnHead`, not a replacement for the live
MDN head or a policy input. The module registers development-fit float64 mean
and inverse-standard-deviation buffers plus one frozen `nn::Linear` per edge.
It accepts either:

1. post-`DirectEdgeReturnHead`-adapter node context `[B,E+1,C,H]`; or
2. the exact cached readout tensor `[B,E,C,F]`.

For the cached form it consumes only the first `3H=384` values in the declared
`[base, quote, base - quote]` layout and ignores the 16-value identity suffix.
The comparator now retains both surfaces and requires the cached prefix to be
bit-exact with the prefix reconstructed from context. It was exact across all
1,058 inspected anchors (`max delta = 0`), closing the remaining feature-
boundary ambiguity.

The archive contains strict root metadata and a nested model state. Its
metadata records only development selection `[0,554)`, purge `[554,584)`,
validation `[584,730)`, refit `[0,730)`, and `valid_from=730`, along with graph,
edge/node, capture, probe, and checkpoint identities. It contains no historical
confirmation or final-holdout field. A canonical semantic digest binds those
metadata semantics to big-endian IEEE-754 bytes for the mean, scale, weights,
and biases.

The loaded sidecar preserves the analytic result:

| Split | Direction | Rank | RMSE | Maximum analytic prediction delta |
|---|---:|---:|---:|---:|
| Development refit `[0,730)` | `0.841705` | `0.824962` | `0.016640` | `4.55e-13` |
| Historical confirmation `[760,1088)` | `0.810976` | `0.785908` | `0.018693` | `4.66e-10` |

Direction and rank deltas are exactly zero; RMSE deltas are below `6.4e-14`.
The copied source state and reloaded state are `torch::equal`, parameters remain
frozen, canonical metadata round-trips exactly, and context, exact-readout,
in-memory, and reloaded predictions are bit-exact with one another. The full
loaded prediction digest is
`bae8b301a05c50777c311dcfe8e539dd851b0098b66408db9bd0c7cf28635284`;
the semantic state digest is
`b0f3f00760d92d3026ed8675c9d6f572dd1c0b5de6f1c45bbd8b9253f99fd709`.

Main and replay result probes are byte-identical, as are their canonical
metadata files. The two `.pt` files are intentionally not required to be byte-
identical because Torch archive internals include the differing archive
basenames. Their physical hashes differ, while semantic state, exact loaded
state, metadata, and every loaded prediction digest agree. This is the correct
acceptance boundary for this archive format.

Focused tests cover shape/dtype/device and nonfinite rejection, frozen state,
analytic formula parity, exact prefix/suffix behavior, save/load, canonical
digesting, and corrupted metadata/state rejection. During the first integrated
run they exposed a mixed-dtype archive-reader bug caused by reusing one tensor
across integer and float metadata fields; the loader now creates a fresh tensor
for every field, and the corrected round-trip test passes.

The hardened runner froze its comparator, runner, full local header closure,
compiler, binary, dynamic libraries, options, inputs, pair artifacts, and
outputs before atomic installation. Every manifest verified. It revalidated
6,570 development rows and 2,952 historical-diagnostic rows, with maximum
anchor 1087. The legacy authority remains
`verified_legacy_content_manifest_not_original_stage_seal`; no stronger
provenance claim is made. `[1088,1170)` remains sealed.

This step makes the analytic decoder operational and reproducible; it does not
improve the feature ceiling, repair the production training objective, or wire
the sidecar into MDN/policy execution. The next narrow experiment can now use
the frozen sidecar as a truth oracle in a predeclared objective ladder while
holding Adam, initialization, batches, seed, and frozen features fixed: raw-
target MSE first; then target scaling plus Huber regression; then direction;
then rank. Each arm keeps the already declared `0.01` direction/rank and
`5e-4` RMSE closeness requirements and must not tune on historical
confirmation. If raw-target MSE already fails, the next branch is optimizer,
batching, and conditioning rather than another representation ablation. Only
after this isolation should an honest nonlinear readout or upstream feature
change be judged against the still-open `0.95/0.95` gap.

## 2026-07-14 Frozen-Affine Objective Ladder and Recovery Matrix

The predeclared frozen-feature objective ladder is complete. It used the exact
sealed post-adapter development capture and frozen affine sidecar, without a
policy path. Selection stayed inside development: fit `[0,554)`, purge
`[554,584)`, and validation `[584,730)`. A selected arm would have been refit
on `[0,730)` before the already-consumed historical diagnostic `[760,1088)`
was opened once. No arm passed, so no refit occurred, historical confirmation
was not opened, and the final `[1088,1170)` holdout remains sealed. The maximum
loaded anchor is 729.

The canonical runner recursively hashed the development capture, pinned
checkpoints, split contract, ridge reference, and frozen sidecar. It carried
the historical probe only as a predeclared conditional CLI pathname. It did
not require, parse, validate, hash, or seal that raw historical content before
selection. Because no candidate passed, the helper never opened it. This
result therefore proves non-access; it makes no new claim about the current
integrity of that previously consumed historical file.

The fit surface contains 4,986 finite edge/channel targets and validation
contains 1,314. Every arm started from the same bit-identical zero affine
state. Mini-batch arms used the frozen seed-2031 production sampler: 3,500
updates, 389 epochs, and 215,464 sampled-anchor visits. The one-sided gate
allowed at most `0.01` direction shortfall, `0.01` rank shortfall, and
`0.0005` RMSE excess relative to the fit-only float64 analytic ridge oracle.

Two controls show that this is not a module-copy or gross CUDA
representability failure. The analytic selection oracle reached
`0.805936 / 0.793760 / 0.020837` direction/rank/RMSE. Its independently
remapped and frozen CUDA-float32 affine copy reached
`0.806697 / 0.794521 / 0.020834` and passed all three parity gates. Across the
full development range, the sealed CPU sidecar and its CUDA-float32 copy
reached respectively `0.841705 / 0.824962 / 0.016640` and
`0.842009 / 0.826180 / 0.016643`. State copying was exact; their maximum
prediction delta, `5.969e-4`, is recorded descriptively as CPU-float64 versus
CUDA-float32 accumulation.

Validation results are:

| Arm | Role | Direction | Rank | RMSE | Correlation | Gate |
|---|---|---:|---:|---:|---:|---|
| Float64 analytic ridge | reference | `0.805936` | `0.793760` | `0.020837` | `0.687220` | reference |
| P1 frozen CUDA oracle copy | control only | `0.806697` | `0.794521` | `0.020834` | `0.687282` | pass |
| L0 raw MSE, mini-batch Adam | candidate | `0.574581` | `0.588280` | `0.028125` | `0.204108` | fail |
| L1 scaled Huber | descriptive | `0.513699` | `0.528919` | `0.033824` | `0.120945` | fail |
| L2 scaled Huber + direction | descriptive | `0.514460` | `0.515982` | `0.031274` | `0.144651` | fail |
| L3 scaled Huber + direction + rank | descriptive | `0.508371` | `0.528919` | `0.034331` | `0.106905` | fail |
| B2 raw MSE, full-batch Adam | candidate | `0.559361` | `0.570015` | `0.028043` | `0.243436` | fail |
| B3 raw MSE, PCA-whitened mini-batch Adam | candidate | `0.771689` | `0.767884` | `0.022291` | `0.619904` | fail |
| B4 raw MSE, full-batch LBFGS | candidate | `0.700913` | `0.694064` | `0.024414` | `0.501197` | fail |

Raw MSE already fails, so the production-style scaled objective cannot be the
only cause. L0 clipped zero updates, yet its final full-gradient norm grew from
`0.007955` to `0.158535`. The conditional no-clip arm was therefore correctly
inapplicable. Full-batch Adam also clipped zero updates and ended at
essentially the same failure, ruling out mini-batch noise as the sole cause.

The scaled-Huber arms expose a second, compounding problem. Their initial
full-gradient norms jumped to `533.5`, `549.0`, and `573.7`; every one of their
3,500 updates clipped; and their final full-gradient norms were approximately
`14,344`, `12,926`, and `16,568`. Target scaling, Huber regression, direction,
and rank therefore do not repair this affine smoke test under the current
weights and clip setting. Since L0 itself fails, adjacent L1/L2/L3 differences
remain descriptive rather than a clean causal decomposition.

Conditioning is the strongest measured boundary. Each edge has 1,662 fit
observations and 384 dynamic coordinates. The raw surface retains only 26
eigen-directions per edge at the declared threshold, with nullity 358, stable
rank near `1.122`, effective rank near `1.410`, retained condition numbers
from `6.68e9` to `8.68e9`, maximum off-diagonal correlation above
`0.99999998`, and 99th-percentile absolute correlation above `0.9997538`.
Shared standardization raises retained ranks only to `121/119/120`; stable
rank remains `1.634-1.642`, effective rank `2.051-2.087`, and retained
condition numbers remain `9.50e9-9.90e9`.

PCA whitening is consequently the strongest tested training intervention. B3
improved to `0.771689 / 0.767884 / 0.022291`, but its remaining oracle
shortfalls were `0.034247` direction, `0.025875` rank, and `0.001454` RMSE,
all outside the declared limits. Its whitening covariance checks were clean
(maximum diagonal error `1.02e-4`, maximum off-diagonal magnitude `3.34e-4`
against a `0.05` ceiling), but collapsing the explicit transform back to raw
CUDA-float32 affine weights produced a `2.650e-5` maximum prediction delta,
slightly above the independent `2e-5` deployability gate. Full-batch LBFGS
reduced the full-gradient norm from `0.007955` to `0.000591`, but exhausted all
500 iterations and 523 closure evaluations without oracle parity.

The near-random result therefore contains two separable failures:

1. the current affine training path does not recover signal that the same
   registered CUDA affine parameterization can represent; extreme feature
   collinearity is the strongest measured cause, while the scaled composite
   objective aggravates optimization; and
2. even the analytic affine oracle remains near `0.81/0.79`, below the
   benchmark's `0.95/0.95` requirement, so upstream feature quality remains a
   real second problem after the readout optimizer is repaired.

This experiment rules out raw-MSE gradient clipping, mini-batch stochasticity
alone, missing direction/rank terms, and registered affine copy/wiring as the
immediate explanation. It does not itself make a new causal statement about
augmentation. The separate outer-augmentation-off, hidden-weak-view-off, and
aggregate-VICReg-gradient-off experiments already failed to improve the
task-aligned signal.

Three independent production defects were repaired during this diagnosis:
checkpoint persistence of the private warmup optimizer-step index, close-slot
resolution through reordered `target_coords`, and train-equivalent scheduled-
NLL accounting in run/eval. Focused production-path and launcher tests pass.
None caused this frozen result: the ladder starts fresh without checkpoint
resume, its captures use the identity close slot, and the affine harness
bypasses MDN eval-objective accounting.

Main and replay are byte-identical at
`1b43e31eae93a8a88ebe07b7ac9c7ab23b4881bf4b101f60566694286f8b4c7f`;
both success logs are empty. The exact validator pins all 1,109 records and
rejects an expanded historical key surface when selection is absent.

The next experiment stays on this frozen surface and has three ordered
controls. First, compute the float64 analytic ridge ceiling in B3's exact
retained PCA basis to measure truncation loss. Second, train that explicit
uncollapsed basis with full-batch raw-MSE Adam and LBFGS against its own
analytic ceiling. Third, compare ordinary-coordinate ridge optimization with
a full-rank damped eigenspace preconditioner using the same `alpha=1e-10`
objective, all 384 coordinates, mapped-weight penalty, and unpenalized bias.
The third control separates coordinate conditioning from PCA truncation.
Historical confirmation remains conditional on the first predeclared passing
candidate, and `[1088,1170)` remains sealed.

## 2026-07-14 Frozen-Affine Conditioning-Parity Diagnostic

The ordered conditioning diagnostic is complete on the same sealed
post-adapter development surface. It used fit `[0,554)`, purge `[554,584)`,
and validation `[584,730)`, loaded no anchor above 729, and left the final
`[1088,1170)` holdout sealed. The experiment is diagnostic-only, contains no
policy path, and does not grant benchmark-acceptance authority.

The canonical design is the cached 384-coordinate float32 dynamic-feature
surface, normalized from fit anchors only with shared float32 statistics,
materialized in float32, and then promoted to float64 for the analytic
solves. Every fitted method minimizes the same raw-target ridge objective:
data MSE plus `alpha / edge_count` times the squared norm of the weights after
mapping to canonical standardized coordinates, with `alpha=1e-10` and an
unpenalized bias. Main and replay used one CPU thread, deterministic CUDA, and
seed 31.

The analytic rows below are fit-objective noninferiority references. They
minimize their declared objective on `[0,554)` and are then measured on
`[584,730)`; they are not claims about the best possible validation metrics.
The separately sealed affine sidecar was fitted on all `[0,730)` development
anchors. Its `0.848554 / 0.834094 / 0.016803` result on the `[584,730)` slice
is therefore descriptive and in-sample, and it is not used by any gate.

| Method | Gate reference | Direction | Rank | RMSE | Objective result | Raw-f32 collapse |
|---|---|---:|---:|---:|---|---|
| Full analytic, all 384 modes | fit-objective reference | `0.805936` | `0.793760` | `0.02083634` | reference | fail, `1.814e-4` |
| R0 retained PCA analytic, ranks `121/119/120` | full analytic | `0.765601` | `0.756469` | `0.02251255` | operational fail | pass, `1.701e-5` |
| R1 retained PCA, full-batch Adam | R0 retained analytic | `0.762557` | `0.754947` | `0.02255043` | scientific pass | pass, `1.756e-5` |
| R1 retained PCA, full-batch LBFGS | R0 retained analytic | `0.765601` | `0.756469` | `0.02251255` | pass in 3 iterations | pass, `1.901e-5` |
| R2 ordinary full-rank LBFGS | full analytic | `0.694064` | `0.691781` | `0.02452816` | fail after 500 iterations | pass for this failed state |
| R2 damped full-rank LBFGS | full analytic | `0.805936` | `0.793760` | `0.02083634` | pass in 3 iterations | fail, `1.654e-4` |

R0 measures PCA truncation independently of optimizer failure. Its exact
analytic result misses the full-reference operational gate by `0.040335`
direction, `0.037291` rank, and `0.001676` RMSE. Its fit objective rises from
`2.705303e-4` to `3.405849e-4`. The retained basis therefore discards
material task signal; whitening only those retained coordinates cannot fully
recover the all-mode result, even with an exact optimizer.

The explicit retained-coordinate controls answer the optimizer question for
that reduced surface. Adam's relative same-surface objective gap is
`2.126e-4`; it stays inside all three scientific tolerances and its raw-f32
collapse passes at `1.756e-5`. LBFGS reaches a `5.43e-12` relative objective
gap in three iterations and seven closures, with a `2.21e-7` maximum
prediction delta from the materialized analytic reference. Its optimizer-
surface gradient norm is `4.97e-8`, and its raw-f32 collapse narrowly passes
at `1.901e-5` against the independent `2e-5` deployment tolerance.

The matched full-rank comparison isolates coordinate conditioning on this
affine development diagnostic. Ordinary-coordinate LBFGS exhausts all 500
iterations and 533 closures, retains an `0.8746` relative objective gap, and
misses the scientific gate. The damped eigenspace coordinate system keeps all
384 modes, uses the same objective, initialization, solver, and materialized
surface, but reaches a `3.23e-14` relative objective gap in three iterations
and three closures. Its final optimizer-surface gradient norm is `3.80e-9`,
its maximum prediction delta from its analytic reference is `1.86e-8`, and it
matches the full fit-objective reference on every validation metric.
Coordinate conditioning therefore causally closes the measured full-rank
LBFGS optimizer gap within this experiment. It is not a causal claim about the
remaining upstream representation-quality gap.

Float32 materialization of the analytic targets is too small to explain that
ordinary-versus-damped difference. The ordinary materialized analytic result
differs from the full analytic validation prediction by at most `1.76e-6`;
the damped materialized result differs by at most `7.45e-9`. Likewise, the
full analytic transformed-coordinate runtime prediction has only `3.64e-9`
maximum quantization error. The failure is optimization in ordinary
coordinates, not loss of the target solution during direct transformed-
coordinate float32 evaluation.

Collapsing a successful full-rank transformed solution into raw float32
affine weights is a separate numerical boundary. The full analytic collapse
changes development predictions by at most `1.814e-4`, and the successful R2
damped arm changes them by `1.654e-4`; both exceed the `2e-5` deployment
tolerance. The ordinary arm's `1.91e-6` collapse passes only because that arm
stopped far from the high-weight analytic solution. This is not evidence that
the desired full-rank solution can safely use raw float32 collapsed weights.
The retained R1 solutions do pass, but with little margin.

The eigenspectrum explains both observations. All three standardized edge
designs are numerically rank 384 at the SVD tolerance, but only
`121/119/120` modes exceed the retained threshold. Per edge, `225/225/226`
modes lie at or below `alpha`, another `38/40/38` lie between `alpha` and the
retention threshold, and only `121/119/120` lie above it. The small modes are
difficult optimizer directions and have tiny raw target-coupling energy, yet
their ridge-weighted objective contribution and the retained-versus-full
analytic loss are operationally important. Discarding them loses validation
signal; preserving and damping them solves the optimizer problem but produces
mapped weights whose raw-float32 collapse is cancellation-sensitive.

Optimizer-surface KKT residuals and same-surface objective gaps are the
convergence authority. The separately reported full-canonical-surface KKT
residual includes omitted modes and float32 materialization error, so it must
not be read as the gradient of a reduced or transformed optimizer surface.

The result sharpens, but does not erase, the two-layer diagnosis. Conditioning
is causal for the measured affine optimizer failure and now has an effective
solution in explicit damped coordinates. Yet the full fit-objective reference
still reaches only about `0.806/0.794`, below the benchmark's `0.95/0.95`
requirement. Upstream feature quality remains an independent open problem.
This experiment makes no new causal statement about augmentation.

Main and replay probes are byte-identical at
`1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1`;
both success logs are empty, and the exact validator pins all 608 keys. The
canonical master-manifest file is sealed at
`414242616aa71e74aaa5812a05d620073099b26cb7c014a0bc3b75eeb480329e`.
The durable report is
`src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_affine_conditioning_parity_v1.report`.

The next implementation experiment should preserve the centered damped
coordinate parameterization explicitly, or establish a higher-precision
collapse contract, then retest the actual forecasting path before changing
the representation training recipe. Once optimizer parity and deployment are
both preserved, an honest nonlinear readout can test whether the remaining
`0.95/0.95` gap belongs to affine capacity or upstream features.

## 2026-07-15 Frozen-Affine Deployment-Bridge Diagnostic

The deployment follow-up is complete on the same development-only surface.
It consumed anchors `[0,730)` and no others: fit `[0,554)`, purge
`[554,584)`, and validation `[584,730)`, with maximum loaded anchor 729. The
final `[1088,1170)` holdout remains sealed. No policy path was used, and the
new v2 artifact is explicitly run-only and policy-ineligible.

This experiment starts from the successful full-rank damped analytic state
from the conditioning diagnostic. That analytic state was reconstructed
exactly, including its prediction digest and `2.70530270559889488e-4`
fit-objective value. The trained damped arm is at most `1.862645149e-8` away
from the analytic reference over development, so the bridge is materially
connected to the optimized state rather than to a different fitted solution.

Four live arithmetic paths were evaluated against the canonical materialized
damped-float64 prediction:

| Runtime path | Maximum development delta | `2e-5` gate |
|---|---:|---|
| Uncentered mapped weights and bias in float32 | `1.5605241e-4` | fail |
| Centered mapped weights and bias in float32 | `1.5287660e-4` | fail |
| Explicit damped transform and theta in float32 | `2.1270663e-4` | fail |
| Fit-only float32 normalization, then centered mapped float64 accumulation | `1.6942620e-5` | pass |

Centering alone is therefore insufficient in float32, and retaining the
explicit transform does not help when all transformed arithmetic is also
float32. The matched centered-mapped comparison isolates the effective
repair: keep the existing float32 feature normalization, promote the
normalized values on the same device, then subtract the edge center and
perform the mapped-weight dot product plus bias in float64 before casting the
output to float32. That path passes on both the full development range and
validation.

The pass is narrow. Its maximum delta uses `84.7131%` of the tolerance and
leaves only `3.05738e-6` absolute headroom. This is enough for the declared
canonical gate, but it is not evidence of generous numerical margin across
unmeasured hardware or runtime changes.

The v2 archive exercises the actual public runtime contract rather than only
an offline tensor expression. The in-memory and reloaded semantic digest is
`84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38`.
CPU versus CUDA differs by at most `1.4551915e-11` over development and is
exact on validation. Full versus 37-anchor chunked execution, in-memory
versus archive reload, and CPU/CUDA suffix-perturbation controls are all
exact. The ignored 16-value suffix was replaced by alternating quiet NaNs and
positive infinities without changing predictions, confirming that only the
first 384 readout values participate.

This is a causal deployment-arithmetic result on a frozen affine development
surface: the centered float64 accumulation contract carries the desired
full-rank conditioned solution across archive, device, and chunk boundaries
where the tested float32 contracts fail. It does not show that upstream
representations are good enough, recover the production nonlinear forecasting
path, measure policy behavior, resolve augmentation causality, or raise the
affine ceiling above its roughly `0.806/0.794` direction/rank result.

Main and replay probes are byte-identical at
`00a382d060d20d7a8277d9f120fdc6578c572072264847157424c8a09207d3fb`;
the exact validator pins 504 keys with key-set digest
`275b86d4ece31ec00bbb9591724d439335d9de5f9a72a99fe84d0fd1abc82f6b`.
The main/replay archives are also byte-identical at
`7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739`,
both success logs are empty, and the canonical master-manifest file is sealed
at `467c69c9a05035c9b58f26d90e29df127fdc11dec36f4e7deed8c7b905b23a91`.
The frozen helper, public v2 header, runner, and helper binary are respectively
bound at `03d264055115b0973475a9e4a1974f88dd9ed83a4a08c67e17641374344e2c71`,
`69d633d604318fba16813f48bd539465be7e58c13e5d517cfc32b04f8c1599de`,
`e0bf779989cfc744d6abb86f8160f97dd0bbc1d48ade9164b7bd66b4a4a5057b`,
and `57bf4b90fc1de3f78be51ce0a025c3c3a75ac1169732d5d3186897b819e8d703`.

The durable report is
`src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_affine_deployment_bridge_v1.report`.
The next experiment should connect this run-only artifact to the frozen
forecasting path and test the real composite forecasting objective, still
without policy or final-holdout access.

## 2026-07-16 One-Shot Final-Holdout Infrastructure Failure

The preregistered `[1088,1170)` launch did not produce a scientific result.
The no-clobber ledger records `created_utc=2026-07-16T12:12:05Z` and was then
durably published, after which the frozen production executable entered
source initialization and exited with status 1 before it created the capture
job. Under the frozen contract, the 82-anchor interval is now terminally
classified here as consumed without capture. The official ledger status is
`capture_authorized_interval_conservatively_consumed`; recapture is forbidden,
and evaluate-only recovery requires complete immutable capture artifacts
sufficient to validate or create `capture.complete`.

This is a validity/infrastructure outcome, not a failed model gate. The
capture directory, representation probe, MDN probe, channel report,
`job.manifest`, `runtime.result.fact`, `capture.complete`, selected evaluation,
runtime result package, and installed final report are all absent. Therefore
zero final-holdout prediction rows were durably materialized, neither the
checkpoint nor conditioned candidate was scored, and none of the seven
scientific gates has an observed final value. The protocol conservatively
marks the interval consumed at ledger publication even though there is no
evidence that wave or anchor evaluation began.

The external and vault ledger paths are the same read-only inode with link
count 2. Their common SHA-256 is
`e5a1b2d8b7a4927d46a7536fb0332afdcd92d52b877ace2340c75e0bc6f0f59d`.
The ledger records `anchor_range=[1088,1170)`, `anchor_count=82`,
`expected_rows=738`, `capture_policy=exactly_once`,
`recapture_permitted=false`, and
`resume_policy=evaluate_from_capture_complete_only`. The observed postmortem
SHA-256 of the retained production failure log is
`a4a3e370e941d47a5e3559e40f583fe87ce2b5e51ed6625d8eea91c764f1035e`;
the corresponding observed SHA-256 of the generated capture config is
`23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0`.

The failure cause is exact. The live `SYNBETASYNUSD/1d` raw and normalized
caches were byte-identical to their frozen copies, but `cp -a` across the
mounted filesystem collapsed their subsecond modification times:

| File | Live mtime | Frozen mtime | SHA-256 |
|---|---|---|---|
| Raw cache | `00:15:49.062546800` | `00:15:49.000000000` | `bf43643f6cd59c1426250e10f143b37898728da1b769503067e2b4269923017f` |
| Normalized cache | `00:15:49.957121500` | `00:15:49.000000000` | `6db1f38d8fefe7025e4be3ad59de6c1abcb404e00f1ff3640633716921f83c20` |

The current source loader accepts a normalized cache only when its mtime is
strictly newer than the raw cache. Equality is treated as stale, so the
executable attempted to copy raw to normalized. The frozen files had
already been sealed mode `0400` under mode-`0555` directories; the overwrite
failed with `Permission denied`. The preceding five cache chains had
whole-second ordering that survived copying; this was the first chain whose
freshness margin existed only within one second. The preflight and frozen manifests
hashed contents but did not validate this metadata-dependent no-write
predicate, so they could not detect the semantic change before ledger
publication.

The executed closure remains preserved in the vault. Its frozen runner,
helper source, production executable, and helper binary are respectively:

```text
dc05760a62d04ed28e44a5e0b9ab02e65e108e999c266999e323c43fd0c9eea0
1bd51f5f5fece2f75297b9a1c36d3365974096e9af4c9c3e801d28e76a8b93f0
c3bfb3df0ebfe23a0b2859bef2a6d4a4f607c60974dc2a96abe57a1927f92a48
1c7f231aac31be3093cfc0cc6fe559067fd33e9743fd5b72183b05bb6c2b441a
```

The immutable preregistration and consumed v1 runner were not edited after
the launch. No permission, timestamp, cache byte, or capture path in the
retained vault was repaired, and neither `--run` nor
`--resume-evaluate-only` was invoked again. That preserves the exactly-once
record rather than converting an operational failure into an unregistered
retry.

The scientific interpretation is correspondingly narrow. The final segment
cannot independently confirm or falsify conditioned-head generalization. Two
earlier qualifications still stand as development/adaptive evidence:

- On real-launcher validation `[584,730)`, the live conditioned shadow reached
  `0.806697` direction, `0.792237` pairwise rank, `0.686404` correlation, and
  `0.0208663` RMSE. Enabling it left canonical `log_pi`, `mu`, `sigma`, direct
  return, and the canonical report byte-identical. Comparison with the older
  sealed metric report was operationally equivalent, not exact; the maximum
  metric delta was `0.00152207` under a different batch partition.
- On repeatedly inspected historical `[760,1088)`, without refit, the fixed
  conditioned head reached `0.802507 / 0.780488 / 0.729269 / 0.019188`
  direction/rank/correlation/RMSE, versus
  `0.481030 / 0.508130 / -0.007598 / 0.028335` for the checkpoint head. All
  seven predeclared gates passed, but this interval is adaptive evidence and
  not an independent test.

This refines the audit's early ordering that treated representation/interface
loss as the primary failure. The strongest available evidence now localizes
the near-random production score to the learned direct readout/training path:
the same frozen live features support a fixed
conditioned affine readout near `0.80` direction and `0.78-0.79` rank. It does
not clear the upstream feature path. The best full affine development
reference remains around `0.806/0.794`, below the benchmark's `0.95/0.95`
requirement, so feature quality or affine capacity remains a separate gap.
The unavailable final result leaves independent final-segment temporal
generalization unresolved.

Postmortem hardening was added outside the consumed v1 closure. The shared
`cache_freshness.h` predicate preserves the production strict-`>` semantics,
rejects missing, non-regular, and symlinked chain members, and never writes.
The read-only `check_frozen_cache_freshness.cpp` CLI exposes that predicate to
future experiment runners. It passes all nine live cache chains and rejects
the retained frozen incident chain specifically as
`normalized_not_strictly_newer`, reproducing the failure without modifying
either tree. Their current source hashes are respectively
`71872156d233526c37f840788108249e88366e1172a0bb5c33a490b03c4825a4`
and
`3b6ae49d9fc28f4902efa1809114d909f0ea131cc695eaf7f016059b9f81cc84`.
The existing Torch-backed memory-mapped storage suite now covers equal-mtime
read-only rejection, strict-order acceptance, missing normalized cache, and
symlink rejection. It also proves the no-force production sanitizer selects a
strictly fresh read-only chain without changing its bytes, timestamps, or
permissions; the full target passes. The CLI is intentionally not wired into
the consumed v1 runner and therefore protects no existing ledger transition.
Compiling, sealing, hashing, and invoking it is a mandatory step for the next
runner rather than a retroactive repair.

The next scientific attempt must use a newly generated process/seed, a new
schema, and a newly sealed final interval. Before its ledger can exist, its
runner must compile, hash, and freeze the guard; invoke it over every copied
CSV/raw/normalized triple immediately after closure copying; seal the tree;
invoke the same guard again on the read-only clone; and prove in an
integration test that any failure leaves no ledger, vault, capture config, or
launcher invocation. Cache generation or refresh belongs before holdout
sealing. Equality must remain a failure until cache provenance uses content
digests or equivalent sidecars rather than mtimes. This candidate/interval
pairing cannot be retried; the interval cannot be scored again or used for
tuning.

## 2026-07-16 Fresh-Seed V2 Data-Predictability Gate

The fresh-seed successor is now generated, structurally validated, loaded by
the production graph source, and sealed without model scoring or a final
consumption ledger. Its preregistration was fixed before source generation or
metric inspection at
`bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56`.
The seed commitment is
`7d22bae27c203eeec9fb2147d63d1cbe55d9de056db7c048f74e4798849ee227`.

V2 corrects a data-design defect discovered during the reset: the v1 1d, 3d,
and 1w files reused the same numeric bar-index waveform and mostly changed
only their timestamps. V2 instead generates one 4,137-day deterministic
quasiperiodic master market and causally aggregates real, non-overlapping
3-day and 1-week OHLCV bars from it. The published source contains 4,126 1d,
1,376 3d, and 591 1w rows per instrument. An independent validator rebuilds
the committed SplitMix64 phases, daily process, and every aggregate, then
requires exact canonical 8-decimal rows and exact development-prefix byte
equality.

The timestamp-synchronized production loader authoritatively reports 4,096
accepted anchors. The fixed exposure contract is train `[0,2496)`, validation
`[2560,2816)`, certified development evaluation `[2880,3264)`, and final
holdout `[3328,4096)`, with 64-anchor purges between them. The physically
truncated development view contains only 3,294 1d, 1,098 3d, and 471 1w rows
per instrument; its highest target is anchor 3,263 / daily row 3,293.

The rebuilt production executable is bound at
`9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff`.
Its cache-building representation dry run resolved train `[0,2496)`, reported
zero optimizer steps, no checkpoint write, and no replay artifacts. All nine
CSV/raw-cache/normalized-cache chains passed strict freshness before and after
the data tree became read-only. The completed data-closure receipt is
`36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831`.

Only after that closure, a train-only order-24 ridge autoregression was run
twice against the dedicated development-prefix files. Main and replay reports
are byte-identical. It passed every preregistered gate on both untouched
development ranges:

| Range | Direction | Pairwise rank | Correlation | RMSE / target RMS |
|---|---:|---:|---:|---:|
| validation `[2560,2816)` | `1.000000` | `1.000000` | `0.999999995` | `0.00009848` |
| certified eval `[2880,3264)` | `1.000000` | `1.000000` | `0.999999995` | `0.00009811` |

This is not a trivial zero-control success. Lag-1 alone reached about
`0.940/0.932` direction/rank on both ranges, with correlation around `0.978`
and RMSE around `0.208-0.210` target RMS. The order-24 recurrence then removed
essentially all remaining error. The durable baseline report is
`src/config/benchmarks/synthetic_continuous_graph_v2/artifacts/synthetic_v2_data_predictability_baselines.v1.report`
at `b3ae5ed64e0ed40f33cda10653d3f197b2641a88a32d89e319db7e7b45925905`.

The scientific conclusion is narrow but decisive: on v2, the data is
sequential and readily forecastable from the declared causal history. A model
near chance cannot be explained by an inherently unforecastable synthetic
series. The next layer is the already motivated raw-history MDN/readout
isolation, followed by fixed probes on untrained and trained representations.
Those arms must use only the same development prefix; the final
`[3328,4096)` interval remains unopened and unauthorized.

## 2026-07-16 V2 Raw-History Production-MDN Isolation

The representation-bypass arm is complete. Its remaining choices were fixed
before execution in the supplemental diagnostic preregistration at
`de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8`.
The helper used the production `ChannelContextMdn` and production training
loss implementation, but constructed its context directly from the preceding
29 completed daily edge returns. It did not execute a representation forward,
load a representation or MDN checkpoint, open `data/raw`, or access policy
code. Effective fitting was `[1,2496)`; validation and certified development
were scored unchanged at `[2560,2816)` and `[2880,3264)`.

The deterministic CPU main and replay reports and logs are byte-identical and
their complete runtime manifests verify. The durable report is
`src/config/benchmarks/synthetic_continuous_graph_v2/artifacts/synthetic_v2_raw_history_supervised_isolation.v1.report`
at `b94763b06594572c6b2af05fc691e8884f12481651d8ab5c568db8895676d5ae`.

The no-representation results are:

| Arm/output | Range | Direction | Pairwise rank | Correlation | RMSE / target RMS |
|---|---|---:|---:|---:|---:|
| linear ridge | validation | `1.000000` | `1.000000` | `1.000000` | `0.00000016` |
| linear ridge | certified | `1.000000` | `1.000000` | `1.000000` | `0.00000016` |
| K=1 NLL, expectation | validation | `0.979167` | `0.984375` | `0.996020` | `0.090160` |
| K=1 NLL, expectation | certified | `0.973958` | `0.979167` | `0.995993` | `0.090336` |
| K=3 current objective, expectation | validation | `0.980469` | `0.980469` | `0.995252` | `0.137145` |
| K=3 current objective, expectation | certified | `0.975694` | `0.976563` | `0.995287` | `0.136575` |
| K=3 current objective, direct output | validation | `0.979167` | `0.975260` | `0.990697` | `0.269321` |
| K=3 current objective, direct output | certified | `0.972222` | `0.969618` | `0.990359` | `0.270985` |

K=1 and both K=3 mixture-expectation ranges pass every strong capability
gate. The preregistered K=3 primary direct output misses the complete gate
only because its RMSE ratio is `0.269-0.271` rather than at most `0.25`; its
direction, rank, and correlation all pass by a wide margin. Its direct-head
parameters changed by maximum absolute `0.345237`, no step was skipped, and
the mechanical canary passed. Thus the current direct objective is not a
near-random no-op on forecastable raw context, although its regression
calibration remains slightly below the deliberately strict target.

This sharply narrows the active diagnosis. The production MDN family, target
projection, causal batching, likelihood path, direct-return loss, and
optimizer can learn this same v2 process when supplied raw causal history.
Consequently, near-chance performance after the learned representation would
point to information lost or made inaccessible at the
representation/interface boundary, not to an inherently unforecastable
series or a generally incapable MDN. That attribution is not yet final: the
unchanged trained representation must finish, then its frozen features must
be tested with the preregistered affine decoder and the production learned
readout on the identical splits. The final `[3328,4096)` interval remains
unopened and unauthorized.

## 2026-07-16 V2 Representation/Interface Diagnostic Hardening

The canonical v2 representation completed all 3,000 train-core optimizer
steps and its immutable receipt verifies. The checkpoint SHA-256 is
`9341bd613e3385894eba5f716c469f372ebcff37e4039c37198f42635338e6a2`.
End-of-run health did not show collapse: latent standard deviation was
`0.362748`, effective rank was `25.7529 / 32` (`0.804779`), condition number
was `58.1197`, mean loss was `0.247339`, and the last checkpoint step was
3,000. Outer augmentation retained `0.864126` of already-valid cells. These
are geometry and execution checks, not forecast evidence.

Source inspection corrected an important localization ambiguity before any
v2 frozen-feature capture. `mdn_edge_context_features.probe` is not raw
representation: it is emitted after the learned MDN backbone, learned channel
adapters, and direct-head adapter. Its 400 values are base, quote, and
base-minus-quote at hidden width 128 (384 dynamic values), followed by 16
edge-identity values. Runtime separately emits the actual frozen encoder
output in `representation_edge_features.probe`: base, quote, and their
difference at encoder width 32, for 96 values total. A 384-value affine
failure therefore cannot by itself be assigned to the representation.

The fixed downstream procedure now reports three causally matched controls:

- the preregistered post-MDN 384-value affine decoder, excluding identity;
- a raw 96-value encoder affine decoder on the same trained checkpoint; and
- a deterministic seed-17, no-representation-checkpoint raw-encoder control
  on train and validation only.

All three arms first fit only `[0,2496)` and select the declared ridge grid
only on `[2560,2816)`, without a certified path being opened, hashed, or
parsed. The trained raw-96 validation gate then mechanically publishes one
immutable route. A strong pass permits the two trained canonical arms to
receive one exact certified `[2880,3264)` capture; a failure permanently
forbids canonical certification and invokes the fixed four-arm
representation screen instead. Only that screen's validation-selected arm
may receive the single certified attempt. The untrained control can never
accept a certified input. Before any authorized certified Runtime launch, an
immutable attempt receipt is published and cannot be retried. Candidate
numerical failure is based only on the train solve and rejects only that
declared ridge; an all-invalid grid fails before certified scoring. Main and
replay must remain byte-identical.

The design history is explicit. The supplemental diagnostic preregistration
is `de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8`;
the exact-range capture amendment is
`3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5`.
The raw-interface localization design was first fixed pre-MDN at
`756fca5ba6ffee50bcee3f6ed9bd360a95f8ed1b7e409e4102774394e4a29d2d`.
Its current audit-hardening revision is
`2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4`;
it was made after train-core MDN checkpoints existed but before any
validation/certified capture or affine result, and changed no arm, split,
ridge, gate, or checkpoint. The conditional representation-ablation protocol
was fixed before any canonical representation forecast at
`6a4175f431347387f33c250b747f1f34c29099aaf4b3c94a75ea2e4960cef6cd`.
Its certification-order amendment is
`30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67`;
it was fixed while the canonical MDN still had only train-core intermediate
metrics and before any canonical validation probe or certified row existed.

The structural reason for that conditional protocol is narrower than the
original augmentation suspicion. With history length 30, canonical time
scales `8,16,32,64` produce duplicate full-history content for scales 32 and
64. Time tokens contain window mean and standard deviation rather than raw
order or endpoints; frequency tokens retain magnitude but not complex phase;
and time-frequency alignment pressures time latents toward that phase-poor
branch. `USE_RAW_RECONSTRUCTION_TARGETS=true` reconstructs those descriptors,
not the raw sequence. If the trained raw-96 affine arm fails its strong
validation gate, the fixed first screen is therefore: replace scale 64 with
scale 1 to expose endpoints, disable frequency tokens, and independently set
time-frequency alignment weight to zero. Those are three separately declared
one-field arms; broad augmentation/VICReg removal is not repeated first
because the corresponding v1 tests worsened decodability.

Two independent read-only implementation audits cleared the original
evaluator's numerical and provenance mechanics. A later cross-document audit
then found that its capture runner would have opened canonical certified
development before the already declared conditional ablation decision. No
such capture had run. The defect is being corrected by distinct immutable
development and certification stages; the earlier source hashes are
superseded and will not authorize execution. The helper continues to compile
under the production libtorch/CUDA link with `-Wall -Wextra -Werror`, and
historical production raw-96 and MDN-384 probes exercised all six
redundant-feature ridge candidates successfully.

The canonical production MDN is still training on the frozen representation,
so there is not yet a learned-readout, raw-encoder, post-MDN, or untrained
control result to interpret. No validation or certified capture described in
this section has run. The final `[3328,4096)` interval remains unopened and
unauthorized.

## 2026-07-16 V2 Physical Source Exposure Correction

The preceding final-interval statement is superseded. Before any frozen
feature or representation validation capture, a source-provenance audit found
that the completed representation job and running production MDN job inherited
the preparation registry. Their manifests name canonical `data/raw` files and
report an authoritative 4,096-anchor source domain. Their explicit cursor was
still train-only `[0,2496)`, so no final anchor entered an optimizer step or a
reported forecast metric, but Runtime constructed the full source domain from
files containing `[3328,4096)`.

This violates the original fresh-seed requirement that development
diagnostics reject `data/raw` and use the physically truncated
`data/development_prefix` view. The first representation checkpoint
`9341bd613e3385894eba5f716c469f372ebcff37e4039c37198f42635338e6a2`
and its associated MDN run are quarantined. Neither may supply a frozen probe,
route decision, certified confirmation, or final claim. V2's final interval
is conservatively considered operationally exposed; a future independent
final result requires another fresh dataset and one-shot ledger.

The correction was fixed while the MDN had reached train-core step 2,450 and
before any representation validation, affine result, conditional route,
ablation, or certified feature row existed. The immutable amendment is
`src/config/benchmarks/synthetic_continuous_graph_v2/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md`
at `c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39`.
It requires a runtime-local mirror made solely from the nine sealed prefix
CSVs, mirror-local causal caches, source receipts that reject `data/raw`, an
authoritative 3,264-anchor domain, and new step-zero representation and MDN
runs under new schemas. Architecture, seed, optimizer, steps, splits, probes,
gates, and the conditional route remain unchanged. Clean development
localization can continue; independent V2 final evidence cannot.

After quarantine, the invalid MDN process was stopped to release compute. It
had emitted train-core event step 3,080, last checkpoint step 3,050, and no
result receipt. Its cumulative direct output was still chance-level:
direction `0.499946`, pairwise rank `0.499391`, and correlation `-0.00149536`;
the mixture expectation was likewise approximately random. These numbers are
retained only as contaminated train diagnostics and cannot participate in any
route or gate.

## 2026-07-17 V2 Isolated-Prefix Cursor Alignment Erratum

This entry supersedes only the operational domain and range assertions in the
preceding V2 sections. It does not rewrite the original preregistration or the
historical reports executed under the then-declared `[2880,3264)` range. Those
metric rows remain preserved as historical development evidence; they are not
reclassified as clean isolated-Runtime certification.

A clean Runtime reconstruction from the physically isolated
development-prefix mirror reports `accepted_anchor_count=3261`,
`candidate_anchor_count=3261`, and maximum accepted anchor index `3260`. The
last accepted anchor closes on master day 3,289 at key `2177711999999`.
Master day 3,290 is the required weekly future/coarse boundary, not another
anchor. Train `[0,2496)` and validation `[2560,2816)` are unchanged. The
operational certified-development range is `[2880,3261)`: 381 anchors and
3,429 probe rows.

The immutable isolated-source closure receipt is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/development_source_closure.status`
at
`0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7`.
Its retained `source_cursor_last_master_day_index=3290` field names the coarse
future boundary rather than the last accepted anchor. Immutable history was
not rewritten. The layered correction chain is:

- `DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md` at
  `132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d`;
- `DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md` at
  `88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6`;
- `seal_and_verify_cursor_alignment_erratum_v2.sh` at
  `e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99`;
- `cursor_alignment_erratum.status` at
  `c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4`.

All clean downstream stages must bind the original source closure, both
correction documents, and the cursor-alignment erratum receipt.

At this entry, the clean step-zero representation/MDN rerun sequence is active
but not final. No clean representation-validation, frozen-feature,
certified-development, or learned-readout conclusion is asserted here.

During the provenance audit, an agent accidentally performed a read-only grep
over prefix-equivalent rows under canonical `data/raw`. It made no writes, did
not inspect the final tail or model/metric artifacts, and no observation from
that command is used as evidence, selection input, route decision, or claim.
V2 final remains conservatively operationally exposed because of the earlier
full-source Runtime construction. Independent final evidence still requires a
fresh dataset and ledger.

## 2026-07-21 Clean Isolated Retry and Development Affine Route

This entry supersedes the preceding statement that the clean isolated rerun is
still active. It does not restore independent V2 final evidence. All work in
this entry remained inside the physically isolated 3,261-anchor development
domain, and the final `[3328,4096)` interval was unavailable to these Runtimes.

The clean representation rerun completed 3,000 optimizer steps. Its immutable
result is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_isolated_v2/result.status`
at `981971679e4c37ba23919aae549eab1bc1ea1c1452f1978c004802f4807dbd07`;
the checkpoint is
`70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d`.
Execution health again did not show gross collapse: mean loss `0.247339`,
latent standard deviation `0.362748`, effective rank `25.7529 / 32`
(`0.804779`), condition number `58.1197`, zero skipped batches, finite
gradients and parameters, and zero non-finite outputs. These remain geometry
and execution checks, not forecast evidence.

The clean production-MDN retry then completed 3,500 optimizer steps. Its
result is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1/result.status`
at `d9eeddb89be7f2313083f4ea375bbf8c7f4168c95d15c4dbc216eadd009c1d93`;
the checkpoint is
`a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f`.
Because two post-training sealing races were corrected without rewriting the
job, the result alone is not authority. The required completion-closure
receipt is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1/completion_concurrency.status`
at `9ee4e5c78809ee622a9979608248f3db3309b50d3de53e91c2ba86a2187540cf`.

The clean learned readout reproduced the earlier chance behavior. Cumulative
direct output reached direction `0.500169`, pairwise rank `0.499649`,
correlation `-0.00133294`, and RMSE `0.0187018`; mixture expectation reached
`0.501401`, `0.495230`, `-0.000177704`, and `0.0177394`. Mean loss was
`31.5184`, no batch was skipped, parameters were finite, and no non-finite
output was observed. This removes the contaminated source tree as an
explanation for the earlier random result.

Frozen feature capture then completed on train `[0,2496)` and validation
`[2560,2816)` only: 22,464 and 2,304 edge/channel rows respectively. Its
immutable development receipt is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/development.status`
at `fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6`.
Trained and deterministic untrained-control captures both replayed exactly.
Maximum anchor read was 2,815; no certified capture, policy path, canonical
`data/raw` path, or final-holdout path was opened.

The original affine runner first failed closed because CUDA 12.4 exposes
`include` and `lib64` as compatibility symlinks. No affine result existed at
that failure. A separate operational correction retained the scientific
runner `ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173`
and helper `45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`
unchanged, verified only those two exact aliases, and retained the original
compiler, linker, and RPATH arguments. The frozen operational runner is
`run_frozen_representation_affine_probe_isolated_v2_cuda_canonical.sh` at
`008e45996da402a61a4aea8765a6922997cb35de605cd21d9fc255afefa5345d`;
the correction record is `AFFINE_CUDA_CANONICAL_PATH_CORRECTION.md` at
`d9c88f5c37771678016799afb157ec7661e3016eb58cdb4321da67b3329358ce`.
Two independent source audits passed before execution.

The resulting development runtime is sealed at
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_affine_development_isolated_v2`.
Its status hash is
`bf90ba9ef353c6b93aced12a98636351e2380b509454486517ab47f5c8372c06`;
its master-manifest hash is
`2619295b834762cb3b914e3ba8f06b9b423fae5be0a09dcd95bad31505563a2b`.
Main and replay are byte-identical. The validation-selected affine results are:

| Frozen surface | Direction | Pairwise rank | Correlation | RMSE / target RMS | Strong gate |
|---|---:|---:|---:|---:|---|
| trained raw encoder, 96 values | `0.759115` | `0.742622` | `0.716572` | `0.697742` | fail |
| trained post-MDN context, 384 dynamic values | `0.819878` | `0.784722` | `0.797250` | `0.604376` | fail |
| untrained raw encoder control, 96 values | `0.796441` | `0.738281` | `0.744059` | `0.668391` | fail |

Every trained raw-96 gate failed. The immutable route trigger is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/affine_route_trigger.status`
at `bbaa1e5f6e81741569fc905f3e4601b7495c7b9e281581de5bba7d617b7a1860`;
it fixes `route=representation_ablation_screen`. A separate foreground
verification of the complete sealed runtime and route returned exit zero.

The result is narrower than either “the representation is random” or “the MDN
is broken.” The trained raw representation retains partial linearly decodable
signal, but canonical representation training does not improve the served
surface over the untrained control overall. The post-MDN dynamic context
contains more linearly recoverable signal than either raw-96 surface, even
though the production mixture and direct outputs trained on it are at chance.
Together with the successful raw-history production-MDN isolation, this is
evidence for two interacting boundaries: insufficient or poorly exposed
task signal upstream, plus downstream conditioning/optimization that fails to
recover even the partial signal that remains. An affine failure is not proof
that no nonlinear decoder could recover information.

Channel order is `1w,3d,1d`. The daily raw-96 validation slice is the weakest:
direction `0.694010`, rank `0.660156`, correlation `0.471684`, and normalized
RMSE `1.341405`. The 3-day and 1-week channels reuse their next-bar labels
across daily master anchors, so their easier aggregate contribution must not
be mistaken for daily next-step capability. Source audit found no mechanical
off-by-one, edge permutation, checkpoint-loading, target-coordinate, or probe
alignment defect. It did find a plausible semantic cluster: mean/std time
tokens omit exact order and endpoint values; scales 32 and 64 both clamp to
the same full 30-step window; phase-poor frequency tokens receive equal mean-
pool weight; temporal dilation and warp do not anchor the endpoint; and global
VICReg regularizes a different summary from the served per-channel pool.

Per the predeclared conditional protocol, the next authorized development
stage is the four-arm `canonical`, `endpoint_scale`, `time_only`, and
`no_tf_alignment` representation screen. Canonical certified capture is now
forbidden on this route. Exactly one validation-selected ablation arm may
later receive the single certified-development attempt. Independent final
evidence remains unavailable and still requires a fresh dataset and ledger.

## 2026-07-23 Retry1 Operational Interruption Boundary

The one-shot representation-ablation retry1 runner was frozen at
`bebd812c48ba318ec632ed490841188daef9cdd68e02a53616ca9feba809ae43`.
Its full preflight passed before execution. The process later stopped making
observable progress after the endpoint-scale arm completed and while the
time-only arm was still incomplete. Retry1 was not resumed or re-entered.

The retained retry1 runtime contains 49 regular files, 25 directories
including its root, and 64,939,302 regular-file bytes. Its canonical
`find . -xdev` content-inventory digest is
`6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05`.
The endpoint-scale completion status, checkpoint, Runtime result,
representation report, and job manifest respectively have SHA-256 values
`b0fa364d31f32471cf0ff3d69b2836203e4dc47fd79f4c0507a7d7367b2f5ed5`,
`09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec`,
`7d3275bc2bdd8d647f1f06f41c1d11097acff56cc4f5e8cbb7d2497978ce182c`,
`99e370677d4dd1932aa4fd3af66b954e2969a3afc55b1be2436b8875e8c64740`,
and
`f1e514f84f05ae898b8616204e75cfe4034f2b23ff972c525f62635349905c7c`.
These artifacts agree on 3,000 completed optimizer steps, seed 17, train range
`[0,2496)`, finite parameters, and no input representation or MDN checkpoint.
The report's internal Runtime/Lattice checkpoint digest
`14398b461a259bc5` is a distinct identity from the checkpoint file SHA-256.

The latest retained time-only report has SHA-256
`e4dde2289353241dc9dc501e8486f4d14c99028b4d17a579cf3b9968b691d19b`
and records 2,880 attempted/completed optimizer steps, while its last
checkpoint step is reported as 2,850. Its partial checkpoint SHA-256 is
`33335faecacfc34d222389da326594f323243c57e5fb1f456ac266c7e5db01fc`.
No time-only `training.status` or Runtime result exists. The no-TF-alignment
arm has no training directory. No challenger capture, affine probe, selection,
development completion, certified attempt, or final result exists. The
time-only artifacts are interruption evidence only and are not reusable
training or scientific inputs.

Recovery found external Docker-VM evidence consisting of a runaway
`/tmp/ac-full-sanitize.log` at inode 161829 and contemporaneous `sdd` I/O
errors. This evidence is operational and external to retry1. It does not
establish an exact process exit status or stop time, does not attribute the
interruption to a representation arm, and is not model or metric evidence.

The immutable sibling interruption closure is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1/interruption_closure.status`
at
`e6c845233f3f434a9c46bead1b9fb825217492a5da7ae0a95174fc10b15e1117`.
Its source inventories are
`c7cce9005bee5efaa5f85924624839afad57655b4bfdb3d0d4a774fd4bf60926`
and
`40c7b2cf3846f3c439c6ebfe8d86b2839c969996fe6ce211be3577e2d11613ee`.
The closure was independently audited, published by atomic no-clobber rename,
verified after publication, and left the retained retry1 digest unchanged.

A fresh retry2 must restart the time-only and no-TF-alignment arms from
optimizer step zero. The completed endpoint may be considered only through a
separate endpoint-only evidence bundle followed by a second retry2-local,
byte-identical, non-hardlinked copy and an exact scientific/configuration
equivalence check. Historical endpoint training must not be relabeled as a
retry2 training job: source optimizer steps remain 3,000, while the retry2
import performs zero optimizer steps. Retry2 development is being redesigned
as a fixed 12-stage receipt chain so each invocation can complete at most one
durable stage. No retry2 preflight, development stage, certified evaluation,
or final evaluation is asserted by this entry.

## 2026-07-23 Retry1 Endpoint Evidence Bundle

The independently audited endpoint-only historical-evidence bundle is now
sealed at
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry1_endpoint_bundle_for_retry2_v1`.
Its immutable receipt SHA-256 is
`ff675afc779b106f628f3ea65fe3409314bf6ea29a531100e73dfa1a3cca9f96`.
The frozen amendment and sealer SHA-256 values are respectively
`c94c282d93844563f83abf3e1826111e14d640370d38e778bee04070aa1303ad`
and
`b2edac9ef89d2ff630a5dbf33c041f2d3016c3fffbe74a66f5a5c38975d01a77`.
The source regular-file and directory inventory SHA-256 values are
`78171e0900f9e034642a85320dbcabb2b52f57eed40a73ca55a973c5f65efc6d`
and
`8e23a668bad459c9effdd89d45d4c6c461f546daf544d06a7e4ae9b653c9e6ec`.

The bundled endpoint snapshot contains exactly 21 regular files and nine
directories including its root, totaling 32,731,999 regular-file bytes. Its
canonical content digest remains
`bd2f8d55b4e3e3a3a06bf14749b28ea0bec01ea9c07aaa1c1628e9ed4f59e13f`.
Every bundle file is mode `0444` with link count one; every directory is mode
`0555`. The copy used `cp --reflink=never`, is byte-identical to the retained
endpoint source, and has distinct source/destination inode identity.

The terminal Runtime event stream was independently checked as 17,608
contiguous records ending at sequence 17,608. It contains eight exact
artifact-publication kind/path records. Six of those publications contain
digest/schema pairs; the delegated-report and checkpoint publications do not.
The bundle verifier encodes this distinction explicitly rather than
fabricating two absent digest/schema records.

The copied checkpoint SHA-256 remains
`09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec`.
The endpoint policy, network, training configuration, and capture
configuration SHA-256 values are respectively
`c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8`,
`42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e`,
`c517ef409c1829413d18536851aecc48bb94e7f7ef2ed1386106511ee1e3ef28`,
and
`63e042f47bbdbc2970cd8afbfcc639fcec5a7a980aacbf7a59d8db592f97f821`.

This bundle is provenance, not a retry2 training result. It authorizes no
direct model use, optimizer step, checkpoint resume, capture, affine probe,
selection, certified access, or final access. Retry2 must still make a second
local `cp --reflink=never` copy, prove exact scientific/configuration
equivalence, and record zero retry2 endpoint optimizer steps before any
endpoint capture. Post-publication verification returned exit zero and left
the retained retry1 content digest unchanged. Retry2 execution remains
prohibited until its staged runner passes independent audit and both fixed
space gates are satisfied.
