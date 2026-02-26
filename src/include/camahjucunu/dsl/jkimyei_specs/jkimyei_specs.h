/* jkimyei_specs.h */
#pragma once
#include <vector>
#include <deque>
#include <sstream>
#include <iomanip>
#include <unordered_map>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/parser_types.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs_utils.h"

DEFINE_HASH(JKIMYEI_SPECS_HASH_instruction,        "<instruction>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table,              "<table>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_comment,            "<comment>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table_header,       "<table_header>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table_top_line,     "<table_top_line>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_header_line,        "<header_line>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table_divider_line, "<table_divider_line>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_item_line,          "<item_line>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table_bottom_line,  "<table_bottom_line>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_cell,               "<cell>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_line_start,         "<line_start>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_line_ending,        "<line_ending>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_table_title,        "<table_title>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_field,              "<field>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_break_block,        "<break_block>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_character,          "<character>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_literal,            "<literal>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_whitespace,         "<whitespace>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_div,                "<div>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_frame_char,         "<frame_char>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_special,            "<special>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_super_special,      "<super_special>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_letter,             "<letter>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_number,             "<number>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_newline,            "<newline>");
DEFINE_HASH(JKIMYEI_SPECS_HASH_empty,              "<empty>");

#undef JKIMYEI_SPECS_DEBUG          /* define to see verbose parsing output */

namespace cuwacunu {
namespace camahjucunu {

struct jkimyei_specs_t {
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
        case JKIMYEI_SPECS_HASH_table_title: label_str = "<table_title>"; break;
        case JKIMYEI_SPECS_HASH_header_line: label_str = "<header_line>"; break;
        case JKIMYEI_SPECS_HASH_item_line: label_str = "<item_line>"; break;
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

class jkimyeiSpecsConfAccess {
  jkimyei_specs_t train_inst_;
  const std::string table_name_;
  const std::string row_id_;
public:
  jkimyeiSpecsConfAccess(jkimyei_specs_t train_inst, std::string table_name, std::string row_id) :
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
namespace dsl {

/* 
 * jkimyeiSpecsPipeline is a concrete visitor that traverses the AST to 
 * extract execution data (symbol, parameters, file IDs) and executes 
 * corresponding functions based on the parsed instructions.
 */
class jkimyeiSpecsPipeline : public ASTVisitor {
private:
  std::mutex current_mutex;

public:

  std::string JKIMYEI_SPECS_GRAMMAR_TEXT =
      cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_grammar();

  GrammarLexer grammarLexer;
  GrammarParser grammarParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;  
  
  /* Constructor: Registers executable functions */
  jkimyeiSpecsPipeline();
  explicit jkimyeiSpecsPipeline(std::string grammar_text);

  /* Decode: Interprest an instruction string */
  jkimyei_specs_t decode(std::string instruction);

  /* parse Grammar (dummy) */
  ProductionGrammar parseGrammarDefinition();

  /* Override visit methods for each AST node type */
  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};

jkimyei_specs_t decode_jkimyei_specs_from_dsl(
    std::string grammar_text,
    std::string instruction_text);
// Decode specs from the configured board contract.
jkimyei_specs_t decode_jkimyei_specs_from_config();

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
