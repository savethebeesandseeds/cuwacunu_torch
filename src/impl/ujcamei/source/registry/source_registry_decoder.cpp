#include "ujcamei/source/registry/source_registry_decoder.h"

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ujcamei/source/contract/syntax/parse_utils.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace registry {
using namespace ::cuwacunu::ujcamei::source::contract::syntax;
using ::cuwacunu::ujcamei::source::contract::source_form_t;
using ::cuwacunu::ujcamei::source::contract::source_spec_t;

namespace {

struct source_registry_decode_context_t {
  source_spec_t spec{};
  std::string default_source_root{};
  std::vector<std::string> default_kline_intervals{};
};

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

[[nodiscard]] cuwacunu::ujcamei::source::registry::types::interval_type_e
parse_interval_type_strict(const std::string &text, const char *field_name) {
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(std::string("missing value for ") + field_name);
  }
  try {
    return cuwacunu::ujcamei::source::registry::types::string_to_enum<
        cuwacunu::ujcamei::source::registry::types::interval_type_e>(value);
  } catch (...) {
    throw std::runtime_error(std::string("invalid interval for ") + field_name +
                             ": " + value);
  }
}

void collect_node_texts_by_hash(const ASTNode *node, std::uint64_t wanted_hash,
                                std::vector<std::string> &out) {
  if (!node) {
    return;
  }
  if (node->hash == wanted_hash) {
    out.push_back(detail::trim_spaces_tabs(detail::flatten_node_text(node)));
    return;
  }
  if (const auto *root = dynamic_cast<const RootNode *>(node)) {
    for (const auto &child : root->children) {
      collect_node_texts_by_hash(child.get(), wanted_hash, out);
    }
    return;
  }
  if (const auto *mid = dynamic_cast<const IntermediaryNode *>(node)) {
    for (const auto &child : mid->children) {
      collect_node_texts_by_hash(child.get(), wanted_hash, out);
    }
  }
}

void collect_intermediary_nodes_by_hash(
    const ASTNode *node, std::uint64_t wanted_hash,
    std::vector<const IntermediaryNode *> &out) {
  if (!node) {
    return;
  }
  if (node->hash == wanted_hash) {
    if (const auto *matched = dynamic_cast<const IntermediaryNode *>(node)) {
      out.push_back(matched);
    }
    return;
  }
  if (const auto *root = dynamic_cast<const RootNode *>(node)) {
    for (const auto &child : root->children) {
      collect_intermediary_nodes_by_hash(child.get(), wanted_hash, out);
    }
    return;
  }
  if (const auto *mid = dynamic_cast<const IntermediaryNode *>(node)) {
    for (const auto &child : mid->children) {
      collect_intermediary_nodes_by_hash(child.get(), wanted_hash, out);
    }
  }
}

[[nodiscard]] std::string
required_assignment_value(const IntermediaryNode *block,
                          std::uint64_t assignment_hash,
                          std::uint64_t value_hash, const char *field_name) {
  const auto *assignment = dynamic_cast<const IntermediaryNode *>(
      detail::find_direct_child_by_hash(block, assignment_hash));
  const ASTNode *value_node =
      detail::find_direct_child_by_hash(assignment, value_hash);
  std::string value =
      detail::trim_spaces_tabs(detail::flatten_node_text(value_node));
  if (value.empty()) {
    throw std::runtime_error(std::string("missing value for ") + field_name);
  }
  return value;
}

[[nodiscard]] std::string
optional_assignment_value(const IntermediaryNode *block,
                          std::uint64_t assignment_hash,
                          std::uint64_t value_hash) {
  const auto *assignment = dynamic_cast<const IntermediaryNode *>(
      detail::find_direct_child_by_hash(block, assignment_hash));
  if (assignment == nullptr) {
    return {};
  }
  const ASTNode *value_node =
      detail::find_direct_child_by_hash(assignment, value_hash);
  return detail::trim_spaces_tabs(detail::flatten_node_text(value_node));
}

[[nodiscard]] std::string make_kline_source_path(std::string root,
                                                 const std::string &instrument,
                                                 const std::string &interval) {
  root = trim_ascii_ws_copy(std::move(root));
  while (root.size() > 1 && root.back() == '/') {
    root.pop_back();
  }
  return root + "/" + instrument + "/" + interval + "/" + instrument + "-" +
         interval + "-all-years.csv";
}

[[nodiscard]] const IntermediaryNode *
registry_source_defaults_block(const ASTNode *root_node) {
  std::vector<const IntermediaryNode *> blocks;
  collect_intermediary_nodes_by_hash(
      root_node, SOURCE_PIPELINE_HASH_source_defaults_block, blocks);
  if (blocks.empty()) {
    return nullptr;
  }
  if (blocks.size() != 1) {
    throw std::runtime_error("SOURCE_DEFAULTS must be declared at most once");
  }
  return blocks.front();
}

[[nodiscard]] std::string
registry_default_source_root(const IntermediaryNode *defaults_block) {
  if (defaults_block == nullptr) {
    return {};
  }
  return required_assignment_value(
      defaults_block, SOURCE_PIPELINE_HASH_default_source_root_assignment,
      SOURCE_PIPELINE_HASH_source_root_path, "SOURCE_DEFAULTS.SOURCE_ROOT");
}

[[nodiscard]] std::vector<std::string>
registry_default_kline_intervals(const IntermediaryNode *defaults_block) {
  if (defaults_block == nullptr) {
    return {};
  }
  const auto *intervals_assignment =
      dynamic_cast<const IntermediaryNode *>(detail::find_direct_child_by_hash(
          defaults_block,
          SOURCE_PIPELINE_HASH_default_kline_intervals_assignment));
  if (intervals_assignment == nullptr) {
    return {};
  }
  std::vector<std::string> intervals;
  collect_node_texts_by_hash(intervals_assignment,
                             SOURCE_PIPELINE_HASH_interval, intervals);
  if (intervals.empty()) {
    throw std::runtime_error("SOURCE_DEFAULTS.KLINE_INTERVALS must not be "
                             "empty when declared");
  }
  return intervals;
}

void append_kline_source_set(
    const IntermediaryNode *node, source_spec_t &out,
    const std::string &default_source_root,
    const std::vector<std::string> &default_kline_intervals) {
  source_form_t base{};
  base.instrument = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_instrument_assignment,
      SOURCE_PIPELINE_HASH_instrument, "KLINE_SOURCE_SET.INSTRUMENT");
  base.record_type = "kline";
  base.market_type = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_market_type_assignment,
      SOURCE_PIPELINE_HASH_market_type, "KLINE_SOURCE_SET.MARKET_TYPE");
  base.venue = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_venue_assignment,
      SOURCE_PIPELINE_HASH_venue, "KLINE_SOURCE_SET.VENUE");
  base.base_asset = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_base_asset_assignment,
      SOURCE_PIPELINE_HASH_base_asset, "KLINE_SOURCE_SET.BASE_ASSET");
  base.quote_asset = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_quote_asset_assignment,
      SOURCE_PIPELINE_HASH_quote_asset, "KLINE_SOURCE_SET.QUOTE_ASSET");
  base.source_kind = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_kind_assignment,
      SOURCE_PIPELINE_HASH_source_kind, "KLINE_SOURCE_SET.SOURCE_KIND");
  std::string source_root = optional_assignment_value(
      node, SOURCE_PIPELINE_HASH_kline_source_root_assignment,
      SOURCE_PIPELINE_HASH_source_root_path);
  if (source_root.empty()) {
    source_root = default_source_root;
  }
  if (source_root.empty()) {
    throw std::runtime_error("KLINE_SOURCE_SET.SOURCE_ROOT is required when "
                             "SOURCE_DEFAULTS.SOURCE_ROOT is absent");
  }

  std::string signature_error{};
  if (!instrument_signature_validate(
          base.instrument_signature(), /*allow_any=*/false,
          "KLINE_SOURCE_SET " + base.instrument, &signature_error)) {
    throw std::runtime_error(signature_error);
  }

  const auto *intervals_assignment =
      dynamic_cast<const IntermediaryNode *>(detail::find_direct_child_by_hash(
          node, SOURCE_PIPELINE_HASH_kline_intervals_assignment));
  std::vector<std::string> intervals;
  collect_node_texts_by_hash(intervals_assignment,
                             SOURCE_PIPELINE_HASH_interval, intervals);
  if (intervals.empty()) {
    intervals = default_kline_intervals;
  }
  if (intervals.empty()) {
    throw std::runtime_error("KLINE_SOURCE_SET.INTERVALS is required when "
                             "SOURCE_DEFAULTS.KLINE_INTERVALS is absent");
  }

  for (const auto &interval : intervals) {
    source_form_t row = base;
    row.interval =
        parse_interval_type_strict(interval, "KLINE_SOURCE_SET.INTERVALS");
    row.source = make_kline_source_path(source_root, row.instrument, interval);
    out.source_forms.push_back(std::move(row));
  }
}

} // namespace

