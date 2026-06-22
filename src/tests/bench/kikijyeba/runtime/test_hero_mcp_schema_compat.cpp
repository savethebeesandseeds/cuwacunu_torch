#include "hero/mcp_schema_compat.h"
#include "tests/bench/kikijyeba/test_support/lattice_forecast_artifact_fixture.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
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

[[nodiscard]] bool is_lattice_identity_filter_field(const std::string &field) {
  return field == "protocol_id" || field == "protocol_contract_fingerprint" ||
         field == "graph_order_fingerprint" || field == "source_cursor_token" ||
         field == "vicreg_assembly_fingerprint" ||
         field == "mtf_jepa_mae_vicreg_assembly_fingerprint" ||
         field == "mdn_assembly_fingerprint";
}

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

[[nodiscard]] bool skip_json_value(const std::string &s, std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size()) {
    return false;
  }
  if (s[*idx] == '"') {
    (void)parse_json_string_at(s, idx);
    return true;
  }
  if (s[*idx] == '{' || s[*idx] == '[') {
    const char open = s[*idx];
    const char close = open == '{' ? '}' : ']';
    int depth = 0;
    while (*idx < s.size()) {
      if (s[*idx] == '"') {
        (void)parse_json_string_at(s, idx);
        continue;
      }
      if (s[*idx] == open) {
        ++depth;
      } else if (s[*idx] == close) {
        --depth;
        ++(*idx);
        if (depth == 0) {
          return true;
        }
        continue;
      }
      ++(*idx);
    }
    return false;
  }
  const std::size_t begin = *idx;
  while (*idx < s.size() && s[*idx] != ',' && s[*idx] != '}' &&
         s[*idx] != ']') {
    ++(*idx);
  }
  return *idx > begin;
}

struct json_object_field_t {
  std::string key{};
  std::string raw{};
};

[[nodiscard]] std::vector<json_object_field_t>
parse_json_object_fields(const std::string &json) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    throw std::runtime_error("expected JSON object");
  }
  ++idx;
  std::vector<json_object_field_t> fields;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        throw std::runtime_error("trailing data after JSON object");
      }
      return fields;
    }
    std::string key = parse_json_string_at(json, &idx);
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      throw std::runtime_error("expected ':' after JSON object key");
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      throw std::runtime_error("invalid JSON value for key: " + key);
    }
    fields.push_back(
        {std::move(key), json.substr(value_begin, idx - value_begin)});
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      continue;
    }
    throw std::runtime_error("expected ',' or '}' in JSON object");
  }
  throw std::runtime_error("unterminated JSON object");
}

[[nodiscard]] std::string first_json_string_field(const std::string &json,
                                                  const std::string &field) {
  const std::string key = "\"" + field + "\"";
  const std::size_t key_pos = json.find(key);
  if (key_pos == std::string::npos) {
    throw std::runtime_error("missing JSON field: " + field);
  }
  std::size_t value_pos = json.find(':', key_pos + key.size());
  if (value_pos == std::string::npos) {
    throw std::runtime_error("missing JSON field separator: " + field);
  }
  ++value_pos;
  return parse_json_string_at(json, &value_pos);
}

