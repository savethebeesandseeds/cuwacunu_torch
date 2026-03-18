/*
  default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl
  ============================================================
  Purpose:
    Ephemeral transfer-matrix evaluation runtime knobs owned by
    tsi.wikimyei.representation.vicreg.

  Notes:
    - This is no longer a standalone tsi.probe component.
    - No standalone probe hashimyei/history state is produced.
    - Vicreg emits run-end summary + matrix `.lls` reports.

  Format:
    <key>(domain):<type> = <value>
*/

check_temporal_order:bool = true
validate_vicreg_out:bool = true
report_shapes:bool = false
summary_every_steps(0,+inf):int = 256
