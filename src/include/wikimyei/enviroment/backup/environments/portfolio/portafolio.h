#pragma once
#include "dtypes.h"
#include "dconfig.h"
... inherit from abstract
namespace cuwacunu {
class Environment {
public:
  std::vector<mechanic_order_t> mech_buff;            /* mechanical orders vector */
  instrument_v_t<position_space_t> portfolio;        /* vector of all instruments */
  instrument_v_t<position_space_t> past_portfolio;   /* past state of all instruments */
  float total_cap;
  size_t state_size = 5 * COUNT_INSTRUMENTS;         /* state space dimenstionality */
  size_t action_dim = 2 * COUNT_INSTRUMENTS + 4;     /* action space dimentionality */
  Environment();
  virtual ~Environment();
  state_space_t reset();
  float estimate_total_capital();
  void mechinze_order(action_space_t& act);
  void exchange_mechanic_orders();
  reward_space_t get_step_reward();
  state_space_t current_state_features();
  bool is_done();
  cuwacunu::experience_space_t step(action_space_t& action);
};
} /* namespace cuwacunu */


