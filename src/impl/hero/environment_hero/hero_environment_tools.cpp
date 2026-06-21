#include "hero/environment_hero/hero_environment_tools.h"

#include "hero/config_path_defaults.h"
#include "hero/environment_hero/hero_environment.h"
#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/marshal_hero/marshal/digest.h"
#include "hero/marshal_hero/marshal/tool_handler.h"
#include "hero/mcp_cli_client.h"
#include "hero/mcp_schema_compat.h"
#include "hero/mcp_stdio_transport.h"
#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/short_ref.h"
#include "kikijyeba/environment/paper_online_session_contract.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cuwacunu::hero::environment {
namespace {

namespace fs = std::filesystem;
namespace exposure = cuwacunu::hero::lattice::exposure;
namespace kenv = cuwacunu::kikijyeba::environment;
namespace marshal = cuwacunu::hero::marshal;
namespace md = cuwacunu::hero::marshal::tool_detail;
namespace display = cuwacunu::hero::display;

constexpr int kPolicyErrorCode = -32070;
constexpr const char *kProtocolLayerStdio = "STDIO";
constexpr const char *kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol_layer is reserved and not implemented; use STDIO";
constexpr const char *kPolicyAcceptancePrimaryMetricId =
    "after_cost_total_log_growth_delta";
constexpr const char *kPolicyAcceptancePrimaryMetricDirection = "at_or_above";
constexpr const char *kPolicyAcceptanceUncertaintyPolicy =
    "block_bootstrap_required";
constexpr const char *kPolicyAcceptanceUncertaintyModel =
    "validation_block_bootstrap_v1";
constexpr const char *kPolicyAcceptanceSelectorSplit = "validation";
constexpr const char *kPolicyAcceptanceValidationSplit = "validation_holdout";
constexpr const char *kPolicyAcceptanceTestSplit = "sealed_test";
constexpr const char *kPolicyAcceptanceSealedTestPolicy =
    "sealed_until_acceptance";
constexpr const char *kPolicyAcceptanceTiePolicy = "reject_ties";
constexpr const char *kPaperOnlineEnvironmentContractId =
    "kikijyeba.environment.paper_online.v1";
constexpr const char *kPaperOnlineDirectEdgeUniverseValidationPolicyId =
    "direct_graph_node_pair_universe_required.v1";
constexpr const char *kPaperOnlineMissingDirectPairPolicy = "skip_pair_warn";
constexpr const char *kPaperOnlineSyntheticExecutionMarketPolicyId =
    "synthetic_execution_markets_forbidden.v1";
constexpr const char *kPaperOnlineSessionStateSchemaId =
    "kikijyeba.paper_online.session_state.v1";
constexpr const char *kPaperOnlinePersistentLedgerRecoveryPolicyId =
    "recover_persistent_paper_ledger_before_session.v1";
constexpr const char *kPaperOnlineLedgerStoragePolicyId =
    "durable_paper_ledger_artifacts.v1";
constexpr const char *kPaperOnlineSessionLifecyclePolicyId =
    "operator_start_stop_required.v1";
constexpr const char *kPaperOnlineIdempotencyPolicyId =
    "execution_intent_idempotency.v1";
constexpr const char *kPaperOnlineExecutionIntentIdPolicy =
    "stable_intent_id_from_target_digest.v1";
constexpr const char *kPaperOnlineDuplicateActionPolicy =
    "reject_duplicate_action_digest.v1";
constexpr const char *kPaperOnlineDuplicateExecutionIntentPolicy =
    "reject_duplicate_execution_intent_id.v1";
constexpr const char *kPaperOnlineClockPolicyId =
    "monotonic_runtime_clock_required.v1";
constexpr const char *kPaperOnlineTimestampPolicyId =
    "exchange_event_time_then_runtime_receive.v1";
constexpr const char *kPaperOnlineMarketDataStalenessPolicyId =
    "max_receive_age_before_action.v1";
constexpr const char *kPaperOnlineRewardReportArtifactPolicyId =
    "durable_reward_and_report_artifacts.v1";
constexpr const char *kPaperOnlineOperatorAbortPolicyId =
    "operator_abort_closes_session.v1";
constexpr const char *kPaperOnlineKillSwitchPolicyId =
    "kill_switch_no_new_intents.v1";

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

constexpr tool_descriptor_t kTools[] = {
#define HERO_ENVIRONMENT_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON)            \
  tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON},
#include "hero/environment_hero/hero_environment.def"
#undef HERO_ENVIRONMENT_TOOL
};

struct policy_descriptor_t {
  const char *key;
  const char *type;
  const char *default_value;
  const char *summary;
};

