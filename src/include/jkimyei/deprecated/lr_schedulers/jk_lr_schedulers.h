/* jk_lr_schedulers.h */
#pragma once

#include <torch/torch.h> 
#include <functional>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

#define RL_SCHEDULERS_TABLE_NAME "lr_schedulers_table"

/* lr_schedulers.h */
namespace cuwacunu {
namespace jkimyei {

struct jk_lr_schedulers {
public:
  static cuwacunu::camahjucunu::BNF::trainingPipeline training_components;
  static cuwacunu::camahjucunu::training_instruction_t training_instruction;

private:
  jk_lr_schedulers();
  ~jk_lr_schedulers();

  /* Enforce Singleton requirements */
  jk_lr_schedulers(const jk_lr_schedulers&) = delete;
  jk_lr_schedulers& operator=(const jk_lr_schedulers&) = delete;
  jk_lr_schedulers(jk_lr_schedulers&&) = delete;
  jk_lr_schedulers& operator=(jk_lr_schedulers&&) = delete;

public:
  /* Access point Constructor */
  static struct _init { public: _init(){ jk_lr_schedulers::init(); } } _initializer;

  static void init() {
    log_info("Initializing jk_lr_schedulers\n");
    std::atexit(jk_lr_schedulers::finit);

    training_components = cuwacunu::camahjucunu::BNF::trainingPipeline();

    /* Configure to the default configuration */
    jk_lr_schedulers::configure(
      cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction()
    );
  }

  static void finit() {
    log_info("Finalizing jk_lr_schedulers \n");
  }

  static void configure(const std::string &instruction) {
    training_instruction = training_components.decode(instruction);
  }

/* --- --- --- --- --- --- --- --- */
/* -- Learning Rate Schedulers --- */
/* --- --- --- --- --- --- --- --- */

  /* StepLR */
  static std::unique_ptr<torch::optim::LRScheduler> StepLR(
    torch::optim::Optimizer& optimizer, 
    std::string& row_id
  ) {
    auto __ = trainingPipe_ConfAccess(RL_SCHEDULERS_TABLE_NAME, row_id);
    auto options = torch::optim::StepLROptions(
      __<int>("step_size"))
      .gamma(__<double>("gamma"));

    return std::make_unique<torch::optim::StepLR>(optimizer, options);
  }

  /* MultiStepLR */
  static std::unique_ptr<torch::optim::LRScheduler> MultiStepLR(
    torch::optim::Optimizer& optimizer, 
    std::string& row_id
  ) {
    auto __ = trainingPipe_ConfAccess(RL_SCHEDULERS_TABLE_NAME, row_id);
    auto options = torch::optim::MultiStepLROptions(
      __<std::vector<int>>("milestones"))
      .gamma(__<double>("gamma"));

    return std::make_unique<torch::optim::MultiStepLR>(optimizer, options);
  }

  /* ExponentialLR */
  static std::unique_ptr<torch::optim::LRScheduler> ExponentialLR(
    torch::optim::Optimizer& optimizer,
    std::string& row_id
  ) {
    auto __ = trainingPipe_ConfAccess(RL_SCHEDULERS_TABLE_NAME, row_id);
    auto options = torch::optim::ExponentialLROptions(
      __<double>("gamma"));

    return std::make_unique<torch::optim::ExponentialLR>(optimizer, options);
  }

  /* ReduceLROnPlateau */
  static std::unique_ptr<torch::optim::LRScheduler> ReduceLROnPlateau(
    torch::optim::Optimizer& optimizer,
    std::string& row_id
  ) {
    auto __ = trainingPipe_ConfAccess(RL_SCHEDULERS_TABLE_NAME, row_id);
    auto options = torch::optim::ReduceLROnPlateauOptions()
      .mode          (__<std::string>("mode"))
      .factor        (__<double>("factor"))
      .patience      (__<int>("patience"))
      .threshold     (__<double>("threshold"))
      .threshold_mode(__<std::string>("threshold_mode"))
      .cooldown      (__<int>("cooldown"))
      .min_lr        (__<double>("min_lr"))
      .eps           (__<double>("eps"));

    return std::make_unique<torch::optim::ReduceLROnPlateau>(optimizer, options);
  }
};

/* initialize static members */
cuwacunu::camahjucunu::BNF::trainingPipeline jk_lr_schedulers::training_components;
cuwacunu::camahjucunu::training_instruction_t jk_lr_schedulers::training_instruction;

/* initialize static class */
jk_lr_schedulers::_init jk_lr_schedulers::_initializer;

} /* namespace cuwacunu */
} /* namespace jkimyei */
