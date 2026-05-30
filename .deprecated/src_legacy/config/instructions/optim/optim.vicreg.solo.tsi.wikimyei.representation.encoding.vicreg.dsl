/*
  Frozen optimized VICReg runtime wrapper for vicreg.solo.

  The promoted network design is explicit in the referenced network DSL, so
  this wrapper stays intentionally small and stable.
*/

TAG:str = vicreg.solo.optim
enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = optim.vicreg.solo.tsi.wikimyei.representation.encoding.vicreg.network_design.dsl
jkimyei_dsl_file:str = ../defaults/default.tsi.wikimyei.representation.encoding.vicreg.jkimyei.dsl