[[nodiscard]] std::vector<std::string>
json_string_field_values(const std::string &json, const std::string &field) {
  std::vector<std::string> out;
  const std::string key = "\"" + field + "\"";
  std::size_t pos = 0;
  while ((pos = json.find(key, pos)) != std::string::npos) {
    std::size_t value_pos = json.find(':', pos + key.size());
    if (value_pos == std::string::npos) {
      break;
    }
    ++value_pos;
    out.push_back(parse_json_string_at(json, &value_pos));
    pos = value_pos;
  }
  return out;
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

void append_forecast_eval_prefix_candidate_fixture(
    const fs::path &runtime_root, int candidate_index) {
  auto options = lattice_fixture::validation_holdout_forecast_artifact_options(
      /*forecast_baseline_digest_mismatch=*/false);
  options.job_id =
      "artifact_job_prefix_collision_" + std::to_string(candidate_index);
  options.wave_id =
      "artifact_wave_prefix_collision_" + std::to_string(candidate_index);

  const fs::path job_dir = runtime_root / options.job_id;
  fs::create_directories(job_dir);
  lattice_fixture::write_fixture_text(job_dir / "job.manifest", "");

  exposure::lattice_exposure_fact_t parent_seed{};
  parent_seed.fact_type = "exposure";
  parent_seed.contract_fingerprint = options.contract_fingerprint;
  parent_seed.protocol_id = options.protocol_id;
  parent_seed.graph_order_fingerprint = options.graph_order_fingerprint;
  parent_seed.source_cursor_token = options.source_cursor_token;
  parent_seed.component_assembly_fingerprint =
      options.component_assembly_fingerprint;
  parent_seed.target_component_family_id = options.target_component_family_id;
  parent_seed.job_id = options.job_id;
  parent_seed.wave_id = options.wave_id;
  parent_seed.wave_action = options.wave_action;
  parent_seed.job_status = options.job_status;
  lattice_fixture::apply_split_identity(parent_seed, options);
  parent_seed.completed_anchor_range = parent_seed.anchor_range;

  const auto exposure_sidecar =
      exposure::exposure_fact_path_for_job_dir(job_dir);
  exposure::write_exposure_fact_sidecar(exposure_sidecar, parent_seed);
  const auto parent =
      exposure::make_exposure_fact_from_sidecar_file(exposure_sidecar, job_dir);

  exposure::lattice_forecast_eval_fact_t forecast{};
  exposure::populate_artifact_fact_identity(forecast, parent);
  forecast.target_feature_ids = {"ev_close", "ev_return"};
  forecast.horizon = 3;
  forecast.support_count = 42;
  forecast.valid_count = 40;
  forecast.missing_count = 2;
  forecast.weakest_support_rows = 8;
  forecast.mean_nll = 1.10;
  forecast.mean_nll_per_horizon = {1.10};
  forecast.valid_target_count_per_horizon = {40};
  forecast.ev_mae = 0.16;
  forecast.ev_rmse = 0.24;
  forecast.signed_error = -0.02;
  forecast.directional_accuracy = 0.62;
  forecast.calibration_coverage = 0.91;
  forecast.pit_summary = "uniform-ish";
  forecast.sigma_scale_sanity = "ok";
  forecast.support_by_node = "BTC:20,ETH:20";
  forecast.support_by_channel = "close:40";
  forecast.support_by_target_feature = "ev_close:40,ev_return:40";
  forecast.support_by_horizon = "h3:40";
  forecast.evaluated_representation_checkpoint_digest = "rep_checkpoint_2";
  forecast.evaluated_mdn_checkpoint_digest = "mdn_checkpoint_2";
  forecast.target_transform_fact_digest = "target_transform_2";
  forecast.baseline_fact_digests = {"baseline_2"};
  forecast.selection_signal_fact_digests = {"selection_2"};

  forecast.forecast_artifact_digest =
      "forecast_artifact_prefix_collision_" + std::to_string(candidate_index);
  exposure::write_forecast_eval_fact_sidecar(
      job_dir / "lattice.forecast_eval.fact", forecast);
}

void check_catalog(const std::string &label, const fs::path &binary,
                   bool require_lattice_checkpoint_closure) {
  const std::string output = read_command_stdout(
      binary.string() + " --global-config /cuwacunu/src/config/.config "
                        "--list-tools-json");
  const auto tools = extract_tool_schemas(output);
  assert(!tools.empty());
  bool saw_lattice_status = false;
  int lattice_inspect_tool_count = 0;
  int lattice_evaluate_tool_count = 0;
  bool saw_lattice_compare = false;
  int lattice_public_tool_count = 0;
  bool saw_config_status = false;
  int config_inspect_tool_count = 0;
  int config_apply_tool_count = 0;
  bool saw_runtime_status = false;
  bool saw_runtime_inspect_schema = false;
  bool saw_runtime_inspect_wave = false;
  bool saw_runtime_inspect_jobs = false;
  bool saw_runtime_inspect_job = false;
  bool saw_runtime_inspect_artifact = false;
  int runtime_inspect_artifact_tool_count = 0;
  int runtime_inspect_tool_count = 0;
  bool saw_runtime_run = false;
  bool saw_runtime_reset = false;
  bool saw_environment_status = false;
  bool saw_environment_inspect_schema = false;
  bool saw_environment_inspect_job = false;
  int environment_inspect_tool_count = 0;
  int environment_certify_tool_count = 0;
  bool saw_environment_certify_policy_acceptance = false;
  bool saw_environment_certify_paper_online_readiness = false;
  bool saw_environment_certify_paper_online_session_admission = false;
  bool saw_environment_paper_online_session = false;
  bool saw_environment_rollout = false;
  bool saw_marshal_status = false;
  int marshal_prepare_tool_count = 0;
  bool saw_marshal_rollout = false;
  bool saw_marshal_paper_online_session_handoff = false;
  int marshal_inspect_tool_count = 0;
  int marshal_inspect_run_tool_count = 0;
  bool saw_marshal_inspect_target = false;
  bool saw_marshal_inspect_protocol = false;
  int marshal_inspect_protocol_tool_count = 0;
  int marshal_inspect_spawn_tool_count = 0;
  bool saw_marshal_inspect_component = false;
  int marshal_inspect_facts_tool_count = 0;
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
        for (const auto retired_field :
             {"\"config_path\"", "\"runtime_root\""}) {
          if (tool.input_schema.find(retired_field) != std::string::npos) {
            throw std::runtime_error(
                "Lattice status schema still exposes request field: " +
                std::string(retired_field));
          }
        }
      } else if (tool.name.rfind("hero.lattice.inspect.", 0) == 0) {
        ++lattice_inspect_tool_count;
        for (const auto retired_field :
             {"\"subject\"", "\"args_path\"", "\"args_digest\""}) {
          if (tool.input_schema.find(retired_field) != std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect split schema still exposes router field: " +
                std::string(retired_field));
          }
        }
        if (tool.name == "hero.lattice.inspect.schema") {
          if (tool.input_schema.find("\"properties\":{}") ==
              std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect.schema must expose no arguments");
          }
        } else if (tool.name == "hero.lattice.inspect.targets") {
          if (tool.input_schema.find("\"config_path\"") == std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect.targets must expose config_path");
          }
        } else if (tool.name == "hero.lattice.inspect.target") {
          for (const auto retained_field :
               {"\"target_id\"", "\"config_path\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.target is missing field: " +
                  std::string(retained_field));
            }
          }
        } else if (tool.name == "hero.lattice.inspect.exposure") {
          for (const auto retained_field : {"\"runtime_root\"", "\"limit\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.exposure is missing field: " +
                  std::string(retained_field));
            }
          }
        } else if (tool.name == "hero.lattice.inspect.fact_families") {
          if (tool.input_schema.find("\"runtime_root\"") == std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect.fact_families must expose runtime_root");
          }
        } else if (tool.name == "hero.lattice.inspect.facts") {
          throw std::runtime_error(
              "Lattice catalog still exposes broad hero.lattice.inspect.facts");
        } else if (tool.name.rfind("hero.lattice.inspect.facts.", 0) == 0) {
          for (const auto retained_field : {"\"runtime_root\"", "\"family\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.facts split is missing field: " +
                  std::string(retained_field));
            }
          }
          if (tool.input_schema.find("\"mode\"") != std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect.facts split still exposes mode");
          }
          if (tool.name == "hero.lattice.inspect.facts.summary") {
            for (const auto rejected_field :
                 {"\"limit\"", "\"include_facts\"", "\"fact_digest\"",
                  "\"fact_digest_prefix\"", "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.summary exposes wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name == "hero.lattice.inspect.facts.scan") {
            for (const auto retained_field :
                 {"\"limit\"", "\"include_facts\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.scan is missing field: " +
                    std::string(retained_field));
              }
            }
            for (const auto rejected_field :
                 {"\"fact_digest\"", "\"fact_digest_prefix\"",
                  "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.scan exposes wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name == "hero.lattice.inspect.facts.lineage") {
            if (tool.input_schema.find("\"limit\"") == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.facts.lineage is missing limit");
            }
            for (const auto rejected_field :
                 {"\"include_facts\"", "\"fact_digest\"",
                  "\"fact_digest_prefix\"", "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.lineage exposes wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name == "hero.lattice.inspect.facts.preview") {
            if (tool.input_schema.find("\"limit\"") == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.facts.preview is missing limit");
            }
            for (const auto rejected_field :
                 {"\"include_facts\"", "\"fact_digest\"",
                  "\"fact_digest_prefix\"", "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview exposes wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name ==
                     "hero.lattice.inspect.facts.preview.by_digest") {
            for (const auto retained_field : {"\"limit\"", "\"fact_digest\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_digest is missing "
                    "field: " +
                    std::string(retained_field));
              }
            }
            for (const auto rejected_field :
                 {"\"include_facts\"", "\"fact_digest_prefix\"",
                  "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_digest exposes wrong "
                    "field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name ==
                     "hero.lattice.inspect.facts.preview.by_digest_prefix") {
            for (const auto retained_field :
                 {"\"limit\"", "\"fact_digest_prefix\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_digest_prefix is missing "
                    "field: " +
                    std::string(retained_field));
              }
            }
            for (const auto rejected_field :
                 {"\"include_facts\"", "\"fact_digest\"", "\"fact_index\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_digest_prefix exposes "
                    "wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else if (tool.name ==
                     "hero.lattice.inspect.facts.preview.by_index") {
            for (const auto retained_field : {"\"limit\"", "\"fact_index\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_index is missing "
                    "field: " +
                    std::string(retained_field));
              }
            }
            for (const auto rejected_field :
                 {"\"include_facts\"", "\"fact_digest\"",
                  "\"fact_digest_prefix\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice inspect.facts.preview.by_index exposes wrong "
                    "field: " +
                    std::string(rejected_field));
              }
            }
          } else {
            throw std::runtime_error(
                "Lattice catalog exposes unknown facts split tool: " +
                tool.name);
          }
        } else if (tool.name == "hero.lattice.inspect.index") {
          throw std::runtime_error(
              "Lattice catalog still exposes broad hero.lattice.inspect.index");
        } else if (tool.name == "hero.lattice.inspect.index.status") {
          for (const auto retained_field :
               {"\"runtime_root\"", "\"index_path\"", "\"limit\"",
                "\"validation_strength\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.status is missing field: " +
                  std::string(retained_field));
            }
          }
          for (const auto rejected_field :
               {"\"mode\"", "\"relation\"", "\"key\"", "\"key_contains\"",
                "\"digest\"", "\"digest_prefix\"", "\"compare_live_scan\"",
                "\"allow_unproven_cache\""}) {
            if (tool.input_schema.find(rejected_field) != std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.status still exposes query field: " +
                  std::string(rejected_field));
            }
          }
        } else if (tool.name.rfind("hero.lattice.inspect.index.query", 0) ==
                   0) {
          for (const auto retained_field :
               {"\"runtime_root\"", "\"index_path\"", "\"limit\"",
                "\"validation_strength\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.query is missing field: " +
                  std::string(retained_field));
            }
          }
          for (const auto rejected_field : {"\"mode\"", "\"compare_live_scan\"",
                                            "\"allow_unproven_cache\""}) {
            if (tool.input_schema.find(rejected_field) != std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.query split exposes wrong field: " +
                  std::string(rejected_field));
            }
          }
          const auto require_index_query_field = [&](const std::string &field) {
            if (tool.input_schema.find("\"" + field + "\"") ==
                std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.query selector tool is missing "
                  "field: " +
                  field);
            }
          };
          const auto reject_index_query_field = [&](const std::string &field) {
            if (tool.input_schema.find("\"" + field + "\"") !=
                std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.index.query selector tool exposes wrong "
                  "field: " +
                  field);
            }
          };
          if (tool.name == "hero.lattice.inspect.index.query" ||
              tool.name == "hero.lattice.inspect.index.query.unproven_cache") {
            for (const auto field : {"relation", "key", "key_contains",
                                     "digest", "digest_prefix"}) {
              reject_index_query_field(field);
            }
          } else if (tool.name.ends_with(".by_relation")) {
            require_index_query_field("relation");
            for (const auto field :
                 {"key", "key_contains", "digest", "digest_prefix"}) {
              reject_index_query_field(field);
            }
          } else if (tool.name.ends_with(".by_key")) {
            require_index_query_field("key");
            for (const auto field :
                 {"relation", "key_contains", "digest", "digest_prefix"}) {
              reject_index_query_field(field);
            }
          } else if (tool.name.ends_with(".by_key_contains")) {
            require_index_query_field("key_contains");
            for (const auto field :
                 {"relation", "key", "digest", "digest_prefix"}) {
              reject_index_query_field(field);
            }
          } else if (tool.name.ends_with(".by_digest")) {
            require_index_query_field("digest");
            for (const auto field :
                 {"relation", "key", "key_contains", "digest_prefix"}) {
              reject_index_query_field(field);
            }
          } else if (tool.name.ends_with(".by_digest_prefix")) {
            require_index_query_field("digest_prefix");
            for (const auto field :
                 {"relation", "key", "key_contains", "digest"}) {
              reject_index_query_field(field);
            }
          } else {
            throw std::runtime_error(
                "Lattice catalog exposes unknown index query split tool: " +
                tool.name);
          }
        } else if (tool.name == "hero.lattice.inspect.derived") {
          throw std::runtime_error("Lattice catalog still exposes broad "
                                   "hero.lattice.inspect.derived");
        } else if (tool.name.rfind("hero.lattice.inspect.derived.", 0) == 0) {
          if (tool.input_schema.find("\"relation\"") != std::string::npos) {
            throw std::runtime_error(
                "Lattice inspect.derived split still exposes relation");
          }
          if (tool.name == "hero.lattice.inspect.derived.target_satisfied" ||
              tool.name == "hero.lattice.inspect.derived.forbidden_overlap" ||
              tool.name ==
                  "hero.lattice.inspect.derived.unresolved_lineage.target") {
            for (const auto retained_field :
                 {"\"target_id\"", "\"runtime_root\"", "\"config_path\"",
                  "\"limit\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice target-derived split is missing field: " +
                    std::string(retained_field));
              }
            }
            for (const auto field :
                 {"protocol_id", "protocol_contract_fingerprint",
                  "graph_order_fingerprint", "source_cursor_token",
                  "vicreg_assembly_fingerprint",
                  "mtf_jepa_mae_vicreg_assembly_fingerprint",
                  "mdn_assembly_fingerprint"}) {
              if (tool.input_schema.find("\"" + std::string(field) + "\"") !=
                  std::string::npos) {
                throw std::runtime_error("Lattice target-derived split still "
                                         "exposes identity filter: " +
                                         std::string(field));
              }
            }
          } else if (tool.name == "hero.lattice.inspect.derived.stale_cache") {
            for (const auto retained_field :
                 {"\"runtime_root\"", "\"index_path\"",
                  "\"validation_strength\"", "\"limit\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice stale-cache split is missing field: " +
                    std::string(retained_field));
              }
            }
            if (tool.input_schema.find("\"target_id\"") != std::string::npos) {
              throw std::runtime_error(
                  "Lattice stale-cache split exposes target_id");
            }
          } else if (tool.name == "hero.lattice.inspect.derived."
                                  "checkpoint_ancestor") {
            throw std::runtime_error("Lattice catalog still exposes broad "
                                     "checkpoint_ancestor split");
          } else if (tool.name.rfind("hero.lattice.inspect.derived."
                                     "checkpoint_ancestor.",
                                     0) == 0) {
            for (const auto retained_field :
                 {"\"runtime_root\"", "\"limit\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice checkpoint-ancestor split is missing field: " +
                    std::string(retained_field));
              }
            }
            const auto require_checkpoint_field =
                [&](const std::string &field) {
                  if (tool.input_schema.find("\"" + field + "\"") ==
                      std::string::npos) {
                    throw std::runtime_error(
                        "Lattice checkpoint-ancestor split is missing field: " +
                        field);
                  }
                };
            const auto reject_checkpoint_field = [&](const std::string &field) {
              if (tool.input_schema.find("\"" + field + "\"") !=
                  std::string::npos) {
                throw std::runtime_error(
                    "Lattice checkpoint-ancestor split exposes wrong "
                    "field: " +
                    field);
              }
            };
            const bool by_path =
                tool.name.find(".by_checkpoint_path") != std::string::npos;
            const bool by_identity =
                tool.name.find(".by_checkpoint_identity") != std::string::npos;
            const bool with_ancestor_path =
                tool.name.ends_with(".with_ancestor_path");
            const bool with_ancestor_id =
                tool.name.ends_with(".with_ancestor_id");
            if (by_path == by_identity) {
              throw std::runtime_error(
                  "Lattice checkpoint-ancestor split must choose path or "
                  "identity: " +
                  tool.name);
            }
            if (by_path) {
              require_checkpoint_field("checkpoint_path");
              reject_checkpoint_field("checkpoint_id");
              reject_checkpoint_field("checkpoint_file_digest");
            } else {
              require_checkpoint_field("checkpoint_id");
              require_checkpoint_field("checkpoint_file_digest");
              reject_checkpoint_field("checkpoint_path");
            }
            if (with_ancestor_path && with_ancestor_id) {
              throw std::runtime_error(
                  "Lattice checkpoint-ancestor split mixes ancestor selectors");
            }
            if (with_ancestor_path) {
              require_checkpoint_field("ancestor_checkpoint_path");
              reject_checkpoint_field("ancestor_checkpoint_id");
            } else if (with_ancestor_id) {
              require_checkpoint_field("ancestor_checkpoint_id");
              reject_checkpoint_field("ancestor_checkpoint_path");
            } else {
              reject_checkpoint_field("ancestor_checkpoint_path");
              reject_checkpoint_field("ancestor_checkpoint_id");
            }
          } else if (tool.name == "hero.lattice.inspect.derived.unresolved_"
                                  "lineage.checkpoint") {
            for (const auto retained_field :
                 {"\"runtime_root\"", "\"limit\"", "\"checkpoint_path\"",
                  "\"checkpoint_id\"", "\"checkpoint_file_digest\""}) {
              if (tool.input_schema.find(retained_field) == std::string::npos) {
                throw std::runtime_error(
                    "Lattice checkpoint-lineage split is missing field: " +
                    std::string(retained_field));
              }
            }
            for (const auto rejected_field :
                 {"\"target_id\"", "\"ancestor_checkpoint_path\"",
                  "\"ancestor_checkpoint_id\""}) {
              if (tool.input_schema.find(rejected_field) != std::string::npos) {
                throw std::runtime_error(
                    "Lattice checkpoint-lineage split exposes wrong field: " +
                    std::string(rejected_field));
              }
            }
          } else {
            throw std::runtime_error(
                "Lattice catalog exposes unknown derived split tool: " +
                tool.name);
          }
        } else if (tool.name == "hero.lattice.inspect.checkpoint") {
          for (const auto retained_field :
               {"\"runtime_root\"", "\"checkpoint_path\"", "\"checkpoint_id\"",
                "\"checkpoint_file_digest\"", "\"limit\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice inspect.checkpoint is missing field: " +
                  std::string(retained_field));
            }
          }
        } else {
          throw std::runtime_error(
              "Lattice catalog exposes unknown inspect split tool: " +
              tool.name);
        }
      } else if (tool.name.rfind("hero.lattice.evaluate.", 0) == 0) {
        ++lattice_evaluate_tool_count;
        for (const auto retired_field :
             {"\"subject\"", "\"args_path\"", "\"args_digest\""}) {
          if (tool.input_schema.find(retired_field) != std::string::npos) {
            throw std::runtime_error(
                "Lattice evaluate split schema still exposes router field: " +
                std::string(retired_field));
          }
        }
        if (tool.name == "hero.lattice.evaluate.target" ||
            tool.name == "hero.lattice.evaluate.deficit") {
          for (const auto retained_field :
               {"\"target_id\"", "\"config_path\"", "\"runtime_root\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice evaluate target/deficit is missing field: " +
                  std::string(retained_field));
            }
          }
          for (const auto field :
               {"protocol_id", "protocol_contract_fingerprint",
                "graph_order_fingerprint", "source_cursor_token",
                "vicreg_assembly_fingerprint",
                "mtf_jepa_mae_vicreg_assembly_fingerprint",
                "mdn_assembly_fingerprint"}) {
            if (tool.input_schema.find("\"" + std::string(field) + "\"") !=
                std::string::npos) {
              throw std::runtime_error("Lattice evaluate target/deficit still "
                                       "exposes identity filter: " +
                                       std::string(field));
            }
          }
        } else if (tool.name == "hero.lattice.evaluate.targets") {
          for (const auto retained_field : {"\"target_ids\"", "\"config_path\"",
                                            "\"runtime_root\"", "\"limit\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice evaluate.targets is missing field: " +
                  std::string(retained_field));
            }
          }
          for (const auto field :
               {"protocol_id", "protocol_contract_fingerprint",
                "graph_order_fingerprint", "source_cursor_token",
                "vicreg_assembly_fingerprint",
                "mtf_jepa_mae_vicreg_assembly_fingerprint",
                "mdn_assembly_fingerprint"}) {
            if (tool.input_schema.find("\"" + std::string(field) + "\"") !=
                std::string::npos) {
              throw std::runtime_error("Lattice evaluate.targets still exposes "
                                       "identity filter: " +
                                       std::string(field));
            }
          }
        } else if (
            tool.name ==
                "hero.lattice.evaluate.latest_satisfying_checkpoint.target" ||
            tool.name ==
                "hero.lattice.evaluate.latest_satisfying_checkpoint.hint") {
          const char *selector = tool.name == "hero.lattice.evaluate.latest_"
                                              "satisfying_checkpoint.target"
                                     ? "\"target_id\""
                                     : "\"symbolic_hint\"";
          const char *rejected_selector =
              tool.name == "hero.lattice.evaluate.latest_satisfying_checkpoint."
                           "target"
                  ? "\"symbolic_hint\""
                  : "\"target_id\"";
          for (const auto retained_field :
               {selector, "\"config_path\"", "\"runtime_root\""}) {
            if (tool.input_schema.find(retained_field) == std::string::npos) {
              throw std::runtime_error(
                  "Lattice evaluate.latest_satisfying_checkpoint split is "
                  "missing "
                  "field: " +
                  std::string(retained_field));
            }
          }
          for (const auto field :
               {"protocol_id", "protocol_contract_fingerprint",
                "graph_order_fingerprint", "source_cursor_token",
                "vicreg_assembly_fingerprint",
                "mtf_jepa_mae_vicreg_assembly_fingerprint",
                "mdn_assembly_fingerprint"}) {
            if (tool.input_schema.find("\"" + std::string(field) + "\"") !=
                std::string::npos) {
              throw std::runtime_error("Lattice latest_satisfying_checkpoint "
                                       "split still exposes identity filter: " +
                                       std::string(field));
            }
          }
          if (tool.input_schema.find(rejected_selector) != std::string::npos) {
            throw std::runtime_error(
                "Lattice latest_satisfying_checkpoint split exposes wrong "
                "selector: " +
                std::string(rejected_selector));
          }
        } else {
          throw std::runtime_error(
              "Lattice catalog exposes unknown evaluate split tool: " +
              tool.name);
        }
      } else if (tool.name == "hero.lattice.compare") {
        saw_lattice_compare = true;
        for (const auto retained_field :
             {"\"left_target_id\"", "\"right_target_id\"", "\"config_path\"",
              "\"runtime_root\""}) {
          if (tool.input_schema.find(retained_field) == std::string::npos) {
            throw std::runtime_error(
                "Lattice compare schema is missing retained direct field: " +
                std::string(retained_field));
          }
        }
        for (const auto retired_field :
             {"\"args_path\"", "\"args_digest\"", "\"protocol_id\"",
              "\"protocol_contract_fingerprint\"",
              "\"graph_order_fingerprint\"", "\"source_cursor_token\"",
              "\"vicreg_assembly_fingerprint\"",
              "\"mtf_jepa_mae_vicreg_assembly_"
              "fingerprint\"",
              "\"mdn_assembly_fingerprint\""}) {
          if (tool.input_schema.find(retired_field) != std::string::npos) {
            throw std::runtime_error(
                "Lattice compare schema still exposes retired field: " +
                std::string(retired_field));
          }
        }
      } else {
        throw std::runtime_error(
            "Lattice catalog still exposes retired tool: " + tool.name);
      }
    }
    if (tool.name == "hero.config.status") {
      saw_config_status = true;
    }
    if (tool.name == "hero.config.inspect") {
      throw std::runtime_error(
          "Config catalog still exposes broad inspect router");
    }
    if (tool.name.rfind("hero.config.inspect.", 0) == 0) {
      ++config_inspect_tool_count;
      const auto has_property = [&](const std::string &field) {
        const std::string property = "\"" + field + "\":{";
        return tool.input_schema.find(property) != std::string::npos;
      };
      const auto require_property = [&](const std::string &field) {
        if (!has_property(field)) {
          throw std::runtime_error("Config inspect split tool " + tool.name +
                                   " is missing property: " + field);
        }
      };
      const auto reject_property = [&](const std::string &field) {
        if (has_property(field)) {
          throw std::runtime_error("Config inspect split tool " + tool.name +
                                   " still exposes wrong property: " + field);
        }
      };
      for (const auto retired_public :
           {"subject", "args_path", "args_digest", "include_machine_payload"}) {
        reject_property(retired_public);
      }
      if (tool.name == "hero.config.inspect.schema" ||
          tool.name == "hero.config.inspect.show" ||
          tool.name == "hero.config.inspect.validate_global_config" ||
          tool.name == "hero.config.inspect.backups") {
        for (const auto field :
             {"key", "path", "config_path", "include_content", "include_sha256",
              "for_write", "recursive", "limit"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.inspect.value") {
        require_property("key");
        reject_property("value");
      } else if (tool.name == "hero.config.inspect.map") {
        require_property("include_sha256");
      } else if (tool.name == "hero.config.inspect.bundle") {
        require_property("config_path");
        require_property("include_content");
        reject_property("include_text");
      } else if (tool.name == "hero.config.inspect.resolve_path") {
        throw std::runtime_error(
            "Config catalog still exposes broad inspect.resolve_path");
      } else if (tool.name == "hero.config.inspect.resolve_path.read" ||
                 tool.name == "hero.config.inspect.resolve_path.write") {
        require_property("path");
        require_property("include_sha256");
        reject_property("for_write");
      } else if (tool.name == "hero.config.inspect.diff") {
        require_property("include_content");
        reject_property("include_text");
      } else if (tool.name == "hero.config.inspect.file_list") {
        require_property("path");
        require_property("recursive");
        require_property("include_sha256");
        require_property("limit");
      } else if (tool.name == "hero.config.inspect.file_read") {
        require_property("path");
      } else {
        throw std::runtime_error(
            "Config catalog exposes unexpected inspect split tool: " +
            tool.name);
      }
    }
    if (tool.name == "hero.config.apply") {
      throw std::runtime_error(
          "Config catalog still exposes broad apply router");
    }
    if (tool.name.rfind("hero.config.apply.", 0) == 0) {
      ++config_apply_tool_count;
      const bool expected_apply_name =
          tool.name == "hero.config.apply.set" ||
          tool.name == "hero.config.apply.save" ||
          tool.name == "hero.config.apply.reload" ||
          tool.name == "hero.config.apply.rollback" ||
          tool.name == "hero.config.apply.write" ||
          tool.name == "hero.config.apply.delete";
      if (!expected_apply_name) {
        throw std::runtime_error(
            "Config catalog exposes unexpected apply split tool: " + tool.name);
      }
      const auto has_property = [&](const std::string &field) {
        const std::string property = "\"" + field + "\":{";
        return tool.input_schema.find(property) != std::string::npos;
      };
      const auto require_property = [&](const std::string &field) {
        if (!has_property(field)) {
          throw std::runtime_error("Config apply split tool " + tool.name +
                                   " is missing property: " + field);
        }
      };
      const auto reject_property = [&](const std::string &field) {
        if (has_property(field)) {
          throw std::runtime_error("Config apply split tool " + tool.name +
                                   " still exposes wrong property: " + field);
        }
      };
      for (const auto retired_field :
           {"subject", "mode", "args_path", "args_digest", "create_backup"}) {
        reject_property(retired_field);
      }
      if (tool.name == "hero.config.apply.set") {
        require_property("key");
        require_property("value");
        require_property("reason");
        for (const auto field :
             {"path", "content", "backup_id", "expected_sha256",
              "expected_current_sha256", "include_content"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.apply.save") {
        require_property("reason");
        require_property("include_content");
        for (const auto field :
             {"key", "value", "path", "content", "backup_id", "expected_sha256",
              "expected_current_sha256"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.apply.reload") {
        require_property("reason");
        for (const auto field :
             {"key", "value", "path", "content", "backup_id", "expected_sha256",
              "expected_current_sha256", "include_content"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.apply.rollback") {
        require_property("backup_id");
        require_property("reason");
        for (const auto field :
             {"key", "value", "path", "content", "expected_sha256",
              "expected_current_sha256", "include_content"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.apply.write") {
        require_property("path");
        require_property("content");
        require_property("expected_sha256");
        require_property("reason");
        for (const auto field :
             {"key", "value", "backup_id", "expected_current_sha256",
              "include_content"}) {
          reject_property(field);
        }
      } else if (tool.name == "hero.config.apply.delete") {
        require_property("path");
        require_property("expected_sha256");
        require_property("reason");
        for (const auto field :
             {"key", "value", "content", "backup_id", "expected_current_sha256",
              "include_content"}) {
          reject_property(field);
        }
      }
    }
    const auto runtime_has_property = [&](const std::string &field) {
      const std::string property = "\"" + field + "\":{";
      return tool.input_schema.find(property) != std::string::npos;
    };
    const auto runtime_require_property = [&](const std::string &field) {
      if (!runtime_has_property(field)) {
        throw std::runtime_error("Runtime tool " + tool.name +
                                 " is missing property: " + field);
      }
    };
    const auto runtime_reject_property = [&](const std::string &field) {
      if (runtime_has_property(field)) {
        throw std::runtime_error("Runtime tool " + tool.name +
                                 " still exposes wrong property: " + field);
      }
    };
    if (tool.name == "hero.runtime.status") {
      saw_runtime_status = true;
    }
    if (tool.name == "hero.runtime.inspect") {
      throw std::runtime_error(
          "Runtime catalog still exposes broad hero.runtime.inspect");
    }
    if (tool.name.rfind("hero.runtime.inspect.", 0) == 0) {
      ++runtime_inspect_tool_count;
    }
    if (tool.name == "hero.runtime.inspect.schema") {
      saw_runtime_inspect_schema = true;
      for (const auto retired_field :
           {"\"subject\"", "\"args_path\"", "\"args_digest\"",
            "\"config_path\"", "\"root\"", "\"job_id\"", "\"job_dir\"",
            "\"artifact\"", "\"path\"", "\"include_artifacts\"",
            "\"include_text\"", "\"max_bytes\"", "\"limit\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Runtime inspect.schema still exposes retired field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name == "hero.runtime.inspect.wave") {
      saw_runtime_inspect_wave = true;
      runtime_require_property("config_path");
      for (const auto field :
           {"subject", "args_path", "args_digest", "root", "job_id", "job_dir",
            "artifact", "path", "include_artifacts", "include_text",
            "max_bytes", "limit"}) {
        runtime_reject_property(field);
      }
    }
    if (tool.name == "hero.runtime.inspect.jobs") {
      saw_runtime_inspect_jobs = true;
      runtime_require_property("root");
      runtime_require_property("limit");
      runtime_require_property("include_artifacts");
      for (const auto field :
           {"subject", "args_path", "args_digest", "config_path", "job_id",
            "job_dir", "artifact", "path", "include_text", "max_bytes"}) {
        runtime_reject_property(field);
      }
    }
    if (tool.name == "hero.runtime.inspect.job") {
      saw_runtime_inspect_job = true;
      runtime_require_property("job_id");
      runtime_require_property("job_dir");
      runtime_require_property("include_text");
      runtime_require_property("max_bytes");
      for (const auto field :
           {"subject", "args_path", "args_digest", "config_path", "root",
            "artifact", "path", "include_artifacts", "limit"}) {
        runtime_reject_property(field);
      }
    }
    if (tool.name == "hero.runtime.inspect.artifact") {
      throw std::runtime_error(
          "Runtime catalog still exposes broad inspect.artifact");
    }
    if (tool.name.rfind("hero.runtime.inspect.artifact.", 0) == 0) {
      saw_runtime_inspect_artifact = true;
      ++runtime_inspect_artifact_tool_count;
      runtime_require_property("max_bytes");
      if (tool.name == "hero.runtime.inspect.artifact.job") {
        runtime_require_property("job_id");
        runtime_require_property("job_dir");
        runtime_require_property("artifact");
        runtime_reject_property("path");
      } else if (tool.name == "hero.runtime.inspect.artifact.path") {
        runtime_require_property("path");
        runtime_reject_property("job_id");
        runtime_reject_property("job_dir");
        runtime_reject_property("artifact");
      } else {
        throw std::runtime_error(
            "Runtime catalog exposes unknown artifact split: " + tool.name);
      }
      for (const auto field :
           {"subject", "args_path", "args_digest", "config_path", "root",
            "include_artifacts", "include_text", "limit"}) {
        runtime_reject_property(field);
      }
    }
    if (tool.name == "hero.runtime.run") {
      saw_runtime_run = true;
      for (const auto retired_field :
           {"\"args_path\"", "\"args_digest\"", "\"config_path\"",
            "\"runtime_handoff\"", "\"execution_request_path\"",
            "\"execution_request_digest\"", "\"policy_id\"", "\"policy_kind\"",
            "\"job_dir\"", "\"wave_overlay\"", "\"marshal_expected_wave\"",
            "\"timeout_seconds\"", "\"confirm_execute\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Runtime run schema still exposes retired payload field: " +
              std::string(retired_field));
        }
      }
      for (const auto retained_field :
           {"\"mode\"", "\"runtime_handoff_path\"",
            "\"runtime_handoff_digest\"", "\"contract_path\"",
            "\"contract_digest\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Runtime run schema is missing retained artifact field: " +
              std::string(retained_field));
        }
      }
      if (tool.input_schema.find("\"subject\"") != std::string::npos) {
        throw std::runtime_error(
            "Runtime run schema still exposes retired subject field");
      }
    }
    if (tool.name == "hero.runtime.reset") {
      saw_runtime_reset = true;
      for (const auto retained_field :
           {"\"mode\"", "\"runtime_root\"", "\"backup\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Runtime reset schema is missing direct field: " +
              std::string(retained_field));
        }
      }
      for (const auto retired_field :
           {"\"args_path\"", "\"args_digest\"", "\"confirm_dev_nuke\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Runtime reset schema still exposes retired field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name == "hero.environment.status") {
      saw_environment_status = true;
    }
    if (tool.name == "hero.environment.inspect") {
      throw std::runtime_error(
          "Environment catalog still exposes broad hero.environment.inspect");
    }
    if (tool.name.rfind("hero.environment.inspect.", 0) == 0) {
      ++environment_inspect_tool_count;
    }
    if (tool.name == "hero.environment.inspect.schema") {
      saw_environment_inspect_schema = true;
      for (const auto retired_field :
           {"\"subject\"", "\"args_path\"", "\"args_digest\"", "\"job_dir\"",
            "\"include_text\"", "\"max_bytes\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Environment inspect.schema still exposes retired field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name == "hero.environment.inspect.job") {
      saw_environment_inspect_job = true;
      for (const auto retained_field :
           {"\"job_dir\"", "\"include_text\"", "\"max_bytes\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Environment inspect.job schema is missing direct field: " +
              std::string(retained_field));
        }
      }
      for (const auto retired_field :
           {"\"subject\"", "\"args_path\"", "\"args_digest\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Environment inspect.job still exposes retired request field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name == "hero.environment.certify") {
      throw std::runtime_error(
          "Environment catalog still exposes broad hero.environment.certify");
    }
    if (tool.name.rfind("hero.environment.certify.", 0) == 0) {
      ++environment_certify_tool_count;
      if (tool.name == "hero.environment.certify.policy_acceptance") {
        saw_environment_certify_policy_acceptance = true;
      }
      if (tool.name == "hero.environment.certify.paper_online_readiness") {
        saw_environment_certify_paper_online_readiness = true;
      }
      if (tool.name ==
          "hero.environment.certify.paper_online_session_admission") {
        saw_environment_certify_paper_online_session_admission = true;
      }
      for (const auto retired_field :
           {"\"config_path\"", "\"job_id\"", "\"job_dir\"", "\"args_path\"",
            "\"args_digest\"", "\"confirm_issue\"", "\"primary_metric_value\"",
            "\"policy_acceptance_decision\"", "\"paper_online_profile_digest\"",
            "\"kill_switch_bound\"", "\"subject\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error("Environment certify schema still exposes "
                                   "retired payload field: " +
                                   std::string(retired_field));
        }
      }
      std::vector<std::string> retained_fields{"\"mode\"",
                                               "\"expected_preview_digest\""};
      if (tool.name == "hero.environment.certify.policy_acceptance") {
        retained_fields.push_back("\"certification_evidence\"");
        retained_fields.push_back("\"policy_training_job_dir\"");
        retained_fields.push_back("\"tsodao_settings_protection_job_dir\"");
        retained_fields.push_back("\"acceptance_id\"");
      } else if (tool.name ==
                 "hero.environment.certify.paper_online_readiness") {
        retained_fields.push_back("\"certification_evidence\"");
        retained_fields.push_back("\"policy_acceptance_job_dir\"");
        retained_fields.push_back("\"readiness_id\"");
      } else if (tool.name ==
                 "hero.environment.certify.paper_online_session_admission") {
        retained_fields.push_back("\"admission_request\"");
        retained_fields.push_back("\"admission_request_path\"");
      } else {
        throw std::runtime_error("unexpected Environment certify tool: " +
                                 tool.name);
      }
      for (const auto &retained_field : retained_fields) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Environment certify schema is missing direct field: " +
              retained_field);
        }
      }
    }
    if (tool.name == "hero.environment.paper_online.session") {
      saw_environment_paper_online_session = true;
      for (const auto retired_field :
           {"\"subject\"", "\"args_path\"", "\"args_digest\"",
            "\"admission_request\"", "\"expected_preview_digest\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Environment paper_online.session schema still exposes retired "
              "or admission-only field: " +
              std::string(retired_field));
        }
      }
      for (const auto retained_field :
           {"\"mode\"", "\"session_request\"", "\"session_request_path\"",
            "\"include_machine_payload\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Environment paper_online.session schema is missing direct "
              "field: " +
              std::string(retained_field));
        }
      }
    }
    if (tool.name == "hero.environment.rollout") {
      saw_environment_rollout = true;
      for (const auto retired_field : {"\"args_path\"",
                                       "\"args_digest\"",
                                       "\"idempotency_key\"",
                                       "\"experiment_id\"",
                                       "\"config_path\"",
                                       "\"replay_batch_index_path\"",
                                       "\"runtime_exec_path\"",
                                       "\"report_path\"",
                                       "\"environment_mode\"",
                                       "\"environment_assembly_id\"",
                                       "\"graph_order_fingerprint\"",
                                       "\"asset_universe_digest\"",
                                       "\"accounting_numeraire_node_id\"",
                                       "\"policy_set\"",
                                       "\"max_steps\"",
                                       "\"max_parallel_jobs\"",
                                       "\"execution_profile\"",
                                       "\"timeout_seconds\"",
                                       "\"require_existing_runtime_job_dir\"",
                                       "\"require_completed_runtime_job\"",
                                       "\"require_replay_artifacts\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error("Environment rollout schema still exposes "
                                   "request payload field: " +
                                   std::string(retired_field));
        }
      }
      for (const auto retained_field :
           {"\"mode\"", "\"runtime_job_dir\"", "\"rollout_id\"",
            "\"rollout_attempt_id\"", "\"target_node_ids\"",
            "\"include_machine_payload\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Environment rollout schema is missing retained request field: " +
              std::string(retained_field));
        }
      }
    }
    if (tool.name == "hero.marshal.status") {
      saw_marshal_status = true;
      for (const auto retired_field :
           {"\"receipt_root\"", "\"limit\"", "\"receipts\"",
            "\"runtime_policy\"", "\"lattice_advice_surface_available\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Marshal status schema still exposes request field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name.rfind("hero.marshal.prepare.", 0) == 0) {
      ++marshal_prepare_tool_count;
      const bool prepare_facade_name =
          tool.name == "hero.marshal.prepare.train" ||
          tool.name == "hero.marshal.prepare.evaluate";
      if (!prepare_facade_name) {
        throw std::runtime_error("unexpected Marshal prepare facade tool: " +
                                 tool.name);
      }
      for (const auto retained_field :
           {"\"target_id\"", "\"mode\"", "\"profile\"",
            "\"include_machine_payload\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Marshal prepare schema is missing retained request field: " +
              std::string(retained_field));
        }
      }
      for (const auto retired_field : {"\"drive_mode\"",
                                       "\"intent\"",
                                       "\"config_path\"",
                                       "\"runtime_root\"",
                                       "\"driver_policy\"",
                                       "\"resume_ledger\"",
                                       "\"source_lattice_timestamp\"",
                                       "\"max_waves\"",
                                       "\"recommendation_attempt_count\"",
                                       "\"context\"",
                                       "\"protocol_contract_fingerprint\"",
                                       "\"graph_order_fingerprint\"",
                                       "\"source_cursor_token\"",
                                       "\"vicreg_assembly_fingerprint\"",
                                       "\"mdn_assembly_fingerprint\"",
                                       "\"materialize_plan_inputs\"",
                                       "\"include_runtime_dry_run\"",
                                       "\"args_path\"",
                                       "\"args_digest\"",
                                       "\"runtime_policy\"",
                                       "\"runtime_wave\"",
                                       "\"timeout_seconds\"",
                                       "\"ledger_created_at_utc\"",
                                       "\"ledger_nonce\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Marshal prepare schema still exposes request payload field: " +
              std::string(retired_field));
        }
      }
    }
    if (tool.name == "hero.marshal.rollout") {
      saw_marshal_rollout = true;
      for (const auto retired_field : {"\"args_path\"",
                                       "\"args_digest\"",
                                       "\"idempotency_key\"",
                                       "\"experiment_id\"",
                                       "\"config_path\"",
                                       "\"replay_batch_index_path\"",
                                       "\"runtime_exec_path\"",
                                       "\"report_path\"",
                                       "\"environment_mode\"",
                                       "\"environment_assembly_id\"",
                                       "\"graph_order_fingerprint\"",
                                       "\"asset_universe_digest\"",
                                       "\"accounting_numeraire_node_id\"",
                                       "\"policy_set\"",
                                       "\"max_steps\"",
                                       "\"max_parallel_jobs\"",
                                       "\"execution_profile\"",
                                       "\"timeout_seconds\"",
                                       "\"require_existing_runtime_job_dir\"",
                                       "\"require_completed_runtime_job\"",
                                       "\"require_replay_artifacts\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Marshal rollout schema still exposes request payload field: " +
              std::string(retired_field));
        }
      }
      for (const auto retained_field :
           {"\"mode\"", "\"runtime_job_dir\"", "\"rollout_id\"",
            "\"rollout_attempt_id\"", "\"target_node_ids\"", "\"profile\"",
            "\"include_machine_payload\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Marshal rollout schema is missing retained request field: " +
              std::string(retained_field));
        }
      }
    }
    if (tool.name == "hero.marshal.paper_online.session_handoff") {
      saw_marshal_paper_online_session_handoff = true;
      for (const auto retained_field :
           {"\"mode\"", "\"handoff_request\"", "\"handoff_request_path\"",
            "\"include_machine_payload\""}) {
        if (tool.input_schema.find(retained_field) == std::string::npos) {
          throw std::runtime_error(
              "Marshal paper_online.session_handoff schema is missing "
              "retained request field: " +
              std::string(retained_field));
        }
      }
      for (const auto retired_field :
           {"\"config_path\"", "\"runtime_root\"", "\"target_id\"",
            "\"admission_request\"", "\"session_request\"",
            "\"expected_preview_digest\""}) {
        if (tool.input_schema.find(retired_field) != std::string::npos) {
          throw std::runtime_error(
              "Marshal paper_online.session_handoff schema still exposes "
              "wrong request field: " +
              std::string(retired_field));
        }
      }
    }
    const auto marshal_has_property = [&](const std::string &field) {
      const std::string property = "\"" + field + "\":{";
      return tool.input_schema.find(property) != std::string::npos;
    };
    const auto marshal_require_property = [&](const std::string &field) {
      if (!marshal_has_property(field)) {
        throw std::runtime_error("Marshal tool " + tool.name +
                                 " is missing property: " + field);
      }
    };
    const auto marshal_reject_property = [&](const std::string &field) {
      if (marshal_has_property(field)) {
        throw std::runtime_error("Marshal tool " + tool.name +
                                 " still exposes wrong property: " + field);
      }
    };
    if (tool.name == "hero.marshal.inspect") {
      throw std::runtime_error(
          "Marshal catalog still exposes broad hero.marshal.inspect");
    }
    if (tool.name.rfind("hero.marshal.inspect.", 0) == 0) {
      ++marshal_inspect_tool_count;
      for (const auto field : {"subject", "args_path", "args_digest"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name == "hero.marshal.inspect.run" ||
        tool.name == "hero.marshal.inspect.facts" ||
        tool.name == "hero.marshal.inspect.protocol" ||
        tool.name == "hero.marshal.inspect.spawn") {
      throw std::runtime_error("Marshal catalog still exposes broad " +
                               tool.name);
    }
    if (tool.name.rfind("hero.marshal.inspect.run.", 0) == 0) {
      ++marshal_inspect_run_tool_count;
      marshal_require_property("runtime_root");
      marshal_require_property("config_path");
      marshal_require_property("include_machine_payload");
      marshal_reject_property("mode");
      if (tool.name == "hero.marshal.inspect.run.compare") {
        marshal_require_property("baseline_job_id");
        marshal_require_property("candidate_job_id");
        for (const auto field :
             {"job_id", "job_ids", "target_ids",
              "protocol_contract_fingerprint", "graph_order_fingerprint",
              "source_cursor_token", "vicreg_assembly_fingerprint",
              "mdn_assembly_fingerprint", "expected_identity"}) {
          marshal_reject_property(field);
        }
      } else {
        marshal_require_property("target_ids");
        if (tool.name == "hero.marshal.inspect.run.single_job") {
          marshal_require_property("job_id");
          marshal_reject_property("job_ids");
        } else {
          marshal_require_property("job_ids");
          marshal_reject_property("job_id");
        }
        for (const auto field : {"baseline_job_id", "candidate_job_id"}) {
          marshal_reject_property(field);
        }
        for (const auto field :
             {"protocol_contract_fingerprint", "graph_order_fingerprint",
              "source_cursor_token", "vicreg_assembly_fingerprint",
              "mdn_assembly_fingerprint", "expected_identity"}) {
          marshal_reject_property(field);
        }
      }
      for (const auto field :
           {"target_id", "identity_mode", "component_family_id", "spawn_id",
            "component_spawn_id", "component_spawn_registry_id",
            "component_spawn_label", "component_spawn_fingerprint",
            "fact_family_id", "limit", "include_facts", "include_lineage",
            "include_preview", "fact_digest", "fact_digest_prefix",
            "fact_index"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name == "hero.marshal.inspect.target") {
      saw_marshal_inspect_target = true;
      marshal_require_property("target_id");
      marshal_require_property("runtime_root");
      marshal_require_property("config_path");
      marshal_require_property("include_machine_payload");
      for (const auto field : {"mode",
                               "identity_mode",
                               "job_id",
                               "job_ids",
                               "target_ids",
                               "baseline_job_id",
                               "candidate_job_id",
                               "component_family_id",
                               "spawn_id",
                               "component_spawn_id",
                               "component_spawn_registry_id",
                               "component_spawn_label",
                               "component_spawn_fingerprint",
                               "fact_family_id",
                               "limit",
                               "include_facts",
                               "include_lineage",
                               "include_preview",
                               "fact_digest",
                               "fact_digest_prefix",
                               "fact_index",
                               "protocol_contract_fingerprint",
                               "graph_order_fingerprint",
                               "source_cursor_token",
                               "vicreg_assembly_fingerprint",
                               "mdn_assembly_fingerprint",
                               "expected_identity"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name.rfind("hero.marshal.inspect.protocol.", 0) == 0) {
      saw_marshal_inspect_protocol = true;
      ++marshal_inspect_protocol_tool_count;
      marshal_require_property("runtime_root");
      marshal_require_property("config_path");
      marshal_require_property("job_id");
      marshal_require_property("job_ids");
      marshal_require_property("include_machine_payload");
      marshal_reject_property("identity_mode");
      if (tool.name == "hero.marshal.inspect.protocol.strict") {
        marshal_require_property("expected_identity");
      } else if (tool.name == "hero.marshal.inspect.protocol.report") {
        for (const auto field :
             {"protocol_contract_fingerprint", "graph_order_fingerprint",
              "source_cursor_token", "vicreg_assembly_fingerprint",
              "mdn_assembly_fingerprint", "expected_identity"}) {
          marshal_reject_property(field);
        }
      } else {
        throw std::runtime_error(
            "Marshal catalog exposes unknown protocol split: " + tool.name);
      }
      for (const auto field :
           {"mode", "target_id", "target_ids", "baseline_job_id",
            "candidate_job_id", "component_family_id", "spawn_id",
            "component_spawn_id", "component_spawn_registry_id",
            "component_spawn_label", "component_spawn_fingerprint",
            "fact_family_id", "limit", "include_facts", "include_lineage",
            "include_preview", "fact_digest", "fact_digest_prefix",
            "fact_index"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name.rfind("hero.marshal.inspect.spawn.", 0) == 0) {
      ++marshal_inspect_spawn_tool_count;
      marshal_require_property("runtime_root");
      marshal_require_property("config_path");
      marshal_require_property("include_machine_payload");
      if (tool.name == "hero.marshal.inspect.spawn.by_id") {
        marshal_require_property("component_spawn_id");
        for (const auto field :
             {"job_id", "job_ids", "spawn_id", "component_spawn_registry_id",
              "component_spawn_label", "component_spawn_fingerprint"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.spawn.by_label") {
        marshal_require_property("component_spawn_label");
        for (const auto field :
             {"job_id", "job_ids", "spawn_id", "component_spawn_id",
              "component_spawn_registry_id", "component_spawn_fingerprint"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.spawn.by_registry") {
        marshal_require_property("component_spawn_registry_id");
        for (const auto field :
             {"job_id", "job_ids", "spawn_id", "component_spawn_id",
              "component_spawn_label", "component_spawn_fingerprint"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.spawn.by_fingerprint") {
        marshal_require_property("component_spawn_fingerprint");
        for (const auto field :
             {"job_id", "job_ids", "spawn_id", "component_spawn_id",
              "component_spawn_registry_id", "component_spawn_label"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.spawn.by_job") {
        marshal_require_property("job_id");
        for (const auto field :
             {"job_ids", "spawn_id", "component_spawn_id",
              "component_spawn_registry_id", "component_spawn_label",
              "component_spawn_fingerprint"}) {
          marshal_reject_property(field);
        }
      } else {
        throw std::runtime_error(
            "Marshal catalog exposes unknown spawn split: " + tool.name);
      }
      for (const auto field : {"mode",
                               "identity_mode",
                               "target_id",
                               "target_ids",
                               "baseline_job_id",
                               "candidate_job_id",
                               "component_family_id",
                               "fact_family_id",
                               "limit",
                               "include_facts",
                               "include_lineage",
                               "include_preview",
                               "fact_digest",
                               "fact_digest_prefix",
                               "fact_index",
                               "protocol_contract_fingerprint",
                               "graph_order_fingerprint",
                               "source_cursor_token",
                               "vicreg_assembly_fingerprint",
                               "mdn_assembly_fingerprint",
                               "expected_identity"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name == "hero.marshal.inspect.component") {
      saw_marshal_inspect_component = true;
      marshal_require_property("component_family_id");
      marshal_require_property("runtime_root");
      marshal_require_property("config_path");
      marshal_require_property("job_id");
      marshal_require_property("job_ids");
      marshal_require_property("include_machine_payload");
      for (const auto field : {"mode",
                               "identity_mode",
                               "target_id",
                               "target_ids",
                               "baseline_job_id",
                               "candidate_job_id",
                               "spawn_id",
                               "component_spawn_id",
                               "component_spawn_registry_id",
                               "component_spawn_label",
                               "component_spawn_fingerprint",
                               "fact_family_id",
                               "limit",
                               "include_facts",
                               "include_lineage",
                               "include_preview",
                               "fact_digest",
                               "fact_digest_prefix",
                               "fact_index",
                               "protocol_contract_fingerprint",
                               "graph_order_fingerprint",
                               "source_cursor_token",
                               "vicreg_assembly_fingerprint",
                               "mdn_assembly_fingerprint",
                               "expected_identity"}) {
        marshal_reject_property(field);
      }
    }
    if (tool.name.rfind("hero.marshal.inspect.facts.", 0) == 0) {
      ++marshal_inspect_facts_tool_count;
      for (const auto field :
           {"target_id", "fact_family_id", "config_path", "runtime_root",
            "limit", "include_machine_payload"}) {
        marshal_require_property(field);
      }
      marshal_reject_property("mode");
      if (tool.name == "hero.marshal.inspect.facts.summary") {
        marshal_require_property("include_facts");
        marshal_require_property("include_lineage");
        for (const auto field : {"include_preview", "fact_digest",
                                 "fact_digest_prefix", "fact_index"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.facts.lineage") {
        for (const auto field :
             {"include_facts", "include_lineage", "include_preview",
              "fact_digest", "fact_digest_prefix", "fact_index"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.facts.preview") {
        marshal_require_property("include_lineage");
        for (const auto field :
             {"include_facts", "include_preview", "fact_digest",
              "fact_digest_prefix", "fact_index"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.facts.preview.by_digest") {
        marshal_require_property("fact_digest");
        marshal_require_property("include_lineage");
        for (const auto field : {"include_facts", "include_preview",
                                 "fact_digest_prefix", "fact_index"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name ==
                 "hero.marshal.inspect.facts.preview.by_digest_prefix") {
        marshal_require_property("fact_digest_prefix");
        marshal_require_property("include_lineage");
        for (const auto field : {"include_facts", "include_preview",
                                 "fact_digest", "fact_index"}) {
          marshal_reject_property(field);
        }
      } else if (tool.name == "hero.marshal.inspect.facts.preview.by_index") {
        marshal_require_property("fact_index");
        marshal_require_property("include_lineage");
        for (const auto field : {"include_facts", "include_preview",
                                 "fact_digest", "fact_digest_prefix"}) {
          marshal_reject_property(field);
        }
      } else {
        throw std::runtime_error(
            "Marshal catalog exposes unknown facts split: " + tool.name);
      }
      for (const auto field :
           {"identity_mode", "job_id", "job_ids", "target_ids",
            "baseline_job_id", "candidate_job_id", "component_family_id",
            "spawn_id", "component_spawn_id", "component_spawn_registry_id",
            "component_spawn_label", "component_spawn_fingerprint",
            "protocol_contract_fingerprint", "graph_order_fingerprint",
            "source_cursor_token", "vicreg_assembly_fingerprint",
            "mdn_assembly_fingerprint", "expected_identity"}) {
        marshal_reject_property(field);
      }
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
            "hero.runtime.execute", "hero.runtime.replay", "hero.runtime.emit",
            "hero.runtime.run_wave", "hero.runtime.run_replay",
            "hero.runtime.run_policy_training", "hero.runtime.dev_nuke",
            "hero.runtime.list_jobs", "hero.runtime.get_job",
            "hero.runtime.read_artifact"}) {
        if (tool.name == retired_name) {
          throw std::runtime_error(
              "Runtime catalog still exposes retired tool: " + tool.name);
        }
      }
    }
    if (label == "Marshal") {
      for (const auto retired_name :
           {"hero.marshal.summarize_training_state",
            "hero.marshal.compare_runs", "hero.marshal.lookup_target_advice",
            "hero.marshal.prepare_target_dispatch",
            "hero.marshal.validate_advice", "hero.marshal.dry_run_dispatch",
            "hero.marshal.execution_gate", "hero.marshal.replay_receipt",
            "hero.marshal.batch_preview", "hero.marshal.evaluate_run",
            "hero.marshal.inspect", "hero.marshal.inspect_run",
            "hero.marshal.reach_lattice_target", "hero.marshal.evaluate",
            "hero.marshal.inspect_evidence_panel"}) {
        if (tool.name == retired_name) {
          throw std::runtime_error(
              "Marshal catalog still exposes retired tool: " + tool.name);
        }
      }
    }
  }
  if (label == "Config" &&
      (tools.size() != 19U || !saw_config_status ||
       config_inspect_tool_count != 12 || config_apply_tool_count != 6)) {
    throw std::runtime_error(
        "Config catalog must expose exactly hero.config.status, 12 "
        "hero.config.inspect.* tools, and 6 hero.config.apply.* tools");
  }
  if (label == "Runtime" &&
      (tools.size() != 9U || !saw_runtime_status ||
       !saw_runtime_inspect_schema || !saw_runtime_inspect_wave ||
       !saw_runtime_inspect_jobs || !saw_runtime_inspect_job ||
       !saw_runtime_inspect_artifact || runtime_inspect_tool_count != 6 ||
       runtime_inspect_artifact_tool_count != 2 || !saw_runtime_run ||
       !saw_runtime_reset)) {
    throw std::runtime_error(
        "Runtime catalog must expose exactly hero.runtime.status, six "
        "hero.runtime.inspect.* tools, hero.runtime.run, and "
        "hero.runtime.reset");
  }
  if (label == "Environment" &&
      (tools.size() != 8U || !saw_environment_status ||
       !saw_environment_inspect_schema || !saw_environment_inspect_job ||
       environment_inspect_tool_count != 2 ||
       environment_certify_tool_count != 3 ||
       !saw_environment_certify_policy_acceptance ||
       !saw_environment_certify_paper_online_readiness ||
       !saw_environment_certify_paper_online_session_admission ||
       !saw_environment_paper_online_session || !saw_environment_rollout)) {
    throw std::runtime_error(
        "Environment catalog must expose exactly hero.environment.status, "
        "hero.environment.inspect.schema, hero.environment.inspect.job, "
        "hero.environment.certify.*, hero.environment.paper_online.session, "
        "and hero.environment.rollout");
  }
  if (require_lattice_checkpoint_closure &&
      (!saw_lattice_status || lattice_inspect_tool_count != 37 ||
       lattice_evaluate_tool_count != 5 || !saw_lattice_compare ||
       lattice_public_tool_count != 44)) {
    throw std::runtime_error("Lattice catalog must expose status, thirty-seven "
                             "inspect subtools, five "
                             "evaluate subtools, and compare");
  }
  if (label == "Marshal" &&
      (tools.size() != 24U || !saw_marshal_status ||
       marshal_prepare_tool_count != 2 || !saw_marshal_rollout ||
       !saw_marshal_paper_online_session_handoff ||
       marshal_inspect_tool_count != 19 ||
       marshal_inspect_run_tool_count != 4 || !saw_marshal_inspect_target ||
       !saw_marshal_inspect_protocol ||
       marshal_inspect_protocol_tool_count != 2 ||
       marshal_inspect_spawn_tool_count != 5 ||
       !saw_marshal_inspect_component ||
       marshal_inspect_facts_tool_count != 6)) {
    throw std::runtime_error(
        "Marshal catalog must expose exactly hero.marshal.status, "
        "two hero.marshal.prepare.* tools, hero.marshal.rollout, "
        "hero.marshal.paper_online.session_handoff, and nineteen "
        "hero.marshal.inspect.* tools");
  }
}

void check_config_collapsed_surface(const fs::path &binary) {
  const std::string base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";

  const std::string schema_output =
      read_command_stdout(base + "--tool hero.config.inspect.schema "
                                 "--args-json '{}'");
  require_contains(schema_output, "\"keys\":[",
                   "Config inspect schema should return schema keys");

  const std::string value_output =
      read_command_stdout(base + "--tool hero.config.inspect.value "
                                 "--args-json '{\"key\":\"protocol_layer\"}'");
  require_contains(value_output, "\"key\":\"protocol_layer\"",
                   "Config inspect value should read policy values");
  require_contains(value_output, "\"value\":\"STDIO\"",
                   "Config inspect value should expose protocol_layer");

  const std::string validate_output =
      read_command_stdout(base + "--tool "
                                 "hero.config.inspect.validate_global_config "
                                 "--args-json '{}'");
  require_contains(validate_output, "\"valid\":true",
                   "Config inspect validate_global_config should validate "
                   "global config paths");

  const std::string map_output =
      read_command_stdout(base + "--tool hero.config.inspect.map --args-json "
                                 "'{\"include_sha256\":false}'");
  require_contains(map_output, "\"global_config_path\":",
                   "Config inspect map should expose global config paths");

  const std::string apply_output =
      read_command_stdout(base + "--tool hero.config.apply.set --args-json " +
                          "'{\"key\":\"protocol_layer\",\"value\":\"STDIO\","
                          "\"reason\":\"schema-compat\"}'");
  require_contains(apply_output, "\"executed\":true",
                   "Config apply direct set should execute");
  require_contains(apply_output, "\"preflight\":{",
                   "Config apply direct set should include preflight");
  require_contains(apply_output, "\"updated\":true",
                   "Config apply direct set should return mutation result");
  if (apply_output.find("args_digest") != std::string::npos ||
      apply_output.find("args_path") != std::string::npos) {
    throw std::runtime_error(
        "Config apply direct set should not return request-file metadata\n" +
        apply_output);
  }

  const auto unknown_old_tool =
      run_command_capture(base + "--tool hero.config.schema --args-json '{}'");
  if (unknown_old_tool.status == 0 ||
      unknown_old_tool.output.find("unknown tool: hero.config.schema") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Config tool hero.config.schema should fail as unknown\n" +
        unknown_old_tool.output);
  }

  const auto broad_router =
      run_command_capture(base + "--tool hero.config.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  if (broad_router.status == 0 ||
      broad_router.output.find("unknown tool: hero.config.inspect") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired broad Config inspect router should fail as unknown\n" +
        broad_router.output);
  }

  const auto broad_apply_router =
      run_command_capture(base + "--tool hero.config.apply --args-json "
                                 "'{\"subject\":\"set\",\"mode\":\"plan\"}'");
  if (broad_apply_router.status == 0 ||
      broad_apply_router.output.find("unknown tool: hero.config.apply") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired broad Config apply router should fail as unknown\n" +
        broad_apply_router.output);
  }

  const auto unknown_field = run_command_capture(
      base + "--tool hero.config.inspect.schema --args-json "
             "'{\"old_field\":true}'");
  if (unknown_field.status == 0 ||
      unknown_field.output.find("unknown field: old_field") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config inspect split tools should fail closed on unknown fields\n" +
        unknown_field.output);
  }

  const auto retired_inspect_field = run_command_capture(
      base + "--tool hero.config.inspect.value --args-json "
             "'{\"key\":\"protocol_layer\",\"args_path\":\"/tmp/nope.kv\"}'");
  if (retired_inspect_field.status == 0 ||
      retired_inspect_field.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config inspect split tools should reject retired args_path field\n" +
        retired_inspect_field.output);
  }

  const auto retired_apply_field = run_command_capture(
      base + "--tool hero.config.apply.set --args-json "
             "'{\"mode\":\"plan\",\"key\":\"protocol_layer\","
             "\"value\":\"STDIO\"}'");
  if (retired_apply_field.status == 0 ||
      retired_apply_field.output.find("unknown field: mode") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config apply direct tools should reject retired mode field\n" +
        retired_apply_field.output);
  }

  const auto retired_apply_args_path = run_command_capture(
      base + "--tool hero.config.apply.set --args-json "
             "'{\"key\":\"protocol_layer\",\"value\":\"STDIO\","
             "\"args_path\":\"/tmp/nope.kv\"}'");
  if (retired_apply_args_path.status == 0 ||
      retired_apply_args_path.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config apply direct tools should reject retired args_path field\n" +
        retired_apply_args_path.output);
  }

  const auto retired_apply_subject =
      run_command_capture(base + "--tool hero.config.apply.set --args-json "
                                 "'{\"subject\":\"set\","
                                 "\"key\":\"protocol_layer\","
                                 "\"value\":\"STDIO\"}'");
  if (retired_apply_subject.status == 0 ||
      retired_apply_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config apply split tools should reject retired subject field\n" +
        retired_apply_subject.output);
  }

  const auto retired_apply_digest = run_command_capture(
      base + "--tool hero.config.apply.set --args-json "
             "'{\"key\":\"protocol_layer\",\"value\":\"STDIO\","
             "\"args_digest\":\"wrong\"}'");
  if (retired_apply_digest.status == 0 ||
      retired_apply_digest.output.find("unknown field: args_digest") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config apply direct tools should reject retired args_digest field\n" +
        retired_apply_digest.output);
  }

  const auto retired_apply_create_backup = run_command_capture(
      base + "--tool hero.config.apply.write --args-json "
             "'{\"path\":\"hero.config.dsl\",\"content\":\"\","
             "\"create_backup\":true}'");
  if (retired_apply_create_backup.status == 0 ||
      retired_apply_create_backup.output.find("unknown field: create_backup") ==
          std::string::npos) {
    throw std::runtime_error(
        "Config apply write should reject no-op create_backup field\n" +
        retired_apply_create_backup.output);
  }

  const auto retired_validate_tool =
      run_command_capture(base + "--tool hero.config.inspect.validate "
                                 "--args-json '{}'");
  if (retired_validate_tool.status == 0 ||
      retired_validate_tool.output.find(
          "unknown tool: hero.config.inspect.validate") == std::string::npos) {
    throw std::runtime_error(
        "retired Config inspect validate shortcut should fail closed\n" +
        retired_validate_tool.output);
  }
}

[[nodiscard]] fs::path write_runtime_schema_compat_global_config() {
  const fs::path source_config = "/cuwacunu/src/config/.config";
  const fs::path compat_config =
      source_config.parent_path() / ".hero_mcp_schema_compat_runtime.config";

  std::ifstream in(source_config);
  if (!in) {
    throw std::runtime_error("failed to read global config fixture: " +
                             source_config.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  std::string text = buffer.str();

  const std::string from = "runtime_wave_id = policy_training_ppo_v0";
  const std::string to = "runtime_wave_id = cwu_02v_validation_eval_channel_mdn";
  const auto pos = text.find(from);
  if (pos == std::string::npos) {
    throw std::runtime_error(
        "schema compatibility fixture could not find runtime_wave_id");
  }
  text.replace(pos, from.size(), to);

  fs::create_directories(compat_config.parent_path());
  {
    std::error_code ec;
    fs::remove(compat_config, ec);
  }
  std::ofstream out(compat_config);
  if (!out) {
    throw std::runtime_error("failed to write runtime schema compatibility "
                             "global config: " +
                             compat_config.string());
  }
  out << text;
  return compat_config;
}

struct scoped_file_cleanup_t {
  fs::path path{};

  ~scoped_file_cleanup_t() {
    if (path.empty()) {
      return;
    }
    std::error_code ec;
    fs::remove(path, ec);
  }
};

void check_runtime_collapsed_surface(const fs::path &binary) {
  const fs::path compat_config = write_runtime_schema_compat_global_config();
  const scoped_file_cleanup_t cleanup{compat_config};
  const std::string base =
      binary.string() + " --global-config " + compat_config.string() + " ";

  const std::string schema_output = read_command_stdout(
      base + "--tool hero.runtime.inspect.schema --args-json '{}'");
  require_contains(schema_output, "\"keys\":[",
                   "Runtime inspect.schema should return policy keys");

  const std::string wave_output = read_command_stdout(
      base + "--tool hero.runtime.inspect.wave --args-json '{}'");
  require_contains(wave_output, "\"source_range\"",
                   "Runtime inspect.wave should decode active wave");

  const std::string jobs_output =
      read_command_stdout(base + "--tool hero.runtime.inspect.jobs --args-json "
                                 "'{\"root\":\"/tmp\",\"limit\":0,"
                                 "\"include_artifacts\":false}'");
  require_contains(jobs_output, "\"root\":\"/tmp\"",
                   "Runtime inspect.jobs should accept direct selectors");

  const auto retired_inspect_router =
      run_command_capture(base + "--tool hero.runtime.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  if (retired_inspect_router.status == 0 ||
      retired_inspect_router.output.find(
          "unknown tool: hero.runtime.inspect") == std::string::npos) {
    throw std::runtime_error(
        "Runtime broad inspect router should fail as unknown\n" +
        retired_inspect_router.output);
  }

  const auto inspect_schema_retired_args = run_command_capture(
      base + "--tool hero.runtime.inspect.schema --args-json "
             "'{\"subject\":\"schema\"}'");
  if (inspect_schema_retired_args.status == 0 ||
      inspect_schema_retired_args.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime inspect.schema should reject retired subject field\n" +
        inspect_schema_retired_args.output);
  }

  const auto inspect_wave_retired_args =
      run_command_capture(base + "--tool hero.runtime.inspect.wave --args-json "
                                 "'{\"args_path\":\"/tmp/nope.kv\","
                                 "\"args_digest\":\"wrong\"}'");
  if (inspect_wave_retired_args.status == 0 ||
      inspect_wave_retired_args.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime inspect.wave should reject retired request fields\n" +
        inspect_wave_retired_args.output);
  }

  const std::string reset_plan_output =
      read_command_stdout(base + "--tool hero.runtime.reset --args-json "
                                 "'{\"mode\":\"plan\"}'");
  require_contains(reset_plan_output, "\"dry_run\":true",
                   "Runtime reset mode=plan should preview reset");

  const auto reset_retired_args =
      run_command_capture(base + "--tool hero.runtime.reset --args-json "
                                 "'{\"mode\":\"plan\","
                                 "\"args_path\":\"/tmp/reset.kv\"}'");
  if (reset_retired_args.status == 0 ||
      reset_retired_args.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime reset should reject retired args_path field\n" +
        reset_retired_args.output);
  }

  const std::string run_preview_output =
      read_command_stdout(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\"}'");
  require_contains(run_preview_output, "\"argv\":[",
                   "Runtime run dry_run should still be accepted without "
                   "a request file");

  const auto unknown_old_tool =
      run_command_capture(base + "--tool hero.runtime.schema --args-json '{}'");
  if (unknown_old_tool.status == 0 ||
      unknown_old_tool.output.find("unknown tool: hero.runtime.schema") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Runtime tool hero.runtime.schema should fail as unknown\n" +
        unknown_old_tool.output);
  }

  const auto old_dry_run_field =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\",\"dry_run\":true}'");
  if (old_dry_run_field.status == 0 ||
      old_dry_run_field.output.find("unknown field: dry_run") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired top-level dry_run field\n" +
        old_dry_run_field.output);
  }

  const auto old_args_path_field =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\","
                                 "\"args_path\":\"/tmp/runtime-run.kv\"}'");
  if (old_args_path_field.status == 0 ||
      old_args_path_field.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired args_path field\n" +
        old_args_path_field.output);
  }

  const auto old_args_digest_field =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\","
                                 "\"args_digest\":\"digest\"}'");
  if (old_args_digest_field.status == 0 ||
      old_args_digest_field.output.find("unknown field: args_digest") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired args_digest field\n" +
        old_args_digest_field.output);
  }

  const auto old_config_path_field = run_command_capture(
      base + "--tool hero.runtime.run --args-json "
             "'{\"mode\":\"dry_run\","
             "\"config_path\":\"/cuwacunu/src/config/.config\"}'");
  if (old_config_path_field.status == 0 ||
      old_config_path_field.output.find("unknown field: config_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired top-level config_path field\n" +
        old_config_path_field.output);
  }

  const auto old_execution_request_field =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\","
                                 "\"execution_request_path\":\"/tmp/x\"}'");
  if (old_execution_request_field.status == 0 ||
      old_execution_request_field.output.find(
          "unknown field: execution_request_path") == std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired top-level execution_request_path "
        "field\n" +
        old_execution_request_field.output);
  }

  const auto handoff_digest_without_path =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\","
                                 "\"runtime_handoff_digest\":\"digest\"}'");
  if (handoff_digest_without_path.status == 0 ||
      handoff_digest_without_path.output.find(
          "runtime_handoff_digest requires runtime_handoff_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject runtime_handoff_digest without path\n" +
        handoff_digest_without_path.output);
  }

  const auto contract_digest_without_path =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"mode\":\"dry_run\","
                                 "\"contract_digest\":\"digest\"}'");
  if (contract_digest_without_path.status == 0 ||
      contract_digest_without_path.output.find(
          "contract_digest requires contract_path") == std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject contract_digest without path\n" +
        contract_digest_without_path.output);
  }

  const auto run_retired_subject =
      run_command_capture(base + "--tool hero.runtime.run --args-json "
                                 "'{\"subject\":\"wave\","
                                 "\"mode\":\"dry_run\"}'");
  if (run_retired_subject.status == 0 ||
      run_retired_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime run should reject retired subject field\n" +
        run_retired_subject.output);
  }

  const auto split_run_wave_tool = run_command_capture(
      base + "--tool hero.runtime.run_wave --args-json "
             "'{\"mode\":\"dry_run\",\"config_path\":\"/tmp/x\"}'");
  if (split_run_wave_tool.status == 0 ||
      split_run_wave_tool.output.find("unknown tool: hero.runtime.run_wave") ==
          std::string::npos) {
    throw std::runtime_error(
        "Runtime split tool hero.runtime.run_wave should fail as unknown\n" +
        split_run_wave_tool.output);
  }

  const auto replay_delegate_as_tool =
      run_command_capture(base + "--tool hero.runtime.replay --args-json "
                                 "'{\"mode\":\"dry_run\"}'");
  if (replay_delegate_as_tool.status == 0 ||
      replay_delegate_as_tool.output.find(
          "unknown tool: hero.runtime.replay") == std::string::npos) {
    throw std::runtime_error(
        "Runtime replay delegate should not be reachable as a tool\n" +
        replay_delegate_as_tool.output);
  }
}

void check_environment_inspect_derouted_surface(const fs::path &binary) {
  const std::string base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";
  const fs::path root =
      fs::temp_directory_path() / "hero_mcp_schema_compat" / "environment";
  const fs::path job_dir = root / "job";
  fs::create_directories(job_dir);

  {
    std::ofstream sidecar(job_dir / "lattice.policy_acceptance.fact",
                          std::ios::trunc);
    if (!sidecar) {
      throw std::runtime_error("failed to write Environment sidecar fixture");
    }
    sidecar << "policy_acceptance_fixture=ok\n";
  }

  const std::string schema_output = read_command_stdout(
      base + "--tool hero.environment.inspect.schema --args-json '{}'");
  require_contains(schema_output, "\"policy_schema\":",
                   "Environment inspect.schema should return policy schema");

  const std::string job_output = read_command_stdout(
      base +
      "--tool hero.environment.inspect.job --args-json "
      "'{\"job_dir\":\"" +
      job_dir.string() + "\",\"include_text\":true,\"max_bytes\":12}'");
  require_contains(job_output, "\"tool\":\"hero.environment.inspect.job\"",
                   "Environment inspect.job should inspect job");
  require_contains(job_output, "\"job_dir\":\"" + job_dir.string() + "\"",
                   "Environment inspect.job should preserve job_dir identity");
  require_contains(job_output, "\"policy_accep",
                   "Environment inspect.job include_text should preview "
                   "sidecar");
  if (job_output.find("\"args_digest\"") != std::string::npos) {
    throw std::runtime_error(
        "Environment inspect.job should not return request digest metadata\n" +
        job_output);
  }

  const auto retired_router =
      run_command_capture(base + "--tool hero.environment.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  if (retired_router.status == 0 ||
      retired_router.output.find("unknown tool: hero.environment.inspect") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment broad inspect router should fail as unknown\n" +
        retired_router.output);
  }

  const auto schema_retired_args = run_command_capture(
      base + "--tool hero.environment.inspect.schema --args-json "
             "'{\"args_path\":\"/tmp/nope.kv\"}'");
  if (schema_retired_args.status == 0 ||
      schema_retired_args.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment inspect.schema should reject retired args_path field\n" +
        schema_retired_args.output);
  }

  const auto job_retired_args =
      run_command_capture(base +
                          "--tool hero.environment.inspect.job --args-json "
                          "'{\"job_dir\":\"" +
                          job_dir.string() +
                          "\",\"subject\":\"job\",\"args_path\":\"/tmp/nope."
                          "kv\",\"args_digest\":\"wrong\"}'");
  if (job_retired_args.status == 0 ||
      job_retired_args.output.find("unknown field: args_digest") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment inspect.job should reject retired request fields\n" +
        job_retired_args.output);
  }

  const auto job_missing_dir = run_command_capture(
      base + "--tool hero.environment.inspect.job --args-json "
             "'{}'");
  if (job_missing_dir.status == 0 ||
      job_missing_dir.output.find("missing required field: job_dir") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment inspect.job should require direct job_dir\n" +
        job_missing_dir.output);
  }

  const auto retired_certify_router = run_command_capture(
      base + "--tool hero.environment.certify --args-json "
             "'{\"subject\":\"policy_acceptance\",\"mode\":\"check\","
             "\"args_path\":\"/tmp/nope.kv\"}'");
  if (retired_certify_router.status == 0 ||
      retired_certify_router.output.find(
          "unknown tool: hero.environment.certify") == std::string::npos) {
    throw std::runtime_error(
        "Environment broad certify router should fail as unknown\n" +
        retired_certify_router.output);
  }

  const auto certify_retired_subject = run_command_capture(
      base + "--tool hero.environment.certify.policy_acceptance --args-json "
             "'{\"subject\":\"policy_acceptance\",\"mode\":\"check\","
             "\"policy_training_job_dir\":\"/tmp/x\","
             "\"acceptance_id\":\"policy_acceptance_fixture_v1\","
             "\"certification_evidence\":{}}'");
  if (certify_retired_subject.status == 0 ||
      certify_retired_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment certify.policy_acceptance should reject retired subject "
        "field\n" +
        certify_retired_subject.output);
  }

  const auto certify_retired_args_path = run_command_capture(
      base + "--tool hero.environment.certify.policy_acceptance --args-json "
             "'{\"mode\":\"check\",\"policy_training_job_dir\":\"/tmp/x\","
             "\"acceptance_id\":\"policy_acceptance_fixture_v1\","
             "\"certification_evidence\":{},"
             "\"args_path\":\"/tmp/nope.kv\"}'");
  if (certify_retired_args_path.status == 0 ||
      certify_retired_args_path.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment certify.policy_acceptance should reject retired "
        "args_path field\n" +
        certify_retired_args_path.output);
  }

  const auto certify_retired_args_digest = run_command_capture(
      base + "--tool hero.environment.certify.paper_online_readiness "
             "--args-json "
             "'{\"mode\":\"check\",\"policy_acceptance_job_dir\":\"/tmp/x\","
             "\"readiness_id\":\"paper_online_readiness_fixture_v1\","
             "\"certification_evidence\":{},"
             "\"args_digest\":\"digest\"}'");
  if (certify_retired_args_digest.status == 0 ||
      certify_retired_args_digest.output.find("unknown field: args_digest") ==
          std::string::npos) {
    throw std::runtime_error(
        "Environment certify.paper_online_readiness should reject retired "
        "args_digest field\n" +
        certify_retired_args_digest.output);
  }
}

