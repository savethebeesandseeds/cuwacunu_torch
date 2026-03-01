/*
  iitepi_circuit.dsl
  ====================
  Purpose:
    Static wiring/topology declaration for tsiemene runtime.
    This file defines instances and endpoint connections only.

  Scope:
    - Declare runtime instances by alias and canonical tsi type.
    - Declare directed hops between typed endpoints.
    - Keep topology immutable and reusable across train/run waves.

  Out of scope:
    - No data range or symbol/date invocation controls.
    - No epoch/batch budgets.
    - No training profile selection.
    Those are wave-level responsibilities.

  Syntax pattern:
    <circuit_name> = {
      <alias> = <canonical_tsi_type>
      ...
      <src_alias>@<src_port>[:<src_kind>] -> <dst_alias>@<dst_port>[:<dst_kind>]
      ...
    }

  Notes:
    - Alias is local to this circuit and used by hop declarations.
    - Canonical type identifies concrete runtime component.
    - Endpoint kinds should match semantic compatibility rules enforced by
      decode/validation stages.
*/

circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg.0x0000
  sink_null = tsi.sink.null
  sink_log = tsi.sink.log.sys
  w_source@payload:tensor -> w_rep@step
  w_rep@payload:tensor -> sink_null@step
  w_rep@loss:tensor -> sink_log@info
  w_source@meta:str -> sink_log@warn
  w_rep@meta:str -> sink_log@debug
}
