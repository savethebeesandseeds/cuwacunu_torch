#include "ujcamei/source/contract/dsl/graph_forms_decoder.h"

#include <sstream>
#include <utility>

#include "ujcamei/source/contract/dsl/parse_utils.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {
namespace dsl {

graph_forms_decoder_t::graph_forms_decoder_t(std::string grammar_text)
    : SOURCE_GRAPH_GRAMMAR_TEXT(std::move(grammar_text)),
      grammarLexer(SOURCE_GRAPH_GRAMMAR_TEXT), grammarParser(grammarLexer),
      grammar(parseGrammarDefinition()), iParser(iLexer, grammar) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("%s\n", SOURCE_GRAPH_GRAMMAR_TEXT.c_str());
#endif
}

source_spec_t graph_forms_decoder_t::decode(std::string instruction) {
#ifdef SOURCE_PIPELINE_DEBUG
  log_dbg("Request to decode graph_forms_decoder_t\n");
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

ProductionGrammar graph_forms_decoder_t::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void graph_forms_decoder_t::visit(const RootNode *node,
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

void graph_forms_decoder_t::visit(const IntermediaryNode *node,
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

  if (node->hash == SOURCE_PIPELINE_HASH_graph_node_table) {
    out->graph_node_forms.clear();
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_graph_edge_table) {
    out->graph_edge_forms.clear();
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_edge_resolution_policy_assignment) {
    const ASTNode *n_policy = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_edge_resolution_policy);
    out->graph_edge_resolution_policy =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_policy));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_edge_source_kind_assignment) {
    const ASTNode *n_source_kind = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_edge_source_kind);
    out->graph_edge_source_kind =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_source_kind));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_fetch_mode_assignment) {
    const ASTNode *n_fetch_mode = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_fetch_mode);
    out->graph_fetch_mode =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_fetch_mode));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_max_fetch_workers_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_unsigned_int);
    out->graph_max_fetch_workers =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_value));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_parallel_min_work_items_assignment) {
    const ASTNode *n_value = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_policy_unsigned_int);
    out->graph_parallel_min_work_items =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_value));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_graph_node_form) {
    graph_node_form_t f{};
    const ASTNode *n_node_id =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_node_id);
    const ASTNode *n_node_kind =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_node_kind);
    const ASTNode *n_active =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_active);

    f.node_id = detail::trim_spaces_tabs(detail::flatten_node_text(n_node_id));
    f.node_kind =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_node_kind));
    f.active = detail::trim_spaces_tabs(detail::flatten_node_text(n_active));
    out->graph_node_forms.push_back(std::move(f));
    return;
  }

  if (node->hash == SOURCE_PIPELINE_HASH_graph_edge_form) {
    graph_edge_form_t f{};
    const ASTNode *n_edge_id =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_edge_id);
    const ASTNode *n_base_node =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_base_node);
    const ASTNode *n_quote_node = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_quote_node);
    const ASTNode *n_source_instrument = detail::find_direct_child_by_hash(
        node, SOURCE_PIPELINE_HASH_source_instrument);
    const ASTNode *n_active =
        detail::find_direct_child_by_hash(node, SOURCE_PIPELINE_HASH_active);

    f.edge_id = detail::trim_spaces_tabs(detail::flatten_node_text(n_edge_id));
    f.base_node =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_base_node));
    f.quote_node =
        detail::trim_spaces_tabs(detail::flatten_node_text(n_quote_node));
    f.source_instrument = detail::trim_spaces_tabs(
        detail::flatten_node_text(n_source_instrument));
    f.active = detail::trim_spaces_tabs(detail::flatten_node_text(n_active));
    out->graph_edge_forms.push_back(std::move(f));
    return;
  }
}

void graph_forms_decoder_t::visit(const TerminalNode *node,
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
