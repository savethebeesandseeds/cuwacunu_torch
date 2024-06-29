#include <vector>
#include <torch/torch.h>

namespace cuwacunu {
class Environment {
public:
  std::vector<mechanic_order_t> mech_buff;           /* mechanical orders vector */
  instrument_v_t<position_space_t> portafolio;       /* vector of all instruments */
  instrument_v_t<position_space_t> past_portafolio;  /* past state of all instruments */
  float total_cap;
  size_t state_size = 5 * COUNT_INSTRUMENTS;         /* state space dimensionality */
  size_t action_dim = 2 * COUNT_INSTRUMENTS + 4;     /* action space dimensionality */
  
  Environment() {}
  virtual ~Environment() = 0; /* Pure virtual destructor */

  virtual state_space_t reset() = 0;
  virtual float estimate_total_capital() = 0;
  virtual void mechinze_order(action_space_t& act) = 0;
  virtual void exchange_mechanic_orders() = 0;
  virtual reward_space_t get_step_reward() = 0;
  virtual state_space_t current_state_features() = 0;
  virtual bool is_done() = 0;
  virtual experience_space_t step(action_space_t& action) = 0;
};
/* Provide an implementation for the pure virtual destructor */
Environment::~Environment() {}
} /* namespace cuwacunu */