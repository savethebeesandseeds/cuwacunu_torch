#include "hero/environment_hero/hero_environment_tools.h"

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
constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultEnvironmentPolicyPath =
    "/cuwacunu/src/config/hero.environment.dsl";
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
    {"default_config_path", "path", kDefaultGlobalConfigPath,
     "Default global config path for delegated Runtime work."},
    {"runtime_root", "path", "/cuwacunu/.runtime/cuwacunu_exec",
     "Runtime evidence root inspected by Environment admission checks."},
    {"allowed_job_roots", "path_list", "/cuwacunu/.runtime/cuwacunu_exec,/tmp",
     "Roots under which Environment may read/write job-local evidence."},
    {"allow_certify_issue", "bool", "true",
     "Allow Environment certification to issue validated sidecars."},
    {"allow_rollout_replay", "bool", "true",
     "Allow Environment rollout to delegate low-level Runtime replay."},
    {"max_capture_bytes", "int", "65536",
     "Maximum delegated Runtime result payload retained inline."},
    {"max_runtime_seconds", "int", "600",
     "Default timeout for delegated Runtime replay."},
    {"rollout_policy_set", "string",
     "numeraire,current_weight,equal_weight,sdu",
     "Default Environment rollout policy set."},
    {"rollout_max_steps", "int", "250",
     "Default positive finite Environment rollout step cap."},
    {"rollout_max_parallel_jobs", "int", "4",
     "Default positive finite Environment rollout worker cap."},
    {"rollout_runtime_exec_path", "path", "/cuwacunu/.build/exec/cuwacunu_exec",
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

[[nodiscard]] fs::path runtime_root(const environment_policy_t &policy) {
  return normalize_path(fs::path(policy_get(policy, "runtime_root")));
}

[[nodiscard]] bool explicit_job_dir_allowed(const environment_policy_t &policy,
                                            const fs::path &path) {
  for (const auto &root :
       split_path_list(policy_get(policy, "allowed_job_roots"))) {
    if (path_within(root, path)) {
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

[[nodiscard]] fs::path policy_path_or(const environment_policy_t &policy,
                                      std::string_view key,
                                      const fs::path &fallback) {
  const std::string raw = trim_ascii(policy_get(policy, key));
  if (raw.empty()) {
    return fallback;
  }
  return resolve_against(policy.policy_path, raw);
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
    request.config_path = policy_path_or(
        ctx->policy, "default_config_path",
        ctx->global_config_path.empty() ? fs::path(kDefaultGlobalConfigPath)
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
                       fs::path("/cuwacunu/.build/exec/cuwacunu_exec"));
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
  const auto issues = kenv::paper_online_session_contract_issues(contract);
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
      << "},\"validator_issues\":" << string_array_json(issues) << "}";
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
       << ",\"runtime_root\":"
       << json_quote(policy_get(ctx->policy, "runtime_root"))
       << ",\"allowed_job_roots\":"
       << json_quote(policy_get(ctx->policy, "allowed_job_roots"))
       << ",\"allow_certify_issue\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_certify_issue", false))
       << ",\"allow_rollout_replay\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_rollout_replay", false))
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
  json << "},\"authority\":{\"environment_rollout_surface\":true,"
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
         name == "hero.environment.rollout";
}

} // namespace

std::filesystem::path resolve_environment_hero_dsl_path(
    const std::filesystem::path &global_config_path) {
  if (const auto configured = read_ini_value(global_config_path, "HERO",
                                             "environment_hero_dsl_path")) {
    return resolve_against(global_config_path, *configured);
  }
  return kDefaultEnvironmentPolicyPath;
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
