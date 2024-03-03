#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <torch/torch.h>

namespace cuwacunu {

torch::Device kDevice = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
torch::dtype kType = torch::dtype(torch::kFloat32);
typedef enum instrument {
  CONST,
  SINE,
  COUNT_INSTSRUMENTS
} instrument_t;
/* instrument vector, to signifiy that the vector has elements for each instrument_t */
template<typename T>
using instrument_v_t = std::vector<T>;

static const std::string const CURRENCY_STRING[] = {
	std::string("CONST"),
	std::string("SINE")
};
static const std::vector<torch::Tensor> CURRENCY_TOKENIZER = {...#FIXME revisite
  torch::tensor({1, 0}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice)), // CONST
  torch::tensor({0, 1}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice))  // SINE
};
// Forward declare the Broker class
class Broker;
float Broker::get_current_price(instrument_t symb);
float Broker::get_current_price(instrument_t inst);
float Broker::get_current_mean (instrument_t inst);
float Broker::get_current_std  (instrument_t inst);
float Broker::get_current_max  (instrument_t inst);
float Broker::get_current_min  (instrument_t inst);
float Broker::exchange_rate (instrument_t base_symb, instrument_t target_symb);
float Broker::get_current_price(instrument_t target_symb, instrument_t base_symb);
float Broker::get_step_count   ();

struct statistics_t;

struct currency_space_t {
  instrument_t symb;    // Currency identifier
  torch::Tensor token;  // Tokenization of currency
  torch::Tensor price;  // price in ABSOLUTE_BASE_SYMB
  statistics_t stats; // Welford's method statistics of the price
  // ... RSI
  // ... MACD
  currency_space_t(instrument_t symb, float price = Broker::get_current_price(symb)) // default constructor
  : symb(symb), 
    token(CURRENCY_TOKENIZER[static_cast<int>(symb)]),
    price(torch::tensor({price}, cuwacunu::kType.device(cuwacunu::kDevice))),
    stats(statistics_t(price)) {}
};

struct position_space_t {
  instrument_t symb;  /* Currency identifier */
  float amount;    /* Quantity held of the currency */
  float capital() {   /* capital in ABSOLUTE_BASE_SYMB */
    return amount * Broker::get_current_price(symb); 
  }
  /* default consturctor */
  position_space_t(instrument_t symb, float amount = 0)
  : symb(symb), 
    amount(amount) {}
};
// struct wallet_space_t {
//   std::vector<position_space_t> portafolio;   //  |   stakes in all aviable instruments
//   float total_value;       //  |   sum over all positions amount converted to ABSOLUTE_BASE_SYMB
// };
struct action_space_t {
  instrument_t base_symb;     // |   instrument_t     | Currency identifier
  instrument_t target_symb;   // |   instrument_t     | Currency identifier for the result
  float confidence;           // |   interval([0, 1])     | Confidence that an order will close
  float urgency;              // |   interval([0, 1])     | Importance of closing the order
  float threshold;            // |   interval([-10, 10]) | Activation value to close the order, amount of standar deviations (in base_symb) from the mean => close_at = threshold * std + mean
  float delta;                // |   interval([-1, 1])   | (negative) sell, (positive) buy, amount of shares to be executed in the transaction ince threshold price is reach    
  /* target amount are the amount of shares in the target currency */
  float target_amount(float amount) {
    return (delta * amount) * Broker::exchange_rate(base_symb, target_symb);
  }
  float target_amount(instrument_v_t<position_space_t> portafolio) {
    return target_amount(portafolio[base_symb].amount);
  }
  action_space_t(instrument_t base_symb, instrument_t target_symb, float confidence, float threshold, float urgency, float delta) // default constructor
  : base_symb(base_symb),
    target_symb(target_symb),
    confidence(torch::tensor({confidence}, cuwacunu::kType.device(cuwacunu::kDevice))),
    urgency(torch::tensor({urgency}, cuwacunu::kType.device(cuwacunu::kDevice))),
    threshold(torch::tensor({threshold}, cuwacunu::kType.device(cuwacunu::kDevice))),
    delta(torch::tensor({delta}, cuwacunu::kType.device(cuwacunu::kDevice))) 
    { if (base_symb == target_symb) { log_warn("[action_space_t] base_symb and target_symb cannot be the same"); }}
  /* action_space_t can get constructed from a Tensor of size */
  action_space_t(torch::Tensor feat)
  : action_space_t(
    /* base_symb    */  (instrument_t)feat.slice(0, 4, 4 + COUNT_INSTSRUMENTS).argmax().item<int64_t>(),
    /* target_symb  */  (instrument_t)feat.slice(0, 4 + COUNT_INSTSRUMENTS, 4 + 2 * COUNT_INSTSRUMENTS).argmax().item<int64_t>(),
    /* confidence   */  feat[0].item<float>(),
    /* urgency      */  feat[1].item<float>(),
    /* threshold    */  feat[2].item<float>(),
    /* delta        */  feat[3].item<float>()
  ) {}
};
struct order_space_t {      /* there is no concept of BUY or SELL, it depends on the base and target symbols */
  instrument_t base_symb;   /*  |   instrument_t      | the currency of the holding capital to be converted into target_symb, once the order is liquidated */
  instrument_t target_symb; /*  |   instrument_t      | the type of coin that will convert the base_symb into, once the order is liquidated */
  float target_price;       /*  |   interval([0, inf])   | close settlement price (in target_symb/base_symb) */
  float target_amount;      /*  |   interval([0, inf])   | amount of shares to be bought of target_symb  */
  bool liquidated;          /*  |   bool              | flag to indicate if the order has been fulfiled */
  order_space_t(instrument_t base_symb, instrument_t target_symb, float target_price = INFINITY, float target_amount = 0, bool liquidated = false)
  : base_symb(base_symb),
    target_symb(target_symb),
    target_price(target_price),
    target_amount(target_amount),
    liquidated(liquidated)
    { if (base_symb == target_symb) { log_warn("[order_space_t] base_symb and target_symb cannot be the same"); }}
};
struct mechanic_order_t {
  action_space_t action;  /* action space, comming from the base policy network */
  order_space_t order;    /* order space, transforms the action space into an executable instruction*/
  mechanic_order_t(action_space_t& action, float target_amount) 
  : action(action),
    order(
    /* base_symb      */  action.base_symb,
    /* target_symb    */  action.target_symb,
    /* target_price   */  action.threshold * Broker::get_current_std(action.base_symb) + Broker::get_current_mean(action.base_symb),
    /* target_amount  */  target_amount,
    /* liquidated     */  false
    ) {}
};
// struct enviroment_event_t { // bring upon when LSTM is intrduced
//   bool mask;
//   enviroment_event_t(instrument_t base_symb, instrument_t target_symb, float confidence, float threshold, float urgency, float delta, bool mask)
//   :   mech_o(base_symb, target_symb, confidence, threshold, urgency, delta), 
//     mask(mask){}
// };
// struct state_space_t {
//   std::vector<mechanic_order_t> mech_buff;
//   wallet_t wallet;   // finantial status
// };
struct experience_t {
  instrument_v_t<torch::Tensor> state_features;       /* states, there is tensor for each instrument */
  torch::Tensor action_features;                      /* actions, there is tensor for all instruments */
  instrument_v_t<torch::Tensor> next_state_features;  /* next_state has the same shape as state */
  instrument_v_t<float> reward;                       /* there is one reward per instrument */
  bool done;
};
struct experienceBatch_t {
  std::vector<instrument_v_t<torch::Tensor>> states_features;
  std::vector<torch::Tensor> actions_features;
  std::vector<instrument_v_t<torch::Tensor>> next_states_features;
  std::vector<instrument_v_t<float>> rewards;
  std::vector<bool> dones;
};
}
