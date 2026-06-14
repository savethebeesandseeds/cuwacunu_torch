#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cuwacunu::hero::marshal {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set "
    "protocol_layer=STDIO.";

struct marshal_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
};

inline constexpr std::size_t kMarshalToolDescriptorCount = 0
#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON) +1
#include "hero/marshal_hero/hero_marshal.def"
#undef HERO_MARSHAL_TOOL
    ;

inline constexpr std::array<marshal_tool_descriptor_t,
                            kMarshalToolDescriptorCount>
    kMarshalToolDescriptors = {{
#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON)                \
  marshal_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON},
#include "hero/marshal_hero/hero_marshal.def"
#undef HERO_MARSHAL_TOOL
    }};

inline constexpr std::string_view kMarshalHeroBoundaryText =
    "Marshal prepares bounded handoffs and explains state. Runtime executes "
    "and writes evidence. Lattice proves target satisfaction.";

inline constexpr std::string_view kMarshalPolicyTemplateText = R"(
# Marshal Hero v0 policy
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
  policy_set = numeraire,current_weight,equal_weight,sdu
  max_steps = 250
  max_parallel_jobs = 4
  runtime_exec_path = /cuwacunu/.build/exec/cuwacunu_exec
  timeout_seconds = 86400
  execution_backend_id = cajtucu.execution.paper.v1
  cost_model_id = linear_transaction_cost_rate.v1
  allow_synthetic_direct_edges = false
  synthetic_edge_research_reason =
  linear_transaction_cost_rate = 0.001
  allow_partial_fills = false
  equity_mismatch_tolerance = 0.000001
  equity_mismatch_fail_tolerance = 0.01
  live_execution_allowed = false
}
)";

struct marshal_policy_t {
  std::filesystem::path policy_path{};
  std::filesystem::path global_config_path{};
  std::string prepare_profile_id{"single_wave_operator"};
  std::string rollout_profile_id{"replay_validation_default"};
  std::unordered_map<std::string, std::string> values{};
  std::map<std::string, std::map<std::string, std::string>>
      prepare_profile_fields{};
  std::map<std::string, std::map<std::string, std::string>>
      rollout_profile_fields{};
  bool from_template{false};
};

struct marshal_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path policy_path{};
  marshal_policy_t policy{};
};

} // namespace cuwacunu::hero::marshal
