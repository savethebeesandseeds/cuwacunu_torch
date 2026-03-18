/* tsiemene_circuit_decode_ast.cpp */
#include "tsiemene_circuit_decode_internal.h"

#include <filesystem>
#include <sstream>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

namespace {

std::string expand_implicit_hop_targets(std::string instruction) {
  // Target shorthand inference is removed; keep decode path passthrough.
  return instruction;
}

} /* namespace */

tsiemeneCircuits::tsiemeneCircuits(std::string grammar_text)
  : TSIEMENE_CIRCUIT_GRAMMAR_TEXT(std::move(grammar_text))
  , grammarLexer(TSIEMENE_CIRCUIT_GRAMMAR_TEXT)
  , grammarParser(grammarLexer)
  , grammar(parseGrammarDefinition())
  , iParser(iLexer, grammar)
{
#ifdef TSIEMENE_CIRCUIT_DEBUG
  log_info("%s\n", TSIEMENE_CIRCUIT_GRAMMAR_TEXT);
#endif
}

cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
tsiemeneCircuits::decode(std::string instruction) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  log_info("Request to decode tsiemeneCircuits\n");
#endif
  LOCK_GUARD(current_mutex);

  instruction = expand_implicit_hop_targets(std::move(instruction));

  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar tsiemeneCircuits::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void tsiemeneCircuits::visit(const RootNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg(
      "RootNode context: [%s]  ---> %s\n",
      oss.str().c_str(),
      node->lhs_instruction.c_str());
#endif
  (void)node;
  (void)context;
}

void tsiemeneCircuits::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg(
      "IntermediaryNode context: [%s]  ---> %s\n",
      oss.str().c_str(),
      node->alt.str(true).c_str());
#endif
  auto* out = static_cast<cuwacunu::camahjucunu::tsiemene_circuit_instruction_t*>(
      context.user_data);
  if (!out) return;

  if (node->hash == TSIEMENE_CIRCUIT_HASH_instruction) {
    out->circuits.clear();
    return;
  }
  if (node->hash == TSIEMENE_CIRCUIT_HASH_circuit) {
    auto circuit = tsiemene_circuit_decode_internal::parse_circuit_node(node);
    if (!circuit.name.empty()) {
      out->circuits.push_back(std::move(circuit));
    }
    return;
  }
}

void tsiemeneCircuits::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg(
      "TerminalNode context: [%s]  ---> %s\n",
      oss.str().c_str(),
      node->unit.str(true).c_str());
#endif
  (void)node;
  (void)context;
}

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
