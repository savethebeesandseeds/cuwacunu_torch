active_circuit = circuit_1

circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg.0x0000
  sink_null = tsi.sink.null
  probe_repr = tsi.probe.representation.transfer_matrix_evaluation.0x0000
  probe_log = tsi.probe.log(mode=batch)
  w_source@response:cargo -> w_rep@impulse
  w_rep@response:cargo -> probe_repr@impulse
  w_rep@response:cargo -> sink_null@impulse
  w_rep@loss:tensor -> probe_log@info
  w_rep@meta:str -> probe_log@debug
}
