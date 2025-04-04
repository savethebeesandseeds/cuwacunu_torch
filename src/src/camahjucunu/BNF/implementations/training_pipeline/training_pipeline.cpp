/* training_pipeline.cpp */
#include "camahjucunu/BNF/implementations/training_pipeline/training_pipeline.h"

RUNTIME_WARNING("(training_pipeline.cpp)[] mutex on training pipeline might not be needed \n");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {


trainingPipeline::trainingPipeline() :
  bnfLexer(TRAINING_PIPELINE_BNF_GRAMMAR), 
  bnfParser(bnfLexer), 
  grammar(parseBnfGrammar()), 
  iParser(iLexer, grammar) {
#ifdef TRAINING_PIPELINE_DEBUG
  std::cout << TRAINING_PIPELINE_BNF_GRAMMAR << "\n";
#endif
}

training_instruction_t trainingPipeline::decode(std::string instruction) {
#ifdef TRAINING_PIPELINE_DEBUG
  std::cout << "Request to decode trainingPipeline" << "\n";
#endif
  /* guard the thread to avoid multiple decoding in parallel */
  LOCK_GUARD(current_mutex);
  /* Parse the instruction text */
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);
  
#ifdef TRAINING_PIPELINE_DEBUG
  std::cout << "Parsed AST:\n";
  printAST(actualAST.get(), true, 2, std::cout);
#endif

  
  /* Parsed data */
  training_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));
  
  /* file */
  current.instruction_filepath = instruction;

  /* decode and transverse the Abstract Syntax Tree */
  actualAST.get()->accept(*this, context);

  /* decode the raw data into maps */
  current.decode_raw();

  return current;
}

ProductionGrammar trainingPipeline::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

void trainingPipeline::visit(const RootNode* node, VisitorContext& context) {
#ifdef TRAINING_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------    Decode User Data     ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  training_instruction_t *current = static_cast<training_instruction_t*>(context.user_data);

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------       Initialize        ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  current->current_element_value = nullptr;
}

void trainingPipeline::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef TRAINING_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------    Decode User Data     ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  training_instruction_t *current = static_cast<training_instruction_t*>(context.user_data);

  
  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------           Null          ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  if(    context.stack.back()->hash == TRAINING_PIPELINE_HASH_comment
      || context.stack.back()->hash == TRAINING_PIPELINE_HASH_break_block
      || context.stack.back()->hash == TRAINING_PIPELINE_HASH_whitespace
      || context.stack.back()->hash == TRAINING_PIPELINE_HASH_div) {
      /* Ignore */
      current->current_element_value = nullptr;
  }
  
  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------       Table Title       ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  if( context.stack.size() == 4 
    && context.stack[0]->hash == TRAINING_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == TRAINING_PIPELINE_HASH_table
    && context.stack[2]->hash == TRAINING_PIPELINE_HASH_table_header
    && context.stack[3]->hash == TRAINING_PIPELINE_HASH_table_title) {
      /* Push back the new element to the raw deque */
      current->raw.emplace_back(TRAINING_PIPELINE_HASH_table_title, "");
      /* Assing a pointer to the last item's value */
      current->current_element_value = &current->raw.back().value;
  }

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------       Header Line       ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  if( context.stack.size() == 5 
    && context.stack[0]->hash == TRAINING_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == TRAINING_PIPELINE_HASH_table
    && context.stack[2]->hash == TRAINING_PIPELINE_HASH_header_line
    && context.stack[3]->hash == TRAINING_PIPELINE_HASH_cell
    && context.stack[4]->hash == TRAINING_PIPELINE_HASH_field) {
      /* Push back the new element to the raw deque */
      current->raw.emplace_back(TRAINING_PIPELINE_HASH_header_line, "");
      /* Assing a pointer to the last item's value */
      current->current_element_value = &current->raw.back().value;
  }

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------        Item Line        ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  if( context.stack.size() == 5 
    && context.stack[0]->hash == TRAINING_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == TRAINING_PIPELINE_HASH_table
    && context.stack[2]->hash == TRAINING_PIPELINE_HASH_item_line
    && context.stack[3]->hash == TRAINING_PIPELINE_HASH_cell
    && context.stack[4]->hash == TRAINING_PIPELINE_HASH_field) {
      /* Push back the new element to the raw deque */
      current->raw.emplace_back(TRAINING_PIPELINE_HASH_item_line, "");
      /* Assing a pointer to the last item's value */
      current->current_element_value = &current->raw.back().value;
  }
}

void trainingPipeline::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef TRAINING_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------    Decode User Data     ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  training_instruction_t *current = static_cast<training_instruction_t*>(context.user_data);
  
  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------      Assign Values      ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  if(current->current_element_value != nullptr) {
    /* clean the terminal lexeme string */
    std::string aux = node->unit.lexeme;
    cuwacunu::piaabo::string_remove(aux, '\"');
    /* add the terminal value to the current element */
    *(current->current_element_value) += aux;
  }
}



/* ----------------------- ----------------------- ----------------------- */
/* -----------------  training_instruction_t      --------------- */
/* ----------------------- ----------------------- ----------------------- */

