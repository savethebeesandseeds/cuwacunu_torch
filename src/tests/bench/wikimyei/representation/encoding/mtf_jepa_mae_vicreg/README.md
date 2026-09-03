# Isolated representation tests

This directory is the low-cost test boundary for the active
MTF-JEPA-MAE-VICReg representation. The fast gate constructs the model directly
and uses only in-memory rank-4 tensors plus LibTorch. It does not construct a
source, NodeLift stream, MDN, protocol pipeline, Runtime job, checkpoint, or
report artifact.

Run the fast mechanical gate while changing the representation core:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-isolated
```

The gate covers configuration, tokenization, masking, forward and backward
passes, loss branches, missing-data behavior, serving-pool policies, and target
EMA behavior. `all` and `run` are aliases for this fast tier, so neither starts
a training loop.

When a change affects optimization or training readiness, run the bounded
synthetic trainer explicitly:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-training-smoke
```

The training smoke is not evidence of market usefulness. Questions about
learned signal or pooling quality should use a separate fixed synthetic
train/holdout probe, still directly against this module; they should not be
answered by extending this gate into the graph-first or end-to-end pipeline.

## Qualification tiers

Run the exhaustive module contracts when changing loss routing, masking,
serialization, target EMA, or optimizer behavior:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-contracts
```

Run the deterministic three-seed learning qualification when changing the
encoder, objective, or serving pools:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-scientific-qualification
```

The default report is compact `key=value` output. Pass `--verbose` directly to
the built qualification executable only when per-ridge, per-split, and
per-channel diagnostics are needed. The qualification uses disjoint generated
groups, train-only normalization and probe fitting, a fixed holdout, three
model seeds, and neighboring ridge controls. It compares raw inputs, untrained
representations, trained ordered tokens, and each same-width serving policy.

This learning tier deliberately uses a compact architecture on CPU while
mirroring the active train-core objective policy. It establishes behavior of
the module under that bounded workload; it does not prove market usefulness or
claim that the full active architecture will have identical learning quality.
Outer launcher augmentation is outside this module boundary.

`run-qualification` runs the contracts and learning qualification
sequentially. The large LibTorch translation units are also built sequentially
to avoid unnecessary peak memory.

## Representation-quality probe

The preregistered measurement boundary is documented in
`REPRESENTATION_QUALITY_PROTOCOL.md`; executed isolated results and their
limitations are recorded in `REPRESENTATION_QUALITY_FINDINGS.md`.

First run the tokenizer-information characterization. It proves which ordered
sequences collide before the transformer and characterizes the clipped
full-history tokens in the active scale plan:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-tokenizer-information
```

The representation-quality harness evaluates the exact active
`H=30,F=9,D=32` architecture through its active `all_tokens` served output. It
freezes trained and same-seed untrained encoders, selects ridge regularization
only on a disjoint validation split, and measures labeled-sample efficiency on
four generated sequence families: multiscale state, temporal order/regime,
cross-channel dynamics, and multi-horizon future state. It also reports true
centered-covariance eigenspectrum geometry and paired nuisance-versus-semantic
sensitivity. A fixed orthonormal raw-history projection provides an equal-width
`3x32` control, and paired group bootstrapping reuses the final predictions
without rerunning the encoder.

Use the bounded one-seed screen while developing the encoder:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-representation-quality-fast
```

Use the canonical 3000-step, three-seed CUDA tier for the stronger active
screen:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-representation-quality-active
```

For a paired attribution of the module-internal VICReg weak views, invoke the
quality executable directly with `--weak-views on` and `--weak-views off`.
The off arm zeros the active view jitter and time-drop strengths while
preserving their RNG draws, so later objective masks remain paired. This flag
does not control launcher-owned input augmentation, which remains outside this
executable.

The frozen objective/mask attribution protocol is documented in
`REPRESENTATION_OBJECTIVE_MASK_ATTRIBUTION_PROTOCOL.md`; its executed results
and conditional TF/VICReg split are recorded in
`REPRESENTATION_OBJECTIVE_MASK_ATTRIBUTION_FINDINGS.md`. The three-seed CUDA
run compares the full objective, JEPA/MAE-gradient-off, JEPA/MAE-only,
full-objective/context-overlap-allowed, and triggered single-branch arms with
identical initialization, batches, target masks, and per-step CPU/CUDA RNG:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-objective-mask-attribution
```

The subsequent test-only mechanism-repair screen is frozen in
`REPRESENTATION_OBJECTIVE_REPAIR_PROTOCOL.md` and its executed result is
recorded in `REPRESENTATION_OBJECTIVE_REPAIR_FINDINGS.md`. Gradient-matched TF
warmup and projected per-channel-stratified VICReg both failed their independent
rescue/noninferiority/geometry gates under valid mechanics. The combined arm
was therefore not authorized or run, and neither candidate changes the active
production recipe. The complete ordered workflow is:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-repair-screen
```

The separately frozen variance-component necessity follow-up is documented in
`REPRESENTATION_VICREG_VARIANCE_NECESSITY_PROTOCOL.md`; its executed result is
recorded in `REPRESENTATION_VICREG_VARIANCE_NECESSITY_FINDINGS.md`. Disabling
only the projected stratified VICReg variance term erased essentially all of
the observed fixed-seed AULC and geometry damage and returned almost exactly
to JEPA/MAE. The preregistered necessity gate nevertheless failed because the
paired held-out-group rescue interval still crossed zero. The arm is therefore
not a repair, and no next experiment or production change was authorized. The
artifact is authoritative with one behaviorally null manifest deviation: the
inactive JEPA/MAE reference preserves its accepted-screen channel multiplier,
as detailed in the findings. The ordered reproducible workflow is:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-variance-component-necessity-screen
```

