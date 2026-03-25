circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg
  sink_null = tsi.sink.null
  probe_log = tsi.probe.log(mode=batch)
  w_source@response:cargo -> w_rep@impulse
  w_rep@response:cargo -> sink_null@impulse
  w_rep@meta:str -> probe_log@debug
}
