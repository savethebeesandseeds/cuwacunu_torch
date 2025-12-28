#pragma once

#include <string>
#include <vector>

namespace cuwacunu {
namespace iinuji {

struct instructions_diag_t {
  std::vector<std::string> errors;
  std::vector<std::string> warnings;

  bool ok() const { return errors.empty(); }

  void err(std::string s)  { errors.push_back(std::move(s)); }
  void warn(std::string s) { warnings.push_back(std::move(s)); }

  void merge(const instructions_diag_t& o) {
    errors.insert(errors.end(), o.errors.begin(), o.errors.end());
    warnings.insert(warnings.end(), o.warnings.begin(), o.warnings.end());
  }
};

} // namespace iinuji
} // namespace cuwacunu
