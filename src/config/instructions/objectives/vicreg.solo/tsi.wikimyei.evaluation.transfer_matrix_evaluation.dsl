/*
  vicreg.solo objective-local transfer matrix evaluation payload.

  Keep this file colocated with the contract bundle for now so contract
  auto-discovery continues to attach the evaluation sidecar config. When the
  loader learns how to resolve these from ../../defaults directly, this file
  can collapse back into a default reference.
*/

check_temporal_order:bool = true
validate_vicreg_out:bool = true
report_shapes:bool = false
summary_every_steps(0,+inf):int = 256

# Optional probe override. When omitted, transfer-matrix evaluation uses all
# active observation feature dims for the current record_type. In vicreg.solo
# we coordinate it through the contract-scoped __future_target_dims variable so
# expected_value and transfer-matrix evaluation ask about the same future slice
# without introducing a runtime dependency between the modules.
mdn_target_dims:arr[int] = % __future_target_dims ? 3 %
