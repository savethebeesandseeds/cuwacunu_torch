#pragma once
#include "piaabo/dutils.h"

namespace cuwacunu {
/* experience_space_t */
struct experience_space_t {
private: /* to inforce architecture design, only friend classes have access */
  /* state */           state_space_t state;      /* states, there is tensor for each instrument */
  /* action */          action_space_t action;    /* actions, there is tensor for all instruments */
  /* next_state */      state_space_t next_state; /* next_state has the same shape as state */
  /* reward */          reward_space_t reward;    /* there is one reward per instrument */
  /* done */            bool done;                /* mark the episode end */
  /* training metrics*/ learn_space_t learn;      /* training metrics */
  experience_space_t(const state_space_t& state, const action_space_t& action, const state_space_t& next_state, 
    const reward_space_t& reward, bool done, const learn_space_t& learn);
};
using episode_experience_space_t = std::vector<experience_space_t>;
// struct experienceBatch_t {
//   /* states_features */       std::vector<state_space_t>   states_features;
//   /* actions_features */      std::vector<action_space_t>  actions_features;
//   /* next_states_features */  std::vector<state_space_t>   next_states_features;
//   /* rewards */               std::vector<reward_space_t>  rewards;
//   /* dones */                 std::vector<bool>            dones;
//   experienceBatch_t() = default;  
//   void append(experience_space_t exp) {
//     states_features.push_back(exp.state);
//     actions_features.push_back(exp.action);
//     next_states_features.push_back(exp.next_state);
//     rewards.push_back(exp.reward);
//     dones.push_back(exp.done);
//   }
// };
} /* namespace cuwacunu */