void check_marshal_derouted_surface(const fs::path &binary) {
  const std::string base =
      binary.string() + " --global-config /cuwacunu/src/config/.config ";

  const auto broad_inspect =
      run_command_capture(base + "--tool hero.marshal.inspect --args-json "
                                 "'{\"subject\":\"run\"}'");
  if (broad_inspect.status == 0 ||
      broad_inspect.output.find("unknown tool: hero.marshal.inspect") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Marshal tool hero.marshal.inspect should fail as unknown\n" +
        broad_inspect.output);
  }

  const auto retired_run =
      run_command_capture(base + "--tool hero.marshal.inspect.run --args-json "
                                 "'{\"subject\":\"run\"}'");
  if (retired_run.status == 0 ||
      retired_run.output.find("unknown tool: hero.marshal.inspect.run") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Marshal tool hero.marshal.inspect.run should fail as "
        "unknown\n" +
        retired_run.output);
  }

  const auto retired_facts = run_command_capture(
      base + "--tool hero.marshal.inspect.facts --args-json "
             "'{\"mode\":\"summary\",\"args_path\":\"/tmp/request.kv\"}'");
  if (retired_facts.status == 0 ||
      retired_facts.output.find("unknown tool: hero.marshal.inspect.facts") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Marshal tool hero.marshal.inspect.facts should fail as "
        "unknown\n" +
        retired_facts.output);
  }

  const auto retired_subject = run_command_capture(
      base + "--tool hero.marshal.inspect.run.latest_chain --args-json "
             "'{\"subject\":\"run\"}'");
  if (retired_subject.status == 0 ||
      retired_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Marshal inspect.run.latest_chain should reject retired subject "
        "field\n" +
        retired_subject.output);
  }

  const auto retired_args_path = run_command_capture(
      base + "--tool hero.marshal.inspect.facts.summary --args-json "
             "'{\"args_path\":\"/tmp/request.kv\"}'");
  if (retired_args_path.status == 0 ||
      retired_args_path.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Marshal inspect.facts.summary should reject retired args_path "
        "field\n" +
        retired_args_path.output);
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

