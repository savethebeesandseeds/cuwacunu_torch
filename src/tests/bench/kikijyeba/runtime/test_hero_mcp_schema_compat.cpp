#include "hero/mcp_schema_compat.h"
#include "tests/bench/kikijyeba/test_support/lattice_forecast_artifact_fixture.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace lattice_fixture = cuwacunu::tests::kikijyeba::lattice_fixture;
namespace exposure = cuwacunu::hero::lattice::exposure;
namespace schema = cuwacunu::hero::mcp_schema_compat;
namespace split = cuwacunu::hero::lattice::split;
namespace fs = std::filesystem;

namespace {

struct tool_schema_t {
  std::string name{};
  std::string input_schema{};
};

struct command_result_t {
  int status{0};
  std::string output{};
};

[[nodiscard]] command_result_t run_command_capture(const std::string &command) {
  std::array<char, 4096> buffer{};
  FILE *pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen failed for command: " + command);
  }
  command_result_t result{};
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    result.output.append(buffer.data());
  }
  result.status = pclose(pipe);
  return result;
}

[[nodiscard]] std::string read_command_stdout(const std::string &command) {
  const auto result = run_command_capture(command);
  if (result.status != 0) {
    throw std::runtime_error("command failed: " + command + "\n" +
                             result.output);
  }
  return result.output;
}

[[nodiscard]] fs::path first_existing(std::vector<fs::path> candidates) {
  for (const auto &candidate : candidates) {
    if (fs::exists(candidate)) {
      return candidate;
    }
  }
  std::ostringstream msg;
  msg << "missing expected Hero binary:";
  for (const auto &candidate : candidates) {
    msg << " " << candidate.string();
  }
  throw std::runtime_error(msg.str());
}

void skip_ws(const std::string &s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] std::string parse_json_string_at(const std::string &s,
                                               std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size() || s[*idx] != '"') {
    throw std::runtime_error("expected JSON string");
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[*idx];
    ++(*idx);
    if (c == '"') {
      return value;
    }
    if (c == '\\') {
      if (*idx >= s.size()) {
        throw std::runtime_error("unterminated JSON escape");
      }
      value.push_back(s[*idx]);
      ++(*idx);
      continue;
    }
    value.push_back(c);
  }
  throw std::runtime_error("unterminated JSON string");
}

[[nodiscard]] std::string capture_json_object_at(const std::string &s,
                                                 std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size() || s[*idx] != '{') {
    throw std::runtime_error("expected JSON object");
  }
  const std::size_t begin = *idx;
  int depth = 0;
  while (*idx < s.size()) {
    if (s[*idx] == '"') {
      (void)parse_json_string_at(s, idx);
      continue;
    }
    if (s[*idx] == '{') {
      ++depth;
    } else if (s[*idx] == '}') {
      --depth;
      if (depth == 0) {
        ++(*idx);
        return s.substr(begin, *idx - begin);
      }
    }
    ++(*idx);
  }
  throw std::runtime_error("unterminated JSON object");
}

[[nodiscard]] std::vector<tool_schema_t>
extract_tool_schemas(const std::string &catalog_json) {
  std::vector<tool_schema_t> out;
  std::size_t pos = 0;
  while ((pos = catalog_json.find("\"name\"", pos)) != std::string::npos) {
    pos = catalog_json.find(':', pos);
    if (pos == std::string::npos) {
      throw std::runtime_error("tool name missing ':'");
    }
    ++pos;
    tool_schema_t row{};
    row.name = parse_json_string_at(catalog_json, &pos);
    const std::size_t schema_key = catalog_json.find("\"inputSchema\"", pos);
    if (schema_key == std::string::npos) {
      throw std::runtime_error("tool inputSchema missing for " + row.name);
    }
    pos = catalog_json.find(':', schema_key);
    if (pos == std::string::npos) {
      throw std::runtime_error("tool inputSchema missing ':' for " + row.name);
    }
    ++pos;
    row.input_schema = capture_json_object_at(catalog_json, &pos);
    out.push_back(std::move(row));
  }
  return out;
}

void require_contains(const std::string &output, const std::string &needle,
                      const std::string &message);

void check_catalog(const std::string &label, const fs::path &binary,
                   bool require_lattice_checkpoint_closure) {
  const std::string output = read_command_stdout(
      binary.string() + " --global-config /cuwacunu/src/config/.config "
                        "--list-tools-json");
  const auto tools = extract_tool_schemas(output);
  assert(!tools.empty());
  bool saw_lattice_status = false;
  bool saw_lattice_inspect = false;
  bool saw_lattice_evaluate = false;
  bool saw_lattice_compare = false;
  int lattice_public_tool_count = 0;
  bool saw_config_status = false;
  bool saw_config_inspect = false;
  bool saw_config_apply = false;
  bool saw_runtime_status = false;
  bool saw_runtime_inspect = false;
  bool saw_runtime_run = false;
  bool saw_runtime_reset = false;
  for (const auto &tool : tools) {
    const auto issues =
        schema::validate_tool_input_schema(tool.name, tool.input_schema);
    if (!issues.empty()) {
      throw std::runtime_error(label + " schema compatibility failure: " +
                               schema::format_issue(issues.front()));
    }
    if (tool.name.rfind("hero.lattice.", 0) == 0) {
      ++lattice_public_tool_count;
      if (tool.name == "hero.lattice.status") {
        saw_lattice_status = true;
      } else if (tool.name == "hero.lattice.inspect") {
        saw_lattice_inspect = true;
      } else if (tool.name == "hero.lattice.evaluate") {
        saw_lattice_evaluate = true;
      } else if (tool.name == "hero.lattice.compare") {
        saw_lattice_compare = true;
      } else {
        throw std::runtime_error(
            "Lattice catalog still exposes retired tool: " + tool.name);
      }
    }
    if (tool.name == "hero.config.status") {
      saw_config_status = true;
    }
    if (tool.name == "hero.config.inspect") {
      saw_config_inspect = true;
    }
    if (tool.name == "hero.config.apply") {
      saw_config_apply = true;
    }
    if (tool.name == "hero.runtime.status") {
      saw_runtime_status = true;
    }
    if (tool.name == "hero.runtime.inspect") {
      saw_runtime_inspect = true;
    }
    if (tool.name == "hero.runtime.run") {
      saw_runtime_run = true;
    }
    if (tool.name == "hero.runtime.reset") {
      saw_runtime_reset = true;
    }
    if (label == "Config") {
      for (const auto retired_name :
           {"hero.config.schema", "hero.config.show", "hero.config.get",
            "hero.config.set", "hero.config.validate", "hero.config.map",
            "hero.config.capture_bundle", "hero.config.resolve",
            "hero.config.diff", "hero.config.save", "hero.config.reload",
            "hero.config.backups", "hero.config.rollback", "hero.config.list",
            "hero.config.read", "hero.config.write", "hero.config.delete"}) {
        if (tool.name == retired_name) {
          throw std::runtime_error(
              "Config catalog still exposes retired tool: " + tool.name);
        }
      }
    }
    if (label == "Runtime") {
      for (const auto retired_name :
           {"hero.runtime.schema", "hero.runtime.wave", "hero.runtime.dry_run",
            "hero.runtime.execute", "hero.runtime.replay",
            "hero.runtime.dev_nuke", "hero.runtime.list_jobs",
            "hero.runtime.get_job", "hero.runtime.read_artifact"}) {
        if (tool.name == retired_name) {
          throw std::runtime_error(
              "Runtime catalog still exposes retired tool: " + tool.name);
        }
      }
    }
  }
  if (label == "Config" && (tools.size() != 3U || !saw_config_status ||
                            !saw_config_inspect || !saw_config_apply)) {
    throw std::runtime_error(
        "Config catalog must expose exactly hero.config.status, "
        "hero.config.inspect, and hero.config.apply");
  }
  if (label == "Runtime" &&
      (tools.size() != 4U || !saw_runtime_status || !saw_runtime_inspect ||
       !saw_runtime_run || !saw_runtime_reset)) {
    throw std::runtime_error(
        "Runtime catalog must expose exactly hero.runtime.status, "
        "hero.runtime.inspect, hero.runtime.run, and hero.runtime.reset");
  }
  if (require_lattice_checkpoint_closure &&
      (!saw_lattice_status || !saw_lattice_inspect || !saw_lattice_evaluate ||
       !saw_lattice_compare || lattice_public_tool_count != 4)) {
    throw std::runtime_error(
        "Lattice catalog must expose exactly status/inspect/evaluate/compare");
  }
}

void check_config_collapsed_surface(const fs::path &binary) {
  const std::string base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";

  const std::string schema_output =
      read_command_stdout(base + "--tool hero.config.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  require_contains(schema_output, "\"keys\":[",
                   "Config inspect subject=schema should return schema keys");

  const std::string value_output = read_command_stdout(
      base + "--tool hero.config.inspect --args-json "
             "'{\"subject\":\"value\",\"key\":\"protocol_layer\"}'");
  require_contains(value_output, "\"key\":\"protocol_layer\"",
                   "Config inspect subject=value should read policy values");
  require_contains(value_output, "\"value\":\"STDIO\"",
                   "Config inspect subject=value should expose protocol_layer");

  const std::string plan_output = read_command_stdout(
      base + "--tool hero.config.apply --args-json "
             "'{\"operation\":\"set\",\"requested_mode\":\"plan\","
             "\"key\":\"protocol_layer\",\"value\":\"STDIO\"}'");
  require_contains(plan_output, "\"planned\":true",
                   "Config apply requested_mode=plan should not mutate");
  require_contains(plan_output, "\"operation\":\"set\"",
                   "Config apply plan should preserve operation identity");

  const auto unknown_old_tool =
      run_command_capture(base + "--tool hero.config.schema --args-json '{}'");
  if (unknown_old_tool.status == 0 ||
      unknown_old_tool.output.find("unknown tool: hero.config.schema") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Config tool hero.config.schema should fail as unknown\n" +
        unknown_old_tool.output);
  }

  const auto unknown_field = run_command_capture(
      base + "--tool hero.config.inspect --args-json "
             "'{\"subject\":\"schema\",\"old_field\":true}'");
  if (unknown_field.status == 0 ||
      unknown_field.output.find("unknown field: old_field") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config inspect should fail closed on unknown fields\n" +
        unknown_field.output);
  }
}

void check_runtime_collapsed_surface(const fs::path &binary) {
  const std::string base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";

  const std::string schema_output =
      read_command_stdout(base + "--tool hero.runtime.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  require_contains(schema_output, "\"keys\":[",
                   "Runtime inspect subject=schema should return policy keys");

  const std::string wave_output =
      read_command_stdout(base + "--tool hero.runtime.inspect --args-json "
                                 "'{\"subject\":\"wave\"}'");
  require_contains(wave_output, "\"source_range\"",
                   "Runtime inspect subject=wave should decode active wave");

  const std::string reset_plan_output =
      read_command_stdout(base + "--tool hero.runtime.reset --args-json "
                                 "'{\"requested_mode\":\"plan\"}'");
  require_contains(reset_plan_output, "\"dry_run\":true",
                   "Runtime reset requested_mode=plan should preview reset");

  const auto unknown_old_tool =
      run_command_capture(base + "--tool hero.runtime.schema --args-json '{}'");
  if (unknown_old_tool.status == 0 ||
      unknown_old_tool.output.find("unknown tool: hero.runtime.schema") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Runtime tool hero.runtime.schema should fail as unknown\n" +
        unknown_old_tool.output);
  }

  const auto old_dry_run_field = run_command_capture(
      base + "--tool hero.runtime.run --args-json "
             "'{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
             "\"dry_run\":true}'");
  if (old_dry_run_field.status == 0 ||
      old_dry_run_field.output.find("unknown field: dry_run") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired top-level dry_run field\n" +
        old_dry_run_field.output);
  }
}

void require_contains(const std::string &output, const std::string &needle,
                      const std::string &message) {
  if (output.find(needle) == std::string::npos) {
    throw std::runtime_error(message + " missing: " + needle + "\n" + output);
  }
}

void require_not_contains(const std::string &output, const std::string &needle,
                          const std::string &message) {
  if (output.find(needle) != std::string::npos) {
    throw std::runtime_error(message + " unexpected: " + needle + "\n" +
                             output);
  }
}

