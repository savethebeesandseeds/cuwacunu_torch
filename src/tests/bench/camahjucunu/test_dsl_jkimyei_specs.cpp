// test_dsl_jkimyei_specs.cpp
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/specs/jkimyei_schema.h"

namespace {

using value_kind_e = cuwacunu::jkimyei::specs::value_kind_e;
using component_type_e = cuwacunu::jkimyei::specs::component_type_e;
using specs_t = cuwacunu::camahjucunu::jkimyei_specs_t;
using row_t = specs_t::row_t;
using table_t = specs_t::table_t;

struct owner_schema_t {
  std::unordered_map<std::string, value_kind_e> key_kind{};
  std::unordered_set<std::string> required_keys{};
};

struct component_schema_t {
  std::string canonical_type{};
  std::string kind_token{};
  std::vector<cuwacunu::jkimyei::specs::family_rule_t> family_rules{};
};

struct schema_index_t {
  std::unordered_map<std::string, owner_schema_t> owners{};
  std::unordered_map<std::string, component_schema_t> components{};
  std::unordered_set<std::string> selector_fields{};
};

template <typename T>
std::vector<T> sorted_vector(std::vector<T> in) {
  std::sort(in.begin(), in.end());
  return in;
}

std::string lower_ascii_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string trim_ascii_copy(const std::string& s) {
  std::size_t begin = 0;
  while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
  std::size_t end = s.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
  return s.substr(begin, end - begin);
}

std::vector<std::string> split_csv(const std::string& raw) {
  std::vector<std::string> out;
  if (raw.empty()) return out;
  std::size_t cursor = 0;
  while (cursor <= raw.size()) {
    const std::size_t comma = raw.find(',', cursor);
    std::string item = (comma == std::string::npos) ? raw.substr(cursor)
                                                     : raw.substr(cursor, comma - cursor);
    item = trim_ascii_copy(item);
    if (item.empty()) return {};
    out.push_back(std::move(item));
    if (comma == std::string::npos) break;
    cursor = comma + 1;
  }
  return out;
}

bool try_parse_int64(const std::string& raw, long long* out) {
  if (!out) return false;
  const std::string s = trim_ascii_copy(raw);
  if (s.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(s.c_str(), &end, 10);
  if (errno != 0 || end == s.c_str() || *end != '\0') return false;
  *out = v;
  return true;
}

bool try_parse_f64(const std::string& raw, double* out) {
  if (!out) return false;
  const std::string s = trim_ascii_copy(raw);
  if (s.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const double v = std::strtod(s.c_str(), &end);
  if (errno != 0 || end == s.c_str() || *end != '\0') return false;
  if (!std::isfinite(v)) return false;
  *out = v;
  return true;
}

bool value_matches_kind(value_kind_e kind, const std::string& raw) {
  switch (kind) {
    case value_kind_e::Bool: {
      const std::string v = lower_ascii_copy(trim_ascii_copy(raw));
      return v == "true" || v == "false";
    }
    case value_kind_e::Int: {
      long long tmp = 0;
      return try_parse_int64(raw, &tmp);
    }
    case value_kind_e::Float: {
      double tmp = 0.0;
      return try_parse_f64(raw, &tmp);
    }
    case value_kind_e::String:
      return true;
    case value_kind_e::IntList: {
      const auto items = split_csv(raw);
      if (items.empty()) return false;
      for (const auto& item : items) {
        long long tmp = 0;
        if (!try_parse_int64(item, &tmp)) return false;
      }
      return true;
    }
    case value_kind_e::FloatList: {
      const auto items = split_csv(raw);
      if (items.empty()) return false;
      for (const auto& item : items) {
        double tmp = 0.0;
        if (!try_parse_f64(item, &tmp)) return false;
      }
      return true;
    }
    case value_kind_e::StringList: {
      const auto items = split_csv(raw);
      return !items.empty();
    }
  }
  return false;
}

const char* component_kind_token(component_type_e type) {
  switch (type) {
#define JK_COMPONENT_TYPE(kind_token, canonical_type, description) \
  case component_type_e::kind_token: return #kind_token;
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_TYPE
  }
  return "UNKNOWN";
}

schema_index_t build_schema_index() {
  schema_index_t out{};

  for (const auto& d : cuwacunu::jkimyei::specs::kTypedParams) {
    owner_schema_t& owner = out.owners[std::string(d.owner)];
    owner.key_kind[std::string(d.key)] = d.kind;
    if (d.required) owner.required_keys.insert(std::string(d.key));
  }

  for (const auto& comp : cuwacunu::jkimyei::specs::kComponents) {
    component_schema_t schema{};
    schema.canonical_type = std::string(comp.canonical_type);
    schema.kind_token = std::string(component_kind_token(comp.type));
    out.components[schema.canonical_type] = std::move(schema);
  }

  for (const auto& rule : cuwacunu::jkimyei::specs::kFamilyRules) {
    for (const auto& comp : cuwacunu::jkimyei::specs::kComponents) {
      if (comp.type == rule.type) {
        out.components[std::string(comp.canonical_type)].family_rules.push_back(rule);
        break;
      }
    }
  }

  for (const auto& field : cuwacunu::jkimyei::specs::kIniSelectorFields) {
    out.selector_fields.insert(std::string(field.key));
  }

  return out;
}

bool family_forbidden(const component_schema_t& schema, const std::string& family) {
  for (const auto& rule : schema.family_rules) {
    if (rule.family == family) return !rule.required;
  }
  return false;
}

const row_t* find_row_by_id(const table_t& table, const std::string& row_id) {
  for (const auto& row : table) {
    auto it = row.find(ROW_ID_COLUMN_HEADER);
    if (it != row.end() && it->second == row_id) return &row;
  }
  return nullptr;
}

std::vector<const row_t*> find_rows_by_field(const table_t& table,
                                             const std::string& key,
                                             const std::string& value) {
  std::vector<const row_t*> out;
  for (const auto& row : table) {
    auto it = row.find(key);
    if (it != row.end() && it->second == value) out.push_back(&row);
  }
  return out;
}

const std::string& require_field(const row_t& row, const std::string& key) {
  auto it = row.find(key);
  assert(it != row.end());
  return it->second;
}

std::unordered_map<std::string, std::string> payload_without_keys(
    const row_t& row,
    const std::unordered_set<std::string>& skip) {
  std::unordered_map<std::string, std::string> out;
  for (const auto& [k, v] : row) {
    if (skip.find(k) == skip.end()) out[k] = v;
  }
  return out;
}

void validate_owner_payload(const schema_index_t& schema,
                            const std::string& owner,
                            const std::unordered_map<std::string, std::string>& payload) {
  auto owner_it = schema.owners.find(owner);
  assert(owner_it != schema.owners.end());
  const owner_schema_t& owner_schema = owner_it->second;

  for (const auto& [key, value] : payload) {
    auto kind_it = owner_schema.key_kind.find(key);
    assert(kind_it != owner_schema.key_kind.end());
    assert(value_matches_kind(kind_it->second, value));
  }
  for (const auto& req_key : owner_schema.required_keys) {
    assert(payload.find(req_key) != payload.end());
  }
}

void validate_options_row(const schema_index_t& schema,
                          const std::string& owner,
                          const row_t& row) {
  auto it = row.find("options");
  assert(it != row.end());
  const auto options = cuwacunu::camahjucunu::parse_options_kvlist(it->second);
  validate_owner_payload(schema, owner, options);
}

void validate_decoded_against_schema(const specs_t& decoded,
                                     const schema_index_t& schema,
                                     std::unordered_set<std::string>* used_owners_out) {
  const auto table_it = [&](const std::string& name) -> const table_t& {
    auto it = decoded.tables.find(name);
    assert(it != decoded.tables.end());
    return it->second;
  };

  const table_t& selectors = table_it("selectors_table");
  const table_t& components = table_it("components_table");
  const table_t& profiles = table_it("component_profiles_table");
  const table_t& optimizers = table_it("optimizers_table");
  const table_t& schedulers = table_it("lr_schedulers_table");
  const table_t& losses = table_it("loss_functions_table");
  const table_t& reproducibility = table_it("component_reproducibility_table");
  const table_t& numerics = table_it("component_numerics_table");
  const table_t& gradient = table_it("component_gradient_table");
  const table_t& checkpoint = table_it("component_checkpoint_table");
  const table_t& metrics = table_it("component_metrics_table");
  const table_t& data_ref = table_it("component_data_ref_table");
  const table_t& augmentations = table_it("vicreg_augmentations");

  std::unordered_set<std::string> used_owners;

  assert(!selectors.empty());
  const row_t* selector_row = find_row_by_id(selectors, "selectors");
  assert(selector_row != nullptr);
  for (const auto& [k, v] : *selector_row) {
    if (k == ROW_ID_COLUMN_HEADER) continue;
    assert(k == "COMPONENT_ID_KEY" || k == "PROFILE_ID_KEY");
    assert(schema.selector_fields.find(v) != schema.selector_fields.end());
  }

  for (const auto& component_row : components) {
    const std::string component_id = require_field(component_row, ROW_ID_COLUMN_HEADER);
    const std::string canonical_type = require_field(component_row, "component_type");
    const std::string active_profile = require_field(component_row, "active_profile");
    const std::string optimizer_id = require_field(component_row, "optimizer");
    const std::string scheduler_id = require_field(component_row, "lr_scheduler");
    const std::string loss_id = require_field(component_row, "loss_function");

    auto comp_schema_it = schema.components.find(canonical_type);
    assert(comp_schema_it != schema.components.end());
    const component_schema_t& component_schema = comp_schema_it->second;

    const std::string component_owner = "component." + component_schema.kind_token;
    auto component_payload =
        payload_without_keys(component_row,
                             {ROW_ID_COLUMN_HEADER,
                              "component_type",
                              "active_profile",
                              "optimizer",
                              "lr_scheduler",
                              "loss_function"});
    validate_owner_payload(schema, component_owner, component_payload);
    used_owners.insert(component_owner);

    const row_t* optimizer_row = find_row_by_id(optimizers, optimizer_id);
    const row_t* scheduler_row = find_row_by_id(schedulers, scheduler_id);
    const row_t* loss_row = find_row_by_id(losses, loss_id);
    assert(optimizer_row != nullptr);
    assert(scheduler_row != nullptr);
    assert(loss_row != nullptr);

    const std::string optimizer_owner = "optimizer." + require_field(*optimizer_row, "type");
    const std::string scheduler_owner = "scheduler." + require_field(*scheduler_row, "type");
    const std::string loss_owner = "loss." + require_field(*loss_row, "type");
    validate_options_row(schema, optimizer_owner, *optimizer_row);
    validate_options_row(schema, scheduler_owner, *scheduler_row);
    validate_options_row(schema, loss_owner, *loss_row);
    used_owners.insert(optimizer_owner);
    used_owners.insert(scheduler_owner);
    used_owners.insert(loss_owner);

    const auto profile_rows = find_rows_by_field(profiles, "component_id", component_id);
    assert(!profile_rows.empty());
    std::size_t active_count = 0;
    for (const row_t* profile_row : profile_rows) {
      const std::string profile_row_id = require_field(*profile_row, ROW_ID_COLUMN_HEADER);
      const std::string profile_name = require_field(*profile_row, "profile_id");
      const std::string active_token = require_field(*profile_row, "active");
      assert(active_token == "true" || active_token == "false");
      const bool is_active = (active_token == "true");
      if (is_active) ++active_count;
      if (profile_name == active_profile) assert(is_active);

      const std::string opt_ref = require_field(*profile_row, "optimizer");
      const std::string sch_ref = require_field(*profile_row, "lr_scheduler");
      const std::string loss_ref = require_field(*profile_row, "loss_function");
      const row_t* prow_opt = find_row_by_id(optimizers, opt_ref);
      const row_t* prow_sch = find_row_by_id(schedulers, sch_ref);
      const row_t* prow_loss = find_row_by_id(losses, loss_ref);
      assert(prow_opt != nullptr);
      assert(prow_sch != nullptr);
      assert(prow_loss != nullptr);
      const std::string prow_opt_owner = "optimizer." + require_field(*prow_opt, "type");
      const std::string prow_sch_owner = "scheduler." + require_field(*prow_sch, "type");
      const std::string prow_loss_owner = "loss." + require_field(*prow_loss, "type");
      validate_options_row(schema, prow_opt_owner, *prow_opt);
      validate_options_row(schema, prow_sch_owner, *prow_sch);
      validate_options_row(schema, prow_loss_owner, *prow_loss);
      used_owners.insert(prow_opt_owner);
      used_owners.insert(prow_sch_owner);
      used_owners.insert(prow_loss_owner);

      if (!family_forbidden(component_schema, "Reproducibility")) {
        const row_t* r = find_row_by_id(reproducibility, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "reproducibility", payload);
        used_owners.insert("reproducibility");
      }
      if (!family_forbidden(component_schema, "Numerics")) {
        const row_t* r = find_row_by_id(numerics, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "numerics", payload);
        used_owners.insert("numerics");
      }
      if (!family_forbidden(component_schema, "Gradient")) {
        const row_t* r = find_row_by_id(gradient, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "gradient", payload);
        used_owners.insert("gradient");
      }
      if (!family_forbidden(component_schema, "Checkpoint")) {
        const row_t* r = find_row_by_id(checkpoint, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "checkpoint", payload);
        used_owners.insert("checkpoint");
      }
      if (!family_forbidden(component_schema, "Metrics")) {
        const row_t* r = find_row_by_id(metrics, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "metrics", payload);
        used_owners.insert("metrics");
      }
      if (!family_forbidden(component_schema, "DataRef")) {
        const row_t* r = find_row_by_id(data_ref, profile_row_id);
        assert(r != nullptr);
        auto payload = payload_without_keys(*r, {ROW_ID_COLUMN_HEADER, "component_id"});
        validate_owner_payload(schema, "data_ref", payload);
        used_owners.insert("data_ref");
      }
    }
    assert(active_count == 1);

    if (!family_forbidden(component_schema, "Augmentations")) {
      auto it = component_payload.find("augmentation_set");
      if (it != component_payload.end()) {
        const auto aug_rows = find_rows_by_field(augmentations, "augmentation_set", it->second);
        assert(!aug_rows.empty());
      }
    } else {
      assert(component_payload.find("augmentation_set") == component_payload.end());
    }
  }

  for (const auto& row : augmentations) {
    auto payload = payload_without_keys(row, {ROW_ID_COLUMN_HEADER, "augmentation_set", "curve"});
    if (payload.find("kind") == payload.end()) {
      auto curve_it = row.find("curve");
      assert(curve_it != row.end());
      payload["kind"] = curve_it->second;
    }
    validate_owner_payload(schema, "augmentation.curve", payload);
    used_owners.insert("augmentation.curve");
  }

  if (used_owners_out) *used_owners_out = std::move(used_owners);
}

std::string scalar_for_kind(value_kind_e kind, const std::string& key) {
  switch (kind) {
    case value_kind_e::Bool: return "true";
    case value_kind_e::Int: return "1";
    case value_kind_e::Float: return "0.1";
    case value_kind_e::String: return "\"" + key + "_value\"";
    case value_kind_e::IntList: return "[1,2]";
    case value_kind_e::FloatList: return "[0.1,0.2]";
    case value_kind_e::StringList: return "[\"a\",\"b\"]";
  }
  return "\"unsupported\"";
}

void emit_owner_block(std::ostringstream& oss,
                      const owner_schema_t& owner,
                      const std::string& indent,
                      const std::unordered_map<std::string, std::string>& overrides = {}) {
  std::vector<std::string> keys;
  keys.reserve(owner.key_kind.size());
  for (const auto& kv : owner.key_kind) keys.push_back(kv.first);
  keys = sorted_vector(std::move(keys));
  for (const auto& key : keys) {
    auto ov = overrides.find(key);
    if (ov != overrides.end()) {
      oss << indent << key << ": " << ov->second << "\n";
    } else {
      oss << indent << key << ": " << scalar_for_kind(owner.key_kind.at(key), key) << "\n";
    }
  }
}

std::vector<std::string> collect_owner_suffixes(const schema_index_t& schema,
                                                const std::string& prefix) {
  std::vector<std::string> out;
  for (const auto& [owner, _] : schema.owners) {
    if (owner.rfind(prefix, 0) == 0) out.push_back(owner.substr(prefix.size()));
  }
  return sorted_vector(std::move(out));
}

std::string sanitize_token(std::string in) {
  for (char& c : in) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (!(std::isalnum(uc) || c == '_')) c = '_';
  }
  return in;
}

bool replace_first(std::string* text, const std::string& from, const std::string& to) {
  if (!text) return false;
  std::size_t pos = text->find(from);
  if (pos == std::string::npos) return false;
  text->replace(pos, from.size(), to);
  return true;
}

void emit_profile(std::ostringstream& oss,
                  const schema_index_t& schema,
                  const component_schema_t& component,
                  const std::string& profile_name,
                  const std::string& optimizer_type,
                  const std::string& scheduler_type,
                  const std::string& loss_type,
                  const std::string& augmentation_set_name) {
  oss << "  PROFILE \"" << profile_name << "\" {\n";

  if (!family_forbidden(component, "Optimizer")) {
    const std::string owner_name = "optimizer." + optimizer_type;
    auto it = schema.owners.find(owner_name);
    assert(it != schema.owners.end());
    oss << "    OPTIMIZER \"" << optimizer_type << "\" {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Scheduler")) {
    const std::string owner_name = "scheduler." + scheduler_type;
    auto it = schema.owners.find(owner_name);
    assert(it != schema.owners.end());
    oss << "    LR_SCHEDULER \"" << scheduler_type << "\" {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Loss")) {
    const std::string owner_name = "loss." + loss_type;
    auto it = schema.owners.find(owner_name);
    assert(it != schema.owners.end());
    oss << "    LOSS \"" << loss_type << "\" {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "ComponentParams")) {
    const std::string owner_name = "component." + component.kind_token;
    auto it = schema.owners.find(owner_name);
    assert(it != schema.owners.end());
    std::unordered_map<std::string, std::string> overrides;
    if (it->second.key_kind.find("augmentation_set") != it->second.key_kind.end()) {
      overrides["augmentation_set"] = "\"" + augmentation_set_name + "\"";
    }
    oss << "    COMPONENT_PARAMS {\n";
    emit_owner_block(oss, it->second, "      ", overrides);
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Reproducibility")) {
    auto it = schema.owners.find("reproducibility");
    assert(it != schema.owners.end());
    oss << "    REPRODUCIBILITY {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Numerics")) {
    auto it = schema.owners.find("numerics");
    assert(it != schema.owners.end());
    oss << "    NUMERICS {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Gradient")) {
    auto it = schema.owners.find("gradient");
    assert(it != schema.owners.end());
    oss << "    GRADIENT {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Checkpoint")) {
    auto it = schema.owners.find("checkpoint");
    assert(it != schema.owners.end());
    oss << "    CHECKPOINT {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "Metrics")) {
    auto it = schema.owners.find("metrics");
    assert(it != schema.owners.end());
    oss << "    METRICS {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n\n";
  }

  if (!family_forbidden(component, "DataRef")) {
    auto it = schema.owners.find("data_ref");
    assert(it != schema.owners.end());
    oss << "    DATA_REF {\n";
    emit_owner_block(oss, it->second, "      ");
    oss << "    }\n";
  }

  oss << "  }\n\n";
}

