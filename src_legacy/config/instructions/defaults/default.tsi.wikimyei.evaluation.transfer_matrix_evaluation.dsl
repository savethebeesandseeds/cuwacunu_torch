/*
  default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl
  ============================================================
  Purpose:
    Ephemeral transfer-matrix evaluation runtime knobs owned by
    tsi.wikimyei.representation.encoding.vicreg.

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

# Optional probe override. The default instruction binds the contract-owned
# target selection; if the key is omitted entirely, transfer-matrix evaluation
# falls back to all active observation feature indices for the current
# record_type.
mdn_target_feature_indices:arr[int] = % __future_target_feature_indices ? 3 %
