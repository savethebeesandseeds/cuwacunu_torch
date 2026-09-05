# LSA-0 — Clean-Identity Covariance Admission Audit

Registered 2026-09-05, before new measurements. This is a bounded directionality
audit, not a quality certificate or causal training experiment.

## Frozen scope

- Seeds 17, 31, 47; authenticated FSPA-4 archives and GPV mask-2 v2 endpoints.
- Existing RMC normalized synthetic data and masks; confirmation stays unopened.
- GPV global pool, the same three live Linear layers without the two GELUs,
  coupled variance, weights 25/25/1 and existing 0.05 * 0.25 outer scaling.
- Clean-identical inputs, evaluation mode, ordinary sparse structured readout.
  No augmentation, other objectives, optimizer, EMA update, or checkpoint write.
- Each gradient is the arithmetic mean over the existing SSL training-row
  schedules at steps 0, 255, 511, separately at each retained state.
- Compute invariance, variance, covariance and full-loss gradients separately.
  Require finite values, exactly equal projected views and zero invariance;
  weighted components must reconstruct full gradients with maximum residual
  <= 5e-5 and relative L2 residual <= 1e-4. Report trunk, tokenizer, encoder and
  projector norms and trunk cosines. Raw size alone does not decide admission.

## Protected virtual directions

On disposable copies only, subtract the variance, covariance, and full-loss
trunk gradients after normalizing each to the same parameter displacement:
0.0005 and 0.001 times the original trunk parameter L2 norm. These two fixed
radii check direction stability; they are not a step-size search. Projector,
target, and all other parameters remain fixed. No Adam approximation is claimed.

Measure the first 256 development rows and their existing reversed counterparts,
without labels, fitting probes, or accessing confirmation. Protected diagnostics:

1. Per-channel participation ratio: trace(cov)^2 / trace(cov^2) / 32.
2. Order separation: mean squared clean-versus-reversed representation distance
   divided by clean across-sample centered energy.
3. Channel contrast: centered representation energy after removing the per-row
   channel mean, divided by centered energy before removing that mean.

These are label-free structural proxies, not the sealed probe quality gates.
Report every baseline, virtual value, and relative change; all denominators must
be positive and finite. Exact repeated baseline evaluation must reproduce values.

## Admission rule

At either retained state, admit one later matched covariance-on/off comparison
only if the SAME protected diagnostic meets all following requirements in at
least two of three seeds, at BOTH radii:

- covariance-only direction decreases that diagnostic by at least 1% relative;
- its decrease exceeds variance-only's decrease by at least 0.5 percentage point;
- the full-loss direction also decreases that diagnostic;
- covariance/full weighted trunk norm ratio >= 0.01 and cosine(covariance,
  variance) < 0.95, establishing non-negligible and distinguishable direction.

All mechanics/custody checks must pass. Otherwise report `not_admitted`;
mechanical or custody failure reports `invalid`, never a scientific rejection.
This conservative budget gate cannot exclude Adam rescaling, intermediate-state
effects, or adverse directions outside these proxies. Do not claim covariance
innocence or causal harm from either outcome. No encoder training is part of
LSA-0; admission would require a separately sealed LSA-1 protocol.

## Custody and state

Authenticate all six archives against the retained hashes, the GPV authority
log against its findings hash, and endpoint seed/mask/config/data metadata.
The new loader must fail closed without deleting or repairing any artifact.
Preserve historical GPV source and binary; report this source, executable and
protocol digests separately. Verify reference parameters, buffers, target state,
gradient slots, and CPU/CUDA RNG remain unchanged; restore RNG explicitly even
after loading/cloning. No optimizer is constructed; archived GPV endpoints do not
contain Adam state, so no historical optimizer-state comparison is claimed.

Store a separate audit log and concise findings, including every seed/state and
the distinction between directional evidence and training causality.