std::string build_full_coverage_dsl(const schema_index_t& schema) {
  const auto optimizer_types = collect_owner_suffixes(schema, "optimizer.");
  const auto scheduler_types = collect_owner_suffixes(schema, "scheduler.");
  const auto loss_types = collect_owner_suffixes(schema, "loss.");
  assert(!optimizer_types.empty());
  assert(!scheduler_types.empty());
  assert(!loss_types.empty());

  std::vector<std::string> component_types;
  component_types.reserve(schema.components.size());
  for (const auto& [canonical, _] : schema.components) component_types.push_back(canonical);
  component_types = sorted_vector(std::move(component_types));
  assert(!component_types.empty());

  std::string coverage_component = component_types.front();
  for (const auto& canonical : component_types) {
    const component_schema_t& comp = schema.components.at(canonical);
    if (!family_forbidden(comp, "Optimizer") && !family_forbidden(comp, "Scheduler") &&
        !family_forbidden(comp, "Loss")) {
      coverage_component = canonical;
      break;
    }
  }

  std::vector<std::string> selectors(schema.selector_fields.begin(), schema.selector_fields.end());
  selectors = sorted_vector(std::move(selectors));
  assert(!selectors.empty());
  const std::string selector_component =
      (schema.selector_fields.find("jkimyei_component_id") != schema.selector_fields.end())
          ? "jkimyei_component_id"
          : selectors.front();
  std::string selector_profile =
      (schema.selector_fields.find("jkimyei_profile_id") != schema.selector_fields.end())
          ? "jkimyei_profile_id"
          : selectors.front();
  if (selector_profile == selector_component && selectors.size() > 1) {
    selector_profile = selectors[1];
  }

  std::ostringstream oss;
  oss << "JKSPEC 2.0\n\n";
  oss << "SELECTORS {\n";
  oss << "  COMPONENT_ID_KEY: \"" << selector_component << "\"\n";
  oss << "  PROFILE_ID_KEY: \"" << selector_profile << "\"\n";
  oss << "}\n\n";

  const std::string default_opt = optimizer_types.front();
  const std::string default_sch = scheduler_types.front();
  const std::string default_loss = loss_types.front();

  for (const auto& canonical : component_types) {
    const component_schema_t& component = schema.components.at(canonical);
    const std::string component_id = "COMP_" + component.kind_token;
    const std::string augmentation_set_name = "aug_" + lower_ascii_copy(component.kind_token);

    std::vector<std::tuple<std::string, std::string, std::string, std::string>> profiles;
    if (canonical == coverage_component) {
      for (const auto& opt : optimizer_types) {
        profiles.emplace_back("opt_" + sanitize_token(opt), opt, default_sch, default_loss);
      }
      for (const auto& sch : scheduler_types) {
        profiles.emplace_back("sch_" + sanitize_token(sch), default_opt, sch, default_loss);
      }
      for (const auto& loss : loss_types) {
        profiles.emplace_back("loss_" + sanitize_token(loss), default_opt, default_sch, loss);
      }
    } else {
      profiles.emplace_back("baseline", default_opt, default_sch, default_loss);
    }
    assert(!profiles.empty());

    oss << "COMPONENT \"" << canonical << "\" \"" << component_id << "\" {\n\n";
    for (const auto& profile : profiles) {
      emit_profile(oss,
                   schema,
                   component,
                   std::get<0>(profile),
                   std::get<1>(profile),
                   std::get<2>(profile),
                   std::get<3>(profile),
                   augmentation_set_name);
    }

    if (!family_forbidden(component, "Augmentations")) {
      auto it = schema.owners.find("augmentation.curve");
      assert(it != schema.owners.end());
      oss << "  AUGMENTATIONS \"" << augmentation_set_name << "\" {\n";
      oss << "    CURVE \"Linear\" {\n";
      emit_owner_block(oss, it->second, "      ", {{"kind", "\"Linear\""}});
      oss << "    }\n";
      oss << "  }\n\n";
    }

    oss << "  ACTIVE_PROFILE: \"" << std::get<0>(profiles.front()) << "\"\n";
    oss << "}\n\n";
  }

  return oss.str();
}

