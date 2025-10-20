/* training_components.h */
#pragma once
#include <vector>
#include <deque>
#include <sstream>
#include <iomanip>
#include <unordered_map>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/BNF/implementations/training_components/training_components_utils.h"

DEFINE_HASH(TRAINING_COMPONETS_HASH_instruction,        "<instruction>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table,              "<table>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_comment,            "<comment>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table_header,       "<table_header>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table_top_line,     "<table_top_line>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_header_line,        "<header_line>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table_divider_line, "<table_divider_line>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_item_line,          "<item_line>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table_bottom_line,  "<table_bottom_line>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_cell,               "<cell>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_line_start,         "<line_start>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_line_ending,        "<line_ending>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_table_title,        "<table_title>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_field,              "<field>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_break_block,        "<break_block>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_character,          "<character>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_literal,            "<literal>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_whitespace,         "<whitespace>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_div,                "<div>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_frame_char,         "<frame_char>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_special,            "<special>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_super_special,      "<super_special>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_letter,             "<letter>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_number,             "<number>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_newline,            "<newline>");
DEFINE_HASH(TRAINING_COMPONETS_HASH_empty,              "<empty>");

#undef TRAINING_COMPONETS_DEBUG          /* define to see verbose parsing output */

namespace cuwacunu {
namespace camahjucunu {

struct training_instruction_t {
  /* instruction file */
  std::string instruction_filepath;
  /* types */
  struct raw_element_t {
    uint64_t label;
    std::string value;
    raw_element_t(uint64_t lbl, const std::string& val) : label(lbl), value(val) {}
    std::string str() { 
      std::string label_str;
      switch(label) {
        case TRAINING_COMPONETS_HASH_table_title: label_str = "<table_title>"; break;
        case TRAINING_COMPONETS_HASH_header_line: label_str = "<header_line>"; break;
        case TRAINING_COMPONETS_HASH_item_line: label_str = "<item_line>"; break;
        default: label_str = "UNKNOWN!"; break;
      }
      return cuwacunu::piaabo::string_format("raw_element_t: label=%s, value=%s", label_str.c_str(), value.c_str());
    }
  };
  using row_t   = std::unordered_map<std::string, std::string>;
  using table_t = std::vector<row_t>;
  /* data */
  std::unordered_map<std::string, table_t> tables;
  std::deque<raw_element_t> raw;
  std::string *current_element_value;
  /* access methods */
  const table_t     retrive_table (const std::string& table_name) const;
  const row_t       retrive_row   (const table_t& table, std::size_t row_index) const;
  const row_t       retrive_row   (const std::string& table_name, std::size_t row_index) const;
  const row_t       retrive_row   (const table_t& table, const std::string& row_id) const;
  const row_t       retrive_row   (const std::string& table_name, const std::string& row_id) const;
  const std::string retrive_field (const row_t& row, const std::string& column_name) const;
  const std::string retrive_field (const std::string& table_name, std::size_t row_index, const std::string& column_name) const;
  const std::string retrive_field (const table_t& table, const std::string& row_id, const std::string& column_name) const;
  const std::string retrive_field (const std::string& table_name, const std::string& row_id, const std::string& column_name) const;
  /* decode raw - converts the raw deque into the table maps */
  void decode_raw();
  /* print method */
  std::string str() const;
};

class trainingPipe_ConfAccess {
  training_instruction_t train_inst_;
  const std::string table_name_;
  const std::string row_id_;
public:
  trainingPipe_ConfAccess(training_instruction_t train_inst, std::string table_name, std::string row_id) : 
    train_inst_(train_inst), 
    table_name_(std::move(table_name)), 
    row_id_(std::move(row_id)) {}
  template <typename T>
  T operator()(const std::string& column_name) const {
    return cuwacunu::piaabo::string_cast<T>(
      train_inst_.retrive_field(table_name_, row_id_, column_name)
    );
  }
};
} /* namespace camahjucunu */
} /* namespace cuwacunu */

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {
/* 
 * trainingPipeline is a concrete visitor that traverses the AST to 
 * extract execution data (symbol, parameters, file IDs) and executes 
 * corresponding functions based on the parsed instructions.
 */
class trainingPipeline : public ASTVisitor {
private:
  std::mutex current_mutex;

public:

  std::string TRAINING_COMPONETS_BNF_GRAMMAR = cuwacunu::piaabo::dconfig::config_space_t::training_components_bnf();

  GrammarLexer bnfLexer;
  GrammarParser bnfParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;  
  
  /* Constructor: Registers executable functions */
  trainingPipeline();

  /* Decode: Interprest an instruction string */
  training_instruction_t decode(std::string instruction);

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
