/* training_components.cpp */
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

RUNTIME_WARNING("(training_components.cpp)[] mutex on training pipeline might not be needed \n");

#include <sstream>   // for debug printing

/* ───────────────────── training_instruction_t methods (non-BNF) ───────────────────── */
namespace cuwacunu {
namespace camahjucunu {
/* access methods */
const training_instruction_t::table_t
training_instruction_t::retrive_table(const std::string& table_name) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) {
    log_fatal("(training_components)[retrive_table] Table with name '%s' not found. Review instruction file: %s \n",
              table_name.c_str(), instruction_filepath.c_str());
  }
  return it->second;
}

const training_instruction_t::row_t
training_instruction_t::retrive_row(const training_instruction_t::table_t& table, std::size_t row_index) const {
  if (row_index >= table.size()) {
    log_fatal("(training_components)[retrive_row] Row index %zu is out of bounds. Review instruction file: %s \n",
              row_index, instruction_filepath.c_str());
  }
  return table[row_index];
}

const training_instruction_t::row_t
training_instruction_t::retrive_row(const std::string& table_name, std::size_t row_index) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) {
    log_fatal("(training_components)[retrive_row] Table with name '%s' not found. Review instruction file: %s \n",
              table_name.c_str(), instruction_filepath.c_str());
  }
  const table_t& table = it->second;
  if (row_index >= table.size()) {
    log_fatal("(training_components)[retrive_row] Row index %zu is out of bounds in table '%s'. Review instruction file: %s \n",
              row_index, table_name.c_str(), instruction_filepath.c_str());
  }
  return table[row_index];
}

const training_instruction_t::row_t
training_instruction_t::retrive_row(const training_instruction_t::table_t& table, const std::string& row_id) const {
  for (training_instruction_t::row_t row : table) {
    if (training_instruction_t::retrive_field(row, ROW_ID_COLUMN_HEADER) == row_id) {
      return row;
    }
  }
  log_fatal("(training_components)[retrive_field] Unable to find row_id: '%s'. Review instruction file: %s \n",
            row_id.c_str(), instruction_filepath.c_str());
}

const training_instruction_t::row_t
training_instruction_t::retrive_row(const std::string& table_name, const std::string& row_id) const {
  return training_instruction_t::retrive_row(training_instruction_t::retrive_table(table_name), row_id);
}

const std::string
training_instruction_t::retrive_field(const training_instruction_t::row_t& row, const std::string& column_name) const {
  auto it = row.find(column_name);
  if (it == row.end()) {
    log_fatal("(training_components)[retrive_field] Column with name '%s' not found in the row. Review instruction file: %s \n",
              column_name.c_str(), instruction_filepath.c_str());
  }
  return it->second;
}

const std::string
training_instruction_t::retrive_field(const std::string& table_name, std::size_t row_index, const std::string& column_name) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) {
    log_fatal("(training_components)[retrive_field] Table with name '%s' not found. Review instruction file: %s \n",
              table_name.c_str(), instruction_filepath.c_str());
  }
  const table_t& table = it->second;
  if (row_index >= table.size()) {
    log_fatal("(training_components)[retrive_field] Row index %zu is out of bounds in table '%s'. Review instruction file: %s \n",
              row_index, table_name.c_str(), instruction_filepath.c_str());
  }
  const row_t& row = table[row_index];
  auto column_it = row.find(column_name);
  if (column_it == row.end()) {
    log_fatal("(training_components)[retrive_field] Column with name '%s' not found in the row at index %zu in table '%s'. Review instruction file: %s \n",
              column_name.c_str(), row_index, table_name.c_str(), instruction_filepath.c_str());
  }
  return column_it->second;
}

const std::string
training_instruction_t::retrive_field(const training_instruction_t::table_t& table, const std::string& row_id, const std::string& column_name) const {
  return training_instruction_t::retrive_field(training_instruction_t::retrive_row(table, row_id), column_name);
}

const std::string
training_instruction_t::retrive_field(const std::string& table_name, const std::string& row_id, const std::string& column_name) const {
  return training_instruction_t::retrive_field(training_instruction_t::retrive_row(table_name, row_id), column_name);
}

