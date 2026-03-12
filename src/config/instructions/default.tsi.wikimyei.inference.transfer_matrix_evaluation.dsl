/*
  default.tsi.wikimyei.inference.transfer_matrix_evaluation.dsl
  ============================================================
  Purpose:
    Ephemeral transfer-matrix evaluation runtime knobs owned by
    tsi.wikimyei.representation.vicreg.

  Notes:
    - This is no longer a standalone tsi.probe component.
    - No hashimyei-bound report persistence is produced.

  Format:
    <key>(domain):<type> = <value>
*/

check_temporal_order:bool = true
validate_vicreg_out:bool = true
report_shapes:bool = false
summary_every_steps(0,+inf):int = 256
