# FSPA-2 — Frozen Sequence Projection Alignment Convergence Protocol

Status before execution: **sealed design; no FSPA-2 endpoint observed**.

## Frozen question

FSPA-1 reduced its direct alignment loss but remained far from the fixed target
after 128 updates. FSPA-2 asks whether that failure is simply insufficient
optimization time.

Everything from FSPA-1 remains fixed except the training budget:

- identical architecture and `structured_cdsb_sparse_v1` readout;
- identical direct MSE to the same frozen orthonormal raw-history projection;
- no JEPA, MAE, TF, or VICReg optimizer contribution;
- neutral outer augmentation, with zero augmentation calls;
- Adam `1e-3`, batch size `96`, seeds `17,31,47`;
- identical SSL rows, normalization, row schedule, target projection, probe
  splits, ridge grid, 4096 bootstrap table, shuffle controls, and RMC gates;
- exactly **1,024 updates** from the same initialization in every seed.

No intermediate representation endpoint is inspected or selected. Development
is evaluated only after update 1,024, preventing checkpoint cherry-picking.

## Mechanics and decision

Require the FSPA-1 mechanics plus finite nonzero gradients/updates throughout,
first-eight loss greater than last-eight loss, exact inactivity of predictor,
MAE-decoder, and VICReg-head parameters, and exact reproduction of the three
initial structured AULCs.

Apply the complete, unchanged RMC learned-gain, family, raw-control, reversal,
shuffle, and geometry gate at development. If it fails, do not open
confirmation and conclude that longer convergence alone does not repair the
representation. If it passes, open the untouched confirmation rows once and
apply the same gate. A confirmation pass yields
`representation_certified_fspa_convergence_v1`.

No downstream model or end-to-end path is authorized.
