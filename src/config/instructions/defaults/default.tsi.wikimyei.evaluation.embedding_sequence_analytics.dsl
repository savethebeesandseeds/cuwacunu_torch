/*
  default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl
  =================================================================
  Purpose:
    Ephemeral embedding-sequence analytics runtime knobs owned by
    tsi.wikimyei.representation.encoding.vicreg.

  Notes:
    - This config controls run-end embedding analytics sidecars emitted
      from VICReg encodings.
    - Numeric/symbolic analytics kernels remain in piaabo::torch_compat.

  Format:
    <key>(domain):<type> = <value>
*/

max_samples(0,+inf):int = 4096
max_features(0,+inf):int = 2048
mask_epsilon[0,+inf):float = 1e-12
standardize_epsilon(0,+inf):float = 1e-8