/* decode raw - converts the raw deque into the table maps */
void training_instruction_t::decode_raw() {
  /* Variables */
  std::size_t header_index = 0;
  table_t* curr_table = nullptr;
  std::string curr_header;
  std::vector<std::string> temp_headers;

  /* Loop through the raw deque */
  for (auto& raw_element : raw) {
    switch (raw_element.label) {
      case TRAINING_COMPONETS_HASH_table_title: {
        if (header_index != 0 && header_index != temp_headers.size()) {
          log_fatal("(training_instruction_t)[decode_raw] detected table was left unfinished when processing %s \n",
                    raw_element.str().c_str());
        }
        tables[raw_element.value] = table_t{};        /* Create a new table */
        temp_headers.clear();                         /* Reset temp_headers for the new table */
        curr_table = &tables[raw_element.value];      /* Assign the new table */
        break;
      }
      case TRAINING_COMPONETS_HASH_header_line: {
        if (curr_table == nullptr) { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing table name, when trying to process %s \n", raw_element.str().c_str()); }
        if (tables.empty())        { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax empty tables found when trying to process header line at: %s \n", raw_element.str().c_str()); }
        temp_headers.push_back(raw_element.value);    /* append the value to the temp_headers vector */
        header_index = 0;                             /* reset the headers index */
        break;
      }
      case TRAINING_COMPONETS_HASH_item_line: {
        if (curr_table == nullptr) { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing table name, when trying to process %s \n", raw_element.str().c_str()); }
        if (temp_headers.empty())  { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing headers, when trying to process %s \n", raw_element.str().c_str()); }
        if (tables.empty())        { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax empty tables found when trying to process item line at: %s \n", raw_element.str().c_str()); }
        if (header_index == 0 || header_index >= temp_headers.size()) {
          header_index = 0;                                                  /* reset the index */
          curr_table->emplace_back();                                        /* create a new row */
        }
        curr_table->back()[temp_headers[header_index]] = raw_element.value;  /* assign the value */
        header_index++;                                                      /* increment the index */
        break;
      }
      default:
        log_fatal("(training_instruction_t)[decode_raw] unexpected syntax, when trying to process %s \n",
                  raw_element.str().c_str());
    }
  }
}

std::string training_instruction_t::str() const {
  std::ostringstream oss;

  for (const auto& [table_name, table] : tables) {
    /* Append the table name */
    oss << "[ " << table_name << " ]" << "\n";

    if (table.empty()) {
      oss << "  (Empty table)\n";
      continue;
    }

    /* Collect column names from the first row */
    std::vector<std::string> headers;
    for (auto it = table[0].begin(); it != table[0].end(); ++it) {
      headers.push_back(it->first);
    }

    /* Append the headers */
    for (auto it = headers.rbegin(); it != headers.rend(); ++it) {
      oss << std::setw(21) << *it;
    }
    oss << "\n";

    /* Append the rows */
    for (std::size_t i = 0; i < table.size(); ++i) {
      for (auto it = headers.rbegin(); it != headers.rend(); ++it) {
        auto jt = table[i].find(*it);
        if (jt != table[i].end()) {
          oss << std::setw(21) << jt->second;
        } else {
          oss << std::setw(21) << "(null)";
        }
      }
      oss << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */

/* ───────────────────────────── BNF::trainingPipeline ───────────────────────────── */
namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

static inline bool stack_has(const VisitorContext& ctx, uint64_t h) {
  for (const auto* n : ctx.stack) {
    if (n && n->hash == h) return true;
  }
  return false;
}

trainingPipeline::trainingPipeline()
  : bnfLexer(TRAINING_COMPONETS_BNF_GRAMMAR)
  , bnfParser(bnfLexer)
  , grammar(parseBnfGrammar())
  , iParser(iLexer, grammar)
{
#ifdef TRAINING_COMPONETS_DEBUG
  log_info("%s\n", TRAINING_COMPONETS_BNF_GRAMMAR);
#endif
}

cuwacunu::camahjucunu::training_instruction_t
trainingPipeline::decode(std::string instruction)
{
#ifdef TRAINING_COMPONETS_DEBUG
  log_info("Request to decode trainingPipeline\n");
#endif
  /* guard the thread to avoid multiple decoding in parallel */
  LOCK_GUARD(current_mutex);

  /* Parse the instruction text */
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef TRAINING_COMPONETS_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  /* Parsed data */
  cuwacunu::camahjucunu::training_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));

  /* file */
  current.instruction_filepath = instruction;

  /* decode and traverse the Abstract Syntax Tree */
  actualAST.get()->accept(*this, context);

  log_dbg("[trainingPipeline] raw.size()=%zu tables(before decode_raw)=%zu\n", current.raw.size(), current.tables.size());

  /* decode the raw data into maps */
  current.decode_raw();
  log_dbg("[trainingPipeline] tables(after decode_raw)=%zu\n", current.tables.size());
  for (const auto& kv : current.tables) {
    log_dbg("  table='%s' rows=%zu\n", kv.first.c_str(), kv.second.size());
  }

  return current;
}

ProductionGrammar trainingPipeline::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

void trainingPipeline::visit(const RootNode* node, VisitorContext& context) {
#ifdef TRAINING_COMPONETS_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
  /* Decode User Data */
  auto* current = static_cast<cuwacunu::camahjucunu::training_instruction_t*>(context.user_data);

  /* Initialize */
  current->current_element_value = nullptr;
}

void trainingPipeline::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef TRAINING_COMPONETS_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif

  auto* current = static_cast<cuwacunu::camahjucunu::training_instruction_t*>(context.user_data);
  if (!current) return;

  // ---- 1) Null / ignore blocks (use node->hash, not stack size/index) ----
  if (node->hash == TRAINING_COMPONETS_HASH_comment ||
      node->hash == TRAINING_COMPONETS_HASH_break_block ||
      node->hash == TRAINING_COMPONETS_HASH_whitespace ||
      node->hash == TRAINING_COMPONETS_HASH_div) {
    current->current_element_value = nullptr;
    return;
  }

  // ---- 2) Table title ----
  // Old code required stack.size()==4 with exact positions.
  // New code: "we're in a <table_title> node, and somewhere above we are in instruction/table/table_header"
  if (node->hash == TRAINING_COMPONETS_HASH_table_title &&
      stack_has(context, TRAINING_COMPONETS_HASH_instruction) &&
      stack_has(context, TRAINING_COMPONETS_HASH_table) &&
      stack_has(context, TRAINING_COMPONETS_HASH_table_header)) {

    current->raw.emplace_back(TRAINING_COMPONETS_HASH_table_title, "");
    current->current_element_value = &current->raw.back().value;
    return;
  }

  // ---- 3) Header fields ----
  // Old code required stack.size()==5 with exact positions ending in <field>.
  // New code: "we are visiting a <field> node and somewhere above we are in header_line + cell + table + instruction"
  if (node->hash == TRAINING_COMPONETS_HASH_field &&
      stack_has(context, TRAINING_COMPONETS_HASH_instruction) &&
      stack_has(context, TRAINING_COMPONETS_HASH_table) &&
      stack_has(context, TRAINING_COMPONETS_HASH_header_line) &&
      stack_has(context, TRAINING_COMPONETS_HASH_cell)) {

    current->raw.emplace_back(TRAINING_COMPONETS_HASH_header_line, "");
    current->current_element_value = &current->raw.back().value;
    return;
  }

  // ---- 4) Item fields ----
  if (node->hash == TRAINING_COMPONETS_HASH_field &&
      stack_has(context, TRAINING_COMPONETS_HASH_instruction) &&
      stack_has(context, TRAINING_COMPONETS_HASH_table) &&
      stack_has(context, TRAINING_COMPONETS_HASH_item_line) &&
      stack_has(context, TRAINING_COMPONETS_HASH_cell)) {

    current->raw.emplace_back(TRAINING_COMPONETS_HASH_item_line, "");
    current->current_element_value = &current->raw.back().value;
    return;
  }
}

void trainingPipeline::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef TRAINING_COMPONETS_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif

  auto* current = static_cast<cuwacunu::camahjucunu::training_instruction_t*>(context.user_data);

  /* Append terminal lexeme to the current field value */
  if (current->current_element_value != nullptr) {
    std::string aux = node->unit.lexeme;
    cuwacunu::piaabo::string_remove(aux, '\"');
    *(current->current_element_value) += aux;
  }
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
