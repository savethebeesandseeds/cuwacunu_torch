/*
  Objective-local VICReg runtime wrapper for vicreg.solo.promote_optim.

  Architecture and docking stay frozen in ../../optim/, while the local jkimyei
  policy surface remains available to describe the evaluation profile used by
  this objective.
*/

enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = ../../optim/optim.vicreg.solo.tsi.wikimyei.representation.vicreg.network_design.dsl
jkimyei_dsl_file:str = tsi.wikimyei.representation.vicreg.jkimyei.dsl
