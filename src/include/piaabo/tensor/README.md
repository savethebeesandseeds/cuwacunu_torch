# Piaabo Tensor

Tensor helpers live here when they are generic enough to help many callers.

Worker objectives, analytics identities, and runtime evidence do not belong in
clean tensor utilities.

Current rooms:

- `network_design/`: generic network assembly parser and IR used by model
  adapters.
- `torch/`: Torch device/dtype helpers and distribution shims.
