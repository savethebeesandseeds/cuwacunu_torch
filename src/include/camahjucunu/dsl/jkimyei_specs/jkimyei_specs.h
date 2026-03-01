/* jkimyei_specs.h */
#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs_utils.h"

namespace cuwacunu {
namespace camahjucunu {

struct jkimyei_specs_t {
  struct raw_element_t {
    std::string label;
    std::string value;
    raw_element_t() = default;
    raw_element_t(std::string lbl, std::string val)
        : label(std::move(lbl)), value(std::move(val)) {}
    std::string str() const {
      return cuwacunu::piaabo::string_format("raw_element_t: label=%s, value=%s",
                                             label.c_str(), value.c_str());
    }
  };

  using row_t = std::unordered_map<std::string, std::string>;
  using table_t = std::vector<row_t>;

  std::string instruction_filepath{};
  std::unordered_map<std::string, table_t> tables{};
  std::deque<raw_element_t> raw{};

  const table_t retrive_table(const std::string& table_name) const;
  const row_t retrive_row(const table_t& table, std::size_t row_index) const;
  const row_t retrive_row(const std::string& table_name, std::size_t row_index) const;
  const row_t retrive_row(const table_t& table, const std::string& row_id) const;
  const row_t retrive_row(const std::string& table_name, const std::string& row_id) const;
  const std::string retrive_field(const row_t& row, const std::string& column_name) const;
  const std::string retrive_field(const std::string& table_name,
                                  std::size_t row_index,
                                  const std::string& column_name) const;
  const std::string retrive_field(const table_t& table,
                                  const std::string& row_id,
                                  const std::string& column_name) const;
  const std::string retrive_field(const std::string& table_name,
                                  const std::string& row_id,
                                  const std::string& column_name) const;

  void decode_raw();
  std::string str() const;
};

class jkimyeiSpecsConfAccess {
  jkimyei_specs_t train_inst_;
  const std::string table_name_;
  const std::string row_id_;

 public:
  jkimyeiSpecsConfAccess(jkimyei_specs_t train_inst,
                         std::string table_name,
                         std::string row_id)
      : train_inst_(std::move(train_inst)),
        table_name_(std::move(table_name)),
        row_id_(std::move(row_id)) {}

  template <typename T>
  T operator()(const std::string& column_name) const {
    return cuwacunu::piaabo::string_cast<T>(
        train_inst_.retrive_field(table_name_, row_id_, column_name));
  }
};

} /* namespace camahjucunu */
} /* namespace cuwacunu */

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

class jkimyeiSpecsPipeline {
 public:
  explicit jkimyeiSpecsPipeline(std::string grammar_text);
  jkimyei_specs_t decode(std::string instruction);

 private:
  std::mutex current_mutex_;
  std::string grammar_text_;
};

jkimyei_specs_t decode_jkimyei_specs_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