[[nodiscard]] std::string json_string(const std::string &value) {
  std::ostringstream out;
  out << '"';
  for (const char c : value) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      out << c;
      break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] std::string trim_ascii_copy(const std::string &value) {
  std::size_t begin = 0;
  while (begin < value.size() &&
         std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
    ++begin;
  }
  std::size_t end = value.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }
  return value.substr(begin, end - begin);
}

[[nodiscard]] std::string shell_quote(const std::string &value) {
  std::string out = "'";
  for (const char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

[[nodiscard]] bool lattice_inspect_int_arg(const std::string &key) {
  return key == "limit" || key == "fact_index";
}

[[nodiscard]] bool lattice_inspect_bool_arg(const std::string &key) {
  return key == "include_facts" || key == "compare_live_scan" ||
         key == "allow_unproven_cache";
}

[[nodiscard]] std::string
lattice_inspect_request_value_json(const std::string &key,
                                   const std::string &value) {
  const std::string trimmed = trim_ascii_copy(value);
  if (lattice_inspect_int_arg(key) || lattice_inspect_bool_arg(key)) {
    return trimmed;
  }
  return json_string(trimmed);
}

[[nodiscard]] std::string
lattice_inspect_tool_name(const std::string &top_level_json,
                          const std::string &request_text = {}) {
  const auto fields = parse_json_object_fields(top_level_json);
  for (const auto &field : fields) {
    if (field.key != "subject") {
      continue;
    }
    std::size_t value_idx = 0;
    const std::string subject = parse_json_string_at(field.raw, &value_idx);
    if (subject == "schema") {
      return "hero.lattice.inspect.schema";
    }
    if (subject == "targets") {
      return "hero.lattice.inspect.targets";
    }
    if (subject == "target") {
      return "hero.lattice.inspect.target";
    }
    if (subject == "exposure") {
      return "hero.lattice.inspect.exposure";
    }
    if (subject == "fact_families") {
      return "hero.lattice.inspect.fact_families";
    }
    if (subject == "facts") {
      std::string mode = "summary";
      for (const auto &mode_field : fields) {
        if (mode_field.key == "mode") {
          std::size_t mode_idx = 0;
          mode = parse_json_string_at(mode_field.raw, &mode_idx);
          break;
        }
      }
      if (mode == "summary" || mode == "scan" || mode == "lineage" ||
          mode == "preview") {
        if (mode == "preview") {
          bool has_fact_digest = false;
          bool has_fact_digest_prefix = false;
          bool has_fact_index = false;
          for (const auto &selector_field : fields) {
            if (selector_field.key == "fact_digest") {
              has_fact_digest = true;
            } else if (selector_field.key == "fact_digest_prefix") {
              has_fact_digest_prefix = true;
            } else if (selector_field.key == "fact_index") {
              has_fact_index = true;
            }
          }
          std::istringstream request_lines(request_text);
          std::string line;
          while (std::getline(request_lines, line)) {
            line = trim_ascii_copy(line);
            if (line.empty()) {
              continue;
            }
            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) {
              continue;
            }
            const std::string key = trim_ascii_copy(line.substr(0, eq));
            if (key == "fact_digest") {
              has_fact_digest = true;
            } else if (key == "fact_digest_prefix") {
              has_fact_digest_prefix = true;
            } else if (key == "fact_index") {
              has_fact_index = true;
            }
          }
          if (has_fact_digest) {
            return "hero.lattice.inspect.facts.preview.by_digest";
          }
          if (has_fact_digest_prefix) {
            return "hero.lattice.inspect.facts.preview.by_digest_prefix";
          }
          if (has_fact_index) {
            return "hero.lattice.inspect.facts.preview.by_index";
          }
        }
        return "hero.lattice.inspect.facts." + mode;
      }
      throw std::runtime_error("unknown lattice inspect facts mode in test: " +
                               mode);
    }
    if (subject == "index") {
      std::string mode = "status";
      bool unproven_cache = false;
      for (const auto &mode_field : fields) {
        if (mode_field.key == "mode") {
          std::size_t mode_idx = 0;
          mode = parse_json_string_at(mode_field.raw, &mode_idx);
        }
        if (mode_field.key == "allow_unproven_cache" &&
            mode_field.raw == "true") {
          unproven_cache = true;
        }
      }
      if (mode == "status") {
        return "hero.lattice.inspect.index.status";
      }
      if (mode == "query") {
        bool has_relation = false;
        bool has_key = false;
        bool has_key_contains = false;
        bool has_digest = false;
        bool has_digest_prefix = false;
        for (const auto &selector_field : fields) {
          if (selector_field.key == "relation") {
            has_relation = true;
          } else if (selector_field.key == "key") {
            has_key = true;
          } else if (selector_field.key == "key_contains") {
            has_key_contains = true;
          } else if (selector_field.key == "digest") {
            has_digest = true;
          } else if (selector_field.key == "digest_prefix") {
            has_digest_prefix = true;
          }
        }
        std::istringstream request_lines(request_text);
        std::string line;
        while (std::getline(request_lines, line)) {
          line = trim_ascii_copy(line);
          if (line.empty()) {
            continue;
          }
          const std::size_t eq = line.find('=');
          if (eq == std::string::npos) {
            continue;
          }
          const std::string key = trim_ascii_copy(line.substr(0, eq));
          if (key == "relation") {
            has_relation = true;
          } else if (key == "key") {
            has_key = true;
          } else if (key == "key_contains") {
            has_key_contains = true;
          } else if (key == "digest") {
            has_digest = true;
          } else if (key == "digest_prefix") {
            has_digest_prefix = true;
          }
        }
        std::string tool = unproven_cache ? "hero.lattice.inspect.index.query."
                                            "unproven_cache"
                                          : "hero.lattice.inspect.index.query";
        if (has_relation) {
          tool += ".by_relation";
        } else if (has_key) {
          tool += ".by_key";
        } else if (has_key_contains) {
          tool += ".by_key_contains";
        } else if (has_digest) {
          tool += ".by_digest";
        } else if (has_digest_prefix) {
          tool += ".by_digest_prefix";
        }
        return tool;
      }
      throw std::runtime_error("unknown lattice inspect index mode in test: " +
                               mode);
    }
    if (subject == "derived") {
      std::string relation;
      bool has_target_id = false;
      bool has_checkpoint_path = false;
      bool has_checkpoint_identity = false;
      bool has_ancestor_path = false;
      bool has_ancestor_id = false;
      for (const auto &relation_field : fields) {
        if (relation_field.key == "relation") {
          std::size_t relation_idx = 0;
          relation = parse_json_string_at(relation_field.raw, &relation_idx);
        }
        if (relation_field.key == "target_id") {
          has_target_id = true;
        } else if (relation_field.key == "checkpoint_path") {
          has_checkpoint_path = true;
        } else if (relation_field.key == "checkpoint_id" ||
                   relation_field.key == "checkpoint_file_digest") {
          has_checkpoint_identity = true;
        } else if (relation_field.key == "ancestor_checkpoint_path") {
          has_ancestor_path = true;
        } else if (relation_field.key == "ancestor_checkpoint_id") {
          has_ancestor_id = true;
        }
      }
      std::istringstream request_lines(request_text);
      std::string line;
      while (std::getline(request_lines, line)) {
        line = trim_ascii_copy(line);
        if (line.empty()) {
          continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
          continue;
        }
        const std::string key = trim_ascii_copy(line.substr(0, eq));
        const std::string value = trim_ascii_copy(line.substr(eq + 1));
        if (key == "relation" && relation.empty()) {
          relation = value;
        } else if (key == "target_id") {
          has_target_id = true;
        } else if (key == "checkpoint_path") {
          has_checkpoint_path = true;
        } else if (key == "checkpoint_id" || key == "checkpoint_file_digest") {
          has_checkpoint_identity = true;
        } else if (key == "ancestor_checkpoint_path") {
          has_ancestor_path = true;
        } else if (key == "ancestor_checkpoint_id") {
          has_ancestor_id = true;
        }
      }
      if (relation == "target_satisfied" || relation == "satisfied_target") {
        return "hero.lattice.inspect.derived.target_satisfied";
      }
      if (relation == "forbidden_overlap") {
        return "hero.lattice.inspect.derived.forbidden_overlap";
      }
      if (relation == "stale_cache") {
        return "hero.lattice.inspect.derived.stale_cache";
      }
      if (relation == "checkpoint_ancestor") {
        if (has_checkpoint_path && has_checkpoint_identity) {
          throw std::runtime_error(
              "checkpoint_ancestor test mixes checkpoint selectors");
        }
        std::string tool =
            has_checkpoint_path
                ? "hero.lattice.inspect.derived.checkpoint_ancestor."
                  "by_checkpoint_path"
                : "hero.lattice.inspect.derived.checkpoint_ancestor."
                  "by_checkpoint_identity";
        if (!has_checkpoint_path && !has_checkpoint_identity) {
          throw std::runtime_error(
              "checkpoint_ancestor test requires checkpoint path or identity");
        }
        if (has_ancestor_path && has_ancestor_id) {
          throw std::runtime_error(
              "checkpoint_ancestor test mixes ancestor selectors");
        }
        if (has_ancestor_path) {
          tool += ".with_ancestor_path";
        } else if (has_ancestor_id) {
          tool += ".with_ancestor_id";
        }
        return tool;
      }
      if (relation == "unresolved_lineage") {
        return has_target_id
                   ? "hero.lattice.inspect.derived.unresolved_lineage.target"
                   : "hero.lattice.inspect.derived.unresolved_lineage."
                     "checkpoint";
      }
      throw std::runtime_error(
          "unknown lattice inspect derived relation in test: " + relation);
    }
    if (subject == "checkpoint") {
      return "hero.lattice.inspect.checkpoint";
    }
    throw std::runtime_error("unknown lattice inspect subject in test: " +
                             subject);
  }
  throw std::runtime_error("missing lattice inspect subject in test");
}

[[nodiscard]] std::string
lattice_inspect_args_json(const std::string &top_level_json,
                          const std::string &request_text = {}) {
  const auto fields = parse_json_object_fields(top_level_json);
  std::string subject;
  for (const auto &field : fields) {
    if (field.key == "subject") {
      std::size_t value_idx = 0;
      subject = parse_json_string_at(field.raw, &value_idx);
      break;
    }
  }
  std::ostringstream top;
  top << "{";
  bool first = true;
  const auto append_top = [&](const std::string &key, const std::string &raw) {
    if (!first) {
      top << ",";
    }
    top << json_string(key) << ":" << raw;
    first = false;
  };
  for (const auto &field : fields) {
    if (field.key == "subject" || field.key == "args_path" ||
        field.key == "args_digest" ||
        ((subject == "facts" || subject == "index") && field.key == "mode") ||
        (subject == "derived" && field.key == "relation") ||
        (subject == "derived" && is_lattice_identity_filter_field(field.key)) ||
        (subject == "index" && (field.key == "compare_live_scan" ||
                                field.key == "allow_unproven_cache"))) {
      continue;
    }
    append_top(field.key, field.raw);
  }

  std::istringstream request_lines(request_text);
  std::string line;
  while (std::getline(request_lines, line)) {
    line = trim_ascii_copy(line);
    if (line.empty()) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("bad lattice inspect request line in test: " +
                               line);
    }
    const std::string key = trim_ascii_copy(line.substr(0, eq));
    const std::string value = line.substr(eq + 1);
    if ((subject == "derived" &&
         (key == "relation" || is_lattice_identity_filter_field(key))) ||
        (subject == "index" &&
         (key == "compare_live_scan" || key == "allow_unproven_cache"))) {
      continue;
    }
    append_top(key, lattice_inspect_request_value_json(key, value));
  }

  top << "}";
  return top.str();
}