This is a bounded causal localization, not a production qualification. In
particular, the overlap arm changes which non-target tokens are available as
context; it is not a true mask-off arm.

## Outer-augmentation matched training screen

The frozen module-only comparison of neutral preprocessing, the full active
launcher profile, and the semantically qualified subset is documented in
`REPRESENTATION_OUTER_AUGMENTATION_TRAINING_PROTOCOL.md`; the executed result
is recorded in `REPRESENTATION_OUTER_AUGMENTATION_TRAINING_FINDINGS.md`.

The qualified subset did not materially improve clean step-32 representation
AULC over either full active or neutral preprocessing. It tracked neutral
almost exactly. Full active preprocessing removed substantial support and
often removed the terminal anchor, reproducing its semantic failure, but its
clean learned representation was not materially worse than the other two
arms. All three arms lowered training loss while ending below their shared
step-zero clean AULC. The final classification is
`qualified_candidate_not_supported`.

The build-only target compiles the historical contracts, exhaustive gate
fixtures, and isolated harness without starting training:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 outer-augmentation-training-screen
```

There is intentionally no Make target that reruns the scientific screen. Its
single frozen attempt has been consumed, and it authorized no production,
long, end-to-end, or follow-up experiment.

## JEPA/MAE core decomposition

The final clean-input core-objective localization is frozen in
`REPRESENTATION_JEPA_MAE_CORE_DECOMPOSITION_PROTOCOL.md`; its executed result
and human interpretation are recorded in
`REPRESENTATION_JEPA_MAE_CORE_DECOMPOSITION_FINDINGS.md`.

The complete 2x2 comparison used an unchanged factorial-null encoder,
JEPA-only, MAE-only, and the accepted JEPA+MAE objective. JEPA-only and
MAE-only each significantly reduced clean sequence-probe AULC versus the null.
The combined arm was less harmful than either singleton and its factorial
interaction was reliably in the beneficial, not harmful, direction. Removing
either branch is therefore not a supported repair. The terminal classification
is `core_component_marginal_harm_not_localized`: this means the combined arm's
marginal harm could not be assigned to one conditionally removable branch, not
that the singleton objectives were harmless.

The unchanged served encoder still trailed the equal-width raw control by
`0.0830` AULC. The recommended next question is therefore RSSM-1, a no-training
map of raw, tokenizer, encoder pre-pool, and current served surfaces to localize
where that information/sample-efficiency gap enters. JMCD-1 itself authorizes
no such execution, rerun, longer training, production edit, or end-to-end run.

The build-only target compiles the historical contracts, pure JMCD gate, and
isolated harness without launching training:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 jmcd-screen
```

Neither v1 tier emits a full quality qualification. The broader protocol still
requires the PCA, shuffled-target, family-level interval, and five-seed release
controls. A scientific failure is reported as a classification rather than
converted into a process failure. The process exits nonzero only when the
harness or training mechanics are invalid.

## Representation Surface Sufficiency Map

RSSM-1 is frozen in
`REPRESENTATION_SURFACE_SUFFICIENCY_MAP_PROTOCOL.md`; its audited result and
plain-language interpretation are recorded in
`REPRESENTATION_SURFACE_SUFFICIENCY_MAP_FINDINGS.md`.

The one permitted no-training CUDA invocation compared normalized raw history,
tokenizer tokens, encoder tokens before pooling, and the current served
per-channel `all_tokens` mean. Tokenizer and encoder surfaces retained
raw-like sequence accessibility in both native and fixed-96 tracks. The first
material loss occurred from encoder tokens to the served mean in both tracks,
so the terminal classification is `serving_pooling_loss`. The reversal probe
agreed: raw, tokenizer, and encoder surfaces were order-decodable, while the
served surface fell below the frozen decodability threshold.

The result passed exact reproduction of all 72 accepted step-zero and six
legacy-raw reference keys, all capture/identity/permutation controls, and both
continuous and reversal shuffled-target gates. It does not authorize a switch
to an existing time/frequency pool, training, production changes, or an
end-to-end run. The single recommended next question is a separately frozen
no-training pooling-structure map.

The build-only target compiles the focused harness and mechanical auditors but
does not start the consumed scientific experiment:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 rssm-screen
```

## Performance characterization

The performance executable uses the exact active model configuration and
input shape. It exposes both the one-anchor serving microbatch and the active
64-anchor downstream batch:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 perf-cpu
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 perf-cpu-batch64
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 perf-cuda-encode
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 perf-cuda-encode-batch64
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 perf-cuda-train
```

CUDA timings synchronize before and after every measured operation. Encode
measures `eval` plus `NoGradGuard`; training measures the representation core's
forward, backward, Adam update, and target EMA update at the active batch of 32
anchors. Launcher augmentation, graph work, gradient clipping/scans, data
loading, checkpointing, and reporting are excluded. Reported byte counts are
logical tensor sizes, not CPU RSS or CUDA peak allocator memory. A passing
benchmark means execution and measurements are valid; no adequacy or regression
threshold is implied until a machine-specific baseline is registered.
