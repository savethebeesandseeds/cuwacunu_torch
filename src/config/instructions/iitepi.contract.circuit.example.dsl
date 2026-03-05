circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg.0x0000
  probe_repr = tsi.probe.representation.transfer_matrix_evaluation
  sink_null = tsi.sink.null
  sink_log = tsi.sink.log.sys
  w_source@response:cargo -> w_rep@impulse
  w_rep@response:cargo -> sink_null@impulse
  w_rep@response:cargo -> probe_repr@impulse
  w_rep@response:cargo -> sink_log@debug
  probe_repr@meta:str -> sink_log@debug
  w_rep@loss:tensor -> sink_log@info
  w_source@meta:str -> sink_log@warn
  w_rep@meta:str -> sink_log@debug
}