void require_lattice_non_decision_boundary(const std::string &output,
                                           const std::string &surface) {
  require_contains(output, "\"writes_evidence\":false",
                   surface + " must not write evidence");
  require_contains(output, "\"model_selector\":false",
                   surface + " must not select models");
  require_contains(output, "\"best_model_selector\":false",
                   surface + " must not select best models");
  require_contains(output, "\"performance_selector\":false",
                   surface + " must not select by performance");
  require_contains(output, "\"policy_gate\":false",
                   surface + " must not act as a policy gate");
  require_contains(output, "\"allocation_decision\":false",
                   surface + " must not make allocation decisions");
  require_contains(output, "\"market_readiness_decision\":false",
                   surface + " must not make market-readiness decisions");
  require_contains(output, "\"deployment_decision\":false",
                   surface + " must not make deployment decisions");
}

void require_fact_catalog_no_decision_authority(const std::string &output,
                                                const std::string &surface) {
  require_contains(output, "\"quality_authority_family_count\":0",
                   surface + " catalog must not carry quality authority");
  require_contains(output, "\"performance_authority_family_count\":0",
                   surface + " catalog must not carry performance authority");
  require_contains(output, "\"checkpoint_selector_family_count\":0",
                   surface + " catalog must not select checkpoints");
  require_contains(output, "\"allocation_authority_family_count\":0",
                   surface + " catalog must not allocate");
  require_contains(output, "\"execution_authority_family_count\":0",
                   surface + " catalog must not execute");
  require_contains(output, "\"market_readiness_authority_family_count\":0",
                   surface + " catalog must not declare market readiness");
  require_contains(output, "\"deployment_authority_family_count\":0",
                   surface + " catalog must not deploy");
  require_contains(output, "\"policy_gate_family_count\":0",
                   surface + " catalog must not act as policy gate");
  require_contains(output, "\"target_dependency_authority_family_count\":0",
                   surface + " catalog must not create target dependencies");
  require_contains(output, "\"runtime_wave_authority_family_count\":0",
                   surface + " catalog must not authorize Runtime waves");
  require_contains(output, "\"marshal_reachability_family_count\":0",
                   surface + " catalog must not become Marshal reachability");
  require_contains(output, "\"checkpoint_source_authority_family_count\":0",
                   surface + " catalog must not source checkpoints");
  require_contains(output, "\"plan_checkpoint_input_authority_family_count\":0",
                   surface + " catalog must not feed plan checkpoints");
  require_contains(output, "\"decision_authority_family_count\":0",
                   surface + " catalog decision authority count must stay 0");
  require_contains(output, "\"decision_authority_clean\":true",
                   surface + " catalog decision authority must stay clean");
}

void require_fact_identity_contract(const std::string &output,
                                    const std::string &surface) {
  require_contains(
      output,
      "\"fact_identity_contract\":{\"schema\":\"kikijyeba.lattice."
      "fact_identity_contract.v1\",\"contract_id\":\"runtime_evidence_"
      "identity_envelope\"",
      surface + " should expose the common fact identity envelope contract");
  require_contains(output,
                   "\"identity_fields\":[\"schema\",\"fact_type\","
                   "\"digest\",\"parent_exposure_fact_digest\","
                   "\"contract_fingerprint\",\"protocol_id\","
                   "\"graph_order_fingerprint\",\"source_cursor_token\","
                   "\"split_policy_fingerprint\","
                   "\"component_assembly_fingerprint\","
                   "\"target_component_family_id\",\"job_id\",\"wave_id\","
                   "\"split_name\",\"split_role\",\"anchor_range\","
                   "\"completed_anchor_range\"]",
                   surface + " should name common identity fields");
  require_contains(output, "\"fact_digest_required\":true",
                   surface + " should require stable fact digests");
  require_contains(output, "\"row_index_interval_authority\":true",
                   surface + " should keep row-index intervals authoritative");
  require_contains(output, "\"source_key_window_audit_only\":true",
                   surface + " should keep source-key windows audit-only");
  require_contains(output, "\"target_kind_authority\":false",
                   surface + " identity contract must not create target kinds");
  require_contains(output, "\"policy_gate_authority\":false",
                   surface + " identity contract must not create policy gates");
}

void write_text(const fs::path &path, const std::string &text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write fixture: " + path.string());
  }
  out << text;
}

std::string mtf_report_text(const std::string &checkpoint_digest) {
  std::ostringstream out;
  out << "report_schema_id="
         "wikimyei.representation.mtf_jepa_mae_vicreg.report.v1\n"
         "report_schema_version=1\n"
         "report_writer_id=jkimyei.training.representation."
         "mtf_jepa_mae_vicreg_graph_first_launcher\n"
         "report_writer_version=1\n"
         "optimizer_steps=3\n"
         "mean_loss=0.30\n"
         "representation_architecture=mtf_jepa_mae_vicreg.v1\n"
         "representation_contract=standalone.mtf_jepa_mae_vicreg.v1\n"
         "primary_value_shape=[B_flat,De]\n"
         "sequence_value_shape=[B_flat,Ntok,De]\n"
         "channel_value_shape=[B_flat,C,De]\n"
         "channel_axis_policy=optional_channel_output\n"
         "temporal_reduction=mask_aware_token_pool\n"
         "input_nodelift_shape=[B,C,Hx,N,F]\n"
         "mtf_training_shape=[B_flat,C,Hx,F]\n"
         "flattening_contract=anchor_node_flatten.v1\n"
         "recommended_graph_restore_shape=[B,N,C,De]\n"
         "graph_restore_available=false\n"
         "graph_restore_reason="
         "runtime_reports_flattened_anchor_node_samples_only\n"
         "reshape_lossless=true\n"
         "anchor_batch_count=16\n"
         "node_count=2\n"
         "flattened_sample_count=32\n"
         "channel_count=3\n"
         "history_length=16\n"
         "input_feature_width=5\n"
         "run_data_kind=market\n"
         "readiness_scope=market_pretraining\n"
         "synthetic_training_passed=false\n"
         "market_readiness_claimed=false\n"
         "finite_parameter_check=true\n"
         "gradients_finite=true\n"
         "sample_valid_count=32\n"
         "sample_valid_fraction=1.0\n"
         "channel_valid_count=96\n"
         "channel_valid_fraction=1.0\n"
         "valid_latent_rows=96\n"
         "total_valid_projection_rows=96\n"
         "tf_pair_count=64\n"
         "tf_pair_valid_count=64\n"
         "vicreg_global_valid_rows=32\n"
         "vicreg_channel_valid_rows=96\n"
         "context_starved_sample_count=0\n"
         "reduced_targets_for_context_count=0\n"
         "min_context_satisfied_count=32\n"
         "target_ema_distance=0.01\n"
         "latent_std=0.25\n"
         "latent_norm=1.5\n"
         "loss_jepa_mean=0.20\n"
         "loss_mae_time_mean=0.03\n"
         "loss_mae_frequency_mean=0.04\n"
         "loss_tf_align_mean=0.02\n"
         "loss_vicreg_global_mean=0.01\n"
         "loss_vicreg_channel_mean=0.01\n"
         "representation_embedding_dim=32\n"
         "representation_effective_rank=16\n"
         "representation_effective_rank_fraction=0.50\n"
         "representation_min_dimension_variance=0.01\n"
         "representation_max_dimension_variance=1.0\n"
         "representation_condition_number=100\n"
         "representation_isotropy_score=0.25\n"
         "checkpoint_written=true\n"
         "checkpoint_path=mtf.pt\n"
         "checkpoint_path_reported=mtf.pt\n"
         "checkpoint_digest_verified=true\n"
         "checkpoint_file_exists=true\n"
         "checkpoint_bytes=14\n"
      << "checkpoint_digest_reported=" << checkpoint_digest << "\n";
  return out.str();
}

void write_mtf_source_analytics_warning_fixture(const fs::path &runtime_root) {
  fs::remove_all(runtime_root);
  const auto job_dir = runtime_root / "mtf_source_analytics_warn_job";

  const auto split_policy = split::load_lattice_split_policy_from_file(
      "/cuwacunu/src/config/hero.lattice.splits.dsl");
  const auto *train_core = split_policy.find_split("train_core");
  if (train_core == nullptr) {
    throw std::runtime_error("missing train_core split");
  }
  const auto split_policy_fingerprint =
      split::lattice_split_policy_fingerprint(split_policy);

  std::ostringstream manifest;
  manifest << "job_id=mtf_source_analytics_warn_job\n"
              "job_kind=channel_representation_mtf_jepa_mae_vicreg\n"
              "protocol_id=cwu_02v\n"
              "target_component_family_id="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
              "wave_id=wave_mtf_source_analytics\n"
              "wave_action=train\n"
              "mutated_components="
              "wikimyei.representation.encoding.mtf_jepa_mae_vicreg\n"
              "graph_order_fingerprint=graph_1\n"
              "source_range_policy=anchor_index\n"
           << "resolved_anchor_index_begin=" << train_core->anchor_range.begin
           << "\nresolved_anchor_index_end=" << train_core->anchor_range.end
           << "\naccepted_anchor_count=" << train_core->anchor_range.length()
           << "\nsource_cursor_token=cursor_1\n"
              "protocol_contract_fingerprint=contract_1\n"
              "mtf_jepa_mae_vicreg_assembly_fingerprint=mtf_1\n";
  write_text(job_dir / "job.manifest", manifest.str());
  write_text(job_dir / "job.state", "status=completed\n"
                                    "optimizer_steps=3\n"
                                    "last_loss=0.35\n"
                                    "checkpoint_written=true\n"
                                    "checkpoint_path=mtf.pt\n");
  write_text(job_dir / "mtf.pt", "mtf checkpoint");
  const auto digest = exposure::file_content_digest(job_dir / "mtf.pt");
  write_text(job_dir / "channel_representation.report",
             mtf_report_text(digest));
  write_text(job_dir / "lattice.source_analytics.fact",
             "schema=kikijyeba.lattice.source_analytics.v1\n"
             "entropy=1.25\n"
             "entropy_rate=0.50\n"
             "information_density=0.75\n"
             "compression_ratio=2.50\n"
             "power_spectrum_entropy=0.33\n"
             "sample_validity_fraction=0.72\n"
             "missingness_fraction=0.28\n"
             "duplicate_sample_count=3\n"
             "source_receipt_fact_count=1\n"
             "source_health_level=warn\n"
             "visibility_only=true\n"
             "readiness_authority=false\n"
             "coverage_authority=false\n"
             "leakage_authority=false\n"
             "contract_identity_authority=false\n");

  exposure::exposure_build_context_t context{};
  context.cursor_domain = split_policy.cursor_domain;
  context.split_policy_fingerprint = split_policy_fingerprint;
  context.split_name = train_core->split_id;
  context.split_role = train_core->role;
  const auto fact = exposure::make_exposure_fact_from_job_dir(job_dir, context);
  exposure::write_exposure_fact_sidecar(
      exposure::exposure_fact_path_for_job_dir(job_dir), fact);
  exposure::write_checkpoint_fact_sidecar(
      exposure::checkpoint_fact_path_for_job_dir(job_dir),
      exposure::make_checkpoint_fact_from_exposure_fact(fact));
}

