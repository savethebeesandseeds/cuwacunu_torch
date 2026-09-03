# Isolated Representation Module Certificate

Certificate identifier:
`representation_certified_fspa4_minimal_spectral_repair_v1`

Date: 2026-08-28

## Certified configuration

- encoder: existing MTF JEPA/MAE/VICReg representation architecture;
- serving readout: `structured_cdsb_sparse_v1`;
- outer augmentation policy: neutral, zero augmentation calls;
- teacher phase: 1,024 updates of direct MSE to the fixed orthonormal
  per-channel raw-sequence projection;
- repair target: SSL-only minimal spectral cap to participation `0.30`;
- absorption phase: 512 served-encoder-only distillation updates;
- model seeds: `17,31,47`;
- training labels: none;
- downstream models constructed: zero.

## Certification statement

Under the sealed RMC protocol, the configuration above passed development and
untouched confirmation. On confirmation, all three seeds improved over
identical initialization; the mean gain was `+0.052797` with paired 95% lower
bound `+0.037586`. It also exceeded the frozen raw control with lower bound
`+0.000826`, retained causal reversal performance, passed both shuffle controls,
met the family safeguards, and passed every geometry threshold in all nine
seed/channel cases.

The final served representation is `[B,3,32]` from the unchanged structured
readout. No whitening or learned head is required at inference.

## Exact boundary of the claim

This is a representation-only certificate on the sealed causal sequence
benchmark. It establishes that the encoder architecture and structured readout
can learn and serve a useful, non-collapsed sequence representation under the
certified recipe. It does not validate the legacy training objective,
non-neutral augmentations, downstream compatibility, or end-to-end behavior.

The complete measurements are recorded in
`MINIMAL_PARTICIPATION_SPECTRAL_REPAIR_FINDINGS.md`; the precommitted design is
sealed by protocol SHA-256
`4cf4f81ffac1665f85bd233203ccf2f039617ec8d52b41a40258002b42999b00`.
