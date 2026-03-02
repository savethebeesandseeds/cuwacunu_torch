/* jk_setup.h */
#pragma once
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/training_setup/jk_losses.h"
#include "jkimyei/training_setup/jk_optimizers.h"
#include "jkimyei/training_setup/jk_lr_schedulers.h"
#include "piaabo/dconfig.h"

namespace cuwacunu {
namespace jkimyei {

/* ------------------------------- config rows ------------------------------- */
struct jk_conf_t {
  std::string id;
  std::string type;
};

inline jk_conf_t ret_conf(
    const cuwacunu::camahjucunu::jkimyei_specs_t& inst,
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t& row,
    const std::string& component)
{
  jk_conf_t ret;
  ret.id = cuwacunu::camahjucunu::require_column(row, component);
  const auto component_row = inst.retrive_row(component + "s_table", ret.id);
  ret.type = cuwacunu::camahjucunu::require_column(component_row, "type");
  return ret;
}

/* ------------------------------ per-component ------------------------------ */
struct jk_component_t {
  std::string name;
  std::string resolved_component_id;
  std::string resolved_profile_id;
  std::string resolved_profile_row_id;
  jk_conf_t opt_conf;
  jk_conf_t loss_conf;
  jk_conf_t sch_conf;
  cuwacunu::camahjucunu::jkimyei_specs_t inst;
  std::unique_ptr<IOptimizerBuilder>  opt_builder;
  std::unique_ptr<ISchedulerBuilder>  sched_builder;

  void build_from(const cuwacunu::camahjucunu::jkimyei_specs_t& instruction,
                  const std::string& component_lookup_name,
                  const std::string& runtime_component_name = {}) {
    const auto trim_ascii_copy = [](std::string s) -> std::string {
      auto is_space = [](char ch) {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
      };
      std::size_t begin = 0;
      while (begin < s.size() && is_space(s[begin])) ++begin;
      std::size_t end = s.size();
      while (end > begin && is_space(s[end - 1])) --end;
      return s.substr(begin, end - begin);
    };
    const auto find_row_by_id_in_table =
        [&](const std::string& table_name,
            const std::string& row_id)
            -> std::optional<cuwacunu::camahjucunu::jkimyei_specs_t::row_t> {
      const auto table_it = instruction.tables.find(table_name);
      if (table_it == instruction.tables.end()) return std::nullopt;
      const std::string target = trim_ascii_copy(row_id);
      for (const auto& candidate : table_it->second) {
        const auto rid_it = candidate.find(ROW_ID_COLUMN_HEADER);
        if (rid_it == candidate.end()) continue;
        if (trim_ascii_copy(rid_it->second) == target) return candidate;
      }
      return std::nullopt;
    };
    const auto resolve_profile_row =
        [&](const std::string& lookup_name)
            -> std::optional<cuwacunu::camahjucunu::jkimyei_specs_t::row_t> {
      if (auto by_id = find_row_by_id_in_table("component_profiles_table", lookup_name);
          by_id.has_value()) {
        return by_id;
      }

      const std::size_t at_pos = lookup_name.rfind('@');
      if (at_pos == std::string::npos || at_pos == 0 || at_pos + 1 >= lookup_name.size()) {
        return std::nullopt;
      }
      const std::string component_hint = trim_ascii_copy(lookup_name.substr(0, at_pos));
      const std::string profile_hint = trim_ascii_copy(lookup_name.substr(at_pos + 1));
      if (component_hint.empty() || profile_hint.empty()) return std::nullopt;

      const auto table_it = instruction.tables.find("component_profiles_table");
      if (table_it == instruction.tables.end()) return std::nullopt;
      const auto& table = table_it->second;
      const cuwacunu::camahjucunu::jkimyei_specs_t::row_t* fallback_profile_match =
          nullptr;
      std::size_t fallback_count = 0;
      for (const auto& candidate : table) {
        const auto profile_it = candidate.find("profile_id");
        if (profile_it == candidate.end()) continue;
        if (trim_ascii_copy(profile_it->second) != profile_hint) continue;
        ++fallback_count;
        const auto component_it = candidate.find("component_id");
        const auto type_it = candidate.find("component_type");
        const std::string candidate_component =
            (component_it == candidate.end()) ? std::string{} : trim_ascii_copy(component_it->second);
        const std::string candidate_type =
            (type_it == candidate.end()) ? std::string{} : trim_ascii_copy(type_it->second);
        if (candidate_component == component_hint || candidate_type == component_hint) {
          return candidate;
        }
        if (!fallback_profile_match) fallback_profile_match = &candidate;
      }
      if (fallback_count == 1 && fallback_profile_match) {
        return *fallback_profile_match;
      }
      return std::nullopt;
    };

    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    bool from_component_profile_row = false;
    if (const auto component_row =
            find_row_by_id_in_table("components_table", component_lookup_name);
        component_row.has_value()) {
      row = *component_row;
    } else {
      const auto profile_row = resolve_profile_row(component_lookup_name);
      if (profile_row.has_value()) {
        row = *profile_row;
      } else {
        row = instruction.retrive_row("component_profiles_table", component_lookup_name);
      }
      from_component_profile_row = true;
    }
    const std::string row_id =
        cuwacunu::camahjucunu::require_column(row, ROW_ID_COLUMN_HEADER);
    (void)cuwacunu::camahjucunu::require_column(row, "optimizer");
    (void)cuwacunu::camahjucunu::require_column(row, "loss_function");
    (void)cuwacunu::camahjucunu::require_column(row, "lr_scheduler");

    if (from_component_profile_row) {
      resolved_profile_row_id = row_id;
      resolved_component_id =
          cuwacunu::camahjucunu::require_column(row, "component_id");
      resolved_profile_id =
          cuwacunu::camahjucunu::require_column(row, "profile_id");
    } else {
      resolved_component_id = row_id;
      resolved_profile_id =
          cuwacunu::camahjucunu::require_column(row, "active_profile");
      resolved_profile_row_id =
          resolved_component_id + "@" + resolved_profile_id;
    }

    name       = runtime_component_name.empty() ? component_lookup_name : runtime_component_name;
    inst       = instruction; // copy
    opt_conf   = ret_conf(inst, row, "optimizer");
    loss_conf  = ret_conf(inst, row, "loss_function");
    sch_conf   = ret_conf(inst, row, "lr_scheduler");

    opt_builder   = make_optimizer_builder(inst, opt_conf.id);
    validate_loss(inst,  loss_conf.id);
    sched_builder = make_scheduler_builder(inst, sch_conf.id);
  }
};

/* ------------------------------ global registry ---------------------------- */
struct jk_setup_t {
  static jk_setup_t registry;

