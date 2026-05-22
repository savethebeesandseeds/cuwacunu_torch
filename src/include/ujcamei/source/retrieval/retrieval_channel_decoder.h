#pragma once

#include <mutex>
#include <string>

#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/contract/syntax/parser_types.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace retrieval {
using namespace ::cuwacunu::ujcamei::source::contract::syntax;
using ::cuwacunu::ujcamei::source::contract::channel_form_t;
using ::cuwacunu::ujcamei::source::contract::source_spec_t;

class retrieval_channel_decoder_t : public ASTVisitor {
private:
  std::mutex current_mutex;

public:
  std::string SOURCE_CHANNELS_GRAMMAR_TEXT{};

  GrammarLexer grammarLexer;
  GrammarParser grammarParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;

  explicit retrieval_channel_decoder_t(std::string grammar_text);

  source_spec_t decode(std::string instruction);

  ProductionGrammar parseGrammarDefinition();

  void visit(const RootNode *node, VisitorContext &context) override;
  void visit(const IntermediaryNode *node, VisitorContext &context) override;
  void visit(const TerminalNode *node, VisitorContext &context) override;
};

} /* namespace retrieval */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
