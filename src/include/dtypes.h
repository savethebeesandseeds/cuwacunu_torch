#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <torch/torch.h>
#include "torch_compat/distributions.h"

// ...#FIXME be aware of the random number generator seed
namespace cuwacunu {
extern torch::Device kDevice;
extern torch::Dtype kType;
typedef enum instrument {
  CONST,
  SINE,
  COUNT_INSTRUMENTS
} instrument_t;
/* instrument vector, to signifiy that the vector has elements for each instrument_t */
template<typename T>
using instrument_v_t = std::vector<T>;
static const std::string CURRENCY_STRING[] = {
	std::string("CONST"),
	std::string("SINE")
};
static const instrument_v_t<torch::Tensor> CURRENCY_TOKENIZER = {
  torch::tensor({1, 0}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice)), // CONST
  torch::tensor({0, 1}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice))  // SINE
};
/* statistics_t */
/* #FIXME statistics for actual charts involve dt */
struct statistics_t {
  unsigned long ctx = 0;  /* Number of data points */
  float c_max = std::numeric_limits<float>::lowest();   /* Max value */
  float c_min = std::numeric_limits<float>::max();      /* Min value */
  float c_mean = 0;   /* Running mean */
  float c_S = 0;      /* Running variance * (n-1), this is for the calculation of the sample variance */
  /* Constructor declaration */
  statistics_t(float initial_value);
  void update(float x);
  float mean()            const;
  float variance()        const;
  float stddev()          const;
  float max()             const;
  float min()             const;
  unsigned long count()   const;
};
/* currency_space_t */
struct currency_space_t {
  instrument_t symb;    /* Currency identifier */
  torch::Tensor token;  /* Tokenization of currency */
  torch::Tensor price;  /* price in ABSOLUTE_BASE_SYMB */
  statistics_t stats;   /* Welford's method statistics of the price */
  // ... RSI
  // ... MACD
  /* Constructor declaration */
  currency_space_t(instrument_t symb, float price);
};
/* position_space_t */
struct position_space_t {
  instrument_t symb;  /* Currency identifier */
  float amount;       /* Quantity held of the currency */
  float capital();    /* capital in ABSOLUTE_BASE_SYMB */
  /* Constructor declaration */
  position_space_t(instrument_t symb, float amount = 0);
};
/* wallet_space_t */
// struct wallet_space_t {
//   std::vector<position_space_t> portafolio;   //  |   stakes in all aviable instruments
//   float total_value;       //  |   sum over all positions amount converted to ABSOLUTE_BASE_SYMB
// };
struct action_space_t;
struct action_logits_t;
/* action_logits_t */
struct action_logits_t {
  torch::Tensor base_symb_categorical_logits;  /* logits of a categorical distribution */
  torch::Tensor target_symb_categorical_logits;/* logits of a categorical distribution */
  torch::Tensor alpha_values;       /* alpha in a beta distribition */
  torch::Tensor beta_values;        /* beta in a beta distribition */
  /* Constructor declaration */
  action_logits_t() = default;
  action_logits_t(torch::Tensor base_symb_categorical_logits, torch::Tensor target_symb_categorical_logits, torch::Tensor alpha_values, torch::Tensor beta_values);
  /* clone the struct tensors and detach them from the computational graph */
  action_logits_t clone_detached() const;
  /* distributions utils */
  Categorical base_symb_dist();
  Categorical target_symb_dist();
  Beta confidence_dist();
  Beta urgency_dist();
  Beta threshold_dist();
  Beta delta_dist();
  /* utility function for action_space_t: get the base and target symbols */
  static void symbs_from_logits(action_logits_t& logits, action_space_t& action);
  /* utility function for action_space_t: get confidence */
  static void confidence_from_logits(action_logits_t& logits, action_space_t& action);
  /* utility function for action_space_t: get urgency */
  static void urgency_from_logits(action_logits_t& logits, action_space_t& action);
  /* utility function for action_space_t: get threshold */
  static void threshold_from_logits(action_logits_t& logits, action_space_t& action);
  /* utility function for action_space_t: get delta */
  static void delta_from_logits(action_logits_t& logits, action_space_t& action);
};
/* action_space_t */
struct action_space_t {
  action_logits_t logits;     // |   action_logits_t    | Detached copy of the action logits
  instrument_t base_symb;     // |   instrument_t         | Currency identifierfor the base symbol
  instrument_t target_symb;   // |   instrument_t         | Currency identifier for the target symbol
  float confidence;           // |   interval([0, 1])     | Confidence that an order will close
  float urgency;              // |   interval([0, 1])     | Importance of closing the order
  float threshold;            // |   interval([-10, 10])  | Activation value to close the order, amount of standar deviations (in base_symb) from the mean => close_at = threshold * std + mean
  float delta;                // |   interval([-1, 1])    | (negative) sell, (positive) buy, amount of shares to be executed in the transaction ince threshold price is reach      
  /* Constructor declaration */
  action_space_t(const action_logits_t& input_logits);
  /* target amount is the amount of shares in the target currency */
  float target_amount(float amount);
  float target_amount(instrument_v_t<position_space_t> portafolio);
  /* target price is the price of target in base symb terms */
  float target_price();
};
/* state_space_t */
using state_features_t = torch::Tensor;
struct state_space_t {
  instrument_v_t<state_features_t> instruments_state_feat; /* state features per instrument */
  /* Constructor declaration */
  state_space_t(instrument_v_t<state_features_t> instruments_state_feat);
  /* Unpack method to convert vector of tensors into a single tensor */
  torch::Tensor unpack() const;
};
using reward_feature_t = float;
/* reward_space_t */
struct reward_space_t {
  instrument_v_t<reward_feature_t> instrument_reward; /* rewatd per instrument */
  float evaluate_reward() const;
  /* Constructor declaration */
  reward_space_t(instrument_v_t<reward_feature_t> instrument_reward);
};
/* order_space_t */
struct order_space_t {      /* there is no concept of BUY or SELL, it depends on the base and target symbols */
  instrument_t base_symb;   /*  |   instrument_t      | the currency of the holding capital to be converted into target_symb, once the order is liquidated */
  instrument_t target_symb; /*  |   instrument_t      | the type of coin that will convert the base_symb into, once the order is liquidated */
  float target_price;       /*  |   interval([0, inf])   | close settlement price (in target_symb/base_symb) */
  float target_amount;      /*  |   interval([0, inf])   | amount of shares to be bought of target_symb  */
  bool liquidated;          /*  |   bool              | flag to indicate if the order has been fulfiled */
  /* Constructor declaration */
  order_space_t(instrument_t base_symb, instrument_t target_symb, float target_price = INFINITY, float target_amount = 0, bool liquidated = false);
};
/* mechanic_order_t */
struct mechanic_order_t {
  action_space_t action;  /* action space, comming from the base policy network */
  order_space_t order;    /* order space, transforms the action space into an executable instruction*/
  /* Constructor declaration */
  mechanic_order_t(action_space_t& action, float target_amount);
};
/* enviroment_event_t */
// struct enviroment_event_t { // bring upon when LSTM is intrduced
//   bool mask;
//   enviroment_event_t(instrument_t base_symb, instrument_t target_symb, float confidence, float threshold, float urgency, float delta, bool mask)
//   :   mech_o(base_symb, target_symb, confidence, threshold, urgency, delta), 
//     mask(mask){}
// };
/* learn_space_t */
struct learn_space_t {
  torch::Tensor current_value;
  torch::Tensor next_value;
  torch::Tensor expected_value;
  torch::Tensor critic_losses;
  torch::Tensor actor_categorical_losse;
  torch::Tensor actor_continuous_losse;
};
/* experience_t */
struct experience_t {
  /* state */           state_space_t state;      /* states, there is tensor for each instrument */
  /* action */          action_space_t action;    /* actions, there is tensor for all instruments */
  /* next_state */      state_space_t next_state; /* next_state has the same shape as state */
  /* reward */          reward_space_t reward;    /* there is one reward per instrument */
  /* done */            bool done;                /* mark the episode end */
  /* training metrics*/ learn_space_t learn;      /* training metrics */
  experience_t(const state_space_t& state, const action_space_t& action, const state_space_t& next_state, 
    const reward_space_t& reward, bool done, const learn_space_t& learn);
};
using episode_experience_t = std::vector<experience_t>;
// struct experienceBatch_t {
//   /* states_features */       std::vector<state_space_t>   states_features;
//   /* actions_features */      std::vector<action_space_t>  actions_features;
//   /* next_states_features */  std::vector<state_space_t>   next_states_features;
//   /* rewards */               std::vector<reward_space_t>  rewards;
//   /* dones */                 std::vector<bool>            dones;
//   experienceBatch_t() = default;  
//   void append(experience_t exp) {
//     states_features.push_back(exp.state);
//     actions_features.push_back(exp.action);
//     next_states_features.push_back(exp.next_state);
//     rewards.push_back(exp.reward);
//     dones.push_back(exp.done);
//   }
// };
} /* namcespace cuwacunu */
