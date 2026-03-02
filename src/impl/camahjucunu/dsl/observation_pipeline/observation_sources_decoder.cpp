#include "camahjucunu/dsl/observation_pipeline/observation_sources_decoder.h"

#include <filesystem>
#include <sstream>
#include <utility>

#include "camahjucunu/dsl/observation_pipeline/observation_parse_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

observationSourcesDecoder::observationSourcesDecoder(std::string grammar_text)
  : OBSERVATION_SOURCES_GRAMMAR_TEXT(std::move(grammar_text))
  , grammarLexer(OBSERVATION_SOURCES_GRAMMAR_TEXT)
  , grammarParser(grammarLexer)
  , grammar(parseGrammarDefinition())
  , iParser(iLexer, grammar)
{
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_info("%s\n", OBSERVATION_SOURCES_GRAMMAR_TEXT.c_str());
#endif
}

observation_spec_t observationSourcesDecoder::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_info("Request to decode observationSourcesDecoder\n");
#endif
  LOCK_GUARD(current_mutex);
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  observation_spec_t current;
  VisitorContext context(static_cast<void*>(&current));
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar observationSourcesDecoder::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void observationSourcesDecoder::visit(const RootNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif

  (void)node;
  (void)context;
}

void observationSourcesDecoder::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif

  auto* out = static_cast<observation_spec_t*>(context.user_data);
  if (!out) return;

  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_table) {
    out->source_forms.clear();
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    observation_source_t f{};

    const ASTNode* n_instrument = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_instrument);
    const ASTNode* n_interval = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_interval);
    const ASTNode* n_record_type = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_record_type);
    const ASTNode* n_source = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_source);

    f.instrument = detail::trim_spaces_tabs(detail::flatten_node_text(n_instrument));
    std::string interval_s = detail::trim_spaces_tabs(detail::flatten_node_text(n_interval));
    f.record_type = detail::trim_spaces_tabs(detail::flatten_node_text(n_record_type));
    f.source = detail::trim_spaces_tabs(detail::flatten_node_text(n_source));

    try {
      f.interval = cuwacunu::camahjucunu::exchange::string_to_enum<
          cuwacunu::camahjucunu::exchange::interval_type_e>(interval_s);
    } catch (...) {
    }

    out->source_forms.push_back(std::move(f));
    return;
  }
}

void observationSourcesDecoder::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif

  (void)node;
  (void)context;
}

} // namespace dsl
} // namespace camahjucunu
} // namespace cuwacunu
