/* tsiemene_board.h */
#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

#undef TSIEMENE_BOARD_DEBUG /* define to see verbose parsing output */

DEFINE_HASH(TSIEMENE_BOARD_HASH_instruction,   "<instruction>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_circuit,       "<circuit>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_circuit_header,"<circuit_header>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_circuit_close, "<circuit_close>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_circuit_invoke,"<circuit_invoke>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_circuit_name,  "<circuit_name>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_invoke_name,   "<invoke_name>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_invoke_payload,"<invoke_payload>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_instance_decl, "<instance_decl>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_instance_alias,"<instance_alias>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_tsi_type,      "<tsi_type>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_hop_decl,      "<hop_decl>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_endpoint_from, "<endpoint_from>");
DEFINE_HASH(TSIEMENE_BOARD_HASH_endpoint_to,   "<endpoint_to>");

namespace cuwacunu {
namespace camahjucunu {

struct tsiemene_endpoint_t {
  std::string instance;
  std::string directive;
  std::string kind;
};

struct tsiemene_instance_decl_t {
  std::string alias;
  std::string tsi_type;
};

struct tsiemene_hop_decl_t {
  tsiemene_endpoint_t from;
  tsiemene_endpoint_t to;
};

struct tsiemene_circuit_decl_t {
  std::string name;
  std::vector<tsiemene_instance_decl_t> instances;
  std::vector<tsiemene_hop_decl_t> hops;
  std::string invoke_name;
  std::string invoke_payload;
};

struct tsiemene_board_instruction_t {
  std::vector<tsiemene_circuit_decl_t> circuits;
  std::string str() const;
};

namespace BNF {
class tsiemeneBoard : public ASTVisitor {
 private:
  std::mutex current_mutex;

 public:
  std::string TSIEMENE_BOARD_BNF_GRAMMAR = cuwacunu::piaabo::dconfig::config_space_t::tsiemene_board_bnf();

  GrammarLexer bnfLexer;
  GrammarParser bnfParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;

  tsiemeneBoard();
  tsiemene_board_instruction_t decode(std::string instruction);
  ProductionGrammar parseBnfGrammar();

  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};
} /* namespace BNF */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
