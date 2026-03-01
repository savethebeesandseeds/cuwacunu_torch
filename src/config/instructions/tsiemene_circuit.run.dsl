circuit_vicreg_run = {
  w_source = tsi.source.dataloader
  w_rep = tsi.wikimyei.representation.vicreg.0x0000
  sink_payload = tsi.sink.null
  sink_log = tsi.sink.log.sys
  w_source@payload:tensor -> w_rep@step
  w_rep@payload:tensor -> sink_payload@step
  w_source@meta:str -> sink_log@warn
  w_rep@meta:str -> sink_log@debug
}
