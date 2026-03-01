#pragma once

#include <mutex>
#include <string>

#include "camahjucunu/dsl/parser_types.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

class observationChannelsDecoder : public ASTVisitor {
private:
  std::mutex current_mutex;

public:
  std::string OBSERVATION_CHANNELS_GRAMMAR_TEXT{};

  GrammarLexer grammarLexer;
  GrammarParser grammarParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;

  explicit observationChannelsDecoder(std::string grammar_text);

  observation_spec_t decode(std::string instruction);

  ProductionGrammar parseGrammarDefinition();

  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
