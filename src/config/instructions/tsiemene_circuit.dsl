circuit_1 = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg
  sink_null = tsi.sink.null
  sink_log = tsi.sink.log.sys
  w_source@payload:tensor -> w_rep@step
  w_rep@payload:tensor -> sink_null@step
  w_rep@loss:tensor -> sink_log@info
  w_source@meta:str -> sink_log@warn
  w_rep@meta:str -> sink_log@debug
}
circuit_1(BTCUSDT[01.01.2009,31.12.2009]);