void check_lattice_selector_boundaries(const fs::path &binary) {
  const fs::path runtime_root = "/tmp/hero_mcp_schema_compat/lattice_empty";
  fs::remove_all(runtime_root);
  fs::create_directories(runtime_root);
  const auto base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";

  const auto unknown_old_tool =
      run_command_capture(base + "--tool hero.lattice.schema --args-json '{}'");
  if (unknown_old_tool.status == 0 ||
      unknown_old_tool.output.find("unknown tool: hero.lattice.schema") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Lattice tool hero.lattice.schema should fail as unknown\n" +
        unknown_old_tool.output);
  }

  const std::string lattice_status =
      read_command_stdout(base +
                          "--tool hero.lattice.status --args-json "
                          "'{\"runtime_root\":\"" +
                          runtime_root.string() + "\"}'");
  require_contains(
      lattice_status,
      "\"ok\":true,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "status should expose top-level read-only, non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(lattice_status, "hero.lattice.status");

  const std::string lattice_schema = read_command_stdout(
      base +
      "--tool hero.lattice.inspect --args-json '{\"subject\":\"schema\"}'");
  require_contains(
      lattice_schema,
      "\"schema\":\"kikijyeba.lattice.hero_policy_schema.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,\"writes_evidence\":"
      "false,\"fact_families_are_not_target_kinds\":true",
      "schema should expose top-level read-only, non-proof, non-dispatchable "
      "boundaries");
  require_contains(lattice_schema, "\"policy_gate\":false",
                   "schema must not act as a policy gate");

  const std::string target_list = read_command_stdout(
      base +
      "--tool hero.lattice.inspect --args-json '{\"subject\":\"targets\"}'");
  require_contains(
      target_list,
      "\"schema\":\"kikijyeba.lattice.target_catalog.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,\"writes_evidence\":"
      "false,\"fact_families_are_not_target_kinds\":true",
      "list_targets should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_lattice_non_decision_boundary(target_list,
                                        "hero.lattice.inspect subject=targets");
  require_contains(
      target_list,
      "\"policy_gate_reservation_summary\":{\"schema\":\"kikijyeba."
      "lattice.policy_gate_reservation.summary.v1\","
      "\"reservation_count\":2",
      "list_targets should expose disabled policy gate "
      "reservations");
  require_contains(target_list, "\"enabled_policy_gate_count\":0",
                   "policy gate reservations must not be enabled");
  require_contains(target_list, "\"disabled_policy_gate_count\":2",
                   "both active policy gate reservations should be disabled");
  require_contains(target_list, "\"policy_fingerprint_verified_count\":2",
                   "policy gate reservation summary should report verified "
                   "fingerprints");
  require_contains(target_list, "\"policy_fingerprint_mismatch_count\":0",
                   "policy gate reservation summary should reject fingerprint "
                   "mismatches before JSON emission");
  require_contains(target_list, "\"all_policy_fingerprints_verified\":true",
                   "policy gate reservation summary should expose the "
                   "fingerprint invariant");
  require_contains(target_list, "\"policy_input_contract_complete_count\":2",
                   "policy gate reservation summary should report complete "
                   "policy input contracts");
  require_contains(target_list, "\"missing_policy_input_contract_count\":0",
                   "policy gate reservation summary should expose missing "
                   "input contract count");
  require_contains(target_list, "\"all_policy_input_contracts_complete\":true",
                   "policy gate reservation summary should prove all reserved "
                   "policy inputs are declared");
  require_contains(target_list, "\"decision_policy_authority_count\":0",
                   "disabled policy reservations must not become decision "
                   "policy authority");
  require_contains(target_list, "\"target_status_authority_count\":0",
                   "policy gate reservations must not affect target status");
  require_contains(target_list, "\"target_spec_fingerprint_authority_count\":0",
                   "policy gate reservations must not affect target "
                   "fingerprints");
  require_contains(target_list,
                   "\"policy_id\":\"forecast_quality_acceptance_reserved\"",
                   "forecast policy reservation should be inspectable");
  require_contains(target_list,
                   "\"policy_id\":\"allocation_acceptance_reserved\"",
                   "allocation policy reservation should be inspectable");
  require_contains(target_list, "\"policy_fingerprint_verified\":true",
                   "policy gate rows should expose canonical fingerprint "
                   "verification");
  require_contains(target_list, "\"policy_fingerprint_mismatch\":false",
                   "policy gate rows should expose absence of fingerprint "
                   "mismatch");
  require_contains(
      target_list,
      "\"policy_input_contract_schema\":\"kikijyeba.lattice.policy_gate."
      "input_contract.v1\"",
      "policy gate rows should expose the input-contract schema");
  require_contains(target_list, "\"policy_input_contract_complete\":true",
                   "policy gate rows should expose complete input contracts");
  require_contains(target_list, "\"missing_policy_inputs\":[]",
                   "policy gate rows should expose empty missing inputs");
  require_contains(target_list, "\"decision_policy_authority\":false",
                   "policy gate rows should not claim decision authority");
  require_contains(target_list, "\"metric_definition\":\"",
                   "policy gate rows should expose metric definitions");
  require_contains(target_list, "\"baseline_definition\":\"",
                   "policy gate rows should expose baseline definitions");
  require_contains(target_list, "\"uncertainty_model\":\"",
                   "policy gate rows should expose uncertainty model names");
  require_contains(target_list, "\"negative_tests\":\"",
                   "policy gate rows should expose negative-test policy");
  require_contains(target_list, "\"calibration_requirements\":\"",
                   "policy gate rows should expose calibration requirements");
  require_contains(target_list, "\"holdout_declaration\":\"",
                   "policy gate rows should expose holdout declarations");
  require_contains(target_list, "\"threshold_selection_audit\":\"",
                   "policy gate rows should expose threshold-selection audit");
  require_contains(
      target_list,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "list_targets should expose artifact readiness proof template registry");
  require_contains(target_list, "\"proof_template_count\":7",
                   "artifact readiness registry should enumerate proof "
                   "templates");
  require_contains(
      target_list,
      "\"proofable_fact_families\":[\"target_transform\","
      "\"forecast_baseline\",\"forecast_eval\",\"observer_belief\","
      "\"allocation_engine\",\"replay_environment\",\"policy_training\"]",
      "artifact readiness registry should expose proofable fact families");
  require_contains(
      target_list,
      "\"warning_summary_only_fact_families\":[\"source_analytics\"]",
      "artifact readiness registry should keep source analytics warning-only");
  require_contains(target_list,
                   "\"new_fact_family_default\":\"warning_or_summary_only\"",
                   "artifact readiness registry should fail closed for new "
                   "fact families");
  require_contains(target_list, "\"target_kind_expansion_required\":false",
                   "artifact readiness registry should not require TARGET_KIND "
                   "expansion");
  require_contains(
      target_list,
      "\"subject_fact_family\":\"forecast_eval\",\"proof_kind\":\"forecast_"
      "eval_artifact_bound\"",
      "artifact readiness registry should expose forecast eval proof kind");
  require_contains(
      target_list,
      "\"subject_fact_family\":\"policy_training\",\"proof_kind\":\"policy_"
      "training_artifact_bound\"",
      "artifact readiness registry should expose policy training proof kind");

  const std::string latest = read_command_stdout(
      base +
      "--tool hero.lattice.evaluate "
      "--args-json "
      "'{\"operation\":\"latest_satisfying_checkpoint\",\"target_id\":"
      "\"channel_mdn_train_core_no_test_leakage\","
      "\"runtime_root\":\"" +
      runtime_root.string() + "\"}'");
  require_contains(
      latest,
      "\"schema\":\"kikijyeba.lattice.latest_satisfying_resolution.v1\","
      "\"schema_version\":1,\"ok\":true,\"read_only\":true,"
      "\"target_proof\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "latest_satisfying should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(latest, "\"source_target_class\":\"leakage_guard\"",
                   "latest_satisfying should expose source target class");
  require_contains(latest, "\"checkpoint_selector\":true",
                   "latest_satisfying should identify checkpoint selection");
  require_contains(latest, "\"checkpoint_selectable_source_target\":true",
                   "latest_satisfying should accept checkpoint-producing "
                   "leakage guard source targets");
  require_contains(latest, "\"performance_selector\":false",
                   "latest_satisfying must not be a performance selector");
  require_contains(latest, "\"quality_acceptance\":false",
                   "latest_satisfying must not accept forecast quality");
  require_contains(latest, "\"policy_gate\":false",
                   "latest_satisfying must not be a policy gate");
  require_contains(latest, "\"market_readiness_decision\":false",
                   "latest_satisfying must not make market-readiness claims");
  require_contains(latest, "\"deployment_decision\":false",
                   "latest_satisfying must not make deployment decisions");

  const std::string artifact_latest =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"latest_satisfying_checkpoint\","
                          "\"target_id\":\"forecast_eval_artifact_ready\","
                          "\"runtime_root\":\"" +
                          runtime_root.string() + "\"}'");
  require_contains(artifact_latest,
                   "\"source_target_class\":\"artifact_readiness\"",
                   "latest_satisfying should expose artifact source class");
  require_contains(artifact_latest,
                   "\"checkpoint_selectable_source_target\":false",
                   "latest_satisfying must reject artifact source targets");
  require_contains(artifact_latest,
                   "\"resolution_status\":\"non_checkpoint_target_class\"",
                   "latest_satisfying must fail closed for artifact sources");
  require_contains(artifact_latest, "\"resolved\":false",
                   "artifact source targets must not resolve to checkpoints");

  const std::string checkpoint_closure = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"checkpoint\",\"checkpoint_id\":\"missing_checkpoint\","
      "\"checkpoint_file_digest\":\"missing_digest\","
      "\"runtime_root\":\"" +
      runtime_root.string() + "\"}'");
  require_contains(
      checkpoint_closure,
      "\"schema\":\"kikijyeba.lattice.checkpoint_closure.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "checkpoint_closure should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(checkpoint_closure, "\"checkpoint_selector\":false",
                   "checkpoint_closure must not select checkpoints");
  require_contains(checkpoint_closure,
                   "\"automatic_checkpoint_selection\":false",
                   "checkpoint_closure must not auto-select checkpoints");
  require_contains(checkpoint_closure, "\"complete\":false",
                   "missing checkpoint identity should fail closed");

  const std::string comparison = read_command_stdout(
      base +
      "--tool hero.lattice.compare "
      "--args-json '{\"left_target_id\":\"channel_mdn_train_core_no_test_"
      "leakage\","
      "\"right_target_id\":\"channel_mdn_train_core_ready\","
      "\"runtime_root\":\"" +
      runtime_root.string() + "\"}'");
  require_contains(
      comparison,
      "\"schema\":\"kikijyeba.lattice.evidence_comparison.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "compare_evidence should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(comparison, "\"checkpoint_selector\":false",
                   "compare_evidence must not select checkpoints");
  require_contains(comparison, "\"automatic_checkpoint_selection\":false",
                   "compare_evidence must not auto-select checkpoints");
  require_contains(comparison, "\"model_selector\":false",
                   "compare_evidence must not select models");
  require_contains(comparison, "\"best_model_selector\":false",
                   "compare_evidence must not select the best model");
  require_contains(comparison, "\"performance_selector\":false",
                   "compare_evidence must not select by performance");
  require_contains(comparison, "\"pareto_optimizer\":false",
                   "compare_evidence must not optimize checkpoints by Pareto");
  require_contains(comparison, "\"scalar_score_emitted\":false",
                   "compare_evidence must not emit scalar scores");
  require_contains(comparison, "\"quality_acceptance\":false",
                   "compare_evidence must not accept quality");
  require_contains(comparison, "\"policy_gate\":false",
                   "compare_evidence must not act as a policy gate");
  require_contains(comparison, "\"allocation_decision\":false",
                   "compare_evidence must not decide allocation");
  require_contains(comparison, "\"market_readiness_decision\":false",
                   "compare_evidence must not make market-readiness claims");
  require_contains(comparison, "\"deployment_decision\":false",
                   "compare_evidence must not decide deployment");

  const std::string fact_summary = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"facts\",\"mode\":\"summary\",\"runtime_root\":\"" +
      runtime_root.string() + "\",\"family\":\"forecast_eval\"}'");
  require_contains(
      fact_summary,
      "\"schema\":\"kikijyeba.lattice.fact_summary.v1\",\"runtime_root\":\"" +
          runtime_root.string() +
          "\",\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
          "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":"
          "true",
      "fact_summary should expose top-level non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(
      fact_summary, "hero.lattice.inspect subject=facts mode=summary");
  require_fact_catalog_no_decision_authority(
      fact_summary, "hero.lattice.inspect subject=facts mode=summary");
  require_fact_identity_contract(
      fact_summary, "hero.lattice.inspect subject=facts mode=summary");
  require_contains(fact_summary, "\"fact_integrity_summary\":",
                   "fact_summary should expose top-level fact integrity");
  require_contains(fact_summary,
                   "\"schema\":\"kikijyeba.lattice.fact_integrity_summary.v1\"",
                   "fact_summary fact integrity should be schema tagged");
  require_contains(fact_summary, "\"inspected_family_count\":1",
                   "fact_summary fact integrity should scope to requested "
                   "family");
  require_contains(fact_summary, "\"relation_integrity_clean\":true",
                   "empty fact_summary integrity should be clean");
  require_contains(fact_summary, "\"target_proof\":false",
                   "fact_summary integrity must not become proof authority");
  require_contains(fact_summary, "\"dispatchable\":false",
                   "fact_summary integrity must remain non-dispatchable");
  require_contains(
      fact_summary,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "fact_summary should expose artifact proof-template registry");
  require_contains(
      fact_summary,
      "\"family\":\"forecast_eval\",\"fact_schema\":\"kikijyeba.lattice."
      "forecast_eval.v1\",\"relation\":\"forecast_eval\",\"summary_schema\":"
      "\"kikijyeba.lattice.forecast_eval_summary.v1\",\"authority_model\":"
      "\"artifact_evidence\",\"artifact_readiness_proofable\":true,"
      "\"artifact_readiness_proof_kind\":\"forecast_eval_artifact_bound\","
      "\"artifact_readiness_proof_claim\":\"forecast evaluation artifact "
      "existence, checkpoint lineage, target-transform binding, baseline "
      "binding, selection-signal audit binding, and support counts\"",
      "fact_summary should mark forecast_eval as explicitly proofable");

  const std::string fact_family_registry = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json '{\"subject\":\"fact_families\",\"runtime_root\":\"" +
      runtime_root.string() + "\"}'");
  require_contains(
      fact_family_registry,
      "\"schema\":\"kikijyeba.lattice.fact_family_registry.v1\","
      "\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":true",
      "list_fact_families should expose top-level non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(
      fact_family_registry, "hero.lattice.inspect subject=fact_families");
  require_fact_catalog_no_decision_authority(
      fact_family_registry, "hero.lattice.inspect subject=fact_families");
  require_fact_identity_contract(fact_family_registry,
                                 "hero.lattice.inspect subject=fact_families");
  require_contains(
      fact_family_registry,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "list_fact_families should expose artifact proof-template registry");
  require_contains(
      fact_family_registry,
      "\"family\":\"forecast_eval\",\"fact_schema\":\"kikijyeba.lattice."
      "forecast_eval.v1\",\"relation\":\"forecast_eval\",\"summary_schema\":"
      "\"kikijyeba.lattice.forecast_eval_summary.v1\",\"authority_model\":"
      "\"artifact_evidence\",\"artifact_readiness_proofable\":true,"
      "\"artifact_readiness_proof_kind\":\"forecast_eval_artifact_bound\"",
      "fact registry should mark forecast_eval as explicitly proofable");
  require_contains(
      fact_family_registry,
      "\"quality_authority\":false,\"performance_authority\":false,"
      "\"checkpoint_selector\":false,\"allocation_authority\":false,"
      "\"execution_authority\":false,\"market_readiness_authority\":false,"
      "\"deployment_authority\":false,\"policy_gate\":false,"
      "\"target_dependency_authority\":false,\"runtime_wave_authority\":false,"
      "\"marshal_reachability\":false,\"checkpoint_source_authority\":false,"
      "\"plan_checkpoint_input_authority\":false,"
      "\"fact_identity_contract\":",
      "fact registry descriptors should expose full non-decision authority "
      "contract");
  require_contains(
      fact_family_registry,
      "\"fact_family\":\"forecast_eval\",\"fact_schema\":\"kikijyeba.lattice."
      "forecast_eval.v1\",\"parent_exposure_bound\":true",
      "fact registry should bind forecast_eval identity to parent exposure");
  require_contains(fact_family_registry,
                   "\"lineage_digest_fields\":[\"parent_exposure_fact_digest\","
                   "\"forecast_artifact_digest\","
                   "\"evaluated_representation_checkpoint_digest\","
                   "\"evaluated_mdn_checkpoint_digest\","
                   "\"target_transform_fact_digest\",\"baseline_fact_digests\","
                   "\"selection_signal_fact_digests\"]",
                   "fact registry should expose forecast_eval lineage digests");
  require_contains(
      fact_family_registry,
      "\"support_fields\":[\"support_count\",\"valid_count\","
      "\"missing_count\",\"support_by_node\",\"support_by_channel\","
      "\"support_by_target_feature\",\"support_by_horizon\","
      "\"weakest_support_rows\"]",
      "fact registry should expose forecast_eval support-surface fields");
  require_contains(
      fact_family_registry,
      "\"family\":\"source_analytics\",\"fact_schema\":\"kikijyeba.lattice."
      "source_analytics.v1\",\"relation\":\"source_analytics\","
      "\"summary_schema\":\"kikijyeba.lattice.source_analytics_summary.v1\","
      "\"authority_model\":\"visibility_only\","
      "\"artifact_readiness_proofable\":false,"
      "\"artifact_readiness_proof_kind\":null,"
      "\"artifact_readiness_proof_claim\":null,"
      "\"artifact_readiness_requires_explicit_template\":true,"
      "\"artifact_readiness_target_kind_required\":false,"
      "\"artifact_readiness_target_promotion_allowed\":false,"
      "\"artifact_readiness_target_promotion_blocked\":true,"
      "\"artifact_readiness_target_promotion_blocked_reason\":"
      "\"warning_summary_only\","
      "\"artifact_readiness_warning_summary_only\":true",
      "fact registry should keep source analytics warning/summary-only");
  require_contains(
      fact_family_registry,
      "\"family\":\"selection_signal\",\"fact_schema\":\"kikijyeba.lattice."
      "selection_signal.v1\",\"relation\":\"selection_signal\","
      "\"summary_schema\":\"kikijyeba.lattice.selection_signal_summary.v1\","
      "\"authority_model\":\"leakage_visibility\","
      "\"artifact_readiness_proofable\":false,"
      "\"artifact_readiness_proof_kind\":null,"
      "\"artifact_readiness_proof_claim\":null,"
      "\"artifact_readiness_requires_explicit_template\":true,"
      "\"artifact_readiness_target_kind_required\":false,"
      "\"artifact_readiness_target_promotion_allowed\":false,"
      "\"artifact_readiness_target_promotion_blocked\":true,"
      "\"artifact_readiness_target_promotion_blocked_reason\":"
      "\"leakage_visibility_only\","
      "\"artifact_readiness_warning_summary_only\":false",
      "fact registry should keep selection signals as leakage visibility, not "
      "artifact targets");
  require_contains(
      fact_family_registry,
      "\"family\":\"representation_support\",\"fact_schema\":\"kikijyeba."
      "lattice.representation_support.v1\",\"relation\":\"representation_"
      "support\","
      "\"summary_schema\":\"kikijyeba.lattice.representation_support_summary."
      "v1\",\"authority_model\":\"visibility_only\","
      "\"artifact_readiness_proofable\":false,"
      "\"artifact_readiness_proof_kind\":null,"
      "\"artifact_readiness_proof_claim\":null,"
      "\"artifact_readiness_requires_explicit_template\":true,"
      "\"artifact_readiness_target_kind_required\":false,"
      "\"artifact_readiness_target_promotion_allowed\":false,"
      "\"artifact_readiness_target_promotion_blocked\":true,"
      "\"artifact_readiness_target_promotion_blocked_reason\":"
      "\"support_visibility_only\","
      "\"artifact_readiness_warning_summary_only\":false",
      "fact registry should keep representation support as visibility "
      "evidence, "
      "not an artifact target");

  const std::string fact_scan = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"facts\",\"mode\":\"scan\",\"runtime_root\":\"" +
      runtime_root.string() +
      "\",\"family\":\"forecast_eval\",\"include_facts\":false}'");
  require_contains(
      fact_scan,
      "\"schema\":\"kikijyeba.lattice.fact_catalog_scan.v1\",\"runtime_root\":"
      "\"" +
          runtime_root.string() +
          "\",\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
          "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":"
          "true",
      "scan_facts should expose top-level non-proof, non-dispatchable catalog "
      "boundaries");
  require_lattice_non_decision_boundary(
      fact_scan, "hero.lattice.inspect subject=facts mode=scan");
  require_fact_catalog_no_decision_authority(
      fact_scan, "hero.lattice.inspect subject=facts mode=scan");
  require_fact_identity_contract(
      fact_scan, "hero.lattice.inspect subject=facts mode=scan");
  require_contains(fact_scan, "\"fact_integrity_summary\":",
                   "scan_facts should expose top-level fact integrity");
  require_contains(fact_scan, "\"inspected_family_count\":1",
                   "scan_facts fact integrity should scope to requested "
                   "family");
  require_contains(fact_scan, "\"relation_integrity_clean\":true",
                   "empty scan_facts integrity should be clean");
  require_contains(
      fact_scan,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "scan_facts should expose artifact proof-template registry");
  require_contains(
      fact_scan,
      "\"artifact_readiness_proof_kind\":\"forecast_eval_artifact_bound\","
      "\"artifact_readiness_proof_claim\":\"forecast evaluation artifact "
      "existence, checkpoint lineage, target-transform binding, baseline "
      "binding, selection-signal audit binding, and support counts\"",
      "scan_facts family summaries should expose artifact proof-template "
      "claims");

  const std::string fact_lineage = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"facts\",\"mode\":\"lineage\",\"runtime_root\":\"" +
      runtime_root.string() + "\",\"family\":\"forecast_eval\",\"limit\":4}'");
  require_contains(fact_lineage,
                   "\"schema\":\"kikijyeba.lattice.fact_lineage.v1\"",
                   "fact_lineage should expose its schema");
  require_contains(fact_lineage, "\"read_only\":true",
                   "fact_lineage should be read-only");
  require_contains(fact_lineage, "\"target_proof\":false",
                   "fact_lineage must not become proof authority");
  require_contains(fact_lineage, "\"dispatchable\":false",
                   "fact_lineage must remain non-dispatchable");
  require_contains(fact_lineage, "\"runtime_executor\":false",
                   "fact_lineage must not execute runtime work");
  require_contains(fact_lineage, "\"fact_families_are_not_target_kinds\":true",
                   "fact_lineage should preserve the catalog boundary");
  require_lattice_non_decision_boundary(
      fact_lineage, "hero.lattice.inspect subject=facts mode=lineage");
  require_contains(fact_lineage, "\"lineage_rows_are_audit_only\":true",
                   "fact_lineage rows should be audit-only");
  require_contains(fact_lineage,
                   "\"cache_rows_used_for_target_satisfaction\":false",
                   "fact_lineage rows must not satisfy targets");
  require_contains(fact_lineage, "\"fact_integrity_summary\":",
                   "fact_lineage should expose top-level fact integrity");
  require_contains(fact_lineage, "\"inspected_family_count\":1",
                   "fact_lineage fact integrity should scope to requested "
                   "family");
  require_contains(fact_lineage, "\"selected_relation_count\":1",
                   "fact_lineage should scope selected relations to family");
  require_contains(fact_lineage, "\"selected_relations\":[\"forecast_eval\"]",
                   "fact_lineage should expose selected lineage relations");
  require_contains(fact_lineage, "\"matching_row_count\":0",
                   "empty fact_lineage should report no matching rows");
  require_contains(fact_lineage, "\"returned_row_count\":0",
                   "empty fact_lineage should return no rows");
  require_contains(fact_lineage, "\"truncated\":false",
                   "empty fact_lineage should not be truncated");

  const std::string index_status = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"index\",\"mode\":\"status\",\"runtime_root\":\"" +
      runtime_root.string() + "\",\"limit\":4}'");
  require_contains(
      index_status,
      "\"schema\":\"kikijyeba.lattice.runtime_index_status.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "index_status should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(index_status, "\"checkpoint_selector\":false",
                   "index_status must not select checkpoints");
  require_contains(index_status, "\"automatic_checkpoint_selection\":false",
                   "index_status must not auto-select checkpoints");
  require_contains(index_status, "\"db_source_of_truth\":false",
                   "index_status must keep runtime evidence as authority");
  require_contains(index_status, "\"cache_used_for_target_satisfaction\":false",
                   "index_status cache rows must not satisfy targets");

  const std::string index_query = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"index\",\"mode\":\"query\",\"runtime_root\":\"" +
      runtime_root.string() +
      "\",\"relation\":\"forecast_eval\",\"limit\":4}'");
  require_contains(
      index_query,
      "\"schema\":\"kikijyeba.lattice.runtime_index_query_result.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "index_query should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(index_query, "\"checkpoint_selector\":false",
                   "index_query must not select checkpoints");
  require_contains(index_query, "\"automatic_checkpoint_selection\":false",
                   "index_query must not auto-select checkpoints");
  require_contains(index_query, "\"db_source_of_truth\":false",
                   "index_query must keep runtime evidence as authority");
  require_contains(index_query, "\"cache_used_for_target_satisfaction\":false",
                   "index_query cache rows must not satisfy targets");
  require_contains(index_query, "\"cache_used_unproven_for_audit_query\":false",
                   "default index_query must not use unproven cache answers");

  const std::string derived_query =
      read_command_stdout(base +
                          "--tool hero.lattice.inspect "
                          "--args-json "
                          "'{\"subject\":\"derived\",\"relation\":\"stale_"
                          "cache\",\"runtime_root\":\"" +
                          runtime_root.string() + "\",\"limit\":4}'");
  require_contains(
      derived_query,
      "\"schema\":\"kikijyeba.lattice.derived_query.v1\","
      "\"schema_version\":1,\"relation\":\"stale_cache\","
      "\"canonical_relation\":\"stale_cache\"",
      "derived_query should expose the versioned query schema and relation");
  require_contains(
      derived_query,
      "\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":true",
      "derived_query should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(derived_query, "\"checkpoint_selector\":false",
                   "derived_query must not select checkpoints");
  require_contains(derived_query, "\"automatic_checkpoint_selection\":false",
                   "derived_query must not auto-select checkpoints");
  require_contains(derived_query, "\"db_source_of_truth\":false",
                   "derived_query must keep runtime evidence as authority");
  require_contains(derived_query,
                   "\"cache_rows_used_for_target_satisfaction\":false",
                   "derived_query cache rows must not satisfy targets");

  const std::string exposure_scan = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json '{\"subject\":\"exposure\",\"runtime_root\":\"" +
      runtime_root.string() + "\",\"limit\":4}'");
  require_contains(
      exposure_scan, "\"read_only\":true",
      "scan_exposure should advertise read-only evidence scanning");
  require_contains(exposure_scan, "\"target_proof\":false",
                   "scan_exposure must not become target proof authority");
  require_contains(exposure_scan, "\"dispatchable\":false",
                   "scan_exposure must remain non-dispatchable");
  require_contains(exposure_scan, "\"runtime_executor\":false",
                   "scan_exposure must not execute runtime work");
  require_contains(exposure_scan, "\"fact_families_are_not_target_kinds\":true",
                   "scan_exposure should preserve the fact-catalog boundary");
  require_lattice_non_decision_boundary(
      exposure_scan, "hero.lattice.inspect subject=exposure");

  const fs::path source_analytics_runtime_root =
      "/tmp/hero_mcp_schema_compat/lattice_source_analytics_warning";
  write_mtf_source_analytics_warning_fixture(source_analytics_runtime_root);
  const std::string source_analytics_eval = read_command_stdout(
      base +
      "--tool hero.lattice.evaluate "
      "--args-json "
      "'{\"operation\":\"target\",\"target_id\":\"mtf_jepa_mae_vicreg_train_"
      "core_ready\","
      "\"runtime_root\":\"" +
      source_analytics_runtime_root.string() +
      "\",\"protocol_id\":\"cwu_02v\","
      "\"protocol_contract_fingerprint\":\"contract_1\","
      "\"graph_order_fingerprint\":\"graph_1\","
      "\"source_cursor_token\":\"cursor_1\","
      "\"mtf_jepa_mae_vicreg_assembly_fingerprint\":\"mtf_1\"}'");
  require_contains(source_analytics_eval, "\"status\":\"satisfied\"",
                   "source analytics warning fixture should satisfy the "
                   "MTF readiness target");
  require_contains(source_analytics_eval, "\"source_analytics_fact_count\":1",
                   "source analytics warning fixture should scan one source "
                   "analytics fact");
  require_contains(source_analytics_eval,
                   "\"warning_id\":\"mtf_source_missingness_high_visibility_"
                   "only\"",
                   "source analytics warning result should be serialized");
  require_contains(source_analytics_eval, "\"kind\":\"source_analytics\"",
                   "source analytics warning result should preserve kind");
  require_contains(source_analytics_eval, "\"status\":\"warning\"",
                   "source analytics warning should trigger");
  require_contains(source_analytics_eval, "\"threshold_triggered\":true",
                   "source analytics warning should mark threshold triggered");
  require_contains(source_analytics_eval,
                   "\"threshold_relation\":\"above_threshold\"",
                   "source analytics warning should expose threshold relation");
  require_contains(source_analytics_eval, "\"measured_value\":0.28",
                   "source analytics warning should expose measured "
                   "missingness");
  require_contains(source_analytics_eval, "\"threshold\":0.1",
                   "source analytics warning should expose configured "
                   "threshold");
  require_contains(source_analytics_eval, "\"threshold_direction\":\"above\"",
                   "source analytics warning should expose threshold "
                   "direction");
  require_contains(source_analytics_eval, "\"unit\":\"fraction\"",
                   "source analytics missingness should be a fraction metric");
  require_contains(source_analytics_eval,
                   "\"evidence_basis\":\"source_analytics_facts\"",
                   "source analytics warning should name catalog facts as "
                   "basis");
  require_contains(source_analytics_eval, "\"measurement_available\":true",
                   "source analytics warning should have a measurement");
  require_contains(source_analytics_eval, "\"diagnostic_sample_count\":1",
                   "source analytics warning should expose receipt support "
                   "count");
  require_contains(source_analytics_eval, "\"use\":\"source_observation\"",
                   "source analytics warning should not claim exposure-use "
                   "authority");
  require_contains(source_analytics_eval, "\"effect\":\"none\"",
                   "source analytics warning should not require model "
                   "mutation");
  require_contains(source_analytics_eval, "\"fact_count\":1",
                   "source analytics warning should count matching facts");
  require_contains(source_analytics_eval, "\"plan_ready\":false",
                   "source analytics warning must not make a satisfied target "
                   "plan-ready");
  require_contains(source_analytics_eval,
                   "\"warning_id\":\"mtf_source_missingness_high_visibility_"
                   "only\"",
                   "source analytics warning should project into typed target "
                   "warnings");
  require_contains(source_analytics_eval, "\"source\":\"lattice\"",
                   "source analytics warning should carry source identity");
  require_contains(source_analytics_eval, "\"blocking\":false",
                   "source analytics warning should remain non-blocking");
  require_contains(source_analytics_eval,
                   "\"warning_family\":\"source_health\"",
                   "source analytics warning should expose warning family");
  require_contains(source_analytics_eval,
                   "\"fact_family\":\"source_analytics\"",
                   "source analytics warning should bind to fact family");
  require_contains(source_analytics_eval,
                   "\"warning_messages\":[\"source-analytics "
                   "missingness_fraction",
                   "source analytics warning messages remain visible");
  require_contains(source_analytics_eval, "\"all_warnings_non_blocking\":true",
                   "source analytics warning summary should prove warnings are "
                   "non-blocking");

  const std::string artifact_explain =
      read_command_stdout(base + "--tool hero.lattice.inspect "
                                 "--args-json "
                                 "'{\"subject\":\"target\",\"target_id\":"
                                 "\"forecast_eval_artifact_ready\"}'");
  require_contains(
      artifact_explain,
      "\"schema\":\"kikijyeba.lattice.target_explanation.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,\"writes_evidence\":"
      "false,\"fact_families_are_not_target_kinds\":true",
      "explain_target should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_contains(artifact_explain, "\"target_class\":\"artifact_readiness\"",
                   "artifact explain should preserve target class");
  require_contains(artifact_explain, "\"kind\":\"not_applicable\"",
                   "artifact explain must not report a trainable target kind");
  require_contains(artifact_explain, "\"target_kind_applicable\":false",
                   "artifact explain should mark TARGET_KIND not applicable");
  require_contains(
      artifact_explain, "\"target_kind_effective\":\"none\"",
      "artifact explain should not expose an effective TARGET_KIND");
  require_contains(
      artifact_explain,
      "\"artifact_proof_template\":{\"subject_fact_family\":\"forecast_eval\","
      "\"proof_kind\":\"forecast_eval_artifact_bound\"",
      "artifact explain should bind forecast eval to an explicit proof "
      "template");
  require_contains(artifact_explain, "\"checkpoint_selection_authority\":false",
                   "artifact proof template must not select checkpoints");
  require_contains(artifact_explain, "\"target_dependency_authority\":false",
                   "artifact proof template must not create target "
                   "dependencies");
  require_contains(artifact_explain, "\"runtime_wave_authority\":false",
                   "artifact proof template must not authorize Runtime waves");
  require_contains(artifact_explain, "\"marshal_reachability\":false",
                   "artifact proof template must not become Marshal "
                   "reachability");
  require_contains(artifact_explain, "\"checkpoint_source_authority\":false",
                   "artifact proof template must not source checkpoints");
  require_contains(artifact_explain,
                   "\"plan_checkpoint_input_authority\":false",
                   "artifact proof template must not feed plan checkpoint "
                   "inputs");
  require_not_contains(artifact_explain, "\"kind\":\"channel_mdn_ready\"",
                       "artifact explain must not leak channel_mdn_ready");
  require_contains(artifact_explain,
                   "\"target_surface_kind\":\"evidence_catalog_artifact\"",
                   "artifact explain should expose evidence catalog surface");
  require_contains(artifact_explain, "\"evidence_catalog_target\":true",
                   "artifact explain should be evidence-catalog scoped");
  require_contains(artifact_explain, "\"dispatchable_target\":false",
                   "artifact explain must not be dispatchable");
  require_contains(artifact_explain, "\"runtime_wave_dispatchable\":false",
                   "artifact explain must not expose runtime waves");
  require_contains(artifact_explain,
                   "\"recommended_operator_action\":\"inspect\"",
                   "artifact explain should route to evidence panel");
  require_contains(artifact_explain,
                   "artifact_readiness targets are evidence catalog proof "
                   "obligations",
                   "artifact explain should include definition dispatch "
                   "boundary");
  require_contains(artifact_explain, "non-dispatchable evidence-catalog proof",
                   "artifact explain text should be non-dispatchable");
  require_contains(
      artifact_explain,
      "\"policy_gate_reservation_summary\":{\"schema\":\"kikijyeba.lattice."
      "policy_gate_reservation.summary.v1\",\"reservation_count\":1",
      "artifact explain should expose target-scoped policy reservations");
  require_contains(artifact_explain, "\"policy_fingerprint_verified_count\":1",
                   "artifact explain should expose target-scoped fingerprint "
                   "verification count");
  require_contains(artifact_explain, "\"policy_fingerprint_mismatch_count\":0",
                   "artifact explain should expose zero target-scoped "
                   "fingerprint mismatches");
  require_contains(artifact_explain,
                   "\"all_policy_fingerprints_verified\":true",
                   "artifact explain should expose the target-scoped "
                   "fingerprint invariant");
  require_contains(artifact_explain,
                   "\"all_policy_input_contracts_complete\":true",
                   "artifact explain should expose target-scoped complete "
                   "policy input contracts");
  require_contains(artifact_explain,
                   "\"policy_gate_catalog_summary\":{\"schema\":\"kikijyeba."
                   "lattice.policy_gate_reservation.summary.v1\","
                   "\"reservation_count\":2",
                   "artifact explain should expose catalog policy reservation "
                   "summary");
  require_contains(artifact_explain,
                   "\"policy_id\":\"forecast_quality_acceptance_reserved\"",
                   "artifact explain should show forecast policy reservation");
  require_contains(artifact_explain,
                   "\"policy_kind\":\"forecast_quality_acceptance\"",
                   "artifact explain should preserve policy kind");
  require_contains(artifact_explain, "\"enabled\":false",
                   "artifact policy reservation must be disabled");
  require_contains(artifact_explain, "\"policy_fingerprint_verified\":true",
                   "artifact policy reservation should expose canonical "
                   "fingerprint verification");
  require_contains(artifact_explain, "\"policy_fingerprint_mismatch\":false",
                   "artifact policy reservation should expose absence of "
                   "fingerprint mismatch");
  require_contains(artifact_explain, "\"policy_input_contract_complete\":true",
                   "artifact policy reservation should expose its complete "
                   "input contract");
  require_contains(artifact_explain, "\"decision_policy_authority\":false",
                   "artifact policy reservation should stay separate from "
                   "decision authority");
  require_contains(artifact_explain, "\"target_status_authority\":false",
                   "artifact policy reservation must not affect target status");
  require_contains(artifact_explain,
                   "\"target_spec_fingerprint_authority\":false",
                   "artifact policy reservation must not affect target "
                   "fingerprints");
  require_contains(artifact_explain, "\"proof_certificate_authority\":false",
                   "artifact policy reservation must not affect proof "
                   "certificates");

  const std::string artifact_deficit =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"deficit\",\"target_id\":"
                          "\"forecast_eval_artifact_ready\","
                          "\"runtime_root\":\"" +
                          runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      artifact_deficit,
      "\"schema\":\"kikijyeba.lattice.target_deficit_response.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":true,"
      "\"target_proof_engine\":\"lattice_target_evaluator\","
      "\"plan_advice_only\":true,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"writes_evidence\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "target_deficit should expose proof-evaluation, read-only, "
      "non-dispatchable boundaries");
  require_contains(artifact_deficit, "\"target_class\":\"artifact_readiness\"",
                   "artifact target deficit should preserve target class");
  require_contains(
      artifact_deficit, "\"kind\":\"not_applicable\"",
      "artifact target deficit must not report a trainable target kind");
  require_contains(artifact_deficit, "\"target_kind_applicable\":false",
                   "artifact target deficit should mark TARGET_KIND not "
                   "applicable");
  require_contains(
      artifact_deficit, "\"target_kind_effective\":\"none\"",
      "artifact target deficit should not expose an effective TARGET_KIND");
  require_contains(artifact_deficit,
                   "\"target_surface_kind\":\"evidence_catalog_artifact\"",
                   "artifact target deficit should expose evidence catalog "
                   "surface");
  require_contains(artifact_deficit, "\"evidence_catalog_target\":true",
                   "artifact target deficit should be evidence-catalog scoped");
  require_contains(artifact_deficit, "\"dispatchable_target\":false",
                   "artifact target deficit must not be dispatchable");
  require_contains(artifact_deficit, "\"runtime_wave_dispatchable\":false",
                   "artifact target deficit must not expose runtime waves");
  require_contains(artifact_deficit,
                   "\"recommended_operator_action\":\"inspect\"",
                   "artifact target deficit should route to evidence panel");
  require_contains(artifact_deficit, "\"suggested_wave\":null",
                   "artifact target deficit should not suggest a wave");
  require_contains(artifact_deficit, "\"key\":\"artifact:forecast_eval_fact\"",
                   "artifact target deficit should be a catalog fact deficit");
  require_contains(artifact_deficit,
                   "\"kind\":\"artifact\",\"dimension\":\"forecast_eval_fact\","
                   "\"status\":\"missing\"",
                   "artifact target deficit should classify missing facts as "
                   "artifact evidence");
  require_contains(artifact_deficit,
                   "artifact_readiness target is non-dispatchable; inspect "
                   "evidence catalog",
                   "artifact target plan basis should be evidence inspection");
  require_contains(artifact_deficit, "\"suggested_action\":\"inspect\"",
                   "artifact target plan basis should suggest evidence panel");
  require_contains(artifact_deficit,
                   "\"fact_integrity_summary_available\":true",
                   "artifact target deficit should expose target-scoped fact "
                   "integrity availability");
  require_contains(artifact_deficit,
                   "\"fact_integrity_summary\":{\"schema\":\"kikijyeba."
                   "lattice.fact_integrity_summary.v1\"",
                   "artifact target deficit should include target-scoped fact "
                   "integrity summary");
  require_contains(artifact_deficit, "\"inspected_family_count\":1",
                   "artifact target fact integrity should scope to the "
                   "subject fact family");
  require_contains(
      artifact_deficit, "\"related_fact_integrity_issue_codes\":[]",
      "artifact target deficits should expose related fact-integrity issue "
      "codes even when empty");
  require_not_contains(artifact_deficit, "\"key\":\"status:target\"",
                       "artifact target deficit should not fall back to a "
                       "generic train/readiness status deficit");

  const std::string artifact_evaluation =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"target\",\"target_id\":\"forecast_"
                          "eval_artifact_ready\","
                          "\"runtime_root\":\"" +
                          runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      artifact_evaluation,
      "\"schema\":\"kikijyeba.lattice.target_evaluation_response.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":true,"
      "\"target_proof_engine\":\"lattice_target_evaluator\","
      "\"plan_advice_only\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"writes_evidence\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "evaluate_target should expose proof-evaluation, read-only, "
      "non-dispatchable boundaries");
  require_contains(
      artifact_evaluation,
      "\"target_surface_kind\":\"evidence_catalog_artifact\"",
      "artifact evaluation should expose evidence catalog surface");
  require_contains(artifact_evaluation, "\"kind\":\"not_applicable\"",
                   "artifact evaluation must not report a trainable target "
                   "kind");
  require_contains(
      artifact_evaluation, "\"target_kind_applicable\":false",
      "artifact evaluation should mark TARGET_KIND not applicable");
  require_contains(
      artifact_evaluation, "\"target_kind_effective\":\"none\"",
      "artifact evaluation should not expose an effective TARGET_KIND");
  require_contains(
      artifact_evaluation,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "artifact evaluation should expose artifact readiness proof template "
      "registry");
  require_contains(artifact_evaluation, "\"dispatchable_target\":false",
                   "artifact evaluation must not be dispatchable");
  require_contains(artifact_evaluation,
                   "\"fact_integrity_summary_available\":true",
                   "artifact evaluation should expose target-scoped fact "
                   "integrity availability");
  require_contains(artifact_evaluation,
                   "\"fact_integrity_summary\":{\"schema\":\"kikijyeba."
                   "lattice.fact_integrity_summary.v1\"",
                   "artifact evaluation should include target-scoped fact "
                   "integrity summary");
  require_contains(artifact_evaluation,
                   "\"recommended_operator_action\":\"inspect\"",
                   "artifact evaluation should route to evidence panel");
  require_contains(artifact_evaluation,
                   "\"key\":\"artifact:forecast_eval_fact\"",
                   "artifact evaluation should preserve catalog fact deficit");
  require_contains(
      artifact_evaluation, "\"related_fact_integrity_issue_codes\":[]",
      "artifact evaluation deficits should expose related fact-integrity issue "
      "codes even when empty");

  const std::string artifact_bulk_evaluation =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"targets\",\"target_ids\":["
                          "\"forecast_eval_artifact_ready\"],"
                          "\"runtime_root\":\"" +
                          runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      artifact_bulk_evaluation,
      "\"schema\":\"kikijyeba.lattice.target_evaluation_batch.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":true,"
      "\"target_proof_engine\":\"lattice_target_evaluator\","
      "\"plan_advice_only\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"writes_evidence\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "evaluate_targets should expose proof-evaluation, read-only, "
      "non-dispatchable boundaries");
  require_contains(artifact_bulk_evaluation, "\"evaluated_target_count\":1",
                   "bulk artifact evaluation should evaluate the requested "
                   "target");
  require_contains(
      artifact_bulk_evaluation,
      "\"target_surface_kind\":\"evidence_catalog_artifact\"",
      "bulk artifact evaluation should expose evidence catalog surface");
  require_contains(artifact_bulk_evaluation,
                   "\"target_class\":\"artifact_readiness\"",
                   "bulk artifact evaluation should preserve target class");
  require_contains(artifact_bulk_evaluation, "\"kind\":\"not_applicable\"",
                   "bulk artifact evaluation must not report a trainable "
                   "target kind");
  require_contains(
      artifact_bulk_evaluation, "\"target_kind_applicable\":false",
      "bulk artifact evaluation should mark TARGET_KIND not applicable");
  require_contains(
      artifact_bulk_evaluation, "\"target_kind_effective\":\"none\"",
      "bulk artifact evaluation should not expose an effective TARGET_KIND");
  require_contains(
      artifact_bulk_evaluation,
      "\"artifact_readiness_proof_template_registry\":{\"schema\":\"kikijyeba."
      "lattice.artifact_readiness_proof_template_registry.v1\"",
      "bulk artifact evaluation should expose artifact readiness proof "
      "template registry");
  require_contains(artifact_bulk_evaluation, "\"dispatchable_target\":false",
                   "bulk artifact evaluation must not be dispatchable");
  require_contains(artifact_bulk_evaluation,
                   "\"fact_integrity_summary_available\":true",
                   "bulk artifact evaluation should expose target-scoped fact "
                   "integrity availability");
  require_contains(artifact_bulk_evaluation,
                   "\"fact_integrity_summary\":{\"schema\":\"kikijyeba."
                   "lattice.fact_integrity_summary.v1\"",
                   "bulk artifact evaluation should include target-scoped fact "
                   "integrity summary");
  require_contains(
      artifact_bulk_evaluation, "\"related_fact_integrity_issue_codes\":[]",
      "bulk artifact evaluation deficits should expose related fact-integrity "
      "issue codes even when empty");
  require_contains(
      artifact_bulk_evaluation,
      "\"policy_gate_reservation_summary\":{\"schema\":\"kikijyeba.lattice."
      "policy_gate_reservation.summary.v1\",\"reservation_count\":1,"
      "\"target_match_count\":1,\"enabled_policy_gate_count\":0",
      "bulk artifact evaluation rows should expose target-filtered disabled "
      "policy gate reservations");
  require_contains(
      artifact_bulk_evaluation, "\"policy_fingerprint_verified_count\":1",
      "bulk artifact evaluation rows should expose fingerprint verification "
      "count");
  require_contains(artifact_bulk_evaluation,
                   "\"policy_fingerprint_mismatch_count\":0",
                   "bulk artifact evaluation rows should expose zero "
                   "fingerprint mismatches");
  require_contains(
      artifact_bulk_evaluation, "\"policy_input_contract_complete_count\":1",
      "bulk artifact evaluation rows should expose complete policy input "
      "contract count");
  require_contains(
      artifact_bulk_evaluation, "\"all_policy_input_contracts_complete\":true",
      "bulk artifact evaluation rows should expose complete policy input "
      "contracts");
  require_contains(
      artifact_bulk_evaluation, "\"policy_fingerprint_verified\":true",
      "bulk artifact evaluation rows should expose verified policy "
      "fingerprints");
  require_contains(
      artifact_bulk_evaluation,
      "\"policy_id\":\"forecast_quality_acceptance_reserved\"",
      "bulk artifact evaluation rows should preserve the matching disabled "
      "policy gate reservation");

  const fs::path scanned_runtime_root =
      "/tmp/hero_mcp_schema_compat/lattice_scanned_artifact";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      scanned_runtime_root, /*forecast_baseline_digest_mismatch=*/false);
  const std::string fact_preview = read_command_stdout(
      base +
      "--tool hero.lattice.inspect "
      "--args-json "
      "'{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
      scanned_runtime_root.string() +
      "\",\"family\":\"forecast_eval\",\"fact_index\":0,\"limit\":1}'");
  require_contains(
      fact_preview,
      "\"schema\":\"kikijyeba.lattice.fact_preview.v1\",\"runtime_root\":\"" +
          scanned_runtime_root.string() +
          "\",\"family\":\"forecast_eval\",\"relation\":\"forecast_eval\","
          "\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
          "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":"
          "true",
      "fact_preview should expose top-level non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(
      fact_preview, "hero.lattice.inspect subject=facts mode=preview");
  require_contains(fact_preview, "\"preview_rows_are_audit_only\":true",
                   "fact_preview rows should be audit-only");
  require_contains(fact_preview, "\"facts_used_for_target_satisfaction\":false",
                   "fact_preview facts must not satisfy targets");
  require_contains(fact_preview,
                   "\"cache_rows_used_for_target_satisfaction\":false",
                   "fact_preview lineage rows must not satisfy targets");
  require_contains(fact_preview, "\"checkpoint_selected\":false",
                   "fact_preview must not select checkpoints");
  require_contains(fact_preview, "\"model_selector\":false",
                   "fact_preview must not rank models");
  require_contains(fact_preview, "\"fact_index_filter\":0",
                   "fact_preview should expose the selected fact index");
  require_contains(fact_preview, "\"matching_fact_count\":1",
                   "fact_preview should find the selected forecast fact");
  require_contains(fact_preview, "\"returned_fact_count\":1",
                   "fact_preview should return the selected forecast fact");
  require_contains(fact_preview, "\"matching_lineage_row_count\":1",
                   "fact_preview should relay matching runtime-index lineage");
  require_contains(fact_preview, "\"returned_lineage_row_count\":1",
                   "fact_preview should return matching runtime-index lineage");
  require_contains(fact_preview, "\"fact_index\":0,\"digest\":\"",
                   "fact_preview rows should carry digest and index");
  require_contains(fact_preview,
                   "\"identity_envelope\":{\"schema\":\"kikijyeba.lattice."
                   "fact_identity_envelope.v1\"",
                   "fact_preview rows should carry the normalized fact "
                   "identity envelope");
  require_contains(fact_preview,
                   "\"fact_identity_contract_schema\":\"kikijyeba.lattice."
                   "fact_identity_contract.v1\"",
                   "fact_preview identity envelopes should bind the common "
                   "fact identity contract");
  require_contains(fact_preview, "\"fact_family\":\"forecast_eval\"",
                   "fact_preview identity envelopes should name the fact "
                   "family");
  require_contains(fact_preview, "\"parent_exposure_fact_digests\":[",
                   "fact_preview identity envelopes should expose parent "
                   "exposure lineage");
  require_contains(fact_preview, "\"support_count\":42",
                   "fact_preview identity envelopes should normalize support "
                   "counters");
  require_contains(fact_preview,
                   "\"row_index_interval_authority\":true,"
                   "\"source_key_window_audit_only\":true",
                   "fact_preview identity envelopes should preserve row-index "
                   "authority and source-key audit boundaries");
  require_contains(fact_preview,
                   "\"forecast_artifact_digest\":\"forecast_artifact_1\"",
                   "fact_preview should include the concrete forecast fact");

  const std::string scanned_baseline_evaluation =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"target\",\"target_id\":\"forecast_"
                          "baseline_artifact_ready\","
                          "\"runtime_root\":\"" +
                          scanned_runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      scanned_baseline_evaluation, "\"status\":\"satisfied\"",
      "scanned baseline artifact evaluation should satisfy target");
  require_contains(
      scanned_baseline_evaluation,
      "\"warning_id\":\"forecast_baseline_valid_support_low_visibility_only\"",
      "forecast baseline warning result should be serialized");
  require_contains(
      scanned_baseline_evaluation,
      "\"warning_id\":\"forecast_baseline_kind_coverage_low_visibility_only\"",
      "forecast baseline kind-coverage warning result should be serialized");
  require_contains(
      scanned_baseline_evaluation,
      "\"warning_id\":\"forecast_baseline_metric_coverage_low_visibility_"
      "only\"",
      "forecast baseline metric-coverage warning result should be serialized");
  require_contains(scanned_baseline_evaluation,
                   "\"kind\":\"forecast_baseline\"",
                   "forecast baseline warning should preserve kind");
  require_contains(scanned_baseline_evaluation, "\"status\":\"warning\"",
                   "forecast baseline warning should trigger");
  require_contains(
      scanned_baseline_evaluation, "\"threshold_relation\":\"below_threshold\"",
      "forecast baseline warning should expose threshold relation");
  require_contains(scanned_baseline_evaluation, "\"measured_value\":40",
                   "forecast baseline warning should expose valid support");
  require_contains(scanned_baseline_evaluation, "\"threshold\":50",
                   "forecast baseline warning should expose configured "
                   "threshold");
  require_contains(scanned_baseline_evaluation,
                   "\"threshold_direction\":\"below\"",
                   "forecast baseline warning should expose threshold "
                   "direction");
  require_contains(scanned_baseline_evaluation, "\"unit\":\"count\"",
                   "forecast baseline support should be a count metric");
  require_contains(scanned_baseline_evaluation,
                   "\"evidence_basis\":\"forecast_baseline_facts\"",
                   "forecast baseline warning should name catalog facts as "
                   "basis");
  require_contains(scanned_baseline_evaluation,
                   "\"diagnostic_sample_count\":40",
                   "forecast baseline warning should expose valid support "
                   "count");
  require_contains(scanned_baseline_evaluation,
                   "\"diagnostic_metric_family\":\"baseline_kind_coverage\"",
                   "forecast baseline kind coverage should remain a diagnostic "
                   "family");
  require_contains(scanned_baseline_evaluation,
                   "\"use\":\"evaluation_baseline\"",
                   "forecast baseline warning should be baseline scoped");
  require_contains(scanned_baseline_evaluation, "\"effect\":\"none\"",
                   "forecast baseline warning must not require model mutation");
  require_contains(scanned_baseline_evaluation, "\"fact_count\":1",
                   "forecast baseline warning should count matching facts");
  require_contains(scanned_baseline_evaluation, "\"plan_ready\":false",
                   "forecast baseline warning must not make artifact target "
                   "plan-ready");
  require_contains(scanned_baseline_evaluation,
                   "\"warning_id\":\"forecast_baseline_valid_support_low_"
                   "visibility_only\"",
                   "forecast baseline warning should project into typed target "
                   "warnings");
  require_contains(scanned_baseline_evaluation,
                   "\"warning_family\":\"forecast_support\"",
                   "forecast baseline support warning should expose support "
                   "warning family");
  require_contains(scanned_baseline_evaluation,
                   "\"warning_family\":\"forecast_baseline_comparison\"",
                   "forecast baseline kind/metric warnings should expose "
                   "baseline-comparison warning family");
  require_contains(scanned_baseline_evaluation,
                   "\"warning_messages\":[\"forecast-baseline valid_count",
                   "forecast baseline warning messages remain visible");

  const std::string scanned_observer_evaluation =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"target\",\"target_id\":\"observer_"
                          "belief_artifact_ready\","
                          "\"runtime_root\":\"" +
                          scanned_runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      scanned_observer_evaluation, "\"status\":\"satisfied\"",
      "scanned observer artifact evaluation should satisfy target");
  require_contains(scanned_observer_evaluation,
                   "\"fact_family\":\"observer_belief\"",
                   "scanned observer proof should identify observer_belief");
  require_contains(
      scanned_observer_evaluation,
      "\"warning_id\":\"observer_belief_confidence_low_visibility_only\"",
      "observer belief warning result should be serialized");
  require_contains(
      scanned_observer_evaluation,
      "\"warning_id\":\"observer_belief_data_quality_low_visibility_only\"",
      "observer belief data-quality warning result should be serialized");
  require_contains(
      scanned_observer_evaluation,
      "\"warning_id\":\"observer_belief_liquidity_low_visibility_only\"",
      "observer belief liquidity warning result should be serialized");
  require_contains(scanned_observer_evaluation, "\"kind\":\"observer_belief\"",
                   "observer belief warning should preserve kind");
  require_contains(scanned_observer_evaluation, "\"status\":\"warning\"",
                   "observer belief warning should trigger");
  require_contains(scanned_observer_evaluation,
                   "\"threshold_relation\":\"below_threshold\"",
                   "observer belief warning should expose threshold relation");
  require_contains(scanned_observer_evaluation, "\"measured_value\":0.64",
                   "observer belief warning should expose confidence");
  require_contains(
      scanned_observer_evaluation, "\"threshold\":0.7",
      "observer belief warning should expose configured threshold");
  require_contains(scanned_observer_evaluation,
                   "\"threshold_direction\":\"below\"",
                   "observer belief warning should expose threshold direction");
  require_contains(scanned_observer_evaluation, "\"unit\":\"fraction\"",
                   "observer belief confidence should be a fraction");
  require_contains(
      scanned_observer_evaluation,
      "\"evidence_basis\":\"observer_belief_facts\"",
      "observer belief warning should name catalog facts as basis");
  require_contains(scanned_observer_evaluation, "\"diagnostic_sample_count\":1",
                   "observer belief warning should count matching facts");
  require_contains(scanned_observer_evaluation, "\"use\":\"observer_belief\"",
                   "observer belief warning should be observer scoped");
  require_contains(scanned_observer_evaluation, "\"effect\":\"none\"",
                   "observer belief warning must not require model mutation");
  require_contains(scanned_observer_evaluation, "\"fact_count\":1",
                   "observer belief warning should count matching facts");
  require_contains(scanned_observer_evaluation, "\"plan_ready\":false",
                   "observer belief warning must not make artifact target "
                   "plan-ready");
  require_contains(scanned_observer_evaluation,
                   "\"warning_id\":\"observer_belief_confidence_low_"
                   "visibility_only\"",
                   "observer belief warning should project into typed target "
                   "warnings");
  require_contains(scanned_observer_evaluation,
                   "\"warning_family\":\"observer_belief_consistency\"",
                   "observer belief warning should expose warning family");
  require_contains(scanned_observer_evaluation,
                   "\"warning_messages\":[\"observer-belief confidence",
                   "observer belief warning messages remain visible");

  const std::string scanned_allocation_evaluation = read_command_stdout(
      base +
      "--tool hero.lattice.evaluate "
      "--args-json "
      "'{\"operation\":\"target\",\"target_id\":\"allocation_artifact_ready\","
      "\"runtime_root\":\"" +
      scanned_runtime_root.string() +
      "\",\"protocol_contract_fingerprint\":\"contract_1\","
      "\"graph_order_fingerprint\":\"graph_1\","
      "\"source_cursor_token\":\"cursor_1\","
      "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(
      scanned_allocation_evaluation, "\"status\":\"satisfied\"",
      "scanned allocation artifact evaluation should satisfy target");
  require_contains(scanned_allocation_evaluation,
                   "\"fact_family\":\"allocation_engine\"",
                   "scanned allocation proof should identify "
                   "allocation_engine");
  require_contains(
      scanned_allocation_evaluation,
      "\"warning_id\":\"allocation_turnover_high_visibility_only\"",
      "allocation engine warning result should be serialized");
  require_contains(
      scanned_allocation_evaluation,
      "\"warning_id\":\"allocation_cap_diagnostics_missing_visibility_only\"",
      "allocation cap diagnostic warning result should be serialized");
  require_contains(scanned_allocation_evaluation,
                   "\"warning_id\":\"allocation_scenario_growth_floor_"
                   "attention_visibility_only\"",
                   "allocation growth-floor warning result should be "
                   "serialized");
  require_contains(
      scanned_allocation_evaluation,
      "\"warning_id\":\"allocation_fallback_active_visibility_only\"",
      "allocation fallback warning result should be serialized");
  require_contains(
      scanned_allocation_evaluation,
      "\"warning_id\":\"allocation_derisk_active_visibility_only\"",
      "allocation de-risk warning result should be serialized");
  require_contains(scanned_allocation_evaluation,
                   "\"kind\":\"allocation_engine\"",
                   "allocation engine warning should preserve kind");
  require_contains(scanned_allocation_evaluation, "\"status\":\"warning\"",
                   "allocation engine warning should trigger");
  require_contains(scanned_allocation_evaluation,
                   "\"threshold_relation\":\"above_threshold\"",
                   "allocation engine warning should expose threshold "
                   "relation");
  require_contains(scanned_allocation_evaluation, "\"measured_value\":0.12",
                   "allocation engine warning should expose turnover");
  require_contains(
      scanned_allocation_evaluation, "\"threshold\":0.1",
      "allocation engine warning should expose configured threshold");
  require_contains(scanned_allocation_evaluation,
                   "\"threshold_direction\":\"above\"",
                   "allocation engine warning should expose threshold "
                   "direction");
  require_contains(scanned_allocation_evaluation, "\"unit\":\"fraction\"",
                   "allocation engine turnover should be a fraction");
  require_contains(scanned_allocation_evaluation,
                   "\"evidence_basis\":\"allocation_engine_facts\"",
                   "allocation engine warning should name catalog facts as "
                   "basis");
  require_contains(scanned_allocation_evaluation,
                   "\"diagnostic_sample_count\":1",
                   "allocation engine warning should count matching facts");
  require_contains(scanned_allocation_evaluation,
                   "\"use\":\"allocation_engine\"",
                   "allocation engine warning should be allocation scoped");
  require_contains(scanned_allocation_evaluation, "\"effect\":\"none\"",
                   "allocation engine warning must not require execution");
  require_contains(scanned_allocation_evaluation, "\"fact_count\":1",
                   "allocation engine warning should count matching facts");
  require_contains(scanned_allocation_evaluation, "\"plan_ready\":false",
                   "allocation engine warning must not make artifact target "
                   "plan-ready");
  require_contains(scanned_allocation_evaluation,
                   "\"warning_id\":\"allocation_turnover_high_visibility_"
                   "only\"",
                   "allocation engine warning should project into typed target "
                   "warnings");
  require_contains(scanned_allocation_evaluation,
                   "\"warning_family\":\"allocation_engine_diagnostics\"",
                   "allocation engine warning should expose warning family");
  require_contains(scanned_allocation_evaluation,
                   "\"warning_messages\":[\"allocation-engine turnover",
                   "allocation engine warning messages remain visible");

  const std::string scanned_artifact_evaluation =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"target\",\"target_id\":\"forecast_"
                          "eval_artifact_ready\","
                          "\"runtime_root\":\"" +
                          scanned_runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(scanned_artifact_evaluation, "\"status\":\"satisfied\"",
                   "scanned artifact evaluation should satisfy target");
  require_contains(scanned_artifact_evaluation,
                   "\"target_surface_kind\":\"evidence_catalog_artifact\"",
                   "scanned artifact evaluation should remain catalog-scoped");
  require_contains(scanned_artifact_evaluation, "\"dispatchable_target\":false",
                   "scanned artifact evaluation must not become dispatchable");
  require_contains(scanned_artifact_evaluation,
                   "\"recommended_operator_action\":\"inspect\"",
                   "scanned artifact evaluation should route to inspection");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_integrity_summary_available\":true",
                   "scanned artifact evaluation should expose fact integrity");
  require_contains(scanned_artifact_evaluation, "\"relation_declared_count\":3",
                   "scanned artifact fact integrity should declare transform, "
                   "baseline, and selection-signal relations");
  require_contains(scanned_artifact_evaluation, "\"relation_bound_count\":3",
                   "scanned artifact fact integrity should bind all relations");
  require_contains(scanned_artifact_evaluation,
                   "\"relation_integrity_clean\":true",
                   "scanned artifact fact integrity should be clean");
  require_contains(scanned_artifact_evaluation, "\"deficits\":[]",
                   "scanned artifact evaluation should have no deficits");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_family\":\"forecast_eval\"",
                   "scanned artifact proof should identify forecast_eval");
  require_contains(scanned_artifact_evaluation, "\"proof_template_bound\":true",
                   "scanned artifact proof should bind an explicit proof "
                   "template");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_schema\":\"kikijyeba.lattice.forecast_eval.v1\"",
                   "scanned artifact proof should bind the fact schema");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_contract_schema\":\"kikijyeba.lattice."
                   "fact_identity_contract.v1\"",
                   "scanned artifact proof should bind the fact identity "
                   "contract schema");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_contract_id\":\"runtime_evidence_identity_"
                   "envelope\"",
                   "scanned artifact proof should bind the fact identity "
                   "contract id");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_contract_bound\":true",
                   "scanned artifact proof should bind the catalog identity "
                   "contract");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_envelope_complete\":true",
                   "scanned artifact proof should carry a complete identity "
                   "envelope");
  require_contains(scanned_artifact_evaluation,
                   "\"row_index_interval_authority\":true",
                   "scanned artifact proof should keep row-index authority");
  require_contains(scanned_artifact_evaluation,
                   "\"source_key_window_audit_only\":true",
                   "scanned artifact proof should keep source-key windows "
                   "audit-only");
  require_contains(scanned_artifact_evaluation,
                   "forecast evaluation artifact existence, checkpoint "
                   "lineage",
                   "scanned artifact proof should expose the proof-template "
                   "claim");
  require_contains(scanned_artifact_evaluation, "\"lineage_bound\":true",
                   "scanned artifact proof should bind lineage");
  require_contains(scanned_artifact_evaluation, "\"authority_clean\":true",
                   "scanned artifact proof should stay authority-clean");
  require_contains(scanned_artifact_evaluation,
                   "\"target_dependency_authority\":false",
                   "scanned artifact proof must not create target "
                   "dependencies");
  require_contains(scanned_artifact_evaluation,
                   "\"runtime_wave_authority\":false",
                   "scanned artifact proof must not authorize Runtime waves");
  require_contains(scanned_artifact_evaluation,
                   "\"marshal_reachability\":false",
                   "scanned artifact proof must not become Marshal "
                   "reachability");
  require_contains(scanned_artifact_evaluation,
                   "\"checkpoint_source_authority\":false",
                   "scanned artifact proof must not source checkpoints");
  require_contains(scanned_artifact_evaluation,
                   "\"plan_checkpoint_input_authority\":false",
                   "scanned artifact proof must not feed plan checkpoint "
                   "inputs");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_target_kind_authority\":false",
                   "scanned artifact proof identity contract must not create "
                   "target kinds");
  require_contains(
      scanned_artifact_evaluation,
      "\"fact_identity_runtime_wave_authority\":false",
      "scanned artifact proof identity contract must not authorize "
      "Runtime waves");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_marshal_reachability\":false",
                   "scanned artifact proof identity contract must not create "
                   "Marshal reachability");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_identity_policy_gate_authority\":false",
                   "scanned artifact proof identity contract must not create "
                   "policy gates");
  require_contains(scanned_artifact_evaluation,
                   "\"proof_certificate_check\":{\"passed\":true",
                   "scanned artifact proof certificate should pass");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_id\":\"forecast_eval_calibration_coverage_low_"
                   "visibility_only\"",
                   "forecast eval warning result should be serialized");
  require_contains(scanned_artifact_evaluation, "\"kind\":\"forecast_eval\"",
                   "forecast eval warning should preserve kind");
  require_contains(scanned_artifact_evaluation, "\"status\":\"warning\"",
                   "forecast eval warning should trigger");
  require_contains(scanned_artifact_evaluation,
                   "\"threshold_relation\":\"below_threshold\"",
                   "forecast eval warning should expose threshold relation");
  require_contains(scanned_artifact_evaluation, "\"measured_value\":0.91",
                   "forecast eval warning should expose calibration coverage");
  require_contains(scanned_artifact_evaluation, "\"threshold\":0.95",
                   "forecast eval warning should expose configured threshold");
  require_contains(scanned_artifact_evaluation,
                   "\"threshold_direction\":\"below\"",
                   "forecast eval warning should expose threshold direction");
  require_contains(scanned_artifact_evaluation, "\"unit\":\"fraction\"",
                   "forecast eval calibration coverage should be a fraction");
  require_contains(scanned_artifact_evaluation,
                   "\"evidence_basis\":\"forecast_eval_facts\"",
                   "forecast eval warning should name catalog facts as basis");
  require_contains(scanned_artifact_evaluation,
                   "\"diagnostic_sample_count\":40",
                   "forecast eval warning should expose valid target support");
  require_contains(scanned_artifact_evaluation, "\"use\":\"evaluation_metric\"",
                   "forecast eval warning should be evaluation-metric scoped");
  require_contains(scanned_artifact_evaluation, "\"effect\":\"none\"",
                   "forecast eval warning must not require model mutation");
  require_contains(scanned_artifact_evaluation, "\"fact_count\":1",
                   "forecast eval warning should count matching facts");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_id\":\"forecast_eval_calibration_coverage_low_"
                   "visibility_only\"",
                   "forecast eval warning should project into typed target "
                   "warnings");
  require_contains(scanned_artifact_evaluation, "\"source\":\"lattice\"",
                   "forecast eval warning should carry source identity");
  require_contains(scanned_artifact_evaluation, "\"blocking\":false",
                   "forecast eval warning should remain non-blocking");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_family\":\"forecast_calibration\"",
                   "forecast eval calibration warning should expose its "
                   "warning family");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_family\":\"forecast_baseline_comparison\"",
                   "forecast eval skill warning should expose baseline "
                   "comparison family");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_family\":\"forecast_support\"",
                   "forecast eval horizon-support warning should expose "
                   "support family");
  require_contains(scanned_artifact_evaluation,
                   "\"fact_family\":\"forecast_eval\"",
                   "forecast eval warning should bind to its fact family");
  require_contains(scanned_artifact_evaluation, "\"evidence_digest\":\"",
                   "forecast eval warning should carry a stable evidence "
                   "digest");
  require_contains(scanned_artifact_evaluation,
                   "\"warning_messages\":[\"forecast-eval "
                   "calibration_coverage",
                   "forecast eval warning messages remain visible");
  require_contains(scanned_artifact_evaluation,
                   "\"all_warnings_non_blocking\":true",
                   "forecast eval warning summary should prove warnings are "
                   "non-blocking");

  const std::string scanned_artifact_bulk =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"targets\",\"target_ids\":["
                          "\"forecast_eval_artifact_ready\"],"
                          "\"runtime_root\":\"" +
                          scanned_runtime_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(scanned_artifact_bulk, "\"evaluated_target_count\":1",
                   "bulk scanned artifact evaluation should evaluate one "
                   "target");
  require_contains(scanned_artifact_bulk, "\"status\":\"satisfied\"",
                   "bulk scanned artifact evaluation should satisfy target");
  require_contains(scanned_artifact_bulk, "\"relation_bound_count\":3",
                   "bulk scanned artifact evaluation should bind all "
                   "relations");
  require_contains(scanned_artifact_bulk, "\"deficits\":[]",
                   "bulk scanned artifact evaluation should have no deficits");
  require_contains(scanned_artifact_bulk,
                   "\"proof_certificate_check\":{\"passed\":true",
                   "bulk scanned artifact proof certificate should pass");

  const fs::path scanned_bad_baseline_root =
      "/tmp/hero_mcp_schema_compat/lattice_scanned_bad_baseline";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      scanned_bad_baseline_root,
      /*forecast_baseline_digest_mismatch=*/true);
  const std::string scanned_bad_baseline =
      read_command_stdout(base +
                          "--tool hero.lattice.evaluate "
                          "--args-json "
                          "'{\"operation\":\"target\",\"target_id\":\"forecast_"
                          "eval_artifact_ready\","
                          "\"runtime_root\":\"" +
                          scanned_bad_baseline_root.string() +
                          "\",\"protocol_contract_fingerprint\":\"contract_1\","
                          "\"graph_order_fingerprint\":\"graph_1\","
                          "\"source_cursor_token\":\"cursor_1\","
                          "\"mdn_assembly_fingerprint\":\"mdn_1\"}'");
  require_contains(scanned_bad_baseline, "\"status\":\"blocked\"",
                   "scanned bad-baseline artifact should block target");
  require_contains(scanned_bad_baseline, "\"baseline_fact_digest_not_found\"",
                   "scanned bad-baseline artifact should expose missing "
                   "baseline digest");
  require_contains(scanned_bad_baseline, "\"relation_declared_count\":3",
                   "scanned bad-baseline fact integrity should still declare "
                   "three relations");
  require_contains(scanned_bad_baseline, "\"relation_bound_count\":2",
                   "scanned bad-baseline fact integrity should bind only two "
                   "relations");
  require_contains(scanned_bad_baseline, "\"unresolved_relation_count\":1",
                   "scanned bad-baseline fact integrity should expose one "
                   "unresolved relation");
  require_contains(scanned_bad_baseline, "\"relation_integrity_clean\":false",
                   "scanned bad-baseline fact integrity should not be clean");
  require_contains(scanned_bad_baseline,
                   "\"key\":\"artifact:forecast_eval_issue_baseline_fact_"
                   "digest_not_found\"",
                   "scanned bad-baseline artifact deficit should be explicit");
  require_contains(scanned_bad_baseline,
                   "\"related_fact_integrity_issue_codes\":[\"artifact_job:"
                   "baseline_fact_digest_not_found\"]",
                   "scanned bad-baseline artifact deficit should carry related "
                   "fact-integrity issue code");
  require_contains(scanned_bad_baseline,
                   "\"fact_preview_hint\":{\"available\":"
                   "true,\"tool\":\"hero.lattice.inspect\","
                   "\"subject\":\"facts\",\"mode\":\"preview\"",
                   "scanned bad-baseline artifact proof should point to fact "
                   "preview");
  require_contains(scanned_bad_baseline,
                   "\"marshal_tool\":\"hero.marshal.inspect\","
                   "\"fact_family\":\"forecast_eval\",\"fact_digest\":\"",
                   "scanned bad-baseline artifact preview hint should include "
                   "family and digest");
  require_contains(scanned_bad_baseline,
                   "\"facts_used_for_target_satisfaction\":false,"
                   "\"checkpoint_selected\":false,\"model_selector\":false",
                   "artifact preview hints must remain audit-only");
  require_contains(scanned_bad_baseline, "\"fact_preview_hint_count\":1",
                   "artifact plan basis should count preview hints");
  require_contains(scanned_bad_baseline,
                   "\"primary_fact_preview_hint\":{"
                   "\"available\":true",
                   "artifact plan basis should expose primary preview hint");
  require_contains(scanned_bad_baseline, "\"fact_preview_hints\":[{",
                   "artifact plan basis should expose preview hint list");
}

} // namespace

