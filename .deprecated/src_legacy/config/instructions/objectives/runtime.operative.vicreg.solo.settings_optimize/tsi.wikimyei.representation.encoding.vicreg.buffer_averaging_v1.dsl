/*
  Objective-local VICReg runtime wrapper with buffer averaging enabled.

  Hypothesis:
    the strongest current architecture still degrades on held-out payloads
    because embedding statistics are too noisy at evaluation time. Enable
    VICReg buffer averaging as an objective-local option-policy test while
    keeping network design and jkimyei policy otherwise unchanged.
*/

# VICReg profile: key(domain):type = value # optional comment
TAG:str = vicreg.solo.settings_optimize.buffer_averaging_v1
enable_buffer_averaging:bool = true # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = tsi.wikimyei.representation.encoding.vicreg.network_design.dsl
jkimyei_dsl_file:str = ../../defaults/default.tsi.wikimyei.representation.encoding.vicreg.jkimyei.dsl
