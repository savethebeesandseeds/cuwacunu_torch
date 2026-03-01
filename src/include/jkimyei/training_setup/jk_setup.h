/* jk_setup.h */
#pragma once
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

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
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    bool from_component_profile_row = false;
    try {
      row = instruction.retrive_row("components_table", component_lookup_name);
    } catch (const std::exception&) {
      row = instruction.retrive_row("component_profiles_table", component_lookup_name);
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
