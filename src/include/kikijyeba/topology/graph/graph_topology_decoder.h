#pragma once

#include <mutex>
#include <string>

#include "kikijyeba/topology/graph/graph_topology_spec.h"
#include "ujcamei/source/contract/syntax/parser_types.h"

namespace cuwacunu {
namespace kikijyeba {
namespace topology {
namespace graph {

using namespace ::cuwacunu::ujcamei::source::contract::syntax;

class graph_topology_decoder_t : public ASTVisitor {
private:
  std::mutex current_mutex;

public:
  std::string SOURCE_GRAPH_GRAMMAR_TEXT{};

  GrammarLexer grammarLexer;
  GrammarParser grammarParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;

  explicit graph_topology_decoder_t(std::string grammar_text);

  graph_topology_spec_t decode(std::string instruction);

  ProductionGrammar parseGrammarDefinition();

  void visit(const RootNode *node, VisitorContext &context) override;
  void visit(const IntermediaryNode *node, VisitorContext &context) override;
  void visit(const TerminalNode *node, VisitorContext &context) override;
};

} /* namespace graph */
} /* namespace topology */
} /* namespace kikijyeba */
} /* namespace cuwacunu */
