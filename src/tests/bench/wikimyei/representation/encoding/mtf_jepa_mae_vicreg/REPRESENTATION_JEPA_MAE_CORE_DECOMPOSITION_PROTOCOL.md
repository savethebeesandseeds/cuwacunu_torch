# JEPA/MAE Core Decomposition (JMCD-1)

Date frozen: 2026-08-26

## Question and boundary

JMCD-1 asks whether clean-input representation learning is helped or harmed by
JEPA, MAE, or their interaction. It is a complete `2 x 2` objective-factor
screen at the isolated representation-module boundary.

Nothing in JMCD-1 constructs NodeLift, an MDN/readout, Runtime, observer,
policy, or an end-to-end graph. It changes no production source, configuration,
launcher default, architecture, token definition, mask policy, optimizer,
training length, or augmentation dose. Launcher outer augmentation is absent.
TF and outer VICReg optimizer coefficients are zero in every arm.

The exact active shape is `C=3,H=30,F=9,D=32`, served as the 96-wide
per-channel `all_tokens` representation. All objective branches still execute;
zero coefficients remove optimizer gradients only and must not disable a
predictor, decoder, diagnostic, weak view, or random draw.

## Four contemporaneous arms

| arm | JEPA | MAE | TF | VICReg |
| --- | ---: | ---: | ---: | ---: |
| `core_objective_null` | 0 | 0 | 0 | 0 |
| `jepa_only` | 1.0 | 0 | 0 | 0 |
| `mae_only` | 0 | 0.25 | 0 | 0 |
| `jepa_mae_only` | 1.0 | 0.25 | 0 | 0 |

The accepted JEPA/MAE configuration is otherwise unchanged: context/target
overlap `0.50`, internal weak views active, target EMA active, Adam learning
rate `1e-3`, batch size 96, and one Adam step followed by one target-EMA step.
The null arm is a required factorial cell, not a substitute for initialization.
Its served online parameters, embeddings, probes, and geometry must remain
byte-exact to its step-zero state through update 32.

## Execution and pairing mechanics

Use model seeds `17,31,47`, 32 updates, checkpoints `0,16,32`, CUDA:0,
the accepted deterministic synthetic splits and normalization, and clean probe
sample counts `32,64,128,256`.

All mechanics are conjunctive:

- The four arm configurations differ only in `lambda_jepa` and `lambda_mae`.
  `use_jepa_loss`, `use_mae_decoder`, `use_tf_align_loss`, and
  `use_vicreg_loss` remain true.
- Pair exact named initial parameters, target state, clean batch rows and row
  hashes, clean data/mask hashes, target/context mask tensors, weak-view byte
  hashes, and CPU/CUDA generator states immediately before and after each
  module forward.
- Pair step-zero clean embeddings, probe predictions, selected ridge
  penalties, probe AULC, served geometry, clean global-preprojector geometry,
  and clean projected per-channel geometry.
- At checkpoints, raw JEPA and MAE losses and raw gradients remain finite and
  nonzero even in arms where their coefficient is zero. The effective gradient
  of an inactive branch is exactly zero. Direct arm gradients must equal the
  weighted branch reconstruction within relative error `1e-5` for both all
  trainable parameters and the served tokenizer/encoder trunk.
- Report JEPA/MAE gradient cosine and cancellation at steps `0,16,32`, split
  between tokenizer, encoder, predictor, and MAE decoder where applicable.
- The MAE decoder remains exact in `jepa_only`; the JEPA predictor remains
  exact in `mae_only`; all non-target trainable parameters remain exact in the
  null arm. The appropriate active branch and served trunk must change in each
  singleton arm. The null served update norm is exactly zero.
- Every update is finite, has clip factor exactly one, and performs exactly one
  Adam step followed by one EMA step. Checkpoint diagnostics restore RNG,
  train/eval state, parameters, EMA, and optimizer state exactly.
- A zero-optimizer CUDA preflight must prove the arm table, common config,
  initialization, raw forward/mask/view identity, branch-gradient
  decomposition, and null zero-gradient behavior before scientific training.

The contemporaneous `jepa_mae_only` arm must reproduce the accepted reference
exactly. The reference is
`.build/tests/representation_vicreg_variance_necessity_v1_authoritative.log`,
753029 bytes, SHA-256
`bd382eb9d638bfc9ce42257eaca0ab5cb2cd4f44a96f2c4c946eb27e9e7dd038`.
Select exactly the 2046 unique keys matching
`^(seed_(17|31|47)\.arm\.jepa_mae_only\.|summary\.arm\.jepa_mae_only\.)`.
The sorted `key\n` SHA-256 must be
`7998b81e3aa42e585c75a6ddcc9a3e00e2bc09819d8595add52570ea8b168864`
and the sorted `key=value\n` SHA-256 must be
`f3696f996bccd1dd7485a7959fdb5415817e1f7bce2357dbbbf9e2e9654ff5fc`.
The existing read-only neutral-reference auditor is reused unchanged. Every
new JMCD diagnostic and aggregate must live under `jmcd.*`, outside the frozen
selector. Audit failure forces `invalid_numeric_or_mechanics`.