constexpr policy_descriptor_t kPolicyDescriptors[] = {
    {"protocol_layer", "enum", "STDIO",
     "MCP protocol layer. HTTPS/SSE is reserved and fails fast."},
    {"environment_profile", "enum", "operator_default",
     "Named Environment policy profile."},
    {"default_config_path", "path", ".config",
     "Default global config path for delegated Runtime work."},
    {"runtime_root", "path", "../../.runtime/cuwacunu_exec",
     "Runtime evidence root inspected by Environment admission checks."},
    {"allowed_job_roots", "path_list", "../../.runtime/cuwacunu_exec,/tmp",
     "Roots under which Environment may read/write job-local evidence."},
    {"allow_certify_issue", "bool", "true",
     "Allow Environment certification to issue validated sidecars."},
    {"allow_rollout_replay", "bool", "true",
     "Allow Environment rollout to delegate low-level Runtime replay."},
    {"allow_paper_online_session_run", "bool", "true",
     "Allow Environment to run bounded paper-only online sessions after "
     "validated admission."},
    {"paper_online_session_max_steps", "int", "64",
     "Maximum positive finite step cap for paper-online session runs."},
    {"max_capture_bytes", "int", "65536",
     "Maximum delegated Runtime result payload retained inline."},
    {"max_runtime_seconds", "int", "86400",
     "Default timeout for delegated Runtime replay."},
    {"rollout_policy_set", "string",
     "numeraire,current_weight,equal_weight,sdu",
     "Default Environment rollout policy set."},
    {"rollout_max_steps", "int", "250",
     "Default positive finite Environment rollout step cap."},
    {"rollout_max_parallel_jobs", "int", "4",
     "Default positive finite Environment rollout worker cap."},
    {"rollout_runtime_exec_path", "path", "../../.build/exec/cuwacunu_exec",
     "Runtime executable used for delegated rollout replay."},
    {"rollout_execution_backend_id", "string", "cajtucu.execution.paper.v1",
     "Execution backend identity for Environment rollout replay."},
    {"rollout_cost_model_id", "string", "linear_transaction_cost_rate.v1",
     "Cost model identity for Environment rollout replay."},
    {"rollout_allow_synthetic_direct_edges", "bool", "false",
     "Whether Environment rollout replay may use synthetic direct markets."},
    {"rollout_synthetic_edge_research_reason", "string", "",
     "Research reason required when synthetic direct markets are enabled."},
    {"rollout_linear_transaction_cost_rate", "number", "0.001",
     "Linear transaction cost rate for Environment rollout replay."},
    {"rollout_allow_partial_fills", "bool", "false",
     "Whether Environment rollout replay may model partial fills."},
    {"rollout_equity_mismatch_tolerance", "number", "0.000001",
     "Warning tolerance for rollout equity mismatch checks."},
    {"rollout_equity_mismatch_fail_tolerance", "number", "0.01",
     "Failure tolerance for rollout equity mismatch checks."},
    {"rollout_live_execution_allowed", "bool", "false",
     "Environment rollout replay must not authorize live capital by default."},
};

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string json_quote(std::string_view value) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : value) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
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
      if (c < 0x20) {
        static constexpr char kHex[] = "0123456789abcdef";
        out << "\\u00" << kHex[(c >> 4U) & 0x0f] << kHex[c & 0x0f];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] const char *bool_json(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] bool parse_bool(std::string_view raw, bool *out) {
  const std::string value = lowercase_ascii(trim_ascii(raw));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    if (out != nullptr) {
      *out = true;
    }
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    if (out != nullptr) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int(std::string_view raw, int *out) {
  try {
    const int value = std::stoi(trim_ascii(raw));
    if (out != nullptr) {
      *out = value;
    }
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

[[nodiscard]] std::string
string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string join_strings(const std::vector<std::string> &values,
                                       std::string_view separator) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << separator;
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] std::optional<std::string>
read_ini_value(const fs::path &ini_path, std::string_view section,
               std::string_view key) {
  std::ifstream input(ini_path);
  if (!input.is_open()) {
    return std::nullopt;
  }
  std::string current_section;
  std::string line;
  while (std::getline(input, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      current_section =
          trim_ascii(std::string_view(line).substr(1, line.size() - 2U));
      continue;
    }
    if (current_section != section) {
      continue;
    }
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    const auto found_key = trim_ascii(std::string_view(line).substr(0, pos));
    if (found_key == key) {
      return trim_ascii(std::string_view(line).substr(pos + 1U));
    }
  }
  return std::nullopt;
}

[[nodiscard]] fs::path resolve_against(const fs::path &base_file,
                                       const std::string &raw) {
  fs::path path(trim_ascii(raw));
  if (path.is_relative()) {
    path = base_file.parent_path() / path;
  }
  return path.lexically_normal();
}

[[nodiscard]] bool read_text_file(const fs::path &path, std::string *out,
                                  std::string *error) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "failed to open file: " + path.string();
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (out != nullptr) {
    *out = buffer.str();
  }
  return true;
}

[[nodiscard]] std::string normalize_key(std::string left) {
  left = trim_ascii(left);
  const auto colon = left.find(':');
  const auto bracket = left.find('[');
  std::size_t end = std::string::npos;
  if (colon != std::string::npos) {
    end = colon;
  }
  if (bracket != std::string::npos) {
    end = end == std::string::npos ? bracket : std::min(end, bracket);
  }
  if (end != std::string::npos) {
    left.resize(end);
  }
  return trim_ascii(left);
}

struct parsed_policy_text_t {
  std::unordered_map<std::string, std::string> base{};
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      profiles{};
};

[[nodiscard]] bool parse_policy_text(std::string_view text,
                                     parsed_policy_text_t *out,
                                     std::string *error) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = "policy parse output pointer is null";
    }
    return false;
  }
  std::istringstream input{std::string(text)};
  std::string line;
  std::string active_profile;
  bool in_profile = false;
  while (std::getline(input, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    if (!in_profile && line.rfind("ENVIRONMENT_PROFILE ", 0) == 0) {
      const auto brace = line.find('{');
      if (brace == std::string::npos) {
        if (error != nullptr) {
          *error = "ENVIRONMENT_PROFILE block missing opening brace";
        }
        return false;
      }
      active_profile =
          trim_ascii(std::string_view(line).substr(20U, brace - 20U));
      if (active_profile.empty()) {
        if (error != nullptr) {
          *error = "ENVIRONMENT_PROFILE id is empty";
        }
        return false;
      }
      in_profile = true;
      out->profiles[active_profile];
      continue;
    }
    if (in_profile && line == "}") {
      active_profile.clear();
      in_profile = false;
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key =
        normalize_key(std::string(std::string_view(line).substr(0, eq)));
    std::string value = trim_ascii(std::string_view(line).substr(eq + 1U));
    if (!value.empty() && value.back() == ';') {
      value.pop_back();
      value = trim_ascii(value);
    }
    if (key.empty()) {
      continue;
    }
    if (in_profile) {
      if (key == "environment_profile") {
        if (error != nullptr) {
          *error = "ENVIRONMENT_PROFILE block must not set environment_profile";
        }
        return false;
      }
      out->profiles[active_profile][std::move(key)] = std::move(value);
    } else {
      out->base[std::move(key)] = std::move(value);
    }
  }
  if (in_profile) {
    if (error != nullptr) {
      *error = "unterminated ENVIRONMENT_PROFILE block";
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string policy_get(const environment_policy_t &policy,
                                     std::string_view key) {
  const auto found = policy.values.find(std::string(key));
  return found == policy.values.end() ? std::string{} : found->second;
}

[[nodiscard]] bool policy_bool_or(const environment_policy_t &policy,
                                  std::string_view key, bool fallback) {
  bool parsed = false;
  const std::string value = policy_get(policy, key);
  if (value.empty() || !parse_bool(value, &parsed)) {
    return fallback;
  }
  return parsed;
}

[[nodiscard]] int policy_int_or(const environment_policy_t &policy,
                                std::string_view key, int fallback) {
  int parsed = 0;
  const std::string value = policy_get(policy, key);
  if (value.empty() || !parse_int(value, &parsed)) {
    return fallback;
  }
  return parsed;
}

[[nodiscard]] std::int64_t policy_i64_or(const environment_policy_t &policy,
                                         std::string_view key,
                                         std::int64_t fallback) {
  try {
    const std::string value = trim_ascii(policy_get(policy, key));
    if (value.empty()) {
      return fallback;
    }
    std::size_t parsed_count = 0;
    const std::int64_t parsed = std::stoll(value, &parsed_count, 10);
    if (parsed_count != value.size()) {
      return fallback;
    }
    return parsed;
  } catch (const std::exception &) {
    return fallback;
  }
}

[[nodiscard]] double policy_double_or(const environment_policy_t &policy,
                                      std::string_view key, double fallback) {
  try {
    const std::string value = trim_ascii(policy_get(policy, key));
    if (value.empty()) {
      return fallback;
    }
    std::size_t parsed_count = 0;
    const double parsed = std::stod(value, &parsed_count);
    if (parsed_count != value.size()) {
      return fallback;
    }
    return parsed;
  } catch (const std::exception &) {
    return fallback;
  }
}

[[nodiscard]] std::vector<fs::path> split_path_list(std::string_view raw) {
  std::vector<fs::path> out;
  std::string item;
  std::istringstream input{std::string(raw)};
  while (std::getline(input, item, ',')) {
    item = trim_ascii(item);
    if (!item.empty()) {
      out.emplace_back(item);
    }
  }
  return out;
}

[[nodiscard]] fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  fs::path absolute = path.is_absolute() ? path : fs::absolute(path, ec);
  if (ec) {
    absolute = path;
  }
  fs::path weak = fs::weakly_canonical(absolute, ec);
  if (!ec) {
    return weak;
  }
  return absolute.lexically_normal();
}

[[nodiscard]] bool path_within(const fs::path &root, const fs::path &child) {
  const fs::path normalized_root = normalize_path(root);
  const fs::path normalized_child = normalize_path(child);
  auto root_it = normalized_root.begin();
  auto child_it = normalized_child.begin();
  for (; root_it != normalized_root.end() && child_it != normalized_child.end();
       ++root_it, ++child_it) {
    if (*root_it != *child_it) {
      return false;
    }
  }
  return root_it == normalized_root.end();
}

[[nodiscard]] fs::path policy_path_or(const environment_policy_t &policy,
                                      std::string_view key,
                                      const fs::path &fallback) {
  const std::string raw = trim_ascii(policy_get(policy, key));
  if (raw.empty()) {
    return fallback.empty()
               ? fallback
               : resolve_against(policy.policy_path, fallback.string());
  }
  return resolve_against(policy.policy_path, raw);
}

[[nodiscard]] fs::path runtime_root(const environment_policy_t &policy) {
  return normalize_path(policy_path_or(policy, "runtime_root", {}));
}

[[nodiscard]] std::string
resolved_path_list_text(const environment_policy_t &policy,
                        std::string_view key) {
  std::ostringstream out;
  bool first = true;
  for (const auto &root : split_path_list(policy_get(policy, key))) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << resolve_against(policy.policy_path, root.string()).string();
  }
  return out.str();
}

[[nodiscard]] bool explicit_job_dir_allowed(const environment_policy_t &policy,
                                            const fs::path &path) {
  for (const auto &root :
       split_path_list(policy_get(policy, "allowed_job_roots"))) {
    if (path_within(resolve_against(policy.policy_path, root.string()), path)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool job_path_allowed(const environment_policy_t &policy,
                                    const fs::path &path) {
  const fs::path normalized = normalize_path(path);
  return explicit_job_dir_allowed(policy, normalized) ||
         path_within(runtime_root(policy), normalized);
}

[[nodiscard]] bool
required_string(const std::map<std::string, std::string> &fields,
                const std::string &key, std::string *out, std::string *err);

enum class certify_evidence_value_kind_t {
  string_value,
  bool_value,
  integer_value,
  number_value,
};

struct certify_evidence_field_t {
  const char *key;
  certify_evidence_value_kind_t kind;
};

constexpr certify_evidence_field_t kPolicyAcceptanceEvidenceFields[] = {
    {"policy_training_job_dir", certify_evidence_value_kind_t::string_value},
    {"tsodao_settings_protection_job_dir",
     certify_evidence_value_kind_t::string_value},
    {"acceptance_id", certify_evidence_value_kind_t::string_value},
    {"accepted_policy_training_proof_certificate_digest",
     certify_evidence_value_kind_t::string_value},
    {"tsodao_settings_protection_proof_certificate_digest",
     certify_evidence_value_kind_t::string_value},
    {"primary_metric_value", certify_evidence_value_kind_t::number_value},
    {"accepted_policy_training_ready",
     certify_evidence_value_kind_t::bool_value},
    {"tsodao_settings_protection_ready",
     certify_evidence_value_kind_t::bool_value},
    {"protected_settings_bound", certify_evidence_value_kind_t::bool_value},
    {"mandatory_baselines_bound", certify_evidence_value_kind_t::bool_value},
    {"mandatory_baselines_passed", certify_evidence_value_kind_t::bool_value},
    {"after_cost_metrics_bound", certify_evidence_value_kind_t::bool_value},
    {"primary_metric_passed", certify_evidence_value_kind_t::bool_value},
    {"uncertainty_policy_bound", certify_evidence_value_kind_t::bool_value},
    {"uncertainty_passed", certify_evidence_value_kind_t::bool_value},
    {"selector_split_bound", certify_evidence_value_kind_t::bool_value},
    {"validation_test_disjoint", certify_evidence_value_kind_t::bool_value},
    {"test_sealed_until_acceptance", certify_evidence_value_kind_t::bool_value},
    {"negative_tests_bound", certify_evidence_value_kind_t::bool_value},
    {"negative_tests_passed", certify_evidence_value_kind_t::bool_value},
    {"leakage_negative_tests_passed",
     certify_evidence_value_kind_t::bool_value},
    {"threshold_selection_audit_bound",
     certify_evidence_value_kind_t::bool_value},
    {"threshold_selected_before_test",
     certify_evidence_value_kind_t::bool_value},
    {"tie_policy_bound", certify_evidence_value_kind_t::bool_value},
    {"tie_policy_passed", certify_evidence_value_kind_t::bool_value},
    {"cost_slippage_assumptions_bound",
     certify_evidence_value_kind_t::bool_value},
    {"promotion_criteria_bound", certify_evidence_value_kind_t::bool_value},
    {"promotion_criteria_satisfied", certify_evidence_value_kind_t::bool_value},
    {"policy_acceptance_decision", certify_evidence_value_kind_t::bool_value},
};

constexpr certify_evidence_field_t kPaperOnlineReadinessEvidenceFields[] = {
    {"policy_acceptance_job_dir", certify_evidence_value_kind_t::string_value},
    {"readiness_id", certify_evidence_value_kind_t::string_value},
    {"policy_acceptance_proof_certificate_digest",
     certify_evidence_value_kind_t::string_value},
    {"paper_online_profile_digest",
     certify_evidence_value_kind_t::string_value},
    {"direct_edge_universe_digest",
     certify_evidence_value_kind_t::string_value},
    {"max_market_data_staleness_ms",
     certify_evidence_value_kind_t::integer_value},
    {"clock_skew_tolerance_ms", certify_evidence_value_kind_t::integer_value},
    {"policy_acceptance_ready", certify_evidence_value_kind_t::bool_value},
    {"tsodao_settings_protection_ready",
     certify_evidence_value_kind_t::bool_value},
    {"accepted_policy_bound", certify_evidence_value_kind_t::bool_value},
    {"protected_settings_bound", certify_evidence_value_kind_t::bool_value},
    {"direct_edge_universe_validated",
     certify_evidence_value_kind_t::bool_value},
    {"locked_execution_profile_bound",
     certify_evidence_value_kind_t::bool_value},
    {"persistent_paper_ledger_recovery_bound",
     certify_evidence_value_kind_t::bool_value},
    {"idempotency_bound", certify_evidence_value_kind_t::bool_value},
    {"duplicate_action_protection_bound",
     certify_evidence_value_kind_t::bool_value},
    {"session_lifecycle_bound", certify_evidence_value_kind_t::bool_value},
    {"clock_timestamp_policy_bound", certify_evidence_value_kind_t::bool_value},
    {"market_data_staleness_bound", certify_evidence_value_kind_t::bool_value},
    {"reward_report_artifact_path_bound",
     certify_evidence_value_kind_t::bool_value},
    {"operator_abort_bound", certify_evidence_value_kind_t::bool_value},
    {"kill_switch_bound", certify_evidence_value_kind_t::bool_value},
};

[[nodiscard]] const certify_evidence_field_t *
find_certify_evidence_field(std::string_view subject, std::string_view key) {
  const auto find_in =
      [key](const auto &fields) -> const certify_evidence_field_t * {
    for (const auto &field : fields) {
      if (key == std::string_view(field.key)) {
        return &field;
      }
    }
    return nullptr;
  };
  if (subject == "policy_acceptance") {
    return find_in(kPolicyAcceptanceEvidenceFields);
  }
  if (subject == "paper_online_readiness") {
    return find_in(kPaperOnlineReadinessEvidenceFields);
  }
  return nullptr;
}

[[nodiscard]] std::string
certify_preview_digest_domain(std::string_view subject) {
  if (subject == "policy_acceptance") {
    return "kikijyeba.environment.certify_preview.policy_acceptance.v1";
  }
  if (subject == "paper_online_readiness") {
    return "kikijyeba.environment.certify_preview.paper_online_readiness.v1";
  }
  if (subject == "paper_online_session_admission") {
    return "kikijyeba.environment.certify_preview.paper_online_session_"
           "admission.v1";
  }
  return {};
}

[[nodiscard]] std::string certify_preview_digest(std::string_view subject,
                                                 std::string_view preview) {
  return cuwacunu::hero::marshal::marshal_digest_for_text(
      certify_preview_digest_domain(subject), std::string(preview));
}

[[nodiscard]] bool parse_integer_full(std::string_view raw, std::int64_t *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  try {
    std::size_t parsed_count = 0;
    const std::int64_t parsed = std::stoll(value, &parsed_count, 10);
    if (parsed_count != value.size()) {
      return false;
    }
    if (out != nullptr) {
      *out = parsed;
    }
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

[[nodiscard]] bool parse_double_full(std::string_view raw, double *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  try {
    std::size_t parsed_count = 0;
    const double parsed = std::stod(value, &parsed_count);
    if (parsed_count != value.size() || !std::isfinite(parsed)) {
      return false;
    }
    if (out != nullptr) {
      *out = parsed;
    }
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

[[nodiscard]] bool
normalize_certify_evidence_value(const certify_evidence_field_t &field,
                                 std::string_view raw_value,
                                 std::string *out_json_raw, std::string *err) {
  const std::string value = trim_ascii(raw_value);
  switch (field.kind) {
  case certify_evidence_value_kind_t::string_value: {
    if (!value.empty() && value.front() == '"') {
      try {
        *out_json_raw = json_quote(md::parse_string_raw(value, field.key));
      } catch (const std::exception &ex) {
        *err = ex.what();
        return false;
      }
    } else {
      *out_json_raw = json_quote(value);
    }
    return true;
  }
  case certify_evidence_value_kind_t::bool_value: {
    bool parsed = false;
    if (!parse_bool(value, &parsed)) {
      *err = std::string(field.key) + " must be boolean";
      return false;
    }
    *out_json_raw = bool_json(parsed);
    return true;
  }
  case certify_evidence_value_kind_t::integer_value: {
    std::int64_t parsed = 0;
    if (!parse_integer_full(value, &parsed)) {
      *err = std::string(field.key) + " must be integer";
      return false;
    }
    *out_json_raw = std::to_string(parsed);
    return true;
  }
  case certify_evidence_value_kind_t::number_value: {
    double parsed = 0.0;
    if (!parse_double_full(value, &parsed)) {
      *err = std::string(field.key) + " must be finite number";
      return false;
    }
    std::ostringstream json_number;
    json_number << std::setprecision(17) << parsed;
    *out_json_raw = json_number.str();
    return true;
  }
  }
  *err = std::string(field.key) + " has unsupported certify evidence type";
  return false;
}

[[nodiscard]] std::map<std::string, std::string>
object_fields_no_duplicates(const std::string &json, std::string_view label) {
  std::map<std::string, std::string> fields;
  std::size_t idx = 0;
  md::skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    throw std::runtime_error("expected JSON object");
  }
  ++idx;
  while (idx < json.size()) {
    md::skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      md::skip_ws(json, &idx);
      if (idx != json.size()) {
        throw std::runtime_error("trailing content after JSON object");
      }
      return fields;
    }
    std::string key;
    if (!md::parse_json_string_token(json, &idx, &key)) {
      throw std::runtime_error("expected JSON object key");
    }
    if (fields.find(key) != fields.end()) {
      throw std::runtime_error(
          "E_ENVIRONMENT_CERTIFY_EVIDENCE_DUPLICATE_FIELD: " + key);
    }
    md::skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      throw std::runtime_error("expected ':' after JSON object key");
    }
    ++idx;
    md::skip_ws(json, &idx);
    const std::size_t begin = idx;
    if (!md::skip_json_value(json, &idx)) {
      throw std::runtime_error("invalid JSON value for key " + key);
    }
    fields.emplace(std::move(key), trim_ascii(std::string_view(json).substr(
                                       begin, idx - begin)));
    md::skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      continue;
    }
    throw std::runtime_error("expected ',' or '}' in JSON object");
  }
  throw std::runtime_error(std::string(label) + " unterminated JSON object");
}

[[nodiscard]] bool certify_direct_field_set(std::string_view subject,
                                            std::set<std::string> *out,
                                            std::string *err) {
  if (out == nullptr) {
    *err = "certify direct field set output is null";
    return false;
  }
  out->clear();
  if (subject == "policy_acceptance") {
    *out = {"policy_training_job_dir", "tsodao_settings_protection_job_dir",
            "acceptance_id"};
    return true;
  }
  if (subject == "paper_online_readiness") {
    *out = {"policy_acceptance_job_dir", "readiness_id"};
    return true;
  }
  *err = "unknown Environment certify subject";
  return false;
}

[[nodiscard]] bool append_direct_certify_string_field(
    const std::map<std::string, std::string> &args, const std::string &key,
    bool required, std::map<std::string, std::string> *out, std::string *err) {
  if (required) {
    std::string value;
    if (!required_string(args, key, &value, err)) {
      return false;
    }
    (*out)[key] = json_quote(value);
    return true;
  }
  const auto found = args.find(key);
  if (found == args.end()) {
    return true;
  }
  try {
    (*out)[key] = json_quote(md::parse_string_raw(found->second, key));
  } catch (const std::exception &ex) {
    *err = ex.what();
    return false;
  }
  return true;
}

[[nodiscard]] bool append_certification_evidence_fields(
    const std::map<std::string, std::string> &args, std::string_view subject,
    const std::set<std::string> &direct_fields,
    std::map<std::string, std::string> *out, std::string *err) {
  const auto raw = md::optional_raw(args, "certification_evidence");
  if (!raw.has_value()) {
    *err = "missing required field: certification_evidence";
    return false;
  }
  std::map<std::string, std::string> evidence;
  try {
    evidence = object_fields_no_duplicates(*raw, "certification_evidence");
  } catch (const std::exception &ex) {
    *err =
        std::string("certification_evidence must be an object: ") + ex.what();
    return false;
  }
  for (const auto &[key, raw_value] : evidence) {
    if (direct_fields.find(key) != direct_fields.end()) {
      *err = "certification_evidence must not include direct field: " + key;
      return false;
    }
    const auto *field = find_certify_evidence_field(subject, key);
    if (field == nullptr) {
      *err = "E_ENVIRONMENT_CERTIFY_EVIDENCE_UNKNOWN_FIELD: " + key;
      return false;
    }
    std::string normalized;
    if (!normalize_certify_evidence_value(*field, raw_value, &normalized,
                                          err)) {
      return false;
    }
    out->emplace(key, std::move(normalized));
  }
  return true;
}

[[nodiscard]] bool certify_fields_from_direct_args(
    const std::map<std::string, std::string> &args, std::string_view subject,
    std::map<std::string, std::string> *out, std::string *err) {
  if (out == nullptr) {
    *err = "certify field output is null";
    return false;
  }
  out->clear();
  std::set<std::string> direct_fields;
  if (!certify_direct_field_set(subject, &direct_fields, err)) {
    return false;
  }
  if (subject == "policy_acceptance") {
    if (!append_direct_certify_string_field(args, "policy_training_job_dir",
                                            true, out, err) ||
        !append_direct_certify_string_field(
            args, "tsodao_settings_protection_job_dir", false, out, err) ||
        !append_direct_certify_string_field(args, "acceptance_id", true, out,
                                            err)) {
      return false;
    }
  } else if (subject == "paper_online_readiness") {
    if (!append_direct_certify_string_field(args, "policy_acceptance_job_dir",
                                            true, out, err) ||
        !append_direct_certify_string_field(args, "readiness_id", true, out,
                                            err)) {
      return false;
    }
  }
  return append_certification_evidence_fields(args, subject, direct_fields, out,
                                              err);
}

[[nodiscard]] bool rollout_request_from_direct_args(
    const std::map<std::string, std::string> &args,
    std::string_view environment_requested_mode, environment_context_t *ctx,
    marshal::marshal_rollout_request_t *out, std::string *err) {
  if (ctx == nullptr || out == nullptr) {
    *err = "Environment rollout request context is null";
    return false;
  }
  try {
    marshal::marshal_rollout_request_t request{};
    request.requested_mode =
        environment_requested_mode == "replay" ? "execute" : "plan";
    request.config_path =
        policy_path_or(ctx->policy, "default_config_path",
                       ctx->global_config_path.empty()
                           ? config_paths::default_global_config_path()
                           : ctx->global_config_path);
    request.runtime_job_dir =
        fs::path(md::optional_string(args, "runtime_job_dir"));
    request.rollout_id = md::optional_string(args, "rollout_id");
    request.rollout_attempt_id =
        md::optional_string(args, "rollout_attempt_id");
    request.idempotency_key = request.rollout_attempt_id;
    request.experiment_id = request.rollout_id;
    request.target_node_ids =
        md::optional_string_array(args, "target_node_ids");
    request.policy_tokens = md::split_assignment_string_list(
        policy_get(ctx->policy, "rollout_policy_set"));
    request.max_steps =
        policy_i64_or(ctx->policy, "rollout_max_steps", request.max_steps);
    request.max_parallel_jobs = policy_i64_or(
        ctx->policy, "rollout_max_parallel_jobs", request.max_parallel_jobs);
    request.timeout_seconds =
        policy_int_or(ctx->policy, "max_runtime_seconds", 600);
    request.runtime_exec_path =
        policy_path_or(ctx->policy, "rollout_runtime_exec_path",
                       fs::path("../../.build/exec/cuwacunu_exec"));
    request.execution_profile.execution_backend_id =
        trim_ascii(policy_get(ctx->policy, "rollout_execution_backend_id"));
    if (request.execution_profile.execution_backend_id.empty()) {
      request.execution_profile.execution_backend_id =
          "cajtucu.execution.paper.v1";
    }
    request.execution_profile.cost_model_id =
        trim_ascii(policy_get(ctx->policy, "rollout_cost_model_id"));
    if (request.execution_profile.cost_model_id.empty()) {
      request.execution_profile.cost_model_id =
          "linear_transaction_cost_rate.v1";
    }
    request.execution_profile.allow_synthetic_direct_edges =
        policy_bool_or(ctx->policy, "rollout_allow_synthetic_direct_edges",
                       request.execution_profile.allow_synthetic_direct_edges);
    request.execution_profile.synthetic_edge_research_reason = trim_ascii(
        policy_get(ctx->policy, "rollout_synthetic_edge_research_reason"));
    request.execution_profile.linear_transaction_cost_rate = policy_double_or(
        ctx->policy, "rollout_linear_transaction_cost_rate",
        request.execution_profile.linear_transaction_cost_rate);
    request.execution_profile.allow_partial_fills =
        policy_bool_or(ctx->policy, "rollout_allow_partial_fills",
                       request.execution_profile.allow_partial_fills);
    request.execution_profile.equity_mismatch_tolerance =
        policy_double_or(ctx->policy, "rollout_equity_mismatch_tolerance",
                         request.execution_profile.equity_mismatch_tolerance);
    request.execution_profile.equity_mismatch_fail_tolerance = policy_double_or(
        ctx->policy, "rollout_equity_mismatch_fail_tolerance",
        request.execution_profile.equity_mismatch_fail_tolerance);
    request.execution_profile.live_execution_allowed =
        policy_bool_or(ctx->policy, "rollout_live_execution_allowed",
                       request.execution_profile.live_execution_allowed);

    if (!job_path_allowed(ctx->policy, request.runtime_job_dir)) {
      *err = "E_ENVIRONMENT_ROLLOUT_JOB_DIR_DENIED: runtime_job_dir is "
             "outside Environment allowed_job_roots: " +
             request.runtime_job_dir.string();
      return false;
    }

    const auto job_state = marshal::rollout_marshal_detail::read_kv_file(
        request.runtime_job_dir / "job.state");
    const auto state_batch = job_state.find("replay_batch_index_path");
    if (state_batch != job_state.end() &&
        !trim_ascii(state_batch->second).empty()) {
      request.replay_batch_index_path = state_batch->second;
    } else {
      request.replay_batch_index_path = request.runtime_job_dir / "artifacts" /
                                        "kikijyeba.environment.replay.v1" /
                                        "runtime_replay_batches.index";
    }

    const auto job_manifest = marshal::rollout_marshal_detail::read_kv_file(
        request.runtime_job_dir / "job.manifest");
    const auto manifest_graph = job_manifest.find("graph_order_fingerprint");
    if (manifest_graph != job_manifest.end()) {
      request.graph_order_fingerprint = manifest_graph->second;
    }

    request = marshal::normalize_rollout_request_defaults(std::move(request));
    if (!request.accounting_numeraire_node_id.empty() &&
        !request.target_node_ids.empty()) {
      request.asset_universe_digest = marshal::rollout_asset_universe_digest(
          request.accounting_numeraire_node_id, request.target_node_ids);
    }
    *out = std::move(request);
    return true;
  } catch (const std::exception &ex) {
    *err = ex.what();
    return false;
  }
}

struct rollout_allocation_sidecar_result_t {
  bool attempted{false};
  bool ok{false};
  bool written{false};
  fs::path path{};
  std::string digest{};
  std::string policy_id{};
  std::string observer_belief_fact_digest{};
  std::string forecast_artifact_digest{};
  std::vector<std::string> issues{};
  std::string error{};
};

[[nodiscard]] std::string map_value(const std::map<std::string, std::string> &m,
                                    const std::string &key) {
  const auto it = m.find(key);
  return it == m.end() ? std::string{} : it->second;
}

[[nodiscard]] std::string
first_map_value(const std::map<std::string, std::string> &m,
                std::initializer_list<std::string> keys) {
  for (const auto &key : keys) {
    const auto value = map_value(m, key);
    if (!trim_ascii(value).empty()) {
      return value;
    }
  }
  return {};
}

[[nodiscard]] std::int64_t
map_i64_or(const std::map<std::string, std::string> &m, const std::string &key,
           std::int64_t fallback) {
  const auto value = trim_ascii(map_value(m, key));
  if (value.empty()) {
    return fallback;
  }
  try {
    std::size_t consumed = 0;
    const auto parsed = std::stoll(value, &consumed);
    return consumed == value.size() ? parsed : fallback;
  } catch (...) {
    return fallback;
  }
}

[[nodiscard]] double
map_double_or(const std::map<std::string, std::string> &m,
              const std::string &key,
              double fallback = std::numeric_limits<double>::quiet_NaN()) {
  const auto value = trim_ascii(map_value(m, key));
  if (value.empty()) {
    return fallback;
  }
  try {
    std::size_t consumed = 0;
    const auto parsed = std::stod(value, &consumed);
    return consumed == value.size() ? parsed : fallback;
  } catch (...) {
    return fallback;
  }
}

[[nodiscard]] std::optional<std::int64_t> select_allocation_policy_index(
    const std::map<std::string, std::string> &report) {
  const auto count = map_i64_or(report, "policy_summary_count", 0);
  for (std::int64_t i = 0; i < count; ++i) {
    const std::string prefix = "policy_" + std::to_string(i) + "_";
    if (map_value(report, prefix + "kind") == "deterministic_allocator") {
      return i;
    }
  }
  for (std::int64_t i = 0; i < count; ++i) {
    const std::string prefix = "policy_" + std::to_string(i) + "_";
    if (map_value(report, prefix + "id") == "equal_weight.v1") {
      return i;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string first_episode_prefix_for_policy(
    const std::map<std::string, std::string> &report,
    const std::string &policy_id) {
  const auto count = map_i64_or(report, "episode_count", 0);
  for (std::int64_t i = 0; i < count; ++i) {
    const std::string prefix = "episode_" + std::to_string(i) + "_";
    if (map_value(report, prefix + "policy_id") == policy_id) {
      return prefix;
    }
  }
  return {};
}

[[nodiscard]] std::string first_transition_value_for_episode(
    const std::map<std::string, std::string> &report,
    const std::string &episode_prefix, const std::string &field) {
  const auto count = map_i64_or(report, episode_prefix + "step_count", 0);
  for (std::int64_t i = 0; i < count; ++i) {
    const auto value = map_value(report, episode_prefix + "step_" +
                                             std::to_string(i) + "_" + field);
    if (!trim_ascii(value).empty()) {
      return value;
    }
  }
  return {};
}

[[nodiscard]] rollout_allocation_sidecar_result_t
materialize_rollout_allocation_engine_sidecar(
    const marshal::marshal_rollout_plan_t &plan) {
  rollout_allocation_sidecar_result_t result{};
  result.attempted = true;
  result.path = plan.runtime_job_dir / "lattice.allocation_engine.fact";
  try {
    const auto report = marshal::rollout_marshal_detail::read_kv_file(
        plan.expected_report_path);
    if (report.empty()) {
      result.error = "missing_or_empty_rollout_report";
      return result;
    }
    const auto parent = exposure::make_exposure_fact_from_sidecar_file(
        plan.runtime_job_dir / "lattice.exposure.fact", plan.runtime_job_dir);
    const auto observer_facts =
        exposure::make_observer_belief_facts_from_job_dir(plan.runtime_job_dir,
                                                          parent);
    const auto forecast_facts = exposure::make_forecast_eval_facts_from_job_dir(
        plan.runtime_job_dir, parent);
    if (observer_facts.empty()) {
      result.error = "missing_observer_belief_fact";
      return result;
    }
    if (forecast_facts.empty()) {
      result.error = "missing_forecast_eval_fact";
      return result;
    }
    const auto policy_index = select_allocation_policy_index(report);
    if (!policy_index.has_value()) {
      result.error = "missing_deterministic_allocation_policy_summary";
      return result;
    }

    const std::string policy_prefix =
        "policy_" + std::to_string(*policy_index) + "_";
    const std::string policy_id = map_value(report, policy_prefix + "id");
    const std::string policy_kind = map_value(report, policy_prefix + "kind");
    const std::string episode_prefix =
        first_episode_prefix_for_policy(report, policy_id);
    if (episode_prefix.empty()) {
      result.error = "missing_allocation_policy_episode";
      return result;
    }

    const auto &observer = observer_facts.front();
    exposure::lattice_allocation_engine_fact_t fact{};
    exposure::populate_artifact_fact_identity(fact, parent);
    fact.target_node_weights = first_map_value(
        report, {episode_prefix + "allocation_target_node_weights"});
    if (fact.target_node_weights.empty()) {
      fact.target_node_weights = first_transition_value_for_episode(
          report, episode_prefix, "target_node_weights");
    }
    fact.accounting_numeraire_node_id = first_map_value(
        report, {episode_prefix + "allocation_numeraire_node_id",
                 episode_prefix + "accounting_numeraire_node_id"});
    if (fact.accounting_numeraire_node_id.empty()) {
      fact.accounting_numeraire_node_id = "USDT";
    }
    fact.accounting_numeraire_node_source = "base_policy";
    fact.base_policy_accounting_numeraire_node_id =
        fact.accounting_numeraire_node_id;
    fact.accounting_numeraire_node_graph_bound = true;
    fact.numeraire_weight = map_double_or(
        report, episode_prefix + "allocation_numeraire_weight",
        map_double_or(report, policy_prefix + "mean_post_execution_"
                                              "numeraire_weight"));
    fact.turnover = map_double_or(
        report, episode_prefix + "allocation_turnover",
        map_double_or(report, policy_prefix + "mean_total_turnover"));
    fact.objective_terms = first_map_value(
        report, {episode_prefix + "allocation_objective_terms"});
    if (fact.objective_terms.empty()) {
      fact.objective_terms =
          "scenario_log_growth_cvar_cost_concentration_uncertainty;"
          "policy_id=" +
          policy_id + ";policy_kind=" + policy_kind;
    }
    fact.cvar_loss = map_double_or(
        report, episode_prefix + "allocation_cvar_loss",
        map_double_or(report, policy_prefix + "mean_max_drawdown", 0.0));
    fact.transaction_cost_estimate = map_double_or(
        report, episode_prefix + "allocation_transaction_cost_estimate",
        map_double_or(report,
                      policy_prefix + "mean_total_transaction_cost_numeraire",
                      0.0));
    fact.constraint_diagnostics = first_map_value(
        report, {episode_prefix + "allocation_constraint_diagnostics"});
    if (fact.constraint_diagnostics.empty()) {
      fact.constraint_diagnostics =
          "replay_valid_trace_count=" +
          map_value(report, policy_prefix + "cajtucu_valid_trace_count") +
          ";rejected_order_count=" +
          map_value(report, policy_prefix + "rejected_order_count");
    }
    fact.cap_diagnostics = first_map_value(
        report, {episode_prefix + "allocation_cap_diagnostics"});
    if (fact.cap_diagnostics.empty()) {
      fact.cap_diagnostics =
          "mean_post_execution_numeraire_weight=" +
          map_value(report,
                    policy_prefix + "mean_post_execution_numeraire_weight");
    }
    fact.scenario_growth_floor_status = first_map_value(
        report, {episode_prefix + "allocation_scenario_growth_floor_status"});
    if (fact.scenario_growth_floor_status.empty()) {
      fact.scenario_growth_floor_status = "not_triggered";
    }
    fact.fallback_reasons = first_map_value(
        report, {episode_prefix + "allocation_fallback_reasons"});
    if (fact.fallback_reasons.empty()) {
      fact.fallback_reasons = "none";
    }
    fact.derisk_reasons =
        first_map_value(report, {episode_prefix + "allocation_derisk_reasons"});
    if (fact.derisk_reasons.empty()) {
      fact.derisk_reasons = "none";
    }
    fact.observer_belief_fact_digest =
        exposure::observer_belief_fact_digest(observer);
    fact.forecast_artifact_digest = observer.forecast_artifact_digest;
    fact.base_policy_digest = marshal::marshal_digest_for_text(
        "kikijyeba.environment.rollout.allocation_base_policy.v1",
        policy_id + "|" + policy_kind + "|" +
            map_value(report, "policy_set_digest"));

    result.policy_id = policy_id;
    result.observer_belief_fact_digest = fact.observer_belief_fact_digest;
    result.forecast_artifact_digest = fact.forecast_artifact_digest;
    const auto fact_issues = exposure::allocation_engine_fact_issues(fact);
    result.issues = fact_issues;
    const auto summary = exposure::summarize_allocation_engines(
        std::vector<exposure::lattice_exposure_fact_t>{parent},
        std::vector<exposure::lattice_allocation_engine_fact_t>{fact},
        observer_facts, forecast_facts);
    result.issues.insert(result.issues.end(), summary.issues.begin(),
                         summary.issues.end());
    std::sort(result.issues.begin(), result.issues.end());
    result.issues.erase(std::unique(result.issues.begin(), result.issues.end()),
                        result.issues.end());
    if (!fact_issues.empty()) {
      result.error = "allocation_engine_fact_invalid";
      return result;
    }
    exposure::write_allocation_engine_fact_sidecar(result.path, fact);
    result.digest = exposure::allocation_engine_fact_digest(fact);
    result.ok = true;
    result.written = true;
    return result;
  } catch (const std::exception &ex) {
    result.error = ex.what();
    return result;
  }
}

[[nodiscard]] bool
required_string(const std::map<std::string, std::string> &fields,
                const std::string &key, std::string *out, std::string *err) {
  const auto raw = md::optional_raw(fields, key);
  if (!raw.has_value()) {
    *err = "missing required field: " + key;
    return false;
  }
  try {
    const std::string value = md::parse_string_raw(*raw, key);
    if (out != nullptr) {
      *out = value;
    }
    return true;
  } catch (const std::exception &ex) {
    *err = ex.what();
    return false;
  }
}

[[nodiscard]] bool
required_bool(const std::map<std::string, std::string> &fields,
              const std::string &key, bool *out, std::string *err) {
  const auto raw = md::optional_raw(fields, key);
  if (!raw.has_value()) {
    *err = "missing required field: " + key;
    return false;
  }
  const std::string value = trim_ascii(*raw);
  if (value == "true") {
    if (out != nullptr) {
      *out = true;
    }
    return true;
  }
  if (value == "false") {
    if (out != nullptr) {
      *out = false;
    }
    return true;
  }
  *err = key + " must be boolean";
  return false;
}

[[nodiscard]] bool
required_double(const std::map<std::string, std::string> &fields,
                const std::string &key, double *out, std::string *err) {
  const auto raw = md::optional_raw(fields, key);
  if (!raw.has_value()) {
    *err = "missing required field: " + key;
    return false;
  }
  try {
    if (out != nullptr) {
      *out = std::stod(trim_ascii(*raw));
    }
    return true;
  } catch (const std::exception &) {
    *err = key + " must be number";
    return false;
  }
}

[[nodiscard]] bool
required_i64(const std::map<std::string, std::string> &fields,
             const std::string &key, std::int64_t *out, std::string *err) {
  const auto raw = md::optional_raw(fields, key);
  if (!raw.has_value()) {
    *err = "missing required field: " + key;
    return false;
  }
  try {
    if (out != nullptr) {
      *out = std::stoll(trim_ascii(*raw));
    }
    return true;
  } catch (const std::exception &) {
    *err = key + " must be integer";
    return false;
  }
}

[[nodiscard]] std::string file_digest_or_empty(const fs::path &path,
                                               std::string_view schema) {
  std::string text;
  std::string error;
  if (!read_text_file(path, &text, &error)) {
    return {};
  }
  return marshal::marshal_digest_for_text(std::string(schema), text);
}

[[nodiscard]] bool
load_single_policy_training_fact(const fs::path &job_dir,
                                 exposure::lattice_policy_training_fact_t *out,
                                 std::string *err) {
  const auto parent = exposure::make_exposure_fact_from_job_dir(
      job_dir, exposure::exposure_build_context_t{});
  const auto facts =
      exposure::make_policy_training_facts_from_job_dir(job_dir, parent);
  if (facts.empty()) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_POLICY_TRAINING_FACT_MISSING: "
           "no runtime.policy_training.fact found under " +
           job_dir.string();
    return false;
  }
  if (facts.size() != 1U) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_POLICY_TRAINING_FACT_AMBIGUOUS: "
           "expected exactly one policy-training fact under " +
           job_dir.string();
    return false;
  }
  if (out != nullptr) {
    *out = facts.front();
  }
  return true;
}

[[nodiscard]] bool load_single_tsodao_settings_protection_fact(
    const fs::path &job_dir,
    exposure::lattice_tsodao_settings_protection_fact_t *out,
    std::string *err) {
  const auto parent = exposure::make_exposure_fact_from_job_dir(
      job_dir, exposure::exposure_build_context_t{});
  const auto facts =
      exposure::make_tsodao_settings_protection_facts_from_job_dir(job_dir,
                                                                   parent);
  if (facts.empty()) {
    *err = "E_ENVIRONMENT_TSODAO_SETTINGS_PROTECTION_FACT_MISSING: "
           "no lattice.tsodao_settings_protection.fact found under " +
           job_dir.string();
    return false;
  }
  if (facts.size() != 1U) {
    *err = "E_ENVIRONMENT_TSODAO_SETTINGS_PROTECTION_FACT_AMBIGUOUS: "
           "expected exactly one Tsodao settings-protection fact under " +
           job_dir.string();
    return false;
  }
  if (out != nullptr) {
    *out = facts.front();
  }
  return true;
}

[[nodiscard]] bool load_single_policy_acceptance_fact(
    const fs::path &job_dir, exposure::lattice_policy_acceptance_fact_t *out,
    std::string *err) {
  const auto parent = exposure::make_exposure_fact_from_job_dir(
      job_dir, exposure::exposure_build_context_t{});
  const auto facts =
      exposure::make_policy_acceptance_facts_from_job_dir(job_dir, parent);
  if (facts.empty()) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_READINESS_ACCEPTANCE_FACT_MISSING: "
           "no lattice.policy_acceptance.fact found under " +
           job_dir.string();
    return false;
  }
  if (facts.size() != 1U) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_READINESS_ACCEPTANCE_FACT_AMBIGUOUS: "
           "expected exactly one policy-acceptance fact under " +
           job_dir.string();
    return false;
  }
  if (out != nullptr) {
    *out = facts.front();
  }
  return true;
}

[[nodiscard]] bool load_single_paper_online_readiness_fact(
    const fs::path &job_dir,
    exposure::lattice_paper_online_readiness_fact_t *out, std::string *err) {
  const auto parent = exposure::make_exposure_fact_from_job_dir(
      job_dir, exposure::exposure_build_context_t{});
  const auto facts =
      exposure::make_paper_online_readiness_facts_from_job_dir(job_dir, parent);
  if (facts.empty()) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_READINESS_FACT_MISSING: "
           "no lattice.paper_online_readiness.fact found under " +
           job_dir.string();
    return false;
  }
  if (facts.size() != 1U) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_READINESS_FACT_AMBIGUOUS: "
           "expected exactly one paper-online-readiness fact under " +
           job_dir.string();
    return false;
  }
  if (out != nullptr) {
    *out = facts.front();
  }
  return true;
}

[[nodiscard]] bool append_policy_acceptance_required_fields(
    const std::map<std::string, std::string> &fields,
    exposure::lattice_policy_acceptance_fact_t *fact, std::string *err) {
  if (fact == nullptr) {
    *err = "policy acceptance fact pointer is null";
    return false;
  }
  fact->acceptance_policy_digest =
      exposure::policy_acceptance_governance_thresholds_v0_digest();
  fact->mandatory_baseline_set_digest =
      exposure::policy_acceptance_governance_v0_mandatory_baseline_set_digest();
  fact->mandatory_baselines = std::string(
      exposure::policy_acceptance_governance_v0_mandatory_baselines());
  fact->primary_metric_id = kPolicyAcceptancePrimaryMetricId;
  fact->primary_metric_threshold = 0.0;
  fact->primary_metric_direction = kPolicyAcceptancePrimaryMetricDirection;
  fact->after_cost_metric_set_digest =
      exposure::policy_acceptance_governance_v0_after_cost_metric_set_digest();
  fact->cost_slippage_assumption_digest = exposure::
      policy_acceptance_governance_v0_cost_slippage_assumption_digest();
  fact->uncertainty_policy = kPolicyAcceptanceUncertaintyPolicy;
  fact->uncertainty_model = kPolicyAcceptanceUncertaintyModel;
  fact->selector_split = kPolicyAcceptanceSelectorSplit;
  fact->validation_split = kPolicyAcceptanceValidationSplit;
  fact->test_split = kPolicyAcceptanceTestSplit;
  fact->sealed_test_policy = kPolicyAcceptanceSealedTestPolicy;
  fact->tie_policy = kPolicyAcceptanceTiePolicy;
  fact->negative_tests_digest =
      exposure::policy_acceptance_governance_v0_negative_tests_digest();
  fact->threshold_selection_audit_digest = exposure::
      policy_acceptance_governance_v0_threshold_selection_audit_digest();
  fact->promotion_criteria_digest =
      exposure::policy_acceptance_governance_v0_promotion_criteria_digest();
  if (!required_string(fields, "acceptance_id", &fact->acceptance_id, err) ||
      !required_string(
          fields, "accepted_policy_training_proof_certificate_digest",
          &fact->accepted_policy_training_proof_certificate_digest, err) ||
      !required_string(
          fields, "tsodao_settings_protection_proof_certificate_digest",
          &fact->tsodao_settings_protection_proof_certificate_digest, err) ||
      !required_double(fields, "primary_metric_value",
                       &fact->primary_metric_value, err)) {
    return false;
  }
  return required_bool(fields, "accepted_policy_training_ready",
                       &fact->accepted_policy_training_ready, err) &&
         required_bool(fields, "tsodao_settings_protection_ready",
                       &fact->tsodao_settings_protection_ready, err) &&
         required_bool(fields, "protected_settings_bound",
                       &fact->protected_settings_bound, err) &&
         required_bool(fields, "mandatory_baselines_bound",
                       &fact->mandatory_baselines_bound, err) &&
         required_bool(fields, "mandatory_baselines_passed",
                       &fact->mandatory_baselines_passed, err) &&
         required_bool(fields, "after_cost_metrics_bound",
                       &fact->after_cost_metrics_bound, err) &&
         required_bool(fields, "primary_metric_passed",
                       &fact->primary_metric_passed, err) &&
         required_bool(fields, "uncertainty_policy_bound",
                       &fact->uncertainty_policy_bound, err) &&
         required_bool(fields, "uncertainty_passed", &fact->uncertainty_passed,
                       err) &&
         required_bool(fields, "selector_split_bound",
                       &fact->selector_split_bound, err) &&
         required_bool(fields, "validation_test_disjoint",
                       &fact->validation_test_disjoint, err) &&
         required_bool(fields, "test_sealed_until_acceptance",
                       &fact->test_sealed_until_acceptance, err) &&
         required_bool(fields, "negative_tests_bound",
                       &fact->negative_tests_bound, err) &&
         required_bool(fields, "negative_tests_passed",
                       &fact->negative_tests_passed, err) &&
         required_bool(fields, "leakage_negative_tests_passed",
                       &fact->leakage_negative_tests_passed, err) &&
         required_bool(fields, "threshold_selection_audit_bound",
                       &fact->threshold_selection_audit_bound, err) &&
         required_bool(fields, "threshold_selected_before_test",
                       &fact->threshold_selected_before_test, err) &&
         required_bool(fields, "tie_policy_bound", &fact->tie_policy_bound,
                       err) &&
         required_bool(fields, "tie_policy_passed", &fact->tie_policy_passed,
                       err) &&
         required_bool(fields, "cost_slippage_assumptions_bound",
                       &fact->cost_slippage_assumptions_bound, err) &&
         required_bool(fields, "promotion_criteria_bound",
                       &fact->promotion_criteria_bound, err) &&
         required_bool(fields, "promotion_criteria_satisfied",
                       &fact->promotion_criteria_satisfied, err) &&
         required_bool(fields, "policy_acceptance_decision",
                       &fact->policy_acceptance_decision, err);
}

[[nodiscard]] exposure::lattice_policy_acceptance_fact_t
make_policy_acceptance_fact(
    const std::map<std::string, std::string> &fields,
    const exposure::lattice_policy_training_fact_t &policy_fact,
    const exposure::lattice_tsodao_settings_protection_fact_t &protection_fact,
    std::string *err) {
  exposure::lattice_policy_acceptance_fact_t fact{};
  fact.parent_exposure_fact_digest = policy_fact.parent_exposure_fact_digest;
  fact.contract_fingerprint = policy_fact.contract_fingerprint;
  fact.protocol_id = policy_fact.protocol_id;
  fact.graph_order_fingerprint = policy_fact.graph_order_fingerprint;
  fact.source_cursor_token = policy_fact.source_cursor_token;
  fact.split_policy_fingerprint = policy_fact.split_policy_fingerprint;
  fact.component_assembly_fingerprint =
      policy_fact.component_assembly_fingerprint;
  fact.target_component_family_id = "tsodao.policy_acceptance";
  fact.job_id = policy_fact.job_id;
  fact.wave_id = "policy_acceptance";
  fact.job_status = "completed";
  fact.wave_action = "accept_policy";
  fact.split_name = policy_fact.split_name;
  fact.split_role = policy_fact.split_role;
  fact.anchor_range = policy_fact.anchor_range;
  fact.completed_anchor_range = policy_fact.completed_anchor_range;

  fact.acceptance_contract_id = "policy_acceptance_contract.v1";
  fact.accepted_policy_training_fact_digest =
      exposure::policy_training_fact_digest(policy_fact);
  fact.accepted_policy_training_target_id = "policy_training_artifact_ready";
  fact.tsodao_settings_protection_fact_digest =
      exposure::tsodao_settings_protection_fact_digest(protection_fact);
  fact.tsodao_settings_protection_target_id =
      "tsodao_settings_protection_ready";
  fact.protected_settings_bundle_digest =
      protection_fact.protected_settings_bundle_digest;
  fact.accepted_policy_id = policy_fact.policy_id;
  fact.accepted_policy_kind = policy_fact.policy_kind;
  fact.accepted_policy_family_id = policy_fact.policy_family_id;
  fact.accepted_actor_checkpoint_digest = policy_fact.actor_checkpoint_digest;
  fact.accepted_checkpoint_digest = policy_fact.checkpoint_digest;
  fact.policy_quality_report_digest = policy_fact.policy_quality_report_digest;
  fact.validation_rollout_report_digest =
      policy_fact.validation_rollout_report_digest;
  fact.reward_contract_id = policy_fact.reward_contract_id;
  fact.execution_profile_digest = policy_fact.execution_profile_digest;
  fact.accounting_numeraire_node_id =
      protection_fact.accounting_numeraire_node_id;

  if (!append_policy_acceptance_required_fields(fields, &fact, err)) {
    return {};
  }
  return fact;
}

[[nodiscard]] bool append_paper_online_readiness_required_fields(
    const std::map<std::string, std::string> &fields,
    exposure::lattice_paper_online_readiness_fact_t *fact, std::string *err) {
  if (fact == nullptr) {
    *err = "paper-online readiness fact pointer is null";
    return false;
  }
  std::int64_t max_market_data_staleness_ms = 0;
  std::int64_t clock_skew_tolerance_ms = 0;
  if (!required_string(fields, "readiness_id", &fact->readiness_id, err) ||
      !required_string(fields, "policy_acceptance_proof_certificate_digest",
                       &fact->policy_acceptance_proof_certificate_digest,
                       err) ||
      !required_string(fields, "paper_online_profile_digest",
                       &fact->paper_online_profile_digest, err) ||
      !required_string(fields, "direct_edge_universe_digest",
                       &fact->direct_edge_universe_digest, err) ||
      !required_i64(fields, "max_market_data_staleness_ms",
                    &max_market_data_staleness_ms, err) ||
      !required_i64(fields, "clock_skew_tolerance_ms", &clock_skew_tolerance_ms,
                    err)) {
    return false;
  }
  fact->max_market_data_staleness_ms =
      static_cast<int>(max_market_data_staleness_ms);
  fact->clock_skew_tolerance_ms = static_cast<int>(clock_skew_tolerance_ms);
  fact->paper_online_environment_contract_id =
      kPaperOnlineEnvironmentContractId;
  fact->direct_edge_universe_validation_policy_id =
      kPaperOnlineDirectEdgeUniverseValidationPolicyId;
  fact->missing_direct_pair_policy = kPaperOnlineMissingDirectPairPolicy;
  fact->synthetic_execution_market_policy_id =
      kPaperOnlineSyntheticExecutionMarketPolicyId;
  fact->session_state_schema_id = kPaperOnlineSessionStateSchemaId;
  fact->persistent_paper_ledger_recovery_policy_id =
      kPaperOnlinePersistentLedgerRecoveryPolicyId;
  fact->paper_ledger_storage_policy_id = kPaperOnlineLedgerStoragePolicyId;
  fact->session_lifecycle_policy_id = kPaperOnlineSessionLifecyclePolicyId;
  fact->idempotency_policy_id = kPaperOnlineIdempotencyPolicyId;
  fact->execution_intent_id_policy = kPaperOnlineExecutionIntentIdPolicy;
  fact->duplicate_action_policy = kPaperOnlineDuplicateActionPolicy;
  fact->duplicate_execution_intent_policy =
      kPaperOnlineDuplicateExecutionIntentPolicy;
  fact->clock_policy_id = kPaperOnlineClockPolicyId;
  fact->timestamp_policy_id = kPaperOnlineTimestampPolicyId;
  fact->market_data_staleness_policy_id =
      kPaperOnlineMarketDataStalenessPolicyId;
  fact->reward_report_artifact_policy_id =
      kPaperOnlineRewardReportArtifactPolicyId;
  fact->operator_abort_policy_id = kPaperOnlineOperatorAbortPolicyId;
  fact->kill_switch_policy_id = kPaperOnlineKillSwitchPolicyId;

  return required_bool(fields, "policy_acceptance_ready",
                       &fact->policy_acceptance_ready, err) &&
         required_bool(fields, "tsodao_settings_protection_ready",
                       &fact->tsodao_settings_protection_ready, err) &&
         required_bool(fields, "accepted_policy_bound",
                       &fact->accepted_policy_bound, err) &&
         required_bool(fields, "protected_settings_bound",
                       &fact->protected_settings_bound, err) &&
         required_bool(fields, "direct_edge_universe_validated",
                       &fact->direct_edge_universe_validated, err) &&
         required_bool(fields, "locked_execution_profile_bound",
                       &fact->locked_execution_profile_bound, err) &&
         required_bool(fields, "persistent_paper_ledger_recovery_bound",
                       &fact->persistent_paper_ledger_recovery_bound, err) &&
         required_bool(fields, "idempotency_bound", &fact->idempotency_bound,
                       err) &&
         required_bool(fields, "duplicate_action_protection_bound",
                       &fact->duplicate_action_protection_bound, err) &&
         required_bool(fields, "session_lifecycle_bound",
                       &fact->session_lifecycle_bound, err) &&
         required_bool(fields, "clock_timestamp_policy_bound",
                       &fact->clock_timestamp_policy_bound, err) &&
         required_bool(fields, "market_data_staleness_bound",
                       &fact->market_data_staleness_bound, err) &&
         required_bool(fields, "reward_report_artifact_path_bound",
                       &fact->reward_report_artifact_path_bound, err) &&
         required_bool(fields, "operator_abort_bound",
                       &fact->operator_abort_bound, err) &&
         required_bool(fields, "kill_switch_bound", &fact->kill_switch_bound,
                       err);
}

[[nodiscard]] exposure::lattice_paper_online_readiness_fact_t
make_paper_online_readiness_fact(
    const std::map<std::string, std::string> &fields,
    const exposure::lattice_policy_acceptance_fact_t &acceptance_fact,
    std::string *err) {
  exposure::lattice_paper_online_readiness_fact_t fact{};
  fact.parent_exposure_fact_digest =
      acceptance_fact.parent_exposure_fact_digest;
  fact.contract_fingerprint = acceptance_fact.contract_fingerprint;
  fact.protocol_id = acceptance_fact.protocol_id;
  fact.graph_order_fingerprint = acceptance_fact.graph_order_fingerprint;
  fact.source_cursor_token = acceptance_fact.source_cursor_token;
  fact.split_policy_fingerprint = acceptance_fact.split_policy_fingerprint;
  fact.component_assembly_fingerprint =
      acceptance_fact.component_assembly_fingerprint;
  fact.target_component_family_id = "kikijyeba.paper_online.readiness";
  fact.job_id = acceptance_fact.job_id;
  fact.wave_id = "paper_online_readiness";
  fact.job_status = "completed";
  fact.wave_action = "paper_online_readiness_contract";
  fact.split_name = acceptance_fact.split_name;
  fact.split_role = acceptance_fact.split_role;
  fact.anchor_range = acceptance_fact.anchor_range;
  fact.completed_anchor_range = acceptance_fact.completed_anchor_range;

  fact.readiness_contract_id = "paper_online_readiness_contract.v1";
  fact.policy_acceptance_fact_digest =
      exposure::policy_acceptance_fact_digest(acceptance_fact);
  fact.policy_acceptance_target_id = "policy_acceptance_contract_ready";
  fact.tsodao_settings_protection_fact_digest =
      acceptance_fact.tsodao_settings_protection_fact_digest;
  fact.protected_settings_bundle_digest =
      acceptance_fact.protected_settings_bundle_digest;
  fact.accepted_policy_id = acceptance_fact.accepted_policy_id;
  fact.accepted_policy_kind = acceptance_fact.accepted_policy_kind;
  fact.accepted_policy_family_id = acceptance_fact.accepted_policy_family_id;
  fact.accepted_actor_checkpoint_digest =
      acceptance_fact.accepted_actor_checkpoint_digest;
  fact.accepted_checkpoint_digest = acceptance_fact.accepted_checkpoint_digest;
  fact.policy_quality_report_digest =
      acceptance_fact.policy_quality_report_digest;
  fact.validation_rollout_report_digest =
      acceptance_fact.validation_rollout_report_digest;
  fact.reward_contract_id = acceptance_fact.reward_contract_id;
  fact.execution_profile_digest = acceptance_fact.execution_profile_digest;
  fact.locked_execution_profile_digest =
      acceptance_fact.execution_profile_digest;
  fact.accounting_numeraire_node_id =
      acceptance_fact.accounting_numeraire_node_id;

  if (!append_paper_online_readiness_required_fields(fields, &fact, err)) {
    return {};
  }
  return fact;
}

[[nodiscard]] bool verify_expected_preview_digest(
    std::string_view subject, std::string_view requested_mode,
    bool expected_supplied, const std::string &expected_digest,
    const std::string &actual_digest, std::string *err) {
  if (requested_mode == "issue" && !expected_supplied) {
    *err = "E_ENVIRONMENT_CERTIFY_PREVIEW_DIGEST_REQUIRED: "
           "expected_preview_digest is required when mode=issue";
    return false;
  }
  if (!expected_supplied) {
    return true;
  }
  const std::string trimmed = trim_ascii(expected_digest);
  if (trimmed.empty()) {
    *err = "expected_preview_digest must be non-empty when supplied";
    return false;
  }
  if (trimmed != actual_digest) {
    *err = "E_ENVIRONMENT_CERTIFY_PREVIEW_DIGEST_MISMATCH: expected " +
           trimmed + " actual " + actual_digest + " subject " +
           std::string(subject);
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_policy_acceptance_certification(
    const std::map<std::string, std::string> &fields,
    std::string_view requested_mode, environment_context_t *ctx,
    std::string *out, std::string *err, bool expected_preview_digest_supplied,
    const std::string &expected_preview_digest, std::string_view tool_name) {
  const bool should_write = requested_mode == "issue";
  if (should_write &&
      !policy_bool_or(ctx->policy, "allow_certify_issue", false)) {
    *err = "E_ENVIRONMENT_CERTIFY_ISSUE_DENIED: allow_certify_issue is false";
    return false;
  }

  const std::string policy_training_arg =
      md::optional_string(fields, "policy_training_job_dir");
  const std::string source_raw = trim_ascii(policy_training_arg);
  if (source_raw.empty()) {
    *err = "missing required field: policy_training_job_dir";
    return false;
  }
  const fs::path policy_training_job_dir = normalize_path(fs::path(source_raw));
  if (!job_path_allowed(ctx->policy, policy_training_job_dir)) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_JOB_DIR_DENIED: "
           "policy_training_job_dir is outside Environment Hero roots: " +
           policy_training_job_dir.string();
    return false;
  }
  const std::string tsodao_arg =
      md::optional_string(fields, "tsodao_settings_protection_job_dir");
  const fs::path tsodao_job_dir =
      trim_ascii(tsodao_arg).empty()
          ? policy_training_job_dir
          : normalize_path(fs::path(trim_ascii(tsodao_arg)));
  if (!job_path_allowed(ctx->policy, tsodao_job_dir)) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_TSODAO_JOB_DIR_DENIED: "
           "tsodao_settings_protection_job_dir is outside Environment Hero "
           "roots: " +
           tsodao_job_dir.string();
    return false;
  }

  exposure::lattice_policy_training_fact_t policy_fact{};
  exposure::lattice_tsodao_settings_protection_fact_t protection_fact{};
  if (!load_single_policy_training_fact(policy_training_job_dir, &policy_fact,
                                        err) ||
      !load_single_tsodao_settings_protection_fact(tsodao_job_dir,
                                                   &protection_fact, err)) {
    return false;
  }

  std::string assembly_error;
  const auto acceptance_fact = make_policy_acceptance_fact(
      fields, policy_fact, protection_fact, &assembly_error);
  if (!assembly_error.empty()) {
    *err = assembly_error;
    return false;
  }
  const auto fact_issues =
      exposure::policy_acceptance_fact_issues(acceptance_fact);
  const auto binding_issues = exposure::policy_acceptance_binding_issues(
      acceptance_fact,
      std::vector<exposure::lattice_policy_training_fact_t>{policy_fact},
      std::vector<exposure::lattice_tsodao_settings_protection_fact_t>{
          protection_fact});
  std::vector<std::string> all_issues = fact_issues;
  all_issues.insert(all_issues.end(), binding_issues.begin(),
                    binding_issues.end());
  const bool contract_ready = all_issues.empty();
  const fs::path fact_path =
      policy_training_job_dir / "lattice.policy_acceptance.fact";
  const std::string fact_digest =
      exposure::policy_acceptance_fact_digest(acceptance_fact);
  const std::string preview_text =
      exposure::canonical_policy_acceptance_fact_text(acceptance_fact);
  const std::string preview_digest =
      certify_preview_digest("policy_acceptance", preview_text);
  if (!verify_expected_preview_digest(
          "policy_acceptance", requested_mode, expected_preview_digest_supplied,
          expected_preview_digest, preview_digest, err)) {
    return false;
  }
  if (should_write && !contract_ready) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_CONTRACT_INVALID: " +
           string_array_json(all_issues);
    return false;
  }
  if (should_write && fs::exists(fact_path)) {
    *err = "E_ENVIRONMENT_POLICY_ACCEPTANCE_FACT_EXISTS: refusing to "
           "overwrite " +
           fact_path.string();
    return false;
  }
  std::string file_digest;
  if (should_write) {
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        fact_path, preview_text);
    file_digest = file_digest_or_empty(
        fact_path, "kikijyeba.lattice.policy_acceptance.v1");
  }

  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":" << json_quote(tool_name)
       << ",\"requested_mode\":" << json_quote(requested_mode)
       << ",\"check_only\":" << bool_json(!should_write)
       << ",\"schema_version\":\"kikijyeba.environment.policy_acceptance_"
          "certification_packet.v1\""
       << ",\"subject\":\"policy_acceptance\""
       << ",\"preview_digest_domain\":"
       << json_quote(certify_preview_digest_domain("policy_acceptance"))
       << ",\"preview_digest\":" << json_quote(preview_digest)
       << ",\"expected_preview_digest_verified\":"
       << bool_json(expected_preview_digest_supplied)
       << ",\"policy_acceptance_contract_ready\":" << bool_json(contract_ready)
       << ",\"policy_acceptance_fact_written\":" << bool_json(should_write)
       << ",\"policy_acceptance_fact_path\":" << json_quote(fact_path.string())
       << ",\"policy_acceptance_fact_digest\":" << json_quote(fact_digest)
       << ",\"policy_acceptance_fact_ref\":"
       << json_quote(display::digest_ref("acc", fact_digest))
       << ",\"acceptance_governance_policy_id\":"
       << json_quote(std::string(
              exposure::policy_acceptance_governance_thresholds_v0_id()))
       << ",\"acceptance_governance_policy_digest\":"
       << json_quote(
              exposure::policy_acceptance_governance_thresholds_v0_digest())
       << ",\"acceptance_policy_digest\":"
       << json_quote(acceptance_fact.acceptance_policy_digest)
       << ",\"policy_acceptance_file_digest\":" << json_quote(file_digest)
       << ",\"accepted_policy_training_fact_digest\":"
       << json_quote(acceptance_fact.accepted_policy_training_fact_digest)
       << ",\"accepted_policy_training_fact_ref\":"
       << json_quote(display::digest_ref(
              "ptf", acceptance_fact.accepted_policy_training_fact_digest))
       << ",\"tsodao_settings_protection_fact_digest\":"
       << json_quote(acceptance_fact.tsodao_settings_protection_fact_digest)
       << ",\"tsodao_settings_protection_fact_ref\":"
       << json_quote(display::digest_ref(
              "tsp", acceptance_fact.tsodao_settings_protection_fact_digest))
       << ",\"protected_settings_bundle_digest\":"
       << json_quote(acceptance_fact.protected_settings_bundle_digest)
       << ",\"accepted_policy_id\":"
       << json_quote(acceptance_fact.accepted_policy_id)
       << ",\"accepted_actor_checkpoint_digest\":"
       << json_quote(acceptance_fact.accepted_actor_checkpoint_digest)
       << ",\"policy_quality_report_digest\":"
       << json_quote(acceptance_fact.policy_quality_report_digest)
       << ",\"primary_metric_id\":"
       << json_quote(acceptance_fact.primary_metric_id)
       << ",\"primary_metric_value\":" << acceptance_fact.primary_metric_value
       << ",\"primary_metric_threshold\":"
       << acceptance_fact.primary_metric_threshold
       << ",\"primary_metric_direction\":"
       << json_quote(acceptance_fact.primary_metric_direction)
       << ",\"issues\":" << string_array_json(all_issues)
       << ",\"authority\":{\"environment_emits_policy_acceptance_fact\":true,"
          "\"environment_selects_policy\":false,"
          "\"environment_selects_checkpoint\":false,"
          "\"environment_claims_policy_quality\":false,"
          "\"environment_claims_market_readiness\":false,"
          "\"environment_claims_deployment_readiness\":false,"
          "\"environment_executes_live_capital\":false,"
          "\"environment_proves_lattice_target\":false}"
       << ",\"next_safe_actions\":[\"inspect_policy_acceptance_fact\","
          "\"evaluate_policy_acceptance_contract_ready\"]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_paper_online_readiness_certification(
    const std::map<std::string, std::string> &fields,
    std::string_view requested_mode, environment_context_t *ctx,
    std::string *out, std::string *err, bool expected_preview_digest_supplied,
    const std::string &expected_preview_digest, std::string_view tool_name) {
  const bool should_write = requested_mode == "issue";
  if (should_write &&
      !policy_bool_or(ctx->policy, "allow_certify_issue", false)) {
    *err = "E_ENVIRONMENT_CERTIFY_ISSUE_DENIED: allow_certify_issue is false";
    return false;
  }

  const std::string acceptance_arg =
      md::optional_string(fields, "policy_acceptance_job_dir");
  const std::string source_raw = trim_ascii(acceptance_arg);
  if (source_raw.empty()) {
    *err = "missing required field: policy_acceptance_job_dir";
    return false;
  }
  const fs::path acceptance_job_dir = normalize_path(fs::path(source_raw));
  if (!job_path_allowed(ctx->policy, acceptance_job_dir)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_READINESS_JOB_DIR_DENIED: "
           "policy_acceptance_job_dir is outside Environment Hero roots: " +
           acceptance_job_dir.string();
    return false;
  }

  exposure::lattice_policy_acceptance_fact_t acceptance_fact{};
  exposure::lattice_tsodao_settings_protection_fact_t protection_fact{};
  if (!load_single_policy_acceptance_fact(acceptance_job_dir, &acceptance_fact,
                                          err) ||
      !load_single_tsodao_settings_protection_fact(acceptance_job_dir,
                                                   &protection_fact, err)) {
    return false;
  }

  std::string assembly_error;
  const auto readiness_fact = make_paper_online_readiness_fact(
      fields, acceptance_fact, &assembly_error);
  if (!assembly_error.empty()) {
    *err = assembly_error;
    return false;
  }
  const auto fact_issues =
      exposure::paper_online_readiness_fact_issues(readiness_fact);
  const auto binding_issues = exposure::paper_online_readiness_binding_issues(
      readiness_fact,
      std::vector<exposure::lattice_policy_acceptance_fact_t>{acceptance_fact},
      std::vector<exposure::lattice_tsodao_settings_protection_fact_t>{
          protection_fact});
  std::vector<std::string> all_issues = fact_issues;
  all_issues.insert(all_issues.end(), binding_issues.begin(),
                    binding_issues.end());
  const bool contract_ready = all_issues.empty();
  const fs::path fact_path =
      acceptance_job_dir / "lattice.paper_online_readiness.fact";
  const std::string fact_digest =
      exposure::paper_online_readiness_fact_digest(readiness_fact);
  const std::string preview_text =
      exposure::canonical_paper_online_readiness_fact_text(readiness_fact);
  const std::string preview_digest =
      certify_preview_digest("paper_online_readiness", preview_text);
  if (!verify_expected_preview_digest("paper_online_readiness", requested_mode,
                                      expected_preview_digest_supplied,
                                      expected_preview_digest, preview_digest,
                                      err)) {
    return false;
  }
  if (should_write && !contract_ready) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_READINESS_CONTRACT_INVALID: " +
           string_array_json(all_issues);
    return false;
  }
  if (should_write && fs::exists(fact_path)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_READINESS_FACT_EXISTS: refusing to "
           "overwrite " +
           fact_path.string();
    return false;
  }
  std::string file_digest;
  if (should_write) {
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        fact_path, preview_text);
    file_digest = file_digest_or_empty(
        fact_path, "kikijyeba.lattice.paper_online_readiness.v1");
  }

  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":" << json_quote(tool_name)
       << ",\"requested_mode\":" << json_quote(requested_mode)
       << ",\"check_only\":" << bool_json(!should_write)
       << ",\"schema_version\":\"kikijyeba.environment.paper_online_"
          "readiness_certification_packet.v1\""
       << ",\"subject\":\"paper_online_readiness\""
       << ",\"preview_digest_domain\":"
       << json_quote(certify_preview_digest_domain("paper_online_readiness"))
       << ",\"preview_digest\":" << json_quote(preview_digest)
       << ",\"expected_preview_digest_verified\":"
       << bool_json(expected_preview_digest_supplied)
       << ",\"paper_online_readiness_contract_ready\":"
       << bool_json(contract_ready)
       << ",\"paper_online_readiness_fact_written\":" << bool_json(should_write)
       << ",\"paper_online_readiness_fact_path\":"
       << json_quote(fact_path.string())
       << ",\"paper_online_readiness_fact_digest\":" << json_quote(fact_digest)
       << ",\"paper_online_readiness_fact_ref\":"
       << json_quote(display::digest_ref("por", fact_digest))
       << ",\"paper_online_readiness_file_digest\":" << json_quote(file_digest)
       << ",\"policy_acceptance_fact_digest\":"
       << json_quote(readiness_fact.policy_acceptance_fact_digest)
       << ",\"policy_acceptance_fact_ref\":"
       << json_quote(display::digest_ref(
              "acc", readiness_fact.policy_acceptance_fact_digest))
       << ",\"accepted_policy_id\":"
       << json_quote(readiness_fact.accepted_policy_id)
       << ",\"accepted_actor_checkpoint_digest\":"
       << json_quote(readiness_fact.accepted_actor_checkpoint_digest)
       << ",\"execution_profile_digest\":"
       << json_quote(readiness_fact.execution_profile_digest)
       << ",\"locked_execution_profile_digest\":"
       << json_quote(readiness_fact.locked_execution_profile_digest)
       << ",\"paper_online_environment_contract_id\":"
       << json_quote(readiness_fact.paper_online_environment_contract_id)
       << ",\"market_data_staleness_policy_id\":"
       << json_quote(readiness_fact.market_data_staleness_policy_id)
       << ",\"max_market_data_staleness_ms\":"
       << readiness_fact.max_market_data_staleness_ms
       << ",\"missing_direct_pair_policy\":"
       << json_quote(readiness_fact.missing_direct_pair_policy)
       << ",\"issues\":" << string_array_json(all_issues)
       << ",\"authority\":{\"environment_emits_paper_online_readiness_fact\":"
          "true,"
          "\"environment_runs_paper_online_session\":false,"
          "\"environment_executes_broker_orders\":false,"
          "\"environment_executes_live_capital\":false,"
          "\"environment_claims_market_readiness\":false,"
          "\"environment_claims_deployment_readiness\":false,"
          "\"environment_proves_lattice_target\":false}"
       << ",\"next_safe_actions\":[\"inspect_paper_online_readiness_fact\","
          "\"evaluate_paper_online_readiness_contract_ready\"]}";
  *out = json.str();
  return true;
}

