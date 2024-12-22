#include <vector>
#include <torch/torch.h>

namespace cuwacunu {
class AbstractEnvironment {
public:
  AbstractEnvironment() {}
  virtual ~AbstractEnvironment() const = 0; /* Pure virtual destructor */

  virtual reward_space_t get_step_reward() const = 0;
  virtual state_space_t current_state_features() const = 0;
  virtual state_space_t reset() const = 0;
  virtual bool is_done() const = 0;
  virtual experience_space_t step(action_space_t& action) const = 0;
};
/* Provide an implementation for the pure virtual destructor */
AbstractEnvironment::~AbstractEnvironment() {}
} /* namespace cuwacunu */