## Endpoints and uncertainty

The primary endpoint is step-32 fixed-seed-mean clean macro probe AULC. Step 16
is descriptive. Training loss is diagnostic and never determines success.

Use the same deterministic 512-replicate held-out-group bootstrap table and
seed `8387496322364763509` for every contrast. Each replicate resamples the
same held-out groups in every arm and then averages the three fixed training
seeds. Report point, paired 95% interval, and positive-seed count. These
intervals measure generated-group uncertainty, not training-seed uncertainty.

Let `N`, `J`, `M`, and `JM` denote the null, JEPA-only, MAE-only, and combined
AULC. Report these oriented contrasts:

1. `J-N` and `M-N`: standalone objective effects;
2. `M-JM`: rescue from removing JEPA;
3. `J-JM`: rescue from removing MAE;
4. `JM-N`: accepted combined trajectory relative to the trained null;
5. `J+M-N-JM`: harmful-interaction residual. Positive means the combined arm
   underperforms the additive expectation from the two singleton arms.

The factorial residual is recalculated inside every group-bootstrap replicate;
it is not formed only from reported interval endpoints.

Report all four final sequence-information families, per-seed contrasts, raw
control AULC, losses, parameter-partition changes, and served geometry for all
arms. The accepted combined decline is fixed-seed evidence dominated by seed
31; it is not a claim of seed-general deterioration.

## Frozen gates

A material positive contrast requires all three clauses:

- point estimate at least `+0.0024` (inclusive);
- paired 95% lower bound greater than zero (strict);
- at least two of three seed contrasts strictly positive.

Standalone non-harm for a singleton requires the lower bound of `arm-null` to
be strictly greater than `-0.0024`.

Define the causal subgates:

- `jepa_conditional_harm`: `M-JM` is materially positive;
- `mae_conditional_harm`: `J-JM` is materially positive;
- `jepa_standalone_improvement`: `J-N` is materially positive;
- `mae_standalone_improvement`: `M-N` is materially positive;
- `harmful_interaction`: `J+M-N-JM` is materially positive, both conditional
  harm gates pass, and both singleton non-harm gates pass.

Safety is separate from causal localization. For each singleton, require:

- all four final family deltas versus `JM` and all four versus `N` are at least
  `-0.02`;
- after averaging geometry across seeds, candidate effective and participation
  rank divided by the better of `N` and `JM` are at least `0.90`, and
  `(1-candidate_top)/(1-better_top)` is at least `0.90` with positive finite
  denominators;
- for every geometry metric where `JM` is worse than `N`, the singleton closes
  at least `0.50` of that gap and moves in the repair direction in at least two
  seeds; a metric with no harmful `JM` gap is not applicable and passes;
- every singleton seed has minimum active-dimension fraction at least `0.75`.

A singleton is a supported representation improvement only when its standalone
improvement, corresponding removal rescue, and complete safety gate all pass.
A removal rescue without standalone improvement is explicitly `less_harmful`
and is not a repair.

Numeric/mechanical validity and exact accepted-reference continuity take
precedence. Then emit one primary causal classification:

- `harmful_jepa_mae_interaction_supported`;
- `both_core_branches_conditionally_harmful`;
- `jepa_conditional_contributor_supported`;
- `mae_conditional_contributor_supported`;
- `core_component_marginal_harm_not_localized`.

Also emit separate JEPA and MAE standalone-improvement, less-harmful, safety,
and replacement-support booleans. Before the post-run reference audit, the raw
classification is provisional. Invalid inputs/mechanics/audit classify as
`invalid_numeric_or_mechanics`; a mechanically valid but nonmatching combined
control classifies as `accepted_jepa_mae_reference_not_reproduced`.

## Workflow, artifacts, and stopping

Before training: pin this protocol; implement only a test-side pure gate,
exhaustive gate fixtures, isolated harness mode, build-only Make target, and
zero-optimizer preflight. Run the smallest historical representation contracts,
gate fixtures, protocol-pin check, accepted-auditor self-test, compilation, and
preflight. Record a compact source/binary/reference hash manifest and workspace
status snapshot.

If all preflight checks pass, permit exactly one invocation containing all four
arms and all three seeds. A completed optimizer update or post-step endpoint
consumes the attempt. There is no smoke-training run, partial-seed run, or
rerun. Afterward, hash the raw log, run the exact reference audit, record a
post-run receipt and human-readable findings, and stop the test container.

Canonical artifacts are:

- `.build/tests/representation_jmcd_v1_preflight.log`;
- `.build/tests/representation_jmcd_v1_authoritative.log`;
- `.build/tests/representation_jmcd_v1_reference_audit.log`;
- `.build/tests/representation_jmcd_v1_receipt.sha256`.

Always emit `next_experiment_authorized=false`, `long_run_authorized=false`,
and `production_or_end_to_end_authorized=false`. JMCD-1 authorizes no 64/128
extension, coefficient search, architecture edit, production change, or
end-to-end run regardless of classification.