struct paper_online_session_admission_request_t {
  fs::path readiness_job_dir{};
  std::string admission_id{};
  std::string readiness_target_id{};
  std::string readiness_proof_certificate_digest{};
  std::string expected_readiness_fact_digest{};
  std::int64_t readiness_proof_checked_at_ms{0};
  std::int64_t session_admission_requested_at_ms{0};
  std::int64_t max_readiness_proof_age_ms{0};
  std::string source{"admission_request"};
};

[[nodiscard]] bool
validate_object_keys(const std::map<std::string, std::string> &fields,
                     const std::set<std::string> &allowed,
                     std::string_view label, std::string *err) {
  for (const auto &[key, _] : fields) {
    (void)_;
    if (allowed.find(key) == allowed.end()) {
      *err = std::string(label) + " has unknown field: " + key;
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool required_object_fields(
    const std::map<std::string, std::string> &fields, std::string_view key,
    std::map<std::string, std::string> *out, std::string *err) {
  const auto raw = md::optional_raw(fields, std::string(key));
  if (!raw.has_value()) {
    *err = "missing required object field: " + std::string(key);
    return false;
  }
  try {
    *out = object_fields_no_duplicates(*raw, key);
    return true;
  } catch (const std::exception &ex) {
    *err = std::string(key) + " must be an object: " + ex.what();
    return false;
  }
}

[[nodiscard]] bool parse_paper_online_session_admission_request(
    const std::string &request_json,
    paper_online_session_admission_request_t *out, std::string *err) {
  if (out == nullptr) {
    *err = "paper-online session admission request output is null";
    return false;
  }
  std::map<std::string, std::string> request;
  try {
    request = object_fields_no_duplicates(request_json, "admission_request");
  } catch (const std::exception &ex) {
    *err = std::string("admission_request must be an object: ") + ex.what();
    return false;
  }
  if (!validate_object_keys(request, {"schema", "readiness", "session"},
                            "admission_request", err)) {
    return false;
  }
  const std::string schema = md::optional_string(request, "schema");
  if (!schema.empty() &&
      schema != "kikijyeba.environment.paper_online_session_admission_request."
                "v1") {
    *err = "unsupported admission_request schema: " + schema;
    return false;
  }

  std::map<std::string, std::string> readiness;
  std::map<std::string, std::string> session;
  if (!required_object_fields(request, "readiness", &readiness, err) ||
      !required_object_fields(request, "session", &session, err)) {
    return false;
  }
  if (!validate_object_keys(readiness,
                            {"job_dir", "target_id", "proof_certificate_digest",
                             "proof_checked_at_ms", "max_proof_age_ms",
                             "readiness_fact_digest"},
                            "admission_request.readiness", err) ||
      !validate_object_keys(session, {"admission_id", "requested_at_ms"},
                            "admission_request.session", err)) {
    return false;
  }

  std::string job_dir_raw;
  paper_online_session_admission_request_t parsed{};
  if (!required_string(readiness, "job_dir", &job_dir_raw, err) ||
      !required_string(readiness, "target_id", &parsed.readiness_target_id,
                       err) ||
      !required_string(readiness, "proof_certificate_digest",
                       &parsed.readiness_proof_certificate_digest, err) ||
      !required_i64(readiness, "proof_checked_at_ms",
                    &parsed.readiness_proof_checked_at_ms, err) ||
      !required_i64(readiness, "max_proof_age_ms",
                    &parsed.max_readiness_proof_age_ms, err) ||
      !required_string(session, "admission_id", &parsed.admission_id, err) ||
      !required_i64(session, "requested_at_ms",
                    &parsed.session_admission_requested_at_ms, err)) {
    return false;
  }
  if (trim_ascii(job_dir_raw).empty()) {
    *err = "admission_request.readiness.job_dir must be non-empty";
    return false;
  }
  parsed.readiness_job_dir = normalize_path(fs::path(job_dir_raw));
  parsed.expected_readiness_fact_digest =
      md::optional_string(readiness, "readiness_fact_digest");
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] bool admission_request_from_args(
    const std::map<std::string, std::string> &args, environment_context_t *ctx,
    paper_online_session_admission_request_t *out, std::string *err) {
  const bool has_request =
      md::optional_raw(args, "admission_request").has_value();
  const bool has_request_path =
      md::optional_raw(args, "admission_request_path").has_value();
  if (has_request == has_request_path) {
    *err = "provide exactly one of admission_request or admission_request_path";
    return false;
  }

  std::string request_json;
  std::string source;
  if (has_request) {
    request_json = *md::optional_raw(args, "admission_request");
    source = "admission_request";
  } else {
    const std::string request_path_raw =
        md::optional_string(args, "admission_request_path");
    if (trim_ascii(request_path_raw).empty()) {
      *err = "admission_request_path must be non-empty";
      return false;
    }
    const fs::path request_path = normalize_path(fs::path(request_path_raw));
    if (!job_path_allowed(ctx->policy, request_path)) {
      *err =
          "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_REQUEST_PATH_DENIED: "
          "admission_request_path is outside Environment allowed_job_roots: " +
          request_path.string();
      return false;
    }
    std::string read_error;
    if (!read_text_file(request_path, &request_json, &read_error)) {
      *err =
          "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_REQUEST_READ_FAILED: " +
          read_error;
      return false;
    }
    source = request_path.string();
  }

  if (!parse_paper_online_session_admission_request(request_json, out, err)) {
    return false;
  }
  out->source = std::move(source);
  if (!job_path_allowed(ctx->policy, out->readiness_job_dir)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_READINESS_JOB_DIR_DENIED: "
           "readiness.job_dir is outside Environment allowed_job_roots: " +
           out->readiness_job_dir.string();
    return false;
  }
  return true;
}

[[nodiscard]] kenv::paper_online_readiness_evidence_t
paper_online_readiness_evidence_from_fact(
    const exposure::lattice_paper_online_readiness_fact_t &fact,
    const paper_online_session_admission_request_t &request,
    const std::vector<std::string> &fact_issues) {
  kenv::paper_online_readiness_evidence_t evidence{};
  evidence.readiness_target_id = request.readiness_target_id;
  evidence.readiness_contract_id = fact.readiness_contract_id;
  evidence.readiness_fact_schema_id = fact.schema;
  evidence.readiness_fact_digest =
      exposure::paper_online_readiness_fact_digest(fact);
  evidence.readiness_proof_certificate_digest =
      request.readiness_proof_certificate_digest;
  evidence.readiness_id = fact.readiness_id;
  evidence.accepted_policy_id = fact.accepted_policy_id;
  evidence.accepted_actor_checkpoint_digest =
      fact.accepted_actor_checkpoint_digest;
  evidence.accepted_checkpoint_digest = fact.accepted_checkpoint_digest;
  evidence.execution_profile_digest = fact.execution_profile_digest;
  evidence.locked_execution_profile_digest =
      fact.locked_execution_profile_digest;
  evidence.accounting_numeraire_node_id = fact.accounting_numeraire_node_id;
  evidence.paper_online_profile_digest = fact.paper_online_profile_digest;
  evidence.direct_edge_universe_digest = fact.direct_edge_universe_digest;
  evidence.paper_online_environment_contract_id =
      fact.paper_online_environment_contract_id;
  evidence.session_state_schema_id = fact.session_state_schema_id;
  evidence.reward_contract_id = fact.reward_contract_id;
  evidence.direct_edge_universe_validation_policy_id =
      fact.direct_edge_universe_validation_policy_id;
  evidence.missing_direct_pair_policy = fact.missing_direct_pair_policy;
  evidence.synthetic_execution_market_policy_id =
      fact.synthetic_execution_market_policy_id;
  evidence.persistent_paper_ledger_recovery_policy_id =
      fact.persistent_paper_ledger_recovery_policy_id;
  evidence.paper_ledger_storage_policy_id = fact.paper_ledger_storage_policy_id;
  evidence.session_lifecycle_policy_id = fact.session_lifecycle_policy_id;
  evidence.idempotency_policy_id = fact.idempotency_policy_id;
  evidence.execution_intent_id_policy = fact.execution_intent_id_policy;
  evidence.duplicate_action_policy = fact.duplicate_action_policy;
  evidence.duplicate_execution_intent_policy =
      fact.duplicate_execution_intent_policy;
  evidence.clock_policy_id = fact.clock_policy_id;
  evidence.timestamp_policy_id = fact.timestamp_policy_id;
  evidence.market_data_staleness_policy_id =
      fact.market_data_staleness_policy_id;
  evidence.reward_report_artifact_policy_id =
      fact.reward_report_artifact_policy_id;
  evidence.operator_abort_policy_id = fact.operator_abort_policy_id;
  evidence.kill_switch_policy_id = fact.kill_switch_policy_id;
  evidence.max_market_data_staleness_ms = fact.max_market_data_staleness_ms;
  evidence.clock_skew_tolerance_ms = fact.clock_skew_tolerance_ms;
  evidence.readiness_proof_checked_at_ms =
      request.readiness_proof_checked_at_ms;
  evidence.session_admission_requested_at_ms =
      request.session_admission_requested_at_ms;
  evidence.max_readiness_proof_age_ms = request.max_readiness_proof_age_ms;
  evidence.paper_online_readiness_contract_ready =
      fact_issues.empty() &&
      request.readiness_target_id == kenv::kPaperOnlineReadinessTargetReadyV1 &&
      !trim_ascii(request.readiness_proof_certificate_digest).empty();
  evidence.readiness_proof_fresh =
      request.readiness_proof_checked_at_ms > 0 &&
      request.session_admission_requested_at_ms >=
          request.readiness_proof_checked_at_ms &&
      request.max_readiness_proof_age_ms > 0 &&
      request.session_admission_requested_at_ms -
              request.readiness_proof_checked_at_ms <=
          request.max_readiness_proof_age_ms;
  evidence.policy_acceptance_ready = fact.policy_acceptance_ready;
  evidence.tsodao_settings_protection_ready =
      fact.tsodao_settings_protection_ready;
  evidence.accepted_policy_bound = fact.accepted_policy_bound;
  evidence.protected_settings_bound = fact.protected_settings_bound;
  evidence.direct_edge_universe_validated = fact.direct_edge_universe_validated;
  evidence.locked_execution_profile_bound = fact.locked_execution_profile_bound;
  evidence.persistent_paper_ledger_recovery_bound =
      fact.persistent_paper_ledger_recovery_bound;
  evidence.idempotency_bound = fact.idempotency_bound;
  evidence.duplicate_action_protection_bound =
      fact.duplicate_action_protection_bound;
  evidence.session_lifecycle_bound = fact.session_lifecycle_bound;
  evidence.clock_timestamp_policy_bound = fact.clock_timestamp_policy_bound;
  evidence.market_data_staleness_bound = fact.market_data_staleness_bound;
  evidence.reward_report_artifact_path_bound =
      fact.reward_report_artifact_path_bound;
  evidence.operator_abort_bound = fact.operator_abort_bound;
  evidence.kill_switch_bound = fact.kill_switch_bound;
  evidence.synthetic_execution_markets_allowed =
      fact.synthetic_execution_markets_allowed;
  evidence.numeraire_fallback_allowed = fact.numeraire_fallback_allowed;
  evidence.paper_online_execution_allowed = fact.paper_online_execution_allowed;
  evidence.live_execution_allowed = fact.live_execution_allowed;
  evidence.broker_execution_allowed = fact.broker_execution_allowed;
  evidence.direct_policy_to_broker_allowed =
      fact.direct_policy_to_broker_allowed;
  evidence.direct_policy_to_cajtucu_allowed =
      fact.direct_policy_to_cajtucu_allowed;
  evidence.artifact_evidence = fact.artifact_evidence;
  evidence.visibility_only = fact.visibility_only;
  evidence.paper_online_readiness_artifact =
      fact.paper_online_readiness_artifact;
  evidence.session_runner_authority = false;
  evidence.policy_selection_authority =
      fact.policy_selector || fact.allocation_authority;
  evidence.checkpoint_selection_authority = fact.checkpoint_selector;
  return evidence;
}

[[nodiscard]] std::string canonical_paper_online_session_admission_text(
    const paper_online_session_admission_request_t &request,
    const kenv::paper_online_session_contract_t &contract,
    const kenv::paper_online_readiness_evidence_t &evidence) {
  std::ostringstream out;
  out << "schema=kikijyeba.lattice.paper_online_session_admission.v1\n";
  out << "contract_id=" << contract.contract_id << "\n";
  out << "admission_id=" << request.admission_id << "\n";
  out << "readiness_job_dir=" << request.readiness_job_dir.string() << "\n";
  out << "readiness_target_id=" << evidence.readiness_target_id << "\n";
  out << "readiness_contract_id=" << evidence.readiness_contract_id << "\n";
  out << "readiness_fact_schema_id=" << evidence.readiness_fact_schema_id
      << "\n";
  out << "readiness_fact_digest=" << evidence.readiness_fact_digest << "\n";
  out << "readiness_proof_certificate_digest="
      << evidence.readiness_proof_certificate_digest << "\n";
  out << "readiness_proof_checked_at_ms="
      << evidence.readiness_proof_checked_at_ms << "\n";
  out << "session_admission_requested_at_ms="
      << evidence.session_admission_requested_at_ms << "\n";
  out << "max_readiness_proof_age_ms=" << evidence.max_readiness_proof_age_ms
      << "\n";
  out << "accepted_policy_id=" << evidence.accepted_policy_id << "\n";
  out << "accepted_actor_checkpoint_digest="
      << evidence.accepted_actor_checkpoint_digest << "\n";
  out << "execution_profile_digest=" << evidence.execution_profile_digest
      << "\n";
  out << "locked_execution_profile_digest="
      << evidence.locked_execution_profile_digest << "\n";
  out << "paper_online_profile_digest=" << evidence.paper_online_profile_digest
      << "\n";
  out << "direct_edge_universe_digest=" << evidence.direct_edge_universe_digest
      << "\n";
  out << "session_state_schema_id=" << evidence.session_state_schema_id << "\n";
  out << "market_data_staleness_policy_id="
      << evidence.market_data_staleness_policy_id << "\n";
  out << "operator_abort_policy_id=" << evidence.operator_abort_policy_id
      << "\n";
  out << "kill_switch_policy_id=" << evidence.kill_switch_policy_id << "\n";
  return out.str();
}

[[nodiscard]] bool handle_certify_paper_online_session_admission(
    const std::map<std::string, std::string> &args, environment_context_t *ctx,
    std::string *out, std::string *err) {
  md::validate_fields(args,
                      {"mode", "admission_request", "admission_request_path",
                       "expected_preview_digest"},
                      {"mode"},
                      "hero.environment.certify.paper_online_session_"
                      "admission");
  const std::string requested_mode =
      lowercase_ascii(md::optional_string(args, "mode"));
  if (requested_mode != "check" && requested_mode != "issue") {
    *err = "hero.environment.certify.paper_online_session_admission supports "
           "mode=check|issue";
    return false;
  }
  const bool should_write = requested_mode == "issue";
  if (should_write &&
      !policy_bool_or(ctx->policy, "allow_certify_issue", false)) {
    *err = "E_ENVIRONMENT_CERTIFY_ISSUE_DENIED: allow_certify_issue is false";
    return false;
  }

  paper_online_session_admission_request_t request{};
  if (!admission_request_from_args(args, ctx, &request, err)) {
    return false;
  }

  exposure::lattice_paper_online_readiness_fact_t readiness_fact{};
  if (!load_single_paper_online_readiness_fact(request.readiness_job_dir,
                                               &readiness_fact, err)) {
    return false;
  }
  const auto fact_issues =
      exposure::paper_online_readiness_fact_issues(readiness_fact);
  const auto contract = kenv::default_paper_online_session_contract();
  auto evidence = paper_online_readiness_evidence_from_fact(
      readiness_fact, request, fact_issues);
  std::vector<std::string> issues =
      kenv::paper_online_session_admission_issues(contract, evidence);
  std::vector<std::string> request_issues;
  if (!request.expected_readiness_fact_digest.empty() &&
      request.expected_readiness_fact_digest !=
          evidence.readiness_fact_digest) {
    request_issues.emplace_back("readiness_fact_digest_mismatch");
  }
  issues.insert(issues.end(), fact_issues.begin(), fact_issues.end());
  issues.insert(issues.end(), request_issues.begin(), request_issues.end());
  const bool admission_ready = issues.empty();

  const std::string preview_text =
      canonical_paper_online_session_admission_text(request, contract,
                                                    evidence);
  const std::string preview_digest =
      certify_preview_digest("paper_online_session_admission", preview_text);
  const bool expected_preview_digest_supplied =
      md::optional_raw(args, "expected_preview_digest").has_value();
  const std::string expected_preview_digest =
      md::optional_string(args, "expected_preview_digest");
  if (!verify_expected_preview_digest(
          "paper_online_session_admission", requested_mode,
          expected_preview_digest_supplied, expected_preview_digest,
          preview_digest, err)) {
    return false;
  }
  if (should_write && !admission_ready) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_INVALID: " +
           string_array_json(issues);
    return false;
  }
  const fs::path fact_path =
      request.readiness_job_dir / "lattice.paper_online_session_admission.fact";
  if (should_write && fs::exists(fact_path)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_FACT_EXISTS: "
           "refusing to overwrite " +
           fact_path.string();
    return false;
  }
  std::string file_digest;
  if (should_write) {
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        fact_path, preview_text);
    file_digest = file_digest_or_empty(
        fact_path, "kikijyeba.lattice.paper_online_session_admission.v1");
  }
  const std::string admission_request_digest = marshal::marshal_digest_for_text(
      "kikijyeba.environment.paper_online_session_admission.request.v1",
      preview_text);
  const std::vector<std::string> next_actions =
      admission_ready
          ? std::vector<
                std::string>{"prepare_marshal_paper_online_session_handoff"}
          : std::vector<std::string>{
                "inspect_paper_online_readiness_fact",
                "refresh_paper_online_readiness_proof",
                "fix_paper_online_session_admission_request"};

  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.certify."
          "paper_online_session_admission\""
       << ",\"requested_mode\":" << json_quote(requested_mode)
       << ",\"check_only\":" << bool_json(!should_write)
       << ",\"schema_version\":\"kikijyeba.environment.paper_online_session_"
          "admission_packet.v1\""
       << ",\"admission_ready\":" << bool_json(admission_ready)
       << ",\"paper_online_session_admission_fact_written\":"
       << bool_json(should_write)
       << ",\"paper_online_session_admission_fact_path\":"
       << json_quote(fact_path.string())
       << ",\"paper_online_session_admission_file_digest\":"
       << json_quote(file_digest)
       << ",\"admission_id\":" << json_quote(request.admission_id)
       << ",\"admission_request_source\":" << json_quote(request.source)
       << ",\"admission_request_digest\":"
       << json_quote(admission_request_digest) << ",\"preview_digest_domain\":"
       << json_quote(
              certify_preview_digest_domain("paper_online_session_admission"))
       << ",\"preview_digest\":" << json_quote(preview_digest)
       << ",\"expected_preview_digest_verified\":"
       << bool_json(expected_preview_digest_supplied)
       << ",\"readiness_job_dir\":"
       << json_quote(request.readiness_job_dir.string())
       << ",\"readiness_target_id\":"
       << json_quote(evidence.readiness_target_id)
       << ",\"readiness_fact_digest\":"
       << json_quote(evidence.readiness_fact_digest)
       << ",\"readiness_fact_digest_match\":"
       << bool_json(request.expected_readiness_fact_digest.empty() ||
                    request.expected_readiness_fact_digest ==
                        evidence.readiness_fact_digest)
       << ",\"readiness_proof_certificate_digest\":"
       << json_quote(evidence.readiness_proof_certificate_digest)
       << ",\"freshness\":{\"proof_checked_at_ms\":"
       << evidence.readiness_proof_checked_at_ms
       << ",\"session_requested_at_ms\":"
       << evidence.session_admission_requested_at_ms
       << ",\"max_proof_age_ms\":" << evidence.max_readiness_proof_age_ms
       << ",\"fresh\":" << bool_json(evidence.readiness_proof_fresh) << "}"
       << ",\"bound\":{\"accepted_policy_id\":"
       << json_quote(evidence.accepted_policy_id)
       << ",\"accepted_actor_checkpoint_digest\":"
       << json_quote(evidence.accepted_actor_checkpoint_digest)
       << ",\"accepted_checkpoint_digest\":"
       << json_quote(evidence.accepted_checkpoint_digest)
       << ",\"execution_profile_digest\":"
       << json_quote(evidence.execution_profile_digest)
       << ",\"locked_execution_profile_digest\":"
       << json_quote(evidence.locked_execution_profile_digest)
       << ",\"paper_online_profile_digest\":"
       << json_quote(evidence.paper_online_profile_digest)
       << ",\"direct_edge_universe_digest\":"
       << json_quote(evidence.direct_edge_universe_digest)
       << ",\"session_state_schema_id\":"
       << json_quote(evidence.session_state_schema_id) << "}"
       << ",\"readiness_fact_issues\":" << string_array_json(fact_issues)
       << ",\"request_issues\":" << string_array_json(request_issues)
       << ",\"issues\":" << string_array_json(issues)
       << ",\"next_safe_actions\":" << string_array_json(next_actions)
       << ",\"authority\":{\"environment_checks_paper_online_session_"
          "admission\":true,"
          "\"environment_writes_session_state\":false,"
          "\"environment_runs_paper_online_session\":false,"
          "\"environment_starts_market_streams\":false,"
          "\"environment_executes_cajtucu_paper\":false,"
          "\"environment_executes_broker_orders\":false,"
          "\"environment_executes_live_capital\":false,"
          "\"environment_selects_policy\":false,"
          "\"environment_selects_checkpoint\":false,"
          "\"environment_proves_lattice_target\":false}}";
  *out = json.str();
  return true;
}

struct paper_online_session_run_request_t {
  fs::path admission_job_dir{};
  fs::path session_root{};
  std::string session_id{};
  std::string expected_admission_fact_digest{};
  std::vector<std::string> target_node_ids{};
  std::int64_t requested_at_ms{0};
  std::int64_t max_steps{0};
  std::int64_t step_interval_ms{1000};
  std::int64_t market_data_receive_lag_ms{0};
  bool recover_persistent_ledger{true};
  std::int64_t operator_abort_at_step{-1};
  std::int64_t kill_switch_at_step{-1};
  std::int64_t duplicate_action_at_step{-1};
  std::int64_t duplicate_execution_intent_at_step{-1};
  std::string source{"session_request"};
};

[[nodiscard]] bool id_is_path_safe(std::string_view value) {
  if (trim_ascii(value).empty()) {
    return false;
  }
  return value.find('/') == std::string_view::npos &&
         value.find('\\') == std::string_view::npos &&
         value.find("..") == std::string_view::npos;
}

[[nodiscard]] bool
parse_paper_online_session_run_request(const std::string &request_json,
                                       paper_online_session_run_request_t *out,
                                       std::string *err) {
  if (out == nullptr) {
    *err = "paper-online session request output is null";
    return false;
  }
  std::map<std::string, std::string> request;
  try {
    request = object_fields_no_duplicates(request_json, "session_request");
  } catch (const std::exception &ex) {
    *err = std::string("session_request must be an object: ") + ex.what();
    return false;
  }
  if (!validate_object_keys(request, {"schema", "admission", "session"},
                            "session_request", err)) {
    return false;
  }
  const std::string schema = md::optional_string(request, "schema");
  if (!schema.empty() &&
      schema != "kikijyeba.environment.paper_online_session_run_request.v1") {
    *err = "unsupported session_request schema: " + schema;
    return false;
  }

  std::map<std::string, std::string> admission;
  std::map<std::string, std::string> session;
  if (!required_object_fields(request, "admission", &admission, err) ||
      !required_object_fields(request, "session", &session, err)) {
    return false;
  }
  if (!validate_object_keys(admission, {"job_dir", "admission_fact_digest"},
                            "session_request.admission", err) ||
      !validate_object_keys(
          session,
          {"session_id", "session_root", "requested_at_ms", "max_steps",
           "step_interval_ms", "market_data_receive_lag_ms", "target_node_ids",
           "recover_persistent_ledger", "operator_abort_at_step",
           "kill_switch_at_step", "duplicate_action_at_step",
           "duplicate_execution_intent_at_step"},
          "session_request.session", err)) {
    return false;
  }

  std::string admission_job_dir_raw;
  std::string session_root_raw;
  paper_online_session_run_request_t parsed{};
  if (!required_string(admission, "job_dir", &admission_job_dir_raw, err) ||
      !required_string(session, "session_id", &parsed.session_id, err) ||
      !required_string(session, "session_root", &session_root_raw, err) ||
      !required_i64(session, "requested_at_ms", &parsed.requested_at_ms, err) ||
      !required_i64(session, "max_steps", &parsed.max_steps, err)) {
    return false;
  }
  parsed.expected_admission_fact_digest =
      md::optional_string(admission, "admission_fact_digest");
  parsed.step_interval_ms =
      md::optional_i64(session, "step_interval_ms", parsed.step_interval_ms);
  parsed.market_data_receive_lag_ms = md::optional_i64(
      session, "market_data_receive_lag_ms", parsed.market_data_receive_lag_ms);
  parsed.target_node_ids =
      md::optional_string_array(session, "target_node_ids");
  parsed.recover_persistent_ledger = md::optional_bool(
      session, "recover_persistent_ledger", parsed.recover_persistent_ledger);
  parsed.operator_abort_at_step = md::optional_i64(
      session, "operator_abort_at_step", parsed.operator_abort_at_step);
  parsed.kill_switch_at_step = md::optional_i64(session, "kill_switch_at_step",
                                                parsed.kill_switch_at_step);
  parsed.duplicate_action_at_step = md::optional_i64(
      session, "duplicate_action_at_step", parsed.duplicate_action_at_step);
  parsed.duplicate_execution_intent_at_step =
      md::optional_i64(session, "duplicate_execution_intent_at_step",
                       parsed.duplicate_execution_intent_at_step);
  parsed.admission_job_dir = normalize_path(fs::path(admission_job_dir_raw));
  parsed.session_root = normalize_path(fs::path(session_root_raw));
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] bool session_request_from_args(
    const std::map<std::string, std::string> &args, environment_context_t *ctx,
    paper_online_session_run_request_t *out, std::string *err) {
  const bool has_request =
      md::optional_raw(args, "session_request").has_value();
  const bool has_request_path =
      md::optional_raw(args, "session_request_path").has_value();
  if (has_request == has_request_path) {
    *err = "provide exactly one of session_request or session_request_path";
    return false;
  }
  std::string request_json;
  std::string source;
  if (has_request) {
    request_json = *md::optional_raw(args, "session_request");
    source = "session_request";
  } else {
    const std::string request_path_raw =
        md::optional_string(args, "session_request_path");
    if (trim_ascii(request_path_raw).empty()) {
      *err = "session_request_path must be non-empty";
      return false;
    }
    const fs::path request_path = normalize_path(fs::path(request_path_raw));
    if (!job_path_allowed(ctx->policy, request_path)) {
      *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_REQUEST_PATH_DENIED: "
             "session_request_path is outside Environment allowed_job_roots: " +
             request_path.string();
      return false;
    }
    std::string read_error;
    if (!read_text_file(request_path, &request_json, &read_error)) {
      *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_REQUEST_READ_FAILED: " +
             read_error;
      return false;
    }
    source = request_path.string();
  }
  if (!parse_paper_online_session_run_request(request_json, out, err)) {
    return false;
  }
  out->source = std::move(source);
  if (!job_path_allowed(ctx->policy, out->admission_job_dir)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_JOB_DIR_DENIED: "
           "admission.job_dir is outside Environment allowed_job_roots: " +
           out->admission_job_dir.string();
    return false;
  }
  if (!job_path_allowed(ctx->policy, out->session_root)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ROOT_DENIED: session_root is "
           "outside Environment allowed_job_roots: " +
           out->session_root.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::string
target_weights_string(const std::vector<std::string> &target_node_ids) {
  std::ostringstream out;
  const double weight = target_node_ids.empty()
                            ? 0.0
                            : 1.0 / static_cast<double>(target_node_ids.size());
  out << std::setprecision(10);
  for (std::size_t i = 0; i < target_node_ids.size(); ++i) {
    if (i != 0) {
      out << ";";
    }
    out << target_node_ids[i] << ":" << weight;
  }
  return out.str();
}

[[nodiscard]] std::string artifact_digest_for_text(std::string_view schema,
                                                   std::string_view text) {
  return marshal::marshal_digest_for_text(std::string(schema),
                                          std::string(text));
}

[[nodiscard]] std::string line_count_digest(std::string_view schema,
                                            const fs::path &path) {
  return file_digest_or_empty(path, schema);
}

struct paper_online_session_artifact_packet_t {
  std::string session_manifest_text{};
  std::string session_state_text{};
  std::string session_events_text{};
  std::string market_events_text{};
  std::string action_intents_text{};
  std::string execution_intents_text{};
  std::string paper_ledger_text{};
  std::string reward_reports_text{};
  std::int64_t completed_step_count{0};
  std::int64_t market_event_count{0};
  std::int64_t action_intent_count{0};
  std::int64_t execution_intent_count{0};
  std::int64_t paper_fill_count{0};
  std::int64_t reward_report_count{0};
  std::int64_t stopped_at_ms{0};
  std::int64_t max_observed_staleness_ms{0};
  std::string terminal_state{"stopped"};
  bool operator_abort_triggered{false};
  bool kill_switch_triggered{false};
};

[[nodiscard]] paper_online_session_artifact_packet_t
build_paper_online_session_artifacts(
    const paper_online_session_run_request_t &request,
    const exposure::lattice_paper_online_readiness_fact_t &readiness_fact,
    const std::string &admission_fact_digest) {
  paper_online_session_artifact_packet_t packet{};
  const std::string target_weights =
      target_weights_string(request.target_node_ids);
  const std::string target_nodes = join_strings(request.target_node_ids, ",");
  std::set<std::string> action_digests;
  std::set<std::string> execution_intent_ids;

  packet.session_manifest_text =
      "schema=kikijyeba.paper_online.session_manifest.v1\n"
      "session_id=" +
      request.session_id +
      "\n"
      "admission_fact_digest=" +
      admission_fact_digest +
      "\n"
      "readiness_fact_digest=" +
      exposure::paper_online_readiness_fact_digest(readiness_fact) +
      "\n"
      "accepted_policy_id=" +
      readiness_fact.accepted_policy_id +
      "\n"
      "execution_backend_id=cajtucu.execution.paper.v1\n"
      "target_node_ids=" +
      target_nodes +
      "\n"
      "max_steps=" +
      std::to_string(request.max_steps) + "\n";

  packet.session_events_text =
      "schema=kikijyeba.paper_online.session_events.v1\n"
      "event=session_initialized|at_ms=" +
      std::to_string(request.requested_at_ms) +
      "\n"
      "event=session_admitted|at_ms=" +
      std::to_string(request.requested_at_ms) +
      "\n"
      "event=ledger_recovered|at_ms=" +
      std::to_string(request.requested_at_ms) +
      "\n"
      "event=streaming_started|at_ms=" +
      std::to_string(request.requested_at_ms) + "\n";
  packet.market_events_text =
      "schema=kikijyeba.paper_online.market_events.v1\n";
  packet.action_intents_text =
      "schema=kikijyeba.paper_online.action_intents.v1\n";
  packet.execution_intents_text =
      "schema=kikijyeba.paper_online.execution_intents.v1\n";
  packet.paper_ledger_text = "schema=kikijyeba.paper_online.paper_ledger.v1\n"
                             "ledger_recovered=true\n";
  packet.reward_reports_text =
      "schema=kikijyeba.paper_online.reward_reports.v1\n";

  for (std::int64_t step = 0; step < request.max_steps; ++step) {
    if (request.kill_switch_at_step == step) {
      packet.kill_switch_triggered = true;
      packet.terminal_state = "kill_switch_triggered";
      packet.session_events_text +=
          "event=kill_switch_triggered|step=" + std::to_string(step) +
          "|at_ms=" +
          std::to_string(request.requested_at_ms +
                         step * request.step_interval_ms) +
          "\n";
      break;
    }
    if (request.operator_abort_at_step == step) {
      packet.operator_abort_triggered = true;
      packet.terminal_state = "aborted";
      packet.session_events_text +=
          "event=operator_abort|step=" + std::to_string(step) + "|at_ms=" +
          std::to_string(request.requested_at_ms +
                         step * request.step_interval_ms) +
          "\n";
      break;
    }

    const auto exchange_time =
        request.requested_at_ms + step * request.step_interval_ms;
    const auto receive_time =
        exchange_time + request.market_data_receive_lag_ms;
    packet.max_observed_staleness_ms = std::max(
        packet.max_observed_staleness_ms, request.market_data_receive_lag_ms);
    const std::string action_seed = request.session_id + "|action|" +
                                    std::to_string(step) + "|" + target_weights;
    const std::string action_digest = marshal::marshal_digest_for_text(
        "kikijyeba.paper_online.action_intent.v1", action_seed);
    const std::string execution_intent_id = marshal::marshal_digest_for_text(
        "kikijyeba.paper_online.execution_intent_id.v1",
        request.session_id + "|" + action_digest + "|" + std::to_string(step));
    action_digests.insert(action_digest);
    execution_intent_ids.insert(execution_intent_id);

    packet.market_events_text +=
        "market_event_" + std::to_string(step) +
        "=event_id:" + request.session_id + ":market:" + std::to_string(step) +
        "|exchange_time_ms:" + std::to_string(exchange_time) +
        "|received_at_ms:" + std::to_string(receive_time) +
        "|target_node_ids:" + target_nodes + "\n";
    packet.action_intents_text +=
        "action_" + std::to_string(step) + "=action_digest:" + action_digest +
        "|policy_id:" + readiness_fact.accepted_policy_id +
        "|target_node_weights:" + target_weights + "\n";
    packet.execution_intents_text +=
        "execution_intent_" + std::to_string(step) +
        "=intent_id:" + execution_intent_id +
        "|backend:cajtucu.execution.paper.v1|action_digest:" + action_digest +
        "\n";
    packet.paper_ledger_text +=
        "ledger_step_" + std::to_string(step) +
        "=fill_status:filled|backend:cajtucu.execution.paper.v1|equity:"
        "1000.0|target_node_weights:" +
        target_weights + "\n";
    packet.reward_reports_text +=
        "reward_" + std::to_string(step) +
        "=reward:0.0|reward_contract_id:" + readiness_fact.reward_contract_id +
        "\n";
    packet.session_events_text +=
        "event=action_recorded|step=" + std::to_string(step) +
        "\n"
        "event=paper_executed|step=" +
        std::to_string(step) +
        "\n"
        "event=reward_reported|step=" +
        std::to_string(step) + "\n";
    ++packet.completed_step_count;
  }

  packet.market_event_count = packet.completed_step_count;
  packet.action_intent_count = packet.completed_step_count;
  packet.execution_intent_count = packet.completed_step_count;
  packet.paper_fill_count = packet.completed_step_count;
  packet.reward_report_count = packet.completed_step_count;
  if (packet.terminal_state == "stopped") {
    packet.session_events_text += "event=stopping\n";
  }
  packet.session_events_text += "event=" + packet.terminal_state + "\n";
  packet.stopped_at_ms =
      request.requested_at_ms +
      std::max<std::int64_t>(1, packet.completed_step_count) *
          request.step_interval_ms;
  packet.session_state_text = "schema=kikijyeba.paper_online.session_state.v1\n"
                              "session_id=" +
                              request.session_id +
                              "\n"
                              "terminal_state=" +
                              packet.terminal_state +
                              "\n"
                              "completed_step_count=" +
                              std::to_string(packet.completed_step_count) +
                              "\n"
                              "ledger_recovered=true\n"
                              "broker_execution_allowed=false\n"
                              "live_execution_allowed=false\n";
  return packet;
}

[[nodiscard]] std::vector<std::string> validate_paper_online_session_request(
    const paper_online_session_run_request_t &request,
    const std::map<std::string, std::string> &admission_fields,
    const std::string &admission_fact_digest,
    const exposure::lattice_paper_online_readiness_fact_t &readiness_fact,
    const std::vector<std::string> &readiness_fact_issues,
    const environment_policy_t &policy) {
  std::vector<std::string> issues;
  const auto add_if = [&](bool condition, const char *issue) {
    if (condition) {
      issues.emplace_back(issue);
    }
  };
  add_if(!id_is_path_safe(request.session_id), "invalid_session_id");
  add_if(request.requested_at_ms <= 0, "invalid_session_requested_at_ms");
  const auto max_policy_steps =
      policy_i64_or(policy, "paper_online_session_max_steps", 64);
  add_if(request.max_steps <= 0, "invalid_session_max_steps");
  add_if(max_policy_steps > 0 && request.max_steps > max_policy_steps,
         "session_max_steps_exceeds_environment_policy");
  add_if(request.step_interval_ms <= 0, "invalid_step_interval_ms");
  add_if(request.market_data_receive_lag_ms < 0,
         "invalid_market_data_receive_lag_ms");
  add_if(request.target_node_ids.empty(), "missing_target_node_ids");
  add_if(!request.recover_persistent_ledger,
         "persistent_paper_ledger_recovery_missing");
  add_if(request.duplicate_action_at_step >= 0,
         "duplicate_action_digest_rejected");
  add_if(request.duplicate_execution_intent_at_step >= 0,
         "duplicate_execution_intent_id_rejected");
  if (!request.expected_admission_fact_digest.empty() &&
      request.expected_admission_fact_digest != admission_fact_digest) {
    issues.emplace_back("admission_fact_digest_mismatch");
  }
  if (map_value(admission_fields, "schema") !=
      "kikijyeba.lattice.paper_online_session_admission.v1") {
    issues.emplace_back("unsupported_admission_fact_schema");
  }
  if (map_value(admission_fields, "readiness_fact_digest") !=
      exposure::paper_online_readiness_fact_digest(readiness_fact)) {
    issues.emplace_back("admission_readiness_fact_digest_mismatch");
  }
  if (map_value(admission_fields, "readiness_proof_certificate_digest")
          .empty()) {
    issues.emplace_back("missing_admission_readiness_proof_certificate_digest");
  }
  if (!readiness_fact_issues.empty()) {
    issues.emplace_back("paper_online_readiness_fact_not_ready");
  }
  if (request.market_data_receive_lag_ms >
      readiness_fact.max_market_data_staleness_ms) {
    issues.emplace_back("market_data_staleness_exceeded");
  }
  if (std::find(request.target_node_ids.begin(), request.target_node_ids.end(),
                readiness_fact.accounting_numeraire_node_id) ==
      request.target_node_ids.end()) {
    issues.emplace_back("target_nodes_missing_accounting_numeraire");
  }
  std::set<std::string> unique_nodes(request.target_node_ids.begin(),
                                     request.target_node_ids.end());
  if (unique_nodes.size() != request.target_node_ids.size()) {
    issues.emplace_back("duplicate_target_node_id");
  }
  if (readiness_fact.broker_execution_allowed ||
      readiness_fact.live_execution_allowed ||
      readiness_fact.direct_policy_to_broker_allowed) {
    issues.emplace_back("readiness_fact_authority_drift");
  }
  return issues;
}

[[nodiscard]] bool
handle_paper_online_session(const std::map<std::string, std::string> &args,
                            environment_context_t *ctx, std::string *out,
                            std::string *err) {
  md::validate_fields(args,
                      {"mode", "session_request", "session_request_path",
                       "include_machine_payload"},
                      {"mode"}, "hero.environment.paper_online.session");
  const std::string requested_mode =
      lowercase_ascii(md::optional_string(args, "mode"));
  if (requested_mode != "validate" && requested_mode != "run") {
    *err = "hero.environment.paper_online.session supports mode=validate|run";
    return false;
  }
  const bool should_run = requested_mode == "run";
  if (should_run &&
      !policy_bool_or(ctx->policy, "allow_paper_online_session_run", false)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_RUN_DENIED: "
           "allow_paper_online_session_run is false";
    return false;
  }

  paper_online_session_run_request_t request{};
  if (!session_request_from_args(args, ctx, &request, err)) {
    return false;
  }
  const fs::path admission_fact_path =
      request.admission_job_dir / "lattice.paper_online_session_admission.fact";
  std::string admission_text;
  std::string read_error;
  if (!read_text_file(admission_fact_path, &admission_text, &read_error)) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_ADMISSION_FACT_MISSING: " +
           read_error;
    return false;
  }
  const auto admission_fields =
      marshal::rollout_marshal_detail::read_kv_file(admission_fact_path);
  const std::string admission_fact_digest = artifact_digest_for_text(
      "kikijyeba.lattice.paper_online_session_admission.v1", admission_text);

  exposure::lattice_paper_online_readiness_fact_t readiness_fact{};
  if (!load_single_paper_online_readiness_fact(request.admission_job_dir,
                                               &readiness_fact, err)) {
    return false;
  }
  const auto readiness_fact_issues =
      exposure::paper_online_readiness_fact_issues(readiness_fact);
  const auto runner_contract =
      kenv::default_paper_online_session_runner_contract();
  auto issues =
      kenv::paper_online_session_runner_contract_issues(runner_contract);
  const auto request_issues = validate_paper_online_session_request(
      request, admission_fields, admission_fact_digest, readiness_fact,
      readiness_fact_issues, ctx->policy);
  issues.insert(issues.end(), request_issues.begin(), request_issues.end());
  const bool session_ready = issues.empty();
  if (should_run && !session_ready) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_REQUEST_INVALID: " +
           string_array_json(issues);
    return false;
  }
  if (should_run && fs::exists(request.session_root / "job.manifest")) {
    *err = "E_ENVIRONMENT_PAPER_ONLINE_SESSION_JOB_EXISTS: refusing to "
           "overwrite " +
           request.session_root.string();
    return false;
  }

  paper_online_session_artifact_packet_t packet{};
  fs::path session_fact_path =
      request.session_root / "lattice.paper_online_session.fact";
  std::string session_fact_digest;
  std::string exposure_fact_digest_value;
  bool artifacts_written = false;
  if (should_run) {
    packet = build_paper_online_session_artifacts(request, readiness_fact,
                                                  admission_fact_digest);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "session.manifest",
        packet.session_manifest_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "session.state", packet.session_state_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "session.events.lls",
        packet.session_events_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "market_events.lls", packet.market_events_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "action_intents.lls",
        packet.action_intents_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "execution_intents.lls",
        packet.execution_intents_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "paper_ledger.lls", packet.paper_ledger_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "reward_reports.lls",
        packet.reward_reports_text);

    const std::string session_job_id =
        request.session_id + ".paper_online_session";
    const std::string component_surface_digest =
        marshal::marshal_digest_for_text(
            "kikijyeba.environment.paper_online_session.operator_surface.v1",
            request.session_id + "|" + admission_fact_digest + "|" +
                join_strings(request.target_node_ids, ",") + "|" +
                std::to_string(request.max_steps));
    const std::string manifest_text =
        "manifest_format=kikijyeba.runtime.job_manifest.v1\n"
        "job_id=" +
        session_job_id +
        "\n"
        "job_kind=paper_online_session\n"
        "protocol_id=" +
        readiness_fact.protocol_id +
        "\n"
        "protocol_contract_fingerprint=" +
        readiness_fact.contract_fingerprint +
        "\n"
        "graph_order_fingerprint=" +
        readiness_fact.graph_order_fingerprint +
        "\n"
        "source_cursor_token=" +
        readiness_fact.source_cursor_token +
        "\n"
        "target_component_family_id=kikijyeba.paper_online.session\n"
        "component_assembly_fingerprint=" +
        component_surface_digest +
        "\n"
        "component_operator_surface_digest=" +
        component_surface_digest +
        "\n"
        "wave_id=paper_online_session\n"
        "wave_action=paper_online_session_run\n";
    const std::string state_text =
        "state_format=kikijyeba.runtime.job_state.v1\n"
        "status=completed\n"
        "session_id=" +
        request.session_id +
        "\n"
        "terminal_state=" +
        packet.terminal_state +
        "\n"
        "completed_step_count=" +
        std::to_string(packet.completed_step_count) + "\n";
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "job.manifest", manifest_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        request.session_root / "job.state", state_text);

    exposure::lattice_exposure_fact_t exposure_fact{};
    exposure_fact.contract_fingerprint = readiness_fact.contract_fingerprint;
    exposure_fact.protocol_id = readiness_fact.protocol_id;
    exposure_fact.graph_order_fingerprint =
        readiness_fact.graph_order_fingerprint;
    exposure_fact.source_cursor_token = readiness_fact.source_cursor_token;
    exposure_fact.split_policy_fingerprint =
        readiness_fact.split_policy_fingerprint;
    exposure_fact.component_assembly_fingerprint = component_surface_digest;
    exposure_fact.target_component_family_id = "kikijyeba.paper_online.session";
    exposure_fact.component_operator_surface_digest = component_surface_digest;
    exposure_fact.component_family_id = "kikijyeba.paper_online.session";
    exposure_fact.component_spawn_id = request.session_id;
    exposure_fact.component_spawn_label = "paper_online_session";
    exposure_fact.component_spawn_fingerprint = component_surface_digest;
    exposure_fact.report_schema_id = "kikijyeba.paper_online.session_report.v1";
    exposure_fact.report_schema_version = 1;
    exposure_fact.report_writer_id = "hero.environment.paper_online.session";
    exposure_fact.report_writer_version = "1";
    exposure_fact.job_id = session_job_id;
    exposure_fact.job_stable_id = request.session_id;
    exposure_fact.job_attempt_id = "attempt_000001";
    exposure_fact.job_attempt_policy = "refuse_overwrite";
    exposure_fact.wave_id = "paper_online_session";
    exposure_fact.job_status = "completed";
    exposure_fact.wave_action = "paper_online_session_run";
    exposure_fact.anchor_range = readiness_fact.anchor_range;
    exposure_fact.completed_anchor_range =
        readiness_fact.completed_anchor_range;
    exposure_fact.candidate_anchor_count = request.max_steps;
    exposure_fact.accepted_anchor_count = packet.completed_step_count;
    exposure_fact.accepted_anchor_fraction =
        request.max_steps == 0
            ? 0.0
            : static_cast<double>(packet.completed_step_count) /
                  static_cast<double>(request.max_steps);
    exposure_fact.anchor_domain_warning_level = "none";
    exposure_fact.split_name = readiness_fact.split_name;
    exposure_fact.split_role = readiness_fact.split_role;
    exposure_fact.use.observed_input = true;
    exposure_fact.use.evaluation_metric = true;
    exposure_fact.anchors_seen = packet.completed_step_count;
    exposure_fact.batches_seen = 1;
    exposure_fact.wave_pulses_completed = packet.completed_step_count;
    exposure_fact.valid_target_count = packet.completed_step_count;
    exposure_fact.valid_target_fraction =
        packet.completed_step_count > 0 ? 1.0 : 0.0;
    exposure_fact.run_data_kind = "paper_online_session";
    exposure_fact.readiness_scope = "paper_only";
    exposure::write_exposure_fact_sidecar(
        request.session_root / "lattice.exposure.fact", exposure_fact);
    exposure_fact_digest_value = exposure::exposure_fact_digest(exposure_fact);

    exposure::lattice_paper_online_session_fact_t session_fact{};
    session_fact.parent_exposure_fact_digest = exposure_fact_digest_value;
    session_fact.contract_fingerprint = readiness_fact.contract_fingerprint;
    session_fact.protocol_id = readiness_fact.protocol_id;
    session_fact.graph_order_fingerprint =
        readiness_fact.graph_order_fingerprint;
    session_fact.source_cursor_token = readiness_fact.source_cursor_token;
    session_fact.split_policy_fingerprint =
        readiness_fact.split_policy_fingerprint;
    session_fact.component_assembly_fingerprint = component_surface_digest;
    session_fact.target_component_family_id = "kikijyeba.paper_online.session";
    session_fact.job_id = session_job_id;
    session_fact.wave_id = "paper_online_session";
    session_fact.job_status = "completed";
    session_fact.wave_action = "paper_online_session_run";
    session_fact.split_name = readiness_fact.split_name;
    session_fact.split_role = readiness_fact.split_role;
    session_fact.anchor_range = readiness_fact.anchor_range;
    session_fact.completed_anchor_range = readiness_fact.completed_anchor_range;
    session_fact.session_id = request.session_id;
    session_fact.admission_id = map_value(admission_fields, "admission_id");
    session_fact.admission_fact_digest = admission_fact_digest;
    session_fact.readiness_fact_digest =
        exposure::paper_online_readiness_fact_digest(readiness_fact);
    session_fact.readiness_proof_certificate_digest =
        map_value(admission_fields, "readiness_proof_certificate_digest");
    session_fact.accepted_policy_id = readiness_fact.accepted_policy_id;
    session_fact.accepted_actor_checkpoint_digest =
        readiness_fact.accepted_actor_checkpoint_digest;
    session_fact.accepted_checkpoint_digest =
        readiness_fact.accepted_checkpoint_digest;
    session_fact.execution_profile_digest =
        readiness_fact.execution_profile_digest;
    session_fact.locked_execution_profile_digest =
        readiness_fact.locked_execution_profile_digest;
    session_fact.paper_online_profile_digest =
        readiness_fact.paper_online_profile_digest;
    session_fact.direct_edge_universe_digest =
        readiness_fact.direct_edge_universe_digest;
    session_fact.accounting_numeraire_node_id =
        readiness_fact.accounting_numeraire_node_id;
    session_fact.target_node_ids = join_strings(request.target_node_ids, ",");
    session_fact.reward_contract_id = readiness_fact.reward_contract_id;
    session_fact.session_manifest_digest =
        line_count_digest(kenv::kPaperOnlineSessionManifestSchemaV1,
                          request.session_root / "session.manifest");
    session_fact.session_state_digest =
        line_count_digest(kenv::kPaperOnlineSessionStateSchemaV1,
                          request.session_root / "session.state");
    session_fact.session_events_digest =
        line_count_digest(kenv::kPaperOnlineSessionEventLogSchemaV1,
                          request.session_root / "session.events.lls");
    session_fact.market_events_digest =
        line_count_digest(kenv::kPaperOnlineMarketEventLogSchemaV1,
                          request.session_root / "market_events.lls");
    session_fact.action_intents_digest =
        line_count_digest(kenv::kPaperOnlineActionIntentLogSchemaV1,
                          request.session_root / "action_intents.lls");
    session_fact.execution_intents_digest =
        line_count_digest(kenv::kPaperOnlineExecutionIntentLogSchemaV1,
                          request.session_root / "execution_intents.lls");
    session_fact.paper_ledger_digest =
        line_count_digest(kenv::kPaperOnlinePaperLedgerSchemaV1,
                          request.session_root / "paper_ledger.lls");
    session_fact.reward_reports_digest =
        line_count_digest(kenv::kPaperOnlineRewardReportLogSchemaV1,
                          request.session_root / "reward_reports.lls");
    session_fact.requested_step_count = request.max_steps;
    session_fact.completed_step_count = packet.completed_step_count;
    session_fact.market_event_count = packet.market_event_count;
    session_fact.action_intent_count = packet.action_intent_count;
    session_fact.execution_intent_count = packet.execution_intent_count;
    session_fact.paper_fill_count = packet.paper_fill_count;
    session_fact.reward_report_count = packet.reward_report_count;
    session_fact.started_at_ms = request.requested_at_ms;
    session_fact.stopped_at_ms = packet.stopped_at_ms;
    session_fact.max_market_data_staleness_ms =
        readiness_fact.max_market_data_staleness_ms;
    session_fact.max_observed_market_data_staleness_ms =
        packet.max_observed_staleness_ms;
    session_fact.terminal_state = packet.terminal_state;
    session_fact.ledger_recovered = true;
    session_fact.duplicate_action_protection_enforced = true;
    session_fact.duplicate_execution_intent_protection_enforced = true;
    session_fact.market_data_staleness_enforced = true;
    session_fact.operator_abort_supported = true;
    session_fact.kill_switch_supported = true;
    session_fact.operator_abort_triggered = packet.operator_abort_triggered;
    session_fact.kill_switch_triggered = packet.kill_switch_triggered;
    session_fact.terminal_state_recorded = true;
    session_fact.artifacts_written = true;
    session_fact.paper_execution_attempted = packet.completed_step_count > 0;
    session_fact.paper_execution_completed = packet.completed_step_count > 0;
    exposure::write_paper_online_session_fact_sidecar(session_fact_path,
                                                      session_fact);
    session_fact_digest =
        exposure::paper_online_session_fact_digest(session_fact);
    artifacts_written = true;
  }

  const bool include_machine_payload =
      md::optional_bool(args, "include_machine_payload", false);
  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.paper_online.session\""
       << ",\"schema_version\":\"kikijyeba.environment.paper_online_session_"
          "runner_packet.v1\""
       << ",\"requested_mode\":" << json_quote(requested_mode)
       << ",\"check_only\":" << bool_json(!should_run)
       << ",\"session_ready\":" << bool_json(session_ready)
       << ",\"session_artifacts_written\":" << bool_json(artifacts_written)
       << ",\"session_id\":" << json_quote(request.session_id)
       << ",\"session_root\":" << json_quote(request.session_root.string())
       << ",\"session_request_source\":" << json_quote(request.source)
       << ",\"session_fact_path\":" << json_quote(session_fact_path.string())
       << ",\"session_fact_digest\":" << json_quote(session_fact_digest)
       << ",\"session_fact_ref\":"
       << json_quote(display::digest_ref("pos", session_fact_digest))
       << ",\"admission_job_dir\":"
       << json_quote(request.admission_job_dir.string())
       << ",\"admission_fact_digest\":" << json_quote(admission_fact_digest)
       << ",\"readiness_fact_digest\":"
       << json_quote(
              exposure::paper_online_readiness_fact_digest(readiness_fact))
       << ",\"terminal_state\":" << json_quote(packet.terminal_state)
       << ",\"completed_step_count\":" << packet.completed_step_count
       << ",\"max_observed_market_data_staleness_ms\":"
       << packet.max_observed_staleness_ms
       << ",\"issues\":" << string_array_json(issues)
       << ",\"authority\":{\"environment_runs_bounded_paper_session\":"
       << bool_json(should_run) << ",\"environment_executes_cajtucu_paper\":"
       << bool_json(should_run && packet.completed_step_count > 0)
       << ",\"environment_executes_broker_orders\":false,"
          "\"environment_executes_live_capital\":false,"
          "\"environment_selects_policy\":false,"
          "\"environment_selects_checkpoint\":false,"
          "\"environment_claims_market_readiness\":false,"
          "\"environment_claims_deployment_readiness\":false,"
          "\"environment_proves_lattice_target\":false}";
  if (include_machine_payload) {
    json << ",\"session_request_target_node_ids\":"
         << string_array_json(request.target_node_ids);
  }
  json << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] std::string policy_schema_json() {
  std::ostringstream out;
  out << "{\"keys\":[";
  for (std::size_t i = 0; i < std::size(kPolicyDescriptors); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"key\":" << json_quote(kPolicyDescriptors[i].key)
        << ",\"type\":" << json_quote(kPolicyDescriptors[i].type)
        << ",\"default\":" << json_quote(kPolicyDescriptors[i].default_value)
        << ",\"summary\":" << json_quote(kPolicyDescriptors[i].summary) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string paper_online_session_schema_json() {
  const auto contract = kenv::default_paper_online_session_contract();
  const auto runner_contract =
      kenv::default_paper_online_session_runner_contract();
  const auto issues = kenv::paper_online_session_contract_issues(contract);
  const auto runner_issues =
      kenv::paper_online_session_runner_contract_issues(runner_contract);
  constexpr std::array<kenv::paper_online_session_state_t, 11> kStates{
      kenv::paper_online_session_state_t::initialized,
      kenv::paper_online_session_state_t::admitted,
      kenv::paper_online_session_state_t::ledger_recovered,
      kenv::paper_online_session_state_t::streaming,
      kenv::paper_online_session_state_t::action_recorded,
      kenv::paper_online_session_state_t::paper_executed,
      kenv::paper_online_session_state_t::reward_reported,
      kenv::paper_online_session_state_t::stopping,
      kenv::paper_online_session_state_t::stopped,
      kenv::paper_online_session_state_t::aborted,
      kenv::paper_online_session_state_t::kill_switch_triggered};

  std::ostringstream out;
  out << "{\"contract_id\":" << json_quote(contract.contract_id)
      << ",\"environment_contract_id\":"
      << json_quote(contract.environment_contract_id)
      << ",\"readiness_contract_id\":"
      << json_quote(kenv::kPaperOnlineReadinessContractV1)
      << ",\"readiness_target_id\":" << json_quote(contract.readiness_target_id)
      << ",\"readiness_fact_schema_id\":"
      << json_quote(kenv::kPaperOnlineReadinessFactSchemaV1)
      << ",\"session_state_schema_id\":"
      << json_quote(contract.session_state_schema_id)
      << ",\"action_schema_id\":" << json_quote(contract.action_schema_id)
      << ",\"reward_contract_id\":" << json_quote(contract.reward_contract_id)
      << ",\"durable_artifacts\":[";
  for (std::size_t i = 0; i < contract.durable_artifacts.size(); ++i) {
    const auto &slot = contract.durable_artifacts[i];
    if (i != 0) {
      out << ",";
    }
    out << "{\"artifact_id\":" << json_quote(slot.artifact_id)
        << ",\"relative_path\":" << json_quote(slot.relative_path)
        << ",\"schema_id\":" << json_quote(slot.schema_id)
        << ",\"required\":" << bool_json(slot.required) << "}";
  }
  out << "],\"states\":[";
  for (std::size_t i = 0; i < kStates.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(kenv::paper_online_session_state_name(kStates[i]));
  }
  out << "],\"authority\":{\"session_runner_implemented\":"
      << bool_json(contract.session_runner_implemented)
      << ",\"policy_selection_authority\":"
      << bool_json(contract.policy_selection_authority)
      << ",\"checkpoint_selection_authority\":"
      << bool_json(contract.checkpoint_selection_authority)
      << ",\"broker_execution_allowed\":"
      << bool_json(contract.broker_execution_allowed)
      << ",\"live_execution_allowed\":"
      << bool_json(contract.live_execution_allowed)
      << ",\"direct_policy_to_broker_allowed\":"
      << bool_json(contract.direct_policy_to_broker_allowed)
      << ",\"direct_policy_to_cajtucu_allowed\":"
      << bool_json(contract.direct_policy_to_cajtucu_allowed)
      << "},\"validator_issues\":" << string_array_json(issues)
      << ",\"session_runner_contract\":{\"runner_contract_id\":"
      << json_quote(runner_contract.runner_contract_id)
      << ",\"session_fact_schema_id\":"
      << json_quote(runner_contract.session_fact_schema_id)
      << ",\"execution_backend_id\":"
      << json_quote(runner_contract.execution_backend_id)
      << ",\"session_runner_implemented\":"
      << bool_json(runner_contract.session_runner_implemented)
      << ",\"paper_execution_allowed\":"
      << bool_json(runner_contract.paper_execution_allowed)
      << ",\"broker_execution_allowed\":"
      << bool_json(runner_contract.broker_execution_allowed)
      << ",\"live_execution_allowed\":"
      << bool_json(runner_contract.live_execution_allowed)
      << ",\"validator_issues\":" << string_array_json(runner_issues) << "}}";
  return out.str();
}

[[nodiscard]] std::string path_text_preview(const fs::path &path,
                                            bool include_text,
                                            std::size_t max_bytes) {
  const bool exists = fs::exists(path);
  std::string digest;
  std::string text;
  if (exists) {
    digest =
        file_digest_or_empty(path, "kikijyeba.environment.inspect.file.v1");
    if (include_text) {
      std::string error;
      (void)read_text_file(path, &text, &error);
      if (text.size() > max_bytes) {
        text.resize(max_bytes);
      }
    }
  }
  std::ostringstream out;
  out << "{\"path\":" << json_quote(path.string())
      << ",\"exists\":" << bool_json(exists)
      << ",\"digest\":" << json_quote(digest);
  if (include_text) {
    out << ",\"text\":" << json_quote(text);
  }
  out << "}";
  return out.str();
}

using handler_fn = bool (*)(const std::map<std::string, std::string> &,
                            environment_context_t *, std::string *,
                            std::string *);

[[nodiscard]] bool handle_status(const std::map<std::string, std::string> &args,
                                 environment_context_t *ctx, std::string *out,
                                 std::string *err) {
  (void)err;
  md::validate_fields(args, {}, {}, "hero.environment.status");
  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.status\""
       << ",\"schema\":\"kikijyeba.environment.status.v1\""
       << ",\"policy_path\":" << json_quote(ctx->policy.policy_path.string())
       << ",\"global_config_path\":"
       << json_quote(ctx->policy.global_config_path.string())
       << ",\"profile_id\":" << json_quote(ctx->policy.profile_id)
       << ",\"runtime_root\":" << json_quote(runtime_root(ctx->policy).string())
       << ",\"allowed_job_roots\":"
       << json_quote(resolved_path_list_text(ctx->policy, "allowed_job_roots"))
       << ",\"allow_certify_issue\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_certify_issue", false))
       << ",\"allow_rollout_replay\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_rollout_replay", false))
       << ",\"allow_paper_online_session_run\":"
       << bool_json(policy_bool_or(ctx->policy,
                                   "allow_paper_online_session_run", false))
       << ",\"paper_online_session_max_steps\":"
       << policy_i64_or(ctx->policy, "paper_online_session_max_steps", 64)
       << ",\"authority\":{\"environment_admission_authority\":true,"
          "\"environment_rollout_surface\":true,"
          "\"runtime_low_level_executor\":false,"
          "\"lattice_proof_authority\":false,"
          "\"marshal_orchestrator\":false,"
          "\"policy_selection_authority\":false,"
          "\"market_readiness_authority\":false,"
          "\"deployment_authority\":false,"
          "\"live_capital_authority\":false}}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool
