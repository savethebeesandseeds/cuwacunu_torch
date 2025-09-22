/* jk_setup.h */
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"
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
    const cuwacunu::camahjucunu::training_instruction_t& inst,
    const cuwacunu::camahjucunu::training_instruction_t::row_t row,
    const std::string& component)
{
  jk_conf_t ret;
  ret.id   = cuwacunu::camahjucunu::require_column(row, component);
  ret.type = cuwacunu::camahjucunu::require_column(
               inst.retrive_row(component + "s_table", ret.id), "type");
  return ret;
}

/* ------------------------------ per-component ------------------------------ */
struct jk_component_t {
  std::string name;
  jk_conf_t opt_conf;
  jk_conf_t loss_conf;
  jk_conf_t sch_conf;
  cuwacunu::camahjucunu::training_instruction_t inst;
  std::unique_ptr<IOptimizerBuilder>  opt_builder;
  std::unique_ptr<ISchedulerBuilder>  sched_builder;

  void build_from(const cuwacunu::camahjucunu::training_instruction_t& instruction,
                  const std::string& component_name) {
    const auto& row = instruction.retrive_row("components_table", component_name);
    cuwacunu::camahjucunu::require_columns_exact(
        row, { ROW_ID_COLUMN_HEADER, "optimizer", "loss_function", "lr_scheduler" });

    name       = component_name;
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
  jk_component_t& operator()(const std::string& component_name);

private:
  std::unordered_map<std::string, jk_component_t> components;
  std::mutex mtx;

  static void init();
  static void finit();
  struct _init {
    _init()  { jk_setup_t::init(); }
    ~_init() { jk_setup_t::finit(); }
  };
  static _init _initializer;
};

// ---- Convenience free function to call `jk_setup_t("...")` ----
inline jk_component_t& jk_setup(const std::string& component_name) {
  return jk_setup_t::registry(component_name);
}

} // namespace jkimyei
} // namespace cuwacunu
