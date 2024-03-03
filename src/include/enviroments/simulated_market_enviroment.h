#pragma once
#include "../dtypes.h"
#include "../dconfig.h"
#include "simulated_broker.h"

namespace cuwacunu {
class Environment {
public:
  std::vector<mechanic_order_t> mech_buff;
  instrument_v_t<position_space_t> portafolio;
  instrument_v_t<position_space_t> past_portafolio;
  float total_cap;
  Environment()           { reset(); }
  virtual ~Environment()  { reset(); }
  torch::Tensor reset()   {
    /* Reset the broker   */        Broker::init();
    /* reset the portafolio  */     portafolio.clear();
    /* initialize the portafolio, allocate initial capital in ABSOLUTE_BASE_SYMB */
    FOR_ALL_INSTRUMENTS(inst) { 
      portafolio.push(
        position_space_t(inst, inst == ABSOLUTE_BASE_SYMB ? INITIAL_CAPITAL : 0)
      );
    }
    past_portafolio = portafolio;
    /* reset the orders buffer  */     mech_buff.clear();
    /* estimate instrument capital */  estimate_total_capital();
    return current_state_features();
  }
  void estimate_total_capital() { /* in ABSOLUTE_BASE_SYMB */
    /* initializze the return variable */
    total_cap = 0;
    /* sum over all positions in ABSOLUTE_BASE_SYMB */ 
    FOR_ALL_INSTRUMENTS(inst) {
      total_cap += portafolio[inst].capital();
    }
    return total_cap;
  }
  void mechinze_order(torch::Tensor& action_features) {
    /* there is construction in the struct that goes from action_features to action_space_t */
    action_space_t action(action_features);
    /* append to the enviroment order buffer */
    mech_buff.push_back(
      mechanic_order_t(action, action.target_amount(portafolio))
    );
  }
  void exchange_mechanic_orders() {
    /* request the broker to, if possible, settle all non liquidated orders */
    FOR_ALL(mech_buff, mech_o) {
      if(!mech_o.order.liquidated) {
        Broker::exchange(
          portafolio[mech_o.order.base_symb], 
          portafolio[mech_o.order.target_symb], 
          mech_o.order
        );
      }
    }
  }
  instrument_v_t<float> get_step_reward() { // #FIXME determine if the rewards are too small or are causing problem due to scale
    instrument_v_t<float> reward_per_instrument = {};
    FOR_ALL_INSTRUMENTS(inst) {
      reward_per_instrument[inst] = (portafolio[inst].capital() - past_portafolio[inst].capital());
    }
    estimate_total_capital(); // #FIXME include total_cap as a overall multipler in the rewards
    past_portafolio = portafolio;
    return reward_per_instrument;
  }
  instrument_v_t<torch::Tensor> current_state_features() {
    instrument_v_t<torch::Tensor> ret;
    /* Assuming you have a predefined number of instruments */
    FOR_ALL_INSTRUMENTS(inst) {
      /* Collect state for each instrument */
      ret.push_back(torch::tensor({
        Broker::get_current_price(inst), 
        Broker::get_current_mean(inst), 
        Broker::get_current_std(inst), 
        Broker::get_current_max(inst), 
        Broker::get_current_min(inst)
      }, cuwacunu::kType).to(cuwacunu::kDevice));
    }
    /* Convert the vector of tensors into a single tensor */
    return ret;
  }
  bool is_done() {
    return (total_capital < BANKRUPTCY_CAPITAL) || Broker::get_step_count() > MAX_EPISODE_STEPS;
  }
  cuwacunu::experience_t step(torch::Tensor& action_features) {
    experience_t exp = {};
    exp.state_features = current_state_features();
    exp.action_features = action_features;
    mechinze_order(exp.action_features);
    exchange_mechanic_orders();
    Broker::step(); /* request the broker for the next price update */
    exp.next_state_features = current_state_features();
    exp.reward = get_step_reward();
    exp.done = is_done();
    return exp;
  }
};
}