source_registry_decoder_t::source_registry_decoder_t(std::string grammar_text)
    : SOURCE_FORMS_GRAMMAR_TEXT(std::move(grammar_text)),
      grammarLexer(SOURCE_FORMS_GRAMMAR_TEXT), grammarParser(grammarLexer),
      grammar(parseGrammarDefinition()), iParser(iLexer, grammar) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("%s\n", SOURCE_FORMS_GRAMMAR_TEXT.c_str());
#endif
}

source_spec_t source_registry_decoder_t::decode(std::string instruction) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("Request to decode source_registry_decoder_t\n");
#endif
  LOCK_GUARD(current_mutex);
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef SOURCE_PIPELINE_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_dbg("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  source_registry_decode_context_t current;
  const IntermediaryNode *defaults_block =
      registry_source_defaults_block(actualAST.get());
  current.default_source_root = registry_default_source_root(defaults_block);
  current.default_kline_intervals =
      registry_default_kline_intervals(defaults_block);
  VisitorContext context(static_cast<void *>(&current));
  actualAST.get()->accept(*this, context);

  return current.spec;
}

ProductionGrammar source_registry_decoder_t::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void source_registry_decoder_t::visit(const RootNode *node,
                                      VisitorContext &context) {
#ifdef SOURCE_PIPELINE_DEBUG
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

void source_registry_decoder_t::visit(const IntermediaryNode *node,
                                      VisitorContext &context) {
#ifdef SOURCE_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(),
          node->alt.str(true).c_str());
#endif

  auto *decode_context =
      static_cast<source_registry_decode_context_t *>(context.user_data);
  if (!decode_context)
    return;
  auto *out = &decode_context->spec;

  if (node->hash == SOURCE_PIPELINE_HASH_instrument_table) {
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_data_analytics_policy_block) {
    out->data_analytics_policy.declared = true;
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_kline_source_set_block) {
    append_kline_source_set(node, *out, decode_context->default_source_root,
                            decode_context->default_kline_intervals);
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_csv_bootstrap_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_unsigned_int);
    const std::uint64_t parsed = parse_u64_strict(
        detail::flatten_node_text(n_value), "CSV_BOOTSTRAP_DELTAS");
    if (parsed < 2) {
      throw std::runtime_error("CSV_BOOTSTRAP_DELTAS must be >= 2");
    }
    out->csv_bootstrap_deltas = static_cast<std::size_t>(parsed);
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_csv_step_abs_tol_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "CSV_STEP_ABS_TOL");
    if (!(parsed > 0.0L)) {
      throw std::runtime_error("CSV_STEP_ABS_TOL must be > 0");
    }
    out->csv_step_abs_tol = parsed;
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_csv_step_rel_tol_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "CSV_STEP_REL_TOL");
    if (parsed < 0.0L) {
      throw std::runtime_error("CSV_STEP_REL_TOL must be >= 0");
    }
    out->csv_step_rel_tol = parsed;
    return;
  }

  if (node->hash ==
      SOURCE_PIPELINE_HASH_data_analytics_max_samples_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_unsigned_int);
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
      SOURCE_PIPELINE_HASH_data_analytics_max_features_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_unsigned_int);
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
      SOURCE_PIPELINE_HASH_data_analytics_mask_epsilon_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "MASK_EPSILON");
    if (parsed < 0.0L) {
      throw std::runtime_error("MASK_EPSILON must be >= 0");
    }
    out->data_analytics_policy.mask_epsilon = parsed;
    return;
  }

  if (node->hash ==
      SOURCE_PIPELINE_HASH_data_analytics_standardize_epsilon_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_float);
    const long double parsed = parse_long_double_strict(
        detail::flatten_node_text(n_value), "STANDARDIZE_EPSILON");
    if (!(parsed > 0.0L)) {
      throw std::runtime_error("STANDARDIZE_EPSILON must be > 0");
    }
    out->data_analytics_policy.standardize_epsilon = parsed;
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_instrument_form) {
    source_form_t f{};

    const ASTNode *n_instrument = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_instrument);
    const ASTNode *n_interval =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_interval);
    const ASTNode *n_record_type = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_record_type);
    const ASTNode *n_market_type = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_market_type);
    const ASTNode *n_venue =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_venue);
    const ASTNode *n_base_asset = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_base_asset);
    const ASTNode *n_quote_asset = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_quote_asset);
    const ASTNode *n_source_kind = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_source_kind);
    const ASTNode *n_source =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_source);

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
    f.source_kind =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_source_kind));
    f.source = detail::trim_spaces_tabs(detail::flatten_node_text(n_source));

    std::string signature_error{};
    if (!instrument_signature_validate(
            f.instrument_signature(), /*allow_any=*/false,
            "source row " + f.instrument, &signature_error)) {
      throw std::runtime_error(signature_error);
    }

    f.interval = parse_interval_type_strict(interval_s, "source row interval");

    out->source_forms.push_back(std::move(f));
    return;
  }
}

void source_registry_decoder_t::visit(const TerminalNode *node,
                                      VisitorContext &context) {
#ifdef SOURCE_PIPELINE_DEBUG
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

} // namespace registry
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