[[nodiscard]] std::string
lattice_inspect_call_shell(const std::string &top_level_json,
                           const std::string &request_text = {}) {
  return "--tool " + lattice_inspect_tool_name(top_level_json, request_text) +
         " --args-json " +
         shell_quote(lattice_inspect_args_json(top_level_json, request_text));
}

[[nodiscard]] std::string
lattice_evaluate_args_json(const std::string &operation,
                           const std::string &request_text) {
  std::ostringstream top;
  top << "{";
  bool first = true;
  const auto append_top = [&](const std::string &key, const std::string &raw) {
    if (!first) {
      top << ",";
    }
    top << json_string(key) << ":" << raw;
    first = false;
  };

  std::istringstream request_lines(request_text);
  std::string line;
  while (std::getline(request_lines, line)) {
    line = trim_ascii_copy(line);
    if (line.empty()) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("bad lattice evaluate request line in test: " +
                               line);
    }
    const std::string key = trim_ascii_copy(line.substr(0, eq));
    const std::string value = trim_ascii_copy(line.substr(eq + 1));
    if ((operation == "target" || operation == "targets" ||
         operation == "deficit" ||
         operation == "latest_satisfying_checkpoint") &&
        is_lattice_identity_filter_field(key)) {
      continue;
    }
    if (key == "limit") {
      append_top(key, value);
      continue;
    }
    if (key == "target_ids") {
      if (!value.empty() && value.front() == '[') {
        append_top(key, value);
        continue;
      }
      std::ostringstream array_json;
      array_json << "[";
      std::istringstream csv(value);
      std::string item;
      bool first_item = true;
      while (std::getline(csv, item, ',')) {
        item = trim_ascii_copy(item);
        if (item.empty()) {
          continue;
        }
        if (!first_item) {
          array_json << ",";
        }
        array_json << json_string(item);
        first_item = false;
      }
      array_json << "]";
      append_top(key, array_json.str());
      continue;
    }
    append_top(key, json_string(value));
  }

  top << "}";
  return top.str();
}

