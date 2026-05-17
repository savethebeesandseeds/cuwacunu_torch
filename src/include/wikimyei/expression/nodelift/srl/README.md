# SRL NodeLift

`srl` is the short folder and namespace token for Synthetic Reference Lift.

The public header uses the full prose name:
`synthetic_reference_lift.h`.

The strict Synthetic Reference Lift core is the price-coordinate recovery
solve. In this v1 module, `srl` names the full feature-wise NodeLift
implementation:

- price coordinates by signed incidence recovery,
- uniform synthetic reference gauge per recoverable component,
- residual edge sidecars in full active edge order,
- activity coordinates by endpoint aggregation,
- support-normalized activity intensity for node features.

Public tensors use the full active edge order `L`; coordinatewise usable edge
sets are derived internally from masks.

V1 keeps the coordinate semantics fixed:

- price coordinates are `{0, 1, 2, 3}`;
- activity coordinates are `{4, 5, 6, 7, 8}`;
- `graph_t::node_ids` is required so `N` is stable even when some nodes are
  isolated at a particular anchor;
- price residuals are always returned because they are part of the NodeLift
  contract.