void expect_decode_failure(const std::string& grammar, const std::string& instruction) {
  bool failed = false;
  try {
    auto decoded = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(grammar, instruction);
    (void)decoded;
  } catch (const std::exception&) {
    failed = true;
  }
  assert(failed);
}

} // namespace

int main() {
  const schema_index_t schema = build_schema_index();

  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const std::string contract_hash =
      cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();

  const std::string instruction =
      cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_dsl(contract_hash);
  const std::string grammar =
      cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_grammar(contract_hash);

  TICK(decode_contract_jkimyei_specs);
  const auto decoded_contract =
      cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(grammar, instruction);
  PRINT_TOCK_ns(decode_contract_jkimyei_specs);
  validate_decoded_against_schema(decoded_contract, schema, nullptr);

  const std::string full_coverage_instruction = build_full_coverage_dsl(schema);
  TICK(decode_generated_coverage_jkimyei_specs);
  const auto decoded_generated =
      cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(grammar, full_coverage_instruction);
  PRINT_TOCK_ns(decode_generated_coverage_jkimyei_specs);

  std::unordered_set<std::string> used_owners;
  validate_decoded_against_schema(decoded_generated, schema, &used_owners);
  for (const auto& [owner, _] : schema.owners) {
    assert(used_owners.find(owner) != used_owners.end());
  }

  const auto& generated_components = decoded_generated.tables.at("components_table");
  std::unordered_set<std::string> covered_canonical_types;
  for (const auto& row : generated_components) {
    covered_canonical_types.insert(row.at("component_type"));
  }
  for (const auto& [canonical, _] : schema.components) {
    assert(covered_canonical_types.find(canonical) != covered_canonical_types.end());
  }

  {
    std::string invalid = full_coverage_instruction;
    assert(replace_first(&invalid, "initial_learning_rate:", "unknown_optimizer_key:"));
    expect_decode_failure(grammar, invalid);
  }

  {
    std::string invalid = full_coverage_instruction;
    assert(replace_first(&invalid, "workers: 1", "workers: nope"));
    expect_decode_failure(grammar, invalid);
  }

  {
    std::string invalid = full_coverage_instruction;
    assert(replace_first(&invalid, "COMPONENT_ID_KEY", "BAD_SELECTOR_KEY"));
    expect_decode_failure(grammar, invalid);
  }

  {
    std::string invalid = full_coverage_instruction;
    assert(replace_first(&invalid, "weight_decay: 0.1", "weight_decay: 0.1\n      weight_decay: 0.2"));
    expect_decode_failure(grammar, invalid);
  }

  {
    std::string invalid = full_coverage_instruction;
    const std::string mdn_header = "COMPONENT \"tsi.wikimyei.inference.mdn\"";
    const std::size_t mdn_pos = invalid.find(mdn_header);
    if (mdn_pos != std::string::npos) {
      const std::size_t active_pos = invalid.find("\n  ACTIVE_PROFILE:", mdn_pos);
      assert(active_pos != std::string::npos);
      invalid.insert(
          active_pos,
          "\n  AUGMENTATIONS \"forbidden_mdn_aug\" {\n"
          "    CURVE \"Linear\" {\n"
          "      kind: \"Linear\"\n"
          "      curve_param: 0.1\n"
          "      noise_scale: 0.1\n"
          "      smoothing_kernel_size: 1\n"
          "      point_drop_prob: 0.1\n"
          "      value_jitter_std: 0.1\n"
          "      time_mask_band_frac: 0.1\n"
          "      channel_dropout_prob: 0.1\n"
          "    }\n"
          "  }\n");
      expect_decode_failure(grammar, invalid);
    }
  }

  log_info(
      "[test_dsl_jkimyei_specs] schema translation coverage passed for %zu owners and %zu components\n",
      schema.owners.size(),
      schema.components.size());
  return 0;
}