/* access methods */
const training_instruction_t::table_t training_instruction_t::retrive_table (const std::string& table_name) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) { log_fatal("(training_pipeline)[retrive_table] Table with name '%s' not found. Review instruction file: %s \n", table_name.c_str(), instruction_filepath.c_str()); }
  return it->second;
}
const training_instruction_t::row_t training_instruction_t::retrive_row   (const training_instruction_t::table_t& table, std::size_t row_index) const {
  if (row_index >= table.size()) { log_fatal("(training_pipeline)[retrive_row] Row index %zu is out of bounds. Review instruction file: %s \n", row_index, instruction_filepath.c_str()); }
  return table[row_index];
}
const training_instruction_t::row_t training_instruction_t::retrive_row   (const std::string& table_name, std::size_t row_index) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) { log_fatal("(training_pipeline)[retrive_row] Table with name '%s' not found. Review instruction file: %s \n", table_name.c_str(), instruction_filepath.c_str()); }
  const table_t& table = it->second;
  if (row_index >= table.size()) { log_fatal("(training_pipeline)[retrive_row] Row index %zu is out of bounds in table '%s'. Review instruction file: %s \n", row_index, table_name.c_str(), instruction_filepath.c_str()); }
  return table[row_index];
}
const training_instruction_t::row_t training_instruction_t::retrive_row   (const training_instruction_t::table_t& table, const std::string& row_id) const {
  for(training_instruction_t::row_t row: table) { if(training_instruction_t::retrive_field(row, ROW_ID_COLUMN_HEADER) == row_id) { return row; } }
  log_fatal("(training_pipeline)[retrive_field] Unable to find row_id: '%s'. Review instruction file: %s \n", row_id.c_str(), instruction_filepath.c_str());
}
const training_instruction_t::row_t training_instruction_t::retrive_row   (const std::string& table_name, const std::string& row_id) const {
  return training_instruction_t::retrive_row(training_instruction_t::retrive_table(table_name), row_id);
}
const std::string training_instruction_t::retrive_field (const training_instruction_t::row_t& row, const std::string& column_name) const {
  auto it = row.find(column_name);
  if (it == row.end()) { log_fatal("(training_pipeline)[retrive_field] Column with name '%s' not found in the row. Review instruction file: %s \n", column_name.c_str(), instruction_filepath.c_str()); }
  return it->second;
}
const std::string training_instruction_t::retrive_field (const std::string& table_name, std::size_t row_index, const std::string& column_name) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) { log_fatal("(training_pipeline)[retrive_field] Table with name '%s' not found. Review instruction file: %s \n", table_name.c_str(), instruction_filepath.c_str()); }
  const table_t& table = it->second;
  if (row_index >= table.size()) { log_fatal("(training_pipeline)[retrive_field] Row index %zu is out of bounds in table '%s'. Review instruction file: %s \n", row_index, table_name.c_str(), instruction_filepath.c_str()); }
  const row_t& row = table[row_index];
  auto column_it = row.find(column_name);
  if (column_it == row.end()) { log_fatal("(training_pipeline)[retrive_field] Column with name '%s' not found in the row at index %zu in table '%s'. Review instruction file: %s \n", column_name.c_str(), row_index, table_name.c_str(), instruction_filepath.c_str()); }
  return column_it->second;
}
const std::string training_instruction_t::retrive_field (const training_instruction_t::table_t& table, const std::string& row_id, const std::string& column_name) const {
  return training_instruction_t::retrive_field(training_instruction_t::retrive_row(table, row_id), column_name);
}

const std::string training_instruction_t::retrive_field (const std::string& table_name, const std::string& row_id, const std::string& column_name) const {
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
    switch(raw_element.label) {
      case TRAINING_PIPELINE_HASH_table_title: {
        if(header_index != 0 && header_index != temp_headers.size()) { log_fatal("(training_instruction_t)[decode_raw] detected table was left unfinished when processing %s \n", raw_element.str().c_str()); }
        tables[raw_element.value] = table_t{};        /* Create a new table */
        temp_headers.clear();                         /* Reset temp_headers for the new table */
        curr_table = &tables[raw_element.value];      /* Assing the new table */
        break;
      }
      case TRAINING_PIPELINE_HASH_header_line: {
        if(curr_table == nullptr) { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing table name, when trying to process %s \n", raw_element.str().c_str()); }
        if(tables.empty())        { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax empty tables found when trying to process header line at: %s \n", raw_element.str().c_str()); }
        temp_headers.push_back(raw_element.value);    /* append the value to the temp_headers vector */
        header_index = 0;                             /* reset the headers index */
        break;
      }
      case TRAINING_PIPELINE_HASH_item_line: {
        if(curr_table == nullptr) { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing table name, when trying to process %s \n", raw_element.str().c_str()); }
        if(temp_headers.empty())  { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax missing headers, when trying to process %s \n", raw_element.str().c_str()); }
        if(tables.empty())        { log_fatal("(training_instruction_t)[decode_raw] incorrect syntax empty tables found when trying to process item line at: %s \n", raw_element.str().c_str()); }
        if(header_index == 0 || header_index >= temp_headers.size()) {
          header_index = 0;                                                  /* reset the index */
          curr_table->emplace_back();                                        /* create a new row */
        }
        curr_table->back()[temp_headers[header_index]] = raw_element.value;  /* assign the value */
        header_index++;                                                      /* incremenet the index */
        break;
      }
      default:
        log_fatal("(training_instruction_t)[decode_raw] unexpected syntax, when trying to process %s \n", raw_element.str().c_str());
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
      oss << std::setw(21) << *it; /* Set width for each header */
    }
    oss << "\n";

    /* Append the rows */
    for (std::size_t i = 0; i < table.size(); ++i) {
      for (auto it = headers.rbegin(); it != headers.rend(); ++it) {
        /* Safely get the value for the header in the row */
        auto jt = table[i].find(*it);
        if (jt != table[i].end()) {
          oss << std::setw(21) << jt->second; /* Align each column */
        } else {
          oss << std::setw(21) << "(null)"; /* Handle missing values */
        }
      }
      oss << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