handle_inspect_schema(const std::map<std::string, std::string> &args,
                      environment_context_t *ctx, std::string *out,
                      std::string *err) {
  (void)ctx;
  (void)err;
  md::validate_fields(args, {}, {}, "hero.environment.inspect.schema");
  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.inspect.schema\""
       << ",\"schema\":\"kikijyeba.environment.schema_inspection.v1\""
       << ",\"policy_schema\":" << policy_schema_json()
       << ",\"paper_online_session_contract\":"
       << paper_online_session_schema_json() << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool
handle_inspect_job(const std::map<std::string, std::string> &args,
                   environment_context_t *ctx, std::string *out,
                   std::string *err) {
  md::validate_fields(args, {"job_dir", "include_text", "max_bytes"},
                      {"job_dir"}, "hero.environment.inspect.job");
  std::string job_dir_raw;
  if (!required_string(args, "job_dir", &job_dir_raw, err)) {
    return false;
  }
  if (trim_ascii(job_dir_raw).empty()) {
    *err = "hero.environment.inspect.job requires job_dir";
    return false;
  }
  const fs::path job_dir = normalize_path(fs::path(job_dir_raw));
  if (!job_path_allowed(ctx->policy, job_dir)) {
    *err = "E_ENVIRONMENT_INSPECT_JOB_DIR_DENIED: job_dir is outside "
           "Environment Hero roots: " +
           job_dir.string();
    return false;
  }
  const bool include_text = md::optional_bool(args, "include_text", false);
  const std::size_t max_bytes = static_cast<std::size_t>(std::max<std::int64_t>(
      0, md::optional_i64(
             args, "max_bytes",
             policy_int_or(ctx->policy, "max_capture_bytes", 65536))));
  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.inspect.job\""
       << ",\"schema\":\"kikijyeba.environment.job_inspection.v1\""
       << ",\"job_dir\":" << json_quote(job_dir.string())
       << ",\"sidecars\":{\"policy_acceptance\":"
       << path_text_preview(job_dir / "lattice.policy_acceptance.fact",
                            include_text, max_bytes)
       << ",\"paper_online_readiness\":"
       << path_text_preview(job_dir / "lattice.paper_online_readiness.fact",
                            include_text, max_bytes)
       << ",\"tsodao_settings_protection\":"
       << path_text_preview(job_dir / "lattice.tsodao_settings_protection.fact",
                            include_text, max_bytes)
       << ",\"allocation_engine\":"
       << path_text_preview(job_dir / "lattice.allocation_engine.fact",
                            include_text, max_bytes)
       << "},\"replay_artifacts\":{\"runtime_replay_batches\":"
       << path_text_preview(job_dir / "artifacts" /
                                "kikijyeba.environment.replay.v1" /
                                "runtime_replay_batches.index",
                            include_text, max_bytes)
       << ",\"runtime_replay_experiments\":"
       << path_text_preview(job_dir / "artifacts" /
                                "kikijyeba.environment.replay.v1" /
                                "runtime_replay_experiments.index",
                            include_text, max_bytes)
       << "}}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool
handle_certify_subject(const std::map<std::string, std::string> &args,
                       std::string_view subject, std::string_view tool_name,
                       environment_context_t *ctx, std::string *out,
                       std::string *err) {
  std::set<std::string> allowed_fields{"mode", "certification_evidence",
                                       "expected_preview_digest"};
  std::set<std::string> required_fields{"mode", "certification_evidence"};
  if (subject == "policy_acceptance") {
    allowed_fields.insert("policy_training_job_dir");
    allowed_fields.insert("tsodao_settings_protection_job_dir");
    allowed_fields.insert("acceptance_id");
    required_fields.insert("policy_training_job_dir");
    required_fields.insert("acceptance_id");
  } else if (subject == "paper_online_readiness") {
    allowed_fields.insert("policy_acceptance_job_dir");
    allowed_fields.insert("readiness_id");
    required_fields.insert("policy_acceptance_job_dir");
    required_fields.insert("readiness_id");
  } else {
    *err = "unknown fixed Environment certify subject";
    return false;
  }
  md::validate_fields(args, allowed_fields, required_fields,
                      std::string(tool_name));
  std::string requested_mode =
      lowercase_ascii(md::optional_string(args, "mode"));
  if (requested_mode != "check" && requested_mode != "issue") {
    *err = "mode must be check or issue";
    return false;
  }
  std::map<std::string, std::string> fields;
  if (!certify_fields_from_direct_args(args, subject, &fields, err)) {
    return false;
  }
  const bool expected_preview_digest_supplied =
      args.find("expected_preview_digest") != args.end();
  std::string expected_preview_digest;
  if (expected_preview_digest_supplied) {
    try {
      expected_preview_digest =
          md::optional_string(args, "expected_preview_digest");
    } catch (const std::exception &ex) {
      *err = ex.what();
      return false;
    }
  }
  if (subject == "policy_acceptance") {
    return handle_policy_acceptance_certification(
        fields, requested_mode, ctx, out, err, expected_preview_digest_supplied,
        expected_preview_digest, tool_name);
  }
  if (subject == "paper_online_readiness") {
    return handle_paper_online_readiness_certification(
        fields, requested_mode, ctx, out, err, expected_preview_digest_supplied,
        expected_preview_digest, tool_name);
  }
  *err = "unknown fixed Environment certify subject";
  return false;
}

[[nodiscard]] bool
handle_certify_policy_acceptance(const std::map<std::string, std::string> &args,
                                 environment_context_t *ctx, std::string *out,
                                 std::string *err) {
  return handle_certify_subject(args, "policy_acceptance",
                                "hero.environment.certify.policy_acceptance",
                                ctx, out, err);
}

[[nodiscard]] bool handle_certify_paper_online_readiness(
    const std::map<std::string, std::string> &args, environment_context_t *ctx,
    std::string *out, std::string *err) {
  return handle_certify_subject(
      args, "paper_online_readiness",
      "hero.environment.certify.paper_online_readiness", ctx, out, err);
}

[[nodiscard]] bool
handle_rollout(const std::map<std::string, std::string> &args,
               environment_context_t *ctx, std::string *out, std::string *err) {
  const std::string environment_requested_mode =
      lowercase_ascii(md::optional_string(args, "mode"));
  if (environment_requested_mode != "validate" &&
      environment_requested_mode != "replay") {
    *err = "mode must be validate or replay";
    return false;
  }
  md::validate_fields(args,
                      {"mode", "runtime_job_dir", "rollout_id",
                       "rollout_attempt_id", "target_node_ids",
                       "include_machine_payload"},
                      {"mode", "runtime_job_dir", "rollout_id",
                       "rollout_attempt_id", "target_node_ids"},
                      "hero.environment.rollout");
  marshal::marshal_rollout_request_t request{};
  if (!rollout_request_from_direct_args(args, environment_requested_mode, ctx,
                                        &request, err)) {
    return false;
  }
  const std::string rollout_request_text =
      md::rollout_request_packet_text(request);
  const std::string rollout_request_digest =
      md::rollout_request_file_digest(rollout_request_text);
  const auto plan = marshal::prepare_rollout_plan(request);
  const bool include_machine_payload =
      md::optional_bool(args, "include_machine_payload", false);
  const bool replay_requested =
      plan.accepted && environment_requested_mode == "replay";
  if (replay_requested &&
      !policy_bool_or(ctx->policy, "allow_rollout_replay", false)) {
    *err = "E_ENVIRONMENT_ROLLOUT_REPLAY_DENIED: allow_rollout_replay is false";
    return false;
  }

  bool runtime_replay_attempted = false;
  bool runtime_replay_ok = false;
  bool runtime_executor_call_ok = false;
  bool runtime_executor_result_error = false;
  const std::string runtime_executor_id = "hero.runtime.replay_executor.v1";
  std::string runtime_args_json;
  std::string runtime_args_digest;
  std::string runtime_result_json;
  std::string runtime_result_digest;
  std::string runtime_error_message;
  if (replay_requested) {
    runtime_replay_attempted = true;
    runtime_args_json =
        marshal::rollout_runtime_replay_args_json(request, plan, false);
    runtime_args_digest = marshal::marshal_digest_for_text(
        "kikijyeba.environment.rollout.runtime_replay_args.v1",
        runtime_args_json);
    const auto runtime_call =
        cuwacunu::hero::mcp_cli::call_runtime_replay_delegate(
            request.config_path, {}, runtime_args_json, request.timeout_seconds,
            static_cast<std::size_t>(std::max(
                1024, policy_int_or(ctx->policy, "max_capture_bytes", 65536))));
    runtime_executor_call_ok = runtime_call.process_ok;
    runtime_result_json = runtime_call.result_json;
    runtime_error_message = runtime_call.error_message;
    runtime_executor_result_error =
        cuwacunu::hero::mcp_cli::tool_result_is_error(runtime_result_json);
    runtime_result_digest = marshal::marshal_digest_for_text(
        "kikijyeba.environment.rollout.runtime_replay_result.v1",
        runtime_result_json);
    runtime_replay_ok =
        runtime_executor_call_ok && !runtime_executor_result_error &&
        runtime_result_json.find("\"ok\":true") != std::string::npos;
  }
  rollout_allocation_sidecar_result_t allocation_sidecar{};
  if (runtime_replay_ok || fs::exists(plan.expected_report_path)) {
    allocation_sidecar = materialize_rollout_allocation_engine_sidecar(plan);
  }

  const std::string dispatch_state =
      !plan.accepted
          ? "blocked"
          : (replay_requested ? (runtime_replay_ok ? "replayed" : "blocked")
                              : "validated");
  const std::vector<std::string> next_actions =
      !plan.accepted
          ? std::vector<std::string>{"fix_rollout_request"}
          : (replay_requested
                 ? (runtime_replay_ok
                        ? std::vector<std::string>{"inspect_rollout_report"}
                        : std::vector<std::string>{"inspect_environment_"
                                                   "rollout_failure"})
                 : std::vector<std::string>{"replay_environment_rollout",
                                            "inspect_rollout_plan"});
  auto display_plan = plan;
  display_plan.requested_mode = environment_requested_mode;
  display_plan.plan_digest = marshal::rollout_plan_digest(display_plan);
  std::ostringstream json;
  json << "{\"ok\":true,\"tool\":\"hero.environment.rollout\""
       << ",\"schema_version\":\"kikijyeba.environment.rollout.v1\""
       << ",\"requested_mode\":" << json_quote(environment_requested_mode)
       << ",\"environment_profile\":" << json_quote(ctx->policy.profile_id)
       << ",\"request_digest\":" << json_quote(plan.request_digest)
       << ",\"rollout_request_digest_domain\":"
       << json_quote(md::k_rollout_request_file_digest_domain_v1)
       << ",\"rollout_request_digest\":" << json_quote(rollout_request_digest)
       << ",\"dispatch_state\":" << json_quote(dispatch_state)
       << ",\"next_safe_actions\":" << string_array_json(next_actions)
       << ",\"idempotency\":{\"scope\":\"request_digest_binding\","
          "\"durable_duplicate_handoff_ledger\":false,"
          "\"duplicate_execution_prevented_by_environment\":false}"
       << ",\"receipt_produced\":false"
       << ",\"rollout_plan\":" << marshal::rollout_plan_json(display_plan)
       << ",\"runtime_replay\":{\"attempted\":"
       << bool_json(runtime_replay_attempted)
       << ",\"runtime_executor_id\":" << json_quote(runtime_executor_id)
       << ",\"ok\":" << bool_json(runtime_replay_ok)
       << ",\"executor_call_ok\":" << bool_json(runtime_executor_call_ok)
       << ",\"executor_result_error\":"
       << bool_json(runtime_executor_result_error)
       << ",\"executor_arguments_digest\":" << json_quote(runtime_args_digest)
       << ",\"executor_result_digest\":" << json_quote(runtime_result_digest)
       << ",\"error_message\":" << json_quote(runtime_error_message);
  if (include_machine_payload) {
    json << ",\"executor_arguments_json\":" << json_quote(runtime_args_json)
         << ",\"executor_result_json\":" << json_quote(runtime_result_json);
  }
  json << "},\"allocation_engine_sidecar\":{\"attempted\":"
       << bool_json(allocation_sidecar.attempted)
       << ",\"ok\":" << bool_json(allocation_sidecar.ok)
       << ",\"written\":" << bool_json(allocation_sidecar.written)
       << ",\"path\":" << json_quote(allocation_sidecar.path.string())
       << ",\"digest\":" << json_quote(allocation_sidecar.digest)
       << ",\"policy_id\":" << json_quote(allocation_sidecar.policy_id)
       << ",\"observer_belief_fact_digest\":"
       << json_quote(allocation_sidecar.observer_belief_fact_digest)
       << ",\"forecast_artifact_digest\":"
       << json_quote(allocation_sidecar.forecast_artifact_digest)
       << ",\"issues\":" << string_array_json(allocation_sidecar.issues)
       << ",\"error\":" << json_quote(allocation_sidecar.error)
       << "},\"authority\":{\"environment_rollout_surface\":true,"
          "\"runtime_low_level_replay_executor\":true,"
          "\"lattice_proof_authority\":false,"
          "\"policy_selection_authority\":false,"
          "\"live_capital_authority\":false}}";
  *out = json.str();
  return true;
}

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.environment.status") {
    return handle_status;
  }
  if (name == "hero.environment.inspect.schema") {
    return handle_inspect_schema;
  }
  if (name == "hero.environment.inspect.job") {
    return handle_inspect_job;
  }
  if (name == "hero.environment.certify.policy_acceptance") {
    return handle_certify_policy_acceptance;
  }
  if (name == "hero.environment.certify.paper_online_readiness") {
    return handle_certify_paper_online_readiness;
  }
  if (name == "hero.environment.certify.paper_online_session_admission") {
    return handle_certify_paper_online_session_admission;
  }
  if (name == "hero.environment.paper_online.session") {
    return handle_paper_online_session;
  }
  if (name == "hero.environment.rollout") {
    return handle_rollout;
  }
  return std::nullopt;
}

