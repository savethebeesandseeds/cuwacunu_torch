/* observation_pipeline.h */
#pragma once
#include "piaabo/dutils.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

/* 
  Observation Pipeline Grammar:
    Instruction Examples: 
      <BTCUSDT>{1s=60, 1m=60, 1h=24}
      <BTCUSDT>{1s=15, 1h=5, 1d=10, 1M=2}
      <BTCUSDT>{1s=60, 1m=5, 5m=3, 15m=2, 30m=2, 1h=24}
    With this the Pipieline would know to request the Broker or Query de Data 
    For pattern instructed. E.g. <BTCUSDT>{1s=60, 1m=60, 1h=24} :
      Literal "BTCUSDT" would be parsed to be the symbol
      60 candles of 1 seconds interval  []
      60 candles of 1 minute interval   []
      24 candles of 1 hour interval     []
*/

#undef OBSERVARION_PIPELINE_DEBUG /* define to see verbose parsing output */

DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instruction,    "<instruction>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_symbol,         "<symbol>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_sequence_item,  "<sequence_item>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_input_form,     "<input_form>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_letter,         "<letter>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_interval,       "<interval>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_count,          "<count>");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {


struct input_form_t {
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  size_t count;
};

struct observation_pipeline_instruction_t {
  std::string instruction;
  std::string symbol;
  std::vector<input_form_t> items;
};

/* 
 * observationPipeline is a concrete visitor that traverses the AST to 
 * extract execution data (symbol, parameters, file IDs) and executes 
 * corresponding functions based on the parsed instructions.
 */
class observationPipeline : public ASTVisitor {
private:
  std::mutex current_mutex;

public:
  std::string OBSERVATION_PIPELINE_BNF_GRAMMAR = R"(
<instruction>    ::= "<" <symbol> ">" "{" {<sequence_item>} "}" ;
<symbol>         ::= {<letter>} ;
<sequence_item>  ::= <input_form> <delimiter> | <input_form> ;
<input_form>     ::= <interval> "=" <count> ;
<delimiter>      ::= ", " | "," ;
<interval>       ::= "1s" | "1m" | "3m" | "5m" | "15m" | "30m" | "1h" | "2h" | "4h" | "6h" | "8h" | "12h" | "1d" | "3d" | "1w" | "1M" ;
<count>          ::= "100" | "60" | "30" | "24" | "15" | "10" | "5" | "2" | "1" ;
<letter>         ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z" ;
)";

  GrammarLexer bnfLexer;
  GrammarParser bnfParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;  
  
  /* Constructor: Registers executable functions */
  observationPipeline();

  /* Decode: Interprest an instruction string */
  observation_pipeline_instruction_t decode(std::string instruction);

  /* parse Grammar (dummy) */
  ProductionGrammar parseBnfGrammar();

  /* Override visit methods for each AST node type */
  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
