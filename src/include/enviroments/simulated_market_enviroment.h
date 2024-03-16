#pragma once
#include "../dtypes.h"
#include "../dconfig.h"
#include "simulated_broker.h"

namespace cuwacunu {
class Environment {
public:
  std::vector<mechanic_order_t> mech_buff;            /* mechanical orders vector */
  instrument_v_t<position_space_t> portafolio;        /* vector of all instruments */
  instrument_v_t<position_space_t> past_portafolio;   /* past state of all instruments */
  float total_cap;
  size_t state_size = 5 * COUNT_INSTSRUMENTS;         /* state space dimenstionality */
  size_t action_dim = 2 * COUNT_INSTSRUMENTS + 4;     /* action space dimentionality */
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
  void mechinze_order(action_logits_t& action_features) {
    /* there is construction in the struct that goes from action_features to action_space_t */
    action_space_t act(action_features);
    /* append to the enviroment order buffer */
    mech_buff.push_back(
      mechanic_order_t(act, act.target_amount(portafolio))
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
  reward_space_t get_step_reward() { // #FIXME determine if the rewards are too small or are causing problem due to scale
    instrument_v_t<float> reward_per_instrument = {};
    FOR_ALL_INSTRUMENTS(inst) {
      reward_per_instrument[inst] = (portafolio[inst].capital() - past_portafolio[inst].capital());
    }
    estimate_total_capital(); // #FIXME include total_cap as a overall multipler in the rewards
    past_portafolio = portafolio;
    
    return reward_space_t(reward_per_instrument);
  }
  state_space_t current_state_features() {
    instrument_v_t<state_features_t> instrument_state_feat
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
    return state_space_t(instrument_state_feat);
  }
  bool is_done() {
    return (total_capital < BANKRUPTCY_CAPITAL) || Broker::get_step_count() > MAX_EPISODE_STEPS;
  }
  cuwacunu::experience_t step(action_logits_t& action_features) {
    
    experience_t exp = {};
    /* forward the input state */     exp.state_feat = current_state_features();
    /* forward the input action */    exp.action_feat = action_features;
    /* interpret the action */        mechinze_order(exp.action_feat);
    /* step the enviroment --- execute the action */                exchange_mechanic_orders();
    /* step the enviroment --- request the broker price update */   Broker::step();
    /* forward the step state */      exp.next_state_feat = current_state_features();
    /* forward the step reward */     exp.reward = get_step_reward();
    /* query the episode end */       exp.done = is_done();

    return exp;
  }
};
}