[[nodiscard]] std::string make_tool_result(std::string_view tool_name,
                                           const std::string &structured) {
  return "{\"content\":[{\"type\":\"text\",\"text\":" +
         json_quote(std::string(tool_name) + " executed") +
         "}],\"structuredContent\":" + structured + "}";
}

[[nodiscard]] std::string make_error_result(std::string_view message,
                                            int code = -32602) {
  return "{\"content\":[{\"type\":\"text\",\"text\":" + json_quote(message) +
         "}],\"structuredContent\":{\"ok\":false,\"error_code\":" +
         std::to_string(code) + ",\"error\":" + json_quote(message) +
         "},\"isError\":true}";
}

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.environment.status" ||
         name == "hero.environment.inspect.schema" ||
         name == "hero.environment.inspect.job";
}

[[nodiscard]] bool tool_is_destructive(std::string_view name) {
  return name == "hero.environment.certify.policy_acceptance" ||
         name == "hero.environment.certify.paper_online_readiness" ||
         name == "hero.environment.certify.paper_online_session_admission" ||
         name == "hero.environment.paper_online.session" ||
         name == "hero.environment.rollout";
}

} // namespace

std::filesystem::path resolve_environment_hero_dsl_path(
    const std::filesystem::path &global_config_path) {
  if (const auto configured = read_ini_value(global_config_path, "HERO",
                                             "environment_hero_dsl_path")) {
    return resolve_against(global_config_path, *configured);
  }
  return config_paths::default_config_sibling_path(global_config_path,
                                                   "hero.environment.dsl");
}

