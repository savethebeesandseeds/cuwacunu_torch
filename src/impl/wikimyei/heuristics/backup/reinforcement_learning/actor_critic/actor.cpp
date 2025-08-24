#include <actor.h>

namespace cuwacunu {
ActorModelImpl::ActorModelImpl(int64_t state_size) {
    base_embedding = register_module("base_embedding", torch::nn::Linear(state_size, 128));
    base_activation = register_module("base_activation", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)));
    categorical_head = register_module("categorical_head", torch::nn::Linear(128, 2 * COUNT_INSTRUMENTS));
    continious_base_embedding = register_module("continious_base_embedding", torch::nn::Linear(128, 128));
    continious_base_activation = register_module("continious_base_activation", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)));
    continious_alpha_head = register_module("continious_alpha_head", torch::nn::Linear(128, _action_dim));
    continious_alpha_activation = register_module("continious_alpha_activation", torch::nn::Sigmoid());
    continious_beta_head = register_module("continious_beta_head", torch::nn::Linear(128, _action_dim));
    continious_beta_activation = register_module("continious_beta_activation", torch::nn::Sigmoid());

    validate_module_parameters(*this);
    this->to(cuwacunu::kDevice);
    // ...reset_memory() { // needed for bidirectional recurrent layers
}


action_logits_t ActorModelImpl::forward(const torch::Tensor& x) {
  auto base_features = base_activation(base_embedding->forward(x));

  /* Generating logits for categorical parts */
  auto categorical_logits = categorical_head->forward(base_features);

  /* separete for base symbol and target symbol */
  auto base_symb_categorical_logits = categorical_logits.slice(0, 0, COUNT_INSTRUMENTS);
  auto target_symb_categorical_logits = categorical_logits.slice(0, COUNT_INSTRUMENTS, 2 * COUNT_INSTRUMENTS);

  /* Generating logits for continious parts */
  auto continious_features = continious_base_activation(continious_base_embedding->forward(base_features));
  auto alpha_values = 100 * continious_alpha_activation(0.05 * continious_alpha_head->forward(continious_features)) + 1e-4;
  auto beta_values = 100 * continious_beta_activation(0.05 * continious_beta_head->forward(continious_features)) + 1e-4;

  return {base_symb_categorical_logits, target_symb_categorical_logits, alpha_values, beta_values};
}

RUNTIME_WARNING("(actor.cpp)[ActorModelImpl::selectAction] #FIXME add exploration\n");
RUNTIME_WARNING("(actor.cpp)[ActorModelImpl::selectAction] #FIXME entropy regularization\n");
cuwacunu::action_space_t ActorModelImpl::selectAction(cuwacunu::state_space_t& state, bool explore) {
  // /* Applying mask to categorical logits to ensure invalid actions are not selected */
  // categorical_logits = categorical_logits.masked_fill_(categorical_mask, -1e9);
  // ... 
  action_logits_t logits = forward(state.unpack());
  return action_space_t(logits);
}
void ActorModelImpl::reset_memory() /* needed for BiLSTM */ {}

} /* namespace cuwacunu */
