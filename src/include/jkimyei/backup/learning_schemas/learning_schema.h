/* learning_schema.h */
#pragma once

#include <torch/torch.h>
#include <memory>
#include <vector>
#include <string>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace jkimyei {

/**
 * @brief Abstract class for a learning schema
 *
 * @tparam Q The dataset type, representing the structure of data batches.
 * @tparam K The sample type returned by the dataset or dataloader.
 *
 * The template parameters Q and K define the interaction between the dataloader and the training step.
 * Q specifies the dataset structure, which is compatible with torch::data::Dataset, while K defines the structure
 * of individual samples processed during training and loss computation.
 */
template <typename Q, typename K>
class LearningSchema {
protected:
  using training_instruction_t    = cuwacunu::camahjucunu::training_instruction_t;
  using observation_instruction_t = cuwacunu::camahjucunu::observation_instruction_t;
  std::shared_ptr<torch::nn::Module> model_            = nullptr;
  std::shared_ptr<training_instruction_t> train_inst_  = nullptr;
  std::shared_ptr<observation_instruction_t> obs_inst_ = nullptr;
  std::unique_ptr<torch::optim::Optimizer> optimizer_  = nullptr;
  std::unique_ptr<torch::nn::Module> loss_function_    = nullptr;

public:
  /* Virtual destructor to ensure proper cleanup of derived classes */
  virtual ~LearningSchema() = default;

  /* Initialize the schema with model */
  virtual void initialize(std::shared_ptr<torch::nn::Module> model, training_instruction_t training_instruction, observation_instruction_t) {
    this->model_          = model;
    this->train_inst_     = training_instruction;
    torch::data::DataLoader<Q, K>& data_loader = ...;
    this->loss_function_  = select_loss_function();
    this->optimizer_      = initialize_optimizer();
  }

  /* Select the loss function */
  virtual std::unique_ptr<torch::nn::Module> select_loss_function() = 0;

  /* Select the optimizer */
  virtual std::shared_ptr<torch::optim::Optimizer> select_optimizer() = 0;

  /* Select learning rate scheduler */
  virtual std::unique_ptr<torch::optim::LRScheduler> select_lr_scheduler() = 0;

  /* Execute a single training step */
  virtual void train_step(const K& example) = 0;

  /* Train the model on the learning schema */
  virtual void train_loop() = 0;

  /* Provide a name for the schema */
  virtual std::string name() const = 0;

};

} /* namespace cuwacunu */
} /* namespace jkimyei */
