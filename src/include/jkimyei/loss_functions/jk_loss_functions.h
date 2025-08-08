/* jk_loss_functions.h */
#pragma once

#include <torch/torch.h> 
#include <functional>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/training_pipeline/training_pipeline.h"

#define LOSSES_TABLE_NAME "loss_functions_table"

RUNTIME_WARNING("(jk_loss_functions)[] missing weights on some of the loss functions.\n");
RUNTIME_WARNING("(jk_loss_functions)[] missing ignore_index on some of the loss functions.\n");

namespace cuwacunu {
namespace jkimyei {

struct jk_loss_functions {
public:
  static cuwacunu::camahjucunu::BNF::trainingPipeline training_pipeline;
  static cuwacunu::camahjucunu::BNF::training_instruction_t training_instruction;

private:
  jk_loss_functions();
  ~jk_loss_functions();

  /* Enforce Singleton requirements */
  jk_loss_functions(const jk_loss_functions&) = delete;
  jk_loss_functions& operator=(const jk_loss_functions&) = delete;
  jk_loss_functions(jk_loss_functions&&) = delete;
  jk_loss_functions& operator=(jk_loss_functions&&) = delete;

public:
  /* Access point Constructor */
  static struct _init {
    public:_init(){jk_loss_functions::init();}
  } _initializer;

  static void init() {
    log_info("Initializing jk_loss_functions\n");
    std::atexit(jk_loss_functions::finit);

    training_pipeline = cuwacunu::camahjucunu::BNF::trainingPipeline();

    /* Configure to the default configuration */
    jk_loss_functions::configure(
      cuwacunu::piaabo::dconfig::config_space_t::training_pipeline_instruction()
    );
  }

  static void finit() {
    log_info("Finalizing jk_loss_functions \n");
  }

  static void configure(const std::string &instruction) {
    training_instruction = training_pipeline.decode(instruction);
  }

/* --- --- --- --- --- --- --- --- */
/* --- --- Loss functions  --- --- */
/* --- --- --- --- --- --- --- --- */

  /* CrossEntropyLoss */
  static std::unique_ptr<torch::nn::Module> CrossEntropy(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::CrossEntropyLossOptions()
      .reduction        (__<std::string>("reduction"))
      .weight           (__<std::vector<double>>("weight"))
      .ignore_index     (__<int64_t>("ignore_index"))
      .label_smoothing  (__<double>("label_smoothing"));

    return std::make_unique<torch::nn::CrossEntropyLoss>(options);
  }

  /* BinaryCrossEntropy -> BCEWithLogitsLoss */
  static std::unique_ptr<torch::nn::Module> BinaryCrossEntropy(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::BCEWithLogitsLossOptions()
      .reduction        (__<std::string>("reduction"))
      .weight           (__<std::vector<double>>("weight"))
      .pos_weight       (__<std::vector<double>>("pos_weight"));

    return std::make_unique<torch::nn::BCEWithLogitsLoss>(options);
  }

  /* MeanSquaredError -> MSELoss */
  static std::unique_ptr<torch::nn::Module> MeanSquaredError(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::MSELossOptions()
      .reduction        (__<std::string>("reduction"));

    return std::make_unique<torch::nn::MSELoss>(options);
  }

  /* Hinge -> HingeEmbeddingLoss */
  static std::unique_ptr<torch::nn::Module> Hinge(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::HingeEmbeddingLossOptions()
      .reduction        (__<std::string>("reduction"))
      .margin           (__<double>("margin"));

    return std::make_unique<torch::nn::HingeEmbeddingLoss>(options);
  }

  /* SmoothL1 */
  static std::unique_ptr<torch::nn::Module> SmoothL1(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::SmoothL1LossOptions()
      .reduction        (__<std::string>("reduction"))
      .beta             (__<double>("beta"));

    return std::make_unique<torch::nn::SmoothL1Loss>(options);
  }

  /* NLLLoss */
  static std::unique_ptr<torch::nn::Module> NLLLoss(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::NLLLossOptions()
      .reduction        (__<std::string>("reduction"))
      .weight           (__<std::vector<double>>("weight"))
      .ignore_index     (__<int64_t>("ignore_index"));

    return std::make_unique<torch::nn::NLLLoss>(options);
  }

  /* L1Loss */
  static std::unique_ptr<torch::nn::Module> L1Loss(std::string& row_id) {
    auto __ = trainingPipe_ConfAccess(LOSSES_TABLE_NAME, row_id);
    auto options = torch::nn::L1LossOptions()
      .reduction        (__<std::string>("reduction"));

    return std::make_unique<torch::nn::L1Loss>(options);
  }
};

/* initialize static members */
cuwacunu::camahjucunu::BNF::trainingPipeline jk_loss_functions::training_pipeline;
cuwacunu::camahjucunu::BNF::training_instruction_t jk_loss_functions::training_instruction;

/* initialize static class */
jk_loss_functions::_init jk_loss_functions::_initializer;

} /* namespace cuwacunu */
} /* namespace jkimyei */
