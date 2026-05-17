/*
  Objective-local VICReg runtime wrapper for vicreg.solo.train.

  Architecture and docking stay frozen in ../../optim/, while jkimyei training
  policy is objective-local and intentionally reduced to the cleaned canonical
  train-plus-eval path for this objective.
*/

TAG:str = vicreg.solo.train
enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = ../../optim/optim.vicreg.solo.tsi.wikimyei.representation.encoding.vicreg.network_design.dsl
jkimyei_dsl_file:str = tsi.wikimyei.representation.encoding.vicreg.jkimyei.dsl
