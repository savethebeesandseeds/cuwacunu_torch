#include <actor.h>

namespace cuwacunu {
ActorModelImpl::ActorModelImpl(int64_t state_size)
  : base_embedding(register_module("base_embedding", torch::nn::Linear(state_size, 128))),
    base_activation(register_module("base_activation", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)))),
    categorical_head(register_module("categorical_head", torch::nn::Linear(128, 2 * COUNT_INSTRUMENTS))),
    continuous_base_embedding(register_module("continuous_base_embedding", torch::nn::Linear(128, 128))),
    continuous_base_activation(register_module("continuous_base_activation", torch::nn::LeakyReLU(torch::nn::LeakyReLUOptions().negative_slope(0.01)))),
    continuous_alpha_head(register_module("continuous_alpha_head", torch::nn::Linear(128, _action_dim))), /* 4 is: {confidence, urgency, threshold, delta} */
    continuous_alpha_activation(register_module("continuous_alpha_activation", torch::nn::Sigmoid())), 
    continuous_beta_head(register_module("continuous_beta_head", torch::nn::Linear(128, _action_dim))), /* 4 is: {confidence, urgency, threshold, delta} */
    continuous_beta_activation(register_module("continuous_beta_activation", torch::nn::Sigmoid())) {
      // ...reset_memory() { // needed for bidirectional recurrent layers
      validate_module_parameters(*this);
      this->to(cuwacunu::kDevice);
}

action_logits_t ActorModelImpl::forward(const torch::Tensor x) {
  auto base_features = base_activation(base_embedding->forward(x));

  /* Generating logits for categorical parts */
  auto categorical_logits = categorical_head->forward(base_features);

  /* separete for base symbol and target symbol */
  auto base_symb_categorical_logits = categorical_logits.slice(0, 0, COUNT_INSTRUMENTS);
  auto target_symb_categorical_logits = categorical_logits.slice(0, COUNT_INSTRUMENTS, 2 * COUNT_INSTRUMENTS);

  /* Generating logits for continious parts */
  auto continuous_features = continuous_base_activation(continuous_base_embedding->forward(base_features));
  auto alpha_values = 100 * continuous_alpha_activation(0.05 * continuous_alpha_head->forward(continuous_features)) + 1e-4;
  auto beta_values = 100 * continuous_beta_activation(0.05 * continuous_beta_head->forward(continuous_features)) + 1e-4;

  return {base_symb_categorical_logits, target_symb_categorical_logits, alpha_values, beta_values};
}

cuwacunu::action_space_t ActorModelImpl::selectAction(cuwacunu::state_space_t state, bool explore) {
  // /* Applying mask to categorical logits to ensure invalid actions are not selected */
  // categorical_logits = categorical_logits.masked_fill_(categorical_mask, -1e9);
  // ...
  // ...  #FIXME add exploration
  // ...  #FIXME entropy regularization
  action_logits_t logits = forward(state.unpack());
  return action_space_t(logits);
}
void ActorModelImpl::reset_memory() /* needed for BiLSTM */ {}

} /* namespace cuwacunu */
