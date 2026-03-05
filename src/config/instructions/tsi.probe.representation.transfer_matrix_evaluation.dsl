/*
  tsi.probe.representation.transfer_matrix_evaluation.dsl
  ==============================
  Purpose:
    Runtime diagnostics policy for tsi.probe.representation.transfer_matrix_evaluation.
    This probe checks cargo sanity and anti-look-ahead guardrails while we build
    full transfer-matrix supervised evaluation flows.

  Format:
    <key>:<type> = <value>
*/

check_temporal_order:bool = true
validate_vicreg_out:bool = true
report_shapes:bool = false
