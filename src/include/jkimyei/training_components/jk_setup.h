/* jk_setup */
#pragma once
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

#include "jkimyei/training_components/jk_losses.h"
#include "jkimyei/training_components/jk_optimizers.h"
#include "jkimyei/training_components/jk_lr_schedulers.h"

namespace cuwacunu {
namespace jkimyei {
/* 
 * jk_conf_t 
 */
struct jk_conf_t {
  std::string id;
  std::string type;
};

inline jk_conf_t ret_conf(
    const cuwacunu::camahjucunu::BNF::training_instruction_t& inst, 
    const cuwacunu::camahjucunu::BNF::training_instruction_t::row_t row, 
    const std::string& component) {
  jk_conf_t ret;
  ret.id  = cuwacunu::camahjucunu::require_column(row, component);
  ret.type = cuwacunu::camahjucunu::require_column(inst.retrive_row(component+"s_table", ret.id), "type");
  return ret;
};

/* 
 * jk_setup_t 
 */
struct jk_setup_t {
  std::string name;
  jk_conf_t opt_conf;
  jk_conf_t loss_conf;
  jk_conf_t sch_conf;
  cuwacunu::camahjucunu::BNF::training_instruction_t inst;
  std::unique_ptr<IOptimizerBuilder> opt_builder;
  std::unique_ptr<ISchedulerBuilder> sched_builder;
};

inline jk_setup_t build_training_setup_component(
    const cuwacunu::camahjucunu::BNF::training_instruction_t& inst, 
    const std::string& component_name) {
  /* validate*/
  const auto& row = inst.retrive_row("components_table", component_name);
  cuwacunu::camahjucunu::require_columns_exact(row, {ROW_ID_COLUMN_HEADER,"optimizer","loss_function","lr_scheduler"});
  
  /* construct */
  jk_setup_t c;
  c.name          = component_name;
  c.inst          = std::move(inst);
  c.opt_conf      = ret_conf(c.inst, row, "optimizer");
  c.loss_conf     = ret_conf(c.inst, row, "loss_function");
  c.sch_conf      = ret_conf(c.inst, row, "lr_scheduler");
  c.opt_builder   = make_optimizer_builder(c.inst,  c.opt_conf.id);
  /* validate losses*/       validate_loss(c.inst, c.loss_conf.id);
  c.sched_builder = make_scheduler_builder(c.inst,  c.sch_conf.id);
  
  return c;
}

} // namespace jkimyei
} // namespace cuwacunu
