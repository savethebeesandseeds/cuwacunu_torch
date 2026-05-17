#include "ujcamei/source/contract/dsl/channel_forms_decoder.h"

#include <filesystem>
#include <sstream>
#include <utility>

#include "ujcamei/source/contract/dsl/parse_utils.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {
namespace dsl {

channel_forms_decoder_t::channel_forms_decoder_t(std::string grammar_text)
    : SOURCE_CHANNELS_GRAMMAR_TEXT(std::move(grammar_text)),
      grammarLexer(SOURCE_CHANNELS_GRAMMAR_TEXT), grammarParser(grammarLexer),
      grammar(parseGrammarDefinition()), iParser(iLexer, grammar) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("%s\n", SOURCE_CHANNELS_GRAMMAR_TEXT.c_str());
#endif
}

source_spec_t channel_forms_decoder_t::decode(std::string instruction) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("Request to decode channel_forms_decoder_t\n");
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

ProductionGrammar channel_forms_decoder_t::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void channel_forms_decoder_t::visit(const RootNode *node,
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

void channel_forms_decoder_t::visit(const IntermediaryNode *node,
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
    out->channel_forms.clear();
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

    try {
      f.interval = cuwacunu::ujcamei::source::types::string_to_enum<
          cuwacunu::ujcamei::source::types::interval_type_e>(interval_s);
    } catch (...) {
    }

    out->channel_forms.push_back(std::move(f));
    return;
  }
}

void channel_forms_decoder_t::visit(const TerminalNode *node,
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

} // namespace dsl
} // namespace contract
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
