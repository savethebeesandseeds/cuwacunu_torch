# Representation/interface localization addendum

Status: the localization design, raw 96-value arm, untrained seed-17 control,
and certified-exposure hardening were first fixed on 2026-07-16 at artifact
hash `756fca5ba6ffee50bcee3f6ed9bd360a95f8ed1b7e409e4102774394e4a29d2d`,
before the v2 representation training run completed and before any v2
production MDN checkpoint existed. The raw-history production-MDN isolation
result was already known; no frozen representation/interface result was.

This revision was made after canonical MDN training had started and
intermediate train-core checkpoints and running train metrics existed, but
before any validation or certified representation forecast, frozen-feature
capture, affine result, or certified-development access. It adds the
train-only numerical-candidate rejection rule and clarifies transport hashing
versus semantic certified parsing and the limits of an affine failure. It does
not change any feature arm, split, ridge value, selection priority, success
gate, model checkpoint, or final-holdout rule.

Implementation review established that the preregistration's 384 values
described as "base representation, quote representation, and their
difference" are not raw encoder values. They are produced only after the
learned MDN backbone, learned channel adapters, and direct-head adapter; width
128 is the MDN hidden width. That arm therefore tests the representation plus
the learned MDN pre-head interface. It remains the preregistered primary arm,
with its ridge grid, selection rule, success gates, and interpretation
unchanged. This addendum adds the distinct raw-encoder arm required to localize
a failure correctly.

## Secondary pure-encoder affine decoder

The same three immutable capture jobs emit
`representation_edge_features.probe` alongside the primary
`mdn_edge_context_features.probe`. The secondary probe contains 96 values per
row: base encoder output, quote encoder output, and base-minus-quote, each of
width 32. It contains no MDN channel-adapter or learned edge-identity values.

The secondary decoder uses exactly the primary decoder's split discipline,
train-only standardization, per-edge affine structure with channels pooled,
float64 ridge grid, validation selection order, no-refit rule, certified
single-candidate confirmation, metrics, and strong and partial gates. It is a
localization diagnostic; the post-adapter 384-value decoder remains primary.

The declared ridge grid remains fixed. A candidate whose train-only float64
factorization fails, produces nonfinite coefficients, or has normalized linear
solve residual above `1e-7` is marked numerically invalid before certified
input is parsed; it is not replaced or assigned a metric. Selection uses the
declared lexicographic order over valid candidates by a deterministic
left-to-right incumbent scan with comparison tolerance `1e-12`, followed by
the smaller ridge value. If every candidate is invalid, the procedure fails
closed before certified scoring.

## Untrained-representation control

To retain the original fresh-seed evidence ladder, a deterministic
production-initialized representation using the representation seed 17 and no
representation checkpoint is
captured on train `[0,2496)` and validation `[2560,2816)` only. Its raw
96-value representation probe is fitted and validation-selected with the same
standardization, affine structure, ridge grid, and selection order. It never
opens certified development. The trained-versus-untrained comparison is
reported on validation as a control for whether representation training added
forecast-decodable information; it is not used to select the trained model or
to alter any gate.

Interpretation is fixed as follows:

- Pure encoder succeeds but post-adapter fails: the MDN channel adapter or its
  exposed interface destroys usable information.
- Both pure encoder and post-adapter affine arms fail: forecast signal is not
  linearly exposed under the fixed decoder. This supports investigating the
  encoder objective/input construction, but does not by itself prove that no
  nonlinear decoder could recover the information.
- Both succeed but the production learned readout fails: information is
  exposed and the learned readout/objective/optimization path is faulty.
- Post-adapter succeeds: the adapter does not impose the observed ceiling,
  regardless of the secondary arm.

## Certified-exposure hardening

Before any certified capture begins, the capture runner freezes and hashes
the affine evaluator source and runner that define both decoder arms. It also
publishes an immutable certified-attempt receipt before invoking Runtime. If
that receipt exists without a complete certified job, the capture is failed
closed and cannot be retried or overwritten. Thus certified development can
be exposed at most once and only after the candidate logic is fixed.

Transport-integrity validation may stat, hash, and schema-check the immutable
certified probe after Runtime creates it. "Open only after selection" refers
to semantic parsing of its feature and target rows by the affine selector: the
evaluator reads train and validation, fixes the ridge candidate, and only then
parses and scores certified rows. Integrity hashing is never an input to
candidate selection.

No arm reads, scores, fits, tunes, or authorizes access to `[3328,4096)`.