[[nodiscard]] std::string
lattice_evaluate_tool_name(const std::string &operation,
                           const std::string &request_text = {}) {
  if (operation == "target") {
    return "hero.lattice.evaluate.target";
  }
  if (operation == "targets") {
    return "hero.lattice.evaluate.targets";
  }
  if (operation == "deficit") {
    return "hero.lattice.evaluate.deficit";
  }
  if (operation == "latest_satisfying_checkpoint") {
    return request_text.find("symbolic_hint=") != std::string::npos
               ? "hero.lattice.evaluate.latest_satisfying_checkpoint.hint"
               : "hero.lattice.evaluate.latest_satisfying_checkpoint.target";
  }
  throw std::runtime_error("unknown lattice evaluate operation in test: " +
                           operation);
}

[[nodiscard]] std::string
lattice_evaluate_call_shell(const std::string &operation,
                            const std::string &request_text) {
  return "--tool " + lattice_evaluate_tool_name(operation, request_text) +
         " --args-json " +
         shell_quote(lattice_evaluate_args_json(operation, request_text));
}

[[nodiscard]] std::string
lattice_compare_args_json(const std::string &request_text) {
  std::ostringstream top;
  top << "{";
  bool first = true;
  std::istringstream request_lines(request_text);
  std::string line;
  while (std::getline(request_lines, line)) {
    line = trim_ascii_copy(line);
    if (line.empty()) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("bad lattice compare request line in test: " +
                               line);
    }
    const std::string key = trim_ascii_copy(line.substr(0, eq));
    const std::string value = trim_ascii_copy(line.substr(eq + 1));
    if (!first) {
      top << ",";
    }
    first = false;
    top << json_string(key) << ":" << json_string(value);
  }
  top << "}";
  return top.str();
}

