# VVA-1B Protocol Amendment A1 — Bootstrap Cardinality Correction

Status: frozen before any VVA-1B authoritative optimizer update.

This amendment corrects one textual cardinality error in
`VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.md`. It does not change the frozen
bootstrap table, bootstrap seed, development rows, estimand, endpoints,
thresholds, training budget, or decision rules.

The base protocol pins `bootstrap.table=408205cac33d403d`. That hash identifies
the existing deterministic table returned by `rssm_bootstrap_rows(256)`:
`256` is the number of development groups sampled within each replicate, while
`kRssmBootstrapReplicates` is `512`. The independently reconstructed SRR
auditor also binds this exact 512-by-256 table to the same hash. In contrast,
`rmc_bootstrap_rows(256)` produces a different 4,096-replicate table and is not
the table named by the frozen hash.

Accordingly, this amendment overrides only these two phrases in the base
protocol:

- “the exact inherited 256-replicate table over 256 development groups” means
  “the exact inherited 512-replicate table over 256 development groups”; and
- “the existing deterministic 256-replicate paired generated-group bootstrap”
  means “the existing deterministic 512-replicate paired generated-group
  bootstrap.”

The executable must construct the table with `rssm_bootstrap_rows(256)`, verify
it with `rssm_bootstrap_contract(..., 256)`, require hash
`408205cac33d403d`, and emit `bootstrap_replicates=512`. Any other table or hash
is a pre-optimizer STOP. The base protocol and its checksum remain immutable;
this amendment and its checksum are additional custody anchors.
