# Marshal Hero v0 policy
#
# Marshal Hero is a deterministic coordination and explanation surface. Runtime
# Hero owns execution policy and mutation. Lattice Hero owns proof authority.
# This file exists for Hero policy-path symmetry; it does not grant Marshal
# independent execution, scheduling, proof, model-selection, or checkpoint
# selection authority.
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
prepare_profile:enum = single_wave_operator
rollout_profile:enum = replay_validation_default

MARSHAL_PREPARE_PROFILE single_wave_operator {
  drive_mode = one_step
  max_waves = 1
  max_wall_clock_seconds = 0
  allow_execute = true
  allow_train_execute = true
  require_runtime_job_completion = true
  require_post_wave_lattice_satisfied_check = true
  stop_on_warning_severity =
  stop_on_lattice_warning = false
  stop_on_runtime_warning = false
  no_progress_window = 1
  materialize_plan_inputs = true
  timeout_seconds = 600
}

MARSHAL_PREPARE_PROFILE bounded_operator {
  drive_mode = budgeted
  max_waves = 3
  max_wall_clock_seconds = 86400
  allow_execute = true
  allow_train_execute = true
  require_runtime_job_completion = true
  require_post_wave_lattice_satisfied_check = true
  stop_on_warning_severity =
  stop_on_lattice_warning = false
  stop_on_runtime_warning = false
  no_progress_window = 1
  materialize_plan_inputs = true
  timeout_seconds = 86400
}

MARSHAL_ROLLOUT_PROFILE replay_validation_default {
}
