#include "camahjucunu/dsl/observation_pipeline/observation_sources_decoder.h"

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "camahjucunu/dsl/observation_pipeline/observation_parse_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

namespace {

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::uint64_t parse_u64_strict(const std::string &text,
                                             const char *field_name) {
  std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(std::string("missing value for ") + field_name);
  }
  std::uint64_t parsed = 0;
  const char *b = value.data();
  const char *e = value.data() + value.size();
  const auto r = std::from_chars(b, e, parsed);
  if (r.ec != std::errc{} || r.ptr != e) {
    throw std::runtime_error(std::string("invalid integer for ") + field_name +
                             ": " + value);
  }
  return parsed;
}

[[nodiscard]] long double parse_long_double_strict(const std::string &text,
                                                   const char *field_name) {
  std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(std::string("missing value for ") + field_name);
  }
  char *end = nullptr;
  errno = 0;
  const long double parsed = std::strtold(value.c_str(), &end);
  if (errno != 0 || end == nullptr || end != value.c_str() + value.size()) {
    throw std::runtime_error(std::string("invalid float for ") + field_name +
                             ": " + value);
  }
  return parsed;
}

} // namespace

observationSourcesDecoder::observationSourcesDecoder(std::string grammar_text)
    : OBSERVATION_SOURCES_GRAMMAR_TEXT(std::move(grammar_text)),
      grammarLexer(OBSERVATION_SOURCES_GRAMMAR_TEXT),
      grammarParser(grammarLexer), grammar(parseGrammarDefinition()),
      iParser(iLexer, grammar) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_dbg("%s\n", OBSERVATION_SOURCES_GRAMMAR_TEXT.c_str());
#endif
}

observation_spec_t observationSourcesDecoder::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_dbg("Request to decode observationSourcesDecoder\n");
#endif
  LOCK_GUARD(current_mutex);
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_dbg("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  observation_spec_t current;
  VisitorContext context(static_cast<void *>(&current));
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar observationSourcesDecoder::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void observationSourcesDecoder::visit(const RootNode *node,
                                      VisitorContext &context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(),
          node->lhs_instruction.c_str());
#endif

  (void)node;
  (void)context;
}

void observationSourcesDecoder::visit(const IntermediaryNode *node,
                                      VisitorContext &context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(),
          node->alt.str(true).c_str());
#endif

  auto *out = static_cast<observation_spec_t *>(context.user_data);
  if (!out)
    return;

  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_table) {
    out->source_forms.clear();
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_data_analytics_policy_block) {
    out->data_analytics_policy.declared = true;
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_csv_bootstrap_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_unsigned_int);
    const std::uint64_t parsed = parse_u64_strict(
        detail::flatten_node_text(n_value), "CSV_BOOTSTRAP_DELTAS");
    if (parsed < 2) {
      throw std::runtime_error("CSV_BOOTSTRAP_DELTAS must be >= 2");
    }
    out->csv_bootstrap_deltas = static_cast<std::size_t>(parsed);
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_csv_step_abs_tol_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "CSV_STEP_ABS_TOL");
    if (!(parsed > 0.0L)) {
      throw std::runtime_error("CSV_STEP_ABS_TOL must be > 0");
    }
    out->csv_step_abs_tol = parsed;
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_csv_step_rel_tol_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "CSV_STEP_REL_TOL");
    if (parsed < 0.0L) {
      throw std::runtime_error("CSV_STEP_REL_TOL must be >= 0");
    }
    out->csv_step_rel_tol = parsed;
    return;
  }

  if (node->hash ==
      OBSERVATION_PIPELINE_HASH_data_analytics_max_samples_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_unsigned_int);
    const std::uint64_t parsed =
        parse_u64_strict(detail::flatten_node_text(n_value), "MAX_SAMPLES");
    if (parsed < 1) {
      throw std::runtime_error("MAX_SAMPLES must be >= 1");
    }
    if (parsed >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      throw std::runtime_error("MAX_SAMPLES exceeds int64 range");
    }
    out->data_analytics_policy.max_samples = static_cast<std::int64_t>(parsed);
    return;
  }

  if (node->hash ==
      OBSERVATION_PIPELINE_HASH_data_analytics_max_features_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_unsigned_int);
    const std::uint64_t parsed =
        parse_u64_strict(detail::flatten_node_text(n_value), "MAX_FEATURES");
    if (parsed < 1) {
      throw std::runtime_error("MAX_FEATURES must be >= 1");
    }
    if (parsed >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      throw std::runtime_error("MAX_FEATURES exceeds int64 range");
    }
    out->data_analytics_policy.max_features = static_cast<std::int64_t>(parsed);
    return;
  }

  if (node->hash ==
      OBSERVATION_PIPELINE_HASH_data_analytics_mask_epsilon_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "MASK_EPSILON");
    if (parsed < 0.0L) {
      throw std::runtime_error("MASK_EPSILON must be >= 0");
    }
    out->data_analytics_policy.mask_epsilon = parsed;
    return;
  }

  if (node->hash ==
      OBSERVATION_PIPELINE_HASH_data_analytics_standardize_epsilon_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "STANDARDIZE_EPSILON");
    if (!(parsed > 0.0L)) {
      throw std::runtime_error("STANDARDIZE_EPSILON must be > 0");
    }
    out->data_analytics_policy.standardize_epsilon = parsed;
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    observation_source_t f{};

    const ASTNode *n_instrument = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_instrument);
    const ASTNode *n_interval = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_interval);
    const ASTNode *n_record_type = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_record_type);
    const ASTNode *n_market_type = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_market_type);
    const ASTNode *n_venue = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_venue);
    const ASTNode *n_base_asset = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_base_asset);
    const ASTNode *n_quote_asset = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_quote_asset);
    const ASTNode *n_source = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_source);

    f.instrument =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_instrument));
    std::string interval_s =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_interval));
    f.record_type =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_record_type));
    f.market_type =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_market_type));
    f.venue = detail::trim_spaces_tabs(detail::flatten_node_text(n_venue));
    f.base_asset =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_base_asset));
    f.quote_asset =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_quote_asset));
    f.source = detail::trim_spaces_tabs(detail::flatten_node_text(n_source));

    std::string signature_error{};
    if (!instrument_signature_validate(
            f.instrument_signature(), /*allow_any=*/false,
            "source row " + f.instrument, &signature_error)) {
      throw std::runtime_error(signature_error);
    }

    try {
      f.interval = cuwacunu::camahjucunu::exchange::string_to_enum<
          cuwacunu::camahjucunu::exchange::interval_type_e>(interval_s);
    } catch (...) {
    }

    out->source_forms.push_back(std::move(f));
    return;
  }
}

void observationSourcesDecoder::visit(const TerminalNode *node,
                                      VisitorContext &context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(),
          node->unit.str(true).c_str());
#endif

  (void)node;
  (void)context;
}

} // namespace dsl
} // namespace camahjucunu
} // namespace cuwacunu
