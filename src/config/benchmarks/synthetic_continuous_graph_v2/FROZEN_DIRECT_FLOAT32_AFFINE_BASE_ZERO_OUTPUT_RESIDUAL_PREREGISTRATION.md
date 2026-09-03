# Project Clear Signal — Frozen Direct-Float32 Affine Base with Zero-Output Residual

## Protocol identity and finite stop

- Protocol and report schema:
  `synthetic_v2_frozen_direct_float32_affine_base_zero_output_residual_development_v1`.
- Authority: one-seed causal-repair development diagnostic only. Benchmark
  acceptance and certified authorization are false.
- The identity ends after at most one evaluator invocation and one seed-31,
  3,500-step residual optimizer fit. A valid complete report records exactly one
  fixed scientific classification below. Any base, step-zero, optimizer
  membership, or activation integrity failure; timeout; signal; evaluator
  error; malformed report; incomplete schedule; or post-run verification
  failure consumes and terminalizes the sole attempt without a scientific
  result.
- There is no resume, replay, retry, recapture, seed sweep, parameter sweep,
  residual-scale selection, early stop, validation selection, refit, encoder
  execution, checkpoint access, or automatic follow-on rung under this
  identity.

The narrow question is whether a separately optimized nonlinear residual can
add material train signal without damaging the already proven direct-float32
affine prediction. The affine prediction is a detached, immutable base outside
the residual module and outside Adam. This protocol does not train or evaluate
the representation, MDN, checkpoint, policy, certified set, or final holdout.

## Frozen implementation and authority

The runner binds the following immutable implementation closure:

- evaluator
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_direct_float32_affine_base_zero_output_residual_probe.cpp`,
  SHA-256 `63cd1cb3b26245b85496aaef25ec48c6a28f59dd5375a0258ef2bbcc6f3f23ee`;
- compile-only wrapper
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_direct_float32_affine_base_zero_output_residual_probe.sh`,
  SHA-256 `068641ff17ee212bbd4cbcf1ef356d7e6435499f9bcadd30fd7682c235711400`;
- included frozen warm-start evaluator, SHA-256
  `dcc6112c7920092cca1e36e24afe33fb4e9393325437a67942016052ad32296d`;
- predecessor optimizer-localization evaluator, SHA-256
  `7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b`;
- canonical affine parser/source, SHA-256
  `45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`;
- Phase 2A conditioned-affine source, SHA-256
  `5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570`.

The immediate scientific authority is the sealed warm-start result at
`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_representation_affine_warm_start_stability_development_v1/development.status`,
SHA-256 `a2433caac297f39cf02de2f6240605b43a90d77963a246f6366d8acf6c7a3c41`,
and its report
`frozen_representation_affine_warm_start_stability_report.txt`, SHA-256
`80825ab1cf4406dbe04616d6bb2bc1f424d4948ef15a1242b5f59ca068ec5405`.
The receipt must remain complete, development-only, scientifically available,
and classified `optimizer_destabilization_clear_stop`. It must retain exact
step-zero direct-float32 parity, the fixed seed-31/3,500-step schedule, the
severe train clear-stop, all implementation and authority hashes, and false
protected-access fields. Its frozen runner, preregistration, evaluator, and
wrapper are independently rehashed as:

- runner `4ec9c7ad31e84cd5e60644d4f453763b0cd1669e97604e1d7ea254239e5d9cf2`;
- preregistration
  `15ecb55515c2293b9e864d43cce4009f802399613921495066b2c17f2956b4dd`;
- evaluator
  `dcc6112c7920092cca1e36e24afe33fb4e9393325437a67942016052ad32296d`;
- wrapper
  `43440ad66bf480e281354c80bdd536babe436e0ea14c2d54be99616bac92a89b`.

The frozen train representation probe is SHA-256
`d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75`;
the validation probe is SHA-256
`8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd`.
The Phase 2A receipt is SHA-256
`b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5`
and its main/replay reports are each SHA-256
`2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a`.
The recovered Phase 2B result/report are SHA-256
`cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418`
and `34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36`.
The optimizer-localization V3 result and verification receipt are SHA-256
`ad9be3c76c69bafaa6e42ce63e1e10b05231878f87b465ffd8b56b8156237ad6`
and `edcda0efcfe00fb6f53fa005040e4a17c8d8c5a50b4c97656dbe2b4516ee665d`.

