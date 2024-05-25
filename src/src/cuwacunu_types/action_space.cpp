#include "cuwacunu_types/action_space.h"

RUNTIME_WARNING("(action_space.cpp)[action_logits_t] #FIXME the whole action_space_t could use a better topological base. \n");
RUNTIME_WARNING("(action_space.cpp)[action_space_t] #FIXME the way target_amount and target_price are set could be reviewd. \n");

namespace cuwacunu {
/* action_logits_t */
action_logits_t::action_logits_t(
  torch::Tensor base_symb_categorical_logits, 
  torch::Tensor target_symb_categorical_logits, 
  torch::Tensor alpha_values, 
  torch::Tensor beta_values)
  : base_symb_categorical_logits(base_symb_categorical_logits), 
    target_symb_categorical_logits(target_symb_categorical_logits), 
    alpha_values(alpha_values), 
    beta_values(beta_values) {
      /* validate all the input tensors */
      validate_tensor(base_symb_categorical_logits, "[action_logits_t] ---base_symb_categorical_logits.");
      validate_tensor(target_symb_categorical_logits, "[action_logits_t] ---target_symb_categorical_logits.");
      validate_tensor(alpha_values, "[action_logits_t] ---alpha_values.");
      validate_tensor(beta_values, "[action_logits_t] ---beta_values.");
      /* Validate the base_symb and target_symb sizes */
      assert_tensor_shape(alpha_values, (size_t) COUNT_INSTRUMENTS, "[action_logits_t] ---base_symb_categorical_logits.");
      assert_tensor_shape(alpha_values, (size_t) COUNT_INSTRUMENTS, "[action_logits_t] ---target_symb_categorical_logits.");
      /* Validate the aplha and beta sizes */
      assert_tensor_shape(alpha_values, 4, "[action_logits_t] ---alpha_values.");
      assert_tensor_shape(beta_values, 4, "[action_logits_t] ---beta_values.");
      /* Fabric the distribitions */
      base_symb_dist    = torch_compat::distributions::Categorical(cuwacunu::kDevice, cuwacunu::kType, base_symb_categorical_logits);
      target_symb_dist  = torch_compat::distributions::Categorical(cuwacunu::kDevice, cuwacunu::kType, target_symb_categorical_logits);
      confidence_dist   = torch_compat::distributions::Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[0], beta_values[0]);
      urgency_dist      = torch_compat::distributions::Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[1], beta_values[1]);
      threshold_dist    = torch_compat::distributions::Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[2], beta_values[2]);
      delta_dist        = torch_compat::distributions::Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[3], beta_values[3]);
    }
action_logits_t action_logits_t::clone_detached() const {
  log_warn("(action_space.cpp)[action_logits_t::clone_detached()] make sure you want to clone detached instead of using the move constructor.\n");
  torch::Tensor cloned_base_symb_categorical_logits = base_symb_categorical_logits.clone().detach();
  torch::Tensor cloned_target_symb_categorical_logits = target_symb_categorical_logits.clone().detach();
  torch::Tensor cloned_alpha_values = alpha_values.clone().detach();
  torch::Tensor cloned_beta_values = beta_values.clone().detach();
  return action_logits_t(
    cloned_base_symb_categorical_logits,
    cloned_target_symb_categorical_logits,
    cloned_alpha_values,
    cloned_beta_values);
}
action_logits_t::action_logits_t(action_logits_t&& src) noexcept
  : base_symb_categorical_logits(std::move(src.base_symb_categorical_logits)),
    target_symb_categorical_logits(std::move(src.target_symb_categorical_logits)),
    alpha_values(std::move(src.alpha_values)),
    beta_values(std::move(src.beta_values)),
    base_symb_dist(std::move(src.base_symb_dist)),
    target_symb_dist(std::move(src.target_symb_dist)),
    confidence_dist(std::move(src.confidence_dist)),
    urgency_dist(std::move(src.urgency_dist)),
    threshold_dist(std::move(src.threshold_dist)),
    delta_dist(std::move(src.delta_dist))
{}
instrument_t action_logits_t::sample_base_symb() {
  return base_symb_dist.sample().item<int64_t>();
}
instrument_t action_logits_t::sample_target_symb(const instrument_t base_symb) {
  /* mask the base_symb to have it's probability be zero */
  torch::Tensor mask = torch::ones(COUNT_INSTRUMENTS, cuwacunu::kType).to(cuwacunu::kDevice);
  mask[base_symb] = 0;
  return target_symb_dist.mask_sample(mask).item<int64_t>();
}
float action_logits_t::sample_confidence() {
  return confidence_dist.sample().item<float>();
}
float action_logits_t::sample_urgency() {
  return urgency_dist.sample().item<float>();
}
float action_logits_t::sample_threshold() {
  return threshold_dist.sample().item<float>() * 20.0 - 10.0;
}
float action_logits_t::sample_delta() {
  return delta_dist.sample().item<float>() * 2.0 - 1.0;
}
/* action_space_t */
action_space_t::action_space_t(const action_logits_t& input_logits) 
  : logits(std::move(input_logits)) {
  /*    base_symb   */  base_symb   = logits.sample_base_symb();
  /*    taget_symb  */  taget_symb  = logits.sample_taget_symb(base_symb); /* sample is conditioned to the base_symb to avoid them beeing the same */
  /*    confidence  */  confidence  = logits.sample_confidence();
  /*    urgency     */  urgency     = logits.sample_urgency();
  /*    threshold   */  threshold   = logits.sample_threshold();
  /*    delta       */  delta       = logits.sample_delta();
  if (base_symb == target_symb) { log_warn("[action_space_t] base_symb and target_symb shound't be the same.\n"); }
}
float action_space_t::target_amount(float amount) {
  return (delta * amount) * Broker::exchange_rate(base_symb, target_symb);
}
float action_space_t::target_amount(instrument_v_t<position_space_t> portafolio) {
  return target_amount(portafolio[base_symb].amount);
}
float action_space_t::target_price() {
  return threshold * Broker::get_current_std(base_symb) + Broker::get_current_mean(base_symb);
}
} /* namespace cuwacunu */