[[nodiscard]] std::string
lattice_compare_args_shell(const std::string &request_text) {
  return shell_quote(lattice_compare_args_json(request_text));
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

  const auto split_policy = split::load_lattice_split_policy_from_files(
      "/cuwacunu/src/config/hero.lattice.split_policy.dsl",
      "/cuwacunu/src/config/ujcamei.source.splits.dsl");
  const auto *train_core = split_policy.find_split("train_core");
  if (train_core == nullptr) {
    throw std::runtime_error("missing train_core split");
  }
  const auto accepted_anchor_count = 1000;
  const auto materialized_train_core =
      split::materialize_lattice_split(*train_core, accepted_anchor_count);
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
           << "resolved_anchor_index_begin="
           << materialized_train_core.anchor_range.begin
           << "\nresolved_anchor_index_end="
           << materialized_train_core.anchor_range.end
           << "\naccepted_anchor_count=" << accepted_anchor_count
           << "\nsource_cursor_token=cursor_1|accepted=1000\n"
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

  const auto retired_inspect_router =
      run_command_capture(base + "--tool hero.lattice.inspect --args-json "
                                 "'{\"subject\":\"schema\"}'");
  if (retired_inspect_router.status == 0 ||
      retired_inspect_router.output.find(
          "unknown tool: hero.lattice.inspect") == std::string::npos) {
    throw std::runtime_error(
        "retired Lattice broad inspect router should fail as unknown\n" +
        retired_inspect_router.output);
  }

  const auto inspect_schema_retired_subject = run_command_capture(
      base + "--tool hero.lattice.inspect.schema --args-json "
             "'{\"subject\":\"schema\"}'");
  if (inspect_schema_retired_subject.status == 0 ||
      inspect_schema_retired_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice inspect.schema should reject retired subject field\n" +
        inspect_schema_retired_subject.output);
  }

  const auto inspect_facts_retired_request = run_command_capture(
      base + "--tool hero.lattice.inspect.facts --args-json "
             "'{\"mode\":\"summary\",\"args_path\":\"/tmp/request.kv\"}'");
  if (inspect_facts_retired_request.status == 0 ||
      inspect_facts_retired_request.output.find(
          "unknown tool: hero.lattice.inspect.facts") == std::string::npos) {
    throw std::runtime_error(
        "retired Lattice inspect.facts should fail as unknown\n" +
        inspect_facts_retired_request.output);
  }

  const auto inspect_facts_split_retired_mode = run_command_capture(
      base + "--tool hero.lattice.inspect.facts.summary --args-json "
             "'{\"mode\":\"summary\"}'");
  if (inspect_facts_split_retired_mode.status == 0 ||
      inspect_facts_split_retired_mode.output.find("unknown field: mode") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice inspect.facts.summary should reject retired mode field\n" +
        inspect_facts_split_retired_mode.output);
  }

  const auto retired_derived = run_command_capture(
      base + "--tool hero.lattice.inspect.derived --args-json "
             "'{\"relation\":\"target_satisfied\"}'");
  if (retired_derived.status == 0 ||
      retired_derived.output.find(
          "unknown tool: hero.lattice.inspect.derived") == std::string::npos) {
    throw std::runtime_error(
        "retired Lattice inspect.derived should fail as unknown\n" +
        retired_derived.output);
  }

  const auto retired_latest = run_command_capture(
      base + "--tool hero.lattice.evaluate.latest_satisfying_checkpoint "
             "--args-json '{\"target_id\":\"retired\"}'");
  if (retired_latest.status == 0 ||
      retired_latest.output.find("unknown tool: "
                                 "hero.lattice.evaluate."
                                 "latest_satisfying_checkpoint") ==
          std::string::npos) {
    throw std::runtime_error(
        "retired Lattice latest_satisfying_checkpoint should fail as "
        "unknown\n" +
        retired_latest.output);
  }

  const auto retired_evaluate_router =
      run_command_capture(base + "--tool hero.lattice.evaluate --args-json "
                                 "'{\"subject\":\"target\"}'");
  if (retired_evaluate_router.status == 0 ||
      retired_evaluate_router.output.find(
          "unknown tool: hero.lattice.evaluate") == std::string::npos) {
    throw std::runtime_error(
        "retired Lattice broad evaluate router should fail as unknown\n" +
        retired_evaluate_router.output);
  }

  const auto evaluate_target_retired_subject = run_command_capture(
      base + "--tool hero.lattice.evaluate.target --args-json "
             "'{\"subject\":\"target\",\"target_id\":\"retired\"}'");
  if (evaluate_target_retired_subject.status == 0 ||
      evaluate_target_retired_subject.output.find("unknown field: subject") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice evaluate.target should reject retired subject field\n" +
        evaluate_target_retired_subject.output);
  }

  const auto evaluate_target_retired_request = run_command_capture(
      base + "--tool hero.lattice.evaluate.target --args-json "
             "'{\"target_id\":\"retired\",\"args_path\":\"/tmp/request.kv\"}'");
  if (evaluate_target_retired_request.status == 0 ||
      evaluate_target_retired_request.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice evaluate.target should reject retired args_path field\n" +
        evaluate_target_retired_request.output);
  }

  const auto evaluate_deficit_retired_digest = run_command_capture(
      base + "--tool hero.lattice.evaluate.deficit --args-json "
             "'{\"target_id\":\"retired\",\"args_digest\":\"digest\"}'");
  if (evaluate_deficit_retired_digest.status == 0 ||
      evaluate_deficit_retired_digest.output.find(
          "unknown field: args_digest") == std::string::npos) {
    throw std::runtime_error(
        "Lattice evaluate.deficit should reject retired args_digest field\n" +
        evaluate_deficit_retired_digest.output);
  }

  const auto evaluate_target_retired_identity = run_command_capture(
      base + "--tool hero.lattice.evaluate.target --args-json "
             "'{\"target_id\":\"retired\","
             "\"protocol_contract_fingerprint\":\"pc\"}'");
  if (evaluate_target_retired_identity.status == 0 ||
      evaluate_target_retired_identity.output.find(
          "unknown field: protocol_contract_fingerprint") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice evaluate.target should reject retired identity filters\n" +
        evaluate_target_retired_identity.output);
  }

  const auto derived_retired_identity = run_command_capture(
      base + "--tool hero.lattice.inspect.derived.target_satisfied --args-json "
             "'{\"target_id\":\"retired\",\"protocol_id\":\"protocol\"}'");
  if (derived_retired_identity.status == 0 ||
      derived_retired_identity.output.find("unknown field: protocol_id") ==
          std::string::npos) {
    throw std::runtime_error("Lattice target-derived inspect should reject "
                             "retired identity filters\n" +
                             derived_retired_identity.output);
  }

  const auto missing_compare_left = run_command_capture(
      base + "--tool hero.lattice.compare --args-json '{}'");
  if (missing_compare_left.status == 0 ||
      missing_compare_left.output.find("left_target_id is required") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice compare should require direct left_target_id\n" +
        missing_compare_left.output);
  }

  const auto retired_compare_args_path = run_command_capture(
      base + "--tool hero.lattice.compare --args-json "
             "'{\"left_target_id\":\"left\",\"right_target_id\":\"right\","
             "\"args_path\":\"/tmp/request.kv\"}'");
  if (retired_compare_args_path.status == 0 ||
      retired_compare_args_path.output.find("unknown field: args_path") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice compare should reject retired args_path field\n" +
        retired_compare_args_path.output);
  }

  const auto retired_compare_digest = run_command_capture(
      base + "--tool hero.lattice.compare --args-json "
             "'{\"left_target_id\":\"left\",\"right_target_id\":\"right\","
             "\"args_digest\":\"wrong\"}'");
  if (retired_compare_digest.status == 0 ||
      retired_compare_digest.output.find("unknown field: args_digest") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice compare should reject retired args_digest field\n" +
        retired_compare_digest.output);
  }

  const auto retired_compare_identity = run_command_capture(
      base + "--tool hero.lattice.compare --args-json "
             "'{\"left_target_id\":\"left\",\"right_target_id\":\"right\","
             "\"protocol_contract_fingerprint\":\"pc\"}'");
  if (retired_compare_identity.status == 0 ||
      retired_compare_identity.output.find(
          "unknown field: protocol_contract_fingerprint") ==
          std::string::npos) {
    throw std::runtime_error(
        "Lattice compare should reject retired identity fields\n" +
        retired_compare_identity.output);
  }

  const std::string lattice_status =
      read_command_stdout(base + "--tool hero.lattice.status --args-json "
                                 "'{}'");
  require_contains(
      lattice_status,
      "\"ok\":true,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,"
      "\"fact_families_are_not_target_kinds\":true",
      "status should expose top-level read-only, non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(lattice_status, "hero.lattice.status");

  const std::string lattice_schema = read_command_stdout(
      base + "--tool hero.lattice.inspect.schema --args-json '{}'");
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
      base + "--tool hero.lattice.inspect.targets --args-json '{}'");
  require_contains(
      target_list,
      "\"schema\":\"kikijyeba.lattice.target_catalog.v1\","
      "\"schema_version\":1,\"read_only\":true,\"target_proof\":false,"
      "\"dispatchable\":false,\"runtime_executor\":false,\"writes_evidence\":"
      "false,\"fact_families_are_not_target_kinds\":true",
      "list_targets should expose top-level read-only, non-proof, "
      "non-dispatchable boundaries");
  require_lattice_non_decision_boundary(target_list,
                                        "hero.lattice.inspect.targets");
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
  require_contains(target_list, "\"proof_template_count\":10",
                   "artifact readiness registry should enumerate proof "
                   "templates");
  require_contains(
      target_list,
      "\"proofable_fact_families\":[\"target_transform\","
      "\"forecast_baseline\",\"forecast_eval\",\"observer_belief\","
      "\"allocation_engine\",\"replay_environment\",\"policy_training\","
      "\"tsodao_settings_protection\",\"policy_acceptance\","
      "\"paper_online_readiness\"]",
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
  require_contains(
      target_list,
      "\"subject_fact_family\":\"tsodao_settings_protection\",\"proof_kind\":"
      "\"tsodao_settings_protection_artifact_bound\"",
      "artifact readiness registry should expose Tsodao settings-protection "
      "proof kind");
  require_contains(
      target_list,
      "\"subject_fact_family\":\"policy_acceptance\",\"proof_kind\":\"policy_"
      "acceptance_contract_bound\"",
      "artifact readiness registry should expose policy acceptance proof kind");
  require_contains(
      target_list,
      "\"subject_fact_family\":\"paper_online_readiness\",\"proof_kind\":"
      "\"paper_online_readiness_contract_bound\"",
      "artifact readiness registry should expose paper-online readiness proof "
      "kind");

  const std::string latest = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "latest_satisfying_checkpoint",
                 "target_id=channel_mdn_train_core_no_test_leakage\n"
                 "runtime_root=" +
                     runtime_root.string() + "\n"));
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
      read_command_stdout(base + lattice_evaluate_call_shell(
                                     "latest_satisfying_checkpoint",
                                     "target_id=forecast_eval_artifact_ready\n"
                                     "runtime_root=" +
                                         runtime_root.string() + "\n"));
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
      base + lattice_inspect_call_shell(
                 "{\"subject\":\"checkpoint\",\"runtime_root\":\"" +
                     runtime_root.string() + "\"}",
                 "checkpoint_id=missing_checkpoint\n"
                 "checkpoint_file_digest=missing_digest\n"));
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
  require_contains(checkpoint_closure, "\"checkpoint_ref\":\"ckpt_missing_dige",
                   "checkpoint_closure should expose a display checkpoint ref "
                   "next to the full digest");
  require_contains(checkpoint_closure, "\"complete\":false",
                   "missing checkpoint identity should fail closed");

  const std::string comparison = read_command_stdout(
      base + "--tool hero.lattice.compare --args-json " +
      lattice_compare_args_shell(
          "left_target_id=channel_mdn_train_core_no_test_leakage\n"
          "right_target_id=channel_mdn_train_core_ready\n"
          "runtime_root=" +
          runtime_root.string() + "\n"));
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
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"summary\",\"runtime_root\":\"" +
              runtime_root.string() + "\"}",
          "family=forecast_eval\n"));
  require_contains(
      fact_summary,
      "\"schema\":\"kikijyeba.lattice.fact_summary.v1\",\"runtime_root\":\"" +
          runtime_root.string() +
          "\",\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
          "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":"
          "true",
      "fact_summary should expose top-level non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(fact_summary,
                                        "hero.lattice.inspect.facts.summary");
  require_fact_catalog_no_decision_authority(
      fact_summary, "hero.lattice.inspect.facts.summary");
  require_fact_identity_contract(fact_summary,
                                 "hero.lattice.inspect.facts.summary");
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
      base + lattice_inspect_call_shell(
                 "{\"subject\":\"fact_families\",\"runtime_root\":\"" +
                 runtime_root.string() + "\"}"));
  require_contains(
      fact_family_registry,
      "\"schema\":\"kikijyeba.lattice.fact_family_registry.v1\","
      "\"read_only\":true,\"target_proof\":false,\"dispatchable\":false,"
      "\"runtime_executor\":false,\"fact_families_are_not_target_kinds\":true",
      "list_fact_families should expose top-level non-proof, non-dispatchable "
      "catalog boundaries");
  require_lattice_non_decision_boundary(fact_family_registry,
                                        "hero.lattice.inspect.fact_families");
  require_fact_catalog_no_decision_authority(
      fact_family_registry, "hero.lattice.inspect.fact_families");
  require_fact_identity_contract(fact_family_registry,
                                 "hero.lattice.inspect.fact_families");
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
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"scan\",\"runtime_root\":\"" +
              runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "include_facts=false\n"));
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
  require_lattice_non_decision_boundary(fact_scan,
                                        "hero.lattice.inspect.facts.scan");
  require_fact_catalog_no_decision_authority(fact_scan,
                                             "hero.lattice.inspect.facts.scan");
  require_fact_identity_contract(fact_scan, "hero.lattice.inspect.facts.scan");
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
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"lineage\",\"runtime_root\":\"" +
              runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "limit=4\n"));
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
  require_lattice_non_decision_boundary(fact_lineage,
                                        "hero.lattice.inspect.facts.lineage");
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
      lattice_inspect_call_shell(
          "{\"subject\":\"index\",\"mode\":\"status\",\"runtime_root\":\"" +
              runtime_root.string() + "\"}",
          "limit=4\n"));
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
      lattice_inspect_call_shell(
          "{\"subject\":\"index\",\"mode\":\"query\",\"runtime_root\":\"" +
              runtime_root.string() + "\"}",
          "relation=forecast_eval\n"
          "limit=4\n"));
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

  const std::string derived_query = read_command_stdout(
      base + lattice_inspect_call_shell(
                 "{\"subject\":\"derived\",\"runtime_root\":\"" +
                     runtime_root.string() + "\"}",
                 "relation=stale_cache\n"
                 "limit=4\n"));
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
      base + lattice_inspect_call_shell(
                 "{\"subject\":\"exposure\",\"runtime_root\":\"" +
                     runtime_root.string() + "\"}",
                 "limit=4\n"));
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
  require_lattice_non_decision_boundary(exposure_scan,
                                        "hero.lattice.inspect.exposure");

  const fs::path source_analytics_runtime_root =
      "/tmp/hero_mcp_schema_compat/lattice_source_analytics_warning";
  write_mtf_source_analytics_warning_fixture(source_analytics_runtime_root);
  const std::string source_analytics_eval = read_command_stdout(
      base +
      lattice_evaluate_call_shell(
          "target", "target_id=mtf_jepa_mae_vicreg_train_core_ready\n"
                    "runtime_root=" +
                        source_analytics_runtime_root.string() +
                        "\n"
                        "protocol_id=cwu_02v\n"
                        "protocol_contract_fingerprint=contract_1\n"
                        "graph_order_fingerprint=graph_1\n"
                        "source_cursor_token=cursor_1|accepted=1000\n"
                        "mtf_jepa_mae_vicreg_assembly_fingerprint=mtf_1\n"));
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

  const std::string artifact_explain = read_command_stdout(
      base +
      lattice_inspect_call_shell("{\"subject\":\"target\"}",
                                 "target_id=forecast_eval_artifact_ready\n"));
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

  const fs::path missing_artifact_runtime_root =
      "/tmp/hero_mcp_schema_compat/lattice_missing_forecast_eval_artifact";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      missing_artifact_runtime_root,
      /*forecast_baseline_digest_mismatch=*/false);
  fs::remove(missing_artifact_runtime_root / "artifact_job" /
             "lattice.forecast_eval.fact");

  const std::string artifact_deficit = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "deficit", "target_id=forecast_eval_artifact_ready\n"
                            "runtime_root=" +
                                missing_artifact_runtime_root.string() + "\n"));
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

  const std::string artifact_evaluation = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "target", "target_id=forecast_eval_artifact_ready\n"
                           "runtime_root=" +
                               missing_artifact_runtime_root.string() + "\n"));
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

  const std::string artifact_bulk_evaluation = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "targets", "target_ids=forecast_eval_artifact_ready\n"
                            "runtime_root=" +
                                missing_artifact_runtime_root.string() + "\n"));
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
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              scanned_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "fact_index=0\n"
          "limit=1\n"));
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
  require_lattice_non_decision_boundary(fact_preview,
                                        "hero.lattice.inspect.facts.preview");
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
  require_contains(fact_preview,
                   "\"short_ref_scheme\":\"kikijyeba.hero.short_ref.v1\"",
                   "fact_preview should declare the short-ref display scheme");
  require_contains(fact_preview, "\"display_digest_prefix_length\":12",
                   "fact_preview should declare the display digest prefix "
                   "length");
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
  require_contains(fact_preview, "\"fact_index\":0,\"fact_digest\":\"",
                   "fact_preview rows should carry fact digest and index");
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

  const std::string selected_fact_digest =
      first_json_string_field(fact_preview, "fact_digest");
  const std::string selected_fact_digest_prefix =
      selected_fact_digest.substr(0, 12);
  const std::string selected_fact_ref = "fev_" + selected_fact_digest_prefix;
  require_contains(fact_preview,
                   "\"fact_digest_prefix\":\"" + selected_fact_digest_prefix +
                       "\",\"fact_ref\":\"" + selected_fact_ref + "\"",
                   "fact_preview rows should include display short refs next "
                   "to full digests");
  const std::string exact_digest_preview = read_command_stdout(
      base +
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              scanned_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "fact_digest=" +
              selected_fact_digest +
              "\n"
              "limit=1\n"));
  require_contains(exact_digest_preview,
                   "\"fact_digest_filter\":\"" + selected_fact_digest + "\"",
                   "fact_preview should expose the exact fact_digest filter");
  require_contains(exact_digest_preview, "\"matching_fact_count\":1",
                   "fact_preview should find exact fact_digest matches");
  require_contains(exact_digest_preview,
                   "\"fact_index\":0,\"fact_digest\":\"" +
                       selected_fact_digest + "\"",
                   "fact_preview should return the exact digest-selected row");
  require_contains(exact_digest_preview,
                   "\"fact_ref\":\"" + selected_fact_ref + "\"",
                   "exact fact_digest preview should keep the display ref");

  const std::string unique_prefix_preview = read_command_stdout(
      base +
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              scanned_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "fact_digest_prefix=" +
              selected_fact_digest_prefix +
              "\n"
              "limit=1\n"));
  require_contains(unique_prefix_preview,
                   "\"fact_digest_prefix_filter\":\"" +
                       selected_fact_digest_prefix + "\"",
                   "fact_preview should expose the canonical prefix filter");
  require_contains(unique_prefix_preview, "\"matching_fact_count\":1",
                   "fact_preview should accept a unique fact_digest_prefix");
  require_contains(unique_prefix_preview,
                   "\"fact_index\":0,\"fact_digest\":\"" +
                       selected_fact_digest + "\"",
                   "unique fact_digest_prefix should resolve to the full "
                   "digest-selected row");
  require_contains(unique_prefix_preview,
                   "\"fact_ref\":\"" + selected_fact_ref + "\"",
                   "unique fact_digest_prefix preview should keep the display "
                   "ref");

  for (const auto &legacy_selector :
       std::vector<std::pair<std::string, std::string>>{
           {"digest", "fact_digest"},
           {"digest_prefix", "fact_digest_prefix"},
           {"index", "fact_index"},
       }) {
    const auto legacy_preview = run_command_capture(
        base +
        lattice_inspect_call_shell(
            "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
                scanned_runtime_root.string() + "\"}",
            "family=forecast_eval\n" + legacy_selector.first + "=x\n"));
    if (legacy_preview.status == 0 ||
        legacy_preview.output.find("unknown field: " + legacy_selector.first) ==
            std::string::npos) {
      throw std::runtime_error("fact preview request should reject retired " +
                               legacy_selector.first + " selector");
    }
  }

  const auto missing_prefix_preview = run_command_capture(
      base +
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              scanned_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "fact_digest_prefix=not_found\n"));
  if (missing_prefix_preview.status == 0 ||
      missing_prefix_preview.output.find("E_LATTICE_FACT_REF_NOT_FOUND") ==
          std::string::npos) {
    throw std::runtime_error(
        "fact preview should fail closed when fact_digest_prefix matches no "
        "facts");
  }

  const fs::path ambiguous_runtime_root =
      "/tmp/hero_mcp_schema_compat/lattice_ambiguous_prefix";
  lattice_fixture::write_scanned_forecast_artifact_fixture(
      ambiguous_runtime_root, /*forecast_baseline_digest_mismatch=*/false);
  for (int candidate_index = 0; candidate_index < 32; ++candidate_index) {
    append_forecast_eval_prefix_candidate_fixture(ambiguous_runtime_root,
                                                 candidate_index);
  }
  const std::string ambiguous_all_preview = read_command_stdout(
      base +
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              ambiguous_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "limit=64\n"));
  std::vector<std::string> unique_digests;
  for (const auto &digest :
       json_string_field_values(ambiguous_all_preview, "fact_digest")) {
    if (!digest.empty() &&
        std::find(unique_digests.begin(), unique_digests.end(), digest) ==
            unique_digests.end()) {
      unique_digests.push_back(digest);
    }
  }
  std::array<int, 256> prefix_counts{};
  for (const auto &digest : unique_digests) {
    ++prefix_counts[static_cast<unsigned char>(digest.front())];
  }
  std::string ambiguous_prefix;
  for (std::size_t i = 0; i < prefix_counts.size(); ++i) {
    if (prefix_counts[i] > 1) {
      ambiguous_prefix.push_back(static_cast<char>(i));
      break;
    }
  }
  if (ambiguous_prefix.empty()) {
    throw std::runtime_error(
        "failed to synthesize ambiguous canonical forecast_eval digest prefix");
  }
  const auto ambiguous_prefix_preview = run_command_capture(
      base +
      lattice_inspect_call_shell(
          "{\"subject\":\"facts\",\"mode\":\"preview\",\"runtime_root\":\"" +
              ambiguous_runtime_root.string() + "\"}",
          "family=forecast_eval\n"
          "fact_digest_prefix=" +
              ambiguous_prefix + "\n"));
  if (ambiguous_prefix_preview.status == 0 ||
      ambiguous_prefix_preview.output.find("E_LATTICE_FACT_REF_AMBIGUOUS") ==
          std::string::npos) {
    throw std::runtime_error(
        "fact preview should fail closed when fact_digest_prefix is "
        "ambiguous");
  }

  const std::string scanned_baseline_evaluation = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "target", "target_id=forecast_baseline_artifact_ready\n"
                           "runtime_root=" +
                               scanned_runtime_root.string() +
                               "\n"
                               "protocol_contract_fingerprint=contract_1\n"
                               "graph_order_fingerprint=graph_1\n"
                               "source_cursor_token=cursor_1|accepted=1000\n"
                               "mdn_assembly_fingerprint=mdn_1\n"));
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

  const std::string scanned_observer_evaluation = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "target", "target_id=observer_belief_artifact_ready\n"
                           "runtime_root=" +
                               scanned_runtime_root.string() +
                               "\n"
                               "protocol_contract_fingerprint=contract_1\n"
                               "graph_order_fingerprint=graph_1\n"
                               "source_cursor_token=cursor_1|accepted=1000\n"
                               "mdn_assembly_fingerprint=mdn_1\n"));
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
      base + lattice_evaluate_call_shell(
                 "target", "target_id=allocation_artifact_ready\n"
                           "runtime_root=" +
                               scanned_runtime_root.string() +
                               "\n"
                               "protocol_contract_fingerprint=contract_1\n"
                               "graph_order_fingerprint=graph_1\n"
                               "source_cursor_token=cursor_1|accepted=1000\n"
                               "mdn_assembly_fingerprint=mdn_1\n"));
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

  const std::string scanned_artifact_evaluation = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "target", "target_id=forecast_eval_artifact_ready\n"
                           "runtime_root=" +
                               scanned_runtime_root.string() +
                               "\n"
                               "protocol_contract_fingerprint=contract_1\n"
                               "graph_order_fingerprint=graph_1\n"
                               "source_cursor_token=cursor_1|accepted=1000\n"
                               "mdn_assembly_fingerprint=mdn_1\n"));
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
  require_contains(scanned_artifact_evaluation, "\"certificate_ref\":\"cert_",
                   "scanned artifact proof should expose a display certificate "
                   "ref");
  require_contains(scanned_artifact_evaluation, "\"root_checkpoint_ref\":\"",
                   "scanned artifact proof should expose the checkpoint ref "
                   "display field");
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

  const std::string scanned_artifact_bulk = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "targets", "target_ids=forecast_eval_artifact_ready\n"
                            "runtime_root=" +
                                scanned_runtime_root.string() +
                                "\n"
                                "protocol_contract_fingerprint=contract_1\n"
                                "graph_order_fingerprint=graph_1\n"
                                "source_cursor_token=cursor_1|accepted=1000\n"
                                "mdn_assembly_fingerprint=mdn_1\n"));
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
  const std::string scanned_bad_baseline = read_command_stdout(
      base + lattice_evaluate_call_shell(
                 "target", "target_id=forecast_eval_artifact_ready\n"
                           "runtime_root=" +
                               scanned_bad_baseline_root.string() +
                               "\n"
                               "protocol_contract_fingerprint=contract_1\n"
                               "graph_order_fingerprint=graph_1\n"
                               "source_cursor_token=cursor_1|accepted=1000\n"
                               "mdn_assembly_fingerprint=mdn_1\n"));
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
                   "true,\"tool\":\"hero.lattice.inspect.facts.preview.by_"
                   "digest\"",
                   "scanned bad-baseline artifact proof should point to fact "
                   "preview");
  require_contains(scanned_bad_baseline,
                   "\"marshal_tool\":\"hero.marshal.inspect.facts.preview.by_"
                   "digest\","
                   "\"fact_family\":\"forecast_eval\",\"fact_ref\":\"fev_",
                   "scanned bad-baseline artifact preview hint should include "
                   "family and display ref");
  require_contains(scanned_bad_baseline, "\"fact_digest_prefix\":\"",
                   "scanned bad-baseline artifact preview hint should include "
                   "the display digest prefix");
  require_contains(scanned_bad_baseline, "\"fact_digest\":\"",
                   "scanned bad-baseline artifact preview hint should retain "
                   "the full digest");
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
  const auto environment_binary = first_existing(
      {hero_root / "hero_environment.mcp", hero_root / "hero_environment_mcp"});
  check_catalog("Environment", environment_binary, false);
  check_environment_inspect_derouted_surface(environment_binary);
  const auto lattice_binary = first_existing(
      {hero_root / "hero_lattice.mcp", hero_root / "hero_lattice_mcp"});
  check_catalog("Lattice", lattice_binary, true);
  check_lattice_selector_boundaries(lattice_binary);
  const auto marshal_binary = first_existing(
      {hero_root / "hero_marshal.mcp", hero_root / "hero_marshal_mcp"});
  check_catalog("Marshal", marshal_binary, false);
  check_marshal_derouted_surface(marshal_binary);

  return 0;
}