Authorized anchors are train `[0,2496)` and validation `[2560,2816)`, with
maximum anchor 2815. The trainer reads zero validation batches. Validation
combined states are evaluated only at fixed step zero and after the completed
fit; the post-fit guard is evaluated once and cannot choose a state, step,
seed, residual scale, threshold, or classification rule.

## Fixed causal procedure

All nine heads use `head=channel*3+edge`, flat row order
`anchor,edge,channel`, train-global float32 feature standardization, and
train-only per-edge/channel float32 target standardization. Metrics are
computed in float64.

1. Recompute the Phase 2A float64 centered-Cholesky affine oracle with fixed
   ridge `1e-12` from train only, then construct the exact direct-float32 map.
   Float64 oracle metrics remain provenance only. Every residual preservation,
   benefit, and clear-stop ratio uses the direct-float32 base on the same split
   and head as its denominator.
2. Store the direct-float32 base as two contiguous CPU float32 tensors
   containing 873 elements. They are detached, require no gradient, have no
   gradient, alias no residual storage, are not module parameters, and are
   absent from Adam. The bytes of all 873 float elements are compared with
   immutable clones after every optimizer step and once at the end; final base
   predictions on both splits must also be byte-identical to their snapshots.
3. Instantiate the residual
   `Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)+gather(channel*3+edge)`
   under seed 31. The input and hidden tensors retain their default seeded
   initialization. Only the final `128->9` weight and bias are byte-zeroed.
   The residual has six trainable tensors and 30,089 elements: four default
   input/hidden tensors with 28,928 elements and two zero-output tensors with
   1,161 elements. The combined standardized prediction is the fixed literal
   `base.detach()+1.0*residual`; scale 1 is never fitted or selected.
4. Before schedule or Adam construction, residual predictions must have exact
   maximum absolute value zero and combined/base prediction deltas must be
   exactly zero independently on train and validation. Output weight and bias
   must be byte-zero. Any failure emits no complete report and is terminal
   invalid, not inconclusive.
5. After step-zero integrity, construct exactly one Adam parameter group over
   exactly the residual's six tensors and 30,089 elements, with empty initial
   state and no base identity or storage range. The fixed CPU-float32 fit uses
   deterministic algorithms, seed 31, 3,500 steps, batch size 64, learning rate
   `1e-3`, betas `0.9,0.999`, epsilon `1e-8`, weight decay 0, global gradient
   clip 5, and `mt19937_64` uniform-with-replacement batches. The schedule
   fingerprint is `f2fa41d284a42d60`.
6. On the first backward pass, the aggregate upstream input/hidden gradient
   norm must be exactly zero while the output-weight gradient norm is finite
   and positive. After the first optimizer step, the output weight must differ
   from zero. On the second backward pass the upstream gradient norm must be
   finite and positive. After the full fit, upstream parameter delta and
   residual RMS on both splits must be finite and positive. Any activation
   proof failure emits no report and terminalizes the attempt.
7. Complete all 3,500 steps, all 3,500 base byte-invariance checks, and the
   final fixed train/validation evaluations. The float32 train summary MSE and
   float64 route MSE duplicate pairs at step zero and final use
   `abs(a-b) <= 1.25e-7 + 1.25e-7*max(abs(a),abs(b))`, never exact equality.

Fit accounting is one grouped analytic oracle reconstruction with nine head
solves plus one residual optimizer fit: `affine_oracle_grouped_fit_count=1`,
`affine_oracle_head_solve_count=9`,
`direct_float32_base_construction_count=1`,
`residual_optimizer_fits_completed=1`, `optimizer_fits_completed=1`,
`optimizer_steps_completed=3500`, `seed_count=1`,
`batch_schedule_count=1`, and `total_train_fit_procedures=2`. Retry, refit,
and early-stop counts are zero. A complete report contains exactly 1,246
unique newline-terminated `key=value` lines. Its statically expanded sorted
key inventory has SHA-256
`e2758a3247a46ebf85a2ae3d962fa0c986ddcb3204cdcfb04e337297b095ab92`.
Duplicate, malformed, missing-required, extra, or unknown keys are terminal
invalid.

## Fixed gates and classifications

Let every ratio below be final combined standardized MSE divided by the frozen
direct-float32 base standardized MSE for the same split and, for head gates,
the same head. Signed metric deficits are `base-final`; signed RMSE-ratio
increase is `final-base`.

