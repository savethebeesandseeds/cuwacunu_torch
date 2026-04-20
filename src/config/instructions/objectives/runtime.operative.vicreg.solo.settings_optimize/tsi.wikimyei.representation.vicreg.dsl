/*
  tsi.wikimyei.representation.vicreg.dsl
  ===================
  Purpose:
    Objective-local VICReg runtime wrapper used by contract discovery.
    Objective-local network_design remains the sole authored source for VICReg
    architecture so imported contract variants can sweep private
    encoder/projector settings while keeping the public observation dock
    fixed. Jkimyei policy still defaults to ../../defaults until we start a
    dedicated optimizer/augmentation sweep.
    Typed key-value profile for VICReg runtime/binding settings used by
    wikimyei.
    Values are explicit and parsed as: key(domain):type = value.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment
    Supported type examples used here:
      int, bool, str

  Key groups:
    - Runtime placement: device, dtype
    - Required architecture payload binding: network_design_dsl_file
    - Required jkimyei payload binding: jkimyei_dsl_file
    - Training budget: wave-owned (not configured in this file)
    - Runtime behavior: enable_buffer_averaging

  Semantics:
    - This file defines component-local VICReg runtime configuration and
      payload bindings.
    - VICReg architecture is authored only in `network_design_dsl_file`.
    - Contract-scoped `__variables` still own the public docking widths there:
      input shape (`__obs_channels`, `__obs_seq_length`, `__obs_feature_dim`)
      and encoder output width (`__embedding_dims`).
    - Wave + jkimyei profile selection determine whether and how training is executed.
    - Training enable/disable is controlled by wave root `MODE` + `WIKIMYEI ... JKIMYEI.HALT_TRAIN`.
    - Profile policy knobs (`swa_start_iter`, `optimizer_threshold_reset`) are
      loaded from `jkimyei_dsl_file`.
*/

# VICReg profile: key(domain):type = value # optional comment
enable_buffer_averaging:bool = false # SWA buffer averaging policy
dtype[kFloat16|kFloat32|kFloat64]:str = kFloat32 # torch dtype token
device[cpu|gpu|cuda:0]:str = gpu # cpu | cuda:0 | gpu
network_design_dsl_file:str = tsi.wikimyei.representation.vicreg.network_design.dsl
jkimyei_dsl_file:str = ../../defaults/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl
