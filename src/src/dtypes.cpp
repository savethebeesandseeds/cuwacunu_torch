#include "dconfig.h"
#include "dtypes.h"
#include "dutils.h"
#include "simulated_broker.h"

namespace cuwacunu {
torch::Device kDevice = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
torch::Dtype kType = torch::kFloat32;
/* statistics_t */
statistics_t::statistics_t(float initial_value) 
  : ctx(1),
    c_max(initial_value),
    c_min(initial_value),
    c_mean(initial_value),
    c_S(0) {}
void statistics_t::update(float x) { /* Welford's method */
  float old_mean = c_mean;
  ctx++;
  c_mean += (x - c_mean) / ctx;
  c_S += (x - old_mean) * (x - c_mean);
  c_max = MAX(c_max, x);
  c_min = MIN(c_min, x);
}
float statistics_t::mean()            const { return ctx > 0 ? c_mean : 0.0; }
float statistics_t::variance()        const { return (ctx > 1) ? c_S / (ctx - 1) : 0.0; }
float statistics_t::stddev()          const { return std::sqrt(variance()); }
float statistics_t::max()             const { return c_max; }
float statistics_t::min()             const { return c_min; }
unsigned long statistics_t::count()   const { return ctx; }
/* currency_space_t */
currency_space_t::currency_space_t(instrument_t symb, float price)
  : symb(symb), 
    token(CURRENCY_TOKENIZER[static_cast<int>(symb)]),
    price(torch::tensor({price}, torch::dtype(cuwacunu::kType).device(cuwacunu::kDevice))),
    stats(statistics_t(price)) {}
/* position_space_t */
float position_space_t::capital() {
  return amount * Broker::get_current_price(symb); 
}
position_space_t::position_space_t(instrument_t symb, float amount)
  : symb(symb), amount(amount) {}
/* action_logits_t */
action_logits_t::action_logits_t(torch::Tensor base_symb_categorical_logits, torch::Tensor target_symb_categorical_logits, torch::Tensor alpha_values, torch::Tensor beta_values)
  : base_symb_categorical_logits(base_symb_categorical_logits), 
    target_symb_categorical_logits(target_symb_categorical_logits), 
    alpha_values(alpha_values), 
    beta_values(beta_values) {}
action_logits_t action_logits_t::clone_detached() const {
  action_logits_t cloned;
  cloned.base_symb_categorical_logits = base_symb_categorical_logits.clone().detach();
  cloned.target_symb_categorical_logits = target_symb_categorical_logits.clone().detach();
  cloned.alpha_values = alpha_values.clone().detach();
  cloned.beta_values = beta_values.clone().detach();
  return cloned;
}
Categorical action_logits_t::base_symb_dist() {
  return Categorical(cuwacunu::kDevice, cuwacunu::kType, base_symb_categorical_logits);
}
Categorical action_logits_t::target_symb_dist() {
  return Categorical(cuwacunu::kDevice, cuwacunu::kType, target_symb_categorical_logits);
}
Beta action_logits_t::confidence_dist() {
  return Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[0], beta_values[0]);
}
Beta action_logits_t::urgency_dist() {
  return Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[1], beta_values[1]);
}
Beta action_logits_t::threshold_dist() {
  return Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[2], beta_values[2]);
}
Beta action_logits_t::delta_dist() {
  return Beta(cuwacunu::kDevice, cuwacunu::kType, alpha_values[3], beta_values[3]);
}
void action_logits_t::symbs_from_logits(action_logits_t& logits, action_space_t& action) {
  /* derivate the base_symb */
  action.base_symb = (instrument_t) logits.base_symb_dist().sample().item<int64_t>();
  /* induce a really low probability for the target to be the same as the base */
  logits.target_symb_categorical_logits[(size_t)action.base_symb] = -1e9;
  /* derivate the target_symb */
  action.target_symb = (instrument_t) logits.target_symb_dist().sample().item<int64_t>();
}
void action_logits_t::confidence_from_logits(action_logits_t& logits, action_space_t& action) {
  action.confidence = logits.confidence_dist().sample().item<float>();
}
void action_logits_t::urgency_from_logits(action_logits_t& logits, action_space_t& action) {
  action.urgency = logits.urgency_dist().sample().item<float>();
}
void action_logits_t::threshold_from_logits(action_logits_t& logits, action_space_t& action) {
  action.threshold = logits.threshold_dist().sample().item<float>() * 20.0 - 10.0;
}
void action_logits_t::delta_from_logits(action_logits_t& logits, action_space_t& action) {
  action.delta = logits.delta_dist().sample().item<float>() * 2.0 - 1.0;
}
/* action_space_t */
action_space_t::action_space_t(const action_logits_t& input_logits) 
  : logits(input_logits.clone_detached()) {
  /* {base,taget}_symb    */  action_logits_t::symbs_from_logits(logits, *this);
  /*      confidence      */  action_logits_t::confidence_from_logits(logits, *this);
  /*      urgency         */  action_logits_t::urgency_from_logits(logits, *this);
  /*      threshold       */  action_logits_t::threshold_from_logits(logits, *this);
  /*      delta           */  action_logits_t::delta_from_logits(logits, *this);
  if (base_symb == target_symb) { log_warn("[action_space_t] base_symb and target_symb shound't be the same"); }
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
/* state_space_t */
state_space_t::state_space_t(instrument_v_t<state_features_t> instruments_state_feat) 
  : instruments_state_feat(instruments_state_feat) {}
torch::Tensor state_space_t::unpack() const {
  return torch::cat(instruments_state_feat, 0);
}
/* reward_space_t */
reward_space_t::reward_space_t(instrument_v_t<reward_feature_t> instrument_reward) 
  : instrument_reward(instrument_reward) {}
float reward_space_t::evaluate_reward() const {
  /* retrive the actual reward from reward_space_t */
  log_warn("Fixme: Reward == 1.0 \n");
  return 1.0;
}
/* order_space_t */
order_space_t::order_space_t(instrument_t base_symb, instrument_t target_symb, float target_price, float target_amount, bool liquidated)
  : base_symb(base_symb),
    target_symb(target_symb),
    target_price(target_price),
    target_amount(target_amount),
    liquidated(liquidated)
    { if (base_symb == target_symb) { log_warn("[order_space_t] base_symb and target_symb cannot be the same"); }}
/* mechanic_order_t */
mechanic_order_t::mechanic_order_t(action_space_t& action, float target_amount) 
  : action(action),
    order(
    /* base_symb      */  action.base_symb,
    /* target_symb    */  action.target_symb,
    /* target_price   */  action.target_price(),
    /* target_amount  */  target_amount,        // #FIXME maybe avoid the argument and deduced
    /* liquidated     */  false) {}
/* learn_space_t */
/* experience_t */
experience_t::experience_t(const state_space_t& state, const action_space_t& action, const state_space_t& next_state, 
  const reward_space_t& reward, bool done, const learn_space_t& learn)
  : state(state),
    action(action),
    next_state(next_state),
    reward(reward),
    done(done),
    learn(learn) {}
} /* namcespace cuwacunu */



// #pragma once
// #include <iostream>
// #include <vector>
// #include <random>
// #include <string>
// #include <cmath>
// #include <torch/torch.h>

// // ...#FIXME be aware of the random number generator seed
// namespace cuwacunu {

// auto kDevice = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
// auto kType = torch::dtype(torch::kFloat32);
// typedef enum instrument {
//   CONST,
//   SINE,
//   COUNT_INSTRUMENTS
// } instrument_t;
// /* instrument vector, to signifiy that the vector has elements for each instrument_t */
// template<typename T>
// using instrument_v_t = std::vector<T>;

// static const std::string CURRENCY_STRING[] = {
// 	std::string("CONST"),
// 	std::string("SINE")
// };
// static const instrument_v_t<torch::Tensor> CURRENCY_TOKENIZER = {
//   torch::Tensor({1, 0}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice)), // CONST
//   torch::Tensor({0, 1}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice))  // SINE
// };
// // Forward declare the Broker class utilities
// class Broker;
// float Broker::get_current_price(instrument_t inst);
// float Broker::get_current_mean (instrument_t inst);
// float Broker::get_current_std  (instrument_t inst);
// float Broker::get_current_max  (instrument_t inst);
// float Broker::get_current_min  (instrument_t inst);
// float Broker::exchange_rate    (instrument_t base_symb, instrument_t target_symb);
// float Broker::get_current_price(instrument_t target_symb, instrument_t base_symb);
// float Broker::get_step_count   ();

// struct statistics_t { /* #FIXME statistics for actual charts involve dt */
//   unsigned long ctx = 0;  /* Number of data points */
//   float c_max = std::numeric_limits<float>::lowest();    /* Max value */
//   float c_min = std::numeric_limits<float>::max();     /* Min value */
//   float c_mean = 0;   /* Running mean */
//   float c_S = 0;     /* Running variance * (n-1), this is for the calculation of the sample variance */

//   /* Constructor to initialize the struct with an initial value */
//   statistics_t(float initial_value) 
//   :   ctx(1),
//     c_max(initial_value),
//     c_min(initial_value),
//     c_mean(initial_value),
//     c_S(0) {}

//   void update(float x) { /* Welford's method */
//     float old_mean = c_mean;
//     ctx++;
//     c_mean += (x - c_mean) / ctx;
//     c_S += (x - old_mean) * (x - c_mean);
//     c_max = MAX(c_max, x);
//     c_min = MIN(c_min, x);
//   }

//   float mean()    const { return ctx > 0 ? c_mean : 0.0; }
//   float variance()const { return (ctx > 1) ? c_S / (ctx - 1) : 0.0; }
//   float stddev()  const { return std::sqrt(variance()); }
//   float max()     const { return c_max; }
//   float min()     const { return c_min; }
//   unsigned long count()   const { return ctx; }
// };

// struct currency_space_t {
//   instrument_t symb;    // Currency identifier
//   torch::Tensor token;  // Tokenization of currency
//   torch::Tensor price;  // price in ABSOLUTE_BASE_SYMB
//   statistics_t stats; // Welford's method statistics of the price
//   // ... RSI
//   // ... MACD
//   currency_space_t(instrument_t symb, float price = Broker::get_current_price(symb)) // default constructor
//   : symb(symb), 
//     token(CURRENCY_TOKENIZER[static_cast<int>(symb)]),
//     price(torch::Tensor({price}, cuwacunu::kType.device(cuwacunu::kDevice))),
//     stats(statistics_t(price)) {}
// };

// struct position_space_t {
//   instrument_t symb;  /* Currency identifier */
//   float amount;       /* Quantity held of the currency */
//   float capital() {   /* capital in ABSOLUTE_BASE_SYMB */
//     return amount * Broker::get_current_price(symb); 
//   }
//   /* default consturctor */
//   position_space_t(instrument_t symb, float amount = 0)
//   : symb(symb), 
//     amount(amount) {}
// };
// // struct wallet_space_t {
// //   std::vector<position_space_t> portafolio;   //  |   stakes in all aviable instruments
// //   float total_value;       //  |   sum over all positions amount converted to ABSOLUTE_BASE_SYMB
// // };
// struct action_logits_t {
//   torch::Tensor base_symb_categorical_logits;  /* logits of a categorical distribution */
//   torch::Tensor target_symb_categorical_logits /* logits of a categorical distribution */
//   torch::Tensor alpha_values;       /* alpha in a beta distribition */
//   torch::Tensor beta_values;        /* beta in a beta distribition */
//   /* clone the struct tensors and detach them from the computational graph */
//   action_logits_t clone_detached() const {
//     action_logits_t cloned;
//     cloned.base_symb_categorical_logits = base_symb_categorical_logits.clone().detach();
//     cloned.target_symb_categorical_logits = target_symb_categorical_logits.clone().detach();
//     cloned.alpha_values = alpha_values.clone().detach();
//     cloned.beta_values = beta_values.clone().detach();
//     return cloned;
//   }
//   Categorical base_symb_dist() {
//     return Categorical(logits.base_symb_categorical_logits);
//   }
//   Categorical target_symb_dist() {
//     return Categorical(logits.target_symb_categorical_logits);
//   }
//   Beta confidence_dist() {
//     return Beta(logits.alpha_values[0], logits.beta_values[0]);
//   }
//   Beta urgency_dist() {
//     return Beta(logits.alpha_values[1], logits.beta_values[1]);
//   }
//   Beta threshold_dist() {
//     return Beta(logits.alpha_values[2], logits.beta_values[2]);
//   }
//   Beta delta_dist() {
//     return Beta(logits.alpha_values[3], logits.beta_values[3]);
//   }
//   /* utility function for action_space_t: get the base and target symbols */
//   static void symbs_from_logits(action_logits_t& logits, action_space_t& action) {
//     /* derivate the base_symb */
//     action.base_symb = (instrument_t) logits.base_symb_dist().sample().item<int64_t>();
//     /* induce a really low probability for the target to be the same as the base */
//     logits.target_categorical_logits[(size_t)action.base_symb] += -1e9;
//     /* derivate the target_symb */
//     action.target_symb = (instrument_t) logits.target_symb_dist().sample().item<int64_t>();
//   }
//   /* utility function for action_space_t: get confidence */
//   static void confidence_from_logits(action_logits_t& logits, action_space_t& action) {
//     action.confidence = logits.confidence_dist().sample().item<float>();
//   }
//   /* utility function for action_space_t: get urgency */
//   static void urgency_from_logits(action_logits_t& logits, action_space_t& action) {
//     action.urgency = logits.urgency_dist().sample().item<float>();
//   }
//   /* utility function for action_space_t: get threshold */
//   static void threshold_from_logits(action_logits_t& logits, action_space_t& action) {
//     action.threshold = logits.threshold_dist().sample().item<float>() * 20.0 - 10.0;
//   }
//   /* utility function for action_space_t: get delta */
//   static void delta_from_logits(action_logits_t& logits, action_space_t& action) {
//     action.delta_di = logits.delta_dist().sample().item<float>() * 2.0 - 1.0;
//   }
// };
// struct action_space_t {
//   action_logits_t logits;     // |   action_logits_t    | Detached copy of the action logits
//   instrument_t base_symb;     // |   instrument_t         | Currency identifier
//   instrument_t target_symb;   // |   instrument_t         | Currency identifier for the result
//   float confidence;           // |   interval([0, 1])     | Confidence that an order will close
//   float urgency;              // |   interval([0, 1])     | Importance of closing the order
//   float threshold;            // |   interval([-10, 10])  | Activation value to close the order, amount of standar deviations (in base_symb) from the mean => close_at = threshold * std + mean
//   float delta;                // |   interval([-1, 1])    | (negative) sell, (positive) buy, amount of shares to be executed in the transaction ince threshold price is reach      
//   /* default constructor */
//   action_space_t(const action_logits_t& input_logits) : feat(input_logits.clone_detached()) {
//     /* symb         */  action_logits_t::symbs_from_logits(logits, *this);
//     /* confidence   */  action_logits_t::confidence_from_logits(logits, *this);
//     /* urgency      */  action_logits_t::urgency_from_logits(logits, *this);
//     /* threshold    */  action_logits_t::threshold_from_logits(logits, *this);
//     /* delta        */  action_logits_t::delta_from_logits(logits, *this);
//     if (base_symb == target_symb) { log_warn("[action_space_t] base_symb and target_symb shound't be the same"); }
//   }
//   /* target amount is the amount of shares in the target currency */
//   float target_amount(float amount) {
//     return (delta * amount) * Broker::exchange_rate(base_symb, target_symb);
//   }
//   float target_amount(instrument_v_t<position_space_t> portafolio) {
//     return target_amount(portafolio[base_symb].amount);
//   }
//   /* target price is the price of target in base symb terms */
//   float target_price() {
//     return threshold * Broker::get_current_std(base_symb) + Broker::get_current_mean(base_symb);
//   }
// };
// using state_features_t = torch::Tensor;
// struct state_space_t {
//   instrument_v_t<state_features_t> instruments_state_feat; /* state features per instrument */
//   /* Constructor */
//   state_space_t(instrument_v_t<state_features_t> instruments_state_feat) {
//     instruments_state_feat(instruments_state_feat)
//   }
//   /* Unpack method to convert vector of tensors into a single tensor */
//   torch::Tensor unpack() {
//     return torch::cat(instruments_state_feat, 0);
//   }
// }
// using reward_feature_t = float;
// struct reward_space_t {
//   instrument_v_t<reward_feature_t> instrument_reward; /* rewatd per instrument */
//   reward_space_t(instrument_v_t<reward_feature_t> instrument_reward) {
//     instrument_reward(instrument_reward)
//   }
// }
// struct order_space_t {      /* there is no concept of BUY or SELL, it depends on the base and target symbols */
//   instrument_t base_symb;   /*  |   instrument_t      | the currency of the holding capital to be converted into target_symb, once the order is liquidated */
//   instrument_t target_symb; /*  |   instrument_t      | the type of coin that will convert the base_symb into, once the order is liquidated */
//   float target_price;       /*  |   interval([0, inf])   | close settlement price (in target_symb/base_symb) */
//   float target_amount;      /*  |   interval([0, inf])   | amount of shares to be bought of target_symb  */
//   bool liquidated;          /*  |   bool              | flag to indicate if the order has been fulfiled */
//   order_space_t(instrument_t base_symb, instrument_t target_symb, float target_price = INFINITY, float target_amount = 0, bool liquidated = false)
//   : base_symb(base_symb),
//     target_symb(target_symb),
//     target_price(target_price),
//     target_amount(target_amount),
//     liquidated(liquidated)
//     { if (base_symb == target_symb) { log_warn("[order_space_t] base_symb and target_symb cannot be the same"); }}
// };
// struct mechanic_order_t {
//   action_space_t action;  /* action space, comming from the base policy network */
//   order_space_t order;    /* order space, transforms the action space into an executable instruction*/
//   mechanic_order_t(action_space_t& action, float target_amount) 
//   : action(action),
//     order(
//     /* base_symb      */  action.base_symb,
//     /* target_symb    */  action.target_symb,
//     /* target_price   */  action.target_price(),
//     /* target_amount  */  target_amount,        // #FIXME maybe avoid the argument and deduced
//     /* liquidated     */  false
//     ) {}
// };
// // struct enviroment_event_t { // bring upon when LSTM is intrduced
// //   bool mask;
// //   enviroment_event_t(instrument_t base_symb, instrument_t target_symb, float confidence, float threshold, float urgency, float delta, bool mask)
// //   :   mech_o(base_symb, target_symb, confidence, threshold, urgency, delta), 
// //     mask(mask){}
// // };
// struct learn_space_t {
//   torch::Tensor current_value;
//   torch::Tensor next_value;
//   torch::Tensor expected_value;
//   torch::Tensor critic_losses;
//   torch::Tensor actor_categorical_losse;
//   torch::Tensor actor_continuous_losse;
// };
// struct experience_t {
//   /* state */           state_space_t state;      /* states, there is tensor for each instrument */
//   /* action */          action_space_t action;    /* actions, there is tensor for all instruments */
//   /* next_state */      state_space_t next_state; /* next_state has the same shape as state */
//   /* reward */          reward_space_t reward;    /* there is one reward per instrument */
//   /* done */            bool done;                /* mark the episode end */
//   /* training metrics*/ learn_space_t learn;      /* training metrics */
//   experience_t() = default;
// };
// using episode_experience_t = std::vector<cuwacunu::experience_t>;
// // struct experienceBatch_t {
// //   /* states_features */       std::vector<state_space_t>   states_features;
// //   /* actions_features */      std::vector<action_space_t>  actions_features;
// //   /* next_states_features */  std::vector<state_space_t>   next_states_features;
// //   /* rewards */               std::vector<reward_space_t>  rewards;
// //   /* dones */                 std::vector<bool>            dones;
// //   experienceBatch_t() = default;  
// //   void append(experience_t exp) {
// //     states_features.push_back(exp.state);
// //     actions_features.push_back(exp.action);
// //     next_states_features.push_back(exp.next_state);
// //     rewards.push_back(exp.reward);
// //     dones.push_back(exp.done);
// //   }
// // };
// }