Train preservation passes only if aggregate ratio is `<=1.05`, every one of
nine head ratios is `<=1.10`, direction/rank/correlation deficits are each
`<=0.01`, and RMSE/target-RMS increase is `<=0.05`. Material train benefit
passes only if aggregate ratio is `<=0.90`. The train repair-candidate gate is
material benefit plus every-head and metric safety. Validation preservation is
symmetric: aggregate `<=1.05`, every head `<=1.10`, deficits `<=0.01`, and
RMSE-ratio increase `<=0.05`. The residual repair gate requires the train
repair candidate, train preservation, validation preservation, and the
mandatory integrity/activation preconditions.

The optimizer clear-stop gate passes only if train preservation fails and
either train aggregate ratio is `>=1.25` or the maximum train head ratio is
`>=1.50`. Validation failure alone can never be called optimizer
destabilization.

The original strong validation gate is emitted diagnostically and passes only
if final combined validation direction, pairwise rank, and correlation are
each `>=0.95` and RMSE/target-RMS is `<=0.25`. It has no benchmark-acceptance
or certified authority in this single-seed protocol.

After all integrity preconditions and the fixed fit complete, classification
is the first matching rule:

1. residual repair and original strong gate pass:
   `frozen_affine_base_residual_strong_gate_pass_seed31`;
2. residual repair passes without the strong gate:
   `frozen_affine_base_residual_repair_established_seed31`;
3. optimizer clear-stop passes:
   `residual_optimizer_destabilization_clear_stop`;
4. train repair candidate passes but validation preservation fails:
   `residual_train_only_gain_no_validation_preservation`;
5. otherwise: `frozen_affine_base_residual_inconclusive`.

Only classifications 1 or 2 may authorize proposing a separately frozen
development confirmation using exactly seeds 47 and 73 together with the
sealed seed-31 result. That future confirmation uses fixed set `[31,47,73]`
and the historical `at_least_2_of_3_seeds` rule. It is not executed under this
identity. No classification here authorizes certified, final-holdout, policy,
or benchmark-acceptance access.

## Lifecycle and truthful terminal state

`--prepare` is compile-only, precedes the attempt, and is bounded by GNU
timeout at 300 seconds with 10-second TERM grace. It may not read either probe.
`--run-development` requires its frozen build receipt and cannot compile. The
sole evaluator invocation is foreground inside one private worker bounded by a
single 300-second `setsid` GNU-timeout supervisor with the same grace. The
worker is authorized by an unlinked inherited single-record capability FD and
inherits identity-checked exclusive protocol and shared authority locks.

The attempt is atomically consumed before worker launch. A stale attempt with
no result is sealed terminally under the exclusive lock and can never rerun.
Signals are ignored during kill, wait, reap, evidence publication, and terminal
sealing. Normal completion also reaps before process scanning, report
validation, science-complete publication, or result publication. Attempt,
lifecycle, log, report, science-complete, result, rejected, and terminal
artifacts use private directories, immutable metadata, and atomic no-clobber
publication. The fully validated result candidate is the final atomic commit;
no fallible scientific action follows it. `--verify-development` is read-only
and never compiles or evaluates.

Attempt evidence records planned limits separately from actual terminal
counters. Before an evaluator-started receipt, terminal actual evaluator,
oracle, base-construction, optimizer-fit, step, procedure, and seed counts are
zero. After evaluator start and before a validated science-complete receipt,
all actual oracle, base-construction, optimizer-fit, step, procedure, and seed
counts are `not_available`; the runner never infers progress from exit code,
stderr, or a rejected report. A validated science-complete receipt establishes
exactly evaluator 1, grouped oracle 1, head solves 9, base construction 1,
optimizer fit 1, steps 3,500, procedures 2, and seed count 1 even if a later
result-candidate validation or publication step terminalizes. Every terminal
state has `scientific_result_available=false`, hashes every available rejected
candidate, and forbids retry/resume.

The evaluator CLI is exactly:

`EVAL --development-only --train-input ABS --validation-input ABS --output ABS`

All paths are required, absolute, distinct, and fixed by the runner. Protected
or model/checkpoint options are rejected. No raw source, certified input,
final holdout, policy data, MDN, model checkpoint, capture, or representation
forward may be opened. Successful output remains development-only.
