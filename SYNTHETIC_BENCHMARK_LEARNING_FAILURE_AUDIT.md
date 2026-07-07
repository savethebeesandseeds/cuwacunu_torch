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
