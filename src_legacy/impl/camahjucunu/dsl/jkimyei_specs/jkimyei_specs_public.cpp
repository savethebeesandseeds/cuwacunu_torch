namespace cuwacunu {
namespace camahjucunu {

const jkimyei_specs_t::table_t jkimyei_specs_t::retrive_table(const std::string& table_name) const {
  auto it = tables.find(table_name);
  if (it == tables.end()) {
    log_fatal("(jkimyei_specs)[retrive_table] Table '%s' not found. Source: %s\n",
              table_name.c_str(), instruction_filepath.c_str());
  }
  return it->second;
}

const jkimyei_specs_t::row_t jkimyei_specs_t::retrive_row(const jkimyei_specs_t::table_t& table,
                                                          std::size_t row_index) const {
  if (row_index >= table.size()) {
    log_fatal("(jkimyei_specs)[retrive_row] Row index %zu out of bounds. Source: %s\n",
              row_index, instruction_filepath.c_str());
  }
  return table[row_index];
}

const jkimyei_specs_t::row_t jkimyei_specs_t::retrive_row(const std::string& table_name,
                                                          std::size_t row_index) const {
  const auto table = retrive_table(table_name);
  if (row_index >= table.size()) {
    log_fatal("(jkimyei_specs)[retrive_row] Row index %zu out of bounds in table '%s'. Source: %s\n",
              row_index, table_name.c_str(), instruction_filepath.c_str());
  }
  return table[row_index];
}

const jkimyei_specs_t::row_t jkimyei_specs_t::retrive_row(const jkimyei_specs_t::table_t& table,
                                                          const std::string& row_id) const {
  for (const auto& row : table) {
    auto it = row.find(ROW_ID_COLUMN_HEADER);
    if (it != row.end() && it->second == row_id) return row;
  }
  log_fatal("(jkimyei_specs)[retrive_row] row_id '%s' not found. Source: %s\n",
            row_id.c_str(), instruction_filepath.c_str());
}

const jkimyei_specs_t::row_t jkimyei_specs_t::retrive_row(const std::string& table_name,
                                                          const std::string& row_id) const {
  return retrive_row(retrive_table(table_name), row_id);
}

const std::string jkimyei_specs_t::retrive_field(const jkimyei_specs_t::row_t& row,
                                                 const std::string& column_name) const {
  auto it = row.find(column_name);
  if (it == row.end()) {
    log_fatal("(jkimyei_specs)[retrive_field] Missing column '%s'. Source: %s\n",
              column_name.c_str(), instruction_filepath.c_str());
  }
  return it->second;
}

const std::string jkimyei_specs_t::retrive_field(const std::string& table_name,
                                                 std::size_t row_index,
                                                 const std::string& column_name) const {
  return retrive_field(retrive_row(table_name, row_index), column_name);
}

const std::string jkimyei_specs_t::retrive_field(const jkimyei_specs_t::table_t& table,
                                                 const std::string& row_id,
                                                 const std::string& column_name) const {
  return retrive_field(retrive_row(table, row_id), column_name);
}

const std::string jkimyei_specs_t::retrive_field(const std::string& table_name,
                                                 const std::string& row_id,
                                                 const std::string& column_name) const {
  return retrive_field(retrive_row(table_name, row_id), column_name);
}

void jkimyei_specs_t::decode_raw() {
  /* v2 parser materializes tables directly; legacy raw decode stage is now a no-op */
}

std::string jkimyei_specs_t::str() const {
  std::ostringstream oss;
  for (const auto& kv : tables) {
    oss << "[ " << kv.first << " ]\n";
    if (kv.second.empty()) {
      oss << "  (empty)\n\n";
      continue;
    }

    std::unordered_set<std::string> keyset;
    for (const auto& row : kv.second) {
      for (const auto& c : row) keyset.insert(c.first);
    }
    std::vector<std::string> keys(keyset.begin(), keyset.end());
    std::sort(keys.begin(), keys.end());

    for (const auto& k : keys) oss << k << '\t';
    oss << '\n';
    for (const auto& row : kv.second) {
      for (const auto& k : keys) {
        auto it = row.find(k);
        oss << ((it == row.end()) ? std::string("-") : it->second) << '\t';
      }
      oss << '\n';
    }
    oss << '\n';
  }
  return oss.str();
}

} // namespace camahjucunu
} // namespace cuwacunu

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

jkimyeiSpecsPipeline::jkimyeiSpecsPipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  if (grammar_text_.empty()) {
    DEV_WARNING("(jkimyei_specs) empty grammar text provided; parser uses built-in JKSPEC tokenizer/parser\n");
  }
}

jkimyei_specs_t jkimyeiSpecsPipeline::decode(std::string instruction,
                                             std::string instruction_label) {
  LOCK_GUARD(current_mutex_);

  jkimyei_specs_t out{};
  out.instruction_filepath = std::move(instruction_label);

  try {
    parser_t parser(std::move(instruction));
    const document_t doc = parser.parse();
    validate_document(doc);
    materialize_document(doc, &out);
  } catch (const std::exception& e) {
    log_fatal("(jkimyei_specs) decode failed: %s\n", e.what());
  }

  return out;
}

jkimyei_specs_t decode_jkimyei_specs_from_dsl(
    std::string grammar_text,
    std::string instruction_text,
    std::string instruction_label) {
  jkimyeiSpecsPipeline decoder(std::move(grammar_text));
  return decoder.decode(std::move(instruction_text), std::move(instruction_label));
}

} // namespace dsl
} // namespace camahjucunu
} // namespace cuwacunu
