# VVA-1B Protocol Amendment A2 — Historical GPV Confirmation Receipt

Status: frozen before any VVA-1B authoritative optimizer update.

The base protocol requires both the settled VVA-1 and GPV-1 logs to report an
unopened confirmation split with zero rows. The immutable GPV-1 v2
authoritative implementation emits `gpv1.confirmation.opened=false` and
`gpv1.confirmation.optimizer_updates=0` when the confirmation routine is not
called, but it emits a `gpv1.confirmation.rows` field only in the mutually
exclusive opened branch (`opened=true`, `rows=256`). Its sealed branch therefore
has no literal `rows=0` field.

For the already-pinned GPV-1 v2 authoritative log only, the base requirement is
met by requiring all of the following exact receipts together:

- the authenticated GPV-1 source and authoritative-log hashes already pinned
  by the base protocol;
- `gpv1.confirmation.opened=false`;
- `gpv1.confirmation.optimizer_updates=0`;
- `gpv1.development.selected=none`;
- `gpv1.production_defaults_changed=false`; and
- `execution_status=gpv1_measurements_complete`.

VVA-1 continues to require both `vva1.confirmation.opened=false` and
`vva1.confirmation.rows=0` exactly as written in the base protocol. This
amendment changes no result, data split, gate, or VVA-1B experiment behavior; it
only makes the custody check match the immutable GPV-1 sealed-branch schema.
This amendment and its checksum are additional custody anchors.
