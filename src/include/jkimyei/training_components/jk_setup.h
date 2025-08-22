/* jk_setup */
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

#include "jkimyei/training_components/jk_losses.h"
#include "jkimyei/training_components/jk_optimizers.h"
#include "jkimyei/training_components/jk_lr_schedulers.h"

namespace cuwacunu {
namespace jkimyei {

struct jk_setup_t {
  std::string name;
  std::unique_ptr<ILoss> loss;
  std::unique_ptr<IOptimizerBuilder> opt_builder;
  std::unique_ptr<ISchedulerBuilder> sched_builder;
};

inline jk_setup_t build_training_setup_component(
  const cuwacunu::camahjucunu::BNF::training_instruction_t& inst, const std::string& component_name) {
  const auto& row = inst.retrive_row("components_table", component_name);
  
  cuwacunu::camahjucunu::require_columns_exact(row, {ROW_ID_COLUMN_HEADER,"optimizer","loss_function","lr_scheduler"});
  const auto& opt_id  = cuwacunu::camahjucunu::require_column(row, "optimizer");
  const auto& loss_id = cuwacunu::camahjucunu::require_column(row, "loss_function");
  const auto& sch_id  = cuwacunu::camahjucunu::require_column(row, "lr_scheduler");

  jk_setup_t c;
  c.name = component_name;
  c.loss          = make_loss(inst, loss_id);
  c.opt_builder   = make_optimizer_builder(inst, opt_id);
  c.sched_builder = make_scheduler_builder(inst, sch_id);
  
  return c;
}

} // namespace jkimyei
} // namespace cuwacunu