int main() {
  const auto one_of_issues = schema::validate_tool_input_schema(
      "hero_lattice_checkpoint_closure",
      R"({"type":"object","oneOf":[],"properties":{}})");
  assert(!one_of_issues.empty());
  assert(one_of_issues.front().tool_name == "hero_lattice_checkpoint_closure");
  assert(one_of_issues.front().schema_path == "inputSchema.oneOf");
  assert(one_of_issues.front().construct == "oneOf");
  assert(schema::format_issue(one_of_issues.front())
             .find("hero_lattice_checkpoint_closure") != std::string::npos);

  const auto enum_issues = schema::validate_tool_input_schema(
      "hero.bad.enum", R"({"type":"object","enum":["x"]})");
  assert(!enum_issues.empty());
  assert(enum_issues.front().schema_path == "inputSchema.enum");
  assert(enum_issues.front().construct == "enum");

  const auto type_issues = schema::validate_tool_input_schema(
      "hero.bad.type", R"({"type":"string","properties":{}})");
  assert(!type_issues.empty());
  assert(type_issues.front().schema_path == "inputSchema.type");
  assert(type_issues.front().construct == "type");

  const auto ok_issues = schema::validate_tool_input_schema(
      "hero.good", R"({"type":"object","properties":{},"required":[]})");
  assert(ok_issues.empty());

  const fs::path hero_root = "/cuwacunu/.build/hero";
  const auto config_binary = first_existing(
      {hero_root / "hero_config.mcp", hero_root / "hero_config_mcp"});
  check_catalog("Config", config_binary, false);
  check_config_collapsed_surface(config_binary);
  const auto runtime_binary = first_existing(
      {hero_root / "hero_runtime.mcp", hero_root / "hero_runtime_mcp"});
  check_catalog("Runtime", runtime_binary, false);
  check_runtime_collapsed_surface(runtime_binary);
  const auto lattice_binary = first_existing(
      {hero_root / "hero_lattice.mcp", hero_root / "hero_lattice_mcp"});
  check_catalog("Lattice", lattice_binary, true);
  check_lattice_selector_boundaries(lattice_binary);
  check_catalog("Marshal",
                first_existing({hero_root / "hero_marshal.mcp",
                                hero_root / "hero_marshal_mcp"}),
                false);

  return 0;
}
