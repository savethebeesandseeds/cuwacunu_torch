#include "camahjucunu/dsl/observation_pipeline/observation_channels_decoder.h"

#include <filesystem>
#include <sstream>
#include <utility>

#include "camahjucunu/dsl/observation_pipeline/observation_parse_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

observationChannelsDecoder::observationChannelsDecoder(std::string grammar_text)
  : OBSERVATION_CHANNELS_GRAMMAR_TEXT(std::move(grammar_text))
  , grammarLexer(OBSERVATION_CHANNELS_GRAMMAR_TEXT)
  , grammarParser(grammarLexer)
  , grammar(parseGrammarDefinition())
  , iParser(iLexer, grammar)
{
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_info("%s\n", OBSERVATION_CHANNELS_GRAMMAR_TEXT.c_str());
#endif
}

observation_spec_t observationChannelsDecoder::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_info("Request to decode observationChannelsDecoder\n");
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

ProductionGrammar observationChannelsDecoder::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void observationChannelsDecoder::visit(const RootNode* node, VisitorContext& context) {
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

void observationChannelsDecoder::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif

  auto* out = static_cast<observation_spec_t*>(context.user_data);
  if (!out) return;

  if (node->hash == OBSERVATION_PIPELINE_HASH_input_table) {
    out->channel_forms.clear();
    return;
  }

  if (node->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    observation_channel_t f{};

    const ASTNode* n_interval = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_interval);
    const ASTNode* n_active = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_active);
    const ASTNode* n_record_type = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_record_type);
    const ASTNode* n_seq_length = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_seq_length);
    const ASTNode* n_future_seq_length = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_future_seq_length);
    const ASTNode* n_channel_weight = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_channel_weight);
    const ASTNode* n_norm_window = detail::find_direct_child_by_hash(
        node, OBSERVATION_PIPELINE_HASH_norm_window);

    std::string interval_s = detail::trim_spaces_tabs(detail::flatten_node_text(n_interval));
    f.active = detail::trim_spaces_tabs(detail::flatten_node_text(n_active));
    f.record_type = detail::trim_spaces_tabs(detail::flatten_node_text(n_record_type));
    f.seq_length = detail::trim_spaces_tabs(detail::flatten_node_text(n_seq_length));
    f.future_seq_length = detail::trim_spaces_tabs(detail::flatten_node_text(n_future_seq_length));
    f.channel_weight = detail::trim_spaces_tabs(detail::flatten_node_text(n_channel_weight));
    f.norm_window = detail::trim_spaces_tabs(detail::flatten_node_text(n_norm_window));

    try {
      f.interval = cuwacunu::camahjucunu::exchange::string_to_enum<
          cuwacunu::camahjucunu::exchange::interval_type_e>(interval_s);
    } catch (...) {
    }

    out->channel_forms.push_back(std::move(f));
    return;
  }
}

void observationChannelsDecoder::visit(const TerminalNode* node, VisitorContext& context) {
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
