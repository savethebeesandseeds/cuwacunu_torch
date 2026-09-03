# FSPA-1 — Frozen Sequence Projection Alignment Repair Protocol

Status before execution: **sealed design; no FSPA-1 endpoint observed**.

## Purpose

RMC-1 proved that the structured readout works but current JEPA+MAE updates
slightly reduce clean representation quality and concentrate geometry. FSPA-1
tests the smallest direct objective repair: make the served structured vector
retain a frozen causal projection of the same input history.

The experiment remains strictly:

```text
clean sequence -> encoder -> structured_cdsb_sparse_v1
               -> fixed information-preservation loss / frozen probes
```

No labels enter encoder training. No downstream model, graph, MDN, observer,
policy, execution job, or end-to-end path may be constructed.

## Frozen alignment objective

For normalized clean input `x` with shape `[B,3,30,9]`, flatten each channel to
`[B,3,270]`. Let `Q_raw` be the existing deterministic equal-width orthonormal
projection returned by `make_raw_equal_width_projection`, shape `[270,32]`.
The target is:

```text
t = reshape_channel(x) @ Q_raw              # [B,3,32]
r = structured_cdsb_sparse_v1(encoder(x))   # [B,3,32]
loss = mean((r - stop_gradient(t))^2)
```

`Q_raw` is fixed, consumes no RNG, has orthonormal columns, and receives no
gradient. The loss is applied directly to the production-shaped structured
representation; there is no trainable projection or decoder that could hide
information outside the served surface. Only clean history is used, so the
objective is causal at the representation endpoint.

JEPA, MAE, TF alignment, and VICReg contribute zero optimizer weight in this
repair. Their parameters may exist in the unchanged module but must not update.

## Frozen training and data

- exact active `C=3,H=30,F=9,D=32` architecture;
- outer augmentation policy: neutral identity;
- seeds: `17,31,47`;
- 128 Adam updates, learning rate `1e-3`, batch size `96`;
- deterministic paired row schedule inherited from RMC-1;
- one optimizer step followed by one target-EMA update per iteration;
- SSL rows and normalization: RMC-1 key range `0`, 256 groups;
- probe fit/selection/development: `1000000/2000000/3000000` with
  `256/128/256` groups;
- confirmation key range `4000000`, 256 groups, remains unopened unless the
  development gate passes;
- sparse readout, probes, sample ladder, ridge grid, 4096 bootstrap table,
  shuffle controls, geometry definitions, and every numerical acceptance
  threshold are exactly RMC-1.

## Mechanical gate

Require protocol identity, deterministic CUDA execution, exact same-seed
initialization, finite targets/outputs/losses/gradients, all-valid sparse masks,
strictly decreasing first-eight versus last-eight mean alignment loss, nonzero
served-parameter updates, and exact non-update of predictor, MAE decoder, and
VICReg-head parameters. Training targets, probe targets, and confirmation rows
must be disjoint. No augmentation call or downstream construction is allowed.

The three initial structured AULCs must reproduce RMC-1 within `1e-9`:
`0.603103387209307`, `0.583348781945863`, and `0.592733129656351`.

Any mechanics failure yields `invalid_mechanics` and forbids endpoint use.

## Scientific gate and stop rule

Apply the complete RMC-1 candidate gate without modification:

- trained-minus-initialization point at least `+0.005`, paired lower bound
  above zero, and positive in all three seeds;
- at least three positive family deltas and none below `-0.02`;
- final-minus-raw lower bound at least `-0.01`;
- reversal point/lower bound at least `0.90/0.85` and retention lower bound at
  least `-0.02`;
- continuous and reversal shuffle controls pass;
- every seed/channel passes effective rank, participation rank, top-eigenvalue,
  and active-dimension gates.

If development fails, do not open confirmation and classify the exact failed
clause. If development passes, evaluate the retained three trained models and
their same-seed initializations once on confirmation. The same gate must pass
again for `representation_certified_fspa_v1`.

Passing FSPA-1 establishes that the encoder/readout can be trained to preserve
and expose sequence information better than initialization under a causal,
label-free objective. It does not claim that a fixed raw projection is the
final optimal semantic objective; it provides a valid certified representation
baseline from which richer objectives must demonstrate additional value.
