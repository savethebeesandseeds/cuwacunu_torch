#pragma once

#include <mutex>
#include <string>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/parser_types.h"
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

class observationChannelsPipeline : public ASTVisitor {
private:
  std::mutex current_mutex;

public:
  std::string OBSERVATION_CHANNELS_GRAMMAR_TEXT =
      cuwacunu::piaabo::dconfig::contract_space_t::observation_channels_grammar();

  GrammarLexer grammarLexer;
  GrammarParser grammarParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;

  observationChannelsPipeline();
  explicit observationChannelsPipeline(std::string grammar_text);

  observation_instruction_t decode(std::string instruction);

  ProductionGrammar parseGrammarDefinition();

  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
