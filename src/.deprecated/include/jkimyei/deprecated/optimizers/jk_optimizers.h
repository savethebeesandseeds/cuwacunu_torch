/* jk_optimizers.h */
#pragma once

#include <torch/torch.h>
#include <functional>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"

#define OPTIMZIERS_TABLE_NAME "optimizers_table"

/* optimizers.h */
namespace cuwacunu {
namespace jkimyei {

struct jk_optimizers {
public:
  static cuwacunu::camahjucunu::dsl::jkimyeiSpecsPipeline jkimyei_specs_pipeline;
  static cuwacunu::camahjucunu::jkimyei_specs_t jkimyei_specs_instruction;
private:
  jk_optimizers();
  ~jk_optimizers();

  /* Enforce Singleton requirements */
  jk_optimizers(const jk_optimizers&) = delete;
  jk_optimizers& operator=(const jk_optimizers&) = delete;
  jk_optimizers(jk_optimizers&&) = delete;
  jk_optimizers& operator=(jk_optimizers&&) = delete;

public:
  /* Accespoint Constructor */
  static struct _init {public:_init(){jk_optimizers::init();}}_initializer;
  static void init() {
    log_info("Initializing jk_optimizers\n");

    /* trigger a cleanup process at the end */
    std::atexit(jk_optimizers::finit);

    /* setup variables */
    jkimyei_specs_pipeline = cuwacunu::camahjucunu::dsl::jkimyeiSpecsPipeline();

    /* configure to the defualt configutation */
    jk_optimizers::configure(
      cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_dsl()
    );
  }
  static void finit() {
    log_info("Finalizing jk_optimizers \n");
  }
  
  /* Methods */
  static void configure(const std::string &instruction) {
    jkimyei_specs_instruction = jkimyei_specs_pipeline.decode(instruction);
  }
/* --- --- --- --- --- --- --- --- */
/* --- --- - Optimizers  - --- --- */
/* --- --- --- --- --- --- --- --- */
  /* SGD */
  std::unique_ptr<torch::optim::Optimizer> SDG(torch::nn::Module& model, std::string& row_id) {
    auto __ = cuwacunu::camahjucunu::jkimyeiSpecsConfAccess(jkimyei_specs_instruction, OPTIMZIERS_TABLE_NAME, row_id);
    auto options = torch::optim::SGDOptions(
      __<double>("initial_learning_rate"))
      .momentum     (__<double>("momentum"))
      .dampening    (__<double>("dampening"))
      .weight_decay (__<double>("weight_decay"))
      .nesterov     (__<bool>("nesterov"));
    return std::make_unique<torch::optim::SGD>(model.parameters(), options);
  }
  /* Adam */
  std::unique_ptr<torch::optim::Optimizer> Adam(torch::nn::Module& model, std::string& row_id) {
    auto __ = cuwacunu::camahjucunu::jkimyeiSpecsConfAccess(jkimyei_specs_instruction, OPTIMZIERS_TABLE_NAME, row_id);
    auto options = torch::optim::AdamOptions(
      __<double>("initial_learning_rate"))
      .betas        ({__<double>("beta1"), __<double>("beta2")})
      .eps          (__<double>("eps"))
      .weight_decay (__<double>("weight_decay"))
      .amsgrad      (__<bool>("amsgrad"));
    return std::make_unique<torch::optim::Adam>(model.parameters(), options);
  }
  /* RMSprop */
  std::unique_ptr<torch::optim::Optimizer> RMSprop(torch::nn::Module& model, std::string& row_id) {
    auto __ = cuwacunu::camahjucunu::jkimyeiSpecsConfAccess(jkimyei_specs_instruction, OPTIMZIERS_TABLE_NAME, row_id);
    auto options = torch::optim::RMSpropOptions(
      __<double>("initial_learning_rate"))
      .alpha        (__<double>("alpha"))
      .eps          (__<double>("eps"))
      .weight_decay (__<double>("weight_decay"))
      .momentum     (__<double>("momentum"))
      .centered     (__<bool>("centered"));
    return std::make_unique<torch::optim::RMSprop>(model.parameters(), options);
  }
  /* Adagrad */
  std::unique_ptr<torch::optim::Optimizer> Adagrad(torch::nn::Module& model, std::string& row_id) {
    auto __ = cuwacunu::camahjucunu::jkimyeiSpecsConfAccess(jkimyei_specs_instruction, OPTIMZIERS_TABLE_NAME, row_id);
    auto options = torch::optim::AdagradOptions(
      __<double>("initial_learning_rate"))
      .lr_decay     (__<double>("lr_decay"))
      .weight_decay (__<double>("weight_decay"))
      .eps          (__<double>("eps"));
    return std::make_unique<torch::optim::Adagrad>(model.parameters(), options);
  }
}

/* initialize static members */
cuwacunu::camahjucunu::dsl::jkimyeiSpecsPipeline jk_optimizers::jkimyei_specs_pipeline;
cuwacunu::camahjucunu::jkimyei_specs_t jk_optimizers::jkimyei_specs_instruction;

/* initialize static class */
jk_optimizers::_init jk_optimizers::_initializer;

} /* namespace cuwacunu */
} /* namespace jkimyei */