bool load_environment_policy(const std::filesystem::path &policy_path,
                             const std::filesystem::path &global_config,
                             environment_policy_t *out, std::string *error,
                             std::string_view profile_override) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = "environment policy output pointer is null";
    }
    return false;
  }
  environment_policy_t policy{};
  policy.policy_path = policy_path;
  policy.global_config_path = global_config;
  for (const auto &descriptor : kPolicyDescriptors) {
    policy.values[descriptor.key] = descriptor.default_value;
  }
  std::string text;
  if (!read_text_file(policy_path, &text, error)) {
    return false;
  }
  parsed_policy_text_t parsed{};
  if (!parse_policy_text(text, &parsed, error)) {
    return false;
  }
  for (auto &[key, value] : parsed.base) {
    policy.values[std::move(key)] = std::move(value);
  }
  std::string selected_profile = trim_ascii(profile_override);
  std::string selected_profile_source = "cli";
  if (selected_profile.empty()) {
    selected_profile_source = "global_config";
    if (const auto configured =
            read_ini_value(global_config, "HERO", "environment_hero_profile")) {
      selected_profile = trim_ascii(*configured);
    }
  }
  if (selected_profile.empty()) {
    selected_profile_source = "policy_file";
    selected_profile = trim_ascii(policy_get(policy, "environment_profile"));
  }
  if (selected_profile.empty()) {
    selected_profile = "operator_default";
    selected_profile_source = "default";
  }
  const auto profile_it = parsed.profiles.find(selected_profile);
  if (profile_it == parsed.profiles.end() &&
      !(parsed.profiles.empty() && selected_profile == "operator_default")) {
    if (error != nullptr) {
      *error = "unknown environment profile `" + selected_profile +
               "` in policy file: " + policy_path.string();
    }
    return false;
  }
  if (profile_it != parsed.profiles.end()) {
    for (auto &[key, value] : profile_it->second) {
      policy.values[std::move(key)] = std::move(value);
    }
  }
  policy.profile_id = selected_profile;
  policy.values["environment_profile"] = selected_profile;
  policy.values["environment_profile_source"] = selected_profile_source;

  if (policy_get(policy, "protocol_layer") != kProtocolLayerStdio) {
    if (error != nullptr) {
      *error = kProtocolLayerHttpsSseFailFastMessage;
    }
    return false;
  }
  *out = std::move(policy);
  return true;
}

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       environment_context_t *ctx,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json != nullptr) {
    out_tool_result_json->clear();
  }
  if (out_error_message != nullptr) {
    out_error_message->clear();
  }
  if (ctx == nullptr) {
    const std::string err = "Environment Hero context pointer is null";
    if (out_error_message != nullptr) {
      *out_error_message = err;
    }
    if (out_tool_result_json != nullptr) {
      *out_tool_result_json = make_error_result(err, -32603);
    }
    return false;
  }
  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) {
    arguments_json = "{}";
  }
  if (arguments_json.front() != '{') {
    const std::string err = "--args-json must be a JSON object";
    if (out_error_message != nullptr) {
      *out_error_message = err;
    }
    if (out_tool_result_json != nullptr) {
      *out_tool_result_json = make_error_result(err, -32602);
    }
    return false;
  }

  const auto handler = find_handler(tool_name);
  if (!handler.has_value()) {
    const std::string err = "unknown tool: " + tool_name;
    if (out_error_message != nullptr) {
      *out_error_message = err;
    }
    if (out_tool_result_json != nullptr) {
      *out_tool_result_json = make_error_result(err, -32601);
    }
    return false;
  }

  try {
    const auto args = md::object_fields(arguments_json);
    std::string structured;
    std::string err;
    const bool ok = (*handler)(args, ctx, &structured, &err);
    if (!ok) {
      if (err.empty()) {
        err = "E_ENVIRONMENT_HANDLER_FAILED_WITHOUT_DIAGNOSTIC";
      }
      if (out_error_message != nullptr) {
        *out_error_message = err;
      }
      if (out_tool_result_json != nullptr) {
        const int code =
            err.find("E_ENVIRONMENT_") == 0 ? kPolicyErrorCode : -32602;
        *out_tool_result_json = make_error_result(err, code);
      }
      return false;
    }
    if (out_tool_result_json != nullptr) {
      *out_tool_result_json = make_tool_result(tool_name, structured);
    }
    return true;
  } catch (const std::exception &ex) {
    if (out_error_message != nullptr) {
      *out_error_message = ex.what();
    }
    if (out_tool_result_json != nullptr) {
      *out_tool_result_json = make_error_result(ex.what(), -32602);
    }
    return false;
  }
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kTools); ++i) {
    mcp_schema_compat::assert_tool_input_schema_compatible(
        kTools[i].name, kTools[i].input_schema_json);
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << json_quote(kTools[i].name)
        << ",\"description\":" << json_quote(kTools[i].description)
        << ",\"inputSchema\":" << kTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << bool_json(tool_is_read_only(kTools[i].name))
        << ",\"destructiveHint\":"
        << bool_json(tool_is_destructive(kTools[i].name))
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

void run_jsonrpc_stdio_loop(environment_context_t *ctx) {
  mcp_stdio::message_t message;
  bool shutdown_seen = false;
  while (mcp_stdio::read_message(std::cin, &message)) {
    std::string line = trim_ascii(message.json);
    if (line.empty()) {
      continue;
    }
    const auto send = [&](std::string response) {
      mcp_stdio::write_response(std::cout, response,
                                message.content_length_framed);
    };
    try {
      const auto request = md::object_fields(line);
      std::string id_raw = "null";
      if (const auto raw = md::optional_raw(request, "id")) {
        id_raw = *raw;
      }
      std::string method;
      if (const auto raw = md::optional_raw(request, "method")) {
        method = md::parse_string_raw(*raw, "method");
      } else {
        continue;
      }
      if (method == "notifications/initialized") {
        continue;
      }
      if (method == "exit") {
        break;
      }
      if (method == "initialize") {
        std::string protocol = "2024-11-05";
        if (const auto params_raw = md::optional_raw(request, "params")) {
          const auto params = md::object_fields(*params_raw);
          if (const auto raw = md::optional_raw(params, "protocolVersion")) {
            protocol = md::parse_string_raw(*raw, "protocolVersion");
          }
        } else if (const auto raw =
                       md::optional_raw(request, "protocolVersion")) {
          protocol = md::parse_string_raw(*raw, "protocolVersion");
        }
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":{\"protocolVersion\":" + json_quote(protocol) +
             ",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":"
             "\"hero-environment\",\"version\":\"1.0.0\"},\"instructions\":"
             "\"\"}}");
        continue;
      }
      if (method == "ping") {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":{}}");
        continue;
      }
      if (method == "tools/list") {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":" + build_tools_list_result_json() + "}");
        continue;
      }
      if (method == "resources/list") {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":{\"resources\":[]}}");
        continue;
      }
      if (method == "resources/templates/list") {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":{\"resourceTemplates\":[]}}");
        continue;
      }
      if (method == "tools/call") {
        const auto params_raw = md::optional_raw(request, "params");
        if (!params_raw.has_value()) {
          send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
               ",\"error\":{\"code\":-32602,\"message\":" +
               json_quote("tools/call requires params") + "}}");
          continue;
        }
        const auto params = md::object_fields(*params_raw);
        const auto name_raw = md::optional_raw(params, "name");
        if (!name_raw.has_value()) {
          send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
               ",\"error\":{\"code\":-32602,\"message\":" +
               json_quote("tools/call requires name") + "}}");
          continue;
        }
        const std::string name = md::parse_string_raw(*name_raw, "name");
        std::string args_json = "{}";
        if (const auto raw = md::optional_raw(params, "arguments")) {
          args_json = *raw;
        }
        std::string result;
        std::string error;
        (void)execute_tool_json(name, args_json, ctx, &result, &error);
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":" + result + "}");
        continue;
      }
      if (method == "shutdown") {
        shutdown_seen = true;
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"result\":null}");
        continue;
      }
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"error\":{\"code\":-32601,\"message\":" +
           json_quote("unknown method: " + method) + "}}");
    } catch (const std::exception &ex) {
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":"
                       "-32603,\"message\":") +
           json_quote(ex.what()) + "}}");
    }
    if (shutdown_seen) {
      break;
    }
  }
}

} // namespace cuwacunu::hero::environment
