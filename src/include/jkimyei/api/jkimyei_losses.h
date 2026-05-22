/* jkimyei_losses.h */

#pragma once

#include <cassert>
#include <cmath>
#include <sstream>
#include <torch/nn/functional/loss.h>
#include <torch/torch.h>
#include <unordered_set>

#include "jkimyei/api/jkimyei_specs.h"
#include "jkimyei/api/schema_catalog.h"

namespace cuwacunu {
namespace jkimyei {

inline void ensure_loss_validation_coverage() {
  static const bool kCoverageChecked = [] {
    const std::unordered_set<std::string> kImplemented = {
        "NLLLoss",          "VICReg", "CrossEntropy", "BinaryCrossEntropy",
        "MeanSquaredError", "Hinge",  "SmoothL1",     "L1Loss"};
    for (const auto &type : cuwacunu::jkimyei::api::supported_loss_types()) {
      if (kImplemented.find(type) == kImplemented.end()) {
        throw std::runtime_error("loss declared in jkimyei_schema.def but not "
                                 "validated in jkimyei_losses: " +
                                 type);
      }
    }
    return true;
  }();
  (void)kCoverageChecked;
}
/* Map a config-row (DSL spec) to a concrete loss.
 * - Enforces exact columns: {row_id, type, options}
 * - Enforces exact options per loss (no extras, no missing)
 */
inline void validate_loss(const cuwacunu::jkimyei::specs::jkimyei_specs_t &inst,
                          const std::string &row_id) {
  ensure_loss_validation_coverage();

  const auto &row = inst.retrieve_row("loss_functions_table", row_id);

  // Exact columns; allow empties.
  cuwacunu::jkimyei::specs::require_columns_exact(
      row, {ROW_ID_COLUMN_HEADER, "type", "options"},
      /*enforce_nonempty=*/false);

  const std::string type =
      cuwacunu::jkimyei::specs::require_column(row, "type");
  const std::string canonical_type =
      (type == "MSE") ? "MeanSquaredError" : type;
  cuwacunu::jkimyei::api::require_loss_type_registered(canonical_type);

  auto ensure_no_options = [&]() {
    auto it = row.find("options");
    if (it == row.end())
      return; // column exists per require_columns_exact, but be safe
    std::string s = it->second;

    // Trim simple whitespace (no dependency on utils)
    auto l = s.find_first_not_of(" \t\r\n");
    auto r = s.find_last_not_of(" \t\r\n");
    if (l == std::string::npos)
      s.clear();
    else
      s = s.substr(l, r - l + 1);

    if (s.empty() || s == "-")
      return; // treat "" or "-" as "no options"

    auto kv = cuwacunu::jkimyei::specs::parse_options_kvlist(s);
    if (!kv.empty()) {
      std::ostringstream oss;
      oss << "Unexpected options for loss_function type='" << type
          << "'. None are allowed.";
      throw std::runtime_error(oss.str());
    }
  };

  if (canonical_type == "NLLLoss") { // MDN-NLL
    cuwacunu::jkimyei::specs::validate_options_exact(
        row, {"eps", "sigma_min", "sigma_max", "reduction"});

  } else if (canonical_type == "MeanSquaredError") {
    ensure_no_options();

  } else if (canonical_type == "L1Loss") {
    ensure_no_options();

  } else if (canonical_type == "CrossEntropy") {
    cuwacunu::jkimyei::specs::validate_options_exact(row, {"label_smoothing"});

  } else if (canonical_type == "BinaryCrossEntropy") {
    cuwacunu::jkimyei::specs::validate_options_exact(row, {"pos_weight"});

  } else if (canonical_type == "SmoothL1") {
    cuwacunu::jkimyei::specs::validate_options_exact(row, {"beta"});

  } else if (canonical_type == "Hinge") {
    cuwacunu::jkimyei::specs::validate_options_exact(row, {"margin"});

  } else if (canonical_type == "VICReg") {
    cuwacunu::jkimyei::specs::validate_options_exact(
        row, {"sim_coeff", "std_coeff", "cov_coeff", "huber_delta"});

  } else {
    throw std::runtime_error("Unknown loss_function type: " + canonical_type);
  }
}

} // namespace jkimyei
} // namespace cuwacunu
