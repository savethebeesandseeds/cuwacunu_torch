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
  virtual ~Environment() const = 0; /* Pure virtual destructor */

  virtual float estimate_total_capital() const = 0;
  virtual void mechinze_order(action_space_t& act) const = 0;
  virtual void exchange_mechanic_orders() const = 0;
  virtual reward_space_t get_step_reward() const = 0;
  virtual state_space_t current_state_features() const = 0;
  virtual state_space_t reset() const = 0;
  virtual bool is_done() const = 0;
  virtual experience_space_t step(action_space_t& action) const = 0;
};
/* Provide an implementation for the pure virtual destructor */
Environment::~Environment() {}
} /* namespace cuwacunu */