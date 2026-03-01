// test_jk_setup.cpp
#include <cassert>
#include <iostream>

#include <torch/torch.h>

#include "jkimyei/jkimyei.h"
#include "piaabo/dconfig.h"

namespace {

void step_scheduler(cuwacunu::jkimyei::LRSchedulerAny& sched) {
  switch (sched.mode) {
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch:
      sched.step();
      break;
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch:
      sched.step();
      break;
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric:
      sched.step(1.0);
      break;
  }
}

} // namespace

int main() {
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const std::string contract_hash =
      cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();

  const auto& vicreg_setup = cuwacunu::jkimyei::jk_setup("VICReg_representation", contract_hash);

  assert(!vicreg_setup.opt_conf.id.empty());
  assert(!vicreg_setup.sch_conf.id.empty());
  assert(!vicreg_setup.loss_conf.id.empty());

  assert(vicreg_setup.opt_builder != nullptr);
  assert(vicreg_setup.sched_builder != nullptr);

  assert(cuwacunu::jkimyei::api::has_optimizer_type(vicreg_setup.opt_conf.type));
  assert(cuwacunu::jkimyei::api::has_scheduler_type(vicreg_setup.sch_conf.type));
  assert(cuwacunu::jkimyei::api::has_loss_type(vicreg_setup.loss_conf.type));

  torch::nn::Linear tiny(torch::nn::LinearOptions(4, 2));
  auto params = tiny->parameters();
  auto optimizer = vicreg_setup.opt_builder->build(params);
  assert(optimizer != nullptr);

  auto scheduler = vicreg_setup.sched_builder->build(*optimizer);
  assert(scheduler != nullptr);
  step_scheduler(*scheduler);

  cuwacunu::jkimyei::optim::clamp_adam_step(*optimizer, -1);

  const auto& owners = cuwacunu::jkimyei::api::owner_schemas();
  assert(!owners.empty());
  assert(cuwacunu::jkimyei::api::has_owner("reproducibility"));
  assert(cuwacunu::jkimyei::api::has_owner("numerics"));
  assert(cuwacunu::jkimyei::api::has_owner("gradient"));
  assert(cuwacunu::jkimyei::api::has_owner("checkpoint"));
  assert(cuwacunu::jkimyei::api::has_owner("metrics"));
  assert(cuwacunu::jkimyei::api::has_owner("data_ref"));

  log_info("[test_jk_setup] jkimyei batch-1 API scaffold is active\n");
  return 0;
}
