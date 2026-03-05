// test_dsl_jkimyei_specs.cpp
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "iitepi/board_space_t.h"
#include "iitepi/config_space_t.h"
#include "iitepi/contract_space_t.h"

namespace {

using specs_t = cuwacunu::camahjucunu::jkimyei_specs_t;
using row_t = specs_t::row_t;
using table_t = specs_t::table_t;

[[noreturn]] void fail(const std::string& msg) {
  throw std::runtime_error("[test_dsl_jkimyei_specs] " + msg);
}

const table_t& require_table(const specs_t& decoded, const std::string& table_name) {
  const auto it = decoded.tables.find(table_name);
  if (it == decoded.tables.end()) fail("missing table: " + table_name);
  return it->second;
}

void expect_table_absent(const specs_t& decoded, const std::string& table_name) {
  if (decoded.tables.find(table_name) != decoded.tables.end()) {
    fail("unexpected table present: " + table_name);
  }
}

const row_t& require_row_by_id(const table_t& table,
                               const std::string& table_name,
                               const std::string& row_id) {
  for (const auto& row : table) {
    const auto it = row.find(ROW_ID_COLUMN_HEADER);
    if (it != row.end() && it->second == row_id) return row;
  }
  fail("missing row_id '" + row_id + "' in table '" + table_name + "'");
}

std::vector<const row_t*> rows_by_field(const table_t& table,
                                        const std::string& field_name,
                                        const std::string& field_value) {
  std::vector<const row_t*> out;
  for (const auto& row : table) {
    const auto it = row.find(field_name);
    if (it != row.end() && it->second == field_value) out.push_back(&row);
  }
  return out;
}

const std::string& require_field(const row_t& row,
                                 const std::string& field_name,
                                 const std::string& context) {
  const auto it = row.find(field_name);
  if (it == row.end()) fail("missing field '" + field_name + "' in " + context);
  return it->second;
}

void expect_eq(const row_t& row,
               const std::string& field_name,
               const std::string& expected,
               const std::string& context) {
  const auto& actual = require_field(row, field_name, context);
  if (actual != expected) {
    fail(context + ": field '" + field_name + "' expected '" + expected +
         "' got '" + actual + "'");
  }
}

std::map<std::string, std::string> parse_options(const std::string& options_csv) {
  std::map<std::string, std::string> out;
  if (options_csv.empty()) return out;
  std::size_t cursor = 0;
  while (cursor <= options_csv.size()) {
    const std::size_t comma = options_csv.find(',', cursor);
    const std::string token = (comma == std::string::npos)
                                  ? options_csv.substr(cursor)
                                  : options_csv.substr(cursor, comma - cursor);
    const std::size_t eq = token.find('=');
    if (eq == std::string::npos) fail("invalid options token: '" + token + "'");
    out[token.substr(0, eq)] = token.substr(eq + 1);
    if (comma == std::string::npos) break;
    cursor = comma + 1;
  }
  return out;
}

void validate_profile(const specs_t& decoded,
                      const std::string& profile_row_id,
                      const std::string& profile_name,
                      const std::string& active,
                      const std::string& linear_active,
                      const std::string& linear_noise_scale,
                      const std::string& chaotic_active) {
  const auto& profiles = require_table(decoded, "component_profiles_table");
  const auto& optimizers = require_table(decoded, "optimizers_table");
  const auto& schedulers = require_table(decoded, "lr_schedulers_table");
  const auto& losses = require_table(decoded, "loss_functions_table");
  const auto& gradient = require_table(decoded, "component_gradient_table");
  const auto& augmentations = require_table(decoded, "vicreg_augmentations");

  const row_t& p = require_row_by_id(profiles, "component_profiles_table", profile_row_id);
  expect_eq(p, "component_id", "VICReg_representation", profile_row_id);
  expect_eq(p, "component_type", "tsi.wikimyei.representation.vicreg", profile_row_id);
  expect_eq(p, "profile_id", profile_name, profile_row_id);
  expect_eq(p, "active", active, profile_row_id);
  expect_eq(p, "vicreg_use_swa", "true", profile_row_id);
  expect_eq(p, "vicreg_detach_to_cpu", "true", profile_row_id);
  expect_eq(p, "swa_start_iter", "1000", profile_row_id);
  expect_eq(p, "optimizer_threshold_reset", "500", profile_row_id);

  const std::string optimizer_id = require_field(p, "optimizer", profile_row_id);
  const std::string scheduler_id = require_field(p, "lr_scheduler", profile_row_id);
  const std::string loss_id = require_field(p, "loss_function", profile_row_id);

  const row_t& optimizer_row = require_row_by_id(optimizers, "optimizers_table", optimizer_id);
  expect_eq(optimizer_row, "type", "AdamW", optimizer_id);
  expect_eq(optimizer_row,
            "options",
            "initial_learning_rate=0.001,weight_decay=0.01,epsilon=1e-8,beta1=0.9,beta2=0.999,amsgrad=false",
            optimizer_id);

  const row_t& scheduler_row = require_row_by_id(schedulers, "lr_schedulers_table", scheduler_id);
  expect_eq(scheduler_row, "type", "ConstantLR", scheduler_id);
  expect_eq(scheduler_row, "options", "lr=0.001", scheduler_id);

  const row_t& loss_row = require_row_by_id(losses, "loss_functions_table", loss_id);
  expect_eq(loss_row, "type", "VICReg", loss_id);
  expect_eq(loss_row,
            "options",
            "sim_coeff=25.0,std_coeff=25.0,cov_coeff=1.0,huber_delta=0.03",
            loss_id);

  const row_t& grad_row = require_row_by_id(gradient, "component_gradient_table", profile_row_id);
  expect_eq(grad_row, "accumulate_steps", "1", profile_row_id + "/GRADIENT");
  expect_eq(grad_row, "clip_norm", "1.0", profile_row_id + "/GRADIENT");
  expect_eq(grad_row, "clip_value", "0.0", profile_row_id + "/GRADIENT");
  expect_eq(grad_row, "skip_on_nan", "true", profile_row_id + "/GRADIENT");
  expect_eq(grad_row, "zero_grad_set_to_none", "true", profile_row_id + "/GRADIENT");

  const auto aug_rows = rows_by_field(augmentations, "profile_row_id", profile_row_id);
  if (aug_rows.size() != 2) {
    fail(profile_row_id + ": expected 2 augmentation rows, got " +
         std::to_string(aug_rows.size()));
  }
  const row_t* linear_row = nullptr;
  const row_t* chaotic_row = nullptr;
  for (const row_t* r : aug_rows) {
    const std::string& name = require_field(*r, "name", profile_row_id + "/AUGMENTATIONS");
    if (name == "Linear") linear_row = r;
    if (name == "ChaoticDrift") chaotic_row = r;
  }
  if (!linear_row || !chaotic_row) {
    fail(profile_row_id + ": expected augmentation names [Linear, ChaoticDrift]");
  }
  expect_eq(*linear_row, "active", linear_active, profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "curve_param", "0.0", profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "noise_scale", linear_noise_scale, profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "smoothing_kernel_size", "3", profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "point_drop_prob", (profile_name == "stable_pretrain") ? "0.06" : "0.00",
            profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "value_jitter_std", (profile_name == "stable_pretrain") ? "0.015" : "0.005",
            profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "time_mask_band_frac", "0.00", profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "channel_dropout_prob", "0.00", profile_row_id + "/AUGMENTATIONS Linear");
  expect_eq(*linear_row, "comment",
            (profile_name == "stable_pretrain")
                ? "Identity plus light warp jitter and point drop"
                : "Evaluation-safe near-identity perturbation",
            profile_row_id + "/AUGMENTATIONS Linear");

  expect_eq(*chaotic_row, "active", chaotic_active, profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "curve_param", "0.0", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "noise_scale", "0.10", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "smoothing_kernel_size", "7", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "point_drop_prob", "0.08", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "value_jitter_std", "0.020", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "time_mask_band_frac", "0.03", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "channel_dropout_prob", "0.05", profile_row_id + "/AUGMENTATIONS ChaoticDrift");
  expect_eq(*chaotic_row, "comment",
            (profile_name == "stable_pretrain")
                ? "Noisier structure with smoothing and channel dropout"
                : "Disabled by default for eval payload profile",
            profile_row_id + "/AUGMENTATIONS ChaoticDrift");
}

void print_aug_matrix(const std::vector<const row_t*>& rows) {
  std::vector<std::string> curve_names;
  std::map<std::string, const row_t*> by_name;
  for (const row_t* r : rows) {
    const std::string name = require_field(*r, "name", "AUGMENTATIONS");
    if (by_name.find(name) == by_name.end()) curve_names.push_back(name);
    by_name[name] = r;
  }

  const std::vector<std::string> fields = {
      "active",
      "curve_param",
      "noise_scale",
      "smoothing_kernel_size",
      "point_drop_prob",
      "value_jitter_std",
      "time_mask_band_frac",
      "channel_dropout_prob",
      "comment",
  };

  std::size_t field_width = std::string("name").size();
  for (const auto& f : fields) field_width = std::max(field_width, f.size());

  std::vector<std::size_t> widths;
  widths.reserve(curve_names.size());
  for (const auto& c : curve_names) {
    std::size_t w = c.size();
    const row_t& r = *by_name[c];
    for (const auto& f : fields) {
      const auto it = r.find(f);
      if (it != r.end()) w = std::max(w, it->second.size());
    }
    widths.push_back(w);
  }

  std::cout << "    [AUGMENTATIONS]\n";
  std::cout << "      " << std::left << std::setw(static_cast<int>(field_width)) << "name";
  for (std::size_t i = 0; i < curve_names.size(); ++i) {
    std::cout << " | " << std::left << std::setw(static_cast<int>(widths[i])) << curve_names[i];
  }
  std::cout << "\n";
  for (const auto& field : fields) {
    std::cout << "      " << std::left << std::setw(static_cast<int>(field_width)) << field;
    for (std::size_t i = 0; i < curve_names.size(); ++i) {
      const row_t& r = *by_name[curve_names[i]];
      const auto it = r.find(field);
      const std::string value = (it == r.end()) ? "-" : it->second;
      std::cout << " | " << std::left << std::setw(static_cast<int>(widths[i])) << value;
    }
    std::cout << "\n";
  }
}

void print_profile_readable(const specs_t& decoded, const std::string& profile_row_id) {
  const auto& profiles = require_table(decoded, "component_profiles_table");
  const auto& optimizers = require_table(decoded, "optimizers_table");
  const auto& schedulers = require_table(decoded, "lr_schedulers_table");
  const auto& losses = require_table(decoded, "loss_functions_table");
  const auto& gradient = require_table(decoded, "component_gradient_table");
  const auto& augmentations = require_table(decoded, "vicreg_augmentations");

  const row_t& p = require_row_by_id(profiles, "component_profiles_table", profile_row_id);
  const std::string profile_name = require_field(p, "profile_id", profile_row_id);
  const std::string active = require_field(p, "active", profile_row_id);

  std::cout << "  PROFILE \"" << profile_name << "\" (active=" << active << ")\n";

  const row_t& optimizer_row = require_row_by_id(
      optimizers, "optimizers_table", require_field(p, "optimizer", profile_row_id));
  std::cout << "    [OPTIMIZER]\n";
  std::cout << "      type: " << require_field(optimizer_row, "type", "optimizer") << "\n";
  for (const auto& [k, v] : parse_options(require_field(optimizer_row, "options", "optimizer"))) {
    std::cout << "      " << k << ": " << v << "\n";
  }

  const row_t& scheduler_row = require_row_by_id(
      schedulers, "lr_schedulers_table", require_field(p, "lr_scheduler", profile_row_id));
  std::cout << "    [LR_SCHEDULER]\n";
  std::cout << "      type: " << require_field(scheduler_row, "type", "scheduler") << "\n";
  for (const auto& [k, v] : parse_options(require_field(scheduler_row, "options", "scheduler"))) {
    std::cout << "      " << k << ": " << v << "\n";
  }

  const row_t& loss_row = require_row_by_id(
      losses, "loss_functions_table", require_field(p, "loss_function", profile_row_id));
  std::cout << "    [LOSS]\n";
  std::cout << "      type: " << require_field(loss_row, "type", "loss") << "\n";
  for (const auto& [k, v] : parse_options(require_field(loss_row, "options", "loss"))) {
    std::cout << "      " << k << ": " << v << "\n";
  }

  const auto print_row = [&](const row_t& row, const std::string& title) {
    std::vector<std::string> keys;
    keys.reserve(row.size());
    for (const auto& [k, _] : row) {
      if (k == ROW_ID_COLUMN_HEADER || k == "component_id") continue;
      keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    std::cout << "    [" << title << "]\n";
    for (const auto& key : keys) {
      std::cout << "      " << key << ": " << row.at(key) << "\n";
    }
  };

  print_row(require_row_by_id(gradient, "component_gradient_table", profile_row_id),
            "GRADIENT");
  {
    row_t component_params_only{};
    for (const std::string& key : std::vector<std::string>{
             "vicreg_use_swa",
             "vicreg_detach_to_cpu",
             "swa_start_iter",
             "optimizer_threshold_reset"}) {
      const auto it = p.find(key);
      if (it != p.end()) component_params_only[key] = it->second;
    }
    print_row(component_params_only, "COMPONENT_PARAMS");
  }
  print_aug_matrix(rows_by_field(augmentations, "profile_row_id", profile_row_id));
}

void validate_decoded(const specs_t& decoded) {
  const auto& components = require_table(decoded, "components_table");
  const row_t& c = require_row_by_id(components, "components_table", "VICReg_representation");
  expect_eq(c, "component_type", "tsi.wikimyei.representation.vicreg", "components_table");
  expect_eq(c, "active_profile", "stable_pretrain", "components_table");
  expect_table_absent(decoded, "component_reproducibility_table");
  expect_table_absent(decoded, "component_numerics_table");
  expect_table_absent(decoded, "component_checkpoint_table");
  expect_table_absent(decoded, "component_metrics_table");
  expect_table_absent(decoded, "component_data_ref_table");

  validate_profile(decoded,
                   "VICReg_representation@stable_pretrain",
                   "stable_pretrain",
                   "true",
                   "true",
                   "0.02",
                   "true");
  validate_profile(decoded,
                   "VICReg_representation@eval_payload_only",
                   "eval_payload_only",
                   "false",
                   "true",
                   "0.01",
                   "false");
}

void print_readable_summary(const specs_t& decoded) {
  std::cout << "===== decoded summary (readable) =====\n";
  print_profile_readable(decoded, "VICReg_representation@stable_pretrain");
  std::cout << "\n";
  print_profile_readable(decoded, "VICReg_representation@eval_payload_only");
  std::cout << std::flush;
}

} // namespace

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();

    const std::string contract_hash =
        cuwacunu::iitepi::board_space_t::contract_hash_for_binding(
            cuwacunu::iitepi::config_space_t::locked_board_hash(),
            cuwacunu::iitepi::config_space_t::locked_board_binding_id());
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);

    const std::string instruction = contract_itself->jkimyei.dsl;
    const std::string grammar = contract_itself->jkimyei.grammar;
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
            grammar, instruction);

    validate_decoded(decoded);
    std::cout << "Validation: PASS (decoded values match expected jkimyei.representation.vicreg.dsl values)\n";
    print_readable_summary(decoded);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
