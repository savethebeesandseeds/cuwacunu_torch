#include "simulated_market_enviroment.h"
#include "simulated_broker.h"
#include "piaabo/dutils.h"

RUNTIME_WARNING("(abstract_enviroment.cpp)[] #FIXME change floats to double. \n");

namespace cuwacunu {

Environment::Environment()  { reset(); }
Environment::~Environment() { reset(); }
state_space_t Environment::reset()   {
  /* Reset the broker           */    Broker::reset();
  /* reset the portafolio       */    portafolio.clear();
  /* initialize the portafolio, allocate initial capital in ABSOLUTE_BASE_SYMB */
  FOR_ALL_INSTRUMENTS(inst) { 
    portafolio.push_back(
      position_space_t(inst, inst == ABSOLUTE_BASE_SYMB ? INITIAL_CAPITAL : 0)
    );
  }
  past_portafolio = portafolio;
  /* reset the orders buffer      */  mech_buff.clear();
  /* estimate instrument capital  */  estimate_total_capital();
  return Environment::current_state_features();
}
float Environment::estimate_total_capital() { /* in ABSOLUTE_BASE_SYMB */
  /* initializze the return variable */
  total_cap = 0;
  /* sum over all positions in ABSOLUTE_BASE_SYMB */ 
  FOR_ALL_INSTRUMENTS(inst) {
    total_cap += portafolio[inst].capital();
  }
  return total_cap;
}
void Environment::mechinze_order(action_space_t& act) {
  /* append to the enviroment order buffer */
  mech_buff.push_back(
    mechanic_order_t(act, act.target_amount(portafolio))
  );
}
void Environment::exchange_mechanic_orders() {
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
reward_space_t Environment::get_step_reward() { // #FIXME determine if the rewards are too small or are causing problem due to scale
  instrument_v_t<float> instruments_reward;
  FOR_ALL_INSTRUMENTS(inst) {
    instruments_reward.push_back(portafolio[inst].capital() - past_portafolio[inst].capital());
  }
  estimate_total_capital(); // #FIXME include total_cap as a overall multipler in the rewards
  past_portafolio = portafolio;
  
  return reward_space_t(instruments_reward);
}
state_space_t Environment::current_state_features() {
  instrument_v_t<state_features_t> instruments_state_feat;
  /* Assuming you have a predefined number of instruments */
  FOR_ALL_INSTRUMENTS(inst) {
    auto aux = torch::tensor({
      Broker::get_current_price(inst), 
      Broker::get_current_mean(inst), 
      Broker::get_current_std(inst), 
      Broker::get_current_max(inst), 
      Broker::get_current_min(inst)
    }, cuwacunu::kType).to(cuwacunu::kDevice);
    /* Collect state for each instrument */
    instruments_state_feat.push_back(aux);
  }
  /* Convert the vector of tensors into a single tensor */
  return state_space_t(instruments_state_feat);
}
bool Environment::is_done() {
  return (Environment::estimate_total_capital() < BANKRUPTCY_CAPITAL) || Broker::get_step_count() > MAX_EPISODE_STEPS;
}
experience_space_t Environment::step(action_space_t& action) {
  /* forward the input state      */  state_space_t state(current_state_features());
  {/* step events */
    /* interpret the action                                     */  mechinze_order(action);
    /* step the enviroment --- execute the action               */  exchange_mechanic_orders();
    /* step the enviroment --- request the broker price update  */  Broker::step();
  }
  /* forward the step state       */  state_space_t next_state(current_state_features());
  /* forward the step reward      */  reward_space_t reward(get_step_reward());
  /* query the episode end        */  bool done(is_done());
  /* init the learning space      */  learn_space_t learn;
  
  return experience_space_t(state, action, next_state, reward, done, learn);
}

} /* namespace cuwacunu */