  // Get (or lazily build from CONFIG) a component by name.
  jk_component_t& operator()(
      const std::string& component_name,
      const cuwacunu::iitepi::contract_hash_t& contract_hash);
  // Bind a runtime component to explicit training DSL text (contract-scoped source of truth).
  void set_component_instruction_override(
                                          const cuwacunu::iitepi::contract_hash_t& contract_hash,
                                          std::string runtime_component_name,
                                          std::string component_lookup_name,
                                          std::string instruction_text);
  void clear_component_instruction_override(
      const cuwacunu::iitepi::contract_hash_t& contract_hash,
      const std::string& runtime_component_name);
  void clear_component_instruction_overrides(
      const cuwacunu::iitepi::contract_hash_t& contract_hash);
  void clear_all_component_instruction_overrides();

private:
  struct component_instruction_override_t {
    std::string component_lookup_name{};
    std::string instruction_text{};
  };

  std::unordered_map<std::string, jk_component_t> components;
  std::unordered_map<std::string, component_instruction_override_t> component_instruction_overrides;
  std::mutex mtx;
  static std::string make_component_key(
      const cuwacunu::iitepi::contract_hash_t& contract_hash,
      const std::string& runtime_component_name);

  static void init();
  static void finit();
  struct _init {
    _init()  { jk_setup_t::init(); }
    ~_init() { jk_setup_t::finit(); }
  };
  static _init _initializer;
};

// ---- Convenience free function to call `jk_setup_t("...", contract_hash)` ----
inline jk_component_t& jk_setup(
    const std::string& component_name,
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  return jk_setup_t::registry(component_name, contract_hash);
}

} // namespace jkimyei
} // namespace cuwacunu
