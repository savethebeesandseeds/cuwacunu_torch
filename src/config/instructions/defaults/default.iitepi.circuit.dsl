circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg
  w_ev = tsi.wikimyei.inference.mdn
  sink_null = tsi.sink.null
  probe_log = tsi.probe.log(mode=batch)
  w_source@response:cargo -> w_rep@impulse
  w_rep@response:cargo -> w_ev@impulse
  w_ev@response:cargo -> sink_null@impulse
  w_ev@future:tensor -> probe_log@info
  w_ev@loss:tensor -> probe_log@info
  w_ev@meta:str -> probe_log@debug
  w_rep@loss:tensor -> probe_log@info
  w_rep@meta:str -> probe_log@debug
}
