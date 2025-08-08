#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include <torch/torch.h>
#include "torch_compat/torch_utils.h"
#include "iinuji/instrument_space.h"
#include "torch_compat/distributions/beta.h"
#include "torch_compat/distributions/categorical.h"


action space needs to be refactored, now we persuit differentials on the portfolio
namespace cuwacunu {
namespace iinuji {
struct action_space_t;
struct action_logits_t;
/* action_logits_t */
struct action_logits_t {
public: /* to inforce architecture design, only friend classes have access */
  /* Logits of Distributions */
  torch::Tensor base_symb_categorical_logits;   /* logits of a categorical distribution */
  torch::Tensor target_symb_categorical_logits; /* logits of a categorical distribution */
  torch::Tensor alpha_values;                   /* alpha in a beta distribition */
  torch::Tensor beta_values;                    /* beta in a beta distribition */
  /* Distributions */
  torch_compat::distributions::Categorical base_symb_dist;
  torch_compat::distributions::Categorical target_symb_dist;
  torch_compat::distributions::Beta confidence_dist;
  torch_compat::distributions::Beta urgency_dist;
  torch_compat::distributions::Beta threshold_dist;
  torch_compat::distributions::Beta delta_dist;
  /* Usual Constructor */
  action_logits_t(
    const torch::Tensor base_symb_categorical_logits, 
    const torch::Tensor target_symb_categorical_logits, 
    const torch::Tensor alpha_values, 
    const torch::Tensor beta_values);
  /* Clone Constructor (detach them from the computational graph) */
  action_logits_t action_logits_t::clone_detached() const;
  /* Move Constructor (attached to the computational graph) */
  action_logits_t(action_logits_t&& src) noexcept;
  /* Distribution sample functions */
  instrument_e sample_base_symb();   /* get base_symb */
  instrument_e sample_target_symb(const instrument_e base_symb); /* get target_symb */
  float sample_confidence();      /* get confidence */
  float sample_urgency();         /* get urgency */
  float sample_threshold();       /* get threshold */
  float sample_delta();           /* get delta */
};
ENFORCE_ARCHITECTURE_DESIGN(action_logits_t);
/* action_space_t */
struct action_space_t {
public: /* to inforce architecture design, only friend classes have access */
  action_logits_t logits;     // |   action_logits_t      | action logits, or actor network output
  quote_space_t quote;        // |   quote_space_t        | base and target symbols
  float confidence;           // |   interval([0, 1])     | Confidence that an order will close
  float urgency;              // |   interval([0, 1])     | Importance of closing the order
  float threshold;            // |   interval([-10, 10])  | Activation value to close the order, amount of standar deviations (in base_symb) from the mean => close_at = threshold * std + mean
  float delta;                // |   interval([-1, 1])    | (negative) sell, (positive) buy, amount of shares to be executed in the transaction ince threshold price is reach      
  /* Usual Constructor */
  action_space_t(const action_logits_t& input_logits);
  /* Clone Constructor */
  action_space_t(const action_space_t& src);
  /* target amount is the amount of shares in the target currency */
  float target_amount(float amount);
  float target_amount(instrument_v_t<position_space_t> portfolio);
  /* target price is the price of target in base symb terms */
  float target_price();
};
ENFORCE_ARCHITECTURE_DESIGN(action_space_t);
} /* namespace iinuji */
} /* namespace cuwacunu */