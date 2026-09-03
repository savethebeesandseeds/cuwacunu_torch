# RMC-1 — Representation Module Certification Protocol

Status before execution: **sealed design; no RMC-1 endpoint observed**.

## 1. Claim and strict boundary

RMC-1 asks one question: does encoder training produce a cleaner and more
useful causal sequence representation than the exact same encoder at
initialization when both are read through the accepted
`structured_cdsb_sparse_v1` policy?

The only permitted path is:

```text
synthetic sequence -> optional training-only augmentation -> encoder
                   -> structured_cdsb_sparse_v1 -> frozen linear probes
```

NodeLift, graph construction, MDN, observer, policy, execution, checkpoint
migration, source/runtime jobs, and end-to-end evaluation are forbidden. Probe
weights are diagnostic and may never update the encoder.

Settled SRR-1 through SRR-4 conclusions are not reopened. In particular,
`all_tokens` is a damaged comparison surface and
`structured_cdsb_sparse_v1` has already passed its production mechanics and
representation-value qualification. RMC-1 performs only a small same-object
shape/mask/purity smoke check before using that readout.

## 2. Why bounded retraining is required

The prior outer-augmentation experiment retained complete metric logs but did
not serialize its paired per-arm encoder states. Those states therefore cannot
be re-read through the repaired policy. SRR-1 cannot substitute for them: its
three models were freshly constructed same-seed initializations and executed
zero optimizer steps.

Consequently the no-training stage is exhausted. RMC-1 authorizes exactly one
matched encoder-only development rerun. It does not authorize repeating the
known-unsafe full augmentation arm.

## 3. Frozen development experiment

- architecture: exact active `C=3,H=30,F=9,D=32` module;
- objective: accepted JEPA+MAE core only (`JEPA=1`, `MAE=0.25`, `TF=0`,
  outer VICReg `=0`);
- internal weak views: unchanged active settings;
- arms:
  1. `neutral`: production augmentation helper with every outer transform
     disabled;
  2. `qualified`: Gaussian jitter `0.001`, amplitude scale `[0.98,1.02]`, and
     frequency-gain jitter `0.01`; frequency masking, dilation, and warp are
     disabled;
- seeds: `17,31,47`;
- optimizer: Adam, learning rate `1e-3`;
- updates: exactly `32`, batch size `96`;
- update order: one Adam step followed by one target-EMA update;
- clean datasets: SSL group range from key `0`, probe fit from `1000000`,
  ridge selection from `2000000`, development gate from `3000000`, and
  confirmation from `4000000`;
- group counts: `256/256/128/256/256` respectively;
- normalization: fitted only on SSL rows;
- probe sample ladder: `32,64,128,256`;
- ridge grid: `[1e-5,1e-4,1e-3,1e-2,1e-1,1]`;
- uncertainty: deterministic 4096 paired group-bootstrap rows for the RMC
  endpoint; fixed-seed mean is the estimand.

Initialization, clean rows, optimizer, masks, target EMA, model-forward RNG,
and compute are paired. The exact production augmentation helper is called
training-only. Clean probe capture receives no augmentation.

## 4. Endpoints

The primary endpoint is clean macro predictive AULC across twelve sequence
targets and the fixed sample ladder. For each arm, compute
`step32 - same-seed step0` from predictions on the same held-out rows.

The following safeguards are conjunctive:

- all three seed gains are positive;
- mean gain is at least `+0.005` AULC and its paired 95% lower bound is above
  zero;
- at least three of four family point deltas are positive and no family point
  delta is below `-0.02`;
- final AULC is noninferior to the equal-width raw-history control with paired
  95% lower bound at least `-0.01`;
- exact reversal decoding has final AULC at least `0.90`, lower 95% bound at
  least `0.85`, and trained-minus-initialization lower bound at least `-0.02`;
- shuffled continuous-target upper 95% bound is at most `0.05`;
- shuffled reversal accuracy interval remains inside `[0.40,0.60]`;
- every seed/channel has centered effective-rank fraction at least `0.25`,
  participation-rank fraction at least `0.25`, largest-eigenvalue share at
  most `0.80`, and active-dimension fraction at least `0.75`.

The `+0.005` material floor is fixed before execution. It is larger than twice
the earlier augmentation replacement floor and many orders above deterministic
replay error. Loss reduction is never an endpoint.

If both arms pass, prefer neutral unless qualified-minus-neutral reaches
`+0.0024`, has a positive paired lower bound, and is positive in at least two
of three seeds. This prevents adding augmentation without demonstrated value.

## 5. Mechanical stop gate

Before any scientific classification, require:

- CUDA:0 deterministic execution and finite tensors/gradients;
- exact named-parameter initialization across arms within each seed;
- exact step-zero sparse values, probe predictions, selected penalties, order
  predictions, and geometry across arms;
- identical clean rows, clean masks, JEPA masks, weak-view support masks, and
  model-forward RNG schedule between neutral and qualified;
- neutral augmentation is byte identity;
- qualified augmentation changes valid values, preserves its mask exactly,
  preserves every terminal anchor, and never invents or removes support;
- one Adam and one EMA update per step, with no clipping;
- sparse output `[B,3,32]`, mask `[B,3]`, finiteness, all-channel validity on
  clean rows, deterministic replay, input purity, and parameter/RNG purity;
- target arrays are never passed to the encoder and shuffled-target controls
  remain null.

Any failure yields `invalid_mechanics`; endpoints cannot rescue it.

## 6. Development decisions and confirmation

- `neutral_candidate`: neutral passes all representation gates and qualified
  has no demonstrated advantage.
- `qualified_candidate`: qualified passes all representation gates and either
  neutral fails or qualified demonstrates the frozen augmentation advantage.
- `encoder_training_not_working`: neither arm passes. Do not open confirmation;
  repair the encoder objective or optimization inside this module.
- `invalid_mechanics`: repair the experiment or implementation before reading
  endpoints.

Only a selected development candidate may open confirmation. Reconstruct the
same-seed initializations, retain the trained candidate models, and evaluate
that one augmentation policy once on the untouched `4000000` group range.
Apply the same learned-gain, raw-control, order, shuffle, family, and geometry
gates without changing thresholds. A pass yields
`representation_certified`; a failure yields `confirmation_failed` and no
representation certificate.

## 7. Reporting and authorization

The final report must state, in plain language, whether training improved the
representation, which augmentation policy was selected, whether sequence order
and geometry survived, and the exact failed clause if certification did not
pass. It must show initialization, trained, raw-control, reversal, and geometry
numbers by seed in a compact table.

RMC-1 may freeze an encoder/readout/augmentation identity for subsequent work.
It may not construct or evaluate any downstream model. The explicit readout
rollback remains `all_tokens`.
