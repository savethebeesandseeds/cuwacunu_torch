/*
  vicreg.solo objective-local embedding sequence analytics payload.

  Keep this file colocated with the contract bundle for now so contract
  auto-discovery continues to attach the evaluation sidecar config. When the
  loader learns how to resolve these from ../../defaults directly, this file
  can collapse back into a default reference.
*/

max_samples(0,+inf):int = 4096
max_features(0,+inf):int = 2048
mask_epsilon[0,+inf):float = 1e-12
standardize_epsilon(0,+inf):float = 1e-8
