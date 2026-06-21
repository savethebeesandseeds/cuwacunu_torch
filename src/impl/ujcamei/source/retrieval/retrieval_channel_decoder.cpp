#include "ujcamei/source/retrieval/retrieval_channel_decoder.h"

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ujcamei/source/contract/syntax/parse_utils.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace retrieval {
using namespace ::cuwacunu::ujcamei::source::contract::syntax;
using ::cuwacunu::ujcamei::source::contract::channel_form_t;
using ::cuwacunu::ujcamei::source::contract::source_spec_t;

namespace {

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

[[nodiscard]] cuwacunu::ujcamei::source::registry::types::interval_type_e
parse_interval_type_strict(const std::string &text, const char *field_name) {
  const std::string value = detail::trim_spaces_tabs(text);
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

void append_channel_set(const IntermediaryNode *node, source_spec_t &out) {
  channel_form_t base{};
  base.active = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_active_assignment,
      SOURCE_PIPELINE_HASH_active, "CHANNEL_SET.ACTIVE");
  base.record_type = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_record_type_assignment,
      SOURCE_PIPELINE_HASH_record_type, "CHANNEL_SET.RECORD_TYPE");
  base.input_length = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_input_length_assignment,
      SOURCE_PIPELINE_HASH_input_length, "CHANNEL_SET.INPUT_LENGTH");
  base.future_length = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_future_length_assignment,
      SOURCE_PIPELINE_HASH_future_length, "CHANNEL_SET.FUTURE_LENGTH");
  base.channel_weight = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_weight_assignment,
      SOURCE_PIPELINE_HASH_channel_weight, "CHANNEL_SET.CHANNEL_WEIGHT");
  base.normalization_policy = required_assignment_value(
      node, SOURCE_PIPELINE_HASH_channel_set_normalization_assignment,
      SOURCE_PIPELINE_HASH_normalization_policy,
      "CHANNEL_SET.NORMALIZATION_POLICY");

  const auto *intervals_assignment =
      dynamic_cast<const IntermediaryNode *>(detail::find_direct_child_by_hash(
          node, SOURCE_PIPELINE_HASH_channel_set_intervals_assignment));
  std::vector<std::string> intervals;
  collect_node_texts_by_hash(intervals_assignment,
                             SOURCE_PIPELINE_HASH_interval, intervals);
  if (intervals.empty()) {
    throw std::runtime_error("CHANNEL_SET.INTERVALS must not be empty");
  }

  for (const auto &interval : intervals) {
    channel_form_t row = base;
    row.interval =
        parse_interval_type_strict(interval, "CHANNEL_SET.INTERVALS");
    out.channel_forms.push_back(std::move(row));
  }
}

} // namespace

retrieval_channel_decoder_t::retrieval_channel_decoder_t(
    std::string grammar_text)
    : SOURCE_CHANNELS_GRAMMAR_TEXT(std::move(grammar_text)),
      grammarLexer(SOURCE_CHANNELS_GRAMMAR_TEXT), grammarParser(grammarLexer),
      grammar(parseGrammarDefinition()), iParser(iLexer, grammar) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("%s\n", SOURCE_CHANNELS_GRAMMAR_TEXT.c_str());
#endif
}

source_spec_t retrieval_channel_decoder_t::decode(std::string instruction) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("Request to decode retrieval_channel_decoder_t\n");
#endif
  LOCK_GUARD(current_mutex);
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef SOURCE_PIPELINE_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_dbg("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  source_spec_t current;
  VisitorContext context(static_cast<void *>(&current));
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar retrieval_channel_decoder_t::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void retrieval_channel_decoder_t::visit(const RootNode *node,
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

void retrieval_channel_decoder_t::visit(const IntermediaryNode *node,
                                        VisitorContext &context) {
#ifdef SOURCE_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(),
          node->alt.str(true).c_str());
#endif

  auto *out = static_cast<source_spec_t *>(context.user_data);
  if (!out)
    return;

  if (node->hash == SOURCE_PIPELINE_HASH_channel_table) {
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_channel_set_block) {
    append_channel_set(node, *out);
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_channel_form) {
    channel_form_t f{};

    const ASTNode *n_interval =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_interval);
    const ASTNode *n_active =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_active);
    const ASTNode *n_record_type = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_record_type);
    const ASTNode *n_input_length = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_input_length);
    const ASTNode *n_future_length = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_future_length);
    const ASTNode *n_channel_weight = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_channel_weight);
    const ASTNode *n_normalization_policy = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_normalization_policy);

    std::string interval_s =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_interval));
    f.active = detail::trim_spaces_tabs(detail::flatten_node_text(n_active));
    f.record_type =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_record_type));
    f.input_length =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_input_length));
    f.future_length =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_future_length));
    f.channel_weight =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_channel_weight));
    f.normalization_policy = detail::trim_spaces_tabs(
        detail::flatten_node_text(n_normalization_policy));

    f.interval = parse_interval_type_strict(interval_s, "channel row interval");

    out->channel_forms.push_back(std::move(f));
    return;
  }
}

void retrieval_channel_decoder_t::visit(const TerminalNode *node,
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

} // namespace retrieval
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
